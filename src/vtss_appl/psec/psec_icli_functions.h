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

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \file
 * \brief PSEC ICLI functions
 * \details This header file describes PSEC ICLI functions
 */

/**
 * \brief Function for printing Port Security status on one or more interfaces.
 *
 * \param session_id [IN] Needed for being able to print
 * \param plist      [IN] List of interfaces to print status for.
 * \return Error code.
 **/
mesa_rc psec_icli_show(i32 session_id, icli_stack_port_range_t *plist, BOOL debug);

/**
 * \brief Function for printing MAC addresses learned by Port Security on one or more interfaces.
 *
 * \param session_id [IN]  Needed for being able to print
 * \param plist      [IN] List of interfaces to print MAC addreses for.
 * \return Error code.
 **/
mesa_rc psec_icli_address_show(i32 session_id, icli_stack_port_range_t *plist, BOOL debug);

/**
 * \brief Function for removing one or more MAC addresses on one or more interfaces or one specific VLAN
 **/
mesa_rc psec_icli_mac_clear(i32 session_id, BOOL has_mac, BOOL has_vlan, mesa_mac_t mac, icli_stack_port_range_t *plist, mesa_vid_t vlan);

/**
 * \brief Function for showing current rate-limiter settings.
 **/
mesa_rc psec_icli_rate_limit_config_show(i32 session_id);

/**
 * \brief Function for setting new rate-limiter configuration.
 **/
mesa_rc psec_icli_rate_limit_config_set(i32 session_id, BOOL has_fill_level_min, u32 fill_level_min, BOOL has_fill_level_max, u32 fill_level_max, BOOL has_rate, u32 rate, BOOL has_drop_age, u32 drop_age);

/**
 * \brief Function for showing rate-limiter statistics
 **/
mesa_rc psec_icli_rate_limit_statistics_show(i32 session_id);

/**
 * \brief Function for clearing rate-limiter statistics
 **/
mesa_rc psec_icli_rate_limit_statistics_clear(i32 session_id);

#ifdef __cplusplus
}
#endif

