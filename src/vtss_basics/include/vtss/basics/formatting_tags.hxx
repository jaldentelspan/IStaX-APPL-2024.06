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

#ifndef __FORMATTING_TAGS_HXX__
#define __FORMATTING_TAGS_HXX__

#include "vtss/basics/meta.hxx"
#include "vtss/basics/common.hxx"
#include <math.h>
#include <stdio.h>

namespace vtss {

template <typename T>
struct RangeSpec;

template <typename T0>
struct FormatHex {
    FormatHex(T0 &_t, char _b, uint32_t _min = 0, uint32_t _max = 0,
              char _c = '0')
        : t0(_t), base(_b), width_min(_min), width_max(_max), fill_char(_c) {}

    FormatHex(  // implicit conversion constructor
            const FormatHex<typename meta::remove_const<T0>::type> &rhs)
        : t0(rhs.t0),
          base(rhs.base),
          width_min(rhs.width_min),
          width_max(rhs.width_max),
          fill_char(rhs.fill_char) {}

    T0 &t0;
    char base;
    uint32_t width_min;
    uint32_t width_max;
    char fill_char;
};

// struct AsBool {
//     explicit AsBool(uint8_t &t_) : t(t_) { }
//     uint8_t &t;
// };

struct AsBool {
    explicit AsBool(uint8_t &v)
        : val_uint8(&v),
          val_uint16(nullptr),
          val_uint32(nullptr),
          val_int8(nullptr),
          val_int16(nullptr),
          val_int32(nullptr) {}
    explicit AsBool(uint16_t &v)
        : val_uint8(nullptr),
          val_uint16(&v),
          val_uint32(nullptr),
          val_int8(nullptr),
          val_int16(nullptr),
          val_int32(nullptr) {}
    explicit AsBool(uint32_t &v)
        : val_uint8(nullptr),
          val_uint16(nullptr),
          val_uint32(&v),
          val_int8(nullptr),
          val_int16(nullptr),
          val_int32(nullptr) {}
    explicit AsBool(int8_t &v)
        : val_uint8(nullptr),
          val_uint16(nullptr),
          val_uint32(nullptr),
          val_int8(&v),
          val_int16(nullptr),
          val_int32(nullptr) {}
    explicit AsBool(int16_t &v)
        : val_uint8(nullptr),
          val_uint16(nullptr),
          val_uint32(nullptr),
          val_int8(nullptr),
          val_int16(&v),
          val_int32(nullptr) {}
    explicit AsBool(int32_t &v)
        : val_uint8(nullptr),
          val_uint16(nullptr),
          val_uint32(nullptr),
          val_int8(nullptr),
          val_int16(nullptr),
          val_int32(&v) {}
    uint8_t *val_uint8;
    uint16_t *val_uint16;
    uint32_t *val_uint32;
    int8_t *val_int8;
    int16_t *val_int16;
    int32_t *val_int32;
    void set_value(bool b) {
        if (val_uint8)
            *val_uint8 = b;
        else if (val_uint16)
            *val_uint16 = b;
        else if (val_uint32)
            *val_uint32 = b;
        else if (val_int8)
            *val_int8 = b;
        else if (val_int16)
            *val_int16 = b;
        else if (val_int32)
            *val_int32 = b;
    }
    bool get_value() const {
        if (val_uint8) return *val_uint8 != 0;
        if (val_uint16) return *val_uint16 != 0;
        if (val_uint32) return *val_uint32 != 0;
        if (val_int8) return *val_int8 != 0;
        if (val_int16) return *val_int16 != 0;
        if (val_int32) return *val_int32 != 0;
        return false;
    }
};

template <typename T>
struct AsLocaltime_HrMinSec {
    AsLocaltime_HrMinSec(T &t_) : t(t_) {}
    T &t;
};

template <typename T>
AsLocaltime_HrMinSec<T> as_localtime_hr_min_sec(T &t_) {
    return AsLocaltime_HrMinSec<T>(t_);
}

template <typename T>
struct AsTime_HrMinSec {
    AsTime_HrMinSec(T &t_) : t(t_) {}
    T &t;
};

template <typename T>
AsTime_HrMinSec<T> as_time_hr_min_sec(T &t_) {
    return AsTime_HrMinSec<T>(t_);
}

template <typename T>
struct AsTimeUs {
    AsTimeUs(T &t_) : t(t_) {}
    T &t;
};

template <typename T>
AsTimeUs<T> as_time_us(T &t_) {
    return AsTimeUs<T>(t_);
}

struct AsTimeStampSeconds {
    AsTimeStampSeconds(uint64_t &t_) : t(t_) {}
    uint64_t &t;
};

struct AsUnixTimeStampSeconds {
    AsUnixTimeStampSeconds(uint64_t &t_) : t(t_) {}
    uint64_t &t;
};

struct AsCounter {
    AsCounter(uint64_t &t_) : t(t_) {}
    uint64_t &t;
};

struct AsIpv4 {
    explicit AsIpv4(uint32_t &t_) : t(t_) {}
    uint32_t &t;
};

struct AsIpv4_const {
    explicit AsIpv4_const(const uint32_t &t_) : t(t_) {}
    const uint32_t &t;
};

struct BinaryLen {
    BinaryLen(uint8_t *b, uint32_t min, uint32_t max, uint32_t &v)
        : buf(b), min_len(min), max_len(max), valid_len(v) {}
    BinaryLen(uint8_t *b, uint32_t max, uint32_t &v)
        : buf(b), min_len(0), max_len(max), valid_len(v) {}
    uint8_t *buf;
    const uint32_t min_len;
    const uint32_t max_len;
    uint32_t &valid_len;
};

struct BinaryU32Len {
    BinaryU32Len(uint32_t *b, uint32_t min, uint32_t max, uint32_t &v)
        : buf(b), min_len(min), max_len(max), valid_len(v) {}
    BinaryU32Len(uint32_t *b, uint32_t max, uint32_t &v)
        : buf(b), min_len(0), max_len(max), valid_len(v) {}
    uint32_t *buf;
    const uint32_t min_len;
    const uint32_t max_len;
    uint32_t &valid_len;
};

struct Binary {
    Binary(uint8_t *b, uint32_t m) : buf(b), max_len(m) {}
    uint8_t *buf;
    const uint32_t max_len;
};

template <typename T>
FormatHex<T> hex(T &t, char _c = '0') {
    return FormatHex<T>(t, 'a', 0, 0, _c);
}

template <typename T>
FormatHex<T> HEX(T &t, char _c = '0') {
    return FormatHex<T>(t, 'A', 0, 0, _c);
}

template <uint32_t width, typename T>
FormatHex<T> hex_fixed(T &t, char _c = '0') {
    return FormatHex<T>(t, 'a', width, width, _c);
}

template <uint32_t width, typename T>
FormatHex<T> HEX_fixed(T &t, char _c = '0') {
    return FormatHex<T>(t, 'A', width, width, _c);
}

template <uint32_t min, uint32_t max, typename T>
FormatHex<T> hex_range(T &t, char _c = '0') {
    return FormatHex<T>(t, 'a', min, max, _c);
}

template <uint32_t min, uint32_t max, typename T>
FormatHex<T> HEX_range(T &t, char _c = '0') {
    return FormatHex<T>(t, 'A', min, max, _c);
}

template <unsigned S, typename T>
struct FormatLeft {
    FormatLeft(const T &_t, char f) : t(_t), fill(f) {}
    const T &t;
    const char fill;
};

template <unsigned S, typename T0>
FormatLeft<S, T0> left(const T0 &t0, char c = ' ') {
    return FormatLeft<S, T0>(t0, c);
}

template <unsigned S, typename T0>
struct FormatRight {
    FormatRight(const T0 &_t, char f) : t0(_t), fill(f) {}
    const T0 &t0;
    const char fill;
};

template <unsigned S, typename T0>
FormatRight<S, T0> right(const T0 &t0) {
    return FormatRight<S, T0>(t0, ' ');
}

template <unsigned S, typename T0>
FormatRight<S, T0> fill(const T0 &t0, char f = '0') {
    return FormatRight<S, T0>(t0, f);
}

template <typename T>
struct AsArray {
    AsArray(T *a, uint32_t s) : array(a), size(s) {}
    T *array;
    uint32_t size;
};

struct AsBitMask {
    AsBitMask(uint8_t *val, uint32_t size) : val_(val), size_(size) {}
    uint8_t *val_;
    uint32_t size_;
};

struct AsDisplayString {
    AsDisplayString(char *ds, uint32_t size, uint32_t min = 0) : ds_(ds), size_(size), min_(min) {}
    char *ds_;
    uint32_t size_;
    uint32_t min_;
};

struct AsPasswordSetOnly {
    AsPasswordSetOnly(char *ds, uint32_t size, const char *g = "", uint32_t min = 0)
        : ds_(ds), size_(size), get_value_(g), min_(min) {}
    char *ds_;
    uint32_t size_;
    const char *get_value_;
    uint32_t min_;
};

struct AsOctetString {
    AsOctetString(uint8_t *val, uint32_t size) : val_(val), size_(size) {}
    uint8_t *val_;
    uint32_t size_;
};

struct AsRowEditorState {
    AsRowEditorState(uint32_t &v) : val(v) {}
    uint32_t &val;
};

struct AsPercent {
    AsPercent(uint8_t &v) : val(v) {}
    uint8_t &val;
};

template <typename T, typename BOOL>
struct AsOptional {
    AsOptional(T &r, BOOL &e) : ref(r), exists(e) {}

    T &ref;
    BOOL &exists;
};

struct AsInt {
    explicit AsInt(uint32_t &v, uint8_t o = 0) : val_uint32(&v), val_uint16(0), val_uint8(0), offset(o) {}
    explicit AsInt(uint16_t &v, uint8_t o = 0) : val_uint32(0), val_uint16(&v), val_uint8(0), offset(o) {}
    explicit AsInt(uint8_t &v, uint8_t o = 0) : val_uint32(0), val_uint16(0), val_uint8(&v), offset(o) {}
    uint32_t *val_uint32;
    uint16_t *val_uint16;
    uint8_t *val_uint8;
    uint8_t offset;
    void set_value(int val) {
        if (val_uint8)
            *val_uint8 = val - offset;
        else if (val_uint16)
            *val_uint16 = val - offset;
        else if (val_uint32)
            *val_uint32 = val - offset;
    }
    uint32_t get_value() const {
        if (val_uint8) return static_cast<int>(*val_uint8 + offset);
        if (val_uint16) return static_cast<int>(*val_uint16 + offset);
        if (val_uint32) return static_cast<int>(*val_uint32 + offset);
        return 0;
    }
};

template <typename RANGE_SPEC>
bool range_check(AsInt i, RANGE_SPEC *t) {
    // In case AsInt has an offset, the function get_value return the compensated value
    return i.get_value() >= t->from && i.get_value() <= t->to;
}

struct AsErrorCode {
    AsErrorCode(mesa_rc e) : rc(e) {}
    mesa_rc rc;
};

struct AsSnmpObjectIdentifier {
    AsSnmpObjectIdentifier(uint32_t *b, uint32_t m, uint32_t &l)
        : buf(b), max_length(m), length(l) {}

    uint32_t *buf;
    uint32_t max_length;
    uint32_t &length;
};

struct AsDecimalNumber {
    AsDecimalNumber(double &i, int32_t d) : val(&i), decimals(d) {
        int dd = d>0?d:-d;
        sprintf(json_type_name_,"int32e%s%d_t",d>0?"":"_",dd);
        sprintf(snmp_type_name_,"Integer32e%s%d",d>0?"":"-",dd);
        sprintf(type_descr_,"32 bit signed integer times 10E%d",d);
    }
    void set_value(double d) {
        *val = d;
    }
    void set_value(int32_t d) {
        *val = ((double)d)*pow(10,decimals);
    }
    double get_value() const {
        return *val;
    }
    int32_t get_int_value() const {
        return static_cast<int32_t>((*val)*pow(10,-decimals)+0.5);
    }
    const char *json_type_name() const {
        return json_type_name_;
    }
    const char *snmp_type_name() const {
        return snmp_type_name_;
    }
    const char *type_descr() const {
        return type_descr_;
    }
private:
    double *val;
    const int32_t decimals;
    char json_type_name_[32];
    char snmp_type_name_[32];
    char type_descr_[64];
};
}  // namespace vtss

#endif /* __FORMATTING_TAGS_HXX__ */

