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

#ifndef _VTSS_PVLAN_H_
#define _VTSS_PVLAN_H_

#include "main.h"
#include "msg_api.h"
#include "critd_api.h"
#include "pvlan_api.h"

// This module supports two kinds of private VLANs, srcmask-based PVLANs
// and port-isolation.
//
// srcmask-based PVLANs:
//   These only work within one single switch, and are implemented via
//   a port's srcmask: When a frame arrives on the port, the set of
//   destination ports are looked up, and afterwards masked with the
//   srcmask. The frame can only be forwarded to ports that have a '1'
//   in the corresponding bit in the ingress port's srcmask.
//
// port-isolation:
//   If a port on a given switch is isolated, it can only transmit
//   to non-isolated ports, and can only receive from non-isolated
//   ports. This works across the stack because the stack header contains
//   a bit that tells whether the frame was originally received on an
//   isolated frontport.
//   Here is how it goes: A frame arrives on a front port. Its
//   classified VID is looked up in the VLAN table. A bit in this entry
//   indicates if the port is a private VLAN. If so, the ANA::PRIV_VLAN_MASK
//   register is consulted. If ANA::PRIV_VLAN_MASK[ingress_port] is 1,
//   the normal VLAN_PORT_MASK (also looked up in the VLAN table) is used.
//   In that case the ingress port is "promiscuous", which means that it can send
//   to all ports in the classified VLAN. If ANA::PRIV_VLAN_MASK[ingress_port]
//   is 0, the ANA::PRIV_VLAN_MASK is ANDed with VLAN_PORT_MASK to form the
//   final set of ports that are member of this VID. This feature works
//   across a stack, because the stack header contains a bit (let's call it
//   "isolated ingress port", indicating if the original ingress port was
//   an isolated port or a promiscuous port (according to the above, a port is
//   isolated when the VLAN table's private VLAN bit is 1 and
//   ANA::PRIV_VLAN_MASK[ingress_port]==0).
//   On the neighboring switch, the VLAN table is consulted again to find that
//   switch's VLAN_PORT_MASK, which in turn is ANDed with ANA::PRIV_VLAN_MASK if
//   the stack header's "isolated ingress port" is set, otherwise it's not
//   ANDed. So ANA::PRIV_VLAN_MASK[stack_port] is never used.
//
// To bring the confusion to a higher level, the datasheet talks about
// port-isolation as private VLANs, while this source code (for legacy
// reasons) uses the term private VLANs for srcmask-based VLANs.
//
// It has been decided to only include the srcmask-based PVLANs in
// the standalone version of the software, but it could "easily" be
// implemented for the stacking solution, but it would probably confuse
// the users that it only works for a single switch in the stack.
// In order to be able to enable srcmask-based PVLANs in this module,
// even for the stacking image, a new #define is made, which currently
// is based on VTSS_SWITCH_STANDALONE, but could be changed to a hard-
// coded 1 if needed. This would require changes to web.c and cli.c
// as well, so that this module's API functions are called.
// In pvlan_api.h, PVLAN_SRC_MASK_ENA is defined if this is included

// In the following, structs prefixed with SM concern the srcmask-based
// PVLANs, and structs prefixed by PI are about the port-isolation.

/* ========================================================= *
 * Trace definitions
 * ========================================================= */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_PVLAN

#include <vtss_trace_api.h>

/* ================================================================= *
 * VLAN configuration blocks
 * ================================================================= */
#define PVLAN_DELETE   1
#define PVLAN_ADD      0

#if defined(PVLAN_SRC_MASK_ENA)
typedef struct {
    mesa_pvlan_no_t  privatevid;           /* Private VLAN ID    */
    mesa_port_list_t ports[VTSS_ISID_END]; /* Port list          */
} SM_entry_conf_t;

/* Private VLAN entry */
typedef struct SM_pvlan_t {
    struct SM_pvlan_t *next;                         /* Next in list       */
    SM_entry_conf_t conf;                            /* Configuration      */
} SM_pvlan_t;

/* Private VLAN lists */
typedef struct {
    SM_pvlan_t *used;                                /* Active list        */
    SM_pvlan_t *free;                                /* Free list          */
} SM_pvlan_list_t;
#endif /* PVLAN_SRC_MASK_ENA */

/* ==========================================================================
 *
 * Private VLAN Global Structure
 * =========================================================================*/

typedef struct {
    critd_t           crit;
    mesa_port_list_t  PI_conf[VTSS_ISID_END]; // Entries [VTSS_ISID_START; VTSS_ISID_END] hold stack conf, VTSS_ISID_LOCAL this switch's.

#if defined(PVLAN_SRC_MASK_ENA)
    SM_pvlan_list_t   switch_pvlan; // Holds this switch's actual configuration (entry VTSS_ISID_LOCAL used, only).
    SM_pvlan_list_t   stack_pvlan;  // Holds the whole stack's configuration (entries [VTSS_ISID_START; VTSS_ISID_END[ used).

    CapArray<SM_pvlan_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> pvlan_switch_table;
    CapArray<SM_pvlan_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> pvlan_stack_table;
#endif /* PVLAN_SRC_MASK_ENA */
} pvlan_global_t;

#endif /* _VTSS_PVLAN_H_ */

