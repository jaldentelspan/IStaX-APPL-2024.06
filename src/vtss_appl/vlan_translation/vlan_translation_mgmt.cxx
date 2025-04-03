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

#include "main.h"
#include "critd_api.h"
#include "vlan_translation_api.h"
#include "misc_api.h"
#include "vlan_translation_trace.h"
#include "vtss/basics/trace.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"

const vtss_enum_descriptor_t mesa_vlan_trans_dir_txt[] = {
    {MESA_VLAN_TRANS_DIR_BOTH, "both"},
    {MESA_VLAN_TRANS_DIR_INGRESS, "ingress"},
    {MESA_VLAN_TRANS_DIR_EGRESS, "egress"},
    {0, 0}
};

/****************************************************************************
 * Global configuration
 ****************************************************************************/

mesa_rc vtss_appl_vlan_translation_global_capabilities_get(vtss_appl_vlan_translation_capabilities_t *c)
{
    c->max_number_of_translations = VT_MAX_TRANSLATIONS;

    return MESA_RC_OK;
}

mesa_rc vtss_appl_vlan_translation_group_conf_set(vtss_appl_vlan_translation_group_mapping_key_t mapping,
                                                  const mesa_vid_t *const tvid)
{
    mesa_rc rc;

    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Enter Add/Set VLAN Translation mapping "
                                              << "with GID: " << mapping.gid
                                              << ", direction: " << vlan_trans_dir_txt(mapping.dir)
                                              << ", VID: " << mapping.vid << ", TVID: " << *tvid;
    // We reject rules that translate a VLAN to itself, since it make no sense.
    if (*tvid == mapping.vid) {
        return VT_ERROR_TVID_SAME_AS_VID;
    }
    // Calling the core of the application module
    if ((rc = vlan_trans_mgmt_group_conf_add(mapping, *tvid)) != MESA_RC_OK) {
        return rc;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Exit Add/Set VLAN Translation mapping";
    return MESA_RC_OK;
}

mesa_rc vtss_appl_vlan_translation_group_conf_del(vtss_appl_vlan_translation_group_mapping_key_t mapping)
{
    mesa_rc rc;

    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Enter Delete VLAN Translation mapping "
                                              << "with GID: " << mapping.gid
                                              << ", direction: " << vlan_trans_dir_txt(mapping.dir)
                                              << ", VID: " << mapping.vid;
    if ((rc = vlan_trans_mgmt_group_conf_del(mapping)) != MESA_RC_OK) {
        return rc;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Exit Delete VLAN Translation mapping";
    return MESA_RC_OK;
}

mesa_rc vtss_appl_vlan_translation_group_conf_get(vtss_appl_vlan_translation_group_mapping_key_t mapping,
                                                  mesa_vid_t *tvid)
{
    mesa_rc rc;

    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Enter Get VLAN Translation mapping "
                                              << "with GID: " << mapping.gid
                                              << ", direction: " << vlan_trans_dir_txt(mapping.dir)
                                              << ", VID: " << mapping.vid;
    if ((rc = vlan_trans_mgmt_group_conf_get(mapping, tvid)) != VTSS_RC_OK) {
        return rc;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Exit Get VLAN Translation mapping";
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vlan_translation_group_conf_itr(const vtss_appl_vlan_translation_group_mapping_key_t *const prev_mapping,
                                                  vtss_appl_vlan_translation_group_mapping_key_t *const next_mapping)
{
    mesa_vid_t tvid;
    mesa_rc    rc;

    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Enter VLAN Translation Table iterator";
    if ((rc = vlan_trans_mgmt_group_conf_itr(prev_mapping, next_mapping, &tvid)) != VTSS_RC_OK) {
        return rc;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Exit VLAN Translation Table iterator";
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vlan_translation_group_conf_def(vtss_appl_vlan_translation_group_mapping_key_t *mapping, mesa_vid_t *tvid)
{
    mapping->gid = 1;
    mapping->vid = 1;
    *tvid = 1;
    return VTSS_RC_OK;
}

/****************************************************************************
 * Interface configuration
 ****************************************************************************/

mesa_rc vtss_appl_vlan_translation_if_conf_set(vtss_ifindex_t ifindex,
                                               const vtss_appl_vlan_translation_if_conf_value_t *const group)
{
    vtss_ifindex_elm_t e;
    mesa_rc            rc;

    VTSS_RC(vtss_ifindex_decompose(ifindex, &e));
    if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Interface " << ifindex << " is not a port interface";
        return VT_ERROR_INVALID_IF_TYPE;
    }

    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Enter Set Interface configuration";
    if ((rc = vlan_trans_mgmt_port_conf_set(e.ordinal + 1, *group)) != VTSS_RC_OK) {
        if (rc == VT_ERROR_API_IF_SET) {
            return VTSS_RC_OK;
        } else {
            return rc;
        }
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Exit Set Interface configuration";
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vlan_translation_if_conf_get(vtss_ifindex_t ifindex,
                                               vtss_appl_vlan_translation_if_conf_value_t *group)
{
    vtss_ifindex_elm_t              e;
    mesa_rc                         rc;

    VTSS_RC(vtss_ifindex_decompose(ifindex, &e));
    if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Interface " << ifindex << " is not a port interface";
        return VTSS_RC_ERROR;
    }
    if (e.ordinal >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Port interface " << e.ordinal + 1
                                                  << " does not exist in the switch. This device has a total of "
                                                  << fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) << " port interfaces";
        return VTSS_RC_ERROR;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Enter Get Interface configuration for port#" << e.ordinal + 1;
    if ((rc = vlan_trans_mgmt_port_conf_get(e.ordinal + 1, group)) != VTSS_RC_OK) {
        return rc;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "Exit Get Interface configuration";
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_vlan_translation_if_conf_itr(const vtss_ifindex_t *const prev_ifindex,
                                               vtss_ifindex_t *const next_ifindex)
{
    vtss_ifindex_t ifindex_start, ifindex_end;

    if (next_ifindex == NULL) {
        T_DG(VTSS_TRACE_VT_GRP_MGMT, "next_ifindex == NULL");
        return VTSS_RC_ERROR;
    }

    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, 0, &ifindex_start);
    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_count_max() - 1, &ifindex_end);

    if (prev_ifindex) {
        if (*prev_ifindex < ifindex_start) {
            *next_ifindex = ifindex_start;
        } else {
            if (*prev_ifindex < ifindex_end) {
                *next_ifindex = *prev_ifindex + 1;
            } else {
                return VTSS_RC_ERROR;
            }
        }
    } else {
        *next_ifindex = ifindex_start;
    }

    return VTSS_RC_OK;
}
