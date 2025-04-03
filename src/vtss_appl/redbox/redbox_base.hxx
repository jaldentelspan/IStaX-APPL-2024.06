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

#ifndef _REDBOX_BASE_HXX_
#define _REDBOX_BASE_HXX_

#include "main_types.h"
#include <vtss/appl/redbox.h>
#include <vtss/basics/map.hxx> /* For vtss::Map()                             */
#include "redbox_api.h"        /* For ENUM_INC on port_type                   */
#include "redbox_timer.hxx"    /* For redbox_timer_XXX()                      */
#include "redbox_trace.h"      /* For REDBOX_TRACE_GRP_BASE                   */
#include "packet_api.h"        /* For packet_tx_props_t                       */
#include "mac_utils.hxx"       /* For mesa_mac_t::operator<() used in PNT map */

#define REDBOX_ETYPE_SUPERVISION 0x88fbu /* Supervision frame EtherType */
#define REDBOX_ETYPE_HSR         0x892fu /* HSR-tag's EtherType         */

// Number of ports on this switch
extern uint32_t REDBOX_cap_port_cnt;

typedef struct {
    // Port number that this port state represents (for self-containedness)
    mesa_port_no_t port_no;

    // MAC address of this switch
    mesa_mac_t *chassis_mac;

    // MAC address of this port
    mesa_mac_t redbox_mac;

    // Indicates whether the port is up or not.
    bool link;

    // Contains the current VLAN port configuration used to figure out whether
    // and how to tag supervision frames.
    vtss_appl_vlan_port_detailed_conf_t vlan_conf;

    // The TPID to use if tagging frames.
    mesa_etype_t tpid;

    // The MTU of this port.
    uint32_t mtu;

    // VLANs that this port is member of.
    uint8_t vlan_memberships[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
} redbox_port_state_t;

// Status of one MAC address in the NodesTable (NT).
// Represents MAC addresses from the H/W NodesTable and MAC addresses
// potentially only known by S/W due to proxied SV frames sent by other
// RedBoxes.
typedef struct {
    // Status of the MAC address in the NT.
    vtss_appl_redbox_nt_mac_status_t status;

    // In order to detect when an SMAC is no longer present in the NT, we need
    // a temporary flag that gets cleared upon NT poll start and gets set if
    // this MAC address is retrieved from the NT. Afterwards, we can delete all
    // those entries from the map that has the flag set to false.
    bool present_in_nt;
} redbox_nt_t;

// Status of one MAC address in the ProxyNodeTable (PNT).
typedef struct {
    // Status of the MAC address in the PNT.
    vtss_appl_redbox_pnt_mac_status_t status;

    // In order to detect when an SMAC is no longer present in the PNT, we need
    // a temporary flag that gets cleared upon PNT poll start and gets set if
    // this MAC address is retrieved from the PNT. Afterwards, we can delete all
    // those entries from the mac that has the flag set to false.
    bool present_in_pnt;

    // Timer controlling when to send the next Supervision frame for this one.
    redbox_timer_t timer;
} redbox_pnt_t;

// Status of one MAC address in the NT or PNT.
// We keep both NT and PNT entries it in one single map for two reasons:
// 1) The chip only has one table, so a given MAC address can only be in one of
//    the two tables at a time.
// 2) We can easily find the currennt number of entries in the H/W table, which
//    makes it easier to judge whether to add an entry to the PNT or not.
typedef struct {
    // The location of the entry. True if in PNT, false if in NT.
    bool is_pnt_entry;

    // The NT or PNT status.
    union {
        // Status of the MAC address in the NT.
        redbox_nt_t nt;

        // Status of the MAC address in the PNT.
        redbox_pnt_t pnt;
    }; // Keep it anonymous
} redbox_mac_t;

// Map of entries in the ProxyNodeTable.
typedef vtss::Map<mesa_mac_t, redbox_mac_t> redbox_mac_map_t;
typedef redbox_mac_map_t::iterator          redbox_mac_itr_t;

// We create a number of different SV frame templates for use when we transmit
// SV frames. See redbox_sv_frame_type_t below for a list.
typedef struct {
    // Supervision frame
    packet_tx_props_t tx_props;

    // Pointers to various dynamically updated fields of the SV frame.
    uint8_t *dmac_lsb_ptr;
    uint8_t *smac_ptr;
    uint8_t *vlan_tag_ptr;
    uint8_t *sup_sequence_number_ptr;
    uint8_t *tlv1_type_ptr;
    uint8_t *tlv1_mac_address_ptr;
    uint8_t *tlv2_mac_address_ptr;
    uint8_t *rct_ptr;
} redbox_sv_frame_t;

// Depending on mode, we may create templates for one or more of these SV frame
// types. The reason we create templates is manyfold:
// 1) We don't have to create a new frame for every proxied SV frame we send,
//    but can overwrite particular fields of an existing template and send it
//    right away.
// 2) We don't have to dispose off all template frames if a VLAN changes from
//    e.g. being tagged to being untagged or if the configured VID itself
//    changes. We just use another template when updating fields and transmit.
//    VLAN-tagged/untagged templates are not part of the enumerations, however.
typedef enum {
    REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT,         // SV frames generated by us that should be transmitted with    TLV2
    REDBOX_SV_FRAME_TYPE_GEN_FROM_PNT_NO_TLV2, // SV frames generated by us that should be transmitted without TLV2 (RB's own, only)
    REDBOX_SV_FRAME_TYPE_FWD_PRP_TO_HSR,       // SV frames forwarded by us that should be transmitted HSR-tagged
    REDBOX_SV_FRAME_TYPE_FWD_HSR_TO_PRP,       // SV frames forwarded by us that should be transmitted RCT-tagged

    // Must come last.
    REDBOX_SV_FRAME_TYPE_CNT
} redbox_sv_frame_type_t;

typedef struct {
    vtss_appl_redbox_conf_t       conf;
    vtss_appl_redbox_status_t     status; // status.notif_status is not updated by base. Needs to come from global redbox_notification_status object.
    vtss_appl_redbox_statistics_t statistics; // Only counts S/W-based frames

    // Pointer to a port state for the port representing PRP/HSR port A and B.
    // [0] = Port A, [1] = Port B.
    // Either may be nullptr if it's a RedBox Neighbor port.
    redbox_port_state_t *port_states[2];

    // Map of MAC addresses in the NT and PNT
    redbox_mac_map_t mac_map;

    // Indicates whether the I/L port is member of the configured VLAN to Tx SV
    // frames on (if not forwarded). If it's not a member, we will not transmit
    // proxied SV frames.
    bool interlink_member_of_tx_vlan;

    // The NodesTable is polled upon request from management interfaces, but not
    // faster than every two seconds.
    // In order to correctly show the "last_seen_secs", it must also be polled
    // faster than half the current age time, or we could happen to have stall
    // entries in the S/W map (entries that appear, disappear and reappear in
    // the H/W NodesTable).
    // The nt_last_poll_msecs keeps track of the last time the map was polled.
    // nt_poll_timer makes sure to poll every NT age time seconds.
    uint64_t nt_last_poll_msecs;

    // Similar for PNT
    uint64_t pnt_last_poll_msecs;

    // NodesTable poll timer.
    redbox_timer_t nt_poll_timer;

    // When we think the MAC table (NT/PNT) is full, we set an operational
    // status, but we need to check periodically whether it's still full. This
    // is a timer for that.
    redbox_timer_t nt_pnt_table_full_timer;

    // Timers running for REDBOX_cap.alarm_raised_time_secs seconds if in any
    // HSR mode and receiving an HSR-untagged frame on Port A or Port B or if in
    // HSR-HSR mode and receiving an HSR-untagged frame on Port C.
    redbox_timer_t hsr_untagged_rx_timers[3];

    // In order to figure out that the rx_untagged_cnt counts, we need to keep
    // track of their last values.
    mesa_counter_t hsr_untagged_rx[3];

    // Timers running for REDBOX_cap.alarm_raised_time_secs seconds if receiving
    // frames with wrong LAN ID in PRP-SAN mode on Port A or Port B or if in
    // HSR-PRP mode and receiving with wrong LAN ID on Port C.
    redbox_timer_t cnt_err_wrong_lan_timers[3];

    // In order to figure out that the rx_wrong_lan_cnt counts, we need to keep
    // track of their last values.
    mesa_counter_t cnt_err_wrong_lan[3];

    // And we need a statistics poll timer.
    // This requires a timer to poll statistics and two MESA counters.
    redbox_timer_t statistics_poll_timer;

    // If true, transmission of own and/or proxied supervision frames is
    // suspended.
    bool tx_spv_suspend_own;
    bool tx_spv_suspend_proxied;

    // SV frame templates.
    // 1st index: SV frame type
    // 2nd index: VLAN untagged (0)/VLAN tagged (1)
    redbox_sv_frame_t sv_frames[REDBOX_SV_FRAME_TYPE_CNT][2];

    // Supervision frame's next SupSequenceNumber
    uint16_t sup_sequence_number;

    // RedBox' Redundancy Tag Sequence number (used in HSR-tag and RCT-tags in
    // HSR-PRP mode, where we must insert them before sending frames with
    // RedBox' SMAC to either PRP network or HSR ring.
    uint16_t redundancy_tag_sequence_number;

    // ProxyNodeTable poll timer.
    redbox_timer_t pnt_poll_timer;

    // The number of this RedBox instance
    uint32_t inst;

    // ACE ID used to discard all frames in all VLANs destined to the RB's MAC
    // address.
    mesa_ace_id_t rb_mac_discard_ace_id;

    mesa_rb_id_t rb_id_get(void) const
    {
        // The instance we use in the API is the same as our instance number
        // less one, because ours is 1-based, and the API is 0-based.
        return inst - 1;
    }

    mesa_vid_t tx_vlan_get(vtss_appl_vlan_port_detailed_conf_t *use_this_vlan_conf = nullptr) const
    {
        vtss_appl_vlan_port_detailed_conf_t &vlan_conf = use_this_vlan_conf ? *use_this_vlan_conf : interlink_port_state_get()->vlan_conf;

        // If conf.sv_vlan is zero, we use Port A's PVID, otherwise the
        // configured VLAN ID. See also tx_vlan_tagged().
        if (conf.sv_vlan) {
            return conf.sv_vlan;
        }

        return vlan_conf.pvid;
    }

    bool tx_vlan_tagged(vtss_appl_vlan_port_detailed_conf_t *use_this_vlan_conf = nullptr) const
    {
        // Figure out whether we need to transmit frames VLAN tagged or not.
        mesa_vid_t                          vid        = tx_vlan_get(use_this_vlan_conf);
        vtss_appl_vlan_port_detailed_conf_t &vlan_conf = use_this_vlan_conf ? *use_this_vlan_conf : interlink_port_state_get()->vlan_conf;

        switch (vlan_conf.tx_tag_type) {
        case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
            return vid != vlan_conf.untagged_vid;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
            return vid == vlan_conf.untagged_vid;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
            return true;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
            return false;

        default:
            T_EG_PORT(REDBOX_TRACE_GRP_BASE, interlink_port_no_get(), "%u: Unknown tx_tag_type (%d)", inst, vlan_conf.tx_tag_type);
            return vid != vlan_conf.pvid;
        }
    }

    bool any_hsr_mode(void) const
    {
        return conf.mode == VTSS_APPL_REDBOX_MODE_HSR_SAN ||
               conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP ||
               conf.mode == VTSS_APPL_REDBOX_MODE_HSR_HSR;
    }

    vtss_appl_redbox_port_type_t interlink_port_type_get(void) const
    {
        // If using Port A to connect to left neighbor (and therefore doesn't
        // have a port state), the interlink port becomes Port B, otherwise
        // Port A.
        return port_states[VTSS_APPL_REDBOX_PORT_TYPE_A] == nullptr ? VTSS_APPL_REDBOX_PORT_TYPE_B : VTSS_APPL_REDBOX_PORT_TYPE_A;
    }

    redbox_port_state_t *interlink_port_state_get(void) const
    {
        redbox_port_state_t *port_state = port_states[interlink_port_type_get()];

        if (!port_state) {
            T_EG(REDBOX_TRACE_GRP_BASE, "%u: Unable to get interlink port state (oper_active = %d)", inst, status.oper_state);
        }

        return port_state;
    }

    mesa_port_no_t interlink_port_no_get(void) const
    {
        return interlink_port_state_get()->port_no;
    }

    bool using_port_no(mesa_port_no_t port_no) const
    {
        vtss_appl_redbox_port_type_t port_type;

        for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_B; port_type++) {
            if (port_states[port_type] && port_states[port_type]->port_no == port_no) {
                return true;
            }
        }

        return false;
    }

    bool using_port_x(vtss_appl_redbox_port_type_t port_type) const
    {
        return port_states[port_type] != nullptr;
    }

    mesa_port_no_t port_no_get(vtss_appl_redbox_port_type_t port_type) const
    {
        return using_port_x(port_type) ? port_states[port_type]->port_no : MESA_PORT_NO_NONE;
    }

    bool port_link_get(vtss_appl_redbox_port_type_t port_type) const
    {
        if (using_port_x(port_type)) {
            return port_states[port_type]->link;
        } else {
            // There's always link on the internal connection.
            return true;
        }
    }
} redbox_state_t;

// We maintain a global array of port roles.
typedef enum {
    REDBOX_PORT_ROLE_TYPE_NORMAL,      // Normal switch port
    REDBOX_PORT_ROLE_TYPE_INTERLINK,   // The interlink port belonging to a RedBox instance
    REDBOX_PORT_ROLE_TYPE_UNCONNECTED, // The non-interlink port belonging to a RedBox instance (that doesn't use an internal connection).
} redbox_port_role_type_t;

typedef struct {
    redbox_port_role_type_t type;

    // If type is REDBOX_PORT_ROLE_TYPE_INTERLINK, this is a valid pointer
    // to the RedBox instance belonging to that port. Otherwise, it's nullptr.
    // Only active RBs can have a port set to non-NORMAL.
    redbox_state_t *redbox_state;
} redbox_port_role_t;

extern CapArray<redbox_port_role_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> redbox_port_roles;

// TLV Types. Their values correspond one-to-one with the standard.
typedef enum {
    REDBOX_TLV_TYPE_NONE   =  0, // TLV2 not found.
    REDBOX_TLV_TYPE_PRP_DD = 20, // PRP, where node supports Duplicate Discard
    REDBOX_TLV_TYPE_PRP_DA = 21, // PRP, where node supports Duplicate Accept
    REDBOX_TLV_TYPE_HSR    = 23, // HSR
    REDBOX_TLV_TYPE_REDBOX = 30, // RedBox (TLV2)
} redbox_tlv_type_t;

typedef struct {
    vtss_appl_redbox_port_type_t port_type; // Port on which this SV frame is received.
    uint8_t                      dmac_lsb;  // LSByte of received SV frame's DMAC.
    mesa_mac_t                   smac;
    uint16_t                     sup_sequence_number;
    redbox_tlv_type_t            tlv1_type; // Either PRP_DD, PRP_DA, or HSR
    mesa_mac_t                   tlv1_mac;
    redbox_tlv_type_t            tlv2_type; // REDBOX if TLV2 present, NONE if not.
    mesa_mac_t                   tlv2_mac;
    bool                         rct_present;
    vtss_appl_redbox_lan_id_t    rct_lan_id;

    bool tlv2_present(void) const
    {
        return tlv2_type == REDBOX_TLV_TYPE_REDBOX;
    }
} redbox_pdu_info_t;

typedef vtss::Map<uint32_t, redbox_state_t> redbox_map_t;
typedef redbox_map_t::iterator redbox_itr_t;

extern  redbox_map_t                    REDBOX_map;
extern  vtss_appl_redbox_capabilities_t REDBOX_cap;

// Internal function used between base and pdu:
void redbox_base_rx_sv_on_interlink(redbox_state_t &redbox_state, mesa_port_no_t rx_port_no, const redbox_pdu_info_t &rx_pdu_info, mesa_vlan_tag_t &class_tag, bool tx_vlan_tagged);

mesa_rc redbox_base_activate(                  redbox_state_t &redbox_state);
mesa_rc redbox_base_deactivate(                redbox_state_t &redbox_state);
void    redbox_base_notification_status_update(redbox_state_t &redbox_state);
void    redbox_base_api_conf_changed(          redbox_state_t &redbox_state);
void    redbox_base_sv_frame_interval_changed( redbox_state_t &redbox_state);
void    redbox_base_nt_age_time_changed(       redbox_state_t &redbox_state);
void    redbox_base_xlat_prp_to_hsr_changed(   redbox_state_t &redbox_state);
void    redbox_base_nt_poll(                   redbox_state_t &redbox_state);
void    redbox_base_pnt_poll(                  redbox_state_t &redbox_state);
mesa_rc redbox_base_nt_clear(                  redbox_state_t &redbox_state);
mesa_rc redbox_base_pnt_clear(                 redbox_state_t &redbox_state);
mesa_rc redbox_base_statistics_get(            redbox_state_t &redbox_state, vtss_appl_redbox_statistics_t &statistics);
mesa_rc redbox_base_statistics_clear(          redbox_state_t &redbox_state);
bool    redbox_base_port_is_lre_port(mesa_port_no_t port_no);
void    redbox_base_debug_tx_spv_suspend_get(  redbox_state_t &redbox_state, bool &own_suspended, bool &proxied_suspended);
void    redbox_base_debug_tx_spv_suspend_set(  redbox_state_t &redbox_state, bool suspend_own, bool suspend_proxied);
void    redbox_base_debug_clear_notifications( redbox_state_t &redbox_state);
void    redbox_base_debug_port_state_dump(redbox_icli_pr_t pr);
void    redbox_base_debug_state_dump(                  redbox_state_t &redbox_state, redbox_icli_pr_t pr);

#endif /* _REDBOX_BASE_HXX_ */

