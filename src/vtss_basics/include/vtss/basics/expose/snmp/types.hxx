/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_TYPES_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_TYPES_HXX__

#include "vtss/basics/tags.hxx"
#include "vtss/basics/types.hxx"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/assert.hxx"
#include "vtss/basics/memory.hxx"
#include "vtss/basics/predefs.hxx"
#include "vtss/basics/module_id.hxx"
#include "vtss/basics/meta-data-packet.hxx"
#include "vtss/basics/parser_impl.hxx"
#include "vtss/basics/intrusive_list.hxx"
#include "vtss/basics/enum-utils.hxx"

#include "vtss/basics/expose/snmp/asn-type.hxx"
#include "vtss/basics/expose/snmp/error_code.hxx"
#include "vtss/basics/expose/snmp/enum_macros.hxx"
#include "vtss/basics/expose/snmp/oid_sequence.hxx"
#include "vtss/basics/expose/snmp/oid-offset.hxx"
#include "vtss/basics/expose/snmp/oid-element-value.hxx"
#include "vtss/basics/expose/snmp/type-classes.hxx"
#include "vtss/basics/expose/snmp/types_params.hxx"
#include "vtss/basics/expose/snmp/handler-state.hxx"
#include "vtss/basics/expose/snmp/post-set-action.hxx"
#include "vtss/basics/expose/snmp/pre-get-condition.hxx"
#include "vtss/basics/expose/snmp/namespace-node.hxx"
#include "vtss/basics/expose/snmp/max-access.hxx"
#include "vtss/basics/expose/snmp/range-spec.hxx"
#include "vtss/basics/expose/snmp/struct-base.hxx"
#include "vtss/basics/expose/snmp/iterator-common.hxx"
#include "vtss/basics/expose/snmp/status.hxx"
#include "vtss/basics/expose/snmp/iterator-single-row.hxx"
#include "vtss/basics/expose/snmp/iterator-oid-rwd.hxx"
#include "vtss/basics/expose/snmp/struct-ro.hxx"
#include "vtss/basics/expose/snmp/struct-ro2.hxx"
#include "vtss/basics/expose/snmp/struct-ro-subject.hxx"
#include "vtss/basics/expose/snmp/struct-ro-trap.hxx"
#include "vtss/basics/expose/snmp/struct-rw.hxx"
#include "vtss/basics/expose/snmp/struct-rw2.hxx"
#include "vtss/basics/expose/snmp/list-of-handlers.hxx"
#include "vtss/basics/expose/snmp/node-impl.hxx"
#include "vtss/basics/expose/snmp/table-read-only.hxx"
#include "vtss/basics/expose/snmp/table-read-only2.hxx"
#include "vtss/basics/expose/snmp/table-read-only-trap.hxx"
#include "vtss/basics/expose/snmp/row-editor-state.hxx"
#include "vtss/basics/expose/snmp/table-read-write.hxx"
#include "vtss/basics/expose/snmp/table-read-write2.hxx"
#include "vtss/basics/expose/snmp/table-read-write-add-delete.hxx"
#include "vtss/basics/expose/snmp/table-read-write-add-delete2.hxx"
#include "vtss/basics/expose/snmp/status-spec.hxx"

#ifndef MIBPREFIX
#define MIBPREFIX                       VTSS
#endif
#ifndef MIB_ENTERPRISE_NAME
#define MIB_ENTERPRISE_NAME             vtss
#endif
#ifndef MIB_ENTERPRISE_OID
#define MIB_ENTERPRISE_OID              6603
#endif
#ifndef MIB_ENTERPRISE_PRODUCT_NAME
#define MIB_ENTERPRISE_PRODUCT_NAME     vtssSwitchMgmt
#endif
#ifndef MIB_ENTERPRISE_PRODUCT_ID
#define MIB_ENTERPRISE_PRODUCT_ID       1
#endif

namespace vtss {
namespace expose {
namespace snmp {

/*----------  Enumerated types ----------------*/
void print_enum_name(ostream &o, const vtss_enum_descriptor_t *descr);

template <typename... T>
struct OidIteratorRO;
template <typename... T>
struct OidIteratorRW;
template <typename... T>
struct OidIteratorRWD;

template <typename... T>
struct TableReadWriteAddDelete;

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_TYPES_HXX__
