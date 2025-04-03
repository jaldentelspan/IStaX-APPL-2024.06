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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_TABLE_IMPL_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_TABLE_IMPL_HXX__

#include "vtss/basics/expose/snmp/types_params.hxx"
#include "vtss/basics/expose/snmp/iterator-common.hxx"
#include "vtss/basics/expose/snmp/iterator-row-editor2.hxx"
#include "vtss/basics/expose/snmp/handlers/get.hxx"
#include "vtss/basics/expose/snmp/handlers/set.hxx"
#include "vtss/basics/expose/snmp/handlers/trap.hxx"
#include "vtss/basics/expose/snmp/handlers/oid_handler.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename INTERFACE>
void IteratorSubjectTable<INTERFACE>::call_serialize(GetHandler &h) {
    serialize_values<INTERFACE>(h, v);
}

template <typename INTERFACE>
void IteratorSubjectTable<INTERFACE>::call_serialize_values(GetHandler &h) {
    serialize_values<INTERFACE>(h, v);
}

template <typename INTERFACE>
void IteratorSubjectTable<INTERFACE>::call_serialize_trap(TrapHandler &h) {
    serialize_values<INTERFACE>(h, v);
};

template <typename INTERFACE>
void IteratorSubjectTable<INTERFACE>::call_serialize(SetHandler &h) {}

template <typename INTERFACE>
void IteratorSubjectTable<INTERFACE>::call_serialize_values(SetHandler &h) {}

// Hack hack hack... This could be optimized...
template <typename INTERFACE>
mesa_rc IteratorSubjectTable<INTERFACE>::get_next(const OidSequence &idx, OidSequence &next) {
    // Try see if we can parse the OID sequence
    OidImporter importer(idx, false);
    serialize_keys<INTERFACE>(importer, k);
    if (!importer.ok_ || !importer.all_consumed()) {
        // If not, do a get-next and iterate from there
        mesa_rc rc = subject->get_first_(k, v);
        if (rc != MESA_RC_OK) return rc;
    }

    while (true) {
        OidExporter e;
        serialize_keys<INTERFACE>(e, k);
        if (!e.ok_) return MESA_RC_ERROR;

        if (e.oids_ > idx) {
            next = e.oids_;
            break;
        }

        mesa_rc rc = subject->get_next_(k, v);
        if (rc != MESA_RC_OK) return rc;
    }

    return MESA_RC_OK;
}

template <typename INTERFACE>
mesa_rc IteratorSubjectTable<INTERFACE>::get(const OidSequence &oid) {
    OidImporter importer(oid, false);
    serialize_keys<INTERFACE>(importer, k);

    if (!importer.ok_) return MESA_RC_ERROR;
    if (!importer.all_consumed()) return MESA_RC_ERROR;

    return subject->get_(k, v);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SUBJECT_TABLE_IMPL_HXX__
