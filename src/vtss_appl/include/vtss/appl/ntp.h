/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief Public NTP API
 * \details Header file of NTP (Network Time Protocol).
 *  This header file describes NTP control functions and types.
 */

#ifndef _VTSS_APPL_NTP_H_
#define _VTSS_APPL_NTP_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/** NTP maximum server count */
#define VTSS_APPL_NTP_SERVER_MAX_COUNT              5

/**
 * \brief NTP global configuration
 */
typedef struct {
    /** 
     * \brief Enable/disable NTP functionality.
     */
    mesa_bool_t    mode;
} vtss_appl_ntp_global_config_t;

/**
 * \brief NTP server configuration.
 */
typedef struct {
    /**
     * \brief Address of the NTP server.
     */
    vtss_inet_address_t     address;
} vtss_appl_ntp_server_config_t;

/**
 * \brief Get NTP Parameters
 *
 * To read current global parameters in NTP.
 *
 * \param conf [OUT] The NTP global configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_global_config_get(
    vtss_appl_ntp_global_config_t                       *const conf
);

/**
 * \brief Set NTP Parameters
 *
 * To modify current global parameters in NTP.
 *
 * \param conf [IN] The NTP global configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_global_config_set(
    const vtss_appl_ntp_global_config_t                 *const conf
);

/**
 * \brief Iterate function of NTP Server Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_serverIdx [IN]  previous server index number.
 * \param next_serverIdx [OUT] next server index number.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_server_config_itr(
    const uint32_t                                   *const prev_serverIdx,
    uint32_t                                         *const next_serverIdx
);

/**
 * \brief Get NTP Server Configuration
 *
 * To read configuration of the server in NTP.
 *
 * \param serverIdx  [IN]  Index number of the server.
 *                               
 * \param serverConf [OUT] The current configuration of the server
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_server_config_get(
    uint32_t                                         serverIdx,
    vtss_appl_ntp_server_config_t               *const serverConf
);

/**
 * \brief Set NTP Server Configuration
 *
 * To modify configuration of the server in NTP.
 *
 * \param serverIdx  [IN] Index number of the server.
 *
 * \param serverConf [IN] The configuration set to the server
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ntp_server_config_set(
    uint32_t                                         serverIdx,
    const vtss_appl_ntp_server_config_t         *const serverConf
);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_NTP_H_ */

