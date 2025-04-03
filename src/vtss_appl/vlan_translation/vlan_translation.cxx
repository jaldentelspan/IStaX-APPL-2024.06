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

#include "vlan_translation.h"
#include "vlan_translation_api.h"
#include "vlan_translation_trace.h"
#include "vtss/basics/trace.hxx"

#if defined(VTSS_SW_OPTION_ICFG)
#include "vlan_translation_icfg.h"
#endif

/*lint -sem( vtss_vlan_trans_crit_data_lock, thread_lock ) */
/*lint -sem( vtss_vlan_trans_crit_data_unlock, thread_unlock ) */

/* VLAN Translation Global Data */
static vlan_trans_global_data_t vt_data;

/* VLAN Translation trace data */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "vlan_trans", "VLAN Translation"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_VT_GRP_DEFAULT] = {
        /*.name      = */"default",
        /*.descr     = */"Default",
        /*.lvl       = */VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_VT_GRP_MGMT] = {
        /*.name      = */"mgmt",
        /*.descr     = */"Management interfaces",
        /*.lvl       = */VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* VLAN Translation crit guard */
struct VtssVlanTranslationCritdGuard {
    VtssVlanTranslationCritdGuard(int line)
    {
        critd_enter(&vt_data.crit, __FUNCTION__, line);
    }

    ~VtssVlanTranslationCritdGuard()
    {
        critd_exit(&vt_data.crit, __FUNCTION__, 0);
    }
};

#define VT_CRIT_SCOPE() VtssVlanTranslationCritdGuard __lock_guard__(__LINE__)

const char *vlan_trans_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VT_ERROR_INVALID_GROUP_ID:
        return "VLAN Translation Error - The provided Group ID was invalid (valid: 1 - Max number of switch ports)";
    case VT_ERROR_INVALID_VLAN_ID:
        return "VLAN Translation Error - The provided VLAN ID was invalid (valid: 1 - 4095)";
    case VT_ERROR_INVALID_TRANSLATION_VLAN_ID:
        return "VLAN Translation Error - The provided Translation VLAN ID was invalid (valid: 1 - 4095)";
    case VT_ERROR_INVALID_PORT_NO:
        return "VLAN Translation Error - The provided port no. was invalid";
    case VT_ERROR_INVALID_IF_TYPE:
        return "VLAN Translation Error - The provided interface type was invalid - expected switch port";
    case VT_ERROR_TVID_SAME_AS_VID:
        return "VLAN Translation Error - The provided Translation VLAN ID is the same as the VLAN ID - makes no sense to translate a VLAN to itself";
    case VT_ERROR_ENTRY_NOT_FOUND:
        return "VLAN Translation Error - The requested entry was not found in the switch";
    case VT_ERROR_ENTRY_CONFLICT:
        return "VLAN Translation Error - The translation entry is conflicting with an existing entry in the switch";
    case VT_ERROR_MAPPING_TABLE_FULL:
        return "VLAN Translation Error - The translation map is full and cannot accept new mappings";
    case VT_ERROR_API_IF_SET:
        return "VLAN Translation Error - API returned an error when setting an interface";
    case VT_ERROR_API_IF_DEF:
        return "VLAN Translation Error - API returned an error when setting an interface to defaults";
    case VT_ERROR_API_MAP_ADD:
        return "VLAN Translation Error - API returned an error when adding a mapping";
    case VT_ERROR_API_MAP_DEL:
        return "VLAN Translation Error - API returned an error when deleting a mapping";
    default:
        return "VLAN Translation unknown error";
    }
}

const char *vlan_trans_dir_txt(mesa_vlan_trans_dir_t dir)
{
    switch (dir) {
    case MESA_VLAN_TRANS_DIR_INGRESS:
        return "Ingress";
    case MESA_VLAN_TRANS_DIR_EGRESS:
        return "Egress";
    case MESA_VLAN_TRANS_DIR_BOTH:
        return "Both";
    default:
        return "Unknown";
    }
}

/**************************************************************************************/
// Functions for controlling entries in the VLAN Translation application-level database.
/**************************************************************************************/
static void vlan_trans_group_conf_default_set(void)
{
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch clearing all elements from the VLAN Translation map";
    VT_CRIT_SCOPE();
    vt_data.map.clear();
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch cleared all elements from the VLAN Translation map";
}

static void vlan_trans_port_conf_default_set(void)
{
    mesa_port_no_t port;

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch setting all port interfaces to use the default VLAN Translation groups";
    VT_CRIT_SCOPE();
    for (port = 1; port <= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port++) {
        vt_data.ports[port - 1] = port;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch set all port interfaces to use the default VLAN Translation groups";
}

static mesa_rc vlan_trans_group_conf_add(const VTMappingKey key, const mesa_vid_t tvid)
{
    VTMappingKey tmp = {};

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Adding VLAN translation mapping "
                                                 << "to the map with GID: " << key.gid
                                                 << ", direction: " << vlan_trans_dir_txt(key.dir)
                                                 << ", VID: " << key.vid << ", TVID: " << tvid;

    VT_CRIT_SCOPE();
    // Check whether or not the entry is already in the map by searching for its key.
    // If it does not exist, check if there is space for it in the map,
    // otherwise return an error.
    auto it = vt_data.map.find(key);
    if ((it == vt_data.map.end()) && (vt_data.map.size() == VT_MAX_TRANSLATIONS)) {
        return VT_ERROR_MAPPING_TABLE_FULL;
    }
    // Since the direction field of the key contains overlapping values
    // we need to check if the entry key is in any way conflicting with
    // the keys of existing entries. E.g. the key (GID, DIR_BOTH, VID) is
    // conflicting with the key (GID, DIR_INGRESS, VID). In case of a conflict
    // we return an error.
    tmp = key;
    if (key.dir == MESA_VLAN_TRANS_DIR_INGRESS) {
        tmp.dir = MESA_VLAN_TRANS_DIR_BOTH;
        if ((it = vt_data.map.find(tmp)) != vt_data.map.end()) {
            VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Mapping conflict";
            return VT_ERROR_ENTRY_CONFLICT;
        }
    } else if (key.dir == MESA_VLAN_TRANS_DIR_EGRESS) {
        tmp.dir = MESA_VLAN_TRANS_DIR_BOTH;
        tmp.vid = tvid;
        if ((it = vt_data.map.find(tmp)) != vt_data.map.end()) {
            VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Mapping conflict";
            return VT_ERROR_ENTRY_CONFLICT;
        }
    } else {
        tmp.dir = MESA_VLAN_TRANS_DIR_INGRESS;
        if ((it = vt_data.map.find(tmp)) != vt_data.map.end()) {
            VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Mapping conflict";
            return VT_ERROR_ENTRY_CONFLICT;
        }
        tmp.dir = MESA_VLAN_TRANS_DIR_EGRESS;
        tmp.vid = tvid;
        if ((it = vt_data.map.find(tmp)) != vt_data.map.end()) {
            VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Mapping conflict";
            return VT_ERROR_ENTRY_CONFLICT;
        }
    }
    // Now that we are safe of "conflicting keys" we check whether the entry
    // is already in the map and update it or we insert it.
    // The 'insert()' will only insert an entry if it does not already exist.
    auto ret = vt_data.map.insert(VTMap::value_type(key, tvid));
    if (ret.second == false) {
        // The entry exists already, so we update it
        vt_data.map.set(key, tvid);
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Mapping was already present in the switch and was updated";
    } else {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Mapping was added to the switch";
    }

    return VTSS_RC_OK;
}

static mesa_rc vlan_trans_group_conf_del(const VTMappingKey key)
{

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch deleting VLAN translation mapping "
                                                 << "from the map with GID: " << key.gid
                                                 << ", direction: " << vlan_trans_dir_txt(key.dir)
                                                 << ", VID: " << key.vid;

    VT_CRIT_SCOPE();
    auto del = vt_data.map.erase(key);
    if (del) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Mapping was successfully deleted from "
                                                     << "the map";
    }

    return del == 1 ? VTSS_RC_OK : (mesa_rc)VT_ERROR_ENTRY_NOT_FOUND;
}

static mesa_rc vlan_trans_group_conf_get(const VTMappingKey key, mesa_vid_t *tvid)
{
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch fetching VLAN Translation mapping "
                                                 << "from the map with GID: " << key.gid
                                                 << ", direction: " << vlan_trans_dir_txt(key.dir)
                                                 << ", VID: " << key.vid;

    VT_CRIT_SCOPE();
    auto it = vt_data.map.find(key);
    if (it != vt_data.map.end()) {
        *tvid = it->second;
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Found the requested mapping, TVID: " << *tvid;
    } else {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "The requested mapping is not part of the VLAN Translation map.";
    }
    return it != vt_data.map.end() ? VTSS_RC_OK : (mesa_rc)VT_ERROR_ENTRY_NOT_FOUND;
}

static mesa_rc vlan_trans_group_conf_itr(const VTMappingKey *const in, VTMappingKey *const out, mesa_vid_t *const tvid)
{
    VTMap::const_iterator it;

    VT_CRIT_SCOPE();
    if (in == NULL)  {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch fetching the first VLAN Translation mapping from the map";
        it = vt_data.map.begin();
    } else {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch fetching the VLAN Translation "
                                                     << "mapping after the one with GID: "
                                                     << in->gid << ", direction: "
                                                     << vlan_trans_dir_txt(in->dir)
                                                     << ", VID: " << in->vid;
        it = vt_data.map.greater_than(*in);
    }
    if (it != vt_data.map.end()) {
        *out = it->first;
        *tvid = it->second;
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "The next mapping is GID: " << out->gid
                                                     << ", direction: "
                                                     << vlan_trans_dir_txt(out->dir)
                                                     << ", VID: " << out->vid
                                                     << ", TVID: " << *tvid;
    } else {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, NOISE) << "Iterator cannot provide the next "
                                                     << "mapping - end of map reached";
    }

    return it != vt_data.map.end() ? VTSS_RC_OK : (mesa_rc)VT_ERROR_ENTRY_NOT_FOUND;
}

static void vlan_trans_port_conf_set(const mesa_port_no_t port, const vt_gid_t gid)
{
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch setting port interface "
                                                 << port << " to use VLAN Translation GID: " << gid;
    VT_CRIT_SCOPE();
    vt_data.ports[port - 1] = gid;
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch set the above port interface";
}

static void vlan_trans_port_conf_get(const mesa_port_no_t port, vt_gid_t *const gid)
{
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch fetching configuration of port interface " << port;
    VT_CRIT_SCOPE();
    *gid = vt_data.ports[port - 1];
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch found that the above port interface is using VLAN Translation GID: "
                                                 << *gid;
}

static void vlan_trans_port_conf_def(const mesa_port_no_t port)
{
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch setting port interface "
                                                 << port << " to use the default group";
    VT_CRIT_SCOPE();
    vt_data.ports[port - 1] = port;
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "Switch set the above port interface to defaults";
}

/****************************************************************************/
// VLAN Translation APIs for manipulating translations.
// Can be called by either management interfaces of other application modules.
/****************************************************************************/
mesa_rc vlan_trans_mgmt_group_conf_add(const vtss_appl_vlan_translation_group_mapping_key_t key,
                                       const mesa_vid_t tvid)
{
    VTMappingKey map_key;
    mesa_rc      rc = VTSS_RC_OK;
    mesa_vlan_trans_grp2vlan_conf_t conf;

    if (VT_VALID_GROUP_ID_CHECK(key.gid) == FALSE) {
        return VT_ERROR_INVALID_GROUP_ID;
    }
    if (VT_VALID_VLAN_ID_CHECK(key.vid) == FALSE) {
        return VT_ERROR_INVALID_VLAN_ID;
    }
    if (VT_VALID_VLAN_ID_CHECK(tvid) == FALSE) {
        return VT_ERROR_INVALID_TRANSLATION_VLAN_ID;
    }

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API adding/updating VLAN Translation"
                                                 << " mapping with GID: " << key.gid
                                                 << ", direction: " << vlan_trans_dir_txt(key.dir)
                                                 << ", VID: " << key.vid << ", TVID: " << tvid;
    map_key.gid = key.gid;
    map_key.dir = key.dir;
    map_key.vid = key.vid;
    if ((rc = vlan_trans_group_conf_add(map_key, tvid)) != VTSS_RC_OK) {
        return rc;
    }
    // We fill the data container that will be passed down to the API
    conf.group_id = key.gid;
    conf.dir = key.dir;
    // The API has the perspective that 'vid' is always the "external"
    // VLAN and 'tvid' is always the "internal" VLAN, regardless of the
    // translation direction. The APPL on the other hand considers
    // the direction in its perspective, i.e. 'vid' can be either the
    // "external" VLAN for the Ingress/Both directions or the "internal"
    // for the Egress direction. Therefore we must align the two perspectives
    // every time the APPL interacts with the API.
    if (key.dir != MESA_VLAN_TRANS_DIR_EGRESS) {
        conf.vid = key.vid;
        conf.trans_vid = tvid;
    } else {
        conf.vid = tvid;
        conf.trans_vid = key.vid;
    }
    if ((rc = mesa_vlan_trans_group_add(NULL, &conf)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "API function failed while adding "
                                                     << "the above mapping";
        // The entry in the application database must remain there
        return VT_ERROR_API_MAP_ADD;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API added/updated the above mapping";
    return VTSS_RC_OK;
}

mesa_rc vlan_trans_mgmt_group_conf_del(const vtss_appl_vlan_translation_group_mapping_key_t key)
{
    VTMappingKey                    map_key;
    mesa_rc                         rc = VTSS_RC_OK;
    mesa_vlan_trans_grp2vlan_conf_t conf;

    if (VT_VALID_GROUP_ID_CHECK(key.gid) == FALSE) {
        return VT_ERROR_INVALID_GROUP_ID;
    }
    if (VT_VALID_VLAN_ID_CHECK(key.vid) == FALSE) {
        return VT_ERROR_INVALID_VLAN_ID;
    }

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API deleting VLAN Translation "
                                                 << "mapping with GID: "
                                                 << key.gid
                                                 << ", direction: " << vlan_trans_dir_txt(key.dir)
                                                 << ", VID: " << key.vid;
    map_key.gid = key.gid;
    map_key.dir = key.dir;
    map_key.vid = key.vid;
    if ((rc = vlan_trans_group_conf_del(map_key)) != VTSS_RC_OK) {
        return rc;
    }
    // We fill the data container that will be passed down to the API
    conf.group_id = key.gid;
    conf.dir = key.dir;
    // The API has the perspective that 'vid' is always the "external"
    // VLAN and 'tvid' is always the "internal" VLAN, regardless of the
    // translation direction. The APPL on the other hand considers
    // the direction in its perspective, i.e. 'vid' can be either the
    // "external" VLAN for the Ingress/Both directions or the "internal"
    // for the Egress direction. Therefore we must align the two perspectives
    // every time the APPL interacts with the API.
    if (key.dir != MESA_VLAN_TRANS_DIR_EGRESS) {
        conf.vid = key.vid;
        // The 'trans_vid' will not be used for anything during the lookup
        // for non-egress directions but we still set it to an invalid value.
        conf.trans_vid = 4096;
    } else {
        // The 'vid' is used by the API (even though the lookup is in the
        // egress direction) to return the first entry in its database if
        // its value is set to '0'. To avoid such a thing, we set it to an
        // invalid value.
        conf.vid = 4096;
        conf.trans_vid = key.vid;
    }
    if ((rc = mesa_vlan_trans_group_get(NULL, &conf, &conf)) == VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "API deleting VT mapping with GID: "
                                                     << conf.group_id
                                                     << ", direction: "
                                                     << vlan_trans_dir_txt(key.dir)
                                                     << ", VID: " << conf.vid
                                                     << ", TVID: " << conf.trans_vid;
        if ((rc = mesa_vlan_trans_group_del(NULL, &conf)) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "API function failed while "
                                                         << "deleting the above mapping";
            // The entry in the application database must remain deleted
            return VT_ERROR_API_MAP_DEL;
        }
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API deleted the above mapping";
    return VTSS_RC_OK;
}

mesa_rc vlan_trans_mgmt_group_conf_get(const vtss_appl_vlan_translation_group_mapping_key_t key,
                                       mesa_vid_t *tvid)
{
    VTMappingKey map_key;
    mesa_rc      rc = VTSS_RC_OK;

    if (VT_VALID_GROUP_ID_CHECK(key.gid) == FALSE) {
        return VT_ERROR_INVALID_GROUP_ID;
    }
    if (VT_VALID_VLAN_ID_CHECK(key.vid) == FALSE) {
        return VT_ERROR_INVALID_VLAN_ID;
    }

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API fetching VLAN Translation "
                                                 << "mapping with GID: " << key.gid
                                                 << ", direction: " << vlan_trans_dir_txt(key.dir)
                                                 << ", VID: " << key.vid;
    map_key.gid = key.gid;
    map_key.dir = key.dir;
    map_key.vid = key.vid;
    if ((rc = vlan_trans_group_conf_get(map_key, tvid)) != VTSS_RC_OK) {
        return rc;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API fetched the above mapping";
    return VTSS_RC_OK;
}

mesa_rc vlan_trans_mgmt_group_conf_itr(const vtss_appl_vlan_translation_group_mapping_key_t *const in,
                                       vtss_appl_vlan_translation_group_mapping_key_t *const out,
                                       mesa_vid_t *const tvid)
{
    VTMappingKey *key_in = NULL, key_temp = {}, key_out = {};
    mesa_rc      rc = VTSS_RC_OK;

    if (in == NULL)  {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API fetching the first VLAN "
                                                     << "Translation mapping from the map";
    } else {
        if (VT_VALID_GROUP_ID_CHECK(in->gid) == FALSE) {
            return VT_ERROR_INVALID_GROUP_ID;
        }
        if (VT_VALID_VLAN_ID_CHECK(in->vid) == FALSE) {
            return VT_ERROR_INVALID_VLAN_ID;
        }
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API fetching the VLAN Translation"
                                                     << " mapping after the one with GID: "
                                                     << in->gid << ", direction: "
                                                     << vlan_trans_dir_txt(in->dir)
                                                     << ", VID: " << in->vid;
        key_temp.gid = in->gid;
        key_temp.dir = in->dir;
        key_temp.vid = in->vid;
        key_in = &key_temp;
    }

    if ((rc = vlan_trans_group_conf_itr(key_in, &key_out, tvid)) != VTSS_RC_OK) {
        return rc;
    } else {
        out->gid = key_out.gid;
        out->dir = key_out.dir;
        out->vid = key_out.vid;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API fetched the above mapping";
    return VTSS_RC_OK;
}

mesa_rc vlan_trans_mgmt_port_conf_set(const mesa_port_no_t port, const vtss_appl_vlan_translation_if_conf_value_t group)
{
    mesa_vlan_trans_port2grp_conf_t conf = { 0 };
    mesa_rc                         rc = VTSS_RC_OK;

    if (VT_VALID_PORT_NO_CHECK(port) == FALSE) {
        return VT_ERROR_INVALID_PORT_NO;
    }
    if (VT_VALID_GROUP_ID_CHECK(group.gid) == FALSE) {
        return VT_ERROR_INVALID_GROUP_ID;
    }

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API setting port interface "
                                                 << port << " to use VLAN Translation GID: " << group.gid;
    vlan_trans_port_conf_set(port, group.gid);
    conf.group_id = group.gid;
    conf.port_list[port - 1] = 1;
    if ((rc = mesa_vlan_trans_group_to_port_set(NULL, &conf)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "API function failed while setting the above port interface "
                                                     << "- configuration remained unchanged";
        // The application database entry must remain updated
        return VT_ERROR_API_IF_SET;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API set the above port interface";
    return VTSS_RC_OK;
}

mesa_rc vlan_trans_mgmt_port_conf_get(const mesa_port_no_t port, vtss_appl_vlan_translation_if_conf_value_t *const group)
{
    vt_gid_t gid;

    if (VT_VALID_PORT_NO_CHECK(port) == FALSE) {
        return VT_ERROR_INVALID_PORT_NO;
    }

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API fetching configuration of port interface " << port;
    vlan_trans_port_conf_get(port, &gid);
    group->gid = (u16) gid;
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API found the above port interface configuration";
    return VTSS_RC_OK;
}

mesa_rc vlan_trans_mgmt_port_conf_def(const mesa_port_no_t port)
{
    mesa_vlan_trans_port2grp_conf_t conf = { 0 };
    mesa_rc                         rc = VTSS_RC_OK;

    if (VT_VALID_PORT_NO_CHECK(port) == FALSE) {
        return VT_ERROR_INVALID_PORT_NO;
    }

    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API setting port interface "
                                                 << port << " to use the default VLAN Translation Group";
    vlan_trans_port_conf_def(port);
    conf.group_id = port;
    conf.port_list[port - 1] = 1;
    if ((rc = mesa_vlan_trans_group_to_port_set(NULL, &conf)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "API function failed while setting the above port interface to defaults"
                                                     << "- configuration remained unchanged";
        // The application database entry must remain updated
        return VT_ERROR_API_IF_DEF;
    }
    VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "MGMT API set the above port interface to defaults";
    return VTSS_RC_OK;
}

/************************************************/
// Functions for clearing all translation entries.
// Both from the API, and the APPL.
/************************************************/
static void vlan_trans_group_conf_clear()
{
    mesa_vlan_trans_grp2vlan_conf_t conf;
    mesa_rc                         rc = VTSS_RC_OK;

    memset(&conf, 0, sizeof(conf));
    while ((rc = mesa_vlan_trans_group_get_next(NULL, &conf, &conf)) == VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "API deleting VT mapping with GID: "
                                                     << conf.group_id
                                                     << ", direction: "
                                                     << vlan_trans_dir_txt(conf.dir)
                                                     << " and VID: " << conf.vid;
        if ((rc = mesa_vlan_trans_group_del(NULL, &conf)) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "API function failed to delete "
                                                         << "the above mapping";
        }
        memset(&conf, 0, sizeof(conf));
    }
    vlan_trans_group_conf_default_set();
}

static void vlan_trans_port_conf_clear()
{
    mesa_vlan_trans_port2grp_conf_t conf = { 0 };
    mesa_port_no_t                  port;
    mesa_rc                         rc = VTSS_RC_OK;

    vlan_trans_port_conf_default_set();
    for (port = 1; port <= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port++) {
        conf = { 0 };
        conf.group_id = port;
        conf.port_list[port - 1] = 1;
        if ((rc = mesa_vlan_trans_group_to_port_set(NULL, &conf)) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "API function failed while setting a port interface to defaults";
            // Should it stop here or keep deleting entries?
        }
    }
}

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
#ifdef __cplusplus
extern "C" void vtss_vlan_trans_mib_init(void);
#endif
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
#ifdef __cplusplus
extern "C" void vtss_appl_vlan_trans_json_init(void);
#endif
#endif

extern "C" int vlan_translation_icli_cmd_register();

mesa_rc vlan_trans_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc     rc = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        /* PPP1004: Initialize local data structures; Create and initialize
           OS objects(threads, mutexes, event flags etc). Resume threads if they should
           be running. This command is executed before scheduler is started, so don't
           perform any blocking operation such as critd_enter() */
        /* Initialize and register trace ressources */
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "vlan_trans_init() : INIT_CMD_INIT\n";
        /* Initializing the local data structures */
        critd_init(&vt_data.crit, "vlan_translation", VTSS_MODULE_ID_VLAN_TRANSLATION, CRITD_TYPE_MUTEX);

#if defined(VTSS_SW_OPTION_ICFG)
        // Initialize ICLI "show running" configuration
        VTSS_RC(vlan_translation_icfg_init());
#endif

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        vtss_vlan_trans_mib_init();
#endif
#if defined(VTSS_SW_OPTION_JSON_RPC)
        vtss_appl_vlan_trans_json_init();
#endif
        vlan_translation_icli_cmd_register();
        break;

    case INIT_CMD_START:
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "vlan_trans_init() : INIT_CMD_START\n";
        /* PPP1004 : Initialize the things that might perform blocking operations as
           scheduler has been started. Also, register callbacks from other modules */
        break;

    case INIT_CMD_CONF_DEF:
        /* As stacking is not supported by VLAN Translation module, it is OK to handle one of the
           VTSS_ISID_LOCAL, VTSS_ISID_GLOBAL and VTSS_ISID_LEGAL */
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "vlan_trans_init() : INIT_CMD_CONF_DEF\n";
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            vlan_trans_group_conf_clear();
            vlan_trans_port_conf_clear();
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        VTSS_TRACE(VTSS_TRACE_VT_GRP_DEFAULT, DEBUG) << "vlan_trans_init() : INIT_CMD_ICFG_LOADING_PRE\n";
        /* Read switch configuration */
        vlan_trans_group_conf_clear();
        vlan_trans_port_conf_clear();
        break;

    default:
        break;
    }
    return rc;
}
