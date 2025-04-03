/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_APPL_OPTIONAL_MODULE_HXX_
#define _VTSS_APPL_OPTIONAL_MODULE_HXX_

#include <stdint.h>

#include <vtss/basics/type_traits.hxx>

#define VTSS_PP_PTR_PREFIX_NAME vtss_appl_module_
#define VTSS_PP_MODULE_TYPE_POSTFIX _t
#define VTSS_PP_CAT(a, ...) VTSS_PP_PRIMITIVE_CAT(a, __VA_ARGS__)
#define VTSS_PP_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

// The WrapModuleMethods::call method simplify the process of calling a function
// pointer with in an instance of a structure.
// The purpose of accessing methods through a function-pointer within a
// structure, is to limit the amount of symbols needed when optional-modules
// start calling each other (limiting the number of symbols, will lower the
// overhead in terms of size and load-time).
// It should only be used by the MODULE_WRAP macro. It create a static methods,
// which then can be aliashed to a name in the global name-space.
template <typename FPTR, typename CLASS, void **, size_t OFFSET>
struct WrapModuleMethods;

template <typename R, typename... Args, typename T, void **PP, size_t OFFSET>
struct WrapModuleMethods<R (*)(Args...), T, PP, OFFSET> {
    static R call(Args ... args) {
        typedef R (*fptr)(Args ...);

        if (!*PP) return -1;

        void *p = (void *)PP;
        p = *(void **)p;
        p = (void *)(((uint8_t *)p) + OFFSET);
        p = *(void **)p;

        fptr f = (fptr)p;

        return f(vtss::forward<Args>(args)...);
    }
};

// This macro is used to generate an alias to the method published by the
// optional module.
// The optional module should define all its methods with in the
// vtss::appl::<module-name> name space.
// When using this macro, the public methods will be made avialable in the
// global name-space.
// All methods must therefore still be prefixed with vtss_appl_<module-name>.
#define MODULE_WRAP(GLOBAL_NAME, NAME, OBJ, PTR, MODULE_NAME)           \
    static constexpr auto &GLOBAL_NAME =                                \
            WrapModuleMethods<decltype(&vtss::appl::MODULE_NAME::NAME), \
                              OBJ **, &PTR, offsetof(OBJ, NAME)>::call

#endif
