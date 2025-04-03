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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RWD2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RWD2_HXX__

#include "vtss/basics/expose/snmp/iterator-oid-rw2.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename INTERFACE>
struct OidIteratorRWD2 : public OidIteratorRW2<INTERFACE> {
    typedef typename INTERFACE::P::get_t GetPtr;
    typedef typename INTERFACE::P::itr_t ItrPtr;
    typedef typename INTERFACE::P::set_t SetPtr;
    typedef typename INTERFACE::P::del_t DelPtr;

    typedef typename Interface2Any<meta::vector, INTERFACE>::type TypeList;
    typedef typename FindAction2<meta::vector<RowEditorState, RowEditorState2>,
                                 TypeList>::type ACTION_COLUMN;

    OidIteratorRWD2(GetPtr g, ItrPtr i, SetPtr s, DelPtr d)
        : OidIteratorRW2<INTERFACE>(g, i, s), del_ptr(d) {
        action_set(0);
    }

    mesa_rc get_next(const OidSequence &idx, OidSequence &next) {
        return OidIteratorRW2<INTERFACE>::get_next(idx, next);
    }

    mesa_rc get(const OidSequence &oid) {
        return OidIteratorRW2<INTERFACE>::get(oid);
    }

    mesa_rc set() {
        if (action_get()) {
            // We do not support undo of delete operations!
            OidIteratorRW2<INTERFACE>::undo_possible_ = false;
            return OidIteratorRW2<INTERFACE>::value.del(del_ptr);
        }

        return OidIteratorRW2<INTERFACE>::set();
    }

  private:
    void action_set(uint32_t s) {
        OidIteratorRW2<INTERFACE>::value.template get_value<ACTION_COLUMN>() =
                s;
    }

    uint32_t action_get() {
        return OidIteratorRW2<INTERFACE>::value
                .template get_value<ACTION_COLUMN>();
    }

    DelPtr del_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RWD2_HXX__
