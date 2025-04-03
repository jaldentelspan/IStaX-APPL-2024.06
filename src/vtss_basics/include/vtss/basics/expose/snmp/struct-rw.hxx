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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RW_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RW_HXX__

#include "vtss/basics/expose/snmp/struct-ro.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template<typename ...T>
struct StructRWBase : public StructROBase<T...> {
    typedef IteratorSingleRow<T...> IteratorType;

    StructRWBase(NamespaceNode *p, const OidElement &e) :
                StructROBase<T...>(p, e) { }
    virtual ~StructRWBase() { }

#define X(H) void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X
};

template<typename ...T>
struct StructRW : public StructRWBase<T...> {
    typedef mesa_rc (*GetPtr)(typename T::get_ptr_type...);
    typedef mesa_rc (*SetPtr)(typename T::set_ptr_type...);
    typedef IteratorSingleRow<T...> IteratorType;

    StructRW(NamespaceNode *p, const OidElement &e, GetPtr g, SetPtr s) :
                StructRWBase<T...>(p, e), get_ptr(g), set_ptr(s) { }
    virtual ~StructRW() { }

    virtual unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<IteratorCommon>(
                create<VTSS_MODULE_ID_BASICS, IteratorSingleRow<T...>>(
                    get_ptr, set_ptr));
    }

  private:
    GetPtr get_ptr;
    SetPtr set_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RW_HXX__
