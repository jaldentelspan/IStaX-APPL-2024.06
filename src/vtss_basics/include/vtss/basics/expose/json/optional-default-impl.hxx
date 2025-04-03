/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_JSON_OPTIONAL_DEFAULT_IMPL_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_OPTIONAL_DEFAULT_IMPL_HXX__

#include <type_traits>
#include <vtss/basics/meta.hxx>

namespace vtss {
namespace expose {
namespace json {

namespace optional_default_impl_details {
// Compile time detect if a class has a constexpr.
//
// Explained in details here:
// http://stackoverflow.com/questions/15232758/detecting-constexpr-with-sfinae
// http://stackoverflow.com/questions/22284496/clang-issue-detecting-constexpr-function-pointer-with-sfinae
template <bool>
struct sfinae_true : true_type {};

template <class T>
sfinae_true<(is_same<decltype(T::def), typename T::P::def_t>::value)> check(
        int);

template <class>
false_type check(...);

template <class T>
struct has_constexpr_def : decltype(check<T>(0)) {};

template <typename INTERFACE, class... Args>
struct def_or_memset__def {
    static mesa_rc def(typename Args::ptr_type... args) {
        return INTERFACE::def(args...);
    }
};

#if 0
template <typename... Args>
void memset_everyting(Args...) __attribute__((deprecated(
        "\n\nPLEASE READ THIS> JSON::TableReadWriteAddDelete should always "
        "have a default function\n\n")));
#else
template <typename... Args>
void memset_everyting(Args...);
#endif

template <>
inline void memset_everyting<>() {}

template <typename Head, typename... Tail>
void memset_everyting(Head head, Tail... tail) {
    clear(*head);
    memset_everyting(tail...);
}

template <typename INTERFACE, class... Args>
struct def_or_memset__memset {
    static mesa_rc def(typename Args::ptr_type... args) {
        memset_everyting(args...);
        return MESA_RC_OK;
    }
};

template <typename INTERFACE, class... Args>
struct Impl
        : public meta::__if<has_constexpr_def<INTERFACE>::value,
                            def_or_memset__def<INTERFACE, Args...>,
                            def_or_memset__memset<INTERFACE, Args...>>::type {};
}  // namespace optional_default_impl_details

template <typename INTERFACE, class... Args>
struct OptionalDefaultImpl
        : public optional_default_impl_details::Impl<INTERFACE, Args...> {};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_OPTIONAL_DEFAULT_IMPL_HXX__
