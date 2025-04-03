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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY2_HXX__

#include "vtss/basics/module_id.hxx"
#include "vtss/basics/expose/types.hxx"
#include "vtss/basics/expose/snmp/struct-base.hxx"
#include "vtss/basics/expose/snmp/iterator-oid-ro2.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename INTERFACE>
struct TableReadOnly2 : public StructBase {
    typedef OidIteratorRO2<INTERFACE> iterator;
    TableReadOnly2(NamespaceNode *p, const OidElement &e) : StructBase(p, e) {}

    static constexpr const char *table_description =
            INTERFACE::table_description;
    static constexpr const char *index_description =
            INTERFACE::index_description;

    virtual unique_ptr<IteratorCommon> new_iterator() {
        typename INTERFACE::P::get_t get_ptr = INTERFACE::get;
        typename INTERFACE::P::itr_t itr_ptr = INTERFACE::itr;
        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, iterator>(get_ptr, itr_ptr));
    }

    virtual MaxAccess::E max_access() const { return MaxAccess::ReadOnly; }

#define X(H) \
    virtual void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY2_HXX__
