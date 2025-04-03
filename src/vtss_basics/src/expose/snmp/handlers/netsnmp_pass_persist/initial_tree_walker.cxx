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

#include "vtss/basics/expose/snmp/handlers/netsnmp_pass_persist/initial_tree_walker.hxx"

namespace vtss {
namespace expose {
namespace snmp {

void InitialTreeWalker::add_leaf_(int oid) {
    oid_seq_.push(oid);
    VTSS_BASICS_TRACE(INFO) << "Adding oid: " << oid_seq_;
    inventory.insert(oid_seq_);
    oid_seq_.pop();
}

ostream& operator<<(ostream &o, const InitialTreeWalker &v) {
    return o;
}

void serialize(InitialTreeWalker &c, NamespaceNode &o) {
    c.oid_seq_.push(o.element().numeric_);

    for (auto i = o.leafs.begin(); i != o.leafs.end(); ++i)
        serialize(c, *i);

    c.oid_seq_.pop();
}


}  // namespace snmp
}  // namespace expose
}  // namespace vtss
