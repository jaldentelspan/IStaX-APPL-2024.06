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
#include "vtss/basics/notifications/event.hxx"
#include "vtss/basics/notifications/event-handler.hxx"
#include <vtss/basics/notifications/process-daemon.hxx>
#include "subject.hxx"

#include "dhcp6_relay_trace.h"

struct dhcp6_relay_agent {
    dhcp6_relay_agent(const char *name) 
            : agent_process(&vtss::notifications::subject_main_thread, name) 
    {
        agent_process.executable = "/usr/sbin/dhcrelay";
        agent_process.arguments.push_back("-6");
        agent_process.trace_stdout_conf(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_INFO);
        agent_process.trace_stderr_conf(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_ERROR);
    }
    ~dhcp6_relay_agent() {
        agent_process.adminMode(vtss::notifications::ProcessDaemon::DISABLE);
        agent_process.stop_and_wait(true);
    }

    vtss::notifications::ProcessDaemon agent_process;
};
