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

#ifndef __VTSS_BASICS_EXPOSE_JSON_PARAM_REF_TUPLE_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_PARAM_REF_TUPLE_HXX__

namespace vtss {
namespace expose {
namespace json {

template <class... Args>
struct ParamRefTuple;

template <>
struct ParamRefTuple<> {
    ParamRefTuple() {}

#define BASE_CASE(X)                                \
    template <typename INTERFACE, typename HANDLER> \
    void X(HANDLER &h) {}

    BASE_CASE(serialize_get)
    BASE_CASE(serialize_set)
    BASE_CASE(serialize_keys)
    BASE_CASE(serialize_values)
    BASE_CASE(parse_get)
    BASE_CASE(parse_set)
#undef BASE_CASE

    static constexpr unsigned get_context_input_count() { return 0; }
    static constexpr unsigned set_context_input_count() { return 0; }
    static constexpr unsigned get_context_output_count() { return 0; }
    static constexpr unsigned set_context_output_count() { return 0; }
  protected:
#define BASE_CASE(X)                                            \
    template <unsigned N, typename INTERFACE, typename HANDLER> \
    void X(INTERFACE &i, HANDLER &h) {}

    BASE_CASE(serialize_get_)
    BASE_CASE(serialize_set_)
    BASE_CASE(serialize_keys_)
    BASE_CASE(serialize_values_)
    BASE_CASE(parse_get_)
    BASE_CASE(parse_set_)
#undef BASE_CASE
};

template <class Head, class... Tail>
struct ParamRefTuple<Head, Tail...> : private ParamRefTuple<Tail...> {
    ParamRefTuple(typename Head::ref_type h, typename Tail::ref_type... tail)
        : ParamRefTuple<Tail...>(tail...), data(h) {}

#define WRAP(X, Y)                                  \
    template <typename INTERFACE, typename HANDLER> \
    void X(HANDLER &h) {                            \
        INTERFACE interface;                        \
        Y<1>(interface, h);                         \
    }

    WRAP(serialize_get, serialize_get_)
    WRAP(serialize_set, serialize_set_)
    WRAP(serialize_values, serialize_values_)
    WRAP(serialize_keys, serialize_keys_)
    WRAP(parse_get, parse_get_)
    WRAP(parse_set, parse_set_)
#undef WRAP

#define FOR_ALL_TUPLE_ACCUMULATE(M)                  \
    static constexpr unsigned M() {                  \
        return Head::M() + ParamRefTuple<Tail...>::M(); \
    }
    FOR_ALL_TUPLE_ACCUMULATE(get_context_input_count)
    FOR_ALL_TUPLE_ACCUMULATE(set_context_input_count)
    FOR_ALL_TUPLE_ACCUMULATE(get_context_output_count)
    FOR_ALL_TUPLE_ACCUMULATE(set_context_output_count)
#undef FOR_ALL_TUPLE_ACCUMULATE

  protected:
#define FOR_ALL_TUPLE_ELEMENT(TUPLE_METHOD, PARAM_METHOD)           \
    template <unsigned N, typename INTERFACE, typename HANDLER>     \
    void TUPLE_METHOD(INTERFACE &i, HANDLER &h) {                   \
        data.template PARAM_METHOD<N>(i, h);                        \
        if (!h.ok()) return;                                        \
        ParamRefTuple<Tail...>::template TUPLE_METHOD<N + 1>(i, h); \
    }

    FOR_ALL_TUPLE_ELEMENT(serialize_get_, serialize_get)
    FOR_ALL_TUPLE_ELEMENT(serialize_set_, serialize_set)
    FOR_ALL_TUPLE_ELEMENT(serialize_values_, serialize_val)
    FOR_ALL_TUPLE_ELEMENT(serialize_keys_, serialize_key)
    FOR_ALL_TUPLE_ELEMENT(parse_get_, parse_get)
    FOR_ALL_TUPLE_ELEMENT(parse_set_, parse_set)
#undef FOR_ALL_TUPLE_ELEMENT

  private:
    Head data;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_PARAM_REF_TUPLE_HXX__
