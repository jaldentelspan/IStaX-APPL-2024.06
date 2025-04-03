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

#include <math.h>
#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "vtss/basics/assert.hxx"
#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/snmp/handlers.hxx"

namespace vtss {
namespace expose {
namespace snmp {

static parser::Lit lit_integer("integer");
static parser::Lit lit_unsigned("gauge");
static parser::Lit lit_string("string");
static parser::Lit lit_ipaddress("ipaddress");

template<int N>
struct snmpOctetString : public parser::ParserBase {
    bool operator()(const char *& b, const char * e) {
        const char * _b = b;
        size_ = 0;
        parser::Lit litOctet("octet");
        parser::Lit space(" ");
        parser::Lit quote("\"");
        parser::Int<unsigned char, 16, 1, 2> octet;

        if (Group(b, e, litOctet, space, quote)) {
            while (size_ < N && octet(b, e)) {
                octetString[size_]=octet.get();
                size_++;
                if (quote(b, e)) {
                    return true;
                }
                if (!space(b, e)) {
                    b = _b;
                    return false;
                }
            }
            return true;
        }

        b = _b;
        return false;
    }
    const uint8_t *get() {
        return octetString;
    }
    uint8_t octetString[N];
    uint32_t size_;
};

template<int N>
struct snmpString : public parser::ParserBase {
    bool operator()(const char *& b, const char * e) {
        const char * _b = b;
        size_ = 0;
        parser::Lit litString("string");
        parser::Lit space(" ");
        parser::String<N> string_(N-1);

        if (Group(b, e, litString, space, string_)) {
            VTSS_BASICS_TRACE(NOISE) << "Parsed: " << string_.get();
            memcpy(string, string_.get(), string_.size_);
            size_ = string_.size_;
            return true;
        }

        b = _b;
        return false;
    }
    const char *get() {
        return string;
    }
    char string[N];
    uint32_t size_;
};

// GET HANDLERS ////////////////////////////////////////////////////////////////
void serialize(GetHandler &a, uint64_t  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "VTSSUnsigned64";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsCounter &s) {
    serialize(a, s.t);
}

void serialize(GetHandler &a, uint32_t  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "Unsigned32";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, uint16_t  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "Integer16";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, uint8_t  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "Unsigned32";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, bool  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "Integer32";
    *a.out_ << "\n";
    *a.out_ << (s?1:2) << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, int64_t  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "VTSSsigned64";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, int32_t  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "Integer32";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsDecimalNumber  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "Integer32";
    *a.out_ << "\n";
    *a.out_ << s.get_int_value() << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, int8_t  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "Integer8";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, int16_t  &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "Integer16";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize_enum(GetHandler &h, int32_t &i,
                    const vtss_enum_descriptor_t *d) {
    *h.out_ << h.oid_seq_out() << "\n";
    print_enum_name(*h.out_, d);
    *h.out_ << "\n";
    *h.out_ << i << "\n";
    h.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsDisplayString &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "string\n\"" << s.ds_ << "\"\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsBitMask &s) {
    uint8_t tmp_ = 0;
    *a.out_ << a.oid_seq_out() << "\n" << "octet\n";

    uint32_t i;
    for (i = 0; i < ((s.size_+7)/8)*8; ++i) {
        tmp_ = tmp_*2 + ((i < s.size_ && s.val_[i])?1:0);
        if ((i+1)%8 == 0) {
            *a.out_ << FormatHex<uint8_t>(tmp_, 'a', 2, 2);
            if (i+1 < s.size_) {
                *a.out_ << " ";
            }
            tmp_ = 0;
        }
    }
    *a.out_ << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsOctetString &s) {
    *a.out_ << a.oid_seq_out() << "\n"
            << "octet";
    *a.out_ << "\n";

    uint32_t i;
    for (i = 0; i < s.size_; ++i) {
        *a.out_ << FormatHex<uint8_t>(s.val_[i], 'a', 2, 2);
        if (i+1 < s.size_) {
            *a.out_ << " ";
        }
    }
    *a.out_ << "\n";

    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, Ipv4Address &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "IpAddress";
    *a.out_ << "\n";
    *a.out_ << s << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsIpv4 &s) {
    *a.out_ << a.oid_seq_out() << "\n" << "IpAddress";
    *a.out_ << "\n";
    *a.out_ << Ipv4Address(s.t) << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, mesa_mac_t &s) {
    *a.out_ << a.oid_seq_out() << "\n"
            << "octet";
    *a.out_ << "\n";
    *a.out_ << FormatHex<uint8_t>(s.addr[0], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[1], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[2], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[3], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[4], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[5], 'a', 2, 2) << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, mesa_ipv6_t &s) {
    *a.out_ << a.oid_seq_out() << "\n"
            << "octet";
    *a.out_ << "\n";
    *a.out_ << FormatHex<uint8_t>(s.addr[0], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[1], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[2], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[3], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[4], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[5], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[6], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[7], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[8], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[9], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[10], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[11], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[12], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[13], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[14], 'a', 2, 2) << " "
            << FormatHex<uint8_t>(s.addr[15], 'a', 2, 2) << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsBool &s) {
    *a.out_ << a.oid_seq_out() << "\n"
            << "Integer32";
    *a.out_ << "\n";
    *a.out_ << (s.get_value() ? 1 : 2) << "\n";
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsRowEditorState &s) {
    serialize(a, s.val);
}

void serialize(GetHandler &a, AsPercent &s) {
    serialize(a, s.val);
}

// SET HANDLERS ////////////////////////////////////////////////////////////////
void serialize(SetHandler &a, uint32_t &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<uint32_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_unsigned, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG)
                << "Failed to parse value \""
                << a.value_set_
                << "\" as Unsigned32";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    a.state(HandlerState::DONE);
    s = i.get();
}

void serialize(SetHandler &a, uint64_t &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<uint64_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_unsigned, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG)
                << "Failed to parse value \""
                << a.value_set_
                << "\" as Unsigned64";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    a.state(HandlerState::DONE);
    s = i.get();
}

void serialize(SetHandler &a, uint16_t &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<uint16_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_unsigned, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << a.value_set_ <<
            " as Integer16";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    s = i.get();
    a.state(HandlerState::DONE);
    VTSS_BASICS_TRACE(DEBUG) << "Value parsed: " << s;
}

void serialize(SetHandler &a, uint8_t &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<uint8_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_unsigned, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << a.value_set_ <<
            " as Unsigned8";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Value parsed";
    a.state(HandlerState::DONE);
    s = i.get();
}

void serialize(SetHandler &a, bool &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<uint16_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_integer, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << a.value_set_ <<
            " as TruthValue";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    if (i.get() != 1 && i.get() != 2) {
        VTSS_BASICS_TRACE(DEBUG) << "Invalid value " << a.value_set_ <<
            " as TruthValue";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Value parsed";
    a.state(HandlerState::DONE);
    s = (i.get() == 1);
}

void serialize(SetHandler &a, int64_t &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<int64_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_unsigned, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG)
                << "Failed to parse value \""
                << a.value_set_
                << "\" as Integer64";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    a.state(HandlerState::DONE);
    s = i.get();
}

void serialize(SetHandler &a, int32_t  &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<int32_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_integer, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << a.value_set_ <<
            " as Integer32";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Value parsed: " << s << " -> " << i.get();
    a.state(HandlerState::DONE);
    s = i.get();
}

void serialize(SetHandler &a, AsDecimalNumber  &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<int32_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_integer, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << a.value_set_ <<
            " as Integer32";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Value parsed: " << s << " -> " << i.get();
    a.state(HandlerState::DONE);
    s.set_value(i.get());
}

void serialize(SetHandler &a, int8_t  &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<int8_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_integer, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << a.value_set_ <<
            " as Integer8";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Value parsed";
    a.state(HandlerState::DONE);
    s =  i.get();
}

void serialize(SetHandler &a, int16_t  &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<int16_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_integer, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << a.value_set_ <<
            " as Integer16";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Value parsed";
    a.state(HandlerState::DONE);
    s =  i.get();
}

void serialize_enum(SetHandler &h, int32_t &t,
                    const vtss_enum_descriptor_t *d) {
    const char *b = h.value_set_.begin();
    const char *e = h.value_set_.end();

    parser::Int<int32_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_integer, spaces, i)) {
        // TODO, trace which enum!
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << h.value_set_ <<
            " as enum";
        h.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    h.state(HandlerState::DONE);
    t = i.get();
    return;
}

void serialize(SetHandler &a, AsDisplayString &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();

    parser::String<256> string(s.size_);
    parser::SequenceNR<char> spaces(' ', 1);

    VTSS_BASICS_TRACE(DEBUG) << "Got: " << a.value_set_;
    if (!parser::Group(b, e, lit_string, spaces, string)) {
        VTSS_BASICS_TRACE(DEBUG) << "Value too long: " << a.value_set_;
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }
    VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << string.get();
    strncpy(s.ds_, string.get(), s.size_);
    a.state(HandlerState::DONE);
    return;
}

void unmask(uint8_t *to, const uint8_t *from, uint32_t size) {
    uint8_t mask = 0x80;
    uint32_t i;
    for (i = 0; i < size; i++) {
        to[i] = (from[i/8] & mask);
        if ((i+1)%8 == 0) {
            mask = 0x80;
        } else {
            mask = mask>>1;
        }
    }
}

void serialize(SetHandler &a, AsBitMask &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();

    VTSS_BASICS_TRACE(DEBUG) << "Got: " << a.value_set_;

    snmpOctetString<256> octetString;
    if (octetString(b, e) && octetString.size_ == (s.size_+7)/8) {
        unmask(s.val_, octetString.get(), s.size_);
        VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << s;
        a.state(HandlerState::DONE);
        return;
    }

    snmpString<256> snmpString;
    if (snmpString(b, e) && snmpString.size_ == (s.size_+7)/8) {
        unmask(s.val_, static_cast<const uint8_t*>(
            static_cast<const void*>(snmpString.get())), s.size_);
        VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << s;
        a.state(HandlerState::DONE);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Invalid octet string: " << a.value_set_;
    a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
    return;
}

snmpOctetString<256> octetString;
snmpString<256> tmpSnmpString;

void serialize(SetHandler &a, AsOctetString &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();

    VTSS_BASICS_TRACE(DEBUG) << "Got: " << a.value_set_;

    if (octetString(b, e) && octetString.size_ == s.size_) {
        memcpy(s.val_, octetString.get(), s.size_);
        VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << s;
        a.state(HandlerState::DONE);
        return;
    }

    if (tmpSnmpString(b, e) && tmpSnmpString.size_ == s.size_) {
        memcpy(s.val_, tmpSnmpString.get(), s.size_);
        VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << s;
        a.state(HandlerState::DONE);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Invalid octet string: " << a.value_set_;
    a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
    return;
}

void serialize(SetHandler &a, Ipv4Address &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();

    parser::IPv4 ipaddress;
    parser::SequenceNR<char> spaces(' ', 1);

    VTSS_BASICS_TRACE(DEBUG) << "Got: " << a.value_set_;
    if (!parser::Group(b, e, lit_ipaddress, spaces, ipaddress)) {
        VTSS_BASICS_TRACE(DEBUG) << "Invalid ipaddress: " << a.value_set_;
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }
    VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << ipaddress.ip;
    s = ipaddress.ip;
    a.state(HandlerState::DONE);
    return;
}

void serialize(SetHandler &a, AsIpv4 &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();

    parser::IPv4 ipaddress;
    parser::SequenceNR<char> spaces(' ', 1);

    VTSS_BASICS_TRACE(DEBUG) << "Got: " << a.value_set_;
    if (!parser::Group(b, e, lit_ipaddress, spaces, ipaddress)) {
        VTSS_BASICS_TRACE(DEBUG) << "Invalid ipaddress: " << a.value_set_;
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << ipaddress.ip;
    s.t = ipaddress.ip.as_api_type();
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, mesa_mac_t &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();

    VTSS_BASICS_TRACE(DEBUG) << "Got: " << a.value_set_;

    snmpOctetString<256> octetString;
    if (octetString(b, e) && octetString.size_ == 6) {
        memcpy(s.addr, octetString.get(), 6);
        VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << s;
        a.state(HandlerState::DONE);
        return;
    }

    snmpString<256> snmpString;
    if (snmpString(b, e) && snmpString.size_ == 6) {
        memcpy(s.addr, snmpString.get(), 6);
        VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << s;
        a.state(HandlerState::DONE);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Invalid macaddress: " << a.value_set_;
    a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
}

void serialize(SetHandler &a, mesa_ipv6_t &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();

    VTSS_BASICS_TRACE(DEBUG) << "Got: " << a.value_set_;

    snmpOctetString<256> octetString;
    if (octetString(b, e) && octetString.size_ == 16) {
        memcpy(s.addr, octetString.get(), 16);
        VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << s;
        a.state(HandlerState::DONE);
        return;
    }

    snmpString<256> snmpString;
    if (snmpString(b, e) && snmpString.size_ == 16) {
        memcpy(s.addr, snmpString.get(), 16);
        VTSS_BASICS_TRACE(DEBUG) << "Parsed: " << s;
        a.state(HandlerState::DONE);
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Invalid ipv6Address: " << a.value_set_;
    a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
}

void serialize(SetHandler &a, AsBool  &s) {
    const char *b = a.value_set_.begin();
    const char *e = a.value_set_.end();
    parser::Int<uint16_t, 10> i;
    parser::SequenceNR<char> spaces(' ', 1);

    if (!parser::Group(b, e, lit_integer, spaces, i)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse value " << a.value_set_ <<
            " as TruthValue";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }

    if (i.get() != 1 && i.get() != 2) {
        VTSS_BASICS_TRACE(DEBUG) << "Invalid value " << a.value_set_ <<
            " as TruthValue";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return;
    }
    VTSS_BASICS_TRACE(DEBUG) << "Value parsed";
    a.state(HandlerState::DONE);
    s.set_value(i.get() == 1);
}

void serialize(SetHandler &a, AsRowEditorState &s) {
    serialize(a,s.val);
}

void serialize(SetHandler &a, AsPercent &s) {
    serialize(a,s.val);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
