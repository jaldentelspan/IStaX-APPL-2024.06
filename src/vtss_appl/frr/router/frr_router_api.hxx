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
 * \file frr_router_api.h
 * \brief This file contains the definitions of Router internal API functions,
 * including the application public header APIs (naming starts with
 * vtss_appl_router) and the internal module APIs (naming starts with frr_router)
 */

#ifndef _FRR_ROUTER_API_HXX_
#define _FRR_ROUTER_API_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr.hxx"
#include <vtss/appl/router.h>

/******************************************************************************/
/** Module initialization                                                     */
/******************************************************************************/
mesa_rc frr_router_init(vtss_init_data_t *data);
const char *frr_router_error_txt(mesa_rc rc);

/******************************************************************************/
/** Module internal APIs                                                      */
/******************************************************************************/
/**
 * Check if the name of router access-list is valid or not
 * It allows all printable chacters excluding space.
 *
 * @param  name [IN]
 * @return
 * true when the name is valid. Otherwise, false.
 */
mesa_bool_t ROUTER_name_is_valid(const char *const name);

/**
 * \brief Delete the router access-list configuration by name.
 * \param name    [IN] The access-list name
 * \return Error code.
 */
mesa_rc frr_router_access_list_conf_del_by_name(
    const vtss_appl_router_access_list_name_t *const name);

#endif /* _FRR_ROUTER_API_HXX_ */

