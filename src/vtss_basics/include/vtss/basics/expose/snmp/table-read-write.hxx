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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_HXX__

#include "vtss/basics/expose/snmp/table-read-only.hxx"
#include "vtss/basics/expose/snmp/iterator-oid-rw.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template<typename ...T>
struct TableReadWrite : public TableReadOnly<T...> {
    using typename TableReadOnly<T...>::key_val_type_list;
    using typename TableReadOnly<T...>::key_type_list;
    using typename TableReadOnly<T...>::val_type_list;
    using typename TableReadOnly<T...>::GetPtr;
    using typename TableReadOnly<T...>::ItrPtr;
    using TableReadOnly<T...>::get;
    using TableReadOnly<T...>::itr;
    typedef typename SetPtrTypeCalc<key_val_type_list>::type SetPtr;
    typedef OidIteratorRW<T...> iterator;

    TableReadWrite(NamespaceNode *p, const OidElement &e, const char *td,
               const char *id, GetPtr g, ItrPtr i, SetPtr s) :
            TableReadOnly<T...>(p, e, td, id, g, i), set(s) { }

    virtual unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, iterator>(get, itr, set));
    }

    virtual MaxAccess::E max_access() const { return MaxAccess::ReadWrite; }

#define X(H) virtual void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X

    SetPtr set;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_WRITE_HXX__
