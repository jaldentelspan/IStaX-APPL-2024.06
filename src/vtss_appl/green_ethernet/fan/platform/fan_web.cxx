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

/*lint -esym(459,err_msg)*/
#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#include "l2proto_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif
#include "fan_api.h"
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
// FAN config handler
//

static i32 handler_config_fan_config(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                   ct;
    static char           err_msg[100]; // Need to be static because it is set when posting data, but used getting data.
    vtss_appl_fan_conf_t      local_conf;
    mesa_rc               rc;

    if (!fan_module_enabled()) {
        return -1;
    }

    // The Data is split like this: err_msg|t_max|t_on|pwm|

    T_D ("Fan_Config  web access - ISID =  %d", isid );

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

    T_D ("Fan_Config  web access - ISID =  %d", isid );
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET)) {
        return -1;
    }
#endif

    T_D ("Fan_Config  web access - ISID =  %d", isid );

    // Get configuration for the selected switch
    (void)vtss_appl_fan_conf_get(&local_conf); // Get current configuration

    //
    // Setting new configuration
    //
    if (p->method == CYG_HTTPD_METHOD_POST) {

        // Temperature maximum
        local_conf.glbl_conf.t_max =  httpd_form_get_value_int(p, "t_max", VTSS_APPL_FAN_TEMP_MIN, VTSS_APPL_FAN_TEMP_MAX);


        // Temperature where to start fan.
        local_conf.glbl_conf.t_on = httpd_form_get_value_int(p, "t_on", VTSS_APPL_FAN_TEMP_MIN, VTSS_APPL_FAN_TEMP_MAX);
        // pwm frequency to use
        local_conf.glbl_conf.pwm = str_to_mesa_fan_pwd_freq_t(cyg_httpd_form_varable_find(p, "pwm"));

        // Set current configuration
        if ((rc = vtss_appl_fan_conf_set(&local_conf)) != VTSS_RC_OK) {
            T_W("%s", fan_error_txt(rc));
            misc_strncpyz(err_msg, fan_error_txt(rc), 100); // Set error message
        } else {
            strcpy(err_msg, ""); // Clear error message
        }
        redirect(p, "/fan_config.htm");
    } else {
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
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%d|%s",
                      local_conf.glbl_conf.t_max,
                      local_conf.glbl_conf.t_on,
                      mesa_fan_pwd_freq_t_to_str(local_conf.glbl_conf.pwm));

        cyg_httpd_write_chunked(p->outbuffer, ct);
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}


//
// FAN status handler
//

static i32 handler_status_fan_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t               isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                       ct;
    vtss_appl_fan_status_t    switch_status;
    mesa_rc                   rc;
    char                      err_msg[100]; // Buffer for holding error messages
    T_D ("Fan_Status  web access - ISID =  %d", isid );

    if (!fan_module_enabled()) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET)) {
        return -1;
    }
#endif

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }


    strcpy(err_msg, ""); // Clear error message

    // The Data is split like this: err_msg|chip_temp|fan_speed|
    // Get status for the selected switch
    if ((rc = vtss_appl_fan_status_get(&switch_status, isid))) {
        misc_strncpyz(err_msg, fan_error_txt(rc), 100); // Set error message
    }

    cyg_httpd_start_chunked("html");

    // Apply error message (If any )
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", err_msg);
    cyg_httpd_write_chunked(p->outbuffer, ct);



    //
    // Pass status to Web
    //
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u|", switch_status.fan_speed);
    cyg_httpd_write_chunked(p->outbuffer, ct);


    u8 sensor_id;
    u8 sensor_cnt;
    (void)vtss_appl_fan_temperature_sensors_count_get(isid, &sensor_cnt);
    for (sensor_id = 0; sensor_id < sensor_cnt; sensor_id++) {
        if (sensor_id == 0) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u",
                          switch_status.chip_temp[sensor_id]);

        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "!%u",
                          switch_status.chip_temp[sensor_id]);
        }
        cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_fan_config, "/config/fan_config", handler_config_fan_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_status_fan_status, "/stat/fan_status", handler_status_fan_status);
#endif // VTSS_SW_OPTION_WEB

