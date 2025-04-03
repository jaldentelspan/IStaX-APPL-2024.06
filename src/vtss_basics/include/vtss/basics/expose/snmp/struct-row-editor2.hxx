/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_STRUCT_ROW_EDITOR2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_STRUCT_ROW_EDITOR2_HXX__

#include "vtss/basics/expose/snmp/struct-rw2.hxx"
#include "vtss/basics/expose/snmp/iterator-row-editor2.hxx"
#include "vtss/basics/expose/snmp/row-editor-state.hxx"
#include "vtss/basics/expose/snmp/def_pointer_or_null.hxx"

namespace vtss {
namespace expose {
namespace snmp {


template <typename INTERFACE>
struct StructRowEditor2 : public StructRWBase2<INTERFACE> {
    typedef IteratorRowEditor2<INTERFACE> iterator;
    typedef typename Interface2Any<meta::vector, INTERFACE>::type TypeList;

    typedef typename FindAction2<meta::vector<RowEditorState, RowEditorState2>,
                                 TypeList>::type ACTION_COLUMN;

    StructRowEditor2(NamespaceNode *p, const OidElement &e)
        : StructRWBase2<INTERFACE>(p, e) {
    }

    void init() override {
        typename INTERFACE::P::def_t d = def_pointer_or_null<INTERFACE>::def;
        value.reset_values(d);
        value.template get_value<ACTION_COLUMN>() =
                VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE;
    }

    virtual ~StructRowEditor2() {}

    unique_ptr<IteratorCommon> new_iterator() {
        typename INTERFACE::P::add_t a = INTERFACE::add;
        typename INTERFACE::P::def_t d = def_pointer_or_null<INTERFACE>::def;

        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, iterator>(a, d, value,
                                                        internal_state));
    }

  private:
    typename Interface2ParamTuple<INTERFACE>::type value;
    uint32_t internal_state = VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_STRUCT_ROW_EDITOR2_HXX__
