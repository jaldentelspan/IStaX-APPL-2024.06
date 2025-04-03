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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_LINUX_SET_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_LINUX_SET_HXX__

#include "vtss/basics/predefs.hxx"
#if defined(VTSS_OPSYS_LINUX)

#include "vtss/basics/expose/snmp/handlers/set_base.hxx"

namespace vtss {
namespace expose {
namespace snmp {

bool check_length(SetHandler &a, size_t l);
bool check_type(SetHandler &a, AsnType::E expect);

namespace Action {
enum E {
    reserve1 = 0,
    reserve2 = 1,
    action = 2,
    commit = 3,
    free = 4,
    undo = 5,
    finished_success = 9,
    finished_failure = 10,
};

ostream &operator<<(ostream &o, const E e);
}  // namespace Action

#define VTSS_SNMP_SET_CACHE_SIZE 128
extern SetCacheEntry set_cache[VTSS_SNMP_SET_CACHE_SIZE];

struct SetHandler : public SetHandlerBase<SetHandler> {
    SetHandler(OidSequence &o, OidSequence &i, uint8_t *v, size_t l, AsnType t)
        : SetHandlerBase(o, i), val(v), val_length(l), asn_type(t) {}

    IteratorCommon *cache_loopup(const OidSequence &oid);
    bool cache_add(const OidSequence &oid, IteratorCommon *i);

    typedef SetHandler &Map_t;
    template <typename... Args>
    SetHandler &as_map(Args &&... args) {
        return *this;
    }

    uint8_t *val;
    size_t val_length;
    AsnType asn_type;
    OidSequence sample_oid;
};

void serialize_set_struct_ro_generic(SetHandler &h, StructBase &o);
void serialize_set_struct_rw_generic(SetHandler &h, StructBase &o);
void serialize_set_table_rw_generic(SetHandler &h, StructBase &o);


template <typename... T>
void serialize(SetHandler &h, StructROBase<T...> &o) {
    serialize_set_struct_ro_generic(h, o);
}

template <typename INTERFACE>
void serialize(SetHandler &h, StructROBase2<INTERFACE> &o) {
    serialize_set_struct_ro_generic(h, o);
}

template <typename INTERFACE>
void serialize(SetHandler &h, StructRoTrap<INTERFACE> &o) {}

template <typename INTERFACE>
void serialize(SetHandler &h, StructRoSubject<INTERFACE> &o) {}

template <typename... T>
void serialize(SetHandler &h, StructRWBase<T...> &o) {
    serialize_set_struct_rw_generic(h, o);
}

template <typename INTERFACE>
void serialize(SetHandler &h, StructRWBase2<INTERFACE> &o) {
    serialize_set_struct_rw_generic(h, o);
}

template <typename... T>
void serialize(SetHandler &g, TableReadOnly<T...> &o) {}

template <typename INTERFACE>
void serialize(SetHandler &g, TableReadOnly2<INTERFACE> &o) {}

template <typename INTERFACE>
void serialize(SetHandler &h, TableReadOnlyTrap<INTERFACE> &o) {}

template <typename INTERFACE>
void serialize(SetHandler &h, TableReadOnlySubject<INTERFACE> &o) {}

template <typename... T>
void serialize(SetHandler &h, TableReadWrite<T...> &o) {
    serialize_set_table_rw_generic(h, o);
}

template <typename... T>
void serialize(SetHandler &h, TableReadWriteAddDelete<T...> &o) {
    serialize_set_table_rw_generic(h, o);
}

template <typename INTERFACE>
void serialize(SetHandler &h, TableReadWrite2<INTERFACE> &o) {
    serialize_set_table_rw_generic(h, o);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // VTSS_OPSYS_LINUX
#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_LINUX_SET_HXX__
