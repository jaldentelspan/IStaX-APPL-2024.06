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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RW2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RW2_HXX__

#include "vtss/basics/expose/snmp/handlers/oid_handler.hxx"
#include "vtss/basics/expose/snmp/iterator-oid-ro2.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename INTERFACE>
struct OidIteratorRW2 : public OidIteratorRO2<INTERFACE> {
    typedef typename INTERFACE::P::get_t GetPtr;
    typedef typename INTERFACE::P::itr_t ItrPtr;
    typedef typename INTERFACE::P::set_t SetPtr;

    OidIteratorRW2(GetPtr g, ItrPtr i, SetPtr s)
        : OidIteratorRO2<INTERFACE>(g, i), set_ptr(s) {}

    virtual mesa_rc get(const OidSequence &oid) {
        mesa_rc rc = OidIteratorRO2<INTERFACE>::get(oid);

        if (rc == MESA_RC_OK && !undo_ready_) {
            // Arm the undo mechanism
            undo_value = OidIteratorRO2<INTERFACE>::value;
            undo_ready_ = true;
        }

        return rc;
    }

    virtual mesa_rc set() {
        mesa_rc rc = OidIteratorRO2<INTERFACE>::value.set(set_ptr);

        // Once the set method has been successfully called an undo step is
        // needed in case of failure
        if (rc == MESA_RC_OK) undo_needed_ = true;

        return rc;
    }

    virtual mesa_rc undo() {
        if (!undo_needed_) return MESA_RC_OK;

        // TODO, create an error code for this
        if (!undo_possible_) return MESA_RC_ERROR;

        if (!undo_ready_) return MESA_RC_ERROR;

        return undo_value.set(set_ptr);
    }

    virtual void *set_ptr__() const { return (void *)(set_ptr); }

  protected:
    // A undo is only needed if the set request succeeded. The set function
    // pointers provided by the  user is assumed to be atomic, meaning that they
    // either succeeds or fails completely.
    bool undo_needed_ = false;

    // Keeps track on if valid data is loaded into the undo_value tuple
    bool undo_ready_ = false;

    // Certain operations such as row deletion and addition can not be undone,
    // at least not yet.
    bool undo_possible_ = true;

    // the "get" values are copied, and when undoing, these values are being set
    // again.
    typename Interface2Any<ParamTuple, INTERFACE>::type undo_value;

    SetPtr set_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RW2_HXX__
