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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SINGLE_ROW_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SINGLE_ROW_HXX__

#include "vtss/basics/expose/snmp/iterator-common.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename... T>
class IteratorSingleRow : public IteratorCommon {
  public:
    typedef mesa_rc (*GetPtr)(typename T::get_ptr_type...);
    typedef mesa_rc (*SetPtr)(typename T::set_ptr_type...);

    explicit IteratorSingleRow(GetPtr g)
        : IteratorCommon(MaxAccess::ReadOnly), get_ptr(g), set_ptr(nullptr) {}

    void call_serialize(GetHandler &h);
    void call_serialize_values(GetHandler &h);

    void call_serialize(SetHandler &h);
    void call_serialize_values(SetHandler &h);

    IteratorSingleRow(GetPtr g, SetPtr s)
        : IteratorCommon(MaxAccess::ReadWrite), get_ptr(g), set_ptr(s) {}

    virtual mesa_rc get_next(const OidSequence &idx, OidSequence &next) {
        next.push(0);
        return value.get(get_ptr);
    }

    virtual mesa_rc set() {
        mesa_rc rc = value.set(set_ptr);
        if (rc == MESA_RC_OK) undo_needed_ = true;

        return rc;
    }

    virtual mesa_rc get(const OidSequence &oid) {
        if (oid.valid != 1 || oid.oids[0] != 0) {
            //  VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, NOISE) <<
            //      "Unexpected index oid seq for a dummy iterator. idx: " <<
            //      oid;
            return MESA_RC_ERROR;
        }

        mesa_rc rc = value.get(get_ptr);

        // copying the undo values is only needed if the set pointer is defined!
        if (rc == MESA_RC_OK && set_ptr != nullptr) undo_value_ = value;

        return rc;
    }

    virtual mesa_rc undo() {
        if (!undo_needed_) return MESA_RC_OK;

        return undo_value_.set(set_ptr);
    }

    virtual void *set_ptr__() const { return (void *)set_ptr; }
    virtual void *get_ptr__() const { return (void *)get_ptr; }

    ParamTuple<T...> value;

  private:
    // A undo is only needed if the set request succeeded. The set function
    // pointers provided by the  user is assumed to be atomic, meaning that they
    // either succeeds or fails completely.
    bool undo_needed_ = false;
    ParamTuple<T...> undo_value_;

    GetPtr get_ptr;
    SetPtr set_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_SINGLE_ROW_HXX__
