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

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

extern "C" {
}

#if defined(VTSS_OPSYS_LINUX) && defined(VTSS_SW_OPTION_WEB)
#include "web_api_linux.h"
#endif // defined(VTSS_OPSYS_LINUX)

#include "vtss/basics/config.h"
#include "vtss/basics/stream.hxx"
#if defined(VTSS_USE_API_HEADERS)
#include "vtss/appl/module_id.h"
#endif

#include "vtss/basics/print_fmt.hxx"
#include "vtss/basics/print_fmt_extra.hxx"

namespace vtss {
ostream& operator<<(ostream& o, const std::string& s) {
    print_fmt(o, "%s", s);
    return o;
}

ostream& operator<<(ostream& o, const ostreamBuf& s) {
    o.write(s.begin(), s.end());
    return o;
}

ostream& operator<<(ostream& o, const str& s) {
    print_fmt(o, "%s", s);
    return o;
}

ostream& operator<<(ostream& o, const Buffer& s) {
    o.write(s.begin(), s.end());
    return o;
}

ostream& operator<<(ostream& o, const char * s) {
    str ss(s);
    o.write(ss.begin(), ss.end());
    return o;
}

ostream& operator<<(ostream& o, const int8_t s) {
    o << static_cast<int32_t>(s);
    return o;
}

ostream& operator<<(ostream& o, const int16_t s) {
    o << static_cast<int32_t>(s);
    return o;
}

ostream& operator<<(ostream& o, const int32_t s) {
    ReverseBufStream<SBuf16> buf;
    signed_to_dec_rbuf(s, buf);
    o.write(buf.begin(), buf.end());
    return o;
}

ostream& operator<<(ostream& o, const int64_t s) {
    ReverseBufStream<SBuf32> buf;
    signed_to_dec_rbuf(s, buf);
    o.write(buf.begin(), buf.end());
    return o;
}

ostream& operator<<(ostream& o, const uint8_t s) {
    o << static_cast<uint32_t>(s);
    return o;
}

ostream& operator<<(ostream& o, const uint16_t s) {
    o << static_cast<uint32_t>(s);
    return o;
}

ostream& operator<<(ostream& o, const uint32_t s) {
    ReverseBufStream<SBuf16> buf;
    unsigned_to_dec_rbuf(s, buf);
    o.write(buf.begin(), buf.end());
    return o;
}

#if VTSS_SIZEOF_VOID_P == 4
ostream& operator<<(ostream& o, const long unsigned int s) {
    static_assert(sizeof(s) == 4, "Only meaningful when sizeof(long unsigned int) == 4");
    o << static_cast<uint32_t>(s);
    return o;
}
#endif

ostream& operator<<(ostream& o, const uint64_t s) {
    ReverseBufStream<SBuf32> buf;
    unsigned_to_dec_rbuf(s, buf);
    o.write(buf.begin(), buf.end());
    return o;
}

ostream& operator<<(ostream& o, const AsCounter s) {
    o << s.t;
    return o;
}

ostream& operator<<(ostream& o, const void *s) {
#if VTSS_SIZEOF_VOID_P == 8
    uint64_t p = (uint64_t)s;
#elif VTSS_SIZEOF_VOID_P == 4
    uint32_t p = (uint32_t)s;
#else
#error "Unknown size of void pointer"
#endif
    o << "0x" << hex(p);
    return o;
}

ostream& operator<<(ostream& o, FormatHex<const uint8_t> s) {
    uint32_t i = s.t0;
    o << FormatHex<const uint32_t>(i, s.base, s.width_min, s.width_max,
                                   s.fill_char);
    return o;
}

ostream& operator<<(ostream& o, FormatHex<const uint16_t> s) {
    uint32_t i = s.t0;
    o << FormatHex<const uint32_t>(i, s.base, s.width_min, s.width_max, s.fill_char);
    return o;
}

ostream& operator<<(ostream& o, FormatHex<const uint32_t> s) {
    ReverseBufStream<SBuf16> buf;
    unsigned_to_hex_rbuf(s.t0, buf, s.base, s.width_min, s.fill_char);
    o.write(buf.begin(), buf.end());
    return o;
}

ostream& operator<<(ostream& o, FormatHex<const uint64_t> s) {
    ReverseBufStream<SBuf16> buf;
    unsigned_to_hex_rbuf(s.t0, buf, s.base, s.width_min, s.fill_char);
    o.write(buf.begin(), buf.end());
    return o;
}

ostream& operator<<(ostream& o, const AsDecimalNumber &s) {
    ReverseBufStream<SBuf16> buf;
    int32_t tmp = s.get_int_value();
    signed_to_dec_rbuf(tmp, buf);
    o.write(buf.begin(), buf.end());
    return o;
}

ostream& operator<<(ostream& o, const Binary b) {
    o << "[";

    if (b.max_len)
        o << HEX(b.buf[0]);

    for (uint32_t i = 1; i < b.max_len; ++i)
        o << " " << HEX(b.buf[i]);

    o << "]";
    return o;
}

void print_hr_min_sec(ostream& o, uint64_t hr, uint32_t min, uint32_t sec) {
    ReverseBufStream<SBuf32> buf;

    // reverse print seconds
    unsigned_to_dec_rbuf_fill(sec, buf, 2, '0');
    buf.push(':');

    // reverse print minutes
    unsigned_to_dec_rbuf_fill(min, buf, 2, '0');
    buf.push(':');

    // reverse print hours
    unsigned_to_dec_rbuf_fill(hr, buf, 2, '0');

    o.write(buf.begin(), buf.end());
}


ostream& operator<<(ostream& o, AsLocaltime_HrMinSec<int> s) {
    time_t t = s.t;
    struct tm *ti;
    struct tm timeinfo;
    ti = localtime_r(&t, &timeinfo);

    print_hr_min_sec(o, ti->tm_hour, ti->tm_min, ti->tm_sec);
    return o;
}

ostream& operator<<(ostream& o, AsTime_HrMinSec<uint64_t> s) {
    uint64_t tmp = s.t;
    uint64_t sec, min, hr;
    ReverseBufStream<SBuf32> buf;

    sec = tmp % 60llu;
    tmp /= 60llu;

    min = tmp % 60llu;
    tmp /= 60llu;

    hr = tmp;

    print_hr_min_sec(o, hr, min, sec);
    return o;
}

// TODO, add test
ostream& operator<<(ostream& o, AsTimeUs<uint64_t> s) {
    uint64_t tmp = s.t;
    uint64_t us, ms, sec, min, hr, day, year;
    ReverseBufStream<SBuf64> buf;

    // 10000y-300d-00:00:00.000.000

    us = tmp % 1000llu;
    tmp /= 1000llu;
    unsigned_to_dec_rbuf_fill(us, buf, 3, '0');
    buf.push('.');

    ms = tmp % 1000llu;
    tmp /= 1000llu;
    unsigned_to_dec_rbuf_fill(ms, buf, 3, '0');
    buf.push('.');

    sec = tmp % 60llu;
    tmp /= 60llu;
    unsigned_to_dec_rbuf_fill(sec, buf, 2, '0');
    buf.push(':');

    min = tmp % 60llu;
    tmp /= 60llu;
    unsigned_to_dec_rbuf_fill(min, buf, 2, '0');
    buf.push(':');

    hr = tmp % 60llu;
    tmp /= 60llu;
    unsigned_to_dec_rbuf_fill(hr, buf, 2, '0');

    if (tmp == 0) {
        o.write(buf.begin(), buf.end());
        return o;
    }

    buf.push('-');
    buf.push('d');
    day = tmp % 365llu;
    tmp /= 365llu;
    unsigned_to_dec_rbuf(day, buf);

    if (tmp == 0) {
        o.write(buf.begin(), buf.end());
        return o;
    }

    buf.push('-');
    buf.push('y');
    year = tmp;
    unsigned_to_dec_rbuf(year, buf);

    o.write(buf.begin(), buf.end());
    return o;
}

bool fdstream::push(char val) {
    ssize_t res = ::write(fd_, &val, 1);
    if (res != 1) {
        error_ = errno;
        return false;
    }

    return true;
}

size_t fdstream::write(const char *b, const char *e) {
    ssize_t res = ::write(fd_, b, e - b);

    if (res < 1) {
        error_ = errno;
        return 0;
    }

    return res;
}

#if (defined(VTSS_OPSYS_LINUX) && defined(VTSS_SW_OPTION_WEB)) || defined(CYGPKG_ATHTTPD)
httpstream::httpstream(void *, const char *mime) {
    ok_ = true;
    cyg_httpd_start_chunked(mime);
}

bool httpstream::ok() const {
    return ok_;
}

bool httpstream::push(char val) {
    if (cyg_httpd_write_chunked(&val, sizeof(val)) == sizeof(val)) {
        return true;
    } else {
        ok_ = false;
        return false;
    }
}

size_t httpstream::write(const char *b, const char *e) {
    if (cyg_httpd_write_chunked(b, e - b)) {
        return true;
    } else {
        ok_ = false;
        return false;
    }
}

httpstream::~httpstream() {
    cyg_httpd_end_chunked();
}
#endif /* defined(VTSS_OPSYS_LINUX) || defined(CYGPKG_ATHTTPD) */

const char *BufPtrStream::cstring() {
    // We can not do anything with a null-buffer
    if (!buf_ || buf_->begin() == buf_->end()) {
        ok_ = false;
        return 0;
    }

    // Check if we have space for another char in the buffer
    if (pos_ != buf_->end()) {
        *pos_ = 0;  // add null termination
        return buf_->begin();
    }

    // The buffer is full, but we need to add a null termination! This will
    // cause an overflow. We will overwrite the last char with the zero
    // termination!
    ok_ = false;

    // We must adjust pos_ as we are overwriting the original content
    *(--pos_) = 0;

    return buf_->begin();
}

const char *BufPtrStream::cstringnl() {
    const bool has_nl = (*(pos_ - 1) == '\n');

    // We can not do anything with a null-buffer
    if (!buf_ || buf_->begin() == buf_->end()) {
        ok_ = false;
        return 0;
    }

    // Check if we have space for two extra chars in the buffer
    if (buf_->end() - pos_ >= 2 && !has_nl) {
        char *i = pos_;
        *i++ = '\n';  // add new-line
        *i++ = 0;     // add null termination
        return buf_->begin();
    } else if (buf_->end() - pos_ >= 1 && has_nl) {
        char *i = pos_;
        *i++ = 0;    // add null termination
        return buf_->begin();
    }

    // The buffer can not hold two extra chars, but we need to add a
    // new-line and null termination! This will cause an overflow. We will
    // overwrite the last two chars with the zero termination!
    ok_ = false;

    // make sure pos_ is pointed to a valid part of the buffer
    if (buf_->end() == pos_) pos_--;
    *pos_ = 0;  // Add null termination

    // buffer of length one!!!
    if (pos_ == buf_->begin()) return buf_->begin();

    *(--pos_) = '\n';

    return buf_->begin();
}

ostream& operator<<(ostream& o_, const mesa_mac_t &m) {
    print_fmt(o_, "%s", m);
    return o_;
}

ostream& operator<<(ostream& o, const MacAddress &m) {
    o << m.as_api_type();
    return o;
}

ostream& operator<<(ostream& o_, const AsIpv4 &i) {
    o_ << Ipv4Address(i.t);
    return o_;
}

ostream& operator<<(ostream& o_, const AsIpv4_const &i) {
    o_ << Ipv4Address(i.t);
    return o_;
}

ostream& operator<<(ostream& o_, const Ipv4Address &i) {
    BufStream<SBuf32> o;
    o << ((i.as_api_type() >> 24) & 0xff) << '.' <<
         ((i.as_api_type() >> 16) & 0xff) << '.' <<
         ((i.as_api_type() >>  8) & 0xff) << '.' <<
         (i.as_api_type() & 0xff);
    o_ << o;
    return o_;
}

ostream& operator<<(ostream& o_, const ::mesa_ipv4_network_t&i) {
    print_fmt(o_, "%s", i);
    return o_;
}

ostream& operator<<(ostream& o, const Ipv4Network &i) {
    o << i.as_api_type();
    return o;
}

ostream& operator<<(ostream& o_, const ::mesa_ipv6_t &x) {
    print_fmt(o_, "%s", x);
    return o_;
}
ostream& operator<<(ostream& o, const Ipv6Address &i) {
    o << i.as_api_type();
    return o;
}

ostream& operator<<(ostream& o, const ::mesa_ipv6_network_t &i) {
    print_fmt(o, "%s", i);
    return o;
}
ostream& operator<<(ostream& o, const Ipv6Network &i) {
    o << i.as_api_type();
    return o;
}

ostream& operator<<(ostream& o, const ::mesa_ip_addr_t &i) {
    print_fmt(o, "%s", i);
    return o;
}
ostream& operator<<(ostream& o, const IpAddress &i) {
    o << i.as_api_type();
    return o;
}

ostream& operator<<(ostream& o, const ::mesa_ip_network_t &i) {
    print_fmt(o, "%s", i);
    return o;
}
ostream& operator<<(ostream& o, const IpNetwork &i) {
    o << i.as_api_type();
    return o;
}

ostream& operator<<(ostream& o, const ::mesa_port_list_t &p) {
    print_fmt(o, "%s", p);
    return o;
}

ostream& operator<<(ostream& o, const WhiteSpace &ws) {
    for (unsigned int i = 0; i < ws.width; ++i) {
        o.push(' ');
    }
    return o;
}

ostream& operator<<(ostream &o, const AsBool &rhs) {
    o << (rhs.get_value()?"true":"false");
    return o;
}

ostream& operator<<(ostream &o, const AsBitMask &rhs) {
    for (uint32_t i = 0; i < rhs.size_; ++i) {
        o << (rhs.val_[i]?"1":"0");
    }
    return o;
}

ostream& operator<<(ostream &o, const AsDisplayString &rhs) {
    o << rhs.ds_;
    return o;
}

ostream& operator<<(ostream &o, const AsOctetString &rhs) {
    for (uint32_t i = 0; i < rhs.size_; ++i) {
        o << FormatHex<const uint8_t>(rhs.val_[i], 'a', 2, 2, '0');
    }
    return o;
}

ostream& operator<<(ostream &o, const BinaryLen &rhs) {
    for (uint32_t i = 0; i < rhs.valid_len; ++i) {
        o << FormatHex<const uint8_t>(rhs.buf[i], 'a', 2, 2, '0');
    }
    return o;
}

ostream& operator<<(ostream &o, const AsRowEditorState &rhs) {
    o << rhs.val;
    return o;
}

ostream& operator<<(ostream &o, const AsPercent &rhs) {
    o << rhs.val;
    return o;
}

ostream& operator<<(ostream &o, const AsInt &rhs) {
    o << rhs.get_value();
    return o;
}

ostream &operator<<(ostream &o, const ::vtss_inet_address_t &a) {
    print_fmt(o, "%s", a);
    return o;
}

#if defined(VTSS_USE_API_HEADERS)
extern "C" const char *error_txt(mesa_rc);
ostream &operator<<(ostream &o, AsErrorCode e) {
    o << "[rc:" << e.rc << " module:" << VTSS_RC_GET_MODULE_ID(e.rc)
      << " code:" << VTSS_RC_GET_MODULE_CODE(e.rc)
      << " desc: \"" << error_txt(e.rc) << "\"]";
    return o;
}
#else  // defined(VTSS_USE_API_HEADERS)
ostream &operator<<(ostream &o, AsErrorCode e) {
    o << "[rc:" << e.rc << "]";
    return o;
}
#endif  // defined(VTSS_USE_API_HEADERS)

ostream& operator<<(ostream &o, const AsSnmpObjectIdentifier &rhs) {
    if (rhs.length > 0)
        o << rhs.buf[0];

    for (uint32_t i = 1; i < rhs.length; ++i)
        o << "." << rhs.buf[i];

    return o;
}

#ifdef VTSS_BASICS_OPERATING_SYSTEM_LINUX
ostream& operator<<(ostream &o, const std::chrono::nanoseconds& ns) {
    o << ns.count();
    return o;
}
#endif


}  // namespace vtss
