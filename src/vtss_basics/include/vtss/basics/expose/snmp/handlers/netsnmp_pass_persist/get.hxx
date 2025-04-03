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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_NETSNMP_PASS_PERSIST_GET_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_NETSNMP_PASS_PERSIST_GET_HXX__

#include "vtss/basics/expose/snmp/handlers/get_base.hxx"

namespace vtss {
namespace expose {
namespace snmp {

struct GetHandler : public GetHandlerBase<GetHandler> {
    GetHandler(OidSequence &o, OidSequence &i, ostream *out, bool n = false)
        : GetHandlerBase(o, i, n), out_(out) {}

    typedef GetHandler &Map_t;
    template <typename... Args>
    GetHandler &as_map(Args &&... args) {
        return *this;
    }

    GetHandler &as_tuple() { return *this; }
    GetHandler &as_ref() { return *this; }

    ostream *out_;
};

void serialize_get_table_ro_generic(GetHandler &g, StructBase &o);
void serialize_get_struct_ro_generic(GetHandler &g, StructBase &o);

template <typename... T>
void serialize(GetHandler &h, StructROBase<T...> &o) {
    serialize_get_struct_ro_generic(h, o);
}

template <typename... T>
void serialize(GetHandler &h, TableReadOnly<T...> &o) {
    serialize_get_table_ro_generic(h, o);
}

template <typename... T>
void serialize(GetHandler &h, TableReadWriteAddDelete<T...> &o) {
    serialize_get_table_ro_generic(h, o);
}

template <typename INTERFACE>
void serialize(GetHandler &h, StructROBase2<INTERFACE> &o) {
    serialize_get_struct_ro_generic(h, o);
}

template <typename INTERFACE>
void serialize(GetHandler &h, StructRoTrap<INTERFACE> &o) {
    serialize_get_struct_ro_generic(h, o);
}

template <typename INTERFACE>
void serialize(GetHandler &h, StructRoSubject<INTERFACE> &o) {
    serialize_get_struct_ro_generic(h, o);
}

template <typename INTERFACE>
void serialize(GetHandler &h, TableReadOnly2<INTERFACE> &o) {
    serialize_get_table_ro_generic(h, o);
}

template <typename INTERFACE>
void serialize(GetHandler &h, TableReadOnlyTrap<INTERFACE> &o) {
    serialize_get_table_ro_generic(h, o);
}

template <typename INTERFACE>
void serialize(GetHandler &h, TableReadOnlySubject<INTERFACE> &o) {
    serialize_get_table_ro_generic(h, o);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_NETSNMP_PASS_PERSIST_GET_HXX__
