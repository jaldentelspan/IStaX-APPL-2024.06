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

#include <vtss/basics/trace_basics.hxx>
#include <vtss/basics/expose/snmp/oid_inventory.hxx>

namespace vtss {
namespace expose {
namespace snmp {

void OidInventory::clear() {
    data.clear();
}

void OidInventory::walk(Walkable &o) {
    uint32_t start = 0;

    OidSequence v;
    Indent indent;

    while (start < data.size())
        start += walk_(o, start, v, indent);
}

uint32_t OidInventory::walk_(Walkable &o, uint32_t idx,
                             OidSequence &v, Indent in) {
    uint32_t cnt = data[idx].cnt;
    uint32_t cnt_old = cnt;

    v.push(data[idx].val);

    if (!cnt) { // this marks a leaf
        o.leaf(v);
        v.pop();
        return 1;
    }

    idx ++;
    while (cnt) {
        uint32_t res = walk_(o, idx, v, in);
        idx += res;
        cnt -= res;
    }

    v.pop();
    return cnt_old + 1;
}

bool OidInventory::insert(const OidSequence &v) {
    return insert_(0, v, 0) != 0;
}

uint32_t OidInventory::insert_(uint32_t data_idx,
                               const OidSequence &v, uint32_t v_i) {
    // We must detect when we go from a leaf to a non-leaf
    bool last_was_leaf = false;

    while (data_idx < data.size() && v_i < v.valid) {
        // If we reach the end and still have not a place to insert, then we
        // must insert at the end
        if (data_idx >= data.size())
            break;

        // Test if the input value match the tree value. If so, then we have
        // found a common prefix. The common prefix is taking advantage of by
        // executing the recursive call.
        if (v.oids[v_i] == data[data_idx].val) { // match

            // A leaf element may not be reused
            if (data[data_idx].cnt == 0)
                return 0;  // return zero signals error


            // Do recursive call
            uint32_t c = insert_(data_idx + 1, v, v_i + 1);

            // The usage count must be incremented with the return value.
            data[data_idx].cnt += c;
            return c;
        }

        // Detect when we are walking backwards in the tree. If this happens we
        // must be a the end of the leafs which is where this must be inserted
        if (data[data_idx].cnt != 0 && last_was_leaf)
            break;
        last_was_leaf = (data[data_idx].cnt == 0);

        // We have no common prefix or the common prefix has started to
        // divigate. We must now insert sorted, this means skipping until will
        // find a value which is grater than the input
        if (v.oids[v_i] > data[data_idx].val) { // branch is less-than, skip it
            if (data[data_idx].cnt)
                data_idx += data[data_idx].cnt + 1;
            else
                data_idx += 1;

        } else {
            // data_idx now points to where the remaining part of the index must
            // be inserted.
            break;
        }
    }

    // Insert the date after "data_idx"
    insert_raw(data.begin() + data_idx, v, v_i);

    // The number or elements inserted.
    return v.valid - v_i;
}

bool OidInventory::insert_raw(Vector<Pair>::iterator i,
                              const OidSequence &v, uint32_t offset) {
    OidInventory::Pair dummy(0, 0);
    data.insert(i, v.valid - offset, dummy);

    for (; offset < v.valid; offset++, i++)
        *i = OidInventory::Pair((v.valid - offset - 1), v.oids[offset]);

    return true;
}

bool OidInventory::find_split(const OidSequence &in,
                              OidSequence &out_oid,
                              OidSequence &out_key) {
    uint32_t data_idx = 0, v_i = 0;
    out_oid.clear();
    out_key.clear();

    while (data_idx < data.size() && v_i < in.valid) {
        // Test if current oid matches
        if (data[data_idx].val != in.oids[v_i]) {
            uint32_t c = data[data_idx].cnt;  // No match, skip over the branch
            if (c == 0)  // Skipping over a leaf
                data_idx += 1;
            else         // skipping over a branck in the tree
                data_idx += c + 1;

            continue;
        }

        // Test if the current match is a leaf
        if (data[data_idx].cnt == 0) {
            // This is the stop criteria. We can now split the input sequence
            // into a oid part and a key part
            out_oid.copy_from(in.oids, v_i + 1);
            out_key.copy_from(&(in.oids[v_i + 1]), in.valid - (v_i + 1));

            VTSS_BASICS_TRACE(RACKET) << " in: " << in << " out: " << out_oid <<
                " key: " << out_key;
            return true;
        }

        // Continue on the current path
        data_idx++, v_i++;
    }

    // No such leaf object found in the inventory
    return false;
}

bool OidInventory::find_next(const OidSequence &in, OidSequence &out_oid) {
    Indent indent;
    Path::E path = Path::EXACT;
    out_oid.clear();

    uint32_t start = 0;
    while (start < data.size() && path != Path::DONE)
        start += find_next_(in, out_oid, start, 0, path, indent);

    return path == Path::DONE;
}

uint32_t OidInventory::find_next_(const OidSequence &in, OidSequence &out,
                                  uint32_t data_i, uint32_t in_i,
                                  Path::E &path, Indent indent) {
    uint32_t cnt;
    uint32_t cnt_return = 0;
    bool exact_leaf_match = false;

    VTSS_BASICS_TRACE(RACKET) << indent << path << " in: " << in << " out: " <<
        out << " data_i: " << data_i << " in_i: " << in_i;

    // Make sure we do not perform look-ups in an empty inventory
    if (path == Path::END_OF_MIB || data_i >= data.size()) {
        path = Path::END_OF_MIB;
        return 0;
    }

    // Fast forward at currnet level until we find a suitable branch
    if (in_i < in.valid && path == Path::EXACT) {
        while (data_i < data.size() && in.oids[in_i] > data[data_i].val) {
            // Every time we skip a branch, we must signal the extra consumed
            // oids through the return value
            cnt_return += data[data_i].cnt + 1;

            // The cnt field is the number of elements infront of the current
            // element. We skip all in front, and this element.
            data_i += data[data_i].cnt + 1;
        }

    } else {
        // If the input sequence has ended (or is empty) we are automatic on the
        // next branch
        path = Path::NEXT;
    }

    // The original cnt value must be used as return value. The count must be
    // latched after the "fast-forward" step.
    cnt = data[data_i].cnt;
    cnt_return += cnt;
    exact_leaf_match = false;

    // Check that we did not go across the boundary
    if (data_i >= data.size()) {
        path = Path::END_OF_MIB;
        return 0;
    }

    // Keep track on if we found a OID element which is greater than the
    // requested.
    if (path == Path::EXACT && data[data_i].val > in.oids[in_i]) {
        VTSS_BASICS_TRACE(RACKET) << indent << "EXACT -> NEXT";
        path = Path::NEXT;
    }

    // Capture the current sequence as we might need it.
    out.push(data[data_i].val);
    VTSS_BASICS_TRACE(RACKET) << indent << "Push> data_i: " << data_i <<
            " val: " << data[data_i].val << " out: " << out <<
            " cnt: " << cnt << " cnt_return: " << cnt_return;

    if (cnt == 0) { // This marks a leaf.
        // If this evaluates to true then we have found a leaf which follow the
        // requested. This means that we are done.
        if (path == Path::NEXT) {
            path = Path::DONE;
            return 0;
        }

        // If this evaluates to true, we have found a leaf matching a either the
        // complete sequence of "in" or a prefix of "in". This means that we
        // must be returning the first oid which follows "in".
        if (path == Path::EXACT) {
            exact_leaf_match = true;
            VTSS_BASICS_TRACE(RACKET) << indent <<
                "Got a exact match, grap the next leaf. " << out;

            // We have pushed an element which we could not use any way, this
            // must be cleaned up.
            out.pop();

            // Make sure we return the nest leaf which is found.
            path = Path::NEXT;
        }
    }

    // Nothing found yet, continue the search
    data_i++;
    in_i++;

    // Iterate through all childs at this level, unless we finish before
    while (cnt && path != Path::DONE && path != Path::END_OF_MIB) {
        VTSS_BASICS_TRACE(RACKET) << indent << "Before data_i: " << data_i <<
            " cnt:" << cnt << " path:" << path;
        uint32_t res = find_next_(in, out, data_i, in_i, path, indent);
        data_i += res;
        cnt    -= res;
        VTSS_BASICS_TRACE(RACKET) << indent << "After data_i: " << data_i <<
            " res:" << res << " cnt:" << cnt << " path:" << path;
    }

    // We must keep the history if we are done
    if (path != Path::DONE && !exact_leaf_match) {
        out.pop();
        VTSS_BASICS_TRACE(RACKET) << indent <<
            "This run had nothing to offer, pop: " << out;
    }

    VTSS_BASICS_TRACE(RACKET) << indent << "Return: " << cnt_return + 1;
    return cnt_return + 1;
}

ostream& operator<<(ostream &o, OidInventory::Path::E i) {
    switch (i) {
        case OidInventory::Path::EXACT:
            o << "EXACT"; break;
        case OidInventory::Path::NEXT:
            o << "NEXT"; break;
        case OidInventory::Path::DONE:
            o << "DONE"; break;
        case OidInventory::Path::END_OF_MIB:
            o << "END_OF_MIB"; break;
        default:
            o << "<Unknown:" << (int)i << ">"; break;
    }
    return o;
}

ostream& operator<<(ostream &o, OidInventory::Indent i) {
    for (uint32_t j = 0; j < i.i; j++) o << " ";
    return o;
}

ostream& operator<<(ostream &o, const OidInventory::Pair &v) {
    o << "(" << v.cnt << ", " << v.val << ")";
    return o;
}

ostream& operator<<(ostream &o, const OidInventory &i) {
    uint32_t c = 0;
    o << "\n";
    for (const auto &a : i.data) {
        o << c++ << " " << a << '\n';
    }
    return o;
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
