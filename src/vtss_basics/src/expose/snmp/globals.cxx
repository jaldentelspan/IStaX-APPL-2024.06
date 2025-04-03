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

#include "vtss/basics/predefs.hxx"
#include "vtss/basics/trace_grps.hxx"
#include "vtss/basics/expose/snmp/globals.hxx"

#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

namespace vtss {
namespace expose {
namespace snmp {

Globals vtss_snmp_globals;
Globals::Globals()
    : iso(OidElement(1, "iso")),
      org(&iso, OidElement(3, "org")),
      dod(&org, OidElement(6, "dod")),
      internet(&dod, OidElement(1, "internet")),
      priv(&internet, OidElement(4, "private")),
      enterprises(&priv, OidElement(1, "enterprises")),
      vtss_root(&enterprises, OidElement(MIB_ENTERPRISE_OID, TO_STR(MIB_ENTERPRISE_NAME))),
      modules_root(&vtss_root, OidElement(MIB_ENTERPRISE_PRODUCT_ID, "modules")) {}

mesa_rc Globals::attach_module(Node &n) {
#if defined(VTSS_BASICS_STANDALONE)
    OidSequence prefix;
    prefix.push(1), prefix.push(3), prefix.push(6), prefix.push(1),
            prefix.push(4), prefix.push(1), prefix.push(MIB_ENTERPRISE_OID), prefix.push(MIB_ENTERPRISE_PRODUCT_ID);
    InitialTreeWalker w(*this, prefix);
    serialize(w, n);
#endif

    modules_root.attach(n);

    return MESA_RC_OK;
}

#if defined(VTSS_BASICS_STANDALONE)
ErrorCode::E Globals::get(Vector<int> v, ostream *result) {
    OidSequence o;
    o.push(1), o.push(3), o.push(6), o.push(1);
    o.push(4), o.push(1), o.push(MIB_ENTERPRISE_OID), o.push(MIB_ENTERPRISE_PRODUCT_ID);

    for (int i : v) {
        o.push(i);
    }

    return get(o, result);
}

ErrorCode::E Globals::get(const OidSequence &in, ostream *result) {
    OidSequence oid;
    OidSequence key;

    if (!vtss_snmp_globals.find_split(in, oid, key)) {
        VTSS_BASICS_TRACE(DEBUG) << "Get " << in << " -> noSuchName";
        return ErrorCode::noSuchName;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Get in: " << in << " oid: " << oid
                             << " key: " << key;
    GetHandler handler(oid, key, result);
    serialize(handler, vtss_snmp_globals.iso);

    if (handler.state() != HandlerState::DONE)
        handler.error_code(ErrorCode::noSuchName, __FILE__, __LINE__);

    VTSS_BASICS_TRACE(DEBUG) << "Get in: " << in << " oid: " << oid
                             << " key: " << key << " -> " << handler.error_code_;

    return handler.error_code_;
}

ErrorCode::E Globals::get_next(Vector<int> v, OidSequence &next, ostream *result) {
    OidSequence o;
    o.push(1), o.push(3), o.push(6), o.push(1);
    o.push(4), o.push(1), o.push(MIB_ENTERPRISE_OID), o.push(MIB_ENTERPRISE_PRODUCT_ID);

    for (int i : v) {
        o.push(i);
    }

    return get_next(o, next, result);
}

ErrorCode::E Globals::get_next(OidSequence in, OidSequence &next,
                               ostream *result) {
    OidSequence n;
    OidSequence oid;
    OidSequence key;
    HandlerState::E state;
    ErrorCode::E error_code;

    VTSS_BASICS_TRACE(DEBUG) << "In: " << in;

    // Lookup the input OID in the inventory. If it exists use the OID and the
    // key in the search. The actually get-next oprtation will be implemented by
    // the iterator.
    if (!vtss_snmp_globals.find_split(in, oid, key)) {
        VTSS_BASICS_TRACE(DEBUG) << "Exact lookup" << in << " -> noSuchName";

        // The input OID was not found in the inventory. So we have to start by
        // looking up the next OID in the inventory.
        if (!vtss_snmp_globals.find_next(in, oid)) {
            VTSS_BASICS_TRACE(DEBUG) << "Next lookup: " << in
                                     << " -> end-of-mib";
            return ErrorCode::noSuchName;
        }

        // The key is never valid when using the next-oid from the inventory
        key.clear();
    }

    // Keep getting next while the handler returns AGAIN
    do {
        VTSS_BASICS_TRACE(DEBUG) << "Next in: " << in << " oid: " << oid
                                 << " key: " << key;

        GetHandler handler(oid, key, result, true);
        serialize(handler, vtss_snmp_globals.iso);

        state = handler.state();
        error_code = handler.error_code();
        if (state != HandlerState::AGAIN) {
            next = handler.oid_seq_out();
            break;
        }

        // Prepare for next round
        in = oid;
        key.clear();
    } while (vtss_snmp_globals.find_next(in, oid));

    if (state == HandlerState::SEARCHING) error_code = ErrorCode::noSuchName;

    if (state == HandlerState::AGAIN) error_code = ErrorCode::noSuchName;

    if (error_code == ErrorCode::noError) {
        VTSS_BASICS_TRACE(DEBUG) << "Next in: " << in << " oid: " << oid
                                 << " key: " << key << " -> " << next;
    } else {
        VTSS_BASICS_TRACE(DEBUG) << "Next in: " << in << " oid: " << oid
                                 << " key: " << key << " -> " << error_code;
    }

    return error_code;
}

ErrorCode::E Globals::set(const OidSequence &in, str v, ostream *result) {
    OidSequence oid;
    OidSequence key;

    if (!vtss_snmp_globals.find_split(in, oid, key)) {
        VTSS_BASICS_TRACE(DEBUG) << "Set " << in << " = " << v
                                 << " -> noSuchName";
        return ErrorCode::noSuchName;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Set in: " << in << " oid: " << oid
                             << " key: " << key << " val: " << v;
    SetHandler handler(oid, key, v, result);
    serialize(handler, vtss_snmp_globals.iso);

    if (handler.state() == HandlerState::SEARCHING)
        handler.error_code(ErrorCode::noSuchName, __FILE__, __LINE__);

    VTSS_BASICS_TRACE(DEBUG) << "Set in: " << in << " oid: " << oid
                             << " key: " << key << " val: " << v << " -> "
                             << handler.error_code_;

    for (auto &i : handler.cache_list) {
        VTSS_BASICS_TRACE(NOISE) << "Calling set";
        if (i.iterator->set() != MESA_RC_OK) {
            VTSS_BASICS_TRACE(DEBUG) << "Set failed";
            handler.error_code(ErrorCode::commitFailed, __FILE__, __LINE__);
        }
    }

    return handler.error_code_;
}
#endif

static StructBaseTrap *__walk(NamespaceNode *ns, const char *n) {
    VTSS_BASICS_TRACE(DEBUG) << "Walking namespace: " << ns->element();

    for (auto &i : ns->leafs) {
        switch (i.get_kind()) {
        case Node::Leaf: {
            auto s = static_cast<StructBase *>(&i);
            VTSS_BASICS_TRACE(DEBUG) << "Leaf: " << s->element();
            if (s->is_trap) {
                auto t = static_cast<StructBaseTrap *>(&i);
                VTSS_BASICS_TRACE(DEBUG) << "Trap: " << t->element() << " " << n;
                if (strcmp(t->element().name(), n) == 0) return t;
            }

            break;
        }

        case Node::Namespace: {
            auto p = __walk(static_cast<NamespaceNode *>(&i), n);
            if (p != nullptr) return p;
            break;
        }
        }
    }

    return nullptr;
}

static StructBaseTrap *__walk_next(NamespaceNode *ns, const char *n) {
    StructBaseTrap *found = nullptr;

    VTSS_BASICS_TRACE(DEBUG) << "Walking next namespace: " << ns->element();

    for (auto &i : ns->leafs) {
        switch (i.get_kind()) {
        case Node::Leaf: {
            auto s = static_cast<StructBase *>(&i);
            if (s->is_trap) {
                auto t = static_cast<StructBaseTrap *>(&i);
                VTSS_BASICS_TRACE(DEBUG) << "Trap: " << t->element() << " " << n;
                if (n == NULL || !strcmp(n, "")) { // Get first operation
                    return t;
                } else if (strcmp(t->element().name(), n) > 0 &&
                           (found == nullptr /* Found the first one */ ||
                           (found != nullptr && strcmp(t->element().name(), found->element().name()) < 0) /* Found a new one */ )) {
                    VTSS_BASICS_TRACE(DEBUG) << "Trap: Update found " << t->element() << " " << (found == nullptr ? n : found->element().name());
                    found = t;
                }
            }

            break;
        }

        case Node::Namespace: {
            auto p = __walk_next(static_cast<NamespaceNode *>(&i), n);
            if (p != nullptr &&
                (found == nullptr /* Found the first one */ ||
                (found != nullptr && strcmp(p->element().name(), found->element().name()) < 0) /* Found a new one */ )) {
                VTSS_BASICS_TRACE(DEBUG) << "Trap: Update found " << p->element() << " " << (found == nullptr ? n : found->element().name());
                found = p;
            }
            break;
        }
        }
    }

    return found;
}

StructBaseTrap *Globals::trap_find(const char *n) { return __walk(&iso, n); }
StructBaseTrap *Globals::trap_find_next(const char *n) { return __walk_next(&iso, n); }

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
