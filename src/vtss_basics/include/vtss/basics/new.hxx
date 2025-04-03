/* *****************************************************************************
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
 **************************************************************************** */

#ifndef __VTSS_BASICS_NEW_HXX__
#define __VTSS_BASICS_NEW_HXX__

#include <new>
#include "vtss/basics/meta.hxx"
#include "vtss/basics/utility.hxx"
#include "vtss/basics/predefs.hxx"

#if defined(VTSS_BASICS_STANDALONE)
#  include <stdlib.h>
#  define VTSS_BASICS_FREE(p) ::free(p)
#  define VTSS_BASICS_MALLOC(s) ::malloc(s)
#  define VTSS_BASICS_CALLOC(n, s) ::calloc(n, s)

#elif defined(VTSS_OPSYS_LINUX)
#  include "main.h"
#  include "vtss/appl/module_id.h"
#  define VTSS_BASICS_FREE(p) VTSS_FREE(p)
#  define VTSS_BASICS_MALLOC(s) VTSS_MALLOC_MODID(VTSS_MODULE_ID_BASICS, s, __FILE__, __LINE__)
#  define VTSS_BASICS_CALLOC(n, s) VTSS_CALLOC_MODID(VTSS_MODULE_ID_BASICS, n, s, __FILE__, __LINE__)
#  define VTSS_BASICS_FREE(p)  VTSS_FREE(p)

#else
#  error "Unknown free/alloc configuration"

#endif

namespace vtss {
#if 0
namespace details_new {
template <unsigned ID, typename T_>
struct ObjectWrapper : public T_ {
    template <typename... Args>
    ObjectWrapper(Args &&... args)
        : T_(forward<Args>(args)...) {}

    void *operator new(std::size_t count) {
#if defined(VTSS_OPSYS_LINUX)
        return VTSS_CALLOC_MODID(ID, 1, sizeof(ObjectWrapper<ID, T_>), __FILE__, __LINE__);
#elif defined(VTSS_BASICS_STANDALONE)
        return calloc(1, sizeof(ObjectWrapper<ID, T_>));
#else
#  error "Unknown free/alloc configuration"
#endif
    }

    void operator delete(void *p) {
#if defined(VTSS_OPSYS_LINUX)
        VTSS_FREE(p);
#elif defined(VTSS_BASICS_STANDALONE)
        free(p);
#else
#  error "Unknown free/alloc configuration"
#endif
    }
};

template <unsigned ID, typename T_>
typename meta::enable_if<is_pod<T_>::value, T_>::type *create() {
#if defined(VTSS_OPSYS_LINUX)
    return (T_ *)VTSS_CALLOC_MODID(ID, 1, sizeof(T_), __FILE__, __LINE__);
#elif defined(VTSS_BASICS_STANDALONE)
    return (T_ *)calloc(1, sizeof(T_));
#else
#  error "Unknown free/alloc configuration"
#endif
}

template <unsigned ID, typename T_, typename... Args>
typename meta::enable_if<!is_pod<T_>::value, T_>::type *create(
        Args &&... args) {
    return static_cast<T_ *>(new ObjectWrapper<ID, T_>(forward<Args>(args)...));
}

template <typename T_>
void destroy(typename meta::enable_if<is_pod<T_>::value, T_>::type *t) {
    if (t) {
#if defined(VTSS_OPSYS_LINUX)
        VTSS_FREE(t);
#elif defined(VTSS_BASICS_STANDALONE)
        free(t);
#else
#  error "Unknown free/alloc configuration"
#endif
    }
}

template <typename T_>
void destroy(typename meta::enable_if<!is_pod<T_>::value, T_>::type *t) {
    if (t) delete t;
}
};  // namespace details_new
#endif

template <unsigned ID, typename T_, typename... Args>
T_ *create(Args &&... args) {
    //return details_new::create<ID, T_>(forward<Args>(args)...);
    return new T_(vtss::forward<Args>(args)...);
}

template <typename T_>
void destroy(T_ *t) {
    //details_new::destroy<T_>(t);
    delete t;
}
};
#endif  //  __VTSS_BASICS_NEW_HXX__
