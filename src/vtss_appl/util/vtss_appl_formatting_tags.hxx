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

#ifndef _VTSS_APPL_FORMATTING_TAGS_HXX__
#define _VTSS_APPL_FORMATTING_TAGS_HXX__

#include "vtss/appl/interface.h"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/formatting_tags.hxx"
#include "vtss_appl_serialize.hxx"

namespace vtss {
struct AsInterfaceIndex {
    AsInterfaceIndex(vtss_ifindex_t &v) : val(v) {}
    vtss_ifindex_t &val;
};

// TODO, delete this!
struct AsPortList {
    AsPortList(BOOL *val, size_t size) : val_(val), size_(size) {}
    BOOL *val_;
    size_t size_;
};

template <typename T>
struct AsDscp {
    AsDscp(T &v) : val(v) {}
    T &val;
};

struct AsVlan {
    AsVlan(uint16_t &v) : val(v) {}
    mesa_vid_t &val;
};

struct AsVlanOrZero {
    AsVlanOrZero(uint16_t &v) : val(v) {}
    mesa_vid_t &val;
};

struct AsVlanList {
    AsVlanList(uint8_t *d, uint32_t s) : data_(d), size_(s) {}
    uint8_t *data_;
    uint32_t size_;
};

struct AsVlanListQuarter {
    AsVlanListQuarter(uint8_t *d, uint32_t s) : data_(d), size_(s) {}
    uint8_t *data_;
    uint32_t size_;
};

struct AsEtherType {
    AsEtherType (uint16_t &v) : val(v) {}
    uint16_t &val;
};

struct AsPsecUserBitmaskType {
    AsPsecUserBitmaskType (uint32_t &v) : val(v) {}
    uint32_t &val;
};

struct AsStdDisplayString {
    AsStdDisplayString(std::string &v, uint32_t s) : val(v) , size_(s) {}
    std::string &val;
    uint32_t size_;
};

struct AsTextualTimestamp {
    // Implement timestamp in format YYYY/MM/DD hh:mm:ss.sssssssss
    AsTextualTimestamp(mesa_timestamp_t &stamp) : val(stamp) {}
    mesa_timestamp_t &val;
};

// 64-bit (unsigned) integers cannot be represented accurately in JavaScript,
// because the underlying type is a double-precision float, which only supports
// accurate numbers smaller than 2^53.
// Newer browsers do, however, support a type called BigInt, which allows for
// accurately representing 64-bit numbers and perform computations on them, if
// needed.
// When using this type in your serializer, a 64-bit number is converted to a
// string of up to 20 characters + a terminating null, and vice versa in the
// deserializer.
//
// When used with an SNMP serializer, it will simply become an VTSSUnsigned64.
struct AsBigInt {
    AsBigInt(uint64_t &v) : val(v) {}
    uint64_t &val;
};

// Portlist
#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsPortList &p);
void serialize(expose::snmp::SetHandler &h, AsPortList &p);
void serialize(expose::snmp::Reflector &h, AsPortList &p);
void serialize(expose::snmp::TrapHandler &h, AsPortList &p);
#endif  // VTSS_SW_OPTION_JSON_RPC
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsPortList ifidx);
void serialize(expose::json::Loader &e, AsPortList ifidx);
void serialize(expose::json::HandlerReflector &e, AsPortList ifidx);
#endif  // VTSS_SW_OPTION_JSON_RPC

// Dscp
#ifdef VTSS_SW_OPTION_SNMP
template <typename T>
void serialize(expose::snmp::GetHandler   &h, AsDscp<T> &s) {
    int32_t v = s.val;
    serialize(h, v);
}
    
template <typename T>
void serialize(expose::snmp::SetHandler   &a, AsDscp<T> &s) {
    int32_t v = s.val;
    serialize(a, v);
    if (v < 0 || v > 63) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << v;
        a.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }
    s.val = v;
}

template <typename T>
void serialize(expose::snmp::OidImporter  &a, AsDscp<T> &s) {
    uint32_t x;

    if (!a.consume(x)) {
        a.flag_error();
        return;
    }

    if (x > 63) {
        if (a.next_request()) {
            x = 63;
            a.flag_overflow();
        } else {
            a.flag_error();
        }
    }

    s.val = x;
}

template <typename T>
void serialize(expose::snmp::OidExporter  &a, AsDscp<T> &s) {
    int32_t x = s.val;
    serialize(a, x);
}

template <typename T>
void serialize(expose::snmp::Reflector &h, AsDscp<T> &s) {
    h.type_ref("Dscp", "DIFFSERV-DSCP-TC", vtss::expose::snmp::AsnType::Integer);
}
#endif  // VTSS_SW_OPTION_JSON_RPC
#ifdef VTSS_SW_OPTION_JSON_RPC
template <typename T>
void serialize(expose::json::Exporter &e, AsDscp<T> d) {
    serialize(e, d.val);
}

template <typename T>
void serialize(expose::json::Loader &e, AsDscp<T> d) {
    serialize(e, d.val); }

template <typename T>
void serialize(expose::json::HandlerReflector &e, AsDscp<T> d) {
    serialize(e, d.val);
}
#endif  // VTSS_SW_OPTION_JSON_RPC

// Vlan
#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler   &h, AsVlan &s);
void serialize(expose::snmp::SetHandler   &a, AsVlan &s);
void serialize(expose::snmp::OidImporter  &a, AsVlan &s);
void serialize(expose::snmp::OidExporter  &a, AsVlan &s);
void serialize(expose::snmp::Reflector    &h, AsVlan &s);
void serialize(expose::snmp::TrapHandler  &h, AsVlan &s);
#endif  // VTSS_SW_OPTION_JSON_RPC
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsVlan a);
void serialize(expose::json::Loader &e, AsVlan a);
void serialize(expose::json::HandlerReflector &e, AsVlan a);
#endif  // VTSS_SW_OPTION_JSON_RPC

// VlanOrZero
#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler   &h, AsVlanOrZero &s);
void serialize(expose::snmp::SetHandler   &a, AsVlanOrZero &s);
void serialize(expose::snmp::OidImporter  &a, AsVlanOrZero &s);
void serialize(expose::snmp::OidExporter  &a, AsVlanOrZero &s);
void serialize(expose::snmp::Reflector    &h, AsVlanOrZero &s);
void serialize(expose::snmp::TrapHandler  &h, AsVlanOrZero &s);
#endif  // VTSS_SW_OPTION_JSON_RPC
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter         &e, AsVlanOrZero a);
void serialize(expose::json::Loader           &e, AsVlanOrZero a);
void serialize(expose::json::HandlerReflector &e, AsVlanOrZero a);
#endif  // VTSS_SW_OPTION_JSON_RPC

// Vlan list
#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler   &h, AsVlanListQuarter &s);
void serialize(expose::snmp::SetHandler   &a, AsVlanListQuarter &s);
void serialize(expose::snmp::Reflector    &h, AsVlanListQuarter &s);
void serialize(expose::snmp::TrapHandler  &h, AsVlanListQuarter &s);
#endif  // VTSS_SW_OPTION_JSON_RPC
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsVlanList &s);
void serialize(expose::json::Loader &e, AsVlanList &s);
void serialize(expose::json::HandlerReflector &e, AsVlanList &s);
#endif  // VTSS_SW_OPTION_JSON_RPC

// AsEtherType
#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler   &h, AsEtherType &s);
void serialize(expose::snmp::SetHandler   &a, AsEtherType &s);
void serialize(expose::snmp::Reflector    &h, AsEtherType &s);
void serialize(expose::snmp::TrapHandler  &h, AsEtherType &s);
template <typename RANGE_SPEC>
bool range_check(AsEtherType i, RANGE_SPEC *t) {
    return i.val >= t->from && i.val <= t->to;
}
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsEtherType s);
void serialize(expose::json::Loader &e, AsEtherType s);
void serialize(expose::json::HandlerReflector &e, AsEtherType s);
#endif  // VTSS_SW_OPTION_JSON_RPC

// AsPsecUserBitmaskType
#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsPsecUserBitmaskType &s);
void serialize(expose::snmp::SetHandler &h, AsPsecUserBitmaskType &s);
void serialize(expose::snmp::Reflector  &h, AsPsecUserBitmaskType &s);
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter         &e, AsPsecUserBitmaskType s);
void serialize(expose::json::Loader           &e, AsPsecUserBitmaskType s);
void serialize(expose::json::HandlerReflector &e, AsPsecUserBitmaskType s);
#endif  // VTSS_SW_OPTION_JSON_RPC

// AsInterfaceIndex
ostream& operator<<(ostream &o, const AsInterfaceIndex &rhs);
#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsInterfaceIndex &s);
void serialize(expose::snmp::SetHandler &a, AsInterfaceIndex &s);
void serialize(expose::snmp::OidImporter &a, AsInterfaceIndex &s);
void serialize(expose::snmp::OidExporter &a, AsInterfaceIndex &s);
void serialize(expose::snmp::Reflector &h, AsInterfaceIndex &);
void serialize(expose::snmp::TrapHandler &h, AsInterfaceIndex &);
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsInterfaceIndex ifidx);
void serialize(expose::json::Loader &e, AsInterfaceIndex ifidx);
void serialize(expose::json::HandlerReflector &e, AsInterfaceIndex ifidx);
#endif  // VTSS_SW_OPTION_JSON_RPC

bool parse(const char *&b, const char *e, AsInterfaceIndex &x);
#ifdef VTSS_SW_OPTION_JSON_RPC
bool enum_element_parse(const char *&b, const char *e,
                        const ::vtss_enum_descriptor_t *descriptor,
                        int &value);
#endif  // VTSS_SW_OPTION_JSON_RPC

// AsStdDisplayString
#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsStdDisplayString &p);
void serialize(expose::snmp::SetHandler &h, AsStdDisplayString &p);
void serialize(expose::snmp::Reflector &h, AsStdDisplayString &p);
#endif  // VTSS_SW_OPTION_SNMP

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, vtss::AsStdDisplayString s);
void serialize(expose::json::Loader &e, vtss::AsStdDisplayString s);
void serialize(expose::json::HandlerReflector &e, vtss::AsStdDisplayString s);
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsTextualTimestamp &p);
void serialize(expose::snmp::SetHandler &h, AsTextualTimestamp &p);
void serialize(expose::snmp::Reflector  &h, AsTextualTimestamp &p);
#endif  // VTSS_SW_OPTION_SNMP

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter         &e, vtss::AsTextualTimestamp s);
void serialize(expose::json::Loader           &e, vtss::AsTextualTimestamp s);
void serialize(expose::json::HandlerReflector &e, vtss::AsTextualTimestamp s);
#endif

#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsBigInt &p);
void serialize(expose::snmp::SetHandler &h, AsBigInt &p);
void serialize(expose::snmp::Reflector  &h, AsBigInt &p);
#endif  // VTSS_SW_OPTION_SNMP

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter         &e, vtss::AsBigInt s);
void serialize(expose::json::Loader           &e, vtss::AsBigInt s);
void serialize(expose::json::HandlerReflector &e, vtss::AsBigInt s);
#endif
}  // namespace vtss

vtss::ostream& operator<<(vtss::ostream &o, const vtss_ifindex_t &rhs);
#ifdef VTSS_SW_OPTION_SNMP
void serialize(vtss::expose::snmp::GetHandler  &h, vtss_ifindex_t &s);
void serialize(vtss::expose::snmp::SetHandler  &a, vtss_ifindex_t &s);
void serialize(vtss::expose::snmp::OidImporter &a, vtss_ifindex_t &s);
void serialize(vtss::expose::snmp::OidExporter &a, vtss_ifindex_t &s);
void serialize(vtss::expose::snmp::Reflector   &h, vtss_ifindex_t &);
void serialize(vtss::expose::snmp::TrapHandler &h, vtss_ifindex_t &);
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(vtss::expose::json::Exporter &e, vtss_ifindex_t ifidx);
void serialize(vtss::expose::json::Loader &e, vtss_ifindex_t &ifidx);
void serialize(vtss::expose::json::HandlerReflector &e, vtss_ifindex_t ifidx);
#endif  // VTSS_SW_OPTION_JSON_RPC

// Portlist
#ifdef VTSS_SW_OPTION_SNMP
void serialize(vtss::expose::snmp::GetHandler &h, mesa_port_list_t &p);
void serialize(vtss::expose::snmp::SetHandler &h, mesa_port_list_t &p);
void serialize(vtss::expose::snmp::Reflector &h, mesa_port_list_t &p);
void serialize(vtss::expose::snmp::TrapHandler &h, mesa_port_list_t &p);
#endif  // VTSS_SW_OPTION_JSON_RPC
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(vtss::expose::json::Exporter &e, mesa_port_list_t &p);
void serialize(vtss::expose::json::Loader &e, mesa_port_list_t &p);
void serialize(vtss::expose::json::HandlerReflector &e, mesa_port_list_t &p);
#endif  // VTSS_SW_OPTION_JSON_RPC


#endif  // _VTSS_APPL_FORMATTING_TAGS_HXX__
