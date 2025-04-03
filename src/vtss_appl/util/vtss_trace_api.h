/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
 * Introduction to trace module
 * ============================
 * The trace module provides macros for generating printf-trace to be output
 * on - e.g. - a serial console. Each trace statement is categorized by:
 * - module
 * - group within module
 * - trace level
 *
 * For each (module, group) and each thread, a trace level can be
 * configured.
 * A given trace statement is only output if its trace level is
 * - higher than the trace level configured for (module, group).
 * - higher than the trace level configured for the thread
 * - higher than the compile-time minimum trace level.
 *
 * The following trace levels are defined:
 * - ERROR   Code error encountered
 * - WARNING Potential code error, manual inspection required
 * - INFO    Useful information
 * - DEBUG   Some debug information
 * - NOISE   Lot's of debug information
 * - RACKET  Even more ...
 *
 * "RACKET" generates the most output, "ERROR" generates the least amount of
 * output. By default, modules should have trace level set to ERROR for all
 * groups.
 *
 *
 * Registering a module with vtss_trace
 * ------------------------------------
 * In order for a module to use vtss_trace the following is required (here using
 * sw_sprout/vtss_sprout* as example):
 * - In module's header file (vtss_sprout.h) include <vtss_trace_lvl_api.h>
 * - In module's header file (vtss_sprout.h) define
 *    - Constant VTSS_TRACE_MODULE_ID
 *      Usually the same as the module's vtss_module_id_t.
 *    - Constants for each of the module's trace group.
 *      Trace group 0, must be used for default group.
 * - In module's header file (vtss_sprout.h) include <vtss_trace_api.h>
 *
 * - In module's .c file's global scope, define
 *    - trace registration (vtss_trace_reg_t)
 *    - trace groups (vtss_trace_grp_t)
 *    - VTSS_TRACE_REGISTER()
 *
 * Pls. refer to an existing module for detailed code example
 * (e.g. vtss_sprout.h+vtss_sprout.c).
 *
 *
 * Controlling trace level at compile-time
 * ---------------------------------------
 * To control the trace level at compile time, VTSS_TRACE_LVL_MIN must be used.
 * To have all trace included, use the value VTSS_TRACE_LVL_ALL.
 * To have only error trace included, use the value VTSS_TRACE_LVL_ERROR.
 * To have all trace excluded, use the value VTSS_TRACE_LVL_NONE.
 *
 * See vtss_trace_lvl_api.h for the exact numeric values of these constants.
 *
 *
 * Controlling trace level at run-time
 * -----------------------------------
 * Functions are available for controlling the trace level at
 * compile time.
 *
 * Additional each module provides can provide a default trace level in its
 * registration of trace groups. If no such default level is set, ERROR
 * is used.
 *
 *
 * Using trace in module code
 * --------------------------
 * Once a module has been registered with the vtss_trace, the T_... macros
 * below can be used.
 * The simplest type of trace is:
 *   T_D("bla bla");
 * This call will output "bla bla" if trace level for the module's default group
 * is DEBUG or lower.
 * If the trace shall be part of another group than the default group, then
 * macro T_DG must be used.
 * Macros for hex-dumping (e.g. a packet) also exist. See below.
 *
 *
 * trace io
 * --------
 * The trace io functions are defined in vtss_trace_io.
 * By default trace is output with fputs/putchar on stdout.
 * Additional trace output devices can be registered, e.g. for Telnet
 * connections.
 *
 *
 * Dependencies
 * ============
 * vtss_trace requires the following modules:
 * - Switch API (gw_api/...)
 *
 *
 * Compilation Directives
 * ======================
 * The following defines are used to control the compilation of vtss_trace:
 * + VTSS_TRACE_LVL_MIN
 *   The amount of trace included at compile time. By default all trace is
 *   included. Must be set to one of the values defined in vtss_trace_lvl_api.h.
 *   It is recommended always to include error trace.
 */

#ifndef _VTSS_TRACE_API_H_
#define _VTSS_TRACE_API_H_

#include "main_types.h"
#include "vtss_trace_lvl_api.h"
#include "vtss_module_id.h"
#include <main.h>
#include <stdarg.h>
#include <vtss/basics/print_fmt.hxx>
#include <vtss/basics/preprocessor.h>

#ifndef _VTSS_TRACE_LVL_API_H_
#error "vtss_trace_lvl_api.h has not been included prior to including vtss_trace_api.h"
#endif

#ifndef VTSS_TRACE_LVL_MIN
#define VTSS_TRACE_LVL_MIN VTSS_TRACE_LVL_ALL   /* Include all trace by default */
#endif

/*
 * Compile-time trace disabling
 */
#ifndef VTSS_TRACE_LVL_MIN
#define VTSS_TRACE_LVL_MIN 0 /* Default: All trace enabled */
#endif

/* Default trace group */
#define VTSS_TRACE_GRP_DEFAULT 0

#define VTSS_TRACE_MAX_THREAD_ID    64

/* ===========================================================================
 * Trace registration
 * ------------------------------------------------------------------------ */

/*
 * Before using the trace macros, each module must register with vtss_trace.
 * This is done using the vtss_trace_reg_t and vtss_trace_grp_t structures
 * and the VTSS_TRACE_REGISTER() macro:
 * 1) Allocate and initialize vtss_trace_reg_t and vtss_trace_grp_t statically
 *    in the compilation unit's global space.
 * 2) Use VTSS_TRACE_REGISTER() to register - also at the global space.
 *
 * The module may now call the trace macros (T_...).
 */

#define VTSS_TRACE_MAX_NAME_LEN  15
#define VTSS_TRACE_MAX_DESCR_LEN 60

#define VTSS_TRACE_FLAGS_NONE            (0)

/*
 * Include usec time stamp in trace
 */
#define VTSS_TRACE_FLAGS_USEC      (1 <<  0)

/*
 * Output trace into ring buffer, instead of console
 */
#define VTSS_TRACE_FLAGS_RINGBUF   (1 <<  1)

// Internal flag
#define VTSS_TRACE_FLAGS_INIT      (1 << 16)

/* Group definition */
typedef struct {
    char name[VTSS_TRACE_MAX_NAME_LEN+1];   /* Name of group        */
    char descr[VTSS_TRACE_MAX_DESCR_LEN+1]; /* Description of group */

    int  lvl;       /* Default trace level. If 0, ERROR is used          */
    u32  flags;     /* Trace option flags */

    /* ----- Internal fields, not to be changed by application ----- */

    // Trace level prior to previous call to vtss_trace_module_lvl_set()
    int lvl_prv;

    // Default trace level configured by module itself code-wise
    int lvl_def;

    // Default trace flags as configured by module itself code-wise
    u32 flags_def;

} vtss_trace_grp_t;

/* Module registration */
/* See registration procedure above */
typedef struct {
    int  module_id;                         /* Module ID             */
    char name[VTSS_TRACE_MAX_NAME_LEN+1];   /* Name of module        */
    char descr[VTSS_TRACE_MAX_DESCR_LEN+1]; /* Description of module */

    /* ----- Internal fields, not to be changed by application ----- */
    u32  flags;     /* Trace module flags */
    /* Pointer to array of trace groups. */
    int               grp_cnt;
    vtss_trace_grp_t* grps;
} vtss_trace_reg_t;

/* Registration macros */
#define TRACE_MODULE_ID_UNSPECIFIED -2
#define TRACE_GRP_IDX_UNSPECIFIED   -2
#define THREAD_ID_UNSPECIFIED       -2
#define VTSS_TRACE_LVL_UNSPECIFIED  -2
#define USID_UNSPECIFIED            0xff

/* ===========================================================================
 * Trace macros
 *
 * T_E/T_W/T_I/T_D/T_N/T_R:
 * Printf-style trace output for default trace group.
 *
 * T_E_HEX/T_W_HEX/T_I_HEX/T_D_HEX/T_N_HEX/T_R_HEX:
 * Hex dump of any number of bytes, e.g. a packet, for default trace group.
 *
 * T_EG/T_WG/T_IG/T_DG/T_NG/T_RG:
 * Printf-style trace output for specific trace group.
 *
 * T_EG_HEX/T_WG_HEX/T_IG_HEX/T_DG_HEX/T_NG_HEX/T_RG_HEX:
 * Hex dump of any number of bytes, e.g. a packet, for specific trace group.
 * ------------------------------------------------------------------------ */
#define __VTSS_LOCATION__   __FUNCTION__

#ifdef __GNUC__
#define _trace_likely(x)   __builtin_expect((x),1)
#define _trace_unlikely(x) __builtin_expect((x),0)
#else
#define _trace_likely(x)   (x)
#define _trace_unlikely(x) (x)
#endif

#define TRACE_IS_ENABLED(_m, _grp, _lvl) _trace_unlikely(_lvl >= T_LVL_GET(_m, _grp))

namespace {
class conststr {
    const char *p;
    std::size_t sz;
 public:
    template <std::size_t N>
    constexpr conststr(const char (&a)[N]) : p(a), sz(N-1) {}

    constexpr char operator[](std::size_t n) const
    {
        return p[n];
    }
    constexpr std::size_t size() const { return sz; }
 };

constexpr std::size_t count_conversions(conststr s) {
    std::size_t res = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && s[i+1] == '%') {
            i += 2;
            continue;
        }
        if (s[i] == '%' && s[i+1] == '*') {
            res += 2;
            i += 2;
            continue;
        }
        if (s[i] == '%') {
            ++res;
        }
    }
    return res;
}
}

/* Main macros */
#define T(_grp, _lvl, _fmt, ...)                                        \
    do {                                                                \
        static_assert(count_conversions(_fmt) == 0 ? PP_HAS_ARGS(__VA_ARGS__) == false :\
            count_conversions(_fmt) == PP_TUPLE_ARGV_CNT(__VA_ARGS__), "Wrong ARGS"); \
        if(TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, _grp, _lvl))          \
            vtss_trace_var_printf(VTSS_TRACE_MODULE_ID, _grp, _lvl,     \
                                  __VTSS_LOCATION__, __LINE__,          \
                                  _fmt, ##__VA_ARGS__);                 \
    } while(0)

#define T_MOD(_mod, _grp, _lvl, _fmt, ...)                              \
    do {                                                                \
        static_assert(count_conversions(_fmt) == 0 ? PP_HAS_ARGS(__VA_ARGS__) == false :\
            count_conversions(_fmt) == PP_TUPLE_ARGV_CNT(__VA_ARGS__), "Wrong ARGS"); \
        if(TRACE_IS_ENABLED(_mod, _grp, _lvl))                          \
            vtss_trace_var_printf(_mod, _grp, _lvl,                     \
                                  __VTSS_LOCATION__, __LINE__,          \
                                  _fmt, ##__VA_ARGS__);                 \
    } while(0)


// To be used ONLY when the format is NOT a string literal
// otherwise use T_X
#define T_UNSAFE(_grp, _lvl, _fmt, ...)                                 \
    do {                                                                \
        if(TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, _grp, _lvl))          \
            vtss_trace_var_printf(VTSS_TRACE_MODULE_ID, _grp, _lvl,     \
                                  __VTSS_LOCATION__, __LINE__,          \
                                  _fmt, ##__VA_ARGS__);                 \
    } while(0)

#define T_HEX(_grp, _lvl, _byte, _cnt)                                  \
    do {                                                                \
        if(TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, _grp, _lvl))          \
            vtss_trace_hex_dump(VTSS_TRACE_MODULE_ID, _grp, _lvl,       \
                                __VTSS_LOCATION__, __LINE__,            \
                                _byte, _cnt);                           \
    } while(0)

// Pre-declare this, because inclusion of "icli_porting_util.h" fails
extern "C" char *icli_port_info_txt_short(vtss_usid_t usid, mesa_port_no_t uport, char *str_buf_p);

// Macro for only printing out trace for ports that has trace enabled
#define T_PORT(_p, _grp, _lvl, _fmt, ...)                                                    \
    do {                                                                                     \
        if(TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, _grp, _lvl) && !vtss_trace_port_get(_p)) { \
            char str[300], if_str[40];                                                       \
            (void)icli_port_info_txt_short(VTSS_USID_START, _p + 1, if_str);                 \
            int port_len = snprintf(str, sizeof(str), "%s (%u): ", if_str, _p + 1);          \
            strncpy(str + port_len, _fmt, sizeof(str) - port_len);                           \
            vtss_trace_var_printf(VTSS_TRACE_MODULE_ID, _grp, _lvl,                          \
                                  __VTSS_LOCATION__, __LINE__,                               \
                                  str, ##__VA_ARGS__);                                       \
        }                                                                                    \
    } while(0)

/* ERROR level trace macros */
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_ERROR)
#define T_E(_fmt, ...)                    T_EG(VTSS_TRACE_GRP_DEFAULT, _fmt, ##__VA_ARGS__)
#define T_E_PORT(_port, _fmt, ...)        T_EG_PORT(VTSS_TRACE_GRP_DEFAULT, _port, _fmt, ##__VA_ARGS__)
#define T_EG(_grp, _fmt, ...)             T(_grp, VTSS_TRACE_LVL_ERROR, _fmt, ##__VA_ARGS__)
#define T_EG_PORT(_grp, _port, _fmt, ...) T_PORT(_port, _grp, VTSS_TRACE_LVL_ERROR, _fmt, ##__VA_ARGS__)
#define T_E_HEX(byte_p, byte_cnt)         T_EG_HEX(VTSS_TRACE_GRP_DEFAULT, byte_p, byte_cnt)
#define T_EG_HEX(_grp, byte_p, byte_cnt)  T_HEX(_grp, VTSS_TRACE_LVL_ERROR, byte_p, byte_cnt)
#else
#define T_E(_fmt, ...)
#define T_E_PORT(_port, _fmt, ...)
#define T_EG(_grp, _fmt, ...)
#define T_WG_PORT(_grp, _fmt, ...)
#define T_E_HEX(byte_p, byte_cnt)
#define T_EG_HEX(_grp, byte_p, byte_cnt)
#endif

/* WARNING level trace macros */
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_WARNING)
#define T_W(_fmt, ...)                    T_WG(VTSS_TRACE_GRP_DEFAULT, _fmt, ##__VA_ARGS__)
#define T_W_PORT(_port, _fmt, ...)        T_WG_PORT(VTSS_TRACE_GRP_DEFAULT, _port, _fmt, ##__VA_ARGS__)
#define T_WG(_grp, _fmt, ...)             T(_grp, VTSS_TRACE_LVL_WARNING, _fmt, ##__VA_ARGS__)
#define T_WG_PORT(_grp, _port, _fmt, ...) T_PORT(_port, _grp, VTSS_TRACE_LVL_WARNING, _fmt, ##__VA_ARGS__)
#define T_W_HEX(byte_p, byte_cnt)         T_WG_HEX(VTSS_TRACE_GRP_DEFAULT, byte_p, byte_cnt)
#define T_WG_HEX(_grp, byte_p, byte_cnt)  T_HEX(_grp, VTSS_TRACE_LVL_WARNING, byte_p, byte_cnt)
#else
#define T_W(_fmt, ...)
#define T_W_PORT(_port, _fmt, ...)
#define T_WG(_grp, _fmt, ...)
#define T_WG_PORT(_grp, _fmt, ...)
#define T_W_HEX(byte_p, byte_cnt)
#define T_WG_HEX(_grp, byte_p, byte_cnt)
#endif

/* INFO level trace macros */
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
#define T_I(_fmt, ...)                    T_IG(VTSS_TRACE_GRP_DEFAULT, _fmt, ##__VA_ARGS__)
#define T_I_PORT(_port, _fmt, ...)        T_IG_PORT(VTSS_TRACE_GRP_DEFAULT, _port, _fmt, ##__VA_ARGS__)
#define T_IG(_grp, _fmt, ...)             T(_grp, VTSS_TRACE_LVL_INFO, _fmt, ##__VA_ARGS__)
#define T_IG_PORT(_grp, _port, _fmt, ...) T_PORT(_port, _grp, VTSS_TRACE_LVL_INFO, _fmt, ##__VA_ARGS__)
#define T_I_HEX(byte_p, byte_cnt)         T_IG_HEX(VTSS_TRACE_GRP_DEFAULT, byte_p, byte_cnt)
#define T_IG_HEX(_grp, byte_p, byte_cnt)  T_HEX(_grp, VTSS_TRACE_LVL_INFO, byte_p, byte_cnt)
#else
#define T_I(_fmt, ...)
#define T_I_PORT(_port, _fmt, ...)
#define T_IG(_grp, _fmt, ...)
#define T_IG_PORT(_grp, _port, _fmt, ...)
#define T_I_HEX(byte_p, byte_cnt)
#define T_IG_HEX(_grp, byte_p, byte_cnt)
#endif

/* DEBUG level trace macros */
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
#define T_D(_fmt, ...)                    T_DG(VTSS_TRACE_GRP_DEFAULT, _fmt, ##__VA_ARGS__)
#define T_D_PORT(_port, _fmt, ...)        T_DG_PORT(VTSS_TRACE_GRP_DEFAULT, _port, _fmt, ##__VA_ARGS__)
#define T_DG(_grp, _fmt, ...)             T(_grp, VTSS_TRACE_LVL_DEBUG, _fmt, ##__VA_ARGS__)
#define T_DG_PORT(_grp, _port, _fmt, ...) T_PORT(_port, _grp, VTSS_TRACE_LVL_DEBUG, _fmt, ##__VA_ARGS__)
#define T_D_HEX(byte_p, byte_cnt)         T_DG_HEX(VTSS_TRACE_GRP_DEFAULT, byte_p, byte_cnt)
#define T_DG_HEX(_grp, byte_p, byte_cnt)  T_HEX(_grp, VTSS_TRACE_LVL_DEBUG, byte_p, byte_cnt)
#else
#define T_D(_fmt, ...)
#define T_D_PORT(_port, _fmt, ...)
#define T_DG(_grp, _fmt, ...)
#define T_DG_PORT(_port, _grp, _fmt, ...)
#define T_D_HEX(byte_p, byte_cnt)
#define T_DG_HEX(_grp, byte_p, byte_cnt)
#endif

/* NOISE level trace macros */
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_NOISE)
#define T_N(_fmt, ...)                    T_NG(VTSS_TRACE_GRP_DEFAULT, _fmt, ##__VA_ARGS__)
#define T_N_PORT(_port, _fmt, ...)        T_NG_PORT(VTSS_TRACE_GRP_DEFAULT, _port, _fmt, ##__VA_ARGS__)
#define T_NG(_grp, _fmt, ...)             T(_grp, VTSS_TRACE_LVL_NOISE, _fmt, ##__VA_ARGS__)
#define T_NG_PORT(_grp, _port, _fmt, ...) T_PORT(_port, _grp, VTSS_TRACE_LVL_NOISE, _fmt, ##__VA_ARGS__)
#define T_N_HEX(byte_p, byte_cnt)         T_NG_HEX(VTSS_TRACE_GRP_DEFAULT, byte_p, byte_cnt)
#define T_NG_HEX(_grp, byte_p, byte_cnt)  T_HEX(_grp, VTSS_TRACE_LVL_NOISE, byte_p, byte_cnt)
#else
#define T_N(_fmt, ...)
#define T_N_PORT(_port, _fmt, ...)
#define T_NG(_grp, _fmt, ...)
#define T_NG_PORT(_grp, _port, _fmt, ...)
#define T_N_HEX(byte_p, byte_cnt)
#define T_NG_HEX(_grp, byte_p, byte_cnt)
#endif

/* RACKET level trace macros */
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_RACKET)
#define T_R(_fmt, ...)                    T_RG(VTSS_TRACE_GRP_DEFAULT, _fmt, ##__VA_ARGS__)
#define T_R_PORT(_port, _fmt, ...)        T_RG_PORT(VTSS_TRACE_GRP_DEFAULT, _port, _fmt, ##__VA_ARGS__)
#define T_RG(_grp, _fmt, ...)             T(_grp, VTSS_TRACE_LVL_RACKET, _fmt, ##__VA_ARGS__)
#define T_RG_PORT(_grp, _port, _fmt, ...) T_PORT(_port, _grp, VTSS_TRACE_LVL_RACKET, _fmt, ##__VA_ARGS__)
#define T_R_HEX(byte_p, byte_cnt)         T_RG_HEX(VTSS_TRACE_GRP_DEFAULT, byte_p, byte_cnt)
#define T_RG_HEX(_grp, byte_p, byte_cnt)  T_HEX(_grp, VTSS_TRACE_LVL_RACKET, byte_p, byte_cnt)
#else
#define T_R(_fmt, ...)
#define T_R_PORT(_port, _fmt, ...)
#define T_RG(_grp, _fmt, ...)
#define T_RG_PORT(_grp, _port, _fmt, ...)
#define T_R_HEX(byte_p, byte_cnt)
#define T_RG_HEX(_grp, byte_p, byte_cnt)
#endif

/* Macro for checking whether trace is enabled for (module, grp, lvl) */
#if (VTSS_TRACE_LVL_MIN < VTSS_TRACE_LVL_NONE)
#define T_LVL_GET(module_id, grp_idx) (vtss_trace_global_module_lvl_get(module_id, grp_idx))
#else
#define T_LVL_GET(module_id, grp_idx) (VTSS_TRACE_LVL_NONE)
#endif

/* Trace macros with module, group and level as argument */
#if (VTSS_TRACE_LVL_MIN < VTSS_TRACE_LVL_NONE)
#define T_EXPLICIT(_m, _grp, _lvl, _f, _l, _fmt, ...)                   \
    do {                                                                \
        if(TRACE_IS_ENABLED(_m, _grp, _lvl))                            \
            vtss_trace_var_printf(_m, _grp, _lvl, _f, _l,               \
                                  _fmt, ##__VA_ARGS__);                 \
    } while(0)
#define T_HEX_EXPLICIT(_m, _grp, _lvl, _f, _l, _p, _c)                  \
    do {                                                                \
        if(TRACE_IS_ENABLED(_m, _grp, _lvl))                            \
            vtss_trace_hex_dump(_m, _grp, _lvl, _f, _l, _p, _c);        \
    } while(0)
#else
#define T_EXPLICIT(_m, _grp, _lvl, _f, _l, _fmt, ...)  /* Go Away */
#define T_HEX_EXPLICIT(_m, _grp, _lvl, _f, _l, _p, _c) /* Go Away */
#endif

/* ===========================================================================
 * Run-time trace level control
 *
 * These functions can be used to implement CLI-based trace control
 * ------------------------------------------------------------------------ */

/*
 * Set the trace hunt target.
 */
void vtss_trace_hunt_set(char *target);

/*
 * Set trace level for module/group
 *
 * module_id/grp_idx may be set to -1 for wildcarding.
 */
void vtss_trace_module_lvl_set( int module_id, int grp_idx, int lvl);

/*
 * Limit trace for thread. -1 = all threads.
 */
void vtss_trace_thread_lvl_set(int thread_id, int lvl);

/*
 * Revert to the previous trace level settings
 */
void vtss_trace_lvl_revert(void);

/*
 * Get/set trace level for all modules/threads
 */
int  vtss_trace_global_lvl_get(void);
void vtss_trace_global_lvl_set(int lvl);

/* ======================================================================== */

/* ===========================================================================
 * Run-time trace format control
 *
 * These functions can be used to implement CLI-based trace control
 * ------------------------------------------------------------------------ */

/*
 * Enable usec/ringbuf for module/group.
 */
typedef enum {
    VTSS_TRACE_MODULE_PARM_USEC,
    VTSS_TRACE_MODULE_PARM_RINGBUF
} vtss_trace_module_parm_t;

void vtss_trace_module_parm_set(vtss_trace_module_parm_t parm, int module_id, int grp_idx, BOOL enable);

/* ===========================================================================
 * Utility functions
 * ------------------------------------------------------------------------ */

void vtss_trace_port_set(int port_index,BOOL trace_disabled);
BOOL vtss_trace_port_get(int port_index);

/* Trace settings for thread */
typedef struct {
    /* Limit trace for thread (default: all trace enabled) */
    int  lvl;

    /* ----- Internal fields, not to be changed by application ----- */
    int  lvl_prv;   /* Trace level prior to previous call to
                       vtss_trace_thread_lvl_set */
} vtss_trace_thread_t;

/*
 * Get trace level for (module, group)
 */
int vtss_trace_module_lvl_get(int module_id, int grp_idx);

/*
 * Get trace level for (global, module, group)
 */
int vtss_trace_global_module_lvl_get(int module_id, int grp_idx);

/*
 * Get name of next module or group with id/idx greater than the specified
 * value.
 *
 * NULL pointer is returned when no more modules/groups.
 * To get all names, start by requesting id/idx -1 and keep requesting until
 * NULL is returned.
 * *module_id_p / *grp_idx_p is updated to correspond to the returned name.
 */
const char *vtss_trace_module_name_get_next(int *module_id_p);
const char *vtss_trace_grp_name_get_next(int module_id, int *grp_idx_p);

/* Get module/group description */
const char *vtss_trace_module_dscr_get(int module_id);
const char *vtss_trace_grp_dscr_get(int module_id, int grp_idx);

/* Get parm setting for group */
BOOL vtss_trace_grp_parm_get(vtss_trace_module_parm_t parm, int module_id, int grp_idx);

/*
 * Convert module/group name to module_id/grp_idx
 * Name only needs to be unique, not complete.
 *
 * Returns MESA_RC_OK if id/idx was found.
 */
mesa_rc vtss_trace_module_name_to_id(const char *name, int *module_id_p);
mesa_rc vtss_trace_grp_name_to_id   (const char *name, int  module_id, int *grp_idx_p);
mesa_rc vtss_trace_lvl_to_val(       const char *name, int *level_p);

const char *vtss_trace_lvl_to_str(int lvl);

/* ======================================================================== */

/* ===========================================================================
 * Ring buffer
 * ------------------------------------------------------------------------ */

void vtss_trace_rb_output(int (*print_function)(const char *fmt, ...));
void vtss_trace_rb_output_as_one_str(i32 (*print_function)(const char *str));
void vtss_trace_rb_flush(void);

/*
 * Enable/disable ring buffer (enabled at startup)
 * If disabled, any trace destined for ring buffer is discarded.
 */
void vtss_trace_rb_ena(BOOL ena);

/* ======================================================================== */

/* ===========================================================================
 * Additional IO
 *
 * By default serial port is the only output device for trace macros.
 * Additional IO devices can be registered using below functions.
 * Such IO devices could - e.g. - be telnet connections.
 * ------------------------------------------------------------------------ */

/* IO layer */
typedef struct _vtss_trace_io_t {
    void (*trace_putchar)          (struct _vtss_trace_io_t *pIO, char ch);
    int  (*trace_vprintf)          (struct _vtss_trace_io_t *pIO, const char *fmt, va_list ap);
    void (*trace_write_string)     (struct _vtss_trace_io_t *pIO, const char *str);
    void (*trace_write_string_len) (struct _vtss_trace_io_t *pIO, const char *str, unsigned length);
    void (*trace_flush)            (struct _vtss_trace_io_t *pIO);
} vtss_trace_io_t;

/*
 * Registration function.
 * trace_io_t must NOT be allocated on stack.
 *
 * io_reg_id is returned.
 * To be used if later calling vtss_trace_io_unregister().
 */
mesa_rc vtss_trace_io_register(
    vtss_trace_io_t   *io,
    vtss_module_id_t  module_id,
    uint              *io_reg_id);

/*
 * Unregistration function
 */
mesa_rc vtss_trace_io_unregister(
    uint *io_reg_id);

/* ===========================================================================
 * Internal definitions - do NOT use outside trace module
 * ------------------------------------------------------------------------ */

/* Do not use these functions - use the wrappers above */
void vtss_trace_var_printf_impl(int module_id,
                                int grp_idx,
                                int lvl,
                                const char *location,
                                uint line_no,
                                const char *data,
                                size_t len);

template <typename... TS>
void vtss_trace_var_printf(int module_id,
                           int grp_idx,
                           int lvl,
                           const char *location,
                           uint line_no,
                           const char *fmt,
                           TS&& ... args) {
    char buf[4096];
    size_t len = vtss::print_fmt(buf, 4096, fmt, args...);
    vtss_trace_var_printf_impl(module_id, grp_idx, lvl, location, line_no, buf, len);
}

void vtss_trace_printf(int module_id,
                       int grp_idx,
                       int lvl,
                       const char *location,
                       uint line_no,
                       const char *fmt,
                       ...)
__attribute__ ((format (printf, 6, 7)));

void vtss_trace_vprintf(int module_id,
                        int grp_idx,
                        int lvl,
                        const char *location,
                        uint line_no,
                        const char *fmt,
                        va_list args);

void vtss_trace_hex_dump(int module_id,
                         int grp_idx,
                         int lvl,
                         const char *location,
                         uint line_no,
                         const uchar *byte_p,
                         int  byte_cnt);

/* ======================================================================== */

// Needed for other trace modules to interface
const vtss_trace_grp_t *vtss_trace_grp_get(int module_id, int grp_idx);
const vtss_trace_reg_t *vtss_trace_reg_get(int module_id, int grp_idx);

/* ===========================================================================
 * Special macros, only intended for use in Switch API
 * ------------------------------------------------------------------------ */
#define T_EXPLICIT_NO_VA(_m, _grp, _lvl, _f, _l, _msg)                  \
    do {                                                                \
        if(TRACE_IS_ENABLED(_m, _grp, _lvl))                            \
            vtss_trace_printf(_m, _grp, _lvl, _f, _l,                   \
                              "%s", _msg);                              \
    } while(0)
/* ======================================================================== */

struct TraceRegister {
    TraceRegister(vtss_trace_reg_t *trace_reg_p, vtss_trace_grp_t *trace_grp_p, int grp_cnt);
    TraceRegister() = delete;
};

// The following two functions should only be called in case static
// initialization with the VTSS_TRACE_REGISTER macro is impossible.
void vtss_trace_reg_init(vtss_trace_reg_t* trace_reg_p, vtss_trace_grp_t* trace_grp_p, int grp_cnt);
void vtss_trace_register(vtss_trace_reg_t* trace_reg_p);

// The "__attribute__((init_priority))" part makes sure to initialize this
// object prior to objects that doesn't have such an attribute. A lower value
// causes sooner initialization than functions/constructors with higher values.
// Values below 101 are for compiler internal use.
#define VTSS_TRACE_REGISTER(_r_, _g_) static TraceRegister __attribute__((init_priority (1000))) __trace_##_g_(_r_, _g_, ARRSZ(_g_))

#ifdef __cplusplus
extern "C" {
#endif

// ICLI/Internal functions
/* Initialize module. Only to be called for managed build. */
/* The init function uses type int for argument cmd, since including
 * main.h will cause compilation problems, due to inclusion of trace_api.h
 * in switch API */
mesa_rc vtss_trace_init(vtss_init_data_t *data);

/* Write trace configuration to flash */
mesa_rc vtss_trace_cfg_wr(void);

/* Read trace configuration from flash */
mesa_rc vtss_trace_cfg_rd(void);

/* Erase trace configuration from flash */
mesa_rc vtss_trace_cfg_erase(void);

/* Restore trace settings to what each module has coded */
mesa_rc vtss_trace_defaults(void);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_TRACE_API_H_ */

