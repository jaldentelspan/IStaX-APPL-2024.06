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

#ifndef __VTSS_BASICS_NOTIFICATIONS_STRUCT_STATUS_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_STRUCT_STATUS_HXX__

#include <vtss/basics/as_ref.hxx>

#include <vtss/basics/notifications/subject.hxx>
#include <vtss/basics/notifications/lock-global-subject.hxx>

#include <vtss/basics/expose/param-list.hxx>
#include <vtss/basics/expose/param-tuple-val.hxx>

namespace vtss {
namespace expose {
namespace struct_status_details {

template <class Args, class Values>
struct Impl;

//  Single value  //////////////////////////////////////////////////////////////
template <class Arg, class Val>
struct Impl<ParamList<Arg>, meta::vector<Val>>
        : private notifications::Subject<typename Val::basetype> {
    typedef notifications::Subject<typename Val::basetype> BASE;
    typedef typename Val::basetype V;

    using BASE::get;
    using BASE::set;
    using BASE::attach;
    using BASE::detach;

    void get_(V &v) { v = BASE::get(); }
    void get_(V &v, notifications::Event &ev) { v = BASE::get(ev); }
    void set_(V &v) { BASE::set(v); }

    mesa_rc get(typename Arg::get_ptr_type arg) const {
        as_ref<typename Arg::get_ptr_type>(arg) = BASE::get();
        return MESA_RC_OK;
    }

    mesa_rc set(typename Arg::set_ptr_type arg) {
        BASE::set(as_ref<typename Arg::set_ptr_type>(arg));
        return MESA_RC_OK;
    }
};

//  Multiple values  ///////////////////////////////////////////////////////////
template <typename... Args, typename... Val>
struct Impl<ParamList<Args...>, meta::vector<Val...>>
        : private notifications::Subject<std::tuple<typename Val::basetype...>> {
    typedef typename std::tuple<typename Val::basetype...> V;
    typedef notifications::Subject<V> BASE;
    using BASE::get;
    using BASE::set;
    using BASE::attach;
    using BASE::detach;

    void get_(V &v) { v = BASE::get(); }
    void get_(V &v, notifications::Event &ev) { v = BASE::get(ev); }
    void set_(V &v) { BASE::set(v); }

    mesa_rc get(typename Args::get_ptr_type... args) const {
        std::tuple<typename Val::basetype...> v = BASE::get();
        param_val_get_tuple_to_arg<V, Args...>(v, args...);
        return MESA_RC_OK;
    }

    mesa_rc set(typename Args::set_ptr_type... args) {
        V v;
        param_val_set_arg_to_tuple<V, Args...>(v, args...);
        BASE::set(v);
        return MESA_RC_OK;
    }
};
}  // namespace struct_status_details

template <typename... Args>
using StructStatus =
        struct_status_details::Impl<ParamList<Args...>,
                                    typename details::ValTypeList<Args...>::type>;

template <class INTERFACE>
using StructStatusInterface =
        struct_status_details::Impl<typename INTERFACE::P,
                                    typename INTERFACE::P::val_list>;

}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_STRUCT_STATUS_HXX__
