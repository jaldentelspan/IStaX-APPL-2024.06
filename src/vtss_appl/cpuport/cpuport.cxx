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

#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include <vtss/basics/trace.hxx>
#include <vtss/basics/time_unit.hxx>
#include "vtss/basics/notifications/event-handler.hxx"
#include "vtss/basics/notifications/event.hxx"
#include "vtss/basics/notifications/timer.hxx"

#include "subject.hxx"
#include "cpuport_trace.h"
#include "cpuport_expose.hxx"
#include "cpuport_api.hxx"
#include "vtss/appl/module_id.h"
#include "vtss/appl/ip.h"
#include "board_if.h"
#include "port_expose.hxx" /* For port_status_update */

#ifdef VTSS_SW_OPTION_ICLI
#include "cpuport_icfg.h"
#endif

#define INTERFACE_NAMESIZE 64

static vtss_trace_reg_t trace_reg = {VTSS_MODULE_ID_CPUPORT, "cpuport", "CPUPORT"};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

const char *cpuport_error_txt(mesa_rc error)
{
    switch (error) {
    case VTSS_RC_OK:
        return "CPUPORT: OK";
    }
    return "CPUPORT: Undefined error code";
}

using namespace vtss;
struct vtss_cpuport_conf_details_t {
    char interface_name[INTERFACE_NAMESIZE]; /**<Linux device name */
};

bool operator!=(const vtss_cpuport_conf_details_t &x, const vtss_cpuport_conf_details_t &y)
{
    return memcmp(&x, &y, sizeof(x)) != 0;
}

namespace vtss
{
namespace appl
{
namespace cpuport
{

CpuportConfiguration the_cpuport_config("the_cpuport_config", VTSS_MODULE_ID_CPUPORT);

// typedef TableStatus<expose::ParamKey<vtss_ifindex_t>,
//                     expose::ParamVal<::vtss_cpuport_conf_details_t>> CpuportConfDetails;
// CpuportConfDetails the_cpuport_conf_details("the_cpuport_conf_details", VTSS_MODULE_ID_CPUPORT);
vtss::Map<vtss_ifindex_t, vtss_cpuport_conf_details_t> the_cpuport_conf_details;

CpuportStatistics the_cpuport_statistics("the_cpuport_statistics", VTSS_MODULE_ID_CPUPORT);

}  // namespace cpuport
}  // namespace appl
}  // namespace vtss

using namespace vtss::appl::cpuport;

struct interface {
    int     index;
    bool    link;
    int     flags;      /* IFF_UP etc. */
    long    speed;      /* Mbps; -1 is unknown */
    int     duplex;     /* DUPLEX_FULL, DUPLEX_HALF, or unknown */
    char    name[IFNAMSIZ];
};

vtss::str cpuport_os_interface_name(vtss::Buf *b, vtss_ifindex_t ifidx)
{
    vtss::BufPtrStream ss(b);
    vtss_cpuport_conf_details_t conf_details;
    VTSS_TRACE(ERROR) << "Got index: " << ifidx;
    //    if (the_cpuport_conf_details.get(ifidx, &conf_details) == MESA_RC_OK) {
    if (the_cpuport_conf_details.find(ifidx) != the_cpuport_conf_details.end()) {
        conf_details = the_cpuport_conf_details[ifidx];
        VTSS_TRACE(ERROR) << "Found os interface " << conf_details.interface_name
                          << " for index: " << ifidx;
        ss << conf_details.interface_name;
        ss.push(0);
    }
    return vtss::str(ss.begin(), ss.end());
}

const char *cpuport_os_interface_name(vtss_ifindex_t ifidx)
{
    if (the_cpuport_conf_details.find(ifidx) != the_cpuport_conf_details.end()) {
        return the_cpuport_conf_details[ifidx].interface_name;
    }
    return NULL;
}

bool vtss_cpuport_is_interface(const char *ifname, int ifname_size, vtss_ifindex_t *ifidx)
{
    int size = (ifname_size < IFNAMSIZ - 1) ? ifname_size : IFNAMSIZ - 1;
    for (auto itr : the_cpuport_conf_details) {
        if (strncmp(itr.second.interface_name, ifname, size) == 0) {
            *ifidx = itr.first;
            return true;
        }
    }
    return false;
}

/*
 * Cpu iterator function.
 */
mesa_rc vtss_ifindex_iterator_cpu(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex)
{
    ::vtss_appl_port_conf_t conf;
    if (prev_ifindex) {
        *next_ifindex = *prev_ifindex;
        return the_cpuport_config.get_next(next_ifindex, &conf);
    } else {
        return the_cpuport_config.get_first(next_ifindex, &conf);
    }
}

static int get_interface_info(const int fd, struct ifreq *const ifr, struct interface *const info)
{
    struct ethtool_cmd  cmd;
    struct ethtool_value edata;
    /* Interface flags. */
    if (ioctl(fd, SIOCGIFFLAGS, ifr) == -1) {
        info->flags = 0;
    } else {
        info->flags = ifr->ifr_flags;
    }

    ifr->ifr_data = (char *)&cmd;
    cmd.cmd = ETHTOOL_GSET; /* "Get settings" */
    if (ioctl(fd, SIOCETHTOOL, ifr) == -1) {
        /* Unknown */
        info->speed = -1L;
        info->duplex = DUPLEX_UNKNOWN;
        info->link = false;
        return -1;
    }
    info->speed = ethtool_cmd_speed(&cmd);
    info->duplex = cmd.duplex;
    ifr->ifr_data = (char *)&edata;
    edata.cmd = ETHTOOL_GLINK; /* "Get settings" */
    if (ioctl(fd, SIOCETHTOOL, ifr) == 0) {
        info->link = (edata.data != 0);
    } else {
        info->link = 0;
    }

    return 0;
}

int get_interface_by_index(const int index, struct interface *const info)
{
    int             socketfd, result;
    struct ifreq    ifr;

    if (index < 1 || !info) {
        return errno = EINVAL;
    }

    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socketfd == -1) {
        return errno;
    }

    ifr.ifr_ifindex = index;
    if (ioctl(socketfd, SIOCGIFNAME, &ifr) == -1) {
        do {
            result = close(socketfd);
        } while (result == -1 && errno == EINTR);
        return errno = ENOENT;
    }

    info->index = index;
    strncpy(info->name, ifr.ifr_name, IFNAMSIZ - 1);
    info->name[IFNAMSIZ - 1] = '\0';

    result = get_interface_info(socketfd, &ifr, info);
    close(socketfd);
    return result;
}

int get_interface_by_name(const char *const name,
                          struct ifreq *const ifr,
                          struct interface *const info)
{
    int             socketfd, result;

    if (!name || !*name || !info) {
        return errno = EINVAL;
    }

    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socketfd == -1) {
        return errno;
    }

    strncpy(ifr->ifr_name, name, IFNAMSIZ - 1);
    if (ioctl(socketfd, SIOCGIFINDEX, ifr) == -1) {
        do {
            result = close(socketfd);
        } while (result == -1 && errno == EINTR);
        return errno = ENOENT;
    }

    info->index = ifr->ifr_ifindex;
    strncpy(info->name, name, IFNAMSIZ - 1);
    info->name[IFNAMSIZ - 1] = '\0';

    return socketfd;
}

mesa_rc vtss_cpuport_get_interface_flags(vtss_ifindex_t ifidx, short *flags)
{
    struct ifreq ifr;
    struct interface info;
    vtss_cpuport_conf_details_t conf_details = the_cpuport_conf_details[ifidx];
    int fd = get_interface_by_name(conf_details.interface_name, &ifr, &info);
    if (fd < 0) {
        return MESA_RC_ERROR;
    }

    /* Interface flags. */
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == -1) {
        close(fd);
        return MESA_RC_ERROR;
    }
    *flags = ifr.ifr_flags;
    close(fd);
    return MESA_RC_OK;
}

extern "C" int cpuport_icli_cmd_register();

meba_api_cpu_port_t *cpu_ports;

void update_interface_status(vtss_ifindex_t ifindex, const char *cpu_port)
{
    vtss_appl_port_status_t status = {};
    struct ifreq            ifr;
    struct interface        info;
    int                     fd;

    if ((fd = get_interface_by_name(cpu_port, &ifr, &info)) < 0) {
        return;
    }

    get_interface_info(fd, &ifr, &info);
    close(fd);

    status.link = info.link;
    status.fdx = (info.duplex == DUPLEX_FULL);
    status.speed = (!info.link) ? MESA_SPEED_UNDEFINED :
                   (info.speed ==    10) ? MESA_SPEED_10M   :
                   (info.speed ==   100) ? MESA_SPEED_100M  :
                   (info.speed ==  1000) ? MESA_SPEED_1G    :
                   (info.speed ==  2500) ? MESA_SPEED_2500M :
                   (info.speed ==  5000) ? MESA_SPEED_5G    :
                   (info.speed == 10000) ? MESA_SPEED_10G   :
                   (info.speed == 12000) ? MESA_SPEED_12G   :
                   (info.speed == 25000) ? MESA_SPEED_25G   : MESA_SPEED_UNDEFINED;
    port_status_update.set(ifindex, &status);
}

void update_interface_config(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *config)
{
    struct ifreq ifr;
    struct interface info;
    vtss_cpuport_conf_details_t conf_details = the_cpuport_conf_details[ifindex];
    int fd = get_interface_by_name(conf_details.interface_name, &ifr, &info);
    if (fd < 0) {
        return;
    }

    struct ethtool_cmd  cmd;
    /* Interface flags. */
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == -1) {
        close(fd);
        return;
    }
    if (!config->admin.enable) {
        ifr.ifr_flags &= ~IFF_UP;
    } else {
        ifr.ifr_flags |= IFF_UP;
    }
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) == -1) {
        T_E("Failed setting link up/down");
    }
    ifr.ifr_mtu = config->max_length;
    if (ioctl(fd, SIOCSIFMTU, &ifr) < 0) {
        T_E("Failed setting mtu size");
    }
    cmd.cmd = ETHTOOL_GSET; /* "Get settings" */
    //    cmd.advertising
    ifr.ifr_data = (char *)&cmd;
    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) {
        T_E("Failed active getting port configuration");
    }

    close(fd);
}

class StatusMonitor : public EventHandler
{
public:
    StatusMonitor() : EventHandler(&vtss::notifications::subject_main_thread), t(this)
    {
        t.set_period(seconds(1));
        t.set_repeat(true);
        vtss::notifications::subject_main_thread.timer_add(t, seconds(1));
    }

    void execute(Event *e)
    {
    }

    void execute(Timer *e)
    {
        uint32_t cpu_port_count = fast_cap(MEBA_CAP_CPU_PORTS_COUNT);
        vtss_ifindex_t ifindex;
        for (int i = 0; i < cpu_port_count; ++i) {
            mesa_rc rc = vtss_ifindex_from_cpu(i, &ifindex);
            if (rc != MESA_RC_OK) {
                T_E("Invalid cpu ifindex (%s)", ifindex);
            }
            update_interface_status(ifindex, cpu_ports[i].cpu_port);
        }
    }
private:
    Timer t;
};

struct ConfigHandler : public EventHandler {
    ConfigHandler() : EventHandler(&vtss::notifications::subject_main_thread),
        e_cpuport_configuration(this) {}
    void init()
    {
        mesa_rc rc;
        vtss_ifindex_t ifidx;
        vtss_appl_port_conf_t conf;

        for (rc = the_cpuport_config.get_first(&ifidx, &conf);
             rc == MESA_RC_OK;
             rc = the_cpuport_config.get_next(&ifidx, &conf)) {
            update_interface_config(ifidx, &conf);
            mesa_rc rc_ = vtss_appl_ip_if_conf_set(ifidx);
            if (rc_ != MESA_RC_OK) {
                VTSS_TRACE(ERROR) << "Failed creating interface:" << ifidx;
            }
        }
        the_cpuport_config.observer_new(&e_cpuport_configuration);
    }

    void execute(vtss::notifications::Event *e)
    {
        if (e == &e_cpuport_configuration) {
            T_D("%s: e_cpuport_configuration", __FUNCTION__);
            the_cpuport_config.observer_get(&e_cpuport_configuration, o_cpuport_configuration);
            for (auto i : o_cpuport_configuration.events) {
                vtss_appl_port_conf_t a_config;
                the_cpuport_config.get(i.first, &a_config);
                update_interface_config(i.first, &a_config);
            }
        }

    }
    vtss::notifications::Event e_cpuport_configuration;
    CpuportConfiguration::Observer o_cpuport_configuration;

};

struct ifaddrs *ifaddr;

static void CPUPORT_default_conf_get(vtss_appl_port_conf_t &conf)
{
    vtss_clear(conf);
    conf.admin.enable = true;
    conf.speed        = MESA_SPEED_AUTO;
    conf.fdx          = true;
    conf.media_type   = VTSS_APPL_PORT_MEDIA_CU;
    conf.max_length   = 1500;
}

static void initialize_cpuport_status(void)
{
    uint32_t                    cpu_port_count = fast_cap(MEBA_CAP_CPU_PORTS_COUNT);
    vtss_appl_port_conf_t       conf;
    vtss_cpuport_conf_details_t config_details;
    vtss_ifindex_t              ifindex;
    int                         i;

    if (!board_instance) {
        T_E("Invalid board instance");
        return;
    }

    cpu_ports = board_instance->api_cpu_port;

    if (cpu_port_count > 0 && !cpu_ports) {
        T_E("Inconsistent configuration");
        return;
    }

    CPUPORT_default_conf_get(conf);

    for (i = 0; i < cpu_port_count; ++i) {
        if (vtss_ifindex_from_cpu(i, &ifindex) != VTSS_RC_OK) {
            T_E("Invalid cpu index (%u)", i);
            continue;
        }

        update_interface_status(ifindex, cpu_ports[i].cpu_port);
        the_cpuport_config.set(ifindex, conf);

        vtss_clear(config_details);
        strncpy(config_details.interface_name, cpu_ports[i].cpu_port, INTERFACE_NAMESIZE);
        the_cpuport_conf_details[ifindex] = config_details;
    }
}

static mesa_rc CPUPORT_ifindex_valid(vtss_ifindex_t ifindex)
{
    vtss_ifindex_elm_t ife;

    if (!vtss_ifindex_is_cpu(ifindex)) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    if (ife.ordinal >= fast_cap(MEBA_CAP_CPU_PORTS_COUNT)) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_cpuport_conf_default_get(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *conf)
{
    if (conf == nullptr) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(CPUPORT_ifindex_valid(ifindex));

    CPUPORT_default_conf_get(*conf);
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_cpuport_conf_get(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *conf)
{
    VTSS_RC(CPUPORT_ifindex_valid(ifindex));

    return vtss::appl::cpuport::the_cpuport_config.get(ifindex, conf);
}

mesa_rc vtss_appl_cpuport_conf_set(vtss_ifindex_t ifindex, const vtss_appl_port_conf_t *conf)
{
    vtss_appl_port_conf_t local_conf;

    VTSS_RC(CPUPORT_ifindex_valid(ifindex));

    if (conf == nullptr) {
        return VTSS_RC_ERROR;
    }

    // Silently override some of the parameters.
    local_conf                 = *conf;
    local_conf.media_type      = VTSS_APPL_PORT_MEDIA_CU;
    local_conf.power_mode      = VTSS_PHY_POWER_NOMINAL;
    local_conf.force_clause_73 = false;
    local_conf.fec_mode        = VTSS_APPL_PORT_FEC_MODE_NONE;

    return vtss::appl::cpuport::the_cpuport_config.set(ifindex, local_conf);
}

mesa_rc vtss_appl_cpuport_status_get(vtss_ifindex_t ifindex, vtss_appl_port_status_t *status)
{
    VTSS_RC(CPUPORT_ifindex_valid(ifindex));

    if (status == nullptr) {
        return VTSS_RC_ERROR;
    }

    return port_status_update.get(ifindex, status);
}

mesa_rc vtss_appl_cpuport_statistics_get(vtss_ifindex_t ifindex, mesa_port_counters_t *statistics)
{
    VTSS_RC(CPUPORT_ifindex_valid(ifindex));

    if (statistics == nullptr) {
        return VTSS_RC_ERROR;
    }

    // Not implemented
    vtss_clear(*statistics);
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_cpuport_statistics_clear(vtss_ifindex_t ifindex)
{
    VTSS_RC(CPUPORT_ifindex_valid(ifindex));

    // Not implemented
    return VTSS_RC_OK;
}

static StatusMonitor *the_status_monitor;
static ConfigHandler *the_config_handler;

/* Initialize module */
mesa_rc cpuport_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_I("INIT");
        initialize_cpuport_status();
        break;

    case INIT_CMD_START:
        the_status_monitor = new StatusMonitor();
        the_config_handler = new ConfigHandler();
        the_config_handler->init();
#ifdef VTSS_SW_OPTION_ICFG
        if (cpuport_icfg_init() != VTSS_RC_OK) {
            T_D("Calling cpuport_icfg_init() failed");
        }
#endif
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

