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
#include "ipmc_lib.hxx"
#include "ipmc_lib_base.hxx"
#include "ipmc_lib_pdu.hxx"
#ifdef VTSS_SW_OPTION_SMB_IPMC
#include "ipmc_lib_profile.hxx"
#endif
#include "ipmc_lib_utils.hxx" /* For ipmc_lib_util_compatibility_to_str() */
#include "ipmc_lib_trace.h"
#include "ip_utils.hxx"       /* For mesa_ipv6_t::operator<()             */
#include <vtss/appl/ipmc_lib.h>

#define IPMC_LIB_BASE_QUERY_SUPPRESSION_TIMEOUT 2 /* seconds */

/******************************************************************************/
// ipmc_lib_grp_key_t::operator<()
/******************************************************************************/
bool operator<(const ipmc_lib_grp_key_t &lhs, const ipmc_lib_grp_key_t &rhs)
{
    // First sort by vlan_key, that is, first by Protocol (IPMC before MVR),
    // then by IP family (IPv4 before IPv6), then by VID.
    // Then sort by group address.
    // This is in order to easier support IPMC iterations separate from MVR
    // iterations and IGMP iterations separate from MLD iterations for the
    // management interfaces.

    // Using vtss_appl_ipmc_lib_vlan_key_t::operator!=() and
    // vtss_appl_ipmc_lib_vlan_key_t::operator<()
    if (lhs.vlan_key != rhs.vlan_key) {
        return lhs.vlan_key < rhs.vlan_key;
    }

    if (lhs.grp_addr.is_ipv4 != rhs.grp_addr.is_ipv4) {
        return lhs.grp_addr.is_ipv4;
    }

    if (lhs.grp_addr.is_ipv4) {
        return lhs.grp_addr.ipv4 < rhs.grp_addr.ipv4;
    }

    return lhs.grp_addr.ipv6 < rhs.grp_addr.ipv6;
}

/******************************************************************************/
// IPMC_LIB_BASE_next_src_timeout_update()
// This flavor of the function updates the next timeout of a particular source
// address based on the individual ports' timeout.
// The function returns true if this gave rise to a change, false if not.
/******************************************************************************/
static bool IPMC_LIB_BASE_next_src_timeout_update(ipmc_lib_src_state_t &src_state)
{
    mesa_port_no_t port_no;
    uint32_t       next_timeout = 0;
    bool           result;

    // Only ports set in the "Included List" or "Requested List", which is the
    // (include_port_list[] in this implmentation, are subject to timeout.
    // Corresponds to RFC3376, section 6.4.1's 3rd paragraph's definition of
    // "A".
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!src_state.include_port_list[port_no]) {
            continue;
        }

        ipmc_lib_src_port_state_t &src_port_state = src_state.ports[port_no];

        if (src_port_state.src_timeout == 0) {
            // Irrespective of the filter_mode, a source in the
            // "Include List"/"Requested List" must have a src_timeout > 0.
            T_EG(IPMC_LIB_TRACE_GRP_BASE, "src_state = %s: src_timeout is zero in include/requested list", src_state);

            // Set it to a low value in order to take care of it on the next
            // timer tick
            src_port_state.src_timeout = 1;
        }

        if (next_timeout == 0 || next_timeout > src_port_state.src_timeout) {
            next_timeout = src_port_state.src_timeout;
        }
    }

    result = src_state.next_src_timeout != next_timeout;
    src_state.next_src_timeout = next_timeout;

    return result;
}

/******************************************************************************/
// IPMC_LIB_BASE_next_src_timeout_update()
// This flavor of the function updates the next timeout of a source map based on
// the individual source addresses' next timeout.
/******************************************************************************/
static void IPMC_LIB_BASE_next_src_timeout_update(ipmc_lib_src_map_t &src_map)
{
    ipmc_lib_src_itr_t src_itr;
    uint32_t           next_timeout = 0;

    for (src_itr = src_map.begin(); src_itr != src_map.end(); ++src_itr) {
        ipmc_lib_src_state_t &src_state = src_itr->second;

        if (src_state.next_src_timeout) {
            if (next_timeout == 0 || next_timeout > src_state.next_src_timeout) {
                next_timeout = src_state.next_src_timeout;
            }
        }
    }

    src_map.next_src_timeout = next_timeout;
}

/******************************************************************************/
// IPMC_LIB_BASE_next_query_timeout_update()
// This flavor of the function updates the next query timeout of a particular
// source address based on the individual ports' query timeout.
// The function returns true if this gave rise to a change, false if not.
/******************************************************************************/
static bool IPMC_LIB_BASE_next_query_timeout_update(ipmc_lib_src_state_t &src_state)
{
    mesa_port_no_t port_no;
    uint32_t       next_timeout = 0;
    bool           result;

    // Only ports set in the "Included List" or "Requested List", which is the
    // (include_port_list[] in this implmentation, are subject to timeout.
    // Corresponds to RFC3376, section 6.4.1's 3rd paragraph's definition of
    // "A".
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!src_state.include_port_list[port_no]) {
            continue;
        }

        ipmc_lib_src_port_state_t &src_port_state = src_state.ports[port_no];

        if (src_port_state.query_timeout) {
            if (next_timeout == 0 || next_timeout > src_port_state.query_timeout) {
                next_timeout = src_port_state.query_timeout;
            }
        }
    }

    result = src_state.next_query_timeout != next_timeout;
    src_state.next_query_timeout = next_timeout;

    return result;
}

/******************************************************************************/
// IPMC_LIB_BASE_next_query_timeout_update()
// This flavor of the function updates the next query timeout of a source map
// based on the individual source addresses' next timeout.
/******************************************************************************/
static void IPMC_LIB_BASE_next_query_timeout_update(ipmc_lib_src_map_t &src_map)
{
    ipmc_lib_src_itr_t src_itr;
    uint32_t           next_timeout = 0;

    for (src_itr = src_map.begin(); src_itr != src_map.end(); ++src_itr) {
        ipmc_lib_src_state_t &src_state = src_itr->second;

        if (src_state.next_query_timeout) {
            if (next_timeout == 0 || next_timeout > src_state.next_query_timeout) {
                next_timeout = src_state.next_query_timeout;
            }
        }
    }

    src_map.next_query_timeout = next_timeout;
}

/******************************************************************************/
// IPMC_LIB_BASE_mesa_tcam_add()
/******************************************************************************/
static bool IPMC_LIB_BASE_mesa_tcam_add(ipmc_lib_global_state_t &glb, ipmc_lib_grp_itr_t &grp_itr, const vtss_appl_ipmc_lib_ip_t &sip, mesa_port_list_t &fwd_list)
{
    bool    is_ipv4 = grp_itr->first.grp_addr.is_ipv4;
    mesa_rc rc;

    T_IG(IPMC_LIB_TRACE_GRP_API, "%s: mesa_ipv%c_mc_add(vid = %u, sip = %s, dip = %s, port_list = %s)", grp_itr->first, is_ipv4 ? '4' : '6', grp_itr->first.vlan_key.vid, sip, grp_itr->first.grp_addr, fwd_list);
    if (grp_itr->first.grp_addr.is_ipv4) {
        rc = mesa_ipv4_mc_add(nullptr, grp_itr->first.vlan_key.vid, sip.ipv4, grp_itr->first.grp_addr.ipv4, &fwd_list);
    } else {
        rc = mesa_ipv6_mc_add(nullptr, grp_itr->first.vlan_key.vid, sip.ipv6, grp_itr->first.grp_addr.ipv6, &fwd_list);
    }

    if (rc != VTSS_RC_OK) {
        // We cannot make a trace error here, because it might be that the chip
        // has run out of resources, in which case we switch to using the VLAN
        // table.
        T_IG(IPMC_LIB_TRACE_GRP_API, "%s: mesa_ipv%c_mc_add(vid = %u, sip = %s, dip = %s, port_list = %s) failed: %s", grp_itr->first, is_ipv4 ? '4' : '6', grp_itr->first.vlan_key.vid, sip, grp_itr->first.grp_addr, fwd_list, error_txt(rc));
        return false;
    }

    return true;
}

/******************************************************************************/
// IPMC_LIB_BASE_mesa_tcam_del()
/******************************************************************************/
static void IPMC_LIB_BASE_mesa_tcam_del(ipmc_lib_global_state_t &glb, ipmc_lib_grp_itr_t &grp_itr, const vtss_appl_ipmc_lib_ip_t &sip)
{
    bool    is_ipv4 = grp_itr->first.grp_addr.is_ipv4;
    mesa_rc rc;

    T_IG(IPMC_LIB_TRACE_GRP_API, "%s: mesa_ipv%c_mc_del(vid = %u, sip = %s, dip = %s)", grp_itr->first, is_ipv4 ? '4' : '6', grp_itr->first.vlan_key.vid, sip, grp_itr->first.grp_addr);
    if (grp_itr->first.grp_addr.is_ipv4) {
        rc = mesa_ipv4_mc_del(nullptr, grp_itr->first.vlan_key.vid, sip.ipv4, grp_itr->first.grp_addr.ipv4);
    } else {
        rc = mesa_ipv6_mc_del(nullptr, grp_itr->first.vlan_key.vid, sip.ipv6, grp_itr->first.grp_addr.ipv6);
    }

    if (rc != VTSS_RC_OK) {
        // When deleting an entry, we can indeed throw a trace error if MESA
        // returns an error, because we know that it is already there.
        T_EG(IPMC_LIB_TRACE_GRP_API, "%s: mesa_ipv%c_mc_del(vid = %u, sip = %s, dip = %s) failed: %s", grp_itr->first, is_ipv4 ? '4' : '6', grp_itr->first.vlan_key.vid, sip, grp_itr->first.grp_addr, error_txt(rc));
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_mesa_mac_table_add()
/******************************************************************************/
static bool IPMC_LIB_BASE_mesa_mac_table_add(ipmc_lib_global_state_t &glb, ipmc_lib_grp_itr_t &grp_itr, mesa_port_list_t &fwd_list)
{
    // RBNTBD: We need to traverse all groups to see if more than one group
    // hashes to the same DMAC, and if so we need to add the ports for that
    // group as well.
    return false;
}

/******************************************************************************/
// IPMC_LIB_BASE_mesa_mac_table_del()
/******************************************************************************/
static bool IPMC_LIB_BASE_mesa_mac_table_del(ipmc_lib_global_state_t &glb, ipmc_lib_grp_itr_t &grp_itr)
{
    // RBNTBD: We need to traverse all groups to see if there are more groups
    // that hashes to the same DMAC, and if so we cannot remove it from the
    // MAC table, but need to update it.
    return false;
}

/******************************************************************************/
// IPMC_LIB_BASE_mesa_add()
// This function is used to both update and add a TCAM or MAC table entry.
/******************************************************************************/
static void IPMC_LIB_BASE_mesa_add(ipmc_lib_global_state_t &glb, ipmc_lib_grp_itr_t &grp_itr, const vtss_appl_ipmc_lib_ip_t &sip, ipmc_lib_src_state_t &src_state, mesa_port_list_t &fwd_list)
{
    bool tcam_support = grp_itr->first.grp_addr.is_ipv4 ? glb.lib_cap.ssm_chip_support_ipv4 : glb.lib_cap.ssm_chip_support_ipv6;

    // Check if we already have an entry for this <group, source>
    switch (src_state.hw_location) {
    case VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE:
    case VTSS_APPL_IPMC_LIB_HW_LOCATION_TCAM:
        if (tcam_support) {
            // Whether we don't have one or already have one in TCAM, we need to
            // do the same, because there is no such function as
            // mesa_ipv4_mc_get() or mesa_ipv6_mc_get().
            if (IPMC_LIB_BASE_mesa_tcam_add(glb, grp_itr, sip, fwd_list)) {
                src_state.hw_location = VTSS_APPL_IPMC_LIB_HW_LOCATION_TCAM;
                break;
            } else if (src_state.hw_location == VTSS_APPL_IPMC_LIB_HW_LOCATION_TCAM) {
                // The entry was in the chip when we got in here, so why did it fail
                // now? Anyway, remove it from the chip again and fall back on
                // MAC table-based forwarding.
                IPMC_LIB_BASE_mesa_tcam_del(glb, grp_itr, sip);
                src_state.hw_location = VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE;
            }
        }

    // Fall through

    case VTSS_APPL_IPMC_LIB_HW_LOCATION_MAC_TABLE:
        if (IPMC_LIB_BASE_mesa_mac_table_add(glb, grp_itr, fwd_list)) {
            src_state.hw_location = VTSS_APPL_IPMC_LIB_HW_LOCATION_MAC_TABLE;
        } else {
            src_state.hw_location = VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE;
        }

        break;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_mesa_del()
/******************************************************************************/
static void IPMC_LIB_BASE_mesa_del(ipmc_lib_global_state_t &glb, ipmc_lib_grp_itr_t &grp_itr, const vtss_appl_ipmc_lib_ip_t &sip, ipmc_lib_src_state_t &src_state)
{
    switch (src_state.hw_location) {
    case VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE:
        // This entry is not in H/W. Skip it.
        return;

    case VTSS_APPL_IPMC_LIB_HW_LOCATION_TCAM:
        // This <group, source> is in TCAM. Delete it.
        IPMC_LIB_BASE_mesa_tcam_del(glb, grp_itr, sip);
        break;

    case VTSS_APPL_IPMC_LIB_HW_LOCATION_MAC_TABLE:
        IPMC_LIB_BASE_mesa_mac_table_del(glb, grp_itr);
        break;
    }

    src_state.hw_location = VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE;
}

/******************************************************************************/
// IPMC_LIB_BASE_active_ports_update()
// This function can only clear ports in grp_itr->second.active_ports[] and
// never set them.
/******************************************************************************/
static void IPMC_LIB_BASE_active_ports_update(ipmc_lib_grp_itr_t &grp_itr)
{
    ipmc_lib_src_itr_t   src_itr;
    ipmc_lib_grp_state_t &grp_state = grp_itr->second;
    ipmc_lib_src_map_t   &src_map   = grp_state.src_map;
    mesa_port_list_t     ports_still_in_use;

    ports_still_in_use.clear_all();
    for (src_itr = src_map.begin(); src_itr != src_map.end(); ++src_itr) {
        ports_still_in_use |= src_itr->second.include_port_list | src_itr->second.exclude_port_list;
    }

    // If filter_mode[port] == INCLUDE and ports_still_in_use[port] == false,
    // the port is no longer in use. This simply means: We include no sources,
    // that is, we never forward anything to that port on this group.

    // If, however, filter_mode[port] == EXCLUDE and ports_still_in_use[port]
    // == false, we are still active on the port, because this means: We exclude
    // no sources, that is, we forward all sources to this port.

    // Therefore, we need to add all those ports that are in EXCLUDE mode:
    ports_still_in_use |= grp_state.exclude_mode_ports;

    // Then normalize with the existing active port list, in order not to mark
    // more ports as active than actually are.
    ports_still_in_use &= grp_state.active_ports;

    if (ports_still_in_use != grp_state.active_ports) {
        T_IG(IPMC_LIB_TRACE_GRP_API, "grp = %s: Active ports changed from %s to %s", grp_itr->first, grp_state.active_ports, ports_still_in_use);
        grp_state.active_ports = ports_still_in_use;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_aggr_port_mask_get()
/******************************************************************************/
static void IPMC_LIB_BASE_aggr_port_mask_get(ipmc_lib_global_state_t &glb, mesa_port_list_t &fwd_list)
{
    mesa_port_no_t port_no;

    if (fwd_list.is_empty()) {
        // Nothing to further add.
        return;
    }

    // The aggr_port_masks[] are symmetric in the sense that if
    // aggr_port_masks[X] has Y set, then aggr_port_masks[Y] has X set, at least
    // after the aggregation module stabilizes, so there is no need to go
    // through the port list twice.
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!fwd_list[port_no]) {
            continue;
        }

        fwd_list |= glb.lists->aggr_port_masks[port_no];
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_mesa_update()
/******************************************************************************/
static void IPMC_LIB_BASE_mesa_update(ipmc_lib_global_state_t &glb, ipmc_lib_grp_itr_t &grp_itr)
{
    ipmc_lib_grp_state_t    &grp_state = grp_itr->second;
    ipmc_lib_src_state_t    &asm_state = grp_state.asm_state;
    ipmc_lib_src_map_t      &src_map   = grp_state.src_map;
    mesa_port_list_t        tmp_port_list, fwd_list, router_port_list;
    ipmc_lib_src_itr_t      src_itr, src_itr_next;
    vtss_appl_ipmc_lib_ip_t zero_sip;
    bool                    no_active_ports;

    // The callers of this function mainly set and clear the port members of the
    // source addresses and don't really care about the active ports (these are
    // only marked as active when receiving a report).
    // However, changing filter mode and updating the source addresses may cause
    // the active ports to be fewer. This is what we try to figure out in the
    // following function.
    IPMC_LIB_BASE_active_ports_update(grp_itr);

    no_active_ports = grp_state.active_ports.is_empty();

    // We need the router port list.
    router_port_list = ipmc_lib_base_router_port_mask_get(glb);

    // Create the 0.0.0.0 or :: IP address.
    vtss_clear(zero_sip);
    zero_sip.is_ipv4 = grp_itr->first.grp_addr.is_ipv4;

    // If all ports are now inactive, we delete all rules.
    if (no_active_ports) {
        asm_state.changed = true;
        src_map.changed   = true;
    }

    // Normalize exclude_mode_ports, so that only the ones that are active can
    // be set
    grp_state.exclude_mode_ports &= grp_state.active_ports;

    // First update the ASM entry, if needed.
    if (asm_state.changed) {
        if (no_active_ports) {
            IPMC_LIB_BASE_mesa_del(glb, grp_itr, zero_sip, asm_state);
        } else {
            // The ASM entry must forward to these ports:
            //    1) All router ports.
            //    2) All active ports in EXCLUDE mode
            //    3) All the ports we have added to fwd_list may have aggregated
            //       partner ports that we also need to add, or it may happen
            //       that the traffic doesn't reache the destination.
            fwd_list = router_port_list;
            fwd_list |= grp_state.exclude_mode_ports;
            IPMC_LIB_BASE_aggr_port_mask_get(glb, fwd_list);

            // Let management interfaces know which ports are 'default'
            // forwarding.
            asm_state.include_port_list = grp_state.exclude_mode_ports;
            IPMC_LIB_BASE_mesa_add(glb, grp_itr, zero_sip, asm_state, fwd_list);
        }

        asm_state.changed = false;
    }

    if (src_map.changed) {
        // Now time to go through the sources.
        src_itr = src_map.begin();
        while (src_itr != src_map.end()) {
            ipmc_lib_src_state_t &src_state = src_itr->second;

            src_itr_next = src_itr;
            ++src_itr_next;

            if (!no_active_ports && !src_state.changed) {
                goto next;
            }

            // Normalize include_port_list and exclude_port_list
            src_state.include_port_list &= grp_state.active_ports;
            src_state.exclude_port_list &= grp_state.active_ports;

            if (no_active_ports || (src_state.include_port_list.is_empty() && src_state.exclude_port_list.is_empty())) {
                // This entry is not used anymore. Delete it.
                IPMC_LIB_BASE_mesa_del(glb, grp_itr, src_itr->first, src_itr->second);
                src_map.erase(src_itr);
                goto next;
            }

            // 1) Add all router ports.
            fwd_list = router_port_list;

            // 2) Add all active ports in INCLUDE mode, where include_port_list[port] == true.
            // We have just normalized include_port_list[], so that only active
            // ports are set in the port list.
            // The group's exclude_mode_ports has bits set for the ports that
            // are in EXCLUDE mode. The negated form then contains ports that
            // are in INCLUDE mode (and also ports that have not received any
            // IPMC reports).
            // By ANDing theses two together, we get the list of ports that
            // should be forwarding (and now ports that have not received any
            // IPMC reports are cleared).
            tmp_port_list = src_state.include_port_list & ~grp_state.exclude_mode_ports;
            fwd_list |= tmp_port_list;

            // 3) Add all active ports in EXCLUDE mode, where exclude_port_list[port] == false.
            // Those ports that are in EXCLUDE state are ANDed with those ports
            // that are not being blocked.
            tmp_port_list = ~src_state.exclude_port_list & grp_state.exclude_mode_ports;
            fwd_list |= tmp_port_list;

            // All the ports we have added to fwd_list may have aggregated
            // partner ports that we also need to add, or it may happen that the
            // traffic doesn't reach the destination.
            IPMC_LIB_BASE_aggr_port_mask_get(glb, fwd_list);

            IPMC_LIB_BASE_mesa_add(glb, grp_itr, src_itr->first, src_itr->second, fwd_list);
            src_state.changed = false;

next:
            src_itr = src_itr_next;
        }

        src_map.changed = false;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_grp_mask_all_as_changed()
/******************************************************************************/
static void IPMC_LIB_BASE_grp_mark_all_as_changed(ipmc_lib_grp_state_t &grp_state)
{
    ipmc_lib_src_itr_t src_itr;

    // Changing filter mode also changes the ASM entry.
    grp_state.asm_state.changed = true;

    // Changing filter mode may also change existing src_map entries, so
    // mark all as changed.
    if (!grp_state.src_map.empty()) {
        for (src_itr = grp_state.src_map.begin(); src_itr != grp_state.src_map.end(); ++src_itr) {
            src_itr->second.changed = true;
        }

        // And mark the src_map as changed. This will work even when the source
        // map is empty.
        grp_state.src_map.changed = true;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_filter_mode_set()
/******************************************************************************/
static void IPMC_LIB_BASE_filter_mode_set(ipmc_lib_grp_itr_t &grp_itr, mesa_port_no_t port_no, vtss_appl_ipmc_lib_filter_mode_t new_filter_mode)
{
    ipmc_lib_grp_state_t &grp_state = grp_itr->second;

    // Change filter mode.
    grp_state.exclude_mode_ports[port_no] = new_filter_mode == VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE;

    if (new_filter_mode == VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE) {
        // Group timer not used in INCLUDE mode.
        grp_state.ports[port_no].grp_timeout = 0;
    }

    IPMC_LIB_BASE_grp_mark_all_as_changed(grp_state);
}

/******************************************************************************/
// IPMC_LIB_BASE_older_version_querier_present_interval_get()
/******************************************************************************/
static uint32_t IPMC_LIB_BASE_older_version_querier_present_interval_get(const ipmc_lib_vlan_state_t &vlan_state)
{
    // RFC3376, section 8.12.
    // RFC3810, section 9.12.

    // This value MUST be ([Robustness Variable] times (the [Query Interval]
    // in the last Query received)) plus [Query Response Interval].
    //
    // We take these values from the internal_state, because we are not
    // necessarily the querier (and if we were, the internal state variables are
    // identical to the configured values).
    return vlan_state.internal_state.cur_rv * vlan_state.internal_state.cur_qi + vlan_state.internal_state.cur_qri / 10;
}

/******************************************************************************/
// IPMC_LIB_BASE_older_version_host_present_interval_get()
/******************************************************************************/
static uint32_t IPMC_LIB_BASE_older_version_host_present_interval_get(const ipmc_lib_vlan_state_t &vlan_state)
{
    // RFC3376, section 8.13.
    // RFC3810, section 9.13.

    // This value MUST be ([Robustness Variable] times [Query Interval])
    // plus [Query Response Interval].
    //
    // This is exactly the same as in RFC3376, section 8.12.
    return IPMC_LIB_BASE_older_version_querier_present_interval_get(vlan_state);
}

/******************************************************************************/
// IPMC_LIB_BASE_group_membership_interval_get()
/******************************************************************************/
static uint32_t IPMC_LIB_BASE_group_membership_interval_get(const ipmc_lib_vlan_state_t &vlan_state)
{
    // RFC3376, section 8.4, Group Membership Interval (GMI).
    // RFC3810, section 9.4, Multicast Address Listening Interval (MALI).

    // This value MUST be ([Robustness Variable] times [Query Interval])
    // plus [Query Response Interval].

    // This is exactly the same as in RFC3376, section 8.12.
    return IPMC_LIB_BASE_older_version_querier_present_interval_get(vlan_state);
}

/******************************************************************************/
// IPMC_LIB_BASE_last_member_query_time_get()
/******************************************************************************/
static uint32_t IPMC_LIB_BASE_last_member_query_time_get(const ipmc_lib_vlan_state_t &vlan_state)
{
    uint32_t res;
    // RFC3376, section 6.4.1, Last Member Query Time (LMQT).
    // RFC3810, section 7.2.3, Last Listener Query Time (LLQT).

    // LMQT/LLQT is the total time the router should wait for a report after the
    // querier has sent the first query. During this time, the querier should
    // send [Last Member/Listener Query Count (LMQC/LLQC)] - 1 retransmissions
    // of the query.
    // LLQT represents the "leave latency", or the difference between the
    // transmission of a listener state change and the modification of the
    // information pass to the routing protocol (blk/fwd).

    // The time between retransmissions is cur_lmqi (LMQI/LLQI) and the number
    // of retransmissions is cur_rv, so the LMQT/LLQT is cur_lmqi * cur_rv
    // (10ths of a second).
    res = (vlan_state.internal_state.cur_lmqi * vlan_state.internal_state.cur_rv) / 10;
    T_NG(IPMC_LIB_TRACE_GRP_RX, "(cur_lmqi * cur_rv) / 10 = (%u * %u) / 10 => LMQT = %u", vlan_state.internal_state.cur_lmqi, vlan_state.internal_state.cur_rv, res);
    return res;
}

/******************************************************************************/
// IPMC_LIB_BASE_other_querier_present_timeout_get()
/******************************************************************************/
static uint32_t IPMC_LIB_BASE_other_querier_present_timeout_get(const ipmc_lib_vlan_state_t &vlan_state)
{
    // RFC3376, section 8.5, Other Querier Present Interval (OQPI).
    // RFC3810, section 9.5, Other Querier Present Timeoutl (OQPT).

    // The Other Querier Present Timeout is the length of time that must pass
    // before a multicast router decides that there is no longer another
    // multicast router which should be the Querier. This value MUST be
    // ([Robustness Variable] times ([Query Interval]) plus (one half of [Query
    // Response Interval]).
    return (vlan_state.internal_state.cur_rv * vlan_state.conf.qi) + (vlan_state.internal_state.cur_qri / (10 * 2));
}

/******************************************************************************/
// IPMC_LIB_BASE_query_retransmit_tick()
/******************************************************************************/
static void IPMC_LIB_BASE_query_retransmit_tick(ipmc_lib_grp_itr_t &grp_itr, uint32_t now)
{
    ipmc_lib_vlan_state_t &vlan_state = *grp_itr->second.vlan_state;
    ipmc_lib_src_list_t   tx_list_suppress, tx_list_dont_suppress;
    ipmc_lib_src_itr_t    src_itr;
    ipmc_lib_grp_state_t  &grp_state = grp_itr->second;
    ipmc_lib_src_state_t  &asm_state = grp_state.asm_state;
    ipmc_lib_src_map_t    &src_map   = grp_state.src_map;
    mesa_port_no_t        port_no;
    uint32_t              lmqt;
    bool                  next_query_timeout_update, suppress_router_side_processing;

    lmqt = IPMC_LIB_BASE_last_member_query_time_get(vlan_state) + now;

    // Figure out whether we need to send a group-specific query
    if (asm_state.next_query_timeout != 0 && asm_state.next_query_timeout <= now) {
        // Retransmission of queries is per port, so loop through all ports
        // marked as active.
        for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
            if (!grp_state.active_ports[port_no]) {
                continue;
            }

            ipmc_lib_src_port_state_t &asm_port_state = asm_state.ports[port_no];

            if (!asm_port_state.query_timeout || asm_port_state.query_timeout > now) {
                // Not currently sending queries for this <G, port> or it's not
                // timed out yet.
                T_IG_PORT(IPMC_LIB_TRACE_GRP_TX, port_no, "%s: ASM is not timed out (%u; now = %u)", grp_itr->first, asm_port_state.query_timeout, now);
                continue;
            }

            if (asm_port_state.tx_cnt_left == 0) {
                T_EG_PORT(IPMC_LIB_TRACE_GRP_TX, port_no, "%s: ASM has tx_cnt_left == 0 despite it has a timeout (%u, now = %u)", grp_itr->first, asm_port_state.query_timeout, now);
                continue;
            }

            // RFC3376, section 6.6.3.1
            // RFC3810, section 7.6.3.1
            // If group/filter timer is larger than LMQT/LLQT, set "Suppress
            // Router-Side Processing" in the query message.
            suppress_router_side_processing = grp_state.ports[port_no].grp_timeout > lmqt;
            T_IG_PORT(IPMC_LIB_TRACE_GRP_TX, port_no, "%s: grp_timeout = %u, lmqt = %u => suppress_router_side_processing = %d", grp_itr->first, grp_state.ports[port_no].grp_timeout, lmqt, suppress_router_side_processing);

            // Update the current entry.
            if (--asm_port_state.tx_cnt_left == 0) {
                // No more retransmissions for this group on this port.
                asm_port_state.query_timeout = 0;
            } else {
                // Set timeout for the next time.
                // RFC3376, section 6.6.3.1 states that retransmissions must be
                // scheduled to be sent every [Last Member Query Interval] over
                // [Last Member Query Time]
                asm_port_state.query_timeout = now + vlan_state.internal_state.cur_lmqi / 10;
            }

            (void)IPMC_LIB_BASE_next_query_timeout_update(asm_state);

            // ipmc_lib_pdu_tx_group_specific_query() takes care of looking at
            // the group's compatibility in order to send a query that matches
            // it.
            T_IG(IPMC_LIB_TRACE_GRP_TX, "%s: Tx group-specific query (suppress router-side-processing = %d)", grp_itr->first, suppress_router_side_processing);
            tx_list_suppress.clear(); // Any of the two tx_lists will work, since we use it as empty.
            ipmc_lib_pdu_tx_group_specific_query(vlan_state, grp_itr, tx_list_suppress, port_no, suppress_router_side_processing);
        }
    }

    // Figure out whether we need to send group-and-source specific queries.
    if (src_map.next_query_timeout == 0 || src_map.next_query_timeout > now) {
        // Nothing else to do (yet).
        return;
    }

    next_query_timeout_update = false;

    // Retransmission of queries is per port, so loop through all ports marked
    // as active.
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!grp_state.active_ports[port_no]) {
            continue;
        }

        // Loop through all sources and add - to tx_map_suppress and
        // tx_map_dont_suppress - those that we should send a new query to.
        tx_list_suppress.clear();
        tx_list_dont_suppress.clear();

        for ((src_itr = src_map.begin()); src_itr != src_map.end(); ++src_itr) {
            ipmc_lib_src_state_t      &src_state      = src_itr->second;
            ipmc_lib_src_port_state_t &src_port_state = src_state.ports[port_no];

            if (!src_state.include_port_list[port_no]) {
                // This source is not in use on the port we are currently
                // looking at.
                T_IG_PORT(IPMC_LIB_TRACE_GRP_TX, port_no, "%s is not in the include list", src_itr->first);
                continue;
            }

            if (!src_port_state.query_timeout || src_port_state.query_timeout > now) {
                // Not currently sending queries for this <G, S, port> or it's
                // not timed out yet.
                T_IG_PORT(IPMC_LIB_TRACE_GRP_TX, port_no, "%s is not timed out (%u; now = %u)", src_itr->first, src_port_state.query_timeout, now);
                continue;
            }

            if (src_port_state.tx_cnt_left == 0) {
                T_EG_PORT(IPMC_LIB_TRACE_GRP_TX, port_no, "%s: %s has tx_cnt_left == 0 despite it has a timeout (%u; now = %u) and is in the source's include_port_list (%s)", grp_itr->first, src_itr->first, src_port_state.query_timeout, now, src_state.include_port_list);

                // Clear the timeout and go on.
                src_port_state.query_timeout = 0;
                continue;
            }

            // RFC3376, section 6.6.3.2
            // RFC3810, section 7.6.3.2
            // If source timer is greater than LMQT/LLQT, set "Suppress
            // Router-side Processing" in the query message.
            if (src_port_state.src_timeout > lmqt) {
                tx_list_suppress.set(src_itr->first);
            } else {
                tx_list_dont_suppress.set(src_itr->first);
            }

            // Update the current entry.
            if (--src_port_state.tx_cnt_left == 0) {
                // No more retransmissions for this source on this port.
                src_port_state.query_timeout = 0;
            } else {
                // Set timeout for the next time.
                // RFC3376, section 6.6.3.2 states that retransmissions must be
                // scheduled to be sent every [Last Member Query Interval] over
                // [Last Member Query Time]
                src_port_state.query_timeout = now + vlan_state.internal_state.cur_lmqi / 10;
            }

            if (IPMC_LIB_BASE_next_query_timeout_update(src_state)) {
                // Also update the src_map's next_query_timeout
                next_query_timeout_update = true;
            }
        }

        // ipmc_lib_pdu_tx_group_specific_query() takes care of looking at the
        // group's compatibility in order to send a query that matches it.

        // As the RFCs state: If either of the two calculated messages does
        // not contain any sources, then its transmission is suppressed.
        if (!tx_list_suppress.empty()) {
            T_IG(IPMC_LIB_TRACE_GRP_TX, "%s: Tx group-and-source-specific query (src = %s, suppress router-side processing = 1)", grp_itr->first, tx_list_suppress);
            ipmc_lib_pdu_tx_group_specific_query(vlan_state, grp_itr, tx_list_suppress, port_no, true /* Suppress Router-Side Processing */);
        }

        if (!tx_list_dont_suppress.empty()) {
            T_IG(IPMC_LIB_TRACE_GRP_TX, "%s: Tx group-and-source-specific query (src = %s, suppress router-side processing = 0)", grp_itr->first, tx_list_dont_suppress);
            ipmc_lib_pdu_tx_group_specific_query(vlan_state, grp_itr, tx_list_dont_suppress, port_no, false /* Don't Suppress Router-Side Processing */);
        }
    }

    if (next_query_timeout_update) {
        IPMC_LIB_BASE_next_query_timeout_update(src_map);
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_src_timer_tick()
/******************************************************************************/
static void IPMC_LIB_BASE_src_timer_tick(ipmc_lib_grp_itr_t &grp_itr, uint32_t now)
{
    ipmc_lib_src_itr_t   src_itr;
    ipmc_lib_grp_state_t &grp_state = grp_itr->second;
    ipmc_lib_src_map_t   &src_map   = grp_state.src_map;
    mesa_port_list_t     ports_still_in_use;
    mesa_port_no_t       port_no;
    bool                 next_src_timeout_update = false;

    if (src_map.next_src_timeout == 0 || src_map.next_src_timeout > now) {
        // No source addresses in this src_map have timed out yet.
        return;
    }

    T_IG(IPMC_LIB_TRACE_GRP_TICK, "Now = %u, src_map.next_src_timeout = %u", now, src_map.next_src_timeout);

    // At least one source has timed out.
    for (src_itr = src_map.begin(); src_itr != src_map.end(); ++src_itr) {
        ipmc_lib_src_state_t &src_state = src_itr->second;

        if (src_state.next_src_timeout == 0 || src_state.next_src_timeout > now) {
            // No ports in this source has any active source timers (all ports
            // in EXCLUDE mode) or the source timer hasn't timed out yet.
            continue;
        }

        T_IG(IPMC_LIB_TRACE_GRP_TICK, "Now = %u, src_state.next_src_timeout = %u", now, src_state.next_src_timeout);

        // At least one port has timed out. Loop through the include_port_list,
        // which contains either the "Requested List" (when filter mode is
        // EXCLUDE) or the "Include List" (when filter mode is INCLUDE).
        for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
            if (!src_state.include_port_list[port_no]) {
                // Not a port with a source timer > 0
                continue;
            }

            if (src_state.ports[port_no].src_timeout == 0) {
                T_EG_PORT(IPMC_LIB_TRACE_GRP_TICK, port_no, "%s: Source (%s) is in include_port_list[], but the source timer is 0", grp_itr->first, src_itr->first);
            }

            if (src_state.ports[port_no].src_timeout > now) {
                // The <G, S, port> source timer has not yet timed out.
                continue;
            }

            T_IG_PORT(IPMC_LIB_TRACE_GRP_TICK, port_no, "%s: Source (%s) in %s mode timed out", grp_itr->first, src_itr->first, ipmc_lib_util_filter_mode_to_str(grp_state.exclude_mode_ports[port_no] ? VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE : VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE));

            // The source timer has timed out.
            // RFC3376, section 6.3 and RFC3810 section 7.3 state what to do,
            // depending on the group's filter-mode for this port:
            //   Filter-mode == INCLUDE:
            //     Stop forwarding traffic from source and remove source record.
            //     If there are no more source records for the group, delete
            //     group record.
            //   Filter-mode == EXCLUDE:
            //     Stop forwarding, without removing the source record.

            // Reset src_timeout
            src_state.ports[port_no].src_timeout = 0;

            // Take the port out of forwarding.
            src_state.include_port_list[port_no] = false;

            if (!grp_state.exclude_mode_ports[port_no]) {
                // Filter-mode == INCLUDE
                // IPMC_LIB_BASE_mesa_update() (called by the caller of us)
                // will remove this source if include_port_list[] is now empty.

                // The caller of us will take care of removing the entire group
                // if the filter mode is INCLUDE and the source map is empty.
            } else {
                // Filter-mode == EXCLUDE.
                // Move the port from the "Requested List" to the "Exclude List"
                src_state.exclude_port_list[port_no] = true;
            }

            // Mark the source and the source map as changed.
            src_state.changed = true;
            src_map.changed   = true;

            if (IPMC_LIB_BASE_next_src_timeout_update(src_state)) {
                next_src_timeout_update = true;
            }
        }
    }

    if (src_map.changed) {
        // Changing an entry in the source map may also change the ASM entry.
        grp_state.asm_state.changed = true;
    }

    if (next_src_timeout_update) {
        // Also update the src_map's next_src_timeout.
        IPMC_LIB_BASE_next_src_timeout_update(src_map);
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_group_timer_tick()
/******************************************************************************/
static void IPMC_LIB_BASE_group_timer_tick(ipmc_lib_grp_itr_t &grp_itr, uint32_t now)
{
    ipmc_lib_src_itr_t   src_itr;
    ipmc_lib_grp_state_t &grp_state = grp_itr->second;
    ipmc_lib_src_map_t   &src_map   = grp_state.src_map;
    mesa_port_no_t       port_no;

    // The group timer is only used when the group is in EXCLUDE mode, and helps
    // moving the group's filter-mode back to INCLUDE mode.
    // RFC3376, section 6.2.2 says that when a group timer expires while the
    // group is in EXCLUDE mode, it means that there are no listeners on the
    // attached network (port) in EXCLUDE mode.
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!grp_state.active_ports[port_no]) {
            continue;
        }

        ipmc_lib_grp_port_state_t &grp_port_state = grp_state.ports[port_no];
        if (!grp_state.exclude_mode_ports[port_no]) {
            // The port is not in EXCLUDE more for this group. Go on.
            continue;
        }

        T_DG(IPMC_LIB_TRACE_GRP_TICK, "grp_timeout = %u, now = %u", grp_state.ports[port_no].grp_timeout, now);

        if (grp_port_state.grp_timeout > now) {
            // Group timer not timed out yet.
            continue;
        }

        T_IG(IPMC_LIB_TRACE_GRP_TICK, "Timeout (now = %u). grp_key = %s", now, grp_itr->first);

        // Group timer timed out.
        // RFC3376, section 6.2.2. and 6.5.
        // RFC3810, section 7.2.2 and 7.5.
        for (src_itr = src_map.begin(); src_itr != src_map.end(); ++src_itr) {
            ipmc_lib_src_state_t &src_state = src_itr->second;

            // Go through all sources that are currently excluded and mark them
            // for removal.
            if (src_state.exclude_port_list[port_no]) {
                // Remove <G, S, port>.
                src_state.exclude_port_list[port_no] = false;

                // Mark the source and the source map as changed.
                src_state.changed = true;
                src_map.changed   = true;
            }

            // Go through all sources in the "Requested List" and move them to
            // the "Included List".
            // The "Requested List" and the "Included List" are the same lists
            // in this implementation (include_port_list[]), so nothing to move.
        }

        // If the "Included List" is not empty, change filter mode to INCLUDE,
        // otherwise remove the source.
        // We change filter mode to INCLUDE here and let it be up to
        // IPMC_LIB_BASE_mesa_update() to remove empty source entries and
        // up to the caller of us to remove the group in its entirety.
        IPMC_LIB_BASE_filter_mode_set(grp_itr, port_no, VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE);
    }

    if (src_map.changed) {
        // Changing an entry in the source map may also change the ASM entry.
        grp_state.asm_state.changed = true;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_query_suppresion_tick()
/******************************************************************************/
static void IPMC_LIB_BASE_query_suppresion_tick(ipmc_lib_global_state_t &glb)
{
    if (glb.query_suppression_timeout == 0 || --glb.query_suppression_timeout == 0) {
        glb.query_flooding_cnt = 0;
        glb.query_suppression_timeout = IPMC_LIB_BASE_QUERY_SUPPRESSION_TIMEOUT;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_dynamic_router_port_tick()
/******************************************************************************/
static void IPMC_LIB_BASE_dynamic_router_port_tick(ipmc_lib_global_state_t &glb)
{
    mesa_port_no_t port_no;

    if (glb.dynamic_router_ports.is_empty()) {
        return;
    }

    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!glb.dynamic_router_ports[port_no]) {
            continue;
        }

        if (!glb.port_status[port_no].dynamic_router_timeout) {
            T_EG_PORT(IPMC_LIB_TRACE_GRP_TICK, port_no, "%s: Port marked as dynamic router port, but has no timeout", glb.key);
            // Make it time out below.
            glb.port_status[port_no].dynamic_router_timeout = 1;
        }

        if (--glb.port_status[port_no].dynamic_router_timeout) {
            // Not timed out yet.
            continue;
        }

        ipmc_lib_base_router_status_update(glb, port_no, true /* dynamic */, false /* remove */);
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_host_compat_update()
/******************************************************************************/
static void IPMC_LIB_BASE_host_compat_update(ipmc_lib_vlan_state_t &vlan_state)
{
    ipmc_lib_grp_map_t &grp_map = vlan_state.global->lists->grp_map;
    ipmc_lib_grp_itr_t grp_itr;

    if (vlan_state.conf.compatibility != VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO) {
        vlan_state.status.host_compat = vlan_state.conf.compatibility;
        return;
    }

    // Default to IGMPv3/MLDv2.
    vlan_state.status.host_compat = VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM;
    for (grp_itr = grp_map.begin(); grp_itr != grp_map.end(); ++grp_itr) {
        if (grp_itr->second.vlan_state != &vlan_state) {
            continue;
        }

        if (grp_itr->second.grp_compat < vlan_state.status.host_compat) {
            vlan_state.status.host_compat = grp_itr->second.grp_compat;
        }
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_grp_itr_erase()
/******************************************************************************/
static void IPMC_LIB_BASE_grp_itr_erase(ipmc_lib_grp_itr_t &grp_itr)
{
    ipmc_lib_vlan_state_t &vlan_state = *grp_itr->second.vlan_state;

    if (vlan_state.global->conf.leave_proxy_enable) {
        ipmc_lib_pdu_tx_leave(vlan_state, grp_itr->first.grp_addr);
    }

    vlan_state.global->lists->grp_map.erase(grp_itr);

    // This may have changed the VLAN's host compatibility
    IPMC_LIB_BASE_host_compat_update(vlan_state);
}

/******************************************************************************/
// IPMC_LIB_BASE_older_version_host_tick()
/******************************************************************************/
static void IPMC_LIB_BASE_older_version_host_tick(ipmc_lib_grp_itr_t &grp_itr, uint32_t now)
{
    ipmc_lib_vlan_state_t &vlan_state = *grp_itr->second.vlan_state;
    ipmc_lib_grp_state_t  &grp_state  = grp_itr->second;
    bool                  update_host_compat;

    // RFC3376, section 7.3.2,:
    // The older version host timers are only active when in AUTO compatibility
    // mode and an IGMPv1 or IGMPv2/MLDv1 Report PDU is seen on the interface
    // (VLAN).
    if (vlan_state.conf.compatibility != VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO) {
        return;
    }

    update_host_compat = false;
    if (grp_state.grp_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD) {
        // IGMPv1.
        if (grp_state.grp_older_version_host_present_timeout_old == 0) {
            T_EG(IPMC_LIB_TRACE_GRP_TICK, "%s: Group compatibility is IGMPv1, but timeout is 0 (grp_state = %s)", grp_itr->first, grp_itr->second);
        }

        if (grp_state.grp_older_version_host_present_timeout_old <= now) {
            // Transition to IGMPv2 if that timer is non-zero, otherwise
            // transition to IGMPv3.
            grp_state.grp_compat = grp_state.grp_older_version_host_present_timeout_gen ? VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN : VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM;
            grp_state.grp_older_version_host_present_timeout_old = 0;
            update_host_compat = true;
        }

        // Cannot use else if(), because it could be we should transition from
        // IGMPv1 through IGMPv2 to IGMPv3 (if
        // grp_older_version_host_present_timeout_gen also times out now).
        if (grp_state.grp_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN) {
            // IGMPv2/MLDv1
            if (grp_state.grp_older_version_host_present_timeout_gen == 0) {
                T_EG(IPMC_LIB_TRACE_GRP_TICK, "%s: Group compatibility is IGMPv2/MLDv1, but timer is 0 (grp = %s)", grp_itr->first, grp_itr->second);
            }

            if (grp_state.grp_older_version_host_present_timeout_gen <= now) {
                // Transition from IGMPv2/MLDv1 to IGMPv3/MLDv2
                grp_state.grp_compat = VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM;
                grp_state.grp_older_version_host_present_timeout_gen = 0;
                update_host_compat = true;
            }
        }
    }

    if (update_host_compat) {
        IPMC_LIB_BASE_host_compat_update(vlan_state);
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_older_version_querier_tick()
/******************************************************************************/
static void IPMC_LIB_BASE_older_version_querier_tick(ipmc_lib_vlan_state_t &vlan_state, uint32_t now)
{
    if (vlan_state.conf.compatibility != VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO) {
        vlan_state.status.older_version_querier_present_timeout_old = 0;
        vlan_state.status.older_version_querier_present_timeout_gen = 0;
        return;
    }

    if (vlan_state.status.querier_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD) {
        // IGMPv1.
        if (vlan_state.status.older_version_querier_present_timeout_old == 0) {
            T_EG(IPMC_LIB_TRACE_GRP_TICK, "%s: Querier compatibility is IGMPv1, but timeout is 0 (status = %s)", vlan_state.vlan_key, vlan_state.status);
        }

        if (vlan_state.status.older_version_querier_present_timeout_old <= now) {
            // Transition to IGMPv2 if that timer is non-zero, otherwise
            // transition to IGMPv3.
            vlan_state.status.querier_compat = vlan_state.status.older_version_querier_present_timeout_gen ? VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN : VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM;
            vlan_state.status.older_version_querier_present_timeout_old = 0;
        }

        // Cannot use else if(), because it could be we should transition from
        // IGMPv1 through IGMPv2 to IGMPv3 (if
        // older_version_querier_present_timeout_gen also times out now).
        if (vlan_state.status.querier_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN) {
            // IGMPv2/MLDv1
            if (vlan_state.status.older_version_querier_present_timeout_gen == 0) {
                T_EG(IPMC_LIB_TRACE_GRP_TICK, "%s: Querier compatibility is IGMPv2/MLDv1, but timer is 0 (status = %s)", vlan_state.vlan_key, vlan_state.status);
            }

            if (vlan_state.status.older_version_querier_present_timeout_gen <= now) {
                // Transition from IGMPv2/MLDv1 to IGMPv3/MLDv2
                vlan_state.status.querier_compat = VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM;
                vlan_state.status.older_version_querier_present_timeout_gen = 0;
            }
        }
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_proxy_tick()
// Takes care of sending proxied periodic queries and reports.
/******************************************************************************/
static void IPMC_LIB_BASE_proxy_tick(ipmc_lib_vlan_state_t &vlan_state)
{
    ipmc_lib_grp_itr_t grp_itr;
    ipmc_lib_grp_map_t &grp_map = vlan_state.global->lists->grp_map;

    if (!vlan_state.global->conf.proxy_enable) {
        vlan_state.internal_state.proxy_query_timeout  = 0;
        vlan_state.internal_state.proxy_report_timeout = 0;
        return;
    }

    // Time to transmit a query towards host ports?
    if (vlan_state.internal_state.proxy_query_timeout) {
        vlan_state.internal_state.proxy_query_timeout--;
    }

    if (vlan_state.internal_state.proxy_query_timeout == 0) {
        T_DG(IPMC_LIB_TRACE_GRP_TICK, "%s: Tx general query", vlan_state.vlan_key);
        // MESA_PORT_NO_NONE <=> Send to all ports in VLAN - except for router
        // ports.
        ipmc_lib_pdu_tx_general_query(vlan_state, MESA_PORT_NO_NONE);

        // Restart timer
        vlan_state.internal_state.proxy_query_timeout = vlan_state.conf.qi;
    }

    // Periodic reports to keep routers alive.
    if (vlan_state.internal_state.proxy_report_timeout && --vlan_state.internal_state.proxy_report_timeout == 0) {
        for (grp_itr = grp_map.begin(); grp_itr != grp_map.end(); ++grp_itr) {
            if (grp_itr->second.vlan_state == &vlan_state) {
                ipmc_lib_pdu_tx_report(vlan_state, grp_itr->first.grp_addr, vlan_state.status.querier_compat);
            }
        }
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_proxy_grp_map_tick()
// Takes care of sending proxied reports as a reply to a query.
// These reports must be delayed a random amount of time (within QRI).
/******************************************************************************/
static void IPMC_LIB_BASE_proxy_grp_map_tick(ipmc_lib_proxy_grp_map_t &proxy_grp_map)
{
    ipmc_lib_proxy_grp_map_itr_t proxy_map_itr, proxy_map_itr_next;
    ipmc_lib_grp_itr_t           grp_itr;

    // Time to transmit a report towards router ports?
    // These reports are replies to queries from routers delayed a random amount
    // of time (within QRI).
    // RBNTBD: Where is the timeout?
    proxy_map_itr = proxy_grp_map.begin();
    while (proxy_map_itr != proxy_grp_map.end()) {
        proxy_map_itr_next = proxy_map_itr;
        ++proxy_map_itr_next;

        ipmc_lib_vlan_state_t &vlan_state = *proxy_map_itr->second;

        ipmc_lib_pdu_tx_report(vlan_state, proxy_map_itr->first.grp_addr, vlan_state.status.querier_compat);
        proxy_grp_map.erase(proxy_map_itr);

        proxy_map_itr = proxy_map_itr_next;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_general_query_tx_tick()
/******************************************************************************/
static void IPMC_LIB_BASE_general_query_tx_tick(ipmc_lib_vlan_state_t &vlan_state)
{
    bool tx_general_query;

    // This function is called every 1 sec for each enabled VLAN interface.
    // The function handles Tx of querier messages and decrementing/resetting
    // timers
    T_NG(IPMC_LIB_TRACE_GRP_QUERIER, "%s: querier_enable = %d, querier_state = %s, query_interval_left = %d, startup_query_cnt_left = %d, other_querier_expiry_time = %d", vlan_state.vlan_key, vlan_state.conf.querier_enable, ipmc_lib_util_querier_state_to_str(vlan_state.status.querier_state), vlan_state.status.query_interval_left, vlan_state.internal_state.startup_query_cnt_left, vlan_state.status.other_querier_expiry_time);

    if (!vlan_state.conf.querier_enable) {
        vlan_state.status.querier_state = VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE;

        if (vlan_state.status.other_querier_expiry_time == 0) {
            vlan_state.status.other_querier_expiry_time = IPMC_LIB_BASE_other_querier_present_timeout_get(vlan_state);
        }

        return;
    }

    // We are enabled as a querier.
    tx_general_query = false;
    switch (vlan_state.status.querier_state) {
    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_INIT:
        if (vlan_state.status.query_interval_left == 0) {
            T_IG(IPMC_LIB_TRACE_GRP_QUERIER, "%s: Querier init", vlan_state.vlan_key);
            tx_general_query = !vlan_state.global->conf.proxy_enable;
            vlan_state.status.query_interval_left = vlan_state.conf.qi / 4;
            if (vlan_state.internal_state.startup_query_cnt_left) {
                vlan_state.internal_state.startup_query_cnt_left--;
            }
        } else {
            vlan_state.status.query_interval_left--;
        }

        if (vlan_state.internal_state.startup_query_cnt_left == 0) {
            T_IG(IPMC_LIB_TRACE_GRP_QUERIER, "%s: Changing querier state from Init to Active", vlan_state.vlan_key);
            vlan_state.internal_state.cur_rv   = vlan_state.conf.rv;
            vlan_state.internal_state.cur_qi   = vlan_state.conf.qi;
            vlan_state.internal_state.cur_qri  = vlan_state.conf.qri;
            vlan_state.internal_state.cur_lmqi = vlan_state.conf.lmqi;
            vlan_state.status.querier_uptime  = 0;
            vlan_state.status.querier_state    = VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE;
            ipmc_lib_pdu_our_ip_get(vlan_state, vlan_state.status.active_querier_address);
        }

        break;

    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE:
        vlan_state.status.querier_uptime++;
        ipmc_lib_pdu_our_ip_get(vlan_state, vlan_state.status.active_querier_address);

        if (vlan_state.status.query_interval_left) {
            vlan_state.status.query_interval_left--;
        }

        if (vlan_state.status.query_interval_left == 0) {
            T_IG(IPMC_LIB_TRACE_GRP_QUERIER, "%s: Querier timeout (proxy_enable = %d)", vlan_state.vlan_key, vlan_state.global->conf.proxy_enable);
            tx_general_query = !vlan_state.global->conf.proxy_enable;
            vlan_state.status.query_interval_left = vlan_state.conf.qi;
        }

        break;

    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE:
    default:
        vlan_state.status.querier_uptime = 0;
        if (vlan_state.status.other_querier_expiry_time) {
            vlan_state.status.other_querier_expiry_time--;
        } else {
            T_IG(IPMC_LIB_TRACE_GRP_QUERIER, "%s: Changing querier state from Idle to Active", vlan_state.vlan_key);

            vlan_state.internal_state.cur_rv      = vlan_state.conf.rv;
            vlan_state.internal_state.cur_qi      = vlan_state.conf.qi;
            vlan_state.internal_state.cur_qri     = vlan_state.conf.qri;
            vlan_state.internal_state.cur_lmqi    = vlan_state.conf.lmqi;
            vlan_state.status.query_interval_left = vlan_state.conf.qi;
            vlan_state.status.querier_state       = VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE;
            ipmc_lib_pdu_our_ip_get(vlan_state, vlan_state.status.active_querier_address);
            tx_general_query = !vlan_state.global->conf.proxy_enable;
        }

        break;
    }

    if (tx_general_query) {
        ipmc_lib_pdu_tx_general_query(vlan_state, MESA_PORT_NO_NONE);
    }
}

/******************************************************************************/
// ipmc_lib_base_grp_map_tick()
// Checks:
//  - Group Timers
//  - Source Timers
//  - Older Version Host Present timers.
//  - Query retransmission timers.
//  - Per-group Query Tx timers
/******************************************************************************/
void ipmc_lib_base_grp_map_tick(ipmc_lib_grp_map_t &grp_map, uint32_t now)
{
    ipmc_lib_grp_itr_t grp_itr, grp_itr_next;
    ipmc_lib_src_itr_t src_itr;

    grp_itr = grp_map.begin();
    while (grp_itr != grp_map.end()) {
        // We might delete this group as part of the following, so keep a ptr
        // to the next group
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        // We check the source timers before the group timer, because the group
        // timer needs to know whether all source timers have expired.
        IPMC_LIB_BASE_src_timer_tick(grp_itr, now);

        // Check the group timer.
        IPMC_LIB_BASE_group_timer_tick(grp_itr, now);

        // If src_map.changed is true, so is asm_state.changed, so just check
        // for that one.
        if (grp_itr->second.asm_state.changed) {
            // Time to update MESA.
            IPMC_LIB_BASE_mesa_update(*grp_itr->second.vlan_state->global, grp_itr);

            // If there are no more active ports for this group, we delete the
            // entire group.
            if (grp_itr->second.active_ports.is_empty()) {
                IPMC_LIB_BASE_grp_itr_erase(grp_itr);
                goto next;
            }
        }

        // Check if we need to retransmit a query (as a result of a host leaving
        // a group or <G, S>).
        IPMC_LIB_BASE_query_retransmit_tick(grp_itr, now);

        // Check if we need to change the group compatibility because an
        // older version host timer has timed out.
        IPMC_LIB_BASE_older_version_host_tick(grp_itr, now);

next:
        grp_itr = grp_itr_next;
    }
}

/******************************************************************************/
// ipmc_lib_base_proxy_report_tick()
// Checks:
//  - Proxied reports requested by a querier's query.
/******************************************************************************/
void ipmc_lib_base_proxy_report_tick(ipmc_lib_proxy_grp_map_t &proxy_grp_map)
{
    IPMC_LIB_BASE_proxy_grp_map_tick(proxy_grp_map);
}

/******************************************************************************/
// ipmc_lib_base_vlan_tick()
// Checks:
//  - Proxied periodic queries and periodic reports
//  - General query Tx timers
//  - Oler Version Querier Present timers.
/******************************************************************************/
void ipmc_lib_base_vlan_tick(ipmc_lib_vlan_state_t &vlan_state, uint32_t now)
{
    IPMC_LIB_BASE_proxy_tick(vlan_state);

    // Check if we need to transmit a general query.
    IPMC_LIB_BASE_general_query_tx_tick(vlan_state);

    IPMC_LIB_BASE_older_version_querier_tick(vlan_state, now);
}

/******************************************************************************/
// ipmc_lib_base_global_tick()
// Checks:
//  - Query suppression
//  - Dynamic router ports return to non-dynamic router ports.
/******************************************************************************/
void ipmc_lib_base_global_tick(ipmc_lib_global_state_t &glb)
{
    // Query suppression timeout.
    IPMC_LIB_BASE_query_suppresion_tick(glb);

    // Return of dynamic router ports to non-dynamic router ports.
    IPMC_LIB_BASE_dynamic_router_port_tick(glb);
}

/******************************************************************************/
// IPMC_LIB_BASE_tx_query_cnt_and_timeout_set()
/******************************************************************************/
static void IPMC_LIB_BASE_tx_query_cnt_and_timeout_set(ipmc_lib_vlan_state_t &vlan_state, ipmc_lib_src_port_state_t &src_port_state)
{
    // Add source to retransmission list
    // We do that by setting a query_timeout.
    // Here we set the timeout to 1, because this means that it will time
    // out one second after boot, so that we shortly Tx a query immediately.
    src_port_state.query_timeout = 1;

    // Set the Source Retransmission Counter for each source to LMQC/LLQC
    // (Last Member/Listener Query Count).
    // RBNTBD: Is it the current RV we need here?
    src_port_state.tx_cnt_left = vlan_state.internal_state.cur_rv;
}

/******************************************************************************/
// IPMC_LIB_BASE_lower_group_timer()
/******************************************************************************/
static void IPMC_LIB_BASE_lower_group_timer(ipmc_lib_vlan_state_t &vlan_state, ipmc_lib_grp_itr_t &grp_itr, mesa_port_no_t port_no, uint32_t lmqt, uint32_t now, bool based_on_query)
{
    // RFC3376 section 6.6.3.1 and 6.6.1
    // RFC3810 section 7.5.3.1 and 7.6.1
    ipmc_lib_src_port_state_t &asm_port_state = grp_itr->second.asm_state.ports[port_no];

    // Lower group/filter timer to LMQT.
    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Lowering group/filter timer from %u to %u (now = %u) based on reception of a %s", grp_itr->first, grp_itr->second.ports[port_no].grp_timeout, lmqt, now, based_on_query ? "query" : "report");
    grp_itr->second.ports[port_no].grp_timeout = lmqt;

    if (!based_on_query && vlan_state.status.querier_state != VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE) {
        // This is based on a received report and we are querier.
        // Set query retransmission count and timeout
        IPMC_LIB_BASE_tx_query_cnt_and_timeout_set(vlan_state, asm_port_state);

        // Compute new next query timeout.
        IPMC_LIB_BASE_next_query_timeout_update(grp_itr->second.asm_state);
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_lower_source_timers()
/******************************************************************************/
static void IPMC_LIB_BASE_lower_source_timers(ipmc_lib_vlan_state_t &vlan_state, ipmc_lib_grp_itr_t &grp_itr, mesa_port_no_t port_no, const ipmc_lib_src_list_t &Q, uint32_t lmqt, uint32_t now, bool based_on_query)
{
    ipmc_lib_src_itr_t            src_itr;
    ipmc_lib_src_list_const_itr_t src_list_itr;
    ipmc_lib_src_map_t            &src_map = grp_itr->second.src_map;
    bool                          next_src_timeout_update, next_query_timeout_update;

    next_src_timeout_update   = false;
    next_query_timeout_update = false;

    // If not based_on_query: For each source in the "X" in Send Q(G, X)
    for (src_list_itr = Q.begin(); src_list_itr != Q.end(); ++src_list_itr) {
        // Look up the corresponding source in src_map.
        if ((src_itr = src_map.find(*src_list_itr)) == src_map.end()) {
            // The source wasn't found.
            if (based_on_query) {
                // When it's based on a received query, the source need not be
                // in src_map.
                T_IG(IPMC_LIB_TRACE_GRP_RX, "%s: Unable to find %s in %s", grp_itr->first, *src_list_itr, src_map);
            } else {
                // When it's based on a received report, the source must be
                // findable in src_map.
                T_EG(IPMC_LIB_TRACE_GRP_RX, "%s: Unable to find %s in %s", grp_itr->first, *src_list_itr, src_map);
            }

            continue;
        }

        ipmc_lib_src_state_t &src_state = src_itr->second;

        if (!src_state.include_port_list[port_no]) {
            if (based_on_query) {
                // When it's based on a received query, the source need not be
                // active on the port number.
                T_IG(IPMC_LIB_TRACE_GRP_RX, "%s: %s is not set in include_port_list[%u] of src_map (%s)", grp_itr->first, *src_list_itr, port_no, src_map);
            } else {
                // When it's based on a received report, the source must be
                // active on the port number.
                T_EG(IPMC_LIB_TRACE_GRP_RX, "%s: %s is not set in include_port_list[%u] of src_map (%s)", grp_itr->first, *src_list_itr, port_no, src_map);
            }

            continue;
        }

        ipmc_lib_src_port_state_t &src_port_state = src_state.ports[port_no];

        if (based_on_query) {
            // RFC3376 section 6.6.1 and RFC3810 section 7.6.1 don't say
            // anything about the current value of the source timer, so just go
            // ahead and update it.
        } else {
            // RFC3376, section 6.6.3.2
            // RFC3810, section 7.6.3.2
            // Only add sources to the retransmission list if source timer is
            // larger than LMQT/LLQT.
            if (src_port_state.src_timeout <= lmqt) {
                continue;
            }
        }

        // Lower source timer to LMQT/LLQT
        T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Lowering source timer of %s from %u to %u (now = %u) based on reception of a ", grp_itr->first, *src_list_itr, src_port_state.src_timeout, lmqt, now, based_on_query ? "query" : "report");
        src_port_state.src_timeout = lmqt;

        // Compute a new next_src_timeout for this source entry
        if (IPMC_LIB_BASE_next_src_timeout_update(src_state)) {
            // Compute a new next_src_timeout for the map outside of this loop.
            next_src_timeout_update = true;
        }

        if (!based_on_query && vlan_state.status.querier_state != VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE) {
            // This is based on a received report and we are querier.
            // Set query retransmission count and timeout.
            IPMC_LIB_BASE_tx_query_cnt_and_timeout_set(vlan_state, src_port_state);

            // Compute a new next_query_timeout for this source entry
            if (IPMC_LIB_BASE_next_query_timeout_update(src_state)) {
                next_query_timeout_update = true;
            }
        }
    }

    if (next_src_timeout_update) {
        IPMC_LIB_BASE_next_src_timeout_update(src_map);
    }

    if (next_query_timeout_update) {
        IPMC_LIB_BASE_next_query_timeout_update(src_map);
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_tx_query_start()
/******************************************************************************/
static void IPMC_LIB_BASE_tx_query_start(ipmc_lib_vlan_state_t &vlan_state, ipmc_lib_grp_itr_t &grp_itr, mesa_port_no_t port_no, ipmc_lib_src_list_t &Q, bool tx_group_specific_query, bool is_leave, uint32_t now)
{
    uint32_t lmqt;
    bool     proxy;

    if (Q.empty() && !tx_group_specific_query) {
        // Don't send any queries.
        return;
    }

    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Send Q(G, %s), tx_group_specific_query = %d, is_leave = %d", grp_itr->first, Q, tx_group_specific_query, is_leave);

    proxy = vlan_state.global->conf.proxy_enable || (is_leave && vlan_state.global->conf.leave_proxy_enable);

    if (proxy) {
        if (vlan_state.global->port_status[port_no].router_status != VTSS_APPL_IPMC_LIB_ROUTER_STATUS_NONE) {
            // The port on which we received the report is (also) a router port,
            // so don't send queries back to it.
            T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "Is a router port, so don't send queries");
            return;
        }
    }

    lmqt = IPMC_LIB_BASE_last_member_query_time_get(vlan_state) + now;

    if (tx_group_specific_query) {
        IPMC_LIB_BASE_lower_group_timer(vlan_state, grp_itr, port_no, lmqt, now, false /* based on Rx or a report */);
    }

    IPMC_LIB_BASE_lower_source_timers(vlan_state, grp_itr, port_no, Q, lmqt, now, false /* based on Rx of a report */);

    if (vlan_state.status.querier_state == VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE) {
        // We are not the current querier. Don't send queries.
        T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "We are not the current querier, so don't send queries");
    } else {
        // Send a Tx query right away.
        IPMC_LIB_BASE_query_retransmit_tick(grp_itr, now);
    }
}

/******************************************************************************/
// ipmc_lib_src_list_t::operator+()
// A = B + C, where B is *this and C is rhs.
/******************************************************************************/
ipmc_lib_src_list_t ipmc_lib_src_list_t::operator+(const ipmc_lib_src_list_t &rhs) const
{
    ipmc_lib_src_list_const_itr_t itr;
    ipmc_lib_src_list_t           res(*this);

    for (itr = rhs.cbegin(); itr != rhs.cend(); ++itr) {
        res.set(*itr);
    }

    return res;
}

/******************************************************************************/
// ipmc_lib_src_list_t::operator-()
// A = B - C, where B is *this and C is rhs.
/******************************************************************************/
ipmc_lib_src_list_t ipmc_lib_src_list_t::operator-(const ipmc_lib_src_list_t &rhs) const
{
    ipmc_lib_src_list_const_itr_t itr;
    ipmc_lib_src_list_t           res(*this);

    for (itr = rhs.cbegin(); itr != rhs.cend(); ++itr) {
        // OK to call erase() even if it doesn't exist.
        res.erase(*itr);
    }

    return res;
}

/******************************************************************************/
// ipmc_lib_src_list_t::operator*()
// A = B * C (intersection), where B is *this and C is rhs.
/******************************************************************************/
ipmc_lib_src_list_t ipmc_lib_src_list_t::operator*(const ipmc_lib_src_list_t &rhs) const
{
    ipmc_lib_src_list_const_itr_t itr;
    ipmc_lib_src_list_t           res;

    res.clear();

    for (itr = cbegin(); itr != cend(); ++itr) {
        if (rhs.find(*itr) != rhs.cend()) {
            res.set(*itr);
        }
    }

    return res;
}

/******************************************************************************/
// IPMC_LIB_BASE_src_map_update()
/******************************************************************************/
static void IPMC_LIB_BASE_src_map_update(ipmc_lib_vlan_state_t &vlan_state, ipmc_lib_grp_itr_t &grp_itr, mesa_port_no_t port_no, ipmc_lib_src_list_t &X, ipmc_lib_src_list_t &Y)
{
    ipmc_lib_src_itr_t      src_itr;
    ipmc_lib_src_map_t      &src_map = grp_itr->second.src_map;
    ipmc_lib_src_list_itr_t src_list_itr;
    bool                    found_in_X;
    bool                    found_in_Y;

    // X contains the new sources to forward.
    // Y contains the new sources to block.
    // Go through the existing src_map and add or remove those represented or
    // no longer represented in X and Y.
    for (src_itr = src_map.begin(); src_itr != src_map.end(); ++src_itr) {
        ipmc_lib_src_state_t &src_state = src_itr->second;

        found_in_X = X.find(src_itr->first) != X.end();
        found_in_Y = Y.find(src_itr->first) != Y.end();

        if (found_in_X && found_in_Y) {
            T_EG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Source %s is found in both new X and new Y", grp_itr->first, src_itr->first);
        }

        if (found_in_X) {
            // This source is found in X...
            if (!src_state.include_port_list[port_no]) {
                // ...but it was not forwarding previously.
                src_state.include_port_list[port_no] = true;
                src_state.changed                    = true;
                src_map.changed                      = true;
            }
        } else {
            // This source is no longer or not found in X...
            if (src_state.include_port_list[port_no]) {
                // ...but it was forwarding previously.
                src_state.include_port_list[port_no] = false;
                src_state.changed                    = true;
                src_map.changed                      = true;
            }
        }

        if (found_in_Y) {
            // This source is found in Y...
            if (!src_state.exclude_port_list[port_no]) {
                // ...but it was not blocked previously.
                src_state.exclude_port_list[port_no] = true;
                src_state.changed                    = true;
                src_map.changed                      = true;
            }
        } else {
            // This source is no longer or not found in Y...
            if (src_state.exclude_port_list[port_no]) {
                // ...but it was blocking previously.
                src_state.exclude_port_list[port_no] = false;
                src_state.changed                    = true;
                src_map.changed                      = true;
            }
        }
    }

    // Go through X and add those not already represented in src_map.
    for (src_list_itr = X.begin(); src_list_itr != X.end(); ++src_list_itr) {
        if (src_map.find(*src_list_itr) == src_map.end()) {
            // There is a limit on the number of entries, we can add to the
            // src_map - whether it is forwarding or discarding. If this limit
            // is reached, don't add it.
            if (src_map.size() >= vlan_state.global->lib_cap.src_per_grp_cnt_max) {
                // We have exceeded the maximum number of supported sources per
                // group.
                // RBNTBD: Update operational state?
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Maximum number of supported sources per group (%u) is exceeded. Ignoring %s", grp_itr->first, vlan_state.global->lib_cap.src_per_grp_cnt_max, *src_list_itr);
                continue;
            }

            // The entry is not found. Add it.
            if ((src_itr = src_map.get(*src_list_itr)) == src_map.end()) {
                // Out of memory.
                // RBNTBD: Update operational state?
                continue;
            }

            ipmc_lib_src_state_t &src_state = src_itr->second;
            vtss_clear(src_state);
            src_state.include_port_list[port_no] = true;
            src_state.changed                    = true;
            src_map.changed                      = true;
        }
    }

    // Go through Y and add those not already represented in src_map.
    for (src_list_itr = Y.begin(); src_list_itr != Y.end(); ++src_list_itr) {
        if (src_map.find(*src_list_itr) == src_map.end()) {
            // There is a limit on the number of entries, we can add to the
            // src_map - whether it is forwarding or discarding. If this limit
            // is reached, don't add it.
            if (src_map.size() >= vlan_state.global->lib_cap.src_per_grp_cnt_max) {
                // We have exceeded the maximum number of supported sources per
                // group.
                // RBNTBD: Update operational state?
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Maximum number of supported sources per group (%u) is exceeded. Ignoring %s", grp_itr->first, vlan_state.global->lib_cap.src_per_grp_cnt_max, *src_list_itr);
                continue;
            }

            // The entry is not found. Add it.
            if ((src_itr = src_map.get(*src_list_itr)) == src_map.end()) {
                // Out of memory.
                // RBNTBD: Update operational state?
                continue;
            }

            ipmc_lib_src_state_t &src_state = src_itr->second;
            vtss_clear(src_state);
            src_state.exclude_port_list[port_no] = true;
            src_state.changed                    = true;
            src_map.changed                      = true;
        }
    }

    if (src_map.changed) {
        // A change to the source map also causes a change to the ASM entry.
        grp_itr->second.asm_state.changed = true;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_src_map_timeout_update()
/******************************************************************************/
static void IPMC_LIB_BASE_src_map_timeout_update(ipmc_lib_vlan_state_t &vlan_state, ipmc_lib_grp_itr_t &grp_itr, mesa_port_no_t port_no, ipmc_lib_src_list_t &TO, uint32_t src_timeout, uint32_t now)
{
    ipmc_lib_src_itr_t      src_itr;
    ipmc_lib_src_map_t      &src_map = grp_itr->second.src_map;
    ipmc_lib_src_list_itr_t src_list_itr;
    bool                    next_src_timeout_update;

    T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: (%s) = %u (now = %u)", grp_itr->first, TO, src_timeout, now);

    // Go through TO and update the timeouts in src_map.
    next_src_timeout_update = false;
    for (src_list_itr = TO.begin(); src_list_itr != TO.end(); ++src_list_itr) {
        if ((src_itr = src_map.find(*src_list_itr)) == src_map.end()) {
            // It might happen that the source in TO is not found in src_map,
            // because the limit is reached, so we can't throw a trace error
            // here.
            continue;
        }

        ipmc_lib_src_state_t &src_state = src_itr->second;

        T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Setting %s src_timeout = %u (now = %u)", grp_itr->first, *src_list_itr, src_timeout, now);

        if (!src_state.include_port_list[port_no] && src_timeout) {
            T_EG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Setting %s src_timeout to a non-zero value (%u) while the <G, S, port> is not forwarding", grp_itr->first, *src_list_itr, src_timeout);
        }

        src_state.ports[port_no].src_timeout = src_timeout;

        if (IPMC_LIB_BASE_next_src_timeout_update(src_state)) {
            // A change has occurred to this source state's next timeout. Also
            // update the whole src_map's.
            next_src_timeout_update = true;
        }
    }

    if (next_src_timeout_update) {
        IPMC_LIB_BASE_next_src_timeout_update(src_map);
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_debug_grp()
/******************************************************************************/
static void IPMC_LIB_BASE_debug_grp(mesa_port_no_t port_no, const ipmc_lib_grp_key_t &grp_key,
                                    vtss_appl_ipmc_lib_filter_mode_t old_filter_mode, const ipmc_lib_src_list_t &old_X, const ipmc_lib_src_list_t &old_Y,
                                    ipmc_lib_pdu_record_type_t record_type, const ipmc_lib_src_list_t &A,
                                    vtss_appl_ipmc_lib_filter_mode_t new_filter_mode, const ipmc_lib_src_list_t &new_X, const ipmc_lib_src_list_t &new_Y)
{
    if (old_filter_mode == VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE && !old_Y.empty()) {
        T_EG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: old filter mode is INCLUDE, but old Y is not empty (%s)", grp_key, old_Y);
    }

    if (new_filter_mode == VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE && !new_Y.empty()) {
        T_EG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: old filter mode is INCLUDE, but old Y is not empty (%s)", grp_key, new_Y);
    }

    if (old_filter_mode == VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE) {
        if (new_filter_mode == VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE) {
            T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: INCLUDE(%s) && %s(%s) => INCLUDE(%s)",     grp_key, old_X, ipmc_lib_pdu_record_type_to_str(record_type), A, new_X);
        } else {
            T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: INCLUDE(%s) && %s(%s) => EXCLUDE(%s, %s)", grp_key, old_X, ipmc_lib_pdu_record_type_to_str(record_type), A, new_X, new_Y);
        }
    } else {
        if (new_filter_mode == VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE) {
            T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: EXCLUDE(%s, %s) && %s(%s) => INCLUDE(%s)",     grp_key, old_X, old_Y, ipmc_lib_pdu_record_type_to_str(record_type), A, new_X);
        } else {
            T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: EXCLUDE(%s, %s) && %s(%s) => EXCLUDE(%s, %s)", grp_key, old_X, old_Y, ipmc_lib_pdu_record_type_to_str(record_type), A, new_X, new_Y);
        }
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_process_src()
/******************************************************************************/
static void IPMC_LIB_BASE_process_src(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu, ipmc_lib_grp_itr_t &grp_itr, const ipmc_lib_pdu_group_record_t &grp_rec, uint16_t src_cnt, uint32_t now)
{
    mesa_port_no_t                   port_no          = pdu.rx_info.port_no;
    ipmc_lib_grp_state_t             &grp_state       = grp_itr->second;
    ipmc_lib_src_map_t               &src_map         = grp_state.src_map;
    ipmc_lib_grp_port_state_t        &grp_port_state  = grp_state.ports[port_no];
    vtss_appl_ipmc_lib_filter_mode_t old_filter_mode, new_filter_mode;
    ipmc_lib_src_list_t              old_X, old_Y, A, new_X, new_Y, Q, TO;
    ipmc_lib_src_state_t             src_state;
    ipmc_lib_src_itr_t               src_itr;
    uint32_t                         gmi, src_timeout;
    bool                             tx_group_specific_query, fast_leave;

    T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "Now = %u\ngrp_rec = %s\ngrp_itr = %s", now, grp_rec, grp_itr);

    // We need GMI/MALI in most of the following cases.
    gmi = IPMC_LIB_BASE_group_membership_interval_get(vlan_state);

    T_DG(IPMC_LIB_TRACE_GRP_RX, "gmi (%u) + now (%u) = %u", gmi, now, gmi + now);
    gmi += now;

    if (src_cnt) {
        A = grp_rec.src_list;
    } else {
        // Wither grp_rec.src_list is already empty or we're asked not to use
        // any sources from it.
        A.clear();
    }

    // old_X holds the old forwarding sources.
    // old_Y holds the old blocked sources.
    // A     holds the received sources.
    // new_X holds the new forwarding sources.
    // new_Y holds the new blocked sources.
    // Q     holds the sources for which to send a query
    // TO    holds the sources to change source timeout for (held in src_timeout)
    old_X.clear();
    old_Y.clear();
    for (src_itr = src_map.begin(); src_itr != src_map.end(); ++src_itr) {
        if (src_itr->second.include_port_list[port_no]) {
            old_X.set(src_itr->first);
        } else if (src_itr->second.exclude_port_list[port_no]) {
            old_Y.set(src_itr->first);
        }
    }

    // It's a bit annoying that the RFC doesn't always use X for the current
    // "Include List"/"Requested List", Y for the current "Excluded List", and A
    // for the received source list, so I have changed the RFC's rows to use
    // these three letters only in the following.
    old_filter_mode = grp_state.exclude_mode_ports[port_no] ? VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE : VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE;
    new_filter_mode = old_filter_mode;

    // We only support fast_leave when receiving IGMPv2/MLDv2 Leave/Done PDUs,
    // which are recognized by pdu.report.is_leave == true (and record_type ==
    // IPMC_LIB_PDU_RECORD_TYPE_TO_IN and empty source list).
    fast_leave = pdu.report.is_leave && vlan_state.global->port_conf[port_no].fast_leave;

    tx_group_specific_query = false;
    src_timeout             = 0;

    // Use the same order as in RFC3376 and RFC3810, so start with current
    // filter mode == INCLUDE and go through the record types in the old order
    // done in those RFCs.
    if (old_filter_mode == VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE) {
        // Current filter mode is INCLUDE.
        switch (grp_rec.record_type) {
        case IPMC_LIB_PDU_RECORD_TYPE_IS_IN:
            // RFC3376, section 6.4.1, row 1.
            // RFC3810, section 7.4.1, row 1.

            // INCLUDE(X) && IS_IN(A) => INCLUDE(X + A)
            new_X = old_X + A;
            new_Y.clear();

            // (A) = GMI/MALI
            TO = A;
            src_timeout = gmi;

            // No queries
            Q.clear();

            // Filter mode remains at INCLUDE.
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_IS_EX:
            // RFC3376, section 6.4.1, row 2.
            // RFC3810, section 7.4.1, row 2.

            // INCLUDE(X) && IS_EX(A) => EXCLUDE(X * A, A - X)
            new_X       = old_X * A;
            new_Y       = A - old_X;

            // (A - X) = 0
            TO = A - old_X;
            src_timeout = 0;

            // No queries
            Q.clear();

            // Group/Filter Timer = GMI/MALI
            grp_port_state.grp_timeout = gmi;

            // Change filter mode
            new_filter_mode = VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE;
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_ALLOW:
            // RFC3376, section 6.4.2, row 1.
            // RFC3810, section 7.4.2, row 1.

            // INCLUDE(X) && ALLOW(A) => INCLUDE(X + A)
            new_X = old_X + A;
            new_Y.clear();

            // (A) = GMI/MALI
            TO = A;
            src_timeout = gmi;

            // No queries
            Q.clear();

            // Filter mode remains at INCLUDE
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_BLOCK:
            // RFC3376, section 6.4.2, row 2.
            // RFC3810, section 7.4.2, row 2.

            // INCLUDE(X) && BLOCK(A) => INCLUDE(X) && Send Q(G, X * A)
            new_X = old_X;
            new_Y.clear();

            // No timeout changes.
            TO.clear();
            src_timeout = 0;

            // Send Q(G, X * A)
            Q = old_X * A;

            // Filter mode remains at INCLUDE.
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_TO_EX:
            // RFC3376, section 6.4.2, row 3.
            // RFC3810, section 7.4.2, row 3.

            // INCLUDE(X) && TO_EX(A) => EXCLUDE(X * A, A - X)
            new_X = old_X * A;
            new_Y = A - old_X;

            // (A - X) = 0
            TO = A - old_X;
            src_timeout = 0;

            // Send Q(G, X * A)
            Q = old_X * A;

            // Group/Filter Timer = GMI/MALI
            grp_port_state.grp_timeout = gmi;

            // Change filter mode
            new_filter_mode = VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE;
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_TO_IN:
            // RFC3376, section 6.4.2, row 4.
            // RFC3810, section 7.4.2, row 4.

            if (fast_leave) {
                new_X.clear();
                new_Y.clear();
                Q.clear();
            } else {
                // INCLUDE(X) && TO_IN(A) => INCLUDE (X + A)
                new_X = old_X + A;
                new_Y = old_Y - A;

                // (A) = GMI/MALI
                TO = A;
                src_timeout = gmi;

                // Send Q(G, X - A)
                Q = old_X - A;
            }

            // Filter mode remains at INCLUDE
            break;

        default:
            // This record would have been marked as invalid by ipmc_lib_packet.cxx
            // if the record_type was invalid, and it will therefore not be
            // processed (checked by the caller of this function).
            T_EG(IPMC_LIB_TRACE_GRP_RX, "Unreachable (%d)", grp_rec.record_type);
            return;
        }
    } else {
        // Current filter mode is EXCLUDE.
        switch (grp_rec.record_type) {
        case IPMC_LIB_PDU_RECORD_TYPE_IS_IN:
            // RFC3376, section 6.4.1, row 3.
            // RFC3810, section 7.4.1, row 3.

            // EXCLUDE(X, Y) && IS_IN(A) => EXCLUDE(X + A, Y - A)
            new_X = old_X + A;
            new_Y = old_Y - A;

            // (A) = GMI/MALI
            TO          = A;
            src_timeout = gmi;

            // No queries.
            Q.clear();

            // Filter mode remains at EXCLUDE.
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_IS_EX:
            // RFC3376, section 6.4.1, row 4.
            // RFC3810, section 7.4.1, row 4.

            // EXCLUDE(X, Y) && IS_EX(A) => EXCLUDE(A - Y, Y * A)
            new_X = A - old_Y;
            new_Y = old_Y * A;

            // (A - X - Y) = GMI/MALI
            TO = A - old_X - old_Y;
            src_timeout = gmi;

            // No queries
            Q.clear();

            // Group/Filter Timer = GMI/MALI
            grp_port_state.grp_timeout = gmi;

            // Filter mode remains at EXCLUDE.
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_ALLOW:
            // RFC3376, section 6.4.2, row 5.
            // RFC3810, section 7.4.2, row 5.

            // EXCLUDE(X, Y) && ALLOW(A) => EXCLUDE(X + A, Y - A)
            new_X = old_X + A;
            new_Y = old_Y - A;

            // (A) = GMI/MALI
            TO = A;
            src_timeout = gmi;

            // No queries
            Q.clear();

            // Filter mode remains at EXCLUDE
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_BLOCK:
            // RFC3376, section 6.4.2, row 6.
            // RFC3810, section 7.4.2, row 6.

            // EXCLUDE(X, Y) && BLOCK(A) => EXCLUDE(X + (A - Y), Y)
            new_X = old_X + A - old_Y;
            new_Y = old_Y;

            // (A - X - Y) = Group/Filter Timer
            TO = A - old_X - old_Y;
            src_timeout = grp_port_state.grp_timeout;

            // Send Q(G, A - Y)
            Q = A - old_Y;

            // Filter mode remains at EXCLUDE
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_TO_EX:
            // RFC3376, section 6.4.2, row 7.
            // RFC3810, section 7.4.2, row 7.

            // EXCLUDE(X, Y) && TO_EX(A) => EXCLUDE(A - Y, Y * A)
            new_X = A - old_Y;
            new_Y = old_Y * A;

            // (A - X - Y) = Group/Filter Timer
            TO          = A - old_X - old_Y;
            src_timeout = grp_state.ports[port_no].grp_timeout;

            // Send Q(G, A - Y)
            Q = A - old_Y;

            // Group/Filter Timer = GMI/MALI
            grp_port_state.grp_timeout = gmi;

            // Filter mode remains at EXCLUDE
            break;

        case IPMC_LIB_PDU_RECORD_TYPE_TO_IN:
            // RFC3376, section 6.4.2, row 8.
            // RFC3810, section 7.4.2, row 8.

            if (fast_leave) {
                // On fast leave ports, we just leave immediately without
                // sending any queries
                new_X.clear();
                new_Y.clear();
                Q.clear();

                // Change filter mode to INCLUDE and let it be up to
                // IPMC_LIB_BASE_mesa_update() to remove empty sources and
                // the caller of us to remove the entire group if there's no
                // longer any active ports.
                new_filter_mode = VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE;
            } else {
                // EXCLUDE(X, Y) && TO_IN(A) => EXCLUDE(X + A, Y - A)
                new_X = old_X + A;
                new_Y = old_Y - A;

                // Send Q(G, X - A)
                Q = old_X - A;

                // Send Q(G)
                tx_group_specific_query = true;
            }

            // (A) = GMI/MALI
            TO = A;
            src_timeout = gmi;

            // Filter mode remains at EXCLUDE
            break;

        default:
            // This record would have been marked as invalid by ipmc_lib_packet.cxx
            // if the record_type was invalid, and it will therefore not be
            // processed (checked by the caller of this function).
            T_EG(IPMC_LIB_TRACE_GRP_RX, "Unreachable (%d)", grp_rec.record_type);
            return;
        }
    }

    // With the new calculated lists, update src_map.
    IPMC_LIB_BASE_src_map_update(vlan_state, grp_itr, port_no, new_X, new_Y);

    if (new_filter_mode != old_filter_mode) {
        // Change filter mode to EXCLUDE.
        IPMC_LIB_BASE_filter_mode_set(grp_itr, port_no, new_filter_mode);
    }

    // Update src_timeouts
    IPMC_LIB_BASE_src_map_timeout_update(vlan_state, grp_itr, port_no, TO, src_timeout, now);

    // Time to update H/W if needed
    IPMC_LIB_BASE_mesa_update(*vlan_state.global, grp_itr);

    // Check if we need to lower source timers and send a query ASAP.
    IPMC_LIB_BASE_tx_query_start(vlan_state, grp_itr, port_no, Q, tx_group_specific_query, pdu.report.is_leave, now);

    // Nice to see what has changed in our state.
    IPMC_LIB_BASE_debug_grp(port_no, grp_itr->first, old_filter_mode, old_X, old_Y, grp_rec.record_type, A, new_filter_mode, new_X, new_Y);
}

/******************************************************************************/
// IPMC_LIB_BASE_compatibility_check()
/******************************************************************************/
static bool IPMC_LIB_BASE_compatibility_check(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu)
{
    switch (vlan_state.conf.compatibility) {
    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO:
        // We support all versions, but we might use only parts of the PDU in
        // case the group(s) in the PDU are in an older-version compatibility
        // mode on this port.
        return true;

    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD:
        // Ignore IGMPv2 and IGMPv3 PDUs.
        return pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V1;

    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN:
        // Ignore IGMPv1, IGMPv3 and MLDv2 PDUs, that is, only allow IGMPv2 and
        // MLDv1 PDUs.
        return pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V2 || pdu.version == IPMC_LIB_PDU_VERSION_MLD_V1;

    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM:
    default:
        // Ignore all but IGMPv3 and MLDv2 PDUs.
        return pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V3 || pdu.version == IPMC_LIB_PDU_VERSION_MLD_V2;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_proxy_grp_push()
/******************************************************************************/
static void IPMC_LIB_BASE_proxy_grp_push(ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr)
{
    ipmc_lib_grp_key_t grp_key = {};

    grp_key.vlan_key = vlan_state.vlan_key;
    grp_key.grp_addr = grp_addr;

    if (!vlan_state.global->lists->proxy_grp_map.set(grp_key, &vlan_state)) {
        // Out of memory.
        // RBNTBD: Update operational state?
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_prefix_match()
/******************************************************************************/
static bool IPMC_LIB_BASE_prefix_match(ipmc_lib_vlan_state_t &vlan_state, const vtss_appl_ipmc_lib_ip_t &grp_addr)
{
    vtss_appl_ipmc_lib_ip_t prefix_mask = {};
    vtss_appl_ipmc_lib_ip_t &ssm_addr      = vlan_state.global->conf.ssm_prefix;
    uint32_t                ssm_prefix_len = vlan_state.global->conf.ssm_prefix_len;

    if (grp_addr.is_ipv4 != ssm_addr.is_ipv4) {
        T_EG(IPMC_LIB_TRACE_GRP_RX, "Cannot match IPv4 with IPv6 (grp_addr = %s, ssm_addr = %s)", grp_addr, ssm_addr);
        return false;
    }

    prefix_mask.is_ipv4 = ssm_addr.is_ipv4;
    if (ssm_addr.is_ipv4) {
        prefix_mask.ipv4 = vtss_ipv4_prefix_to_mask(ssm_prefix_len);
    } else {
        if (vtss_conv_prefix_to_ipv6mask(ssm_prefix_len, &prefix_mask.ipv6) != VTSS_RC_OK) {
            // Something went wrong.
            T_EG(IPMC_LIB_TRACE_GRP_RX, "Unable to convert prefix-length = %u to a mask", ssm_prefix_len);
            return false;
        }
    }

    return (ssm_addr & prefix_mask) == (grp_addr & prefix_mask);
}

/******************************************************************************/
// IPMC_LIB_BASE_grp_cnt_max_reached()
/******************************************************************************/
static bool IPMC_LIB_BASE_grp_cnt_max_reached(ipmc_lib_global_state_t &glb, mesa_port_no_t port_no)
{
    ipmc_lib_grp_itr_t grp_itr;
    ipmc_lib_grp_map_t &grp_map = glb.lists->grp_map;
    uint32_t           grp_cnt_cur, grp_cnt_max;

    grp_cnt_max = glb.port_conf[port_no].grp_cnt_max;

    if (grp_cnt_max == 0) {
        // Disabled.
        return false;
    }

    grp_cnt_cur = 0;
    for (grp_itr = grp_map.begin(); grp_itr != grp_map.end(); ++grp_itr) {
        if (static_cast<vtss_appl_ipmc_lib_key_t>(grp_itr->first.vlan_key) != glb.key) {
            // This group is not one we are looking for.
            continue;
        }

        if (grp_itr->second.active_ports[port_no]) {
            // We are active on the port.
            if (++grp_cnt_cur >= grp_cnt_max) {
                // And maximum is already reached.
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "grp_cnt_max (%u) reached", grp_cnt_max);
                return true;
            }
        }
    }

    // If we get here, the max is not yet reached.
    return false;
}

/******************************************************************************/
// IPMC_LIB_BASE_rx_report()
// This function returns false, if we receive an IGMPv2/MLDv1 Leave/Done PDU
// without having an active group on this port, because the caller needs to know
// how to handle forwarding of the PDU.
/******************************************************************************/
static bool IPMC_LIB_BASE_rx_report(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu, vtss_appl_ipmc_lib_profile_key_t &profile_key)
{
    vtss_appl_ipmc_lib_compatibility_t new_grp_compat;
    ipmc_lib_grp_map_t                 &grp_map = vlan_state.global->lists->grp_map;
    ipmc_lib_grp_itr_t                 grp_itr;
    ipmc_lib_grp_key_t                 grp_key;
#ifdef VTSS_SW_OPTION_SMB_IPMC
    ipmc_lib_profile_match_t           profile_match;
#endif
    mesa_port_no_t                     port_no = pdu.rx_info.port_no;
    uint16_t                           rec_cnt, src_cnt;
    uint32_t                           now;
    bool                               count_as_ignored = false, init_port, grp_in_ssm_range, update_host_compat = false;
    bool                               result = true, new_group = false, at_least_one_grp_rec_processed = false;

    if (!vlan_state.vlan_key.is_mvr && (pdu.version != IPMC_LIB_PDU_VERSION_IGMP_V3 && pdu.version != IPMC_LIB_PDU_VERSION_MLD_V2)) {
        // We do not accept IGMPv1/IGMPv2/MLDv1 PDUs in SSM range.
        // Older version reports only had support for a single group record, so
        // we don't have to compute this for every group record in the PDU.
        grp_in_ssm_range = IPMC_LIB_BASE_prefix_match(vlan_state, pdu.report.group_recs[0].grp_addr);

        if (grp_in_ssm_range) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "Received %s report with a group address of %s, which is in the SSM range. Ignoring", ipmc_lib_pdu_version_to_str(pdu.version), pdu.report.group_recs[0].grp_addr);
            count_as_ignored = true;
            goto do_exit;
        }
    }

    // First check our configured compatibility mode against this PDU.
    // See RFC3376, 7.3.2 and 9.2.
    if (!IPMC_LIB_BASE_compatibility_check(vlan_state, pdu)) {
        count_as_ignored = true;
        goto do_exit;
    }

    // Find the group compatibility.
    // See RFC3376, 7.3.2.
    switch (pdu.version) {
    case IPMC_LIB_PDU_VERSION_IGMP_V1:
        new_grp_compat = VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD;
        break;

    case IPMC_LIB_PDU_VERSION_IGMP_V2:
    case IPMC_LIB_PDU_VERSION_MLD_V1:
        new_grp_compat = VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN;
        break;

    case IPMC_LIB_PDU_VERSION_IGMP_V3:
    case IPMC_LIB_PDU_VERSION_MLD_V2:
    default:
        new_grp_compat = VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM;
        break;
    }

#ifdef VTSS_SW_OPTION_SMB_IPMC
    // Fill in the constant part of the profile match structure
    profile_match.profile_key = profile_key;
    profile_match.vid         = vlan_state.vlan_key.vid;
    profile_match.port_no     = port_no;
    profile_match.src         = pdu.sip;
#endif

    // Timestamp everything with the same number of seconds since boot.
    now = vtss::uptime_seconds();

    // Loop across all records in this PDU. The PDU is already in such a shape
    // that we can use pdu.record_type directly whether or not it is an IGMPv3
    // or MLDv2 PDU.
    for (rec_cnt = 0; rec_cnt < pdu.report.rec_cnt; rec_cnt++) {
        const ipmc_lib_pdu_group_record_t &grp_rec = pdu.report.group_recs[rec_cnt];

        if (!grp_rec.valid) {
            // This record was found to be invalid by the parser. Go on with
            // the next.
            T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s is not valid. Skipping", grp_rec.grp_addr);
            continue;
        }

#ifdef VTSS_SW_OPTION_SMB_IPMC
        // Check if we should filter out this group address
        profile_match.dst = grp_rec.grp_addr;

        if (!ipmc_lib_profile_permit(vlan_state, profile_match, true /* log if we run into an entry with logging enabled */)) {
            // Skip this group if it matches a deny rule
            T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s matches a deny rule", grp_rec.grp_addr);
            continue;
        }
#endif

        T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Record type = %s", grp_rec.grp_addr, ipmc_lib_pdu_record_type_to_str(grp_rec.record_type));

        grp_key.vlan_key = vlan_state.vlan_key;
        grp_key.grp_addr = grp_rec.grp_addr;

        if ((grp_itr = grp_map.find(grp_key)) == grp_map.end()) {
            // New group.

            // Ignore the record if it's of type IS_IN or TO_IN with an empty
            // source list, because that corresponds to including nothing (a
            // leave).
            if ((grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_IS_IN || grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_TO_IN) && grp_rec.src_list.size() == 0) {
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Leave on unregistered group received", grp_rec);

                if (pdu.report.is_leave) {
                    // This is an IGMPv2 or MLDv1 leave/done PDU. Tell the
                    // caller this, so that the frame can be forwarded
                    // correctly.
                    // There's only one record in such PDUs, so we can safely
                    // return.
                    result           = false;
                    count_as_ignored = true;
                    goto do_exit; // Gotta count it anyway - in the ignored bucket
                }

                continue;
            }

            // Check if we have room for this.
            if (grp_map.size() >= vlan_state.global->lib_cap.grp_cnt_max) {
                // We have exceeded the maximum number of supported groups.
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Maximum number of supported groups (%u) exceeded. Ignoring new", grp_rec.grp_addr, vlan_state.global->lib_cap.grp_cnt_max);

                // RBNTBD: Update operational state?
                goto do_exit;
            }

            // Also check whether we need to ignore the new group because of
            // reaching a user-configured maximum number of groups.
            if (IPMC_LIB_BASE_grp_cnt_max_reached(*vlan_state.global, port_no)) {
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Max number of groups reached", grp_rec);
                count_as_ignored = !at_least_one_grp_rec_processed;
                goto do_exit;
            }

            // Create a new entry
            if ((grp_itr = grp_map.get(grp_key)) == grp_map.end()) {
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Out of memory", grp_rec.grp_addr);

                // RBNTBD: Update operational state?
                goto do_exit;
            }

            vtss_clear(grp_itr->second);

            // Set the grp_compat to the version of the received PDU, unless we
            // are in forced compatibility mode. Later on in this function, we
            // will set timeouts if configured compatibility is AUTO.
            grp_itr->second.grp_compat = vlan_state.conf.compatibility == VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO ? new_grp_compat : vlan_state.conf.compatibility;
            update_host_compat = true;

            // Create a pointer back to the vlan_state that created this, so
            // that we can always find it when iterating over the entire group
            // map.
            grp_itr->second.vlan_state = &vlan_state;

            init_port = true;
            new_group = true;
        } else if (!grp_itr->second.active_ports[port_no]) {
            // Not a new group, but there's been no listeners on this port
            // before. Initialize it.
            init_port = true;
        } else {
            // Group already exists and the port is indeed already active. Don't
            // initialize it.
            init_port = false;
        }

        if (init_port) {
            // Ignore the record if it's of type IS_IN or TO_IN with an empty
            // source list, because that corresponds to include nothing
            // (a leave).
            // This is the second time we do this check if we are also creating
            // a new group. The reason is that we shouldn't create the group if
            // it's a leave (check above).
            // If, however, the group already exists, we should not process the
            // group record if it's a leave and the port is not yet active (no
            // listeners have been seen yet on this particular port).
            if ((grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_IS_IN || grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_TO_IN) && grp_rec.src_list.size() == 0) {
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Leave on unregistered group received", grp_rec);

                if (pdu.report.is_leave) {
                    // This is an IGMPv2 or MLDv1 leave/done PDU. Tell the
                    // caller this, so that the frame can be forwarded
                    // correctly.
                    // There's only one record in such PDUs, so we can safely
                    // return.
                    result           = false;
                    count_as_ignored = true;
                    goto do_exit; // Gotta count it anyway - in the ignored bucket
                }

                continue;
            }

            // Also check whether we need to ignore the new group because of
            // reaching a user-configured maximum number of groups.
            if (IPMC_LIB_BASE_grp_cnt_max_reached(*vlan_state.global, port_no)) {
                T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Max number of groups reached", grp_rec);
                count_as_ignored = !at_least_one_grp_rec_processed;
                goto do_exit;
            }

            // Always initialize the filter mode to INCLUDE, because that will
            // work for all record types received.
            // The following function also marks asm_state as changed. This will
            // cause an ASM entry to be installed.
            IPMC_LIB_BASE_filter_mode_set(grp_itr, port_no, VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE);
        }

        at_least_one_grp_rec_processed = true;

        ipmc_lib_grp_state_t &grp_state = grp_itr->second;

        if (!grp_state.active_ports[port_no]) {
            // We haven't previously received a report on this port. Add it to
            // the list of active ports and get an ASM entry installed or
            // updated.
            // It will be marked as inactive once the port's group timer expires
            // or when all source timers on the port in INCLUDE mode expire.
            grp_state.active_ports[port_no] = true;
            grp_state.asm_state.changed     = true;
        }

        // We may have to ignore sources in the group, so make it variable.
        src_cnt = grp_rec.src_list.size();

        if (vlan_state.conf.compatibility == VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO) {
            if (new_grp_compat != grp_state.grp_compat) {
                update_host_compat = true;
            }

            // We might have to update the older version timers.
            switch (new_grp_compat) {
            case VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD:
                // This is an IGMPv1 PDU. Update the group compatibility to be
                // IGMPv1 and set the timer (again).
                grp_state.grp_compat = new_grp_compat;
                grp_state.grp_older_version_host_present_timeout_old = IPMC_LIB_BASE_older_version_host_present_interval_get(vlan_state) + now;
                break;

            case VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN:
                // This is an IGMPv2 or MLDv1 PDU. Update the group
                // compatibility unless it's set to IGMPv1, in which case the
                // IGMPv1 older version host present timer needs to expire
                // before we set the group compatibility to IGMPv2.
                if (grp_state.grp_compat != VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD) {
                    grp_state.grp_compat = new_grp_compat;
                }

                // And always update the IGMPv2/MLDv1 timer.
                grp_state.grp_older_version_host_present_timeout_gen = IPMC_LIB_BASE_older_version_host_present_interval_get(vlan_state) + now;
                break;

            case VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM:
            default:
                // Leave the current grp_compat at whatever it is.
                break;
            }

            // RFC3376 section 7.3.2 and RFC3810 section 8.3.2.
            if (grp_state.grp_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD) {
                // IGMPv1 compatibility
                if (grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_BLOCK) {
                    // Ignore IGMPv3 block messages.
                    // We can't get in here, if it's a new group, so no need to
                    // bother to remove that again.
                    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Ignoring BLOCK", grp_rec);
                    continue;
                }

                if (grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_TO_EX) {
                    // Ignore source count in TO_EX messages.
                    src_cnt = 0;
                }

                if (pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V3 && grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_TO_IN) {
                    // Ignore IGMPv3 TO_IN messages
                    // We can't get in here, if it's a new group, so no need to
                    // bother to remove that again.
                    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Ignoring IGMPv3 TO_IN", grp_rec);
                    continue;
                }

                if (pdu.report.is_leave) {
                    // Ignore IGMPv2 Leave messages
                    // We can't get in here, if it's a new group, so no need to
                    // bother to remove that again.
                    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Ignoring IGMPv2 Leave", grp_rec);
                    continue;
                }
            } else if (grp_state.grp_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN) {
                // IGMPv2/MLDv1 compatibility
                if (grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_BLOCK) {
                    // Ignore IGMPv3/MLDv2 BLOCK messages
                    // We can't get in here, if it's a new group, so no need to
                    // bother to remove that again.
                    T_IG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Ignoring BLOCK", grp_rec);
                    continue;
                }

                if (grp_rec.record_type == IPMC_LIB_PDU_RECORD_TYPE_TO_EX) {
                    // Ignore source count in TO_EX messages.
                    src_cnt = 0;
                }
            }
        } else {
            // Leave grp_compat at the configured value.
        }

        // Time to process the record according to RFC3376, section 6.4.1 and
        // 6.4.2.
        IPMC_LIB_BASE_process_src(vlan_state, pdu, grp_itr, grp_rec, src_cnt, now);

        // If this was an IGMPv2 leave or an MLDv1 Done and fast_leave is
        // enabled, we might have backed out of the port, so remove the entire
        // group from the map if there's no longer any active ports.
        if (grp_state.active_ports.is_empty()) {
            T_IG(IPMC_LIB_TRACE_GRP_RX, "%s: Erasing", grp_itr->first);
            IPMC_LIB_BASE_grp_itr_erase(grp_itr);
        } else {
            // If we are proxy, we send a report for this group on the next
            // tick.
            if (new_group && !pdu.report.is_leave && vlan_state.global->conf.proxy_enable) {
                // RBNTBD: I don't understand why we only send a report whenever
                // we create a new group.

                // We do the transmission of the report to router ports
                // ourselves rather than having the caller of us forwarding it.
                // The caller knows this and doesn't forward this PDU.
                // Push it to the list of proxy reports to transmit.
                // Leave PDUs are sent during calls to
                // IPMC_LIB_BASE_grp_itr_erase(), so don't add those.
                IPMC_LIB_BASE_proxy_grp_push(vlan_state, grp_itr->first.grp_addr);
            }

            if (update_host_compat) {
                IPMC_LIB_BASE_host_compat_update(vlan_state);
            }
        }
    }

do_exit:
    // Update statistics
    ipmc_lib_pdu_statistics_update(vlan_state, pdu, true /* is Rx */, count_as_ignored);
    return result;
}

/******************************************************************************/
// IPMC_LIB_BASE_rx_query_forward_decision_get()
/******************************************************************************/
static bool IPMC_LIB_BASE_rx_query_forward_decision_get(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu, mesa_port_list_t &dst_port_mask)
{
    mesa_port_no_t port_no;

    if (vlan_state.global->query_flooding_cnt >= vlan_state.global->protocol_cap.vlan_cnt_max) {
        // Too much flooding. Skip this one.
        return false;
    }

    if (vlan_state.global->conf.proxy_enable) {
        // If we are a proxy, we only send the query to other routers and
        // not to the hosts. After a while, we summarize a report that we
        // send to all routers.
        // This is whether it's a general query or a group-specific query.
        dst_port_mask = ipmc_lib_base_router_port_mask_get(*vlan_state.global);
        return true;
    } else {
        // We are not a proxy.
        if (pdu.query.grp_addr.is_zero()) {
            if (vlan_state.vlan_key.is_mvr) {
                // This is an MVR VLAN. General queries are forwarded to other
                // receiver and source ports.
                for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
                    dst_port_mask[port_no] = vlan_state.port_conf[port_no].role != VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE;
                }
            } else {
                // This is an IPMC VLAN. General queries are forwarded to all
                // other interfaces on the VLAN, routers or not.
                dst_port_mask.set_all();
            }

            return true;
        } else {
            // Group-specific queries only go to members of that group
            ipmc_lib_grp_map_t &grp_map = vlan_state.global->lists->grp_map;
            ipmc_lib_grp_key_t grp_key;
            ipmc_lib_grp_itr_t grp_itr;

            // Group-specific queries should only be forwarded to those
            // interfaces that are members of the group.
            grp_key.vlan_key = vlan_state.vlan_key;
            grp_key.grp_addr = pdu.query.grp_addr;

            if ((grp_itr = grp_map.find(grp_key)) == grp_map.end()) {
                // Can't find that group. Discard
                return false;
            }

            dst_port_mask = grp_itr->second.active_ports;
            return true;
        }
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_rx_report_forward_decision_get()
/******************************************************************************/
static bool IPMC_LIB_BASE_rx_report_forward_decision_get(ipmc_lib_vlan_state_t &vlan_state, mesa_port_list_t &dst_port_mask, bool is_leave, bool is_leave_without_group)
{
    mesa_port_no_t port_no;
    bool           proxy = vlan_state.global->conf.proxy_enable;
    bool           router_forward;

    if (is_leave) {
        proxy |= vlan_state.global->conf.leave_proxy_enable;

        // We forward to routers if we have received an IGMPv2 Leave or MLDv1
        // Done PDU for which we didn't know the group in advance or if we are
        // not acting as proxy.
        router_forward = is_leave_without_group || !proxy;
    } else {
        // We forward other PDUs to routers if we are not a proxy.
        router_forward = !proxy;
    }

    if (router_forward) {
        if (vlan_state.vlan_key.is_mvr) {
            // MVR forwards reports to source ports. MVR will never mark a port
            // as a router port unless it's also a source port, because queries
            // won't get into IPMC_LIB_BASE_rx_query() unless it's received on
            // a source port.
            for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
                dst_port_mask[port_no] = vlan_state.port_conf[port_no].role == VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE;
            }
        } else {
            dst_port_mask = ipmc_lib_base_router_port_mask_get(*vlan_state.global);
        }

        return true;
    }

    return false;
}

/******************************************************************************/
// IPMC_LIB_BASE_querier_state_update_from_pdu()
/******************************************************************************/
static void IPMC_LIB_BASE_querier_state_update_from_pdu(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu)
{
    vtss_appl_ipmc_lib_ip_t our_ip;

    if (pdu.sip.is_zero()) {
        // Nothing to update.
        return;
    }

    ipmc_lib_pdu_our_ip_get(vlan_state, our_ip);

    if (pdu.sip < our_ip) {
        // We are not/no longer the querier.
        vlan_state.status.querier_state                  = VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE;
        vlan_state.status.active_querier_address         = pdu.sip;
        vlan_state.internal_state.startup_query_cnt_left = 0;
        vlan_state.status.other_querier_expiry_time      = IPMC_LIB_BASE_other_querier_present_timeout_get(vlan_state);
    } else {
        if (!vlan_state.conf.querier_enable) {
            if (vlan_state.status.active_querier_address.is_zero()) {
                vlan_state.status.active_querier_address = pdu.sip;
            } else if (pdu.sip < vlan_state.status.active_querier_address) {
                vlan_state.status.active_querier_address = pdu.sip;
            }
        }
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_proxy_report_timeout_set()
// Sets a random timeout from 0 to max response time.
/******************************************************************************/
static void IPMC_LIB_BASE_proxy_report_timeout_set(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu)
{
    uint32_t timeout = pdu.query.max_response_time_ms / 1000;

    if (timeout == 0) {
        timeout = pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V1 ? 1 : vlan_state.internal_state.cur_qri / 10;

        if (timeout == 0) {
            timeout = 1;
        }
    }

    // Make it random
    timeout = (uint32_t)rand() % timeout;
    if (timeout == 0) {
        timeout = 0x1;
    }

    // Restart response timer
    if (vlan_state.internal_state.proxy_report_timeout > timeout) {
        vlan_state.internal_state.proxy_report_timeout = timeout;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_flood_mask_set()
// Sets the list of ports to flood non-IGMP/non-MLD M/C traffic to.
/******************************************************************************/
static void IPMC_LIB_BASE_flood_mask_set(vtss_appl_ipmc_lib_key_t &key, const mesa_port_list_t &flood_mask)
{
    static mesa_port_list_t user_flood_mask[4];
    static mesa_port_list_t mesa_flood_mask[2]; // Flood masks sent to MESA, depending on protocol (IGMP/MLD)
    static bool             initialized;
    int                     i, user_idx;
    mesa_port_list_t        combined_flood_mask;
    mesa_rc                 rc;

    // About user_flood_mask[]:
    //   This is a static array utilized as follows:
    //     [0]: IPMC-IGMP
    //     [1]: IPMC-MLD
    //     [2]: MVR-IGMP
    //     [3]: MVR-MLD
    //
    //   We could have made the sizes compile-time dynamic as we do in
    //   ipmc_lib.cxx, but that would clutter the code too much, IMO.
    //
    //   All four entries are initialized to all-ones the first time this
    //   function is called. So if a protocol or IP family is not supported, its
    //   values will never be changed and therefore it won't affect the final
    //   outcome (see below on an explanation of why).
    //
    // About mesa_flood_mask[]:
    //   This is a static array utilized as follows:
    //     [0]: IPv4
    //     [1]: IPv6
    //
    //   It contains the latest flood mask sent to MESA, so that we can compare
    //   with a new flood mask before invoking MESA (which I guess - not sure,
    //   however - is a time-consuming thing).
    //
    // The flood mask sent to MESA is calculated as follows:
    //   If no users are enabled, the flood mask gets set to all ones, which is
    //   also the default in MESA, meaning that M/C frames are forwarded to all
    //   ports.
    //   If at least one user is enabled, she typically wants to forward all
    //   M/C frames to router ports, only, so listener/host ports should not
    //   receive the M/C traffic by default - only when a listener/host requests
    //   it, which happens through IGMP/MLD reports. Also IGMP and MLD frames
    //   will not be forwarded to other ports by the switch, but will have to
    //   be forwarded by S/W if needed in this case (see
    //   ipmc_lib.cxx#IPMC_LIB_rx_register_update_mesa()).
    //
    //   This means that flooding to a specific port will only happen if both
    //   IPMC and MVR agree that flooding to that port is enabled (both are 1)
    //   for a given IP family. This is also why it's safe to initialize the
    //   user_flood_mask entries to all-ones.

    if (!initialized) {
        // By default, we flood IPv4 and IPv6 M/C to all ports.
        for (i = 0; i < ARRSZ(user_flood_mask); i++) {
            user_flood_mask[i].set_all();
        }

        // MESA's documentation says that it also resets its flood mask to
        // all-ones, so we do the same.
        for (i = 0; i < ARRSZ(mesa_flood_mask); i++) {
            mesa_flood_mask[i].set_all();
        }

        initialized = true;
    }

    // Convert key to an index according to the table above.
    user_idx = key.is_mvr ? (key.is_ipv4 ? 2 : 3) : (key.is_ipv4 ? 0 : 1);

    if (user_flood_mask[user_idx] == flood_mask) {
        // No changes
        return;
    }

    // Save it
    user_flood_mask[user_idx] = flood_mask;

    // Find combined flood mask for IPv4 or IPv6. Remember, only ports for which
    // both protocols say "flood" will flood.
    combined_flood_mask = key.is_ipv4 ? (user_flood_mask[0] & user_flood_mask[2]) : (user_flood_mask[1] & user_flood_mask[3]);

    if (combined_flood_mask == mesa_flood_mask[key.is_ipv4 ? 0 : 1]) {
        // No changes
        return;
    }

    // Save it
    mesa_flood_mask[key.is_ipv4 ? 0 : 1] = combined_flood_mask;

    // Time to send to MESA
    if (key.is_ipv4) {
        // IPv4
        T_IG(IPMC_LIB_TRACE_GRP_API, "%s: mesa_ipv4_mc_flood_members_set(%s)", key, combined_flood_mask);
        if ((rc = mesa_ipv4_mc_flood_members_set(nullptr, &combined_flood_mask)) != VTSS_RC_OK) {
            T_EG(IPMC_LIB_TRACE_GRP_API, "%s: mesa_ipv4_mc_flood_members_set(%s) failed: %s", key, combined_flood_mask, error_txt(rc));
        }
    } else {
        // IPv6
        T_IG(IPMC_LIB_TRACE_GRP_API, "%s: mesa_ipv6_mc_flood_members_set(%s)", key, combined_flood_mask);
        if ((rc = mesa_ipv6_mc_flood_members_set(nullptr, &combined_flood_mask)) != VTSS_RC_OK) {
            T_EG(IPMC_LIB_TRACE_GRP_API, "%s: mesa_ipv6_mc_flood_members_set(%s) failed: %s", key, combined_flood_mask, error_txt(rc));
        }
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_rx_query()
/******************************************************************************/
static void IPMC_LIB_BASE_rx_query(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu)
{
    ipmc_lib_grp_map_t &grp_map = vlan_state.global->lists->grp_map;
    ipmc_lib_grp_itr_t grp_itr;
    ipmc_lib_grp_key_t grp_key;
    bool               proxy_enabled;
    uint32_t           now = vtss::uptime_seconds(), lmqt;
    mesa_port_no_t     port_no = pdu.rx_info.port_no;
    bool               count_as_ignored = false;

    // First check our configured compatibility mode against this PDU.
    // See RFC3376, 7.3.1
    if (!IPMC_LIB_BASE_compatibility_check(vlan_state, pdu)) {
        count_as_ignored = true;
        goto do_exit;
    }

    if (!pdu.sip.is_zero()) {
        // Set the source port as a dynamic router port
        T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "Port %d now dynamic", port_no);
        ipmc_lib_base_router_status_update(*vlan_state.global, port_no, true /* dynamic */, true /* add */);

        // Also set a timeout for this dynamic router port.
        vlan_state.global->port_status[port_no].dynamic_router_timeout = 300; // Five minutes

    } else {
        // RFC4541 section 2.1.1.b): The 0.0.0.0 address is used to indicate
        // that the Query packets are NOT from a multicast router, so don't
        // include that in the router port list.
    }

    // Update Querier state
    IPMC_LIB_BASE_querier_state_update_from_pdu(vlan_state, pdu);

    // Suppress flooding of this query if we have received too many. The caller
    // of us takes care of forwarding based on the value of this variable.
    // It decrements for every tick.
    vlan_state.global->query_flooding_cnt++;

    proxy_enabled = vlan_state.global->conf.proxy_enable;

    if (pdu.query.grp_addr.is_zero()) {
        // General query.
        T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "Received General Query. Proxy enabled = %d", proxy_enabled);

        if (proxy_enabled) {
            IPMC_LIB_BASE_proxy_report_timeout_set(vlan_state, pdu);
        }

        goto update_querier_compat;
    }

    // Group-specific query
    T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "Received group-specific query (%s)", pdu.query.grp_addr);

    // Look up group address
    grp_key.vlan_key = vlan_state.vlan_key;
    grp_key.grp_addr = pdu.query.grp_addr;
    if ((grp_itr = grp_map.find(grp_key)) == grp_map.end()) {
        // Group not found in our map. Go on.
        goto update_querier_compat;
    }

    T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: Found group. Querier state = %d", grp_itr->first, vlan_state.status.querier_state);
    if (vlan_state.status.querier_state != VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE) {
        // We are querier. Don't listen to other queriers' queries (except for
        // querier election.
        goto check_proxy;
    }

    // We are not the current querier
    if (pdu.query.s_flag) {
        // RFC3376, section 4.1.5 and RFC3810, section 5.1.7 and 7.6.1.
        // Ignore timer updates.
        goto check_proxy;
    }

    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!grp_itr->second.active_ports[port_no]) {
            continue;
        }

        T_DG_PORT(IPMC_LIB_TRACE_GRP_RX, port_no, "%s: cur_qri: %u -> %u, cur_rv: %u -> %u, cur_qi: %u -> %u, cur_lmqi: %u -> %u",
                  vlan_state.vlan_key,
                  vlan_state.internal_state.cur_qri,  pdu.query.max_response_time_ms / 100,
                  vlan_state.internal_state.cur_rv,   pdu.query.qrv,
                  vlan_state.internal_state.cur_qi,   pdu.query.qqi,
                  vlan_state.internal_state.cur_lmqi, pdu.query.qqi);

        // Even in IGMPv1, the IPMC_LIB has set max response
        // time to 10 seconds (10000 ms).
        vlan_state.internal_state.cur_qri = pdu.query.max_response_time_ms / 100; // In 10ths of a second.

        if (pdu.query.qrv) {
            // RFC3376, 4.1.6.
            // ipmc_lib_packet.cxx has already changed pdu.query.qrv to 0 if
            // it was > 7 in the PDU.
            vlan_state.internal_state.cur_rv = pdu.query.qrv;
        }

        if (pdu.query.qqi) {
            // RFC3376, 4.1.7
            vlan_state.internal_state.cur_qi = pdu.query.qqi;
        }

        if (pdu.query.qqi) {
            vlan_state.internal_state.cur_lmqi = pdu.query.qqi;
        }

        // Lower group or source timers.
        lmqt = IPMC_LIB_BASE_last_member_query_time_get(vlan_state) + now;
        if (pdu.query.src_list.empty()) {
            IPMC_LIB_BASE_lower_group_timer(vlan_state, grp_itr, port_no, lmqt, now, true /* based on Rx of a query */);
        } else {
            IPMC_LIB_BASE_lower_source_timers(vlan_state, grp_itr, port_no, pdu.query.src_list, lmqt, now, true /* based on Rx of a query */);
        }
    }

check_proxy:
    if (proxy_enabled) {
        // We need to send a report for this group-specific query a bit
        // later (we do it in IPMC_LIB_BASE_proxy_grp_map_tick()), so push
        // the group address to the list of proxy reports to send.
        IPMC_LIB_BASE_proxy_grp_push(vlan_state, pdu.query.grp_addr);
    }

update_querier_compat:
    // Update router compatibility mode
    if (vlan_state.conf.compatibility != VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO) {
        // No change in querier compatibility
        goto do_exit;
    }

    if (pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V1) {
        // This is an IGMPv1 PDU. Update the querier compatibility to be IGMPv1
        // and set the timer (again).
        vlan_state.status.querier_compat = VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD;
        vlan_state.status.older_version_querier_present_timeout_old = IPMC_LIB_BASE_older_version_querier_present_interval_get(vlan_state) + now;
    } else if (pdu.version == IPMC_LIB_PDU_VERSION_IGMP_V2 || pdu.version == IPMC_LIB_PDU_VERSION_MLD_V1) {
        // This is an IGMPv2 or MLDv1 PDU. Update the querier compatibility
        // unless it's set to IGMPv1, in which case the IGMPv1 older version
        // querier present timer needs to expire before we set the querier
        // compatibilitity to IGMPv2.
        if (vlan_state.status.querier_compat != VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD) {
            vlan_state.status.querier_compat = VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN;
        }

        // And always update the IGMPv2/MLDv1 timer
        vlan_state.status.older_version_querier_present_timeout_gen = IPMC_LIB_BASE_older_version_querier_present_interval_get(vlan_state) + now;
    } else {
        // No change in querier compatibility
    }

do_exit:
    // Update statistics
    ipmc_lib_pdu_statistics_update(vlan_state, pdu, true /* is Rx */, count_as_ignored);
}

/******************************************************************************/
// IPMC_LIB_BASE_grp_port_remove()
// Marks a port in the group as inactive and removes the group if there are no
// more active ports.
// Upon return grp_itr may no longer be valid.
/******************************************************************************/
static void IPMC_LIB_BASE_grp_port_remove(ipmc_lib_grp_itr_t &grp_itr, mesa_port_no_t port_no)
{
    ipmc_lib_src_itr_t   src_itr;
    ipmc_lib_grp_state_t &grp_state = grp_itr->second;
    ipmc_lib_src_map_t   &src_map   = grp_state.src_map;

    if (!grp_state.active_ports[port_no]) {
        // Group is not active on this port. Nothing to do.
        return;
    }

    // Remove this port from all sources while marking those where the port is
    // in use as changed.
    for (src_itr = src_map.begin(); src_itr != src_map.end(); ++src_itr) {
        ipmc_lib_src_state_t &src_state = src_itr->second;

        if (src_state.include_port_list[port_no] || src_state.exclude_port_list[port_no]) {
            src_state.include_port_list[port_no] = false;
            src_state.exclude_port_list[port_no] = false;
            src_state.changed                    = true;
            src_map.changed                      = true;
        }
    }

    grp_state.active_ports[port_no] = false;
    grp_state.asm_state.changed     = true;

    // Update the chip
    IPMC_LIB_BASE_mesa_update(*grp_state.vlan_state->global, grp_itr);

    if (grp_state.active_ports.is_empty()) {
        // No more active ports for this group. Delete the entire group.
        IPMC_LIB_BASE_grp_itr_erase(grp_itr);
    }
}

/******************************************************************************/
// ipmc_lib_base_compatibility_status_update()
/******************************************************************************/
void ipmc_lib_base_compatibility_status_update(ipmc_lib_vlan_state_t &vlan_state, vtss_appl_ipmc_lib_compatibility_t old_compat)
{
    ipmc_lib_grp_map_t                 &grp_map = vlan_state.global->lists->grp_map;
    ipmc_lib_grp_itr_t                 grp_itr, grp_itr_next;
    vtss_appl_ipmc_lib_compatibility_t new_compat = vlan_state.conf.compatibility;
    bool                               rgcs;

    // RGCS    = Remove Groups Containing Sources.
    // UGC     = Update Group Compatibility (and thereby Host Compatibility)
    // UQC     = Update Querier Compatibility
    // COVHPT  = Clear Older Version Host Present Timers (both of them).
    // SOVHPTO = Start Older Version Host Present Timer Old (for IGMPv1)
    // SOVHPTG = Start Older Version Host Present Timer Gen (for IGMPv2/MLDv1)
    //
    // Changes to do:
    // +---------------------------------------------------------------------+
    // | Old    | New    || Group + Host                     | Router Compat |
    // | Compat | Compat || Compat                           |               |
    // |--------|--------||----------------------------------|---------------|
    // | Auto   | Auto   || None                             | None          |
    // | Auto   | Old    || RGCS  UGC COVHPT                 | UQC           |
    // | Auto   | Gen    || RGCS  UGC COVHPT                 | UQC           |
    // | Auto   | Sfm    ||       UGC COVHPT                 | UQC           |
    // | Old    | Auto   ||                  SOVHPTO         | UQC           |
    // | Old    | Old    || None                             | None          |
    // | Old    | Gen    ||       UGC                        | UQC           |
    // | Old    | Sfm    ||       UGC                        | UQC           |
    // | Gen    | Auto   ||                          SOVHPTG | UQC           |
    // | Gen    | Old    ||       UGC                        | UQC           |
    // | Gen    | Gen    || None                             | None          |
    // | Gen    | Sfm    ||       UGC                        | UQC           |
    // | Sfm    | Auto   || None                             | UQC           |
    // | Sfm    | Old    || RGCS  UGC                        | UQC           |
    // | Sfm    | Gen    || RGCS  UGC                        | UQC           |
    // | Sfm    | Sfm    || None                             | None          |
    // +---------------------------------------------------------------------+
    if (new_compat == old_compat) {
        return;
    }

    vlan_state.status.querier_compat = vlan_state.conf.compatibility == VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO ? VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM : vlan_state.conf.compatibility;
    vlan_state.status.older_version_querier_present_timeout_old = 0;
    vlan_state.status.older_version_querier_present_timeout_gen = 0;

    rgcs = false;
    if (old_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO || old_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM) {
        rgcs = new_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD || new_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN;
    }

    grp_itr = grp_map.begin();
    while (grp_itr != grp_map.end()) {
        // Keep an iterator to the next entry, because we might delete the
        // current.
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        ipmc_lib_grp_state_t &grp_state = grp_itr->second;

        if (grp_state.vlan_state != &vlan_state) {
            // This group belongs to another VLAN ID. Go on.
            goto next;
        }

        if (rgcs && !grp_state.src_map.empty()) {
            T_IG(IPMC_LIB_TRACE_GRP_BASE, "%s: Deleting (grp_compat = %s) due to change of compatibility mode from %s to %s",
                 grp_itr->first.grp_addr,
                 ipmc_lib_util_compatibility_to_str(grp_itr->second.grp_compat, grp_itr->first.grp_addr.is_ipv4, true),
                 ipmc_lib_util_compatibility_to_str(old_compat, grp_itr->first.grp_addr.is_ipv4, true),
                 ipmc_lib_util_compatibility_to_str(new_compat, grp_itr->first.grp_addr.is_ipv4, true));

            IPMC_LIB_BASE_grp_itr_erase(grp_itr);
            goto next;
        }

        if (new_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO) {
            if (old_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD) {
                // SOVHPTO
                grp_itr->second.grp_older_version_host_present_timeout_old = IPMC_LIB_BASE_older_version_host_present_interval_get(vlan_state);
            } else if (old_compat == VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN) {
                // SOVHPTG
                grp_itr->second.grp_older_version_host_present_timeout_gen = IPMC_LIB_BASE_older_version_host_present_interval_get(vlan_state);
            }
        } else {
            // In forced compatibility, we never use the older version host
            // timers, so simply clear them.

            // COVHPT
            grp_state.grp_older_version_host_present_timeout_old = 0;
            grp_state.grp_older_version_host_present_timeout_gen = 0;

            // UGC
            grp_state.grp_compat = new_compat;
        }

next:
        grp_itr = grp_itr_next;
    }

    // Also update host compatibility
    IPMC_LIB_BASE_host_compat_update(vlan_state);
}

/******************************************************************************/
// ipmc_lib_base_proxy_grp_map_clear()
/******************************************************************************/
void ipmc_lib_base_proxy_grp_map_clear(ipmc_lib_vlan_state_t &vlan_state)
{
    ipmc_lib_proxy_grp_map_t    &proxy_grp_map = vlan_state.global->lists->proxy_grp_map;
    ipmc_lib_proxy_grp_map_itr_t proxy_map_itr, proxy_map_itr_next;

    proxy_map_itr = proxy_grp_map.begin();
    while (proxy_map_itr != proxy_grp_map.end()) {
        proxy_map_itr_next = proxy_map_itr;
        ++proxy_map_itr_next;

        if (proxy_map_itr->second == &vlan_state) {
            proxy_grp_map.erase(proxy_map_itr);
        }

        proxy_map_itr = proxy_map_itr_next;
    }
}

/******************************************************************************/
// ipmc_lib_base_deactivate()
/******************************************************************************/
void ipmc_lib_base_deactivate(ipmc_lib_vlan_state_t &vlan_state)
{
    ipmc_lib_grp_map_t &grp_map = vlan_state.global->lists->grp_map;
    ipmc_lib_grp_itr_t grp_itr, grp_itr_next;

    T_IG(IPMC_LIB_TRACE_GRP_BASE, "%s: Deactivating", vlan_state.vlan_key);

    grp_itr = grp_map.begin();
    while (grp_itr != grp_map.end()) {
        // We will delete this group as part of the following, so keep a ptr
        // to the next group
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        if (grp_itr->first.vlan_key != vlan_state.vlan_key) {
            // Group doesn't belong to this VLAN state.
            goto next;
        }

        // Remove the entire group by emptying active_ports.
        grp_itr->second.active_ports.clear_all();

        // Update the chip
        IPMC_LIB_BASE_mesa_update(*vlan_state.global, grp_itr);

        // And erase the group
        IPMC_LIB_BASE_grp_itr_erase(grp_itr);

next:
        grp_itr = grp_itr_next;
    }

    // Also clear the proxy list.
    ipmc_lib_base_proxy_grp_map_clear(vlan_state);

    // And reinitialize ourselves
    ipmc_lib_base_vlan_state_init(vlan_state, false /* don't clear statistics */);
}

/******************************************************************************/
// ipmc_lib_base_unregistered_flooding_update()
/******************************************************************************/
void ipmc_lib_base_unregistered_flooding_update(ipmc_lib_global_state_t &glb)
{
    mesa_port_list_t flood_mask;

    T_IG(IPMC_LIB_TRACE_GRP_BASE, "%s: conf = %s", glb.key, glb.conf);

    if (!glb.conf.admin_active || glb.conf.unregistered_flooding_enable) {
        flood_mask.set_all();
    } else {
        flood_mask = ipmc_lib_base_router_port_mask_get(glb);
    }

    IPMC_LIB_BASE_flood_mask_set(glb.key, flood_mask);
}

/******************************************************************************/
// ipmc_lib_base_grp_cnt_max_update()
// Invoked when the user configures a maximum group count that is smaller than
// the currently allowed.
// This configuration value is shared amongst all VLANs for a given protocol and
// IP family.
/******************************************************************************/
void ipmc_lib_base_grp_cnt_max_update(ipmc_lib_global_state_t &glb, mesa_port_no_t port_no)
{
    ipmc_lib_grp_itr_t grp_itr, grp_itr_next;
    ipmc_lib_src_itr_t src_itr;
    ipmc_lib_grp_map_t &grp_map = glb.lists->grp_map;
    uint32_t           grp_cnt_max, grp_cnt_cur;

    if (!glb.conf.admin_active) {
        return;
    }

    grp_cnt_max = glb.port_conf[port_no].grp_cnt_max;

    if (grp_cnt_max == 0) {
        // Disabled.
        return;
    }

    grp_cnt_cur = 0;
    grp_itr     = grp_map.begin();

    while (grp_itr != grp_map.end()) {
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        if (static_cast<vtss_appl_ipmc_lib_key_t>(grp_itr->first.vlan_key) != glb.key) {
            // This group is not affected by the changed group count.
            goto next;
        }

        if (!grp_itr->second.active_ports[port_no]) {
            // This group is not active on the requested port number
            goto next;
        }

        if (++grp_cnt_cur <= grp_cnt_max) {
            // We haven't yet reached the maximum. Leave the group alone.
            goto next;
        }

        // Inactivate the group on this port. Note that removing it may cause
        // the entire group to be removed, hence the grp_itr_next stuff.
        IPMC_LIB_BASE_grp_port_remove(grp_itr, port_no);

next:
        grp_itr = grp_itr_next;
    }
}

/******************************************************************************/
// IPMC_LIB_BASE_router_status_changed()
// Whenever a port changes from being a non-router port to a router-port, we
// need to add it to all <G, S> entries and the ASM (<G>) entry.
// Likewise, when a port changes from being a router-port to being a non-router
// port, we need to remove it from all <G, S> entries and the ASM entry if no
// hosts are present on that port.
//
// Router ports are shared amongst VLANs for a given protocol/IP family.
/******************************************************************************/
void IPMC_LIB_BASE_router_status_changed(ipmc_lib_global_state_t &glb)
{
    ipmc_lib_grp_map_t &grp_map = glb.lists->grp_map;
    ipmc_lib_grp_itr_t grp_itr;
    ipmc_lib_src_itr_t src_itr;

    T_IG(IPMC_LIB_TRACE_GRP_BASE, "Router state has changed for one of the ports");

    for (grp_itr = grp_map.begin(); grp_itr != grp_map.end(); grp_itr++) {
        if (static_cast<vtss_appl_ipmc_lib_key_t>(grp_itr->first.vlan_key) != glb.key) {
            // This group is not affected by the router port change.
            continue;
        }

        // We need to mark all sources and all ASM states as having changed.
        IPMC_LIB_BASE_grp_mark_all_as_changed(grp_itr->second);
        IPMC_LIB_BASE_mesa_update(glb, grp_itr);
    }
}

/******************************************************************************/
// ipmc_lib_base_rx_pdu()
/******************************************************************************/
void ipmc_lib_base_rx_pdu(ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_pdu_t &pdu)
{
    mesa_port_list_t dst_port_mask;
    bool             is_leave_without_group;

    if (pdu.type == IPMC_LIB_PDU_TYPE_QUERY) {
        IPMC_LIB_BASE_rx_query(vlan_state, pdu);

        if (!IPMC_LIB_BASE_rx_query_forward_decision_get(vlan_state, pdu, dst_port_mask)) {
            // Discard it
            return;
        }
    } else {
        // Fetch the profile from two different locations depending on IPMC/MVR,
        // because IPMC has one per port (shared amongst VLANs) and MVR has one
        // per VLAN.
        is_leave_without_group = !IPMC_LIB_BASE_rx_report(vlan_state, pdu, vlan_state.vlan_key.is_mvr ? vlan_state.conf.channel_profile : vlan_state.global->port_conf[pdu.rx_info.port_no].profile_key);

        if (!IPMC_LIB_BASE_rx_report_forward_decision_get(vlan_state, dst_port_mask, pdu.report.is_leave, is_leave_without_group)) {
            // Discard it
            return;
        }
    }

    // Still here?
    // Forward the frame to dst_port_mask
    T_IG_PORT(IPMC_LIB_TRACE_GRP_TX, pdu.rx_info.port_no, "%s: Forwarding %s to %s", vlan_state.vlan_key, ipmc_lib_pdu_type_to_str(pdu.type), dst_port_mask);
    (void)ipmc_lib_pdu_tx(pdu.frm, pdu.rx_info.length, dst_port_mask, vlan_state.vlan_key.is_mvr && !vlan_state.conf.tx_tagged, pdu.rx_info.port_no, vlan_state.vlan_key.vid, vlan_state.conf.pcp, pdu.rx_info.tag.dei);
}

/******************************************************************************/
// ipmc_lib_base_router_port_mask_get()
/******************************************************************************/
mesa_port_list_t ipmc_lib_base_router_port_mask_get(ipmc_lib_global_state_t &glb)
{
    mesa_port_list_t port_list;

    port_list = glb.dynamic_router_ports | glb.static_router_ports;

    return port_list;
}

/******************************************************************************/
// ipmc_lib_base_port_down()
/******************************************************************************/
void ipmc_lib_base_port_down(ipmc_lib_grp_map_t &grp_map, mesa_port_no_t port_no)
{
    ipmc_lib_grp_itr_t grp_itr, grp_itr_next;

    T_IG_PORT(IPMC_LIB_TRACE_GRP_BASE, port_no, "Port went down");

    grp_itr = grp_map.begin();
    while (grp_itr != grp_map.end()) {
        // We might delete this group as part of the following, so keep a ptr
        // to the next group
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        // Inactivate the group on this port. Note that removing it may cause
        // the entire group to be removed, hence the grp_itr_next stuff.
        IPMC_LIB_BASE_grp_port_remove(grp_itr, port_no);

        grp_itr = grp_itr_next;
    }
}

/******************************************************************************/
// ipmc_lib_base_querier_state_update()
/******************************************************************************/
void ipmc_lib_base_querier_state_update(ipmc_lib_vlan_state_t &vlan_state)
{
    uint32_t new_expiry;

    if (vlan_state.status.oper_state != VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE) {
        // Reset
        vlan_state.status.querier_state = VTSS_APPL_IPMC_LIB_QUERIER_STATE_DISABLED;
    }

    switch (vlan_state.status.querier_state) {
    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_DISABLED:
        // This is a transient state used to signal that querier went from
        // disabled to enabled or vice versa.
        vlan_state.internal_state.proxy_query_timeout = 0;
        vlan_state.internal_state.cur_rv              = vlan_state.conf.rv;
        vlan_state.internal_state.cur_qi              = vlan_state.conf.qi;
        vlan_state.internal_state.cur_qri             = vlan_state.conf.qri;
        vlan_state.internal_state.cur_lmqi            = vlan_state.conf.lmqi;

        vlan_state.status.active_querier_address.all_zeros_set();
        vlan_state.status.querier_uptime = 0;

        if (vlan_state.conf.querier_enable) {
            ipmc_lib_pdu_our_ip_get(vlan_state, vlan_state.status.active_querier_address);
            vlan_state.status.querier_state                  = VTSS_APPL_IPMC_LIB_QUERIER_STATE_INIT;
            vlan_state.status.query_interval_left            = 0; // Force a query on the next tick.
            vlan_state.status.other_querier_expiry_time      = 0;
            vlan_state.internal_state.startup_query_cnt_left = vlan_state.conf.rv;
        } else {
            vlan_state.status.querier_state                  = VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE;
            vlan_state.status.query_interval_left            = vlan_state.conf.qi;
            vlan_state.status.other_querier_expiry_time      = IPMC_LIB_BASE_other_querier_present_timeout_get(vlan_state);
            vlan_state.internal_state.startup_query_cnt_left = 0;
        }

        break;

    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_INIT:
        if (!vlan_state.conf.querier_enable) {
            T_EG(IPMC_LIB_TRACE_GRP_QUERIER, "%s: How can querier be disabled when querier state is INIT?", vlan_state.vlan_key);
            break;
        }

        vlan_state.internal_state.cur_rv   = vlan_state.conf.rv;
        vlan_state.internal_state.cur_qi   = vlan_state.conf.qi;
        vlan_state.internal_state.cur_qri  = vlan_state.conf.qri;
        vlan_state.internal_state.cur_lmqi = vlan_state.conf.lmqi;

        if (vlan_state.internal_state.startup_query_cnt_left > vlan_state.conf.rv) {
            // It appears that the RV was changed. Modify our startup count.
            vlan_state.internal_state.startup_query_cnt_left = vlan_state.conf.rv;
        }

        if (vlan_state.status.query_interval_left > vlan_state.conf.qi / 4) {
            // It appears that the QI has changed. Modify our interval
            vlan_state.status.query_interval_left = vlan_state.conf.qi / 4;
        }

        break;

    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE:
        if (vlan_state.status.other_querier_expiry_time) {
            // We are currently not querier. Check if there's something we
            // should change.
            new_expiry = IPMC_LIB_BASE_other_querier_present_timeout_get(vlan_state);
            if (vlan_state.status.other_querier_expiry_time > new_expiry) {
                // It seems that RV, QI or cur_qri has changed. Modify new
                // expiry to new value.
                vlan_state.status.other_querier_expiry_time = new_expiry;
            }

            if (vlan_state.vlan_key.is_ipv4 && vlan_state.conf.querier_enable) {
                // Check if our querier address is smaller than the current
                // querier's
                if (vlan_state.status.active_querier_address.is_zero()) {
                    T_EG(IPMC_LIB_TRACE_GRP_QUERIER, "%s: How can active querier address be 0.0.0.0 when we are in IDLE state and querier is enabled?", vlan_state.vlan_key);
                    break;
                }

                if (!vlan_state.conf.querier_address.is_zero()) {
                    if (vlan_state.conf.querier_address < vlan_state.status.active_querier_address) {
                        // It seems our querier address has changed and that the
                        // new one is smaller than the active querier's. Go
                        // become active again.
                        T_IG(IPMC_LIB_TRACE_GRP_QUERIER, "%s: Changing querier state from Idle to Init", vlan_state.vlan_key);
                        vlan_state.internal_state.cur_rv                 = vlan_state.conf.rv;
                        vlan_state.internal_state.cur_qi                 = vlan_state.conf.qi;
                        vlan_state.internal_state.cur_qri                = vlan_state.conf.qri;
                        vlan_state.internal_state.cur_lmqi               = vlan_state.conf.lmqi;
                        vlan_state.internal_state.startup_query_cnt_left = vlan_state.conf.rv;
                        ipmc_lib_pdu_our_ip_get(vlan_state, vlan_state.status.active_querier_address);
                        vlan_state.status.querier_state                  = VTSS_APPL_IPMC_LIB_QUERIER_STATE_INIT;
                        vlan_state.status.query_interval_left            = 0; // Force a query on the next tick.
                        vlan_state.status.other_querier_expiry_time      = 0;
                    }
                }
            }
        }

        break;

    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE:
        // Could be a new querier address should take effect.
        ipmc_lib_pdu_our_ip_get(vlan_state, vlan_state.status.active_querier_address);

        vlan_state.internal_state.cur_rv      = vlan_state.conf.rv;
        vlan_state.internal_state.cur_qi      = vlan_state.conf.qi;
        vlan_state.internal_state.cur_qri     = vlan_state.conf.qri;
        vlan_state.internal_state.cur_lmqi    = vlan_state.conf.lmqi;

        if (vlan_state.status.query_interval_left > vlan_state.conf.qi) {
            // Seems that QI has changed.
            vlan_state.status.query_interval_left = vlan_state.conf.qi;
        }

        break;
    }
}

/******************************************************************************/
// ipmc_lib_base_vlan_state_init()
/******************************************************************************/
void ipmc_lib_base_vlan_state_init(ipmc_lib_vlan_state_t &vlan_state, bool clear_statistics)
{
    vtss_clear(vlan_state.status);
    vtss_clear(vlan_state.internal_state);

    if (clear_statistics) {
        vtss_clear(vlan_state.statistics);
    }

    // The following fields are updated on the fly upon calls to
    // vtss_appl_ipmc_lib_vlan_status_get(), so no need to initialize them
    // further in this function.
    //  - vlan_state.status.older_version_host_present_timeout_old
    //  - vlan_state.status.older_version_host_present_timeout_gen
    IPMC_LIB_BASE_host_compat_update(vlan_state);

    vlan_state.status.querier_compat                 = vlan_state.conf.compatibility == VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO ? VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM : vlan_state.conf.compatibility;
    vlan_state.status.active_querier_address.is_ipv4 = vlan_state.vlan_key.is_ipv4;

    ipmc_lib_base_querier_state_update(vlan_state);
}

/******************************************************************************/
// ipmc_lib_base_router_status_update()
// If invoked with port_no == MESA_PORT_NO_NONE, all ports will get changed.
/******************************************************************************/
void ipmc_lib_base_router_status_update(ipmc_lib_global_state_t &glb, mesa_port_no_t port_no, bool dynamic, bool add)
{
    mesa_port_list_t &port_list  = dynamic ? glb.dynamic_router_ports : glb.static_router_ports;
    mesa_port_list_t &other_list = dynamic ? glb.static_router_ports  : glb.dynamic_router_ports;
    mesa_port_no_t   min_port_no, max_port_no, port_no_itr;
    bool             update_chip;

    T_IG(IPMC_LIB_TRACE_GRP_BASE, "port_no = %u, dynamic = %d, add = %d", port_no, dynamic, add);

    if (port_no == MESA_PORT_NO_NONE) {
        min_port_no = 0;
        max_port_no = IPMC_LIB_port_cnt - 1;
    } else {
        min_port_no = port_no;
        max_port_no = port_no;
    }

    update_chip = false;
    for (port_no_itr = min_port_no; port_no_itr <= max_port_no; port_no_itr++) {
        if (port_list[port_no_itr] == add) {
            // Either clearing an already cleared bit or setting an already set.
            // Nothing to do.
            continue;
        }

        port_list[port_no_itr] = add;

        if (dynamic && !add) {
            // Clear the dynamic router port timeout for this port.
            glb.port_status[port_no_itr].dynamic_router_timeout = 0;
        }

        vtss_appl_ipmc_lib_router_status_t old_status  = glb.port_status[port_no_itr].router_status;
        vtss_appl_ipmc_lib_router_status_t &new_status = glb.port_status[port_no_itr].router_status;

        // Update port status.
        if (port_list[port_no_itr] && other_list[port_no_itr]) {
            new_status = VTSS_APPL_IPMC_LIB_ROUTER_STATUS_BOTH;
        } else if (port_list[port_no_itr]) {
            new_status = dynamic ? VTSS_APPL_IPMC_LIB_ROUTER_STATUS_DYNAMIC : VTSS_APPL_IPMC_LIB_ROUTER_STATUS_STATIC;
        } else if (other_list[port_no_itr]) {
            new_status = dynamic ? VTSS_APPL_IPMC_LIB_ROUTER_STATUS_STATIC : VTSS_APPL_IPMC_LIB_ROUTER_STATUS_DYNAMIC;
        } else {
            new_status = VTSS_APPL_IPMC_LIB_ROUTER_STATUS_NONE;
        }

        if (!glb.conf.admin_active) {
            // We are just preparing the status, so that it can be used whenever
            // we receive a new report.
            continue;
        }

        // We are currently active, so all <G> and <G, S> entries need to be
        // updated to reflect the new router port state.
        if (new_status != old_status && (new_status == VTSS_APPL_IPMC_LIB_ROUTER_STATUS_NONE || old_status == VTSS_APPL_IPMC_LIB_ROUTER_STATUS_NONE)) {
            // All IGMP or MLD groups must change now, because a port has
            // changed from being a router port to being a non-router port or
            // vice versa.
            update_chip = true;
        }
    }

    if (update_chip) {
        IPMC_LIB_BASE_router_status_changed(glb);
    }

    // Also set the unregistered flood mask
    ipmc_lib_base_unregistered_flooding_update(glb);
}

/******************************************************************************/
// ipmc_lib_base_stp_forwarding_set()
// This function is invoked whenever STP sets port_no to forwarding.
//
// The caller has ensured that we are active.
/******************************************************************************/
void ipmc_lib_base_stp_forwarding_set(ipmc_lib_vlan_state_t &vlan_state, mesa_port_no_t port_no)
{
    if (!vlan_state.global->conf.proxy_enable) {
        // We only react if we are a proxy.
        return;
    }

    if (vlan_state.global->port_status[port_no].router_status != VTSS_APPL_IPMC_LIB_ROUTER_STATUS_NONE) {
        // Don't send queries to ports configured or discovered as router
        // ports.
        return;
    }

    if (vlan_state.status.querier_state != VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE &&
        vlan_state.status.querier_state != VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE) {
        return;
    }

    // Send a general query to port_no
    ipmc_lib_pdu_tx_general_query(vlan_state, port_no);
}

/******************************************************************************/
// ipmc_lib_base_port_profile_changed()
/******************************************************************************/
void ipmc_lib_base_port_profile_changed(ipmc_lib_global_state_t &glb, mesa_port_no_t port_no)
{
#ifdef VTSS_SW_OPTION_SMB_IPMC
    ipmc_lib_profile_match_t         profile_match = {};
    vtss_appl_ipmc_lib_profile_key_t &profile_key = glb.port_conf[port_no].profile_key;
    ipmc_lib_grp_map_t               &grp_map = glb.lists->grp_map;
    ipmc_lib_grp_itr_t               grp_itr, grp_itr_next;

    profile_match.profile_key = profile_key;

    // Go through all groups on this <MVR/IPMC, IGMP/MLD> key and check that
    // they adhere to the new profile on port_no. Delete those that don't.
    grp_itr = grp_map.begin();
    while (grp_itr != grp_map.end()) {
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        if (static_cast<vtss_appl_ipmc_lib_key_t>(grp_itr->first.vlan_key) != glb.key) {
            // Group not belonging to this global state. Go on
            goto next;
        }

        if (!grp_itr->second.active_ports[port_no]) {
            // Group not active on this port. Go on.
            goto next;
        }

        profile_match.dst = grp_itr->first.grp_addr;

        if (!ipmc_lib_profile_permit(*grp_itr->second.vlan_state, profile_match, false /* don't log if we run into an entry with logging enabled */)) {
            // This group is no longer permitted. Remove it from the group.
            // Note that removing it may cause the entire group to be removed,
            // hence the grp_itr_next stuff.
            IPMC_LIB_BASE_grp_port_remove(grp_itr, port_no);
        }

next:
        grp_itr = grp_itr_next;
    }
#endif
}

/******************************************************************************/
// ipmc_lib_base_vlan_profile_changed()
/******************************************************************************/
void ipmc_lib_base_vlan_profile_changed(ipmc_lib_vlan_state_t &vlan_state)
{
#ifdef VTSS_SW_OPTION_SMB_IPMC
    ipmc_lib_profile_match_t         profile_match = {};
    vtss_appl_ipmc_lib_profile_key_t &profile_key = vlan_state.conf.channel_profile;
    ipmc_lib_grp_map_t               &grp_map = vlan_state.global->lists->grp_map;
    ipmc_lib_grp_itr_t               grp_itr, grp_itr_next;

    profile_match.profile_key = profile_key;

    // Go through all groups on this VLAN and check that they adhere to the new
    // profile. Delete those that don't.
    grp_itr = grp_map.begin();
    while (grp_itr != grp_map.end()) {
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        if (grp_itr->second.vlan_state != &vlan_state) {
            goto next;
        }

        profile_match.dst = grp_itr->first.grp_addr;

        if (ipmc_lib_profile_permit(vlan_state, profile_match, false /* don't log if we run into an entry with logging enabled */)) {
            goto next;
        }

        // This group matched a deny rule. Delete it.
        IPMC_LIB_BASE_grp_itr_erase(grp_itr);

next:
        grp_itr = grp_itr_next;
    }
#endif
}

/******************************************************************************/
// ipmc_lib_base_vlan_compatible_mode_changed()
// Only MVR has a concept of VLAN mode, where dynamic means that IGMP and MLD
// reports are allowed - even on source ports, and where comptible means that
// they are discarded when received on source ports.
// If we are going from compatible to dynamic, there's nothing to do, but if we
// are going from dynamic to compatible, we must delete all groups learned on
// source ports.
// We are only invoked if the mode changed. The configuration is already
// updated.
/******************************************************************************/
void ipmc_lib_base_vlan_compatible_mode_changed(ipmc_lib_vlan_state_t &vlan_state)
{
    ipmc_lib_grp_map_t &grp_map = vlan_state.global->lists->grp_map;
    ipmc_lib_grp_itr_t grp_itr, grp_itr_next;
    mesa_port_no_t     port_no;

    if (!vlan_state.conf.compatible_mode) {
        // New mode allows IGMP/MLD reports on all ports. Nothing to do.
        return;
    }

    // New mode disallows IGMP/MLD reports on source ports. Remove the groups if
    // we saw some.
    grp_itr = grp_map.begin();
    while (grp_itr != grp_map.end()) {
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        if (grp_itr->second.vlan_state != &vlan_state) {
            goto next;
        }

        for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
            if (!grp_itr->second.active_ports[port_no]) {
                // Port not active
                continue;
            }

            if (vlan_state.port_conf[port_no].role != VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE) {
                // Port is not a source port. Go on.
                continue;
            }

            // The group is not permitted on this port any longer. Remove it
            // from the group. Note that removing it may cause the entire group
            // to be removed, hence the grp_itr_next stuff.
            IPMC_LIB_BASE_grp_port_remove(grp_itr, port_no);
        }

next:
        grp_itr = grp_itr_next;
    }
}

/******************************************************************************/
// ipmc_lib_base_vlan_port_role_changed()
// See ipmc_lib_base_vlan_compatible_mode_changed() for an explanation.
/******************************************************************************/
void ipmc_lib_base_vlan_port_role_changed(ipmc_lib_vlan_state_t &vlan_state, mesa_port_no_t port_no)
{
    ipmc_lib_grp_map_t &grp_map = vlan_state.global->lists->grp_map;
    ipmc_lib_grp_itr_t grp_itr, grp_itr_next;

    if (vlan_state.port_conf[port_no].role != VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE) {
        // We are only interested in source ports in this function.
        return;
    }

    if (!vlan_state.conf.compatible_mode) {
        // We are only interested in VLANs in compatible mode.
        return;
    }

    // Port is now a source port (was something else before) and the VLAN is
    // configured not to receive IGMP/MLD reports on source ports. Go remove
    // those groups where port_no is active.
    grp_itr = grp_map.begin();
    while (grp_itr != grp_map.end()) {
        grp_itr_next = grp_itr;
        ++grp_itr_next;

        if (grp_itr->second.vlan_state != &vlan_state) {
            goto next;
        }

        if (!grp_itr->second.active_ports[port_no]) {
            // Port not active
            goto next;
        }

        // The group is not permitted on this port any longer. Remove it from
        // the group. Note that removing it may cause the entire group to be
        // removed, hence the grp_itr_next stuff.
        IPMC_LIB_BASE_grp_port_remove(grp_itr, port_no);

next:
        grp_itr = grp_itr_next;
    }
}

/******************************************************************************/
// ipmc_lib_base_aggr_port_update()
// When this one is invoked, it's because port_no is now aggregated with
// another port, in which case all destination masks must include that other
// port, or it's because port_no is no longer aggregated with other ports.
// In either case, we must update all MESA entries to include or exclude those
// other ports.
/******************************************************************************/
void ipmc_lib_base_aggr_port_update(ipmc_lib_grp_map_t &grp_map, mesa_port_no_t port_no)
{
    ipmc_lib_grp_itr_t grp_itr;

    for (grp_itr = grp_map.begin(); grp_itr != grp_map.end(); ++grp_itr) {
        if (!grp_itr->second.active_ports[port_no]) {
            // This group is not affected by the aggregation change.
            continue;
        }

        IPMC_LIB_BASE_grp_mark_all_as_changed(grp_itr->second);
        IPMC_LIB_BASE_mesa_update(*grp_itr->second.vlan_state->global, grp_itr);
    }
}

