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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RO2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RO2_HXX__

#include "vtss/basics/module_id.hxx"
#include "vtss/basics/expose/types.hxx"
#include "vtss/basics/expose/snmp/struct-base.hxx"
#include "vtss/basics/expose/snmp/types_params.hxx"
#include "vtss/basics/expose/snmp/list-of-handlers.hxx"
#include "vtss/basics/expose/snmp/iterator-single-row2.hxx"
#include "vtss/basics/expose/snmp/param-list-converter.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename T>
struct StructROBase2 : public StructBase {
    typedef typename Interface2ParamTuple<T>::type ValueType;
    typedef IteratorSingleRow2<T> IteratorType;

    StructROBase2(NamespaceNode *p, const OidElement &e) : StructBase(p, e) {
        static_assert(sizeof(ValueType) < 1024,
                      "Size of structs cannot be more than one 1024 bytes");
    }

    virtual ~StructROBase2() {}

#define X(H) \
    void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X
};

template <typename INTERFACE_DESCRIPTOR>
struct StructRO2 : public StructROBase2<INTERFACE_DESCRIPTOR> {
    typedef StructROBase2<INTERFACE_DESCRIPTOR> BASE;
    typedef typename INTERFACE_DESCRIPTOR::P::get_t GetPtr;
    typedef typename BASE::IteratorType IteratorType;

    StructRO2(NamespaceNode *p, const OidElement &e) : BASE(p, e) {}

    virtual ~StructRO2() {}

    virtual unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, IteratorType>(get_ptr));
    }

  private:
    GetPtr get_ptr = INTERFACE_DESCRIPTOR::get;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RO2_HXX__
