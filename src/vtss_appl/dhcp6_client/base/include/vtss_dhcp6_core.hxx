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

#ifndef _VTSS_DHCP6_CORE_HXX_
#define _VTSS_DHCP6_CORE_HXX_

#include <forward_list>

#include "netdb.h"
#include "vtss_dhcp6_type.hxx"
#include "vtss_dhcp6_frame.hxx"

#define VTSS_TRACE_MODULE_ID                VTSS_MODULE_ID_DHCP6C

namespace vtss
{
namespace dhcp6
{
#define DHCPV6_DUID_TYPE_LLT                1
#define DHCPV6_DUID_TYPE_EN                 2
#define DHCPV6_DUID_TYPE_LL                 3
#define DHCPV6_DUID_HARDWARE_TYPE           1   /* Ethernet */

#define DHCPV6_DO_AUTHENTICATION            0

#define DHCPV6_SOL_MAX_DELAY                1
#define DHCPV6_SOL_TIMEOUT                  1
#define DHCPV6_SOL_MAX_RT                   120

#define DHCPV6_REQ_TIMEOUT                  1
#define DHCPV6_REQ_MAX_RT                   30
#define DHCPV6_REQ_MAX_RC                   10

#define DHCPV6_CNF_MAX_DELAY                1
#define DHCPV6_CNF_TIMEOUT                  1
#define DHCPV6_CNF_MAX_RT                   4
#define DHCPV6_CNF_MAX_RD                   10

#define DHCPV6_REN_TIMEOUT                  10
#define DHCPV6_REN_MAX_RT                   600

#define DHCPV6_REB_TIMEOUT                  10
#define DHCPV6_REB_MAX_RT                   600

#define DHCPV6_INF_MAX_DELAY                1
#define DHCPV6_INF_TIMEOUT                  1
#define DHCPV6_INF_MAX_RT                   120

#define DHCPV6_REL_TIMEOUT                  1
#define DHCPV6_REL_MAX_RC                   5

#define DHCPV6_DEC_TIMEOUT                  1
#define DHCPV6_DEC_MAX_RC                   5

#define DHCPV6_REC_TIMEOUT                  2
#define DHCPV6_REC_MAX_RC                   8

#define DHCPV6_HOP_COUNT_LIMIT              32

#define DHCP6_ADRS_CMP(x, y)                memcmp((x), (y), sizeof(mesa_ipv6_t))
#define DHCP6_ADRS_CPY(x, y)                memcpy((x), (y), sizeof(mesa_ipv6_t))
#define DHCP6_ADRS_SET(x, y)                memset((x), (y), sizeof(mesa_ipv6_t))
#define DHCP6_ADRS_CLEAR(x)                 DHCP6_ADRS_CPY(x, &dhcp6_unspecified_address)
#define DHCP6_ADRS_ISZERO(x)                !DHCP6_ADRS_CMP(x, &dhcp6_unspecified_address)
#define DHCP6_ADRS_EQUAL(x, y)              !DHCP6_ADRS_CMP(x, y)

#define DHCP6_DUID_LLT_LEN(x)               (sizeof((x)->duid_type) + sizeof((x)->type.llt))
#define DHCP6_DUID_LL_LEN(x)                (sizeof((x)->duid_type) + sizeof((x)->type.ll))
#define DHCP6_DUID_EN_LEN(x)                (sizeof((x)->duid_type) + sizeof((x)->type.en))

#define DHCP6_OPT_CODE_SET(x, y)            (x)->common.code = htons((y))
#define DHCP6_OPT_CODE_GET(x)               DHCP6_MSG_OPT_CODE(&((x)->common))
#define DHCP6_OPT_LEN_SET(x, y)             (x)->common.length = htons((y))
#define DHCP6_OPT_DATA_LEN_GET(x)           DHCP6_MSG_OPT_LENGTH(&((x)->common))
#define DHCP6_OPT_LEN_GET(x)                DHCP6_OPT_DATA_LEN_GET((x)) + sizeof(OptCommon)
#define DHCP6_OPT_LEN_INC(x, y)             (x) = (x) + DHCP6_OPT_LEN_GET((y))
#define DHCP6_OPT_DO_COPY(a, b, c, d, e, f) do {    \
    void    *_opt = (d);                            \
    size_t  _ofst = (e), _len = (f);                \
    memcpy((a), (b), (c));                          \
    if (_opt && _len) {                             \
        memcpy((a) + _ofst, (u8 *)_opt, _len);      \
    }                                               \
    (a) += DHCP6_OPT_LEN_GET((b));                  \
} while (0)


struct Rxmit {
    vtss_tick_count_t           rxmit_timeout;
    u16                         init_rxmit_time;
    u16                         max_rxmit_time;
    vtss_tick_count_t           max_rxmit_duration;
    u16                         max_rxmit_count;
    u16                         max_delay;
    u32                         rtprev;
};

BOOL duid_equal(BOOL nwo_a, const DeviceDuid *const a, BOOL nwo_b, const DeviceDuid *const b);
BOOL duid_assign(BOOL nwo_a, DeviceDuid *const a, BOOL nwo_b, const DeviceDuid *const b);
BOOL client_support_msg_type(const Dhcp6Message *const dhcp6_msg);
BOOL client_validate_opt_code(const Dhcp6Message *const dhcp6_msg, u32 len);


namespace client
{

#define DHCP6_WALK_FWD_LIST(x, y)           for (auto y = (x).begin(); (y) != (x).end(); ++(y))

const OptionCode SOL_OPT_REQ[] = {
    OPT_DNS_SERVERS,
    OPT_DOMAIN_LIST,
    /*
        OPT_IA_PD,
        OPT_IAPREFIX,
        OPT_VENDOR_OPTS,
    */

    /* add new support option(s) above this line */
    OPT_RESERVED
};
#define DHCP6_SOL_OPT_REQ_CNT               ((sizeof(vtss::dhcp6::client::SOL_OPT_REQ) / sizeof(vtss::dhcp6::OptionCode)) - 1)

#define DHCP6_SOL_OPT_REQ_ENCODE(x)         do {        \
    u8  _i = 0;                                         \
    while (_i < DHCP6_SOL_OPT_REQ_CNT) {                \
        if (_i % 2) {                                   \
            *(x) = *(x) | (SOL_OPT_REQ[_i] & 0xFFFF);   \
            *(x) = htonl(*(x));                         \
            if (++_i < DHCP6_SOL_OPT_REQ_CNT) {         \
                (x)++;                                  \
            }                                           \
        } else {                                        \
            *(x) = 0;                                   \
            *(x) = SOL_OPT_REQ[_i] << 16;               \
            if (++_i == DHCP6_SOL_OPT_REQ_CNT) {        \
                *(x) = htonl(*(x));                     \
            }                                           \
        }                                               \
    }                                                   \
} while (0)

struct AddrInfo {
    IaType                      type;
    mesa_ipv6_t                 address;
    u32                         prefix_length;

    vtss_tick_count_t           refresh_ts;
    vtss_tick_count_t           t1;
    vtss_tick_count_t           t2;
    vtss_tick_count_t           preferred_lifetime;
    vtss_tick_count_t           valid_lifetime;
};

struct ServerRecord {
    mesa_ipv6_t                 addrs;
    mesa_ipv6_t                 ucast;
    BOOL                        rapid_commit;
    BOOL                        has_unicast;
    u16                         reserved;
    DeviceDuid                  duid;
    u32                         xid_prf;
    std::forward_list<mesa_ipv6_t>   name_server;
    std::forward_list<std::string>   name_list;
    std::forward_list<AddrInfo>      adrs_pool;
};

#define DHCP6_SERVER_RECORD_CNT             3
#define DHCP6_SERVER_RECORD_XIDX_GET(x)     ((x)->xid_prf >> 8)
#define DHCP6_SERVER_RECORD_PREF_GET(x)     ((x)->xid_prf & 0xFF)
#define DHCP6_SERVER_RECORD_XIDX_SET(x, y)  (x)->xid_prf = ((x)->xid_prf & 0xFF) | (((y) << 8) & 0xFFFFFF00)
#define DHCP6_SERVER_RECORD_PREF_SET(x, y)  (x)->xid_prf = ((x)->xid_prf & 0xFFFFFF00) | ((y) & 0xFF)

#define DHCP6_INTF_MEM_CALLOC_CAST(x) (x = VTSS_CREATE(vtss::dhcp6::client::Interface))
#define DHCP6_INTF_MEM_FREE(x)              do {        \
    u8  _cnt = DHCP6_SERVER_RECORD_CNT + 1;             \
    for (; _cnt; --_cnt) {                              \
        if (!(x)->srvrs[_cnt - 1].name_server.empty())  \
            (x)->srvrs[_cnt - 1].name_server.clear();   \
        if (!(x)->srvrs[_cnt - 1].name_list.empty())    \
            (x)->srvrs[_cnt - 1].name_list.clear();     \
        if (!(x)->srvrs[_cnt - 1].adrs_pool.empty())    \
            (x)->srvrs[_cnt - 1].adrs_pool.clear();     \
    }                                                   \
    vtss_destroy(x);                                    \
} while (0)

struct Counter {
    u32                         rx_advertise;
    u32                         rx_reply;
    u32                         rx_reconfigure;
    u32                         rx_error;
    u32                         rx_drop;
    u32                         rx_unknown;

    u32                         tx_solicit;
    u32                         tx_request;
    u32                         tx_confirm;
    u32                         tx_renew;
    u32                         tx_rebind;
    u32                         tx_release;
    u32                         tx_decline;
    u32                         tx_information_request;
    u32                         tx_error;
    u32                         tx_drop;
    u32                         tx_unknown;
};

struct Interface {
    DeviceDuid                  duid;
    ServiceType                 type;

    mesa_vid_t                  ifidx;
    BOOL                        rapid_commit;
    BOOL                        stateless;
    BOOL                        link;
    BOOL                        dad;
    BOOL                        m_flag;
    BOOL                        o_flag;

    AddrInfo                    addrs;
    Counter                     cntrs;

    u32                         iaid;
    u32                         xidx;

    Rxmit                       rxmit;
    MessageType                 active_msg;
    vtss_tick_count_t           init_xmt;
    vtss_tick_count_t           last_xmt;
    u32                         xmt_cntr;

    ServerRecord                *server;
    ServerRecord                srvrs[DHCP6_SERVER_RECORD_CNT + 1];
};

void initialize(u8 max_intf_cnt);
mesa_rc tick(void);

mesa_rc receive(
    u8                  *const frm,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
);

mesa_rc transmit(
    mesa_vid_t          ifx,
    const mesa_ipv6_t   *const sip,
    const mesa_ipv6_t   *const dip,
    MessageType         msg_type,
    u32                 xid,
    u32                 opt_len,
    const u8            *const opts
);

mesa_rc do_rx_solicit(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_advertise(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_request(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_confirm(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_renew(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_rebind(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_reply(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_release(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_decline(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_reconfigure(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);
mesa_rc do_rx_information_request(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
);

mesa_rc do_tx_solicit(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_advertise(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_request(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_confirm(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_renew(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_rebind(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_reply(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_release(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_decline(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_reconfigure(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_information_request(Interface *const intf, vtss_tick_count_t ts);
mesa_rc do_tx_migrate_counter(Interface *const intf);

mesa_rc interface_itr(mesa_vid_t ifx, Interface *const intf);
mesa_rc interface_get(mesa_vid_t ifx, Interface *const intf);
mesa_rc interface_set(mesa_vid_t ifx, const Interface *const intf);
mesa_rc interface_del(mesa_vid_t ifx);
mesa_rc interface_rst(mesa_vid_t ifx);
mesa_rc interface_clr(mesa_vid_t ifx, BOOL cntr, BOOL addr);
mesa_rc interface_link(mesa_vid_t ifx, i8 link);
mesa_rc interface_dad(mesa_vid_t ifx);
mesa_rc interface_flag(mesa_vid_t ifx, BOOL managed, BOOL other);
mesa_rc interface_rx_cntr_inc(mesa_vid_t ifx, MessageType msg_type, BOOL err, BOOL drop);
mesa_rc interface_tx_cntr_inc(mesa_vid_t ifx, MessageType msg_type, BOOL err, BOOL drop);
} /* client */
} /* dhcp6 */
} /* vtss */

#undef VTSS_TRACE_MODULE_ID

#endif /* _VTSS_DHCP6_CORE_HXX_ */
