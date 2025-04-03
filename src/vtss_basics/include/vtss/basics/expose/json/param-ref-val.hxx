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

#ifndef __VTSS_BASICS_EXPOSE_JSON_PARAM_REF_VAL_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_PARAM_REF_VAL_HXX__

#include <vtss/basics/expose/json/param-ref-data.hxx>

namespace vtss {
namespace expose {
namespace json {

template <typename T>
struct ParamRefValBase : public ParamRefData<T> {
    ParamRefValBase(T *t) : ParamRefData<T>(t) {}

    static constexpr bool is_value() { return true; }
    static constexpr bool is_key() { return false; }

    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_get(INTERFACE &i, HANDLER &h) {
        ParamRefData<T>::template serialize<N>(i, h);
        h.increment_argument_index();
    }

    template <unsigned N, typename INTERFACE, typename HANDLER>
    void parse_set(INTERFACE &i, HANDLER &h) {}

    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_get(INTERFACE &i, HANDLER &h) {}

    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_set(INTERFACE &i, HANDLER &h) {
        ParamRefData<T>::template serialize<N>(i, h);
    }

    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_val(INTERFACE &i, HANDLER &h) {
        ParamRefData<T>::template serialize<N>(i, h);
    }

    template <unsigned N, typename INTERFACE, typename HANDLER>
    void serialize_key(INTERFACE &i, HANDLER &h) {}

    static constexpr unsigned get_context_input_count() { return 0; }
    static constexpr unsigned set_context_input_count() { return 1; }
    static constexpr unsigned get_context_output_count() { return 1; }
    static constexpr unsigned set_context_output_count() { return 0; }
};

template <typename T>
struct ParamRefVal : public ParamRefValBase<T> {
    typedef T param_as_in_t;
    typedef T *param_as_out_t;
    typedef T &ref_type;

    ParamRefVal(T &t) : ParamRefValBase<T>(&t) {}
    param_as_in_t param_in_set_context() { return *ParamRefValBase<T>::data; }
    param_as_out_t param_in_get_context() { return ParamRefValBase<T>::data; }
};

template <typename T>
struct ParamRefVal<T *> : public ParamRefValBase<T> {
    typedef const T *param_as_in_t;
    typedef T *param_as_out_t;
    typedef T *ref_type;

    ParamRefVal(T *t) : ParamRefValBase<T>(t) {}
    param_as_in_t param_in_set_context() { return ParamRefValBase<T>::data; }
    param_as_out_t param_in_get_context() { return ParamRefValBase<T>::data; }
};

template <typename T>
struct ParamRefVal<T &> : public ParamRefValBase<T> {
    typedef const T &param_as_in_t;
    typedef T &param_as_out_t;
    typedef T &ref_type;

    ParamRefVal(T &t) : ParamRefValBase<T>(&t) {}
    param_as_in_t param_in_set_context() { return *ParamRefValBase<T>::data; }
    param_as_out_t param_in_get_context() { return *ParamRefValBase<T>::data; }
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_PARAM_REF_VAL_HXX__
