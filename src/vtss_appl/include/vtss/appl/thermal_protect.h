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
 * \brief Public Thermal Protection APIs.
 * \details This header file describes public thermal protection APIs.
 *          The PHY thermal protections consists of four groups. Each PHY is associated to a group,
 *          and each group has a configured max temperature. If the average temperature of all sensors
 *          exceeds the configured max temperature of a group, then the PHYs in that group is shoutdown.
 */

#ifndef _VTSS_APPL_THERMAL_PROTECT_H_
#define _VTSS_APPL_THERMAL_PROTECT_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/module_id.h>            // For MODULE_ERROR_START()
#include <vtss/basics/enum-descriptor.h>    // For vtss_enum_descriptor_t

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    VTSS_APPL_THERMAL_PROTECT_ERROR_ISID = MODULE_ERROR_START(VTSS_MODULE_ID_THERMAL_PROTECT), /*!< Invalid Switch ID.                            */
    VTSS_APPL_THERMAL_PROTECT_ERROR_FLASH,                                                     /*!< Could not store configuration in flash.       */
    VTSS_APPL_THERMAL_PROTECT_ERROR_SECONDARY_SWITCH,                                          /*!< Could not get data from secondary switch.     */
    VTSS_APPL_THERMAL_PROTECT_ERROR_NOT_PRIMARY_SWITCH,                                        /*!< Switch must to be primary switch.             */
    VTSS_APPL_THERMAL_PROTECT_ERROR_VALUE,                                                     /*!< Invalid value.                                */
    VTSS_APPL_THERMAL_PROTECT_ERROR_IFINDEX                                                    /**< Interface index (ifindex) is not a port index */
};

/*!
 * \brief Thermal protection group.
 */
typedef struct {
    uint8_t group;                                 /**< The group number. */
} vtss_appl_thermal_protect_group_t;

/*!
 *  \brief Thermal protection group temperature, each group has a temperature value.
 *
 */
typedef struct {
    int16_t group_temperature;                     /**< The group temperature ( in C.). */
} vtss_appl_thermal_protect_group_temperature_t;

/*!
 *   \brief Thermal protection port status.
 */
typedef struct {
    int16_t  temperature;                     /**< The current port temperature. */
    mesa_bool_t power_status;                    /**< The port power status based on thermal protection, True if the port is powered down. */
} vtss_appl_thermal_protect_port_status_t;

/*!
 *   \brief Thermal protection platform specific definitions.
 */
typedef struct {
    uint8_t max_supported_group;                /**< Maximum supported group. */
} vtss_appl_thermal_protect_capabilities_t;

/**
 * \brief Get capabilities
 *
 * \param capabilities [OUT] The capabilities of thermal protection.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_thermal_protect_capabilities_get(vtss_appl_thermal_protect_capabilities_t
                                                    *const capabilities);

/**
 * \brief Associate a group with a temperature, it is used to set temperature value for a given group
 *
 * \param group  [IN]: Group number
 *
 * \param temp   [IN]: Temperature
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_thermal_protect_group_temp_set(vtss_appl_thermal_protect_group_t group,
                                                 const vtss_appl_thermal_protect_group_temperature_t  *temp);

/**
 * \brief Get associated temperature for a given group
 *
 * \param group  [IN]: Group number
 *
 * \param temp   [OUT]: Temperature
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_thermal_protect_group_temp_get(vtss_appl_thermal_protect_group_t group,
                                                 vtss_appl_thermal_protect_group_temperature_t  *temp);

/**
 * \brief Associate a port with a group 
 *
 * \param ifIndex    [IN]: Interface index
 *
 * \param group      [IN]: The port group value
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_thermal_protect_port_group_set(vtss_ifindex_t ifIndex,
                                                  const vtss_appl_thermal_protect_group_t *group);

/**
 * \brief Get the group associated to a given port, 
 *
 * \param ifIndex    [IN]: Interface index
 *
 * \param group      [OUT]: The port group value
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_thermal_protect_port_group_get(vtss_ifindex_t ifIndex,
                                                  vtss_appl_thermal_protect_group_t *group);

/**
 * \brief Group iterate function, it is used to get first and get next group indexes.
 *
 * \param prev_group [IN]  previous group index.
 * \param next_group [OUT] next group index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_thermal_protect_group_iterator(const vtss_appl_thermal_protect_group_t *const prev_group,
                                                 vtss_appl_thermal_protect_group_t *const next_group);

/**
 * \brief Get the port current status 
 *
 * \param ifIndex    [IN]: Interface index
 *
 * \param status     [OUT]: The port status data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_thermal_protect_port_status_get(vtss_ifindex_t ifIndex,
                                                   vtss_appl_thermal_protect_port_status_t *status);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // _VTSS_APPL_THERMAL_PROTECT_H_
