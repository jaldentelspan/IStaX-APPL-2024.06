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

/**
 * \file
 * \brief Public Types
 * \details This header file describes public fan
 */

#ifndef _VTSS_APPL_FAN_H_
#define _VTSS_APPL_FAN_H_

#include <vtss/appl/types.h>
#include <vtss/appl/module_id.h>            // For MODULE_ERROR_START()
#include <vtss/basics/enum-descriptor.h>    // For vtss_enum_descriptor_t

#ifdef __cplusplus
extern "C" {
#endif

//************************************************
// Configuration definition
//************************************************

/**  MAX number of temperature sensors */
#define VTSS_APPL_FAN_TEMPERATURE_SENSOR_CNT_MAX 5

/*!
 *  \brief
 *      fan conf params.
 */
typedef struct {
    int16_t   t_max; /**< The temperature where fan is running at full speed */
    int16_t   t_on;  /**< The temperature where cooling is needed (fan is started) */
    mesa_fan_pwd_freq_t pwm; /**< The pwm frequency used for controlling the fan */
} vtss_appl_fan_conf_struct_t;

/*!
 *  \brief
 *      fan conf.
 */
typedef struct {
    vtss_appl_fan_conf_struct_t glbl_conf; /**< fan conf */
} vtss_appl_fan_conf_t;

/*!
 *  \brief
 *      fan current status
 *
 */
typedef struct {
    int16_t chip_temp[VTSS_APPL_FAN_TEMPERATURE_SENSOR_CNT_MAX]; /**< Chip temperature ( in C.) */
    uint16_t     fan_speed;                                 /**< The speed the fan is currently running (in RPM) */
    uint8_t      fan_speed_setting_pct;                     /**< The fan speed level at which it is set to (in %) */
} vtss_appl_fan_status_t;

/*!
 *  \brief
 *      fan current speed
 *
 */
typedef struct {
    uint16_t fan_speed;                                 /**< The speed the fan is currently running (in RPM) */
} vtss_appl_fan_speed_t;

/*!
 *  \brief
 *      fan chip temp
 *
 */
typedef struct {
    int16_t chip_temp;                              /**< Chip temperature ( in C.) */
} vtss_appl_fan_chip_temp_t;

/*!
 *   \brief Fan platform specific definitions.
 */
typedef struct {
    uint8_t sensor_count;            /**< Maximum number of temperature sensors availabe in a switch. */
} vtss_appl_fan_capabilities_t;

/**
 * \brief Get capabilities
 *
 * \param capabilities [OUT] The capabilities of fan.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_fan_capabilities_get(vtss_appl_fan_capabilities_t
                                                    *const capabilities);

/*!
 * Get Fan Global Parameters
 *
 * \param conf [OUT] The global configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_fan_conf_get(
    vtss_appl_fan_conf_t *conf
);

/*!
 * Get Fan Global Parameters
 *
 * \param conf [OUT] The global default configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_fan_default_conf_get(
    vtss_appl_fan_conf_t *conf
);

/*!
 * Set Fan Global Parameters
 *
 * \param conf [IN] The global configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_fan_conf_set(
    const vtss_appl_fan_conf_t *conf
);

/*!
 * \brief Get Chip Temperature
 *
 * \param usid          [IN]: Switch ID for user view (The valid value starts from 1)
 * \param sensorId      [IN]: Sensor ID
 * \param chipTemp      [OUT]: The chip temperature
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_fan_sensor_temp_get(
    vtss_usid_t                 usid,
    uint8_t                          sensorId,
    vtss_appl_fan_chip_temp_t   *const chipTemp
);

/*!
 * \brief Fan speed get function, it is used to get fan speed.
 *
 * \param usid    [IN]:  switch ID.
 * \param speed   [OUT]: fan speed.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_fan_speed_get(
    vtss_usid_t             usid,
    vtss_appl_fan_speed_t  *const speed
);

/*!
* \brief Fan sensors iterate function, it is used to get first and get next indexes.
* \param prev_swid_idx   [IN]:  previous switch ID.
* \param next_swid_idx   [OUT]: next switch ID.
* \param prev_sensor_idx [IN]:  previous sensor ID.
* \param next_sensor_idx [OUT]: next sensor ID.
* \return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_fan_sensors_itr(
    const vtss_usid_t *const prev_swid_idx,
    vtss_usid_t       *const next_swid_idx,
    const uint8_t          *const prev_sensor_idx,
    uint8_t                *const next_sensor_idx);


#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // _VTSS_APPL_FAN_H_
