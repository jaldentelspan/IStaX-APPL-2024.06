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

#ifndef __VTSS_BASICS_EXPOSE_JSON_PARAM_TUPLE_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_PARAM_TUPLE_HXX__
#include <memory>
#include <vtss/basics/meta.hxx>
#include <vtss/basics/api_types.h>

namespace vtss {
namespace expose {
namespace json {

/*
 * ParamTuple synopsis
 *
 * The class can be used as a tuple with special functions for serializing each
 * element in the tuple and call a given function either in a set or get
 * context.
 *
 * The list of arguments which it is ready to accept must be encapsulated with
 * the ParamVal or ParamKey type.
 *
 * When used the calling a function, the argument is parsed as input or output
 * depending on if it is a key/value and it we are in get/set context. The
 * following table shows the details:
 *
 *       | Get  Set
 * ------+---------
 * Key   | IN   IN
 * Value | OUT  IN
 *
 * Inputs may either be passed by value, pointer or reference, while outpus may
 * be passed by pointer or reference. The details of this can be found in
 * param-val.hxx and param-key.hxx
 *
 * template <class... Args>
 * struct ParamTuple {
 *     // Serialize all values for output
 *     template <typename INTERFACE, typename HANDLER>
 *     void serialize_get(HANDLER &h);
 *
 *     // Do nothing, added for the sake of completness. A set operation should
 *     // not return anything.
 *     template <typename INTERFACE, typename HANDLER>
 *     void serialize_set(HANDLER &h);
 *
 *     // Parses all keys
 *     template <typename INTERFACE, typename HANDLER>
 *     void parse_get(HANDLER &h);
 *
 *     // Parses all keys and values
 *     template <typename INTERFACE, typename HANDLER>
 *     void parse_set(HANDLER &h);
 *
 *     // Call the function f with the param typle data elements as argument in
 *     // get context
 *     template <typename F>
 *     mesa_rc call_as_get(F f);
 *
 *     // Call the function f with the param typle data elements as argument in
 *     // set context
 *     template <typename F>
 *     mesa_rc call_as_set(F f);
 *
 *     // calculate how many input arguments is expected when used in
 *     // get-context
 *     constexpr unsigned get_context_input_count() const;
 *
 *     // calculate how many input arguments is expected when used in
 *     // set-context
 *     constexpr unsigned set_context_input_count() const;
 *
 *     // calculate how many output arguments is generated when used in
 *     // get-context
 *     constexpr unsigned get_context_output_count() const;
 *
 *     // calculate how many output arguments is generated when used in
 *     // set-context
 *     constexpr unsigned set_context_output_count() const;
 * }
 *
 * */

namespace details {
template <class... Args>
struct ParamTupleImpl;

template <>
struct ParamTupleImpl<> {
#define BASE_CASE(X)                                \
    template <typename INTERFACE, typename HANDLER> \
    unsigned X(HANDLER &h) {                        \
        return 0;                                   \
    }

    BASE_CASE(serialize_get)
    BASE_CASE(serialize_set)
    BASE_CASE(serialize_keys)
    BASE_CASE(serialize_values)
    BASE_CASE(parse_get)
    BASE_CASE(parse_set)
    BASE_CASE(parse_keys)
    BASE_CASE(parse_values)
#undef BASE_CASE

    static constexpr unsigned get_context_input_count() { return 0; }
    static constexpr unsigned set_context_input_count() { return 0; }
    static constexpr unsigned get_context_output_count() { return 0; }
    static constexpr unsigned set_context_output_count() { return 0; }

#define CALL_AS(X)             \
    template <typename F>      \
    mesa_rc call_as_##X(F f) { \
        return f->X();         \
    }
    CALL_AS(get)
    CALL_AS(set)
    CALL_AS(add)
    CALL_AS(del)
    CALL_AS(def)
#undef CALL_AS

  protected:
#define BASE_CASE(X)                                            \
    template <unsigned N, typename INTERFACE, typename HANDLER> \
    unsigned X(INTERFACE &i, HANDLER &h) {                      \
        return 0;                                               \
    }

    BASE_CASE(serialize_get_)
    BASE_CASE(serialize_set_)
    BASE_CASE(serialize_keys_)
    BASE_CASE(serialize_values_)
    BASE_CASE(parse_get_)
    BASE_CASE(parse_set_)
    BASE_CASE(parse_keys_)
    BASE_CASE(parse_values_)
#undef BASE_CASE

    template <typename F, typename... Args>
    mesa_rc call_as_get_(F f, Args &&... args) {
        return f->get(args...);
    }

    template <typename F, typename... Args>
    mesa_rc call_as_set_(F f, Args &&... args) {
        return f->set(args...);
    }

    template <typename F, typename... Args>
    mesa_rc call_as_add_(F f, Args &&... args) {
        return f->add(args...);
    }

    template <typename F, typename... Args>
    mesa_rc call_as_del_(F f, Args &&... args) {
        return f->del(args...);
    }

    template <typename F, typename... Args>
    mesa_rc call_as_def_(F f, Args &&... args) {
        return f->def(args...);
    }

    template <typename F, typename... Args>
    mesa_rc call_as_itr_first_(F f, Args &&... args) {
        return f->itr(args...);
    }

    template <typename F, typename Prev, typename... Args>
    mesa_rc call_as_itr_(F f, Prev &prev, Args &&... args) {
        return f->itr(args...);
    }
};

template <class... T>
struct ParamTupleAlloc;

template <class Head, class... Tail>
struct ParamTupleImpl<Head, Tail...> : private ParamTupleImpl<Tail...> {
    friend struct ParamTupleAlloc<Head, Tail...>;

#define WRAP(X, Y)                                  \
    template <typename INTERFACE, typename HANDLER> \
    unsigned X(HANDLER &h) {                        \
        INTERFACE interface;                        \
        return Y<1>(interface, h);                  \
    }

    WRAP(serialize_get, serialize_get_)
    WRAP(serialize_set, serialize_set_)
    WRAP(serialize_values, serialize_values_)
    WRAP(serialize_keys, serialize_keys_)
    WRAP(parse_get, parse_get_)
    WRAP(parse_set, parse_set_)
    WRAP(parse_values, parse_values_)
    WRAP(parse_keys, parse_keys_)
#undef WRAP

#define CALL_AS(X)                   \
    template <typename F>            \
    mesa_rc call_as_##X(F f) {       \
        return call_as_##X##_<F>(f); \
    }

    CALL_AS(get)
    CALL_AS(set)
    CALL_AS(add)
    CALL_AS(del)
    CALL_AS(def)
    CALL_AS(itr_first)
#undef CALL_AS

    template <typename F>
    mesa_rc call_as_itr(F f) {
        // Clone self, as we need a set of values representing the previous call
        ParamTupleImpl<Head, Tail...> prev(*this);
        return call_as_itr_<F>(f, prev);
    }

#define FOR_ALL_TUPLE_ACCUMULATE(M)                      \
    static constexpr unsigned M() {                      \
        return Head::M() + ParamTupleImpl<Tail...>::M(); \
    }
    FOR_ALL_TUPLE_ACCUMULATE(get_context_input_count)
    FOR_ALL_TUPLE_ACCUMULATE(set_context_input_count)
    FOR_ALL_TUPLE_ACCUMULATE(get_context_output_count)
    FOR_ALL_TUPLE_ACCUMULATE(set_context_output_count)
#undef FOR_ALL_TUPLE_ACCUMULATE

  protected:
#define FOR_ALL_TUPLE_ELEMENT(TUPLE_METHOD, PARAM_METHOD)                   \
    template <unsigned N, typename INTERFACE, typename HANDLER>             \
    unsigned TUPLE_METHOD(INTERFACE &i, HANDLER &h) {                       \
        head.template PARAM_METHOD<N>(i, h);                                \
        if (!h.ok()) return N;                                              \
        return ParamTupleImpl<Tail...>::template TUPLE_METHOD<N + 1>(i, h); \
    }
    FOR_ALL_TUPLE_ELEMENT(serialize_get_, serialize_get)
    FOR_ALL_TUPLE_ELEMENT(serialize_set_, serialize_set)
    FOR_ALL_TUPLE_ELEMENT(serialize_values_, serialize_val)
    FOR_ALL_TUPLE_ELEMENT(serialize_keys_, serialize_key)
    FOR_ALL_TUPLE_ELEMENT(parse_get_, parse_get)
    FOR_ALL_TUPLE_ELEMENT(parse_set_, parse_set)
    FOR_ALL_TUPLE_ELEMENT(parse_values_, parse_val)
    FOR_ALL_TUPLE_ELEMENT(parse_keys_, parse_key)
#undef FOR_ALL_TUPLE_ELEMENT

    template <typename F, typename... Args>
    mesa_rc call_as_get_(F f, Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_get_(
                f, args..., head.param_in_get_context());
    }

    template <typename F, typename... Args>
    mesa_rc call_as_set_(F f, Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_set_(
                f, args..., head.param_in_set_context());
    }

    template <typename F, typename... Args>
    mesa_rc call_as_add_(F f, Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_add_(
                f, args..., head.param_in_set_context());
    }

    template <typename F, typename... Args>
    mesa_rc call_as_def_(F f, Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_def_<F>(
                f, args..., head.param_as_ptr());
    }

    template <typename F, typename... Args>
    mesa_rc call_as_itr_first_(
            typename meta::enable_if<Head::is_key(), F>::type f,
            Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_itr_first_<F>(
                f, args..., nullptr, head.param_as_ptr());
    }

    template <typename F, typename... Args>
    mesa_rc call_as_itr_first_(
            typename meta::enable_if<!Head::is_key(), F>::type f,
            Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_itr_first_<F>(f,
                                                                       args...);
    }

    template <typename F, typename Prev, typename... Args>
    mesa_rc call_as_itr_(typename meta::enable_if<Head::is_key(), F>::type f,
                         Prev &prev, Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_itr_<F>(
                f, static_cast<ParamTupleImpl<Tail...> &>(prev), args...,
                prev.head.param_as_const_ptr(), head.param_as_ptr());
    }

    template <typename F, typename Prev, typename... Args>
    mesa_rc call_as_itr_(typename meta::enable_if<!Head::is_key(), F>::type f,
                         Prev &prev, Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_itr_<F>(
                f, static_cast<ParamTupleImpl<Tail...> &>(prev), args...);
    }

    template <typename F, typename... Args>
    mesa_rc call_as_del_(typename meta::enable_if<Head::is_key(), F>::type f,
                         Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_del_<F>(
                f, args..., head.param_in_get_context());
    }

    template <typename F, typename... Args>
    mesa_rc call_as_del_(typename meta::enable_if<!Head::is_key(), F>::type f,
                         Args &&... args) {
        return ParamTupleImpl<Tail...>::template call_as_del_<F>(f, args...);
    }

  private:
    Head head;
};

template <class... T>
struct ParamTupleAlloc {
    ParamTupleAlloc() : impl(new ParamTupleImpl<T...>()) {}

    ParamTupleAlloc(const ParamTupleAlloc &rhs)
        : impl(new ParamTupleImpl<T...>()) {
        *impl = *(rhs.impl);
    }

#define WRAP(X)                                     \
    template <typename INTERFACE, typename HANDLER> \
    unsigned X(HANDLER &h) {                        \
        return impl->template X<INTERFACE>(h);      \
    }

    WRAP(serialize_get)
    WRAP(serialize_set)
    WRAP(serialize_values)
    WRAP(serialize_keys)
    WRAP(parse_get)
    WRAP(parse_set)
    WRAP(parse_values)
    WRAP(parse_keys)
#undef WRAP

#define CALL_AS(X)                   \
    template <typename F>            \
    mesa_rc call_as_##X(F f) {       \
        return impl->call_as_##X(f); \
    }

    CALL_AS(get)
    CALL_AS(set)
    CALL_AS(add)
    CALL_AS(del)
    CALL_AS(def)
    CALL_AS(itr_first)
#undef CALL_AS

    template <typename F>
    mesa_rc call_as_itr(F f) {
        // Clone self, as we need a set of values representing the previous call
        ParamTupleAlloc<T...> prev(*this);
        ParamTupleImpl<T...> &ref = *(prev.impl.get());
        return impl->template call_as_itr_<F>(f, ref);
    }

#define FOR_ALL_TUPLE_ACCUMULATE(M) \
    static constexpr unsigned M() { return ParamTupleImpl<T...>::M(); }
    FOR_ALL_TUPLE_ACCUMULATE(get_context_input_count)
    FOR_ALL_TUPLE_ACCUMULATE(set_context_input_count)
    FOR_ALL_TUPLE_ACCUMULATE(get_context_output_count)
    FOR_ALL_TUPLE_ACCUMULATE(set_context_output_count)
#undef FOR_ALL_TUPLE_ACCUMULATE

  private:
    std::unique_ptr<ParamTupleImpl<T...>> impl;
};
}  // namespace details

template <class... T>
struct ParamTuple
        : public meta::__if<(sizeof(details::ParamTupleImpl<T...>) > 256),
                            details::ParamTupleAlloc<T...>,
                            details::ParamTupleImpl<T...>>::type {};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_PARAM_TUPLE_HXX__
