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
#include <vtss/appl/psec.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \file
 * \brief port security icli functions
 * \details This header file describes port security icli functions
 */

/**
 * \brief Function for enable/disable port security for a specific interface
 *
 * \param session_id [IN]  Needed for being able to print
 * \param plist [IN] List of interfaces to print statistics for.
 * \param no    [IN] TRUE to set to default.
 * \return Error code.
 **/
mesa_rc psec_limit_icli_enable(i32 session_id, icli_stack_port_range_t *plist, BOOL no);

/**
 * \brief Function for setting the maximum number of MAC addresses
 *
 * \param session_id [IN]  Needed for being able to print
 * \param plist [IN] List of interfaces to print statistics for.
 * \param limit [IN] Max number of addresses
 * \param no    [IN] TRUE to set to default.
 * \return Error code.
 **/
mesa_rc psec_limit_icli_maximum(i32 session_id, icli_stack_port_range_t *plist, u32 limit, BOOL no);

/**
 * \brief Function for setting the maximum number of violating MAC addresses
 *
 * \param session_id    [IN]  Needed for being able to print
 * \param plist         [IN] List of interfaces to print statistics for.
 * \param violate_limit [IN] Max number of addresses
 * \param no            [IN] TRUE to set to default.
 * \return Error code.
 **/
mesa_rc psec_limit_icli_maximum_violation(i32 session_id, icli_stack_port_range_t *plist, u32 violate_limit, BOOL no);

/**
 * \brief Function for setting what to do in case of port security violation
 *
 * \param session_id [IN]  Needed for being able to print
 * \param plist [IN] List of interfaces to print statistics for.
 * \param has_protect [IN] TRUE to set violation_mode to protect
 * \param has_restrict [IN] TRUE to set violation_mode to restrict
 * \param has_shutdown [IN] TRUE to set violation_mode to shutdown
 * \param no    [IN] TRUE to set to default.
 * \return Error code.
 **/
mesa_rc psec_limit_icli_violation(i32 session_id, BOOL has_protect, BOOL has_restrict, BOOL has_shutdown, icli_stack_port_range_t *plist, BOOL no);

/**
 */
mesa_rc psec_limit_icli_mac_address(i32 session_id, icli_stack_port_range_t *plist, vtss_appl_psec_mac_conf_t *mac_conf, BOOL no);

/**
 * \brief Function for enabling aging
 *
 * \param session_id [IN]  Needed for being able to print
 * \param no    [IN] TRUE to set to default.
 * \return Error code.
 **/
mesa_rc psec_limit_icli_aging(i32 session_id, BOOL no);

/**
 * \brief Function for setting the aging time
 *
 * \param session_id [IN]  Needed for being able to print
 * \param value [IN] New aging time
 * \param no    [IN] TRUE to set to default.
 * \return Error code.
 **/
mesa_rc psec_limit_icli_aging_time(i32 session_id, u32 value, BOOL no);

/**
 * \brief Function for setting the hold time
 *
 * \param session_id [IN]  Needed for being able to print
 * \param value [IN] New hold time
 * \param no    [IN] TRUE to set to default.
 * \return Error code.
 **/
mesa_rc psec_limit_icli_hold_time(i32 session_id, u32 value, BOOL no);

mesa_rc psec_limit_icli_debug_ref_cnt(i32 session_id, icli_stack_port_range_t *plist);

/**
 * \brief Init function ICLI CFG
 * \return Error code.
 **/
mesa_rc psec_limit_icfg_init(void);

#ifdef __cplusplus
}
#endif

