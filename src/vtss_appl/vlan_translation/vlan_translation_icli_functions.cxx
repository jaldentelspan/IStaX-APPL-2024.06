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

#include "icli_api.h"
#include "icli_porting_util.h"

#include "vlan_translation_api.h"
#include "vlan_translation_trace.h"
#include "vtss/basics/trace.hxx"
#include "vlan_translation_icli_functions.h"
#include "port_iter.hxx"

// Function mapping a list of VLAN to a VLAN translation for a given group
mesa_rc vlan_trans_icli_map_conf_obsolete(i32 session_id, u32 gid,
                                          icli_unsigned_range_t *vlan_list, u32 tvid, BOOL no)
{
    vtss_appl_vlan_translation_group_mapping_key_t key;
    u16                                            list_index;
    mesa_rc                                        rc;

    // Initialize key
    memset(&key, 0, sizeof(key));
    // Loop through all the VLANs to translate
    for (list_index = 0; list_index < vlan_list->cnt; list_index++) {
        mesa_vid_t vid;
        for (vid = vlan_list->range[list_index].min; vid <= vlan_list->range[list_index].max; vid++) {
            key.gid = gid;
            key.dir = MESA_VLAN_TRANS_DIR_BOTH;
            key.vid = vid;
            if (no) { // Delete
                VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "ICLI delete VLAN Translation mapping";
                rc = vlan_trans_mgmt_group_conf_del(key);
                if ((rc == VT_ERROR_ENTRY_NOT_FOUND) || (rc == VT_ERROR_API_MAP_DEL)) {
                    rc = VTSS_RC_OK;
                }
            } else { // Add
                VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "ICLI add VLAN Translation mapping";
                if (tvid == vid) { // make no sense to map a vlan to itself
                    return VT_ERROR_TVID_SAME_AS_VID;
                }
                rc = vlan_trans_mgmt_group_conf_add(key, tvid);
                if (rc == VT_ERROR_API_MAP_ADD) {
                    rc = VTSS_RC_OK;
                }
            }
            VTSS_RC(rc);
        }
    }
    return VTSS_RC_OK;
}

mesa_rc vlan_trans_icli_map_conf(i32 session_id, u32 gid, BOOL has_both, BOOL has_ingress,
                                 BOOL has_egress, mesa_vid_t vid, mesa_vid_t tvid, BOOL no)
{
    vtss_appl_vlan_translation_group_mapping_key_t key;
    mesa_rc                                        rc = MESA_RC_OK;

    // Initialize key
    memset(&key, 0, sizeof(key));
    key.gid = gid;
    if (has_both || has_ingress || has_egress) {
        if (has_both) {
            key.dir = MESA_VLAN_TRANS_DIR_BOTH;
        } else if (has_ingress) {
            key.dir = MESA_VLAN_TRANS_DIR_INGRESS;
        } else {
            key.dir = MESA_VLAN_TRANS_DIR_EGRESS;
        }
    } else {
        return MESA_RC_OK;
    }
    key.vid = vid;
    if (no) { // Delete
        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "ICLI delete VLAN Translation mapping";
        // Common management interface function that talks to the core module
        rc = vtss_appl_vlan_translation_group_conf_del(key);
        // We hide certain error codes, since they are not really errors
        // from a management interface perspective
        if ((rc == VT_ERROR_ENTRY_NOT_FOUND) || (rc == VT_ERROR_API_MAP_DEL)) {
            rc = MESA_RC_OK;
        }
    } else { // Add
        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "ICLI add VLAN Translation mapping";
        // Common management interface function that talks to the core module
        rc = vtss_appl_vlan_translation_group_conf_set(key, &tvid);
        if (rc == VT_ERROR_API_MAP_ADD) {
            rc = MESA_RC_OK;
        }
    }
    return rc;
}

// Function for mapping ports to groups
mesa_rc vlan_trans_icli_interface_conf(i32 session_id, u8 gid, icli_stack_port_range_t *plist, BOOL no)
{
    vtss_appl_vlan_translation_if_conf_value_t group;
    switch_iter_t                              sit;
    port_iter_t                                pit;
    mesa_rc                                    rc;

    // In fact VLAN translation doesn't support stacking at the moment, this is for future use.
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all the ports in the plist, and set the corresponding bit in the ports BOOL list is the port is part of the user's port list
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (no) {
                VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "ICLI set interface to default VLAN Translation group";
                rc = vlan_trans_mgmt_port_conf_def(pit.iport + 1);
            } else {
                VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "ICLI set interface to VLAN Translation group #" << gid;
                group.gid = gid;
                rc = vlan_trans_mgmt_port_conf_set(pit.iport + 1, group);
            }
            if ((rc == VT_ERROR_API_IF_SET) || (rc == VT_ERROR_API_IF_DEF)) {
                rc = VTSS_RC_OK;
            }
            VTSS_RC(rc);
        }
    }
    return VTSS_RC_OK;
}

//  see vlan_translation_icli_functions.h
BOOL vlan_trans_icli_runtime_groups(u32                session_id,
                                    icli_runtime_ask_t ask,
                                    icli_runtime_t     *runtime)
{
    switch (ask) {
    case ICLI_ASK_BYWORD:
        icli_sprintf(runtime->byword, "<group id : %u-%u>", VT_GID_START, VT_GID_END);
        return TRUE;
    case ICLI_ASK_RANGE:
        runtime->range.type = ICLI_RANGE_TYPE_UNSIGNED;
        runtime->range.u.sr.cnt = 1;
        runtime->range.u.sr.range[0].min = VT_GID_START;
        runtime->range.u.sr.range[0].max = VT_GID_END;
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

