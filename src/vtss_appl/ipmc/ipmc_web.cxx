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

#include "web_api.h"
#include "mgmt_api.h"
#include "port_api.h" /* For port_count_max() */
#include "ipmc_trace.h"
#include "ipmc_lib_utils.hxx"
#include <vtss/appl/ipmc_lib.h>
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_IPMC
#define IPMC_WEB_BUF_LEN     512

/******************************************************************************/
// IPMC_WEB_key_get()
/******************************************************************************/
static vtss_appl_ipmc_lib_key_t IPMC_WEB_key_get(bool is_ipv4)
{
    vtss_appl_ipmc_lib_key_t key = {};

    key.is_mvr  = false;
    key.is_ipv4 = is_ipv4;

    return key;
}

/******************************************************************************/
// IPMC_WEB_vlan_key_get()
/******************************************************************************/
static vtss_appl_ipmc_lib_vlan_key_t IPMC_WEB_vlan_key_get(mesa_vid_t vid, bool is_ipv4)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key = {};

    vlan_key.is_mvr  = false;
    vlan_key.is_ipv4 = is_ipv4;
    vlan_key.vid     = vid;

    return vlan_key;
}

/******************************************************************************/
// IPMC_WEB_global_conf_get()
/******************************************************************************/
static bool IPMC_WEB_global_conf_get(vtss_appl_ipmc_lib_global_conf_t &global_conf, bool is_ipv4)
{
    vtss_appl_ipmc_lib_key_t key = IPMC_WEB_key_get(is_ipv4);
    mesa_rc                  rc;

    if ((rc = vtss_appl_ipmc_lib_global_conf_get(key, &global_conf)) != VTSS_RC_OK) {
        T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_global_conf_get(%s) failed: %s", key, error_txt(rc));
        vtss_clear(global_conf);
    }

    return rc == VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_WEB_port_conf_get()
/******************************************************************************/
static bool IPMC_WEB_port_conf_get(mesa_port_no_t port_no, vtss_appl_ipmc_lib_port_conf_t &port_conf, bool is_ipv4)
{
    vtss_appl_ipmc_lib_key_t key = IPMC_WEB_key_get(is_ipv4);
    mesa_rc                  rc;

    if ((rc = vtss_appl_ipmc_lib_port_conf_get(key, port_no, &port_conf)) != VTSS_RC_OK) {
        T_EG_PORT(IPMC_TRACE_GRP_WEB, port_no, "vtss_appl_ipmc_lib_port_conf_get(%s, %u) failed: %s", key, port_no, error_txt(rc));
        vtss_clear(port_conf);
    }

    return rc == VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_WEB_port_conf_set()
/******************************************************************************/
static void IPMC_WEB_port_conf_set(mesa_port_no_t port_no, const vtss_appl_ipmc_lib_port_conf_t &port_conf, bool is_ipv4)
{
    vtss_appl_ipmc_lib_key_t key = IPMC_WEB_key_get(is_ipv4);
    mesa_rc                  rc;

    if ((rc = vtss_appl_ipmc_lib_port_conf_set(key, port_no, &port_conf)) != VTSS_RC_OK) {
        T_EG_PORT(IPMC_TRACE_GRP_WEB, port_no, "vtss_appl_ipmc_lib_port_conf_set(%s, %u) failed: %s", key, port_no, error_txt(rc));
    }
}

/******************************************************************************/
// IPMC_WEB_vlan_conf_get()
/******************************************************************************/
static bool IPMC_WEB_vlan_conf_get(mesa_vid_t vid, vtss_appl_ipmc_lib_vlan_conf_t &vlan_conf, bool is_ipv4)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key = IPMC_WEB_vlan_key_get(vid, is_ipv4);
    mesa_rc                       rc;

    if ((rc = vtss_appl_ipmc_lib_vlan_conf_get(vlan_key, &vlan_conf)) != VTSS_RC_OK) {
        T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_conf_get(%s) failed: %s", vlan_key, error_txt(rc));
        vtss_clear(vlan_conf);
    }

    return rc == VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_WEB_vlan_conf_set()
/******************************************************************************/
static bool IPMC_WEB_vlan_conf_set(mesa_vid_t vid, vtss_appl_ipmc_lib_vlan_conf_t &vlan_conf, bool is_ipv4)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key = IPMC_WEB_vlan_key_get(vid, is_ipv4);
    mesa_rc                       rc;

    if ((rc = vtss_appl_ipmc_lib_vlan_conf_set(vlan_key, &vlan_conf)) != VTSS_RC_OK) {
        T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_conf_set(%s, %s) failed: %s", vlan_key, vlan_conf, error_txt(rc));
        vtss_clear(vlan_conf);
    }

    return rc == VTSS_RC_OK;
}

/******************************************************************************/
// IPMC_WEB_vlan_itr()
/******************************************************************************/
static bool IPMC_WEB_vlan_itr(mesa_vid_t &vid, bool is_ipv4)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key = IPMC_WEB_vlan_key_get(vid, is_ipv4);

    if (vtss_appl_ipmc_lib_vlan_itr(&vlan_key, &vlan_key, true /* Stay in IPMC and this IP family */) == VTSS_RC_OK) {
        vid = vlan_key.vid;
        return true;
    }

    return false;
}

/******************************************************************************/
// IPMC_WEB_config()
/******************************************************************************/
static int32_t IPMC_WEB_config(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                      sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t                   port_no;
    vtss_uport_no_t                  uport;
    int                              ct = 0;
    uint32_t                         port_count = port_count_max();
    vtss_appl_ipmc_lib_key_t         key;
    vtss_appl_ipmc_lib_port_conf_t   port_conf;
    vtss_appl_ipmc_lib_global_conf_t global_conf, global_conf_def;
    char                             var_router_port[16], var_fast_leave_port[20], ip_buf[IPV6_ADDR_IBUF_MAX_LEN];
    int                              var_value;
    bool                             is_ipv4 = true;
    const char                       *var_string;
    mesa_rc                          rc;
#ifdef VTSS_SW_OPTION_SMB_IPMC
    char                             var_throttling_port[20];
    int                              ipmc_smb = 1, var_value2;
#else
    int                              ipmc_smb = 0;
#endif /* VTSS_SW_OPTION_SMB_IPMC */

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        size_t nlen;

        (void)cyg_httpd_form_varable_int(p, "ipmc_version", &var_value);
        is_ipv4 = !(bool)var_value; // note: version 0=igmp; 1=mld
        if (!IPMC_WEB_global_conf_get(global_conf, is_ipv4)) {
            return -1;
        }

        key = IPMC_WEB_key_get(is_ipv4);
        if ((rc = vtss_appl_ipmc_lib_global_conf_default_get(key, &global_conf_def)) != VTSS_RC_OK) {
            T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_global_conf_default_get(%s) failed: %s", key, error_txt(rc));
            return -1;
        }

        if (cyg_httpd_form_varable_string(p, "ipmc_mode", &nlen)) {
            global_conf.admin_active = true;
        } else {
            global_conf.admin_active = false;
        }

        if (cyg_httpd_form_varable_string(p, "unreg_ipmc", &nlen)) {
            global_conf.unregistered_flooding_enable = true;
        } else {
            global_conf.unregistered_flooding_enable = false;
        }

#ifdef VTSS_SW_OPTION_SMB_IPMC
        if (is_ipv4) {
            (void)cyg_httpd_form_varable_ipv4(p, "ssm4_range_prefix", &global_conf.ssm_prefix.ipv4);
            (void)cyg_httpd_form_varable_int(p, "ssm4_range_length", &var_value2);
            global_conf.ssm_prefix_len = var_value2;
        } else {
            if (!cyg_httpd_form_varable_ipv6(p, "ssm6_range_prefix", &global_conf.ssm_prefix.ipv6)) {
                global_conf.ssm_prefix.ipv6 = global_conf_def.ssm_prefix.ipv6;
            }

            if (cyg_httpd_form_varable_int(p, "ssm6_range_length", &var_value2)) {
                global_conf.ssm_prefix_len = var_value2;
            } else {
                global_conf.ssm_prefix_len = global_conf_def.ssm_prefix_len;
            }
        }

        if (cyg_httpd_form_varable_string(p, "leave_proxy", &nlen)) {
            global_conf.leave_proxy_enable = true;
        } else {
            global_conf.leave_proxy_enable = false;
        }

        if (cyg_httpd_form_varable_string(p, "proxy", &nlen)) {
            global_conf.proxy_enable = true;
        } else {
            global_conf.proxy_enable = false;
        }
#endif /* VTSS_SW_OPTION_SMB_IPMC */

        if ((rc = vtss_appl_ipmc_lib_global_conf_set(key, &global_conf)) != VTSS_RC_OK) {
            T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_global_conf_set(%s) failed: %s", key, error_txt(rc));
        }

        for (port_no = 0; port_no < port_count; port_no++) {
            uport = iport2uport(port_no);
            sprintf(var_router_port, "router_port_%d", uport);
            sprintf(var_fast_leave_port, "fast_leave_port_%d", uport);

            (void)IPMC_WEB_port_conf_get(port_no, port_conf, is_ipv4);

            if (cyg_httpd_form_varable_string(p, var_router_port, &nlen)) {
                port_conf.router = true;
            } else {
                port_conf.router = false;
            }

            if (cyg_httpd_form_varable_string(p, var_fast_leave_port, &nlen)) {
                port_conf.fast_leave = true;
            } else {
                port_conf.fast_leave = false;
            }

#ifdef VTSS_SW_OPTION_SMB_IPMC
            sprintf(var_throttling_port, "throttling_port_%d", uport);
            if (cyg_httpd_form_varable_int(p, var_throttling_port, &var_value2)) {
                port_conf.grp_cnt_max = var_value2;
            }
#endif

            IPMC_WEB_port_conf_set(port_no, port_conf, is_ipv4);
        }

        redirect(p, is_ipv4 ? "/ipmc_igmps.htm" : "/ipmc_mldsnp.htm");
    } else {
        /* CYG_HTTPD_METHOD_GET (+HEAD) */

        // Get ipmc_version
        if ((var_string = cyg_httpd_form_varable_find(p, "ipmc_version")) != NULL) {
            is_ipv4 = atoi(var_string) == 0; // Note: version 0=igmp; 1=mld
        }

        (void)cyg_httpd_start_chunked("html");

        if (!IPMC_WEB_global_conf_get(global_conf, is_ipv4)) {
            return -1;
        }

        // Format:
        //   [ipmc_smb]/[ipmc_mode]/[unreg_ipmc]/[ssm6_range_prefix]/[ssm6_range_length]/[leave_proxy]/[proxy],[port_no]/[router_port]/[fast_leave]/[throttling]|...
        (void)global_conf.ssm_prefix.print(ip_buf);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%d/%u/%u/%s/%u/%u/%u,",
                      ipmc_smb,
                      global_conf.admin_active,
                      global_conf.unregistered_flooding_enable,
                      ip_buf,
                      global_conf.ssm_prefix_len,
                      global_conf.leave_proxy_enable,
                      global_conf.proxy_enable);

        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        for (port_no = 0; port_no < port_count; port_no++) {
            if (IPMC_WEB_port_conf_get(port_no, port_conf, is_ipv4)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u|", iport2uport(port_no), port_conf.router, port_conf.fast_leave, port_conf.grp_cnt_max);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/******************************************************************************/
// IPMC_WEB_vlan_config()
/******************************************************************************/
static int32_t IPMC_WEB_vlan_config(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                    sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ipmc_capabilities_t  cap;
    vtss_appl_ipmc_lib_vlan_conf_t vlan_conf;
    vtss_ifindex_t                 ifidx;
    int                            ct = 0, cntr, var_value;
    mesa_vid_t                     vid = 0;
    char                           var_vlan[32];
    bool                           is_ipv4 = true;;
    const char                     *var_string, *err_buf_ptr;
    mesa_rc                        rc;
#ifdef VTSS_SW_OPTION_SMB_IPMC
    int                            ipmc_smb = 1;
#else
    int                            ipmc_smb = 0;
#endif /* VTSS_SW_OPTION_SMB_IPMC */

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC)) {
        return -1;
    }
#endif

    if ((rc = vtss_appl_ipmc_capabilities_get(false /* IPMC */, &cap)) != VTSS_RC_OK) {
        T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_capabilities_get() failed: %s", error_txt(rc));
        return -1;
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void)cyg_httpd_form_varable_int(p, "ipmc_version", &var_value);
        is_ipv4 = !(bool)var_value; // note: version 0=igmp; 1=mld

        for (cntr = 0; cntr < fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT); cntr++) {
            if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "mvid_ipmc_vlan_%d", cntr)) {
                continue;
            }

            vid = (mesa_vid_t)(var_value & 0xFFFF);
            if (vtss_ifindex_from_vlan(vid, &ifidx) != VTSS_RC_OK) {
                continue;
            }

            if (!IPMC_WEB_vlan_conf_get(vid, vlan_conf, is_ipv4)) {
                err_buf_ptr = "Invalid VLAN Interface!";
                send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                return -1; // Trying to configure a non-existing VLAN-interface is considered an error.
            }

            if (is_ipv4) {
                vlan_conf.admin_active = cyg_httpd_form_variable_check_fmt(p, "vlan_mode_%d", vid);
                vlan_conf.querier_enable = cyg_httpd_form_variable_check_fmt(p, "vlan_query_%d", vid);

                sprintf(var_vlan, "vlan_qradr_%d", vid);
                if (!cyg_httpd_form_varable_ipv4(p, var_vlan, &vlan_conf.querier_address.ipv4)) {
                    err_buf_ptr = "Invalid Querier Address!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

#ifdef VTSS_SW_OPTION_SMB_IPMC
                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_compat_%d", vid)) {
                    err_buf_ptr = "Invalid IGMP Compatibility!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.compatibility = (vtss_appl_ipmc_lib_compatibility_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_pri_%d", vid)) {
                    err_buf_ptr = "Invalid Priority!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.pcp = (uint8_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_rv_%d", vid)) {
                    err_buf_ptr = "Invalid Robustness Variable!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.rv = (uint32_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_qi_%d", vid)) {
                    err_buf_ptr = "Invalid Query Interval!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.qi = (uint32_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_qri_%d", vid)) {
                    err_buf_ptr = "Invalid Query Response Interval!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.qri = (uint32_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_llqi_%d", vid)) {
                    err_buf_ptr = "Invalid Last Lisnener Query Interval!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.lmqi = (uint32_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_uri_%d", vid)) {
                    err_buf_ptr = "Invalid Unsolicited Report Interval!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.uri = (uint32_t)var_value;

#endif /* VTSS_SW_OPTION_SMB_IPMC */

                if (!IPMC_WEB_vlan_conf_set(vid, vlan_conf, is_ipv4)) {
                    err_buf_ptr = "Could not set the configuration!";
                    send_custom_error(p, "IPMC Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }
            } else {
#ifdef VTSS_SW_OPTION_SMB_IPMC

                vlan_conf.admin_active = cyg_httpd_form_variable_check_fmt(p, "vlan_mode_%d", vid);
                vlan_conf.querier_enable = cyg_httpd_form_variable_check_fmt(p, "vlan_query_%d", vid);

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_compat_%d", vid)) {
                    err_buf_ptr = "Invalid MLD Compatibility!";
                    send_custom_error(p, "MLD Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.compatibility = (vtss_appl_ipmc_lib_compatibility_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_pri_%d", vid)) {
                    err_buf_ptr = "Invalid Priority!";
                    send_custom_error(p, "MLD Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.pcp = (uint8_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_rv_%d", vid)) {
                    err_buf_ptr = "Invalid Robustness Variable!";
                    send_custom_error(p, "MLD Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.rv = (uint32_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_qi_%d", vid)) {
                    err_buf_ptr = "Invalid Query Interval!";
                    send_custom_error(p, "MLD Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.qi = (uint32_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_qri_%d", vid)) {
                    err_buf_ptr = "Invalid Query Response Interval!";
                    send_custom_error(p, "MLD Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.qri = (uint32_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_llqi_%d", vid)) {
                    err_buf_ptr = "Invalid Last Lisnener Query Interval!";
                    send_custom_error(p, "MLD Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.lmqi = (uint32_t)var_value;

                if (!cyg_httpd_form_variable_int_fmt(p, &var_value, "vlan_uri_%d", vid)) {
                    err_buf_ptr = "Invalid Unsolicited Report Interval!";
                    send_custom_error(p, "MLD Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }

                vlan_conf.uri = (uint32_t)var_value;

                if (!IPMC_WEB_vlan_conf_set(vid, vlan_conf, is_ipv4)) {
                    err_buf_ptr = "Could not set the configuration!";
                    send_custom_error(p, "MLD Interface Configuration Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }
#endif /* VTSS_SW_OPTION_SMB_IPMC */
            }

            /* bz21630 shorten the wait time from 1000ms to 100 ms */
            VTSS_OS_MSLEEP(100); // Wait for processing querier state
        }

        redirect(p, is_ipv4 ? "/ipmc_igmps_vlan.htm" : "/ipmc_mldsnp_vlan.htm");
    } else {
        int         entry_cnt, num_of_entries, dyn_get_next_entry;
        mesa_vid_t  start_vid;
        /* CYG_HTTPD_METHOD_GET (+HEAD) */

        // Get ipmc_version
        if ((var_string = cyg_httpd_form_varable_find(p, "ipmc_version")) != NULL) {
            is_ipv4 = atoi(var_string) == 0; // Note: version 0=igmp; 1=mld
        }

        // Format: [start_vid],[num_of_entries],[ipmc_smb],[vid]/[vlan_mode]/[vlan_query]|...
        start_vid = 1;
        entry_cnt = num_of_entries = dyn_get_next_entry = 0;
        (void)cyg_httpd_start_chunked("html");

        // Get start vid
        if ((var_string = cyg_httpd_form_varable_find(p, "DynStartVid")) != NULL) {
            start_vid = atoi(var_string);
        }

        // Get number of entries per page
        if ((var_string = cyg_httpd_form_varable_find(p, "DynNumberOfEntries")) != NULL) {
            num_of_entries = atoi(var_string);
        }

        if (num_of_entries <= 0 || num_of_entries > cap.vlan_cnt_max) {
            num_of_entries = cap.vlan_cnt_max;
        }

        // Get or GetNext
        if ((var_string = cyg_httpd_form_varable_find(p, "GetNextEntry")) != NULL) {
            dyn_get_next_entry = atoi(var_string);
        }

        if (dyn_get_next_entry && start_vid < 4095) {
            start_vid++;
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%d,%d,",
                      start_vid, num_of_entries, ipmc_smb);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        // Walk through all VLANs
        vid = 0;
        while (IPMC_WEB_vlan_itr(vid, is_ipv4)) {
            if (!IPMC_WEB_vlan_conf_get(vid, vlan_conf, is_ipv4)) {
                continue;
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/", vid);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u",
                          vlan_conf.admin_active, vlan_conf.querier_enable);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            if (is_ipv4) {
                char ip_buf[IPV6_ADDR_IBUF_MAX_LEN];
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", vlan_conf.querier_address.print(ip_buf));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
#ifdef VTSS_SW_OPTION_SMB_IPMC
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%u/%u/%u/%u/%u/%u",
                          vlan_conf.compatibility,
                          vlan_conf.rv,
                          vlan_conf.qi,
                          vlan_conf.qri,
                          vlan_conf.lmqi,
                          vlan_conf.uri,
                          vlan_conf.pcp);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
#endif /* #ifdef VTSS_SW_OPTION_SMB_IPMC */
            (void)cyg_httpd_write_chunked("|", 1);
            entry_cnt++;
        }

        if (entry_cnt == 0) { /* No entry existing */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "NoEntries");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_WEB_filtering_conf()
/******************************************************************************/
static int32_t IPMC_WEB_filtering_conf(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                      sid = web_retrieve_request_sid(p);
    bool                             is_ipv4;
    const char                       *var_string;
    uint32_t                         port_count = port_count_max();
    mesa_port_no_t                   port_no;
    vtss_appl_ipmc_lib_profile_key_t profile_key;
    vtss_appl_ipmc_lib_port_conf_t   port_conf;
    char                             encoded_string[3 * sizeof(profile_key.name) + 1], search_str[65];
    int                              var_value, ct, cntr;
    size_t                           var_len;

    // Redirect unmanaged/invalid access to handler
    if (!p || redirectUnmanagedOrInvalid(p, sid)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IPMC)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void)cyg_httpd_form_varable_int(p, "ipmc_version", &var_value);
        is_ipv4 = var_value == 0; // note: version 0=igmp; 1=mld
        for (port_no = 0; port_no < port_count; port_no++) {
            if (!IPMC_WEB_port_conf_get(port_no, port_conf, is_ipv4)) {
                continue;
            }

            sprintf(search_str, "ref_port_filter_profile_%u", iport2uport(port_no));
            var_string = cyg_httpd_form_varable_string(p, search_str, &var_len);
            port_conf.profile_key.name[0] = '\0';
            if (var_len > 0) {
                (void)cgi_unescape(var_string, port_conf.profile_key.name, var_len, sizeof(port_conf.profile_key.name));
            }

            IPMC_WEB_port_conf_set(port_no, port_conf, is_ipv4);
        }

        redirect(p, is_ipv4 ? "/ipmc_igmps_filtering.htm" : "/ipmc_mldsnp_filtering.htm");
    } else {
        // Get ipmc_version
        if ((var_string = cyg_httpd_form_varable_find(p, "ipmc_version")) != NULL) {
            is_ipv4 = atoi(var_string) == 0; // note: version 0=igmp; 1=mld
        } else {
            is_ipv4 = true;
        }

        (void)cyg_httpd_start_chunked("html");

        // Format: [profile_1]|...|[profile_n];port,[profile]|...;

        // First list all existing profiles.
        cntr = 0;
        vtss_clear(profile_key);
        while (vtss_appl_ipmc_lib_profile_itr(&profile_key, &profile_key) == VTSS_RC_OK) {
            (void)cgi_escape(profile_key.name, encoded_string);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s", cntr ? "|" : "", encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            cntr++;
        }

        // Then send the currently configured per-port profiles.
        cntr = 0;
        for (port_no = 0; port_no < port_count; port_no++) {
            (void)IPMC_WEB_port_conf_get(port_no, port_conf, is_ipv4);

            (void)cgi_escape(port_conf.profile_key.name, encoded_string);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u,%s", cntr ? "|" : ";", iport2uport(port_no), encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            cntr++;
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* VTSS_SW_OPTION_SMB_IPMC */

/******************************************************************************/
// IPMC_WEB_vlan_status()
/******************************************************************************/
static int32_t IPMC_WEB_vlan_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                          sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ipmc_lib_key_t             key;
    vtss_appl_ipmc_lib_vlan_key_t        vlan_key;
    vtss_appl_ipmc_lib_global_conf_t     global_conf;
    vtss_appl_ipmc_lib_vlan_conf_t       vlan_conf;
    vtss_appl_ipmc_lib_vlan_status_t     vlan_status;
    vtss_appl_ipmc_lib_vlan_statistics_t vlan_statistics;
    vtss_appl_ipmc_lib_port_status_t     port_status;
    mesa_port_no_t                       port_no;
    uint32_t                             port_count = port_count_max();
    int                                  ct = 0;
    mesa_vid_t                           vid;
    bool                                 is_ipv4 = true;
    const char                           *var_string;
    mesa_rc                              rc;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC)) {
        return -1;
    }
#endif

    // Get ipmc_version
    if ((var_string = cyg_httpd_form_varable_find(p, "ipmc_version")) != NULL) {
        is_ipv4 = atoi(var_string) == 0; // note: version 0=igmp; 1=mld
    }

    /* get form data
       Format: [vid],[querier_ver],[host_ver],[querier_state],[querier_transmitted],[received_v1_reports],[received_v2_reports],[received_v1_leave]
               | [port_no],[status]/[port_no],[status]/...
       status 0: None     1: Static      2: Dynamic      3: Both
    */
    (void)cyg_httpd_start_chunked("html");

    if ((cyg_httpd_form_varable_find(p, "clear") != NULL)) { /* Clear? */
        vid = 0;
        vlan_key = IPMC_WEB_vlan_key_get(vid, is_ipv4);
        while (IPMC_WEB_vlan_itr(vid, is_ipv4)) {
            if ((rc = vtss_appl_ipmc_lib_vlan_statistics_clear(vlan_key)) != VTSS_RC_OK) {
                T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_statistics_clear(%s) failed: %s", vlan_key, error_txt(rc));
            }
        }
    }

    if (!IPMC_WEB_global_conf_get(global_conf, is_ipv4)) {
        return -1;
    }

    // Walk through all VLANs
    vid = 0;
    while (IPMC_WEB_vlan_itr(vid, is_ipv4)) {
        if (!IPMC_WEB_vlan_conf_get(vid, vlan_conf, is_ipv4)) {
            continue;
        }

        vlan_key = IPMC_WEB_vlan_key_get(vid, is_ipv4);
        if ((rc = vtss_appl_ipmc_lib_vlan_status_get(vlan_key, &vlan_status)) != VTSS_RC_OK) {
            T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_status_get(%s) failed: %s", vlan_key, error_txt(rc));
            continue;
        }

        if ((rc = vtss_appl_ipmc_lib_vlan_statistics_get(vlan_key, &vlan_statistics)) != VTSS_RC_OK) {
            T_EG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_vlan_statistics_get(%s) failed: %s", vlan_key, error_txt(rc));
            continue;
        }

        /* Don't show disabled interfaces, since no meaningful running info */
        if (!global_conf.admin_active || !vlan_conf.admin_active) {
            T_D("Don't show VLAN %d, since it's disabled", vid);
            continue;
        }

        /* VLAN ID */
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", vid);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s,", ipmc_lib_util_compatibility_to_str(vlan_status.querier_compat, is_ipv4));
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s,", ipmc_lib_util_compatibility_to_str(vlan_status.host_compat, is_ipv4));
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        /* querier */
        if (vlan_status.querier_state != VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,",
                          VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE,
                          vlan_statistics.tx_query);
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,",
                          VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE,
                          vlan_statistics.tx_query);
        }

        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        /* counters */
        if (is_ipv4) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,%u,%u,%u/",
                          vlan_statistics.rx_query,
                          vlan_statistics.rx.igmp.utilized.v1_report + vlan_statistics.rx.igmp.ignored.v1_report,
                          vlan_statistics.rx.igmp.utilized.v2_report + vlan_statistics.rx.igmp.ignored.v2_report,
                          vlan_statistics.rx.igmp.utilized.v3_report + vlan_statistics.rx.igmp.ignored.v3_report,
                          vlan_statistics.rx.igmp.utilized.v2_leave  + vlan_statistics.rx.igmp.ignored.v2_leave);
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,%u,%u/",
                          vlan_statistics.rx_query,
                          vlan_statistics.rx.mld.utilized.v1_report + vlan_statistics.rx.mld.ignored.v1_report,
                          vlan_statistics.rx.mld.utilized.v2_report + vlan_statistics.rx.mld.ignored.v2_report,
                          vlan_statistics.rx.mld.utilized.v1_done   + vlan_statistics.rx.mld.ignored.v1_done);
        }

        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    (void)cyg_httpd_write_chunked("|", 1);
    key = IPMC_WEB_key_get(is_ipv4);
    for (port_no = 0; port_no < port_count; port_no++) {
        if ((rc = vtss_appl_ipmc_lib_port_status_get(key, port_no, &port_status)) != VTSS_RC_OK) {
            T_EG_PORT(IPMC_TRACE_GRP_WEB, port_no, "vtss_appl_ipmc_lib_port_status_get(%s) failed: %s", key, error_txt(rc));
            port_status.router_status = VTSS_APPL_IPMC_LIB_ROUTER_STATUS_NONE;
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%d/", iport2uport(port_no), port_status.router_status);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/******************************************************************************/
// IPMC_WEB_grp_status()
/******************************************************************************/
static int32_t IPMC_WEB_grp_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ipmc_lib_vlan_key_t   vlan_key;
    vtss_appl_ipmc_lib_grp_status_t grp_status;
    int                             ct, entry_cnt = 0, entry_cnt_max = 0, var_value;
    mesa_port_no_t                  port_no;
    uint32_t                        port_cnt = port_count_max();
    vtss_appl_ipmc_lib_ip_t         grp_addr = {};
    mesa_vid_t                      vid = 1;
    char                            ip_buf[IPV6_ADDR_IBUF_MAX_LEN];
    bool                            is_ipv4 = true, dyn_get_next_entry;
    const char                      *var_string;
    mesa_rc                         rc;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void)cyg_httpd_form_varable_int(p, "ipmc_version", &var_value);
        is_ipv4 = !(bool)var_value; // note: version 0=igmp; 1=mld
        redirect(p, is_ipv4 ? "/ipmc_igmps_groups_info.htm" : "/ipmc_mldsnp_groups_info.htm");
    } else { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        // Get ipmc_version
        if ((var_string = cyg_httpd_form_varable_find(p, "ipmc_version")) != NULL) {
            is_ipv4 = atoi(var_string) == 0; // Note: version 0=igmp; 1=mld
        }

        grp_addr.is_ipv4 = is_ipv4;

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
        }

        // Get start group address
        if (cyg_httpd_form_varable_find(p, "DynStartGroup") != NULL) {
            if (is_ipv4) {
                (void)cyg_httpd_form_varable_ipv4(p, "DynStartGroup", &grp_addr.ipv4);
            } else {
                (void)cyg_httpd_form_varable_ipv6(p, "DynStartGroup", &grp_addr.ipv6);
            }
        }

        // Get or GetNext
        dyn_get_next_entry = false;
        if ((var_string = cyg_httpd_form_varable_find(p, "GetNextEntry")) != NULL) {
            dyn_get_next_entry = atoi(var_string) != 0;
        }

        // Format:
        //  <start_vid>|<start_group>|<entry_cnt_max>|<port_cnt>|[vid],[groups],[port1_is_member],[port2_is_member],[port3_is_member],.../...
        if (!dyn_get_next_entry) {
            // Not get next button
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|%d|%u|",
                          vid,
                          grp_addr.print(ip_buf),
                          entry_cnt_max,
                          port_cnt);

            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            // We also need to subract one from the current IP address in order
            // to not starting with the next.
            --grp_addr; // Using vtss_appl_ipmc_ip_lib_t::operator--() prefix operator
        }

        T_IG(IPMC_TRACE_GRP_WEB, "Starting from VID = %u and addr = %s", vid, grp_addr);

        // Iterate through all groups
        vlan_key = IPMC_WEB_vlan_key_get(vid, is_ipv4);
        while (vtss_appl_ipmc_lib_grp_itr(&vlan_key, &vlan_key, &grp_addr, &grp_addr, true /* Don't mix IPMC/MVR and IPv4/IPv6 */) == VTSS_RC_OK) {
            if ((rc = vtss_appl_ipmc_lib_grp_status_get(vlan_key, &grp_addr, &grp_status)) != VTSS_RC_OK) {
                // Don't use T_EG(), because it may be deleted between the
                // itr() and the get()
                T_IG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_grp_status_get(%s, %s) failed: %s", vlan_key, grp_addr, error_txt(rc));
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
    }

    return -1; // Do not further search the file system.
}

#ifdef VTSS_SW_OPTION_SMB_IPMC
/******************************************************************************/
// IPMC_WEB_src_info()
/******************************************************************************/
static int32_t IPMC_WEB_src_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ipmc_lib_vlan_key_t   vlan_key;
    vtss_appl_ipmc_lib_src_status_t src_status;
    vtss_appl_ipmc_lib_ip_t         grp_addr = {}, src_addr = {}, prev_grp_addr = {};
    int                             ct = 0, entry_cnt = 0, entry_cnt_max = 0, var_value;
    mesa_port_no_t                  port_no = 0, prev_port_no = 0;
    mesa_vid_t                      vid = 1, prev_vid = 0;
    char                            ip_buf1[IPV6_ADDR_IBUF_MAX_LEN], ip_buf2[IPV6_ADDR_IBUF_MAX_LEN];
    bool                            is_ipv4 = true, same_grp_as_before, dyn_get_next_entry;
    const char                      *var_string;
    mesa_rc                         rc;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IPMC)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void)cyg_httpd_form_varable_int(p, "ipmc_version", &var_value);
        is_ipv4 = !(bool)var_value; // note: version 0=igmp; 1=mld
        redirect(p, is_ipv4 ? "/ipmc_igmps_v2info.htm" : "/ipmc_mldsnp_v2info.htm");
    } else {
        // Get ipmc_version
        if ((var_string = cyg_httpd_form_varable_find(p, "ipmc_version")) != NULL) {
            is_ipv4 = atoi(var_string) == 0; // Note: version 0=igmp; 1=mld
        }

        grp_addr.is_ipv4 = is_ipv4;
        src_addr.is_ipv4 = is_ipv4;

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
        }

        // Get start group address
        if (cyg_httpd_form_varable_find(p, "DynStartGroup") != NULL) {
            if (is_ipv4) {
                (void)cyg_httpd_form_varable_ipv4(p, "DynStartGroup", &grp_addr.ipv4);
            } else {
                (void)cyg_httpd_form_varable_ipv6(p, "DynStartGroup", &grp_addr.ipv6);
            }
        }

        // Get or GetNext
        dyn_get_next_entry = false;
        if ((var_string = cyg_httpd_form_varable_find(p, "GetNextEntry")) != NULL) {
            dyn_get_next_entry = atoi(var_string) != 0;
        }

        // Format:
        //   <start_vid>/<start_group>/<entry_cnt_max>|[vid]/[group]/[port]/[mode]/[source_addr]/[type]/[in_hw]|...
        if (!dyn_get_next_entry) {
            // Not get next button
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%s/%u|", vid, grp_addr.print(ip_buf1), entry_cnt_max);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            // We also need to subract one from the current IP address in order
            // to not starting with the next.
            --grp_addr; // Using vtss_appl_ipmc_lib_ip_t::operator--() prefix operator
        }

        T_IG(IPMC_TRACE_GRP_WEB, "Starting from VID = %u and addr = %s", vid, grp_addr);

        // Iterate through all groups and sources
        vlan_key = IPMC_WEB_vlan_key_get(vid, is_ipv4);
        while (vtss_appl_ipmc_lib_src_itr(&vlan_key, &vlan_key, &grp_addr, &grp_addr, &port_no, &port_no, &src_addr, &src_addr, true /* Don't mix IPMC/MVR and IPv4/IPv6 */) == VTSS_RC_OK) {
            if ((rc = vtss_appl_ipmc_lib_src_status_get(vlan_key, &grp_addr, port_no, &src_addr, &src_status)) != VTSS_RC_OK) {
                // Don't use T_EG(), because it may be deleted between the
                // itr() and the get()
                T_IG(IPMC_TRACE_GRP_WEB, "vtss_appl_ipmc_lib_src_status_get(%s, %s, %u, %s) failed: %s", vlan_key, grp_addr, port_no, src_addr, error_txt(rc));
                continue;
            }

            if (++entry_cnt > entry_cnt_max) {
                break;
            }

            if (dyn_get_next_entry && entry_cnt == 1) { /* Only for GetNext button */
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%s/%u|", vlan_key.vid, grp_addr.print(ip_buf1), entry_cnt_max);
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

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s/%u/%d/%s/%d/%d|",
                          vlan_key.vid,
                          grp_addr.print(ip_buf1),
                          iport2uport(port_no),
                          src_status.filter_mode,
                          ip_buf2,
                          src_status.forwarding,
                          src_status.hw_location != VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE);

            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            prev_vid      = vlan_key.vid;
            prev_grp_addr = grp_addr;
            prev_port_no  = port_no;
        }

        if (entry_cnt == 0) { /* No entry existing */
            if (dyn_get_next_entry) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%s/%u|", vid, grp_addr.print(ip_buf1), entry_cnt_max);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "NoEntries");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* VTSS_SW_OPTION_SMB_IPMC */

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t ipmc_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buf[IPMC_WEB_BUF_LEN];
    (void)snprintf(buf, IPMC_WEB_BUF_LEN, "var configIpmcVLANsMax = %d;\n", fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT));
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buf);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(ipmc_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ipmc,           "/config/ipmc",           IPMC_WEB_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ipmc_vlan,      "/config/ipmc_vlan",      IPMC_WEB_vlan_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ipmc_status,      "/stat/ipmc_status",      IPMC_WEB_vlan_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ipmc_groups_info, "/stat/ipmc_groups_info", IPMC_WEB_grp_status);
#ifdef VTSS_SW_OPTION_SMB_IPMC
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ipmc_filtering, "/config/ipmc_filtering", IPMC_WEB_filtering_conf);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ipmc_sfm_info,    "/stat/ipmc_v2info",      IPMC_WEB_src_status);
#endif

