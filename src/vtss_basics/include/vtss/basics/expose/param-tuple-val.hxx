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

#ifndef __VTSS_BASICS_EXPOSE_PARAM_TUPLE_VAL_HXX__
#define __VTSS_BASICS_EXPOSE_PARAM_TUPLE_VAL_HXX__

#include <tuple>
#include <type_traits>
#include <vtss/basics/expose/param-data.hxx>

namespace vtss {
namespace expose {

template <class... T>
struct ParamTupleVal;

template <>
struct ParamTupleVal<> {
    void get() const {}
    void set() {}
    bool equal() const { return true; }
};

template <class Head, class... Tail>
struct ParamTupleVal<ParamVal<Head>, Tail...> : public ParamTupleVal<Tail...> {
    template <class H, class... Args>
    bool equal(H h, Args... args) const {
        if (!data.equal(h)) return false;
        return ParamTupleVal<Tail...>::equal(args...);
    }

    template <class H, class... Args>
    void get(H h, Args... args) const {
        data.get(h);
        ParamTupleVal<Tail...>::get(args...);
    }

    template <class H, class... Args>
    void set(H h, Args... args) {
        data.set(h);
        ParamTupleVal<Tail...>::set(args...);
    }

    ParamData<ParamVal<Head>> data;
};

template <class Head, class... Tail>
struct ParamTupleVal<Head, Tail...> : public ParamTupleVal<Tail...> {
    template <class H, class... Args>
    void get(H, Args... args) const {
        return ParamTupleVal<Tail...>::get(args...);
    }

    template <class H, class... Args>
    void set(H, Args... args) {
        return ParamTupleVal<Tail...>::set(args...);
    }

    template <class H, class... Args>
    bool equal(H, Args... args) const {
        return ParamTupleVal<Tail...>::equal(args...);
    }
};

#define IS_VAL typename std::enable_if<Head::is_val, void>::type
#define IS_KEY typename std::enable_if<Head::is_key, void>::type


////////////////////////////////////////////////////////////////////////////////
template <size_t I, class T, class... Args>
void param_val_get_tuple_to_arg_(T& t) {}

// Prototype is value
template <size_t I, class T, class Head, class... Tail>
IS_VAL param_val_get_tuple_to_arg_(T& t, typename Head::get_ptr_type head,
                                   typename Tail::get_ptr_type... tail);

// Prototype is key
template <size_t I, class T, class Head, class... Tail>
IS_KEY param_val_get_tuple_to_arg_(T& t, typename Head::get_ptr_type head,
                                   typename Tail::get_ptr_type... tail);

// Is key
template <size_t I, class T, class Head, class... Tail>
IS_KEY param_val_get_tuple_to_arg_(T& t, typename Head::get_ptr_type head,
                                   typename Tail::get_ptr_type... tail) {
    param_val_get_tuple_to_arg_<I, T, Tail...>(t, tail...);
}

// Is val
template <size_t I, class T, class Head, class... Tail>
IS_VAL param_val_get_tuple_to_arg_(T& t, typename Head::get_ptr_type head,
                                   typename Tail::get_ptr_type... tail) {
    ParamData<Head>::dereference_get_ptr_type(head) = std::get<I>(t);
    param_val_get_tuple_to_arg_<I + 1, T, Tail...>(t, tail...);
}

template <class T, class... Args>
void param_val_get_tuple_to_arg(T& t, typename Args::get_ptr_type... args) {
    param_val_get_tuple_to_arg_<0, T, Args...>(t, args...);
}

////////////////////////////////////////////////////////////////////////////////
template <size_t I, class T, class... Args>
void param_val_get_next_tuple_to_arg_(T& t) {}

// Prototype is value
template <size_t I, class T, class Head, class... Tail>
IS_VAL param_val_get_next_tuple_to_arg_(T& t, typename Head::get_next_type head,
                                   typename Tail::get_next_type... tail);

// Prototype is key
template <size_t I, class T, class Head, class... Tail>
IS_KEY param_val_get_next_tuple_to_arg_(T& t, typename Head::get_next_type head,
                                   typename Tail::get_next_type... tail);

// Is key
template <size_t I, class T, class Head, class... Tail>
IS_KEY param_val_get_next_tuple_to_arg_(T& t, typename Head::get_next_type head,
                                   typename Tail::get_next_type... tail) {
    param_val_get_next_tuple_to_arg_<I, T, Tail...>(t, tail...);
}

// Is val
template <size_t I, class T, class Head, class... Tail>
IS_VAL param_val_get_next_tuple_to_arg_(T& t, typename Head::get_next_type head,
                                   typename Tail::get_next_type... tail) {
    ParamData<Head>::dereference_get_next_type(head) = std::get<I>(t);
    param_val_get_next_tuple_to_arg_<I + 1, T, Tail...>(t, tail...);
}

template <class T, class... Args>
void param_val_get_next_tuple_to_arg(T& t, typename Args::get_next_type... args) {
    param_val_get_next_tuple_to_arg_<0, T, Args...>(t, args...);
}

////////////////////////////////////////////////////////////////////////////////
template <size_t I, class T, class... Args>
void param_val_set_arg_to_tuple_(T& t) {}

// Prototype is value
template <size_t I, class T, class Head, class... Tail>
IS_VAL param_val_set_arg_to_tuple_(T& t, typename Head::set_ptr_type head,
                                   typename Tail::set_ptr_type... tail);

// Prototype is key
template <size_t I, class T, class Head, class... Tail>
IS_KEY param_val_set_arg_to_tuple_(T& t, typename Head::set_ptr_type head,
                                   typename Tail::set_ptr_type... tail);

// Is key
template <size_t I, class T, class Head, class... Tail>
IS_KEY param_val_set_arg_to_tuple_(T& t, typename Head::set_ptr_type head,
                                   typename Tail::set_ptr_type... tail) {
    param_val_set_arg_to_tuple_<I, T, Tail...>(t, tail...);
}

// Is val
template <size_t I, class T, class Head, class... Tail>
IS_VAL param_val_set_arg_to_tuple_(T& t, typename Head::set_ptr_type head,
                                   typename Tail::set_ptr_type... tail) {
    std::get<I>(t) = ParamData<Head>::dereference_set_ptr_type(head);
    param_val_set_arg_to_tuple_<I + 1, T, Tail...>(t, tail...);
}

template <class T, class... Args>
void param_val_set_arg_to_tuple(T& t, typename Args::set_ptr_type... args) {
    param_val_set_arg_to_tuple_<0, T, Args...>(t, args...);
}

#undef IS_VAL
#undef IS_KEY

}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_PARAM_TUPLE_VAL_HXX__
