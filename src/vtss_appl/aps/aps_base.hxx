/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _APS_BASE_HXX_
#define _APS_BASE_HXX_

#include "main_types.h"
#include <vtss/appl/aps.h>
#include <vtss/basics/map.hxx>     /* For vtss::Map()                */
#include <vtss/basics/ringbuf.hxx> /* For vtss::RingBuf()            */
#include "aps_timer.hxx"           /* For aps_timer_t                */
#include "acl_api.h"               /* For acl_entry_conf_t           */
#include "packet_api.h"            /* For packet_tx_props_t          */
#include <vtss/appl/vlan.h>        /* For vtss_appl_vlan_port_type_t */

// Number of APS-specific bytes in L-APS PDUs
#define APS_LAPS_DATA_LENGTH 4

// The opcode used in LAPS PDUs
#define APS_OPCODE_LAPS 39

typedef struct {
    mesa_port_no_t             port_no;
    mesa_mac_t                 smac;
    bool                       link;
    vtss_appl_vlan_port_type_t port_type;
    mesa_etype_t               tpid;
    mesa_vid_t                 pvid;
} aps_port_state_t;

// The higher the enumeration, the higher prioritized, so do not swap any
// members
typedef enum {
    APS_BASE_REQUEST_NR,          // No Request (near and far end)
    APS_BASE_REQUEST_DNR,         // Do Not Revert (far end only)
    APS_BASE_REQUEST_RR,          // Reverse Request (far end only)
    APS_BASE_REQUEST_CLEAR,       // Clear switch command (near end only)
    APS_BASE_REQUEST_EXER,        // Exercise command (near end and far end)
    APS_BASE_REQUEST_WTR_EXPIRED, // Wait-To-Restore timer has just expired (near-end only)
    APS_BASE_REQUEST_WTR,         // Waiting to restore (far-end only)
    APS_BASE_REQUEST_MS_TO_W,     // Manual Switch to Working command (near and far end)
    APS_BASE_REQUEST_MS_TO_P,     // Manual Switch to Protect command (near and far end)
    APS_BASE_REQUEST_SD_W_OFF,    // Recover from Signal Degrade on Working (near end only)
    APS_BASE_REQUEST_SD_W_ON,     // Signal Degrade on Working (near and far end)
    APS_BASE_REQUEST_SD_P_OFF,    // Recover from Signal Degrade on Protect (near end only)
    APS_BASE_REQUEST_SD_P_ON,     // Signal Degrade on Protect (near and far end)
    APS_BASE_REQUEST_SF_W_OFF,    // Recover from Signal Fail on Working (near end only)
    APS_BASE_REQUEST_SF_W_ON,     // Signal Fail on Working (near and far end)
    APS_BASE_REQUEST_FS,          // Forced switch command (near and far end)
    APS_BASE_REQUEST_SF_P_OFF,    // Recover from Signal Fail on Protect (near end only)
    APS_BASE_REQUEST_SF_P_ON,     // Signal Fail on Protect (near and far end)
    APS_BASE_REQUEST_LO,          // Lockout command (near and far end)
} aps_base_request_t;

typedef struct {
    uint64_t                   event_time_ms;
    aps_base_request_t         local_request;
    aps_base_request_t         far_end_request;
    vtss_appl_aps_prot_state_t prot_state;
} aps_base_history_element_t;

typedef vtss::RingBuf<aps_base_history_element_t, 50> aps_base_history_t;
typedef aps_base_history_t::iterator aps_base_history_itr_t;

// The configuration and status of one instance.
typedef struct {
    vtss_appl_aps_conf_t       conf;
    vtss_appl_aps_status_t     status;
    vtss_appl_aps_command_t    command;

    // The following members are temporary and used for conf-changes and
    // inactivation of an APS instance.
    vtss_appl_aps_conf_t         old_conf;
    mesa_port_no_t               old_w_port_no;
    mesa_port_no_t               old_p_port_no;
    vtss_appl_aps_oper_state_t   old_oper_state;
    vtss_appl_aps_oper_warning_t old_oper_warning;
    mesa_mac_t                   old_smac;

    uint32_t                   inst;                  // For internal trace
    mesa_mac_t                 smac;                  // Source MAC address
    aps_port_state_t           *w_port_state;         // Pointer to the working port state
    aps_port_state_t           *p_port_state;         // Pointer to the protect port state

    // Variables handled by aps_base.
    bool                       sf_w;                  // Last received SF from W port/MEP (w/o waiting for hold-off. Hold-off'ed variant in status.w_state)
    bool                       sf_p;                  // Last received SF from P port/MEP (w/o waiting for hold-off. Hold-off'ed variant in status.p_state)
    bool                       sd_w;                  // Last received SD from W port/MEP
    bool                       sd_p;                  // Last received SD from P port/MEP
    bool                       wtr_event;
    bool                       coming_from_sf;
    aps_timer_t                wtr_timer;
    aps_timer_t                hoff_w_timer;
    aps_timer_t                hoff_p_timer;
    aps_timer_t                dFOP_NR_timer;
    uint8_t                    rx_aps_info[APS_LAPS_DATA_LENGTH];
    uint8_t                    tx_aps_info[APS_LAPS_DATA_LENGTH];

    // We add an ACL rule (ACE) for capturing and termination of L-APS PDUs.
    // This is especially needed if we use link as a SF-trigger, because there
    // might not be a MEP that terminates the PDUs then.
    acl_entry_conf_t ace_conf;

    // History for debugging purposes
    aps_base_history_t         history;
    aps_base_history_element_t last_history_element;

    // Variables handled by aps_laps.
    packet_tx_props_t          tx_props;
    aps_timer_t                rx_p_timer;
    aps_timer_t                rx_w_timer;
    aps_timer_t                tx_timer;
    bool                       laps_pdu_transmitted;
    uint32_t                   tx_aps_info_offset;

    bool tx_laps_pdus(void) const
    {
        if (conf.mode == VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL && !conf.tx_aps) {
            // In unidirectional 1+1 with Tx of LAPS PDUs == false, we don't send
            // LAPS PDUs.
            return false;
        }

        // In all other modes we do.
        return true;
    }

    bool is_tagged(void) const
    {
        return conf.vlan != 0;
    }

    mesa_vid_t classified_vid_get(bool working = false) const
    {
        // Get the VID that a frame destined for this APS instance will get
        // classified to. If it's untagged, it will become the PVID.
        if (is_tagged()) {
            return conf.vlan;
        } else {
            return working ? w_port_state->pvid : p_port_state->pvid;
        }

        return 0;
    }
} aps_state_t;

typedef vtss::Map<uint32_t, aps_state_t> aps_map_t;
typedef aps_map_t::iterator aps_itr_t;

// Functions in base invoked by platform
mesa_rc aps_base_activate(        aps_state_t *aps_state, bool initial_sf_w, bool initial_sf_p);
mesa_rc aps_base_deactivate(      aps_state_t *aps_state);
mesa_rc aps_base_command_set(     aps_state_t *aps_state, vtss_appl_aps_command_t new_cmd);
mesa_rc aps_base_matching_update( aps_state_t *aps_state);
void    aps_base_statistics_clear(aps_state_t *aps_state);
void    aps_base_tx_frame_update( aps_state_t *aps_state);
void    aps_base_exercise_sm(     aps_state_t *aps_state);
void    aps_base_rx_frame(        aps_state_t *aps_state, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info);
void    aps_base_sf_sd_set(       aps_state_t *aps_state, bool working, bool sf, bool sd);
void    aps_base_history_dump(    aps_state_t *aps_state, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr);
void    aps_base_rule_dump(       aps_state_t *aps_state, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr);

// Functions in base invoked by aps_laps
void aps_base_rx_timeout(aps_state_t *aps_state, bool working);

#endif /* _APS_BASE_HXX_ */

