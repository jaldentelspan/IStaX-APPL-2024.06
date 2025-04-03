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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_STRUCT_BASE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_STRUCT_BASE_HXX__

#include <vtss/basics/trace_grps.hxx>
#include <vtss/basics/trace_basics.hxx>

#include "vtss/basics/assert.hxx"
#include "vtss/basics/memory.hxx"
#include "vtss/basics/expose/snmp/node.hxx"
#include "vtss/basics/expose/snmp/namespace-node.hxx"
#include "vtss/basics/expose/snmp/iterator-common.hxx"
#include "vtss/basics/expose/snmp/list-of-handlers.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(113, VTSS_BASICS_TRACE_GRP_SNMP, X)

namespace vtss {
namespace expose {
namespace snmp {

struct StructBase : public Node {
    friend struct Node;

    StructBase(NamespaceNode *p, const OidElement &e) : Node(Node::Leaf, e) {
        if (p) p->attach(*this);
    }

    StructBase(NamespaceNode *p, const OidElement &e, bool trap)
        : Node(Node::Leaf, e), is_trap(trap) {
        if (p) p->attach(*this);
    }

    // No copying
    StructBase(const StructBase &) = delete;
    StructBase &operator=(const StructBase &) = delete;

#define X(H) virtual void do_serialize(H &a) = 0;
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X

    virtual ~StructBase() { unlink(); }

    virtual unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<IteratorCommon>(nullptr);
    }

    const bool is_trap = false;
    static bool classof(const Node *n) { return n->get_kind() == Leaf; }
};

template <typename HANDLER>
void serialize(HANDLER &h, StructBase &v) {
    // Use the virtual functions to call serialize on the base class of
    // StructBase
    h.recursive_call++;

    if (h.recursive_call > 10) {
        TRACE(ERROR)
                << __PRETTY_FUNCTION__
                << " Too many recursive calls... Most likely due to a missing "
                << "speciliazed serializer";
        VTSS_ASSERT(0);
    }

    v.do_serialize(h);
    h.recursive_call--;
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#undef TRACE
#endif  // __VTSS_BASICS_EXPOSE_SNMP_STRUCT_BASE_HXX__
