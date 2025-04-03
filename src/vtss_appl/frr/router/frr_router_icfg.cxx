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
#include "frr_router_icfg.hxx"
#include "frr_router_api.hxx"
#include "icfg_api.h"
#include "icli_porting_util.h"
#include "ip_utils.hxx"  // For the operator of mesa_ipv4_network_t, vtss_ipv4_prefix_to_mask()
#include "mgmt_api.h"  // For mgmt_vid_list_to_txt()
#include "misc_api.h"  // For misc_ipv4_txt()

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_ICFG
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using-declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Internal variables and APIs                                               */
/******************************************************************************/
/* ICFG callback functions for Router global mode */
static mesa_rc frr_router_global_conf_mode(const vtss_icfg_query_request_t *req,
                                           vtss_icfg_query_result_t *result)
{
    // Access-list
    /* Command: router access-list <word1-31> {permit|deny} {any|<ipv4_addr>
     * <ipv4_mask>} */
    mesa_ipv4_network_t any_ipv4_network = {};
    mesa_ipv4_t network_mask;
    char str_buf[16];
    vtss::Vector<vtss_appl_router_ace_conf_t> conf;

    /* Walk through all router ACEs by precedence */
    if (vtss_appl_router_access_list_precedence_get_all(conf) == VTSS_RC_OK) {
        for (const auto &itr : conf) {
            network_mask = vtss_ipv4_prefix_to_mask(itr.network.prefix_size);

            VTSS_RC(vtss_icfg_printf( result, "router access-list %s %s", itr.name, itr.mode == VTSS_APPL_ROUTER_ACCESS_LIST_MODE_PERMIT ? "permit" : "deny"));
            if (itr.network == any_ipv4_network) {
                VTSS_RC(vtss_icfg_printf(result, " any\n"));
            } else {
                VTSS_RC(vtss_icfg_printf( result, " %s", misc_ipv4_txt(itr.network.address, str_buf)));
                VTSS_RC(vtss_icfg_printf(result, " %s\n", misc_ipv4_txt(network_mask, str_buf)));
            }
        }
    }

    return VTSS_RC_OK;
}

/* ICFG callback functions for Router key-chain mode */
static mesa_rc frr_router_keychain_conf_mode(const vtss_icfg_query_request_t *req,
                                             vtss_icfg_query_result_t *result)
{
    /* Command: key <key-id> key-string { unencrypted <word1-63> |
     *                                    encrypted <word128-224> }
     */

    /* Walk through all router key chain entries,
     * and then only process the same key-chain name with the valid key ID.
     */
    Map<vtss::Pair<std::string, uint32_t>, vtss_appl_router_key_chain_key_conf_t> key_conf;
    mesa_rc rc = vtss_appl_router_key_chain_key_conf_get_all(key_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    auto itr = key_conf.greater_than_or_equal({req->instance_id.string, 0});
    while (itr != key_conf.end()) {
        if (strcmp(itr->first.first.c_str(), req->instance_id.string) ||
            itr->first.second == VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY) {
            // Only process the same key-chain name with the valid key ID
            break;
        }

        // Output command of the key-chain key ID
        VTSS_RC(vtss_icfg_printf(
                    result, " key %u key-string %s %s\n", itr->first.second,
                    itr->second.is_encrypted ? "encrypted" : "unencrypted",
                    itr->second.key));

        // Next loop
        itr++;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
/** Module ICFG initialization                                                */
/******************************************************************************/
/**
 * \brief FRR ICFG initialization function.
 *
 * Call once, preferably from the INIT_CMD_INIT section of the module's init()
 * function.
 */
mesa_rc frr_router_icfg_init(void)
{
    /* Register callback functions to ICFG module.
     *  The configuration divided into two groups for this module.
     *  1. Global mode configuration
     *  2. Router key-chain sub-mode configuration
     */
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_ROUTER_GLOBAL_CONF,
                                     "router_global_conf",
                                     frr_router_global_conf_mode));

    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_ROUTER_KEYCHAIN_CONF,
                                     "router_keychain_conf",
                                     frr_router_keychain_conf_mode));
    return VTSS_RC_OK;
}

