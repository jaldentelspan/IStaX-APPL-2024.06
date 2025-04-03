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

#ifndef __VTSS_BASICS_EXPOSE_PARAM_LIST_HXX__
#define __VTSS_BASICS_EXPOSE_PARAM_LIST_HXX__

#include <vtss/basics/expose/param-key.hxx>
#include <vtss/basics/expose/param-val.hxx>
#include <vtss/basics/expose/json/request.hxx>
//#include <vtss/basics/expose/param-action.hxx>

namespace vtss {
namespace expose {
namespace details {

template <typename... T>
struct ItrPtrTypeListCalc;

// Base condition
template <>
struct ItrPtrTypeListCalc<> {
    typedef meta::vector<> list;
};

// Filter out ParamVal
template <typename T, typename... TAIL>
struct ItrPtrTypeListCalc<ParamVal<T>, TAIL...> {
    typedef typename ItrPtrTypeListCalc<TAIL...>::list list;
};

// Push (T::const_ptr_type, T::ptr_type) to the resulting list
template <typename T, typename... TAIL>
struct ItrPtrTypeListCalc<ParamKey<T>, TAIL...> {
    // push T::ptr_type
    typedef typename meta::push_head<
            typename ParamKey<T>::ptr_type,
            typename ItrPtrTypeListCalc<TAIL...>::list>::type list_;

    // T::const_ptr_type
    typedef typename meta::push_head<typename ParamKey<T>::const_ptr_type,
                                     list_>::type list;
};

// Calculate the function pointer type of the iterator function pointer
template <typename... T>
struct ItrPtrTypeCalc_;

template <typename... T>
struct ItrPtrTypeCalc_<meta::vector<T...>> {
    typedef mesa_rc (*type)(T...);
};

template <typename... T>
struct ItrPtrTypeCalc {
    typedef typename ItrPtrTypeListCalc<T...>::list list;
    typedef typename ItrPtrTypeCalc_<list>::type type;
};

// key type list prototype
template <typename... T>
struct KeyTypeList;

// key type list base condition, and empty list
template <>
struct KeyTypeList<> {
    typedef meta::vector<> type;
    static constexpr size_t size = 0;
};

// Push keys to the list
template <typename T, typename... X>
struct KeyTypeList<ParamKey<T>, X...> {
    static constexpr size_t size = KeyTypeList<X...>::size + 1;
    typedef typename meta::push_head<
            ParamKey<T>, typename KeyTypeList<X...>::type>::type type;
};

// Filter out values
template <typename T, typename... X>
struct KeyTypeList<ParamVal<T>, X...> {
    static constexpr size_t size = KeyTypeList<X...>::size;
    typedef typename KeyTypeList<X...>::type type;
};

// key type list prototype
template <typename... T>
struct ValTypeList;

// key type list base condition, and empty list
template <>
struct ValTypeList<> {
    static constexpr size_t size = 0;
    typedef meta::vector<> type;
};

// Push values to the list
template <typename T, typename... X>
struct ValTypeList<ParamVal<T>, X...> {
    static constexpr size_t size = ValTypeList<X...>::size + 1;
    typedef typename meta::push_head<
            ParamVal<T>, typename ValTypeList<X...>::type>::type type;
};

// Filter out keys
template <typename T, typename... X>
struct ValTypeList<ParamKey<T>, X...> {
    static constexpr size_t size = ValTypeList<X...>::size;
    typedef typename ValTypeList<X...>::type type;
};

// Calculate the function pointer type of the delete function pointer
template <typename... T>
struct DelPtrTypeCalc_;
template <typename... T>
struct DelPtrTypeCalc_<meta::vector<T...>> {
    typedef mesa_rc (*type)(typename T::const_type...);
};

// Put the pieces together and provide the delete function pointer
template <typename... T>
struct DelPtrTypeCalc {
    typedef typename KeyTypeList<T...>::type list;
    typedef typename DelPtrTypeCalc_<list>::type type;
};
}  // namespace details


template <typename... T>
struct ParamList {
    typedef mesa_rc (*get_t)(typename T::get_ptr_type...);
    typedef mesa_rc (*set_t)(typename T::set_ptr_type...);
    typedef mesa_rc (*add_t)(typename T::set_ptr_type...);
    typedef mesa_rc (*def_t)(typename T::ptr_type...);
    typedef typename details::DelPtrTypeCalc<T...>::type del_t;
    typedef typename details::ItrPtrTypeCalc<T...>::type itr_t;
    typedef mesa_rc (*json_get_all_t)(const vtss::expose::json::Request *req,
                                      vtss::ostreamBuf *os);

    typedef typename details::KeyTypeList<T...>::type key_list;
    typedef typename details::ValTypeList<T...>::type val_list;
    static constexpr size_t key_cnt = details::KeyTypeList<T...>::size;
    static constexpr size_t val_cnt = details::ValTypeList<T...>::size;
};

}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_PARAM_LIST_HXX__
