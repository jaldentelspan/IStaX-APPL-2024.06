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
 * \brief Public Port Power Saving APIs.
 * \details This header file describes public port power saving APIs.
 *          Port power saving reduces the switch power consumption
 *          by lowering the port power supply when there is no link partner connected to a port
 *          as well as when link partner is connected through a short cable.
 */

#ifndef _VTSS_APPL_PORT_POWER_SAVINGS_H_
#define _VTSS_APPL_PORT_POWER_SAVINGS_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>    // For vtss_enum_descriptor_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief Port power saving configurable parameters.
 */
typedef struct {
    /** Save port power if there is no link partner connected to the port. */
    mesa_bool_t energy_detect;
    /** Save port power if port is connected to link partner through short cable. */
    mesa_bool_t short_reach;
} vtss_appl_port_power_saving_conf_t;

/**
 * \brief Port power saving status type.
 */
typedef enum {
    /** No port power saving. */
    VTSS_APPL_PORT_POWER_SAVING_STATUS_NO,
    /** Saving port power. */
    VTSS_APPL_PORT_POWER_SAVING_STATUS_YES,
    /** Power saving is not supported. */
    VTSS_APPL_PORT_POWER_SAVING_NOT_SUPPORTED
} vtss_appl_power_saving_status_t;

/**
 *  \brief Port power saving current status.
 */
typedef struct {
    /** Whether port is saving power due to no link partner connected. */
    vtss_appl_power_saving_status_t energy_detect_power_savings;
    /** Whether port is saving power due to link partner connected through short cable. */
    vtss_appl_power_saving_status_t   short_reach_power_savings;
} vtss_appl_port_power_saving_status_t;

/**
 *   \brief Platform specific port power saving capabilities.
 */
typedef struct {
    /** Whether port is capable for detecting link partner or not. */
    mesa_bool_t energy_detect_capable;
    /** Whether port is able to determine the cable length connected to partner port. */
    mesa_bool_t short_reach_capable;
} vtss_appl_port_power_saving_capabilities_t;

/**
 * \brief Get Platform specific port power saving capabilities
 *
 * \param ifIndex       [IN]: Interface index
 * \param capabilities [OUT]: Platform specific port power saving capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_port_power_saving_capabilities_get(
    vtss_ifindex_t                             ifIndex,
    vtss_appl_port_power_saving_capabilities_t *const capabilities);


/**
 * \brief Set Port power saving configuration.
 *
 * \param ifIndex  [IN]: Interface index
 * \param conf     [IN]: Port power saving configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_port_power_saving_conf_set(
    vtss_ifindex_t                            ifIndex,
    const vtss_appl_port_power_saving_conf_t  *const conf);

/**
 * \brief Get Port power saving configuration.
 *
 * \param ifIndex  [IN] : Interface index
 * \param conf     [OUT]: Port power saving configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_port_power_saving_conf_get(
    vtss_ifindex_t                     ifIndex,
    vtss_appl_port_power_saving_conf_t *const conf);


/**
 * \brief Get Port power saving current status
 *
 * \param ifIndex     [IN]: Interface index
 * \param status     [OUT]: Port power saving status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_port_power_saving_status_get(
    vtss_ifindex_t                       ifIndex,
    vtss_appl_port_power_saving_status_t *const status);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // _VTSS_APPL_PORT_POWER_SAVINGS_H_
