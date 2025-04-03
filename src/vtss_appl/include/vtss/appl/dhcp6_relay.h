/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief Public DHCPv6 Relay API
 * \details This header file describes DHCPv6 relay interface
 */

#include <vtss/basics/api_types.h>
#include <vtss/appl/interface.h>

#ifndef _VTSS_APPL_DHCPV6_RELAY_H_
#define _VTSS_APPL_DHCPV6_RELAY_H_

/** \brief DHCPv6 relay configuration/status per (vlan interface, relay interface). */
typedef struct {
    /** The address of the DHCPv6 server. The default address used if nothing was
        specified is the multicast address ALL_DHCP_SERVERS (FF05::1:3)*/
    mesa_ipv6_t relay_destination;
} vtss_appl_dhcpv6_relay_vlan_t;


#ifdef __cplusplus
extern "C" {
#endif

/* Iterators -------------------------------------------------------------- */

/**
 * \brief Iterate through all vlan for which a dhcpv6_relay is defined
 * \param in [IN]   Pointer to current vlan interface id. Provide a null 
 *                  pointer to get the first interface id.
 * \param out [OUT] Next interface id (relative to the value provided in
 *                  'in').
 * \param relay_in  [IN]  Pointer to current relay vlan interface id. Provide a 
 *                  null pointer to get the first dhcpv6 relay interface vlan id.
 * \param relay_out [OUT] Next relay vlan interface id (relative to the value provided in
 *                  'relay_in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" vlan id exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_dhcpv6_relay_vlan_conf_itr(const vtss_ifindex_t *const in,
                                             vtss_ifindex_t *const out,
                                             const vtss_ifindex_t *const relay_in,
                                             vtss_ifindex_t *const relay_out);

/* Dhcpv6_Relay functions ----------------------------------------------------- */

/**
 * \brief Get dhcpv6 relay configuration for a specific vlan
 * \param interface [IN] The vlan interface id
 * \param relay_in [IN] The relay vlan interface id.
 * \param conf [OUT] The dhcp relay configuration for the vlan id
 * \return Error code.
 */
mesa_rc vtss_appl_dhcpv6_relay_conf_get(vtss_ifindex_t interface,
                                        vtss_ifindex_t relay_in,
                                        vtss_appl_dhcpv6_relay_vlan_t *conf);

/**
 * \brief Set dhcp relay configuration for a specific vlan
 * \param interface [IN] The vlan interface id
 * \param relay_in [IN] The relay vlan interface id.
 * \param conf [IN] The dhcp relay configuration for the vlan interface
 * \return Error code.
 */
mesa_rc vtss_appl_dhcpv6_relay_conf_set(vtss_ifindex_t interface,
                                        vtss_ifindex_t relay_in,
                                        const vtss_appl_dhcpv6_relay_vlan_t *const conf);

/**
 * \brief Set dhcp relay configuration for a specific vlan
 * \param interface [IN] The vlan interface id
 * \param relay_in [IN] The relay vlan interface id.
 * \return Error code.
 */
mesa_rc vtss_appl_dhcpv6_relay_conf_del(vtss_ifindex_t interface,
                                        vtss_ifindex_t relay_in);

/**
 * \brief Iterate through all vlan for which a dhcpv6_relay is active
 * \param in [IN]   Pointer to current vlan interface id. Provide a null 
 *                  pointer to get the first interface id.
 * \param out [OUT] Next interface id (relative to the value provided in
 *                  'in').
 * \param relay_in  [IN]  Pointer to current relay vlan interface id. Provide a 
 *                  null pointer to get the first dhcpv6 relay interface vlan id.
 * \param relay_out [OUT] Next relay vlan interface id (relative to the value provided in
 *                  'relay_in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" vlan id exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_dhcpv6_relay_vlan_status_itr(const vtss_ifindex_t *const in,
                                               vtss_ifindex_t *const out,
                                               const vtss_ifindex_t *const relay_in,
                                               vtss_ifindex_t *const relay_out);

/**
 * \brief Get dhcpv6 relay status for a specific (vlan interface, relay interface)
 * \param interface [IN] The vlan interface id
 * \param relay_in [IN] The relay vlan interface id.
 * \param status [OUT] The dhcp relay status.
 * \return Error code.
 */
mesa_rc vtss_appl_dhcpv6_relay_status_get(vtss_ifindex_t interface,
                                          vtss_ifindex_t relay_in,
                                          vtss_appl_dhcpv6_relay_vlan_t *status);


#ifdef __cplusplus
}
#endif

#endif  // _VTSS_APPL_DHCPV6_RELAY_H_
