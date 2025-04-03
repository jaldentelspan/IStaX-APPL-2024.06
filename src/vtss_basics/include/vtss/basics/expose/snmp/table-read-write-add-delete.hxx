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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_ADD_DELETE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_ADD_DELETE_HXX__

#include "vtss/basics/expose/snmp/iterator-row-editor.hxx"
#include "vtss/basics/expose/snmp/row-editor-struct.hxx"
#include "vtss/basics/expose/snmp/table-read-write.hxx"
#include "vtss/basics/expose/snmp/iterator-oid-rwd.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template<typename ...T>
struct TableReadWriteAddDelete : public TableReadWrite<T...> {
    using typename TableReadWrite<T...>::key_val_type_list;
    using typename TableReadWrite<T...>::key_type_list;
    using typename TableReadWrite<T...>::val_type_list;
    using typename TableReadWrite<T...>::GetPtr;
    using typename TableReadWrite<T...>::ItrPtr;
    using typename TableReadWrite<T...>::SetPtr;
    typedef SetPtr AddPtr;
    typedef typename DelPtrTypeCalc<key_type_list>::type DelPtr;
    typedef typename SetDefPtrTypeCalc<key_val_type_list>::type SetDefPtr;
    typedef typename FindAction<meta::vector<RowEditorState, RowEditorState2>,
                                T...>::type ROW_EDITOR;
    typedef OidIteratorRWD<T...> iterator;

    using TableReadWrite<T...>::get;
    using TableReadWrite<T...>::itr;
    using TableReadWrite<T...>::set;

    TableReadWriteAddDelete(NamespaceNode *parent,
                 const OidElement &table_element,
                 const OidElement &row_element,
                 const char *table_description,
                 const char *index_description,
                       GetPtr g, ItrPtr i, SetPtr s, AddPtr a, DelPtr d,
                       SetDefPtr df=nullptr) :
                TableReadWrite<T...>(parent, table_element, table_description,
                                     index_description, g, i, s),
                del(d), row_add(parent, row_element, a, df) {
        static_assert(!meta::equal_type<
                           ROW_EDITOR,
                           PARAM_ACTION<NoSuchAction>
                      >::value,
                      "No RowEditorState action was found in the param list");
    }

    virtual MaxAccess::E max_access() const { return MaxAccess::ReadWrite; }

    virtual unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, iterator>(get, itr, set, del));
    }

#define X(H) virtual void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X

    DelPtr del;

    RowEditorStruct<T...> row_add;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_ADD_DELETE_HXX__
