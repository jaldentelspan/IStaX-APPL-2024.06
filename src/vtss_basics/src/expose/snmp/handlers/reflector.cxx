/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss/basics/expose/snmp/handlers/reflector.hxx"

namespace vtss {
namespace expose {
namespace snmp {

// Implement serializers ///////////////////////////////////////////////////////
void serialize(Reflector &g, NamespaceNode &o) {
    g.node_begin(o.element(), nullptr);
    for (auto i = o.leafs.begin(); i != o.leafs.end(); ++i) serialize(g, *i);
    g.node_end(nullptr);
}

void serialize(Reflector &h, AsBitMask &ds) {
    h.type_ref("OCTET STRING", nullptr, AsnType::OctetString);
}

void serialize(Reflector &h, AsBool &) {
    h.type_ref("TruthValue", "SNMPv2-TC", AsnType::Integer);
}

void serialize(Reflector &h, AsCounter &) {
    h.type_ref("Counter64", "SNMPv2-SMI", AsnType::Counter64);
}

void serialize(Reflector &h, AsDisplayString &ds) {
    StringStream ss;
    ss << " (SIZE(" << ds.min_ << ".." << ds.size_ - 1 << "))";
    h.type_def("DisplayString", AsnType::OctetString);
    h.type_range(ss.buf);
}

void serialize(Reflector &h, AsInt &) {
    h.type_ref("Integer32", "SNMPv2-SMI", AsnType::Integer);
}

void serialize(Reflector &h, AsIpv4 &val) {
    h.type_ref("IpAddress", "SNMPv2-SMI", AsnType::IpAddress);
}

void serialize(Reflector &h, AsOctetString &val) {
    StringStream ss;
    ss << " (SIZE(" << val.size_ << "))";
    h.type_ref("OCTET STRING", nullptr, AsnType::OctetString);
    h.type_range(ss.buf);
}

void serialize(Reflector &h, AsPasswordSetOnly &ds) {
    h.type_def("DisplayString", AsnType::OctetString);
}

void serialize(Reflector &h, AsPercent &) {
    h.type_def("Percent", AsnType::Unsigned);
}

void serialize(Reflector &h, AsRowEditorState &) {
    h.type_def("RowEditorState", AsnType::Unsigned);
}

void serialize(Reflector &h, AsSnmpObjectIdentifier &val) {
    h.type_ref("OBJECT IDENTIFIER", nullptr, AsnType::ObjectIdentifier);
}

void serialize(Reflector &h, AsDecimalNumber &val) {
    h.type_def(val.snmp_type_name(), AsnType::Integer);
}

void serialize(Reflector &h, AsTimeStampSeconds &v) {
    h.type_def("TimeStamp", AsnType::OctetString);
}

void serialize(Reflector &h, AsUnixTimeStampSeconds &v) {
    h.type_ref("DateAndTime", "SNMPv2-TC", AsnType::OctetString);
}

void serialize(Reflector &h, BinaryLen &val) {
    StringStream ss;
    if (val.min_len == val.max_len) {
        ss << " (SIZE(" << val.min_len << "))";
    } else {
        ss << " (SIZE(" << val.min_len << ".." << val.max_len << "))";
    }
    h.type_ref("OCTET STRING", nullptr, AsnType::OctetString);
    h.type_range(ss.buf);
}

void serialize(Reflector &h, BinaryU32Len &val) {
    StringStream ss;
    if (val.min_len == val.max_len) {
        ss << " (SIZE(" << val.min_len * 4 << "))";
    } else {
        ss << " (SIZE(" << val.min_len * 4 << ".." << val.max_len * 4 << "))";
    }
    h.type_ref("OCTET STRING", nullptr, AsnType::OctetString);
    h.type_range(ss.buf);
}

void serialize(Reflector &h, Ipv4Address &val) {
    h.type_ref("IpAddress", "SNMPv2-SMI", AsnType::IpAddress);
}

void serialize(Reflector &h, bool &) {
    h.type_ref("TruthValue", "SNMPv2-TC", AsnType::Integer);
}

void serialize(Reflector &h, int16_t &) {
    h.type_def("Integer16", AsnType::Integer);
}

void serialize(Reflector &h, int32_t &) {
    h.type_ref("Integer32", "SNMPv2-SMI", AsnType::Integer);
}

void serialize(Reflector &h, int64_t &) {
    h.type_def("Integer64", AsnType::OctetString);
}

void serialize(Reflector &h, int8_t &) {
    h.type_def("Integer8", AsnType::Integer);
}

void serialize(Reflector &h, uint16_t &) {
    h.type_def("Unsigned16", AsnType::Unsigned);
}

void serialize(Reflector &h, uint32_t &) {
    h.type_ref("Unsigned32", "SNMPv2-SMI", AsnType::Unsigned);
}

void serialize(Reflector &h, uint64_t &v) {
    h.type_def("Unsigned64", AsnType::OctetString);
}

void serialize(Reflector &h, uint8_t &) {
    h.type_def("Unsigned8", AsnType::Unsigned);
}

void serialize(Reflector &h, mesa_ipv6_t &val) {
    h.type_ref("InetAddressIPv6", "INET-ADDRESS-MIB", AsnType::OctetString);
}

void serialize(Reflector &h, mesa_mac_t &val) {
    h.type_ref("MacAddress", "SNMPv2-TC", AsnType::OctetString);
}

void serialize_enum(Reflector &h, const char *name, const char *desc,
                    const vtss_enum_descriptor_t *d) {
    h.type_enum(name, desc, d);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
