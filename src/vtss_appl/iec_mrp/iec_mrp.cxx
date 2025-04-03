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

#include "main.h"
#include "conf_api.h"       /* For conf_mgmt_mac_addr_get()              */
#include "critd_api.h"
#include <microchip/ethernet/switch/api.h>
#include <vtss/appl/iec_mrp.h>
#include <vtss/appl/cfm.hxx>
#ifdef VTSS_SW_OPTION_MSTP
#include <vtss/appl/mstp.h> /* For vtss_appl_mstp_interface_config_get() */
#include "mstp_api.h"       /* For mstp_register_config_change_cb()      */
#endif
#include "iec_mrp_api.h"
#include "iec_mrp_base.hxx"
#include "iec_mrp_expose.hxx"
#include "iec_mrp_lock.hxx"
#include "iec_mrp_timer.hxx"
#include "iec_mrp_trace.h"
#include "cfm_api.h"        /* For cfm_util_XXX()                        */
#include "cfm_expose.hxx"   /* For cfm_mep_notification_status           */
#include "interrupt_api.h"  /* For vtss_interrupt_source_hook_set()      */
#include "ip_utils.hxx"     /* For operator==(mesa_mac_t)                */
#include "misc_api.h"       /* For misc_mac_txt()                        */
#include "port_api.h"       /* For port_change_register()                */
#include "vlan_api.h"       /* For vlan_mgmt_XXX()                       */
#include "subject.hxx"      /* For subject_main_thread                   */
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/event-handler.hxx>

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void iec_mrp_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void iec_mrp_json_init(void);
#endif

static vtss_trace_reg_t MRP_trace_reg = {
    VTSS_TRACE_MODULE_ID, "MRP", "Media Redundancy Protoocol"
};

static vtss_trace_grp_t MRP_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_BASE] = {
        "base",
        "Base",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_TIMER] = {
        "timers",
        "Timers",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_CALLBACK] = {
        "callback",
        "Callbacks",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_FRAME_RX] = {
        "rx",
        "Frame Rx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_FRAME_TX] = {
        "tx",
        "Frame Tx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_ICLI] = {
        "icli",
        "CLI printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_API] = {
        "api",
        "Switch API printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_CFM] = {
        "cfm",
        "CFM callbacks/notifications",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_ACL] = {
        "acl",
        "ACL Updates",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MRP_TRACE_GRP_HIST] = {
        "hist",
        "History Updates",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&MRP_trace_reg, MRP_trace_grps);

critd_t                                 IEC_MRP_crit;
uint32_t                                MRP_cap_port_cnt;
mrp_map_t                               MRP_map;
static vtss_appl_iec_mrp_capabilities_t MRP_cap;
static vtss_appl_iec_mrp_conf_t         MRP_default_conf;
static mesa_etype_t                     MRP_s_custom_tpid;
static bool                             MRP_started;
mesa_mac_t                              MRP_chassis_mac;
bool                                    MRP_can_use_afi;
bool                                    MRP_hw_support;
static CapArray<mrp_port_state_t, MESA_CAP_PORT_CNT> MRP_port_state;

// mrp_notification_status holds the per-instance state that one can get
// notifications on, that being SNMP traps or JSON notifications. Each row in
// this table is a struct of type vtss_appl_iec_mrp_notification_status_t.
mrp_notification_status_t mrp_notification_status("mrp_notification_status", VTSS_MODULE_ID_IEC_MRP);

// Use memcmp() for comparing the following types
VTSS_BASICS_MEMCMP_OPERATOR(mrp_manager_timing_t);
VTSS_BASICS_MEMCMP_OPERATOR(mrp_client_timing_t);

/******************************************************************************/
// MRP_hex2int()
// Prior to using this function, it must be checked that isxdigit(ch) != 0.
/******************************************************************************/
static uint8_t MRP_hex2int(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    return toupper(ch) - 'A' + 10;
}

/******************************************************************************/
// vtss_appl_iec_mrp_port_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_iec_mrp_port_conf_t &port_conf)
{
    o << "{ifindex = "      << port_conf.ifindex
      << ", sf_trigger = "  << iec_mrp_util_sf_trigger_to_str(port_conf.sf_trigger)
      << ", mep = "         << port_conf.mep
      << "}";

    return o;
}

/******************************************************************************/
// mrp_manager_timing_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mrp_manager_timing_t &timing)
{
    o << "{top_chg_int = "      << timing.topology_change_interval_usec  << " usec"
      << ", top_chg_cnt = "     << timing.topology_change_repeat_cnt
      << ", tst_int_short = "   << timing.test_interval_short_usec       << " usec"
      << ", tst_int_def = "     << timing.test_interval_default_usec     << " usec"
      << ", tst_mon_cnt = "     << timing.test_monitoring_cnt
      << ", tst_mon_ext_cnt = " << timing.test_monitoring_extended_cnt
      << ", lnk_stat_int = "    << timing.link_status_poll_interval_usec << " usec"
      << ", lnk_stat_cnt = "    << timing.link_status_poll_repeat_cnt
      << "}";

    return o;
}

/******************************************************************************/
// mrp_client_timing_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mrp_client_timing_t &timing)
{
    o << "{lnk_down_int = "  << timing.link_down_interval_usec << " usec"
      << ", lnk_up_int = "   << timing.link_up_interval_usec   << " usec"
      << ", lnk_chg_cnt = "  << timing.link_change_repeat_cnt
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_iec_mrp_mrm_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_iec_mrp_mrm_conf_t &conf)
{
    o << "{prio = "               << conf.prio
      << ", react_on_link_chg = " << conf.react_on_link_change
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_iec_mrp_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_iec_mrp_conf_t &conf)
{
    char buf[37];

    o << "{role = "          << iec_mrp_util_role_to_str(conf.role)
      << ", name = "         << conf.name
      << ", uuid = "         << iec_mrp_util_domain_id_to_uuid(buf, sizeof(buf), conf.domain_id)
      << ", port1 = "        << conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1] // vtss_appl_iec_mrp_port_conf_t
      << ", port2 = "        << conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2] // vtss_appl_iec_mrp_port_conf_t
      << ", vlan = "         << conf.vlan
      << ", recv_prof = "    << iec_mrp_util_recovery_profile_to_str(conf.recovery_profile)
      << ", mrm = "          << conf.mrm // vtss_appl_iec_mrp_mrm_conf_t
      << ", in_role = "      << iec_mrp_util_in_role_to_str(conf.in_role)
      << ", in_mode = "      << iec_mrp_util_in_mode_to_str(conf.in_mode)
      << ", in_id = "        << conf.in_id
      << ", in_name = "      << conf.in_name
      << ", in_port = "      << conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION] // vtss_appl_iec_mrp_port_conf_t
      << ", in_vlan = "      << conf.in_vlan
      << ", in_recv_prof = " << iec_mrp_util_recovery_profile_to_str(conf.in_recovery_profile)
      << ", admin_active = " << conf.admin_active
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_iec_mrp_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_iec_mrp_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_mrp_event_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_mrp_event_t &event)
{
    o << "{p_mask = 0x"  << vtss::hex(event.p_mask)
      << ", s_mask = 0x" << vtss::hex(event.s_mask)
      << ", i_mask = 0x" << vtss::hex(event.i_mask)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_mrp_event_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_mrp_event_t *event)
{
    o << *event;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

//*****************************************************************************/
// MRP_sf_trigger_get()
//*****************************************************************************/
static vtss_appl_iec_mrp_sf_trigger_t MRP_sf_trigger_get(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    vtss_appl_iec_mrp_oper_warnings_t w;

    if (mrp_state->conf.ring_port[port_type].sf_trigger == VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK) {
        return VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK;
    }

    // Configured for using a MEP for SF triggering, but it could be that the
    // MEP is in such a shape that we cannot use it, so that we have to revert
    // to using link-state. This depends on the operational warnings that we
    // have by now.
    switch (port_type) {
    case VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1:
        w = VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_PORT1      |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_PORT1 |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_PORT1   |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_PORT1;

        if (w & mrp_state->status.oper_warnings) {
            return VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK;
        }

        break;

    case VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2:
        w = VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_PORT2      |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_PORT2 |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_PORT2   |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_PORT2;

        if (w & mrp_state->status.oper_warnings) {
            return VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK;
        }

        break;

    case VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION:
        w = VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_IN      |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_IN |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_IN   |
            VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_IN;

        if (w & mrp_state->status.oper_warnings) {
            return VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK;
        }

        break;

    default:
        T_E("Invalid port_type (%d)", port_type);
        break;
    }

    return VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP;
}

//*****************************************************************************/
// MRP_sf_get()
//*****************************************************************************/
static mesa_rc MRP_sf_get(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool &sf)
{
    vtss_appl_iec_mrp_sf_trigger_t          sf_trigger;
    mrp_port_state_t                        *port_state;
    vtss_appl_cfm_mep_key_t                 mep_key;
    vtss_appl_cfm_mep_notification_status_t notif_status;
    mesa_rc                                 rc;

    sf_trigger = MRP_sf_trigger_get(mrp_state, port_type);

    switch (sf_trigger) {
    case VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK:
        port_state = mrp_state->port_states[port_type];
        sf = !port_state->link;

        T_I("%u: %s %u: SF = %d", mrp_state->inst, port_type, port_state->port_no, sf);
        return VTSS_RC_OK;

    case VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP:
        mep_key = mrp_state->conf.ring_port[port_type].mep;
        if ((rc = vtss_appl_cfm_mep_notification_status_get(mep_key, &notif_status)) != VTSS_RC_OK) {
            // We also get notifications whenever a MEP gets deleted, so if this
            // function returns something != VTSS_RC_OK, it's probably the
            // reason. In that case, we will also get a MEP configuration change
            // callback,  which will take proper action
            // (MRP_cfm_conf_change_callback()).
            T_I("%u: %s-MEP %s: Unable to get status: %s", mrp_state->inst, port_type, mep_key, error_txt(rc));
            sf = true;
        } else {
            sf = !notif_status.mep_ok;
        }

        T_I("%u: %s-MEP %s: SF = %d", mrp_state->inst, port_type, mep_key, sf);
        return rc;

    default:
        T_E("%u:%s: Unknown SF trigger (%d)", mrp_state->inst, port_type, sf_trigger);
        return VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }
}

//*****************************************************************************/
// MRP_sf_update()
//*****************************************************************************/
static void MRP_sf_update(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    bool sf;

    (void)MRP_sf_get(mrp_state, port_type, sf);
    T_IG(MRP_TRACE_GRP_CFM, "%u: Setting %s-sf = %d", mrp_state->inst, port_type, sf);
    mrp_base_sf_set(mrp_state, port_type, sf);
}

//*****************************************************************************/
// MRP_cfm_change_notifications.
// Snoops on changes to the CFM MEP change notification structure to be able
// to react to changes in the MEP status (SF).
/******************************************************************************/
static struct mrp_cfm_change_notifications_t : public vtss::notifications::EventHandler {
    vtss::notifications::Event              e;
    cfm_mep_notification_status_t::Observer o;

    mrp_cfm_change_notifications_t() : EventHandler(&vtss::notifications::subject_main_thread), e(this)
    {
    }

    void init()
    {
        cfm_mep_notification_status.observer_new(&e);
    }

    void execute(vtss::notifications::Event *event)
    {
        vtss_appl_iec_mrp_port_conf_t *port_conf;
        vtss_appl_iec_mrp_port_type_t port_type;
        mrp_itr_t                     itr;
        mrp_state_t                   *mrp_state;

        // The observer_get() moves all the events captured per key into
        // #o. This object contains a map, events, whose events->first is a
        // vtss_appl_cfm_mep_key_t and whose events->second is an integer
        // indicating whether event->first has been added (EventType::Add; 1),
        // modified (EventType::Modify; 2), or deleted (EventType::Delete; 3).
        // The observer runs a state machine, so that it only needs to return
        // one value per key. So if e.g. first an Add, then a Delete operation
        // was performed on a key before this execute() function got invoked,
        // the observer's event map would have EventType::None (0), which would
        // be erased from the map, so that we don't get to see it.
        cfm_mep_notification_status.observer_get(&e, o);

        for (auto i = o.events.cbegin(); i != o.events.cend(); ++i) {
            T_DG(MRP_TRACE_GRP_CFM, "%s: %s event", i->first, vtss::notifications::EventType::txt[vtss::notifications::EventType::E(i->second)].valueName);
        }

        MRP_LOCK_SCOPE();

        if (!MRP_started) {
            // Defer these change notifications until we've told the base about
            // all MRP instances after boot.
            return;
        }

        // Loop through all MRP instances
        for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
            mrp_state = &itr->second;

            if (mrp_state->status.oper_state != VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
                // Don't care about MEP changes on this instance, since it's not
                // active.
                continue;
            }

            for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
                if (!mrp_state->using_port_type(port_type)) {
                    // This MRP instance is not using interconnection port.
                    continue;
                }

                port_conf = &mrp_state->conf.ring_port[port_type];

                if (port_conf->sf_trigger != VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP) {
                    // Not using MEP-triggered SF.
                    continue;
                }

                if (o.events.find(port_conf->mep) == o.events.end()) {
                    // No events on this ring-port's MEP
                    continue;
                }

                // Event on this MEP-SF-triggered ring port.
                MRP_sf_update(mrp_state, port_type);
            }
        }
    }
} MRP_cfm_change_notifications;

/******************************************************************************/
// MRP_base_activate()
/******************************************************************************/
static mesa_rc MRP_base_activate(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    bool                          port_sf[3];

    // Gotta find the initial values of signal fail for the three ports.
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        if (mrp_state->using_port_type(port_type)) {
            VTSS_RC(MRP_sf_get(mrp_state, port_type, port_sf[port_type]));
        } else {
            port_sf[port_type] = true;
        }
    }

    return mrp_base_activate(mrp_state, port_sf[0], port_sf[1], port_sf[2]);
}

/******************************************************************************/
// MRP_capabilities_set()
// This is the only place where we define maximum values for various parameters,
// so if you need different max. values, change here - only!
/******************************************************************************/
static void MRP_capabilities_set(void)
{
    if (!MRP_can_use_afi) {
        // On chips where we cannot use the AFI to transmit frames, we only
        // support one single MRP instance, since everything needs to be done in
        // S/W.
        MRP_cap.inst_cnt_max = 1;

        // And the fastest recovery-profile we support is 200ms.
        MRP_cap.fastest_recovery_profile = VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS;
    } else if (MRP_hw_support) {
        // On chips with H/W support, we can have up to number of ports divided
        // by two instances, since each instance requires two ring ports, and
        // because it's the whole port that gets blocked/unblocked and not only
        // a particular set of VLANs. There will be less if some of the
        // instances use interconnections.
        MRP_cap.inst_cnt_max = fast_cap(MESA_CAP_MRP_CNT);

        // The fastest recovery-profile we support is 10ms.
        MRP_cap.fastest_recovery_profile = VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS;
    } else {
        // On chips with AFI support, but no MRP H/W support, we support two
        //  MRP instances.
        MRP_cap.inst_cnt_max = 2;

        // And the fastest recovery-profile we support is 200ms.
        MRP_cap.fastest_recovery_profile = VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS;
    }

    // H/W transmits MRP_Test and MRP_InTest PDUs? If so, the Tx counters for
    // these frames types do not count.
    MRP_cap.hw_tx_test_pdus = MRP_can_use_afi;
}

/******************************************************************************/
// MRP_manager_timing_get()
/******************************************************************************/
static void MRP_manager_timing_get(mrp_manager_timing_t &timing, vtss_appl_iec_mrp_conf_t &conf, bool is_mrm)
{
    vtss_appl_iec_mrp_recovery_profile_t recovery_profile = is_mrm ? conf.recovery_profile : conf.in_recovery_profile;

    vtss_clear(timing);

    // The timing structure is filled out using Table 59 if is_mrm is true and
    // Table 61 if is_mrm is false. is_mrm == false implies MIM.
    switch (recovery_profile) {
    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS:
        if (!is_mrm) {
            T_E("No 10ms profile is specified for MIM");
            return;
        }

        // MRM only
        timing.topology_change_interval_usec   =  500; // MRP_TOPchgT
        timing.topology_change_repeat_cnt      =    3; // MRP_TOPNRmax
        timing.test_interval_short_usec        =  500; // MRP_TSTshortT
        timing.test_interval_default_usec      = 1000; // MRP_TSTdefaultT
        timing.test_monitoring_cnt             =    3; // MRP_TSTNRmax
        break;

    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS:
        if (!is_mrm) {
            T_E("No 30ms profile is specified for MIM");
            return;
        }

        // MRM only
        timing.topology_change_interval_usec   =  500; // MRP_TOPchgT
        timing.topology_change_repeat_cnt      =    3; // MRP_TOPNRmax
        timing.test_interval_short_usec        = 1000; // MRP_TSTshortT
        timing.test_interval_default_usec      = 3500; // MRP_TSTdefaultT
        timing.test_monitoring_cnt             =    3; // MRP_TSTNRmax
        break;

    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS:
        // MRM : MIM
        timing.topology_change_interval_usec   = is_mrm ? 10000 : 10000; // MRP_TOPchgT     : MRP_IN_TOPchgT
        timing.topology_change_repeat_cnt      = is_mrm ?     3 :     3; // MRP_TOPNRmax    : MRP_IN_TOPNRmax
        timing.test_interval_short_usec        = is_mrm ? 10000 :     0; // MRP_TSTshortT   : Not used
        timing.test_interval_default_usec      = is_mrm ? 20000 : 20000; // MRP_TSTdefaultT : MRP_IN_TSTdefaultT
        timing.test_monitoring_cnt             = is_mrm ?     3 :     8; // MRP_TSTNRmax    : MRP_IN_TSTNRmax
        timing.test_monitoring_extended_cnt    = is_mrm ?     0 :     0; // Not used        : Not used
        timing.link_status_poll_interval_usec  = is_mrm ?     0 : 20000; // Not used        : MRP_IN_LNKSTATchgT
        timing.link_status_poll_repeat_cnt     = is_mrm ?     0 :     8; // Not used        : MRP_IN_LNKSTATNRmax
        break;

    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS:
        // MRM : MIM
        timing.topology_change_interval_usec   = is_mrm ? 20000 : 20000; // MRP_TOPchgT     : MRP_IN_TOPchgT
        timing.topology_change_repeat_cnt      = is_mrm ?     3 :     3; // MRP_TOPNRmax    : MRP_IN_TOPNRmax
        timing.test_interval_short_usec        = is_mrm ? 30000 :     0; // MRP_TSTshortT   : Not used
        timing.test_interval_default_usec      = is_mrm ? 50000 : 50000; // MRP_TSTdefaultT : MRP_IN_TSTdefaultT
        timing.test_monitoring_cnt             = is_mrm ?     5 :     8; // MRP_TSTNRmax    : MRP_IN_TSTNRmax
        timing.test_monitoring_extended_cnt    = is_mrm ?    15 :     0; // MRP_TSTExtNRmax : Not used
        timing.link_status_poll_interval_usec  = is_mrm ?     0 : 20000; // Not used        : MRP_IN_LNKSTATchgT
        timing.link_status_poll_repeat_cnt     = is_mrm ?     0 :     8; // Not used        : MRP_IN_LNKSTATNRmax
        break;

    default:
        T_E("Invalid recovery_profile (%d)", recovery_profile);
        break;
    }
}

/******************************************************************************/
// MRP_client_timing_get()
/******************************************************************************/
static void MRP_client_timing_get(mrp_client_timing_t &timing, vtss_appl_iec_mrp_conf_t &conf, bool is_mrc)
{
    vtss_appl_iec_mrp_recovery_profile_t recovery_profile = is_mrc ? conf.recovery_profile : conf.in_recovery_profile;

    vtss_clear(timing);

    // The timing structure is filled out using Table 60 if is_mrc is true and
    // Table 62 if is_mrc is false. is_mrc == false implies MIC.
    switch (recovery_profile) {
    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS:
    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS:
        if (!is_mrc) {
            T_E("No 10ms or 30ms profile is specified for MIC");
            return;
        }

        // MRC only
        timing.link_down_interval_usec = 1000; // MRP_LNKdownT
        timing.link_up_interval_usec   = 1000; // MRP_LNKupT
        timing.link_change_repeat_cnt  =    4; // MRP_LNKNRmax
        break;

    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS:
    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS:
        // MRC and MIC
        timing.link_down_interval_usec = 20000; // Both MRP_LNKdownT and MRP_IN_LNKdownT
        timing.link_up_interval_usec   = 20000; // Both MRP_LNKupT   and MRP_IN_LNKupT
        timing.link_change_repeat_cnt  =     4; // Both MRP_LNKNRmax and MRP_IN_LNKNRmax
        break;

    default:
        T_E("Invalid recovery_profile (%d)", recovery_profile);
        break;
    }
}

/******************************************************************************/
// MRP_default_conf_set()
/******************************************************************************/
static void MRP_default_conf_set(void)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    vtss_appl_iec_mrp_port_conf_t *port_conf;

    vtss_clear(MRP_default_conf);

    memset(MRP_default_conf.domain_id, 0xff, sizeof(MRP_default_conf.domain_id));

    MRP_default_conf.role                              = VTSS_APPL_IEC_MRP_ROLE_MRA;
    MRP_default_conf.oui_type                          = VTSS_APPL_IEC_MRP_OUI_TYPE_DEFAULT;
    MRP_default_conf.vlan                              = 0;
    MRP_default_conf.recovery_profile                  = VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS;
    MRP_default_conf.mrm.prio                          = 0xa000; // Default for MRA
    MRP_default_conf.mrm.react_on_link_change          = false;
    MRP_default_conf.in_role                           = VTSS_APPL_IEC_MRP_IN_ROLE_NONE;
    MRP_default_conf.in_mode                           = VTSS_APPL_IEC_MRP_IN_MODE_LC;
    MRP_default_conf.in_id                             = 0;
    MRP_default_conf.in_vlan                           = 0;
    MRP_default_conf.in_recovery_profile               = VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        port_conf = &MRP_default_conf.ring_port[port_type];
        port_conf->sf_trigger = VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK;
        port_conf->ifindex    = VTSS_IFINDEX_NONE;
    }
}

/******************************************************************************/
// MRP_tx_frame_update()
/******************************************************************************/
static void MRP_tx_frame_update(mrp_state_t *mrp_state)
{
    if (!MRP_started) {
        // Defer all mrp_base calls until we are all set after boot
        return;
    }

    if (mrp_state->status.oper_state == VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
        // The contents of a MRP PDU has changed. Get it updated.
        mrp_base_tx_frame_update(mrp_state);
    }
}

/******************************************************************************/
// MRP_recovery_profile_update()
/******************************************************************************/
static void MRP_recovery_profile_update(mrp_state_t *mrp_state)
{
    if (!MRP_started) {
        // Defer all mrp_base calls until we are all set after boot
        return;
    }

    if (mrp_state->status.oper_state == VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
        // The recovery_profile or in_recovery_profile has changed. Get it
        // updated.
        mrp_base_recovery_profile_update(mrp_state);
    }
}

/******************************************************************************/
// MRP_frame_rx_callback()
/******************************************************************************/
static BOOL MRP_frame_rx_callback(void *contxt, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mesa_etype_t                  etype;
    mrp_itr_t                     itr;
    mrp_state_t                   *mrp_state;
    mrp_port_state_t              *port_state;

    // As long as we can't receive a frame behind two tags, the frame is always
    // normalized, so that the MRP Ethertype comes at frm[12] and frm[13],
    // because the packet module strips the outer tag.
    etype   = frm[12] << 8 | frm[13];

    T_DG(MRP_TRACE_GRP_FRAME_RX, "Rx frame on port_no = %u of length %u excl. FCS. EtherType = 0x%04x, hints = 0x%x, class-tag: tpid = 0x%04x, vid = %u, stripped_tag: tpid = 0x%04x, vid = %u",
         rx_info->port_no,
         rx_info->length,
         etype,
         rx_info->hints,
         rx_info->tag.tpid,
         rx_info->tag.vid,
         rx_info->stripped_tag.tpid,
         rx_info->stripped_tag.vid);

    if (etype != MRP_ETYPE) {
        // Why did we receive this frame? Our Packet Rx Filter tells the packet
        // module only to send us frames with MRP EtherType.
        T_EG(MRP_TRACE_GRP_FRAME_RX, "Rx frame on port_no %u with etype = 0x%04x, which isn't the MRP EtherType", rx_info->port_no, etype);

        // Not consumed
        return FALSE;
    }

    MRP_LOCK_SCOPE();

    if (!MRP_started) {
        // Don't forward frames to the base until we are all set after a boot
        T_IG(MRP_TRACE_GRP_FRAME_RX, "Skipping Rx frame, because we're not ready yet");
        return FALSE;
    }

    // Loop through all MRP states and find - amongst the active MRPs - the
    // one that matches this frame.
    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        mrp_state = &itr->second;

        if (mrp_state->status.oper_state != VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
            T_DG(MRP_TRACE_GRP_FRAME_RX, "%u: Not active", mrp_state->inst);
            continue;
        }

        // According to clause 8.1.2, Table 17, note d), the decoder shall
        // accept MRP PDUs with or without VLAN fields.

        // We may receive MRPS PDUs on both port1, port2, and on the
        // interconnection port
        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
            port_state = mrp_state->port_states[port_type];

            if (!port_state) {
                continue;
            }

            if (port_state->port_no == rx_info->port_no) {
                break;
            }
        }

        if (port_type > VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) {
            // No match in above ring-port loop
            T_DG(MRP_TRACE_GRP_FRAME_RX, "%u: Ingress port_no (%u) doesn't match any of our ring ports", mrp_state->inst, rx_info->port_no);
            continue;
        }

        T_DG(MRP_TRACE_GRP_FRAME_RX, "%u:%s handles the frame", itr->first, port_type);
        T_NG_HEX(MRP_TRACE_GRP_FRAME_RX, frm, rx_info->length);
        mrp_base_rx_frame(mrp_state, port_type, frm, rx_info);

        // Consumed
        return TRUE;
    }

    // Not consumed
    return FALSE;
}

/******************************************************************************/
// MRP_vlan_port_members_get()
/******************************************************************************/
static void MRP_vlan_port_members_get(mrp_state_t *mrp_state, mesa_vid_t vlan, mesa_port_list_t &vlan_ports)
{
    mesa_rc rc;

    T_N("%u: mesa_vlan_port_members_get(%u)", mrp_state->inst, vlan);
    if ((rc = mesa_vlan_port_members_get(nullptr, vlan, &vlan_ports)) != VTSS_RC_OK) {
        T_E("%u: mesa_vlan_port_members_get(%u) failed: %s", mrp_state->inst, vlan, error_txt(rc));
        vtss_clear(vlan_ports);
    }
}

/******************************************************************************/
// MRP_vlan_gets_tagged_on_egress()
// Returns true if frames on port_no get transmitted tagged on vid.
/******************************************************************************/
static bool MRP_vlan_gets_tagged_on_egress(mrp_state_t *mrp_state, mesa_vid_t vid, mesa_port_no_t port_no)
{
    vtss_appl_vlan_port_detailed_conf_t *vlan_conf = &MRP_port_state[port_no].vlan_conf;

    if (vid == 0) {
        T_E("%u: Invoked with VID == 0 for port_no = %u", mrp_state->inst, port_no);
        return false;
    }

    switch (vlan_conf->tx_tag_type) {
    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
        // untagged_vid gets transmitted untagged.
        return vid != vlan_conf->untagged_vid;

    case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
        // untagged_vid gets tagged, but pvid doesn't if it differs from
        // untagged_vid.
        if (vlan_conf->untagged_vid == vlan_conf->pvid) {
            // All frames get tagged
            return true;
        } else {
            // When pvid differs from untagged_vid, all but pvid get tagged.
            return vid != vlan_conf->pvid;
        }

        break;

    case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
        // All get tagged
        return true;

    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
        // All get untagged.
        return false;

    default:
        T_E("Unsupported tx_tag_type (%d)", vlan_conf->tx_tag_type);
        return false; // Whatever
    }
}

/******************************************************************************/
// MRP_oper_warnings_mep_update()
/******************************************************************************/
static void MRP_oper_warnings_mep_update(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_conf_t     *port_conf;
    vtss_appl_iec_mrp_port_type_t     port_type;
    vtss_appl_iec_mrp_oper_warnings_t &oper_warnings = mrp_state->status.oper_warnings;
    vtss_appl_cfm_md_conf_t           md_conf;
    vtss_appl_cfm_ma_conf_t           ma_conf;
    vtss_appl_cfm_mep_conf_t          mep_conf;
    vtss_ifindex_t                    ifindex;
    vtss_appl_cfm_mep_key_t           *mep_key;

    // Clear operational warnings that this function may set.
    oper_warnings &= ~(VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_PORT1      |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_PORT2      |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_IN         |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_PORT1 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_PORT2 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_IN    |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_PORT1   |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_PORT2   |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_IN      |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_PORT1 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_PORT2 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_IN);

    // Update per-port MEP warnings.
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        if (!mrp_state->using_port_type(port_type)) {
            continue;
        }

        port_conf = &mrp_state->conf.ring_port[port_type];

        if (port_conf->sf_trigger != VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP) {
            // No MEP operational warnings unless we use a MEP for Signal Fail
            // trigger.
            continue;
        }

        ifindex =  mrp_state->conf.ring_port[port_type].ifindex;
        mep_key = &mrp_state->conf.ring_port[port_type].mep;

        // MEP found?
        if (vtss_appl_cfm_md_conf_get( *mep_key, &md_conf)  != VTSS_RC_OK ||
            vtss_appl_cfm_ma_conf_get( *mep_key, &ma_conf)  != VTSS_RC_OK ||
            vtss_appl_cfm_mep_conf_get(*mep_key, &mep_conf) != VTSS_RC_OK) {
            oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_PORT1 :
                             port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_FOUND_IN;
            // Cannot set any of the other operational warnings, since the MEP
            // doesn't exist.
            continue;
        }

        // MEP administratively enabled?
        if (!mep_conf.admin_active) {
            oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_PORT1 :
                             port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_ADMIN_DISABLED_IN;
        }

        // MEP's direction is down?
        if (mep_conf.direction != VTSS_APPL_CFM_DIRECTION_DOWN) {
            oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_PORT1 :
                             port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_NOT_DOWN_MEP_IN;
        }

        // MEP's ifindex must be the same as this ring port's ifindex
        if (mep_conf.ifindex != ifindex) {
            oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_PORT1 :
                             port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_MEP_IFINDEX_DIFFER_IN;
        }
    }
}

/******************************************************************************/
// MRP_oper_warnings_stp_update()
/******************************************************************************/
static void MRP_oper_warnings_stp_update(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_oper_warnings_t &oper_warnings = mrp_state->status.oper_warnings;

    // Clear operational STP warnings first
    oper_warnings &= ~(VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_PORT1 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_PORT2 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_IN);

#ifdef VTSS_SW_OPTION_MSTP
    vtss_appl_mstp_port_config_t      mstp_conf;
    vtss_appl_iec_mrp_port_type_t     port_type;
    vtss_ifindex_t                    ifindex;
    mesa_rc                           rc;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        if (!mrp_state->using_port_type(port_type)) {
            continue;
        }

        ifindex = mrp_state->conf.ring_port[port_type].ifindex;

        if ((rc = vtss_appl_mstp_interface_config_get(ifindex, &mstp_conf)) != VTSS_RC_OK) {
            T_E("%u: vtss_appl_mstp_interface_config_get(%s) failed: %s", mrp_state->inst, ifindex, error_txt(rc));
            mstp_conf.enable = true; // Pretend it is enabled.
        }

        if (mstp_conf.enable) {
            // Clause 5.2 and 5.15: Ring/interconnection ports are to behave as
            // if STP, RSTP or MSTP is disabled. So if it's enabled, we raise
            // warnings.
            oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_PORT1 :
                             port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_STP_ENABLED_IN;
        }
    }
#else
    // STP cannot be enabled on any ports when not included in the build.
#endif
}

/******************************************************************************/
// MRP_oper_warnings_vlan_update()
//
// Check that the actual VLAN configuration is correct w.r.t. the MRP VLAN
// configuration.
//
// We operate with two different VLANs: A ring port VLAN and an interconnection
// VLAN.
//
// MRP_Xxx PDUs must be able to pass through ring ports.
//
// MRP_InXxx PDUs must be able to pass through both interconnection and ring
// ports.
//
// This function may only be invoked with enabled instances, because we then
// know that all configuration is valid.
/******************************************************************************/
static void MRP_oper_warnings_vlan_update(mrp_state_t *mrp_state)
{
    bool                              ring_vlan_untagged = mrp_state->conf.vlan    == 0;
    bool                              in_vlan_untagged   = mrp_state->conf.in_vlan == 0;
    vtss_appl_iec_mrp_oper_warnings_t &oper_warnings     = mrp_state->status.oper_warnings;
    mesa_vid_t                        vid;
    mesa_port_list_t                  vlan_ports;
    mesa_port_no_t                    port_no;
    vtss_appl_iec_mrp_port_type_t     port_type;

    // Clear operational VLAN warnings first.
    oper_warnings &= ~(VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_VLAN_PORT1 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_VLAN_PORT2 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_PORT1   |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_PORT2   |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_IN      |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_PVID_PORT1 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_PVID_PORT2 |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_PVID_DIFFER_RING_PORT1_2      |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_PORT1   |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_PORT2   |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_IN      |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_RING_VLAN_PORT1        |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_RING_VLAN_PORT2        |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_PORT1          |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_PORT2          |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_IN             |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_RING_PVID_PORT1          |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_RING_PVID_PORT2          |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_PORT1            |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_PORT2            |
                       VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_IN);

    if (ring_vlan_untagged) {
        // Untagged ring VLAN.
        // Use the two ring ports' PVID for VLANs
        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2; port_type++) {
            vid     = mrp_state->port_states[port_type]->vlan_conf.pvid;
            port_no = mrp_state->port_states[port_type]->port_no;

            MRP_vlan_port_members_get(mrp_state, vid, vlan_ports);

            if (!vlan_ports[port_no]) {
                // This ring port is not a member of its own PVID.
                oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_PVID_PORT1 : VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_PVID_PORT2;
            }

            if (MRP_vlan_gets_tagged_on_egress(mrp_state, vid, port_no)) {
                // We are configured for untagged operation, but this PVID gets
                // tagged on egress.
                oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_RING_PVID_PORT1 : VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_RING_PVID_PORT2;
            }
        }

        if (mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1]->vlan_conf.pvid != mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2]->vlan_conf.pvid) {
            // PVID differs between the two ring ports.
            oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_PVID_DIFFER_RING_PORT1_2;
        }
    } else {
        // Tagged ring VLAN
        vid = mrp_state->conf.vlan;
        MRP_vlan_port_members_get(mrp_state, vid, vlan_ports);

        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2; port_type++) {
            port_no = mrp_state->port_states[port_type]->port_no;

            if (!vlan_ports[port_no]) {
                // This ring port is not a member of the ring VLAN ID.
                oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_VLAN_PORT1 : VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_VLAN_PORT2;
            }

            if (!MRP_vlan_gets_tagged_on_egress(mrp_state, vid, port_no)) {
                // We are configured for tagged operation, but the ring VLAN
                // does not get tagged on egress.
                oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_RING_VLAN_PORT1 : VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_RING_VLAN_PORT2;
            }
        }
    }

    if (mrp_state->using_port_type(VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION)) {
        // Interconnection port is in use.
        // Get either the members of the configured interconnection VLAN or of
        // the interconnection port's PVID.

        vid = in_vlan_untagged ? mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION]->vlan_conf.pvid : mrp_state->conf.in_vlan;
        MRP_vlan_port_members_get(mrp_state, vid, vlan_ports);

        // Loop through all three ports to see if they are members of the
        // interconnection port's PVID (untagged) or the interconnection VLAN
        // (tagged operation).
        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
            port_no = mrp_state->port_states[port_type]->port_no;
            if (!vlan_ports[port_no]) {
                if (in_vlan_untagged) {
                    // This ring/interconnection port is not member of the
                    // interconnection port's PVID.
                    oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_PORT1 :
                                     port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_PVID_IN;
                } else {
                    // This ring/interconnection port is not member of the
                    // configured interconnection VLAN.
                    oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_PORT1 :
                                     port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_IN_VLAN_IN;
                }
            }

            if (MRP_vlan_gets_tagged_on_egress(mrp_state, vid, port_no)) {
                // The VLAN ID used for interconnection PDUs gets tagged on
                // egress...
                if (in_vlan_untagged) {
                    // ...but we are using untagged interconnection PDUs.
                    oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_PORT1 :
                                     port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_TAGS_IN_PVID_IN;
                }
            } else {
                // The VLAN ID used for interconnection PDUs gets untagged on
                // egress.
                if (!in_vlan_untagged) {
                    // ...but we are using tagged interconnection PDUs.
                    oper_warnings |= port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_PORT1 :
                                     port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_PORT2 : VTSS_APPL_IEC_MRP_OPER_WARNING_UNTAGS_IN_VLAN_IN;
                }
            }
        }
    }
}

/******************************************************************************/
// MRP_vlan_custom_etype_change_callback()
/******************************************************************************/
static void MRP_vlan_custom_etype_change_callback(mesa_etype_t tpid)
{
    mrp_itr_t                     itr;
    mesa_port_no_t                port_no;
    mrp_port_state_t              *port_state;
    mrp_state_t                   *mrp_state;
    vtss_appl_iec_mrp_port_type_t port_type;

    T_IG(MRP_TRACE_GRP_CALLBACK, "S-Custom TPID: 0x%04x -> 0x%04x", MRP_s_custom_tpid, tpid);

    MRP_LOCK_SCOPE();

    if (MRP_s_custom_tpid == tpid) {
        return;
    }

    MRP_s_custom_tpid = tpid;

    for (port_no = 0; port_no < MRP_cap_port_cnt; port_no++) {
        port_state = &MRP_port_state[port_no];

        if (port_state->vlan_conf.port_type != VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM) {
            continue;
        }

        port_state->tpid = MRP_s_custom_tpid;

        // Loop across all MRP instances and see if they use this port state.
        for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
            mrp_state = &itr->second;

            for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
                if (port_state == mrp_state->port_states[port_type]) {
                    // This MRP instance uses the port in question.
                    MRP_tx_frame_update(mrp_state);
                    break;
                }
            }
        }
    }
}

/******************************************************************************/
// MRP_vlan_port_conf_change_callback()
/******************************************************************************/
static void MRP_vlan_port_conf_change_callback(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *new_vlan_conf)
{
    mrp_itr_t                     itr;
    mrp_port_state_t              *port_state = &MRP_port_state[port_no];
    mrp_state_t                   *mrp_state;
    mesa_etype_t                  new_tpid;
    vtss_appl_iec_mrp_port_type_t port_type;
    bool                          port_type_changed, pvid_changed, tpid_changed, tx_tagging_changed;

    MRP_LOCK_SCOPE();

    T_DG(MRP_TRACE_GRP_CALLBACK, "port_no = %u: pvid: %u -> %u, uvid: %u -> %u, port_type: %s -> %s, tx_tag_type = %s -> %s",
         port_no,
         port_state->vlan_conf.pvid,
         new_vlan_conf->pvid,
         port_state->vlan_conf.untagged_vid,
         new_vlan_conf->untagged_vid,
         vlan_mgmt_port_type_to_txt(port_state->vlan_conf.port_type),
         vlan_mgmt_port_type_to_txt(new_vlan_conf->port_type),
         vlan_mgmt_tx_tag_type_to_txt(port_state->vlan_conf.tx_tag_type, FALSE),
         vlan_mgmt_tx_tag_type_to_txt(new_vlan_conf->tx_tag_type, FALSE));

    port_type_changed  = port_state->vlan_conf.port_type   != new_vlan_conf->port_type;
    pvid_changed       = port_state->vlan_conf.pvid        != new_vlan_conf->pvid;
    tx_tagging_changed = port_state->vlan_conf.tx_tag_type != new_vlan_conf->tx_tag_type || port_state->vlan_conf.untagged_vid != new_vlan_conf->untagged_vid;

    port_state->vlan_conf = *new_vlan_conf;

    if (!port_type_changed && !pvid_changed && !tx_tagging_changed) {
        // Nothing more to do. We got invoked because of another VLAN port
        // configuration change.
        return;
    }

    switch (port_state->vlan_conf.port_type) {
    case VTSS_APPL_VLAN_PORT_TYPE_UNAWARE:
    case VTSS_APPL_VLAN_PORT_TYPE_C:
        new_tpid = VTSS_APPL_VLAN_C_TAG_ETHERTYPE;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S:
        new_tpid = 0x88a8;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM:
        new_tpid = MRP_s_custom_tpid;
        break;

    default:
        T_E("Unknown port_type (%d)", port_state->vlan_conf.port_type);
        new_tpid = port_state->tpid;
        break;
    }

    tpid_changed = port_state->tpid != new_tpid;
    port_state->tpid = new_tpid;

    if (!tpid_changed && !pvid_changed && !tx_tagging_changed) {
        return;
    }

    // Loop across all MRP instances and see if they need an update.
    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        mrp_state = &itr->second;

        if (mrp_state->status.oper_state != VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
            continue;
        }

        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
            if (port_state == mrp_state->port_states[port_type]) {
                // This MRP instance uses the port in question.
                MRP_tx_frame_update(mrp_state);
                MRP_oper_warnings_vlan_update(mrp_state);
                break;
            }
        }
    }
}

/******************************************************************************/
// MRP_vlan_membership_change_callback()
/******************************************************************************/
static void MRP_vlan_membership_change_callback(vtss_isid_t isid, mesa_vid_t vid, vlan_membership_change_t *changes)
{
    mrp_itr_t   itr;
    mrp_state_t *mrp_state;

    T_NG(MRP_TRACE_GRP_CALLBACK, "Enter, vid = %u", vid);

    MRP_LOCK_SCOPE();

    // Loop across all MRP instances and update the operational warnings.
    // We can't just check whether the VLAN ID we are changing is either the
    // ring VLAN or the interconnection VLAN, because if untagged operation is
    // requested for either, the PVID is used.
    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        mrp_state = &itr->second;

        if (mrp_state->status.oper_state != VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
            continue;
        }

        MRP_oper_warnings_vlan_update(mrp_state);
    }
}

/******************************************************************************/
// MRP_cfm_conf_change_callback()
/******************************************************************************/
static void MRP_cfm_conf_change_callback(void)
{
    mrp_itr_t   itr;
    mrp_state_t *mrp_state;

    T_IG(MRP_TRACE_GRP_CFM, "CFM conf changed");

    // Some configuration has changed in the CFM module. Check if this has any
    // influence on the MRP instances.
    MRP_LOCK_SCOPE();

    // This function is called whenever any CFM configuration has changed, so we
    // need to go through all MRP instances to see if something has happened.
    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        mrp_state = &itr->second;

        if (mrp_state->status.oper_state != VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
            continue;
        }

        MRP_oper_warnings_mep_update(mrp_state);
    }
}

#ifdef VTSS_SW_OPTION_MSTP
/******************************************************************************/
// MRP_stp_config_change_callback()
/******************************************************************************/
static void MRP_stp_config_change_callback(void)
{
    mrp_itr_t   itr;
    mrp_state_t *mrp_state;

    T_IG(MRP_TRACE_GRP_CALLBACK, "Enter");

    MRP_LOCK_SCOPE();

    // This function is called whenever any STP configuration has changed, so we
    // need to go through all MRP instances to see if something has happened.
    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        mrp_state = &itr->second;

        if (mrp_state->status.oper_state != VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
            continue;
        }

        MRP_oper_warnings_stp_update(mrp_state);
    }
}
#endif

/******************************************************************************/
// MRP_link_state_change()
// In case we don't base the protection switching on MEPs, we can do it simply
// based on our own port states. This will, however, only work for link-to-link
// protections, not across a network.
/******************************************************************************/
static void MRP_link_state_change(const char *func, mesa_port_no_t port_no, bool link)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_state_t                   *mrp_state;
    mrp_port_state_t              *port_state;
    mrp_itr_t                     itr;

    if (port_no >= MRP_cap_port_cnt) {
        T_EG(MRP_TRACE_GRP_CALLBACK, "Invalid port_no (%u). Max = %u", port_no, MRP_cap_port_cnt);
        return;
    }

    port_state = &MRP_port_state[port_no];

    T_DG(MRP_TRACE_GRP_CALLBACK, "%s: port_no = %u: Link state: %d -> %d", func, port_no, port_state->link, link);

    if (link == port_state->link) {
        return;
    }

    port_state->link = link;

    if (!MRP_started) {
        // Defer all mrp_base updates until we are all set after a boot.
        return;
    }

    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        mrp_state = &itr->second;

        if (mrp_state->status.oper_state != VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
            // This MRP instance is not active, so it doesn't care.
            continue;
        }

        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
            if (!mrp_state->using_port_type(port_type)) {
                // Interconnection port is not in use. Don't change SF.
                continue;
            }

            if (mrp_state->port_states[port_type] != port_state) {
                // Not for this port.
                continue;
            }

            if (!link || MRP_sf_trigger_get(mrp_state, port_type) == VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK) {
                // This port is either using link as SF trigger or the link went
                // down. That is, we cannot use link-up when SF-trigger is MEP.
                // In that case we need to wait for the MEP to signal non-SF.
                mrp_base_sf_set(mrp_state, port_type, !link);
            }
        }
    }
}

/******************************************************************************/
// MRP_port_link_state_change_callback()
/******************************************************************************/
static void MRP_port_link_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    MRP_LOCK_SCOPE();
    MRP_link_state_change(__FUNCTION__, port_no, status->link);
}

/******************************************************************************/
// MRP_port_interrupt_callback()
/******************************************************************************/
static void MRP_port_interrupt_callback(meba_event_t source_id, u32 port_no)
{
    mesa_rc rc;

    MRP_LOCK_SCOPE();

    // We need to re-register for link-state-changes
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_IEC_MRP, MRP_port_interrupt_callback, source_id, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_CALLBACK, "vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    MRP_link_state_change(__FUNCTION__, port_no, false);
}

/******************************************************************************/
// MRP_frame_rx_register()
/******************************************************************************/
static void MRP_frame_rx_register(void)
{
    void               *filter_id;
    packet_rx_filter_t filter;
    mesa_rc            rc;

    packet_rx_filter_init(&filter);

    // We only need to install one packet filter, matching on the MRP ethertype.
    // The reason is that the packet module strips a possible outer tag before
    // it starts matching. This means that as long as we only support MRP behind
    // one tag, this is good enough. If we some day start supporting MRP behind
    // two (or more tags), we must also install filters for the inner tag and
    // let the MRP_frame_rx_callback() function look at the EtherType behind
    // that one tag.
    filter.modid = VTSS_MODULE_ID_IEC_MRP;
    filter.match = PACKET_RX_FILTER_MATCH_ETYPE;
    filter.cb    = MRP_frame_rx_callback;

    // We really need the frames ASAP on a high-priority thread, because if we
    // are an MRA in MRC mode and start to miss MRP_Test PDUs, we will
    // transition to MRM.
    filter.thread_prio = VTSS_THREAD_PRIO_HIGHEST;
    filter.prio        = PACKET_RX_FILTER_PRIO_SUPER;
    filter.etype       = MRP_ETYPE;
    if ((rc = packet_rx_filter_register(&filter, &filter_id)) != VTSS_RC_OK) {
        T_E("packet_rx_filter_register() failed: %s", error_txt(rc));
    }
}

/******************************************************************************/
// MRP_ptr_check()
/******************************************************************************/
static mesa_rc MRP_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_IEC_MRP_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// MRP_inst_check()
// Range [1; MRP_cap.inst_cnt_max]
/******************************************************************************/
static mesa_rc MRP_inst_check(uint32_t inst)
{
    if (inst < 1 || inst > MRP_cap.inst_cnt_max) {
        return VTSS_APPL_IEC_MRP_RC_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_ifindex_to_port()
/******************************************************************************/
static mesa_rc MRP_ifindex_to_port(vtss_ifindex_t ifindex, vtss_appl_iec_mrp_port_type_t port_type, mesa_port_no_t &port_no)
{
    vtss_ifindex_elm_t ife;

    // Check that we can decompose the ifindex and that it's a port.
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        port_no = VTSS_PORT_NO_NONE;
        return port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_RC_INVALID_PORT1_IFINDEX :
               port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_RC_INVALID_PORT2_IFINDEX : VTSS_APPL_IEC_MRP_RC_INVALID_INTERCONNECTION_IFINDEX;
    }

    port_no = ife.ordinal;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// MRP_name_chk()
/******************************************************************************/
static mesa_rc MRP_name_chk(const char *name, size_t size, bool is_in)
{
    size_t len = strlen(name), i;
    const char *p;

    // Check lengths
    if (len > size - 1) {
        return is_in ? VTSS_APPL_IEC_MRP_RC_NAME_NOT_NULL_TERMINATED_INTERCONNECTION : VTSS_APPL_IEC_MRP_RC_NAME_NOT_NULL_TERMINATED;
    }

    // All chars must be within [32; 126] (isprint()).
    p = name;
    for (i = 0; i < len; i++) {
        if (!isprint(*(p++))) {
            return is_in ? VTSS_APPL_IEC_MRP_RC_NAME_INVALID_INTERCONNECTION : VTSS_APPL_IEC_MRP_RC_NAME_INVALID;
        }
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// MRP_name_remainder_clr()
/******************************************************************************/
static void MRP_name_remainder_clr(char *name, size_t size)
{
    size_t len = strlen(name);

    if (len >= size - 1) {
        return;
    }

    memset(&name[len], 0, size - len);
}

//*****************************************************************************/
// MRP_port_conf_chk()
/******************************************************************************/
static mesa_rc MRP_port_conf_chk(const vtss_appl_iec_mrp_conf_t *conf, vtss_appl_iec_mrp_port_type_t port_type)
{
    const vtss_appl_iec_mrp_port_conf_t *port_conf;
    mesa_port_no_t                      port_no;
    bool                                empty;

    port_conf = &conf->ring_port[port_type];

    if (port_conf->sf_trigger != VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK && port_conf->sf_trigger != VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP) {
        return port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_PORT1 :
               port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_PORT2 : VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_INTERCONNECTION;
    }

    if (conf->admin_active) {
        // Check that the ifindex is representing a port.
        if (port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) {
            if (conf->in_role != VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
                // Only check interconnection port when MIC or MIM is enabled.
                VTSS_RC(MRP_ifindex_to_port(port_conf->ifindex, port_type, port_no));
            }
        } else {
            // Both ring ports must be defined when administratively enabling
            // the instance.
            VTSS_RC(MRP_ifindex_to_port(port_conf->ifindex, port_type, port_no));
        }
    }

    if (port_conf->sf_trigger == VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP) {
        // Figure out whether the user has remembered to fill in the MEP key.
        VTSS_RC(cfm_util_key_check(port_conf->mep, &empty));

        if (!empty) {
            // The MEP key is not empty, so its contents must be valid.
            VTSS_RC(cfm_util_key_check(port_conf->mep));
        } else if (conf->admin_active) {
            // The MEP key cannot be empty when SF trigger is MEP and we are
            // administratively enabled.
            return port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_RC_PORT1_MEP_MUST_BE_SPECIFIED :
                   port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_RC_PORT2_MEP_MUST_BE_SPECIFIED : VTSS_APPL_IEC_MRP_RC_INTERCONNECTION_MEP_MUST_BE_SPECIFIED;
        }
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// MRP_vlan_chk()
/******************************************************************************/
static mesa_rc MRP_vlan_chk(mesa_vid_t vlan, bool is_in)
{
    if (vlan != 0 && (vlan < VTSS_APPL_VLAN_ID_MIN || vlan > VTSS_APPL_VLAN_ID_MAX)) {
        return is_in ? VTSS_APPL_IEC_MRP_RC_INVALID_CONTROL_VLAN_INTERCONNECTION : VTSS_APPL_IEC_MRP_RC_INVALID_CONTROL_VLAN_RING;
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// MRP_prio_chk()
// See table 30 for valid manager priorities.
/******************************************************************************/
static mesa_rc MRP_prio_chk(const vtss_appl_iec_mrp_conf_t *conf)
{
    switch (conf->role) {
    case VTSS_APPL_IEC_MRP_ROLE_MRM:
        // Valid values are 0x0000, 0x1000-0x7000, 0x8000
        if (conf->mrm.prio != 0x0000 && (conf->mrm.prio < 0x1000 || conf->mrm.prio > 0x7000) && conf->mrm.prio != 0x8000) {
            return VTSS_APPL_IEC_MRP_RC_INVALID_MRM_PRIO;
        }

        break;

    case VTSS_APPL_IEC_MRP_ROLE_MRA:
        // Valid values are 0x9000-0xF000, 0xFFFF
        if ((conf->mrm.prio < 0x9000 || conf->mrm.prio > 0xF000) && conf->mrm.prio != 0xFFFF) {
            return VTSS_APPL_IEC_MRP_RC_INVALID_MRA_PRIO;
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// MRP_inter_port_chk()
/******************************************************************************/
static mesa_rc MRP_inter_port_chk(const vtss_appl_iec_mrp_port_conf_t *conf1, const vtss_appl_iec_mrp_port_conf_t *conf2, int combination)
{
    if (!conf2) {
        // Interconnection port is nullptr, indicating that it is not in use.
        return VTSS_RC_OK;
    }

    if (conf1->ifindex == conf2->ifindex) {
        return combination == 1 ? VTSS_APPL_IEC_MRP_RC_PORT_1_2_IFINDEX_IDENTICAL  :
               combination == 2 ? VTSS_APPL_IEC_MRP_RC_PORT_1_IN_IFINDEX_IDENTICAL : VTSS_APPL_IEC_MRP_RC_PORT_2_IN_IFINDEX_IDENTICAL;
    }

    if (conf1->sf_trigger == VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP &&
        conf2->sf_trigger == VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP &&
        conf1->mep        == conf2->mep) {
        // The two ports cannot use the same MEP.
        return combination == 1 ? VTSS_APPL_IEC_MRP_RC_PORT_1_2_MEP_IDENTICAL  :
               combination == 2 ? VTSS_APPL_IEC_MRP_RC_PORT_1_IN_MEP_IDENTICAL : VTSS_APPL_IEC_MRP_RC_PORT_2_IN_MEP_IDENTICAL;
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// MRP_inter_instance_port_chk()
// It is not allowed to have ring- or interconnection-ports in common with any
// other MRP instance, since MRP protects the entire port, and not just certain
// VLANs.
// Also, it is not allowed (see clause 4, page 15, bullet #2) to have multiple
// active MRM/MRC/MRA instances while having an active MIM/MIC at the same time.
/******************************************************************************/
static mesa_rc MRP_inter_instance_port_chk(const vtss_appl_iec_mrp_conf_t *this_conf, const vtss_appl_iec_mrp_conf_t *other_conf)
{
    const vtss_appl_iec_mrp_port_conf_t *this_port_conf, *other_port_conf;
    vtss_appl_iec_mrp_port_type_t       this_port_type, other_port_type;

    for (this_port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; this_port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; this_port_type++) {
        if (this_port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION && this_conf->in_role == VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
            // Interconnection of this_conf not used.
            continue;
        }

        this_port_conf = &this_conf->ring_port[this_port_type];

        for (other_port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; other_port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; other_port_type++) {
            if (other_port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION && other_conf->in_role == VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
                // Interconnection of other_conf not used.
                continue;
            }

            other_port_conf = &other_conf->ring_port[other_port_type];

            if (other_port_conf->ifindex == this_port_conf->ifindex) {
                return this_port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_PORT1_IFINDEX :
                       this_port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_PORT2_IFINDEX : VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_IN_IFINDEX;
            }
        }
    }

    if (this_conf->in_role != VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
        // We are either a MIM or a MIC. No other active MRP instance can be
        // that at the same time (see page 15, bullet #2).
        if (other_conf->in_role != VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
            return VTSS_APPL_IEC_MRP_RC_ONLY_ONE_MIM_MIC_ON_SAME_NODE;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_voe_interrupt_callback()
/******************************************************************************/
static void MRP_voe_interrupt_callback(meba_event_t source_id, u32 instance_id)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    uint32_t                      mask;
    mrp_itr_t                     mrp_itr;
    mrp_state_t                   *mrp_state;
    mesa_mrp_event_t              mrp_event;
    mesa_rc                       rc;

    T_IG(MRP_TRACE_GRP_CALLBACK, "source_id = %d, instance_id = %d", source_id, instance_id);

    MRP_LOCK_SCOPE();

    // We need to re-register for VOE-interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_IEC_MRP, MRP_voe_interrupt_callback, source_id, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_CALLBACK, "vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    // Loop through all MRP instances to find the one(s) that generated this
    // interrupt.
    for (mrp_itr = MRP_map.begin(); mrp_itr != MRP_map.end(); ++mrp_itr) {
        mrp_state = &mrp_itr->second;
        if (!mrp_state->added_to_mesa) {
            // This instance is not administratively enabled or mesa_mrp_add()
            // failed.
            continue;
        }

        if ((rc = mesa_mrp_event_get(nullptr, mrp_state->mrp_idx(), &mrp_event)) != VTSS_RC_OK) {
            T_EG(MRP_TRACE_GRP_CALLBACK, "%u: mesa_mrp_event_get(%u) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), error_txt(rc));
            continue;
        }

        T_IG(MRP_TRACE_GRP_CALLBACK, "%u: mesa_mrp_event_get(%u) => %s", mrp_state->inst, mrp_state->mrp_idx(), mrp_event);

        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
            if (!mrp_state->using_port_type(port_type)) {
                continue;
            }

            // The API swaps p_port and s_port (and therefore p_mask and s_mask)
            // whenever the primary and secondary ring ports get swapped by us.
            // Therefore, we must figure out what we have told the API and
            // convert to the configured Port1 and Port2 accordingly.
            if (port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1) {
                mask = mrp_state->vars.prm_ring_port == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? mrp_event.p_mask : mrp_event.s_mask;
            } else if (port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2) {
                mask = mrp_state->vars.prm_ring_port == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? mrp_event.s_mask : mrp_event.p_mask;
            } else {
                mask = mrp_event.i_mask;
            }

            if (mask & MESA_MRP_EVENT_MASK_TST_LOC) {
                mrp_base_loc_set(mrp_state, port_type, false /* MRP_Test */);
            }

            if (mask & MESA_MRP_EVENT_MASK_ITST_LOC) {
                mrp_base_loc_set(mrp_state, port_type, true /* MRP_InTest */);
            }
        }
    }
}

/******************************************************************************/
// MRP_port_state_init()
/******************************************************************************/
static void MRP_port_state_init(void)
{
    vtss_appl_vlan_port_conf_t vlan_conf;
    mrp_port_state_t           *port_state;
    mesa_port_no_t             port_no;
    mesa_rc                    rc;

    {
        MRP_LOCK_SCOPE();

        MRP_s_custom_tpid = VTSS_APPL_VLAN_CUSTOM_S_TAG_DEFAULT;

        (void)conf_mgmt_mac_addr_get(MRP_chassis_mac.addr, 0);

        for (port_no = 0; port_no < MRP_cap_port_cnt; port_no++) {
            // Get current VLAN configuration as configured in H/W. We could
            // have used vtss_appl_vlan_interface_conf_get(), but that would
            // require us to provide an ifindex rather than a port number, so we
            // use the internal API instead.
            // The function returns what we need in vlan_conf.hybrid.
            if ((rc = vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, port_no, &vlan_conf, VTSS_APPL_VLAN_USER_ALL, true)) != VTSS_RC_OK) {
                T_E("vlan_mgmt_port_conf_get(%u) failed: %s", port_no, error_txt(rc));
            }

            port_state            = &MRP_port_state[port_no];
            port_state->port_no   = port_no;
            port_state->tpid      = VTSS_APPL_VLAN_C_TAG_ETHERTYPE;
            port_state->vlan_conf = vlan_conf.hybrid;
            port_state->link      = false;
            (void)conf_mgmt_mac_addr_get(port_state->smac.addr, port_no + 1);
        }
    }

    // Don't take our own mutex during callback registrations (see comment in
    // similar function in cfm.cxx)

    // Subscribe to Custom S-tag TPID changes in the VLAN module
    vlan_s_custom_etype_change_register(VTSS_MODULE_ID_IEC_MRP, MRP_vlan_custom_etype_change_callback);

    // Subscribe to VLAN port changes in the VLAN module
    vlan_port_conf_change_register(VTSS_MODULE_ID_IEC_MRP, MRP_vlan_port_conf_change_callback, TRUE);

    // Subscribe to VLAN port membership changes in the VLAN module
    vlan_membership_change_register(VTSS_MODULE_ID_IEC_MRP, MRP_vlan_membership_change_callback);

    // Subscribe to link changes in the Port module
    if ((rc = port_change_register(VTSS_MODULE_ID_IEC_MRP, MRP_port_link_state_change_callback)) != VTSS_RC_OK) {
        T_E("port_change_register() failed: %s", error_txt(rc));
    }

    // Do not subscribe to 'shutdown' commands, because that will cause
    // additional state changes, because first the MRM will go into ring open
    // state, but because the port is still open, it will receive its own
    // MRP_Test PDUs and go into the ring closed state, until it doesn't receive
    // them anymore, after which it will finally go into ring open state.

    // Subscribe to FLNK
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_IEC_MRP, MRP_port_interrupt_callback, MEBA_EVENT_FLNK, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set(FLNK) failed: %s", error_txt(rc));
    }

    // Subscribe to LoS interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_IEC_MRP, MRP_port_interrupt_callback, MEBA_EVENT_LOS, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set(LoS) failed: %s", error_txt(rc));
    }

#ifdef VTSS_SW_OPTION_MSTP
    // Subscribe to MSTP configuration changes
    if (!mstp_register_config_change_cb(MRP_stp_config_change_callback)) {
        T_E("mstp_register_config_change_cb() failed");
    }
#endif
}

//*****************************************************************************/
// MRP_conf_update()
/******************************************************************************/
static void MRP_conf_update(mrp_itr_t itr, const vtss_appl_iec_mrp_conf_t &new_conf)
{
    mrp_state_t                        *mrp_state     = &itr->second;
    vtss_appl_iec_mrp_oper_state_t     old_oper_state = mrp_state->status.oper_state;
    mesa_port_no_t                     port_no;
    vtss_appl_iec_mrp_port_type_t      port_type;
    bool                               deactivate_first, frame_update;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        mrp_state->old_port_states[port_type] = mrp_state->port_states[port_type];
    }

    mrp_state->old_conf = mrp_state->conf;
    mrp_state->conf     = new_conf;

    // The state's #inst member is only used for tracing.
    mrp_state->inst = itr->first;

    if (!MRP_started) {
        // Defer all mrp_base updates until after the CFM module is up and
        // running after a boot.
        return;
    }

    T_I("%u: Enter", mrp_state->inst);

    // Assume no warnings.
    mrp_state->status.oper_warnings = VTSS_APPL_IEC_MRP_OPER_WARNING_NONE;

    // This MRP instance administratively enabled?
    if (mrp_state->conf.admin_active) {
        mrp_state->status.oper_state = VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE;

        // Update our state with correct port states
        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
            if (!mrp_state->using_port_type(port_type)) {
                // Interconnection port not used.
                mrp_state->port_states[port_type] = nullptr;
                continue;
            }

            // The configuration's ifindex has already been checked by
            // vtss_appl_iec_mrp_conf_set(), so no need to check return value.
            (void)MRP_ifindex_to_port(mrp_state->conf.ring_port[port_type].ifindex, port_type, port_no);
            mrp_state->port_states[port_type] = &MRP_port_state[port_no];
        }

        // Update actual timing to be used by the state machines.
        MRP_manager_timing_get(mrp_state->mrm_timing, mrp_state->conf, true);
        MRP_manager_timing_get(mrp_state->mim_timing, mrp_state->conf, false);
        MRP_client_timing_get( mrp_state->mrc_timing, mrp_state->conf, true);
        MRP_client_timing_get( mrp_state->mic_timing, mrp_state->conf, false);

        // Update operational warnings arosen from VLAN configuration
        MRP_oper_warnings_vlan_update(mrp_state);

        // Update operational warnings arosen from MEP configuration
        MRP_oper_warnings_mep_update(mrp_state);

        // Update operational warnings aronse from STP configuration
        MRP_oper_warnings_stp_update(mrp_state);
    } else {
        mrp_state->status.oper_state = VTSS_APPL_IEC_MRP_OPER_STATE_ADMIN_DISABLED;
    }

    // Time to update base - if needed.
    // Check if the operational state has changed.
    if (mrp_state->status.oper_state != old_oper_state) {
        // Operational state has changed.
        if (mrp_state->status.oper_state == VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
            // Inactive-to-active
            if (MRP_base_activate(mrp_state) != VTSS_RC_OK) {
                mrp_state->status.oper_state = VTSS_APPL_IEC_MRP_OPER_STATE_INTERNAL_ERROR;
            }
        } else if (old_oper_state == VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
            // Active-to-inactive
            (void)mrp_base_deactivate(mrp_state);
        }
    } else if (mrp_state->status.oper_state == VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
        // Active-to_active.

        // To minimize the effect on the ring when the user changes a parameter,
        // we need to judge whether we can avoid disabling and re-enabling the
        // instance.

        // We divide the configuration parameters into four categories:
        // 1) Parameters that require us to deactivate the MRP instance first.
        // 2) Parameters that affect the MRP PDUs only.
        // 3) Parameters that require signal fail updates.
        // 4) Parameters that require timing updates.
        // 5) Parameters that doesn't affect anything.

        // Here's a table
        // +----------------------------------------+
        // | Parameter                   | Category |
        // |-----------------------------|----------|
        // | role                        | 1        |
        // | name                        | 5        |
        // | domain_id                   | 2        |
        // | oui_type                    | 2        |
        // | custom_oui                  | 2        |
        // | ring_port.ifindex           | 1        |
        // | ring_port.sf_trigger        | 3        |
        // | ring_port.mep               | 3        |
        // | vlan                        | 2        |
        // | recovery_profile            | 4        |
        // | mrm.prio                    | 1        |
        // | mrm.react_on_link_change    | 1        |
        // | in_role                     | 1        |
        // | in_mode                     | 1        |
        // | in_id                       | 2        |
        // | in_name                     | 5        |
        // | in_vlan                     | 2        |
        // | in_recovery_profile         | 4        |
        // +----------------------------------------+

        // Category 1 checks.
        deactivate_first = false;

        if (mrp_state->old_conf.role                                                      != mrp_state->conf.role                                                      ||
            mrp_state->old_conf.in_role                                                   != mrp_state->conf.in_role                                                   ||
            mrp_state->old_conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].ifindex != mrp_state->conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].ifindex ||
            mrp_state->old_conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].ifindex != mrp_state->conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].ifindex) {
            deactivate_first = true;
        }

        if (mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRM || mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRA) {
            if (mrp_state->old_conf.mrm.prio != mrp_state->conf.mrm.prio ||
                mrp_state->old_conf.mrm.react_on_link_change != mrp_state->conf.mrm.react_on_link_change) {
                deactivate_first = true;
            }
        }

        if (mrp_state->using_port_type(VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION)) {
            if (mrp_state->old_conf.in_mode                                                        != mrp_state->conf.in_mode ||
                mrp_state->old_conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION].ifindex != mrp_state->conf.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION].ifindex) {
                deactivate_first = true;
            }
        }

        if (deactivate_first) {
            // The configuration has changed. First deactivate the old, then
            // activate the new.
            (void)mrp_base_deactivate(mrp_state);
            if (MRP_base_activate(mrp_state) != VTSS_RC_OK) {
                mrp_state->status.oper_state = VTSS_APPL_IEC_MRP_OPER_STATE_INTERNAL_ERROR;
            }
        } else {
            // Category 2 checks
            frame_update = false;

            if (memcmp(mrp_state->old_conf.domain_id, mrp_state->conf.domain_id, sizeof(mrp_state->old_conf.domain_id)) ||
                mrp_state->old_conf.vlan != mrp_state->conf.vlan) {
                frame_update = true;
            }

            if (mrp_state->old_conf.oui_type != mrp_state->conf.oui_type) {
                frame_update = true;
            } else if (mrp_state->conf.oui_type == VTSS_APPL_IEC_MRP_OUI_TYPE_CUSTOM) {
                if (memcmp(mrp_state->conf.custom_oui, mrp_state->old_conf.custom_oui, sizeof(mrp_state->conf.custom_oui)) != 0) {
                    frame_update = true;
                }
            }

            if (mrp_state->using_port_type(VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION)) {
                if (mrp_state->old_conf.in_id   != mrp_state->conf.in_id ||
                    mrp_state->old_conf.in_vlan != mrp_state->conf.in_vlan) {
                    frame_update = true;
                }
            }

            if (frame_update) {
                MRP_tx_frame_update(mrp_state);
            }

            // Category 3 checks
            // If we are using a different signal fail source now, go and
            // update SF.
            for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
                if (!mrp_state->using_port_type(port_type)) {
                    continue;
                }

                // Signal fail may come from either the MEP or the link
                // status.
                MRP_sf_update(mrp_state, port_type);
            }

            // Category 4
            if (mrp_state->old_conf.recovery_profile != mrp_state->conf.recovery_profile ||
                (mrp_state->using_port_type(VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) &&
                 mrp_state->old_conf.in_recovery_profile != mrp_state->conf.in_recovery_profile)) {
                MRP_recovery_profile_update(mrp_state);
            }

            // Category 5 checks
            // Nothing to do
        }
    }

    // Make them in sync.
    mrp_state->old_conf = mrp_state->conf;
}

/******************************************************************************/
// MRP_conf_update_all()
/******************************************************************************/
static void MRP_conf_update_all(void)
{
    mrp_itr_t itr;

    // This function is invoked the very first time we are allowed to update
    // base, so MRP_conf_update() will see that old oper_state is inactive,
    // which in turn will cause the configuration to be applied for now active
    // instances.
    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        MRP_conf_update(itr, itr->second.conf);
    }
}

//*****************************************************************************/
// MRP_default()
/******************************************************************************/
static void MRP_default(void)
{
    vtss_appl_iec_mrp_conf_t new_conf;
    mrp_itr_t                itr, next_itr;

    MRP_LOCK_SCOPE();

    // Start by setting all MRP instances inactive and call to update the
    // configuration. This will release the MRP resources in both MESA and the
    // CFM module, so that we can erase all of it in one go afterwards.
    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        if (itr->second.conf.admin_active) {
            new_conf = itr->second.conf;
            new_conf.admin_active = false;
            MRP_conf_update(itr, new_conf);
        }
    }

    // Then erase all elements from the map.
    MRP_map.clear();
}

/******************************************************************************/
// vtss_appl_iec_mrp_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_capabilities_get(vtss_appl_iec_mrp_capabilities_t *cap)
{
    VTSS_RC(MRP_ptr_check(cap));
    *cap = MRP_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_iec_mrp_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_conf_default_get(vtss_appl_iec_mrp_conf_t *conf)
{
    VTSS_RC(MRP_ptr_check(conf));

    *conf = MRP_default_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_iec_mrp_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_conf_get(uint32_t inst, vtss_appl_iec_mrp_conf_t *conf)
{
    mrp_itr_t itr;

    VTSS_RC(MRP_inst_check(inst));
    VTSS_RC(MRP_ptr_check(conf));

    MRP_LOCK_SCOPE();

    if ((itr = MRP_map.find(inst)) == MRP_map.end()) {
        return VTSS_APPL_IEC_MRP_RC_NO_SUCH_INSTANCE;
    }

    *conf = itr->second.conf;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_iec_mrp_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_conf_set(uint32_t inst, const vtss_appl_iec_mrp_conf_t *conf)
{
    vtss_appl_iec_mrp_port_type_t       port_type;
    const vtss_appl_iec_mrp_port_conf_t *port1_conf, *port2_conf, *in_port_conf;
    vtss_appl_iec_mrp_port_conf_t       *port_conf;
    vtss_appl_iec_mrp_conf_t            local_conf, *conf2;
    mrp_itr_t                           itr, itr2;
    bool                                using_in_conf;
    int                                 i;

    VTSS_RC(MRP_inst_check(inst));
    VTSS_RC(MRP_ptr_check(conf));

    T_D("Enter. inst = %u, conf = %s", inst, *conf);

    if (conf->role != VTSS_APPL_IEC_MRP_ROLE_MRC && conf->role != VTSS_APPL_IEC_MRP_ROLE_MRM && conf->role != VTSS_APPL_IEC_MRP_ROLE_MRA) {
        return VTSS_APPL_IEC_MRP_RC_INVALID_ROLE;
    }

    if (conf->in_role != VTSS_APPL_IEC_MRP_IN_ROLE_NONE && conf->in_role != VTSS_APPL_IEC_MRP_IN_ROLE_MIC && conf->in_role != VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
        return VTSS_APPL_IEC_MRP_RC_INVALID_IN_ROLE;
    }

    if (conf->in_mode != VTSS_APPL_IEC_MRP_IN_MODE_LC && conf->in_mode != VTSS_APPL_IEC_MRP_IN_MODE_RC) {
        return VTSS_APPL_IEC_MRP_RC_INVALID_IN_MODE;
    }

    VTSS_RC(MRP_name_chk(conf->name,    sizeof(conf->name),    false));
    VTSS_RC(MRP_name_chk(conf->in_name, sizeof(conf->in_name), true));

    // Domain ID may not be all-zeros (cf. Table 38)
    for (i = 0; i < sizeof(conf->domain_id); i++) {
        if (conf->domain_id[i] != 0) {
            break;
        }
    }

    if (i == sizeof(conf->domain_id)) {
        return VTSS_APPL_IEC_MRP_RC_UUID_MAY_NOT_BE_ALL_ZEROS;
    }

    if (conf->oui_type != VTSS_APPL_IEC_MRP_OUI_TYPE_DEFAULT && conf->oui_type != VTSS_APPL_IEC_MRP_OUI_TYPE_SIEMENS && conf->oui_type != VTSS_APPL_IEC_MRP_OUI_TYPE_CUSTOM) {
        return VTSS_APPL_IEC_MRP_RC_INVALID_OUI_TYPE;
    }

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        VTSS_RC(MRP_port_conf_chk(conf, port_type));
    }

    VTSS_RC(MRP_vlan_chk(conf->vlan,    false));
    VTSS_RC(MRP_vlan_chk(conf->in_vlan, true));

    if (conf->recovery_profile != VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS  &&
        conf->recovery_profile != VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS  &&
        conf->recovery_profile != VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS &&
        conf->recovery_profile != VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS) {
        return VTSS_APPL_IEC_MRP_RC_INVALID_RECOVERY_PROFILE;
    }

    if (conf->recovery_profile < MRP_cap.fastest_recovery_profile) {
        return VTSS_APPL_IEC_MRP_RC_RECOVERY_PROFILE_NOT_SUPPORTED_ON_THIS_SWITCH;
    }

    if (conf->in_recovery_profile != VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS &&
        conf->in_recovery_profile != VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS) {
        return VTSS_APPL_IEC_MRP_RC_INVALID_RECOVERY_PROFILE_INTERCONNECTION;
    }

    using_in_conf = conf->in_role != VTSS_APPL_IEC_MRP_IN_ROLE_NONE;
    port1_conf    = &conf->ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1];
    port2_conf    = &conf->ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2];
    in_port_conf  = using_in_conf ? &conf->ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION] : nullptr;

    // Certain checks must be deferred until we are getting administratively
    // enabled
    if (conf->admin_active) {
        // Cannot check priority until we know the role, because valid values
        // differ between MRM and MRA.
        VTSS_RC(MRP_prio_chk(conf));

        // Inter port1-port2-in_port checks
        VTSS_RC(MRP_inter_port_chk(port1_conf, port2_conf,   1));
        VTSS_RC(MRP_inter_port_chk(port1_conf, in_port_conf, 2));
        VTSS_RC(MRP_inter_port_chk(port2_conf, in_port_conf, 3));
    }

    // Time to normalize the configuration, that is, default unused
    // configuration. This is in order to avoid happening to re-use dead
    // configuration if that configuration becomes active again due to a change
    // in e.g. ring-type.
    local_conf = *conf;
    if (local_conf.admin_active) {
        for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
            port_conf = &local_conf.ring_port[port_type];
            if (port_conf->sf_trigger == VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK) {
                vtss_clear(port_conf->mep);
            }
        }

        // Clear out remainder of names.
        MRP_name_remainder_clr(local_conf.name,    sizeof(local_conf.name));
        MRP_name_remainder_clr(local_conf.in_name, sizeof(local_conf.in_name));
    }

    MRP_LOCK_SCOPE();

    if ((itr = MRP_map.find(inst)) != MRP_map.end()) {
        if (memcmp(&local_conf, &itr->second, sizeof(local_conf)) == 0) {
            // No changes.
            return VTSS_RC_OK;
        }
    }

    // Check that we haven't created more instances than we can allow
    if (itr == MRP_map.end()) {
        if (MRP_map.size() >= MRP_cap.inst_cnt_max) {
            return VTSS_APPL_IEC_MRP_RC_LIMIT_REACHED;
        }
    }

    // Cross-MRP instance checks
    if (local_conf.admin_active) {
        for (itr2 = MRP_map.begin(); itr2 != MRP_map.end(); ++itr2) {
            if (itr2 == itr) {
                // Don't check against ourselves.
                continue;
            }

            conf2 = &itr2->second.conf;

            if (!conf2->admin_active) {
                continue;
            }

            VTSS_RC(MRP_inter_instance_port_chk(&local_conf, conf2));
        }
    }

    // Create a new or update an existing entry
    if ((itr = MRP_map.get(inst)) == MRP_map.end()) {
        return VTSS_APPL_IEC_MRP_RC_OUT_OF_MEMORY;
    }

    MRP_conf_update(itr, local_conf);
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_iec_mrp_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_conf_del(uint32_t inst)
{
    vtss_appl_iec_mrp_conf_t new_conf;
    mrp_itr_t                itr;

    VTSS_RC(MRP_inst_check(inst));

    MRP_LOCK_SCOPE();

    if ((itr = MRP_map.find(inst)) == MRP_map.end()) {
        return VTSS_APPL_IEC_MRP_RC_NO_SUCH_INSTANCE;
    }

    if (itr->second.conf.admin_active) {
        new_conf = itr->second.conf;
        new_conf.admin_active = false;

        // Back out of everything
        MRP_conf_update(itr, new_conf);
    }

    // Delete MRP instance from our map
    MRP_map.erase(inst);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_iec_mrp_itr()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_itr(const uint32_t *prev_inst, uint32_t *next_inst)
{
    mrp_itr_t itr;

    VTSS_RC(MRP_ptr_check(next_inst));

    MRP_LOCK_SCOPE();

    if (prev_inst) {
        // Here we have a valid prev_inst. Find the next from that one.
        itr = MRP_map.greater_than(*prev_inst);
    } else {
        // We don't have a valid prev_inst. Get the first.
        itr = MRP_map.begin();
    }

    if (itr != MRP_map.end()) {
        *next_inst = itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_iec_mrp_notification_status_get()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_notification_status_get(uint32_t inst, vtss_appl_iec_mrp_notification_status_t *const notif_status)
{
    VTSS_RC(MRP_inst_check(inst));
    VTSS_RC(MRP_ptr_check(notif_status));

    T_D("%u: Enter", inst);

    // No need to lock scope, because the .get() function is guaranteed to be
    // atomic.
    return mrp_notification_status.get(inst, notif_status);
}

/******************************************************************************/
// vtss_appl_iec_mrp_status_get()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_status_get(uint32_t inst, vtss_appl_iec_mrp_status_t *status)
{
    mrp_itr_t itr;

    VTSS_RC(MRP_inst_check(inst));
    VTSS_RC(MRP_ptr_check(status));

    MRP_LOCK_SCOPE();

    if ((itr = MRP_map.find(inst)) == MRP_map.end()) {
        return VTSS_APPL_IEC_MRP_RC_NO_SUCH_INSTANCE;
    }

    *status = itr->second.status;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_iec_mrp_statistics_clear()
/******************************************************************************/
mesa_rc vtss_appl_iec_mrp_statistics_clear(uint32_t inst)
{
    mrp_itr_t itr;

    VTSS_RC(MRP_inst_check(inst));

    MRP_LOCK_SCOPE();

    if ((itr = MRP_map.find(inst)) == MRP_map.end()) {
        return VTSS_APPL_IEC_MRP_RC_NO_SUCH_INSTANCE;
    }

    mrp_base_statistics_clear(&itr->second);

    return VTSS_RC_OK;
}

/******************************************************************************/
// iec_mrp_util_port_type_to_str()
/******************************************************************************/
const char *iec_mrp_util_port_type_to_str(vtss_appl_iec_mrp_port_type_t port_type, bool capital, bool short_form)
{
    switch (port_type) {
    case VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1:
        return short_form ? "P1" : capital ? "Port1" : "port1";

    case VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2:
        return short_form ? "P2" : capital ? "Port2" : "port2";

    case VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION:
        return short_form ? "In" : capital ? "Interconnection" : "interconnection";

    default:
        T_E("Invalid port_type (%d)", port_type);
        return short_form ? "Unk" : capital ? "Unknown" : "unknown";
    }
}

/******************************************************************************/
// iec_mrp_util_port_type_to_short_str()
/******************************************************************************/
const char *iec_mrp_util_port_type_to_short_str(vtss_appl_iec_mrp_port_type_t port_type)
{
    switch (port_type) {
    case VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1:
        return "Port1";

    case VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2:
        return "Port2";

    case VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION:
        return "In";

    default:
        T_E("Invalid port_type (%d)", port_type);
        return "Unk";
    }
}

/******************************************************************************/
// iec_mrp_util_sf_trigger_to_str()
/******************************************************************************/
const char *iec_mrp_util_sf_trigger_to_str(vtss_appl_iec_mrp_sf_trigger_t sf_trigger)
{
    switch (sf_trigger) {
    case VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK:
        return "link";

    case VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP:
        return "mep";

    default:
        T_E("Invalid sf_trigger (%d)", sf_trigger);
        return "unknown";
    }
}

/******************************************************************************/
// iec_mrp_util_role_to_str()
/******************************************************************************/
const char *iec_mrp_util_role_to_str(vtss_appl_iec_mrp_role_t role, bool capital)
{
    switch (role) {
    case VTSS_APPL_IEC_MRP_ROLE_MRC:
        return capital ? "MRC" : "mrc";

    case VTSS_APPL_IEC_MRP_ROLE_MRM:
        return capital ? "MRM" : "mrm";

    case VTSS_APPL_IEC_MRP_ROLE_MRA:
        return capital ? "MRA" : "mra";

    default:
        T_E("Invalid role (%d)", role);
        return capital ? "UNK" : "unk";
    }
}

/******************************************************************************/
// iec_mrp_util_oui_type_to_str()
/******************************************************************************/
const char *iec_mrp_util_oui_type_to_str(vtss_appl_iec_mrp_oui_type_t oui_type)
{
    switch (oui_type) {
    case VTSS_APPL_IEC_MRP_OUI_TYPE_DEFAULT:
        return "default";

    case VTSS_APPL_IEC_MRP_OUI_TYPE_SIEMENS:
        return "siemens";

    case VTSS_APPL_IEC_MRP_OUI_TYPE_CUSTOM:
        return "custom";

    default:
        T_E("Invalid oui_type (%d)", oui_type);
        return "unknown";
    }
}

/******************************************************************************/
// iec_mrp_util_domain_id_from_uuid()
// Converts an UUID (e.g. FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF) to a domain_id.
/******************************************************************************/
mesa_rc iec_mrp_util_domain_id_from_uuid(uint8_t *domain_id, const char *uuid)
{
    int        i, j;
    const char *p = uuid;

    VTSS_RC(MRP_ptr_check(domain_id));
    VTSS_RC(MRP_ptr_check(uuid));

    if (strlen(uuid) != 36) {
        return VTSS_APPL_IEC_MRP_RC_UUID_MUST_BE_EXACTLY_36_CHARS_LONG;
    }

    j = 0;
    for (i = 0; i < 35; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            // Expect a '-'
            if (*(p++) != '-') {
                return VTSS_APPL_IEC_MRP_RC_UUID_MUST_CONTAIN_DASHES_AT_POS_9_14_19_24;
            }
        } else {
            // Consume two hex chars per iteration.
            if (!isxdigit(p[0]) || !isxdigit(p[1])) {
                return VTSS_APPL_IEC_MRP_RC_UUID_MUST_ONLY_CONTAIN_HEX_DIGITS;
            }

            domain_id[j++] = 16 * MRP_hex2int(p[0]) + MRP_hex2int(p[1]);
            p += 2;
            i++; // Consumed two chars
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// iec_mrp_util_domain_id_to_uuid()
// sizeof(buf) must be at least 37.
// sizeof(domain_id) must be 16.
//
// Returns on UUID on the following format:
// FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF
/******************************************************************************/
char *iec_mrp_util_domain_id_to_uuid(char *buf, size_t size, const uint8_t *domain_id)
{
    int  i;
    char *p = buf;

    if (size < 37) {
        T_E("Size must be >= 37. Was %zu", size);
        return "unknown";
    }

    for (i = 0; i < sizeof(vtss_appl_iec_mrp_conf_t::domain_id); i++) {
        p += sprintf(p, "%02X", domain_id[i]);

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            *(p++) = '-';
        }
    }

    return buf;
}

/******************************************************************************/
// iec_mrp_util_recovery_profile_to_str()
/******************************************************************************/
const char *iec_mrp_util_recovery_profile_to_str(vtss_appl_iec_mrp_recovery_profile_t recovery_profile)
{
    switch (recovery_profile) {
    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS:
        return "10ms";

    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS:
        return "30ms";

    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS:
        return "200ms";

    case VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS:
        return "500ms";

    default:
        T_E("Invalid recovery_profile (%d)", recovery_profile);
        return "unknown";
    }
}

/******************************************************************************/
// iec_mrp_util_in_role_to_str()
/******************************************************************************/
const char *iec_mrp_util_in_role_to_str(vtss_appl_iec_mrp_in_role_t in_role, bool capital)
{
    switch (in_role) {
    case VTSS_APPL_IEC_MRP_IN_ROLE_NONE:
        return capital ? "None" : "none";

    case VTSS_APPL_IEC_MRP_IN_ROLE_MIC:
        return capital ? "MIC" : "mic";

    case VTSS_APPL_IEC_MRP_IN_ROLE_MIM:
        return capital ? "MIM" : "mim";

    default:
        T_E("Invalid in_role (%d)", in_role);
        return capital ? "UNK" : "unk";
    }
}

/******************************************************************************/
// iec_mrp_util_in_mode_to_str()
/******************************************************************************/
const char *iec_mrp_util_in_mode_to_str(vtss_appl_iec_mrp_in_mode_t in_mode, bool short_form)
{
    switch (in_mode) {
    case VTSS_APPL_IEC_MRP_IN_MODE_LC:
        return short_form ? "LC" : "link-check";

    case VTSS_APPL_IEC_MRP_IN_MODE_RC:
        return short_form ? "RC" : "ring-check";

    default:
        T_E("Invalid in_mode (%d)", in_mode);
        return short_form ? "UNK" : "unknown";
    }
}

/******************************************************************************/
// iec_mrp_util_in_role_and_mode_to_str()
/******************************************************************************/
const char *iec_mrp_util_in_role_and_mode_to_str(vtss_appl_iec_mrp_in_role_t in_role, vtss_appl_iec_mrp_in_mode_t in_mode)
{
    switch (in_role) {
    case VTSS_APPL_IEC_MRP_IN_ROLE_NONE:
        return "None";

    case VTSS_APPL_IEC_MRP_IN_ROLE_MIC:
        return in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC ? "MIC-LC" : "MIC-RC";

    case VTSS_APPL_IEC_MRP_IN_ROLE_MIM:
        return in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC ? "MIM-LC" : "MIM-RC";

    default:
        T_E("Invalid in_role (%d)", in_role);
        return "UNK";
    }
}

/******************************************************************************/
// iec_mrp_util_oper_state_to_str()
/******************************************************************************/
const char *iec_mrp_util_oper_state_to_str(vtss_appl_iec_mrp_oper_state_t oper_state)
{
    switch (oper_state) {
    case VTSS_APPL_IEC_MRP_OPER_STATE_ADMIN_DISABLED:
        return "Administratively disabled";

    case VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE:
        return "Active";

    case VTSS_APPL_IEC_MRP_OPER_STATE_INTERNAL_ERROR:
        return "Internal error has occurred. See console or crashlog for details";

    default:
        T_E("Invalid oper_state (%d)", oper_state);
        return "Unknown";
    }
}

//*****************************************************************************/
// iec_mrp_util_oper_state_to_str()
/******************************************************************************/
const char *iec_mrp_util_oper_state_to_str(vtss_appl_iec_mrp_oper_state_t oper_state, vtss_appl_iec_mrp_oper_warnings_t oper_warnings)
{
    if (oper_warnings == VTSS_APPL_IEC_MRP_OPER_WARNING_NONE || oper_state != VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE) {
        return iec_mrp_util_oper_state_to_str(oper_state);
    }

    return "Active (warnings)";
}

/******************************************************************************/
// iec_mrp_util_oper_warnings_to_txt()
// Buf must be ~400 bytes long if all bits are set.
/******************************************************************************/
char *iec_mrp_util_oper_warnings_to_txt(char *buf, size_t size, vtss_appl_iec_mrp_oper_warnings_t oper_warnings)
{
    int  s = 0, res;
    bool first = true;

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

#define F(X, _name_)                                          \
    if (oper_warnings & VTSS_APPL_IEC_MRP_OPER_WARNING_##X) { \
        oper_warnings &= ~VTSS_APPL_IEC_MRP_OPER_WARNING_##X; \
        if (first) {                                          \
            first = false;                                    \
            P(_name_);                                        \
        } else {                                              \
            P(", " _name_);                                   \
        }                                                     \
    }

    buf[0] = 0;
    s = 0;

    // Example of a field name (just so that we can search for this function):
    // VTSS_APPL_IEC_MRP_OPER_WARNING_NOT_MEMBER_OF_RING_VLAN_PORT1

    F(NOT_MEMBER_OF_RING_VLAN_PORT1, "Port1 is not member of the ring's control VLAN, which is configured for tagged operation");
    F(NOT_MEMBER_OF_RING_VLAN_PORT2, "Port2 is not member of the ring's control VLAN, which is configured for tagged operation");
    F(NOT_MEMBER_OF_IN_VLAN_PORT1,   "Port1 is not member of the interconnection control VLAN, which is configured for tagged operation");
    F(NOT_MEMBER_OF_IN_VLAN_PORT2,   "Port2 is not member of the interconnection control VLAN, which is configured for tagged operation");
    F(NOT_MEMBER_OF_IN_VLAN_IN,      "Interconnection port is not member of the interconnection control VLAN, which is configured for tagged operation");
    F(NOT_MEMBER_OF_RING_PVID_PORT1, "Port1 is not member of its own PVID (ring's control VLAN is configured for untagged operation)");
    F(NOT_MEMBER_OF_RING_PVID_PORT2, "Port2 is not member of its own PVID (ring's control VLAN is configured for untagged operation)");
    F(PVID_DIFFER_RING_PORT1_2,      "Port1 and Port2's PVID differ (ring VLAN is configured for untagged operation)");
    F(NOT_MEMBER_OF_IN_PVID_PORT1,   "Port1 is not member of the interconnection port's PVID (interconnection's control VLAN is configured for untagged operation)");
    F(NOT_MEMBER_OF_IN_PVID_PORT2,   "Port2 is not member of the interconnection port's PVID (interconnection's control VLAN is configured for untagged operation)");
    F(NOT_MEMBER_OF_IN_PVID_IN,      "Interconnection port is not member of its own PVID (interconnection's control VLAN is configured for untagged operation)");
    F(UNTAGS_RING_VLAN_PORT1,        "Port1 untags ring's control VLAN, which is configured for tagged operation");
    F(UNTAGS_RING_VLAN_PORT2,        "Port2 untags ring's control VLAN, which is configured for tagged operation");
    F(UNTAGS_IN_VLAN_PORT1,          "Port1 untags interconnection's control VLAN, which is configured for tagged operation");
    F(UNTAGS_IN_VLAN_PORT2,          "Port2 untags interconnection's control VLAN, which is configured for tagged operation");
    F(UNTAGS_IN_VLAN_IN,             "Interconnection port untags interconnection's control VLAN, which is configured for tagged operation");
    F(TAGS_RING_PVID_PORT1,          "Port1 tags its own PVID (ring's control VLAN is configured for untagged operation)");
    F(TAGS_RING_PVID_PORT2,          "Port2 tags its own PVID (ring's control VLAN is configured for untagged operation)");
    F(TAGS_IN_PVID_PORT1,            "Port1 tags the interconnection port's PVID (interconnection's control VLAN is configured for untagged operation)");
    F(TAGS_IN_PVID_PORT2,            "Port2 tags the interconnection port's PVID (interconnection's control VLAN is configured for untagged operation)");
    F(TAGS_IN_PVID_IN,               "Interconnection port tags itw own PVID (interconnection's control VLAN is configured for untagged operation)");
    F(MEP_NOT_FOUND_PORT1,           "Port1 MEP is not found. Using link-state for signal-fail instead");
    F(MEP_NOT_FOUND_PORT2,           "Port2 MEP is not found. Using link-state for signal-fail instead");
    F(MEP_NOT_FOUND_IN,              "Interconnection MEP is not found. Using link-state for signal-fail instead");
    F(MEP_ADMIN_DISABLED_PORT1,      "Port1 MEP is administratively disabled. Using link-state for signal-fail instead");
    F(MEP_ADMIN_DISABLED_PORT2,      "Port2 MEP is administratively disabled. Using link-state for signal-fail instead");
    F(MEP_ADMIN_DISABLED_IN,         "Interconnection MEP is administratively disabled. Using link-state for signal-fail instead");
    F(MEP_NOT_DOWN_MEP_PORT1,        "Port1 MEP is not a Down-MEP. Using link-state for signal-fail instead");
    F(MEP_NOT_DOWN_MEP_PORT2,        "Port2 MEP is not a Down-MEP. Using link-state for signal-fail instead");
    F(MEP_NOT_DOWN_MEP_IN,           "Interconnection MEP is not a Down-MEP. Using link-state for signal-fail instead");
    F(MEP_IFINDEX_DIFFER_PORT1,      "Port1 MEP's residence port is not that of Port1. Using link-state for signal-fail instead");
    F(MEP_IFINDEX_DIFFER_PORT2,      "Port2 MEP's residence port is not that of Port2. Using link-state for signal-fail instead");
    F(MEP_IFINDEX_DIFFER_IN,         "Interconnection MEP's residence port is not that of the interconnection port. Using link-state for signal-fail instead");
    F(STP_ENABLED_PORT1,             "Port1 has spanning tree enabled");
    F(STP_ENABLED_PORT2,             "Port2 has spanning tree enabled");
    F(STP_ENABLED_IN,                "Interconnection port has spanning tree enabled");
    F(MULTIPLE_MRMS,                 "Multiple MRMs detected on the ring. This is normal if MRAs are negotiating. Cleared after 10 seconds w/o detection");
    F(MULTIPLE_MIMS,                 "Multiple MIMs with same ID detected on the interconnection ring. Cleared after 10 seconds w/o detection");
    F(INTERNAL_ERROR,                "An internal error has occurred. A code update is required. Please check console for details");

    buf[MIN(size - 1, s)] = 0;
#undef F
#undef P

    if (oper_warnings != 0) {
        T_E("Not all operational warnings are handled. Missing = 0x%x", oper_warnings);
    }

    return buf;
}

//*****************************************************************************/
// iec_mrp_util_ring_state_to_str()
//*****************************************************************************/
const char *iec_mrp_util_ring_state_to_str(vtss_appl_iec_mrp_ring_state_t ring_state)
{
    switch (ring_state) {
    case VTSS_APPL_IEC_MRP_RING_STATE_DISABLED:
        return "Disabled";

    case VTSS_APPL_IEC_MRP_RING_STATE_OPEN:
        return "Open";

    case VTSS_APPL_IEC_MRP_RING_STATE_CLOSED:
        return "Closed";

    case VTSS_APPL_IEC_MRP_RING_STATE_UNDEFINED:
        return "Undefined";

    default:
        T_E("Invalid ring_state (%d)", ring_state);
        return "Unknown";
    }
}

//*****************************************************************************/
// iec_mrp_util_pdu_type_to_str()
//*****************************************************************************/
const char *iec_mrp_util_pdu_type_to_str(vtss_appl_iec_mrp_pdu_type_t pdu_type, bool inc_mrp)
{
    switch (pdu_type) {
    case VTSS_APPL_IEC_MRP_PDU_TYPE_TEST:
        return inc_mrp ? "MRP_Test"             : "Test";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_TOPOLOGY_CHANGE:
        return inc_mrp ? "MRP_TopologyChange"   : "TopologyChange";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_DOWN:
        return inc_mrp ? "MRP_LinkDown"         : "LinkDown";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_UP:
        return inc_mrp ? "MRP_LinkUp"           : "LinkUp";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_MGR_NACK:
        return inc_mrp ? "MRP_TestMgrNAck"      : "TestMgrNAck";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_PROPAGATE:
        return inc_mrp ? "MRP_TestPropagate"    : "TestPropagate";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION:
        return inc_mrp ? "MRP_Option"           : "Option";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST:
        return inc_mrp ? "MRP_InTest"           : "InTest";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TOPOLOGY_CHANGE:
        return inc_mrp ? "MRP_InTopologyChange" : "InTopologyChange";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_DOWN:
        return inc_mrp ? "MRP_InLinkDown"       : "InLinkDown";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_UP:
        return inc_mrp ? "MRP_InLinkUp"         : "InLinkUp";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_STATUS_POLL:
        return inc_mrp ? "MRP_InLinkStatusPoll" : "InLinkStatusPoll";

    case VTSS_APPL_IEC_MRP_PDU_TYPE_UNKNOWN:
        return inc_mrp ? "MRP_Unknown"          : "Unknown";

    default:
        T_E("Invalid pdu_type (%d)", pdu_type);
        return "INVALID";
    }
}

//*****************************************************************************/
// iec_mrp_error_txt()
//*****************************************************************************/
const char *iec_mrp_error_txt(mesa_rc error)
{
    switch (error) {
    case VTSS_APPL_IEC_MRP_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR:
        return "Internal Error: A code-update is required. See console or crashlog for details";

    case VTSS_APPL_IEC_MRP_RC_NO_SUCH_INSTANCE:
        return "No such MRP instance";

    case VTSS_APPL_IEC_MRP_RC_INVALID_ROLE:
        return "Invalid role";

    case VTSS_APPL_IEC_MRP_RC_INVALID_IN_ROLE:
        return "Invalid interconnection role";

    case VTSS_APPL_IEC_MRP_RC_INVALID_IN_MODE:
        return "Invalid interconnection mode";

    case VTSS_APPL_IEC_MRP_RC_NAME_NOT_NULL_TERMINATED:
        return "Name string is not NULL terminated";

    case VTSS_APPL_IEC_MRP_RC_NAME_NOT_NULL_TERMINATED_INTERCONNECTION:
        return "Interconnection name string is not NULL terminated";

    case VTSS_APPL_IEC_MRP_RC_NAME_INVALID:
        return "Name contains invalid characters. Only [32; 126] (isprint()) are allowed";

    case VTSS_APPL_IEC_MRP_RC_NAME_INVALID_INTERCONNECTION:
        return "Interconnection name contains invalid characters. Only [32; 126] (isprint()) are allowed";

    case VTSS_APPL_IEC_MRP_RC_UUID_MUST_BE_EXACTLY_36_CHARS_LONG:
        return "A UUID must be exactly 36 characters long";

    case VTSS_APPL_IEC_MRP_RC_UUID_MUST_CONTAIN_DASHES_AT_POS_9_14_19_24:
        return "A UUID must contain dashes (-) at position 9, 14, 19, and 24";

    case VTSS_APPL_IEC_MRP_RC_UUID_MUST_ONLY_CONTAIN_HEX_DIGITS:
        return "Besides dashes (-), a UUID may only contain hexadecimal digits";

    case VTSS_APPL_IEC_MRP_RC_UUID_MAY_NOT_BE_ALL_ZEROS:
        return "UUID may not be all-zeros";

    case VTSS_APPL_IEC_MRP_RC_INVALID_OUI_TYPE:
        return "Invalid OUI type";

    case VTSS_APPL_IEC_MRP_RC_INVALID_PORT1_IFINDEX:
        return "A valid ring port1 interface must be specified when instance is administratively enabled";

    case VTSS_APPL_IEC_MRP_RC_INVALID_PORT2_IFINDEX:
        return "A valid ring port2 interface must be specified when instance is administratively enabled";

    case VTSS_APPL_IEC_MRP_RC_INVALID_INTERCONNECTION_IFINDEX:
        return "A valid interconnection interface must be specified when instance is administratively enabled and interconnection role is enabled";

    case VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_PORT1:
        return "Invalid signal-fail trigger for port1";

    case VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_PORT2:
        return "Invalid signal-fail trigger for port2";

    case VTSS_APPL_IEC_MRP_RC_INVALID_SF_TRIGGER_INTERCONNECTION:
        return "Invalid signal-fail trigger for interconnection port";

    case VTSS_APPL_IEC_MRP_RC_PORT1_MEP_MUST_BE_SPECIFIED:
        return "Port1 MEP must be specified when using MEP as signal-fail trigger and instance is adminstratively enabled";

    case VTSS_APPL_IEC_MRP_RC_PORT2_MEP_MUST_BE_SPECIFIED:
        return "Port2 MEP must be specified when using MEP as signal-fail trigger and instance is administratively enabled";

    case VTSS_APPL_IEC_MRP_RC_INTERCONNECTION_MEP_MUST_BE_SPECIFIED:
        return "Interconnection MEP must be specified when using MEP as signal-fail trigger and instance is administratively enabled";

    case VTSS_APPL_IEC_MRP_RC_INVALID_CONTROL_VLAN_RING:
        return "Invalid ring control VLAN. Valid range is 0 and [" vtss_xstr(VTSS_APPL_VLAN_ID_MIN) "; " vtss_xstr(VTSS_APPL_VLAN_ID_MAX) "]";

    case VTSS_APPL_IEC_MRP_RC_INVALID_CONTROL_VLAN_INTERCONNECTION:
        return "Invalid interconnection control VLAN. Valid range is 0 and [" vtss_xstr(VTSS_APPL_VLAN_ID_MIN) "; " vtss_xstr(VTSS_APPL_VLAN_ID_MAX) "]";

    case VTSS_APPL_IEC_MRP_RC_INVALID_RECOVERY_PROFILE:
        return "Invalid recovery profile";

    case VTSS_APPL_IEC_MRP_RC_RECOVERY_PROFILE_NOT_SUPPORTED_ON_THIS_SWITCH:
        return "Recovery profile not supported on this switch";

    case VTSS_APPL_IEC_MRP_RC_INVALID_RECOVERY_PROFILE_INTERCONNECTION:
        return "Invalid interconnection recovery profile";

    case VTSS_APPL_IEC_MRP_RC_INVALID_MRM_PRIO:
        return "Manager priority must be 0x0000, 0x1000-0x7000, or 0x8000 when configured as MRM";

    case VTSS_APPL_IEC_MRP_RC_INVALID_MRA_PRIO:
        return "Manager priority must be 0x9000-0xF000, or 0xFFFF when configured as MRA";

    case VTSS_APPL_IEC_MRP_RC_PORT_1_2_MEP_IDENTICAL:
        return "Port1 and Port2 MEPs cannot be the same";

    case VTSS_APPL_IEC_MRP_RC_PORT_1_IN_MEP_IDENTICAL:
        return "Port1 and interconnection MEPs cannot be the same";

    case VTSS_APPL_IEC_MRP_RC_PORT_2_IN_MEP_IDENTICAL:
        return "Port2 and interconnection MEPs cannot be the same";

    case VTSS_APPL_IEC_MRP_RC_PORT_1_2_IFINDEX_IDENTICAL:
        return "Port1 and Port2 cannot use the same interface";

    case VTSS_APPL_IEC_MRP_RC_PORT_1_IN_IFINDEX_IDENTICAL:
        return "Port1 and interconnection cannot use the same interface";

    case VTSS_APPL_IEC_MRP_RC_PORT_2_IN_IFINDEX_IDENTICAL:
        return "Port2 and interconnection cannot use the same interface";

    case VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_PORT1_IFINDEX:
        return "Another administratively enabled MRP instance has an interface in common with this instance's port1";

    case VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_PORT2_IFINDEX:
        return "Another administratively enabled MRP instance has an interface in common with this instance's port2";

    case VTSS_APPL_IEC_MRP_RC_TWO_INST_WITH_SAME_IN_IFINDEX:
        return "Another administratively enabled MRP instance has an interface in common with this instance's interconnection port";

    case VTSS_APPL_IEC_MRP_RC_ONLY_ONE_MIM_MIC_ON_SAME_NODE:
        return "Another administratively enabled MRP instance is configured as a MIM or a MIC. There can only be one MRP instance that has MIM or MIC enabled at a time";

    case VTSS_APPL_IEC_MRP_RC_LIMIT_REACHED:
        return "The maximum number of MRP instances is reached";

    case VTSS_APPL_IEC_MRP_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_IEC_MRP_RC_NOT_READY_TRY_AGAIN_IN_A_FEW_SECONDS:
        return "MRP is not yet ready. Please try again in a few seconds";

    case VTSS_APPL_IEC_MRP_RC_HW_RESOURCES:
        return "Out of H/W resources";

    default:
        T_E("Unknown error code (%u)", error);
        return "MRP: Unknown error code";
    }
}

extern "C" int iec_mrp_icli_cmd_register(void);

//*****************************************************************************/
// iec_mrp_init()
/******************************************************************************/
mesa_rc iec_mrp_init(vtss_init_data_t *data)
{
    mesa_rc rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // We get initialized before the port module, to avoid loops during
        // startup if we have an active MRP instance. However, we cannot use
        // port_count_max(), because that is not set yet, so we do it the same
        // way as the port module does.
        MRP_cap_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_COUNT) - fast_cap(MEBA_CAP_CPU_PORTS_COUNT);

        // Only Lu26 doesn't have an AFI. However, those platforms with an old
        // AFI may burst frames transmitted with the AFI. This in turn causes
        // listeners to MRP_Test PDUs to time out, and MRAs start act as an MRM
        // and MRMs start unblocking their bloced port.
        // Therefore, we cannot use AFI_V1, only AFI_V2.
        MRP_can_use_afi = fast_cap(MESA_CAP_AFI_V2) != 0;

        // Do we have H/W MRP support?
        MRP_hw_support = fast_cap(MESA_CAP_MRP) != 0;

        MRP_capabilities_set();
        MRP_default_conf_set();

        critd_init(&IEC_MRP_crit, "MRP", VTSS_MODULE_ID_IEC_MRP, CRITD_TYPE_MUTEX);

        iec_mrp_icli_cmd_register();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        iec_mrp_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        iec_mrp_json_init();
#endif

        MRP_cfm_change_notifications.init();
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        // Initialize ICLI "show running" configuration
        mesa_rc iec_mrp_icfg_init(void);
        VTSS_RC(iec_mrp_icfg_init());
#endif
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            MRP_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        // Initialize the port-state array
        MRP_port_state_init();

        if (MRP_hw_support) {
            // Hook up for VOE events (Loss of MRP_Test and MRP_InTest
            // Continuity).
            if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_IEC_MRP, MRP_voe_interrupt_callback, MEBA_EVENT_VOE, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
                T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
            }
        }

        {
            MRP_LOCK_SCOPE();
            mrp_timer_init();
        }

        // Register for CFM configuration change callbacks in CFM module
        if ((rc = cfm_util_conf_change_callback_register(VTSS_MODULE_ID_IEC_MRP, MRP_cfm_conf_change_callback)) != VTSS_RC_OK) {
            T_E("cfm_util_conf_change_callback_register() failed: %s", error_txt(rc));
        }

        MRP_default();

        // Register for MRP PDUs
        MRP_frame_rx_register();

        break;

    case INIT_CMD_ICFG_LOADING_POST: {
        // Now that ICLI has applied all configuration, start creating all the
        // MRP instances. Only do this the very first time an
        // INIT_CMD_ICFG_LOADING_POST is issued.
        // It's fine not to wait for CFM to have all its MEPs up and running,
        // because we will resort to signal fail based on link if any MEP is not
        // yet constructed.
        MRP_LOCK_SCOPE();

        if (!MRP_started) {
            MRP_started = true;
            MRP_conf_update_all();
        }

        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

