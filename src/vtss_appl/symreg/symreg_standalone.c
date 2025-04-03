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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>            /* For va_list   */
#include "symreg_api.h"
#include "symreg_standalone.h" /* For ourselves */

/* Convert text to list or bit field */
mesa_rc mgmt_txt2list_bf(char *buf, BOOL *list, u32 min, u32 max, BOOL def, BOOL bf)
{
    u32   i, start = 0, n;
    char  *p, *end;
    BOOL  error, range = 0, comma = 0;

    /* Clear list by default */
    if (bf) {
        VTSS_BF_CLR(list, max + 1);
    } else {
        for (i = min; i <= max; i++) {
            list[i] = 0;
        }
    }

    p = buf;
    error = (p == NULL);
    while (p != NULL && *p != '\0') {
        /* Read integer */
        n = strtoul(p, &end, 0);
        if (end == p) {
            error = 1;
            break;
        }
        p = end;

        /* Check legal range */
        if (n < min || n > max) {
            error = 1;
            break;
        }

        if (range) {
            /* End of range has been read */
            if (n < start) {
                error = 1;
                break;
            }
            for (i = start ; i <= n; i++) {
                if (bf) {
                    VTSS_BF_SET(list, i, 1);
                } else {
                    list[i] = 1;
                }
            }
            range = 0;
        } else if (*p == '-') {
            /* Start of range has been read */
            start = n;
            range = 1;
            p++;
        } else {
            /* Single value has been read */
            if (bf) {
                VTSS_BF_SET(list, n, 1);
            } else {
                list[n] = 1;
            }
        }

        comma = 0;
        if (!range && *p == ',') {
            comma = 1;
            p++;
        }
    }

    /* Check for trailing comma/dash */
    if (comma || range) {
        error = 1;
    }

    /* Restore defaults if error */
    if (error) {
        if (bf) {
            memset(list, def ? 0xff : 0x00, VTSS_BF_SIZE(max + 1));
        } else {
            for (i = min; i <= max; i++) {
                list[i] = def;
            }
        }
    }
    return (error ? VTSS_RC_ERROR : VTSS_RC_OK);
}

mesa_rc mgmt_txt2bf(char *buf, BOOL *list, u32 min, u32 max, BOOL def)
{
    return mgmt_txt2list_bf(buf, list, min, max, def, 1);
}

/* Convert list to text
 * The resulting buf will contain numbers in range [min + 1; max + 1]
 * or [min; max] if #one_based is FALSE.
 * The case where #one_based == TRUE is a legacy case, that actually
 * never should have been implemented, or implemented differently.
 * It should have been using iport2uport(i) and iport2uport(i - 1)
 */
static char *list2txt(BOOL *list, int min, int max, char *buf, BOOL bf, BOOL one_based)
{
    int  i, count = 0;
    BOOL member, first = TRUE;
    char *p;

    p = buf;
    *p = '\0';

    for (i = min; i <= max; i++) {
        member = bf ? VTSS_BF_GET(list, i) : list[i];
        if ((member && (count == 0 || i == max)) || (!member && count > 1)) {
            p += sprintf(p, "%s%d", first ? "" : count > (member ? 1 : 2) ? "-" : ",", one_based ? (member ? i + 1 : i) : (member ? i : i - 1));
            first = FALSE;
        }

        if (member) {
            count++;
        } else {
            count = 0;
        }
    }

    return buf;
}

/* Convert bitfield list to text
 * The resulting buf will contain numbers in range [min; max]
 */
char *mgmt_bf2txt(BOOL *list, int min, int max, char *buf)
{
    return list2txt(list, min, max, buf, 1, 0);
}

void symreg_printf(BOOL debug, const char *fmt, ...)
{
    va_list args;

#if defined(DEBUG)
    BOOL print = TRUE;
#else
    BOOL print = !debug;
#endif

    if (!print) {
        return;
    }

    va_start(args, fmt);
    (void)vfprintf(stderr, fmt, args);
    va_end(args);
    (void)fprintf(stderr, "\n");
}

/******************************************************************************/
// usage()
/******************************************************************************/
static void usage(char *prog_name)
{
    printf("Usage: %s <reg_syntax>\n\n", prog_name);
    printf("where <reg_syntax> is a statement on the form\n");
    printf("  target[t]:reggrp[g]:reg[r]\n");
    printf("  where\n");
    printf("    'target' is the name of the target (e.g. dev).\n");
    printf("    'reggrp' is the name of the register group. May be omitted.\n");
    printf("    'reg'    is the name of the register.\n");
    printf("    t        is a list of target replications if applicable.\n");
    printf("    g        is a list of register group replications if applicable.\n");
    printf("    r        is a list of register replications if applicable.\n");
    printf("  If a given replication (t, g, r) is omitted, all applicable replications will be accessed.\n");
    printf("  If reggrp is omitted, add two consecutive colons.\n");
    printf("  Except for the above exception, matches will be exact, but wildcards (*, ?)\n");
    printf("  are allowed.\n");
    printf("  Example 1. dev1g[0-7,12]::dev_ptp_tx_id[0-3]\n");
    printf("  Example 2. dev1g::dev_rst_ctrl\n");
    printf("  Example 3: ana_ac:aggr[10]:aggr_cfg\n");
    printf("  Example 4: ana_ac::aggr_cfg\n");
    printf("  Example 5: ::MAC_ENA_CFG\n");
    printf("  Example 6: ::*cfg[3]\n");
}

/******************************************************************************/
// main()
/******************************************************************************/
int main(int argc, char *argv[])
{
    mesa_rc rc;
    void    *handle;
    u32     max_width, reg_cnt, addr;
    BOOL    next = FALSE;
    char    *name = NULL;
    int     result = 0;

    if (argc != 2) {
        fprintf(stderr, "%% Invalid number of arguments.\n");
        usage(argv[0]);
        return -1;
    }

    if ((rc = symreg_query_init(&handle, argv[1], &max_width, &reg_cnt)) != VTSS_RC_OK) {
        fprintf(stderr, "Error: %s\n", symreg_error_txt(rc));
        return -1;
    }

    printf("Number of registers = %u\n", reg_cnt);
    printf("Maximum width = %u\n", max_width);

    if ((name = VTSS_MALLOC(max_width + 1)) == NULL) {
        fprintf(stderr, "Out of memory while attempting to allocate %u bytes\n", max_width + 1);
        result = -1;
        goto do_exit;
    }

    printf("%-*s %-10s\n", max_width, "Register", "Address");
    while ((rc = symreg_query_next(handle, name, &addr, next)) == VTSS_RC_OK) {
        next = TRUE;
        printf("%-*s 0x%08x\n", max_width, name, addr);
    }

    if (rc != SYMREG_RC_NO_MORE_REGS) {
        fprintf(stderr, "symreg_query_next() failed with error = %s\n", symreg_error_txt(rc));
        result = -1;
        goto do_exit;
    }

do_exit:
    if ((rc = symreg_query_uninit(handle)) != VTSS_RC_OK) {
        fprintf(stderr, "Error: %s\n", symreg_error_txt(rc));
        return -1;
    }

    if (name) {
        VTSS_FREE(name);
    }

    return result;
}
