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

#ifndef __JSON_RPC_JSON_RPC_SERIALIZE_HXX__
#define __JSON_RPC_JSON_RPC_SERIALIZE_HXX__

#include <string>
#include "vtss/basics/vector.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/basics/expose/json.hxx"
#include <vtss/basics/expose/json/specification/inventory.hxx>
#include "vtss_appl_expose_json_array.hxx"

namespace vtss {
namespace appl {
namespace json_rpc {

mesa_rc inventory_generic_get(const std::string *,
                              expose::json::specification::Inventory *const);

mesa_rc inventory_specific_get(const std::string *,
                               expose::json::specification::Inventory *const);

mesa_rc nodes_generic_get(const std::string *, Vector<std::string> *nodes);

void serialize(vtss::expose::json::Exporter &e,
               expose::json::specification::Inventory &i) {
    e.os() << i;
}

void serialize(vtss::expose::json::HandlerReflector &r,
               expose::json::specification::Inventory &i) {
    r.type_terminal(vtss::expose::json::JsonCoreType::Object,
                    "vtss_appl_json_rpc_inventory_t", "See VTSS-JSON-SPEC(7)");
}

void serialize_node_list(vtss::expose::json::Exporter &e, Vector<std::string> &n) {
    ArrayExporter<std::string> a(e);
    for (const auto &i : n)
        a.add(i);
}

void serialize_node_list(vtss::expose::json::HandlerReflector &r, Vector<std::string> &n) {
    r.type_terminal(vtss::expose::json::JsonCoreType::Array,
                    "std::vector<std::string>", "List of strings");
}

namespace interfaces {

struct InventoryGeneric {
    typedef expose::ParamList<
            expose::ParamKey<std::string *>,
            expose::ParamVal<expose::json::specification::Inventory>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(std::string &i) {
        h.argument_properties(tag::Name("query"),
                              tag::Description(
                                      "The query may be used to specify from "
                                      "what point in the JSON-RPC command tree "
                                      "the specification should be generated"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(expose::json::specification::Inventory &i) {
        h.argument_properties(tag::Name("spec"),
                              tag::Description(
                                      "The retuned structure is the "
                                      "inventory generated from the "
                                      "query."));
        serialize(h, i);
    }

    static constexpr const char *desc_get_func =
            "Generate a complete or a partial generic-specific specification "
            "of the JSON-RPC interface. The query string can be used to either "
            "generate a complete specification or a specification of a subset "
            "of the command tree.";

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_JSON_RPC);
    VTSS_EXPOSE_GET_PTR(inventory_generic_get);
};

struct InventorySpecific {
    typedef expose::ParamList<
            expose::ParamKey<std::string *>,
            expose::ParamVal<expose::json::specification::Inventory>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(std::string &i) {
        h.argument_properties(tag::Name("query"),
                              tag::Description(
                                      "The query may be used to specify from "
                                      "what point in the JSON-RPC command tree "
                                      "the specification should be generated"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(expose::json::specification::Inventory &i) {
        h.argument_properties(tag::Name("spec"),
                              tag::Description(
                                      "The retuned structure is the "
                                      "inventory generated from the "
                                      "query."));
        serialize(h, i);
    }

    static constexpr const char *desc_get_func =
            "Generate a complete or a partial target-specific specification of "
            "the JSON-RPC interface. The query string can be used to either "
            "generate a complete specification or a specification of a subset "
            "of the command tree.";

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_JSON_RPC);
    VTSS_EXPOSE_GET_PTR(inventory_specific_get);
};

struct NodesGeneric {
    typedef expose::ParamList<expose::ParamKey<std::string *>,
                              expose::ParamVal<Vector<std::string>>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(std::string &i) {
        h.argument_properties(tag::Name("query"),
                              tag::Description(
                                      "The query may be used to specify from "
                                      "what point in the JSON-RPC command tree "
                                      "the specification should be generated"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(Vector<std::string> &i) {
        h.argument_properties(
                tag::Name("node_list"),
                tag::Description("List of nodes (namespaces or methods)"));

        serialize_node_list(h, i);
    }

    static constexpr const char *desc_get_func =
            "Get a list of sub-nodes (namespace or methods) found relative to "
            "the query.";

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_JSON_RPC);
    VTSS_EXPOSE_GET_PTR(nodes_generic_get);
};

}  // namespace interfaces
}  // namespace json_rpc
}  // namespace appl
}  // namespace vtss

#endif  // __JSON_RPC_JSON_RPC_SERIALIZE_HXX__
