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
 * \brief Public GVRP API
 * \details This header file describes GVRP control functions and types.
 *          GVRP is specified in IEEE 802.1Q-2005, clause 11, and is
 *          build on top of GARP, IEEE 802.1D-2004.
 */

#ifndef _VTSS_APPL_GVRP_H_
#define _VTSS_APPL_GVRP_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief GVRP global configuration
 */
typedef struct {
    /** Global mode of GVRP, TRUE - enable GVRP, FALSE - disable GVRP. */
    mesa_bool_t    mode;

    /** Join period time in the unit of centisecond [cs]. */
    uint32_t     join_time;

    /** Leave period time in the unit of centisecond [cs]. */
    uint32_t     leave_time;

    /** Leave all period time in the unit of centisecond [cs]. */
    uint32_t     leave_all_time;

    /** Number of VLANs that GVRP can simultaneously control. */
    uint32_t     max_vlans;
} vtss_appl_gvrp_config_globals_t;


/** \brief GVRP interface configuration. */
typedef struct {
    /** Per-port mode of GVRP,
      * TRUE - enable GVRP on this interface,
      * FALSE - disable GVRP on this interface. */
    mesa_bool_t    mode;
} vtss_appl_gvrp_config_interface_t;

/**
 * \brief Get GVRP global configuration.
 *
 * \param config [OUT] The data pointer of configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_globals_get(
    vtss_appl_gvrp_config_globals_t     *const config
);

/**
 * \brief Set GVRP global configuration.
 *
 * \param config [IN] The data pointer of configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_globals_set(
    const vtss_appl_gvrp_config_globals_t   *const config
);

/**
 * \brief Iterate function of interface table.
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 *                           If this is a NULL pointer then
 *                           it is used for getting the first ifindex.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_interface_itr(
    const vtss_ifindex_t    *prev_ifindex,
    vtss_ifindex_t          *next_ifindex
);

/**
 * \brief Get GVRP interface configuration.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param config  [OUT] The data pointer of interface configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_interface_get(
    vtss_ifindex_t                      ifindex,
    vtss_appl_gvrp_config_interface_t   *const config
);

/**
 * \brief Set GVRP interface configuration.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param config  [OUT] The data pointer of interface configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_interface_set(
    vtss_ifindex_t                              ifindex,
    const vtss_appl_gvrp_config_interface_t     *const config
);

#ifdef __cplusplus
}
#endif
#endif  // _VTSS_APPL_GVRP_H_
