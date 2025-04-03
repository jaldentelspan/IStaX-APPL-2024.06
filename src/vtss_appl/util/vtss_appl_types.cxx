/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "main_types.h"
#include "vtss/appl/types.h"
#include "vtss/appl/types.hxx"
#include "vtss/basics/snmp.hxx"
#include "vtss/basics/types.hxx"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/parser_impl.hxx"
#include "vtss/basics/formatting_tags.hxx"
#include "vtss/basics/expose/json/literal.hxx"
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/algorithm.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss_appl_expose_json_array.hxx"
#include "icli_api.h"
#include "misc_api.h"
#include "topo_api.h"
#ifdef VTSS_SW_OPTION_IP
#include "ip_utils.hxx"
#endif
#include "mac_utils.hxx"

#define TRACE_JSON(X) VTSS_BASICS_TRACE(113, VTSS_BASICS_TRACE_GRP_JSON, X)

#define EXPECT_TYPE(EXPECT) \
    if (!vtss::expose::snmp::check_type(h, EXPECT)) return;
#define EXPECT_LENGTH(EXPECT) \
    if (!vtss::expose::snmp::check_length(h, EXPECT)) return;

#if defined(VTSS_SW_OPTION_PRIVATE_MIB_GEN)
int vtss_appl_mibgenerator_common(CYG_HTTPD_STATE *p,
                                  ::vtss::expose::snmp::NamespaceNode &ns,
                                  const char *name1, const char *name2,
                                  uint32_t module_id,
                                  vtss_appl_mib_build_specific_t build_mib) {
    vtss::StringStream ss;
    vtss::httpstream output(p);
    vtss::expose::snmp::MibGenerator h(ss);

    // setup default values here
    h.contact(VTSS_SNMP_ORGANISATION, VTSS_SNMP_CONTACT);
    h.definition_name(name2);
    h.parent(TO_STR(MIB_ENTERPRISE_PRODUCT_NAME), TO_STR(MIBPREFIX)"-SMI");
    // the build_mib function may choose to override default values
    build_mib(h);

    // generate the actual MIB
    h.go(ns);

    // transfer output to HTTP
    output << ss;
    return -1;
}
#endif

const vtss_enum_descriptor_t vtss_sfp_transceiver_txt[] {
    {MEBA_SFP_TRANSRECEIVER_NONE,          "none"},
    {MEBA_SFP_TRANSRECEIVER_NOT_SUPPORTED, "notSupported"},
    {MEBA_SFP_TRANSRECEIVER_100FX,         "sfp100FX"},
    {MEBA_SFP_TRANSRECEIVER_100BASE_LX,    "sfp100LX"},
    {MEBA_SFP_TRANSRECEIVER_100BASE_ZX,    "sfp100ZX"},
    {MEBA_SFP_TRANSRECEIVER_100BASE_SX,    "sfp100SX"},
    {MEBA_SFP_TRANSRECEIVER_100BASE_BX10,  "sfp100BaseBx10"},
    {MEBA_SFP_TRANSRECEIVER_100BASE_T,     "sfp100BaseT"},
    {MEBA_SFP_TRANSRECEIVER_1000BASE_BX10, "sfp1000BaseBx10"},
    {MEBA_SFP_TRANSRECEIVER_1000BASE_T,    "sfp1000BaseT"},
    {MEBA_SFP_TRANSRECEIVER_1000BASE_CX,   "sfp1000BaseCx"},
    {MEBA_SFP_TRANSRECEIVER_1000BASE_SX,   "sfp1000BaseSx"},
    {MEBA_SFP_TRANSRECEIVER_1000BASE_LX,   "sfp1000BaseLx"},
    {MEBA_SFP_TRANSRECEIVER_1000BASE_ZX,   "sfp1000BaseZx"},
    {MEBA_SFP_TRANSRECEIVER_1000BASE_LR,   "sfp1000BaseLr"},
    {MEBA_SFP_TRANSRECEIVER_1000BASE_X,    "sfp1000BaseX"},
    {MEBA_SFP_TRANSRECEIVER_2G5,           "sfp2G5"},
    {MEBA_SFP_TRANSRECEIVER_5G,            "sfp5G"},
    {MEBA_SFP_TRANSRECEIVER_10G,           "sfp10G"},
    {MEBA_SFP_TRANSRECEIVER_10G_SR,        "sfp10GSR"},
    {MEBA_SFP_TRANSRECEIVER_10G_LR,        "sfp10GLR"},
    {MEBA_SFP_TRANSRECEIVER_10G_LRM,       "sfp10GLRM"},
    {MEBA_SFP_TRANSRECEIVER_10G_ER,        "sfp10GER"},
    {MEBA_SFP_TRANSRECEIVER_10G_DAC,       "sfp10GDAC"},
    {MEBA_SFP_TRANSRECEIVER_25G,           "sfp25G"},
    {MEBA_SFP_TRANSRECEIVER_25G_SR,        "sfp25GSR"},
    {MEBA_SFP_TRANSRECEIVER_25G_LR,        "sfp25GLR"},
    {MEBA_SFP_TRANSRECEIVER_25G_LRM,       "sfp25GLRM"},
    {MEBA_SFP_TRANSRECEIVER_25G_ER,        "sfp25GER"},
    {MEBA_SFP_TRANSRECEIVER_25G_DAC,       "sfp25GDAC"},
    {0, 0},
};

namespace vtss {

PortListStackable::PortListStackable() { clear_all(); }

PortListStackable::PortListStackable(const PortListStackable &rhs) {
    for (uint32_t i = 0; i < 128; ++i) data_.data[i] = rhs.data_.data[i];
}

PortListStackable::PortListStackable(const ::vtss_port_list_stackable_t &rhs) {
    for (uint32_t i = 0; i < 128; ++i) data_.data[i] = rhs.data[i];
}

PortListStackable &PortListStackable::operator=(const PortListStackable &rhs) {
    for (uint32_t i = 0; i < 128; ++i) data_.data[i] = rhs.data_.data[i];
    return *this;
}

PortListStackable &PortListStackable::operator=(
        const ::vtss_port_list_stackable_t &rhs) {
    for (uint32_t i = 0; i < 128; ++i) data_.data[i] = rhs.data[i];
    return *this;
}

bool PortListStackable::get(const vtss_ifindex_t &i) const {
    if (!vtss_ifindex_is_port(i)) return false;

    vtss_ifindex_elm_t e;
    if (vtss_ifindex_decompose(i, &e) != VTSS_RC_OK) return false;

    return get(e.isid, e.ordinal);
}

bool PortListStackable::set(const vtss_ifindex_t &i) {
    if (!vtss_ifindex_is_port(i)) return false;

    vtss_ifindex_elm_t e;
    if (vtss_ifindex_decompose(i, &e) != VTSS_RC_OK) return false;

    return set(e.isid, e.ordinal);
}

bool PortListStackable::clear(const vtss_ifindex_t &i) {
    if (!vtss_ifindex_is_port(i)) return false;

    vtss_ifindex_elm_t e;
    if (vtss_ifindex_decompose(i, &e) != VTSS_RC_OK) return false;

    return clear(e.isid, e.ordinal);
}

bool PortListStackable::get(vtss_isid_t i, mesa_port_no_t p) const {
    if (i < VTSS_ISID_START || i >= VTSS_ISID_END) return false;

    if (p >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) return false;

    return index_get(PortListStackable::isid_port_to_index(i, p));
}

bool PortListStackable::set(vtss_isid_t i, mesa_port_no_t p) {
    if (i < VTSS_ISID_START || i >= VTSS_ISID_END) return false;

    if (p >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) return false;

    return index_set(PortListStackable::isid_port_to_index(i, p));
}

bool PortListStackable::clear(vtss_isid_t i, mesa_port_no_t p) {
    if (i < VTSS_ISID_START || i >= VTSS_ISID_END) return false;

    if (p >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) return false;

    return index_clear(PortListStackable::isid_port_to_index(i, p));
}

uint32_t PortListStackable::isid_port_to_index(vtss_isid_t i,
                                               mesa_port_no_t p) {
    static_assert(VTSS_PORT_NO_START == 0, "VTSS_PORT_NO_START must be zero");
    static_assert(VTSS_ISID_START == 1, "VTSS_ISID_START must be one");
    VTSS_ASSERT(fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1 <= port_chunk_size);
    static_assert(VTSS_ISID_END - 1 <= (data_bit_end) / port_chunk_size,
                  "Port count may not be larger than port_chunk_size");
    static_assert(sizeof(data_) == data_array_size,
                  "Unexpected size of storage");

    return (i - VTSS_ISID_START) * port_chunk_size + iport2uport(p);
}

void PortListStackable::clear_all() {
    for (uint32_t i = 0; i < 128; ++i) data_.data[i] = 0;
}

bool PortListStackable::equal_to(const ::vtss_port_list_stackable_t &rhs)
        const {
    for (uint32_t i = 0; i < 128; ++i)
        if (data_.data[i] != rhs.data[i]) return false;

    return true;
}

bool PortListStackable::index_get(uint32_t i) const {
    if (i >= 8 * 128) return false;

    uint32_t idx_bit = i & 7;
    uint32_t idx = i >> 3;
    return (data_.data[idx] >> idx_bit) & 1;
}

bool PortListStackable::index_set(uint32_t i) {
    if (i >= 8 * 128) return false;

    uint8_t val = 1;
    uint32_t idx_bit = i & 7;
    uint32_t idx = i >> 3;
    val <<= idx_bit;
    data_.data[idx] |= val;
    return true;
}

bool PortListStackable::index_clear(uint32_t i) {
    if (i >= 8 * 128) return false;

    uint8_t val = 1;
    uint32_t idx_bit = i & 7;
    uint32_t idx = i >> 3;
    val <<= idx_bit;
    data_.data[idx] &= (~val);
    return true;
}

void PortListStackable::ConstIterator::update_ifindex() const {
    uint32_t index_array = idx >> 3;
    uint32_t index_bit = idx & 0x7;

    // forward until we hit a non-zero value in the data array
    for (; index_array < data_array_size; ++index_array)
        if (parent->data_.data[index_array] >> index_bit)
            break;  // Stop forwarding as we found a valid bit
        else
            index_bit = 0;  // reset the index as we have skipped the entry

    if (index_array >= data_array_size) {
        // no valid entries found in the array. Move the cursor to the last
        // entry
        idx = data_bit_end;
        return;
    }

    // Find the first valid bit (after index_bit) in the current byte
    for (; index_bit < 8; ++index_bit)
        if ((parent->data_.data[index_array] >> index_bit) & 1)
            break;  // We got a valid bit

    // we got a valid bit at (index_array, index_bit) update the idx member
    idx = index_array * 8 + index_bit;

    if (idx > data_bit_end)
        idx = data_bit_end;


    uint32_t isid = (idx / port_chunk_size) + 1;
    uint32_t port = idx % port_chunk_size;
    port = uport2iport(port);

    (void)vtss_ifindex_from_port(isid, port, &ifindex);
}

static void print_port_element(mesa_port_no_t s, mesa_port_no_t e, bool &first,
                               ostream &o) {
    if (!first)
        o << ",";
    else
        first = false;

    o << iport2uport(s);
    if (s != e) o << "-" << iport2uport(e);
}

ostream &operator<<(ostream &o, const PortListStackable &l) {
    bool first = true;
    bool first_group = true;
    vtss_ifindex_elm_t e;
    vtss_isid_t isid;
    mesa_port_no_t port_start, port_end;
    icli_switch_port_range_t icli_port;
    const char *type;

    auto iter = l.begin();

    if (iter == l.end()) return o;

    (void)vtss_ifindex_decompose(*iter, &e);
    isid = e.isid;
    port_start = port_end = e.ordinal;
    icli_port.usid = topo_isid2usid(e.isid);
    icli_port.begin_uport = iport2uport(e.ordinal);
    icli_port.port_cnt = 1;
    if (!icli_port_from_usid_uport(&icli_port)) return o;
    type = icli_port_type_get_short_name((icli_port_type_t)icli_port.port_type);

    // print interface group
    o << type << " " << icli_port.usid << "/";

    ++iter;
    for (; iter != l.end(); ++iter) {
        (void)vtss_ifindex_decompose(*iter, &e);
        icli_port.usid = topo_isid2usid(e.isid);
        icli_port.begin_uport = iport2uport(e.ordinal);
        icli_port.port_cnt = 1;
        if (!icli_port_from_usid_uport(&icli_port)) continue;

        // test if we are still in the same interface group
        if (e.isid != isid ||
            type != icli_port_type_get_short_name(
                            (icli_port_type_t)icli_port.port_type)) {
            // as we ended a interface group, the started sequence must be ended
            print_port_element(port_start, port_end, first, o);

            // startup a new interface group
            if (first_group) {
                first_group = false;
                o << ", ";
            }

            first = true;
            isid = e.isid;
            type = icli_port_type_get_short_name(
                    (icli_port_type_t)icli_port.port_type);
            o << type << " " << icli_port.usid << "/";
            port_start = port_end = e.ordinal;

        } else if (port_end + 1 != e.ordinal) {
            // Print the cached element
            print_port_element(port_start, port_end, first, o);

            // Start a new cache
            port_start = port_end = e.ordinal;

        } else {
            // Expand the sequence
            port_end = e.ordinal;
        }
    }

    print_port_element(port_start, port_end, first, o);
    return o;
}

VlanList::VlanList() { clear_all(); }

VlanList::VlanList(const VlanList &rhs) { data_ = rhs.data_; }

VlanList::VlanList(const ::vtss_vlan_list_t &rhs) { data_ = rhs; }

VlanList &VlanList::operator=(const VlanList &rhs) {
    data_ = rhs.data_;
    return *this;
}

VlanList &VlanList::operator=(const ::vtss_vlan_list_t &rhs) {
    data_ = rhs;
    return *this;
}

void VlanList::clear_all() {
    for (uint32_t i = 0; i < 512; ++i) data_.data[i] = 0;
}

bool VlanList::get(const mesa_vid_t &i) const {
    if (i >= 4096) return false;

    uint32_t idx_bit = i & 7;
    uint32_t idx = i >> 3;
    return (data_.data[idx] >> idx_bit) & 1;
}

bool VlanList::set(const mesa_vid_t &i) {
    if (i <= 0 || i >= 4096) return false;

    uint8_t val = 1;
    uint32_t idx_bit = i & 7;
    uint32_t idx = i >> 3;
    val <<= idx_bit;
    data_.data[idx] |= val;
    return true;
}

bool VlanList::clear(const mesa_vid_t &i) {
    if (i >= 4096) return false;

    uint8_t val = 1;
    uint32_t idx_bit = i & 7;
    uint32_t idx = i >> 3;
    val <<= idx_bit;
    data_.data[idx] &= (~val);
    return true;
}

bool VlanList::equal_to(const ::vtss_vlan_list_t &rhs) const {
    for (uint32_t i = 0; i < 512; ++i)
        if (data_.data[i] != rhs.data[i]) return false;
    return true;
}

void VlanList::ConstIterator::update_ifindex() const {
    uint32_t index_array = idx >> 3;
    uint32_t index_bit = idx & 0x7;

    // forward until we hit a non-zero value in the data array
    for (; index_array < 512; ++index_array)
        if (parent->data_.data[index_array] >> index_bit)
            break;  // Stop forwarding as we found a valid bit
        else
            index_bit = 0;  // reset the index as we have skipped the entry

    if (index_array >= 512) {
        // no valid entries found in the array. Move the cursor to the last
        // entry
        idx = 4096;
        return;
    }

    // Find the first valid bit (after index_bit) in the current byte
    for (; index_bit < 8; ++index_bit)
        if ((parent->data_.data[index_array] >> index_bit) & 1)
            break;  // We got a valid bit

    // we got a valid bit at (index_array, index_bit) update the idx member
    idx = index_array * 8 + index_bit;
}

static void print_vlan_element(mesa_vid_t s, mesa_vid_t e, bool &first,
                               ostream &o) {
    if (!first)
        o << ", ";
    else
        first = false;

    o << s;
    if (s != e) o << "-" << e;
}

ostream &operator<<(ostream &o, const VlanList &l) {
    bool first = true;
    mesa_vid_t vid_start, vid_end;

    auto iter = l.begin();

    if (iter == l.end()) return o;

    vid_start = vid_end = *iter;

    ++iter;
    for (; iter != l.end(); ++iter) {
        if (vid_end + 1 != *iter) {
            // Print the cached element
            print_vlan_element(vid_start, vid_end, first, o);

            // Start a new cache
            vid_start = vid_end = *iter;
        } else {
            // Expand the sequence
            vid_end = *iter;
        }
    }

    print_vlan_element(vid_start, vid_end, first, o);
    return o;
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
void serialize(vtss::expose::snmp::GetHandler &h, PortListStackable &s) {
    serialize(h, s.as_api_type());
}

void serialize(vtss::expose::snmp::SetHandler &h, PortListStackable &s) {
    serialize(h, s.as_api_type());
}

void serialize(vtss::expose::snmp::Reflector &h, PortListStackable &) {
    h.type_def("PortList", vtss::expose::snmp::AsnType::OctetString);
}
#endif  // VTSS_SW_OPTION_PRIVATE_MIB
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, PortListStackable &s) {
    ArrayExporter<AsInterfaceIndex> a(e);
    for (auto i : s) a.add(AsInterfaceIndex(i));
}
void serialize(expose::json::Loader &e, PortListStackable &s) {
    s.clear_all();
    vtss_ifindex_t idx_;
    AsInterfaceIndex idx(idx_);
    ArrayLoad<AsInterfaceIndex> a(e);

    while (a.get(idx)) {
        if (!s.set(idx_)) {
            e.flag_error();
            return;
        }
    }
}
void serialize(expose::json::HandlerReflector &e, PortListStackable &s) {
    e.type_terminal(expose::json::JsonCoreType::Array,
                    "vtss_port_list_stackable_t",
                    "vtss_port_list_stackable_t");
}
#endif  // VTSS_SW_OPTION_JSON_RPC
};  // namespace vtss

#ifdef VTSS_SW_OPTION_PRIVATE_MIB

#define MAX_OCTET_STRING_SIZE 128
static char GLOBAL_DISPLAY_STRING[MAX_OCTET_STRING_SIZE];

// Must be in global namespace due to ADL rules
void serialize(vtss::expose::snmp::GetHandler &h, vtss_port_list_stackable_t &s) {
    static_assert(MAX_OCTET_STRING_SIZE == sizeof(GLOBAL_DISPLAY_STRING),
                  "GLOBAL_DISPLAY_STRING has wrong size");

    // find the most significant byte used in the port list
    uint32_t length = 1;  // atleast on byte must be included
    for (int i = 0; i < sizeof(s.data); ++i)
        if (s.data[i])
            length = i + 1;

    VTSS_ASSERT(length <= sizeof(GLOBAL_DISPLAY_STRING));
    memcpy(GLOBAL_DISPLAY_STRING, s.data, length);
    h.length = length;
    h.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    h.state(vtss::expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::SetHandler &h, vtss_port_list_stackable_t &s) {
    using namespace vtss;
    using namespace vtss::expose::snmp;

    EXPECT_TYPE(AsnType::OctetString);
    if (h.val_length < 1 || h.val_length > sizeof(s.data)) {
        h.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    memset(s.data, 0, sizeof(s.data));
    memcpy(s.data, h.val, min(h.val_length, sizeof(s.data)));
    h.state(vtss::expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::Reflector &h, vtss_port_list_stackable_t &) {
    h.type_def("PortList", vtss::expose::snmp::AsnType::OctetString);
}
#endif  // VTSS_SW_OPTION_PRIVATE_MIB

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
static vtss::BufStream<vtss::SBuf256> ss;
void serialize(vtss::expose::snmp::GetHandler &h, vtss_inet_address_t &a) {
    ss.clear();
    ss << a;
    h.length = ss.end() - ss.begin();
    h.value = reinterpret_cast<uint8_t *>(ss.begin());
    h.state(vtss::expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::SetHandler &h, vtss_inet_address_t &a) {
    if (!vtss::expose::snmp::check_type(h, vtss::expose::snmp::AsnType::OctetString)) return;
    if (h.val_length > 253 || h.val_length < 1) {
        h.error_code(vtss::expose::snmp::ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    const char *b = (const char *)h.val;
    const char *e = (const char *)h.val + h.val_length;

    vtss::InetAddress address;
    if (address.string_parse(b, e) && b == e) {
        a = address.as_api_type();
        h.state(vtss::expose::snmp::HandlerState::DONE);
        return;
    }

    h.error_code(vtss::expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
}

void serialize(vtss::expose::snmp::Reflector &h, vtss_inet_address_t &a) {
    h.type_def("InetAddress", vtss::expose::snmp::AsnType::OctetString);
}
#endif  // VTSS_SW_OPTION_PRIVATE_MIB
#ifdef VTSS_SW_OPTION_JSON_RPC
static bool parse(const char *&b, const char *e, vtss::InetAddress &x) {
    const char *i = b;
    vtss::parser::InetAddress p;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e, vtss::expose::json::quote_start)) return false;

    if (!p(i, e)) return false;

    if (!parse(i, e, vtss::expose::json::quote_end)) return false;

    x = p.get();
    b = i;
    return true;
}

void serialize(vtss::expose::json::Exporter &e, vtss_inet_address_t &s) {
    auto o = e.encoded_stream();
    o << s;
}

void serialize(vtss::expose::json::Loader &l, vtss_inet_address_t &s) {
    vtss::InetAddress i;
    if (!parse(l.pos_, l.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }

    s = i.as_api_type();
}

void serialize(vtss::expose::json::HandlerReflector &e, vtss_inet_address_t &s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String,
                    "vtss_inet_address_t",
                    "vtss_inet_address_t");
}
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
static vtss::BufStream<vtss::SBuf64> ipss;
void serialize(vtss::expose::snmp::GetHandler &h, mesa_ip_addr_t &a) {
    ipss.clear();
    ipss << a;
    h.length = ipss.end() - ipss.begin();
    h.value = reinterpret_cast<uint8_t *>(ipss.begin());
    h.state(vtss::expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::SetHandler &h, mesa_ip_addr_t &a) {
    if (!vtss::expose::snmp::check_type(h, vtss::expose::snmp::AsnType::OctetString)) return;
    if (h.val_length > 46 || h.val_length < 1) {
        h.error_code(vtss::expose::snmp::ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    const char *b = (const char *)h.val;
    const char *e = (const char *)h.val + h.val_length;

    vtss::IpAddress address;
    if (address.string_parse(b, e) && b == e) {
        a = address.as_api_type();
        h.state(vtss::expose::snmp::HandlerState::DONE);
        return;
    }

    h.error_code(vtss::expose::snmp::ErrorCode::wrongValue, __FILE__, __LINE__);
}

void serialize(vtss::expose::snmp::Reflector &h, mesa_ip_addr_t &a) {
    h.type_def("IpAddress", vtss::expose::snmp::AsnType::OctetString);
}
#endif  // VTSS_SW_OPTION_PRIVATE_MIB

/****************************************************************************
 * Shared serializers for TS types
 ****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB

void serialize(vtss::expose::snmp::GetHandler &h, mesa_timestamp_t &t) {
    VTSS_ASSERT(sizeof(GLOBAL_DISPLAY_STRING) >= 10);

    // Read from t, and write into GLOBAL_DISPLAY_STRING
    GLOBAL_DISPLAY_STRING[0] = (t.sec_msb >> 8)      & 0xff;
    GLOBAL_DISPLAY_STRING[1] =  t.sec_msb            & 0xff;
    GLOBAL_DISPLAY_STRING[2] = (t.seconds >> 24)     & 0xff;
    GLOBAL_DISPLAY_STRING[3] = (t.seconds >> 16)     & 0xff;
    GLOBAL_DISPLAY_STRING[4] = (t.seconds >> 8)      & 0xff;
    GLOBAL_DISPLAY_STRING[5] =  t.seconds            & 0xff;
    GLOBAL_DISPLAY_STRING[6] = (t.nanoseconds >> 24) & 0xff;
    GLOBAL_DISPLAY_STRING[7] = (t.nanoseconds >> 16) & 0xff;
    GLOBAL_DISPLAY_STRING[8] = (t.nanoseconds >> 8)  & 0xff;
    GLOBAL_DISPLAY_STRING[9] =  t.nanoseconds        & 0xff;
    h.length = 10;
    h.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    h.state(vtss::expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::SetHandler &h, mesa_timestamp_t &t) {
    EXPECT_TYPE(vtss::expose::snmp::AsnType::OctetString);
    EXPECT_LENGTH(10);

    // Read 10 bytes from h.val, and assign it to t
    t.sec_msb     = (h.val[0] << 8) + h.val[1];
    t.seconds     = (h.val[2] << 24) + (h.val[3] << 16) + (h.val[4] << 8) + h.val[5];
    t.nanoseconds = (h.val[6] << 24) + (h.val[7] << 16) + (h.val[8] << 8) + h.val[9];
    h.state(vtss::expose::snmp::HandlerState::DONE);
}

void serialize(vtss::expose::snmp::Reflector &h, mesa_timestamp_t &t) {
    // NOTE: The IEEE8021-ST-MIB is not yet approved!
    h.type_ref("IEEE8021STPTPtimeValue", "IEEE8021-ST-MIB", vtss::expose::snmp::AsnType::OctetString);
}
#endif  // VTSS_SW_OPTION_PRIVATE_MIB

#ifdef VTSS_SW_OPTION_JSON_RPC

namespace {
template<typename T>
void serialize_json_time_stamp_t(T &e, mesa_timestamp_t &t) {
    typename T::Map_t m =
            e.as_map(vtss::tag::Typename("mesa_timestamp_t"));

    m.add_leaf(t.sec_msb,
               vtss::tag::Name("SecondsMsb"),
               vtss::tag::Description("16-bit unsigned integer representing most significant bits of seconds."));

    m.add_leaf(t.seconds,
               vtss::tag::Name("Seconds"),
               vtss::tag::Description("32-bit unsigned integer  representing number of seconds."));

    m.add_leaf(t.nanoseconds,
               vtss::tag::Name("NanoSeconds"),
               vtss::tag::Description("32-bit unsigned integer representing the number of nanoseconds."));

}
}  // namespace

void serialize(vtss::expose::json::Exporter &e, mesa_timestamp_t &t) {
    serialize_json_time_stamp_t(e, t);
}

void serialize(vtss::expose::json::Loader &e, mesa_timestamp_t &t) {
    serialize_json_time_stamp_t(e, t);
}

void serialize(vtss::expose::json::HandlerReflector &e, mesa_timestamp_t &t) {
    serialize_json_time_stamp_t(e, t);
}
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_JSON_RPC
// vtss_appl_vcap_mac_t //////////////////////////////////////////////////////////////////
static constexpr const char *txt_type_vcap_mac = "vtss_appl_vcap_mac_t";
static constexpr const char *txt_desc_vcap_mac =
        "Ethernet MAC address filter encoded either as a single mac address "
        "in the form \"a:b:c:d:e:f\", where a-f is a "
        "base-16 human readable integer in the range [0-ff], or as an arbitrary "
        "multicast address represented by the string \"mc\", or as an arbitrary "
        "broadcast address represented by the string \"bc\", or as an arbitrary "
        "unicast address represented by the string \"uc\", or as any arbitrary "
        "address represented by the string \"any\"";
static constexpr const mesa_mac_t bc_mac_address = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static constexpr const mesa_mac_t bc_mac_mask = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static constexpr const mesa_mac_t mc_mac_address = {0x01,0x00,0x00,0x00,0x00,0x00};
static constexpr const mesa_mac_t mc_mac_mask = {0x01,0x00,0x00,0x00,0x00,0x00};
static constexpr const mesa_mac_t uc_mac_address = {0x00,0x00,0x00,0x00,0x00,0x00};
static constexpr const mesa_mac_t uc_mac_mask = {0x01,0x00,0x00,0x00,0x00,0x00};
static constexpr const mesa_mac_t any_mac_address = {0x00,0x00,0x00,0x00,0x00,0x00};
static constexpr const mesa_mac_t any_mac_mask = {0x00,0x00,0x00,0x00,0x00,0x00};
static constexpr const mesa_mac_t single_mac_mask = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

static const vtss::expose::json::Literal vcap_mac_bc("bc");
static const vtss::expose::json::Literal vcap_mac_mc("mc");
static const vtss::expose::json::Literal vcap_mac_uc("uc");
static const vtss::expose::json::Literal vcap_mac_any("any");

void serialize(vtss::expose::json::Exporter &e, const vtss_appl_vcap_mac_t &s) {
    if (s.mask == bc_mac_mask && s.value == bc_mac_address) {
        serialize(e, vtss::expose::json::quote_start);
        serialize(e, vcap_mac_bc);
        serialize(e, vtss::expose::json::quote_end);
    } else if (s.mask == mc_mac_mask && s.value == mc_mac_address) {
        serialize(e, vtss::expose::json::quote_start);
        serialize(e, vcap_mac_mc);
        serialize(e, vtss::expose::json::quote_end);
    } else if (s.mask == uc_mac_mask && s.value == uc_mac_address) {
        serialize(e, vtss::expose::json::quote_start);
        serialize(e, vcap_mac_uc);
        serialize(e, vtss::expose::json::quote_end);
    } else if (s.mask == any_mac_mask && s.value == any_mac_address) {
        serialize(e, vtss::expose::json::quote_start);
        serialize(e, vcap_mac_any);
        serialize(e, vtss::expose::json::quote_end);
    } else {
        serialize(e, const_cast<mesa_mac_t &>(s.value));
    }
}

void serialize(vtss::expose::json::Loader &e, vtss_appl_vcap_mac_t &s) {
    const char *i = e.pos_;
    if (parse(i, e.end_, vtss::expose::json::quote_start) &&
        parse(i, e.end_, vcap_mac_bc) &&
        parse(i, e.end_, vtss::expose::json::quote_end)) {
        s.value = bc_mac_address;
        s.mask = bc_mac_mask;
        e.pos_ = i;
    } else if ((i = e.pos_) &&
               parse(i, e.end_, vtss::expose::json::quote_start) &&
               parse(i, e.end_, vcap_mac_mc) &&
               parse(i, e.end_, vtss::expose::json::quote_end)) {
        s.value = mc_mac_address;
        s.mask = mc_mac_mask;
        e.pos_ = i;
    } else if ((i = e.pos_) &&
               parse(i, e.end_, vtss::expose::json::quote_start) &&
               parse(i, e.end_, vcap_mac_uc) &&
               parse(i, e.end_, vtss::expose::json::quote_end)) {
        s.value = uc_mac_address;
        s.mask = uc_mac_mask;
        e.pos_ = i;
    } else if ((i = e.pos_) &&
               parse(i, e.end_, vtss::expose::json::quote_start) &&
               parse(i, e.end_, vcap_mac_any) &&
               parse(i, e.end_, vtss::expose::json::quote_end)) {
        s.value = any_mac_address;
        s.mask = any_mac_mask;
        e.pos_ = i;
    } else {
        serialize(e, s.value);
        if (e.ok_) {
            s.mask = single_mac_mask;
        }
    }
}

void serialize(vtss::expose::json::HandlerReflector &h, vtss_appl_vcap_mac_t &s) {
    h.type_terminal(vtss::expose::json::JsonCoreType::String, txt_type_vcap_mac,
                    txt_desc_vcap_mac);
}

#endif  // VTSS_SW_OPTION_JSON_RPC
