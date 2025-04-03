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

/*
 * NOTE: The actual filtering mechanism is implemented in the kernel module
 * vtss-if-mux. This file is "just" a wrapper which configure the kernel module.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/genetlink.h>

#include "main.h"
#include "ip_trace.h"
#include "microchip/ethernet/switch/api.h"

#include "vtss_netlink.hxx"
#include "ip_filter_api.hxx"
#include "ip_api.h"
#include "vtss/basics/trace.hxx"
#include "vtss/basics/memory.hxx"
#include "port_iter.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_IP

#define D VTSS_TRACE(VTSS_MODULE_ID_IP, IP_TRACE_GRP_FILTER, DEBUG)
#define I VTSS_TRACE(VTSS_MODULE_ID_IP, IP_TRACE_GRP_FILTER, INFO)
#define N VTSS_TRACE(VTSS_MODULE_ID_IP, IP_TRACE_GRP_FILTER, NOISE)
#define E VTSS_TRACE(VTSS_MODULE_ID_IP, IP_TRACE_GRP_FILTER, ERROR)

namespace vtss
{
namespace appl
{
namespace ip
{
namespace filter
{

// Kernel space definitions - must be kept in sync!
enum {
    VTSS_IF_MUX_ATTR_NONE,
    VTSS_IF_MUX_ATTR_ID,
    VTSS_IF_MUX_ATTR_OWNER,
    VTSS_IF_MUX_ATTR_LIST,
    VTSS_IF_MUX_ATTR_ACTION,
    VTSS_IF_MUX_ATTR_RULE,
    VTSS_IF_MUX_ATTR_ELEMENT,
    VTSS_IF_MUX_ATTR_ELEMENT_TYPE,
    VTSS_IF_MUX_ATTR_ELEMENT_PORT_MASK,
    VTSS_IF_MUX_ATTR_ELEMENT_ADDR,
    VTSS_IF_MUX_ATTR_ELEMENT_INT,
    VTSS_IF_MUX_ATTR_ELEMENT_PREFIX,
    VTSS_IF_MUX_ATTR_PORT_CONF,
    VTSS_IF_MUX_ATTR_PORT_CONF_ENTRY,
    VTSS_IF_MUX_ATTR_PORT_CONF_CHIP_PORT,
    VTSS_IF_MUX_ATTR_PORT_CONF_ETYPE,
    VTSS_IF_MUX_ATTR_PORT_CONF_ETYPE_CUSTOM,
    VTSS_IF_MUX_ATTR_PORT_CONF_VLAN_MASK,
    VTSS_IF_MUX_ATTR_PORT_CONF_RX_FILTER,
    VTSS_IF_MUX_ATTR_PORT_CONF_RX_FORWARD,
    VTSS_IF_MUX_ATTR_PORT_CONF_TX_FORWARD,
    VTSS_IF_MUX_ATTR_VLAN_VSI_MAP,
};

// Kernel space definitions - must be kept in sync!
enum {
    VTSS_IF_MUX_GENL_NOOP,
    VTSS_IF_MUX_GENL_RULE_CREATE,
    VTSS_IF_MUX_GENL_RULE_DELETE,
    VTSS_IF_MUX_GENL_RULE_MODIFY,
    VTSS_IF_MUX_GENL_RULE_GET,
    VTSS_IF_MUX_GENL_PORT_CONF_SET,
    VTSS_IF_MUX_GENL_VLAN_VSI_MAP_SET,

    // Add new entries here, and remember to update user-space applications
};

// Kernel space definitions - must be kept in sync!
enum {
    VTSS_IF_MUX_ACTION_DROP,
    VTSS_IF_MUX_ACTION_CHECK_ALLOW,
};

// Kernel space definitions - must be kept in sync!
enum {
    VTSS_IF_MUX_LIST_ALLOW,
    VTSS_IF_MUX_LIST_DENY,
};

static const char *module_name = "vtss_if_mux";

static int id = 0;
static int ip_filter_channel_id()
{
    if (id != 0 && id != -1) {
        return id;
    }

    id = netlink::genelink_channel_by_name(module_name, __FUNCTION__);
    if (id == -1) {
        E << "Failed to get channel for " << module_name;
    }

    return id;
}

struct genl_if_mux_req {
    genl_if_mux_req(int cmd)
    {
        n.nlmsg_seq = netlink::netlink_seq();
        n.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
        n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
        n.nlmsg_type = ip_filter_channel_id();
        g.cmd = cmd;
        g.version = 0;
    }

    // ORDERING ARE IMPORTANT!!! This structure is being casted to a "struct
    // nlmsghdr" plus payload...
    static constexpr uint32_t max_size = 10240;
    struct nlmsghdr n = {};
    struct genlmsghdr g = {};
    char attr[max_size];
};

#define DO(FUNC, ...)                                    \
    {                                                    \
        rc = FUNC(__VA_ARGS__);                          \
        if (rc != VTSS_RC_OK) {                          \
            I << "Failed: " #FUNC " error code: " << rc; \
            return rc;                                   \
        }                                                \
    }

#define ADD(T, TT, ...)                                                     \
    {                                                                       \
        DO(netlink::attr_add_##TT, &req.n, req.max_size, T, __VA_ARGS__)    \
    }                                                                       \

struct CaptureId : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        struct genlmsghdr *genl;
        int len = n->nlmsg_len;

        genl = (struct genlmsghdr *)NLMSG_DATA(n);
        len = n->nlmsg_len - NLMSG_LENGTH(sizeof(*genl));
        if (len < 0) {
            E << "Msg too short for this type!";
            return;
        }

        struct rtattr *rta = GENL_RTA(genl);
        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case VTSS_IF_MUX_ATTR_ID: {
                if (RTA_PAYLOAD(rta) != 4) {
                    E << "Unexpected size";
                    break;
                }

                id = *(int *)RTA_DATA(rta);
                ok = true;
                break;
            }
            }

            rta = RTA_NEXT(rta, len);
        }
    }

    bool ok = false;
    uint32_t id;
};

mesa_rc convert_port_map(PortMaskArray user, PortMaskArray &chip)
{
    int idx = 0, cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
    mesa_rc rc = VTSS_RC_OK;
    CapArray<mesa_port_map_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_map;
    PortMask usermask(user);
    PortMask chipmask;

    DO(mesa_port_map_get, NULL, cnt, port_map.data());

    for (idx = 0; idx < PORT_MASK_BITS; ++idx) {
        if (idx < cnt && usermask.test(idx)) {
            if (port_map[idx].chip_port != -1) {
                VTSS_ASSERT(port_map[idx].chip_port < PORT_MASK_BITS);
                chipmask.set(port_map[idx].chip_port);
            }
        }
    }

    chipmask.to_array(chip.data);
    return VTSS_RC_OK;
}

static mesa_rc rta_rule_add(genl_if_mux_req &req, const Rule &r)
{
    mesa_rc rc = VTSS_RC_OK;
    auto rta_rule = netlink::attr_nest(&req.n, req.max_size,
                                       VTSS_IF_MUX_ATTR_RULE | NLA_F_NESTED);
    if (!rta_rule) {
        return VTSS_RC_ERROR;
    }

    for (const auto &e : r) {
        auto rta_element = netlink::attr_nest(
                               &req.n, req.max_size, VTSS_IF_MUX_ATTR_ELEMENT | NLA_F_NESTED);
        if (!rta_element) {
            return VTSS_RC_ERROR;
        }

        ADD(VTSS_IF_MUX_ATTR_ELEMENT_TYPE, u32, (uint32_t)e.type);

        switch (e.type) {
        case Type::none:
        case Type::arp_gratuitous:
            break;

        case Type::port_mask: {
            PortMaskArray chip;
            DO(convert_port_map, e.data.mask, chip);
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_PORT_MASK, binary, chip.data, PORT_MASK_LEN);
            break;
        }

        case Type::mac_src:
        case Type::mac_dst:
        case Type::mac_src_or_dst:
        case Type::arp_hw_sender:
        case Type::arp_hw_target:
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_ADDR, mac, e.data.mac);
            break;

        case Type::vlan:
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_INT, u32, e.data.vlan);
            break;

        case Type::ether_type:
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_INT, u32, e.data.protocol);
            break;

        case Type::ipv4_src:
        case Type::ipv4_dst:
        case Type::ipv4_src_or_dst:
        case Type::arp_proto_sender:
        case Type::arp_proto_target:
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_ADDR, ipv4, e.data.ipv4.address);
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_PREFIX, u32, e.data.ipv4.prefix_size);
            break;

        case Type::ipv6_src:
        case Type::ipv6_dst:
        case Type::ipv6_src_or_dst:
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_ADDR, ipv6, e.data.ipv6.address);
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_PREFIX, u32, e.data.ipv6.prefix_size);
            break;

        case Type::arp_operation:
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_INT, u32, e.data.operation);
            break;

        case Type::acl_id:
            // Kind of a hack... but it is just a 16 byte bit field like an ipv6
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_ADDR, binary, e.data.acl_id.buf.data,
                16);
            ADD(VTSS_IF_MUX_ATTR_ELEMENT_PREFIX, u32, e.data.acl_id.offset);
            break;
        }

        netlink::attr_nest_end(&req.n, rta_element);
    }

    netlink::attr_nest_end(&req.n, rta_rule);

    return rc;
}

static mesa_rc rule_add(int *id, const Owner *o, const Rule &r, const Action *a,
                        int list)
{
    CaptureId capture;
    mesa_rc rc = VTSS_RC_OK;
    genl_if_mux_req req(VTSS_IF_MUX_GENL_RULE_CREATE);

    ADD(VTSS_IF_MUX_ATTR_OWNER, u64, (uint64_t)o);
    if (a) {
        ADD(VTSS_IF_MUX_ATTR_ACTION, u32, (uint32_t)(*a));
    }

    ADD(VTSS_IF_MUX_ATTR_LIST, u32, list);

    DO(rta_rule_add, req, r);
    DO(netlink::genl_req, (const void *)&req, req.n.nlmsg_len, req.n.nlmsg_seq,
       __FUNCTION__, &capture);

    if (!capture.ok) {
        E << "No ID returned";
        return VTSS_RC_ERROR;
    }

    if (id) {
        *id = capture.id;
    }

    // Check that the returned ID is never the same as what this module's
    // interface specifies as a non-used ID.
    VTSS_ASSERT(capture.id != VTSS_IP_FILTER_ID_NONE);

    return rc;
}

mesa_rc deny_list_rule_add(int *id, const Owner *o, const Rule &r, Action a)
{
    return rule_add(id, o, r, &a, VTSS_IF_MUX_LIST_DENY);
}

mesa_rc allow_list_rule_add(int *id, const Owner *o, const Rule &r)
{
    return rule_add(id, o, r, nullptr, VTSS_IF_MUX_LIST_ALLOW);
}

mesa_rc rule_update(int id, const Rule &r)
{
    mesa_rc rc = VTSS_RC_OK;
    genl_if_mux_req req(VTSS_IF_MUX_GENL_RULE_MODIFY);
    ADD(VTSS_IF_MUX_ATTR_ID, u32, id);
    DO(rta_rule_add, req, r);
    DO(netlink::genl_req, (const void *)&req, req.n.nlmsg_len, req.n.nlmsg_seq, __FUNCTION__);
    return rc;
}

mesa_rc rule_del(int id)
{
    mesa_rc rc = VTSS_RC_OK;
    genl_if_mux_req req(VTSS_IF_MUX_GENL_RULE_DELETE);
    ADD(VTSS_IF_MUX_ATTR_ID, u32, id);
    DO(netlink::genl_req, (const void *)&req, req.n.nlmsg_len, req.n.nlmsg_seq, __FUNCTION__);
    return rc;
}

mesa_rc rule_del(Owner *o, uint32_t *cnt)
{
    // TODO, capture cnt
    mesa_rc rc = VTSS_RC_OK;
    genl_if_mux_req req(VTSS_IF_MUX_GENL_RULE_DELETE);
    ADD(VTSS_IF_MUX_ATTR_OWNER, u64, (uint64_t)o);
    *cnt = 0;
    DO(netlink::genl_req, (const void *)&req, req.n.nlmsg_len, req.n.nlmsg_seq, __FUNCTION__);
    return rc;
}

// Prepared for up to 64 ports with 1024 bytes data
#define IF_MUX_PORT_CONF_MAX (64*1024)

struct genl_if_mux_port_conf_req {
    // The ordering below is important, because this structure casted to
    // "struct nlmsghdr" plus payload...
    struct nlmsghdr n;
    struct genlmsghdr g;
    char attr[IF_MUX_PORT_CONF_MAX];
};

#define DO_EXIT(FUNC, ...)                               \
    {                                                    \
        rc = FUNC(__VA_ARGS__);                          \
        if (rc != VTSS_RC_OK) {                          \
            I << "Failed: " #FUNC " error code: " << rc; \
            goto do_exit;                                \
        }                                                \
    }

mesa_rc port_conf_update(void)
{
    CapArray<mesa_port_map_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_map;
    CapArray<mesa_packet_vlan_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> vlan_filter;
    struct genl_if_mux_port_conf_req *req;
    mesa_vlan_conf_t                 global_conf;
    mesa_vlan_port_conf_t            conf;
    port_iter_t                      pit;
    u32                              tpid, chip_port, max_size = IF_MUX_PORT_CONF_MAX;
    u32                              port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
    uint8_t                          vlan_up[MESA_VIDS / 8];
    bool                             vlan_state[MESA_VIDS];
    int                              vid, i;
    struct rtattr                    *nla_table, *nla_entry;
    mesa_rc                          rc;

    req = (struct genl_if_mux_port_conf_req *)VTSS_CALLOC(sizeof(struct genl_if_mux_port_conf_req), 1);
    memset(vlan_up, 0, sizeof(vlan_up));

    if (!req) {
        return VTSS_RC_ERROR;
    }

    req->n.nlmsg_seq = netlink::netlink_seq();
    req->n.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
    req->n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req->n.nlmsg_type = ip_filter_channel_id();
    req->g.cmd = VTSS_IF_MUX_GENL_PORT_CONF_SET;
    req->g.version = 0;

    DO_EXIT(mesa_port_map_get, NULL, port_cnt, port_map.data());
    DO_EXIT(mesa_vlan_conf_get, NULL, &global_conf);
    DO_EXIT(mesa_packet_vlan_filter_get, NULL, port_cnt, &vlan_filter[0]);
    port_iter_init_local(&pit);
    nla_table = netlink::attr_nest(&req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF | NLA_F_NESTED);

    if (!nla_table) {
        rc = VTSS_RC_ERROR;
        goto do_exit;
    }

    while (port_iter_getnext(&pit) && mesa_vlan_port_conf_get(NULL, pit.iport, &conf) == VTSS_RC_OK) {
        chip_port = port_map[pit.iport].chip_port;
        tpid = (conf.port_type == MESA_VLAN_PORT_TYPE_C ? 0x8100 :
                conf.port_type == MESA_VLAN_PORT_TYPE_S ? 0x88a8 :
                conf.port_type == MESA_VLAN_PORT_TYPE_S_CUSTOM ? global_conf.s_etype : 0);

        T_N("iport: %u(%u), tpid: 0x%04x, vlan_discard[0]: 0x%02x", pit.iport, chip_port, tpid, vlan_filter[pit.iport].vlan_discard[0]);
        nla_entry = netlink::attr_nest(&req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF_ENTRY | NLA_F_NESTED);

        if (!nla_entry) {
            rc = VTSS_RC_ERROR;
            goto do_exit;
        }

        DO_EXIT(netlink::attr_add_u32, &req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF_CHIP_PORT, chip_port);
        DO_EXIT(netlink::attr_add_u32, &req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF_ETYPE, tpid);
        DO_EXIT(netlink::attr_add_u32, &req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF_ETYPE_CUSTOM, global_conf.s_etype);
        DO_EXIT(netlink::attr_add_u32, &req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF_RX_FILTER, conf.ingress_filter);
        DO_EXIT(netlink::attr_add_u32, &req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF_RX_FORWARD, vlan_filter[pit.iport].rx_forward);
        DO_EXIT(netlink::attr_add_u32, &req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF_TX_FORWARD, vlan_filter[pit.iport].tx_forward);
        DO_EXIT(netlink::attr_add_binary, &req->n, max_size, VTSS_IF_MUX_ATTR_PORT_CONF_VLAN_MASK, vlan_filter[pit.iport].vlan_discard, 512);
        netlink::attr_nest_end(&req->n, nla_entry);

        if (vlan_filter[pit.iport].tx_forward) {
            for (i = 0; i < MESA_VIDS / 8; i++) {
                vlan_up[i] |= ~vlan_filter[pit.iport].vlan_discard[i];
            }
        }
    }

    netlink::attr_nest_end(&req->n, nla_table);

    DO_EXIT(netlink::genl_req, (const void *)&req->n, sizeof(struct genl_if_mux_port_conf_req), req->n.nlmsg_seq, __FUNCTION__, NULL, max_size + 1024);

    /* Update VLAN status */
    for (vid = 1; vid < MESA_VIDS; vid++) {
        vlan_state[vid] = (vlan_up[vid / 8] & (1 << (vid % 8)) ? 1 : 0);
    }

    vtss_ip_vlan_state_update(vlan_state);

do_exit:
    VTSS_FREE(req);
    return rc;
}

}  // namespace filter
}  // namespace ip
}  // namespace appl
}  // namespace vtss

