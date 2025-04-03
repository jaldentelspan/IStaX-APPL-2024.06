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

#include "frr_daemon.hxx"
#include "frr_utils.hxx"
#include "frr_trace.hxx"
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <initializer_list>
#include <vtss/basics/fd.hxx>
#include <vtss/basics/notifications/process-daemon.hxx>
#include <vtss/basics/notifications/subject-runner.hxx>
#include <vtss/basics/parse_group.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <vtss/basics/string-utils.hxx>
#include <vtss/basics/time.hxx>
#include <vtss/basics/vector.hxx>
#include <vtss_timer_api.h>
#include <vtss/appl/ip.h> // For vtss_appl_ip_if_exists()
#include "ip_api.h"       // For vtss_ip_if_callback_add()
#include "ip_utils.hxx"   // For the operator of mesa_ipv4_network_t
#include "subject.hxx"    // For subject_main_thread

static critd_t FRR_DAEMON_crit;

struct FRR_DAEMON_Lock {
    FRR_DAEMON_Lock(const char *file, int line)
    {
        critd_enter(&FRR_DAEMON_crit, file, line);
    }

    ~FRR_DAEMON_Lock()
    {
        critd_exit(&FRR_DAEMON_crit, __FILE__, 0);
    }
};

#define FRR_DAEMON_LOCK_SCOPE() FRR_DAEMON_Lock __frr_daemon_lock_guard__(__FILE__, __LINE__)
#define FRR_DAEMON_LOCK_ASSERT_LOCKED(_fmt_, ...) if (!critd_is_locked(&FRR_DAEMON_crit)) {T_EG(FRR_TRACE_GRP_DAEMON, _fmt_, ##__VA_ARGS__);}

/******************************************************************************/
// FRR_DAEMON_type_validate()
/******************************************************************************/
static mesa_rc FRR_DAEMON_type_validate(frr_daemon_type_t type)
{
    if (type == FRR_DAEMON_TYPE_ZEBRA || type == FRR_DAEMON_TYPE_STATIC || type == FRR_DAEMON_TYPE_OSPF || type == FRR_DAEMON_TYPE_OSPF6 || type == FRR_DAEMON_TYPE_RIP) {
        return VTSS_RC_OK;
    }

    return FRR_RC_DAEMON_TYPE_INVALID;
}

/******************************************************************************/
// FRR_DAEMON_type_to_str()
/******************************************************************************/
static const char *FRR_DAEMON_type_to_str(frr_daemon_type_t type)
{
    switch (type) {
    case FRR_DAEMON_TYPE_ZEBRA:
        return "Zebra";

    case FRR_DAEMON_TYPE_STATIC:
        return "Static";

    case FRR_DAEMON_TYPE_OSPF:
        return "OSPF";

    case FRR_DAEMON_TYPE_OSPF6:
        return "OSPF6";

    case FRR_DAEMON_TYPE_RIP:
        return "RIP";

    default:
        T_EG(FRR_TRACE_GRP_DAEMON, "Invalid daemon type (%d)", type);
        break;
    }

    return "<Unknown!>";
}

/******************************************************************************/
// FRR_DAEMON_file_exists()
/******************************************************************************/
static bool FRR_DAEMON_file_exists(const char *filename)
{
    struct stat buffer;
    bool        result = stat(filename, &buffer) == 0;

    T_DG(FRR_TRACE_GRP_DAEMON, "%s exists: %s", filename, result ? "Yes" : "No");

    return result;
}

/******************************************************************************/
// FRR_DAEMON_remove_file()
/******************************************************************************/
static void FRR_DAEMON_remove_file(const char *filename)
{
    if (::remove(filename)) {
        T_EG(FRR_TRACE_GRP_DAEMON, "Failed to delete file %s", filename);
    }
}

/******************************************************************************/
// FRR_DAEMON_copy_file()
/******************************************************************************/
static void FRR_DAEMON_copy_file(const char *src, const char *dst)
{
    vtss::FixedBuffer buf = vtss::read_file_into_buf(src);
    vtss::write_buf_into_file(buf.begin(), buf.size(), dst);
}

/******************************************************************************/
// FRR_DAEMON_write_to_file()
/******************************************************************************/
static ssize_t FRR_DAEMON_write_to_file(const char *filename, const char *data, size_t size)
{
    return vtss::write_buf_into_file(data, size, filename);
}

/******************************************************************************/
// FRR_DAEMON_pid_get()
/******************************************************************************/
static int FRR_DAEMON_pid_get(const char *filename)
{
    vtss::FixedBuffer buffer = vtss::read_file_into_buf(filename);
    int               result = std::atoi(buffer.begin());

    return result == 0 ? -1 : result;
}

/******************************************************************************/
// FRR_DAEMON_pid_exists()
/******************************************************************************/
static bool FRR_DAEMON_pid_exists(int pid_id)
{
    return ::kill(pid_id, 0) == 0;
}

/******************************************************************************/
// FRR_DAEMON_pid_kill()
/******************************************************************************/
static void FRR_DAEMON_pid_kill(int pid_id)
{
    ::kill(pid_id, 9);
}

/******************************************************************************/
// FRR_DAEMON_is_process_alive()
/******************************************************************************/
static bool FRR_DAEMON_is_process_alive(vtss::notifications::ProcessDaemon *process)
{
    return process->status().alive();
}

/******************************************************************************/
// FrrDaemonState
/******************************************************************************/
struct FrrDaemonState {
    FrrDaemonState(frr_daemon_type_t type,
                   const char *name, const char *vty,
                   const char *bin, const char *pidfile, const char *conffile,
                   const char *runconffile)
        : is_started(false),
          type(type),
          name(name),
          vty(vty),
          bin(bin),
          pidfile(pidfile),
          conffile(conffile),
          runconffile(runconffile),
          process(&vtss::notifications::subject_main_thread, name)
    {
        // Figure out whether we can become enabled (has_daemon).
        has_daemon = FRR_DAEMON_file_exists(bin);

        switch (type) {
        case FRR_DAEMON_TYPE_ZEBRA:
        case FRR_DAEMON_TYPE_STATIC:
            break;

        case FRR_DAEMON_TYPE_OSPF:
#if !defined(VTSS_SW_OPTION_FRR_OSPF)
            // Compile-time disabled
            has_daemon = false;
#endif
            break;

        case FRR_DAEMON_TYPE_OSPF6:
#if !defined(VTSS_SW_OPTION_FRR_OSPF6)
            // Compile-time disabled
            has_daemon = false;
#endif
            break;

        case FRR_DAEMON_TYPE_RIP:
#if !defined(VTSS_SW_OPTION_FRR_RIP)
            // Compile-time disabled
            has_daemon = false;
#endif
            break;

        default:
            T_EG(FRR_TRACE_GRP_DAEMON, "Invalid type (%d)", type);
            break;
        }

        T_IG(FRR_TRACE_GRP_DAEMON, "Has %s daemon = %d", FRR_DAEMON_type_to_str(type), has_daemon);
    }

    mesa_rc           start(bool clean);
    mesa_rc           stop(void);
    bool              has_daemon;
    bool              is_started;
    frr_daemon_type_t type;
    const char        *name;
    const char        *vty;
    const char        *bin;
    const char        *pidfile;
    const char        *conffile;
    const char        *runconffile;
    time_t            last_running_config_update;
    std::string       running_config; // Cached running config. Only updated every so many seconds.
    vtss::notifications::ProcessDaemon process;
};

static const char *FRR_DAEMON_zebra_socket = "/tmp/zebra.socket";

// Make it possible to iterate over daemon types.
VTSS_ENUM_INC(frr_daemon_type_t);

/******************************************************************************/
// FrrDaemonState array
// Notes:
//   1. The 'name' parameter must be able to be used as a filename under the
//      '/tmp/' directory. After the daemon is stopped, the following files
//      are deleted (see details in FRR_DAEMON_process_stop()):
//      {".pid", ".log", ".socket", ".vty"}
//
//   2. For the Zebra daemon, we use the same parameter for 'conffile' and
//      'runconffile' since it never performs a 'clean' operation.
/******************************************************************************/
static FrrDaemonState FRR_DAEMON_states[] = {
    [FRR_DAEMON_TYPE_ZEBRA] = {
        FRR_DAEMON_TYPE_ZEBRA,
        "zebra",
        "/tmp/zebra.vty",
        "/usr/sbin/zebra",
        "/tmp/zebra.pid",
        "/etc/quagga/zebra.conf",
        "/etc/quagga/zebra.conf"
    },
    [FRR_DAEMON_TYPE_STATIC] = {
        FRR_DAEMON_TYPE_STATIC,
        "staticd",
        "/tmp/staticd.vty",
        "/usr/sbin/staticd",
        "/tmp/staticd.pid",
        "/etc/quagga/staticd.conf",
        "/tmp/staticd.conf"
    },
    [FRR_DAEMON_TYPE_OSPF] = {
        FRR_DAEMON_TYPE_OSPF,
        "ospfd",
        "/tmp/ospfd.vty",
        "/usr/sbin/ospfd",
        "/tmp/ospfd.pid",
        "/etc/quagga/ospfd.conf",
        "/tmp/ospfd.conf"
    },
    [FRR_DAEMON_TYPE_OSPF6] = {
        FRR_DAEMON_TYPE_OSPF6,
        "ospf6d",
        "/tmp/ospf6d.vty",
        "/usr/sbin/ospf6d",
        "/tmp/ospf6d.pid",
        "/etc/quagga/ospf6d.conf",
        "/tmp/ospf6d.conf"
    },
    [FRR_DAEMON_TYPE_RIP] = {
        FRR_DAEMON_TYPE_RIP,
        "ripd",
        "/tmp/ripd.vty",
        "/usr/sbin/ripd",
        "/tmp/ripd.pid",
        "/etc/quagga/ripd.conf",
        "/tmp/ripd.conf"
    },
};

/******************************************************************************/
// FRR_DAEMON_prepare()
/******************************************************************************/
static void FRR_DAEMON_prepare(void)
{
    frr_daemon_type_t type;

    T_IG(FRR_TRACE_GRP_DAEMON, "Preparing for first start");
    FRR_DAEMON_write_to_file("/proc/sys/net/ipv4/igmp_max_memberships", "200", 3);

    for (type = (frr_daemon_type_t)0; type < FRR_DAEMON_TYPE_LAST; type++) {
        FrrDaemonState &state = FRR_DAEMON_states[type];

        if (state.conffile == state.runconffile) {
            // This one doesn't have a special running conf file, so skip it.
            continue;
        }

        if (!state.has_daemon) {
            continue;
        }

        FRR_DAEMON_copy_file(state.conffile, state.runconffile);
    }
}

/******************************************************************************/
// FRR_DAEMON_check_condition()
/******************************************************************************/
template <typename F, typename... Args>
bool FRR_DAEMON_check_condition(vtss::seconds delay, F f, Args... args)
{
    static_assert(std::is_same<std::result_of_t<F(Args...)>, bool>::value == true, "Function doesn't return a bool");
    auto timeout = vtss::LinuxClock::now() + vtss::LinuxClock::to_time_t(delay);
    while (timeout > vtss::LinuxClock::now()) {
        if (f(args...)) {
            return true;
        }

        vtss::LinuxClock::sleep(vtss::milliseconds {200});
    }

    return false;
}

/******************************************************************************/
// FRR_DAEMON_process_stop()
/******************************************************************************/
static void FRR_DAEMON_process_stop(vtss::notifications::ProcessDaemon &process, const char *name)
{
    vtss::BufStream<vtss::SBuf32> buf;
    int                           pid;

    process.stop();
    if (FRR_DAEMON_check_condition(vtss::seconds {2}, FRR_DAEMON_is_process_alive, &process)) {
        process.stop(true);
    } else {
        T_DG(FRR_TRACE_GRP_DAEMON, "The %s process is still alive after called process.stop()", name);
    }

    buf << "/tmp/" << name << ".pid";
    if (FRR_DAEMON_file_exists(buf.cstring())) {
        pid = FRR_DAEMON_pid_get(buf.cstring());
        FRR_DAEMON_pid_kill(pid);
        for (const char *extension : {
                 ".pid", ".log", ".socket", ".vty"
             }) {
            buf.clear();
            buf << "/tmp/" << name << extension;
            if (FRR_DAEMON_file_exists(buf.cstring())) {
                FRR_DAEMON_remove_file(buf.cstring());
            }
        }
    }
}

/******************************************************************************/
// FRR_DAEMON_process_start()
/******************************************************************************/
static void FRR_DAEMON_process_start(vtss::notifications::ProcessDaemon &process, const std::string &executable, const std::initializer_list<std::string> &args)
{
    T_IG(FRR_TRACE_GRP_DAEMON, "Starting %s daemon", executable.c_str());
    process.executable = executable;
    process.arguments = vtss::Vector<std::string>(args);
    process.trace_stdout_conf(VTSS_MODULE_ID_FRR, FRR_TRACE_GRP_DAEMON, VTSS_TRACE_LVL_DEBUG);
    process.trace_stderr_conf(VTSS_MODULE_ID_FRR, FRR_TRACE_GRP_DAEMON, VTSS_TRACE_LVL_WARNING);
    process.kill_policy(true);
    process.adminMode(vtss::notifications::ProcessDaemon::ENABLE);
}

/******************************************************************************/
// FRR_DAEMON_do_start()
/******************************************************************************/
static mesa_rc FRR_DAEMON_do_start(frr_daemon_type_t type, bool clean)
{
    return FRR_DAEMON_states[type].start(type == FRR_DAEMON_TYPE_ZEBRA ? false : clean);
}

/******************************************************************************/
// FRR_DAEMON_do_stop()
/******************************************************************************/
static mesa_rc FRR_DAEMON_do_stop(frr_daemon_type_t type)
{
    return FRR_DAEMON_states[type].stop();
}

/******************************************************************************/
// FrrDaemonState::start()
//
// Before starting a daemon, check if the pid file exists, if it does, read the
// pid from it and see if that pid actually exists. If it does, return error, if
// not delete the pid and start.
/******************************************************************************/
mesa_rc FrrDaemonState::start(bool clean)
{
    T_IG(FRR_TRACE_GRP_DAEMON, "Starting %s", name);
    if (!has_daemon) {
        return FRR_RC_NOT_SUPPORTED;
    }

    if (is_started) {
        // Quit silently when it is started
        return VTSS_RC_OK;
    }

    if (type != FRR_DAEMON_TYPE_ZEBRA) {
        // This daemon, which is not Zebra, relies on Zebra. So make sure Zebra
        // is started first.
        FRR_DAEMON_do_start(FRR_DAEMON_TYPE_ZEBRA, false);

        if (clean) {
            FRR_DAEMON_copy_file(conffile, runconffile);
        }
    }

    if (FRR_DAEMON_file_exists(pidfile)) {
        int pid_id = FRR_DAEMON_pid_get(pidfile);
        if (FRR_DAEMON_check_condition(vtss::seconds {2}, FRR_DAEMON_pid_exists, pid_id)) {
            is_started = true;
            return VTSS_RC_OK;
        } else {
            FRR_DAEMON_remove_file(pidfile);
            T_DG(FRR_TRACE_GRP_DAEMON, "The process ID of %s doesn't exist", name);
        }
    }

    FRR_DAEMON_process_start(process, bin, {"-f", runconffile, "-i", pidfile, "-P", "0", "-z", FRR_DAEMON_zebra_socket});

    /* Wait for the process ID existed */
    if (!FRR_DAEMON_check_condition(vtss::seconds {10}, FRR_DAEMON_file_exists, pidfile)) {
        T_DG(FRR_TRACE_GRP_DAEMON, "The process ID of %s is either not found or need more time to start", name);
        return VTSS_RC_ERROR;
    }

    /* Wait for the process VTY existed */
    if (!FRR_DAEMON_check_condition(vtss::seconds {2}, FRR_DAEMON_file_exists, vty)) {
        T_DG(FRR_TRACE_GRP_DAEMON, "The VTY of %s is not found", name);
        return VTSS_RC_ERROR;
    }

    is_started = true;
    return VTSS_RC_OK;
}

/******************************************************************************/
// FrrDaemonState::stop()
/******************************************************************************/
mesa_rc FrrDaemonState::stop(void)
{
    if (!is_started) {
        return VTSS_RC_OK;  // Quit silently when it is stopped
    }

    process.adminMode(vtss::notifications::ProcessDaemon::DISABLE);
    FRR_DAEMON_process_stop(process, name);
    is_started = false;

    // Clear cache after stopping process.
    running_config = "";
    return VTSS_RC_OK;
}

/******************************************************************************/
// FRR_DAEMON_end_of_message()
/******************************************************************************/
static bool FRR_DAEMON_end_of_message(const std::string &msg)
{
    // Checks the bytes 2,3 and 4 from end to be 0, first byte from end is the
    // return code
    if (msg.size() < 4) {
        return false;
    }

    return vtss::equal(msg.rbegin() + 1, msg.rbegin() + 4, "\0\0\0");
}

/******************************************************************************/
// FRR_DAEMON_create_socket()
/******************************************************************************/
static int FRR_DAEMON_create_socket(const char *socket_name)
{
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        T_EG(FRR_TRACE_GRP_DAEMON, "Create socket failed %s", socket_name);
        return s;
    }

    struct sockaddr_un saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, socket_name, sizeof(saddr.sun_path));

    if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        close(s);
        T_EG(FRR_TRACE_GRP_DAEMON, "Connect socket failed %s", socket_name);
        return -1;
    }

    return s;
}

/******************************************************************************/
// FRR_DAEMON_send_socket()
/******************************************************************************/
static ssize_t FRR_DAEMON_send_socket(int socket, const char *buff, size_t len)
{
    size_t transmitted = 0;
    while (transmitted < len) {
        size_t to_transmit = len < 4096 ? len : 4096;
        ssize_t tmp = send(socket, &buff[transmitted], to_transmit, 0);
        transmitted += tmp;
    }

    return transmitted;
}

/******************************************************************************/
// FRR_DAEMON_recv_socket()
/******************************************************************************/
static ssize_t FRR_DAEMON_recv_socket(int socket, void *buff, size_t len)
{
    return recv(socket, buff, len, 0);
}

/******************************************************************************/
// FRR_DAEMON_cmd_single()
/******************************************************************************/
static std::string FRR_DAEMON_cmd_single(int socket, const char *cmd)
{
    char        buf[4096];
    int         bytes;
    std::string result;

    T_NG(FRR_TRACE_GRP_DAEMON, "Send command: %s", cmd);
    FRR_DAEMON_send_socket(socket, cmd, strlen(cmd) + 1);
    while (true) {
        buf[0] = '\0';

        if ((bytes = FRR_DAEMON_recv_socket(socket, buf, sizeof(buf) - 1)) < 0) {
            T_EG(FRR_TRACE_GRP_DAEMON, "Socket error: %s", cmd);
            return std::string();
        }

        T_NG_HEX(FRR_TRACE_GRP_DAEMON, (unsigned char *)buf, bytes);
        T_NG(FRR_TRACE_GRP_DAEMON, "Received %d bytes", bytes);
        T_NG(FRR_TRACE_GRP_DAEMON, "Buff: %s", buf);

        result.append(buf, bytes);

        if (FRR_DAEMON_end_of_message(result)) {
            // Done
            break;
        }
    }

    // removes the end of message part
    result.erase(result.size() - 4, 4);
    return result;
}

/******************************************************************************/
// FRR_DAEMON_do_cmd()
/******************************************************************************/
static mesa_rc FRR_DAEMON_do_cmd(frr_daemon_type_t type, vtss::Vector<std::string> cmds, std::string &result)
{
    FrrDaemonState &state = FRR_DAEMON_states[type];
    std::string    buff;
    vtss::Fd       socket;

    T_DG(FRR_TRACE_GRP_DAEMON, "Executing the following commands on %s daemon", state.name);
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    for (std::string &cmd : cmds) {
        T_DG(FRR_TRACE_GRP_DAEMON, "  %s", cmd.c_str());
    }
#endif

    if (!state.has_daemon) {
        return FRR_RC_NOT_SUPPORTED;
    }

    if (!state.is_started) {
        return FRR_RC_DAEMON_NOT_STARTED;
    }

    socket = FRR_DAEMON_create_socket(state.vty);

    if (socket.raw() == -1) {
        T_EG(FRR_TRACE_GRP_DAEMON, "Unable to create socket %s while executing commands:", state.vty);
        for (std::string &cmd : cmds) {
            T_EG(FRR_TRACE_GRP_DAEMON, "  %s", cmd.c_str());
        }

        return FRR_RC_INTERNAL_ERROR;
    }

    FRR_DAEMON_cmd_single(socket.raw(), "enable\0");

    for (std::string &cmd : cmds) {
        // If command includes "configure terminal", our cached running config
        // is likely to become out of date, so we clear it.
        if (cmd.find("configure terminal") != std::string::npos) {
            T_DG(FRR_TRACE_GRP_DAEMON, "%s: Clearing cached running config", state.name);
            state.running_config = "";
        }

        std::string tmp = FRR_DAEMON_cmd_single(socket.raw(), cmd.c_str());
        if (!tmp.empty()) {
            buff += tmp;
        }
    }

    result = std::move(buff);

    T_DG(FRR_TRACE_GRP_DAEMON, "Result =\n%s", result.c_str());

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRR_DAEMON_running_config_do_get()
/******************************************************************************/
static mesa_rc FRR_DAEMON_running_config_do_get(frr_daemon_type_t type, std::string &result)
{
    FrrDaemonState &state = FRR_DAEMON_states[type];
    time_t         now    = vtss::uptime_seconds();
    mesa_rc        rc;

    if (!state.is_started) {
        state.running_config = "";
        result = "";
        return VTSS_RC_OK;
    }

    if (state.running_config == "" || now - state.last_running_config_update > 30) {
        vtss::Vector<std::string> cmds = vtss::Vector<std::string> {"show running-config"};

        T_IG(FRR_TRACE_GRP_DAEMON, "%s: Cached running-config out of date. Updating", FRR_DAEMON_type_to_str(type));

        if ((rc = FRR_DAEMON_do_cmd(type, cmds, state.running_config)) != VTSS_RC_OK) {
            T_IG(FRR_TRACE_GRP_DAEMON, "%s: Update of running-config failed: %s", FRR_DAEMON_type_to_str(type), error_txt(rc));
            state.running_config = "";
            return rc;
        }

        // Update time.
        state.last_running_config_update = now;
    }

    result = state.running_config;
    return VTSS_RC_OK;
}

/******************************************************************************/
// FRR_DAEMON_ip_vlan_interface_callback()
// Invoked by IP module whenever an IP interface is added or deleted.
/******************************************************************************/
static void FRR_DAEMON_ip_vlan_interface_callback(vtss_ifindex_t ifindex)
{
    vtss::FrrRes<std::string> ifname = frr_util_os_ifname_get(ifindex);
    vtss::Vector<std::string> cmds;
    vtss::StringStream        buf;
    std::string               vty_res;
    frr_daemon_type_t         type;
    mesa_rc                   rc;

    if (!vtss_ifindex_is_vlan(ifindex)) {
        return;
    }

    if (vtss_appl_ip_if_exists(ifindex)) {
        // Adding, not deleting it.
        return;
    }

    if (!ifname) {
        return;
    }

    FRR_DAEMON_LOCK_SCOPE();

    // Delete FRR interface when vlan interface is deleted.
    T_DG(FRR_TRACE_GRP_DAEMON, "Deleting I/F %s", ifindex);

    buf << "no interface " << ifname.val;
    cmds.emplace_back("configure terminal");
    cmds.emplace_back(vtss::move(buf.buf));

    for (type = (frr_daemon_type_t)0; type < FRR_DAEMON_TYPE_LAST; type++) {
        FrrDaemonState &state = FRR_DAEMON_states[type];

        if (type == FRR_DAEMON_TYPE_ZEBRA) {
            // Skip Zebra
            continue;
        }

        if (!state.is_started) {
            // Daemon not started. Go on.
            continue;
        }

        if ((rc = FRR_DAEMON_do_cmd(type, cmds, vty_res)) != VTSS_RC_OK) {
            T_EG(FRR_TRACE_GRP_DAEMON, "Delete of I/F %s in daemon %s failed: %s", ifindex, FRR_DAEMON_type_to_str(type), error_txt(rc));
        }
    }
}

/******************************************************************************/
// frr_has_zebra()
/******************************************************************************/
bool frr_has_zebra(void)
{
    return FRR_DAEMON_states[FRR_DAEMON_TYPE_ZEBRA].has_daemon;
}

/******************************************************************************/
// frr_has_staticd()
/******************************************************************************/
bool frr_has_staticd(void)
{
    return FRR_DAEMON_states[FRR_DAEMON_TYPE_STATIC].has_daemon;
}

/******************************************************************************/
// frr_has_ospfd()
/******************************************************************************/
bool frr_has_ospfd(void)
{
    return FRR_DAEMON_states[FRR_DAEMON_TYPE_OSPF].has_daemon;
}

/******************************************************************************/
// frr_has_ospf6d()
/******************************************************************************/
bool frr_has_ospf6d(void)
{
    return FRR_DAEMON_states[FRR_DAEMON_TYPE_OSPF6].has_daemon;
}

/******************************************************************************/
// frr_has_ripd()
/******************************************************************************/
bool frr_has_ripd(void)
{
    return FRR_DAEMON_states[FRR_DAEMON_TYPE_RIP].has_daemon;
}

/******************************************************************************/
// frr_has_router()
/******************************************************************************/
bool frr_has_router()
{
#if defined(VTSS_SW_OPTION_FRR_ROUTER)
    // The Router module should be enabled for all FRR supported protocols.
    // TODO: Currently only RIP is needed.
    return FRR_DAEMON_states[FRR_DAEMON_TYPE_RIP].has_daemon;
#else
    return false;
#endif
}

/******************************************************************************/
// frr_daemon_start()
/******************************************************************************/
mesa_rc frr_daemon_start(frr_daemon_type_t type, bool clean)
{
    T_IG(FRR_TRACE_GRP_DAEMON, "Starting %s daemon", FRR_DAEMON_type_to_str(type));

    FRR_DAEMON_LOCK_SCOPE();

    return FRR_DAEMON_do_start(type, clean);
}

/******************************************************************************/
// frr_daemon_started()
/******************************************************************************/
bool frr_daemon_started(frr_daemon_type_t type)
{
    if (FRR_DAEMON_type_validate(type) != VTSS_RC_OK) {
        return false;
    }

    FRR_DAEMON_LOCK_SCOPE();

    return FRR_DAEMON_states[type].is_started;
}

/******************************************************************************/
// frr_daemon_reload()
/******************************************************************************/
mesa_rc frr_daemon_reload(frr_daemon_type_t type)
{
    std::string running_config;
    size_t      written_size;

    VTSS_RC(FRR_DAEMON_type_validate(type));

    FRR_DAEMON_LOCK_SCOPE();

    // Make sure, we get the latest running config by clearing the cache.
    FRR_DAEMON_states[type].running_config = "";

    // Get new running config
    VTSS_RC(FRR_DAEMON_running_config_do_get(type, running_config));

    VTSS_RC(FRR_DAEMON_do_stop(type));

    if ((written_size = FRR_DAEMON_write_to_file(FRR_DAEMON_states[type].runconffile, running_config.c_str(), running_config.size())) != running_config.size()) {
        return VTSS_RC_ERROR;
    }

    return FRR_DAEMON_do_start(type, false);
}

/******************************************************************************/
// frr_daemon_stop()
/******************************************************************************/
mesa_rc frr_daemon_stop(frr_daemon_type_t type)
{
    VTSS_RC(FRR_DAEMON_type_validate(type));

    FRR_DAEMON_LOCK_SCOPE();

    T_IG(FRR_TRACE_GRP_DAEMON, "Stopping %s daemon", FRR_DAEMON_type_to_str(type));

    return FRR_DAEMON_do_stop(type);
}

/******************************************************************************/
// frr_daemon_cmd()
/******************************************************************************/
mesa_rc frr_daemon_cmd(frr_daemon_type_t type, vtss::Vector<std::string> cmds, std::string &result)
{
    VTSS_RC(FRR_DAEMON_type_validate(type));

    FRR_DAEMON_LOCK_SCOPE();

    return FRR_DAEMON_do_cmd(type, cmds, result);
}

/******************************************************************************/
// frr_daemon_running_config_get()
/******************************************************************************/
mesa_rc frr_daemon_running_config_get(frr_daemon_type_t type, std::string &running_config)
{
    FRR_DAEMON_LOCK_SCOPE();
    return FRR_DAEMON_running_config_do_get(type, running_config);
}

extern "C" int frr_daemon_icli_cmd_register();

/******************************************************************************/
// frr_daemon_init()
/******************************************************************************/
mesa_rc frr_daemon_init(vtss_init_data_t *data)
{
    mesa_rc rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        critd_init(&FRR_DAEMON_crit, "frr_daemon.crit", VTSS_MODULE_ID_FRR, CRITD_TYPE_MUTEX);
        frr_daemon_icli_cmd_register();
        FRR_DAEMON_prepare();
        break;

    case INIT_CMD_START:
        // Start the Zebra daemon right away. Zebra takes care of gathering all
        // routes from the other daemons.
        if ((rc = frr_daemon_start(FRR_DAEMON_TYPE_ZEBRA)) != VTSS_RC_OK) {
            T_EG(FRR_TRACE_GRP_DAEMON, "Unable to start Zebra daemon: %s", error_txt(rc));
        }

        // If we have the staticd daemon, it means that we are running FRR
        // v6.0 or higher, which in turn indicates that static routes must be
        // installed through this daemon. Otherwise, they must be installed
        // directly through the Zebra daemon.
        if (frr_has_staticd()) {
            if ((rc = frr_daemon_start(FRR_DAEMON_TYPE_STATIC)) != VTSS_RC_OK) {
                T_EG(FRR_TRACE_GRP_DAEMON, "Unable to start staticd daemon: %s", error_txt(rc));
            }
        }

        // Subscribe to VLAN-inteface changes.
        if ((rc = vtss_ip_if_callback_add(FRR_DAEMON_ip_vlan_interface_callback)) != VTSS_RC_OK) {
            T_EG(FRR_TRACE_GRP_DAEMON, "vtss_ip_if_callback_add() failed: %s", error_txt(rc));
        }

    default:
        break;
    }

    return VTSS_RC_OK;
}

