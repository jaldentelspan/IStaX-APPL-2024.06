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
 * \brief   Public MVRP API
 * \details This header file describes MVRP public functions and types, applicable to MVRP.
 *          MVRP stands for Multiple VLAN Registration Protocol, specified in
 *          IEEE 802.1Q-2014, clause 11, and is utilizing the MRP framework,
 *          specified in IEEE 802.1Q-2014, clause 10.
 *          MVRP is responsible for propagating VLAN registrations along
 *          a network of MVRP enabled switches, through the exchange of MVRPDUs
 *          that are carrying registration information of all managed VLANs.
 *          This exchange takes place only on MVRP enabled ports of a given switch.
 *          The user can therefore configure a set of ports as MVRP enabled and also
 *          configure the global status of the protocol, i.e. enabled or disabled.
 *          Finally, a limit can be set on the number of VLANS the protocol will manage,
 *          in the form of a VLAN list.
 */

#ifndef _VTSS_APPL_MVRP_H_
#define _VTSS_APPL_MVRP_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MVRP global configuration Structure required in vtss_appl_mvrp_config_global_set()
 * and vtss_appl_mvrp_config_global_get() functions,
 * acting as the value of the configuration to be set or retrieved.
 **/
typedef struct {
    /**
     * Global MVRP state.
     *
     * The global state of the MVRP, i.e. enabled (TRUE) or disabled (FALSE).
     */
    mesa_bool_t state;

    /**
     * List of managed VLANs.
     *
     * The vlan list of the entire 4K range, in a bitmask format,
     * indicating specifically which VLANs the protocol will manage at a given time.
     */
    vtss_vlan_list_t vlans;
} vtss_appl_mvrp_config_global_t;

/**
 * MVRP interface configuration Structure required in vtss_appl_mvrp_config_interface_set()
 * and vtss_appl_mvrp_config_interface_get() functions,
 * acting as the value of the configuration to be set or retrieved.
 **/
typedef struct {
    /**
     * Interface MVRP state.
     *
     * The state of the MVRP for a given interface,
     * i.e. enabled (TRUE) or disabled (FALSE).
     */
    mesa_bool_t state;
} vtss_appl_mvrp_config_interface_t;

/**
 * MVRP interface statistics structure required in vtss_appl_mvrp_stat_interface_get()
 * function, acting as the value of the statistics to be retrieved.
 **/
typedef struct {
    /**
     * Number of failed registrations.
     *
     * Each interface running MVRP maintains a count of the number of times
     * that it has received a registration request, but has failed to register
     * the VLAN due to lack of space in the filtering database.
     **/
    uint64_t failed_registrations;
    /**
     * Source MAC address of the last received MVRPDU.
     *
     * Each interface running MVRP records the MAC address of the
     * originator of the most recently received MVRPDU.
     **/
    mesa_mac_t last_pdu_origin;
} vtss_appl_mvrp_stat_interface_t;

/* Global functions -------------------------------------------------------- */

/**
 * \brief Set MVRP global configuration.
 *
 * This function can be used to set the global state of MVRP (enabled or disabled)
 * and the maximum number of managed VLANs.
 *
 * Managed VLANs : any list with values in the range 1 - 4094
 *
 * Note that even though any update to the list of managed VLANs is possible, if MVRP is
 * already enabled, then the time to update all state machines is directly proportional
 * to the number of VLANs that are added/removed to/from the list. Therefore this operation
 * is recommended only for small updated to the VLAN list. For large updates (e.g 1k VLANs),
 * consider disabling the protocol first and then perform the update.
 *
 * \param config [IN] MVRP state and list of managed VLANs.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error
           codes corresponding to invalid input parameters.
 **/
mesa_rc vtss_appl_mvrp_config_global_set(const vtss_appl_mvrp_config_global_t *const config);

/**
 * \brief Get MVRP global configuration.
 *
 * This function can be used to retrieve the global state of MVRP (enabled or disabled)
 * and the list of managed VLANs.
 *
 * \param config [OUT] MVRP state and list of managed VLANs.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mvrp_config_global_get(vtss_appl_mvrp_config_global_t *config);

/* Interface functions ----------------------------------------------------- */

/**
 * \brief Set an interface's MVRP configuration.
 *
 * Use it to set an interface as MVRP-enabled or MVRP-disabled.
 * The provided interface ID must of course be a valid interface of the switch/stack.
 *
 * \param ifindex [IN] The ID of the interface that is being configured.
 * \param config  [IN] MVRP state to be set on the above interface.
 *
 * \return VTSS_RC_OK if the operation succeeded, otherwise it might return various error
 * codes corresponding to invalid input parameters (e.g. non-existing interface).
 **/
mesa_rc vtss_appl_mvrp_config_interface_set(vtss_ifindex_t ifindex,
        const vtss_appl_mvrp_config_interface_t *const config);

/**
 * \brief Get an interface configuration, i.e. the MVRP mode the interface is set to.
 *
 * Furthermore, the user can either use the iterator function in order to select a valid
 * interface from the switch/stack or directly call this function with a proper
 * interface ID and retrieve its configuration.
 *
 * When searching for an interface with the iterator (the interface ID is thus provided),
 * then this function will always be successful and return VTSS_RC_OK.
 *
 * \param ifindex [IN]  The ID of the interface that is requested.
 *
 * \param config  [OUT] MVRP state of the above interface.
 *
 * \return VTSS_RC_OK if the operation succeeded, or some error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_mvrp_config_interface_get(vtss_ifindex_t ifindex,
        vtss_appl_mvrp_config_interface_t *config);

/**
 * \brief Get an interface's MVRP statistics.
 *
 * This function can be used to retrieve the MVRP statistics for a given interface,
 * i.e. the number of failed registrations since the protocol was enabled and the
 * source MAC address of the last received MVRPDU.
 * Note that if the interface is not MVRP enabled, this function will return 0 and
 * 00-00-00-00-00-00. Same if it has not received any MVRPDUs yet.
 *
 * \param ifindex [IN]  The ID of the interface that is requested.
 *
 * \param stat    [OUT] MVRP statistics for the above interface.
 *
 * \return VTSS_RC_OK if the operation succeeded, or some error code depending on the invalid input parameter.
 **/
mesa_rc vtss_appl_mvrp_stat_interface_get(vtss_ifindex_t ifindex,
        vtss_appl_mvrp_stat_interface_t *stat);

/**
 * \brief Iterator function of the MVRP interface configuration.
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
mesa_rc vtss_appl_mvrp_config_interface_itr(const vtss_ifindex_t *const prev_ifindex,
        vtss_ifindex_t *const next_ifindex);
#ifdef __cplusplus
}
#endif
#endif  // _VTSS_APPL_MVRP_H_
