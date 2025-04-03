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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_ROW_EDITOR_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_ROW_EDITOR_HXX__

#include "vtss/basics/expose/snmp/types_params.hxx"
#include "vtss/basics/expose/snmp/iterator-common.hxx"
#include "vtss/basics/expose/snmp/iterator-row-editor-base.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename... T>
class IteratorRowEditor : public IteratorRowEditorBase, public IteratorCommon {
  public:
    typedef typename KeyValTypeList<T...>::type key_val_type_list;
    typedef typename ValTypeList<T...>::type val_type_list;
    typedef typename SetPtrTypeCalc<key_val_type_list>::type AddPtr;
    typedef typename SetDefPtrTypeCalc<key_val_type_list>::type SetDefPtr;

    typedef typename FindAction<meta::vector<RowEditorState, RowEditorState2>,
                                T...>::type ACTION_COLUMN;

    IteratorRowEditor(AddPtr a, SetDefPtr df, ParamTuple<T...> &v,
                      uint32_t &state)
        : IteratorRowEditorBase(state),
          IteratorCommon(MaxAccess::ReadWrite),
          value(v),
          add_ptr(a),
          set_def_ptr(df) {}

    void call_serialize(GetHandler &h);
    void call_serialize_values(GetHandler &h);

    void call_serialize(SetHandler &h);
    void call_serialize_values(SetHandler &h);

    uint32_t get_external_state() {
        return value.template get_value<ACTION_COLUMN>();
    }

    void set_external_state(uint32_t s) {
        value.template get_value<ACTION_COLUMN>() = s;
    }

    mesa_rc set() { return IteratorRowEditorBase::set(); }

    mesa_rc get(const OidSequence &oid) {
        return IteratorRowEditorBase::get(oid);
    }

    mesa_rc get_next(const OidSequence &idx, OidSequence &next) {
        return IteratorRowEditorBase::get_next(idx, next);
    }

    mesa_rc undo() {
        if (undo_error_)
            return MESA_RC_ERROR;
        else
            return MESA_RC_OK;
    }

    mesa_rc invoke_add() {
        undo_error_ = true;
        return value.set(add_ptr);
    }

    void invoke_reset_values() { value.reset_values(set_def_ptr); }

    // there is no userdefined set and get methods for row-editors
    virtual void *set_ptr__() const { return nullptr; }
    virtual void *get_ptr__() const { return nullptr; }

    ParamTuple<T...> &value;

  private:
    // deiting the row editor has no side effects (unless the row is commited)
    // and there is because of this no need to the row-editor to do any thing.
    // But if the row has been committed the undo step must fail as we do not
    // intend to support the undo step for row add/delete.
    bool undo_error_ = false;

    // function pointers
    AddPtr add_ptr;
    SetDefPtr set_def_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_ROW_EDITOR_HXX__
