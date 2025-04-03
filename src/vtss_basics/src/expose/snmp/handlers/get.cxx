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

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/expose/snmp/handlers/get.hxx"

namespace vtss {
namespace expose {
namespace snmp {

void serialize_get_struct_ro_generic(GetHandler &h, StructBase &o) {
    VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, NOISE) << __PRETTY_FUNCTION__;

    // Check that the OID represented by this entity matches the query.
    if (!h.consume_oid(o.element().numeric())) return;

    if (h.getnext) {
        // Do never allow to iterate on a scalar
        if (h.oid_seq_index.valid > 0) {
            h.state(HandlerState::AGAIN);
            return;
        }

        // A normal get as iteration on a get is not allowed
        h.next_index.push(0);
    } else {
        // We assume that h.oid_seq_index is "{0}"
        h.next_index = h.oid_seq_index;
    }

    // Create a new iterator which holds the values
    auto iter = o.new_iterator();

    // Ask the iterator to call the get function.
    if (!iter || iter->get(h.next_index) != MESA_RC_OK) {
        if (h.getnext)
            h.state(HandlerState::AGAIN);
        else
            h.error_code(ErrorCode::noSuchName, __FILE__, __LINE__);
        return;
    }

    iter->call_serialize(h);
}

void serialize_get_table_ro_generic(GetHandler &g, StructBase &o) {
    // Early break if the object is already found
    if (g.state() != HandlerState::SEARCHING) return;

    // consume the current oid
    if (!g.consume_oid(o.element().numeric())) return;

    // Every table has a hard-coded "1" node to contain the table.
    if (!g.consume_oid(1)) return;

    // Fetch the values
    auto i = o.new_iterator();

    // Prepare the first iteration
    if (g.getnext) {
        g.next_index.clear();
        if (!i || i->get_next(g.oid_seq_index, g.next_index) != MESA_RC_OK) {
            g.state(HandlerState::AGAIN);
            return;
        }

    } else {
        g.next_index = g.oid_seq_index;
        if (!i || i->get(g.oid_seq_index) != MESA_RC_OK) {
            g.error_code(ErrorCode::noSuchName, __FILE__, __LINE__);
            return;
        }
    }

    // Out of memory? (or maybe other cases???)
    if (!i.get()) return;

    if (!g.getnext) {  // implement the get-handler path
        i->call_serialize_values(g);
        return;
    }

    // get-next only from here ////////////////////////////////////////////////

    // In a sparsely populated table we must iterate several times. Each
    // iteration must use the same query, and to restore this query the
    // g.oid_index_ is used.
    uint32_t oid_index_old = g.oid_index_;

    while (true) {
        // Serialize the values which has been prepared in the iterator.
        i->call_serialize_values(g);

        // The handler must explicitly signal if it wants an other iteration!
        if (g.state() != HandlerState::AGAIN) break;

        // call the "itr" function followed by "get", until both succedes or the
        // "itr" function fails (which means end-of-table).
        if (i->get_next(g.next_index, g.next_index) != MESA_RC_OK) break;

        // Restore the seraching state (was in AGAIN state).
        g.state(HandlerState::SEARCHING);

        // Reverting the index as we are re-quering the same object (but with
        // new index)!
        g.oid_index_ = oid_index_old;
    }
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
