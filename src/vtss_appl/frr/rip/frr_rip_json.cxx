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
#include "frr_rip_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_RIP
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::rip::interfaces;

/******************************************************************************/
/** Register module JSON resources                                            */
/******************************************************************************/
namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_rip("rip");
extern "C" void frr_rip_json_init()
{
    json_node_add(&ns_rip);
}

/******************************************************************************/
/** Module JSON nodes                                                         */
/******************************************************************************/
/* Hierarchical overview
 *
 *  rip
 *      .capabilities
 *      .config
 *          .router
 *          .router_interface
 *          .network
 *          .neighbor
 *          .interface
 *          .offset_list
 *      .status
 *          .general
 *          .interface
 *          .peer
 *      .control
 *          .global
 */

/******************************************************************************/
/** The JSON Get-All functions                                                */
/******************************************************************************/
mesa_rc vtss_appl_rip_db_get_all_json(const vtss::expose::json::Request *req,
                                      vtss::ostreamBuf *os)
{
    using namespace vtss;
    using namespace vtss::expose;
    using namespace vtss::expose::json;

    HandlerFunctionExporterParser handler_in(req);

    /* Declarate response type.
     * Notice:
     * Use 'ResponseMapRowSingleKeySingleVal' for single entry key.
     * USe 'ResponseMapRowMultiKeySingleVal' for multiple entry keys. */
    typedef ResponseMap<ResponseMapRowSingleKeySingleVal> response_type;

    /* The handler type is derived from the response type. */
    typedef typename response_type::handler_type handler_type;

    /* Create an exporter which the output parameters is written to. */
    response_type response(os, req->id);

    /* Start serializing process */
    vtss::Vector<vtss::Pair<vtss_appl_rip_db_key_t, vtss_appl_rip_db_data_t>>
                                                                           database = {};
    if (vtss_appl_rip_db_get_all(database) != VTSS_RC_OK || !database.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return VTSS_RC_OK;
    }

    for (const auto &itr : database) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();
            vtss_appl_rip_db_key_t key = vtss::move(itr.first);
            serialize(key_handler, key);

            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler error: RIP database entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry data
            auto &&val_handler = handler_out.valHandler();
            vtss_appl_rip_db_data_t data = vtss::move(itr.second);
            serialize(val_handler, data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: RIP database "
                                  "entry data";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rip_peer_get_all_json(const vtss::expose::json::Request *req,
                                        vtss::ostreamBuf *os)
{
    using namespace vtss;
    using namespace vtss::expose;
    using namespace vtss::expose::json;

    HandlerFunctionExporterParser handler_in(req);

    /* Declarate response type.
     * Notice:
     * Use 'ResponseMapRowSingleKeySingleVal' for single entry key.
     * USe 'ResponseMapRowMultiKeySingleVal' for multiple entry keys. */
    typedef ResponseMap<ResponseMapRowSingleKeySingleVal> response_type;

    /* The handler type is derived from the response type. */
    typedef typename response_type::handler_type handler_type;

    /* Create an exporter which the output parameters is written to. */
    response_type response(os, req->id);

    /* Start serializing process */
    vtss::Vector<vtss::Pair<mesa_ipv4_t, vtss_appl_rip_peer_data_t>> database = {};
    if (vtss_appl_rip_peer_get_all(database) != VTSS_RC_OK || !database.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return VTSS_RC_OK;
    }

    for (const auto &itr : database) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();
            mesa_ipv4_t key = vtss::move(itr.first);
            serialize(key_handler, rip_key_ipv4_addr(key));

            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler error: RIP peer entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry data
            auto &&val_handler = handler_out.valHandler();
            vtss_appl_rip_peer_data_t data = vtss::move(itr.second);
            serialize(val_handler, data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: RIP peer "
                                  "entry data";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rip_intf_status_get_all_json(
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
    typedef ResponseMap<ResponseMapRowSingleKeySingleVal> response_type;

    /* The handler type is derived from the response type. */
    typedef typename response_type::handler_type handler_type;

    /* Create an exporter which the output parameters is written to. */
    response_type response(os, req->id);

    /* Start serializing process */
    vtss::Vector<vtss::Pair<vtss_ifindex_t, vtss_appl_rip_interface_status_t>>
                                                                            database = {};
    if (vtss_appl_rip_interface_status_get_all(database) != VTSS_RC_OK ||
        !database.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return VTSS_RC_OK;
    }

    for (const auto &itr : database) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();
            vtss_ifindex_t key = vtss::move(itr.first);
            serialize(key_handler, rip_key_intf_index(key));

            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler error: RIP peer entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry data
            auto &&val_handler = handler_out.valHandler();
            vtss_appl_rip_interface_status_t data = vtss::move(itr.second);
            serialize(val_handler, data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: RIP peer "
                                  "entry data";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rip_offset_list_conf_get_all_json(
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
    vtss::Vector<vtss::Pair<vtss_appl_rip_offset_entry_key_t, vtss_appl_rip_offset_entry_data_t>>
            conf;
    if (vtss_appl_rip_offset_list_conf_get_all(conf) != VTSS_RC_OK || !conf.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return VTSS_RC_OK;
    }

    for (const auto &itr : conf) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();
            vtss_appl_rip_offset_entry_key_t entry_key = vtss::move(itr.first);
            serialize(key_handler, rip_key_intf_index(entry_key.ifindex));
            serialize(key_handler,
                      vtss_appl_rip_offset_direction_key(entry_key.direction));

            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler entry key error: RIP "
                                  "offset-list configuration";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry data
            auto &&val_handler = handler_out.valHandler();
            vtss_appl_rip_offset_entry_data_t entry_data = vtss::move(itr.second);
            serialize(val_handler, entry_data);
            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler entry data error: RIP "
                                  "offset-list configuration";
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

// rip.config
NS(ns_conf, ns_rip, "config");

// rip.capabilities
static StructReadOnly<RipCapabilitiesTabular> rip_capabilities_tabular(
    &ns_rip, "capabilities");

// rip.status
NS(ns_status, ns_rip, "status");

// rip.control
NS(ns_control, ns_rip, "control");

// rip.config.router
static StructReadWrite<RipConfigRouterTabular> rip_config_router_tabular(
    &ns_conf, "router");

// ospf.config.router_interface
static TableReadWrite<RipConfigRouterInterfaceEntry>
rip_config_router_interface_table(&ns_conf, "router_interface");

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
// rip.config.network
static TableReadWriteAddDelete<RipConfigNetworkEntry> rip_config_network_table(
    &ns_conf, "network");

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
// rip.config.neighbor
static TableReadWriteAddDelete<RipConfigNeighborEntry> rip_config_neighbor_table(
    &ns_conf, "neighbor");

//----------------------------------------------------------------------------
//** RIP interface configuration
//----------------------------------------------------------------------------
// rip.config.interface
static TableReadWrite<RipConfigInterfaceEntry> rip_config_interface_table(
    &ns_conf, "interface");

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
// rip.config.offset_list
static TableReadWriteAddDelete<RipConfigOffsetListEntry>
router_config_offset_list_table(&ns_conf, "offset_list");

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
static StructReadOnly<RipStatusGeneralEntry> rip_general_status_table(
    &ns_status, "general");

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
static TableReadOnly<RipStatusInterfaceEntry> rip_interface_status_table(&ns_status, "interface");

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
// rip.status.peer
static TableReadOnly<RipStatusPeerEntry> rip_status_peer_table(&ns_status,
                                                               "peer");

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
static TableReadOnly<RipStatusDbEntry> rip_status_db_table(&ns_status, "db");

// rip.control.globals
static StructWriteOnly<RIpControlGlobalsTabular> rip_control_globals_tabular(
    &ns_control, "globals");

}  // namespace interfaces
}  // namespace frr
}  // namespace appl
}  // namespace vtss

