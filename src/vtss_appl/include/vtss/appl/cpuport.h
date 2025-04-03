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
 * \brief Public Cpuport API
 * \details This header file describes cpuport control functions and types
 */

#ifndef _VTSS_APPL_CPUPORT_H_
#define _VTSS_APPL_CPUPORT_H_

#include <vtss/basics/api_types.h>
#include <vtss/appl/port.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Function for getting a default CPU port configuration
 * \param ifindex [IN]  The logical interface index.
 * \param conf    [OUT] Pointer to where to put the default configuration
 * \return VTSS_RC_OK on success else error code
 */
mesa_rc vtss_appl_cpuport_conf_default_get(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *conf);

/**
 * \brief Function for getting configuration for a specific CPU port
 * \param ifindex [IN]  The logical interface index.
 * \param conf    [OUT] Pointer to where to put the configuration
 * \return VTSS_RC_OK on success else error code
 */
mesa_rc vtss_appl_cpuport_conf_get(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *conf);

/**
 * \brief Function for setting configuration for a specific CPU port
 * \param ifindex [IN]  The logical interface index.
 * \param conf    [OUT] Pointer to where to the configuration
 * \return VTSS_RC_OK on success else error code
 */
mesa_rc vtss_appl_cpuport_conf_set(vtss_ifindex_t ifindex, const vtss_appl_port_conf_t *conf);

/**
 * \brief Function for getting status for a specific CPU port
 * \param ifindex [IN]  The logical interface index.
 * \param status  [OUT] Pointer to where to put the status
 * \return VTSS_RC_OK on success else error code
 */
mesa_rc vtss_appl_cpuport_status_get(vtss_ifindex_t ifindex, vtss_appl_port_status_t *status);

/**
 * \brief Function for getting statistics for a specific CPU port
 * \param ifindex    [IN]  The logical interface index.
 * \param statistics [OUT] Pointer to where to put the counters
 * \return VTSS_RC_OK if counters are valid else error code
 */
mesa_rc vtss_appl_cpuport_statistics_get(vtss_ifindex_t ifindex, mesa_port_counters_t *statistics);

/**
 * \brief Function for clearing statistics for a specific CPU port
 * \param ifindex [IN] The logical interface index.
 * \return VTSS_RC_OK if counters are cleared successfully, otherwise error code
 */
mesa_rc vtss_appl_cpuport_statistics_clear(vtss_ifindex_t ifindex);

#ifdef __cplusplus
}
#endif

#endif  // _VTSS_APPL_CPUPORT_H_
