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

#ifndef __VTSS_BASICS_EXPOSE_TABLE_STATUS_HXX__
#define __VTSS_BASICS_EXPOSE_TABLE_STATUS_HXX__

#include <tuple>
#include <vtss/basics/set.hxx>
#include <vtss/basics/as_ref.hxx>
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/subject-set.hxx>
#include <vtss/basics/notifications/subject-table.hxx>

#include <vtss/basics/expose/param-list.hxx>
#include <vtss/basics/expose/struct-status.hxx>
#include <vtss/basics/expose/param-tuple-key.hxx>
#include <vtss/basics/expose/param-tuple-val.hxx>

namespace vtss {
namespace expose {
namespace table_status_details {

template <class Head, class... Tail>
struct FindFirstVal {
    typedef typename FindFirstVal<Tail...>::Set Set;
    typedef typename FindFirstVal<Tail...>::Get Get;
    typedef typename FindFirstVal<Tail...>::GetNext GetNext;

    static Get as_get(typename Head::get_ptr_type,
                      typename Tail::get_ptr_type... tail) {
        return FindFirstVal<Tail...>::as_get(tail...);
    }

    static Get as_get_next(typename Head::get_next_type,
                           typename Tail::get_next_type... tail) {
        return FindFirstVal<Tail...>::as_get_next(tail...);
    }

    static Set as_set(typename Head::set_ptr_type,
                      typename Tail::set_ptr_type... tail) {
        return FindFirstVal<Tail...>::as_set(tail...);
    }
};

template <class Head, class... Tail>
struct FindFirstVal<ParamVal<Head>, Tail...> {
    typedef typename AsRef<typename ParamVal<Head>::set_ptr_type>::R Set;
    typedef typename AsRef<typename ParamVal<Head>::get_ptr_type>::R Get;
    typedef typename AsRef<typename ParamVal<Head>::get_next_type>::R GetNext;

    static Get as_get(typename ParamVal<Head>::get_ptr_type head,
                      typename Tail::get_ptr_type...) {
        return as_ref<typename ParamVal<Head>::get_ptr_type>(head);
    }

    static GetNext as_get_next(typename ParamVal<Head>::get_next_type head,
                               typename Tail::get_next_type...) {
        return as_ref<typename ParamVal<Head>::get_next_type>(head);
    }

    static Set as_set(typename ParamVal<Head>::set_ptr_type head,
                      typename Tail::set_ptr_type...) {
        return as_ref<typename ParamVal<Head>::set_ptr_type>(head);
    }
};

template <class Head, class... Tail>
struct FindFirstKey {
    typedef typename FindFirstKey<Tail...>::Set Set;
    typedef typename FindFirstKey<Tail...>::Get Get;
    typedef typename FindFirstKey<Tail...>::GetNext GetNext;

    static Get as_get(typename Head::get_ptr_type,
                      typename Tail::get_ptr_type... tail) {
        return FindFirstKey<Tail...>::as_get(tail...);
    }

    static GetNext as_get_next(typename Head::get_next_type,
                               typename Tail::get_next_type... tail) {
        return FindFirstKey<Tail...>::as_get_next(tail...);
    }

    static Set as_set(typename Head::set_ptr_type,
                      typename Tail::set_ptr_type... tail) {
        return FindFirstKey<Tail...>::as_set(tail...);
    }
};

template <class Head, class... Tail>
struct FindFirstKey<ParamKey<Head>, Tail...> {
    typedef typename AsRef<typename ParamKey<Head>::set_ptr_type>::R Set;
    typedef typename AsRef<typename ParamKey<Head>::get_ptr_type>::R Get;
    typedef typename AsRef<typename ParamKey<Head>::get_next_type>::R GetNext;

    static Get as_get(typename ParamKey<Head>::get_ptr_type head,
                      typename Tail::get_ptr_type...) {
        return as_ref<typename ParamKey<Head>::get_ptr_type>(head);
    }

    static GetNext as_get_next(typename ParamKey<Head>::get_next_type head,
                               typename Tail::get_next_type...) {
        return as_ref<typename ParamKey<Head>::get_next_type>(head);
    }

    static Set as_set(typename ParamKey<Head>::set_ptr_type head,
                      typename Tail::set_ptr_type...) {
        return as_ref<typename ParamKey<Head>::set_ptr_type>(head);
    }
};


template <class Args, class Keys, class Values>
struct Impl;

// Single Key and single value  ////////////////////////////////////////////////
template <class... Args, class Key, class Val>
struct Impl<ParamList<Args...>, meta::vector<Key>, meta::vector<Val>>
        : private notifications::SubjectTable<typename Key::basetype,
                                              typename Val::basetype> {
    typedef typename Key::basetype K;
    typedef typename Val::basetype V;
    typedef typename Key::set_ptr_type K_set;
    typedef notifications::SubjectTable<K, V> BASE;
    typedef typename notifications::SubjectTable<K, V>::Lock Lock;

    using typename BASE::Observer;
    using typename BASE::Callback;
    using BASE::observer_new;
    using BASE::observer_get;
    using BASE::observer_del;
    using BASE::observer_mask_key;
    using BASE::clear;
    using BASE::size;
    using BASE::lock_get;

    Impl() = delete;
    Impl(const char *mutex_name, uint32_t module_id) :
        BASE(mutex_name, module_id) {}

    mesa_rc get_(const K &k, V &v) const { return BASE::get(k, v); }
    mesa_rc set_(const K &k, V &v) const { return BASE::set(k, v); }
    mesa_rc del_(const K &k, V &v) const { return BASE::del(k, v); }
    mesa_rc get_first_(K &k, V &v) const { return BASE::get_first(k, v); }
    mesa_rc get_next_(K &k, V &v) const { return BASE::get_next(k, v); }

    mesa_rc set(Lock &l, const Map<K, V> &v) { return BASE::set(l, v); }
    mesa_rc set(const Map<K, V> &v) { return BASE::set(v); }
    mesa_rc set(const Map<K, V> &v, Callback &cb) { return BASE::set(v, cb); }
    mesa_rc set(Lock &l, const Map<K, V> &v, Callback &cb) {
        return BASE::set(l, v, cb);
    }

    const Map<K, V> &ref(Lock &l) const { return BASE::ref(l); }

    mesa_rc get_first(typename Args::get_next_type... args) {
        return BASE::get_first(FindFirstKey<Args...>::as_get_next(args...),
                               FindFirstVal<Args...>::as_get_next(args...));
    }

    mesa_rc get_next(typename Args::get_next_type... args) {
        return BASE::get_next(FindFirstKey<Args...>::as_get_next(args...),
                              FindFirstVal<Args...>::as_get_next(args...));
    }

    mesa_rc get(typename Args::get_ptr_type... args) {
        pre_get(FindFirstKey<Args...>::as_get(args...));
        return BASE::get(FindFirstKey<Args...>::as_get(args...),
                         FindFirstVal<Args...>::as_get(args...));
    }

    mesa_rc set(typename Args::set_ptr_type... args) {
        return BASE::set(FindFirstKey<Args...>::as_set(args...),
                         FindFirstVal<Args...>::as_set(args...));
    }

    mesa_rc set(Lock &l, typename Args::set_ptr_type... args) {
        return BASE::set(l,
                         FindFirstKey<Args...>::as_set(args...),
                         FindFirstVal<Args...>::as_set(args...));
    }

    mesa_rc del(K_set k) { return BASE::del(as_ref<K_set>(k)); }

  private:
    // when implementing this in the base class _ALWAYS_ use the "override"
    // keyword to ensure that the signature is matching
    virtual void pre_get(const K &k) {}
};

// Multiple Keys and single value  /////////////////////////////////////////////
template <class... Args, class... Key, class Val>
struct Impl<ParamList<Args...>, meta::vector<Key...>, meta::vector<Val>>
        : private notifications::SubjectTable<std::tuple<typename Key::basetype...>,
                                              typename Val::basetype> {
    typedef std::tuple<typename Key::basetype...> K;
    typedef typename Val::basetype V;
    typedef notifications::SubjectTable<K, V> BASE;
    typedef typename notifications::SubjectTable<K, V>::Lock Lock;

    using typename BASE::Observer;
    using BASE::observer_new;
    using BASE::observer_get;
    using BASE::observer_del;
    using BASE::observer_mask_key;
    using BASE::clear;
    using BASE::size;
    using BASE::lock_get;

    Impl() = delete;
    Impl(const char *mutex_name, uint32_t module_id) :
        BASE(mutex_name, module_id) {}

    mesa_rc get_(const K &k, V &v) const { return BASE::get(k, v); }
    mesa_rc set_(const K &k, V &v) const { return BASE::set(k, v); }
    mesa_rc set(const Map<K, V> &v) { return BASE::set(v); }
    mesa_rc del_(const K &k, V &v) const { return BASE::del(k, v); }
    mesa_rc get_first_(K &k, V &v) const { return BASE::get_first(k, v); }
    mesa_rc get_next_(K &k, V &v) const { return BASE::get_next(k, v); }
    const Map<K, V> &ref(Lock &l) const { return BASE::ref(l); }

    mesa_rc get_first(typename Args::get_next_type... args) {
        K k;
        mesa_rc rc;
        rc = BASE::get_first(k, FindFirstVal<Args...>::as_get_next(args...));
        if (rc != MESA_RC_OK) return rc;
        param_key_get_next_tuple_to_arg<K, Args...>(k, args...);
        return rc;
    }

    mesa_rc get_next(typename Args::get_next_type... args) {
        K k;
        mesa_rc rc;
        param_key_get_next_arg_to_tuple<K, Args...>(k, args...);
        rc = BASE::get_next(k, FindFirstVal<Args...>::as_get_next(args...));
        if (rc != MESA_RC_OK) return rc;
        param_key_get_next_tuple_to_arg<K, Args...>(k, args...);
        return rc;
    }

    mesa_rc get(typename Args::get_ptr_type... args) {
        K k;
        param_key_get_arg_to_tuple<K, Args...>(k, args...);
        pre_get(k);  // Must be done outside the critical section
        return BASE::get(k, FindFirstVal<Args...>::as_get(args...));
    }

    mesa_rc set(typename Args::set_ptr_type... args) {
        K k;
        param_key_set_arg_to_tuple<K, Args...>(k, args...);
        return BASE::set(k, FindFirstVal<Args...>::as_set(args...));
    }

    mesa_rc set(Lock &l, typename Args::set_ptr_type... args) {
        K k;
        param_key_set_arg_to_tuple<K, Args...>(k, args...);
        return BASE::set(l, k, FindFirstVal<Args...>::as_set(args...));
    }

    mesa_rc del(typename Key::set_ptr_type... args) {
        K k;
        param_key_set_arg_to_tuple<K, Key...>(k, args...);
        return BASE::del(k);
    }

  private:
    // when implementing this in the base class _ALWAYS_ use the "override"
    // keyword to ensure that the signature is matching
    virtual void pre_get(const K &k) {}
};

// Single Key and multiple values  /////////////////////////////////////////////
template <class... Args, class Key, class... Val>
struct Impl<ParamList<Args...>, meta::vector<Key>, meta::vector<Val...>>
        : private notifications::SubjectTable<
                  typename Key::basetype, std::tuple<typename Val::basetype...>> {
    typedef typename Key::basetype K;
    typedef std::tuple<typename Val::basetype...> V;
    typedef typename Key::get_ptr_type K_get;
    typedef typename Key::set_ptr_type K_set;
    typedef typename notifications::SubjectTable<K, V>::Lock Lock;

    typedef notifications::SubjectTable<typename Key::basetype,
                                        std::tuple<typename Val::basetype...>> BASE;

    using typename BASE::Observer;
    using BASE::observer_new;
    using BASE::observer_get;
    using BASE::observer_del;
    using BASE::observer_mask_key;
    using BASE::clear;
    using BASE::size;
    using BASE::lock_get;

    Impl() = delete;
    Impl(const char *mutex_name, uint32_t module_id) :
        BASE(mutex_name, module_id) {}

    mesa_rc get_(const K &k, V &v) const { return BASE::get(k, v); }
    mesa_rc set_(const K &k, V &v) const { return BASE::set(k, v); }
    mesa_rc set(const Map<K, V> &v) { return BASE::set(v); }
    mesa_rc del_(const K &k, V &v) const { return BASE::del(k, v); }
    mesa_rc get_first_(K &k, V &v) const { return BASE::get_first(k, v); }
    mesa_rc get_next_(K &k, V &v) const { return BASE::get_next(k, v); }
    const Map<K, V> &ref(Lock &l) const { return BASE::ref(l); }

    mesa_rc get_first(typename Args::get_next_type... args) {
        V v;
        mesa_rc rc;
        rc = BASE::get_first(FindFirstKey<Args...>::as_get_next(args...), v);
        if (rc != MESA_RC_OK) return rc;
        param_val_get_next_tuple_to_arg<V, Args...>(v, args...);
        return MESA_RC_OK;
    }

    mesa_rc get_next(typename Args::get_next_type... args) {
        V v;
        mesa_rc rc;
        rc = BASE::get_next(FindFirstKey<Args...>::as_get_next(args...), v);
        if (rc != MESA_RC_OK) return rc;
        param_val_get_next_tuple_to_arg<V, Args...>(v, args...);
        return MESA_RC_OK;
    }

    mesa_rc get(typename Args::get_ptr_type... args) {
        V v;
        pre_get(FindFirstKey<Args...>::as_get(args...));
        mesa_rc rc = BASE::get(FindFirstKey<Args...>::as_get(args...), v);
        if (rc != MESA_RC_OK) return rc;
        param_val_get_tuple_to_arg<V, Args...>(v, args...);
        return MESA_RC_OK;
    }

    mesa_rc set(typename Args::set_ptr_type... args) {
        V v;
        param_val_set_arg_to_tuple<V, Args...>(v, args...);
        return BASE::set(FindFirstKey<Args...>::as_set(args...), v);
    }

    mesa_rc set(Lock &l, typename Args::set_ptr_type... args) {
        V v;
        param_val_set_arg_to_tuple<V, Args...>(v, args...);
        return BASE::set(l, FindFirstKey<Args...>::as_set(args...), v);
    }

    mesa_rc del(K_set k) { return BASE::del(as_ref<K_set>(k)); }

  private:
    // when implementing this in the base class _ALWAYS_ use the "override"
    // keyword to ensure that the signature is matching
    virtual void pre_get(K_set k) {}
};

// Multiple keys and multiple values ///////////////////////////////////////////
template <class... Args, class... Key, class... Val>
struct Impl<ParamList<Args...>, meta::vector<Key...>, meta::vector<Val...>>
        : private notifications::SubjectTable<std::tuple<typename Key::basetype...>,
                                              std::tuple<typename Val::basetype...>> {
    typedef std::tuple<typename Key::basetype...> K;
    typedef std::tuple<typename Val::basetype...> V;
    typedef notifications::SubjectTable<K, V> BASE;
    typedef typename notifications::SubjectTable<K, V>::Lock Lock;

    using typename BASE::Observer;
    using BASE::observer_new;
    using BASE::observer_get;
    using BASE::observer_del;
    using BASE::observer_mask_key;
    using BASE::clear;
    using BASE::size;
    using BASE::lock_get;

    Impl() = delete;
    Impl(const char *mutex_name, uint32_t module_id) :
        BASE(mutex_name, module_id) {}

    mesa_rc get_(const K &k, V &v) const { return BASE::get(k, v); }
    mesa_rc set_(const K &k, V &v) const { return BASE::set(k, v); }
    mesa_rc set(const Map<K, V> &v) { return BASE::set(v); }
    mesa_rc del_(const K &k, V &v) const { return BASE::del(k, v); }
    mesa_rc get_first_(K &k, V &v) const { return BASE::get_first(k, v); }
    mesa_rc get_next_(K &k, V &v) const { return BASE::get_next(k, v); }
    const Map<K, V> &ref(Lock &l) const { return BASE::ref(l); }

    mesa_rc get_first(typename Args::get_next_type... args) {
        K k;
        V v;
        mesa_rc rc = BASE::get_first(k, v);
        if (rc != MESA_RC_OK) return rc;
        param_key_get_next_tuple_to_arg<K, Args...>(k, args...);
        param_val_get_next_tuple_to_arg<V, Args...>(v, args...);
        return rc;
    }

    mesa_rc get_next(typename Args::get_next_type... args) {
        K k;
        V v;
        param_key_get_next_arg_to_tuple<K, Args...>(k, args...);
        mesa_rc rc = BASE::get_next(k, v);
        if (rc != MESA_RC_OK) return rc;
        param_key_get_next_tuple_to_arg<K, Args...>(k, args...);
        param_val_get_next_tuple_to_arg<V, Args...>(v, args...);
        return rc;
    }

    mesa_rc get(typename Args::get_ptr_type... args) {
        K k;
        V v;
        param_key_get_arg_to_tuple<K, Args...>(k, args...);
        pre_get(k);  // Must be done outside the critical section
        mesa_rc rc = BASE::get(k, v);
        if (rc != MESA_RC_OK) return rc;
        param_val_get_tuple_to_arg<V, Args...>(v, args...);
        return MESA_RC_OK;
    }

    mesa_rc set(typename Args::set_ptr_type... args) {
        K k;
        V v;
        param_key_set_arg_to_tuple<K, Args...>(k, args...);
        param_val_set_arg_to_tuple<V, Args...>(v, args...);
        return BASE::set(k, v);
    }

    mesa_rc set(Lock &l, typename Args::set_ptr_type... args) {
        K k;
        V v;
        param_key_set_arg_to_tuple<K, Args...>(k, args...);
        param_val_set_arg_to_tuple<V, Args...>(v, args...);
        return BASE::set(l, k, v);
    }

    mesa_rc del(typename Key::set_ptr_type... args) {
        K k;
        param_key_set_arg_to_tuple<K, Key...>(k, args...);
        return BASE::del(k);
    }

  private:
    // when implementing this in the base class _ALWAYS_ use the "override"
    // keyword to ensure that the signature is matching
    virtual void pre_get(const K &k) {}
};

// Specialization for instances with only one key and no value. ////////////////
template <class... Args, class Key>
struct Impl<ParamList<Args...>, meta::vector<Key>, meta::vector<>>
        : private notifications::SubjectSet<typename Key::basetype> {
    typedef typename Key::basetype K;
    typedef typename Key::set_ptr_type K_set;
    typedef notifications::SubjectSet<K> BASE;

    using typename BASE::Observer;
    using BASE::observer_new;
    using BASE::observer_get;
    using BASE::observer_del;
    using BASE::observer_mask_key;
    using BASE::clear;
    using BASE::size;

    mesa_rc get_(const K &k) const { return BASE::get(k); }
    mesa_rc set_(const K &k) const { return BASE::set(k); }
    mesa_rc del_(const K &k) const { return BASE::del(k); }
    mesa_rc get_first_(K &k) const { return BASE::get_first(k); }
    mesa_rc get_next_(K &k) const { return BASE::get_next(k); }

    mesa_rc get_first(typename Key::get_next_type k) {
        return BASE::get_first(as_ref<typename Key::get_next_type>(k));
    }

    mesa_rc get_next(typename Key::get_next_type k) {
        return BASE::get_next(as_ref<typename Key::get_next_type>(k));
    }

    mesa_rc get(K_set k) {
        pre_get(k);
        return BASE::get(as_ref<K_set>(k));
    }

    mesa_rc set(const K_set &k) { return BASE::set(as_ref<K_set>(k)); }

    mesa_rc del(K_set k) { return BASE::del(as_ref<K_set>(k)); }

  private:
    // when implementing this in the base class _ALWAYS_ use the "override"
    // keyword to ensure that the signature is matching
    virtual void pre_get(const K &k) {}
};

// Specialization for instances with multiple keys and no value. ///////////////
template <class... Args, class... Key>
struct Impl<ParamList<Args...>, meta::vector<Key...>, meta::vector<>>
        : private notifications::SubjectSet<std::tuple<typename Key::basetype...>> {
    typedef std::tuple<typename Key::basetype...> K;
    typedef notifications::SubjectSet<K> BASE;

    using typename BASE::Observer;
    using BASE::observer_new;
    using BASE::observer_get;
    using BASE::observer_del;
    using BASE::observer_mask_key;
    using BASE::clear;
    using BASE::size;

    mesa_rc get_(const K &k) const { return BASE::get(k); }
    mesa_rc set_(const K &k) const { return BASE::set(k); }
    mesa_rc del_(const K &k) const { return BASE::del(k); }
    mesa_rc get_first_(K &k) const { return BASE::get_first(k); }
    mesa_rc get_next_(K &k) const { return BASE::get_next(k); }

    mesa_rc get_first(typename Args::get_next_type... args) {
        K k;
        mesa_rc rc = BASE::get_first(k);
        if (rc != MESA_RC_OK) return rc;
        param_key_get_next_tuple_to_arg<K, Args...>(k, args...);
        return rc;
    }

    mesa_rc get_next(typename Args::get_next_type... args) {
        K k;
        param_key_get_next_arg_to_tuple<K, Args...>(k, args...);
        mesa_rc rc = BASE::get_next(k);
        if (rc != MESA_RC_OK) return rc;
        param_key_get_next_tuple_to_arg<K, Args...>(k, args...);
        return rc;
    }

    mesa_rc get(typename Args::get_ptr_type... args) {
        K k;
        param_key_get_arg_to_tuple<K, Args...>(k, args...);
        pre_get(k);  // Must be done outside the critical section
        return BASE::get(k);
    }

    mesa_rc set(typename Args::set_ptr_type... args) {
        K k;
        param_key_set_arg_to_tuple<K, Args...>(k, args...);
        return BASE::set(k);
    }

    mesa_rc del(typename Key::set_ptr_type... args) {
        K k;
        param_key_set_arg_to_tuple<K, Key...>(k, args...);
        return BASE::del(k);
    }

  private:
    // when implementing this in the base class _ALWAYS_ use the "override"
    // keyword to ensure that the signature is matching
    virtual void pre_get(const K &k) {}
};
}  // namespace table_status_details

template <typename... Args>
using TableStatus =
        table_status_details::Impl<ParamList<Args...>,
                                   typename details::KeyTypeList<Args...>::type,
                                   typename details::ValTypeList<Args...>::type>;

template <class INTERFACE>
using TableStatusInterface =
        table_status_details::Impl<typename INTERFACE::P,
                                   typename INTERFACE::P::key_list,
                                   typename INTERFACE::P::val_list>;

}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_TABLE_STATUS_HXX__
