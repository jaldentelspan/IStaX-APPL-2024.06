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

/**
 * \file
 * \brief Public Clock APIs.
 * \details This header file describes Summer time(Daylight Saving) and Time Zone APIs.
 *  Daylight saving time (DST) is a system of setting clocks ahead so that 
 *  both sunrise and sunset occur at a later hour. The effect is additional daylight in the evening.
 *  Many countries observe DST, although most have their own rules 
 *  and regulations for when it begins and ends. The dates of DST may change from year to year.
 */

#ifndef __VTSS_APPL_DAYLIGHT_SAVING_H__
#define __VTSS_APPL_DAYLIGHT_SAVING_H__

#include <vtss/appl/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The maximum length length of time zone acronym */
#define VTSS_APPL_CLOCK_TIMEZONE_NAME_LENGTH    16 

/*!
 * \brief 
 *      Clock summer time mode.  
 */
typedef enum {
    VTSS_APPL_CLOCK_SUMMER_TIME_DISABLED,     /*!< Summer time feature is disabled. */
    VTSS_APPL_CLOCK_SUMMER_TIME_RECURRING,    /*!< Summer time feature is enabled in recurring mode. */
    VTSS_APPL_CLOCK_SUMMER_TIME_NON_RECURRING /*!< Summer time feature is enabled in non recurring mode. */
} vtss_appl_clock_summertime_mode_t;

/*!
 * \brief 
 *      Clock summer time parameters. 
 *      This structure is used both for absolute(non-recurring) and recurring summer time mode.
 *       
 */
typedef struct {
    /** Week of month (1st week, 2nd week..week of month). 
        Used only in recurring summer time mode. 
        Valid range is 1-5, in absolute(non-recurring) mode it will be 0. 
     */
    uint8_t   week;  
    /** Day of week (Monday=1, Tuesday=2..Sunday=7). 
        Used only in recurring summer time mode.
        Valid range is 1-7, in absolute(non-recurring) mode it will be 0. 
     */ 
    uint8_t   day;    
    /** Date of date (1 to 31). 
        Used in absolute(non-recurring) summer time mode. 
        Valid range is 1-31, in recurring mode it will be 0.
     */
    uint8_t   date;
    /** Month (1 to 12). Used only in non-recurring and recurring summer time mode. 
        Valid range is 1-12.
     */
    uint8_t   month; 
    /** Year. Used only in absolute(non-recurring) summer time mode.
        Valid range is 2014-2097, in recurring mode it will be 0.
     */
    uint16_t  year; 
    /** Hour. Used in non-recurring and recurring summer time mode. 
        Valid range is 0-23.
     */
    uint8_t   hour;
    /** Minute.  Used in non-recurring and recurring summer time mode. 
        Valid range is 0-59.
     */
    uint8_t   minute;
} vtss_appl_clock_summertime_struct_t;

/*!
 * \brief 
 *      Clock summertime(daylight saving) configurations.
 */
typedef struct {
    vtss_appl_clock_summertime_mode_t    stime_mode;  /*!< Summertime feature mode. */
    vtss_appl_clock_summertime_struct_t  stime_start; /*!< Summertime start time(in UTC). */
    vtss_appl_clock_summertime_struct_t  stime_end;   /*!< Summertime end time(in UTC). */
    int16_t                                  stime_offset;/*!< Summer timezone offset from UTC in minutes. */
} vtss_appl_clock_summertime_conf_t;

/*!
 * \brief 
 *      Clock timezone parameters.
 */
typedef struct {
    /** This is a acronym to identify the time zone. */
    char    timezone_acronym[VTSS_APPL_CLOCK_TIMEZONE_NAME_LENGTH + 1];
    /** This is a standard timezone offset from UTC in minutes. */
    int16_t     timezone_offset;
} vtss_appl_clock_timezone_conf_t;

/*!
 * \brief 
 *      Clock configurations.
 */
typedef struct {
    vtss_appl_clock_summertime_conf_t summertime_conf; /*!< The summer time configuration. */
    vtss_appl_clock_timezone_conf_t   timezone_conf;   /*!< The timezone configuration. */
} vtss_appl_clock_conf_t;

/*!
 * \brief Set Clock global configuration parameters. 
 *
 * This is used to set summer time as well as time zone parameters.
 * 
 * \param conf  [IN]: The global clock configuration data.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_clock_conf_set(const vtss_appl_clock_conf_t *conf);

/*!
 * \brief Get Clock global configuration parameters.
 *
 * This is used to get summer time as well as time zone parameters.
 *
 * \param conf   [OUT]: The global clock configuration data.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_clock_conf_get(vtss_appl_clock_conf_t *conf);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // __VTSS_APPL_DAYLIGHT_SAVING_H__
