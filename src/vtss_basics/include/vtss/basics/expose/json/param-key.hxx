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

#ifndef __VTSS_BASICS_EXPOSE_JSON_PARAM_KEY_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_PARAM_KEY_HXX__

#include <vtss/basics/expose/json/param-data.hxx>

/*
 * A key is always use as an input regardless of this is a get or set context.
 * but different functions may either be parsing information by-value,
 * by-reference or by-ponter.
 *
 * The following table show the return type of the param_in_set_context and
 * param_in_get_context methods in the ParamKey classes:
 *
 *    | GET       SET
 * ---+--------------------
 * T  | T         T
 * T* | const T*  const T*
 * T& | const T&  const T&
 *
 * */

namespace vtss {
namespace expose {
namespace json {

template <typename T>
struct ParamKeyBase : public ParamData<T> {
    static constexpr bool is_value() { return false; }
    static constexpr bool is_key() { return true; }

    // Keys must be parsed as input in get context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_get(INTERFACE &i, HANDLER &h) {
        ParamData<T>::template serialize<N>(i, h);
        h.increment_argument_index();
    }

    // Keys must be parsed as input in set context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_set(INTERFACE &i, HANDLER &h) {
        ParamData<T>::template serialize<N>(i, h);
        h.increment_argument_index();
    }

    // Keys must be parsed as input in get context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_key(INTERFACE &i, HANDLER &h) {
        ParamData<T>::template serialize<N>(i, h);
        h.increment_argument_index();
    }

    // This object does not represents a key, and must therefor do nothing
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_val(INTERFACE &i, HANDLER &h) {}

    // Do nothing, keys should not be serialized in get context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_get(INTERFACE &i, HANDLER &h) {}

    // Do nothing, keys should not be serialized in set context
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_set(INTERFACE &i, HANDLER &h) {}

    // This object does not represents a key, and must therefor do nothing
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_val(INTERFACE &i, HANDLER &h) {}

    // This object represents a key, and must therefor call the serializer
    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_key(INTERFACE &i, HANDLER &h) {
        ParamData<T>::template serialize<N>(i, h);
    }

    static constexpr unsigned get_context_input_count() { return 1; }
    static constexpr unsigned set_context_input_count() { return 1; }
    static constexpr unsigned get_context_output_count() { return 0; }
    static constexpr unsigned set_context_output_count() { return 0; }
};

template <typename T>
struct ParamKey : public ParamKeyBase<T> {
    typedef T param_as_in_t;
    typedef T *param_as_out_t;

    param_as_in_t param_in_set_context() { return ParamKeyBase<T>::data; }
    param_as_in_t param_in_get_context() { return ParamKeyBase<T>::data; }
    param_as_out_t param_in_getnext_context() { return &(ParamKeyBase<T>::data); }
};

template <typename T>
struct ParamKey<T *> : public ParamKeyBase<T> {
    typedef const T *param_as_in_t;
    typedef const T *param_as_out_t;

    param_as_in_t param_in_set_context() { return &(ParamKeyBase<T>::data); }
    param_as_in_t param_in_get_context() { return &(ParamKeyBase<T>::data); }
    param_as_out_t param_in_getnext_context() {
        return &(ParamKeyBase<T>::data);
    }
};

template <typename T>
struct ParamKey<T &> : public ParamKeyBase<T> {
    typedef const T &param_as_in_t;
    typedef const T &param_as_out_t;

    param_as_in_t param_in_set_context() { return ParamKeyBase<T>::data; }
    param_as_in_t param_in_get_context() { return ParamKeyBase<T>::data; }
    param_as_out_t param_in_getnext_context() { return ParamKeyBase<T>::data; }
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_PARAM_KEY_HXX__
