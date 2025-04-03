/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief   Public MRP API
 * \details This header file describes MRP public functions and types, applicable to MRP.
 *          MRP stands for Multiple Registration Protocol, specified in
 *          IEEE 802.1Q-2014, clause 10.
 *          MRP is responsible for dynamic registration and de-registration of
 *          attributes in a Bridged Local Area Network. MRP is the framework
 *          that generically defines how this dynamic registration/de-registration
 *          of attributes is to be accomplished but it is not a protocol by itself.
 *          MRP Applications such as MVRP and MMRP are the actual protocols that
 *          target specific attributes each (e.g. MVRP is for VLAN identifiers and
 *          MMRP is for Multicast Group MAC addresses) and those are using the MRP
 *          framework. The framework has a limited number of objects that are
 *          user-configured and are application independent, i.e. they apply for all
 *          MRP applications that are present and enabled on a given bridge.
 *          These objects are basically a set of timers (IEEE 802.1Q-2014 10.7)
 *          that are defined on a bridge port basis, plus a control over the state
 *          of the PeriodicTransmission state machine (IEEE 802.1Q-2014 10.7).
 *          The user can configure the timeout values of these timers and the control
 *          (enable/disable) the PeriodicTransmission STM.
 */

#ifndef _VTSS_APPL_MRP_H_
#define _VTSS_APPL_MRP_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MRP global configuration Structure required in vtss_appl_mrp_config_interface_set()
 * and vtss_appl_mrp_config_interface_get() functions,
 * acting as the value of the configuration to be set or retrieved.
 **/
typedef struct {
    /**
     * Join timeout.
     *
     * The timeout value of the Join timer for this interface,
     * in centiseconds [cs].
     * Default value: 20cs, configuration range: 1 - 20cs.
     * IEEE 802.1Q-2014 section 10.7.11
     */
    uint32_t join_timeout;

    /**
     * Leave timeout.
     *
     * The timeout value of the Leave timer for this interface,
     * in centiseconds [cs].
     * Default value: 60cs, configuration range: 60 - 300cs.
     * IEEE 802.1Q-2014 section 10.7.11
     */
    uint32_t leave_timeout;

    /**
     * LeaveAll timeout.
     *
     * The timeout value of the LeaveAll timer for this interface,
     * in centiseconds [cs].
     * Default value: 1000cs, configuration range: 1000 - 5000cs.
     * IEEE 802.1Q-2014 section 10.7.11
     */
    uint32_t leave_all_timeout;

    /**
     * PeriodicTransmission state
     *
     * The state of the PeriodicTransmission STM,
     * i.e. enabled (TRUE) or disabled (FALSE),
     * for this interface.
     * IEEE 802.1Q-2014 section 10.7.
     */
    mesa_bool_t periodic_transmission;
} vtss_appl_mrp_config_interface_t;

/* Interface functions ----------------------------------------------------- */

/**
 * \brief Set an interface's MRP configuration.
 *
 * This function can be used to set the timeout values of the MRP timers and
 * the state of the PeriodicTransmission STM for a specific interface.
 * The provided interface ID must of course be a valid interface of the switch/stack.
 *
 * Configuration range:
 * Join timeout       : 1 - 20cs
 * Leave timeout      : 60 - 300cs
 * LeaveAll timeout   : 1000 - 5000cs
 *
 * \param ifindex [IN] The ID of the interface that is being configured.
 * \param config  [IN] MRP timers and PeriodicTransmission state to be set
 *                     on the above interface.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error
 * codes corresponding to invalid input parameters (e.g. non-existing interface, timeout
 * value outside the configurable range).
 **/
mesa_rc vtss_appl_mrp_config_interface_set(vtss_ifindex_t ifindex,
        const vtss_appl_mrp_config_interface_t *const config);

/**
 * \brief Get an interface's MRP configuration.
 *
 * This function can be used to retrieve the timeout values of the MRP timers and
 * the state of the PeriodicTransmission STM of a specific interface.
 *
 * Furthermore, the user can either use the iterator function in order to select
 * a valid interface from the switch/stack or directly call this function with
 * a proper interface ID and retrieve its configuration.
 *
 * When searching for an interface with the iterator (the interface ID is thus provided),
 * then this function will always be successful and return VTSS_RC_OK.
 *
 * \param ifindex [IN]  The ID of the interface that is requested.
 *
 * \param config  [OUT] MRP timers and PeriodicTransmission state of
 *                      the above interface.
 *
 * \return VTSS_RC_OK if the operation succeeded, or some error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_mrp_config_interface_get(vtss_ifindex_t ifindex,
        vtss_appl_mrp_config_interface_t *config);

/**
 * \brief Iterator function of the MRP interface configuration.
 * This function will iterate through all interfaces available in the switch/stack
 * so they can be used to fetch their configuration.
 *
 * Use the NULL pointer as an input in order to get the index of the first available
 * interface, otherwise the iterator will provide the index of the next interface.
 *
 * \param prev_ifindex [IN]  ID of the previous switch interface.
 *
 * \param next_ifindex [OUT] ID of the next switch inteface.
 *
 * \return VTSS_RC_OK if the next interface ID was found, otherwise VTSS_RC_ERROR
 * to signal the end of the interface list.
 **/
mesa_rc vtss_appl_mrp_config_interface_itr(const vtss_ifindex_t *const prev_ifindex,
        vtss_ifindex_t *const next_ifindex);
#ifdef __cplusplus
}
#endif
#endif  // _VTSS_APPL_MRP_H_
