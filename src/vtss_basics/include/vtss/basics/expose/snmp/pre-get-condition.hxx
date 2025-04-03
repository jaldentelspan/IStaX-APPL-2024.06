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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_PRE_GET_CONDITION_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_PRE_GET_CONDITION_HXX__

#include "vtss/basics/meta.hxx"
#include "vtss/basics/meta-data-packet.hxx"

namespace vtss {
namespace expose {
namespace snmp {
namespace Details {
    template <typename T>
    struct PreGetCondition {
        static constexpr bool is_precondition = true;
        explicit PreGetCondition(T t_) : t(t_) { }
        bool test() { return t(); }
        T t;
    };

    template<typename T>
    struct is_a_pre_get_condition {
        static constexpr bool val = false;
    };

    template<typename T>
    struct is_a_pre_get_condition<PreGetCondition<T>> {
        static constexpr bool val = true;
    };
}

template <typename T>
Details::PreGetCondition<T> PreGetCondition(T t) {
    return Details::PreGetCondition<T>(t);
}

template<typename... T>
bool check_pre_get_condition(meta::VAArgs<T...> &vaargs);

template<>
inline bool check_pre_get_condition(meta::VAArgs<> &vaargs) {
    return true;
}

template<typename HEAD, typename... TAIL>
typename meta::enable_if<
        Details::is_a_pre_get_condition<HEAD>::val, bool
>::type check_pre_get_condition(meta::VAArgs<HEAD, TAIL...> &vaargs) {
    return vaargs.data.test();
}

template<typename HEAD, typename... TAIL>
typename meta::enable_if<
        !Details::is_a_pre_get_condition<HEAD>::val, bool
>::type check_pre_get_condition(meta::VAArgs<HEAD, TAIL...> &vaargs) {
    return check_pre_get_condition<TAIL...>(vaargs.rest);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_PRE_GET_CONDITION_HXX__
