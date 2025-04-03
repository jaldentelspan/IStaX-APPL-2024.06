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

#include <string>
#include "vtss/basics/expose/json/node.hxx"
#include "vtss/basics/expose/json/namespace-node.hxx"

namespace vtss {
namespace expose {
namespace json {

Node::Node(NamespaceNode *p, const char *n, const char *d)
    : name_(n), description_(d) {
    attach_to(p);
}

void Node::attach_to(NamespaceNode *p) {
    unlink();
    p->attach__(this);
    parent_ = p;
}

std::string Node::abs_name(const std::string &n) const {
    auto s = abs_name();
    s.push_back('.');
    s.append(n);
    return s;
}

std::string Node::abs_name() const {
    Vector<const Node *> stack;
    std::string s;

    const Node *p = this;
    while (p) {
        if (str("ROOT") == p->name()) break;

        stack.push_back(p);
        p = p->parent();
    }

    bool first = true;
    for (auto i = stack.rbegin(); i != stack.rend(); ++i) {
        if (first)
            first = false;
        else
            s.push_back('.');

        s.append((*i)->name().begin());
    }

    return s;
}

Node *Node::lookup(str method_name)
{
    return this;
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

