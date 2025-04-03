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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_PARAM_TUPLE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_PARAM_TUPLE_HXX__

#include <memory>
#include <vtss/basics/meta.hxx>
#include <vtss/basics/expose/snmp/type-classes.hxx>

namespace vtss {
namespace expose {
namespace snmp {

namespace details {
template <class... Args>
struct ParamTupleImpl;

template <>
struct ParamTupleImpl<> {
    ParamTupleImpl<> &operator=(const ParamTupleImpl<> &rhs) { return *this; }

    void clear_input() {}

    void memset_zero() {}

    template <typename HANDLER>
    bool parse_input(HANDLER &o) {
        return true;
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool parse_input2(HANDLER &h, INTERFACE &i) {
        return true;
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool parse_input2_(HANDLER &h, INTERFACE &i) {
        return true;
    }

    template <typename HANDLER>
    bool parse_input_(HANDLER &o) {
        return true;
    }

    template <typename HANDLER>
    bool export_output(HANDLER &o) {
        return true;
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool export_output2(HANDLER &h, INTERFACE &i) {
        return true;
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool export_output2_(HANDLER &h, INTERFACE &i) {
        return true;
    }

    template <typename F, class... Args>
    int set_(F f, Args... args) {
        return (*f)(args...);
    }
    template <typename F>
    int set(F f) {
        return (*f)();
    }

    template <typename F, class... Args>
    int def_(F f, Args... args) {
        return (*f)(args...);
    }
    template <typename F>
    int def(F f) {
        return (*f)();
    }

    template <typename F, class... Args>
    int get_(F &&f, Args... args) {
        return f(args...);
    }
    template <typename F>
    int get(F &&f) {
        return f();
    }

    template <typename F, class... Args>
    int del_(F f, Args... args) {
        return (*f)(args...);
    }
    template <typename F>
    int del(F f) {
        return (*f)();
    }

    template <typename F, class... Args>
    int itr_(F f, Args... args) {
        return (*f)(args...);
    }
    template <typename F>
    int itr(F f) {
        return (*f)();
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void do_serialize2_(HANDLER &h, INTERFACE &i) {}

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void do_serialize2_values_(HANDLER &h, INTERFACE &i) {}

    template <typename HANDLER, typename INTERFACE>
    void do_serialize2_values(HANDLER &h, INTERFACE &i) {}

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void do_serialize2_keys_(HANDLER &h, INTERFACE &i) {}

    template <typename HANDLER, typename INTERFACE>
    void do_serialize2_keys(HANDLER &h, INTERFACE &i) {}

    template <typename HANDLER>
    void do_serialize(HANDLER &h) {}
    template <typename HANDLER>
    void do_serialize_values_(HANDLER &h) {}
    template <typename HANDLER>
    void do_serialize_values(HANDLER &h) {}

    void copy_next_to_data() {}
};

template <class Head, class... Tail>
struct ParamTupleImpl<Head, Tail...> {
    Head data;
    ParamTupleImpl<Tail...> rest;

    ParamTupleImpl<Head, Tail...> &operator=(
            const ParamTupleImpl<Head, Tail...> &rhs) {
        data.data.data = rhs.data.data.data;
        rest = rhs.rest;
        return *this;
    }

    void clear_input() {
        data.clear_input();
        rest.clear_input();
    }

    void copy_next_to_data() {
        data.copy_next_to_data();
        rest.copy_next_to_data();
    }

    template <typename HANDLER>
    bool parse_input(HANDLER &o) {
        clear_input();
        return parse_input_(o);
    }

    template <typename HANDLER>
    bool parse_input_(HANDLER &o) {
        if (!data.parse_input(o)) {
            return false;
        }

        return rest.parse_input_(o);
    }

    template <typename HANDLER>
    bool export_output(HANDLER &o) {
        if (!data.export_output(o)) return false;
        return rest.export_output(o);
    }

    // Invoke set-like function pointer
    template <typename F, class... Args>
    int set_(typename meta::enable_if<!Head::is_action, F>::type f, Args... args) {
        typedef typename Head::set_param_type type;
        return rest.template set_<F, Args..., type>(f, args..., data.set_param());
    }

    template <typename F, class... Args>
    int set_(typename meta::enable_if<Head::is_action, F>::type f, Args... args) {
        return rest.template set_<F, Args...>(f, args...);
    }

    template <typename F>
    int set(F f) {
        return set_<F>(f);
    }

    void memset_zero() {
        data.memset_zero();
        rest.memset_zero();
    }

    // Invoke def-like function pointer
    template <typename F, class... Args>
    int def_(typename meta::enable_if<!Head::is_action, F>::type f, Args... args) {
        typedef typename Head::def_ptr_type type;
        return rest.template def_<F, Args..., type>(f, args..., data.def_param());
    }

    template <typename F, class... Args>
    int def_(typename meta::enable_if<Head::is_action, F>::type f, Args... args) {
        return rest.template def_<F, Args...>(f, args...);
    }

    template <typename F>
    void reset_values(F f) {
        if (f)
            def_<F>(f);
        else
            memset_zero();
    }

    // Invoke get-like function pointer
    template <typename F, class... Args>
    int get_(typename meta::enable_if<!Head::is_action, F &&>::type f,
             Args... args) {
        typedef typename Head::get_param_type type;
        return rest.template get_<F, Args..., type>(f, args..., data.get_param());
    }

    template <typename F, class... Args>
    int get_(typename meta::enable_if<Head::is_action, F &&>::type f,
             Args... args) {
        return rest.template set_<F, Args...>(f, args...);
    }
    template <typename F>
    int get(F &&f) {
        return get_<F>(f);
    }

    // Invoke del-like function pointer
    template <typename F, class... Args>
    int del_(typename meta::enable_if<Head::is_key, F>::type f, Args... args) {
        typedef typename Head::set_param_type type;

        return rest.template del_<F, Args..., type>(f, args..., data.set_param());
    }
    template <typename F, class... Args>
    int del_(typename meta::enable_if<!Head::is_key, F>::type f, Args... args) {
        return rest.template del_<F, Args...>(f, args...);
    }
    template <typename F>
    int del(F f) {
        return del_<F>(f);
    }

    // Invoke iterator-like function pointer
    template <typename F, class... Args>
    int itr_(typename meta::enable_if<Head::is_key, F>::type f, Args... args) {
        typedef typename Head::itr_param_in_type type_in;
        typedef typename Head::itr_param_out_type type_out;

        return rest.template itr_<F, Args..., type_in, type_out>(
                f, args..., data.itr_in_param(), data.itr_out_param());
    }
    template <typename F, class... Args>
    int itr_(typename meta::enable_if<!Head::is_key, F>::type f, Args... args) {
        return rest.template itr_<F, Args...>(f, args...);
    }
    template <typename F>
    int itr(F f) {
        return itr_<F>(f);
    }

    // Get value of a given type
    template <typename T>
    typename meta::enable_if<!meta::equal_type<Head, T>::value,
                             typename T::storage_type &>::type
    get_value_() {
        return rest.template get_value_<T>();
    }

    template <typename T>
    typename meta::enable_if<meta::equal_type<Head, T>::value,
                             typename T::storage_type &>::type
    get_value_() {
        return data.data.data;
    }

    template <typename T>
    typename T::storage_type &get_value() {
        return get_value_<T>();
    }

    // Invoke a serializer on all arguments
    template <typename HANDLER>
    void do_serialize(HANDLER &h) {
        typedef typename Head::meta_base_type mata_type_t;
        typedef typename SnmpType<mata_type_t>::TypeClass type_class_t;
        SerializeClass<HANDLER, mata_type_t, type_class_t> s;

        h.argument_begin(-1, data.argument_type());
        auto &m = data.data.as_meta_type();
        s(h, m);  // Invoke serialize
        rest.template do_serialize(h);
    }

    // Invoke the serializer N provided as a member function in INTERFACE
    template <typename HANDLER, typename INTERFACE, unsigned N>
    void do_serialize2_(HANDLER &h, INTERFACE &i) {
        data.template param_serialize2<HANDLER, INTERFACE, N>(h, i);
        rest.template do_serialize2_<HANDLER, INTERFACE, N + 1>(h, i);
    }

    // Prepare invoke member function serializers in INTERFACE
    template <typename HANDLER, typename INTERFACE>
    void do_serialize2(HANDLER &h, INTERFACE &i) {
        do_serialize2_<HANDLER, INTERFACE, 1>(h, i);
    }

    // Invoke a serializer on all value value/action arguments
    template <typename HANDLER>
    void do_serialize_values_(
            typename meta::enable_if<Head::is_key, HANDLER>::type &h) {
        rest.template do_serialize_values_<HANDLER>(h);
    }

    template <typename HANDLER>
    void do_serialize_values_(
            typename meta::enable_if<!Head::is_key, HANDLER>::type &h) {
        typedef typename Head::meta_base_type mata_type_t;
        typedef typename SnmpType<mata_type_t>::TypeClass type_class_t;
        SerializeClass<HANDLER, mata_type_t, type_class_t> s;

        auto &m = data.data.as_meta_type();
        s(h, m);
        rest.template do_serialize_values_<HANDLER>(h);
    }

    template <typename HANDLER>
    void do_serialize_values(HANDLER &h) {
        do_serialize_values_<HANDLER>(h);
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void do_serialize2_values_(
            typename meta::enable_if<Head::is_key, HANDLER>::type &h,
            INTERFACE &i) {
        rest.template do_serialize2_values_<HANDLER, INTERFACE, N + 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void do_serialize2_values_(
            typename meta::enable_if<!Head::is_key, HANDLER>::type &h,
            INTERFACE &i) {
        data.template param_serialize2<HANDLER, INTERFACE, N>(h, i);
        rest.template do_serialize2_values_<HANDLER, INTERFACE, N + 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE>
    void do_serialize2_values(HANDLER &h, INTERFACE &i) {
        do_serialize2_values_<HANDLER, INTERFACE, 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void do_serialize2_keys_(
            typename meta::enable_if<Head::is_key, HANDLER>::type &h,
            INTERFACE &i) {
        data.template param_serialize2<HANDLER, INTERFACE, N>(h, i);
        rest.template do_serialize2_keys_<HANDLER, INTERFACE, N + 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void do_serialize2_keys_(
            typename meta::enable_if<!Head::is_key, HANDLER>::type &h,
            INTERFACE &i) {
        rest.template do_serialize2_keys_<HANDLER, INTERFACE, N + 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE>
    void do_serialize2_keys(HANDLER &h, INTERFACE &i) {
        do_serialize2_keys_<HANDLER, INTERFACE, 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE>
    bool parse_input2(HANDLER &h, INTERFACE &i) {
        clear_input();
        return parse_input2_<HANDLER, INTERFACE, 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool parse_input2_(HANDLER &h, INTERFACE &i) {
        if (!data.template parse_input2<HANDLER, INTERFACE, N>(h, i))
            return false;
        return rest.template parse_input2_<HANDLER, INTERFACE, N + 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE>
    bool export_output2(HANDLER &h, INTERFACE &i) {
        return export_output2_<HANDLER, INTERFACE, 1>(h, i);
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool export_output2_(HANDLER &h, INTERFACE &i) {
        if (!data.template export_output2<HANDLER, INTERFACE, N>(h, i))
            return false;
        return rest.template export_output2_<HANDLER, INTERFACE, N + 1>(h, i);
    }
};

template <class... T>
struct ParamTupleAlloc {
    ParamTupleAlloc() : impl(new ParamTupleImpl<T...>()) {}

    ParamTupleAlloc &operator=(const ParamTupleAlloc &rhs) {
        *impl = *(rhs.impl);
        return *this;
    }

    void clear_input() { impl->clear_input(); }

    template <typename F>
    void reset_values(F f) {
        return impl->reset_values(f);
    }

    template <typename HANDLER>
    bool parse_input(HANDLER &o) {
        return impl->parse_input(o);
    }

    template <typename HANDLER, typename INTERFACE>
    bool parse_input2(HANDLER &h, INTERFACE &i) {
        return impl->parse_input2(h, i);
    }

    template <typename HANDLER>
    bool export_output(HANDLER &o) {
        return impl->export_output(o);
    }

    template <typename HANDLER, typename INTERFACE>
    bool export_output2(HANDLER &h, INTERFACE &i) {
        return impl->export_output2(h, i);
    }

    template <typename F>
    int set(F f) {
        return impl->set(f);
    }

    template <typename F>
    int get(F f) {
        return impl->get(f);
    }

    template <typename F>
    int del(F f) {
        return impl->del(f);
    }

    template <typename F>
    int itr(F f) {
        return impl->itr(f);
    }

    template <typename TT>
    typename TT::storage_type &get_value() {
        return impl->template get_value<TT>();
    }

    template <typename HANDLER, typename INTERFACE>
    void do_serialize2(HANDLER &h, INTERFACE &i) {
        impl->do_serialize2(h, i);
    }

    template <typename HANDLER, typename INTERFACE>
    void do_serialize2_values(HANDLER &h, INTERFACE &i) {
        impl->do_serialize2_values(h, i);
    }

    template <typename HANDLER, typename INTERFACE>
    void do_serialize2_keys(HANDLER &h, INTERFACE &i) {
        impl->do_serialize2_keys(h, i);
    }

    template <typename HANDLER>
    void do_serialize(HANDLER &h) {
        impl->do_serialize(h);
    }

    template <typename HANDLER>
    void do_serialize_values(HANDLER &h) {
        impl->do_serialize_values(h);
    }

    void copy_next_to_data() { impl->copy_next_to_data(); }

  private:
    std::unique_ptr<ParamTupleImpl<T...>> impl;
};

}  // namespace details

template <class... T>
struct ParamTuple
        : public meta::__if<(sizeof(details::ParamTupleImpl<T...>) > 256),
                            details::ParamTupleAlloc<T...>,
                            details::ParamTupleImpl<T...>>::type {};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_PARAM_TUPLE_HXX__
