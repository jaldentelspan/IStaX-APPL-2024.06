/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "mgmt_api.h"
#include "port_api.h" /* For port_count_max() */

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#include "mvr_api.h"
#include "mvr_trace.h"
#include <vtss/appl/mvr.h>

#define MVR_WEB_BUF_LEN 512

/******************************************************************************/
// MVR_WEB_global_conf_get()
/******************************************************************************/
static mesa_rc MVR_WEB_global_conf_get(vtss_appl_ipmc_lib_global_conf_t &global_conf)
{
    vtss_appl_ipmc_lib_key_t key;
    mesa_rc                  rc;

    key.is_mvr  = true;
    key.is_ipv4 = true; // Doesn't matter, since IGMP and MLD conf are identical

    if ((rc = vtss_appl_ipmc_lib_global_conf_get(key, &global_conf)) != VTSS_RC_OK) {
        T_EG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_global_conf_get(%s) failed: %s", key, error_txt(rc));

        if ((rc = vtss_appl_ipmc_lib_global_conf_default_get(key, &global_conf)) != VTSS_RC_OK) {
            T_EG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_global_conf_default_get(%s) failed: %s", key, error_txt(rc));
            vtss_clear(global_conf);
            return rc;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// MVR_WEB_port_conf_get()
/******************************************************************************/
static void MVR_WEB_port_conf_get(mesa_port_no_t port_no, vtss_appl_ipmc_lib_port_conf_t &port_conf)
{
    vtss_appl_ipmc_lib_key_t key;
    mesa_rc                  rc;

    key.is_mvr  = true;
    key.is_ipv4 = true; // Doesn't matter, since IGMP and MLD conf are identical

    if ((rc = vtss_appl_ipmc_lib_port_conf_get(key, port_no, &port_conf)) != VTSS_RC_OK) {
        T_EG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_port_conf_get(%s, %u) failed: %s", key, port_no, error_txt(rc));
        goto default_conf_get;
    }

    return;

default_conf_get:
    if ((rc = vtss_appl_ipmc_lib_port_conf_default_get(key, &port_conf)) != VTSS_RC_OK) {
        T_EG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_port_conf_default_get() failed: %s", error_txt(rc));
        vtss_clear(port_conf);
    }
}

/******************************************************************************/
// MVR_WEB_port_conf_set()
/******************************************************************************/
static void MVR_WEB_port_conf_set(mesa_port_no_t port_no, const vtss_appl_ipmc_lib_port_conf_t &port_conf)
{
    mesa_rc rc;

    if ((rc = vtss_appl_mvr_port_conf_set(port_no, &port_conf)) != VTSS_RC_OK) {
        T_EG(MVR_TRACE_GRP_WEB, "vtss_appl_mvr_port_conf_set(%u) failed: %s", port_no, error_txt(rc));
    }
}

/******************************************************************************/
// MVR_WEB_vlan_port_conf_default_get()
/******************************************************************************/
static void MVR_WEB_vlan_port_conf_default_get(vtss_appl_ipmc_lib_vlan_port_conf_t &vlan_port_conf_default)
{
    vtss_appl_ipmc_lib_key_t key;
    mesa_rc                  rc;

    key.is_mvr  = true;
    key.is_ipv4 = true; // Doesn't matter, since IGMP and MLD conf are identical

    if ((rc = vtss_appl_ipmc_lib_vlan_port_conf_default_get(key, &vlan_port_conf_default)) != VTSS_RC_OK) {
        T_EG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_port_conf_default_get(%s) failed: %s", key, error_txt(rc));
        vtss_clear(vlan_port_conf_default);
    }
}

/******************************************************************************/
// MVR_WEB_vlan_port_conf_get()
/******************************************************************************/
static void MVR_WEB_vlan_port_conf_get(mesa_vid_t vid, mesa_port_no_t port_no, vtss_appl_ipmc_lib_vlan_port_conf_t &vlan_port_conf)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    mesa_rc                       rc;

    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true; // Doesn't matter, since IGMP and MLD conf are identical
    vlan_key.vid     = vid;

    if ((rc = vtss_appl_ipmc_lib_vlan_port_conf_get(vlan_key, port_no, &vlan_port_conf)) != VTSS_RC_OK) {
        // Cannot use T_EG() since functions involving a VLAN may fail if the
        // MVR VLAN is not yet created.
        T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_port_conf_get(%s, %s) failed: %s", vlan_key, port_no, error_txt(rc));
        MVR_WEB_vlan_port_conf_default_get(vlan_port_conf);
    }
}

/******************************************************************************/
// MVR_WEB_vlan_port_conf_set()
/******************************************************************************/
static void MVR_WEB_vlan_port_conf_set(mesa_vid_t vid, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_vlan_port_conf_t &vlan_port_conf)
{
    mesa_rc rc;
    if ((rc = vtss_appl_mvr_vlan_port_conf_set(vid, port_no, &vlan_port_conf)) != VTSS_RC_OK) {
        // Cannot use T_EG() since functions involving a VLAN may fail if the
        // MVR VLAN is not yet created.
        T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_mvr_vlan_port_conf_set(%u, %u) failed: %s", vid, port_no, error_txt(rc));
    }
}

/******************************************************************************/
// MVR_WEB_vlan_conf_default_get()
/******************************************************************************/
static void MVR_WEB_vlan_conf_default_get(vtss_appl_ipmc_lib_vlan_conf_t &vlan_conf_default)
{
    vtss_appl_ipmc_lib_key_t key;
    mesa_rc                  rc;

    key.is_mvr  = true;
    key.is_ipv4 = true; // Doesn't matter, since IGMP and MLD conf are identical

    if ((rc = vtss_appl_ipmc_lib_vlan_conf_default_get(key, &vlan_conf_default)) != VTSS_RC_OK) {
        // Cannot use T_EG() since functions involving a VLAN may fail if the
        // MVR VLAN is not yet created.
        T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_conf_default_get(%s) failed: %s", key, error_txt(rc));
        vtss_clear(vlan_conf_default);
    }
}

/******************************************************************************/
// MVR_WEB_vlan_conf_get()
/******************************************************************************/
static bool MVR_WEB_vlan_conf_get(mesa_vid_t vid, vtss_appl_ipmc_lib_vlan_conf_t &vlan_conf)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    mesa_rc                       rc;

    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true; // Doesn't matter, since IGMP and MLD conf are identical
    vlan_key.vid     = vid;

    if ((rc = vtss_appl_ipmc_lib_vlan_conf_get(vlan_key, &vlan_conf)) != VTSS_RC_OK) {
        // Cannot use T_EG() since functions involving a VLAN may fail if the
        // MVR VLAN is not yet created.
        T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_conf_get(%s) failed: %s", vlan_key, error_txt(rc));
        MVR_WEB_vlan_conf_default_get(vlan_conf);
        return false;
    } else {
        return true;
    }
}

/******************************************************************************/
// MVR_WEB_vlan_statistics_clear()
/******************************************************************************/
static void MVR_WEB_vlan_statistics_clear(void)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    mesa_rc                       rc;

    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true; // Doesn't matter since MVR will clear both IGMP and MLD statistics.
    vlan_key.vid     = 0;

    while (vtss_appl_ipmc_lib_vlan_itr(&vlan_key, &vlan_key, true /* stay in this key */) == VTSS_RC_OK) {
        if ((rc = vtss_appl_mvr_vlan_statistics_clear(vlan_key.vid)) != VTSS_RC_OK) {
            T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_mvr_vlan_statistics_clear(%s) failed: %s", vlan_key, error_txt(rc));
        }
    }
}

/******************************************************************************/
// MVR_WEB_vlan_status_get()
/******************************************************************************/
static void MVR_WEB_vlan_status_get(vtss_appl_ipmc_lib_vlan_key_t &vlan_key, vtss_appl_ipmc_lib_vlan_status_t &vlan_status)
{
    mesa_rc rc;

    if ((rc = vtss_appl_ipmc_lib_vlan_status_get(vlan_key, &vlan_status)) != VTSS_RC_OK) {
        T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_status_get(%s) failed: %s", vlan_key, error_txt(rc));
        vtss_clear(vlan_status);
        vlan_status.active_querier_address.is_ipv4 = vlan_key.is_ipv4;
    }
}

/******************************************************************************/
// MVR_WEB_vlan_statistics_get()
/******************************************************************************/
static void MVR_WEB_vlan_statistics_get(vtss_appl_ipmc_lib_vlan_key_t &vlan_key, vtss_appl_ipmc_lib_vlan_statistics_t &vlan_statistics)
{
    mesa_rc rc;

    if ((rc = vtss_appl_ipmc_lib_vlan_statistics_get(vlan_key, &vlan_statistics)) != VTSS_RC_OK) {
        T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_statistics_get(%s) failed: %s", vlan_key, error_txt(rc));
        vtss_clear(vlan_statistics);
    }
}

/******************************************************************************/
// MVR_WEB_config()
/******************************************************************************/
static int32_t MVR_WEB_config(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                         sid = web_retrieve_request_sid(p);  /* Includes USID = ISID */
    int                                 ct, cntr;
    vtss_appl_ipmc_lib_vlan_key_t       vlan_key;
    vtss_appl_ipmc_capabilities_t       cap;
    vtss_appl_ipmc_lib_global_conf_t    global_conf;
    vtss_appl_ipmc_lib_vlan_conf_t      vlan_conf;
    vtss_appl_ipmc_lib_vlan_port_conf_t vlan_port_conf, vlan_port_default_conf;
    vtss_appl_ipmc_lib_profile_key_t    profile_key;
    char                                new_str[10], ip_buf[IPV6_ADDR_IBUF_MAX_LEN];
    char                                encoded_name[3 * sizeof(vlan_conf.name)], encoded_profile_name[3 * sizeof(profile_key.name) + 1];
    int                                 channel_conflict;
    vtss_appl_ipmc_lib_port_conf_t      port_conf;
    uint32_t                            port_count = port_count_max();
    mesa_port_no_t                      port_no;
    mesa_vid_t                          vid;
    mesa_ipv4_t                         ipv4;
    int                                 i;
    mesa_rc                             rc;

    /* Redirect unmanaged/invalid access to handler */
    if (redirectUnmanagedOrInvalid(p, sid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MVR)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int                     var_value;
        bool                    entry_found;
        size_t                  name_len;
        char                    search_str[128];
        const char              *var_string;

        (void)vtss_appl_ipmc_capabilities_get(true,  &cap);

        if ((rc = MVR_WEB_global_conf_get(global_conf)) != VTSS_RC_OK) {
            sprintf(search_str, "Unable to get global configuration: %s", error_txt(rc));
            send_custom_error(p, "MVR Configuration Error", search_str, strlen(search_str));
            return -1;
        }

        if (cyg_httpd_form_varable_int(p, "mvr_mode", &var_value)) {
            global_conf.admin_active = var_value ? true : false;
            if ((rc = vtss_appl_mvr_global_conf_set(&global_conf)) != VTSS_RC_OK) {
                sprintf(search_str, "Unable to set global configuration: %s", error_txt(rc));
                send_custom_error(p, "MVR Configuration Error", search_str, strlen(search_str));
                return -1;
            }
        }

        MVR_WEB_vlan_port_conf_default_get(vlan_port_default_conf);

        for (i = 0; i < 2; i++) {
            // i == 0 <=> changed or deleted entries. cntr starts from 0 and is used selected places.
            // i == 1 <=> new entries.                cntr starts from 1 and is used most places
            sprintf(new_str, "%s", i == 0 ? "" : "new_");

            for (cntr = 1; cntr <= cap.vlan_cnt_max; cntr++) {
                sprintf(search_str, "%smvid_mvr_vlan_%d", new_str, i == 0 ? cntr - 1 : cntr);
                if (!cyg_httpd_form_varable_int(p, search_str, &var_value) || !var_value) {
                    continue;
                }

                vid         = (mesa_vid_t)(var_value & 0xFFFF);
                entry_found = MVR_WEB_vlan_conf_get(vid, vlan_conf);

                if (i == 0) {
                    // Deleting an entry?
                    sprintf(search_str, "delete_mvr_vlan_%u", vid);
                    if (cyg_httpd_form_varable_find(p, search_str)) {
                        /* "Delete" checked */
                        if (entry_found) {
                            if ((rc  = vtss_appl_mvr_vlan_conf_del(vid)) != VTSS_RC_OK) {
                                sprintf(search_str, "Unable to delete MVR VLAN %u: %s", vid, error_txt(rc));
                                send_custom_error(p, "MVR Interface Configuration Error", search_str, strlen(search_str));
                                return -1;
                            }
                        }

                        continue;
                    }
                }

                sprintf(search_str, "%sname_mvr_vlan_%d", new_str, i == 0 ? vid : cntr);
                var_string = cyg_httpd_form_varable_string(p, search_str, &name_len);
                vlan_conf.name[0] = '\0';
                if (name_len > 0) {
                    (void)cgi_unescape(var_string, vlan_conf.name, name_len, sizeof(vlan_conf.name));
                }

                vlan_conf.querier_enable = cyg_httpd_form_variable_check_fmt(p, "%selection_mvr_vlan_%d", new_str, i == 0 ? vid : cntr);

                sprintf(search_str, "%sadrs_mvr_vlan_%d", new_str, i == 0 ? vid : cntr);
                if (!cyg_httpd_form_varable_ipv4(p, search_str, &ipv4)) {
                    sprintf(search_str, "Invalid IGMP Address (used for MVR VLAN %u)!!!", vid);
                    send_custom_error(p, "MVR Interface Configuration Error", search_str, strlen(search_str));
                    return -1;
                }

                vlan_conf.querier_address.is_ipv4 = true;
                vlan_conf.querier_address.ipv4    = ipv4;

                sprintf(search_str, "%sprofile_mvr_vlan_%d", i == 0 ? "ref_" : "new_", i == 0 ? vid : cntr);
                var_string = cyg_httpd_form_varable_string(p, search_str, &name_len);
                vlan_conf.channel_profile.name[0] = '\0';
                if (name_len > 0) {
                    (void)cgi_unescape(var_string, vlan_conf.channel_profile.name, name_len, sizeof(vlan_conf.channel_profile.name));
                }

                sprintf(search_str, "%svmode_mvr_vlan_%d", new_str, i == 0 ? vid : cntr);
                if (cyg_httpd_form_varable_int(p, search_str, &var_value)) {
                    vlan_conf.compatible_mode = var_value != 0;
                }

                sprintf(search_str, "%svtag_mvr_vlan_%d", new_str, i == 0 ? vid : cntr);
                if (cyg_httpd_form_varable_int(p, search_str, &var_value)) {
                    vlan_conf.tx_tagged = var_value != 0;
                }

                sprintf(search_str, "%svpri_mvr_vlan_%d", new_str, i == 0 ? vid : cntr);
                if (cyg_httpd_form_varable_int(p, search_str, &var_value)) {
                    vlan_conf.pcp = var_value;
                }

                sprintf(search_str, "%svllqi_mvr_vlan_%d", new_str, i == 0 ? vid : cntr);
                if (cyg_httpd_form_varable_int(p, search_str, &var_value)) {
                    vlan_conf.lmqi = var_value;
                }

                if ((rc = vtss_appl_mvr_vlan_conf_set(vid, &vlan_conf)) != VTSS_RC_OK) {
                    sprintf(search_str, "Unable to set configuration for MVR VLAN %u: %s", vid, error_txt(rc));
                    send_custom_error(p, "MVR Interface Configuration Error", search_str, strlen(search_str));
                    return -1;  // Do not further search the file system.
                }

                for (port_no = 0; port_no < port_count; port_no++) {
                    var_value = 0;
                    sprintf(search_str, "%sprole_%d_mvr_vlan_%d", new_str, port_no, i == 0 ? cntr - 1 /* RBNTBD. Shouldn't it be vid? */ : cntr);
                    (void)cyg_httpd_form_varable_int(p, search_str, &var_value);

                    MVR_WEB_vlan_port_conf_get(vid, port_no, vlan_port_conf);
                    if (vlan_port_conf.role != var_value) {
                        vlan_port_conf.role = (vtss_appl_ipmc_lib_port_role_t)var_value;
                        MVR_WEB_vlan_port_conf_set(vid, port_no, vlan_port_conf);
                    }
                }
            }
        }

        for (port_no = 0; port_no < port_count; port_no++) {
            var_value = 0;
            sprintf(search_str, "fastleave_port_%d", iport2uport(port_no));
            (void)cyg_httpd_form_varable_int(p, search_str, &var_value);

            MVR_WEB_port_conf_get(port_no, port_conf);
            if (port_conf.fast_leave != var_value) {
                port_conf.fast_leave = var_value;
                MVR_WEB_port_conf_set(port_no, port_conf);
            }
        }

        redirect(p, "/mvr.htm");
    } else {
        // Format:
        //   [mvr_mode];[channel_conflict];[profile_1]|...|[profile_n];[pmode]|...
        //   ;[vid]/[name]/[election]/[vmode]/[tx_tagged]/[vpri]/[vllqi]/[profile]/[igmp_adrs],[prole]/...|...
        (void)cyg_httpd_start_chunked("html");

        (void)MVR_WEB_global_conf_get(global_conf);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u", global_conf.admin_active);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        // RBNTBD: Show operational warnings rather than the old channel_conflict.
        channel_conflict = 0;
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";%d", channel_conflict);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        // Send all existing IPMC profile names
        cntr = 0;
        vtss_clear(profile_key);
        while (vtss_appl_ipmc_lib_profile_itr(&profile_key, &profile_key) == VTSS_RC_OK) {
            (void)cgi_escape(profile_key.name, encoded_profile_name);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), cntr ? "|%s" : ";%s", encoded_profile_name);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            cntr++;
        }

        if (!cntr) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        for (port_no = 0; port_no < port_count; port_no++) {
            MVR_WEB_port_conf_get(port_no, port_conf);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), port_no == 0 ? ";%u" :  "|%u", port_conf.fast_leave);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cntr = 0;
        vlan_key.is_mvr  = true;
        vlan_key.is_ipv4 = true; // Doesn't matter since both IGMP and MLD VLANs are created at the same time
        vlan_key.vid     = 0;
        while (vtss_appl_ipmc_lib_vlan_itr(&vlan_key, &vlan_key, true /* stay in this key */) == VTSS_RC_OK) {
            (void)MVR_WEB_vlan_conf_get(vlan_key.vid, vlan_conf);
            (void)cgi_escape(vlan_conf.name, encoded_name);
            (void)cgi_escape(vlan_conf.channel_profile.name, encoded_profile_name);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          (cntr > 0) ? "|%u/%s/%d/%d/%d/%u/%u/%s/%s" : ";%u/%s/%d/%d/%d/%u/%u/%s/%s",
                          vlan_key.vid,
                          encoded_name,
                          vlan_conf.querier_enable,
                          vlan_conf.compatible_mode,
                          vlan_conf.tx_tagged,
                          vlan_conf.pcp,
                          vlan_conf.lmqi,
                          encoded_profile_name,
                          vlan_conf.querier_address.print(ip_buf));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            for (port_no = 0; port_no < port_count; port_no++) {
                MVR_WEB_vlan_port_conf_get(vlan_key.vid, port_no, vlan_port_conf);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), (port_no == 0) ? ",%d" : "/%d", vlan_port_conf.role);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            cntr++;
        }

        if (!cntr) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";NoEntries");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        (void)cyg_httpd_end_chunked();
    }

    return -1;  // Do not further search the file system.
}

/******************************************************************************/
// MVR_WEB_vlan_status()
/******************************************************************************/
static int32_t MVR_WEB_vlan_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                          sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ipmc_lib_vlan_key_t        vlan_key;
    vtss_appl_ipmc_lib_vlan_conf_t       vlan_conf;
    vtss_appl_ipmc_lib_vlan_status_t     vlan_status;
    vtss_appl_ipmc_lib_vlan_statistics_t vlan_statistics;
    int                                  ct, cntr;

    /* Redirect unmanaged/invalid access to handler */
    if (redirectUnmanagedOrInvalid(p, sid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR)) {
        return -1;
    }
#endif

    /* Clear? */
    if ((cyg_httpd_form_varable_find(p, "clear") != NULL)) {
        MVR_WEB_vlan_statistics_clear();
    }

    //  Format:
    //  [vid],[name],[querier_state];
    //  [rx_igmp_query],[tx_igmp_query],
    //  [rx_igmpv1_joins],[rx_igmpv2_joins],[rx_igmpv3_joins],[rx_igmpv2_leaves];
    //  [rx_mld_query],[tx_mld_query],
    //  [rx_mldv1_reports],[rx_mldv2_reports],[rx_mldv1_dones]|...
    (void)cyg_httpd_start_chunked("html");

    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true;
    vlan_key.vid     = 0;
    cntr = 0;
    while (vtss_appl_ipmc_lib_vlan_itr(&vlan_key, &vlan_key, true /* stay in this key */) == VTSS_RC_OK) {
        // When we get here, it's the IGMP MVR VLAN we have.
        (void)MVR_WEB_vlan_conf_get(vlan_key.vid, vlan_conf);
        MVR_WEB_vlan_status_get(vlan_key, vlan_status);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u,%s,%d",
                      cntr ? "|" : "",
                      vlan_key.vid,
                      vlan_conf.name,
                      vlan_status.querier_state);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        MVR_WEB_vlan_statistics_get(vlan_key, vlan_statistics);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      ";%u,%u,%u,%u,%u,%u",
                      vlan_statistics.rx_query,
                      vlan_statistics.tx_query,
                      vlan_statistics.rx.igmp.utilized.v1_report,
                      vlan_statistics.rx.igmp.utilized.v2_report,
                      vlan_statistics.rx.igmp.utilized.v3_report,
                      vlan_statistics.rx.igmp.utilized.v2_leave);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        vlan_key.is_ipv4 = false;
        MVR_WEB_vlan_statistics_get(vlan_key, vlan_statistics);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      ";%u,%u,%u,%u,%u",
                      vlan_statistics.rx_query,
                      vlan_statistics.tx_query,
                      vlan_statistics.rx.mld.utilized.v1_report,
                      vlan_statistics.rx.mld.utilized.v2_report,
                      vlan_statistics.rx.mld.utilized.v1_done);

        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        // Back to searching for IPv4 MVR VLANs.
        vlan_key.is_ipv4 = true;
        cntr++;
    }

    if (!cntr) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "NoEntries|");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/******************************************************************************/
// MVR_WEB_grp_status()
/******************************************************************************/
static int32_t MVR_WEB_grp_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ipmc_lib_vlan_key_t   vlan_key;
    vtss_appl_ipmc_lib_grp_status_t grp_status;
    int                             i, ct, entry_cnt = 0, entry_cnt_max = 0;
    mesa_port_no_t                  port_no;
    uint32_t                        port_cnt = port_count_max();
    vtss_appl_ipmc_lib_ip_t         grp_addr = {};
    mesa_vid_t                      vid = 1, prev_vid_in_itr;
    char                            ip_buf[IPV6_ADDR_IBUF_MAX_LEN];
    const char                      *var_string;
    bool                            first, dyn_get_next_entry;
    mesa_rc                         rc;

    /* Redirect unmanaged/invalid access to handler */
    if (redirectUnmanagedOrInvalid(p, sid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR)) {
        return -1;
    }
#endif

    (void)cyg_httpd_start_chunked("html");

    // Get number of entries per page
    if ((var_string = cyg_httpd_form_varable_find(p, "DynNumberOfEntries")) != NULL) {
        entry_cnt_max = atoi(var_string);
    }

    if (entry_cnt_max <= 0 || entry_cnt_max > 99) {
        entry_cnt_max = 20;
    }

    // Get start VID
    if ((var_string = cyg_httpd_form_varable_find(p, "DynStartVid")) != NULL) {
        vid = atoi(var_string);
        if (vid < 1) {
            vid = 1;
        }
    }

    // Get start group address
    if (!cyg_httpd_form_varable_ipv6(p, "DynStartGroup", &grp_addr.ipv6)) {
        grp_addr.is_ipv4 = true;

        if (!cyg_httpd_form_varable_ipv4(p, "DynStartGroup", &grp_addr.ipv4)) {
            grp_addr.all_zeros_set();
        }
    } else {
        grp_addr.is_ipv4 = false;
    }

    // Get or GetNext
    dyn_get_next_entry = false;
    if ((var_string = cyg_httpd_form_varable_find(p, "GetNextEntry")) != NULL) {
        dyn_get_next_entry = atoi(var_string) != 0;
    }

    //    Format:
    //    <startVid>|<startGroup>|<NumberOfEntries>|<portCnt>|[vid],[groups],[p1_is_mbr],[p2_is_mbr],[p3_is_mbr],.../...
    if (!dyn_get_next_entry) {
        // Not get next button
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|%d|%u|",
                      vid,
                      grp_addr.print(ip_buf),
                      entry_cnt_max,
                      port_cnt);

        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        // We also need to subract one from the current IP address in order to
        // not starting with the next.
        --grp_addr; // Using vtss_appl_ipmc_ip_lib_t::operator--() prefix operator
    }

    T_IG(MVR_TRACE_GRP_WEB, "Starting from VID = %u and addr = %s", vid, grp_addr);

    // Unfortunately, the group map is sorted first by IPMC/MVR, then by IP
    // family, then by VLAN ID, and finally by group address. Since this web
    // page needs it by IPMC/MVR, then by VLAN ID, then by IP family, and
    // finally by IP address, we need to do some tricks, where we first get the
    // next MVR VLAN ID, and then iterate first across IPv4 groups and then IPv6
    // groups.
    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true;
    vlan_key.vid     = vid == 0 ? vid : vid - 1; // Gotta start with previous VLAN ID.
    first            = true;
    while (vtss_appl_ipmc_lib_vlan_itr(&vlan_key, &vlan_key, true /* stay in this key.is_mvr and key.is_ipv4 */) == VTSS_RC_OK) {
        prev_vid_in_itr = vlan_key.vid;

        // Get all IPv4 and all IPv6 groups for this VLAN unless this is the
        // first time we iterate and we were requested to start with an IPv6
        // address.
        for (i = (first && !grp_addr.is_ipv4) ? 1 : 0; i < 2; i++) {
            // i == 0 <=> IPv4
            // i == 1 <=> IPv6.

            first = false;
            vlan_key.is_ipv4 = i == 0;
            grp_addr.is_ipv4 = i == 0;
            while (vtss_appl_ipmc_lib_grp_itr(&vlan_key, &vlan_key, &grp_addr, &grp_addr, true /* stay in this key.is_mvr and key.is_ipv4 */) == VTSS_RC_OK) {
                if (vlan_key.vid != prev_vid_in_itr) {
                    // The iterator iterates past VID boundaries.
                    break;
                }

                if ((rc = vtss_appl_ipmc_lib_grp_status_get(vlan_key, &grp_addr, &grp_status)) != VTSS_RC_OK) {
                    // Don't use T_EG(), because it may be deleted between the
                    // itr() and the get()
                    T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_grp_status_get(%s, %s) failed: %s", vlan_key, grp_addr, error_txt(rc));
                    continue;
                }

                if (++entry_cnt > entry_cnt_max) {
                    break;
                }

                if (dyn_get_next_entry && entry_cnt == 1) { /* Only for GetNext button */
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|%d|%u|",
                                  vlan_key.vid,
                                  grp_addr.print(ip_buf),
                                  entry_cnt_max,
                                  port_cnt);

                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%s", vlan_key.vid, grp_addr.print(ip_buf));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                for (port_no = 0; port_no < port_cnt; port_no++) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",%u", (uint32_t)grp_status.port_list[port_no]);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                (void)cyg_httpd_write_chunked("/", 1);
            }

            if (entry_cnt > entry_cnt_max) {
                break;
            }

            // Back to the VID we are iterating across, either to do the same
            // for IPv6 addresses or to get the next MVR VLAN.
            vlan_key.vid = prev_vid_in_itr;
        }

        if (entry_cnt > entry_cnt_max) {
            break;
        }

        // Get next IPv4 MVR VLAN (IPv4 and IPv6 VLANs (also) go hand-in-hand in
        // MVR).
        vlan_key.is_ipv4 = true;
    }

    if (entry_cnt == 0) { /* No entry existing */
        if (dyn_get_next_entry) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|%d|%u|",
                          vlan_key.vid,
                          grp_addr.print(ip_buf),
                          entry_cnt_max,
                          port_cnt);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "NoEntries");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/******************************************************************************/
// MVR_WEB_src_status()
/******************************************************************************/
static int32_t MVR_WEB_src_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ipmc_lib_vlan_key_t   vlan_key;
    vtss_appl_ipmc_lib_src_status_t src_status;
    vtss_appl_ipmc_lib_ip_t         grp_addr = {}, src_addr = {}, prev_grp_addr = {};
    int                             i, ct = 0, entry_cnt = 0, entry_cnt_max = 0;
    mesa_port_no_t                  port_no, prev_port_no = 0;
    mesa_vid_t                      vid = 1, prev_vid_in_itr = 0, prev_vid = 0;
    char                            ip_buf1[IPV6_ADDR_IBUF_MAX_LEN], ip_buf2[IPV6_ADDR_IBUF_MAX_LEN];
    bool                            first, same_grp_as_before, dyn_get_next_entry;
    const char                      *var_string;
    mesa_rc                         rc;

    /* Redirect unmanaged/invalid access to handler */
    if (redirectUnmanagedOrInvalid(p, sid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MVR)) {
        return -1;
    }
#endif

    (void)cyg_httpd_start_chunked("html");

    // Get number of entries per page
    entry_cnt_max = 0;
    if ((var_string = cyg_httpd_form_varable_find(p, "DynNumberOfEntries")) != NULL) {
        entry_cnt_max = atoi(var_string);
    }

    if (entry_cnt_max <= 0 || entry_cnt_max > 99) {
        entry_cnt_max = 20;
    }

    // Get start vid
    if ((var_string = cyg_httpd_form_varable_find(p, "DynStartVid")) != NULL) {
        vid = atoi(var_string);
    }

    // Get start group address
    if (!cyg_httpd_form_varable_ipv6(p, "DynStartGroup", &grp_addr.ipv6)) {
        grp_addr.is_ipv4 = true;

        if (!cyg_httpd_form_varable_ipv4(p, "DynStartGroup", &grp_addr.ipv4)) {
            grp_addr.all_zeros_set();
        }
    } else {
        grp_addr.is_ipv4 = false;
    }

    // Get or GetNext
    dyn_get_next_entry = false;
    if ((var_string = cyg_httpd_form_varable_find(p, "GetNextEntry")) != NULL) {
        dyn_get_next_entry = atoi(var_string) != 0;
    }

    // Format:
    //  <start_vid>|<start_group>|<entry_cnt_max>;[vid]/[group]/[port]/[mode]/[source_addr]/[type]/[in_hw]|...
    if (!dyn_get_next_entry) {
        // Not get next button
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|%u", vid, grp_addr.print(ip_buf1), entry_cnt_max);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        // We also need to subract one from the current IP address in order to
        // not starting with the next.
        --grp_addr; // Using vtss_appl_ipmc_lib_ip_t::operator--() prefix operator
    }

    T_IG(MVR_TRACE_GRP_WEB, "Starting from VID = %u and addr = %s", vid, grp_addr);

    // Unfortunately, the group map is sorted first by IPMC/MVR, then by IP
    // family, then by VLAN ID, and finally by group address. Since this web
    // page needs it by IPMC/MVR, then by VLAN ID, then by IP family, and
    // finally by IP address, we need to do some tricks, where we first get the
    // next MVR VLAN ID, and then iterate first across IPv4 groups and then IPv6
    // groups.
    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true;
    vlan_key.vid     = vid == 0 ? vid : vid - 1; // Gotta start with previous VLAN ID.
    first            = true;
    while (vtss_appl_ipmc_lib_vlan_itr(&vlan_key, &vlan_key, true /* stay in this key.is_mvr and key.is_ipv4 */) == VTSS_RC_OK) {
        prev_vid_in_itr = vlan_key.vid;

        // Get all IPv4 and all IPv6 groups for this VLAN unless this is the
        // first time we iterate and we were requested to start with an IPv6
        // address.
        for (i = (first && !grp_addr.is_ipv4) ? 1 : 0; i < 2; i++) {
            // i == 0 <=> IPv4
            // i == 1 <=> IPv6.

            first = false;
            vlan_key.is_ipv4 = i == 0;
            grp_addr.is_ipv4 = i == 0;
            while (vtss_appl_ipmc_lib_src_itr(&vlan_key, &vlan_key, &grp_addr, &grp_addr, &port_no, &port_no, &src_addr, &src_addr, true /* stay in this key.is_mvr and key.is_ipv4 */) == VTSS_RC_OK) {
                if (vlan_key.vid != prev_vid_in_itr) {
                    // The iterator iterates past VID boundaries.
                    break;
                }

                if ((rc = vtss_appl_ipmc_lib_src_status_get(vlan_key, &grp_addr, port_no, &src_addr, &src_status)) != VTSS_RC_OK) {
                    // Don't use T_EG(), because it may be deleted between the
                    // itr() and the get()
                    T_IG(MVR_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_src_status_get(%s, %s, %u, %s) failed: %s", vlan_key, grp_addr, port_no, src_addr, error_txt(rc));
                    continue;
                }

                if (++entry_cnt > entry_cnt_max) {
                    break;
                }

                if (dyn_get_next_entry && entry_cnt == 1) { /* Only for GetNext button */
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|%u", vlan_key.vid, grp_addr.print(ip_buf1), entry_cnt_max);
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }

                if (src_addr.is_all_ones()) {
                    same_grp_as_before = vlan_key.vid == prev_vid && grp_addr == prev_grp_addr && port_no == prev_port_no;

                    if (same_grp_as_before) {
                        strcpy(ip_buf2, "<Other>");
                    } else {
                        strcpy(ip_buf2, "<Any>");
                    }
                } else {
                    // Print the source address
                    (void)src_addr.print(ip_buf2);
                }

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%s/%u/%d/%s/%d/%d",
                              entry_cnt > 1 ? "|" : ";",
                              vlan_key.vid,
                              grp_addr.print(ip_buf1),
                              iport2uport(port_no),
                              src_status.filter_mode,
                              ip_buf2,
                              src_status.forwarding,
                              src_status.hw_location != VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE);

                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            if (entry_cnt > entry_cnt_max) {
                break;
            }

            // Back to the VID we are iterating across, either to do the same
            // for IPv6 addresses or to get the next MVR VLAN.
            vlan_key.vid = prev_vid_in_itr;
        }

        if (entry_cnt > entry_cnt_max) {
            break;
        }

        // Get next IPv4 MVR VLAN (IPv4 and IPv6 VLANs (also) go hand-in-hand in
        // MVR).
        vlan_key.is_ipv4 = true;
    }

    if (entry_cnt == 0) {
        // No entries
        if (dyn_get_next_entry) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|%u", vlan_key.vid, grp_addr.print(ip_buf1), entry_cnt_max);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";NoEntries");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/******************************************************************************/
// MVR_WEB_js_config()
/******************************************************************************/
static size_t MVR_WEB_js_config(char **base_ptr, char **cur_ptr, size_t *length)
{
    vtss_appl_ipmc_capabilities_t cap;
    char                          buff[MVR_WEB_BUF_LEN];

    (void)vtss_appl_ipmc_capabilities_get(true, &cap);

    (void)snprintf(buff, MVR_WEB_BUF_LEN,
                   "var configMvrVlanMax = %u;\n"
                   "var configMvrVlanNameLen = %d;\n"
                   "var configMvrCharAsciiMin = %d;\n"
                   "var configMvrCharAsciiMax = %d;\n",
                   cap.vlan_cnt_max,
                   VTSS_APPL_IPMC_LIB_VLAN_NAME_LEN_MAX,
                   33,
                   126);

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

web_lib_config_js_tab_entry(MVR_WEB_js_config);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_mvr,           "/config/mvr",           MVR_WEB_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_mvr_status,      "/stat/mvr_status",      MVR_WEB_vlan_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_mvr_groups_info, "/stat/mvr_groups_info", MVR_WEB_grp_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_mvr_groups_sfm,  "/stat/mvr_groups_sfm",  MVR_WEB_src_status);

