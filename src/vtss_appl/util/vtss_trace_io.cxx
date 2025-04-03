/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <stdio.h>
#include "main.h"
#include "vtss_trace.h"
#include "cli_io_api.h" /* To get access to cli_printf() */

#include <stdio.h>
#define diag_write_string(s) fputs(s, stdout)
#define diag_write(s, l)     fwrite(s, 1, l, stdout)
#define diag_write_char(c)   putchar(c)

#define TRACE_MAX_IO_REGS 8 /* 4 for Telnet (CLI_TELNET_MAX_CLIENT) and 4 for SSH (DROPBEAR_MAX_SOCK_NUM) */
typedef struct _trace_io_reg_t {
    vtss_trace_io_t  *io;
    vtss_module_id_t module_id;
} trace_io_reg_t;

static int trace_io_reg_cnt = 0;
static trace_io_reg_t trace_io_regs[TRACE_MAX_IO_REGS];

/* Trace ring buffer */
#define TRACE_RB_SIZE           100000
#define TRACE_RB_MAX_ENTRY_SIZE 1000 /* Max size of a single entry in rb */
typedef struct _trace_rb_t {
    char buf[TRACE_RB_SIZE+1];
    int  wr_pos;
    BOOL dis; // Disable field instead of an enable field, so that we can avoid initializing it
} trace_rb_t;
static trace_rb_t trace_rb;

/* Don't wait for a semaphore, because the caller of trace
   might be marked as a caller from IRQ context. */
#define WAIT_RINGBUF() vtss_global_lock(__FILE__, __LINE__)
#define POST_RINGBUF() vtss_global_unlock(__FILE__, __LINE__)

/* ###########################################################################
 * Internal functions
 *
 * The "err_buf" argument is used to store a copy of the trace message,
 * used for writing error messages to flash.
 * If err_buf is NULL, then such copy is generated.
 * ------------------------------------------------------------------------ */

/* ===========================================================================
 * Console functions
 * ------------------------------------------------------------------------ */

int  trace_sprintf(char *buf, const char *fmt, ...)
{
    int rv;
    va_list ap;
    va_start(ap, fmt);

    rv = sprintf(buf, fmt, ap);

    va_end(ap);

    return rv;
} /* trace_sprintf */


/* NB: This function must *only* use ap *once*. Thus, the formatting
 * is now done to a stack buffer, which incur overhead for each
 * calling thread context. Using va_list is a non-conformance issue on
 * some Linux targets. */
int  trace_vprintf(char *err_buf, const char *fmt, va_list ap)
{
    int rv;
    char buf[1536];  /* Limiting factor - trace messages exceeding the
                      * capacity of this buffer will be truncated */

    rv = vsnprintf(buf, sizeof(buf), fmt, ap);
    if (rv) {
        diag_write_string(buf);

        if (trace_io_reg_cnt) {
            int i = 0;
            for (i = 0; i < TRACE_MAX_IO_REGS; i++) {
                if (trace_io_regs[i].io != NULL) {
                    trace_io_regs[i].io->trace_write_string(trace_io_regs[i].io, buf);
                }
            }
        }

        if (err_buf) {
            size_t plen = strlen(err_buf);
            strncpy(&err_buf[plen], buf, TRACE_ERR_BUF_SIZE-plen);
        }
    }

    return rv;
} /* trace_vprintf */


int  trace_printf(char *err_buf,  const char *fmt, ...)
{
    int rv = 0;

    va_list ap;
    va_start(ap, fmt);

    rv = trace_vprintf(err_buf, fmt, ap);

    va_end(ap);

    return rv;
} /* trace_printf */


void trace_write_string_len(char *err_buf, const char *str, unsigned length)
{
    diag_write(str, length);
    if (trace_io_reg_cnt) {
        int i = 0;
        for (i = 0; i < TRACE_MAX_IO_REGS; i++) {
            if (trace_io_regs[i].io != NULL) {
                trace_io_regs[i].io->trace_write_string_len(trace_io_regs[i].io, str, length);
            }
        }
    }

    if (err_buf) {
        snprintf(&err_buf[strlen(err_buf)],
                 MIN(TRACE_ERR_BUF_SIZE-strlen(err_buf), length), "%s", str);
    }

} /* trace_write_string_len */

void trace_write_string(char *err_buf, const char *str)
{
    diag_write_string(str);

    if (trace_io_reg_cnt) {
        int i = 0;
        for (i = 0; i < TRACE_MAX_IO_REGS; i++) {
            if (trace_io_regs[i].io != NULL) {
                trace_io_regs[i].io->trace_write_string(trace_io_regs[i].io, str);
            }
        }
    }

    if (err_buf) {
        snprintf(&err_buf[strlen(err_buf)], TRACE_ERR_BUF_SIZE-strlen(err_buf), "%s", str);
    }

} /* trace_write_string */


void trace_write_char(char *err_buf, char c)
{
    diag_write_char(c);

    if (trace_io_reg_cnt) {
        int i = 0;
        for (i = 0; i < TRACE_MAX_IO_REGS; i++) {
            if (trace_io_regs[i].io != NULL) {
                trace_io_regs[i].io->trace_putchar(trace_io_regs[i].io, c);
            }
        }
    }

    if (err_buf) {
        snprintf(&err_buf[strlen(err_buf)], TRACE_ERR_BUF_SIZE-strlen(err_buf), "%c", c);
    }

} /* trace_write_char */


void trace_flush(void)
{
    if (trace_io_reg_cnt) {
        int i = 0;
        for (i = 0; i < TRACE_MAX_IO_REGS; i++) {
            if (trace_io_regs[i].io != NULL) {
                trace_io_regs[i].io->trace_flush(trace_io_regs[i].io);
            }
        }
    }
} /* trace_flush */

/* ---------------------------------------------------------------------------
 * Console functions
 * ======================================================================== */


/* ===========================================================================
 * Ring buffer functions
 * ------------------------------------------------------------------------ */

static void rb_write_string_len(char *err_buf, const char *str, uint len_)
{
    uint str_len = MIN(len_, TRACE_RB_SIZE);

    if (trace_rb.dis != 0) return;

    /* Copy into ring buffer */
    if (trace_rb.wr_pos + str_len <= TRACE_RB_SIZE) {
        /* Fast path */
        memcpy(&trace_rb.buf[trace_rb.wr_pos], str, str_len);
        trace_rb.wr_pos += str_len;
        if (trace_rb.wr_pos >= TRACE_RB_SIZE) {
            trace_rb.wr_pos = 0;
        }
    } else {
        /* Wrap-around */
        memcpy(&trace_rb.buf[trace_rb.wr_pos], str,                                     TRACE_RB_SIZE - trace_rb.wr_pos);
        memcpy(&trace_rb.buf[0],               &str[(TRACE_RB_SIZE - trace_rb.wr_pos)], str_len - (TRACE_RB_SIZE - trace_rb.wr_pos));
        trace_rb.wr_pos = str_len - (TRACE_RB_SIZE - trace_rb.wr_pos);
    }

    /* Make sure to zero-terminate current rb content */
    trace_rb.buf[trace_rb.wr_pos] = 0;

    if (err_buf) {
        snprintf(&err_buf[strlen(err_buf)],
                 MIN(TRACE_ERR_BUF_SIZE-strlen(err_buf), len_), "%s", str);
    }

    TRACE_ASSERT(trace_rb.wr_pos               <= TRACE_RB_SIZE,       ("trace_rb.wr_pos=%d",               trace_rb.wr_pos));
} /* rb_write_string_len */

static void rb_write_string(char *err_buf, const char *str)
{
    rb_write_string_len(err_buf, str, strlen(str));
} /* rb_write_string */

int trace_rb_vprintf(char *err_buf, const char *fmt, va_list ap)
{
    int rv;
    static char entry[TRACE_RB_MAX_ENTRY_SIZE];

    WAIT_RINGBUF();

    /* Build entry */
    rv = vsnprintf(entry, TRACE_RB_MAX_ENTRY_SIZE, fmt, ap);

    /* Write to ring buffer */
    rb_write_string(err_buf, entry);

    POST_RINGBUF();

    return rv;
}

void trace_rb_write_string_len(char *err_buf, const char *str, unsigned length)
{
    WAIT_RINGBUF();
    rb_write_string_len(err_buf, str, length);
    POST_RINGBUF();
}

void trace_rb_write_string(char *err_buf, const char *str)
{
    WAIT_RINGBUF();

    rb_write_string(err_buf, str);

    POST_RINGBUF();
}

void trace_rb_write_char(char *err_buf, char c)
{
    WAIT_RINGBUF();

    static char str[2];
    str[0] = c;
    str[1] = 0;

    rb_write_string(err_buf, str);

    POST_RINGBUF();
}


/* ---------------------------------------------------------------------------
 * Ring buffer functions
 * ======================================================================== */


/* ---------------------------------------------------------------------------
 * Internal functions
 * ######################################################################## */

/* ###########################################################################
 * API functions
 * ------------------------------------------------------------------------ */

mesa_rc vtss_trace_io_register(
    vtss_trace_io_t  *trace_io,
    vtss_module_id_t module_id,
    uint        *io_reg_id)
{
    mesa_rc rc = VTSS_RC_OK;
    uint         i;

    T_D("enter, module_id=%d", module_id);

    vtss_sem_wait(&trace_io_crit);

    /* Find an unused entry in registration table */
    for (i = 0; i < TRACE_MAX_IO_REGS; i++) {
        if (trace_io_regs[i].io == NULL) {
            break;
        }
    }

    if (i < TRACE_MAX_IO_REGS) {
        trace_io_regs[i].io = trace_io;
        trace_io_regs[i].module_id = module_id;
        *io_reg_id = i;
        trace_io_reg_cnt++;
    } else {
        T_E("All io registrations in use(!), TRACE_MAX_IO_REGS=%d, trace_io_reg_cnt=%d",
            TRACE_MAX_IO_REGS, trace_io_reg_cnt);
        rc = VTSS_UNSPECIFIED_ERROR;
    }

    vtss_sem_post(&trace_io_crit);

    T_D("exit, module_id=%d, rc=%d, io_reg_id=%u", module_id, rc, *io_reg_id);

    return rc;
} /* vtss_trace_io_register */


mesa_rc vtss_trace_io_unregister(
    uint *io_reg_id)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, *io_reg_id=%u", *io_reg_id);

    vtss_sem_wait(&trace_io_crit);

    TRACE_ASSERT(*io_reg_id < TRACE_MAX_IO_REGS, ("*io_reg_id=%d", *io_reg_id));

    trace_io_regs[*io_reg_id].io = NULL;
    trace_io_reg_cnt--;

    vtss_sem_post(&trace_io_crit);

    T_D("exit, module_id=%d, rc=%d, io_reg_id=%u", trace_io_regs[*io_reg_id].module_id, rc, *io_reg_id);

    return rc;
} /* vtss_trace_io_unregister */


/* ===========================================================================
 * Ring buffer functions used by CLI
 * ------------------------------------------------------------------------ */
#ifdef VTSS_SW_OPTION_CLI

void vtss_trace_rb_output(int (*print_function)(const char *fmt, ...))
{
    WAIT_RINGBUF();

    trace_rb.buf[TRACE_RB_SIZE] = 0;

    if (trace_rb.wr_pos+1 < TRACE_RB_SIZE) {
        (void)print_function("%s", &trace_rb.buf[trace_rb.wr_pos+1]);
    }
    if (trace_rb.wr_pos != 0) {
        (void)print_function("%s", trace_rb.buf);
    }

    POST_RINGBUF();
} /* vtss_trace_rb_output */

void vtss_trace_rb_output_as_one_str(i32 (*print_function)(const char *str))
{
    WAIT_RINGBUF();

    trace_rb.buf[TRACE_RB_SIZE] = 0;

    if (trace_rb.wr_pos+1 < TRACE_RB_SIZE) {
        (void)print_function(&trace_rb.buf[trace_rb.wr_pos+1]);
    }
    if (trace_rb.wr_pos != 0) {
        (void)print_function(trace_rb.buf);
    }

    POST_RINGBUF();
} /* vtss_trace_rb_output_as_one_str */

/* Flush current content ring buffer */
void vtss_trace_rb_flush(void)
{
    WAIT_RINGBUF();

    memset(trace_rb.buf, 0, TRACE_RB_SIZE);
    trace_rb.wr_pos = 0;

    POST_RINGBUF();
} /* vtss_trace_rb_flush */


void vtss_trace_rb_ena(BOOL ena)
{
    trace_rb.dis = !ena;
} /* vtss_trace_rb_ena */
#endif


/* ---------------------------------------------------------------------------
 * Ring buffer functions used by CLI
 * ======================================================================== */

/* ---------------------------------------------------------------------------
 * API functions
 * ######################################################################## */

