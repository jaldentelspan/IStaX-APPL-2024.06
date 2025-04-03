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

#include <net/if.h>
#include "critd_api.h"
#include "vtss/appl/dhcp6_relay.h"
#include "ip_utils.hxx"
#include "ip_expose.hxx"
#include "dhcp6_relay_trace.h"
#include "dhcp6_relay.h"
#include "dhcp6_relay_agent.hxx"
#include "packet_parser.hxx"
#include "../dhcp6_client/base/include/vtss_dhcp6_type.hxx"
#include <sys/ioctl.h>


#include "vtss/basics/expose/table-status.hxx"
#include "vtss/basics/memcmp-operator.hxx"
#include "vtss/basics/notifications/event-type.hxx"
#include "vtss/basics/notifications/event.hxx"
#include "vtss/basics/notifications/event-handler.hxx"
#include "vtss/basics/notifications/subject.hxx"
#include "subject.hxx"

#ifdef VTSS_SW_OPTION_ICFG
#include "dhcp6_relay_icfg.h"
#endif

#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif /* VTSS_SW_OPTION_WEB */

#include "vtss_common_iterator.hxx"


#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif  // VTSS_SW_OPTION_SYSLOG


#include "vtss_tftp_api.h"

using namespace dhcp6_snooping;

/****************************************************************************/
/*  TRACE system                                                            */
/****************************************************************************/
static const char *ifidx2dbg_str(const vtss_ifindex_t *const idx)
{
    static char buf[16];
    return idx ? vtss_ifindex2str(buf, sizeof(buf), *idx) : "NONE";
}

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "dhcp6_relay", "DHCPv6 Relay"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define DHCP6_RELAY_CRIT_ENTER() critd_enter(&dhcp6_relay_crit, __FILE__, __LINE__)
#define DHCP6_RELAY_CRIT_EXIT()  critd_exit( &dhcp6_relay_crit, __FILE__, __LINE__)

/****************************************************************************/
/*  Global types */
/****************************************************************************/
struct vtss_appl_dhcpv6_relay_vlan_server_info_t
{
    struct sockaddr_in6 link;
    vtss_appl_dhcpv6_relay_vlan_server_info_t() {}
};

struct vtss_appl_dhcpv6_relay_vlan_index_key_t
{
    vtss_ifindex_t idx;
    vtss_ifindex_t relay_idx;

    vtss_appl_dhcpv6_relay_vlan_index_key_t(){}
    vtss_appl_dhcpv6_relay_vlan_index_key_t(vtss_ifindex_t if_idx, vtss_ifindex_t r_if_idx) : idx(if_idx), relay_idx(r_if_idx) {}

    void set_keys(vtss_ifindex_t if_idx, vtss_ifindex_t r_if_idx) {
        idx = if_idx;
        relay_idx = r_if_idx;
    }
};


/****************************************************************************/
/*  Global variables */
/****************************************************************************/

bool operator==(const vtss_dhcpv6_relay_agent &a, const vtss_dhcpv6_relay_agent &b)
{
    if (a.upper_interfaces.size() != b.upper_interfaces.size()) {
        return false;
    }
    auto i = a.upper_interfaces.begin();
    auto j = b.upper_interfaces.begin();
    while (i != a.upper_interfaces.end() && j != b.upper_interfaces.end()) {
        if (memcmp(&(*i), &(*j), sizeof(*i)) !=0) {
            return false;
        }
        i++;
        j++;
    }

    if (memcmp(&a.lower_interface, &b.lower_interface, sizeof(b.lower_interface)) != 0) {
        return false;
    }

    if (a.joined != b.joined) {
        return false;
    }
    return true;
}

bool operator!=(const vtss_dhcpv6_relay_agent &a, const vtss_dhcpv6_relay_agent &b)
{
    return !(a == b);
}

bool operator==(const vtss_appl_dhcpv6_relay_vlan_server_info_t &a, const vtss_appl_dhcpv6_relay_vlan_server_info_t &b)
{
    return (memcmp(&a.link, &b.link, sizeof(a.link)) == 0);
}

bool operator!=(const vtss_appl_dhcpv6_relay_vlan_server_info_t &a, const vtss_appl_dhcpv6_relay_vlan_server_info_t &b)
{
    return !(a == b);
}

bool operator<(const vtss_appl_dhcpv6_relay_vlan_index_key_t &a, const vtss_appl_dhcpv6_relay_vlan_index_key_t &b)
{
        if (memcmp(&a.idx, &b.idx, sizeof(a.idx)) < 0) {
            return true;
        }
        if (memcmp(&a.idx, &b.idx, sizeof(a.idx)) > 0) {
            return false;
        }
        if (memcmp(&a.relay_idx, &b.relay_idx, sizeof(a.relay_idx)) < 0) {
            return true;
        }
        return false;
}

bool operator ==(const vtss_appl_dhcpv6_relay_vlan_statistics_t &a, const vtss_appl_dhcpv6_relay_vlan_statistics_t &b)
{
    if (a.tx_to_server != b.tx_to_server) {
        return false;
    }
    if (a.rx_from_server != b.rx_from_server) {
        return false;
    }
    if (a.server_pkt_dropped != b.server_pkt_dropped) {
        return false;
    }
    if (a.tx_to_client != b.tx_to_client) {
        return false;
    }
    if (a.rx_from_client != b.rx_from_client) {
        return false;
    }
    if (a.client_pkt_dropped != b.client_pkt_dropped) {
        return false;
    }
    return true;
}

bool operator !=(const vtss_appl_dhcpv6_relay_vlan_statistics_t &a, const vtss_appl_dhcpv6_relay_vlan_statistics_t &b) {
    return !(a == b);
}

VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_dhcpv6_relay_vlan_t);

/* Critical region protection */
static critd_t dhcp6_relay_crit;  // Critical region for global variables

RelayAgent relay_agent("relay_agent", VTSS_MODULE_ID_DHCP6_RELAY);
Dhcp6RelayVlanConfiguration dhcp6_relay_vlan_configuration("dhcp6_relay_vlan_configuration", VTSS_MODULE_ID_DHCP6_RELAY);
Dhcp6RelayVlanStatus dhcp6_relay_vlan_status("dhcp6_relay_vlan_status", VTSS_MODULE_ID_DHCP6_RELAY);

/* Upstream server information */
vtss::Map<vtss_appl_dhcpv6_relay_vlan_index_key_t, vtss_appl_dhcpv6_relay_vlan_server_info_t *> dhcp6_relay_vlan_upstream_socket_info;

/* Downstream send information */
typedef vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamVal<int *>> Dhcp6RelayVlanAgentSocketFd;
Dhcp6RelayVlanAgentSocketFd dhcp6_relay_vlan_downstream_socket_fd("dhcp6_relay_vlan_downstream_socket_fd", VTSS_MODULE_ID_DHCP6_RELAY);

typedef vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamVal<vtss_appl_dhcpv6_relay_vlan_statistics_t *>> Dhcp6RelayStatistics;

Dhcp6RelayStatistics dhcp6_relay_statistics("dhcp6_relay_statistics", VTSS_MODULE_ID_DHCP6_RELAY);
int server_pkts_interface_missing = 0;

vtss::notifications::Subject<int> callbackSubject; // Value not used, just to signal callback from ip module

mesa_ipv6_t ipv6_zero;
mesa_ipv6_t ipv6_all_dhcp_servers = {
    0xFF, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x03};
mesa_ipv6_t ipv6_all_dhcp_relay_agents_and_servers = {
    0xFF, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x02};

/*************************************************************************
** Misc Functions
*************************************************************************/
const char *dhcp6_relay_error_txt(mesa_rc rc)
{
    switch (rc) {
        case DHCP6_RELAY_ERROR_INTERFACE:
            return "Invalid vlan interface";
        case DHCP6_RELAY_INVALID_RELAY_INTERFACE:
            return "Invalid relay interface";
        case DHCP6_RELAY_INVALID_RELAY_DESTINATION:
            return "Invalid relay destination";
        default:
            return "Generic ipv6 relay error";
    }
}

/**
 * \brief Iterate through all vlan for which a dhcpv6_relay is defined
 * \param in [IN]   Pointer to current vlan interface id. Provide a null
 *                  pointer to get the first interface id.
 * \param out [OUT] Next interface id (relative to the value provided in
 *                  'in').
 * \param relay_in  [IN]  Pointer to current relay vlan interface id. Provide a
 *                  null pointer to get the first dhcpv6 relay interface vlan id.
 * \param relay_out [OUT] Next relay vlan interface id (relative to the value provided in
 *                  'relay_in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" vlan id exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_dhcpv6_relay_vlan_conf_itr(const vtss_ifindex_t *const in,
                                             vtss_ifindex_t *const out,
                                             const vtss_ifindex_t *const relay_in,
                                             vtss_ifindex_t *const relay_out)
{
    T_D("vtss_appl_dhcpv6_relay_conf_itr: %s, %s",
        ifidx2dbg_str(in), ifidx2dbg_str(relay_in));

    if (out == nullptr || relay_out == nullptr) {
        return MESA_RC_ERROR;
    }

    vtss_appl_dhcpv6_relay_vlan_t conf;

    if (in) {
        *out = *in;
    } else {
        memset(out, 0, sizeof(*out));
    }

    if (relay_in) {
        *relay_out = *relay_in;
    } else {
        memset(relay_out,0,sizeof(*relay_out));
    }

    return dhcp6_relay_vlan_configuration.get_next(out, relay_out, &conf);
}

/* Dhcpv6_Relay functions ----------------------------------------------------- */

/**
 * \brief Get dhcpv6 relay configuration for a specific vlan
 * \param interface [IN] The vlan interface id
 * \param relay_in [IN] The relay vlan interface id.
 * \param conf [OUT] The dhcp relay configuration for the vlan id
 * \return Error code.
 */
mesa_rc vtss_appl_dhcpv6_relay_conf_get(vtss_ifindex_t interface,
                                        vtss_ifindex_t relay_in,
                                        vtss_appl_dhcpv6_relay_vlan_t *conf)
{
    T_D("vtss_appl_dhcpv6_relay_conf_itr: %s, %s",
        ifidx2dbg_str(&interface), ifidx2dbg_str(&relay_in));

    return dhcp6_relay_vlan_configuration.get(interface, relay_in, conf);
}

/**
 * \brief Set dhcp relay configuration for a specific vlan
 * \param interface [IN] The vlan interface id
 * \param relay_in [IN] The relay vlan interface id.
 * \param conf [IN] The dhcp relay configuration for the vlan interface
 * \return Error code.
 */
mesa_rc vtss_appl_dhcpv6_relay_conf_set(vtss_ifindex_t interface,
                                        vtss_ifindex_t relay_in,
                                        const vtss_appl_dhcpv6_relay_vlan_t *const conf)
{
    mesa_rc rc;
    vtss_appl_dhcpv6_relay_vlan_t status;
    T_D("vtss_appl_dhcpv6_relay_conf_set: %s, %s, %s",
        ifidx2dbg_str(&interface),
        ifidx2dbg_str(&relay_in),
        conf->relay_destination);

    VTSS_ASSERT(conf != nullptr);
    if (interface == relay_in) {
        return DHCP6_RELAY_INVALID_RELAY_INTERFACE;
    }
    if (!vtss_appl_ip_if_exists(interface)) {
        return DHCP6_RELAY_ERROR_INTERFACE;
    }
    if (conf->relay_destination==ipv6_zero) {
        return DHCP6_RELAY_INVALID_RELAY_DESTINATION;
    }

    rc = dhcp6_relay_vlan_configuration.set(interface, relay_in, conf);

    if (rc == VTSS_RC_OK) {
        /* If status table does not have an entry for this configuration, rc is returned. */
        if (dhcp6_relay_vlan_status.get(interface, relay_in, &status) != VTSS_RC_OK) {
            return rc;
        }

        if (dhcp6_relay_vlan_status.set(interface, relay_in, conf) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }
    }

    return rc;
}

/**
 * \brief Delete dhcp relay configuration for a specific vlan
 * \param interface [IN] The vlan interface id
 * \param relay_in [IN] The relay vlan interface id.
 * \return Error code.
 */
mesa_rc vtss_appl_dhcpv6_relay_conf_del(vtss_ifindex_t interface,
                                        vtss_ifindex_t relay_in)
{
    mesa_rc rc;
    T_D("vtss_appl_dhcpv6_relay_conf_del: %s, %s",
        ifidx2dbg_str(&interface),
        ifidx2dbg_str(&relay_in));
    rc = dhcp6_relay_vlan_configuration.del(interface, relay_in);
    if (rc == VTSS_RC_OK) {
        if (dhcp6_relay_statistics.del(interface, relay_in) == VTSS_RC_OK) {
            T_D("Statistics entry sucessfully deleted.");
        }
    }
    return rc;
}

mesa_rc vtss_appl_dhcpv6_relay_vlan_status_itr(const vtss_ifindex_t *const in,
                                               vtss_ifindex_t *const out,
                                               const vtss_ifindex_t *const relay_in,
                                               vtss_ifindex_t *const relay_out)
{
    T_D("vtss_appl_dhcpv6_relay_vlan_status_itr: %s, %s",
        ifidx2dbg_str(in), ifidx2dbg_str(relay_in));
    vtss_appl_dhcpv6_relay_vlan_t status;

    if (out == nullptr || relay_out == nullptr) {
        return MESA_RC_ERROR;
    }

    if (in) {
        *out = *in;
    } else {
        memset(out, 0, sizeof(*out));
    }

    if (relay_in) {
        *relay_out = *relay_in;
    }

    return dhcp6_relay_vlan_status.get_next(out, relay_out, &status);
}

mesa_rc vtss_appl_dhcpv6_relay_status_get(vtss_ifindex_t vid,
                                          vtss_ifindex_t relay_in,
                                          vtss_appl_dhcpv6_relay_vlan_t *status)
{
    T_D("vtss_appl_dhcpv6_relay_status_get: %s, %s",
        ifidx2dbg_str(&vid), ifidx2dbg_str(&relay_in));
    return dhcp6_relay_vlan_status.get(vid, relay_in, status);
}

mesa_rc vtss_dhcpv6_relay_status_clear(vtss_ifindex_t vid,
                                       vtss_ifindex_t relay_in)
{
    T_D("vtss_dhcpv6_relay_status_clear: %s, %s",
        ifidx2dbg_str(&vid), ifidx2dbg_str(&relay_in));
    return dhcp6_relay_vlan_status.del(vid, relay_in);
}

mesa_rc vtss_dhcpv6_relay_status_clear(vtss_ifindex_t vid)
{
    vtss_ifindex_t itr = vid;
    vtss_ifindex_t relay = VTSS_IFINDEX_NONE;
    vtss_appl_dhcpv6_relay_vlan_t status;
    for (auto rc = dhcp6_relay_vlan_status.get_next(&itr,&relay, &status);
         rc == VTSS_RC_OK && itr == vid;
         rc = dhcp6_relay_vlan_status.get_next(&itr,&relay, &status)) {
        VTSS_RC(vtss_dhcpv6_relay_status_clear(itr,relay));
    }
    return VTSS_RC_OK;
}

/* Iterates through statistics table. */
mesa_rc vtss_appl_dhcpv6_relay_vlan_statistics_itr(
    const vtss_ifindex_t *const in,
    vtss_ifindex_t *const out,
    const vtss_ifindex_t *const relay_in,
    vtss_ifindex_t *const relay_out)
{
    T_D("vtss_appl_dhcpv6_relay_vlan_statistics_itr: %s, %s",
        ifidx2dbg_str(in), ifidx2dbg_str(relay_in));
    vtss_appl_dhcpv6_relay_vlan_statistics_t stats;

    if (out == nullptr || relay_out == nullptr) {
        return MESA_RC_ERROR;
    }

    if (in) {
        *out = *in;
    } else {
        memset(out, 0, sizeof(*out));
    }

    if (relay_in) {
        *relay_out = *relay_in;
    }

    return dhcp6_relay_statistics.get_next(out, relay_out, &stats);
}

/* Gets the statistics for a given relay ifindex duo. Result is stored in stat. */
mesa_rc vtss_appl_dhcpv6_relay_vlan_statistics_get(
    vtss_ifindex_t idx,
    vtss_ifindex_t relay_idx,
    vtss_appl_dhcpv6_relay_vlan_statistics_t *stat)
{
    T_D("vtss_appl_dhcpv6_relay_vlan_statistics_get: %s, %s",
        ifidx2dbg_str(&idx), ifidx2dbg_str(&relay_idx));
    return  dhcp6_relay_statistics.get(idx, relay_idx, stat);
}

/* Clears the statistics for a given relay ifindex duo. */
mesa_rc vtss_appl_dhcpv6_relay_vlan_statistics_set(
    vtss_ifindex_t idx,
    vtss_ifindex_t relay_idx,
    const vtss_appl_dhcpv6_relay_vlan_statistics_t  *const stats)
{
    vtss_appl_dhcpv6_relay_vlan_statistics_t stat;
    T_D("vtss_appl_dhcpv6_relay_vlan_statistics_set: %s, %s",
        ifidx2dbg_str(&idx), ifidx2dbg_str(&relay_idx));

    if (dhcp6_relay_statistics.get(idx, relay_idx, &stat) != VTSS_RC_OK) {
        T_D("exit");
        return MESA_RC_ERROR;
    }

    stat.init();
    T_D("exit");
    return dhcp6_relay_statistics.set(idx, relay_idx, &stat);
}

/* Control action get, always returns control->clear_all_stats as false. */
mesa_rc vtss_appl_dhcpv6_relay_control_get(vtss_appl_dhcpv6_relay_control_t *control)
{
    if (control == NULL) {
        return VTSS_RC_ERR_INVALID_NULL_PTR;
    }
    control->clear_all_stats = false;
    return VTSS_RC_OK;
}

/* If control->clear_all_stats is true then all statistics for all vlans are cleared as well as global statistics. */
mesa_rc vtss_appl_dhcpv6_relay_control_set(
    const vtss_appl_dhcpv6_relay_control_t     *const control)
{
    vtss_ifindex_t itr = VTSS_IFINDEX_NONE;
    vtss_ifindex_t relay_vid = VTSS_IFINDEX_NONE;
    vtss_appl_dhcpv6_relay_vlan_statistics_t stat;

    T_D("enter");

    if (control == NULL) {
        T_D("exit");
        return VTSS_RC_ERR_INVALID_NULL_PTR;
    }
    if (control->clear_all_stats) {
        T_D("clear all stats");
        vtss_appl_server_pkts_interface_missing_clear();

        for (auto rc = vtss_appl_dhcpv6_relay_vlan_statistics_itr(&itr, &itr, nullptr, &relay_vid);
         rc == VTSS_RC_OK;
         rc = vtss_appl_dhcpv6_relay_vlan_statistics_itr(&itr, &itr, nullptr, &relay_vid) )
        {
            if (vtss_appl_dhcpv6_relay_vlan_statistics_set(itr, relay_vid, &stat) != VTSS_RC_OK) {
                return VTSS_RC_ERROR;
            }
        }

    }
    T_D("exit");
    return VTSS_RC_OK;
}

int vtss_appl_server_pkts_interface_missing_get()
{
    int pkts_missing;
    DHCP6_RELAY_CRIT_ENTER();
    pkts_missing = server_pkts_interface_missing;
    DHCP6_RELAY_CRIT_EXIT();
    return pkts_missing;
}

void vtss_appl_server_pkts_interface_missing_clear()
{
    DHCP6_RELAY_CRIT_ENTER();
    server_pkts_interface_missing = 0;
    DHCP6_RELAY_CRIT_EXIT();
}

mesa_rc vtss_appl_dhcpv6_relay_interface_missing_get(vtss_appl_dhcpv6_relay_intf_missing_t *missing) {
    if (missing == NULL) {
        return VTSS_RC_ERR_INVALID_NULL_PTR;
    }
    DHCP6_RELAY_CRIT_ENTER();
    missing->num = server_pkts_interface_missing;
    DHCP6_RELAY_CRIT_EXIT();
    return VTSS_RC_OK;
}



/****************************************************************************/
/*  Event handlers                                                          */
/****************************************************************************/

static bool is_ipv6_interface(vtss_ifindex_t &ifindex)
{
    vtss_appl_ip_if_key_ipv6_t    key = {};
    vtss_appl_ip_if_status_link_t link;
    vtss_appl_ip_if_status_ipv6_t status;

    T_D("enter");

    if (!vtss_appl_ip_if_exists(ifindex)) {
        T_D("exit");
        return false;
    }

    if (vtss_appl_ip_if_status_link_get(ifindex, &link) != VTSS_RC_OK) {
        T_D("exit");
        return false;
    }

    if ((link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) == 0) {
        T_D("exit");
        return false;
    }

    key.ifindex = ifindex;

    while (vtss_appl_ip_if_status_ipv6_itr(&key, &key) == VTSS_RC_OK && key.ifindex == ifindex) {
        if (vtss_ipv6_addr_is_link_local(&key.addr.address)) {
            if (vtss_appl_ip_if_status_ipv6_get(&key, &status) != VTSS_RC_OK) {
                continue;
            }

            // If the tentative flag is set, a socket can not be bound to the IPv6 address.
            if ((status.info.flags & VTSS_APPL_IP_IF_IPV6_FLAG_TENTATIVE) == 0) {
                T_D("exit");
                return true;
            }
        }
    }

    T_D("exit");
    return false;
}

static
void setup_multicast_address(struct ipv6_mreq *mreq, vtss_ifindex_t idx)
{
    char char_buf[128];
    sprintf(char_buf, "vtss.vlan.%d", idx2vid(idx));
    memcpy(&mreq->ipv6mr_multiaddr,
           &ipv6_all_dhcp_relay_agents_and_servers,
           sizeof(ipv6_all_dhcp_relay_agents_and_servers));
    mreq->ipv6mr_interface = if_nametoindex(char_buf);
}

static mesa_rc dhcp6_relay_check_valid_global_address(mesa_ipv6_t address)
{
        if (vtss_ipv6_addr_is_link_local(&address)) {
            return DHCP6_RELAY_ERROR_LINK_LOCAL;
        }
        if (vtss_ipv6_addr_is_zero(&address)) {
            return DHCP6_RELAY_ERROR_UNSPEC;
        }
        if (vtss_ipv6_addr_is_loopback(&address)) {
            return DHCP6_RELAY_ERROR_LOOPBACK;
        }
        if (vtss_ipv6_addr_is_multicast(&address)) {
            return DHCP6_RELAY_ERROR_MC;
        }
        return VTSS_RC_OK;
}

static void get_global_interface_ipv6_address(unsigned int if_idx, mesa_ipv6_t *link_address)
{
    vtss_appl_ip_if_status_t status[20];
    vtss_ifindex_t ifindex;
    uint32_t count = 0;
    char buf[24];
    mesa_rc rc;

    ifindex = vtss_ifindex_from_os_ifindex(if_idx);
    vtss_clear(*link_address);

    T_D("if_idx is %u", if_idx);

    if ((rc = vtss_appl_ip_if_status_get(ifindex, VTSS_APPL_IP_IF_STATUS_TYPE_IPV6, 20, &count, status)) != VTSS_RC_OK) {
        T_D("error: %s", ip_error_txt(rc));
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        if (dhcp6_relay_check_valid_global_address(status[i].u.ipv6.net.address) == VTSS_RC_OK) {
            memcpy(link_address, status[i].u.ipv6.net.address.addr, 16);
            break;
        }
    }

    misc_ipv6_txt(link_address, buf);
    T_D("vlan ipv6 address: %s\n", buf);
    return;
}

struct Dhcpv6RelayServer : public vtss::notifications::EventHandler {
    Dhcpv6RelayServer() : EventHandler(&vtss::notifications::subject_main_thread),
                          e_fd(this),
                          e_callback(this),
                          e_dhcp6_relay_vlan_configuration(this),
                          e_relay_status_if_ipv6(this) {}

    // Start server
    void init() {

        e_callback.signal();
        dhcp6_relay_vlan_configuration.observer_new(&e_dhcp6_relay_vlan_configuration);
        status_if_ipv6.observer_new(&e_relay_status_if_ipv6);

    }

    // Opens relay socket, ip ff02::1:2
    void open_socket() {
        struct sockaddr_in6 server_addr = {};
        int                 flag=1;

        if ((sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) <= 0) {
            T_E("socket failed");
            return;
        }
        T_D("Created socket: %d", sock);

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))<0) {
            T_E("Failed setting SO_REUSEADDR");
            return;
        }

        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_addr = in6addr_any;
        server_addr.sin6_port = htons(SERVER_UDP_PORT); // 547
        if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr))<0) {
            T_E("bind failed");
            return;
        }

        if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &flag, sizeof(flag))<0) {
            T_E("Failed setting IPV6_RECVPKTINFO");
            return;
        }

        vtss::Fd fd_new(sock);
        e_fd.assign(vtss::move(fd_new));
        vtss::notifications::subject_main_thread.event_fd_add(e_fd, vtss::notifications::EventFd::READ);
    }

    void close_socket() {
        if (sock>0) {
            if (close(sock)<0) {
                T_E("Failed closing socket: %m");
            }
            e_fd.close();
            T_D("Deleted socket: %d", sock);
            sock=0;
        }
    }

    void add_multicast_address(vtss_ifindex_t idx)
    {
        if (sock == 0) {
            open_socket();
        }
        VTSS_ASSERT(sock != 0);
        setup_multicast_address(&mreq, idx);

        if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                       &mreq, sizeof(mreq)) < 0) {
            T_E("setsockopt: IPV6_JOIN_GROUP: %m");
        }
        int hops = 32;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
                       &hops, sizeof(hops)) < 0) {
            T_E("Failed setting: IPV6_MULTICAST_HOPS: %m");
        }
    }

    void delete_multicast_address(vtss_ifindex_t idx)
    {
        VTSS_ASSERT(sock != 0);
        setup_multicast_address(&mreq, idx);

        if (setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
                       &mreq, sizeof(mreq)) < 0) {
            T_E("setsockopt: IPV6_LEAVE_GROUP: %m");
        }
    }

    void execute(vtss::notifications::Event *e) {
        // Re-attach events
        if (e == &e_dhcp6_relay_vlan_configuration) {
            T_D("Dhcpv6RelayApplyConf::execute: e_dhcp6_relay_vlan_configuration");
            dhcp6_relay_vlan_configuration.observer_get(&e_dhcp6_relay_vlan_configuration,o_dhcp6_relay_vlan_configuration);
        }
        if (e == &e_relay_status_if_ipv6) {
            T_D("Dhcpv6RelayApplyConf::execute: e_relay_status_if_ipv6");
            status_if_ipv6.observer_get(&e_relay_status_if_ipv6, o_relay_status_if_ipv6);
        }
        if (e == &e_callback) {
            T_D("Dhcpv6RelayApplyConf::execute: e_callback");
            (void) callbackSubject.get(e_callback);
        }

        vtss_ifindex_t itr, itr_relay;
        vtss_ifindex_t tmp_itr, tmp_itr_relay;
        vtss_appl_dhcpv6_relay_vlan_t conf;
        vtss_appl_dhcpv6_relay_vlan_t status;
        vtss_appl_dhcpv6_relay_vlan_server_info_t *s_info;
        vtss_appl_dhcpv6_relay_vlan_index_key_t key;
        vtss_appl_dhcpv6_relay_vlan_statistics_t stats;

        // Walk through status to see if any entries shall be deleted
        for (auto rc = dhcp6_relay_vlan_status.get_first(&itr, &itr_relay, &status);
             rc == VTSS_RC_OK;
             rc = dhcp6_relay_vlan_status.get_next(&itr, &itr_relay, &status)) {
            if (dhcp6_relay_vlan_configuration.get(itr, itr_relay, &conf) != VTSS_RC_OK ||
                !is_ipv6_interface(itr) || !is_ipv6_interface(itr_relay)) {
                T_I("Deleting relay agent relaying between vlan %d and vlan %d\n", idx2vid(itr), idx2vid(itr_relay));
                dhcp6_relay_vlan_status.del(itr, itr_relay);

                // Removing sending info from vlan.
                key.set_keys(itr, itr_relay);
                auto it = dhcp6_relay_vlan_upstream_socket_info.find(key);
                if (it != dhcp6_relay_vlan_upstream_socket_info.end()) {
                    s_info = dhcp6_relay_vlan_upstream_socket_info[key];
                    dhcp6_relay_vlan_upstream_socket_info.erase(it);
                    delete s_info;
                }
                tmp_itr = itr;
                tmp_itr_relay = VTSS_IFINDEX_NONE;
                if (VTSS_RC_OK != dhcp6_relay_vlan_status.get_next(&tmp_itr,
                                                                   &tmp_itr_relay,
                                                                   &status) ||
                    tmp_itr !=itr) {
                    T_I("Stop dhcp6 relay agent for vlan %d\n", idx2vid(itr));
                    delete_multicast_address(itr);
                }
            }
        }
        // Iterate through configuration table.
        for (auto rc = dhcp6_relay_vlan_configuration.get_first(&itr, &itr_relay, &conf);
             rc == VTSS_RC_OK;
             rc = dhcp6_relay_vlan_configuration.get_next(&itr, &itr_relay, &conf)) {
            if (!vtss_appl_ip_if_exists(itr)) {
                // VLAN interface has been deleted, delete configuration
                T_D("Delete configuration for int vlan %d", idx2vid(itr));
                vtss_appl_dhcpv6_relay_conf_del(itr, itr_relay);
            } else if (is_ipv6_interface(itr) && is_ipv6_interface(itr_relay)) {
                tmp_itr = itr;
                tmp_itr_relay = VTSS_IFINDEX_NONE;
                if (VTSS_RC_OK != dhcp6_relay_vlan_status.get_next(&tmp_itr,
                                                                   &tmp_itr_relay,
                                                                   &status) ||
                    tmp_itr !=itr) {
                    T_I("Start dhcp6 relay agent for interface %d\n", idx2vid(itr));
                    add_multicast_address(itr);
                }
                if (dhcp6_relay_vlan_status.get(itr, itr_relay, &status) != VTSS_RC_OK) {
                    status = conf;
                    dhcp6_relay_vlan_status.set(itr, itr_relay, &status);
                }
                /* Filling out sockaddr_in6 struct for the upstream interface.*/
                key.set_keys(itr, itr_relay);
                auto it = dhcp6_relay_vlan_upstream_socket_info.find(key);
                if (it == dhcp6_relay_vlan_upstream_socket_info.end()) {
                    T_D("Registering send info for vlan interface %d, key is: %d,%d", idx2vid(itr_relay), idx2vid(itr), idx2vid(itr_relay));
                    vtss_appl_dhcpv6_relay_vlan_server_info_t *up = new vtss_appl_dhcpv6_relay_vlan_server_info_t();
                    struct sockaddr_in6 link = {};
                    memcpy(&link.sin6_addr, conf.relay_destination.addr, sizeof(conf.relay_destination.addr));
                    link.sin6_family = AF_INET6;
                    link.sin6_port = htons(SERVER_UDP_PORT);
                    up->link = link;
                    dhcp6_relay_vlan_upstream_socket_info.set(key, up);
                }

                if (dhcp6_relay_statistics.get(itr, itr_relay, &stats) != VTSS_RC_OK) {
                    stats.init();
                    dhcp6_relay_statistics.set(itr, itr_relay, &stats);
                }
            }
        }
    }

    void send_relay_msg_to_server(unsigned char *msg, size_t msg_len, unsigned int if_idx) {
        vtss_appl_dhcpv6_relay_vlan_t conf;
        vtss_appl_dhcpv6_relay_vlan_server_info_t *s_info;
        vtss_appl_dhcpv6_relay_vlan_index_key_t key;
        vtss_appl_dhcpv6_relay_vlan_statistics_t stats;
        char buf[20];
        mesa_vid_t vlan;
        vtss_ifindex_t in;
        vtss_ifindex_t out;
        vtss_ifindex_t *relay_in = NULL;
        vtss_ifindex_t relay_out;
        unsigned int out_if_idx;
        int result = 0;

        T_D("enter");

        in = vtss_ifindex_from_os_ifindex(if_idx);

        T_D("Received on vlan %u\n", idx2vid(in));

        while (vtss_appl_dhcpv6_relay_vlan_conf_itr(&in, &out, relay_in, &relay_out) == VTSS_RC_OK) {
            if (out != in) {
                T_D("No more vlans to relay through, exit.");
                break;
            }

            if (vtss_appl_dhcpv6_relay_conf_get(out, relay_out, &conf) != VTSS_RC_OK) {
                in = out;
                relay_in = &relay_out;
                continue;
            }

            key.set_keys(out, relay_out);
            auto it = dhcp6_relay_vlan_upstream_socket_info.find(key);
            if (it == dhcp6_relay_vlan_upstream_socket_info.end()) {
                in = out;
                relay_in = &relay_out;
                continue;
            }

            s_info = dhcp6_relay_vlan_upstream_socket_info[key];
            memcpy(&s_info->link.sin6_addr, conf.relay_destination.addr, sizeof(conf.relay_destination.addr));
            vlan = idx2vid(relay_out);
            sprintf(buf, "vtss.vlan.%d", vlan);
            out_if_idx = if_nametoindex(buf);

            char ipv6addr[100];
            inet_ntop(AF_INET6, &(s_info->link.sin6_addr), ipv6addr, sizeof(ipv6addr));
            result = send_relay_packet(msg, msg_len, &s_info->link, out_if_idx, sock);
            T_I("sent %d bytes to server %s from vlan %d using socket %d", result, ipv6addr, vlan, sock);

            if (dhcp6_relay_statistics.get(out, relay_out, &stats) == VTSS_RC_OK) {
                stats.inc_rx_from_client();
                if (result > 0) {
                    stats.inc_tx_to_server();
                }
                dhcp6_relay_statistics.set(out, relay_out, &stats);
            }

            in = out;
            relay_in = &relay_out;
        }

        T_D("exit");
    }

    int send_relay_packet(unsigned char *msg, size_t msg_len, struct sockaddr_in6 *to, unsigned int out_if_idx, int socket_no) {
        int result;
        // Use dhcp_relay's base to send such a packet
        ssize_t send_packet6(unsigned int out_if_idx, int socket_no, const unsigned char *raw, size_t len, struct sockaddr_in6 *to);
        if ((result = send_packet6(out_if_idx, socket_no, msg, msg_len, to)) < 0) {
            T_D("sending packet failed, errno: ", errno);
        }

        return result;
    }

    void inc_client_packet_dropped_counter(unsigned int if_idx) {
        vtss_ifindex_t in;
        vtss_ifindex_t out;
        vtss_ifindex_t *relay_in = NULL;
        vtss_ifindex_t relay_out;
        vtss_appl_dhcpv6_relay_vlan_statistics_t stats;

        in = vtss_ifindex_from_os_ifindex(if_idx);

        while (vtss_appl_dhcpv6_relay_vlan_conf_itr(&in, &out, relay_in, &relay_out) == VTSS_RC_OK) {
            if (out != in) {
                T_D("No more vlans to relay through, exit.");
                break;
            }

            if (dhcp6_relay_statistics.get(out, relay_out, &stats) == VTSS_RC_OK) {
                stats.inc_rx_from_client();
                stats.inc_client_pkt_dropped();
                in = out;
                relay_in = &relay_out;
                dhcp6_relay_statistics.set(out, relay_out, &stats);
                continue;
            }

            in = out;
            relay_in = &relay_out;
        }
    }

    void forward_upstream(in6_addr *from_addr,
                           in6_addr *to_addr,
                           unsigned char *message,
                           unsigned char msg_type,
                           unsigned int if_idx,
                           int len) {
        mesa_ipv6_t link_address;
        unsigned char buffer[1024];
        unsigned char hop_count = 0;
        unsigned int pkt_size = 0;
        vtss_dhcpv6_relay_agent_interface_id_option intf_option(if_idx);
        vtss_dhcpv6_relay_agent_msg_option msg_option(message, len);
        vtss_dhcpv6_relay_agent_packet relay_packet;

        T_D("enter");

        /* Construct relay forward message. */

        get_global_interface_ipv6_address(if_idx, &link_address);
        if (msg_type == dhcp6::DHCP6RELAY_FORW) {
            hop_count = message[1];
            /* Increment hop count as this is a packet from another relay agent. */
            hop_count++;
            if (hop_count >= HOP_COUNT_LIMIT) {
                T_D("Hop count limit has been reached and packet will be dropped.");
                inc_client_packet_dropped_counter(if_idx);
                return;
            }
            relay_packet.init(12, hop_count, from_addr, &link_address, if_idx);

        } else {
            /* All other cases that end up here are packets directly from client. */
            relay_packet.init(12, 0, from_addr, &link_address, if_idx);
        }
        relay_packet.add_option(&intf_option);
        relay_packet.add_option(&msg_option);
        pkt_size = relay_packet.get_packet_htons(buffer, sizeof(buffer));
        T_D("Upstream: packet size is %u, relay_packet.size is %u", pkt_size, relay_packet.size);
        if (pkt_size != relay_packet.size) {
            T_D("buffer does not contain full packet, packet will be dropped, exit");
            inc_client_packet_dropped_counter(if_idx);
            return;
        }

        T_D("constructed relay_packet of size %u", pkt_size);

        send_relay_msg_to_server(buffer, pkt_size, if_idx);
    }

    void forward_downstream(unsigned char *message, int msg_len, unsigned int in_if_idx) {
        vtss_ifindex_t send_idx, recv_idx;
        struct sockaddr_in6 to;
        mesa_ipv6_t addr;
        unsigned char forward_msg[512];
        unsigned char *tmp_msg;
        unsigned char peer_addr[16];
        u16 option, len, send_msg_len = 0;
        unsigned int out_if_idx = 0;
        int result;
        vtss_appl_dhcpv6_relay_vlan_statistics_t stats;
        unsigned char recv_msg_type;

        T_D("forward_downstream, enter");

        recv_idx = vtss_ifindex_from_os_ifindex(in_if_idx);

        tmp_msg = message+18;

        /* Extract address to send to from peer address in message. */
        memcpy(peer_addr, tmp_msg, sizeof(peer_addr));
        tmp_msg += sizeof(peer_addr);

        memcpy(&option, tmp_msg, sizeof(option));
        option = ntohs(option);

        for (unsigned int i = 0; i < msg_len - 18 - sizeof(peer_addr);) {

            memcpy(&option, tmp_msg, sizeof(option));
            option = ntohs(option);

            tmp_msg += sizeof(option);

            memcpy(&len, tmp_msg, sizeof(len));
            len = ntohs(len);

            tmp_msg += sizeof(len);

            /* Process packet interface option, no. 18 */
            if(option == RELAY_INTERFACE_ID_OPTION_CODE) {
                memcpy(&out_if_idx, tmp_msg, len);
                out_if_idx = ntohl(out_if_idx);
            }

             /* Extract dhcp message from relay message option, no. 9 */
            if (option == RELAY_MESSAGE_OPTION_CODE) {
                send_msg_len = len;
                memcpy(forward_msg, tmp_msg, len);
                break;
            }

            tmp_msg += len;

            i += sizeof(option) + sizeof(len) + len;
        }

        /* If the packet contained the information needed to forward it, we do so. */
        if (out_if_idx && send_msg_len) {
            send_idx = vtss_ifindex_from_os_ifindex(out_if_idx);

            memcpy(&to.sin6_addr, peer_addr, sizeof(peer_addr));
            recv_msg_type = forward_msg[0];
            T_D("recv_msg_type is %u", recv_msg_type);

            to.sin6_family = AF_INET6;
            if (recv_msg_type == dhcp6::DHCP6RELAY_REPL) {
                to.sin6_port = htons(SERVER_UDP_PORT);
            } else {
                to.sin6_port = htons(CLIENT_UDP_PORT);
            }
            memcpy(&addr.addr, &to.sin6_addr, sizeof(addr.addr));
            if (vtss_ipv6_addr_is_link_local(&addr)) {
                to.sin6_scope_id = out_if_idx;
            }
            result = send_relay_packet(forward_msg, (size_t) send_msg_len, &to, out_if_idx, sock);
            T_I("sending downstream: sent %d bytes from socket %d, and vlan %d", result, sock, idx2vid(send_idx));

            if (dhcp6_relay_statistics.get(send_idx, recv_idx, &stats) == VTSS_RC_OK) {
                stats.inc_rx_from_server();
                if (result > 0) {
                    stats.inc_tx_to_client();
                }
                dhcp6_relay_statistics.set(send_idx, recv_idx, &stats);
            }
        } else if (out_if_idx){
            T_D("Relay message option missing, dropping packet from server");
            send_idx = vtss_ifindex_from_os_ifindex(out_if_idx);
            if (dhcp6_relay_statistics.get(send_idx, recv_idx, &stats) == VTSS_RC_OK) {
                stats.inc_rx_from_server();
                stats.inc_server_pkt_dropped();
                dhcp6_relay_statistics.set(send_idx, recv_idx, &stats);
            }
        } else {
            T_D("Interface option missing, dropping server packet");
            // Can't increment rx from server and server_pkt_dropped as dont't have relay index
            DHCP6_RELAY_CRIT_ENTER();
            server_pkts_interface_missing++;
            T_D("Intf_option_missing count: %d", server_pkts_interface_missing);
            DHCP6_RELAY_CRIT_EXIT();
        }

    }

    // Handle events
    void execute(vtss::notifications::EventFd *e) override {
        struct msghdr m;
        struct iovec v;
        struct cmsghdr *cmsg;
        struct in6_pktinfo *pktinfo;
        unsigned char buf[2000];
        char info[100];
        struct sockaddr_in6 from;
        struct in6_addr to;
        unsigned int if_idx;
        unsigned char msg_type;

        T_D("Dhcpv6RelayServer::execute::EventFd. Received packet on socket: %d", e->raw());

        if (e->events() & vtss::notifications::EventFd::READ) {
            memset(&m, 0, sizeof(m));

            m.msg_name = &from;
            m.msg_namelen = sizeof(from);

            v.iov_base = buf;
            v.iov_len = sizeof(buf);
            m.msg_iov = &v;
            m.msg_iovlen = 1;

            m.msg_control = info;
            m.msg_controllen = sizeof(info);
            int ret = recvmsg(e->raw(), &m, 0);
            if (ret<0) return;
            buf[ret]=0;

            cmsg = CMSG_FIRSTHDR(&m);
            while (cmsg != NULL) {
                if ((cmsg->cmsg_level == IPPROTO_IPV6) &&
                    (cmsg->cmsg_type == IPV6_PKTINFO)) {
                    pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
                    to = pktinfo->ipi6_addr;
                    if_idx = pktinfo->ipi6_ifindex;
                    break;
                }
                cmsg = CMSG_NXTHDR(&m, cmsg);
            }
            if (cmsg == NULL) {
                T_E("No IPV6_PKTINFO found");
                return;
            };

            char from_addr[100];
            char to_addr[100];

            inet_ntop(AF_INET6, &(from.sin6_addr),
                      from_addr, sizeof(from_addr));
            inet_ntop(AF_INET6, &to,
                      to_addr, sizeof(to_addr));

            msg_type = buf[0];

            T_D("msg_type is %u", msg_type);

            switch(msg_type) {
            case dhcp6::DHCP6SOLICIT:
            case dhcp6::DHCP6REQUEST:
            case dhcp6::DHCP6CONFIRM:
            case dhcp6::DHCP6RENEW:
            case dhcp6::DHCP6REBIND:
            case dhcp6::DHCP6RELEASE:
            case dhcp6::DHCP6DECLINE:
            case dhcp6::DHCP6INFORMATION_REQUEST:
            case dhcp6::DHCP6LEASEQUERY:
            case dhcp6::DHCP6RELAY_FORW:
                T_D("found client or relay forward message");
                forward_upstream(&from.sin6_addr, &to, buf, msg_type, if_idx, ret);
                break;
            case dhcp6::DHCP6RELAY_REPL:
                T_D("found relay reply server msg");
                forward_downstream(buf, ret, if_idx);
                break;
            default:
                /*RFC 7283, section 4.2: If the relay agent
                receives messages other than Relay-forward and Relay-reply and the
                relay agent does not recognize its message type, it MUST forward them
                as described in Section 20.1.1 of [RFC3315]*/
                T_D("found something else, packet will be relayed to server");
                forward_upstream(&from.sin6_addr, &to, buf, msg_type, if_idx, ret);
                break;
            }

        }
        vtss::notifications::subject_main_thread.event_fd_add(e_fd, vtss::notifications::EventFd::READ);
        T_D("exit");
    }


    int                 sock;
    vtss::notifications::EventFd e_fd;
    struct ipv6_mreq mreq;

    vtss::notifications::Event e_callback;

    vtss::notifications::Event e_dhcp6_relay_vlan_configuration;
    Dhcp6RelayVlanConfiguration::Observer o_dhcp6_relay_vlan_configuration;

    vtss::notifications::Event e_relay_status_if_ipv6;
    StatusIfIpv6::Observer o_relay_status_if_ipv6;

} eh_server;

/****************************************************************************/
/*  Callback functions                                                      */
/****************************************************************************/
void dhcpv6_relay_client_ip_vlan_interface_callback(vtss_ifindex_t ifidx)
{
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, DEBUG) << "dhcpv6_relay_client_ip_vlan_interface_callback: " << ifidx;
    //    T_D("dhcpv6_relay_client_ip_vlan_interface_callback: %d", if_id);
    callbackSubject.signal();
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_ICLI
extern "C" int dhcp6_relay_icli_cmd_register();
#endif
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void dhcp6_relay_mib_init(void);
#endif

vtss_appl_dhcpv6_relay_vlan_t defaultConf;

/* Initialize module */
mesa_rc dhcp6_relay_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_I("INIT_CMD_INIT");
        critd_init(&dhcp6_relay_crit, "dhcp6_relay", VTSS_MODULE_ID_DHCP6_RELAY, CRITD_TYPE_MUTEX);

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        dhcp6_relay_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_dhcp6_relay_json_init();
#endif
#ifdef VTSS_SW_OPTION_ICLI
        dhcp6_relay_icli_cmd_register();
#endif

        /* Subscribe to VLAN-inteface changes. */
        VTSS_RC(vtss_ip_if_callback_add(dhcpv6_relay_client_ip_vlan_interface_callback));

        //eh_dhcpv6_relay_apply_conf.init();
        //        eh_relay_agent.init();
        //eh_callback.init();
        eh_server.init();

        T_I("INIT_CMD_INIT");
        break;

    case INIT_CMD_START:
        T_I("INIT_CMD_START");
#ifdef VTSS_SW_OPTION_ICFG
        if (dhcp6_relay_icfg_init() != VTSS_RC_OK) {
            T_D("Calling alarm_icfg_init() failed");
        }
#endif
        T_I("INIT_CMD_START LEAVE");
        break;

    case INIT_CMD_CONF_DEF:
        T_I("RESTORE TO DEFAULT");
        dhcp6_relay_vlan_configuration.clear();
        dhcp6_relay_statistics.clear();
        T_I("INIT_CMD_CONF_DEF LEAVE");
        break;

    default:
        break;
    }

    return 0;

}

