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

#ifndef __VTSS_BASICS_EXPOSE_JSON_PARAM_VAL_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_PARAM_VAL_HXX__

#include <vtss/basics/meta.hxx>
#include <vtss/basics/expose/json/param-data.hxx>

namespace vtss {
namespace expose {
namespace json {

template <typename T>
struct ParamValBase : public ParamData<T> {
    static constexpr bool is_value() { return true; }
    static constexpr bool is_key() { return false; }

    // Do nothing, values should not be passed in get context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_get(INTERFACE &i, HANDLER &h) {}

    // Values must be parsed as input in set context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_set(INTERFACE &i, HANDLER &h) {
        ParamData<T>::template serialize<N>(i, h);
        h.increment_argument_index();
    }

    // Do nothing, this is not a key
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_key(INTERFACE &i, HANDLER &h) {}

    // Parse the value
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_val(INTERFACE &i, HANDLER &h) {
        ParamData<T>::template serialize<N>(i, h);
        h.increment_argument_index();
    }

    // Values must be serialized as output in get context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_get(INTERFACE &i, HANDLER &h) {
        ParamData<T>::template serialize<N>(i, h);
    }

    // Do nothing, values should not be serialized for output in set context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_set(INTERFACE &i, HANDLER &h) {}

    // This object represents a value, and must therefor call the serializer
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_val(INTERFACE &i, HANDLER &h) {
        ParamData<T>::template serialize<N>(i, h);
    }

    // This object does not represents a key, and must therefor do nothing
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_key(INTERFACE &i, HANDLER &h) {}


    static constexpr unsigned get_context_input_count() { return 0; }
    static constexpr unsigned set_context_input_count() { return 1; }
    static constexpr unsigned get_context_output_count() { return 1; }
    static constexpr unsigned set_context_output_count() { return 0; }
};

/*
 * A value in get context is used to return information, thuse the param type in
 * get context must be a output.
 *
 * A value in set context is used to pass information, thuse the param type in
 * set context must be a input.
 *
 * The following classes will implement a param_in_set_context and
 * param_in_get_context functions which (depending on T) will return the
 * parameter suitable for the given context.
 *
 * The functionallity can be summerized by the table:
 *
 *    | GET  SET
 * ---+--------------
 * T  | T*   T
 * T* | T*   const T*
 * T& | T&   const T&
 *
 * */

template <typename T>
struct ParamVal : public ParamValBase<T> {
    typedef T param_as_in_t;
    typedef T* param_as_out_t;

    param_as_in_t param_in_set_context() { return ParamValBase<T>::data; }
    param_as_out_t param_in_get_context() { return &(ParamValBase<T>::data); }
    param_as_out_t param_in_getnext_context() { return &(ParamValBase<T>::data); }
};

template <typename T>
struct ParamVal<T*> : public ParamValBase<T> {
    typedef const T* param_as_in_t;
    typedef T* param_as_out_t;

    param_as_in_t param_in_set_context() { return &(ParamValBase<T>::data); }
    param_as_out_t param_in_get_context() { return &(ParamValBase<T>::data); }
    param_as_out_t param_in_getnext_context() { return &(ParamValBase<T>::data); }
};

template <typename T>
struct ParamVal<T&> : public ParamValBase<T> {
    typedef const T& param_as_in_t;
    typedef T& param_as_out_t;

    param_as_in_t param_in_set_context() { return ParamValBase<T>::data; }
    param_as_out_t param_in_get_context() { return ParamValBase<T>::data; }
    param_as_out_t param_in_getnext_context() { return ParamValBase<T>::data; }
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_PARAM_VAL_HXX__
