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

#include "web_api.h"
#include "port_iter.hxx"
#include "udld.h"
#include "udld_api.h"
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

static i32 handler_config_udld(CYG_HTTPD_STATE *p)
{
    vtss_isid_t sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int ct, var;
    port_iter_t pit;
    mesa_rc rc = TRUE;
    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_UDLD)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        /* Ports */
        (void) port_iter_init(&pit, NULL, sid,
                              PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_appl_udld_port_conf_struct_t conf;
            int i = pit.iport, uport = pit.uport;
            if (vtss_appl_udld_port_conf_get(sid, i, &conf) == VTSS_RC_OK) {
                if (cyg_httpd_form_variable_int_fmt(p, &var, "action_%d", uport)) {
                    conf.udld_mode = (vtss_appl_udld_mode_t)var;
                }
                if (cyg_httpd_form_variable_int_fmt(p, &var, "msg_inter_%d", uport)) {
                    if ((var > 6) && (var < 91)) {
                        conf.probe_msg_interval = var;
                    }
                }
                if (vtss_appl_udld_port_conf_set(sid, i, &conf) != VTSS_RC_OK) {
                    T_W("rc = %d", rc);
                    errors++;
                }
            } else {
                T_W("rc = %d", rc);
                errors++;
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/udld_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        /* Ports */
        (void) port_iter_init(&pit, NULL, sid,
                              PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_appl_udld_port_conf_struct_t conf;
            int i = pit.iport, uport = pit.uport;
            if (vtss_appl_udld_port_conf_get(sid, i, &conf) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d/%d|",
                              uport,
                              conf.udld_mode,
                              conf.probe_msg_interval);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(cb_handler_config_udld, "/config/udld_config", handler_config_udld);

static const char *web_udld_detection_state_str(vtss_udld_detection_state_t detection_state)
{
    const char *str;
    switch (detection_state) {
    case VTSS_UDLD_DETECTION_STATE_LOOPBACK:
        str = "Loopback";
        break;
    case VTSS_UDLD_DETECTION_STATE_BI_DIRECTIONAL:
        str = "Bi-directional";
        break;
    case VTSS_UDLD_DETECTION_STATE_UNI_DIRECTIONAL:
        str = "Uni-directional";
        break;
    case VTSS_UDLD_DETECTION_STATE_NEIGHBOR_MISMATCH:
        str = "Neighbor mismatch";
        break;
    case VTSS_UDLD_DETECTION_STATE_MULTIPLE_NEIGHBOR:
        str = "Multiple neighbors connected";
        break;
    case VTSS_UDLD_DETECTION_STATE_UNKNOWN:
    default:
        str = "Indeterminant";
    }
    return str;
}

static i32 handler_udld_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                                 isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t                                 pit;
    int                                         ct = 0;
    BOOL                                        rc = TRUE, rc_admin = TRUE;
    vtss_appl_udld_admin_t                      admin;
    vtss_appl_udld_neighbor_info_t              info;
    vtss_appl_udld_port_info_t                  port_info;
    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LACP)) {
        return -1;
    }
#endif
    cyg_httpd_start_chunked("html");
    // Loop through all front ports
    if (port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI) == VTSS_RC_OK) {
        while (port_iter_getnext(&pit)) {
            rc_admin = TRUE;
            rc = TRUE;
            if (vtss_appl_udld_port_admin_get(isid, pit.iport, &admin) != VTSS_RC_OK) {
                rc_admin = FALSE;
            }
            if (vtss_appl_udld_port_info_get(isid, pit.iport, &port_info) != VTSS_RC_OK) {
                rc = FALSE;
            }
            if (rc == TRUE && rc_admin == TRUE) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "%u/%s/%s/%s/%s,",
                              pit.uport,
                              admin == VTSS_APPL_UDLD_ADMIN_ENABLE ? "Enable" : "Disable",
                              port_info.device_id,
                              port_info.device_name,
                              web_udld_detection_state_str(port_info.detection_state)
                             );
                if (ct <= 0) {
                    T_D("Failed to do snprintf\n");
                }
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            if (vtss_appl_udld_neighbor_info_get_first(isid, pit.iport, &info) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "%s/%s/%s/%s,",
                              info.port_id,
                              info.device_id,
                              web_udld_detection_state_str(info.detection_state),
                              info.device_name
                             );
                cyg_httpd_write_chunked(p->outbuffer, ct);
                while (info.next != NULL) {
                    if (vtss_appl_udld_neighbor_info_get_next(isid, pit.iport, &info)  != VTSS_RC_OK) {
                        break;
                    }
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                  "%s/%s/%s/%s,",
                                  info.port_id,
                                  info.device_id,
                                  web_udld_detection_state_str(info.detection_state),
                                  info.device_name
                                 );
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    }
    cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}
CYG_HTTPD_HANDLER_TABLE_ENTRY(cb_handler_udld_status, "/stat/udld_status", handler_udld_status);
