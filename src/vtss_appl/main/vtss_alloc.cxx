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

#if defined(VTSS_FEATURE_HEAP_WRAPPERS)
#include <vtss_module_id.h>
#include "vtss_os_wrapper.h"
#include "vtss_alloc.h"
#include <vtss/basics/notifications/lock-global-subject.hxx>
#include "main_trace.h"

static bool alloc_trace_init_done;

// Layout of overlaid memory for memory check functionality:
//
// u32_user_ptr[-2] = magic #1 (3 bytes) bitwise ORed with (modid << 24)
// u32_user_ptr[-1] = User-requested size (sz)
// sz bytes of user memory
// u8_user_ptr[sz]  = magic #2 (1 byte, for simplicity, or we would have to R/W a byte at a time due to alignment problems). To test memory overwrites at the end of the allocation.

#define VTSS_MEMALLOC_MAGIC_1          0xC0FFEEU /* 3 bytes */
#define VTSS_MEMALLOC_MAGIC_2          0xBAU     /* 1 byte  */
#define VTSS_MEMALLOC_ADDITIONAL_BYTES 9         /* We need 2 * 4 + 1 bytes for this */

heap_usage_t heap_usage_cur[VTSS_MODULE_ID_NONE + 1];
u32          heap_usage_tot_max_cur, heap_usage_tot_cur;

static void vtss_memalloc_check_modid(const char *caller, vtss_module_id_t *modid)
{
    if (*modid < 0 || *modid > VTSS_MODULE_ID_NONE) {
        // Allow VTSS_MODULE_ID_NONE for code that doesn't have a module ID.
        T_EG(MAIN_TRACE_GRP_ALLOC, "%s: Invalid module id (%d). Using VTSS_MODULE_ID_NONE", caller, *modid);
        *modid = VTSS_MODULE_ID_NONE;
    }
}

static void vtss_set_free(void *ptr, vtss_module_id_t check_against_modid, const char *const file, const int line)
{
    vtss_module_id_t modid;
    u32              *p, magics[2];
    BOOL             modid_ok, magics_ok;
    size_t           sz;

#if VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG
    u32              new_usage = 0, new_alive = 0;
#endif

    if (!ptr) {
        return;
    }

    p         = (u32 *)ptr - 2;
    modid     = (p[0] >> 24) & 0xFF;
    sz        = p[1];
    modid_ok  = modid >= 0 && modid <= VTSS_MODULE_ID_NONE;
    magics[0] = p[0] & 0xFFFFFF;
    magics[1] = ((u8 *)ptr)[sz];
    magics_ok = magics[0] == VTSS_MEMALLOC_MAGIC_1 && magics[1] == VTSS_MEMALLOC_MAGIC_2;

    if (!magics_ok) {
        T_EG(MAIN_TRACE_GRP_ALLOC, "%s#%d: Memory corruption at %p. One or more invalid magics: Got <0x%x, 0x%x>, expected <0x%x, 0x%x>. Module = %d = %s", file, line, ptr, magics[0], magics[1], VTSS_MEMALLOC_MAGIC_1, VTSS_MEMALLOC_MAGIC_2, modid, modid_ok ? vtss_module_names[modid] : "");
    }

    if (!modid_ok) {
        T_EG(MAIN_TRACE_GRP_ALLOC, "%s#%d: Memory corruption at %p. Invalid module ID (%d)", file, line, ptr, modid);
    }

    if (check_against_modid != -1 && check_against_modid != modid) {
        T_EG(MAIN_TRACE_GRP_ALLOC, "%s#%d: realloc(%p) called with a module id (%d = %s) different from what alloc originally was called with (%d = %s)", file, line, ptr, check_against_modid, vtss_module_names[check_against_modid], modid, modid_ok ? vtss_module_names[modid] : "<Unknown>");
    }

    if (magics_ok && modid_ok) {
        BOOL m_sz_ok = TRUE, tot_sz_ok = TRUE;
        u32  m_mem_in_use = 0, tot_mem_in_use = 0; // Initialize to satisfy Lint
        heap_usage_t *m = &heap_usage_cur[modid];

        {
            // Use the lazy-initialized leaf-mutex to protect ourselves.
            vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);

            // Module memory consumption updates
            m->frees++;
            if (m->usage < sz) {
                m_mem_in_use = m->usage;
                m_sz_ok = FALSE;
            } else {
                m->usage -= sz;
            }

#if VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG
            new_usage = m->usage;
            new_alive = m->allocs - m->frees;
#endif

            // Total memory consumption updates
            if (heap_usage_tot_cur < sz) {
                tot_mem_in_use = heap_usage_tot_cur;
                tot_sz_ok = FALSE;
            } else {
                heap_usage_tot_cur -= sz;
            }
        }

        if (!m_sz_ok) {
            T_EG(MAIN_TRACE_GRP_ALLOC, "%s:%d: Something fishy going on (%p). Module %d = %s hasn't allocated " VPRIz " bytes. Only %u bytes still unfreed", file, line, ptr, modid, vtss_module_names[modid], sz, m_mem_in_use);
        }

        if (!tot_sz_ok) {
            T_EG(MAIN_TRACE_GRP_ALLOC, "%s:%d: Something fishy going on (%p). Total memory currently allocated = %u bytes. Module %d = %s attempted to deallocate " VPRIz " bytes", file, line, ptr, tot_mem_in_use, modid, vtss_module_names[modid], sz);
        }
    }

    if (alloc_trace_init_done) {
        T_DG(MAIN_TRACE_GRP_ALLOC, "Module %s: free(%zu) bytes @ %p from %s#%d. New usage = %u bytes, new alive ptrs = %u", vtss_module_names[modid], sz, ptr, file, line, new_usage, new_alive);
    }

    p[0] &= 0xFF000000; // Clear magic only, so that we can detect double-freeing and still produce valuable module information.
}

void vtss_free(void *ptr, const char *const file, const int line)
{
    if (!ptr) {
        return;
    }

    vtss_set_free(ptr, -1, file, line);
    free((u32 *)ptr - 2);
}

static void *vtss_do_alloc(vtss_module_id_t modid, size_t sz, void *ptr, const char *caller, const char *const file, const int line)
{
    u32 *p;

#if VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG
    u32 new_usage = 0, new_alive = 0;
#endif

    vtss_memalloc_check_modid(caller, &modid);

    if (ptr) {
        p = (u32 *)realloc((u32 *)ptr - 2, sz + VTSS_MEMALLOC_ADDITIONAL_BYTES);
    } else {
        p = (u32 *)malloc(sz + VTSS_MEMALLOC_ADDITIONAL_BYTES);
    }

    if (p != NULL) {
        heap_usage_t *m = &heap_usage_cur[modid];

        {
            // Use the lazy-initialized leaf-mutex to protect ourselves.
            vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);

            // Module memory consumption updates
            m->usage += sz;
            if (m->usage > m->max) {
                m->max = m->usage;
            }

            m->allocs++;
            m->total += sz;

#if VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG
            new_usage = m->usage;
            new_alive = m->allocs - m->frees;
#endif

            // Total memory consumption updates
            heap_usage_tot_cur += sz;
            if (heap_usage_tot_cur > heap_usage_tot_max_cur) {
                heap_usage_tot_max_cur = heap_usage_tot_cur;
            }
        }

        p[0] = ((VTSS_MEMALLOC_MAGIC_1 & 0xFFFFFF) << 0) | (modid & 0xFF) << 24;
        p[1] = sz;
        ((u8 *)p)[2 * sizeof(u32) + sz] = VTSS_MEMALLOC_MAGIC_2;

        if (alloc_trace_init_done) {
            T_DG(MAIN_TRACE_GRP_ALLOC, "Module %s: %s(%zu bytes) @ %p from %s#%d. New usage = %u bytes, new alive ptrs = %u", vtss_module_names[modid], caller, sz, p + 2, file, line, new_usage, new_alive);
        }

        return p + 2;
    }

    return NULL;
}

void *vtss_malloc(vtss_module_id_t modid, size_t sz, const char *const file, const int line)
{
    return vtss_do_alloc(modid, sz, NULL, "malloc", file, line);
}

void *vtss_calloc(vtss_module_id_t modid, size_t nm, size_t sz, const char *const file, const int line)
{
    // We have to implement our own version of calloc() because
    // we cannot just request another X bytes.
    size_t real_size = nm * sz;
    void   *ptr;

    ptr = vtss_do_alloc(modid, real_size, NULL, "calloc", file, line);

    if (!ptr) {
        return NULL;
    }

    memset(ptr, 0, real_size);
    return ptr;
}

void *vtss_realloc(vtss_module_id_t modid, void *ptr, size_t sz, const char *const file, const int line)
{
    vtss_set_free(ptr, modid, file, line);

    return vtss_do_alloc(modid, sz, ptr, "realloc", file, line);
}

char *vtss_strdup(vtss_module_id_t modid, const char *str, const char *const file, const int line)
{
    char *x = (char *)vtss_malloc(modid, strlen(str) + 1, file, line);

    if (x != NULL) {
        strcpy(x, str);
    }

    return x;
}

/******************************************************************************/
// vtss_alloc_init()
/******************************************************************************/
mesa_rc vtss_alloc_init(vtss_init_data_t *data)
{

    if (data->cmd == INIT_CMD_EARLY_INIT) {
        // We can't use trace until it's initialized. This is the hen and the
        // egg problem: The first module that registers in the trace module,
        // causes the trace module itself to register, followed by load of trace
        // configuration from a file. This loading requires memory,
        // so it calls vtss_malloc(), which utilizes the trace module to print
        // some info. We can't do that until main.cxx has registered its trace.
        alloc_trace_init_done = true;
    }

    return MESA_RC_OK;
}

#endif /* VTSS_FEATURE_HEAP_WRAPPERS */

