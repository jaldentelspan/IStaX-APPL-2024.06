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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_ADD_DELETE2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_ADD_DELETE2_HXX__

#include "vtss/basics/expose/snmp/iterator-oid-rwd2.hxx"
#include "vtss/basics/expose/snmp/table-read-write2.hxx"
#include "vtss/basics/expose/snmp/struct-row-editor2.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename INTERFACE>
struct TableReadWriteAddDelete2 : public TableReadWrite2<INTERFACE> {
    typedef OidIteratorRWD2<INTERFACE> iterator;

    TableReadWriteAddDelete2(NamespaceNode *parent,
                             const OidElement &table_element,
                             const OidElement &row_element)
        : TableReadWrite2<INTERFACE>(parent, table_element),
          row_editor(parent, row_element) {
        static_assert(INTERFACE::snmpRowEditorOid > 0,
                      "snmpRowEditorOid must be greater than zero");
    }

    virtual MaxAccess::E max_access() const { return MaxAccess::ReadWrite; }

    virtual unique_ptr<IteratorCommon> new_iterator() {
        typename INTERFACE::P::get_t g = INTERFACE::get;
        typename INTERFACE::P::itr_t i = INTERFACE::itr;
        typename INTERFACE::P::set_t s = INTERFACE::set;
        typename INTERFACE::P::del_t d = INTERFACE::del;

        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, iterator>(g, i, s, d));
    }

#define X(H) \
    virtual void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X

    StructRowEditor2<INTERFACE> row_editor;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_ADD_DELETE2_HXX__
