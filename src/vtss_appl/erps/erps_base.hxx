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

#ifndef _ERPS_BASE_HXX_
#define _ERPS_BASE_HXX_

#include "main_types.h"
#include <vtss/appl/erps.h>
#include <vtss/basics/map.hxx>     /* For vtss::Map()                      */
#include <vtss/basics/ringbuf.hxx> /* For vtss::RingBuf()                  */
#include <erps_api.h>              /* For erps_util_using_ring_port_conf() */
#include "erps_timer.hxx"          /* For erps_timer_t                     */
#include "packet_api.h"            /* For packet_tx_props_t                */
#include "acl_api.h"               /* For acl_entry_conf_t                 */
#include <vtss/appl/vlan.h>        /* For vtss_appl_vlan_port_type_t       */

// Number of *active* R-APS-specific bytes to be transmitted in R-APS PDUs.
// The R-APS-specific field is 32 bytes long, but only the first two change with
// ring-topology changes/commands.
// The next 6 bytes are Node ID, which can be set by configuration, and the last
// 24 bytes are reserved and must always be set to 0.
#define ERPS_RAPS_TX_DATA_LENGTH 2

// Number of *active* R-APS-specific bytes received in R-APS PDUs.
// The R-APS-specific field is 32 bytes long of which we only look at the first
// 8 bytes (Req/state + Node ID).
#define ERPS_RAPS_RX_DATA_LENGTH 8

// The opcode used in RAPS PDUs
#define ERPS_OPCODE_RAPS 40

extern const mesa_mac_t erps_raps_dmac;

size_t fmt(vtss::ostream &stream, const vtss::Fmt &fmt, const vtss_appl_erps_ring_port_t *ring_port);

typedef struct {
    mesa_port_no_t                      port_no;
    mesa_mac_t                          smac;
    bool                                link;
    vtss_appl_vlan_port_detailed_conf_t vlan_conf;
    mesa_etype_t                        tpid;
} erps_port_state_t;

// Local and remote requests sorted by priority (G.8032-2015, clause 10.1.1).
// The higher the enumeration, the higher prioritized, so do not swap any
// members.
typedef enum {
    ERPS_BASE_REQUEST_NONE,                  // Local & Remote: No requests.
    ERPS_BASE_REQUEST_REMOTE_NR,             // Remote: No Request
    ERPS_BASE_REQUEST_REMOTE_NR_RB,          // Remote: No Request, RPL Blocked
    ERPS_BASE_REQUEST_LOCAL_WTB_RUNNING,     // Local:  Wait-to-block timer is running
    ERPS_BASE_REQUEST_LOCAL_WTB_EXPIRES,     // Local:  Wait-to-block timer expires
    ERPS_BASE_REQUEST_LOCAL_WTR_RUNNING,     // Local:  Wait-to-restore timer is running
    ERPS_BASE_REQUEST_LOCAL_WTR_EXPIRES,     // Local:  Wait-to-restore timer expires
    ERPS_BASE_REQUEST_LOCAL_MS,              // Local:  Manual Switch command
    ERPS_BASE_REQUEST_REMOTE_MS,             // Remote: Manual Switch command
    ERPS_BASE_REQUEST_REMOTE_SF,             // Remote: Signal Fail
    ERPS_BASE_REQUEST_LOCAL_SF_CLEAR,        // Local:  Signal Fail has cleared
    ERPS_BASE_REQUEST_LOCAL_SF,              // Local:  Signal Fail
    ERPS_BASE_REQUEST_REMOTE_FS,             // Remote: Forced Switch command
    ERPS_BASE_REQUEST_LOCAL_FS,              // Local:  Forced Switch command
    ERPS_BASE_REQUEST_LOCAL_CLEAR,           // Local:  Clear command
} erps_base_request_t;

// As per G.8032-2015, clause 10.1.10.
// Used for flush logic
typedef struct  {
    // Node ID of last received R-APS PDU on a ring port
    mesa_mac_t node_id;

    // Blocked Port Reference of last received R-APS PDU on a ring port
    vtss_appl_erps_ring_port_t bpr;
} erps_base_node_id_bpr_t;

// Why are we flushing FDB (only used in history)
typedef enum {
    ERPS_BASE_FLUSH_REASON_NONE,    // FDB flush was not performed
    ERPS_BASE_FLUSH_REASON_FSM,     // State-machine dictated
    ERPS_BASE_FLUSH_REASON_EVENT,   // Received a VTSS_APPL_ERPS_REQUEST_EVENT request
    ERPS_BASE_FLUSH_REASON_NODE_ID, // Node ID differs and DNF is cleared
} erps_base_flush_reason_t;

typedef struct {
    uint64_t                    event_time_ms;
    erps_base_request_t         local_request;
    erps_base_request_t         remote_request;
    vtss_appl_erps_ring_port_t  rx_ring_port;
    vtss_appl_erps_node_state_t node_state;
    bool                        port0_sf;
    bool                        port1_sf;
    bool                        port0_blocked;
    bool                        port1_blocked;
    erps_base_flush_reason_t    flush_reason;
    bool                        tx_raps_active;
    vtss_appl_erps_raps_info_t  tx_raps_info;
    erps_base_node_id_bpr_t     rx_node_id_bpr;
} erps_base_history_element_t;

typedef vtss::RingBuf<erps_base_history_element_t, 50> erps_base_history_t;
typedef erps_base_history_t::iterator erps_base_history_itr_t;

// State of port0 and port1
typedef struct {
    // Pointer to the port states for this ring-port.
    //
    // If this is a major ring, only index 0 is used for both port0 and port1.
    // If this is a sub-ring,   only index 0 is used for both port0 and port1.
    // If this is an interconnected sub-ring without virtual channel:
    //    Only index 0 is used for port0
    //    Neither index 0 nor index 1 is used for port1
    // If this is an interconnected sub-ring with virtual channel:
    //    Only index 0 is used for port0
    //    Both index 0 and 1 are used for port1, but may be nullptr if the
    //    connected ring is not configured correctly.
    erps_port_state_t *port_states[2];

    // The source MAC to use in R-APS PDUs on this port. Defaults to the port's
    // source MAC, but may be overridden by configuration.
    mesa_mac_t smac;

    // R-APS PDU to Tx.
    // Index 1 is only used for interconnected sub-rings with virtual channel.
    packet_tx_props_t tx_props[2];

    // Where in the R-APS PDU to update Tx R-APS info.
    // Index 1 is only used for interconnected sub-rings with virtual channel.
    uint32_t tx_raps_info_offset[2];

    // The R-APS-specific Rx received on this rings port
    uint8_t rx_raps_info[ERPS_RAPS_RX_DATA_LENGTH];

    // Per ring-port timers
    erps_timer_t pm_timer;
    erps_timer_t hoff_timer;

    // True if at least one R-APS PDU has been transmitted.
    bool raps_pdu_transmitted;

    // Last received SF w/o waiting for hold-off. Hold-off'ed variant in
    // status.ring_port_status[].defect_state)
    bool sf;

    // Transient variable indicating that SF is clearing.
    bool sf_clearing;

    // The last received <Node ID, BPR> pair on this ring port.
    erps_base_node_id_bpr_t rx_node_id_bpr;
} erps_ring_port_state_t;

// This structure contains all the configured protection group information.
typedef struct {
    bool using_ring_port_conf(vtss_appl_erps_ring_port_t ring_port) const
    {
        return erps_util_using_ring_port_conf(&conf, ring_port);
    }

    mesa_erpi_t erpi(void) const
    {
        // The instance we use in the API is the same as our instance less one,
        // because ours is 1-based, and the API is 0-based.
        return inst - 1;
    }

    vtss_appl_erps_conf_t   conf;
    vtss_appl_erps_status_t status;
    vtss_appl_erps_command_t command;

    // The following members are temporary and used for conf-changes and
    // inactivation of an ERPS instance.
    vtss_appl_erps_conf_t         old_conf;
    vtss_appl_erps_oper_state_t   old_oper_state;
    vtss_appl_erps_oper_warning_t old_oper_warning;
    erps_ring_port_state_t        old_ring_port_state[2];

    uint32_t                   inst;                  // For internal trace
    erps_ring_port_state_t     ring_port_state[2];    // Indexed by VTSS_APPL_ERPS_RING_PORT0/1.
    vtss_appl_erps_ring_port_t local_event_ring_port;
    erps_base_request_t        top_prio_request;      // The last top-priority request needed outside SM function by ERPS_BASE_ace_port_list_get()

    // The R-APS-specific Tx info common to both rings and maintained by
    // erps_raps.cxx.
    uint8_t tx_info[ERPS_RAPS_TX_DATA_LENGTH];

    // We add ACL rules (ACEs) for capturing and forwarding of R-APS PDUs.
    acl_entry_conf_t ace_conf;

    // Timers
    erps_timer_t tx_timer;
    erps_timer_t rx_timer;
    erps_timer_t wtr_timer;
    erps_timer_t guard_timer;
    erps_timer_t wtb_timer;
    erps_timer_t tc_timer;

    // Timeout (not received any R-APS PDUs on interface within last 17.5
    // seconds. This runs independently of cFOP_TO
    bool dFOP_TO;

    // True if we expect reception of R-APS PDUs on at least one of our ring
    // ports.
    bool expect_raps_rx;

    // These are true a very short time, namely when the WTR and WTB timers
    // fire.
    bool wtr_event;
    bool wtb_event;

    // For debugging purposes
    erps_base_history_t         history;
    erps_base_history_element_t last_history_element;
} erps_state_t;

typedef vtss::Map<uint32_t, erps_state_t> erps_map_t;
typedef erps_map_t::iterator erps_itr_t;
extern  erps_map_t           ERPS_map;

mesa_rc erps_base_activate(                   erps_state_t *erps_state, bool initial_sf_port0, bool initial_sf_port1);
mesa_rc erps_base_deactivate(                 erps_state_t *erps_state);
mesa_rc erps_base_command_set(                erps_state_t *erps_state, vtss_appl_erps_command_t new_cmd);
mesa_rc erps_base_protected_vlans_update(     erps_state_t *erps_state);
mesa_rc erps_base_matching_update(            erps_state_t *erps_state);
mesa_rc erps_base_connected_ring_ports_update(erps_state_t *erps_state);
void    erps_base_statistics_clear(           erps_state_t *erps_state);
void    erps_base_tx_frame_update(            erps_state_t *erps_state);
void    erps_base_rx_frame(                   erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info);
void    erps_base_sf_set(                     erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, bool sf);
void    erps_base_history_dump(               erps_state_t *erps_state, uint32_t relative_to, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr);
void    erps_base_history_clear(              erps_state_t *erps_state);
void    erps_base_rule_dump(                  erps_state_t *erps_state, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr);

#endif /* _ERPS_BASE_HXX_ */

