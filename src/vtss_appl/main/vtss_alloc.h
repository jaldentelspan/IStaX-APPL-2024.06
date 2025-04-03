/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_ALLOC_H_
#define _VTSS_ALLOC_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <vtss/appl/module_id.h>

#if defined(VTSS_FEATURE_HEAP_WRAPPERS)
void *vtss_malloc(vtss_module_id_t modid, size_t sz, const char *const file, const int line);
void *vtss_calloc(vtss_module_id_t modid, size_t nm, size_t sz, const char *const file, const int line);
void *vtss_realloc(vtss_module_id_t modid, void *ptr, size_t sz, const char *const file, const int line);
char *vtss_strdup(vtss_module_id_t modid, const char *str, const char *const file, const int line);
void vtss_free(void *ptr, const char *const file, const int line);

// Per module ID dynamic memory usage statistics
typedef struct {
    uint32_t usage;   // Current usage [bytes]
    uint32_t max;     // Maximum seen usage [bytes]
    uint64_t total;   // Accummulated usage [bytes]
    uint32_t allocs;  // Number of allocations
    uint32_t frees;   // Number of frees
} heap_usage_t;

#define VTSS_MALLOC_MODID(_m_, _s_, _f_, _l_)       vtss_malloc(_m_, _s_, _f_, _l_)
#define VTSS_CALLOC_MODID(_m_, _n_, _s_, _f_, _l_)  vtss_calloc(_m_, _n_, _s_, _f_, _l_)
#define VTSS_REALLOC_MODID(_m_, _p_, _s_, _f_, _l_) vtss_realloc(_m_, _p_, _s_, _f_, _l_)
#define VTSS_STRDUP_MODID(_m_, _s_, _f_, _l_)       vtss_strdup(_m_, _s_, _f_, _l_)
#define VTSS_FREE(_p_)                              vtss_free(_p_, __FILE__, __LINE__)

#else

#define VTSS_MALLOC_MODID(_m_, _s_, _f_, _l_)       malloc(_s_)
#define VTSS_CALLOC_MODID(_m_, _n_, _s_, _f_, _l_)  calloc(_n_, _s_)
#define VTSS_REALLOC_MODID(_m_, _p_, _s_, _f_, _l_) realloc(_p_, _s_)
#define VTSS_STRDUP_MODID(_m_, _s_, _f_, _l_)       strdup(_s_)
#define VTSS_FREE(_p_)                              free(_p_)
#endif /* (!)defined(VTSS_FEATURE_HEAP_WRAPPERS) */

#endif /* _VTSS_ALLOC_H_ */

