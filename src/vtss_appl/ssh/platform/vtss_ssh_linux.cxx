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

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "main.h"
#include "subject.hxx"
#include "vtss_ssh_api.h"
#include "vtss_ssh_trace.h"
#include "vtss/appl/ssh.h"
#include "vtss/basics/trace.hxx"
#include "vtss/basics/notifications/process-daemon.hxx"

// TODO:
// - No integration with auth module
// - No iCli shell

using namespace ::vtss::notifications;

Process process_keygen(&subject_main_thread, "ssh_keygen");
ProcessDaemon process_daemon(&subject_main_thread, "sshd");
Subject<bool> adminMode;

#define DROPBEAR_RSA_HOST_KEY VTSS_FS_FLASH_DIR "/dropbear_rsa_host_key"

struct ProcessHandler : public EventHandler {
    ProcessHandler()
        : EventHandler(&subject_main_thread),
          admin_mode_event(this),
          keygen(this)
    {
        // SSH Server should always be disabled in the constructor - the
        // default mode is applied in the INIT_CMD_ICFG_LOADING_PRE event.
        adminMode.set(false);
        adminMode.attach(admin_mode_event);

        // Configure the key generator process /////////////////////////////////
        process_keygen.executable = "/usr/bin/dropbearkey";

        // Generate rsa host key
        process_keygen.arguments.push_back("-t");
        process_keygen.arguments.push_back("rsa");

        // Placement of the host key
        process_keygen.arguments.push_back("-f");
        process_keygen.arguments.push_back(DROPBEAR_RSA_HOST_KEY);

        // Get events from the key generator
        process_keygen.status(keygen);

        // Configure the ssh daemon process ////////////////////////////////////
        process_daemon.executable = "/usr/sbin/dropbear";

        // When asking to stop, send SIGKILL instead of SIGTERM
        process_daemon.kill_policy(true);

        // Use the previouse generated host key
        process_daemon.arguments.push_back("-r");
        process_daemon.arguments.push_back(DROPBEAR_RSA_HOST_KEY);

        // Bind to port 22 (all interfaces)
        process_daemon.arguments.push_back("-p");
        process_daemon.arguments.push_back("22");

        // Disable local port forwarding
        process_daemon.arguments.push_back("-j");

        // Disable remote port forwarding
        process_daemon.arguments.push_back("-k");

        // Allow blank password logins
        process_daemon.arguments.push_back("-B");

        // Log to stderr rather than syslog
        process_daemon.arguments.push_back("-E");

        // Don't fork into background
        process_daemon.arguments.push_back("-F");

        // Disallow root logins
        process_daemon.arguments.push_back("-w");

        // Redirect stdout to the trace system
        process_daemon.trace_stdout_conf(VTSS_TRACE_MODULE_ID,
                                         VTSS_TRACE_GRP_DEFAULT,
                                         VTSS_TRACE_LVL_INFO);

        // Redirect stderr to the trace system
        process_daemon.trace_stderr_conf(VTSS_TRACE_MODULE_ID,
                                         VTSS_TRACE_GRP_DEFAULT,
                                         VTSS_TRACE_LVL_INFO);
    }

    void execute(Event *e)
    {
        bool has_rsa_key = false;
        auto m = adminMode.get(e, admin_mode_event);
        auto keygen_status = process_keygen.status(e, keygen);
        VTSS_TRACE(INFO) << "Admin mode: " << m
                         << " keygen state: " << keygen_status.state();

        // If this is a event generated from the key generator, then check if
        // the key was generated successfully.
        if (e == &keygen && keygen_status.state() == ProcessState::dead) {
            if (keygen_status.exit_value() != 0) {
                unlink(DROPBEAR_RSA_HOST_KEY);
                VTSS_TRACE(ERROR) << "Failed to generate host keys";
            } else {
                VTSS_TRACE(INFO) << "Host key generated successfully";
            }

            keygen_running = false;
        }

        // We do not need any keys to disable the key server
        if (!m) {
            VTSS_TRACE(INFO) << "Disable ssh";
            process_daemon.adminMode(ProcessDaemon::DISABLE);

            // We also need to kill all currently open sessions
            system("killall -q -SIGKILL dropbear");

            return;
        }

        // Wait for keygen to exit
        if (keygen_running) {
            VTSS_TRACE(DEBUG) << "Key generator already running";
            return;
        }

        // Check that we can find the host key
        struct stat st;
        int res = stat(DROPBEAR_RSA_HOST_KEY, &st);
        if (res == 0 && st.st_size > 0) {
            has_rsa_key = true;
        }

        if (has_rsa_key) {
            VTSS_TRACE(INFO) << "Host key found - starting the daemon";
            process_daemon.adminMode(ProcessDaemon::ENABLE);
        } else {
            VTSS_TRACE(INFO) << "No host key found - generate one";
            keygen_running = true;
            process_keygen.run();
        }
    }

    bool keygen_running = false;
    Event admin_mode_event;
    Event keygen;
} process_handler;

mesa_rc vtss_appl_ssh_conf_get(vtss_appl_ssh_conf_t *const ssh_cfg)
{
    VTSS_TRACE(DEBUG) << "vtss_appl_ssh_conf_get";
    ssh_cfg->mode = adminMode.get();
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_ssh_conf_set(const vtss_appl_ssh_conf_t *const ssh_cfg)
{
    VTSS_TRACE(DEBUG) << "vtss_appl_ssh_conf_set";

    adminMode.set(ssh_cfg->mode);
    return VTSS_RC_OK;
}

mesa_rc ssh_os_init(vtss_init_data_t *data)
{
    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid,
        data->flags);

    switch (data->cmd) {
    case INIT_CMD_CONF_DEF:
    case INIT_CMD_ICFG_LOADING_PRE:
        adminMode.set(SSH_MGMT_DEF_MODE);
        break;

    default:
        break;
    }

    T_D("exit");

    return VTSS_RC_OK;
}
