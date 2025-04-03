/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef _TOPO_API_H_
#define _TOPO_API_H_

#if defined(VTSS_SWITCH_STANDALONE) && VTSS_SWITCH_STANDALONE
#include "standalone_api.h" /* Include the topology module functions replacements */
#else

#include <time.h>

#include "vtss_sprout_api.h"

#ifdef __cplusplus
extern "C" {
#endif


/* TOPO error text */
const char *topo_error_txt(mesa_rc rc);

/* Set maximum table size slightly larger to handle transitional unit counts during quick removal+insertion */
#define TOPO_SIT_SIZE           (TOPO_MAX_UNITS_IN_STACK + TOPO_MAX_UNITS_IN_STACK/2)


// Maximum number of chip/units in a stack
#define TOPO_MAX_UNITS_IN_STACK 32

// Master election priority.
// Smaller priority => higher probability of becoming master */
#define TOPO_MST_ELECT_PRIO_END      (TOPO_MST_ELECT_PRIO_START+TOPO_MST_ELECT_PRIOS)
#define TOPO_MST_ELECT_PRIO_LEGAL(v) (TOPO_MST_ELECT_PRIO_START <= v && v < TOPO_MST_ELECT_PRIO_END)
typedef uint topo_mst_elect_prio_t;

// Firmware version mode
// Same as vtss_sprout_fw_ver_mode_t
#define TOPO_FW_VER_MODE_LEGAL(v) (VTSS_SPROUT_FW_VER_MODE_NULL == v || v == VTSS_SPROUT_FW_VER_MODE_NORMAL)


// Chip index (within switch)
// CHIP_IDX=1 is the chip with the (primary) CPU.
#define TOPO_CHIP_IDX_START 1
#define TOPO_CHIP_IDXS      2
#define TOPO_CHIP_IDX_END   (TOPO_CHIP_IDX_START+TOPO_CHIP_IDXS)
typedef uint topo_chip_idx_t;

// Topology type
typedef enum {TopoBack2Back,  // Not used for VTSS_SPROUT_V2
              TopoClosedLoop,
              TopoOpenLoop
             } topo_topology_type_t;

// Topo port status
typedef vtss_appl_topo_stack_port_stat_t topo_stack_port_stat_t;
// Topo switch status
typedef vtss_appl_topo_switch_stat_t     topo_switch_stat_t;

/* Distance (hop count) to switch. 0..  -1 => Infinity */
#define TOPO_DIST_INFINITY -1
typedef int topo_dist_t;

typedef vtss_sprout_sit_entry_t topo_sit_entry_t;
typedef vtss_sprout_sit_t       topo_sit_t;

/* Use of bits in topo_state_change_mask_t */
/* TTL changed (including link up/down) */
#define TOPO_STATE_CHANGE_MASK_TTL (1 << 0)

// State change call back function type
typedef uchar topo_state_change_mask_t;
typedef void (*topo_state_change_callback_t)(topo_state_change_mask_t mask);

// Master change call back function type
typedef void (*topo_mst_change_callback_t)(mesa_mac_addr_t mst_mac_addr);

// UPSID change call back function type
//
// Registered functions are called when an UPSID changes for an already
// existing switch. This is a rare event that only occurs when inserting
// an already powered on switch into a stack and the new switch has a
// preferred UPSID which conflicts with another switch in the stack.
//
// To get the new UPSID value(s), topo_isid_port2upsid() must be called.
//
// Registered functions will only be called on master, not on slaves.
typedef void (*topo_upsid_change_callback_t)(vtss_isid_t isid);

// ===========================================================================
// API functions
// ---------------------------------------------------------------------------

// Initialize module
mesa_rc topo_init(vtss_init_data_t *data);

// Set whether any ports on chip has been enabled as mirror port
mesa_rc topo_have_mirror_port_set(
    const topo_chip_idx_t chip_idx,
    const mesa_port_no_t  mirror_port);

// Set adm state of stack port
mesa_rc topo_stack_port_adm_state_set(
    const mesa_port_no_t port_no,
    const BOOL           adm_up);

// Set IPv4 address. Host order.
mesa_rc topo_ipv4_addr_set(
    const mesa_ipv4_t ipv4_addr);


// Get copy of switch information table
// "id" field of sit_entry is set to ISID (0 if no ISID assigned)
mesa_rc topo_sit_get(
    topo_sit_t *const sit_p);

// Register function to be called upon topo state changes
mesa_rc topo_state_change_callback_register(
    const topo_state_change_callback_t callback,
    const vtss_module_id_t             module_id);

// Register function to be called upon master change (before TTL reconfiguration)
mesa_rc topo_mst_change_callback_register(
    const topo_mst_change_callback_t   callback,
    const vtss_module_id_t             module_id);

// Register function to be called upon UPSID changes
mesa_rc topo_upsid_change_callback_register(
    const topo_upsid_change_callback_t callback,
    const vtss_module_id_t             module_id);


/** Stacking (port) configuration stored in flash*/
typedef struct {
    BOOL             stacking; /**< Set to TRUE to enable stacking.*/
    mesa_port_no_t port_0; /**< Stack port numbers. For dual-chip switches, port_0 is stack port on primary chip.*/
    mesa_port_no_t port_1; /**< Stack port numbers. For dual-chip switches, port_1 is stack port on secondary chip.*/
} topo_stack_config_t;

// Topo parameter get/set
// ----------------------
/* Get/set Topo parameters. E.g. from CLI. Intended for debug purposes */
typedef enum {
    TOPO_PARM_MST_ELECT_PRIO,
    TOPO_PARM_MST_TIME_IGNORE,
    TOPO_PARM_SPROUT_UPDATE_INTERVAL_SLV,
    TOPO_PARM_SPROUT_UPDATE_INTERVAL_MST,
    TOPO_PARM_SPROUT_UPDATE_AGE_TIME,
    TOPO_PARM_SPROUT_UPDATE_LIMIT,
    TOPO_PARM_FAST_MAC_AGE_TIME,
    TOPO_PARM_FAST_MAC_AGE_COUNT,
    TOPO_PARM_UID_0_0, // Read-access only
    TOPO_PARM_UID_0_1, // Read-access only
    TOPO_PARM_UID_1_0, // Read-access only
    TOPO_PARM_UID_1_1, // Read-access only
    TOPO_PARM_FW_VER_MODE, // Only supported for V2
    TOPO_PARM_CMEF_MODE,   // Only supported for V2
    TOPO_PARM_ISID_TBL // Only for use internally in Topo
} topo_parm_t;

#define TOPO_MST_TIME_MIN 30     /** If no switch has set mst_time_ignore then:
                                  *      if any switch(es) has been master for more than than
                                  *      VTSS_APPL_TOPO_MST_TIME_MIN seconds, then choose the one, which
                                  *      has been master for the longest period of time.*/

#define TOPO_PARM_MST_ELECT_PRIO_MIN 1
#define TOPO_PARM_MST_ELECT_PRIO_MAX 4

#define TOPO_PARM_SPROUT_UPDATE_INTERVAL_MIN 1
#define TOPO_PARM_SPROUT_UPDATE_AGE_TIME_MIN 1
#define TOPO_PARM_SPROUT_UPDATE_LIMIT_MIN    1
#define TOPO_PARM_FAST_MAC_AGE_TIME_MIN      1
#define TOPO_PARM_FAST_MAC_AGE_COUNT_MIN     1

/* Get parameter value */
int topo_parm_get(const topo_parm_t parm);

// Set parameter value
// isid=0 => Local switch
// For all parameters, except TOPO_PARM_MST_ELECT_PRIO, only isid=0
// is supported (i.e. only local switch can be controlled).
vtss_appl_topo_error_t topo_parm_set(vtss_isid_t       isid,
                                     const topo_parm_t parm,
                                     const int         val);

// Get port number through which isid is to be reached from primary unit.
// Result is one of:
//   0:
//     No switch assigned to this ISID or ISID is this switch.
//   PORT_NO_STACK_0:
//     Stack port 0.
//   PORT_NO_STACK_1:
//     Stack port 1.
mesa_port_no_t topo_isid2port(const vtss_isid_t isid);


// Get port number thorugh which switch with mac_addr is to be reached from primary unit.
// Result is one of:
//   0:
//     No switch with this MAC address is known to Topo.
//   PORT_NO_STACK_0:
//     Stack port 0.
//   PORT_NO_STACK_1:
//     Stack port 1.
mesa_port_no_t topo_mac2port(const mesa_mac_addr_t mac_addr);

// Address Translation
// -------------------
// Translate USID to ISID
vtss_isid_t topo_usid2isid(const vtss_usid_t usid);

// Translate ISID to USID
vtss_usid_t topo_isid2usid(const vtss_isid_t isid);

// Translate ISID to MAC address
// VTSS_APPL_TOPO_ERROR_GEN is returned if no ISID is assigned to MAC address
mesa_rc topo_isid2mac(
    const vtss_isid_t isid,
    mesa_mac_addr_t mac_addr);

// Translate MAC address to ISID
// If MAC address is not known, 0 is returned.
vtss_isid_t topo_mac2isid(const mesa_mac_addr_t mac_addr);

// Translate (ISID, port number)  to UPSID
//
// isid argument may be set to VTSS_ISID_LOCAL.
// topo_isid_port2upsid() is not supported on slaves and it may
// return VTSS_VSTAX_UPSID_UNDEF in transient scenarios (i.e.
// unstable stacks).
//
// Setting #port_no to VTSS_PORT_NO_NONE, will cause the function
// to return the UPSID of the primary unit.
//
// IMPORTANT:
// UPSIDs are NOT static.
// They may - under rare circumstances - change without causing any
// switch add/delete events. Any module, which uses UPSIDs must thus
// subscribe to UPSID changes using topo_upsid_change_callback_register().
mesa_vstax_upsid_t topo_isid_port2upsid(
    const vtss_isid_t    isid,
    const mesa_port_no_t port_no);


// Translate UPSID to ISID
//
// For local UPSIDs, the actual ISID is returned (not VTSS_ISID_LOCAL).
//
// IMPORTANT:
// UPSIDs are NOT static. See note for topo_isid_port2upsid().
vtss_isid_t topo_upsid2isid(const mesa_vstax_upsid_t upsid);

// Monitoring
// ----------
// These functions are intended only for monitoring purposes, e.g.
// CLI and web.

typedef enum {
    TOPO_STACK_PORT_FWD_MODE_NONE,   // Unreachable
    TOPO_STACK_PORT_FWD_MODE_LOCAL,  // Local chip
    TOPO_STACK_PORT_FWD_MODE_ACTIVE, // Unreachable
    TOPO_STACK_PORT_FWD_MODE_BACKUP, // Unreachable
} topo_stack_port_fwd_mode_t;

typedef struct {
    // Distance to chip from primary chip of this switch
    // The primary chip is the chip with active CPU.
    //
    // dist[0] is distance via external stack port on primary chip.
    // dist[1] is distance through secondary chip.
    //
    // -1 = Unreachable
    // 0  = Local chip
    vtss_sprout_dist_t           dist_pri[2];

    // Distance to chip from secondary chip of this switch
    // The secondary chip is the chip with inactive CPU.
    //
    // dist[0] is distance via external stack port on secondary chip.
    // dist[1] is distance through primary chip.
    //
    // -1 = Unreachable
    // 0  = Local chip
    vtss_sprout_dist_t           dist_sec[2];

    // Distance string for each external stack port
    char dist_str[2][sizeof("11-12")];

    // Forwarding mode (Active/Backup) to this chip for each external stack port
    topo_stack_port_fwd_mode_t stack_port_fwd_mode[2];

    // Stack port used to reach unit from this switch's primary unit.
    // 0 => Local unit
    //
    // When equal length paths exists (for SPROUT v2), then shortest path
    // is the path used for multicast/flooding.
    mesa_port_no_t               shortest_path;

    // Number of UPSes of chip
    ushort             ups_cnt;

    // UPSIDs for UPSes of chip
    mesa_vstax_upsid_t upsid[2];

    // Port masks with front ports for UPSes of chip
    u64                ups_port_mask[2];
} topo_chip_t;


typedef struct {
    BOOL               vld;

    // Switch Information
    // ------------------
    BOOL               me;

    mesa_mac_addr_t    mac_addr;
    vtss_usid_t        usid; // 0 if no ISID/USID assigned
    vtss_isid_t        isid; // 0 if no ISID/USID assigned

    BOOL               present;

    BOOL                         mst_capable;
    vtss_sprout_mst_elect_prio_t mst_elect_prio;
    time_t                       mst_time;
    BOOL                         mst_time_ignore;

    mesa_ipv4_t                  ip_addr;

    uint                         chip_cnt;

    // Chip Information
    // ----------------
    // [0] is primary chip (chip with CPU active)
    // [1] is secondary chip (if present)
    topo_chip_t                  chip[2];
} topo_switch_t;

// Sort order:
//   a) Present, ISID assigned, rising USID order
//   b) Present, no ISID assigned
//   c) Not present, ISID assigned, rising USID order
typedef struct {
    mesa_mac_addr_t mst_switch_mac_addr;
    ulong           mst_change_time;

    topo_switch_t ts[VTSS_SPROUT_SIT_SIZE + 15];
} topo_switch_list_t;

// Get topo switch table. tbl must be malloc'ed by caller.
mesa_rc topo_switch_list_get(topo_switch_list_t *const tsl_p);

const char *topo_stack_port_fwd_mode_to_str(const topo_stack_port_fwd_mode_t stack_port_fwd_mode);

// Debug functions
// ---------------
typedef int (*topo_dbg_printf_t)(const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));

// Print ISID/USID mapping and associated MAC addresses
void topo_dbg_isid_tbl_print(
    const topo_dbg_printf_t  dbg_printf);

// Set parameter value to default
// Both global and local parameters are reset.
void topo_parm_set_default(void);

void topo_dbg_sprout(
    const topo_dbg_printf_t  dbg_printf,
    const uint               parms_cnt,
    const ulong *const       parms);

void topo_dbg_test(
    const topo_dbg_printf_t  dbg_printf,
    const int arg1,
    const int arg2,
    const int arg3);

// When updating firmware, slaves no longer
// update their LED display, but retain the
// latest value received from the master, because
// the firmware image may be so large that
// the slave would otherwise time out and show
// two dots rather than its USID.
void topo_led_update_set(BOOL enable);

//  Mib and JSON initialization
void topo_mib_init(void);
void vtss_appl_topo_json_init(void);

// ===========================================================================

#ifdef __cplusplus
}
#endif
#endif /* VTSS_SWITCH_STANDALONE */
#endif // _TOPO_API_H_

