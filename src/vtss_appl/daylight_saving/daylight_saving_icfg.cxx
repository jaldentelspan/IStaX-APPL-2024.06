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
#include "daylight_saving_api.h"
#include "daylight_saving_icfg.h"
#include "misc_api.h"   //uport2iport(), iport2uport()
#include "icli_porting_util.h"

#define SECSPERMIN          60

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

/* ICFG callback functions */
static mesa_rc DAYLIGHT_SAVING_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                                vtss_icfg_query_result_t *result)
{
    mesa_rc                             rc = VTSS_RC_OK;
    time_conf_t                         conf, def_conf;
    u32                                 sub_type;

    time_dst_default_set(&def_conf);
    if ((rc = time_dst_get_config(&conf)) != VTSS_RC_OK) {
        return rc;
    }

    if (req->all_defaults ||
        conf.dst_mode != def_conf.dst_mode ||
        memcmp(&conf.dst_start_time, &def_conf.dst_start_time, sizeof(conf.dst_start_time)) ||
        memcmp(&conf.dst_end_time, &def_conf.dst_end_time, sizeof(conf.dst_end_time))) {
        /* entries */
        // example 1:   clock summer-time <word> recurring [<1-5> <1-7> <1-12> <time> <1-5> <1-7> <1-12> <time> [<1-1439>]]
        // example 2:   clock summer-time <word> date [<1-12> <1-31> <2000-2097> <time> <1-12> <1-31> <2000-2097> <time> [<1-1439>]]
        switch (conf.dst_mode) {
        case VTSS_APPL_CLOCK_SUMMER_TIME_DISABLED:
            break;
        case VTSS_APPL_CLOCK_SUMMER_TIME_RECURRING:
            rc = vtss_icfg_printf(result, "%s %s %s %d %d %d %02d:%02d %d %d %d %02d:%02d %d\n",
                                  VTSS_NTP_GLOBAL_MODE_CLOCK_TEXT,
                                  strlen(conf.tz_acronym) > 0 ? conf.tz_acronym : TIME_DST_NULL_ACRONYM,
                                  VTSS_NTP_GLOBAL_MODE_RECURRING_TEXT,
                                  conf.dst_start_time.week,
                                  conf.dst_start_time.day,
                                  conf.dst_start_time.month,
                                  conf.dst_start_time.hour,
                                  conf.dst_start_time.minute,
                                  conf.dst_end_time.week,
                                  conf.dst_end_time.day,
                                  conf.dst_end_time.month,
                                  conf.dst_end_time.hour,
                                  conf.dst_end_time.minute,
                                  conf.dst_offset);
            break;
        case VTSS_APPL_CLOCK_SUMMER_TIME_NON_RECURRING:
            rc = vtss_icfg_printf(result, "%s %s %s %d %d %d %02d:%02d %d %d %d %02d:%02d %d\n",
                                  VTSS_NTP_GLOBAL_MODE_CLOCK_TEXT,
                                  strlen(conf.tz_acronym) > 0 ? conf.tz_acronym : TIME_DST_NULL_ACRONYM,
                                  VTSS_NTP_GLOBAL_MODE_NO_RECURRING_TEXT,
                                  conf.dst_start_time.month,
                                  conf.dst_start_time.date,
                                  conf.dst_start_time.year,
                                  conf.dst_start_time.hour,
                                  conf.dst_start_time.minute,
                                  conf.dst_end_time.month,
                                  conf.dst_end_time.date,
                                  conf.dst_end_time.year,
                                  conf.dst_end_time.hour,
                                  conf.dst_end_time.minute,
                                  conf.dst_offset);
            break;
        }
        if (rc != VTSS_RC_OK) {
            return rc;
        }

    }

    if (req->all_defaults ||
        conf.dst_offset != def_conf.dst_offset ||
        strcmp(conf.tz_acronym, def_conf.tz_acronym) ||
        conf.tz_offset != def_conf.tz_offset ||
        conf.tz != def_conf.tz) {
        /* timezone */
        // example: clock timezone <word> <-23-23> [<0-59> [sub-type]]

        if (conf.tz == 0) {
            sub_type = 0;
        } else {
            // Should be always positive value
            sub_type = conf.tz >= (conf.tz_offset * 10) ? (conf.tz - (conf.tz_offset * 10)) : (conf.tz_offset * 10 - conf.tz);
        }

        // acronym
        rc = vtss_icfg_printf(result, "%s %s",
                              VTSS_NTP_GLOBAL_MODE_CLOCK_TIMEZONE_TEXT,
                              strlen(conf.tz_acronym) ? conf.tz_acronym : TIME_DST_NULL_ACRONYM);
        if (rc != VTSS_RC_OK) {
            return rc;
        }

        // hours
        rc = vtss_icfg_printf(result, " %d",
                              conf.tz_offset != 0 ? conf.tz_offset / SECSPERMIN : 0);
        if (rc != VTSS_RC_OK) {
            return rc;
        }

        // minutes
        if ((conf.tz_offset % SECSPERMIN) != 0 || sub_type != 0) {
            rc = vtss_icfg_printf(result, " %d",
                                  (conf.tz_offset % SECSPERMIN) >= 0 ? (conf.tz_offset % SECSPERMIN) : 0 - (conf.tz_offset % SECSPERMIN));
            if (rc != VTSS_RC_OK) {
                return rc;
            }
        }

        // sub_type
        if (sub_type != 0) {
            rc = vtss_icfg_printf(result, " %d", sub_type);
            if (rc != VTSS_RC_OK) {
                return rc;
            }
        }

        rc = vtss_icfg_printf(result, "\n");
    }

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc vtss_daylight_saving_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_DAYLIGHT_SAVING_GLOBAL_CONF, "clock", DAYLIGHT_SAVING_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return rc;
}
