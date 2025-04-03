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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY_HXX__

#include "vtss/basics/module_id.hxx"
#include "vtss/basics/expose/snmp/struct-base.hxx"
#include "vtss/basics/expose/snmp/iterator-oid-ro.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template<typename ...T>
struct TableReadOnly : public StructBase {
    typedef typename KeyValTypeList<T...>::type key_val_type_list;
    typedef typename KeyTypeList<T...>::type key_type_list;
    typedef typename ValTypeList<T...>::type val_type_list;
    typedef typename ItrPtrTypeListCalc<T...>::list itr_type_list;
    typedef typename ItrPtrTypeCalc<itr_type_list>::type ItrPtr;
    typedef typename GetPtrTypeCalc<key_val_type_list>::type GetPtr;
    typedef OidIteratorRO<T...> iterator;

    TableReadOnly(NamespaceNode *p, const OidElement &e, const char *td,
               const char *id, GetPtr g, ItrPtr i) : StructBase(p, e),
            table_description(td), index_description(id),
            get(g), itr(i) {
    }

    const char *table_description;
    const char *index_description;

    virtual unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, iterator>(get, itr));
    }

    virtual MaxAccess::E max_access() const { return MaxAccess::ReadOnly; }

#define X(H) virtual void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X

    GetPtr get;
    ItrPtr itr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY_HXX__
