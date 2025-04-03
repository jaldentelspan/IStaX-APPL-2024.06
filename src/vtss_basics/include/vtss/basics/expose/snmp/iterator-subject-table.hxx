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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_TABLE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_TABLE_HXX__

#include <vtss/basics/expose/table-status.hxx>
#include <vtss/basics/expose/serialize-keys.hxx>
#include <vtss/basics/expose/serialize-values.hxx>
#include <vtss/basics/expose/snmp/types_params.hxx>
#include <vtss/basics/expose/snmp/iterator-common.hxx>

namespace vtss {
namespace expose {
namespace snmp {

struct TrapHandler;

template <typename INTERFACE>
struct IteratorSubjectTable : public IteratorCommon {
    typedef TableStatusInterface<INTERFACE> Subject;

    IteratorSubjectTable(MaxAccess::E a, Subject *s)
        : IteratorCommon(a), subject(s) {}

    void call_serialize(GetHandler &h);
    void call_serialize_values(GetHandler &h);
    void call_serialize_trap(TrapHandler &h);
    void call_serialize(SetHandler &h);
    void call_serialize_values(SetHandler &h);

    // Hack hack hack... This could be optimized...
    mesa_rc get_next(const OidSequence &idx, OidSequence &next);

    mesa_rc get(const OidSequence &oid);

    mesa_rc get_trap() {
        VTSS_ASSERT(0);
        return MESA_RC_ERROR;
    }

    mesa_rc set() { return MESA_RC_ERROR; }
    mesa_rc undo() { return MESA_RC_ERROR; }

    void *set_ptr__() const { return (void *)subject; }
    void *get_ptr__() const { return (void *)subject; }

    typename Subject::K k;
    typename Subject::V v;

  private:
    Subject *subject;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_TABLE_HXX__
