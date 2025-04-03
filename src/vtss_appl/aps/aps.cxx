/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include <vtss/appl/aps.h>
#include "aps_api.h"
#include "aps_base.hxx"
#include "aps_lock.hxx"
#include "aps_timer.hxx"
#include "aps_trace.h"
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

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_APS

static vtss_trace_reg_t APS_trace_reg = {
    VTSS_TRACE_MODULE_ID, "APS", "Automatic Protection Switching"
};

static vtss_trace_grp_t APS_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_BASE] = {
        "base",
        "Base",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_TIMER] = {
        "timers",
        "Timers",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_CALLBACK] = {
        "callback",
        "Callbacks",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_FRAME_RX] = {
        "rx",
        "Frame Rx",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_FRAME_TX] = {
        "tx",
        "Frame Tx",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_ICLI] = {
        "icli",
        "CLI printout",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_API] = {
        "api",
        "Switch API printout",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_CFM] = {
        "cfm",
        "CFM callbacks/notifications",
        VTSS_TRACE_LVL_ERROR
    },
    [APS_TRACE_GRP_ACL] = {
        "acl",
        "ACL Updates",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&APS_trace_reg, APS_trace_grps);

critd_t                             APS_crit;
static uint32_t                     APS_cap_port_cnt;
static aps_map_t                    APS_map;
static vtss_appl_aps_capabilities_t APS_cap;
static vtss_appl_aps_conf_t         APS_default_conf;
static mesa_etype_t                 APS_s_custom_tpid;
static bool                         APS_started;
static aps_timer_t                  APS_apply_config_timer;
static CapArray<aps_port_state_t, MEBA_CAP_BOARD_PORT_COUNT> APS_port_state;

//*****************************************************************************/
// APS_sf_trigger_get()
//*****************************************************************************/
static vtss_appl_aps_sf_trigger_t APS_sf_trigger_get(aps_state_t *aps_state, bool working)
{
    vtss_appl_aps_port_conf_t    &port_conf = working ? aps_state->conf.w_port_conf : aps_state->conf.p_port_conf;
    vtss_appl_aps_oper_warning_t w;

    if (port_conf.sf_trigger == VTSS_APPL_APS_SF_TRIGGER_LINK) {
        return VTSS_APPL_APS_SF_TRIGGER_LINK;
    }

    // Configured for using a MEP for SF triggering, but it could be that the
    // MEP is in such a shape that we cannot use it, so that we have to revert
    // to using link-state. This depends on the operational warnings that we
    // have by now.
    w = aps_state->status.oper_warning;
    if (w == VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_FOUND               ||
        w == VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_FOUND               ||
        w == VTSS_APPL_APS_OPER_WARNING_WMEP_ADMIN_DISABLED          ||
        w == VTSS_APPL_APS_OPER_WARNING_PMEP_ADMIN_DISABLED          ||
        w == VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_DOWN_MEP            ||
        w == VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_DOWN_MEP            ||
        w == VTSS_APPL_APS_OPER_WARNING_WMEP_AND_PORT_IFINDEX_DIFFER ||
        w == VTSS_APPL_APS_OPER_WARNING_PMEP_AND_PORT_IFINDEX_DIFFER) {
        // Those are the only operational warnings where we cannot use the MEP
        // for SF triggering and must fall back on link.
        return VTSS_APPL_APS_SF_TRIGGER_LINK;
    }

    return VTSS_APPL_APS_SF_TRIGGER_MEP;
}

//*****************************************************************************/
// APS_sf_get()
//*****************************************************************************/
static mesa_rc APS_sf_get(aps_state_t *aps_state, bool working, bool &sf)
{
    vtss_appl_aps_sf_trigger_t              sf_trigger;
    aps_port_state_t                        *port_state;
    vtss_appl_cfm_mep_key_t                 mep_key;
    vtss_appl_cfm_mep_notification_status_t notif_status;
    mesa_rc                                 rc;

    sf_trigger = APS_sf_trigger_get(aps_state, working);

    switch (sf_trigger) {
    case VTSS_APPL_APS_SF_TRIGGER_LINK:
        port_state = working ? aps_state->w_port_state : aps_state->p_port_state;
        sf = !port_state->link;
        T_I("%u: %c-port %u: SF = %d", aps_state->inst, working ? 'W' : 'P', port_state->port_no, sf);
        return VTSS_RC_OK;

    case VTSS_APPL_APS_SF_TRIGGER_MEP:
        mep_key = working ? aps_state->conf.w_port_conf.mep : aps_state->conf.p_port_conf.mep;
        if ((rc = vtss_appl_cfm_mep_notification_status_get(mep_key, &notif_status)) != VTSS_RC_OK) {
            // We also get notifications whenever a MEP gets deleted, so if this
            // function returns something != VTSS_RC_OK, it's probably the
            // reason. In that case, we will also get a MEP configuration change
            // callback,  which will take proper action
            // (APS_cfm_conf_change_callback()).
            T_I("%u: %c-MEP %s: Unable to get status: %s", aps_state->inst, working ? 'W' : 'P', mep_key, error_txt(rc));
            sf = true;
        } else {
            sf = !notif_status.mep_ok;
        }

        T_I("%u: %c-MEP %s: SF = %d", aps_state->inst, working ? 'W' : 'P', mep_key, sf);
        return rc;

    default:
        T_E("%u: Unknown SF trigger (%d)", aps_state->inst, sf_trigger);
        return VTSS_APPL_APS_RC_INTERNAL_ERROR;
    }
}

//*****************************************************************************/
// APS_sf_update()
//*****************************************************************************/
static void APS_sf_update(aps_state_t *aps_state, bool working)
{
    bool sf;

    (void)APS_sf_get(aps_state, working, sf);
    T_IG(APS_TRACE_GRP_CFM, "%u: Setting %c-sf = %d", aps_state->inst, working ? 'W' : 'P', sf);
    aps_base_sf_sd_set(aps_state, working, sf, false);
}

//*****************************************************************************/
// APS_cfm_change_notifications.
// Snoops on changes to the CFM MEP change notification structure to be able
// to react to changes in the MEP status (SF).
/******************************************************************************/
static struct aps_cfm_change_notifications_t : public vtss::notifications::EventHandler {
    vtss::notifications::Event              e;
    cfm_mep_notification_status_t::Observer o;

    aps_cfm_change_notifications_t() : EventHandler(&vtss::notifications::subject_main_thread), e(this)
    {
    }

    void init()
    {
        cfm_mep_notification_status.observer_new(&e);
    }

    void execute(vtss::notifications::Event *event)
    {
        vtss_appl_aps_port_conf_t *port_conf;
        aps_itr_t                 itr;
        aps_state_t               *aps_state;
        bool                      working;
        int                       i;

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
            T_DG(APS_TRACE_GRP_CFM, "%s: %s event", i->first, vtss::notifications::EventType::txt[vtss::notifications::EventType::E(i->second)].valueName);
        }

        APS_LOCK_SCOPE();

        if (!APS_started) {
            // Defer these change notifications until we have told the base about
            // all APS instances after boot.
            return;
        }

        // Loop through all APS instances
        for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
            aps_state = &itr->second;

            if (aps_state->status.oper_state != VTSS_APPL_APS_OPER_STATE_ACTIVE) {
                // Don't care about MEP changes on this instance, since it's not
                // active.
                continue;
            }

            for (i = 0; i < 2; i++) {
                working = i == 0;

                port_conf = working ? &aps_state->conf.w_port_conf : &aps_state->conf.p_port_conf;

                if (port_conf->sf_trigger != VTSS_APPL_APS_SF_TRIGGER_MEP) {
                    // Not using MEP-triggered SF.
                    continue;
                }

                if (o.events.find(port_conf->mep) == o.events.end()) {
                    // No events on this port's MEP
                    continue;
                }

                APS_sf_update(aps_state, working);
            }
        }
    }
} APS_cfm_change_notifications;

/******************************************************************************/
// APS_base_activate()
/******************************************************************************/
static mesa_rc APS_base_activate(aps_state_t *aps_state)
{
    bool w_sf, p_sf;

    // Gotta find the initial values of signal fail for W and P.
    VTSS_RC(APS_sf_get(aps_state, true,  w_sf));
    VTSS_RC(APS_sf_get(aps_state, false, p_sf));

    return aps_base_activate(aps_state, w_sf, p_sf);
}

/******************************************************************************/
// APS_capabilities_set()
/******************************************************************************/
static void APS_capabilities_set(void)
{
    // Two APS instances that use the same Working Port are illegal, because if
    // the working port goes down, the two instances will race to move the
    // traffic to their protecting port (the whole port is moved in APS).
    // This means that we cannot create more APS instances than there are ports
    // on the device.
    APS_cap.inst_cnt_max = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);

    APS_cap.wtr_secs_max       =   720; // 12 minutes
    APS_cap.hold_off_msecs_max = 10000; // 10 seconds.
}

//*****************************************************************************/
// APS_default_conf_set()
/******************************************************************************/
static void APS_default_conf_set(void)
{
    vtss_clear(APS_default_conf);

    APS_default_conf.level                  = 0;
    APS_default_conf.vlan                   = 0; // Untagged
    APS_default_conf.pcp                    = 7;
    APS_default_conf.mode                   = VTSS_APPL_APS_MODE_ONE_FOR_ONE;
    APS_default_conf.tx_aps                 = false;
    APS_default_conf.admin_active           = false;
    APS_default_conf.revertive              = false;
    APS_default_conf.wtr_secs               = 300; // seconds
    APS_default_conf.hold_off_msecs         = 0;   // Hold-off disabled
    APS_default_conf.w_port_conf.sf_trigger = VTSS_APPL_APS_SF_TRIGGER_LINK;
    APS_default_conf.p_port_conf.sf_trigger = VTSS_APPL_APS_SF_TRIGGER_LINK;

    if (vtss_ifindex_from_port(VTSS_ISID_LOCAL, VTSS_PORT_NO_START, &APS_default_conf.w_port_conf.ifindex) != VTSS_RC_OK) {
        T_E("Unable to convert <isid, port> = <%u, %u> to ifindex", VTSS_ISID_LOCAL, VTSS_PORT_NO_START);
    }

    APS_default_conf.p_port_conf.ifindex = APS_default_conf.w_port_conf.ifindex;
}

/******************************************************************************/
// APS_tx_frame_update()
/******************************************************************************/
static void APS_tx_frame_update(aps_state_t *aps_state)
{
    if (!APS_started) {
        // Defer all aps_base calls until we are all set after boot
        return;
    }

    if (aps_state->status.oper_state == VTSS_APPL_APS_OPER_STATE_ACTIVE && aps_state->tx_laps_pdus()) {
        // The contents of a LAPS PDU has changed. Get it updated.
        aps_base_tx_frame_update(aps_state);
    }
}

/******************************************************************************/
// APS_matching_update()
/******************************************************************************/
static void APS_matching_update(aps_state_t *aps_state)
{
    if (!APS_started) {
        // Defer all aps_base calls until we are all set after boot
        return;
    }

    if (aps_state->status.oper_state == VTSS_APPL_APS_OPER_STATE_ACTIVE) {
        // The ACE rules may require an update
        (void)aps_base_matching_update(aps_state);
    }
}

/******************************************************************************/
// APS_vlan_custom_etype_change_callback()
/******************************************************************************/
static void APS_vlan_custom_etype_change_callback(mesa_etype_t tpid)
{
    aps_itr_t        itr;
    mesa_port_no_t   port_no;
    aps_port_state_t *port_state;
    aps_state_t      *aps_state;

    T_IG(APS_TRACE_GRP_CALLBACK, "S-Custom TPID: 0x%04x -> 0x%04x", APS_s_custom_tpid, tpid);

    APS_LOCK_SCOPE();

    if (APS_s_custom_tpid == tpid) {
        return;
    }

    APS_s_custom_tpid = tpid;

    for (port_no = 0; port_no < APS_cap_port_cnt; port_no++) {
        port_state = &APS_port_state[port_no];

        if (port_state->port_type != VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM) {
            continue;
        }

        port_state->tpid = APS_s_custom_tpid;

        // Loop across all APS instances and see if they use this port state.
        // Only protect port states are interesting, because they may use the
        // new TPID for LAPS PDUs.
        for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
            aps_state = &itr->second;

            if (aps_state->p_port_state == port_state) {
                APS_tx_frame_update(aps_state);
            }
        }
    }
}

/******************************************************************************/
// APS_vlan_port_conf_change_callback()
/******************************************************************************/
static void APS_vlan_port_conf_change_callback(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *new_vlan_conf)
{
    aps_itr_t        itr;
    aps_port_state_t *port_state = &APS_port_state[port_no];
    aps_state_t      *aps_state;
    mesa_etype_t     new_tpid;
    bool             port_type_changed, pvid_changed, tpid_changed;

    APS_LOCK_SCOPE();

    T_DG(APS_TRACE_GRP_CALLBACK, "port_no = %u: pvid: %u -> %u, port_type: %s -> %s",
         port_no,
         port_state->pvid, new_vlan_conf->pvid,
         vlan_mgmt_port_type_to_txt(port_state->port_type),
         vlan_mgmt_port_type_to_txt(new_vlan_conf->port_type));

    port_type_changed = port_state->port_type != new_vlan_conf->port_type;
    pvid_changed      = port_state->pvid      != new_vlan_conf->pvid;

    if (!port_type_changed && !pvid_changed) {
        // Nothing more to do. We got invoked because of another VLAN port
        // configuration change.
        return;
    }

    port_state->port_type = new_vlan_conf->port_type;
    port_state->pvid      = new_vlan_conf->pvid;

    switch (port_state->port_type) {
    case VTSS_APPL_VLAN_PORT_TYPE_UNAWARE:
    case VTSS_APPL_VLAN_PORT_TYPE_C:
        new_tpid = VTSS_APPL_VLAN_C_TAG_ETHERTYPE;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S:
        new_tpid = 0x88a8;
        break;

    case VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM:
        new_tpid = APS_s_custom_tpid;
        break;

    default:
        T_E("Unknown port_type (%d)", port_state->port_type);
        new_tpid = port_state->tpid;
        break;
    }

    tpid_changed = port_state->tpid != new_tpid;
    port_state->tpid = new_tpid;

    // Loop across all APS instances and see if they use this port state.
    // Protect port states are of interest, because they may use the new TPID
    // for L-APS PDUs, but working port states are also of interest, because
    // if the APS instance is untagged and the PVID is changed, we may need to
    // update the ACE rules.
    for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
        aps_state = &itr->second;

        if (tpid_changed && aps_state->p_port_state == port_state) {
            // Tx frames only use TPID, so unless that one is changed, we don't
            // invoke APS_tx_frame_update().
            APS_tx_frame_update(aps_state);
        }

        if (aps_state->p_port_state == port_state || aps_state->w_port_state == port_state) {
            // May require ACE update
            APS_matching_update(aps_state);
        }
    }
}

/******************************************************************************/
// APS_link_state_change()
// In case we don't base the protection switching on MEPs, we can do it simply
// based on our own port states. This will, however, only work for link-to-link
// protections, not across a network.
/******************************************************************************/
static void APS_link_state_change(const char *func, mesa_port_no_t port_no, bool link)
{
    aps_state_t      *aps_state;
    aps_port_state_t *port_state;
    aps_itr_t        itr;
    bool             working;

    if (port_no >= APS_cap_port_cnt) {
        T_EG(APS_TRACE_GRP_CALLBACK, "Invalid port_no (%u). Max = %u", port_no, APS_cap_port_cnt);
        return;
    }

    port_state = &APS_port_state[port_no];

    T_DG(APS_TRACE_GRP_CALLBACK, "%s: port_no = %u: Link state: %d -> %d", func, port_no, port_state->link, link);

    if (link == port_state->link) {
        return;
    }

    port_state->link = link;

    if (!APS_started) {
        // Defer all aps_base updates until we are all set after a boot.
        return;
    }

    for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
        aps_state = &itr->second;

        if (aps_state->status.oper_state != VTSS_APPL_APS_OPER_STATE_ACTIVE) {
            // This APS instance is not active, so it doesn't care.
            continue;
        }

        if (aps_state->p_port_state != port_state && aps_state->w_port_state != port_state) {
            // This APS instance is not using this port, so it doesn't care.
            continue;
        }

        working = port_state == aps_state->w_port_state;

        if (!link || APS_sf_trigger_get(aps_state, working) == VTSS_APPL_APS_SF_TRIGGER_LINK) {
            // This port is either using link as SF trigger or the link went
            // down. That is, we cannot use link-up when SF-trigger is MEP. In
            // that case we need to wait for the MEP to signal non-sf.
            aps_base_sf_sd_set(aps_state, working, !link, false /* We can't set signal degrade */);
        }
    }
}

/******************************************************************************/
// APS_port_link_state_change_callback()
/******************************************************************************/
static void APS_port_link_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    APS_LOCK_SCOPE();
    APS_link_state_change(__FUNCTION__, port_no, status->link);
}

/******************************************************************************/
// APS_port_shutdown_callback()
/******************************************************************************/
static void APS_port_shutdown_callback(mesa_port_no_t port_no)
{
    APS_LOCK_SCOPE();
    APS_link_state_change(__FUNCTION__, port_no, false);
}

/******************************************************************************/
// APS_port_interrupt_callback()
/******************************************************************************/
static void APS_port_interrupt_callback(meba_event_t source_id, u32 port_no)
{
    mesa_rc rc;

    APS_LOCK_SCOPE();

    // We need to re-register for link-state-changes
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_APS, APS_port_interrupt_callback, source_id, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_EG(APS_TRACE_GRP_CALLBACK, "vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    APS_link_state_change(__FUNCTION__, port_no, false);
}

/******************************************************************************/
// APS_frame_rx_callback()
/******************************************************************************/
static BOOL APS_frame_rx_callback(void *contxt, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    mesa_etype_t etype;
    aps_itr_t    itr;
    aps_state_t  *aps_state;
    bool         working;
    uint8_t      opcode;

    // As long as we can't receive a frame behind two tags, the frame is always
    // normalized, so that the CFM Ethertype comes at frm[12] and frm[13],
    // because the packet module strips the outer tag.
    etype  = frm[12] << 8 | frm[13];
    opcode = frm[15];

    if (opcode != APS_OPCODE_LAPS) {
        // Not for us.
        T_NG(APS_TRACE_GRP_FRAME_RX, "Opcode = %u not for us, which is %u", opcode, APS_OPCODE_LAPS);
        return FALSE; // Not consumed
    }

    T_DG(APS_TRACE_GRP_FRAME_RX, "Rx frame on port_no = %u of length %u excl. FCS. EtherType = 0x%04x, opcode = %u, iflow_id = %u, hints = 0x%x, class-tag: tpid = 0x%04x, vid = %u, pcp = %u, dei = %u, stripped_tag: tpid = 0x%04x, vid = %u, pcp = %u, dei = %u",
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

    T_NG_HEX(APS_TRACE_GRP_FRAME_RX, frm, rx_info->length);

    if (etype != CFM_ETYPE) {
        // Why did we receive this frame? Our Packet Rx Filter tells the packet
        // module only to send us frames with CFM EtherType.
        T_EG(APS_TRACE_GRP_FRAME_RX, "Rx frame on port_no %u with etype = 0x%04x, which isn't the CFM EtherType", rx_info->port_no, etype);

        // Not consumed
        return FALSE;
    }

    APS_LOCK_SCOPE();

    if (!APS_started) {
        // Don't forward frames to the base until we are all set after a boot
        T_IG(APS_TRACE_GRP_FRAME_RX, "Skipping Rx frame, because we're not ready yet");
        return FALSE;
    }

    // Loop through all APS states and find - amongst the active APSs - the one
    // that matches this frame.
    for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
        aps_state = &itr->second;

        if (aps_state->status.oper_state != VTSS_APPL_APS_OPER_STATE_ACTIVE) {
            continue;
        }

        // Match on ingress port and classified VLAN.
        // We may receive APS PDUs on both working and protect port.
        if (aps_state->w_port_state->port_no != rx_info->port_no && aps_state->p_port_state->port_no != rx_info->port_no) {
            continue;
        }

        working = rx_info->port_no == aps_state->w_port_state->port_no;

        if (aps_state->classified_vid_get(working) != rx_info->tag.vid) {
            // The APS instance's expected classified VID must match the frame's
            // classified VID, but it doesn't, so not for us.
            continue;
        }

        if (aps_state->is_tagged()) {
            // This is a tagged APS instance
            if (!rx_info->stripped_tag.tpid) {
                // There must be a stripped tag in the frame, but there isn't.
                continue;
            }
        } else {
            // This is a an untagged APS instance. We don't accept frames with tags.
            if (rx_info->stripped_tag.tpid) {
                continue;
            }
        }

        T_DG(APS_TRACE_GRP_FRAME_RX, "%u handles the frame", itr->first);
        T_NG_HEX(APS_TRACE_GRP_FRAME_RX, frm, rx_info->length);
        aps_base_rx_frame(aps_state, frm, rx_info);

        // Consumed
        return TRUE;
    }

    // Not consumed
    return FALSE;
}

/******************************************************************************/
// APS_port_state_init()
/******************************************************************************/
static void APS_port_state_init(void)
{
    aps_port_state_t *port_state;
    mesa_port_no_t   port_no;
    mesa_rc          rc;

    {
        APS_LOCK_SCOPE();

        APS_s_custom_tpid = VTSS_APPL_VLAN_CUSTOM_S_TAG_DEFAULT;

        for (port_no = 0; port_no < APS_cap_port_cnt; port_no++) {
            port_state                    = &APS_port_state[port_no];
            port_state->port_no           = port_no;
            port_state->tpid              = VTSS_APPL_VLAN_C_TAG_ETHERTYPE;
            port_state->pvid              = VTSS_APPL_VLAN_ID_DEFAULT;
            port_state->port_type         = VTSS_APPL_VLAN_PORT_TYPE_C;
            port_state->link              = false;
            (void)conf_mgmt_mac_addr_get(port_state->smac.addr, port_no + 1);
        }
    }

    // Don't take our own mutex during callback registrations (see comment in
    // similar function in cfm.cxx)

    // Subscribe to Custom S-tag TPID changes in the VLAN module
    vlan_s_custom_etype_change_register(VTSS_MODULE_ID_APS, APS_vlan_custom_etype_change_callback);

    // Subscribe to VLAN port changes in the VLAN module
    vlan_port_conf_change_register(VTSS_MODULE_ID_APS, APS_vlan_port_conf_change_callback, TRUE);

    // Subscribe to link changes in the Port module
    if ((rc = port_change_register(VTSS_MODULE_ID_APS, APS_port_link_state_change_callback)) != VTSS_RC_OK) {
        T_E("port_change_register() failed: %s", error_txt(rc));
    }

    // Subscribe to 'shutdown' commands
    if ((rc = port_shutdown_register(VTSS_MODULE_ID_APS, APS_port_shutdown_callback)) != VTSS_RC_OK) {
        T_E("port_shutdown_register() failed: %s", error_txt(rc));
    }

    // Subscribe to FLNK and LOS interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_APS, APS_port_interrupt_callback, MEBA_EVENT_FLNK, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    // Subscribe to LoS interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_APS, APS_port_interrupt_callback, MEBA_EVENT_LOS, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }
}

/******************************************************************************/
// APS_frame_rx_register()
/******************************************************************************/
static void APS_frame_rx_register(void)
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
    filter.modid = VTSS_MODULE_ID_APS;
    filter.match = PACKET_RX_FILTER_MATCH_ETYPE;
    filter.cb    = APS_frame_rx_callback;
    filter.prio  = PACKET_RX_FILTER_PRIO_HIGH; // Get the frames before CFM
    filter.etype = CFM_ETYPE;
    if ((rc = packet_rx_filter_register(&filter, &filter_id)) != VTSS_RC_OK) {
        T_E("packet_rx_filter_register() failed: %s", error_txt(rc));
    }
}

/******************************************************************************/
// APS_ptr_check()
/******************************************************************************/
static mesa_rc APS_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_APS_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// APS_inst_check()
// Range [1-APS_cap.inst_cnt_max]
/******************************************************************************/
static mesa_rc APS_inst_check(uint32_t inst)
{
    if (inst < 1 || inst > APS_cap.inst_cnt_max) {
        return VTSS_APPL_APS_RC_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// APS_ifindex_to_port()
/******************************************************************************/
static mesa_rc APS_ifindex_to_port(vtss_ifindex_t ifindex, bool working, mesa_port_no_t &port_no)
{
    vtss_ifindex_elm_t ife;

    // Check that we can decompose the ifindex and that it's a port.
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        port_no = VTSS_PORT_NO_NONE;
        return working ? VTSS_APPL_APS_RC_INVALID_W_IFINDEX : VTSS_APPL_APS_RC_INVALID_P_IFINDEX;
    }

    port_no = ife.ordinal;
    return VTSS_RC_OK;
}

/******************************************************************************/
// APS_mep_info_get()
/******************************************************************************/
static bool APS_mep_info_get(aps_state_t *aps_state, bool working)
{
    vtss_appl_cfm_md_conf_t   md_conf;
    vtss_appl_cfm_ma_conf_t   ma_conf;
    vtss_appl_cfm_mep_conf_t  mep_conf;
    vtss_appl_aps_port_conf_t &port_conf = working ? aps_state->conf.w_port_conf : aps_state->conf.p_port_conf;

    // If we are using a MEP for SF, that MEP must have certain properties
    if (port_conf.sf_trigger != VTSS_APPL_APS_SF_TRIGGER_MEP) {
        // Not using MEP as SF trigger, so nothing more to do.
        return true;
    }

    // MEP found?
    if (vtss_appl_cfm_ma_conf_get( port_conf.mep, &ma_conf)  != VTSS_RC_OK ||
        vtss_appl_cfm_md_conf_get( port_conf.mep, &md_conf)  != VTSS_RC_OK ||
        vtss_appl_cfm_mep_conf_get(port_conf.mep, &mep_conf) != VTSS_RC_OK) {
        aps_state->status.oper_warning = working ? VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_FOUND : VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_FOUND;
        return false;
    }

    // MEP administratively enabled?
    if (!mep_conf.admin_active) {
        aps_state->status.oper_warning = working ? VTSS_APPL_APS_OPER_WARNING_WMEP_ADMIN_DISABLED : VTSS_APPL_APS_OPER_WARNING_PMEP_ADMIN_DISABLED;
        return false;
    }

    // MEP's direction is down?
    if (mep_conf.direction != VTSS_APPL_CFM_DIRECTION_DOWN) {
        aps_state->status.oper_warning = working ? VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_DOWN_MEP : VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_DOWN_MEP;
        return false;
    }

    // MEP's ifindex must be the same as this port's ifindex
    if (mep_conf.ifindex != port_conf.ifindex) {
        aps_state->status.oper_warning = working ? VTSS_APPL_APS_OPER_WARNING_WMEP_AND_PORT_IFINDEX_DIFFER : VTSS_APPL_APS_OPER_WARNING_PMEP_AND_PORT_IFINDEX_DIFFER;
        return false;
    }

    // Everything is OK for this MEP
    return true;
}

/******************************************************************************/
// APS_oper_state_update_single()
// There can happen changes to the MEPs' configuration that affect the APS
// instance's ability to work. This function is invoked when MEP configuration
// or APS configuration changes to figure out whether a given APS instance can
// now be operationally up.
/******************************************************************************/
static void APS_oper_state_update_single(aps_state_t *aps_state)
{
    static mesa_mac_t zero_mac;
    aps_port_state_t  **port_state;
    vtss_ifindex_t    ifindex;
    mesa_port_no_t    port_no;
    bool              working;
    int               i;

    T_I("%u, Enter", aps_state->inst);

    // Assume it's operational active...
    aps_state->status.oper_state = VTSS_APPL_APS_OPER_STATE_ACTIVE;

    // ...and assume no warnings.
    aps_state->status.oper_warning = VTSS_APPL_APS_OPER_WARNING_NONE;

    // This APS instance administratively enabled?
    if (!aps_state->conf.admin_active) {
        aps_state->status.oper_state = VTSS_APPL_APS_OPER_STATE_ADMIN_DISABLED;
        return;
    }

    for (i = 0; i < 2; i++) {
        working = i == 0;

        ifindex    = working ? aps_state->conf.w_port_conf.ifindex : aps_state->conf.p_port_conf.ifindex;
        port_state = working ? &aps_state->w_port_state            : &aps_state->p_port_state;

        // The configuration's ifindex has already been checked by
        // vtss_appl_aps_conf_set(), so no need to check return value.
        (void)APS_ifindex_to_port(ifindex, working, port_no);
        *port_state = &APS_port_state[port_no];
    }

    // Update our own state with SMAC.
    // operator== on mesa_mac_t is from ip_utils.hxx.
    if (aps_state->conf.smac == zero_mac) {
        // SMAC is not overridden. Use Port MAC.
        aps_state->smac = aps_state->p_port_state->smac;
    } else {
        // SMAC is overridden by configuration. Use that one.
        aps_state->smac = aps_state->conf.smac;
    }

    // Time to check for operational warnings

    // Check per-port warnings. We stop at the first operational warning.
    for (i = 0; i < 2; i++) {
        working = i == 0;

        // Check to see if MEP is valid and active (if using MEP as SF trigger)
        if (!APS_mep_info_get(aps_state, working)) {
            return;
        }
    }
}

/******************************************************************************/
// APS_oper_state_update_all()
// There can happen changes to the MEPs' configuration that affect the APS
// instance's ability to work. This function is invoked when MEP configuration
// or APS configuration changes to figure out whether a given APS instance can
// now be operational up.
//
// The nature of this function will favor those APS instances with higher
// instance numbers if there are APS cross-checks that fail.
/******************************************************************************/
static void APS_oper_state_update_all(void)
{
    aps_state_t *aps_state;
    aps_itr_t   itr, itr2;
    bool        change_in_laps_pdus;

    if (!APS_started) {
        // Defer all aps_base updates until after the CFM module is up and
        // running after a boot.
        return;
    }

    // First update the operational state of all APS instances based on each APS
    // instance's own configuration.
    for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
        // Notice that we don't react on changed working MEP VID and working MEP
        // level, because these are only used in APS_frame_rx_callback() in case
        // we receive LAPS PDUs on the working port.
        itr->second.old_oper_state   = itr->second.status.oper_state;
        itr->second.old_oper_warning = itr->second.status.oper_warning;
        itr->second.old_w_port_no    = itr->second.w_port_state == nullptr ? VTSS_PORT_NO_NONE : itr->second.w_port_state->port_no;
        itr->second.old_p_port_no    = itr->second.p_port_state == nullptr ? VTSS_PORT_NO_NONE : itr->second.p_port_state->port_no;
        itr->second.old_smac         = itr->second.smac;
        APS_oper_state_update_single(&itr->second);
    }

    // Loop a second time, where we deactivate instances that are operational
    // down and activate instances that are operational up.
    for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
        aps_state = &itr->second;

        // Check if the operational state has changed.
        if (aps_state->old_oper_state != aps_state->status.oper_state) {
            // Operational state has changed.
            if (aps_state->status.oper_state == VTSS_APPL_APS_OPER_STATE_ACTIVE) {
                // Inactive-to-active
                if (APS_base_activate(aps_state) != VTSS_RC_OK) {
                    aps_state->status.oper_state = VTSS_APPL_APS_OPER_STATE_INTERNAL_ERROR;
                }
            } else if (aps_state->old_oper_state == VTSS_APPL_APS_OPER_STATE_ACTIVE) {
                // Active-to-inactive
                (void)aps_base_deactivate(aps_state);
            }
        } else if (aps_state->status.oper_state == VTSS_APPL_APS_OPER_STATE_ACTIVE) {
            // Active-to_active.
            // Check if any of the parameters affecting APS port configuration
            // has changed. If so, we need to deactivate and re-activate it.
            if (aps_state->old_w_port_no != itr->second.w_port_state->port_no ||
                aps_state->old_p_port_no != itr->second.p_port_state->port_no ||
                aps_state->old_conf.mode != itr->second.conf.mode ||
                (aps_state->conf.mode == VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL && aps_state->old_conf.tx_aps != aps_state->conf.tx_aps)) {
                // The configuration has changed. First deactivate the old, then
                // activate the new.
                (void)aps_base_deactivate(&itr->second);
                if (APS_base_activate(&itr->second) != VTSS_RC_OK) {
                    aps_state->status.oper_state = VTSS_APPL_APS_OPER_STATE_INTERNAL_ERROR;
                }
            } else {
                // If any of the parameters affecting LAPS PDU contents has
                // changed, we need to get base to update the LAPS PDUs it is
                // sending.
                change_in_laps_pdus = false;
                if (memcmp(aps_state->old_smac.addr, aps_state->smac.addr, sizeof(aps_state->old_smac)) ||
                    aps_state->old_conf.level != aps_state->conf.level                                  ||
                    aps_state->old_conf.vlan  != aps_state->conf.vlan) {
                    change_in_laps_pdus = true;
                } else if (aps_state->is_tagged() && aps_state->old_conf.pcp != aps_state->conf.pcp) {
                    // Change in PCP value only affects the PDU if sending
                    // tagged
                    change_in_laps_pdus = true;
                }

                if (change_in_laps_pdus) {
                    APS_tx_frame_update(aps_state);
                }

                if (APS_started && aps_state->old_conf.revertive != aps_state->conf.revertive) {
                    aps_base_exercise_sm(aps_state);
                }

                // If we are using a different signal fail source now, go and
                // update SF.
                APS_sf_update(aps_state, false);
                APS_sf_update(aps_state, true);
            }
        }

        // Now we are in sync.
        itr->second.old_conf = itr->second.conf;
    }
}

/******************************************************************************/
// APS_cfm_conf_change_callback()
/******************************************************************************/
static void APS_cfm_conf_change_callback(void)
{
    T_IG(APS_TRACE_GRP_CFM, "CFM conf changed");
    // Some configuration has changed in the CFM module. Check if this has any
    // influence on the APS instances.
    APS_LOCK_SCOPE();
    APS_oper_state_update_all();
}

/******************************************************************************/
// APS_conf_update()
/******************************************************************************/
static mesa_rc APS_conf_update(aps_itr_t itr, const vtss_appl_aps_conf_t *new_conf)
{
    aps_itr_t itr2;

    // We need to save a copy of the old configuration in case of deactivation
    // and other changes that may cause base updates.
    itr->second.old_conf = itr->second.conf;

    // The state's #inst member is only used for tracing.
    itr->second.inst = itr->first;

    // Update our internal configuration
    itr->second.conf = *new_conf;

    // With this new configuration, it could be that other APS instances become
    // inactive or active.
    APS_oper_state_update_all();

    return VTSS_RC_OK;
}

//*****************************************************************************/
// APS_port_conf_chk()
/******************************************************************************/
static mesa_rc APS_port_conf_chk(const vtss_appl_aps_conf_t *conf, bool working)
{
    const vtss_appl_aps_port_conf_t *port_conf;
    mesa_port_no_t                  port_no;
    bool                            empty;

    port_conf = working ? &conf->w_port_conf : &conf->p_port_conf;

    if (port_conf->sf_trigger != VTSS_APPL_APS_SF_TRIGGER_LINK && port_conf->sf_trigger != VTSS_APPL_APS_SF_TRIGGER_MEP) {
        return working ? VTSS_APPL_APS_RC_INVALID_SF_TRIGGER_W : VTSS_APPL_APS_RC_INVALID_SF_TRIGGER_P;
    }

    // Check that the ifindex is representing a port.
    VTSS_RC(APS_ifindex_to_port(port_conf->ifindex, working, port_no));

    if (port_conf->sf_trigger == VTSS_APPL_APS_SF_TRIGGER_MEP) {
        // Figure out whether the user has remembered to fill in the MEP key.
        VTSS_RC(cfm_util_key_check(port_conf->mep, &empty));

        if (!empty) {
            // The MEP key is not empty, so its contents must be valid.
            VTSS_RC(cfm_util_key_check(port_conf->mep));
        } else if (conf->admin_active) {
            // The MEP key cannot be empty when SF trigger is MEP and we are
            // administratively enabled.
            return working ? VTSS_APPL_APS_RC_WMEP_MUST_BE_SPECIFIED : VTSS_APPL_APS_RC_PMEP_MUST_BE_SPECIFIED;
        }
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// APS_apply_config_for_the_first_time()
/******************************************************************************/
static void APS_apply_config_for_the_first_time(aps_timer_t &timer, void *context)
{
    APS_LOCK_ASSERT_LOCKED("APS mutex not locked");

    T_I("Applying config to base for the first time after boot");

    APS_started = true;

    APS_oper_state_update_all();
}

//*****************************************************************************/
// APS_default()
/******************************************************************************/
static void APS_default(void)
{
    vtss_appl_aps_conf_t new_conf;
    aps_itr_t            itr, next_itr;

    APS_LOCK_SCOPE();

    // Start by setting all APS instances inactive and call to update the
    // configuration. This will release the APS resources in both MESA and the
    // CFM module, so that we can erase all of it in one go afterwards.
    for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
        if (itr->second.conf.admin_active) {
            new_conf = itr->second.conf;
            new_conf.admin_active = false;
            (void)APS_conf_update(itr, &new_conf);
        }
    }

    // Then erase all elements from the map.
    APS_map.clear();
}

/******************************************************************************/
// vtss_appl_aps_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_aps_capabilities_get(vtss_appl_aps_capabilities_t *cap)
{
    VTSS_RC(APS_ptr_check(cap));
    *cap = APS_cap;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_aps_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_aps_conf_default_get(vtss_appl_aps_conf_t *conf)
{
    if (conf == nullptr) {
        return VTSS_APPL_APS_RC_INVALID_PARAMETER;
    }

    *conf = APS_default_conf;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_aps_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_aps_conf_get(uint32_t inst, vtss_appl_aps_conf_t *conf)
{
    aps_itr_t itr;

    VTSS_RC(APS_inst_check(inst));
    VTSS_RC(APS_ptr_check(conf));

    T_N("Enter, %u", inst);

    APS_LOCK_SCOPE();

    if ((itr = APS_map.find(inst)) == APS_map.end()) {
        return VTSS_APPL_APS_RC_NO_SUCH_INSTANCE;
    }

    *conf = itr->second.conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_aps_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_aps_conf_set(uint32_t inst, const vtss_appl_aps_conf_t *conf)
{
    vtss_appl_aps_conf_t local_conf;
    aps_itr_t            itr, itr2;
    char                 mac_str[18];

    VTSS_RC(APS_inst_check(inst));
    VTSS_RC(APS_ptr_check(conf));

    T_D("Enter, inst = %u, w.sf_trigger = %s, w.ifindex = %u, w.mep = %s, p.sf_trigger = %s, p.ifindex = %s, p.mep = %s, level = %u, vlan = %u, pcp = %u, smac = %s, mode = %s, tx_aps = %d, revertive = %d, wtr_secs = %u, hold_off_msecs = %u, admin_active = %d",
        inst,
        aps_util_sf_trigger_to_str(conf->w_port_conf.sf_trigger), VTSS_IFINDEX_PRINTF_ARG(conf->w_port_conf.ifindex), conf->w_port_conf.mep,
        aps_util_sf_trigger_to_str(conf->p_port_conf.sf_trigger), VTSS_IFINDEX_PRINTF_ARG(conf->p_port_conf.ifindex), conf->p_port_conf.mep,
        conf->level, conf->vlan, conf->pcp, misc_mac_txt(conf->smac.addr, mac_str),
        aps_util_mode_to_str(conf->mode), conf->tx_aps, conf->revertive, conf->wtr_secs, conf->hold_off_msecs,
        conf->admin_active);

    VTSS_RC(APS_port_conf_chk(conf, true));
    VTSS_RC(APS_port_conf_chk(conf, false));

    if (conf->mode != VTSS_APPL_APS_MODE_ONE_FOR_ONE                 &&
        conf->mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL &&
        conf->mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_BIDIRECTIONAL) {
        return VTSS_APPL_APS_RC_MODE;
    }

    if (conf->wtr_secs < 1 || conf->wtr_secs > APS_cap.wtr_secs_max) {
        return VTSS_APPL_APS_RC_WTR;
    }

    if (conf->hold_off_msecs > APS_cap.hold_off_msecs_max) {
        return VTSS_APPL_APS_RC_HOLD_OFF;
    }

    if (conf->hold_off_msecs % 100) {
        // Must be in steps of 100 milliseconds
        return VTSS_APPL_APS_RC_HOLD_OFF;
    }

    if (conf->level > 7) {
        return VTSS_APPL_APS_RC_INVALID_LEVEL;
    }

    if (conf->vlan > VTSS_APPL_VLAN_ID_MAX) {
        return VTSS_APPL_APS_RC_INVALID_VLAN;
    }

    if (conf->pcp > 7) {
        return VTSS_APPL_APS_RC_INVALID_PCP;
    }

    // Check that the MAC address is a unicast MAC address usable as SMAC.
    if (conf->smac.addr[0] & 0x1) {
        // SMAC must not be a multicast SMAC
        return VTSS_APPL_APS_RC_INVALID_SMAC;
    }

    // Check configuration.
    if (conf->admin_active) {
        if (conf->w_port_conf.ifindex == conf->p_port_conf.ifindex) {
            return VTSS_APPL_APS_RC_W_P_IFINDEX_IDENTICAL;
        }

        if (conf->w_port_conf.sf_trigger == VTSS_APPL_APS_SF_TRIGGER_MEP &&
            conf->p_port_conf.sf_trigger == VTSS_APPL_APS_SF_TRIGGER_MEP &&
            conf->w_port_conf.mep        == conf->p_port_conf.mep) {
            // The two ports cannot use the same MEP.
            return VTSS_APPL_APS_RC_W_P_MEP_IDENTICAL;
        }
    }

    // Time to normalize the configuration, that is, default unused
    // configuration. This is in order to avoid happening to re-use dead
    // configuration if that configuration becomes active again due to a change
    // in e.g. sf-trigger.
    local_conf = *conf;
    if (local_conf.admin_active) {
        if (local_conf.w_port_conf.sf_trigger == VTSS_APPL_APS_SF_TRIGGER_LINK) {
            local_conf.w_port_conf.mep = APS_default_conf.w_port_conf.mep;
        }

        if (local_conf.p_port_conf.sf_trigger == VTSS_APPL_APS_SF_TRIGGER_LINK) {
            local_conf.p_port_conf.mep = APS_default_conf.p_port_conf.mep;
        }
    }

    APS_LOCK_SCOPE();

    if ((itr = APS_map.find(inst)) != APS_map.end()) {
        if (memcmp(&local_conf, &itr->second, sizeof(local_conf)) == 0) {
            // No changes.
            return VTSS_RC_OK;
        }
    }

    // Check that we haven't created more instances than we can allow
    if (itr == APS_map.end()) {
        if (APS_map.size() >= APS_cap.inst_cnt_max) {
            return VTSS_APPL_APS_RC_LIMIT_REACHED;
        }
    }

    // Cross-APS checks
    if (local_conf.admin_active) {
        for (itr2 = APS_map.begin(); itr2 != APS_map.end(); ++itr2) {
            if (itr2 == itr) {
                // Don't check against our selves.
                continue;
            }

            if (!itr2->second.conf.admin_active) {
                continue;
            }

            // Check that no other administratively enabled APS instance is
            // using the same L-APS VLAN as we are on the same protect port,
            // because if so, the L-APS info (which might be different on the
            // two instances) will go to the same MEP on the receiving side,
            // which could confuse the receiver.
            if (itr2->second.conf.p_port_conf.ifindex == local_conf.p_port_conf.ifindex &&
                itr2->second.conf.vlan                == local_conf.vlan) {
                return VTSS_APPL_APS_RC_ANOTHER_INST_USES_SAME_P_PORT_AND_VLAN;
            }

            // Two APS instances that use the same working port are illegal,
            // because if W port goes down, then two APS instances will race to
            // move traffic to two different P ports.
            if (itr2->second.conf.w_port_conf.ifindex == local_conf.w_port_conf.ifindex) {
                return VTSS_APPL_APS_RC_ANOTHER_INST_USES_SAME_W_PORT;
            }
        }
    }

    // Create a new or update an existing entry
    if ((itr = APS_map.get(inst)) == APS_map.end()) {
        return VTSS_APPL_APS_RC_OUT_OF_MEMORY;
    }

    return APS_conf_update(itr, &local_conf);
}

/******************************************************************************/
// vtss_appl_aps_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_aps_conf_del(uint32_t inst)
{
    vtss_appl_aps_conf_t new_conf;
    aps_itr_t            itr;
    mesa_rc              rc;

    VTSS_RC(APS_inst_check(inst));

    T_D("Enter, %u", inst);

    APS_LOCK_SCOPE();

    if ((itr = APS_map.find(inst)) == APS_map.end()) {
        return VTSS_APPL_APS_RC_NO_SUCH_INSTANCE;
    }

    if (itr->second.conf.admin_active) {
        new_conf = itr->second.conf;
        new_conf.admin_active = false;

        // Back out of everything
        rc = APS_conf_update(itr, &new_conf);
    } else {
        rc = VTSS_RC_OK;
    }

    // Delete APS instance from our map
    APS_map.erase(inst);

    return rc;
}

/******************************************************************************/
// vtss_appl_aps_itr()
/******************************************************************************/
mesa_rc vtss_appl_aps_itr(const uint32_t *prev_inst, uint32_t *next_inst)
{
    aps_itr_t itr;

    VTSS_RC(APS_ptr_check(next_inst));

    APS_LOCK_SCOPE();

    if (prev_inst) {
        // Here we have a valid prev_inst. Find the next from that one.
        itr = APS_map.greater_than(*prev_inst);
    } else {
        // We don't have a valid prev_inst. Get the first.
        itr = APS_map.begin();
    }

    if (itr != APS_map.end()) {
        *next_inst = itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

//*****************************************************************************/
// vtss_appl_aps_control_get()
/******************************************************************************/
mesa_rc vtss_appl_aps_control_get(uint32_t inst, vtss_appl_aps_control_t *ctrl)
{
    aps_itr_t itr;

    VTSS_RC(APS_inst_check(inst));
    VTSS_RC(APS_ptr_check(ctrl));

    APS_LOCK_SCOPE();

    if ((itr = APS_map.find(inst)) == APS_map.end()) {
        return VTSS_APPL_APS_RC_NO_SUCH_INSTANCE;
    }

    memset(ctrl, 0, sizeof(*ctrl));

    ctrl->command = itr->second.command;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_aps_control_set()
/******************************************************************************/
mesa_rc vtss_appl_aps_control_set(uint32_t inst, const vtss_appl_aps_control_t *ctrl)
{
    aps_itr_t               itr;
    vtss_appl_aps_conf_t    *conf;
    vtss_appl_aps_command_t cur_cmd, new_cmd;

    VTSS_RC(APS_inst_check(inst));
    VTSS_RC(APS_ptr_check(ctrl));

    T_D("Enter, inst = %u, command = %s", inst, aps_util_command_to_str(ctrl->command));

    new_cmd = ctrl->command;

    if (new_cmd <= VTSS_APPL_APS_COMMAND_NR || new_cmd > VTSS_APPL_APS_COMMAND_FREEZE_CLEAR) {
        return VTSS_APPL_APS_RC_COMMAND;
    }

    APS_LOCK_SCOPE();

    if ((itr = APS_map.find(inst)) == APS_map.end()) {
        return VTSS_APPL_APS_RC_NO_SUCH_INSTANCE;
    }

    conf    = &itr->second.conf;
    cur_cmd = itr->second.command;

    if (!conf->admin_active) {
        return VTSS_APPL_APS_RC_NOT_ACTIVE;
    }

    if (new_cmd == cur_cmd) {
        return VTSS_RC_OK;
    }

    if (!APS_started) {
        return VTSS_APPL_APS_RC_NOT_READY_TRY_AGAIN_IN_A_FEW_SECONDS;
    }

    // Various invalid command values.
    // VTSS_APPL_APS_COMMAND_CLEAR is used to clear:
    //   LO, FS, MS-to-W, MS-to-P, EXER commands and WTR conditions.

    // Only way to get out of freeze is to clear the freeze
    if (cur_cmd == VTSS_APPL_APS_COMMAND_FREEZE && new_cmd != VTSS_APPL_APS_COMMAND_FREEZE_CLEAR) {
        return VTSS_APPL_APS_RC_COMMAND_FREEZE;
    }

    // It is not allowed to exercise in unidirectional mode.
    if (new_cmd == VTSS_APPL_APS_COMMAND_EXER && conf->mode == VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL) {
        return VTSS_APPL_APS_RC_COMMAND_EXERCISE_UNIDIRECTIONAL;
    }

    return aps_base_command_set(&itr->second, new_cmd);
}

//*****************************************************************************/
// vtss_appl_aps_status_get()
/******************************************************************************/
mesa_rc vtss_appl_aps_status_get(uint32_t inst, vtss_appl_aps_status_t *status)
{
    aps_itr_t itr;

    VTSS_RC(APS_inst_check(inst));
    VTSS_RC(APS_ptr_check(status));

    APS_LOCK_SCOPE();

    if ((itr = APS_map.find(inst)) == APS_map.end()) {
        return VTSS_APPL_APS_RC_NO_SUCH_INSTANCE;
    }

    *status = itr->second.status;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_aps_statistics_clear()
/******************************************************************************/
mesa_rc vtss_appl_aps_statistics_clear(uint32_t inst)
{
    aps_itr_t itr;

    VTSS_RC(APS_inst_check(inst));

    APS_LOCK_SCOPE();

    if ((itr = APS_map.find(inst)) == APS_map.end()) {
        return VTSS_APPL_APS_RC_NO_SUCH_INSTANCE;
    }

    aps_base_statistics_clear(&itr->second);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// aps_util_mode_to_str()
/******************************************************************************/
const char *aps_util_mode_to_str(vtss_appl_aps_mode_t mode)
{
    switch (mode) {
    case VTSS_APPL_APS_MODE_ONE_FOR_ONE:
        return "1-for-1";

    case VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL:
        return "unidirectional-1-plus-1";

    case VTSS_APPL_APS_MODE_ONE_PLUS_ONE_BIDIRECTIONAL:
        return "bidirectional-1-plus-1";

    default:
        T_E("Invalid mode (%d)", mode);
        return "unknown";
    }
}

//*****************************************************************************/
// aps_util_command_to_str()
/******************************************************************************/
const char *aps_util_command_to_str(vtss_appl_aps_command_t command)
{
    switch (command) {
    case VTSS_APPL_APS_COMMAND_NR:
        return "None";

    case VTSS_APPL_APS_COMMAND_LO:
        return "Lockout";

    case VTSS_APPL_APS_COMMAND_FS:
        return "Forced switch";

    case VTSS_APPL_APS_COMMAND_MS_TO_W:
        return "Manual-switch-to-W";

    case VTSS_APPL_APS_COMMAND_MS_TO_P:
        return "Manual-switch-to-P";

    case VTSS_APPL_APS_COMMAND_EXER:
        return "Exercise";

    case VTSS_APPL_APS_COMMAND_CLEAR:
        return "Clear";

    case VTSS_APPL_APS_COMMAND_FREEZE:
        return "Freeze";

    case VTSS_APPL_APS_COMMAND_FREEZE_CLEAR:
        return "Clear Freeze";

    default:
        T_E("Invalid command (%d)", command);
        return "unknown";
    }
}

//*****************************************************************************/
// aps_util_sf_trigger_to_str()
/******************************************************************************/
const char *aps_util_sf_trigger_to_str(vtss_appl_aps_sf_trigger_t sf_trigger)
{
    switch (sf_trigger) {
    case VTSS_APPL_APS_SF_TRIGGER_LINK:
        return "link";

    case VTSS_APPL_APS_SF_TRIGGER_MEP:
        return "mep";

    default:
        T_E("Invalid sf_trigger (%d)", sf_trigger);
        return "unknown";
    }
}

//*****************************************************************************/
// aps_util_oper_state_to_str()
/******************************************************************************/
const char *aps_util_oper_state_to_str(vtss_appl_aps_oper_state_t oper_state)
{
    switch (oper_state) {
    case VTSS_APPL_APS_OPER_STATE_ADMIN_DISABLED:
        return "Administratively disabled";

    case VTSS_APPL_APS_OPER_STATE_ACTIVE:
        return "Active";

    case VTSS_APPL_APS_OPER_STATE_INTERNAL_ERROR:
        return "Internal error has occurred. See console or crashlog for details";

    default:
        T_E("Invalid operational state (%d)", oper_state);
        return "unknown";
    }
}

//*****************************************************************************/
// aps_util_oper_state_to_str()
/******************************************************************************/
const char *aps_util_oper_state_to_str(vtss_appl_aps_oper_state_t oper_state, vtss_appl_aps_oper_warning_t oper_warning)
{
    if (oper_warning == VTSS_APPL_APS_OPER_WARNING_NONE || oper_state != VTSS_APPL_APS_OPER_STATE_ACTIVE) {
        return aps_util_oper_state_to_str(oper_state);
    }

    return "Active (warnings)";
}

//*****************************************************************************/
// aps_util_oper_warning_to_str()
/******************************************************************************/
const char *aps_util_oper_warning_to_str(vtss_appl_aps_oper_warning_t oper_warning)
{
    switch (oper_warning) {
    case VTSS_APPL_APS_OPER_WARNING_NONE:
        return "None";

    case VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_FOUND:
        return "The working MEP does not exist. Using link-state for signal-fail";

    case VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_FOUND:
        return "The protect MEP does not exist. Using link-state for signal-fail";

    case VTSS_APPL_APS_OPER_WARNING_WMEP_ADMIN_DISABLED:
        return "The working MEP is administratively disabled. Using link-state for signal-fail";

    case VTSS_APPL_APS_OPER_WARNING_PMEP_ADMIN_DISABLED:
        return "The protect MEP is administratively disabled. Using link-state for signal-fail";

    case VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_DOWN_MEP:
        return "The working MEP is not a Down-MEP. Using link-state for signal-fail";

    case VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_DOWN_MEP:
        return "The protect MEP is not a Down-MEP. Using link-state for signal-fail";

    case VTSS_APPL_APS_OPER_WARNING_WMEP_AND_PORT_IFINDEX_DIFFER:
        return "The working MEP is installed on another interface. Using link-state for signal-fail";

    case VTSS_APPL_APS_OPER_WARNING_PMEP_AND_PORT_IFINDEX_DIFFER:
        return "The protect MEP is installed on another interface. Using link-state for signal-fail";

    default:
        T_E("Invalid operational warning (%d)", oper_warning);
        return "unknown";
    }
}

//*****************************************************************************/
// aps_util_prot_state_to_str()
// Longest in short format:  7
// Longest in long  format: 19
/******************************************************************************/
const char *aps_util_prot_state_to_str(vtss_appl_aps_prot_state_t prot_state, bool short_format)
{
    switch (prot_state) {
    case VTSS_APPL_APS_PROT_STATE_NR_W:
        // A
        return short_format ? "NR-W"    : "No Request (W)";

    case VTSS_APPL_APS_PROT_STATE_NR_P:
        // B
        return short_format ? "NR-P"    : "No Request (P)";

    case VTSS_APPL_APS_PROT_STATE_LO:
        // C
        return short_format ? "LO"      : "Lockout";

    case VTSS_APPL_APS_PROT_STATE_FS:
        // D
        return short_format ? "FS"      : "Forced Switch";

    case VTSS_APPL_APS_PROT_STATE_SF_W:
        // E
        return short_format ? "SF-W"    : "Signal Fail (W)";

    case VTSS_APPL_APS_PROT_STATE_SF_P:
        // F
        return short_format ? "SF-P"    : "Signal Fail (P)";

    case VTSS_APPL_APS_PROT_STATE_MS_TO_P:
        // G
        return short_format ? "MS-to-P" : "Manual Switch to P";

    case VTSS_APPL_APS_PROT_STATE_MS_TO_W:
        // H
        return short_format ? "MS-to-W" : "Manual Switch to W";

    case VTSS_APPL_APS_PROT_STATE_WTR:
        // I
        return short_format ? "WTR"     : "Wait-to-Restore";

    case VTSS_APPL_APS_PROT_STATE_DNR:
        // J
        return short_format ? "DNR"     : "Do Not Revert";

    case VTSS_APPL_APS_PROT_STATE_EXER_W:
        // K
        return short_format ? "EXER-W"  : "Exercise (W)";

    case VTSS_APPL_APS_PROT_STATE_EXER_P:
        // L
        return short_format ? "EXER-P"  : "Exercise (P)";

    case VTSS_APPL_APS_PROT_STATE_RR_W:
        // M
        return short_format ? "RR-W"    : "Reverse Request (W)";

    case VTSS_APPL_APS_PROT_STATE_RR_P:
        // N
        return short_format ? "RR-P"    : "Reverse Request (P)";

    case VTSS_APPL_APS_PROT_STATE_SD_W:
        // P
        return short_format ? "SD-W"    : "Signal Degrade (W)";

    case VTSS_APPL_APS_PROT_STATE_SD_P:
        // Q
        return short_format ? "SD-P"    : "Signal Degrade (P)";

    default:
        T_E("Invalid protection state (%d)", prot_state);
        return "unknown";
    }
}

//*****************************************************************************/
// aps_util_defect_state_to_str()
/******************************************************************************/
const char *aps_util_defect_state_to_str(vtss_appl_aps_defect_state_t defect_state)
{
    switch (defect_state) {
    case VTSS_APPL_APS_DEFECT_STATE_OK:
        return "OK";

    case VTSS_APPL_APS_DEFECT_STATE_SD:
        return "SD";

    case VTSS_APPL_APS_DEFECT_STATE_SF:
        return "SF";

    default:
        T_E("Invalid defect_state (%d)", defect_state);
        return "unknown";
    }
}

//*****************************************************************************/
// aps_util_request_to_str()
/******************************************************************************/
const char *aps_util_request_to_str(vtss_appl_aps_request_t request)
{
    switch (request) {
    case VTSS_APPL_APS_REQUEST_NR:
        return "NR";

    case VTSS_APPL_APS_REQUEST_DNR:
        return "DNR";

    case VTSS_APPL_APS_REQUEST_RR:
        return "RR";

    case VTSS_APPL_APS_REQUEST_EXER:
        return "EXER";

    case VTSS_APPL_APS_REQUEST_WTR:
        return "WTR";

    case VTSS_APPL_APS_REQUEST_MS:
        return "MS";

    case VTSS_APPL_APS_REQUEST_SD:
        return "SD";

    case VTSS_APPL_APS_REQUEST_SF_W:
        return "SF-W";

    case VTSS_APPL_APS_REQUEST_FS:
        return "FS";

    case VTSS_APPL_APS_REQUEST_SF_P:
        return "SF-P";

    case VTSS_APPL_APS_REQUEST_LO:
        return "LO";

    default:
        T_E("Invalid request (%d)", request);
        return "unknown";
    }
}

//*****************************************************************************/
// aps_error_txt()
/******************************************************************************/
const char *aps_error_txt(mesa_rc error)
{
    switch (error) {
    case VTSS_APPL_APS_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_APS_RC_INTERNAL_ERROR:
        return "Internal Error: A code-update is required. See console or crashlog for details";

    case VTSS_APPL_APS_RC_NO_SUCH_INSTANCE:
        return "No such APS instance";

    case VTSS_APPL_APS_RC_MODE:
        return "Invalid mode";

    case VTSS_APPL_APS_RC_WTR:
        return "Invalid wait-to-restore value. Valid range is [1; 720] seconds";

    case VTSS_APPL_APS_RC_HOLD_OFF:
        return "Invalid hold-off-time. Valid range is [0; 10000] milliseconds in steps of 100 milliseconds";

    case VTSS_APPL_APS_RC_INVALID_LEVEL:
        return "Invalid MD/MEG level. Valid range is [0; 7]";

    case VTSS_APPL_APS_RC_INVALID_VLAN:
        return "Invalid VLAN. Valid range is [0; " vtss_xstr(VTSS_APPL_VLAN_ID_MAX) "]";

    case VTSS_APPL_APS_RC_INVALID_PCP:
        return "Invalid PCP. Valid range is [0; 7]";

    case VTSS_APPL_APS_RC_INVALID_SMAC:
        return "Source MAC address must be a unicast address";

    case VTSS_APPL_APS_RC_INVALID_W_IFINDEX:
        return "Invalid working interface. The interface index must represent a port";

    case VTSS_APPL_APS_RC_INVALID_P_IFINDEX:
        return "Invalid protect interface. The interface index must represent a port";

    case VTSS_APPL_APS_RC_INVALID_SF_TRIGGER_W:
        return "Invalid signal-fail trigger for working port";

    case VTSS_APPL_APS_RC_INVALID_SF_TRIGGER_P:
        return "Invalid signal-fail trigger for protect port";

    case VTSS_APPL_APS_RC_WMEP_MUST_BE_SPECIFIED:
        return "Working MEP must be specified when using MEP as signal-fail trigger and instance is adminstratively enabled";

    case VTSS_APPL_APS_RC_PMEP_MUST_BE_SPECIFIED:
        return "Protect MEP must be specified when using MEP as signal-fail trigger and instance is administratively enabled";

    case VTSS_APPL_APS_RC_W_P_MEP_IDENTICAL:
        return "Working and protecting MEPs cannot be the same";

    case VTSS_APPL_APS_RC_W_P_IFINDEX_IDENTICAL:
        return "Working and protect ports cannot use the same interface";

    case VTSS_APPL_APS_RC_ANOTHER_INST_USES_SAME_W_PORT:
        return "Another APS instance is using the same working port interface as this one";

    case VTSS_APPL_APS_RC_ANOTHER_INST_USES_SAME_P_PORT_AND_VLAN:
        return "Another APS instance is using the same protect port and VLAN";

    case VTSS_APPL_APS_RC_LIMIT_REACHED:
        return "The maximum number of APS instances is reached";

    case VTSS_APPL_APS_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_APS_RC_NOT_ACTIVE:
        return "APS instance is not active";

    case VTSS_APPL_APS_RC_COMMAND:
        return "Invalid command";

    case VTSS_APPL_APS_RC_COMMAND_FREEZE:
        return "Exit freeze mode to enter other local requests";

    case VTSS_APPL_APS_RC_COMMAND_EXERCISE_UNIDIRECTIONAL:
        return "Exercise of the APS protocol is not possible in unidirectional mode";

    case VTSS_APPL_APS_RC_NOT_READY_TRY_AGAIN_IN_A_FEW_SECONDS:
        return "APS is not yet ready. Please try again in a few seconds";

    case VTSS_APPL_APS_RC_HW_RESOURCES:
        return "Out of H/W resources";

    default:
        T_E("Unknown error code (%u)", error);
        return "APS: Unknown error code";
    }
}

/******************************************************************************/
// aps_history_dump()
/******************************************************************************/
void aps_history_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    aps_itr_t itr;
    bool      first = true;

    APS_LOCK_SCOPE();

    for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
        aps_base_history_dump(&itr->second, session_id, pr, first);
        first = false;
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// aps_rules_dump()
/******************************************************************************/
void aps_rules_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    aps_itr_t itr;
    bool      first = true;

    APS_LOCK_SCOPE();

    for (itr = APS_map.begin(); itr != APS_map.end(); ++itr) {
        aps_base_rule_dump(&itr->second, session_id, pr, first);
        first = false;
    }

    pr(session_id, "\n");
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void aps_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_ICFG
mesa_rc aps_icfg_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_aps_json_init(void);
#endif

VTSS_PRE_DECLS int aps_icli_cmd_register();

//*****************************************************************************/
// aps_init()
/******************************************************************************/
mesa_rc aps_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc     rc;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);
    switch (data->cmd) {
    case INIT_CMD_INIT:
        APS_capabilities_set();
        APS_default_conf_set();

        critd_init(&APS_crit, "APS Platform", VTSS_MODULE_ID_APS, CRITD_TYPE_MUTEX);

        aps_icli_cmd_register();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        aps_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_aps_json_init();
#endif

        APS_cfm_change_notifications.init();
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        // Initialize ICLI "show running" configuration
        VTSS_RC(aps_icfg_init());
#endif

        APS_cap_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);
        break;

    case INIT_CMD_CONF_DEF:
        if (isid == VTSS_ISID_GLOBAL) {
            APS_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        // Initialize the port-state array
        APS_port_state_init();

        {
            APS_LOCK_SCOPE();
            aps_timer_init();
        }

        // Register for CFM configuration change callbacks in CFM module
        if ((rc = cfm_util_conf_change_callback_register(VTSS_MODULE_ID_APS, APS_cfm_conf_change_callback)) != VTSS_RC_OK) {
            T_E("cfm_util_conf_change_callback_register() failed: %s", error_txt(rc));
        }

        APS_default();

        // Register for LAPS PDUs
        APS_frame_rx_register();

        break;

    case INIT_CMD_ICFG_LOADING_POST: {
        // Now that ICLI has applied all configuration, start a timer, which -
        // when it expires - will create all APS instances. The reason for this
        // deferred creation is that the MEPs on which the APS instances rely
        // may not be up and running when we otherwise wanted to create the APS
        // instances. Since we ask the CFM module for SF on W and P MEPs when we
        // create an APS instance, the CFM module may return that there's SF on
        // one or both MEPs. In case of non-revertive APS operation, this might
        // cause a change to the protect interface, which would then stay there
        // (DNR) forever.
        APS_LOCK_SCOPE();
        if (!APS_started) {
            aps_timer_init(APS_apply_config_timer, "APS Apply Config", 0, APS_apply_config_for_the_first_time, nullptr);
            aps_timer_start(APS_apply_config_timer, 10000 /* is 10 seconds too much or too little? */, false);
        }

        break;
    }

    default:
        break;
    }

    T_D("exit");
    return 0;
}

