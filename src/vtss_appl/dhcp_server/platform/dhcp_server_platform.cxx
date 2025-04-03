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

/**
 *  \file
 *      dhcp_server_platform.c
 *
 *  \brief
 *      Platform-dependent APIs
 */

/*
==============================================================================

    Include File

==============================================================================
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "vtss_os_wrapper_network.h"


#include "main.h"
#include "main_types.h"
#include "dhcp_server_platform.h"
#include "vtss_dhcp_server.h"
#include "vtss_dhcp_server_message.h"
#include "ip_utils.hxx"
#include "ip_api.h"
#include "msg_api.h"
#include "dhcp_helper_api.h"
#include "conf_api.h"
#include "critd_api.h"
#include "packet_api.h"

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#include "vtss_trace_api.h"
#include "vtss_module_id.h"
#include "vtss_trace_lvl_api.h"

#define _DHCP_SERVER_PORT       67      /**< UDP port of DHCP server */
#define _DHCP_CLIENT_PORT       68      /**< UDP port of DHCP client */
#define _THREAD_MAX_CNT         2       /**< max number of threads */
#define _MAX_STR_BUF_SIZE       128     /**< max buffer size for trace and syslog */

static vtss_handle_t g_thread_handle[_THREAD_MAX_CNT];
static vtss_thread_t g_thread_block[_THREAD_MAX_CNT];
static critd_t       g_critd;

/* Vitesse trace */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "dhcp_server", "DHCP Server"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/*
==============================================================================

    Static Function

==============================================================================
*/
static BOOL _packet_rx(
    IN const u8 *const  packet,
    IN size_t           len,
    IN const dhcp_helper_frame_info_t *helper_info,
    IN const dhcp_helper_rx_cb_flag_t flag
)
{
    BOOL    b;
    vtss_ifindex_t ifindex;

    /* if not primary switch then de-register */
    if (!msg_switch_is_primary()) {
        dhcp_helper_user_receive_unregister(DHCP_HELPER_USER_SERVER);
        return FALSE;
    }

    /* invalid packet */
    if ( packet == NULL ) {
        return FALSE;
    }

    if (vtss_ifindex_from_port(helper_info->isid, helper_info->port_no, &ifindex) != VTSS_RC_OK) {
        return FALSE;
    }

    /* unused parameters */
    if ( len ) {}
    if ( flag ) {}

    __SEMA_TAKE();

    b = vtss_dhcp_server_packet_rx( packet, helper_info->vid, ifindex );

    __SEMA_GIVE();

    return b;
}

/*
==============================================================================

    Public APIs

==============================================================================
*/
/**
 * \brief
 *      initialize platform
 *
* \return
 *      TRUE  - successful
 *      FALSE - failed
 */
BOOL dhcp_server_platform_init(void)
{
    /* create semaphore */
    critd_init(&g_critd, "DHCP Server", VTSS_MODULE_ID_DHCP_SERVER, CRITD_TYPE_MUTEX);
    return TRUE;
}

/**
 * \brief
 *      Create thread.
 *
 * \param
 *      session_id [IN]: session ID
 *      name       [IN]: name of thread.
 *      priority   [IN]: thread priority.
 *      entry_cb   [IN]: thread running entry.
 *      entry_data [IN]: input parameter of thread running entry.
 *
 * \return
 *      TRUE : successful.
 *      FALSE: failed.
 */
BOOL dhcp_server_platform_thread_create(
    IN  i32                                     session_id,
    IN  const char                              *name,
    IN  u32                                     priority,
    IN  dhcp_server_platform_thread_entry_cb_t  *entry_cb,
    IN  i32                                     entry_data
)
{
    if (session_id >= _THREAD_MAX_CNT) {
        T_D("session_id(%d) is too large\n", session_id);
        return FALSE;
    }

    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       (vtss_thread_entry_f *)entry_cb,
                       (vtss_addrword_t)((u64)session_id),
                       name,
                       nullptr,
                       0,
                       &(g_thread_handle[session_id]),
                       &(g_thread_block[session_id]));

    return TRUE;
}

/**
 * \brief
 *      get the time elapsed from system start in seconds.
 *      process wrap around.
 *
 * \param
 *      n/a.
 *
 * \return
 *      seconds from system start.
 */
u32 dhcp_server_platform_current_time_get(
    void
)
{
    struct timespec     tp;

    if ( clock_gettime(CLOCK_MONOTONIC, &tp) == -1 ) {
        T_D("failed to get system up time\n");
        return 0;
    }
    return tp.tv_sec;
}

/**
 * \brief
 *      sleep for milli-seconds.
 *
 * \param
 *      t [IN]: milli-seconds for sleep.
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_sleep(
    IN u32  t
)
{
    VTSS_OS_MSLEEP( t );
}

/**
 * \brief
 *      take semaphore
 *
 * \param
 *      file [IN]: file name
 *      line [IN]: line number
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_sema_take(const char *const file, const int line)
{
    critd_enter(&g_critd, file, line);
}

/**
 * \brief
 *      give semaphore
 *
 * \param
 *      file [IN]: file name
 *      line [IN]: line number
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_sema_give(const char *const file, const int line)
{
    critd_exit(&g_critd, file, line);
}

/**
 *  \brief
 *      syslog message.
 *
 *  \param
 *      format [IN] : message format.
 *      ...    [IN] : message parameters
 *
 *  \return
 *      n/a.
 */
void dhcp_server_platform_syslog(
    IN  const char  *format,
    IN  ...
)
{
    char        str_buf[ _MAX_STR_BUF_SIZE + 1 ];
    va_list     arglist;
    int         r;

    memset(str_buf, 0, sizeof(str_buf));

    va_start( arglist, format );
    r = vsnprintf(str_buf, _MAX_STR_BUF_SIZE, format, arglist);
    va_end( arglist );

    if ( r ) {
#ifdef VTSS_SW_OPTION_SYSLOG
        //        S_I( str_buf );
#else
        puts( str_buf );
#endif
    }
}

/**
 *  \brief
 *      get IP interface of VLAN
 *
 *  \param
 *      vid     [IN] : VLAN ID
 *      ip      [OUT]: IP address of the VLAN
 *      netmask [OUT]: Netmask of the VLAN
 *
 *  \return
 *      TRUE  : successful
 *      FALSE : failed
 */
BOOL dhcp_server_platform_vid_info_get(
    IN  mesa_vid_t          vid,
    OUT mesa_ipv4_t         *ip,
    OUT mesa_ipv4_t         *netmask
)
{
    vtss_ifindex_t ifidx;
    vtss_appl_ip_if_status_t    ipv4_status;

    (void) vtss_ifindex_from_vlan(vid, &ifidx);
    if (vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &ipv4_status) != VTSS_RC_OK) {
        //T_D("Fail to get ip interface status on VLAN %u\n", vid);
        return FALSE;
    }

    if ( ip ) {
        *ip = ipv4_status.u.ipv4.net.address;
    }

    if (netmask) {
        *netmask = vtss_ipv4_prefix_to_mask(ipv4_status.u.ipv4.net.prefix_size);
    }

    return TRUE;
}

/**
 * \brief
 *      register packet rx callback.
 *
 * \param
 *      n/a.
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_packet_rx_register(
    void
)
{
    /* register only if primary switch */
    if (msg_switch_is_primary()) {
        dhcp_helper_user_receive_register(DHCP_HELPER_USER_SERVER, _packet_rx);
    }
}

/**
 * \brief
 *      deregister packet rx callback.
 *
 * \param
 *      n/a.
 *
 * \return
 *      n/a.
 */
void dhcp_server_platform_packet_rx_deregister(
    void
)
{
    dhcp_helper_user_receive_unregister(DHCP_HELPER_USER_SERVER);
}

/**
 * \brief
 *      send DHCP message.
 *
 * \param
 *      dhcp_message [IN]: DHCP message.
 *      option_len   [IN]: option field length.
 *      vid          [IN]: VLAN ID to send.
 *      sip          [IN]: source IP.
 *      dmac         [IN]: destination MAC.
 *      dip          [IN]: destination IP.
 *
 * \return
 *      TRUE  : successfully.
 *      FALSE : fail to send
 */
BOOL dhcp_server_platform_packet_tx(
    IN  dhcp_server_message_t   *dhcp_message,
    IN  u32                     option_len,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             sip_in,
    IN  u8                      *dmac,
    IN  mesa_ipv4_t             dip_in
)
{
    void                        *pbufref;
    u8                          *packet;
    size_t                      packet_size;
    dhcp_server_eth_header_t    *ether;
    dhcp_server_ip_header_t     *ip;
    dhcp_server_udp_header_t    *udp;
    char                        *payload;
    static u16                  ident = 11111;
    u8                          smac[DHCP_SERVER_MAC_LEN];
    u16                         udp_len;
    mesa_ip_addr_t              sip, dip;

    /* padding DHCP message */
    udp_len = (u16)sizeof(dhcp_server_udp_header_t) + (u16)sizeof(dhcp_server_message_t) + (u16)option_len;
    if (udp_len % 4) {
        udp_len += (4 - (udp_len % 4));
    } else {
        udp_len += 4;
    }

    // get packet buffer
    packet_size = sizeof(dhcp_server_eth_header_t) + sizeof(dhcp_server_ip_header_t) + udp_len;
    packet = (u8 *)dhcp_helper_alloc_xmit(packet_size, VTSS_ISID_GLOBAL, &pbufref);
    if (packet == NULL) {
        T_D("memory insufficient\n");
        return FALSE;
    }

    // clear packet buffer
    memset(packet, 0, packet_size);

    // get source MAC
    (void)conf_mgmt_mac_addr_get(smac, 0);

    // Ethernet header
    ether = (dhcp_server_eth_header_t *)packet;

    memcpy(ether->dmac, dmac, DHCP_SERVER_MAC_LEN);
    memcpy(ether->smac, smac, DHCP_SERVER_MAC_LEN);
    ether->etype[0] = 0x08;
    ether->etype[1] = 0x00;

    // IP header
    ip = (dhcp_server_ip_header_t *)(packet + sizeof(dhcp_server_eth_header_t));

    sip.type      = MESA_IP_TYPE_IPV4;
    sip.addr.ipv4 = sip_in;
    dip.type      = MESA_IP_TYPE_IPV4;
    dip.addr.ipv4 = dip_in;

    ip->vhl   = 0x45;
    ip->len   = htons((u16)(packet_size - sizeof(dhcp_server_eth_header_t)));
    ip->ident = htons(ident++);
    ip->ttl   = 64; // hops
    ip->proto = 17; // UDP
    ip->sip   = htonl(sip.addr.ipv4);
    ip->dip   = htonl(dip.addr.ipv4);

    // UDP header
    udp = (dhcp_server_udp_header_t *)(packet + sizeof(dhcp_server_eth_header_t) + sizeof(dhcp_server_ip_header_t));

    udp->sport = htons(_DHCP_SERVER_PORT);
    udp->dport = dhcp_message->giaddr ? htons(_DHCP_SERVER_PORT) : htons(_DHCP_CLIENT_PORT); //If message is to/from Relay agent
    udp->len   = htons(udp_len);

    // Payload
    payload = (char *)(packet + sizeof(dhcp_server_eth_header_t) + sizeof(dhcp_server_ip_header_t) + sizeof(dhcp_server_udp_header_t));

    memcpy(payload, dhcp_message, sizeof(dhcp_server_message_t) + option_len);

    // IP checksum
    ip->chksum = 0; // Make sure it's zero inside the packet before checksum calc.
    ip->chksum = htons(vtss_ip_checksum((const uint8_t *)ip, sizeof(dhcp_server_ip_header_t)));

    // UDP checksum
    udp->chksum = 0; // Make sure it's zero inside the packet before checksum calc.
    udp->chksum = htons(vtss_ip_pseudo_header_checksum((const uint8_t *)udp, udp_len, sip, dip, IP_PROTO_UDP));

    if (dhcp_helper_xmit(DHCP_HELPER_USER_SERVER, packet, packet_size, vid, VTSS_ISID_GLOBAL, 0, FALSE, VTSS_ISID_END, VTSS_PORT_NO_NONE, VTSS_GLAG_NO_NONE, pbufref) != 0) {
        T_D("dhcp_helper_xmit(%u)", vid);
        return FALSE;
    }

    return TRUE;
}

