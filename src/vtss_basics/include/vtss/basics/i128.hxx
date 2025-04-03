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

#ifndef __VTSS_BASICS_I128_HXX__
#define __VTSS_BASICS_I128_HXX__

#include <vtss/basics/common.hxx>
// #include <vtss/basics/stream.hxx>

namespace vtss {



struct I128 {
    // Class that enables I128 constructor to be called with uint64_t argument. See syntax below.
    struct AsUnSigned {
        AsUnSigned(uint64_t d) : data(d) {}
        uint64_t data;
    };

    int64_t high = 0;
    uint64_t low = 0;
    I128() = default;
    I128(AsUnSigned v) : high(0), low(v.data) {}  // Example: vtss::I128 x(vtss::I128::AsUnSigned(5));
    I128(int64_t v);
    I128(int64_t v1, uint64_t v2) : high(v1), low(v2) {}
    int64_t to_int64() { return low; }
    int32_t to_int32() { return low & 0xffffffff; }
    char to_char() { return low & 0xff; }
    I128& operator+=(I128 v);
    I128& operator-=(I128 v);
};

bool operator<(const I128& a, const I128& b);
bool operator>(const I128& a, const I128& b);
bool operator<=(const I128& a, const I128& b);
bool operator>=(const I128& a, const I128& b);
bool operator==(const I128& a, const I128& b);
bool operator!=(const I128& a, const I128& b);

I128 operator-(const I128& a);
I128 operator-(const I128& a, const I128& b);
I128 operator+(const I128& a, const I128& b);
// I128 operator*(const I128& a, const I128& b);
// I128 operator/(const I128& a, const I128& b);
I128 operator*(const I128& a, const uint32_t b);
I128 operator*(const I128& a, const int32_t b);
I128 operator/(const I128& a, const uint32_t b);
I128 operator%(const I128& a, const uint32_t b);
I128 operator%(const I128& a, const int32_t b);

// ostream& operator<<(ostream &o, const I128& v);

}  // namespace vtss

#endif  // __VTSS_BASICS_I128_HXX__
