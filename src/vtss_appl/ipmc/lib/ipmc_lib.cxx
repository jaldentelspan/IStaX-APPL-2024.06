/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "ipmc_lib_utils.hxx"
#include "ipmc_lib_base.hxx"
#include "ipmc_lib_pdu.hxx"
#ifdef VTSS_SW_OPTION_SMB_IPMC
#include "ipmc_lib_profile.hxx"
#include "ipmc_lib_profile_observer.hxx"
#endif
#include "ipmc_lib.hxx"
#include "ipmc_lib_api.h"
#include "ipmc_lib_trace.h"
#include <vtss/appl/ipmc_lib.h>
#include "main.h"
#include "critd_api.h"
#include "misc_api.h"
#include "ip_utils.hxx"             /* For vtss_ipv6_addr_is_link_local()                */
#include "l2proto_api.h"            /* For l2port2port()                                 */
#include "msg_api.h"                /* For msg_wait()                                    */
#include "packet_api.h"             /* For packet_rx_filter_register()                   */
#include "port_api.h"               /* For port_change_register()                        */
#include "vlan_api.h"               /* For vlan_mgmt_port_conf_set()                     */
#include "vtss_bip_buffer_api.h"    /* For vtss_bip_buffer_XXX()     .                   */
#include "vtss_common_iterator.hxx" /* For vtss_appl_iterator_ifindex_front_port_exist() */
#include <vtss/appl/vlan.h>         /* For VTSS_APPL_VLAN_ID_MIN/MAX                     */
#include "subject.hxx"              /* For subject_main_thread                           */
#include <vtss/basics/map.hxx>
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/event-handler.hxx>

#define IPMC_LIB_ADDR_V4_MIN_BIT_LEN        4
#define IPMC_LIB_ADDR_V4_MAX_BIT_LEN        32
#define IPMC_LIB_ADDR_V6_MIN_BIT_LEN        8
#define IPMC_LIB_ADDR_V6_MAX_BIT_LEN        128

// See IPMC_LIB_is_mvr_to_idx_get() of how to index into arrays dimensioned by
// IPMC_LIB_USER_CNT.
#if defined(VTSS_SW_OPTION_IPMC) && defined(VTSS_SW_OPTION_MVR)
// Both IPMC and MVR
#define IPMC_LIB_USER_CNT 2
#elif defined(VTSS_SW_OPTION_IPMC)
#define IPMC_LIB_USER_CNT 1
#elif defined(VTSS_SW_OPTION_MVR)
#define IPMC_LIB_USER_CNT 1
#else
#error "Neither IPMC nor MVR is defined"
#endif

// See IPMC_LIB_key_to_idx_get() of how to index into arrays
// dimensioned by IPMC_LIB_STATE_CNT.
#ifdef VTSS_SW_OPTION_IPMC
# ifdef VTSS_SW_OPTION_MVR
#  ifdef VTSS_SW_OPTION_SMB_IPMC
#   define IPMC_LIB_STATE_CNT 4 // IPMC-IGMP, IPMC-MLD, MVR-IGMP, MVR-MLD
#  else
#   define IPMC_LIB_STATE_CNT 2 // IPMC-IGMP,           MVR-IGMP
#  endif
# else
#  ifdef VTSS_SW_OPTION_SMB_IPMC
#   define IPMC_LIB_STATE_CNT 2 // IPMC-IGMP, IPMC-MLD
#  else
#   define IPMC_LIB_STATE_CNT 1 // IPMC-IGMP
#  endif
# endif
#else
# ifdef VTSS_SW_OPTION_MVR
#  ifdef VTSS_SW_OPTION_SMB_IPMC
#    define IPMC_LIB_STATE_CNT 2 //                      MVR-IGMP, MVR-MLD
#  else
#    define IPMC_LIB_STATE_CNT 1 //                      MVR-IGMP
#  endif
# else
#  error "Neither IPMC nor MVR is defined"
# endif
#endif

// For every <vid, is_mvr, is_ipv4> we have one vlan_state. The following map
// uses such a key. It is sorted first by is_mvr (IPMC before MVR), then by
// is_ipv4 (IPv4 before IPv6), and finally by VLAN ID.
// See vtss_appl_ipmc_lib_vlan_key_t::operator<().
typedef vtss::Map<vtss_appl_ipmc_lib_vlan_key_t, ipmc_lib_vlan_state_t> ipmc_lib_vlan_map_t;
typedef ipmc_lib_vlan_map_t::iterator ipmc_lib_vlan_itr_t;
static ipmc_lib_vlan_map_t IPMC_LIB_vlan_map;

static vtss_appl_ipmc_lib_capabilities_t IPMC_LIB_cap;
static critd_t                           IPMC_LIB_crit;
static critd_t                           IPMC_LIB_pdu_crit;
uint32_t                                 IPMC_LIB_port_cnt;

// For IPMC_LIB_tick_thread()
static vtss_handle_t IPMC_LIB_thread_handle;
static vtss_thread_t IPMC_LIB_thread_block;
static bool          IPMC_LIB_thread_running;

// We allocate one instance of ipmc_lib_global_lists_t, which is common to both
// MVR and IPMC as well as IGMP and MLD. It is linked into the
// IPMC_LIB_global_state[] array in IPMC_LIB_global_state_reset().
static ipmc_lib_global_lists_t IPMC_LIB_global_lists;

// We allocate one instance of ipmc_lib_global_state_t per
//   IPMC-IGMP, IPMC-MLD, MVR-IGMP, and MVR-MLD.
// This amounts to something between 1 and 4 instances.
// See also IPMC_LIB_key_to_idx_get().
static ipmc_lib_global_state_t IPMC_LIB_global_state[IPMC_LIB_STATE_CNT];

// Each of IPMC and MVR calls us once with their capabilities, which is then
// stored in the following array.
static vtss_appl_ipmc_capabilities_t IPMC_LIB_protocol_cap[IPMC_LIB_USER_CNT];

// Each of IPMC and MVR calls us once with their global default configuration,
// which is then stored in the following array.
static vtss_appl_ipmc_lib_global_conf_t IPMC_LIB_global_conf_defaults[IPMC_LIB_STATE_CNT];

// Each of IPMC and MVR calls us once with their default port configuration,
// which is then stored in the following array.
static vtss_appl_ipmc_lib_port_conf_t IPMC_LIB_port_conf_defaults[IPMC_LIB_STATE_CNT];

// Each of IPMC and MVR calls us once with their default VLAN configuration,
// which is then stored in the following array.
static vtss_appl_ipmc_lib_vlan_conf_t IPMC_LIB_vlan_conf_defaults[IPMC_LIB_STATE_CNT];

// Each of IPMC and MVR calls us once with their default per-port-per-VLAN
// configuration, which is then stored in the following array.
static vtss_appl_ipmc_lib_vlan_port_conf_t IPMC_LIB_vlan_port_conf_defaults[IPMC_LIB_STATE_CNT];

// Frames we receive from the packet module are passed through a so-called BIP-
// buffer in order not to disturb the packet module's threads too much.
static vtss_bip_buffer_t IPMC_LIB_rx_bip_buffer;

// Keep track of the ports that are up, so that we doesn't happen to handle PDUs
// received on ports that are no longer up (this is needed because there might
// be a delay from Rx of a frame until it is actually handled by us, mainly
// because the frame needs to go through the BIP buffer).
static mesa_port_list_t IPMC_LIB_ports_with_link;

// Flag for signalling the IPMC_LIB_rx_thread() that a frame is ready to be
// handled.
static vtss_flag_t IPMC_LIB_rx_flag;

// For IPMC_LIB_rx_thread()
static vtss_handle_t IPMC_LIB_rx_thread_handle;
static vtss_thread_t IPMC_LIB_rx_thread_block;

// Parsed PDU. This needs not be protected, because we only parse it in the
// IPMC_LIB_rx_thread()
static ipmc_lib_pdu_t IPMC_LIB_rx_pdu;

// When MVR is enabled, we override VLAN port and membership configuration in
// the VLAN module. This one remembers what we have overridden.
static CapArray<vtss_appl_vlan_port_detailed_conf_t, MEBA_CAP_BOARD_PORT_COUNT> IPMC_LIB_vlan_port_conf;

// When set, we can safely call into other modules and run all the configuration
// we have received from startup-config.
static bool IPMC_LIB_started;

// This structure holds interesting parameters of a VLAN interface administered
// by the IP module, used by both MVR and IGMP/MLD.
typedef struct {
    // The VLAN ID of this VLAN interface.
    mesa_vid_t vid;

    // True if this VLAN interface is up (at least one port is member of this
    // VLAN interface and at least one member port is up).
    bool if_up;

    // The last  IPv4 address assigned to this VLAN interface.
    // 0.0.0.0 if no address assigned.
    mesa_ipv4_t ipv4;

    // The last link-local IPv6 address assigned to this VLAN interface.
    // All-zeros if no address assigned.
    mesa_ipv6_t ipv6_link_local;

    // The last non-link-local IPv6 address assigned to this VLAN interface.
    // All-zeros if no address assigned.
    mesa_ipv6_t ipv6;
} ipmc_lib_vlan_if_t;

typedef vtss::Map<mesa_vid_t, ipmc_lib_vlan_if_t> ipmc_lib_vlan_if_map_t;
typedef ipmc_lib_vlan_if_map_t::iterator          ipmc_lib_vlan_if_itr_t;
static  ipmc_lib_vlan_if_map_t                    IPMC_LIB_vlan_if_map;

static vtss_trace_reg_t IPMC_LIB_trace_reg = {
    VTSS_MODULE_ID_IPMC_LIB, "IPMC_LIB", "IPMC Library"
};

static vtss_trace_grp_t IPMC_LIB_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_BASE] = {
        "base",
        "Base",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_RX] = {
        "rx",
        "Packet Rx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_TX] = {
        "tx",
        "Packet Tx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_API] = {
        "api",
        "API calls",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_TICK] = {
        "tick",
        "One second ticks",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_ICLI] = {
        "icli",
        "CLI printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_WEB] = {
        "web",
        "Web",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_QUERIER] = {
        "querier",
        "Querier state",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_CALLBACK] = {
        "callbacks",
        "Callbacks",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_LOG] = {
        "log",
        "Logs",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_BIP] = {
        "bip",
        "Bip-buffer operations",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_LIB_TRACE_GRP_PROFILE] = {
        "profile",
        "Profile operations",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&IPMC_LIB_trace_reg, IPMC_LIB_trace_grps);

struct IPMC_LIB_Lock {
    IPMC_LIB_Lock(const char *file, int line)
    {
        critd_enter(&IPMC_LIB_crit, file, line);
    }

    ~IPMC_LIB_Lock()
    {
        critd_exit(&IPMC_LIB_crit, __FILE__, 0);
    }
};

#define IPMC_LIB_LOCK_SCOPE() IPMC_LIB_Lock __ipmc_lib_lock_guard__(__FILE__, __LINE__)
#define IPMC_LIB_LOCK_ASSERT_LOCKED(_fmt_, ...) if (!critd_is_locked(&IPMC_LIB_crit)) {T_E(_fmt_, ##__VA_ARGS__);}

struct IPMC_LIB_PDU_Lock {
    IPMC_LIB_PDU_Lock(const char *file, int line)
    {
        critd_enter(&IPMC_LIB_pdu_crit, file, line);
    }

    ~IPMC_LIB_PDU_Lock()
    {
        critd_exit(&IPMC_LIB_pdu_crit, __FILE__, 0);
    }
};

#define IPMC_LIB_PDU_LOCK_SCOPE() IPMC_LIB_PDU_Lock __ipmc_lib_pdu_lock_guard__(__FILE__, __LINE__)

/******************************************************************************/
// IPMC_LIB_vlan_oper_warnings_mvr_vs_ipmc_update()
/******************************************************************************/
static void IPMC_LIB_vlan_oper_warnings_mvr_vs_ipmc_update(const vtss_appl_ipmc_lib_vlan_key_t &the_key)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key = the_key;
    ipmc_lib_vlan_itr_t           ipmc_vlan_itr, mvr_vlan_itr;
    bool                          ipmc_exists;
    bool                          ipmc_active, mvr_active;

    // Check if both MVR and IPMC has the same VLAN active.
    // If so, MVR wins, and we need to remove all entries from IPMC.

    // Check IPMC
    vlan_key.is_mvr = false;
    ipmc_exists     = false;
    ipmc_active     = false;
    if ((ipmc_vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) != IPMC_LIB_vlan_map.end()) {
        ipmc_exists = true;

        if (ipmc_vlan_itr->second.global->conf.admin_active && ipmc_vlan_itr->second.conf.admin_active) {
            ipmc_active = true;
        }
    }

    // Check MVR
    vlan_key.is_mvr = true;
    mvr_active      = false;
    if ((mvr_vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) != IPMC_LIB_vlan_map.end()) {
        if (mvr_vlan_itr->second.status.oper_state == VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
            mvr_active = true;
        }
    }

    if (ipmc_active && mvr_active) {
        // MVR wins. Deactivate (but don't delete) the IPMC instance.
        // When new frames arrive for this VLAN, they are directed to MVR in
        // IPMC_LIB_rx_packet_dispatch().
        ipmc_lib_base_deactivate(ipmc_vlan_itr->second);
        ipmc_vlan_itr->second.status.oper_warnings |= VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_IPMC_AND_MVR_BOTH_ACTIVE_IPMC;
    } else {
        if (ipmc_exists) {
            ipmc_vlan_itr->second.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_IPMC_AND_MVR_BOTH_ACTIVE_IPMC;
        }
    }
}

/******************************************************************************/
// IPMC_LIB_oper_warnings_port_role_update()
/******************************************************************************/
static void IPMC_LIB_oper_warnings_port_role_update(ipmc_lib_vlan_state_t &vlan_state)
{
    vtss_appl_ipmc_lib_port_role_t role;
    mesa_port_no_t                 port_no;
    bool                           at_least_one_port_active[2] = {}; // [0] = Source, [1] = Receiver
    int                            i;

    // First clear the two warnings this function is responsible for.
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_NO_SOURCE_PORTS_CONFIGURED;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_NO_RECEIVER_PORTS_CONFIGURED;

    if (!vlan_state.vlan_key.is_mvr) {
        // Only MVR VLANs can have this warning set.
        return;
    }

    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        role = vlan_state.port_conf[port_no].role;
        if (role == VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE) {
            continue;
        }

        at_least_one_port_active[role == VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE ? 0 : 1] = true;
    }

    // Time to check for operational warnings
    for (i = 0; i < ARRSZ(at_least_one_port_active); i++) {
        if (!at_least_one_port_active[i]) {
            // Set the appropriate operational warning
            vlan_state.status.oper_warnings |= i == 0 ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_NO_SOURCE_PORTS_CONFIGURED : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_NO_RECEIVER_PORTS_CONFIGURED;
        }
    }
}

/******************************************************************************/
// IPMC_LIB_oper_warnings_vlan_interface_update()
/******************************************************************************/
static void IPMC_LIB_oper_warnings_vlan_interface_update(ipmc_lib_vlan_state_t &vlan_state)
{
    ipmc_lib_vlan_if_itr_t vlan_if_itr;
    vtss_appl_vlan_entry_t membership;
    mesa_port_no_t         port_no;
    bool                   at_least_one_source_port_is_member;
    mesa_rc                rc;

    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_AT_LEAST_ONE_SOURCE_PORT_MEMBER_OF_VLAN_INTERFACE;

    if (!vlan_state.vlan_key.is_mvr) {
        // This is an MVR thing.
        return;
    }

    if ((vlan_if_itr = IPMC_LIB_vlan_if_map.find(vlan_state.vlan_key.vid)) == IPMC_LIB_vlan_if_map.end()) {
        // This MVR VLAN ID is not used by any VLAN interface. No warnings to
        // raise.
        return;
    }

    // According to the old code, an MVR source port must not be a member of a
    // VLAN interface. I don't know exactly why, because the old code didn't
    // write why, only gave a trace warning every 5 seconds. I think it may be
    // because all frames are now transmitted tagged on the MVR VLAN, so if the
    // MVR VLAN is also an IP interface, management access may no longer work.

    // We ask for what the user has configured, because we have ourselves added
    // the MVR VLAN to source ports.
    if ((rc = vtss_appl_vlan_get(VTSS_ISID_START, vlan_state.vlan_key.vid, &membership, false, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
        // This is quite common if the end-user hasn't added this VLAN.
        T_I("vtss_appl_vlan_get(%u) failed: %s", vlan_state.vlan_key.vid, error_txt(rc));

        // In this case, no source ports can be members of this VLAN. No
        // operational warnings.
        return;
    }

    at_least_one_source_port_is_member = false;
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (vlan_state.port_conf[port_no].role == VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE && membership.ports[port_no]) {
            at_least_one_source_port_is_member = true;
            break;
        }
    }

    if (at_least_one_source_port_is_member) {
        vlan_state.status.oper_warnings |= VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_AT_LEAST_ONE_SOURCE_PORT_MEMBER_OF_VLAN_INTERFACE;
    }
}

/******************************************************************************/
// IPMC_LIB_vlan_module_port_conf_update()
// Notice, the reason that we override VLAN port configuration the way we do is
// because of this module's legacy. Back when this module was originally
// written, it was written this way, so we need to keep that functionality.
//
// Had it been today, we had asked the end-user to configure VLANs using CLI
// commands to set up everything as it should be.
//
// The old implementation also changed uvid/untagged_vid, but this member is not
// used at all when we only can set a port to Tx all frames untagged or all
// frames tagged.
/******************************************************************************/
static void IPMC_LIB_vlan_module_port_conf_update(mesa_port_no_t port_no)
{
#ifdef VTSS_SW_OPTION_MVR
    vtss_appl_vlan_port_detailed_conf_t hybrid = IPMC_LIB_vlan_port_conf[port_no];
    vtss_appl_vlan_port_conf_t          vlan_port_conf;
    ipmc_lib_vlan_itr_t                 vlan_itr;
    vtss_appl_ipmc_lib_port_role_t      port_role;
    bool                                at_least_one_mvr_vlan_active;
    mesa_rc                             rc;

    // An invariant is that one port cannot be configured as source in one MVR
    // VLAN and receiver in another (and vice versa).
    // This is checked in vtss_appl_ipmc_lib_vlan_port_conf_set().

    port_role                    = VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE;
    at_least_one_mvr_vlan_active = false;
    for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
        if (!vlan_itr->first.is_mvr) {
            continue;
        }

        if (vlan_itr->second.status.oper_state == VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
            at_least_one_mvr_vlan_active = true;
        }

        if (vlan_itr->second.port_conf[port_no].role != VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE) {
            port_role = vlan_itr->second.port_conf[port_no].role;
        }
    }

    if (!at_least_one_mvr_vlan_active || port_role == VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE) {
        // No MVR VLAN instance is active on this port (anymore).
        // Un-override all our settings.
        hybrid.flags = 0;
    } else {
        // Source or receiver port.

        // Accept both tagged and untagged frames.
        // Corresponding flag is VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE.
        hybrid.frame_type = MESA_VLAN_FRAME_ALL;

        // Make it a C-port, accepting C-tagged frames.
        // Corresponding flag is VTSS_APPL_VLAN_PORT_FLAGS_AWARE.
        hybrid.port_type = VTSS_APPL_VLAN_PORT_TYPE_C;

        // All frames are transmitted tagged on source ports and untagged on
        // receiver ports.
        // Corresponding flag is VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE.
        hybrid.tx_tag_type = port_role == VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE ? VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL : VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL;

        // Set the flags corresponding to the fields we wish to override.
        hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE | VTSS_APPL_VLAN_PORT_FLAGS_AWARE | VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE;
    }

    if (memcmp(&hybrid, &IPMC_LIB_vlan_port_conf[port_no], sizeof(hybrid)) == 0) {
        // No changes.
        return;
    }

    vtss_clear(vlan_port_conf);
    vlan_port_conf.hybrid = hybrid;

    T_I("vlan_mgmt_port_conf_set(%u, %s)", port_no, hybrid);
    if ((rc = vlan_mgmt_port_conf_set(VTSS_ISID_START, port_no, &vlan_port_conf, VTSS_APPL_VLAN_USER_MVR)) != VTSS_RC_OK) {
        T_E("vlan_mgmt_port_conf_set(%u, %s) failed: %s", port_no, hybrid, error_txt(rc));
        return;
    }

    // One could check (with
    // vlan_mgmt_port_conf_get(..., VTSS_APPL_VLAN_USER_ALL)), that all our
    // changes indeed made it to H/W and raise an operational warning if not.
    IPMC_LIB_vlan_port_conf[port_no] = hybrid;
#endif
}

/******************************************************************************/
// IPMC_LIB_vlan_module_membership_update()
// See top or IPMC_LIB_vlan_module_port_conf_update() for a description of why
// we actually add or delete VLAN memberships rather than letting the user do it.
/******************************************************************************/
static void IPMC_LIB_vlan_module_membership_update(vtss_appl_ipmc_lib_vlan_key_t &the_key)
{
#ifdef VTSS_SW_OPTION_MVR
    vtss_appl_vlan_entry_t        vlan_memberships;
    vtss_appl_ipmc_lib_vlan_key_t vlan_key = the_key;
    ipmc_lib_vlan_itr_t           vlan_itr;
    mesa_port_no_t                port_no;
    int                           i;
    mesa_rc                       rc;

    if (!vlan_key.is_mvr) {
        return;
    }

    vtss_clear(vlan_memberships);
    vlan_memberships.vid = vlan_key.vid;

    // If at least one of IGMP and MLD are active, we need to add MVR VLAN
    // memberships for both source and receiver ports.
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vlan_key.is_ipv4 = i == 0;
        if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) != IPMC_LIB_vlan_map.end()) {
            if (vlan_itr->second.status.oper_state == VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
                // Find source and receiver ports. In reality, we should only
                // make source ports members, but in order for M/C frames to
                // come from source to receiver port, we also make receiver
                // ports members.
                for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
                    vlan_memberships.ports[port_no] = vlan_itr->second.port_conf[port_no].role != VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE;
                }
            }
        }
    }

    // Now, we have per-port VLAN memberships, which are only non-zero if MVR
    // is active (globally and locally enabled as well as no operational
    // warnings that makes it inactive).

    // If we call vlan_mgmt_vlan_add() with an empty port list, the VLAN module
    // will actually delete our memberships of the VID. This is easier than
    // calling vlan_mgmt_vlan_del() if the list is empty.
    T_I("vlan_mgmt_vlan_add(%u, %s)", vlan_memberships.vid, vlan_memberships.ports);
    if ((rc = vlan_mgmt_vlan_add(VTSS_ISID_START, &vlan_memberships, VTSS_APPL_VLAN_USER_MVR)) != VTSS_RC_OK) {
        T_E("vlan_mgmt_vlan_add(%u, %s) failed: %s", vlan_memberships.vid, vlan_memberships.ports, error_txt(rc));
    }
#endif
}

/******************************************************************************/
// IPMC_LIB_vlan_module_membership_update_all()
/******************************************************************************/
static void IPMC_LIB_vlan_module_membership_update_all(void)
{
    ipmc_lib_vlan_itr_t vlan_itr;

    for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
        IPMC_LIB_vlan_module_membership_update(vlan_itr->second.vlan_key);
    }
}

/******************************************************************************/
// IPMC_LIB_vlan_module_port_conf_update_all()
/******************************************************************************/
static void IPMC_LIB_vlan_module_port_conf_update_all(void)
{
    mesa_port_no_t port_no;

    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        IPMC_LIB_vlan_module_port_conf_update(port_no);
    }
}

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_LIB_profile_ranges_overlap()
// Returns true if the two profile ranges overlap
/******************************************************************************/
static bool IPMC_LIB_profile_ranges_overlap(vtss_appl_ipmc_lib_profile_range_conf_t &range_a, vtss_appl_ipmc_lib_profile_range_conf_t &range_b)
{
    // There is an overlap if StartA <= EndB && EndA >= StartB.
    if (range_a.start.is_ipv4) {
        // IPv4
        return range_a.start.ipv4 <= range_b.end.ipv4 && range_a.end.ipv4 >= range_b.start.ipv4;
    }

    // IPv6
    // We only have operator< for mesa_ipv6_t, so we need to change the above
    // rule (StartA <= EndB...) to something that uses that.
    // The rule is the same as !(StartA > EndB) && !(EndA < StartB), which is
    // the same as !(EndB < StartA) && !(EndA < StartB), which is the same as
    // (de Morgan): !(EndB < StartA || EndA < StartB).
    return !(range_b.end.ipv6 < range_a.start.ipv6 || range_a.end.ipv6 < range_b.start.ipv6);
}
#endif

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_LIB_profile_rule_shadow_check()
/******************************************************************************/
static void IPMC_LIB_profile_rule_shadow_check(ipmc_lib_vlan_state_t &vlan_state, vtss_appl_ipmc_lib_profile_rule_conf_t &outer_rule, vtss_appl_ipmc_lib_profile_range_conf_t &outer_range, vtss_appl_ipmc_lib_profile_rule_conf_t &inner_rule, vtss_appl_ipmc_lib_profile_range_conf_t &inner_range)
{
    if (outer_range.start.is_ipv4 != inner_range.start.is_ipv4) {
        // IPv4 rules cannot affect IPv6 rules and vice versa.
        return;
    }

    if (vlan_state.vlan_key.is_ipv4 != outer_range.start.is_ipv4) {
        // IGMP VLAN states doesn't care about IPv6 ranges and vice versa.
        return;
    }

    if (!outer_rule.deny) {
        // When outer rule (the one that comes first in the list) is a permit
        // rule, an inner rule (one that comes later in the list) cannot affect
        // the permit rule, whether it's a permit or deny rule.
        return;
    }

    // When outer rule is a deny rule, it may affect inner permit rules, but not
    // inner deny rules (except perhaps for logging, which we ignore).
    if (inner_rule.deny) {
        return;
    }

    // Here, outer rule is a deny rule and inner rule is a permit rule. The
    // inner rule has no effect if there is overlap between addresses.
    if (IPMC_LIB_profile_ranges_overlap(outer_range, inner_range)) {
        if (vlan_state.vlan_key.is_mvr) {
            vlan_state.status.oper_warnings |= vlan_state.vlan_key.is_ipv4 ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV4_MVR  : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV6_MVR;
        } else {
            vlan_state.status.oper_warnings |= vlan_state.vlan_key.is_ipv4 ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV4_IPMC : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV6_IPMC;
        }
    }
}
#endif

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_LIB_profile_rule_conf_get()
/******************************************************************************/
static bool IPMC_LIB_profile_rule_conf_get(vtss_appl_ipmc_lib_profile_key_t &profile_key, vtss_appl_ipmc_lib_profile_range_key_t &range_key, vtss_appl_ipmc_lib_profile_rule_conf_t &rule_conf)
{
    mesa_rc rc;

    if ((rc = vtss_appl_ipmc_lib_profile_rule_conf_get(&profile_key, &range_key, &rule_conf)) != VTSS_RC_OK) {
        // Could be we have to demote this to a T_I() is someone deletes this
        // rule between the iterator and this get.
        T_E("vtss_appl_ipmc_lib_profile_rule_conf_get(%s, %s) failed: %s", profile_key.name, range_key.name, error_txt(rc));
        return false;
    }

    return true;
}
#endif

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_LIB_profile_range_conf_get()
/******************************************************************************/
static bool IPMC_LIB_profile_range_conf_get(vtss_appl_ipmc_lib_profile_range_key_t &range_key, vtss_appl_ipmc_lib_profile_range_conf_t &range_conf)
{
    mesa_rc rc;

    // Stupid to have a concept of IPMC ranges that defines the IPv4 address
    // range rather than having these embedded directly in the rules.
    if ((rc = vtss_appl_ipmc_lib_profile_range_conf_get(&range_key, &range_conf)) != VTSS_RC_OK) {
        // Could be we have to demote this to a T_I() if someone deletes this
        // range between the iterator and this get.
        T_E("vtss_appl_ipmc_lib_profile_range_conf_get(%s) failed: %s", range_key.name, error_txt(rc));
        return false;
    }

    return true;
}
#endif

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_LIB_oper_warnings_profile_update_profile()
/******************************************************************************/
static bool IPMC_LIB_oper_warnings_profile_update_profile(ipmc_lib_vlan_state_t &vlan_state, vtss_appl_ipmc_lib_profile_key_t &profile_key)
{
    vtss_appl_ipmc_lib_profile_conf_t       profile_conf;
    vtss_appl_ipmc_lib_profile_range_key_t  range_key_outer, range_key_inner;
    vtss_appl_ipmc_lib_profile_range_conf_t range_conf_outer, range_conf_inner;
    vtss_appl_ipmc_lib_profile_rule_conf_t  rule_conf_outer, rule_conf_inner;
    bool                                    has_permit_rule;
    int                                     cnt;
    mesa_rc                                 rc;

    if ((rc = vtss_appl_ipmc_lib_profile_conf_get(&profile_key, &profile_conf)) != VTSS_RC_OK) {
        // Not a trace error.
        T_I("vtss_appl_ipmc_lib_profile_conf_get(%s) failed. %s", profile_key.name, error_txt(rc));
        vlan_state.status.oper_warnings |= vlan_state.vlan_key.is_mvr ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_DOESNT_EXIST_MVR : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_DOESNT_EXIST_IPMC;

        // Nothing else to do.
        return false;
    }

    // Loop through all rules and see if they are OK.
    vtss_clear(range_key_outer);
    cnt = 0;
    has_permit_rule = false;
    while (vtss_appl_ipmc_lib_profile_rule_itr(&profile_key, &profile_key, &range_key_outer, &range_key_outer, true /* stay in this profile */) == VTSS_RC_OK) {
        if (!IPMC_LIB_profile_rule_conf_get(profile_key, range_key_outer, rule_conf_outer)) {
            continue;
        }

        if (!IPMC_LIB_profile_range_conf_get(range_key_outer, range_conf_outer)) {
            continue;
        }

        if (range_conf_outer.start.is_ipv4 != vlan_state.vlan_key.is_ipv4) {
            // IPv4 VLAN states don't care about IPv6 ranges and vice versa
            continue;
        }

        cnt++;

        if (!rule_conf_outer.deny) {
            // This is a permit rule, so we have at least one.
            has_permit_rule = true;
        }

        // Check for range overlaps within this profile.
        // Iterate once more, this time starting with range_key_outer.
        range_key_inner = range_key_outer;
        while (vtss_appl_ipmc_lib_profile_rule_itr(&profile_key, &profile_key, &range_key_inner, &range_key_inner, true /* stay in this profile */) == VTSS_RC_OK) {
            if (!IPMC_LIB_profile_rule_conf_get(profile_key, range_key_inner, rule_conf_inner)) {
                continue;
            }

            if (!IPMC_LIB_profile_range_conf_get(range_key_inner, range_conf_inner)) {
                continue;
            }

            IPMC_LIB_profile_rule_shadow_check(vlan_state, rule_conf_outer, range_conf_outer, rule_conf_inner, range_conf_inner);
        }
    }

    if (cnt == 0) {
        if (vlan_state.vlan_key.is_mvr) {
            vlan_state.status.oper_warnings |= vlan_state.vlan_key.is_ipv4 ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_RANGES_MVR  : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_RANGES_MVR;
        } else {
            vlan_state.status.oper_warnings |= vlan_state.vlan_key.is_ipv4 ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_RANGES_IPMC : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_RANGES_IPMC;
        }

        // No need to check the rest. The only other warning, we could raise is
        // if some other MVR VLAN uses the same profile, but that comes when
        // this warning disappears.
        return false;
    } else if (!has_permit_rule) {
        if (vlan_state.vlan_key.is_mvr) {
            vlan_state.status.oper_warnings |= vlan_state.vlan_key.is_ipv4 ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_PERMIT_RULES_MVR : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_PERMIT_RULES_MVR;
        } else {
            vlan_state.status.oper_warnings |= vlan_state.vlan_key.is_ipv4 ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_PERMIT_RULES_IPMC : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_PERMIT_RULES_IPMC;
        }

        // No need to check against other profiles, since this profile only has
        // deny rules.
        return false;
    }

    // Tells caller to go on with checking against other profiles.
    return true;
}
#endif

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_LIB_oper_warnings_profile_update_mvr()
/******************************************************************************/
static void IPMC_LIB_oper_warnings_profile_update_mvr(ipmc_lib_vlan_state_t &vlan_state, bool globally_enabled)
{
    vtss_appl_ipmc_lib_profile_key_t         profile_key, profile_key_other;
    vtss_appl_ipmc_lib_profile_range_key_t   range_key_this,  range_key_other;
    vtss_appl_ipmc_lib_profile_range_conf_t  range_conf_this, range_conf_other;
    vtss_appl_ipmc_lib_profile_rule_conf_t   rule_conf_this,  rule_conf_other;
    ipmc_lib_vlan_itr_t                      vlan_itr;

    if (!globally_enabled) {
        vlan_state.status.oper_warnings |= VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_GLOBALLY_DISABLED_MVR;
    }

    profile_key = vlan_state.conf.channel_profile;

    if (profile_key.name[0] == '\0') {
        vlan_state.status.oper_warnings |= VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_NOT_SET_MVR;

        // Nothing else to do
        return;
    }

    if (!IPMC_LIB_oper_warnings_profile_update_profile(vlan_state, profile_key)) {
        // Nothing else to do.
        return;
    }

    // Check for overlap against other MVR VLAN instances.
    vtss_clear(range_key_this);
    while (vtss_appl_ipmc_lib_profile_rule_itr(&profile_key, &profile_key, &range_key_this, &range_key_this, true /* stay in this profile */) == VTSS_RC_OK) {
        if (!IPMC_LIB_profile_rule_conf_get(profile_key, range_key_this, rule_conf_this)) {
            continue;
        }

        if (rule_conf_this.deny) {
            // We don't care about deny rules.
            continue;
        }

        if (!IPMC_LIB_profile_range_conf_get(range_key_this, range_conf_this)) {
            continue;
        }

        for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
            if (!vlan_itr->first.is_mvr) {
                // Only check against other MVR VLANs.
                continue;
            }

            if (!vlan_itr->second.conf.admin_active) {
                // Only check against other enabled MVR VLANs (in case someone
                // someday decides to be able to disable an MVR VLAN from
                // management interfaces, which is not possible today).
                continue;
            }

            if (vlan_itr->first.vid == vlan_state.vlan_key.vid) {
                // Don't check against ourselves. The thing is that both IGMP
                // and MLD use the same channel profile, so it's OK that if we
                // configure IGMP, then the MLD channel profile is the same and
                // vice versa. Therefore, we don't compare vlan_key.is_ipv4 with
                // vlan_itr's.
                continue;
            }

            // If the other MVR VLAN uses the same profile, there is a problem,
            // but this should not be possible, because this is checked in
            // vtss_appl_ipmc_lib_vlan_conf_set().
            profile_key_other = vlan_itr->second.conf.channel_profile;
            if (profile_key == profile_key_other) {
                T_EG(IPMC_LIB_TRACE_GRP_CALLBACK, "MVR VLAN %u uses the same channel profile (%s) as MVR VLAN %u", vlan_itr->first.vid, profile_key.name, vlan_state.vlan_key.vid);

                // Nothing else to check for this instance.
                continue;
            }

            // Check if the other has a permit rule that overlap our permit
            // rule.
            vtss_clear(range_key_other);
            while (vtss_appl_ipmc_lib_profile_rule_itr(&profile_key_other, &profile_key_other, &range_key_other, &range_key_other, true /* stay in other's profile */) == VTSS_RC_OK) {
                if (!IPMC_LIB_profile_rule_conf_get(profile_key_other, range_key_other, rule_conf_other)) {
                    continue;
                }

                if (rule_conf_other.deny) {
                    // We don't care about deny rules.
                    continue;
                }

                if (!IPMC_LIB_profile_range_conf_get(range_key_other, range_conf_other)) {
                    continue;
                }

                if (IPMC_LIB_profile_ranges_overlap(range_conf_this, range_conf_other)) {
                    vlan_state.status.oper_warnings |= range_conf_this.start.is_ipv4 ? VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_OTHER_OVERLAPS_IPV4_MVR : VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_OTHER_OVERLAPS_IPV6_MVR;
                }
            }
        }
    }
}
#endif

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_LIB_oper_warnings_profile_update_ipmc()
/******************************************************************************/
static void IPMC_LIB_oper_warnings_profile_update_ipmc(ipmc_lib_vlan_state_t &vlan_state, bool globally_enabled)
{
    vtss_appl_ipmc_lib_profile_key_t profile_key;
    mesa_port_no_t                   port_no;

    // Profiles need not be created for IPMC. If at least one port has a profile
    // we check it and update operational warnings accordingly.
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        profile_key = vlan_state.global->port_conf[port_no].profile_key;

        if (profile_key.name[0] == '\0') {
            continue;
        }

        if (!globally_enabled) {
            vlan_state.status.oper_warnings |= VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_GLOBALLY_DISABLED_IPMC;
        }

        (void)IPMC_LIB_oper_warnings_profile_update_profile(vlan_state, profile_key);
    }
}
#endif

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_LIB_oper_warnings_profile_update()
/******************************************************************************/
static void IPMC_LIB_oper_warnings_profile_update(ipmc_lib_vlan_state_t &vlan_state)
{
    vtss_appl_ipmc_lib_profile_global_conf_t global_profile_conf;
    mesa_rc                                  rc;

    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_GLOBALLY_DISABLED_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_GLOBALLY_DISABLED_IPMC;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_NOT_SET_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_DOESNT_EXIST_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_DOESNT_EXIST_IPMC;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_RANGES_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_RANGES_IPMC;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_RANGES_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_RANGES_IPMC;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_PERMIT_RULES_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_PERMIT_RULES_IPMC;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_PERMIT_RULES_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_PERMIT_RULES_IPMC;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV4_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV4_IPMC;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV6_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV6_IPMC;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_OTHER_OVERLAPS_IPV4_MVR;
    vlan_state.status.oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_OTHER_OVERLAPS_IPV6_MVR;

    if (!vlan_state.global->conf.admin_active || !vlan_state.conf.admin_active) {
        // Only update warnings on administratively active VLANs.
        vlan_state.status.oper_state = VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ADMIN_DISABLED;
        return;
    }

    if ((rc = vtss_appl_ipmc_lib_profile_global_conf_get(&global_profile_conf)) != VTSS_RC_OK) {
        T_E("vtss_appl_ipmc_lib_profile_global_conf_get() failed: %s", error_txt(rc));
        global_profile_conf.enable = false;
    }

    if (vlan_state.vlan_key.is_mvr) {
        IPMC_LIB_oper_warnings_profile_update_mvr(vlan_state, global_profile_conf.enable);
    } else {
        IPMC_LIB_oper_warnings_profile_update_ipmc(vlan_state, global_profile_conf.enable);
    }

    // Only MVR VLANs may become operational inactive. This happens when either
    // of the following operational warnings are set. These can never be set on
    // IPMC VLANs.
    if ((vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_GLOBALLY_DISABLED_MVR)                                          ||
        (vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_NOT_SET_MVR)                                                    ||
        (vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_DOESNT_EXIST_MVR)                                               ||
        ( vlan_state.vlan_key.is_ipv4 && (vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_RANGES_MVR))       ||
        (!vlan_state.vlan_key.is_ipv4 && (vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_RANGES_MVR))       ||
        ( vlan_state.vlan_key.is_ipv4 && (vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_PERMIT_RULES_MVR)) ||
        (!vlan_state.vlan_key.is_ipv4 && (vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_PERMIT_RULES_MVR)) ||
        ( vlan_state.vlan_key.is_ipv4 && (vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_OTHER_OVERLAPS_IPV4_MVR))      ||
        (!vlan_state.vlan_key.is_ipv4 && (vlan_state.status.oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_OTHER_OVERLAPS_IPV6_MVR))) {
        if (vlan_state.status.oper_state == VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
            // Deactivate it.
            ipmc_lib_base_deactivate(vlan_state);
            IPMC_LIB_vlan_module_membership_update(vlan_state.vlan_key);
            IPMC_LIB_vlan_module_port_conf_update_all();
        }

        vlan_state.status.oper_state = VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_INACTIVE;
    } else {
        if (vlan_state.vlan_key.is_mvr && vlan_state.status.oper_state != VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
            // (Re-)activate MVR instance
            IPMC_LIB_vlan_module_membership_update(vlan_state.vlan_key);
            IPMC_LIB_vlan_module_port_conf_update_all();
            ipmc_lib_base_vlan_state_init(vlan_state, false /* don't clear statistics */);
        }

        vlan_state.status.oper_state = VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE;
    }
}
#endif

/******************************************************************************/
// IPMC_LIB_vlan_oper_warnings_update()
/******************************************************************************/
static void IPMC_LIB_vlan_oper_warnings_update(ipmc_lib_vlan_state_t &vlan_state)
{
    IPMC_LIB_oper_warnings_port_role_update(vlan_state);
    IPMC_LIB_oper_warnings_vlan_interface_update(vlan_state);
#ifdef VTSS_SW_OPTION_SMB_IPMC
    IPMC_LIB_oper_warnings_profile_update(vlan_state);
#endif

    // This one uses vlan_state.status.oper_state, possibly changed by
    // IPMC_LIB_oper_warnings_profile_update().
    IPMC_LIB_vlan_oper_warnings_mvr_vs_ipmc_update(vlan_state.vlan_key); // Uses Inter-VLAN-states
}

/******************************************************************************/
// IPMC_LIB_vlan_oper_warnings_update_all()
/******************************************************************************/
static void IPMC_LIB_vlan_oper_warnings_update_all(void)
{
    ipmc_lib_vlan_itr_t vlan_itr;

    for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
        IPMC_LIB_vlan_oper_warnings_update(vlan_itr->second);
    }
}

#ifdef VTSS_SW_OPTION_SMB_IPMC
//*****************************************************************************/
// IPMC_LIB_profile_change_notifications().
// Snoops on changes to IPMC profiles to be able to raise appropriate
// operational warnings if a profile we use gets deleted or is invalid.
/******************************************************************************/
static struct ipmc_lib_profile_change_notifications_t : public vtss::notifications::EventHandler {
    vtss::notifications::Event                       e;
    ipmc_lib_profile_change_notification_t::Observer o;

    ipmc_lib_profile_change_notifications_t() : EventHandler(&vtss::notifications::subject_main_thread), e(this)
    {
    }

    void init()
    {
        ipmc_lib_profile_change_notification.observer_new(&e);
    }

    void execute(vtss::notifications::Event *event)
    {
        ipmc_lib_vlan_itr_t vlan_itr;
        mesa_port_no_t      port_no;
        int                 i;

        // The observer_get() moves all the events captured per key into
        // #o. This object contains a map, events, whose events->first is a
        // vtss_appl_ipmc_lib_profile_key_t and whose events->second is an
        // integer indicating whether event->first has been added
        // (EventType::Add; 1), modified (EventType::Modify; 2), or deleted
        // (EventType::Delete; 3).
        // The observer runs a state machine, so that it only needs to return
        // one value per key. So if e.g. first an Add, then a Delete operation
        // was performed on a key before this execute() function got invoked,
        // the observer's event map would have EventType::None (0), which would
        // be erased from the map, so that we don't get to see it.
        ipmc_lib_profile_change_notification.observer_get(&e, o);

        for (auto i = o.events.cbegin(); i != o.events.cend(); ++i) {
            T_DG(IPMC_LIB_TRACE_GRP_CALLBACK, "%s: %s event", i->first.name, vtss::notifications::EventType::txt[vtss::notifications::EventType::E(i->second)].valueName);
        }

        IPMC_LIB_LOCK_SCOPE();

        if (!IPMC_LIB_started) {
            // Defer these change notifications until we are ready to handle
            // them.
            return;
        }

        // Changes to a profile used by one VLAN may affect another VLAN, so it
        // is not enough to check only the VLAN that uses this profile. Also,
        // in case of MVR, the entire MVR VLAN may get activated or deactivate,
        // which may also cause changes to IPMC VLANs' operational warnings.
        IPMC_LIB_vlan_oper_warnings_update_all();

        // Loop through all global states to see if they use a profile. If so,
        // call the base to adjust the currently registered groups according to
        // the possibly changed profile.
        for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
            ipmc_lib_global_state_t &glb = IPMC_LIB_global_state[i];

            if (glb.key.is_mvr) {
                // Not IPMC (MVR cannot have per-port profiles).
                continue;
            }

            for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
                if (glb.port_conf[port_no].profile_key.name[0] != '\0') {
                    ipmc_lib_base_port_profile_changed(glb, port_no);
                }
            }
        }

        // Loop through all MVR VLANs to see if they use a channel profile. If
        // so, call the base to adjust the currently registered groups according
        // to the possibly changed profile.
        for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
            if (vlan_itr->second.status.oper_state == VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
                ipmc_lib_base_vlan_profile_changed(vlan_itr->second);
            }
        }
    }
} IPMC_LIB_profile_change_notifications;
#endif

/******************************************************************************/
// IPMC_LIB_key_to_idx_get()
// Returns a number between 0 and 3 depending on what is included in this build.
// Used for indexing into various arrays whose size is given by
// IPMC_LIB_STATE_CNT.
/******************************************************************************/
static int IPMC_LIB_key_to_idx_get(const vtss_appl_ipmc_lib_key_t &key)
{
#ifdef VTSS_SW_OPTION_IPMC
# ifdef VTSS_SW_OPTION_MVR
#  ifdef VTSS_SW_OPTION_SMB_IPMC
    // IPMC-IGMP, IPMC-MLD, MVR-IGMP, MVR-MLD
    return key.is_mvr ? (key.is_ipv4 ? 2 : 3) : (key.is_ipv4 ? 0 : 1);
#  else
    // IPMC-IGMP,           MVR-IGMP
    if (!key.is_ipv4) {
        T_E("MLD requested, but MLD not included in build");
        return 0;
    }

    return key.is_mvr ? 1 : 0;
#  endif
# else
    if (key.is_mvr) {
        T_E("MVR requested, but MVR not included in build");
        return 0;
    }
#  ifdef VTSS_SW_OPTION_SMB_IPMC
    // IPMC-IGMP, IPMC-MLD
    return key.is_ipv4 ? 0 : 1;
#  else
    // IPMC-IGMP
    return 0;
#  endif
# endif
#else
    if (!key.is_mvr) {
        T_E("IPMC requested, but IPMC not included in build");
        return 0;
    }
# ifdef VTSS_SW_OPTION_MVR
#  ifdef VTSS_SW_OPTION_SMB_IPMC
    //                      MVR-IGMP, MVR-MLD
    return key.is_ipv4 ? 0 : 1;
#  else
    //                      MVR-IGMP
    if (!key.is_ipv4) {
        T_E("MLD requsted, but MLD not included in build");
    }

    return 0;
#  endif
# else
#  error "Neither IPMC nor MVR is defined"
# endif
#endif
}

/******************************************************************************/
// IPMC_LIB_key_from_index_get()
// Given an index, returns whether it's is_mvr and is_ipv4.
/******************************************************************************/
static vtss_appl_ipmc_lib_key_t IPMC_LIB_key_from_index_get(int index)
{
    vtss_appl_ipmc_lib_key_t key;

    if (index >= IPMC_LIB_STATE_CNT) {
        T_E("index = %u is greater than number of states (%u)", index, IPMC_LIB_STATE_CNT);
        key.is_mvr  = false;
        key.is_ipv4 = false;
        return key;
    }

#ifdef VTSS_SW_OPTION_IPMC
# ifdef VTSS_SW_OPTION_MVR
#  ifdef VTSS_SW_OPTION_SMB_IPMC
    // IPMC-IGMP, IPMC-MLD, MVR-IGMP, MVR-MLD
    key.is_mvr  = index == 2 || index == 3;
    key.is_ipv4 = index == 0 || index == 2;
#  else
    // IPMC-IGMP,           MVR-IGMP
    key.is_mvr  = index == 1;
    key.is_ipv4 = true;
#  endif
# else
#  ifdef VTSS_SW_OPTION_SMB_IPMC
    // IPMC-IGMP, IPMC-MLD
    key.is_mvr  = false;
    key.is_ipv4 = index == 0;
#  else
    // IPMC-IGMP
    key.is_mvr  = false;
    key.is_ipv4 = true;
#  endif
# endif
#else
# ifdef VTSS_SW_OPTION_MVR
#  ifdef VTSS_SW_OPTION_SMB_IPMC
    //                      MVR-IGMP, MVR-MLD
    key.is_mvr  = true;
    key.is_ipv4 = index == 0;
#  else
    //                      MVR-IGMP
    key.is_mvr  = true;
    key.is_ipv4 = true;
#  endif
# else
#  error "Neither IPMC nor MVR is defined"
# endif
#endif

    return key;
}

/******************************************************************************/
// IPMC_LIB_is_mvr_to_idx_get()
/******************************************************************************/
static int IPMC_LIB_is_mvr_to_idx_get(bool is_mvr)
{
#ifdef VTSS_SW_OPTION_IPMC
# ifdef VTSS_SW_OPTION_MVR
    // IPMC MVR
    return is_mvr ? 1 : 0;
# else
    // IPMC
    return 0;
# endif
#else
# ifdef VTSS_SW_OPTION_MVR
    //      MVR
    return 0;
# else
# error "Neither IPMC nor MVR is defined."
# endif
#endif
}

/******************************************************************************/
// IPMC_LIB_rx_bip_buf_init()
/******************************************************************************/
static void IPMC_LIB_rx_bip_buf_init(void)
{
    // Make room for ~100 frames in the Rx BIP buffer
    const int bip_buf_size = 100 * IPMC_LIB_PDU_FRAME_SIZE_MAX;

    T_DG(IPMC_LIB_TRACE_GRP_BIP, "vtss_bip_buffer_init(%u bytes)", bip_buf_size);
    if (!vtss_bip_buffer_init(&IPMC_LIB_rx_bip_buffer, bip_buf_size)) {
        T_EG(IPMC_LIB_TRACE_GRP_BIP, "Unable to allocate a BIP buffer of %u bytes", bip_buf_size);
    }
}

/******************************************************************************/
// IPMC_LIB_rx_packet_dispatch()
/******************************************************************************/
static void IPMC_LIB_rx_packet_dispatch(const uint8_t *frm, const mesa_packet_rx_info_t *rx_info)
{
    ipmc_lib_vlan_itr_t           vlan_itr;
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    mesa_packet_frame_info_t      frame_info;
    mesa_packet_filter_t          filter;
    mesa_port_list_t              vlan_member_port_list, dst_port_mask;
    mesa_etype_t                  etype;
    bool                          is_ipv4, count_as_rx_error = false, handled = false;
    mesa_rc                       rc;
#ifdef VTSS_SW_OPTION_MVR
    ipmc_lib_profile_match_t      profile_match;
    int                           rec_cnt;
    bool                          permit_found;
#endif

    // When this function floods a frame, we call ipmc_lib_pdu_tx() with a
    // destination port set of all-ones. The function takes care of only Tx'ing
    // to members of the VLAN, and not the port on which it was received, and
    // not to ports that are not up or are blocked by other protocols.
    dst_port_mask.set_all();

    // Early IPv4/IPv6 detection to find whether any callbacks are active.
    // As long as we can't receive a frame behind two tags, the frame is always
    // normalized, so that the ethertype comes at frm[12] and frm[13], because
    // the packet module strips the outer tag.
    etype = (frm[12] << 8) | frm[13];

    if (etype == ETYPE_IPV4) {
        is_ipv4 = true;
    } else if (etype == ETYPE_IPV6) {
        is_ipv4 = false;
    } else {
        // Should not be possible here, hence T_EG()
        T_EG(IPMC_LIB_TRACE_GRP_RX, "Invalid EtherType (0x%04x)", etype);
        is_ipv4 = false; // Satisfy compiler
        goto discard;
    }

    // Ingress filter check. Only allow this frame if the port is a member of
    // the VLAN ID.
    if ((rc = mesa_vlan_port_members_get(nullptr, rx_info->tag.vid, &vlan_member_port_list)) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_API, "mesa_vlan_port_members_get(%u) failed: %s. Flooding", rx_info->tag.vid, error_txt(rc));
        T_EG(IPMC_LIB_TRACE_GRP_RX, "rx_info = %s", rx_info);

        // RBNTBD: Remove the +/- 36 once we have the IFH.
        T_EG_HEX(IPMC_LIB_TRACE_GRP_RX, frm - 36, rx_info->length + 36);
        goto flood;
    }

    if (!vlan_member_port_list[rx_info->port_no]) {
        T_IG(IPMC_LIB_TRACE_GRP_RX, "Rx PDU on port %u, but the port is not a member of the VLAN (%u). Discarding", rx_info->port_no, rx_info->tag.vid);
        goto discard;
    }

    // Check that the port is not blocked by STP (or other protocols)
    vtss_clear(frame_info);
    frame_info.port_no = rx_info->port_no;
    frame_info.vid     = rx_info->tag.vid;
    frame_info.port_tx = MESA_PORT_NO_NONE;
    if ((rc = mesa_packet_frame_filter(nullptr, &frame_info, &filter)) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_API, "mesa_packet_frame_filter(port_no = %u, vid = %u) failed: %s. Flooding", rx_info->port_no, rx_info->tag.vid, error_txt(rc));
        goto flood;
    }

    if (filter == MESA_PACKET_FILTER_DISCARD) {
        T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "Ingress port is blocked by STP or some other protocol. Discarding");
        goto discard;
    }

    // Parse the PDU. This needs not be IPMC_LIB_LOCK_SCOPE() protected, since
    // it's not using anything else than IPMC_LIB_rx_pdu, which only can be
    // touched by this function, which is always called from the same thread.
    switch (ipmc_lib_pdu_rx_parse(frm, *rx_info, IPMC_LIB_rx_pdu)) {
    case IPMC_LIB_PDU_RX_ACTION_PROCESS:
        // No errors. Handle the frame
        T_DG(IPMC_LIB_TRACE_GRP_RX, "%s", IPMC_LIB_rx_pdu); // Using ipmc_lib_pdu_t::operator<<
        break;

    case IPMC_LIB_PDU_RX_ACTION_DISCARD:
        // Something seriously was wrong with this frame. Discard it.
        T_IG_HEX(IPMC_LIB_TRACE_GRP_RX, frm, rx_info->length);
        goto discard;

    case IPMC_LIB_PDU_RX_ACTION_FLOOD:
        // Something not so serious was wrong with this frame, but serious
        // enough to not let us process it. Flood it.

        // This happens pretty often, so only using trace debug here.
        T_DG_HEX(IPMC_LIB_TRACE_GRP_RX, frm, rx_info->length);
        count_as_rx_error = true;
        break;
    }

    {
        IPMC_LIB_LOCK_SCOPE();

        // If the port doesn't have link, discard the PDU without counting it
        // anywhere.
        if (!IPMC_LIB_ports_with_link[rx_info->port_no]) {
            return;
        }

#ifdef VTSS_SW_OPTION_MVR
        // Go through all MVR VLANs first, because they are all potential
        // receivers of this PDU.
        // However, at most one MVR VLAN will actually receive it.
        // The way this is handled for reports is through profile matching: Two
        // MVR VLANs cannot have overlapping permit rules, so they won't be able
        // both to get them. If they had overlapping permit rules, they would be
        // set operationally inactive, and therefore never get so far as to
        // check the rules. Let me rephrase this slightly:
        //   If receiving an IGMPv3 or MLDv2 report with two groups in it and
        //   one matches a profile in MVR VLAN #1 and another one matches a
        //   profile in MVR VLAN #2, they will both receive the PDU. Whether
        //   this is actually correct, I don't know, but it will be forwarded to
        //   all source ports in both MVR VLANs, where in fact, only the groups
        //   that are permitted should be sent (proxied).
        // The way it is handled for queries is that the port first of all must
        // be a source port and the classified VLAN ID must in that case match
        // that of the MVR VLAN.
        for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
            if (!vlan_itr->first.is_mvr) {
                // This is an IPMC VLAN. Go on.
                continue;
            }

            if (vlan_itr->first.is_ipv4 != is_ipv4) {
                // This is an IGMP VLAN receiving an MLD PDU or vice versa. Go on.
                continue;
            }

            if (vlan_itr->second.status.oper_state != VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
                // This MVR VLAN is not operationally active. Go on.
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Ignoring, because this MVR VLAN is inactive", vlan_itr->first);
                continue;
            }

            if (vlan_itr->second.port_conf[rx_info->port_no].role == VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE) {
                // Neither a source nor a receiver port. Go on.
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Ignoring, because PDU is received on port that is neither source nor receiver", vlan_itr->first);
                continue;
            }

            if (vlan_itr->second.port_conf[rx_info->port_no].role == VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE && vlan_itr->first.vid != rx_info->tag.vid) {
                // Ignore frames received on source ports that are not
                // classified to the MVR VLAN.
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Ignoring, because received on a source port but is classified to %u", vlan_itr->first, rx_info->tag.vid);
                continue;
            }

            if (count_as_rx_error) {
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Ignoring, because PDU was erroneous", vlan_itr->first);
                vlan_itr->second.statistics.rx_errors++;
                continue;
            }

            if (IPMC_LIB_rx_pdu.type == IPMC_LIB_PDU_TYPE_REPORT) {
                if (vlan_itr->second.conf.compatible_mode && vlan_itr->second.port_conf[rx_info->port_no].role == VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE) {
                    // In compatible mode, we discard reports received on source
                    // ports. But let's count it as ignored.
                    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Ignoring, because received on a source port in compatible mode", vlan_itr->first);
                    ipmc_lib_pdu_statistics_update(vlan_itr->second, IPMC_LIB_rx_pdu, true /* is Rx */, true /* count as ignored */);
                    continue;
                }

                // We need to go through all groups to see if at least one group is
                // matching a permit rule for this MVR VLAN. If not, we don't pass
                // this vlan_state to ipmc_lib_base_rx_pdu().
                profile_match.profile_key = vlan_itr->second.conf.channel_profile;
                profile_match.vid         = vlan_itr->first.vid;
                profile_match.port_no     = rx_info->port_no;
                profile_match.src         = IPMC_LIB_rx_pdu.sip;

                // Check if any group is permitted within this report.
                permit_found = false;
                for (rec_cnt = 0; rec_cnt < IPMC_LIB_rx_pdu.report.rec_cnt; rec_cnt++) {
                    const ipmc_lib_pdu_group_record_t &grp_rec = IPMC_LIB_rx_pdu.report.group_recs[rec_cnt];

                    if (!grp_rec.valid) {
                        continue;
                    }

                    // Check if we should filter out this group address
                    profile_match.dst = grp_rec.grp_addr;

                    if (ipmc_lib_profile_permit(vlan_itr->second, profile_match, false /* Don't log if we run into an entry with logging enabled. IPMC_LIB_BASE_rx_report() will do that if we get that far */)) {
                        permit_found = true;
                        break;
                    }
                }

                if (!permit_found) {
                    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Ignoring, because no rules match", vlan_itr->first);
                    continue;
                }
            } else {
                // Query.

                // Must be received on source port.
                if (vlan_itr->second.port_conf[rx_info->port_no].role != VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE) {
                    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Ignoring, because the query is not received on a source port", vlan_itr->first);

                    // Count this as ignored.
                    ipmc_lib_pdu_statistics_update(vlan_itr->second, IPMC_LIB_rx_pdu, true /* is Rx */, true /* count as ignored */);
                    continue;
                }
            }

            // Still here? Let this MVR VLAN handle the PDU.
            T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Handling %s %s", vlan_itr->first, ipmc_lib_pdu_version_to_str(IPMC_LIB_rx_pdu.version), ipmc_lib_pdu_type_to_str(IPMC_LIB_rx_pdu.type));
            ipmc_lib_base_rx_pdu(vlan_itr->second, IPMC_LIB_rx_pdu);

            handled = true;
        }
#endif

        if (!handled) {
            // See if IPMC wants it.
            vlan_key.vid = rx_info->tag.vid;
            vlan_key.is_mvr = false;
            vlan_key.is_ipv4 = is_ipv4;

            if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) != IPMC_LIB_vlan_map.end()) {
                if (vlan_itr->second.status.oper_state != VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
                    // The instance that should have handled it is not active.
                    // Flood it.
                    goto flood;
                }

                if (count_as_rx_error) {
                    vlan_itr->second.statistics.rx_errors++;
                } else {
                    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, rx_info->port_no, "%s: Handling %s %s", vlan_itr->first, ipmc_lib_pdu_version_to_str(IPMC_LIB_rx_pdu.version), ipmc_lib_pdu_type_to_str(IPMC_LIB_rx_pdu.type));
                    ipmc_lib_base_rx_pdu(vlan_itr->second, IPMC_LIB_rx_pdu);
                    handled = true;
                }
            }
        }

        if (handled) {
            // ipmc_lib_base_rx_pdu() handled forwarding of the PDU.
            return;
        }
    }

flood:
    // If we get here, no one handled the PDU. Flood it.
    T_IG(IPMC_LIB_TRACE_GRP_RX, "Neither MVR nor IPMC wants the frame. Flooding in VLAN %u", rx_info->tag.vid);

    (void)ipmc_lib_pdu_tx(frm, rx_info->length, dst_port_mask, false, rx_info->port_no, rx_info->tag.vid, rx_info->tag.pcp, rx_info->tag.dei);
    return;

discard:
    T_DG_HEX(IPMC_LIB_TRACE_GRP_RX, frm, rx_info->length);
}

/******************************************************************************/
// IPMC_LIB_rx_packet_from_packet_module()
/******************************************************************************/
static BOOL IPMC_LIB_rx_packet_from_packet_module(void *contxt, const uint8_t *frm, const mesa_packet_rx_info_t *rx_info)
{
    uint8_t  *buf;
    uint32_t aligned_rx_info_len_bytes, aligned_frm_len_bytes, aligned_sum;

    aligned_rx_info_len_bytes = sizeof(int) * ((sizeof(mesa_packet_rx_info_t) + 3) / sizeof(int));
    aligned_frm_len_bytes     = sizeof(int) * ((rx_info->length               + 3) / sizeof(int));
    aligned_sum               = aligned_rx_info_len_bytes + aligned_frm_len_bytes;

    // We put it through a BIP buffer (a queue) in order not to disturb other
    // frames received on this priority too much and in order for keeping the
    // amount of frame data left to process to a certain maximum size - the
    // size of the BIP buffer.
    // IPMC_LIB_rx_packet_from_bip_buffer() takes care of reading the BIP buffer
    // whenever there's somehting in it. It does so from our own thread called
    // IPMC_LIB_rx_thread().
    IPMC_LIB_PDU_LOCK_SCOPE();

    if (rx_info->length > IPMC_LIB_PDU_FRAME_SIZE_MAX) {
        T_DG(IPMC_LIB_TRACE_GRP_BIP, "Got frame of %u bytes, but we only support %u bytes. Discarding", rx_info->length, IPMC_LIB_PDU_FRAME_SIZE_MAX);
        return true;
    }

    T_DG(IPMC_LIB_TRACE_GRP_BIP, "vtss_bip_buffer_reserve(%u + %u = %u)", aligned_rx_info_len_bytes, aligned_frm_len_bytes, aligned_sum);
    if ((buf = vtss_bip_buffer_reserve(&IPMC_LIB_rx_bip_buffer, aligned_sum)) == nullptr) {
        // This is quite normal if there's hard pressure on the system
        T_DG(IPMC_LIB_TRACE_GRP_BIP, "vtss_bip_buffer_reserve(%u + %u = %u) failed", aligned_rx_info_len_bytes, aligned_frm_len_bytes, aligned_sum);

        // If there had been room in the buffer, we would have consumed it, so
        // also pretend we did that now.
        return true;
    }

    if ((uint64_t)buf & 0x3) {
        T_EG(IPMC_LIB_TRACE_GRP_BIP, "BIP buffer (%p) not correctly aligned", buf);
        return true;
    }

    memcpy(&buf[0],                         rx_info, sizeof(mesa_packet_rx_info_t));
    memcpy(&buf[aligned_rx_info_len_bytes], frm,     rx_info->length);

    vtss_bip_buffer_commit(&IPMC_LIB_rx_bip_buffer);

    // Kick-start the thread.
    vtss_flag_setbits(&IPMC_LIB_rx_flag, 0x1 /* any flag value will do */);

    return true; // Do not allow other subscribers to receive the packet
}

/******************************************************************************/
// IPMC_LIB_rx_packet_from_bip_buffer()
/******************************************************************************/
static bool IPMC_LIB_rx_packet_from_bip_buffer(uint8_t **frm, mesa_packet_rx_info_t **rx_info, int *sz)
{
    uint8_t  *buf;
    int      buf_size;
    uint32_t aligned_rx_info_len_bytes;
    uint32_t aligned_frm_len_bytes;

    IPMC_LIB_PDU_LOCK_SCOPE();

    // The producer may produce frames faster than the consumer can take them
    // off. vtss_bip_buffer_get_contiguous_block() will in that case return a
    // pointer to all frames not currently consumed.
    // Since we can consume only one frame at a time, and since we use zero-copy
    // (work with the frame in the bip buffer itself without copying it to a
    // separate buffer), we consume only one frame and decommit only that frame
    // per time we are called. The size of that one frame is returned in "sz".
    if ((buf = vtss_bip_buffer_get_contiguous_block(&IPMC_LIB_rx_bip_buffer, &buf_size)) == NULL) {
        // No more frames.
        return false;
    }

    if ((uint64_t)buf & 0x3) {
        T_EG(IPMC_LIB_TRACE_GRP_BIP, "BIP buffer (%p) of size %d not correctly aligned", buf, buf_size);
        return false;
    }

    aligned_rx_info_len_bytes = sizeof(int) * ((sizeof(**rx_info) + 3) / sizeof(int));
    *rx_info = (mesa_packet_rx_info_t *)&buf[0];
    *frm = &buf[aligned_rx_info_len_bytes];

    aligned_frm_len_bytes = sizeof(int) * (((*rx_info)->length + 3) / sizeof(int));
    *sz  = aligned_rx_info_len_bytes + aligned_frm_len_bytes;

    if (*sz > buf_size) {
        T_EG(IPMC_LIB_TRACE_GRP_BIP, "We got a frame that we think is %u bytes long, but the BIP buffer only returned %u bytes", *sz, buf_size);
    }

    if ((*rx_info)->length > IPMC_LIB_PDU_FRAME_SIZE_MAX) {
        T_EG(IPMC_LIB_TRACE_GRP_BIP, "We are about to consume a frame of %u bytes, but the producer has promised not to send frames longer than than %u bytes to us", (*rx_info)->length, IPMC_LIB_PDU_FRAME_SIZE_MAX);
    }

    T_DG(IPMC_LIB_TRACE_GRP_BIP, "vtss_bip_buffer_get_contiguous_block() returned %u bytes. We are using %u + %u = %u bytes this time", buf_size, aligned_rx_info_len_bytes, aligned_frm_len_bytes, *sz);

    // Only one frame at a time. The caller will decommit sz bytes once done.
    return true;
}

/******************************************************************************/
// IPMC_LIB_rx_bip_buffer_decommit()
/******************************************************************************/
static void IPMC_LIB_rx_bip_buffer_decommit(int sz)
{
    IPMC_LIB_PDU_LOCK_SCOPE();
    T_DG(IPMC_LIB_TRACE_GRP_BIP, "vtss_bip_buffer_decommit_block(sz = %u)", sz);
    vtss_bip_buffer_decommit_block(&IPMC_LIB_rx_bip_buffer, sz);
}

/******************************************************************************/
// IPMC_LIB_rx_register_update_mesa()
/******************************************************************************/
static void IPMC_LIB_rx_register_update_mesa(bool is_ipv4, bool enable)
{
    mesa_packet_rx_conf_t conf;
    mesa_rc               rc;

    // mesa_packet_rx_conf_get()/set() must be called without interference.
    VTSS_APPL_API_LOCK_SCOPE();

    T_IG(IPMC_LIB_TRACE_GRP_API, "mesa_packet_rx_conf_get()");
    if ((rc = mesa_packet_rx_conf_get(nullptr, &conf)) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_API, "mesa_packet_rx_conf_get() failed: %s", error_txt(rc));
        return;
    }

    // If disabling, IGMP/MLD frames will be flooded in VLAN.
    if (is_ipv4) {
        conf.reg.igmp_cpu_only = enable;
    } else {
        conf.reg.mld_cpu_only  = enable;
    }

    T_IG(IPMC_LIB_TRACE_GRP_API, "mesa_packet_rx_conf_set(is_ipv4 = %d, enable = %d)", is_ipv4, enable);
    if ((rc = mesa_packet_rx_conf_set(nullptr, &conf)) != VTSS_RC_OK) {
        T_EG(IPMC_LIB_TRACE_GRP_API, "mesa_packet_rx_conf_set(is_ipv4 = %d, enable = %d) failed: %s", is_ipv4, enable, error_txt(rc));
    }
}

/******************************************************************************/
// IPMC_LIB_rx_register_update_packet()
/******************************************************************************/
static void IPMC_LIB_rx_register_update_packet(bool is_ipv4, bool enable)
{
    static void        *filter_id[IPMC_LIB_PROTOCOL_CNT];
    packet_rx_filter_t filter;
    int                idx = is_ipv4 ? 0 : 1;
    const char         *proto_txt = is_ipv4 ? "IGMP" : "MLD";
    mesa_rc            rc;

    if (enable) {
        // Register for IGMP or MLD PDUs in the packet module
        packet_rx_filter_init(&filter);

        filter.cb    = IPMC_LIB_rx_packet_from_packet_module;
        filter.modid = VTSS_MODULE_ID_IPMC_LIB;
        filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;

        if (is_ipv4) {
            filter.match    = PACKET_RX_FILTER_MATCH_IP_PROTO | PACKET_RX_FILTER_MATCH_ETYPE;
            filter.etype    = ETYPE_IPV4;
            filter.ip_proto = IPMC_LIB_PDU_IGMP_PROTOCOL_ID;
        } else {
            filter.match = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_ETYPE;
            filter.etype = ETYPE_IPV6;

            memset(&filter.dmac_mask[0], 0xFF, sizeof(filter.dmac_mask));
            filter.dmac[0]      = 0x33; /* 0x3333 */
            filter.dmac_mask[0] = 0x0;
            filter.dmac[1]      = 0x33; /* 0x3333 */
            filter.dmac_mask[1] = 0x0;
        }

        if (!filter_id[idx]) {
            T_DG(IPMC_LIB_TRACE_GRP_RX, "%s: packet_rx_filter_register()", proto_txt);
            if ((rc = packet_rx_filter_register(&filter, &filter_id[idx])) != VTSS_RC_OK) {
                T_EG(IPMC_LIB_TRACE_GRP_RX, "%s: packet_rx_filter_register() failed: %s", proto_txt, error_txt(rc));
            }
        } else {
            T_EG(IPMC_LIB_TRACE_GRP_RX, "Internal error: %s: filter_id is not null", proto_txt);
        }
    } else {
        // Unregister for IGMP/MLD PDUs in the packet module
        if (filter_id[idx]) {
            T_DG(IPMC_LIB_TRACE_GRP_RX, "%s: packet_rx_filter_unregister()", proto_txt);
            if ((rc = packet_rx_filter_unregister(filter_id[idx])) != VTSS_RC_OK) {
                T_EG(IPMC_LIB_TRACE_GRP_RX, "%s: packet_rx_filter_unregister() failed: %s", proto_txt, error_txt(rc));
            }

            filter_id[idx] = nullptr;
        } else {
            T_EG(IPMC_LIB_TRACE_GRP_RX, "Internal error: %s: filter_id is already null", proto_txt);
        }
    }
}

/******************************************************************************/
// IPMC_LIB_rx_thread()
/******************************************************************************/
static void IPMC_LIB_rx_thread(vtss_addrword_t data)
{
    mesa_packet_rx_info_t *rx_info;
    uint8_t               *frm;
    int                   sz;

    while (1) {
        vtss_flag_wait(&IPMC_LIB_rx_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR);

        // Empty the BIP buffer before waiting again.
        while (IPMC_LIB_rx_packet_from_bip_buffer(&frm, &rx_info, &sz)) {
            IPMC_LIB_rx_packet_dispatch(frm, rx_info);
            IPMC_LIB_rx_bip_buffer_decommit(sz);
        }
    }
}

/******************************************************************************/
// IPMC_LIB_ptr_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_IPMC_LIB_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// ipmc_lib_vlan_check()
/******************************************************************************/
mesa_rc ipmc_lib_vlan_check(mesa_vid_t vid)
{
    if (vid < VTSS_APPL_VLAN_ID_MIN || vid > VTSS_APPL_VLAN_ID_MAX) {
        T_D("VID (%u) out of range ([%u; %u])", vid, VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX);
        return VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_ID;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_port_no_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_port_no_check(mesa_port_no_t port_no)
{
    if (port_no >= IPMC_LIB_port_cnt) {
        T_D("port_no (%u) is >= number of ports (%u)", port_no, IPMC_LIB_port_cnt);
        return VTSS_APPL_IPMC_LIB_RC_INVALID_PORT_NUMBER;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_prefix_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_prefix_check(const vtss_appl_ipmc_lib_ip_t &prefix_addr, uint32_t prefix_len)
{
    vtss_appl_ipmc_lib_ip_t prefix_mask;

    if (!prefix_addr.is_mc()) {
        // Not an IP multicast address
        return VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_NOT_MC;
    }

    // Check prefix length
    if (prefix_addr.is_ipv4) {
        if (prefix_len < IPMC_LIB_ADDR_V4_MIN_BIT_LEN || prefix_len > IPMC_LIB_ADDR_V4_MAX_BIT_LEN) {
            return VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_LEN_INVALID;
        }
    } else {
        if (prefix_len < IPMC_LIB_ADDR_V6_MIN_BIT_LEN || prefix_len > IPMC_LIB_ADDR_V6_MAX_BIT_LEN) {
            return VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_LEN_INVALID;
        }
    }

    prefix_mask.is_ipv4 = prefix_addr.is_ipv4;
    if (prefix_addr.is_ipv4) {
        prefix_mask.ipv4 = vtss_ipv4_prefix_to_mask(prefix_len);
    } else {
        if (vtss_conv_prefix_to_ipv6mask(prefix_len, &prefix_mask.ipv6) != VTSS_RC_OK) {
            // Something went wrong.
            T_E("Unable to convert prefix-length = %u to a mask", prefix_len);
            return VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR;
        }
    }

    T_D("prefix_addr = %s, prefix_len = %u => prefix_mask = %s, ~prefix_mask = %s, result = %d", prefix_addr, prefix_len, prefix_mask, ~prefix_mask, (prefix_addr & ~prefix_mask).is_zero());

    // Match on the least significant bits by inverting the prefix mask. They
    // must all be zeros.
    if (!(prefix_addr & ~prefix_mask).is_zero()) {
        return VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_BITS_OUTSIDE_OF_MASK_SET;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_user_supported_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_user_supported_check(bool is_mvr)
{
    if (is_mvr) {
#ifdef VTSS_SW_OPTION_MVR
        return VTSS_RC_OK;
#else
        return VTSS_APPL_IPMC_LIB_RC_MVR_NOT_SUPPORTED;
#endif
    } else {
#ifdef VTSS_SW_OPTION_IPMC
        return VTSS_RC_OK;
#else
        return VTSS_APPL_IPMC_LIB_RC_IPMC_NOT_SUPPORTED;
#endif
    }
}

/******************************************************************************/
// IPMC_LIB_ip_family_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_ip_family_check(bool is_ipv4)
{
    if (is_ipv4) {
        // IGMP is always supported
        return VTSS_RC_OK;
    }

    return IPMC_LIB_cap.mld_support ? VTSS_RC_OK : (mesa_rc)VTSS_APPL_IPMC_LIB_RC_MLD_NOT_SUPPORTED;
}

/******************************************************************************/
// IPMC_LIB_is_mvr_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_is_mvr_check(bool is_mvr)
{
    return is_mvr ? VTSS_RC_OK : (mesa_rc)VTSS_APPL_IPMC_LIB_RC_FUNCTION_ONLY_SUPPORTED_BY_MVR;
}

/******************************************************************************/
// IPMC_LIB_key_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_key_check(const vtss_appl_ipmc_lib_key_t &key)
{
    VTSS_RC(IPMC_LIB_user_supported_check(key.is_mvr));
    return IPMC_LIB_ip_family_check(key.is_ipv4);
}

/******************************************************************************/
// IPMC_LIB_key_mvr_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_key_mvr_check(const vtss_appl_ipmc_lib_key_t &key)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    return IPMC_LIB_is_mvr_check(key.is_mvr);
}

/******************************************************************************/
// IPMC_LIB_vlan_key_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_vlan_key_check(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key)
{
    VTSS_RC(IPMC_LIB_key_check(static_cast<vtss_appl_ipmc_lib_key_t>(vlan_key)));
    return ipmc_lib_vlan_check(vlan_key.vid);
}

/******************************************************************************/
// IPMC_LIB_vlan_key_mvr_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_vlan_key_mvr_check(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key)
{
    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));
    return IPMC_LIB_is_mvr_check(vlan_key.is_mvr);
}

/******************************************************************************/
// IPMC_LIB_name_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_name_check(const char *name, size_t *the_len = nullptr)
{
    size_t len, i;

    len = strlen(name);

    if (the_len) {
        *the_len = len;
    }

    if (len == 0) {
        // An empty name is fine.
        return VTSS_RC_OK;
    }

    if (len > VTSS_APPL_IPMC_LIB_VLAN_NAME_LEN_MAX) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_LENGTH;
    }

    // First character must be [a-zA-Z]
    if (!isalpha(name[0])) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS;
    }

    for (i = 1; i < len; i++) {
        if (!isgraph(name[i])) {
            return VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS;
        }

        if (name[i] == ':') {
            return VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS_COLON;
        }
    }

    // "all" is reserved in ICLI. Using case-insensitive match, because ICLI is
    // case-insensitive.
    if (strcasecmp(name, "all") == 0) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS_ALL;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_ifindex_to_vlan()
/******************************************************************************/
static mesa_rc IPMC_LIB_ifindex_to_vlan(vtss_ifindex_t ifindex, mesa_vid_t &vid)
{
    vtss_ifindex_elm_t ife;

    // Check that we can decompose the ifindex and that it's a port.
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        vid = MESA_VID_NULL;
        return VTSS_RC_ERROR;
    }

    vid = ife.ordinal;
    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_rx_register_update()
/******************************************************************************/
static void IPMC_LIB_rx_register_update(void)
{
    static bool              ipv4_was_enabled, ipv6_was_enabled;
    bool                     ipv4_enable, ipv6_enable;
    vtss_appl_ipmc_lib_key_t key;
    int                      i;

    ipv4_enable = false;
    ipv6_enable = false;
    for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
        if (IPMC_LIB_global_state[i].conf.admin_active) {
            key = IPMC_LIB_key_from_index_get(i);
            if (key.is_ipv4) {
                ipv4_enable = true;
            } else {
                ipv6_enable = true;
            }
        }
    }

    T_IG(IPMC_LIB_TRACE_GRP_RX, "ipv4_enable = %d, ipv4_was_enabled = %d, ipv6_enable = %d, ipv6_was_enabled = %d", ipv4_enable, ipv4_was_enabled, ipv4_enable, ipv6_was_enabled);

    if (ipv4_enable == ipv4_was_enabled && ipv6_enable == ipv6_was_enabled) {
        // Nothing to do.
        T_IG(IPMC_LIB_TRACE_GRP_RX, "No change in registration (ipv4 = %d, ipv6 = %d)", ipv4_enable, ipv6_enable);
        return;
    }

    if (ipv4_enable != ipv4_was_enabled) {
        // Register or unregister for frames to the CPU in MESA
        IPMC_LIB_rx_register_update_mesa(true, ipv4_enable);

        // Register or unregister for frames to us in the packet module
        IPMC_LIB_rx_register_update_packet(true, ipv4_enable);

        ipv4_was_enabled = ipv4_enable;
    }

    if (ipv6_enable != ipv6_was_enabled) {
        // Register or unregister for frames to the CPU in MESA
        IPMC_LIB_rx_register_update_mesa(false, ipv6_enable);

        // Register or unregister for frames to us in the packet module
        IPMC_LIB_rx_register_update_packet(false, ipv6_enable);

        ipv6_was_enabled = ipv6_enable;
    }

    if (!ipv4_enable && !ipv6_enable) {
        // Clear the BIP buffer.
        // It's safe to take the mutex that protects the BIP buffer, even though
        // IPMC_LIB_LOCK_SCOPE() mutex is also taken, because the BIP buffer
        // mutex is a leaf mutex that doesn't require other mutexes to be taken
        // while it's taken.
        T_IG(IPMC_LIB_TRACE_GRP_BIP, "Clearing BIP buffer, because all protocols are globally disabled");
        IPMC_LIB_PDU_LOCK_SCOPE();
        vtss_bip_buffer_clear(&IPMC_LIB_rx_bip_buffer);
    }
}

/******************************************************************************/
// IPMC_LIB_tick_thread()
// Keeps ticks alive.
/******************************************************************************/
static void IPMC_LIB_tick_thread(vtss_addrword_t data)
{
    ipmc_lib_vlan_itr_t vlan_itr;
    bool                at_least_one_globally_enabled;
    uint32_t            now;
    int                 i;

    // Wait until all configuration has been applied.
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_IPMC_LIB);

    IPMC_LIB_thread_running = true;

    T_D("Entering endless while-loop");
    while (1) {
        VTSS_OS_MSLEEP(1000); // One tick is one second.
        if (!IPMC_LIB_thread_running) {
            // We are currently suspended.
            continue;
        }

        IPMC_LIB_LOCK_SCOPE();

        at_least_one_globally_enabled = false;
        for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
            if (IPMC_LIB_global_state[i].conf.admin_active) {
                at_least_one_globally_enabled = true;
                break;
            }
        }

        if (!at_least_one_globally_enabled) {
            continue;
        }

        now = vtss::uptime_seconds();

        // Group map tick - one single that covers all
        ipmc_lib_base_grp_map_tick(IPMC_LIB_global_lists.grp_map, now);

        // Proxy group map tick - one single that covers all
        ipmc_lib_base_proxy_report_tick(IPMC_LIB_global_lists.proxy_grp_map);

        // Per-VLAN ticks
        for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
            ipmc_lib_vlan_state_t &vlan_state = vlan_itr->second;

            if (vlan_state.status.oper_state != VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
                continue;
            }

            ipmc_lib_base_vlan_tick(vlan_state, now);
        }

        for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
            ipmc_lib_global_state_t &glb = IPMC_LIB_global_state[i];
            if (glb.conf.admin_active) {
                // Called once per tick per IPMC/MVR/IGMP/MLD
                ipmc_lib_base_global_tick(glb);
            }
        }
    }
}

/******************************************************************************/
// IPMC_LIB_capabilities_set()
// This is the only place where we define maximum values for various parameters,
// so if you need different max. values, change here - only!
/******************************************************************************/
static void IPMC_LIB_capabilities_set(void)
{
    uint32_t chip_family = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);

    IPMC_LIB_cap.igmp_support = true;

#ifdef VTSS_SW_OPTION_SMB_IPMC
    IPMC_LIB_cap.mld_support = true;
#else
    IPMC_LIB_cap.mld_support = false;
#endif

    // Does the chip support Source Specific Multicast Forwarding for IPv4?
    IPMC_LIB_cap.ssm_chip_support_ipv4 = fast_cap(MESA_CAP_L2_IPV4_MC_SIP);

    // Does the chip support Source Specific Multicast Forwarding for IPv6?
    IPMC_LIB_cap.ssm_chip_support_ipv6 = fast_cap(MESA_CAP_L2_IPV6_MC_SIP);

    // Unfortunately, there is no MESA capabilities for the number of IPv4 or
    // IPv6 groups we can create, so the following is found by looking at
    // the output of 'debug api ipmc' on various platforms.
    // Moreover, the numbers that can be seen with 'debug api ipmc' are the
    // best-case numbers for IPv4. The resources (IS2) in the chip are shared
    // amongst other features, so there's no guarantee that we can reach this
    // number.
    // The number of IPv6 entries is at best half the number of IPv4 entries.
    switch (chip_family) {
    case MESA_CHIP_FAMILY_CARACAL:
        IPMC_LIB_cap.grp_cnt_max = 2048;
        IPMC_LIB_cap.src_cnt_max =  128;
        break;

    case MESA_CHIP_FAMILY_OCELOT:
    case MESA_CHIP_FAMILY_LAN966X:
        IPMC_LIB_cap.grp_cnt_max =  128;
        IPMC_LIB_cap.src_cnt_max =  128;
        break;

    case MESA_CHIP_FAMILY_SERVALT:
        IPMC_LIB_cap.grp_cnt_max = 1536;
        IPMC_LIB_cap.src_cnt_max = 1536;
        break;

    case MESA_CHIP_FAMILY_JAGUAR2:
        IPMC_LIB_cap.grp_cnt_max = 4096;
        IPMC_LIB_cap.src_cnt_max = 4096;
        break;

    case MESA_CHIP_FAMILY_SPARX5:
    case MESA_CHIP_FAMILY_LAN969X:
        IPMC_LIB_cap.grp_cnt_max = 5120;
        IPMC_LIB_cap.src_cnt_max = 5120;
        break;

    default:
        T_E("Unsupported chip family (%d)", chip_family);
        break;
    }

    // We limit the number of groups that can be created to 1K to prevent the
    // CPU to be overburdened.
    IPMC_LIB_cap.grp_cnt_max = MIN(IPMC_LIB_cap.grp_cnt_max, 1024);

    // We also limit the number of sources that can be created in total to 1K.
    IPMC_LIB_cap.src_cnt_max = MIN(IPMC_LIB_cap.src_cnt_max, 1024);

    // Furthermore, since we always use one zero-SIP entry (the ASM entry), we
    // can have one less source address.
    if (IPMC_LIB_cap.src_cnt_max) {
        IPMC_LIB_cap.src_cnt_max--;
    }

    // We also limit the number of sources that can be maintained for a given
    // group.
    IPMC_LIB_cap.src_per_grp_cnt_max = 8;
}

/******************************************************************************/
// IPMC_LIB_time_abs_to_rel()
// Internally, we use absolute time since boot for a timeout, when when
// reporting status, we change this to be relative from now.
/******************************************************************************/
static uint32_t IPMC_LIB_time_abs_to_rel(uint32_t abs_timeout, uint32_t now)
{
    uint32_t timeout;

    if (abs_timeout) {
        if (now >= abs_timeout) {
            // Pretend there is one second until it times out.
            timeout = 1;
        } else {
            timeout = abs_timeout - now;
        }
    } else {
        timeout = 0;
    }

    return timeout;
}

/******************************************************************************/
// IPMC_LIB_global_conf_changed()
/******************************************************************************/
static void IPMC_LIB_global_conf_changed(const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_global_conf_t &new_conf)
{
    ipmc_lib_vlan_itr_t              vlan_itr;
    ipmc_lib_global_state_t          &glb      = IPMC_LIB_global_state[IPMC_LIB_key_to_idx_get(key)];
    vtss_appl_ipmc_lib_global_conf_t &old_conf = glb.conf;
    bool                             unregistered_flooding_update, router_status_update = false, admin_active_changed;

    T_I("%s: old_conf = %s, new_conf = %s", key, old_conf, new_conf);

    if (!IPMC_LIB_started) {
        // Just save the configuration for later.
        old_conf = new_conf;
        return;
    }

    if (memcmp(&new_conf, &old_conf, sizeof(new_conf)) == 0) {
        // No changes
        return;
    }

    admin_active_changed = new_conf.admin_active != old_conf.admin_active;
    if (admin_active_changed && old_conf.admin_active) {
        // We are deactivating.
        // Go through all VLANs that utilize this global conf and remove entries
        // from the chip.
        // If we are activating, we don't need to do anything.
        for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
            if (vlan_itr->second.global == &glb) {
                // This VLAN is utilizing this global state. Deactivate.
                ipmc_lib_base_deactivate(vlan_itr->second);
            }
        }

        // We also need to reset the dynamic router ports. Since we do this
        // after we have removed all chip-entries, it's fast.
        router_status_update = true;
    }

    // We need to update MESA with unregistered flooding if we are either
    // changing admin_active or changing the unregistered flooding
    // configuration. We cannot do this before we have updated the conf.
    unregistered_flooding_update = admin_active_changed  || new_conf.unregistered_flooding_enable != old_conf.unregistered_flooding_enable;

    // If we are deactivating proxy, we need to clear the current proxy list.
    if (new_conf.proxy_enable != old_conf.proxy_enable && old_conf.proxy_enable) {
        for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
            if (vlan_itr->second.global == &glb) {
                // This VLAN is utilizing this global state.
                ipmc_lib_base_proxy_grp_map_clear(vlan_itr->second);
            }
        }
    }

    // If SSM prefix changes, we should go through existing entries that matches
    // the new prefix and remove them, but since SSM prefix is only used with
    // IGMPv1/IGMPv2/MLDv1 reports and since we don't know which report versions
    // were used to create them, we cannot remove them.

    // Change conf
    old_conf = new_conf;

    // The following ipmc_lib_base_XXX() functions utilize the current
    // configuration, so we have to postpone the calls to them until it gets
    // set.

    if (router_status_update) {
        ipmc_lib_base_router_status_update(glb, MESA_PORT_NO_NONE /* all ports */, true /* dynamic */, false /* remove */);
    }

    if (unregistered_flooding_update) {
        ipmc_lib_base_unregistered_flooding_update(glb);
    }

    if (admin_active_changed) {
        IPMC_LIB_vlan_oper_warnings_update_all();
        IPMC_LIB_vlan_module_membership_update_all();
        IPMC_LIB_vlan_module_port_conf_update_all();
    }

    // Change packet Rx registration (both in MESA and the packet module)
    IPMC_LIB_rx_register_update();
}

/******************************************************************************/
// IPMC_LIB_port_conf_changed()
/******************************************************************************/
static void IPMC_LIB_port_conf_changed(const vtss_appl_ipmc_lib_key_t &key, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_port_conf_t &new_conf)
{
    ipmc_lib_vlan_itr_t            vlan_itr;
    ipmc_lib_global_state_t        &glb      = IPMC_LIB_global_state[IPMC_LIB_key_to_idx_get(key)];
    vtss_appl_ipmc_lib_port_conf_t &old_conf = glb.port_conf[port_no];
    bool                           grp_cnt_max_update, router_status_update, profile_key_changed;

    T_I_PORT(port_no, "old_conf = %s, new_conf = %s", old_conf, new_conf);

    if (!IPMC_LIB_started) {
        // Just save the configuration for later.
        old_conf = new_conf;
        return;
    }

    if (memcmp(&new_conf, &old_conf, sizeof(new_conf)) == 0) {
        // No changes.
        return;
    }

    grp_cnt_max_update = false;
    if (new_conf.grp_cnt_max) {
        // Trottling is now enabled.
        if (old_conf.grp_cnt_max) {
            // It was also enabled before.
            if (new_conf.grp_cnt_max < old_conf.grp_cnt_max) {
                // The maximum has been reduced.
                grp_cnt_max_update = true;
            }
        } else {
            // It was not enabled before, so remove surplus groups
            grp_cnt_max_update = true;
        }
    } else {
        // No limit on number of groups that can be registered. Nothing to do.
    }

    router_status_update = new_conf.router != old_conf.router;

    profile_key_changed = strcmp(new_conf.profile_key.name, old_conf.profile_key.name) != 0;

    old_conf = new_conf;

    // The following ipmc_lib_base_XXX() functions utilize the current
    // configuration, so we have to postpone the calls to them until it gets
    // set.

    if (grp_cnt_max_update) {
        ipmc_lib_base_grp_cnt_max_update(glb, port_no);
    }

    if (router_status_update) {
        ipmc_lib_base_router_status_update(glb, port_no, false /* static */, new_conf.router /* enable/disable */);
    }

    if (profile_key_changed) {
        ipmc_lib_base_port_profile_changed(glb, port_no);
    }

    IPMC_LIB_vlan_oper_warnings_update_all();
}

/******************************************************************************/
// IPMC_LIB_vlan_conf_changed()
/******************************************************************************/
static void IPMC_LIB_vlan_conf_changed(ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_vlan_conf_t &new_conf)
{
    ipmc_lib_vlan_itr_t                vlan_itr;
    vtss_appl_ipmc_lib_vlan_conf_t     &old_conf = vlan_state.conf;
    vtss_appl_ipmc_lib_compatibility_t old_compat;
    bool                               router_status_update, querier_state_update, channel_profile_changed, compatible_mode_changed, admin_active_changed;

    if (!IPMC_LIB_started) {
        // Just save the configuration for later.
        old_conf = new_conf;
        return;
    }

    router_status_update = false;
    admin_active_changed = new_conf.admin_active != old_conf.admin_active;
    if (admin_active_changed && old_conf.admin_active) {
        // We are deactivating.
        // Remove entries from the chip.
        // If we are activating, we don't need to do anything.
        ipmc_lib_base_deactivate(vlan_state);

        // If this was the last active VLAN, we must also clear dynamic router
        // ports (which are shared amongst all VLANs on a given <Protocol, IP
        // family>.
        router_status_update = true;
        for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
            if (&vlan_itr->second != &vlan_state && vlan_itr->second.global->key == vlan_state.global->key) {
                // Another VLAN instance is using the same router configuration.
                // Don't reset dynamic ports.
                router_status_update = false;
                break;
            }
        }
    }

    if (new_conf.querier_enable != old_conf.querier_enable) {
        // Hack to make ipmc_lib_base_querier_state_update() start over.
        vlan_state.status.querier_state = VTSS_APPL_IPMC_LIB_QUERIER_STATE_DISABLED;
    }

    querier_state_update = new_conf.admin_active   != old_conf.admin_active                                      ||
                           new_conf.querier_enable != old_conf.querier_enable                                    ||
                           (vlan_state.vlan_key.is_ipv4 && new_conf.querier_address != old_conf.querier_address) ||
                           new_conf.rv             != old_conf.rv                                                ||
                           new_conf.qi             != old_conf.qi                                                ||
                           new_conf.qri            != old_conf.qri                                               ||
                           new_conf.lmqi           != old_conf.lmqi;

    old_compat = old_conf.compatibility;

    channel_profile_changed = strcmp(new_conf.channel_profile.name, old_conf.channel_profile.name) != 0;

    compatible_mode_changed = new_conf.compatible_mode != old_conf.compatible_mode;

    // Change conf
    old_conf = new_conf;

    // The following ipmc_lib_base_XXX() functions utilize the current
    // configuration, so we have to postpone the calls to them until it gets
    // set.

    if (router_status_update) {
        ipmc_lib_base_router_status_update(*vlan_state.global, MESA_PORT_NO_NONE /* all ports */, true /* dynamic */, false /* remove */);
    }

    if (querier_state_update) {
        ipmc_lib_base_querier_state_update(vlan_state);
    }

    if (new_conf.compatibility != old_compat) {
        ipmc_lib_base_compatibility_status_update(vlan_state, old_compat);
    }

    if (channel_profile_changed) {
        ipmc_lib_base_vlan_profile_changed(vlan_state);
    }

    if (compatible_mode_changed) {
        ipmc_lib_base_vlan_compatible_mode_changed(vlan_state);
    }

    if (admin_active_changed) {
        IPMC_LIB_vlan_module_membership_update(vlan_state.vlan_key);
        IPMC_LIB_vlan_module_port_conf_update_all();
    }

    if (channel_profile_changed) {
        // This may actually affect other MVR VLANs, because it may now use a
        // profile whose permit rules overlap another MVR VLAN's profile's
        // permit rules.
        IPMC_LIB_vlan_oper_warnings_update_all();
    } else {
        // The changes will only affect this VLAN.
        IPMC_LIB_vlan_oper_warnings_update(vlan_state);
    }
}

/******************************************************************************/
// IPMC_LIB_vlan_port_conf_changed()
/******************************************************************************/
static void IPMC_LIB_vlan_port_conf_changed(ipmc_lib_vlan_state_t &vlan_state, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_vlan_port_conf_t &new_conf)
{
    vtss_appl_ipmc_lib_vlan_port_conf_t &old_conf = vlan_state.port_conf[port_no];

    if (!IPMC_LIB_started) {
        // Just save the configuration for later.
        old_conf = new_conf;
        return;
    }

    if (old_conf.role == new_conf.role) {
        return;
    }

    old_conf = new_conf;

    // Update our VLAN port configuration for this port.
    IPMC_LIB_vlan_module_port_conf_update(port_no);

    // Update VLAN memberships for this VLAN ID, so that the port becomes a
    // member or not of this VLAN.
    IPMC_LIB_vlan_module_membership_update(vlan_state.vlan_key);

    ipmc_lib_base_vlan_port_role_changed(vlan_state, port_no);

    // Update operational warnings
    IPMC_LIB_oper_warnings_port_role_update(vlan_state);
}

/******************************************************************************/
// IPMC_LIB_keys_differ()
// Checks only is_mvr and is_ipv4, not vid.
/******************************************************************************/
static bool IPMC_LIB_keys_differ(const vtss_appl_ipmc_lib_vlan_key_t &lhs, const vtss_appl_ipmc_lib_vlan_key_t &rhs)
{
    // Use vtss_appl_ipmc_lib_key_t::operator!=()
    return static_cast<const vtss_appl_ipmc_lib_key_t>(lhs) != static_cast<vtss_appl_ipmc_lib_key_t>(rhs);
}

/******************************************************************************/
// IPMC_LIB_global_state_populate()
/******************************************************************************/
static void IPMC_LIB_global_state_populate(bool call_base)
{
    mesa_port_no_t port_no;
    int            i;

    for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
        ipmc_lib_global_state_t &glb = IPMC_LIB_global_state[i];

        vtss_clear(glb);
        glb.key          = IPMC_LIB_key_from_index_get(i); // MVR or IPMC and IGMP or MLD?
        glb.lists        = &IPMC_LIB_global_lists;
        glb.lib_cap      = IPMC_LIB_cap;
        glb.protocol_cap = IPMC_LIB_protocol_cap[IPMC_LIB_is_mvr_to_idx_get(glb.key.is_mvr)];

        if (call_base) {
            // This is in order to get e.g. unregistered flooding set up and
            // packet Rx registration set up.
            IPMC_LIB_global_conf_changed(glb.key, IPMC_LIB_global_conf_defaults[i]);

            for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
                IPMC_LIB_port_conf_changed(glb.key, port_no, IPMC_LIB_port_conf_defaults[i]);
            }
        }
    }
}

/******************************************************************************/
// IPMC_LIB_vlan_cnt_get()
/******************************************************************************/
static uint32_t IPMC_LIB_vlan_cnt_get(bool is_mvr)
{
    ipmc_lib_vlan_itr_t vlan_itr;
    uint32_t            cnt = 0;

    for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
        if (vlan_itr->first.is_mvr == is_mvr) {
            cnt++;
        }
    }

    return cnt;
}

/******************************************************************************/
// IPMC_LIB_vlan_do_create()
/******************************************************************************/
static mesa_rc IPMC_LIB_vlan_do_create(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key)
{
    ipmc_lib_vlan_itr_t   vlan_itr;
    ipmc_lib_vlan_state_t *vlan_state;
    uint32_t              vlan_cnt_max, state_idx;
    mesa_port_no_t        port_no;

    // See if it exists.
    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) != IPMC_LIB_vlan_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_VLAN_ALREADY_EXISTS;
    }

    // Check that we won't create more instances than we can allow for this
    // protocol.
    vlan_cnt_max = IPMC_LIB_protocol_cap[IPMC_LIB_is_mvr_to_idx_get(vlan_key.is_mvr)].vlan_cnt_max;

#ifdef VTSS_SW_OPTION_SMB_IPMC
    // One VLAN takes two entries - one for IGMP and one for MLD, so we actually
    // allow twice the number of entries when also MLD is supported.
    vlan_cnt_max *= 2;
#endif

    if (IPMC_LIB_vlan_cnt_get(vlan_key.is_mvr) >= vlan_cnt_max) {
        return VTSS_APPL_IPMC_LIB_RC_VLAN_LIMIT_REACHED;
    }

    if ((vlan_itr = IPMC_LIB_vlan_map.get(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_OUT_OF_MEMORY;
    }

    // Populate new entry with defaults
    state_idx = IPMC_LIB_key_to_idx_get(static_cast<vtss_appl_ipmc_lib_key_t>(vlan_key));
    vtss_clear(vlan_itr->second);
    vlan_state           = &vlan_itr->second;
    vlan_state->vlan_key = vlan_key;
    vlan_state->conf     = IPMC_LIB_vlan_conf_defaults[state_idx];
    vlan_state->global   = &IPMC_LIB_global_state[state_idx];

    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        vlan_state->port_conf[port_no] = IPMC_LIB_vlan_port_conf_defaults[state_idx];
    }

    // Further initialization done by base.
    ipmc_lib_base_vlan_state_init(*vlan_state, true /* clear statistics */);

    // Update operational warnings of all VLANs, because they may be affected by
    // this VLAN being added.
    IPMC_LIB_vlan_oper_warnings_update_all();

    T_D("%s: Added VLAN", vlan_key);
    return VTSS_RC_OK;
}

/******************************************************************************/
// ipmc_lib_vlan_create()
/******************************************************************************/
mesa_rc ipmc_lib_vlan_create(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, bool called_back)
{
    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));

    if (called_back) {
        if (vlan_key.is_mvr) {
            T_E("%s: MVR is not allowed to call us with called_back == true", vlan_key);
            return VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR;
        }

        // We already have our own mutex, because this is a result of us calling
        // ipmc_vlan_auto_vivify_check()
        return IPMC_LIB_vlan_do_create(vlan_key);
    } else {
        // We don't have our own mutex, because the IPMC module just detected a
        // new VLAN interface or MVR was configured with a new VLAN that they
        // want us to create.
        IPMC_LIB_LOCK_SCOPE();
        return IPMC_LIB_vlan_do_create(vlan_key);
    }
}

/******************************************************************************/
// ipmc_lib_vlan_remove()
/******************************************************************************/
mesa_rc ipmc_lib_vlan_remove(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key)
{
    ipmc_lib_vlan_itr_t vlan_itr;

    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));

    IPMC_LIB_LOCK_SCOPE();

    // See if it exists.
    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
    }

    // Remove all chip entries.
    ipmc_lib_base_deactivate(vlan_itr->second);

    // Remove it from our map
    IPMC_LIB_vlan_map.erase(vlan_itr);

    // And update operational warnings of all remaining VLAN instances, because
    // they may be affected by this VLAN being removed.
    IPMC_LIB_vlan_oper_warnings_update_all();

    return VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_LIB_load_defaults()
/******************************************************************************/
static void IPMC_LIB_load_defaults(void)
{
    vtss_appl_ipmc_lib_global_conf_t conf;
    int                              i;

    // We reload defaults by first setting all global states' admin_active to
    // false (see also IPMC_LIB_start_all()).
    for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
        ipmc_lib_global_state_t &glb = IPMC_LIB_global_state[i];

        conf = glb.conf;
        conf.admin_active = false;
        IPMC_LIB_global_conf_changed(glb.key, conf);
    }

    // Clear the map now.
    IPMC_LIB_vlan_map.clear();

    // Then re-populate the global state with defaults.
    IPMC_LIB_global_state_populate(true);
}

/******************************************************************************/
// IPMC_LIB_port_state_change_callback()
/******************************************************************************/
static void IPMC_LIB_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    int i;

    IPMC_LIB_LOCK_SCOPE();

    IPMC_LIB_ports_with_link[port_no] = status->link;

    if (!status->link) {
        // Nothing else to do.
        return;
    }

    ipmc_lib_base_port_down(IPMC_LIB_global_lists.grp_map, port_no);

    for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
        // When a port goes down and it's a dynamic router port, we need to mark
        // it as un-dynamic and take it out of all router-port-forwarding masks.
        // This is a per IPMC/MVR and IGMP/MLD job.
        ipmc_lib_base_router_status_update(IPMC_LIB_global_state[i], port_no, true /* dynamic */, false /* remove */);
    }
}

#ifdef VTSS_SW_OPTION_MSTP
/******************************************************************************/
// IPMC_LIB_stp_state_change_callback()
/******************************************************************************/
static void IPMC_LIB_stp_state_change_callback(vtss_common_port_t l2port, vtss_common_stpstate_t new_state)
{
    ipmc_lib_vlan_itr_t      vlan_itr;
    aggr_mgmt_group_member_t members;
    vtss_isid_t              isid;
    vtss_poag_no_t           aggr_no;
    mesa_port_no_t           port_no;
    mesa_rc                  rc;

    // We only care about the event when the port gets to forwarding state.
    if (new_state != VTSS_COMMON_STPSTATE_FORWARDING) {
        return;
    }

    // Convert l2port to isid/iport
    if (l2port_is_port(l2port)) {
        port_no = l2port;
    } else if (l2port_is_poag(l2port)) {
        if (!l2port2poag(l2port, &isid, &aggr_no)) {
            T_IG(IPMC_LIB_TRACE_GRP_CALLBACK, "l2port2poag(%u) failed", l2port);
            return;
        }

        if ((rc = aggr_mgmt_members_get(VTSS_ISID_START, aggr_no, &members, false)) != VTSS_RC_OK) {
            T_IG(IPMC_LIB_TRACE_GRP_CALLBACK, "aggr_mgmt_members_get(l2port = %u => aggr_no = %u) failed: %s", l2port, aggr_no, error_txt(rc));
            return;
        }

        for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
            if (members.entry.member[port_no]) {
                // Just pick the first port in the aggregation, because
                // ipmc_lib_pdu_tx() will take care of transmitting to the
                // correct port.
                break;
            }
        }

        if (port_no >= IPMC_LIB_port_cnt) {
            // No ports were members of this aggregation. Odd.
            T_IG(IPMC_LIB_TRACE_GRP_CALLBACK, "No ports were members of l2port = %u => aggr_no = %u", l2port, aggr_no);
            return;
        }
    } else {
        T_IG(IPMC_LIB_TRACE_GRP_CALLBACK, "l2port (%u) is neither a normal port nor an aggregation", l2port);
        return;
    }

    T_DG_PORT(IPMC_LIB_TRACE_GRP_CALLBACK, port_no, "STP change to forward");

    IPMC_LIB_LOCK_SCOPE();

    for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
        if (vlan_itr->second.status.oper_state != VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
            // This is not active. Nothing to do.
            continue;
        }

        ipmc_lib_base_stp_forwarding_set(vlan_itr->second, port_no);
    }
}
#endif

/******************************************************************************/
// IPMC_LIB_ip_vlan_interface_change_callback()
/******************************************************************************/
static void IPMC_LIB_ip_vlan_interface_change_callback(vtss_ifindex_t ifindex)
{
    vtss_appl_ip_if_status_link_t link_status;
    vtss_appl_ip_if_key_ipv4_t    ipv4_key;
    vtss_appl_ip_if_key_ipv6_t    ipv6_key;
    ipmc_lib_vlan_if_t            status;
    ipmc_lib_vlan_if_itr_t        vlan_if_itr;
    ipmc_lib_vlan_itr_t           vlan_itr;
    mesa_vid_t                    vid;
    bool                          exists;
    mesa_rc                       rc;

    if (IPMC_LIB_ifindex_to_vlan(ifindex, vid) != VTSS_RC_OK) {
        // ifindex not representing a VLAN ID.
        return;
    }

    T_DG(IPMC_LIB_TRACE_GRP_CALLBACK, "Got change on VLAN interface %u", vid);

    exists = vtss_appl_ip_if_exists(ifindex);

    if (exists) {
        // Build up a new structure
        vtss_clear(status);
        status.vid = vid;

        if ((rc = vtss_appl_ip_if_status_link_get(ifindex, &link_status)) != VTSS_RC_OK) {
            // Someone may have deleted the VLAN interface between check for
            // exists and this call, so only T_IG()
            T_IG(IPMC_LIB_TRACE_GRP_CALLBACK, "vtss_appl_ip_if_status_link_get(%s) failed: %s", ifindex, error_txt(rc));
        } else {
            status.if_up = (link_status.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) != 0;
        }

        // Get last assigned IPv4 address on interface.
        vtss_clear(ipv4_key);
        ipv4_key.ifindex = ifindex;

        while (vtss_appl_ip_if_status_ipv4_itr(&ipv4_key, &ipv4_key) == VTSS_RC_OK) {
            if (ipv4_key.ifindex != ifindex) {
                // Got into next VLAN interface
                break;
            }

            status.ipv4 = ipv4_key.addr.address;
        }

        // Get last assigned IPv6 address on interface
        vtss_clear(ipv6_key);
        ipv6_key.ifindex = ifindex;

        while (vtss_appl_ip_if_status_ipv6_itr(&ipv6_key, &ipv6_key) == VTSS_RC_OK) {
            if (ipv6_key.ifindex != ifindex) {
                // Got into next VLAN interface
                break;
            }

            if (vtss_ipv6_addr_is_link_local(&ipv6_key.addr.address)) {
                status.ipv6_link_local = ipv6_key.addr.address;
            } else {
                status.ipv6 = ipv6_key.addr.address;
            }
        }
    }

    IPMC_LIB_LOCK_SCOPE();

    if (exists) {
        // Map::get creates a new entry if it doesn't exist.
        if ((vlan_if_itr = IPMC_LIB_vlan_if_map.get(vid)) == IPMC_LIB_vlan_if_map.end()) {
            // Out of memory.
            T_EG(IPMC_LIB_TRACE_GRP_CALLBACK, "Ran out of memory when attempting to add VLAN interface for %u to our map", vid);
            return;
        }

        vlan_if_itr->second = status;
    } else {
        // Erase it
        IPMC_LIB_vlan_if_map.erase(vid);
    }

    // Update operational warnings.
    for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
        IPMC_LIB_oper_warnings_vlan_interface_update(vlan_itr->second);
    }
}

/******************************************************************************/
// IPMC_LIB_aggr_port_mask_update()
/******************************************************************************/
static void IPMC_LIB_aggr_port_mask_update(mesa_port_no_t port_no, mesa_port_list_t port_mask)
{
    // Remove ourselves from the port list.
    port_mask[port_no] = false;

    if (IPMC_LIB_global_lists.aggr_port_masks[port_no] == port_mask) {
        // Nothing to do for this one, because we already know what we are
        // supposed to know.
        return;
    }

    T_IG_PORT(IPMC_LIB_TRACE_GRP_CALLBACK, port_no, "Aggregation port_mask: %s -> %s", IPMC_LIB_global_lists.aggr_port_masks[port_no], port_mask);

    IPMC_LIB_global_lists.aggr_port_masks[port_no] = port_mask;
    ipmc_lib_base_aggr_port_update(IPMC_LIB_global_lists.grp_map, port_no);
}

/******************************************************************************/
// IPMC_LIB_aggr_change_callback()
/******************************************************************************/
static void IPMC_LIB_aggr_change_callback(vtss_isid_t isid, uint aggr_no)
{
    aggr_mgmt_group_member_t members;
    mesa_port_list_t         empty_port_mask;
    mesa_port_no_t           port_no;
    mesa_rc                  rc;

    T_IG(IPMC_LIB_TRACE_GRP_CALLBACK, "aggr_no = %u", aggr_no);

    empty_port_mask.clear_all();

    IPMC_LIB_LOCK_SCOPE();

    // Yes, we get the aggr_no, but if we get called back due to an aggregation
    // being deleted, we don't know which port numbers the aggregation referred
    // to :-(
    // So better go through all and get our aggregation port map updated.
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if ((aggr_no = aggr_mgmt_get_aggr_id(VTSS_ISID_START, port_no)) == VTSS_AGGR_NO_NONE) {
            // Port not aggregated (anymore)
            IPMC_LIB_aggr_port_mask_update(port_no, empty_port_mask);
        } else {
            vtss_clear(members);
            if ((rc = aggr_mgmt_members_get(VTSS_ISID_START, aggr_no, &members, false)) != VTSS_RC_OK) {
                // If it returns something != VTSS_RC_OK, it's either because
                // the aggregation was deleted or because a port went down, so
                // that there's only one port left in the aggregation. Either
                // way, we treat it as if the port is no longer aggregated.
                IPMC_LIB_aggr_port_mask_update(port_no, empty_port_mask);
            } else {
                // Here, this port is aggregated with at least one other port. If
                // the port mask that was returned by the AGGR module differs from
                // our port mask, we need to do some updating.
                IPMC_LIB_aggr_port_mask_update(port_no, members.entry.member);
            }
        }
    }
}

/******************************************************************************/
// IPMC_LIB_vlan_membership_change_callback()
/******************************************************************************/
static void IPMC_LIB_vlan_membership_change_callback(vtss_isid_t isid, mesa_vid_t vid, vlan_membership_change_t *changes)
{
    ipmc_lib_vlan_itr_t           vlan_itr;
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    int                           i;

    // Our only purpose of this callback is to update MVR operational warnings.
    vlan_key.is_mvr = true;
    vlan_key.vid    = vid;

    IPMC_LIB_LOCK_SCOPE();

    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vlan_key.is_ipv4 = i == 0;

        if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) != IPMC_LIB_vlan_map.end()) {
            T_IG(IPMC_LIB_TRACE_GRP_CALLBACK, "%s: Updating operational warnings", vlan_key);
            IPMC_LIB_oper_warnings_vlan_interface_update(vlan_itr->second);
        }
    }
}

/******************************************************************************/
// IPMC_LIB_vlan_auto_vivify_check()
/******************************************************************************/
static mesa_rc IPMC_LIB_vlan_auto_vivify_check(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key)
{
    if (vlan_key.is_mvr) {
        // If this is MVR, the VLAN must exist, because it is created
        // directly by MVR with a call to ipmc_lib_vlan_create().
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
    } else {
        // If this is IPMC, the thing is that during boot and load of
        // startup-config we haven't yet been told of a new VLAN interface,
        // because the IP module hasn't yet called IPMC back. However, if we ask
        // the IP module whether it exists, we get a "yes", so in that case, we
        // simply ask the IPMC module to create the VLAN interface for us before
        // we continue. It does so by calling ipmc_lib_vlan_create() during
        // execution of the following function.
        mesa_rc ipmc_vlan_auto_vivify_check(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key);
        return ipmc_vlan_auto_vivify_check(vlan_key);
    }
}

/******************************************************************************/
// IPMC_LIB_start_all()
/******************************************************************************/
static void IPMC_LIB_start_all(void)
{
    vtss_appl_ipmc_lib_global_conf_t global_conf;
    vtss_appl_ipmc_lib_vlan_conf_t   vlan_conf;
    ipmc_lib_vlan_itr_t              vlan_itr;
    bool                             was_admin_active[IPMC_LIB_STATE_CNT];
    mesa_port_no_t                   port_no;
    int                              i;

    // It's enough to toggle each global state's admin_active bit to false and
    // back to what it is configured to in order to get everything started
    // correctly, except that we also need to update the static router ports,
    // because the base saves those in two separate arrays and don't use the
    // port_conf.

    // We need to take a snapshot of the current global_conf, because
    // IPMC_LIB_global_conf_changed() expects a configuration that doesn't
    // point to the current global_conf.
    for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
        ipmc_lib_global_state_t &glb = IPMC_LIB_global_state[i];

        global_conf              = glb.conf;
        was_admin_active[i]      = global_conf.admin_active;
        global_conf.admin_active = false;
        IPMC_LIB_global_conf_changed(glb.key, global_conf);

        ipmc_lib_base_router_status_update(glb, MESA_PORT_NO_NONE, false /* static */, IPMC_LIB_port_conf_defaults[i].router);
    }

    IPMC_LIB_started = true;

    for (i = 0; i < IPMC_LIB_STATE_CNT; i++) {
        ipmc_lib_global_state_t &glb = IPMC_LIB_global_state[i];

        global_conf              = glb.conf;
        global_conf.admin_active = was_admin_active[i];
        IPMC_LIB_global_conf_changed(glb.key, global_conf);

        for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
            ipmc_lib_base_router_status_update(glb, port_no, false /* static */, glb.port_conf[port_no].router);
        }
    }

    IPMC_LIB_vlan_oper_warnings_update_all();

    for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
        if (vlan_itr->second.status.oper_state != VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
            continue;
        }

        // Enforce an update by first setting this to a default configuration
        // and call IPMC_LIB_vlan_conf_changed() with the current
        // configuration. A.o. querier_enable requires a call to base.
        vlan_conf = vlan_itr->second.conf;
        vlan_itr->second.conf = IPMC_LIB_vlan_conf_defaults[IPMC_LIB_key_to_idx_get(vlan_itr->second.global->key)];
        IPMC_LIB_vlan_conf_changed(vlan_itr->second, vlan_conf);
    }
}

/******************************************************************************/
// ipmc_lib_vlan_if_ipv4_get()
/******************************************************************************/
vtss_appl_ipmc_lib_ip_t ipmc_lib_vlan_if_ipv4_get(mesa_vid_t vid, const vtss_appl_ipmc_lib_ip_t &querier)
{
    ipmc_lib_vlan_if_itr_t  vlan_if_itr;
    vtss_appl_ipmc_lib_ip_t res;

    res.is_ipv4 = true;

    if (!querier.is_ipv4) {
        T_E("Invoked with querier.is_ipv4 != true: %s", querier);
    }

    if (querier.ipv4 != 0) {
        // Always use configured querier address.
        res.ipv4 = querier.ipv4;
        return res;
    }

    IPMC_LIB_LOCK_ASSERT_LOCKED("");

    if ((vlan_if_itr = IPMC_LIB_vlan_if_map.find(vid)) != IPMC_LIB_vlan_if_map.end()) {
        // VLAN interface exists.
        if (vlan_if_itr->second.if_up && vlan_if_itr->second.ipv4 != 0) {
            res.ipv4 = vlan_if_itr->second.ipv4;
            return res;
        }
    }

    // VLAN interface for VID doesn't exist or is not up or doesn't have an IP
    // address.
    // The old code found IPv4 address of first VLAN interface that was up with
    // a non-zero IP address.
    for (vlan_if_itr = IPMC_LIB_vlan_if_map.begin(); vlan_if_itr != IPMC_LIB_vlan_if_map.end(); ++vlan_if_itr) {
        if (vlan_if_itr->second.if_up && vlan_if_itr->second.ipv4 != 0) {
            res.ipv4 = vlan_if_itr->second.ipv4;
            return res;
        }
    }

    // Still here? Use default IP address
    res.ipv4 = 0xc0000201; // 192.0.2.1
    return res;
}

/******************************************************************************/
// vtss_appl_ipmc_capabilities_get()
// Get either IPMC or MVR capabilities.
/******************************************************************************/
mesa_rc vtss_appl_ipmc_capabilities_get(bool is_mvr, vtss_appl_ipmc_capabilities_t *cap)
{
    VTSS_RC(IPMC_LIB_user_supported_check(is_mvr));
    VTSS_RC(IPMC_LIB_ptr_check(cap));

    *cap = IPMC_LIB_protocol_cap[IPMC_LIB_is_mvr_to_idx_get(is_mvr)];
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_capabilities_get()
// Get capabilities common to MVR and IPMC.
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_capabilities_get(vtss_appl_ipmc_lib_capabilities_t *cap)
{
    VTSS_RC(IPMC_LIB_ptr_check(cap));

    *cap = IPMC_LIB_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// ipmc_lib_capabilities_set()
// Internal function.
/******************************************************************************/
void ipmc_lib_capabilities_set(bool is_mvr, const vtss_appl_ipmc_capabilities_t &cap)
{
    IPMC_LIB_protocol_cap[IPMC_LIB_is_mvr_to_idx_get(is_mvr)] = cap;
}

/******************************************************************************/
// ipmc_lib_global_default_conf_set()
// Internal function.
/******************************************************************************/
void ipmc_lib_global_default_conf_set(const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_global_conf_t &global_conf)
{
    IPMC_LIB_global_conf_defaults[IPMC_LIB_key_to_idx_get(key)] = global_conf;
}

/******************************************************************************/
// ipmc_lib_port_default_conf_set()
// Internal function.
/******************************************************************************/
void ipmc_lib_port_default_conf_set(const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_port_conf_t &port_conf)
{
    IPMC_LIB_port_conf_defaults[IPMC_LIB_key_to_idx_get(key)] = port_conf;
}

/******************************************************************************/
// ipmc_lib_vlan_default_conf_set()
// Internal function.
/******************************************************************************/
void ipmc_lib_vlan_default_conf_set(const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_vlan_conf_t &vlan_conf)
{
    IPMC_LIB_vlan_conf_defaults[IPMC_LIB_key_to_idx_get(key)] = vlan_conf;
}

/******************************************************************************/
// ipmc_lib_vlan_port_default_conf_set()
// Internal function.
/******************************************************************************/
void ipmc_lib_vlan_port_default_conf_set(const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_vlan_port_conf_t &vlan_port_conf)
{
    IPMC_LIB_vlan_port_conf_defaults[IPMC_LIB_key_to_idx_get(key)] = vlan_port_conf;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_global_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_global_conf_default_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_global_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    // No need to take mutex, because ipmc_lib_global_default_conf_set() is
    // guaranteed to be called before this function can be called, and is never
    // touched again.
    *conf = IPMC_LIB_global_conf_defaults[IPMC_LIB_key_to_idx_get(key)];
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_global_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_global_conf_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_global_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    IPMC_LIB_LOCK_SCOPE();
    *conf = IPMC_LIB_global_state[IPMC_LIB_key_to_idx_get(key)].conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_global_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_global_conf_set(const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_global_conf_t *new_conf)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    VTSS_RC(IPMC_LIB_ptr_check(new_conf));

    vtss_appl_ipmc_lib_global_conf_t &default_conf = IPMC_LIB_global_conf_defaults[IPMC_LIB_key_to_idx_get(key)];

    T_I("%s: %s", key, *new_conf);

    if (key.is_mvr) {
        // MVR configuration checks
        if (new_conf->unregistered_flooding_enable != default_conf.unregistered_flooding_enable) {
            return VTSS_APPL_IPMC_LIB_RC_UNREGISTERED_FLOODING_ENABLE_CANNOT_BE_CHANGED;
        }

        if (new_conf->proxy_enable != default_conf.proxy_enable) {
            return VTSS_APPL_IPMC_LIB_RC_PROXY_CANNOT_BE_CHANGED;
        }

        if (new_conf->leave_proxy_enable != default_conf.leave_proxy_enable) {
            return VTSS_APPL_IPMC_LIB_RC_LEAVE_PROXY_CANNOT_BE_CHANGED;
        }

        if (new_conf->ssm_prefix != default_conf.ssm_prefix) {
            return VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_CANNOT_BE_CHANGED;
        }

        if (new_conf->ssm_prefix_len != default_conf.ssm_prefix_len) {
            return VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_LEN_CANNOT_BE_CHANGED;
        }
    } else {
        // IPMC configuration checks
#if !defined(VTSS_SW_OPTION_SMB_IPMC)
        // A number of parameters cannot be changed. They used to be hidden in the
        // second/third layer and set to a fixed value, but nowadays these values
        // come from the IPMC configuration, which allows to set them if compiling
        // for SMBStaX or higher.
        if (new_conf->proxy_enable != default_conf.proxy_enable) {
            return VTSS_APPL_IPMC_LIB_RC_PROXY_CANNOT_BE_CHANGED;
        }

        if (new_conf->leave_proxy_enable != default_conf.leave_proxy_enable) {
            return VTSS_APPL_IPMC_LIB_RC_LEAVE_PROXY_CANNOT_BE_CHANGED;
        }

        if (new_conf->ssm_prefix != default_conf.ssm_prefix) {
            return VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_CANNOT_BE_CHANGED;
        }

        if (new_conf->ssm_prefix_len != default_conf.ssm_prefix_len) {
            return VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_LEN_CANNOT_BE_CHANGED;
        }
#endif // !defined(VTSS_SW_OPTION_SMB_IPMC)
    }

    VTSS_RC(IPMC_LIB_prefix_check(new_conf->ssm_prefix, new_conf->ssm_prefix_len));

    IPMC_LIB_LOCK_SCOPE();

    IPMC_LIB_global_conf_changed(key, *new_conf);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_port_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_port_conf_default_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_port_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    // No need to take mutex, because IPMC_LIB_port_conf_defaults[] is
    // guaranteed to be created before this function can be called, and is never
    // touched again.
    *conf = IPMC_LIB_port_conf_defaults[IPMC_LIB_key_to_idx_get(key)];
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_port_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_port_conf_get(const vtss_appl_ipmc_lib_key_t &key, mesa_port_no_t port_no, vtss_appl_ipmc_lib_port_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    VTSS_RC(IPMC_LIB_port_no_check(port_no));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    IPMC_LIB_LOCK_SCOPE();

    *conf = IPMC_LIB_global_state[IPMC_LIB_key_to_idx_get(key)].port_conf[port_no];
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_port_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_port_conf_set(const vtss_appl_ipmc_lib_key_t &key, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_port_conf_t *new_conf)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    VTSS_RC(IPMC_LIB_port_no_check(port_no));
    VTSS_RC(IPMC_LIB_ptr_check(new_conf));

    T_D_PORT(port_no, "%s: %s", key, *new_conf);

    vtss_appl_ipmc_lib_port_conf_t &default_port_conf = IPMC_LIB_port_conf_defaults[IPMC_LIB_key_to_idx_get(key)];

    // Checks common to IPMC and MVR:
    if (new_conf->grp_cnt_max > 10) {
        return VTSS_APPL_IPMC_LIB_RC_MAX_GROUP_CNT_INVALID;
    }

    if (key.is_mvr) {
        // MVR checks:
        if (new_conf->router != default_port_conf.router) {
            return VTSS_APPL_IPMC_LIB_RC_ROUTER_CANNOT_BE_CHANGED;
        }

        if (new_conf->grp_cnt_max != default_port_conf.grp_cnt_max) {
            return VTSS_APPL_IPMC_LIB_RC_MAX_GROUP_CNT_CANNOT_BE_CHANGED;
        }

        if (strcmp(new_conf->profile_key.name, default_port_conf.profile_key.name) != 0) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_KEY_CANNOT_BE_CHANGED;
        }
    } else {
        // IPMC checks:

#if !defined(VTSS_SW_OPTION_SMB_IPMC)
        // A number of parameters cannot be changed. They used to be hidden in the
        // second/third layer and set to a fixed value, but nowadays these values
        // come from the IPMC configuration, which allows to set them if compiling
        // for SMBStaX or higher.
        if (new_conf->grp_cnt_max != default_port_conf.grp_cnt_max) {
            return VTSS_APPL_IPMC_LIB_RC_MAX_GROUP_CNT_CANNOT_BE_CHANGED;
        }

        if (strcmp(new_conf->profile_key.name, default_port_conf.profile_key.name) != 0) {
            return VTSS_APPL_IPMC_LIB_RC_PROFILE_KEY_CANNOT_BE_CHANGED;
        }
#endif // !defined(VTSS_SW_OPTION_SMB_IPMC)
    }

#ifdef VTSS_SW_OPTION_SMB_IPMC
    // Check profile_key. Use a public API to figure out whether the profile
    // name lives up to the rules. The function returns a suitable error code.
    VTSS_RC(ipmc_lib_profile_key_check(&new_conf->profile_key, true /* allow empty profile name */));
#endif

    IPMC_LIB_LOCK_SCOPE();

    IPMC_LIB_port_conf_changed(key, port_no, *new_conf);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_port_itr()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_port_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next)
{
    return vtss_appl_iterator_ifindex_front_port_exist(prev, next);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_conf_default_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_vlan_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    *conf = IPMC_LIB_vlan_conf_defaults[IPMC_LIB_key_to_idx_get(key)];
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_conf_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, vtss_appl_ipmc_lib_vlan_conf_t *conf)
{
    ipmc_lib_vlan_itr_t vlan_itr;

    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    IPMC_LIB_LOCK_SCOPE();

    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        VTSS_RC(IPMC_LIB_vlan_auto_vivify_check(vlan_key));

        // Still here? Get a new iterator
        if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
            // Nothing else we can do.
            return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
        }
    }

    *conf = vlan_itr->second.conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_conf_set(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, const vtss_appl_ipmc_lib_vlan_conf_t *new_conf)
{
    ipmc_lib_vlan_itr_t            vlan_itr;
    vtss_appl_ipmc_lib_vlan_conf_t local_new_conf;
    size_t                         name_len, profile_name_len;

    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));
    VTSS_RC(IPMC_LIB_ptr_check(new_conf));

    vtss_appl_ipmc_lib_vlan_conf_t &default_conf = IPMC_LIB_vlan_conf_defaults[IPMC_LIB_key_to_idx_get(static_cast<vtss_appl_ipmc_lib_key_t>(vlan_key))];

    T_D("%s: %s", vlan_key, *new_conf);

    // Configuration checks common to both IPMC and MVR
    if (vlan_key.is_ipv4) {
        if (!new_conf->querier_address.is_ipv4) {
            return VTSS_APPL_IPMC_LIB_RC_QUERIER_ADDR_NOT_IPV4;
        }

        if (!vtss_ipv4_addr_is_unicast(&new_conf->querier_address.ipv4) || vtss_ipv4_addr_is_loopback(&new_conf->querier_address.ipv4)) {
            return VTSS_APPL_IPMC_LIB_RC_QUERIER_ADDR_NOT_UNICAST;
        }
    }

    if (new_conf->compatibility != VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO &&
        new_conf->compatibility != VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD  &&
        new_conf->compatibility != VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN  &&
        new_conf->compatibility != VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM) {
        return VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY;
    }

    if (!vlan_key.is_ipv4 && new_conf->compatibility == VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD) {
        return VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY_OLD_WITH_MLD;
    }

    if (new_conf->pcp > 7) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_PCP;
    }

    if (new_conf->rv < 2 || new_conf->rv > 255) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_ROBUSTNESS_VARIABLE;
    }

    if (new_conf->qi < 1 || new_conf->qi > 31744) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_QUERY_INTERVAL;
    }

    if (new_conf->qri > 31744) {
        // RBNTBD: IGMPv3 and MLDv2 has 16-bit floating point support.
        return VTSS_APPL_IPMC_LIB_RC_INVALID_QUERY_RESPONSE_INTERVAL;
    }

    if (new_conf->lmqi > 31744) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_LMQI;
    }

    if (new_conf->uri < 1 || new_conf->uri > 31744) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_URI;
    }

    if (vlan_key.is_mvr) {
        // MVR configuration checks.

        if (new_conf->admin_active != default_conf.admin_active) {
            // Cannot be changed in MVR.
            return VTSS_APPL_IPMC_LIB_RC_ADMIN_ACTIVE_CANNOT_BE_CHANGED;
        }

        VTSS_RC(IPMC_LIB_name_check(new_conf->name, &name_len));

        if (new_conf->compatibility != default_conf.compatibility) {
            return VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY_CANNOT_BE_CHANGED;
        }

        if (new_conf->rv != default_conf.rv) {
            return VTSS_APPL_IPMC_LIB_RC_RV_CANNOT_BE_CHANGED;
        }

        if (new_conf->qi != default_conf.qi) {
            return VTSS_APPL_IPMC_LIB_RC_QI_CANNOT_BE_CHANGED;
        }

        if (new_conf->qri != default_conf.qri) {
            return VTSS_APPL_IPMC_LIB_RC_QRI_CANNOT_BE_CHANGED;
        }

        if (new_conf->uri != default_conf.uri) {
            return VTSS_APPL_IPMC_LIB_RC_URI_CANNOT_BE_CHANGED;
        }

#ifdef VTSS_SW_OPTION_SMB_IPMC
        // Check channel_profile. Use a public API to figure out whether the
        // profile name lives up to the rules. The function returns a suitable
        // error code.
        VTSS_RC(ipmc_lib_profile_key_check(&new_conf->channel_profile, true /* allow empty profile name */));
#endif
    } else {
        // IPMC configuration checks.

        if (new_conf->name[0] != '\0') {
            return VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_SUPPORTED;
        }

        name_len = 0;

        if (new_conf->compatible_mode != default_conf.compatible_mode) {
            // This is only used by MVR.
            return VTSS_APPL_IPMC_LIB_RC_VLAN_MODE_CANNOT_BE_CHANGED;
        }

        if (new_conf->tx_tagged != default_conf.tx_tagged) {
            // This is only used by MVR.
            return VTSS_APPL_IPMC_LIB_RC_TX_TAGGED_CANNOT_BE_CHANGED;
        }

#if !defined(VTSS_SW_OPTION_SMB_IPMC)
        // A number of parameters cannot be changed. They used to be hidden in the
        // second/third layer and set to a fixed value, but nowadays these values
        // come from the IPMC configuration, which allows for setting them if
        // compiling for SMBStaX or higher.
        if (new_conf->compatibility != default_conf.compatibility) {
            return VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY_CANNOT_BE_CHANGED;
        }

        if (new_conf->pcp != default_conf.pcp) {
            return VTSS_APPL_IPMC_LIB_RC_PCP_CANNOT_BE_CHANGED;
        }

        if (new_conf->rv != default_conf.rv) {
            return VTSS_APPL_IPMC_LIB_RC_RV_CANNOT_BE_CHANGED;
        }

        if (new_conf->qi != default_conf.qi) {
            return VTSS_APPL_IPMC_LIB_RC_QI_CANNOT_BE_CHANGED;
        }

        if (new_conf->qri != default_conf.qri) {
            return VTSS_APPL_IPMC_LIB_RC_QRI_CANNOT_BE_CHANGED;
        }

        if (new_conf->lmqi != default_conf.lmqi) {
            return VTSS_APPL_IPMC_LIB_RC_LMQI_CANNOT_BE_CHANGED;
        }

        if (new_conf->uri != default_conf.uri) {
            return VTSS_APPL_IPMC_LIB_RC_URI_CANNOT_BE_CHANGED;
        }
#endif // !defined(VTSS_SW_OPTION_SMB_IPMC)

        if (new_conf->channel_profile.name[0] != '\0') {
            return VTSS_APPL_IPMC_LIB_RC_CHANNEL_PROFILES_NOT_SUPPORTED;
        }
    }

    // Final check.
    if (new_conf->admin_active && new_conf->querier_enable) {
        // Defer this check until the instance is active and the querier is
        // enabled, because QI and QRI won't be used until then.
        // The thing is that there is an interdependency between the two, and if
        // we checked it while the user was configuring them one by one, she
        // wouldn't be able to configure new values independently.
        // As long as admin_active and querier_enable are false, they can be set
        // to any value.
        // The "biggest" problem with this approach is that we need to change
        // the output of "show running-config" to output querier-enable *after*
        // the QI and QRI are both output. This was not done in the old
        // implementation of this module, but it doesn't matter, because we know
        // that existing startup-configs have valid values for these two
        // parameters.
        if (new_conf->qri >= 10 * new_conf->qi) {
            // RFC3810, 9.3, Query Response Interval:
            // The number of seconds presented by the QRI must be less than the
            // QI.
            return VTSS_APPL_IPMC_LIB_RC_QRI_QI_INVALID;
        }
    }

    // Normalize the configuration, that is, make sure that all unused chars in
    // new_conf->name and new_conf->channel_profile are '\0', and make sure the
    // querier address is indeed all-zeros for MLD. This allows for doing a
    // memcmp() with an existing configuration.
    local_new_conf = *new_conf;

    // First local_new_conf.name
    if (name_len < sizeof(local_new_conf.name)) {
        memset(&local_new_conf.name[name_len], 0, sizeof(local_new_conf.name) - name_len);
    }

    // Then local_new_conf.channel_profile
    profile_name_len = strlen(local_new_conf.channel_profile.name);
    if (profile_name_len < sizeof(local_new_conf.channel_profile.name)) {
        memset(&local_new_conf.channel_profile.name[profile_name_len], 0, sizeof(local_new_conf.channel_profile.name) - profile_name_len);
    }

    // Finally local_new_conf.querier_address.
    if (!vlan_key.is_ipv4) {
        local_new_conf.querier_address.is_ipv4 = false;
        local_new_conf.querier_address.all_zeros_set();
    }

    IPMC_LIB_LOCK_SCOPE();

    // Check channel profile against other MVR VLANs. The channel profile must
    // be unique.
    if (vlan_key.is_mvr && local_new_conf.admin_active && local_new_conf.channel_profile.name[0] != '\0') {
        for (vlan_itr = IPMC_LIB_vlan_map.begin(); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
            if (!vlan_itr->first.is_mvr) {
                // Only check against other MVR VLANs.
                continue;
            }

            if (vlan_itr->first.vid == vlan_key.vid) {
                // Don't check against ourselves. The thing is that both IGMP
                // and MLD use the same channel profile, so it's OK that if we
                // configure IGMP, then the MLD channel profile is the same and
                // vice versa. Therefore, we don't compare vlan_key.is_ipv4 with
                // vlan_itr's.
                continue;
            }

            if (!vlan_itr->second.conf.admin_active) {
                // Not active. Not interesting.
                continue;
            }

            if (strcmp(vlan_itr->second.conf.channel_profile.name, local_new_conf.channel_profile.name) == 0) {
                return VTSS_APPL_IPMC_LIB_RC_PROFILE_USED_BY_ANOTHER_MVR_VLAN;
            }
        }
    }

    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        VTSS_RC(IPMC_LIB_vlan_auto_vivify_check(vlan_key));

        // Still here? Get a new iterator
        if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
            // Nothing else we can do.
            return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
        }
    }

    if (memcmp(&local_new_conf, &vlan_itr->second, sizeof(local_new_conf)) == 0) {
        // No changes
        return VTSS_RC_OK;
    }

    IPMC_LIB_vlan_conf_changed(vlan_itr->second, local_new_conf);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_itr()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_itr(const vtss_appl_ipmc_lib_vlan_key_t *prev, vtss_appl_ipmc_lib_vlan_key_t *next, bool stay_in_this_key)
{
    ipmc_lib_vlan_itr_t vlan_itr;

    VTSS_RC(IPMC_LIB_ptr_check(next));

    IPMC_LIB_LOCK_SCOPE();

    if (prev) {
        // Here, we have a valid prev. Find the next from that one.
        vlan_itr = IPMC_LIB_vlan_map.greater_than(*prev);
    } else {
        if (stay_in_this_key) {
            // Since the previous VLAN is not specified, we don't know which
            // of IPMC/MVR and IGMP/MLD we should stay in.
            T_E("Unable to stay in this key when prev is nullptr");
            return VTSS_RC_ERROR;
        }

        // We don't have a valid prev. Get the first.
        vlan_itr = IPMC_LIB_vlan_map.begin();
    }

    if (vlan_itr == IPMC_LIB_vlan_map.end()) {
        // No next
        return VTSS_RC_ERROR;
    }

    // The VLAN map is first sorted by IPMC/MVR, then by IPv4/IPv6, and finally
    // by VID. This means that we can return VTSS_RC_ERROR as soon as is_mvr or
    // is_ipv4 changes.
    if (stay_in_this_key) {
        // Only compare is_mvr and is_ipv4.
        if (IPMC_LIB_keys_differ(*prev, vlan_itr->first)) {
            // Stepping out of this protocol or IP family
            return VTSS_RC_ERROR;
        }
    }

    *next = vlan_itr->first;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_ipmc_lib_vlan_name_to_vid()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_name_to_vid(const char *name, mesa_vid_t *vid)
{
    ipmc_lib_vlan_itr_t itr;

    VTSS_RC(IPMC_LIB_name_check(name));
    VTSS_RC(IPMC_LIB_ptr_check(vid));

    if (name[0] == '\0') {
        return VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_SPECIFIED;
    }

    IPMC_LIB_LOCK_SCOPE();

    for (itr = IPMC_LIB_vlan_map.begin(); itr != IPMC_LIB_vlan_map.end(); ++itr) {
        if (strcmp(itr->second.conf.name, name) == 0) {
            *vid = itr->first.vid;
            return VTSS_RC_OK;
        }
    }

    return VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_FOUND;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_port_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_port_conf_default_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_vlan_port_conf_t *conf)
{
    VTSS_RC(IPMC_LIB_key_mvr_check(key));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    *conf = IPMC_LIB_vlan_port_conf_defaults[IPMC_LIB_key_to_idx_get(key)];
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_port_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_port_conf_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, mesa_port_no_t port_no, vtss_appl_ipmc_lib_vlan_port_conf_t *conf)
{
    ipmc_lib_vlan_itr_t vlan_itr;

    VTSS_RC(IPMC_LIB_vlan_key_mvr_check(vlan_key));
    VTSS_RC(IPMC_LIB_port_no_check(port_no));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    IPMC_LIB_LOCK_SCOPE();

    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        // Not used by IPMC, so no need to auto-vivify VLAN.
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
    }

    *conf = vlan_itr->second.port_conf[port_no];
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_port_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_port_conf_set(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_vlan_port_conf_t *conf)
{
    ipmc_lib_vlan_itr_t            vlan_itr, vlan_itr2;
    vtss_appl_ipmc_lib_port_role_t other_role;

    VTSS_RC(IPMC_LIB_vlan_key_mvr_check(vlan_key));
    VTSS_RC(IPMC_LIB_port_no_check(port_no));
    VTSS_RC(IPMC_LIB_ptr_check(conf));

    if (conf->role != VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE   &&
        conf->role != VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE &&
        conf->role != VTSS_APPL_IPMC_LIB_PORT_ROLE_RECEIVER) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_PORT_ROLE;
    }

    T_D_PORT(port_no, "%s: %s", vlan_key, *conf);

    IPMC_LIB_LOCK_SCOPE();

    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        // Not used by IPMC, so no need to auto-vivify VLAN.
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
    }

    if (vlan_itr->second.port_conf[port_no].role == conf->role) {
        // No changes
        return VTSS_RC_OK;
    }

    // Check that no other MVR VLAN has set this port to source when this MVR
    // VLAN sets it to receiver and vice versa.
    if (conf->role != VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE) {
        for (vlan_itr2 = IPMC_LIB_vlan_map.begin(); vlan_itr2 != IPMC_LIB_vlan_map.end(); ++vlan_itr2) {
            if (vlan_itr == vlan_itr2) {
                // Don't check against ourselves
                continue;
            }

            if (!vlan_itr2->first.is_mvr) {
                // Only check against MVR VLANs.
                continue;
            }

            if (vlan_itr2->first.vid == vlan_itr->first.vid) {
                // Don't check against IGMP or MLD instance of this MVR VLAN.
                continue;
            }

            other_role = vlan_itr2->second.port_conf[port_no].role;

            if (other_role == VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE) {
                // Neither source nor receiver in this other MVR VLAN.
                continue;
            }

            if (other_role != conf->role) {
                return conf->role == VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE ? VTSS_APPL_IPMC_LIB_RC_ROLE_SOURCE_OTHER_RECEIVER : VTSS_APPL_IPMC_LIB_RC_ROLE_RECEIVER_OTHER_SOURCE;
            }
        }
    }

    IPMC_LIB_vlan_port_conf_changed(vlan_itr->second, port_no, *conf);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_port_itr()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_port_itr(const vtss_appl_ipmc_lib_vlan_key_t *vlan_key_prev, vtss_appl_ipmc_lib_vlan_key_t *vlan_key_next, const mesa_port_no_t *port_prev, mesa_port_no_t *port_next, bool stay_in_this_key)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    ipmc_lib_vlan_itr_t           vlan_itr;
    mesa_port_no_t                port_no;

    VTSS_RC(IPMC_LIB_ptr_check(vlan_key_next));
    VTSS_RC(IPMC_LIB_ptr_check(port_next));

    if (vlan_key_prev) {
        vlan_key = *vlan_key_prev;
    } else {
        if (stay_in_this_key) {
            // Since the previous VLAN is not specified, we don't know which
            // of IPMC/MVR and IGMP/MLD we should stay in.
            T_E("Unable to stay in this key when prev is nullptr");
            return VTSS_RC_ERROR;
        }

        // We should start with IPMC and IPv4, because that's how they are
        // sorted in the VLAN map.
        vlan_key.is_mvr = false;
        vlan_key.is_ipv4 = true;
    }

    if (port_prev) {
        port_no = *port_prev;
    } else {
        port_no = MESA_PORT_NO_NONE; // Indicates we should start over.
    }

    IPMC_LIB_LOCK_SCOPE();

    for (vlan_itr = IPMC_LIB_vlan_map.greater_than_or_equal(vlan_key); vlan_itr != IPMC_LIB_vlan_map.end(); ++vlan_itr) {
        if (stay_in_this_key) {
            // Only compare is_mvr and is_ipv4.
            if (IPMC_LIB_keys_differ(*vlan_key_prev, vlan_itr->first)) {
                return VTSS_RC_ERROR;
            }
        }

        if (vlan_itr->first != vlan_key) {
            // Start over on port
            port_no = MESA_PORT_NO_NONE;
        }

        if (port_no == MESA_PORT_NO_NONE) {
            *port_next = 0;
        } else if (port_no < IPMC_LIB_port_cnt - 1) {
            *port_next = port_no + 1;
        } else {
            port_no = MESA_PORT_NO_NONE;
            continue;
        }

        *vlan_key_next = vlan_itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_port_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_port_status_get(const vtss_appl_ipmc_lib_key_t &key, mesa_port_no_t port_no, vtss_appl_ipmc_lib_port_status_t *status)
{
    VTSS_RC(IPMC_LIB_key_check(key));
    VTSS_RC(IPMC_LIB_port_no_check(port_no));
    VTSS_RC(IPMC_LIB_ptr_check(status));

    IPMC_LIB_LOCK_SCOPE();

    *status = IPMC_LIB_global_state[IPMC_LIB_key_to_idx_get(key)].port_status[port_no];
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_status_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, vtss_appl_ipmc_lib_vlan_status_t *status)
{
    ipmc_lib_grp_map_t                 &grp_map = IPMC_LIB_global_lists.grp_map;
    vtss_appl_ipmc_lib_compatibility_t compat;
    ipmc_lib_vlan_itr_t                vlan_itr;
    ipmc_lib_grp_itr_t                 grp_itr;
    uint32_t                           now;

    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));
    VTSS_RC(IPMC_LIB_ptr_check(status));

    T_D("%s: Get status", vlan_key);

    IPMC_LIB_LOCK_SCOPE();

    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        T_D("%s: Can't find entry", vlan_key);
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
    }

    ipmc_lib_vlan_state_t &vlan_state = vlan_itr->second;
    *status = vlan_state.status;

    // Map from vlan_list to vtss_appl_ipmc_lib_vlan_status_t
    if (vlan_state.status.oper_state == VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
        now = vtss::uptime_seconds();

        // Instead of computing the older version host present timeouts whenever
        // they change in ipmc_lib_base.cxx, we do it only when retrieving the
        // status.
        compat = vlan_state.conf.compatibility;
        status->older_version_host_present_timeout_old = 0;
        status->older_version_host_present_timeout_gen = 0;
        status->grp_cnt = 0;
        for (grp_itr = grp_map.begin(); grp_itr != grp_map.end(); ++grp_itr) {
            if (grp_itr->first.vlan_key != vlan_state.vlan_key) {
                continue;
            }

            status->grp_cnt++;

            if (compat != VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO) {
                continue;
            }

            // Find the highest timeouts amongst all groups.
            status->older_version_host_present_timeout_old = MAX(grp_itr->second.grp_older_version_host_present_timeout_old, status->older_version_host_present_timeout_old);
            status->older_version_host_present_timeout_gen = MAX(grp_itr->second.grp_older_version_host_present_timeout_gen, status->older_version_host_present_timeout_gen);
        }

        // Convert to seconds from now instead of absolute timeout.
        status->older_version_host_present_timeout_old = IPMC_LIB_time_abs_to_rel(status->older_version_host_present_timeout_old, now);
        status->older_version_host_present_timeout_gen = IPMC_LIB_time_abs_to_rel(status->older_version_host_present_timeout_gen, now);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_grp_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_grp_status_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, const vtss_appl_ipmc_lib_ip_t *grp, vtss_appl_ipmc_lib_grp_status_t *status)
{
    ipmc_lib_grp_itr_t grp_itr;
    ipmc_lib_grp_key_t grp_key;
    ipmc_lib_grp_map_t &grp_map = IPMC_LIB_global_lists.grp_map;

    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));
    VTSS_RC(IPMC_LIB_ptr_check(grp));
    VTSS_RC(IPMC_LIB_ptr_check(status));

    vtss_clear(grp_key);
    grp_key.vlan_key = vlan_key;
    grp_key.grp_addr = *grp;

    IPMC_LIB_LOCK_SCOPE();

    if ((grp_itr = grp_map.find(grp_key)) == grp_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_GRP_ADDR;
    }

    status->port_list   = grp_itr->second.asm_state.include_port_list;
    status->hw_location = grp_itr->second.asm_state.hw_location;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_grp_itr()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_grp_itr(const vtss_appl_ipmc_lib_vlan_key_t *vlan_key_prev, vtss_appl_ipmc_lib_vlan_key_t *vlan_key_next, const vtss_appl_ipmc_lib_ip_t *grp_prev, vtss_appl_ipmc_lib_ip_t *grp_next, bool stay_in_this_key)
{
    ipmc_lib_grp_key_t grp_key;
    ipmc_lib_grp_itr_t grp_itr;
    ipmc_lib_grp_map_t &grp_map = IPMC_LIB_global_lists.grp_map;

    VTSS_RC(IPMC_LIB_ptr_check(vlan_key_next));
    VTSS_RC(IPMC_LIB_ptr_check(grp_next));

    vtss_clear(grp_key);
    if (vlan_key_prev) {
        grp_key.vlan_key = *vlan_key_prev;
    } else {
        if (stay_in_this_key) {
            // Since the previous vlan_key is not specified, we don't know which
            // protocol and IP family to stay in.
            T_E("Unable to stay in this protocol and IP family when vlan_key_prev is nullptr");
            return VTSS_RC_ERROR;
        }

        // We should start with IPMC and IPv4, because that's how they are
        // sorted in the group map.
        grp_key.vlan_key.is_mvr  = false;
        grp_key.vlan_key.is_ipv4 = true;
    }

    if (grp_prev) {
        grp_key.grp_addr = *grp_prev;
    } else {
        grp_key.grp_addr.is_ipv4 = grp_key.vlan_key.is_ipv4;
    }

    // The grp_map is sorted first by IPMC/MVR, then by IP family, then by VID,
    // and finally by IP address. So all IPv4 groups come before all IPv6
    // groups.
    if ((grp_itr = grp_map.greater_than(grp_key)) == grp_map.end()) {
        // No next.
        return VTSS_RC_ERROR;
    }

    if (stay_in_this_key) {
        // Only compare is_mvr and is_ipv4.
        if (IPMC_LIB_keys_differ(*vlan_key_prev, grp_itr->first.vlan_key)) {
            // Stepping out of this protocol or IP family
            return VTSS_RC_ERROR;
        }
    }

    *vlan_key_next = grp_itr->first.vlan_key;
    *grp_next      = grp_itr->first.grp_addr;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_src_status_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_src_status_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, const vtss_appl_ipmc_lib_ip_t *grp, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_ip_t *src, vtss_appl_ipmc_lib_src_status_t *status)
{
    ipmc_lib_grp_itr_t   grp_itr;
    ipmc_lib_grp_key_t   grp_key;
    ipmc_lib_grp_map_t   &grp_map = IPMC_LIB_global_lists.grp_map;
    ipmc_lib_src_itr_t   src_itr;
    ipmc_lib_src_state_t *src_state;
    uint32_t             now = vtss::uptime_seconds();

    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));
    VTSS_RC(IPMC_LIB_ptr_check(grp));
    VTSS_RC(IPMC_LIB_port_no_check(port_no));
    VTSS_RC(IPMC_LIB_ptr_check(src));
    VTSS_RC(IPMC_LIB_ptr_check(status));

    if (vlan_key.is_ipv4 != grp->is_ipv4) {
        return vlan_key.is_ipv4 ? VTSS_APPL_IPMC_LIB_RC_KEY_IS_IPV4_GRP_IS_IPV6 : VTSS_APPL_IPMC_LIB_RC_KEY_IS_IPV6_GRP_IS_IPV4;
    }

    if (grp->is_ipv4 != src->is_ipv4) {
        return grp->is_ipv4 ? VTSS_APPL_IPMC_LIB_RC_GRP_IS_IPV4_SRC_IS_IPV6 : VTSS_APPL_IPMC_LIB_RC_GRP_IS_IPV6_SRC_IS_IPV4;
    }

    vtss_clear(grp_key);
    grp_key.vlan_key = vlan_key;
    grp_key.grp_addr = *grp;

    IPMC_LIB_LOCK_SCOPE();

    if ((grp_itr = grp_map.find(grp_key)) == grp_map.end()) {
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_GRP_ADDR;
    }

    if (!grp_itr->second.active_ports[port_no]) {
        return VTSS_APPL_IPMC_LIB_RC_PORT_NO_ACTIVE_ON_GRP;
    }

    if (src->is_all_ones()) {
        // Looking for the ASM entry.
        src_state = &grp_itr->second.asm_state;
    } else {
        if ((src_itr = grp_itr->second.src_map.find(*src)) == grp_itr->second.src_map.end()) {
            return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_SRC_ADDR;
        }

        src_state = &src_itr->second;
    }

    vtss_clear(*status);
    status->filter_mode = grp_itr->second.exclude_mode_ports[port_no] ? VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE : VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE;

    status->grp_timeout = IPMC_LIB_time_abs_to_rel(grp_itr->second.ports[port_no].grp_timeout, now);
    status->forwarding  = src_state->include_port_list[port_no];
    status->src_timeout = IPMC_LIB_time_abs_to_rel(src_state->ports[port_no].src_timeout, now);
    status->hw_location = src_state->hw_location;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_src_itr()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_src_itr(const vtss_appl_ipmc_lib_vlan_key_t *vlan_key_prev, vtss_appl_ipmc_lib_vlan_key_t *vlan_key_next,
                                   const vtss_appl_ipmc_lib_ip_t       *grp_prev,      vtss_appl_ipmc_lib_ip_t       *grp_next,
                                   const mesa_port_no_t                *port_prev,     mesa_port_no_t                *port_next,
                                   const vtss_appl_ipmc_lib_ip_t       *src_prev,      vtss_appl_ipmc_lib_ip_t       *src_next, bool stay_in_this_key)
{
    vtss_appl_ipmc_lib_ip_t src;
    mesa_port_no_t          port_no, start_port_no;
    ipmc_lib_grp_key_t      grp_key;
    ipmc_lib_grp_itr_t      grp_itr;
    ipmc_lib_grp_map_t      &grp_map = IPMC_LIB_global_lists.grp_map;
    ipmc_lib_src_itr_t      src_itr;
    bool                    find_next_grp = false;

    VTSS_RC(IPMC_LIB_ptr_check(vlan_key_next));
    VTSS_RC(IPMC_LIB_ptr_check(grp_next));
    VTSS_RC(IPMC_LIB_ptr_check(port_next));
    VTSS_RC(IPMC_LIB_ptr_check(src_next));

    IPMC_LIB_LOCK_SCOPE();

    vtss_clear(grp_key);
    if (vlan_key_prev) {
        grp_key.vlan_key = *vlan_key_prev;
    } else {
        if (stay_in_this_key) {
            // Since the previous vlan_key is not specified, we don't know which
            // protocol and IP family to stay in.
            T_E("Unable to stay in this protocol and IP family when vlan_key_prev is nullptr");
            return VTSS_RC_ERROR;
        }

        // We should start with IPMC and IPv4, because that's how they are
        // sorted in the group map.
        grp_key.vlan_key.is_mvr  = false;
        grp_key.vlan_key.is_ipv4 = true;
    }

    if (grp_prev) {
        grp_key.grp_addr = *grp_prev;
    } else {
        grp_key.grp_addr.is_ipv4 = grp_key.vlan_key.is_ipv4;
    }

    if (port_prev) {
        port_no = *port_prev;
        if (port_no >= IPMC_LIB_port_cnt) {
            // Port number out of range. Start over.
            // Notice, that we can't get out of range if using this iterator
            // correctly, because it will always return a valid port number,
            // so this check is just a precaution.
            port_no = MESA_PORT_NO_NONE;
        }
    } else {
        port_no = MESA_PORT_NO_NONE;
    }

    if (!src_prev) {
        src.is_ipv4 = grp_key.grp_addr.is_ipv4;
        src.all_zeros_set();
    } else {
        if (src_prev->is_ipv4 != grp_key.grp_addr.is_ipv4) {
            src.is_ipv4 = grp_key.grp_addr.is_ipv4;
            src.all_zeros_set();
        } else {
            src = *src_prev;

            if (src.is_all_ones()) {
                // Go on with the next port.
                if (port_no != MESA_PORT_NO_NONE) {
                    if (++port_no >= IPMC_LIB_port_cnt) {
                        find_next_grp = true;
                    }
                } else {
                    find_next_grp = true;
                }

                src.all_zeros_set();
            }
        }
    }

    T_I("find_next_grp = %d, grp_key = %s, port_no = %u, src = %s", find_next_grp, grp_key, port_no, src);
    // See if we are already iterating over this <vid, grp>.
    if (find_next_grp || (grp_itr = grp_map.find(grp_key)) == grp_map.end()) {
        // We are not. Get the next one.
        if ((grp_itr = grp_map.greater_than(grp_key)) == grp_map.end()) {
            // No next.
            return VTSS_RC_ERROR;
        }

        if (stay_in_this_key) {
            // Only compare is_mvr and is_ipv4.
            if (IPMC_LIB_keys_differ(*vlan_key_prev, grp_itr->first.vlan_key)) {
                // Stepping out of this protocol or IP family
                return VTSS_RC_ERROR;
            }
        }

        // New group => new port and new source.
        port_no = MESA_PORT_NO_NONE;
        src.is_ipv4 = grp_itr->first.grp_addr.is_ipv4;
    } else {
        // We are already iterating over <vid, grp>. See where we got to.
    }

    // When we get here:
    //   * grp_itr is valid.
    //   * port_no is MESA_PORT_NO_NONE if we should start over using grp_itr,
    //     otherwise it points to the previous iteration's port_no.
    //   * src points to the previous entry if port_no is not MESA_PORT_NO_NONE.
    T_I("grp_itr->first = %s, port_no = %u, src = %s", grp_itr->first, port_no, src);
    while (1) {
        ipmc_lib_src_map_t &src_map = grp_itr->second.src_map;

        if (port_no == MESA_PORT_NO_NONE || !grp_itr->second.active_ports[port_no]) {
            // Either we are asked to start over on this group, or this group
            // is not active on the chosen port number.
            // Either way start over on the source and find the next active port
            // number in this group.
            src.all_zeros_set();

            // Look for the first active port.
            start_port_no = port_no == MESA_PORT_NO_NONE ? 0 : port_no + 1;
            for (port_no = start_port_no; port_no < IPMC_LIB_port_cnt; port_no++) {
                if (grp_itr->second.active_ports[port_no]) {
                    break;
                }
            }

            // If no more active ports in this group, port_no == IPMC_LIB_port_cnt.
        }

        T_I("grp_itr->first = %s, port_no = %u, src = %s", grp_itr->first, port_no, src);
        if (port_no == IPMC_LIB_port_cnt) {
            // No more active ports in this group. Get on with the next one.
            if ((grp_itr = grp_map.greater_than(grp_itr->first)) == grp_map.end()) {
                // No next
                return VTSS_RC_ERROR;
            }

            if (stay_in_this_key) {
                // Only compare is_mvr and is_ipv4.
                if (IPMC_LIB_keys_differ(*vlan_key_prev, grp_itr->first.vlan_key)) {
                    // Stepping out of this protocol or IP family
                    return VTSS_RC_ERROR;
                }
            }

            // New group => new port and source
            port_no = MESA_PORT_NO_NONE;
            src.is_ipv4 = grp_itr->first.grp_addr.is_ipv4;
            continue;
        }

        // Here, port_no is a valid port number for which we must look for the
        // next source.

        T_I("grp_itr->first = %s, port_no = %u, src = %s", grp_itr->first, port_no, src);

        // Time to find the next, higher source for this port number.
        if ((src_itr = src_map.greater_than(src)) == src_map.end()) {
            // No more sources. Time to populate the "catch remaining" entry,
            // which is the ASM entry, which always exists.

            // Tell caller that this is the catch-remaining (ASM) entry.
            src.all_ones_set();
            *vlan_key_next  = grp_itr->first.vlan_key;
            *grp_next       = grp_itr->first.grp_addr;
            *port_next      = port_no;
            *src_next        = src;

            T_I("grp_itr->first = %s, port_no = %u, src = %s", grp_itr->first, port_no, src);
            return VTSS_RC_OK;
        }

        src = src_itr->first;

        if (!src_itr->second.include_port_list[port_no] && !src_itr->second.exclude_port_list[port_no]) {
            // This source address is neither in the "Include List", the
            // "Exclude List" or the "Requested List", so it must belong to a
            // different port. Go on with the next source.
            T_I("grp_itr->first = %s, port_no = %u, src = %s", grp_itr->first, port_no, src);
            continue;
        }

        T_I("grp_itr->first = %s, port_no = %u, src = %s", grp_itr->first, port_no, src);

        // The source is active on this <G, port>
        *vlan_key_next  = grp_itr->first.vlan_key;
        *grp_next       = grp_itr->first.grp_addr;
        *port_next      = port_no;
        *src_next       = src;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_statistics_get()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_statistics_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, vtss_appl_ipmc_lib_vlan_statistics_t *statistics)
{
    ipmc_lib_vlan_itr_t vlan_itr;

    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));
    VTSS_RC(IPMC_LIB_ptr_check(statistics));

    T_D("%s: Get statistics", vlan_key);

    IPMC_LIB_LOCK_SCOPE();

    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        T_D("%s: Can't find entry", vlan_key);
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
    }

    *statistics = vlan_itr->second.statistics;

    // Gotta summarize some of the counters into general counters.
    if (vlan_key.is_ipv4) {
        vtss_appl_ipmc_lib_igmp_vlan_statistics_t &tx      = vlan_itr->second.statistics.tx.igmp;
        vtss_appl_ipmc_lib_igmp_vlan_statistics_t &rx_util = vlan_itr->second.statistics.rx.igmp.utilized;
        vtss_appl_ipmc_lib_igmp_vlan_statistics_t &rx_igno = vlan_itr->second.statistics.rx.igmp.ignored;

        statistics->tx_query          = tx.v1_query + tx.v2_g_query + tx.v2_gs_query + tx.v3_g_query + tx.v3_gs_query + tx.v3_gss_query;
        statistics->tx_specific_query =               tx.v2_g_query + tx.v2_gs_query +                 tx.v3_gs_query + tx.v3_gss_query;

        statistics->rx_query = rx_util.v1_query + rx_util.v2_g_query + rx_util.v2_gs_query + rx_util.v3_g_query + rx_util.v3_gs_query + rx_util.v3_gss_query +
                               rx_igno.v1_query + rx_igno.v2_g_query + rx_igno.v2_gs_query + rx_igno.v3_g_query + rx_igno.v3_gs_query + rx_igno.v3_gss_query;
    } else {
        vtss_appl_ipmc_lib_mld_vlan_statistics_t &tx      = vlan_itr->second.statistics.tx.mld;
        vtss_appl_ipmc_lib_mld_vlan_statistics_t &rx_util = vlan_itr->second.statistics.rx.mld.utilized;
        vtss_appl_ipmc_lib_mld_vlan_statistics_t &rx_igno = vlan_itr->second.statistics.rx.mld.ignored;

        statistics->tx_query          = tx.v1_g_query + tx.v1_gs_query + tx.v2_g_query + tx.v2_gs_query + tx.v2_gss_query;
        statistics->tx_specific_query =                 tx.v1_gs_query +                 tx.v2_gs_query + tx.v2_gss_query;

        statistics->rx_query = rx_util.v1_g_query + rx_util.v1_gs_query + rx_util.v2_g_query + rx_util.v2_gs_query + rx_util.v2_gss_query +
                               rx_igno.v1_g_query + rx_igno.v1_gs_query + rx_igno.v2_g_query + rx_igno.v2_gs_query + rx_igno.v2_gss_query;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_statistics_clear()
/******************************************************************************/
mesa_rc vtss_appl_ipmc_lib_vlan_statistics_clear(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key)
{
    ipmc_lib_vlan_itr_t vlan_itr;

    VTSS_RC(IPMC_LIB_vlan_key_check(vlan_key));

    IPMC_LIB_LOCK_SCOPE();

    if ((vlan_itr = IPMC_LIB_vlan_map.find(vlan_key)) == IPMC_LIB_vlan_map.end()) {
        T_D("%s: No such VID", vlan_key);
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
    }

    vtss_clear(vlan_itr->second.statistics);
    return VTSS_RC_OK;
}

/******************************************************************************/
// ipmc_lib_debug_thread_state_change()
/******************************************************************************/
void ipmc_lib_debug_thread_state_change(bool resume)
{
    IPMC_LIB_thread_running = resume;
}

/******************************************************************************/
// ipmc_lib_debug_src_dump()
/******************************************************************************/
void ipmc_lib_debug_src_dump(ipmc_lib_icli_pr_t pr)
{
    vtss_appl_ipmc_lib_src_status_t src_status;
    vtss_appl_ipmc_lib_vlan_key_t   vlan_key = {}, prev_vlan_key;
    vtss_appl_ipmc_lib_ip_t         grp = {}, src = {}, prev_grp;
    mesa_port_no_t                  port_no = MESA_PORT_NO_NONE, prev_port_no;
    bool                            first = true, is_asm, differs_from_prev;
    char                            who_buf[10], vid_buf[10], grp_buf[40], if_str[40], src_buf[40];
    mesa_rc                         rc;

    vlan_key.is_ipv4 = true; // Needed because IPv4 addresses come before IPv6.
    grp.is_ipv4      = true;

    prev_vlan_key = vlan_key;
    prev_grp      = grp;
    prev_port_no  = port_no;
    while (vtss_appl_ipmc_lib_src_itr(&vlan_key, &vlan_key, &grp, &grp, &port_no, &port_no, &src, &src) == VTSS_RC_OK) {
        if ((rc = vtss_appl_ipmc_lib_src_status_get(vlan_key, &grp, port_no, &src, &src_status)) != VTSS_RC_OK) {
            T_E("vtss_appl_ipmc_lib_src_status_get(%s, %s, %u, %s) failed: %s", vlan_key, grp, port_no, src, error_txt(rc));
            continue;
        }

        if (first) {
            pr("Who  VLAN Group Address                           Port        Source Address                          Filter Mode Grp Timeout Fwd Src Timeout H/W Loc\n");
            pr("---- ---- --------------------------------------- ----------- --------------------------------------- ----------- ----------- --- ----------- -------\n");
        }

        is_asm = src.is_all_ones();
        differs_from_prev = first || prev_vlan_key != vlan_key || prev_grp.is_ipv4 != grp.is_ipv4 || prev_grp != grp || prev_port_no != port_no;

        if (is_asm) {
            // This is the ASM entry
            if (differs_from_prev) {
                // No previous sources on this <vid, grp, port>.
                sprintf(src_buf, "Any");
            } else {
                // Catch remaining
                sprintf(src_buf, "Other");
            }
        }

        if (differs_from_prev) {
            // Print who, vid, grp, port
            sprintf(who_buf, "%s", vlan_key.is_mvr ? "MVR" : "IPMC");
            sprintf(vid_buf, "%4u", vlan_key.vid);
            (void)grp.print(grp_buf);
            (void)icli_port_info_txt_short(VTSS_USID_START, iport2uport(port_no), if_str);
        } else {
            // Don't re-print vid, grp, port
            who_buf[0] = '\0';
            vid_buf[0] = '\0';
            grp_buf[0] = '\0';
            if_str[0]  = '\0';
        }

        pr("%-4s %4s %-39s %-11s %-39s %-11s %11u %-3s %11u %s\n",
           who_buf, vid_buf, grp_buf, if_str, is_asm ? src_buf : src.print(src_buf),
           ipmc_lib_util_filter_mode_to_str(src_status.filter_mode), src_status.grp_timeout, src_status.forwarding ? "Yes" : "No", src_status.src_timeout, src_status.hw_location != VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE ? "Yes" : "No");

        prev_vlan_key = vlan_key;
        prev_grp      = grp;
        prev_port_no  = port_no;
        first         = false;
    }
}

/******************************************************************************/
// ipmc_lib_error_txt()
/******************************************************************************/
const char *ipmc_lib_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_IPMC_LIB_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR:
        return "Internal error. Check console for details";

    case VTSS_APPL_IPMC_LIB_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_IPMC_LIB_RC_MLD_NOT_SUPPORTED:
        return "MLD is not supported";

    case VTSS_APPL_IPMC_LIB_RC_IPMC_NOT_SUPPORTED:
        return "IPMC is not supported";

    case VTSS_APPL_IPMC_LIB_RC_MVR_NOT_SUPPORTED:
        return "MVR is not supported";

    case VTSS_APPL_IPMC_LIB_RC_FUNCTION_ONLY_SUPPORTED_BY_MVR:
        return "This function is only supported by MVR";

    case VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN:
        return "No such VLAN";

    case VTSS_APPL_IPMC_LIB_RC_VLAN_ALREADY_EXISTS:
        return "VLAN already exists";

    case VTSS_APPL_IPMC_LIB_RC_VLAN_LIMIT_REACHED:
        return "The maximum number of VLANs is reached";

    case VTSS_APPL_IPMC_LIB_RC_IFINDEX_NOT_VLAN:
        return "The interface index does not represent a VLAN";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_ID:
        return "Invalid VLAN ID";

    case VTSS_APPL_IPMC_LIB_RC_IFINDEX_NOT_PORT:
        return "The interface index does not represent a port";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_PORT_NUMBER:
        return "Invalid port number";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_LENGTH:
        return "Invalid VLAN name length. Must be a string of length 0 to " vtss_xstr(VTSS_APPL_IPMC_LIB_VLAN_NAME_LEN_MAX) " (excl. terminating NULL)";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS:
        return "Invalid VLAN name contents. First character must be from [a-zA-Z]. Remaining characters must be from a printable character excluding space and colon ([33-126], except 58)";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS_COLON:
        return "The VLAN name must not contain a colon. First character must be from [a-zA-Z]. Remaining characters must be from a printable character excluding space and colon ([33-126], except 58)";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS_ALL:
        return "The VLAN name 'all' (case-insensitively) is reserved";

    case VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_SPECIFIED:
        return "The VLAN name must be specified";

    case VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_FOUND:
        return "No such VLAN name";

    case VTSS_APPL_IPMC_LIB_RC_UNREGISTERED_FLOODING_ENABLE_CANNOT_BE_CHANGED:
        return "Unknown flooding cannot be changed by MVR. Use IPMC's management interface instead";

    case VTSS_APPL_IPMC_LIB_RC_PROXY_CANNOT_BE_CHANGED:
        return "Proxy enabledness cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_LEAVE_PROXY_CANNOT_BE_CHANGED:
        return "Leave Proxy enabledness cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_CANNOT_BE_CHANGED:
        return "SSM prefix address cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_LEN_CANNOT_BE_CHANGED:
        return "SSM prefix length cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_NOT_MC:
        return "SSM prefix address is not a multicast address";

    case VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_LEN_INVALID:
        return "Invalid SSM prefix length";

    case VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_BITS_OUTSIDE_OF_MASK_SET:
        return "Not all bits beyond the prefix length are zero in the SSM prefix address";

    case VTSS_APPL_IPMC_LIB_RC_ROUTER_CANNOT_BE_CHANGED:
        return "Globally marking a port as a router port is not possible by MVR. Use the per-VLAN port configuration instead";

    case VTSS_APPL_IPMC_LIB_RC_MAX_GROUP_CNT_CANNOT_BE_CHANGED:
        return "The maximum group count (throttling) cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_KEY_CANNOT_BE_CHANGED:
        return "The filtering profile cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_MAX_GROUP_CNT_INVALID:
        return "Invalid maximum group count (throttling). Valid values are from 1 to 10 (0 to disable the check)";

    case VTSS_APPL_IPMC_LIB_RC_QUERIER_ADDR_NOT_IPV4:
        return "The IGMP querier address is not specified as IPv4";

    case VTSS_APPL_IPMC_LIB_RC_QUERIER_ADDR_NOT_UNICAST:
        return "The IGMP querier address is not a unicast IPv4 address, or it is in the loopback range (127.0.0.0/8).";

    case VTSS_APPL_IPMC_LIB_RC_ADMIN_ACTIVE_CANNOT_BE_CHANGED:
        return "MVR VLANs cannot be set inactive. Delete it to make inactive";

    case VTSS_APPL_IPMC_LIB_RC_VLAN_MODE_CANNOT_BE_CHANGED:
        return "The VLAN mode property (dynamic/compatible) cannot be changed";

    case VTSS_APPL_IPMC_LIB_RC_TX_TAGGED_CANNOT_BE_CHANGED:
        return "The Tx Tagged property cannot be changed";

    case VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_SUPPORTED:
        return "Naming of IPMC VLANs is not supported";

    case VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY_CANNOT_BE_CHANGED:
        return "The compatibility cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_PCP_CANNOT_BE_CHANGED:
        return "The priority (PCP value) cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_RV_CANNOT_BE_CHANGED:
        return "The robustness variable (RV) cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_QI_CANNOT_BE_CHANGED:
        return "The query interval (QI) cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_QRI_CANNOT_BE_CHANGED:
        return "The maximum response time (QRI; Query Response Interval) cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_LMQI_CANNOT_BE_CHANGED:
        return "The last member query interval (LMQI/LLQI) cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_URI_CANNOT_BE_CHANGED:
        return "The unsolicited report interval (URI) cannot be changed in this version of the software";

    case VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY:
        return "Compatibility is set to an invalid value";

    case VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY_OLD_WITH_MLD:
        return "Compatibility cannot be set to \"old\" for MLD";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_PCP:
        return "Invalid priority (PCP value). Must be a value between 0 and 7";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_ROBUSTNESS_VARIABLE:
        return "The robustness variable (RV) must be a value between 2 and 255";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_QUERY_INTERVAL:
        return "The query interval (QI) must be a value between 1 and 31744 seconds";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_QUERY_RESPONSE_INTERVAL:
        return "The maximum response time (QRI; Query Response Interval)  must be a value between 0 and 31744 (measured in 10ths of a second)";

    case VTSS_APPL_IPMC_LIB_RC_QRI_QI_INVALID:
        return "The number of seconds represented by the query response interval (QRI) must be less than the query interval (QI). To change, first disable querier election or snooping for this VLAN, then change and finally re-enable querier election or snooping";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_LMQI:
        return "The last member query interval (LMQI/LLQI) must be a value between 0 and 31744 (measured in 10ths of a second)";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_URI:
        return "The unsolicited report interval (URI) must be a value between 1 and 31744 seconds";

    case VTSS_APPL_IPMC_LIB_RC_CHANNEL_PROFILES_NOT_SUPPORTED:
        return "Per-VLAN channel profiles are not supported by IPMC";

    case VTSS_APPL_IPMC_LIB_RC_INVALID_PORT_ROLE:
        return "Invalid port role";

    case VTSS_APPL_IPMC_LIB_RC_ROLE_SOURCE_OTHER_RECEIVER:
        return "Cannot configure port as source, since another MVR VLAN has configured it as receiver";

    case VTSS_APPL_IPMC_LIB_RC_ROLE_RECEIVER_OTHER_SOURCE:
        return "Cannot configure port as receiver, since another MVR VLAN has configured it as source";

    case VTSS_APPL_IPMC_LIB_RC_NO_SUCH_GRP_ADDR:
        return "The requested <VLAN ID, Group Address> was not found";

    case VTSS_APPL_IPMC_LIB_RC_PORT_NO_ACTIVE_ON_GRP:
        return "The port is not active on the requested <VLAN ID, Group Address>";

    case VTSS_APPL_IPMC_LIB_RC_NO_SUCH_SRC_ADDR:
        return "The requested source address was not found within the requested <VLAN ID, Group Address>";

    case VTSS_APPL_IPMC_LIB_RC_KEY_IS_IPV4_GRP_IS_IPV6:
        return "Requesting an IPv6 group address in an IGMP VLAN";

    case VTSS_APPL_IPMC_LIB_RC_KEY_IS_IPV6_GRP_IS_IPV4:
        return "Requesting an IPv4 group address in an MLD VLAN";

    case VTSS_APPL_IPMC_LIB_RC_GRP_IS_IPV4_SRC_IS_IPV6:
        return "Requesting an IPv6 source address within an IPv4 group address";

    case VTSS_APPL_IPMC_LIB_RC_GRP_IS_IPV6_SRC_IS_IPV4:
        return "Requesting an IPv4 source address within an IPv6 group address";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_EMPTY:
        return "Profile name cannot be empty";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_TOO_LONG:
        return "Profile name too long";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_CONTENTS:
        return "Invalid profile name contents. All characters must be from a printable character excluding space ([33-126])";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_CONTENTS_ALL:
        return "The profile name 'all' (case-insensitively) is reserved";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_DSCR_TOO_LONG:
        return "The profile description if too long";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_DSCR_CONTENTS:
        return "Invalid profile description contents. Only characters in [32-126] are accepted";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_NO_SUCH:
        return "No such profile name";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_USED_BY_ANOTHER_MVR_VLAN:
        return "Channel profile used by another MVR VLAN";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_LIMIT_REACHED:
        return "The maximum number of profiles reached";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_EMPTY:
        return "Range name cannot be empty";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_TOO_LONG:
        return "Range name too long";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_CONTENTS:
        return "Invalid range name contents. All characters must be from a printable character excluding space ([33-126])";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_CONTENTS_ALL:
        return "The range name 'all' (case-insensitively) is reserved";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NO_SUCH:
        return "No such range name";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_LIMIT_REACHED:
        return "The maximum number of ranges reached";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_INVALID_TYPE:
        return "Invalid range type. Must be either IPv4 or IPv6";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_AND_END_NOT_SAME_IP_VERSION:
        return "Range's start address IP version differs from Range's end address IP version";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_NOT_IPV4_MC:
        return "Range's start address is not a multicast IPv4 address";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_END_NOT_IPV4_MC:
        return "Range's end address is not a multicast IPv4 address";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_NOT_IPV6_MC:
        return "Range's start address is not a multicast IPv6 address";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_END_NOT_IPV6_MC:
        return "Range's end address is not a multicast IPv6 address";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_GREATER_THAN_END_IPV4:
        return "Range's IPv4 end address must be greater than or equal to its start address";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_GREATER_THAN_END_IPV6:
        return "Range's IPv6 end address must be greater than or equal to its start address";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_RULE_NO_SUCH:
        return "The profile does not use the specified range as a rule";

    case VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_INSERT_BEFORE_NO_SUCH:
        return "The range to insert this rule before does not exist in this profile";

    default:
        T_E("Unknown error code (%u)", rc);
        return "IPMC-LIB: Unknown error code";
    }
}

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
VTSS_PRE_DECLS void ipmc_lib_profile_mib_init(void);
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC) && defined(VTSS_SW_OPTION_SMB_IPMC)
VTSS_PRE_DECLS void vtss_appl_ipmc_lib_profile_json_init(void);
#endif

extern "C" int ipmc_lib_icli_cmd_register(void);

/******************************************************************************/
// ipmc_lib_init()
/******************************************************************************/
mesa_rc ipmc_lib_init(vtss_init_data_t *data)
{
    mesa_rc rc;

#if defined(VTSS_SW_OPTION_SMB_IPMC)
    mesa_rc ipmc_lib_profile_init(vtss_init_data_t *data);
    VTSS_RC(ipmc_lib_profile_init(data));
#endif

    mesa_rc ipmc_lib_pdu_init(vtss_init_data_t *data);
    VTSS_RC(ipmc_lib_pdu_init(data));

    switch (data->cmd) {
    case INIT_CMD_INIT:
        IPMC_LIB_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_COUNT) - fast_cap(MEBA_CAP_CPU_PORTS_COUNT);
        IPMC_LIB_capabilities_set();
        IPMC_LIB_global_state_populate(false);
        IPMC_LIB_rx_bip_buf_init();
        critd_init(&IPMC_LIB_crit, "ipmc_lib", VTSS_MODULE_ID_IPMC_LIB, CRITD_TYPE_MUTEX);

        // Subscribe to VLAN interface changes
        if ((rc = vtss_ip_if_callback_add(IPMC_LIB_ip_vlan_interface_change_callback)) != VTSS_RC_OK) {
            T_E("vtss_ip_if_callback_add() failed: %s", error_txt(rc));
        }

        // Subscribe to static link aggregation and LACP changes
        aggr_change_register(IPMC_LIB_aggr_change_callback);

        // Subscribe to VLAN membership changes
        vlan_membership_change_register(VTSS_MODULE_ID_IPMC_LIB, IPMC_LIB_vlan_membership_change_callback);

#if defined(VTSS_SW_OPTION_PRIVATE_MIB) && defined(VTSS_SW_OPTION_SMB_IPMC)
        // Register IPMC Profile private MIB
        ipmc_lib_profile_mib_init();
#endif
#if defined(VTSS_SW_OPTION_JSON_RPC) && defined(VTSS_SW_OPTION_SMB_IPMC)
        vtss_appl_ipmc_lib_profile_json_init();
#endif

        ipmc_lib_icli_cmd_register();

#if defined(VTSS_SW_OPTION_ICFG) && defined(VTSS_SW_OPTION_SMB_IPMC)
        mesa_rc ipmc_lib_icfg_init(void);
        if (ipmc_lib_icfg_init() != VTSS_RC_OK) {
            T_E("ipmc_lib_icfg_init failed!");
        }
#endif

#ifdef VTSS_SW_OPTION_SMB_IPMC
        IPMC_LIB_profile_change_notifications.init();
#endif

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           IPMC_LIB_tick_thread,
                           0,
                           "IPMC Library",
                           nullptr,
                           0,
                           &IPMC_LIB_thread_handle,
                           &IPMC_LIB_thread_block);

        critd_init(&IPMC_LIB_pdu_crit, "ipmc_lib_pdu", VTSS_MODULE_ID_IPMC_LIB, CRITD_TYPE_MUTEX);
        vtss_flag_init(&IPMC_LIB_rx_flag);
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           IPMC_LIB_rx_thread,
                           0,
                           "IPMC_LIB Packet Rx",
                           nullptr,
                           0,
                           &IPMC_LIB_rx_thread_handle,
                           &IPMC_LIB_rx_thread_block);
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_MSTP
        (void)l2_stp_state_change_register(IPMC_LIB_stp_state_change_callback);
#endif

        // Register for port link state change events
        (void)port_change_register(VTSS_MODULE_ID_IPMC_LIB, IPMC_LIB_port_state_change_callback);
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            IPMC_LIB_LOCK_SCOPE();
            IPMC_LIB_load_defaults();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE: {
        IPMC_LIB_LOCK_SCOPE();
        IPMC_LIB_load_defaults();
        break;
    }

    case INIT_CMD_ICFG_LOADING_POST: {
        IPMC_LIB_LOCK_SCOPE();

        // Up until now, all configuration gathered during the
        // INIT_CMD_ICFG_LOADING_PRE phase have not been applied to H/W yet.
        // This is in order to ensure that other modules that we depend on are
        // ready to accept changes.
        IPMC_LIB_start_all();
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

