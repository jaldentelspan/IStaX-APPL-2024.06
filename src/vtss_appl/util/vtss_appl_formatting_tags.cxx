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

#include "main.h"
#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "misc_api.h"
#include "icli_api.h"
#include "topo_api.h"
#include "vtss/appl/vlan.h"
#if defined(VTSS_SW_OPTION_PTP)
#include "vtss_tod_api.h"
#endif
#include "vtss/basics/parse_group.hxx"
#include "vtss/basics/trace_basics.hxx"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_appl_expose_json_array.hxx"
#include "vtss/basics/expose/json/literal.hxx"
#include "vtss/basics/expose/json/serialize.hxx"
#include "vtss/basics/expose/json/string-literal.hxx"
#include "vtss/basics/expose/json/parse-and-compare-no-qoutes.hxx"

#define TRACE_JSON(X) VTSS_BASICS_TRACE(113, VTSS_BASICS_TRACE_GRP_JSON, X)

#define EXPECT_TYPE(EXPECT) \
    if (!check_type(a, EXPECT)) return;
#define EXPECT_LENGTH(EXPECT) \
    if (!check_length(a, EXPECT)) return;

#define MAX_OCTET_STRING_SIZE 256
#ifdef VTSS_SW_OPTION_SNMP
static char GLOBAL_DISPLAY_STRING[MAX_OCTET_STRING_SIZE];
#endif  // VTSS_SW_OPTION_SNMP

namespace vtss {
#ifdef VTSS_SW_OPTION_SNMP
void serialize(vtss::expose::snmp::GetHandler &a, AsPortList &p) {
    VTSS_ASSERT(sizeof(GLOBAL_DISPLAY_STRING) >= 8);
    const uint32_t bit_length = 8 * 8;

    // Clear output area as the port list may not be continuer
    for (int i = 0; i < 8; ++i) GLOBAL_DISPLAY_STRING[i] = 0;

    // Update the output data
    for (int i = 0; i < p.size_; ++i) {
        if (!p.val_[i]) continue;

        uint32_t uport = iport2uport(i);

        // No room for this port...
        if (uport >= bit_length) continue;

        uint32_t byte_index = uport / 8;
        uint32_t bit_index = uport % 8;
        uint8_t mask = 1 << bit_index;

        GLOBAL_DISPLAY_STRING[byte_index] |= mask;
    }

    // Truncate the list to only contain the active part
    a.length = 1;  // At least one byte must be included
    for (int i = 0; i < 8; ++i)
        if (GLOBAL_DISPLAY_STRING[i]) a.length = i + 1;

    a.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    a.state(expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::SetHandler &a, AsPortList &p) {
    EXPECT_TYPE(vtss::expose::snmp::AsnType::OctetString);
    if (a.val_length < 1 || a.val_length > 128) {
        a.error_code(vtss::expose::snmp::ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    // Variable length input is accepted, if input is larger than the allocated
    // resource, then trunkcate input
    const uint32_t bit_length = min(a.val_length * 8, p.size_ * 8);

    // Clear the array
    for (int i = 0; i < p.size_; ++i) p.val_[i] = 0;

    // Copy input value to p, while converting from uport2iport
    for (int i = 0; i < bit_length; ++i) {
        uint8_t val = a.val[i / 8];
        bool is_set = (val >> (i % 8)) & 1;

        if (!is_set) continue;

        uint32_t iport = uport2iport(i);

        if (iport < p.size_)  // Just ignore out-of-range requests
            p.val_[iport] = 1;
    }

    a.state(expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::Reflector &h, AsPortList &p) {
    h.type_def("PortList", vtss::expose::snmp::AsnType::OctetString);
}
#endif  // VTSS_SW_OPTION_SNMP

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsPortList ifidx) {
    ArrayExporter<AsInterfaceIndex> a(e);
    for (int i = 0; i < ifidx.size_; ++i) {
        if (ifidx.val_[i]) {
            vtss_ifindex_t ii;
            mesa_rc rc = vtss_ifindex_from_port(VTSS_ISID_LOCAL, i, &ii);

            if (rc == VTSS_RC_OK) {
                AsInterfaceIndex idx(ii);
                a.add(idx);
            } else {
                TRACE_JSON(DEBUG)
                        << "Could not convert port to ifindex: iport: " << i
                        << " rc: " << rc;
                e.flag_error();
                return;
            }
        }
    }
}

void serialize(expose::json::Loader &e, AsPortList ifidx) {
    vtss_ifindex_t idx_;
    AsInterfaceIndex idx(idx_);
    ArrayLoad<AsInterfaceIndex> a(e);

    for (int i = 0; i < ifidx.size_; ++i) ifidx.val_[i] = 0;

    while (a.get(idx)) {
        vtss_ifindex_elm_t ife;
        mesa_rc rc = vtss_ifindex_decompose(idx.val, &ife);

        if (rc != VTSS_RC_OK) {
            TRACE_JSON(DEBUG) << "vtss_ifindex_decompose: " << idx
                              << " rc: " << rc;
            e.flag_error();
            return;
        }

        if (ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
            TRACE_JSON(DEBUG) << "Expecting a port: " << VTSS_IFINDEX_TYPE_PORT
                              << " got: " << ife.iftype;
            e.flag_error();
            return;
        }

        int iport = ife.ordinal;
        if (iport < 0 || iport > ifidx.size_) {
            TRACE_JSON(DEBUG) << "Iport: " << iport << " is out of range";
            e.flag_error();
            return;
        }

        ifidx.val_[iport] = 1;
    }
}
void serialize(expose::json::HandlerReflector &e, AsPortList ifidx) {
    e.type_terminal(expose::json::JsonCoreType::Array,
                    "vtss_port_list_stackable_t", "vtss_port_list_stackable_t");
}

#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_SNMP
// Vlan -----------------------------------------------------------------------
void serialize(expose::snmp::GetHandler &a, AsVlan &s) { serialize(a, s.val); }

void serialize(expose::snmp::SetHandler &a, AsVlan &s) {
    serialize(a, s.val);

    if (s.val < VTSS_APPL_VLAN_ID_MIN || s.val > VTSS_APPL_VLAN_ID_MAX) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << s.val;
        a.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
    }
}

void serialize(expose::snmp::OidImporter &a, AsVlan &s) {
    int32_t x;
    serialize(a, x);

    if (x < VTSS_APPL_VLAN_ID_MIN || x > VTSS_APPL_VLAN_ID_MAX) {
        if (a.next_request()) {
            s.val = VTSS_APPL_VLAN_ID_MAX;
            a.flag_overflow();

        } else {
            a.flag_error();
        }
    } else {
        s.val = x;
    }
}

void serialize(expose::snmp::OidExporter &a, AsVlan &s) {
    int32_t x = s.val;
    serialize(a, x);
}

void serialize(expose::snmp::Reflector &a, AsVlan &s) {
    a.type_def("Vlan", vtss::expose::snmp::AsnType::Unsigned);
}
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsVlan a) { serialize(e, a.val); }
void serialize(expose::json::Loader &e, AsVlan a) { serialize(e, a.val); }
void serialize(expose::json::HandlerReflector &e, AsVlan a) {
    e.type_terminal(expose::json::JsonCoreType::Number, "mesa_vid_t",
                    "Vlan ID");
}
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_SNMP
// VlanOrZero -----------------------------------------------------------------
void serialize(expose::snmp::GetHandler &a, AsVlanOrZero &s) { serialize(a, s.val); }

void serialize(expose::snmp::SetHandler &a, AsVlanOrZero &s) {
    serialize(a, s.val);

    if (s.val && (s.val < VTSS_APPL_VLAN_ID_MIN || s.val > VTSS_APPL_VLAN_ID_MAX)) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << s.val;
        a.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
    }
}

void serialize(expose::snmp::OidImporter &a, AsVlanOrZero &s) {
    int32_t x;
    serialize(a, x);

    if (x && (x < VTSS_APPL_VLAN_ID_MIN || x > VTSS_APPL_VLAN_ID_MAX)) {
        if (a.next_request()) {
            s.val = VTSS_APPL_VLAN_ID_MAX;
            a.flag_overflow();

        } else {
            a.flag_error();
        }
    } else {
        s.val = x;
    }
}

void serialize(expose::snmp::OidExporter &a, AsVlanOrZero &s) {
    int32_t x = s.val;
    serialize(a, x);
}

void serialize(expose::snmp::Reflector &a, AsVlanOrZero &s) {
    a.type_def("VlanOrZero", vtss::expose::snmp::AsnType::Unsigned);
}

void serialize(expose::snmp::TrapHandler &a, AsVlanOrZero &s) {
    serialize(a, s.val);
}

#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsVlanOrZero a) { serialize(e, a.val); }
void serialize(expose::json::Loader &e, AsVlanOrZero a) { serialize(e, a.val); }
void serialize(expose::json::HandlerReflector &e, AsVlanOrZero a) {
    e.type_terminal(expose::json::JsonCoreType::Number, "mesa_vid_t", "Vlan ID");
}
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsVlanListQuarter &s) {
    static_assert(sizeof(GLOBAL_DISPLAY_STRING) >= 128, "size too small");

    memcpy(GLOBAL_DISPLAY_STRING, s.data_, 128);

    h.length = 128;
    h.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    h.state(expose::snmp::HandlerState::DONE);
}

void serialize(expose::snmp::SetHandler &a, AsVlanListQuarter &s) {
    EXPECT_TYPE(expose::snmp::AsnType::OctetString);
    EXPECT_LENGTH(128);

    memcpy(s.data_, a.val, 128);
    s.size_ = 128;

    a.state(expose::snmp::HandlerState::DONE);
}

void serialize(expose::snmp::Reflector &h, AsVlanListQuarter &s) {
    h.type_def("VlanListQuarter", vtss::expose::snmp::AsnType::OctetString);
}
#endif  // VTSS_SW_OPTION_SNMP

#ifdef VTSS_SW_OPTION_JSON_RPC
bool vlan_list_idx_update(uint32_t &idx, uint32_t size, uint8_t *data) {
    uint32_t index_array = idx >> 3;
    uint32_t index_bit = idx & 0x7;

    // forward until we hit a non-zero value in the data array
    for (; index_array < size; ++index_array)
        if (data[index_array] >> index_bit)
            break;  // Stop forwarding as we found a valid bit
        else
            index_bit = 0;  // reset the index as we have skipped the entry

    if (index_array >= size) return false;

    // Find the first valid bit (after index_bit) in the current byte
    for (; index_bit < 8; ++index_bit)
        if ((data[index_array] >> index_bit) & 1) break;  // We got a valid bit

    // we got a valid bit at (index_array, index_bit) update the idx member
    idx = index_array * 8 + index_bit;
    return true;
}

void serialize(expose::json::Exporter &e, AsVlanList &s) {
    uint32_t idx = 0;
    ArrayExporter<uint32_t> a(e);
    while (vlan_list_idx_update(idx, s.size_, s.data_)) a.add(idx++);
}
void serialize(expose::json::Loader &e, AsVlanList &s) {
    uint32_t vlan = 0;
    ArrayLoad<uint32_t> a(e);
    memset(s.data_, 0, s.size_);

    while (a.get(vlan)) {
        if (vlan > 4095) {
            e.flag_error();
            return;
        }

        s.data_[vlan / 8] |= (1U << (vlan % 8));
    }
}
void serialize(expose::json::HandlerReflector &e, AsVlanList &s) {
    e.type_terminal(expose::json::JsonCoreType::Array, "vtss_vid_list_t",
                    "vtss_vid_list_t");
}
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsEtherType &s) { serialize(h, s.val); }
void serialize(expose::snmp::SetHandler &h, AsEtherType &s) { serialize(h, s.val); }
void serialize(expose::snmp::Reflector &h, AsEtherType &s) {
    h.type_def("EtherType", vtss::expose::snmp::AsnType::Unsigned);
}
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, AsEtherType a) {
    serialize(e, a.val);
}
void serialize(expose::json::Loader &e, AsEtherType a) { serialize(e, a.val); }
void serialize(expose::json::HandlerReflector &e, AsEtherType a) {
    serialize(e, a.val);
}
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_SNMP
void serialize(expose::snmp::GetHandler &h, AsPsecUserBitmaskType &s) {serialize(h, s.val);}
void serialize(expose::snmp::SetHandler &h, AsPsecUserBitmaskType &s) {serialize(h, s.val);}
void serialize(expose::snmp::Reflector  &h, AsPsecUserBitmaskType &s) {h.type_def("PsecUserBitmaskType", vtss::expose::snmp::AsnType::Unsigned);}
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter         &e, AsPsecUserBitmaskType a) {serialize(e, a.val);}
void serialize(expose::json::Loader           &e, AsPsecUserBitmaskType a) {serialize(e, a.val);}
void serialize(expose::json::HandlerReflector &e, AsPsecUserBitmaskType a) {serialize(e, a.val);}
#endif  // VTSS_SW_OPTION_JSON_RPC

// AsInterfaceIndex -----------------------------------------------------------
ostream &operator<<(ostream &o, const AsInterfaceIndex &ifidx) {
    mesa_rc rc;
    vtss_ifindex_elm_t elm;

    rc = vtss_ifindex_decompose(ifidx.val, &elm);
    if (rc != VTSS_RC_OK) {
        o << "<UNKNOWN:" << ifidx.val << ">";
        return o;
    }

    switch (elm.iftype) {
    case VTSS_IFINDEX_TYPE_NONE:
        o << "NONE";
        return o;

    case VTSS_IFINDEX_TYPE_PORT: {
        icli_switch_port_range_t icli_port;
        icli_port.usid = elm.usid;
        icli_port.begin_uport = iport2uport(elm.ordinal);
        icli_port.port_cnt = 1;

        if (!icli_port_from_usid_uport(&icli_port)) {
            TRACE_JSON(ERROR) << "ifidx:" << ifidx.val << " usid:" << elm.usid
                              << " isid:" << elm.isid
                              << " ordinal:" << elm.ordinal
                              << " uport:" << iport2uport(elm.ordinal);
            o << "<INVALID:" << ifidx.val << ">";
            return o;
        }

        const char *type = icli_port_type_get_short_name(
                (icli_port_type_t)icli_port.port_type);
        o << type << " " << icli_port.usid << "/" << icli_port.begin_port;
        return o;
    }

    case VTSS_IFINDEX_TYPE_LLAG:
        o << "LLAG " << elm.usid << "/";
        break;

    case VTSS_IFINDEX_TYPE_GLAG:
        o << "GLAG ";
        break;

    case VTSS_IFINDEX_TYPE_VLAN:
        o << "VLAN ";
        break;

    case VTSS_IFINDEX_TYPE_REDBOX_NEIGHBOR:
        o << "RedBox-Neighbor";
        return o;

    case VTSS_IFINDEX_TYPE_CPU:
        o << "CPU " << elm.usid << "/" << iport2uport(elm.ordinal);
        return o;

    case VTSS_IFINDEX_TYPE_FRR_VLINK:
        o << "OSPF-VLINK ";
        break;

    default:
        o << "<UNKNOWN:" << ifidx.val << ">";
        return o;
    }

    o << elm.ordinal;
    return o;
}

#ifdef VTSS_SW_OPTION_JSON_RPC
bool parse(const char *&b, const char *e, AsInterfaceIndex &x) {
    mesa_rc rc = VTSS_RC_OK;
    const char *i = b;
    int type = 0;
    expose::json::StringLiteral sep("/");
    parser::OneOrMoreSpaces space;
    parser::Int<int, 10, 1, 0> number1;
    parser::Int<int, 10, 1, 0> number2;

    const vtss_enum_descriptor_t interface_names[] = {
            {0, "NONE"},
            {1, "LLAG"},
            {2, "GLAG"},
            {3, "VLAN"},
            {4, "EVC (obsolete)"},
            {5, "MPLS-link (obsolete)"},
            {6, "MPLS-tunnel (obsolete)"},
            {7, "MPLS-PW (obsolete)"},
            {8, "MPLS-LSP (obsolete)"},
            {9, "FOREIGN"},
            {10, "OSPF-VLINK"},
            {11, "RedBox-Neighbor"},
#define FIRST_PORT 100
            {100, "FastEthernet"},
            {100, "Fa"},
            {101, "GigabitEthernet"},
            {101, "Gi"},
            {102, "2.5GigabitEthernet"},
            {102, "2.5G"},
            {103, "5GigabitEthernet"},
            {103, "5G"},
            {104, "10GigabitEthernet"},
            {104, "10G"},
            {105, "CPU"},
            {106, "25GigabitEthernet"},
            {106, "25G"},
#define LAST_PORT 106
            {0, 0}};

    // Parse the type
    if (!enum_element_parse(i, e, interface_names, type)) {
        TRACE_JSON(DEBUG) << "Failed to parse interface type. Input string: "
                          << str(b, e);
        return false;
    }

    if (type != 0 && type != 11) {
        // Only types different from NONE and RedBox-Neighbor use extra numbers
        // Parse one of more white-spaces followed by a number
        if (!parser::Group(i, e, space, number1)) {
            TRACE_JSON(DEBUG) << "Parse error. Input string: " << str(b, e);
            return false;
        }

        // type is a port - this means that the "number1" is usid, and we still
        // need to parse the port number part
        if (type == 1 || (type >= FIRST_PORT && type <= LAST_PORT)) {
            if (!parser::Group(i, e, sep, number2)) {
                TRACE_JSON(DEBUG) << "Parse error. Input string: " << str(b, e);
                return false;
            }
        }
    }

    // Parsed completly - we just need to pick up the result
    icli_switch_port_range_t icli_port = {};
    switch (type) {
    case 0:  // NONE
        x.val = VTSS_IFINDEX_NONE;
        break;

    case 1:  // llag
        if (number1.get() == 0) {
            // Internally ISID == 0 means local - but this is not allowed at the
            // external interface.
            TRACE_JSON(DEBUG) << "usid/isid == 0 is not allowed!";
            return false;
        }
        rc = vtss_ifindex_from_llag(topo_usid2isid(number1.get()),
                                    number2.get(), &x.val);
        break;

    case 2:  // glag
        rc = vtss_ifindex_from_glag(number1.get(), &x.val);
        break;

    case 3:  // vlan
        rc = vtss_ifindex_from_vlan(number1.get(), &x.val);
        break;

    case 9:  // foreign
        rc = vtss_ifindex_from_foreign(number1.get(), &x.val);
        break;

    case 10:  // vlink
        rc = vtss_ifindex_from_frr_vlink(number1.get(), &x.val);
        break;

    case 11: // RedBox-Neighbor
        x.val = VTSS_IFINDEX_REDBOX_NEIGHBOR;
        break;

    case 100:  // FastEthernet
        icli_port.port_type = ICLI_PORT_TYPE_FAST_ETHERNET;
        break;

    case 101:  // GigabitEthernet
        icli_port.port_type = ICLI_PORT_TYPE_GIGABIT_ETHERNET;
        break;

    case 102:  // 2.5GigabitEthernet
        icli_port.port_type = ICLI_PORT_TYPE_2_5_GIGABIT_ETHERNET;
        break;

    case 103:  // 5GigabitEthernet
        icli_port.port_type = ICLI_PORT_TYPE_FIVE_GIGABIT_ETHERNET;
        break;

    case 104:  // 10GigabitEthernet
        icli_port.port_type = ICLI_PORT_TYPE_TEN_GIGABIT_ETHERNET;
        break;

    case 105:  // cpu
        icli_port.port_type = ICLI_PORT_TYPE_CPU;
        break;

    case 106:  // 25GigabitEthernet
        icli_port.port_type = ICLI_PORT_TYPE_25_GIGABIT_ETHERNET;
        break;

    default:
        return false;
    }

    if (type >= FIRST_PORT && type <= LAST_PORT) {
        if (number1.get() == 0) {
            TRACE_JSON(DEBUG) << "usid/isid == 0 is not allowed!";
            // Internally ISID == 0 means local - but this is not allowed at the
            // external interface.
            return false;
        }

        icli_port.switch_id = number1.get();
        icli_port.port_cnt = 1;
        icli_port.begin_port = number2.get();
        TRACE_JSON(DEBUG) << "usid: " << icli_port.switch_id
                          << " type: " << icli_port.port_type
                          << " cnt: " << icli_port.port_cnt
                          << " begin: " << icli_port.begin_port;
        rc = icli_port_get(&icli_port) ? VTSS_RC_OK : VTSS_RC_ERROR;
        if (rc == VTSS_RC_OK) {
            rc = vtss_ifindex_from_port(icli_port.isid, icli_port.begin_iport,
                                        &x.val);
            if (rc != VTSS_RC_OK)
                TRACE_JSON(DEBUG) << "vtss_ifindex_from_port failed: " << rc;
        } else {
            TRACE_JSON(DEBUG) << "icli_port_get failed";
        }
    }

    if (rc != VTSS_RC_OK) return false;

    b = i;
    return true;
}

void serialize(expose::json::Exporter &e, AsInterfaceIndex ifidx) {
    auto o = e.encoded_stream();
    o << ifidx;
}

bool enum_element_parse(const char *&b, const char *e,
                        const ::vtss_enum_descriptor_t *descriptor,
                        int &value) {
    const char *i = b;

    while (true) {
        // break on end-of-descriptor
        if (!descriptor->valueName) break;

        // continue if nat match
        str expect(descriptor->valueName);
        if (!expose::json::parse_and_compare_no_qoutes(i, e, expect)) {
            descriptor++;
            continue;
        }

        // found a match
        value = descriptor->intValue;
        b = i;
        return true;
    }

    return false;
}


void serialize(expose::json::Loader &e, AsInterfaceIndex ifidx) {
    const char *i = e.pos_;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e.end_, expose::json::quote_start)) {
        TRACE_JSON(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }

    // Parse the actually interface index
    if (!parse(i, e.end_, ifidx)) {
        TRACE_JSON(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }

    // Parse the ending qoute
    if (!parse(i, e.end_, expose::json::quote_end)) {
        TRACE_JSON(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }

    e.pos_ = i;
    return;
}

void serialize(expose::json::HandlerReflector &e, AsInterfaceIndex ifidx) {
    // TODO - added better description
    e.type_terminal(expose::json::JsonCoreType::String, "vtss_ifindex_t",
                    "Interface identification");
}
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_SNMP

template<typename T>
void serialize_export(T &h, AsInterfaceIndex &s) {
    uint32_t x = vtss_ifindex_cast_to_u32(s.val);
    int32_t y;

    if (x > 0x7fffffff) {
        VTSS_BASICS_TRACE(ERROR) << "Value too big: " << x;
        x = 0x7fffffff;
    }

    y = (int32_t)x;
    serialize(h, y);
}

void serialize(expose::snmp::GetHandler &h, AsInterfaceIndex &s) {
    serialize_export(h, s);
}

void serialize(expose::snmp::SetHandler &a, AsInterfaceIndex &s) {
    int32_t x = 0;
    serialize(a, x);

    if (a.error_code() != expose::snmp::ErrorCode::noError) {
        VTSS_BASICS_TRACE(INFO)
                << "Failed to serialize as int: " << a.error_code();
        return;
    }

    if (x < 0) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << x;
        a.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    s.val = vtss_ifindex_t{(u32)x};
}

void serialize(expose::snmp::OidImporter &a, AsInterfaceIndex &s) {
    if (!a.ok_) return;

    uint32_t x = 0;
    if (!a.consume(x)) {
        a.flag_error();
        return;
    }

    if (x > 0x7fffffff) {  // Out of range
        if (a.next_request()) {
            x = 0x7fffffff;
            a.flag_overflow();
        } else {
            a.flag_error();
        }
    }

    s.val = vtss_ifindex_t{x};
    VTSS_BASICS_TRACE(NOISE) << "res: " << s.val;
}

void serialize(expose::snmp::OidExporter &a, AsInterfaceIndex &s) {
    serialize_export(a, s);
}

void serialize(expose::snmp::Reflector &h, AsInterfaceIndex &) {
    h.type_def("InterfaceIndex", vtss::expose::snmp::AsnType::Integer);
}

void serialize(expose::snmp::TrapHandler &a, AsInterfaceIndex &s) {
    serialize_export(a, s);
}
#endif  // VTSS_SW_OPTION_SNMP


// AsStdDisplayString -----------------------------------------------------------
#ifdef VTSS_SW_OPTION_SNMP
void serialize(vtss::expose::snmp::GetHandler &h, vtss::AsStdDisplayString &p) {
    char y[MAX_OCTET_STRING_SIZE];
    if (p.val.length() >= MAX_OCTET_STRING_SIZE ) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected length: " << p.val.length()
                                << " expected max length " << MAX_OCTET_STRING_SIZE - 1 ;
        h.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    strncpy(y, p.val.c_str(), MAX_OCTET_STRING_SIZE);
    y[MAX_OCTET_STRING_SIZE - 1] = 0;
    vtss::AsDisplayString x_(y, p.size_);
    serialize(h, x_);
};

void serialize(vtss::expose::snmp::SetHandler &h, vtss::AsStdDisplayString &p) {
    char y[MAX_OCTET_STRING_SIZE];

    if (h.val_length >= MAX_OCTET_STRING_SIZE ) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected length: " << h.val_length
                                << " expected max length " << MAX_OCTET_STRING_SIZE - 1 ;
        h.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    if (p.size_ >= MAX_OCTET_STRING_SIZE ) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << p.size_;
        h.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    vtss::AsDisplayString x_(y, p.size_);
    serialize(h, x_);
    p.val.assign(x_.ds_);
    p.size_ =  x_.size_;
};

void serialize(vtss::expose::snmp::Reflector &h, vtss::AsStdDisplayString &p) {
    vtss::AsDisplayString x_("", p.size_); // Only size and type is used
    serialize(h, x_);
};
#endif  // VTSS_SW_OPTION_SNMP

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, vtss::AsStdDisplayString s) {
    // input s, output e
    char y[MAX_OCTET_STRING_SIZE];
    if (s.val.length() >= MAX_OCTET_STRING_SIZE ) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected length: " << s.val.length()
                                << " expected max length " << MAX_OCTET_STRING_SIZE - 1 ;
        e.flag_error();
        return;
    }
    strncpy(y, s.val.c_str(), MAX_OCTET_STRING_SIZE);
    y[MAX_OCTET_STRING_SIZE - 1] = 0;
    vtss::AsDisplayString x_(y, s.size_);
    serialize(e, x_);
};

void serialize(expose::json::Loader &e, vtss::AsStdDisplayString s) {
    // input e, output s
    char y[MAX_OCTET_STRING_SIZE];
    if (s.size_ >= MAX_OCTET_STRING_SIZE ) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << s.size_;
        e.flag_error();
        return;
    }
    vtss::AsDisplayString x_(y, s.size_);
    serialize(e, x_);
    s.val.assign(x_.ds_);
    s.size_ =  x_.size_;
};

void serialize(expose::json::HandlerReflector &e, vtss::AsStdDisplayString s) {
    vtss::AsDisplayString x_("", s.size_);
    serialize(e, x_);
};
#endif // VTSS_SW_OPTION_JSON_RPC

// AsTextualTimestamp
#if defined(VTSS_SW_OPTION_PTP) && (defined(VTSS_SW_OPTION_SNMP) || defined(VTSS_SW_OPTION_JSON_RPC))

// Maximum value is "281474976710655.999999999"
// so maximum length is:
//   seconds: 2^48 =    15 chars
//   dot:                1 char
//   subseconds:         9 chars
//   terminating zero:   1 char
//   --------------------------
//                      26 chars
#define TEXTUAL_TIME_STAMP_STR_LEN_MAX 26
static char *textual_timestamp_to_str(vtss::AsTextualTimestamp &p, char *buf, size_t buf_size)
{
    uint64_t tmp;

    tmp = p.val.sec_msb;
    tmp = (tmp << 32) + p.val.seconds;
    if (p.val.nanoseconds != 0) {
        (void)snprintf(buf, buf_size, VPRI64u ".%09u", tmp, p.val.nanoseconds);
    } else {
        (void)snprintf(buf, buf_size, VPRI64u, tmp);
    }

    buf[buf_size - 1] = '\0';
    return buf;
}

// Returns a pointer to the character after the last one parsed.
static const char *str_to_textual_timestamp(const str &s, vtss::AsTextualTimestamp &p)
{
    uint64_t    secs = 0;
    uint32_t    secs_digits = 0, ns = 0, ns_digits = 0;
    parser::Lit string_begin("\"");
    parser::Lit string_end("\"");
    const char  *i;

    i = s.begin();
    if (!string_begin(i, s.end())) {
        return nullptr;
    }

    while (*i >= '0' && *i <= '9' && secs_digits < 15) {
        secs = secs * 10 + (*i - '0');
        ++i;
        ++secs_digits;
    }

    if (*i == '.') {
        ++i;
        while (*i >= '0' && *i <= '9' && ns_digits < 9) {
            ns = ns * 10 + (*i - '0');
            ++i;
            ++ns_digits;
        }
    }

    if (!string_end(i, s.end())) {
        return nullptr;
    }

    p.val.sec_msb = (secs & 0xFFFF00000000) >> 32;
    p.val.seconds = (secs & 0x0000FFFFFFFF);
    p.val.nanoseconds = ns * pow(10, (9 - ns_digits));
    p.val.nanosecondsfrac = 0;

    return i;
}
#endif

#if defined(VTSS_SW_OPTION_SNMP) && defined(VTSS_SW_OPTION_PTP)
// Function for converting a mesa_timestamp_t to an SNMP display string
void serialize(vtss::expose::snmp::GetHandler &h, vtss::AsTextualTimestamp &p)
{
    char buf[TEXTUAL_TIME_STAMP_STR_LEN_MAX + 1];

    vtss::AsDisplayString x_(textual_timestamp_to_str(p, buf, sizeof(buf)), sizeof(buf));
    serialize(h, x_);
}

// Function for converting an SNMP display string to a mesa_timestamp_t
void serialize(vtss::expose::snmp::SetHandler &a, vtss::AsTextualTimestamp &p)
{
    const char *end_pos;

    EXPECT_TYPE(vtss::expose::snmp::AsnType::OctetString);

    if (a.val_length >= TEXTUAL_TIME_STAMP_STR_LEN_MAX) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected length: " << a.val_length << " expected max length " << TEXTUAL_TIME_STAMP_STR_LEN_MAX - 1 ;
        a.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    str my_str(reinterpret_cast<const char *>(a.val), a.val_length);
    end_pos = str_to_textual_timestamp(my_str, p);

    if (end_pos) {
        a.state(expose::snmp::HandlerState::DONE);
    } else {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << a.val;
        a.error_code(expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
    }
}

void serialize(vtss::expose::snmp::Reflector &h, vtss::AsTextualTimestamp &p) {
    h.type_def("TextualTimestamp", expose::snmp::AsnType::OctetString);
}
#endif // VTSS_SW_OPTION_SNMP && VTSS_SW_OPTION_PTP

#if defined(VTSS_SW_OPTION_JSON_RPC) && defined(VTSS_SW_OPTION_PTP)
void serialize(expose::json::Exporter &e, vtss::AsTextualTimestamp s) {
    char buf[TEXTUAL_TIME_STAMP_STR_LEN_MAX + 1];

    str my_str(textual_timestamp_to_str(s, buf, sizeof(buf)));

    serialize(e, my_str);
}

void serialize(expose::json::Loader &e, vtss::AsTextualTimestamp s)
{
    str my_str(e.pos_, e.end_);
    const char *end_pos = str_to_textual_timestamp(my_str, s);

    if (end_pos) {
        e.pos_ = end_pos;
    } else {
        TRACE_JSON(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &e, vtss::AsTextualTimestamp s)
{
    e.type_terminal(vtss::expose::json::JsonCoreType::String,
                    "vtss_ptp_time", "A PTP time presented in a human readable format: "
                    "sssssssssssssss[.sssssssss].");
};
#endif  // VTSS_SW_OPTION_JSON_RPC && VTSS_SW_OPTION_PTP

#ifdef VTSS_SW_OPTION_SNMP
void serialize(vtss::expose::snmp::GetHandler &a, AsBigInt &p)
{
    // Simply serialize as a VTSSUnsigned64, because that is accurate in the
    // SNMP world
    serialize(a, p.val);
}

void serialize(vtss::expose::snmp::SetHandler &a, AsBigInt &p)
{
    // Simply serialize as a VTSSUnsigned64, because that is accurate in the
    // SNMP world
    serialize(a, p.val);
}

void serialize(vtss::expose::snmp::Reflector &h, AsBigInt &p)
{
    // Simply serialize as a VTSSUnsigned64, because that is accurate in the
    // SNMP world
    serialize(h, p.val);
}
#endif // VTSS_SW_OPTION_SNMP

#ifdef VTSS_SW_OPTION_JSON_RPC

// The maximum length of a BigInt (64-bit unsigned integer) represented as a
// string is 20 chars (2^64 - 1 = 18.446.744.073.709.551.615) + a terminating
// zero.
#define BIG_INT_STR_LEN_MAX 21
void serialize(expose::json::Exporter &e, vtss::AsBigInt s)
{
    char buf[BIG_INT_STR_LEN_MAX];
    (void)snprintf(buf, sizeof(buf), VPRI64u, s.val);

    buf[sizeof(buf) - 1] = '\0';
    str my_str(buf);
    serialize(e, my_str);
}

static const char *str_to_big_int(const str &s, vtss::AsBigInt &p)
{
    parser::Lit string_begin("\"");
    parser::Lit string_end("\"");
    const char *i;
    char       *first_char_after_parsing;

    i = s.begin();
    if (!string_begin(i, s.end())) {
        return nullptr;
    }

    errno = 0;
    p.val = strtoull(i, &first_char_after_parsing, 10);
    if (errno) {
        return nullptr;
    }

    i = first_char_after_parsing;
    if (!string_end(i, s.end())) {
        return nullptr;
    }

    return i;
}

void serialize(expose::json::Loader &e, vtss::AsBigInt s)
{
    str my_str(e.pos_, e.end_);
    const char *end_pos = str_to_big_int(my_str, s);

    if (end_pos) {
        e.pos_ = end_pos;
    } else {
        TRACE_JSON(DEBUG) << "AsBigInt: Error: " << my_str;
        e.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &e, vtss::AsBigInt s)
{
    e.type_terminal(expose::json::JsonCoreType::String, "uint64_t", "64-bit unsigned integer represented as a string");

}
#endif // VTSS_SW_OPTION_JSON_RPC

}  // namespace vtss

vtss::ostream &operator<<(vtss::ostream &o, const vtss_ifindex_t &i) {
    vtss_ifindex_t j = i;
    o << vtss::AsInterfaceIndex(j);
    return o;
}
#ifdef VTSS_SW_OPTION_SNMP
void serialize(vtss::expose::snmp::GetHandler &h, vtss_ifindex_t &s) {
    vtss::AsInterfaceIndex s_(s);
    serialize(h, s_);
}
void serialize(vtss::expose::snmp::SetHandler &a, vtss_ifindex_t &s) {
    vtss::AsInterfaceIndex s_(s);
    serialize(a, s_);
}

void serialize(vtss::expose::snmp::OidImporter &a, vtss_ifindex_t &s) {
    vtss::AsInterfaceIndex s_(s);
    serialize(a, s_);
}

void serialize(vtss::expose::snmp::OidExporter &a, vtss_ifindex_t &s) {
    vtss::AsInterfaceIndex s_(s);
    serialize(a, s_);
}

void serialize(vtss::expose::snmp::Reflector &h, vtss_ifindex_t &s) {
    vtss::AsInterfaceIndex s_(s);
    serialize(h, s_);
}
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(vtss::expose::json::Exporter &e, vtss_ifindex_t s) {
    vtss::AsInterfaceIndex s_(s);
    serialize(e, s_);
}

void serialize(vtss::expose::json::Loader &e, vtss_ifindex_t &s) {
    vtss::AsInterfaceIndex s_(s);
    serialize(e, s_);
}

void serialize(vtss::expose::json::HandlerReflector &e, vtss_ifindex_t s) {
    vtss::AsInterfaceIndex s_(s);
    serialize(e, s_);
}

#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef __VTSS_TRACE_STREAM_DETAILS_LINUX_HXX__
const char* vtss_basics_default_severity_level_get() {
    switch (vtss::trace::global_settings.default_severity_level_get()) {
        case vtss::trace::severity::FATAL:   return "FATAL";
        case vtss::trace::severity::ERROR:   return "ERROR";
        case vtss::trace::severity::WARNING: return "WARNING";
        case vtss::trace::severity::INFO:    return "INFO";
        case vtss::trace::severity::DEBUG:   return "DEBUG";
        case vtss::trace::severity::NOISE:   return "NOISE";
        case vtss::trace::severity::RACKET:  return "RACKET";
    }
    return "UNKNOWN";
}

bool parse_vtss_basics_trace_level(bool b_fatal, bool b_error, bool b_warn, bool b_info,
                                   bool b_debug, bool b_noise, bool b_racket, int *level) {
    using namespace vtss::trace::severity;
    if (b_fatal) {
        *level = (int)FATAL;
    } else if (b_error) {
        *level = (int)ERROR;
    } else if (b_warn) {
        *level = (int)WARNING;
    } else if (b_info) {
        *level = (int)INFO;
    } else if (b_debug) {
        *level = (int)DEBUG;
    } else if (b_noise) {
        *level = (int)NOISE;
    } else if (b_racket) {
        *level = (int)RACKET;
    } else {
        *level = -1;
        return FALSE;
    }
    return TRUE;
}

void vtss_basics_default_severity_level_set(int level) {
    vtss::trace::global_settings.default_severity_level_set((vtss::trace::severity::E)level);
}
#endif /* #define __VTSS_TRACE_STREAM_DETAILS_LINUX_HXX__ */

#ifdef VTSS_SW_OPTION_SNMP
void serialize(vtss::expose::snmp::GetHandler &a, mesa_port_list_t &p) {
    using namespace vtss;

    VTSS_ASSERT(sizeof(GLOBAL_DISPLAY_STRING) >= 8);
    const uint32_t bit_length = p.max_size();

    // Clear output area as the port list may not be continuer
    for (int i = 0; i < 8; ++i) GLOBAL_DISPLAY_STRING[i] = 0;

    // Update the output data
    for (int i = 0; i < p.max_size(); ++i) {
        if (!p[i]) continue;

        uint32_t uport = iport2uport(i);

        // No room for this port...
        if (uport >= bit_length) continue;

        uint32_t byte_index = uport / 8;
        uint32_t bit_index = uport % 8;
        uint8_t mask = 1 << bit_index;

        GLOBAL_DISPLAY_STRING[byte_index] |= mask;
    }

    // Truncate the list to only contain the active part
    a.length = 1;  // Atleast one byte must be included
    for (int i = 0; i < 8; ++i)
        if (GLOBAL_DISPLAY_STRING[i]) a.length = i + 1;

    a.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    a.state(expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::SetHandler &a, mesa_port_list_t &p) {
    using namespace vtss;

    EXPECT_TYPE(expose::snmp::AsnType::OctetString);
    if (a.val_length < 1 || a.val_length > 128) {
        a.error_code(expose::snmp::ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    // Variable length input is accepted, if input is larger than the allocated
    // resource, then trunkcate input
    const uint32_t bit_length = min(a.val_length * 8, p.max_size());

    // Clear the array
    p.clear_all();

    // Copy input value to p, while converting from uport2iport
    for (int i = 0; i < bit_length; ++i) {
        uint8_t val = a.val[i / 8];
        bool is_set = (val >> (i % 8)) & 1;

        if (!is_set) continue;

        uint32_t iport = uport2iport(i);

        if (iport < p.max_size())  // Just ignore out-of-range requests
            p.set(iport);
    }

    a.state(expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::Reflector &h, mesa_port_list_t &p) {
    using namespace vtss;

    h.type_def("PortList", expose::snmp::AsnType::OctetString);
}
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(vtss::expose::json::Exporter &e, mesa_port_list_t &p) {
    using namespace vtss;

    ArrayExporter<AsInterfaceIndex> a(e);
    for (int i = 0; i < p.max_size(); ++i) {
        if (p[i]) {
            vtss_ifindex_t ii;
            mesa_rc rc = vtss_ifindex_from_port(VTSS_ISID_LOCAL, i, &ii);

            if (rc == VTSS_RC_OK) {
                AsInterfaceIndex idx(ii);
                a.add(idx);
            } else {
                TRACE_JSON(DEBUG)
                        << "Could not convert port to ifindex: iport: " << i
                        << " rc: " << rc;
                e.flag_error();
                return;
            }
        }
    }
}

void serialize(vtss::expose::json::Loader &e, mesa_port_list_t &p) {
    using namespace vtss;

    vtss_ifindex_t idx_;
    AsInterfaceIndex idx(idx_);
    ArrayLoad<AsInterfaceIndex> a(e);

    p.clear_all();

    while (a.get(idx)) {
        vtss_ifindex_elm_t ife;
        mesa_rc rc = vtss_ifindex_decompose(idx.val, &ife);

        if (rc != VTSS_RC_OK) {
            TRACE_JSON(DEBUG) << "vtss_ifindex_decompose: " << idx
                              << " rc: " << rc;
            e.flag_error();
            return;
        }

        if (ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
            TRACE_JSON(DEBUG) << "Expecting a port: " << VTSS_IFINDEX_TYPE_PORT
                              << " got: " << ife.iftype;
            e.flag_error();
            return;
        }

        int iport = ife.ordinal;
        if (iport < 0 || iport > p.max_size()) {
            TRACE_JSON(DEBUG) << "Iport: " << iport << " is out of range";
            e.flag_error();
            return;
        }

        p.set(iport);
    }
}

void serialize(vtss::expose::json::HandlerReflector &e, mesa_port_list_t &p) {
    using namespace vtss;

    // We are going to replace "AsPortList" types with mesa_port_list_t, but we
    // do not want this to change the public interfaces. We will therefore leave
    // the "vtss_port_list_stackable_t" even though it is not stackable
    e.type_terminal(expose::json::JsonCoreType::Array,
                    "vtss_port_list_stackable_t", "vtss_port_list_stackable_t");
}
#endif  // VTSS_SW_OPTION_JSON_RPC
