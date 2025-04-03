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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "sysutil_api.h"
#include "vtss_os_wrapper.h"
//#include "cyg/athttpd/auth.h"

#define TM_ICFG_STR "temperature monitor %s low %d high %d critical %d hysteresis %d\n"

/* ICFG callback functions */
static mesa_rc SYSUTIL_icfg(
    IN const vtss_icfg_query_request_t  *req,
    IN vtss_icfg_query_result_t         *result
)
{
    if (req == NULL || result == NULL) {
        return VTSS_RC_ERROR;
    }

    switch (req->cmd_mode) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
#if !defined(VTSS_SW_OPTION_USERS)
        system_conf_t conf;
        memset(&conf, 0x0, sizeof(conf));
        if (system_get_config(&conf) == VTSS_RC_OK) {
            (void) vtss_icfg_printf(result, "password encrypted %s\n", conf.sys_passwd);

#ifndef VTSS_SW_OPTION_DAYLIGHT_SAVING
            /* On WebstaX branch, we don't support daylight saving on it. So the offset has to implement in order to print */
            if (conf.tz_off != 0) {
                (void) vtss_icfg_printf(result, "clock timezone offset %d\n", conf.tz_off);
            }
#endif
        }
#endif
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
        vtss_appl_sysutil_tm_config_t def_conf, now_conf;
        vtss_appl_sysutil_tm_sensor_type_t now_sensor, next_sensor;
        vtss_appl_sysutil_tm_sensor_type_t *now_sensor_p;

        now_sensor_p = NULL;
        while (!vtss_appl_sysutil_temperature_monitor_sensor_itr(now_sensor_p, &next_sensor)) {
            now_sensor = next_sensor;
            now_sensor_p = &now_sensor;
            system_get_temperature_config(now_sensor, &now_conf);
            system_init_temperature_config_default(now_sensor, &def_conf);
            if (req->all_defaults ||
                (now_conf.low != def_conf.low) || (now_conf.high != def_conf.high) ||
                (now_conf.critical != def_conf.critical) || (now_conf.hysteresis != def_conf.hysteresis)) {
                (void) vtss_icfg_printf(result, TM_ICFG_STR, get_sensor_type_name(now_sensor),
                    now_conf.low, now_conf.high, now_conf.critical, now_conf.hysteresis);
            }
        }
#endif
        break;

    default:
        /* no config in other modes */
        break;
    }

    return VTSS_RC_OK;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc sysutil_icfg_init(void)
{
    mesa_rc rc;

    /*
        Register Global config callback functions to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_GLOBAL_SYSUTIL, "sysutil", SYSUTIL_icfg);
    return rc;
}
