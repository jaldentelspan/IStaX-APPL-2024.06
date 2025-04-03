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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_POST_SET_ACTION_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_POST_SET_ACTION_HXX__

#include "vtss/basics/meta.hxx"
#include "vtss/basics/meta-data-packet.hxx"

namespace vtss {
namespace expose {
namespace snmp {

namespace Details {
    template <typename T>
    struct PostSetAction {
        static constexpr bool is_precondition = true;
        explicit PostSetAction(T t_) : t(t_) { }
        void invoke() { t(); }
        T t;
    };

    template<typename T>
    struct is_a_post_set_action {
        static constexpr bool val = false;
    };

    template<typename T>
    struct is_a_post_set_action<PostSetAction<T>> {
        static constexpr bool val = true;
    };
}

template <typename T>
Details::PostSetAction<T> PostSetAction(T t) {
    return Details::PostSetAction<T>(t);
}

template<typename... T>
void invoke_post_set_action(meta::VAArgs<T...> &vaargs);

template<>
inline void invoke_post_set_action(meta::VAArgs<> &vaargs) { }

template<typename HEAD, typename... TAIL>
typename meta::enable_if<
        Details::is_a_post_set_action<HEAD>::val, void
>::type invoke_post_set_action(meta::VAArgs<HEAD, TAIL...> &vaargs) {
    return vaargs.data.invoke();
}

template<typename HEAD, typename... TAIL>
typename meta::enable_if<
        !Details::is_a_post_set_action<HEAD>::val, void
>::type invoke_post_set_action(meta::VAArgs<HEAD, TAIL...> &vaargs) {
    return invoke_post_set_action<TAIL...>(vaargs.rest);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_POST_SET_ACTION_HXX__
