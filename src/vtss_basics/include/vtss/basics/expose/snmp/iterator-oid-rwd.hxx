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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RWD_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RWD_HXX__

#include "vtss/basics/expose/snmp/iterator-oid-rw.hxx"
#include "vtss/basics/expose/snmp/row-editor-state.hxx"
#include "vtss/basics/expose/snmp/handlers/oid_handler.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template<typename ...T>
struct OidIteratorRWD : public OidIteratorRW<T...> {
    typedef typename KeyTypeList<T...>::type key_type_list;
    typedef typename KeyValTypeList<T...>::type key_val_type_list;
    typedef typename ItrPtrTypeListCalc<T...>::list itr_type_list;
    typedef typename ItrPtrTypeCalc<itr_type_list>::type ItrPtr;
    typedef typename GetPtrTypeCalc<key_val_type_list>::type GetPtr;
    typedef typename SetPtrTypeCalc<key_val_type_list>::type SetPtr;
    typedef typename DelPtrTypeCalc<key_type_list>::type DelPtr;

    typedef typename FindAction<meta::vector<RowEditorState, RowEditorState2>,
                                T...>::type ACTION_COLUMN;

    OidIteratorRWD(GetPtr g, ItrPtr i, SetPtr s, DelPtr d)
            : OidIteratorRW<T...>(g, i, s), del_ptr(d) {
        action_set(0);
    }

    mesa_rc get_next(const OidSequence &idx, OidSequence &next) {
        return OidIteratorRW<T...>::get_next(idx, next);
    }

    mesa_rc get(const OidSequence &oid) {
        return OidIteratorRW<T...>::get(oid);
    }

    mesa_rc set() {
        if (action_get()) {
            // We do not support undo of delete operations!
            OidIteratorRW<T...>::undo_possible_ = false;
            return OidIteratorRW<T...>::value.del(del_ptr);
        }

        return OidIteratorRW<T...>::set();
    }

  private:
    void action_set(uint32_t s) {
        OidIteratorRW<T...>::value.template get_value<ACTION_COLUMN>() = s;
    }

    uint32_t action_get() {
        return OidIteratorRW<T...>::value.template get_value<ACTION_COLUMN>();
    }

    DelPtr del_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RWD_HXX__
