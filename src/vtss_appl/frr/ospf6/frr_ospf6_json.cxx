/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "frr_ospf6_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_OSPF6
#include "frr_trace.hxx"  // For module trace group definitions

mesa_rc vtss_appl_ospf6_interface_status_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
{
    using namespace vtss;
    using namespace vtss::expose;
    using namespace vtss::expose::json;

    HandlerFunctionExporterParser handler_in(req);
    /*
        Single   key use ResponseMapRowSingleKeySingleVal
        Multiple key use ResponseMapRowMultiKeySingleVal
    */
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_type;

    // The handler type is derived from the response type.
    typedef typename response_type::handler_type handler_type;

    // Create an exporter which the output parameters is written to
    response_type response(os, req->id);

    vtss::Map<vtss_ifindex_t, vtss_appl_ospf6_interface_status_t> interface;

    (void)vtss_appl_ospf6_interface_status_get_all(interface);

    for (const auto &itr : interface) {
        handler_type handler_out = response.resultHandler();

        // serialize the key
        {
            auto &&key_handler = handler_out.keyHandler();
            vtss_ifindex_t ifindex = itr.first;

            serialize(key_handler, ospf6_key_intf_index(ifindex));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Key Handler error";
                return VTSS_RC_ERROR;
            }
        }

        // serialize the value
        {
            auto &&val_handler = handler_out.valHandler();
            vtss_appl_ospf6_interface_status_t status = itr.second;
            serialize(val_handler, status);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Value Handler error";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::ospf6::interfaces;

mesa_rc vtss_appl_ospf6_neighbor_status_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
{
    using namespace vtss;
    using namespace vtss::expose;
    using namespace vtss::expose::json;

    HandlerFunctionExporterParser handler_in(req);
    /*
        Single   key use ResponseMapRowSingleKeySingleVal
        Multiple key use ResponseMapRowMultiKeySingleVal
    */
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_type;

    // The handler type is derived from the response type.
    typedef typename response_type::handler_type handler_type;

    // Create an exporter which the output parameters is written to
    response_type response(os, req->id);

    vtss::Vector<vtss_appl_ospf6_neighbor_data_t> neighbors;

    (void)vtss_appl_ospf6_neighbor_status_get_all(neighbors);

    for (const auto &it : neighbors) {
        handler_type handler_out = response.resultHandler();

        // serialize the key
        {
            auto &&key_handler = handler_out.keyHandler();
            vtss_appl_ospf6_id_t id = it.id;
            vtss_appl_ospf6_router_id_t nid = it.neighbor_id;
            mesa_ipv6_t nip = it.neighbor_ip;
            vtss_ifindex_t ifidx = it.neighbor_ifidx;

            serialize(key_handler, ospf6_key_instance_id(id));
            serialize(key_handler, ospf6_key_router_id(nid));
            serialize(key_handler, ospf6_key_ipv6_addr(nip));
            serialize(key_handler, ospf6_key_intf_index(ifidx));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Key Handler error";
                return VTSS_RC_ERROR;
            }
        }

        // serialize the value
        {
            auto &&val_handler = handler_out.valHandler();
            vtss_appl_ospf6_neighbor_status_t status = it.status;
            serialize(val_handler, status);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Value Handler error";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_ospf6_route_ipv6_status_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
{
    using namespace vtss;
    using namespace vtss::expose;
    using namespace vtss::expose::json;

    HandlerFunctionExporterParser handler_in(req);
    /*
        Single   key use ResponseMapRowSingleKeySingleVal
        Multiple key use ResponseMapRowMultiKeySingleVal
    */
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_type;

    // The handler type is derived from the response type.
    typedef typename response_type::handler_type handler_type;

    // Create an exporter which the output parameters is written to
    response_type response(os, req->id);

    vtss::Vector<vtss_appl_ospf6_route_ipv6_data_t> routes;

    /* 'routes' is still empty if any errors occurs, so the error handler
       can be ignored. */
    (void)vtss_appl_ospf6_route_ipv6_status_get_all(routes);

    for (auto &it : routes) {
        handler_type handler_out = response.resultHandler();

        // serialize the key
        {
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(it.id));
            serialize(key_handler, ospf6_key_route_type(it.rt_type));
            serialize(key_handler, ospf6_key_ipv6_network(it.dest));
            serialize(key_handler, ospf6_key_route_area(it.area));
            serialize(key_handler, ospf6_key_ipv6_addr(it.nexthop));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Key Handler error";
                return VTSS_RC_ERROR;
            }
        }

        // serialize the value
        {
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, it.status);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Value Handler error";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_ospf6_db_get_all_json(const vtss::expose::json::Request *req,
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
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_type;

    /* The handler type is derived from the response type. */
    typedef typename response_type::handler_type handler_type;

    /* Create an exporter which the output parameters is written to. */
    response_type response(os, req->id);

    /* Start serializing process */
    vtss::Vector<vtss_appl_ospf6_db_entry_t> db_entries = {};
    (void)vtss_appl_ospf6_db_get_all(db_entries);

    for (auto &itr : db_entries) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(itr.inst_id));
            serialize(key_handler, ospf6_key_area_id(itr.area_id));
            serialize(key_handler, ospf6_key_db_lsdb_type(itr.lsdb_type));
            serialize(key_handler, ospf6_key_db_link_state_id(itr.link_state_id));
            serialize(key_handler, ospf6_key_db_adv_router_id(itr.adv_router_id));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG)
                        << "Serialize Handler error: OSPF6 database entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry values
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, itr.db);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "entry values";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/* Link */
mesa_rc vtss_appl_ospf6_db_link_get_all_json(
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
    vtss::Vector<vtss_appl_ospf6_db_detail_link_entry_t> db_entries = {};
    (void)vtss_appl_ospf6_db_detail_link_get_all(db_entries);

    for (auto &itr : db_entries) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(itr.inst_id));
            serialize(key_handler, ospf6_key_area_id(itr.area_id));
            serialize(key_handler, ospf6_key_db_lsdb_type(itr.lsdb_type));
            serialize(key_handler, ospf6_key_db_link_state_id(itr.link_state_id));
            serialize(key_handler, ospf6_key_db_adv_router_id(itr.adv_router_id));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail router entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry values
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, itr.data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail router entry values";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/* Intra Area Prefix */
mesa_rc vtss_appl_ospf6_db_intra_area_prefix_get_all_json(
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
    vtss::Vector<vtss_appl_ospf6_db_detail_intra_area_prefix_entry_t> db_entries = {};
    (void)vtss_appl_ospf6_db_detail_intra_area_prefix_get_all(db_entries);

    for (auto &itr : db_entries) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(itr.inst_id));
            serialize(key_handler, ospf6_key_area_id(itr.area_id));
            serialize(key_handler, ospf6_key_db_lsdb_type(itr.lsdb_type));
            serialize(key_handler, ospf6_key_db_link_state_id(itr.link_state_id));
            serialize(key_handler, ospf6_key_db_adv_router_id(itr.adv_router_id));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail router entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry values
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, itr.data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail router entry values";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/* type 1 */
mesa_rc vtss_appl_ospf6_db_router_get_all_json(
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
    vtss::Vector<vtss_appl_ospf6_db_detail_router_entry_t> db_entries = {};
    (void)vtss_appl_ospf6_db_detail_router_get_all(db_entries);

    for (auto &itr : db_entries) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(itr.inst_id));
            serialize(key_handler, ospf6_key_area_id(itr.area_id));
            serialize(key_handler, ospf6_key_db_lsdb_type(itr.lsdb_type));
            serialize(key_handler, ospf6_key_db_link_state_id(itr.link_state_id));
            serialize(key_handler, ospf6_key_db_adv_router_id(itr.adv_router_id));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail router entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry values
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, itr.data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail router entry values";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/* type 2 */
mesa_rc vtss_appl_ospf6_db_network_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
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
    vtss::Vector<vtss_appl_ospf6_db_detail_network_entry_t> db_entries = {};
    (void)vtss_appl_ospf6_db_detail_network_get_all(db_entries);

    for (auto &itr : db_entries) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(itr.inst_id));
            serialize(key_handler, ospf6_key_area_id(itr.area_id));
            serialize(key_handler, ospf6_key_db_lsdb_type(itr.lsdb_type));
            serialize(key_handler, ospf6_key_db_link_state_id(itr.link_state_id));
            serialize(key_handler, ospf6_key_db_adv_router_id(itr.adv_router_id));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail network entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry values
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, itr.data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail network entry values";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/* type 3 */
mesa_rc vtss_appl_ospf6_db_inter_area_prefix_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
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
    vtss::Vector<vtss_appl_ospf6_db_detail_inter_area_prefix_entry_t> db_entries = {};
    (void)vtss_appl_ospf6_db_detail_inter_area_prefix_get_all(db_entries);

    for (auto &itr : db_entries) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(itr.inst_id));
            serialize(key_handler, ospf6_key_area_id(itr.area_id));
            serialize(key_handler, ospf6_key_db_lsdb_type(itr.lsdb_type));
            serialize(key_handler, ospf6_key_db_link_state_id(itr.link_state_id));
            serialize(key_handler, ospf6_key_db_adv_router_id(itr.adv_router_id));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail summary entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry values
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, itr.data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail summary entry values";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/* type 4 */
mesa_rc vtss_appl_ospf6_db_inter_area_router_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
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
    vtss::Vector<vtss_appl_ospf6_db_detail_inter_area_router_entry_t> db_entries = {};
    (void)vtss_appl_ospf6_db_detail_inter_area_router_get_all(db_entries);

    for (auto &itr : db_entries) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(itr.inst_id));
            serialize(key_handler, ospf6_key_area_id(itr.area_id));
            serialize(key_handler, ospf6_key_db_lsdb_type(itr.lsdb_type));
            serialize(key_handler, ospf6_key_db_link_state_id(itr.link_state_id));
            serialize(key_handler, ospf6_key_db_adv_router_id(itr.adv_router_id));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail asbr summary entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry values
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, itr.data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail asbr summary entry values";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/* type 5 */
mesa_rc vtss_appl_ospf6_db_external_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
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
    vtss::Vector<vtss_appl_ospf6_db_detail_external_entry_t> db_entries = {};
    (void)vtss_appl_ospf6_db_detail_external_get_all(db_entries);

    for (auto &itr : db_entries) {
        handler_type handler_out = response.resultHandler();

        {
            // Serialize the entry keys
            auto &&key_handler = handler_out.keyHandler();

            serialize(key_handler, ospf6_key_instance_id(itr.inst_id));
            serialize(key_handler, ospf6_key_area_id(itr.area_id));
            serialize(key_handler, ospf6_key_db_lsdb_type(itr.lsdb_type));
            serialize(key_handler, ospf6_key_db_link_state_id(itr.link_state_id));
            serialize(key_handler, ospf6_key_db_adv_router_id(itr.adv_router_id));
            if (!key_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail external entry keys";
                return VTSS_RC_ERROR;
            }
        }

        {
            // Serialize the entry values
            auto &&val_handler = handler_out.valHandler();
            serialize(val_handler, itr.data);

            if (!val_handler.ok()) {
                VTSS_TRACE(DEBUG) << "Serialize Handler error: OSPF6 database "
                                  "detail external entry values";
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
/** Register module JSON resources                                            */
/******************************************************************************/
namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_ospf6("ospf6");
extern "C" void frr_ospf6_json_init()
{
    json_node_add(&ns_ospf6);
}

/******************************************************************************/
/** Module JSON nodes                                                         */
/******************************************************************************/
/* Hierarchical overview
 *
 *  ospf6
 *      .capabilities
 *      .config
 *          .process
 *          .router
 *          .router_interface
 *          .area
 *          .area_auth
 *          .area_range
 *          .vlink
 *          .vlink_md_key
 *          .stub_area
 *          .interface
 *          .interface_md_key
 *      .status
 *          .router
 *          .area
 *          .interface
 *          .neighbor
 *              .ipv4
 *          .interface_md_key_precedence
 *          .vlink_md_key_precedence
 *          .route
 *              .ipv4
 *          .db
 *          .db_detail
 *              .router
 *              .network
 *              .summary
 *              .inter_area_router
 *              .external
 *              .nssa_external
 *      .control
 *          .global
 */

namespace vtss
{
namespace appl
{
namespace frr
{
namespace interfaces
{

// ospf6.config
NS(ns_conf, ns_ospf6, "config");

// ospf6.status
NS(ns_status, ns_ospf6, "status");

// ospf6.control
NS(ns_control, ns_ospf6, "control");

// ospf6.status.route
NS(ns_route, ns_status, "route");

// ospf6.status.neighbor
NS(ns_status_nb, ns_status, "neighbor");

// ospf6.status.db_detail
NS(ns_db_detail, ns_status, "db_detail");

// ospf6.capabilities
static StructReadOnly<Ospf6CapabilitiesTabular> ospf6_capabilities_tabular(
    &ns_ospf6, "capabilities");

// ospf6.config.process
static TableReadWriteAddDelete<Ospf6ConfigProcessEntry> ospf6_config_process_table(
    &ns_conf, "process");

// ospf6.config.router
static TableReadWrite<Ospf6ConfigRouterEntry> ospf6_config_router_table(&ns_conf,
                                                                        "router");
// ospf6.config.router_interface
static TableReadWrite<Ospf6ConfigRouterInterfaceEntry>
ospf6_config_router_interface_table(&ns_conf, "router_interface");

// ospf6.config.area_range
static TableReadWriteAddDelete<Ospf6ConfigAreaRangeEntry>
ospf6_config_area_range_table(&ns_conf, "area_range");

// ospf6.config.stub_area
static TableReadWriteAddDelete<Ospf6ConfigStubAreaEntry> ospf6_config_stub_area_table(
    &ns_conf, "stub_area");

// ospf6.config.interface
static TableReadWrite<Ospf6ConfigInterfaceEntry> ospf6_config_interface_table(
    &ns_conf, "interface");

// ospf6.status.router
static TableReadOnly<Ospf6StatusRouterEntry> ospf6_status_router_table(&ns_status,
                                                                       "router");
// ospf6.status.area
static TableReadOnly<Ospf6StatusAreaEntry> ospf6_status_area_table(&ns_status,
                                                                   "area");

// ospf6.status.interface
static TableReadOnly<Ospf6StatusInterfaceEntry> ospf6_status_interface_table(
    &ns_status, "interface");

// ospf6.status.neighbor.ipv6
static TableReadOnly<Ospf6StatusNeighborIpv6Entry> ospf6_status_neighbor_table(
    &ns_status_nb, "ipv6");

// ospf6.status.route.ipv6
static TableReadOnly<Ospf6StatusRouteIpv6Entry> ospf6_status_route_ipv6_table(
    &ns_route, "ipv6");

// ospf6.status.db
static TableReadOnly<Ospf6StatusDbEntry> ospf6_status_db_table(&ns_status, "db");

// ospf6.status.db_detail.link
static TableReadOnly<Ospf6StatusDbLinkEntry> ospf6_status_db_link_table(
    &ns_db_detail, "link");

// ospf6.status.db_detail.router
static TableReadOnly<Ospf6StatusDbRouterEntry> ospf6_status_db_router_table(
    &ns_db_detail, "router");

// ospf6.status.db_detail.network
static TableReadOnly<Ospf6StatusDbNetworkEntry> ospf6_status_db_network_table(
    &ns_db_detail, "network");

// ospf6.status.db_detail.summary
static TableReadOnly<Ospf6StatusDbInterAreaPrefixEntry> ospf6_status_db_inter_area_prefix_table(
    &ns_db_detail, "summary");

// ospf6.status.db_detail.inter_area_router
static TableReadOnly<Ospf6StatusDbAsbrSummaryEntry>
ospf6_status_db_inter_area_router_table(&ns_db_detail, "inter_area_router");

// ospf6.status.db_detail.external
static TableReadOnly<Ospf6StatusDbExternalEntry> ospf6_status_db_external_table(
    &ns_db_detail, "external");

// ospf6.status.db_detail.intraAreaPrefix
static TableReadOnly<Ospf6StatusDbIntraAreaPrefixEntry> ospf6_status_db_intra_area_prefix_table(
    &ns_db_detail, "intraAreaPrefix");

// ospf6.control.globals
static StructWriteOnly<Ospf6ControlGlobalsTabular> ospf6_control_globals_tabular(
    &ns_control, "globals");

}  // namespace interfaces
}  // namespace frr
}  // namespace appl
}  // namespace vtss

