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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_PARAM_LIST_CONVERTER_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_PARAM_LIST_CONVERTER_HXX__

#include <vtss/basics/meta.hxx>
#include <vtss/basics/expose.hxx>
#include <vtss/basics/expose/snmp/types_params.hxx>
#include <vtss/basics/expose/snmp/row-editor-state.hxx>

namespace vtss {
namespace expose {
namespace snmp {

template <typename T>
struct ParamConvert;

template <typename T>
struct ParamConvert<expose::ParamVal<T>> {
    typedef PARAM_VAL<T> type;
};

template <typename T>
struct ParamConvert<expose::ParamKey<T>> {
    typedef PARAM_KEY<T> type;
};

/*
template <typename... T>
struct ParamListConvert;

template <typename... T>
struct ParamListConvert<expose::ParamList<T...>> {
    typedef ParamTuple<typename ParamConvert<T>::type...> type;
};

template <template <typename... A> class OUTER, typename... T>
struct ParamListConvertEncapsulate;

template <template <typename... A> class OUTER, typename... T>
struct ParamListConvertEncapsulate<OUTER, expose::ParamList<T...>> {
    typedef OUTER<typename ParamConvert<T>::type...> type;
};


template <typename... T>
struct ParamListConvertRowEditor;

template <typename... T>
struct ParamListConvertRowEditor<expose::ParamList<T...>> {
    typedef ParamTuple<typename ParamConvert<T>::type...,
                       PARAM_ACTION<RowEditorState2>> type;
};

template <template <typename... A> class OUTER, typename... T>
struct ParamListConvertRowEditorEncapsulate;

template <template <typename... A> class OUTER, typename... T>
struct ParamListConvertRowEditorEncapsulate<OUTER, expose::ParamList<T...>> {
    typedef OUTER<typename ParamConvert<T>::type...,
                  PARAM_ACTION<RowEditorState2>> type;
};
*/



namespace details {

// Check for snmpRowEditorOid - start //////////////////////////////////////////
namespace HasSnmpRowEditorOidNs {
template <uint32_t>
struct requires_a_constant : true_type {};

template <class T>
requires_a_constant<T::snmpRowEditorOid> check(int);

template <class>
false_type check(...);
}  // namespace HasSnmpRowEditorOidNs

template <class T>
struct HasSnmpRowEditorOid : decltype(HasSnmpRowEditorOidNs::check<T>(0)) {};

template <typename INTERFACE, typename LIST, bool ACTIVE>
struct push_row_editor;

template <typename INTERFACE, typename LIST>
struct push_row_editor<INTERFACE, LIST, false> {
    typedef LIST type;
};

template <typename INTERFACE, typename LIST>
struct push_row_editor<INTERFACE, LIST, true> {
    typedef typename meta::push_back<PARAM_ACTION<RowEditorState2>, LIST>::type
            type;
};
// Check for snmpRowEditorOid - end ////////////////////////////////////////////


template <template <typename... A> class OUTER, typename LIST>
struct ParamListConvertDoEncapsulate;

template <template <typename... A> class OUTER, typename... ARGS>
struct ParamListConvertDoEncapsulate<OUTER, meta::vector<ARGS...>> {
    typedef OUTER<ARGS...> type;
};

template <template <typename... A> class OUTER, typename IF, typename LIST>
struct ParamListConvert2;

template <template <typename... A> class OUTER, typename IF, typename... T>
struct ParamListConvert2<OUTER, IF, expose::ParamList<T...>> {
    // Start by converting the individual arguments
    typedef meta::vector<typename ParamConvert<T>::type...> LIST0;

    // If interface contains a snmpRowEditorOid constexpr, then add a
    // PARAM_ACTION<RowEditorState2> type to the list
    typedef typename details::push_row_editor<
            IF, LIST0, details::HasSnmpRowEditorOid<IF>::value>::type LIST1;

    // The list is complete, expose it encapsulated
    typedef typename details::ParamListConvertDoEncapsulate<OUTER, LIST1>::type
            type;
};

}  // namespace details

template <typename IF>
struct Interface2ParamTuple {
    typedef typename details::ParamListConvert2<ParamTuple, IF,
                                                typename IF::P>::type type;
};

template <template <typename... A> class OUTER, typename IF>
struct Interface2Any {
    typedef typename details::ParamListConvert2<OUTER, IF, typename IF::P>::type
            type;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_PARAM_LIST_CONVERTER_HXX__
