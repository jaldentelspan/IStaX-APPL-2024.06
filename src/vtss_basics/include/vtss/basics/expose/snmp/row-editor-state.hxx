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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ROW_EDITOR_STATE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ROW_EDITOR_STATE_HXX__

#include "vtss/basics/tags.hxx"
#include "vtss/basics/expose/snmp/status.hxx"
#include "vtss/basics/expose/snmp/oid-element-value.hxx"
#include "vtss/basics/expose/snmp/tag-serialize.hxx"

namespace vtss {
namespace expose {
namespace snmp {

struct RowEditorState2 {
    RowEditorState2() : row_state(0) {}
    RowEditorState2(uint32_t rs) : row_state(rs) {}
    RowEditorState2(const RowEditorState2 &rs) : row_state(rs.row_state) {}

    RowEditorState2 &operator=(const RowEditorState2 &rhs) {
        row_state = rhs.row_state;
        return *this;
    }

    RowEditorState2 &operator=(uint32_t rhs) {
        row_state = rhs;
        return *this;
    }

    operator uint32_t () const { return row_state; }

    uint32_t row_state = 0;
};

VTSS_SNMP_TAG_SERIALIZE(RowEditorState, uint32_t, a, s) {
    a.add_leaf(vtss::AsRowEditorState(s.inner), vtss::tag::Name("Action"),
               Status::Current, OidElementValue(0),
               vtss::tag::Description("Action"));
}

#define VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE 0
#define VTSS_SNMP_ROW_TRANSACTION_STATE_CLEAR 1
#define VTSS_SNMP_ROW_TRANSACTION_STATE_WRITE 2
#define VTSS_SNMP_ROW_TRANSACTION_STATE_IS_IDLE(X) \
    ((X) == VTSS_SNMP_ROW_TRANSACTION_STATE_IDLE)

#define VTSS_SNMP_ROW_TRANSACTION_STATE_IS_CLEAR(X) \
    ((X) == VTSS_SNMP_ROW_TRANSACTION_STATE_CLEAR)

#define VTSS_SNMP_ROW_TRANSACTION_STATE_IS_WRITE(X) \
    ((X) == VTSS_SNMP_ROW_TRANSACTION_STATE_WRITE)

#define VTSS_SNMP_ROW_TRANSACTION_STATE_IS_ACTIVE(X) \
    ((X) > VTSS_SNMP_ROW_TRANSACTION_STATE_WRITE)

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ROW_EDITOR_STATE_HXX__
