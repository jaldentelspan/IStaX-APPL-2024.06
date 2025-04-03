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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr_router_icli_functions.hxx"
#include "frr_router_api.hxx"
#include "icli_api.h"
#include "icli_porting_util.h"
#include "ip_utils.hxx"  // For vtss_conv_ipv4mask_to_prefix()
#include "vtss/appl/router.h"
#include "vtss/basics/stream.hxx"  // For vtss::BufStream

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_ICLI
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using-declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Module ICLI APIs                                                          */
/******************************************************************************/
//----------------------------------------------------------------------------
//** Key-chain
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_router_key_chain_key_conf_set(const FrrRouterCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_router_key_chain_key_conf_t conf;

    conf.is_encrypted = req.has_key_string_encrypted;
    strcpy(conf.key, req.keychain_key_string);

    rc = vtss_appl_router_key_chain_key_conf_add(&req.keychain_name,
                                                 req.keychain_key_id, &conf);
    if (rc != VTSS_RC_OK) {
        if (rc == FRR_RC_ENTRY_ALREADY_EXISTS) {
            ICLI_PRINTF("%% The same key ID is already existing.\n");
        } else {
            ICLI_PRINTF("%% %s.\n", error_txt(rc));
        }
    }

    return ICLI_RC_OK;
}

mesa_rc FRR_ICLI_router_key_chain_key_del(const FrrRouterCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;

    /* Delete the key */
    rc = vtss_appl_router_key_chain_key_conf_del(&req.keychain_name,
                                                 req.keychain_key_id);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** Access-list
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_router_access_list_set(const FrrRouterCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;

    if (req.has_no_form && req.del_access_list_by_name) {  // Delete by name
        rc = frr_router_access_list_conf_del_by_name(&req.access_list_name);
        if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("%% Delete access-list name %s failed.\n",
                        req.access_list_name.name);
        }
    } else if (req.has_no_form) {  // Delete operation
        rc = vtss_appl_router_access_list_conf_del(
                 &req.access_list_name, req.access_list_mode, req.ipv4_network);
        if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("%% Delete access-list entry failed.\n");
        }
    } else {  // Add operation
        rc = vtss_appl_router_access_list_conf_add(
                 &req.access_list_name, req.access_list_mode, req.ipv4_network);
        if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("%% %s.\n", error_txt(rc));
        }
    }

    return ICLI_RC_OK;
}

