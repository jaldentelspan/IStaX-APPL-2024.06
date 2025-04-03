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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_SCALAR_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_SCALAR_HXX__

#include <vtss/basics/expose/struct-status.hxx>
#include <vtss/basics/expose/serialize-values.hxx>
#include <vtss/basics/expose/snmp/types_params.hxx>
#include <vtss/basics/expose/snmp/iterator-common-trap.hxx>

namespace vtss {
namespace expose {
namespace snmp {

template <typename INTERFACE>
struct IteratorSubjectScalar : public IteratorCommonTrap {
    typedef StructStatusInterface<INTERFACE> Subject;
    typedef typename Subject::V VALUE;

    IteratorSubjectScalar(MaxAccess::E a, Subject *s)
        : IteratorCommonTrap(a), subject(s) {}

    void call_serialize(GetHandler &h) {
        serialize_values<INTERFACE>(h, value);
    }

    void call_serialize_values(GetHandler &h) {
        serialize_values<INTERFACE>(h, value);
    }

    void call_serialize_trap(TrapHandler &h) {
        serialize_values<INTERFACE>(h, value);
    };

    void call_serialize(SetHandler &h) {}
    void call_serialize_values(SetHandler &h) {}

    mesa_rc get_next(const OidSequence &idx, OidSequence &next) {
        next.push(0);
        value = subject->get();

        return MESA_RC_OK;
    }

    mesa_rc set() { return MESA_RC_ERROR; }

    mesa_rc get(const OidSequence &oid) {
        if (oid.valid != 1 || oid.oids[0] != 0) {
            //  VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, NOISE) <<
            //      "Unexpected index oid seq for a dummy iterator. idx: " <<
            //      oid;
            return MESA_RC_ERROR;
        }

        return get_trap();
    }

    mesa_rc get_trap() {
        value = subject->get();
        return MESA_RC_OK;
    }

    mesa_rc undo() { return MESA_RC_ERROR; }

    void *set_ptr__() const { return (void *)subject; }
    void *get_ptr__() const { return (void *)subject; }

    VALUE value;

  private:
    Subject *subject;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_SCALAR_HXX__
