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

#include <vtss/basics/notifications/process-daemon.hxx>
#include <vtss/basics/notifications/subject-runner.hxx>

using namespace vtss;
using namespace vtss::notifications;

// An instance of the subject runner - use 'subject_main_thread' in SMBStaX
vtss::notifications::SubjectRunner main_thread("main", 0, true);

// Instantiate the process - 'test' is just the name used in traces
ProcessDaemon p(&main_thread, "test");

int main(int argc, char *argv[]) {
    // Associate the executable
    p.executable = CHILD_TEST;
    p.arguments.push_back("-l");

    // Only for SMBStaX
    //p.trace_stdout_conf(<trace groups>);

    // Setup the various timers and counters.
    p.max_restart_cnt(3);
    p.restart_sleep_time(seconds(15));
    p.reset_restart_cnt(seconds(120));
    p.adminMode(ProcessDaemon::ENABLE);

    // Start the subject runner - not needed for SMBStaX
    main_thread.run();  // will not return

    return 0;
}
