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

#include <stdio.h>
#include "main.h"
#include "web_api.h"
#include "vtss_upnp_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_UPNP

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

static i32 handler_config_upnp(CYG_HTTPD_STATE *p)
{
    int         ct;
    vtss_appl_upnp_param_t conf, newconf;
    int         var_value;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_UPNP)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        if (vtss_appl_upnp_system_config_get(&conf) == VTSS_RC_OK) {
            newconf = conf;

            //mode
            if (cyg_httpd_form_varable_int(p, "mode", &var_value)) {
                newconf.mode = var_value;
            }

            //ttl
            if (cyg_httpd_form_varable_int(p, "ttl", &var_value)) {
                newconf.ttl = var_value;
            }

            //interval
            if (cyg_httpd_form_varable_int(p, "interval", &var_value)) {
                newconf.adv_interval = var_value;
            }

            //ipmode
            if (cyg_httpd_form_varable_int(p, "ipmode", &var_value)) {
                newconf.ip_addressing_mode = (vtss_appl_upnp_ip_addressing_mode_t)var_value;
            }

            //vid
            if (cyg_httpd_form_varable_int(p, "vid", &var_value)) {
                newconf.static_ifindex = vtss_ifindex_cast_from_u32(var_value, VTSS_IFINDEX_TYPE_VLAN);
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                T_D("Calling vtss_appl_upnp_system_config_set()");
                if (vtss_appl_upnp_system_config_set(&newconf) < 0) {
                    T_D("vtss_appl_upnp_system_config_set(): failed");
                }
            }
        }
        redirect(p, "/upnp.htm");

    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [mode]/[ttl]/[interval]
        */

        if (vtss_appl_upnp_system_config_get(&conf) == VTSS_RC_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d/%d/%d/%d",
                          conf.mode,
                          conf.ttl,
                          conf.adv_interval,
                          conf.ip_addressing_mode,
                          vtss_ifindex_cast_to_u32(conf.static_ifindex));

            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_upnp, "/config/upnp", handler_config_upnp);

