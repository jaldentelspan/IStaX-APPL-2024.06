/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "main.h"
#include "vtss_trace.h"
#include "vtss_trace_api.h"
#include "vtss/basics/trace.hxx"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/print_fmt.hxx"
#include "vtss/basics/map.hxx"
#include "vtss/basics/fd.hxx"
#include "vtss/basics/json/stream-parser.hxx"
#include "backtrace.hxx"
#include "crashhandler.hxx"
#include "vtss_os_wrapper.h"

#ifdef VTSS_SW_OPTION_SYSUTIL
#include <sysutil_api.h>
#endif

#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
#include "daylight_saving_api.h"
#endif

#if defined(VTSS_SW_OPTION_SYSLOG)
#include <syslog_api.h>
#endif

#include "misc_api.h"
#include "led_api.h"

// This defines a function which forces text to the console independent of context.
#define NONBLOCKING_PRINTF printf

// Printing of errors if no module is defined, else we will do recursive calls to vtss_trace_printf which makes the system hang.
#define PRINTE(args) { NONBLOCKING_PRINTF("\nERROR: %s#%d: ", __FUNCTION__, __LINE__); NONBLOCKING_PRINTF args; NONBLOCKING_PRINTF("\n"); }

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_TRACE

// Trace for the trace module itself
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "trace", "Trace module."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_NONE
    },
};

/* A heap-allocated string for comparing with running trace messages. */
static char *vtss_trace_hunt_target = NULL;

static BOOL trace_init_done;
static BOOL api_and_led_crits_created;

/* Max number of module index (inherited from module_id.h) */
#define MODULE_ID_MAX VTSS_MODULE_ID_NONE

/* Array with pointers to registrations */
static vtss_trace_reg_t *trace_regs[MODULE_ID_MAX + 1];

/* Semaphore for IO registrations */
vtss_sem_t trace_io_crit;

/* Semaphore for ring buffer access. */
vtss_sem_t trace_rb_crit;

BOOL vtss_trace_port_list[100]; // List that enables/disables trace per port for the T_*_PORT commands. *MUST* be global.

const vtss_trace_thread_t trace_thread_default = {
    .lvl      = VTSS_TRACE_LVL_RACKET,
    .lvl_prv  = VTSS_TRACE_LVL_RACKET,
};

static vtss_trace_thread_t trace_threads[VTSS_TRACE_MAX_THREAD_ID + 1];

// Global level for all modules and threads
static int global_lvl = VTSS_TRACE_LVL_RACKET;

/******************************************************************************/
// grp_trace_printf()
/******************************************************************************/
template <typename... TS>
static int grp_trace_printf(const vtss_trace_grp_t *const trace_grp_p,
                            char *const err_buf,
                            const char *fmt, TS &&... args)
{
    vtss::StringStream stream;
    vtss::print_fmt(stream, fmt, args...);
    if (HAS_FLAGS(trace_grp_p, VTSS_TRACE_FLAGS_RINGBUF)) {
        trace_rb_write_string(err_buf, stream.cstring());
    } else {
        trace_write_string(err_buf, stream.cstring());
    }

    return stream.end() - stream.begin();
}

/******************************************************************************/
// grp_trace_printf()
/******************************************************************************/
static int grp_trace_printf(const vtss_trace_grp_t *const trace_grp_p,
                            char *const err_buf,
                            const char *data, size_t len)
{
    if (HAS_FLAGS(trace_grp_p, VTSS_TRACE_FLAGS_RINGBUF)) {
        trace_rb_write_string(err_buf, data);
    } else {
        trace_write_string(err_buf, data);
    }

    return len;
}

/******************************************************************************/
// grp_trace_vprintf()
/******************************************************************************/
static int grp_trace_vprintf(const vtss_trace_grp_t *const trace_grp_p,
                             char *const err_buf,
                             const char *fmt, va_list ap)
{
    int rv = 0;

    if (HAS_FLAGS(trace_grp_p, VTSS_TRACE_FLAGS_RINGBUF)) {
        rv = trace_rb_vprintf(err_buf, fmt, ap);
    } else {
        rv = trace_vprintf(err_buf, fmt, ap);
    }

    return rv;
}

/******************************************************************************/
// grp_trace_write_string()
/******************************************************************************/
static void grp_trace_write_string(const vtss_trace_grp_t *const trace_grp_p,
                                   char *const err_buf,
                                   const char *str)
{
    if (HAS_FLAGS(trace_grp_p, VTSS_TRACE_FLAGS_RINGBUF)) {
        trace_rb_write_string(err_buf, str);
    } else {
        trace_write_string(err_buf, str);
    }
}

/******************************************************************************/
// grp_trace_write_char()
/******************************************************************************/
static void grp_trace_write_char(const vtss_trace_grp_t *const trace_grp_p,
                                 char *const err_buf,
                                 const char c)
{
    if (HAS_FLAGS(trace_grp_p, VTSS_TRACE_FLAGS_RINGBUF)) {
        trace_rb_write_char(err_buf, c);
    } else {
        trace_write_char(err_buf, c);
    }
}

/******************************************************************************/
// grp_trace_flush()
/******************************************************************************/
static void grp_trace_flush(const vtss_trace_grp_t *const trace_grp_p)
{
    if (!HAS_FLAGS(trace_grp_p, VTSS_TRACE_FLAGS_RINGBUF)) {
        trace_flush();
    }
}

/******************************************************************************/
// trace_lvl_to_str()
/******************************************************************************/
static const char *trace_lvl_to_str(int lvl)
{
    switch (lvl) {
    case VTSS_TRACE_LVL_ERROR:
        return "E";
    case VTSS_TRACE_LVL_WARNING:
        return "W";
    case VTSS_TRACE_LVL_INFO:
        return "I";
    case VTSS_TRACE_LVL_DEBUG:
        return "D";
    case VTSS_TRACE_LVL_NOISE:
        return "N";
    case VTSS_TRACE_LVL_RACKET:
        return "R";
    default:
        break;
    }

    return "?";
}

/******************************************************************************/
// time_str()
/******************************************************************************/
static void time_str(char *buf)
{
    time_t t = time(NULL);
    struct tm *timeinfo_p;
    struct tm timeinfo;

#if defined(VTSS_SW_OPTION_SYSUTIL)
    /* Correct for timezone */
    t += (system_get_tz_off() * 60);
#endif

#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
    /* Correct for DST */
    t += (time_dst_get_offset() * 60);
#endif

    timeinfo_p = localtime_r(&t, &timeinfo);

    sprintf(buf, "%02d:%02d:%02d",
            timeinfo_p->tm_hour,
            timeinfo_p->tm_min,
            timeinfo_p->tm_sec);
}

/******************************************************************************/
// trace_grp_init()
/******************************************************************************/
static void trace_grp_init(vtss_trace_grp_t *trace_grp_p)
{
    if (trace_grp_p->lvl == 0) {
        trace_grp_p->lvl    = VTSS_TRACE_LVL_ERROR;
    }

    TRACE_ASSERT((trace_grp_p->flags & VTSS_TRACE_FLAGS_INIT) == 0, ("grp (%s) already initialized", trace_grp_p->name));
    trace_grp_p->flags |= VTSS_TRACE_FLAGS_INIT;
}

/******************************************************************************/
// trace_grps_init()
/******************************************************************************/
static void  trace_grps_init(vtss_trace_grp_t *trace_grp_p, int cnt)
{
    int i;

    for (i = 0; i < cnt; i++) {
        trace_grp_init(&trace_grp_p[i]);
    }
}

/******************************************************************************/
// trace_threads_init()
/******************************************************************************/
static void trace_threads_init(void)
{
    int i;

    for (i = 0; i <= VTSS_TRACE_MAX_THREAD_ID; i++) {
        trace_threads[i] = trace_thread_default;
    }
}

/******************************************************************************/
// trace_filename()
// Strip leading path from file
/******************************************************************************/
static const char *trace_filename(const char *fn)
{
    int i, start;

    if (!fn) {
        return NULL;
    }

    for (start = 0, i = strlen(fn); i > 0; i--) {
        if (fn[i - 1] == '/') {
            start = i;
            break;
        }
    }

    return fn + start;
}

/******************************************************************************/
// vtss_trace_hunt_cmp()
// Compare the trace messages with the pattern we are looking for and dump stack
// backtrace if there is a hit.
/******************************************************************************/
static void vtss_trace_hunt_cmp(vtss_trace_grp_t *trace_grp_p, char *err_buf, const char *fmt, va_list ap)
{
    char b[256];

    vsnprintf(b, 255, fmt, ap);
    b[255] = '\0';
    if (strstr(b, vtss_trace_hunt_target)) {
        grp_trace_printf(trace_grp_p, err_buf, "Trace hunt hit: '%s'\n", vtss_trace_hunt_target);

        // Dump backtrace when target is found.
        misc_thread_status_print(NONBLOCKING_PRINTF, TRUE, TRUE);
    }
}

/******************************************************************************/
// trace_msg_prefix()
/******************************************************************************/
static void trace_msg_prefix(
    /* Generate copy of trace message for storing errors in flash */
    char             *err_buf,
    vtss_trace_reg_t *trace_reg_p,
    vtss_trace_grp_t *trace_grp_p,
    int              grp_idx,
    int              lvl,
    const char       *location,
    uint             line_no)
{
    grp_trace_write_string(trace_grp_p, err_buf, trace_lvl_to_str(lvl)); /* Trace type */
    grp_trace_write_char(trace_grp_p, err_buf, ' ');
    grp_trace_write_string(trace_grp_p, err_buf, trace_reg_p->name);      /* Registration name */

    if (grp_idx > 0) {
        /* Not default group => print group name */
        grp_trace_write_char(trace_grp_p, err_buf, '/');
        grp_trace_write_string(trace_grp_p, err_buf, trace_grp_p->name);
    }

    grp_trace_write_char(trace_grp_p, err_buf, ' ');

    /* Wall clock time stamp hh:mm:ss */
    {
        /* Calculate current time. This won't work if called from an interrupt handler
         * because time_str() calls time(), which inserts a waiting point. */
        char buf[strlen("hh:mm:ss") + 2];
        time_str(buf);

        grp_trace_printf(trace_grp_p, err_buf, "%s ", &buf[0]);
    }

    /* usec time stamp. Format: ss.mmm,uuu */
    /* Output a usec timestamp if either requested to or users wants a
     * normal timestamp while being marked as a caller from interrupt context */
    if (HAS_FLAGS(trace_grp_p, VTSS_TRACE_FLAGS_USEC)) {
        u64 usecs, s, m, u;

        usecs = hal_time_get();

        s = (usecs / 1000000ULL);
        m = (usecs - 1000000ULL * s) / 1000ULL;
        u = (usecs - 1000000ULL * s - 1000ULL * m);
        s %= 100LLU;

        grp_trace_printf(trace_grp_p, err_buf, "%02u.%03u,%03u ", (u32)s, (u32)m, (u32)u);
    }

    grp_trace_printf(trace_grp_p, err_buf, "%d/%s#%d: ", (int)vtss_thread_id_get(), trace_filename(location), line_no);
}

/******************************************************************************/
// strn_tolower()
/******************************************************************************/
static void strn_tolower(char *str, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        str[i] = tolower(str[i]);
    }
}

/******************************************************************************/
// str_contains_spaces()
/******************************************************************************/
static BOOL str_contains_spaces(char *str)
{
    int i;

    for (i = 0; i < strlen(str); i++) {
        if (str[i] == ' ') {
            return TRUE;
        }
    }

    return FALSE;
}

/******************************************************************************/
// trace_store_prev_lvl()
/******************************************************************************/
static void trace_store_prev_lvl(void)
{
    int mid, gidx;
    int i;

    /* Copy current trace levels to previous */
    for (mid = 0; mid <= MODULE_ID_MAX; mid++) {
        if (trace_regs[mid]) {
            for (gidx = 0; gidx < trace_regs[mid]->grp_cnt; gidx++) {
                trace_regs[mid]->grps[gidx].lvl_prv = trace_regs[mid]->grps[gidx].lvl;
            }
        }
    }

    for (i = 0; i <= VTSS_TRACE_MAX_THREAD_ID; i++) {
        trace_threads[i].lvl_prv = trace_threads[i].lvl;
    }
}

/******************************************************************************/
// trace_to_flash()
/******************************************************************************/
static void trace_to_flash(vtss_trace_grp_t *trace_grp_p, char *err_buf)
{
    if (err_buf) {
#if defined(VTSS_SW_OPTION_SYSLOG)
        syslog_flash_log(SYSLOG_CAT_DEBUG, VTSS_APPL_SYSLOG_LVL_ERROR, err_buf);
#endif /* defined(VTSS_SW_OPTION_SYSLOG) */

        crashfile_printf("%s", err_buf);

        // It's nice to also get a stack backtrace of the running thread
        // when a trace error occurs.
        if (api_and_led_crits_created) {
            misc_thread_status_print(NONBLOCKING_PRINTF, TRUE, TRUE);
            misc_thread_status_print(crashfile_printf,   TRUE, TRUE);
        } else {
            vtss_backtrace(NONBLOCKING_PRINTF, 0);
            vtss_backtrace(crashfile_printf,   0);
        }

        crashfile_close();
    }
}

/******************************************************************************/
// vtss_trace_port_get()
/******************************************************************************/
BOOL vtss_trace_port_get(int port_index)
{
    return vtss_trace_port_list[port_index];
}

/******************************************************************************/
// vtss_trace_port_set()
/******************************************************************************/
void vtss_trace_port_set(int port_index, BOOL trace_disabled)
{
    vtss_trace_port_list[port_index] = trace_disabled;
}

/******************************************************************************/
// vtss_trace_register()
/******************************************************************************/
void vtss_trace_register(vtss_trace_reg_t *trace_reg_p)
{
    int i;

    TRACE_ASSERT(trace_reg_p != NULL, ("trace_reg_p=NULL"));
    TRACE_ASSERT(trace_reg_p->module_id <= MODULE_ID_MAX,
                 ("module_id too large: %d", trace_reg_p->module_id));
    TRACE_ASSERT(trace_reg_p->module_id >= 0,
                 ("module_id too small: %d", trace_reg_p->module_id));

    /* Make sure to zero-terminate name and description string */
    trace_reg_p->name[VTSS_TRACE_MAX_NAME_LEN]   = 0;
    trace_reg_p->descr[VTSS_TRACE_MAX_DESCR_LEN] = 0;

    TRACE_ASSERT(trace_regs[trace_reg_p->module_id] == NULL,
                 ("module_id %d already registered:\n"
                  "Name of 1st registration: %s\n"
                  "Name of 2nd registration: %s",
                  trace_reg_p->module_id,
                  trace_regs[trace_reg_p->module_id]->name,
                  trace_reg_p->name));
    TRACE_ASSERT(trace_reg_p->grps != NULL,
                 ("No groups defined (at least one required), module_id=%d",
                  trace_reg_p->module_id));
    TRACE_ASSERT(trace_init_done,
                 ("trace_init_done=%d", trace_init_done));

    /* Check that registration name contains no spaces */
    TRACE_ASSERT(!str_contains_spaces(trace_reg_p->name),
                 ("Registration name contains spaces.\n"
                  "name: \"%s\"", trace_reg_p->name));

    /* Check that registration name is unique */
    for (i = 0; i <= MODULE_ID_MAX; i++) {
        if (trace_regs[i]) {
            TRACE_ASSERT(strcmp(trace_regs[i]->name, trace_reg_p->name),
                         ("Registration name is not unique. "
                          "Registrations %d and %d are both named \"%s\".",
                          i, trace_reg_p->module_id, trace_reg_p->name));
        }
    }

    /* Check group definitions */
    for (i = 0; i < trace_reg_p->grp_cnt; i++) {
        int j;

        /* Make sure to zero-terminate name and description string */
        trace_reg_p->grps[i].name[VTSS_TRACE_MAX_NAME_LEN]   = 0;
        trace_reg_p->grps[i].descr[VTSS_TRACE_MAX_DESCR_LEN] = 0;

        /* Check that group name contains no spaces */
        TRACE_ASSERT(!str_contains_spaces(trace_reg_p->grps[i].name),
                     ("Group name contains spaces.\n"
                      "module: %s\n"
                      "group: \"%s\"",
                      trace_reg_p->name,
                      trace_reg_p->grps[i].name));

        /* Check that group names within registration are unique */
        for (j = 0; j < trace_reg_p->grp_cnt; j++) {
            if (j != i) {
                TRACE_ASSERT(strcmp(trace_reg_p->grps[i].name,
                                    trace_reg_p->grps[j].name),
                             ("Group names are not unique for registration #%d (\"%s\"): "
                              "Two groups are named \"%s\".",
                              trace_reg_p->module_id, trace_reg_p->name,
                              trace_reg_p->grps[i].name));
            }
        }
    }

    /* Set lvl_def, lvl_prv, and flags_def to initial value */
    for (i = 0; i < trace_reg_p->grp_cnt; i++) {
        trace_reg_p->grps[i].lvl_prv   = trace_reg_p->grps[i].lvl;
        trace_reg_p->grps[i].lvl_def   = trace_reg_p->grps[i].lvl;
        trace_reg_p->grps[i].flags_def = trace_reg_p->grps[i].flags;
    }

    /* Convert module and group name to lowercase */
    str_tolower(trace_reg_p->name);
    for (i = 0; i < trace_reg_p->grp_cnt; i++) {
        str_tolower(trace_reg_p->grps[i].name);
    }

    trace_regs[trace_reg_p->module_id] = trace_reg_p;

    // Apply trace config from configuration file to this module.
    (void)trace_conf_apply(trace_regs, ARRSZ(trace_regs), trace_reg_p->module_id);
}

/******************************************************************************/
// vtss_trace_reg_init()
/******************************************************************************/
void vtss_trace_reg_init(vtss_trace_reg_t *trace_reg_p, vtss_trace_grp_t *trace_grp_p, int grp_cnt)
{
    if (!trace_init_done) {
        // Initialize and register our own trace ressources
        trace_init_done = 1;
        trace_threads_init();

        vtss_trace_reg_init(&trace_reg, trace_grps, ARRSZ(trace_grps));
        vtss_trace_register(&trace_reg);

        // Read and apply configuration from flash (if present)
        vtss_trace_cfg_rd();

        // Create critical region variables
        vtss_sem_init(&trace_io_crit, 1);

        // Ring-buf protection is handled through vtss_global_lock/unlock()
        // calls because the caller may be calling from a locked context.
        vtss_sem_init(&trace_rb_crit, 1);
    }

    TRACE_ASSERT(trace_reg_p->flags == 0, ("trace_reg_p->flags!=0"));
    trace_reg_p->flags = VTSS_TRACE_FLAGS_INIT;

    trace_grps_init(trace_grp_p, grp_cnt);
    trace_reg_p->grp_cnt = grp_cnt;
    trace_reg_p->grps    = trace_grp_p;
}

/******************************************************************************/
// TraceRegister::TraceRegister()
/******************************************************************************/
TraceRegister::TraceRegister(vtss_trace_reg_t *trace_reg_p, vtss_trace_grp_t *trace_grp_p, int grp_cnt)
{
#if (VTSS_TRACE_LVL_MIN < VTSS_TRACE_LVL_NONE)
    if (trace_init_done) {
        T_D("Registering %s", vtss_module_names[trace_reg_p->module_id]);
    }

    vtss_trace_reg_init(trace_reg_p, trace_grp_p, grp_cnt);
    vtss_trace_register(trace_reg_p);
#endif
}

/******************************************************************************/
// vtss_trace_reg_get()
/******************************************************************************/
const vtss_trace_reg_t *vtss_trace_reg_get(int module_id, int grp_idx)
{
    if (module_id < 0 || module_id >= VTSS_MODULE_ID_NONE) {
        return NULL;
    }

    return trace_regs[module_id];
}

/******************************************************************************/
// vtss_trace_grp_get()
/******************************************************************************/
const vtss_trace_grp_t *vtss_trace_grp_get(int module_id, int grp_idx)
{
    vtss_trace_reg_t *trace_reg_p;

    if (module_id < 0 || module_id >= VTSS_MODULE_ID_NONE) {
        return NULL;
    }

    trace_reg_p = trace_regs[module_id];

    if (!trace_reg_p) {
        return NULL;
    }

    if (grp_idx >= trace_reg_p->grp_cnt) {
        return NULL;
    }

    return &trace_reg_p->grps[grp_idx];
}

/******************************************************************************/
// vtss_trace_vprintf()
/******************************************************************************/
void vtss_trace_vprintf(int module_id,
                        int grp_idx,
                        int lvl,
                        const char *location,
                        uint line_no,
                        const char *fmt,
                        va_list args)
{
    vtss_trace_grp_t *trace_grp_p;
    vtss_trace_reg_t *trace_reg_p;
    char             *err_buf = NULL;

    trace_reg_p = trace_regs[module_id];
    if (trace_reg_p == NULL || trace_reg_p->grps == NULL) {
        trace_reg_p = trace_regs[VTSS_MODULE_ID_TRACE];
        if (trace_reg_p == NULL) { // There is no TRACE modules at the moment ????
            PRINTE(("module_id:%d, module_name:%s, grp_idx:%d, location: %s  line: %u\n", module_id, vtss_module_names[module_id], grp_idx, location, line_no));
            return;
        }

        TRACE_ASSERT(FALSE, ("trace_reg_p is NULL for module_id=%d %s#%d", module_id, location, line_no));
        return;
    }

    /* Get module's registration */
    TRACE_ASSERT(trace_reg_p != NULL, ("module_id=%d %s#%d", module_id, location, line_no));

    /* Get group pointer */
    trace_grp_p = &trace_reg_p->grps[grp_idx];
    TRACE_ASSERT(trace_grp_p != NULL, ("module_id=%d", module_id));

    // #lvl checked at outer level (T_xxx() macros)

    /* Check that level appears to be one of the well-defined ones */
    TRACE_ASSERT(
        lvl == VTSS_TRACE_LVL_NONE    ||
        lvl == VTSS_TRACE_LVL_ERROR   ||
        lvl == VTSS_TRACE_LVL_WARNING ||
        lvl == VTSS_TRACE_LVL_INFO    ||
        lvl == VTSS_TRACE_LVL_DEBUG   ||
        lvl == VTSS_TRACE_LVL_NOISE   ||
        lvl == VTSS_TRACE_LVL_RACKET,
        ("Unknown trace level used in %s#%d: lvl=%d",
         location, line_no, lvl));

    if (lvl == VTSS_TRACE_LVL_ERROR) {
        if (api_and_led_crits_created) {
            /* Don't set LED from ISR/DSR context. */
            led_front_led_state(LED_FRONT_LED_ERROR, FALSE);
        }

        /* Allocate buffer in which to build error message for flash */
        if ((VTSS_MALLOC_CAST(err_buf, TRACE_ERR_BUF_SIZE))) {
            err_buf[0] = 0;
        }
    }

    /* Print prefix */
    trace_msg_prefix(err_buf,
                     trace_reg_p,
                     trace_grp_p,
                     grp_idx,
                     lvl,
                     location,
                     line_no);

    /* If error or warning add prefix which runtc can identify */
    if (lvl == VTSS_TRACE_LVL_ERROR) {
        grp_trace_printf(trace_grp_p, err_buf, "Error: ");
    }

    if (lvl == VTSS_TRACE_LVL_WARNING) {
        grp_trace_printf(trace_grp_p, err_buf, "Warning: ");
    }

    /* Print message */
    grp_trace_vprintf(trace_grp_p, err_buf, fmt, args);

    grp_trace_write_char(trace_grp_p, err_buf, '\n');
    grp_trace_flush(trace_grp_p);

    if (vtss_trace_hunt_target) {
        vtss_trace_hunt_cmp(trace_grp_p, err_buf, fmt, args);
    }

    trace_to_flash(trace_grp_p, err_buf);
    if (err_buf) {
        VTSS_FREE(err_buf);
    }
}

/******************************************************************************/
// vtss_trace_var_printf_impl()
/******************************************************************************/
void vtss_trace_var_printf_impl(int module_id, int grp_idx, int lvl,
                                const char *location, uint line_no,
                                const char *data, size_t len)
{
    vtss_trace_grp_t *trace_grp_p;
    vtss_trace_reg_t *trace_reg_p;
    char             *err_buf = NULL;

    trace_reg_p = trace_regs[module_id];
    if (trace_reg_p == NULL || trace_reg_p->grps == NULL) {
        trace_reg_p = trace_regs[VTSS_MODULE_ID_TRACE];
        if (trace_reg_p == NULL) { // There is no TRACE modules at the moment ????
            PRINTE(("module_id:%d, module_name:%s, grp_idx:%d, location: %s  line: %u\n", module_id, vtss_module_names[module_id], grp_idx, location, line_no));
            return;
        }

        TRACE_ASSERT(FALSE, ("trace_reg_p is NULL for module_id=%d %s#%d", module_id, location, line_no));
        return;
    }

    /* Get module's registration */
    TRACE_ASSERT(trace_reg_p != NULL, ("module_id=%d %s#%d", module_id, location, line_no));

    /* Get group pointer */
    trace_grp_p = &trace_reg_p->grps[grp_idx];
    TRACE_ASSERT(trace_grp_p != NULL, ("module_id=%d", module_id));

    // #lvl checked at outer level (T_xxx() macros)

    /* Check that level appears to be one of the well-defined ones */
    TRACE_ASSERT(
        lvl == VTSS_TRACE_LVL_NONE    ||
        lvl == VTSS_TRACE_LVL_ERROR   ||
        lvl == VTSS_TRACE_LVL_WARNING ||
        lvl == VTSS_TRACE_LVL_INFO    ||
        lvl == VTSS_TRACE_LVL_DEBUG   ||
        lvl == VTSS_TRACE_LVL_NOISE   ||
        lvl == VTSS_TRACE_LVL_RACKET,
        ("Unknown trace level used in %s#%d: lvl=%d",
         location, line_no, lvl));

    if (lvl == VTSS_TRACE_LVL_ERROR) {
        if (api_and_led_crits_created) {
            /* Don't set LED from ISR/DSR context. */
            led_front_led_state(LED_FRONT_LED_ERROR, FALSE);
        }

        /* Allocate buffer in which to build error message for flash */
        if ((VTSS_MALLOC_CAST(err_buf, TRACE_ERR_BUF_SIZE))) {
            err_buf[0] = 0;
        }
    }

    /* Print prefix */
    trace_msg_prefix(err_buf,
                     trace_reg_p,
                     trace_grp_p,
                     grp_idx,
                     lvl,
                     location,
                     line_no);

    /* If error or warning add prefix which runtc can identify */
    if (lvl == VTSS_TRACE_LVL_ERROR) {
        grp_trace_printf(trace_grp_p, err_buf, "Error: ");
    }

    if (lvl == VTSS_TRACE_LVL_WARNING) {
        grp_trace_printf(trace_grp_p, err_buf, "Warning: ");
    }

    /* Print message */
    grp_trace_printf(trace_grp_p, err_buf, data, len);

    grp_trace_write_char(trace_grp_p, err_buf, '\n');
    grp_trace_flush(trace_grp_p);

    // if (vtss_trace_hunt_target) {
    //     vtss_trace_hunt_cmp(trace_grp_p, err_buf, fmt, args);
    // }

    trace_to_flash(trace_grp_p, err_buf);
    if (err_buf) {
        VTSS_FREE(err_buf);
    }
}

/******************************************************************************/
// vtss_trace_printf()
/******************************************************************************/
void vtss_trace_printf(int module_id,
                       int grp_idx,
                       int lvl,
                       const char *location,
                       uint line_no,
                       const char *fmt,
                       ...)
{
    va_list args;
    va_start(args, fmt);
    vtss_trace_vprintf(module_id, grp_idx, lvl, location, line_no, fmt, args);
    va_end(args);
}

/******************************************************************************/
// vtss_trace_hex_dump()
/******************************************************************************/
void vtss_trace_hex_dump(int module_id,
                         int grp_idx,
                         int lvl,
                         const char *location,
                         uint line_no,
                         const uchar *byte_p,
                         int  byte_cnt)
{
    int               i = 0;
    vtss_trace_grp_t *trace_grp_p;
    vtss_trace_reg_t *trace_reg_p;
    char             *err_buf = NULL;

    trace_reg_p = trace_regs[module_id];
    TRACE_ASSERT(trace_reg_p != NULL, ("module_id=%d", module_id));

    trace_grp_p = &trace_reg_p->grps[grp_idx];
    TRACE_ASSERT(trace_grp_p != NULL, ("module_id=%d", module_id));

    if (lvl < trace_grp_p->lvl ||
        lvl >= VTSS_TRACE_LVL_NONE) {
        return;
    }

    /* Check that level appears to be one of the well-defined ones */
    TRACE_ASSERT(
        lvl == VTSS_TRACE_LVL_NONE    ||
        lvl == VTSS_TRACE_LVL_ERROR   ||
        lvl == VTSS_TRACE_LVL_WARNING ||
        lvl == VTSS_TRACE_LVL_INFO    ||
        lvl == VTSS_TRACE_LVL_DEBUG   ||
        lvl == VTSS_TRACE_LVL_NOISE   ||
        lvl == VTSS_TRACE_LVL_RACKET,
        ("Unknown trace level used in %s#%d: lvl=%d",
         location, line_no, lvl));

    if (lvl == VTSS_TRACE_LVL_ERROR) {
        if (api_and_led_crits_created) {
            /* Don't set LED from ISR/DSR context. */
            led_front_led_state(LED_FRONT_LED_ERROR, FALSE);
        }

        /* Allocate buffer in which to build error message for flash */
        if ((VTSS_MALLOC_CAST(err_buf, TRACE_ERR_BUF_SIZE))) {
            err_buf[0] = 0;
        }
    }

    trace_msg_prefix(err_buf,
                     trace_reg_p,
                     trace_grp_p,
                     grp_idx,
                     lvl,
                     location,
                     line_no);

    if (byte_cnt > 16) {
        grp_trace_write_char(trace_grp_p, err_buf, '\n');
    }

    i = 0;
    while (i < byte_cnt) {
        int j = 0;
        grp_trace_printf(trace_grp_p, err_buf, "%p/%04x: ", byte_p + i, i);
        while (j + i < byte_cnt && j < 16) {
            grp_trace_write_char(trace_grp_p, err_buf, ' ');
            grp_trace_printf(trace_grp_p, err_buf, "%02x", byte_p[i + j]);
            j++;
        }

        grp_trace_write_char(trace_grp_p, err_buf, '\n');
        i += 16;
    }

    grp_trace_flush(trace_grp_p);

    trace_to_flash(trace_grp_p, err_buf);
    if (err_buf) {
        VTSS_FREE(err_buf);
    }
}

/******************************************************************************/
// vtss_trace_hunt_set()
/******************************************************************************/
void vtss_trace_hunt_set(char *tgt)
{
    char *clone = NULL;

    if (tgt && strlen(tgt) && VTSS_MALLOC_CAST(clone, strlen(tgt) + 1)) {
        strcpy(clone, tgt);
    }

    if (vtss_trace_hunt_target) {
        VTSS_FREE(vtss_trace_hunt_target);
    }

    vtss_trace_hunt_target = clone;
}

/******************************************************************************/
// vtss_trace_module_lvl_set()
/******************************************************************************/
void vtss_trace_module_lvl_set(int module_id, int grp_idx, int lvl)
{
    int i, j;
    int module_id_start = 0, module_id_stop = MODULE_ID_MAX;
    int grp_idx_start = 0, grp_idx_stop = 0;

    if (module_id != -1) {
        module_id_start = module_id;
        module_id_stop  = module_id;

        if (trace_regs[module_id] == NULL) {
            T_E("No registration for module_id=%d", module_id);
            return;
        }
    } else {
        /* Always wildcard group if module is wildcarded */
        grp_idx         = -1;
    }

    if (grp_idx != -1) {
        grp_idx_start = grp_idx;
        grp_idx_stop = grp_idx;

        if (grp_idx > trace_regs[module_id]->grp_cnt - 1) {
            T_E("grp_idx=%d too big for module_id=%d (grp_cnt=%d)", grp_idx, module_id, trace_regs[module_id]->grp_cnt);
            return;
        }
    }

    /* Store current trace levels before changing */
    trace_store_prev_lvl();

    for (i = module_id_start; i <= module_id_stop; i++) {
        if (trace_regs[i] == NULL) {
            continue;
        }

        if (grp_idx == -1) {
            grp_idx_start = 0;
            grp_idx_stop = trace_regs[i]->grp_cnt - 1;
        }

        for (j = grp_idx_start; j <= grp_idx_stop; j++) {
            trace_regs[i]->grps[j].lvl = lvl;
        }
    }
}

/******************************************************************************/
// vtss_trace_module_parm_set()
/******************************************************************************/
void vtss_trace_module_parm_set(vtss_trace_module_parm_t parm, int module_id, int grp_idx, BOOL enable)
{
    int i, j;
    int module_id_start = 0, module_id_stop = MODULE_ID_MAX;
    int grp_idx_start = 0, grp_idx_stop = 0;
    u32 flag = 0;

    if (module_id != -1) {
        module_id_start = module_id;
        module_id_stop  = module_id;

        if (trace_regs[module_id] == NULL) {
            T_E("No registration for module_id=%d", module_id);
            vtss_backtrace(NONBLOCKING_PRINTF, 0);
            return;
        }
    } else {
        /* Always wildcard group if module is wildcarded */
        grp_idx         = -1;
    }

    if (grp_idx != -1) {
        grp_idx_start = grp_idx;
        grp_idx_stop = grp_idx;

        if (grp_idx > trace_regs[module_id]->grp_cnt - 1) {
            T_E("grp_idx=%d too big for module_id=%d (grp_cnt=%d)",
                grp_idx, module_id, trace_regs[module_id]->grp_cnt);
            return;
        }
    }

    switch (parm) {
    case VTSS_TRACE_MODULE_PARM_USEC:
        flag = VTSS_TRACE_FLAGS_USEC;
        break;
    case VTSS_TRACE_MODULE_PARM_RINGBUF:
        flag = VTSS_TRACE_FLAGS_RINGBUF;
        break;
    default:
        T_E("Unknown parm: %d", parm);
        return;
    }

    for (i = module_id_start; i <= module_id_stop; i++) {
        if (trace_regs[i] == NULL) {
            continue;
        }

        if (grp_idx == -1) {
            grp_idx_start = 0;
            grp_idx_stop = trace_regs[i]->grp_cnt - 1;
        }

        for (j = grp_idx_start; j <= grp_idx_stop; j++) {
            vtss_trace_grp_t *grp = &trace_regs[i]->grps[j];
            if (enable) {
                grp->flags |= flag;
            } else {
                grp->flags &= ~flag;
            }
        }
    }
}

/******************************************************************************/
// vtss_trace_lvl_revert()
/******************************************************************************/
void vtss_trace_lvl_revert(void)
{
    int mid, gidx;
    int lvl;
    int i;

    for (mid = 0; mid <= MODULE_ID_MAX; mid++) {
        if (trace_regs[mid]) {
            for (gidx = 0; gidx < trace_regs[mid]->grp_cnt; gidx++) {
                lvl = trace_regs[mid]->grps[gidx].lvl;
                trace_regs[mid]->grps[gidx].lvl     = trace_regs[mid]->grps[gidx].lvl_prv;
                trace_regs[mid]->grps[gidx].lvl_prv = lvl;
            }
        }
    }

    for (i = 0; i <= VTSS_TRACE_MAX_THREAD_ID; i++) {
        lvl = trace_threads[i].lvl;
        trace_threads[i].lvl = trace_threads[i].lvl_prv;
        trace_threads[i].lvl_prv = lvl;
    }
}

/******************************************************************************/
// vtss_trace_defaults()
/******************************************************************************/
mesa_rc vtss_trace_defaults(void)
{
    int module_id, grp_idx;

    /* Copy current trace levels to previous */
    for (module_id = 0; module_id < ARRSZ(trace_regs); module_id++) {
        vtss_trace_reg_t *trace_reg = trace_regs[module_id];

        if (trace_reg == NULL) {
            continue;
        }

        for (grp_idx = 0; grp_idx < trace_reg->grp_cnt; grp_idx++) {
            vtss_trace_grp_t *grp = &trace_reg->grps[grp_idx];
            grp->lvl   = grp->lvl_def;
            grp->flags = grp->flags_def;
        }
    }

    return MESA_RC_OK;
}

/******************************************************************************/
// vtss_trace_global_lvl_get()
/******************************************************************************/
int vtss_trace_global_lvl_get(void)
{
    return global_lvl;
}

/******************************************************************************/
// vtss_trace_global_lvl_set()
/******************************************************************************/
void vtss_trace_global_lvl_set(int lvl)
{
    global_lvl = lvl;
}

/******************************************************************************/
// vtss_trace_global_module_lvl_get()
/******************************************************************************/
int vtss_trace_global_module_lvl_get(int module_id, int grp_idx)
{
    if (trace_regs[module_id] == NULL) {
        T_E("No registration for module_id = %d (%s)", module_id, vtss_module_names[module_id]);
        return VTSS_TRACE_LVL_NONE;
    }

    if (grp_idx > trace_regs[module_id]->grp_cnt - 1) {
        T_E("grp_idx (%d) too big for module %s, (id = %d) (grp_cnt = %d)", grp_idx, vtss_module_names[module_id],  module_id, trace_regs[module_id]->grp_cnt);
        return 0;
    }

    return (global_lvl > trace_regs[module_id]->grps[grp_idx].lvl) ? global_lvl : trace_regs[module_id]->grps[grp_idx].lvl;
}

/******************************************************************************/
// vtss_trace_module_lvl_get()
/******************************************************************************/
int vtss_trace_module_lvl_get(int module_id, int grp_idx)
{
    if (trace_regs[module_id] == NULL) {
        T_E("No registration for module_id = %d (%s)", module_id, vtss_module_names[module_id]);
        return 0;
    }

    if (grp_idx > trace_regs[module_id]->grp_cnt - 1) {
        T_E("grp_idx=%d too big for module_id=%d (grp_cnt=%d)",
            grp_idx, module_id, trace_regs[module_id]->grp_cnt);
        return 0;
    }

    return trace_regs[module_id]->grps[grp_idx].lvl;
}

/******************************************************************************/
// vtss_trace_module_name_get_next()
/******************************************************************************/
const char *vtss_trace_module_name_get_next(int *module_id_p)
{
    int i;

    TRACE_ASSERT(*module_id_p >= -1, ("*module_id_p=%d", *module_id_p));

    for (i = *module_id_p + 1; i < MODULE_ID_MAX; i++) {
        if (trace_regs[i]) {
            *module_id_p = i;
            return trace_regs[i]->name;
        }
    }

    return NULL;
}

/******************************************************************************/
// vtss_trace_grp_name_get_next()
/******************************************************************************/
const char *vtss_trace_grp_name_get_next(int module_id, int *grp_idx_p)
{
    TRACE_ASSERT(*grp_idx_p >= -1, (" "));

    if (!trace_regs[module_id]) {
        return NULL;
    }

    if (*grp_idx_p + 1 >= trace_regs[module_id]->grp_cnt) {
        return NULL;
    }

    (*grp_idx_p)++;

    /* Check flags */
    if (trace_regs[module_id]->flags != VTSS_TRACE_FLAGS_INIT) {
        T_E("module_id=%d: Flags=0x%08x, expected 0x%08x",
            module_id,
            trace_regs[module_id]->flags,
            VTSS_TRACE_FLAGS_INIT);
    }

    if (!(trace_regs[module_id]->grps[*grp_idx_p].flags & VTSS_TRACE_FLAGS_INIT)) {
        T_E("module_id=%d, grp_idx=%d: Flags=0x%08x, expected 0x%08x",
            module_id,
            *grp_idx_p,
            trace_regs[module_id]->grps[*grp_idx_p].flags,
            VTSS_TRACE_FLAGS_INIT);
    }

    return trace_regs[module_id]->grps[*grp_idx_p].name;
}

/******************************************************************************/
// vtss_trace_module_dscr_get()
/******************************************************************************/
const char *vtss_trace_module_dscr_get(int module_id)
{
    if (!trace_regs[module_id]) {
        return NULL;
    }

    return trace_regs[module_id]->descr;
}

/******************************************************************************/
// vtss_trace_grp_dscr_get()
/******************************************************************************/
const char *vtss_trace_grp_dscr_get(int module_id, int grp_idx)
{
    if (!trace_regs[module_id]) {
        return NULL;
    }

    if (grp_idx > trace_regs[module_id]->grp_cnt - 1) {
        return NULL;
    }

    return trace_regs[module_id]->grps[grp_idx].descr;
}

/******************************************************************************/
// vtss_trace_grp_parm_get()
/******************************************************************************/
BOOL vtss_trace_grp_parm_get(vtss_trace_module_parm_t parm, int module_id, int grp_idx)
{
    const vtss_trace_grp_t *grp;
    if (!trace_regs[module_id]) {
        T_E("trace_regs[module_id]=null");
        return 0;
    }

    if (grp_idx > trace_regs[module_id]->grp_cnt - 1) {
        T_E("grp_idx=%d, grp_cnt=%d", grp_idx, trace_regs[module_id]->grp_cnt);
        return 0;
    }

    grp = &trace_regs[module_id]->grps[grp_idx];
    switch (parm) {
    case VTSS_TRACE_MODULE_PARM_USEC:
        return !!HAS_FLAGS(grp, VTSS_TRACE_FLAGS_USEC);
    case VTSS_TRACE_MODULE_PARM_RINGBUF:
        return !!HAS_FLAGS(grp, VTSS_TRACE_FLAGS_RINGBUF);
    default:
        T_E("Unknown parm: %d", parm);
        break;
    }

    return 0;
}

/******************************************************************************/
// vtss_trace_module_name_to_id()
/******************************************************************************/
mesa_rc vtss_trace_module_name_to_id(const char *name, int *module_id_p)
{
    int i;
    char name_lc[VTSS_TRACE_MAX_NAME_LEN + 1];
    int  match_cnt = 0;

    /* Convert name to lowercase */
    strncpy(name_lc, name, VTSS_TRACE_MAX_NAME_LEN);
    name_lc[VTSS_TRACE_MAX_NAME_LEN] = 0;
    strn_tolower(name_lc, strlen(name_lc));

    for (i = 0; i < MODULE_ID_MAX; i++) {
        if (!trace_regs[i]) {
            continue;
        }

        // Test if the search string matches the module name
        if (strncmp(name_lc, trace_regs[i]->name, strlen(name_lc)) != 0) {
            continue;
        }

        if (strlen(trace_regs[i]->name) == strlen(name_lc)) {
            // Exact match found
            *module_id_p = i;
            return MESA_RC_OK;
        }

        match_cnt ++;
        *module_id_p = i;
    }

    return match_cnt == 1 ? MESA_RC_OK : MESA_RC_ERROR;
}

/******************************************************************************/
// vtss_trace_grp_name_to_id()
/******************************************************************************/
mesa_rc vtss_trace_grp_name_to_id(const char *name, int  module_id, int *grp_idx_p)
{
    int i;
    char name_lc[VTSS_TRACE_MAX_NAME_LEN + 1];
    int gidx_match = -1;

    /* Check for valid module_id, in case CLI should call us with -2 */
    if (module_id < 0 || module_id > MODULE_ID_MAX || !trace_regs[module_id]) {
        return MESA_RC_ERROR;
    }

    /* Convert name to lowercase */
    strncpy(name_lc, name, VTSS_TRACE_MAX_NAME_LEN);
    name_lc[VTSS_TRACE_MAX_NAME_LEN] = 0;
    strn_tolower(name_lc, strlen(name_lc));

    for (i = 0; i < trace_regs[module_id]->grp_cnt; i++) {
        if (strncmp(name_lc, trace_regs[module_id]->grps[i].name, strlen(name_lc)) == 0) {
            if (strlen(trace_regs[module_id]->grps[i].name) == strlen(name_lc)) {
                /* Exact match found */
                gidx_match = i;
                break;
            }

            if (gidx_match == -1) {
                /* First match found */
                gidx_match = i;
            } else {
                /* >1 match found */
                return MESA_RC_ERROR;
            }
        }
    }

    if (gidx_match != -1) {
        *grp_idx_p = gidx_match;
        return MESA_RC_OK;
    }

    return MESA_RC_ERROR;
}

/******************************************************************************/
// vtss_trace_lvl_to_val()
/******************************************************************************/
mesa_rc vtss_trace_lvl_to_val(const char *name, int *level_p)
{
    static struct {
        int level;
        const char *name;
    } levels[] = {
        {VTSS_TRACE_LVL_NONE,    "none"},
        {VTSS_TRACE_LVL_ERROR,   "error"},
        {VTSS_TRACE_LVL_WARNING, "warning"},
        {VTSS_TRACE_LVL_INFO,    "info"},
        {VTSS_TRACE_LVL_DEBUG,   "debug"},
        {VTSS_TRACE_LVL_NOISE,   "noise"},
        {VTSS_TRACE_LVL_RACKET,  "racket"},
    };

    int i;
    for (i = 0; i < ARRSZ(levels); i++) {
        if (strcmp(levels[i].name, name) == 0) {
            *level_p = levels[i].level;
            return MESA_RC_OK;
        }
    }

    return MESA_RC_ERROR;
}

/******************************************************************************/
// vtss_trace_lvl_to_str()
/******************************************************************************/
const char *vtss_trace_lvl_to_str(int lvl)
{
    switch (lvl) {
    case VTSS_TRACE_LVL_NONE:
        return "none";
    case VTSS_TRACE_LVL_ERROR:
        return "error";
    case VTSS_TRACE_LVL_WARNING:
        return "warning";
    case VTSS_TRACE_LVL_INFO:
        return "info";
    case VTSS_TRACE_LVL_DEBUG:
        return "debug";
    case VTSS_TRACE_LVL_NOISE:
        return "noise";
    case VTSS_TRACE_LVL_RACKET:
        return "racket";
    default:
        return "others";
    }
}

/******************************************************************************/
// vtss_trace_thread_lvl_set()
/******************************************************************************/
void vtss_trace_thread_lvl_set(int thread_id, int lvl)
{
    TRACE_ASSERT(thread_id == -1 || thread_id <= VTSS_TRACE_MAX_THREAD_ID, ("thread_id=%d", thread_id));

    /* Store current trace levels before changing */
    trace_store_prev_lvl();

    if (thread_id == -1) {
        int i;
        for (i = 0; i <= VTSS_TRACE_MAX_THREAD_ID; i++) {
            trace_threads[i].lvl = lvl;
        }
    } else {
        trace_threads[thread_id].lvl = lvl;
    }
}

/******************************************************************************/
// vtss_trace_cfg_erase()
/******************************************************************************/
mesa_rc vtss_trace_cfg_erase(void)
{
    return trace_conf_erase();
}

/******************************************************************************/
// vtss_trace_cfg_rd()
/******************************************************************************/
mesa_rc vtss_trace_cfg_rd(void)
{
    VTSS_RC(trace_conf_load());

    return trace_conf_apply(trace_regs, ARRSZ(trace_regs));
}

/******************************************************************************/
// vtss_trace_cfg_wr()
/******************************************************************************/
mesa_rc vtss_trace_cfg_wr(void)
{
    return trace_conf_save(trace_regs, ARRSZ(trace_regs));
}

extern "C" int util_icli_cmd_register();

/******************************************************************************/
// vtss_trace_init()
/******************************************************************************/
mesa_rc vtss_trace_init(vtss_init_data_t *data)
{
    mesa_rc rc = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        util_icli_cmd_register();
        break;

    case INIT_CMD_START:
        // At this point, we can start calling led_front_led_state(), because
        // both the LED module's critd and the API's critd (and the API itself)
        // are created.
        api_and_led_crits_created = TRUE;
        break;

    default:
        break;
    }

    return rc;
}

