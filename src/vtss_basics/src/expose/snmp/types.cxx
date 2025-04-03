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

#include <stdlib.h>

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "vtss/basics/expose/snmp/types.hxx"

//inline void *operator new(size_t size, void *ptr) { return ptr; }

namespace vtss {
namespace expose {
namespace snmp {

void print_enum_name(ostream &o, const vtss_enum_descriptor_t * descr) {
    o << "INTEGER {";

    if (descr->valueName) {
        o << " " << descr->valueName << "(" << descr->intValue << ")";
        descr++;
    }

    while (descr->valueName) {
        o << ", " << descr->valueName << "(" << descr->intValue << ")";
        descr++;
    }

    o << " }";
}

ostream& operator<<(ostream &o, const OidElement &e) {
    o << e.name_ << "(" << e.numeric_ << ")";
    return o;
}

struct KnownOid {
    KnownOid *parent;
    vtss_oid oid;
    const char *oidName;
};

KnownOid known_oids[] = {
    {nullptr, 1, "iso"},
    {&known_oids[0], 3, "identified-organization"},
    {&known_oids[1], 6, "dod"},
    {&known_oids[2], 1, "internet"},
    {&known_oids[3], 4, "private"},
    {&known_oids[4], 1, "enterprises"},
    {&known_oids[5], MIB_ENTERPRISE_OID, TO_STR(MIB_ENTERPRISE_NAME)},
    {&known_oids[6], MIB_ENTERPRISE_PRODUCT_ID, TO_STR(MIB_ENTERPRISE_PRODUCT_NAME)},
};

KnownOid *is_known_oid(const NamespaceNode *o) {
    KnownOid *known_oid = nullptr;
    if (o->parent()) {
        known_oid = is_known_oid(o->parent());
        if (!known_oid) return known_oid;
    }

    for (uint32_t i = 0; i < sizeof(known_oids)/sizeof(KnownOid); ++i) {
        if (known_oids[i].parent == known_oid &&
            known_oids[i].oid == o->element().numeric_) return &known_oids[i];
    }
    return nullptr;
}

void print_numeric_oid_seq(ostream *os, const NamespaceNode *o) {
    KnownOid *known_oid = is_known_oid(o);
    if (known_oid) {
        (*os) << known_oid->oidName;
        return;
    }
    if (o->parent()) {
        print_numeric_oid_seq(os, o->parent());
        (*os) << " ";
    }
    (*os) << (o->element().numeric_);
};

namespace MaxAccess {
    ostream& operator<<(ostream &o, const E &e) {
        switch (e) {
          case ReadOnly:
            o << "read-only";
            return o;

          case WriteOnly:
            o << "write-only";
            return o;

          case ReadWrite:
            o << "read-write";
            return o;

          case AccessibleForNotify:
            o << "accessible-for-notify";
            return o;

          case NotAccessible:
            o << "not-accessible";
            return o;

          default:
            o << "<invalid:" << static_cast<int>(e) << ">";
            return o;
        }
    }
};  // namespace MaxAccess

template<typename T>
ostream& operator<<(ostream &o, const RangeSpec<T> &r) {
    o << "(" << r.from << ".." << r.to << ")";
    return o;
}

namespace Status {
    ostream& operator<<(ostream &o, const E &e) {
        switch (e) {
          case Current:
            o << "current";
            return o;

          case Deprecated:
            o << "deprecated";
            return o;

          case Obsoleted:
            o << "obsolete";
            return o;

          default:
            o << "<invalid:" << static_cast<int>(e) << ">";
            return o;
        }
    }
};  // namespace Status

mesa_rc IteratorRowEditorBase::get_next(const OidSequence &idx,
                                        OidSequence &next) {
    next.push(0);
    return MESA_RC_OK;
}

mesa_rc IteratorRowEditorBase::set() {
    uint32_t v = get_external_state();
    VTSS_BASICS_TRACE(NOISE) << "Current state is: " << internal_state <<
        " action is: " << v;

    switch (internal_state) {
      case VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE:  return set_is_idle(v);
      case VTSS_SNMP_ROW_TRANSACTION_STATE_CLEAR: return set_is_clear(v);
      case VTSS_SNMP_ROW_TRANSACTION_STATE_WRITE: return set_is_write(v);
      default:                                    return set_is_active(v);
    }
}

mesa_rc IteratorRowEditorBase::go_idle() {
    internal_state = VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE;
    set_external_state(VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE);
    invoke_reset_values();
    return MESA_RC_OK;
}

mesa_rc IteratorRowEditorBase::go_active(uint32_t s) {
    VTSS_BASICS_TRACE(INFO) << "go active: " << s;
    if (!VTSS_SNMP_ROW_TRANSACTION_STATE_IS_ACTIVE(s))
        return MESA_RC_OK;

    internal_state = s;
    set_external_state(s);
    return MESA_RC_OK;
}

mesa_rc IteratorRowEditorBase::set_is_idle(uint32_t s) {
    switch (s) {
      case VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE:  return MESA_RC_OK;
      case VTSS_SNMP_ROW_TRANSACTION_STATE_CLEAR: return go_idle();

      case VTSS_SNMP_ROW_TRANSACTION_STATE_WRITE:
        VTSS_BASICS_TRACE(INFO) << "Can not write in idle state";
        (void) go_idle();
        return MESA_RC_ERROR;

      default:  // active
        return go_active(s);
    }
}

mesa_rc IteratorRowEditorBase::set_is_clear(uint32_t s) {
    VTSS_BASICS_TRACE(ERROR) << "The state machine may never enter the "
        "clear state. This must only be used as an action";
    (void) go_idle();
    return MESA_RC_OK;
}

mesa_rc IteratorRowEditorBase::set_is_write(uint32_t) {
    VTSS_BASICS_TRACE(ERROR) << "The state machine may never enter the "
        "write state. This must only be used as an action";
    (void) go_idle();
    return MESA_RC_OK;
}

mesa_rc IteratorRowEditorBase::set_is_active(uint32_t s) {
    switch (s) {
      case VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE:  return MESA_RC_OK;
      case VTSS_SNMP_ROW_TRANSACTION_STATE_CLEAR: return go_idle();

      case VTSS_SNMP_ROW_TRANSACTION_STATE_WRITE: {
        VTSS_BASICS_TRACE(INFO) << "Invoke add function";
        mesa_rc rc = invoke_add();
        (void) go_idle();
        return rc;
      }

      default:  // active
        VTSS_BASICS_TRACE(INFO) << "Already active: " <<
            get_external_state() << " " << internal_state;
        set_external_state(internal_state);
        VTSS_BASICS_TRACE(INFO) << "Already active: " <<
            get_external_state() << " " << internal_state;
        return MESA_RC_OK;
    }
}

mesa_rc IteratorRowEditorBase::get(const OidSequence &oid) {
    if (oid.valid != 1 || oid.oids[0] != 0) {
        VTSS_BASICS_TRACE(NOISE) << "Unexpected index oid seq for a dummy "
            "iterator. idx: " << oid;
        return MESA_RC_ERROR;
    }

    return MESA_RC_OK;
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
