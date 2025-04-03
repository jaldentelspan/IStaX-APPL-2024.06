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
#include "frr_router_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_ROUTER
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::router::interfaces;

/******************************************************************************/
/** Register module JSON resources                                            */
/******************************************************************************/
namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_router("router");
extern "C" void frr_router_json_init()
{
    json_node_add(&ns_router);
}

/******************************************************************************/
/** Module JSON nodes                                                         */
/******************************************************************************/
/* Hierarchical overview
 *
 *  router
 *      .capabilities
 *      .config
 *          .key_chain
 *          .access_list
 *      .status
 *          .access_list
 */

/******************************************************************************/
/** The JSON Get-All functions                                                */
/******************************************************************************/
mesa_rc vtss_appl_router_key_chain_name_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
{
    using namespace vtss;
    using namespace vtss::expose;
    using namespace vtss::expose::json;

    HandlerFunctionExporterParser handler_in(req);

    /* Declarate response type.
     * Notice:
     * Use 'ResponseMapRowSingleKeySingleVal' for single entry key.
     * USe 'ResponseMapRowMultiKeySingleVal' for multiple entry keys. */
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_type;

    /* The handler type is derived from the response type. */
    typedef typename response_type::handler_type handler_type;

    /* Create an exporter which the output parameters is written to. */
    response_type response(os, req->id);

    /* Start serializing process */
    vtss::Vector<vtss_appl_router_key_chain_name_t> conf;
    if (vtss_appl_router_key_chain_name_get_all(conf) != VTSS_RC_OK || !conf.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return VTSS_RC_OK;
    }

    vtss_appl_router_key_chain_name_t name = {};

    for (const auto &itr : conf) {
        handler_type handler_out = response.resultHandler();

        strcpy(name.name, itr.name);
        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();
            serialize(key_handler, router_key_chain_name(name));

            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler entry key error: Router "
                        "key-chain name configuration";
                return VTSS_RC_ERROR;
            }
        }

        // No entry data
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_router_key_chain_key_conf_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
{
    using namespace vtss;
    using namespace vtss::expose;
    using namespace vtss::expose::json;

    HandlerFunctionExporterParser handler_in(req);

    /* Declarate response type.
     * Notice:
     * Use 'ResponseMapRowSingleKeySingleVal' for single entry key.
     * USe 'ResponseMapRowMultiKeySingleVal' for multiple entry keys. */
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_type;

    /* The handler type is derived from the response type. */
    typedef typename response_type::handler_type handler_type;

    /* Create an exporter which the output parameters is written to. */
    response_type response(os, req->id);

    /* Start serializing process */
    vtss::Map<vtss::Pair<std::string, uint32_t>, vtss_appl_router_key_chain_key_conf_t>
    conf;
    if (vtss_appl_router_key_chain_key_conf_get_all(conf) != VTSS_RC_OK ||
        !conf.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return VTSS_RC_OK;
    }

    vtss_appl_router_key_chain_name_t name = {};
    uint32_t key_id = 0;
    for (const auto &itr : conf) {
        handler_type handler_out = response.resultHandler();
        strcpy(name.name, itr.first.first.c_str());
        key_id = itr.first.second;
        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();
            serialize(key_handler, router_key_chain_name(name));
            serialize(key_handler, router_key_chain_key_id(key_id));

            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler entry key error: Router "
                        "key chain key configuration";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry data
            auto &&val_handler = handler_out.valHandler();
            // vtss_appl_router_ace_conf_t ace_conf = vtss::move(itr);
            vtss_appl_router_key_chain_key_conf_t keychain_conf =
                vtss::move(itr.second);
            serialize(val_handler, keychain_conf);
            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler entry data error: Router "
                        "key-chain key configuration";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_router_access_list_status_get_all_json(
    const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
{
    using namespace vtss;
    using namespace vtss::expose;
    using namespace vtss::expose::json;

    HandlerFunctionExporterParser handler_in(req);

    /* Declarate response type.
     * Notice:
     * Use 'ResponseMapRowSingleKeySingleVal' for single entry key.
     * USe 'ResponseMapRowMultiKeySingleVal' for multiple entry keys. */
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_type;

    /* The handler type is derived from the response type. */
    typedef typename response_type::handler_type handler_type;

    /* Create an exporter which the output parameters is written to. */
    response_type response(os, req->id);

    /* Start serializing process */
    vtss::Vector<vtss_appl_router_ace_conf_t> conf;
    if (vtss_appl_router_access_list_precedence_get_all(conf) != VTSS_RC_OK ||
        !conf.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return VTSS_RC_OK;
    }

    vtss_appl_router_access_list_name_t name = {};
    uint32_t precedence_key = 0;
    for (const auto &itr : conf) {
        handler_type handler_out = response.resultHandler();

        // Reset precedence_key when name is updated
        if (strcmp(name.name, itr.name)) {
            strcpy(name.name, itr.name);
            precedence_key = 0;
        }

        precedence_key++;

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();
            serialize(key_handler, router_key_access_list_name(name));
            serialize(key_handler,
                      router_key_access_list_precedence(precedence_key));

            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler entry key error: Router "
                        "access-list configuration";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry data
            auto &&val_handler = handler_out.valHandler();
            vtss_appl_router_ace_conf_t ace_conf = vtss::move(itr);
            serialize(val_handler, ace_conf);
            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler entry data error: Router "
                        "access-list configuration";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
/** The JSON Nodes                                                            */
/******************************************************************************/
namespace vtss
{
namespace appl
{
namespace frr
{
namespace interfaces
{
// router.config
NS(ns_conf, ns_router, "config");

// router.config.key_chain
NS(ns_conf_keychain, ns_conf, "key_chain");

// router.capabilities
static StructReadOnly<RouterCapabilitiesTabular> router_capabilities_tabular(
    &ns_router, "capabilities");

// router.status
NS(ns_status, ns_router, "status");

//----------------------------------------------------------------------------
//** Key-chain
//----------------------------------------------------------------------------
// router.config.key_chain
static TableReadWriteAddDelete<RouterConfigKeyChainNameEntry>
router_config_key_chain_name_table(&ns_conf_keychain, "name");

// router.config.key_chain.key_id
static TableReadWriteAddDelete<RouterConfigKeyChainKeyConfEntry>
router_config_key_chain_key_conf_table(&ns_conf_keychain, "key");

//----------------------------------------------------------------------------
//** Access-list
//----------------------------------------------------------------------------
// router.config.access_list
static TableReadWriteAddDelete<RouterConfigAccessListEntry>
router_config_access_list_table(&ns_conf, "access_list");

// router.status.access_list
static TableReadOnly<RouterStatusAccessListEntry> router_status_access_list_table(
    &ns_status, "access_list");

}  // namespace interfaces
}  // namespace frr
}  // namespace appl
}  // namespace vtss

