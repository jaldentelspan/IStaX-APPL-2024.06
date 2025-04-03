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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ROW_EDITOR_STRUCT_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ROW_EDITOR_STRUCT_HXX__

#include "vtss/basics/expose/snmp/struct-rw.hxx"
#include "vtss/basics/expose/snmp/row-editor-state.hxx"
#include "vtss/basics/expose/snmp/iterator-row-editor.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template<typename ...T>
struct RowEditorStruct : public StructRWBase<T...> {
    typedef typename KeyValTypeList<T...>::type key_val_type_list;
    typedef typename ValTypeList<T...>::type val_type_list;
    typedef typename SetPtrTypeCalc<key_val_type_list>::type AddPtr;
    typedef typename SetDefPtrTypeCalc<key_val_type_list>::type SetDefPtr;

    typedef IteratorRowEditor<T...> IteratorType;

    typedef typename FindAction<meta::vector<RowEditorState, RowEditorState2>,
                                T...>::type ACTION_COLUMN;

    RowEditorStruct(NamespaceNode *p, const OidElement &e, AddPtr a,
                    SetDefPtr df) :
            StructRWBase<T...>(p, e), add_ptr(a),
            set_def_ptr(df) {
        value.reset_values(df);

        value.template get_value<ACTION_COLUMN>() =
            VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE;
    }

    virtual ~RowEditorStruct() { }

    unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<
            IteratorCommon
        >(create<VTSS_MODULE_ID_BASICS, IteratorType>(
                   add_ptr, set_def_ptr, value, internal_state));
    }

  private:
    ParamTuple<T...> value;
    uint32_t internal_state = VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE;
    AddPtr add_ptr;
    SetDefPtr set_def_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ROW_EDITOR_STRUCT_HXX__
