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

#include "vtss/appl/ip.h"
#include "types.hxx"
#include "vtss/basics/string.hxx"
#include "vtss/basics/preprocessor.h"
#include "vtss/basics/formatting_tags.hxx"
#include "dhcp_pool.hxx"
#include "dhcp_client.hxx"
#include "dhcp_frame.hxx"
#include "webstax_frame_service.hxx"
#include "ip_api.h"
#include "vtss_common_os.h" /* For vtss_os_get_portmac() */
#include "misc_api.h" /* For misc_hexstr_to_array() */
#include <iostream>

#define PRINTF(...)                                         \
    if (size - s > 0) {                                     \
        int res = snprintf(buf + s, size - s, __VA_ARGS__); \
        if (res >0 ) {                                      \
            s += res;                                       \
        }                                                   \
    }

#define PRINTFUNC(F, ...)                       \
    if (size - s > 0) {                         \
        s += F(buf + s, size - s, __VA_ARGS__); \
    }

#include "main.h"
#include "critd_api.h"
#include "packet_api.h"
#include "sysutil_api.h"
#include "dhcp_client_api.h"
#include "ip_utils.hxx"
#include <vtss/basics/vector.hxx>
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
#include "dhcp_helper_api.h"
#endif
#include "subject.hxx"

const char *dhcp4c_state_to_txt(vtss_appl_ip_dhcp4c_state_t s)
{
    switch(s) {
    case VTSS_APPL_IP_DHCP4C_STATE_STOPPED:         return "STOPPED";
    case VTSS_APPL_IP_DHCP4C_STATE_INIT:            return "INIT";
    case VTSS_APPL_IP_DHCP4C_STATE_SELECTING:       return "SELECTING";
    case VTSS_APPL_IP_DHCP4C_STATE_REQUESTING:      return "REQUESTING";
    case VTSS_APPL_IP_DHCP4C_STATE_REBINDING:       return "REBINDING";
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND:           return "BOUND";
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK: return "BOUND_ARP_CHECK";
    case VTSS_APPL_IP_DHCP4C_STATE_RENEWING:        return "RENEWING";
    case VTSS_APPL_IP_DHCP4C_STATE_FALLBACK:        return "FALLBACK";
    default:                                        return "UNKNOWN";
    }
}

namespace vtss {
namespace dhcp {

ostream &operator<<(ostream &o, const ConfPacket &e) {
    o << "{ xid:" << FormatHex<const uint32_t>(e.xid, 'a', 0, 0, '0') << " ip:" << e.ip << " server_mac:" << e.server_mac;

    o << " server_ip:";
    if (e.server_ip.valid()) {
        auto x = e.server_ip.get();
        o << vtss::AsIpv4(x);
    } else {
        o << "<none>";
    }

    o << " default_gateway:";
    if (e.default_gateway.valid()) {
        auto x = e.default_gateway.get();
        o << vtss::AsIpv4(x);
    } else {
        o << "<none>";
    }

    o << " domain_name_server:";
    if (e.domain_name_server.valid()) {
        auto x = e.domain_name_server.get();
        o << vtss::AsIpv4(x);
    } else {
        o << "<none>";
    }

    o << " domain_name:";
    if (e.domain_name.size()) {
        o << e.domain_name;
    }
    else {
        o << "<none>";
    }

    return o;
}

ostream &operator<<(ostream &o, const AckConfPacket &e) {
    if (e.valid())
        o << e.get();
    else
        o << "<none>";

    return o;
}

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DHCP_CLIENT

static vtss_trace_reg_t trace_reg = {
    VTSS_MODULE_ID_DHCP_CLIENT, "dhcpc", "DHCP client"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

static critd_t DHCP_CLIENT_crit;

#define DHCP_CLIENT_CRIT_ENTER()                 \
    critd_enter(&DHCP_CLIENT_crit,               \
                __FILE__, __LINE__)

#define DHCP_CLIENT_CRIT_EXIT()                  \
    critd_exit(&DHCP_CLIENT_crit,                \
               __FILE__, __LINE__)

#define DHCP_CLIENT_CRIT_ASSERT_LOCKED()         \
    critd_assert_locked(&DHCP_CLIENT_crit,       \
                        __FILE__, __LINE__)

#define DHCP_CLIENT_CRIT_RETURN(T, X) \
do {                          \
    T __val = (X);            \
    DHCP_CLIENT_CRIT_EXIT();          \
    return __val;             \
} while(0)

#define DHCP_CLIENT_CRIT_RETURN_RC(X)   \
    DHCP_CLIENT_CRIT_RETURN(mesa_rc, X)

static struct LockRef {
    void lock() { DHCP_CLIENT_CRIT_ENTER(); }
    void unlock() { DHCP_CLIENT_CRIT_EXIT(); }
} lock;

static vtss::WebStaXRawFrameService raw_frame_service(VTSS_MODULE_ID_DHCP_CLIENT);

typedef vtss::dhcp::DhcpPool<
    vtss::WebStaXRawFrameService,
    vtss::notifications::SubjectRunner,
    LockRef,
    VTSS_VIDS
> DhcpPool_t;
DhcpPool_t pool(raw_frame_service, vtss::notifications::subject_main_thread, lock);

#if defined(VTSS_SW_OPTION_DHCP_HELPER)
static int c_sock = 0;
static vtss::Vector<vtss_ifindex_t> vlans;
#endif /* VTSS_SW_OPTION_DHCP_HELPER */

#if !defined(VTSS_SW_OPTION_DHCP_HELPER)
static void DHCP_CLIENT_rx_filter_reg(void)
{
    using namespace vtss;
    void *packet_filter_id;
    packet_rx_filter_t packet_filter;

    packet_rx_filter_init(&packet_filter);
    packet_filter.modid = VTSS_MODULE_ID_DHCP_CLIENT;
    packet_filter.match = PACKET_RX_FILTER_MATCH_UDP_DST_PORT | PACKET_RX_FILTER_MATCH_ETYPE;
    packet_filter.prio = PACKET_RX_FILTER_PRIO_BELOW_NORMAL;
    packet_filter.cb = client_packet_handler;
    packet_filter.udp_dst_port_min = 68;
    packet_filter.udp_dst_port_max = 68;
    packet_filter.etype            = ETYPE_IPV4;

    mesa_rc rc = packet_rx_filter_register(&packet_filter, &packet_filter_id);
    if (rc != VTSS_RC_OK) {
        T_W("packet_rx_filter_register() failed");
    }
}
#endif /* !VTSS_SW_OPTION_DHCP_HELPER */

int to_txt(char *buf, int size,
           const vtss_appl_ip_if_status_dhcp4c_t *const st) {
    int s = 0;

    PRINTF("State: %s", dhcp4c_state_to_txt(st->state));

    switch (st->state) {
    case VTSS_APPL_IP_DHCP4C_STATE_SELECTING:
        /*
        PRINTF(" offers: [");

        if (st->offers.valid_offers == 0) {
            PRINTF("none");
        }

        for (i = 0; i < st->offers.valid_offers; ++i) {
            if (i != 0) {
                PRINTF(", ");
            }
            PRINTF(VTSS_IPV4N_FORMAT " from " VTSS_IPV4_FORMAT,
                   VTSS_IPV4N_ARG(st->offers.list[i].ip),
                   VTSS_IPV4_ARGS(st->offers.list[i].server_ip));
        }
        PRINTF("]");
        */
        break;

    case VTSS_APPL_IP_DHCP4C_STATE_REQUESTING:
    case VTSS_APPL_IP_DHCP4C_STATE_REBINDING:
    case VTSS_APPL_IP_DHCP4C_STATE_BOUND:
    case VTSS_APPL_IP_DHCP4C_STATE_RENEWING:
        PRINTF(" server: " VTSS_IPV4_FORMAT, VTSS_IPV4_ARGS(st->server_ip));
        break;

    default:
        ;
    }

    return s;
}

#define E PP_TUPLE(ip,                          \
                   server_mac,                  \
                   server_ip,                   \
                   default_gateway,             \
                   domain_name_server,          \
                   domain_name,                 \
                   vendor_specific_information, \
                   boot_file_name               \
)
ConfPacket::ConfPacket(const ConfPacket& rhs) {
    VTSS_PP_STRUCT_IMPL_ASSIGN(E);
}
ConfPacket& ConfPacket::operator=(const ConfPacket& rhs) {
    VTSS_PP_STRUCT_IMPL_ASSIGN(E);
    return *this;
}
bool ConfPacket::operator==(const ConfPacket& rhs) {
    VTSS_PP_STRUCT_IMPL_EQUAL(E);
}
bool ConfPacket::operator!=(const ConfPacket& rhs) {
    VTSS_PP_STRUCT_IMPL_NOT_EQUAL(E);
}
void ConfPacket::clear() {
    ip.clear();
    server_mac.clear();
    server_ip.clear();
    default_gateway.clear();
    domain_name_server.clear();
    domain_name.clear();
    vendor_specific_information.clear();
    boot_file_name.clear();
}
#undef E

/* Opening socket bound to UDP port 68. */
int dhcp_client_open_socket()
{
    struct sockaddr_in addr_in;
    int sock = 0;

    T_D("Enter");

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock <= 0) {
        T_D("socket() failed");
        return sock;
    }

    memset(&addr_in, 0, sizeof(addr_in));

    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(68);
    addr_in.sin_addr.s_addr = INADDR_ANY;

    int flag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag)) < 0) {
        T_D("Could not set SO_REUSEADDR on socket");
        close(sock);
        return -1;
    }

    if (bind(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)) < 0) {
        T_D("bind() failed. Closing socket.");
        close(sock);
        return -1;
    }

    T_D("Exit after opening socket.");
    return sock;

}

mesa_rc client_start(vtss_ifindex_t ifidx, const vtss_appl_ip_dhcp4c_param_t *params)
{
    vtss_appl_ip_if_status_link_t if_status_link;
    bool                          if_is_up;

    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, INFO) << "Start dhcp client on " << ifidx;
    //    T_I("Start dhcp client on vlan %u", vlan);
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
        //Receive DHCP packet from DHCP helper
        dhcp_helper_user_receive_register(DHCP_HELPER_USER_CLIENT, client_packet_handler);
        if (vlans.size() == 0) {
            T_D("Opening client socket.");
            DHCP_CLIENT_CRIT_ENTER();
            c_sock = dhcp_client_open_socket();
            DHCP_CLIENT_CRIT_EXIT();
        }
        auto it = find(vlans.begin(), vlans.end(), ifidx);
        if (it == vlans.end()) {
            vlans.push_back(ifidx);
        }
#endif /* VTSS_SW_OPTION_DHCP_HELPER */

    DHCP_CLIENT_CRIT_ENTER();

    if (vtss_appl_ip_if_status_link_get(ifidx, &if_status_link) == VTSS_RC_OK && (if_status_link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP)) {
        if_is_up = true;
    } else {
        // Currently, the interface is not up, so don't start the whole state
        // machine, 'cause it doesn't make sense. We rely on the fact that the
        // IP module calls us back in client_if_up_down() when status changes.
        if_is_up = false;
    }

    T_I("pool.start(%s, if_is_up = %d)", ifidx, if_is_up);
    DHCP_CLIENT_CRIT_RETURN_RC(pool.start(ifidx, params, if_is_up));
}

mesa_rc client_stop(vtss_ifindex_t ifidx) {
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, INFO) << "Stopping dhcp client on " << ifidx;
    //    T_I("Stopping dhcp client on vlan %u", vlan);
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
        //Remove not to receive DHCP packet from DHCP helper
        dhcp_helper_user_receive_unregister(DHCP_HELPER_USER_CLIENT);
#endif
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.stop(ifidx));
}

mesa_rc client_fallback(vtss_ifindex_t ifidx) {
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, INFO) << "Fallback dhcp client on " << ifidx;
    //    T_I("Fallback dhcp client on vlan %u", vlan);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.fallback(ifidx));
}

mesa_rc client_kill(vtss_ifindex_t ifidx) {
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, INFO) << "Kill dhcp client on " << ifidx;
    //    T_I("Kill dhcp client on vlan %u", vlan);
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
        //Remove not to receive DHCP packet from DHCP helper
        dhcp_helper_user_receive_unregister(DHCP_HELPER_USER_CLIENT);
        auto it = find(vlans.begin(), vlans.end(), ifidx);
        if (it != vlans.end()) {
            vlans.erase(it);
        }
        if (vlans.size() == 0) {
            T_D("Closing client socket.");
            int close_sock;
            DHCP_CLIENT_CRIT_ENTER();
            close_sock = c_sock;
            c_sock = 0;
            DHCP_CLIENT_CRIT_EXIT();
            close(close_sock);
        }

#endif
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.kill(ifidx));
}

mesa_rc client_if_down(vtss_ifindex_t ifidx) {
    T_I("if_down dhcp client on %s", ifidx);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.if_down(ifidx));
}

mesa_rc client_if_up(vtss_ifindex_t ifidx) {
    T_I("if_up dhcp client on %s", ifidx);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.if_up(ifidx));
}

mesa_rc client_release(vtss_ifindex_t ifidx) {
    T_I("release dhcp client on %s", ifidx);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.release(ifidx));
}

mesa_rc client_decline(vtss_ifindex_t ifidx) {
    T_I("decline dhcp client on %s", ifidx);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.decline(ifidx));
}

mesa_rc client_bind(vtss_ifindex_t ifidx) {
    T_I("bind dhcp client on %s", ifidx);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.bind(ifidx));
}

BOOL client_bound_get(vtss_ifindex_t ifidx) {
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN(BOOL, pool.bound_get(ifidx));
}

static void client_if_up_down(vtss_ifindex_t ifidx) {

    vtss_appl_ip_if_status_link_t if_status_link;
    if (vtss_appl_ip_if_status_link_get(ifidx, &if_status_link) != VTSS_RC_OK) {
        return;
    }

    if (if_status_link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) {
        T_I("Client if up (%s)", ifidx);
        client_if_up(ifidx);
    } else {
        T_I("Client if down (%s)", ifidx);
        client_if_down(ifidx);
    }
}

mesa_rc client_offers_get(vtss_ifindex_t                ifidx,
                          size_t                        max_offers,
                          size_t                       *valid_offers,
                          ConfPacket                   *list) {
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, INFO) << "offers_get dhcp client on " << ifidx;
    //    T_I("offers_get dhcp client on vlan %u", vlan);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.offers_get(ifidx, max_offers,
                                               valid_offers, list));
}

mesa_rc client_offer_accept(vtss_ifindex_t ifidx,
                            unsigned       idx) {
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, INFO) << "accept dhcp client on " << ifidx;
    //    T_I("accept dhcp client on vlan %u", vlan);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.accept(ifidx, idx));
}

mesa_rc client_status(vtss_ifindex_t                     ifidx,
                      vtss_appl_ip_if_status_dhcp4c_t *status) {
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, NOISE) << "status dhcp client on " << ifidx;
    //    T_N("status dhcp client on vlan %u", vlan);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.status(ifidx, status));
}

mesa_rc client_callback_add(vtss_ifindex_t    ifidx,
                            client_callback_t v1) {
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, INFO) << "cb_add dhcp client on " << ifidx;
    //    T_I("cb_add dhcp client on vlan %u", vlan);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.callback_add(ifidx, v1));
}

mesa_rc client_callback_del(vtss_ifindex_t    ifidx,
                            client_callback_t v1) {
    VTSS_TRACE(VTSS_TRACE_GRP_DEFAULT, INFO) << "cb_del dhcp client on " << ifidx;
    //    T_I("cb_del dhcp client on vlan %u", vlan);
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.callback_del(ifidx, v1));
}

mesa_rc client_fields_get(vtss_ifindex_t ifidx, ConfPacket *v1) {
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.fields_get(ifidx, v1));
}

mesa_rc client_dns_option_ip_any_get(mesa_ipv4_t  prefered,
                                     mesa_ipv4_t *ip) {
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.dns_option_any_get(prefered, ip));
}

mesa_rc client_dns_option_domain_any_get(vtss::Buffer *name) {
    DHCP_CLIENT_CRIT_ENTER();
    DHCP_CLIENT_CRIT_RETURN_RC(pool.dns_option_domain_any_get(name));
}

size_t client_id_if_mac_get(char *buf, size_t max, vtss_ifindex_t ifidx) {
    mesa_rc               rc;
    vtss_ifindex_elm_t    elm;
    vtss_common_macaddr_t mac;

    if (!max) {
        return 0;
    }

    memset(&mac, 0, sizeof(mac));
    if ((rc = vtss_ifindex_decompose(ifidx, &elm)) == VTSS_RC_OK) {
        vtss_os_get_portmac(elm.ordinal, &mac);
    }
    memcpy(buf, &mac, max);

    return sizeof(mac);
}

size_t client_id_hex_str_convert(char *buf, size_t max, const char *hex_str) {
    if (!max) {
        return 0;
    }

    int convert_size = misc_hexstr_to_array((uchar *)buf, max, hex_str);
    if (convert_size <= 0) {
        return 0;
    }

    return convert_size;
}

#define DEFAULT_HOST_NAME "Switch"
size_t client_def_hostname_get(char * buf, int max) {
    system_conf_t conf;
    size_t        c, i, len;
    uchar         mac[6];

    if (!max) {
        return 0;
    }

    if (system_get_config(&conf) == VTSS_RC_OK) {
        if ((len = strlen(conf.sys_name)) == 0) {
            /* Create unique name */
            (void) conf_mgmt_mac_addr_get(mac, 0);
            sprintf(conf.sys_name, "estax-%02x-%02x-%02x",
                    mac[3], mac[4], mac[5]);
        } else {
            /* Convert system name to lower case and replace spaces by dashes */
            for (i = 0; i < len; i++) {
                c = tolower(conf.sys_name[i]);
                if (c == ' ') {
                    c = '-';
                }
                conf.sys_name[i] = c;
            }
        }
    }

    strncpy(buf, conf.sys_name, max);
    for (i = 0; i < max;) { // strnlen - including terminating zero
        if (buf[i++] == 0) {
            break;
        }
    }

    T_D("Host name is: %s %s " VPRIz, buf, conf.sys_name, i);
    return i;
}

size_t client_vendor_class_identifier(char * buf, int max) {
    uchar         mac[6];

    (void) conf_mgmt_mac_addr_get(mac, 0);
    return snprintf(buf, max,
                    VTSS_PRODUCT_NAME "-%02x-%02x-%02x-%02x-%02x-%02x", mac[0],
                    mac[1], mac[2], mac[3], mac[4], mac[5]);
}

#if defined(VTSS_SW_OPTION_DHCP_HELPER)
BOOL client_packet_handler(const u8 *const frm,
                           size_t          length,
                           const dhcp_helper_frame_info_t *helper_info,
                           const dhcp_helper_rx_cb_flag_t flags) {
    DHCP_CLIENT_CRIT_ENTER();
    vtss_ifindex_t ifidx;
    (void)vtss_ifindex_from_vlan(helper_info->vid, &ifidx);
    DhcpPool_t::DhcpClient_t *client = pool.get(ifidx);

    if (client == 0) {
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    typedef const vtss::FrameRef L0;
    L0 f(frm, length);

    T_N_HEX(frm, length);

    typedef const vtss::EthernetFrame<L0> L1;
    L1 e(f);

    if (e.etype() != 0x0800) {
        T_D("%u Ethernet: Not IP", helper_info->vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    typedef const vtss::IpFrame<L1> L2;
    L2 i(e);

    if (!i.check()) {
        T_D("%u IP: Did not pass checks", helper_info->vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    if (!i.is_simple()) {
        T_D("%u IP: Non simple IP", helper_info->vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    if (i.protocol() != 0x11) {
        T_D("%u IP: Not UDP", helper_info->vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    typedef const vtss::UdpFrame<L2> L3;
    L3 u(i);

    if (!u.check()) {
        T_D("%u UDP: Did not pass checks", helper_info->vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    if (u.dst() != 68) {
        T_D("%u UDP: wrong port", helper_info->vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    typedef const vtss::dhcp::DhcpFrame<L3> L4;
    L4 d(u);

    if (!d.check()) {
        T_D("%u DHCP-frame: Did not pass checks", helper_info->vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }
    client->frame_event(d, e.src());
    T_N("%u Packet has been delivered to dhcp client", helper_info->vid);

    DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
}

#else

BOOL client_packet_handler(void *contxt, const u8 *const frm,
                           const mesa_packet_rx_info_t *const rx_info)
{
    DHCP_CLIENT_CRIT_ENTER();
    vtss_ifindex_t ifidx;
    (void)vtss_ifindex_from_vlan(rx_info->tag.vid, &ifidx);
    DhcpPool_t::DhcpClient_t *client = pool.get(ifidx);

    if (client == 0) {
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    typedef const vtss::FrameRef L0;
    L0 f(frm, rx_info->length);

    T_N_HEX(frm, rx_info->length);

    typedef const vtss::EthernetFrame<L0> L1;
    L1 e(f);

    if (e.etype() != 0x0800) {
        T_D("%u Ethernet: Not IP", rx_info->tag.vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    typedef const vtss::IpFrame<L1> L2;
    L2 i(e);

    if (!i.check()) {
        T_D("%u IP: Did not pass checks", rx_info->tag.vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    if (!i.is_simple()) {
        T_D("%u IP: Non simple IP", rx_info->tag.vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    if (i.protocol() != 0x11) {
        T_D("%u IP: Not UDP", rx_info->tag.vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    typedef const vtss::UdpFrame<L2> L3;
    L3 u(i);

    if (!u.check()) {
        T_D("%u UDP: Did not pass checks", rx_info->tag.vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    if (u.dst() != 68) {
        T_D("%u UDP: wrong port", rx_info->tag.vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }

    typedef const vtss::dhcp::DhcpFrame<L3> L4;
    L4 d(u);

    if (!d.check()) {
        T_D("%u DHCP-frame: Did not pass checks", rx_info->tag.vid);
        DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
    }
    client->frame_event(d, e.src());
    T_N("%u Packet has been delivered to dhcp client", rx_info->tag.vid);

    DHCP_CLIENT_CRIT_RETURN(BOOL, FALSE);
}
#endif /* VTSS_SW_OPTION_DHCP_HELPER */


mesa_rc client_init(vtss_init_data_t *data) {
    T_D("enter, cmd: %d, isid: %u, flags: 0x%x",
        data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        critd_init(&DHCP_CLIENT_crit, "dhcp_client",
                   VTSS_MODULE_ID_DHCP_CLIENT,
                   CRITD_TYPE_MUTEX);
        break;

    case INIT_CMD_START:
        (void)vtss_ip_if_callback_add(client_if_up_down);
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
#if !defined(VTSS_SW_OPTION_DHCP_HELPER)
        DHCP_CLIENT_rx_filter_reg();
#endif /* !VTSS_SW_OPTION_DHCP_HELPER */
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

}  // namespace dhcp
}  // namespace vtss

mesa_rc vtss_dhcp_client_init(vtss_init_data_t *data) {
    return vtss::dhcp::client_init(data);
}

const char * dhcp_client_error_txt(mesa_rc rc){
    return 0;
}
