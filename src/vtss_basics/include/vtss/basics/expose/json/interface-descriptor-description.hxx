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

#ifndef __VTSS_BASICS_EXPOSE_JSON_INTERFACE_DESCRIPTOR_DESCRIPTION_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_INTERFACE_DESCRIPTOR_DESCRIPTION_HXX__

#include "vtss/basics/meta.hxx"

namespace vtss {
namespace expose {
namespace json {

namespace InterfaceDescriptorDescription_ {
// Compile time detect if a class has a constexpr.
//
// Explained in details here:
// http://stackoverflow.com/questions/15232758/detecting-constexpr-with-sfinae
// http://stackoverflow.com/questions/22284496/clang-issue-detecting-constexpr-function-pointer-with-sfinae
template <bool>
struct sfinae_true : true_type {};

template <class T>
sfinae_true<(is_same<decltype(T::rpc_description), const char *>::value)>
        check_rpc(int);

template <class>
false_type check_rpc(...);

template <class T>
struct has_rpc : decltype(check_rpc<T>(0)) {};


template <class T>
sfinae_true<(is_same<decltype(T::table_description), const char *>::value)>
        check_common(int);

template <class>
false_type check_common(...);

template <class T>
struct has_common : decltype(check_common<T>(0)) {};


template <typename IF>
struct RPC {
    static constexpr const char *desc = IF::rpc_description;
};

template <typename IF>
struct COMMON {
    static constexpr const char *desc = IF::table_description;
};

template <typename IF>
struct NULL_ {
    static constexpr const char *desc = nullptr;
};


template <typename IF>
struct CommonOrNull : public meta::__if<has_common<IF>::value, COMMON<IF>,
                                        NULL_<IF>>::type {};

template <typename IF>
struct RcpOrCommonOrNull : public meta::__if<has_rpc<IF>::value, RPC<IF>,
                                             CommonOrNull<IF>>::type {};


}  // namespace InterfaceDescriptorDescription_

// use rpc_description if such exists
// otherwise use table_description if such exists
// otherwise use a nullptr
template <typename IF>
struct InterfaceDescriptorDescription
        : public InterfaceDescriptorDescription_::RcpOrCommonOrNull<IF> {};


}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_INTERFACE_DESCRIPTOR_DESCRIPTION_HXX__
