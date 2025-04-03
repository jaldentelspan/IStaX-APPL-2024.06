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
#include "lacp_api.h"
#include "mgmt_api.h"
#include "lacp.h"
#include "port_api.h"
#include "port_iter.hxx"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static i32 handler_config_lacp_ports(CYG_HTTPD_STATE *p)
{
    vtss_isid_t              isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_uport_no_t          uport;
    int                      ct;
    vtss_lacp_port_config_t  conf;
    const char               *err = NULL;
    mesa_rc                  rc;
    port_iter_t              pit;
    vtss_lacp_system_config_t   conf_system;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LACP)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int val;
        // LACP port configuration
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
        while (port_iter_getnext(&pit)) {
            if (lacp_mgmt_port_conf_get(isid, pit.iport, &conf) != VTSS_RC_OK) {
                continue;
            }

            vtss_lacp_port_config_t new_conf, *old = &conf;
            uport = iport2uport(pit.iport);
            new_conf = *old;

            if (cyg_httpd_form_variable_int_fmt(p, &val, "timeout_%d", uport)) {
                new_conf.xmit_mode = val;
            }

            if (cyg_httpd_form_variable_int_fmt(p, &val, "prio_%d", uport)) {
                new_conf.port_prio = val;
            }

            if (memcmp(&new_conf, old, sizeof(*old)) != 0) {
                *old = new_conf;
                if ((rc = lacp_mgmt_port_conf_set(isid, pit.iport, &conf)) != VTSS_RC_OK) {
                    if (rc == LACP_ERROR_STATIC_AGGR_ENABLED) {
                        err = "LACP and Static aggregation can not both be enabled on the same ports";
                    } else if (rc == LACP_ERROR_DOT1X_ENABLED) {
                        err = "LACP cannot be enabed on ports whose 802.1X Admin State is not Authorized";
                    } else {
                        err = "Unspecified error.";
                    }
                }
            }
        }

        // LACP system configuration
        if (lacp_mgmt_system_conf_get(&conf_system) == VTSS_RC_OK) {
            if (cyg_httpd_form_varable_int(p, "system_priority", &val)) {
                conf_system.system_prio = val;
            }
            if (lacp_mgmt_system_conf_set(&conf_system) != VTSS_RC_OK) {
                err = "LACP cannot set system priority.";
            }
        }

        if (err != NULL) {
            send_custom_error(p, "LACP Error", err, strlen(err));
        } else {
            redirect(p, "/lacp_port_config.htm");
        }

    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */

        cyg_httpd_start_chunked("html");

        // LACP port configuration
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
        while (port_iter_getnext(&pit)) {
            if (lacp_mgmt_port_conf_get(isid, pit.iport, &conf) != VTSS_RC_OK) {
                continue;
            }

            const vtss_lacp_port_config_t *pp = &conf;
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%d/%d/%d|",
                          iport2uport(pit.iport),
                          pp->enable_lacp,
                          pp->xmit_mode,
                          pp->port_prio);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        // LACP system priority
        if (lacp_mgmt_system_conf_get(&conf_system) == VTSS_RC_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%d",
                          conf_system.system_prio);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_stat_lacp_internal_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                  isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t                  pit;
    l2_port_no_t                 l2port;
    vtss_lacp_port_config_t      conf;
    vtss_lacp_portstatus_t       stat;
    int                          ct;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        l2port = L2PORT2PORT(isid, pit.iport);
        if (lacp_mgmt_port_conf_get(isid, pit.iport, &conf) != VTSS_RC_OK || lacp_mgmt_port_status_get(l2port, &stat) != VTSS_RC_OK) {
            continue;
        }

        // only show LACP-enable ports
        if (!conf.enable_lacp) {
            continue;
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s/%u/%u/%s/%s/%s/%s/%s/%s/%s/%s|",
                      pit.uport,
                      stat.port_state == LACP_PORT_ACTIVE ? "Active" : stat.port_state == LACP_PORT_STANDBY ? "Standby" : "Down",
                      stat.actor_oper_port_key,
                      conf.port_prio,
                      stat.actor_lacp_state & VTSS_LACP_PORTSTATE_LACP_ACTIVITY ? "Active" : "Passive",
                      stat.actor_lacp_state & VTSS_LACP_PORTSTATE_LACP_TIMEOUT ? "Fast" : "Slow",
                      stat.actor_lacp_state & VTSS_LACP_PORTSTATE_AGGREGATION ? "Yes" : "No",
                      stat.actor_lacp_state & VTSS_LACP_PORTSTATE_SYNCHRONIZATION ? "Yes" : "No",
                      stat.actor_lacp_state & VTSS_LACP_PORTSTATE_COLLECTING ? "Yes" : "No",
                      stat.actor_lacp_state & VTSS_LACP_PORTSTATE_DISTRIBUTING ? "Yes" : "No",
                      stat.actor_lacp_state & VTSS_LACP_PORTSTATE_DEFAULTED ? "Yes" : "No",
                      stat.actor_lacp_state & VTSS_LACP_PORTSTATE_EXPIRED ? "Yes" : "No");

        cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}


static i32 handler_stat_lacp_neighbor_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                  isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t                  pit;
    l2_port_no_t                 l2port;
    vtss_lacp_portstatus_t       stat;
    int                          search_aid, return_aid;
    BOOL                         first_search;
    aggr_mgmt_group_no_t         aggr_no;
    int                          ct;
    u32                          partner_key[AGGR_MGMT_GROUP_NO_END_];

    memset(partner_key, 0, sizeof(u32) * AGGR_MGMT_GROUP_NO_END_);
    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP)) {
        return -1;
    }
#endif

    first_search = TRUE;
    while (aggr_mgmt_lacp_id_get_next(first_search ? NULL : &search_aid, &return_aid) == VTSS_RC_OK) {
        search_aid = return_aid;
        first_search = FALSE;

        vtss_lacp_aggregatorstatus_t aggr;
        if (!lacp_mgmt_aggr_status_get(return_aid, &aggr)) {
            continue;
        }

        partner_key[mgmt_aggr_no2id(lacp_to_aggr_id(aggr.aggrid))] = aggr.partner_oper_key;
    }

    cyg_httpd_start_chunked("html");

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        l2port = L2PORT2PORT(isid, pit.iport);
        if (lacp_mgmt_port_status_get(l2port, &stat) != VTSS_RC_OK) {
            continue;
        }

        // only show LACP-enable ports with valid neighbor state
        if (!stat.port_enabled || stat.partner_lacp_state == 0) {
            continue;
        }

        aggr_no = lacp_to_aggr_id(stat.actor_port_aggregator_identifier);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s/%u/%u/%u/%u/%s/%s/%s/%s/%s/%s/%s/%s|",
                      pit.uport,
                      stat.port_state == LACP_PORT_ACTIVE ? "Active" : stat.port_state == LACP_PORT_STANDBY ? "Standby" : "Down",
                      mgmt_aggr_no2id(aggr_no),
                      partner_key[aggr_no],
                      stat.partner_oper_port_number,
                      stat.partner_oper_port_priority,
                      stat.partner_lacp_state & VTSS_LACP_PORTSTATE_LACP_ACTIVITY ? "Active" : "Passive",
                      stat.partner_lacp_state & VTSS_LACP_PORTSTATE_LACP_TIMEOUT ? "Fast" : "Slow",
                      stat.partner_lacp_state & VTSS_LACP_PORTSTATE_AGGREGATION ? "Yes" : "No",
                      stat.partner_lacp_state & VTSS_LACP_PORTSTATE_SYNCHRONIZATION ? "Yes" : "No",
                      stat.partner_lacp_state & VTSS_LACP_PORTSTATE_COLLECTING ? "Yes" : "No",
                      stat.partner_lacp_state & VTSS_LACP_PORTSTATE_DISTRIBUTING ? "Yes" : "No",
                      stat.partner_lacp_state & VTSS_LACP_PORTSTATE_DEFAULTED ? "Yes" : "No",
                      stat.partner_lacp_state & VTSS_LACP_PORTSTATE_EXPIRED ? "Yes" : "No");

        cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

static i32 handler_stat_lacp_sys_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                          ct;
    vtss_lacp_aggregatorstatus_t aggr;
    char                         buf[320];
    char                         portbuf[100];
    l2_port_no_t                 l2port;
    int                          first;
    mesa_port_no_t               iport;
    vtss_isid_t                  isid_id = VTSS_ISID_START;
    const char                   *aggr_id;
    int                          search_aid, return_aid;
    BOOL                         first_search = 1;
    vtss_lacp_system_config_t    sysconf;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    // Get config from base, not mgmt. The reason is that the MAC isn't really configurable, so we can't use
    // the value stored in mgmt; we need to get the real value from base. But prio is correctly stored in both
    // places, so we might as well use the one from base as well.
    vtss_lacp_get_config(&sysconf);

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s|",
                  sysconf.system_prio,
                  misc_mac_txt(sysconf.system_id.macaddr, buf));

    cyg_httpd_write_chunked(p->outbuffer, ct);

    // write LACP aggregation status for each group
    while (aggr_mgmt_lacp_id_get_next(first_search ? NULL : &search_aid,  &return_aid) == VTSS_RC_OK) {
        search_aid = return_aid;
        first_search = 0;
        if (lacp_mgmt_aggr_status_get(return_aid, &aggr)) {
            int used = 0;
            portbuf[sizeof(portbuf) - 1] = '\0';
            first = 1;
            for (l2port = 0; l2port < VTSS_LACP_MAX_PORTS_; l2port++) {
                if (aggr.port_list[l2port]) {
                    if (used >= sizeof(portbuf) - 1) {
                        T_E("Not room in string for all ports");
                        break;
                    }

                    used += snprintf(portbuf + used, sizeof(portbuf) - used - 1, "%s%s", first ? "" : ",", l2port2str(l2port));
                    if (first) {
                        if (!l2port2port(l2port, &isid_id, &iport)) {
                            continue;
                        }
                    }
                    first = 0;
                }
            }

            if (AGGR_MGMT_GROUP_IS_LAG(lacp_to_aggr_id(aggr.aggrid))) {
                aggr_id = l2port2str(L2LLAG2PORT(isid_id, lacp_to_aggr_id(aggr.aggrid) - AGGR_MGMT_GROUP_NO_START));
            } else {
                aggr_id = l2port2str(L2GLAG2PORT(lacp_to_aggr_id(aggr.aggrid) - AGGR_MGMT_GLAG_START_ + VTSS_GLAG_NO_START));
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%s/%s/%d/%d/%s/%s|",
                          aggr_id,
                          misc_mac_txt(aggr.partner_oper_system.macaddr, buf),
                          aggr.partner_oper_system_priority,
                          aggr.partner_oper_key,
                          misc_time2interval(aggr.secs_since_last_change),
                          portbuf);

            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    }

    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

static i32 handler_stat_lacp_statistics(CYG_HTTPD_STATE *p)
{
    vtss_isid_t isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                    ct;
    mesa_port_no_t         iport;
    vtss_lacp_portstatus_t pp;
    u32                    port_count = port_count_max();
    vtss_lacp_port_config_t conf;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    for (iport = VTSS_PORT_NO_START; iport < port_count; iport++) {
        if (cyg_httpd_form_varable_find(p, "clear")) {    /* Clear? */
            lacp_mgmt_statistics_clear(L2PORT2PORT(isid, iport));
        }

        if (lacp_mgmt_port_conf_get(isid, iport, &conf) != VTSS_RC_OK ||
            lacp_mgmt_port_status_get(L2PORT2PORT(isid, iport), &pp) != VTSS_RC_OK) {
            continue;
        }
        if (!conf.enable_lacp) {
            continue;
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%u/%lu/%lu/%lu/%lu|",
                      iport2uport(iport),
                      pp.port_stats.lacp_frame_recvs,
                      pp.port_stats.lacp_frame_xmits,
                      pp.port_stats.illegal_frame_recvs,
                      pp.port_stats.unknown_frame_recvs);
        cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_lacp_ports, "/config/lacp_ports", handler_config_lacp_ports);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_lacp_internal_status, "/stat/lacp_internal_status", handler_stat_lacp_internal_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_lacp_neighbor_status, "/stat/lacp_neighbor_status", handler_stat_lacp_neighbor_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_lacp_sys_status, "/stat/lacp_sys_status", handler_stat_lacp_sys_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_lacp_statistics, "/stat/lacp_statistics", handler_stat_lacp_statistics);

