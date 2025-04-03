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
#include "dhcp_relay_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static i32 handler_config_dhcp_relay(CYG_HTTPD_STATE *p)
{
    int                 ct;
    dhcp_relay_conf_t   conf, newconf;
    int                 var_value;
    char                str_buff[40];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        if (dhcp_relay_mgmt_conf_get(&conf) == VTSS_RC_OK) {
            /* store form data */
            newconf = conf;

            /* relay_mode */
            if (cyg_httpd_form_varable_int(p, "relay_mode", &var_value)) {
                newconf.relay_mode = var_value;
            }

            /* relay_server */
            (void)cyg_httpd_form_varable_ipv4(p, "relay_server", &newconf.relay_server[0]);
            if (newconf.relay_server[0] == 0) {
                newconf.relay_server_cnt = 0;
            } else {
                newconf.relay_server_cnt = 1;
            }

            /* relay_info_mode */
            if (cyg_httpd_form_varable_int(p, "relay_info_mode", &var_value)) {
                newconf.relay_info_mode = var_value;
            }

            /* relay_info_policy */
            if (cyg_httpd_form_varable_int(p, "relay_info_policy", &var_value)) {
                newconf.relay_info_policy = var_value;
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                T_D("Calling dhcp_relay_mgmt_conf_set()");
                if (dhcp_relay_mgmt_conf_set(&newconf) < 0) {
                    T_E("dhcp_relay_mgmt_conf_set(): failed");
                }
            }
        }
        redirect(p, "/dhcp_relay.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [dhcp_snooping_supported]/[relay_mode]/[relay_server]/[relay_info_mode]/[relay_info_policy]
        */
        if (dhcp_relay_mgmt_conf_get(&conf) == VTSS_RC_OK) {
            if ( conf.relay_server_cnt == 0 ) {
                conf.relay_server[0] = 0;
            }
#ifdef VTSS_SW_OPTION_DHCP_SNOOPING
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "1/%u/%s/%u/%u", conf.relay_mode, misc_ipv4_txt(conf.relay_server[0], str_buff), conf.relay_info_mode, conf.relay_info_policy);
#else
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "0/%u/%s/%u/%u", conf.relay_mode, misc_ipv4_txt(conf.relay_server[0], str_buff), conf.relay_info_mode, conf.relay_info_policy);
#endif
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_stat_dhcp_relay_statistics(CYG_HTTPD_STATE *p)
{
    int                 ct;
    dhcp_relay_stats_t  stats;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP)) {
        return -1;
    }
#endif

    (void)cyg_httpd_start_chunked("html");

    if ((cyg_httpd_form_varable_find(p, "clear") != NULL)) { /* Clear? */
        dhcp_relay_stats_clear();
    }
    dhcp_relay_stats_get(&stats);

    //Format: [interface]/[interface_receive_cnt]/[interface_allow_cnt]/[interface_discard_cnt]|...
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u/%u/%u/%u,%u/%u/%u/%u/%u/%u/%u",
                  stats.client_packets_relayed,
                  stats.client_packet_errors,
                  stats.receive_server_packets,
                  stats.missing_agent_option,
                  stats.missing_circuit_id,
                  stats.missing_remote_id,
                  stats.bad_circuit_id,
                  stats.bad_remote_id,
                  stats.server_packets_relayed,
                  stats.server_packet_errors,
                  stats.receive_client_packets,
                  stats.receive_client_agent_option,
                  stats.replace_agent_option,
                  stats.keep_agent_option,
                  stats.drop_agent_option);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_dhcp_relay, "/config/dhcp_relay", handler_config_dhcp_relay);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_dhcp_relay_statistics, "/stat/dhcp_relay_statistics", handler_stat_dhcp_relay_statistics);

