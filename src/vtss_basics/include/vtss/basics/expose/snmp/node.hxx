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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_NODE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_NODE_HXX__

#include "vtss/basics/api_types.h"
#include "vtss/basics/intrusive_list.hxx"
#include "vtss/basics/expose/snmp/oid-element.hxx"

namespace vtss {
namespace expose {
namespace snmp {

struct NamespaceNode;

struct Node : public intrusive::ListNode {
    friend struct NamespaceNode;
    enum NodeKind { Leaf, Namespace };
    NodeKind get_kind() const { return kind; }

    Node(NodeKind k, const OidElement &e) : parent_(0), kind(k), element_(e) {}

    NamespaceNode *parent() { return parent_; }
    const NamespaceNode *parent() const { return parent_; }
    const OidElement &element() const { return element_; }
    bool operator<(const Node &x) const { return element_ < x.element_; }

    virtual void init() {}

  protected:
    NamespaceNode *parent_;

  private:
    const NodeKind kind;
    const OidElement element_;
};

// A node can be serialized by "any" handler, the implementation of this depends
// on namespace-node and struct-base. Implemented in node-impl.hxx
template <typename HANDLER>
void serialize(HANDLER &h, Node &o);

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_NODE_HXX__
