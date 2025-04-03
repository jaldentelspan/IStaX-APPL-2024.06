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
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_DDMI
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DDMI

#include <vtss_module_id.h>
#include "port_api.h"
#include "port_iter.hxx"
#include "ddmi_api.h"

// Allow type++ on the following type
VTSS_ENUM_INC(vtss_appl_ddmi_monitor_type_t);

/******************************************************************************/
// DDMI_handler_overview()
/******************************************************************************/
static int32_t DDMI_handler_overview(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                  sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ddmi_global_conf_t conf;
    vtss_appl_ddmi_port_status_t status;
    vtss_ifindex_t               ifindex;
    bool                         first = true;
    port_iter_t                  pit;
    int                          ct;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DDMI)) {
        return -1;
    }
#endif

    (void)vtss_appl_ddmi_global_conf_get(&conf);

    // Format:
    /* 1/ZyXEL/SFP-SX-D/S111132000061/V1.0/2011-08-10/1000BASE-SX|2/ZyXEL/SFP-SX-D/S111132000061/V1.0/2011-08-10/1000BASE-SX|3/ZyXEL/SFP-SX-D/S111132000061/V1.0/2011-08-10/1000BASE-SX|4/-/-/-/-/-/- */

    cyg_httpd_start_chunked("html");

    (void)port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, pit.iport, &ifindex);
        if (vtss_appl_ddmi_port_status_get(ifindex, &status) != VTSS_RC_OK) {
            continue;
        }

        if (!status.a0_supported) {
            continue;
        }

        if (!first) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        if (status.sfp_detected && conf.admin_enable) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s/%s/%s/%s/%s/%s",
                          pit.uport,
                          status.vendor,
                          status.part_number,
                          status.serial_number,
                          status.revision,
                          status.date_code,
                          port_sfp_transceiver_to_txt(status.sfp_type));
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/-/-/-/-/-/-", pit.uport);
        }

        cyg_httpd_write_chunked(p->outbuffer, ct);
        first = false;
    }

    cyg_httpd_end_chunked();

    return -1;
}

/******************************************************************************/
// DDMI_handler_detailed()
/******************************************************************************/
static int32_t DDMI_handler_detailed(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                   sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_ddmi_global_conf_t  conf;
    vtss_appl_ddmi_port_status_t  status;
    vtss_appl_ddmi_monitor_type_t type;
    vtss_ifindex_t                ifindex;
    bool                          first = true;
    port_iter_t                   pit;
    vtss_uport_no_t               selected_uport = 0;
    mesa_port_no_t                iport = 0;
    int                           ct, val;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DDMI)) {
        return -1;
    }
#endif

    if (cyg_httpd_form_varable_int(p, "port", &val)) {
        selected_uport = val;
    }

    (void)vtss_appl_ddmi_global_conf_get(&conf);

    // Format:
    /* 8,12,14|id/ZyXEL/SFP-SX-D/S111132000061/V1.0/2011-08-10/1000BASE-SX|1/2/3/4/5|18/2/1/4/5|65535/2/4/4/2|1/2/3/4/5|18/2/1/4/5 */

    cyg_httpd_start_chunked("html");

    // Start with a list of SFP ports
    (void)port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, pit.iport, &ifindex);
        if (vtss_appl_ddmi_port_status_get(ifindex, &status) != VTSS_RC_OK) {
            continue;
        }

        if (!status.a0_supported) {
            continue;
        }

        if (!first) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u", pit.uport);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        if (first) {
            iport = pit.iport;
            first = false;
        }
    }

    if (selected_uport) {
        iport = uport2iport(selected_uport);
    }

    if (first) {
        // No SFP ports on this DUT
        cyg_httpd_end_chunked();
        return -1;
    }

    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, iport, &ifindex);
    (void)vtss_appl_ddmi_port_status_get(ifindex, &status);

    if (status.sfp_detected && conf.admin_enable) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u/%s/%s/%s/%s/%s/%s",
                      iport2uport(iport),
                      status.vendor,
                      status.part_number,
                      status.serial_number,
                      status.revision,
                      status.date_code,
                      port_sfp_transceiver_to_txt(status.sfp_type));
    } else {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u/-/-/-/-/-/-", iport2uport(iport));

    }
    cyg_httpd_write_chunked(p->outbuffer, ct);

    if (status.a2_supported & conf.admin_enable) {
        for (type = (vtss_appl_ddmi_monitor_type_t)0; type < VTSS_APPL_DDMI_MONITOR_TYPE_CNT; type++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%s/%s/%s/%s/%s/%s",
                          status.monitor_status[type].current,
                          ddmi_monitor_state_to_txt(status.monitor_status[type].state),
                          status.monitor_status[type].warn_lo,
                          status.monitor_status[type].warn_hi,
                          status.monitor_status[type].alarm_lo,
                          status.monitor_status[type].alarm_hi);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    } else {
        for (type = (vtss_appl_ddmi_monitor_type_t)0; type < VTSS_APPL_DDMI_MONITOR_TYPE_CNT; type++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|-/-/-/-/-/-");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    }

    cyg_httpd_end_chunked();

    return -1;
}

/******************************************************************************/
// DDMI_handler_config()
/******************************************************************************/
static int32_t DDMI_handler_config(CYG_HTTPD_STATE *p)
{
    vtss_appl_ddmi_global_conf_t conf;
    int                          var_value;
    int                          ct;
    mesa_rc                      rc;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DDMI)) {
        return -1;
    }
#endif

    (void)vtss_appl_ddmi_global_conf_get(&conf);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        if (cyg_httpd_form_varable_int(p, "ddmi_mode", &var_value)) {
            conf.admin_enable = var_value;
        }

        if ((rc = vtss_appl_ddmi_global_conf_set(&conf)) != VTSS_RC_OK) {
            T_E("vtss_appl_ddmi_global_conf_set() failed: %s", error_txt(rc));
        }

        redirect(p, "/ddmi_config.htm");
    } else {
        // Format: [ddmi_mode]
        (void)cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d", conf.admin_enable);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        (void)cyg_httpd_end_chunked();
    }

    return -1;
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_ddmi_overview, "/stat/ddmi_overview", DDMI_handler_overview);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_ddmi_detail,   "/stat/ddmi_detailed", DDMI_handler_detailed);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ddmi,   "/config/ddmi",        DDMI_handler_config);
