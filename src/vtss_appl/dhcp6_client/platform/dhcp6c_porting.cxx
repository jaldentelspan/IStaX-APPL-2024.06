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

#include "port_iter.hxx"
#if defined(VTSS_SW_OPTION_IP)
#include "ip_api.h"
#endif /* defined(VTSS_SW_OPTION_IP) */
#include "vtss_dhcp6_type.hxx"
#include "dhcp6c_porting.hxx"
#include "dhcp6c_conf.hxx"

/* ************************************************************************ **
 *
 * Defines
 *
 * ************************************************************************ */
#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_DHCP6C
#define VTSS_ALLOC_MODULE_ID        VTSS_MODULE_ID_DHCP6C

namespace vtss
{
namespace dhcp6c
{
#if !VTSS_DHCP6C_PKT_HELPER
static mesa_rc DHCP6C_egress_ip2mac_set(dhcp6c_ifidx_t ifidx, const mesa_ipv6_t *const ipa)
{
    mesa_vid_mac_t  vid_mac_entry;
    mesa_rc         rc;

    if ((rc = utils::convert_ip_to_mac(ipa, &vid_mac_entry.mac)) == VTSS_RC_OK) {
        port_iter_t             pit;
        mesa_mac_table_entry_t  mac_table_entry;

        vid_mac_entry.vid = ifidx;
        vtss_clear(mac_table_entry);
        if (mesa_mac_table_get(NULL, &vid_mac_entry, &mac_table_entry) != VTSS_RC_OK) {
            memcpy(&mac_table_entry.vid_mac, &vid_mac_entry, sizeof(mesa_vid_mac_t));
        }
        mac_table_entry.copy_to_cpu = FALSE;
        mac_table_entry.locked = TRUE;
        mac_table_entry.aged = FALSE;
        mac_table_entry.cpu_queue = PACKET_XTR_QU_MGMT_MAC;
        (void) port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
        while (port_iter_getnext(&pit)) {
            mac_table_entry.destination[pit.iport] = TRUE;
        }

        rc = mesa_mac_table_add(NULL, &mac_table_entry);
    }

    DHCP6C_PRIV_TD_RETURN(rc);
}

static mesa_rc DHCP6C_egress_ip2mac_clr(dhcp6c_ifidx_t ifidx, const mesa_ipv6_t *const ipa)
{
    mesa_vid_mac_t  vid_mac_entry;
    mesa_rc         rc;

    if ((rc = utils::convert_ip_to_mac(ipa, &vid_mac_entry.mac)) == VTSS_RC_OK) {
        mesa_mac_table_entry_t  mac_table_entry;

        vid_mac_entry.vid = ifidx;
        vtss_clear(mac_table_entry);
        if ((rc = mesa_mac_table_get(NULL, &vid_mac_entry, &mac_table_entry)) == VTSS_RC_OK) {
            rc = mesa_mac_table_del(NULL, &vid_mac_entry);
            /* should notify MLD to confirm registration of this vid_mac_entry */
        }
    }

    DHCP6C_PRIV_TD_RETURN(rc);
}
#endif /* !VTSS_DHCP6C_PKT_HELPER */

namespace porting
{

namespace pkt
{
/* Set/Unset DHCPv6 frame trapper */
mesa_rc rx_trapper_set(BOOL state)
{
    if (state) {
        /* Set the chip/hardware to trap DHCPv6 frames */
    } else {
        /* UnSet the chip/hardware to trap DHCPv6 frames */
    }
    /* Currently, we don't need to do anything since IP already did it  */

    return VTSS_RC_OK;
};

#if VTSS_DHCP6C_MY_PKT_HANDLER
struct dhcp6c_porting_pkt_t {
    void                            *dhcp6c_packet_filter_id;
    BOOL                            st;
    mesa_vid_t                      idx;
};
static CapArray<dhcp6c_porting_pkt_t, VTSS_APPL_CAP_IP_INTERFACE_CNT> pkt_filter;

static dhcp6c_porting_pkt_t *DHCP6C_pkt_filter_rtv(dhcp6c_ifidx_t idx)
{
    uint32_t                 i;
    dhcp6c_porting_pkt_t    *p = NULL;

    for (i = 0; i < DHCP6C_MAX_INTERFACES; ++i) {
        if (pkt_filter[i].st && pkt_filter[i].idx == idx) {
            p = &pkt_filter[i];
            break;
        }
    }

    return p;
}

static dhcp6c_porting_pkt_t *DHCP6C_pkt_filter_new(dhcp6c_ifidx_t idx)
{
    uint32_t                 i;
    dhcp6c_porting_pkt_t    *p = NULL;

    for (i = 0; i < DHCP6C_MAX_INTERFACES; ++i) {
        if (!pkt_filter[i].st) {
            p = &pkt_filter[i];
            p->st = TRUE;
            p->idx = idx;
            p->dhcp6c_packet_filter_id = NULL;
            break;
        }
    }

    return p;
}

static BOOL DHCP6C_pkt_filter_clr(dhcp6c_ifidx_t idx)
{
    dhcp6c_porting_pkt_t    *p = DHCP6C_pkt_filter_rtv(idx);

    if (p) {
        memset(p, 0x0, sizeof(dhcp6c_porting_pkt_t));
    } else {
        return FALSE;
    }

    return TRUE;
}

static BOOL DHCP6C_rx_callback(
    void                        *contxt,
    const u8                    *const frm,
    const mesa_packet_rx_info_t *const rx_info
)
{
    BOOL        absorb_pkt = FALSE;
    pkt_info_t  rinfo;

    if (!frm || !rx_info) {
        return FALSE;
    }

    rinfo.ifidx = rx_info->tag.vid;
    rinfo.len = rx_info->length - VTSS_IPV6_ETHER_LENGTH;
    rinfo.tstamp = vtss_current_time();

    absorb_pkt = queue_snd(frm + VTSS_IPV6_ETHER_LENGTH, &rinfo);

    return (absorb_pkt == TRUE);
}

static mesa_rc DHCP6C_rx_filter_unregister(dhcp6c_ifidx_t ifx)
{
    dhcp6c_porting_pkt_t    *fltr;
    mesa_rc                 rc = VTSS_RC_ERROR;

    if ((fltr = DHCP6C_pkt_filter_rtv(ifx)) == NULL) {
        DHCP6C_PRIV_TI_RETURN(rc);
    }

    if (fltr->dhcp6c_packet_filter_id != NULL &&
        (rc = packet_rx_filter_unregister(fltr->dhcp6c_packet_filter_id)) == VTSS_RC_OK) {
        if (!DHCP6C_pkt_filter_clr(ifx)) {
            rc = VTSS_RC_ERROR;
        }
    }

    if (rc != VTSS_RC_OK) {
        T_W("packet_rx_filter_unregister() failed");
    }

    DHCP6C_PRIV_TI_RETURN(rc);
}

static mesa_rc DHCP6C_rx_filter_register(dhcp6c_ifidx_t ifx)
{
    packet_rx_filter_t      packet_filter;
    dhcp6c_porting_pkt_t    *fltr;
    mesa_rc                 rc = VTSS_RC_ERROR;

    if ((fltr = DHCP6C_pkt_filter_rtv(ifx)) == NULL) {
        fltr = DHCP6C_pkt_filter_new(ifx);
    }
    if (fltr == NULL) {
        DHCP6C_PRIV_TI_RETURN(rc);
    }

    packet_rx_filter_init(&packet_filter);
    packet_filter.modid = VTSS_MODULE_ID_DHCP6C;
    packet_filter.match = PACKET_RX_FILTER_MATCH_IP_ANY | PACKET_RX_FILTER_MATCH_VID | PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY | PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_IP_PROTO | PACKET_RX_FILTER_MATCH_UDP_DST_PORT;
    packet_filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;

    packet_filter.vid = ifx;                                        /* VLAN */
    packet_filter.etype = ETYPE_IPV6;                               /* IPv6 */
    packet_filter.ip_proto = IP_PROTO_UDP;                          /* UDP */
    packet_filter.udp_dst_port_min = VTSS_DHCP6_CLIENT_UDP_PORT;    /* 546 */
    packet_filter.udp_dst_port_max = VTSS_DHCP6_CLIENT_UDP_PORT;    /* 546 */

    packet_filter.cb = DHCP6C_rx_callback;

    if (fltr->dhcp6c_packet_filter_id) {
        rc = packet_rx_filter_change(&packet_filter, &fltr->dhcp6c_packet_filter_id);
    } else {
        rc = packet_rx_filter_register(&packet_filter, &fltr->dhcp6c_packet_filter_id);
    }

    if (rc != VTSS_RC_OK) {
        T_W("packet_rx_filter_register() failed");
    }

    DHCP6C_PRIV_TI_RETURN(rc);
}

static mesa_rc DHCP6C_egress(const pkt_info_t *const info, const u8 *const frm)
{
    packet_tx_props_t   tx_props;
    u8                  *pkt_buf;
    mesa_rc             tx_status = VTSS_RC_ERROR;

    if (!info || !frm) {
        return tx_status;
    }

    if ((pkt_buf = packet_tx_alloc(info->len))) {
        packet_tx_props_init(&tx_props);
        tx_props.packet_info.modid  = VTSS_MODULE_ID_DHCP6C;
        tx_props.packet_info.frm    = pkt_buf;
        tx_props.packet_info.len    = info->len;
        tx_props.tx_info.switch_frm = TRUE;
        tx_props.tx_info.tag.vid    = info->ifidx;

        memcpy(pkt_buf, frm, info->len);

        tx_status = packet_tx(&tx_props);
    }

    DHCP6C_PRIV_TD_RETURN(tx_status);
}
#endif /* VTSS_DHCP6C_MY_PKT_HANDLER */

/* Start the DHCP6C RX Process */
mesa_rc rx_process_start(dhcp6c_ifidx_t ifx)
{
    mesa_rc rc = VTSS_RC_OK;

#if VTSS_DHCP6C_MY_PKT_HANDLER
    rc = DHCP6C_rx_filter_register(ifx);
#else
#if VTSS_DHCP6C_PKT_HELPER
    /* HELPER access */
#else
    /* SOCKET access */
#endif /* VTSS_DHCP6C_PKT_HELPER */
#endif /* VTSS_DHCP6C_MY_PKT_HANDLER */

    if (pkt::rx_trapper_set(TRUE) != VTSS_RC_OK) {
        T_D("rx_trapper_set() failed");
    }

    DHCP6C_PRIV_TI_RETURN(rc);
}

/* Stop the DHCP6C RX Process */
mesa_rc rx_process_stop(dhcp6c_ifidx_t ifx)
{
    mesa_rc rc = VTSS_RC_OK;

    if (pkt::rx_trapper_set(FALSE) != VTSS_RC_OK) {
        T_D("rx_trapper_set() failed");
    }

#if VTSS_DHCP6C_MY_PKT_HANDLER
    rc = DHCP6C_rx_filter_unregister(ifx);
#else
#if VTSS_DHCP6C_PKT_HELPER
    /* HELPER access */
#else
    /* SOCKET access */
#endif /* VTSS_DHCP6C_PKT_HELPER */
#endif /* VTSS_DHCP6C_MY_PKT_HANDLER */

    DHCP6C_PRIV_TI_RETURN(rc);
}

/* TX misc. */
mesa_rc tx_misc_config(dhcp6c_ifidx_t ifidx, BOOL state)
{
    mesa_rc rc = VTSS_RC_OK;

#if VTSS_DHCP6C_PKT_HELPER
    /* HELPER access */
#else
    if (state) {
        if ((rc = vtss::dhcp6c::DHCP6C_egress_ip2mac_set(ifidx, &dhcp6::dhcp6_linkscope_relay_agents_and_servers)) == VTSS_RC_OK) {
            rc = vtss::dhcp6c::DHCP6C_egress_ip2mac_set(ifidx, &dhcp6::dhcp6_sitescope_all_dhcp_servers);
        }
    } else {
        if ((rc = vtss::dhcp6c::DHCP6C_egress_ip2mac_clr(ifidx, &dhcp6::dhcp6_linkscope_relay_agents_and_servers)) == VTSS_RC_OK) {
            rc = vtss::dhcp6c::DHCP6C_egress_ip2mac_clr(ifidx, &dhcp6::dhcp6_sitescope_all_dhcp_servers);
        }
    }
#endif /* VTSS_DHCP6C_PKT_HELPER */

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc tx_process(const pkt_info_t *const info, const u8 *const frm)
{
    mesa_rc rc;

    if (!info || !frm) {
        return VTSS_RC_ERROR;
    }

    T_N("TimeStamp:" VPRI64u" / EgressVid:%u / Length:%u", info->tstamp, info->ifidx, info->len);
    T_N_HEX(frm, info->len);

    rc = VTSS_RC_OK;
#if VTSS_DHCP6C_MY_PKT_HANDLER
    rc = DHCP6C_egress(info, frm);
#else
#if VTSS_DHCP6C_PKT_HELPER
    /* HELPER access */
#else
    /* SOCKET access */
#endif /* VTSS_DHCP6C_PKT_HELPER */
#endif /* VTSS_DHCP6C_MY_PKT_HANDLER */

    DHCP6C_PRIV_TD_RETURN(rc);
}
}; /* pkt */

namespace os
{
/* Create thread */
void thread_create(
    vtss_thread_entry_f *entry,
    const char          *name,
    void                *stack_base,
    u32                 stack_size,
    vtss_handle_t       *handle,
    vtss_thread_t       *thrd
)
{
    my_thread_entry_data_t  *buf;
    vtss_addrword_t         data = 0;

    if (VTSS_MALLOC_CAST(buf, sizeof(my_thread_entry_data_t)) != NULL) {
        data = (vtss_addrword_t)((vtss_addrword_t *)buf);
    }
    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       entry,
                       data,
                       name,
                       nullptr,
                       0,
                       handle,
                       thrd);
}

/* Thread Exit */
void thread_exit(vtss_addrword_t data)
{
    VTSS_FREE((my_thread_entry_data_t *)data);
}

/* Event Create */
void event_create(vtss_flag_t *const flag)
{
    vtss_flag_init(flag);
}

/* Event Set */
void event_set(vtss_flag_t *const flag, vtss_flag_value_t value)
{
    vtss_flag_setbits(flag, value);
}

/* Event Mask */
void event_mask(vtss_flag_t *const flag, vtss_flag_value_t value)
{
    vtss_flag_maskbits(flag, value);
}

/* Event Delete */
void event_delete(vtss_flag_t *const flag)
{
    vtss_flag_destroy(flag);
}

/* Event Wait */
vtss_flag_value_t event_wait(
    vtss_flag_t         *const flag,
    vtss_flag_value_t   pattern,
    vtss_flag_mode_t    mode
)
{
    return vtss_flag_wait(flag, pattern, mode);
}
}; /* os */

} /* porting */

} /* dhcp6c */
} /* vtss */
