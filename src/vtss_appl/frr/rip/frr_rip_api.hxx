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
 * \file frr_rip_api.h
 * \brief This file contains the definitions of RIP internal API functions,
 * including the application public header APIs(naming start with
 * vtss_appl_rip) and the internal module APIs(naming start with frr_rip)
 */

#ifndef _FRR_RIP_API_HXX_
#define _FRR_RIP_API_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr.hxx"
#include <vtss/appl/rip.h>

/******************************************************************************/
/** Module initialization                                                     */
/******************************************************************************/
mesa_rc frr_rip_init(vtss_init_data_t *data);
const char *frr_rip_error_txt(mesa_rc rc);

/******************************************************************************/
/** Module internal APIs                                                      */
/******************************************************************************/

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
/**
 * \brief Get the classful prefix for the given address.
 * It does NOT follow private address space defined in RFC 1918.
 *    Private Address Space -
 *       10.0.0.0        -   10.255.255.255  (10/8 prefix)
 *       172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
 *       192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
 * This function returns the prefix 8/16/24 for class A/B/C IP addresses
 * separately and returns 0 for class D/E.
 * \param ip [IN] IP address.
 * \return the prefix length. 0 means no classful prefix.
 */
uint32_t FRR_ICLI_ipv4_addr_to_prefix(mesa_ipv4_t ip);

/**
 * \brief Get the RIP network default configuration.
 * \param network [OUT] RIP network.
 * \return Error code.
 */
mesa_rc frr_rip_network_conf_def(mesa_ipv4_network_t *const network);

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
/**
 * \brief Set the RIP neighbor connection configuration.
 * It is a dummy function for JSON/SNMP serialzer only.
 * \param neighbor_addr [IN] The RIP neighbor address.
 * \return Always return FRR_RC_NOT_SUPPORTED.
 */
mesa_rc frr_rip_neighbor_dummy_set(const mesa_ipv4_t neighbor_addr);

//----------------------------------------------------------------------------
//** RIP interface configuration
//----------------------------------------------------------------------------
/**
 * \brief Get the RIP VLAN interface default configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] The RIP VLAN interface configuration.
 * \return Error code.
 */
mesa_rc frr_rip_intf_conf_def(vtss_ifindex_t *const ifindex,
                              vtss_appl_rip_intf_conf_t *const conf);

//----------------------------------------------------------------------------
//** RIP control global options
//----------------------------------------------------------------------------
/**
 * \brief Get RIP control of global options.
 * It is a dummy function for SNMP serialzer only.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc frr_rip_control_globals_dummy_get(
    vtss_appl_rip_control_globals_t *const control);

#endif /* _FRR_RIP_API_HXX_ */

