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

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif /* VTSS_SW_OPTION_PRIV_LVL */

#include "ipmc_lib_trace.h"
#include <vtss/appl/ipmc_lib.h>

/******************************************************************************/
// IPMC_LIB_PROFILE_web_range_conf()
/******************************************************************************/
static int32_t IPMC_LIB_PROFILE_web_range_conf(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                               sid = web_retrieve_request_sid(p);
    vtss_appl_ipmc_lib_profile_capabilities_t cap;
    vtss_appl_ipmc_lib_profile_range_key_t    range_key;
    vtss_appl_ipmc_lib_profile_range_conf_t   range_conf;
    char                                      new_str[10];
    char                                      bgn_buf[IPV6_ADDR_IBUF_MAX_LEN], end_buf[IPV6_ADDR_IBUF_MAX_LEN];
    char                                      encoded_string[3 * sizeof(range_key.name)];
    int                                       i, cntr, ct, display_cnt;
    size_t                                    var_len;
    const char                                *var_string;
    bool                                      entry_found;
    char                                      search_str[65];
    mesa_rc                                   rc;

    if (!p || redirectUnmanagedOrInvalid(p, sid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC_LIB)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void)vtss_appl_ipmc_lib_profile_capabilities_get(&cap);

        for (i = 0 ; i < 2; i++) {
            // i == 0 <=> changed or deleted entries. cntr starts from 0
            // i == 1 <=> new entries.                cntr starts from 1
            sprintf(new_str, "%s", i == 0 ? "" : "new_");

            for (cntr = 1; cntr <= cap.range_cnt_max; cntr++) {
                sprintf(search_str, "%sidx_ipmcpf_entry_%d", new_str, i ==  0 ? cntr - 1 : cntr);
                var_string = cyg_httpd_form_varable_string(p, search_str, &var_len);
                if (var_len == 0) {
                    continue;
                }

                (void)cgi_unescape(var_string, range_key.name, var_len, sizeof(range_key.name));
                if (range_key.name[0] == '\0') {
                    continue;
                }

                entry_found = vtss_appl_ipmc_lib_profile_range_conf_get(&range_key, &range_conf) == VTSS_RC_OK;
                if (!entry_found) {
                    vtss_clear(range_conf);
                }

                if (i == 0) {
                    // Deleting an entry?
                    sprintf(search_str, "delete_ipmcpf_entry_%d", cntr - 1);
                    if (cyg_httpd_form_varable_find(p, search_str)) {
                        // "Delete" checked
                        if (entry_found) {
                            if ((rc = vtss_appl_ipmc_lib_profile_range_conf_del(&range_key)) != VTSS_RC_OK) {
                                sprintf(search_str, "Unable to delete address range %s: %s", range_key.name, error_txt(rc));
                                send_custom_error(p, "IPMC Profile Address Configuration Error", search_str, strlen(search_str));
                                return -1;
                            }
                        }

                        continue;
                    }
                }

                sprintf(search_str, "%sbgn_ipmcpf_entry_%d", new_str, i == 0 ? cntr - 1 : cntr);
                if (cyg_httpd_form_varable_ipv6(p, search_str, &range_conf.start.ipv6)) {
                    range_conf.start.is_ipv4 = false;
                    range_conf.end.is_ipv4   = false;
                    sprintf(search_str, "%send_ipmcpf_entry_%d", new_str, i == 0 ? cntr - 1 : cntr);
                    if (!cyg_httpd_form_varable_ipv6(p, search_str, &range_conf.end.ipv6)) {
                        // Send error?
                        continue;
                    }
                } else if (cyg_httpd_form_varable_ipv4(p, search_str, &range_conf.start.ipv4)) {
                    range_conf.start.is_ipv4 = true;
                    range_conf.end.is_ipv4   = true;
                    sprintf(search_str, "%send_ipmcpf_entry_%d", new_str, i == 0 ? cntr - 1 : cntr);
                    if (!cyg_httpd_form_varable_ipv4(p, search_str, &range_conf.end.ipv4)) {
                        // Send error?
                        continue;
                    }
                } else {
                    // Send error?
                    continue;
                }

                if ((rc = vtss_appl_ipmc_lib_profile_range_conf_set(&range_key, &range_conf)) != VTSS_RC_OK) {
                    sprintf(search_str, "Unable to set address range %s: %s", range_key.name, error_txt(rc));
                    send_custom_error(p, "IPMC Profile Address Configuration Error", search_str, strlen(search_str));
                    return -1;
                }
            }
        }

        if ((var_string = cyg_httpd_form_varable_find(p, "NumberOfEntries")) != NULL) {
            cntr = atoi(var_string);
        } else {
            cntr = 20;
        }

        sprintf(search_str, "/ipmc_lib_range_table.htm?&DynDisplayNum=%d&DynChannelGrp=", cntr);
        redirect(p, search_str);
    } else {
        cntr        = 0;
        display_cnt = 0;
        if ((var_string = cyg_httpd_form_varable_find(p, "DynDisplayNum")) != NULL) {
            display_cnt = atoi(var_string);
        }

        var_string = cyg_httpd_form_varable_string(p, "DynChannelGrp", &var_len);
        if (var_len > 0) {
            // We start iteration with the next of this range
            (void)cgi_unescape(var_string, range_key.name, var_len, sizeof(range_key.name));
        } else {
            // Start over.
            vtss_clear(range_key);
        }

        // Format:
        //   [range_name]/[start_addr]/[end_addr]|...|
        (void)cyg_httpd_start_chunked("html");

        while (vtss_appl_ipmc_lib_profile_range_itr(&range_key, &range_key) == VTSS_RC_OK) {
            if ((rc = vtss_appl_ipmc_lib_profile_range_conf_get(&range_key, &range_conf)) != VTSS_RC_OK) {
                T_EG(IPMC_LIB_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_profile_range_conf_get(%s) failed: %s", range_key.name, error_txt(rc));
                continue;
            }

            (void)cgi_escape(range_key.name, encoded_string);

            if (range_conf.start.is_ipv4) {
                (void)misc_ipv4_txt(range_conf.start.ipv4, bgn_buf);
                (void)misc_ipv4_txt(range_conf.end.ipv4,   end_buf);
            } else {
                (void)misc_ipv6_txt(&range_conf.start.ipv6, bgn_buf);
                (void)misc_ipv6_txt(&range_conf.end.ipv6,   end_buf);
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s/%s/%s", cntr ? "|" : "", encoded_string, bgn_buf, end_buf);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            cntr++;

            if (display_cnt && (cntr >= display_cnt)) {
                break;
            }
        }

        if (!cntr) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "NoEntries|");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/******************************************************************************/
// IPMC_LIB_PROFILE_web_profile_conf()
/******************************************************************************/
static int32_t IPMC_LIB_PROFILE_web_profile_conf(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                               sid = web_retrieve_request_sid(p);
    vtss_appl_ipmc_lib_profile_capabilities_t cap;
    vtss_appl_ipmc_lib_profile_global_conf_t  global_conf;
    vtss_appl_ipmc_lib_profile_key_t          profile_key;
    vtss_appl_ipmc_lib_profile_conf_t         profile_conf;
    char                                      new_str[10];
    char                                      encoded_string_name[3 * sizeof(profile_key.name)], encoded_string_dscr[3 * sizeof(profile_conf.dscr)];
    int                                       i, cntr, ct;
    bool                                      entry_found;
    size_t                                    var_len;
    const char                                *var_string;
    char                                      search_str[65];
    mesa_rc                                   rc;

    if (!p || redirectUnmanagedOrInvalid(p, sid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC_LIB)) {
        return -1;
    }
#endif

    if ((rc = vtss_appl_ipmc_lib_profile_global_conf_get(&global_conf)) != VTSS_RC_OK) {
        T_E("vtss_appl_ipmc_lib_profile_global_conf_get() failed: %s", error_txt(rc));
        vtss_clear(global_conf);
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void)vtss_appl_ipmc_lib_profile_capabilities_get(&cap);

        if (cyg_httpd_form_varable_int(p, "profile_mode", &ct)) {
            global_conf.enable = ct ? true : false;
            if ((rc = vtss_appl_ipmc_lib_profile_global_conf_set(&global_conf)) != VTSS_RC_OK) {
                T_E("vtss_appl_ipmc_lib_profile_global_conf_set(%d) failed: %s", global_conf.enable, error_txt(rc));
                return -1;
            }
        }

        for (i = 0 ; i < 2; i++) {
            // i == 0 <=> changed or deleted entries. cntr starts from 0
            // i == 1 <=> new entries.                cntr starts from 1
            sprintf(new_str, "%s", i == 0 ? "" : "new_");

            for (cntr = 1; cntr <= cap.profile_cnt_max; cntr++) {
                sprintf(search_str, "%sidx_ipmcpf_profile_%d", new_str, i == 0 ? cntr - 1 : cntr);
                var_string = cyg_httpd_form_varable_string(p, search_str, &var_len);
                if (var_len == 0) {
                    continue;
                }

                (void)cgi_unescape(var_string, profile_key.name, var_len, sizeof(profile_key.name));
                if (profile_key.name[0] == '\0') {
                    continue;
                }

                entry_found = vtss_appl_ipmc_lib_profile_conf_get(&profile_key, &profile_conf) == VTSS_RC_OK;

                if (i == 0) {
                    // Deleting an entry?
                    sprintf(search_str, "delete_ipmcpf_profile_%d", cntr - 1);
                    if (cyg_httpd_form_varable_find(p, search_str)) {
                        // "Delete" checked
                        if (entry_found) {
                            if ((rc = vtss_appl_ipmc_lib_profile_conf_del(&profile_key)) != VTSS_RC_OK) {
                                sprintf(search_str, "Unable to delete profile %s: %s", profile_key.name, error_txt(rc));
                                send_custom_error(p, "IPMC Profile Configuration Error", search_str, strlen(search_str));
                                return -1;
                            }
                        }

                        continue;
                    }
                }

                sprintf(search_str, "%sdesc_ipmcpf_profile_%d", new_str, i == 0  ? cntr - 1 : cntr);
                var_string = cyg_httpd_form_varable_string(p, search_str, &var_len);
                if (var_len > 0) {
                    (void)cgi_unescape(var_string, profile_conf.dscr, var_len, sizeof(profile_conf.dscr));
                } else {
                    vtss_clear(profile_conf.dscr);
                }

                if ((rc = vtss_appl_ipmc_lib_profile_conf_set(&profile_key, &profile_conf)) != VTSS_RC_OK) {
                    sprintf(search_str, "Unable to set profile %s: %s", profile_key.name, error_txt(rc));
                    send_custom_error(p, "IPMC Profile Configuration Error", search_str, strlen(search_str));
                    return -1;
                }
            }
        }

        redirect(p, "/ipmc_lib_profile_table.htm");
    } else {
        // Format:
        //   [globally_enabled];[profile_name]/[description]|...;
        (void)cyg_httpd_start_chunked("html");

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u;", global_conf.enable);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        cntr = 0;
        vtss_clear(profile_key);
        while (vtss_appl_ipmc_lib_profile_itr(&profile_key, &profile_key) == VTSS_RC_OK) {
            if ((rc = vtss_appl_ipmc_lib_profile_conf_get(&profile_key, &profile_conf)) != VTSS_RC_OK) {
                T_EG(IPMC_LIB_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_profile_conf_get(%s) failed: %s", profile_key.name, error_txt(rc));
                continue;
            }

            (void)cgi_escape(profile_key.name,  encoded_string_name);
            (void)cgi_escape(profile_conf.dscr, encoded_string_dscr);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s/%s", cntr ? "|" : "", encoded_string_name, encoded_string_dscr);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            cntr++;
        }

        if (!cntr) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "NoEntries;");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1;  // Do not further search the file system.
}

/******************************************************************************/
// IPMC_LIB_PROFILE_web_rule_conf()
/******************************************************************************/
static int32_t IPMC_LIB_PROFILE_web_rule_conf(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                               sid = web_retrieve_request_sid(p);  /* Includes USID = ISID */
    vtss_appl_ipmc_lib_profile_capabilities_t cap;
    vtss_appl_ipmc_lib_profile_key_t          profile_key;
    vtss_appl_ipmc_lib_profile_conf_t         profile_conf;
    vtss_appl_ipmc_lib_profile_range_key_t    range_key;
    vtss_appl_ipmc_lib_profile_range_conf_t   range_conf;
    vtss_appl_ipmc_lib_profile_rule_conf_t    rule_conf;
    char                                      encoded_string[4 * sizeof(profile_key.name) + 1];
    char                                      bgn_buf[IPV6_ADDR_IBUF_MAX_LEN], end_buf[IPV6_ADDR_IBUF_MAX_LEN];
    int                                       cntr, ct;
    size_t                                    var_len;
    const char                                *var_string;
    char                                      search_str[MAX(134, 3 * sizeof(profile_key.name))];
    mesa_rc                                   rc;

    /* Redirect unmanaged/invalid access to handler */
    if (!p || redirectUnmanagedOrInvalid(p, sid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC_LIB)) {
        return -1;
    }
#endif /* VTSS_SW_OPTION_PRIV_LVL */

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void)vtss_appl_ipmc_lib_profile_capabilities_get(&cap);

        var_string = cyg_httpd_form_varable_string(p, "pdx", &var_len);
        if (var_len == 0) {
            T_EG(IPMC_LIB_TRACE_GRP_WEB, "Unable to find profile name");
            return -1;
        }

        (void)cgi_unescape(var_string, profile_key.name, var_len, sizeof(profile_key.name));
        if (profile_key.name[0] == '\0') {
            T_EG(IPMC_LIB_TRACE_GRP_WEB, "cgi_unescape(%s, %d) failed", var_string, var_len);
            return -1;
        }

        if ((rc = vtss_appl_ipmc_lib_profile_conf_get(&profile_key, &profile_conf)) == VTSS_RC_OK) {
            // The easiest thing to keep track of the order is to delete all
            // current rules and create them again.
            vtss_clear(range_key);
            while (vtss_appl_ipmc_lib_profile_rule_itr(&profile_key, &profile_key, &range_key, &range_key, true /* only iterate across this profile */) == VTSS_RC_OK) {
                if ((rc = vtss_appl_ipmc_lib_profile_rule_conf_del(&profile_key, &range_key)) != VTSS_RC_OK) {
                    sprintf(search_str, "Unable to delete rule %s for profile %s", range_key.name, profile_key.name);
                    send_custom_error(p, "IPMC Profile Rule Configuration Error", search_str, strlen(search_str));
                    return -1;
                }
            }

            // Create the rules again.
            for (cntr = 1; cntr <= cap.range_cnt_max; cntr++) {
                sprintf(search_str, "edx_ipmcpf_rule_%d", cntr);
                var_string = cyg_httpd_form_varable_string(p, search_str, &var_len);
                if (var_len == 0) {
                    break;
                }

                (void)cgi_unescape(var_string, range_key.name, var_len, sizeof(range_key.name));
                if (range_key.name[0] == '\0') {
                    continue;
                }

                vtss_clear(rule_conf);

                sprintf(search_str, "action_ipmcpf_rule_%d", cntr);
                if (cyg_httpd_form_varable_int(p, search_str, &ct)) {
                    rule_conf.deny = ct == 0 ? true : false;
                }

                sprintf(search_str, "log_ipmcpf_rule_%d", cntr);
                if (cyg_httpd_form_varable_int(p, search_str, &ct)) {
                    rule_conf.log = ct ? true : false;
                }

                if ((rc = vtss_appl_ipmc_lib_profile_rule_conf_set(&profile_key, &range_key, &rule_conf, nullptr /* insert last */)) != VTSS_RC_OK) {
                    sprintf(search_str, "Unable to add rule for range %s for profile %s: %s", range_key.name, profile_key.name, error_txt(rc));
                    send_custom_error(p, "IPMC Profile Rule Configuration Error", search_str, strlen(search_str));
                    return -1;
                }
            }
        } else {
            T_IG(IPMC_LIB_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_profile_conf_get(%s) failed: %s", profile_key.name, error_txt(rc));
        }

        if (cgi_text_str_to_ascii_str(profile_key.name, encoded_string, strlen(profile_key.name), sizeof(encoded_string)) < 0) {
            sprintf(search_str, "Profile %s cannot be converted correctly!", profile_key.name);
            redirect_errmsg(p, "/ipmc_lib_profile_table.htm", search_str);
        } else {
            sprintf(search_str, "/ipmc_lib_rule_table.htm?CurSidV=%d&DoPdxOp=1&DynBgnPdx=%s", sid, encoded_string);
            redirect(p, search_str);
        }
    } else {
        var_string = cyg_httpd_form_varable_string(p, "DynBgnPdx", &var_len);
        if (var_len > 0) {
            (void)cgi_ascii_str_to_text_str(var_string, profile_key.name, var_len, sizeof(profile_key.name));
        } else {
            vtss_clear(profile_key);
        }

        // Format:
        //   [profile_name];[entry_1]/[bgn]/[end]|...|[entry_n]/[bgn]/[end];[entry_name]/[action]/[log]|...;
        (void)cyg_httpd_start_chunked("html");

        if ((rc = vtss_appl_ipmc_lib_profile_conf_get(&profile_key, &profile_conf)) == VTSS_RC_OK) {
            (void)cgi_escape(profile_key.name, encoded_string);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            // Loop once, getting *all* range configurations
            cntr = 0;
            vtss_clear(range_key);
            while (vtss_appl_ipmc_lib_profile_range_itr(&range_key, &range_key) == VTSS_RC_OK) {
                if ((rc = vtss_appl_ipmc_lib_profile_range_conf_get(&range_key, &range_conf)) != VTSS_RC_OK) {
                    T_EG(IPMC_LIB_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_profile_range_conf_get(%s) failed: %s", range_key.name, error_txt(rc));
                    continue;
                }

                (void)cgi_escape(range_key.name, encoded_string);

                if (range_conf.start.is_ipv4) {
                    (void)misc_ipv4_txt(range_conf.start.ipv4, bgn_buf);
                    (void)misc_ipv4_txt(range_conf.end.ipv4,   end_buf);
                } else {
                    (void)misc_ipv6_txt(&range_conf.start.ipv6, bgn_buf);
                    (void)misc_ipv6_txt(&range_conf.end.ipv6,   end_buf);
                }

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s/%s/%s", cntr ? "|" : ";", encoded_string, bgn_buf, end_buf);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                cntr++;
            }

            if (!cntr) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";NoEntries");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            // Loop again, this time getting rule configuration for our profile.
            cntr = 0;
            vtss_clear(range_key);
            while (vtss_appl_ipmc_lib_profile_rule_itr(&profile_key, &profile_key, &range_key, &range_key, true /* only iterate across this profile */) == VTSS_RC_OK) {
                if ((rc = vtss_appl_ipmc_lib_profile_rule_conf_get(&profile_key, &range_key, &rule_conf)) != VTSS_RC_OK) {
                    T_EG(IPMC_LIB_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_profile_rule_conf_get(%s, %s) failed: %s", profile_key.name, range_key.name, error_txt(rc));
                    continue;
                }

                (void)cgi_escape(range_key.name, encoded_string);

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s/%d/%u", cntr ? "|" : ";", encoded_string, !rule_conf.deny, rule_conf.log);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                cntr++;
            }

            if (!cntr) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";NoEntries;");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "INVALID;NoEntries;NoEntries;");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1;  // Do not further search the file system.
}

/******************************************************************************/
// IPMC_LIB_PROFILE_web_config_js()
/******************************************************************************/
static size_t IPMC_LIB_PROFILE_web_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    vtss_appl_ipmc_lib_profile_capabilities_t cap;
    char                                      buf[512];

    (void)vtss_appl_ipmc_lib_profile_capabilities_get(&cap);
    (void)snprintf(buf, sizeof(buf),
                   "var cfgIpmcLibPfeMax = %u;\n"
                   "var cfgIpmcLibPftMax = %u;\n"
                   "var cfgIpmcLibPfrMax = %u;\n"
                   "var cfgIpmcLibChrAscMin = %d;\n"
                   "var cfgIpmcLibChrAscMax = %d;\n"
                   "var cfgIpmcLibChrAscSpc = %d;\n",
                   cap.range_cnt_max,
                   cap.profile_cnt_max,
                   cap.range_cnt_max,
                   33,  // Chars in range [33; ...
                   126, // ...126]
                   32); // and sometimes a space

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buf);
}

// Entries
web_lib_config_js_tab_entry(IPMC_LIB_PROFILE_web_config_js);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ipmclib_range,   "/config/ipmclib_range",   IPMC_LIB_PROFILE_web_range_conf);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ipmclib_profile, "/config/ipmclib_profile", IPMC_LIB_PROFILE_web_profile_conf);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ipmclib_rule,    "/config/ipmclib_rule",    IPMC_LIB_PROFILE_web_rule_conf);

