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

// Compile with
//   gcc -Wall bt.c -lunwind -lunwind-ptrace -lunwind-x86_64 -o bt
// or
//   gcc -Wall bt.c -lunwind -lunwind-ptrace -lunwind-mips -o bt
//
// Must run as root.
#define BT_VER "0.1"

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define UNW_REMOTE_ONLY
#include <libunwind.h>
#include <libunwind-ptrace.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#if defined(__mips__)
#define HAS_GET_IMAGE_NAME 1
#else
#define HAS_GET_IMAGE_NAME 0
#endif

#define BT_IMAGE_LEN_MAX 512
#define BT_PROC_LEN_MAX  512
#define BT_LINE_LEN_MAX (BT_IMAGE_LEN_MAX + 1 + BT_PROC_LEN_MAX + 1 + (2 + sizeof(void *) * 2) + 1 + 1 + (2 + sizeof(void *) * 2) + 1 + 1) /* Format: image(proc+0x01234567) [0x89abcdef]'\0' */

typedef int BOOL;
#define FALSE 0
#define TRUE 1

/******************************************************************************/
// BT_step()
/******************************************************************************/
static int BT_step(unw_cursor_t *cursor, char **result, void **array, int size, BOOL skip_first)
{
    unw_word_t offset, pc;
    char       proc[BT_PROC_LEN_MAX];
    int        cnt = 0;
    char       image[BT_IMAGE_LEN_MAX];

    // Unwind frames one by one, going up the frame stack.
    while (unw_step(cursor) > 0 && cnt < size) {
        unw_get_reg(cursor, UNW_REG_IP, &pc);
        if (pc == 0) {
            // Done
            break;
        }

        if (skip_first) {
            // When doing local unwind, the first entry points to ourselves, so skip it
            skip_first = FALSE;
            continue;
        }

        array[cnt] = (void *)(intptr_t)pc;

        image[sizeof(image) - 1] = '\0';
#if HAS_GET_IMAGE_NAME
        if (unw_get_image_name(cursor, image, sizeof(image) - 1) != 0)
#endif
        {
            snprintf(image, sizeof(image) - 1, "<UnknownImage>");
        }

        proc[sizeof(proc) - 1] = '\0';
        if (unw_get_proc_name(cursor, proc, sizeof(proc) - 1, &offset) == 0) {
            snprintf(result[cnt], BT_LINE_LEN_MAX - 1, "%s(%s+%p) [%p]", image, proc, (void *)(intptr_t)offset, array[cnt]);
        } else {
            snprintf(result[cnt], BT_LINE_LEN_MAX - 1, "%s [%p]", image, array[cnt]);
        }

        cnt++;
    }

    return cnt;
}

/******************************************************************************/
// BT_remote()
/******************************************************************************/
static int BT_remote(char **result, void **array, int size, pid_t tid)
{
    unw_addr_space_t unw_addr_space;
    unw_cursor_t     cursor;
    int              cnt = 0;
    struct UPT_info  *upt_info;

    if ((unw_addr_space = unw_create_addr_space(&_UPT_accessors, 0)) == NULL) {
        fprintf(stderr, "unw_create_addr_space() failed\n");
        goto out;
    }

    if ((upt_info = (struct UPT_info *)_UPT_create(tid)) == NULL) {
        fprintf(stderr, "_UPT_create() failed\n");
        goto out;
    }

    if (ptrace(PTRACE_ATTACH, tid, 0, 0) != 0) {
        fprintf(stderr, "Error: ptrace(ATTACH, tid = %d) failed with errno = %d = %s\n", tid, errno, strerror(errno));
        goto destroy;
    }

    // Wait for the thread that we are backtracing to make a system call
    if (waitpid(tid, NULL, __WALL) <= 0) {
        fprintf(stderr, "Error: Wait for %d to make a system call failed (%s)\n", tid, strerror(errno));
    }

    if (unw_init_remote(&cursor, unw_addr_space, upt_info) != 0) {
        fprintf(stderr, "Error: unw_init_remote(%d) failed with errno = %d = %s\n", tid, errno, strerror(errno));
        goto detach;
    }

    cnt = BT_step(&cursor, result, array, size, FALSE);

detach:
    if (ptrace(PTRACE_DETACH, tid, 0, 0)) {
        fprintf(stderr, "Error: ptrace(DETACH, tid = %d) failed with errno = %d = %s\n", tid, errno, strerror(errno));
    }

destroy:
    _UPT_destroy(upt_info);

out:
    return cnt;
}

/******************************************************************************/
// BT_local()
/******************************************************************************/
static int BT_local(char **result, void **array, int size)
{
    unw_cursor_t  cursor;
    unw_context_t context;
    int           cnt;

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    cnt = BT_step(&cursor, result, array, size, TRUE);

    return cnt;
}

/******************************************************************************/
// BT()
/******************************************************************************/
static char **BT_do(void **array, int size, int *cnt, pid_t tid)
{
    int  i;
    char **result, *str;

    *cnt = 0;

    if ((result = (char **)calloc(1, size * (sizeof(char *) + BT_LINE_LEN_MAX))) == NULL) {
        return result;
    }

    // The first size entries in result are pointers to the actual strings - also embeded in result.
    // Make str point to the very first char of the very first result string
    str = (char *)result + size * sizeof(char *);
    for (i = 0; i < size; i++) {
        result[i] = &str[i * BT_LINE_LEN_MAX];
    }

    if (tid && tid != syscall(SYS_gettid)) {
        *cnt = BT_remote(result, array, size, tid);
    } else {
        *cnt = BT_local(result, array, size);
    }

    if (*cnt > 0) {
        return result;
    } else {
        free(result);
        return NULL;
    }
}

/******************************************************************************/
// BT()
/******************************************************************************/
static void BT(pid_t tid)
{
    const int NSYMS_MAX = 100;
    void      *array[NSYMS_MAX];
    char      **symbols;
    int       i, nsyms;

    if ((symbols = BT_do(array, NSYMS_MAX, &nsyms, tid)) != NULL) {
        BOOL printed_something = FALSE;

        // Skip first stack frame (points here)
        for (i = 0; i < nsyms && symbols[i]; i++) {
            printf("[bt]: (%d) %s\n", i, symbols[i]);
            printed_something = TRUE;
        }

        if (printed_something) {
            printf("\n");
        }

        free(symbols);
    }
}

/******************************************************************************/
// print_ver()
/******************************************************************************/
static void print_ver(const char *app_name)
{
    printf("%s v" BT_VER "\n", app_name);
}

/******************************************************************************/
// usage()
/******************************************************************************/
static void usage(const char *app_name, const char *err_str)
{
    if (err_str) {
        printf("Error: %s\n\n", err_str);
    }

    printf("Usage: %s [options] <process-id>\n", app_name);
    printf("where [options] is one of\n");
    printf("  -h, --help:    Show this help text and exit\n");
    printf("  -v, --version: Show version and exit\n");
    printf("and\n");
    printf("  <process-id> is the pid or tid of the process or thread to backtrace (> 0).\n\n");
}

/******************************************************************************/
// main()
/******************************************************************************/
int main(int argc, char *argv[])
{
    int i, tid;

    if (argc < 2) {
        usage(argv[0], "Insufficient number of arguments");
        return -1;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0], NULL);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_ver(argv[0]);
            return 0;
        } else if (i == argc - 1 && (tid = atoi(argv[i])) > 0) {
            BT(tid);
        } else {
            usage(argv[0], "Invalid argument");
            return -1;
        }
    }

    return 0;
}

