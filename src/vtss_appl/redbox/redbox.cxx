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

#include "main.h"
#include "critd_api.h"
#include <microchip/ethernet/switch/api.h>
#include <vtss/appl/redbox.h>
#ifdef VTSS_SW_OPTION_MSTP
#include <vtss/appl/mstp.h>  /* For vtss_appl_mstp_interface_config_get() */
#include "mstp_api.h"        /* For mstp_register_config_change_cb()      */
#endif
#include "redbox_api.h"
#include "redbox_base.hxx"
#include "redbox_expose.hxx"
#include "redbox_lock.hxx"
#include "redbox_timer.hxx"
#include "redbox_trace.h"
#include "conf_api.h"        /* For conf_mgmt_mac_addr_get()              */
#include "mac_utils.hxx"     /* For mesa_mac_t::operator==()              */
#include "misc_api.h"        /* For misc_mac_txt()                        */
#include "port_api.h"        /* For port_change_register()                */
#include "vlan_api.h"        /* For vlan_mgmt_port_conf_default_get()     */
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/event-handler.hxx>

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void redbox_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void redbox_json_init(void);
#endif

static vtss_trace_reg_t REDBOX_trace_reg = {
    VTSS_TRACE_MODULE_ID, "Redbox", "Redbox (PRP/HSR)"
};

static vtss_trace_grp_t REDBOX_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [REDBOX_TRACE_GRP_BASE] = {
        "base",
        "Base",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [REDBOX_TRACE_GRP_CALLBACK] = {
        "callback",
        "Callbacks",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [REDBOX_TRACE_GRP_FRAME_RX] = {
        "rx",
        "Frame Rx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [REDBOX_TRACE_GRP_FRAME_TX] = {
        "tx",
        "Frame Tx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [REDBOX_TRACE_GRP_ICLI] = {
        "icli",
        "CLI printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [REDBOX_TRACE_GRP_API] = {
        "api",
        "Switch API printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [REDBOX_TRACE_GRP_NOTIF] = {
        "notif",
        "Notification updates",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [REDBOX_TRACE_GRP_TIMER] = {
        "timer",
        "Timer",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&REDBOX_trace_reg, REDBOX_trace_grps);

// The RedBoxes have FIFOs that can cope with 2000 byte frames.
// The LRE ports must be configured to discard frames of size bigger than this
// number.
#define REDBOX_MTU_MAX_LRE_PORTS 2000

// Non-LRE ports don't have a PRP trailer or HSR tag and must discard frames of
// size bigger than the MTU on the LRE ports less 6 bytes for a PRP trailer or
// HSR tag.
// Don't compute this from REDBOX_MTU_MAX_LRE_PORTS, because this is also used
// in a vtss_xstr() construct.
#define REDBOX_MTU_MAX_NON_LRE_PORTS 1994

critd_t                                                             REDBOX_crit;
uint32_t                                                            REDBOX_cap_port_cnt;
redbox_map_t                                                        REDBOX_map;
vtss_appl_redbox_capabilities_t                                     REDBOX_cap;
static vtss_appl_redbox_conf_t                                      REDBOX_default_conf;
static bool                                                         REDBOX_started;
static bool                                                         REDBOX_vlan_membership_updates_started;
static mesa_mac_t                                                   REDBOX_chassis_mac;
static CapArray<mesa_rb_cap_t, MESA_CAP_L2_REDBOX_CNT>              REDBOX_ports; // 0-based indices
static CapArray<redbox_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> REDBOX_port_states;
static mesa_etype_t                                                 REDBOX_s_custom_tpid;

// redbox_notification_status holds the per-instance state that one can get
// notifications on, that being SNMP traps or JSON notifications. Each row in
// this table is a struct of type vtss_appl_redbox_notification_status_t.
redbox_notification_status_t redbox_notification_status("redbox_notification_status", VTSS_MODULE_ID_REDBOX);

/******************************************************************************/
// vtss_appl_redbox_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_redbox_conf_t &conf)
{
    o << "{mode = "                              << redbox_util_mode_to_str(conf.mode)
      << ", port_a = "                           << conf.port_a
      << ", port_b = "                           << conf.port_b
      << ", hsr_mode = "                         << redbox_util_hsr_mode_to_str(conf.hsr_mode)
      << ", duplicate_discard = "                << conf.duplicate_discard
      << ", net_id = "                           << conf.net_id
      << ", lan_id = "                           << redbox_util_lan_id_to_str(conf.lan_id)
      << ", nt_age_time_secs = "                 << conf.nt_age_time_secs
      << ", pnt_age_time_secs = "                << conf.pnt_age_time_secs
      << ", duplicate_discard_age_time_msecs = " << conf.duplicate_discard_age_time_msecs
      << ", sv_vlan = "                          << conf.sv_vlan
      << ", sv_pcp = "                           << conf.sv_pcp
      << ", sv_dmac_lsb = "                      << "0x" << vtss::hex(conf.sv_dmac_lsb)
      << ", sv_frame_interval_secs = "           << conf.sv_frame_interval_secs
      << ", sv_xlat_prp_to_hsr = "               << conf.sv_xlat_prp_to_hsr
      << ", sv_xlat_hsr_to_prp = "               << conf.sv_xlat_hsr_to_prp
      << ", admin_active = "                     << conf.admin_active
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_redbox_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_redbox_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// REDBOX_capabilities_set()
// This is the only place where we define maximum values for various parameters,
// so if you need different max. values, change here - only!
/******************************************************************************/
static void REDBOX_capabilities_set(void)
{
    int     i;
    mesa_rc rc;

    REDBOX_cap.inst_cnt_max                         = fast_cap(MESA_CAP_L2_REDBOX_CNT);
    REDBOX_cap.nt_pnt_size                          =  4096; // RBNTBD: Educated guess
    REDBOX_cap.nt_age_time_secs_min                 =     1;
    REDBOX_cap.nt_age_time_secs_max                 =    65;
    REDBOX_cap.pnt_age_time_secs_min                =     1;
    REDBOX_cap.pnt_age_time_secs_max                =    65;
    REDBOX_cap.duplicate_discard_age_time_msecs_min =    10;
    REDBOX_cap.duplicate_discard_age_time_msecs_max = 10000;
    REDBOX_cap.sv_frame_interval_secs_min           =     1;
    REDBOX_cap.sv_frame_interval_secs_max           =    60;
    REDBOX_cap.statistics_poll_interval_secs        =    10;
    REDBOX_cap.alarm_raised_time_secs               =    20; // Don't make this smaller than statistics_poll_interval_secs.

    if (REDBOX_cap.inst_cnt_max == 0) {
        return;
    }

    // Fill in the REDBOX_ports CapArray, which indicates the ports that can be
    // used for a given redbox instance. The API uses 0-based redbox IDs, while
    // the application uses 1-based.
    for (i = 0; i < REDBOX_cap.inst_cnt_max; i++) {
        if ((rc = mesa_rb_cap_get(nullptr, i, &REDBOX_ports[i])) != VTSS_RC_OK) {
            T_EG(REDBOX_TRACE_GRP_API, "mesa_rb_cap_get(%u) failed: %s", i, error_txt(rc));
            vtss_clear(REDBOX_ports[i]);
        }
    }
}

/******************************************************************************/
// REDBOX_default_conf_set()
// This is the only place where we need to set defaults.
/******************************************************************************/
static void REDBOX_default_conf_set(void)
{
    vtss_clear(REDBOX_default_conf);
    REDBOX_default_conf.mode                             = VTSS_APPL_REDBOX_MODE_PRP_SAN;
    REDBOX_default_conf.port_a                           = VTSS_IFINDEX_NONE;
    REDBOX_default_conf.port_b                           = VTSS_IFINDEX_NONE;
    REDBOX_default_conf.hsr_mode                         = VTSS_APPL_REDBOX_HSR_MODE_H;
    REDBOX_default_conf.duplicate_discard                = true;
    REDBOX_default_conf.net_id                           = 1;
    REDBOX_default_conf.lan_id                           = VTSS_APPL_REDBOX_LAN_ID_A;
    REDBOX_default_conf.nt_age_time_secs                 = 60;
    REDBOX_default_conf.pnt_age_time_secs                = 60;
    REDBOX_default_conf.duplicate_discard_age_time_msecs = 40;
    REDBOX_default_conf.sv_vlan                          = 0; // Native
    REDBOX_default_conf.sv_pcp                           = 7;
    REDBOX_default_conf.sv_dmac_lsb                      = 0x00;
    REDBOX_default_conf.sv_frame_interval_secs           = 2;
    REDBOX_default_conf.sv_xlat_prp_to_hsr               = true;
    REDBOX_default_conf.sv_xlat_hsr_to_prp               = true;
    REDBOX_default_conf.admin_active                     = false;
}

/******************************************************************************/
// REDBOX_module_enabled()
/******************************************************************************/
static bool REDBOX_module_enabled(void)
{
    // The redbox module is included in istax_arm_multi.mk, but not all
    // ARM7-based chips have redbox H/W support, so if it doesn't, we need to
    // runtime-disable ourselves.
    return REDBOX_cap.inst_cnt_max != 0;
}

/******************************************************************************/
// REDBOX_supported()
/******************************************************************************/
static mesa_rc REDBOX_supported(void)
{
    return REDBOX_cap.inst_cnt_max == 0 ? (mesa_rc)VTSS_APPL_REDBOX_RC_NOT_SUPPORTED : VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_vlans_in_common()
/******************************************************************************/
static bool REDBOX_vlans_in_common(redbox_port_state_t *port_state_1, redbox_port_state_t *port_state_2)
{
    int i;

    for (i = 0; i < ARRSZ(port_state_1->vlan_memberships); i++) {
        if ((port_state_1->vlan_memberships[i] & port_state_2->vlan_memberships[i]) != 0) {
            return true;
        }
    }

    return false;
}

/******************************************************************************/
// REDBOX_oper_warnings_stp_update()
/******************************************************************************/
static void REDBOX_oper_warnings_stp_update(redbox_state_t &redbox_state)
{
    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_STP_ENABLED_INTERLINK;

    if (redbox_state.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        return;
    }

#ifdef VTSS_SW_OPTION_MSTP
    vtss_appl_mstp_port_config_t mstp_conf;
    mesa_rc                      rc;

    // It's enough to make sure that the interlink port has spanning tree
    // disabled, because even if force-transmitting BPDUs to the unconnected
    // port, it won't get out of the LRE ports, because the physical mux that
    // selects where the physical port is connected is now connected to the
    // RedBox rather than the switch port.
    // We can use the status' port_c, which is an ifindex updated by the base
    // whenever the instance gets activated.
    if ((rc = vtss_appl_mstp_interface_config_get(redbox_state.status.port_c, &mstp_conf)) != VTSS_RC_OK) {
        T_E("%u: vtss_appl_mstp_interface_config_get(%s) failed: %s", redbox_state.inst, redbox_state.status.port_c, error_txt(rc));
        mstp_conf.enable = true; // Pretend it is enabled.
    }

    if (mstp_conf.enable) {
        // Spanning tree is enabled on interlink port.
        T_I("%u: STP enabled on %s", redbox_state.inst, redbox_state.status.port_c);
        redbox_state.status.oper_warnings |= VTSS_APPL_REDBOX_OPER_WARNING_STP_ENABLED_INTERLINK;
    }
#else
    // STP cannot be enabled on any ports when not included in the build.
#endif
}

//*****************************************************************************/
// REDBOX_oper_warnings_vlan_update()
/******************************************************************************/
static void REDBOX_oper_warnings_vlan_update(redbox_state_t &redbox_state)
{
    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_C_TAGGED;
    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_MEMBER_OF_VLAN;

    if (redbox_state.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        return;
    }

    // The base picks up the value of this one on the fly, so no need to invoke
    // base if it changes.
    redbox_state.interlink_member_of_tx_vlan = VTSS_BF_GET(redbox_state.interlink_port_state_get()->vlan_memberships, redbox_state.tx_vlan_get()) != 0;

    // The VLAN classification is part of the switchcore, and therefore, only
    // the interlink port is of interest here.
    if (redbox_state.interlink_port_state_get()->tpid != VTSS_APPL_VLAN_C_TAG_ETHERTYPE) {
        redbox_state.status.oper_warnings |= VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_C_TAGGED;
    }

    if (!redbox_state.interlink_member_of_tx_vlan) {
        redbox_state.status.oper_warnings |= VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_MEMBER_OF_VLAN;
    }
}

//*****************************************************************************/
// REDBOX_oper_warnings_mtu_update()
/******************************************************************************/
static void REDBOX_oper_warnings_mtu_update(redbox_state_t &redbox_state)
{
    redbox_port_state_t *interlink_port_state, *this_port_state;
    mesa_port_no_t      port_no;

    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_MTU_TOO_HIGH_LRE_PORTS;
    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_MTU_TOO_HIGH_NON_LRE_PORTS;

    if (redbox_state.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        return;
    }

    interlink_port_state = redbox_state.interlink_port_state_get();

    // The MTU is checked at the physical port level, so both Port A and Port B
    // are of interest here.
    // We loop through all ports, because we also have warnings for non-LRE
    // ports, because they may send to the LRE ports.
    for (port_no = 0; port_no < REDBOX_cap_port_cnt; port_no++) {
        this_port_state = &REDBOX_port_states[port_no];

        if (redbox_state.using_port_no(port_no)) {
            if (this_port_state->mtu > REDBOX_MTU_MAX_LRE_PORTS) {
                redbox_state.status.oper_warnings |= VTSS_APPL_REDBOX_OPER_WARNING_MTU_TOO_HIGH_LRE_PORTS;
            }
        } else {
            // We gotta ask the base whether this port is part of another RB. If
            // so, that other RB will raise a warning, but we shouldn't.
            if (!redbox_base_port_is_lre_port(port_no)) {
                // It's a normal switch port. If port_no has VLANs in common
                // with the LRE port, we need to raise a warning.
                if (REDBOX_vlans_in_common(interlink_port_state, this_port_state)) {
                    if (this_port_state->mtu > REDBOX_MTU_MAX_NON_LRE_PORTS) {
                        redbox_state.status.oper_warnings |= VTSS_APPL_REDBOX_OPER_WARNING_MTU_TOO_HIGH_NON_LRE_PORTS;
                    }
                }
            }
        }
    }
}

/******************************************************************************/
// REDBOX_oper_warnings_neighbor_redbox_update()
/******************************************************************************/
static void REDBOX_oper_warnings_neighbor_redbox_update(redbox_state_t &redbox_state)
{
    vtss_appl_redbox_port_type_t port_type;
    redbox_itr_t                 neighbor_redbox_itr;
    vtss_ifindex_t               ifindex, neighbor_ifindex;
    redbox_port_state_t          *our_port_state, *neighbor_port_state;
    uint32_t                     neighbor_inst;

    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_NOT_CONFIGURED;
    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_NOT_ACTIVE;
    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_PORT_A_NOT_SET_TO_NEIGHBOR;
    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_PORT_B_NOT_SET_TO_NEIGHBOR;
    redbox_state.status.oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_OVERLAPPING_VLANS;

    if (redbox_state.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        // Nothing else to do.
        return;
    }

    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_B; port_type++) {
        ifindex = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? redbox_state.conf.port_a : redbox_state.conf.port_b;
        if (ifindex != VTSS_IFINDEX_REDBOX_NEIGHBOR) {
            continue;
        }

        neighbor_inst = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? redbox_state.inst - 1 : redbox_state.inst + 1;

        if ((neighbor_redbox_itr = REDBOX_map.find(neighbor_inst)) == REDBOX_map.end()) {
            redbox_state.status.oper_warnings |= VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_NOT_CONFIGURED;
            continue;
        }

        if (neighbor_redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            redbox_state.status.oper_warnings |= VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_NOT_ACTIVE;
            continue;
        }

        // If neighbor's interface towards us is not set to neighbor, it's also
        // an operational warning *for us* (not for the neighbor, because from
        // the neighbor's perspective, everything looks fine).
        // Our port-a is connected to neighbor's port-b and vice versa.
        neighbor_ifindex = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? neighbor_redbox_itr->second.conf.port_b : neighbor_redbox_itr->second.conf.port_a;
        if (neighbor_ifindex != VTSS_IFINDEX_REDBOX_NEIGHBOR) {
            redbox_state.status.oper_warnings |= (port_type == VTSS_APPL_REDBOX_PORT_TYPE_B ? VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_PORT_A_NOT_SET_TO_NEIGHBOR : VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_PORT_B_NOT_SET_TO_NEIGHBOR);
        } else {
            // If there's overlap between our VLANs and the neighbor's
            // VLANs, there will be loops in the network, because frames
            // forwarded from e.g. our LRE port towards both the other LRE
            // port and the interlink will arrive twice at the switch core
            // and go back to the other RB instance's interlink and get
            // forwarded towards both LRE ports of which one is the internal
            // connection, and so forth. So warn about that as well.
            our_port_state      = redbox_state.interlink_port_state_get();
            neighbor_port_state = neighbor_redbox_itr->second.interlink_port_state_get();

            if (REDBOX_vlans_in_common(our_port_state, neighbor_port_state)) {
                T_R("%u: Coinciding VLANs with neighbor", redbox_state.inst);
                redbox_state.status.oper_warnings |= VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_OVERLAPPING_VLANS;
                break;
            }
        }
    }
}

/******************************************************************************/
// REDBOX_oper_warnings_update()
/******************************************************************************/
static void REDBOX_oper_warnings_update(redbox_state_t &redbox_state, int trace_level = VTSS_TRACE_LVL_INFO)
{
    char buf[400];

    // Update operational warnings arosen from VLAN configuration
    REDBOX_oper_warnings_vlan_update(redbox_state);

    // Update operational warnings arosen from port configuration.
    REDBOX_oper_warnings_mtu_update(redbox_state);

    // Update operational warnings arosen from STP configuration.
    REDBOX_oper_warnings_stp_update(redbox_state);

    // Check to see if the neighboring RedBox is active and OK.
    REDBOX_oper_warnings_neighbor_redbox_update(redbox_state);

    T(VTSS_TRACE_GRP_DEFAULT, trace_level, "%u: oper_warnings = 0x%08x = %s", redbox_state.inst, redbox_state.status.oper_warnings, redbox_util_oper_warnings_to_str(buf, sizeof(buf), redbox_state.status.oper_warnings));
}

/******************************************************************************/
// REDBOX_oper_warnings_update_all()
/******************************************************************************/
static void REDBOX_oper_warnings_update_all(int trace_level = VTSS_TRACE_LVL_INFO)
{
    redbox_itr_t redbox_itr;

    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        REDBOX_oper_warnings_update(redbox_itr->second, trace_level);
    }
}

//*****************************************************************************/
// REDBOX_vlan_memberships_cache_and_oper_warnings_update_all()
//*****************************************************************************/
static void REDBOX_vlan_memberships_cache_and_oper_warnings_update_all(void)
{
    redbox_port_state_t *port_state;
    mesa_port_no_t      port_no;
    mesa_rc             rc;

    for (port_no = 0; port_no < REDBOX_cap_port_cnt; port_no++) {
        port_state = &REDBOX_port_states[port_no];
        if ((rc = vlan_mgmt_membership_per_port_get(VTSS_ISID_START, port_no, VTSS_APPL_VLAN_USER_ALL, port_state->vlan_memberships)) != VTSS_RC_OK) {
            T_E_PORT(port_no, "vlan_mgmt_membership_per_port_get() failed: %s", error_txt(rc));
            vtss_clear(port_state->vlan_memberships);
        }
    }

    // Update operational warnings.
    REDBOX_oper_warnings_update_all(VTSS_TRACE_LVL_NOISE);
}

#ifdef VTSS_SW_OPTION_MSTP
/******************************************************************************/
// REDBOX_stp_config_change_callback()
/******************************************************************************/
static void REDBOX_stp_config_change_callback(void)
{
    redbox_itr_t redbox_itr;

    T_IG(REDBOX_TRACE_GRP_CALLBACK, "STP configuration has changed");

    REDBOX_LOCK_SCOPE();

    // This function is called whenever any STP configuration has changed, so we
    // need to go through all RedBox instances to see if something has happened.
    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        REDBOX_oper_warnings_stp_update(redbox_itr->second);
    }
}
#endif

/******************************************************************************/
// REDBOX_vlan_custom_etype_change_callback()
/******************************************************************************/
static void REDBOX_vlan_custom_etype_change_callback(mesa_etype_t tpid)
{
    redbox_itr_t   redbox_itr;
    mesa_port_no_t port_no;

    T_IG(REDBOX_TRACE_GRP_CALLBACK, "S-Custom TPID: 0x%04x -> 0x%04x", REDBOX_s_custom_tpid, tpid);

    REDBOX_LOCK_SCOPE();

    if (REDBOX_s_custom_tpid == tpid) {
        return;
    }

    REDBOX_s_custom_tpid = tpid;

    for (port_no = 0; port_no < REDBOX_cap_port_cnt; port_no++) {
        redbox_port_state_t &port_state = REDBOX_port_states[port_no];

        if (port_state.vlan_conf.port_type == VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM) {
            // The base picks up the value of this one on the fly, so no need to
            // invoke base if it changes.
            port_state.tpid = REDBOX_s_custom_tpid;
        }
    }

    // Update operational warnings
    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        REDBOX_oper_warnings_vlan_update(redbox_itr->second);
    }
}

/******************************************************************************/
// REDBOX_vlan_port_conf_change_callback()
/******************************************************************************/
static void REDBOX_vlan_port_conf_change_callback(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *new_vlan_conf)
{
    redbox_itr_t        redbox_itr;
    redbox_port_state_t &port_state = REDBOX_port_states[port_no];

    REDBOX_LOCK_SCOPE();

    T_DG_PORT(REDBOX_TRACE_GRP_CALLBACK, port_no, "vlan_port_conf = %s", *new_vlan_conf);

    // The base picks up the value of port_state.tpid on the fly, so no need to
    // invoke base if it changes.
    switch (new_vlan_conf->port_type) {
    case VTSS_APPL_VLAN_PORT_TYPE_UNAWARE:
    case VTSS_APPL_VLAN_PORT_TYPE_C:
        port_state.tpid = VTSS_APPL_VLAN_C_TAG_ETHERTYPE;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S:
        port_state.tpid = VTSS_APPL_VLAN_S_TAG_ETHERTYPE;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM:
        port_state.tpid = REDBOX_s_custom_tpid;
        break;

    default:
        T_E("Unknown port_type (%d)", new_vlan_conf->port_type);
        // Leave TPID at what it was.
        break;
    }

    // Update
    port_state.vlan_conf = *new_vlan_conf;

    // Update operational warnings
    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            continue;
        }

        if (port_no == redbox_itr->second.interlink_port_no_get()) {
            REDBOX_oper_warnings_vlan_update(redbox_itr->second);
        }
    }
}

/******************************************************************************/
// REDBOX_vlan_membership_change_callback()
/******************************************************************************/
static void REDBOX_vlan_membership_change_callback(void)
{
    redbox_itr_t redbox_itr;

    if (!REDBOX_vlan_membership_updates_started) {
        // Postpone any updates until after configuration has been applied.
        // In this way we avoid calling T_IG() 4K times during boot.
        return;
    }

    T_IG(REDBOX_TRACE_GRP_CALLBACK, "VLAN membership change");

    REDBOX_LOCK_SCOPE();

    // Cache the new vlan_memberships and update operational warnings
    REDBOX_vlan_memberships_cache_and_oper_warnings_update_all();
}

/******************************************************************************/
// REDBOX_port_link_state_change_callback()
/******************************************************************************/
static void REDBOX_port_link_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    redbox_port_state_t &port_state = REDBOX_port_states[port_no];
    redbox_itr_t        redbox_itr;

    REDBOX_LOCK_SCOPE();

    T_DG_PORT(REDBOX_TRACE_GRP_CALLBACK, port_no, "Link state: %d -> %d", port_state.link, status->link);

    port_state.link = status->link;

    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            continue;
        }

        if (redbox_itr->second.using_port_no(port_no)) {
            redbox_base_notification_status_update(redbox_itr->second);
        }
    }
}

/******************************************************************************/
// REDBOX_port_conf_change_callback()
/******************************************************************************/
static void REDBOX_port_conf_change_callback(mesa_port_no_t port_no, const vtss_appl_port_conf_t *conf)
{
    redbox_itr_t redbox_itr;

    REDBOX_LOCK_SCOPE();

    if (REDBOX_port_states[port_no].mtu == conf->max_length) {
        return;
    }

    REDBOX_port_states[port_no].mtu = conf->max_length;

    // Update operational warnings
    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        REDBOX_oper_warnings_mtu_update(redbox_itr->second);
    }
}

/******************************************************************************/
// REDBOX_port_state_init()
/******************************************************************************/
static void REDBOX_port_state_init(void)
{
    vtss_appl_vlan_port_conf_t vlan_port_conf_default;
    vtss_appl_port_conf_t      port_conf;
    vtss_ifindex_t             ifindex;
    mesa_port_no_t             port_no;
    mesa_rc                    rc;

    // Don't take our own mutex during callback registrations (see comment in
    // similar function in cfm.cxx)

    // Subscribe to Custom S-tag TPID changes in the VLAN module
    vlan_s_custom_etype_change_register(VTSS_MODULE_ID_REDBOX, REDBOX_vlan_custom_etype_change_callback);

    // Subscribe to VLAN port configuration changes in the VLAN module
    vlan_port_conf_change_register(VTSS_MODULE_ID_REDBOX, REDBOX_vlan_port_conf_change_callback, TRUE);

    // Subscribe to VLAN membership changes in the VLAN module
    vlan_membership_bulk_change_register(VTSS_MODULE_ID_REDBOX, REDBOX_vlan_membership_change_callback);

    // Subscribe to link changes in the Port module
    if ((rc = port_change_register(VTSS_MODULE_ID_REDBOX, REDBOX_port_link_state_change_callback)) != VTSS_RC_OK) {
        T_E("port_change_register() failed: %s", error_txt(rc));
    }

    // Subscribe to port configuration changes in the Port module
    if ((rc = port_conf_change_register(VTSS_MODULE_ID_REDBOX, REDBOX_port_conf_change_callback)) != VTSS_RC_OK) {
        T_E("port_conf_change_register() failed: %s", error_txt(rc));
    }

    if ((rc = vlan_mgmt_port_conf_default_get(&vlan_port_conf_default)) != VTSS_RC_OK) {
        T_E("vlan_mgmt_port_conf_default_get() failed: %s", error_txt(rc));
        vtss_clear(vlan_port_conf_default);
    }

#ifdef VTSS_SW_OPTION_MSTP
    // Subscribe to MSTP configuration changes
    if (!mstp_register_config_change_cb(REDBOX_stp_config_change_callback)) {
        T_E("mstp_register_config_change_cb() failed");
    }
#endif

    {
        REDBOX_LOCK_SCOPE();

        REDBOX_s_custom_tpid = VTSS_APPL_VLAN_CUSTOM_S_TAG_DEFAULT;

        (void)conf_mgmt_mac_addr_get(REDBOX_chassis_mac.addr, 0);

#ifndef VTSS_SW_OPTION_IP
        {
            // If the IP module is not included, we need to install the
            // management MAC address in the MAC table, or frames to this MAC
            // will be flooded.
            mesa_mac_table_entry_t entry = {};

            entry.vid_mac.vid = VTSS_APPL_VLAN_ID_DEFAULT;
            entry.vid_mac.mac = REDBOX_chassis_mac;
            entry.locked      = true;
            entry.cpu_queue   = PACKET_XTR_QU_MGMT_MAC;
            entry.copy_to_cpu = true;

            T_IG(REDBOX_TRACE_GRP_API, "mesa_mac_table_add(%u:%s)", entry.vid_mac.vid, entry.vid_mac.mac);
            if ((rc = mesa_mac_table_add(NULL, &entry)) != VTSS_RC_OK) {
                T_EG(REDBOX_TRACE_GRP_API, "mesa_mac_table_add(%u:%s) failed: %s", entry.vid_mac.vid, entry.vid_mac.mac, error_txt(rc));
            }
        }
#endif

        for (port_no = 0; port_no < REDBOX_cap_port_cnt; port_no++) {
            redbox_port_state_t &port_state = REDBOX_port_states[port_no];
            vtss_clear(port_state);
            port_state.port_no     = port_no;
            port_state.link        = false;
            port_state.vlan_conf   = vlan_port_conf_default.hybrid;
            port_state.tpid        = VTSS_APPL_VLAN_C_TAG_ETHERTYPE;
            port_state.chassis_mac = &REDBOX_chassis_mac;
            (void)conf_mgmt_mac_addr_get(port_state.redbox_mac.addr, port_no + 1);

            if ((rc = vtss_ifindex_from_port(VTSS_ISID_START, port_no, &ifindex)) != VTSS_RC_OK) {
                T_E_PORT(port_no, "vtss_ifindex_from_port() failed: %s", error_txt(rc));
                port_state.mtu = fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX);
            } else if ((rc = vtss_appl_port_conf_get(ifindex, &port_conf)) != VTSS_RC_OK) {
                T_E_PORT(port_no, "vtss_appl_port_conf_get(%s) failed: %s", ifindex, error_txt(rc));
                port_state.mtu = fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX);
            } else {
                port_state.mtu = port_conf.max_length;
            }
        }
    }
}

/******************************************************************************/
// REDBOX_ptr_check()
/******************************************************************************/
static mesa_rc REDBOX_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_REDBOX_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_inst_check()
// Range [1; REDBOX_cap.inst_cnt_max]
/******************************************************************************/
static mesa_rc REDBOX_inst_check(uint32_t inst)
{
    if (inst < 1 || inst > REDBOX_cap.inst_cnt_max) {
        return VTSS_APPL_REDBOX_RC_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_inst_active()
/******************************************************************************/
static mesa_rc REDBOX_inst_active(redbox_itr_t &redbox_itr)
{
    if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        return VTSS_APPL_REDBOX_RC_INSTANCE_NOT_ACTIVE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_ifindex_to_port()
/******************************************************************************/
static mesa_rc REDBOX_ifindex_to_port(vtss_ifindex_t ifindex, vtss_appl_redbox_port_type_t port_type, mesa_port_no_t &port_no)
{
    vtss_ifindex_elm_t ife;

    // Check that we can decompose the ifindex and that it's a port.
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        port_no = VTSS_PORT_NO_NONE;
        return port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? VTSS_APPL_REDBOX_RC_INVALID_PORT_A_IFINDEX : VTSS_APPL_REDBOX_RC_INVALID_PORT_B_IFINDEX;
    }

    port_no = ife.ordinal;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// REDBOX_conf_update()
/******************************************************************************/
static void REDBOX_conf_update(redbox_itr_t redbox_itr, const vtss_appl_redbox_conf_t &new_conf)
{
    redbox_state_t                &redbox_state  = redbox_itr->second;
    vtss_appl_redbox_conf_t       old_conf       = redbox_state.conf;
    vtss_appl_redbox_oper_state_t old_oper_state = redbox_state.status.oper_state;
    mesa_port_no_t                port_no;
    vtss_appl_redbox_port_type_t  port_type;
    vtss_ifindex_t                ifindex;
    bool                          deactivate_first;

    redbox_state.conf = new_conf;
    redbox_state.inst = redbox_itr->first;

    if (!REDBOX_started) {
        // Defer all redbox_base updates until after all configuration has been
        // applied from startup-config after boot.
        return;
    }

    // Assume no warnings.
    redbox_state.status.oper_warnings = VTSS_APPL_REDBOX_OPER_WARNING_NONE;

    // This RedBox instance administratively enabled?
    if (redbox_state.conf.admin_active) {
        redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_ACTIVE;

        // Update our state with correct port states
        for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_B; port_type++) {
            ifindex = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? redbox_state.conf.port_a : redbox_state.conf.port_b;

            if (ifindex == VTSS_IFINDEX_REDBOX_NEIGHBOR) {
                // We don't have a port state if using the internal connection.
                redbox_state.port_states[port_type] = nullptr;
            } else {
                // The configuration's ifindex has already been checked by
                // vtss_appl_redbox_conf_set(), so no need to check return value.
                (void)REDBOX_ifindex_to_port(ifindex, port_type, port_no);
                redbox_state.port_states[port_type] = &REDBOX_port_states[port_no];
            }
        }
    } else {
        redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_ADMIN_DISABLED;
    }

    // Time to update base - if needed.
    // Check if the operational state has changed.
    if (redbox_state.status.oper_state != old_oper_state) {
        // Operational state has changed.
        if (redbox_state.status.oper_state == VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            // Inactive-to-active
            if (redbox_base_activate(redbox_state) != VTSS_RC_OK) {
                redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR;
            }
        } else if (old_oper_state == VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            // Active-to-inactive
            (void)redbox_base_deactivate(redbox_state);
        }
    } else if (redbox_state.status.oper_state == VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        // Active-to_active.

        // To minimize the effect on the current RedBox operation when the user
        // changes a parameter, we need to judge whether we can avoid disabling
        // and re-enabling the instance.

        // We divide the configuration parameters into three categories:
        // 1) Parameters that require us to deactivate the RedBox instance first
        // 2) Parameters that require dedicated call(s) into base
        // 3) Parameters that DO NOT require a dedicated call into base, because
        //    it picks up the new value on the fly.

        // Here's a table
        // +---------------------------------------------+
        // | Parameter                        | Category |
        // |----------------------------------|----------|
        // | mode                             | 1        |
        // | port_a, port_b                   | 1        |
        // | hsr_mode                         | 1        |
        // | duplicate_discard                | 2        |
        // | net_id                           | 2        |
        // | lan_id                           | 2        |
        // | nt_age_time_secs                 | 2        |
        // | pnt_age_time_secs                | 2        |
        // | duplicate_discard_age_time_msecs | 2        |
        // | sv_vlan, sv_pcp                  | 3        |
        // | sv_dmac_lsb                      | 2 + 3    |
        // | sv_frame_interval_secs           | 2        |
        // | sv_xlat_prp_to_hsr               | 2        |
        // | sv_xlat_hsr_to_prp               | 2        |
        // +---------------------------------------------+

        // Category 1 checks.
        deactivate_first = false;

        // Notice that we rely on parameters not used in a particular mode to be
        // reset to their defaults, so that they don't give rise to changes.

        if (old_conf.mode     != redbox_state.conf.mode     ||
            old_conf.port_a   != redbox_state.conf.port_a   ||
            old_conf.port_b   != redbox_state.conf.port_b   ||
            old_conf.hsr_mode != redbox_state.conf.hsr_mode) {
            deactivate_first = true;
        }

        if (deactivate_first) {
            // The configuration has changed. First deactivate the old, then
            // activate the new.
            redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_ADMIN_DISABLED;
            (void)redbox_base_deactivate(redbox_state);
            redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_ACTIVE;
            if (redbox_base_activate(redbox_state) != VTSS_RC_OK) {
                redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR;
            }
        } else {
            // Category 2 checks
            if (old_conf.sv_frame_interval_secs != redbox_state.conf.sv_frame_interval_secs) {
                redbox_base_sv_frame_interval_changed(redbox_state);
            }

            if (old_conf.nt_age_time_secs != redbox_state.conf.nt_age_time_secs) {
                redbox_base_nt_age_time_changed(redbox_state);
            }

            if (old_conf.sv_xlat_prp_to_hsr != redbox_state.conf.sv_xlat_prp_to_hsr) {
                redbox_base_xlat_prp_to_hsr_changed(redbox_state);
            }

            if (old_conf.duplicate_discard                != redbox_state.conf.duplicate_discard                ||
                old_conf.net_id                           != redbox_state.conf.net_id                           ||
                old_conf.lan_id                           != redbox_state.conf.lan_id                           ||
                old_conf.pnt_age_time_secs                != redbox_state.conf.pnt_age_time_secs                ||
                old_conf.duplicate_discard_age_time_msecs != redbox_state.conf.duplicate_discard_age_time_msecs ||
                old_conf.sv_dmac_lsb                      != redbox_state.conf.sv_dmac_lsb                      ||
                old_conf.sv_xlat_hsr_to_prp               != redbox_state.conf.sv_xlat_hsr_to_prp               ||
                old_conf.sv_xlat_prp_to_hsr               != redbox_state.conf.sv_xlat_prp_to_hsr) {
                redbox_base_api_conf_changed(redbox_state);
            }
        }
    }

    // A change of the RB configuration may affect both this RB's and other RBs'
    // operational warnings. Update all.
    REDBOX_oper_warnings_update_all();
}

/******************************************************************************/
// REDBOX_port_check()
/******************************************************************************/
static mesa_rc REDBOX_port_check(uint32_t inst, vtss_ifindex_t ifindex, vtss_appl_redbox_port_type_t port_type)
{
    mesa_port_no_t port_no;

    // ifindex may either represent a port or be VTSS_IFINDEX_REDBOX_NEIGHBOR.
    if (vtss_ifindex_is_redbox_neighbor(ifindex)) {
        if (inst == 1 && port_type == VTSS_APPL_REDBOX_PORT_TYPE_A) {
            // Port A on RedBox instance 1 cannot have a left neighbor.
            return VTSS_APPL_REDBOX_RC_REDBOX_1_CANNOT_HAVE_A_LEFT_NEIGHBOR;
        } else if (inst == REDBOX_cap.inst_cnt_max && port_type == VTSS_APPL_REDBOX_PORT_TYPE_B) {
            // Port B on last RedBox instance cannot have a right neighbor.
            return VTSS_APPL_REDBOX_RC_REDBOX_N_CANNOT_HAVE_A_RIGHT_NEIGHBOR;
        }

        // The remaining of this function is when referring to a specific port.
        return VTSS_RC_OK;
    }

    // First check that the ifindex actually represents a port
    VTSS_RC(REDBOX_ifindex_to_port(ifindex, port_type, port_no));

    // Then check that this port is valid for this redbox instance.
    if (!REDBOX_ports[inst - 1].port_list[port_no]) {
        return port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? VTSS_APPL_REDBOX_RC_PORT_A_NOT_VALID_FOR_REDBOX_INSTANCE : VTSS_APPL_REDBOX_RC_PORT_B_NOT_VALID_FOR_REDBOX_INSTANCE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_conf_update_all()
/******************************************************************************/
static void REDBOX_conf_update_all(void)
{
    redbox_itr_t redbox_itr;

    // This function is invoked the very first time we are allowed to update
    // base, so REDBOX_conf_update() will see that old oper_state is inactive,
    // which in turn will cause the configuration to be applied for now active
    // instances.
    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        REDBOX_conf_update(redbox_itr, redbox_itr->second.conf);
    }
}

//*****************************************************************************/
// REDBOX_default()
/******************************************************************************/
static void REDBOX_default(void)
{
    vtss_appl_redbox_conf_t new_conf;
    redbox_itr_t            redbox_itr;

    REDBOX_LOCK_SCOPE();

    // Start by setting all redbox instances inactive and call to update the
    // configuration. This will release the redbox resources in MESA so that we
    // can erase all of it in one go afterwards.
    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        if (redbox_itr->second.conf.admin_active) {
            new_conf = redbox_itr->second.conf;
            new_conf.admin_active = false;
            REDBOX_conf_update(redbox_itr, new_conf);
        }
    }

    // Then erase all elements from the map.
    REDBOX_map.clear();
}

/******************************************************************************/
// vtss_appl_redbox_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_capabilities_get(vtss_appl_redbox_capabilities_t *cap)
{
    VTSS_RC(REDBOX_ptr_check(cap));
    *cap = REDBOX_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_supported()
/******************************************************************************/
mesa_bool_t vtss_appl_redbox_supported(void)
{
    return REDBOX_supported() == VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_capabilities_port_list_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_capabilities_port_list_get(uint32_t inst, mesa_port_list_t *port_list)
{
    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(port_list));

    *port_list = REDBOX_ports[inst - 1].port_list;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_conf_default_get(vtss_appl_redbox_conf_t *conf)
{
    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_ptr_check(conf));

    *conf = REDBOX_default_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_conf_get(uint32_t inst, vtss_appl_redbox_conf_t *conf)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(conf));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    *conf = redbox_itr->second.conf;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_redbox_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_redbox_conf_set(uint32_t inst, const vtss_appl_redbox_conf_t *conf)
{
    vtss_appl_redbox_port_type_t port_type;
    vtss_appl_redbox_conf_t      local_conf;
    redbox_itr_t                 redbox_itr;
    bool                         new_entry;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(conf));

    T_D("Enter. inst = %u, conf = %s", inst, *conf);

    if (conf->mode != VTSS_APPL_REDBOX_MODE_PRP_SAN && conf->mode != VTSS_APPL_REDBOX_MODE_HSR_SAN && conf->mode != VTSS_APPL_REDBOX_MODE_HSR_PRP && conf->mode != VTSS_APPL_REDBOX_MODE_HSR_HSR) {
        return VTSS_APPL_REDBOX_RC_INVALID_MODE;
    }

    if (conf->mode != VTSS_APPL_REDBOX_MODE_PRP_SAN) {
        if (conf->hsr_mode != VTSS_APPL_REDBOX_HSR_MODE_H && conf->hsr_mode != VTSS_APPL_REDBOX_HSR_MODE_N &&
            conf->hsr_mode != VTSS_APPL_REDBOX_HSR_MODE_T && conf->hsr_mode != VTSS_APPL_REDBOX_HSR_MODE_U &&
            conf->hsr_mode != VTSS_APPL_REDBOX_HSR_MODE_M && conf->hsr_mode != VTSS_APPL_REDBOX_HSR_MODE_R) {
            return VTSS_APPL_REDBOX_RC_INVALID_HSR_MODE;
        }
    }

    if (conf->mode == VTSS_APPL_REDBOX_MODE_HSR_PRP || conf->mode == VTSS_APPL_REDBOX_MODE_HSR_HSR) {
        if (conf->net_id < 1 || conf->net_id > 7) {
            // Valid range is [1; 7]
            return VTSS_APPL_REDBOX_RC_INVALID_NET_ID;
        }
    }

    if (conf->mode == VTSS_APPL_REDBOX_MODE_HSR_PRP) {
        if (conf->lan_id != VTSS_APPL_REDBOX_LAN_ID_A && conf->lan_id != VTSS_APPL_REDBOX_LAN_ID_B) {
            return VTSS_APPL_REDBOX_RC_INVALID_LAN_ID;
        }
    }

    if (conf->nt_age_time_secs < REDBOX_cap.nt_age_time_secs_min || conf->nt_age_time_secs > REDBOX_cap.nt_age_time_secs_max) {
        return VTSS_APPL_REDBOX_RC_INVALID_NT_AGE_TIME;
    }

    if (conf->mode != VTSS_APPL_REDBOX_MODE_HSR_HSR) {
        if (conf->pnt_age_time_secs < REDBOX_cap.pnt_age_time_secs_min || conf->pnt_age_time_secs > REDBOX_cap.pnt_age_time_secs_max) {
            return VTSS_APPL_REDBOX_RC_INVALID_PNT_AGE_TIME;
        }
    }

    if (conf->duplicate_discard_age_time_msecs < REDBOX_cap.duplicate_discard_age_time_msecs_min || conf->duplicate_discard_age_time_msecs > REDBOX_cap.duplicate_discard_age_time_msecs_max) {
        return VTSS_APPL_REDBOX_RC_INVALID_DUPLICATE_DISCARD_AGE_TIME;
    }

    if (conf->sv_vlan > VTSS_APPL_VLAN_ID_MAX) {
        return VTSS_APPL_REDBOX_RC_INVALID_SV_VLAN;
    }

    if (conf->sv_pcp > 7) {
        return VTSS_APPL_REDBOX_RC_INVALID_SV_PCP;
    }

    if (conf->sv_frame_interval_secs < REDBOX_cap.sv_frame_interval_secs_min || conf->sv_frame_interval_secs > REDBOX_cap.sv_frame_interval_secs_max) {
        return VTSS_APPL_REDBOX_RC_INVALID_SV_FRAME_INTERVAL;
    }

    // Copy the configuration to a locally defined variable, so that we can
    // change it.
    local_conf = *conf;

    // Checks that we need to defer until the user attempts to enable instance
    if (local_conf.admin_active) {
        for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_B; port_type++) {
            VTSS_RC(REDBOX_port_check(inst, port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? local_conf.port_a : local_conf.port_b, port_type));
        }

        // Both port_a and port_b cannot both be VTSS_IFINDEX_REDBOX_NEIGHBOR
        if (local_conf.port_a == VTSS_IFINDEX_REDBOX_NEIGHBOR && local_conf.port_b == VTSS_IFINDEX_REDBOX_NEIGHBOR) {
            // Can only reach this if inst > 1 && inst < REDBOX_cap.inst_cnt_max
            return VTSS_APPL_REDBOX_RC_PORT_A_OR_PORT_B_OR_BOTH_MUST_REFER_TO_A_PORT;
        }

        if (local_conf.port_a == local_conf.port_b) {
            return VTSS_APPL_REDBOX_RC_PORT_A_B_IDENTICAL;
        }

        // If using a neighbor RedBox, the only supported modes are HSR.
        if (local_conf.port_a == VTSS_IFINDEX_REDBOX_NEIGHBOR || local_conf.port_b == VTSS_IFINDEX_REDBOX_NEIGHBOR) {
            if (local_conf.mode == VTSS_APPL_REDBOX_MODE_PRP_SAN) {
                return VTSS_APPL_REDBOX_RC_MODE_MUST_BE_HSR_IF_USING_REDBOX_NEIGHBOR;
            }
        }

        // Default parameters that are not used in this mode
        if (local_conf.mode == VTSS_APPL_REDBOX_MODE_PRP_SAN) {
            local_conf.hsr_mode = REDBOX_default_conf.hsr_mode;
        }

        if (local_conf.mode != VTSS_APPL_REDBOX_MODE_HSR_PRP) {
            if (local_conf.mode != VTSS_APPL_REDBOX_MODE_HSR_HSR) {
                // This is valid in both HSR-PRP and HSR-HSR mode
                local_conf.net_id = REDBOX_default_conf.net_id;
            }

            local_conf.lan_id             = REDBOX_default_conf.lan_id;
            local_conf.sv_xlat_prp_to_hsr = REDBOX_default_conf.sv_xlat_prp_to_hsr;
            local_conf.sv_xlat_hsr_to_prp = REDBOX_default_conf.sv_xlat_hsr_to_prp;
        }

        if (local_conf.mode == VTSS_APPL_REDBOX_MODE_HSR_HSR) {
            local_conf.pnt_age_time_secs = REDBOX_default_conf.pnt_age_time_secs;
        }
    }

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) != REDBOX_map.end()) {
        if (memcmp(&local_conf, &redbox_itr->second, sizeof(local_conf)) == 0) {
            // No changes.
            T_D("%u: No changes", inst);
            return VTSS_RC_OK;
        }
    }

    // Check that we haven't created more instances than we can allow
    if (redbox_itr == REDBOX_map.end()) {
        if (REDBOX_map.size() >= REDBOX_cap.inst_cnt_max) {
            return VTSS_APPL_REDBOX_RC_LIMIT_REACHED;
        }

        new_entry = true;
    } else {
        new_entry = false;
    }

    // Create a new or update an existing entry
    if ((redbox_itr = REDBOX_map.get(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_OUT_OF_MEMORY;
    }

    if (new_entry) {
        vtss_clear(redbox_itr->second);
    }

    REDBOX_conf_update(redbox_itr, local_conf);
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_redbox_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_redbox_conf_del(uint32_t inst)
{
    vtss_appl_redbox_conf_t new_conf;
    redbox_itr_t            redbox_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    if (redbox_itr->second.conf.admin_active) {
        new_conf = redbox_itr->second.conf;
        new_conf.admin_active = false;

        // Back out of everything
        REDBOX_conf_update(redbox_itr, new_conf);
    }

    // Delete redbox instance from our map
    REDBOX_map.erase(inst);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_redbox_itr()
/******************************************************************************/
mesa_rc vtss_appl_redbox_itr(const uint32_t *prev_inst, uint32_t *next_inst)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_ptr_check(next_inst));

    REDBOX_LOCK_SCOPE();

    if (prev_inst) {
        // Here we have a valid prev_inst. Find the next from that one.
        redbox_itr = REDBOX_map.greater_than(*prev_inst);
    } else {
        // We don't have a valid prev_inst. Get the first.
        redbox_itr = REDBOX_map.begin();
    }

    if (redbox_itr != REDBOX_map.end()) {
        *next_inst = redbox_itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_redbox_notification_status_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_notification_status_get(uint32_t inst, vtss_appl_redbox_notification_status_t *const notif_status)
{
    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(notif_status));

    T_D("%u: Enter", inst);

    // No need to lock scope, because the .get() function is guaranteed to be
    // atomic.
    return redbox_notification_status.get(inst, notif_status);
}

/******************************************************************************/
// vtss_appl_redbox_status_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_status_get(uint32_t inst, vtss_appl_redbox_status_t *status)
{
    redbox_itr_t redbox_itr;
    mesa_rc      rc;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(status));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    *status = redbox_itr->second.status;

    if (status->oper_state == VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        // The base does NOT update redbox_itr->second.status.notif_status. It
        // only updates the global redbox_notification_status object.
        if ((rc = redbox_notification_status.get(inst, &status->notif_status)) != VTSS_RC_OK) {
            T_E("redbox_notification_status.get(%u) failed: %s", inst, error_txt(rc));
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_nt_status_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_nt_status_get(uint32_t inst, vtss_appl_redbox_nt_status_t *status)
{
    redbox_itr_t     redbox_itr;
    redbox_mac_itr_t mac_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(status));

    vtss_clear(*status);

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        // Also return OK if it's not active. The caller must ensure that it's
        // active before utilizing the result.
        return VTSS_RC_OK;
    }

    // If the NT entries in the MAC map are too old, we need to update it.
    redbox_base_nt_poll(redbox_itr->second);

    for (mac_itr = redbox_itr->second.mac_map.begin(); mac_itr != redbox_itr->second.mac_map.end(); ++mac_itr) {
        if (mac_itr->second.is_pnt_entry) {
            continue;
        }

        status->mac_cnt++;

        if (mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_A].rx_wrong_lan_cnt || mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_B].rx_wrong_lan_cnt) {
            status->wrong_lan = true;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_nt_mac_status_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_nt_mac_status_get(uint32_t inst, const mesa_mac_t *mac, vtss_appl_redbox_nt_mac_status_t *status)
{
    vtss_appl_redbox_port_type_t port_type;
    redbox_itr_t                 redbox_itr;
    redbox_mac_itr_t             mac_itr;
    uint64_t                     now_secs = vtss::uptime_seconds();

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(mac));
    VTSS_RC(REDBOX_ptr_check(status));

    T_I("%u: mac = %s", inst, *mac);
    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    VTSS_RC(REDBOX_inst_active(redbox_itr));

    // If the NT entries in the MAC map are too old, we need to update it.
    redbox_base_nt_poll(redbox_itr->second);

    if ((mac_itr = redbox_itr->second.mac_map.find(*mac)) == redbox_itr->second.mac_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_NT_INSTANCE;
    }

    if (mac_itr->second.is_pnt_entry) {
        // Belongs to the PNT table and not the NT table.
        return VTSS_APPL_REDBOX_RC_NO_SUCH_NT_INSTANCE;
    }

    *status = mac_itr->second.nt.status;

    // Adjust the "last seen" for data and SV.
    // These are only valid if the corresponding rx_cnt is non-zero.
    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_B; port_type++) {
        vtss_appl_redbox_mac_port_status_t &port = status->port[port_type];
        port.last_seen_secs    = port.rx_cnt    ? now_secs - port.last_seen_secs    : 0;
        port.sv_last_seen_secs = port.sv_rx_cnt ? now_secs - port.sv_last_seen_secs : 0;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_nt_clear()
/******************************************************************************/
mesa_rc vtss_appl_redbox_nt_clear(uint32_t inst)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));

    T_I("inst = %u", inst);

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        // Pretend everything went fine when the instance is not active
        return VTSS_RC_OK;
    }

    return redbox_base_nt_clear(redbox_itr->second);
}

/******************************************************************************/
// vtss_appl_redbox_nt_itr()
/******************************************************************************/
mesa_rc vtss_appl_redbox_nt_itr(const uint32_t *prev_inst, uint32_t *next_inst, const mesa_mac_t *prev_mac, mesa_mac_t *next_mac)
{
    redbox_itr_t     redbox_itr;
    redbox_mac_itr_t mac_itr;
    mesa_mac_t       previous_mac;
    char             inst_str[20], mac_str[20];

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_ptr_check(next_inst));
    VTSS_RC(REDBOX_ptr_check(next_mac));

    // Cannot do this directly in the T_I() below
    if (prev_inst) {
        sprintf(inst_str, "%u", *prev_inst);
    } else {
        strcpy(inst_str, "Not set");
    }

    if (prev_mac) {
        (void)misc_mac_txt(prev_mac->addr, mac_str);
    } else {
        strcpy(mac_str, "Not set");
    }

    T_I("Enter: prev_inst = %s, prev_mac = %s", inst_str, mac_str);

    REDBOX_LOCK_SCOPE();

    if (prev_inst) {
        // We have a valid previous. Get the same or the next.
        if ((redbox_itr = REDBOX_map.greater_than_or_equal(*prev_inst)) == REDBOX_map.end()) {
            T_I("No next");
            return VTSS_RC_ERROR;
        }

        if (redbox_itr->first != *prev_inst || !prev_mac) {
            vtss_clear(previous_mac);
        } else {
            previous_mac = *prev_mac;
        }
    } else {
        redbox_itr = REDBOX_map.begin();
        vtss_clear(previous_mac);
    }

    while (redbox_itr != REDBOX_map.end()) {
        if (redbox_itr->second.status.oper_state == VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            // If the NT entries in the mac_map are too old, we need to update it.
            redbox_base_nt_poll(redbox_itr->second);

            while ((mac_itr = redbox_itr->second.mac_map.greater_than(previous_mac)) != redbox_itr->second.mac_map.end()) {
                if (mac_itr->second.is_pnt_entry) {
                    // This is a PNT entry. Get the next
                    previous_mac = mac_itr->first;
                } else {
                    // The entry is indeed an NT entry. Use that.
                    *next_inst = redbox_itr->first;
                    *next_mac  = mac_itr->first;
                    T_I("Exit: next_inst = %u, next_mac = %s", *next_inst, *next_mac);
                    return VTSS_RC_OK;
                }
            }
        }

        // No more MAC addresses for this instance.
        vtss_clear(previous_mac);
        ++redbox_itr;
    }

    T_I("No next");
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_redbox_pnt_status_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_pnt_status_get(uint32_t inst, vtss_appl_redbox_pnt_status_t *status)
{
    redbox_itr_t     redbox_itr;
    redbox_mac_itr_t mac_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(status));

    vtss_clear(*status);

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        // Also return OK if it's not active. The caller must ensure that it's
        // active before utilizing the result.
        return VTSS_RC_OK;
    }

    // If the PNT entries in the MAC map are too old, we need to update it.
    redbox_base_pnt_poll(redbox_itr->second);

    for (mac_itr = redbox_itr->second.mac_map.begin(); mac_itr != redbox_itr->second.mac_map.end(); ++mac_itr) {
        if (!mac_itr->second.is_pnt_entry) {
            continue;
        }

        status->mac_cnt++;

        if (mac_itr->second.pnt.status.port.rx_wrong_lan_cnt) {
            status->wrong_lan = true;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_pnt_mac_status_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_pnt_mac_status_get(uint32_t inst, const mesa_mac_t *mac, vtss_appl_redbox_pnt_mac_status_t *status)
{
    redbox_itr_t     redbox_itr;
    redbox_mac_itr_t mac_itr;
    uint64_t         now_secs = vtss::uptime_seconds();

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(mac));
    VTSS_RC(REDBOX_ptr_check(status));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    VTSS_RC(REDBOX_inst_active(redbox_itr));

    if ((mac_itr = redbox_itr->second.mac_map.find(*mac)) == redbox_itr->second.mac_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_PNT_INSTANCE;
    }

    if (!mac_itr->second.is_pnt_entry) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_PNT_INSTANCE;
    }

    *status = mac_itr->second.pnt.status;

    // Adjust the "last seen" for data and SV.
    // These are only valid if the corresponding rx_cnt is non-zero.
    vtss_appl_redbox_mac_port_status_t &port = status->port;
    port.last_seen_secs    = port.rx_cnt    ? now_secs - port.last_seen_secs    : 0;
    port.sv_last_seen_secs = port.sv_rx_cnt ? now_secs - port.sv_last_seen_secs : 0;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_redbox_pnt_clear()
/******************************************************************************/
mesa_rc vtss_appl_redbox_pnt_clear(uint32_t inst)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));

    T_I("inst = %u", inst);

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        // Pretend everything went fine when the instance is not active
        return VTSS_RC_OK;
    }

    return redbox_base_pnt_clear(redbox_itr->second);
}

/******************************************************************************/
// vtss_appl_redbox_pnt_itr()
/******************************************************************************/
mesa_rc vtss_appl_redbox_pnt_itr(const uint32_t *prev_inst, uint32_t *next_inst, const mesa_mac_t *prev_mac, mesa_mac_t *next_mac)
{
    redbox_itr_t     redbox_itr;
    redbox_mac_itr_t mac_itr;
    mesa_mac_t       previous_mac;
    char             inst_str[20], mac_str[20];

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_ptr_check(next_inst));
    VTSS_RC(REDBOX_ptr_check(next_mac));

    // Cannot do this directly in the T_I() below
    if (prev_inst) {
        sprintf(inst_str, "%u", *prev_inst);
    } else {
        strcpy(inst_str, "Not set");
    }

    if (prev_mac) {
        (void)misc_mac_txt(prev_mac->addr, mac_str);
    } else {
        strcpy(mac_str, "Not set");
    }

    T_I("Enter: prev_inst = %s, prev_mac = %s", inst_str, mac_str);

    REDBOX_LOCK_SCOPE();

    if (prev_inst) {
        // We have a valid previous. Get the same or the next.
        if ((redbox_itr = REDBOX_map.greater_than_or_equal(*prev_inst)) == REDBOX_map.end()) {
            T_I("No next");
            return VTSS_RC_ERROR;
        }

        if (redbox_itr->first != *prev_inst || !prev_mac) {
            vtss_clear(previous_mac);
        } else {
            previous_mac = *prev_mac;
        }
    } else {
        redbox_itr = REDBOX_map.begin();
        vtss_clear(previous_mac);
    }

    while (redbox_itr != REDBOX_map.end()) {
        if (redbox_itr->second.status.oper_state == VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            // If the PNT entries in the mac_map are too old, we need to update it.
            redbox_base_pnt_poll(redbox_itr->second);

            while ((mac_itr = redbox_itr->second.mac_map.greater_than(previous_mac)) != redbox_itr->second.mac_map.end()) {
                if (!mac_itr->second.is_pnt_entry) {
                    // This is an NT entry. Get the next
                    previous_mac = mac_itr->first;
                } else {
                    // The entry is indeed a PNT entry. Use that.
                    *next_inst = redbox_itr->first;
                    *next_mac  = mac_itr->first;
                    T_I("Exit: next_inst = %u, next_mac = %s", *next_inst, *next_mac);
                    return VTSS_RC_OK;
                }
            }
        }

        // No more MAC addresses for this instance.
        vtss_clear(previous_mac);
        ++redbox_itr;
    }

    T_I("No next");
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_redbox_statistics_get()
/******************************************************************************/
mesa_rc vtss_appl_redbox_statistics_get(uint32_t inst, vtss_appl_redbox_statistics_t *statistics)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));
    VTSS_RC(REDBOX_ptr_check(statistics));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        vtss_clear(*statistics);
        return VTSS_RC_OK;
    }

    return redbox_base_statistics_get(redbox_itr->second, *statistics);
}

//*****************************************************************************/
// vtss_appl_redbox_statistics_clear()
/******************************************************************************/
mesa_rc vtss_appl_redbox_statistics_clear(uint32_t inst)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_supported());
    VTSS_RC(REDBOX_inst_check(inst));

    T_I("inst = %u", inst);

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
        // Pretend everything went fine when the instance is not active
        return VTSS_RC_OK;
    }

    return redbox_base_statistics_clear(redbox_itr->second);
}

/******************************************************************************/
// redbox_util_mode_to_str()
/******************************************************************************/
const char *redbox_util_mode_to_str(vtss_appl_redbox_mode_t mode, bool capital)
{
    switch (mode) {
    case VTSS_APPL_REDBOX_MODE_PRP_SAN:
        return capital ? "PRP-SAN" : "prp-san";

    case VTSS_APPL_REDBOX_MODE_HSR_SAN:
        return capital ? "HSR-SAN" : "hsr-san";

    case VTSS_APPL_REDBOX_MODE_HSR_PRP:
        return capital ? "HSR-PRP" : "hsr-prp";

    case VTSS_APPL_REDBOX_MODE_HSR_HSR:
        return capital ? "HSR-HSR" : "hsr-hsr";

    default:
        T_E("Invalid mode (%d)", mode);
        return capital ? "UNK" : "unk";
    }
}

/******************************************************************************/
// redbox_util_hsr_mode_to_str()
/******************************************************************************/
const char *redbox_util_hsr_mode_to_str(vtss_appl_redbox_hsr_mode_t hsr_mode, bool capital)
{
    switch (hsr_mode) {
    case VTSS_APPL_REDBOX_HSR_MODE_H:
        return capital ? "H" : "h";

    case VTSS_APPL_REDBOX_HSR_MODE_N:
        return capital ? "N" : "n";

    case VTSS_APPL_REDBOX_HSR_MODE_T:
        return capital ? "T" : "t";

    case VTSS_APPL_REDBOX_HSR_MODE_U:
        return capital ? "U" : "u";

    case VTSS_APPL_REDBOX_HSR_MODE_M:
        return capital ? "M" : "m";

    case VTSS_APPL_REDBOX_HSR_MODE_R:
        return capital ? "R" : "r";

    default:
        T_E("Invalid hsr_mode (%d)", hsr_mode);
        return capital ? "UNK" : "unk";
    }
}

/******************************************************************************/
// redbox_util_port_type_to_str()
/******************************************************************************/
const char *redbox_util_port_type_to_str(vtss_appl_redbox_port_type_t port_type, bool capital, bool contract)
{
    switch (port_type) {
    case VTSS_APPL_REDBOX_PORT_TYPE_A:
        return contract ? (capital ? "PortA" : "portA") : (capital ? "Port A" : "port-a");

    case VTSS_APPL_REDBOX_PORT_TYPE_B:
        return contract ? (capital ? "PortB" : "portB") : (capital ? "Port B" : "port-b");

    case VTSS_APPL_REDBOX_PORT_TYPE_C:
        return contract ? (capital ? "PortC" : "portC") : (capital ? "Port C" : "port-c");

    default:
        T_E("Invalid port_type (%d)", port_type);
        return capital ? "UNK" : "unk";
    }
}

/******************************************************************************/
// redbox_util_lan_id_to_str()
/******************************************************************************/
const char *redbox_util_lan_id_to_str(vtss_appl_redbox_lan_id_t lan_id, bool capital)
{
    switch (lan_id) {
    case VTSS_APPL_REDBOX_LAN_ID_A:
        return capital ? "A" : "a";

    case VTSS_APPL_REDBOX_LAN_ID_B:
        return capital ? "B" : "b";

    default:
        T_E("Invalid lan_id (%d)", lan_id);
        return capital ? "UNK" : "unk";
    }
}

/******************************************************************************/
// redbox_util_node_type_to_str()
/******************************************************************************/
const char *redbox_util_node_type_to_str(vtss_appl_redbox_node_type_t node_type)
{
    switch (node_type) {
    case VTSS_APPL_REDBOX_NODE_TYPE_DANP:
        return "DANP";

    case VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB:
        return "DANP-RedBox";

    case VTSS_APPL_REDBOX_NODE_TYPE_VDANP:
        return "VDANP";

    case VTSS_APPL_REDBOX_NODE_TYPE_DANH:
        return "DANH";

    case VTSS_APPL_REDBOX_NODE_TYPE_DANH_RB:
        return "DANH-RedBox";

    case VTSS_APPL_REDBOX_NODE_TYPE_VDANH:
        return "VDANH";

    case VTSS_APPL_REDBOX_NODE_TYPE_SAN:
        return "SAN";

    default:
        T_E("Invalid node_type (%d)", node_type);
        return "Unk";
    }
}

/******************************************************************************/
// redbox_util_sv_type_to_str()
/******************************************************************************/
const char *redbox_util_sv_type_to_str(vtss_appl_redbox_sv_type_t sv_type, bool contract)
{
    switch (sv_type) {
    case VTSS_APPL_REDBOX_SV_TYPE_PRP_DD:
        return contract ? "PrpDd" : "PRP-DD";

    case VTSS_APPL_REDBOX_SV_TYPE_PRP_DA:
        return contract ? "PrpDa" : "PRP-DA";

    case VTSS_APPL_REDBOX_SV_TYPE_HSR:
        return contract ? "Hsr" : "HSR";

    default:
        T_E("Invalid sv_type (%d)", sv_type);
        return "Unk";
    }
}

/******************************************************************************/
// redbox_util_oper_state_to_str()
/******************************************************************************/
const char *redbox_util_oper_state_to_str(vtss_appl_redbox_oper_state_t oper_state)
{
    switch (oper_state) {
    case VTSS_APPL_REDBOX_OPER_STATE_ADMIN_DISABLED:
        return "Inactive";

    case VTSS_APPL_REDBOX_OPER_STATE_ACTIVE:
        return "Active";

    case VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR:
        return "Internal error has occurred. See console or crashlog for details";

    default:
        T_E("Invalid oper_state (%d)", oper_state);
        return "Unknown";
    }
}

/******************************************************************************/
// redbox_util_oper_warnings_to_str()
// Buf must be ~400 bytes long if all bits are set.
/******************************************************************************/
char *redbox_util_oper_warnings_to_str(char *buf, size_t size, vtss_appl_redbox_oper_warnings_t oper_warnings)
{
    int  s, res;
    bool first;

    if (!buf) {
        return buf;
    }

#define P(_str_)                                        \
    if (size - s > 0) {                                 \
        res = snprintf(buf + s, size - s, "%s", _str_); \
        if (res > 0) {                                  \
            s += res;                                   \
        }                                               \
    }

#define F(X, _name_)                                         \
    if (oper_warnings & VTSS_APPL_REDBOX_OPER_WARNING_##X) { \
        oper_warnings &= ~VTSS_APPL_REDBOX_OPER_WARNING_##X; \
        if (first) {                                         \
            first = false;                                   \
            P(_name_);                                       \
        } else {                                             \
            P(", " _name_);                                  \
        }                                                    \
    }

    buf[0] = 0;
    s      = 0;
    first  = true;

    // Example of a field name (just so that we can search for this function):
    // VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_C_TAGGED

    F(MTU_TOO_HIGH_LRE_PORTS,                     "The MTU is too high on at least one of the LRE ports (max is " vtss_xstr(REDBOX_MTU_MAX_LRE_PORTS) ")");
    F(MTU_TOO_HIGH_NON_LRE_PORTS,                 "The MTU is too high on at least one non-LRE port. Frames larger than " vtss_xstr(REDBOX_MTU_MAX_NON_LRE_PORTS) " cannot traverse the HSR/PRP network");
    F(INTERLINK_NOT_C_TAGGED,                     "Interlink port must use C-tags");
    F(INTERLINK_NOT_MEMBER_OF_VLAN,               "Interlink port is not member of the supervision frame VLAN ID");
    F(NEIGHBOR_REDBOX_NOT_CONFIGURED,             "The neighbor RedBox is not configured");
    F(NEIGHBOR_REDBOX_NOT_ACTIVE,                 "The neighbor RedBox is not active");
    F(NEIGHBOR_REDBOX_PORT_A_NOT_SET_TO_NEIGHBOR, "The neighbor's port A is not configured as a RedBox neighbor");
    F(NEIGHBOR_REDBOX_PORT_B_NOT_SET_TO_NEIGHBOR, "The neighbor's port B is not configured as a RedBox neighbor");
    F(NEIGHBOR_REDBOX_OVERLAPPING_VLANS,          "The neighbor's interlink port has coinciding VLAN memberships with this RedBox's interlink port"); // This may cause loops
    F(STP_ENABLED_INTERLINK,                      "Interlink port has spanning tree enabled");

    buf[MIN(size - 1, s)] = 0;
#undef F
#undef P

    if (oper_warnings != 0) {
        T_E("Not all operational warnings are handled. Missing = 0x%x", oper_warnings);
    }

    return buf;
}

//*****************************************************************************/
// redbox_error_txt()
//*****************************************************************************/
const char *redbox_error_txt(mesa_rc error)
{
    switch (error) {
    case VTSS_APPL_REDBOX_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_REDBOX_RC_NOT_SUPPORTED:
        return "IEC 62439-3 (PRP/HSR) not supported on this platform";

    case VTSS_APPL_REDBOX_RC_INTERNAL_ERROR:
        return "Internal Error: A code-update is required. See console or crashlog for details";

    case VTSS_APPL_REDBOX_RC_HW_RESOURCES:
        return "Unable to create Redbox instance. Out of H/W resources";

    case VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE:
        return "No such Redbox instance";

    case VTSS_APPL_REDBOX_RC_INSTANCE_NOT_ACTIVE:
        return "Redbox instance in not active";

    case VTSS_APPL_REDBOX_RC_NO_SUCH_NT_INSTANCE:
        return "Requested MAC address does not exist in NodesTable";

    case VTSS_APPL_REDBOX_RC_NO_SUCH_PNT_INSTANCE:
        return "Requested MAC address does not exist in ProxyNodeTable";

    case VTSS_APPL_REDBOX_RC_INVALID_MODE:
        return "Invalid RedBox mode";

    case VTSS_APPL_REDBOX_RC_INVALID_HSR_MODE:
        return "Invalid HSR mode";

    case VTSS_APPL_REDBOX_RC_INVALID_NET_ID:
        return "The NetId must be a value between 1 and 7";

    case VTSS_APPL_REDBOX_RC_INVALID_LAN_ID:
        return "Invalid LAN ID";

    case VTSS_APPL_REDBOX_RC_INVALID_NT_AGE_TIME:
        return "Invalid NodesTable age time (must be between 1 and 65 seconds)";

    case VTSS_APPL_REDBOX_RC_INVALID_PNT_AGE_TIME:
        return "Invalid ProxyNodeTable age time (must be between 1 and 65 seconds)";

    case VTSS_APPL_REDBOX_RC_INVALID_DUPLICATE_DISCARD_AGE_TIME:
        return "Invalid duplicate-discard age time (must be between 10 and 10000 milliseconds)";

    case VTSS_APPL_REDBOX_RC_INVALID_SV_VLAN:
        return "Invalid supervision frame VLAN. Valid range is [0; " vtss_xstr(VTSS_APPL_VLAN_ID_MAX) "]";

    case VTSS_APPL_REDBOX_RC_INVALID_SV_PCP:
        return "Invalid supervision frame PCP. Valid range is [0; 7]";

    case VTSS_APPL_REDBOX_RC_INVALID_SV_FRAME_INTERVAL:
        return "Invalid supervision frame interval (must be between 1 and 60 seconds)";

    case VTSS_APPL_REDBOX_RC_INVALID_PORT_A_IFINDEX:
        return "Invalid Port A interface";

    case VTSS_APPL_REDBOX_RC_INVALID_PORT_B_IFINDEX:
        return "Invalid Port B interface";

    case VTSS_APPL_REDBOX_RC_REDBOX_1_CANNOT_HAVE_A_LEFT_NEIGHBOR:
        return "Port A on the first RedBox instance cannot refer to a neighbor to the left, and must therefore refer to a port";

    case VTSS_APPL_REDBOX_RC_REDBOX_N_CANNOT_HAVE_A_RIGHT_NEIGHBOR:
        return "Port B on the last RedBox instance cannot refer to a neighbor to the right, and must therefore refer to a port";

    case VTSS_APPL_REDBOX_RC_PORT_A_OR_PORT_B_OR_BOTH_MUST_REFER_TO_A_PORT:
        return "Port A and Port B cannot both refer to their neighbor RedBox instances";

    case VTSS_APPL_REDBOX_RC_PORT_A_NOT_VALID_FOR_REDBOX_INSTANCE:
        return "The specified Port A cannot be used with this redbox instance. Use 'show redbox interfaces' for a list of valid port interfaces for a given redbox instance";

    case VTSS_APPL_REDBOX_RC_PORT_B_NOT_VALID_FOR_REDBOX_INSTANCE:
        return "The specified Port B cannot be used with this redbox instance. Use 'show redbox interfaces' for a list of valid port interfaces for a given redbox instance";

    case VTSS_APPL_REDBOX_RC_PORT_A_B_IDENTICAL:
        return "Port A and Port B cannot be identical";

    case VTSS_APPL_REDBOX_RC_MODE_MUST_BE_HSR_IF_USING_REDBOX_NEIGHBOR:
        return "If using a neigboring RedBox, the RedBox mode cannot be PRP-SAN";

    case VTSS_APPL_REDBOX_RC_LIMIT_REACHED:
        return "The maximum number of redbox instances is reached";

    case VTSS_APPL_REDBOX_RC_OUT_OF_MEMORY:
        return "Out of memory";

    default:
        T_E("Unknown error code (%u)", error);
        return "Redbox: Unknown error code";
    }
}

/******************************************************************************/
// redbox_debug_tx_spv_suspend_get()
/******************************************************************************/
mesa_rc redbox_debug_tx_spv_suspend_get(uint32_t inst, bool &own_suspended, bool &proxied_suspended)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_inst_check(inst));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    redbox_base_debug_tx_spv_suspend_get(redbox_itr->second, own_suspended, proxied_suspended);
    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_debug_tx_spv_suspend_set()
/******************************************************************************/
mesa_rc redbox_debug_tx_spv_suspend_set(uint32_t inst, bool suspend_own, bool suspend_proxied)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_inst_check(inst));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    redbox_base_debug_tx_spv_suspend_set(redbox_itr->second, suspend_own, suspend_proxied);
    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_debug_port_state_dump()
/******************************************************************************/
mesa_rc redbox_debug_port_state_dump(redbox_icli_pr_t pr)
{
    REDBOX_LOCK_SCOPE();

    redbox_base_debug_port_state_dump(pr);
    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_debug_state_dump()
/******************************************************************************/
mesa_rc redbox_debug_state_dump(uint32_t inst, redbox_icli_pr_t pr)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_inst_check(inst));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    redbox_base_debug_state_dump(redbox_itr->second, pr);
    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_debug_clear_notifications()
/******************************************************************************/
mesa_rc redbox_debug_clear_notifications(uint32_t inst)
{
    redbox_itr_t redbox_itr;

    VTSS_RC(REDBOX_inst_check(inst));

    REDBOX_LOCK_SCOPE();

    if ((redbox_itr = REDBOX_map.find(inst)) == REDBOX_map.end()) {
        return VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE;
    }

    redbox_base_debug_clear_notifications(redbox_itr->second);
    return VTSS_RC_OK;
}

extern "C" int redbox_icli_cmd_register(void);

//*****************************************************************************/
// redbox_init()
/******************************************************************************/
mesa_rc redbox_init(vtss_init_data_t *data)
{

    if (data->cmd == INIT_CMD_EARLY_INIT) {
        // Not used, but we don't want to get the T_I() below because we don't
        // know yet whether RedBox functionality is supported.
        return VTSS_RC_OK;
    }

    if (data->cmd == INIT_CMD_INIT) {
        // Gotta get the capabilities before initializing anything, because
        // if RedBox functionality is not supported on this platform, we don't
        // do anything.
        REDBOX_capabilities_set();
    }

    if (REDBOX_supported() != VTSS_RC_OK) {
        // Stop here.
        T_I("RedBox functionality is not supported. Exiting");
        return VTSS_RC_OK;
    }

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // We get initialized before the port module, to avoid loops during
        // startup if we have an active Redbox instance. However, we cannot use
        // port_count_max(), because that is not set yet, so we do it the same
        // way as the port module does.
        REDBOX_cap_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_COUNT) - fast_cap(MEBA_CAP_CPU_PORTS_COUNT);

        REDBOX_default_conf_set();

        critd_init(&REDBOX_crit, "Redbox", VTSS_MODULE_ID_REDBOX, CRITD_TYPE_MUTEX);

        if (REDBOX_module_enabled()) {
            redbox_icli_cmd_register();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
            redbox_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
            redbox_json_init();
#endif
        }

        break;

    case INIT_CMD_START:
        if (REDBOX_module_enabled()) {
#ifdef VTSS_SW_OPTION_ICFG
            // Initialize ICLI "show running" configuration
            mesa_rc redbox_icfg_init(void);
            VTSS_RC(redbox_icfg_init());
#endif
        }

        break;

    case INIT_CMD_CONF_DEF:
        if (REDBOX_module_enabled() && data->isid == VTSS_ISID_GLOBAL) {
            REDBOX_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        if (REDBOX_module_enabled()) {
            {
                REDBOX_LOCK_SCOPE();
                redbox_timer_init();
            }

            // Initialize the port-state array
            REDBOX_port_state_init();

            REDBOX_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_POST:
        if (REDBOX_module_enabled()) {
            // Now that ICLI has applied all configuration, start creating all
            // the redbox instances.
            REDBOX_LOCK_SCOPE();

            if (!REDBOX_started) {
                REDBOX_started = true;
                REDBOX_conf_update_all();
            }
        }

        break;

    case INIT_CMD_ICFG_LOADED_POST: {
        // We have postponed updating VLAN membership of all ports until
        // now, because the VLAN module calls back 4K times during
        // LOADING_POST, which can be quite cumbersome.
        REDBOX_LOCK_SCOPE();
        REDBOX_vlan_membership_updates_started = true;
        REDBOX_vlan_memberships_cache_and_oper_warnings_update_all();
    }

    break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

