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

#include "vtss/basics/i128.hxx"

namespace vtss {

I128::I128(int64_t v)
{  
    if (v < 0) {
        high = -1;
    }
    else {
        high = 0;
    }
    low = ((uint64_t) v);
}

bool operator<(const I128& a, const I128& b) {
    return (a.high < b.high) || ((a.high == b.high) && (a.low < b.low));
}

bool operator>(const I128& a, const I128& b) {
    return (a.high > b.high) || ((a.high == b.high) && (a.low > b.low));
}

bool operator<=(const I128& a, const I128& b) {
    return (a.high < b.high) || ((a.high == b.high) && (a.low <= b.low));
}

bool operator>=(const I128& a, const I128& b) {
    return (a.high > b.high) || ((a.high == b.high) && (a.low >= b.low));
}

bool operator==(const I128& a, const I128& b) {
    return a.high == b.high && a.low == b.low;
}

bool operator!=(const I128& a, const I128& b) {
    return a.high != b.high || a.low != b.low;
}

I128 operator+(const I128& a, const I128& b)
{
    I128 tmp = a;

    tmp.low += b.low;
    if (tmp.low < b.low) {
        tmp.high += (b.high + 1);
    }
    else {
        tmp.high += b.high;
    }
    return tmp;
}

I128& I128::operator+=(I128 v)
{
    return (*this = *this + v);
}

I128 operator-(const I128& a)
{
    I128 tmp = a;
    
    tmp.low = ~tmp.low;
    tmp.high = ~tmp.high;
    
    tmp.low++;
    if (tmp.low == 0) tmp.high++;
    
    return tmp;
}

I128 operator-(const I128& a, const I128& b)
{ 
    return a + (-b);
}

I128& I128::operator-=(I128 v)
{
    return (*this = *this - v);
}

I128 operator*(const I128& a, const uint32_t b)
{
    uint64_t res_high = a.high * b;
    uint64_t intermidate = (a.low >> 32) * b;
    uint64_t res_low = (a.low & 0xFFFFFFFFL) * b;

    return I128(res_high, res_low) + I128(intermidate >> 32, intermidate << 32);
}

I128 operator*(const I128& a, const int32_t b)
{
    if (b < 0)
       return -(a * (uint32_t)(-b));
    else
       return a * (uint32_t)b;
}

I128 operator/(const I128& a, const uint32_t b)
{
    if (a.high < 0) {
        return -(-a / b);
    }
    else {
        uint64_t res_high = a.high / b;
        uint64_t intermidate1 = ((a.high % b) << 32) + (a.low >> 32);
        uint64_t res_low_msb_part = intermidate1 / b;
        uint64_t intermidate2 = ((intermidate1 % b) << 32) + (a.low & 0xFFFFFFFFL);
        uint32_t res_low_lsb_part = intermidate2 / b;
        return I128(res_high, ((res_low_msb_part << 32) + res_low_lsb_part));
    }
}

I128  operator%(const I128& a, const u32 b)
{
    I128 res;
    res = a -(a/b)*b;
    if (res < 0) res = res + I128(b);
    return res;
}

I128  operator%(const I128& a, const i32 b)
{
    I128 res;
    if (b < 0)  {
        res = (a -(a/(-b))*(-b));
        if (res > 0) res = res + I128(b);
    } else {
        res = (a -(a/b)*b);
        if (res < 0) res = res + I128(b);
    }
    return res;
}

}  // namespace vtss
