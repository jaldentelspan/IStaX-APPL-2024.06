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

#define _DHCP_HELPER_USER_NAME_C_

#include "main.h"
#include "conf_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#if defined(VTSS_SW_OPTION_DHCP_RELAY) || defined(VTSS_SW_OPTION_IP)
#include "ip_api.h"
#endif
#include "packet_api.h"
#include "dhcp_helper_api.h"
#include "dhcp_helper.h"
#include "acl_api.h"
#include "port_api.h" // For port_change_register()
#include "vlan_api.h"
#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif /* VTSS_SW_OPTION_SYSLOG */
#include "mac_api.h"
#include "vtss_bip_buffer_api.h"
#include "vtss_free_list_api.h"
#include "vtss_avl_tree_api.h"
#include "ip_utils.hxx"
#include "aggr_api.h"

#include "dhcp_frame.hxx"
#include "vtss_bip_buffer_api.h"
#include "port_iter.hxx"

#if defined(VTSS_SW_OPTION_MSTP)
#include "mstp_api.h"
#endif

#ifdef __cplusplus
#include "enum_macros.hxx"
VTSS_ENUM_INC(dhcp_helper_user_t);
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_DHCP_HELPER

#define ETHER_TYPE_IPV4                 0x0800
#define IP_PROTOCOL_UDP                 0x11

/* DHCP port number */
#define DHCP_SERVER_UDP_PORT            67
#define DHCP_CLIENT_UDP_PORT            68

#define DHCP_OPT_SUBNET_MASK_ADDRESS    1
#define DHCP_OPT_ROUTE                  3
#define DHCP_OPT_DOMAIN_NAME_SERVER     6
#define DHCP_OPT_REQUESTED_ADDRESS      50
#define DHCP_OPT_LEASE_TIME             51
#define DHCP_OPT_SERVER_ID              54

#ifndef IP_VHL_HL
#define IP_VHL_HL(vhl)      ((vhl) & 0x0f)
#endif /* IP_VHL_HL */

/*
*  BZ#14251 - The DHCP client cannot work when IP management with two tags from NNI port
*
*  Reason:
*      The DHCP client sends through the DHCP helper module, which sends the frame
*      to each individual port and this method does not support adding two tags.
*
*  Solution:
*      1. Let the DHCP client send frames using H/W switching, so that double tagged
*      management works.
*      2. The ingress frame for DHCP client should use the MAC address (already setup
*      when the IP interface is created) instead of ACL rule and the packet register
*      flag 'PACKET_RX_FILTER_MATCH_ACL' should be removed.
*      3. If the DHCP Snooping or Relay mode is enabled, the ACL rule with isdx_disable=1
*      should be set. This flag is used to filter the service classified.
*/
#undef DHCP_HELPER_HW_SWITCHING

static void DHCP_HELPER_rx_filter_register(BOOL registerd);

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static dhcp_helper_global_t DHCP_HELPER_global;

static vtss_trace_reg_t DHCP_HELPER_trace_reg = {
    VTSS_TRACE_MODULE_ID, "dhcphelper", "DHCP helper, processing all DHCP received packets"
};

static vtss_trace_grp_t DHCP_HELPER_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&DHCP_HELPER_trace_reg, DHCP_HELPER_trace_grps);

#define DHCP_HELPER_CRIT_ENTER()             critd_enter(        &DHCP_HELPER_global.crit,     __FILE__, __LINE__)
#define DHCP_HELPER_CRIT_EXIT()              critd_exit(         &DHCP_HELPER_global.crit,     __FILE__, __LINE__)
#define DHCP_HELPER_CRIT_ASSERT_LOCKED()     critd_assert_locked(&DHCP_HELPER_global.crit,     __FILE__, __LINE__)
#define DHCP_HELPER_BIP_CRIT_ENTER()         critd_enter(        &DHCP_HELPER_global.bip_crit, __FILE__, __LINE__)
#define DHCP_HELPER_BIP_CRIT_EXIT()          critd_exit(         &DHCP_HELPER_global.bip_crit, __FILE__, __LINE__)
#define DHCP_HELPER_BIP_CRIT_ASSERT_LOCKED() critd_assert_locked(&DHCP_HELPER_global.bip_crit, __FILE__, __LINE__)

/* Thread variables */
static vtss_handle_t DHCP_HELPER_thread_handle;
static vtss_thread_t DHCP_HELPER_thread_block;

/* BIP buffer Thread variables */
static vtss_handle_t DHCP_HELPER_bip_buffer_thread_handle;
static vtss_thread_t DHCP_HELPER_bip_buffer_thread_block;

/* BIP buffer event declaration */
#define DHCP_HELPER_EVENT_PKT_RX        0x00000001
#define DHCP_HELPER_EVENT_PKT_TX        0x00000002
#define DHCP_HELPER_EVENT_ANY           (DHCP_HELPER_EVENT_PKT_RX | DHCP_HELPER_EVENT_PKT_TX) /* Any possible bit */
static vtss_flag_t   DHCP_HELPER_bip_buffer_thread_events;

/* BIP buffer data declaration */
#define DHCP_HELPER_BIP_BUF_PKT_SIZE    1520  /* 4-byte aligned */
#define DHCP_HELPER_BIP_BUF_CNT         DHCP_HELPER_FRAME_INFO_MAX_CNT

// Local variable of S-Custom VLAN tag TPID
static mesa_etype_t DHCP_HELPER_vlan_tpid = 0x88A8;

#if 1 /* Bugzilla#15786 - memory is used out */
#define DHCP_HELPER_TX_CNT_MAX      2048

static u32  DHCP_HELPER_msg_tx_pkt_cnt = 0;
#endif

typedef struct {
    unsigned char               pkt[DHCP_HELPER_BIP_BUF_PKT_SIZE];  // Used in tx and rx
    size_t                      len;                                // Used in tx and rx
    BOOL                        is_rx;                              // Used in tx and rx
    mesa_vid_t                  vid;                                // Used in tx and rx
    dhcp_helper_tagged_info_t   tagged_info;                        // Used in rx
    vtss_isid_t                 src_isid;                           // Used in tx and rx
    mesa_port_no_t              src_port_no;                        // Used in tx and rx
    mesa_glag_no_t              src_glag_no;                        // Used in tx and rx
    vtss_isid_t                 dst_isid;                           // Used in tx
    u64                         dst_port_mask;                      // Used in tx
    BOOL                        no_filter_dest;                     // Used in tx - if TRUE then send directly to port without filtering
    dhcp_helper_user_t          user;                               // Used in tx
    BOOL                        acl_hit;                            // Used in rx to identify if the incoming packet is hit acl or not
} dhcp_helper_bip_buf_t;

// Make each entry take a multiple of 4 bytes.
#define DHCP_HELPER_BIP_BUF_ALIGNED_SIZE (4 * ((sizeof(dhcp_helper_bip_buf_t) + 3) / 4))

/* BIP buffer parameters variables */
#define DHCP_HELPER_BIP_BUF_TOTAL_SIZE          (DHCP_HELPER_BIP_BUF_CNT * DHCP_HELPER_BIP_BUF_ALIGNED_SIZE)
static vtss_bip_buffer_t DHCP_HELPER_bip_buf;

/* Record incoming frame information */
#define DHCP_HELPER_FRAME_INFO_MONITOR_INTERVAL     5000    // Unit is msec., 5 seconds
#define DHCP_HELPER_FRAME_INFO_OBSOLETED_TIMEOUT    18000   // Unit is sytem time ticket, 180 seconds (3 minutes)

/* Frame information list */
#define DHCP_HELPER_FRAME_INFO_MAX_ENTRY_CNT        (DHCP_HELPER_FRAME_INFO_MAX_CNT * 2)
static i32 DHCP_Helper_frame_info_entry_compare_func(void *elm1, void *elm2);
VTSS_FREE_LIST(DHCP_HELPER_frame_info_free_list, dhcp_helper_frame_info_t, DHCP_HELPER_FRAME_INFO_MAX_ENTRY_CNT)
VTSS_AVL_TREE(DHCP_HELPER_frame_info_avlt, "DHCP_HELPER_frame_info_avlt", VTSS_MODULE_ID_DHCP_HELPER, DHCP_Helper_frame_info_entry_compare_func, DHCP_HELPER_FRAME_INFO_MAX_ENTRY_CNT)

/* packet rx filter */
static packet_rx_filter_t  DHCP_HELPER_rx_filter;
static void                 *DHCP_HELPER_filter_id = NULL; //Filter id for subscribing dhcp packet.

/* Callback function for the dynamic IP address obtained */
static dhcp_helper_frame_info_callback_t DHCP_HELPER_ip_addr_obtained_cb = NULL, DHCP_HELPER_release_ip_addr_cb = NULL;

/* Callback function when clear all DHCP Helper deatiled statistics
   We need to clear user's local statistics too */
static dhcp_helper_user_clear_local_stat_callback_t DHCP_HELPER_clear_local_stat_cb[DHCP_HELPER_USER_CNT];

/* Callback function when receive DHCP frame */
static dhcp_helper_stack_rx_callback_t DHCP_HELPER_rx_cb[DHCP_HELPER_USER_CNT];

/******************************************************************************/
// dhcp_helper_tagged_info_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const dhcp_helper_tagged_info_t &info)
{
    int i;
    char buf[3];

    o << "{type = " << info.type
      << ", data = ";

    for (i = 0; i < sizeof(info.data); i++) {
        if (i) {
            o << "-";
        }

        sprintf(buf, "%02x", info.data[i]);
        o << buf;
    }

    o << "}";

    return o;
}

/******************************************************************************/
// dhcp_helper_frame_info_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const dhcp_helper_frame_info_t &info)
{
    mesa_mac_t          mac;
    mesa_ipv4_network_t ipv4;

    memcpy(mac.addr, info.mac, sizeof(mac.addr));

    vtss_clear(ipv4);
    ipv4.address = info.assigned_ip;

    o << "{mac = "               << mac
      << ", vid = "              << info.vid
      << ", transaction_id = 0x" << vtss::hex(info.transaction_id)
      << ", tagged_info = "      << info.tagged_info
      << ", isid = "             << info.isid
      << ", port_no = "          << info.port_no
      << ", glag_no = "          << (int)info.glag_no
      << ", op_code = "          << info.op_code
      << ", assigned_ip = "      << ipv4;

    ipv4.address = info.assigned_mask;
    o << ", assigned_mask = " << ipv4;

    ipv4.address = info.dhcp_server_ip;
    o << ", dhcp_server_ip = " << ipv4;

    ipv4.address = info.gateway_ip;
    o << ", gateway_ip = " << ipv4;

    ipv4.address = info.dns_server_ip;
    o << ", dns_server_ip = " << ipv4;

    o << ", lease_time = " << info.lease_time
      << ", timestamp = "  << info.timestamp
      << ", local_dhcp_server = " << info.local_dhcp_server
      << "}";

    return o;
}

/******************************************************************************/
// dhcp_helper_frame_info_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const dhcp_helper_frame_info_t *info)
{
    o << *info;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// dhcp_helper_bip_buf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const dhcp_helper_bip_buf_t &b)
{
    o << "{pkt = "               << b.pkt
      << ", len = "              << b.len
      << ", is_rx = "            << b.is_rx
      << ", vid = "              << b.vid
      << ", tagged_info = "      << b.tagged_info
      << ", src_isid = "         << b.src_isid
      << ", src_port_no = "      << b.src_port_no
      << ", src_glag_no = "      << (int)b.src_glag_no
      << ", dst_isid = "         << b.dst_isid
      << ", dst_port_mask = 0x"  << vtss::hex(b.dst_port_mask)
      << ", no_filter_dest = "   << b.no_filter_dest
      << ", user = "             << b.user
      << ", acl_hit = "          << b.acl_hit
      << "}";

    return o;
}

/******************************************************************************/
// dhcp_helper_bip_buf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const dhcp_helper_bip_buf_t *b)
{
    o << *b;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/****************************************************************************/
/*  Frame information maintain functions                                    */
/****************************************************************************/

/* Register Register dynmaic IP address obtained
   Callback after the IP assigned action is completed */
void dhcp_helper_notify_ip_addr_obtained_register(dhcp_helper_frame_info_callback_t cb)
{
    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_ip_addr_obtained_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }

    DHCP_HELPER_ip_addr_obtained_cb = cb;
    DHCP_HELPER_CRIT_EXIT();
}

/* Register dynmaic IP address released
   Callback when receive a DHCP release frame */
void dhcp_helper_release_ip_addr_register(dhcp_helper_frame_info_callback_t cb)
{
    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_release_ip_addr_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }

    DHCP_HELPER_release_ip_addr_cb = cb;
    DHCP_HELPER_CRIT_EXIT();
}

/* Get DHCP frame information */
template <class T>
static void DHCP_HELPER_frame_info_parse(const vtss::dhcp::DhcpFrame<T> &d, dhcp_helper_frame_info_t *info)
{
    vtss::dhcp::Option::SubnetMask mask;
    if (d.option_get(mask)) {
        info->assigned_mask = mask.as_int();
        T_D("[assigned_mask]");
    }

    vtss::dhcp::Option::RouterOption router;
    if (d.option_get(router)) {
        info->gateway_ip = router.as_int();
        T_D("[gateway_ip]");
    }

    vtss::dhcp::Option::DnsServer dns;
    if (d.option_get(dns)) {
        info->dns_server_ip = dns.as_int();
        T_D("[dns_server]");
    }

    vtss::dhcp::Option::RequestedIPAddress req_ip;
    if (d.option_get(req_ip)) {
        info->assigned_ip = req_ip.as_int();
        T_D("[requested_ip]");
    }

    vtss::dhcp::Option::IpAddressLeaseTime lease;
    if (d.option_get(lease)) {
        info->lease_time = lease.val();
        T_D("[lease_time]");
    }

    vtss::dhcp::Option::ServerIdentifier sid;
    if (d.option_get(sid)) {
        info->dhcp_server_ip = sid.as_int();
        T_D("[server_ip]");
    }
}

/* Alloc memory for frame information entry */
static dhcp_helper_frame_info_t *DHCP_HELPER_frame_info_free_list_malloc(void)
{
    return (dhcp_helper_frame_info_t *)vtss_free_list_malloc(&DHCP_HELPER_frame_info_free_list);
}

/* Free memory of frame information entry */
static void DHCP_HELPER_frame_info_free_list_free(dhcp_helper_frame_info_t *entry)
{
    (void) vtss_free_list_free(&DHCP_HELPER_frame_info_free_list, entry);
}

/* Initialize frame information list */
static void DHCP_HELPER_frame_info_free_list_init(void)
{
    (void) vtss_free_list_init(&DHCP_HELPER_frame_info_free_list);
}

/* Initialize DHCP Helper frame information list */
static void DHCP_HELPER_frame_info_init(void)
{
    if (vtss_avl_tree_init(&DHCP_HELPER_frame_info_avlt) == FALSE) {
        T_E("Fail to create AVL tree for DHCP Helper frame info. list\n");
        return;
    }
    DHCP_HELPER_frame_info_free_list_init();
}

/* Compare function for frame information entry */
static i32 DHCP_Helper_frame_info_entry_compare_func(void *elm1, void *elm2)
{
    dhcp_helper_frame_info_t *element1 = (dhcp_helper_frame_info_t *)elm1;
    dhcp_helper_frame_info_t *element2 = (dhcp_helper_frame_info_t *)elm2;

    if (memcmp(element1->mac, element2->mac, 6) > 0) {
        return 1;
    } else if (memcmp(element1->mac, element2->mac, 6) < 0) {
        return -1;
    } else if (element1->vid > element2->vid) {
        return 1;
    } else if (element1->vid < element2->vid) {
        return -1;
    } else if (element1->transaction_id > element2->transaction_id) {
        return 1;
    } else if (element1->transaction_id < element2->transaction_id) {
        return -1;
    } else {
        return 0;
    }

}

/* Clear DHCP helper frame information entry */
static void DHCP_HELPER_frame_info_clear_all(void)
{
    DHCP_HELPER_CRIT_ENTER();
    if (vtss_avl_tree_init(&DHCP_HELPER_frame_info_avlt) == TRUE) {
        DHCP_HELPER_frame_info_free_list_init();
    }
    DHCP_HELPER_CRIT_EXIT();
}

/* Clear DHCP helper frame information obsoleted entry */
static void DHCP_HELPER_frame_info_clear_obsoleted(void)
{
    dhcp_helper_frame_info_t    temp_entry, *entry_p = &temp_entry;
    u32                         timestamp = vtss_current_time();
    /* Clear entries which are reached the expired time */
    DHCP_HELPER_CRIT_ENTER();
    memset(&temp_entry, 0, sizeof(dhcp_helper_frame_info_t));
    while (vtss_avl_tree_get(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) == TRUE) {
        if (timestamp - entry_p->timestamp < DHCP_HELPER_FRAME_INFO_OBSOLETED_TIMEOUT) {
            continue;
        }
        if (vtss_avl_tree_delete(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p) == TRUE) {
            DHCP_HELPER_frame_info_free_list_free(entry_p);
        }
    }
    DHCP_HELPER_CRIT_EXIT();
}

/* Add DHCP helper frame information entry.
   Return TRUE when the IP assigned action is completed.
   Otherwise, retrun FALSE.
 */
static BOOL DHCP_HELPER_frame_info_add(dhcp_helper_frame_info_t *info)
{
    dhcp_helper_frame_info_t *entry_p = info;
    BOOL ip_assigned_completed = FALSE;

    if (info->op_code != DHCP_HELPER_MSG_TYPE_DISCOVER &&
        info->op_code != DHCP_HELPER_MSG_TYPE_OFFER &&
        info->op_code != DHCP_HELPER_MSG_TYPE_REQUEST &&
        info->op_code != DHCP_HELPER_MSG_TYPE_ACK) {
        return FALSE;
    }

    T_D("info = %s", *info);

    DHCP_HELPER_CRIT_ENTER();
    /* Search entry exist? */
    if (vtss_avl_tree_get(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET) == TRUE) {
        /* Found it, update content */
        if ((entry_p->op_code == DHCP_HELPER_MSG_TYPE_DISCOVER && info->op_code == DHCP_HELPER_MSG_TYPE_OFFER) ||
            (entry_p->op_code == DHCP_HELPER_MSG_TYPE_OFFER && info->op_code == DHCP_HELPER_MSG_TYPE_REQUEST) ||
            (entry_p->op_code == DHCP_HELPER_MSG_TYPE_REQUEST && info->op_code == DHCP_HELPER_MSG_TYPE_ACK)) {
            entry_p->op_code = info->op_code;
            if (info->op_code == DHCP_HELPER_MSG_TYPE_REQUEST ||
                info->op_code == DHCP_HELPER_MSG_TYPE_ACK) {
                if (info->assigned_ip) {
                    entry_p->assigned_ip = info->assigned_ip;
                }
                if (info->assigned_mask) {
                    entry_p->assigned_mask = info->assigned_mask;
                }
                if (info->dhcp_server_ip) {
                    entry_p->dhcp_server_ip = info->dhcp_server_ip;
                }
                if (info->gateway_ip) {
                    entry_p->gateway_ip = info->gateway_ip;
                }
                if (info->dns_server_ip) {
                    entry_p->dns_server_ip = info->dns_server_ip;
                }
                if (info->lease_time) {
                    entry_p->lease_time = info->lease_time;
                }
                entry_p->timestamp = vtss_current_time();
                entry_p->local_dhcp_server = info->local_dhcp_server;
            }
        }

        if (info->op_code == DHCP_HELPER_MSG_TYPE_ACK &&
            entry_p->assigned_ip &&
            entry_p->assigned_mask &&
            entry_p->lease_time) {
            if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING] && DHCP_HELPER_ip_addr_obtained_cb) {
                /* Notice snooping module the IP assigned action is completed. */
                T_D("[notice snooping]");
                DHCP_HELPER_ip_addr_obtained_cb(entry_p);
            }
            ip_assigned_completed = TRUE;
        }
    } else if ((info->op_code == DHCP_HELPER_MSG_TYPE_DISCOVER || info->op_code == DHCP_HELPER_MSG_TYPE_REQUEST)) {
        /* Not found, create a new one */
        if ((entry_p = DHCP_HELPER_frame_info_free_list_malloc()) != NULL) {    // Allocate new entry buffer
            memcpy(entry_p, info, sizeof(dhcp_helper_frame_info_t));
            entry_p->timestamp = vtss_current_time();
            T_D("Adding %s", *entry_p);
            if (vtss_avl_tree_add(&DHCP_HELPER_frame_info_avlt, entry_p) == FALSE) {
                T_D("Calling vtss_avl_tree_add() failed");
            }
        } else {
            T_D("Reach the DHCP Helper frame information the maximum count");
        }
    }

    DHCP_HELPER_CRIT_EXIT();

    return ip_assigned_completed;
}

/* Delete DHCP helper frame information entry */
static void DHCP_HELPER_frame_info_del(dhcp_helper_frame_info_t *info)
{
    dhcp_helper_frame_info_t *entry_p = info;

    DHCP_HELPER_CRIT_ENTER();
    if (vtss_avl_tree_get(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET) == TRUE) {
        if (vtss_avl_tree_delete(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p) == TRUE) {
            DHCP_HELPER_frame_info_free_list_free(entry_p);
        }
    }
    DHCP_HELPER_CRIT_EXIT();
}

/* Delete DHCP helper frame information entry by MAC port */
static void DHCP_HELPER_frame_info_del_by_port(vtss_isid_t isid, mesa_port_no_t port_no)
{
    dhcp_helper_frame_info_t temp_entry, *entry_p = &temp_entry;

    DHCP_HELPER_CRIT_ENTER();
    memset(&temp_entry, 0, sizeof(dhcp_helper_frame_info_t));
    while (vtss_avl_tree_get(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) == TRUE) {
        if (entry_p->isid != isid || entry_p->port_no != port_no) {
            continue;
        }
        if (vtss_avl_tree_delete(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p) == TRUE) {
            DHCP_HELPER_frame_info_free_list_free(entry_p);
        }
    }
    DHCP_HELPER_CRIT_EXIT();
}

/* Clear DHCP helper frame information other uncompleted entry.
   The API is called after the IP assigned action is completed. */
static void DHCP_HELPER_frame_info_clear(dhcp_helper_frame_info_t *info, BOOL include_myself)
{
    dhcp_helper_frame_info_t temp_entry, *entry_p = &temp_entry;

    DHCP_HELPER_CRIT_ENTER();
    memset(&temp_entry, 0, sizeof(dhcp_helper_frame_info_t));
    while (vtss_avl_tree_get(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) == TRUE) {
        if (!memcmp(entry_p->mac, info->mac, 6) &&
            entry_p->vid == info->vid &&
            (!include_myself || (include_myself && entry_p->transaction_id != info->transaction_id))) {
            if (vtss_avl_tree_delete(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p) == TRUE) {
                DHCP_HELPER_frame_info_free_list_free(entry_p);
            }
        }
    }
    DHCP_HELPER_CRIT_EXIT();
}

/* Lookup DHCP helper frame information entry */
BOOL dhcp_helper_frame_info_lookup(u8 *mac, mesa_vid_t vid, uint transaction_id, dhcp_helper_frame_info_t *info)
{
    dhcp_helper_frame_info_t temp_entry, *entry_p = &temp_entry;
    BOOL found = FALSE;

    DHCP_HELPER_CRIT_ENTER();
    if (vid) {
        memcpy(entry_p->mac, mac, 6);
        entry_p->vid = vid;
        entry_p->transaction_id = transaction_id;
        if ((found = vtss_avl_tree_get(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET)) == TRUE) {
            *info = *entry_p;
        }
    } else {    // It is a specific case when vid = 0. Lookup entry which matched MAC & Transaction ID only
        memset(&temp_entry, 0, sizeof(dhcp_helper_frame_info_t));
        while (vtss_avl_tree_get(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT) == TRUE) {
            if (!memcmp(entry_p->mac, mac, 6) && entry_p->transaction_id == transaction_id) {
                *info = *entry_p;
                found = TRUE;
                break;
            }
        }
    }
    DHCP_HELPER_CRIT_EXIT();

    T_D("Found = %d, info = %s", found, *info);
    return found;
}

/* Getnext DHCP helper frame information entry */
BOOL dhcp_helper_frame_info_getnext(u8 *mac, mesa_vid_t vid, uint transaction_id, dhcp_helper_frame_info_t *info)
{
    dhcp_helper_frame_info_t temp_entry, *entry_p = &temp_entry;
    BOOL found = FALSE;

    DHCP_HELPER_CRIT_ENTER();
    memcpy(entry_p->mac, mac, 6);
    entry_p->vid = vid;
    entry_p->transaction_id = transaction_id;
    if ((found = vtss_avl_tree_get(&DHCP_HELPER_frame_info_avlt, (void **) &entry_p, VTSS_AVL_TREE_GET_NEXT)) == TRUE) {
        *info = *entry_p;
    }
    DHCP_HELPER_CRIT_EXIT();
    return found;
}


/****************************************************************************/
/*  Callback functions                                                      */
/****************************************************************************/

static void DHCP_HELPER_port_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    /* Clear releated frame information entries when port status is link-down */
    if (!status->link) {
        DHCP_HELPER_frame_info_del_by_port(VTSS_ISID_START, port_no);
    }
}

/*  Get output destination portmask.
*/
static u64 DHCP_HELPER_dst_port_mask_get(mesa_vid_t vid, vtss_isid_t dst_isid, vtss_isid_t src_isid, mesa_port_no_t src_port_no, mesa_glag_no_t src_glag_no, u8 dhcp_message)
{
    port_iter_t             pit;
    u64                     dst_port_mask = 0;

    T_D("enter, vid=%d, dst_isid=%d, src_isid=%d, src_port_no=%d src_glag_no=%d", vid, dst_isid, src_isid, src_port_no, src_glag_no);

    if (!msg_switch_is_primary() || !msg_switch_exists(dst_isid)) {
        return dst_port_mask;
    }

    (void) port_iter_init(&pit, NULL, dst_isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
    while (port_iter_getnext(&pit)) {
        /* Filter source port */
        if (src_isid == dst_isid && pit.iport == src_port_no) {
            continue;
        }

        /* Filter untrusted ports */
        DHCP_HELPER_CRIT_ENTER();
        if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING] && !DHCP_HELPER_MSG_FROM_SERVER(dhcp_message)) {
            if (DHCP_HELPER_global.port_conf[dst_isid].port_mode[pit.iport] == DHCP_HELPER_PORT_MODE_UNTRUSTED) {
                T_D("Filter untrusted ports: %d/%d", dst_isid, pit.uport);
                DHCP_HELPER_CRIT_EXIT();
                continue;
            }
        }
        DHCP_HELPER_CRIT_EXIT();

        /* Performance improvement, use mesa_packet_port_filter_get() to filter unused ports */
        dst_port_mask |= VTSS_BIT64(pit.iport);
    }

    T_D("exit");
    return dst_port_mask;
}

void DHCP_HELPER_stats_add(dhcp_helper_user_t user, vtss_isid_t isid, u64 dst_port_mask, u8 dhcp_message, dhcp_helper_direction_t dhcp_message_direction)
{
    mesa_port_no_t  port_idx;

    if (user >= DHCP_HELPER_USER_CNT) {
        return;
    }

    if (!msg_switch_is_primary()) {
        isid = VTSS_ISID_LOCAL;
    }

    DHCP_HELPER_CRIT_ENTER();
    /* Filter destination ports */
    for (port_idx = VTSS_PORT_NO_START; port_idx < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_idx++) {
        if (VTSS_EXTRACT_BITFIELD64(dst_port_mask, port_idx,  1) == 0) {
            continue;
        }

        if (dhcp_message_direction == DHCP_HELPER_DIRECTION_RX) {
            switch (dhcp_message) {
            case DHCP_HELPER_MSG_TYPE_DISCOVER:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.discover_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_OFFER:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.offer_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_REQUEST:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.request_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_DECLINE:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.decline_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_ACK:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.ack_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_NAK:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.nak_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_RELEASE:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.release_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_INFORM:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.inform_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_LEASEQUERY:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.leasequery_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_LEASEUNASSIGNED:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.leaseunassigned_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_LEASEUNKNOWN:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.leaseunknown_rx++;
                break;
            case DHCP_HELPER_MSG_TYPE_LEASEACTIVE:
                DHCP_HELPER_global.stats[user][isid][port_idx].rx_stats.leaseactive_rx++;
                break;
            }
        } else if (dhcp_message_direction == DHCP_HELPER_DIRECTION_TX) {
            switch (dhcp_message) {
            case DHCP_HELPER_MSG_TYPE_DISCOVER:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.discover_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_OFFER:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.offer_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_REQUEST:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.request_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_DECLINE:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.decline_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_ACK:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.ack_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_NAK:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.nak_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_RELEASE:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.release_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_INFORM:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.inform_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_LEASEQUERY:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.leasequery_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_LEASEUNASSIGNED:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.leaseunassigned_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_LEASEUNKNOWN:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.leaseunknown_tx++;
                break;
            case DHCP_HELPER_MSG_TYPE_LEASEACTIVE:
                DHCP_HELPER_global.stats[user][isid][port_idx].tx_stats.leaseactive_tx++;
                break;
            }
        }
    }
    DHCP_HELPER_CRIT_EXIT();

#if 0 // Don't use anymore
//#if defined(VTSS_SW_OPTION_DHCP_SNOOPING)
    /* When DHCP snooping mode is enabled, the DHCP Snooping statistics will be increased. */
    if (user != DHCP_HELPER_USER_SNOOPING) {
        DHCP_HELPER_stats_add(DHCP_HELPER_USER_SNOOPING, isid, dst_port_mask, dhcp_message, dhcp_message_direction);
    }
#endif /* VTSS_SW_OPTION_DHCP_SNOOPING */
}

/* Check DIP is myself interfaces */
static BOOL DHCP_HELPER_is_myself_interfaces(u32 dip)
{
#ifdef VTSS_SW_OPTION_DHCP_RELAY
    vtss_ifindex_t           prev_ifindex, ifindex;
    vtss_appl_ip_if_status_t ifstat;

    prev_ifindex = VTSS_IFINDEX_NONE;
    while (vtss_appl_ip_if_itr(&prev_ifindex, &ifindex) == VTSS_RC_OK) {
        prev_ifindex = ifindex;

        if (vtss_appl_ip_if_status_get(ifindex, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &ifstat) != VTSS_RC_OK) {
            continue;
        }

        if (ifstat.u.ipv4.net.address == dip) {
            return TRUE;
        }
    }
#endif

    return FALSE;
}

/******************************************************************************/
// DHCP_HELPER_aggr_no_get()
/******************************************************************************/
static aggr_mgmt_group_no_t DHCP_HELPER_aggr_no_get(vtss_isid_t isid, mesa_port_no_t port_no)
{
    vtss_port_participation_t aggr_status;

    // Check if port where packet was received is part of an aggregation group
    aggr_status = aggr_mgmt_port_participation(isid, port_no);

    // Get aggregation group number based on aggregation type.
    if (aggr_status == PORT_PARTICIPATION_TYPE_STATIC) {
        return aggr_mgmt_get_port_aggr_id(isid, port_no);
    } else if (aggr_status == PORT_PARTICIPATION_TYPE_LACP) {
        return aggr_lacp_mgmt_get_port_aggr_id(isid, port_no);
    }

    return 0;
}

static void DHCP_HELPER_process_rx_pkt(dhcp_helper_bip_buf_t *bip_buf)
{
    const u8 *const             packet = bip_buf->pkt;
    const size_t                len = bip_buf->len;
    const mesa_vid_t            vid = bip_buf->vid;
    const vtss_isid_t           isid = bip_buf->src_isid;
    const mesa_port_no_t        port_no =  bip_buf->src_port_no;
    const mesa_glag_no_t        glag_no = bip_buf->src_glag_no;
    dhcp_helper_frame_info_t    record_info;
    u32                         transaction_id;
    vtss::IPv4_t                bp_client_addr;
    vtss::Mac_t                 client_mac;
    vtss::dhcp::MessageType     dhcp_message;
    u8                          system_mac_addr[6];
    dhcp_helper_user_t          user;

    /* Invariant for 'user':
     *
     *      DHCP_HELPER_rx_cb[user] != NULL
     *
     * In other words, don't assign to user if the callback isn't available.
     */

    T_D("enter, RX isid %d vid %d port %d len %zd", isid, vid, port_no, len);

#if defined(VTSS_SW_OPTION_MSTP)
    /*
     * Check if ingress port is in DISCARDING state in a STP bridge and
     * discard the packet if so.
     */
    vtss_appl_mstp_bridge_status_t bstatus;
    mstp_port_mgmt_status_t pstatus;

    for (uchar msti = 0; msti < VTSS_APPL_MSTP_MAX_MSTI; msti++) {
        // Check if MSTI is valid
        if (vtss_appl_mstp_bridge_status_get(msti, &bstatus) != VTSS_RC_OK) {
            continue;
        }
        // Get port status for current MSTI
        if (!mstp_get_port_status(msti, port_no, &pstatus) && pstatus.active) {
            continue;
        }
        if (pstatus.core.state == VTSS_APPL_MSTP_PORTSTATE_DISCARDING) {
            // Ingress port is discarding in this MSTI - ignore this packet
            T_D("MSTI %u, port %u is DISCARDING - drop packet", msti, port_no);
            return;
        }
    }
#endif

    typedef const vtss::FrameRef L0;
    L0 f(packet, len);

    T_N_HEX(packet, len);

    typedef const vtss::EthernetFrame<L0> L1;
    L1 e(f);

    if (e.etype() != ETHER_TYPE_IPV4) {
        T_D("%u Ethernet: Not IP", vid);
        return;
    }

    memset(system_mac_addr, 0, 6);
    (void)conf_mgmt_mac_addr_get(system_mac_addr, 0);

    typedef const vtss::IpFrame<L1> L2;
    L2 i(e);

    if (!i.check()) {
        T_D("%u IP: Did not pass checks", vid);
        return;
    }

    if (!i.is_simple()) {
        T_D("%u IP: Non simple IP", vid);
        // BZ#20073 - Continue the process to allow the DHCP packet forwarding (if needed)
    }

    if (i.protocol() != IP_PROTOCOL_UDP) {
        T_D("%u IP: Not UDP", vid);
        return;
    }

    typedef const vtss::UdpFrame<L2> L3;
    L3 u(i);

    if (!u.check()) {
        T_D("%u UDP: Did not pass checks", vid);
        return;
    }

    if ((u.src() == DHCP_CLIENT_UDP_PORT && u.dst() == DHCP_SERVER_UDP_PORT) ||
        (u.src() == DHCP_SERVER_UDP_PORT && u.dst() == DHCP_CLIENT_UDP_PORT)) {
        T_N("%u UDP: DHCP src %d dst %d", vid, u.src(), u.dst());
    } else if (u.src() == DHCP_SERVER_UDP_PORT && u.dst() == DHCP_SERVER_UDP_PORT) {
        T_D("%u UDP: DHCP src %d dst %d This is probably a Relayed frame", vid, u.src(), u.dst());
    } else {
        T_D("%u UDP: wrong ports src %d dst %d", vid, u.src(), u.dst());
        return;
    }

    typedef const vtss::dhcp::DhcpFrame<L3> L4;
    L4 d(u);

    if (!d.check()) {
        T_D("%u DHCP-frame: Did not parse checks", vid);
        return;
    }

    /* Get DHCP message type, chaddr and transaction ID */
    vtss::dhcp::Option::MessageType message_type_;
    if (!d.option_get(message_type_)) {
        T_I("%u No message type", vid);
        return;
    }

    dhcp_message = message_type_.message_type();
    client_mac = d.chaddr();
    transaction_id = d.xid();
    bp_client_addr = d.ciaddr();

    char buf[20];
    switch (dhcp_message) {
    case DHCP_HELPER_MSG_TYPE_DISCOVER:
        sprintf(buf, "DISCOVER");
        break;
    case DHCP_HELPER_MSG_TYPE_OFFER:
        sprintf(buf, "OFFER");
        break;
    case DHCP_HELPER_MSG_TYPE_REQUEST:
        sprintf(buf, "REQUEST");
        break;
    case DHCP_HELPER_MSG_TYPE_ACK:
        sprintf(buf, "ACK");
        break;
    default:
        sprintf(buf, "%d", dhcp_message);
        break;
    }
    mesa_ipv4_t src_ip = i.src().as_int();
    mesa_ipv4_t dst_ip = i.dst().as_int();

    char buf1[24];
    char buf2[24];
    char buf3[24];
    char buf4[24];
    T_I("Got %s: VID = %u XID = 0x%08x, src_ip = %s, dst_ip = %s", buf, vid, transaction_id, misc_ipv4_txt(src_ip, buf1), misc_ipv4_txt(dst_ip, buf2));

    /* Record incoming frame information */
    memset(&record_info, 0x0, sizeof(record_info));
    memcpy(record_info.mac, client_mac.data, 6);
    record_info.vid = vid;
    record_info.isid = isid;
    record_info.port_no = port_no;
    record_info.glag_no = glag_no;
    record_info.op_code = dhcp_message;
    record_info.transaction_id = transaction_id;
    record_info.assigned_ip = bp_client_addr.as_int();

    /* Initialize DHCP user, going from lowest and up */

    DHCP_HELPER_CRIT_ENTER();

    user = DHCP_HELPER_USER_HELPER;

    if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING]) {
        user = DHCP_HELPER_USER_SNOOPING;
        T_N("user set to %u", user);
    }

    if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_RELAY]) {
        user = DHCP_HELPER_USER_RELAY;
        T_N("user set to %u", user);
    }

    /* All DHCP request packets go to Server module first */
    if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SERVER] && !DHCP_HELPER_MSG_FROM_SERVER(dhcp_message)) {
        user = DHCP_HELPER_USER_SERVER;
        T_N("user set to %u", user);
    }

    /* If it's a DHCP reply packet and "chaddr" equals switch MAC address, send to Client module */
    if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_CLIENT] &&
        DHCP_HELPER_MSG_FROM_SERVER(dhcp_message) &&
        !memcmp(system_mac_addr, client_mac.data, 6)) {
        user = DHCP_HELPER_USER_CLIENT;
        T_N("user set to %u", user);
    }

    DHCP_HELPER_CRIT_EXIT();

    /* Verify IP/UDP checksum */
    /* XXX - this is checked earlier now. Verify behavior! Possibly move stat counter update */
    if (i.calc_chksum() ||
        (u.checksum() != 0 && u.calc_chksum() != 0)) {

        DHCP_HELPER_CRIT_ENTER();
        DHCP_HELPER_global.stats[user][isid][port_no].rx_stats.discard_chksum_err_rx++;
        DHCP_HELPER_CRIT_EXIT();
        T_D("exit: IP/UDP checksum error");
        return;
    }

    DHCP_HELPER_CRIT_ENTER();
    /* Drop incoming reply packet if it arrives on untrusted port */
    if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING] &&
        DHCP_HELPER_MSG_FROM_SERVER(dhcp_message) &&
        DHCP_HELPER_global.port_conf[isid].port_mode[port_no] == DHCP_HELPER_PORT_MODE_UNTRUSTED) {

        DHCP_HELPER_global.stats[DHCP_HELPER_USER_SNOOPING][isid][port_no].rx_stats.discard_untrust_rx++;
        DHCP_HELPER_CRIT_EXIT();

        //Delete related entry
        DHCP_HELPER_frame_info_del(&record_info);
        T_D("exit: Packet from untrusted port");
        return;
    }
    DHCP_HELPER_CRIT_EXIT();


    /* Record assigned IP and lease time */
    if (dhcp_message == DHCP_HELPER_MSG_TYPE_REQUEST || dhcp_message == DHCP_HELPER_MSG_TYPE_ACK) {
        DHCP_HELPER_frame_info_parse(d, &record_info);
    }

    //Save the DHCP frame info. expect for this switch
    if (user != DHCP_HELPER_USER_CLIENT) {
        if (dhcp_message == DHCP_HELPER_MSG_TYPE_NAK || dhcp_message == DHCP_HELPER_MSG_TYPE_RELEASE) {
            //Delete related entry
            DHCP_HELPER_frame_info_del(&record_info);
            DHCP_HELPER_CRIT_ENTER();
            /* Notify snooping module that the client released the dynamic IP */
            if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING] &&
                dhcp_message == DHCP_HELPER_MSG_TYPE_RELEASE &&
                DHCP_HELPER_release_ip_addr_cb) {
                DHCP_HELPER_release_ip_addr_cb(&record_info);
            }
            DHCP_HELPER_CRIT_EXIT();
        } else if (DHCP_HELPER_frame_info_add(&record_info)) {
            T_D("Entry completed, but ack has not been sent to client");
        }
    }

    /*
     * Ok, frame has been checked, the port checked for trust,
     * and we've found the first user module to process it.
     *
     * Now proceed with user module processing, from highest priority down.
     * If the frame is processed by the user module, we're done. A user module
     * will indicate this by returning TRUE from its callback.
     *
     */

    T_N("User is set to %u", user);

    if (user == DHCP_HELPER_USER_SERVER) {
        VTSS_ASSERT(DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SERVER] != NULL);

        if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SERVER](packet, len, &record_info, DHCP_HELPER_RX_CB_FLAG_NONE)) {
            DHCP_HELPER_stats_add(user, isid, VTSS_BIT64(port_no), dhcp_message, DHCP_HELPER_DIRECTION_RX);
            T_D("exit: DHCP Server");
            return;
        }

        /* Change user if the DHCP Server module doesn't process it */
        user = DHCP_HELPER_USER_HELPER;
        T_N("user set to %u", user);
        DHCP_HELPER_CRIT_ENTER();
        if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING]) {
            user = DHCP_HELPER_USER_SNOOPING;
            T_N("user set to %u", user);
        }
        if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_RELAY]) {
            user = DHCP_HELPER_USER_RELAY;
            T_N("user set to %u", user);
        }
        DHCP_HELPER_CRIT_EXIT();
    }

    if (user == DHCP_HELPER_USER_CLIENT) {
        VTSS_ASSERT(DHCP_HELPER_rx_cb[DHCP_HELPER_USER_CLIENT] != NULL);

        DHCP_HELPER_stats_add(user, isid, VTSS_BIT64(port_no), dhcp_message, DHCP_HELPER_DIRECTION_RX);
        (void) DHCP_HELPER_rx_cb[DHCP_HELPER_USER_CLIENT](packet, len, &record_info, DHCP_HELPER_RX_CB_FLAG_NONE);
        T_D("exit: DHCP Client");
        return;
    }

    if (user == DHCP_HELPER_USER_RELAY) {
        static const u8 broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        bool from_server = DHCP_HELPER_MSG_FROM_SERVER(dhcp_message);
        bool dmac_bc     = memcmp(broadcast_mac,   e.dst().data, sizeof(broadcast_mac))   == 0;
        bool dmac_mine   = memcmp(system_mac_addr, e.dst().data, sizeof(system_mac_addr)) == 0;
        bool dip_mine    = DHCP_HELPER_is_myself_interfaces(dst_ip);
        bool sip_nz      = src_ip != 0;
        bool sip_mc      = vtss_ipv4_addr_is_multicast(&src_ip);
        bool sip_lb      = vtss_ipv4_addr_is_loopback(&src_ip);

        // In the case a client wants to renew its IP address, the DIP is the IP
        // address of the DHCP helper (DHCP server). This must also go to the
        // relay agent. However, we don't have the "ip helper-address" servers
        // here, so we give it to the relay agent indepndent of DIP - provided
        // the remaining fields match.
        bool dip_helper  = true;

        bool bc_from_client       = !from_server && dmac_bc;
        bool uc_from_server_to_me =  from_server && dmac_mine && dip_mine;
        bool uc_from_client_to_me = !from_server && dmac_mine && (dip_mine || dip_helper) && sip_nz && !sip_mc && !sip_lb;

        T_D("DMAC = %s, SMAC = %s, DIP = %s, SIP = %s, from server: %s, my DMAC: %s, my DIP: %s",
            misc_mac_txt(e.dst().data, buf1), misc_mac_txt(e.src().data, buf2),
            misc_ipv4_txt(dst_ip, buf3), misc_ipv4_txt(src_ip, buf4),
            from_server ? "Yes" : "No", dmac_mine ? "Yes" : "No", dip_mine ? "Yes" : "No");

        T_D("Broadcast packet from client: %s",               bc_from_client       ? "Yes" : "No");
        T_D("Unicast reply from server to relay: %s",         uc_from_server_to_me ? "Yes" : "No");
        T_D("Client request arriving from another relay: %s", uc_from_client_to_me ? "Yes" : "No");

        /* First check is for broadcast packets from clients, second check is for server replies to relay,
           third check is for client requests arriving from another relay.*/
        if (bc_from_client || uc_from_server_to_me || uc_from_client_to_me) {
            T_D("Sending to relay, src ip = %s", misc_ipv4_txt(src_ip, buf));

            DHCP_HELPER_stats_add(user, isid, VTSS_BIT64(port_no), dhcp_message, DHCP_HELPER_DIRECTION_RX);
            if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_RELAY]) {
                (void) DHCP_HELPER_rx_cb[DHCP_HELPER_USER_RELAY](packet, len, &record_info, DHCP_HELPER_RX_CB_FLAG_NONE);
            }

            T_D("exit: DHCP Relay");
            return;

        }

        if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING]) {
            user = DHCP_HELPER_USER_SNOOPING;
            T_N("user set to %u", user);
        } else {
            user = DHCP_HELPER_USER_HELPER;
            T_N("user set to %u", user);
        }
    }

#if defined(VTSS_SW_OPTION_IP)
    /* Do L3 forwarding if DMAC == myself && DIP != myself.
       Otherwise, do L2 forwarding */
    if (!memcmp(e.dst().data, system_mac_addr, 6) && !DHCP_HELPER_is_myself_interfaces(i.dst().as_int())) {
        DHCP_HELPER_stats_add(DHCP_HELPER_USER_HELPER, isid, VTSS_BIT64(port_no), dhcp_message, DHCP_HELPER_DIRECTION_RX);
        T_D("Under linux, update statistic only");
        T_D("exit: L3 forwarding");
        return;
    }
#endif

    if (user == DHCP_HELPER_USER_SNOOPING) {
        DHCP_HELPER_stats_add(user, isid, VTSS_BIT64(port_no), dhcp_message, DHCP_HELPER_DIRECTION_RX);
        VTSS_ASSERT(DHCP_HELPER_rx_cb[user] != NULL);
        (void) DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING](packet, len, &record_info, DHCP_HELPER_RX_CB_FLAG_NONE);
        T_D("exit: DHCP Snooping");
        return;
    }



    if (user == DHCP_HELPER_USER_HELPER && bip_buf->acl_hit) {
        /* BZ#23347:
         * DHCP packet is forwarded twice. One copy is by DHCP helper module and the other copy is by
         * Hardware broadcast address entry
         *
         * No specific DHCP user processed the DHCP packet. Forward it to other front ports.
         * The packet module rx callback will identify the incoming packet is hitted by ACL rule or not.
         * If the packet is "redirected" to CPU by ACL rule instead of "copied" to CPU,then DHCP helper
         * MUST forwards this packet to other front ports.
         */
        DHCP_HELPER_stats_add(user, isid, VTSS_BIT64(port_no), dhcp_message, DHCP_HELPER_DIRECTION_RX);

        if (dhcp_helper_xmit(DHCP_HELPER_USER_HELPER, (void *)packet, len, vid, VTSS_ISID_GLOBAL, 0, FALSE, isid, port_no, glag_no, NULL)) {
            T_D("Calling dhcp_helper_xmit() failed");
        }
        T_D("exit: DHCP Helper");
    }

    T_D("exit");
    return;
}

static BOOL DHCP_HELPER_source_port_lookup(mesa_vid_mac_t *vid_mac, vtss_isid_t *src_isid, mesa_port_no_t *src_port_no)
{
    mesa_mac_table_entry_t  mac_entry;
    switch_iter_t           sit;
    port_iter_t             pit;

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        if (mac_mgmt_table_get_next(sit.isid, vid_mac, &mac_entry, FALSE) == VTSS_RC_OK) {
            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
            while (port_iter_getnext(&pit)) {
                if (mac_entry.destination[pit.iport]) {
                    *src_isid = sit.isid;
                    *src_port_no = pit.iport;
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

/****************************************************************************/
/*  Local Transmit functions                                                */
/****************************************************************************/
static void DHCP_HELPER_msg_tx_pkt_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    if (contxt) {}
    if (rc) {}

    if (msg) {
        VTSS_FREE(msg);

        // decrease Tx cnt
        DHCP_HELPER_CRIT_ENTER();
        --DHCP_HELPER_msg_tx_pkt_cnt;
        T_D("Free: DHCP_HELPER_msg_tx_pkt_cnt = %u", DHCP_HELPER_msg_tx_pkt_cnt);
        DHCP_HELPER_CRIT_EXIT();
    }
}

static mesa_rc DHCP_HELPER_local_xmit(dhcp_helper_user_t user,
                                      const u8 *const packet,
                                      size_t len,
                                      mesa_vid_t vid,
                                      vtss_isid_t isid,
                                      aggr_mgmt_group_no_t src_aggr_no,
                                      u64 req_dst_port_mask,
                                      BOOL no_filter_dest,
                                      mesa_port_no_t src_port_no,
                                      mesa_glag_no_t src_glag_no,
                                      u8 dhcp_message,
                                      BOOL count_statistic_only)
{
    CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;
    mesa_port_no_t              port_idx;
    u8                          *frame;
    mesa_rc                     rc;
    packet_tx_props_t           tx_props;
    aggr_mgmt_group_member_t    src_port_members;
    uint64_t                    act_dst_port_mask = 0;

    T_D("Enter. user = %s, packet = %p, len = %zu, vid = %u, isid = %d, src_aggr_no = %d, req_dst_port_mask = 0x" VPRI64Fx("016") ", no_filter_dest = %d, src_port_no = %u, src_glag_no = %d, dhcp_message = %d, count_statistic_only = %d",
        user, packet, len, vid, isid, src_aggr_no, req_dst_port_mask, no_filter_dest, src_port_no, src_glag_no, dhcp_message, count_statistic_only);

    if (!no_filter_dest) {
        // Get port filter information
        mesa_packet_port_info_t info;
        if ((rc = mesa_packet_port_info_init(&info)) != VTSS_RC_OK) {
            return rc;
        }
        info.vid = vid;
        if (msg_switch_is_local(isid)) {
            info.port_no = src_port_no;
        }

        if ((rc = mesa_packet_port_filter_get(NULL, &info, filter.size(), filter.data())) != VTSS_RC_OK) {
            return rc;
        }
    }

    if (src_aggr_no != 0) {
        if (aggr_mgmt_members_get(isid, src_aggr_no, &src_port_members, 0) != VTSS_RC_OK) {
            T_D("Could not get members of aggregation group.");
            return VTSS_RC_ERROR;
        }
    }

    /* Filter destination ports */
    uint32_t txcount = 0;
    for (port_idx = VTSS_PORT_NO_START; port_idx < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_idx++) {
        if (VTSS_EXTRACT_BITFIELD64(req_dst_port_mask, port_idx,  1) == 0) {
            T_N("Skipping port_no %u, as it's not part of req_dst_port_mask", port_idx);
            continue;
        }

        if (!no_filter_dest) {
            // The frame has passed the requested destination port mask and we
            // are asked to filter based on other parameters.
            if (port_idx == src_port_no) {
                T_N("Skipping port_no %u, because that's where the frame originally arrived", port_idx);
                continue;
            }

            if (src_aggr_no != 0) {
                if (src_port_members.entry.member[port_idx]) {
                    T_N("Skipping port_no %u, because that's part of the source port's (%u) aggregation group (%u)", port_idx, src_port_no, src_aggr_no);
                    continue;
                }
            }

            if (filter[port_idx].filter == MESA_PACKET_FILTER_DISCARD) {
                // The filter also takes into account if the destination port is
                // part of an aggregation.
                T_N("Skipping port_no %u, because it would have gotten filtered by H/W in the first place", port_idx);
                continue;
            }
        }

        act_dst_port_mask |= VTSS_BIT64(port_idx);

        if (count_statistic_only) {
            T_D("Only counting statistics, skipping port %u", port_idx);
            continue;
        }

        /* Alloc frame buffer */
        if ((frame = packet_tx_alloc(len)) == NULL) {
            T_D("exit: pkt_buf = NULL");
            return DHCP_HELPER_ERROR_FRAME_BUF_ALLOCATED;
        }

        T_I("Sending on port_no %u", port_idx);
        memcpy(frame, packet, len);
        packet_tx_props_init(&tx_props);
        tx_props.tx_info.tag.vid             = vid;
        tx_props.packet_info.modid           = VTSS_MODULE_ID_DHCP_HELPER;
        tx_props.packet_info.frm             = frame;
        tx_props.packet_info.len             = len;
        tx_props.tx_info.dst_port_mask       = VTSS_BIT64(port_idx);

        if (!no_filter_dest) {
            // By setting TPID to a non-zero value, packet_tx()will insert a
            // VLAN tag according to tx_info.tag
            if (filter[port_idx].filter == MESA_PACKET_FILTER_TAGGED) {
                tx_props.tx_info.tag.tpid = filter[port_idx].tpid;
            } else {
                // In order not to get the rewriter enabled.
                tx_props.tx_info.tag.vid = VTSS_VID_NULL;
            }
        }

        if ((rc = packet_tx(&tx_props)) != VTSS_RC_OK) {
            T_D("Frame transmit failed");
        } else {
            txcount++;
        }
    }

    T_I("%u packets sent (act_dst_port_mask = 0x" VPRI64Fx("016") ")", txcount, act_dst_port_mask);

    DHCP_HELPER_stats_add(user, isid, act_dst_port_mask, dhcp_message, DHCP_HELPER_DIRECTION_TX);
    return VTSS_RC_OK;
}

#if defined(DHCP_HELPER_HW_SWITCHING)
static void DHCP_HELPER_hw_switching_xmit(mesa_vid_t vid, u8 *frame, size_t len)
{
    u8                  *pkt_buf;
    packet_tx_props_t   tx_props;
    mesa_rc             rc;

    T_D("enter: vid = %d, len = " VPRIz, vid, len);

    if ((pkt_buf = packet_tx_alloc(len)) == NULL) {
        T_D("exit: pkt_buf = NULL");
        return;
    }

    memcpy(pkt_buf, frame, len);
    packet_tx_props_init(&tx_props);

    tx_props.packet_info.modid  = VTSS_MODULE_ID_DHCP_HELPER;
    tx_props.packet_info.frm    = pkt_buf;
    tx_props.packet_info.len    = len;
    tx_props.tx_info.tag.vid    = vid;
    tx_props.tx_info.switch_frm = TRUE;

    rc = packet_tx(&tx_props);
    T_D("exit: rc = %d", rc);
}
#endif /* DHCP_HELPER_HW_SWITCHING */

/* Push back the VLAN tagged info., if needed. */
static void DHCP_HELPER_tagged_info_push_back(dhcp_helper_bip_buf_t *bip_buf)
{
    dhcp_helper_frame_info_t    record_info;

    typedef const vtss::FrameRef L0;
    L0 f(bip_buf->pkt, bip_buf->len);

    typedef const vtss::EthernetFrame<L0> L1;
    L1 e(f);

    typedef const vtss::IpFrame<L1> L2;
    L2 i(e);

    typedef const vtss::UdpFrame<L2> L3;
    L3 u(i);

    typedef const vtss::dhcp::DhcpFrame<L3> L4;
    L4 d(u);


    u8                          *ptr = (u8 *)(bip_buf->pkt);
    //    struct ip                   *ip = (struct ip *)(ptr + 14);
    //    int                         ip_header_len = IP_VHL_HL(ip->ip_hl) << 2;
    //    struct bootp                bp;

    /* Get DHCP message type, chaddr and transaction ID */
    //    memcpy(&bp, ptr + 14 + ip_header_len + 8, sizeof(bp)); /* 14:DA+SA+ETYPE, 8:udp header length */

    //    memcpy(record_info.mac, bp.bp_chaddr, 6);
    //    record_info.vid = bip_buf->vid;
    //    record_info.transaction_id = bp.bp_xid;
    if (dhcp_helper_frame_info_lookup(d.chaddr().data, bip_buf->vid, d.xid(), &record_info) &&
        record_info.tagged_info.type != DHCP_HELPER_TAGGED_TYPE_NONE) {
        /* Push back VLAN tagged info. */
        u32 tagged_offset = (record_info.tagged_info.type == DHCP_HELPER_TAGGED_TYPE_SINGLE ? 4 : 8);
        memmove(ptr + 12 + tagged_offset, ptr + 12, bip_buf->len - 12);
        memcpy(ptr + 12, record_info.tagged_info.data, tagged_offset);
        bip_buf->len += tagged_offset;
        T_D_HEX(ptr, 32);
    }
}

// Do transmit DHCP frame
static void DHCP_HELPER_tx_pkt(dhcp_helper_bip_buf_t *bip_buf)
{
    u8                broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    u8                dhcp_message;
    BOOL              flooding_pkt = FALSE;
    vtss_isid_t       isid_idx, isid_idx_start, isid_idx_end;
    dhcp_helper_msg_t *msg = NULL;
    int               need_send_cnt = 0;
    u64               temp_port_mask, send_port_mask[VTSS_ISID_END];
    BOOL              count_statistic_only = FALSE;

    typedef const vtss::FrameRef L0;
    L0 f(bip_buf->pkt, bip_buf->len);
    typedef const vtss::EthernetFrame<L0> L1;
    L1 e(f);
    typedef const vtss::IpFrame<L1> L2;
    L2 i(e);
    typedef const vtss::UdpFrame<L2> L3;
    L3 u(i);
    typedef const vtss::dhcp::DhcpFrame<L3> L4;
    L4 d(u);

    T_D("bip_buf = %s", *bip_buf);

    /* Get DHCP message type, chaddr and transaction ID */
    vtss::dhcp::Option::MessageType message_type_;
    if (!d.option_get(message_type_)) {
        T_I("%u No message type", bip_buf->vid);
        return;
    }

    dhcp_message = message_type_.message_type();

    if (bip_buf->dst_isid == VTSS_ISID_GLOBAL) {
        if (!memcmp(broadcast_mac, bip_buf->pkt, 6)) {
            flooding_pkt   = TRUE;
            isid_idx_start = VTSS_ISID_START;
            isid_idx_end   = VTSS_ISID_END - 1;
            T_D("TX Global flooding");
        } else {    // Unicast. Lookup the source port
            mesa_vid_mac_t  vid_mac;
            vtss_isid_t     unicast_pkt_src_isid = VTSS_ISID_START;
            mesa_port_no_t  unicast_pkt_src_port_no = VTSS_PORT_NO_START;

            vid_mac.vid = bip_buf->vid;
            memcpy(vid_mac.mac.addr, bip_buf->pkt, 6);
            if (DHCP_HELPER_source_port_lookup(&vid_mac, &unicast_pkt_src_isid, &unicast_pkt_src_port_no)) {
                isid_idx_start = isid_idx_end = unicast_pkt_src_isid;
                bip_buf->dst_port_mask = VTSS_BIT64(unicast_pkt_src_port_no);

                // When snooping, Filter discover, request and release packets to untrusted ports.
                DHCP_HELPER_CRIT_ENTER();
                if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING] &&
                    bip_buf->user == DHCP_HELPER_USER_SNOOPING &&
                    !DHCP_HELPER_MSG_FROM_SERVER(dhcp_message) &&
                    (DHCP_HELPER_global.port_conf[unicast_pkt_src_isid].port_mode[unicast_pkt_src_port_no] == DHCP_HELPER_PORT_MODE_UNTRUSTED)) {
                    T_D("exit: Destination port %d/%d not trusted. Packet will be dropped.", unicast_pkt_src_isid, unicast_pkt_src_port_no);
                    DHCP_HELPER_CRIT_EXIT();
                    return;
                }
                DHCP_HELPER_CRIT_EXIT();
            } else {
                // Cannot find the source port. Flood it.
                flooding_pkt   = TRUE;
                isid_idx_start = VTSS_ISID_START;
                isid_idx_end   = VTSS_ISID_END - 1;
            }
        }
    } else {
        isid_idx_start = isid_idx_end = bip_buf->dst_isid;

        /* Filter untrusted ports */
        DHCP_HELPER_CRIT_ENTER();
        if (DHCP_HELPER_rx_cb[DHCP_HELPER_USER_SNOOPING] &&
            bip_buf->user != DHCP_HELPER_USER_SNOOPING &&
            bip_buf->user != DHCP_HELPER_USER_SERVER &&
            !DHCP_HELPER_MSG_FROM_SERVER(dhcp_message) &&
            bip_buf->dst_port_mask) {
            port_iter_t pit;

            (void) port_iter_init(&pit, NULL, bip_buf->dst_isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
            while (port_iter_getnext(&pit)) {
                if (VTSS_EXTRACT_BITFIELD64(bip_buf->dst_port_mask, pit.iport,  1) == 0) {
                    continue;
                }

                /* Only forward client's packet to trust port when DHCP Snooping is enabled. */
                if (DHCP_HELPER_global.port_conf[bip_buf->dst_isid].port_mode[pit.iport] == DHCP_HELPER_PORT_MODE_UNTRUSTED) {
                    T_D("Filter untrusted ports: %d/%d", bip_buf->dst_isid, pit.iport);
                    bip_buf->dst_port_mask &= ~VTSS_BIT64(pit.iport);

                    if (bip_buf->dst_port_mask == 0) {
                        T_D("No destination ports");
                        DHCP_HELPER_CRIT_EXIT();
                        return;
                    }
                }
            }
        }
        DHCP_HELPER_CRIT_EXIT();
    }

    /* Get destination port mask */
    memset(send_port_mask, 0, sizeof(send_port_mask));
    for (isid_idx = isid_idx_start; isid_idx <= isid_idx_end; isid_idx++) {
        if (!msg_switch_exists(isid_idx)) {
            continue;
        }

        if (flooding_pkt) {
            temp_port_mask = DHCP_HELPER_dst_port_mask_get(bip_buf->vid, isid_idx, bip_buf->src_isid, bip_buf->src_port_no, bip_buf->src_glag_no, dhcp_message);
        } else {
            temp_port_mask = bip_buf->dst_port_mask;
        }

        if (temp_port_mask) {
            T_D("send_port_mask[%u] = 0x" VPRI64Fx("016"), temp_port_mask, isid_idx);
            send_port_mask[isid_idx] = temp_port_mask;
            need_send_cnt++;
        }
    }

    /* Free allocated memory if we don't need to send out any packet */
    if (need_send_cnt == 0) {
        return;
    }

#if defined(DHCP_HELPER_HW_SWITCHING)
    if (bip_buf->user == DHCP_HELPER_USER_CLIENT) {
        // Use HW switching to send out the DHCP frame
        DHCP_HELPER_hw_switching_xmit(bip_buf->vid, bip_buf->pkt, bip_buf->len);
        count_statistic_only = TRUE;
    }
#endif /* DHCP_HELPER_HW_SWITCHING */

    /* Push back the VLAN tagged info., if needed. */
    DHCP_HELPER_tagged_info_push_back(bip_buf);

    // Use SW to transmit DHCP frames or count statistic only
    for (isid_idx = isid_idx_start; isid_idx <= isid_idx_end; isid_idx++) {
        u8      *new_pkt_buf_p;
        void    *new_bufref_p;

        if (!msg_switch_exists(isid_idx) || send_port_mask[isid_idx] == 0) {
            continue;
        }

        if (msg_switch_is_local(isid_idx)) { // Bypass message module
            aggr_mgmt_group_no_t aggr_no;

            if (bip_buf->src_port_no != VTSS_PORT_NO_NONE) {
                // Get source port's aggregation so that we don't happen to send it
                // out on another port in the same aggregation as it came in on.
                aggr_no = DHCP_HELPER_aggr_no_get(isid_idx, bip_buf->src_port_no);
            } else {
                aggr_no = 0;
            }

            T_D("user = %d, src_port_no = %u/%u => aggr_no = %d", bip_buf->user, isid_idx, bip_buf->src_port_no, aggr_no);
            (void)DHCP_HELPER_local_xmit(bip_buf->user,
                                         bip_buf->pkt,
                                         bip_buf->len,
                                         bip_buf->vid,
                                         isid_idx,
                                         aggr_no,
                                         send_port_mask[isid_idx],
                                         bip_buf->no_filter_dest,
                                         isid_idx == bip_buf->src_isid ? bip_buf->src_port_no : VTSS_PORT_NO_NONE,
                                         bip_buf->src_glag_no,
                                         dhcp_message,
                                         count_statistic_only);
        } else {
            // Call msg_tx() will automatic free the dynamic memory. We need to re-alloc a new memory.
            if ((new_pkt_buf_p = (u8 *)dhcp_helper_alloc_xmit(bip_buf->len, isid_idx, &new_bufref_p)) == NULL) {
                T_D("Calling dhcp_helper_alloc_xmit() failed.\n");
                continue;
            }

            if (!count_statistic_only) {
                memcpy(new_pkt_buf_p, bip_buf->pkt, bip_buf->len);
            }
            msg = (dhcp_helper_msg_t *)new_bufref_p;
            msg->data.tx_req.user          = bip_buf->user;
            msg->data.tx_req.vid           = bip_buf->vid;
            msg->data.tx_req.isid          = isid_idx;
            msg->data.tx_req.dst_port_mask = send_port_mask[isid_idx];
            msg->data.tx_req.src_port_no   = isid_idx == bip_buf->src_isid ? bip_buf->src_port_no : VTSS_PORT_NO_NONE;
            msg->data.tx_req.src_glag_no   = bip_buf->src_glag_no;
            msg->data.tx_req.dhcp_message  = dhcp_message;
            msg->data.tx_req.count_statistic_only = count_statistic_only;

#if 1 /* Bugzilla#15786 - memory is used out */
            msg_tx_adv(NULL, DHCP_HELPER_msg_tx_pkt_done,
                       (msg_tx_opt_t)(MSG_TX_OPT_DONT_FREE | MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK),
                       VTSS_MODULE_ID_DHCP_HELPER, isid_idx, msg, bip_buf->len + sizeof(*msg));
#else
            msg_tx(VTSS_MODULE_ID_DHCP_HELPER, isid_idx, msg, bip_buf->len + sizeof(*msg));
#endif
        }
    }

    if (dhcp_message == DHCP_HELPER_MSG_TYPE_OFFER ||
        dhcp_message == DHCP_HELPER_MSG_TYPE_ACK ||
        dhcp_message == DHCP_HELPER_MSG_TYPE_NAK) {
        dhcp_helper_frame_info_t    record_info;
        memset(&record_info, 0x0, sizeof(record_info));
        memcpy(record_info.mac, d.chaddr().data, 6);
        record_info.vid = bip_buf->vid;
        record_info.transaction_id = d.xid();
        record_info.op_code = dhcp_message;

#if defined(VTSS_SW_OPTION_DHCP_SERVER) || defined(VTSS_SW_OPTION_DHCP_RELAY)
        if (dhcp_message == DHCP_HELPER_MSG_TYPE_OFFER ||
            dhcp_message == DHCP_HELPER_MSG_TYPE_ACK) {
            if (dhcp_message == DHCP_HELPER_MSG_TYPE_ACK) {
                /* Record assigned IP and lease time */
                DHCP_HELPER_frame_info_parse(d, &record_info);

                if (bip_buf->user == DHCP_HELPER_USER_SERVER) {
                    record_info.local_dhcp_server = TRUE;
                }
            }
            (void) DHCP_HELPER_frame_info_add(&record_info);
        }
#endif /* VTSS_SW_OPTION_DHCP_SERVER || VTSS_SW_OPTION_DHCP_RELAY */

        if (dhcp_message == DHCP_HELPER_MSG_TYPE_ACK ||
            dhcp_message == DHCP_HELPER_MSG_TYPE_NAK) {
            /* Clear related record informations when relayed packets to client,
            cause we got the output sid/port now, don't need it anymore */
            DHCP_HELPER_frame_info_clear(&record_info, TRUE);
        }
    }
}

static void DHCP_HELPER_bip_buffer_rx_enqueue(const u8 *const packet,
                                              size_t len,
                                              mesa_vid_t vid,
                                              vtss_isid_t isid,
                                              mesa_port_no_t port_no,
                                              mesa_glag_no_t glag_no,
                                              BOOL acl_hit)
{
    dhcp_helper_bip_buf_t   *bip_buf;
    int                     i, data_offset = 12; // DMAC + SMAC
    u16                     *vlan_hdr = (u16 *)(packet + data_offset), etype = ntohs(*vlan_hdr);
    BOOL                    single_tagged = FALSE, double_tagged = FALSE;
    mesa_etype_t            tpid;

    T_D("enter, port_no: %u len " VPRIz" vid %d glag %u acl_hit %s", port_no, len, vid, glag_no, acl_hit ? "T" : "F");

    /* Check input parameters */
    if (packet == NULL || len == 0) {
        T_E("exit: packet == NULL or len == 0");
        return;
    }

    DHCP_HELPER_CRIT_ENTER();
    tpid = DHCP_HELPER_vlan_tpid;
    DHCP_HELPER_CRIT_EXIT();

    // Skip VLAN tags, notice that the packet module may strip off the first tag in the packet content
    for (i = 0; i < 2; i++) {
        if (etype == 0x8100 || etype == 0x88A8 || etype == tpid) {
            if (single_tagged == FALSE) {
                single_tagged = TRUE;
            } else {
                single_tagged = FALSE;
                double_tagged = TRUE;
            }
            data_offset += 4;
            vlan_hdr = (u16 *)(packet + data_offset);
            etype = ntohs(*vlan_hdr);
        }
    }

    DHCP_HELPER_BIP_CRIT_ENTER();
    bip_buf = (dhcp_helper_bip_buf_t *)vtss_bip_buffer_reserve(&DHCP_HELPER_bip_buf, DHCP_HELPER_BIP_BUF_ALIGNED_SIZE);
    if (bip_buf == NULL) {
        DHCP_HELPER_BIP_CRIT_EXIT();
        T_D("exit: Failure in reserving DHCP Helper BIP buffer");
        return;
    }

    /* Record the VLAN tagged info. */
    memset(&bip_buf->tagged_info, 0, sizeof(bip_buf->tagged_info));
    if (single_tagged) {
        data_offset = 4;
        bip_buf->tagged_info.type = DHCP_HELPER_TAGGED_TYPE_SINGLE;
        memcpy(bip_buf->tagged_info.data, packet + 12, 4);
    } else if (double_tagged) {
        data_offset = 8;
        bip_buf->tagged_info.type = DHCP_HELPER_TAGGED_TYPE_DOUBLE;
        memcpy(bip_buf->tagged_info.data, packet + 12, 8);
    } else {
        data_offset = 0;
    }

    /* Pop the VLAN tagged info. */
    memcpy(bip_buf->pkt, packet, 12);
    memcpy(bip_buf->pkt + 12, packet + 12 + data_offset, len - 12 - data_offset);
    bip_buf->len         = len - data_offset;
    bip_buf->is_rx       = TRUE;
    bip_buf->vid         = vid;
    bip_buf->src_isid    = isid;
    bip_buf->src_port_no = port_no;
    bip_buf->src_glag_no = glag_no;
    bip_buf->acl_hit     = acl_hit;

    T_D_HEX(bip_buf->pkt, 64);
    T_D("bip_buf = %s", *bip_buf);

    vtss_bip_buffer_commit(&DHCP_HELPER_bip_buf);
    vtss_flag_setbits(&DHCP_HELPER_bip_buffer_thread_events, DHCP_HELPER_EVENT_PKT_RX);
    DHCP_HELPER_BIP_CRIT_EXIT();
    T_D("exit");
}

static void DHCP_HELPER_bip_buffer_dequeue(void)
{
    dhcp_helper_bip_buf_t *bip_buf;
    int                   buf_size;

    do {
        DHCP_HELPER_BIP_CRIT_ENTER();
        bip_buf = (dhcp_helper_bip_buf_t *)vtss_bip_buffer_get_contiguous_block(&DHCP_HELPER_bip_buf, &buf_size);
        DHCP_HELPER_BIP_CRIT_EXIT();

        if (bip_buf) {
            if (buf_size < (int)DHCP_HELPER_BIP_BUF_ALIGNED_SIZE) {
                T_E("Odd. buf_size = %d, expected at least %zd", buf_size, DHCP_HELPER_BIP_BUF_ALIGNED_SIZE);
            } else if (bip_buf->is_rx) {
                T_D("BIP buffer Rx, len = %zd", bip_buf->len);
                DHCP_HELPER_process_rx_pkt(bip_buf);
            } else {
                T_D("BIP buffer Tx, len = %zd", bip_buf->len);
                DHCP_HELPER_tx_pkt(bip_buf);
            }

            if (buf_size) {
                DHCP_HELPER_BIP_CRIT_ENTER();
                vtss_bip_buffer_decommit_block(&DHCP_HELPER_bip_buf, DHCP_HELPER_BIP_BUF_ALIGNED_SIZE);
                DHCP_HELPER_BIP_CRIT_EXIT();
            }
        }
    } while (bip_buf);
}

/****************************************************************************/
/*  Reserved ACEs functions                                                 */
/****************************************************************************/
/* Add reserved ACE */
static void DHCP_HELPER_ace_add(void)
{
    acl_entry_conf_t conf;

    /* Set two ACEs (bootps and bootpc) on all switches (primary and secondary). */
    //bootps
    if (acl_mgmt_ace_init(MESA_ACE_TYPE_IPV4, &conf) != VTSS_RC_OK) {
        return;
    }
    conf.id = DHCP_HELPER_BOOTPS_ACE_ID;

    conf.isdx_disable = TRUE;

    conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
    memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
    conf.action.force_cpu = TRUE;
    conf.action.cpu_queue = PACKET_XTR_QU_ACL_REDIR;
    conf.action.cpu_once = FALSE;
    conf.action.inject_manual = TRUE;
    conf.action.inject_into_ip_stack = TRUE;
    conf.isid = VTSS_ISID_LOCAL;
    VTSS_BF_SET(conf.flags.mask, ACE_FLAG_IP_FRAGMENT, 1);
    VTSS_BF_SET(conf.flags.value, ACE_FLAG_IP_FRAGMENT, 0);
    conf.frame.ipv4.proto.value = 17; //UDP
    conf.frame.ipv4.proto.mask = 0xFF;
    conf.frame.ipv4.sport.low = conf.frame.ipv4.sport.high = DHCP_SERVER_UDP_PORT;
    conf.frame.ipv4.dport.low = conf.frame.ipv4.dport.high = DHCP_CLIENT_UDP_PORT;
    if (acl_mgmt_ace_add(ACL_USER_DHCP, ACL_MGMT_ACE_ID_NONE, &conf) != VTSS_RC_OK) {
        T_D("Add DHCP helper reserved ACE (BOOTPS) fail.\n");
    }

    //bootpc
    conf.id = DHCP_HELPER_BOOTPC_ACE_ID;
    conf.frame.ipv4.sport.low = conf.frame.ipv4.sport.high = DHCP_CLIENT_UDP_PORT;
    conf.frame.ipv4.dport.low = conf.frame.ipv4.dport.high = DHCP_SERVER_UDP_PORT;
    if (acl_mgmt_ace_add(ACL_USER_DHCP, ACL_MGMT_ACE_ID_NONE, &conf) != VTSS_RC_OK) {
        T_D("Add DHCP helper reserved ACE (BOOTPC) fail.\n");
    }
}

/* Delete reserved ACE */
static void DHCP_HELPER_ace_del(void)
{
    acl_entry_conf_t conf;

    while (acl_mgmt_ace_get(ACL_USER_DHCP, ACL_MGMT_ACE_ID_NONE, &conf, NULL, TRUE) == VTSS_RC_OK) {
        if (acl_mgmt_ace_del(ACL_USER_DHCP, conf.id) != VTSS_RC_OK) {
            T_D("Calling acl_mgmt_ace_del(user=%d, ace_id=%d) failed.\n", ACL_USER_DHCP, conf.id);
        }
    }
}

/****************************************************************************/
/*  Message functions                                                       */
/****************************************************************************/

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
static const char *DHCP_HELPER_msg_id_txt(dhcp_helper_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case DHCP_HELPER_MSG_ID_FRAME_RX_IND:
        txt = "DHCP_HELPER_MSG_ID_FRAME_RX_IND";
        break;
    case DHCP_HELPER_MSG_ID_FRAME_TX_REQ:
        txt = "DHCP_HELPER_MSG_ID_FRAME_TX_REQ";
        break;
    case DHCP_HELPER_MSG_ID_LOCAL_ACE_SET:
        txt = "DHCP_HELPER_MSG_ID_LOCAL_ACE_SET";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}
#endif /* VTSS_TRACE_LVL_DEBUG */

/* Allocate request buffer */
static dhcp_helper_msg_t *DHCP_HELPER_msg_alloc(dhcp_helper_msg_buf_t *buf, dhcp_helper_msg_id_t msg_id, BOOL request)
{
    dhcp_helper_msg_t *msg;

    DHCP_HELPER_CRIT_ENTER();
    buf->sem = request ? &DHCP_HELPER_global.request.sem : &DHCP_HELPER_global.reply.sem;
    msg = request ? &DHCP_HELPER_global.request.msg : &DHCP_HELPER_global.reply.msg;
    buf->msg = msg;
    DHCP_HELPER_CRIT_EXIT();
    vtss_sem_wait(buf->sem);
    msg->msg_id = msg_id;
    return msg;
}

static void DHCP_HELPER_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    dhcp_helper_msg_id_t msg_id = *(dhcp_helper_msg_id_t *)msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s", msg_id, DHCP_HELPER_msg_id_txt(msg_id));
    vtss_sem_post((vtss_sem_t *)contxt);
}

static void DHCP_HELPER_msg_tx(dhcp_helper_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    dhcp_helper_msg_id_t msg_id = *(dhcp_helper_msg_id_t *)buf->msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s, len: %zd, isid: %d", msg_id, DHCP_HELPER_msg_id_txt(msg_id), len, isid);
    msg_tx_adv(buf->sem, DHCP_HELPER_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_MODULE_ID_DHCP_HELPER, isid, buf->msg, len + MSG_TX_DATA_HDR_LEN(dhcp_helper_msg_t, data));
}

/* When we're the primary swtich, the isid is the secondary switch's ISID.
   When we're a secondary switch, the isid is sthe connection ID which is the "arbitrarily" chosen mcb->connid.
   Please be careful under the secondary switch role. In such case, we *MUST* use 'VTSS_ISID_LOCAL' instead of the value of isid.
*/
static BOOL DHCP_HELPER_msg_rx(void *contxt,
                               const void *rx_msg,
                               size_t len,
                               vtss_module_id_t modid,
                               u32 isid)
{
    const dhcp_helper_msg_t *msg = (dhcp_helper_msg_t *)rx_msg;

    T_D("Sid %u, rx %zd bytes, msg %d", isid, len, msg->msg_id);

    switch (msg->msg_id) {
    case DHCP_HELPER_MSG_ID_FRAME_RX_IND: {

        /* The API of mac_mgmt_table_get_next() will use msg to get secondary switch data.
           It will occur message timeout in current message thread.
           To avoid it, we store the incoming packet to BIP buffer first and
           use another thread to process it. */
        if (msg_switch_is_primary()) {
            DHCP_HELPER_bip_buffer_rx_enqueue((u8 *)&msg[1], msg->data.rx_ind.len, msg->data.rx_ind.vid, isid, msg->data.rx_ind.port_no, msg->data.rx_ind.glag_no, msg->data.rx_ind.acl_hit);
        }
        break;
    }
    case DHCP_HELPER_MSG_ID_FRAME_TX_REQ: {
        (void)DHCP_HELPER_local_xmit(msg->data.tx_counters_get.user,
                                     (u8 *)&msg[1],
                                     msg->data.tx_req.len,
                                     msg->data.tx_req.vid,
                                     msg->data.tx_req.isid,
                                     0,
                                     msg->data.tx_req.dst_port_mask,
                                     FALSE,
                                     msg->data.tx_req.src_port_no,
                                     msg->data.tx_req.src_glag_no,
                                     msg->data.tx_req.dhcp_message,
                                     msg->data.tx_req.count_statistic_only);
        break;
    }
    case DHCP_HELPER_MSG_ID_LOCAL_ACE_SET: {

        if (msg->data.local_ace_set.add) {
            DHCP_HELPER_rx_filter_register(TRUE);
            if (msg->data.local_ace_set.register_only) {
                DHCP_HELPER_ace_del();
                DHCP_HELPER_frame_info_clear_all();
            } else {
                DHCP_HELPER_ace_add();
            }
        } else {
            DHCP_HELPER_ace_del();
            DHCP_HELPER_rx_filter_register(FALSE);
            DHCP_HELPER_frame_info_clear_all();
        }
        break;
    }
    case DHCP_HELPER_MSG_ID_COUNTERS_GET_REQ: {

        if (msg->data.tx_counters_get.port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
            dhcp_helper_msg_t       *msg_rep;
            dhcp_helper_msg_buf_t   buf;

            msg_rep = DHCP_HELPER_msg_alloc(&buf, DHCP_HELPER_MSG_ID_COUNTERS_GET_REP, FALSE);
            msg_rep->data.tx_counters_get.user = msg->data.tx_counters_get.user;
            msg_rep->data.tx_counters_get.port_no = msg->data.tx_counters_get.port_no;
            msg_rep->data.tx_counters_get.tx_stats = DHCP_HELPER_global.stats[msg->data.tx_counters_get.user][msg_switch_is_primary() ? isid : VTSS_ISID_LOCAL][msg->data.tx_counters_get.port_no].tx_stats;
            DHCP_HELPER_msg_tx(&buf, isid, sizeof(msg_rep->data.tx_counters_get));
        }
        break;
    }
    case DHCP_HELPER_MSG_ID_COUNTERS_GET_REP: {

        if (msg_switch_is_primary() &&
            msg->data.tx_counters_get.port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {   // primary switch only
            dhcp_helper_user_t user = msg->data.tx_counters_get.user;
            dhcp_helper_stats_tx_t *tx_stats;
            DHCP_HELPER_CRIT_ENTER();
            tx_stats = &DHCP_HELPER_global.stats[user][isid][msg->data.tx_counters_get.port_no].tx_stats;
            if (user == DHCP_HELPER_USER_RELAY && !msg_switch_is_local(isid)) {
                //Relay's TX statistics: Secondary switch local counter +  primary switch socket counter
                tx_stats->discover_tx          += msg->data.tx_counters_get.tx_stats.discover_tx;
                tx_stats->offer_tx             += msg->data.tx_counters_get.tx_stats.offer_tx;
                tx_stats->request_tx           += msg->data.tx_counters_get.tx_stats.request_tx;
                tx_stats->decline_tx           += msg->data.tx_counters_get.tx_stats.decline_tx;
                tx_stats->ack_tx               += msg->data.tx_counters_get.tx_stats.ack_tx;
                tx_stats->nak_tx               += msg->data.tx_counters_get.tx_stats.nak_tx;
                tx_stats->release_tx           += msg->data.tx_counters_get.tx_stats.release_tx;
                tx_stats->inform_tx            += msg->data.tx_counters_get.tx_stats.inform_tx;
                tx_stats->leasequery_tx        += msg->data.tx_counters_get.tx_stats.leasequery_tx;
                tx_stats->leaseunassigned_tx   += msg->data.tx_counters_get.tx_stats.leaseunassigned_tx;
                tx_stats->leaseunknown_tx      += msg->data.tx_counters_get.tx_stats.leaseunknown_tx;
                tx_stats->leaseactive_tx       += msg->data.tx_counters_get.tx_stats.leaseactive_tx;
            } else {
                *tx_stats = msg->data.tx_counters_get.tx_stats;
            }
            VTSS_MTIMER_START(&DHCP_HELPER_global.stats_timer[isid], DHCP_HELPER_COUNTERS_TIMER);
            vtss_flag_setbits(&DHCP_HELPER_global.stats_flags, 1 << isid);
            DHCP_HELPER_CRIT_EXIT();
        }
        break;
    }
    case DHCP_HELPER_MSG_ID_COUNTERS_CLR: {

        DHCP_HELPER_CRIT_ENTER();
        if (msg->data.counters_clear.user == DHCP_HELPER_USER_CNT) {
            dhcp_helper_user_t user_idx;
            for (user_idx = DHCP_HELPER_USER_HELPER; user_idx < DHCP_HELPER_USER_CNT; user_idx++) {
                if (msg->data.counters_clear.port_no == fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                    memset(&DHCP_HELPER_global.stats[user_idx][VTSS_ISID_LOCAL][msg->data.counters_clear.port_no], 0, sizeof(dhcp_helper_stats_t));
                } else {
                    for (size_t i = 0; i < DHCP_HELPER_global.stats[user_idx][VTSS_ISID_LOCAL].size(); ++i) {
                        vtss_clear(DHCP_HELPER_global.stats[user_idx][VTSS_ISID_LOCAL][i]);
                    }
                }
            }
        } else {
            if (msg->data.counters_clear.port_no == fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                for (size_t i = 0; i < DHCP_HELPER_global.stats[msg->data.counters_clear.user][VTSS_ISID_LOCAL].size(); ++i) {
                    vtss_clear(DHCP_HELPER_global.stats[msg->data.counters_clear.user][VTSS_ISID_LOCAL][i]);
                }
            } else {
                memset(&DHCP_HELPER_global.stats[msg->data.counters_clear.user][VTSS_ISID_LOCAL][msg->data.counters_clear.port_no], 0, sizeof(dhcp_helper_stats_t));
            }
        }
        DHCP_HELPER_CRIT_EXIT();
        break;
    }
    default:
        T_W("unknown message ID: %d", msg->msg_id);

        break;
    }
    return TRUE;
}

/* Stack Register */
static mesa_rc DHCP_HELPER_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = DHCP_HELPER_msg_rx;
    filter.modid = VTSS_MODULE_ID_DHCP_HELPER;
    return msg_rx_filter_register(&filter);
}

/* Set stack DHCP helper local ACEs */
static void DHCP_HELPER_stack_ace_conf_set(vtss_isid_t isid_add)
{
    dhcp_helper_msg_t       *msg;
    dhcp_helper_msg_buf_t   buf;
    vtss_isid_t             isid;
    dhcp_helper_user_t      user_idx;
#if defined(DHCP_HELPER_HW_SWITCHING)
    int                     used_user_cnt = 0;
#endif /* DHCP_HELPER_HW_SWITCHING */


    T_D("enter, isid_add: %d", isid_add);
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if ((isid_add != VTSS_ISID_GLOBAL && isid_add != isid) ||
            !msg_switch_exists(isid)) {
            continue;
        }
        msg = DHCP_HELPER_msg_alloc(&buf, DHCP_HELPER_MSG_ID_LOCAL_ACE_SET, TRUE);
        msg->data.local_ace_set.add = FALSE;
        msg->data.local_ace_set.register_only = FALSE;

        DHCP_HELPER_CRIT_ENTER();
        for (user_idx = DHCP_HELPER_USER_HELPER; user_idx < DHCP_HELPER_USER_CNT; user_idx++) {
#if defined(DHCP_HELPER_HW_SWITCHING)
            if (DHCP_HELPER_rx_cb[user_idx]) {
                used_user_cnt++;
            }
#endif /* DHCP_HELPER_HW_SWITCHING */
            if (msg->data.local_ace_set.add == FALSE) {
                msg->data.local_ace_set.add = (DHCP_HELPER_rx_cb[user_idx] ? TRUE : FALSE);
            }
        }
#if defined(DHCP_HELPER_HW_SWITCHING)
        if (used_user_cnt == 1 && DHCP_HELPER_rx_cb[DHCP_HELPER_USER_CLIENT]) {
            msg->data.local_ace_set.register_only = TRUE;
        }
#endif /* DHCP_HELPER_HW_SWITCHING */
        DHCP_HELPER_CRIT_EXIT();
        DHCP_HELPER_msg_tx(&buf, isid, sizeof(msg->data.local_ace_set));
    }

    T_D("exit, isid_add: %d", isid_add);
}


/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* DHCP helper error text */
const char *dhcp_helper_error_txt(mesa_rc rc)
{
    switch (rc) {
    case DHCP_HELPER_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on primary switch";

    case DHCP_HELPER_ERROR_ISID:
        return "Invalid Switch ID";

    case DHCP_HELPER_ERROR_ISID_NON_EXISTING:
        return "Switch ID is non-existing";

    case DHCP_HELPER_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    case DHCP_HELPER_ERROR_REQ_TIMEOUT:
        return "Request timeout";

    case DHCP_HELPER_ERROR_FRAME_BUF_ALLOCATED:
        return "DHCP frame buffer allocated fail";

    default:
        return "DHCP Helper: Unknown error code";
    }
}

/* Get DHCP helper port configuration */
mesa_rc dhcp_helper_mgmt_port_conf_get(vtss_isid_t isid, dhcp_helper_port_conf_t *switch_cfg)
{
    T_D("enter");

    if (switch_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_HELPER_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_HELPER_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return DHCP_HELPER_ERROR_ISID_NON_EXISTING;
    }

    DHCP_HELPER_CRIT_ENTER();
    *switch_cfg = DHCP_HELPER_global.port_conf[isid];
    DHCP_HELPER_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set DHCP helper port configuration */
mesa_rc dhcp_helper_mgmt_port_conf_set(vtss_isid_t isid, dhcp_helper_port_conf_t *switch_cfg)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    if (switch_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_HELPER_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_HELPER_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return DHCP_HELPER_ERROR_ISID_NON_EXISTING;
    }

    DHCP_HELPER_CRIT_ENTER();
    DHCP_HELPER_global.port_conf[isid] = *switch_cfg;
    DHCP_HELPER_CRIT_EXIT();

    T_D("exit");

    return rc;
}

/****************************************************************************
 * Module thread
 ****************************************************************************/
/* Clear the uncompleted entries in period time */
static void DHCP_HELPER_thread(vtss_addrword_t data)
{
    while (1) {
        if (msg_switch_is_primary()) {
            while (msg_switch_is_primary()) {
                VTSS_OS_MSLEEP(DHCP_HELPER_FRAME_INFO_MONITOR_INTERVAL);
                DHCP_HELPER_frame_info_clear_obsoleted();
            }
        }

        DHCP_HELPER_bip_buffer_dequeue();

        // No reason for using CPU ressources when we're a secondary switch
        T_D("Suspending DHCP helper thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_DHCP_HELPER);
        T_D("Resumed DHCP helper thread");
    }
}

static void DHCP_HELPER_bip_buffer_thread(vtss_addrword_t data)
{
    while (1) {
        if (msg_switch_is_primary()) {
            while (msg_switch_is_primary()) {
                (void)vtss_flag_wait(&DHCP_HELPER_bip_buffer_thread_events, DHCP_HELPER_EVENT_ANY, VTSS_FLAG_WAITMODE_OR_CLR);
                T_D("Dequeueing");
                DHCP_HELPER_bip_buffer_dequeue();
            }
        }

        // No reason for using CPU ressources when we're a secondary switch
        T_D("Suspending DHCP helper bip buffer thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_DHCP_HELPER);
        T_D("Resumed DHCP helper bip buffer thread");
    }
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
static void DHCP_HELPER_vlan_tpid_change_cb(mesa_etype_t tpid)
{
    T_D("New TPID: 0x%04x", tpid);
    DHCP_HELPER_CRIT_ENTER();
    DHCP_HELPER_vlan_tpid = tpid;
    DHCP_HELPER_CRIT_EXIT();
}

static void DHCP_HELPER_vlan_tpid_init(void)
{
    mesa_rc      rc;
    mesa_etype_t tpid;

    // Get notified if the S-custom VLAN tag's TPID changes
    vlan_s_custom_etype_change_register(VTSS_MODULE_ID_DHCP_HELPER, DHCP_HELPER_vlan_tpid_change_cb);

    // And update our own cached version.
    if ((rc = vtss_appl_vlan_s_custom_etype_get(&tpid)) != VTSS_RC_OK) {
        tpid = 0x88a8;
        T_W("vtss_appl_vlan_s_custom_etype_get() failed (%s). Setting to 0x88a8", error_txt(rc));
    }

    DHCP_HELPER_vlan_tpid_change_cb(tpid);
}

/* Module start */
static void DHCP_HELPER_start(BOOL init)
{
    mesa_rc     rc;
    vtss_isid_t isid_idx;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize callback function reference */
        memset(DHCP_HELPER_rx_cb, 0, sizeof(DHCP_HELPER_rx_cb));
        memset(DHCP_HELPER_clear_local_stat_cb, 0, sizeof(DHCP_HELPER_clear_local_stat_cb));

        /* Initialize DHCP helper port configuration */
        vtss_clear(DHCP_HELPER_global.stats);

        /* Initialize message buffers */
        vtss_sem_init(&DHCP_HELPER_global.request.sem, 1);
        vtss_sem_init(&DHCP_HELPER_global.reply.sem, 1);

        /* Initialize BIP buffer */
        if (!vtss_bip_buffer_init(&DHCP_HELPER_bip_buf, DHCP_HELPER_BIP_BUF_TOTAL_SIZE)) {
            T_E("vtss_bip_buffer_init failed!");
        }

        /* Create semaphore for critical regions */
        critd_init(&DHCP_HELPER_global.crit,     "dhcp_helper",     VTSS_MODULE_ID_DHCP_HELPER, CRITD_TYPE_MUTEX);
        critd_init(&DHCP_HELPER_global.bip_crit, "dhcp_helper_bip", VTSS_MODULE_ID_DHCP_HELPER, CRITD_TYPE_MUTEX);

        /* Initialize counter timers */
        for (isid_idx = VTSS_ISID_START; isid_idx < VTSS_ISID_END; isid_idx++) {
            VTSS_MTIMER_START(&DHCP_HELPER_global.stats_timer[isid_idx], 1);
        }

        /* Initialize counter flag */
        vtss_flag_init(&DHCP_HELPER_global.stats_flags);

        /* Initialize BIP buffer flag */
        vtss_flag_init(&DHCP_HELPER_bip_buffer_thread_events);

        /* Initialize Ethertype for Custom S-ports */
        DHCP_HELPER_vlan_tpid_init();

        /* Initialize frame information list */
        DHCP_HELPER_CRIT_ENTER();
        DHCP_HELPER_frame_info_init();
        DHCP_HELPER_CRIT_EXIT();

        /* Create DHCP helper thread */
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           DHCP_HELPER_thread,
                           0,
                           "DHCP Helper",
                           nullptr,
                           0,
                           &DHCP_HELPER_thread_handle,
                           &DHCP_HELPER_thread_block);

        /* Create DHCP helper bip buffer thread */
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           DHCP_HELPER_bip_buffer_thread,
                           0,
                           "DHCP Helper bip buffer",
                           nullptr,
                           0,
                           &DHCP_HELPER_bip_buffer_thread_handle,
                           &DHCP_HELPER_bip_buffer_thread_block);

    } else {
        /* Register port change callback function */
        if ((rc = port_change_register(VTSS_MODULE_ID_DHCP_HELPER, DHCP_HELPER_port_change_callback)) != VTSS_RC_OK) {
            T_E("Calling port_change_register() failed, rc = %d", rc);
        }

        /* Register for stack messages */
        if ((rc = DHCP_HELPER_stack_register()) != VTSS_RC_OK) {
            T_W("VOICE_VLAN_stack_register(): failed rc = %d", rc);
        }
    }
    T_D("exit");
}

extern "C" int dhcp_helper_icli_cmd_register();

/* Initialize module */
mesa_rc dhcp_helper_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        DHCP_HELPER_start(1);
        dhcp_helper_icli_cmd_register();
        break;

    case INIT_CMD_START:
        /* Register for stack messages */
        DHCP_HELPER_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            DHCP_HELPER_CRIT_ENTER();
            vtss_clear(DHCP_HELPER_global.stats);
            DHCP_HELPER_CRIT_EXIT();
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");

        /* Clear statistics */
        DHCP_HELPER_CRIT_ENTER();
        vtss_clear(DHCP_HELPER_global.stats);
        DHCP_HELPER_CRIT_EXIT();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        DHCP_HELPER_stack_ace_conf_set(isid);
        break;

    default:
        break;
    }

    T_D("exit");

    return VTSS_RC_OK;
}

/****************************************************************************/
/*  DHCP helper receive functions                                           */
/****************************************************************************/

// Avoid "Custodual pointer 'msg' has not been freed or returned, since
// the msg is freed by the message module.
/*lint -e{429} */
static void DHCP_HELPER_receive_indication(const u8 *const packet,
                                           size_t len,
                                           mesa_vid_t vid,
                                           mesa_port_no_t switchport,
                                           mesa_glag_no_t glag_no,
                                           BOOL acl_hit)
{
    T_D("len %zd port %u vid %d glag %u acl_hit %s", len, switchport, vid, glag_no, acl_hit ? "T" : "F");

    if (msg_switch_is_primary()) {   /* Bypass message module! */
        DHCP_HELPER_bip_buffer_rx_enqueue(packet, len, vid, msg_primary_switch_isid(), switchport, glag_no, acl_hit);
    } else {
        size_t msg_len = sizeof(dhcp_helper_msg_t) + len;
        dhcp_helper_msg_t *msg = (dhcp_helper_msg_t *)VTSS_MALLOC(msg_len);
        if (msg) {
            msg->msg_id = DHCP_HELPER_MSG_ID_FRAME_RX_IND;
            msg->data.rx_ind.len = len;
            msg->data.rx_ind.vid = vid;
            msg->data.rx_ind.port_no = switchport;
            msg->data.rx_ind.glag_no = glag_no;
            msg->data.rx_ind.acl_hit = acl_hit;
            memcpy(&msg[1], packet, len); /* Copy frame */
            //These frames are subject to shaping.
            msg_tx_adv(NULL, NULL, (msg_tx_opt_t)(MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK | MSG_TX_OPT_SHAPE), VTSS_MODULE_ID_DHCP_HELPER, 0, msg, msg_len);
        } else {
            T_W("Unable to allocate %zd bytes, tossing frame on port %u", msg_len, switchport);
        }
    }
}


/****************************************************************************/
/*  Rx filter register functions                                            */
/****************************************************************************/

/* Local port packet receive indication - forward through DHCP helper */
static BOOL DHCP_HELPER_rx_packet_callback(void *contxt, const u8 *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    BOOL rc = FALSE;
    u16  sport;
    u16  dport;

    T_D("enter, port_no: %u len %d vid %d", rx_info->port_no, rx_info->length, rx_info->tag.vid);

    typedef const vtss::FrameRef L0;
    L0 f(frm, rx_info->length);

    T_D_HEX(frm, rx_info->length);

    typedef const vtss::EthernetFrame<L0> L1;
    L1 e(f);

    if (e.etype() != ETHER_TYPE_IPV4) {
        T_D("Ethernet: Not IP");
        return rc;
    }

    typedef const vtss::IpFrame<L1> L2;
    L2 i(e);

    if (!i.check()) {
        T_D("IP: Did not pass checks");
        return rc;
    }

    if (!i.is_simple()) {
        T_D("IP: Non simple IP");
        // BZ#20073 - Continue the process to allow the DHCP packet forwarding (if needed)
    }

    if (i.protocol() != IP_PROTOCOL_UDP) {
        T_D("IP: Not UDP");
        return rc;
    }

    typedef const vtss::UdpFrame<L2> L3;
    L3 u(i);

    if (!u.check()) {
        T_D("UDP: Did not pass checks");
        return rc;
    }

    sport = u.src();
    dport = u.dst();

    if ((sport == DHCP_SERVER_UDP_PORT && dport == DHCP_CLIENT_UDP_PORT) ||
        (sport == DHCP_CLIENT_UDP_PORT && dport == DHCP_SERVER_UDP_PORT)) {
        T_D("Server or client packet.");
        DHCP_HELPER_receive_indication(frm, rx_info->length, rx_info->tag.vid, rx_info->port_no, VTSS_GLAG_NO_NONE, rx_info->acl_hit);
        rc = TRUE; // Do not allow other subscribers to receive the packet
    } else if (sport == DHCP_SERVER_UDP_PORT && dport == DHCP_SERVER_UDP_PORT) { // This condition is valid when message arrives through a relay agent
        T_D("Packet arriving through relay agent");
        DHCP_HELPER_receive_indication(frm, rx_info->length, rx_info->tag.vid, rx_info->port_no, VTSS_GLAG_NO_NONE, rx_info->acl_hit);
        rc = TRUE; // Do not allow other subscribers to receive the pa
    } else {
        T_D("UDP: wrong ports src %d dst %d", sport, dport);
        return rc;
    }

    T_D("exit");

    return rc;
}

static void DHCP_HELPER_rx_filter_register(BOOL registerd)
{
    DHCP_HELPER_CRIT_ENTER();

    if (!DHCP_HELPER_filter_id) {
        packet_rx_filter_init(&DHCP_HELPER_rx_filter);
    }

    DHCP_HELPER_rx_filter.modid            = VTSS_MODULE_ID_DHCP_HELPER;
#if defined(DHCP_HELPER_HW_SWITCHING)
    T_D("Hardware switching defined.");
    DHCP_HELPER_rx_filter.match            = PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY | PACKET_RX_FILTER_MATCH_IP_ANY;
#else
    /* As on caracal platforms the acl flag is not set when the DUT receives a unicast bootp offer pkt from
       server to relay, the helper should not filter packages where acl flag is not set.*/
    DHCP_HELPER_rx_filter.match            = (PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_IP_PROTO | PACKET_RX_FILTER_MATCH_UDP_DST_PORT);
    DHCP_HELPER_rx_filter.etype            = ETYPE_IPV4;
    DHCP_HELPER_rx_filter.ip_proto         = IP_PROTO_UDP;
    DHCP_HELPER_rx_filter.udp_dst_port_min = DHCP_SERVER_UDP_PORT;
    DHCP_HELPER_rx_filter.udp_dst_port_max = DHCP_CLIENT_UDP_PORT;
#endif /* DHCP_HELPER_HW_SWITCHING */
    DHCP_HELPER_rx_filter.prio             = PACKET_RX_FILTER_PRIO_NORMAL;
    DHCP_HELPER_rx_filter.cb               = DHCP_HELPER_rx_packet_callback;

    if (registerd && !DHCP_HELPER_filter_id) {
        if (packet_rx_filter_register(&DHCP_HELPER_rx_filter, &DHCP_HELPER_filter_id) != VTSS_RC_OK) {
            T_W("DHCP helper module register packet RX filter fail./n");
        }
    } else if (!registerd && DHCP_HELPER_filter_id) {
        if (packet_rx_filter_unregister(DHCP_HELPER_filter_id) == VTSS_RC_OK) {
            DHCP_HELPER_filter_id = NULL;
            DHCP_HELPER_BIP_CRIT_ENTER();
            vtss_bip_buffer_clear(&DHCP_HELPER_bip_buf);
            DHCP_HELPER_BIP_CRIT_EXIT();
        }
    }

    DHCP_HELPER_CRIT_EXIT();
}


/****************************************************************************/
/*  Receive register functions                                              */
/****************************************************************************/

/* Register DHCP user frame receive */
void dhcp_helper_user_receive_register(dhcp_helper_user_t user, dhcp_helper_stack_rx_callback_t cb)
{
    BOOL update_ace = FALSE;

    if (user >= DHCP_HELPER_USER_CNT) {
        return;
    }

    DHCP_HELPER_CRIT_ENTER();
    if (!DHCP_HELPER_rx_cb[user]) {
        update_ace = TRUE;
        DHCP_HELPER_rx_cb[user] = cb;
    }
    DHCP_HELPER_CRIT_EXIT();

    if (update_ace) {
        DHCP_HELPER_stack_ace_conf_set(VTSS_ISID_GLOBAL);
    }
}

/* Unregister DHCP user frame receive */
void dhcp_helper_user_receive_unregister(dhcp_helper_user_t user)
{
    BOOL update_ace = FALSE;

    if (user >= DHCP_HELPER_USER_CNT) {
        return;
    }

    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_rx_cb[user]) {
        update_ace = TRUE;
        DHCP_HELPER_rx_cb[user] = NULL;
    }
    DHCP_HELPER_CRIT_EXIT();

    if (update_ace) {
        DHCP_HELPER_stack_ace_conf_set(VTSS_ISID_GLOBAL);
    }
}


/****************************************************************************/
/*  Clear local statistics register functions                                              */
/****************************************************************************/
/* Register DHCP user clear local statistics */
#if defined(VTSS_SW_OPTION_DHCP_SERVER) || defined(VTSS_SW_OPTION_DHCP_RELAY)
void dhcp_helper_user_clear_local_stat_register(dhcp_helper_user_t user, dhcp_helper_user_clear_local_stat_callback_t cb)
{
    if (user >= DHCP_HELPER_USER_CNT) {
        return;
    }
    DHCP_HELPER_CRIT_ENTER();
    if (!DHCP_HELPER_clear_local_stat_cb[user]) {
        DHCP_HELPER_clear_local_stat_cb[user] = cb;
    }
    DHCP_HELPER_CRIT_EXIT();
}
#endif /* VTSS_SW_OPTION_DHCP_SERVER || VTSS_SW_OPTION_DHCP_RELAY */

/****************************************************************************/
/*  DHCP helper transmit functions                                          */
/****************************************************************************/

/* Alloc memory for transmit DHCP frame */
void *dhcp_helper_alloc_xmit(size_t len, vtss_isid_t isid, void **pbufref)
{
    void *p = NULL;

#if 1 /* Bugzilla#15786 - memory is used out */
    dhcp_helper_msg_t   *msg;

    DHCP_HELPER_CRIT_ENTER();

    if (DHCP_HELPER_msg_tx_pkt_cnt > DHCP_HELPER_TX_CNT_MAX) {
        DHCP_HELPER_CRIT_EXIT();
        T_D("exit: msg tx count is full");
        return NULL;
    }

    msg = (dhcp_helper_msg_t *)VTSS_MALLOC(sizeof(dhcp_helper_msg_t) + len);

    if (msg) {
        // increase Tx cnt
        ++DHCP_HELPER_msg_tx_pkt_cnt;

        T_D("Allocate: DHCP_HELPER_msg_tx_pkt_cnt = %u", DHCP_HELPER_msg_tx_pkt_cnt);
    }

    DHCP_HELPER_CRIT_EXIT();
#else
    dhcp_helper_msg_t *msg = (dhcp_helper_msg_t *)VTSS_MALLOC(sizeof(dhcp_helper_msg_t) + len);
#endif

    if (msg) {
        msg->msg_id = DHCP_HELPER_MSG_ID_FRAME_TX_REQ;
        msg->data.tx_req.len = len;
        msg->data.tx_req.isid = isid;
        *pbufref = (void *)msg; /* Remote op */
        p = ((u8 *)msg) + sizeof(*msg);
        memset(&msg->tagged_info, 0, sizeof(msg->tagged_info));
        msg->data.tx_req.count_statistic_only = FALSE;
    } else {
        T_E("Allocation failure, length %zd", len);
    }

    T_D("%s(%zd) ret %p", __FUNCTION__, len, p);
    return p;
}

/* Transmit DHCP frame.

   Input parameter of "frame_p" must cotains DA + SA + ETYPE + UDP header + UDP payload.
   Calling API dhcp_helper_alloc_xmit() to allocate the resource for "frame_p" and get the "bufref_p".

   Input parameters of "dst_isid", "dst_port_mask", "src_port_no" and "src_glag_no"
   will be ingored when user is "DHCP_HELPER_USER_SERVER" or "DHCP_HELPER_USER_CLIENT".

   Return 0  : Success
   Return -1 : Fail */
int dhcp_helper_xmit(dhcp_helper_user_t user,
                     const void         *frame_p,
                     size_t             len,
                     mesa_vid_t         vid,
                     vtss_isid_t        dst_isid,
                     u64                dst_port_mask,
                     BOOL               no_filter_dest,
                     vtss_isid_t        src_isid,
                     mesa_port_no_t     src_port_no,
                     mesa_glag_no_t     src_glag_no,
                     void               *bufref_p)
{
    int                     rc = -1;
    dhcp_helper_bip_buf_t   *bip_buf;

    T_D("enter: %s(frame_p = %p, len = %zd, dst_isid = %d, vid = %d, src_port_no = %d, src_glag_no = %d)", __FUNCTION__, frame_p, len, dst_isid, vid, src_port_no, src_glag_no);

    if (!msg_switch_is_primary()) {
        // Avoid pushing on the BIP buffer if we're not the primary switch.
        goto do_exit_with_error;
    }

    /* Check input parameters. */
    // Check NULL pointer
    if (!frame_p) {
        T_E("TX frame pointer is NULL");
        goto do_exit_with_error;
    }

    // Check VID
    if ((vid < VTSS_APPL_VLAN_ID_MIN) || (vid > VTSS_APPL_VLAN_ID_MAX)) {
        T_E("Invalid VID (%u)", vid);
        goto do_exit_with_error;
    }

    // Check length
    if (len > sizeof(bip_buf->pkt)) {
        T_E("Unable to transmit more than %zd bytes (requested %zd)", sizeof(bip_buf->pkt), len);
        goto do_exit_with_error;
    }

    // Check isid
    if (user == DHCP_HELPER_USER_SERVER) {
        /* Initialize default value */
        dst_isid = VTSS_ISID_GLOBAL;
        dst_port_mask = 0;
        src_port_no = VTSS_PORT_NO_NONE;
        src_glag_no = VTSS_GLAG_NO_NONE;
    }

    if (user == DHCP_HELPER_USER_CLIENT) {
        /* Initialize default value */
        dst_isid = VTSS_ISID_GLOBAL;
        dst_port_mask = 0;
        src_port_no = VTSS_PORT_NO_NONE;
        src_glag_no = VTSS_GLAG_NO_NONE;
    }

    if (dst_isid != VTSS_ISID_GLOBAL && !msg_switch_exists(dst_isid)) {
        T_W("ISID %u doesn't exist", dst_isid);
        goto do_exit_with_error;
    }

    // At this point, the original implementation of this function
    // could no longer return with error (-1), so it's OK to make
    // the remainder an asynchronous operation.
    // The reason for making the remainder asynchronous is to
    // be able to return to the current caller so that he can
    // release his mutex, which may be asked for by another module
    // that awaits for it in the message Rx thread. In one circumstance,
    // we also need the message Rx thread, and that's when transmitting
    // to a unicast DMAC. In that case DHCP_HELPER_source_port_lookup()
    // calls mac_mgmt_table_get_next(), which attempts to perform
    // message request/response operations, that will not succeed,
    // because the message Rx thread is occupied.
    // See Bugzilla#13886 for details.

    DHCP_HELPER_BIP_CRIT_ENTER();
    bip_buf = (dhcp_helper_bip_buf_t *)vtss_bip_buffer_reserve(&DHCP_HELPER_bip_buf, DHCP_HELPER_BIP_BUF_ALIGNED_SIZE);

    if (bip_buf == NULL) {
        DHCP_HELPER_BIP_CRIT_EXIT();
        T_D("BIP-buffer ran out of entries");
        goto do_exit_with_error;
    }

    memcpy(bip_buf->pkt, frame_p, len);
    bip_buf->len           = len;
    bip_buf->is_rx         = FALSE;
    bip_buf->vid           = vid;
    bip_buf->src_isid      = src_isid;
    bip_buf->src_port_no   = src_port_no;
    bip_buf->src_glag_no   = src_glag_no;
    bip_buf->dst_isid      = dst_isid;
    bip_buf->dst_port_mask = dst_port_mask;
    bip_buf->no_filter_dest = no_filter_dest;
    bip_buf->user          = user;

    T_D("bip_buf = %s", *bip_buf);

    vtss_bip_buffer_commit(&DHCP_HELPER_bip_buf);
    vtss_flag_setbits(&DHCP_HELPER_bip_buffer_thread_events, DHCP_HELPER_EVENT_PKT_TX);
    DHCP_HELPER_BIP_CRIT_EXIT();

    rc = 0;

do_exit_with_error:
    if (bufref_p) {
        VTSS_FREE(bufref_p);

#if 1 /* Bugzilla#15786 - memory is used out */
        // decrease Tx cnt
        DHCP_HELPER_CRIT_ENTER();
        --DHCP_HELPER_msg_tx_pkt_cnt;
        T_D("Free: DHCP_HELPER_msg_tx_pkt_cnt = %u", DHCP_HELPER_msg_tx_pkt_cnt);
        DHCP_HELPER_CRIT_EXIT();
#endif
    }

    return rc;
}

/****************************************************************************/
/*  Statistics functions                                                    */
/****************************************************************************/

/* Wait for reply to request */
static BOOL DHCP_HELPER_req_counter_timeout(dhcp_helper_user_t user,
                                            vtss_isid_t isid,
                                            mesa_port_no_t port_no,
                                            dhcp_helper_msg_id_t msg_id,
                                            vtss_mtimer_t *timer,
                                            vtss_flag_t *flags)
{
    dhcp_helper_msg_t       *msg;
    BOOL                    timeout;
    vtss_flag_value_t        flag;
    vtss_tick_count_t        time_tick;
    dhcp_helper_msg_buf_t   buf;

    T_D("enter, isid: %d", isid);

    DHCP_HELPER_CRIT_ENTER();
    timeout = VTSS_MTIMER_TIMEOUT(timer);
    DHCP_HELPER_CRIT_EXIT();

    if (timeout) {
        T_D("info old, sending GET_REQ(isid=%d)", isid);
        msg = DHCP_HELPER_msg_alloc(&buf, msg_id, TRUE);
        msg->data.tx_counters_get.user = user;
        msg->data.tx_counters_get.port_no = port_no;
        flag = (1 << isid);
        vtss_flag_maskbits(flags, ~flag);
        DHCP_HELPER_msg_tx(&buf, isid, sizeof(msg->data.tx_counters_get));
        time_tick = vtss_current_time() + VTSS_OS_MSEC2TICK(DHCP_HELPER_REQ_TIMEOUT * 1000);
        return (vtss_flag_timed_wait(flags, flag, VTSS_FLAG_WAITMODE_OR, time_tick) & flag ? 0 : 1);
    }
    return FALSE;
}

/*lint -esym(459,DHCP_HELPER_global) */ //Avoid Lint detecte an unprotected access to variable 'DHCP_HELPER_global.stats_flags'
/* Get DHCP helper statistics */
mesa_rc dhcp_helper_stats_get(dhcp_helper_user_t user, vtss_isid_t isid, mesa_port_no_t port_no, dhcp_helper_stats_t *stats)
{
    dhcp_helper_user_t user_idx, user_idx_start, user_idx_end;

    T_D("enter, isid: %d, port_no: %u", isid, port_no);

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return DHCP_HELPER_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return DHCP_HELPER_ERROR_ISID_NON_EXISTING;
    } else if (!msg_switch_exists(isid)) {
        memset(stats, 0, sizeof(*stats));
        return VTSS_RC_OK;
    }

    if (stats == NULL || port_no >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) || user > DHCP_HELPER_USER_CNT) {
        return DHCP_HELPER_ERROR_INV_PARAM;
    }

    if (user == DHCP_HELPER_USER_CNT) {
        user_idx_start = DHCP_HELPER_USER_HELPER;
        user_idx_end = (dhcp_helper_user_t)(DHCP_HELPER_USER_CNT - 1);
    } else {
        user_idx_start = user_idx_end = user;
    }

    memset(stats, 0, sizeof(*stats));
    for (user_idx = user_idx_start; user_idx <= user_idx_end; user_idx++) {
        if (!msg_switch_is_local(isid) &&   // speed up the process if the "isid" is local switch
            DHCP_HELPER_req_counter_timeout(user_idx,
                                            isid,
                                            port_no,
                                            DHCP_HELPER_MSG_ID_COUNTERS_GET_REQ,
                                            &DHCP_HELPER_global.stats_timer[isid],
                                            &DHCP_HELPER_global.stats_flags)) {
            T_W("timeout, DHCP_HELPER_COUNTERS_GET_REQ");
            return DHCP_HELPER_ERROR_REQ_TIMEOUT;
        }

        if (user != DHCP_HELPER_USER_CNT) {
            DHCP_HELPER_CRIT_ENTER();
            *stats = DHCP_HELPER_global.stats[user][isid][port_no];
            DHCP_HELPER_CRIT_EXIT();
            break;
        }

        DHCP_HELPER_CRIT_ENTER();
        stats->rx_stats.discover_rx           += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.discover_rx;
        stats->rx_stats.offer_rx              += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.offer_rx;
        stats->rx_stats.request_rx            += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.request_rx;
        stats->rx_stats.decline_rx            += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.decline_rx;
        stats->rx_stats.ack_rx                += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.ack_rx;
        stats->rx_stats.nak_rx                += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.nak_rx;
        stats->rx_stats.release_rx            += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.release_rx;
        stats->rx_stats.inform_rx             += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.inform_rx;
        stats->rx_stats.leasequery_rx         += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.leasequery_rx;
        stats->rx_stats.leaseunassigned_rx    += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.leaseunassigned_rx;
        stats->rx_stats.leaseunknown_rx       += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.leaseunknown_rx;
        stats->rx_stats.leaseactive_rx        += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.leaseactive_rx;
        stats->rx_stats.discard_untrust_rx    += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.discard_untrust_rx;
        stats->rx_stats.discard_chksum_err_rx += DHCP_HELPER_global.stats[user_idx][isid][port_no].rx_stats.discard_chksum_err_rx;
        stats->tx_stats.discover_tx           += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.discover_tx;
        stats->tx_stats.offer_tx              += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.offer_tx;
        stats->tx_stats.request_tx            += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.request_tx;
        stats->tx_stats.decline_tx            += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.decline_tx;
        stats->tx_stats.ack_tx                += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.ack_tx;
        stats->tx_stats.nak_tx                += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.nak_tx;
        stats->tx_stats.release_tx            += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.release_tx;
        stats->tx_stats.inform_tx             += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.inform_tx;
        stats->tx_stats.leasequery_tx         += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.leasequery_tx;
        stats->tx_stats.leaseunassigned_tx    += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.leaseunassigned_tx;
        stats->tx_stats.leaseunknown_tx       += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.leaseunknown_tx;
        stats->tx_stats.leaseactive_tx        += DHCP_HELPER_global.stats[user_idx][isid][port_no].tx_stats.leaseactive_tx;
        DHCP_HELPER_CRIT_EXIT();
    }

    T_D("exit, isid: %d, port_no: %u", isid, port_no);
    return VTSS_RC_OK;
}

/* Clear DHCP helper statistics */
mesa_rc dhcp_helper_stats_clear(dhcp_helper_user_t user, vtss_isid_t isid, mesa_port_no_t port_no)
{
    dhcp_helper_msg_t       *msg;
    dhcp_helper_msg_buf_t   buf;

    T_D("enter, isid: %d, port_no: %u", isid, port_no);

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return DHCP_HELPER_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return DHCP_HELPER_ERROR_ISID_NON_EXISTING;
    } else if (!msg_switch_exists(isid)) {
        return VTSS_RC_OK;
    }

    if (port_no >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) || user > DHCP_HELPER_USER_CNT) {
        T_W("illegal port_no: %u", port_no);
        return DHCP_HELPER_ERROR_INV_PARAM;
    }

    DHCP_HELPER_CRIT_ENTER();
    if (user == DHCP_HELPER_USER_CNT) {
        dhcp_helper_user_t user_idx;
        for (user_idx = DHCP_HELPER_USER_HELPER; user_idx < DHCP_HELPER_USER_CNT; user_idx++) {
            memset(&DHCP_HELPER_global.stats[user_idx][isid][port_no], 0, sizeof(dhcp_helper_stats_t));
        }
    } else {
        memset(&DHCP_HELPER_global.stats[user][isid][port_no], 0, sizeof(dhcp_helper_stats_t));
    }
    DHCP_HELPER_CRIT_EXIT();

    // Clear TX counter on secondary switches
    if (!msg_switch_is_local(isid)) {
        msg = DHCP_HELPER_msg_alloc(&buf, DHCP_HELPER_MSG_ID_COUNTERS_CLR, TRUE);
        msg->data.counters_clear.user = user;
        msg->data.counters_clear.port_no = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
        DHCP_HELPER_msg_tx(&buf, isid, sizeof(msg->data.counters_clear));
    }

    T_D("exit, isid: %d, port_no: %u", isid, port_no);

    return VTSS_RC_OK;
}

/* Clear DHCP helper statistics by user */
mesa_rc dhcp_helper_stats_clear_by_user(dhcp_helper_user_t user)
{
    dhcp_helper_msg_t       *msg;
    dhcp_helper_msg_buf_t   buf;
    switch_iter_t           sit;

    T_D("enter, user: %d", user);

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return DHCP_HELPER_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (user > DHCP_HELPER_USER_CNT) {
        return DHCP_HELPER_ERROR_INV_PARAM;
    }

    DHCP_HELPER_CRIT_ENTER();
    if (user == DHCP_HELPER_USER_CNT) {
        dhcp_helper_user_t user_idx;
        for (user_idx = DHCP_HELPER_USER_HELPER; user_idx < DHCP_HELPER_USER_CNT; user_idx++) {
            for (size_t i = 0; i < DHCP_HELPER_global.stats[user_idx].size(); ++i) {
                for (size_t j = 0; j < DHCP_HELPER_global.stats[user_idx][i].size(); ++j) {
                    vtss_clear(DHCP_HELPER_global.stats[user_idx][i][j]);
                }
            }
        }
    } else {
        for (size_t i = 0; i < DHCP_HELPER_global.stats[user].size(); ++i) {
            for (size_t j = 0; j < DHCP_HELPER_global.stats[user][i].size(); ++j) {
                vtss_clear(DHCP_HELPER_global.stats[user][i][j]);
            }
        }
    }
    DHCP_HELPER_CRIT_EXIT();

    // Clear TX counter on secondary switches
    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        msg = DHCP_HELPER_msg_alloc(&buf, DHCP_HELPER_MSG_ID_COUNTERS_CLR, TRUE);
        msg->data.counters_clear.user = user;
        msg->data.counters_clear.port_no = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
        DHCP_HELPER_msg_tx(&buf, sit.isid, sizeof(msg->data.counters_clear));
    }

    /* Synchronize the global overview statistics */
    if (user != DHCP_HELPER_USER_CNT) {
        DHCP_HELPER_CRIT_ENTER();
        if (DHCP_HELPER_clear_local_stat_cb[user]) {
            DHCP_HELPER_clear_local_stat_cb[user]();
        }
        DHCP_HELPER_CRIT_EXIT();
    }

    T_D("exit");

    return VTSS_RC_OK;
}

