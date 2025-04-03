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
 * \file frr_router_icli_functions.hxx
 * \brief This file contains the definitions of FRR module's ICLI API functions.
 */

#ifndef _FRR_ROUTER_ICLI_FUNCTIONS_HXX_
#define _FRR_ROUTER_ICLI_FUNCTIONS_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include <vtss/appl/router.h>
#include "misc_api.h"        // For misc_ipv4_txt()
#include "vtss_icli_type.h"  // For icli_unsigned_range_t

/******************************************************************************/
/** Module ICLI request structure declaration                                 */
/******************************************************************************/
struct FrrRouterCliReq {
    FrrRouterCliReq(u32 &id)
    {
        session_id = id;
    }

    // CLI session ID
    u32 session_id;

    // Indicate if user has typed "no" keyword or not
    bool has_no_form = false;

    // Router access-list
    vtss_appl_router_access_list_name_t access_list_name = {};
    vtss_appl_router_access_list_mode_t access_list_mode =
        VTSS_APPL_ROUTER_ACCESS_LIST_MODE_COUNT;
    bool del_access_list_by_name = false;

    // RIP IPv4 network address
    mesa_ipv4_network_t ipv4_network = {};

    // Meteric value
    mesa_bool_t has_metric = false;
    vtss_appl_router_metric_t metric = 0;

    // Router key chain
    vtss_appl_router_key_chain_name_t keychain_name = {};
    mesa_bool_t has_keychain_key_id = false;
    mesa_bool_t has_key_string_encrypted = false;
    vtss_appl_router_key_chain_key_id_t keychain_key_id = {};
    char keychain_key_string[VTSS_APPL_ROUTER_KEY_CHAIN_ENCRYPTED_KEY_STR_MAX_LEN + 1];
};

/******************************************************************************/
/** Module ICLI APIs                                                          */
/******************************************************************************/
//----------------------------------------------------------------------------
//** Key-chain
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_router_key_chain_key_conf_set(const FrrRouterCliReq &req);
mesa_rc FRR_ICLI_router_key_chain_key_del(const FrrRouterCliReq &req);

//----------------------------------------------------------------------------
//** Access-list
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_router_access_list_set(const FrrRouterCliReq &req);

#endif /* _FRR_ROUTER_ICLI_FUNCTIONS_HXX_ */

