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
#include "vtss/basics/expose/snmp/handlers/set.hxx"

namespace vtss {
namespace expose {
namespace snmp {

void serialize_set_struct_ro_generic(SetHandler &h, StructBase &o) {
    if (!h.consume_oid(o.element().numeric()))
        return;

    VTSS_BASICS_TRACE(NOISE) << "Read only";
    h.error_code(ErrorCode::readOnly, __FILE__, __LINE__);
    return;
}

void serialize_set_struct_rw_generic(SetHandler &h, StructBase &o) {
    if (!h.consume_oid(o.element().numeric()))
        return;

    OidSequence current_oid(h.seq_.oids, h.oid_index_);
    VTSS_BASICS_TRACE(DEBUG) << "CurrentOID: " << current_oid;

    IteratorCommon *common_iterator = nullptr;
    common_iterator = h.cache_loopup(current_oid);

    if (!common_iterator) {  // non cached iterator was found
        auto iter = o.new_iterator();
        if (!iter || iter->get(h.oid_seq_index) != MESA_RC_OK) {
            VTSS_BASICS_TRACE(DEBUG) << "No iterator returned or get faied: " <<
                current_oid;
            h.error_code(ErrorCode::noSuchName, __FILE__, __LINE__);
            return;
        }

        // new iterator has been created, store it in the cache
        common_iterator = iter.release(); // handover owner ship
        if (h.cache_add(current_oid, common_iterator)) {
            VTSS_BASICS_TRACE(DEBUG) << "Added iter: " << current_oid;
        } else {
            VTSS_BASICS_TRACE(DEBUG) << "Failed adding iter: " << current_oid;
            h.error_code(ErrorCode::noSuchName, __FILE__, __LINE__);
            destroy(common_iterator);
            return;
        }
    } else {
        VTSS_BASICS_TRACE(DEBUG) << "Using cached iterator";
    }

    if (common_iterator)
        common_iterator->call_serialize(h);
}

void serialize_set_table_rw_generic(SetHandler &h, StructBase &o) {
    if (!h.consume_oid(o.element().numeric()))
        return;

    // Every table has a hard-coded "1" node to contain the table.
    if (!h.consume_oid(1))
        return;

    // Create a unique index by concatenating the current OID string with the
    // OID index value(s)
    OidSequence current_oid(h.seq_.oids, h.oid_index_);
    current_oid.push(h.oid_seq_index);

    IteratorCommon *common_iterator = nullptr;
    common_iterator = h.cache_loopup(current_oid);

    if (!common_iterator) {  // non cached iterator was found
        auto iter = o.new_iterator();
        if (!iter || iter->get(h.oid_seq_index) != MESA_RC_OK) {
            VTSS_BASICS_TRACE(DEBUG) << "No iterator returned or get faied: " <<
                current_oid;
            h.error_code(ErrorCode::noSuchName, __FILE__, __LINE__);
            return;
        }

        // new iterator has been created, store it in the cache
        common_iterator = iter.release(); // handover owner ship
        if (h.cache_add(current_oid, common_iterator)) {
            VTSS_BASICS_TRACE(DEBUG) <<
                "Added iter: " << current_oid;
        } else {
            VTSS_BASICS_TRACE(DEBUG) << "Failed adding iter: " << current_oid;
            h.error_code(ErrorCode::noSuchName, __FILE__, __LINE__);
            destroy(common_iterator);
            return;
        }
    } else {
        VTSS_BASICS_TRACE(DEBUG) << "Using cached iterator";
    }

    if (common_iterator)
        common_iterator->call_serialize(h);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
