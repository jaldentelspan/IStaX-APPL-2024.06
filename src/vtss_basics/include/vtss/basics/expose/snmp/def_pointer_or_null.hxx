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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_DEF_POINTER_OR_NULL_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_DEF_POINTER_OR_NULL_HXX__

#include "vtss/basics/meta.hxx"

namespace vtss {
namespace expose {
namespace snmp {

namespace details {
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

template <typename INTERFACE>
struct def_pointer_or_null__def {
    static constexpr typename INTERFACE::P::def_t def = INTERFACE::def;
};

template <typename INTERFACE>
struct def_pointer_or_null__null {
    static constexpr typename INTERFACE::P::def_t def = nullptr;
};
}  // namespace details

template <typename INTERFACE>
struct def_pointer_or_null
        : public meta::__if<
                  details::has_constexpr_def<INTERFACE>::value,
                  details::def_pointer_or_null__def<INTERFACE>,
                  details::def_pointer_or_null__null<INTERFACE>>::type {};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_DEF_POINTER_OR_NULL_HXX__
