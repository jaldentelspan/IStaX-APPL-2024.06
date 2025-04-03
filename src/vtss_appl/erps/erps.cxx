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
#include "conf_api.h"       /* For conf_mgmt_mac_addr_get()         */
#include "critd_api.h"
#include <microchip/ethernet/switch/api.h>
#include <vtss/appl/erps.h>
#include <vtss/appl/cfm.hxx>
#include "erps_api.h"
#include "erps_base.hxx"
#include "erps_lock.hxx"
#include "erps_timer.hxx"
#include "erps_trace.h"
#include "cfm_api.h"        /* For cfm_util_XXX()                   */
#include "cfm_expose.hxx"   /* For cfm_mep_notification_status      */
#include "interrupt_api.h"  /* For vtss_interrupt_source_hook_set() */
#include "mac_utils.hxx"    /* For operator==(mesa_mac_t)           */
#include "misc_api.h"       /* For misc_mac_txt()                   */
#include "port_api.h"       /* For port_change_register()           */
#include "vlan_api.h"       /* For vlan_mgmt_XXX()                  */
#include "subject.hxx"      /* For subject_main_thread              */
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/event-handler.hxx>

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void erps_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_erps_json_init(void);
#endif

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "ERPS", "Ethernet Ring Protection"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_BASE] = {
        "base",
        "Base",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_TIMER] = {
        "timers",
        "Timers",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_CALLBACK] = {
        "callback",
        "Callbacks",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_FRAME_RX] = {
        "rx",
        "Frame Rx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_FRAME_TX] = {
        "tx",
        "Frame Tx",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_ICLI] = {
        "icli",
        "CLI printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_API] = {
        "api",
        "Switch API printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_CFM] = {
        "cfm",
        "CFM callbacks/notifications",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_ACL] = {
        "acl",
        "ACL Updates",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [ERPS_TRACE_GRP_HIST] = {
        "hist",
        "History Updates",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

critd_t                              ERPS_crit;
static uint32_t                      ERPS_cap_port_cnt;
static bool                          ERPS_cap_cfm_has_shared_meg_level;
erps_map_t                           ERPS_map;
static vtss_appl_erps_capabilities_t ERPS_cap;
static vtss_appl_erps_conf_t         ERPS_default_conf;
static mesa_etype_t                  ERPS_s_custom_tpid;
static bool                          ERPS_started;
static mesa_mac_t                    ERPS_zero_mac, ERPS_chassis_mac;
static CapArray<erps_port_state_t, MESA_CAP_PORT_CNT> ERPS_port_state;
volatile bool                        ERPS_vlan_being_configured_by_ourselves;

/******************************************************************************/
// vtss_appl_erps_ring_port_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &stream, const vtss::Fmt &fmt, const vtss_appl_erps_ring_port_t ring_port)
{
    char   buf[256];
    size_t sz;

    sz = snprintf(buf, sizeof(buf), "%s", erps_util_ring_port_to_str(ring_port));
    return stream.write(buf, buf + MIN(sz, sizeof(buf)));
}

//*****************************************************************************/
// ERPS_sf_trigger_get()
//*****************************************************************************/
static vtss_appl_erps_sf_trigger_t ERPS_sf_trigger_get(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port)
{
    vtss_appl_erps_oper_warning_t w;

    if (erps_state->conf.ring_port_conf[ring_port].sf_trigger == VTSS_APPL_ERPS_SF_TRIGGER_LINK) {
        return VTSS_APPL_ERPS_SF_TRIGGER_LINK;
    }

    // Configured for using a MEP for SF triggering, but it could be that the
    // MEP is in such a shape that we cannot use it, so that we have to revert
    // to using link-state. This depends on the operational warnings that we
    // have by now.
    w = erps_state->status.oper_warning;
    if (w == VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_FOUND          ||
        w == VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_FOUND          ||
        w == VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_ADMIN_DISABLED     ||
        w == VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_ADMIN_DISABLED     ||
        w == VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_DOWN_MEP       ||
        w == VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_DOWN_MEP       ||
        w == VTSS_APPL_ERPS_OPER_WARNING_PORT0_AND_MEP_IFINDEX_DIFFER ||
        w == VTSS_APPL_ERPS_OPER_WARNING_PORT1_AND_MEP_IFINDEX_DIFFER) {
        // Those are the only operational warnings where we cannot use the MEP
        // for SF triggering and must fall back on link.
        return VTSS_APPL_ERPS_SF_TRIGGER_LINK;
    }

    return VTSS_APPL_ERPS_SF_TRIGGER_MEP;
}

//*****************************************************************************/
// ERPS_sf_get()
//*****************************************************************************/
static mesa_rc ERPS_sf_get(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, bool &sf, bool activating = false)
{
    vtss_appl_erps_sf_trigger_t             sf_trigger;
    erps_port_state_t                       *port_state;
    vtss_appl_cfm_mep_key_t                 mep_key;
    vtss_appl_cfm_mep_notification_status_t notif_status;
    mesa_rc                                 rc;

    if (ring_port == VTSS_APPL_ERPS_RING_PORT1 && erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
        // On an interconnected sub-ring, port1 is not used, but we still use
        // the SF state as described below.
        if (erps_state->conf.virtual_channel) {
            // If using virtual channel, we always pretend port1 is up, so that
            // we can report cFOP_TO correctly if we don't receive any R-APS
            // PDUs across the virtual channel on this interconnected sub-ring.
            sf = false;
        } else {
            // If not using virtual-channel, we never expect any R-APS PDUs on
            // port1, so in order not to report cFOP_TO, we pretend the link is
            // always down.
            sf = true;
        }

        return VTSS_RC_OK;
    }

    sf_trigger = ERPS_sf_trigger_get(erps_state, ring_port);

    switch (sf_trigger) {
    case VTSS_APPL_ERPS_SF_TRIGGER_LINK:
        port_state = erps_state->ring_port_state[ring_port].port_states[0];
        sf = !port_state->link;
        T_I("%u: %s %u: SF = %d", erps_state->inst, ring_port, port_state->port_no, sf);
        return VTSS_RC_OK;

    case VTSS_APPL_ERPS_SF_TRIGGER_MEP:
        mep_key = erps_state->conf.ring_port_conf[ring_port].mep;
        if ((rc = vtss_appl_cfm_mep_notification_status_get(mep_key, &notif_status)) != VTSS_RC_OK) {
            // We also get notifications whenever a MEP gets deleted, so if this
            // function returns something != VTSS_RC_OK, it's probably the
            // reason. In that case, we will also get a MEP configuration change
            // callback,  which will take proper action
            // (ERPS_cfm_conf_change_callback()).
            T_I("%u: %s-MEP %s: Unable to get status: %s", erps_state->inst, ring_port, mep_key, error_txt(rc));
            sf = true;
        } else {
            sf = !notif_status.mep_ok;
        }

        T_I("%u: %s-MEP %s: SF = %d", erps_state->inst, ring_port, mep_key, sf);

        // If we are activating, we pretend it went OK, although
        // vtss_appl_cfm_mep_notification_status_get() failed, because it will
        // fail if the MEP doesn't exist, and the function that activates will
        // not start the ERPS instance if we return an error code.
        return activating ? VTSS_RC_OK : rc;

    default:
        T_E("%u:%s: Unknown SF trigger (%d)", erps_state->inst, ring_port, sf_trigger);
        return VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
    }
}

//*****************************************************************************/
// ERPS_sf_update()
//*****************************************************************************/
static void ERPS_sf_update(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port)
{
    bool sf;

    (void)ERPS_sf_get(erps_state, ring_port, sf);
    T_IG(ERPS_TRACE_GRP_CFM, "%u: Setting %s-sf = %d", erps_state->inst, ring_port, sf);
    erps_base_sf_set(erps_state, ring_port, sf);
}

//*****************************************************************************/
// ERPS_cfm_change_notifications.
// Snoops on changes to the CFM MEP change notification structure to be able
// to react to changes in the MEP status (SF).
/******************************************************************************/
static struct erps_cfm_change_notifications_t : public vtss::notifications::EventHandler {
    vtss::notifications::Event              e;
    cfm_mep_notification_status_t::Observer o;

    erps_cfm_change_notifications_t() : EventHandler(&vtss::notifications::subject_main_thread), e(this)
    {
    }

    void init()
    {
        cfm_mep_notification_status.observer_new(&e);
    }

    void execute(vtss::notifications::Event *event)
    {
        vtss_appl_erps_ring_port_conf_t *ring_port_conf;
        vtss_appl_erps_ring_port_t      ring_port;
        erps_itr_t                      itr;
        erps_state_t                    *erps_state;

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
            T_DG(ERPS_TRACE_GRP_CFM, "%s: %s event", i->first, vtss::notifications::EventType::txt[vtss::notifications::EventType::E(i->second)].valueName);
        }

        ERPS_LOCK_SCOPE();

        if (!ERPS_started) {
            // Defer these change notifications until we have told the base about
            // all ERPS instances after boot.
            return;
        }

        // Loop through all ERPS instances
        for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
            erps_state = &itr->second;

            if (erps_state->status.oper_state != VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
                // Don't care about MEP changes on this instance, since it's not
                // active.
                continue;
            }

            for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
                if (!erps_state->using_ring_port_conf(ring_port)) {
                    // This ERPS instance is not using port1.
                    continue;
                }

                ring_port_conf = &erps_state->conf.ring_port_conf[ring_port];

                if (ring_port_conf->sf_trigger != VTSS_APPL_ERPS_SF_TRIGGER_MEP) {
                    // Not using MEP-triggered SF.
                    continue;
                }

                if (o.events.find(ring_port_conf->mep) == o.events.end()) {
                    // No events on this ring-port's MEP
                    continue;
                }

                // Event on this MEP-SF-triggered ring port.
                ERPS_sf_update(erps_state, ring_port);
            }
        }
    }
} ERPS_cfm_change_notifications;

/******************************************************************************/
// ERPS_base_activate()
/******************************************************************************/
static mesa_rc ERPS_base_activate(erps_state_t *erps_state)
{
    vtss_appl_erps_ring_port_t ring_port;
    bool                       port_sf[2];

    // Gotta find the initial values of signal fail for port0 and port1

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        VTSS_RC(ERPS_sf_get(erps_state, ring_port, port_sf[ring_port], true));
    }

    return erps_base_activate(erps_state, port_sf[VTSS_APPL_ERPS_RING_PORT0], port_sf[VTSS_APPL_ERPS_RING_PORT1]);
}

/******************************************************************************/
// ERPS_capabilities_set()
/******************************************************************************/
static void ERPS_capabilities_set(void)
{
    ERPS_cap.inst_cnt_max         =    64; // No MESA capability for VTSS_ERPIS
    ERPS_cap.wtr_secs_max         =   720; // 12 minutes.
    ERPS_cap.guard_time_msecs_max =  2000; // milliseconds in steps of 10 ms.
    ERPS_cap.hold_off_msecs_max   = 10000; // 10 seconds in steps of 100 ms.
}

//*****************************************************************************/
// ERPS_default_conf_set()
/******************************************************************************/
static void ERPS_default_conf_set(void)
{
    vtss_appl_erps_ring_port_t ring_port;

    vtss_clear(ERPS_default_conf);

    ERPS_default_conf.version                               = VTSS_APPL_ERPS_VERSION_V2;
    ERPS_default_conf.ring_type                             = VTSS_APPL_ERPS_RING_TYPE_MAJOR;
    ERPS_default_conf.virtual_channel                       = true; // G.8032, clause 10.1.14.
    ERPS_default_conf.ring_id                               = 1;
    ERPS_default_conf.level                                 = 7;
    ERPS_default_conf.control_vlan                          = 1;
    ERPS_default_conf.pcp                                   = 7;
    ERPS_default_conf.interconnect_conf.connected_ring_inst = 0;
    ERPS_default_conf.interconnect_conf.tc_propagate        = false;
    ERPS_default_conf.revertive                             = true;
    ERPS_default_conf.wtr_secs                              = 300;
    ERPS_default_conf.guard_time_msecs                      = 500;
    ERPS_default_conf.hold_off_msecs                        = 0;
    ERPS_default_conf.rpl_mode                              = VTSS_APPL_ERPS_RPL_MODE_NONE;
    ERPS_default_conf.rpl_port                              = VTSS_APPL_ERPS_RING_PORT0;
    ERPS_default_conf.admin_active                          = false;

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        ERPS_default_conf.ring_port_conf[ring_port].sf_trigger = VTSS_APPL_ERPS_SF_TRIGGER_LINK;

        if (vtss_ifindex_from_port(VTSS_ISID_LOCAL, VTSS_PORT_NO_START, &ERPS_default_conf.ring_port_conf[ring_port].ifindex) != VTSS_RC_OK) {
            T_E("Unable to convert <isid, port> = <%u, %u> to ifindex", VTSS_ISID_LOCAL, VTSS_PORT_NO_START);
        }
    }
}

/******************************************************************************/
// ERPS_tx_frame_update()
/******************************************************************************/
static void ERPS_tx_frame_update(erps_state_t *erps_state)
{
    if (!ERPS_started) {
        // Defer all erps_base calls until we are all set after boot
        return;
    }

    if (erps_state->status.oper_state == VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
        // The contents of a R-APS PDU has changed. Get it updated.
        erps_base_tx_frame_update(erps_state);
    }
}

/******************************************************************************/
// ERPS_vlan_custom_etype_change_callback()
/******************************************************************************/
static void ERPS_vlan_custom_etype_change_callback(mesa_etype_t tpid)
{
    erps_itr_t        itr;
    mesa_port_no_t    port_no;
    erps_port_state_t *port_state;
    erps_state_t      *erps_state;

    T_IG(ERPS_TRACE_GRP_CALLBACK, "S-Custom TPID: 0x%04x -> 0x%04x", ERPS_s_custom_tpid, tpid);

    ERPS_LOCK_SCOPE();

    if (ERPS_s_custom_tpid == tpid) {
        return;
    }

    ERPS_s_custom_tpid = tpid;

    for (port_no = 0; port_no < ERPS_cap_port_cnt; port_no++) {
        port_state = &ERPS_port_state[port_no];

        if (port_state->vlan_conf.port_type != VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM) {
            continue;
        }

        port_state->tpid = ERPS_s_custom_tpid;

        // Loop across all ERPS instances and see if they use this port state.
        for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
            erps_state = &itr->second;

            if (port_state == erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT0].port_states[0] ||
                port_state == erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[0] ||
                port_state == erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[1]) {
                ERPS_tx_frame_update(erps_state);
            }
        }
    }
}

/******************************************************************************/
// ERPS_control_vlan_gets_tagged_check()
/******************************************************************************/
static bool ERPS_control_vlan_gets_tagged_check(erps_state_t *erps_state)
{
    vtss_appl_vlan_port_detailed_conf_t *vlan_conf;
    vtss_appl_erps_ring_port_t          ring_port;
    bool                                warn;

    if (erps_state->status.oper_warning != VTSS_APPL_ERPS_OPER_WARNING_NONE                      &&
        erps_state->status.oper_warning != VTSS_APPL_ERPS_OPER_WARNING_PORT0_UNTAGS_CONTROL_VLAN &&
        erps_state->status.oper_warning != VTSS_APPL_ERPS_OPER_WARNING_PORT1_UNTAGS_CONTROL_VLAN) {
        // There is already another warning in effect, so don't update this one.
        // The return value means: Operational warning is in effect.
        return false;
    }

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        if (!erps_state->using_ring_port_conf(ring_port)) {
            continue;
        }

        vlan_conf = &erps_state->ring_port_state[ring_port].port_states[0]->vlan_conf;
        warn      = false;

        switch (vlan_conf->tx_tag_type) {
        case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
            // untagged_vid gets transmitted untagged.
            if (vlan_conf->untagged_vid == erps_state->conf.control_vlan) {
                // Not good.
                warn = true;
            }

            break;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
            // untagged_vid gets tagged, but pvid doesn't if it differs from
            // untagged_vid.
            if (vlan_conf->untagged_vid != vlan_conf->pvid && vlan_conf->pvid == erps_state->conf.control_vlan) {
                // Not good.
                warn = true;
            }

            break;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
            // All 4K VLANs egress tagged. Good.
            break;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
            // All 4K VLANs egress untagged. Not good.
            warn = true;
            break;

        default:
            T_E("Unsupported tx_tag_type (%d)", vlan_conf->tx_tag_type);
            break;
        }

        if (warn) {
            break;
        }
    }

    erps_state->status.oper_warning = !warn ? VTSS_APPL_ERPS_OPER_WARNING_NONE : ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_OPER_WARNING_PORT0_UNTAGS_CONTROL_VLAN : VTSS_APPL_ERPS_OPER_WARNING_PORT1_UNTAGS_CONTROL_VLAN;

    return !warn;
}

/******************************************************************************/
// ERPS_vlan_port_conf_change_callback()
/******************************************************************************/
static void ERPS_vlan_port_conf_change_callback(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *new_vlan_conf)
{
    erps_itr_t        itr;
    erps_port_state_t *port_state = &ERPS_port_state[port_no];
    erps_state_t      *erps_state;
    mesa_etype_t      new_tpid;
    bool              port_type_changed, pvid_changed, tpid_changed, tx_tagging_changed;

    if (ERPS_vlan_being_configured_by_ourselves) {
        // The VLAN module performs callbacks immediately whenever someone
        // changes the port configuration. Since we ourselves may change VLAN
        // port configuration in erps_base.cxx, we would end up in a deadlock
        // if we tried to take our own crit here, so better check if that's the
        // case and ignore the change in that case.
        return;
    }

    ERPS_LOCK_SCOPE();

    T_DG(ERPS_TRACE_GRP_CALLBACK, "port_no = %u: pvid: %u -> %u, uvid: %u -> %u, port_type: %s -> %s, tx_tag_type = %s -> %s",
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
        new_tpid = ERPS_s_custom_tpid;
        break;

    default:
        T_E("Unknown port_type (%d)", port_state->vlan_conf.port_type);
        new_tpid = port_state->tpid;
        break;
    }

    tpid_changed = port_state->tpid != new_tpid;
    port_state->tpid = new_tpid;

    if (!tpid_changed && !tx_tagging_changed) {
        // Tx frames only use TPID, so unless that one is changed, we don't
        // invoke ERPS_tx_frame_update().
        // Operational warnings may arise if tx_tagging_changed, so we need to
        // go through all ERPS instances to check that.
        // Rx frames also use PVID, but that will come all by itself upon the
        // next invocation of ERPS_frame_rx_callback().
        return;
    }

    // Loop across all ERPS instances and see if they need an update.
    for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
        erps_state = &itr->second;

        if (tpid_changed &&
            (port_state == erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT0].port_states[0] ||
             port_state == erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[0] ||
             port_state == erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[1])) {
            ERPS_tx_frame_update(erps_state);
        }

        if (tx_tagging_changed && erps_state->status.oper_state == VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
            (void)ERPS_control_vlan_gets_tagged_check(erps_state);
        }
    }
}

/******************************************************************************/
// ERPS_control_vlan_membership_check()
/******************************************************************************/
static bool ERPS_control_vlan_membership_check(erps_state_t *erps_state)
{
    vtss_appl_erps_ring_port_t ring_port;
    mesa_port_no_t             port_no;
    mesa_port_list_t           ports;
    mesa_vid_t                 vid;
    mesa_rc                    rc;

    if (erps_state->status.oper_warning != VTSS_APPL_ERPS_OPER_WARNING_NONE                             &&
        erps_state->status.oper_warning != VTSS_APPL_ERPS_OPER_WARNING_PORT0_NOT_MEMBER_OF_CONTROL_VLAN &&
        erps_state->status.oper_warning != VTSS_APPL_ERPS_OPER_WARNING_PORT1_NOT_MEMBER_OF_CONTROL_VLAN) {
        // There is already another warning in effect, so don't update this one.
        // The return value means: Operational warning is in effect.
        return false;
    }

    vid = erps_state->conf.control_vlan;

    T_N("%u: mesa_vlan_port_members_get(%u)", erps_state->inst, vid);
    if ((rc = mesa_vlan_port_members_get(nullptr, vid, &ports)) != VTSS_RC_OK) {
        T_E("%u: mesa_vlan_port_members_get(%u) failed: %s", erps_state->inst, vid, error_txt(rc));
        return true;
    }

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        if (!erps_state->using_ring_port_conf(ring_port)) {
            continue;
        }

        port_no = erps_state->ring_port_state[ring_port].port_states[0]->port_no;

        if (!ports[port_no]) {
            T_I("%u:%s. Not member of control VLAN (%u) on port_no %u", erps_state->inst, ring_port, vid, port_no);
            erps_state->status.oper_warning = ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_OPER_WARNING_PORT0_NOT_MEMBER_OF_CONTROL_VLAN : VTSS_APPL_ERPS_OPER_WARNING_PORT1_NOT_MEMBER_OF_CONTROL_VLAN;
            return false;
        }
    }

    erps_state->status.oper_warning = VTSS_APPL_ERPS_OPER_WARNING_NONE;

    return true;
}

/******************************************************************************/
// ERPS_vlan_membership_change_callback()
/******************************************************************************/
static void ERPS_vlan_membership_change_callback(vtss_isid_t isid, mesa_vid_t vid, vlan_membership_change_t *changes)
{
    erps_itr_t   itr;
    erps_state_t *erps_state;

    ERPS_LOCK_SCOPE();

    // Loop across all ERPS instances and see if they use this VID as control
    // VLAN.
    for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
        erps_state = &itr->second;

        if (erps_state->status.oper_state == VTSS_APPL_ERPS_OPER_STATE_ACTIVE && erps_state->conf.control_vlan == vid) {
            (void)ERPS_control_vlan_membership_check(erps_state);
        }
    }
}

/******************************************************************************/
// ERPS_link_state_change()
// In case we don't base the protection switching on MEPs, we can do it simply
// based on our own port states. This will, however, only work for link-to-link
// protections, not across a network.
/******************************************************************************/
static void ERPS_link_state_change(const char *func, mesa_port_no_t port_no, bool link)
{
    vtss_appl_erps_ring_port_t ring_port;
    erps_ring_port_state_t     *ring_port_state;
    erps_state_t               *erps_state;
    erps_port_state_t          *port_state;
    erps_itr_t                 itr;

    if (port_no >= ERPS_cap_port_cnt) {
        T_EG(ERPS_TRACE_GRP_CALLBACK, "Invalid port_no (%u). Max = %u", port_no, ERPS_cap_port_cnt);
        return;
    }

    port_state = &ERPS_port_state[port_no];

    T_DG(ERPS_TRACE_GRP_CALLBACK, "%s: port_no = %u: Link state: %d -> %d", func, port_no, port_state->link, link);

    if (link == port_state->link) {
        return;
    }

    port_state->link = link;

    if (!ERPS_started) {
        // Defer all erps_base updates until we are all set after a boot.
        return;
    }

    for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
        erps_state = &itr->second;

        if (erps_state->status.oper_state != VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
            // This ERPS instance is not active, so it doesn't care.
            continue;
        }

        for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
            ring_port_state = &erps_state->ring_port_state[ring_port];

            if (!erps_state->using_ring_port_conf(ring_port)) {
                // This is port1 on an interconnected sub-ring. Don't change SF.
                continue;
            }

            if (ring_port_state->port_states[0] != port_state) {
                // Not for this ring port.
                continue;
            }

            if (!link || ERPS_sf_trigger_get(erps_state, ring_port) == VTSS_APPL_ERPS_SF_TRIGGER_LINK) {
                // This ring port is either using link as SF trigger or the link
                // went down. That is, we cannot use link-up when SF-trigger is
                // MEP. In that case we need to wait for the MEP to signal
                // non-SF.
                erps_base_sf_set(erps_state, ring_port, !link);
            }
        }
    }
}

/******************************************************************************/
// ERPS_port_link_state_change_callback()
/******************************************************************************/
static void ERPS_port_link_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    ERPS_LOCK_SCOPE();
    ERPS_link_state_change(__FUNCTION__, port_no, status->link);
}

/******************************************************************************/
// ERPS_port_shutdown_callback()
/******************************************************************************/
static void ERPS_port_shutdown_callback(mesa_port_no_t port_no)
{
    ERPS_LOCK_SCOPE();
    ERPS_link_state_change(__FUNCTION__, port_no, false);
}

/******************************************************************************/
// ERPS_port_interrupt_callback()
/******************************************************************************/
static void ERPS_port_interrupt_callback(meba_event_t source_id, u32 port_no)
{
    mesa_rc rc;

    ERPS_LOCK_SCOPE();

    // We need to re-register for link-state-changes
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_ERPS, ERPS_port_interrupt_callback, source_id, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_EG(ERPS_TRACE_GRP_CALLBACK, "vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    ERPS_link_state_change(__FUNCTION__, port_no, false);
}

/******************************************************************************/
// ERPS_frame_rx_callback()
/******************************************************************************/
static BOOL ERPS_frame_rx_callback(void *contxt, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    vtss_appl_erps_ring_port_t ring_port;
    mesa_etype_t               etype;
    erps_itr_t                 itr;
    erps_state_t               *erps_state;
    erps_port_state_t          *port_state;
    int                        p;
    uint8_t                    opcode, ring_id;
    bool                       found;

    // The Ring ID must also match, or we discard the frame.
    ring_id = frm[5];

    // As long as we can't receive a frame behind two tags, the frame is always
    // normalized, so that the CFM Ethertype comes at frm[12] and frm[13],
    // because the packet module strips the outer tag.
    etype   = frm[12] << 8 | frm[13];
    opcode  = frm[15];

    if (opcode != ERPS_OPCODE_RAPS) {
        // Not for us.
        return FALSE; // Not consumed
    }

    T_DG(ERPS_TRACE_GRP_FRAME_RX, "Rx frame on port_no = %u of length %u excl. FCS. EtherType = 0x%04x, opcode = %u, iflow_id = %u, hints = 0x%x, class-tag: tpid = 0x%04x, vid = %u, pcp = %u, dei = %u, stripped_tag: tpid = 0x%04x, vid = %u, pcp = %u, dei = %u",
         rx_info->port_no,
         rx_info->length,
         etype,
         opcode,
         rx_info->iflow_id,
         rx_info->hints,
         rx_info->tag.tpid,
         rx_info->tag.vid,
         rx_info->tag.pcp,
         rx_info->tag.dei,
         rx_info->stripped_tag.tpid,
         rx_info->stripped_tag.vid,
         rx_info->stripped_tag.pcp,
         rx_info->stripped_tag.dei);

    if (etype != CFM_ETYPE) {
        // Why did we receive this frame? Our Packet Rx Filter tells the packet
        // module only to send us frames with CFM EtherType.
        T_EG(ERPS_TRACE_GRP_FRAME_RX, "Rx frame on port_no %u with etype = 0x%04x, which isn't the CFM EtherType", rx_info->port_no, etype);

        // Not consumed
        return FALSE;
    }

    // Match on ingress port and classified VLAN.
    if (!rx_info->stripped_tag.tpid) {
        // There must be a stripped tag in the frame, but there isn't.
        T_DG(ERPS_TRACE_GRP_FRAME_RX, "Skipping, because there's no stripped tag in frame");
        return FALSE;
    }

    ERPS_LOCK_SCOPE();

    if (!ERPS_started) {
        // Don't forward frames to the base until we are all set after a boot
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "Skipping Rx frame, because we're not ready yet");
        return FALSE;
    }

    // Loop through all ERPS states and find - amongst the active ERPSs - the
    // one that matches this frame.
    for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
        erps_state = &itr->second;

        if (erps_state->status.oper_state != VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
            T_DG(ERPS_TRACE_GRP_FRAME_RX, "%u: Not active", erps_state->inst);
            continue;
        }

        // All R-APS PDUs are tagged, whether they match PVID or not.
        if (erps_state->conf.control_vlan != rx_info->tag.vid) {
            // The ERPS instance's expected classified VID must match the
            // frame's classified VID, but it doesn't, so not for us.
            T_DG(ERPS_TRACE_GRP_FRAME_RX, "%u: VLAN in frame (%u) doesn't match ours (%u)", erps_state->inst, rx_info->tag.vid, erps_state->conf.control_vlan);
            continue;
        }

        if (erps_state->conf.ring_id != ring_id) {
            // The ERPS instance's Ring ID must also match. It is possible to
            // have two instances running on the same port with same control
            // VLAN, but different Ring IDs.
            T_DG(ERPS_TRACE_GRP_FRAME_RX, "%u: Ring ID in frame (%u) doesn't match ours (%u)", erps_state->inst, ring_id, erps_state->conf.ring_id);
            continue;
        }

        // We may receive R-APS PDUs on both port0 and port1 and on
        // interconnected sub-rings with virtual channel - also on connected
        // ring's ring ports.
        found = false;
        for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
            for (p = 0; p < 2; p++) {
                port_state = erps_state->ring_port_state[ring_port].port_states[p];
                if (!port_state) {
                    continue;
                }

                if (port_state->port_no == rx_info->port_no) {
                    found = true;
                    break;
                }
            }

            if (found) {
                break;
            }
        }

        if (ring_port > VTSS_APPL_ERPS_RING_PORT1) {
            // No match in above ring-port loop
            T_DG(ERPS_TRACE_GRP_FRAME_RX, "%u: Ingress port_no (%u) doesn't match any of our ring ports", erps_state->inst, rx_info->port_no);
            continue;
        }

        T_DG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s handles the frame", itr->first, ring_port);
        T_NG_HEX(ERPS_TRACE_GRP_FRAME_RX, frm, rx_info->length);
        erps_base_rx_frame(erps_state, ring_port, frm, rx_info);

        // Consumed
        return TRUE;
    }

    // Not consumed
    return FALSE;
}

/******************************************************************************/
// ERPS_port_state_init()
/******************************************************************************/
static void ERPS_port_state_init(void)
{
    vtss_appl_vlan_port_conf_t vlan_conf;
    erps_port_state_t          *port_state;
    mesa_port_no_t             port_no;
    mesa_rc                    rc;

    {
        ERPS_LOCK_SCOPE();

        ERPS_s_custom_tpid = VTSS_APPL_VLAN_CUSTOM_S_TAG_DEFAULT;

        (void)conf_mgmt_mac_addr_get(ERPS_chassis_mac.addr, 0);

        for (port_no = 0; port_no < ERPS_cap_port_cnt; port_no++) {
            // Get current VLAN configuration as configured in H/W. We could have
            // used vtss_appl_vlan_interface_conf_get(), but that would require us
            // to provide an ifindex rather than a port number, so we use the
            // internal API instead.
            // The function returns what we need in vlan_conf.hybrid.
            if ((rc = vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, port_no, &vlan_conf, VTSS_APPL_VLAN_USER_ALL, true)) != VTSS_RC_OK) {
                T_E("vlan_mgmt_port_conf_get(%u) failed: %s", port_no, error_txt(rc));
            }

            port_state            = &ERPS_port_state[port_no];
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
    vlan_s_custom_etype_change_register(VTSS_MODULE_ID_ERPS, ERPS_vlan_custom_etype_change_callback);

    // Subscribe to VLAN port changes in the VLAN module
    vlan_port_conf_change_register(VTSS_MODULE_ID_ERPS, ERPS_vlan_port_conf_change_callback, TRUE);

    // Subscribe to VLAN port membership changes in the VLAN module
    vlan_membership_change_register(VTSS_MODULE_ID_ERPS, ERPS_vlan_membership_change_callback);

    // Subscribe to link changes in the Port module
    if ((rc = port_change_register(VTSS_MODULE_ID_ERPS, ERPS_port_link_state_change_callback)) != VTSS_RC_OK) {
        T_E("port_change_register() failed: %s", error_txt(rc));
    }

    // Subscribe to 'shutdown' commands
    if ((rc = port_shutdown_register(VTSS_MODULE_ID_ERPS, ERPS_port_shutdown_callback)) != VTSS_RC_OK) {
        T_E("port_shutdown_register() failed: %s", error_txt(rc));
    }

    // Subscribe to FLNK and LOS interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_ERPS, ERPS_port_interrupt_callback, MEBA_EVENT_FLNK, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    // Subscribe to LoS interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_ERPS, ERPS_port_interrupt_callback, MEBA_EVENT_LOS, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }
}

/******************************************************************************/
// ERPS_frame_rx_register()
/******************************************************************************/
static void ERPS_frame_rx_register(void)
{
    void               *filter_id;
    packet_rx_filter_t filter;
    mesa_rc            rc;

    packet_rx_filter_init(&filter);

    // We only need to install one packet filter, matching on the CFM ethertype.
    // The reason is that the packet module strips a possible outer tag before
    // it starts matching. This means that as long as we only support CFM behind
    // one tag, this is good enough. If we some day start supporting CFM behind
    // two (or more tags), we must also install filters for the inner tag and
    // let the CFM_frame_rx_callback() function look at the EtherType behind
    // that one tag.
    filter.modid = VTSS_MODULE_ID_ERPS;
    filter.match = PACKET_RX_FILTER_MATCH_ETYPE;
    filter.cb    = ERPS_frame_rx_callback;
    filter.prio  = PACKET_RX_FILTER_PRIO_HIGH; // Get the frames before CFM
    filter.etype = CFM_ETYPE;
    if ((rc = packet_rx_filter_register(&filter, &filter_id)) != VTSS_RC_OK) {
        T_E("packet_rx_filter_register() failed: %s", error_txt(rc));
    }
}

/******************************************************************************/
// ERPS_ptr_check()
/******************************************************************************/
static mesa_rc ERPS_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_ERPS_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// ERPS_inst_check()
// Range [1-ERPS_cap.inst_cnt_max]
/******************************************************************************/
static mesa_rc ERPS_inst_check(uint32_t inst)
{
    if (inst < 1 || inst > ERPS_cap.inst_cnt_max) {
        return VTSS_APPL_ERPS_RC_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// ERPS_ifindex_to_port()
/******************************************************************************/
static mesa_rc ERPS_ifindex_to_port(vtss_ifindex_t ifindex, vtss_appl_erps_ring_port_t ring_port, mesa_port_no_t &port_no)
{
    vtss_ifindex_elm_t ife;

    // Check that we can decompose the ifindex and that it's a port.
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        port_no = VTSS_PORT_NO_NONE;
        return ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_RC_INVALID_PORT0_IFINDEX : VTSS_APPL_ERPS_RC_INVALID_PORT1_IFINDEX;
    }

    port_no = ife.ordinal;
    return VTSS_RC_OK;
}

/******************************************************************************/
// ERPS_mep_info_get()
/******************************************************************************/
static bool ERPS_mep_info_get(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port)
{
    vtss_appl_cfm_md_conf_t  md_conf;
    vtss_appl_cfm_ma_conf_t  ma_conf;
    vtss_appl_cfm_mep_conf_t mep_conf;
    vtss_ifindex_t           ifindex  = erps_state->conf.ring_port_conf[ring_port].ifindex;
    vtss_appl_cfm_mep_key_t  &mep_key = erps_state->conf.ring_port_conf[ring_port].mep;

    // MEP found?
    if (vtss_appl_cfm_md_conf_get( mep_key, &md_conf)  != VTSS_RC_OK ||
        vtss_appl_cfm_ma_conf_get( mep_key, &ma_conf)  != VTSS_RC_OK ||
        vtss_appl_cfm_mep_conf_get(mep_key, &mep_conf) != VTSS_RC_OK) {
        erps_state->status.oper_warning = ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_FOUND : VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_FOUND;
        return false;
    }

    // MEP administratively enabled?
    if (!mep_conf.admin_active) {
        erps_state->status.oper_warning = ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_ADMIN_DISABLED : VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_ADMIN_DISABLED;
        return false;
    }

    // MEP's direction is down?
    if (mep_conf.direction != VTSS_APPL_CFM_DIRECTION_DOWN) {
        erps_state->status.oper_warning = ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_DOWN_MEP : VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_DOWN_MEP;
        return false;
    }

    // MEP's ifindex must be the same as this ring port's ifindex
    if (mep_conf.ifindex != ifindex) {
        erps_state->status.oper_warning = ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_OPER_WARNING_PORT0_AND_MEP_IFINDEX_DIFFER : VTSS_APPL_ERPS_OPER_WARNING_PORT1_AND_MEP_IFINDEX_DIFFER;
        return false;
    }

    // This MEP did not give rise to any operational warnings.
    return true;
}

/******************************************************************************/
// ERPS_mep_doesnt_shadow_mip()
/******************************************************************************/
static bool ERPS_mep_doesnt_shadow_mip(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port)
{
    vtss_appl_cfm_mep_key_t  mep_key, next_mep_key;
    vtss_appl_cfm_md_conf_t  md_conf;
    vtss_appl_cfm_ma_conf_t  ma_conf;
    vtss_appl_cfm_mep_conf_t mep_conf;
    vtss_ifindex_t           ifindex = erps_state->conf.ring_port_conf[ring_port].ifindex;
    uint8_t                  level   = erps_state->conf.level;
    mesa_vid_t               mep_vlan;
    mesa_rc                  rc;

    mep_key.md = "";

    while (vtss_appl_cfm_md_itr(&mep_key, &next_mep_key) == VTSS_RC_OK) {
        mep_key    = next_mep_key;
        mep_key.ma = "";

        if ((rc = vtss_appl_cfm_md_conf_get(mep_key, &md_conf)) != VTSS_RC_OK) {
            // Don't use T_E(), because it could be that the MD disappeared
            // between call to md_itr() and md_conf_get().
            T_I("%u:%s: vtss_appl_cfm_md_conf_get(%s) failed: %s", erps_state->inst, ring_port, mep_key, error_txt(rc));
            continue;
        }

        if (md_conf.level < level) {
            // The MEPs in this domain doesn't bother us.
            continue;
        }

        T_D("%u:%s: %s: MD level %u >= %u. Gotta check MAs and MEPs.", erps_state->inst, ring_port, mep_key, md_conf.level, level);

        while (vtss_appl_cfm_ma_itr(&mep_key, &next_mep_key, true) == VTSS_RC_OK) {
            mep_key       = next_mep_key;
            mep_key.mepid = 0;

            if ((rc = vtss_appl_cfm_ma_conf_get(mep_key, &ma_conf)) != VTSS_RC_OK) {
                // Don't use T_E(), because it could be that the MA disappeared
                // between call to ma_itr() and ma_conf_get().
                T_I("%u: vtss_appl_cfm_ma_conf_get(%s) failed: %s", erps_state->inst, mep_key, error_txt(rc));
                continue;
            }

            while (vtss_appl_cfm_mep_itr(&mep_key, &next_mep_key, true) == VTSS_RC_OK) {
                mep_key = next_mep_key;

                T_D("%u:%s: Investigating shadowing from MEP %s", erps_state->inst, ring_port, mep_key);

                if ((rc = vtss_appl_cfm_mep_conf_get(mep_key, &mep_conf)) != VTSS_RC_OK) {
                    // Don't use T_E(), because it could be that the MEP
                    // disappeared between call to mep_itr() and mep_conf_get().
                    T_I("%u: vtss_appl_cfm_mep_conf_get(%s) failed: %s", erps_state->inst, mep_key, error_txt(rc));
                    continue;
                }

                if (!mep_conf.admin_active) {
                    continue;
                }

                if (mep_conf.ifindex != ifindex) {
                    continue;
                }

                if (ERPS_cap_cfm_has_shared_meg_level && ma_conf.vlan == 0) {
                    // Any Port MEP at or above this MIP's level will shadow
                    // the MIP, because VOPv1 (Serval-1 and Ocelot) runs shared
                    // MEG level.
                    T_I("%u:%s: Port MEP %s shadows R-APS PDUs", erps_state->inst, ring_port, mep_key);
                    erps_state->status.oper_warning = ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_OPER_WARNING_PORT_MEP_SHADOWS_PORT0_MIP : VTSS_APPL_ERPS_OPER_WARNING_PORT_MEP_SHADOWS_PORT1_MIP;
                    return false;
                }

                mep_vlan = mep_conf.vlan != 0 ? mep_conf.vlan : ma_conf.vlan;

                if (mep_vlan != erps_state->conf.control_vlan) {
                    continue;
                }

                T_I("%u:%s: MEP %s shadows R-APS PDUs", erps_state->inst, ring_port, mep_key);
                erps_state->status.oper_warning = ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_OPER_WARNING_MEP_SHADOWS_PORT0_MIP : VTSS_APPL_ERPS_OPER_WARNING_MEP_SHADOWS_PORT1_MIP;
                return false;
            }
        }
    }

    // This ring port is not shadowed by any MEPs.
    return true;
}

/******************************************************************************/
// ERPS_ring_port_oper_state_update()
//
// This function updates the used SMAC and the port_states[] array for index 0
// only. ports_states[1] is updated elsewhere (and is only used for
// interconnected sub-rings with virtual channel on ring port1).
//
// In principle this function could give rise to setting oper_state non-active,
// but no conditions can currently do that.
/******************************************************************************/
static bool ERPS_ring_port_oper_state_update(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port)
{
    mesa_port_no_t                  port_no;
    vtss_appl_erps_ring_port_conf_t *ring_port_conf  = &erps_state->conf.ring_port_conf[ring_port];
    erps_ring_port_state_t          *ring_port_state = &erps_state->ring_port_state[ring_port];

    // The configuration's ifindex has already been checked by
    // vtss_appl_erps_conf_set(), so no need to check return value.
    (void)ERPS_ifindex_to_port(ring_port_conf->ifindex, ring_port, port_no);

    // Only update index 0, because that's used for all ring ports except for
    // port1 on interconnected sub-rings, which are filtered out by the caller
    // of us.
    ring_port_state->port_states[0] = &ERPS_port_state[port_no];

    // Update our own state with SMAC.
    // operator== on mesa_mac_t is from ip_utils.hxx.
    if (ring_port_conf->smac == ERPS_zero_mac) {
        // SMAC is not overridden. Use Port MAC.
        ring_port_state->smac = ring_port_state->port_states[0]->smac;
    } else {
        // SMAC is overridden by configuration. Use that one.
        ring_port_state->smac = ring_port_conf->smac;
    }

    return true;
}

/******************************************************************************/
// ERPS_ring_port_oper_warning_update()
/******************************************************************************/
static bool ERPS_ring_port_oper_warning_update(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port)
{
    vtss_appl_erps_ring_port_conf_t *ring_port_conf = &erps_state->conf.ring_port_conf[ring_port];

    // If we are using a MEP for SF, that MEP must have certain properties
    if (ring_port_conf->sf_trigger == VTSS_APPL_ERPS_SF_TRIGGER_MEP) {
        // Using MEP for SF. Check to see if MEP is valid and active. This can
        // only give rise to oper_warning, not an inactive oper_state.
        if (!ERPS_mep_info_get(erps_state, ring_port)) {
            return false;
        }
    }

    // If a MEP shadows reception of R-APS PDUs, it gives rise to an operational
    // warning, not an operational error, so the ERPS instance will indeed be
    // created.
    if (!ERPS_mep_doesnt_shadow_mip(erps_state, ring_port)) {
        return false;
    }

    return true;
}

/******************************************************************************/
// ERPS_oper_state_update_single()
// There can happen changes to the MEPs' configuration that affect the ERPS
// instance's ability to work. This function is invoked when MEP configuration
// or ERPS configuration changes to figure out whether a given ERPS instance can
// now be operationally up.
/******************************************************************************/
static void ERPS_oper_state_update_single(erps_state_t *erps_state)
{
    vtss_appl_erps_ring_port_t ring_port;

    T_I("%u, Enter", erps_state->inst);

    // Assume it's operational active...
    erps_state->status.oper_state = VTSS_APPL_ERPS_OPER_STATE_ACTIVE;

    // ...and assume no warnings.
    erps_state->status.oper_warning = VTSS_APPL_ERPS_OPER_WARNING_NONE;

    // This ERPS instance administratively enabled?
    if (!erps_state->conf.admin_active) {
        erps_state->status.oper_state = VTSS_APPL_ERPS_OPER_STATE_ADMIN_DISABLED;
        return;
    }

    // Update node-id
    if (erps_state->conf.node_id == ERPS_zero_mac) {
        // Use switch's own.
        erps_state->status.tx_raps_info.node_id = ERPS_chassis_mac;
    } else {
        erps_state->status.tx_raps_info.node_id = erps_state->conf.node_id;
    }

    // Update transmitted version number
    erps_state->status.tx_raps_info.version = erps_state->conf.version;

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        erps_state->ring_port_state[ring_port].port_states[0] = nullptr;
        erps_state->ring_port_state[ring_port].port_states[1] = nullptr;
        erps_state->ring_port_state[ring_port].smac           = ERPS_zero_mac;

        if (!erps_state->using_ring_port_conf(ring_port)) {
            // This is port1 on an interconnected sub-ring. Don't check now, but
            // defer it to later if it's using a virtual channel.
            continue;
        }

        // Update oper_state and port_state.
        if (!ERPS_ring_port_oper_state_update(erps_state, ring_port)) {
            return;
        }
    }

    // Time to check for operational warnings

    // Check that ring ports are members of the control VLAN
    if (!ERPS_control_vlan_membership_check(erps_state)) {
        // Operational warning set.
        return;
    }

    // Check that the control VLAN gets tagged on egress when H/W forwarding
    // from one ring port to another.
    if (!ERPS_control_vlan_gets_tagged_check(erps_state)) {
        // Operational warning set
        return;
    }

    // Check for per-ring-port warnings.
    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        if (!erps_state->using_ring_port_conf(ring_port)) {
            // This is port1 on an interconnected sub-ring. Don't check now, but
            // defer it to later if it's using a virtual channel.
            continue;
        }

        // Only run for this ring port if there's no warnings so far.
        if (!ERPS_ring_port_oper_warning_update(erps_state, ring_port)) {
            // Operational warning set. Nothing more to do.
            return;
        }
    }
}

/******************************************************************************/
// ERPS_oper_state_update_all()
// There can happen changes to the MEPs' configuration that affect the ERPS
// instance's ability to work. This function is invoked when MEP configuration
// or ERPS configuration changes to figure out whether a given ERPS instance can
// now be operational up.
//
// The nature of this function will favor those ERPS instances with higher
// instance numbers if there are ERPS cross-checks that fail.
/******************************************************************************/
static void ERPS_oper_state_update_all(void)
{
    vtss_appl_erps_ring_port_t ring_port;
    erps_state_t               *erps_state;
    erps_itr_t                 itr1, itr2;

    if (!ERPS_started) {
        // Defer all erps_base updates until after the CFM module is up and
        // running after a boot.
        return;
    }

    // First update the operational state of all ERPS instances based on each
    // ERPS instance's own configuration.
    for (itr1 = ERPS_map.begin(); itr1 != ERPS_map.end(); ++itr1) {
        itr1->second.old_oper_state   = itr1->second.status.oper_state;
        itr1->second.old_oper_warning = itr1->second.status.oper_warning;
        memcpy(itr1->second.old_ring_port_state, itr1->second.ring_port_state, sizeof(itr1->second.old_ring_port_state));
        ERPS_oper_state_update_single(&itr1->second);
    }

    // Then update the operational state/warnings of the ERPS instances based on
    // cross-checks amongst ERPS instances.
    // Also update the port_states[] of port1 for interconnected sub-rings with
    // virtual channel, now that we have updated the connected ring's state.
    for (itr1 = ERPS_map.begin(); itr1 != ERPS_map.end(); ++itr1) {
        erps_state = &itr1->second;

        if (erps_state->status.oper_state != VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
            // This instance was caught as inactive in the first iteration
            // above. Nothing else to do.
            continue;
        }

        if (erps_state->status.oper_warning != VTSS_APPL_ERPS_OPER_WARNING_NONE) {
            // We can only show one warning at a time, so nothing more to do.
            continue;
        }

        if (erps_state->conf.ring_type != VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
            continue;
        }

        // This is an interconnected sub-ring. Check the referenced connected
        // ring.
        if ((itr2 = ERPS_map.find(erps_state->conf.interconnect_conf.connected_ring_inst)) == ERPS_map.end()) {
            // Connected ring instance doesn't exist, so nowhere to send FDB
            // flushes.
            erps_state->status.oper_warning = VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_DOESNT_EXIST;
            continue;
        }

        if (itr2->second.conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
            // Referenced connected ring is an interconnected sub-ring.
            erps_state->status.oper_warning = VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_IS_AN_INTERCONNECTED_SUB_RING;
            continue;
        }

        if (itr2->second.status.oper_state != VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
            // Connected ring is not active.
            erps_state->status.oper_warning = VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_IS_NOT_OPERATIVE;
            continue;
        }

        if (erps_state->conf.ring_port_conf[0].ifindex == itr2->second.conf.ring_port_conf[0].ifindex ||
            erps_state->conf.ring_port_conf[0].ifindex == itr2->second.conf.ring_port_conf[1].ifindex) {
            // The connected ring uses the same port as this interconnected
            // sub-ring
            erps_state->status.oper_warning = VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_INTERFACE_CONFLICT;
            continue;
        }

        if (erps_state->conf.virtual_channel) {
            if (!VTSS_BF_GET(itr2->second.conf.protected_vlans, erps_state->conf.control_vlan)) {
                // If we use the connected ring as a virtual channel, that ring
                // should carry our control VLAN in its protected VLANs, but it
                // doesn't.
                erps_state->status.oper_warning = VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_DOESNT_PROTECT_CONTROL_VLAN;
                continue;
            }

            // Let this interconnected sub-ring w/ virtual channel utilize the
            // connected ring's ring-links for R-APS PDUs.
            erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[0] = itr2->second.ring_port_state[VTSS_APPL_ERPS_RING_PORT0].port_states[0];
            erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[1] = itr2->second.ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[0];

            // If port1's SMAC is configured for this interconnected sub-ring
            // with virtual channel, we use that one. Otherwise we resort to the
            // chassis (CPU's) MAC address, because we don't have a resident
            // port for port1.
            if (erps_state->conf.ring_port_conf[VTSS_APPL_ERPS_RING_PORT1].smac == ERPS_zero_mac) {
                erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].smac = ERPS_chassis_mac;
            } else {
                erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].smac = erps_state->conf.ring_port_conf[VTSS_APPL_ERPS_RING_PORT1].smac;
            }
        }
    }

    // Loop a third time, where we deactivate instances that are operational
    // down and activate instances that are operational up.
    for (itr1 = ERPS_map.begin(); itr1 != ERPS_map.end(); ++itr1) {
        erps_state = &itr1->second;

        // Check if the operational state has changed.
        if (erps_state->old_oper_state != erps_state->status.oper_state) {
            // Operational state has changed.
            if (erps_state->status.oper_state == VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
                // Inactive-to-active
                if (ERPS_base_activate(erps_state) != VTSS_RC_OK) {
                    erps_state->status.oper_state = VTSS_APPL_ERPS_OPER_STATE_INTERNAL_ERROR;
                }
            } else if (erps_state->old_oper_state == VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
                // Active-to-inactive
                (void)erps_base_deactivate(erps_state);
            }
        } else if (erps_state->status.oper_state == VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
            // Active-to_active.

            // Check if any of the parameters affecting ERPS port configuration
            // has changed. If so, we need to deactivate and re-activate it.
            if (erps_state->old_conf.ring_port_conf[0].ifindex             != erps_state->conf.ring_port_conf[0].ifindex             ||
                erps_state->old_conf.ring_port_conf[1].ifindex             != erps_state->conf.ring_port_conf[1].ifindex             ||
                erps_state->old_conf.revertive                             != erps_state->conf.revertive                             ||
                erps_state->old_conf.version                               != erps_state->conf.version                               ||
                erps_state->old_conf.ring_type                             != erps_state->conf.ring_type                             ||
                erps_state->old_conf.control_vlan                          != erps_state->conf.control_vlan                          ||
                erps_state->old_conf.interconnect_conf.connected_ring_inst != erps_state->conf.interconnect_conf.connected_ring_inst ||
                erps_state->old_conf.virtual_channel                       != erps_state->conf.virtual_channel                       ||
                erps_state->old_conf.rpl_mode                              != erps_state->conf.rpl_mode                              ||
                erps_state->old_conf.rpl_port                              != erps_state->conf.rpl_port) {
                // The configuration has changed. First deactivate the old, then
                // activate the new.
                (void)erps_base_deactivate(erps_state);
                if (ERPS_base_activate(erps_state) != VTSS_RC_OK) {
                    erps_state->status.oper_state = VTSS_APPL_ERPS_OPER_STATE_INTERNAL_ERROR;
                }
            } else {
                // The following function checks if the MEG level or Ring ID
                // has changed and in that case updates the ACE matching rule.
                if (erps_base_matching_update(erps_state) != VTSS_RC_OK) {
                    erps_state->status.oper_state = VTSS_APPL_ERPS_OPER_STATE_INTERNAL_ERROR;
                    (void)erps_base_deactivate(erps_state);
                    continue;
                }

                // If any of the parameters affecting R-APS PDU contents has
                // changed, we need to get base to update the R-APS PDUs it is
                // sending.
                if (erps_state->old_ring_port_state[VTSS_APPL_ERPS_RING_PORT0].smac != erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT0].smac ||
                    erps_state->old_ring_port_state[VTSS_APPL_ERPS_RING_PORT1].smac != erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].smac ||
                    erps_state->old_conf.level   != erps_state->conf.level                                                                         ||
                    erps_state->old_conf.ring_id != erps_state->conf.ring_id                                                                       ||
                    erps_state->old_conf.node_id != erps_state->conf.node_id                                                                       ||
                    erps_state->old_conf.pcp     != erps_state->conf.pcp) {
                    ERPS_tx_frame_update(erps_state);
                }

                // If protected VLANs have changed, go and update them
                if (!vlan_mgmt_bitmasks_identical(erps_state->old_conf.protected_vlans, erps_state->conf.protected_vlans)) {
                    if (erps_base_protected_vlans_update(erps_state) != VTSS_RC_OK) {
                        erps_state->status.oper_state = VTSS_APPL_ERPS_OPER_STATE_INTERNAL_ERROR;
                        (void)erps_base_deactivate(erps_state);
                        continue;
                    }
                }

                if (erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB && erps_state->conf.virtual_channel) {
                    if (erps_state->old_ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[0] != erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[0] ||
                        erps_state->old_ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[1] != erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[1]) {
                        // This is an interconnected sub-ring with a virtual
                        // channel, and its referenced connected ring's ring
                        // port configuration has changed. We gotta update our
                        // forwarding rules.
                        if (erps_base_connected_ring_ports_update(erps_state) != VTSS_RC_OK) {
                            erps_state->status.oper_state = VTSS_APPL_ERPS_OPER_STATE_INTERNAL_ERROR;
                            (void)erps_base_deactivate(erps_state);
                            continue;
                        }
                    }
                }

                // If we are using a different signal fail source now, go and
                // update SF.
                for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
                    if (!erps_state->using_ring_port_conf(ring_port)) {
                        continue;
                    }

                    // Signal fail may come from either the MEP or the link
                    // status.
                    ERPS_sf_update(erps_state, ring_port);
                }
            }
        }

        // Now we are in sync.
        erps_state->old_conf = erps_state->conf;
    }
}

/******************************************************************************/
// ERPS_cfm_conf_change_callback()
/******************************************************************************/
static void ERPS_cfm_conf_change_callback(void)
{
    T_IG(ERPS_TRACE_GRP_CFM, "CFM conf changed");
    // Some configuration has changed in the CFM module. Check if this has any
    // influence on the ERPS instances.
    ERPS_LOCK_SCOPE();
    ERPS_oper_state_update_all();
}

/******************************************************************************/
// ERPS_conf_update()
/******************************************************************************/
static mesa_rc ERPS_conf_update(erps_itr_t itr, const vtss_appl_erps_conf_t *new_conf)
{
    // We need to save a copy of the old configuration in case of deactivation
    // and other changes that may cause base updates.
    itr->second.old_conf = itr->second.conf;

    // The state's #inst member is only used for tracing.
    itr->second.inst = itr->first;

    // Update our internal configuration
    itr->second.conf = *new_conf;

    // With this new configuration, it could be that other ERPS instances become
    // inactive or active.
    ERPS_oper_state_update_all();

    return VTSS_RC_OK;
}

//*****************************************************************************/
// ERPS_ring_port_conf_chk()
/******************************************************************************/
static mesa_rc ERPS_ring_port_conf_chk(const vtss_appl_erps_conf_t *conf, vtss_appl_erps_ring_port_t ring_port)
{
    const vtss_appl_erps_ring_port_conf_t *ring_port_conf;
    mesa_port_no_t                        port_no;
    bool                                  empty, check_smac_only = false;

    if (!erps_util_using_ring_port_conf(conf, ring_port)) {
        if (conf->virtual_channel) {
            // The SMAC is used on interconnected sub-rings with a with virtual
            // channel
            check_smac_only = true;
        } else {
            // Don't check an unused port configuration.
            return VTSS_RC_OK;
        }
    }

    ring_port_conf = &conf->ring_port_conf[ring_port];

    if (!check_smac_only) {
        if (ring_port_conf->sf_trigger != VTSS_APPL_ERPS_SF_TRIGGER_LINK && ring_port_conf->sf_trigger != VTSS_APPL_ERPS_SF_TRIGGER_MEP) {
            return ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_RC_INVALID_SF_TRIGGER_PORT0 : VTSS_APPL_ERPS_RC_INVALID_SF_TRIGGER_PORT1;
        }

        // Check that the ifindex is representing a port.
        VTSS_RC(ERPS_ifindex_to_port(ring_port_conf->ifindex, ring_port, port_no));

        if (ring_port_conf->sf_trigger == VTSS_APPL_ERPS_SF_TRIGGER_MEP) {
            // Figure out whether the user has remembered to fill in the MEP key.
            VTSS_RC(cfm_util_key_check(ring_port_conf->mep, &empty));

            if (!empty) {
                // The MEP key is not empty, so its contents must be valid.
                VTSS_RC(cfm_util_key_check(ring_port_conf->mep));
            } else if (conf->admin_active) {
                // The MEP key cannot be empty when SF trigger is MEP and we are
                // administratively enabled.
                return ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_RC_PORT0_MEP_MUST_BE_SPECIFIED : VTSS_APPL_ERPS_RC_PORT1_MEP_MUST_BE_SPECIFIED;
            }
        }
    }

    // Check that the MAC address is a unicast MAC address usable as SMAC.
    if (ring_port_conf->smac.addr[0] & 0x1) {
        // SMAC must not be a multicast SMAC
        return ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_RC_INVALID_SMAC_PORT0 : VTSS_APPL_ERPS_RC_INVALID_SMAC_PORT1;
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// ERPS_ring_ports_in_common()
/******************************************************************************/
static bool ERPS_ring_ports_in_common(const vtss_appl_erps_conf_t *conf1, const vtss_appl_erps_conf_t *conf2)
{
    vtss_ifindex_t conf1_ifindex0    = conf1->ring_port_conf[VTSS_APPL_ERPS_RING_PORT0].ifindex;
    vtss_ifindex_t conf1_ifindex1    = conf1->ring_port_conf[VTSS_APPL_ERPS_RING_PORT1].ifindex;
    vtss_ifindex_t conf2_ifindex0    = conf2->ring_port_conf[VTSS_APPL_ERPS_RING_PORT0].ifindex;
    vtss_ifindex_t conf2_ifindex1    = conf2->ring_port_conf[VTSS_APPL_ERPS_RING_PORT1].ifindex;
    bool           conf1_using_port1 = erps_util_using_ring_port_conf(conf1, VTSS_APPL_ERPS_RING_PORT1);
    bool           conf2_using_port1 = erps_util_using_ring_port_conf(conf2, VTSS_APPL_ERPS_RING_PORT1);

    return (                                          conf1_ifindex0 == conf2_ifindex0) ||
           (                     conf2_using_port1 && conf1_ifindex0 == conf2_ifindex1) ||
           (conf1_using_port1                      && conf1_ifindex1 == conf2_ifindex0) ||
           (conf1_using_port1 && conf2_using_port1 && conf1_ifindex1 == conf2_ifindex1);
}

//*****************************************************************************/
// ERPS_default()
/******************************************************************************/
static void ERPS_default(void)
{
    vtss_appl_erps_conf_t new_conf;
    erps_itr_t            itr;

    ERPS_LOCK_SCOPE();

    // Start by setting all ERPS instances inactive and call to update the
    // configuration. This will release the ERPS resources in both MESA and the
    // CFM module, so that we can erase all of it in one go afterwards.
    for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
        if (itr->second.conf.admin_active) {
            new_conf = itr->second.conf;
            new_conf.admin_active = false;
            (void)ERPS_conf_update(itr, &new_conf);
        }
    }

    // Then erase all elements from the map.
    ERPS_map.clear();
}

/******************************************************************************/
// vtss_appl_erps_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_erps_capabilities_get(vtss_appl_erps_capabilities_t *cap)
{
    VTSS_RC(ERPS_ptr_check(cap));
    *cap = ERPS_cap;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_erps_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_erps_conf_default_get(vtss_appl_erps_conf_t *conf)
{
    if (conf == nullptr) {
        return VTSS_APPL_ERPS_RC_INVALID_PARAMETER;
    }

    *conf = ERPS_default_conf;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_erps_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_erps_conf_get(uint32_t inst, vtss_appl_erps_conf_t *conf)
{
    erps_itr_t itr;

    VTSS_RC(ERPS_inst_check(inst));
    VTSS_RC(ERPS_ptr_check(conf));

    T_N("Enter, %u", inst);

    ERPS_LOCK_SCOPE();

    if ((itr = ERPS_map.find(inst)) == ERPS_map.end()) {
        return VTSS_APPL_ERPS_RC_NO_SUCH_INSTANCE;
    }

    *conf = itr->second.conf;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_erps_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_erps_conf_set(uint32_t inst, const vtss_appl_erps_conf_t *conf)
{
    vtss_appl_erps_ring_port_t            ring_port;
    const vtss_appl_erps_ring_port_conf_t *port0_conf, *port1_conf;
    vtss_appl_erps_ring_port_conf_t       *ring_port_conf;
    vtss_appl_erps_conf_t                 local_conf;
    erps_itr_t                            itr, itr2;
    erps_state_t                          *erps_state2;
    mesa_vid_t                            vid;
    char                                  mac_str1[18], mac_str2[18];
    char                                  vid_str[VLAN_VID_LIST_AS_STRING_LEN_BYTES];
    int                                   i;

    VTSS_RC(ERPS_inst_check(inst));
    VTSS_RC(ERPS_ptr_check(conf));

    port0_conf = &conf->ring_port_conf[VTSS_APPL_ERPS_RING_PORT0];
    port1_conf = &conf->ring_port_conf[VTSS_APPL_ERPS_RING_PORT1];

    T_D("Enter, inst = %u, version = %s, ring_type = %s, ring_id = %u, level = %u, "
        "control_vlan = %u, pcp = %u, connected_ring_inst = %u, tc_propagate = %d, virtual_channel = %d, "
        "port0.sf_trigger = %s, port0.ifindex = %u, port0.mep = %s, port0.smac = %s, "
        "port1.sf_trigger = %s, port1.ifindex = %u, port1.mep = %s, port1.smac = %s, "
        "revertive = %d, wtr_secs = %u, guard_time_msecs = %u, hold_off_msecs = %u, "
        "rpl_mode = %s, rpl_port = %s, protected_vlans = %s, admin_active = %d",
        inst, erps_util_version_to_str(conf->version), erps_util_ring_type_to_str(conf->ring_type), conf->ring_id, conf->level,
        conf->control_vlan, conf->pcp, conf->interconnect_conf.connected_ring_inst, conf->interconnect_conf.tc_propagate, conf->virtual_channel,
        erps_util_sf_trigger_to_str(port0_conf->sf_trigger), VTSS_IFINDEX_PRINTF_ARG(port0_conf->ifindex), port0_conf->mep, misc_mac_txt(port0_conf->smac.addr, mac_str1),
        erps_util_sf_trigger_to_str(port1_conf->sf_trigger), VTSS_IFINDEX_PRINTF_ARG(port1_conf->ifindex), port1_conf->mep, misc_mac_txt(port1_conf->smac.addr, mac_str2),
        conf->revertive, conf->wtr_secs, conf->guard_time_msecs, conf->hold_off_msecs,
        erps_util_rpl_mode_to_str(conf->rpl_mode), conf->rpl_port, vlan_mgmt_vid_bitmask_to_txt(conf->protected_vlans, vid_str), conf->admin_active);

    if (conf->version != VTSS_APPL_ERPS_VERSION_V1 && conf->version != VTSS_APPL_ERPS_VERSION_V2) {
        return VTSS_APPL_ERPS_RC_INVALID_VERSION;
    }

    if (conf->ring_type != VTSS_APPL_ERPS_RING_TYPE_MAJOR && conf->ring_type != VTSS_APPL_ERPS_RING_TYPE_SUB && conf->ring_type != VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
        return VTSS_APPL_ERPS_RC_INVALID_RING_TYPE;
    }

    if (conf->ring_id < 1 || conf->ring_id > 239) {
        return VTSS_APPL_ERPS_RC_INVALID_RING_ID;
    }

    if (conf->level > 7) {
        return VTSS_APPL_ERPS_RC_INVALID_LEVEL;
    }

    if (conf->control_vlan < VTSS_APPL_VLAN_ID_MIN || conf->control_vlan > VTSS_APPL_VLAN_ID_MAX) {
        return VTSS_APPL_ERPS_RC_INVALID_CONTROL_VLAN;
    }

    if (conf->pcp > 7) {
        return VTSS_APPL_ERPS_RC_INVALID_PCP;
    }

    if (conf->ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB && ERPS_inst_check(conf->interconnect_conf.connected_ring_inst) != VTSS_RC_OK) {
        return VTSS_APPL_ERPS_RC_INVALID_CONNECTED_RING_INST;
    }

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        VTSS_RC(ERPS_ring_port_conf_chk(conf, ring_port));
    }

    // Check that the Node ID is a unicast MAC address.
    if (conf->node_id.addr[0] & 0x1) {
        // SMAC must not be a multicast SMAC
        return VTSS_APPL_ERPS_RC_INVALID_NODE_ID;
    }

    if (conf->wtr_secs < 1 || conf->wtr_secs > ERPS_cap.wtr_secs_max) {
        return VTSS_APPL_ERPS_RC_INVALID_WTR;
    }

    if (conf->guard_time_msecs > ERPS_cap.guard_time_msecs_max) {
        return VTSS_APPL_ERPS_RC_INVALID_GUARD_TIME;
    }

    if (conf->guard_time_msecs % 10) {
        // Must be in steps of 10 milliseconds
        return VTSS_APPL_ERPS_RC_INVALID_GUARD_TIME;
    }

    if (conf->hold_off_msecs > ERPS_cap.hold_off_msecs_max) {
        return VTSS_APPL_ERPS_RC_INVALID_HOLD_OFF;
    }

    if (conf->hold_off_msecs % 100) {
        // Must be in steps of 100 milliseconds
        return VTSS_APPL_ERPS_RC_INVALID_HOLD_OFF;
    }

    if (conf->rpl_mode != VTSS_APPL_ERPS_RPL_MODE_NONE && conf->rpl_mode != VTSS_APPL_ERPS_RPL_MODE_OWNER && conf->rpl_mode != VTSS_APPL_ERPS_RPL_MODE_NEIGHBOR) {
        return VTSS_APPL_ERPS_RC_INVALID_RPL_MODE;
    }

    if (conf->rpl_port != VTSS_APPL_ERPS_RING_PORT0 && conf->rpl_port != VTSS_APPL_ERPS_RING_PORT1) {
        return VTSS_APPL_ERPS_RC_INVALID_RPL_PORT;
    }

    // Checks that have to be deferred until we get administratively enabled
    if (conf->admin_active) {
        // Inter-port0-port1 checks
        if (erps_util_using_ring_port_conf(conf, VTSS_APPL_ERPS_RING_PORT1)) {
            // port0 and port1 must use different ifindices.
            if (port0_conf->ifindex == port1_conf->ifindex) {
                return VTSS_APPL_ERPS_RC_0_1_IFINDEX_IDENTICAL;
            }

            if (port0_conf->sf_trigger == VTSS_APPL_ERPS_SF_TRIGGER_MEP &&
                port1_conf->sf_trigger == VTSS_APPL_ERPS_SF_TRIGGER_MEP &&
                port0_conf->mep        == port1_conf->mep) {
                // The two ring ports cannot use the same MEP.
                return VTSS_APPL_ERPS_RC_0_1_MEP_IDENTICAL;
            }
        } else {
            // Interconnected sub-ring checks.
            // Port1 cannot be an RPL port.
            if (conf->rpl_mode != VTSS_APPL_ERPS_RPL_MODE_NONE && conf->rpl_port == VTSS_APPL_ERPS_RING_PORT1) {
                return VTSS_APPL_ERPS_RC_INTERCONNECTED_SUB_RING_CANNOT_USE_RPL_PORT1;
            }
        }

        // There must be at least one protected VLAN...
        for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
            if (VTSS_BF_GET(conf->protected_vlans, vid)) {
                break;
            }
        }

        if (vid > VTSS_APPL_VLAN_ID_MAX) {
            // ...but there isn't.
            return VTSS_APPL_ERPS_RC_INVALID_PROTECTED_VLANS;
        }

        // The control VLAN cannot be part of the protected VLANs.
        if (VTSS_BF_GET(conf->protected_vlans, conf->control_vlan)) {
            return VTSS_APPL_ERPS_RC_CONTROL_VLAN_CANNOT_BE_PROTECTED_VLAN;
        }

        // Restrictions when using G.8032v1.
        if (conf->version == VTSS_APPL_ERPS_VERSION_V1) {
            // It must be a major ring
            if (conf->ring_type != VTSS_APPL_ERPS_RING_TYPE_MAJOR) {
                return VTSS_APPL_ERPS_RC_RING_TYPE_MUST_BE_MAJOR_WHEN_USING_V1;
            }

            // It must have Ring ID 1
            if (conf->ring_id != 1) {
                return VTSS_APPL_ERPS_RC_RING_ID_MUST_BE_1_WHEN_USING_V1;
            }

            // It must not be non-revertive
            if (!conf->revertive) {
                return VTSS_APPL_ERPS_RC_REVERTIVE_MUST_BE_TRUE_WHEN_USING_V1;
            }
        }

        if (conf->ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
            if (conf->interconnect_conf.connected_ring_inst == inst) {
                // The referenced connected ring cannot be ourselves. We cannot
                // check that the referenced ERPS instance is not an
                // interconnected sub-ring itself at this time, because it may
                // change if that referenced ring is changed, so that check will
                // cause an operational warning.
                return VTSS_APPL_ERPS_RC_INTERCONNECTED_SUB_RING_CANNOT_REFERENCE_ITSELF;
            }
        }
    }

    // Time to normalize the configuration, that is, default unused
    // configuration. This is in order to avoid happening to re-use dead
    // configuration if that configuration becomes active again due to a change
    // in e.g. ring-type.
    local_conf = *conf;
    if (local_conf.admin_active) {
        for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
            ring_port_conf = &local_conf.ring_port_conf[ring_port];
            if (ring_port_conf->sf_trigger == VTSS_APPL_ERPS_SF_TRIGGER_LINK) {
                ring_port_conf->mep = ERPS_default_conf.ring_port_conf[ring_port].mep;
            }
        }

        if (local_conf.ring_type != VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
            // Default interconnect parameters
            local_conf.interconnect_conf = ERPS_default_conf.interconnect_conf;
        }

        if (local_conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_MAJOR) {
            // Major rings don't use virtual channels
            local_conf.virtual_channel = false;
        }
    }

    ERPS_LOCK_SCOPE();

    if ((itr = ERPS_map.find(inst)) != ERPS_map.end()) {
        if (memcmp(&local_conf, &itr->second, sizeof(local_conf)) == 0) {
            // No changes.
            return VTSS_RC_OK;
        }
    }

    // Check that we haven't created more instances than we can allow
    if (itr == ERPS_map.end()) {
        if (ERPS_map.size() >= ERPS_cap.inst_cnt_max) {
            return VTSS_APPL_ERPS_RC_LIMIT_REACHED;
        }
    }

    // Cross-ERPS instance checks
    if (local_conf.admin_active) {
        for (itr2 = ERPS_map.begin(); itr2 != ERPS_map.end(); ++itr2) {
            if (itr2 == itr) {
                // Don't check against our selves.
                continue;
            }

            erps_state2 = &itr2->second;

            if (!erps_state2->conf.admin_active) {
                continue;
            }

            if (!ERPS_ring_ports_in_common(&local_conf, &erps_state2->conf)) {
                // Done with inter-instance-checks
                continue;
            }

            // Two ERPS instances that have ring ports in common must protect
            // different VLANs.
            for (i = 0; i < ARRSZ(local_conf.protected_vlans); i++) {
                if (local_conf.protected_vlans[i] & erps_state2->conf.protected_vlans[i]) {
                    return VTSS_APPL_ERPS_RC_TWO_INST_ON_SAME_INTERFACES_WITH_OVERLAPPING_VLANS;
                }
            }

            // Two ERPS instances that have ring ports in common cannot use the
            // same control VLAN and Ring ID.
            if (local_conf.control_vlan == erps_state2->conf.control_vlan &&
                local_conf.ring_id      == erps_state2->conf.ring_id) {
                return VTSS_APPL_ERPS_RC_TWO_INST_ON_SAME_INTERFACES_WITH_SAME_CONTROL_VLAN_AND_RING_ID;
            }
        }
    }

    // Create a new or update an existing entry
    if ((itr = ERPS_map.get(inst)) == ERPS_map.end()) {
        return VTSS_APPL_ERPS_RC_OUT_OF_MEMORY;
    }

    return ERPS_conf_update(itr, &local_conf);
}

//*****************************************************************************/
// vtss_appl_erps_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_erps_conf_del(uint32_t inst)
{
    vtss_appl_erps_conf_t new_conf;
    erps_itr_t            itr;
    mesa_rc               rc;

    VTSS_RC(ERPS_inst_check(inst));

    T_D("Enter, %u", inst);

    ERPS_LOCK_SCOPE();

    if ((itr = ERPS_map.find(inst)) == ERPS_map.end()) {
        return VTSS_APPL_ERPS_RC_NO_SUCH_INSTANCE;
    }

    if (itr->second.conf.admin_active) {
        new_conf = itr->second.conf;
        new_conf.admin_active = false;

        // Back out of everything
        rc = ERPS_conf_update(itr, &new_conf);
    } else {
        rc = VTSS_RC_OK;
    }

    // Delete ERPS instance from our map
    ERPS_map.erase(inst);

    return rc;
}

//*****************************************************************************/
// vtss_appl_erps_itr()
/******************************************************************************/
mesa_rc vtss_appl_erps_itr(const uint32_t *prev_inst, uint32_t *next_inst)
{
    erps_itr_t itr;

    VTSS_RC(ERPS_ptr_check(next_inst));

    ERPS_LOCK_SCOPE();

    if (prev_inst) {
        // Here we have a valid prev_inst. Find the next from that one.
        itr = ERPS_map.greater_than(*prev_inst);
    } else {
        // We don't have a valid prev_inst. Get the first.
        itr = ERPS_map.begin();
    }

    if (itr != ERPS_map.end()) {
        *next_inst = itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

//*****************************************************************************/
// vtss_appl_erps_control_get()
/******************************************************************************/
mesa_rc vtss_appl_erps_control_get(uint32_t inst, vtss_appl_erps_control_t *ctrl)
{
    erps_itr_t itr;

    VTSS_RC(ERPS_inst_check(inst));
    VTSS_RC(ERPS_ptr_check(ctrl));

    ERPS_LOCK_SCOPE();

    if ((itr = ERPS_map.find(inst)) == ERPS_map.end()) {
        return VTSS_APPL_ERPS_RC_NO_SUCH_INSTANCE;
    }

    memset(ctrl, 0, sizeof(*ctrl));

    ctrl->command = itr->second.command;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_erps_control_set()
/******************************************************************************/
mesa_rc vtss_appl_erps_control_set(uint32_t inst, const vtss_appl_erps_control_t *ctrl)
{
    erps_itr_t               itr;
    vtss_appl_erps_conf_t    *conf;
    vtss_appl_erps_command_t cur_cmd, new_cmd;

    VTSS_RC(ERPS_inst_check(inst));
    VTSS_RC(ERPS_ptr_check(ctrl));

    T_D("Enter, inst = %u, command = %s", inst, erps_util_command_to_str(ctrl->command));

    new_cmd = ctrl->command;

    if (new_cmd <= VTSS_APPL_ERPS_COMMAND_NR || new_cmd > VTSS_APPL_ERPS_COMMAND_CLEAR) {
        return VTSS_APPL_ERPS_RC_INVALID_COMMAND;
    }

    ERPS_LOCK_SCOPE();

    if ((itr = ERPS_map.find(inst)) == ERPS_map.end()) {
        return VTSS_APPL_ERPS_RC_NO_SUCH_INSTANCE;
    }

    conf    = &itr->second.conf;
    cur_cmd = itr->second.command;

    if (!conf->admin_active) {
        return VTSS_APPL_ERPS_RC_NOT_ACTIVE;
    }

    if (new_cmd == cur_cmd) {
        return VTSS_RC_OK;
    }

    if (!ERPS_started) {
        return VTSS_APPL_ERPS_RC_NOT_READY_TRY_AGAIN_IN_A_FEW_SECONDS;
    }

    if (conf->version == VTSS_APPL_ERPS_VERSION_V1) {
        // If we are using V1 of G.8032, FS and MS are not possible.
        if (new_cmd == VTSS_APPL_ERPS_COMMAND_FS_TO_PORT0 ||
            new_cmd == VTSS_APPL_ERPS_COMMAND_FS_TO_PORT1 ||
            new_cmd == VTSS_APPL_ERPS_COMMAND_MS_TO_PORT0 ||
            new_cmd == VTSS_APPL_ERPS_COMMAND_MS_TO_PORT1) {
            return VTSS_APPL_ERPS_RC_COMMAND_NOT_SUPPORTED_WHEN_USING_V1;
        }
    }

    return erps_base_command_set(&itr->second, new_cmd);
}

//*****************************************************************************/
// vtss_appl_erps_status_get()
/******************************************************************************/
mesa_rc vtss_appl_erps_status_get(uint32_t inst, vtss_appl_erps_status_t *status)
{
    erps_itr_t itr;

    VTSS_RC(ERPS_inst_check(inst));
    VTSS_RC(ERPS_ptr_check(status));

    ERPS_LOCK_SCOPE();

    if ((itr = ERPS_map.find(inst)) == ERPS_map.end()) {
        return VTSS_APPL_ERPS_RC_NO_SUCH_INSTANCE;
    }

    *status = itr->second.status;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_erps_statistics_clear()
/******************************************************************************/
mesa_rc vtss_appl_erps_statistics_clear(uint32_t inst)
{
    erps_itr_t itr;

    VTSS_RC(ERPS_inst_check(inst));

    ERPS_LOCK_SCOPE();

    if ((itr = ERPS_map.find(inst)) == ERPS_map.end()) {
        return VTSS_APPL_ERPS_RC_NO_SUCH_INSTANCE;
    }

    erps_base_statistics_clear(&itr->second);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// erps_util_using_ring_port_conf()
//*****************************************************************************/
bool erps_util_using_ring_port_conf(const vtss_appl_erps_conf_t *conf, vtss_appl_erps_ring_port_t ring_port)
{
    if (!conf) {
        T_E("conf cannot be nullptr");
        return false;
    }

    return ring_port == VTSS_APPL_ERPS_RING_PORT0 || (ring_port == VTSS_APPL_ERPS_RING_PORT1 && conf->ring_type != VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB);
}

//*****************************************************************************/
// erps_util_command_to_str()
/******************************************************************************/
const char *erps_util_command_to_str(vtss_appl_erps_command_t command)
{
    switch (command) {
    case VTSS_APPL_ERPS_COMMAND_NR:
        return "None";

    case VTSS_APPL_ERPS_COMMAND_FS_TO_PORT0:
        return "Forced-switch-to-port0";

    case VTSS_APPL_ERPS_COMMAND_FS_TO_PORT1:
        return "Forced-switch-to-port1";

    case VTSS_APPL_ERPS_COMMAND_MS_TO_PORT0:
        return "Manual-switch-to-port0";

    case VTSS_APPL_ERPS_COMMAND_MS_TO_PORT1:
        return "Manual-switch-to-port1";

    case VTSS_APPL_ERPS_COMMAND_CLEAR:
        return "Clear";

    default:
        T_E("Invalid command (%d)", command);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_version_to_str()
/******************************************************************************/
const char *erps_util_version_to_str(vtss_appl_erps_version_t version)
{
    switch (version) {
    case VTSS_APPL_ERPS_VERSION_V1:
        return "v1";

    case VTSS_APPL_ERPS_VERSION_V2:
        return "v2";

    default:
        T_E("Invalid version (%d)", version);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_ring_type_to_str()
/******************************************************************************/
const char *erps_util_ring_type_to_str(vtss_appl_erps_ring_type_t ring_type, bool capital_first_letter)
{
    switch (ring_type) {
    case VTSS_APPL_ERPS_RING_TYPE_MAJOR:
        return capital_first_letter ? "Major" : "major";

    case VTSS_APPL_ERPS_RING_TYPE_SUB:
        return capital_first_letter ? "Sub-ring" : "sub-ring";

    case VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB:
        return capital_first_letter ? "Interconnected sub-ring" : "interconnected-sub-ring";

    default:
        T_E("Invalid ring_type (%d)", ring_type);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_sf_trigger_to_str()
/******************************************************************************/
const char *erps_util_sf_trigger_to_str(vtss_appl_erps_sf_trigger_t sf_trigger)
{
    switch (sf_trigger) {
    case VTSS_APPL_ERPS_SF_TRIGGER_LINK:
        return "link";

    case VTSS_APPL_ERPS_SF_TRIGGER_MEP:
        return "mep";

    default:
        T_E("Invalid sf_trigger (%d)", sf_trigger);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_rpl_mode_to_str()
/******************************************************************************/
const char *erps_util_rpl_mode_to_str(vtss_appl_erps_rpl_mode_t rpl_mode, bool capital_first_letter)
{
    switch (rpl_mode) {
    case VTSS_APPL_ERPS_RPL_MODE_NONE:
        return capital_first_letter ? "None" : "none";

    case VTSS_APPL_ERPS_RPL_MODE_OWNER:
        return capital_first_letter ? "Owner" : "owner";

    case VTSS_APPL_ERPS_RPL_MODE_NEIGHBOR:
        return capital_first_letter ? "Neighbor" : "neighbor";

    default:
        T_E("Invalid rpl_mode (%d)", rpl_mode);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_ring_port_to_str()
/******************************************************************************/
const char *erps_util_ring_port_to_str(vtss_appl_erps_ring_port_t ring_port)
{
    switch (ring_port) {
    case VTSS_APPL_ERPS_RING_PORT0:
        return "port0";

    case VTSS_APPL_ERPS_RING_PORT1:
        return "port1";

    default:
        T_E("Invalid ring_port (%d)", ring_port);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_oper_state_to_str()
/******************************************************************************/
const char *erps_util_oper_state_to_str(vtss_appl_erps_oper_state_t oper_state)
{
    switch (oper_state) {
    case VTSS_APPL_ERPS_OPER_STATE_ADMIN_DISABLED:
        return "Administratively disabled";

    case VTSS_APPL_ERPS_OPER_STATE_ACTIVE:
        return "Active";

    case VTSS_APPL_ERPS_OPER_STATE_INTERNAL_ERROR:
        return "Internal error has occurred. See console or crashlog for details";

    default:
        T_E("Invalid operational state (%d)", oper_state);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_oper_state_to_str()
/******************************************************************************/
const char *erps_util_oper_state_to_str(vtss_appl_erps_oper_state_t oper_state, vtss_appl_erps_oper_warning_t oper_warning)
{
    if (oper_warning == VTSS_APPL_ERPS_OPER_WARNING_NONE || oper_state != VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
        return erps_util_oper_state_to_str(oper_state);
    }

    return "Active (warnings)";
}

//*****************************************************************************/
// erps_util_oper_warning_to_str()
/******************************************************************************/
const char *erps_util_oper_warning_to_str(vtss_appl_erps_oper_warning_t oper_warning)
{
    switch (oper_warning) {
    case VTSS_APPL_ERPS_OPER_WARNING_NONE:
        return "None";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT0_NOT_MEMBER_OF_CONTROL_VLAN:
        return "Ring port0 is not member of the control VLAN";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT1_NOT_MEMBER_OF_CONTROL_VLAN:
        return "Ring port1 is not member of the control VLAN";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT0_UNTAGS_CONTROL_VLAN:
        return "Ring port0's VLAN configuration causes the control VLAN to become untagged on egress";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT1_UNTAGS_CONTROL_VLAN:
        return "Ring port1's VLAN configuration causes the control VLAN to become untagged on egress";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_FOUND:
        return "The port0 MEP does not exist. Using link-state for signal-fail";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_FOUND:
        return "The port1 MEP does not exist. Using link-state for signal-fail";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_ADMIN_DISABLED:
        return "The port0 MEP is administratively disabled. Using link-state for signal-fail";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_ADMIN_DISABLED:
        return "The port1 MEP is administratively disabled. Using link-state for signal-fail";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_DOWN_MEP:
        return "The port0 MEP is not a Down-MEP. Using link-state for signal-fail";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_DOWN_MEP:
        return "The port1 MEP is not a Down-MEP. Using link-state for signal-fail";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT0_AND_MEP_IFINDEX_DIFFER:
        return "Port0's MEP is installed on another interface. Using link-state for signal-fail";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT1_AND_MEP_IFINDEX_DIFFER:
        return "Port1's MEP is installed on another interface. Using link-state for signal-fail";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT_MEP_SHADOWS_PORT0_MIP:
        return "A port MEP on same interface as port0 shadows reception of R-APS PDUs due to MD/MEG level filtering";

    case VTSS_APPL_ERPS_OPER_WARNING_PORT_MEP_SHADOWS_PORT1_MIP:
        return "A port MEP on same interface as port1 shadows reception of R-APS PDUs due to MD/MEG level filtering";

    case VTSS_APPL_ERPS_OPER_WARNING_MEP_SHADOWS_PORT0_MIP:
        return "A MEP on same interface and VLAN as port0 shadows reception of R-APS PDUs due to MD/MEG level filtering";

    case VTSS_APPL_ERPS_OPER_WARNING_MEP_SHADOWS_PORT1_MIP:
        return "A MEP on same interface and VLAN as port1 shadows reception of R-APS PDUs due to MD/MEG level filtering";

    case VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_DOESNT_EXIST:
        return "The referenced connected ring doesn't exist";

    case VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_IS_AN_INTERCONNECTED_SUB_RING:
        return "The referenced connected ring cannot itself be an interconnected sub-ring";

    case VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_IS_NOT_OPERATIVE:
        return "The referenced connected ring is not operatively active";

    case VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_INTERFACE_CONFLICT:
        return "The referenced connected ring's port0 or port1 is the same as this interconnected sub-ring's port0";

    case VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_DOESNT_PROTECT_CONTROL_VLAN:
        return "The referenced connected ring does not protect this sub-ring's control VLAN";

    default:
        T_E("Invalid operational warning (%d)", oper_warning);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_node_state_to_str()
/******************************************************************************/
const char *erps_util_node_state_to_str(vtss_appl_erps_node_state_t node_state)
{
    switch (node_state) {
    case VTSS_APPL_ERPS_NODE_STATE_INIT:
        return "Init";

    case VTSS_APPL_ERPS_NODE_STATE_IDLE:
        return "Idle";

    case VTSS_APPL_ERPS_NODE_STATE_PROTECTION:
        return "Protection";

    case VTSS_APPL_ERPS_NODE_STATE_MS:
        return "MS";

    case VTSS_APPL_ERPS_NODE_STATE_FS:
        return "FS";

    case VTSS_APPL_ERPS_NODE_STATE_PENDING:
        return "Pending";

    default:
        T_E("Invalid node state (%d)", node_state);
        return "unknown";
    }
}

//*****************************************************************************/
// erps_util_request_to_str()
//*****************************************************************************/
const char *erps_util_request_to_str(vtss_appl_erps_request_t request)
{
    switch (request) {
    case VTSS_APPL_ERPS_REQUEST_NR:
        return "NR";

    case VTSS_APPL_ERPS_REQUEST_MS:
        return "MS";

    case VTSS_APPL_ERPS_REQUEST_SF:
        return "SF";

    case VTSS_APPL_ERPS_REQUEST_FS:
        return "FS";

    case VTSS_APPL_ERPS_REQUEST_EVENT:
        return "Event";

    default:
        T_E("Invalid request (%d)", request);
        return "unknown";
    }
}

/******************************************************************************/
// erps_util_raps_info_to_str()
// Example:
//   NR, RB, DNF, BPR=port0
//
// If #include_bpr is true:
//     Longest possible: "Event, RB, DNF, BPR=port1" = 26 chars incl. null
//     Longest valid:    "NR, RB, DNF, BPR=port1"    = 23 chars incl. null
// If #include_bpr is false:
//     Longest possible: "Event, RB, DNF"            = 15 chars incl. null
//     Longest valid:    "NR, RB, DNF"               = 12 chars incl. null
/******************************************************************************/
const char *erps_util_raps_info_to_str(vtss_appl_erps_raps_info_t &raps_info, char str[26], bool active, bool include_bpr)
{
    if (!active || raps_info.update_time_secs == 0) {
        // Should not be displayed or never been updated
        strcpy(str, "-");
    } else {
        sprintf(str, "%s%s%s%s%s", erps_util_request_to_str(raps_info.request), raps_info.rb ? ", RB" : "", raps_info.dnf ? ", DNF" : "", include_bpr ? ", BPR=" : "", include_bpr ? erps_util_ring_port_to_str(raps_info.bpr) : "");
    }

    return str;
}

//*****************************************************************************/
// erps_error_txt()
//*****************************************************************************/
const char *erps_error_txt(mesa_rc error)
{
    switch (error) {
    case VTSS_APPL_ERPS_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_ERPS_RC_INTERNAL_ERROR:
        return "Internal Error: A code-update is required. See console or crashlog for details";

    case VTSS_APPL_ERPS_RC_NO_SUCH_INSTANCE:
        return "No such ERPS instance";

    case VTSS_APPL_ERPS_RC_INVALID_PORT0_IFINDEX:
        return "Invalid ring port0 interface. The interface index must represent a port";

    case VTSS_APPL_ERPS_RC_INVALID_PORT1_IFINDEX:
        return "Invalid ring port1 interface. The interface index must represent a port";

    case VTSS_APPL_ERPS_RC_INVALID_VERSION:
        return "Invalid version";

    case VTSS_APPL_ERPS_RC_INVALID_RING_TYPE:
        return "Invalid ring type";

    case VTSS_APPL_ERPS_RC_INVALID_RING_ID:
        return "Invalid ring ID. Valid range is [1; 239]";

    case VTSS_APPL_ERPS_RC_INVALID_LEVEL:
        return "Invalid MD/MEG level. Valid range is [0; 7]";

    case VTSS_APPL_ERPS_RC_INVALID_CONTROL_VLAN:
        return "Invalid control VLAN. Valid range is [" vtss_xstr(VTSS_APPL_VLAN_ID_MIN) "; " vtss_xstr(VTSS_APPL_VLAN_ID_MAX) "]";

    case VTSS_APPL_ERPS_RC_INVALID_PCP:
        return "Invalid PCP. Valid range is [0; 7]";

    case VTSS_APPL_ERPS_RC_INVALID_CONNECTED_RING_INST:
        return "Invalid instance number specified for connected ring";

    case VTSS_APPL_ERPS_RC_INVALID_SF_TRIGGER_PORT0:
        return "Invalid signal-fail trigger for port 0";

    case VTSS_APPL_ERPS_RC_INVALID_SF_TRIGGER_PORT1:
        return "Invalid signal-fail trigger for port 1";

    case VTSS_APPL_ERPS_RC_INVALID_SMAC_PORT0:
        return "Port0's source MAC address must be a unicast address";

    case VTSS_APPL_ERPS_RC_INVALID_SMAC_PORT1:
        return "Port1's source MAC address must be a unicast address";

    case VTSS_APPL_ERPS_RC_INVALID_NODE_ID:
        return "Node ID must be a unicast MAC address";

    case VTSS_APPL_ERPS_RC_INVALID_WTR:
        return "Invalid wait-to-restore value. Valid range is [1; 720] seconds";

    case VTSS_APPL_ERPS_RC_INVALID_GUARD_TIME:
        return "Invalid guard-time. Valid range is [10; 2000] milliseconds in steps of 10 milliseconds";

    case VTSS_APPL_ERPS_RC_INVALID_HOLD_OFF:
        return "Invalid hold-off-time. Valid range is [0; 10000] milliseconds in steps of 100 milliseconds";

    case VTSS_APPL_ERPS_RC_INVALID_RPL_MODE:
        return "Invalid RPL mode";

    case VTSS_APPL_ERPS_RC_INVALID_RPL_PORT:
        return "Invalid RPL port";

    case VTSS_APPL_ERPS_RC_INVALID_PROTECTED_VLANS:
        return "Protected VLANs is empty. At least one VLAN must be protected";

    case VTSS_APPL_ERPS_RC_CONTROL_VLAN_CANNOT_BE_PROTECTED_VLAN:
        return "The control VLAN cannot be part of the protected VLANs";

    case VTSS_APPL_ERPS_RC_PORT0_MEP_MUST_BE_SPECIFIED:
        return "Port0 MEP must be specified when using MEP as signal-fail trigger and instance is adminstratively enabled";

    case VTSS_APPL_ERPS_RC_PORT1_MEP_MUST_BE_SPECIFIED:
        return "Port1 MEP must be specified when using MEP as signal-fail trigger and instance is administratively enabled";

    case VTSS_APPL_ERPS_RC_RING_TYPE_MUST_BE_MAJOR_WHEN_USING_V1:
        return "Ring type must be \"major\" when using G.8032v1";

    case VTSS_APPL_ERPS_RC_RING_ID_MUST_BE_1_WHEN_USING_V1:
        return "Ring ID must be \"1\" when using G.8032v1";

    case VTSS_APPL_ERPS_RC_REVERTIVE_MUST_BE_TRUE_WHEN_USING_V1:
        return "Revertive switching must be enabled when using G.8032v1";

    case VTSS_APPL_ERPS_RC_INTERCONNECTED_SUB_RING_CANNOT_REFERENCE_ITSELF:
        return "An interconnected sub-ring cannot reference itself as its connected ring";

    case VTSS_APPL_ERPS_RC_0_1_MEP_IDENTICAL:
        return "Port0 and Port1 MEPs cannot be the same";

    case VTSS_APPL_ERPS_RC_INTERCONNECTED_SUB_RING_CANNOT_USE_RPL_PORT1:
        return "An interconnected sub-ring cannot use Port1 as RPL port";

    case VTSS_APPL_ERPS_RC_0_1_IFINDEX_IDENTICAL:
        return "Port0 and Port1 cannot use the same interface";

    case VTSS_APPL_ERPS_RC_LIMIT_REACHED:
        return "The maximum number of ERPS instances is reached";

    case VTSS_APPL_ERPS_RC_TWO_INST_ON_SAME_INTERFACES_WITH_OVERLAPPING_VLANS:
        return "Another ERPS instance has ring interfaces and protected VLANs in common with this one";

    case VTSS_APPL_ERPS_RC_TWO_INST_ON_SAME_INTERFACES_WITH_SAME_CONTROL_VLAN_AND_RING_ID:
        return "Another ERPS instance has ring interfaces in common with this one and use the same control VLAN and ring ID";

    case VTSS_APPL_ERPS_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_ERPS_RC_INVALID_COMMAND:
        return "Invalid command";

    case VTSS_APPL_ERPS_RC_COMMAND_NOT_SUPPORTED_WHEN_USING_V1:
        return "ERPS instance is configured to using G.8032v1, where this command is not valid";

    case VTSS_APPL_ERPS_RC_NOT_ACTIVE:
        return "ERPS instance is not active";

    case VTSS_APPL_ERPS_RC_NOT_READY_TRY_AGAIN_IN_A_FEW_SECONDS:
        return "ERPS is not yet ready. Please try again in a few seconds";

    case VTSS_APPL_ERPS_RC_HW_RESOURCES:
        return "Out of H/W resources";

    default:
        T_E("Unknown error code (%u)", error);
        return "ERPS: Unknown error code";
    }
}

/******************************************************************************/
// erps_history_dump()
/******************************************************************************/
void erps_history_dump(uint32_t relative_to, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    erps_itr_t itr;
    bool       first = true;

    ERPS_LOCK_SCOPE();

    for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
        erps_base_history_dump(&itr->second, relative_to, session_id, pr, first);
        first = false;
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// erps_history_clear()
/******************************************************************************/
void erps_history_clear(void)
{
    erps_itr_t itr;

    ERPS_LOCK_SCOPE();

    for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
        erps_base_history_clear(&itr->second);
    }
}

/******************************************************************************/
// erps_rules_dump()
/******************************************************************************/
void erps_rules_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    erps_itr_t itr;
    bool       first = true;

    ERPS_LOCK_SCOPE();

    for (itr = ERPS_map.begin(); itr != ERPS_map.end(); ++itr) {
        erps_base_rule_dump(&itr->second, session_id, pr, first);
        first = false;
    }

    pr(session_id, "\n");
}

extern "C" int erps_icli_cmd_register(void);

//*****************************************************************************/
// erps_init()
/******************************************************************************/
mesa_rc erps_init(vtss_init_data_t *data)
{
    vtss_appl_cfm_capabilities_t cfm_cap;
    mesa_rc                      rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        ERPS_capabilities_set();
        ERPS_default_conf_set();

        critd_init(&ERPS_crit, "ERPS", VTSS_MODULE_ID_ERPS, CRITD_TYPE_MUTEX);

        erps_icli_cmd_register();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        erps_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_erps_json_init();
#endif

        ERPS_cfm_change_notifications.init();
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        // Initialize ICLI "show running" configuration
        mesa_rc erps_icfg_init(void);
        VTSS_RC(erps_icfg_init());
#endif

        ERPS_cap_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);

        if ((rc = vtss_appl_cfm_capabilities_get(&cfm_cap)) != VTSS_RC_OK) {
            T_E("vtss_appl_cfm_capabilities_get() failed: %s", error_txt(rc));
            ERPS_cap_cfm_has_shared_meg_level = fast_cap(MESA_CAP_VOP_V1);
        } else {
            ERPS_cap_cfm_has_shared_meg_level = cfm_cap.has_shared_meg_level;
        }

        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            ERPS_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        // Initialize the port-state array
        ERPS_port_state_init();

        {
            ERPS_LOCK_SCOPE();
            erps_timer_init();
        }

        // Register for CFM configuration change callbacks in CFM module
        if ((rc = cfm_util_conf_change_callback_register(VTSS_MODULE_ID_ERPS, ERPS_cfm_conf_change_callback)) != VTSS_RC_OK) {
            T_E("cfm_util_conf_change_callback_register() failed: %s", error_txt(rc));
        }

        ERPS_default();

        // Register for R-APS PDUs
        ERPS_frame_rx_register();

        break;

    case INIT_CMD_ICFG_LOADING_POST: {
        // Now that ICLI has applied all configuration, start creating all the
        // ERPS instances. Only do this the very first time an
        // INIT_CMD_ICFG_LOADING_POST is issued.
        // It's fine not to wait for CFM to have all its MEPs up and running,
        // because we will resort to signal fail based on link if any MEP is not
        // yet constructed.
        ERPS_LOCK_SCOPE();

        if (!ERPS_started) {
            ERPS_started = true;
            ERPS_oper_state_update_all();
        }

        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

