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
#include <vtss/appl/port.h>
#include "port_iter.hxx"
#include "loop_protect_api.h"
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

static i32 handler_config_loop_protect(CYG_HTTPD_STATE *p)
{
    vtss_isid_t sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int ct, var;
    port_iter_t pit;
    vtss_appl_loop_protect_conf_t conf, newconf;

    if(redirectUnmanagedOrInvalid(p, sid)) /* Redirect unmanaged/invalid access to handler */
        return -1;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LOOP_PROTECT)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (vtss_appl_loop_protect_conf_get(&conf) < 0) {
            errors++;   /* Probably stack error */
        } else {
            mesa_rc rc;
            newconf = conf;

            if (cyg_httpd_form_varable_int(p, "gbl_enable", &var))
                newconf.enabled = var;
            if (cyg_httpd_form_varable_int(p, "txtime", &var))
                newconf.transmission_time = var;
            if (cyg_httpd_form_varable_int(p, "shuttime", &var))
                newconf.shutdown_time = var;

            if ((rc = vtss_appl_loop_protect_conf_set(&newconf)) < 0) {
                T_D("%s: failed rc = %d", __FUNCTION__, rc);
                errors++; /* Probably stack error */
            }

            /* Ports */
            (void) port_iter_init(&pit, NULL, sid,
                                  PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                vtss_appl_loop_protect_port_conf_t pconf;
                int i = pit.iport, uport = pit.uport;
                if((rc = vtss_appl_loop_protect_conf_port_get(sid, i, &pconf)) == VTSS_RC_OK) {
                    pconf.enabled = cyg_httpd_form_variable_check_fmt(p, "enable_%d", uport);
                    if(cyg_httpd_form_variable_int_fmt(p, &var, "action_%d", uport))
                        pconf.action = (vtss_appl_loop_protect_action_t)var;
                    if(cyg_httpd_form_variable_int_fmt(p, &var, "txmode_%d", uport))
                        pconf.transmit = var;
                    if((rc = vtss_appl_loop_protect_conf_port_set(sid, i, &pconf)) != VTSS_RC_OK) {
                        T_W("rc = %d", rc);
                        errors++;
                    }
                } else {
                    T_W("rc = %d", rc);
                    errors++;
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/loop_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        /* should get these values from management APIs */
        if (vtss_appl_loop_protect_conf_get(&conf) == VTSS_RC_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%d,%d,",
                          conf.enabled, conf.transmission_time, conf.shutdown_time);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        /* Ports */
        (void) port_iter_init(&pit, NULL, sid,
                              PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_appl_loop_protect_port_conf_t pconf;
            int i = pit.iport, uport = pit.uport;
            if(vtss_appl_loop_protect_conf_port_get(sid, i, &pconf) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d/%d/%d|",
                              uport,
                              pconf.enabled,
                              pconf.action,
                              pconf.transmit);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(cb_handler_config_loop_protect, "/config/loop_config", handler_config_loop_protect);

static i32 handler_status_loop_protect(CYG_HTTPD_STATE *p)
{
    vtss_isid_t sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int ct;
    port_iter_t pit;
    vtss_appl_loop_protect_conf_t conf;

    if(redirectUnmanagedOrInvalid(p, sid) ||
       vtss_appl_loop_protect_conf_get(&conf) != VTSS_RC_OK) /* Redirect unmanaged/invalid access to handler */
        return -1;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LOOP_PROTECT)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");
    (void) port_iter_init(&pit, NULL, sid,
                          PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (conf.enabled && port_iter_getnext(&pit)) {
        vtss_appl_loop_protect_port_conf_t pconf;
        int i = pit.iport;
        if(vtss_appl_loop_protect_conf_port_get(sid, i, &pconf) == VTSS_RC_OK) {
            vtss_appl_loop_protect_port_info_t linfo;
            vtss_appl_port_status_t pinfo;
            vtss_ifindex_t ifindex;
            if (vtss_ifindex_from_port(sid, i, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
            };

            if(pconf.enabled &&
               vtss_appl_loop_protect_port_info_get(sid, i, &linfo) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%s/%s/%u/%s/%s/%s|",
                              pit.uport,
                              loop_protect_action2string(pconf.action),
                              pconf.transmit ? "Enabled" : "Disabled",
                              linfo.loops,
                              linfo.disabled ? "Disabled" : 
                              ((vtss_appl_port_status_get(ifindex, &pinfo) == VTSS_RC_OK) && pinfo.link) ? "Up" : "Down",
                              linfo.loop_detect ? "Loop" : "-",
                              linfo.loops ? misc_time2str(linfo.last_loop) : "-");
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
    }
    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(cb_handler_status_loop_protect, "/stat/loop_status", handler_status_loop_protect);
