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

#ifndef _VTSS_DHCP6_RELAY_H_
#define _VTSS_DHCP6_RELAY_H_

#include "vtss/appl/dhcp6_relay.h"
#include "vtss/appl/interface.h"
#include "vtss/basics/expose/table-status.hxx"
#include <vtss/basics/vector.hxx>
#include "ip_api.h"
#include "ip_utils.hxx"
#include "misc_api.h"
#include <errno.h>
#include "vtss/appl/port.h"

using namespace vtss;

extern int errno ;

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_dhcp6_relay_json_init(void);
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void dhcp6_relay_mib_init(void);
#endif

//#define VTSS_DHCP6_RELAY_PACK_STRUCT __attribute__((packed))
#define HOP_COUNT_LIMIT  32                 // Max hop count in a Relay-forward message. RFC 3315.
#define RELAY_MESSAGE_OPTION_CODE   9       // RFC 3315.
#define RELAY_INTERFACE_ID_OPTION_CODE 18   // RFC 3315.
#define SERVER_UDP_PORT 547                 // RFC 3315.
#define CLIENT_UDP_PORT 546                 // RFC 3315.

const char *dhcp6_relay_error_txt(mesa_rc rc);

inline mesa_vid_t idx2vid(const vtss_ifindex_t &idx)
{
    vtss_ifindex_elm_t elm;
    VTSS_ASSERT(VTSS_RC_OK == vtss::ifindex_decompose(idx, &elm));
    VTSS_ASSERT(elm.iftype == VTSS_IFINDEX_TYPE_VLAN);
    return elm.ordinal;
}

enum {
    DHCP6_RELAY_ERROR_INTERFACE = MODULE_ERROR_START(VTSS_MODULE_ID_DHCP6_RELAY),
    DHCP6_RELAY_INVALID_RELAY_INTERFACE,
    DHCP6_RELAY_INVALID_RELAY_DESTINATION,
    DHCP6_RELAY_ERROR_LINK_LOCAL,
    DHCP6_RELAY_ERROR_UNSPEC,
    DHCP6_RELAY_ERROR_LOOPBACK,
    DHCP6_RELAY_ERROR_MC,
    DHCP6_RELAY_ERROR_NOT_FOUND,
};

struct vtss_dhcp6_relay_spec
{
    vtss_ifindex_t relay_interface;
    mesa_ipv6_t relay_destination;
};

struct vtss_dhcpv6_relay_agent
{
    vtss::Vector<vtss_dhcp6_relay_spec>  upper_interfaces;
    vtss_dhcp6_relay_spec                lower_interface;
    bool                                 joined;
};

struct vtss_dhcpv6_relay_agent_option_info 
{
    u16 option_code;
    u16 option_len;
    
  vtss_dhcpv6_relay_agent_option_info(uint32_t code, uint32_t len) :
      option_code(code), 
      option_len(len) { }

    virtual unsigned int get_htons(unsigned char *buf, uint32_t buf_size) {
        if (buf_size < get_size()) {
            return 0;
        }
        unsigned char *tmp = buf;
        u16 an_u16 = htons(option_code);
        memcpy(tmp, &an_u16, sizeof(option_code));
        tmp += sizeof(option_code);
        an_u16 = htons(option_len);
        memcpy(tmp, &an_u16, sizeof(option_len));
        tmp += sizeof(option_len);
        return tmp - buf;
    }

    size_t get_option_length() {
        return (size_t) option_len;
    }

    size_t get_size() {
        return (size_t) option_len + 4;
    }


};

/* RFC 3315
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      OPTION_INTERFACE_ID      |         option-len            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
.                                                               .
.                         interface-id                          .
.                                                               .
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
// Option code: 18
struct vtss_dhcpv6_relay_agent_interface_id_option : public vtss_dhcpv6_relay_agent_option_info 
{
    unsigned int if_idx;

    vtss_dhcpv6_relay_agent_interface_id_option(unsigned int idx) :
            vtss_dhcpv6_relay_agent_option_info(RELAY_INTERFACE_ID_OPTION_CODE,
                                                sizeof(if_idx)),
            if_idx(idx) { }

    unsigned int get_htons(unsigned char *buf, uint32_t buf_size) override {
        if (buf_size < get_size()) {
            return 0;
        }
        unsigned char *tmp = buf;
        tmp += vtss_dhcpv6_relay_agent_option_info::get_htons(tmp, buf_size);
        unsigned int idx = htonl(if_idx);
        memcpy(tmp, &idx, get_option_length());
        tmp += get_option_length();
        return tmp - buf;
    }

};

/* RFC 3315
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|        OPTION_RELAY_MSG       |           option-len          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
.                       DHCP-relay-message                      .
.                                                               .
.                                                               .
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
// Option code: 9
struct vtss_dhcpv6_relay_agent_msg_option : public vtss_dhcpv6_relay_agent_option_info 
{
    unsigned char *dhcp_relay_msg;

    vtss_dhcpv6_relay_agent_msg_option(unsigned char *message, int m_size) :
        vtss_dhcpv6_relay_agent_option_info(RELAY_MESSAGE_OPTION_CODE, (uint32_t) m_size),
        dhcp_relay_msg(message) { }

    unsigned int get_htons(unsigned char *buf, uint32_t buf_size) override {
        if (buf_size < get_size()) {
            return 0;
        }
        unsigned char *tmp = buf;
        tmp += vtss_dhcpv6_relay_agent_option_info::get_htons(tmp, buf_size);
        memcpy(tmp, dhcp_relay_msg, get_option_length());
        tmp += get_option_length();
        return tmp - buf;
    }

};

/* RFC 3315
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    msg-type   |   hop-count   |                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
|                                                               |
|                         link-address                          |
|                                                               |
|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
|                               |                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
|                                                               |
|                         peer-address                          |
|                                                               |
|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
|                               |                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
.                                                               .
.            options (variable number and length)   ....        .
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

struct vtss_dhcpv6_relay_agent_packet 
{
	unsigned char msg_type;
	unsigned char hop_count;
	unsigned char link_address[16];
	unsigned char peer_address[16];
    size_t size;
    vtss::Vector<vtss_dhcpv6_relay_agent_option_info*> options;

    void init(unsigned char message_type, unsigned char hop_cnt, in6_addr *from_addr, mesa_ipv6_t *link_addr, unsigned int if_idx) {
        msg_type = message_type;
        hop_count = hop_cnt;
        memcpy(link_address, link_addr, sizeof(link_address));
        memcpy(peer_address, from_addr, sizeof(peer_address));
        size = sizeof(msg_type) + sizeof(hop_count) + sizeof(link_address) + sizeof(peer_address);
        options.clear();
    }

    void add_option(vtss_dhcpv6_relay_agent_option_info *option) {
        options.push_back(option);
        size += option->get_size();
    }

    unsigned int get_packet_htons(unsigned char *buf, uint32_t buf_size) {
        if (buf_size < size) {
            return 0;
        }
        unsigned char *tmp = buf;
        vtss_dhcpv6_relay_agent_option_info *tmp_opt;
        memcpy(tmp, &msg_type, sizeof(msg_type));
        tmp += sizeof(msg_type);
        memcpy(tmp, &hop_count, sizeof(hop_count));
        tmp += sizeof(hop_count);
        memcpy(tmp, link_address, sizeof(link_address));
        tmp += sizeof(link_address);
        memcpy(tmp, peer_address, sizeof(peer_address));
        tmp += sizeof(peer_address);
        for (size_t i = 0; i < options.size(); i++) {
            tmp_opt = options[i];
            tmp += tmp_opt->get_htons(tmp, buf_size - (tmp - buf));
        }
        return tmp - buf;
    }
};

struct vtss_appl_dhcpv6_relay_vlan_statistics_t
{
    // Server statistics
    int tx_to_server;
    int rx_from_server;
    int server_pkt_dropped;
    
    // Client statistics
    int tx_to_client;
    int rx_from_client;
    int client_pkt_dropped;

    void init() {
        tx_to_server = 0;
        rx_from_server = 0;
        server_pkt_dropped = 0;
        tx_to_client = 0;
        rx_from_client = 0;
        client_pkt_dropped = 0;
    }

    // Increment functions
    void inc_tx_to_server() {
        tx_to_server++;
    }

    void inc_rx_from_server() {
        rx_from_server++;
    }

    void inc_server_pkt_dropped() {
        server_pkt_dropped++;
    }

    void inc_tx_to_client() {
        tx_to_client++;
    }

    void inc_rx_from_client() {
        rx_from_client++;
    }

    void inc_client_pkt_dropped() {
        client_pkt_dropped++;
    }

    // Get functions
    int get_tx_to_server() {
        return tx_to_server;
    }

    int get_rx_from_server() {
        return rx_from_server;
    }

    int get_server_pkt_dropped() {
        return server_pkt_dropped;
    }

    int get_tx_to_client() {
        return tx_to_client;
    }

    int get_rx_from_client() {
        return rx_from_client;
    }

    int get_client_pkt_dropped() {
        return client_pkt_dropped;
    }

};

struct vtss_appl_dhcpv6_relay_control_t {
    mesa_bool_t clear_all_stats;
};

struct vtss_appl_dhcpv6_relay_intf_missing_t {
    int num;
};

/* Iterates through statistics table. */
mesa_rc vtss_appl_dhcpv6_relay_vlan_statistics_itr(
    const vtss_ifindex_t *const in,
    vtss_ifindex_t *const out,
    const vtss_ifindex_t *const relay_in,
    vtss_ifindex_t *const relay_out);

/* Gets the statistics for a given relay ifindex duo. Result is stored in stat. */
mesa_rc vtss_appl_dhcpv6_relay_vlan_statistics_get(
    vtss_ifindex_t idx, 
    vtss_ifindex_t relay_idx,
    vtss_appl_dhcpv6_relay_vlan_statistics_t *stat);

/* Clears the statistics for a given relay ifindex duo. */
mesa_rc vtss_appl_dhcpv6_relay_vlan_statistics_set(
    vtss_ifindex_t idx, 
    vtss_ifindex_t relay_idx,
    const vtss_appl_dhcpv6_relay_vlan_statistics_t *const stat);

/* Control action get, always returns control->clear_all_stats as false. */
mesa_rc vtss_appl_dhcpv6_relay_control_get(vtss_appl_dhcpv6_relay_control_t *control);

/* If control->clear_all_stats is true then all statistics for all vlans are cleared. */
mesa_rc vtss_appl_dhcpv6_relay_control_set(
    const vtss_appl_dhcpv6_relay_control_t     *const control);

mesa_rc vtss_appl_dhcpv6_relay_interface_missing_get(vtss_appl_dhcpv6_relay_intf_missing_t *missing);

int vtss_appl_server_pkts_interface_missing_get();
void vtss_appl_server_pkts_interface_missing_clear();

typedef vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamVal<vtss_dhcpv6_relay_agent *>> RelayAgent;
extern RelayAgent relay_agent;

typedef vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamVal<vtss_appl_dhcpv6_relay_vlan_t *>> Dhcp6RelayVlanConfiguration;
extern Dhcp6RelayVlanConfiguration dhcp6_relay_vlan_configuration;

typedef vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamVal<vtss_appl_dhcpv6_relay_vlan_t *>> Dhcp6RelayVlanStatus;
extern Dhcp6RelayVlanStatus dhcp6_relay_vlan_status;

extern mesa_ipv6_t ipv6_zero;
extern mesa_ipv6_t ipv6_all_dhcp_servers;
extern mesa_ipv6_t ipv6_all_dhcp_relay_agents_and_servers;

#define IPV6_MAX_EXT_HEADER_DEPTH 10

#endif //_VTSS_DHCP6_RELAY_H_

