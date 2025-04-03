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

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON
#include <vtss/basics/vector.hxx>
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/expose/json/namespace-node.hxx"
#include "vtss/basics/expose/json/specification/walk.hxx"
#include "vtss/basics/expose/json/specification/reflector-echo.hxx"

namespace vtss {
namespace expose {
namespace json {
namespace specification {

void walk_(Inventory &inv, Node *node, Vector<str> &stack,
           Vector<std::string> &g, bool target_specific);

void walk__(Inventory &inv, Node *node, Vector<str> &stack,
            Vector<std::string> &g, bool target_specific) {
    std::string s, p;

    for (auto n : stack) {
        if (s.size() != 0) s.append(".");
        s.append(n.begin(), n.size());
    }

    if (target_specific && !node->implemented()) {
        VTSS_BASICS_TRACE(INFO)
                << "Skipping target " << s.c_str()
                << " as it is not implemented on current target";
        return;
    }

    if (node->is_function_exporter()) {
        return;
    }
    
    VTSS_BASICS_TRACE(INFO) << "node name: " << s.c_str();
    if (g.size()) p = g.back();
    ReflectorEcho r(inv, s, p, node, target_specific);
    node->handle_reflection(&r);
}

void walk__(Inventory &inv, NamespaceNode *ns, Vector<str> &stack,
            Vector<std::string> &g, bool target_specific) {
    typedef intrusive::List<Node>::iterator I;

    if (ns->description().size()) {
        std::string s, p;
        for (auto n : stack) {
            if (s.size() != 0) s.append(".");
            s.append(n.begin(), n.size());
        }

        if (inv.groups.find(s) == inv.groups.end()) {
            if (g.size()) p = g.back();

            VTSS_BASICS_TRACE(INFO) << "Description: " << p << " " << s << " "
                                    << ns->description();

            GroupDescriptor gg;
            gg.parent = p;
            gg.description = ns->description().begin();
            inv.groups.set(s, gg);
        }

        g.push_back(s);
    }

    for (I i = ns->leafs.begin(); i != ns->leafs.end(); ++i)
        walk_(inv, &*i, stack, g, target_specific);

    if (ns->description().size()) g.pop_back();
}

void walk_(Inventory &inv, Node *node, Vector<str> &stack,
           Vector<std::string> &g, bool target_specific) {
    stack.push_back(node->name());

    if (node->is_namespace_node())
        walk__(inv, static_cast<NamespaceNode *>(node), stack, g,
               target_specific);
    else
        walk__(inv, node, stack, g, target_specific);

    stack.pop_back();
}

void walk(Inventory &inv, Node *node, bool target_specific) {
    Vector<str> s;
    Vector<std::string> g;
    walk_(inv, node, s, g, target_specific);
}

}  // namespace specification
}  // namespace json
}  // namespace expose
}  // namespace vtss

