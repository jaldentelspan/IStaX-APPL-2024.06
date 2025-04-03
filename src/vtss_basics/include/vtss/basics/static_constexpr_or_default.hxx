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

#ifndef __VTSS_BASICS_STATIC_CONSTEXPR_OR_DEFAULT_HXX__
#define __VTSS_BASICS_STATIC_CONSTEXPR_OR_DEFAULT_HXX__

#include "vtss/basics/meta.hxx"

// Compile time detect if a class has a constexpr.
//
// Explained in details here:
// http://stackoverflow.com/questions/15232758/detecting-constexpr-with-sfinae
// http://stackoverflow.com/questions/22284496/clang-issue-detecting-constexpr-function-pointer-with-sfinae
#define VTSS_BASICS_STATIC_CONSTEXPR_OR_DEFAULT(NAME, MEMBER, TYPE, DEFAULT)   \
    namespace NAME##_ {                                                        \
        template <bool>                                                        \
        struct sfinae_true : true_type {};                                     \
                                                                               \
        template <class T>                                                     \
        sfinae_true<(is_same<decltype(T::MEMBER), TYPE>::value)> check(int);   \
                                                                               \
        template <class>                                                       \
        false_type check(...);                                                 \
                                                                               \
        template <class T>                                                     \
        struct has : decltype(check<T>(0)) {};                                 \
                                                                               \
        template <typename T>                                                  \
        struct A {                                                             \
            static constexpr TYPE value = T::MEMBER;                           \
        };                                                                     \
                                                                               \
        template <typename T>                                                  \
        struct B {                                                             \
            static constexpr TYPE value = DEFAULT;                             \
        };                                                                     \
                                                                               \
        template <typename T>                                                  \
        struct select : public meta::__if<has<T>::value, A<T>, B<T>>::type {}; \
    }                                                                          \
    template <typename T>                                                      \
    struct NAME : public NAME##_::select<T> {}

#endif  // __VTSS_BASICS_STATIC_CONSTEXPR_OR_DEFAULT_HXX__
