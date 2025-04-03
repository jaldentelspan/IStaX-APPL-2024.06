/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "icfg_api.h"
#include "vlan_api.h"
#include "misc_api.h"   /* For str_tolower() */
#include "vlan_icfg.h"
#include "vlan_trace.h" /* For T_xxx() */

#ifdef __cplusplus
#include "enum_macros.hxx"
#endif

#undef  VTSS_ALLOC_MODULE_ID
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_VLAN

#ifdef __cplusplus
VTSS_ENUM_INC(vtss_appl_vlan_port_mode_t);
#endif /* #ifdef __cplusplus */

/******************************************************************************/
// VLAN_ICFG_s_custom_etype_print()
/******************************************************************************/
static mesa_rc VLAN_ICFG_s_custom_etype_print(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_etype_t tpid;
    int          conf_changed = 0;

    VTSS_RC(vtss_appl_vlan_s_custom_etype_get(&tpid));

    // If conf has changed from default
    conf_changed = tpid != VTSS_APPL_VLAN_CUSTOM_S_TAG_DEFAULT;
    if (req->all_defaults || conf_changed) {
        VTSS_RC(vtss_icfg_printf(result, "vlan ethertype s-custom-port 0x%x\n", tpid));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICFG_svl_print()
/******************************************************************************/
static mesa_rc VLAN_ICFG_svl_print(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    char       fid_in_use[((VTSS_APPL_VLAN_FID_CNT + 1) + 7) / 8], *vid_buf = NULL;
    u8         vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    BOOL       found = FALSE;
    mesa_vid_t vid, fid;
    mesa_rc    rc;

    if (!VTSS_APPL_VLAN_FID_CNT) {
        return VTSS_RC_OK;
    }

    // The output is per FID, whereas the vtss_appl_vlan_fid_get() is keyed by VID,
    // so it can be quite time-consuming to synthesize the command. Let's give it
    // a try.
    memset(fid_in_use, 0, sizeof(fid_in_use));

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        if ((rc = vtss_appl_vlan_fid_get(vid, &fid)) != VTSS_RC_OK) {
            T_E("Unable to get FID for VLAN %u. rc = %s", vid, error_txt(rc));
            return rc;
        }

        if (fid != 0) {
            VTSS_BF_SET(fid_in_use, fid, 1);
            found = TRUE;
        }
    }

    if (!found) {
        if (req->all_defaults) {
            VTSS_RC(vtss_icfg_printf(result, "no svl fid all\n"));
        }

        return VTSS_RC_OK;
    }

    if ((vid_buf = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Out of memory");
        return VTSS_RC_ERROR;
    }

    // At least one non-default mapping found
    for (fid = 1; fid <= VTSS_APPL_VLAN_FID_CNT; fid++) {
        if (VTSS_BF_GET(fid_in_use, fid)) {
            // Gotta traverse all VIDs once more while building up an array of members.
            // if not already allocated, allocate a buffer for it.

            memset(vids, 0, sizeof(vids));

            for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                mesa_vid_t temp_fid;

                if ((rc = vtss_appl_vlan_fid_get(vid, &temp_fid)) != VTSS_RC_OK) {
                    T_E("Unable to get FID for VLAN %u. rc = %s", vid, error_txt(rc));
                    VTSS_FREE(vid_buf);
                    return rc;
                }

                if (fid == temp_fid) {
                    VTSS_BF_SET(vids, vid, 1);
                }
            }

            if ((rc = vtss_icfg_printf(result, "svl fid %u vlan %s\n", fid, vlan_mgmt_vid_bitmask_to_txt(vids, vid_buf))) != VTSS_RC_OK) {
                VTSS_FREE(vid_buf);
                return rc;
            }
        }
    }

    VTSS_FREE(vid_buf);

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICFG_global_conf()
/******************************************************************************/
static mesa_rc VLAN_ICFG_global_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    VTSS_RC(VLAN_ICFG_s_custom_etype_print(req, result));
    VTSS_RC(VLAN_ICFG_svl_print(req, result));

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICFG_tag_type_to_txt()
/******************************************************************************/
static const char *VLAN_ICFG_tag_type_to_txt(vtss_appl_vlan_tx_tag_type_t tx_tag_type)
{
    switch (tx_tag_type) {
    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
        return "all except-native";

    case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
        return "all";

    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
        return "none";

    default:
        T_E("Que? (%d)", tx_tag_type);
        return "";
    }
}

/******************************************************************************/
// VLAN_ICFG_port_mode_to_str[]
/******************************************************************************/
static const char *const VLAN_ICFG_port_mode_to_str[3] = {
    [VTSS_APPL_VLAN_PORT_MODE_ACCESS] = "access",
    [VTSS_APPL_VLAN_PORT_MODE_TRUNK]  = "trunk",
    [VTSS_APPL_VLAN_PORT_MODE_HYBRID] = "hybrid"
};

/******************************************************************************/
// VLAN_ICFG_port_mode_native_to_str[]
/******************************************************************************/
static const char *const VLAN_ICFG_port_mode_native_to_str[3] = {
    [VTSS_APPL_VLAN_PORT_MODE_ACCESS] = "",
    [VTSS_APPL_VLAN_PORT_MODE_TRUNK]  = " native",
    [VTSS_APPL_VLAN_PORT_MODE_HYBRID] = " native"
};

/******************************************************************************/
// VLAN_ICFG_port_mode_pvid_print()
/******************************************************************************/
static mesa_rc VLAN_ICFG_port_mode_pvid_print(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result, vtss_appl_vlan_port_mode_t port_mode, vtss_appl_vlan_port_conf_t *conf)
{
    mesa_vid_t pvid;

    switch (port_mode) {
    case VTSS_APPL_VLAN_PORT_MODE_ACCESS:
        pvid = conf->access_pvid;
        break;

    case VTSS_APPL_VLAN_PORT_MODE_TRUNK:
        pvid = conf->trunk_pvid;
        break;

    default:
        // Hybrid:
        pvid = conf->hybrid.pvid;
        break;
    }

    if (req->all_defaults || pvid != VTSS_APPL_VLAN_ID_DEFAULT) {
        VTSS_RC(vtss_icfg_printf(result, " switchport %s%s vlan %u\n", VLAN_ICFG_port_mode_to_str[port_mode], VLAN_ICFG_port_mode_native_to_str[port_mode], pvid));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICFG_port_mode_allowed_vids_print_cmd()
/******************************************************************************/
static mesa_rc VTSS_ICFG_port_mode_allowed_vlan_print(vtss_icfg_query_result_t *result, vtss_appl_vlan_port_mode_t port_mode, char *keyword, u8 *vid_bitmask)
{
    mesa_rc rc;
    char    *buf = NULL;

    if (keyword == NULL) {
        // Caller doesn't have a special keyword to print, so print the #vid_bitmask.
        if ((buf = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
            T_E("Out of memory");
            return VTSS_RC_ERROR;
        }

        (void)vlan_mgmt_vid_bitmask_to_txt(vid_bitmask, buf);
    }

    rc = vtss_icfg_printf(result, " switchport %s allowed vlan %s\n", VLAN_ICFG_port_mode_to_str[port_mode], keyword ? (char *)keyword : buf);

    if (buf) {
        VTSS_FREE(buf);
    }

    return rc;
}

/******************************************************************************/
// VLAN_ICFG_port_mode_allowed_vids_print()
/******************************************************************************/
static mesa_rc VLAN_ICFG_port_mode_allowed_vids_print(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result, vtss_appl_vlan_port_mode_t port_mode, vtss_appl_vlan_port_conf_t *conf, vtss_appl_vlan_port_conf_t *conf_default)
{
    u8   *vid_bitmask = port_mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? conf->trunk_allowed_vids : conf->hybrid_allowed_vids;
    u8   vid_bitmask_all[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    BOOL is_all_zeros, is_all_ones;

    // In the following, we have to use the vlan_mgmt_bitmasks_identical()
    // utility function to get to know whether two VLAN bitmasks are
    // identical or not, because not all bits in the array may be in use.

    if (req->all_defaults) {
        // Here, we always print the whole range, whether it's all VLANs or just partial set.
        // There is one exception, which is when it's empty, in which case we print "none"
        memset(vid_bitmask_all, 0x0, sizeof(vid_bitmask_all));
        is_all_zeros = vlan_mgmt_bitmasks_identical(vid_bitmask, vid_bitmask_all);

        VTSS_RC(VTSS_ICFG_port_mode_allowed_vlan_print(result, port_mode, (char *)(is_all_zeros ? "none" : NULL), vid_bitmask));
    } else {
        if (!vlan_mgmt_bitmasks_identical(vid_bitmask, port_mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? conf_default->trunk_allowed_vids : conf_default->hybrid_allowed_vids)) {
            // The current bitmask is not default. Gotta print a string.
            memset(vid_bitmask_all, 0x0, sizeof(vid_bitmask_all));
            is_all_zeros = vlan_mgmt_bitmasks_identical(vid_bitmask, vid_bitmask_all);

            if (is_all_zeros) {
                // Cannot be all-zeros and all-ones at the same time :)
                is_all_ones = FALSE;
            } else {
                (void)vlan_mgmt_bitmask_all_ones_set(vid_bitmask_all);
                is_all_ones = vlan_mgmt_bitmasks_identical(vid_bitmask, vid_bitmask_all);
            }

            VTSS_RC(VTSS_ICFG_port_mode_allowed_vlan_print(result, port_mode, (char *)(is_all_zeros ? "none" : is_all_ones ? "all" : NULL), vid_bitmask));
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICFG_port_conf()
/******************************************************************************/
static mesa_rc VLAN_ICFG_port_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_vid_t                 vid;
    vtss_isid_t                isid = req->instance_id.port.isid;
    mesa_port_no_t             iport = req->instance_id.port.begin_iport;
    vtss_appl_vlan_port_conf_t conf_default, conf;
    vtss_appl_vlan_port_mode_t port_mode;
    vtss_appl_vlan_entry_t     forbidden_conf;
    u8                         forbidden_vid_mask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    BOOL                       found, conf_changed;
    char                       str[32];

    str[sizeof(str) - 1] = '\0';

    (void)vlan_mgmt_port_conf_default_get(&conf_default);
    VTSS_RC(vlan_mgmt_port_conf_get(isid, iport, &conf, VTSS_APPL_VLAN_USER_STATIC, FALSE));

    // Access, Trunk, and Hybrid PVIDs (access_pvid/trunk_pvid/hybrid.pvid)
    for (port_mode = VTSS_APPL_VLAN_PORT_MODE_ACCESS; port_mode <= VTSS_APPL_VLAN_PORT_MODE_HYBRID; port_mode++) {
        VTSS_RC(VLAN_ICFG_port_mode_pvid_print(req, result, port_mode, &conf));
    }

    // Trunk and Hybrid: Allowed VLANs
    for (port_mode = VTSS_APPL_VLAN_PORT_MODE_TRUNK; port_mode <= VTSS_APPL_VLAN_PORT_MODE_HYBRID; port_mode++) {
        VTSS_RC(VLAN_ICFG_port_mode_allowed_vids_print(req, result, port_mode, &conf, &conf_default));
    }

    // Trunk native VLAN tagging
    conf_changed = conf.trunk_tag_pvid != conf_default.trunk_tag_pvid;
    if (req->all_defaults || conf_changed) {
        VTSS_RC(vtss_icfg_printf(result, " %sswitchport trunk vlan tag native\n", conf.trunk_tag_pvid ? "" : "no "));
    }

    // Hybrid port mode details
    // Acceptable frame type (accept all, accept tagged only, accept untagged only).
    conf_changed = conf.hybrid.frame_type != conf_default.hybrid.frame_type;
    if (req->all_defaults || conf_changed) {
        strncpy(str, vlan_mgmt_frame_type_to_txt(conf.hybrid.frame_type), sizeof(str) - 1);
        VTSS_RC(vtss_icfg_printf(result, " switchport hybrid acceptable-frame-type %s\n", str_tolower(str)));
    }

    // Ingress filtering
    conf_changed = conf.hybrid.ingress_filter != conf_default.hybrid.ingress_filter;
    if (req->all_defaults || conf_changed) {
        VTSS_RC(vtss_icfg_printf(result, " %sswitchport hybrid ingress-filtering\n", conf.hybrid.ingress_filter ? "" : "no "));
    }

    // Tx Tag type (untag all ("none"), untag this ("all except-native"), or tag all ("all");
    conf_changed = conf.hybrid.tx_tag_type != conf_default.hybrid.tx_tag_type;
    if (req->all_defaults || conf_changed) {
        VTSS_RC(vtss_icfg_printf(result, " switchport hybrid egress-tag %s\n", VLAN_ICFG_tag_type_to_txt(conf.hybrid.tx_tag_type)));
    }

    // Port Type (Unaware, C-tag-aware, S-tag aware, Custom-S-tag aware)
    conf_changed = conf.hybrid.port_type != conf_default.hybrid.port_type;
    if (req->all_defaults || conf_changed) {
        // vlan_mgmt_port_type_to_txt() returns a pointer to a fixed string, so we should not lowercase that copy.
        strncpy(str, vlan_mgmt_port_type_to_txt(conf.hybrid.port_type), sizeof(str) - 1);
        VTSS_RC(vtss_icfg_printf(result, " switchport hybrid port-type %s\n", str_tolower(str)));
    }

    // Port Mode. It's on purpose it's synthesized here. It's potentially much faster to change port mode after allowed VIDs
    // and other stuff is set-up than before.
    conf_changed = conf.mode != conf_default.mode;
    if (req->all_defaults || conf_changed) {
        VTSS_RC(vtss_icfg_printf(result, " switchport mode %s\n", VLAN_ICFG_port_mode_to_str[conf.mode]));
    }

    // Forbidden VLANs
    vid   = VTSS_VID_NULL;
    found = FALSE;
    memset(forbidden_vid_mask, 0, sizeof(forbidden_vid_mask));

    while (vtss_appl_vlan_get(isid, vid, &forbidden_conf, TRUE, VTSS_APPL_VLAN_USER_FORBIDDEN) == VTSS_RC_OK) {
        vid = forbidden_conf.vid; // Select next entry
        if (forbidden_conf.ports[iport]) {
            found = TRUE;
            VTSS_BF_SET(forbidden_vid_mask, vid, 1);
        }
    }

    if (found) {
        char    *buf;
        mesa_rc rc;

        // Gotta convert the forbidden_vid_mask to a string. This string
        // can potentially be quite long, so we have to dynamically allocate
        // some memory for it.
        if ((buf = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
            T_E("Out of memory");
            return VTSS_RC_ERROR;
        }

        // Do convert to string
        (void)vlan_mgmt_vid_bitmask_to_txt(forbidden_vid_mask, buf);

        rc = vtss_icfg_printf(result, " switchport forbidden vlan add %s\n", buf);

        // And free it again
        VTSS_FREE(buf);
        if (rc != VTSS_RC_OK) {
            return rc;
        }

    } else if (req->all_defaults) {
        // No forbidden VLANs found on this interface. This is the default.
        // Only print a command if requested to (all_defaults)
        // And do not print the no-form. Use remove <all-vlans> instead.
        VTSS_RC(vtss_icfg_printf(result, " switchport forbidden vlan remove %u-%u\n", VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// VLAN_ICFG_vlan_list_print()
/******************************************************************************/
static mesa_rc VLAN_ICFG_vlan_list_print(vtss_icfg_query_result_t *result, u8 vlan_list[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    char    *buf;
    mesa_rc rc;

    // We have to convert the vlan_list to a string. This string can potentially
    // become quite long, so we have to dynamically allocate some memory for it.
    if ((buf = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Out of memory");
        return VTSS_RC_ERROR;
    }

    // Do convert to string
    (void)vlan_mgmt_vid_bitmask_to_txt(vlan_list, buf);

    if ((rc = vtss_icfg_printf(result, "vlan %s\n", buf)) == VTSS_RC_OK) {
        rc = vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n");
    }

    VTSS_FREE(buf);
    return rc;
}

/******************************************************************************/
// VLAN_ICFG_vlan_conf()
/******************************************************************************/
static mesa_rc VLAN_ICFG_vlan_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    u8         access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES], vlan_list[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    u32        idx;
    BOOL       at_least_one_vlan_with_defaults = FALSE;
    mesa_vid_t vid;
    mesa_rc    rc;

    // Only show access VLANs.
    if ((rc = vtss_appl_vlan_access_vids_get(access_vids)) != VTSS_RC_OK) {
        T_E("Huh? %s\n", error_txt(rc));
        return rc;
    }

    // The request method must support only showing a range of VLANs.
    // Most likely, the caller has set .cnt to 1 and min and max to the full
    // VLAN range. To speed things up, detect this special case.
    if (!(req->instance_id.vlan_list.cnt == 1 && req->instance_id.vlan_list.range[0].min == VTSS_APPL_VLAN_ID_MIN && req->instance_id.vlan_list.range[0].max == VTSS_APPL_VLAN_ID_MAX)) {
        memset(vlan_list, 0, sizeof(vlan_list));

        for (idx = 0; idx < req->instance_id.vlan_list.cnt; idx++) {
            for (vid = req->instance_id.vlan_list.range[idx].min; vid <= req->instance_id.vlan_list.range[idx].max; vid++) {
                VTSS_BF_SET(vlan_list, vid, 1);
            }
        }

        for (idx = 0; idx < sizeof(access_vids); idx++) {
            // Mask out those VIDs we shouldn't display
            access_vids[idx] &= vlan_list[idx];
        }
    }

    memset(vlan_list, 0, sizeof(vlan_list));
    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        BOOL    vlan_name_is_default;
        BOOL    flooding_is_default = TRUE;
        char    vlan_name[VTSS_APPL_VLAN_NAME_MAX_LEN];

        if (!VTSS_BF_GET(access_vids, vid)) {
            if (VTSS_APPL_VLAN_ID_DEFAULT == vid) {
                VTSS_RC(vtss_icfg_printf(result, "no vlan %u\n", VTSS_APPL_VLAN_ID_DEFAULT));
            }
            continue;
        }

        if ((rc = vtss_appl_vlan_name_get(vid, vlan_name, &vlan_name_is_default)) != VTSS_RC_OK) {
            // All VLANs have a name, so what's going on?
            T_E("Huh? vid = %u, rc = %s", vid, error_txt(rc));
            return VTSS_RC_ERROR;
        }

        if (fast_cap(VTSS_APPL_CAP_VLAN_FLOODING)) {
            mesa_bool_t flooding;
            if ((rc = vtss_appl_vlan_flooding_get(vid, &flooding)) != VTSS_RC_OK) {
                T_E("Failed to get flooding info. vid = %u, rc = %s", vid, error_txt(rc));
                return VTSS_RC_ERROR;
            }

            flooding_is_default = flooding ? TRUE : FALSE;
        }

        if (req->all_defaults || !vlan_name_is_default || !flooding_is_default) {
            // Gotta output the VLAN separately, since it has none-default
            // sub-params, or we are requested to output the sub-params. But
            // before we can do that, we need to possibly flush all VLANs
            // gathered so far which have default VLAN names.
            if (at_least_one_vlan_with_defaults) {
                if ((rc = VLAN_ICFG_vlan_list_print(result, vlan_list)) != VTSS_RC_OK) {
                    return rc;
                }

                // Start over
                memset(vlan_list, 0, sizeof(vlan_list));
                at_least_one_vlan_with_defaults = FALSE;
            }

            VTSS_RC(vtss_icfg_printf(result, "vlan %u\n", vid));
            if (!vlan_name_is_default) {
                VTSS_RC(vtss_icfg_printf(result, " name %s\n", vlan_name));
            }

            if (fast_cap(VTSS_APPL_CAP_VLAN_FLOODING)) {
                if (!flooding_is_default) {
                    VTSS_RC(vtss_icfg_printf(result, " no flooding\n"));
                }
            }

            VTSS_RC(vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n"));
        } else {
            VTSS_BF_SET(vlan_list, vid, 1); // Output it later.
            at_least_one_vlan_with_defaults = TRUE;
        }
    }

    if (at_least_one_vlan_with_defaults) {
        // Flush the remainder of the list
        rc = VLAN_ICFG_vlan_list_print(result, vlan_list);
    }

    return rc;
}

/******************************************************************************/
// VLAN_icfg_init()
/******************************************************************************/
mesa_rc VLAN_icfg_init(void)
{
    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_VLAN_GLOBAL_CONF, "vlan", VLAN_ICFG_global_conf));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_VLAN_PORT_CONF,   "vlan", VLAN_ICFG_port_conf));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_VLAN_CONF,        "vlan", VLAN_ICFG_vlan_conf));
    return VTSS_RC_OK;
}

