/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_NAMESPACE_NODE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_NAMESPACE_NODE_HXX__

#include "vtss/basics/expose/snmp/node.hxx"

namespace vtss {
namespace expose {
namespace snmp {

struct StructBase;

// TODO, apply reference counting to allow implicit destruction
struct NamespaceNode : public Node {
    typedef intrusive::List<StructBase>::const_iterator leaf_const_iterator;
    typedef intrusive::List<StructBase>::iterator leaf_iterator;

    typedef intrusive::List<NamespaceNode>::const_iterator node_const_iterator;
    typedef intrusive::List<NamespaceNode>::iterator node_iterator;

    explicit NamespaceNode(const OidElement &e) : Node(Node::Namespace, e) {}

    NamespaceNode(const char *n, uint32_t o)
        : Node(Node::Namespace, OidElement(o, n)) {}

    NamespaceNode(NamespaceNode *p, const OidElement &e)
        : Node(Node::Namespace, e) {
        p->attach(*this);
    }

    NamespaceNode(NamespaceNode *p, const char *n, uint32_t o)
        : Node(Node::Namespace, OidElement(o, n)) {
        p->attach(*this);
    }

    NamespaceNode(const NamespaceNode &) = delete;
    NamespaceNode &operator=(const NamespaceNode &) = delete;

    // TODO, thread safety!
    ~NamespaceNode() { unlink(); }

    // TODO, check for collaps
    void attach(Node &l) {
        l.parent_ = this;
        leafs.insert_sorted(l);
    }

    virtual void init() override {
        for (auto &i : leafs) {
            i.init();
        }
    }

    static bool classof(const Node *n) { return n->get_kind() == Namespace; }

    intrusive::List<Node> leafs;
};

void print_numeric_oid_seq(ostream *os, const NamespaceNode *o);

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_NAMESPACE_NODE_HXX__
