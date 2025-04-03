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
 * \brief Public Switch LED (light emitting diode) Power Reduction APIs.
 * \details This header file describes public switch LEDs power reduction APIs.
 *  The LEDs power consumption can be reduced by lowering the LEDs intensity.
 */

#ifndef _VTSS_APPL_LED_POWER_REDUCTION_H_
#define _VTSS_APPL_LED_POWER_REDUCTION_H_

#include <vtss/appl/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief LED power intensity level.
 */
typedef struct {
    /** The LEDs brightness level in percentage
      (100 means full power consumption, 0 means no power consumption). 
     */ 
    uint8_t intensity;                               
} vtss_appl_led_power_reduction_intensity_t;

/*!
 * \brief LEDs clock time when LEDs start glowing with a associated intensity.
 */
typedef struct {
 /** Hour is based on 24-hour time notation. 
   If hour is set to x then led will start glowing with a given intensity level at clock time x:00 of the day. 
  */
    uint8_t hour;                               
} vtss_appl_led_power_reduction_led_glow_start_time_t;

/*!
 *  \brief LEDs config params to glow LEDs in full brightness(full power consumption).
 */
typedef struct {
    /** The switch maintenance duration, valid range is from 0 to 65535 seconds. 
      During switch maintenance LEDs will glow in full intensity after either a port has changed link state, 
      or the LED push button has been pushed. 
     */
    uint16_t  maintenance_duration;
   /** If its TRUE then LEDs will be turned on at full brightness(100% intensity) when LED is blinking in red,
     because of either software error or fatal occurred. 
    */ 
    mesa_bool_t error_enable;   
} vtss_appl_led_power_reduction_led_full_brightness_conf_t;

/**
 * \brief Associate a LED intensity level with a clock time of the day.
 *
 * \details It is used to set LEDs intensity level for a given clock time of the day.
 *
 * \param clockTime [IN]: Clock time based on 24-hour time notation
 *
 * \param intensity  [IN]: LEDs Intensity (in percentage)
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_led_power_reduction_clock_time_intensity_set(
         vtss_appl_led_power_reduction_led_glow_start_time_t clockTime, 
         const vtss_appl_led_power_reduction_intensity_t  *const intensity);

/**
 * \brief Get associated LEDs intensity level for a given clock time of the day.
 *
 * \param clockTime [IN]: Clock time based on 24-hour time notation
 *
 * \param intensity [OUT]: LEDs Intensity level(in percentage)
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_led_power_reduction_clock_time_intensity_get(
         vtss_appl_led_power_reduction_led_glow_start_time_t clockTime,
         vtss_appl_led_power_reduction_intensity_t  *const intensity);

/**
 * \brief Set parameters to glow LEDs in full brightness(100% intensity). 
 *
 * \param conf    [IN]: Configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_led_power_reduction_full_led_brightness_conf_set(
         const vtss_appl_led_power_reduction_led_full_brightness_conf_t *const conf);

/**
 * \brief Get parameters to glow LEDs in full brightness(100% intensity).
 *
 * \param conf [OUT]: The configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_led_power_reduction_full_led_brightness_conf_get(
          vtss_appl_led_power_reduction_led_full_brightness_conf_t *const conf);

/**
 * \brief Intensity clock time iterator function, it is used to get first or next clock time.
 *
 * \param prev_clock_time [IN]:Previous clock time
 *
 * \param next_clock_time [OUT]:Next clock time
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_led_power_reduction_clock_time_iterator(
        const vtss_appl_led_power_reduction_led_glow_start_time_t *const prev_clock_time,
        vtss_appl_led_power_reduction_led_glow_start_time_t *const next_clock_time);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // _VTSS_APPL_LED_POWER_REDUCTION_H_
