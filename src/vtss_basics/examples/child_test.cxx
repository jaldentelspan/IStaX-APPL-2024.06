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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int opt;
    int long_lived = 0;
    //int verbose = 0;
    int quite = 0;
    setlinebuf(stdout);
    setlinebuf(stderr);

    while ((opt = getopt(argc, argv, "qvle:")) != -1) {
        switch (opt) {
        case 'l':
            long_lived = 1;
            break;
        case 'v':
            //verbose = 1;
            break;
        case 'e':
            break;

        case 'q':


        default: /* '?' */
            fprintf(stderr, "no help here\n");
            exit(EXIT_FAILURE);
        }
    }

    if (!quite) {
        pid_t p = getpid();
        pid_t pp = getppid();

        fprintf(stdout, "This is STDOUT Pid=%d, Parent-Pid=%d\n", p, pp);
        fprintf(stderr, "This is STDERR Pid=%d, Parent-Pid=%d\n", p, pp);

        fprintf(stdout, "Argv: %d\n", argc);
        for (int i = 0; i < argc; ++i) {
            fprintf(stdout, "  [%d] = %s\n", i, argv[i]);
        }
        fprintf(stdout, "\n");
    }

    if (long_lived) {
        while (1) {
            sleep(1);
            fprintf(stdout, "This is STDOUT\n");
            sleep(1);
            fprintf(stderr, "This is STDERR\n");
        }
    }

    exit(EXIT_SUCCESS);
}
