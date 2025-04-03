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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_SET_BASE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_SET_BASE_HXX__

#include "vtss/basics/intrusive_list.hxx"
#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/snmp/handlers/getset_base.hxx"
#include "vtss/basics/expose/snmp/post-set-action.hxx"

namespace vtss {
namespace expose {
namespace snmp {

struct SetCacheEntry {
    SetCacheEntry() {}
    SetCacheEntry(const OidSequence &o, IteratorCommon *i)
        : oid(o), iterator(i) {}

    OidSequence oid;
    unique_ptr<IteratorCommon> iterator;
};

template <typename Child>
class SetHandlerBase : public GetSetHandlerCommon {
  public:
    static constexpr bool need_all_elements = false;
    static constexpr bool is_importer = false;
    static constexpr bool is_exporter = true;

    SetHandlerBase(OidSequence &s, OidSequence &i)
        : GetSetHandlerCommon(s, i, false) {}

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args... args) {
        typedef SnmpType<typename meta::BaseType<T>::type> SNMP_TYPE;
        // Build the argument pack, which allows the position independent
        // interface
        vtss::meta::VAArgs<Args...> argpack(args...);

        // Calculate the OID.
        uint32_t oid = argpack.template get<OidElementValue>().i + oid_offset_;

        // A wrapper class to call the serialize function through.
        SerializeClass<Child, T, typename SNMP_TYPE::TypeClass> s;

        if (state() != HandlerState::SEARCHING) return;

        // Test and consume an OID from the input sequence
        if (!consume_oid_leaf(oid)) return;

        s(static_cast<Child &>(*this), value);
        invoke_post_set_action(argpack);

        if (state() == HandlerState::DONE &&
            (!range_spec_check(value, argpack)))
            error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
    }

    template <typename T, typename... Args>
    void capability(const Args... args) {
        // capabilities are per definition read-only and should be ignored here
    }

    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {}

    void argument_begin(size_t n, ArgumentType::E type) {}
    template <typename... Args>
    void argument_properties(const Args... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);
        OidOffset *offset = argpack.template get_optional<OidOffset>();
        if (offset) oid_offset_ += offset->i;
    }

    void argument_properties_clear() { oid_offset_ = 0; }
};

void serialize(SetHandler &g, NamespaceNode &o);
void serialize_enum(SetHandler &h, int32_t &t, const vtss_enum_descriptor_t *d);
void serialize(SetHandler &a, uint64_t &s);
void serialize(SetHandler &a, uint32_t &s);
void serialize(SetHandler &a, uint16_t &s);
void serialize(SetHandler &a, uint8_t &s);
void serialize(SetHandler &a, int64_t &s);
void serialize(SetHandler &a, int32_t &s);
void serialize(SetHandler &a, int16_t &s);
void serialize(SetHandler &a, int8_t &s);
void serialize(SetHandler &a, bool &s);
void serialize(SetHandler &a, AsCounter &s);
void serialize(SetHandler &a, AsDisplayString &s);
void serialize(SetHandler &a, AsPasswordSetOnly &s);
void serialize(SetHandler &a, AsBitMask &s);
void serialize(SetHandler &a, AsOctetString &s);
void serialize(SetHandler &a, BinaryLen &s);
void serialize(SetHandler &a, BinaryU32Len &s);
void serialize(SetHandler &a, Ipv4Address &s);
void serialize(SetHandler &a, AsIpv4 &s);
void serialize(SetHandler &a, mesa_mac_t &s);
void serialize(SetHandler &a, mesa_ipv6_t &s);
void serialize(SetHandler &a, AsBool &s);
void serialize(SetHandler &a, AsRowEditorState &s);
void serialize(SetHandler &a, AsPercent &s);
void serialize(SetHandler &a, AsInt &s);
void serialize(SetHandler &a, AsSnmpObjectIdentifier &s);
void serialize(SetHandler &a, AsDecimalNumber &s);

inline void serialize(SetHandler &h, AsTimeStampSeconds &s) {
    serialize(h, s.t);
}

template <typename... T>
void serialize(SetHandler &g, TableReadOnly<T...> &o);
template <typename... T>
void serialize(SetHandler &g, TableReadWrite<T...> &o);
template <typename... T>
void serialize(SetHandler &g, TableReadWriteAddDelete<T...> &o);

template <typename T>
void serialize(SetHandler &g, TableReadOnly2<T> &o);

inline void serialize(SetHandler &h, AsUnixTimeStampSeconds &s) {
    assert(0);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_SET_BASE_HXX__
