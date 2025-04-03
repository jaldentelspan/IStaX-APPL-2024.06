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

#include "vtss/basics/arithmetic-overflow.hxx"

#include <limits>

namespace vtss {

#define IMPL_UNSIGNED(NAME, T1, T2, OP)           \
    T1 NAME(T1 a, T1 b, bool *overflow) {         \
        T2 _a = a;                                \
        T2 _b = b;                                \
        T2 r = _a OP _b;                          \
                                                  \
        if (r > std::numeric_limits<T1>::max()) { \
            *overflow = true;                     \
            return 0;                             \
        }                                         \
                                                  \
        *overflow = false;                        \
        return static_cast<T1>(r);                \
    }

IMPL_UNSIGNED(add_overflow, uint8_t, uint16_t, +);
IMPL_UNSIGNED(add_overflow, uint16_t, uint32_t, +);
IMPL_UNSIGNED(add_overflow, uint32_t, uint64_t, +);

IMPL_UNSIGNED(sub_overflow, uint8_t, uint16_t, -);
IMPL_UNSIGNED(sub_overflow, uint16_t, uint32_t, -);
IMPL_UNSIGNED(sub_overflow, uint32_t, uint64_t, -);

IMPL_UNSIGNED(mult_overflow, uint8_t, uint16_t, *);
IMPL_UNSIGNED(mult_overflow, uint16_t, uint32_t, *);
IMPL_UNSIGNED(mult_overflow, uint32_t, uint64_t, *);

#define IMPL_SIGNED(NAME, T1, T2, OP)             \
    T1 NAME(T1 a, T1 b, bool *overflow) {         \
        T2 _a = a;                                \
        T2 _b = b;                                \
        T2 r = _a OP _b;                          \
                                                  \
        if (r > std::numeric_limits<T1>::max()) { \
            *overflow = true;                     \
            return 0;                             \
        }                                         \
                                                  \
        if (r < std::numeric_limits<T1>::min()) { \
            *overflow = true;                     \
            return 0;                             \
        }                                         \
                                                  \
        *overflow = false;                        \
        return static_cast<T1>(r);                \
    }

IMPL_SIGNED(add_overflow, int8_t, int16_t, +);
IMPL_SIGNED(add_overflow, int16_t, int32_t, +);
IMPL_SIGNED(add_overflow, int32_t, int64_t, +);

IMPL_SIGNED(sub_overflow, int8_t, int16_t, -);
IMPL_SIGNED(sub_overflow, int16_t, int32_t, -);
IMPL_SIGNED(sub_overflow, int32_t, int64_t, -);

IMPL_SIGNED(mult_overflow, int8_t, int16_t, *);
IMPL_SIGNED(mult_overflow, int16_t, int32_t, *);
IMPL_SIGNED(mult_overflow, int32_t, int64_t, *);

uint64_t add_overflow(uint64_t a, uint64_t b, bool *overflow) {
    if (std::numeric_limits<uint64_t>::max() - a < b) {
        *overflow = true;
        return 0;
    } else {
        *overflow = false;
        return a + b;
    }
}

int64_t add_overflow(int64_t a, int64_t b, bool *overflow) {
    bool a_sign = (a < 0);
    bool b_sign = (b < 0);

    if (a_sign && b_sign) {
        // both are negative
        if (std::numeric_limits<int64_t>::min() - a > b) {
            *overflow = true;
            return 0;
        }

    } else if ((!a_sign) && (!b_sign)) {
        // both are positive
        if (std::numeric_limits<int64_t>::max() - a < b) {
            *overflow = true;
            return 0;
        }
    }

    *overflow = false;
    return a + b;
}

uint64_t sub_overflow(uint64_t a, uint64_t b, bool *overflow) {
    if (a < b) {
        *overflow = true;
        return 0;
    } else {
        *overflow = false;
        return a - b;
    }
}

int64_t sub_overflow(int64_t a, int64_t b, bool *overflow) {
    bool a_sign = (a < 0);
    bool b_sign = (b < 0);

    if (a_sign && (!b_sign)) {
        if (std::numeric_limits<int64_t>::min() + b > a) {
            *overflow = true;
            return 0;
        }

    } else if ((!a_sign) && b_sign) {
        if (std::numeric_limits<int64_t>::max() + b < a) {
            *overflow = true;
            return 0;
        }
    }

    *overflow = false;
    return a - b;
}

uint64_t mult_overflow(uint64_t a, uint64_t b, bool *overflow) {
    if (a == 0llu || b == 0llu) {
        *overflow = false;
        return 0;
    }

    uint64_t r = a * b;
    if (r / a != b) {
        *overflow = true;
        return 0;
    }

    *overflow = false;
    return r;
}

int64_t mult_overflow(int64_t a, int64_t b, bool *overflow) {
    if (a == 0llu || b == 0llu) {
        *overflow = false;
        return 0;
    }

    int64_t r = a * b;

    if (r / a != b) {
        *overflow = true;
        return 0;
    }

    *overflow = false;
    return r;
}


}  // namespace vtss

