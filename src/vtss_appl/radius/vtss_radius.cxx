/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.
*/

#include "vtss_radius_api.h" /* For RADIUS types            */
#include "misc_api.h"        /* For misc_strncpyz()         */
#include "critd_api.h"       /* For semaphore wrapper       */
#include "msg_api.h"         /* For msg_switch_is_primary() */
#include "vtss_md5_api.h"    /* For MD5 computations.       */
#include "sysutil_api.h"     /* For VTSS_SYS_PASSWD_LEN     */
#include "vtss_os_wrapper_network.h"
#include "ip_utils.hxx"

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#else
/* Define dummy syslog macros */
#define S_W(fmt, ...)
#endif

/******************************************************************************/
// Naming conventions:
//   Public functions and variables : radius_xxx()
//   Private functions and variables: RADIUS_xxx()
/******************************************************************************/

// This is how much we can handle upon reception from RADIUS server
#define RADIUS_MAX_FRAMED_MTU_SIZE_BYTES 1344

// Thread is called every 100 msecs if we're the primary switch. This is used to
// check for RADIUS Rx.
#define RADIUS_THREAD_TIMEOUT_MS 100

#define RADIUS_ADDR_PORT_SIZE (INET6_ADDRSTRLEN + 20) /* IP address and port string length */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_RADIUS
/****************************************************************************/
// Trace definitions
/****************************************************************************/
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_RADIUS
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_RADIUS
#include <vtss_trace_api.h>

/* Trace registration. Initialized by vtss_radius_init() */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "radius", "RADIUS module"
};

#ifndef RADIUS_DEFAULT_TRACE_LVL
#define RADIUS_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif

#define T_LW(_fmt, ...)                                                  \
    do {                                                                 \
      static vtss_tick_count_t prev_time = 0;                            \
      if (vtss_current_time() - prev_time > VTSS_OS_MSEC2TICK(10000)) {  \
        T_W(_fmt, ##__VA_ARGS__);                                        \
        prev_time = vtss_current_time();                                 \
      }                                                                  \
    } while (0)

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        RADIUS_DEFAULT_TRACE_LVL
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/******************************************************************************/
// Semaphore stuff.
/******************************************************************************/
static critd_t RADIUS_crit;

// Macros for accessing semaphore functions
// -----------------------------------------
#define RADIUS_CRIT_ENTER()         critd_enter(        &RADIUS_crit, __FILE__, __LINE__)
#define RADIUS_CRIT_EXIT()          critd_exit(         &RADIUS_crit, __FILE__, __LINE__)
#define RADIUS_CRIT_ASSERT_LOCKED() critd_assert_locked(&RADIUS_crit, __FILE__, __LINE__)

#define RADIUS_PKT_IDX_CODE           0 /* Size  1 */
#define RADIUS_PKT_IDX_ID             1 /* Size  1 */
#define RADIUS_PKT_IDX_LEN            2 /* Size  2 */
#define RADIUS_PKT_IDX_AUTHENTICATOR  4 /* Size 16 */
#define RADIUS_PKT_IDX_FIRST_ATTRIB  20 /* Unknown */
#define RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR_LEN 18 /* Including Type and Len */

/******************************************************************************/
// Thread variables
/******************************************************************************/
static vtss_handle_t RADIUS_thread_handle;
static vtss_thread_t RADIUS_thread_state;  // Contains space for the scheduler to hold the current thread state.

/******************************************************************************/
// ...and finally some useful declarations :-)
/******************************************************************************/

// We can have at most 256 outstanding RADIUS requests at a time, since the ID
// in the RADIUS packets is an 8-bit number.
// In order to support RADIUS-based user-logon, 802.1X, and MAC-based
// authentication (especially the latter), it could easily happen that more than
// 256 clients would like to authenticate at the same time - especially if
// running in a stack that just booted. Therefore, we make provisions to not
// re-use an ID in case it is already in use.

// This enum identifies the type of the RADIUS server: authentication or accounting.
typedef enum {
    RADIUS_SERVER_TYPE_AUTH, // This is an authentication server
    RADIUS_SERVER_TYPE_ACCT, // This is an accounting server
    RADIUS_SERVER_TYPE_LAST  // Use this when defining arrays
} RADIUS_server_type_e;

// This is the internal state structure we use to store info about who's
// currently using a RADIUS identifier, timeouts, server, etc.
typedef struct {
    u8                         *tx_frm;                        // Tx frame buffer
    vtss_radius_rx_callback_f  *callback;                      // Pointer to user's Rx callback.
    vtss_tick_count_t          tx_time_ticks;                  // Time (in ticks) for when the @tx_frm was transmitted last.
    ctx_t                      ctx;                            // User's context as specified in vtss_radius_tx()
    BOOL                       in_use;                         // Is this ID free for use or already in use by someone?
    BOOL                       waiting_for_callback_to_finish; // Flag to check that the handle was not re-allocated by another RADIUS request during a callback.
    u8                         *tlv_ptr;                       // Points either to a TLV in @tx_frm or RADIUS_rx_buffer depending on where we are in the flow.
    RADIUS_server_type_e       server_type;                    // Authentication or accounting
    int                        server_idx;                     // Only valid if @in_use is TRUE. -2 = Not filled in yet, -1 = Ready to be transmitted the first time, 0 .. #servers-1 = transmitted to this server last.
    u8                         user_pw[VTSS_SYS_PASSWD_LEN];   // Unfortunately, we need to save a copy of the user-password (when not using EAP) due to possible server-change.
    int                        retries_left;                   // Number of retries left for this server.
    int                        use_this_server_idx_only;       // If >= 0, this is the server index to use for this frame.
    BOOL                       force_next_server;              // If server responds with ECONNREFUSED or similar (because RADIUS Auth/Acct port is not open), go to next server ASAP rather than retrying.
} RADIUS_id_state_s;

typedef struct {
    struct vtss_MD5Context shared_key_context_1;
    struct vtss_MD5Context shared_key_context_2;
} RADIUS_md5_cache_s;

static RADIUS_md5_cache_s RADIUS_md5_cache[VTSS_RADIUS_NUMBER_OF_SERVERS];

typedef struct {
    vtss_tick_count_t       next_time_to_try_this_server_ticks; // Absolute time when it is ok to contact this server again
    vtss_tick_count_t       timeout_per_retransmit_ticks;       // Number of ticks per retransmit of a frame.
    int                     sockd;                              // The socket used for RADIUS communication
    struct sockaddr_storage src_addr;                           // IP address of outgoing interface.
    struct sockaddr_storage dst_addr;                           // RADIUS server IP address from DNS lookup.

    // This identifies this server uniquely.It allows clients of
    // this RADIUS module to ask for the same server in a conversation that
    // spans multiple RADIUS packets. If we didn't have this ability, and
    // two servers were defined and e.g. the first didn't reply, we would go
    // on to the next. If this one works, the remaining of the authentication
    // must be done with that. The client will call vtss_radius_alloc()
    // for each and every frame in the same conversation, and if he couldn't
    // ask for a specific server, server #1 would be retried over and over again
    // if dead time for it has expired (or is 0). This causes the authentication
    // to take a loooong time, and in case of 802.1X, the supplicant may timeout
    // by itself and send a new EAPoL Start frame, which causes the state machine
    // to start over (this may happen anyway, if the supplicant timeout is lower
    // than the total server timeout for server #1).
    // 0 means that it is not in use.
    u32                     server_id;
} RADIUS_server_state_s;

typedef struct {
    BOOL                      initialized;                          // When initialized, we've opened the relevant sockets.
    int                       active_server_cnt;                    // Number of configured and enabled servers.
    RADIUS_server_state_s     state[VTSS_RADIUS_NUMBER_OF_SERVERS]; // State for each server
    vtss_radius_server_info_t *config;                              // Pointer to current configuration
} RADIUS_server_info_s;

// When testing, decrease the RADIUS_ID_CNT with RADIUS_dbg_cmd_cfg_max_id_cnt().
#define RADIUS_MAX_ID_CNT 256

// Internal state.
typedef struct {
    // Each possible RADIUS ID must have its own state:
    RADIUS_id_state_s id_states[RADIUS_MAX_ID_CNT];

    // We keep the RADIUS_MAX_ID_CNT in a variable to allow the users of this API
    // to change it (through RADIUS_dbg_cmd_cfg_max_id_cnt()), so that they can test
    // that his function works even when all IDs are currently in use (to limit
    // the number of required outstanding RADIUS frames before saturating).
    u8 max_id_cnt_minus_one;

    // The next ID to be used is this one.
    u8 next_valid_id;

    // Number of IDs currently in use.
    int ids_in_use;

    // We cannot do anything as long as we haven't been configured.
    // This is taken care of by another module.
    BOOL configured;

    // Only call srand() once.
    BOOL srand_initialized;

    // Event flag that is used to wake up the RADIUS_thread().
    vtss_flag_t thread_control_flag;

    // State information for all servers
    RADIUS_server_info_s server_info[RADIUS_SERVER_TYPE_LAST];

    // Number of ticks a given server is considered 'dead'.
    vtss_tick_count_t dead_time_per_server_ticks;

    // This is a monotonically increasing number that may be used by client of
    // this RADIUS module to ask for the same server in a conversation that
    // spans multiple RADIUS packets. If we didn't have this ability, and
    // two servers were defined and e.g. the first didn't reply, we would go
    // on to the next. If this one works, the remaining of the authentication
    // must be done with that. The client will call vtss_radius_alloc()
    // for each and every frame in the same conversation, and if he couldn't
    // ask for a specific server, server #1 would be retried over and over again
    // if dead time for it has expired (or is 0). This causes the authentication
    // to take a loooong time, and in case of 802.1X, the supplicant may timeout
    // by itself and send a new EAPoL Start frame, which causes the state machine
    // to start over (this may happen anyway, if the supplicant timeout is lower
    // than the total server timeout for server #1).
    // unique_server_id > 0. 0 is not used (unless it wraps around), but since
    // it only counts when the administrator changes the RADIUS configuration,
    // we don't bother with that.
    u32 unique_server_id;
} RADIUS_state_s;

// Our local state.
static RADIUS_state_s RADIUS_state;

// Our local copy of the configuration.
static vtss_radius_cfg_s RADIUS_cfg;
static vtss_radius_cfg_s RADIUS_tmp_cfg; // Used when transferring from user-thread to RADIUS thread.

// Temporary to transport other cfg from user-thread to RADIUS thread
static u8 RADIUS_tmp_max_id_cnt_minus_one;

// Authentication Client MIB counters.
static vtss_radius_auth_client_mib_s RADIUS_auth_client_mib;

// Accounting Client MIB counters.
static vtss_radius_acct_client_mib_s RADIUS_acct_client_mib;

/****************************************************************************/
// Flag values for waking up thread.
/****************************************************************************/
typedef enum {
    // The next two ones must come first:
    RADIUS_THREAD_FLAG_POS_CFG_CHANGED        = 0x01, /**< Gets set by a configuration change */
    RADIUS_THREAD_FLAG_POS_MAX_ID_CNT_CHANGED = 0x02, /**< Gets set by a debug configuration change */
    RADIUS_THREAD_FLAG_POS_WAKE_UP_NOW        = 0x04, /**< Gets set when a new RADIUS frame is to be transmitted */
} RADIUS_thread_flag_pos_e;

// We need to provide an interface to the users to allow them to know when
// the RADIUS module is initialized.

#define RADIUS_INIT_CHG_REGISTRANT_MAX_CNT 2
typedef struct {
    ctx_t                            ctx;       // User-specified context
    vtss_radius_init_chg_callback_f *callback;  // User's callback function
} RADIUS_init_chg_registrant_s;

static RADIUS_init_chg_registrant_s RADIUS_init_chg_registrants[RADIUS_INIT_CHG_REGISTRANT_MAX_CNT];

#define RADIUS_ID_STATE_CHG_REGISTRANT_MAX_CNT 1
typedef struct {
    ctx_t                                  ctx;       // User-specified context
    vtss_radius_id_state_chg_callback_f *callback;  // User's callback function
} RADIUS_id_state_chg_registrant_s;

static RADIUS_id_state_chg_registrant_s RADIUS_id_state_chg_registrants[RADIUS_ID_STATE_CHG_REGISTRANT_MAX_CNT];

// Packet buffer for in-coming radius frames
static u8 RADIUS_rx_buffer[RADIUS_MAX_FRAME_SIZE_BYTES];

/******************************************************************************/
//
//  PRIVATE FUNCTIONS
//
/******************************************************************************/

/******************************************************************************/
// RADIUS_auth_client_mib_clr()
/******************************************************************************/
static void RADIUS_auth_client_mib_clr(int idx)
{
    RADIUS_CRIT_ASSERT_LOCKED();

    if (idx >= ARRSZ(RADIUS_cfg.servers_auth)) {
        idx = -1;
    }
    if (idx == -1) {
        // Unconditionally clear the whole structure.
        memset(&RADIUS_auth_client_mib, 0, sizeof(RADIUS_auth_client_mib));
    } else {
        // Remember the pending count, since this is used internally in this module.
        // And we can only get here as a result of a user-clearing.
        u32 PendingRequests = RADIUS_auth_client_mib.radiusAuthServerExtTable[idx].radiusAuthClientExtPendingRequests;
        memset(&RADIUS_auth_client_mib.radiusAuthServerExtTable[idx], 0, sizeof(RADIUS_auth_client_mib.radiusAuthServerExtTable[idx]));
        RADIUS_auth_client_mib.radiusAuthServerExtTable[idx].radiusAuthClientExtPendingRequests = PendingRequests;
    }
}

/******************************************************************************/
// RADIUS_acct_client_mib_clr()
/******************************************************************************/
static void RADIUS_acct_client_mib_clr(int idx)
{
    RADIUS_CRIT_ASSERT_LOCKED();

    if (idx >= ARRSZ(RADIUS_cfg.servers_acct)) {
        idx = -1;
    }
    if (idx == -1) {
        // Unconditionally clear the whole structure.
        memset(&RADIUS_acct_client_mib, 0, sizeof(RADIUS_acct_client_mib));
    } else {
        // Remember the pending count, since this is used internally in this module.
        // And we can only get here as a result of a user-clearing.
        u32 PendingRequests = RADIUS_acct_client_mib.radiusAccServerExtTable[idx].radiusAccClientExtPendingRequests;
        memset(&RADIUS_acct_client_mib.radiusAccServerExtTable[idx], 0, sizeof(RADIUS_acct_client_mib.radiusAccServerExtTable[idx]));
        RADIUS_acct_client_mib.radiusAccServerExtTable[idx].radiusAccClientExtPendingRequests = PendingRequests;
    }
}

/****************************************************************************/
// RADIUS_srand()
/****************************************************************************/
static void RADIUS_srand(void)
{
    uint32_t usecs = static_cast<uint32_t>(hal_time_get());
    srand(usecs);
}

/******************************************************************************/
// RADIUS_sockaddr_to_string()
// Convert a struct sockaddr address to a string
/******************************************************************************/
static char *RADIUS_sockaddr_to_string(const struct sockaddr *sa, char *s, size_t maxlen)
{
    char ts[RADIUS_ADDR_PORT_SIZE];

    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        (void)snprintf(s, maxlen, "%s %d", inet_ntop(AF_INET, (char *)&sin->sin_addr, ts, sizeof(ts)), ntohs(sin->sin_port));
        s[maxlen - 1] = 0;
        break;
    }
#ifdef VTSS_SW_OPTION_IPV6
    case AF_INET6: {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
        (void)snprintf(s, maxlen, "%s %d", inet_ntop(AF_INET6, (char *)&sin6->sin6_addr, ts, sizeof(ts)), ntohs(sin6->sin6_port));
        s[maxlen - 1] = 0;
        break;
    }
#endif
    default:
        strncpy(s, "Unknown Address Family", maxlen);
        return NULL;
    }
    return s;
}

static mesa_rc RADIUS_check_intf_ip_ready(const int ai_family, const char *host)
{
    vtss_appl_ip_if_key_ipv4_t    ipv4_key = {};
    vtss_appl_ip_if_key_ipv6_t    ipv6_key = {};
    vtss_appl_ip_if_status_link_t link;

    if (ai_family == AF_INET) {
        // Find IPv4 address on available interface
        while (vtss_appl_ip_if_status_ipv4_itr(&ipv4_key, &ipv4_key) == VTSS_RC_OK) {
            if (vtss_appl_ip_if_status_link_get(ipv4_key.ifindex, &link) == VTSS_RC_OK) {
                if (link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) {
                    return VTSS_RC_OK;
                }
            }
        }
    } else if (ai_family == AF_INET6) {
        // Find IPv6 address on available interface
        while (vtss_appl_ip_if_status_ipv6_itr(&ipv6_key, &ipv6_key) == VTSS_RC_OK) {
            if (vtss_appl_ip_if_status_link_get(ipv6_key.ifindex, &link) == VTSS_RC_OK) {
                if (link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) {
                    return VTSS_RC_OK;
                }
            }
        }
    }

    return VTSS_RC_ERROR;
}

/******************************************************************************/
// RADIUS_create_socket()
// If a compatible socket does not exists then create a socket that is compatible
// (IPv4 or IPv6) with the specified host else reuse existing socket.
// Returns VTSS_RC_OK, VTSS_UNSPECIFIED_ERROR or VTSS_RADIUS_ERROR_NO_NAME.
/******************************************************************************/
static mesa_rc RADIUS_create_socket(RADIUS_server_state_s *state, const char *host, ushort port)
{
    struct addrinfo  hints;
    char             port_str[16];
    int              error, intf_ip_ready = -1;
    struct addrinfo *result, *rp;
    VTSS_ASSERT(host);
    (void)snprintf(port_str, sizeof(port_str), "%d", port);

    // Setup hints structure
    memset(&hints, 0, sizeof(hints));
#ifdef VTSS_SW_OPTION_IPV6
    hints.ai_family = AF_UNSPEC; // Accept both IPv4 and IPv6
#else
    hints.ai_family = AF_INET;   // Accept IPv4 only
#endif
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_ADDRCONFIG; // Only if any address is assigned

    if ((error = getaddrinfo(host, port_str, &hints, &result))) {
        // No error or warning message here. This can happen if the IP stack isn't ready yet
        T_D("getaddrinfo %s:%d failed (%d, %s)", host, port, error, gai_strerror(error));
        if (state->sockd >= 0) {
            close(state->sockd);
            state->sockd = -1;
        }
        return VTSS_RADIUS_ERROR_NO_NAME; // Always return NO_NAME here
    }

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_NOISE) /* Compile check */
    if (TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_NOISE)) { /* Runtime check */
        // Dump all result entries
        int num = 1;
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            char ip_str[RADIUS_ADDR_PORT_SIZE];
            T_N("addr %d = %s", num, RADIUS_sockaddr_to_string(rp->ai_addr, ip_str, sizeof(ip_str)));
            num++;
        }
    }
#endif

    if (state->sockd >= 0) {
        // Socket exists. Verify it.
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            if (memcmp(&state->dst_addr, rp->ai_addr, rp->ai_addrlen) == 0) {
                if (connect(state->sockd, rp->ai_addr, rp->ai_addrlen) == 0) {
                    socklen_t addr_len = sizeof(state->src_addr);
                    if (getsockname(state->sockd, (struct sockaddr *)&state->src_addr, &addr_len) == 0) {
                        T_D("Reusing valid IP address");
                        break; // Success
                    } else {
                        T_LW("getsockname() failed: %s", strerror(errno));
                        S_W("getsockname() failed: %s", strerror(errno));
                    }
                } else {
                    T_LW("connect() failed: %s", strerror(errno));
                    S_W("connect() failed: %s", strerror(errno));
                }
            }
        }

        if (rp == NULL) { // No address succeeded
            T_D("No valid IP address - Close and reopen socket");
            close(state->sockd);
            state->sockd = -1;
        }
    }

    if (state->sockd < 0) {
        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully connect().
        // If socket() (or connect()) fails, we (close the socket
        // and) try the next address. */
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            state->sockd = vtss_socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (state->sockd == -1) {
                continue;
            }

            /* Check IP ready, if yes, create socket to connect */
            intf_ip_ready = RADIUS_check_intf_ip_ready(rp->ai_family, host);

            if (intf_ip_ready == VTSS_RC_OK) {
                if (connect(state->sockd, rp->ai_addr, rp->ai_addrlen) == 0) {
                    memcpy(&state->dst_addr, rp->ai_addr, rp->ai_addrlen);
                    socklen_t addr_len = sizeof(state->src_addr);
                    if (getsockname(state->sockd, (struct sockaddr *)&state->src_addr, &addr_len) == 0) {
                        T_D("Connected to new IP address");
                        break; // Success
                    } else {
                        T_LW("getsockname() failed: %s", strerror(errno));
                        S_W("getsockname() failed: %s", strerror(errno));
                    }
                } else {
                    T_LW("connect() failed: %s", strerror(errno));
                    S_W("connect() failed: %s", strerror(errno));
                }
            }
            close(state->sockd);
            state->sockd = -1;
        }

        if (intf_ip_ready == VTSS_RC_OK && rp == NULL) { // No address succeeded
            T_LW("Error opening RADIUS socket (%d, %s)", errno, strerror(errno));
            S_W("Error opening RADIUS socket (%d, %s)", errno, strerror(errno));
        }
    }
    freeaddrinfo(result);
    return (state->sockd >= 0) ? VTSS_RC_OK : VTSS_UNSPECIFIED_ERROR;
}

/******************************************************************************/
// RADIUS_call_init_chg_callbacks()
// Calls all registrants that need to know whether RADIUS is initialized or not.
/******************************************************************************/
static void RADIUS_call_init_chg_callbacks(vtss_radius_init_chg_callback_result_e res)
{
    int i;
    T_D("Initialization status = %d", res);
    RADIUS_CRIT_ASSERT_LOCKED();
    for (i = 0; i < ARRSZ(RADIUS_init_chg_registrants); i++) {
        if (RADIUS_init_chg_registrants[i].callback != NULL) {
            RADIUS_init_chg_registrants[i].callback(RADIUS_init_chg_registrants[i].ctx, res);
        }
    }
}

/******************************************************************************/
// RADIUS_call_radius_id_state_chg_callbacks()
// Calls all registrants that need to know whether we're out of RADIUS IDs or not.
/******************************************************************************/
static void RADIUS_call_id_state_chg_callbacks(vtss_radius_id_state_chg_callback_result_e res)
{
    int i;
    T_D("Out of RADIUS identifiers");
    RADIUS_CRIT_ASSERT_LOCKED();
    for (i = 0; i < ARRSZ(RADIUS_id_state_chg_registrants); i++) {
        if (RADIUS_id_state_chg_registrants[i].callback != NULL) {
            RADIUS_id_state_chg_registrants[i].callback(RADIUS_id_state_chg_registrants[i].ctx, res);
        }
    }
}

/******************************************************************************/
// RADIUS_socket_uninit()
/******************************************************************************/
static void RADIUS_socket_uninit(RADIUS_server_type_e server_type, bool call_callbacks)
{
    int i;

    RADIUS_CRIT_ASSERT_LOCKED();
    VTSS_ASSERT(server_type < RADIUS_SERVER_TYPE_LAST);

    for (i = 0; i < VTSS_RADIUS_NUMBER_OF_SERVERS; i++) {
        RADIUS_server_state_s *server_state = &RADIUS_state.server_info[server_type].state[i];
        if (server_state->sockd != -1) {
            close(server_state->sockd);
            server_state->sockd = -1;
        }
    }

    RADIUS_state.server_info[server_type].initialized = FALSE;

    if (call_callbacks) {
        RADIUS_call_init_chg_callbacks(VTSS_RADIUS_INIT_CHG_CALLBACK_UNINITIALIZED);
    }
}


/******************************************************************************/
// RADIUS_socket_init()
// Attempts to open sockets for all configured servers.
/******************************************************************************/
static void RADIUS_socket_init(RADIUS_server_type_e server_type, bool call_callbacks)
{
    int i;
    BOOL result = TRUE;
    mesa_rc rc;

    RADIUS_CRIT_ASSERT_LOCKED();
    VTSS_ASSERT(server_type < RADIUS_SERVER_TYPE_LAST);

    for (i = 0; i < VTSS_RADIUS_NUMBER_OF_SERVERS; i++) {
        RADIUS_server_state_s     *server_state = &RADIUS_state.server_info[server_type].state[i];
        vtss_radius_server_info_t *server_cfg   = &RADIUS_state.server_info[server_type].config[i];

        if (server_cfg->host[0]) {
            if ((rc = RADIUS_create_socket(server_state, server_cfg->host, server_cfg->port)) != VTSS_RC_OK) {
                if (rc == VTSS_RADIUS_ERROR_NO_NAME) {
                    // hostname contains a domain name that cannot be resolved by DNS.
                    // The IP stack is up and running, so we won't flag this as an error but
                    // defer the socket creation until we try to send a RADIUS packet in RADIUS_do_tx()
                    T_I("RADIUS server lookup failed for %s, port %d. Retrying later.", (char *)server_cfg->host, server_cfg->port);
                } else {
                    // No error or warning message here. This can happen if the IP stack isn't ready yet
                    result = FALSE;
                    break;
                }
            } else {
                char ip_dst[RADIUS_ADDR_PORT_SIZE];
                T_D("Configured RADIUS Host %s, IP %s, socket fd %d", (char *)server_cfg->host,
                    RADIUS_sockaddr_to_string((struct sockaddr *)&server_state->dst_addr, ip_dst, sizeof(ip_dst)), server_state->sockd);
            }
            RADIUS_state.server_info[server_type].initialized = TRUE;
        }
    }

    if (result == FALSE) {
        // An error occurred above. Close all sockets again.
        RADIUS_socket_uninit(server_type, FALSE); // Do not call callbacks
    } else if (RADIUS_state.server_info[server_type].initialized) {
        if (!RADIUS_state.srand_initialized) {
            RADIUS_srand();
            RADIUS_state.srand_initialized = TRUE;
        }

        if (call_callbacks) {
            RADIUS_call_init_chg_callbacks(VTSS_RADIUS_INIT_CHG_CALLBACK_INITIALIZED);
        }
    }
}

/******************************************************************************/
// RADIUS_dealloc_handle()
/******************************************************************************/
static void RADIUS_dealloc_handle(RADIUS_id_state_s *id_state)
{
    RADIUS_CRIT_ASSERT_LOCKED();

    if (id_state->server_idx >= 0) {
        if (id_state->server_type == RADIUS_SERVER_TYPE_AUTH) {
            vtss_radius_auth_client_server_mib_s *mib = &RADIUS_auth_client_mib.radiusAuthServerExtTable[id_state->server_idx];
            if (mib->radiusAuthClientExtPendingRequests == 0) {
                T_E("Something is wrong with the pending request counter (1)");
            } else {
                T_D("Dec(Pending)");
                mib->radiusAuthClientExtPendingRequests--;
            }
        } else {
            vtss_radius_acct_client_server_mib_s *mib = &RADIUS_acct_client_mib.radiusAccServerExtTable[id_state->server_idx];
            if (mib->radiusAccClientExtPendingRequests == 0) {
                T_E("Something is wrong with the pending request counter (1)");
            } else {
                T_D("Dec(Pending)");
                mib->radiusAccClientExtPendingRequests--;
            }
        }
    }

    id_state->in_use = FALSE;
    VTSS_ASSERT(RADIUS_state.ids_in_use > 0);
    RADIUS_state.ids_in_use--;

    // Add some hysteresis to the use count, so that one that relies on this doesn't
    // turn on/off some frame reception over and over again.
    if (RADIUS_state.ids_in_use == ((int)RADIUS_state.max_id_cnt_minus_one + 1) / 2) {
        RADIUS_call_id_state_chg_callbacks(VTSS_RADIUS_ID_STATE_CHG_CALLBACK_READY);
    }

    if (id_state->tx_frm) {
        T_D("Freeing tx frame buffer");
        VTSS_FREE(id_state->tx_frm);
        id_state->tx_frm = NULL;
    }

}

/******************************************************************************/
// RADIUS_call_rx_callbacks()
// Calls all active callbacks with the specified result.
/******************************************************************************/
static void RADIUS_call_rx_callbacks(vtss_radius_rx_callback_result_e res)
{
    int i;
    RADIUS_id_state_s *id_state;

    // Must be taken
    RADIUS_CRIT_ASSERT_LOCKED();
    for (i = 0; i < RADIUS_state.max_id_cnt_minus_one + 1; i++) {
        id_state = &RADIUS_state.id_states[i];
        if (id_state->in_use) {
            if (id_state->callback != NULL) {
                // If the agent has allocated a handle, but not called vtss_radius_tx(), the
                // callback is NULL.
                T_I("Calling back(handle = %u, res = %d)", i, res);
                id_state->callback(i, id_state->ctx, (vtss_radius_access_codes_e)0, res, 0);
            }
            RADIUS_dealloc_handle(id_state);
        }
    }
    VTSS_ASSERT(RADIUS_state.ids_in_use == 0);
}

/******************************************************************************/
// RADIUS_md5_precalc()
/******************************************************************************/
static void RADIUS_md5_precalc(void)
{
    int i;
    RADIUS_md5_cache_s *m;
    vtss_radius_server_info_t *c;

    // Must be taken
    RADIUS_CRIT_ASSERT_LOCKED();

    for (i = 0; i < ARRSZ(RADIUS_md5_cache); i++) {
        m = &RADIUS_md5_cache[i];
        c = &RADIUS_cfg.servers_auth[i];
        if (c->host[0]) {
            vtss_md5_shared_key_init(&m->shared_key_context_1, &m->shared_key_context_2, (md5_u8 *)c->key, strlen(c->key));
        }
    }
}

/******************************************************************************/
// RADIUS_frm_len_get()
/******************************************************************************/
static inline u16 RADIUS_frm_len_get(u8 *frm)
{
    return (((u16)frm[RADIUS_PKT_IDX_LEN]) << 8 | frm[RADIUS_PKT_IDX_LEN + 1]);
}

/******************************************************************************/
// RADIUS_frm_len_set()
/******************************************************************************/
static inline void RADIUS_frm_len_set(u8 *frm, u16 len)
{
    frm[RADIUS_PKT_IDX_LEN + 0] = (len >> 8) & 0xFF;
    frm[RADIUS_PKT_IDX_LEN + 1] = (len >> 0) & 0xFF;
}

/******************************************************************************/
// RADIUS_find_attribute()
// Returns a pointer to the idx'th attribute in the packet containing @attribute,
// or NULL if none is found.
/******************************************************************************/
static u8 *RADIUS_find_attribute(u8 *dat, vtss_radius_attributes_e attribute, int idx)
{
    u8  *p  = &dat[RADIUS_PKT_IDX_FIRST_ATTRIB];
    u16 len = RADIUS_frm_len_get(dat);

    if (len < RADIUS_PKT_IDX_FIRST_ATTRIB) {
        return NULL;
    }

    while (p < (dat + len)) {
        if (*p == attribute) {
            if (idx == 0) {
                return p;
            } else {
                idx--;
                p += p[1];
            }
        } else {
            p += p[1];
        }
    }

    return NULL;
}

/******************************************************************************/
// RADIUS_add_tlv_len_check()
/******************************************************************************/
static bool RADIUS_add_tlv_len_check(RADIUS_id_state_s *id_state, vtss_radius_attributes_e type, int len, bool check_len = true)
{
    int rlen = len;

    if (!id_state->tx_frm || !id_state->tlv_ptr) {
        T_E("No Tx frame buffer");
        return false;
    }

    if (check_len && len > 255) {
        T_E("Invalid length (%d)", len);
        return false;
    }

    // Check if there's room for the TLV(s). There must be room for a
    // Message-Authenticator at the end.
    if (type != VTSS_RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR) {
        rlen += RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR_LEN;
    }

    // Is there room for the TLV?
    if (id_state->tlv_ptr + rlen > id_state->tx_frm + RADIUS_MAX_FRAME_SIZE_BYTES) {
        T_LW("No room for TLV type %d, (rlen = %u)", type, rlen);
        return false;
    }

    return true;
}

/******************************************************************************/
// RADIUS_add_tlv()
// Adds a TLV to the @tx_frm of id_state. type and val must be checked before
// this function is called.
// The length field must accommodate for the type and len fields here.
// Returns a pointer to the Value part of the TLV just added.
/******************************************************************************/
static u8 *RADIUS_add_tlv(RADIUS_id_state_s *id_state, vtss_radius_attributes_e type, int len, const u8 *val)
{
    u8 *result;

    T_D("Adding TLV (type %d, len %d) at offset %p", type, len, id_state->tlv_ptr - id_state->tx_frm);

    if (!RADIUS_add_tlv_len_check(id_state, type, len)) {
        return NULL;
    }

    result = id_state->tlv_ptr + 2;

    RADIUS_CRIT_ASSERT_LOCKED();

    *(id_state->tlv_ptr + 0) = type;
    *(id_state->tlv_ptr + 1) = (u8)len;
    memcpy(result, val, len - 2);
    id_state->tlv_ptr += len;

    // Update the length field.
    RADIUS_frm_len_set(id_state->tx_frm, id_state->tlv_ptr - id_state->tx_frm);

    return result;
}

/******************************************************************************/
// RADIUS_add_or_update_required_tlvs()
//
// For authentication request only!
//
// Add or update NAS-IP-Address/NAS-IPv6-Address.
// Use configured value if present else use IP address of outgoing interface.
//
// Add NAS-Identifier if configured.
//
// Called from RADIUS_do_tx() right before each transmission as this is the
// only point in time where the IP address of the outgoing interface is
// guaranteed to be known.
//
// Returns TRUE if frame has been modified, FALSE if not.
/******************************************************************************/
static BOOL RADIUS_add_or_update_required_tlvs(RADIUS_id_state_s *id_state, const struct sockaddr *sa)
{
    u8 *tlv_ptr;
    int tlv_len;
    BOOL changed = FALSE;

    T_D("enter");
    RADIUS_CRIT_ASSERT_LOCKED();

    if (id_state->server_type != RADIUS_SERVER_TYPE_AUTH) {
        T_E("This is NOT an authentication request!");
        return changed;
    }

    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        if ((tlv_ptr = RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_NAS_IP_ADDRESS, 0))) {
            // Update NAS-IP-Address if not using configured value
            if (!RADIUS_cfg.nas_ip_address_enable && memcmp(tlv_ptr + 2, &sin->sin_addr, sizeof(sin->sin_addr))) {
                T_D("TLV found - update ipv4 address");
                memcpy(tlv_ptr + 2, &sin->sin_addr, sizeof(sin->sin_addr));
                changed = TRUE;
            } else {
                T_D("TLV found - update not needed");
            }
        } else {
            // Add NAS-IP-Address
            tlv_len = sizeof(sin->sin_addr) + 2; // tlv_len must include type and length in the call to RADIUS_add_tlv().
            T_D("TLV not found - add ipv4 address");
            if (RADIUS_add_tlv(id_state, VTSS_RADIUS_ATTRIBUTE_NAS_IP_ADDRESS, tlv_len, RADIUS_cfg.nas_ip_address_enable ? (u8 *)&RADIUS_cfg.nas_ip_address : (u8 *)&sin->sin_addr)) {
                changed = TRUE;
            }
        }
        break;
    }
#ifdef VTSS_SW_OPTION_IPV6
    case AF_INET6: {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
        if ((tlv_ptr = RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_NAS_IPV6_ADDRESS, 0))) {
            // Update NAS-IPv6-Address if not using configured value
            if (!RADIUS_cfg.nas_ipv6_address_enable && memcmp(tlv_ptr + 2, &sin6->sin6_addr, sizeof(sin6->sin6_addr))) {
                T_D("TLV found - update ipv6 address");
                memcpy(tlv_ptr + 2, &sin6->sin6_addr, sizeof(sin6->sin6_addr));
                changed = TRUE;
            } else {
                T_D("TLV found - update not needed");
            }
        } else {
            // Add NAS-IPv6-Address
            tlv_len = sizeof(sin6->sin6_addr) + 2; // tlv_len must include type and length in the call to RADIUS_add_tlv().
            T_D("NAS_IP_ADDRESS TLV not found - add ipv6 address");
            if (RADIUS_add_tlv(id_state, VTSS_RADIUS_ATTRIBUTE_NAS_IPV6_ADDRESS, tlv_len, RADIUS_cfg.nas_ipv6_address_enable ? (u8 *)&RADIUS_cfg.nas_ipv6_address : (u8 *)&sin6->sin6_addr)) {
                changed = TRUE;
            }
        }
        break;
    }
#endif
    default:
        T_LW("Unknown Address Family!");
        break;
    }

    if (RADIUS_cfg.nas_identifier[0] && !RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_NAS_IDENTIFIER, 0)) {
        // Add NAS-Identifier
        tlv_len = strlen(RADIUS_cfg.nas_identifier) + 2; // tlv_len must include type and length in the call to RADIUS_add_tlv().
        T_D("TLV not found - add nas identifier");
        if (RADIUS_add_tlv(id_state, VTSS_RADIUS_ATTRIBUTE_NAS_IDENTIFIER, tlv_len, (u8 *)RADIUS_cfg.nas_identifier)) {
            changed = TRUE;
        }
    }

    return changed;
}

/******************************************************************************/
// RADIUS_ins_len_and_calc_authenticator()
//
// For authentication request:
// (Re-)compute the length field of the RADIUS Tx Frame, and a new Message-
// Authenticator or Request-Authenticator, and possibly User-Password attribute.
//
// For accounting request:
// (Re-)compute the length field of the RADIUS Tx Frame, and a new Message-
// Authenticator or Request-Authenticator, and possibly User-Password attribute.
//
// Used initially before transmitting RADIUS frame the first time, or when
// changing server.
/******************************************************************************/
static void RADIUS_ins_len_and_calc_authenticator(RADIUS_id_state_s *id_state)
{
    u8 *user_pw_tlv_ptr, *eap_msg_tlv_ptr, *ptr;
    unsigned int rnd, i, j;

    RADIUS_CRIT_ASSERT_LOCKED();

    if (id_state->server_type == RADIUS_SERVER_TYPE_AUTH) {
        user_pw_tlv_ptr = RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_USER_PASSWORD, 0);
        eap_msg_tlv_ptr = RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_EAP_MESSAGE, 0);

        // EAP-Message and User-Password attributes are mutually exclusive, but one of them must exist.
        VTSS_ASSERT((uint)(eap_msg_tlv_ptr != NULL) ^ (uint)(user_pw_tlv_ptr != NULL));

        // Add a random Request-Authenticator
        ptr = &id_state->tx_frm[RADIUS_PKT_IDX_AUTHENTICATOR];
        for (i = 0; i < MD5_MAC_LEN / sizeof(int); i++) {
            rnd = rand();
            for (j = 0; j < sizeof(int); j++) {
                *(ptr++) = rnd & 0xFF;
                rnd >>= 8;
            }
        }

        if (eap_msg_tlv_ptr) {
            T_D("Contains EAP-Message");
            u8 *msg_auth_tlv_ptr = RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR, 0);
            RADIUS_md5_cache_s *m = &RADIUS_md5_cache[id_state->server_idx];

            // Frame contains an EAP-Message attribute, and therefore must also contain a Message-Authenticator attribute.
            if (msg_auth_tlv_ptr) {
                T_D("Contains Message-Authenticator");
                // The length must already be correct:
                VTSS_ASSERT(id_state->tlv_ptr - id_state->tx_frm == RADIUS_frm_len_get(id_state->tx_frm));
                msg_auth_tlv_ptr += 2; // Skip type and len fields of the TLV.
                // It's already been added, so the reason that this function has been called is because we've changed server.
                // Gotta zero-out the Message-Authenticator TLV before re-computing the MD5.
                memset(msg_auth_tlv_ptr, 0, MD5_MAC_LEN);
            } else {
                u8 temp_str[MD5_MAC_LEN] = {0};
                T_D("Doesn't contain Message-Authenticator");

                // This is the initial transmit.
                // Create the Message-Authenticator TLV. Initially with 0's.
                // RADIUS_add_tlv() returns pointer to Value-part of TLV.
                msg_auth_tlv_ptr = RADIUS_add_tlv(id_state, VTSS_RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR, RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR_LEN, temp_str);
                if (msg_auth_tlv_ptr == NULL) {
                    return;
                }
            }

            T_D("Computing MD5 on len= %u", RADIUS_frm_len_get(id_state->tx_frm));

            // Calculate hmac-md5, and place it in the Message-Authenticator TLV.
            vtss_md5_shared_key(&m->shared_key_context_1, &m->shared_key_context_2, id_state->tx_frm, id_state->tlv_ptr - id_state->tx_frm, msg_auth_tlv_ptr);
        } else {
            struct vtss_MD5Context md5_ctx;
            struct vtss_MD5Context md5_secret_ctx;
            u8 *secret = (u8 *)RADIUS_cfg.servers_auth[id_state->server_idx].key;
            u8 user_pw[MD5_MAC_LEN];
            u8 *user_pw_ptr = id_state->user_pw;
            int pw_len = strlen((char *)user_pw_ptr);
            unsigned int md5_cnt = (pw_len + 15) / 16;
            if (md5_cnt == 0) {
                md5_cnt++;
            }

            // No EAP-Message attributes in packet. Put MD5 hash in User-Password attribute.
            // First update the length field.
            RADIUS_frm_len_set(id_state->tx_frm, id_state->tlv_ptr - id_state->tx_frm);
            user_pw_tlv_ptr += 2; // Go pass the Type and Length fields.

            // Successively compute the MD5, according to the following taken from RFC2865:
            //   b1 = MD5(S + RA)       c(1) = p1 xor b1
            //   b2 = MD5(S + c(1))     c(2) = p2 xor b2
            //          .                       .
            //          .                       .
            //          .                       .
            //   bi = MD5(S + c(i-1))   c(i) = pi xor bi
            vtss_MD5Init(&md5_secret_ctx);
            vtss_MD5Update(&md5_secret_ctx, secret, strlen((char *)secret));
            ptr = &id_state->tx_frm[RADIUS_PKT_IDX_AUTHENTICATOR];

            for (i = 0; i < md5_cnt; i++) {
                md5_ctx = md5_secret_ctx;

                vtss_MD5Update(&md5_ctx, ptr, MD5_MAC_LEN);
                vtss_MD5Final(user_pw_tlv_ptr, &md5_ctx);

                ptr = user_pw_tlv_ptr;

                // strncpy() NULL-pads the dst if src is less than MD5_MAC_LEN bytes (this is required in the following).
                strncpy((char *)user_pw, (char *)user_pw_ptr, MD5_MAC_LEN);
                user_pw_ptr += MD5_MAC_LEN;

                // c(1) = p1 xor b1
                for (j = 0; j < MD5_MAC_LEN; j++) {
                    *(user_pw_tlv_ptr++) ^= user_pw[j];
                }
            }
        }
    } else { // This is an accounting request frame
        struct vtss_MD5Context md5_ctx;
        u8 *secret = (u8 *)RADIUS_cfg.servers_acct[id_state->server_idx].key;

        // First update the length field.
        RADIUS_frm_len_set(id_state->tx_frm, id_state->tlv_ptr - id_state->tx_frm);

        // Then set a zero Request-Authenticator
        memset(&id_state->tx_frm[RADIUS_PKT_IDX_AUTHENTICATOR], 0, MD5_MAC_LEN);

        // Calculate the new Request-Authenticator
        vtss_MD5Init(&md5_ctx);
        vtss_MD5Update(&md5_ctx, id_state->tx_frm, id_state->tlv_ptr - id_state->tx_frm);
        vtss_MD5Update(&md5_ctx, secret, strlen((char *)secret));
        vtss_MD5Final(&id_state->tx_frm[RADIUS_PKT_IDX_AUTHENTICATOR], &md5_ctx);
    }
}

/******************************************************************************/
// RADIUS_check_authenticator()
// As per RFC2869:
//   If the packet contains a Message-Authenticator attribute, then that
//   value MUST be used to calculate the correct value of the Message-Authenticator
//   and silently discard the packet if it's not correct.
//   If the packet contains an EAP-Message attribute, then the packet SHOULD
//   also contain a Message-Authenticator attribute. If not, the packet SHOULD be
//   silently discarded by the NAS (the switch). We do that.
//
//   Message-Authenticator = HMAC-MD5(Type, Identifier, Length,
//                                    Request Authenticator, Attributes).
//
//   When computing the checksum, the Message-Authenticator value should be
//   considered to be sixteen octests of zero. The shared secret is used as the
//   key for the HMAC-MD5 hash.
//
// As per RFC2865:
//   If the packet does not contain a Message-Authenticator attribute, then
//   the Response-Authenticator is used and compared to the MD5 calculated as:
//     MD5(Code+ID+Length+RequestAuth+Attributes+Secret)
/******************************************************************************/
static inline BOOL RADIUS_check_authenticator(u8 *dat, RADIUS_id_state_s *id_state, u16 len)
{
    u8 md5[MD5_MAC_LEN];
    u8 *eap_msg_attrib  = RADIUS_find_attribute(dat, VTSS_RADIUS_ATTRIBUTE_EAP_MESSAGE, 0);
    u8 *msg_auth_attrib = RADIUS_find_attribute(dat, VTSS_RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR, 0);

    if (eap_msg_attrib && !msg_auth_attrib) {
        T_D("EAP-message needs message-authenticator, but none found");
        return FALSE;
    }

    if (msg_auth_attrib) {
        RADIUS_md5_cache_s *m = &RADIUS_md5_cache[id_state->server_idx];
        u8 msg_auth_backup[MD5_MAC_LEN];

        if (id_state->server_type != RADIUS_SERVER_TYPE_AUTH) {
            T_D("Message authenticator found in an accounting packet");
            return FALSE;
        }

        // Backup supplied Message-Authenticator
        memcpy(msg_auth_backup, &msg_auth_attrib[2], MD5_MAC_LEN);

        // Fill-in zeros @ Message-Authenticator
        memset((void *)&msg_auth_attrib[2], 0, MD5_MAC_LEN);

        // Replace Response-Authenticator with Request-Authenticator
        memcpy(&dat[RADIUS_PKT_IDX_AUTHENTICATOR], &id_state->tx_frm[RADIUS_PKT_IDX_AUTHENTICATOR], MD5_MAC_LEN);

        // Calculate expected Response-Authenticator based on the pre-shared key.
        vtss_md5_shared_key(&m->shared_key_context_1, &m->shared_key_context_2, dat, len, md5);

        if (memcmp(md5, msg_auth_backup, MD5_MAC_LEN) == 0) {
            return TRUE;
        } else {
            T_D("Wrong message authenticator found");
            return FALSE;
        }
    } else {
        struct vtss_MD5Context md5_ctx;
        u8 resp_auth_backup[MD5_MAC_LEN];
        u8 *secret;

        // Message-Authenticator not found in packet. Use the Authenticator field.
        secret = (u8 *)RADIUS_state.server_info[id_state->server_type].config[id_state->server_idx].key;

        // Backup supplied Response-Authenticator
        memcpy(resp_auth_backup, &dat[RADIUS_PKT_IDX_AUTHENTICATOR], MD5_MAC_LEN);

        // Replace Response-Authenticator with Request-Authenticator
        memcpy(&dat[RADIUS_PKT_IDX_AUTHENTICATOR], &id_state->tx_frm[RADIUS_PKT_IDX_AUTHENTICATOR], MD5_MAC_LEN);

        // Calculate expected Response-Authenticator
        vtss_MD5Init(&md5_ctx);
        vtss_MD5Update(&md5_ctx, dat, len);
        vtss_MD5Update(&md5_ctx, secret, strlen((char *)secret));
        vtss_MD5Final(md5, &md5_ctx);

        if (memcmp(md5, resp_auth_backup, MD5_MAC_LEN) == 0) {
            return TRUE;
        }

        T_D("Incorrect Response-authenticator in RADIUS data");
    }

    // If we get here, the authenticator was not OK
    return FALSE;
}

/******************************************************************************/
// RADIUS_check_length()
// Loops through attributes and checks if the total length matches that of
// the attributes. Returns TRUE if OK, FALSE if not.
/******************************************************************************/
static inline BOOL RADIUS_check_length(u8 *frm, ssize_t frm_len)
{
    u8  *p         = &frm[RADIUS_PKT_IDX_FIRST_ATTRIB];
    u8  *end       = frm + frm_len;
    u8  l;
    u16 len        = RADIUS_frm_len_get(frm); // Length as reported in packet
    u16 attrib_len = 0;                       // Length as reported by the individual attributes.

    if (len != frm_len) {
        T_E("RADIUS length abnormality. Exp=" VPRIsz", got %u", frm_len, len);
        return FALSE;
    }

    if (len < RADIUS_PKT_IDX_FIRST_ATTRIB) {
        T_E("RADIUS length smaller than smallest possible. Exp>=%d, got %u", RADIUS_PKT_IDX_FIRST_ATTRIB, len);
        return FALSE;
    }

    while (p < end) {
        l = p[1];
        if (l < 2) {
            T_E("RADIUS attribute length is less than 2 (%u)", l);
            return FALSE;
        }
        attrib_len += l;
        p          += l;
    }

    if (attrib_len != len - RADIUS_PKT_IDX_FIRST_ATTRIB) {
        T_E("RADIUS attribute length abnormality. Exp=%u, got %u", len - RADIUS_PKT_IDX_FIRST_ATTRIB, attrib_len);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
// RADIUS_auth_process_frm()
/******************************************************************************/
/*lint -e{454,455} ... locked at entry */
static void RADIUS_auth_process_frm(int server_idx, vtss_tick_count_t cur_time_ticks, ssize_t len)
{
    vtss_radius_access_codes_e           radius_code = (vtss_radius_access_codes_e)RADIUS_rx_buffer[RADIUS_PKT_IDX_CODE];
    u8                                   radius_id   = RADIUS_rx_buffer[RADIUS_PKT_IDX_ID];
    vtss_radius_auth_client_server_mib_s *mib        = &RADIUS_auth_client_mib.radiusAuthServerExtTable[server_idx];
    RADIUS_id_state_s                    *id_state;

    // Must be taken
    RADIUS_CRIT_ASSERT_LOCKED();

    T_D("RADIUS auth data with length: " VPRIsz, len);

    // Check lengths (both too small, packet length vs. frm length, and attribute lengths).
    if (!RADIUS_check_length(RADIUS_rx_buffer, len)) {
        mib->radiusAuthClientExtMalformedAccessResponses++;
        return;
    }

    // Counter gymnastics
    if (radius_code == VTSS_RADIUS_CODE_ACCESS_ACCEPT) {
        mib->radiusAuthClientExtAccessAccepts++;
    } else if (radius_code == VTSS_RADIUS_CODE_ACCESS_REJECT) {
        mib->radiusAuthClientExtAccessRejects++;
    } else if (radius_code == VTSS_RADIUS_CODE_ACCESS_CHALLENGE) {
        mib->radiusAuthClientExtAccessChallenges++;
    } else {
        mib->radiusAuthClientExtUnknownTypes++;
        return;
    }

    id_state = &RADIUS_state.id_states[radius_id];
    if (!id_state->in_use) {
        mib->radiusAuthClientExtPacketsDropped++;
        T_D("Received a RADIUS packet without pending agent (id=%u).", (unsigned)radius_id);
        return;
    }

    if (id_state->server_idx != server_idx) {
        // Received frame from a server other than the expected
        mib->radiusAuthClientExtPacketsDropped++;
        T_D("Received RADIUS packet from wrong server (exp=%d, act=%d).", id_state->server_idx, server_idx);
        return;
    }

    if (!RADIUS_check_authenticator(RADIUS_rx_buffer, id_state, len)) {
        // Wrong response authenticator => silently discard packet
        mib->radiusAuthClientExtBadAuthenticators++;
        return;
    }

    // Update the latest round-trip time (measured in hundredths of a second)
    mib->radiusAuthClientExtRoundTripTime = VTSS_OS_TICK2MSEC(cur_time_ticks - id_state->tx_time_ticks) / 10;

    // Change the tlv_ptr to point to the received frame, so that the agent can iterate the TLVs in just a second.
    // It is OK to re-use the tlv_ptr for that purpose, since we're not going to retransmit the id_state->tx_frm anymore.
    id_state->tlv_ptr = &RADIUS_rx_buffer[RADIUS_PKT_IDX_FIRST_ATTRIB];

    // Call the user's callback function. He may now start to extract the TLVs using
    // the vtss_radius_tlv_get() or vtss_radius_tlv_iterate() functions.
    // Unfortunately, we need to release the semaphore before calling the callback, because
    // the callback function may acquire its own semaphore, which may already be taken and
    // in turn it may have made a call into the RADIUS module. If we didn't release the
    // semaphore, we would deadlock.
    // In order to ensure that the vtss_radius_tlv_get() and vtss_radius_tlv_iterate()
    // functions work on the same received frame as is currently in the RADIUS_rx_buffer,
    // they must check the handle against the RADIUS_rx_buffer's ID.

    id_state->waiting_for_callback_to_finish = TRUE;
    RADIUS_CRIT_EXIT();
    T_I("Calling back");
    id_state->callback(radius_id, id_state->ctx, radius_code, VTSS_RADIUS_RX_CALLBACK_OK, RADIUS_state.server_info[id_state->server_type].state[id_state->server_idx].server_id);
    RADIUS_CRIT_ENTER();

    // It may happen that while the RADIUS_CRIT is released above, another thread frees the
    // handle. If that is the case, we cannot call the RADIUS_dealloc_handle() here again
    // since that would result in reference counting failure. That's why we do a check for
    // id_state->in_use. Furthermore, if the same handle gets allocated again by a third
    // module while the callback was called, it's id_state->in_use would still be TRUE,
    // but since it's not the same owner of the handle as just before the call to the
    // callback, we must not free it. Therefore, we use the waiting_for_callback_to_finish
    // to check if the handle has been re-allocated (the vtss_radius_alloc() clears this flag)
    // to another RADIUS request.
    if (id_state->waiting_for_callback_to_finish && id_state->in_use) {
        RADIUS_dealloc_handle(id_state);
    } else {
        T_I("RADIUS handle got deallocated while the callback was executing (in_use=%d, waiting_for_callback_to_finish=%d)", id_state->in_use, id_state->waiting_for_callback_to_finish);
    }
}

/******************************************************************************/
// RADIUS_acct_process_frm()
/******************************************************************************/
/*lint -e{454,455} ... locked at entry */
static void RADIUS_acct_process_frm(int server_idx, vtss_tick_count_t cur_time_ticks, ssize_t len)
{
    vtss_radius_access_codes_e           radius_code = (vtss_radius_access_codes_e)RADIUS_rx_buffer[RADIUS_PKT_IDX_CODE];
    u8                                   radius_id   = RADIUS_rx_buffer[RADIUS_PKT_IDX_ID];
    vtss_radius_acct_client_server_mib_s *mib        = &RADIUS_acct_client_mib.radiusAccServerExtTable[server_idx];
    RADIUS_id_state_s                    *id_state;

    // Must be taken
    RADIUS_CRIT_ASSERT_LOCKED();

    T_D("RADIUS auth data with length: " VPRIsz, len);

    // Check lengths (both too small, packet length vs. frm length, and attribute lengths).
    if (!RADIUS_check_length(RADIUS_rx_buffer, len)) {
        mib->radiusAccClientExtMalformedResponses++;
        return;
    }

    // Counter gymnastics
    if (radius_code == VTSS_RADIUS_CODE_ACCOUNTING_RESPONSE) {
        mib->radiusAccClientExtResponses++;
    } else {
        mib->radiusAccClientExtUnknownTypes++;
        return;
    }

    id_state = &RADIUS_state.id_states[radius_id];
    if (!id_state->in_use) {
        mib->radiusAccClientExtPacketsDropped++;
        T_D("Received a RADIUS packet without pending agent (id=%u).", (unsigned)radius_id);
        return;
    }

    if (id_state->server_idx != server_idx) {
        // Received frame from a server other than the expected
        mib->radiusAccClientExtPacketsDropped++;
        T_D("Received RADIUS packet from wrong server (exp=%d, act=%d).", id_state->server_idx, server_idx);
        return;
    }

    if (!RADIUS_check_authenticator(RADIUS_rx_buffer, id_state, len)) {
        // Wrong response authenticator => silently discard packet
        mib->radiusAccClientExtBadAuthenticators++;
        return;
    }

    // Update the latest round-trip time (measured in hundredths of a second)
    mib->radiusAccClientExtRoundTripTime = VTSS_OS_TICK2MSEC(cur_time_ticks - id_state->tx_time_ticks) / 10;

    // Change the tlv_ptr to point to the received frame, so that the agent can iterate the TLVs in just a second.
    // It is OK to re-use the tlv_ptr for that purpose, since we're not going to retransmit the id_state->tx_frm anymore.
    id_state->tlv_ptr = &RADIUS_rx_buffer[RADIUS_PKT_IDX_FIRST_ATTRIB];

    // Call the user's callback function. He may now start to extract the TLVs using
    // the vtss_radius_tlv_get() or vtss_radius_tlv_iterate() functions.
    // Unfortunately, we need to release the semaphore before calling the callback, because
    // the callback function may acquire its own semaphore, which may already be taken and
    // in turn it may have made a call into the RADIUS module. If we didn't release the
    // semaphore, we would deadlock.
    // In order to ensure that the vtss_radius_tlv_get() and vtss_radius_tlv_iterate()
    // functions work on the same received frame as is currently in the RADIUS_rx_buffer,
    // they must check the handle against the RADIUS_rx_buffer's ID.
    RADIUS_CRIT_EXIT();
    T_I("Calling back");
    id_state->callback(radius_id, id_state->ctx, radius_code, VTSS_RADIUS_RX_CALLBACK_OK, RADIUS_state.server_info[id_state->server_type].state[id_state->server_idx].server_id);
    RADIUS_CRIT_ENTER();
    RADIUS_dealloc_handle(id_state);
}

/******************************************************************************/
// RADIUS_rx_check()
/******************************************************************************/
static void RADIUS_rx_check(void)
{
    struct timeval tv;
    ssize_t len;
    int i, t;
    fd_set fds;
    vtss_tick_count_t cur_time_ticks;

    RADIUS_CRIT_ASSERT_LOCKED();

    // Read all responses from all servers.
    for (i = 0; i < VTSS_RADIUS_NUMBER_OF_SERVERS; i++) {
        for (t = 0; t < RADIUS_SERVER_TYPE_LAST; t++) {
            RADIUS_server_state_s *server_state = &RADIUS_state.server_info[t].state[i];
            if (server_state->sockd != -1) {
                while (1) {
                    /*lint --e{573} Suppress Lint Warning 573: Signed-unsigned mix with divide */
                    tv.tv_sec = 0;
                    tv.tv_usec = 0;

                    FD_ZERO(&fds);
                    FD_SET(server_state->sockd, &fds); // Adds sock to the file descriptor set

                    if (select(server_state->sockd + 1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET(server_state->sockd, &fds)) {
                        if ((len = recv(server_state->sockd, RADIUS_rx_buffer, sizeof(RADIUS_rx_buffer), 0)) > 0) {
                            cur_time_ticks = vtss_current_time();
                            if (t == RADIUS_SERVER_TYPE_AUTH) {
                                T_D("Incoming auth radius data, len " VPRIsz, len);
                                RADIUS_auth_process_frm(i, cur_time_ticks, len);
                            } else {
                                T_D("Incoming acct radius data, len " VPRIsz, len);
                                RADIUS_acct_process_frm(i, cur_time_ticks, len);
                            }
                        } else {
                            int handle;

                            T_I("Error receiving RADIUS data (%d, %s)", errno, strerror(errno)); // This can happen if we e.g. receive an ICMP Destination Unreachable
                            // Shortcut all RADIUS handles using this server to speed up authentication
                            for (handle = 0; handle < RADIUS_state.max_id_cnt_minus_one + 1; handle++) {
                                RADIUS_id_state_s *id_state = &RADIUS_state.id_states[handle];
                                if (id_state->in_use && id_state->server_idx == i && id_state->server_type == t) {
                                    T_I("Forcing next server (current index = %d, type = %d) on handle = %u", i, t, handle);
                                    id_state->force_next_server = TRUE;
                                    id_state->retries_left = 0;
                                }
                            }
                        }
                    } else {
                        break;
                    }
                }
            }
        }
    }
}

/******************************************************************************/
// RADIUS_server_alive()
/******************************************************************************/
static BOOL RADIUS_server_alive(RADIUS_id_state_s *id_state, vtss_tick_count_t cur_time_ticks, int server_idx)
{
    RADIUS_server_type_e      server_type   = id_state->server_type;
    RADIUS_server_state_s     *server_state = &RADIUS_state.server_info[server_type].state[server_idx];
    vtss_radius_server_info_t *server_cfg   = &RADIUS_state.server_info[server_type].config[server_idx];
    BOOL                      result        = FALSE;

    VTSS_ASSERT(RADIUS_state.server_info[server_type].active_server_cnt > 0);

    if (server_cfg->host[0]) {
        if ((id_state->server_idx == -1 && RADIUS_state.server_info[server_type].active_server_cnt == 1) || // Ignore dead-time when there's only one server, and this is the first time we try to transmit a frame (server_idx == -1).
            (cur_time_ticks >= server_state->next_time_to_try_this_server_ticks)) {                         // The server still seems to be alive. Use it.
            id_state->server_idx = server_idx;
            result = TRUE;
        }
    }

    T_D("Exit(result=%d) when invoked with server_idx = %d", result, server_idx);

    return result;
}

/******************************************************************************/
// RADIUS_decide_server()
// Returns FALSE if there are no more active servers to pick from.
/******************************************************************************/
static BOOL RADIUS_decide_server(RADIUS_id_state_s *id_state, vtss_tick_count_t cur_time_ticks)
{
    int server_idx;

    RADIUS_CRIT_ASSERT_LOCKED();

    T_D("Enter. use_this_server_idx_only = %d", id_state->use_this_server_idx_only);

    if (id_state->use_this_server_idx_only >= 0) {
        // Client wants to use a particular server (probably because he already has a conversation ongoing with that server).
        if (id_state->server_idx == -1) {
            return RADIUS_server_alive(id_state, cur_time_ticks, id_state->use_this_server_idx_only);
        } else {
            return FALSE; // Only one server is usable in this case.
        }
    } else {
        for (server_idx = id_state->server_idx + 1; server_idx < VTSS_RADIUS_NUMBER_OF_SERVERS; server_idx++) {
            if (RADIUS_server_alive(id_state, cur_time_ticks, server_idx)) {
                return TRUE;
            }
        }
    }

    // No more servers to try.
    return FALSE;
}

/******************************************************************************/
// RADIUS_do_tx()
/******************************************************************************/
static void RADIUS_do_tx(RADIUS_id_state_s *id_state, BOOL new_server, vtss_tick_count_t cur_time_ticks)
{
    RADIUS_server_state_s     *server_state = &RADIUS_state.server_info[id_state->server_type].state[id_state->server_idx];
    vtss_radius_server_info_t *server_cfg   = &RADIUS_state.server_info[id_state->server_type].config[id_state->server_idx];
    mesa_rc                   rc;

    if (id_state->server_type == RADIUS_SERVER_TYPE_AUTH) {
        vtss_radius_auth_client_server_mib_s *mib = &RADIUS_auth_client_mib.radiusAuthServerExtTable[id_state->server_idx];
        if (new_server) {
            id_state->retries_left = server_cfg->retransmit; // Encoding: 0 = transmit once, 1 = transmit at most twice, ...
            // Transmit the frame right away.
            mib->radiusAuthClientExtAccessRequests++;
            T_D("Inc(Pending)");
            mib->radiusAuthClientExtPendingRequests++;
        } else {
            // We're here because of a retransmission
            mib->radiusAuthClientExtAccessRetransmissions++;
            id_state->retries_left--;
        }
    } else {
        vtss_radius_acct_client_server_mib_s *mib = &RADIUS_acct_client_mib.radiusAccServerExtTable[id_state->server_idx];
        if (new_server) {
            id_state->retries_left = server_cfg->retransmit; // Encoding: 0 = transmit once, 1 = transmit at most twice, ...
            // Transmit the frame right away.
            mib->radiusAccClientExtRequests++;
            T_D("Inc(Pending)");
            mib->radiusAccClientExtPendingRequests++;
        } else {
            // We're here because of a retransmission
            mib->radiusAccClientExtRetransmissions++;
            id_state->retries_left--;
        }
    }

    id_state->force_next_server = FALSE;

    if ((rc = RADIUS_create_socket(server_state, server_cfg->host, server_cfg->port)) != VTSS_RC_OK) {
        if (rc == VTSS_RADIUS_ERROR_NO_NAME) {
            T_LW("RADIUS server lookup failed for %s, port %d.", server_cfg->host, server_cfg->port);
            S_W("RADIUS server lookup failed for %s, port %d.", server_cfg->host, server_cfg->port);
        }
    } else {
        BOOL changed = FALSE;
        u16  len;
        int  res;

        if (id_state->server_type == RADIUS_SERVER_TYPE_AUTH) {
            changed = RADIUS_add_or_update_required_tlvs(id_state, (struct sockaddr *)&server_state->src_addr);
        }
        if (new_server || changed) {
            // (Re-)calculate Request-Authenticator or Message-Authenticator.
            RADIUS_ins_len_and_calc_authenticator(id_state);
        }
        id_state->tx_time_ticks = cur_time_ticks;
        len = RADIUS_frm_len_get(id_state->tx_frm);
        T_N("Transmitting RADIUS packet on fd=%d, len=%u. Data:", server_state->sockd, len);
        T_N_HEX(id_state->tx_frm, len);

        if ((res = send(server_state->sockd, id_state->tx_frm, len, 0)) < 0) {
            if (errno == EHOSTUNREACH || errno == EHOSTDOWN || errno == ENETDOWN || errno == ECONNREFUSED) {
                // Demote these particular errors to warnings.
                T_LW("Couldn't transmit RADIUS frame , errno = (%d, %s)", errno, strerror(errno));
                S_W("Couldn't transmit RADIUS frame , errno = (%d, %s)", errno, strerror(errno));
            } else {
                T_E("Error transmitting RADIUS frame, errno = (%d, %s)", errno, strerror(errno));
            }
        } else {
            char ip_src[RADIUS_ADDR_PORT_SIZE], ip_dst[RADIUS_ADDR_PORT_SIZE];
            T_D("Transmitted %d bytes of RADIUS data from %s to %s", res,
                RADIUS_sockaddr_to_string((struct sockaddr *)&server_state->src_addr, ip_src, sizeof(ip_src)),
                RADIUS_sockaddr_to_string((struct sockaddr *)&server_state->dst_addr, ip_dst, sizeof(ip_dst)));
        }
    }
}

/******************************************************************************/
// RADIUS_tx_check()
/******************************************************************************/
static void RADIUS_tx_check(void)
{
    int               i;
    vtss_tick_count_t cur_time_ticks = vtss_current_time();
    RADIUS_id_state_s *id_state;

    RADIUS_CRIT_ASSERT_LOCKED();

    // Loop through all IDs to see if they're active.
    for (i = 0; i < RADIUS_state.max_id_cnt_minus_one + 1; i++) {
        id_state = &RADIUS_state.id_states[i];
        if (id_state->in_use) {
            if (id_state->server_idx == -2) {
                // This indicates that the agent has called vtss_radius_alloc(), but
                // hasn't called vtss_radius_tx() yet. Nothing to do.
            } else if (id_state->server_idx == -1) {
                // This indicates that the agent has called vtss_radius_tx(), but
                // the frame hasn't been transmitted the first time yet.
                // Find a server.
                if (!RADIUS_decide_server(id_state, cur_time_ticks)) {
                    RADIUS_CRIT_EXIT();
                    T_I("%d: Calling back with timeout", i);
                    id_state->callback(i, id_state->ctx, (vtss_radius_access_codes_e)0, VTSS_RADIUS_RX_CALLBACK_TIMEOUT, 0);
                    RADIUS_CRIT_ENTER();
                    RADIUS_dealloc_handle(id_state);
                } else {
                    // Found a server.
                    T_D("Calling RADIUS_do_tx(%u) from server_idx = -1", i);
                    if (id_state->server_idx >= 0) {
                        RADIUS_do_tx(id_state, TRUE, cur_time_ticks);
                    }
                }
            } else {
                RADIUS_server_state_s *server_state = &RADIUS_state.server_info[id_state->server_type].state[id_state->server_idx];
                if (id_state->force_next_server || cur_time_ticks >= id_state->tx_time_ticks + server_state->timeout_per_retransmit_ticks) {
                    // Get a pointer to the current server's MIB (before id_state->server_idx it potentially gets overwritten below):
                    vtss_radius_auth_client_server_mib_s *mib_auth = &RADIUS_auth_client_mib.radiusAuthServerExtTable[id_state->server_idx];
                    vtss_radius_acct_client_server_mib_s *mib_acct = &RADIUS_acct_client_mib.radiusAccServerExtTable[id_state->server_idx];
                    // Timed out.
                    if (id_state->server_type == RADIUS_SERVER_TYPE_AUTH) {
                        mib_auth->radiusAuthClientExtTimeouts++;
                    } else {
                        mib_acct->radiusAccClientExtTimeouts++;
                    }

                    // Check if we need to retransmit.
                    if (id_state->retries_left) {
                        T_D("Calling RADIUS_do_tx(%u) from retry (cur_time_ticks = " VPRI64u", tx_time_ticks = " VPRI64u", timeout_per_retransmit_ticks=" VPRI64u")", i, cur_time_ticks, id_state->tx_time_ticks, server_state->timeout_per_retransmit_ticks);
                        RADIUS_do_tx(id_state, FALSE, cur_time_ticks);
                    } else {
                        // Update this server's dead-time so that we don't re-use it in up-coming requests.
                        if (RADIUS_cfg.dead_time_secs != 0) {
                            server_state->next_time_to_try_this_server_ticks = cur_time_ticks + RADIUS_state.dead_time_per_server_ticks;
                        }

                        // Try the next server.
                        if (!RADIUS_decide_server(id_state, cur_time_ticks)) {
                            RADIUS_CRIT_EXIT();
                            T_I("%d: Calling back with timeout", i);
                            id_state->callback(i, id_state->ctx, (vtss_radius_access_codes_e)0, VTSS_RADIUS_RX_CALLBACK_TIMEOUT, 0);
                            RADIUS_CRIT_ENTER();
                            RADIUS_dealloc_handle(id_state); // Reduces the radiusAuthClientExtPendingRequests.
                        } else {
                            // No more retries to the old server. Reduce the pending requests. This cannot be done by a call to RADIUS_dealloc_handle(),
                            // because we're not gonna deallocate the handle yet.
                            if (id_state->server_type == RADIUS_SERVER_TYPE_AUTH) {
                                if (mib_auth->radiusAuthClientExtPendingRequests == 0) {
                                    T_E("Something is wrong with the pending request counter (2)");
                                } else {
                                    T_D("Dec(Pending)");
                                    mib_auth->radiusAuthClientExtPendingRequests--;
                                }
                            } else {
                                if (mib_acct->radiusAccClientExtPendingRequests == 0) {
                                    T_E("Something is wrong with the pending request counter (2)");
                                } else {
                                    T_D("Dec(Pending)");
                                    mib_acct->radiusAccClientExtPendingRequests--;
                                }
                            }

                            // Found a server.
                            T_D("Calling RADIUS_do_tx(%u) from new server (cur_time_ticks = " VPRI64u", tx_time_ticks = " VPRI64u", timeout_per_retransmit_ticks=" VPRI64u")",
                                i, cur_time_ticks, id_state->tx_time_ticks, server_state->timeout_per_retransmit_ticks);
                            RADIUS_do_tx(id_state, TRUE, cur_time_ticks);
                        }
                    }
                }
            }
        }
    }
}

/******************************************************************************/
// RADIUS_clear_server_states()
/******************************************************************************/
static void RADIUS_clear_server_states(void)
{
    int t, server;
    for (t = 0; t < RADIUS_SERVER_TYPE_LAST; t++) {
        RADIUS_state.server_info[t].active_server_cnt = 0;
        for (server = 0; server < VTSS_RADIUS_NUMBER_OF_SERVERS; server++) {
            memset(&RADIUS_state.server_info[t].state[server], 0, sizeof(RADIUS_state.server_info[t].state[server]));
            RADIUS_state.server_info[t].state[server].sockd = -1;
        }
    }
}

/******************************************************************************/
// RADIUS_thread()
/******************************************************************************/
static void RADIUS_thread(vtss_addrword_t data)
{
    vtss_flag_value_t wait_result;
    vtss_tick_count_t cur_time_ticks;
    vtss_tick_count_t next_wakeup_ticks;
    int               i, t;

    while (1) {
        if (msg_switch_is_primary()) {
            while (msg_switch_is_primary()) {
                // As long as we're the primary switch, we should check for rx'd
                // RADIUS packets every 100 ms.
                cur_time_ticks =  vtss_current_time();
                next_wakeup_ticks = cur_time_ticks + VTSS_OS_MSEC2TICK(RADIUS_THREAD_TIMEOUT_MS);

                RADIUS_CRIT_ENTER();
                if (RADIUS_state.configured) {

                    if (!RADIUS_state.server_info[RADIUS_SERVER_TYPE_AUTH].initialized) {
                        // Initialize new auth sockets and call callbacks
                        RADIUS_socket_init(RADIUS_SERVER_TYPE_AUTH, TRUE);
                    }
                    if (!RADIUS_state.server_info[RADIUS_SERVER_TYPE_ACCT].initialized) {
                        // Initialize new acct sockets.
                        RADIUS_socket_init(RADIUS_SERVER_TYPE_ACCT, FALSE);
                    }

                    if (RADIUS_state.server_info[RADIUS_SERVER_TYPE_AUTH].initialized || RADIUS_state.server_info[RADIUS_SERVER_TYPE_ACCT].initialized) {
                        RADIUS_rx_check();
                        RADIUS_tx_check();
                    }
                }
                RADIUS_CRIT_EXIT();

                wait_result = 1;
                while (wait_result != 0) {
                    RADIUS_CRIT_ENTER();
                    if ((RADIUS_state.server_info[RADIUS_SERVER_TYPE_AUTH].active_server_cnt + RADIUS_state.server_info[RADIUS_SERVER_TYPE_ACCT].active_server_cnt) == 0) {
                        RADIUS_CRIT_EXIT();
                        wait_result = vtss_flag_wait(&RADIUS_state.thread_control_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR);
                    } else {
                        // vtss_flag_timed_wait() returns 0 on time out. If called with a next_wakeup_ticks which lies in the past, it returns immediately.
                        RADIUS_CRIT_EXIT();
                        wait_result = vtss_flag_timed_wait(&RADIUS_state.thread_control_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR, next_wakeup_ticks);
                    }
                    if (wait_result != 0) {
                        RADIUS_CRIT_ENTER();
                        if (wait_result & RADIUS_THREAD_FLAG_POS_CFG_CHANGED) {
                            // Tell all agents that they've been cancelled because of a configuration change.
                            RADIUS_call_rx_callbacks(VTSS_RADIUS_RX_CALLBACK_CFG_CHANGED);
                            // Uninitialize current sockets.
                            RADIUS_socket_uninit(RADIUS_SERVER_TYPE_AUTH, TRUE); // Call registered callbacks
                            RADIUS_socket_uninit(RADIUS_SERVER_TYPE_ACCT, FALSE);
                            // Get new cfg
                            RADIUS_cfg = RADIUS_tmp_cfg;
                            // Clear the current server_state, so that previous dead-times etc. is not in effect anymore.
                            RADIUS_clear_server_states();

                            for (t = 0; t < RADIUS_SERVER_TYPE_LAST; t++) {
                                for (i = 0; i < VTSS_RADIUS_NUMBER_OF_SERVERS; i++) {
                                    RADIUS_server_state_s     *server_state = &RADIUS_state.server_info[t].state[i];
                                    vtss_radius_server_info_t *server_cfg   = &RADIUS_state.server_info[t].config[i];

                                    // Get the number of active servers for fast computations.
                                    if (server_cfg->host[0]) {
                                        RADIUS_state.server_info[t].active_server_cnt++;
                                        server_state->server_id  = ++RADIUS_state.unique_server_id;
                                    }

                                    // Compute the number of ticks per retransmit of a frame to speed-up checks, since we only operate in tick units.
                                    // server_cfg->timeout is measured in seconds.
                                    server_state->timeout_per_retransmit_ticks = VTSS_OS_MSEC2TICK(1000 * (vtss_tick_count_t)server_cfg->timeout);
                                }
                                T_D("Active %s servers: %d", (t == RADIUS_SERVER_TYPE_AUTH) ? "auth" : "acct", RADIUS_state.server_info[t].active_server_cnt);
                            }

                            // Compute the dead-time in ticks to speed-up checks, since we only operate in tick units.
                            RADIUS_state.dead_time_per_server_ticks = VTSS_OS_MSEC2TICK(1000 * (vtss_tick_count_t)RADIUS_cfg.dead_time_secs);

                            // We're successfully configured.
                            RADIUS_state.configured = TRUE;
                            // Clear our counters, since it may be that we've been configured with new servers.
                            RADIUS_auth_client_mib_clr(-1);
                            RADIUS_acct_client_mib_clr(-1);
                            // Recalculate MD5 contexts for shared keys.
                            RADIUS_md5_precalc();
                        } // If configuration changed

                        if (wait_result & RADIUS_THREAD_FLAG_POS_MAX_ID_CNT_CHANGED) {
                            // Debug feature only.
                            // The maximum number of allowed IDs has changed. If the new count is bigger than the old, do nothing,
                            // except copying the new count. Otherwise we need to cancel all Rx callbacks.
                            if (RADIUS_tmp_max_id_cnt_minus_one < RADIUS_state.max_id_cnt_minus_one) {
                                RADIUS_call_rx_callbacks(VTSS_RADIUS_RX_CALLBACK_MAX_ID_CNT_CHANGED);
                                RADIUS_state.next_valid_id = 0; // Start all over.
                            }
                            RADIUS_state.max_id_cnt_minus_one = RADIUS_tmp_max_id_cnt_minus_one;
                        }
                        RADIUS_CRIT_EXIT();
                    }
                }
            }
        }

        // No reason for using CPU ressources when we're a secondary switch
        T_D("Suspending RADIUS thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_RADIUS);
        T_D("Resumed RADIUS thread");
    }
}

/******************************************************************************/
// RADIUS_tx_check_required_tlvs()
/******************************************************************************/
static inline mesa_rc RADIUS_tx_check_required_tlvs(RADIUS_id_state_s *id_state)
{
    RADIUS_CRIT_ASSERT_LOCKED();

    if (id_state->tx_frm[RADIUS_PKT_IDX_CODE] == VTSS_RADIUS_CODE_ACCOUNTING_REQUEST) {
        if (RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_ACCT_STATUS_TYPE, 0) == NULL) {
            return VTSS_RADIUS_ERROR_ATTRIB_MISSING_ACCT_STATUS_TYPE;
        }
        if (RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_ACCT_SESSION_ID, 0) == NULL) {
            return VTSS_RADIUS_ERROR_ATTRIB_MISSING_ACCT_SESSION_ID;
        }
    } else {
        u8 *user_pw_tlv_ptr, *eap_msg_tlv_ptr;
        user_pw_tlv_ptr = RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_USER_PASSWORD, 0);
        eap_msg_tlv_ptr = RADIUS_find_attribute(id_state->tx_frm, VTSS_RADIUS_ATTRIBUTE_EAP_MESSAGE,   0);

        // EAP-Message and User-Password attributes are mutually exclusive, but one of them must exist.
        if (eap_msg_tlv_ptr && user_pw_tlv_ptr) {
            return VTSS_RADIUS_ERROR_ATTRIB_BOTH_USER_PW_AND_EAP_MSG_FOUND;
        }
        if (!eap_msg_tlv_ptr && !user_pw_tlv_ptr) {
            return VTSS_RADIUS_ERROR_ATTRIB_NEITHER_USER_PW_NOR_EAP_MSG_FOUND;
        }
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
// RADIUS_get_dead_time_left_state()
// Returns -2 if server pointed to by @idx is not enabled.
// Returns -1 if RADIUS module is not yet initialized.
// Returns  0 if server pointed to by @idx is ready to transmit another frame right away.
// Otherwise returns the number of seconds (>0) left of current dead time.
/****************************************************************************/
static long RADIUS_get_dead_time_left_state(int idx, RADIUS_server_type_e server_type, vtss_tick_count_t cur_time_ticks)
{
    if (!RADIUS_state.server_info[server_type].config[idx].host[0]) {
        return -2;
    }

    if (!RADIUS_state.server_info[server_type].initialized) {
        return -1;
    }

    if (RADIUS_state.server_info[server_type].active_server_cnt == 1 || cur_time_ticks >= RADIUS_state.server_info[server_type].state[idx].next_time_to_try_this_server_ticks) {
        // When there's only one active server, we always attempt to transmit a RADIUS frame whether it's dead or not.
        return 0;
    }

    return (long)(VTSS_OS_TICK2MSEC(RADIUS_state.server_info[server_type].state[idx].next_time_to_try_this_server_ticks - cur_time_ticks) / 1000);
}

/****************************************************************************/
// RADIUS_compute_state_and_dead_time()
/****************************************************************************/
static void RADIUS_compute_state_and_dead_time(int idx, RADIUS_server_type_e server_type, vtss_tick_count_t cur_time_ticks, u32 *diff_time_secs, vtss_radius_server_state_e *state)
{
    RADIUS_CRIT_ASSERT_LOCKED();
    long diff_time_state;

    // Compute the time in seconds it takes before this server can be used again.
    diff_time_state = RADIUS_get_dead_time_left_state(idx, server_type, cur_time_ticks);
    if (diff_time_state == -2) {
        // This server is not enabled.
        *state = VTSS_RADIUS_SERVER_STATE_DISABLED;
    } else if (diff_time_state == -1) {
        // The RADIUS module is not yet ready.
        *state = VTSS_RADIUS_SERVER_STATE_NOT_READY;
    } else if (diff_time_state == 0) {
        // When there's only one server, we always attempt to transmit a RADIUS frame whether it's dead or not.
        *state = VTSS_RADIUS_SERVER_STATE_READY;
    } else {
        // More than one server
        *state = VTSS_RADIUS_SERVER_STATE_DEAD;
    }
    *diff_time_secs = diff_time_state < 0 ? 0 : diff_time_state;
}

/****************************************************************************/
// RADIUS_dbg_cmd_syntax_error()
/****************************************************************************/
static void RADIUS_dbg_cmd_syntax_error(msg_dbg_printf_t dbg_printf, const char *fmt, ...)
{
    va_list ap;
    char s[200] = "Command syntax error: ";
    int len;

    len = strlen(s);

    /*lint -e{530} ... 'ap' is initialized by va_start() */
    va_start(ap, fmt);

    (void)vsprintf(s + len, fmt, ap);
    (void)dbg_printf("%s\n", s);

    va_end(ap);
}

/******************************************************************************/
// RADIUS_dbg_cmd_cfg_max_id_cnt()
// cmd_text   : "Configure maximum number of available RADIUS IDs"
// arg_syntax : [<max_id_cnt>]
// max_arg_cnt: 1
/******************************************************************************/
static void RADIUS_dbg_cmd_cfg_max_id_cnt(vtss_radius_dbg_printf_f dbg_printf, ulong parms_cnt, ulong *parms)
{
    RADIUS_CRIT_ENTER();

    if (parms_cnt == 0) {
        // Don't wanna protect this, as it's just a debug function.
        (void)dbg_printf(
            "RADIUS IDs available: %3u\n"
            "RADIUS IDs in use:    %3u\n",
            (unsigned int)RADIUS_state.max_id_cnt_minus_one + 1,
            RADIUS_state.ids_in_use);
        goto do_exit;
    }

    if (parms_cnt != 1) {
        RADIUS_dbg_cmd_syntax_error(dbg_printf, "This function takes 0 or 1 argument, not %lu", parms_cnt);
        goto do_exit;
    }

    if (parms[0] == 0 || parms[0] > RADIUS_MAX_ID_CNT) {
        RADIUS_dbg_cmd_syntax_error(dbg_printf, "The number of IDs must be in range [1; %d]", RADIUS_MAX_ID_CNT);
        goto do_exit;
    }

    if (parms[0] - 1 != RADIUS_state.max_id_cnt_minus_one) {
        RADIUS_tmp_max_id_cnt_minus_one = parms[0] -  1;
        vtss_flag_setbits(&RADIUS_state.thread_control_flag, RADIUS_THREAD_FLAG_POS_MAX_ID_CNT_CHANGED); // Wake-up the thread
    }

do_exit:
    RADIUS_CRIT_EXIT();
}

/******************************************************************************/
/******************************************************************************/
typedef enum {
    RADIUS_DBG_CMD_STAT = 1,
    RADIUS_DBG_CMD_CFG_MAX_ID_CNT = 20,
} RADIUS_dbg_cmd_num_e;

/******************************************************************************/
/******************************************************************************/
typedef struct {
    RADIUS_dbg_cmd_num_e cmd_num;
    const char *cmd_txt;
    const char *arg_syntax;
    uint max_arg_cnt;
    void (*func)(vtss_radius_dbg_printf_f dbg_printf, ulong parms_cnt, ulong *parms);
} RADIUS_dbg_cmd_s;

/******************************************************************************/
/******************************************************************************/
static RADIUS_dbg_cmd_s RADIUS_dbg_cmds[] = {
    {
        RADIUS_DBG_CMD_CFG_MAX_ID_CNT,
        "Configure maximum number of available RADIUS IDs",
        "[<max_id_cnt>]",
        1,
        RADIUS_dbg_cmd_cfg_max_id_cnt
    },
    {
        (RADIUS_dbg_cmd_num_e)0,
        NULL,
        NULL,
        0,
        NULL
    }
};

/******************************************************************************/
//
//  PUBLIC FUNCTIONS
//
/******************************************************************************/

/******************************************************************************/
// vtss_radius_init_chg_callback_register()
/******************************************************************************/
mesa_rc vtss_radius_init_chg_callback_register(u32 *register_id, ctx_t ctx, vtss_radius_init_chg_callback_f cb)
{
    u32 i;
    mesa_rc result;

    T_D("Enter");

    if (!cb) {
        return VTSS_RADIUS_ERROR_CALLBACK;
    }

    RADIUS_CRIT_ENTER();
    for (i = 0; i < ARRSZ(RADIUS_init_chg_registrants); i++) {
        if (RADIUS_init_chg_registrants[i].callback == NULL) {
            RADIUS_init_chg_registrants[i].ctx      = ctx;
            RADIUS_init_chg_registrants[i].callback = cb;
            if (register_id) {
                *register_id = i;
            }

            result = VTSS_RC_OK;

            // RBNTBD: Callback *now* if already initialized. This may cause dead-lock problems
            if (RADIUS_state.server_info[RADIUS_SERVER_TYPE_AUTH].initialized) {
                cb(ctx, VTSS_RADIUS_INIT_CHG_CALLBACK_INITIALIZED);
            }

            goto do_exit;
        }
    }

    result = VTSS_RADIUS_ERROR_OUT_OF_REGISTRANT_ENTRIES;

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_init_chg_callback_unregister()
/******************************************************************************/
mesa_rc vtss_radius_init_chg_callback_unregister(u32 register_id)
{
    if (register_id >= ARRSZ(RADIUS_init_chg_registrants)) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    RADIUS_CRIT_ENTER();
    RADIUS_init_chg_registrants[register_id].callback = NULL;
    RADIUS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_radius_id_state_chg_callback_register()
/******************************************************************************/
mesa_rc vtss_radius_id_state_chg_callback_register(u32 *register_id, ctx_t ctx, vtss_radius_id_state_chg_callback_f cb)
{
    u32 i;
    mesa_rc result;

    T_D("Enter");

    if (!cb) {
        return VTSS_RADIUS_ERROR_CALLBACK;
    }

    RADIUS_CRIT_ENTER();
    for (i = 0; i < ARRSZ(RADIUS_id_state_chg_registrants); i++) {
        if (RADIUS_id_state_chg_registrants[i].callback == NULL) {
            RADIUS_id_state_chg_registrants[i].ctx      = ctx;
            RADIUS_id_state_chg_registrants[i].callback = cb;
            if (register_id) {
                *register_id = i;
            }
            result = VTSS_RC_OK;
            goto do_exit;
        }
    }
    result = VTSS_RADIUS_ERROR_OUT_OF_REGISTRANT_ENTRIES;
do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_id_state_chg_callback_unregister()
/******************************************************************************/
mesa_rc vtss_radius_id_state_chg_callback_unregister(u32 register_id)
{
    if (register_id >= ARRSZ(RADIUS_id_state_chg_registrants)) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    RADIUS_CRIT_ENTER();
    RADIUS_id_state_chg_registrants[register_id].callback = NULL;
    RADIUS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// radius_cfg_get()
// Get the current RADIUS configuration
/******************************************************************************/
mesa_rc vtss_radius_cfg_get(vtss_radius_cfg_s *cfg)
{
    mesa_rc result;
    T_D("Enter");

    if (!cfg) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    RADIUS_CRIT_ENTER();

    if (!RADIUS_state.configured) {
        result = VTSS_RADIUS_ERROR_NOT_CONFIGURED;
        goto do_exit;
    }

    *cfg = RADIUS_cfg;
    result = VTSS_RC_OK;

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// radius_cfg_set()
// Sets the current RADIUS configuration
/******************************************************************************/
mesa_rc vtss_radius_cfg_set(vtss_radius_cfg_s *cfg)
{
    vtss_radius_server_info_t *server_info[RADIUS_SERVER_TYPE_LAST];
    int                       t, server, i, len;
    u32                       msb, ip_addr;
    struct addrinfo           hints;
    struct addrinfo           *res;

    T_D("Enter");

    if (!cfg) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    // Check that NAS_IDENTIFIER is zero terminated and not greater than 253 chars
    len = MIN(254, ARRSZ(cfg->nas_identifier)); // len includes zero terminator
    for (i = 0; i < len; i++) {
        if (cfg->nas_identifier[i] == '\0') {
            break;
        }
    }
    if (i == len) {
        return VTSS_RADIUS_ERROR_ATTRIB_NAS_IDENTIFIER_TOO_LONG;
    }

    server_info[RADIUS_SERVER_TYPE_AUTH] = &cfg->servers_auth[0];
    server_info[RADIUS_SERVER_TYPE_ACCT] = &cfg->servers_acct[0];

    for (t = 0; t < RADIUS_SERVER_TYPE_LAST; t++) {
        for (server = 0; server < VTSS_RADIUS_NUMBER_OF_SERVERS; server++) {
            vtss_radius_server_info_t *server_cfg = &server_info[t][server];
            if (server_cfg->host[0]) {
                // RADIUS host must be NULL terminated within VTSS_RADIUS_KEY_LEN chars
                for (i = 0; i < ARRSZ(server_cfg->host); i++) {
                    if (server_cfg->host[i] == '\0') {
                        break;
                    }
                }
                if (i == ARRSZ(server_cfg->host)) {
                    return VTSS_RADIUS_ERROR_CFG_IP;
                }

                memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_DGRAM;
                hints.ai_protocol = IPPROTO_UDP;
                if (getaddrinfo(server_cfg->host, NULL, &hints, &res) == 0) {
                    if (res->ai_family == AF_INET) { // IPv4: Check that the RADIUS IP is a legal unicast address
                        struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
                        ip_addr = htonl(sin->sin_addr.s_addr);
                        msb = (vtss_radius_access_codes_e)((ip_addr >> 24) & 0xFF);
                        if (msb == 0 || msb == 127 || msb > 223) {
                            freeaddrinfo(res);
                            return VTSS_RADIUS_ERROR_CFG_IP;
                        }
                    }
                    freeaddrinfo(res);
                }

                // Check timeouts.
                if (server_cfg->timeout < 1) {
                    return VTSS_RADIUS_ERROR_CFG_TIMEOUT;
                }


                // RADIUS key must be NULL terminated within VTSS_RADIUS_KEY_LEN chars
                for (i = 0; i < ARRSZ(server_cfg->key); i++) {
                    if (server_cfg->key[i] == '\0') {
                        break;
                    }
                }
                if (i == ARRSZ(server_cfg->key)) {
                    return VTSS_RADIUS_ERROR_CFG_SECRET;
                }
                // The array cannot contain the same server:port twice.
                for (i = 0; i < server; i++) {
                    if (cfg->servers_auth[i].host[0] &&
                        (strcmp(server_cfg->host, cfg->servers_auth[i].host) == 0) &&
                        (server_cfg->port == cfg->servers_auth[i].port)) {
                        return VTSS_RADIUS_ERROR_CFG_SAME_SERVER_AND_PORT;
                    }
                }
            }
        }
    }

    RADIUS_CRIT_ENTER();
    if (memcmp(&RADIUS_tmp_cfg, cfg, sizeof(RADIUS_tmp_cfg)) != 0) {
        RADIUS_tmp_cfg = *cfg;
        vtss_flag_setbits(&RADIUS_state.thread_control_flag, RADIUS_THREAD_FLAG_POS_CFG_CHANGED); // Wake-up the thread
    }
    RADIUS_CRIT_EXIT();
    T_D("Exit");
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_radius_alloc()
/******************************************************************************/
mesa_rc vtss_radius_alloc(u8 *handle, vtss_radius_access_codes_e code)
{
    u8                   id;
    BOOL                 found = FALSE;
    mesa_rc              result = VTSS_RC_OK;
    u8                   temp_str[4];
    RADIUS_id_state_s    *id_state;
    RADIUS_server_type_e server_type;

    T_D("Enter");

    if ((code != VTSS_RADIUS_CODE_ACCESS_REQUEST && code != VTSS_RADIUS_CODE_ACCOUNTING_REQUEST) || handle == NULL) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    RADIUS_CRIT_ENTER();

    if (!RADIUS_state.configured) {
        result = VTSS_RADIUS_ERROR_NOT_CONFIGURED;
        goto do_exit;
    }

    server_type = (code == VTSS_RADIUS_CODE_ACCESS_REQUEST) ? RADIUS_SERVER_TYPE_AUTH : RADIUS_SERVER_TYPE_ACCT;

    if (!RADIUS_state.server_info[server_type].initialized) {
        result = VTSS_RADIUS_ERROR_NOT_INITIALIZED;
        goto do_exit;
    }

    VTSS_ASSERT(RADIUS_state.next_valid_id <= RADIUS_state.max_id_cnt_minus_one);
    id = RADIUS_state.next_valid_id;

    // Find an ID that is not in use.
    // Start with the next_valid_id and stop when we've wrapped around or
    // we found an unused one.
    do {
        if (!RADIUS_state.id_states[id].in_use) {
            found = TRUE;
            break;
        }
        // Limit the number of IDs (which is useful in case we're debugging).
        if (id == RADIUS_state.max_id_cnt_minus_one) {
            id = 0;
        } else {
            id++;
        }
    } while (id != RADIUS_state.next_valid_id); // Stop when wrapped around.

    if (!found) {
        T_D("Out of handles.");
        result = VTSS_RADIUS_ERROR_OUT_OF_HANDLES;
        goto do_exit;
    }

    if (RADIUS_state.ids_in_use  == (int)RADIUS_state.max_id_cnt_minus_one) {
        // Tell whoever wants to know, that we've run out of IDs.
        RADIUS_call_id_state_chg_callbacks(VTSS_RADIUS_ID_STATE_CHG_CALLBACK_OUT_OF_IDS);
    }

    *handle = id;
    id_state = &RADIUS_state.id_states[id];

    if (id_state->tx_frm == NULL) {
        id_state->tx_frm = (u8 *)VTSS_MALLOC(RADIUS_MAX_FRAME_SIZE_BYTES);
        if (!id_state->tx_frm) {
            result = VTSS_RADIUS_ERROR_OUT_OF_HANDLES;
            goto do_exit;
        }

        memset(id_state->tx_frm, 0, RADIUS_MAX_FRAME_SIZE_BYTES);
        T_D("Allocated tx frame buffer for handle %d", id);
    }

    RADIUS_state.ids_in_use++;

    if (RADIUS_state.next_valid_id == RADIUS_state.max_id_cnt_minus_one) {
        RADIUS_state.next_valid_id = 0;
    } else {
        RADIUS_state.next_valid_id++;
    }

    id_state->tx_frm[RADIUS_PKT_IDX_CODE] = code;
    id_state->tx_frm[RADIUS_PKT_IDX_ID]   = id;
    id_state->in_use = TRUE;
    id_state->waiting_for_callback_to_finish = FALSE;
    id_state->server_type = server_type;
    id_state->server_idx = -2;               // The Tx frame is not filled in by the client yet.
    id_state->use_this_server_idx_only = -1; // Use best server
    id_state->callback = NULL;

    // Make the tlv_ptr point to the next free TLV in the frame that is going
    // to be transmitted. Once the agent has filled in all TLVs, and has called
    // the vtss_radius_tx() function, this pointer will be re-used as a pointer
    // into the received packet, so that the agent may call vtss_radius_tlv_iterate()
    // successively.
    id_state->tlv_ptr = &id_state->tx_frm[RADIUS_PKT_IDX_FIRST_ATTRIB];

    if (code == VTSS_RADIUS_CODE_ACCESS_REQUEST) {
        // By default, we provide the MTU TLV to ask the server to limit the size of its packets.
        temp_str[0] = temp_str[1] = 0;
        temp_str[2] = (RADIUS_MAX_FRAMED_MTU_SIZE_BYTES >> 8) & 0xFF;
        temp_str[3] = (RADIUS_MAX_FRAMED_MTU_SIZE_BYTES >> 0) & 0xFF;

        if (RADIUS_add_tlv(id_state, VTSS_RADIUS_ATTRIBUTE_FRAMED_MTU, 6, temp_str) == NULL) {
            result = VTSS_RADIUS_ERROR_NOT_ROOM_FOR_TLV;
            goto do_exit;
        }
    }

    T_D("Exit(%u)", *handle);
    result = VTSS_RC_OK;

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_free()
/******************************************************************************/
mesa_rc vtss_radius_free(u8 handle)
{
    mesa_rc result;
    RADIUS_id_state_s *id_state;

    T_D("Enter(%u)", handle);

    RADIUS_CRIT_ENTER();

    id_state = &RADIUS_state.id_states[handle];
    if (!id_state->in_use) {
        result = VTSS_RADIUS_ERROR_INVALID_HANDLE;
        goto do_exit;
    }

    RADIUS_dealloc_handle(id_state);
    result = VTSS_RC_OK;

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_tlv_iterate()
// This function and vtss_radius_tlv_get() may *ONLY* be called from the
// callback function.
/******************************************************************************/
mesa_rc vtss_radius_tlv_iterate(u8 handle, vtss_radius_attributes_e *type, u8 *len, u8 const **val, BOOL start_over)
{
    RADIUS_id_state_s        *id_state = &RADIUS_state.id_states[handle];
    u8                       *end      = RADIUS_rx_buffer + RADIUS_frm_len_get(RADIUS_rx_buffer);
    u8                       l, *v;
    vtss_radius_attributes_e t;
    mesa_rc                  result;

    T_D("Enter(%u)", handle);

    if (type == NULL || len == NULL || val == NULL) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    // This function MUST be called from the rx callback.
    RADIUS_CRIT_ENTER();

    if (!id_state->in_use || RADIUS_rx_buffer[RADIUS_PKT_IDX_ID] != handle) {
        result = VTSS_RADIUS_ERROR_INVALID_HANDLE;
        goto do_exit;
    }

    if (start_over) {
        id_state->tlv_ptr = &RADIUS_rx_buffer[RADIUS_PKT_IDX_FIRST_ATTRIB];
    }

    while (id_state->tlv_ptr < end) {
        t =  (vtss_radius_attributes_e)(*(id_state->tlv_ptr + 0));
        l =  *(id_state->tlv_ptr + 1);
        v =    id_state->tlv_ptr + 2;
        *type = t;
        *len  = l - 2; // Compensate for Type + Len overhead. It is checked in RADIUS_check_length() that l is >= 2.
        *val  = v;
        id_state->tlv_ptr += l;

        // Skip Message-Authenticator attributes, since these are internal to RADIUS protocol.
        if (t != VTSS_RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR) {
            result = VTSS_RC_OK;
            goto do_exit;
        }
    }

    result = VTSS_RADIUS_ERROR_NO_MORE_TLVS;

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_tlv_get()
// This function and vtss_radius_tlv_iterate() may *ONLY* be called from the
// callback function.
/******************************************************************************/
mesa_rc vtss_radius_tlv_get(u8 handle, vtss_radius_attributes_e type, u16 *len, u8 *val)
{
    RADIUS_id_state_s *id_state = &RADIUS_state.id_states[handle];
    u8                *v;
    u16               l = 0;
    int               idx = 0;
    mesa_rc           result;

    T_D("Enter(%u)", handle);

    if (len == NULL || val == NULL || *len == 0) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    // This function MUST be called from the rx callback.
    RADIUS_CRIT_ENTER();

    if (!id_state->in_use || RADIUS_rx_buffer[RADIUS_PKT_IDX_ID] != handle) {
        result = VTSS_RADIUS_ERROR_INVALID_HANDLE;
        goto do_exit;
    }

    while (1) {
        v = RADIUS_find_attribute(RADIUS_rx_buffer, type, idx++);
        if (v) {
            u32 tlv_value_len = v[1] - 2;
            l += tlv_value_len;
            if (l > *len) {
                result = VTSS_RADIUS_ERROR_NOT_ROOM_FOR_TLV;
                goto do_exit;
            }
            if (tlv_value_len > 0) {
                memcpy(val, v + 2, tlv_value_len);
                val += tlv_value_len;
            }
        } else {
            break;
        }
    }

    if (l == 0) {
        result = VTSS_RADIUS_ERROR_TLV_NOT_FOUND;
        goto do_exit;
    }

    *len = l;
    result = VTSS_RC_OK;

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_tlv_set()
/******************************************************************************/
mesa_rc vtss_radius_tlv_set(u8 handle, vtss_radius_attributes_e type, u16 len, const u8 *val, BOOL dealloc)
{
    RADIUS_id_state_s *id_state;
    mesa_rc           result = VTSS_RC_OK;
    int               tlv_cnt, tot_len, orig_len = len;

    T_D("Enter(%u)", handle);

    if ((type != VTSS_RADIUS_ATTRIBUTE_USER_PASSWORD) && (len == 0)) {
        // Guess empty TLVs don't make sense for anything else than user passwords?
        return VTSS_RC_OK;
    }

    if (val == NULL) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (type == VTSS_RADIUS_ATTRIBUTE_CHAP_PASSWORD) {
        // There's no support for CHAP passwords (yet).
        return VTSS_RADIUS_ERROR_ATTRIB_UNSUPPORTED;
    }

    if (type == VTSS_RADIUS_ATTRIBUTE_NAS_IP_ADDRESS ||
        type == VTSS_RADIUS_ATTRIBUTE_NAS_IDENTIFIER ||
        type == VTSS_RADIUS_ATTRIBUTE_NAS_IPV6_ADDRESS ||
        type == VTSS_RADIUS_ATTRIBUTE_FRAMED_MTU ||
        type == VTSS_RADIUS_ATTRIBUTE_MESSAGE_AUTHENTICATOR) {
        // These attributes are appended by this module, so the user shouldn't do it.
        return VTSS_RADIUS_ERROR_ATTRIB_AUTO_APPENDED;
    }

    RADIUS_CRIT_ENTER();
    id_state = &RADIUS_state.id_states[handle];

    if (!id_state->in_use) {
        result = VTSS_RADIUS_ERROR_INVALID_HANDLE;
        goto do_exit;
    }

    if (type == VTSS_RADIUS_ATTRIBUTE_USER_PASSWORD) {
        // We gotta save the password as sideband info, because we may need to re-calculate the MD5-hash
        // if we're changing server.
        unsigned int pw_len = strlen((char *)val);
        if (pw_len >= sizeof(id_state->user_pw)) {
            result = VTSS_RADIUS_ERROR_ATTRIB_PASSWORD_TOO_LONG;
            goto do_exit;
        }
        strcpy((char *)id_state->user_pw, (char *)val);
        // Make room for the password in the tx_frm. The size must be the next multiple of 16 of the pw length.
        if (pw_len == 0) {
            // Special case.
            len = 16;
        } else {
            len = 16 * ((pw_len + 15) / 16);
        }
        // The correct value of this TLV is updated upon transmission, or when changing the server.
    }

    // Compute the required length up front, so that we don't add half of a TLV.
    // If the len > 253, we add multiple TLVs of the same type.
    if (len > 253) {
        tlv_cnt = (len / 253) + 1;
        if (len % 253 == 0) {
            tlv_cnt--;
        }
    } else {
        tlv_cnt = 1;
    }

    // Two bytes are needed for Type and Length fields per TLV.
    tot_len = len + (2 * tlv_cnt);

    // Check if there's room for the TLV(s).
    if (!RADIUS_add_tlv_len_check(id_state, type, tot_len, false /* don't check tot_len is <= 255 */)) {
        result = VTSS_RADIUS_ERROR_NOT_ROOM_FOR_TLV;
        if (dealloc) {
            // This appears to be a mandatory TLV, so the frame shall never be
            // sent, so we should dealloc the handle.
            RADIUS_dealloc_handle(id_state);
        }

        goto do_exit;
    }

    // If len > 253, we add multiple TLVs.
    while (len > 0) {
        int act_len = len > 253 ? 253 : len;
        // Add the attribute(s). len must accommodate for the type and length
        // fields in the call to this function.
        if (RADIUS_add_tlv(id_state, type, act_len + 2, val) == NULL) {
            T_E("Huh? Already checked that there's room for this TLV of length %u (now %u) => total length %u", orig_len, len, tot_len);
            result = VTSS_RADIUS_ERROR_NOT_ROOM_FOR_TLV;
            if (dealloc) {
                // This appears to be a mandatory TLV, so the frame shall never be sent, so we should dealloc the handle.
                RADIUS_dealloc_handle(id_state);
            }

            goto do_exit;
        }

        len -= act_len;
        /*lint -e{662} Supress Lint Warning 662: Possible creation of out-of-bounds pointer */
        val += act_len;
    }

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_tx()
/******************************************************************************/
mesa_rc vtss_radius_tx(u8 handle, ctx_t ctx, vtss_radius_rx_callback_f callback, u32 use_this_server_id)
{
    RADIUS_id_state_s *id_state;
    mesa_rc           result;
    BOOL              dealloc_handle = FALSE;
    int               i, server_idx = -1;

    T_D("Enter(%u, use_this_server_id = %u)", handle, use_this_server_id);

    if (callback == NULL) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    RADIUS_CRIT_ENTER();

    id_state = &RADIUS_state.id_states[handle];
    if (!id_state->in_use) {
        result = VTSS_RADIUS_ERROR_INVALID_HANDLE;
        goto do_exit;
    }

    // Avoid race-conditions in case primary switch goes down now, by putting
    // this check within the RADIUS_CRIT_ENTER() call.
    if (!msg_switch_is_primary()) {
        result = VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
        dealloc_handle = TRUE;
        goto do_exit;
    }

    if (!RADIUS_state.server_info[id_state->server_type].initialized) {
        result = VTSS_RADIUS_ERROR_NOT_INITIALIZED;
        dealloc_handle = TRUE;
        goto do_exit;
    }

    if (use_this_server_id != 0) {
        // The user wants a specific server, probably because he has an ongoing conversation.
        // Check to see if we can find it (must match server type).
        for (i = 0; i < VTSS_RADIUS_NUMBER_OF_SERVERS; i++) {
            if (RADIUS_state.server_info[id_state->server_type].state[i].server_id == use_this_server_id) {
                server_idx = i;
                T_D("Found server_idx = %d", server_idx);
                break;
            }
        }

        // If no server with this ID was found, we'll default to starting all over.
        // The reason that no such server was found is probably that administrator
        // has re-configured the RADIUS servers.
        // Could return an error, but that makes it harder for the caller. Perhaps better to
        // just start all over. The RADIUS server will probably say "what?" if it's not the same
        // as before and a conversation is ongoing.
    }

    // Check if the required TLVs are in the tx frame. If not, de-allocate the handle and return an error.
    // We gotta update the length field before we call RADIUS_tx_check_required_tlvs(), because it uses
    // the frame data itself to find the seeked attributes.
    RADIUS_frm_len_set(id_state->tx_frm, id_state->tlv_ptr - id_state->tx_frm);
    if ((result = RADIUS_tx_check_required_tlvs(id_state)) != VTSS_RC_OK) {
        dealloc_handle = TRUE;
        goto do_exit;
    }

    id_state->callback = callback;
    id_state->ctx      = ctx;

    // Tell the RADIUS thread to find a server and transmit this frame ASAP once woken up.
    id_state->server_idx = -1;
    id_state->use_this_server_idx_only = server_idx;

    // Wake-up the thread right away, so that this frame can be transmitted. It must be transmitted
    // the first time from the RADIUS thread as well, because it might be that all servers are currently
    // down, so that we actually can't transmit it, in which case we must provide an error code
    // in the callback's result code.
    vtss_flag_setbits(&RADIUS_state.thread_control_flag, RADIUS_THREAD_FLAG_POS_WAKE_UP_NOW);

    result = VTSS_RC_OK;

do_exit:
    if (dealloc_handle) {
        RADIUS_dealloc_handle(id_state);
    }

    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_auth_ready()
// This function only checks if any of the authentication servers are ready
/******************************************************************************/
BOOL vtss_radius_auth_ready(void)
{
    BOOL             result = FALSE;
    vtss_tick_count_t cur_time_ticks;
    int              server;

    RADIUS_CRIT_ENTER();

    if (!RADIUS_state.server_info[RADIUS_SERVER_TYPE_AUTH].initialized) {
        goto do_exit;
    }

    if (RADIUS_state.server_info[RADIUS_SERVER_TYPE_AUTH].active_server_cnt == 1) {
        // If there's only one enabled server, we always attempt to transmit the frame to it,
        // whether it has previously been responding or not.
        result = TRUE;
        goto do_exit;
    }

    cur_time_ticks = vtss_current_time();
    for (server = 0; server < VTSS_RADIUS_NUMBER_OF_SERVERS; server++) {
        if (RADIUS_cfg.servers_auth[server].host[0] && cur_time_ticks >= RADIUS_state.server_info[RADIUS_SERVER_TYPE_AUTH].state[server].next_time_to_try_this_server_ticks) {
            // This server is enabled and its dead-time has been exceeded.
            result = TRUE;
            goto do_exit;
        }
    }

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_acct_ready()
// This function only checks if any of the accounting servers are ready
/******************************************************************************/
BOOL vtss_radius_acct_ready(void)
{
    BOOL             result = FALSE;
    vtss_tick_count_t cur_time_ticks;
    int              server;

    RADIUS_CRIT_ENTER();

    if (!RADIUS_state.server_info[RADIUS_SERVER_TYPE_ACCT].initialized) {
        goto do_exit;
    }

    if (RADIUS_state.server_info[RADIUS_SERVER_TYPE_ACCT].active_server_cnt == 1) {
        // If there's only one enabled server, we always attempt to transmit the frame to it,
        // whether it has previously been responding or not.
        result = TRUE;
        goto do_exit;
    }

    cur_time_ticks = vtss_current_time();
    for (server = 0; server < VTSS_RADIUS_NUMBER_OF_SERVERS; server++) {
        if (RADIUS_cfg.servers_acct[server].host[0] && cur_time_ticks >= RADIUS_state.server_info[RADIUS_SERVER_TYPE_ACCT].state[server].next_time_to_try_this_server_ticks) {
            // This server is enabled and its dead-time has been exceeded.
            result = TRUE;
            goto do_exit;
        }
    }

do_exit:
    RADIUS_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// vtss_radius_auth_client_mib_get()
/******************************************************************************/
mesa_rc vtss_radius_auth_client_mib_get(int idx, vtss_radius_auth_client_server_mib_s *mib)
{
    vtss_radius_server_state_e state;
    u32                        diff_time_secs;
    struct addrinfo            hints;
    struct addrinfo            *res;

    if (!mib) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (idx < 0 || idx >= ARRSZ(RADIUS_auth_client_mib.radiusAuthServerExtTable)) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    RADIUS_CRIT_ENTER();
    *mib = RADIUS_auth_client_mib.radiusAuthServerExtTable[idx];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(RADIUS_cfg.servers_auth[idx].host, NULL, &hints, &res)) {
        mib->radiusAuthServerInetAddress = 0;
    } else {
        struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
        mib->radiusAuthServerInetAddress = htonl(sin->sin_addr.s_addr);
        freeaddrinfo(res);
    }

    mib->radiusAuthClientServerInetPortNumber = RADIUS_cfg.servers_auth[idx].port;
    // Get state and dead time left.
    RADIUS_compute_state_and_dead_time(idx, RADIUS_SERVER_TYPE_AUTH, vtss_current_time(), &diff_time_secs, &state);
    mib->state               = state;
    mib->dead_time_left_secs = diff_time_secs;
    RADIUS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_radius_acct_client_mib_get()
/******************************************************************************/
mesa_rc vtss_radius_acct_client_mib_get(int idx, vtss_radius_acct_client_server_mib_s *mib)
{
    vtss_radius_server_state_e state;
    u32                        diff_time_secs;
    struct addrinfo            hints;
    struct addrinfo            *res;

    if (!mib) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (idx < 0 || idx >= ARRSZ(RADIUS_acct_client_mib.radiusAccServerExtTable)) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    RADIUS_CRIT_ENTER();
    *mib = RADIUS_acct_client_mib.radiusAccServerExtTable[idx];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(RADIUS_cfg.servers_acct[idx].host, NULL, &hints, &res)) {
        mib->radiusAccServerInetAddress = 0;
    } else {
        struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
        mib->radiusAccServerInetAddress = htonl(sin->sin_addr.s_addr);
        freeaddrinfo(res);
    }

    mib->radiusAccClientServerInetPortNumber = RADIUS_cfg.servers_acct[idx].port;
    // Get state and dead time left.
    RADIUS_compute_state_and_dead_time(idx, RADIUS_SERVER_TYPE_ACCT, vtss_current_time(), &diff_time_secs, &state);
    mib->state               = state;
    mib->dead_time_left_secs = diff_time_secs;
    RADIUS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_radius_auth_client_mib_clr()
/******************************************************************************/
mesa_rc vtss_radius_auth_client_mib_clr(int idx)
{
    if (!msg_switch_is_primary()) {
        return VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (idx < 0 || idx >= ARRSZ(RADIUS_auth_client_mib.radiusAuthServerExtTable)) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    RADIUS_CRIT_ENTER();
    RADIUS_auth_client_mib_clr(idx);
    RADIUS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_radius_acct_client_mib_clr()
/******************************************************************************/
mesa_rc vtss_radius_acct_client_mib_clr(int idx)
{
    if (!msg_switch_is_primary()) {
        return VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (idx < 0 || idx >= ARRSZ(RADIUS_acct_client_mib.radiusAccServerExtTable)) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    RADIUS_CRIT_ENTER();
    RADIUS_acct_client_mib_clr(idx);
    RADIUS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_radius_auth_server_status_get()
/******************************************************************************/
mesa_rc vtss_radius_auth_server_status_get(vtss_radius_all_server_status_s *status)
{
    vtss_tick_count_t          cur_time_ticks;
    u32                        diff_time_secs;
    int                        i;

    if (!status) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    RADIUS_CRIT_ENTER();
    cur_time_ticks = vtss_current_time();

    for (i = 0; i < VTSS_RADIUS_NUMBER_OF_SERVERS; i++) {
        misc_strncpyz(status->status[i].host, RADIUS_cfg.servers_auth[i].host, sizeof(status->status[i].host));
        status->status[i].port                = RADIUS_cfg.servers_auth[i].port;
        RADIUS_compute_state_and_dead_time(i, RADIUS_SERVER_TYPE_AUTH, cur_time_ticks, &diff_time_secs, &status->status[i].state);
        status->status[i].dead_time_left_secs = diff_time_secs;
    }
    RADIUS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_radius_acct_server_status_get()
/******************************************************************************/
mesa_rc vtss_radius_acct_server_status_get(vtss_radius_all_server_status_s *status)
{
    vtss_tick_count_t          cur_time_ticks;
    u32                        diff_time_secs;
    int                        i;

    if (!status) {
        return VTSS_RADIUS_ERROR_PARAM;
    }

    if (!msg_switch_is_primary()) {
        return VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    RADIUS_CRIT_ENTER();
    cur_time_ticks = vtss_current_time();

    for (i = 0; i < VTSS_RADIUS_NUMBER_OF_SERVERS; i++) {
        misc_strncpyz(status->status[i].host, RADIUS_cfg.servers_acct[i].host, sizeof(status->status[i].host));
        status->status[i].port                = RADIUS_cfg.servers_acct[i].port;
        RADIUS_compute_state_and_dead_time(i, RADIUS_SERVER_TYPE_ACCT, cur_time_ticks, &diff_time_secs, &status->status[i].state);
        status->status[i].dead_time_left_secs = diff_time_secs;
    }
    RADIUS_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_radius_dbg()
/******************************************************************************/
void vtss_radius_dbg(vtss_radius_dbg_printf_f dbg_printf, ulong parms_cnt, ulong *parms)
{
    int i;
    ulong cmd_num;
    if (parms_cnt == 0) {
        (void)dbg_printf("Usage: debug radius <cmd idx>\n");
        (void)dbg_printf("Most commands show current settings if called without arguments\n\n");
        (void)dbg_printf("Commands:\n");
        i = 0;
        while (RADIUS_dbg_cmds[i].cmd_num != 0) {
            (void)dbg_printf("  %2d: %s\n", RADIUS_dbg_cmds[i].cmd_num, RADIUS_dbg_cmds[i].cmd_txt);
            if (RADIUS_dbg_cmds[i].arg_syntax && RADIUS_dbg_cmds[i].arg_syntax[0]) {
                (void)dbg_printf("      Arguments: %s.\n", RADIUS_dbg_cmds[i].arg_syntax);
            }
            i++;
        }
        return;
    }

    cmd_num = parms[0];

    // Verify that command is known and argument count is correct
    i = 0;
    while (RADIUS_dbg_cmds[i].cmd_num != 0) {
        if (RADIUS_dbg_cmds[i].cmd_num == cmd_num) {
            break;
        }
        i++;
    }
    if (RADIUS_dbg_cmds[i].cmd_num == 0) {
        RADIUS_dbg_cmd_syntax_error(dbg_printf, "Unknown command number: %lu", cmd_num);
        return;
    }
    if (parms_cnt - 1 > RADIUS_dbg_cmds[i].max_arg_cnt) {
        RADIUS_dbg_cmd_syntax_error(dbg_printf, "Incorrect number of arguments (%lu).\n"
                                    "Arguments: %s",
                                    parms_cnt - 1,
                                    RADIUS_dbg_cmds[i].arg_syntax);
        return;
    }
    if (RADIUS_dbg_cmds[i].func == NULL) {
        (void)dbg_printf("Internal Error: Function for command " VPRIlu" not implemented (yet?)", cmd_num);
        return;
    }

    RADIUS_dbg_cmds[i].func(dbg_printf, parms_cnt - 1, parms + 1);
}

/****************************************************************************/
// vtss_radius_error_txt()
/****************************************************************************/
const char *vtss_radius_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_RADIUS_ERROR_PARAM:
        return "Invalid parameter (e.g. function called with NULL-pointer)";

    case VTSS_RADIUS_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on primary switch";

    case VTSS_RADIUS_ERROR_NOT_CONFIGURED:
        return "RADIUS not configured";

    case VTSS_RADIUS_ERROR_NOT_INITIALIZED:
        return "RADIUS configured, but not yet initialized. Waiting for IP stack to come up";

    case VTSS_RADIUS_ERROR_NO_NAME:
        return "Unable to resolve hostname via DNS";

    case VTSS_RADIUS_ERROR_CFG_TIMEOUT:
        return "Invalid timeout_secs configuration parameter";

    case VTSS_RADIUS_ERROR_CFG_DEAD_TIME:
        return "Invalid dead-time configuration parameter";

    case VTSS_RADIUS_ERROR_CFG_IP:
        return "Invalid IP address was detected in configuration parameter";

    case VTSS_RADIUS_ERROR_CFG_SECRET:
        return "Shared secret is not NULL-terminated or is zero-length";

    case VTSS_RADIUS_ERROR_CFG_SAME_SERVER_AND_PORT:
        return "Same server and UDP port number is configured at least twice";

    case VTSS_RADIUS_ERROR_CALLBACK:
        return "Invalid callback function specified";

    case VTSS_RADIUS_ERROR_OUT_OF_REGISTRANT_ENTRIES:
        return "No more vacant entries in the registrants array. Increase RADIUS_(INIT_CHG/ID_STATE_CHG)_REGISTRANT_MAX_CNT";

    case VTSS_RADIUS_ERROR_INVALID_HANDLE:
        return "The handle parameter supplied to the function is invalid";

    case VTSS_RADIUS_ERROR_ATTRIB_UNSUPPORTED:
        return "The RADIUS module doesn't support this attribute (currently this is VTSS_RADIUS_ATTRIBUTE_CHAP_PASSWORD)";

    case VTSS_RADIUS_ERROR_ATTRIB_AUTO_APPENDED:
        return "The RADIUS module will append this attribute itself if needed.";

    case VTSS_RADIUS_ERROR_ATTRIB_PASSWORD_TOO_LONG:
        return "The RADIUS module doesn't support User-Passwords longer than VTSS_SYS_PASSWD_LEN";

    case VTSS_RADIUS_ERROR_ATTRIB_NAS_IDENTIFIER_TOO_LONG:
        return "The RADIUS module doesn't support NAS-Identifier longer than 253";

    case VTSS_RADIUS_ERROR_ATTRIB_MISSING_ACCT_STATUS_TYPE:
        return "Missing Acct-Status-Type attribute.";

    case VTSS_RADIUS_ERROR_ATTRIB_MISSING_ACCT_SESSION_ID:
        return "Missing Acct-Session-Id attribute.";

    case VTSS_RADIUS_ERROR_ATTRIB_BOTH_USER_PW_AND_EAP_MSG_FOUND:
        return "Both a User-Password and EAP-Message attribute was found in the frame to tx. This makes it hard to compute an MD5";

    case VTSS_RADIUS_ERROR_ATTRIB_NEITHER_USER_PW_NOR_EAP_MSG_FOUND:
        return "Neither a User-Password nor an EAP-Message attribute was found in the frame to tx. This makes it hard to compute an MD5";

    case VTSS_RADIUS_ERROR_NO_MORE_TLVS:
        return "Returned by vtss_radius_tlv_iterate() when there are no more TLVs in the received frame";

    case VTSS_RADIUS_ERROR_NOT_ROOM_FOR_TLV:
        return "Returned by vtss_radius_tlv_set() when the TLV is too large to fit into the pre-allocated Tx frame, or by vtss_radius_tlv_get() when there's not room in the supplied array";

    case VTSS_RADIUS_ERROR_TLV_NOT_FOUND:
        return "Returned by vtss_radius_tlv_get() when the requested TLV was not found in the frame";

    case VTSS_RADIUS_ERROR_OUT_OF_HANDLES:
        return "Temporarily out of RADIUS handles";

    default:
        return "RADIUS: Unknown error code";
    }
}

/******************************************************************************/
// vtss_radius_init()
// Initialize RADIUS Module
/******************************************************************************/
mesa_rc vtss_radius_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        // Reset state and configuration.
        memset(&RADIUS_state,                    0, sizeof(RADIUS_state));
        memset(&RADIUS_cfg,                      0, sizeof(RADIUS_cfg));
        memset(RADIUS_init_chg_registrants,      0, sizeof(RADIUS_init_chg_registrants));
        memset(RADIUS_id_state_chg_registrants,  0, sizeof(RADIUS_id_state_chg_registrants));
        memset(&RADIUS_auth_client_mib,          0, sizeof(RADIUS_auth_client_mib));
        memset(&RADIUS_tmp_cfg,                  0, sizeof(RADIUS_tmp_cfg));
        RADIUS_clear_server_states();
        RADIUS_state.max_id_cnt_minus_one = RADIUS_MAX_ID_CNT - 1;

        // Cache a pointer to the configuration for faster access
        RADIUS_state.server_info[RADIUS_SERVER_TYPE_AUTH].config = RADIUS_cfg.servers_auth;
        RADIUS_state.server_info[RADIUS_SERVER_TYPE_ACCT].config = RADIUS_cfg.servers_acct;

        vtss_flag_init(&RADIUS_state.thread_control_flag);

        // Initialize sempahore. Initially taken.
        critd_init(&RADIUS_crit, "radius", VTSS_MODULE_ID_RADIUS, CRITD_TYPE_MUTEX);

        // Initialize the thread
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           RADIUS_thread,
                           0,
                           "RADIUS",
                           nullptr,
                           0,
                           &RADIUS_thread_handle,
                           &RADIUS_thread_state);
    }

    return VTSS_RC_OK;
}

