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

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "vtss/basics/assert.hxx"
#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/snmp/handlers.hxx"


namespace vtss {
namespace expose {
namespace snmp {

template <typename T>
void serialize_get_set(T &g, NamespaceNode &o) {
    if (g.state() != HandlerState::SEARCHING) return;

    if (!g.consume_oid(o.element().numeric_)) return;

    VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, DEBUG)
            << ">ns-node: " << o.element() << " " << g.state();

    for (auto i = o.leafs.begin(); i != o.leafs.end(); ++i) {
        switch (i->get_kind()) {
        case Node::Leaf:
        case Node::Namespace:
            VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, NOISE)
                    << ">visit:   " << i->element() << " " << g.state();
            serialize(g, *i);
            VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, RACKET)
                    << "<visit:   " << i->element() << " " << g.state();
            break;

        default:
            VTSS_ASSERT(0);
        }

        if (g.state() != HandlerState::SEARCHING) {
            VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, DEBUG)
                    << "Leaving: " << o.element() << " " << g.state();
            break;
        }
    }

    VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, RACKET)
            << "<ns-node: " << o.element() << " " << g.state();
}

void serialize(GetHandler &g, NamespaceNode &o) { serialize_get_set(g, o); }

void serialize(SetHandler &g, NamespaceNode &o) { serialize_get_set(g, o); }

void serialize(SetHandler &a, AsInt &s) {
    int32_t x = 0;
    serialize(a, x);

    if (a.error_code() != ErrorCode::noError) {
        VTSS_BASICS_TRACE(INFO) << "Failed to serialize as int: "
                << a.error_code();
        return;
    }

    if (x < s.offset) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << x;
        a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    s.set_value(x);
}

void serialize(GetHandler &a, AsInt &s) {
    uint32_t x = s.get_value();
    int32_t y;

    if (x > 0x7fffffff) {
        VTSS_BASICS_TRACE(ERROR) << "Value too big: " << x;
        x = 0x7fffffff;
    }

    // cast to integer
    y = (int32_t)x;
    serialize(a, y);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
