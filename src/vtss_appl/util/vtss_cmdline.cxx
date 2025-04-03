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

#include "main_types.h"
#include "vtss_cmdline.hxx"
#include "vtss_trace.h"

static char cmdline[1024] = "";
static unsigned int cmdline_arguments = 0;

static void vtss_cmdline_get()
{
    if (cmdline[0] == 0) {
        FILE* fp;
        if ( (fp = fopen ("/proc/cmdline", "r")) == NULL) {
            T_D("fopen /proc/cmdline FAILED! [%s]\n", strerror(errno));
            return;
        }
        if ( (fgets(cmdline, sizeof(cmdline), fp)) == NULL) {
            T_D("fgets FAILED! [%s]\n", strerror(errno));
            fclose(fp);
            return;
        }
        /* cleanup */
        fclose(fp);
        unsigned int cmdline_size = strlen(cmdline);
        if (cmdline_size > 0) {
            cmdline_arguments++;
        }
        for (unsigned int i=0; i<cmdline_size; i++) {
            if (cmdline[i] == ' ') {
                cmdline[i] = 0;
                cmdline_arguments++;
            }
        }

    }
    return;
}

const char* vtss_cmdline_get(const char *key)
{
    int key_len=strlen(key);
    if (key_len == 0) {
        return nullptr;
    }

    vtss_cmdline_get();
    const char *pcmdline = cmdline;
    
    pcmdline = cmdline;
    for (unsigned int i=0; i<cmdline_arguments; i++, pcmdline+=strlen(pcmdline)+1) {
        const char *delimiter = strchr(pcmdline, '=');
        if (delimiter>pcmdline) {
            if (key_len != delimiter-pcmdline) {
                continue;
            }
            if (strncmp(key, pcmdline, key_len) != 0) {
                continue;                
            }
            return delimiter+1;
        }
    }
    return 0;
}
