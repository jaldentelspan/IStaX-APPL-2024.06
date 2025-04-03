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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_OID_HANDLER_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_OID_HANDLER_HXX__

#include "vtss/basics/types.hxx"
#include "vtss/basics/expose/snmp/oid_sequence.hxx"
#include "vtss/basics/expose/snmp/oid-offset.hxx"
#include "vtss/basics/expose/snmp/type-classes.hxx"
#include "vtss/basics/enum-descriptor.h"
#include "vtss/basics/meta-data-packet.hxx"

namespace vtss {
namespace expose {
namespace snmp {

struct OidImporter {
    struct Mode {
        enum E { Normal, Overflow, Incomplete };
    };

    OidImporter(const OidSequence &o, bool next, Mode::E m = Mode::Normal);
    OidImporter(const OidImporter &) = delete;

    template <typename... Args>
    void argument_properties(const Args... args) {
        meta::VAArgs<Args...> argpack(args...);
        OidOffset *offset = argpack.template get_optional<OidOffset>();
        if (offset) oid_offset_ += offset->i;
    }

    void argument_properties_clear() { oid_offset_ = 0; }

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args... args) {
        typedef typename meta::BaseType<T>::type BT;
        SerializeClass<OidImporter, T, typename SnmpType<BT>::TypeClass> s;
        s(*this, value);
    }

    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {}

    typedef OidImporter &Map_t;
    template <typename... Args>
    OidImporter &as_map(Args &&... args) {
        return *this;
    }

    OidImporter &as_tuple() { return *this; }
    OidImporter &as_ref() { return *this; }

    bool consume(uint32_t &i);
    bool next_request() { return next_request_; }
    void flag_error() { ok_ = false; }
    bool all_consumed() { return oids_.valid == 0; }
    void flag_overflow();

    bool force_get_first() const;

    bool ok_ = true;
    uint32_t oid_offset_ = 0;

  private:
    OidSequence oids_;
    Mode::E mode_;
    bool next_request_ = false;
};

struct OidExporter {
    template <typename... Args>
    void argument_properties(const Args... args) {
        meta::VAArgs<Args...> argpack(args...);
        OidOffset *offset = argpack.template get_optional<OidOffset>();
        if (offset) oid_offset_ += offset->i;
    }

    void argument_properties_clear() { oid_offset_ = 0; }

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args... args) {
        typedef typename meta::BaseType<T>::type BT;
        SerializeClass<OidExporter, T, typename SnmpType<BT>::TypeClass> s;
        s(*this, value);
    }

    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {}

    typedef OidExporter &Map_t;
    template <typename... Args>
    OidExporter &as_map(Args &&... args) {
        return *this;
    }

    OidExporter &as_tuple() { return *this; }
    OidExporter &as_ref() { return *this; }

    void flag_error() { ok_ = false; }

    bool ok_ = true;
    OidSequence oids_;
    uint32_t oid_offset_ = 0;
};


// void serialize(OidImporter &a, uint64_t  &s);
void serialize(OidImporter &a, uint32_t &s);
void serialize(OidImporter &a, uint16_t &s);
void serialize(OidImporter &a, uint8_t &s);
void serialize(OidImporter &a, int32_t &s);
void serialize(OidImporter &a, bool &s);
void serialize(OidImporter &a, mesa_mac_t &s);
void serialize(OidImporter &a, mesa_ipv6_t &s);
void serialize_enum(OidImporter &h, int32_t &i, const vtss_enum_descriptor_t *d);
void serialize(OidImporter &h, Ipv4Address &s);
void serialize(OidImporter &h, AsIpv4 &s);
void serialize(OidImporter &a, AsRowEditorState &s);
void serialize(OidImporter &a, AsPercent &s);
void serialize(OidImporter &a, AsInt &s);
void serialize(OidImporter &a, AsDisplayString &s);
void serialize(OidImporter &a, BinaryLen &s);
void serialize(OidImporter &a, AsSnmpObjectIdentifier &s);

// void serialize(OidExporter &a, uint64_t  &s);
void serialize(OidExporter &a, uint32_t &s);
void serialize(OidExporter &a, uint16_t &s);
void serialize(OidExporter &a, uint8_t &s);
void serialize(OidExporter &a, int32_t &s);
void serialize(OidExporter &a, bool &s);
void serialize(OidExporter &a, mesa_mac_t &s);
void serialize(OidExporter &a, mesa_ipv6_t &s);
void serialize_enum(OidExporter &h, int32_t &i, const vtss_enum_descriptor_t *d);
void serialize(OidExporter &h, Ipv4Address &s);
void serialize(OidExporter &h, AsIpv4 &s);
void serialize(OidExporter &a, AsRowEditorState &s);
void serialize(OidExporter &a, AsPercent &s);
void serialize(OidExporter &a, AsInt &s);
void serialize(OidExporter &a, AsDisplayString &s);
void serialize(OidExporter &a, BinaryLen &s);
void serialize(OidExporter &a, AsSnmpObjectIdentifier &s);

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_OID_HANDLER_HXX__
