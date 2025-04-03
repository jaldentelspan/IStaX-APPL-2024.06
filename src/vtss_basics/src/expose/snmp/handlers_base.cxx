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

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include <math.h>
#include <time.h>
#include "vtss/basics/stream.hxx"
#include "vtss/basics/assert.hxx"
#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/snmp/handlers.hxx"

#include "vtss_os_wrapper_snmp.h"
extern "C" {
u_char *VTSS_SNMP_default_handler(struct variable *vp, vtss_oid *name,
                                  size_t *length, int exact, size_t *var_len,
                                  WriteMethod **write_method);
}

namespace vtss {
namespace expose {
namespace snmp {

namespace Action {
ostream &operator<<(ostream &o, const E e) {
#define CASE(X)  \
    case X:      \
        o << #X; \
        break
    switch (e) {
        CASE(reserve1);
        CASE(reserve2);
        CASE(action);
        CASE(commit);
        CASE(free);
        CASE(undo);
        CASE(finished_success);
        CASE(finished_failure);
    default:
        o << "<unknown(" << static_cast<int32_t>(e) << ")>";
        break;
    }
#undef CASE
    return o;
}
}  // namespace Action


// GET HANDLERS ////////////////////////////////////////////////////////////////
#define MAX_OCTET_STRING_SIZE 4096
static counter64 GLOBAL_COUNTER_VALUE;
static uint64_t GLOBAL_UINT64_VALUE;
static uint32_t GLOBAL_UINT32_VALUE;
static int32_t GLOBAL_INT32_VALUE;
static char GLOBAL_DISPLAY_STRING[MAX_OCTET_STRING_SIZE];

void serialize(GetHandler &a, AsCounter &s) {
    GLOBAL_COUNTER_VALUE.high = s.t >> 32;
    GLOBAL_COUNTER_VALUE.low  = s.t & 0xffffffff;

    a.length = sizeof(counter64);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_COUNTER_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, uint64_t &s) {
    GLOBAL_UINT64_VALUE = s;
    a.length = sizeof(uint64_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_UINT64_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, uint32_t &s) {
    GLOBAL_UINT32_VALUE = s;
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_UINT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, uint16_t &s) {
    GLOBAL_UINT32_VALUE = s;
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_UINT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, uint8_t &s) {
    GLOBAL_UINT32_VALUE = s;
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_UINT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, int64_t &s) {
    uint64_t tmp = (uint64_t)s;
    for (int i = 7; i >= 0; --i) {
        uint8_t c = tmp & 0x00000000000000FFLLU;
        GLOBAL_DISPLAY_STRING[i] = c;
        tmp >>= 8;
    }

    a.length = 8;
    a.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, int32_t &s) {
    GLOBAL_INT32_VALUE = s;
    a.length = sizeof(int32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_INT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, int16_t &s) {
    GLOBAL_INT32_VALUE = s;
    a.length = sizeof(int32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_INT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, int8_t &s) {
    GLOBAL_INT32_VALUE = s;
    a.length = sizeof(int32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_INT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize_enum(GetHandler &a, int32_t &s,
                    const vtss_enum_descriptor_t *d) {
    GLOBAL_UINT32_VALUE = s;
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_UINT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, bool &s) {
    GLOBAL_UINT32_VALUE = s ? 1 : 2;
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_UINT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &g, AsDisplayString &d) {
    VTSS_ASSERT(d.size_ < sizeof(GLOBAL_DISPLAY_STRING));
    strncpy(GLOBAL_DISPLAY_STRING, d.ds_, d.size_);
    g.length = strlen(GLOBAL_DISPLAY_STRING);
    g.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    g.state(HandlerState::DONE);
}

void serialize(GetHandler &g, AsPasswordSetOnly &d) {
    vtss::str s(d.get_value_);

    VTSS_ASSERT(s.size() < sizeof(GLOBAL_DISPLAY_STRING));
    strncpy(GLOBAL_DISPLAY_STRING, s.begin(), s.size());
    g.length = s.size();
    g.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    g.state(HandlerState::DONE);
}

void serialize(GetHandler &g, AsBitMask &bm) {
    uint32_t i;
    uint8_t tmp_ = 0;
    uint32_t tmp_size = ((bm.size_ + 7) / 8) * 8;

    VTSS_ASSERT(tmp_size < sizeof(GLOBAL_DISPLAY_STRING));

    for (i = 0; i < tmp_size; ++i) {
        tmp_ = tmp_ * 2 + ((i < bm.size_ && bm.val_[i]) ? 1 : 0);
        if ((i + 1) % 8 == 0) {
            GLOBAL_DISPLAY_STRING[i / 8] = tmp_;
            tmp_ = 0;
        }
    }

    g.length = tmp_size / 8;
    g.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    g.state(HandlerState::DONE);
}

void serialize(GetHandler &g, AsOctetString &val) {
    VTSS_ASSERT(val.size_ < sizeof(GLOBAL_DISPLAY_STRING));
    memcpy(GLOBAL_DISPLAY_STRING, val.val_, val.size_);
    g.length = val.size_;
    g.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    g.state(HandlerState::DONE);
}

void serialize(GetHandler &g, BinaryLen &val) {
    VTSS_ASSERT(val.valid_len < sizeof(GLOBAL_DISPLAY_STRING));
    memcpy(GLOBAL_DISPLAY_STRING, val.buf, val.valid_len);
    g.length = val.valid_len;
    g.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    g.state(HandlerState::DONE);
}

void serialize(GetHandler &g, BinaryU32Len &val) {
    VTSS_ASSERT(val.valid_len * 4 < sizeof(GLOBAL_DISPLAY_STRING));
    uint32_t tmp;
    for (int i = 0; i < val.valid_len; i++) {
        tmp = val.buf[i];
        GLOBAL_DISPLAY_STRING[i * 4 + 0] = (tmp & 0xFF000000) >> 24;
        GLOBAL_DISPLAY_STRING[i * 4 + 1] = (tmp & 0xFF0000) >> 16;
        GLOBAL_DISPLAY_STRING[i * 4 + 2] = (tmp & 0xFF00) >> 8;
        GLOBAL_DISPLAY_STRING[i * 4 + 3] = (tmp & 0xFF);
    }
    g.length = val.valid_len * 4;
    g.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    g.state(HandlerState::DONE);
}

void serialize(GetHandler &g, AsSnmpObjectIdentifier &val) {
    VTSS_ASSERT(val.length * 4 < sizeof(GLOBAL_DISPLAY_STRING));
    memcpy(GLOBAL_DISPLAY_STRING, val.buf, val.length * 4);
    g.length = val.length * 4;
    g.value = reinterpret_cast<uint8_t *>(GLOBAL_DISPLAY_STRING);
    g.state(HandlerState::DONE);
}

void serialize(GetHandler &a, Ipv4Address &s) {
    uint32_t tmp = s.as_api_type();
    GLOBAL_DISPLAY_STRING[0] = (tmp & 0xFF000000) >> 24;
    GLOBAL_DISPLAY_STRING[1] = (tmp & 0xFF0000) >> 16;
    GLOBAL_DISPLAY_STRING[2] = (tmp & 0xFF00) >> 8;
    GLOBAL_DISPLAY_STRING[3] = (tmp & 0xFF);
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_DISPLAY_STRING);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsIpv4 &s) {
    uint32_t tmp = s.t;
    GLOBAL_DISPLAY_STRING[0] = (tmp & 0xFF000000) >> 24;
    GLOBAL_DISPLAY_STRING[1] = (tmp & 0xFF0000) >> 16;
    GLOBAL_DISPLAY_STRING[2] = (tmp & 0xFF00) >> 8;
    GLOBAL_DISPLAY_STRING[3] = (tmp & 0xFF);
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_DISPLAY_STRING);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, mesa_mac_t &s) {
    memcpy(GLOBAL_DISPLAY_STRING, s.addr, 6);
    a.length = 6;
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_DISPLAY_STRING);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, mesa_ipv6_t &s) {
    memcpy(GLOBAL_DISPLAY_STRING, s.addr, 16);
    a.length = 16;
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_DISPLAY_STRING);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsBool &s) {
    GLOBAL_UINT32_VALUE = (s.get_value() ? 1 : 2);
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_UINT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsDecimalNumber &s) {
    GLOBAL_INT32_VALUE = s.get_int_value();
    a.length = sizeof(uint32_t);
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_INT32_VALUE);
    a.state(HandlerState::DONE);
}

void serialize(GetHandler &a, AsRowEditorState &s) { serialize(a, s.val); }

void serialize(GetHandler &a, AsPercent &s) { serialize(a, s.val); }

void serialize(GetHandler &a, AsUnixTimeStampSeconds &s) {
    struct tm *t;
    struct tm timeinfo;
    time_t _t = s.t;  // Warning not 2038 ready!
    t = gmtime_r(&_t, &timeinfo);

    if (t == 0) {
        VTSS_BASICS_TRACE(WARNING) << "gmtime failed";
        a.state(HandlerState::FAILED);
        return;
    }

    uint32_t year = 1900 + t->tm_year;
    GLOBAL_DISPLAY_STRING[0] = (year >> 8) & 0xff;
    GLOBAL_DISPLAY_STRING[1] = (year) & 0xff;
    GLOBAL_DISPLAY_STRING[2] = (t->tm_mon + 1) & 0xff;
    GLOBAL_DISPLAY_STRING[3] = (t->tm_mday) & 0xff;
    GLOBAL_DISPLAY_STRING[4] = (t->tm_hour) & 0xff;
    GLOBAL_DISPLAY_STRING[5] = (t->tm_min) & 0xff;
    GLOBAL_DISPLAY_STRING[6] = (t->tm_sec) & 0xff;
    GLOBAL_DISPLAY_STRING[7] = 0;

    a.length = 8;
    a.value = reinterpret_cast<uint8_t *>(&GLOBAL_DISPLAY_STRING);
    a.state(HandlerState::DONE);
}

// SET HANDLERS ////////////////////////////////////////////////////////////////
bool check_type(SetHandler &a, AsnType::E expect) {
    if (a.asn_type.data != expect) {
        VTSS_BASICS_TRACE(INFO) << "Wrong type. Actually: " << a.asn_type << "("
                                << (int)a.asn_type.data
                                << ") Expected: " << AsnType(expect) << "("
                                << (int)expect << ")";
        a.error_code(ErrorCode::wrongType, __FILE__, __LINE__);
        return false;
    }
    return true;
}

bool check_length(SetHandler &a, size_t l) {
    if (a.val_length != l) {
        VTSS_BASICS_TRACE(INFO) << "Wrong length. Actually: " << a.val_length
                                << " Expected: " << l
                                << " (type: " << a.asn_type << ")";
        a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
        return false;
    }
    return true;
}

template <typename T1, typename T2>
static void trace_update(const OidSequence &o, T1 p, T2 n) {
    VTSS_BASICS_TRACE(DEBUG) << "Update oid: " << o << " " << p << " -> " << n;
}

#define EXPECT_TYPE(EXPECT) \
    if (!check_type(a, EXPECT)) return;
#define EXPECT_LENGTH(EXPECT) \
    if (!check_length(a, EXPECT)) return;

void serialize(SetHandler &a, AsCounter &s) {
    EXPECT_TYPE(AsnType::Counter64);
    EXPECT_LENGTH(sizeof(counter64));

    counter64 *pval = (reinterpret_cast<counter64 *>(a.val));
    uint64_t tmp = pval->high;
    tmp = tmp << 32 | pval->low;

    trace_update(a.seq_, s, tmp);
    s.t = tmp;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, uint64_t &s) {
    EXPECT_TYPE(AsnType::OctetString);
    EXPECT_LENGTH(sizeof(counter64));

    counter64 *pval = (reinterpret_cast<counter64 *>(a.val));
    uint64_t tmp = ntohl(pval->high);
    tmp = tmp << 32 | ntohl(pval->low);

    trace_update(a.seq_, s, tmp);
    s = tmp;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, uint32_t &s) {
    EXPECT_TYPE(AsnType::Unsigned);
    EXPECT_LENGTH(sizeof(long unsigned));

    uint32_t tmp = *(reinterpret_cast<uint32_t *>(a.val));
    trace_update(a.seq_, s, tmp);
    s = tmp;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, uint16_t &s) {
    EXPECT_TYPE(AsnType::Unsigned);
    EXPECT_LENGTH(sizeof(long unsigned));

    uint32_t tmp = *(reinterpret_cast<uint32_t *>(a.val));
    if (tmp > 0xffff) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << tmp
                                << " expected INTEGER(0..65535)";
        a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    trace_update(a.seq_, s, tmp);
    s = static_cast<uint16_t>(tmp);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, uint8_t &s) {
    EXPECT_TYPE(AsnType::Unsigned);
    EXPECT_LENGTH(sizeof(long unsigned));

    uint32_t tmp = *(reinterpret_cast<uint32_t *>(a.val));
    if (tmp > 0xff) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << tmp
                                << " expected INTEGER(0..255)";
        a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    trace_update(a.seq_, s, tmp);
    s = static_cast<uint8_t>(tmp);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, int64_t &s) {
    EXPECT_TYPE(AsnType::OctetString);
    EXPECT_LENGTH(8);

    uint64_t tmp = 0;
    for (int i = 0; i < 8; ++i) {
        tmp <<= 8;
        tmp |= (uint8_t)a.val[i];
    }

    s = (int64_t)tmp;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, int32_t &s) {
    EXPECT_TYPE(AsnType::Integer);
    EXPECT_LENGTH(sizeof(long int));

    int32_t tmp = *(reinterpret_cast<uint32_t *>(a.val));
    trace_update(a.seq_, s, tmp);
    s = tmp;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, int16_t &s) {
    EXPECT_TYPE(AsnType::Integer);
    EXPECT_LENGTH(sizeof(long int));

    int32_t tmp = *(reinterpret_cast<int32_t *>(a.val));
    if (tmp < -32768 || tmp > 32767) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << tmp;
        a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    trace_update(a.seq_, s, tmp);
    s = static_cast<int16_t>(tmp);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, int8_t &s) {
    EXPECT_TYPE(AsnType::Integer);
    EXPECT_LENGTH(sizeof(long int));

    int32_t tmp = *(reinterpret_cast<int32_t *>(a.val));
    if (tmp < -128 || tmp > 127) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << tmp;
        a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    trace_update(a.seq_, s, tmp);
    s = static_cast<int8_t>(tmp);
    a.state(HandlerState::DONE);
}

void serialize_enum(SetHandler &a, int32_t &t,
                    const vtss_enum_descriptor_t *d) {
    EXPECT_TYPE(AsnType::Integer);
    EXPECT_LENGTH(sizeof(long int));

    int32_t tmp = *(reinterpret_cast<int32_t *>(a.val));
    int32_t n;
    if (!int_to_enum_int(tmp, d, n)) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected (enum) value: " << tmp;
        a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    trace_update(a.seq_, t, n);
    t = n;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, bool &s) {
    EXPECT_TYPE(AsnType::Integer);
    EXPECT_LENGTH(sizeof(long int));

    uint32_t tmp = *(reinterpret_cast<uint32_t *>(a.val));
    if (tmp < 1 || tmp > 2) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << tmp
                                << " expected INTEGER(1|2)";
        a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    bool val = (tmp == 1 ? true : false);
    trace_update(a.seq_, s, val);
    s = val;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, AsDisplayString &d) {
    EXPECT_TYPE(AsnType::OctetString);

    if (a.val_length >= d.size_ || a.val_length > 255) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected length: " << a.val_length
                                << " expected max length " << d.size_;
        a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    for (uint32_t i = 0; i < a.val_length; ++i) {
        if (reinterpret_cast<uint8_t *>(a.val)[i] > 126 ||
            reinterpret_cast<uint8_t *>(a.val)[i] < 32) {
            VTSS_BASICS_TRACE(INFO) << "WrongValue: "
                                    << (int)(reinterpret_cast<uint8_t *>(
                                               a.val)[i]);
            a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
            return;
        }
    }

    VTSS_BASICS_TRACE(INFO) << "Update oid: " << a.seq_ << " " << d << " -> "
                            << AsDisplayString(reinterpret_cast<char *>(a.val),
                                               a.val_length);

    strncpy(d.ds_, reinterpret_cast<char *>(a.val), a.val_length);
    d.ds_[a.val_length] = 0;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, AsPasswordSetOnly &d) {
    AsDisplayString ds(d.ds_, d.size_);
    serialize(a, ds);
}

void serialize(SetHandler &a, AsBitMask &bm) {
    EXPECT_TYPE(AsnType::OctetString);

    uint32_t tmp_size = (bm.size_ + 7) / 8;
    uint32_t i;

    if (a.val_length != tmp_size) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected length: " << a.val_length
                                << " expected length " << bm.size_;
        a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    uint8_t mask = 0x80;
    for (i = 0; i < bm.size_ && i < (a.val_length * 8); ++i) {
        bm.val_[i] = (a.val[i / 8] & mask) ? true : false;
        if (mask == 1)
            mask = 0x80;
        else
            mask = mask >> 1;
    }
    while (i < bm.size_) bm.val_[i++] = false;

    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, AsOctetString &val) {
    EXPECT_TYPE(AsnType::OctetString);
    EXPECT_LENGTH(val.size_);

    memcpy(val.val_, a.val, a.val_length);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, BinaryLen &val) {
    EXPECT_TYPE(AsnType::OctetString);
    if (a.val_length < val.min_len || a.val_length > val.max_len) {
        VTSS_BASICS_TRACE(INFO) << "Wrong length. Actually: " << a.val_length
                                << " Expected: [" << val.min_len << ", "
                                << val.max_len << "] (type: " << a.asn_type
                                << ")";
        a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    memcpy(val.buf, a.val, a.val_length);
    val.valid_len = a.val_length;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, BinaryU32Len &val) {
    EXPECT_TYPE(AsnType::OctetString);
    /* Return wrongLength when the value exceeds 'val.max_len' no matter it's in 4-byte alignment or not. */
    if (((a.val_length + 3) / 4) > val.max_len) {
        VTSS_BASICS_TRACE(INFO) << "Wrong length. Actually: "
                                << (a.val_length + 3) / 4 << " Expected: [0, "
                                << val.max_len << "] (type: " << a.asn_type
                                << ")";
        a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    uint32_t tmp;
    for (int i = 0; i < (a.val_length / 4); i++) {
        tmp = *(reinterpret_cast<uint32_t *>(a.val + i * 4));
        tmp = ntohl(tmp);
        val.buf[i] = tmp;
    }
    val.valid_len = a.val_length / 4;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, AsSnmpObjectIdentifier &val) {
    EXPECT_TYPE(AsnType::ObjectIdentifier);
    if ((a.val_length / 4) > val.max_length) {
        VTSS_BASICS_TRACE(INFO) << "Wrong length. Actually: "
                                << a.val_length / 4 << " Expected: [0, "
                                << val.max_length << "] (type: " << a.asn_type
                                << ")";
        a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    memcpy(val.buf, a.val, a.val_length);
    val.length = a.val_length / 4;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, Ipv4Address &s) {
    EXPECT_TYPE(AsnType::IpAddress);
    EXPECT_LENGTH(4);

    uint32_t tmp = *(reinterpret_cast<uint32_t *>(a.val));
    tmp = ntohl(tmp);
    trace_update(a.seq_, s, AsIpv4(tmp));
    s = Ipv4Address(tmp);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, AsIpv4 &s) {
    EXPECT_TYPE(AsnType::IpAddress);
    EXPECT_LENGTH(4);

    uint32_t tmp = *(reinterpret_cast<uint32_t *>(a.val));
    tmp = ntohl(tmp);
    trace_update(a.seq_, s, AsIpv4(tmp));
    s.t = tmp;
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, mesa_mac_t &s) {
    EXPECT_TYPE(AsnType::OctetString);
    EXPECT_LENGTH(6);

    mesa_mac_t *tmp = reinterpret_cast<mesa_mac_t *>(a.val);
    trace_update(a.seq_, MacAddress(s), MacAddress(*tmp));
    memcpy(s.addr, a.val, 6);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, mesa_ipv6_t &s) {
    EXPECT_TYPE(AsnType::OctetString);
    EXPECT_LENGTH(16);

    mesa_ipv6_t *tmp = reinterpret_cast<mesa_ipv6_t *>(a.val);
    trace_update(a.seq_, Ipv6Address(s), Ipv6Address(*tmp));
    memcpy(s.addr, a.val, 16);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, AsBool &s) {
    EXPECT_TYPE(AsnType::Integer);
    EXPECT_LENGTH(sizeof(long int));

    uint32_t tmp = *(reinterpret_cast<uint32_t *>(a.val));
    if (tmp < 1 || tmp > 2) {
        VTSS_BASICS_TRACE(INFO) << "Unexpected value: " << tmp
                                << " expected INTEGER(1|2)";
        a.error_code(ErrorCode::wrongValue, __FILE__, __LINE__);
        return;
    }

    bool val = (tmp == 1);
    trace_update(a.seq_, s, val);
    s.set_value(val);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, AsDecimalNumber &s) {
    EXPECT_TYPE(AsnType::Integer);
    EXPECT_LENGTH(sizeof(long int));

    int32_t tmp = *(reinterpret_cast<int32_t *>(a.val));

    trace_update(a.seq_, s, tmp);
    s.set_value(tmp);
    a.state(HandlerState::DONE);
}

void serialize(SetHandler &a, AsRowEditorState &s) { serialize(a, s.val); }

void serialize(SetHandler &a, AsPercent &s) { serialize(a, s.val); }

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
