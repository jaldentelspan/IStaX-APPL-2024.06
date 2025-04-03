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

#ifndef __VTSS_TRACE_STREAM_HXX__
#define __VTSS_TRACE_STREAM_HXX__

#if defined(VTSS_OPSYS_LINUX)
#include "vtss_basics_trace.hxx"
#include "vtss_trace_api.h"
#elif defined(VTSS_BASICS_STANDALONE)
#include "vtss/basics/trace_details_standalone.hxx"
#else
#error "Unknown trace configuration"
#endif

#include "vtss/basics/preprocessor.h"
#include "vtss/basics/trace_common.hxx"

// A few macroes to make tracing easier.
//
// Example:
//      VTSS_TRACE(ERROR) << "This is an error";
//
//      Will emit a trace-error message on the trace_module_id (if defined) at
//      group zero.
//
//      #define RX 1
//      VTSS_TRACE(RX, ERROR) << "This is an error";
//
//      Will emit a trace-error message on the trace_module_id (if defined) at
//      group RX.
//
//      #define RX 1
//      #define SOME_MODULE_ID
//      VTSS_TRACE(SOME_MODULE_ID, RX, ERROR) << "This is an error";
//
//      Will emit a trace-error message on the SOME_MODULE_ID at group RX.
//
// All messages will be terminated with a newline.

#if defined(VTSS_TRACE_DEFAULT_GROUP)
#define VTSS_TRACE_DEFAULT_GROUP_OR_DEFAULT VTSS_TRACE_DEFAULT_GROUP
#else
#define VTSS_TRACE_DEFAULT_GROUP_OR_DEFAULT 0
#endif

#if defined(VTSS_OPSYS_LINUX)
#define VTSS_TRACE3(MOD, GRP, L)                                                  \
    if (TRACE_IS_ENABLED(MOD, GRP, ::vtss::trace::severity_to_vtss_level(         \
                                           ::vtss::trace::severity::L)))          \
        if (::vtss::trace::MessageMetaData meta = ::vtss::trace::MessageMetaData( \
                    __LINE__, __FILE__, MOD, GRP, ::vtss::trace::severity::L))    \
    (*::vtss::trace::message_transaction(meta).stream())

#elif defined(VTSS_BASICS_STANDALONE)
#define VTSS_TRACE3(MOD, GRP, L)                                              \
    if (::vtss::trace::MessageMetaData meta = ::vtss::trace::MessageMetaData( \
                __LINE__, __FILE__, MOD, GRP, ::vtss::trace::severity::L))    \
        if (::vtss::trace::global_settings.enable(meta))                      \
    (*::vtss::trace::message_transaction(meta).stream())

#endif


#define VTSS_TRACE2(GRP, L) VTSS_TRACE3(VTSS_TRACE_MODULE_ID, GRP, L)
#define VTSS_TRACE1(L) VTSS_TRACE2(VTSS_TRACE_DEFAULT_GROUP_OR_DEFAULT, L)

#define VTSS_TOUCH VTSS_TRACE(DEBUG) << "__TOUCH__"

#define VTSS_TRACE(...) PP_TUPLE_OVERLOAD(VTSS_TRACE, __VA_ARGS__)

#endif
