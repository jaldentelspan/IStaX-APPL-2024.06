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

/*lint -esym(459,err_msg)*/

#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#include "l2proto_api.h"


#include "thermal_protect_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_web_api.h"
#endif


/****************************************************************************/
/*    Trace definition                                                      */
/****************************************************************************/
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
//
// THERMAL_PROTECT config handler
//

i32 handler_config_thermal_protect_config(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                   ct;
    static char           err_msg[100]; // Need to be static because it is set when posting data, but used getting data.
    vtss_appl_thermal_protect_switch_conf_t      switch_conf, newconf;
    mesa_rc               rc;
    char                  form_name[50];
    int                   i;
    port_iter_t           pit;

    if (!thermal_protect_module_enabled()) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET)) {
        return -1;
    }
#endif

    //
    // Setting new configuration
    //
    if (web_get_method(p, VTSS_MODULE_ID_GREEN_ETHERNET) == CYG_HTTPD_METHOD_POST) {
        // Get configuration for the selected switch
        vtss_appl_thermal_protect_switch_conf_get(&switch_conf, isid); // Get current configuration
        newconf = switch_conf;

        // Group Temperatures
        for (i = 0; i < VTSS_APPL_THERMAL_PROTECT_GROUP_CNT; i++) {
            sprintf(form_name, "thermal_grp_temp_%d", i); // Set form name
            newconf.glbl_conf.grp_temperatures[i] =  httpd_form_get_value_int(p, form_name, VTSS_APPL_THERMAL_PROTECT_TEMP_MIN, VTSS_APPL_THERMAL_PROTECT_TEMP_MAX);
        }

        // Loop through all front ports
        if (port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                // Group where to start thermal_protect per port.
                sprintf(form_name, "thermal_port_grp_%u", pit.iport); // Set form name
                newconf.local_conf.port_grp[pit.iport] = httpd_form_get_value_int(p, form_name, VTSS_APPL_THERMAL_PROTECT_GROUP_MIN_VALUE, VTSS_APPL_THERMAL_PROTECT_GROUP_CNT);
                T_D_PORT(pit.iport, "port_grp = %lu", newconf.local_conf.port_grp[pit.iport]);
            }
        }

        // Set current configuration
        if (memcmp(&switch_conf, &newconf, sizeof(newconf)) && (rc = vtss_appl_thermal_protect_switch_conf_set(&newconf, isid)) != VTSS_RC_OK) {
            misc_strncpyz(err_msg, error_txt(rc), 100); // Set error message
        } else {
            strcpy(err_msg, ""); // Clear error message
        }

        redirect(p, "/thermal_protect_config.htm");

    } else if (web_get_method(p, VTSS_MODULE_ID_GREEN_ETHERNET) == CYG_HTTPD_METHOD_GET) {
        //
        // Getting the configuration.
        //

        cyg_httpd_start_chunked("html");

        // Apply error message (If any )
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", err_msg);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        strcpy(err_msg, ""); // Clear error message


        //
        // Pass configuration to Web
        //

        // Get configuration for the global configuration
        vtss_appl_thermal_protect_switch_conf_get(&switch_conf, VTSS_ISID_LOCAL); // Get current configuration
        for (i = 0; i < VTSS_APPL_THERMAL_PROTECT_GROUP_CNT; i++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d",
                          switch_conf.glbl_conf.grp_temperatures[i]);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            // Split with "," between each group temperature, and end with a "|".
            if (i == VTSS_APPL_THERMAL_PROTECT_GROUP_CNT - 1) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",");
            }
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        // Port groups
        // Get configuration for the local configuration for the switch
        vtss_appl_thermal_protect_switch_conf_get(&switch_conf, isid); // Get current configuration
        // Loop through all front ports
        if (port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d",
                              switch_conf.local_conf.port_grp[pit.iport]);
                cyg_httpd_write_chunked(p->outbuffer, ct);

                // Split with "," between each group
                if (!pit.last) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",");
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }
        }
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

//
// THERMAL_PROTECT status handler
//
i32 handler_status_thermal_protect_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                                 isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                                         ct;
    vtss_appl_thermal_protect_local_status_t    switch_status;
    mesa_rc                                     rc;
    char                                        err_msg[100]; // Buffer for holding error messages
    port_iter_t                                 pit;

    if (!thermal_protect_module_enabled()) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET)) {
        return -1;
    }
#endif

    if (web_get_method(p, VTSS_MODULE_ID_GREEN_ETHERNET) == CYG_HTTPD_METHOD_GET) {
        strcpy(err_msg, ""); // Clear error message

        // The Data is split like this: err_msg|chip_temp|thermal_protect_speed|

        // Get status for the selected switch
        if ((rc = vtss_appl_thermal_protect_get_switch_status(&switch_status, isid))) {
            misc_strncpyz(err_msg, error_txt(rc), 100); // Set error message
        }

        cyg_httpd_start_chunked("html");

        // Apply error message (If any )
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", err_msg);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        //
        // Pass status to Web
        //
        // Loop through all front ports
        if (port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d&%s|",
                              switch_status.port_temp[pit.iport],
                              thermal_protect_power_down2txt(switch_status.port_powered_down[pit.iport]));

                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_thermal_protect_config, "/config/thermal_protect_config", handler_config_thermal_protect_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_status_thermal_protect_status, "/stat/thermal_protect_status", handler_status_thermal_protect_status);
#endif //VTSS_SW_OPTION_WEB

