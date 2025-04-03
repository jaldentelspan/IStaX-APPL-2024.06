/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
* \brief Public MVR (Multicast VLAN Registration) API
* \details This header file describes MVR control functions and types.
* MVR utilizes functionality from ipmc_lib.h. All XXX_get() and _itr() functions
* are the ones from ipmc_lib.h, that is, vtss_appl_ipmc_lib_XXX_get() and
* vtss_appl_ipmc_lib_XXX_itr(). The key must have .is_mvr set to true and it
* really doesn't matter whether using .is_ipv4 equal to true or false, because
* configuration is identical for both IGMP and MLD (with the exception of
* the querier-address, which can be non-zero only for IGMP, so it's better to
* use .is_ipv4 = true).
*
* The _set() functions must use the vtss_appl_mvr_XXX_set()
*/

#ifndef _VTSS_APPL_MVR_HXX_
#define _VTSS_APPL_MVR_HXX_

#include <vtss/appl/ipmc_lib.h>        /* For vtss_appl_ipmc_lib_XXX */

/**
 * Set global configuration.
 *
 * \param conf [IN] Global configuration to be set.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_mvr_global_conf_set(const vtss_appl_ipmc_lib_global_conf_t *conf);

/**
 * Set port configuration.
 *
 * \param port_no [IN] Port number
 * \param conf    [IN] New port configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_mvr_port_conf_set(mesa_port_no_t port_no, const vtss_appl_ipmc_lib_port_conf_t *conf);

/**
 * Set port configuration for a particular MVR VLAN.
 *
 * \param vid     [IN] MVR VLAN
 * \param port_no [IN] Port number
 * \param conf    [IN] New port configuration for a particular VLAN
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_mvr_vlan_port_conf_set(mesa_vid_t vid, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_vlan_port_conf_t *conf);

/**
 * Set VLAN configuration.
 *
 * \param vid  [IN] MVR VLAN
 * \param conf [IN] New VLAN configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_mvr_vlan_conf_set(mesa_vid_t vid, const vtss_appl_ipmc_lib_vlan_conf_t *conf);

/**
 * Delete an MVR VLAN.
 *
 * \param vid [IN] MVR VLAN ID to delete.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_mvr_vlan_conf_del(mesa_vid_t vid);

/**
 * Clear statistics
 *
 * \param vid [IN] MVR VLAN
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_mvr_vlan_statistics_clear(mesa_vid_t vid);

#endif  /* _VTSS_APPL_MVR_HXX_ */
