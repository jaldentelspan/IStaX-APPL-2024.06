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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_GET_BASE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_GET_BASE_HXX__

#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/snmp/handlers/getset_base.hxx"
#include "vtss/basics/expose/snmp/pre-get-condition.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template<typename Child>
struct GetHandlerBase : public GetSetHandlerCommon {
    static constexpr bool need_all_elements = false;
    static constexpr bool is_importer = false;
    static constexpr bool is_exporter = true;

    GetHandlerBase(OidSequence &s, OidSequence &i, bool n) :
                GetSetHandlerCommon(s, i, n) { }

    template <typename... Args>
    void argument_properties(const Args... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);
        OidOffset *offset = argpack.template get_optional<OidOffset>();
        if (offset) oid_offset_ += offset->i;
    }

    void argument_begin(size_t n, ArgumentType::E type) {}
    void argument_properties_clear() { oid_offset_ = 0; }

    template<typename T, typename... Args>
    void add_leaf(T &&value, const Args... args) {
        typedef SnmpType<typename meta::BaseType<T>::type> SNMP_TYPE;
        // Build the argument pack, which allows the position independent
        // interface
        vtss::meta::VAArgs<Args...> argpack(args...);

        // Calculate the OID.
        uint32_t oid = argpack.template get<OidElementValue>().i + oid_offset_;

        // A wrapper class to call the serialize function through.
        SerializeClass<Child, T, typename SNMP_TYPE::TypeClass> s;

        // Do not waste any time if the request has already bee handled
        if (state() != HandlerState::SEARCHING)
            return;

        // Test and consume an OID from the input sequence
        if (!consume_oid_leaf(oid))
            return;

        if (check_pre_get_condition(argpack)) {
            s(static_cast<Child &>(*this), value);
        } else {
            state(HandlerState::AGAIN);
        }
    }

    template <typename T, typename... Args>
    void capability(const Args... args) {
        auto val = T::get();
        add_leaf(val,
                 vtss::tag::Name(T::name),
                 Status::Current,
                 vtss::tag::Description(T::desc),
                 args...);
    }


    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {}

    AsnType asn_type_;

};

void serialize(GetHandler &h, NamespaceNode &o);
void serialize_enum(GetHandler &h, int32_t &i, const vtss_enum_descriptor_t *d);
void serialize(GetHandler &h, uint64_t &s);
void serialize(GetHandler &h, uint32_t &s);
void serialize(GetHandler &h, uint16_t &s);
void serialize(GetHandler &h, uint8_t &s);
void serialize(GetHandler &h, int64_t &s);
void serialize(GetHandler &h, int32_t &s);
void serialize(GetHandler &h, int16_t &s);
void serialize(GetHandler &h, int8_t &s);
void serialize(GetHandler &h, bool &s);
void serialize(GetHandler &h, AsCounter &s);
void serialize(GetHandler &h, AsDisplayString &s);
void serialize(GetHandler &h, AsPasswordSetOnly &s);
void serialize(GetHandler &h, AsBitMask &s);
void serialize(GetHandler &h, AsOctetString &s);
void serialize(GetHandler &h, BinaryLen &s);
void serialize(GetHandler &h, BinaryU32Len &s);
void serialize(GetHandler &h, Ipv4Address &s);
void serialize(GetHandler &h, AsIpv4 &s);
void serialize(GetHandler &h, mesa_mac_t &s);
void serialize(GetHandler &h, mesa_ipv6_t &s);
void serialize(GetHandler &h, AsBool &s);
void serialize(GetHandler &h, AsRowEditorState &s);
void serialize(GetHandler &h, AsPercent &s);
void serialize(GetHandler &h, AsInt &s);
void serialize(GetHandler &h, AsSnmpObjectIdentifier &s);
void serialize(GetHandler &h, AsDecimalNumber &s);

inline void serialize(GetHandler &h, AsTimeStampSeconds &s) {
    serialize(h, s.t);
}

void serialize(GetHandler &h, AsUnixTimeStampSeconds &s);

template<typename ...T>
void serialize(GetHandler &h, StructROBase<T...> &o);

template<typename ...T>
void serialize(GetHandler &g, TableReadOnly<T...> &o);

template<typename ...T>
void serialize(GetHandler &g, TableReadWriteAddDelete<T...> &o);

template <typename T>
void serialize(GetHandler &h, StructROBase2<T> &o);

template <typename T>
void serialize(GetHandler &h, StructRoTrap<T> &o);

template <typename T>
void serialize(GetHandler &h, StructRoSubject<T> &o);

template<typename T>
void serialize(GetHandler &g, TableReadOnly2<T> &o);

template <typename T>
void serialize(GetHandler &h, TableReadOnlyTrap<T> &o);

template <typename T>
void serialize(GetHandler &h, TableReadOnlySubject<T> &o);


#define RANGE_CHECK_IMPL(X)            \
template <typename RANGE_SPEC>         \
bool range_check(X i, RANGE_SPEC *t) { \
    return i >= t->from && i <= t->to; \
}

RANGE_CHECK_IMPL(uint8_t);
RANGE_CHECK_IMPL(uint16_t);
RANGE_CHECK_IMPL(uint32_t);

RANGE_CHECK_IMPL(int8_t);
RANGE_CHECK_IMPL(int16_t);
RANGE_CHECK_IMPL(int32_t);

#undef RANGE_CHECK_IMPL

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_GET_BASE_HXX__
