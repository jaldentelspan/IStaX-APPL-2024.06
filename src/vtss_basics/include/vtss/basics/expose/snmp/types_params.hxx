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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_TYPES_PARAMS_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_TYPES_PARAMS_HXX__

#include <string.h>
#include "vtss/basics/meta.hxx"
#include "vtss/basics/clear.hxx"
#include "vtss/basics/expose/snmp/type-classes.hxx"
#include "vtss/basics/expose/argn.hxx"
#include "vtss/basics/expose/snmp/row-editor-state.hxx"
#include "vtss/basics/expose/snmp/param-tuple.hxx"

namespace vtss {
namespace expose {
namespace snmp {

namespace ArgumentType {
enum E {
    Key,
    Val,
    Action
};
}  // namespace ArgumentType

template<uint32_t OFFSET, typename T, typename U = T>
struct SnmpOidOffset;

template<uint32_t OFFSET, typename T>
struct SnmpOidOffset<OFFSET, T, T> {
    SnmpOidOffset(typename remove_pointer<T>::type &i) : inner(i) { }
    typename remove_pointer<T>::type &inner;
};

template<uint32_t OFFSET, typename T, typename U>
struct SnmpOidOffset {
    SnmpOidOffset(typename remove_pointer<T>::type &i) : inner(i) { }
    U inner;
};

template<typename ARCHIVER, uint32_t OFFSET, typename T, typename U>
void serialize(ARCHIVER &a, SnmpOidOffset<OFFSET, T, U> &v) {
    a.oid_offset_ += OFFSET;
    serialize(a, v.inner);
    a.oid_offset_ -= OFFSET;
}

template<uint32_t OFFSET, typename T, typename U>
struct SnmpBaseType<SnmpOidOffset<OFFSET, T, U>> {
    typedef typename SnmpBaseType<U>::type type;
};

template<typename Handler, uint32_t OFFSET, typename T, typename U>
struct SerializeClass<Handler, SnmpOidOffset<OFFSET, T, U>, TypeClassEnum> {
    void operator()(Handler &h, SnmpOidOffset<OFFSET, T, U> &v) {
        typedef SnmpOidOffset<OFFSET, T, U> Value;
        typedef typename SnmpBaseType<Value>::type BaseType;
        int32_t &tmp = reinterpret_cast<int32_t &>(v.inner);
        serialize_enum(h, tmp, SnmpType<BaseType>::enum_descriptor());
    }
};

template<typename T> struct ParamData {
    typedef T type;
    typedef const T & in_type;
    typedef const T * in_ptr_type;
    typedef T * out_type;
    typedef T * out_ptr_type;

    T &          as_ref() { return data; }
    in_type      as_input_param() const { return data; }
    out_type     as_output_param() { return &data; }
    in_ptr_type  as_input_ptr_param() const { return &data; }
    out_ptr_type as_output_ptr_param() { return &data; }

    T data;
};

template<typename T> struct ParamData<T *> {
    typedef T type;
    typedef const T * in_type;
    typedef const T * in_ptr_type;
    typedef T * out_type;
    typedef T * out_ptr_type;

    T &          as_ref() { return data; }
    in_type      as_input_param() const { return &data; }
    out_type     as_output_param() { return &data; }
    in_ptr_type  as_input_ptr_param() const { return &data; }
    out_ptr_type as_output_ptr_param() { return &data; }

    T data;
};

template<typename T> struct ParamData<T &> {
    typedef T type;
    typedef const T & in_type;
    typedef const T * in_ptr_type;
    typedef T & out_type;
    typedef T * out_ptr_type;

    T &          as_ref() { return data; }
    in_type      as_input_param() const { return data; }
    out_type     as_output_param() { return data; }
    in_ptr_type  as_input_ptr_param() const { return &data; }
    out_ptr_type as_output_ptr_param() { return &data; }

    T data;
};

template<typename T, typename U> struct ParamMetaData : public ParamData<T> {
    typedef typename meta::BaseType<U>::type meta_base_type;
    typedef typename ParamData<T>::type storage_type;
    meta_base_type mdata;
    ParamMetaData() : mdata(ParamData<T>::data) { }
    meta_base_type & as_meta_type() { return mdata; }
    const meta_base_type & as_meta_type() const { return mdata; }
};

template<typename T> struct ParamMetaData<T, T> : public ParamData<T> {
    typedef typename ParamData<T>::type meta_base_type;
    typedef typename ParamData<T>::type storage_type;

    meta_base_type & as_meta_type() {
        return ParamData<T>::as_ref();
    }

    const meta_base_type & as_meta_type() const {
        return ParamData<T>::as_ref();
    }
};

template<class T, class U = T>
struct PARAM_KEY {
    typedef T type;
    typedef typename meta::add_const<T>::type const_type;

    typedef typename meta::add_const<T>::type get_ptr_type;
    typedef typename meta::add_const<T>::type set_ptr_type;

    typedef typename meta::BaseType<T>::type basetype;
    typedef typename meta::BaseType<U>::type metatype; // TODO, delete this
    typedef typename meta::BaseType<U>::type meta_base_type;
    typedef typename ParamMetaData<T, U>::storage_type storage_type;

    typedef typename meta::BaseType<T>::type * ptr_type;
    typedef const typename meta::BaseType<T>::type * const_ptr_type;
    enum IS_KEY    { is_key = 1 };
    enum IS_VAL    { is_val = 0 };
    enum IS_ACTION { is_action = 0 };

    typedef typename ParamData<T>::out_ptr_type def_ptr_type;
    def_ptr_type def_param() {
        return data.as_output_ptr_param();
    }

    typedef typename ParamData<T>::in_type get_param_type;
    get_param_type get_param() {
        return data.as_input_param();
    }

    typedef typename ParamData<T>::in_type set_param_type;
    set_param_type set_param() {
        return data.as_input_param();
    }

    typedef typename ParamData<T>::in_ptr_type itr_param_in_type;
    itr_param_in_type itr_in_param() {
        if (parsed_ok) return data.as_input_ptr_param();
        else           return nullptr;
    }

    typedef typename ParamData<T>::out_ptr_type itr_param_out_type;
    itr_param_out_type itr_out_param() {
        return data_next.as_output_ptr_param();
    }

    ArgumentType::E argument_type() const { return ArgumentType::Key; }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void param_serialize2(HANDLER &h, INTERFACE &i) {
        h.argument_begin(N, ArgumentType::Key);
        typename expose::arg::Int2ArgN<N>::type argn;
        i.argument(h, argn, data.as_ref());
        h.argument_properties_clear();
    }

    template<typename HANDLER>
    bool parse_input(HANDLER &h) {
        typedef typename SnmpBaseType<U>::type snmp_base_type;
        typedef typename SnmpType<snmp_base_type>::TypeClass type_class;
        SerializeClass<HANDLER, typename remove_pointer<U>::type, type_class> s;

        // This is a mess... sorry.
        if (h.force_get_first()) {
            // No need for parsing as we know there is not oid-elements to
            // consume. But if we return false here in a get-next context, the
            // parser will go into the incomplete mode. And the incomplete mode
            // is only needed for partially parsed elements!

            // Signal that the iterator function must be called with a null
            // pointer instead of a pointer to the actually content.
            parsed_ok = false;

            if (h.next_request()) {
                return true;
            } else {
                return false;
            }

        } else {
            s(h, data.as_meta_type());
            parsed_ok = h.ok_;
            return parsed_ok;
        }
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool parse_input2(HANDLER &h, INTERFACE &i) {
        // This is a mess... sorry.
        if (h.force_get_first()) {
            // No need for parsing as we know there is not oid-elements to
            // consume. But if we return false here in a get-next context, the
            // parser will go into the incomplete mode. And the incomplete mode
            // is only needed for partially parsed elements!

            // Signal that the iterator function must be called with a null
            // pointer instead of a pointer to the actually content.
            parsed_ok = false;

            if (h.next_request()) {
                return true;
            } else {
                return false;
            }

        } else {
            typename expose::arg::Int2ArgN<N>::type argn;
            i.argument(h, argn, data.as_ref());
            h.argument_properties_clear();
            parsed_ok = h.ok_;
            return parsed_ok;
        }
    }

    void clear_input() { parsed_ok = false; }

    void memset_zero() {
        static_assert(!is_pod<decltype(*data.as_output_param())>::value,
                      "Only POD data is allowed");
        memset(data.as_output_param(), 0, sizeof(*data.as_output_param()));
    }

    template<typename HANDLER>
    bool export_output(HANDLER &h) {
        typedef typename SnmpBaseType<U>::type snmp_base_type;
        typedef typename SnmpType<snmp_base_type>::TypeClass type_class;
        SerializeClass<HANDLER, typename remove_pointer<U>::type, type_class> s;

        s(h, data.as_meta_type());
        return h.ok_;
    }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool export_output2(HANDLER &h, INTERFACE &i) {
        typename expose::arg::Int2ArgN<N>::type argn;
        i.argument(h, argn, data.as_ref());
        h.argument_properties_clear();
        return h.ok_;
    }

    void copy_next_to_data() {
        parsed_ok = true;
        data.data = data_next.data;
    }

    bool parsed_ok = false;
    ParamMetaData<T, U> data;
    ParamMetaData<T, U> data_next;
};

template<class T, class U = T>
struct PARAM_VAL {
    typedef T type;

    typedef typename meta::add_const<T>::type const_type;

    typedef typename meta::OutputType<T>::type get_ptr_type;
    typedef typename meta::add_const<T>::type set_ptr_type;

    typedef typename meta::BaseType<T>::type basetype;
    typedef typename meta::BaseType<U>::type metatype; // TODO, delete this
    typedef typename meta::BaseType<U>::type meta_base_type;
    typedef typename ParamMetaData<T, U>::storage_type storage_type;
    typedef basetype & ref_type;

    enum IS_KEY        { is_key = 0 };
    enum IS_VAL        { is_val = 1 };
    enum IS_ACTION     { is_action = 0 };

    typedef typename ParamData<T>::out_ptr_type def_ptr_type;
    def_ptr_type def_param() {
        return data.as_output_ptr_param();
    }

    typedef typename ParamData<T>::out_type get_param_type;
    get_param_type get_param() {
        return data.as_output_param();
    }

    typedef typename ParamData<T>::in_type set_param_type;
    set_param_type set_param() {
        return data.as_input_param();
    }

    typedef typename ParamData<T>::in_ptr_type itr_param_in_type;
    itr_param_in_type itr_in_param() { return nullptr; }

    typedef typename ParamData<T>::out_ptr_type itr_param_out_type;
    itr_param_out_type itr_out_param() { return nullptr; }


    template<typename HANDLER> bool parse_input(HANDLER &h) { return true; }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool parse_input2(HANDLER &h, INTERFACE &i) { return true; }

    template<typename HANDLER> bool export_output(HANDLER &h) { return true; }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool export_output2(HANDLER &h, INTERFACE &i) { return true; }

    void clear_input() { }

    ArgumentType::E argument_type() const { return ArgumentType::Val; }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void param_serialize2(HANDLER &h, INTERFACE &i) {
        h.argument_begin(N, ArgumentType::Val);
        typename expose::arg::Int2ArgN<N>::type argn;
        i.argument(h, argn, data.as_ref());
        h.argument_properties_clear();
    }

    void memset_zero() {
        clear(data.as_ref());
    }

    void copy_next_to_data() { }

    ParamMetaData<T, U> data;
};

template<class T, class U = T>
struct PARAM_ACTION {
    // TODO!!!
    typedef T type;

    typedef typename meta::add_const<T>::type const_type;

    typedef typename meta::OutputType<T>::type get_ptr_type;
    typedef typename meta::add_const<T>::type set_ptr_type;

    typedef typename meta::BaseType<T>::type basetype;
    typedef typename meta::BaseType<U>::type metatype; // TODO, delete this
    typedef typename meta::BaseType<U>::type meta_base_type;
    typedef typename ParamMetaData<T, U>::storage_type storage_type;
    enum IS_KEY        { is_key = 0 };
    enum IS_VAL        { is_val = 0 };
    enum IS_ACTION     { is_action = 1 };

    typedef typename ParamData<T>::out_ptr_type def_ptr_type;
    def_ptr_type def_param() {
        return data.as_output_ptr_param();
    }

    typedef typename ParamData<T>::out_type get_param_type;
    get_param_type get_param() {
        return data.as_output_param();
    }

    typedef typename ParamData<T>::in_type set_param_type;
    set_param_type set_param() {
        return data.as_input_param();
    }

    typedef typename ParamData<T>::in_ptr_type itr_param_in_type;
    itr_param_in_type itr_in_param() { return nullptr; }

    typedef typename ParamData<T>::out_ptr_type itr_param_out_type;
    itr_param_out_type itr_out_param() { return nullptr; }


    template<typename HANDLER> bool parse_input(HANDLER &h) { return true; }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool parse_input2(HANDLER &h, INTERFACE &i) { return true; }

    template<typename HANDLER> bool export_output(HANDLER &h) { return true; }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    bool export_output2(HANDLER &h, INTERFACE &i) { return true; }

    void clear_input() { }

    void memset_zero() { }

    void copy_next_to_data() { }

    ArgumentType::E argument_type() const { return ArgumentType::Action; }

    template <typename HANDLER, typename INTERFACE, unsigned N>
    void param_serialize2(HANDLER &h, INTERFACE &i) {
        h.argument_begin(N, ArgumentType::Action);
        h.add_leaf(vtss::AsRowEditorState(data.as_ref().row_state),
                   vtss::tag::Name("Action"),
                   Status::Current,
                   OidElementValue(INTERFACE::snmpRowEditorOid),
                   vtss::tag::Description("Action"));
        h.argument_properties_clear();
    }

    ParamMetaData<T, U> data;
};

template<typename ...T> struct KeyTypeList;
template<> struct KeyTypeList<> { typedef meta::vector<> type; };
template<typename T, typename U, typename ...X>
struct KeyTypeList<PARAM_KEY<T, U>, X...> {
    typedef typename meta::push_head<
        PARAM_KEY<T, U>,
        typename KeyTypeList<X...>::type
    >::type type;
};
template<typename T, typename U, typename ...X>
struct KeyTypeList<PARAM_VAL<T, U>, X...> {
    typedef typename KeyTypeList<X...>::type type;
};
template<typename T, typename U, typename ...X>
struct KeyTypeList<PARAM_ACTION<T, U>, X...> {
    typedef typename KeyTypeList<X...>::type type;
};

template<typename ...T> struct ValTypeList;
template<> struct ValTypeList<> { typedef meta::vector<> type; };
template<typename T, typename U, typename ...X>
struct ValTypeList<PARAM_KEY<T, U>, X...> {
    typedef typename ValTypeList<X...>::type type;
};
template<typename T, typename U, typename ...X>
struct ValTypeList<PARAM_VAL<T, U>, X...> {
    typedef typename meta::push_head<
        PARAM_VAL<T, U>,
        typename ValTypeList<X...>::type
    >::type type;
};
template<typename T, typename U, typename ...X>
struct ValTypeList<PARAM_ACTION<T, U>, X...> {
    typedef typename ValTypeList<X...>::type type;
};

template<typename ...T> struct KeyValTypeList;
template<> struct KeyValTypeList<> { typedef meta::vector<> type; };
template<typename T, typename U, typename ...X>
struct KeyValTypeList<PARAM_KEY<T, U>, X...> {
    typedef typename meta::push_head<
        PARAM_KEY<T, U>,
        typename KeyValTypeList<X...>::type
    >::type type;
};
template<typename T, typename U, typename ...X>
struct KeyValTypeList<PARAM_VAL<T, U>, X...> {
    typedef typename meta::push_head<
        PARAM_VAL<T, U>,
        typename KeyValTypeList<X...>::type
    >::type type;
};
template<typename T, typename U, typename ...X>
struct KeyValTypeList<PARAM_ACTION<T, U>, X...> {
    typedef typename KeyValTypeList<X...>::type type;
};

template<typename ...T> struct DelPtrTypeCalc;
template<typename ...T> struct DelPtrTypeCalc<meta::vector<T...>> {
    typedef mesa_rc (*type)(typename T::const_type...);
};

template<typename ...T> struct ItrPtrTypeListCalc;
template<> struct ItrPtrTypeListCalc<> { typedef meta::vector<> list; };

template<typename T, typename U, typename ...TAIL>
struct ItrPtrTypeListCalc<PARAM_VAL<T, U>, TAIL...> {
    typedef typename ItrPtrTypeListCalc<TAIL...>::list list;
};

template<typename T, typename U, typename ...TAIL>
struct ItrPtrTypeListCalc<PARAM_KEY<T, U>, TAIL...> {
    typedef typename meta::push_head<
        typename PARAM_KEY<T, U>::ptr_type,
        typename ItrPtrTypeListCalc<TAIL...>::list
    >::type list_;

    typedef typename meta::push_head<
        typename PARAM_KEY<T, U>::const_ptr_type, list_
    >::type list;
};

template<typename T, typename U, typename ...TAIL>
struct ItrPtrTypeListCalc<PARAM_ACTION<T, U>, TAIL...> {
    typedef typename ItrPtrTypeListCalc<TAIL...>::list list;
};

template<typename ...T> struct ItrPtrTypeCalc;
template<typename ...T> struct ItrPtrTypeCalc<meta::vector<T...> > {
    typedef mesa_rc (*type)(T...);
};

template<typename ...T> struct GetPtrTypeCalc;
template<typename ...T> struct GetPtrTypeCalc<meta::vector<T...> > {
    typedef mesa_rc (*type)(typename T::get_ptr_type...);
};

template<typename ...T> struct SetDefPtrTypeCalc;
template<typename ...T> struct SetDefPtrTypeCalc<meta::vector<T...> > {
    typedef mesa_rc (*type)(typename T::def_ptr_type...);
};

template<typename ...T> struct SetPtrTypeCalc;
template<typename ...T> struct SetPtrTypeCalc<meta::vector<T...> > {
    typedef mesa_rc (*type)(typename T::set_ptr_type...);
};


// Extract the meta type of a PARAM_ACTION
template <typename ACTIOM>
struct ParamActionType;

// PARAM_ACTION<X, SnmpOidOffset<N, X, ACTION_TYPE>> -> ACTION_TYPE
template <typename X, uint32_t N, typename ACTION_TYPE>
struct ParamActionType<PARAM_ACTION<X, SnmpOidOffset<N, X, ACTION_TYPE>>> {
    typedef ACTION_TYPE type;
};

// PARAM_ACTION<X, ACTION_TYPE> -> ACTION_TYPE
template <typename X, typename ACTION_TYPE>
struct ParamActionType<PARAM_ACTION<X, ACTION_TYPE>> {
    typedef ACTION_TYPE type;
};

// Find any action 'ACTION_LIST' in the list of PARAM_ACTION<...>
template <typename ACTION_LIST, typename ...T>
struct FindAction;

struct NoSuchAction { };

// Base condition
template <typename ACTION_LIST>
struct FindAction<ACTION_LIST> {
    typedef PARAM_ACTION<NoSuchAction> type;
};

// Iterate through the list of actions
template <typename ACTION_LIST, typename T1, typename T2, typename ...TAIL>
struct FindAction<ACTION_LIST, PARAM_ACTION<T1, T2>, TAIL...> {
    typedef typename meta::__if<
        meta::vector_include<  // condition start
            typename ParamActionType<PARAM_ACTION<T1, T2>>::type,
            ACTION_LIST
        >::value,  // condition end
        PARAM_ACTION<T1, T2>,  // true condition
        typename FindAction<ACTION_LIST, TAIL...>::type  // false condition
    >::type type;
};

// T is not an PARAM_ACTION...
template <typename ACTION_LIST, typename T, typename ...TAIL>
struct FindAction<ACTION_LIST, T, TAIL...> {
    typedef typename FindAction<ACTION_LIST, TAIL...>::type type;
};

template <typename ACTION_LIST, typename T>
struct FindAction2;

template <typename ACTION_LIST, typename ...T>
struct FindAction2<ACTION_LIST, meta::vector<T...>> {
    typedef typename FindAction<ACTION_LIST, T...>::type type;
};

} //  namespace snmp
}  // namespace expose
} //  namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_TYPES_PARAMS_HXX__
