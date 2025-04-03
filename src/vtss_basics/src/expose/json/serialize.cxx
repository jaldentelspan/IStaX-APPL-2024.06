/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <vtss/basics/trace_grps.hxx>
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON
#include <vtss/basics/trace_basics.hxx>

#include "time.h"
#include "vtss/basics/parse.hxx"
#include "vtss/basics/algorithm.hxx"
#include "vtss/basics/parser_impl.hxx"
#include "vtss/basics/expose/json/loader.hxx"
#include "vtss/basics/expose/json/exporter.hxx"
#include "vtss/basics/expose/json/serialize.hxx"
#include "vtss/basics/expose/json/string-encode.hxx"
#include "vtss/basics/expose/json/string-decoder.hxx"
#include "vtss/basics/expose/json/json-core-type.hxx"
#include "vtss/basics/expose/json/char-out-iterator.hxx"
#include "vtss/basics/expose/json/handler-reflector.hxx"
#include "vtss/basics/expose/json/string-decode-count.hxx"
#include "vtss/basics/expose/json/string-decode-capture.hxx"
namespace vtss {
static bool parse(const char *&b, const char *e, IpAddress &x);
static bool parse(const char *&b, const char *e, IpNetwork &x);
static bool parse(const char *&b, const char *e, Ipv4Address &x);
static bool parse(const char *&b, const char *e, Ipv4Network &x);
static bool parse(const char *&b, const char *e, Ipv6Address &x);
static bool parse(const char *&b, const char *e, Ipv6Network &x);
}  // namespace vtss

static constexpr const char *txt_type_string = "string";
static constexpr const char *txt_desc_string = "A variable length string";

static constexpr const char *txt_type_binary_string = "binary_buffer";
static constexpr const char *txt_desc_binary_string =
        "A variable length binary string. The string is HEX encoded";

static constexpr const char *txt_type_mac = "mesa_mac_t";
static constexpr const char *txt_desc_mac =
        "Ethernet MAC address encoded as \"a:b:c:d:e:f\", where a-f is a "
        "base-16 human readable integer in the range [0-ff]";

static constexpr const char *txt_type_ipv4 = "mesa_ipv4_t";
static constexpr const char *txt_desc_ipv4 =
        "Ipv4 address encoded as \"a.b.c.d\", where a-d is a "
        "base-10 human readable integer in the range [0-255]";

static constexpr const char *txt_type_ipv4_network = "mesa_ipv4_network_t";
static constexpr const char *txt_desc_ipv4_network =
        "A IPv4 network encoded as \"<ipv4>/<prefix-length>\" where <ipv4> is "
        "encoded as defined for mesa_ipv4_t, and <prefix-length> is a BASE-10 "
        "human readable integer in the range [0-32]";

static constexpr const char *txt_type_ipv6 = "mesa_ipv6_t";
static constexpr const char *txt_desc_ipv6 =
        "An Ipv6 address represented as human readable test as specified in "
        "RFC5952";

static constexpr const char *txt_type_ipv6_network = "mesa_ipv6_network_t";
static constexpr const char *txt_desc_ipv6_network =
        "A IPv6 network encoded as \"<ipv6>/<prefix-length>\" where <ipv6> is "
        "encoded as defined for mesa_ipv6_t, and <prefix-length> is a BASE-10 "
        "human readable integer in the range [0-128]";

static constexpr const char *txt_type_ipv4_or_6 = "mesa_ip_addr_t";
static constexpr const char *txt_desc_ip4_or_6 =
        "String representation of either an IPv4 address, an IPv6 address or "
        "\"no-address\". The string representation of IPv4 addresses is documented "
        "for vtss_ipv4_addr_t and the string representation for IPv6 address "
        "is documented for mesa_ipv6_t.";

static constexpr const char *txt_type_ip_network = "mesa_ip_network_t";
static constexpr const char *txt_desc_ip_network =
        "IP address and network prefix size encoded as "
        "\"<ip-address>/<prefix-length>\", where <ip-address> is encoded as "
        "mesa_ip_addr_t and <prefix-length> is a BASE-10 human readable "
        "integer in the range [0-128]";


class hex_out {
  public:
    explicit hex_out(vtss::expose::json::Exporter &e, char b)
        : e_(e), base_(b) {}

    void push_half(char c) {
        if (c <= 9) {
            e_.push_char(c + '0');
        } else {
            e_.push_char(c - 10 + base_);
        }
    }

    hex_out &operator=(const char &v) {
        push_half((unsigned char)v >> 4);
        push_half((unsigned char)v & 0xf);
        return (*this);
    }

    hex_out &operator*() { return (*this); }
    hex_out &operator++() { return (*this); }
    hex_out operator++(int /*rhs*/) { return (*this); }

  private:
    vtss::expose::json::Exporter &e_;
    char base_;
};

void serialize_formathex_uint32(vtss::expose::json::Exporter &e,
                                vtss::FormatHex<uint32_t> &s) {
    vtss::BufStream<vtss::SBuf8> buf;
    unsigned_to_hex_rbuf(s.t0, buf, s.base);

    // we have done the serialization of the actuall number, and we now know
    // the encoded length
    size_t encoded_length = buf.end() - buf.begin();

    // if we have a defined max, we must make sure that we can stay inside
    if (s.width_max && encoded_length > s.width_max) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }

    // if a minimum length is defined fill in the fill-chars
    if (s.width_min && encoded_length < s.width_min) {
        size_t diff = s.width_min - encoded_length;
        for (size_t i = 0; i < diff; ++i) e.push_char(s.fill_char);
    }

    // The encoded content exists in reverse order, fix that on its way out
    vtss::copy_backward(buf.begin(), buf.end(),
                        vtss::expose::json::CharOutIterator(e));
}

static bool parse_hex_char(const char *&b, const char *e, uint8_t &res) {
    if (e - b < 2) {
        // we need two digits here!
        return false;
    }

    vtss::parser::Int<uint8_t, 16, 2, 2> p;
    bool r = p(b,       // start of string
               b + 2);  // consumt max two chars

    if (!r) return false;

    res = p.get();
    return true;
}

template <class T>
bool parse_int(const char *&b, const char *e, T &t) {
    const char *i = b;
    vtss::parser::Int<T, 10, 1> p;

    // Try parse as a normal int
    if (p(i, e)) goto OK;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e, vtss::expose::json::quote_start)) return false;

    if (!p(i, e)) return false;

    if (!parse(i, e, vtss::expose::json::quote_end)) return false;

OK:
    t = p.get();
    b = i;
    return true;
}

// mesa_ip_addr_t //////////////////////////////////////////////////////////////
void serialize(vtss::expose::json::Exporter &e, const mesa_ip_addr_t &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(vtss::expose::json::Loader &l, mesa_ip_addr_t &x) {
    vtss::IpAddress i;
    if (!parse(l.pos_, l.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }

    x = i.as_api_type();
}

void serialize(vtss::expose::json::HandlerReflector &l, mesa_ip_addr_t &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_ipv4_or_6, txt_desc_ip4_or_6);
}

// mesa_ip_network_t ///////////////////////////////////////////////////////////
void serialize(vtss::expose::json::Exporter &e, const mesa_ip_network_t &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(vtss::expose::json::Loader &l, mesa_ip_network_t &x) {
    vtss::IpNetwork i;
    if (!parse(l.pos_, l.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }

    x = i.as_api_type();
}

void serialize(vtss::expose::json::HandlerReflector &l, mesa_ip_network_t &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_ip_network, txt_desc_ip_network);
}

// mesa_ipv4_network_t /////////////////////////////////////////////////////////
void serialize(vtss::expose::json::Exporter &e, const mesa_ipv4_network_t &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(vtss::expose::json::Loader &l, mesa_ipv4_network_t &x) {
    vtss::Ipv4Network i;
    if (!parse(l.pos_, l.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }

    x = i.as_api_type();
}

void serialize(vtss::expose::json::HandlerReflector &l,
               mesa_ipv4_network_t &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_ipv4_network, txt_desc_ipv4_network);
}

// mesa_ipv6_network_t /////////////////////////////////////////////////////////
void serialize(vtss::expose::json::Exporter &e, const mesa_ipv6_network_t &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(vtss::expose::json::Loader &l, mesa_ipv6_network_t &x) {
    vtss::Ipv6Network i;
    if (!parse(l.pos_, l.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }

    x = i.as_api_type();
}

void serialize(vtss::expose::json::HandlerReflector &e,
               mesa_ipv6_network_t &i) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_ipv6_network, txt_desc_ipv6_network);
}

// mesa_ipv6_t /////////////////////////////////////////////////////////////////
void serialize(vtss::expose::json::Exporter &e, const mesa_ipv6_t &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(vtss::expose::json::Loader &l, mesa_ipv6_t &x) {
    vtss::Ipv6Address i;
    if (!parse(l.pos_, l.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }

    x = i.as_api_type();
}

void serialize(vtss::expose::json::HandlerReflector &h, mesa_ipv6_t &x) {
    h.type_terminal(vtss::expose::json::JsonCoreType::String, txt_type_ipv6,
                    txt_desc_ipv6);
}

// mesa_mac_t //////////////////////////////////////////////////////////////////
template <class A, class B>
void serialize_json(A &a, B &s) {
    vtss::expose::json::Literal sep(":");
    serialize(a, vtss::expose::json::quote_start);
    serialize(a, vtss::HEX_fixed<2>(s.addr[0]));
    serialize(a, sep);
    serialize(a, vtss::HEX_fixed<2>(s.addr[1]));
    serialize(a, sep);
    serialize(a, vtss::HEX_fixed<2>(s.addr[2]));
    serialize(a, sep);
    serialize(a, vtss::HEX_fixed<2>(s.addr[3]));
    serialize(a, sep);
    serialize(a, vtss::HEX_fixed<2>(s.addr[4]));
    serialize(a, sep);
    serialize(a, vtss::HEX_fixed<2>(s.addr[5]));
    serialize(a, vtss::expose::json::quote_end);
}

void serialize(vtss::expose::json::Exporter &e, const mesa_mac_t &s) {
    serialize_json(e, const_cast<mesa_mac_t &>(s));
}

void serialize(vtss::expose::json::Loader &e, mesa_mac_t &s) {
    serialize_json(e, s);
}

void serialize(vtss::expose::json::HandlerReflector &h, mesa_mac_t &s) {
    h.type_terminal(vtss::expose::json::JsonCoreType::String, txt_type_mac,
                    txt_desc_mac);
}

// std::string /////////////////////////////////////////////////////////////////
struct StringDecodeCaptureStdString
        : public vtss::expose::json::StringDecodeHandler {
    explicit StringDecodeCaptureStdString(std::string *b) : buf_(b) {}
    void push(char c) { buf_->push_back(c); }

    std::string *buf_;
};

namespace std {
static bool parse(const char *&b, const char *e, std::string &s) {
    bool res;
    s.clear();
    StringDecodeCaptureStdString cap(&s);

    res = vtss::expose::json::string_decoder(b, e, &cap);
    if (!res) {
        return res;
    }

    return true;
}

void serialize(vtss::expose::json::Exporter &e, const std::string &s) {
    if (!vtss::expose::json::string_encode(e, &*s.begin(), s.c_str() + s.size())) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(vtss::expose::json::Loader &e, std::string &s) {
    if (!parse(e.pos_, e.end_, s)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(vtss::expose::json::HandlerReflector &e, std::string &s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, txt_type_string,
                    txt_desc_string);
}
}  // namespace std

namespace vtss {
// Buf /////////////////////////////////////////////////////////////////////////
static bool parse(const char *&b, const char *e, Buf &s) {
    bool res;
    expose::json::StringDecodeCapture cap(&s);

    res = expose::json::string_decoder(b, e, &cap);
    if (!res) {
        return res;
    }

    // we want to be C compatible
    cap.push(0);

    return cap.ok_;
}

void serialize(expose::json::Exporter &e, const Buf &s) {
    if (!expose::json::string_encode(e, s.begin(), s.end())) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(expose::json::Loader &e, Buf &s) {
    if (!parse(e.pos_, e.end_, s)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &e, Buf &s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, txt_type_string,
                    txt_desc_string);
}

// IpAddress /////////////////////////////////////////////////////////////////
static bool parse(const char *&b, const char *e, IpAddress &x) {
    const char *i = b;
    parser::IpAddress p;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e, expose::json::quote_start)) return false;

    if (!p(i, e)) return false;

    if (!parse(i, e, expose::json::quote_end)) return false;

    x = p.get();
    b = i;
    return true;
}

void serialize(expose::json::Exporter &e, const IpAddress &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(expose::json::Loader &l, IpAddress &x) {
    if (!parse(l.pos_, l.end_, x)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &l, IpAddress &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_ipv4_or_6, txt_desc_ip4_or_6);
}

// IpNetwork /////////////////////////////////////////////////////////////////
static bool parse(const char *&b, const char *e, IpNetwork &x) {
    const char *i = b;
    parser::IpNetwork p;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e, expose::json::quote_start)) return false;

    if (!p(i, e)) return false;

    if (!parse(i, e, expose::json::quote_end)) return false;

    x = p.get();
    b = i;
    return true;
}

void serialize(expose::json::Exporter &e, const IpNetwork &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(expose::json::Loader &l, IpNetwork &x) {
    if (!parse(l.pos_, l.end_, x)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &l, IpNetwork &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_ip_network, txt_desc_ip_network);
}

// Ipv4Address /////////////////////////////////////////////////////////////////
static bool parse(const char *&b, const char *e, Ipv4Address &x) {
    const char *i = b;
    parser::IPv4 p;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e, expose::json::quote_start)) return false;

    if (!p(i, e)) return false;

    if (!parse(i, e, expose::json::quote_end)) return false;

    x = p.get();
    b = i;
    return true;
}

void serialize(expose::json::Exporter &e, const Ipv4Address &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(expose::json::Loader &l, Ipv4Address &x) {
    if (!parse(l.pos_, l.end_, x)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &l, Ipv4Address &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String, txt_type_ipv4,
                    txt_desc_ipv4);
}

// Ipv4Network /////////////////////////////////////////////////////////////////
static bool parse(const char *&b, const char *e, Ipv4Network &x) {
    const char *i = b;
    parser::Ipv4Network p;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e, expose::json::quote_start)) return false;

    if (!p(i, e)) return false;

    if (!parse(i, e, expose::json::quote_end)) return false;

    x = p.get();
    b = i;
    return true;
}

void serialize(expose::json::Exporter &e, const Ipv4Network &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(expose::json::Loader &l, Ipv4Network &x) {
    if (!parse(l.pos_, l.end_, x)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &l, Ipv4Network &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_ipv4_network, txt_desc_ipv4_network);
}

// Ipv6Address /////////////////////////////////////////////////////////////////
static bool parse(const char *&b, const char *e, Ipv6Address &x) {
    const char *i = b;
    parser::IPv6 p;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e, expose::json::quote_start)) return false;

    if (!p(i, e)) return false;

    if (!parse(i, e, expose::json::quote_end)) return false;

    x = p.get();
    b = i;
    return true;
}

void serialize(expose::json::Exporter &e, const Ipv6Address &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(expose::json::Loader &l, Ipv6Address &x) {
    if (!parse(l.pos_, l.end_, x)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &l, Ipv6Address &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String, txt_type_ipv6,
                    txt_desc_ipv6);
}

// Ipv6Network /////////////////////////////////////////////////////////////////
static bool parse(const char *&b, const char *e, Ipv6Network &x) {
    const char *i = b;
    parser::Ipv6Network p;

    // Try parse a string with an encapsulated integer
    if (!parse(i, e, expose::json::quote_start)) return false;

    if (!p(i, e)) return false;

    if (!parse(i, e, expose::json::quote_end)) return false;

    x = p.get();
    b = i;
    return true;
}

void serialize(expose::json::Exporter &e, const Ipv6Network &i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(expose::json::Loader &l, Ipv6Network &x) {
    if (!parse(l.pos_, l.end_, x)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &l, Ipv6Network &x) {
    l.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_ipv6_network, txt_desc_ipv6_network);
}


// AsDecimalNumber /////////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const AsDecimalNumber &i) {
    BufStream<SBuf16> buf;
    int32_t tmp = i.get_int_value();
    signed_to_dec_rbuf(tmp, buf, false);
    copy_backward(buf.begin(), buf.end(), expose::json::CharOutIterator(e));
}

void serialize(expose::json::Loader &e, AsDecimalNumber &i) {
    int32_t tmp;
    if (!parse_int<int32_t>(e.pos_, e.end_, tmp)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
    i.set_value(tmp);
}

void serialize(expose::json::HandlerReflector &e, AsDecimalNumber &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, i.json_type_name(),
                    i.type_descr());
}

// AsBitMask ///////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const AsBitMask i) {
    hex_out hexer(e, 'a');
    uint8_t buf = 0;
    e.push_char('"');
    uint32_t index;
    for (index = 0; index < i.size_; ++index) {
        buf = (buf * 2) + (i.val_[index] ? 1 : 0);
        if ((index + 1) % 8 == 0) {
            hexer = buf;
            buf = 0;
        }
    }
    if (index % 8 != 0) {
        while (index++ % 8 != 0) {
            buf = buf << 1;
        }
        hexer = buf;
    }
    e.push_char('"');
}

void serialize(expose::json::Loader &e, AsBitMask val) {
    size_t hexSize = (val.size_ + 7) / 8;
    const char *pos = e.pos_;
    if (!parse(pos, e.end_, expose::json::Literal("\""))) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }
    uint8_t tmp;
    for (size_t i = 0; i < hexSize; ++i) {
        if (!parse_hex_char(pos, e.end_, tmp)) {
            VTSS_BASICS_TRACE(DEBUG) << "flag_error";
            e.flag_error();
            return;
        }
        uint8_t mask = 0x80;
        for (uint32_t j = 0; j < 8; ++j) {
            val.val_[i * 8 + j] = (tmp & mask);
            mask = mask / 2;
        }
    }
    if (!parse(pos, e.end_, expose::json::Literal("\""))) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }
    e.pos_ = pos;
}

void serialize(expose::json::HandlerReflector &e, AsBitMask val) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, "-", "TODO");
}

// BOOL ////////////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, AsBool i) {
    if ((i.val_uint8 && *i.val_uint8) || (i.val_uint16 && *i.val_uint16) ||
        (i.val_uint32 && *i.val_uint32) || (i.val_int8 && *i.val_int8) ||
        (i.val_int16 && *i.val_int16) || (i.val_int32 && *i.val_int32)) {
        serialize(e, expose::json::lit_true);
    } else {
        serialize(e, expose::json::lit_false);
    }
}

void serialize(expose::json::Loader &e, AsBool i) {
    parser::Int<int32_t, 10, 1> int_parser;

    if (parse(e.pos_, e.end_, expose::json::lit_true)) {
        i.set_value(true);
    } else if (parse(e.pos_, e.end_, expose::json::lit_false)) {
        i.set_value(false);
    } else if (int_parser(e.pos_, e.end_)) {  // Allow int to boolean conv.
        if (int_parser.get())
            i.set_value(true);
        else
            i.set_value(false);
    } else {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(expose::json::HandlerReflector &e, AsBool i) {
    bool b;
    serialize(e, b);
}

// AsCounter ///////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, AsCounter i) { serialize(e, i.t); }
void serialize(expose::json::Loader &e, AsCounter i) { serialize(e, i.t); }
void serialize(expose::json::HandlerReflector &e, AsCounter i) {
    serialize(e, i.t);
}

// AsDisplayString /////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, AsDisplayString s) {
    char *p = find(s.ds_, s.ds_ + s.size_, '\0');
    str my_str(s.ds_, p);
    serialize(e, my_str);
}

void serialize(expose::json::Loader &e, AsDisplayString s) {
    BufPtr b(s.ds_, s.ds_ + s.size_ - 1);
    const char *p = e.pos_;
    expose::json::StringDecodeCount counter;
    bool res = string_decoder(p, e.end_, &counter);
    if (!res || (counter.count_ < s.min_) || (counter.count_ >= s.size_)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }
    p = e.pos_;
    expose::json::StringDecodeCapture cap(&b);
    res = string_decoder(p, e.end_, &cap);
    if (!res || !cap.ok_) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }
    e.pos_ = p;
    *cap.pos_ = 0;
}

void serialize(expose::json::HandlerReflector &e, AsDisplayString s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, txt_type_string,
                    txt_desc_string);
}

// AsPasswordSetOnly
// /////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, AsPasswordSetOnly s) {
    str my_str(s.get_value_);
    serialize(e, my_str);
}

void serialize(expose::json::Loader &l, AsPasswordSetOnly s) {
    AsDisplayString ds(s.ds_, s.size_);
    serialize(l, ds);
}

void serialize(expose::json::HandlerReflector &e, AsPasswordSetOnly s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String,
                    "vtss_appl_password_t",
                    "Password string. The string can be set, but when reading "
                    "an empty string is returned.");
}

// AsInt ///////////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const AsInt i) {
    int32_t x = i.get_value();
    serialize(e, x);
}

void serialize(expose::json::Loader &e, AsInt i) {
    int32_t x;
    serialize(e, x);

    if (x < i.offset) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    } else if (i.val_uint8) {
        if (x > 0xff) {
            VTSS_BASICS_TRACE(DEBUG) << "flag_error";
            e.flag_error();
        }
    } else if (i.val_uint16) {
        if (x > 0xffff) {
            VTSS_BASICS_TRACE(DEBUG) << "flag_error";
            e.flag_error();
        }
    }

    i.set_value(x);
}

void serialize(expose::json::HandlerReflector &e, AsInt i) {
    int x = 0;
    serialize(e, x);
}

// AsIpv4 //////////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, AsIpv4 i) {
    Ipv4Address x(i.t);
    auto o = e.encoded_stream();
    o << x;
}

void serialize(expose::json::Loader &l, AsIpv4 i) {
    Ipv4Address x;
    if (!parse(l.pos_, l.end_, x)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }
    i.t = x.as_api_type();
}

void serialize(expose::json::HandlerReflector &h, AsIpv4 i) {
    h.type_terminal(expose::json::JsonCoreType::String,
                          txt_type_ipv4, txt_desc_ipv4);
}

// AsOctetString ///////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const AsOctetString i) {
    hex_out hexer(e, 'a');
    e.push_char('"');
    uint32_t index;
    for (index = 0; index < i.size_; ++index) {
        hexer = i.val_[index];
    }
    e.push_char('"');
}

void serialize(expose::json::Loader &e, AsOctetString val) {
    const char *pos = e.pos_;
    if (!parse(pos, e.end_, expose::json::Literal("\""))) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }
    uint32_t i;
    for (i = 0; i < val.size_; ++i) {
        if (!parse_hex_char(pos, e.end_, val.val_[i])) {
            VTSS_BASICS_TRACE(DEBUG) << "flag_error";
            e.flag_error();
            return;
        }
    }
    if (!parse(pos, e.end_, expose::json::Literal("\""))) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }
    e.pos_ = pos;
}

void serialize(expose::json::HandlerReflector &e, AsOctetString val) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_binary_string, txt_desc_binary_string);
}

// AsSnmpObjectIdentifier //////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const AsSnmpObjectIdentifier i) {
    auto o = e.encoded_stream();
    o << i;
}

void serialize(expose::json::Loader &l, AsSnmpObjectIdentifier val) {
    const char *pos = l.pos_;
    vtss::parser::Lit sep(".");
    parser::Int<uint32_t, 10> e;

    val.length = 0;

    if (!parse(pos, l.end_, expose::json::Literal("\""))) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }

    // Parse an optional starting dot
    (void)sep(pos, l.end_);

    // Check if we are finished here (empty OID)
    if (parse(pos, l.end_, expose::json::Literal("\""))) {
        goto OK;
    }

    // Parse the first oid-element
    if (e(pos, l.end_)) {
        val.buf[val.length++] = e.get();
    }

    // continue to parse dot-int-pair as long as possible
    while (val.length < val.max_length) {
        if (!vtss::parser::Group(pos, l.end_, sep, e)) break;

        val.buf[val.length++] = e.get();
    }

    if (!parse(pos, l.end_, expose::json::Literal("\""))) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        l.flag_error();
        return;
    }

 OK:
    l.pos_ = pos;
}

void serialize(expose::json::HandlerReflector &e, AsSnmpObjectIdentifier val) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, "vtss_snmp_oid_t",
                    "SNMP Object Identifier\n"
                    "The string starts with an optional dot followed by a "
                    "dot-seperated sequence of integers. 0-255 integers are "
                    "allowed.\n\n"
                    "Examples of allowed strings:\n"
                    "\".\"        -- Empty sequence\n"
                    "\".1\"       -- Sequence with one element of value 1\n"
                    "\"1\"        -- Sequence with one element of value 1\n"
                    "\"1.1.1.1\"  -- Sequence with four elements of value 1\n"
                    "\".1.1.1.1\" -- Sequence with four elements of value 1\n");
}

// AsPercent ///////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const AsPercent i) {
    serialize(e, i.val);
}
void serialize(expose::json::Loader &e, AsPercent i) { serialize(e, i.val); }
void serialize(expose::json::HandlerReflector &e, AsPercent i) {
    e.type_terminal(vtss::expose::json::JsonCoreType::Number, "uint8_t",
                    "A 8bit unsigned number");
}

// AsUnixTimeStampSeconds //////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const AsUnixTimeStampSeconds i) {
    char buf[128];

    struct tm *t;
    struct tm timeinfo;
    time_t _t = i.t;  // Warning not 2038 ready!
    t = gmtime_r(&_t, &timeinfo);

    if (t == 0) {
        VTSS_BASICS_TRACE(WARNING) << "gmtime failed";
        e.flag_error();
        return;
    }

    size_t res = strftime(buf, 128, "%Y-%m-%d %H:%M:%S", t);
    if (res == 0) {
        VTSS_BASICS_TRACE(WARNING) << "strftime failed";
        e.flag_error();
        return;
    }

    auto o = e.encoded_stream();
    o << str(buf);
}

void serialize(expose::json::Loader &e, AsUnixTimeStampSeconds i) {
    SBuf128 buf;
    struct tm t;

    if (!parse(e.pos_, e.end_, buf)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }

    const char *end = strptime(buf.begin(), "%Y-%m-%d %H:%M:%S", &t);
    if (end == nullptr || *end != 0) {
        VTSS_BASICS_TRACE(WARNING) << "strptime failed";
        e.flag_error();
        return;
    }

    i.t = 0;
    i.t = mktime(&t);
}

void serialize(expose::json::HandlerReflector &e, AsUnixTimeStampSeconds i) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, "time_t",
                    "Seconds since 1970/1/1");
}

// AsRowEditorState ////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const AsRowEditorState i) {
    serialize(e, i.val);
}
void serialize(expose::json::Loader &e, AsRowEditorState i) {
    serialize(e, i.val);
}
void serialize(expose::json::HandlerReflector &e, AsRowEditorState i) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String,
                    "vtss_appl_snmp_row_editor_state", "Not used for JSON");
}

// Binary //////////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, Binary s) {
    serialize(e, expose::json::quote_start);

    // Output the buffer content as hex-chars
    copy(s.buf, s.buf + s.max_len, hex_out(e, 'A'));
    serialize(e, expose::json::quote_end);
}

void serialize(expose::json::Loader &e, Binary s) {
    uint32_t valid_len = 0;

    // start with a qoute
    serialize(e, expose::json::quote_start);

    while (1) {
        uint8_t res;

        // Make sure that we do not do a buffer overflow
        if (valid_len >= s.max_len) {
            break;
        }

        // Try to consume a two-digit hex value from input
        if (parse_hex_char(e.pos_, e.end_, res)) {
            s.buf[valid_len++] = res;

        } else {
            break;
        }
    }

    // end the hext string
    serialize(e, expose::json::quote_end);
}

void serialize(expose::json::HandlerReflector &e, Binary s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_binary_string, txt_desc_binary_string);
}

// BinaryLen ///////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, BinaryLen s) {
    serialize(e, expose::json::quote_start);

    // Output the buffer content as hex-chars
    copy(s.buf, s.buf + s.valid_len, hex_out(e, 'A'));
    serialize(e, expose::json::quote_end);
}

void serialize(expose::json::Loader &e, BinaryLen s) {
    s.valid_len = 0;

    // start with a qoute
    serialize(e, expose::json::quote_start);

    while (1) {
        uint8_t res;

        // Make sure that we do not do a buffer overflow
        if (s.valid_len >= s.max_len) {
            break;
        }

        // Try to consume a two-digit hex value from input
        if (parse_hex_char(e.pos_, e.end_, res)) {
            s.buf[s.valid_len++] = res;

        } else {
            break;
        }
    }

    // end the hext string
    serialize(e, expose::json::quote_end);

    if (s.valid_len < s.min_len) e.flag_error();
}

void serialize(expose::json::HandlerReflector &e, BinaryLen s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String,
                    txt_type_binary_string, txt_desc_binary_string);
}

// FormatHex<uint8_t> //////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, FormatHex<uint8_t> s) {
    uint32_t i = s.t0;
    FormatHex<uint32_t> x(i, s.base, s.width_min, s.width_max, s.fill_char);
    serialize_formathex_uint32(e, x);
}

void serialize(expose::json::Loader &e, FormatHex<uint8_t> s) {
    VTSS_ASSERT(s.fill_char == '0');
    const char *i = e.pos_;
    const char *start = e.pos_;

    parser::Int<uint8_t, 16, 1> p;
    bool r = p(i, e.end_);

    if (!r) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }

    // check ranges
    size_t length = i - start;
    if (s.width_min && length < s.width_min) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }
    if (s.width_max && length > s.width_max) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
        return;
    }

    // char accepted! update pos pointer
    e.pos_ = i;
    s.t0 = p.get();
}

void serialize(expose::json::HandlerReflector &e, FormatHex<uint8_t> s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, "uint8_hex_t",
                    "A 8bit unsigned integer HEX encoded.");
}

// FormatHex<uint16_t> /////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, FormatHex<uint16_t> &s) {
    uint32_t i = s.t0;
    FormatHex<uint32_t> x(i, s.base, s.width_min, s.width_max, s.fill_char);
    serialize_formathex_uint32(e, x);
}

void serialize(expose::json::Loader & /*e*/,
               const FormatHex<uint16_t> & /*s*/) {
    VTSS_ASSERT(0);
}

void serialize(expose::json::HandlerReflector &e, FormatHex<uint16_t> &s) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, "uint16_hex_t",
                    "A 16bit unsigned integer HEX encoded.");
}

// FormatHex<uint32_t> /////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, FormatHex<uint32_t> &s) {
    serialize_formathex_uint32(e, s);
}

void serialize(expose::json::Loader &, const FormatHex<uint32_t> &) {
    VTSS_ASSERT(0);
}

void serialize(expose::json::HandlerReflector &e, const FormatHex<uint32_t> &) {
    e.type_terminal(vtss::expose::json::JsonCoreType::String, "uint32_hex_t",
                    "A 32bit unsigned integer HEX encoded.");
}

// str /////////////////////////////////////////////////////////////////////////
void serialize(expose::json::Exporter &e, const str &s) {
    if (!string_encode(e, s.begin(), s.end())) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

namespace expose {
namespace json {

// bool /////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const bool &i) {
    if (i) {
        serialize(e, lit_true);
    } else {
        serialize(e, lit_false);
    }
}

void serialize(Loader &e, bool &i) {
    if (parse(e.pos_, e.end_, lit_true)) {
        i = true;
    } else if (parse(e.pos_, e.end_, lit_false)) {
        i = false;
    } else {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, bool &i) {
    e.type_terminal(expose::json::JsonCoreType::Boolean, "vtss_bool_t",
                    "Boolean value");
}

// int8_t //////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const int8_t &i) {
    BufStream<SBuf8> buf;
    signed_to_dec_rbuf(i, buf, false);
    copy_backward(buf.begin(), buf.end(), CharOutIterator(e));
}

void serialize(Loader &e, int8_t &i) {
    if (!parse_int<int8_t>(e.pos_, e.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, int8_t &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, "int8_t",
                    "8 bit signed integer");
}

// int16_t /////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const int16_t &i) {
    BufStream<SBuf8> buf;
    signed_to_dec_rbuf(i, buf, false);
    copy_backward(buf.begin(), buf.end(), CharOutIterator(e));
}

void serialize(Loader &e, int16_t &i) {
    if (!parse_int<int16_t>(e.pos_, e.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, int16_t &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, "int16_t",
                    "16 bit signed integer");
}

// int32_t /////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const int32_t &i) {
    BufStream<SBuf16> buf;
    signed_to_dec_rbuf(i, buf, false);
    copy_backward(buf.begin(), buf.end(), CharOutIterator(e));
}

void serialize(Loader &e, int32_t &i) {
    if (!parse_int<int32_t>(e.pos_, e.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, int32_t &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, "int32_t",
                    "32 bit signed integer");
}

// int64_t ////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const int64_t &i) {
    BufStream<SBuf32> buf;
    signed_to_dec_rbuf(i, buf, false);
    copy_backward(buf.begin(), buf.end(), CharOutIterator(e));
}

void serialize(Loader &e, int64_t &i) {
    if (!parse_int<int64_t>(e.pos_, e.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, int64_t &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, "int64_t",
                    "64 bit signed integer");
}

// uint8_t /////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const uint8_t &i) {
    BufStream<SBuf8> buf;
    unsigned_to_dec_rbuf(i, buf);
    copy_backward(buf.begin(), buf.end(), CharOutIterator(e));
}

void serialize(Loader &e, uint8_t &i) {
    if (!parse_int<uint8_t>(e.pos_, e.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, uint8_t &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, "uint8_t",
                    "8 bit unsigned integer");
}

// uint16_t ////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const uint16_t &i) {
    BufStream<SBuf8> buf;
    unsigned_to_dec_rbuf(i, buf);
    copy_backward(buf.begin(), buf.end(), CharOutIterator(e));
}

void serialize(Loader &e, uint16_t &i) {
    if (!parse_int<uint16_t>(e.pos_, e.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, uint16_t &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, "uint16_t",
                    "16 bit unsigned integer");
}

// uint32_t ////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const uint32_t &i) {
    BufStream<SBuf16> buf;
    unsigned_to_dec_rbuf(i, buf);
    copy_backward(buf.begin(), buf.end(), CharOutIterator(e));
}

void serialize(Loader &e, uint32_t &i) {
    if (!parse_int<uint32_t>(e.pos_, e.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, uint32_t &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, "uint32_t",
                    "32 bit unsigned integer");
}

// uint64_t ////////////////////////////////////////////////////////////////////
void serialize(Exporter &e, const uint64_t &i) {
    BufStream<SBuf32> buf;
    unsigned_to_dec_rbuf(i, buf);
    copy_backward(buf.begin(), buf.end(), CharOutIterator(e));
}

void serialize(Loader &e, uint64_t &i) {
    if (!parse_int<uint64_t>(e.pos_, e.end_, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "flag_error";
        e.flag_error();
    }
}

void serialize(HandlerReflector &e, uint64_t &i) {
    e.type_terminal(expose::json::JsonCoreType::Number, "uint64_t",
                    "64 bit unsigned integer");
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

