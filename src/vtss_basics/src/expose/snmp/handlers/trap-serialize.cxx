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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "vtss/basics/expose/snmp/handlers/trap-serialize.hxx"

namespace vtss {
namespace expose {
namespace snmp {

static void add_variable(TrapHandler &h, AsnType::E type,
                         const unsigned char *buf, size_t length) {
    if (!h.notification_vars_.size()) {
        h.ok_ = false;
        return;
    }

    uint32_t oid_base_length_ = h.oid_base_.size();
    h.oid_base_.push_back(h.oid_element_);
    h.oid_base_.push(h.oid_index_);

    VTSS_BASICS_TRACE(DEBUG) << "New variable " << h.oid_base_
                             << " added to var-binding "
                             << (void *)&h.notification_vars_.back()
                             << " handler: " << (void *)&h;

    snmp_varlist_add_variable(&h.notification_vars_.back(),
                              (vtss_oid *)h.oid_base_.data(),
                              h.oid_base_.size(), type, buf, length);

    // clean-up
    while (h.oid_base_.size() > oid_base_length_) h.oid_base_.pop_back();
}

static void add_variable(TrapHandler &h, AsnType::E type, const char *buf,
                         size_t length) {
    add_variable(h, type, (const unsigned char *)buf, length);
}

void serialize(TrapHandler &h, const uint64_t &s) {
    counter64 c;
    c.high = s >> 32;
    c.low = s & 0xffffffff;

    add_variable(h, AsnType::Counter64, (u_char *)&c, sizeof(c));
}

void serialize(TrapHandler &h, const uint32_t &s) {
    add_variable(h, AsnType::Unsigned, (const char *)&s, sizeof(s));
}

void serialize(TrapHandler &h, const uint16_t &s) {
    uint32_t x = s;
    serialize(h, x);
}

void serialize(TrapHandler &h, const uint8_t &s) {
    uint32_t x = s;
    serialize(h, x);
}

void serialize(TrapHandler &h, const int64_t &s) {
    uint64_t c = s;
    serialize(h, c);
}

void serialize(TrapHandler &h, const int32_t &s) {
    add_variable(h, AsnType::Integer, (const char *)&s, sizeof(s));
}

void serialize(TrapHandler &h, const AsDecimalNumber &s) {
    int32_t x = s.get_int_value();
    serialize(h, x);
}

void serialize(TrapHandler &h, const int16_t &s) {
    int32_t x = s;
    serialize(h, x);
}

void serialize(TrapHandler &h, const int8_t &s) {
    int32_t x = s;
    serialize(h, x);
}

void serialize_enum(TrapHandler &h, const int32_t &i,
                    const vtss_enum_descriptor_t *d) {
    serialize(h, i);
}

void serialize(TrapHandler &h, const bool &s) {
    int32_t x = s ? 1 : 2;
    serialize(h, x);
}

void serialize(TrapHandler &h, const AsCounter &s) { serialize(h, s.t); }

void serialize(TrapHandler &h, const AsDisplayString &s) {
    uint32_t length = vtss::min(s.size_, (uint32_t)(strlen(s.ds_)));
    add_variable(h, AsnType::OctetString, s.ds_, length);
}

void serialize(TrapHandler &h, const AsPasswordSetOnly &s) {
    add_variable(h, AsnType::OctetString, "", 0);
}

void serialize(TrapHandler &h, const AsBitMask &bm) {
    uint32_t i;
    uint8_t tmp_ = 0;
    uint32_t tmp_size = ((bm.size_ + 7) / 8) * 8;

    Vector<char> buf(tmp_size);
    if (buf.size() != tmp_size) {
        h.ok_ = false;
        return;
    }

    for (i = 0; i < tmp_size; ++i) {
        tmp_ = tmp_ * 2 + ((i < bm.size_ && bm.val_[i]) ? 1 : 0);
        if ((i + 1) % 8 == 0) {
            buf[i / 8] = tmp_;
            tmp_ = 0;
        }
    }

    add_variable(h, AsnType::OctetString, buf.data(), tmp_size / 8);
}

void serialize(TrapHandler &h, const AsOctetString &s) {
    add_variable(h, AsnType::OctetString, s.val_, s.size_);
}

void serialize(TrapHandler &h, const BinaryLen &s) {
    add_variable(h, AsnType::OctetString, s.buf, s.valid_len);
}

static void serialize_ip(TrapHandler &h, uint32_t i) {
    unsigned char val[4];
    val[0] = (i & 0xFF000000) >> 24;
    val[1] = (i & 0xFF0000) >> 16;
    val[2] = (i & 0xFF00) >> 8;
    val[3] = (i & 0xFF);

    add_variable(h, AsnType::IpAddress, val, 4);
}

void serialize(TrapHandler &h, const Ipv4Address &s) {
    serialize_ip(h, s.as_api_type());
}

void serialize(TrapHandler &h, const AsIpv4 &s) { serialize_ip(h, s.t); }

void serialize(TrapHandler &h, const mesa_mac_t &s) {
    add_variable(h, AsnType::OctetString, s.addr, 6);
}

void serialize(TrapHandler &h, const mesa_ipv6_t &s) {
    add_variable(h, AsnType::OctetString, s.addr, 16);
}

void serialize(TrapHandler &h, const AsBool &s) {
    bool b = s.get_value();
    serialize(h, b);
}

// void serialize(TrapHandler &h, const AsRowEditorState &s);

void serialize(TrapHandler &h, const AsPercent &s) { serialize(h, s.val); }

void serialize(TrapHandler &h, const AsInt &s) {
    uint32_t x = s.get_value();
    int32_t y;

    if (x > 0x7fffffff) {
        VTSS_BASICS_TRACE(ERROR) << "Value too big: " << x;
        x = 0x7fffffff;
    }

    // cast to integer
    y = (int32_t)x;
    serialize(h, y);
}

void serialize(TrapHandler &h, const AsSnmpObjectIdentifier &s) {
    add_variable(h, AsnType::ObjectIdentifier, (const char *)s.buf, s.length * 4);
}

void serialize(TrapHandler &h, AsTimeStampSeconds &s) { serialize(h, s.t); }

void serialize(TrapHandler &h, AsUnixTimeStampSeconds &s) {
    struct tm *t;
    struct tm timeinfo;
    time_t _t = s.t;  // Warning not 2038 ready!
    t = gmtime_r(&_t, &timeinfo);

    if (t == 0) {
        VTSS_BASICS_TRACE(WARNING) << "gmtime failed";
        h.ok_ = false;
        return;
    }

    char buf[8];
    uint32_t year = 1900 + t->tm_year;
    buf[0] = (year >> 8) & 0xff;
    buf[1] = (year)&0xff;
    buf[2] = (t->tm_mon + 1) & 0xff;
    buf[3] = (t->tm_mday) & 0xff;
    buf[4] = (t->tm_hour) & 0xff;
    buf[5] = (t->tm_min) & 0xff;
    buf[6] = (t->tm_sec) & 0xff;
    buf[7] = 0;

    add_variable(h, AsnType::OctetString, buf, 8);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
