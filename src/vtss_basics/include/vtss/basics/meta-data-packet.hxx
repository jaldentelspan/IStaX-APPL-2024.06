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

#ifndef __VTSS_BASICS_META_DATA_PACKET_HXX__
#define __VTSS_BASICS_META_DATA_PACKET_HXX__

#include "vtss/basics/meta.hxx"

namespace vtss {
namespace meta {


template <class... Args>
struct VAArgs;

template <>
struct VAArgs<> {
    VAArgs() {}
    template <class T>
    T* get_optional() {
        return nullptr;
    }

    template <class T>
    const T* get_optional() const {
        return nullptr;
    }

    template <typename T, typename TT>
    TT get_or_default(TT&& t) {
        return t;
    }

    template <typename T, typename TT>
    typename add_const<TT>::type get_or_default(TT&& t) const {
        return t;
    }
};

template <class Head, class... Tail>
struct VAArgs<Head, Tail...> {
    VAArgs(Head head, Tail... tail) : data(head), rest(tail...) {}

    template <typename T,
              typename enable_if<!equal_type<T, Head>::value>::type* = nullptr>
    T& get() {
        return rest.template get<T>();
    }

    template <typename T,
              typename enable_if<equal_type<T, Head>::value>::type* = nullptr>
    Head& get() {
        return data;
    }

    template <typename T,
              typename enable_if<!equal_type<T, Head>::value>::type* = nullptr>
    const T& get() const {
        return rest.template get<T>();
    }

    template <typename T,
              typename enable_if<equal_type<T, Head>::value>::type* = nullptr>
    const Head& get() const {
        return data;
    }

    template <typename T,
              typename enable_if<!equal_type<T, Head>::value>::type* = nullptr>
    T* get_optional() {
        return rest.template get_optional<T>();
    }

    template <typename T,
              typename enable_if<equal_type<T, Head>::value>::type* = nullptr>
    Head* get_optional() {
        return &data;
    }

    template <typename T,
              typename enable_if<!equal_type<T, Head>::value>::type* = nullptr>
    const T* get_optional() const {
        return rest.template get_optional<T>();
    }

    template <typename T,
              typename enable_if<equal_type<T, Head>::value>::type* = nullptr>
    const Head* get_optional() const {
        return &data;
    }

    template <
            typename T, typename TT,
            typename enable_if<
                    !equal_type<typename BaseType<T>::type,
                                typename BaseType<Head>::type>::value>::type* =
                    nullptr>
    TT get_or_default(TT&& t) {
        return rest.template get_or_default<T>(t);
    }

    template <typename T, typename TT,
              typename enable_if<equal_type<
                      typename BaseType<T>::type,
                      typename BaseType<Head>::type>::value>::type* = nullptr>
    TT get_or_default(TT&& t) {
        return data;
    }

    template <
            typename T, typename TT,
            typename enable_if<
                    !equal_type<typename BaseType<T>::type,
                                typename BaseType<Head>::type>::value>::type* =
                    nullptr>
    typename add_const<TT>::type get_or_default(TT&& t) const {
        return rest.template get_or_default<T>(t);
    }

    template <typename T, typename TT,
              typename enable_if<equal_type<
                      typename BaseType<T>::type,
                      typename BaseType<Head>::type>::value>::type* = nullptr>
    typename add_const<TT>::type get_or_default(TT&& t) const {
        return data;
    }

    Head data;
    VAArgs<Tail...> rest;
};

}  // namespace meta
}  // namespace vtss

#endif  // __VTSS_BASICS_META_DATA_PACKET_HXX__
