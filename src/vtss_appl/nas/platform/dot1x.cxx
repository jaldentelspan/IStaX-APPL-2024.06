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

// All Base-lib functions and base-lib callout functions are thread safe, as they are called
// with DOT1X_CRIT() taken, but Lint cannot see that in its final wrap-up (thread walk)
/*lint -esym(459, DOT1X_rx_bpdu) */

/****************************************************************************/
// Includes
/****************************************************************************/
#include <time.h>                  /* For time_t, time()           */
#include <misc_api.h>              /* For misc_time2str()          */
#include "vtss_os_wrapper.h"
#include "dot1x.h"                 /* For semi-public functions    */
#include "main.h"                  /* For init_cmd_t def           */
#include "critd_api.h"             /* For semaphore wrapper        */
#include "dot1x_api.h"             /* For public structs           */
#include "packet_api.h"            /* For rx and tx of frames      */
#include "conf_api.h"              /* For MAC address              */
#include "port_api.h"              /* For port_count_max(), etc.   */
#include "msg_api.h"               /* For Tx/Rx of msgs            */
#include "topo_api.h"              /* For topo_usid2isid()         */
#include "vtss_nas_platform_api.h" /* For core NAS lib             */
#include "vtss_radius_api.h"       /* For RADIUS services          */
#include "vtss_common_os.h"        /* For vtss_os_get_portmac()    */
#include "vlan_api.h"              /* For VLAN awareness setting   */
#include "l2proto_api.h"           /* For L2PORT2PORT()            */
#ifdef NAS_USES_PSEC
#include "psec_api.h"              /* For Port Security services   */
#endif
#ifdef VTSS_SW_OPTION_DOT1X_ACCT
#include "dot1x_acct.h"            /* For dot1x_acct_XXX()         */
#endif
#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"              /* For Aggr port config         */
#endif
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"            /* For S_W() macro              */
#endif
#ifdef VTSS_SW_OPTION_MSTP
#include "mstp_api.h"              /* For MSTP port config         */
#endif
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
#include "mgmt_api.h"              /* For mgmt_prio2txt()          */
#endif
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_CUSTOM
#include "nas_qos_custom_api.h"    /* For NAS_QOS_CUSTOM_xxx       */
#endif

#ifdef VTSS_SW_OPTION_ICFG
#include "dot1x_icli_functions.h" // For dot1x_icfg_init
#endif
#include "vtss/appl/nas.h" // For NAS return codes

/****************************************************************************/
// Various defines
/****************************************************************************/
#define DOT1X_FLASH_CFG_VERSION 2

/****************************************************************************/
//
/****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

enum {
    DOT1X_PORT_STATE_FLAGS_LINK_UP    = (1 << 0), /**< '0' = link is down, '1' = Link is up                                               */
    DOT1X_PORT_STATE_FLAGS_AUTHORIZED = (1 << 1), /**< '0' = port auth-state set to unauthorized, '1' = port auth-state set to authorized */
}; // To satisfy Lint, we make this enum anonymous and whereever it's used, we declare an u8. I would've liked to call this "dot1x_port_state_flags_t".

typedef struct {
    u8 flags; /**< A combination of the DOT1X_PORT_STATE_FLAGS_xxx flags */
} dot1x_port_state_t;

typedef struct {
    BOOL switch_exists;
    CapArray<dot1x_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_state;
} dot1x_switch_state_t;

typedef struct {
    dot1x_switch_state_t switch_state[VTSS_ISID_CNT];
} dot1x_stack_state_t;

dot1x_stack_state_t DOT1X_stack_state;

/****************************************************************************/
// Overall configuration (valid on primary switch only).
/****************************************************************************/
typedef struct {
    // One instance of the global configuration
    vtss_appl_glbl_cfg_t glbl_cfg;

    // One instance per switch in the stack of the switch configuration.
    // Indices are in the range [0; VTSS_ISID_CNT[, so all derefs must
    // subtract VTSS_ISID_START from @isid to index correctly.
    vtss_nas_switch_cfg_t switch_cfg[VTSS_ISID_CNT];
} dot1x_stack_cfg_t;

/****************************************************************************/
// Trace definitions
/****************************************************************************/
#include "dot1x_trace.h"
#include <vtss_trace_api.h>

/* Trace registration. Initialized by dot1x_init() */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "nas", "NAS module"
};

#ifndef DOT1X_DEFAULT_TRACE_LVL
#define DOT1X_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        DOT1X_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_BASE] = {
        "base",
        "Base-lib calls",
        DOT1X_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_ACCT] = {
        "acct",
        "Accounting Module Calls",
        DOT1X_DEFAULT_TRACE_LVL
    },
    [TRACE_GRP_ICLI] = {
        "icli",
        "iCLI",
        DOT1X_DEFAULT_TRACE_LVL
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/******************************************************************************/
// Semaphore stuff.
/******************************************************************************/
static critd_t DOT1X_crit;

// Macros for accessing mutex functions
// -----------------------------------------
#define DOT1X_CRIT_ENTER()         critd_enter(        &DOT1X_crit, __FILE__, __LINE__)
#define DOT1X_CRIT_EXIT()          critd_exit(         &DOT1X_crit, __FILE__, __LINE__)
#define DOT1X_CRIT_ASSERT_LOCKED() critd_assert_locked(&DOT1X_crit, __FILE__, __LINE__)

/******************************************************************************/
// Configuration and state
/******************************************************************************/
static dot1x_stack_cfg_t DOT1X_stack_cfg; // Configuration for whole stack (used when we're primary switch, only).

// If not debugging, set DOT1X_INLINE to inline
#define DOT1X_INLINE inline

/******************************************************************************/
// Thread variables
/******************************************************************************/
static vtss_handle_t DOT1X_thread_handle;
static vtss_thread_t DOT1X_thread_state;  // Contains space for the scheduler to hold the current thread state.

// Value to use for supplicant timeout
#define SUPPLICANT_TIMEOUT 10

#define DOT1X_MAC_STR_BUF_SIZE 70

/******************************************************************************/
// DOT1X_vid_mac_to_str()
/******************************************************************************/
static const char *DOT1X_vid_mac_to_str(mesa_port_no_t api_port, const mesa_vid_mac_t *const vid_mac, char buf[DOT1X_MAC_STR_BUF_SIZE])
{
    char mac_str[18];
    sprintf(buf, "<uport, VID, MAC> = <%u, %u, %s>", iport2uport(api_port), vid_mac->vid, misc_mac_txt(vid_mac->mac.addr, mac_str));
    return buf;
}

/******************************************************************************/
//
// Message handling functions, structures, and state.
//
/******************************************************************************/

/****************************************************************************/
// Message IDs
/****************************************************************************/
typedef enum {
    DOT1X_MSG_ID_PRI_TO_SEC_PORT_STATE = 1, // Tell secondary switch to set port state on a single port.
    DOT1X_MSG_ID_PRI_TO_SEC_EAPOL,          // Tell secondary switch to send an EAPOL frame (BPDU) on one of its ports
    DOT1X_MSG_ID_SEC_TO_PRI_EAPOL,          // Secondary switch received an EAPOL frame (BPDU) on one of its front ports and is now sending it to the primary switch.
} dot1x_msg_id_t;

/******************************************************************************/
// Current version of 802.1X messages (1-based).
// Future revisions of this module should support previous versions if applicable.
/******************************************************************************/
#define DOT1X_MSG_VERSION 3

/******************************************************************************/
// Pri->Sec. State of single port.
// Sent when the authorized state or the configuration of the port changes.
/******************************************************************************/
typedef struct {
    mesa_port_no_t api_port;
    BOOL           authorized;
} dot1x_msg_port_state_t;

/******************************************************************************/
// Pri->Sec. State of all switch ports.
// Sent when the configuration of a switch changes.
/******************************************************************************/
typedef struct {
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> authorized;
} dot1x_msg_switch_state_t;

/****************************************************************************/
// Pri->Sec: Tx this EAPOL on a front a port.
// Sec->Pri: This EAPOL was received on a front port.
// Whether it's for the primary switch or the secondary switch is given by DOT1X_MSG_ID_xxx_EAPOL.
/****************************************************************************/
typedef struct {
    mesa_port_no_t api_port;                                 // Front port to receive/transmit the frame onto.
    size_t         len;                                      // Length of frame
    mesa_vid_t     vid;                                      // For Sec->Pri: The VLAN ID that this frame was received on.
    u8             frm[NAS_IEEE8021X_MAX_EAPOL_PACKET_SIZE]; // Actual frame (this field must come last, since we normally won't fill out all of the frame before sending it as a message).
} dot1x_msg_eapol_t;

/****************************************************************************/
// Message Identification Header
/****************************************************************************/
typedef struct {
    // Message Version Number
    u32 version; // Set to DOT1X_MSG_VERSION

    // Message ID
    dot1x_msg_id_t msg_id;
} dot1x_msg_hdr_t;

/****************************************************************************/
// Message.
// This struct contains a union, whose primary purpose is to give the
// size of the biggest of the contained structures.
/****************************************************************************/
typedef struct {
    // Header stuff
    dot1x_msg_hdr_t hdr;

    // Request message
    union {
        dot1x_msg_port_state_t   port_state;
        dot1x_msg_eapol_t        eapol;
    } u;
} dot1x_msg_t;

/****************************************************************************/
// Message buffer pool. Statically allocated and protected by a semaphore.
/****************************************************************************/
typedef struct {
    // Buffers and semaphores
    vtss_sem_t  sem;
    dot1x_msg_t msg;
} dot1x_msg_buf_pool_t;

/****************************************************************************/
// Generic message structure, that can be used for both request and reply.
/****************************************************************************/
typedef struct {
    vtss_sem_t *sem;
    void       *msg;
} dot1x_msg_buf_t;

// Static, semaphore-protected message transmission buffer(s).
static dot1x_msg_buf_pool_t DOT1X_msg_buf_pool;

/******************************************************************************/
// DOT1X_msg_buf_alloc()
// Blocks until a buffer is available, then takes and returns it.
/******************************************************************************/
static dot1x_msg_t *DOT1X_msg_buf_alloc(dot1x_msg_buf_t *buf, dot1x_msg_id_t msg_id)
{
    dot1x_msg_t *msg = &DOT1X_msg_buf_pool.msg;
    buf->sem = &DOT1X_msg_buf_pool.sem;
    buf->msg = msg;
    vtss_sem_wait(buf->sem);
    msg->hdr.version = DOT1X_MSG_VERSION;
    msg->hdr.msg_id = msg_id;
    return msg;
}

/******************************************************************************/
// DOT1X_msg_id_to_str()
/******************************************************************************/
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_NOISE)
static const char *DOT1X_msg_id_to_str(dot1x_msg_id_t msg_id)
{
    switch (msg_id) {
    case DOT1X_MSG_ID_PRI_TO_SEC_PORT_STATE:
        return "PRI_TO_SEC_PORT_STATE";

    case DOT1X_MSG_ID_PRI_TO_SEC_EAPOL:
        return "PRI_TO_SEC_EAPOL";

    case DOT1X_MSG_ID_SEC_TO_PRI_EAPOL:
        return "SEC_TO_PRI_EAPOL";

    default:
        return "***Unknown Message ID***";
    }
}
#endif /* VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_NOISE */

/******************************************************************************/
// DOT1X_msg_tx_done()
// Called when message successfully or unsuccessfully transmitted.
/******************************************************************************/
static void DOT1X_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    // The context contains a pointer to the semaphore
    // protecting the transmitted buffer. Release it.
    vtss_sem_post((vtss_sem_t *)contxt);
}

/******************************************************************************/
// DOT1X_msg_tx()
// Do transmit a message.
/******************************************************************************/
static void DOT1X_msg_tx(dot1x_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
    msg_tx_adv(buf->sem, DOT1X_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_DOT1X, isid, buf->msg, len + MSG_TX_DATA_HDR_LEN(dot1x_msg_t, u));
}

/******************************************************************************/
// DOT1X_msg_tx_port_state()
/******************************************************************************/
static void DOT1X_msg_tx_port_state(vtss_isid_t isid, mesa_port_no_t api_port, BOOL authorized)
{
    dot1x_msg_buf_t buf;
    dot1x_msg_t     *msg;

    // Update the flags
    if (authorized) {
        DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[api_port - VTSS_PORT_NO_START].flags |=  DOT1X_PORT_STATE_FLAGS_AUTHORIZED;
    } else {
        DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[api_port - VTSS_PORT_NO_START].flags &= ~DOT1X_PORT_STATE_FLAGS_AUTHORIZED;
    }

    if (!msg_switch_exists(isid)) {
        return;
    }

    // Get a buffer.
    msg = DOT1X_msg_buf_alloc(&buf, DOT1X_MSG_ID_PRI_TO_SEC_PORT_STATE);

    // Copy state to buffer
    msg->u.port_state.api_port   = api_port;
    msg->u.port_state.authorized = authorized;

    T_D("%d:%d: Tx state (%s)", isid, iport2uport(api_port), authorized ? "Auth" : "Unauth");

    // Transmit it.
    DOT1X_msg_tx(&buf, isid, sizeof(msg->u.port_state));
}

/******************************************************************************/
// DOT1X_msg_tx_switch_state()
/******************************************************************************/
static void DOT1X_msg_tx_switch_state(vtss_isid_t isid, BOOL *authorized)
{
    mesa_rc     rc;
    port_iter_t pit;

    if (!msg_switch_exists(isid)) {
        return;
    }

    // Set all port states.
    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        T_D("VTSS_ISID_LOCAL:%d: Changing port state to %s", pit.uport, authorized[pit.iport] ? "Auth" : "Unauth");
        if ((rc = mesa_auth_port_state_set(NULL, pit.iport, authorized[pit.iport] ? MESA_AUTH_STATE_BOTH : MESA_AUTH_STATE_NONE)) != VTSS_RC_OK) {
            T_E("VTSS_ISID_LOCAL:%d: API err: %s", pit.uport, error_txt(rc));
        }
    }
}

/******************************************************************************/
// DOT1X_msg_tx_eapol()
/******************************************************************************/
static void DOT1X_msg_tx_eapol(vtss_isid_t isid, mesa_port_no_t api_port, dot1x_msg_id_t msg_id, mesa_vid_t vid, const u8 *const frm, size_t len)
{
    dot1x_msg_buf_t buf;
    dot1x_msg_t     *msg;

    if (len > sizeof(msg->u.eapol.frm)) {
        T_E("%d:%d: Attempting to send a frame whose length is larger than we can handle (%zu > %zu)", isid, iport2uport(api_port), len, sizeof(msg->u.eapol.frm));
        return;
    }

    // Get a buffer
    msg = DOT1X_msg_buf_alloc(&buf, msg_id);

    // Copy frame to message buffer
    msg->u.eapol.api_port = api_port;
    msg->u.eapol.len      = len;
    msg->u.eapol.vid      = vid;
    memcpy(&msg->u.eapol.frm[0], frm, len);

    T_N("%d:%d: Sending BPDU to %s switch, len=%zu, vid=%d", isid, iport2uport(api_port), msg_id == DOT1X_MSG_ID_SEC_TO_PRI_EAPOL ? "primary" : "secondary", len, vid);

    // Send message
    DOT1X_msg_tx(&buf, isid, sizeof(dot1x_msg_eapol_t) - (sizeof(msg->u.eapol.frm) - len));
}

/******************************************************************************/
// DOT1X_tx_switch_state()
/******************************************************************************/
static void DOT1X_tx_switch_state(vtss_isid_t isid, BOOL glbl_enabled)
{
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> authorized;
    port_iter_t pit;

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        vtss_appl_nas_port_control_t admin_state = DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[pit.iport].admin_state;
        authorized[pit.iport] = !glbl_enabled || NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(admin_state) || admin_state == VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED;
        if (authorized[pit.iport]) {
            DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[pit.iport].flags |= DOT1X_PORT_STATE_FLAGS_AUTHORIZED;
        } else {
            DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[pit.iport].flags &= ~DOT1X_PORT_STATE_FLAGS_AUTHORIZED;
        }
    }

    DOT1X_msg_tx_switch_state(isid, authorized.data());
}

/******************************************************************************/
// DOT1X_tx_port_state()
/******************************************************************************/
static void DOT1X_tx_port_state(vtss_isid_t isid, mesa_port_no_t api_port, BOOL glbl_enabled, vtss_appl_nas_port_control_t admin_state)
{
    // Check to see if we must transmit the new port control
    dot1x_port_state_t *port_state = &DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[api_port - VTSS_PORT_NO_START];
    BOOL               cur_authorized = (port_state->flags & DOT1X_PORT_STATE_FLAGS_AUTHORIZED) ? TRUE : FALSE;
    BOOL               new_authorized = !glbl_enabled || NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(admin_state) || admin_state == VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED;

    if (new_authorized != cur_authorized) {
        DOT1X_msg_tx_port_state(isid, api_port, new_authorized);
    }
}

/******************************************************************************/
// DOT1X_cfg_valid_glbl()
/******************************************************************************/
static mesa_rc DOT1X_cfg_valid_glbl(vtss_appl_glbl_cfg_t *glbl_cfg)
{
    if (glbl_cfg->reauth_period_secs < VTSS_APPL_NAS_REAUTH_PERIOD_SECS_MIN || glbl_cfg->reauth_period_secs > VTSS_APPL_NAS_REAUTH_PERIOD_SECS_MAX) {
        return VTSS_APPL_NAS_ERROR_INVALID_REAUTH_PERIOD;
    }

    // In case someone changes DOT1X_EAPOL_TIMEOUT_SECS_MAX to something smaller than what eapol_timeout_secs
    // can cope with, we need to do the test for glbl_cfg->eapol_timeout_secs > DOT1X_EAPOL_TIMEOUT_SECS_MAX,
    // so tell Lint not to report "Warning -- Relational operator '>' always evaluates to 'false')
    /*lint -e{685} */
    if (glbl_cfg->eapol_timeout_secs < VTSS_APPL_NAS_EAPOL_TIMEOUT_SECS_MIN || glbl_cfg->eapol_timeout_secs > VTSS_APPL_NAS_EAPOL_TIMEOUT_SECS_MAX) {
        return VTSS_APPL_NAS_ERROR_INVALID_EAPOL_TIMEOUT;
    }

#ifdef NAS_USES_PSEC
    if (glbl_cfg->psec_aging_period_secs < VTSS_APPL_NAS_PSEC_AGING_PERIOD_SECS_MIN || glbl_cfg->psec_aging_period_secs > VTSS_APPL_NAS_PSEC_AGING_PERIOD_SECS_MAX) {
        return VTSS_APPL_NAS_ERROR_INVALID_AGING_PERIOD;
    }

    if (glbl_cfg->psec_hold_time_secs < VTSS_APPL_NAS_PSEC_HOLD_TIME_SECS_MIN || glbl_cfg->psec_hold_time_secs > VTSS_APPL_NAS_PSEC_HOLD_TIME_SECS_MAX) {
        return VTSS_APPL_NAS_ERROR_INVALID_HOLD_TIME;
    }
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    if (glbl_cfg->guest_vid < 1 || glbl_cfg->guest_vid >= VTSS_VIDS) {
        return VTSS_APPL_NAS_ERROR_INVALID_GUEST_VLAN_ID;
    }

    if (glbl_cfg->reauth_max < VTSS_APPL_NAS_REAUTH_MIN || glbl_cfg->reauth_max > VTSS_APPL_NAS_REAUTH_MAX) {
        return VTSS_APPL_NAS_ERROR_INVALID_REAUTH_MAX;
    }
#endif

    return VTSS_RC_OK;
}

/******************************************************************************/
// DOT1X_cfg_valid_port()
/******************************************************************************/
static DOT1X_INLINE mesa_rc DOT1X_cfg_valid_port(vtss_appl_nas_port_cfg_t *port_cfg)
{
    if (
#ifdef VTSS_SW_OPTION_NAS_DOT1X_SINGLE
        port_cfg->admin_state != VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE     &&
#endif
#ifdef VTSS_SW_OPTION_NAS_DOT1X_MULTI
        port_cfg->admin_state != VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI      &&
#endif
#ifdef VTSS_SW_OPTION_NAS_MAC_BASED
        port_cfg->admin_state != VTSS_APPL_NAS_PORT_CONTROL_MAC_BASED        &&
#endif
        port_cfg->admin_state != VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED &&
        port_cfg->admin_state != VTSS_APPL_NAS_PORT_CONTROL_AUTO             &&
        port_cfg->admin_state != VTSS_APPL_NAS_PORT_CONTROL_FORCE_UNAUTHORIZED) {
        T_W("Invalid administrative state");
        return VTSS_APPL_NAS_ERROR_INVALID_ADMIN_STATE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// DOT1X_cfg_valid_switch()
/******************************************************************************/
static mesa_rc DOT1X_cfg_valid_switch(vtss_nas_switch_cfg_t *switch_cfg)
{
    mesa_port_no_t api_port;
    mesa_rc        rc;

    for (api_port = 0; api_port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); api_port++) {
        if ((rc = DOT1X_cfg_valid_port(&switch_cfg->port_cfg[api_port])) != VTSS_RC_OK) {
            return rc;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// DOT1X_cfg_default_switch()
// Initialize per-switch settings to defaults.
/******************************************************************************/
void DOT1X_cfg_default_switch(vtss_nas_switch_cfg_t *cfg)
{
    mesa_port_no_t api_port;

    vtss_clear(*cfg);
    for (api_port = 0; api_port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); api_port++) {
        vtss_appl_nas_port_cfg_t *port_cfg = &cfg->port_cfg[api_port];

        port_cfg->admin_state = VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED;
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
        port_cfg->qos_backend_assignment_enabled = FALSE;
#endif
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
        port_cfg->vlan_backend_assignment_enabled = FALSE;
#endif
#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
        port_cfg->guest_vlan_enabled = FALSE;
#endif
    }
}

/******************************************************************************/
// DOT1X_cfg_default_glbl()
// Initialize global settings to defaults.
/******************************************************************************/
void DOT1X_cfg_default_glbl(vtss_appl_glbl_cfg_t *cfg)
{
    // First reset whole structure...
    memset(cfg, 0, sizeof(vtss_appl_glbl_cfg_t));

    // ...then override specific fields
    cfg->reauth_period_secs = DOT1X_REAUTH_PERIOD_SECS_DEFAULT;
    cfg->eapol_timeout_secs = DOT1X_EAPOL_TIMEOUT_SECS_DEFAULT;


#ifdef NAS_USES_PSEC
    cfg->psec_aging_enabled     = NAS_PSEC_AGING_ENABLED_DEFAULT;
    cfg->psec_aging_period_secs = NAS_PSEC_AGING_PERIOD_SECS_DEFAULT;
    cfg->psec_hold_enabled      = NAS_PSEC_HOLD_ENABLED_DEFAULT;
    cfg->psec_hold_time_secs    = NAS_PSEC_HOLD_TIME_SECS_DEFAULT;
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
    cfg->qos_backend_assignment_enabled = FALSE;
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
    cfg->vlan_backend_assignment_enabled = FALSE;
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    cfg->guest_vlan_enabled      = FALSE;
    cfg->guest_vid               = 1;
    cfg->reauth_max              = 2;
    cfg->guest_vlan_allow_eapols = FALSE;
#endif
}

/******************************************************************************/
// DOT1X_cfg_default_all()
// Initialize global and per-switch settings
/******************************************************************************/
static void DOT1X_cfg_default_all(dot1x_stack_cfg_t *cfg)
{
    vtss_isid_t zisid;

    DOT1X_cfg_default_glbl(&cfg->glbl_cfg);
    // Create defaults per switch.
    for (zisid = 0; zisid < VTSS_ISID_CNT; zisid++) {
        DOT1X_cfg_default_switch(&cfg->switch_cfg[zisid]);
    }
}

/******************************************************************************/
// DOT1X_vlan_force_unaware_get()
// Return TRUE if we should force the port in VLAN unaware mode.
// FALSE otherwise.
/******************************************************************************/
static DOT1X_INLINE BOOL DOT1X_vlan_force_unaware_get(vtss_appl_nas_port_control_t admin_state)
{
    // All BPDU-based modes require VLAN unawareness.
    // In principle, port-based 802.1X wouldn't necessarily require it unless RADIUS-assigned VLANs were enabled,
    // but for simplicity, we set it to VLAN unaware.
    return NAS_PORT_CONTROL_IS_BPDU_BASED(admin_state);
}

/******************************************************************************/
// DOT1X_vlan_awareness_set()
/******************************************************************************/
static DOT1X_INLINE void DOT1X_vlan_awareness_set(nas_port_t nas_port, vtss_appl_nas_port_control_t admin_state)
{
    vtss_isid_t                isid     = DOT1X_NAS_PORT_2_ISID(nas_port);
    mesa_port_no_t             api_port = DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port);
    vtss_uport_no_t            uport    = iport2uport(api_port);
    mesa_rc                    rc;
    vtss_appl_vlan_port_conf_t vlan_cfg;

    memset(&vlan_cfg, 0, sizeof(vlan_cfg));

    if (DOT1X_vlan_force_unaware_get(admin_state)) {
        T_D("%d:%d: Forcing VLAN unaware", isid, uport);
        vlan_cfg.hybrid.flags |= VTSS_APPL_VLAN_PORT_FLAGS_AWARE;
    } else {
        T_D("%d:%d: Unsetting VLAN unawareness", isid, uport);
    }

    if (msg_switch_configurable(isid)) {
        if ((rc = vlan_mgmt_port_conf_set(isid, api_port, &vlan_cfg, VTSS_APPL_VLAN_USER_DOT1X)) != VTSS_RC_OK) {
            T_E("%u:%d: Unable to change VLAN awareness (%s)", isid, uport, error_txt(rc));
        }
    }
}

#ifdef NAS_USES_VLAN
/******************************************************************************/
// DOT1X_vlan_membership_set()
/******************************************************************************/
static void DOT1X_vlan_membership_set(vtss_isid_t isid, mesa_port_no_t api_port, mesa_vid_t vid, BOOL add_membership)
{
    vtss_uport_no_t        uport = iport2uport(api_port);
    vtss_appl_vlan_entry_t entry;

    T_I("%d:%d vid = %d, add_membership = %d", isid, iport2uport(api_port), vid, add_membership);

    if (vid != 0) {
        // Gotta set or clear this port's membership for new_vid.
        // Get the current port mask for this VID.
        if (vtss_appl_vlan_get(isid, vid, &entry, FALSE, VTSS_APPL_VLAN_USER_DOT1X) != VTSS_RC_OK) {
            if (add_membership == FALSE) {
                // Only print a warning if we expect the VID to be there (i.e. when we're trying
                // to remove our membership).
                T_W("%u:%d: Unable to get current membership ports of vid = %d", isid, uport, vid);
            }

            vtss_clear(entry);
            entry.vid = vid;
        } else {
            if (entry.vid != vid) {
                T_W("%u:%d: The port membership entry's VID (%d) is not what we requested (%d)", isid, uport, entry.vid, vid);
                entry.vid = vid;
            }

            // If going to add membership, we expect that it previously was cleared (and hence have to compare with TRUE)
            if (entry.ports[api_port] == add_membership) {
                T_W("%u:%d: Expected port member of vid = %d to be %s, but it isn't", isid, uport, vid, add_membership ? "cleared" : "set");
            }
        }

        entry.ports[api_port] = add_membership;

        {
            u64 val = 0;
            int i;
            u32 port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

            for (i = 0; i < port_cnt; i++) {
                val |= ((u64)entry.ports[i] << i);
            }

            T_I("%d: Invoking vlan_mgmt_vlan_add() with VID = %d and ports = 0x" VPRI64x, isid, vid, val);
        }

        if (vlan_mgmt_vlan_add(isid, &entry, VTSS_APPL_VLAN_USER_DOT1X) != VTSS_RC_OK) {
            T_W("%u:%d: Couldn't %s port bit in membership entry (vid = %d)", isid, uport, add_membership ? "set" : "clear", vid);
        }
    }
}
#endif

#ifdef NAS_USES_VLAN
/******************************************************************************/
// DOT1X_vlan_port_set()
/******************************************************************************/
static DOT1X_INLINE void DOT1X_vlan_port_set(nas_port_info_t *port_info, vtss_isid_t isid, mesa_port_no_t api_port, mesa_vid_t new_vid)
{
    vtss_appl_vlan_port_conf_t vlan_cfg;
    mesa_rc                    rc;

    memset(&vlan_cfg, 0, sizeof(vlan_cfg));

    if (new_vid != 0) {
        vlan_cfg.hybrid.pvid   = new_vid;
        vlan_cfg.hybrid.flags |= VTSS_APPL_VLAN_PORT_FLAGS_PVID;
    }

    if (DOT1X_vlan_force_unaware_get(port_info->port_control)) {
        vlan_cfg.hybrid.flags |= VTSS_APPL_VLAN_PORT_FLAGS_AWARE;
    }

    T_I("%d:%d Setting new_vid = %d. Flags = 0x%02x", isid, iport2uport(api_port), new_vid, vlan_cfg.hybrid.flags);

    if ((rc = vlan_mgmt_port_conf_set(isid, api_port, &vlan_cfg, VTSS_APPL_VLAN_USER_DOT1X)) != VTSS_RC_OK) {
        T_E("%u:%d: Unable to set PVID volatile to %d (%s)", isid, iport2uport(api_port), new_vid, error_txt(rc));
    }
}
#endif /* NAS_USES_VLAN */

#ifdef NAS_USES_VLAN
/******************************************************************************/
// DOT1X_vlan_set()
/******************************************************************************/
static void DOT1X_vlan_set(nas_port_t nas_port, mesa_vid_t new_vid, vtss_appl_nas_vlan_type_t vlan_type)
{
    nas_port_info_t *port_info = nas_get_port_info(nas_get_top_sm(nas_port));

    DOT1X_CRIT_ASSERT_LOCKED();

    if (new_vid != port_info->current_vid) {
        vtss_isid_t    isid     = DOT1X_NAS_PORT_2_ISID(nas_port);
        mesa_port_no_t api_port = DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port);

        T_I("%d:%d: Changing PVID from %d to %d", isid, iport2uport(api_port), port_info->current_vid, new_vid);

        DOT1X_vlan_port_set(port_info, isid, api_port, new_vid);
        // First clear the old membership
        DOT1X_vlan_membership_set(isid, api_port, port_info->current_vid, FALSE);
        // Then add the new.
        DOT1X_vlan_membership_set(isid, api_port, new_vid,                TRUE);
        port_info->current_vid = new_vid;
    }

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
    // This can only survive one single call to this function.
    port_info->backend_assigned_vid = 0;
#endif

    // Change the type
    port_info->vlan_type = vlan_type;
}
#endif

#ifdef NAS_USES_PSEC
/******************************************************************************/
// DOT1X_psec_use_chg()
/******************************************************************************/
static void DOT1X_psec_use_chg(vtss_isid_t isid, mesa_port_no_t api_port, nas_stop_reason_t stop_reason, BOOL enable, psec_port_mode_t port_mode)
{
    mesa_rc    rc;
    nas_port_t nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);

    if ((rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, enable, port_mode)) != VTSS_RC_OK) {
        if (rc == VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH) {
            // In some scenarios, where a switch is going from being a primary switch to being a secondary switch, it's OK to get a VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH error code.
            T_D("%d:%d: psec fail: %s", isid, iport2uport(api_port), error_txt(rc));
        } else {
            T_E("%d:%d: psec fail: %s", isid, iport2uport(api_port), error_txt(rc));
        }
    }

    // Since we've now deleted all entries in PSEC, we should also free all SMs.
    nas_free_all_sms(nas_port, stop_reason);
}
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
/******************************************************************************/
// DOT1X_guest_vlan_enter()
/******************************************************************************/
static DOT1X_INLINE void DOT1X_guest_vlan_enter(vtss_isid_t isid, mesa_port_no_t api_port, nas_port_info_t *port_info)
{
    T_D("%d:%d: Entering Guest VLAN", isid, iport2uport(api_port));

    // If MAC-table-based, exit PSEC enable. Here, we should ask the PSEC module for
    // keeping the port open even though we're not a member, so that we can get
    // BPDUs.
    if (NAS_PORT_CONTROL_IS_MAC_TABLE_AND_BPDU_BASED(port_info->port_control)) {
        // This will delete all SMs currently on the port and cause the base lib
        // to auto-allocate a request identity SM, which is the one we will
        // massage further down in this function.
        T_D("%d:%d: Changing PSEC enable to FALSE - start", isid, iport2uport(api_port));
        DOT1X_psec_use_chg(isid, api_port, NAS_STOP_REASON_GUEST_VLAN_ENTER, FALSE, PSEC_PORT_MODE_KEEP_BLOCKED);
        T_D("%d:%d: Changing PSEC enable to FALSE - done", isid, iport2uport(api_port));
    }

    // Put it into Guest VLAN
    DOT1X_vlan_set(port_info->port_no, DOT1X_stack_cfg.glbl_cfg.guest_vid, VTSS_APPL_NAS_VLAN_TYPE_GUEST_VLAN);

    // Gotta set the port in authorized state (only needed for port-based), and only
    // needed because the nas_set_fake_force_authorized() doesn't call-back the
    // nas_os_set_authorized(), because that would start accounting.
    if (port_info->port_control == VTSS_APPL_NAS_PORT_CONTROL_AUTO) {
        DOT1X_msg_tx_port_state(isid, api_port, TRUE);
    }

    // Put the SM into fake force authorized, so that it doesn't reauthenticate, re-initialize, or react on anything.
    nas_set_fake_force_authorized(port_info->top_sm, TRUE);
}
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
/******************************************************************************/
// DOT1X_guest_vlan_exit()
/******************************************************************************/
static void DOT1X_guest_vlan_exit(vtss_isid_t isid, mesa_port_no_t api_port, nas_port_info_t *port_info)
{
    if (port_info->vlan_type != VTSS_APPL_NAS_VLAN_TYPE_GUEST_VLAN) {
        return;
    }

    T_D("%d:%d: Exiting Guest VLAN", isid, iport2uport(api_port));

    // Take it out of the Guest VLAN mode.
    DOT1X_vlan_set(port_info->port_no, 0, VTSS_APPL_NAS_VLAN_TYPE_NONE);

    // If MAC-table-based, re-enter PSEC enable. All MAC-addresses that were learned by
    // other PSEC users will get deleted.
    if (NAS_PORT_CONTROL_IS_MAC_TABLE_AND_BPDU_BASED(port_info->port_control)) {
        DOT1X_psec_use_chg(isid, api_port, NAS_STOP_REASON_GUEST_VLAN_EXIT, TRUE, PSEC_PORT_MODE_KEEP_BLOCKED);
    }

    // Re-initialize state machine. This will cause the SM to call-back
    // nas_os_set_authorized(), which in turn will cause the new port state
    // to be set to unauthorized for port-based 802.1X.
    nas_set_fake_force_authorized(port_info->top_sm, FALSE);
}
#endif

/******************************************************************************/
// DOT1X_set_port_control()
// Free existing SMs and set the port control
/******************************************************************************/
static void DOT1X_set_port_control(nas_port_t nas_port, vtss_appl_nas_port_control_t new_admin_state, nas_stop_reason_t stop_reason)
{
#if defined(VTSS_SW_OPTION_PSEC) || defined(VTSS_SW_OPTION_NAS_GUEST_VLAN)
    vtss_isid_t    isid     = DOT1X_NAS_PORT_2_ISID(nas_port);
    mesa_port_no_t api_port = DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port);
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    nas_port_info_t *port_info = nas_get_port_info(nas_get_top_sm(nas_port));
    DOT1X_guest_vlan_exit(isid, api_port, port_info);
    if (stop_reason == NAS_STOP_REASON_PORT_MODE_CHANGED ||
        stop_reason == NAS_STOP_REASON_PORT_LINK_DOWN    ||
        stop_reason == NAS_STOP_REASON_PORT_SHUT_DOWN    ||
        stop_reason == NAS_STOP_REASON_PORT_STP_MSTI_DISCARDING) {
        port_info->eapol_frame_seen = FALSE;
    }
#endif

#ifdef NAS_USES_PSEC
    {
        // Update the Port Security module with our new state.
        BOOL new_is_mac_table_based  = NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(new_admin_state);
        BOOL new_is_bpdu_based       = NAS_PORT_CONTROL_IS_MAC_TABLE_AND_BPDU_BASED(new_admin_state);

        // If either the old or the new mode was MAC table based, we need to inform the Port Security module to either
        // enable our use and delete all current entries or to disable our use.
        // Any attached SMs will be deleted afterwards.
        // The @ctx parameter holds the reason for deleting the SM.
        // Also, if we're in an 802.1X mode (single or multi, that is) and not a MAC-based mode, we must
        // ask the PSEC module to not do CPU copying initially, since we have to gain control of this flag
        // and add the MAC addresses ourselves based on BPDUs.
        DOT1X_psec_use_chg(isid, api_port, stop_reason, new_is_mac_table_based, new_is_bpdu_based ? PSEC_PORT_MODE_KEEP_BLOCKED : PSEC_PORT_MODE_NORMAL);
    }
#endif

    nas_free_all_sms(nas_port, stop_reason);
    nas_set_port_control(nas_port, new_admin_state);
    DOT1X_vlan_awareness_set(nas_port, new_admin_state);

    if (new_admin_state == VTSS_APPL_NAS_PORT_CONTROL_DISABLED) {
        // Enforce authorizing the port.
        // The reason for this is that it can happen that the port
        // would otherwise remain unauthorized.
        // Suppose a port is in Guest VLAN (and therefore Authorized) and the
        // end-user invokes "reload defaults". Then the following series of
        // events would occur:
        // 1) This function would be called and in the lines above, the port
        //    would be taken out of the Guest VLAN. This by itself
        //    would cause the port to become unauthorized.
        // 2) Later on in this function, nas_set_port_control() would be
        //    called, causing all SMs on the port to be removed.
        //    Unfortunately, no-one re-opens the port (nas_set_port_control()
        //    doesn't do it mainly because there are no state-machines left
        //    to do it, but also because the handle it got to do it is
        //    nas_os_set_authorized(), which doesn't allow to be called
        //    when globally disabled - and it would start accounting.
        DOT1X_msg_tx_port_state(isid, api_port, TRUE);
    }
}

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
/******************************************************************************/
// DOT1X_qos_set()
/******************************************************************************/
static void DOT1X_qos_set(nas_port_t nas_port, mesa_prio_t qos_class)
{
    nas_port_info_t *port_info = nas_get_port_info(nas_get_top_sm(nas_port));

    DOT1X_CRIT_ASSERT_LOCKED();

    if (port_info->qos_class != qos_class) {
        vtss_isid_t     isid     = DOT1X_NAS_PORT_2_ISID(nas_port);
        mesa_port_no_t  api_port = DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port);
        vtss_uport_no_t uport    = iport2uport(api_port);
        if (qos_class == VTSS_PRIO_NO_NONE) {
            T_D("%d:%d: Unsetting QoS class", isid, uport);
        } else {
            T_D("%d:%d: Setting QoS class to %u", isid, uport, qos_class);
        }
        if (qos_port_volatile_default_prio_set(isid, api_port, qos_class) != VTSS_RC_OK) {
            T_W("%d:%d: Qos setting to %u failed", isid, uport, qos_class);
        }
        port_info->qos_class = qos_class;
    }
}
#endif

#ifdef NAS_USES_PSEC
/****************************************************************************/
// DOT1X_psec_chg()
/****************************************************************************/
static void DOT1X_psec_chg(nas_port_t nas_port, vtss_isid_t isid, mesa_port_no_t api_port, nas_port_info_t *port_info, nas_eap_info_t *eap_info, vtss_nas_client_info_t *client_info, psec_add_method_t new_method)
{
    vtss_uport_no_t uport = iport2uport(api_port);
    mesa_rc         rc;

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
    mesa_vid_t      next_pvid, next_mac_vid;
    vtss_appl_nas_vlan_type_t vlan_type;

    if (new_method == PSEC_ADD_METHOD_FORWARD) {
        next_pvid = port_info->backend_assigned_vid;
    } else {
        next_pvid = 0;
    }

    vlan_type = next_pvid ? VTSS_APPL_NAS_VLAN_TYPE_BACKEND_ASSIGNED : VTSS_APPL_NAS_VLAN_TYPE_NONE;

    // Sanity checks
    // The MAC's VID must be the current overridden - if overridden.
    if (port_info->current_vid != 0 && port_info->current_vid != client_info->vid_mac.vid) {
        T_E("%d:%d(%s): Invalid state. Current VID = %d, MAC's VID = %d", isid, uport, client_info->mac_addr_str, port_info->current_vid, client_info->vid_mac.vid);
    }

    // If overridden, then the revert VID must be non-zero.
    if (port_info->current_vid != 0 && eap_info->revert_vid == 0) {
        T_E("%d:%d(%s): Invalid revert VID (0). Current VID = %d.", isid, uport, client_info->mac_addr_str, port_info->current_vid);
    }

    if (port_info->current_vid == next_pvid) {
        // Going from non-overridden to non-overridden or
        // from overriden to the same overridden. Either way,
        // just change the MAC address' forward state.
        next_mac_vid = client_info->vid_mac.vid;
    } else if (port_info->current_vid == 0) {
        // Changing from not overridden to overridden.
        // Save the current MAC's VID in revert VID.
        eap_info->revert_vid = client_info->vid_mac.vid;
        next_mac_vid = next_pvid;
    } else if (next_pvid == 0) {
        // Changing from overridden to not overridden.
        // Go back to the revert VID.
        next_mac_vid = eap_info->revert_vid;
    } else {
        // Changing from one overridden to another overridden
        next_mac_vid = next_pvid;
    }

    if (client_info->vid_mac.vid != next_mac_vid) {
        // Change VID by first deleting the current entry and then adding a new.
        T_D("%d:%d(%s): VID change from %d to %d. New method: %s", isid, uport, client_info->mac_addr_str, client_info->vid_mac.vid, next_mac_vid, psec_add_method_to_str(new_method));

        // Gotta delete before adding for the sake of PSEC Limit, or we may saturate the port, causing a trap.
        if ((rc = psec_mgmt_mac_del(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &client_info->vid_mac)) != VTSS_RC_OK) {
            T_E("%d:%d: Couldn't delete %s:%d from MAC table. Error=%s", isid, uport, client_info->mac_addr_str, client_info->vid_mac.vid, error_txt(rc));
        }

        client_info->vid_mac.vid = next_mac_vid;
        if ((rc = psec_mgmt_mac_add(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &client_info->vid_mac, new_method)) != VTSS_RC_OK) {
            T_E("%d:%d: Couldn't add %s:%d to MAC table. Error=%s", isid, uport, client_info->mac_addr_str, client_info->vid_mac.vid, error_txt(rc));
            // Ask base lib to delete this SM upon the next timer tick (we're in the middle of a nas_os_XXX() call, so we can't
            // just free it.
            eap_info->delete_me = TRUE;
        }
    } else
#endif
    {
        // The new MAC VID is the same, so just change the current entry.
        T_I("%d:%d(%s:%d): Change method to %s", isid, uport, client_info->mac_addr_str, client_info->vid_mac.vid, psec_add_method_to_str(new_method));

        if ((rc = psec_mgmt_mac_chg(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &client_info->vid_mac, new_method)) != VTSS_RC_OK) {
            T_E("%d:%d(%s): Error from PSEC module: %s", isid, uport, client_info->mac_addr_str, error_txt(rc));
        }
    }

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
    // Always set the port VID, since that call also clears the backend_assigned_vid, which is necessary
    // in order to go back to non-overridden if e.g. the network adminstrator disables VLAN assignment.
    DOT1X_vlan_set(nas_port, next_pvid, vlan_type);
#endif
}
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
/******************************************************************************/
// DOT1X_guest_vlan_enter_check()
/******************************************************************************/
static DOT1X_INLINE BOOL DOT1X_guest_vlan_enter_check(vtss_isid_t isid, mesa_port_no_t api_port, nas_port_info_t *port_info)
{
    // If either we're not in Port-based, Single- or Multi-802.1X mode, or
    // guest VLAN is not globally enabled, or
    // guest VLAN is not enabled on this port, or
    // we're not allowed to enter Guest VLAN if at least one EAPOL frame is seen, and one EAPOL frame *is* seen,
    // then there's nothing to do.
    if (NAS_PORT_CONTROL_IS_BPDU_BASED(port_info->port_control) == FALSE                                                       ||
        DOT1X_stack_cfg.glbl_cfg.guest_vlan_enabled             == FALSE                                                       ||
        DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port - VTSS_PORT_NO_START].guest_vlan_enabled == FALSE ||
        (DOT1X_stack_cfg.glbl_cfg.guest_vlan_allow_eapols == FALSE && port_info->eapol_frame_seen)) {
        return FALSE;
    }

    DOT1X_guest_vlan_enter(isid, api_port, port_info);
    return TRUE;
}
#endif

/******************************************************************************/
// DOT1X_apply_cfg()
/******************************************************************************/
static mesa_rc DOT1X_apply_cfg(const vtss_isid_t isid, dot1x_stack_cfg_t *new_cfg, BOOL switch_cfg_may_be_changed, nas_stop_reason_t stop_reason)
{
    vtss_isid_t       zisid_start, zisid_end, zisid;
    mesa_port_no_t    api_port;
    dot1x_stack_cfg_t *old_cfg = &DOT1X_stack_cfg;
    BOOL              old_enabled = old_cfg->glbl_cfg.enabled;
    BOOL              new_enabled = new_cfg->glbl_cfg.enabled;
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
    BOOL              old_qos_glbl_enabled = old_cfg->glbl_cfg.qos_backend_assignment_enabled;
    BOOL              new_qos_glbl_enabled = new_cfg->glbl_cfg.qos_backend_assignment_enabled;
#endif
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
    BOOL              old_backend_vlan_glbl_enabled = old_cfg->glbl_cfg.vlan_backend_assignment_enabled;
    BOOL              new_backend_vlan_glbl_enabled = new_cfg->glbl_cfg.vlan_backend_assignment_enabled;
#endif
#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    BOOL              old_guest_vlan_glbl_enabled = old_cfg->glbl_cfg.guest_vlan_enabled;
    BOOL              new_guest_vlan_glbl_enabled = new_cfg->glbl_cfg.guest_vlan_enabled;
    BOOL              old_guest_vlan_allow_eapols = old_cfg->glbl_cfg.guest_vlan_allow_eapols;
    BOOL              new_guest_vlan_allow_eapols = new_cfg->glbl_cfg.guest_vlan_allow_eapols;
    mesa_vid_t        old_guest_vid               = old_cfg->glbl_cfg.guest_vid;
    mesa_vid_t        new_guest_vid               = new_cfg->glbl_cfg.guest_vid;
#endif
#ifdef NAS_USES_PSEC
    mesa_rc           rc;
#endif

    DOT1X_CRIT_ASSERT_LOCKED();

    // Change the age- and hold-times if requested to.
    if (isid == VTSS_ISID_GLOBAL) {
        // When just booted, it's really not necessary to apply the re-auth param to the underlying SMs, since
        // there are no such SMs attached, since the SMs call nas_os_get_reauth_timer() whenever one is allocated.
        // This means that we can do with ONLY calling nas_reauth_param_changed() when the parameters have
        // actually changed from the current configuration.
        if (new_cfg->glbl_cfg.reauth_enabled     != old_cfg->glbl_cfg.reauth_enabled ||
            new_cfg->glbl_cfg.reauth_period_secs != old_cfg->glbl_cfg.reauth_period_secs) {
            // This will loop through all attached SMs and change the reAuthEnabled and reAuthWhen settings.
            nas_reauth_param_changed(new_cfg->glbl_cfg.reauth_enabled, new_cfg->glbl_cfg.reauth_period_secs);
        }

#ifdef NAS_USES_PSEC
        // This is a fast call if nothing has changed.
        if ((rc = psec_mgmt_time_conf_set(
                      VTSS_APPL_PSEC_USER_DOT1X,
                      new_cfg->glbl_cfg.psec_aging_enabled ? new_cfg->glbl_cfg.psec_aging_period_secs : 0,
                      new_cfg->glbl_cfg.psec_hold_enabled  ? new_cfg->glbl_cfg.psec_hold_time_secs    : PSEC_HOLD_TIME_MAX)) != VTSS_RC_OK) {
            return rc;
        }
#endif
    }

    // In the following, we use zero-based port- and isid-counters.
    if ((switch_cfg_may_be_changed && isid == VTSS_ISID_GLOBAL) || (old_enabled != new_enabled)
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
        || (old_qos_glbl_enabled != new_qos_glbl_enabled)
#endif
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
        || (old_backend_vlan_glbl_enabled != new_backend_vlan_glbl_enabled)
#endif
#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
        || (old_guest_vlan_glbl_enabled != new_guest_vlan_glbl_enabled)
        || (new_guest_vlan_glbl_enabled && (old_guest_vid != new_guest_vid || old_guest_vlan_allow_eapols != new_guest_vlan_allow_eapols))
#endif
       ) {
        // If one of the global enable/disable configuration options change,
        // then we must apply the enable/disable thing to all ports.
        zisid_start = 0;
        zisid_end   = VTSS_ISID_CNT - 1;
    } else if (switch_cfg_may_be_changed && isid != VTSS_ISID_GLOBAL) {
        zisid_start = zisid_end = isid - VTSS_ISID_START;
    } else {
        // Nothing more to do.
        return VTSS_RC_OK;
    }

    for (zisid = zisid_start; zisid <= zisid_end; zisid++) {
        if (old_enabled != new_enabled) {
            // 'Guess' the best new mode, so that we send as few messages as possible.
            DOT1X_tx_switch_state(zisid + VTSS_ISID_START, new_enabled);
        }

        vtss_nas_switch_cfg_t *new_switch_cfg = &new_cfg->switch_cfg[zisid];
#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS) || defined(NAS_USES_VLAN)
        vtss_nas_switch_cfg_t *old_switch_cfg = &old_cfg->switch_cfg[zisid];
#endif
        for (api_port = 0; api_port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); api_port++) {
            // The base lib may not agree on the admin_state, because if there's no link on the port, it will be set to NAS_PORT_CONTROL_DISABLED to remove all SMs from the port.
            nas_port_t         nas_port;
            vtss_appl_nas_port_control_t cur_admin_state;
            vtss_appl_nas_port_control_t new_admin_state;
            BOOL               reauthenticate = FALSE;
#ifdef NAS_USES_VLAN
            nas_sm_t           *top_sm;
            nas_port_info_t    *port_info;
#endif

            if (api_port >= port_count_max()) {
                break;
            }

            // The base lib may not agree on the admin_state, because if there's no link on the port, it will be set to VTSS_APPL_NAS_PORT_CONTROL_DISABLED to remove all SMs from the port.
            nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(zisid + VTSS_ISID_START, api_port + VTSS_PORT_NO_START);
            cur_admin_state = nas_get_port_control(nas_port);
            new_admin_state = new_enabled ? new_switch_cfg->port_cfg[api_port].admin_state : VTSS_APPL_NAS_PORT_CONTROL_DISABLED;
#ifdef NAS_USES_VLAN
            top_sm    = nas_get_top_sm(nas_port);
            port_info = nas_get_port_info(top_sm);
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
            {
                BOOL old_qos_enabled = old_qos_glbl_enabled && old_switch_cfg->port_cfg[api_port].qos_backend_assignment_enabled;
                BOOL new_qos_enabled = new_qos_glbl_enabled && new_switch_cfg->port_cfg[api_port].qos_backend_assignment_enabled;
                if (NAS_PORT_CONTROL_IS_SINGLE_CLIENT(cur_admin_state) && new_qos_enabled != old_qos_enabled) {
                    // The port is a single-client port (which allows for overriding QoS), and the
                    // new mode is different from the old one. We need to revert any override of the QoS and request a reauthentication.
                    reauthenticate = TRUE;
                    DOT1X_qos_set(nas_port, VTSS_PRIO_NO_NONE);
                }
            }
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
            {
                BOOL old_backend_vlan_enabled = old_backend_vlan_glbl_enabled && old_switch_cfg->port_cfg[api_port].vlan_backend_assignment_enabled;
                BOOL new_backend_vlan_enabled = new_backend_vlan_glbl_enabled && new_switch_cfg->port_cfg[api_port].vlan_backend_assignment_enabled;
                if (NAS_PORT_CONTROL_IS_SINGLE_CLIENT(cur_admin_state) && old_backend_vlan_enabled != new_backend_vlan_enabled) {
                    // The port is a single-client port (which allows for overriding PVID), and the
                    // new mode is different from the old one. We need to revert any override of the VLAN and request a reauthentication.
                    reauthenticate = TRUE;

                    // Gotta revert the VLAN on the port (if set).
                    if (port_info->vlan_type == VTSS_APPL_NAS_VLAN_TYPE_BACKEND_ASSIGNED) {
                        nas_sm_t *sm = nas_get_next(top_sm);
                        if (sm) {
                            if (NAS_PORT_CONTROL_IS_MAC_TABLE_AND_BPDU_BASED(cur_admin_state)) {
#ifdef NAS_USES_PSEC
                                // In principle, I should also set whether the MAC should go into BLOCK or KEEP_BLOCKED if unauthorized, but that's
                                // impossible as of now, and I don't think it really matters.
                                u32 auth_cnt, unauth_cnt;
                                DOT1X_psec_chg(nas_port, zisid + VTSS_ISID_START, api_port + VTSS_PORT_NO_START, port_info, nas_get_eap_info(sm), nas_get_client_info(sm), nas_get_port_status(nas_port, &auth_cnt, &unauth_cnt) == VTSS_APPL_NAS_PORT_STATUS_AUTHORIZED ? PSEC_ADD_METHOD_FORWARD : PSEC_ADD_METHOD_BLOCK);
#endif
                            } else {
                                DOT1X_vlan_set(nas_port, 0, VTSS_APPL_NAS_VLAN_TYPE_NONE);
                            }
                        }
                    }
                }
            }
#endif

            // Check if we also need to tell the new admin state to the base lib.
            if (cur_admin_state != new_admin_state && DOT1X_stack_state.switch_state[zisid].port_state[api_port].flags & DOT1X_PORT_STATE_FLAGS_LINK_UP) {
                T_D("%d:%d: Chg admin state from %s to %s", zisid + VTSS_ISID_START, iport2uport(api_port + VTSS_PORT_NO_START), dot1x_port_control_to_str(cur_admin_state, FALSE), dot1x_port_control_to_str(new_admin_state, FALSE));
                DOT1X_tx_port_state(zisid + VTSS_ISID_START, api_port + VTSS_PORT_NO_START, new_enabled, new_admin_state);
                DOT1X_set_port_control(nas_port, new_admin_state, stop_reason);
            }

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
            {
                BOOL old_guest_vlan_enabled = old_guest_vlan_glbl_enabled && old_switch_cfg->port_cfg[api_port].guest_vlan_enabled;
                BOOL new_guest_vlan_enabled = new_guest_vlan_glbl_enabled && new_switch_cfg->port_cfg[api_port].guest_vlan_enabled;
                if ((old_guest_vlan_enabled && !new_guest_vlan_enabled) || (new_guest_vlan_enabled && old_guest_vlan_allow_eapols && !new_guest_vlan_allow_eapols && port_info->eapol_frame_seen)) {
                    // Gotta revert the guest VLAN on the port (if set).
                    DOT1X_guest_vlan_exit(zisid + VTSS_ISID_START, api_port + VTSS_PORT_NO_START, port_info);
                } else if (port_info->vlan_type == VTSS_APPL_NAS_VLAN_TYPE_GUEST_VLAN && old_guest_vid != new_guest_vid) {
                    DOT1X_vlan_set(nas_port, new_guest_vid, VTSS_APPL_NAS_VLAN_TYPE_GUEST_VLAN);
                } else if (!old_guest_vlan_enabled && new_guest_vlan_enabled) {
                    // Reset the reauth count.
                    nas_reset_reauth_cnt(top_sm);
                }
            }
#endif

            if (reauthenticate) {
                nas_reauthenticate(nas_port);
            }
        } /* for (api_port...) */
    } /* for (zisid... */
    return VTSS_RC_OK;
}

/******************************************************************************/
// DOT1X_default()
// Create defaults for either
//   1) the global section (#default_all == FALSE && isid == VTSS_ISID_GLOBAL)
//   2) one switch (#default_all == FALSE && VTSS_ISID_LEGAL(isid), or for both
//   3) for both the global section and all switches (#default_all == TRUE).
/******************************************************************************/
static void DOT1X_default(vtss_isid_t isid, BOOL default_all)
{
    dot1x_stack_cfg_t tmp_stack_cfg;
    BOOL              switch_cfg_may_be_changed;

    T_D("%d:<all>: Enter, default_all = %d", isid, default_all);

    DOT1X_CRIT_ASSERT_LOCKED();
    // Get the current settings into tmp_stack_cfg, so that we can compare changes later on.
    tmp_stack_cfg = DOT1X_stack_cfg;

    if (default_all) {
        DOT1X_cfg_default_all(&tmp_stack_cfg);
        isid = VTSS_ISID_GLOBAL;
        switch_cfg_may_be_changed = TRUE;
    } else if (isid == VTSS_ISID_GLOBAL) {
        // Default the global settings.
        DOT1X_cfg_default_glbl(&tmp_stack_cfg.glbl_cfg);
        switch_cfg_may_be_changed = FALSE;
    } else {
        // Default per-switch settings.
        DOT1X_cfg_default_switch(&tmp_stack_cfg.switch_cfg[isid - VTSS_ISID_START]);
        switch_cfg_may_be_changed = TRUE;
    }

    // Apply the new configuration
    (void)DOT1X_apply_cfg(isid, &tmp_stack_cfg, switch_cfg_may_be_changed, NAS_STOP_REASON_PORT_MODE_CHANGED);

    // Copy our temporary settings to the real settings.
    DOT1X_stack_cfg = tmp_stack_cfg;
}

/******************************************************************************/
// DOT1X_handle_bpdu_reception()
/******************************************************************************/
static DOT1X_INLINE void DOT1X_handle_bpdu_reception(vtss_isid_t isid, mesa_port_no_t api_port, u8 *frm, mesa_vid_t vid, size_t len)
{
    nas_port_t           nas_port      = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);
    dot1x_switch_state_t *switch_state = &DOT1X_stack_state.switch_state[isid - VTSS_ISID_START];
    vtss_appl_nas_port_control_t   admin_state   = nas_get_port_control(nas_port); // Gotta use the base-libs notion of port control, since the platform lib's port_cfg not necessarily reflects the actual.
    vtss_uport_no_t      uport         = iport2uport(api_port);
    char                 prefix_str[100];

    sprintf(prefix_str, "%d:%d(%s:%d): ", isid, uport, misc_mac2str(&frm[6]), vid);

    if (api_port >= port_count_max()) {
        T_E("%s:Invalid port number. Dropping", prefix_str);
        return;
    }

    if (switch_state->switch_exists == FALSE || (switch_state->port_state[api_port - VTSS_PORT_NO_START].flags & DOT1X_PORT_STATE_FLAGS_LINK_UP) == 0) {
        // Received either too early or too late.
        T_D("%sBPDU rxd on non-existing switch or on linked-down port. Dropping", prefix_str);
        return;
    }

    if (!NAS_PORT_CONTROL_IS_BPDU_BASED(admin_state)) {
        // Received on a port that doesn't need BPDUs.
        T_D("%sBPDU rxd on port that isn't in a BPDU-mode. Dropping", prefix_str);
        return;
    }

    if (!DOT1X_stack_cfg.glbl_cfg.enabled) {
        T_D("%sDropping BPDU as NAS is globally disabled", prefix_str);
        return;
    }

    // Here, we're either in Port-based, Single-, or Multi- 802.1X

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    {
        nas_port_info_t *port_info = nas_get_port_info(nas_get_top_sm(nas_port));
        DOT1X_guest_vlan_exit(isid, api_port, nas_get_port_info(nas_get_top_sm(nas_port)));
        port_info->eapol_frame_seen = TRUE;
    }
#endif

#ifdef NAS_DOT1X_SINGLE_OR_MULTI
    if (NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(admin_state)) {
        mesa_vid_mac_t vid_mac;
        nas_sm_t       *sm;
        mesa_rc        rc;

        // Here we're in Single- or Multi- 802.1X.

        // Received on a port that requires PSEC module intervention.
        // Check to see if we already have a state machine for this MAC address.
        memset(&vid_mac, 0, sizeof(vid_mac)); // Zero-out unused/padding bytes.
        memcpy(vid_mac.mac.addr, &frm[6], sizeof(vid_mac.mac.addr));
        vid_mac.vid = vid;
        // Only use the MAC address for lookup. And lookup on all ports.
        // Since we're in control of adding MAC addresses to the MAC table
        // (since we're in Single- or Multi-802.1X), we're also in charge
        // of detecting port-changes (and thereby possible VID changes).
        // In one of these modes, we only tolerate the same MAC address
        // once - despite the VID it was received on.
        sm = nas_get_sm_from_mac(&vid_mac);

        if (sm) {
            nas_port_info_t   *port_info   = nas_get_port_info(sm);
            vtss_nas_client_info_t *client_info = nas_get_client_info(sm);

            T_D("%sFound existing SM", prefix_str);

            // Check if it's moved.
            if (port_info->port_no != nas_port) {
                T_D("%sBPDU rxd, which was previously found on %d:%d", prefix_str, DOT1X_NAS_PORT_2_ISID(port_info->port_no), DOT1X_NAS_PORT_2_SWITCH_API_PORT(port_info->port_no));

                // Delete the old SM - both from the MAC table and the base lib
                if (NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(port_info->port_control)) {
                    // This call may fail if the port that the MAC address came from is
                    // MAC-based (as opposed to MAC-table and BPDU-based).
                    (void)psec_mgmt_mac_del(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &client_info->vid_mac);
                }

                // The following call also takes care of unregistering volatile QoS and VLAN if needed.
                nas_free_sm(sm, NAS_STOP_REASON_STATION_MOVED);
                sm = NULL; // Cause the next if() to create a new SM.
            } else if (client_info->vid_mac.vid != vid_mac.vid) {
                // Received on another VLAN ID. We don't really care right now.
                T_D("%sBPDU rxd, which is another VID than what we thought (%d)", prefix_str, client_info->vid_mac.vid);
            }
        }

        if (!sm) {
#ifdef VTSS_SW_OPTION_NAS_DOT1X_SINGLE
            // The SM doesn't exist. We need to create another one - if allowed.
            nas_port_info_t *port_info = nas_get_port_info(nas_get_top_sm(nas_port));

            // If we're in single 802.1X and one SM is already allocated, drop it.
            // The SM may time-out after some time, and get deleted so that others
            // get a chance.
            if (admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE && port_info->cur_client_cnt >= 1) {
                T_D("%sBPDU dropped because another supplicant is using the Single 802.1X SM", prefix_str);
                return;
            }
#endif

            // Check if we can add it to the PSEC module.
            T_D("%sAdding as keep blocked", prefix_str);
            rc = psec_mgmt_mac_add(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &vid_mac, PSEC_ADD_METHOD_KEEP_BLOCKED);
            if (rc != VTSS_RC_OK) {
                // Either one of the other modules denied this MAC address, or it's a zombie (a MAC address that couldn't be
                // added to the MAC table due to its hash layout, or because the MAC module ran out of software entries).
                // Either way, we attempt to delete it (in case its a zombie).
                T_W("%sPSEC add failed: \"%s\". Deleting it from PSEC", prefix_str, error_txt(rc));
                (void)psec_mgmt_mac_del(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &vid_mac);

                if (rc == VTSS_APPL_PSEC_RC_PORT_IS_SHUT_DOWN) {
                    // Since DOT1X_on_mac_del_callback() won't get called back if the Limit module determines to shut down the port
                    // during the above call to psec_mgmt_mac_add() (because the PSEC module don't call the calling user back to avoid
                    // mutex problems), we have to trap this event and remove all state machines on the port.
                    // The easiest way to do this is to fake an admin_state change.
                    T_D("%sRemoving all SMs from port", prefix_str);
                    DOT1X_set_port_control(nas_port, admin_state, NAS_STOP_REASON_PORT_SHUT_DOWN);
                }
                return;
            }

            // Now allocate an SM for it.
            T_D("%sAllocating SM", prefix_str);
            if ((sm = nas_alloc_sm(nas_port, &vid_mac)) == NULL) {
                T_E("%sCan't allocate SM. Moving entry to blocked w/ hold-time-expiration)", prefix_str);
                // We're out of SMs. Tell the PSEC module to time it out.
                // This will result in another error when the MAC address is removed
                // in the PSEC module (in the callback call to DOT1X_on_mac_del_callback()).
                (void)psec_mgmt_mac_chg(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &vid_mac, PSEC_ADD_METHOD_BLOCK);
                return;
            }
        }

        // Now the SM definitely exists. Give the BPDU to it.
        nas_ieee8021x_eapol_frame_received_sm(sm, frm, vid, len);
    } else
#endif
    {
        // Here we're in Port-based 802.1X
        nas_ieee8021x_eapol_frame_received(nas_port, frm, vid, len);
    }
}

/******************************************************************************/
// DOT1X_msg_rx()
/******************************************************************************/
static BOOL DOT1X_msg_rx(void *contxt, const void *const the_rxd_msg, size_t len, vtss_module_id_t modid, u32 isid)
{
    mesa_rc     rc;
    dot1x_msg_t *rx_msg = (dot1x_msg_t *)the_rxd_msg;

    T_N("%u:<any>: msg_id: %d, %s, ver: %u, len: %zu", isid, rx_msg->hdr.msg_id, DOT1X_msg_id_to_str(rx_msg->hdr.msg_id), rx_msg->hdr.version, len);

    // Check if we support this version of the message. If not, print a warning and return.
    if (rx_msg->hdr.version != DOT1X_MSG_VERSION) {
        T_W("%u:<any>: Unsupported version of the message (%u)", isid, rx_msg->hdr.version);
        return TRUE;
    }

    switch (rx_msg->hdr.msg_id) {
    case DOT1X_MSG_ID_PRI_TO_SEC_PORT_STATE: {
        // Set port state.
        dot1x_msg_port_state_t *msg  = &rx_msg->u.port_state;
        if (msg->api_port >= port_count_max()) {
            T_E("VTSS_ISID_LOCAL:%d: Invalid port number", iport2uport(msg->api_port));
            break;
        }
        if ((rc = mesa_auth_port_state_set(NULL, msg->api_port, msg->authorized ? MESA_AUTH_STATE_BOTH : MESA_AUTH_STATE_NONE)) != VTSS_RC_OK) {
            T_E("VTSS_ISID_LOCAL:%d: API err: %s", iport2uport(msg->api_port), error_txt(rc));
        }
        break;
    }

    case DOT1X_MSG_ID_PRI_TO_SEC_EAPOL: {
        // Transmit an EAPOL frame (BPDU) on a front port.
        dot1x_msg_eapol_t *msg = &rx_msg->u.eapol;
        uchar             *frm;
        size_t            tx_len;
        packet_tx_props_t tx_props;

        // Allocate a buffer for the frame.
        tx_len = MAX(64 - FCS_SIZE_BYTES, msg->len);
        frm = packet_tx_alloc(tx_len);
        if (frm == NULL) {
            T_W("%u:%d: Unable to allocate %zu bytes", isid, iport2uport(msg->api_port), tx_len);
            break;
        }

        if (tx_len > msg->len) {
            // Make sure unused bytes of the frame are zeroed out.
            memset(frm + msg->len, 0, tx_len - msg->len);
        }

        memcpy(frm, msg->frm, msg->len);
        packet_tx_props_init(&tx_props);
        tx_props.packet_info.modid     = VTSS_MODULE_ID_DOT1X;
        tx_props.packet_info.frm       = frm;
        tx_props.packet_info.len       = tx_len; // On Linux, we need to tell a minimum-sized Ethernet frame in order to get the last bytes of a shorter EAPOL frame zeroed out.
        tx_props.tx_info.dst_port_mask = VTSS_BIT64(msg->api_port);
        tx_props.tx_info.cos           = MESA_PRIO_CNT; // Super priority
        (void)packet_tx(&tx_props);
        break;
    }

    case DOT1X_MSG_ID_SEC_TO_PRI_EAPOL: {
        // Received a BPDU on some front port in the stack.
        // We need to be primary switch in order to handle it.
        dot1x_msg_eapol_t *msg = &rx_msg->u.eapol;

        if (!msg_switch_is_primary()) {
            break;
        }
        T_N("%u:%d: Received BPDU, len=%zu, vid=%d", isid, iport2uport(msg->api_port), msg->len, msg->vid);
        if (!VTSS_ISID_LEGAL(isid)) {
            T_W("%u:%d: Invalid ISID. Ignoring", isid, iport2uport(msg->api_port));
            break;
        }

        DOT1X_CRIT_ENTER();
        DOT1X_handle_bpdu_reception(isid, msg->api_port, msg->frm, msg->vid, msg->len);
        DOT1X_CRIT_EXIT();
        break;
    }

    default:
        T_D("%u:<any>: Unknown message ID: %d", isid, rx_msg->hdr.msg_id);
        break;
    }
    return TRUE;
}

/******************************************************************************/
// DOT1X_rx_bpdu()
// 802.1X BPDU packet reception on local switch.
// Send it to the current primary switch.
/******************************************************************************/
static BOOL DOT1X_rx_bpdu(void *contxt, const uchar *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    BOOL rc = FALSE; // Allow other subscribers to receive the packet

    T_N("<sec>:%d: Rx'd BPDU from %s", iport2uport(rx_info->port_no), misc_mac2str(&frm[6]));

    // Check if we are able to transport it.
    if (rx_info->length > NAS_IEEE8021X_MAX_EAPOL_PACKET_SIZE) {
        T_E("<sec>:%d: Received BPDU with length (%u) larger than supported (%d). Dropping it", iport2uport(rx_info->port_no), rx_info->length, NAS_IEEE8021X_MAX_EAPOL_PACKET_SIZE);
        return rc;
    }

    // Transmit the frame to the primary switch
    DOT1X_msg_tx_eapol(0, rx_info->port_no, DOT1X_MSG_ID_SEC_TO_PRI_EAPOL, rx_info->tag.vid, frm, rx_info->length);
    return rc;
}

/******************************************************************************/
// DOT1X_thread()
/******************************************************************************/
static void DOT1X_thread(vtss_addrword_t data)
{
    while (1) {
        if (msg_switch_is_primary()) {
            while (msg_switch_is_primary()) {

                // We should timeout every one second (1000 ms)
                VTSS_OS_MSLEEP(1000);
                if (!msg_switch_is_primary()) {
                    break;
                }

                // Timeout.
                DOT1X_CRIT_ENTER();
                if (DOT1X_stack_cfg.glbl_cfg.enabled) {
                    nas_1sec_timer_tick();
                }
                DOT1X_CRIT_EXIT();
            }
        }

        // No longer primary switch. Time to bail out.
        // No reason for using CPU ressources when we're a secondary switch
        T_I("Suspending 802.1X thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_DOT1X);
        T_I("Resumed 802.1X thread");
    } // while (1)
}

/******************************************************************************/
// DOT1X_msg_rx_init()
/******************************************************************************/
static DOT1X_INLINE void DOT1X_msg_rx_init(void)
{
    msg_rx_filter_t filter;
    mesa_rc         rc;

    memset(&filter, 0, sizeof(filter));
    filter.cb = DOT1X_msg_rx;
    filter.modid = VTSS_MODULE_ID_DOT1X;
    if ((rc = msg_rx_filter_register(&filter)) != VTSS_RC_OK) {
        T_E("Unable to register for messages (%s)", error_txt(rc));
    }
}

/******************************************************************************/
// DOT1X_bpdu_rx_init()
/******************************************************************************/
static DOT1X_INLINE void DOT1X_bpdu_rx_init(void)
{
    packet_rx_filter_t rx_filter;
    void               *rx_filter_id;
    mesa_rc            rc;
    mesa_mac_addr_t    bpdu_mac = NAS_IEEE8021X_MAC_ADDR;

    // BPDUs are already being captured (initialized from
    // ../port/port.cxx::port_api_init() by call to vtss_init()).
    // We need to register for 802.1X BPDUs from the packet module
    // so that they get to us as they arrive.
    packet_rx_filter_init(&rx_filter);
    memcpy(rx_filter.dmac, bpdu_mac, sizeof(rx_filter.dmac));
    rx_filter.modid = VTSS_MODULE_ID_DOT1X;
    rx_filter.match = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_ETYPE;
    rx_filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;
    rx_filter.etype = NAS_IEEE8021X_ETH_TYPE;
    rx_filter.cb    = DOT1X_rx_bpdu;
    if ((rc = packet_rx_filter_register(&rx_filter, &rx_filter_id)) != VTSS_RC_OK) {
        T_E("Unable to register for BPDUs (%s)", error_txt(rc));
    }
}

/******************************************************************************/
// DOT1X_isid_port_check()
// Returns VTSS_RC_OK if we're primary switch, and isid and api_port are legal values, or
// if we're not primary switch but VTSS_ISID_LOCAL is allowed and api_port is legal.
/******************************************************************************/
static mesa_rc DOT1X_isid_port_check(vtss_isid_t isid, mesa_port_no_t api_port, BOOL allow_local, BOOL check_port)
{
    if ((isid != VTSS_ISID_LOCAL && !VTSS_ISID_LEGAL(isid)) || (isid == VTSS_ISID_LOCAL && !allow_local)) {
        return VTSS_APPL_NAS_ERROR_SID;
    }

    if (isid != VTSS_ISID_LOCAL && !msg_switch_is_primary()) {
        return VTSS_APPL_NAS_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (check_port && api_port >= port_count_max()) {
        return VTSS_APPL_NAS_ERROR_PORT;
    }

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_RFC4675
/****************************************************************************/
// DOT1X_backend_check_qos_rfc4675()
/****************************************************************************/
static DOT1X_INLINE mesa_prio_t DOT1X_backend_check_qos_rfc4675(u8 radius_handle)
{
#define USER_PRIO_TABLE_LEN 8
    u16         len = USER_PRIO_TABLE_LEN;
    u8          attr[USER_PRIO_TABLE_LEN];
    int         i;
    mesa_prio_t result = VTSS_PRIO_NO_NONE;

    if (vtss_radius_tlv_get(radius_handle, VTSS_RADIUS_ATTRIBUTE_USER_PRIORITY_TABLE, &len, attr) == VTSS_RC_OK) {
        // Attribute is present. Check to see if its valid.

        // Exactly USER_PRIO_TABLE_LEN bytes in attribute
        if (len != USER_PRIO_TABLE_LEN) {
#ifdef VTSS_SW_OPTION_SYSLOG
            S_W("RADIUS-ASSIGNED_QOS: Invalid RADIUS attribute length (got %u, expected %d)", len, USER_PRIO_TABLE_LEN);
#endif
            T_D("QoS(RFC4675): Invalid RADIUS attribute length (got %u, expected %d)", len, USER_PRIO_TABLE_LEN);
            return result;
        }

        // All USER_PRIO_TABLE_LEN octets must be the same
        for (i = 1; i < USER_PRIO_TABLE_LEN; i++) {
            if (attr[i - 1] != attr[i]) {
#ifdef VTSS_SW_OPTION_SYSLOG
                S_W("RADIUS-ASSIGNED_QOS: The %d octets are not the same", USER_PRIO_TABLE_LEN);
#endif
                T_D("QoS(RFC4675): The %d octets are not the same", USER_PRIO_TABLE_LEN);
                return result;
            }
        }

        // Must be in range [0; VTSS_PRIOS[.
        if (attr[0] < '0' || attr[0] >= '0' + VTSS_PRIOS) {
#ifdef VTSS_SW_OPTION_SYSLOG
            S_W("RADIUS-ASSIGNED_QOS: The QoS class is out of range (act: %d, allowed: [0; %d])", attr[0] - '0', VTSS_PRIOS - 1);
#endif
            T_D("QoS(RFC4675): The QoS class is out of range (act: 0x%x, allowed: [0x30; 0x%x])", attr[0], '0' + VTSS_PRIOS - 1);
            return result;
        }

        result = attr[0] - '0';
        T_D("QoS(RFC4675): Setting QoS class to %u", result);
    }
#undef USER_PRIO_TABLE_LEN

    return result;
}
#endif /* VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_RFC4675 */

#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_CUSTOM) || defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN)
/****************************************************************************/
// DOT1X_isdigits()
/****************************************************************************/
static DOT1X_INLINE BOOL DOT1X_isdigits(u8 *ptr, int len)
{
    while (len-- > 0) {
        if (!isdigit(*(ptr++))) {
            return FALSE;
        }
    }
    return TRUE;
}
#endif

#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_CUSTOM) || defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN)
/****************************************************************************/
// DOT1X_strtoul()
// It is assumed that the string only contains ASCII digits in range '0'-'9'.
/****************************************************************************/
static DOT1X_INLINE BOOL DOT1X_strtoul(u8 *ptr, int len, u32 max, u32 *result)
{
    *result = 0;

    // Skip leading zeros
    while (len > 0) {
        // Skip leading zeros
        if (*ptr == '0') {
            len--;
            ptr++;
        } else {
            break;
        }
    }

    // For each iteration check if we'we exceeded max,
    // so that we don't run into wrap-around problems
    // if the number represented in the string is too
    // long.
    while (len-- > 0) {
        u8 ch = *(ptr++);
        *result *= 10;
        *result += ch - '0';
        if (*result > max) {
            return FALSE;
        }
    }
    return TRUE;
}
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_CUSTOM
/****************************************************************************/
// DOT1X_backend_check_qos_custom()
/****************************************************************************/
static DOT1X_INLINE mesa_prio_t DOT1X_backend_check_qos_custom(u8 radius_handle)
{
    // The attribute values come from nas_qos_custom_api.h, which can be overridden by the customer.
    vtss_radius_attributes_e avp_type;
    u8                       avp_len;
    u8                       *avp_ptr;
    u8                       *avp_start;
    BOOL                     start_over = TRUE;
    size_t                   prefix_len = strlen(NAS_QOS_CUSTOM_VSA_PREFIX);
    u32                      val;
    mesa_prio_t              result = VTSS_PRIO_NO_NONE;
    BOOL                     one_found = FALSE;

    // We have to iterate over the TLVs, since the same custom type may be present
    // more than once among the TLVs (AVP pairs) in the RADIUS frame.
    while (vtss_radius_tlv_iterate(radius_handle, &avp_type, &avp_len, (const u8 **)&avp_start, start_over) == VTSS_RC_OK) {
        // Next time, get the next TLV
        start_over = FALSE;

        // Use avp_ptr until we reach the decoding of the QoS Class.
        avp_ptr = avp_start;

        // Check if it's the one we're looking for.
        if (avp_type != NAS_QOS_CUSTOM_AVP_TYPE) {
            // Nope. Go on with the next attribute.
            continue;
        }

        // avp_len contains the number of octets in avp_ptr. This must be greater than
        // AVP-VendorID + VSA-Type + VSA-Length + X + Y = 4 + 1 + 1 + X + Y.
        if (avp_len <= 6 + prefix_len) {
            // Too short to be ours
            continue;
        }

        // Check Vendor ID. MSByte must be zero.
        if (avp_ptr[0] != 0                                         ||
            avp_ptr[1] != ((NAS_QOS_CUSTOM_VENDOR_ID >> 16) & 0xFF) ||
            avp_ptr[2] != ((NAS_QOS_CUSTOM_VENDOR_ID >>  8) & 0xFF) ||
            avp_ptr[3] != ((NAS_QOS_CUSTOM_VENDOR_ID >>  0) & 0xFF)) {
            // Doesn't match vendor ID. Next.
            continue;
        }

        // Next field
        avp_ptr += 4;

        // Check VSA-Type
        if (avp_ptr[0] != NAS_QOS_CUSTOM_VSA_TYPE) {
            // Not the right type.
            continue;
        }

        // Next field
        avp_ptr++;

        // Check the VSA-Length.
        // The VSA-Length includes the VSA-Type, itself, the prefix and the value.
        // The @avp_len includes the same lengths, but the vtss_radius API has
        // already removed 2 bytes for the AVP type and AVP length, so we expect
        // the VSA-length to be 4 bytes (corresponding the the Vendor ID) longer
        // than the VSA length.
        if (avp_ptr[0] != avp_len - 4) {
            // Doesn't match the way that we lay out vendor specific attributes.
            continue;
        }

        // Next field
        avp_ptr++;

        // Check the prefix string
        if (strncmp((char *)avp_ptr, NAS_QOS_CUSTOM_VSA_PREFIX, prefix_len) != 0) {
            // Doesn't match the prefix string. Next.
            continue;
        }

        // Proceed to the first char of the actual vendor-specific attribute value.
        avp_ptr += prefix_len;

        // This is the number of chars left for the actual QoS class.
        avp_len -= (avp_ptr - avp_start);

#if VTSS_PRIOS == 4
        // The QoS class can be either of "low", "normal", "medium", and "high".
        if (avp_len == 3 && (strncasecmp((char *)avp_ptr, "low", 3)) == 0) {
            result    = 0;
            one_found = TRUE;
            continue;
        } else if (avp_len == 6 && (strncasecmp((char *)avp_ptr, "normal", 6)) == 0) {
            result    = 1;
            one_found = TRUE;
            continue;
        } else if (avp_len == 6 && (strncasecmp((char *)avp_ptr, "medium", 6)) == 0) {
            result    = 2;
            one_found = TRUE;
            continue;
        } else if (avp_len == 4 && (strncasecmp((char *)avp_ptr, "high", 4)) == 0) {
            result    = 3;
            one_found = TRUE;
            continue;
        }
#endif

        // The Qos Class must be a decimal string between 0 and VTSS_PRIOS - 1.
        // Since the buffer isn't terminated, we iterate until there are no more chars.
        // First check if it contains only valid decimal chars.
        if (!DOT1X_isdigits(avp_ptr, avp_len)) {
#ifdef VTSS_SW_OPTION_SYSLOG
            S_W("RADIUS-ASSIGNED_QOS: Invalid character found in decimal ASCII string");
#endif
            T_D("QoS(Custom): Invalid char found in decimal ASCII string");
            return VTSS_PRIO_NO_NONE;
        }

        // Then get the value.
        if (!DOT1X_strtoul(avp_ptr, avp_len, VTSS_PRIOS - 1, &val)) {
#ifdef VTSS_SW_OPTION_SYSLOG
            S_W("RADIUS-ASSIGNED_QOS: The QoS class is out of range ([0; %d])", VTSS_PRIOS - 1);
#endif
            T_D("QoS(Custom): The QoS class is out of range ([0; %d])", VTSS_PRIOS - 1);
            return VTSS_PRIO_NO_NONE;
        }

        // Success
        if (one_found) {
            // There shall only be one.
#ifdef VTSS_SW_OPTION_SYSLOG
            S_W("RADIUS-ASSIGNED_QOS: Two or more QoS class specifications found");
#endif
            T_D("QoS(Custom): Two or more QoS class specifications found");
            return VTSS_PRIO_NO_NONE;
        } else {
            result = val;
            one_found = TRUE;
        }
    }

    if (result != VTSS_PRIO_NO_NONE) {
        T_D("QoS(Custom): Setting QoS class to %lu", result);
    }

    return result;
}
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
/****************************************************************************/
// DOT1X_backend_check_qos()
/****************************************************************************/
static DOT1X_INLINE void DOT1X_backend_check_qos(nas_sm_t *sm, u8 radius_handle)
{
    mesa_prio_t     qos_class = VTSS_PRIO_NO_NONE;
    nas_port_info_t *port_info = nas_get_port_info(sm);
    vtss_isid_t     isid;
    mesa_port_no_t  api_port;

    DOT1X_CRIT_ASSERT_LOCKED();

    if (!NAS_PORT_CONTROL_IS_SINGLE_CLIENT(port_info->port_control)) {
        return; // Don't care when not in Port-based or Single 802.1X
    }

    isid     = DOT1X_NAS_PORT_2_ISID(port_info->port_no);
    api_port = DOT1X_NAS_PORT_2_SWITCH_API_PORT(port_info->port_no);

    if (!DOT1X_stack_cfg.glbl_cfg.qos_backend_assignment_enabled || !DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port - VTSS_PORT_NO_START].qos_backend_assignment_enabled) {
        return; // Nothing to do.
    }

    // One of three methods supported:
    //   a) RFC4675 (VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_RFC4675
    //   b) Vitesse-specific (VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_CUSTOM)
    //   c) Customer-specific (VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_CUSTOM)
    // The difference between b) and c) is only the contents of nas_qos_custom_api.h.
#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_RFC4675)
    qos_class = DOT1X_backend_check_qos_rfc4675(radius_handle);
#elif defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_CUSTOM)
    qos_class = DOT1X_backend_check_qos_custom(radius_handle);
#else
#error "At least one of the methods RFC4675 or Custom must be selected"
#endif

    DOT1X_qos_set(port_info->port_no, qos_class);
}
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
/****************************************************************************/
// DOT1X_backend_check_vlan_rfc2868_rfc3580()
/****************************************************************************/
// I think there's a bug in FlexeLint. It states that:
//   error 438: (Warning -- Last value assigned to variable 'start_over' (defined at line 1731) not used)
//   error 830: (Info -- Location cited in prior message)
// But I'm pretty sure that all assignments to start_over are used.
/*lint -e{438} */
static mesa_vid_t DOT1X_backend_check_vlan_rfc2868_rfc3580(u8 radius_handle)
{
    BOOL                     tunnel_type_seen                  = FALSE;
    BOOL                     matching_tunnel_medium_type_seen  = FALSE;
    BOOL                     start_over                        = TRUE;
    u8                       *tlv_tunnel_type_ptr              = NULL;
    u8                       tag                               = 0;
    mesa_vid_t               result                            = VTSS_VID_NULL;
    vtss_radius_attributes_e tlv_type;
    u8                       tlv_len;
    u8                       *tlv_ptr;
    u32                      val;

    // Gotta traverse the RADIUS attributes a couple of times because we need
    // info from the Tunnel-Type and Tunnel-Medium-Type attributes before being
    // able to assess the Private-Group-ID
    while (vtss_radius_tlv_iterate(radius_handle, &tlv_type, &tlv_len, (const u8 **)&tlv_ptr, start_over) == VTSS_RC_OK) {
        // Next time, get the next TLV
        start_over = FALSE;

        // Check to see if we're trying to continue from previous iteration
        // in case we haven't found a matching Tunnel-Medium-Type attribute
        if (tlv_tunnel_type_ptr != NULL && tlv_ptr <= tlv_tunnel_type_ptr) {
            continue;
        }

        // Look for the Tunnel-Type (64; RFC2868)
        if (tlv_type != VTSS_RADIUS_ATTRIBUTE_TUNNEL_TYPE) {
            continue;
        }

        // Check length. Two bytes are already subtracted by the RADIUS module (type and length)
        if (tlv_len != 4) {
            continue;
        }

        // Get the Tag. This must be a number in range [0x00; 0x1F].
        if ((tag = tlv_ptr[0]) > 0x1F) {
            // Invalid.
            continue;
        }

        // Check Tunnel Type. This must be "VLAN" (13 = 0x0D) as defined in RFC3580
        if (tlv_ptr[1] != 0x00 ||
            tlv_ptr[2] != 0x00 ||
            tlv_ptr[3] != 0x0D) {
            continue;
        }

        tunnel_type_seen = TRUE;

        // Keep a snap-shot of this attribute's TLV ptr for use in case we couldn't
        // find a matching Tunnel-Medium-Type.
        tlv_tunnel_type_ptr = tlv_ptr;

        // Now search for the corresponding Tunnel-Media-Type
        matching_tunnel_medium_type_seen = FALSE;
        start_over                       = TRUE;
        while (vtss_radius_tlv_iterate(radius_handle, &tlv_type, &tlv_len, (const u8 **)&tlv_ptr, start_over) == VTSS_RC_OK) {
            // Next time, get the next TLV
            start_over = FALSE;

            // Look for the Tunnel-Medium-Type (65; RFC2868)
            if (tlv_type != VTSS_RADIUS_ATTRIBUTE_TUNNEL_MEDIUM_TYPE) {
                continue;
            }

            // Check length. Two bytes are already subtracted by the RADIUS module (type and length)
            if (tlv_len != 4) {
                continue;
            }

            // Get the Tag. This must be the same as for the Tunnel-Type. Otherwise
            // we haven't found the matching attribute.
            if (tag != tlv_ptr[0]) {
                continue;
            }

            // Check Medium Type. This must be set to 0x00 0x00 0x06 for IEEE-802
            if (tlv_ptr[1] != 0x00 ||
                tlv_ptr[2] != 0x00 ||
                tlv_ptr[3] != 0x06) {
                continue;
            }

            // Got it.
            matching_tunnel_medium_type_seen = TRUE;
            break;
        }

        if (!matching_tunnel_medium_type_seen) {
            // We gotta find the next Tunnel-Type and see if that one
            // has a matching Tunnel-Medium-Type.
            // The tlv_tunnel_type_ptr is now non-NULL, which means
            // that the outer TLV iterator skips all the TLVs up to
            // and including the Tunnel-Type that didn't have a
            // Tunnel-Medium-Type match.
            tunnel_type_seen = FALSE;
            start_over       = TRUE;
        } else {
            break;
        }
    }

    if (tunnel_type_seen == FALSE || matching_tunnel_medium_type_seen == FALSE) {
        // No matching Tunnel-Type and Tunnel-Type-Medium attributes found.
        return 0;
    }

    // Now that we have the Tag, go and get the VLAN ID from the Tunnel-Private-Group-ID
    start_over = TRUE;
    while (vtss_radius_tlv_iterate(radius_handle, &tlv_type, &tlv_len, (const u8 **)&tlv_ptr, start_over) == VTSS_RC_OK) {
        // Next time, get the next TLV
        start_over = FALSE;

        // Look for the Tunnel-Private-Group-ID (81; RFC2868)
        if (tlv_type != VTSS_RADIUS_ATTRIBUTE_TUNNEL_PRIVATE_GROUP_ID) {
            continue;
        }

        // Check length. Two bytes are already subtracted by the RADIUS module (type and length)
        // The length must accommodate at least one byte.
        if (tlv_len == 0) {
            continue;
        }

        // Check the tag.
        // If Tag from Tunnel-Type and Tunnel-Medium-Type is non-zero, then the
        // tag of the Tunnel-Private-Group-ID must be the same.
        // If Tag from Tunnel-Type and Tunnel-Medium-Type is zero, then the
        // tag of the Tunnel-Private-Group-ID can be anything. If it's greater
        // than 0x1F, then it's considered the first byte of the VLAN ID.
        if (tag != 0x00) {
            if (tlv_ptr[0] != tag) {
                // When Tag is non-zero, the Tunnel-Private-Group-ID tag must be the same.
                continue;
            } else {
                // Skip the tag
                tlv_ptr++;
                tlv_len--;
            }
        } else if (tlv_ptr[0] <= 0x1F) {
            tlv_ptr++;
            tlv_len--;
        }

        // If there are no bytes left after eating a possible tag, try the next TLV.
        if (tlv_len == 0) {
            continue;
        }

        // Now it's time to interpret the VLAN ID.
        // Check if it's all digits.
        if (!DOT1X_isdigits(tlv_ptr, tlv_len)) {
            // It's not.
            // If the switch supports VLAN naming, try to look it up.
#ifdef VTSS_SW_OPTION_VLAN_NAMING
            char       vlan_name[VTSS_APPL_VLAN_NAME_MAX_LEN];
            mesa_vid_t vid;

            if (tlv_len > sizeof(vlan_name) - 1) {
#ifdef VTSS_SW_OPTION_SYSLOG
                S_W("RADIUS-ASSIGNED_VLAN: VLAN name (%s) too long. Supported length: " VPRIz" characters", tlv_ptr, sizeof(vlan_name) - 1);
#endif
                T_D("RADIUS-assigned VLAN: VLAN name (%s) too long. Supported length: " VPRIz" characters", tlv_ptr, sizeof(vlan_name) - 1);
            } else {
                strncpy(vlan_name, (char *)tlv_ptr, tlv_len);
                vlan_name[tlv_len] = '\0';
                if (vlan_mgmt_name_to_vid(vlan_name, &vid) != VTSS_RC_OK) {
#ifdef VTSS_SW_OPTION_SYSLOG
                    S_W("RADIUS-ASSIGNED_VLAN: VLAN name (%s) not found", vlan_name);
#endif
                    T_D("RADIUS-assigned VLAN: VLAN name (%s) not found", vlan_name);
                } else {
                    result = vid;
                }
            }
            break;
#else /* !defined(VTSS_SW_OPTION_VLAN_NAMING) */
#ifdef VTSS_SW_OPTION_SYSLOG
            S_W("RADIUS-ASSIGNED_VLAN: Invalid character found in decimal ASCII string");
#endif
            T_D("RADIUS-ASSIGNED_VLAN: Invalid character found in decimal ASCII string");
            break;
#endif /* !defined(VTSS_SW_OPTION_VLAN_NAMING) */
        }

        // Parse decimal string
        // Then get the value.
        if (!DOT1X_strtoul(tlv_ptr, tlv_len, VTSS_VIDS - 1, &val) || val == 0) {
#ifdef VTSS_SW_OPTION_SYSLOG
            S_W("RADIUS-ASSIGNED_VLAN: The VLAN ID is out of range ([1; %d])", VTSS_VIDS - 1);
#endif
            T_D("RADIUS-assigned VLAN: The VLAN ID is out of range ([1; %d])", VTSS_VIDS - 1);
            break;
        }

        result = val;
        break;
    }

    if (result != VTSS_VID_NULL) {
        T_D("RADIUS-assigned VLAN: Setting VLAN ID to %d", result);
    }

    return result;
}
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
/****************************************************************************/
// DOT1X_backend_check_vlan()
/****************************************************************************/
static void DOT1X_backend_check_vlan(nas_sm_t *sm, u8 radius_handle)
{
    nas_port_info_t *port_info = nas_get_port_info(sm);
    vtss_isid_t     isid;
    mesa_port_no_t  api_port;

    DOT1X_CRIT_ASSERT_LOCKED();

    if (!NAS_PORT_CONTROL_IS_SINGLE_CLIENT(port_info->port_control)) {
        return; // Don't care when not in Port-based or Single 802.1X
    }

    isid     = DOT1X_NAS_PORT_2_ISID(port_info->port_no);
    api_port = DOT1X_NAS_PORT_2_SWITCH_API_PORT(port_info->port_no);

    if (!DOT1X_stack_cfg.glbl_cfg.vlan_backend_assignment_enabled || !DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port - VTSS_PORT_NO_START].vlan_backend_assignment_enabled) {
        return; // Nothing to do.
    }

    // Defer setting the actual VID until the call to nas_os_set_authorized()
    nas_get_port_info(sm)->backend_assigned_vid = DOT1X_backend_check_vlan_rfc2868_rfc3580(radius_handle);
}
#endif

/****************************************************************************/
// DOT1X_syslog_warning()
/****************************************************************************/
static void DOT1X_syslog_warning(mesa_rc rc)
{
    BOOL          sent_to_syslog = FALSE;

#ifdef VTSS_SW_OPTION_SYSLOG
    static time_t last_s_w;
    time_t        cur_time = time(NULL);

    // Hold back these messages, since they could otherwise come quite rapidly
    // and we don't want to spam the syslog.
    if (cur_time - last_s_w >= 30) {
        last_s_w = cur_time;
        S_W("NAS-RADIUS_SERVER: %s", error_txt(rc));
        sent_to_syslog = TRUE;
    }
#endif

    T_D("NAS-RADIUS_SERVER: %s (%ssent to syslog)", error_txt(rc), sent_to_syslog ? "" : "not ");
}

/****************************************************************************/
// DOT1X_radius_rx_callback()
/****************************************************************************/
static void DOT1X_radius_rx_callback(u8 radius_handle, ctx_t ctx, vtss_radius_access_codes_e code, vtss_radius_rx_callback_result_e res, u32 radius_server_id)
{
    char            buf[DOT1X_MAC_STR_BUF_SIZE];
    nas_sm_t        *sm;
    nas_port_t      nas_port;
    u16             tlv_len;
    nas_eap_info_t  *eap_info;
    vtss_isid_t     isid;
    vtss_uport_no_t uport;

    // We use the nas_port as context, rather than the state machine itself, because the
    // state machine may have been used for other purposes once we get called back. Furthermore,
    // if we had used dynamic memory allocation for statemachines (we don't), then the ctx
    // may have been out-of-date (freed) once called back.
    // The combination of port number and RADIUS identifier should uniquely identify the SM.
    nas_port = ctx.u;
    isid     = DOT1X_NAS_PORT_2_ISID(nas_port);
    uport    = iport2uport(DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port));

    DOT1X_CRIT_ENTER();

    // Look up the state-machine based on <port, radius_handle> tuple.
    sm = nas_get_sm_from_radius_handle(nas_port, (int)((unsigned int)radius_handle));

    if (!sm) {
        // It has been removed, probably due to port down or configuration change. Even if
        // we have called vtss_radius_free(), it may be that we get called back because of an unavoidable
        // race-condition.
        T_I("%d:%d: Couldn't find matching SM for RADIUS handle=%u", isid, uport, radius_handle);
        goto do_exit;
    }

    T_I("%s: RADIUS callback (handle = %u, server_id = %u). Result = %d", DOT1X_vid_mac_to_str(uport2iport(uport), &nas_get_client_info(sm)->vid_mac, buf), radius_handle, radius_server_id, res);

    eap_info = nas_get_eap_info(sm);
    VTSS_ASSERT(eap_info);
    eap_info->radius_handle = -1;
    eap_info->radius_server_id = 0;

    if (!DOT1X_stack_cfg.glbl_cfg.enabled) {
        goto do_exit;
    }

    if (res == VTSS_RADIUS_RX_CALLBACK_OK) {
        switch (code) {
        case VTSS_RADIUS_CODE_ACCESS_ACCEPT:
        case VTSS_RADIUS_CODE_ACCESS_REJECT:
        case VTSS_RADIUS_CODE_ACCESS_CHALLENGE: {
            // Get the TLVs.
            tlv_len = sizeof(eap_info->radius_state_attribute);
            if (vtss_radius_tlv_get(radius_handle, VTSS_RADIUS_ATTRIBUTE_STATE, &tlv_len, eap_info->radius_state_attribute) != VTSS_RC_OK) {
                eap_info->radius_state_attribute_len = 0;
            } else {
                eap_info->radius_state_attribute_len = tlv_len;
            }

            tlv_len = sizeof(eap_info->last_frame);
            if (vtss_radius_tlv_get(radius_handle, VTSS_RADIUS_ATTRIBUTE_EAP_MESSAGE, &tlv_len, eap_info->last_frame) != VTSS_RC_OK) {
                // EAP message TLV not found in this RADIUS frame.
                // Use the previously received.
                if (eap_info->radius_eap_attribute_len > 0) {
                    memcpy(eap_info->last_frame, eap_info->radius_eap_attribute, eap_info->radius_eap_attribute_len);
                    eap_info->last_frame_len = eap_info->radius_eap_attribute_len;
                } else {
                    eap_info->last_frame_len = 0;
                }
            } else {
                // In case a RADIUS frame is split in two, we save the EAP
                // message for later use.
                eap_info->last_frame_len = tlv_len;
                eap_info->radius_eap_attribute_len = (tlv_len > sizeof(eap_info->radius_eap_attribute)) ? sizeof(eap_info->radius_eap_attribute) : tlv_len;
                memcpy(eap_info->radius_eap_attribute, eap_info->last_frame, eap_info->radius_eap_attribute_len);
            }

            // Register this frame type
            eap_info->last_frame_type = FRAME_TYPE_EAPOL;

            if (code == VTSS_RADIUS_CODE_ACCESS_ACCEPT) {
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
                DOT1X_backend_check_qos(sm, radius_handle);
#endif
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
                DOT1X_backend_check_vlan(sm, radius_handle);
#endif
            }

#ifdef VTSS_SW_OPTION_DOT1X_ACCT
            dot1x_acct_radius_rx(radius_handle, eap_info);
#endif

            if (code == VTSS_RADIUS_CODE_ACCESS_CHALLENGE) {
                T_D("Setting eap_info->radius_server_id to %u", radius_server_id);
                eap_info->radius_server_id = radius_server_id; // More requests are expected to come to the same server
            }

            // All required info has now been saved in SM's eap_info, so it's up
            // to the SM to do the rest.
            nas_backend_frame_received(sm, (nas_backend_code_t)code);

            break;
        }

        default:
            T_E("%u:%d: Unknown RADIUS type received: %d", DOT1X_NAS_PORT_2_ISID(nas_port), iport2uport(DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port)), code);
            break;
        }
    } else {
        // Treat all error messages as a backend server timeout.
        DOT1X_syslog_warning(VTSS_APPL_NAS_ERROR_BACKEND_SERVER_TIMEOUT);
        nas_backend_server_timeout_occurred(sm);
    }

do_exit:
    DOT1X_CRIT_EXIT();
}

/******************************************************************************/
// DOT1X_link_state_change_callback()
/******************************************************************************/
static void DOT1X_link_state_change_callback(mesa_port_no_t api_port, const vtss_appl_port_status_t *status)
{
    vtss_isid_t                  isid = VTSS_ISID_START;
    dot1x_switch_state_t         *switch_state;
    nas_port_t                   nas_port;
    vtss_appl_nas_port_control_t cur_state;

    nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);

    DOT1X_CRIT_ENTER();
    switch_state = &DOT1X_stack_state.switch_state[isid - VTSS_ISID_START];
    cur_state = nas_get_port_control(nas_port);

    if (status->link) {
        switch_state->port_state[api_port - VTSS_PORT_NO_START].flags |= DOT1X_PORT_STATE_FLAGS_LINK_UP;

        // Postpone the setting of the admin_state until the
        // INIT_CMD_ICFG_LOADING_POST event has been sent, if it's not already
        // called. This can happen if the port module comes before the dot1x
        // module in the array of modules being called back with events.
        if (switch_state->switch_exists) {
            // If the port is MAC-table based, the Port Security module already knows this and
            // may already have started adding MAC addresses on us. If so, the port may already be in link-up state,
            // since the on-mac-add function may have set it to that.
            vtss_appl_nas_port_control_t admin_state = DOT1X_stack_cfg.glbl_cfg.enabled ? DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port - VTSS_PORT_NO_START].admin_state : VTSS_APPL_NAS_PORT_CONTROL_DISABLED;
            if (cur_state != admin_state) {
                DOT1X_tx_port_state(isid, api_port, DOT1X_stack_cfg.glbl_cfg.enabled, admin_state);
                DOT1X_set_port_control(nas_port, admin_state, NAS_STOP_REASON_PORT_MODE_CHANGED);
            }
        }
    } else {
        // Link down
        switch_state->port_state[api_port - VTSS_PORT_NO_START].flags &= ~DOT1X_PORT_STATE_FLAGS_LINK_UP;
        // If it's MAC-table based and have at least one attached SM, do nothing, but wait for the Port Security module to
        // delete the entries. Otherwise set the port control to VTSS_APPL_NAS_PORT_CONTROL_DISABLED.
        if (NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(cur_state) == FALSE || nas_get_client_cnt(nas_port) == 0) {
            DOT1X_set_port_control(nas_port, VTSS_APPL_NAS_PORT_CONTROL_DISABLED, NAS_STOP_REASON_PORT_LINK_DOWN);
        }
    }

    DOT1X_CRIT_EXIT();
}

#ifdef NAS_USES_PSEC
/******************************************************************************/
// DOT1X_on_mac_add_callback()
/******************************************************************************/
static psec_add_method_t DOT1X_on_mac_add_callback(vtss_isid_t isid, mesa_port_no_t api_port, mesa_vid_mac_t *vid_mac, u32 mac_cnt_before_callback, vtss_appl_psec_user_t originating_user, psec_add_action_t *action)
{
    char                         buf[DOT1X_MAC_STR_BUF_SIZE];
    dot1x_switch_state_t         *switch_state;
    dot1x_port_state_t           *port_state;
    vtss_appl_nas_port_control_t admin_state, cur_state;
    nas_port_t                   nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);
    psec_add_method_t            result = PSEC_ADD_METHOD_KEEP_BLOCKED; // Whenever the MAC address is authenticated, the state will change. Do not age it until then.

    (void)DOT1X_vid_mac_to_str(api_port, vid_mac, buf);
    T_I("%s: Registering", buf);

    DOT1X_CRIT_ENTER();
    switch_state = &DOT1X_stack_state.switch_state[isid - VTSS_ISID_START];
    port_state   = &switch_state->port_state[api_port - VTSS_PORT_NO_START];
    admin_state  = DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port - VTSS_PORT_NO_START].admin_state;

    if (DOT1X_stack_cfg.glbl_cfg.enabled == 0) {
        T_D("Global mode is disabled. Allowing MAC %s", buf);
        result = PSEC_ADD_METHOD_FORWARD;
        goto do_exit;
    }

    if (!NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(admin_state)) {
        T_D("%d:%d: We're not MAC-table based. Allowing MAC %s", isid, iport2uport(api_port), buf);
        result = PSEC_ADD_METHOD_FORWARD;
        goto do_exit;
    }

    if (NAS_PORT_CONTROL_IS_BPDU_BASED(admin_state)) {
        // We're currently in Single or Multi 802.1X mode (MAC-table-
        // and BPDU-based). This means that all MAC addresses added
        // to the MAC table come through BPDUs, and not from other
        // modules.
        // In the event that another PSEC user module is in the
        // PSEC_PORT_MODE_KEEP_BLOCKED mode, that other module
        // may add MAC addresses to the PSEC module, thus causing
        // the PSEC module to callback all other modules - among
        // those us. Since we're in charge of what is going to
        // be added to the MAC table (802.1X takes precedence),
        // we must add it as blocked with a timeout.
        // When the entry times out, we can't find a corresponding
        // state machine and will print a warning. No problem.
        // At the time of writing no such other module exists.
        result = PSEC_ADD_METHOD_BLOCK;
        goto do_exit;
    }

    if (switch_state->switch_exists == FALSE || !(port_state->flags & DOT1X_PORT_STATE_FLAGS_LINK_UP)) {
        // This may happen if we get called before the
        // INIT_CMD_ICFG_LOADING_POST event or link-up events have propagated to
        // us.
        T_I("%d:%d: Artificially setting switch and link up", isid, iport2uport(api_port));
        switch_state->switch_exists  = TRUE;
        port_state->flags           |= DOT1X_PORT_STATE_FLAGS_LINK_UP;
        cur_state = nas_get_port_control(nas_port);
        if (cur_state != admin_state) {
            DOT1X_tx_port_state(isid, api_port, DOT1X_stack_cfg.glbl_cfg.enabled, admin_state);
            DOT1X_set_port_control(nas_port, admin_state, NAS_STOP_REASON_PORT_MODE_CHANGED);
        }
    }

    if (!nas_alloc_sm(nas_port, vid_mac)) {
        // This is a very rare event, but it may happen if all the SMs
        // are in use, which may happen if - say - one port is in a multi-client
        // mode and another is in a single-client mode. The total number of
        // SMs correspond to the number of times this function can be called,
        // so if one of the SMs is taken by a port in single-client mode,
        // nas_alloc_sm() may fail here.
        // If that happens, block the MAC address with a hold-timeout.
        // This will give one more T_E() error when the MAC address
        // times out in the DOT1X_on_mac_del_callback() function, because
        // we can't find the corresponding state machine.
        T_I("%d:%d: Can't allocate SM", isid, iport2uport(api_port));
#ifdef VTSS_SW_OPTION_SYSLOG
        S_PORT_W(isid, api_port, "NAS-CANNOT-ALLOCATE: on Interface %s vid:%u, mac:%02x:%02x:%02x:%02x:%02x:%02x.",
                 SYSLOG_PORT_INFO_REPLACE_KEYWORD,
                 vid_mac->vid,
                 vid_mac->mac.addr[0], vid_mac->mac.addr[1], vid_mac->mac.addr[2],
                 vid_mac->mac.addr[3], vid_mac->mac.addr[4], vid_mac->mac.addr[5]);
#endif
        result = PSEC_ADD_METHOD_BLOCK;
        goto do_exit;
    }

do_exit:
    DOT1X_CRIT_EXIT();
    return result;
}
#endif /* NAS_USES_PSEC */

#ifdef NAS_USES_PSEC
/******************************************************************************/
// DOT1X_psec_free_to_nas_stop_reason()
/******************************************************************************/
static DOT1X_INLINE nas_stop_reason_t DOT1X_psec_free_to_nas_stop_reason(psec_del_reason_t psec_free_reason)
{
    switch (psec_free_reason) {
    case PSEC_DEL_REASON_HW_ADD_FAILED:
        return NAS_STOP_REASON_MAC_TABLE_ERROR;
    case PSEC_DEL_REASON_SW_ADD_FAILED:
        return NAS_STOP_REASON_MAC_TABLE_ERROR;
    case PSEC_DEL_REASON_PORT_LINK_DOWN:
        return NAS_STOP_REASON_PORT_LINK_DOWN;
    case PSEC_DEL_REASON_STATION_MOVED:
        return NAS_STOP_REASON_STATION_MOVED;
    case PSEC_DEL_REASON_AGED_OUT:
        return NAS_STOP_REASON_AGED_OUT;
    case PSEC_DEL_REASON_HOLD_TIME_EXPIRED:
        return NAS_STOP_REASON_HOLD_TIME_EXPIRED;
    case PSEC_DEL_REASON_USER_DELETED:
        return NAS_STOP_REASON_PORT_MODE_CHANGED;
    case PSEC_DEL_REASON_PORT_SHUT_DOWN:
        return NAS_STOP_REASON_PORT_SHUT_DOWN;
    case PSEC_DEL_REASON_NO_MORE_USERS:
        return NAS_STOP_REASON_PORT_MODE_CHANGED;
    case PSEC_DEL_REASON_PORT_STP_MSTI_DISCARDING:
        return NAS_STOP_REASON_PORT_STP_MSTI_DISCARDING;
    default:
        T_E("Unknown PSEC delete reason (%d)", psec_free_reason);
        return NAS_STOP_REASON_UNKNOWN;
    }
}
#endif

#ifdef NAS_USES_PSEC
/******************************************************************************/
// DOT1X_on_mac_del_callback()
/******************************************************************************/
static void DOT1X_on_mac_del_callback(vtss_isid_t isid, mesa_port_no_t api_port, const mesa_vid_mac_t *vid_mac, psec_del_reason_t reason, psec_add_method_t add_method, vtss_appl_psec_user_t originating_user)
{
    char               buf[DOT1X_MAC_STR_BUF_SIZE];
    nas_sm_t           *sm;
    vtss_appl_nas_port_control_t cur_state;
    nas_port_t         nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);
    const char         *mac_str = misc_mac2str(vid_mac->mac.addr);
    vtss_uport_no_t    uport = iport2uport(api_port);

    (void)DOT1X_vid_mac_to_str(api_port, vid_mac, buf);
    T_I("%s: Unregistering", buf);

    DOT1X_CRIT_ENTER();

    cur_state = nas_get_port_control(nas_port);

    T_D("%d:%d: Unregistering MAC address (%s on %d) because: %s", isid, uport, mac_str, vid_mac->vid, psec_del_reason_to_str(reason));

    if (!NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(cur_state)) {
        T_D("%d:%d: Port is not in MAC-table-based mode. Cannot unregister %s", isid, uport, buf);
        goto do_exit;
    }

    sm = nas_get_sm_from_vid_mac_port(vid_mac, nas_port);

    if (!sm) {
        T_W("%d:%d: Don't know anything about this MAC address (%s on %d)", isid, uport, mac_str, vid_mac->vid);
        goto do_exit;
    }

    nas_free_sm(sm, DOT1X_psec_free_to_nas_stop_reason(reason));

    // If this was the last and the port link is in link-down, then change the port mode to disabled.
    if (nas_get_client_cnt(nas_port) == 0 && !(DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[api_port - VTSS_PORT_NO_START].flags & DOT1X_PORT_STATE_FLAGS_LINK_UP)) {
        DOT1X_set_port_control(nas_port, VTSS_APPL_NAS_PORT_CONTROL_DISABLED, NAS_STOP_REASON_PORT_MODE_CHANGED /* doesn't matter */);
    }

do_exit:
    DOT1X_CRIT_EXIT();
}
#endif /* NAS_USES_PSEC */

/******************************************************************************/
// DOT1X_fill_statistics()
// Fill in everything, but the admin_status and status fields.
// The status field differs depending on whether sm is a top of sub SM,
// and the admin_state requires api_port and isid, since we must take
// that from what the user has configured and not what the NAS base lib
// thinks.
/******************************************************************************/
static void DOT1X_fill_statistics(nas_sm_t *sm, vtss_nas_statistics_t *statistics)
{
    vtss_nas_eapol_counters_t *eapol_counters;
    vtss_appl_nas_backend_counters_t *backend_counters;
#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS) || defined(NAS_USES_VLAN)
    nas_port_info_t        *port_info = nas_get_port_info(sm);
#endif

    DOT1X_CRIT_ASSERT_LOCKED();

    // Get the EAPOL counters (if this mode supports it).
    if ((eapol_counters = nas_get_eapol_counters(sm)) != NULL) {
        statistics->eapol_counters = *eapol_counters;
    }

    // Get the backend counters (if this mode supports it).
    if ((backend_counters = nas_get_backend_counters(sm)) != NULL) {
        statistics->backend_counters = *backend_counters;
    }

    // Get the supplicant/client info (either last supplicant/client (if top-sm) or actual supplicant/client (if sub-sm)
    statistics->client_info = *nas_get_client_info(sm);

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
    // The QoS class that this port is assigned to by the backend server.
    // VTSS_PRIO_NO_NONE if unassigned.
    statistics->qos_class = port_info->qos_class;
#endif

#ifdef NAS_USES_VLAN
    // The VLAN ID of this port (either backend-assigned or Guest VLAN)
    // 0 if unassigned.
    statistics->vlan_type = port_info->vlan_type;
    statistics->vid       = port_info->current_vid;
#endif
}

/****************************************************************************/
// DOT1X_auth_fail_reason_to_str()
/****************************************************************************/
static const char *DOT1X_auth_fail_reason_to_str(nas_stop_reason_t reason)
{
    switch (reason) {
    case NAS_STOP_REASON_NONE:
        return "None";
    case NAS_STOP_REASON_UNKNOWN:
        return "Unknown";
    case NAS_STOP_REASON_INITIALIZING:
        return "(Re-)initializing";
    case NAS_STOP_REASON_AUTH_FAILURE:
        return "Backend server sent an Authentication Failure";
    case NAS_STOP_REASON_AUTH_NOT_CONFIGURED:
        return "Backend server not configured";
    case NAS_STOP_REASON_AUTH_TOO_MANY_ROUNDS:
        return "Too many authentication rounds";
    case NAS_STOP_REASON_AUTH_TIMEOUT:
        return "Backend server didn't reply";
    case NAS_STOP_REASON_EAPOL_START:
        return "Supplicant sent EAPOL start frame";
    case NAS_STOP_REASON_EAPOL_LOGOFF:
        return "Supplicant sent EAPOL logoff frame";
    case NAS_STOP_REASON_REAUTH_COUNT_EXCEEDED:
        return "Reauth count exceeded";
    case NAS_STOP_REASON_FORCED_UNAUTHORIZED:
        return "Port mode forced unauthorized";
    case NAS_STOP_REASON_MAC_TABLE_ERROR:
        return "Couldn't add MAC address to MAC Table";
    case NAS_STOP_REASON_PORT_LINK_DOWN:
        return "Port-link went down";
    case NAS_STOP_REASON_STATION_MOVED:
        return "The supplicant moved from one port to another";
    case NAS_STOP_REASON_AGED_OUT:
        return "The MAC address aged out";
    case NAS_STOP_REASON_HOLD_TIME_EXPIRED:
        return "The hold-time expired";
    case NAS_STOP_REASON_PORT_MODE_CHANGED:
        return "The port mode changed";
    case NAS_STOP_REASON_PORT_SHUT_DOWN:
        return "The port was shut down by the Port Security Limit Control Module";
    case NAS_STOP_REASON_SWITCH_REBOOT:
        return "Switch is about to reboot";
    case NAS_STOP_REASON_PORT_STP_MSTI_DISCARDING:
        return "The port STP MSTI state is discarding";
    default:
        return "Unknown stop reason";
    }
}

/******************************************************************************/
//
// IMPLEMENTATION OF NAS CORE LIBRARY OS-DEPENDENT FUNCTIONS.
// THE API HEADER FILE FOR THIS IS LOCATED IN sw_nas/nas_os.h (core/nas_os.h)
//
/******************************************************************************/

/****************************************************************************/
// nas_os_freeing_sm()
/****************************************************************************/
void nas_os_freeing_sm(nas_sm_t *sm)
{
#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS) || defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN)
    nas_port_info_t *port_info = nas_get_port_info(sm);
    if (NAS_PORT_CONTROL_IS_SINGLE_CLIENT(port_info->port_control) && port_info->cur_client_cnt == 1) {
        // The last client on this single-client port.

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
        DOT1X_qos_set(port_info->port_no, VTSS_PRIO_NO_NONE);
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
        DOT1X_vlan_set(port_info->port_no, 0, VTSS_APPL_NAS_VLAN_TYPE_NONE);
#endif
    }
#endif
}

/****************************************************************************/
// nas_os_ieee8021x_send_eapol_frame()
// Send a BPDU on external port number
/****************************************************************************/
void nas_os_ieee8021x_send_eapol_frame(nas_port_t nas_port, u8 *frame, size_t len)
{
    DOT1X_msg_tx_eapol(DOT1X_NAS_PORT_2_ISID(nas_port), DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port), DOT1X_MSG_ID_PRI_TO_SEC_EAPOL, 1 /* Doesn't matter (unused) */, frame, len);
}

/****************************************************************************/
// nas_os_get_port_mac()
/****************************************************************************/
void nas_os_get_port_mac(nas_port_t nas_port, u8 *frame)
{
    // Convert from NAS core library port number to Layer 2 port number before
    // calling vtss_os_get_portmac(), which expects an L2 port number.
    mesa_port_no_t api_port = DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port);

    DOT1X_CRIT_ASSERT_LOCKED();

    vtss_os_get_portmac(api_port, (vtss_common_macaddr_t *)frame);
}

/****************************************************************************/
// nas_os_set_authorized()
// if @sm is port-based, then this function causes a message to be sent to
// the relevant switch where its port state is changed.
// If @sm is mac-based, then this function causes a MAC address to be added
// or removed from the MAC table using the MAC module.
// If @chgd, then the previous call to this function had @authorized set to its
// opposite value.
/****************************************************************************/
void nas_os_set_authorized(struct nas_sm *sm, BOOL authorized, BOOL chgd)
{
    nas_port_t             nas_port     = nas_get_port_info(sm)->port_no;
    vtss_isid_t            isid         = DOT1X_NAS_PORT_2_ISID(nas_port);
    mesa_port_no_t         api_port     = DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port);
    vtss_uport_no_t        uport        = iport2uport(api_port);
    nas_eap_info_t         *eap_info    = nas_get_eap_info(sm);
    vtss_nas_client_info_t *client_info = nas_get_client_info(sm);
    nas_port_info_t        *port_info   = nas_get_port_info(sm);
#ifdef NAS_USES_PSEC
    mesa_rc                rc           = VTSS_RC_OK;
    BOOL                   psec_chg     = FALSE;
    psec_add_method_t      chg_method   = PSEC_ADD_METHOD_BLOCK; // Initialize it to keep the compiler happy
#endif
    vtss_appl_nas_port_control_t admin_state;
    char               mac_or_port_str[30];

    // The critical section must be taken by now, since this must be called back from the base lib as a response
    // to this module calling the base lib.
    DOT1X_CRIT_ASSERT_LOCKED();

    admin_state = port_info->port_control;

    // Update the time of (successful/unsuccessful) authentication.
    client_info->rel_auth_time_secs = nas_os_get_uptime_secs();

#ifdef NAS_USES_PSEC
    if (NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(admin_state)) {
        // Show MAC address in subsequent T_x output.
        (void)snprintf(mac_or_port_str, sizeof(mac_or_port_str) - 1, "%s:%d", client_info->mac_addr_str, client_info->vid_mac.vid);
    } else
#endif
    {
        // Show "port" in subsequent T_x output.
        strcpy(mac_or_port_str, "port");
    }

    T_D("%d:%d: Setting %s to %s (reason = %s)", isid, uport, mac_or_port_str, authorized ? "Auth" : "Unauth", authorized ? "Success" : DOT1X_auth_fail_reason_to_str(eap_info->stop_reason));

    if (!DOT1X_stack_cfg.glbl_cfg.enabled) {
        T_E("%d:%d: 802.1X not enabled", isid, uport);
    }

    if (authorized && admin_state == VTSS_APPL_NAS_PORT_CONTROL_FORCE_UNAUTHORIZED) {
        T_E("%d:%d: Base lib told to authorize, but the admin state indicates FU", isid, uport);
    }
    if (!authorized && admin_state == VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED) {
        T_E("%d:%d: Base lib told to unauthorize, but the admin state indicates FA", isid, uport);
    }
    if (authorized && !(DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[api_port - VTSS_PORT_NO_START].flags & DOT1X_PORT_STATE_FLAGS_LINK_UP)) {
        T_E("%d:%d: Base lib told us authorize %s, but the cached link state says Down", isid, uport, mac_or_port_str);
    }

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
    if (NAS_PORT_CONTROL_IS_SINGLE_CLIENT(admin_state) && !authorized) {
        DOT1X_qos_set(nas_port, VTSS_PRIO_NO_NONE);
    }
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    if (port_info->vlan_type == VTSS_APPL_NAS_VLAN_TYPE_GUEST_VLAN) {
        // Nothing more to do here.
        return;
    }
    if (eap_info->stop_reason == NAS_STOP_REASON_REAUTH_COUNT_EXCEEDED && NAS_PORT_CONTROL_IS_BPDU_BASED(admin_state)) {
        if (DOT1X_guest_vlan_enter_check(isid, api_port, port_info)) {
            // Just entered guest VLAN. Nothing more to do.
            return;
        }
    }
#endif

#ifdef NAS_USES_PSEC
    if (NAS_PORT_CONTROL_IS_MAC_TABLE_AND_BPDU_BASED(admin_state)) {
        // This is a Single- or Multi- 802.1X SM.
        if (client_info->vid_mac.vid == 0) {
            // Indicates that this has not yet been assigned. Nothing to do, since we're waiting for the first client to attach.
            T_D("%d:%d: Doing nothing. Awaiting a client to attach", isid, uport);
        } else if (!authorized && (eap_info->stop_reason == NAS_STOP_REASON_INITIALIZING || eap_info->stop_reason == NAS_STOP_REASON_EAPOL_START)) {
            // Client is attached and either the network administrator is forcing a "reauthentication now" or the client has sent an EAPOL Start frame.
            // Either way, we need to keep the MAC address blocked, so that it doesn't age out while authenticating.
            psec_chg   = TRUE;
            chg_method = PSEC_ADD_METHOD_KEEP_BLOCKED;
        } else if (!authorized && eap_info->stop_reason == NAS_STOP_REASON_EAPOL_LOGOFF) {
            // Client sent an EAPOL Logoff frame. Gotta remove the SM and the entry from the MAC table.
            // The VLAN will be reverted once its actually deleted.
            rc = psec_mgmt_mac_del(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &client_info->vid_mac);
            eap_info->delete_me = TRUE;
            T_D("%d:%d(%s): Deleting MAC and asking baselib to delete SM (rc=%s)", isid, uport, mac_or_port_str, error_txt(rc));
        } else {
            // If any other reason, we add it with either forwarding or blocked with hold timeout.
            psec_chg = TRUE;
            chg_method = authorized ? PSEC_ADD_METHOD_FORWARD : PSEC_ADD_METHOD_BLOCK;
        }
        if (psec_chg) {
            // It really doesn't matter if this function is also called for multi-802.1X even though
            // backend-assigned VLAN is only supported for single- and port-based 802.1X.
            DOT1X_psec_chg(nas_port, isid, api_port, port_info, eap_info, client_info, chg_method);
        }
    } else if (NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(admin_state)) {
        // Only restart aging if we're going from unauthorized to authorized. If we didn't have this check, and
        // reauthentication is enabled and the reauthentication period is shorter than the age time,
        // then the age timer will never expire.
        if (chgd || !authorized) {
            char buf[DOT1X_MAC_STR_BUF_SIZE];

            // Age and hold timers are updated automatically when changing the entry.
            T_I("%s: Changing to %s", DOT1X_vid_mac_to_str(api_port, &client_info->vid_mac, buf), authorized ? "Forward" : "Block");
            if ((rc = psec_mgmt_mac_chg(VTSS_APPL_PSEC_USER_DOT1X, isid, api_port, &client_info->vid_mac, authorized ? PSEC_ADD_METHOD_FORWARD : PSEC_ADD_METHOD_BLOCK)) != VTSS_RC_OK) {
                T_E("%d:%d: psec_mgmt_mac_chg(): %s", isid, uport, error_txt(rc));
            }
        }
    } else
#endif
    {
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
        if (authorized) {
            DOT1X_vlan_set(nas_port, port_info->backend_assigned_vid, port_info->backend_assigned_vid ? VTSS_APPL_NAS_VLAN_TYPE_BACKEND_ASSIGNED : VTSS_APPL_NAS_VLAN_TYPE_NONE);
        } else {
            DOT1X_vlan_set(nas_port, 0, VTSS_APPL_NAS_VLAN_TYPE_NONE);
        }
#endif
        if (chgd) {
            DOT1X_msg_tx_port_state(isid, api_port, authorized);
        }
    }

#ifdef VTSS_SW_OPTION_DOT1X_ACCT
    dot1x_acct_authorized_changed(admin_state, sm, authorized);
#endif
}

/******************************************************************************/
// nas_os_backend_server_free_resources()
/******************************************************************************/
void nas_os_backend_server_free_resources(nas_sm_t *sm)
{
    nas_eap_info_t *eap_info = nas_get_eap_info(sm);

    DOT1X_CRIT_ASSERT_LOCKED();

    eap_info->radius_server_id = 0;

    if (eap_info->radius_handle != -1) {
        (void)vtss_radius_free(eap_info->radius_handle);
        eap_info->radius_handle = -1;
    }
}

/****************************************************************************/
// nas_os_backend_server_tx_request()
/****************************************************************************/
BOOL nas_os_backend_server_tx_request(nas_sm_t *sm, int *old_handle, u8 *eap, u16 eap_len, u8 *state, u8 state_len, char *user, nas_port_t nas_port, u8 *mac_addr)
{
    mesa_rc               res = VTSS_RC_OK;
    u8                    radius_handle;
    char                  temp_str[20];
    vtss_common_macaddr_t sys_mac;
    nas_eap_info_t        *eap_info = nas_get_eap_info(sm);
    vtss_isid_t           isid = DOT1X_NAS_PORT_2_ISID(nas_port);
    vtss_uport_no_t       uport = iport2uport(DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port));
    u32                   radius_server_id;

    T_D("RADIUS Server Request for user = %s with radius_server_id = %u", user, eap_info->radius_server_id);

    if (!eap_info) {
        T_E("%d:%d: Called from non-backend-server-aware SM", isid, uport);
        return FALSE;
    }

    DOT1X_CRIT_ASSERT_LOCKED();

    if (*old_handle != -1) {
        T_E("%d:%d: The previous RADIUS handle (%d) was not freed", isid, uport, *old_handle);
        // Better free it.
        (void)vtss_radius_free(*old_handle);
        *old_handle = -1;
    }

    if (!vtss_radius_auth_ready()) {
        // The RADIUS server is not ready to accept requests.
        res = VTSS_APPL_NAS_ERROR_BACKEND_SERVERS_NOT_READY;
    } else if ((res = vtss_radius_alloc(&radius_handle, VTSS_RADIUS_CODE_ACCESS_REQUEST)) != VTSS_RC_OK) {
        // RADIUS server is ready, but returned another error.
        if (res == VTSS_RADIUS_ERROR_NOT_INITIALIZED || res == VTSS_RADIUS_ERROR_NOT_CONFIGURED || res == VTSS_RADIUS_ERROR_OUT_OF_HANDLES) {
            // "Not Initialized", "Not Configured", and "Out of handles" are not real coding errors.
            T_D("%d:%d: Got \"%s\" from vtss_radius_alloc()", isid, uport, error_txt(res));
        } else {
            T_E("%d:%d: Got \"%s\" from vtss_radius_alloc()", isid, uport, error_txt(res));
        }
    }

    if (res != VTSS_RC_OK) {
        DOT1X_syslog_warning(res);
        return FALSE;
    }

    // Add the required attributes
    if (((res = vtss_radius_tlv_set(radius_handle, VTSS_RADIUS_ATTRIBUTE_EAP_MESSAGE, eap_len,      eap,        TRUE)) != VTSS_RC_OK) ||
        ((res = vtss_radius_tlv_set(radius_handle, VTSS_RADIUS_ATTRIBUTE_STATE,       state_len,    state,      TRUE)) != VTSS_RC_OK) ||
        ((res = vtss_radius_tlv_set(radius_handle, VTSS_RADIUS_ATTRIBUTE_USER_NAME,   strlen(user), (u8 *)user, TRUE)) != VTSS_RC_OK)) {
        T_E("%d:%d: Got \"%s\" from vtss_tlv_set() on required attribute", isid, uport, error_txt(res));
        return FALSE;
    }

    // Add the optional attributes.
#define DOT1X_ADD_OPTIONAL_RADIUS_ATTRIB(t, l, v)                                                        \
    res = vtss_radius_tlv_set(radius_handle, t, l, v, FALSE);                                            \
    if (res != VTSS_RC_OK && res != VTSS_RADIUS_ERROR_NOT_ROOM_FOR_TLV) {                                \
        T_E("%d:%d: Got \"%s\" from vtss_tlv_set() on optional attribute", isid, uport, error_txt(res)); \
        return FALSE;                                                                                    \
    }

    // NAS-Service-Type. Use different service type depending on whether it's
    // MAC-based or BPDU-based.
    temp_str[0] = temp_str[1] = temp_str[2] = 0;
    if (nas_is_sm_mac_based(sm)) {
        temp_str[3] = VTSS_RADIUS_NAS_SERVICE_TYPE_CALL_CHECK;
    } else {
        temp_str[3] = VTSS_RADIUS_NAS_SERVICE_TYPE_FRAMED_USER;
    }

    DOT1X_ADD_OPTIONAL_RADIUS_ATTRIB(VTSS_RADIUS_ATTRIBUTE_SERVICE_TYPE, 4, (u8 *)temp_str);

    // NAS-Port-Type
    temp_str[0] = temp_str[1] = temp_str[2] = 0;
    temp_str[3] = VTSS_RADIUS_NAS_PORT_TYPE_ETHERNET;
    DOT1X_ADD_OPTIONAL_RADIUS_ATTRIB(VTSS_RADIUS_ATTRIBUTE_NAS_PORT_TYPE, 4, (u8 *)temp_str);

    // NAS-Port-Id
    temp_str[0] = temp_str[1] = temp_str[2] = 0;
    temp_str[3] = nas_port;
    DOT1X_ADD_OPTIONAL_RADIUS_ATTRIB(VTSS_RADIUS_ATTRIBUTE_NAS_PORT, 4, (u8 *)temp_str);

    (void)snprintf(temp_str, sizeof(temp_str) - 1, "Port %u", nas_port);
    DOT1X_ADD_OPTIONAL_RADIUS_ATTRIB(VTSS_RADIUS_ATTRIBUTE_NAS_PORT_ID, strlen(temp_str), (u8 *)temp_str);

    // Calling-Station-Id
    nas_os_mac2str(mac_addr, temp_str);
    DOT1X_ADD_OPTIONAL_RADIUS_ATTRIB(VTSS_RADIUS_ATTRIBUTE_CALLING_STATION_ID, strlen(temp_str), (u8 *)temp_str);

    // Called-Station-Id
    vtss_os_get_systemmac(&sys_mac);
    nas_os_mac2str(sys_mac.macaddr, temp_str);
    DOT1X_ADD_OPTIONAL_RADIUS_ATTRIB(VTSS_RADIUS_ATTRIBUTE_CALLED_STATION_ID, strlen(temp_str), (u8 *)temp_str);

#undef DOT1X_ADD_OPTIONAL_RADIUS_ATTRIB

#ifdef VTSS_SW_OPTION_DOT1X_ACCT
    if (!dot1x_acct_append_radius_tlv(radius_handle, eap_info)) {
        return FALSE;
    }
#endif

    // Transmit the RADIUS frame and ask to be called back whenever a response arrives.
    // The RADIUS module takes care of retransmitting, changing server, etc.
    // We use the nas_port as context, rather than the state machine itself, because the
    // state machine may have been used for other purposes once we get called back. Furthermore,
    // if we had used dynamic memory allocation for statemachines (we don't), then the ctx
    // may have been out-of-date (freed) once called back.
    // The combination of port number and RADIUS identifier should uniquely identify the SM.
    radius_server_id = eap_info->radius_server_id;
    eap_info->radius_server_id = 0;
    ctx_t ctx;
    ctx.u = nas_port;
    T_D("Sending RADIUS request");
    if ((res = vtss_radius_tx(radius_handle, ctx, DOT1X_radius_rx_callback, radius_server_id)) != VTSS_RC_OK) {
        T_E("%d:%d: vtss_radius_tx() returned \"%s\"", isid, uport, error_txt(res));
        return FALSE;
    }

    eap_info->radius_handle = radius_handle;

    return TRUE;
}

/****************************************************************************/
// nas_os_mac2str()
// For MAC-based authentication, this is also used to generate the
// supplicant ID from the MAC address. Therefore the separator cannot be
// a colon, since IAS doesn't support colons in its user name.
/****************************************************************************/
void nas_os_mac2str(u8 *mac, char *str)
{
    sprintf(str, "%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/****************************************************************************/
// nas_os_get_reauth_timer()
/****************************************************************************/
u16 nas_os_get_reauth_timer(void)
{
    DOT1X_CRIT_ASSERT_LOCKED();
    return DOT1X_stack_cfg.glbl_cfg.reauth_period_secs;
}

/****************************************************************************/
// nas_os_get_reauth_enabled()
/****************************************************************************/
BOOL nas_os_get_reauth_enabled(void)
{
    DOT1X_CRIT_ASSERT_LOCKED();
    return DOT1X_stack_cfg.glbl_cfg.reauth_enabled;
}

/****************************************************************************/
// nas_os_get_reauth_max()
/****************************************************************************/
vtss_appl_nas_counter_t nas_os_get_reauth_max(void)
{
    DOT1X_CRIT_ASSERT_LOCKED();
#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    return DOT1X_stack_cfg.glbl_cfg.reauth_max;
#else
    return 2;
#endif
}

/****************************************************************************/
// nas_os_get_eapol_challenge_timeout()
// All EAPOL request packets (except Request Identity) sent to the supplicant
// are subject to retransmission after this timeout.
// Request Identity Timeout is returned with
// nas_os_get_eapol_request_identity_timeout().
/****************************************************************************/
u16 nas_os_get_eapol_challenge_timeout(void)
{
    DOT1X_CRIT_ASSERT_LOCKED();
    return SUPPLICANT_TIMEOUT;
}

/****************************************************************************/
// nas_os_get_eapol_request_identity_timeout()
// The timeout between retransmission of Request Identity EAPOL frames to
// the supplicant.
// Timeout of other requests to the supplicant is returned with
// nas_os_get_eapol_challenge_timeout().
/****************************************************************************/
u16 nas_os_get_eapol_request_identity_timeout(void)
{
    DOT1X_CRIT_ASSERT_LOCKED();
    return  DOT1X_stack_cfg.glbl_cfg.eapol_timeout_secs;
}

/****************************************************************************/
// nas_os_get_uptime_secs()
/****************************************************************************/
time_t nas_os_get_uptime_secs(void)
{
    DOT1X_CRIT_ASSERT_LOCKED();
    return msg_uptime_get(VTSS_ISID_LOCAL);
}

/******************************************************************************/
// nas_os_init()
/******************************************************************************/
void nas_os_init(nas_port_info_t *port_info)
{
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
    port_info->qos_class = VTSS_PRIO_NO_NONE;
#endif

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN
    port_info->backend_assigned_vid = 0;
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    port_info->eapol_frame_seen = FALSE;
#endif

#ifdef NAS_USES_VLAN
    port_info->current_vid = 0;
    port_info->vlan_type   = VTSS_APPL_NAS_VLAN_TYPE_NONE;
#endif
}

/******************************************************************************/
//
// SEMI-PUBLIC FUNCTIONS
//
/******************************************************************************/

/******************************************************************************/
// dot1x_crit_enter()
/******************************************************************************/
void dot1x_crit_enter(void)
{
    // Avoid Lint warning: A thread mutex has been locked but not unlocked
    /*lint --e{454} */
    DOT1X_CRIT_ENTER();
}

/******************************************************************************/
// dot1x_crit_exit()
/******************************************************************************/
void dot1x_crit_exit(void)
{
    // Avoid Lint warning: A thread mutex that had not been locked is being unlocked
    /*lint -e(455) */
    DOT1X_CRIT_EXIT();
}

/******************************************************************************/
// dot1x_crit_assert_locked()
/******************************************************************************/
void dot1x_crit_assert_locked(void)
{
    DOT1X_CRIT_ASSERT_LOCKED();
}

/****************************************************************************/
// dot1x_disable_due_to_soon_boot()
/****************************************************************************/
void dot1x_disable_due_to_soon_boot(void)
{
    dot1x_stack_cfg_t artificial_cfg;
    DOT1X_CRIT_ENTER();
    if (DOT1X_stack_cfg.glbl_cfg.enabled) {
        artificial_cfg = DOT1X_stack_cfg;
        artificial_cfg.glbl_cfg.enabled = FALSE;
        (void)DOT1X_apply_cfg(VTSS_ISID_GLOBAL, &artificial_cfg, FALSE, NAS_STOP_REASON_SWITCH_REBOOT);
        T_D("Disabling 802.1X");
    }
    DOT1X_CRIT_EXIT();
}

/****************************************************************************/
// dot1x_glbl_enabled(void)
/****************************************************************************/
BOOL dot1x_glbl_enabled(void)
{
    DOT1X_CRIT_ASSERT_LOCKED();
    return DOT1X_stack_cfg.glbl_cfg.enabled;
}

/******************************************************************************/
// dot1x_port_control_to_str()
// Transforms a vtss_appl_nas_port_control_t to string
// IN : Brief - TRUE to return text in a brief format in order to show all status within 80 characters
/******************************************************************************/
const char *dot1x_port_control_to_str(vtss_appl_nas_port_control_t port_control, BOOL brief)
{
    switch (port_control) {
    case VTSS_APPL_NAS_PORT_CONTROL_DISABLED:
        return brief ? "Dis"   : "NAS Disabled";
    case VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED:
        return brief ? "Auth"  : "Force Authorized";
    case VTSS_APPL_NAS_PORT_CONTROL_AUTO:
        return brief ? "Port"  : "Port-based 802.1X";
    case VTSS_APPL_NAS_PORT_CONTROL_FORCE_UNAUTHORIZED:
        return brief ? "UnAut" : "Force Unauthorized";
#ifdef VTSS_SW_OPTION_NAS_DOT1X_SINGLE
    case VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE:
        return brief ? "Sigle" : "Single 802.1X";
#endif
#ifdef VTSS_SW_OPTION_NAS_DOT1X_MULTI
    case VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI:
        return brief ? "Multi" : "Multi 802.1X";
#endif
#ifdef VTSS_SW_OPTION_NAS_MAC_BASED
    case VTSS_APPL_NAS_PORT_CONTROL_MAC_BASED:
        return brief ? "MAC"   : "MAC-Based Auth";
#endif
    default:
        return "Unknown";
    }
}

/******************************************************************************/
//
// PUBLIC FUNCTIONS
//
/******************************************************************************/
/******************************************************************************/
// vtss_appl_nas_glbl_cfg_get()
/******************************************************************************/
mesa_rc vtss_appl_nas_glbl_cfg_get(vtss_appl_glbl_cfg_t *const glbl_cfg)
{
    mesa_rc rc;

    if (!glbl_cfg) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    if ((rc = DOT1X_isid_port_check(VTSS_ISID_START, VTSS_PORT_NO_START, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    // Use the stack config.
    DOT1X_CRIT_ENTER();
    *glbl_cfg = DOT1X_stack_cfg.glbl_cfg;
    DOT1X_CRIT_EXIT();
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_nas_glbl_cfg_set()
/******************************************************************************/
mesa_rc vtss_appl_nas_glbl_cfg_set(const vtss_appl_glbl_cfg_t *const cfg)
{
    mesa_rc           rc;
    dot1x_stack_cfg_t tmp_stack_cfg;
    vtss_appl_glbl_cfg_t *glbl_cfg = (vtss_appl_glbl_cfg_t *)cfg;

    if (!glbl_cfg) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    if ((rc = DOT1X_isid_port_check(VTSS_ISID_START, VTSS_PORT_NO_START, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = DOT1X_cfg_valid_glbl(glbl_cfg)) != VTSS_RC_OK) {
        return rc;
    }

    DOT1X_CRIT_ENTER();

    // We need to create a new structure with the current config
    // and only replace the glbl_cfg member.
    tmp_stack_cfg          = DOT1X_stack_cfg;
    tmp_stack_cfg.glbl_cfg = *glbl_cfg;

    // Apply the configuration. The function will check differences between old and new config
    if ((rc = DOT1X_apply_cfg(VTSS_ISID_GLOBAL, &tmp_stack_cfg, FALSE, NAS_STOP_REASON_PORT_MODE_CHANGED)) == VTSS_RC_OK) {
        // Copy the user's configuration to our configuration
        DOT1X_stack_cfg.glbl_cfg = *glbl_cfg;
    } else {
        // Roll back to previous settings without checking the return code
        (void)DOT1X_apply_cfg(VTSS_ISID_GLOBAL, &DOT1X_stack_cfg, FALSE, NAS_STOP_REASON_PORT_MODE_CHANGED);
    }

    DOT1X_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_nas_port_cfg_get(vtss_ifindex_t ifindex, vtss_appl_nas_port_cfg_t *const port_cfg)
{
    vtss_nas_switch_cfg_t switch_cfg;
    mesa_rc                    rc = VTSS_RC_ERROR;

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    if ((rc = vtss_nas_switch_cfg_get(ife.usid, &switch_cfg)) == VTSS_RC_OK) {
        *port_cfg = switch_cfg.port_cfg[ife.ordinal];
    }
    return rc;
}
/******************************************************************************/
// vtss_nas_switch_cfg_get()
/******************************************************************************/
mesa_rc vtss_nas_switch_cfg_get(vtss_usid_t usid, vtss_nas_switch_cfg_t *switch_cfg)
{
    mesa_rc rc;

    vtss_isid_t isid = topo_usid2isid(usid);

    if (!switch_cfg) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    if ((rc = DOT1X_isid_port_check(isid, VTSS_PORT_NO_START, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    DOT1X_CRIT_ENTER();
    *switch_cfg = DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START];
    DOT1X_CRIT_EXIT();
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_nas_port_cfg_set(vtss_ifindex_t ifindex, const vtss_appl_nas_port_cfg_t *const port_cfg)
{
    vtss_nas_switch_cfg_t switch_cfg;
    mesa_rc                    rc = VTSS_RC_ERROR;

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    if ((rc = vtss_nas_switch_cfg_get(ife.usid, &switch_cfg)) == VTSS_RC_OK) {
        switch_cfg.port_cfg[ife.ordinal] = *port_cfg;
        rc = vtss_nas_switch_cfg_set(ife.usid, &switch_cfg);
    }
    return rc;
}

/******************************************************************************/
// vtss_nas_switch_cfg_set()
/******************************************************************************/
mesa_rc vtss_nas_switch_cfg_set(vtss_usid_t usid, vtss_nas_switch_cfg_t *switch_cfg)
{
    mesa_rc           rc;
    dot1x_stack_cfg_t tmp_stack_cfg;
    port_iter_t       pit;

    if (!switch_cfg) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    vtss_isid_t isid = topo_usid2isid(usid);

    if ((rc = DOT1X_isid_port_check(isid, VTSS_PORT_NO_START, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = DOT1X_cfg_valid_switch(switch_cfg)) != VTSS_RC_OK) {
        return rc;
    }

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (switch_cfg->port_cfg[pit.iport - VTSS_PORT_NO_START].admin_state != VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED) {
            // Inter-protocol check #1.
            // If aggregation is enabled (LACP or static), then we cannot set a given
            // port to anything but Force Authorized. This is independent of the value
            // of 802.1X globally enabled.
#ifdef VTSS_SW_OPTION_AGGR
            switch (aggr_mgmt_port_participation(isid, pit.iport)) { // This function always exists independent of VTSS_SW_OPTION_LACP
            case 1:
                return VTSS_APPL_NAS_ERROR_STATIC_AGGR_ENABLED;

            case 2:
                return VTSS_APPL_NAS_ERROR_DYNAMIC_AGGR_ENABLED;

            default:
                break;
            }
#endif /* VTSS_SW_OPTION_AGGR */

            // Inter-protocol check #2.
            // If spanning tree is enabled, then we cannot set a given port
            // to anything but Force Authorized. This is independent of the value
            // of 802.1X globally enabled.
#ifdef VTSS_SW_OPTION_MSTP
            {
                mstp_port_param_t rstp_conf;
                BOOL              stp_enabled = FALSE;
                if ((rc = vtss_mstp_port_config_get(isid, pit.iport, &stp_enabled, &rstp_conf)) != VTSS_RC_OK) {
                    return rc;
                }
                if (stp_enabled) {
                    return VTSS_APPL_NAS_ERROR_STP_ENABLED;
                }
            }
#endif /* VTSS_SW_OPTION_MSTP */
        }
    }

    DOT1X_CRIT_ENTER();

    // We need to create a new structure with the current config
    // and only replace this switch's member.
    tmp_stack_cfg = DOT1X_stack_cfg;
    tmp_stack_cfg.switch_cfg[isid - VTSS_ISID_START] = *switch_cfg;

    // Apply the configuration. The function will check differences between old and new config
    if ((rc = DOT1X_apply_cfg(isid, &tmp_stack_cfg, TRUE, NAS_STOP_REASON_PORT_MODE_CHANGED)) == VTSS_RC_OK) {
        // Copy the user's configuration to our configuration
        DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START] = *switch_cfg;
    } else {
        // Roll back to previous settings without checking the return code
        (void)DOT1X_apply_cfg(isid, &DOT1X_stack_cfg, TRUE, NAS_STOP_REASON_PORT_MODE_CHANGED);
    }

    DOT1X_CRIT_EXIT();
    return rc;
}

/******************************************************************************/
// vtss_appl_nas_switch_status_get()
/******************************************************************************/
mesa_rc vtss_nas_switch_status_get(vtss_usid_t usid, vtss_nas_switch_status_t *switch_status)
{
    port_iter_t pit;
    mesa_rc     rc;

    vtss_isid_t isid = topo_usid2isid(usid);

    if (!switch_status) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    if ((rc = DOT1X_isid_port_check(isid, VTSS_PORT_NO_START, FALSE, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    DOT1X_CRIT_ENTER();

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        switch_status->admin_state[pit.iport] = DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[pit.iport].admin_state;
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
        switch_status->qos_class[pit.iport] = VTSS_PRIO_NO_NONE;
#endif
#ifdef NAS_USES_VLAN
        switch_status->vlan_type[pit.iport] = VTSS_APPL_NAS_VLAN_TYPE_NONE;
        switch_status->vid[pit.iport]       = 0;
#endif

        if (DOT1X_stack_cfg.glbl_cfg.enabled) {
            if (DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].switch_exists && (DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[pit.iport].flags & DOT1X_PORT_STATE_FLAGS_LINK_UP)) {
                u32 auth_cnt, unauth_cnt;
                nas_port_t nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, pit.iport + VTSS_PORT_NO_START);
#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS) || defined(NAS_USES_VLAN)
                nas_port_info_t *port_info = nas_get_port_info(nas_get_top_sm(nas_port));
#endif

                // Link is up. Check for authorized/Non authorized state (this may not be correct, because we may
                // just have got link up, but the core lib may not have been updated on that).
                switch_status->status[pit.iport]     = nas_get_port_status(nas_port, &auth_cnt, &unauth_cnt);
                switch_status->auth_cnt[pit.iport]   = auth_cnt;
                switch_status->unauth_cnt[pit.iport] = unauth_cnt;
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
                switch_status->qos_class[pit.iport] = port_info->qos_class;
#endif
#ifdef NAS_USES_VLAN
                switch_status->vlan_type[pit.iport] = port_info->vlan_type;
                switch_status->vid[pit.iport]       = port_info->current_vid;
#endif
            } else {
                // The port has link down
                switch_status->status[pit.iport] = VTSS_APPL_NAS_PORT_STATUS_LINK_DOWN;
            }
        } else {
            // 802.1X is globally disabled
            switch_status->status[pit.iport] = VTSS_APPL_NAS_PORT_STATUS_DISABLED;
        }
    }

    DOT1X_CRIT_EXIT();
    return VTSS_RC_OK;
}

#ifdef NAS_MULTI_CLIENT
/******************************************************************************/
// vtss_nas_multi_client_status_get()
/******************************************************************************/
mesa_rc vtss_nas_multi_client_status_get(vtss_ifindex_t ifindex, vtss_nas_multi_client_status_t *client_status, BOOL *found, BOOL start_all_over)
{
    nas_port_t         nas_port;
    mesa_rc            rc;
    nas_sm_t           *sm;
    vtss_nas_client_info_t  *client_info = NULL;
    vtss_appl_nas_port_control_t admin_state;

    if (!found || !client_status) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    *found = FALSE;

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    mesa_port_no_t api_port = ife.ordinal;
    vtss_isid_t    isid     = ife.isid;

    if ((rc = DOT1X_isid_port_check(isid, api_port, FALSE, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);

    DOT1X_CRIT_ENTER();

    admin_state = DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port - VTSS_PORT_NO_START].admin_state;
    if (DOT1X_stack_cfg.glbl_cfg.enabled == FALSE || !NAS_PORT_CONTROL_IS_MULTI_CLIENT(admin_state)) {
        rc = VTSS_RC_OK; // Not available. This is not an error, so just return.
        goto do_exit;
    }

    sm = nas_get_top_sm(nas_port);
    for (sm = nas_get_next(sm); sm; sm = nas_get_next(sm)) {
        if (start_all_over) {
            *found = TRUE;
            break;
        }

        // start_all_over is FALSE, so we've been called before.
        client_info = nas_get_client_info(sm);
        if (memcmp(&client_status->client_info.vid_mac, &client_info->vid_mac, sizeof(client_status->client_info.vid_mac)) == 0) {
            nas_sm_t *temp_sm = nas_get_next(sm);
            *found = temp_sm != NULL;
            sm = temp_sm;
            break;
        }
    }

    if (*found) {
        client_status->client_info = *nas_get_client_info(sm);
        client_status->status      = nas_get_status(sm);
    }

    rc = VTSS_RC_OK;

do_exit:
    DOT1X_CRIT_EXIT();
    return rc;
}
#endif

mesa_rc vtss_appl_nas_port_status_get(vtss_ifindex_t ifindex, vtss_appl_nas_interface_status_t *const status)
{
    vtss_nas_statistics_t stati;

    if (!status) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    memset(status, 0, sizeof(vtss_appl_nas_interface_status_t));
    VTSS_RC(vtss_nas_statistics_get(ifindex, NULL, &stati));
    status->qos_class = stati.qos_class;
    status->vid       = stati.vid;
    status->status    = stati.status;
    status->vlan_type = stati.vlan_type;

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_nas_eapol_statistics_get(vtss_ifindex_t ifindex, vtss_appl_nas_eapol_counters_t *const stats)
{
    vtss_nas_statistics_t stati;

    if (!stats) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    memset(stats, 0, sizeof(vtss_appl_nas_eapol_counters_t));
    VTSS_RC(vtss_nas_statistics_get(ifindex, NULL, &stati));
    stats->dot1xAuthEapolFramesRx = stati.eapol_counters.dot1xAuthEapolFramesRx;
    stats->dot1xAuthEapolFramesTx = stati.eapol_counters.dot1xAuthEapolFramesTx;
    stats->dot1xAuthEapolStartFramesRx = stati.eapol_counters.dot1xAuthEapolStartFramesRx;
    stats->dot1xAuthEapolLogoffFramesRx = stati.eapol_counters.dot1xAuthEapolLogoffFramesRx;
    stats->dot1xAuthEapolRespIdFramesRx = stati.eapol_counters.dot1xAuthEapolRespIdFramesRx;
    stats->dot1xAuthEapolRespFramesRx = stati.eapol_counters.dot1xAuthEapolRespFramesRx;
    stats->dot1xAuthEapolReqIdFramesTx = stati.eapol_counters.dot1xAuthEapolReqIdFramesTx;
    stats->dot1xAuthEapolReqFramesTx = stati.eapol_counters.dot1xAuthEapolReqFramesTx;
    stats->dot1xAuthInvalidEapolFramesRx = stati.eapol_counters.dot1xAuthInvalidEapolFramesRx;
    stats->dot1xAuthEapLengthErrorFramesRx = stati.eapol_counters.dot1xAuthEapLengthErrorFramesRx;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_nas_radius_statistics_get(vtss_ifindex_t ifindex, vtss_appl_nas_backend_counters_t *const stats)
{
    vtss_nas_statistics_t stati;

    if (!stats) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    memset(stats, 0, sizeof(vtss_appl_nas_backend_counters_t));
    VTSS_RC(vtss_nas_statistics_get(ifindex, NULL, &stati));
    *stats = stati.backend_counters;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_nas_statistics_get()
// If @vid_mac == NULL or vid_mac->vid == 0, get the port statistics,
// otherwise get the statistics given by @vid_mac.
/******************************************************************************/
mesa_rc vtss_nas_statistics_get(vtss_ifindex_t ifindex, mesa_vid_mac_t *vid_mac, vtss_nas_statistics_t *statistics)
{
    nas_port_t nas_port;
    mesa_rc    rc;
    nas_sm_t   *sm;

    if (!statistics) {
        return VTSS_APPL_NAS_ERROR_INV_PARAM;
    }

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    mesa_port_no_t api_port = ife.ordinal;
    vtss_isid_t    isid     = (ife.isid < VTSS_ISID_START) ? VTSS_ISID_START : ife.isid;

    if ((rc = DOT1X_isid_port_check(isid, api_port, FALSE, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);

    memset(statistics, 0, sizeof(*statistics));

    DOT1X_CRIT_ENTER();

    if (vid_mac == NULL || vid_mac->vid == 0) {
        // Get port-statistics.
        sm = nas_get_top_sm(nas_port);

        // Compose a port-status
        if (DOT1X_stack_cfg.glbl_cfg.enabled) {
            if (DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].switch_exists && (DOT1X_stack_state.switch_state[isid - VTSS_ISID_START].port_state[api_port - VTSS_PORT_NO_START].flags & DOT1X_PORT_STATE_FLAGS_LINK_UP)) {
                statistics->status = nas_get_port_status(nas_port, &statistics->auth_cnt, &statistics->unauth_cnt); // auth_cnt and uauth_cnt only non-zero if multi-client
            } else {
                statistics->status = VTSS_APPL_NAS_PORT_STATUS_LINK_DOWN;
            }
        } else {
            statistics->status = VTSS_APPL_NAS_PORT_STATUS_DISABLED;
        }
    } else {
        // Asking for specific <MAC, VID>
        if ((sm = nas_get_sm_from_vid_mac_port(vid_mac, nas_port)) == NULL) {
            rc = VTSS_APPL_NAS_ERROR_MAC_ADDRESS_NOT_FOUND;
            goto do_exit;
        }
        statistics->status = nas_get_status(sm);
    }

    DOT1X_fill_statistics(sm, statistics);

    // Get the admin_state. Do not use that of the NAS base lib (nas_get_port_info(sm)->port_control),
    // because it may return VTSS_APPL_NAS_PORT_CONTROL_DISABLED, and we want what the user has configured.
    statistics->admin_state = DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port].admin_state;

    rc = VTSS_RC_OK;

do_exit:
    DOT1X_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_nas_port_statistics_clear(vtss_ifindex_t ifindex)
{
    return vtss_nas_statistics_clear(ifindex, NULL);
}
/******************************************************************************/
// vtss_nas_statistics_clear()
// If @vid_mac == NULL or vid_mac->vid == 0, clear everything on that port,
// otherwise only clear entry given by @vid_mac.
/******************************************************************************/
mesa_rc vtss_nas_statistics_clear(vtss_ifindex_t ifindex, mesa_vid_mac_t *vid_mac)
{
    nas_port_t nas_port;
    mesa_rc    rc;

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    mesa_port_no_t api_port = ife.ordinal;
    vtss_isid_t    isid     = ife.isid;

    if ((rc = DOT1X_isid_port_check(isid, api_port, FALSE, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);

    DOT1X_CRIT_ENTER();
    nas_clear_statistics(nas_port, vid_mac);
    DOT1X_CRIT_EXIT();

    return VTSS_RC_OK;
}

#ifdef NAS_MULTI_CLIENT
/******************************************************************************/
// vtss_appl_nas_multi_client_statistics_get()
// For the sake of CLI, we pass a vtss_nas_statistics_t structure to this
// function.
/******************************************************************************/
mesa_rc vtss_nas_multi_client_statistics_get(vtss_ifindex_t ifindex, vtss_nas_statistics_t *statistics, BOOL *found, BOOL start_all_over)
{
    nas_port_t         nas_port;
    mesa_rc            rc;
    nas_sm_t           *sm;
    vtss_nas_client_info_t  *client_info = NULL;
    vtss_appl_nas_port_control_t admin_state;

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    mesa_port_no_t api_port = ife.ordinal;
    vtss_isid_t    isid     = ife.isid;

    if ((rc = DOT1X_isid_port_check(isid, api_port, FALSE, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);
    *found = FALSE;

    DOT1X_CRIT_ENTER();

    admin_state = DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port - VTSS_PORT_NO_START].admin_state;
    if (DOT1X_stack_cfg.glbl_cfg.enabled == FALSE || !NAS_PORT_CONTROL_IS_MULTI_CLIENT(admin_state)) {
        memset(statistics, 0, sizeof(*statistics));
        rc = VTSS_RC_OK; // Not available. This is not an error, so just return.
        goto do_exit;
    }

    sm = nas_get_next(nas_get_top_sm(nas_port));
    if (!start_all_over) {
        while (sm) {
            client_info = nas_get_client_info(sm);
            sm = nas_get_next(sm);
            if (memcmp(&statistics->client_info.vid_mac, &client_info->vid_mac, sizeof(client_info->vid_mac)) == 0) {
                break;
            }
        }
    }

    memset(statistics, 0, sizeof(*statistics));

    if (sm) {
        *found = TRUE;
        DOT1X_fill_statistics(sm, statistics);
        // Status is not filled in by DOT1X_fill_statistics(), because it differs between top- and sub-SMs.
        // Here, @sm is a sub-SM.
        statistics->status = nas_get_status(sm);
        // Get the admin_state. Do not use that of the NAS base lib (nas_get_port_info(sm)->port_control),
        // because it may return VTSS_APPL_NAS_PORT_CONTROL_DISABLED, and we want what the user has configured.
        statistics->admin_state = admin_state;
    }

    rc = VTSS_RC_OK;

do_exit:
    DOT1X_CRIT_EXIT();
    return rc;
}
#endif

// Function for getting last supplicant MAC address as console-presentable string (e.g. "AA-BB-CC-DD-EE-FF").
mesa_rc nas_last_supplicant_info_mac_addr_as_str_get(vtss_ifindex_t ifindex, char *mac_addr_str)
{
    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    mesa_port_no_t api_port = ife.ordinal;

    nas_port_t nas_port  DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);
    vtss_nas_client_info_t tmp_info;
    memset(&tmp_info, 0, sizeof(vtss_nas_client_info_t));
    DOT1X_CRIT_ENTER();

    tmp_info = *nas_get_client_info(nas_get_top_sm(nas_port));
    DOT1X_CRIT_EXIT();
    memcpy(mac_addr_str, tmp_info.mac_addr_str,
           sizeof(tmp_info.mac_addr_str));
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_nas_last_supplicant_info_get()
/******************************************************************************/
mesa_rc vtss_appl_nas_last_supplicant_info_get(vtss_ifindex_t ifindex, vtss_appl_nas_client_info_t *last_supplicant_info)
{
    nas_port_t nas_port;
    mesa_rc    rc;
    vtss_nas_client_info_t tmp_info;

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    mesa_port_no_t api_port = ife.ordinal;
    vtss_isid_t    isid     = ife.isid;

    memset(&tmp_info, 0, sizeof(vtss_nas_client_info_t));

    if ((rc = DOT1X_isid_port_check(isid, api_port, FALSE, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);

    DOT1X_CRIT_ENTER();
    tmp_info = *nas_get_client_info(nas_get_top_sm(nas_port));
    DOT1X_CRIT_EXIT();
    last_supplicant_info->vid_mac = tmp_info.vid_mac;
    memcpy(last_supplicant_info->identity, tmp_info.identity, VTSS_APPL_NAS_SUPPLICANT_ID_MAX_LENGTH);
    last_supplicant_info->rel_creation_time_secs = tmp_info.rel_creation_time_secs;
    memcpy(last_supplicant_info->abs_auth_time,
           misc_time2str(msg_abstime_get(VTSS_ISID_LOCAL, tmp_info.rel_auth_time_secs)),
           sizeof(last_supplicant_info->abs_auth_time));

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_nas_port_reinitialize(vtss_ifindex_t ifindex, const BOOL *reinit)
{
    if (!reinit) {
        return VTSS_RC_ERROR;
    }
    return vtss_nas_reauth(ifindex, *reinit);
}
/******************************************************************************/
// vtss_nas_reauth()
// @now == TRUE  => Reinitialize
// @now == FALSE => Reauthenticate (only affects already authenticated clients)
/******************************************************************************/
mesa_rc vtss_nas_reauth(vtss_ifindex_t ifindex, const BOOL now)
{
    nas_port_t nas_port;
    mesa_rc    rc;

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    mesa_port_no_t api_port = ife.ordinal;
    vtss_isid_t    isid     = ife.isid;

    if ((rc = DOT1X_isid_port_check(isid, api_port, FALSE, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    T_D("%d:%d: Enter (now=%d)", isid, iport2uport(api_port), now);

    DOT1X_CRIT_ENTER();
    if (DOT1X_stack_cfg.glbl_cfg.enabled) {
        nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port);
        if (now) {
            nas_reinitialize(nas_port);
        } else {
            nas_reauthenticate(nas_port);
        }
    }
    DOT1X_CRIT_EXIT();

    return VTSS_RC_OK;
}

/****************************************************************************/
// dot1x_error_txt()
/****************************************************************************/
const char *dot1x_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_NAS_ERROR_INV_PARAM:
        return "Invalid parameter";

    case VTSS_APPL_NAS_ERROR_INVALID_REAUTH_PERIOD:
        // Not nice to use specific values in this string, but
        // much easier than constructing a constant string dynamically.
        return "Reauthentication period out of bounds ([1; 3600] seconds)";

    case VTSS_APPL_NAS_ERROR_INVALID_EAPOL_TIMEOUT:
        // Not nice to use specific values in this string, but
        // much easier than constructing a constant string dynamically.
        return "EAPOL timeout out of bounds ([1; 255] seconds)";

    case VTSS_APPL_NAS_ERROR_INVALID_ADMIN_STATE:
        return "Invalid administrative state";

    case VTSS_APPL_NAS_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on the primary switch";

    case VTSS_APPL_NAS_ERROR_SID:
        return "Invalid Switch ID";

    case VTSS_APPL_NAS_ERROR_PORT:
        return "Invalid port number";

    case VTSS_APPL_NAS_ERROR_STATIC_AGGR_ENABLED:
        return "The 802.1X Admin State must be set to Authorized for ports that are enabled for static aggregation";

    case VTSS_APPL_NAS_ERROR_DYNAMIC_AGGR_ENABLED:
        return "The 802.1X Admin State must be set to Authorized for ports that are enabled for LACP";

    case VTSS_APPL_NAS_ERROR_STP_ENABLED:
        return "The 802.1X Admin State must be set to Authorized for ports that are enabled for Spanning Tree";

    case VTSS_APPL_NAS_ERROR_MAC_ADDRESS_NOT_FOUND:
        return "MAC Address not found on specified port";

#ifdef NAS_USES_PSEC
    case VTSS_APPL_NAS_ERROR_INVALID_HOLD_TIME:
        return "Hold time for clients whose authentication failed is out of bounds";

    case VTSS_APPL_NAS_ERROR_INVALID_AGING_PERIOD:
        return "Aging period for clients whose authentication succeeded is out of bounds";
#endif

#ifdef VTSS_SW_OPTION_NAS_GUEST_VLAN
    case VTSS_APPL_NAS_ERROR_INVALID_GUEST_VLAN_ID:
        return "Guest VLAN ID is out of bounds";

    case VTSS_APPL_NAS_ERROR_INVALID_REAUTH_MAX:
        return "Reauth-Max is out of bounds";
#endif

    case VTSS_APPL_NAS_ERROR_BACKEND_SERVERS_NOT_READY:
        return "No RADIUS servers are ready";

    case VTSS_APPL_NAS_ERROR_BACKEND_SERVER_TIMEOUT:
        return "Timeout during authentication";

    default:
        return "802.1X: Unknown error code";
    }
}

/****************************************************************************/
// dot1x_qos_class_to_str()
/****************************************************************************/
void dot1x_qos_class_to_str(mesa_prio_t iprio, char *str)
{
#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
    if (iprio == VTSS_PRIO_NO_NONE) {
        strcpy(str, "-");
    } else {
        sprintf(str, "%s", mgmt_prio2txt(iprio, FALSE));
    }
#else
    str[0] = '\0';
#endif
}

/****************************************************************************/
// dot1x_vlan_to_str()
/****************************************************************************/
void dot1x_vlan_to_str(mesa_vid_t vid, char *str)
{
#ifdef NAS_USES_VLAN
    if (vid == 0) {
        strcpy(str, "-");
    } else {
        sprintf(str, "%d", vid);
    }
#else
    str[0] = '\0';
#endif
}

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_nas_json_init();
#endif

// Initialize module
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void nas_mib_init(void);
#endif

extern "C" int dot1x_icli_cmd_register();

/******************************************************************************/
// dot1x_init()
// Initialize 802.1X Module
/******************************************************************************/
mesa_rc dot1x_init(vtss_init_data_t *data)
{
    vtss_isid_t    isid = data->isid;
    mesa_rc        rc;
    nas_port_t     nas_port;
    mesa_port_no_t api_port;

    /*lint --e{454,456} ... We leave the Mutex locked */
    switch (data->cmd) {
    case INIT_CMD_INIT:
#ifdef VTSS_SW_OPTION_DOT1X_ACCT
        dot1x_acct_init();
#endif

        vtss_clear(DOT1X_stack_state);

        // Gotta default our configuration, so that we avoid race-conditions
        // (e.g. port-up before we get our ICFG_LOADING_PRE event).
        DOT1X_cfg_default_all(&DOT1X_stack_cfg);
        nas_init();

        // Initialize mutex while keeping it locked
        critd_init_legacy(&DOT1X_crit, "dot1x", VTSS_MODULE_ID_DOT1X, CRITD_TYPE_MUTEX);

        // Initialize message buffer(s)
        vtss_sem_init(&DOT1X_msg_buf_pool.sem, 1);

        // Initialize the thread
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           DOT1X_thread,
                           0,
                           "802.1X",
                           nullptr,
                           0,
                           &DOT1X_thread_handle,
                           &DOT1X_thread_state);
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_nas_json_init();
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register our private mib */
        nas_mib_init();
#endif
        dot1x_icli_cmd_register();
        break;

    case INIT_CMD_START:
        // Register for messages sent from the 802.1X module on other switches in the stack.
        DOT1X_msg_rx_init();

        // Register for 802.1X BPDUs
        DOT1X_bpdu_rx_init();

        // Register for port link-state change events
        if ((rc = port_change_register(VTSS_MODULE_ID_DOT1X, DOT1X_link_state_change_callback)) != VTSS_RC_OK) {
            T_E("Unable to hook link-state change events (%s)", error_txt(rc));
        }

#ifdef NAS_USES_PSEC
        if ((rc = psec_mgmt_register_callbacks(VTSS_APPL_PSEC_USER_DOT1X, DOT1X_on_mac_add_callback, DOT1X_on_mac_del_callback)) != VTSS_RC_OK) {
            T_E("Unable to register callbacks (%s)", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_ICFG
        VTSS_RC(dot1x_icfg_init()); // ICFG initialization (Show running)
#endif
        // Release ourselves for the first time.
        /*lint -e(455) */
        DOT1X_CRIT_EXIT();
        break;

    case INIT_CMD_CONF_DEF:
        if (isid == VTSS_ISID_LOCAL) {
            // Reset local configuration
            // No such configuration for this module
        } else if (VTSS_ISID_LEGAL(isid) || isid == VTSS_ISID_GLOBAL) {
            // Reset switch or stack configuration
            DOT1X_CRIT_ENTER();
            DOT1X_default(isid, FALSE);
            DOT1X_CRIT_EXIT();
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE: {
        // In this state, we are 100% sure that DOT1X_stack_cfg contains
        // default values so that we can safely get called back by the
        // port module before we've actually defaulted the current config.
        DOT1X_CRIT_ENTER();
        DOT1X_default(VTSS_ISID_GLOBAL, TRUE);
        DOT1X_CRIT_EXIT();
        break;
    }

    case INIT_CMD_ICFG_LOADING_POST: {
        dot1x_switch_state_t *switch_state;

        DOT1X_CRIT_ENTER();
        switch_state = &DOT1X_stack_state.switch_state[isid - VTSS_ISID_START];
        switch_state->switch_exists = TRUE;

        // Send the whole switch state to the new switch.
        DOT1X_tx_switch_state(isid, DOT1X_stack_cfg.glbl_cfg.enabled);

        // Loop through all ports on this switch and check if one of them seem to have link
        // and set the port state accordingly. This may happen if the port module calls
        // the DOT1X_link_state_change_callback() before this piece of code gets executed.
        for (api_port = 0; api_port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); api_port++) {
            if (switch_state->port_state[api_port].flags & DOT1X_PORT_STATE_FLAGS_LINK_UP) {
                nas_port = DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port + VTSS_PORT_NO_START);
                vtss_appl_nas_port_control_t admin_state = DOT1X_stack_cfg.glbl_cfg.enabled ? DOT1X_stack_cfg.switch_cfg[isid - VTSS_ISID_START].port_cfg[api_port].admin_state : VTSS_APPL_NAS_PORT_CONTROL_DISABLED;
                vtss_appl_nas_port_control_t cur_state   = nas_get_port_control(nas_port);
                if (cur_state != admin_state) {
                    DOT1X_tx_port_state(isid, api_port + VTSS_PORT_NO_START, DOT1X_stack_cfg.glbl_cfg.enabled, admin_state);
                    DOT1X_set_port_control(nas_port, admin_state, NAS_STOP_REASON_UNKNOWN /* Doesn't matter as there aren't any clients by now */);
                }
            }
        }

        DOT1X_CRIT_EXIT();
        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

#ifdef __cplusplus
}
#endif

