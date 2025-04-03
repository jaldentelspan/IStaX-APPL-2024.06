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

#include "main.h"
#include "conf_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "vtss_upnp_api.h"
#include "vtss_upnp.h"
#include "upnp_device.h"
#include "acl_api.h"
#include "ip_api.h"
#include "vtss_os_wrapper.h"
#include <linux/ip.h>
#include <pthread.h>
#include <netinet/udp.h>
#include <linux/if.h> /* For IFNAMSIZ (defined as 16 including terminating '\0') */

#ifndef IP_VHL_HL
#define IP_VHL_HL(vhl)      ((vhl) & 0x0f)
#endif /* IP_VHL_HL */

#ifdef VTSS_SW_OPTION_ICFG
#include "upnp_icfg.h"
#endif
#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_UPNP

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/
/* Global structure */
static upnp_global_t upnp_global;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "upnp", "UPNP"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define UPNP_CRIT_ENTER() critd_enter(&upnp_global.crit, __FILE__, __LINE__)
#define UPNP_CRIT_EXIT()  critd_exit( &upnp_global.crit, __FILE__, __LINE__)

/* Thread variables */
#define UPNP_CERT_THREAD_NO                 15
static vtss_handle_t upnp_thread_handle;
static vtss_thread_t upnp_thread_block;

/* packet rx filter */
#ifdef VTSS_SW_OPTION_PACKET
static packet_rx_filter_t upnp_rx_ssdp_filter;
static packet_rx_filter_t upnp_rx_igmp_filter;
static void              *upnp_ssdp_filter_id = NULL; // Filter id for subscribing upnp packet.
static void              *upnp_igmp_filter_id = NULL; // Filter id for subscribing upnp packet.
#endif /* VTSS_SW_OPTION_PACKET */

/* RX loopback on primary switch */
static vtss_isid_t primary_switch_isid = VTSS_ISID_LOCAL;

/* ================================================================= *
 *  UPNP event definitions
 * ================================================================= */
static vtss_flag_t           UPnP_Thread_Event;

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/
#define upnp_device_start(a, b, c, d, e) base_upnp_device_start(a, b, c, d, e)
static mesa_rc vtss_upnp_get_ip(vtss_appl_upnp_param_t conf, char *ipaddr, mesa_vid_t *vid);
static void vtss_upnp_get_udnstr(char *buffer);

/* Determine if UPNP configuration has changed */
static BOOL upnp_conf_changed
(vtss_appl_upnp_param_t *old, vtss_appl_upnp_param_t *new_, vtss_appl_upnp_capabilities_t cap)
{
    if (new_->mode != old->mode || (cap.support_ttl_write && (new_->ttl != old->ttl)) ||
        new_->ip_addressing_mode != old->ip_addressing_mode || new_->adv_interval != old->adv_interval) {
        return TRUE;
    } else if ((VTSS_APPL_UPNP_IPADDRESSING_MODE_STATIC == new_->ip_addressing_mode) && (new_->static_ifindex != old->static_ifindex)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* Set UPNP defaults */
void upnp_default_get(vtss_appl_upnp_param_t *conf)
{
    T_D("Enter");

    conf->mode = UPNP_MGMT_DISABLED;
    conf->ttl = UPNP_MGMT_DEFAULT_TTL;
    conf->adv_interval = UPNP_MGMT_DEFAULT_INT;
    conf->ip_addressing_mode = VTSS_APPL_UPNP_IPADDRESSING_MODE_DYNAMIC;
    conf->static_ifindex = vtss_ifindex_cast_from_u32(UPNP_MGMT_DEF_VLAN_ID, VTSS_IFINDEX_TYPE_VLAN);
}

/* Allocate request buffer */
static upnp_msg_req_t *
upnp_alloc_pkt_message(size_t size, upnp_msg_id_t msg_id)
{
    upnp_msg_req_t *msg = (upnp_msg_req_t *)VTSS_MALLOC(size);
    if (msg) {
        msg->msg_id = msg_id;
    }
    T_D("msg len %zu, type %d => %p", size, msg_id, msg);
    return msg;
}

/****************************************************************************/
/*  Callback functions                                                      */
/****************************************************************************/

static void
upnp_do_rx_callback(const void *packet,
                    size_t len,
                    ulong vid,
                    ulong isid,
                    ulong port_no)
{
    T_D("enter, RX isid %ld vid %ld port %ld len %zu", isid, vid, port_no, len);
    T_D("Now vtss_ip_if_inject is not available and will not be supported under linux!");
    T_D("exit");
}

/****************************************************************************/
/*  Reserved ACEs functions                                                 */
/****************************************************************************/
/* Add reserved ACE */
static mesa_rc upnp_ace_add(void)
{
    acl_entry_conf_t conf;
    mesa_rc          rc = VTSS_RC_OK;

    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_IPV4, &conf)) != VTSS_RC_OK) {
        T_D("acl_mgmt_ace_init(MESA_ACE_TYPE_IPV4) fail!");
        return rc;
    }
    conf.id = UPNP_SSDP_ACE_ID;
    conf.action.force_cpu = TRUE;
    conf.action.cpu_once = FALSE;
    conf.isid = VTSS_ISID_LOCAL;
    conf.frame.ipv4.proto.value = 17; //UDP
    conf.frame.ipv4.proto.mask = 0xFF;
    conf.frame.ipv4.sport.in_range = conf.frame.ipv4.dport.in_range = 1;
    conf.frame.ipv4.sport.high = 65535;
    conf.frame.ipv4.dport.low = conf.frame.ipv4.dport.high = UPNP_UDP_PORT;

    rc = acl_mgmt_ace_add(ACL_USER_UPNP, ACL_MGMT_ACE_ID_NONE, &conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_IPV4, &conf)) != VTSS_RC_OK) {
        T_D("acl_mgmt_ace_init(MESA_ACE_TYPE_IPV4) fail!");
        if ((rc = acl_mgmt_ace_del(ACL_USER_UPNP, UPNP_SSDP_ACE_ID)) != VTSS_RC_OK) {
            T_D("acl_mgmt_ace_del(UPNP_SSDP_ACE_ID) fail!");
        }
        return rc;
    }
    conf.id = IGMP_QUERY_ACE_ID;
    conf.action.force_cpu = TRUE;
    conf.action.cpu_once = FALSE;
    conf.isid = VTSS_ISID_LOCAL;
    conf.frame.ipv4.dip.value = 0xe0000001; /* 224.0.0.1 */
    conf.frame.ipv4.dip.mask = 0xffffffff; /* 255.255.255.255 */
    conf.frame.ipv4.sport.in_range = conf.frame.ipv4.dport.in_range = TRUE;
    conf.frame.ipv4.sport.high = conf.frame.ipv4.dport.high = 65535;

    if (acl_mgmt_ace_add(ACL_USER_UPNP, ACL_MGMT_ACE_ID_NONE, &conf)) {
        T_D("acl_mgmt_ace_add(IGMP_QUERY_ACE_ID) fail!");
        if ((rc = acl_mgmt_ace_del(ACL_USER_UPNP, UPNP_SSDP_ACE_ID)) != VTSS_RC_OK) {
            T_D("acl_mgmt_ace_del(UPNP_SSDP_ACE_ID) fail!");
        }
        return rc;
    }

    return rc;
}

/* Delete reserved ACE */
static mesa_rc upnp_ace_del(void)
{
    mesa_rc rc = VTSS_RC_OK;

    rc = acl_mgmt_ace_del(ACL_USER_UPNP, UPNP_SSDP_ACE_ID);
    if (rc != VTSS_RC_OK) {
        T_D("acl_mgmt_ace_del(UPNP_SSDP_ACE_ID) fail!");
        return rc;
    }

    rc = acl_mgmt_ace_del(ACL_USER_UPNP, IGMP_QUERY_ACE_ID);
    if (rc != VTSS_RC_OK) {
        T_D("acl_mgmt_ace_del(IGMP_QUERY_ACE_ID) fail!");
        return rc;
    }

    return rc;
}

/****************************************************************************/
/*  UPNP receive functions                                           */
/****************************************************************************/

static void upnp_receive_indication(const void *packet,
                                    size_t len,
                                    mesa_port_no_t switchport,
                                    mesa_vid_t vid,
                                    mesa_glag_no_t glag_no)
{
    T_D("len %zu port %u vid %d glag %u", len, switchport, vid, glag_no);

    if (msg_switch_is_primary() && VTSS_ISID_LEGAL(primary_switch_isid)) {   /* Bypass message module! */
        upnp_do_rx_callback(packet, len, vid, primary_switch_isid, switchport);
    } else {
        size_t msg_len = sizeof(upnp_msg_req_t) + len;
        upnp_msg_req_t *msg = upnp_alloc_pkt_message(msg_len, UPNP_MSG_ID_FRAME_RX_IND);
        if (msg) {
            msg->req.rx_ind.len = len;
            msg->req.rx_ind.vid = vid;
            msg->req.rx_ind.port_no = switchport;
            memcpy(&msg[1], packet, len); /* Copy frame */
            msg_tx(VTSS_MODULE_ID_UPNP, 0, msg, msg_len);
        } else {
            T_D("Unable to allocate %zu bytes, tossing frame on port %u", msg_len, switchport);
        }
    }
}

/****************************************************************************/
/*  Rx filter register functions                                            */
/****************************************************************************/

/* Local port packet receive indication - forward through UPnP */
BOOL upnp_rx_packet_callback(void  *contxt, const uchar *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    uchar           *ptr = (uchar *)(frm);
    struct iphdr    *ip = (struct iphdr *)(ptr + 14);
    int             ip_header_len = IP_VHL_HL(ip->ihl);

    struct udphdr   *udp_header = (struct udphdr *)(ptr + 14 + ip_header_len); /* 14:DA+SA+ETYPE */
    ushort          dport = ntohs(udp_header->uh_dport);

    if (msg_switch_is_primary() && VTSS_ISID_LEGAL(primary_switch_isid)) {   /* Bypass message module! */
        /*  If this is the primary switch, let the packet go to IP layer naturally  */
        return FALSE;

    } else if ((ip->protocol == 17 && dport == UPNP_UDP_PORT) || ip->protocol == 2) {
        /* If this is a secondary switch, use the message to pack the packet and then transmit to the primary switch */
        T_D("enter, port_no: %u len %d vid %d", rx_info->port_no, rx_info->length, rx_info->tag.vid);

        // NB: Null out the GLAG (port is 1st in aggr)
        upnp_receive_indication(frm, rx_info->length, rx_info->port_no, rx_info->tag.vid,
                                (mesa_glag_no_t)(VTSS_GLAG_NO_START - 1));

        T_D("exit");

        return TRUE; // Do not allow other subscribers to receive the packet
    }

    return FALSE;
}

static mesa_rc upnp_rx_filter_register(BOOL registerd)
{
    mesa_rc ret = VTSS_RC_OK;
#ifdef VTSS_SW_OPTION_PACKET
    UPNP_CRIT_ENTER();

    if (!upnp_ssdp_filter_id) {
        packet_rx_filter_init(&upnp_rx_ssdp_filter);
    }
    if (!upnp_igmp_filter_id) {
        packet_rx_filter_init(&upnp_rx_igmp_filter);
    }

    upnp_rx_ssdp_filter.modid    = VTSS_MODULE_ID_UPNP;
    upnp_rx_ssdp_filter.match    = PACKET_RX_FILTER_MATCH_ACL | PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_IP_PROTO;
    upnp_rx_ssdp_filter.etype    = ETYPE_IPV4;
    upnp_rx_ssdp_filter.ip_proto = IP_PROTO_UDP;
    upnp_rx_ssdp_filter.prio     = PACKET_RX_FILTER_PRIO_NORMAL;
    upnp_rx_ssdp_filter.cb       = upnp_rx_packet_callback;

    upnp_rx_igmp_filter.modid    = VTSS_MODULE_ID_UPNP;
    upnp_rx_igmp_filter.match    = PACKET_RX_FILTER_MATCH_ACL | PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_IP_PROTO;
    upnp_rx_igmp_filter.etype    = ETYPE_IPV4;
    upnp_rx_igmp_filter.ip_proto = 2; //IGMP
    upnp_rx_igmp_filter.prio     = PACKET_RX_FILTER_PRIO_NORMAL;
    upnp_rx_igmp_filter.cb       = upnp_rx_packet_callback;

    if (registerd && !upnp_ssdp_filter_id) {
        if (VTSS_RC_OK != (ret = packet_rx_filter_register(&upnp_rx_ssdp_filter, &upnp_ssdp_filter_id))) {
            T_D("UPNP module register packet RX filter fail./n");
            return ret;
        }
    } else if (!registerd && upnp_ssdp_filter_id) {
        if (packet_rx_filter_unregister(upnp_ssdp_filter_id) == VTSS_RC_OK) {
            upnp_ssdp_filter_id = NULL;
        }
    }

    if (registerd && !upnp_igmp_filter_id) {
        if (VTSS_RC_OK != (ret = packet_rx_filter_register(&upnp_rx_igmp_filter, &upnp_igmp_filter_id))) {
            T_D("UPNP module register packet RX filter fail./n");
            return ret;
        }
    } else if (!registerd && upnp_igmp_filter_id) {
        if (packet_rx_filter_unregister(upnp_igmp_filter_id) == VTSS_RC_OK) {
            upnp_igmp_filter_id = NULL;
        }
    }

    UPNP_CRIT_EXIT();
#endif /* VTSS_SW_OPTION_PACKET */
    return ret;
}

/****************************************************************************/
/*  Receive register functions                                              */
/****************************************************************************/

#define SSDP_MC_IPADDR    0xeffffffa /* 239.255.255.250 */
/* Register UPnP receive */
static mesa_rc upnp_receive_register(mesa_vid_t intf_vid)
{
    mesa_rc ret = VTSS_RC_OK;

    T_D("Enter");

    if (VTSS_RC_OK != (ret = upnp_rx_filter_register(TRUE))) {
        T_D("Calling upnp_rx_filter_register() failed.\n");
        return ret;
    }
    if (VTSS_RC_OK != (ret = upnp_ace_add())) {
        T_D("Calling upnp_ace_add() failed.\n");
        return ret;
    }

    return ret;
}

/* Unregister UPnP receive */
static void upnp_receive_unregister(mesa_vid_t intf_vid)
{
    T_D("Enter");

    if (VTSS_RC_OK != upnp_ace_del()) {
        T_D("Calling upnp_ace_del() failed.\n");
    }

    if (VTSS_RC_OK != upnp_rx_filter_register(FALSE)) {
        T_D("Calling upnp_rx_filter_register() failed.\n");
    }
}

/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
static const char *upnp_msg_id_txt(upnp_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
#if 0 /* SAM_TO_CHK: Wait for stacking mechanism ready */
    case UPNP_MSG_ID_UPNP_CONF_SET_REQ:
        txt = "UPNP_MSG_ID_UPNP_CONF_SET_REQ";
        break;
#endif
    case UPNP_MSG_ID_FRAME_RX_IND:
        txt = "UPNP_MSG_ID_FRAME_RX_IND";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}
#endif /* VTSS_TRACE_LVL_DEBUG */

#if 0 /* SAM_TO_CHK: Wait for stacking mechanism ready */
/* Allocate request buffer */
static upnp_msg_req_t *upnp_msg_req_alloc(upnp_msg_buf_t *buf, upnp_msg_id_t msg_id)
{
    upnp_msg_req_t *msg = &upnp_global.request.msg;

    buf->sem = &upnp_global.request.sem;
    buf->msg = msg;
    vtss_sem_wait(buf->sem);
    msg->msg_id = msg_id;
    return msg;
}
#endif

#if 0 /* SAM_TO_CHK: Wait for stacking mechanism ready */
static void upnp_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    upnp_msg_id_t msg_id = *(upnp_msg_id_t *)msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s", msg_id, upnp_msg_id_txt(msg_id));
    vtss_sem_post((vtss_sem_t *)contxt);
}
#endif

#if 0 /* SAM_TO_CHK: Wait for stacking mechanism ready */
static void upnp_msg_tx(upnp_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    upnp_msg_id_t msg_id = *(upnp_msg_id_t *)buf->msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s, len: %zu, isid: %d", msg_id, upnp_msg_id_txt(msg_id), len, isid);
    msg_tx_adv(buf->sem, upnp_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_MODULE_ID_UPNP, isid, buf->msg, len + MSG_TX_DATA_HDR_LEN(upnp_msg_req_t, req));
}
#endif

static BOOL upnp_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, vtss_isid_t isid)
{
    upnp_msg_id_t msg_id = *(upnp_msg_id_t *)rx_msg;
    upnp_msg_req_t *msg = (upnp_msg_req_t *)rx_msg;

    T_D("msg_id: %d, %s, len: %zu, isid: %u", msg_id, upnp_msg_id_txt(msg_id), len, isid);

    switch (msg_id) {
    case UPNP_MSG_ID_FRAME_RX_IND: {
        upnp_do_rx_callback(&msg[1], msg->req.rx_ind.len, msg->req.rx_ind.vid, isid, msg->req.rx_ind.port_no);
        break;
    }
    default:
        T_D("unknown message ID: %d", msg_id);
        break;
    }
    return TRUE;
}

static mesa_rc upnp_stack_register(void)
{
    msg_rx_filter_t filter;

    T_D("Enter");

    memset(&filter, 0, sizeof(filter));
    filter.cb = upnp_msg_rx;
    filter.modid = VTSS_MODULE_ID_UPNP;
    return msg_rx_filter_register(&filter);
}

#if 0 /* SAM_TO_CHK: Wait for stacking mechanism ready */
/* Set stack UPNP configuration */
static mesa_rc upnp_stack_upnp_conf_set(vtss_isid_t isid_add)
{
    upnp_msg_req_t  *msg;
    upnp_msg_buf_t  buf;
    vtss_isid_t     isid;

    T_D("Enter, isid_add: %d", isid_add);
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if ((isid_add != VTSS_ISID_GLOBAL && isid_add != isid) ||
            !msg_switch_exists(isid)) {
            continue;
        }
        UPNP_CRIT_ENTER();
        msg = upnp_msg_req_alloc(&buf, UPNP_MSG_ID_UPNP_CONF_SET_REQ);
        msg->req.conf_set.conf = upnp_global.upnp_conf;
        UPNP_CRIT_EXIT();
        upnp_msg_tx(&buf, isid, sizeof(msg->req.conf_set.conf));
    }

    T_D("Exit, isid_add: %d", isid_add);
    return VTSS_RC_OK;
}
#endif

static void upnp_device_stop(mesa_vid_t vid)
{
    T_D("Enter");
    /* Call libupnp API to free resource */
    base_upnp_device_stop();

    /* Delete ACL rule */
    upnp_receive_unregister(vid);
}

static void upnp_base_pthread(vtss_addrword_t data)
{
    conf_board_t board_info;
    char udn_str[100];
    vtss_appl_upnp_param_t conf;
    char ip_address[UPNP_MGMT_IPSTR_SIZE] = {0};
    vtss_flag_value_t e_ret = 0;
    int to_sec = UPNP_EVENT_WAIT_TO_LONG;
    BOOL valid_event;

    /* The thread waits for ICFG_LOADING_PRE instead of ICFG_LOADING_POST
       because the default waiting time of UPnP_Thread_Event is
       UPNP_EVENT_WAIT_TO_LONG, if we wait ICFG_LOADING_POST, some
       events seem to be missed, then the thread is almost stuck.
    */
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_PRE, VTSS_MODULE_ID_UPNP);
    vtss_upnp_get_udnstr(&udn_str[0]);
    conf_mgmt_board_get(&board_info);
    T_D("\t\nUDNstr = %s, board_id = %lu", udn_str, board_info.board_id);

    /* Initialize thread event flag */
    vtss_flag_init(&UPnP_Thread_Event);

    while (1) {
        e_ret = vtss_flag_timed_wait(&UPnP_Thread_Event, UPNP_EVENT_ANY, VTSS_FLAG_WAITMODE_OR_CLR,
                                     vtss_current_time() + VTSS_OS_MSEC2TICK(to_sec));

        if (!msg_switch_is_primary()) {
            continue;
        }

        /* Load DB */
        UPNP_CRIT_ENTER();
        conf = upnp_global.upnp_conf;
        memset(ip_address, 0, sizeof(ip_address));
        UPNP_CRIT_EXIT();

        T_D("UPnP Event: %d", e_ret);
        if (e_ret == 0) {
            T_D("UPnP Event: UPNP_EVENT_TIMEOUT");
            /* Check IP changed or not */
            (void)vtss_upnp_get_ip(conf, ip_address, &upnp_global.upnp_dev_vid_running);
            T_D("\n\tip_address = %s, upnp_dev_ip_running = %s", ip_address, upnp_global.upnp_dev_ip_running);
            if (!strcmp(ip_address, upnp_global.upnp_dev_ip_running)) {
                T_D("IP not changed!");
                continue;
            }
        } else {
            valid_event = FALSE;
            if (UPNP_EVENT_DB_CHANGE & e_ret) {
                valid_event = TRUE;
                T_D("UPnP Event: UPNP_EVENT_DB_CHANGE");
            }
            if (UPNP_EVENT_FIND_IP & e_ret) {
                valid_event = TRUE;
                T_D("UPnP Event: UPNP_EVENT_FIND_IP");
            }
            if (UPNP_EVENT_SET_ACL & e_ret) {
                valid_event = TRUE;
                T_D("UPnP Event: UPNP_EVENT_SET_ACL");
            }
            if (UPNP_EVENT_RESTART & e_ret) {
                valid_event = TRUE;
                T_D("UPnP Event: UPNP_EVENT_RESTART");
            }
            if (!valid_event) {
                T_D("Unknown UPnP Event: %d", e_ret);
                continue;
            }
        }

        /* Stop the libupnp first and delete ACL to ensure the process can work normally */
        upnp_device_stop(upnp_global.upnp_dev_vid_running);

        /* Init running variables */
        memset(upnp_global.upnp_dev_ip_running, 0, sizeof(upnp_global.upnp_dev_ip_running));
        upnp_global.upnp_dev_vid_running = UPNP_MGMT_DEF_VLAN_ID;
        e_ret = 0;

        if (UPNP_MGMT_ENABLED == conf.mode) {
            T_D("Mode enable");
            if (VTSS_RC_OK == vtss_upnp_get_ip(conf, ip_address, &upnp_global.upnp_dev_vid_running)) {
                if (VTSS_RC_OK == upnp_receive_register(upnp_global.upnp_dev_vid_running)) {
                    T_D("\n\tip_address = %s, adv_int = %d\n\tudn_str = %s, sn = %lu",
                        ip_address, conf.adv_interval, udn_str, board_info.board_id);
                    if (!upnp_device_start(ip_address, UPNP_UDP_PORT, conf.adv_interval, udn_str, board_info.board_id)) {
                        T_D(">>> UPnP device start");
                        strcpy(upnp_global.upnp_dev_ip_running, ip_address);
                        to_sec = UPNP_EVENT_WAIT_TO_DEF;
                        continue;
                    } else {
                        T_D("UPnP device start fail");
                        e_ret = UPNP_EVENT_RESTART;
                    }
                } else {
                    T_D("ACL set fail");
                    e_ret = UPNP_EVENT_SET_ACL;
                }
            } else {
                T_D("IP get fail");
                e_ret = UPNP_EVENT_FIND_IP;
            }
        } else {
            to_sec = UPNP_EVENT_WAIT_TO_LONG;
            T_D("Mode disable");
            continue;
        }

        T_D("events = %d", e_ret);
        if (e_ret) {
            VTSS_OS_MSLEEP(5000);
            /* Trigger UPnP thread check sequence again */
            vtss_flag_setbits(&UPnP_Thread_Event, e_ret);
        }
    }
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* UPNP error text */
const char *upnp_error_txt(mesa_rc rc)
{
    const char *txt;

    switch (rc) {
    case VTSS_APPL_UPNP_ERROR_GEN:
        txt = "UPNP generic error";
        break;
    case VTSS_APPL_UPNP_ERROR_MODE:
        txt = "Invalid UPNP mode";
        break;
    case VTSS_APPL_UPNP_ERROR_ADV_INT:
        txt = "Invalid UPNP advertisement duration";
        break;
    case VTSS_APPL_UPNP_ERROR_TTL:
        txt = "Invalid UPNP TTL";
        break;
    case VTSS_APPL_UPNP_ERROR_IP_ADDR_MODE:
        txt = "Invalid UPNP IP addressing mode";
        break;
    case VTSS_APPL_UPNP_ERROR_IP_INTF_VID:
        txt = "Invalid UPNP IP interface VLAN ID";
        break;
    case VTSS_APPL_UPNP_ERROR_STACK_STATE:
        txt = "UPNP stack state error";
        break;
    case VTSS_APPL_UPNP_ERROR_CAPABILITY_TTL:
        txt = "UPNP TTL isn't writable";
        break;
    default:
        txt = "UPNP unknown error";
        break;
    }
    return txt;
}

/* UPNP parameters check */
static vtss_appl_upnp_error_t upnp_params_check
(vtss_appl_upnp_param_t *conf, vtss_appl_upnp_capabilities_t cap)
{
    /* check UPnP Mode */
    if (UPNP_MGMT_ENABLED != conf->mode && UPNP_MGMT_DISABLED != conf->mode) {
        return VTSS_APPL_UPNP_ERROR_MODE;
    }

    /* check advertisement duration */
    if (UPNP_MGMT_MIN_INT > conf->adv_interval || UPNP_MGMT_MAX_INT < conf->adv_interval) {
        return VTSS_APPL_UPNP_ERROR_ADV_INT;
    }

    /* check ttl */
    if (cap.support_ttl_write && (UPNP_MGMT_MIN_TTL > conf->ttl || UPNP_MGMT_MAX_TTL < conf->ttl)) {
        return VTSS_APPL_UPNP_ERROR_TTL;
    } else if (!cap.support_ttl_write && conf->ttl != UPNP_MGMT_DEFAULT_TTL) {
        return VTSS_APPL_UPNP_ERROR_CAPABILITY_TTL;
    }

    /* check IP addressing mode */
    if (VTSS_APPL_UPNP_IPADDRESSING_MODE_DYNAMIC != conf->ip_addressing_mode
        && VTSS_APPL_UPNP_IPADDRESSING_MODE_STATIC != conf->ip_addressing_mode) {
        return VTSS_APPL_UPNP_ERROR_IP_ADDR_MODE;
    }

    /* check IP interface VLAN ID */
    if ((UPNP_MGMT_MAX_VLAN_ID < vtss_ifindex_cast_to_u32(conf->static_ifindex)) ||
        (UPNP_MGMT_MIN_VLAN_ID > vtss_ifindex_cast_to_u32(conf->static_ifindex))) {
        return VTSS_APPL_UPNP_ERROR_IP_INTF_VID;
    }

    return VTSS_APPL_UPNP_OK;
}

static mesa_rc vtss_upnp_get_ip(vtss_appl_upnp_param_t conf, char *ipaddr, mesa_vid_t *vid)
{
    mesa_vid_t               intf_vid;
    mesa_ipv4_t              intf_adr;
    u32                      ops_idx;
    u32                      ops_cnt;
    vtss_appl_ip_if_status_t *ops, ip_status[UPNP_IP_INTF_MAX_OPST];
    vtss_ifindex_t           prev_ifindex, ifindex;

    T_D("Enter");

    if (!ipaddr || !vid) {
        T_D("Error Exit");
        return VTSS_RC_ERROR;
    }

    intf_adr = 0;
    memset(ip_status, 0, sizeof(ip_status));

    if (VTSS_APPL_UPNP_IPADDRESSING_MODE_DYNAMIC == conf.ip_addressing_mode) {

        prev_ifindex = VTSS_IFINDEX_NONE;
        while (vtss_appl_ip_if_itr(&prev_ifindex, &ifindex, true /* only VLAN interfaces */) == VTSS_RC_OK) {
            prev_ifindex = ifindex;

            intf_vid = vtss_ifindex_as_vlan(ifindex);

            if (!UPNP_IP_INTF_OPST_GET(ifindex, ip_status, ops_cnt)) {
                continue;
            }

            T_D("\n\tintf_vid : %u, ops_cnt = %u", intf_vid, ops_cnt);

            if (!(ops_cnt > UPNP_IP_INTF_MAX_OPST)) {
                for (ops_idx = 0; ops_idx < ops_cnt; ops_idx++) {
                    ops = &ip_status[ops_idx];

                    if (ops->type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV4) {
                        intf_adr = UPNP_IP_INTF_OPST_ADR4(ops);
                        break;
                    }
                }

                if (intf_adr) {
                    break;
                }
            }
        }
    } else {
        intf_vid = vtss_ifindex_cast_to_u32(conf.static_ifindex);
        if (UPNP_IP_INTF_OPST_GET(conf.static_ifindex, ip_status, ops_cnt)) {
            T_D("\n\tintf_vid : %u, ops_cnt = %u", intf_vid, ops_cnt);
            if (!(ops_cnt > UPNP_IP_INTF_MAX_OPST)) {
                for (ops_idx = 0; ops_idx < ops_cnt; ops_idx++) {
                    ops = &ip_status[ops_idx];

                    if (ops->type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV4) {
                        intf_adr = UPNP_IP_INTF_OPST_ADR4(ops);
                        break;
                    }
                }
            }
        }
    }

    T_D("Done with VID-%u IPA-%u", intf_vid, intf_adr);

    if (intf_vid != VTSS_VID_NULL && intf_adr) {
        *vid = intf_vid;
        snprintf(ipaddr, IFNAMSIZ, "vtss.vlan.%d", *vid);
        T_D("EXIT: IP interface = %s / IntfVid = %u", ipaddr, *vid);
        return VTSS_RC_OK;
    } else {
        T_D("EXIT: No available IP interface");
        return VTSS_RC_ERROR;
    }
}

static void vtss_upnp_get_udnstr(char *buffer)
{
    static char udnstr[UPNP_MGMT_UDNSTR_SIZE];
    static int  init_flag = 0;
    uchar       mac[6];
    int         rc;

    if (!init_flag) {
        rc = conf_mgmt_mac_addr_get(mac, 0);
        if (rc  !=  0) {
            T_D("vtss_upnp_get_udnstr: conf_mgmt_mac_addr_get fails");
        }
        memset(udnstr, 0, UPNP_MGMT_UDNSTR_SIZE);

        (void)sprintf(udnstr,
                      "uuid:%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                      (unsigned int)mac[5],
                      mac[3],
                      mac[1],
                      mac[1],
                      mac[0],
                      mac[0],
                      mac[1],
                      mac[2],
                      mac[3],
                      mac[4],
                      mac[5]);
    }

    (void)strncpy(buffer, udnstr, UPNP_MGMT_UDNSTR_SIZE);
    return;
}

/****************************************************************************
 * Module thread
 ****************************************************************************/
/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#if 0 /* SAM_TO_CHK: Wait for stacking mechanism ready */
/* Read/create UPNP stack configuration */
static mesa_rc upnp_conf_read_stack(BOOL create)
{
    int             changed = 0;
    vtss_appl_upnp_param_t      *old_upnp_conf_p, new_upnp_conf;
    mesa_rc         rc;

    T_D("Enter, create: %d", create);
    /* Use default values */
    upnp_default_get(&new_upnp_conf);

    UPNP_CRIT_ENTER();
    old_upnp_conf_p = &upnp_global.upnp_conf;
    if (upnp_conf_changed(old_upnp_conf_p, &new_upnp_conf)) {
        changed = 1;
    }
    upnp_global.upnp_conf = new_upnp_conf;
    UPNP_CRIT_EXIT();

    if (changed) {
        rc = upnp_stack_upnp_conf_set(VTSS_ISID_GLOBAL);
        if (rc != VTSS_RC_OK) {
            T_W("upnp_conf_read_stack: upnp_stack_upnp_conf_set fails");
        }
    }

    T_D("exit");

    return VTSS_RC_OK;
}
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void upnp_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_upnp_json_init(void);
#endif
extern "C" int upnp_icli_cmd_register();

/* Initialize module */
mesa_rc upnp_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc     rc;
    vtss_appl_upnp_param_t *conf_p;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");

#ifdef VTSS_SW_OPTION_ICFG
        /* Initialize and register iCFG */
        if ((rc = upnp_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling upnp_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif
#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        /* Initialize and register Private Mib */
        upnp_mib_init();
#endif
#if defined(VTSS_SW_OPTION_JSON_RPC)
        /* Initialize JSON */
        vtss_appl_upnp_json_init();
#endif

        /* Initialize UPNP configuration */
        conf_p = &upnp_global.upnp_conf;
        upnp_default_get(conf_p);

        /* Initialize message buffers */
        vtss_sem_init(&upnp_global.request.sem, 1);

        /* Create semaphore for critical regions */
        critd_init(&upnp_global.crit, "upnp", VTSS_MODULE_ID_UPNP, CRITD_TYPE_MUTEX);

        /* Init running variables */
        memset(upnp_global.upnp_dev_ip_running, 0, sizeof(upnp_global.upnp_dev_ip_running));
        upnp_global.upnp_dev_vid_running = UPNP_MGMT_DEF_VLAN_ID;
        upnp_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");

        /* Register MSG module for stacking messages */
        if ((rc = upnp_stack_register()) != VTSS_RC_OK) {
            T_D("upnp_start: upnp_stack_register fails");
        }

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           upnp_base_pthread,
                           0,
                           "UPNP",
                           nullptr,
                           0,
                           &upnp_thread_handle,
                           &upnp_thread_block);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF - isid: %d", isid);

        if (isid == VTSS_ISID_GLOBAL) {
            /* Reload UPNP DB to default */
            conf_p = &upnp_global.upnp_conf;
            upnp_default_get(conf_p);

            /* Trigger UPnP thread reload DB */
            vtss_flag_setbits(&UPnP_Thread_Event, UPNP_EVENT_DB_CHANGE);
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the capabilities of UPnP.
 *
 * \param cap       [OUT]   The capability properties of the UPnP module.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_upnp_capabilities_get(
    vtss_appl_upnp_capabilities_t        *const cap
)
{
    T_D("enter");

    if (!cap) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(cap, 0x0, sizeof(vtss_appl_upnp_capabilities_t));
    cap->support_ttl_write = FALSE;

    T_D("exit");
    return VTSS_RC_OK;
}

/**
 * \brief Get UPnP system parameters
 *
 * To read current system parameters in UPnP.
 *
 * \param param [OUT] The UPnP system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_upnp_system_config_get(vtss_appl_upnp_param_t *const param)
{
    T_D("Enter");
    if ( NULL == param ) {
        T_D("param == NULL\n");
        return VTSS_RC_ERROR;
    }

    UPNP_CRIT_ENTER();
    *param = upnp_global.upnp_conf;
    UPNP_CRIT_EXIT();

    return VTSS_RC_OK;
}

/**
 * \brief Set UPnP system parameters
 *
 * To modify current system parameters in UPnP.
 *
 * \param param [IN] The UPnP system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_upnp_system_config_set(
    const vtss_appl_upnp_param_t *const param )
{
    vtss_appl_upnp_error_t rc = VTSS_APPL_UPNP_OK;
    vtss_appl_upnp_param_t conf = {0};
    vtss_appl_upnp_capabilities_t cap = {0};
    BOOL changed = FALSE;
    u8 ttl = UPNP_MGMT_DEFAULT_TTL;

    if ( NULL == param ) {
        T_D("param == NULL\n");
        return VTSS_RC_ERROR;
    }
    conf = *param;

    T_D("enter, mode: %d", conf.mode);

    (void)vtss_appl_upnp_capabilities_get(&cap);
    if (VTSS_APPL_UPNP_OK != (rc = upnp_params_check(&conf, cap))) {
        T_D("Error, rc = %d", rc);
        return rc;
    }

    UPNP_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        changed = upnp_conf_changed(&upnp_global.upnp_conf, &conf, cap);
        ttl = upnp_global.upnp_conf.ttl;
        upnp_global.upnp_conf = conf;

        if (!cap.support_ttl_write) {
            upnp_global.upnp_conf.ttl = ttl;
        }
    } else {
        T_D("not primary switch");
        rc = VTSS_APPL_UPNP_ERROR_STACK_STATE;
    }
    UPNP_CRIT_EXIT();

    if (changed) {
#if 1
        /* Trigger UPnP thread reload DB */
        vtss_flag_setbits(&UPnP_Thread_Event, UPNP_EVENT_DB_CHANGE);
#else /* SAM_TO_CHK: Wait for stacking mechanism ready */
        /* Activate changed configuration */
        rc = upnp_stack_upnp_conf_set(VTSS_ISID_GLOBAL);
        if (rc != VTSS_RC_OK) {
            T_W("upnp_mgmt_conf_set: upnp_stack_upnp_conf_set fails");

        }
#endif
    }

    T_D("exit");

    return VTSS_APPL_UPNP_OK != rc ? VTSS_RC_ERROR : VTSS_RC_OK;
}

