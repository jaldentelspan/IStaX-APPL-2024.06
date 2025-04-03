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

#ifndef __VTSS_BASICS_EXPOSE_SERIALIZE_KEYS_HXX__
#define __VTSS_BASICS_EXPOSE_SERIALIZE_KEYS_HXX__

#include <tuple>
#include <vtss/basics/expose/argn.hxx>
#include <vtss/basics/expose/param-list.hxx>
#include <vtss/basics/expose/param-key.hxx>
#include <vtss/basics/expose/param-val.hxx>

namespace vtss {
namespace expose {

namespace details_serialize_keys {
template <class H, class IF, class Args, class Keys>
struct Impl;

// No key  /////////////////////////////////////////////////////////////////////
template <class H, class IF, class... Args>
struct Impl<H, IF, ParamList<Args...>, meta::vector<>> {
    // static void s(H& h) {}
};

// One key  ////////////////////////////////////////////////////////////////////
template <class H, class IF, class V, size_t N, class... Args>
struct SerializeOneKey;

template <class H, class IF, class V, size_t N, class Head, class... Tail>
struct SerializeOneKey<H, IF, V, N, ParamKey<Head>, Tail...> {
    static void s(H &h, const V &v) {
        IF i;
        V _v = v;  // hack to avoid const cast
        typename arg::Int2ArgN<N>::type a;
        i.argument(h, a, _v);
    }
    static void s(H &h, V &v) {
        IF i;
        typename arg::Int2ArgN<N>::type a;
        i.argument(h, a, v);
    }
};

template <class H, class IF, class V, size_t N, class Head, class... Tail>
struct SerializeOneKey<H, IF, V, N, ParamVal<Head>, Tail...> {
    static void s(H &h, const V &v) {
        SerializeOneKey<H, IF, V, N + 1, Tail...>::s(h, v);
    }
    static void s(H &h, V &v) {
        SerializeOneKey<H, IF, V, N + 1, Tail...>::s(h, v);
    }
};

template <class H, class IF, class... Args, class Key>
struct Impl<H, IF, ParamList<Args...>, meta::vector<Key>> {
    static void s(H &h, const typename Key::basetype &k) {
        SerializeOneKey<H, IF, typename Key::basetype, 1, Args...>::s(h, k);
    }
    static void s(H &h, typename Key::basetype &k) {
        SerializeOneKey<H, IF, typename Key::basetype, 1, Args...>::s(h, k);
    }
};

// More than one key ///////////////////////////////////////////////////////////
template <class H, class IF, class V, size_t N, size_t I, class... Args>
struct SerializeTupleKeys;

template <class H, class IF, class V, size_t N, size_t I>
struct SerializeTupleKeys<H, IF, V, N, I> {
    static void s(H &h, const V &v) {}
    static void s(H &h, V &v) {}
};

template <class H, class IF, class V, size_t N, size_t I, class Head, class... Tail>
struct SerializeTupleKeys<H, IF, V, N, I, ParamKey<Head>, Tail...> {
    static void s(H &h, const V &v) {
        IF i;
        auto _v = std::get<I>(v);  // to avoid const cast
        decltype(h.as_ref()) _h = h.as_ref();
        i.argument(_h, typename arg::Int2ArgN<N>::type(), _v);
        SerializeTupleKeys<H, IF, V, N + 1, I + 1, Tail...>::s(h, v);
    }
    static void s(H &h, V &v) {
        IF i;
        decltype(h.as_ref()) _h = h.as_ref();
        i.argument(_h, typename arg::Int2ArgN<N>::type(), std::get<I>(v));
        SerializeTupleKeys<H, IF, V, N + 1, I + 1, Tail...>::s(h, v);
    }
};

template <class H, class IF, class V, size_t N, size_t I, class Head, class... Tail>
struct SerializeTupleKeys<H, IF, V, N, I, ParamVal<Head>, Tail...> {
    static void s(H &h, const V &v) {
        SerializeTupleKeys<H, IF, V, N + 1, I, Tail...>::s(h, v);
    }
    static void s(H &h, V &v) {
        SerializeTupleKeys<H, IF, V, N + 1, I, Tail...>::s(h, v);
    }
};

template <class H, class IF, class... Args, class... Keys>
struct Impl<H, IF, ParamList<Args...>, meta::vector<Keys...>> {
    typedef std::tuple<typename Keys::basetype...> V;
    static void s(H &h, const V &v) {
        decltype(h.as_tuple()) t = h.as_tuple();
        SerializeTupleKeys<decltype(t), IF, V, 1, 0, Args...>::s(t, v);
    }
    static void s(H &h, V &v) {
        decltype(h.as_tuple()) t = h.as_tuple();
        SerializeTupleKeys<decltype(t), IF, V, 1, 0, Args...>::s(t, v);
    }
};
}  // namespace details_serialize_Args

template <class IF, class H>
using SerializeKeys =
        details_serialize_keys::Impl<H, IF, typename IF::P, typename IF::P::key_list>;

template <class IF, class H, class V>
void serialize_keys(H &&h, const V &v) {
    SerializeKeys<IF, H>::s(h, v);
}

template <class IF, class H, class V>
void serialize_keys(H &&h, V &v) {
    SerializeKeys<IF, H>::s(h, v);
}

}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SERIALIZE_KEYS_HXX__
