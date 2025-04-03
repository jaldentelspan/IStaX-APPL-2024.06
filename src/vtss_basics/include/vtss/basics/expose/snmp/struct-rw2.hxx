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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RW2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RW2_HXX__

#include "vtss/basics/expose/snmp/struct-ro2.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename T>
struct StructRWBase2 : public StructROBase2<T> {
    typedef typename Interface2ParamTuple<T>::type ValueType;
    typedef IteratorSingleRow2<T> IteratorType;

    StructRWBase2(NamespaceNode *p, const OidElement &e)
        : StructROBase2<T>(p, e) {}

    virtual ~StructRWBase2() {}

#define X(H) \
    void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X
};

template <typename INTERFACE_DESCRIPTOR>
struct StructRW2 : public StructRWBase2<INTERFACE_DESCRIPTOR> {
    typedef StructRWBase2<INTERFACE_DESCRIPTOR> BASE;
    typedef typename INTERFACE_DESCRIPTOR::P::get_t GetPtr;
    typedef typename INTERFACE_DESCRIPTOR::P::set_t SetPtr;
    typedef typename BASE::IteratorType IteratorType;

    StructRW2(NamespaceNode *p, const OidElement &e) : BASE(p, e) {}
    virtual ~StructRW2() {}

    virtual unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, IteratorType>(get_ptr, set_ptr));
    }

  private:
    GetPtr get_ptr = INTERFACE_DESCRIPTOR::get;
    SetPtr set_ptr = INTERFACE_DESCRIPTOR::set;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RW2_HXX__
