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

#ifndef __VTSS_BASICS_EXPOSE_TYPES_HXX__
#define __VTSS_BASICS_EXPOSE_TYPES_HXX__

#include <vtss/basics/expose/argn.hxx>

#ifdef VTSS_BASICS_EXPOSE_NULL_PTR
#define VTSS_EXPOSE_GET_PTR(X) static constexpr typename P::get_t get = nullptr
#else
#define VTSS_EXPOSE_GET_PTR(X) static constexpr typename P::get_t get = &X
#endif

#ifdef VTSS_BASICS_EXPOSE_NULL_PTR
#define VTSS_JSON_GET_ALL_PTR(X) \
    static constexpr typename P::json_get_all_t json_get_all = nullptr
#else
#define VTSS_JSON_GET_ALL_PTR(X) \
    static constexpr typename P::json_get_all_t json_get_all = &X
#endif

#ifdef VTSS_BASICS_EXPOSE_NULL_PTR
#define VTSS_EXPOSE_SET_PTR(X) static constexpr typename P::set_t set = nullptr
#else
#define VTSS_EXPOSE_SET_PTR(X) static constexpr typename P::set_t set = &X
#endif

#ifdef VTSS_BASICS_EXPOSE_NULL_PTR
#define VTSS_EXPOSE_ITR_PTR(X) static constexpr typename P::itr_t itr = nullptr
#else
#define VTSS_EXPOSE_ITR_PTR(X) static constexpr typename P::itr_t itr = &X
#endif

#ifdef VTSS_BASICS_EXPOSE_NULL_PTR
#define VTSS_EXPOSE_ADD_PTR(X) static constexpr typename P::add_t add = nullptr
#else
#define VTSS_EXPOSE_ADD_PTR(X) static constexpr typename P::add_t add = &X
#endif

#ifdef VTSS_BASICS_EXPOSE_NULL_PTR
#define VTSS_EXPOSE_DEL_PTR(X) static constexpr typename P::del_t del = nullptr
#else
#define VTSS_EXPOSE_DEL_PTR(X) static constexpr typename P::del_t del = &X
#endif

#ifdef VTSS_BASICS_EXPOSE_NULL_PTR
#define VTSS_EXPOSE_DEF_PTR(X) static constexpr typename P::def_t def = nullptr
#else
#define VTSS_EXPOSE_DEF_PTR(X) static constexpr typename P::def_t def = &X
#endif

#define VTSS_EXPOSE_SERIALIZE_ARG_1(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_1, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_2(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_2, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_3(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_3, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_4(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_4, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_5(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_5, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_6(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_6, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_7(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_7, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_8(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_8, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_9(X) \
    template <typename HANDLER>        \
    void argument(HANDLER &h, ::vtss::expose::arg::_9, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_10(X) \
    template <typename HANDLER>         \
    void argument(HANDLER &h, ::vtss::expose::arg::_10, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_11(X) \
    template <typename HANDLER>         \
    void argument(HANDLER &h, ::vtss::expose::arg::_11, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_12(X) \
    template <typename HANDLER>         \
    void argument(HANDLER &h, ::vtss::expose::arg::_12, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_13(X) \
    template <typename HANDLER>         \
    void argument(HANDLER &h, ::vtss::expose::arg::_13, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_14(X) \
    template <typename HANDLER>         \
    void argument(HANDLER &h, ::vtss::expose::arg::_14, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_15(X) \
    template <typename HANDLER>         \
    void argument(HANDLER &h, ::vtss::expose::arg::_15, X)

#define VTSS_EXPOSE_SERIALIZE_ARG_16(X) \
    template <typename HANDLER>         \
    void argument(HANDLER &h, ::vtss::expose::arg::_16, X)

#ifdef VTSS_SW_OPTION_PRIV_LVL
#define VTSS_EXPOSE_WEB_PRIV(L, M)      \
    static constexpr int priv_type = L; \
    static constexpr int priv_module = M
#else
#define VTSS_EXPOSE_WEB_PRIV(L, M)
#endif

#endif  // __VTSS_BASICS_EXPOSE_TYPES_HXX__
