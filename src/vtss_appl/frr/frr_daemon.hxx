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

#ifndef _FRR_DAEMON_HXX_
#define _FRR_DAEMON_HXX_

#include "frr.hxx"
#include <unistd.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/types.h>
#include <string>
#include <vtss/basics/optional.hxx>
#include <vtss/basics/parse_group.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <vtss/basics/vector.hxx>

bool frr_has_zebra(void);
bool frr_has_staticd(void);
bool frr_has_ospfd(void);
bool frr_has_ospf6d(void);
bool frr_has_ripd(void);
bool frr_has_router(void);

// Currently supported daemons.
typedef enum {
    FRR_DAEMON_TYPE_ZEBRA,
    FRR_DAEMON_TYPE_STATIC,
    FRR_DAEMON_TYPE_OSPF,
    FRR_DAEMON_TYPE_OSPF6,
    FRR_DAEMON_TYPE_RIP,
    FRR_DAEMON_TYPE_LAST,
} frr_daemon_type_t;

// Start a daemon. Nothing happens if the daemon is already started, despite the
// value of the clean flag.
mesa_rc frr_daemon_start(frr_daemon_type_t type, bool clean = false);

// Is a particular daemon started?
bool frr_daemon_started(frr_daemon_type_t type);

// Stop and start a daemon again, attempting to preserve the currently running
// configuration.
mesa_rc frr_daemon_reload(frr_daemon_type_t type);

// Stop a daemon.
// Notice that the daemon's cache is cleared when it is not running since the
// configuration is not saved in the daemon process.
// Zebra cannot be stopped.
mesa_rc frr_daemon_stop(frr_daemon_type_t type);

// Get running config for a particular daemon.
// It returns a cached version if it's less than 30 seconds ago it was last
// updated. Otherwise it reads the daemon directly (and caches the result). In
// order to stay up-to-date, it snoops all frr_daemon_cmd() calls for
// "configure terminal" and if seen, the cache is cleared.
mesa_rc frr_daemon_running_config_get(frr_daemon_type_t type, std::string &running_config);

// Send a command to a particular daemon
mesa_rc frr_daemon_cmd(frr_daemon_type_t type, vtss::Vector<std::string> cmds, std::string &result);

// Internal module init.
mesa_rc frr_daemon_init(vtss_init_data_t *data);

#endif // _FRR_DAEMON_HXX_

