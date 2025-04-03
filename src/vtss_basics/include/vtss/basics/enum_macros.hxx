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

#ifndef __VTSS_OVERLOAD_ENUM_TYPE__
#define __VTSS_OVERLOAD_ENUM_TYPE__

#include <vtss/basics/pragma.hxx>

#ifdef __cplusplus
#define VTSS_ENUM_INC(T)                                       \
clang_pragma("clang diagnostic push")                          \
clang_pragma("clang diagnostic ignored \"-Wunused-function\"") \
static inline T& operator++(T& x) {                            \
    x = (T) (x + 1);                                           \
    return x;                                                  \
}                                                              \
                                                               \
static inline T operator++(T& x, int) {                        \
    T old = x;                                                 \
    x = (T) (x + 1);                                           \
    return old;                                                \
}                                                              \
clang_pragma("clang diagnostic pop")

#define VTSS_ENUM_BITWISE(T)                                   \
clang_pragma("clang diagnostic push")                          \
clang_pragma("clang diagnostic ignored \"-Wunused-function\"") \
static inline T &operator |=(T &lhs, T rhs) {                  \
    lhs = (T) ((int)lhs | (int)rhs);                           \
    return lhs;                                                \
}                                                              \
static inline T operator |(T lhs, T rhs) {                     \
    return (T) ((int)lhs | (int)rhs);                          \
}                                                              \
static inline T &operator &=(T &lhs, T rhs) {                  \
    lhs = (T) ((int)lhs & (int)rhs);                           \
    return lhs;                                                \
}                                                              \
static inline T operator &(T lhs, T rhs) {                     \
    return (T) ((int)lhs & (int)rhs);                          \
}                                                              \
static inline T &operator ^=(T &lhs, T rhs) {                  \
    lhs = (T) ((int)lhs ^ (int)rhs);                           \
    return lhs;                                                \
}                                                              \
static inline T operator ^(T lhs, T rhs) {                     \
    return (T) ((int)lhs ^ (int)rhs);                          \
}                                                              \
static inline T operator ~(T rhs) {                            \
    return (T) (~((int)rhs));                                  \
}                                                              \
clang_pragma("clang diagnostic pop")


#else   // __cplusplus
#define VTSS_ENUM_INC(T)     /*lint -e19 -save */ /* Avoid "Useless declaration" */ /*lint -e19 -restore */
#define VTSS_ENUM_BITWISE(T) /*lint -e19 -save */ /* Avoid "Useless declaration" */ /*lint -e19 -restore */
#endif  // __cplusplus

#endif  // __VTSS_OVERLOAD_ENUM_TYPE__
