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

#include "web_api.h"
#include "vlan_api.h"
#include "mgmt_api.h"
#include "port_iter.hxx"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#ifdef __cplusplus
#include "enum_macros.hxx"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_VLAN
#define VLAN_WEB_BUF_LEN 512

#ifdef __cplusplus
VTSS_ENUM_INC(vtss_appl_vlan_user_t);
VTSS_ENUM_INC(vlan_port_flags_idx_t);
VTSS_ENUM_INC(vtss_appl_vlan_port_mode_t);
#endif /* #ifdef __cplusplus */

/******************************************************************************/
// Trace stuff
/******************************************************************************/
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>

/******************************************************************************/
// Web Handler Functions
/******************************************************************************/

/******************************************************************************/
// VLAN_WEB_id_not_found()
/******************************************************************************/
static void VLAN_WEB_id_not_found(const char *id)
{
    T_E("Element with id = %s not found", id);
}

/******************************************************************************/
// VLAN_WEB_vlan_list_get()
/******************************************************************************/
static BOOL VLAN_WEB_vlan_list_get(CYG_HTTPD_STATE *p, const char *id, char big_storage[3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES], char small_storage[VLAN_VID_LIST_AS_STRING_LEN_BYTES], u8 result[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    size_t found_len, cnt = 0;
    char   *found_ptr;

    if ((found_ptr = (char *)cyg_httpd_form_varable_string(p, id, &found_len)) != NULL) {

        // Unescape the string
        if (!cgi_unescape(found_ptr, big_storage, found_len, 3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES)) {
            T_E("Unable to unescape %s", found_ptr);
            return FALSE;
        }

        // Filter out white space
        found_ptr = big_storage;
        while (*found_ptr) {
            char c = *(found_ptr++);
            if (c != ' ') {
                small_storage[cnt++] = c;
            }
        }
    } else {
        VLAN_WEB_id_not_found(id);
        return FALSE;
    }

    small_storage[cnt] = '\0';

    VTSS_ASSERT(cnt < VLAN_VID_LIST_AS_STRING_LEN_BYTES);

    if (mgmt_txt2bf(small_storage, result, VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX, 0) != VTSS_RC_OK) {
        T_E("Failed to convert %s to binary form", small_storage);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
// VLAN_WEB_vlan()
/******************************************************************************/
static i32 VLAN_WEB_vlan(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                isid  = web_retrieve_request_sid(p);
    u8                         vid_bitmask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    vtss_appl_vlan_entry_t     *forbidden_vids = NULL;
    char                       *unescaped_vlan_list_as_string = NULL, *escaped_vlan_list_as_string = NULL;
    ulong                      an_ulong;
    vtss_appl_vlan_port_conf_t old_conf, new_conf;
    char                       buf[32];
    int                        an_int, cnt;
    mesa_rc                    rc;
    BOOL                       bulk_started = FALSE;
    mesa_vid_t                 vid;
    port_iter_t                pit;

    if (redirectUnmanagedOrInvalid(p, isid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN)) {
        return -1;
    }
#endif

    // A couple of things comment to both POST and GET operations:
    if ((unescaped_vlan_list_as_string = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Alloc of %d bytes failed", VLAN_VID_LIST_AS_STRING_LEN_BYTES);
        goto do_exit;
    }

    if ((escaped_vlan_list_as_string = (char *)VTSS_MALLOC(3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Alloc of %d bytes failed", 3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES);
        goto do_exit;
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {

        // Too bad that forbidden VLANs is not a per-port property, but a per-VLAN property in the VLAN module API.
        if (VTSS_CALLOC_CAST(forbidden_vids, VTSS_APPL_VLAN_ID_MAX + 1, sizeof(vtss_appl_vlan_entry_t)) == NULL) {
            T_E("Alloc of forbidden_vids failed");
            goto do_exit;
        }

        //
        // GLOBAL CONFIGURATION
        //

        vlan_bulk_update_begin();
        bulk_started = TRUE;

        // Create/Delete VLANs.
        if (!VLAN_WEB_vlan_list_get(p, "vlans", escaped_vlan_list_as_string, unescaped_vlan_list_as_string, vid_bitmask)) {
            goto do_exit;
        }

        // Create or delete VIDs as requested.
        if ((rc = vtss_appl_vlan_access_vids_set(vid_bitmask)) != VTSS_RC_OK) {
            T_E("VLAN add/delete failed: %s", error_txt(rc));
            goto do_exit;
        }

        // Change S-Custom Ethertype
        if (cyg_httpd_form_varable_hex(p, "tpid", &an_ulong)) {
            if (vtss_appl_vlan_s_custom_etype_set((mesa_etype_t)an_ulong) != VTSS_RC_OK) {
                T_E("Unable to set TPID");
            }
        } else {
            VLAN_WEB_id_not_found("tpid");
            goto do_exit;
        }

        //
        // PER-PORT CONFIGURATION
        //
        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {

            if (vlan_mgmt_port_conf_get(isid, pit.iport, &old_conf, VTSS_APPL_VLAN_USER_STATIC, FALSE) != VTSS_RC_OK) {
                continue; // Probably stack error
            }

            new_conf = old_conf;

            // Port mode (access/trunk/hybrid): Drop-down
            sprintf(buf, "mode_%d", pit.uport);

            if (cyg_httpd_form_varable_int(p, buf, &an_int)) {
                if (an_int != VTSS_APPL_VLAN_PORT_MODE_ACCESS && an_int != VTSS_APPL_VLAN_PORT_MODE_TRUNK && an_int != VTSS_APPL_VLAN_PORT_MODE_HYBRID) {
                    T_E("Invalid port mode %d", new_conf.mode);
                    goto do_exit;
                }
                new_conf.mode = (vtss_appl_vlan_port_mode_t)an_int;
            } else {
                VLAN_WEB_id_not_found(buf);
                goto do_exit;
            }

            // PVID
            sprintf(buf, "pvid_%d", pit.uport);
            if (cyg_httpd_form_varable_int(p, buf, &an_int)) {
                if (an_int < VTSS_APPL_VLAN_ID_MIN || an_int > VTSS_APPL_VLAN_ID_MAX) {
                    T_E("Invalid PVID (%u)", an_int);
                    goto do_exit;
                }

                switch (new_conf.mode) {
                case VTSS_APPL_VLAN_PORT_MODE_ACCESS:
                    new_conf.access_pvid = (mesa_vid_t)an_int;
                    break;

                case VTSS_APPL_VLAN_PORT_MODE_TRUNK:
                    new_conf.trunk_pvid = (mesa_vid_t)an_int;
                    break;

                case VTSS_APPL_VLAN_PORT_MODE_HYBRID:
                    new_conf.hybrid.pvid = (mesa_vid_t)an_int;
                    break;

                default:
                    break; // Unreachable
                }
            } else {
                VLAN_WEB_id_not_found(buf);
                goto do_exit;
            }

            // Port Type (unaware/C-port/S-Port/S-Custom-Port)
            // This can only be changed if port is in hybrid mode
            if (new_conf.mode == VTSS_APPL_VLAN_PORT_MODE_HYBRID) {
                // porttypev2_%d: drop-down
                sprintf(buf, "porttypev2_%d", pit.uport);
                // Enumeration on web-page matches enumeration of vtss_appl_vlan_port_type_t
                if (cyg_httpd_form_varable_int(p, buf, &an_int)) {
                    new_conf.hybrid.port_type = (vtss_appl_vlan_port_type_t)an_int;
                } else {
                    VLAN_WEB_id_not_found(buf);
                    goto do_exit;
                }
            }

            // Ingress Filtering: Checkbox
            // This can only be changed if port is in hybrid mode
            if (new_conf.mode == VTSS_APPL_VLAN_PORT_MODE_HYBRID) {
                sprintf(buf, "ingressflt_%d", pit.uport);
                new_conf.hybrid.ingress_filter = cyg_httpd_form_varable_find(p, buf) != NULL;
            }

            // Acceptable frames (a.k.a. Frame Type): All/Tagged/Untagged
            // This can only be changed if port is in hybrid mode
            if (new_conf.mode == VTSS_APPL_VLAN_PORT_MODE_HYBRID) {
                // frametypev2_%d: drop-down
                sprintf(buf, "frametypev2_%d", pit.uport);
                // "untagged" if 2, "tagged" if 1, all if 0
                if (cyg_httpd_form_varable_int(p, buf, &an_int)) {
                    new_conf.hybrid.frame_type = (mesa_vlan_frame_t)an_int;
                } else {
                    VLAN_WEB_id_not_found(buf);
                    goto do_exit;
                }
            }

            // Egress tagging: Drop-down
            // This can only be changed if port is in trunk or hybrid mode
            if (new_conf.mode != VTSS_APPL_VLAN_PORT_MODE_ACCESS) {
                if (new_conf.mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK) {
                    sprintf(buf, "tx_tag_trunk_%d", pit.uport);
                } else {
                    sprintf(buf, "tx_tag_%d", pit.uport);
                }
                if (cyg_httpd_form_varable_int(p, buf, &an_int)) {
                    vtss_appl_vlan_tx_tag_type_t t = (vtss_appl_vlan_tx_tag_type_t)an_int;

                    // It can only be untag_this or tag_all for trunk, and
                    // untag_this, tag_all, or untag_all for hybrid.
                    switch (t) {
                    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
                    case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
                        break;

                    case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
                        if (new_conf.mode != VTSS_APPL_VLAN_PORT_MODE_HYBRID) {
                            // Signal error
                            t = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS;
                        }
                        break;

                    default:
                        // Signal error
                        t = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS;
                        break;
                    }

                    if (t == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS) {
                        t = (vtss_appl_vlan_tx_tag_type_t)an_int;
                        T_E("%u:%u: Invalid Tx tag type (%d) with mode = %d", isid, pit.uport, t, new_conf.mode);
                        goto do_exit;
                    }

                    if (new_conf.mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK) {
                        new_conf.trunk_tag_pvid = t == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL;
                    } else {
                        new_conf.hybrid.tx_tag_type = t;
                        // Can only untag PVID from Web.
                        new_conf.hybrid.untagged_vid = new_conf.hybrid.pvid;
                    }
                } else {
                    VLAN_WEB_id_not_found(buf);
                    goto do_exit;
                }
            }

            //
            // Allowed VIDs
            //

            // Allowed VIDs can only be changed in trunk and hybrid modes.
            if (new_conf.mode != VTSS_APPL_VLAN_PORT_MODE_ACCESS) {
                sprintf(buf, "alw_vids_%d", pit.uport);
                if (!VLAN_WEB_vlan_list_get(p, buf, escaped_vlan_list_as_string, unescaped_vlan_list_as_string, new_conf.mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? new_conf.trunk_allowed_vids : new_conf.hybrid_allowed_vids)) {
                    goto do_exit;
                }
            }

            if (memcmp(&new_conf, &old_conf, sizeof(new_conf)) != 0) {
                if ((rc = vlan_mgmt_port_conf_set(isid, pit.iport, &new_conf, VTSS_APPL_VLAN_USER_STATIC)) != VTSS_RC_OK) {
                    T_E("%u:%u: %s", isid, pit.uport, error_txt(rc));
                    goto do_exit;
                }
            }

            //
            // Forbidden VIDs
            //

            // We need to gather it first for all ports, because when we apply it, it's per VLAN rather than per port :(
            sprintf(buf, "forb_vids_%d", pit.uport);
            if (!VLAN_WEB_vlan_list_get(p, buf, escaped_vlan_list_as_string, unescaped_vlan_list_as_string, vid_bitmask)) {
                goto do_exit;
            }

            for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                forbidden_vids[vid].ports[pit.iport] = VTSS_BF_GET(vid_bitmask, vid);
            }
        }

        // Time to traverse and apply the forbidden VIDs
        for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
            // The vlan_mgmt_vlan_add() function also allows for deleting VTSS_APPL_VLAN_USER_FORBIDDEN.
            forbidden_vids[vid].vid = vid;
            if ((rc = vlan_mgmt_vlan_add(isid, &forbidden_vids[vid], VTSS_APPL_VLAN_USER_FORBIDDEN)) != VTSS_RC_OK) {
                T_E("%u:%u: %s", isid, pit.uport, error_txt(rc));
                goto do_exit;
            }
        }

        goto do_exit;
    } else {
        // CYG_HTTPD_METHOD_GET (+HEAD)
        mesa_etype_t tpid = 0x8100;

        // Format: <tpid>#<vlans>#<all_port_conf>
        //         <all_port_conf> = <port_conf_1>#<port_conf_2>#...#<port_conf_N>
        //         <port_conf_x>   = port_no/port_mode/access_pvid/trunk_pvid/trunk_tag_pvid/hyb_port_type/hyb_ingress_filter/hyb_frame_type/hyb_pvid/hyb_tx_tag_type/trk_allowed_vids/hyb_allowed_vids/forbidden_vids

        cyg_httpd_start_chunked("html");

        // TPID
        if ((rc = vtss_appl_vlan_s_custom_etype_get(&tpid)) != VTSS_RC_OK) {
            T_E("vtss_appl_vlan_s_custom_etype_get() failed: %s", error_txt(rc));
        }

        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%x#", tpid);
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        // Get current list of access VLANs defined by admin
        (void)vtss_appl_vlan_access_vids_get(vid_bitmask);
        (void)cgi_escape(vlan_mgmt_vid_bitmask_to_txt(vid_bitmask, unescaped_vlan_list_as_string), escaped_vlan_list_as_string);
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#", escaped_vlan_list_as_string);
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        // Per-port properties
        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if ((rc = vlan_mgmt_port_conf_get(isid, pit.iport, &old_conf, VTSS_APPL_VLAN_USER_STATIC, FALSE)) != VTSS_RC_OK) {
                T_E("%u:%u: %s", isid, pit.uport, error_txt(rc));
                continue; /* Probably stack error */
            }

            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%d/%u/%u/%d/%d/%d/%d/%u/%d/",
                           pit.uport,
                           old_conf.mode,
                           old_conf.access_pvid,
                           old_conf.trunk_pvid,
                           old_conf.trunk_tag_pvid,
                           old_conf.hybrid.port_type,
                           old_conf.hybrid.ingress_filter,
                           old_conf.hybrid.frame_type,
                           old_conf.hybrid.pvid,
                           old_conf.hybrid.tx_tag_type);
            cyg_httpd_write_chunked(p->outbuffer, cnt);

            // Allowed VIDs for trunk mode
            for (old_conf.mode = VTSS_APPL_VLAN_PORT_MODE_TRUNK; old_conf.mode <= VTSS_APPL_VLAN_PORT_MODE_HYBRID; old_conf.mode++) {
                (void)cgi_escape(vlan_mgmt_vid_bitmask_to_txt(old_conf.mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK ? old_conf.trunk_allowed_vids : old_conf.hybrid_allowed_vids, unescaped_vlan_list_as_string), escaped_vlan_list_as_string);
                cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/", escaped_vlan_list_as_string);
                cyg_httpd_write_chunked(p->outbuffer, cnt);
            }

            // Forbidden VIDs
            if ((rc = vlan_mgmt_membership_per_port_get(isid, pit.iport, VTSS_APPL_VLAN_USER_FORBIDDEN, vid_bitmask)) != VTSS_RC_OK) {
                T_E("%u:%u: %s", isid, pit.uport, error_txt(rc));
                continue; /* Probably stack error */
            }

            (void)cgi_escape(vlan_mgmt_vid_bitmask_to_txt(vid_bitmask, unescaped_vlan_list_as_string), escaped_vlan_list_as_string);
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s", escaped_vlan_list_as_string, pit.last ? "" : "#");
            if (cnt > 0) {
                // If there are no forbidden VLANs, the last snprintf() may have returned 0.
                cyg_httpd_write_chunked(p->outbuffer, cnt);
            }
        }

        cyg_httpd_end_chunked();
    }

do_exit:
    VTSS_FREE(forbidden_vids);
    VTSS_FREE(escaped_vlan_list_as_string);
    VTSS_FREE(unescaped_vlan_list_as_string);

    if (bulk_started) {
        vlan_bulk_update_end();
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/vlan.htm");
    }

    return -1; // Do not further search the file system.
}

/******************************************************************************/
// VLAN_WEB_svl()
/******************************************************************************/
static i32 VLAN_WEB_svl(CYG_HTTPD_STATE *p)
{
    vtss_isid_t isid = web_retrieve_request_sid(p);
    BOOL        bulk_started = FALSE;
    mesa_vid_t  vid, fid, temp_fid;
    u32         id, fid_cnt = 0;
    long        a_long = 0;
    char        *unescaped_vlan_list_as_string = NULL, *escaped_vlan_list_as_string = NULL;
    char        fid_in_use[((4095 + 1) + 7) / 8];
    char        buf[32];
    u8          old_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES], new_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    mesa_rc     rc;
    size_t      cnt;

    T_I("Enter");

    if (!VTSS_APPL_VLAN_FID_CNT) {
        // No SVL support
        return -1;
    }

    if (redirectUnmanagedOrInvalid(p, isid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN)) {
        return -1;
    }
#endif

    // A couple of things common to both POST and GET operations:
    if ((unescaped_vlan_list_as_string = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Alloc of %d bytes failed", VLAN_VID_LIST_AS_STRING_LEN_BYTES);
        goto do_exit;
    }

    if ((escaped_vlan_list_as_string = (char *)VTSS_MALLOC(3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Alloc of %d bytes failed", 3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES);
        goto do_exit;
    }

    // Start by figuring out which FIDs are currently in use, so that we know
    // what to delete when done. This is needed both for POST and GET.
    memset(fid_in_use, 0, sizeof(fid_in_use));
    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        if ((rc = vtss_appl_vlan_fid_get(vid, &fid)) != VTSS_RC_OK) {
            T_E("Unable to get FID for VLAN %u. rc = %s", vid, error_txt(rc));
            goto do_exit;
        }

        if (fid != 0) {
            if (!VTSS_BF_GET(fid_in_use, fid)) {
                VTSS_BF_SET(fid_in_use, fid, 1);
                fid_cnt++;
            }
        }
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        vlan_bulk_update_begin();
        bulk_started = TRUE;

        // The IDs are allocated in the range [0; VTSS_APPL_VLAN_FID_CNT[
        // The FIDs are not the same as the IDs but must be in a similar range ([1; VTSS_APPL_VLAN_FID_CNT]).
        for (id = 0; id < VTSS_APPL_VLAN_FID_CNT; id++) {
            T_I("Looking for ID = %u", id);

            sprintf(buf, "fid_%u", id);
            if (!cyg_httpd_form_varable_long_int(p, buf, &a_long)) {
                // This ID doesn't exist. Next.
                continue;
            }

            // FID
            fid = (mesa_vid_t)a_long;

            T_I("Found. ID = %u maps to FID = %u", id, fid);

            // VLAN list
            sprintf(buf, "vlans_%u", id);
            if (!VLAN_WEB_vlan_list_get(p, buf, escaped_vlan_list_as_string, unescaped_vlan_list_as_string, new_vids)) {
                goto do_exit;
            }

            T_I_HEX(new_vids, sizeof(new_vids));

            // Gotta get a bitmask of VIDs already mapped to this FID.
            // If we didn't do it this way, we would have to delete all current
            // mappings and start on a fresh. This could lead to intermittent
            // unexpected forwarding of frames in the switch core.
            memset(old_vids, 0, sizeof(old_vids));
            if (VTSS_BF_GET(fid_in_use, fid)) {
                // Already mapped. Get current mapping.
                for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                    if ((rc = vtss_appl_vlan_fid_get(vid, &temp_fid)) != VTSS_RC_OK) {
                        T_E("Unable to get FID for VLAN %u. rc = %s", vid, error_txt(rc));
                        goto do_exit;
                    }

                    if (temp_fid == fid) {
                        VTSS_BF_SET(old_vids, vid, 1);
                    }
                }
            }

            T_I_HEX(old_vids, sizeof(old_vids));

            BOOL msg_printed = FALSE;

            for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                BOOL old_vid_mapped_to_this_fid = VTSS_BF_GET(old_vids, vid);
                BOOL new_vid_mapped_to_this_fid = VTSS_BF_GET(new_vids, vid);

                if (old_vid_mapped_to_this_fid != new_vid_mapped_to_this_fid) {
                    mesa_vid_t new_fid;

                    if (old_vid_mapped_to_this_fid) {
                        // Gotta remove membership of this FID
                        new_fid = 0;

                    } else {
                        // Gotta add membership of this FID
                        new_fid = fid;
                    }

                    if (!msg_printed) {
                        T_I("Grouping VID %u into FID %u (old_vid_mapped_to_this_fid = %d, new_vid_mapped_to_this_fid = %d). FID = %u", vid, new_fid, old_vid_mapped_to_this_fid, new_vid_mapped_to_this_fid, fid);
                        msg_printed = TRUE;
                    }

                    if ((rc = vtss_appl_vlan_fid_set(vid, new_fid)) != VTSS_RC_OK) {
                        T_E("Unable to set FID %u for VLAN %u. rc = %s", new_fid, vid, error_txt(rc));
                        goto do_exit;
                    }
                }
            }

            // We know that the JavaScript has checked that the same FID is not
            // in use twice, so we can safely remove this FID from the fid_in_use
            // array, so that we know what to delete after we've run through all
            // current FIDs.
            VTSS_BF_SET(fid_in_use, fid, 0);
        }

        // Remove those FIDs no longer in use
        for (fid = 1; fid <= VTSS_APPL_VLAN_FID_CNT; fid++) {
            if (VTSS_BF_GET(fid_in_use, fid)) {
                // Remove membership for those that was deleted from the Web page
                T_I("Removing all VIDs from FID %u", fid);
                for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                    if ((rc = vtss_appl_vlan_fid_get(vid, &temp_fid)) != VTSS_RC_OK) {
                        T_E("Unable to get FID for VLAN %u. rc = %s", vid, error_txt(rc));
                        goto do_exit;
                    }

                    if (temp_fid == fid) {
                        if ((rc = vtss_appl_vlan_fid_set(vid, 0)) != VTSS_RC_OK) {
                            T_E("Unable to remove membership of VLAN %u. rc = %s", vid, error_txt(rc));
                            goto do_exit;
                        }
                    }
                }
            }
        }
    } else {
        // CYG_HTTPD_METHOD_GET (+HEAD)
        BOOL first = TRUE;

        // Format: <fid_cnt>|<fid_conf_1>#<fid_conf_2>#...#<fid_conf_N>
        //         <fid_conf_x> = fid/vids
        // <fid_cnt> is just to always make it non-empty to avoid the SpomHandleError() below

        cyg_httpd_start_chunked("html");

        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u|", fid_cnt);
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        for (fid = 1; fid <= VTSS_APPL_VLAN_FID_CNT; fid++) {
            if (!VTSS_BF_GET(fid_in_use, fid)) {
                continue;
            }

            // Get a VLAN list for #fid.
            memset(new_vids, 0, sizeof(new_vids));
            for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
                if ((rc = vtss_appl_vlan_fid_get(vid, &temp_fid)) != VTSS_RC_OK) {
                    T_E("Unable to get FID for VLAN %u. rc = %s", vid, error_txt(rc));
                    goto do_exit;
                }

                if (temp_fid == fid || (vid == fid && temp_fid == 0)) {
                    VTSS_BF_SET(new_vids, vid, 1);
                }
            }

            // Convert the VLAN list to a string
            (void)cgi_escape(vlan_mgmt_vid_bitmask_to_txt(new_vids, unescaped_vlan_list_as_string), escaped_vlan_list_as_string);
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%s", first ? "" : "#", fid, escaped_vlan_list_as_string);
            cyg_httpd_write_chunked(p->outbuffer, cnt);
            first = FALSE;
        }

        cyg_httpd_end_chunked();
    }

do_exit:
    VTSS_FREE(escaped_vlan_list_as_string);
    VTSS_FREE(unescaped_vlan_list_as_string);

    if (bulk_started) {
        vlan_bulk_update_end();
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/vlan_svl.htm");
    }

    return -1; // Do not further search the file system.
}

/******************************************************************************/
// VLAN_WEB_port_status()
/******************************************************************************/
static i32 VLAN_WEB_port_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                isid  = web_retrieve_request_sid(p);
    vtss_appl_vlan_port_conf_t port_conf;
    vtss_appl_vlan_user_t      user, requested_user;
    vlan_port_conflicts_t      conflicts;
    BOOL                       conflicts_exist;
    port_iter_t                pit;
    int                        cnt, an_int;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_VLAN)) {
        return -1;
    }
#endif

    // Get the requested user.
    if (cyg_httpd_form_varable_int(p, "user", &an_int) == FALSE || an_int < 1 || an_int > VTSS_APPL_VLAN_USER_LAST) {
        // Convention with the web-page is that the "user" is 1-based, so if
        // it's out of bounds, default to VTSS_APPL_VLAN_USER_ALL (i.e. "Combined");
        requested_user = VTSS_APPL_VLAN_USER_ALL;
    } else {
        // User is 1-based, our enumeration is 0-based.
        requested_user = (vtss_appl_vlan_user_t)(an_int - 1);

        // VTSS_APPL_VLAN_USER_ALL is not a valid membership changer, and we should never be called with forbidden.
        if (requested_user != VTSS_APPL_VLAN_USER_ALL && !vlan_mgmt_user_is_port_conf_changer(requested_user)) {
            T_E("Internal error (%d)", an_int);
            requested_user = VTSS_APPL_VLAN_USER_ALL;
        }
    }

    // Format: [all_user_names_and_ids]#[port_infos]
    // Where
    // [all_user_names_and_ids] = [user_name_id_1]/[user_name_id_2]/.../[user_name_id_n]
    // [user_name_id_N]         = user_name_1|user_enum_1
    // [port_infos]             = requested_user_name/[port_info_1]/[port_info_2]/.../[port_info_n]
    // [port_info_N]            = uport|conflicts_exist|pvid|pvid_set|ingress_filter|ingress_filter_set|frame_type|frame_type_set|tx_tag_type|tx_tag_type_set|port_type|port_type_set|untagged_vid
    // There are only [port_info_x] elements for ports that actually have data to present.

    cyg_httpd_start_chunked("html");

    // We wish to show "Combined" by default, so it must come first.
    cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|%d", vlan_mgmt_user_to_txt(VTSS_APPL_VLAN_USER_ALL), VTSS_APPL_VLAN_USER_ALL + 1);
    cyg_httpd_write_chunked(p->outbuffer, cnt);

    for (user = (vtss_appl_vlan_user_t)0; user < VTSS_APPL_VLAN_USER_LAST; user++) {
        // Only publish those VLAN users that may port configuration
        if (!vlan_mgmt_user_is_port_conf_changer(user)) {
            continue;
        }

        // We offset the enum by one to avoid problems in case the Web page is requesting for the first time.
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s|%d", vlan_mgmt_user_to_txt(user), user + 1);
        cyg_httpd_write_chunked(p->outbuffer, cnt);
    }

    // The requested user name must be present once.
    cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s", vlan_mgmt_user_to_txt(requested_user));
    cyg_httpd_write_chunked(p->outbuffer, cnt);

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        mesa_vid_t uvid = VTSS_VID_NULL;
        BOOL       can_be_any_uvid = FALSE, pvid_overridden, tx_tag_overridden;

        memset(&conflicts, 0, sizeof(conflicts));

        if (vlan_mgmt_port_conf_get(isid, pit.iport, &port_conf, requested_user, TRUE) != VTSS_RC_OK ||
            vlan_mgmt_port_conflicts_get(isid, pit.iport, &conflicts)                  != VTSS_RC_OK) {
            T_E("vlan_port GET error, isid %u, iport %u", isid, pit.iport);
            continue;
        }

        if (conflicts.port_flags == 0) {
            conflicts_exist = FALSE;
        } else if (requested_user == VTSS_APPL_VLAN_USER_ALL) {
            // For the combined user, don't go through the loop below. If you did, you wouldn't see any conflicts.
            conflicts_exist = TRUE;
        } else {
            vlan_port_flags_idx_t temp;
            u32                   all_usrs = 0;

            for (temp = (vlan_port_flags_idx_t)0; temp < VLAN_PORT_FLAGS_IDX_CNT; temp++) {
                all_usrs |= conflicts.users[temp];
            }

            conflicts_exist = (all_usrs & VTSS_BIT(requested_user)) != 0;
        }

        if (!port_conf.hybrid.flags) {
            // No parameters are overridden for this user on this port.
            continue;
        }

        pvid_overridden   = (port_conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_PVID)        != 0;
        tx_tag_overridden = (port_conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE) != 0;

        if (tx_tag_overridden && (port_conf.hybrid.tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS || port_conf.hybrid.tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS)) {
            // User has overridden Tx tag, and he cares about the tagged or untagged VID.
            // If he hasn't overridden the PVID, it must be a separate UVID he is attempting to tag.
            // The same applies if he has overridden PVID, but it differs from the requested untagged VID.
            if (!pvid_overridden || port_conf.hybrid.untagged_vid != port_conf.hybrid.pvid) {
                uvid = port_conf.hybrid.untagged_vid;
                can_be_any_uvid = TRUE;
            }
        }

        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u|%d|%d|%d|%d|%d|%s|%d|%s|%d|%s|%d|%u",
                       pit.uport,
                       conflicts_exist,
                       port_conf.hybrid.pvid,                                                       (port_conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_PVID)        != 0,
                       port_conf.hybrid.ingress_filter,                                             (port_conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT)   != 0,
                       vlan_mgmt_frame_type_to_txt(port_conf.hybrid.frame_type),                    (port_conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE) != 0,
                       vlan_mgmt_tx_tag_type_to_txt(port_conf.hybrid.tx_tag_type, can_be_any_uvid), (port_conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE) != 0,
                       vlan_mgmt_port_type_to_txt(port_conf.hybrid.port_type),                      (port_conf.hybrid.flags & VTSS_APPL_VLAN_PORT_FLAGS_AWARE)       != 0,
                       uvid);
        cyg_httpd_write_chunked(p->outbuffer, cnt);
    }

    cyg_httpd_end_chunked();
    return -1;
}

/******************************************************************************/
// VLAN_WEB_vlan_status()
/******************************************************************************/
static i32 VLAN_WEB_vlan_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           isid  = web_retrieve_request_sid(p);
    vtss_appl_vlan_user_t user, requested_user;
    mesa_vid_t            vid;
    int                   cnt, entry_cnt, left_cnt, an_int, start_vid;

    if (redirectUnmanagedOrInvalid(p, isid)) {
        /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_VLAN)) {
        return -1;
    }
#endif

    if (!cyg_httpd_form_varable_int(p, "DynNumberOfEntries", &entry_cnt) || entry_cnt < 1) {
        entry_cnt = 20; // Default
    }

    if (!cyg_httpd_form_varable_int(p, "DynStartVid", &start_vid) ||
        start_vid < VTSS_APPL_VLAN_ID_MIN || start_vid > VTSS_APPL_VLAN_ID_MAX) {
        start_vid = VTSS_VID_NULL + 1; // Default
    }

    // Get the requested user.
    if (cyg_httpd_form_varable_int(p, "user", &an_int) == FALSE || an_int < 1 || an_int > VTSS_APPL_VLAN_USER_LAST) {
        // Convention with the web-page is that the "user" is 1-based, so if
        // it's out of bounds, default to VTSS_APPL_VLAN_USER_ALL (i.e. "Combined");
        requested_user = VTSS_APPL_VLAN_USER_ALL;
    } else {
        // User is 1-based, our enumeration is 0-based.
        requested_user = (vtss_appl_vlan_user_t)(an_int - 1);

        // VTSS_APPL_VLAN_USER_ALL is not a valid membership changer, and we should never be called with forbidden.
        if (requested_user != VTSS_APPL_VLAN_USER_ALL && (requested_user == VTSS_APPL_VLAN_USER_FORBIDDEN || !vlan_mgmt_user_is_membership_changer(requested_user))) {
            T_E("Internal error (%d)", an_int);
            requested_user = VTSS_APPL_VLAN_USER_ALL;
        }
    }

    // Format: [all_user_names_and_ids]#[vlan_infos]
    // Where
    // [all_user_names_and_ids] = [user_name_id_1]/[user_name_id_2]/.../[user_name_id_n]
    // [user_name_id_N]         = user_name_1|user_enum_1
    // [vlan_infos]             = requested_user_name/[vlan_info_1]/[vlan_info_2]/.../[vlan_info_n]
    // [vlan_info_N]            = vid|port_val_1|port_val_2|...|port_val_n
    // port_val_N               = 0: not member, 1: member, 2: forbidden, 3: conflict (VLAN user says include port, forbidden says don't).

    cyg_httpd_start_chunked("html");

    // We wish to show "Combined" by default, so it must come first.
    cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|%d", vlan_mgmt_user_to_txt(VTSS_APPL_VLAN_USER_ALL), VTSS_APPL_VLAN_USER_ALL + 1);
    cyg_httpd_write_chunked(p->outbuffer, cnt);

    for (user = (vtss_appl_vlan_user_t)0; user < VTSS_APPL_VLAN_USER_LAST; user++) {
        // Only publish those VLAN users that may change membership
        if (!vlan_mgmt_user_is_membership_changer(user)) {
            continue;
        }

        // Do not publish VTSS_APPL_VLAN_USER_FORBIDDEN. This will be included in VTSS_APPL_VLAN_USER_STATIC to be backward-lookalike-compatible.
        if (user != VTSS_APPL_VLAN_USER_FORBIDDEN) {
            // We offset the enum by one to avoid problems in case the Web page is requesting for the first time.
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s|%d", vlan_mgmt_user_to_txt(user), user + 1);
            cyg_httpd_write_chunked(p->outbuffer, cnt);
        }
    }

    // The requested user name must always be present whether or not any VLANs are found for that user.
    cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s", vlan_mgmt_user_to_txt(requested_user));
    cyg_httpd_write_chunked(p->outbuffer, cnt);

    vid = start_vid - 1;  // Subtract one because we are going to ask for the NEXT entry.
    left_cnt = entry_cnt; // Used to determine how many entries that shall be passed to browser

    while (left_cnt > 0) {
        port_iter_t pit;
        vtss_appl_vlan_entry_t forbidden_conf, user_conf;
        BOOL                   forbidden_found;
        BOOL                   user_found = vtss_appl_vlan_get(isid, vid, &user_conf, TRUE, requested_user) == VTSS_RC_OK;

        if (!user_found) {
            // Nothing more to display.
            break;
        }

        vid = user_conf.vid;

        forbidden_found = vtss_appl_vlan_get(isid, vid, &forbidden_conf, FALSE, VTSS_APPL_VLAN_USER_FORBIDDEN) == VTSS_RC_OK;

        if (forbidden_found && requested_user == VTSS_APPL_VLAN_USER_ALL) {
            // Unfortunately, we have to loop once more to figure out if a non-forbidden user
            // has added a VLAN. If not, then it's only the forbidden user that has, in which case
            // we don't display it.
            // Also, in order to show the conflict symbol on the Web-page, we have to
            // combine all the non-forbidden users ourselves...
            vtss_appl_vlan_entry_t temp_conf;
            u32                    port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

            vtss_clear(user_conf);
            user_found = FALSE;

            for (user = (vtss_appl_vlan_user_t)0; user < VTSS_APPL_VLAN_USER_LAST; user++) {
                unsigned int i;

                if (user == VTSS_APPL_VLAN_USER_ALL || user == VTSS_APPL_VLAN_USER_FORBIDDEN) {
                    continue;
                }

                if (vtss_appl_vlan_get(isid, vid, &temp_conf, FALSE, user) != VTSS_RC_OK) {
                    continue;
                }

                user_found = TRUE;

                for (i = 0; i < port_cnt; i++) {
                    user_conf.ports[i] |= temp_conf.ports[i];
                }
            }

            if (!user_found) {
                // Only forbidden user had a share in this one. Skip displaying it.
                continue;
            }
        }

        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", vid);
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            BOOL f = forbidden_found ? forbidden_conf.ports[pit.iport] : 0;
            BOOL u = user_conf.ports[pit.iport];
            int v;

            if (f && u) {
                v = 3; // Conflict, no matter user.
            } else if (f && (requested_user == VTSS_APPL_VLAN_USER_STATIC || requested_user == VTSS_APPL_VLAN_USER_ALL)) {
                v = 2; // Forbidden port. Only show this for static or combined users.
            } else {
                v = u; // Either volatile user or not forbidden. Display membership as it's configured.
            }

            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d", v);
            cyg_httpd_write_chunked(p->outbuffer, cnt);
        }

        left_cnt--;
    }

    cyg_httpd_end_chunked();
    return -1;
}

/******************************************************************************/
// VLAN_WEB_config_js()
/******************************************************************************/
static size_t VLAN_WEB_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[VLAN_WEB_BUF_LEN];
    (void)snprintf(buff, VLAN_WEB_BUF_LEN,
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                   "var configHasIngressFiltering = 1;\n"
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
                   "var configVlanIdMin = %d;\n"
                   "var configVlanIdMax = %d;\n"
                   "var configVlanEntryCnt = %d;\n"
                   "var vlan_svl_fid_cnt = %d;\n",
                   VTSS_APPL_VLAN_ID_MIN,   /* First VLAN ID */
                   VTSS_APPL_VLAN_ID_MAX,   /* Last VLAN ID */
                   VLAN_ENTRY_CNT,          /* Number of configurable entries */
                   VTSS_APPL_VLAN_FID_CNT   /* Shared VLAN Learning: Number of FIDs, 0 if feature is unsupported */
                  );

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/******************************************************************************/
// VLAN_WEB_filter_css()
/******************************************************************************/
static size_t VLAN_WEB_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[VLAN_WEB_BUF_LEN];
    BOOL buggy_vlan = FALSE;

    switch (fast_cap(MESA_CAP_MISC_CHIP_FAMILY)) {
    case MESA_CHIP_FAMILY_CARACAL:
    case MESA_CHIP_FAMILY_OCELOT:
        // Luton26 and Serval1's VLAN handling of S- and Custom-S ports
        // is a bit weird for C-tagged frames.
        buggy_vlan = TRUE;
        break;
    default:
        break;
    }

    (void)snprintf(buff, VLAN_WEB_BUF_LEN,
#ifndef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                   ".has_vlan_ingress_filtering { display: none; }\r\n"
#endif
#ifndef VTSS_SW_OPTION_VLAN_NAMING
                   ".has_vlan_naming { display: none; }\r\n"
#endif
#ifndef VTSS_SW_OPTION_MVRP
                   ".has_mvrp { display: none; }\r\n"
#endif
                   ".has_vlan_v1 { display: none; }\r\n"
                   "%s",
                   buggy_vlan ? "" : ".has_buggy_vlan { display: none; }\r\n");

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/******************************************************************************/
// Registration of various handlers
/******************************************************************************/
web_lib_filter_css_tab_entry(VLAN_WEB_filter_css);
web_lib_config_js_tab_entry(VLAN_WEB_config_js);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vlan,          "/config/vlan",               VLAN_WEB_vlan);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vlan_svl,      "/config/vlan_svl",           VLAN_WEB_svl);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_vlan_port,       "/stat/vlan_port_stat",       VLAN_WEB_port_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_vlan_membership, "/stat/vlan_membership_stat", VLAN_WEB_vlan_status);

