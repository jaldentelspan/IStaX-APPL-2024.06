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

#include <vtss/basics/trace_grps.hxx>
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON

#include <vtss/basics/trace_basics.hxx>
#include "vtss/basics/expose/json/exporter.hxx"
#include "vtss/basics/expose/json/method-split.hxx"
#include "vtss/basics/expose/json/namespace-node.hxx"

namespace vtss {
namespace expose {
namespace json {

vtss::json::Result::Code NamespaceNode::handle(int priv, str method_name,
                                               const Request *req,
                                               ostreamBuf &out) {
    VTSS_BASICS_TRACE(DEBUG) << "Handling " << method_name;
    str method_head, method_tail;
    Request::id_t id(req->id);
    vtss::json::Result::Code error_code = vtss::json::Result::METHOD_NOT_FOUND;

    // This is not a leafs, so the methind name must not be empty
    if (method_name.begin() == method_name.end()) {
        VTSS_BASICS_TRACE(NOISE) << "This is not a leaf!";
        goto WriteResponseMsg;
    }

    if (!implemented()) {
        VTSS_BASICS_TRACE(INFO) << "Leaf is not implemented";
        goto WriteResponseMsg;
    }

    // consume until first delimitor, and forward the remaining
    method_split(method_name, method_head, method_tail);
    VTSS_BASICS_TRACE(NOISE) << "Method name: " << method_name
                             << " head: " << method_head
                             << " tail: " << method_tail
                             << "priv: " << priv;

    typedef intrusive::List<Node>::iterator I;
    for (I i = leafs.begin(); i != leafs.end(); ++i) {
        if (method_head == i->name()) {
            VTSS_BASICS_TRACE(DEBUG) << "Match: " << method_head;
            if (i->implemented()) {
                return i->handle(priv, method_tail, req, out);
            } else {
                VTSS_BASICS_TRACE(DEBUG) << "Not implemented";
                break;
            }

        } else {
            VTSS_BASICS_TRACE(NOISE) << "No match: " << i->name();
        }
    }

WriteResponseMsg:
    VTSS_BASICS_TRACE(DEBUG) << "No such element found matching: "
                             << method_name;

    // Compose a response message
    Exporter e(&out);
    {
        Exporter::Map m = e.as_map();
        m.add_leaf(id, vtss::tag::Name("id"));
        m.add_leaf(lit_null, vtss::tag::Name("result"));
        Exporter::Map em = m.as_map(vtss::tag::Name("error"));
        em.add_leaf(error_code, vtss::tag::Name("code"));
        str msg("No such method");
        em.add_leaf(msg, vtss::tag::Name("message"));
        Exporter::Map dm = em.as_map(vtss::tag::Name("data"));
        dm.add_leaf(method_name,
                    vtss::tag::Name("vtss-non-matched-method-part"));
    }

    return error_code;
}

Node *NamespaceNode::lookup(str method_name)
{
    VTSS_BASICS_TRACE(DEBUG) << "Handling " << method_name;
    str method_head, method_tail;

    // consume until first delimitor, and forward the remaining
    method_split(method_name, method_head, method_tail);

    VTSS_BASICS_TRACE(NOISE) << "Method name: " << method_name
                             << " head: " << method_head
                             << " tail: " << method_tail;

    typedef intrusive::List<Node>::iterator I;
    for (I i = leafs.begin(); i != leafs.end(); ++i) {
        if (method_head == i->name()) {
            if (method_tail.begin() == method_tail.end()) {
                return &*i;
            }
            VTSS_BASICS_TRACE(DEBUG) << "Match: " << method_head;
            if (!i->implemented()) {
                VTSS_BASICS_TRACE(DEBUG) << "Not implemented";
                return nullptr;
            }
            return i->lookup(method_tail);
        }
    }
    return nullptr;
}

void NamespaceNode::attach__(Node *leaf) {

    leafs.push_back(*leaf);
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

