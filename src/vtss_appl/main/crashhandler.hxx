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

#ifndef __VTSS_APPL_MAIN_CRASHHANDLER__
#define __VTSS_APPL_MAIN_CRASHHANDLER__

#define CRASHFILE_PATH "/switch/icfg/"
#define CRASHFILE "crashfile"

// Associate the common "crash-signals" with a backtrace action.
void vtss_crashhandler_setup();

// Write a line to the crash file
int crashfile_printf(const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));

// Callers of crashfile_printf() must close the file after they are done with
// writing to it, because otherwise, it's not possible to do a
// "more flash:crashfile" without a reboot.
void crashfile_close(void);

// Disable gdbserver by setting port == 0;
void vtss_crashhandler_gdbserver(int port);

// Start a GDB server and attach it to ourselves...
void vtss_gdb_server_start();

// Stop GDB server
void vtss_gdb_server_stop();

#endif
