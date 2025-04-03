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

#ifndef __VTSS_TRACE_STREAM_BASICS_HXX__
#define __VTSS_TRACE_STREAM_BASICS_HXX__

#if defined(VTSS_OPSYS_LINUX)
#  include "vtss_basics_trace.hxx"
#elif defined(VTSS_BASICS_STANDALONE)
#  include "vtss/basics/trace_details_standalone.hxx"
#else
#  error "Unknown trace configuration"
#endif

#include "vtss/basics/preprocessor.h"
#include "vtss/basics/trace_common.hxx"
#include "vtss/basics/trace_grps.hxx"

#ifndef VTSS_BASICS_TRACE_MODULE_ID
#define VTSS_BASICS_TRACE_MODULE_ID 113
#endif

#if defined(VTSS_TRACE_DEFAULT_GROUP)
#  define VTSS_BASICS_TRACE_DEFAULT_GROUP_OR_DEFAULT VTSS_TRACE_DEFAULT_GROUP
#else
#  define VTSS_BASICS_TRACE_DEFAULT_GROUP_OR_DEFAULT 0
#endif

#define VTSS_BASICS_TRACE1(L)                                                  \
    if (::vtss::trace::MessageMetaData meta =                                  \
        ::vtss::trace::MessageMetaData(__LINE__, __FILE__,                     \
                                       VTSS_BASICS_TRACE_MODULE_ID,            \
                                       VTSS_BASICS_TRACE_DEFAULT_GROUP_OR_DEFAULT,    \
                                       ::vtss::trace::severity::L))            \
        if (::vtss::trace::global_settings.enable(meta))                       \
            (*::vtss::trace::message_transaction(meta).stream())

#define VTSS_BASICS_TRACE2(GRP, L)                                             \
    if (::vtss::trace::MessageMetaData meta =                                  \
            ::vtss::trace::MessageMetaData(__LINE__, __FILE__,                 \
                                           VTSS_BASICS_TRACE_MODULE_ID,        \
                                           GRP,                                \
                                           ::vtss::trace::severity::L))        \
        if (::vtss::trace::global_settings.enable(meta))                       \
            (*::vtss::trace::message_transaction(meta).stream())

#define VTSS_BASICS_TRACE3(MOD, GRP, L)                                        \
    if (::vtss::trace::MessageMetaData meta =                                  \
            ::vtss::trace::MessageMetaData(__LINE__, __FILE__,                 \
                                           MOD, GRP,                           \
                                           ::vtss::trace::severity::L))        \
        if (::vtss::trace::global_settings.enable(meta))                       \
            (*::vtss::trace::message_transaction(meta).stream())

#define VTSS_BASICS_TOUCH VTSS_TRACE(DEBUG) << "__TOUCH__"
#define VTSS_BASICS_TRACE(...) PP_TUPLE_OVERLOAD(VTSS_BASICS_TRACE, __VA_ARGS__)

#endif
