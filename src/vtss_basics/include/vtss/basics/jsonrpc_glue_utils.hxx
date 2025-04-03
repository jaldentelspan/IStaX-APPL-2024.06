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


/* Read this first, then complain....
 *
 * This code tries to make it as easy as possible to implement rpc-exporter and
 * rpc-importer for existing functions.
 *
 * The usage of this code is not mandatory, but if your needs are like mine,
 * this will most likely help you get the job done.
 *
 * Here is how it should be used:
 *
 * - Create a public header file for your component and call it
 *   <name>_rpc_glue_impl.hxx
 *
 * - Include this file and the "normal" public header file for your module
 *
 * - Declare the (json-rpc) namespaces you need. May be done like this:
 *       VTSS_PUBLIC_NAMESPACE(ns_test,       "test");
 *       VTSS_PUBLIC_NAMESPACE(ns_debug_test, "test");
 *
 * - Use the VTSS_PUBLIC_FUNCTION macro to declare how each functions should be
 *   glued into the rpc server/client
 *
 *       Assuming we have two functions in our public module header file:
 *       mesa_rc bar1(int);
 *       mesa_rc bar2(bool, int);
 *
 *       Gluing for these functions may be declared like this:
 *
 *       VTSS_PUBLIC_FUNCTION(bar1,                 // function pointer
 *                            "bar1",               // function name
 *                            ns_test,              // function namespace
 *
 *                            // function argument list WITH OUT COMMA!!!
 *                            VTSS_PARAM_AUTO(int)  // type of arg 0
 *                            VTSS_PARAM_LAST) // TERMINATES ARGUMENT LIST
 *
 *       VTSS_PUBLIC_FUNCTION(bar2,                 // function pointer
 *                            "bar2",               // function name
 *                            ns_test,              // function namespace
 *
 *                            // function argument list WITH OUT COMMA!!!
 *                            VTSS_PARAM_AUTO(bool) // type of arg 0
 *                            VTSS_PARAM_AUTO(int)  // type of arg 1
 *                            VTSS_PARAM_LAST) // TERMINATES ARGUMENT LIST
 *
 * - Add a new file called: <name>_rpc_exporter.cxx and include it in the normal
 *   compilation.
 *
 *   The file must define VTSS_IMPL_EXPORTER before including
 *   <name>_rpc_glue_impl.hxx
 *
 * - Customers which needs the importer can compile it by setting
 *   VTSS_IMPL_IMPORTER
 *
 * */

#ifndef __VTSS_JSONRPC_GLUE_UTILS_HXX__
#define __VTSS_JSONRPC_GLUE_UTILS_HXX__

#include "vtss/basics/preprocessor.h"

#define NEWLINE

#define VTSS_PARAM_AUTO(X, ...)                          \
    ((::vtss::json::PARAM_AUTO<__VA_ARGS__>), (__VA_ARGS__)),
#define VTSS_PARAM_OUT(X, ...)                           \
    ((::vtss::json::PARAM_OUT<__VA_ARGS__>), (__VA_ARGS__)),
#define VTSS_PARAM_IN(X, ...)                            \
    ((::vtss::json::PARAM_IN<__VA_ARGS__>), (__VA_ARGS__)),
#define VTSS_PARAM_IN_OUT(X, ...)                        \
    ((::vtss::json::PARAM_IN_OUT<__VA_ARGS__>), (__VA_ARGS__)),

#define VTSS_POP_PARENTHESES__(...) __VA_ARGS__
#define VTSS_POP_PARENTHESES(X) VTSS_POP_PARENTHESES__ X

#define VTSS_PARAM_GET(X) VTSS_POP_PARENTHESES(PP_EXPAND(PP_TUPLE_0 X))
#define VTSS_PARAM_INNER_GET(X) VTSS_POP_PARENTHESES(PP_EXPAND(PP_TUPLE_1 X))


#if defined(VTSS_IMPL_IMPORTER)

// Bound parameters must expand to nothing on the importer side
#define VTSS_PARAM_NULLPTR(...)

#define VTSS_PUBLIC_NAMESPACE(var_name, ns_name)                               \
    ::vtss::json::NamespaceImport var_name(ns_name)

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION1(func, rpc_name, ns,                              \
                              dummy)                                   NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func() {                                                NEWLINE \
    return import##_##func();                                          NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION2(func, rpc_name, ns,                              \
                              a0, dummy)                               NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0)                                             NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func(VTSS_PARAM_INNER_GET(a0) _a0) {                    NEWLINE \
    return import##_##func(_a0);                                       NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION3(func, rpc_name, ns,                              \
                              a0, a1, dummy)                           NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1)                                             NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func(VTSS_PARAM_INNER_GET(a0) _a0,                              \
                    VTSS_PARAM_INNER_GET(a1) _a1) {                    NEWLINE \
    return import##_##func(_a0, _a1);                                  NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION4(func, rpc_name, ns,                              \
                              a0, a1, a2, dummy)                       NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2)                                             NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func(VTSS_PARAM_INNER_GET(a0) _a0,                              \
                    VTSS_PARAM_INNER_GET(a1) _a1,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a2) _a2) {                    NEWLINE \
    return import##_##func(_a0, _a1, _a2);                             NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION5(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, dummy)                   NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3)                                             NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func(VTSS_PARAM_INNER_GET(a0) _a0,                              \
                    VTSS_PARAM_INNER_GET(a1) _a1,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a2) _a2,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a3) _a3) {                    NEWLINE \
    return import##_##func(_a0, _a1, _a2, _a3);                        NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION6(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, a4, dummy)               NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3),                                            NEWLINE \
        VTSS_PARAM_GET(a4)                                             NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func(VTSS_PARAM_INNER_GET(a0) _a0,                              \
                    VTSS_PARAM_INNER_GET(a1) _a1,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a2) _a2,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a3) _a3,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a4) _a4) {                    NEWLINE \
    return import##_##func(_a0, _a1, _a2, _a3, _a4);                   NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION7(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, a4, a5, dummy)           NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3),                                            NEWLINE \
        VTSS_PARAM_GET(a4),                                            NEWLINE \
        VTSS_PARAM_GET(a5)                                             NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func(VTSS_PARAM_INNER_GET(a0) _a0,                              \
                    VTSS_PARAM_INNER_GET(a1) _a1,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a2) _a2,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a3) _a3,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a4) _a4,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a5) _a5) {                    NEWLINE \
    return import##_##func(_a0, _a1, _a2, _a3, _a4, _a5);              NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION8(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, a4, a5, a6, dummy)       NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3),                                            NEWLINE \
        VTSS_PARAM_GET(a4),                                            NEWLINE \
        VTSS_PARAM_GET(a5),                                            NEWLINE \
        VTSS_PARAM_GET(a6)                                             NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func(VTSS_PARAM_INNER_GET(a0) _a0,                              \
                    VTSS_PARAM_INNER_GET(a1) _a1,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a2) _a2,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a3) _a3,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a4) _a4,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a5) _a5,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a6) _a6) {                    NEWLINE \
    return import##_##func(_a0, _a1, _a2, _a3, _a4, _a5, _a6);         NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION9(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, a4, a5, a6, a7, dummy)   NEWLINE \
::vtss::json::FunctionImporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3),                                            NEWLINE \
        VTSS_PARAM_GET(a4),                                            NEWLINE \
        VTSS_PARAM_GET(a5),                                            NEWLINE \
        VTSS_PARAM_GET(a6),                                            NEWLINE \
        VTSS_PARAM_GET(a7)                                             NEWLINE \
> import##_##func(&ns, rpc_name);                                      NEWLINE \
extern "C" int func(VTSS_PARAM_INNER_GET(a0) _a0,                              \
                    VTSS_PARAM_INNER_GET(a1) _a1,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a2) _a2,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a3) _a3,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a4) _a4,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a5) _a5,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a6) _a6,                      NEWLINE \
                    VTSS_PARAM_INNER_GET(a7) _a7) {                    NEWLINE \
    return import##_##func(_a0, _a1, _a2, _a3, _a4, _a5, _a6, _a7);    NEWLINE \
}

////////////////////////////////////////////////////////////////////////////////
#elif defined(VTSS_IMPL_EXPORTER)
////////////////////////////////////////////////////////////////////////////////

#define VTSS_PUBLIC_NAMESPACE(var_name, ns_name)                               \
    ::vtss::json::NamespaceNode var_name(ns_name)

#define VTSS_PARAM_NULLPTR(...)                                                \
    ((::vtss::json::PARAM_NULLPTR<__VA_ARGS__>), (__VA_ARGS__)),

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION1(func, rpc_name, ns,                              \
                              dummy)                                   NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION2(func, rpc_name, ns,                              \
                              a0, dummy)                               NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0)                                             NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION3(func, rpc_name, ns,                              \
                              a0, a1, dummy)                           NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1)                                             NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION4(func, rpc_name, ns,                              \
                              a0, a1, a2, dummy)                       NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2)                                             NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION5(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, dummy)                   NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3)                                             NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION6(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, a4, dummy)               NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3),                                            NEWLINE \
        VTSS_PARAM_GET(a4)                                             NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION7(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, a4, a5, dummy)           NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3),                                            NEWLINE \
        VTSS_PARAM_GET(a4),                                            NEWLINE \
        VTSS_PARAM_GET(a5)                                             NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION8(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, a4, a5, a6, dummy)       NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3),                                            NEWLINE \
        VTSS_PARAM_GET(a4),                                            NEWLINE \
        VTSS_PARAM_GET(a5),                                            NEWLINE \
        VTSS_PARAM_GET(a6)                                             NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#define VTSS_PUBLIC_FUNCTION9(func, rpc_name, ns,                              \
                              a0, a1, a2, a3, a4, a5, a6, a7, dummy)   NEWLINE \
::vtss::json::FunctionExporter<                                        NEWLINE \
        VTSS_PARAM_GET(a0),                                            NEWLINE \
        VTSS_PARAM_GET(a1),                                            NEWLINE \
        VTSS_PARAM_GET(a2),                                            NEWLINE \
        VTSS_PARAM_GET(a3),                                            NEWLINE \
        VTSS_PARAM_GET(a4),                                            NEWLINE \
        VTSS_PARAM_GET(a5),                                            NEWLINE \
        VTSS_PARAM_GET(a6),                                            NEWLINE \
        VTSS_PARAM_GET(a7)                                             NEWLINE \
> export##_##func(&ns, rpc_name, &func);                               NEWLINE \

////////////////////////////////////////////////////////////////////////////////
#else
//# error "Either VTSS_IMPL_IMPORTER or VTSS_IMPL_EXPORTER must be defined"
#endif

#define VTSS_PUBLIC_FUNCTION_OVERLOAD(N, func, rpc_name, ns, ...) \
    PP_EXPAND(PP_CONCATENATE(VTSS_PUBLIC_FUNCTION, N) \
            (func, rpc_name, ns, __VA_ARGS__))

#define VTSS_PUBLIC_FUNCTION(func, rpc_name, ns, ...)   \
    VTSS_PUBLIC_FUNCTION_OVERLOAD(PP_TUPLE_ARGV_CNT(__VA_ARGS__), \
                        func, rpc_name, ns, __VA_ARGS__)


#endif  // __VTSS_JSONRPC_GLUE_UTILS_HXX__

