/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_link.h>
#include <linux/if_addr.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <pthread.h>

#include "conf_api.h"
#include "ip_dhcp6c.hxx"
#include "ip_utils.hxx"
#include "ip_trace.h"
#include "ip_os.hxx"
#include "ip_chip.hxx"
#include "ip_expose.hxx"
#include "vtss_netlink.hxx"
#include "conf_api.h"    // for conf_mgmt_mac_addr_get
#include "packet_api.h"  // For definition of PACKET_XTR_QU_IGMP
#include "mac_utils.hxx" // For mesa_mac_t::operator!=()

#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h" // For S_E()
#endif

#ifdef VTSS_SW_OPTION_ACL
#include "acl_api.h"
#endif
#ifdef VTSS_SW_OPTION_CPUPORT
#include "cpuport_api.hxx"
#endif
#include <vtss/basics/fd.hxx>
#include <vtss/basics/set.hxx>
#include <vtss/basics/map.hxx>
#include <vtss/basics/array.hxx>
#include <vtss/basics/memory.hxx>
#include <vtss/basics/enum_macros.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <vtss/basics/string-utils.hxx>
#include <vtss/basics/synchronized.hxx>
#include <vtss/basics/memcmp-operator.hxx>
#include <vtss/basics/formatting_tags.hxx>
#include <vtss/basics/expose/table-status.hxx>

static vtss_handle_t IP_OS_thread_handle;
static vtss_thread_t IP_OS_thread_block;

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_IP

using namespace vtss::appl;

StatusIfLink status_if_link("status_if_link", VTSS_MODULE_ID_IP);
StatusNb     status_nb_ipv4("status_nb_ipv4", VTSS_MODULE_ID_IP);
StatusIfIpv4 status_if_ipv4("status_if_ipv4", VTSS_MODULE_ID_IP);

// This must be present whether or not IPV6 is included, because it's used by
// JSON. However, it may not be filled with data.
StatusIfIpv6 status_if_ipv6("status_if_ipv6", VTSS_MODULE_ID_IP);

#if defined(VTSS_SW_OPTION_IPV6)
StatusNb     status_nb_ipv6("status_nb_ipv6", VTSS_MODULE_ID_IP);
#endif

// The value is a dummy. We use an underlying map to be able to find differences
// between the current and the new routes through ip_os_routes_t::Callback
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<mesa_routing_entry_t *>, vtss::expose::ParamVal<bool>> ip_os_routes_t;

#if defined(VTSS_SW_OPTION_L3RT)
static ip_os_routes_t IP_OS_status_rt_ipv4("IP_OS_status_rt_ipv4", VTSS_MODULE_ID_IP);
#endif

#if defined(VTSS_SW_OPTION_L3RT) && defined(VTSS_SW_OPTION_IPV6)
static ip_os_routes_t IP_OS_status_rt_ipv6("IP_OS_status_rt_ipv6", VTSS_MODULE_ID_IP);
#endif

// ip_global_notification_status holds global state that one can get
// notifications on, that being SNMP traps or JSON notifications.
// The type it holds is of vtss_appl_ip_global_notification_status_t.
IpGlobalNotificationStatus ip_global_notification_status;

/******************************************************************************/
// vtss_appl_ip_neighbor_status_t::operator!=()
// For insertion into status_nb_ipv4 and status_nb_ipv6.
/******************************************************************************/
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ip_neighbor_status_t);

struct State {
    vtss::Set<mesa_vid_mac_t> mac_addr_list;
    vtss::Map<int, vtss_ifindex_t> os_ifindex_to_ifindex;
    vtss::Map<vtss_ifindex_t, mesa_ipv6_network_t> vid_to_ipv6_network;
};

static vtss::Synchronized<State, VTSS_MODULE_ID_IP> state;

#define DO(FUNC, ...)                                                                 \
    do {                                                                              \
        if ((rc = FUNC(__VA_ARGS__)) != VTSS_RC_OK) {                                 \
            T_EG(IP_TRACE_GRP_OS, "Failed: " #FUNC " error code: %s", error_txt(rc)); \
            return rc;                                                                \
        }                                                                             \
    } while (0)

#define DO_(FUNC, ...)                                                                \
    do {                                                                              \
        if ((rc = FUNC(__VA_ARGS__)) != VTSS_RC_OK) {                                 \
            T_IG(IP_TRACE_GRP_OS, "Failed: " #FUNC " error code: %s", error_txt(rc)); \
            return;                                                                   \
        }                                                                             \
    } while (0)

/******************************************************************************/
// IP_OS_u8_to_u32()
/******************************************************************************/
static mesa_ipv4_t IP_OS_u8_to_u32(unsigned char *p)
{
    return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

/******************************************************************************/
// IP_OS_nl_req_link_dump()
/******************************************************************************/
static inline mesa_rc IP_OS_nl_req_link_dump(netlink::NetlinkCallbackAbstract *cb, int sndbuf = 32768, int rcvbuf = 1048576)
{
    int seq = netlink::netlink_seq();

    struct {
        struct nlmsghdr nlh;
        struct ifinfomsg r;
    } req;

    memset(&req, 0, sizeof(req));
    req.r.ifi_family = AF_PACKET;
    req.nlh.nlmsg_len = sizeof(req);
    req.nlh.nlmsg_type = RTM_GETLINK;
    req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nlh.nlmsg_pid = 0;
    req.nlh.nlmsg_seq = seq;

    return nl_req((const void *)&req, sizeof(req), seq, __FUNCTION__, cb, sndbuf, rcvbuf);
}

/******************************************************************************/
// IP_OS_nl_req_link_ipv6_dump()
/******************************************************************************/
static inline mesa_rc IP_OS_nl_req_link_ipv6_dump(netlink::NetlinkCallbackAbstract *cb, int sndbuf = 32768, int rcvbuf = 1048576)
{
    int seq = netlink::netlink_seq();

    struct {
        struct nlmsghdr nlh;
        struct rtgenmsg r;
    } req;

    memset(&req, 0, sizeof(req));
    req.r.rtgen_family = AF_INET6;
    req.nlh.nlmsg_len = sizeof(req);
    req.nlh.nlmsg_type = RTM_GETLINK;
    req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nlh.nlmsg_pid = 0;
    req.nlh.nlmsg_seq = seq;

    return nl_req((const void *)&req, sizeof(req), seq, __FUNCTION__, cb, sndbuf, rcvbuf);
}

/******************************************************************************/
// IP_OS_nl_req_neigh_dump()
/******************************************************************************/
static inline mesa_rc IP_OS_nl_req_neigh_dump(netlink::NetlinkCallbackAbstract *cb, unsigned char family, int sndbuf = 32768, int rcvbuf = 1048576)
{
    int seq = netlink::netlink_seq();

    struct {
        struct nlmsghdr nlh;
        struct ndmsg ndm;
    } req;

    memset(&req, 0, sizeof(req));
    req.ndm.ndm_family = family;
    req.nlh.nlmsg_len = sizeof(req);
    req.nlh.nlmsg_type = RTM_GETNEIGH;
    req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nlh.nlmsg_pid = 0;
    req.nlh.nlmsg_seq = seq;

    return nl_req((const void *)&req, sizeof(req), seq, __FUNCTION__, cb, sndbuf, rcvbuf);
}

/******************************************************************************/
// IP_OS_nl_req_ipaddr_dump()
/******************************************************************************/
static inline mesa_rc IP_OS_nl_req_ipaddr_dump(netlink::NetlinkCallbackAbstract *cb, unsigned char family, int sndbuf = 32768, int rcvbuf = 1048576)
{
    int seq = netlink::netlink_seq();

    struct {
        struct nlmsghdr nlh;
        struct ifaddrmsg ifa;
    } req;

    memset(&req, 0, sizeof(req));
    req.ifa.ifa_family = family;
    req.nlh.nlmsg_len = sizeof(req);
    req.nlh.nlmsg_type = RTM_GETADDR;
    req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nlh.nlmsg_pid = 0;
    req.nlh.nlmsg_seq = seq;

    return nl_req((const void *)&req, sizeof(req), seq, __FUNCTION__, cb, sndbuf, rcvbuf);
}

/******************************************************************************/
// IP_OS_nl_req_route_dump()
/******************************************************************************/
static inline mesa_rc IP_OS_nl_req_route_dump(netlink::NetlinkCallbackAbstract *cb, unsigned char family, int sndbuf = 32768, int rcvbuf = 1048576)
{
    int seq = netlink::netlink_seq();

    struct {
        struct nlmsghdr nlh;
        struct rtmsg rtm;
    } req;

    memset(&req, 0, sizeof(req));
    req.rtm.rtm_family = family;
    req.nlh.nlmsg_len = sizeof(req);
    req.nlh.nlmsg_type = RTM_GETROUTE;
    req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nlh.nlmsg_pid = 0;
    req.nlh.nlmsg_seq = seq;

    return nl_req((const void *)&req, sizeof(req), seq, __FUNCTION__, cb, sndbuf, rcvbuf);
}

#ifdef VTSS_SW_OPTION_ACL
/******************************************************************************/
// IP_OS_hw_ip_mac_ace_add()
// Add multicast IP group into ACL
//
// Only 23 bits of IP multicast address are mapped to the MAC-layer multicast
// address. For class D convention, that means 5 bits in the IP multicast
// address don't map to the MAC-layer multicast address. So we encounter a
// problem when using chip MAC-table to copy the control packets, say
// '224.0.0.5', to CPU, that also copies '225.0.0.5', '226.0.0.5' such kinds of
// packets to CPU unexpectedly.
//
// To avoid having non-control traffic copied to the CPU, we choose to use ACL
// to qualify DIP and copy it to CPU instead of using the chip MAC table.
// Refer to the IPv4 Multicast Address Space Registry, we have chosen to use the
// 23 bits from the MAC address, and assume that the remaining 5 bits are set to
// zero. This covers the control packets locating from 224.0.0.0 to
// 224.127.255.255. This may be an issue if we want to use the linux kernel for
// doing IGMPv3 as this is using SSM (source specific multicast)
/******************************************************************************/
static mesa_rc IP_OS_hw_ip_mac_ace_add(mesa_vid_t vid, const mesa_mac_t *mac)
{
    acl_entry_conf_t conf;
    mesa_rc          rc;
    mesa_ip_t        dip = 0xE0000000 | (mac->addr[5]);

    dip |= (mac->addr[4]) << 8;
    dip |= (mac->addr[3] & 0x7f) << 16;

    // Initialize ACE
    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_IPV4, &conf)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_OS, "acl_mgmt_ace_init() failed: %s", error_txt(rc));
        return rc;
    }

    // Update ACE
    conf.isdx_disable         = TRUE;
    conf.action.force_cpu     = TRUE;
    conf.action.cpu_queue     = PACKET_XTR_QU_IGMP;
    conf.action.cpu_once      = FALSE;
    conf.isid                 = VTSS_ISID_LOCAL;
    conf.vid.value            = vid;
    conf.vid.mask             = 0xfff;
    conf.frame.ipv4.dip.value = dip;
    conf.frame.ipv4.dip.mask  = 0xffffffff;

    // Apply ACE
    T_DG(IP_TRACE_GRP_OS, "acl_mgmt_ace_add(vid = %u, IP: %s/%s", conf.vid.value, conf.frame.ipv4.dip.value, conf.frame.ipv4.dip.mask);
    if ((rc = acl_mgmt_ace_add(ACL_USER_IP, ACL_MGMT_ACE_ID_NONE, &conf)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_OS, "acl_mgmt_ace_add(vid = %u, IP = %s/%s) failed: %s", conf.vid.value, conf.frame.ipv4.dip.value, conf.frame.ipv4.dip.mask, error_txt(rc));
    }

    return rc;
}
#endif // VTSS_SW_OPTION_ACL

#ifdef VTSS_SW_OPTION_ACL
/******************************************************************************/
// IP_OS_hw_ip_mac_ace_del()
// Delete multicast IP group from ACL
/******************************************************************************/
static mesa_rc IP_OS_hw_ip_mac_ace_del(const mesa_vid_t vid, const mesa_mac_t *const mac)
{
    acl_entry_conf_t ace_conf;
    mesa_ace_id_t    id = ACL_MGMT_ACE_ID_NONE;
    mesa_rc          rc = VTSS_RC_OK;
    mesa_ip_t        dip = 0xE0000000 | (mac->addr[5]);

    dip |= (mac->addr[4]) << 8;
    dip |= (mac->addr[3] & 0x7f) << 16;

    while (acl_mgmt_ace_get(ACL_USER_IP, id, &ace_conf, NULL, 1) == VTSS_RC_OK) {
        // Assign for next loop
        id = ace_conf.id;

        // Find the matched ACE ID
        if (ace_conf.vid.value            != vid   ||
            ace_conf.vid.mask             != 0xfff ||
            ace_conf.frame.ipv4.dip.value != dip   ||
            ace_conf.frame.ipv4.dip.mask  != 0xffffffff) {
            continue;
        }

        T_DG(IP_TRACE_GRP_OS, "acl_mgmt_ace_del(%u)", ace_conf.id);
        if ((rc = acl_mgmt_ace_del(ACL_USER_IP, ace_conf.id)) != VTSS_RC_OK && rc != VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND) {
            T_EG(IP_TRACE_GRP_OS, "acl_mgmt_ace_del(%u) failed: %s", ace_conf.id, error_txt(rc));
            return rc;
        }
    }

    return rc;
}
#endif // VTSS_SW_OPTION_ACL

#ifdef VTSS_SW_OPTION_ACL
/******************************************************************************/
// IP_OS_is_multicast_mac_address()
// Check if the MAC is ethernet multicast address
/******************************************************************************/
static bool IP_OS_is_multicast_mac_address(const mesa_mac_t *mac)
{
    if (mac->addr[0] == 0x01 &&
        mac->addr[1] == 0x00 &&
        mac->addr[2] == 0x5e &&
        ((mac->addr[3] & 0x10) == 0x00)) {
        return true;
    }

    return false;
}
#endif // VTSS_SW_OPTION_ACL

/******************************************************************************/
// IP_OS_hw_ip_mac_subscribe()
// We use the ACL rule to trap the multicase control packet to CPU instead of
// MAC table entry because the solution won't copy more unexpected multicast
// packets to CPU (both of control and data packets).
/******************************************************************************/
static mesa_rc IP_OS_hw_ip_mac_subscribe(const mesa_vid_t vid, const mesa_mac_t *const mac)
{
    mesa_rc rc = VTSS_RC_OK;

#ifdef VTSS_SW_OPTION_ACL
    if (IP_OS_is_multicast_mac_address(mac)) {
        // 01005exxxxxx  Add to ACL
        T_IG(IP_TRACE_GRP_OS, "Add IP Group ACE, vid = %u, mac = %s", vid, *mac);
        rc = IP_OS_hw_ip_mac_ace_add(vid, mac);
        return rc;
    }
#endif // VTSS_SW_OPTION_ACL

    // Add to MAC table
    T_IG(IP_TRACE_GRP_OS, "Add to chip mac table, vid = %u, mac = %s", vid, *mac);
    rc = vtss_ip_chip_mac_subscribe(vid, mac);

    return rc;
}

/******************************************************************************/
// IP_OS_hw_ip_mac_unsubscribe()
/******************************************************************************/
static mesa_rc IP_OS_hw_ip_mac_unsubscribe(const mesa_vid_t vid, const mesa_mac_t *const mac)
{
    mesa_rc rc = VTSS_RC_OK;

#ifdef VTSS_SW_OPTION_ACL
    if (IP_OS_is_multicast_mac_address(mac)) {
        // 01005exxxxxx
        T_IG(IP_TRACE_GRP_OS, "Delete IP Group ACE, vid = %u, mac = %s", vid, *mac);
        rc = IP_OS_hw_ip_mac_ace_del(vid, mac);
        return rc;
    }
#endif

    T_IG(IP_TRACE_GRP_OS, "Delete chip mac table, vid = %u, mac = %s", vid, *mac);
    rc = vtss_ip_chip_mac_unsubscribe(vid, mac);

    return rc;
}

/******************************************************************************/
// IP_OS_vlan_ipv6_network_match()
/******************************************************************************/
static bool IP_OS_vlan_ipv6_network_match(vtss_ifindex_t ifindex, mesa_ipv6_t *addr)
{
    bool match = false;

    SYNCHRONIZED(state) {
        auto itr = state.vid_to_ipv6_network.find(ifindex);
        if (itr != state.vid_to_ipv6_network.end() && vtss_ipv6_net_include(&itr->second, addr)) {
            match = true;
        }
    }

    return match;
}

/******************************************************************************/
// ParseIfName
/******************************************************************************/
struct ParseIfName : public vtss::parser::ParserBase {
    typedef uint16_t value_type;

    bool operator()(const char *&b, const char *e)
    {
        const char *_b = b;

        vtss::parser::Lit sep(VTSS_APPL_IP_VLAN_IF_PREFIX);
        if (vtss::parser::Group(b, e, sep, vlan)) {
            return true;
        }

        b = _b;
        return false;
    }

    const value_type &get() const
    {
        return vlan.get();
    }

    vtss::parser::Int<uint16_t, 10, 1, 4> vlan;
};

#if defined(VTSS_SW_OPTION_L3RT)
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ip_global_notification_status_t);
#endif // VTSS_SW_OPTION_L3RT

#if defined(VTSS_SW_OPTION_L3RT)
/******************************************************************************/
// IP_OS_route_table_full_report()
/******************************************************************************/
void IP_OS_route_table_full_report(void)
{
    static bool                               already_warned;
    vtss_appl_ip_global_notification_status_t global_notif_status;

    if (already_warned) {
        return;
    }

    already_warned = true;
    T_WG(IP_TRACE_GRP_NETLINK, "Failed to add HW route - probably due to resource starvation.");

#if defined(VTSS_SW_OPTION_SYSLOG)
    S_E("ROUTE-ADD: Hardware FIB has reached its capacity. Packets through some routes will be dropped");
#endif

    // Can only be set, never cleared.
    if (ip_global_notification_status.get(&global_notif_status) != VTSS_RC_OK) {
        T_E("Unable to get global notification status");
        memset(&global_notif_status, 0, sizeof(global_notif_status));
    }

    global_notif_status.hw_routing_table_depleted = true;

    if (ip_global_notification_status.set(&global_notif_status) != VTSS_RC_OK) {
        T_E("Unable to set global notification status");
    }

}
#endif // VTSS_SW_OPTION_L3RT

/******************************************************************************/
// LinkStateCallbackPoll
/******************************************************************************/
struct LinkStateCallbackPoll : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        int len = n->nlmsg_len;

        T_NG(IP_TRACE_GRP_NETLINK, "CB type: %s", netlink::AsRtmType(n->nlmsg_type));
        T_NG(IP_TRACE_GRP_NETLINK, "%s", *n);

        if (n->nlmsg_type != RTM_GETLINK && n->nlmsg_type != RTM_NEWLINK) {
            return;
        }

        struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(n);
        len -= NLMSG_LENGTH(sizeof(*ifi));
        if (len < 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "Msg too short for this type!");
            return;
        }

        vtss_appl_ip_if_status_link_t st = {};
        st.os_ifindex = ifi->ifi_index;
#define F(X, Y) \
    if ((X)&ifi->ifi_flags) st.flags |= Y;
        F(IFF_LOWER_UP, VTSS_APPL_IP_IF_LINK_FLAG_UP);
        F(IFF_BROADCAST, VTSS_APPL_IP_IF_LINK_FLAG_BROADCAST);
        F(IFF_LOOPBACK, VTSS_APPL_IP_IF_LINK_FLAG_LOOPBACK);
        F(IFF_NOARP, VTSS_APPL_IP_IF_LINK_FLAG_NOARP);
        F(IFF_PROMISC, VTSS_APPL_IP_IF_LINK_FLAG_PROMISC);
        F(IFF_MULTICAST, VTSS_APPL_IP_IF_LINK_FLAG_MULTICAST);
#undef F

        mesa_vid_t vid = 0;
        bool has_mac = false;
        bool has_bcast = false;
        vtss_appl_ip_if_link_flags_t flags = VTSS_APPL_IP_IF_LINK_FLAG_NONE;
        vtss_ifindex_t ifindex = VTSS_IFINDEX_NONE;

        struct rtattr *rta = IFLA_RTA(ifi);
        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case IFLA_IFNAME: {
                const char *n = (const char *)RTA_DATA(rta);
                const char *e = n + RTA_PAYLOAD(rta);
                ParseIfName p;

                if (p(n, e)) {
                    T_NG(IP_TRACE_GRP_NETLINK, "VLAN: %s", p.get());
                    vid = p.get();
                    if (vid == 0 || vtss_ifindex_from_vlan(vid, &ifindex) != VTSS_RC_OK) {
                        return;
                    }
#ifdef VTSS_SW_OPTION_CPUPORT
                } else if (vtss_cpuport_is_interface(n, RTA_PAYLOAD(rta), &ifindex)) {
                    T_NG(IP_TRACE_GRP_NETLINK, "Interface: %s", vtss::str(n, e));
#endif /* VTSS_SW_OPTION_CPUPORT */
                } else {
                    T_NG(IP_TRACE_GRP_NETLINK, "Unrecognized interface: %s", vtss::str(n, e));
                    return;
                }

                break;
            }

            case IFLA_ADDRESS: {
                if (RTA_PAYLOAD(rta) == 6) {
                    memcpy(st.mac.addr, RTA_DATA(rta), 6);
                    has_mac = true;
                } else {
                    T_EG(IP_TRACE_GRP_NETLINK, "Un-expected address length: %d", RTA_PAYLOAD(rta));
                }

                break;
            }

            case IFLA_BROADCAST: {
                if (RTA_PAYLOAD(rta) == 6) {
                    memcpy(st.bcast.addr, RTA_DATA(rta), 6);
                    has_bcast = true;
                } else {
                    T_EG(IP_TRACE_GRP_NETLINK, "Un-expected address length: %d", RTA_PAYLOAD(rta));
                }

                break;
            }

            case IFLA_MTU: {
                if (RTA_PAYLOAD(rta) == 4) {
                    int *i = (int *)RTA_DATA(rta);
                    st.mtu = *i;
                } else {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected length: %d", RTA_PAYLOAD(rta));
                }

                break;
            }

            case IFLA_PROTINFO: {
                struct rtattr *rta1 = (rtattr *)RTA_DATA(rta);
                int len1 = rta->rta_len;

                T_NG(IP_TRACE_GRP_NETLINK, "IFLA_PROTINFO");

                while (RTA_OK(rta1, len1)) {
                    if (rta1->rta_type == IFLA_INET6_FLAGS) {
                        uint32_t ipv6_flags = *(uint32_t *)RTA_DATA(rta1);

                        T_NG(IP_TRACE_GRP_NETLINK, "Index: %u, IPv6 flags: 0x%x", ifi->ifi_index, ipv6_flags);

                        if (ipv6_flags & 0x40 /* IF_RA_MANAGED */) {
                            flags |= VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_MANAGED;
                        }

                        if (ipv6_flags & 0x80 /* IF_RA_OTHERCONF */) {
                            flags |= VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_OTHER;
                        }
                    }

                    rta1 = RTA_NEXT(rta1, len1);
                }

                break;
            }

            default:
                break;
            }

            rta = RTA_NEXT(rta, len);
        }

        if (!vtss_ifindex_is_none(ifindex)) {
            if (has_mac && has_bcast) {
                if (!data.set(ifindex, st)) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Failed to insert - out-of-memory");
                    ok_ = false;
                } else {
                    T_DG(IP_TRACE_GRP_NETLINK, "Link-Add: %s %s", ifindex, st);
                }
            } else if (flags) {
                auto itr = data.find(ifindex);
                if (itr != data.end()) {
                    st = itr->second;
                    st.flags |= flags;
                    T_DG(IP_TRACE_GRP_NETLINK, "Link-Upd: %s %s", ifindex, st);
                    data.set(ifindex, st);
                }
            }
        } else {
            T_DG(IP_TRACE_GRP_NETLINK, "Skipping invalid entry %u, has_mac = %d, has_bcast = %d", vid, has_mac, has_bcast);
        }
    }

    bool ok() const
    {
        return ok_;
    }

    // Cache is needed because we must delete before adding!
    bool ok_ = true;
    vtss::Map<vtss_ifindex_t, vtss_appl_ip_if_status_link_t> data;
};

/******************************************************************************/
// LinkStateChipCallback
/******************************************************************************/
struct LinkStateChipCallback : public StatusIfLink::Callback {
    LinkStateChipCallback(State &_s, vtss::Map<vtss_ifindex_t, vtss_appl_ip_if_link_flags_t> &_ra_flags_chgd_map) : state_ref(_s), ra_flags_chgd_map(_ra_flags_chgd_map) {}

    void add(const vtss_ifindex_t &ifindex, vtss_appl_ip_if_status_link_t &st) override
    {
        mesa_vid_t vid;
        mesa_rc    rc;

        T_IG(IP_TRACE_GRP_NETLINK, "LINK-ADD %s: %s", ifindex, st);

        if (vtss_ifindex_is_cpu(ifindex)) {
            state_ref.os_ifindex_to_ifindex.set(st.os_ifindex, ifindex);
            return;
        }

        if ((vid = vtss_ifindex_as_vlan(ifindex)) == 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "Not a VLAN: %u", ifindex);
            return;
        }

        if ((rc = vtss_ip_chip_rleg_add(vid)) != VTSS_RC_OK) {
            T_EG(IP_TRACE_GRP_NETLINK, "vtss_ip_chip_rleg_add(%u) failed: %s", vid, error_txt(rc));
            goto ERROR0;
        }

        if ((rc = IP_OS_hw_ip_mac_subscribe(vid, &st.bcast)) != VTSS_RC_OK) {
            T_EG(IP_TRACE_GRP_NETLINK, "IP_OS_hw_ip_mac_subscribe(%u::%s) failed: %s", vid, st.bcast, error_txt(rc));
            goto ERROR1;
        }

        if ((rc = IP_OS_hw_ip_mac_subscribe(vid, &st.mac)) != VTSS_RC_OK) {
            T_EG(IP_TRACE_GRP_NETLINK, "IP_OS_hw_ip_mac_subscribe(%u::%s) failed: %s", vid, st.mac, error_txt(rc));
            goto ERROR2;
        }

        state_ref.os_ifindex_to_ifindex.set(st.os_ifindex, ifindex);

        return;

ERROR2:
        (void)IP_OS_hw_ip_mac_unsubscribe(vid, &st.bcast);

ERROR1:
        (void)vtss_ip_chip_rleg_del(vid);

ERROR0:
        T_EG(IP_TRACE_GRP_NETLINK, "Failed to add VLAN I/F %s", ifindex);
    }

    void mod(const vtss_ifindex_t &ifindex, const vtss_appl_ip_if_status_link_t &before, vtss_appl_ip_if_status_link_t &after) override
    {
        mesa_vid_t                   vid;
        vtss_appl_ip_if_link_flags_t ra_flg = VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_MANAGED | VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_OTHER;
        mesa_rc                      rc = VTSS_RC_OK;

        T_IG(IP_TRACE_GRP_NETLINK, "LINK-MOD %s %s->%s", ifindex, before, after);

        if ((vid = vtss_ifindex_as_vlan(ifindex)) == 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "Not a VLAN (%u)", ifindex);
            return;
        }

        if (before.mac != after.mac) {
            T_IG(IP_TRACE_GRP_NETLINK, "Interface %s. Update MAC address %s->%s", ifindex, before.mac, after.mac);
            DO_(IP_OS_hw_ip_mac_unsubscribe, vid, &before.mac);
            DO_(IP_OS_hw_ip_mac_subscribe, vid, &after.mac);
        }

        if (before.bcast != after.bcast) {
            T_IG(IP_TRACE_GRP_NETLINK, "Interface %s. Update B/C MAC address %s->%s", ifindex, before.bcast, after.bcast);
            DO_(IP_OS_hw_ip_mac_unsubscribe, vid, &before.bcast);
            DO_(IP_OS_hw_ip_mac_subscribe, vid, &after.bcast);
        }

        if ((before.flags & ra_flg) != (after.flags & ra_flg)) {
            // Gotta defer the call to ip_dhcp6c_ra_flags_change() until after
            // we modify the entries of status_if_link, because that function
            // will call ip_os_ifindex_from_ifindex(), which attempts to get the
            // mutex protecting status_if_link, which we currently hold.
            // So we save these entries into its own map, which is iterated
            // across after we have released the status_if_link mutex.
            T_IG(IP_TRACE_GRP_NETLINK, "RA flags changed: 0x%x->0x%x", before.flags, after.flags);
            ra_flags_chgd_map.set(ifindex, after.flags);
        }
    }

    void del(const vtss_ifindex_t &ifindex, vtss_appl_ip_if_status_link_t &st) override
    {
        mesa_vid_t vid;
        mesa_rc    rc = VTSS_RC_OK;

        T_IG(IP_TRACE_GRP_NETLINK, "LINK-DEL %s: %s", ifindex, st);

        if (vtss_ifindex_is_cpu(ifindex)) {
            state_ref.os_ifindex_to_ifindex.erase(st.os_ifindex);
            return;
        }

        if ((vid = vtss_ifindex_as_vlan(ifindex)) == 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "Not a VLAN (%u)", ifindex);
            return;
        }

        T_IG(IP_TRACE_GRP_NETLINK, "Deleting interface %s", ifindex);
        DO_(IP_OS_hw_ip_mac_unsubscribe, vid, &st.mac);
        DO_(IP_OS_hw_ip_mac_unsubscribe, vid, &st.bcast);
        DO_(vtss_ip_chip_rleg_del, vid);

        for (auto i = state_ref.mac_addr_list.begin(); i != state_ref.mac_addr_list.end();) {
            if (i->vid != vid) {
                ++i;
            } else {
                auto m = i->mac;
                state_ref.mac_addr_list.erase(i++);
                DO_(IP_OS_hw_ip_mac_unsubscribe, vid, &m);
            }
        }

        state_ref.os_ifindex_to_ifindex.erase(st.os_ifindex);
    }

    State &state_ref;
    vtss::Map<vtss_ifindex_t, vtss_appl_ip_if_link_flags_t> &ra_flags_chgd_map;
};

/******************************************************************************/
// IP_OS_poll_link_state()
/******************************************************************************/
static void IP_OS_poll_link_state(void)
{
    LinkStateCallbackPoll cb;
    vtss::Map<vtss_ifindex_t, vtss_appl_ip_if_link_flags_t> ra_flags_chgd_map;

    if (!cb.ok()) {
        T_EG(IP_TRACE_GRP_NETLINK, "Out of memory");
        return;
    }

    T_DG(IP_TRACE_GRP_NETLINK, "Polling link state");
    IP_OS_nl_req_link_dump(&cb);
    IP_OS_nl_req_link_ipv6_dump(&cb);

    SYNCHRONIZED(state) {
        LinkStateChipCallback cb2(state, ra_flags_chgd_map);
        status_if_link.set(cb.data, cb2);
    }

    for (const auto &itr : ra_flags_chgd_map) {
        // Deferred call to ip_dhcp6c_ra_flags_change() to avoid double mutex
        // acquisition.
        T_IG(IP_TRACE_GRP_NETLINK, "ifindex = %s: New RA flags: 0x%x", itr.first, itr.second);
        ip_dhcp6c_ra_flags_change(itr.first, itr.second);
    }

    T_DG(IP_TRACE_GRP_NETLINK, "Polling link state - done");
}

/******************************************************************************/
// NetlinkCallbackCopyIfMacAddrList
/******************************************************************************/
struct NetlinkCallbackCopyIfMacAddrList : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        int           len = n->nlmsg_len;
        struct ndmsg  *ndm;
        struct rtattr *rta;

        T_NG(IP_TRACE_GRP_NETLINK, "CB type: %s", netlink::AsRtmType(n->nlmsg_type));
        T_NG(IP_TRACE_GRP_NETLINK, "%s", *n);

        if (n->nlmsg_type != RTM_NEWNEIGH && n->nlmsg_type != RTM_GETNEIGH) {
            return;
        }

        ndm = (struct ndmsg *)NLMSG_DATA(n);
        if ((len -= NLMSG_LENGTH(sizeof(*ndm))) < 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "<msg too short for this type!>");
            return;
        }

        if ((ndm->ndm_flags & NTF_SELF) == 0) {
            // Seems like the I/F Mux sets NTF_SELF, so if it's not set, it must
            // be someone else's interface, so that it's not for us.
            return;
        }

        rta = NDM_RTA(ndm);

        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case NDA_LLADDR: {
                if (RTA_PAYLOAD(rta) != 6) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size (%d)", RTA_PAYLOAD(rta));
                    break;
                }

                unsigned char *m = (unsigned char *)RTA_DATA(rta);
                std::pair<int, mesa_mac_t> d;
                d.first = ndm->ndm_ifindex;
                for (int i = 0; i < 6; ++i) {
                    d.second.addr[i] = m[i];
                }

                T_DG(IP_TRACE_GRP_NETLINK, "MAC: os_ifindex = %u: %s", d.first, d.second);
                data.emplace(d);
                break;
            }

            default:
                break;
            }

            rta = RTA_NEXT(rta, len);
        }
    }

    vtss::Set<std::pair<int, mesa_mac_t>> data;
};

/******************************************************************************/
// IP_OS_poll_link_mac_address_list()
/******************************************************************************/
static void IP_OS_poll_link_mac_address_list(void)
{
    NetlinkCallbackCopyIfMacAddrList p;

    T_DG(IP_TRACE_GRP_NETLINK, "Polling interface mac-address list");
    IP_OS_nl_req_neigh_dump(&p, AF_BRIDGE);

    vtss::Set<mesa_vid_mac_t> pending;
    vtss::Set<mesa_vid_mac_t> data_;

    SYNCHRONIZED(state) {
        for (auto &e : p.data) {
            // convert data.first from ifindex to vlan
            auto itr = state.os_ifindex_to_ifindex.find(e.first);
            if (itr == state.os_ifindex_to_ifindex.end()) {
                continue;
            }

            if (vtss_ifindex_is_vlan(itr->second)) {
                mesa_vid_mac_t d;
                d.vid = vtss_ifindex_as_vlan(itr->second);
                d.mac = e.second;
                data_.emplace(d);
            }
        }

        p.data.clear();

        auto cache = state.mac_addr_list.begin();
        auto cache_end = state.mac_addr_list.end();

        auto data = data_.begin();
        auto data_end = data_.end();

        while (cache != cache_end || data != data_end) {
            if (data == data_end || (cache != cache_end && *cache < *data)) {
                // Entry found in cache but not in data -> DELETE EVENT
                T_IG(IP_TRACE_GRP_NETLINK, "MAC-DEL: %s", *cache);
                if (IP_OS_hw_ip_mac_unsubscribe(cache->vid, &cache->mac) != VTSS_RC_OK) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Failed to unsubscribe %s", *cache);
                }

                state.mac_addr_list.erase(cache++);
            } else if (cache == cache_end || (data != data_end && *data < *cache)) {
                // Entry found in data but not in cache -> ADD EVENT
                T_IG(IP_TRACE_GRP_NETLINK, "MAC-ADD: %s", *data);
                if (IP_OS_hw_ip_mac_subscribe(data->vid, &data->mac) == VTSS_RC_OK) {
                    pending.emplace(*data);
                } else {
                    T_EG(IP_TRACE_GRP_NETLINK, "Failed to subscribe %s", *data);
                }

                data++;
            } else {
                // Entry found in both -> dont care
                T_DG(IP_TRACE_GRP_NETLINK, "MAC-NO-CHANGE: %s", *cache);
                data++;
                cache++;
            }
        }

        for (const auto &e : pending) {
            state.mac_addr_list.insert(e);
        }
    }
}

/******************************************************************************/
// NetlinkCallbackCopyIfIpv4AddrList
/******************************************************************************/
struct NetlinkCallbackCopyIfIpv4AddrList : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        struct ifaddrmsg            *ifa;
        int                         len;
        bool                        got_addr;
        vtss_appl_ip_if_key_ipv4_t  key = {};
        vtss_appl_ip_if_info_ipv4_t val = {};

        T_NG(IP_TRACE_GRP_NETLINK, "CB type: %s", netlink::AsRtmType(n->nlmsg_type));
        T_NG(IP_TRACE_GRP_NETLINK, "%s", *n);

        if (n->nlmsg_type != RTM_NEWADDR && n->nlmsg_type != RTM_GETADDR) {
            return;
        }

        ifa = (struct ifaddrmsg *)NLMSG_DATA(n);
        len = n->nlmsg_len;
        len -= NLMSG_LENGTH(sizeof(*ifa));

        if (len < 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "<msg too short for this type!>");
            return;
        }

        got_addr = false;

        struct rtattr *rta = IFA_RTA(ifa);
        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case IFA_ADDRESS:
                if (RTA_PAYLOAD(rta) != 4) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                key.ifindex          = ip_os_ifindex_to_ifindex(ifa->ifa_index);
                key.addr.prefix_size = ifa->ifa_prefixlen;
                key.addr.address     = IP_OS_u8_to_u32((u8 *)RTA_DATA(rta));

                T_NG(IP_TRACE_GRP_NETLINK, "Got os_ifindex = %u: %s", ifa->ifa_index, key);
                got_addr = key.ifindex != VTSS_IFINDEX_NONE;
                break;

            case IFA_BROADCAST:
                if (RTA_PAYLOAD(rta) != 4) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                val.broadcast = IP_OS_u8_to_u32((u8 *)RTA_DATA(rta));
                break;

            default:
                break;
            }

            rta = RTA_NEXT(rta, len);
        }

        if (got_addr) {
            // Use info trace, since there's no subsequent filtering on the
            // data.
            T_IG(IP_TRACE_GRP_NETLINK, "ADDR4-ADD: %s,  %s", key, val);
            data.emplace(key, val);
        }
    }

    vtss::Map<vtss_appl_ip_if_key_ipv4_t, vtss_appl_ip_if_info_ipv4_t> data;
};

/******************************************************************************/
// IP_OS_poll_ipv4_state()
/******************************************************************************/
static void IP_OS_poll_ipv4_state(void)
{
    NetlinkCallbackCopyIfIpv4AddrList p;

    T_DG(IP_TRACE_GRP_NETLINK, "Polling ipv4 addr list");
    IP_OS_nl_req_ipaddr_dump(&p, AF_INET);
    status_if_ipv4.set(p.data);
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// NetlinkCallbackCopyIfIpv6AddrList
/******************************************************************************/
struct NetlinkCallbackCopyIfIpv6AddrList : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        struct ifaddrmsg            *ifa;
        struct rtattr               *rta;
        int                         len;
        bool                        got_addr;
        vtss_appl_ip_if_key_ipv6_t  key = {};
        vtss_appl_ip_if_info_ipv6_t val = {};

        T_NG(IP_TRACE_GRP_NETLINK, "CB type: %s", netlink::AsRtmType(n->nlmsg_type));
        T_NG(IP_TRACE_GRP_NETLINK, "%s", *n);

        if (n->nlmsg_type != RTM_NEWADDR && n->nlmsg_type != RTM_GETADDR) {
            return;
        }

        ifa = (struct ifaddrmsg *)NLMSG_DATA(n);
        len = n->nlmsg_len;
        len -= NLMSG_LENGTH(sizeof(*ifa));

        if (len < 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "<msg too short for this type!>");
            return;
        }

        got_addr = false;

        rta = IFA_RTA(ifa);
        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case IFA_ADDRESS: {
                if (RTA_PAYLOAD(rta) != 16) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                key.ifindex          = ip_os_ifindex_to_ifindex(ifa->ifa_index);
                key.addr.prefix_size = ifa->ifa_prefixlen;
                memcpy(key.addr.address.addr, RTA_DATA(rta), 16);

                val.os_ifindex       = ifa->ifa_index;
                val.flags            = VTSS_APPL_IP_IF_IPV6_FLAG_NONE;

#define F(X, Y) if (ifa->ifa_flags & (X)) val.flags |= (Y)
                F(IFA_F_NODAD,      VTSS_APPL_IP_IF_IPV6_FLAG_NODAD);
                F(IFA_F_TENTATIVE,  VTSS_APPL_IP_IF_IPV6_FLAG_TENTATIVE);
                F(IFA_F_DEPRECATED, VTSS_APPL_IP_IF_IPV6_FLAG_DEPRECATED);
                F(IFA_F_DADFAILED,  VTSS_APPL_IP_IF_IPV6_FLAG_DUPLICATED);
#undef F

                got_addr = key.ifindex != VTSS_IFINDEX_NONE;
                break;
            }

            // case IFA_FLAGS:
            // Future flags are placed in this 32bit field, because
            // ifa->ifa_flags is only 8 bits.

            default:
                break;
            }

            rta = RTA_NEXT(rta, len);
        }

        if (got_addr) {
            // Use info trace, since there's no subsequent filtering on the
            // data.
            T_IG(IP_TRACE_GRP_NETLINK, "ADDR6-ADD: %s,  %s", key, val);
            data.emplace(key, val);
        }
    }

    vtss::Map<vtss_appl_ip_if_key_ipv6_t, vtss_appl_ip_if_info_ipv6_t> data;
};
#endif // VTSS_SW_OPTION_IPV6

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_OS_poll_ipv6_state()
/******************************************************************************/
static void IP_OS_poll_ipv6_state(void)
{
    NetlinkCallbackCopyIfIpv6AddrList p;

    T_DG(IP_TRACE_GRP_NETLINK, "Polling IPv6 addr table");
    IP_OS_nl_req_ipaddr_dump(&p, AF_INET6);
    status_if_ipv6.set(p.data);
}
#endif // VTSS_SW_OPTION_IPV6

#if defined(VTSS_SW_OPTION_L3RT)
/******************************************************************************/
// NetlinkCallbackCopyIpv4RouteTable
/******************************************************************************/
struct NetlinkCallbackCopyIpv4RouteTable : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        struct rtmsg         *rtm;
        struct rtattr        *rta, *rta_nh, *rta_gw;
        unsigned char        *m;
        int                  os_ifindex, mp_len, len;
        bool                 got_dst, got_gateway, is_blackhole;
        mesa_routing_entry_t k = {};
        vtss_ifindex_t       ifindex;

        len = n->nlmsg_len;

        T_NG(IP_TRACE_GRP_NETLINK, "CB type: %s", netlink::AsRtmType(n->nlmsg_type));
        T_NG(IP_TRACE_GRP_NETLINK, "%s", *n);

        if (n->nlmsg_type != RTM_NEWROUTE && n->nlmsg_type != RTM_GETROUTE) {
            return;
        }

        rtm = (struct rtmsg *)NLMSG_DATA(n);
        len -= NLMSG_LENGTH(sizeof(*rtm));
        if (len < 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "<msg too short for this type!>");
            return;
        }

        got_dst      = false;
        got_gateway  = false;
        is_blackhole = rtm->rtm_type == RTN_BLACKHOLE;
        k.type       = MESA_ROUTING_ENTRY_TYPE_IPV4_UC;
        os_ifindex   = 0;
        rta          = RTM_RTA(rtm);

        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case RTA_DST: {
                if (RTA_PAYLOAD(rta) != 4) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                m = (unsigned char *)RTA_DATA(rta);
                k.route.ipv4_uc.network.prefix_size = rtm->rtm_dst_len;
                k.route.ipv4_uc.network.address     = IP_OS_u8_to_u32(m);

                if (vtss_ipv4_addr_is_routable(&k.route.ipv4_uc.network.address)) {
                    // We do not install non-routable IPv4 addresses, because
                    // these are not routable (SIC!). Instead, we have a static
                    // entry in the LPM-table (installed by ip_os.cxx), that
                    // makes sure to redirect all non-routable entries
                    // (169.254.0.0./16) to the CPU.
                    got_dst = true;
                }

                break;
            }

            case RTA_MULTIPATH: {
                mp_len = rta->rta_len;
                rta_nh = (struct rtattr *)RTA_DATA(rta);
                for ( ; RTA_OK(rta_nh, mp_len); rta_nh = RTA_NEXT(rta_nh, mp_len)) {
                    if (rta_nh->rta_len != 16) {
                        continue;
                    }

                    rta_nh->rta_len = 8;
                    rta_gw = RTA_NEXT(rta_nh, len);
                    rta_nh->rta_len = 16;
                    if (rta_gw->rta_type == RTA_GATEWAY && RTA_PAYLOAD(rta_gw) == 4) {
                        m = (unsigned char *)RTA_DATA(rta_gw);
                        k.route.ipv4_uc.destination = IP_OS_u8_to_u32(m);

                        if (got_dst) {
                            T_DG(IP_TRACE_GRP_NETLINK, "ROUTE4-ADD-MP: %s", k);
                            data.emplace(k, false);
                        } else {
                            T_DG(IP_TRACE_GRP_NETLINK, "ROUTE4-DISCARD-MP: %s", k);
                        }
                    }
                }

                return;
            }

            case RTA_GATEWAY: {
                if (RTA_PAYLOAD(rta) != 4) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                m = (unsigned char *)RTA_DATA(rta);
                k.route.ipv4_uc.destination = IP_OS_u8_to_u32(m);

                got_gateway = true;
                break;
            }

            case RTA_OIF: {
                if (RTA_PAYLOAD(rta) != 4) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                int *x = (int *)RTA_DATA(rta);
                os_ifindex = *x;
                break;
            }

            case RTA_TABLE: {
                if (RTA_PAYLOAD(rta) != 4) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                int *x = (int *)RTA_DATA(rta);

                // Filtering out loop-back routes
                if (*x != RT_TABLE_MAIN) {
                    return;
                }

                break;
            }

            default:
                break;
            }

            rta = RTA_NEXT(rta, len);
        }

        if (is_blackhole) {
            if (got_gateway) {
                T_EG(IP_TRACE_GRP_NETLINK, "Blackholes don't have a gateway (k = %s)", k);
            }

            k.route.ipv4_uc.destination = vtss_ipv4_blackhole_route;
        }

        ifindex = ip_os_ifindex_to_ifindex(os_ifindex);
        if ((got_gateway || got_dst) && (ifindex != VTSS_IFINDEX_NONE || is_blackhole)) {
            T_DG(IP_TRACE_GRP_NETLINK, "ROUTE4-ADD: %s, ifindex = %s", k, ifindex);
            data.emplace(k, false);
        } else {
            T_DG(IP_TRACE_GRP_NETLINK, "ROUTE4-DISCARD: %s, ifindex = %s", k, ifindex);
        }
    }

    vtss::Map<mesa_routing_entry_t, bool> data;
};
#endif // VTSS_SW_OPTION_L3RT

#if defined(VTSS_SW_OPTION_L3RT)
/******************************************************************************/
// IP_OS_poll_ipv4_route_state()
/******************************************************************************/
static void IP_OS_poll_ipv4_route_state(void)
{
    NetlinkCallbackCopyIpv4RouteTable p;
    uint32_t                          rt_missing, tmp;
    uint64_t                          a, b;

    struct Ipv4RouteStateChangePrint : ip_os_routes_t::Callback {
        vtss::Vector<mesa_routing_entry_t> rt_add;
        vtss::Vector<mesa_routing_entry_t> rt_del;

        Ipv4RouteStateChangePrint() : rt_add(4096), rt_del(4096)
        {
            rt_add.grow_size(4096);
            rt_del.grow_size(4096);
        }

        // Invoked for entries in p.data not already in IP_OS_status_rt_ipv4.
        void add(const mesa_routing_entry_t &rt, bool &dummy) override
        {
            T_IG(IP_TRACE_GRP_NETLINK, "ROUTE4-ADD-NEW: %s", rt);
            rt_add.push_back(rt);
        };

        // Invoked whenever an entry in IP_OS_status_rt_ipv4 is removed, because
        // it doesn't exist in p.data.
        void del(const mesa_routing_entry_t &rt, bool &dummy) override
        {
            T_IG(IP_TRACE_GRP_NETLINK, "ROUTE4-DEL-OLD: %s", rt);
            rt_del.push_back(rt);
        };
    } cb;

    if (!fast_cap(MESA_CAP_L3)) {
        return;
    }

    T_DG(IP_TRACE_GRP_NETLINK, "Polling IPv4 route table");

    a = vtss::uptime_milliseconds();
    IP_OS_nl_req_route_dump(&p, AF_INET);
    b = vtss::uptime_milliseconds();
    T_DG(IP_TRACE_GRP_NETLINK, "Got: %zu entries, which took %s ms", p.data.size(),  (b - a) / 1000);

    a = vtss::uptime_milliseconds();
    // The following call overwrites the current IP_OS_status_rt_ipv4 contents,
    // while calling cb::add() with new entries, cb::del() with deleted
    // entries, and cb::mod() with modified entries (where only the value
    // is changed).
    IP_OS_status_rt_ipv4.set(p.data, cb);
    b = vtss::uptime_milliseconds();
    T_DG(IP_TRACE_GRP_NETLINK, "Event processing took %s ms", (b - a) / 1000);

    a = vtss::uptime_milliseconds();

    // Delete before adding
    rt_missing = cb.rt_del.size();
    mesa_routing_entry_t *rt_ptr = cb.rt_del.data();

    while (rt_missing) {
        tmp = 0;
        mesa_rc rc = vtss_ip_chip_route_bulk_del(rt_missing, rt_ptr, &tmp);
        T_IG(IP_TRACE_GRP_NETLINK, "ROUTE4-CHIP-DEL %u/%u", rt_missing, tmp);
        if (rc == VTSS_RC_OK) {
            rt_ptr     += tmp;
            rt_missing -= tmp;
        } else {
            break;
        }
    }

    // Add routes
    rt_missing = cb.rt_add.size();
    rt_ptr = cb.rt_add.data();
    while (rt_missing) {
        tmp = 0;
        mesa_rc rc = vtss_ip_chip_route_bulk_add(rt_missing, rt_ptr, &tmp);
        T_IG(IP_TRACE_GRP_NETLINK, "ROUTE4-CHIP-ADD %u/%u", rt_missing, tmp);
        if (rc == VTSS_RC_OK) {
            rt_ptr     += tmp;
            rt_missing -= tmp;
        } else {
            IP_OS_route_table_full_report();
            break;
        }
    }

    b = vtss::uptime_milliseconds();

    T_DG(IP_TRACE_GRP_NETLINK, "Took %s ms. Add-cnt = %zu, del-cnt = %zu", (b - a) / 1000, cb.rt_add.size(), cb.rt_del.size());
}
#endif // VTSS_SW_OPTION_L3RT

#if defined(VTSS_SW_OPTION_L3RT)
/******************************************************************************/
// NetlinkCallbackCopyIpv6RouteTable
/******************************************************************************/
struct NetlinkCallbackCopyIpv6RouteTable : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        struct rtmsg         *rtm;
        struct rtattr        *rta, *rta_nh, *rta_gw;
        struct rtnexthop     *nh;
        unsigned char        *m;
        int                  os_ifindex, mp_len, len;
        bool                 got_dst, got_gateway, is_blackhole;
        mesa_routing_entry_t k = {};
        vtss_ifindex_t       ifindex;

        len = n->nlmsg_len;

        T_NG(IP_TRACE_GRP_NETLINK, "CB type: %s", netlink::AsRtmType(n->nlmsg_type));
        T_NG(IP_TRACE_GRP_NETLINK, "%s", *n);

        if (n->nlmsg_type != RTM_NEWROUTE && n->nlmsg_type != RTM_GETROUTE) {
            return;
        }

        rtm = (struct rtmsg *)NLMSG_DATA(n);
        len -= NLMSG_LENGTH(sizeof(*rtm));
        if (len < 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "<msg too short for this type!>");
            return;
        }

        got_dst      = false;
        got_gateway  = false;
        is_blackhole = rtm->rtm_type == RTN_BLACKHOLE;
        k.type       = MESA_ROUTING_ENTRY_TYPE_IPV6_UC;
        os_ifindex   = 0;
        rta          = RTM_RTA(rtm);

        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case RTA_DST: {
                if (RTA_PAYLOAD(rta) != 16) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                m = (unsigned char *)RTA_DATA(rta);
                k.route.ipv6_uc.network.prefix_size = rtm->rtm_dst_len;
                memcpy(k.route.ipv6_uc.network.address.addr, m, 16);

                if (!vtss_ipv6_addr_is_multicast(&k.route.ipv6_uc.network.address) &&
                    vtss_ipv6_addr_is_routable(&k.route.ipv6_uc.network.address)) {
                    // We do not install link-local IPv6 addresses, because
                    // these are not routable. Instead, we have a static entry
                    // in the LPM-table (installed by ip_os.cxx), that makes
                    // sure to redirect all link-local (fe80::/10) entries to
                    // the CPU.
                    got_dst = true;
                }

                break;
            }

            case RTA_MULTIPATH: {
                mp_len = rta->rta_len;
                rta_nh = (struct rtattr *)RTA_DATA(rta);
                for ( ; RTA_OK(rta_nh, mp_len); rta_nh = RTA_NEXT(rta_nh, mp_len)) {
                    if (rta_nh->rta_len != 28) {
                        continue;
                    }

                    nh = (struct rtnexthop *)rta_nh;
                    ifindex = ip_os_ifindex_to_ifindex(nh->rtnh_ifindex);
                    rta_nh->rta_len = 8;
                    rta_gw = RTA_NEXT(rta_nh, len);
                    rta_nh->rta_len = 28;
                    if (rta_gw->rta_type == RTA_GATEWAY && RTA_PAYLOAD(rta_gw) == 16) {
                        m = (unsigned char *)RTA_DATA(rta_gw);
                        memcpy(k.route.ipv6_uc.destination.addr, m, 16);
                        if (got_dst && ifindex != VTSS_IFINDEX_NONE) {
                            k.vlan = vtss_ifindex_as_vlan(ifindex);
                            T_DG(IP_TRACE_GRP_NETLINK, "ROUTE6-ADD-MP: %s", k);
                            data.emplace(k, false);
                        } else {
                            T_DG(IP_TRACE_GRP_NETLINK, "ROUTE6-DISCARD-MP: %s", k);
                        }
                    }
                }

                return;
            }

            case RTA_GATEWAY: {
                if (RTA_PAYLOAD(rta) != 16) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                m = (unsigned char *)RTA_DATA(rta);
                memcpy(k.route.ipv6_uc.destination.addr, m, 16);
                got_gateway = true;

                break;
            }

            case RTA_OIF: {
                if (RTA_PAYLOAD(rta) != 4) {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    break;
                }

                int *x = (int *)RTA_DATA(rta);
                os_ifindex = *x;
                break;
            }

            default:
                break;
            }

            rta = RTA_NEXT(rta, len);
        }

        if (is_blackhole) {
            if (got_gateway) {
                T_EG(IP_TRACE_GRP_NETLINK, "Blackholes don't have a gateway (k = %s)", k);
            }

            k.route.ipv6_uc.destination = vtss_ipv6_blackhole_route;
        }

        ifindex = ip_os_ifindex_to_ifindex(os_ifindex);
        if ((got_gateway || got_dst) && (ifindex != VTSS_IFINDEX_NONE || is_blackhole)) {
            k.vlan = vtss_ifindex_as_vlan(ifindex);
            T_DG(IP_TRACE_GRP_NETLINK, "ROUTE6-ADD: %s, ifindex = %s", k, ifindex);
            data.emplace(k, false);
        } else {
            T_DG(IP_TRACE_GRP_NETLINK, "ROUTE6-DISCARD: %s, ifindex = %s", k, ifindex);
        }
    }

    vtss::Map<mesa_routing_entry_t, bool> data;
};
#endif // VTSS_SW_OPTION_L3RT

#if defined(VTSS_SW_OPTION_L3RT) && defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_OS_poll_ipv6_route_state()
/******************************************************************************/
static void IP_OS_poll_ipv6_route_state(void)
{
    NetlinkCallbackCopyIpv6RouteTable p;
    uint32_t                          rt_missing, tmp;
    uint64_t                          a, b;

    struct Ipv6RouteStateChangePrint : ip_os_routes_t::Callback {
        vtss::Vector<mesa_routing_entry_t> rt_add;
        vtss::Vector<mesa_routing_entry_t> rt_del;

        Ipv6RouteStateChangePrint() : rt_add(4096), rt_del(4096)
        {
            rt_add.grow_size(4096);
            rt_del.grow_size(4096);
        }

        // Invoked for entries in p.data not already in IP_OS_status_rt_ipv6.
        void add(const mesa_routing_entry_t &rt, bool &dummy) override
        {
            T_IG(IP_TRACE_GRP_NETLINK, "ROUTE6-ADD-NEW: %s", rt);
            rt_add.push_back(rt);
        };

        // Invoked whenever an entry in IP_OS_status_rt_ipv6 is removed, because
        // it doesn't exist in p.data.
        void del(const mesa_routing_entry_t &rt, bool &dummy) override
        {
            T_IG(IP_TRACE_GRP_NETLINK, "ROUTE6-DEL-OLD: %s", rt);
            rt_del.push_back(rt);
        };
    } cb;

    if (!fast_cap(MESA_CAP_L3)) {
        return;
    }

    T_DG(IP_TRACE_GRP_NETLINK, "Polling IPv6 route table");

    a = vtss::uptime_milliseconds();
    IP_OS_nl_req_route_dump(&p, AF_INET6);
    b = vtss::uptime_milliseconds();
    T_DG(IP_TRACE_GRP_NETLINK, "Got: %zu entries, which took %s ms", p.data.size(),  (b - a) / 1000);

    a = vtss::uptime_milliseconds();
    // The following call overwrites the current IP_OS_status_rt_ipv6 contents,
    // while calling cb::add() with new entries, cb::del() with deleted
    // entries, and cb::mod() with modified entries (where only the value
    // is changed).
    IP_OS_status_rt_ipv6.set(p.data, cb);
    b = vtss::uptime_milliseconds();
    T_DG(IP_TRACE_GRP_NETLINK, "Event processing took %s ms", (b - a) / 1000);

    a = vtss::uptime_milliseconds();

    // Delete before adding
    rt_missing = cb.rt_del.size();
    mesa_routing_entry_t *rt_ptr = cb.rt_del.data();

    while (rt_missing) {
        tmp = 0;
        mesa_rc rc = vtss_ip_chip_route_bulk_del(rt_missing, rt_ptr, &tmp);
        T_IG(IP_TRACE_GRP_NETLINK, "ROUTE6-CHIP-DEL %u/%u", rt_missing, tmp);
        if (rc == VTSS_RC_OK) {
            rt_ptr     += tmp;
            rt_missing -= tmp;
        } else {
            break;
        }
    }

    // Add routes
    rt_missing = cb.rt_add.size();
    rt_ptr = cb.rt_add.data();
    while (rt_missing) {
        tmp = 0;
        mesa_rc rc = vtss_ip_chip_route_bulk_add(rt_missing, rt_ptr, &tmp);
        T_IG(IP_TRACE_GRP_NETLINK, "ROUTE6-CHIP-ADD %u/%u", rt_missing, tmp);
        if (rc == VTSS_RC_OK) {
            rt_ptr     += tmp;
            rt_missing -= tmp;
        } else {
            IP_OS_route_table_full_report();
            break;
        }
    }

    b = vtss::uptime_milliseconds();

    T_DG(IP_TRACE_GRP_NETLINK, "Took %s ms. Add-cnt = %zu, del-cnt = %zu", (b - a) / 1000, cb.rt_add.size(), cb.rt_del.size());
}
#endif // VTSS_SW_OPTION_L3RT && VTSS_SW_OPTION_IPV6

/******************************************************************************/
// NetlinkCallbackNeighborList
// Invoked by netlink whenever we receive IPv4 neighbors.
/******************************************************************************/
struct NetlinkCallbackNeighborList : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        struct rtattr                  *rta;
        vtss_appl_ip_neighbor_key_t    k         = {};
        vtss_appl_ip_neighbor_status_t v         = {};
        bool                           ip_valid  = false;
        bool                           mac_valid = false;
        int                            len       = n->nlmsg_len;
        int                            i;
        struct ndmsg                   *ndm;
        unsigned char                  *m;

        T_NG(IP_TRACE_GRP_NETLINK, "CB type: %s", netlink::AsRtmType(n->nlmsg_type));
        T_NG(IP_TRACE_GRP_NETLINK, "%s", *n);

        if (n->nlmsg_type != RTM_NEWNEIGH) {
            return;
        }

        ndm = (struct ndmsg *)NLMSG_DATA(n);
        len -= NLMSG_LENGTH(sizeof(*ndm));
        if (len < 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "<msg too short for this type!>");
            return;
        }

        switch (ndm->ndm_family) {
        case AF_INET:
            k.dip.type = MESA_IP_TYPE_IPV4;
            v.type     = MESA_IP_TYPE_IPV4;
            break;

        case AF_INET6:
            k.dip.type = MESA_IP_TYPE_IPV6;
            v.type     = MESA_IP_TYPE_IPV6;
            break;

        default:
            return;
        }

        if ((ndm->ndm_state & NUD_NOARP) != 0) {
            // Multicast address or similar
            T_NG(IP_TRACE_GRP_NETLINK, "NUD_NOARP");
            return;
        }

        v.os_ifindex = ndm->ndm_ifindex;
        if ((k.ifindex = ip_os_ifindex_to_ifindex(v.os_ifindex)) == VTSS_IFINDEX_NONE) {
            // Unable to convert to a vtss_ifindex_t.
            T_NG(IP_TRACE_GRP_NETLINK, "Unknown os_ifindex (%u)", v.os_ifindex);
            return;
        }

        v.ifindex = k.ifindex;

        for (rta = NDM_RTA(ndm); RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
            m = (unsigned char *)RTA_DATA(rta);

            switch (rta->rta_type) {
            case NDA_DST:
                if (k.dip.type == MESA_IP_TYPE_IPV4) {
                    if (RTA_PAYLOAD(rta) == 4) {
                        ip_valid = true;
                        k.dip.addr.ipv4 = IP_OS_u8_to_u32(m);
                    } else {
                        T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    }
                } else {
                    if (RTA_PAYLOAD(rta) == 16) {
                        ip_valid = true;
                        for (i = 0; i < 16; i++) {
                            k.dip.addr.ipv6.addr[i] = m[i];
                        }
                    } else {
                        T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                    }
                }

                break;

            case NDA_LLADDR:
                if (RTA_PAYLOAD(rta) == 6) {
                    for (i = 0; i < 6; i++) {
                        v.dmac.addr[i] = m[i];
                    }

                    mac_valid = true;
                } else {
                    T_EG(IP_TRACE_GRP_NETLINK, "Unexpected size");
                }

                break;

            default:
                break;
            }
        }

        if (ip_valid && mac_valid) {
            v.flags = VTSS_APPL_IP_NEIGHBOR_FLAG_VALID;

            if ((ndm->ndm_flags & NTF_ROUTER) != 0) {
                v.flags |= VTSS_APPL_IP_NEIGHBOR_FLAG_ROUTER;
            }

            if ((ndm->ndm_state & NUD_PERMANENT) != 0) {
                v.flags |= VTSS_APPL_IP_NEIGHBOR_FLAG_PERMANENT;
            }

            if (k.dip.type == MESA_IP_TYPE_IPV4) {
                T_DG(IP_TRACE_GRP_NETLINK, "NB4-ADD: %s %s", k, v);
                data.emplace(k, v);
            } else {
                // Only add IPv6 neighbor if link-local address or IPv6 network
                // match
                if (vtss_ipv6_addr_is_link_local(&k.dip.addr.ipv6) || (vtss_ifindex_is_vlan(k.ifindex) && IP_OS_vlan_ipv6_network_match(k.ifindex, &k.dip.addr.ipv6))) {
                    T_DG(IP_TRACE_GRP_NETLINK, "NB6-ADD: %s %s", k, v);
                    data.emplace(k, v);
                }
            }
        } else {
            if (ip_valid) {
                T_DG(IP_TRACE_GRP_NETLINK, "NB%d-DISC (MAC not valid): %s", k.dip.type == MESA_IP_TYPE_IPV4 ? 4 : 6, k.dip);
            } else if (mac_valid) {
                T_DG(IP_TRACE_GRP_NETLINK, "NB%d-DISC (IP not valid): %s", k.dip.type == MESA_IP_TYPE_IPV4 ? 4 : 6, v);
            } else {
                T_DG(IP_TRACE_GRP_NETLINK, "NB%d-DISC (Neither IP nor MAC valid):", k.dip.type == MESA_IP_TYPE_IPV4 ? 4 : 6);
            }
        }
    }

    vtss::Map<vtss_appl_ip_neighbor_key_t, vtss_appl_ip_neighbor_status_t> data;
};

#if defined(VTSS_SW_OPTION_L3RT)
/******************************************************************************/
// IP_OS_neighbor_to_chip()
// Add/delete neighbor entry and host route
/******************************************************************************/
static mesa_rc IP_OS_neighbor_to_chip(const vtss_appl_ip_neighbor_key_t &k, vtss_appl_ip_neighbor_status_t &v, bool add)
{
    mesa_rc              rc;
    mesa_routing_entry_t rt;
    bool                 is_link_local = false;

    if (!vtss_ifindex_is_vlan(k.ifindex)) {
        T_DG(IP_TRACE_GRP_NETLINK, "Ignore interface: %s", k.ifindex);
        return VTSS_RC_OK;
    }

    if (k.dip.type == MESA_IP_TYPE_IPV4) {
        rt.type                              = MESA_ROUTING_ENTRY_TYPE_IPV4_UC;
        rt.route.ipv4_uc.network.address     = k.dip.addr.ipv4;
        rt.route.ipv4_uc.network.prefix_size = 32;
        rt.route.ipv4_uc.destination         = k.dip.addr.ipv4;
    } else if (vtss_ipv6_addr_is_link_local(&k.dip.addr.ipv6)) {
        // Avoid adding route entries for link-local addresses
        is_link_local = true;
    } else {
        rt.type                              = MESA_ROUTING_ENTRY_TYPE_IPV6_UC;
        rt.route.ipv6_uc.network.address     = k.dip.addr.ipv6;
        rt.route.ipv6_uc.network.prefix_size = 128;
        rt.route.ipv6_uc.destination         = k.dip.addr.ipv6;
        rt.vlan = vtss_ifindex_as_vlan(k.ifindex);
    }

    if (add) {
        if (!is_link_local) {
            if ((rc = vtss_ip_chip_route_add(&rt)) != VTSS_RC_OK) {
                IP_OS_route_table_full_report();
                return rc;
            }
        }

        if ((rc = vtss_ip_chip_neighbor_add(k, v)) == VTSS_RC_OK) {
            if (fast_cap(MESA_CAP_L3)) {
                // Entry is now in H/W.
                v.flags |= VTSS_APPL_IP_NEIGHBOR_FLAG_HARDWARE;
            }
        } else {
            if (!is_link_local) {
                // Neighbor add failed. Delete route again.
                (void)vtss_ip_chip_route_del(&rt);
            }
        }
    } else {
        VTSS_RC(vtss_ip_chip_neighbor_del(k, v));

        if (!is_link_local) {
            rc = vtss_ip_chip_route_del(&rt);
        } else {
            rc = VTSS_RC_OK;
        }
    }

    return rc;
}
#endif // VTSS_SW_OPTION_L3RT

/******************************************************************************/
// IP_OS_poll_neighbor_list()
/******************************************************************************/
static void IP_OS_poll_neighbor_list(mesa_ip_type_t type)
{
    NetlinkCallbackNeighborList p;

#if defined(VTSS_SW_OPTION_L3RT)
    // This class' add() and del() members are invoked as follows:
    // When p.data contains a neighbor, which is not already in status_nb_ipv4
    // or status_nb_ipv6, add() is invoked.
    // When status_nb_ipv4 or status_nb_ipv6 contains a neighbor, which is no
    // longer in p.data, del() is invoked.
    // These functions will make sure to add/delete the entry from the chip.
    struct NeighborChange : StatusNb::Callback {
        void add_del(const vtss_appl_ip_neighbor_key_t &k, vtss_appl_ip_neighbor_status_t &v, bool add)
        {
            T_IG(IP_TRACE_GRP_NETLINK, "NB%d-CHIP-%s: %s %s", k.dip.type == MESA_IP_TYPE_IPV4 ? 4 : 6, add ? "ADD" : "DEL", k, v);

            if (IP_OS_neighbor_to_chip(k, v, add) != VTSS_RC_OK) {
                T_WG(IP_TRACE_GRP_NETLINK, "%s neighbor %s %s failed", add ? "Add" : "Del", k, v);
            }
        }

        void add(const vtss_appl_ip_neighbor_key_t &k, vtss_appl_ip_neighbor_status_t &v) override
        {
            add_del(k, v, true);
        }

        void mod(const vtss_appl_ip_neighbor_key_t &k, const vtss_appl_ip_neighbor_status_t &before, vtss_appl_ip_neighbor_status_t &after) override
        {
            // Keep the flag possibly set by IP_OS_neighbor_to_chip()
            if (before.flags & VTSS_APPL_IP_NEIGHBOR_FLAG_HARDWARE) {
                after.flags |= VTSS_APPL_IP_NEIGHBOR_FLAG_HARDWARE;
            }
        }

        void del(const vtss_appl_ip_neighbor_key_t &k, vtss_appl_ip_neighbor_status_t &v) override
        {
            add_del(k, v, false);
        }
    } cb;
#endif // VTSS_SW_OPTION_L3RT

    T_DG(IP_TRACE_GRP_NETLINK, "Polling %s neighbor list", type == MESA_IP_TYPE_IPV4 ? "IPv4" : "IPv6");

    // Make a netlink request of all IPv4 members and store them in p.data
    IP_OS_nl_req_neigh_dump(&p, type == MESA_IP_TYPE_IPV4 ? AF_INET : AF_INET6);

#if defined(VTSS_SW_OPTION_L3RT)
    // The following call overwrites the current status_nb_ipv4 or
    // status_nb_ipv6 contents, while calling cb::add() with new entries,
    // cb::del() with deleted entries, and cb::mod() with modified entries
    // (where only the value is changed).
    if (type == MESA_IP_TYPE_IPV4) {
        status_nb_ipv4.set(p.data, cb);
    } else {
#if defined(VTSS_SW_OPTION_IPV6)
        status_nb_ipv6.set(p.data, cb);
#endif
    }
#else
    if (type == MESA_IP_TYPE_IPV4) {
        status_nb_ipv4.set(p.data);
    } else {
#if defined(VTSS_SW_OPTION_IPV6)
        status_nb_ipv6.set(p.data);
#endif
    }
#endif // VTSS_SW_OPTION_L3RT
}

/******************************************************************************/
// IP_OS_netlink_monitor_thread_msg()
/******************************************************************************/
static int IP_OS_netlink_monitor_thread_msg(struct nlmsghdr *nh, int len, ip_os_netlink_poll_t *poll)
{
    T_DG(IP_TRACE_GRP_NETLINK, "Got message of %d bytes: %s", len, nh);

    for (; NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len)) {
        switch (nh->nlmsg_type) {
        case NLMSG_DONE:
            T_DG(IP_TRACE_GRP_NETLINK, "NLMSG_DONE");
            break;

        case NLMSG_ERROR:
            T_EG(IP_TRACE_GRP_NETLINK, "NLMSG_ERROR");
            return -1;

        case RTM_NEWLINK:
        case RTM_DELLINK:
        case RTM_GETLINK:
            T_DG(IP_TRACE_GRP_NETLINK, "RTM_NEWLINK/RTM_DELLINK/RTM_GETLINK");
            poll->link = true;
            break;

        case RTM_NEWADDR:
        case RTM_DELADDR:
        case RTM_GETADDR: {
            T_DG(IP_TRACE_GRP_NETLINK, "RTM_NEWADDR/RTM_DELADDR/RTM_GETADDR");
            struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nh);
            if (len < NLMSG_LENGTH(sizeof(*ifa))) {
                T_EG(IP_TRACE_GRP_NETLINK, "msg too short for this type!");
                break;
            }

            if (ifa->ifa_family == AF_INET) {
                poll->ipv4_addr = true;
            } else if (ifa->ifa_family == AF_INET6) {
#if defined(VTSS_SW_OPTION_IPV6)
                poll->ipv6_addr = true;
#endif
            }

            break;
        }

#if defined(VTSS_SW_OPTION_L3RT)
        case RTM_NEWROUTE:
        case RTM_DELROUTE:
        case RTM_GETROUTE: {
            T_DG(IP_TRACE_GRP_NETLINK, "RTM_NEWROUTE/RTM_DELROUTE/RTM_GETROUTE");
            struct rtmsg *rtm = (struct rtmsg *)NLMSG_DATA(nh);
            if (len < NLMSG_LENGTH(sizeof(*rtm))) {
                T_EG(IP_TRACE_GRP_NETLINK, "msg too short for this type!");
                break;
            }

            if (rtm->rtm_family == AF_INET) {
                poll->ipv4_route = true;
            } else if (rtm->rtm_family == AF_INET6) {
#if defined(VTSS_SW_OPTION_IPV6)
                poll->ipv6_route = true;
#endif
            }

            break;
        }
#endif // VTSS_SW_OPTION_L3RT

        case RTM_GETNEIGH:
        case RTM_NEWNEIGH:
        case RTM_DELNEIGH: {
            T_DG(IP_TRACE_GRP_NETLINK, "RTM_GETNEIGH/RTM_NEWNEIGH/RTM_DELNEIGH");
            int len = nh->nlmsg_len;
            struct ndmsg *ndm = (struct ndmsg *)NLMSG_DATA(nh);
            len -= NLMSG_LENGTH(sizeof(*ndm));
            if (len < 0) {
                T_EG(IP_TRACE_GRP_NETLINK, "Msg too short");
                break;
            }

            if (ndm->ndm_family == AF_INET) {
                poll->ipv4_neighbor = true;
            } else if (ndm->ndm_family == AF_INET6) {
#if defined(VTSS_SW_OPTION_IPV6)
                poll->ipv6_neighbor = true;
#endif
            } else {
                poll->mac_addr_list = true;
            }

            break;
        }

        default:
            T_DG(IP_TRACE_GRP_NETLINK, "default");
        }
    }

    return 0;
}

/******************************************************************************/
// IP_OS_netlink_poll()
/******************************************************************************/
static void IP_OS_netlink_poll(ip_os_netlink_poll_t &poll)
{
    if (poll.link) {
        IP_OS_poll_link_state();
    }

    if (poll.mac_addr_list) {
        IP_OS_poll_link_mac_address_list();
    }

    if (poll.ipv4_addr) {
        IP_OS_poll_ipv4_state();
    }

#if defined(VTSS_SW_OPTION_IPV6)
    if (poll.ipv6_addr) {
        IP_OS_poll_ipv6_state();
    }
#endif

#if defined(VTSS_SW_OPTION_L3RT)
    if (poll.ipv4_route) {
        IP_OS_poll_ipv4_route_state();
    }
#endif

#if defined(VTSS_SW_OPTION_L3RT) && defined(VTSS_SW_OPTION_IPV6)
    if (poll.ipv6_route) {
        IP_OS_poll_ipv6_route_state();
    }
#endif

    if (poll.ipv4_neighbor) {
        IP_OS_poll_neighbor_list(MESA_IP_TYPE_IPV4);
    }

#if defined(VTSS_SW_OPTION_IPV6)
    if (poll.ipv6_neighbor) {
        IP_OS_poll_neighbor_list(MESA_IP_TYPE_IPV6);
    }
#endif
}

/******************************************************************************/
// IP_OS_thread()
/******************************************************************************/
static void IP_OS_thread(vtss_addrword_t data)
{
    char               buf[4096];
    int                len, fd, res;
    struct iovec       iov = {buf, sizeof(buf)};
    struct msghdr      msg;
    struct sockaddr_nl sa;
    bool               socket_ok;

    while (1) {
        T_DG(IP_TRACE_GRP_NETLINK, "Creating netlink socket");

        socket_ok = true;
        fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        int pid = 0;
        if (fd < 0) {
            T_EG(IP_TRACE_GRP_NETLINK, "Socket failed: %s", strerror(errno));
            socket_ok = false;
        }

        memset(&sa, 0, sizeof(sa));
        sa.nl_pid = pid;
        sa.nl_family = AF_NETLINK;
        sa.nl_groups = RTMGRP_LINK | RTMGRP_NOTIFY | RTMGRP_NEIGH |
                       RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE |
                       RTMGRP_IPV4_RULE | RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE |
                       RTMGRP_IPV6_IFINFO | RTMGRP_IPV6_PREFIX;

        if (fd >= 0) {
            res = bind(fd, (struct sockaddr *)&sa, sizeof(sa));
            if (res != 0) {
                T_EG(IP_TRACE_GRP_NETLINK, "Bind failed: %s", strerror(errno));
                socket_ok = false;
            }
        }

        // The netlink connection has (possibly) been reset, meaning that we
        // may have lost messages. We therefore need to poll all as we do not
        // know what messages have been lost.
        IP_OS_poll_link_state();
        IP_OS_poll_link_mac_address_list();
        IP_OS_poll_ipv4_state();

#if defined(VTSS_SW_OPTION_IPV6)
        IP_OS_poll_ipv6_state();
#endif

#if defined(VTSS_SW_OPTION_L3RT)
        IP_OS_poll_ipv4_route_state();
#endif

#if defined(VTSS_SW_OPTION_L3RT) && defined(VTSS_SW_OPTION_IPV6)
        IP_OS_poll_ipv6_route_state();
#endif

        IP_OS_poll_neighbor_list(MESA_IP_TYPE_IPV4);

#if defined(VTSS_SW_OPTION_IPV6)
        IP_OS_poll_neighbor_list(MESA_IP_TYPE_IPV6);
#endif

        while (socket_ok) {
            int res = 0;
            ip_os_netlink_poll_t poll;
            msg = {&sa, sizeof(sa), &iov, 1, NULL, 0, 0};
            int recvmsg_flag = 0;  // Must block at first call
            int cnt = 0;
            memset(&poll, 0, sizeof(poll));

            do {
                len = recvmsg(fd, &msg, recvmsg_flag);
                T_DG(IP_TRACE_GRP_NETLINK, "len = %d", len);
                if (len == -1) {
                    // Closing and re-opening the socket alleviates most
                    // problems, including EAGAIN and EWOULDBLOCK.
                    socket_ok = false;

                    // Make sure not to poll anything when breaking out.
                    vtss_clear(poll);
                    break;
                }

                res = IP_OS_netlink_monitor_thread_msg((struct nlmsghdr *)buf, len, &poll);

                // We now want to empty the queue but not block
                recvmsg_flag = MSG_DONTWAIT;
                cnt++;

                if (res != 0) {
                    socket_ok = false;
                }

                // We read the socket up to 100 times to avoid calling the poll
                // functions too many times in case of many new messages from
                // the kernel.
            } while (socket_ok && cnt < 100);

            IP_OS_netlink_poll(poll);
        }

        if (fd >= 0) {
            close(fd);
        }
    }
}

/******************************************************************************/
// IP_OS_sysctl_set_int()
/******************************************************************************/
static mesa_rc IP_OS_sysctl_set_int(const char *path, int i)
{
    char buf[128], *p = buf;
    int  cnt, res, len = sizeof(buf);

    if (snprintf(buf, len, "/proc/sys/net/%s", path) >= len) {
        return VTSS_RC_ERROR;
    }

    vtss::Fd fd(open(buf, O_WRONLY));
    len = 16;
    if (fd.raw() < 0 || (cnt = snprintf(buf, len, "%d\n", i)) >= len) {
        return VTSS_RC_ERROR;
    }

    while (cnt) {
        res = write(fd.raw(), p, cnt);
        if (res <= 0) {
            return VTSS_RC_ERROR;
        }

        p += res;
        cnt -= res;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_OS_ifname_from_vlan()
/******************************************************************************/
static vtss::str IP_OS_ifname_from_vlan(vtss::Buf *b, mesa_vid_t vid)
{
    vtss::BufPtrStream ss(b);

    ss << VTSS_APPL_IP_VLAN_IF_PREFIX << vid;
    ss.push(0);

    return vtss::str(ss.begin(), ss.end());
}

/******************************************************************************/
// IP_OS_ifname_from_ifindex()
/******************************************************************************/
static vtss::str IP_OS_ifname_from_ifindex(vtss::Buf *b, vtss_ifindex_t ifindex)
{
    if (vtss_ifindex_is_vlan(ifindex)) {
        vtss_ifindex_elm_t ife;
        (void)vtss_ifindex_decompose(ifindex, &ife);
        mesa_vid_t vid = ife.ordinal;
        return IP_OS_ifname_from_vlan(b, vid);
#ifdef VTSS_SW_OPTION_CPUPORT
    } else if (vtss_ifindex_is_cpu(ifindex)) {
        return cpuport_os_interface_name(b, ifindex);
#endif /* VTSS_SW_OPTION_CPUPORT */
    }

    vtss::BufPtrStream ss(b);
    return vtss::str(ss.begin(), ss.end());
}

/******************************************************************************/
// IP_OS_if_cmd()
/******************************************************************************/
static mesa_rc IP_OS_if_cmd(vtss::str ifname, int cmd, int flags, const char *func, int ifi_flags = 0, int ifi_change = 0, int mtu = -1)
{
    netlink::NetlinkCallbackPrint cb;
    static const char             *ifkind = "vtss_if_mux";
    int                           seq = netlink::netlink_seq();
    struct rtattr                 *linkinfo;
    mesa_mac_t                    m;
    mesa_rc                       rc;

    struct {
        struct nlmsghdr n;
        struct ifinfomsg i;
        char attr[1024];
    } req;

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_seq = seq;
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.n.nlmsg_flags = flags;
    req.n.nlmsg_type = cmd;
    req.i.ifi_family = AF_UNSPEC;
    req.i.ifi_change = ifi_change;
    req.i.ifi_flags = ifi_flags;

    // Add ifname
    if (ifname.size() >= IFNAMSIZ) {
        T_EG(IP_TRACE_GRP_NETLINK, "Invalid ifname (%s)", ifname);
        return VTSS_RC_ERROR;
    }

    DO(netlink::attr_add_binary, &req.n, sizeof(req), IFLA_IFNAME, ifname.begin(), ifname.size());

    // Prepare nested type: linkinfo/info_kind
    if ((linkinfo = netlink::attr_nest(&req.n, sizeof(req), IFLA_LINKINFO)) != nullptr) {
        DO(netlink::attr_add_str, &req.n, sizeof(req), IFLA_INFO_KIND, ifkind);
        netlink::attr_nest_end(&req.n, linkinfo);
    }

    if (cmd == RTM_NEWLINK) {
        if (conf_mgmt_mac_addr_get(m.addr, 0) == 0) {
            DO(netlink::attr_add_mac, &req.n, sizeof(req), IFLA_ADDRESS, m);
        } else {
            T_EG(IP_TRACE_GRP_NETLINK, "Failed to get mac address");
        }
    }

    if (mtu != -1) {
        DO(netlink::attr_add_binary, &req.n, sizeof(req), IFLA_MTU, &mtu, 4);
    }

    DO(nl_req, (const void *)&req, sizeof(req), seq, func, &cb, 2048, 2048);

    return rc;
}

/******************************************************************************/
// IP_OS_ipv4_cmd()
/******************************************************************************/
static mesa_rc IP_OS_ipv4_cmd(vtss_ifindex_t ifindex, int cmd, int flags, const mesa_ipv4_network_t *network, mesa_ipv4_t bcast = 0)
{
    netlink::NetlinkCallbackPrint cb;
    int                           os_ifindex = ip_os_ifindex_from_ifindex(ifindex);
    int                           seq = netlink::netlink_seq();
    mesa_rc                       rc;

    struct {
        struct nlmsghdr n;
        struct ifaddrmsg i;
        char attr[1024];
    } req;

    if (os_ifindex < 0) {
        T_EG(IP_TRACE_GRP_NETLINK, "No such interface found: %s", ifindex);
        return VTSS_RC_ERROR;
    }

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_seq = seq;
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req.n.nlmsg_flags = flags;
    req.n.nlmsg_type = cmd;
    req.i.ifa_family = AF_INET;
    req.i.ifa_index = os_ifindex;
    req.i.ifa_prefixlen = network->prefix_size;

    DO(netlink::attr_add_ipv4, &req.n, sizeof(req), IFA_LOCAL, network->address);
    DO(netlink::attr_add_ipv4, &req.n, sizeof(req), IFA_ADDRESS, network->address);
    if (bcast) {
        DO(netlink::attr_add_ipv4, &req.n, sizeof(req), IFA_BROADCAST, bcast);
    }

    DO(nl_req, (const void *)&req, sizeof(req), seq, __FUNCTION__, &cb, 2048, 2048);

    return rc;
}

/******************************************************************************/
// IP_OS_ipv4_rleg_update()
/******************************************************************************/
static void IP_OS_ipv4_rleg_update(mesa_vid_t vid, mesa_bool_t enable)
{
    mesa_l3_rleg_conf_t conf;

    if (mesa_l3_rleg_get_specific(NULL, vid, &conf) == VTSS_RC_OK) {
        conf.ipv4_unicast_enable = enable;
        (void)mesa_l3_rleg_update(NULL, &conf);
    }
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// IP_OS_ipv6_cmd()
/******************************************************************************/
static mesa_rc IP_OS_ipv6_cmd(vtss_ifindex_t ifindex, int cmd, int flags, const mesa_ipv6_network_t *network)
{
    netlink::NetlinkCallbackPrint cb;
    int                           os_ifindex = ip_os_ifindex_from_ifindex(ifindex);
    int                           seq = netlink::netlink_seq();
    mesa_rc                       rc;

    struct {
        struct nlmsghdr n;
        struct ifaddrmsg i;
        char attr[1024];
    } req;

    if (os_ifindex < 0) {
        T_EG(IP_TRACE_GRP_NETLINK, "No such interface found: %s", ifindex);
        return VTSS_APPL_IP_RC_OS_IF_NOT_FOUND;
    }

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_seq = seq;
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req.n.nlmsg_flags = flags;
    req.n.nlmsg_type = cmd;
    req.i.ifa_family = AF_INET6;
    req.i.ifa_index = os_ifindex;
    req.i.ifa_prefixlen = network->prefix_size;

    DO(netlink::attr_add_ipv6, &req.n, sizeof(req), IFA_LOCAL, network->address);
    DO(netlink::attr_add_ipv6, &req.n, sizeof(req), IFA_ADDRESS, network->address);
    DO(nl_req, (const void *)&req, sizeof(req), seq, __FUNCTION__, &cb, 2048, 2048);

    return rc;
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// IP_OS_kernel_neighbor_del()
// Deletes the neighbor in the kernel.
/******************************************************************************/
static mesa_rc IP_OS_kernel_neighbor_del(const mesa_ip_addr_t *ip, uint32_t os_ifindex)
{
    netlink::NetlinkCallbackPrint cb;
    int                           seq = netlink::netlink_seq();
    mesa_rc                       rc;

    struct {
        struct nlmsghdr n;
        struct ndmsg ndm;
        char attr[256];
    } req;

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_seq   = seq;
    req.n.nlmsg_len   = NLMSG_LENGTH(sizeof(struct ndmsg));
    req.n.nlmsg_flags = (NLM_F_REQUEST | NLM_F_ACK);
    req.n.nlmsg_type  = RTM_DELNEIGH;

    if (ip->type == MESA_IP_TYPE_IPV4) {
        req.ndm.ndm_family = AF_INET;
        DO(netlink::attr_add_ipv4, &req.n, sizeof(req), NDA_DST,  ip->addr.ipv4);
    } else {
        req.ndm.ndm_family = AF_INET6;
        DO(netlink::attr_add_ipv6, &req.n, sizeof(req), NDA_DST,  ip->addr.ipv6);
    }

    req.ndm.ndm_ifindex = os_ifindex;
    DO(nl_req, (const void *)&req, sizeof(req), seq, __FUNCTION__, &cb, 2048, 2048);

    return rc;
}

/******************************************************************************/
// IP_OS_parse_statistics()
/******************************************************************************/
static mesa_rc IP_OS_parse_statistics(vtss::Map<vtss::str, int64_t> &m, vtss::str buf, vtss::str type)
{
    vtss::parser::Int<int64_t, 10, 1> i;
    vtss::Vector<vtss::str> hdr(64), val(64);
    bool got_hdr = false, got_val = false;
    for (vtss::str line : vtss::LineIterator(buf)) {
        vtss::str h, t;
        if (!split(line, ':', h, t)) {
            continue;
        }

        t = trim(t);
        if (h == type) {
            if (!got_hdr) {
                hdr = split(t, ' ');
                got_hdr = true;
            } else {
                val = split(t, ' ');
                got_val = true;
                break;
            }
        }
    }

    if (!got_hdr || !got_val) {
        T_EG(IP_TRACE_GRP_OS, "Buf: %s\ngot_hdr: %d, got_val: %d", buf, got_hdr, got_val);
        return VTSS_RC_ERROR;
    }

    if (hdr.size() != val.size()) {
        T_EG(IP_TRACE_GRP_OS, "Buf: %s\nhdr.size(%zu) != val.size(%zu)", buf, hdr.size(), val.size());
        return VTSS_RC_ERROR;
    }

    for (auto h = hdr.begin(), v = val.begin(); h != hdr.end() && v != val.end(); ++h, ++v) {
        const char *b = v->begin(), *e = v->end();
        if (i(b, e) && b == e) {
            m.set(*h, i.get());
        } else {
            T_EG(IP_TRACE_GRP_OS, "Failed to parse: %s -> %s", *h, *v);
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// IP_OS_parse_ipv6_statistics()
/******************************************************************************/
static mesa_rc IP_OS_parse_ipv6_statistics(const char *name, vtss_appl_ip_statistics_t *statistics)
{
    auto buf = vtss::read_file_into_buf(name);
    if (!buf.size()) {
        T_DG(IP_TRACE_GRP_OS, "Failed to read %s", name);
        return VTSS_RC_ERROR;
    }

    vtss::Map<vtss::str, int64_t> m;
    for (vtss::str line : vtss::LineIterator(vtss::str(buf))) {
        vtss::parser::OneOrMore<vtss::parser::group::Word> w;
        vtss::parser::OneOrMore<vtss::parser::group::Space> sp;
        vtss::parser::Int<int64_t, 10, 1> i;

        const char *b = line.begin(), *e = line.end();
        if (vtss::parser::Group(b, e, w, sp, i)) {
            m.set(w.get(), i.get());
        } else {
            T_IG(IP_TRACE_GRP_OS, "Could not parse: %s",  line);
        }
    }

#define IP_OS_ASSIGN2(A, V)                                \
    {                                                      \
        auto i = m.find(vtss::str(A));                     \
        if (i == m.end()) {                                \
            T_IG(IP_TRACE_GRP_OS, "No such field: %s", A); \
        } else {                                           \
            statistics->V = i->second;                     \
            statistics->V##Valid = 1;                      \
        }                                                  \
    }

#define IP_OS_ASSIGN3(A, V1, V2)                           \
    {                                                      \
        auto i = m.find(vtss::str(A));                     \
        if (i == m.end()) {                                \
            T_IG(IP_TRACE_GRP_OS, "No such field: %s", A); \
        } else {                                           \
            statistics->V1 = i->second;                    \
            statistics->V2 = i->second;                    \
            statistics->V1##Valid = 1;                     \
            statistics->V2##Valid = 1;                     \
        }                                                  \
    }

    // Not used:
    //  - Ip6InTooBigErrors
    //  - Ip6ReasmTimeout
    //  - Ip6InBcastOctets
    //  - Ip6OutBcastOctets
    //  - Ip6InNoECTPkts
    //  - Ip6InECT1Pkts
    //  - Ip6InECT0Pkts
    //  - Ip6InCEPkts

    // Missing fields in vtss_appl_ip_statistics_t:
    //  - DiscontinuityTime
    //  - HCInBcastPkts, InBcastPkts
    //  - HCInForwDatagrams, InForwDatagrams
    //  - HCOutBcastPkts, OutBcastPkts
    //  - HCOutTransmits, OutTransmits
    //  - OutFragReqds
    //  - RefreshRate

    memset(statistics, 0, sizeof(*statistics));
    IP_OS_ASSIGN2("Ip6FragCreates",      OutFragCreates);
    IP_OS_ASSIGN2("Ip6FragFails",        OutFragFails);
    IP_OS_ASSIGN2("Ip6FragOKs",          OutFragOKs);
    IP_OS_ASSIGN2("Ip6InAddrErrors",     InAddrErrors);
    IP_OS_ASSIGN3("Ip6InDelivers",       InDelivers,       HCInDelivers);
    IP_OS_ASSIGN2("Ip6InDiscards",       InDiscards);
    IP_OS_ASSIGN2("Ip6InHdrErrors",      InHdrErrors);
    IP_OS_ASSIGN3("Ip6InMcastOctets",    InMcastOctets,    HCInMcastOctets);
    IP_OS_ASSIGN3("Ip6InMcastPkts",      InMcastPkts,      HCInMcastPkts);
    IP_OS_ASSIGN2("Ip6InNoRoutes",       InNoRoutes);
    IP_OS_ASSIGN3("Ip6InOctets",         InOctets,         HCInOctets);
    IP_OS_ASSIGN3("Ip6InReceives",       InReceives,       HCInReceives);
    IP_OS_ASSIGN2("Ip6InTruncatedPkts",  InTruncatedPkts);
    IP_OS_ASSIGN2("Ip6InUnknownProtos",  InUnknownProtos);
    IP_OS_ASSIGN2("Ip6OutDiscards",      OutDiscards);
    IP_OS_ASSIGN3("Ip6OutForwDatagrams", OutForwDatagrams, HCOutForwDatagrams);
    IP_OS_ASSIGN3("Ip6OutMcastOctets",   OutMcastOctets,   HCOutMcastOctets);
    IP_OS_ASSIGN3("Ip6OutMcastPkts",     OutMcastPkts,     HCOutMcastPkts);
    IP_OS_ASSIGN2("Ip6OutNoRoutes",      OutNoRoutes);
    IP_OS_ASSIGN3("Ip6OutOctets",        OutOctets,        HCOutOctets);
    IP_OS_ASSIGN3("Ip6OutRequests",      OutRequests,      HCOutRequests);
    IP_OS_ASSIGN2("Ip6ReasmFails",       ReasmFails);
    IP_OS_ASSIGN2("Ip6ReasmOKs",         ReasmOKs);
    IP_OS_ASSIGN2("Ip6ReasmReqds",       ReasmReqds);

    return VTSS_RC_OK;
}

/******************************************************************************/
//
// Public interface functions start here
//
/******************************************************************************/

/******************************************************************************/
// ip_os_debug_netlink_poll()
/******************************************************************************/
void ip_os_debug_netlink_poll(ip_os_netlink_poll_t *poll)
{
    if (!poll) {
        return;
    }

    IP_OS_netlink_poll(*poll);
}

/******************************************************************************/
// ip_os_ifindex_to_ifindex()
/******************************************************************************/
vtss_ifindex_t ip_os_ifindex_to_ifindex(int32_t os_ifindex)
{
    SYNCHRONIZED(state) {
        auto itr = state.os_ifindex_to_ifindex.find(os_ifindex);
        if (itr != state.os_ifindex_to_ifindex.end()) {
            return itr->second;
        }
    }

    return VTSS_IFINDEX_NONE;
}

/******************************************************************************/
// ip_os_ifindex_from_ifindex()
/******************************************************************************/
int32_t ip_os_ifindex_from_ifindex(vtss_ifindex_t ifindex)
{
    vtss_appl_ip_if_status_link_t link_st;

    if (status_if_link.get(ifindex, &link_st) != VTSS_RC_OK) {
        return -1;
    }

    return link_st.os_ifindex;
}

/******************************************************************************/
// ip_os_global_conf_set()
/******************************************************************************/
mesa_rc ip_os_global_conf_set(const vtss_appl_ip_global_conf_t *conf)
{
    int enable = conf->routing_enable ? 1 : 0;

    VTSS_RC(IP_OS_sysctl_set_int("ipv4/ip_forward", enable));
#ifdef VTSS_SW_OPTION_IPV6
    VTSS_RC(IP_OS_sysctl_set_int("ipv6/conf/all/forwarding", enable));
#endif

    return vtss_ip_chip_routing_enable(enable);
}

/******************************************************************************/
// ip_os_if_add()
/******************************************************************************/
mesa_rc ip_os_if_add(vtss_ifindex_t ifindex)
{
    vtss::StaticBuffer<IFNAMSIZ> buf;
    mesa_rc                      rc;

    T_IG(IP_TRACE_GRP_OS, "%s", IP_OS_ifname_from_ifindex(&buf, ifindex));
    rc = IP_OS_if_cmd(IP_OS_ifname_from_ifindex(&buf, ifindex), RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK, __FUNCTION__);
    IP_OS_poll_link_state();
    return rc;
}

/******************************************************************************/
// ip_os_if_add_chip_port()
/******************************************************************************/
mesa_rc ip_os_if_add_chip_port(uint32_t chip_port)
{
    char buf[IFNAMSIZ];

    snprintf(buf, sizeof(buf), "vtss.port.%u", chip_port);
    return IP_OS_if_cmd(vtss::str(buf), RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK, __FUNCTION__, IFF_UP, IFF_UP);
}

/******************************************************************************/
// ip_os_if_set()
/******************************************************************************/
mesa_rc ip_os_if_set(vtss_ifindex_t ifindex, vtss_appl_ip_if_conf_t *conf)
{
    vtss::StaticBuffer<IFNAMSIZ> buf;
    mesa_rc                      rc;

    T_IG(IP_TRACE_GRP_OS, "%u", ifindex);
    rc = IP_OS_if_cmd(IP_OS_ifname_from_ifindex(&buf, ifindex), RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK, __FUNCTION__, 0, 0, conf->mtu);
    IP_OS_poll_link_state();
    return rc;
}

/******************************************************************************/
// ip_os_if_ctl()
/******************************************************************************/
mesa_rc ip_os_if_ctl(vtss_ifindex_t ifindex, bool up)
{
    vtss::StaticBuffer<IFNAMSIZ> buf;
    mesa_rc                      rc;

    T_IG(IP_TRACE_GRP_OS, "%s: %s", ifindex, up ? "Up" : "Down");
    rc = IP_OS_if_cmd(IP_OS_ifname_from_ifindex(&buf, ifindex), RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK, __FUNCTION__, up ? IFF_UP : 0, IFF_UP);
    IP_OS_poll_link_state();

#if defined(VTSS_SW_OPTION_L3RT)
    IP_OS_poll_ipv4_route_state();
#endif

#if defined(VTSS_SW_OPTION_L3RT) && defined(VTSS_SW_OPTION_IPV6)
    IP_OS_poll_ipv6_route_state();
#endif // VTSS_SW_OPTION_L3RT && VTSS_SW_OPTION_IPV6

    return rc;
}

/******************************************************************************/
// ip_os_if_del()
/******************************************************************************/
mesa_rc ip_os_if_del(vtss_ifindex_t ifindex)
{
    vtss::StaticBuffer<IFNAMSIZ> buf;
    mesa_rc                      rc;

    T_IG(IP_TRACE_GRP_OS, "I/F: %s", ifindex);
    rc = IP_OS_if_cmd(IP_OS_ifname_from_ifindex(&buf, ifindex), RTM_DELLINK, NLM_F_REQUEST | NLM_F_ACK, __FUNCTION__);
    IP_OS_poll_link_state();
    return rc;
}

/******************************************************************************/
// ip_os_neighbor_clear()
// We remove all entries of the given type from the kernel, and await a kernel
// message to get out own status_nb_ipv4 or status_nb_ipv6 tables updated.
// Once this message comes, the neighbors will also be removed from the API.
/******************************************************************************/
mesa_rc ip_os_neighbor_clear(mesa_ip_type_t type)
{
    mesa_rc rc = VTSS_RC_OK;

    if (type == MESA_IP_TYPE_NONE || type == MESA_IP_TYPE_IPV4) {
        auto lock = status_nb_ipv4.lock_get(__FILE__, __LINE__);
        auto ref  = status_nb_ipv4.ref(lock);
        auto i    = ref.begin();
        auto e    = ref.end();

        for (; i != e; ++i) {
            if ((rc = IP_OS_kernel_neighbor_del(&i->first.dip, i->second.os_ifindex)) != VTSS_RC_OK) {
                break;
            }
        }
    }

#if defined(VTSS_SW_OPTION_IPV6)
    if (type == MESA_IP_TYPE_NONE || type == MESA_IP_TYPE_IPV6) {
        auto lock = status_nb_ipv6.lock_get(__FILE__, __LINE__);
        auto ref  = status_nb_ipv6.ref(lock);
        auto i    = ref.begin();
        auto e    = ref.end();

        for (; i != e; ++i) {
            if ((rc = IP_OS_kernel_neighbor_del(&i->first.dip, i->second.os_ifindex)) != VTSS_RC_OK) {
                break;
            }
        }
    }
#endif // VTSS_SW_OPTION_IPV6

    return rc;
}

/******************************************************************************/
// ip_os_ipv4_add()
/******************************************************************************/
mesa_rc ip_os_ipv4_add(vtss_ifindex_t ifindex, const mesa_ipv4_network_t *network)
{
    mesa_ipv4_t mask, bcast = network->address;
    mesa_vid_t  vid;
    mesa_rc     rc;

    T_IG(IP_TRACE_GRP_OS, "%s: IPV4-ADD %s", ifindex, *network);

    mask = vtss_ipv4_prefix_to_mask(network->prefix_size);
    bcast |= (~mask);

    rc = IP_OS_ipv4_cmd(ifindex, RTM_NEWADDR, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK, network, bcast);

    if ((vid = vtss_ifindex_as_vlan(ifindex)) != 0) {
        IP_OS_ipv4_rleg_update(vid, true);
    }

    IP_OS_poll_ipv4_state();
    return rc;
}

/******************************************************************************/
// ip_os_ipv4_del()
/******************************************************************************/
mesa_rc ip_os_ipv4_del(vtss_ifindex_t ifindex, const mesa_ipv4_network_t *network)
{
    mesa_vid_t vid;
    mesa_rc    rc;

    T_IG(IP_TRACE_GRP_OS, "%s: IPV4-DEL %s", ifindex, *network);

    rc = IP_OS_ipv4_cmd(ifindex, RTM_DELADDR, NLM_F_REQUEST | NLM_F_ACK, network);

    if ((vid = vtss_ifindex_as_vlan(ifindex)) != 0) {
        IP_OS_ipv4_rleg_update(vid, false);
    }

    IP_OS_poll_ipv4_state();
    return rc;
}

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// ip_os_ipv6_add()
/******************************************************************************/
mesa_rc ip_os_ipv6_add(vtss_ifindex_t ifindex, const mesa_ipv6_network_t *network)
{
    mesa_rc rc;

    T_IG(IP_TRACE_GRP_OS, "%s: IPV6-ADD %s", ifindex, *network);

    rc = IP_OS_ipv6_cmd(ifindex, RTM_NEWADDR, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK, network);

    IP_OS_poll_ipv6_state();

    SYNCHRONIZED(state) {
        state.vid_to_ipv6_network.set(ifindex, *network);
    }

    IP_OS_poll_neighbor_list(MESA_IP_TYPE_IPV6);
    return rc;
}
#endif // VTSS_SW_OPTION_IPV6

#if defined(VTSS_SW_OPTION_IPV6)
/******************************************************************************/
// ip_os_ipv6_del()
/******************************************************************************/
mesa_rc ip_os_ipv6_del(vtss_ifindex_t ifindex, const mesa_ipv6_network_t *network, bool if_up)
{
    mesa_rc rc;

    T_IG(IP_TRACE_GRP_OS, "%s: IPV6-DEL %s", ifindex, *network);

    if (if_up) {
        // The IPv6 addresses only exist in the kernel if the VLAN interface is
        // up. If down, the kernel auto-deletes IPv6 addresses.
        rc = IP_OS_ipv6_cmd(ifindex, RTM_DELADDR, NLM_F_REQUEST | NLM_F_ACK, network);
    } else {
        // Don't attempt to remove the IPv6 address, since it's already gone.
        rc = VTSS_RC_OK;
    }

    IP_OS_poll_ipv6_state();

    SYNCHRONIZED(state) {
        state.vid_to_ipv6_network.erase(ifindex);
    }

    IP_OS_poll_neighbor_list(MESA_IP_TYPE_IPV6);
    return rc;
}
#endif // VTSS_SW_OPTION_IPV6

/******************************************************************************/
// ip_os_system_statistics_ipv4_get()
/******************************************************************************/
mesa_rc ip_os_system_statistics_ipv4_get(vtss_appl_ip_statistics_t *statistics)
{
    vtss::FixedBuffer b1 = vtss::read_file_into_buf("/proc/net/snmp");
    if (!b1.size()) {
        T_EG(IP_TRACE_GRP_OS, "Failed to read /proc/net/snmp");
        return VTSS_RC_ERROR;
    }

    vtss::FixedBuffer b2 = vtss::read_file_into_buf("/proc/net/netstat");
    if (!b2.size()) {
        T_EG(IP_TRACE_GRP_OS, "Failed to read /proc/net/netstat");
        return VTSS_RC_ERROR;
    }

    vtss::Map<vtss::str, int64_t> m;
    VTSS_RC(IP_OS_parse_statistics(m, vtss::str(b1), vtss::str("Ip")));
    VTSS_RC(IP_OS_parse_statistics(m, vtss::str(b2), vtss::str("IpExt")));

// Not used:
//  - DefaultTTL
//  - Forwarding
//  - InBcastOctets
//  - InCEPkts
//  - InCsumErrors
//  - InECT0Pkts
//  - InECT1Pkts
//  - InNoECTPkts
//  - OutBcastOctets
//  - ReasmTimeout

// Missing fields in vtss_appl_ip_statistics_t:
//  - DiscontinuityTime
//  - HCInForwDatagrams, InForwDatagrams
//  - HCOutTransmits, OutTransmits
//  - OutFragReqds
//  - RefreshRate

    memset(statistics, 0, sizeof(*statistics));
    IP_OS_ASSIGN3("ForwDatagrams",   OutForwDatagrams, HCOutForwDatagrams);
    IP_OS_ASSIGN2("FragCreates",     OutFragCreates);
    IP_OS_ASSIGN2("FragFails",       OutFragFails);
    IP_OS_ASSIGN2("FragOKs",         OutFragOKs);
    IP_OS_ASSIGN2("InAddrErrors",    InAddrErrors);
    IP_OS_ASSIGN3("InBcastPkts",     InBcastPkts,     HCInBcastPkts);
    IP_OS_ASSIGN3("InDelivers",      InDelivers,      HCInDelivers);
    IP_OS_ASSIGN2("InDiscards",      InDiscards);
    IP_OS_ASSIGN2("InHdrErrors",     InHdrErrors);
    IP_OS_ASSIGN3("InMcastOctets",   InMcastOctets,   HCInMcastOctets);
    IP_OS_ASSIGN3("InMcastPkts",     InMcastPkts,     HCInMcastPkts);
    IP_OS_ASSIGN2("InNoRoutes",      InNoRoutes);
    IP_OS_ASSIGN3("InOctets",        InOctets,        HCInOctets);
    IP_OS_ASSIGN3("InReceives",      InReceives,      HCInReceives);
    IP_OS_ASSIGN2("InTruncatedPkts", InTruncatedPkts);
    IP_OS_ASSIGN2("InUnknownProtos", InUnknownProtos);
    IP_OS_ASSIGN3("OutBcastPkts",    OutBcastPkts,    HCOutBcastPkts);
    IP_OS_ASSIGN2("OutDiscards",     OutDiscards);
    IP_OS_ASSIGN3("OutMcastOctets",  OutMcastOctets,  HCOutMcastOctets);
    IP_OS_ASSIGN3("OutMcastPkts",    OutMcastPkts,    HCOutMcastPkts);
    IP_OS_ASSIGN2("OutNoRoutes",     OutNoRoutes);
    IP_OS_ASSIGN3("OutOctets",       OutOctets,       HCOutOctets);
    IP_OS_ASSIGN3("OutRequests",     OutRequests,     HCOutRequests);
    IP_OS_ASSIGN2("ReasmFails",      ReasmFails);
    IP_OS_ASSIGN2("ReasmOKs",        ReasmOKs);
    IP_OS_ASSIGN2("ReasmReqds",      ReasmReqds);

    return VTSS_RC_OK;
}

/******************************************************************************/
// ip_os_system_statistics_ipv6_get()
/******************************************************************************/
mesa_rc ip_os_system_statistics_ipv6_get(vtss_appl_ip_statistics_t *statistics)
{
    return IP_OS_parse_ipv6_statistics("/proc/net/snmp6", statistics);
}

/******************************************************************************/
// ip_os_if_statistics_link_get()
/******************************************************************************/
mesa_rc ip_os_if_statistics_link_get(mesa_vid_t vlan, vtss_appl_ip_if_statistics_t *statistics)
{
    const   char *p, *b = NULL, *e;
    int     cnt = 0;
    int64_t val[16];
    vtss::parser::Int<int64_t, 10, 1> i;

    auto buf = vtss::read_file_into_buf("/proc/net/dev");
    if (!buf.size()) {
        T_EG(IP_TRACE_GRP_OS, "Failed to read /proc/net/dev");
        return VTSS_RC_ERROR;
    }

    memset(statistics, 0, sizeof(*statistics));
    vtss::StaticBuffer<IFNAMSIZ> name;
    vtss::str h, t, ifname = IP_OS_ifname_from_vlan(&name, vlan);
    for (vtss::str line : vtss::LineIterator(vtss::str(buf))) {
        if (split(line, ':', h, t) && strncmp(h.begin(), ifname.begin(), ifname.size() - 1) == 0) {
            for (p = t.begin(); p != t.end(); p++) {
                if (isdigit(*p)) {
                    if (b == NULL) {
                        b = p;
                    }
                } else if (b != NULL) {
                    e = p;
                    if (i(b, e) && b == e) {
                        val[cnt++] = i.get();
                    }

                    b = NULL;
                    if (cnt == 10) {
                        statistics->in_bytes = val[0];
                        statistics->in_packets = val[1];
                        statistics->out_bytes = val[8];
                        statistics->out_packets = val[9];
                        return VTSS_RC_OK;
                    }
                }
            }
        }
    }

    return VTSS_RC_ERROR;
}

/******************************************************************************/
// ip_os_if_statistics_ipv6_get()
/******************************************************************************/
mesa_rc ip_os_if_statistics_ipv6_get(mesa_vid_t vid, vtss_appl_ip_statistics_t *statistics)
{
    vtss::StaticBuffer<IFNAMSIZ> name;
    char                         buf[64], *p = buf;
    const char                   *c;
    vtss::str                    ifname = IP_OS_ifname_from_vlan(&name, vid);

    p += sprintf(p, "/proc/net/dev_snmp6/");
    for (c = ifname.begin(); c < ifname.end(); c++, p++) {
        *p = *c;
    }

    *p = '\0';

    return IP_OS_parse_ipv6_statistics(buf, statistics);
}

/******************************************************************************/
// ip_os_init()
/******************************************************************************/
mesa_rc ip_os_init(void)
{
    T_DG(IP_TRACE_GRP_OS, "Enter");
    state.init(__LINE__, "ip_netlink_monitor");
    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       IP_OS_thread,
                       0,
                       "IP_netlink_monitor",
                       nullptr,
                       0,
                       &IP_OS_thread_handle,
                       &IP_OS_thread_block);

    // Enable gratuitous ARP
    VTSS_RC(IP_OS_sysctl_set_int("ipv4/conf/default/arp_notify", 1));

    // Avoid IPv4 routes with link down
    VTSS_RC(IP_OS_sysctl_set_int("ipv4/conf/default/ignore_routes_with_linkdown", 1));

    // Disable IPv6 on vtss.ifh to avoid incrementing global IPv6 statistics
    VTSS_RC(IP_OS_sysctl_set_int("ipv6/conf/" VTSS_NPI_DEVICE "/disable_ipv6", 1));

    // use IN6_ADDR_GEN_MODE_RANDOM to comply with RFC 7217
    VTSS_RC(IP_OS_sysctl_set_int("ipv6/conf/all/addr_gen_mode", 3));

    // vtss.ifh should always be up.
    // Link state is only being managed on the derived VLAN interfaces
    (void)IP_OS_if_cmd(vtss::str(VTSS_NPI_DEVICE), RTM_SETLINK, NLM_F_REQUEST | NLM_F_ACK, __FUNCTION__, IFF_UP, IFF_UP);

    return VTSS_RC_OK;
}

