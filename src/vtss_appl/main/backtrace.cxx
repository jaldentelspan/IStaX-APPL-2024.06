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

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "vtss_os_wrapper.h"
#include "backtrace.hxx"

// Using libgcc_s.so.1 to unwind current thread's stack
#include <execinfo.h>
#include <unwind.h>

struct BT_arg
{
  void **array;
  int cnt, size;
};

/******************************************************************************/
// BT_helper()
/******************************************************************************/
static _Unwind_Reason_Code BT_helper(struct _Unwind_Context *ctx, void *a)
{
    struct BT_arg *arg = (BT_arg *)a;

    // Skip the very first call, since it's vtss_backtrace()
    if (arg->cnt != -1) {
        arg->array[arg->cnt] = (void *)_Unwind_GetIP(ctx);
    }

    if (++arg->cnt == arg->size) {
        return _URC_END_OF_STACK;
    }

    return _URC_NO_REASON;
}

/******************************************************************************/
// libgcc_s_backtrace()
/******************************************************************************/
static char **libgcc_s_backtrace(void **array, int size, int *cnt)
{
    struct BT_arg arg;

    arg.array = array;
    arg.size  = size;
    arg.cnt   = -1;

    if (size >= 1) {
        _Unwind_Backtrace(BT_helper, &arg);
    }

    *cnt = arg.cnt != -1 ? arg.cnt : 0;

    if (*cnt) {
        return backtrace_symbols(array, *cnt);
    } else {
        return nullptr;
    }
}

#if defined(VTSS_SW_OPTION_DEBUG)
#include <vtss/basics/notifications/process-cmd.hxx>
/******************************************************************************/
// bt_backtrace()
/******************************************************************************/
static void bt_backtrace(int (*pr)(const char *fmt, ...), pid_t thread_id)
{
    std::string o;
    char        cmd[20];

    cmd[sizeof(cmd) - 1] = '\0';

    // The source code for the program we invoke is located in
    // .../vtss_appl/backtrace. It takes one parameter - the thread ID.
    snprintf(cmd, sizeof(cmd) - 1, "bt %d", thread_id);

    vtss::notifications::process_cmd(cmd, &o, nullptr);
    pr("%s", o.c_str());
}
#endif /* defined(VTSS_SW_OPTION_DEBUG) */

void vtss_backtrace(int (*pr)(const char *fmt, ...), pid_t thread_id)
{
    const int NSYMS_MAX = 100;
    void      *array[NSYMS_MAX];
    char      **symbols;
    int       i, nsyms;

    if (thread_id && thread_id != syscall(SYS_gettid)) {
#if defined(VTSS_SW_OPTION_DEBUG)
        // The thread we're going to unwind is not ourselves. In order to do so,
        // we must invoke an external program (bt), which is only available when
        // compiling with debug, because it uses a library called libunwind,
        // which takes space, and is therefore only included in the debug SDK.
        // The reason that we can't unwind from within the calling thread is
        // that the remote unwinding code uses ptrace(PTRACE_ATTACH), which is
        // not possible if the calling thread is within the same thread-group
        // as the thread that is to be unwound.
        // A kernel-hack could get around this by editing
        //   linux-src/kernel/ptrace.c#ptrace_attach()
        // and comment out the check for "same_thread_group(task, current)".
        //
        // The problem was discussed back in 2006 with a.o. Linus. See e.g.
        // https://lkml.org/lkml/2006/8/31/241.
        bt_backtrace(pr, thread_id);
#endif /* defined(VTSS_SW_OPTION_DEBUG) */

        return;
    }

    // Here, we must unwind the currently running thread's stack (local unwind).
    if ((symbols = libgcc_s_backtrace(array, NSYMS_MAX, &nsyms)) != nullptr) {
        bool printed_something = false;

        /* skip first stack frame (points here) */
        for (i = 0; i < nsyms && symbols[i]; i++) {
            pr("[bt]: (%d) %s\n", i, symbols[i]);
            printed_something = true;
        }

        if (printed_something) {
            pr("\n");
        }

        // Use free(), not VTSS_FREE()
        free(symbols);
    }
}
