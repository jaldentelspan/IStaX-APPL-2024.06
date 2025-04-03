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

#include "vlan_translation_api.h"
#include "web_api.h"
#include "port_iter.hxx"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif
#include "vlan_translation_trace.h"
#include "vtss/basics/trace.hxx"

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
static i32 handler_config_vlan_trans_port2group_conf(CYG_HTTPD_STATE *p)
{
    vtss_appl_vlan_translation_if_conf_value_t group;
    port_iter_t                                pit;
    vtss_isid_t                                isid;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN_TRANSLATION)) {
        return -1;
    }
#endif
    isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    if (p->method == CYG_HTTPD_METHOD_POST) {/* POST Method */
        if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
            return -1;
        }
        int temp, errors = 0, def;
        mesa_rc rc;

        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "WEB update VLAN Translation port configuration";
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if ((rc = vlan_trans_mgmt_port_conf_get(pit.uport, &group)) != VTSS_RC_OK) {
                errors++;
                VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "WEB: Error while fetching port #" << pit.uport << " configuration - " << error_txt(rc);
                continue;
            }
            /* Translate */
            def = cyg_httpd_form_variable_check_fmt(p, "default_%d", pit.uport);
            if (def == 0) {
                /* Group ID */
                if (cyg_httpd_form_variable_int_fmt(p, &temp, "group_id_%u", pit.uport)) {
                    if (group.gid == temp) {
                        continue;
                    } else {
                        group.gid = temp;
                    }
                }
                if ((rc = vlan_trans_mgmt_port_conf_set(pit.uport, group)) != VTSS_RC_OK) {
                    if (rc != VT_ERROR_API_IF_SET) {
                        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "WEB: Error while updating port #" << pit.uport << " configuration - " << error_txt(rc);
                        errors++;
                    }
                }
            } else {
                if ((rc = vlan_trans_mgmt_port_conf_def(pit.uport)) != VTSS_RC_OK) {
                    if (rc != VT_ERROR_API_IF_DEF) {
                        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "WEB: Error while updating port #" << pit.uport << " configuration - " << error_txt(rc);
                        errors++;
                    }
                }
            }
        }
        redirect(p, errors ? "/vlan_trans_port_config.htm?IF_error=1" : "/vlan_trans_port_config.htm");
    } else {/* GET Method */
        int ct, def = 0;

        /*
         * Format:
         * <port 1>|<port 2>|<port 3>|...<port n>
         *
         * port x :== <port_no>#<default>#<group_id>
         *   port_no   :== 1..max
         *   default   :== 1 for default, 0 for manual
         *   group_id  :== 1..max
         */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            VTSS_RC(vlan_trans_mgmt_port_conf_get(pit.uport, &group));
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u#%u#%u",
                          pit.first ? "" : "|",
                          pit.uport,
                          def, // always send 0 so that checkbox is always clear
                          group.gid);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}

static i32 handler_config_vlan_trans_map_conf(CYG_HTTPD_STATE *p)
{
    vtss_appl_vlan_translation_group_mapping_key_t temp = {}, *in = NULL, out = {};
    mesa_vid_t                                     tvid;
    int                                            map_flag = 0, var_value, ct;
    mesa_rc                                        rc = MESA_RC_OK;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN_TRANSLATION)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) {    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        // Format       : [mapConfigFlag]/[selectGroupId]/[selectDir]/[selectVlanId]
        // <Edit>       :               1/          [gid]/      [dir]/         [vid]
        // <Delete>     :               2/          [gid]/      [dir]/         [vid]
        // <Delete All> :               3/
        // <Add New>    :               4/

        if (cyg_httpd_form_varable_int(p, "mapConfigFlag", &map_flag)) {
            switch (map_flag) {
            case 2:
                if (cyg_httpd_form_varable_int(p, "selectGroupId", &var_value)) {
                    temp.gid = (u16) var_value;
                    if (cyg_httpd_form_varable_int(p, "selectDir", &var_value)) {
                        temp.dir = (mesa_vlan_trans_dir_t) var_value;
                        if (cyg_httpd_form_varable_int(p, "selectVlanId", &var_value)) {
                            temp.vid = (mesa_vid_t) var_value;
                            if (map_flag == 2) {
                                if ((rc = vtss_appl_vlan_translation_group_conf_del(temp)) != MESA_RC_OK) {
                                    if (rc != VT_ERROR_API_MAP_DEL) {
                                        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "WEB: Error while deleting old mapping - " << error_txt(rc);
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            case 3:
                while (vtss_appl_vlan_translation_group_conf_itr(in, &out) == MESA_RC_OK) {
                    temp = out;
                    if ((rc = vtss_appl_vlan_translation_group_conf_del(temp)) != VTSS_RC_OK) {
                        if (rc != VT_ERROR_API_MAP_DEL) {
                            VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "WEB: Error while deleting old mapping - " << error_txt(rc);
                            break;
                        }
                    }
                }
                break;
            default:
                break;
            }
        }

        /* Format: <Group ID i>/<Direction i>/<VID i>/<Translated to VID i>|... */
        cyg_httpd_start_chunked("html");
        in = NULL;
        while (vtss_appl_vlan_translation_group_conf_itr(in, &out) == MESA_RC_OK) {
            if (vtss_appl_vlan_translation_group_conf_get(out, &tvid) == MESA_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u|", out.gid, out.dir, out.vid, tvid);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            temp = out;
            in = &temp;
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}

static i32 handler_config_vlan_trans_map_conf_edit(CYG_HTTPD_STATE *p)
{
    vtss_appl_vlan_translation_group_mapping_key_t temp = {};
    int                                            map_flag = 0, var_value = 0, ct;
    mesa_vid_t                                     tvid = 0;
    mesa_rc                                        rc;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN_TRANSLATION)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
        if (cyg_httpd_form_varable_int(p, "gid_", &var_value)) {
            temp.gid = (u16) var_value;
            if (cyg_httpd_form_varable_int(p, "dir_", &var_value)) {
                temp.dir = (mesa_vlan_trans_dir_t) var_value;
                if (cyg_httpd_form_varable_int(p, "vid_", &var_value)) {
                    temp.vid = (mesa_vid_t) var_value;
                    if (cyg_httpd_form_varable_int(p, "tvid_", &var_value)) {
                        tvid = (mesa_vid_t) var_value;
                        if ((rc = vtss_appl_vlan_translation_group_conf_set(temp, &tvid)) != MESA_RC_OK) {
                            if (rc != VT_ERROR_API_MAP_ADD) {
                                VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "WEB: " << error_txt(rc);
                            }
                            if (rc == VT_ERROR_ENTRY_CONFLICT) {
                                redirect(p, "/map.htm?vt_error=1");
                                return -1;
                            }
                        }
                    }
                }
            }
        }
        redirect(p, "/map.htm");
    } else {    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        // Format       : [mapEditFlag]/[selectGroupId]/[selectDir]/[selectVlanId]
        // <Edit>       :             1/          [gid]/      [dir]/         [vid]
        // <Add New> :                4/

        if (cyg_httpd_form_varable_int(p, "mapEditFlag", &map_flag)) {
            switch (map_flag) {
            case 1:
                // Editing an existing entry, so first parse the key
                // and then fetch the entry from the mgmt API.
                if (cyg_httpd_form_varable_int(p, "selectGroupId", &var_value)) {
                    temp.gid = (u16) var_value;
                    if (cyg_httpd_form_varable_int(p, "selectDir", &var_value)) {
                        temp.dir = (mesa_vlan_trans_dir_t) var_value;
                        if (cyg_httpd_form_varable_int(p, "selectVlanId", &var_value)) {
                            temp.vid = (u16) var_value;
                            if (vtss_appl_vlan_translation_group_conf_get(temp, &tvid) != MESA_RC_OK) {
                                tvid = 0;
                            }
                        }
                    }
                }
                break;
            case 4:
                // Adding new entry, so provide an empty template
                temp.gid = 0;
                temp.dir = MESA_VLAN_TRANS_DIR_BOTH;
                temp.vid = 0;
                tvid = 0;
                break;
            default:
                break;
            }
        }

        //Format: <gid>/<dir>/<vid>/<tvid>
        (void) cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u",
                      temp.gid,
                      temp.dir,
                      temp.vid,
                      tvid);
        (void) cyg_httpd_write_chunked(p->outbuffer, ct);
        (void) cyg_httpd_end_chunked();
    }
    return -1;
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t vlan_translation_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[512];
    (void) snprintf(buff, 512, "var configVlanTranslationMax = %d;\n", VT_MAX_TRANSLATIONS);
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}


/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(vlan_translation_lib_config_js);
/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vlan_trans_port2group_conf, "/config/port2group_conf", handler_config_vlan_trans_port2group_conf);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vlan_trans_map_conf, "/config/mapping_conf", handler_config_vlan_trans_map_conf);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_vlan_trans_map_conf_edit, "/config/mapping_conf_edit", handler_config_vlan_trans_map_conf_edit);

