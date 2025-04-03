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

#include "main.h"
#include "msg_api.h"
#include "critd_api.h"
#include "vtss_bip_buffer_api.h"
#include "misc_api.h"
#include "port_iter.hxx"
#include "vtss_timer_api.h"

#if defined(VTSS_SW_OPTION_IP)
#include "ip_os.hxx"
#endif /* defined(VTSS_SW_OPTION_IP) */

#undef VTSS_TRACE_MODULE_ID

#include "dhcp6_client_api.h"
#include "dhcp6_client_serializer.hxx"

#include "dhcp6c_conf.hxx"
#include "dhcp6_client.hxx"
#include "vtss_dhcp6_core.hxx"

namespace vtss
{
namespace dhcp6c
{
#define VTSS_TRACE_MODULE_ID            VTSS_MODULE_ID_DHCP6C
#define VTSS_ALLOC_MODULE_ID            VTSS_MODULE_ID_DHCP6C

struct dhcp6c_global_t {
    critd_t                             crit;
    critd_t                             rx_crit;
    critd_t                             nm_crit;
    dhcp6c_configuration_t              configuration;
    u32                                 switch_event_value[VTSS_ISID_END];
    /* Stacking MSG Buffer */
    vtss_sem_t                          stksem;
    u8                                  *stkmsg[DHCP6C_STKMSG_MAX_ID];
    u32                                 stkmsize[DHCP6C_STKMSG_MAX_ID];
};
static dhcp6c_global_t                  DHCP6C_globals;

static vtss_handle_t                    DHCP6C_thread_handle;
static vtss_thread_t                    DHCP6C_thread_block;
static vtss_flag_t                      DHCP6C_thread_flag;
static vtss::Timer                      DHCP6C_thread_timer;
static vtss_bip_buffer_t                DHCP6C_bip;

static CapArray<msg_info_t, VTSS_APPL_CAP_IP_INTERFACE_CNT> dhcp6c_msg;

static vtss_trace_reg_t dhcp6c_trace_reg = {
    VTSS_MODULE_ID_DHCP6C, "dhcp6c", "DHCPv6 Client"
};

static vtss_trace_grp_t dhcp6c_trace_grp[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&dhcp6c_trace_reg, dhcp6c_trace_grp);

#define DHCP6C_CRIT_ENTER()                     \
    critd_enter(&DHCP6C_globals.crit,           \
                __FILE__, __LINE__)
#define DHCP6C_CRIT_EXIT()                      \
    critd_exit(&DHCP6C_globals.crit,            \
               __FILE__, __LINE__)
#define DHCP6C_CRIT_ASSERT_LOCKED()             \
    critd_assert_locked(&DHCP6C_globals.crit,   \
                        __FILE__, __LINE__)

#define DHCP6C_RX_CRIT_ENTER()                  \
    critd_enter(&DHCP6C_globals.rx_crit,        \
                __FILE__, __LINE__)
#define DHCP6C_RX_CRIT_EXIT()                   \
    critd_exit(&DHCP6C_globals.rx_crit,         \
               __FILE__, __LINE__)
#define DHCP6C_RX_CRIT_ASSERT_LOCKED()          \
    critd_assert_locked(&DHCP6C_globals.rx_crit,\
                        __FILE__, __LINE__)

#define DHCP6C_MSG_CRIT_ENTER()                 \
    critd_enter(&DHCP6C_globals.nm_crit,        \
                __FILE__, __LINE__)
#define DHCP6C_MSG_CRIT_EXIT()                  \
    critd_exit(&DHCP6C_globals.nm_crit,         \
               __FILE__, __LINE__)
#define DHCP6C_MSG_CRIT_ASSERT_LOCKED()         \
    critd_assert_locked(&DHCP6C_globals.nm_crit,\
                        __FILE__, __LINE__)

#define DHCP6C_IP_DST_ADR_OFFSET        (VTSS_IPV6_ETHER_LENGTH + sizeof(Ip6Header) - sizeof(mesa_ipv6_t))
#define DHCP6C_CRIT_RETURN(T, X)        do {    \
    T __val = (X);                              \
    DHCP6C_CRIT_EXIT();                         \
    DHCP6C_PRIV_RC_TN(__val);                   \
    return __val;                               \
} while (0)
#define DHCP6C_CRIT_RETURN_RC(X)        DHCP6C_CRIT_RETURN(mesa_rc, X)

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void vtss_appl_dhcp6_client_mib_init(void);
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_dhcp6_client_json_init(void);
#endif /* VTSS_SW_OPTION_JSON_RPC */

static mesa_rc DHCP6C_conf_default(void)
{
    dhcp6c_configuration_t          *cfg;
    dhcp6c_conf_intf_t              *ifx;
    uint32_t                         idx;
    vtss::dhcp6::client::Interface  *ifdx;

    if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx) != NULL) {
        mesa_vid_t  vidx = VTSS_VID_NULL;

        while (vtss::dhcp6::client::interface_itr(vidx, ifdx) == VTSS_RC_OK) {
            vidx = ifdx->ifidx;
            if (vtss::dhcp6::client::interface_del(vidx) != VTSS_RC_OK) {
                T_W("interface_del failed!");
            }
            if (porting::pkt::rx_process_stop(vidx) == VTSS_RC_OK) {
                if (porting::pkt::tx_misc_config(vidx, FALSE) != VTSS_RC_OK) {
                    T_W("rx_process_stop->tx_misc_config failed!");
                }
            }
        }

        DHCP6_INTF_MEM_FREE(ifdx);
    } else {
        T_W("Memory allocation failed!");
    }

    DHCP6C_CRIT_ENTER();
    cfg = &DHCP6C_globals.configuration;
    vtss_clear(*cfg);
    cfg->duid_type = DHCP6C_CONF_DEF_DUID_TYPE;
    for (idx = 0; idx < cfg->interface.size(); ++idx) {
        ifx = &cfg->interface[idx];
        ifx->slaac = DHCP6C_CONF_DEF_SLAAC_STATE;
        ifx->dhcp6 = DHCP6C_CONF_DEF_DHCP6_STATE;
        ifx->rapid_commit = DHCP6C_CONF_DEF_RAPID_COMMIT;
    }

    DHCP6C_CRIT_RETURN_RC(VTSS_RC_OK);
}

static const char *DHCP6C_stkmsg_id_txt(dhcp6c_stkmsg_id_t msg_id)
{
    const char    *txt;

    switch ( msg_id ) {
    case DHCP6C_STKMSG_ID_CFG_DEFAULT:
        txt = "DHCP6C_STKMSG_ID_CFG_DEFAULT";
        break;
    case DHCP6C_STKMSG_ID_SYS_MGMT_SET_REQ:
        txt = "DHCP6C_STKMSG_ID_SYS_MGMT_SET_REQ";
        break;
    case DHCP6C_STKMSG_ID_GLOBAL_PURGE_REQ:
        txt = "DHCP6C_STKMSG_ID_GLOBAL_PURGE_REQ";
        break;
    default:
        txt = "?";
        break;
    }

    return txt;
}

/* Allocate request/reply buffer */
static void DHCP6C_stkmsg_alloc(dhcp6c_stkmsg_id_t msg_id, dhcp6c_stkmsg_buf_t *buf)
{
    u32 msg_size;

    DHCP6C_CRIT_ENTER();
    buf->sem = &DHCP6C_globals.stksem;
    buf->msg = DHCP6C_globals.stkmsg[msg_id];
    msg_size = DHCP6C_globals.stkmsize[msg_id];
    DHCP6C_CRIT_EXIT();

    vtss_sem_wait(buf->sem);
    T_I("msg_id: %d->%s-LOCK", msg_id, DHCP6C_stkmsg_id_txt(msg_id));
    memset(buf->msg, 0x0, msg_size);
}

static void DHCP6C_stkmsg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    dhcp6c_stkmsg_id_t  msg_id;

    vtss_sem_post((vtss_sem_t *)contxt);
    if (msg) {
        msg_id = *(dhcp6c_stkmsg_id_t *)msg;
        T_I("msg_id: %d->%s-UNLOCK", msg_id, DHCP6C_stkmsg_id_txt(msg_id));
    } else {
        T_D("Just UNLOCK");
    }
}

static void DHCP6C_stkmsg_tx(dhcp6c_stkmsg_buf_t *buf, vtss_isid_t isid, size_t len)
{
    dhcp6c_stkmsg_id_t  msg_id;

    if (buf) {
        msg_id = *(dhcp6c_stkmsg_id_t *)buf->msg;
        T_D("msg_id: %d->%s, len: %zd, isid: %d", msg_id, DHCP6C_stkmsg_id_txt(msg_id), len, isid);
        msg_tx_adv(buf->sem, DHCP6C_stkmsg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_DHCP6C, isid, buf->msg, len);
    } else {
        T_E("NULL-BUF(len: %zd, isid: %d)", len, isid);
    }
}

static BOOL DHCP6C_stkmsg_rx(void *contxt, const void *const rx_msg, size_t len, vtss_module_id_t modid, u32 isid)
{
    dhcp6c_stkmsg_id_t              msg_id;
    mesa_vid_t                      vidx;
    vtss::dhcp6::client::Interface  *ifdx;

    if (!rx_msg) {
        return FALSE;
    }
    if (msg_switch_is_primary()) {
        return TRUE;
    }

    msg_id = *(dhcp6c_stkmsg_id_t *)rx_msg;
    T_D("enter(msg_id: %d, len: %zd, modid: %u, isid: %u)", msg_id, len, modid, isid);

    switch ( msg_id ) {
    case DHCP6C_STKMSG_ID_CFG_DEFAULT: {
        dhcp6c_stkmsg_cfg_default_req_t     *msg;

        msg = (dhcp6c_stkmsg_cfg_default_req_t *)rx_msg;
        T_D("Receiving %s", DHCP6C_stkmsg_id_txt(msg->msg_id));
        (void) DHCP6C_conf_default();
        break;
    }
    case DHCP6C_STKMSG_ID_SYS_MGMT_SET_REQ: {
        dhcp6c_stkmsg_sysmgmt_set_req_t     *msg;

        msg = (dhcp6c_stkmsg_sysmgmt_set_req_t *)rx_msg;
        T_D("Receiving %s", DHCP6C_stkmsg_id_txt(msg->msg_id));
        DHCP6C_CRIT_ENTER();
        utils::system_mac_set((mesa_mac_t *)msg->mgmt_mac);
        T_I("Update SystemMAC %2X-%2X-%2X-%2X-%2X-%2X",
            msg->mgmt_mac[0], msg->mgmt_mac[1], msg->mgmt_mac[2],
            msg->mgmt_mac[3], msg->mgmt_mac[4], msg->mgmt_mac[5]);
        DHCP6C_CRIT_EXIT();
        break;
    }
    case DHCP6C_STKMSG_ID_GLOBAL_PURGE_REQ: {
        dhcp6c_stkmsg_purge_req_t           *msg;

        msg = (dhcp6c_stkmsg_purge_req_t *)rx_msg;
        T_D("Receiving %s", DHCP6C_stkmsg_id_txt(msg->msg_id));
        if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx) != NULL) {
            uint32_t             idx;
            dhcp6c_conf_intf_t  *ifx;

            vidx = VTSS_VID_NULL;
            while (vtss::dhcp6::client::interface_itr(vidx, ifdx) == VTSS_RC_OK) {
                vidx = ifdx->ifidx;
                if (vtss::dhcp6::client::interface_del(vidx) != VTSS_RC_OK) {
                    T_W("interface_del failed!");
                }
                if (porting::pkt::rx_process_stop(vidx) == VTSS_RC_OK) {
                    if (porting::pkt::tx_misc_config(vidx, FALSE) != VTSS_RC_OK) {
                        T_W("rx_process_stop->tx_misc_config failed!");
                    }
                }
            }

            DHCP6_INTF_MEM_FREE(ifdx);

            DHCP6C_CRIT_ENTER();
            for (idx = 0; idx < DHCP6C_globals.configuration.interface.size(); ++idx) {
                ifx = &DHCP6C_globals.configuration.interface[idx];
                memset(ifx, 0x0, sizeof(dhcp6c_conf_intf_t));
                ifx->slaac = DHCP6C_CONF_DEF_SLAAC_STATE;
                ifx->dhcp6 = DHCP6C_CONF_DEF_DHCP6_STATE;
                ifx->rapid_commit = DHCP6C_CONF_DEF_RAPID_COMMIT;
            }
            DHCP6C_CRIT_EXIT();
        } else {
            T_W("Memory allocation failed!");
        }
        break;
    }
    default:
        T_E("unknown message ID: %d", msg_id);
        break;
    }

    T_D("exit()");
    return TRUE;
}

#define DHCP6C_INTF_CNTR_ASSIGN(x, y)   do {    \
T_D("Y-SOL:%u", (y)->tx_solicit);\
    (x)->rx_advertise = (y)->rx_advertise;      \
    (x)->rx_reply = (y)->rx_reply;              \
    (x)->rx_reconfigure = (y)->rx_reconfigure;  \
    (x)->rx_error = (y)->rx_error;              \
    (x)->rx_drop = (y)->rx_drop;                \
    (x)->rx_unknown = (y)->rx_unknown;          \
    (x)->tx_solicit = (y)->tx_solicit;          \
    (x)->tx_request = (y)->tx_request;          \
    (x)->tx_confirm = (y)->tx_confirm;          \
    (x)->tx_renew = (y)->tx_renew;              \
    (x)->tx_rebind = (y)->tx_rebind;            \
    (x)->tx_release = (y)->tx_release;          \
    (x)->tx_decline = (y)->tx_decline;          \
    (x)->tx_information_request =               \
    (y)->tx_information_request;                \
    (x)->tx_error = (y)->tx_error;              \
    (x)->tx_drop = (y)->tx_drop;                \
    (x)->tx_unknown = (y)->tx_unknown;          \
T_D("X-SOL:%u", (x)->tx_solicit);\
} while (0)

#define DHCP6C_INTF_ASSIGN(x, y)        do {                                            \
    switch ( (y)->duid.duid_type ) {                                                    \
    case DHCPV6_DUID_TYPE_LLT:                                                          \
        (x)->duid.duid_type = DHCP6_DUID_LLT;                                           \
        (x)->duid.type.llt.hardware_type = (y)->duid.type.llt.hardware_type;            \
        (x)->duid.type.llt.time = (y)->duid.type.llt.time;                              \
        memcpy(&(x)->duid.type.llt.lla, &(y)->duid.type.llt.lla, sizeof(mesa_mac_t));   \
        break;                                                                          \
    case DHCPV6_DUID_TYPE_EN:                                                           \
        (x)->duid.duid_type = DHCP6_DUID_EN;                                            \
        (x)->duid.type.en.enterprise_number = (y)->duid.type.en.enterprise_number;      \
        (x)->duid.type.en.id = (y)->duid.type.en.id;                                    \
        break;                                                                          \
    default:                                                                            \
        (x)->duid.duid_type = DHCP6_DUID_LL;                                            \
        (x)->duid.type.ll.hardware_type = (y)->duid.type.ll.hardware_type;              \
        memcpy(&(x)->duid.type.ll.lla, &(y)->duid.type.ll.lla, sizeof(mesa_mac_t));     \
        break;                                                                          \
    }                                                                                   \
    (x)->ifidx = (y)->ifidx;                                                            \
    (x)->rapid_commit = (y)->rapid_commit;                                              \
    (x)->srvc = ((y)->type & 0xFF);                                                     \
    DHCP6_ADRS_CPY(&(x)->address, &(y)->addrs.address);                                 \
    (x)->prefix_length = (y)->addrs.prefix_length;                                      \
    (x)->t1 = (y)->addrs.t1;                                                            \
    (x)->t2 = (y)->addrs.t2;                                                            \
    (x)->preferred_lifetime = (y)->addrs.preferred_lifetime;                            \
    (x)->valid_lifetime = (y)->addrs.valid_lifetime;                                    \
    (x)->refresh_ts = (y)->addrs.refresh_ts;                                            \
    if ((y)->server) {                                                                  \
        vtss::dhcp6::client::ServerRecord   *s = (y)->server;                           \
        if (s->name_server.empty()) {                                                   \
            DHCP6_ADRS_SET(&(x)->dns_srv_addr, 0x0);                                    \
            memset((x)->dns_domain_name, 0x0, 256);                                     \
        } else {                                                                        \
            auto                            nsa = s->name_server.begin();               \
            DHCP6_ADRS_CPY(&(x)->dns_srv_addr, &(*nsa));                                \
            memset((x)->dns_domain_name, 0x0, 256);                                     \
            if (!(s->name_list.empty())) {                                              \
                auto                        nsn = s->name_list.begin();                 \
                strncpy((x)->dns_domain_name, &(*nsn)[0], strlen(&(*nsn)[0]));          \
            }                                                                           \
        }                                                                               \
        DHCP6_ADRS_CPY(&(x)->srv_addr, &s->addrs);                                      \
    } else {                                                                            \
        DHCP6_ADRS_SET(&(x)->srv_addr, 0x0);                                            \
        DHCP6_ADRS_SET(&(x)->dns_srv_addr, 0x0);                                        \
        memset((x)->dns_domain_name, 0x0, 256);                                         \
    }                                                                                   \
    DHCP6C_INTF_CNTR_ASSIGN(&((x)->cntrs), &((y)->cntrs));                              \
} while (0)

mesa_rc dhcp6_client_interface_itr(mesa_vid_t vidx, Dhcp6cInterface *const intf)
{
    mesa_rc                         rc;
    vtss::dhcp6::client::Interface  *ifdx;

    if (!intf) {
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }
    if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx) == NULL) {
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    if ((rc = vtss::dhcp6::client::interface_itr(vidx, ifdx)) != VTSS_RC_OK) {
        T_D("interface_itr failed");
    } else {
        DHCP6C_INTF_ASSIGN(intf, ifdx);
    }
    DHCP6_INTF_MEM_FREE(ifdx);

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc dhcp6_client_interface_get(mesa_vid_t vidx, Dhcp6cInterface *const intf)
{
    mesa_rc                         rc;
    vtss::dhcp6::client::Interface  *ifdx;

    if (!intf) {
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }
    if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx) == NULL) {
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    if ((rc = vtss::dhcp6::client::interface_get(vidx, ifdx)) != VTSS_RC_OK) {
        T_D("interface_get failed(VID:%u)", vidx);
    } else {
        DHCP6C_INTF_ASSIGN(intf, ifdx);
    }
    DHCP6_INTF_MEM_FREE(ifdx);

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc dhcp6_client_interface_set(mesa_vid_t vidx, const Dhcp6cInterface *const intf)
{
    mesa_rc                         rc;
    vtss_ifindex_t                  ifindex;
    vtss::dhcp6::client::Interface  *ifdx;
#if defined(VTSS_SW_OPTION_IP)
    vtss_appl_ip_if_status_link_t   link_status;
    vtss_appl_ip_if_status_t        ipv6_status;
#endif /* defined(VTSS_SW_OPTION_IP) */

    if (!intf) {
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }
    if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx) == NULL) {
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    if (vtss::dhcp6::client::interface_get(vidx, ifdx) != VTSS_RC_OK) {
        u32 i, iaid_calc;

        /* Give DUID and IAID as the default parameter for new interface */
        vtss_clear(*ifdx);
        ifdx->duid.duid_type = DHCPV6_DUID_TYPE_LL;
        ifdx->duid.type.ll.hardware_type = DHCPV6_DUID_HARDWARE_TYPE;
        if (!utils::system_mac_get(&ifdx->duid.type.ll.lla)) {
            T_W("Cannot Get MAC for DUID");
        }
        ifdx->ifidx = vidx;

        iaid_calc = ifdx->ifidx;
        iaid_calc = (iaid_calc << 16 | ifdx->duid.duid_type);
        for (i = 0; i < sizeof(mesa_mac_t); ++i) {
            iaid_calc += ifdx->duid.type.ll.lla.addr[i];
        }
        ifdx->iaid = iaid_calc << ifdx->duid.duid_type;
    }

    /* duid and ifidx cannot be updated */

    ifdx->rapid_commit = intf->rapid_commit;
    if (intf->srvc == DHCP6C_RUN_AUTO_BOTH) {
        ifdx->type = vtss::dhcp6::client::TYPE_SLAAC_AND_DHCP;
    } else if (intf->srvc == DHCP6C_RUN_SLAAC_ONLY) {
        ifdx->type = vtss::dhcp6::client::TYPE_SLAAC;
    } else if (intf->srvc == DHCP6C_RUN_DHCP_ONLY) {
        ifdx->type = vtss::dhcp6::client::TYPE_DHCP;
    } else {
        ifdx->type = vtss::dhcp6::client::TYPE_NONE;
    }

    /* Leave the rest Status alone, except LinkStatus M/O Flag */
    ifdx->stateless = FALSE;
#if defined(VTSS_SW_OPTION_IP)
    (void)vtss_ifindex_from_vlan(vidx, &ifindex);

    rc = vtss_appl_ip_if_status_link_get(ifindex, &link_status);

    if (rc == VTSS_RC_OK && (link_status.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) && vtss_appl_ip_if_status_get(ifindex, VTSS_APPL_IP_IF_STATUS_TYPE_IPV6, 1, nullptr, &ipv6_status) == VTSS_RC_OK) {
        ifdx->link = TRUE;
    } else {
        T_D("%s", error_txt(rc));
    }

    if (rc == VTSS_RC_OK) {
        ifdx->m_flag = (link_status.flags & VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_MANAGED) != 0;
        ifdx->o_flag = (link_status.flags & VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_OTHER)   != 0;
        if (ifdx->o_flag && !ifdx->m_flag) {
            ifdx->stateless = TRUE;
        }
    } else {
        T_D("%s", error_txt(rc));
    }
#else
    ifdx->link = TRUE;
#endif /* defined(VTSS_SW_OPTION_IP) */

    T_I("INTF%u-Link%s/%s/M(%s)/O(%s)/%sDAD%s",
        vidx,
        ifdx->link ? "Up" : "Down",
        ifdx->stateless ? "Stateless" : "Stateful",
        ifdx->m_flag ? "Y" : "N",
        ifdx->o_flag ? "Y" : "N",
        ifdx->dad ? "" : "No",
        ifdx->rapid_commit ? " with RapidCommit" : "");

    if ((rc = vtss::dhcp6::client::interface_set(vidx, ifdx)) != VTSS_RC_OK) {
        T_W("interface_set failed");
    } else {
        if ((rc = porting::pkt::tx_misc_config(vidx, TRUE)) == VTSS_RC_OK) {
            rc = porting::pkt::rx_process_start(vidx);
        }
    }

    DHCP6_INTF_MEM_FREE(ifdx);

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc dhcp6_client_interface_del(mesa_vid_t vidx)
{
    mesa_rc                         rc;
    vtss::dhcp6::client::Interface  *ifdx;

    if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx) == NULL) {
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    if ((rc = vtss::dhcp6::client::interface_get(vidx, ifdx)) != VTSS_RC_OK) {
        T_D("interface_get failed");
    } else {
        if ((rc = vtss::dhcp6::client::interface_del(vidx)) == VTSS_RC_OK) {
            if ((rc = porting::pkt::rx_process_stop(vidx)) == VTSS_RC_OK) {
                rc = porting::pkt::tx_misc_config(vidx, FALSE);
            }
        }
    }
    DHCP6_INTF_MEM_FREE(ifdx);

    DHCP6C_PRIV_TD_RETURN(rc);
}

#define DHCP6C_INTF_MSG_ENCODE(x, y, z) do {                    \
    if (!(y) || !(z)) break;                                    \
    if ((y)->type == DHCP6C_MSG_IF_DEL) {                       \
        (x) = DHCP6C_EVENT_MSG_DEL;                             \
        (z)->del_msg = TRUE;                                    \
    } else if ((y)->type == DHCP6C_MSG_IF_RA_FLAG) {            \
        (x) = DHCP6C_EVENT_MSG_RA;                              \
        (z)->ra_msg = TRUE;                                     \
        (z)->m_flag = (y)->msg.ra.m_flag;                       \
        (z)->o_flag = (y)->msg.ra.o_flag;                       \
    } else if ((y)->type == DHCP6C_MSG_IF_DAD) {                \
        (x) = DHCP6C_EVENT_MSG_DAD;                             \
        (z)->dad_msg = TRUE;                                    \
        DHCP6_ADRS_CPY(&(z)->address, &(y)->msg.dad.address);   \
    } else if ((y)->type == DHCP6C_MSG_IF_LINK) {               \
        (x) = DHCP6C_EVENT_MSG_LINK;                            \
        (z)->link_msg = TRUE;                                   \
        (z)->new_state = (y)->msg.link.new_state;               \
        (z)->old_state = (y)->msg.link.old_state;               \
    }                                                           \
} while (0)

mesa_rc dhcp6_client_interface_notify(const Dhcp6cNotify *const nmsg)
{
    u16                 idx, avail;
    msg_info_t          *minf;
    vtss_flag_value_t   notify;

    if (!nmsg) {
        return VTSS_RC_ERROR;
    }

    T_D("IFIDX:%u/Type:%d", nmsg->ifidx, nmsg->type);

    avail = DHCP6C_MAX_INTERFACES + 1;
    notify = DHCP6C_EVENT_ANY;
    DHCP6C_MSG_CRIT_ENTER();
    for (idx = 0; idx < dhcp6c_msg.size(); ++idx) {
        minf = &dhcp6c_msg[idx];
        if (!minf->valid) {
            if (avail > DHCP6C_MAX_INTERFACES) {
                avail = idx;
            }
            continue;
        }

        T_D("IDX:%u/AVAIL:%u [IFIDX:%u]", idx, avail, minf->ifidx);
        if (minf->ifidx == nmsg->ifidx) {
            T_D("OLD-IDX:%u", idx);
            DHCP6C_INTF_MSG_ENCODE(notify, nmsg, minf);
            break;
        }
    }

    if (idx >= DHCP6C_MAX_INTERFACES &&
        avail < DHCP6C_MAX_INTERFACES) {
        T_D("NEW-IDX:%u [IDX:%u]", avail, idx);
        minf = &dhcp6c_msg[avail];
        memset(minf, 0x0, sizeof(msg_info_t));
        minf->valid = TRUE;
        minf->ifidx = nmsg->ifidx;
        minf->vlanx = nmsg->vlanx;
        DHCP6C_INTF_MSG_ENCODE(notify, nmsg, minf);
    }
    DHCP6C_MSG_CRIT_EXIT();

    if (notify == DHCP6C_EVENT_ANY) {
        return VTSS_RC_ERROR;
    }

    porting::os::event_set(&DHCP6C_thread_flag, notify);

    return VTSS_RC_OK;
}

static mesa_rc DHCP6C_intf_conf_itr(mesa_vid_t vidx, dhcp6c_conf_intf_t *const output)
{
    dhcp6c_configuration_t  *cfg;
    dhcp6c_conf_intf_t      *ifx;
    uint32_t                idx, val;
    mesa_vid_t              nxt;
    mesa_rc                 rc;

    if (!output) {
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    val = 0;
    nxt = VTSS_VID_NULL;
    rc = VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND;
    DHCP6C_CRIT_ENTER();
    cfg = &DHCP6C_globals.configuration;
    for (idx = 0; idx < cfg->interface.size(); ++idx) {
        ifx = &cfg->interface[idx];
        if (!ifx->valid) {
            continue;
        }

        if (ifx->index > vidx) {
            if (nxt == VTSS_VID_NULL) {
                nxt = ifx->index;
                val = idx + 1;
            } else {
                if (nxt > ifx->index) {
                    nxt = ifx->index;
                    val = idx + 1;
                }
            }
        }
    }

    if (val && nxt != VTSS_VID_NULL) {
        memcpy(output, &cfg->interface[val - 1], sizeof(dhcp6c_conf_intf_t));
        rc = VTSS_RC_OK;
    }

    DHCP6C_CRIT_RETURN_RC(rc);
}

static mesa_rc DHCP6C_intf_conf_get(mesa_vid_t vidx, dhcp6c_conf_intf_t *const output)
{
    dhcp6c_configuration_t  *cfg;
    dhcp6c_conf_intf_t      *ifx;
    uint32_t                idx, val;
    mesa_rc                 rc;

    if (!output) {
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    val = 0;
    rc = VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND;
    DHCP6C_CRIT_ENTER();
    cfg = &DHCP6C_globals.configuration;
    for (idx = 0; idx < cfg->interface.size(); ++idx) {
        ifx = &cfg->interface[idx];
        if (!ifx->valid || ifx->index != vidx) {
            continue;
        }

        val = idx + 1;
        break;
    }

    if (val) {
        memcpy(output, &cfg->interface[val - 1], sizeof(dhcp6c_conf_intf_t));
        rc = VTSS_RC_OK;
    }

    DHCP6C_CRIT_RETURN_RC(rc);
}

static mesa_rc DHCP6C_intf_conf_set(dhcp6_conf_op_t op, mesa_vid_t vidx, const dhcp6c_conf_intf_t *const input)
{
    dhcp6c_configuration_t  *cfg;
    dhcp6c_conf_intf_t      *ifx;
    uint32_t                idx, val;
    mesa_rc                 rc;

    if (!input) {
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    val = 0;
    rc = VTSS_RC_OK;
    DHCP6C_CRIT_ENTER();
    cfg = &DHCP6C_globals.configuration;
    for (idx = 0; idx < cfg->interface.size(); ++idx) {
        ifx = &cfg->interface[idx];
        if (!ifx->valid || ifx->index != vidx) {
            continue;
        }

        val = idx + 1;
        break;
    }

    if (op != DHCP6_CONF_OP_ADD) {
        if (!val) {
            rc = VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND;
            DHCP6C_CRIT_EXIT();
            DHCP6C_PRIV_TD_RETURN(rc);
        }
    } else {
        if (val) {
            rc = VTSS_APPL_DHCP6C_ERROR_ENTRY_EXISTING;
            DHCP6C_CRIT_EXIT();
            DHCP6C_PRIV_TD_RETURN(rc);
        }

        for (idx = 0; idx < cfg->interface.size(); ++idx) {
            if (!cfg->interface[idx].valid) {
                val = idx + 1;
                break;
            }
        }
    }

    if (val) {
        if (op != DHCP6_CONF_OP_DEL) {
            ifx = &cfg->interface[val - 1];
            memcpy(ifx, input, sizeof(dhcp6c_conf_intf_t));
            ifx->valid = TRUE;
            ifx->index = vidx;
        } else {
            memset(&cfg->interface[val - 1], 0x0, sizeof(dhcp6c_conf_intf_t));
        }

        rc = VTSS_RC_OK;
    } else {
        rc = VTSS_APPL_DHCP6C_ERROR_TABLE_FULL;
    }
    DHCP6C_CRIT_EXIT();

    if (rc == VTSS_RC_OK) {
        Dhcp6cInterface *ifce = NULL;

        switch ( op ) {
        case DHCP6_CONF_OP_DEL:
            rc = dhcp6_client_interface_del(vidx);
            break;
        default:
            if (VTSS_MALLOC_CAST(ifce, sizeof(Dhcp6cInterface)) == NULL) {
                T_D("Not enough memory!");
                rc = VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
                break;
            }

            memset(ifce, 0x0, sizeof(Dhcp6cInterface));
            if (input->slaac && input->dhcp6) {
                ifce->srvc = DHCP6C_RUN_AUTO_BOTH;
            } else {
                if (input->slaac) {
                    ifce->srvc = DHCP6C_RUN_SLAAC_ONLY;
                }
                if (input->dhcp6) {
                    ifce->srvc = DHCP6C_RUN_DHCP_ONLY;
                }
            }
            ifce->rapid_commit = input->rapid_commit;
            rc = dhcp6_client_interface_set(vidx, ifce);
            VTSS_FREE(ifce);
            break;
        }
    }

    DHCP6C_PRIV_TD_RETURN(rc);
}

static mesa_rc DHCP6C_intf_restart(mesa_vid_t vidx)
{
    dhcp6c_configuration_t  *cfg;
    dhcp6c_conf_intf_t      *ifx;
    uint32_t                idx, val;
    mesa_rc                 rc;

    val = 0;
    rc = VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND;
    DHCP6C_CRIT_ENTER();
    cfg = &DHCP6C_globals.configuration;
    for (idx = 0; idx < cfg->interface.size(); ++idx) {
        ifx = &cfg->interface[idx];
        if (!ifx->valid || ifx->index != vidx) {
            continue;
        }

        val = idx + 1;
        break;
    }
    DHCP6C_CRIT_EXIT();

    if (val && (rc = vtss::dhcp6::client::interface_rst(vidx)) != VTSS_RC_OK) {
        T_D("interface_rst failed");
    }

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc frame_snd(dhcp6c_ifidx_t ifx, u32 len, u8 *const frm)
{
    pkt_info_t  info;

    info.tstamp = vtss_current_time();
    T_D("TimeStamp:" VPRI64u" / EgressVid:%u / Length:%u", info.tstamp, ifx, len);
    T_D_HEX(frm, len);

    if (utils::convert_ip_to_mac((mesa_ipv6_t *)&frm[DHCP6C_IP_DST_ADR_OFFSET], (mesa_mac_t *)&frm[0]) != VTSS_RC_OK ||
        !utils::device_mac_get((mesa_mac_t *)&frm[6])) {
        return VTSS_APPL_DHCP6C_ERROR_PKT_GEN;
    }
    frm[12] = 0x86;
    frm[13] = 0xdd;

    info.ifidx = ifx;
    info.len = len;

    return porting::pkt::tx_process(&info, frm);
}

BOOL queue_snd(const u8 *const frm, const pkt_info_t *const info)
{
    u8      *rcv_buf;
    u32     aligned_rx_info_len_bytes, aligned_frm_len_bytes;

    if (!frm || !info) {
        return FALSE;
    }

    T_D("TimeStamp:" VPRI64u" / Length:%u / IngressVid:%u", info->tstamp, info->len, info->ifidx);
    T_D_HEX(frm, info->len);

    aligned_rx_info_len_bytes = sizeof(pkt_info_t);
    aligned_rx_info_len_bytes = sizeof(int) * ((aligned_rx_info_len_bytes + 3) / sizeof(int));
    aligned_frm_len_bytes = info->len;
    aligned_frm_len_bytes = sizeof(int) * ((aligned_frm_len_bytes + 3) / sizeof(int));

    DHCP6C_RX_CRIT_ENTER();
    rcv_buf = vtss_bip_buffer_reserve(&DHCP6C_bip, aligned_rx_info_len_bytes + aligned_frm_len_bytes);
    if (!rcv_buf) {
        T_I("Failure in reserving BIP(BUF_SZ:%d/CMT_SZ:%d)",
            vtss_bip_buffer_get_buffer_size(&DHCP6C_bip),
            vtss_bip_buffer_get_committed_size(&DHCP6C_bip));
        DHCP6C_RX_CRIT_EXIT();
        return FALSE;
    }
    if ((u64)rcv_buf & 0x3) {
        T_D("BIP buffer not correctly aligned");
    }

    memcpy(&rcv_buf[0], info, sizeof(pkt_info_t));
    memcpy(&rcv_buf[aligned_rx_info_len_bytes], frm, info->len);

    vtss_bip_buffer_commit(&DHCP6C_bip);
    DHCP6C_RX_CRIT_EXIT();

    porting::os::event_set(&DHCP6C_thread_flag, DHCP6C_EVENT_QRTV);

    return TRUE;
}

void queue_rtv(void)
{
    u8          *rcv_buf;
    int         buf_size;
    pkt_info_t  info;
    size_t      sz_val, sz_offset;

    buf_size = 0;
    DHCP6C_RX_CRIT_ENTER();
    while ((rcv_buf = vtss_bip_buffer_get_contiguous_block(&DHCP6C_bip, &buf_size)) != NULL) {
        if ((u64)rcv_buf & 0x3) {
            T_D("BIP buffer not correctly aligned");
        }
        sz_val = sz_offset = 0;
        DHCP6C_RX_CRIT_EXIT();

        sz_val = sizeof(pkt_info_t);
        memcpy(&info, &rcv_buf[sz_offset], sz_val);
        sz_offset = sz_val;
        sz_offset = sizeof(int) * ((sz_offset + 3) / sizeof(int));
        DHCP6C_PRIV_RC_TD(dhcp6::client::receive(&rcv_buf[sz_offset], info.tstamp, info.len, info.ifidx));
        sz_val = sz_offset;
        sz_offset = info.len;
        sz_offset = sizeof(int) * ((sz_offset + 3) / sizeof(int));
        sz_val += sz_offset;

        DHCP6C_RX_CRIT_ENTER();
        vtss_bip_buffer_decommit_block(&DHCP6C_bip, sz_val);
    }
    DHCP6C_RX_CRIT_EXIT();
}

static void DHCP6C_timer_isr(struct vtss::Timer *timer)
{
    porting::os::event_set(&DHCP6C_thread_flag, DHCP6C_EVENT_WAKEUP);
}

static void DHCP6C_handle_msg(const msg_info_t *const minf)
{
    BOOL               skip;
    mesa_vid_t         vidx;
    dhcp6c_conf_intf_t *intf;
    vtss_ifindex_t     ifindex;

    if (VTSS_MALLOC_CAST(intf, sizeof(dhcp6c_conf_intf_t)) == NULL) {
        T_D("Not enough memory!");
        return;
    }

    if (!minf || !minf->valid) {
        T_D("DROP-SIG(INTF-%s-%s)", minf ? "OK" : "NG", minf ? (minf->valid ? "VALID" : "INVALID") : "NULL");
        VTSS_FREE(intf);
        return;
    }

    vidx = minf->vlanx;
#if defined(VTSS_SW_OPTION_IP)
    if (vidx == VTSS_VID_NULL) {
        if ((ifindex = ip_os_ifindex_to_ifindex(minf->ifidx)) == VTSS_IFINDEX_NONE || (vidx = vtss_ifindex_as_vlan(ifindex)) == VTSS_VID_NULL) {
            T_D("VID:%u->%u", minf->ifidx, vidx);
            VTSS_FREE(intf);
            return;
        }
    }
#endif /* defined(VTSS_SW_OPTION_IP) */

    if (DHCP6C_intf_conf_get(vidx, intf) != VTSS_RC_OK) {
        T_D("DROP-SIG(!INTF:%u->%u)", minf->ifidx, vidx);
        VTSS_FREE(intf);
        return;
    }
    T_D("Vlan%u-MSG(%sDEL/%sLink/%sRA/%sDAD)", vidx,
        minf->del_msg ? "" : "No",
        minf->link_msg ? "" : "No",
        minf->ra_msg ? "" : "No",
        minf->dad_msg ? "" : "No");

    skip = FALSE;
    if (minf->del_msg) {
        skip = (DHCP6C_intf_conf_set(DHCP6_CONF_OP_DEL, vidx, intf) == VTSS_RC_OK);
    }
    VTSS_FREE(intf);
    if (!skip && minf->link_msg) {
        i8 link_status = 0;

        if (minf->old_state < 0) {
            skip = TRUE;
            if (minf->new_state > 0) {
                skip = FALSE;
                link_status = 1;
            }
        } else {
            if (minf->new_state < 0) {
                skip = TRUE;
                link_status = -1;
            }
        }

        if (link_status) {
            T_D("Vlan%u-Link%s", vidx, link_status > 0 ? "Up" : "Down");
            if (vtss::dhcp6::client::interface_link(vidx, link_status) != VTSS_RC_OK) {
                T_D("Link is not handled on VID %u", vidx);
            }
        }
    }

    if (!skip && minf->ra_msg) {
        T_D("Vlan%u-RA-FLAG(%s%s)", vidx, minf->m_flag ? "M" : "", minf->o_flag ? "O" : "");
        if (vtss::dhcp6::client::interface_flag(vidx, minf->m_flag, minf->o_flag) != VTSS_RC_OK) {
            T_D("Failed in adjust RA-M/O Flag on VID %u", vidx);
        }
    }

    if (!skip && minf->dad_msg) {
        Dhcp6cInterface *ifce = NULL;

        if (VTSS_MALLOC_CAST(ifce, sizeof(Dhcp6cInterface)) != NULL &&
            dhcp6_client_interface_get(vidx, ifce) == VTSS_RC_OK) {
            if (DHCP6_ADRS_EQUAL(&ifce->address, &minf->address)) {
                char adrString[40];

                memset(adrString, 0x0, sizeof(adrString));
                T_D("Vlan%u-DAD(%s)", vidx, misc_ipv6_txt(&ifce->address, adrString));
                if (vtss::dhcp6::client::interface_dad(vidx) != VTSS_RC_OK) {
                    T_D("Decline(DAD) failure on VID %u", vidx);
                }
            }
        }

        if (ifce) {
            VTSS_FREE(ifce);
        }
    }
}

static mesa_rc DHCP6C_stacking_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = DHCP6C_stkmsg_rx;
    filter.modid = VTSS_MODULE_ID_DHCP6C;
    return msg_rx_filter_register(&filter);
}

static void DHCP6C_stacking_conf_default(vtss_isid_t isidx)
{
    switch_iter_t                   sit;
    vtss_isid_t                     isid;
    dhcp6c_stkmsg_cfg_default_req_t *stkmsg;
    dhcp6c_stkmsg_buf_t             stkbuf;

    if (!msg_switch_is_primary() || isidx == VTSS_ISID_LOCAL) {
        return;
    }

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        isid = sit.isid;
        if (msg_switch_is_local(isid) ||
            (isidx != VTSS_ISID_GLOBAL && isidx != isid)) {
            continue;
        }

        DHCP6C_stkmsg_alloc(DHCP6C_STKMSG_ID_CFG_DEFAULT, &stkbuf);
        stkmsg = (dhcp6c_stkmsg_cfg_default_req_t *)stkbuf.msg;

        stkmsg->isidx = isid;
        stkmsg->msg_id = DHCP6C_STKMSG_ID_CFG_DEFAULT;
        DHCP6C_stkmsg_tx(&stkbuf, isid, sizeof(*stkmsg));
    }
}

static void DHCP6C_stacking_sysmgmt_set(vtss_isid_t isidx)
{
    switch_iter_t                   sit;
    vtss_isid_t                     isid;
    dhcp6c_stkmsg_sysmgmt_set_req_t *stkmsg;
    dhcp6c_stkmsg_buf_t             stkbuf;

    if (!msg_switch_is_primary() || isidx == VTSS_ISID_LOCAL) {
        return;
    }

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        isid = sit.isid;
        if (msg_switch_is_local(isid) ||
            (isidx != VTSS_ISID_GLOBAL && isidx != isid)) {
            continue;
        }

        DHCP6C_stkmsg_alloc(DHCP6C_STKMSG_ID_SYS_MGMT_SET_REQ, &stkbuf);
        stkmsg = (dhcp6c_stkmsg_sysmgmt_set_req_t *)stkbuf.msg;

        stkmsg->isidx = isid;
        stkmsg->msg_id = DHCP6C_STKMSG_ID_SYS_MGMT_SET_REQ;
        if (!utils::system_mac_get((mesa_mac_t *)stkmsg->mgmt_mac)) {
            memset(stkmsg->mgmt_mac, 0x0, sizeof(mesa_mac_t));
        }
        DHCP6C_stkmsg_tx(&stkbuf, isid, sizeof(*stkmsg));
    }
}

static void DHCP6C_stacking_intdb_purge(vtss_isid_t isidx)
{
    switch_iter_t                   sit;
    vtss_isid_t                     isid;
    dhcp6c_stkmsg_purge_req_t       *stkmsg;
    dhcp6c_stkmsg_buf_t             stkbuf;

    if (!msg_switch_is_primary() || isidx == VTSS_ISID_LOCAL) {
        return;
    }

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        isid = sit.isid;
        if (msg_switch_is_local(isid) ||
            (isidx != VTSS_ISID_GLOBAL && isidx != isid)) {
            continue;
        }

        DHCP6C_stkmsg_alloc(DHCP6C_STKMSG_ID_GLOBAL_PURGE_REQ, &stkbuf);
        stkmsg = (dhcp6c_stkmsg_purge_req_t *)stkbuf.msg;

        stkmsg->isidx = isid;
        stkmsg->msg_id = DHCP6C_STKMSG_ID_GLOBAL_PURGE_REQ;
        DHCP6C_stkmsg_tx(&stkbuf, isid, sizeof(*stkmsg));
    }
}

void dhcp6c_thread(vtss_addrword_t data)
{
    vtss_flag_value_t events;

    /* Initialize EVENT groups */
    porting::os::event_create(&DHCP6C_thread_flag);

    /* Initialize Periodical Wakeup Timer  */
    DHCP6C_thread_timer.set_repeat(true);
    DHCP6C_thread_timer.set_period(vtss::milliseconds(DHCP6C_THREAD_TICK_TIME));
    DHCP6C_thread_timer.callback = DHCP6C_timer_isr;
    DHCP6C_thread_timer.modid = VTSS_MODULE_ID_DHCP6C;
    if (vtss_timer_start(&DHCP6C_thread_timer) != VTSS_RC_OK) {
        T_D("vtss_timer_start failed");
    }

    /* Initialize STKMSG-RX */
    (void) DHCP6C_stacking_register();
    DHCP6C_CRIT_ENTER();
    utils::system_mac_set(0);
    DHCP6C_CRIT_EXIT();

    while (TRUE) {
        events = porting::os::event_wait(&DHCP6C_thread_flag, DHCP6C_EVENT_ANY, VTSS_FLAG_WAITMODE_OR_CLR);

        T_N("DHCP6C_EVENT: %d", events);

        if (events & DHCP6C_EVENT_EXIT) {
            DHCP6C_RX_CRIT_ENTER();
            vtss_bip_buffer_clear(&DHCP6C_bip);
            DHCP6C_RX_CRIT_EXIT();

            break;
        }

        if (events & DHCP6C_EVENT_RESUME) {
            mesa_mac_t  sys_mac;

            T_D("DHCP6C_EVENT_RESUME");
            if (vtss_timer_start(&DHCP6C_thread_timer) != VTSS_RC_OK) {
                T_W("Unable to start timer");
            }

            if (msg_switch_is_primary() && !utils::system_mac_get(&sys_mac)) {
                T_I("Reset SystemMAC");

                DHCP6C_CRIT_ENTER();
                utils::system_mac_set(0);
                DHCP6C_CRIT_EXIT();

                DHCP6C_stacking_sysmgmt_set(VTSS_ISID_GLOBAL);
            }

            queue_rtv();
        }

        if (events & (DHCP6C_EVENT_MSG_RA | DHCP6C_EVENT_MSG_DAD | DHCP6C_EVENT_MSG_LINK | DHCP6C_EVENT_MSG_DEL)) {
            u16         idx, rdx;
            msg_info_t  *rd_n_clr;

            T_D("DHCP6C_EVENT_MSG");

            if (VTSS_MALLOC_CAST(rd_n_clr, sizeof(msg_info_t) * DHCP6C_MAX_INTERFACES) != NULL) {
                rdx = 0;
                DHCP6C_MSG_CRIT_ENTER();
                for (idx = 0; idx < dhcp6c_msg.size(); ++idx) {
                    if (dhcp6c_msg[idx].valid) {
                        memcpy(&rd_n_clr[rdx++], &dhcp6c_msg[idx], sizeof(msg_info_t));
                    }
                    memset(&dhcp6c_msg[idx], 0x0, sizeof(msg_info_t));
                }
                DHCP6C_MSG_CRIT_EXIT();

                T_D("GoFor %u MSG", rdx);

                for (idx = 0; idx < rdx; ++idx) {
                    DHCP6C_handle_msg(&rd_n_clr[idx]);
                }

                VTSS_FREE(rd_n_clr);
            } else {
                T_D("Not enough memory!");
            }
        }

        if (events & DHCP6C_EVENT_ICFG_LOADING_POST) {
            vtss_isid_t isidx;
            BOOL        doact[VTSS_ISID_END];

            T_D("DHCP6C_EVENT_SWITCH");
            DHCP6C_CRIT_ENTER();
            for (isidx = 0; isidx < VTSS_ISID_END; ++isidx) {
                doact[isidx] = FALSE;
                if (DHCP6C_globals.switch_event_value[isidx] & DHCP6C_EVENT_ICFG_LOADING_POST) {
                    doact[isidx] = TRUE;
                }
                DHCP6C_globals.switch_event_value[isidx] = 0;
            }
            DHCP6C_CRIT_EXIT();

            for (isidx = 0; isidx < VTSS_ISID_END; ++isidx) {
                if (doact[isidx]) {
                    DHCP6C_stacking_intdb_purge(isidx);
                    DHCP6C_stacking_sysmgmt_set(isidx);
                }
            }
        }

        if (events & DHCP6C_EVENT_DEFAULT) {
            T_D("DHCP6C_EVENT_DEFAULT");
            DHCP6C_stacking_conf_default(VTSS_ISID_GLOBAL);
        }

        if (events & DHCP6C_EVENT_QRTV) {
            T_N("DHCP6C_EVENT_QRTV");
            queue_rtv();
        }

        if (events & DHCP6C_EVENT_WAKEUP) {
            T_N("DHCP6C_EVENT_WAKEUP");
            DHCP6C_PRIV_RC_TN(dhcp6::client::tick());
        }
    }

    vtss_timer_cancel(&DHCP6C_thread_timer);
    porting::os::event_delete(&DHCP6C_thread_flag);
    porting::os::thread_exit(data);
    T_W("DHCP6C_EVENT_EXIT");
}

extern "C" int dhcp6_client_icli_cmd_register();

mesa_rc client_init(vtss_init_data_t *data)
{
    dhcp6c_stkmsg_id_t idx;
    vtss_isid_t        isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_I("INIT");
        critd_init(&DHCP6C_globals.crit, "dhcp6_client",
                   VTSS_MODULE_ID_DHCP6C,
                   CRITD_TYPE_MUTEX);

        critd_init(&DHCP6C_globals.rx_crit, "dhcp6_client_rx",
                   VTSS_MODULE_ID_DHCP6C,
                   CRITD_TYPE_MUTEX);

        critd_init(&DHCP6C_globals.nm_crit, "dhcp6_client_msg",
                   VTSS_MODULE_ID_DHCP6C,
                   CRITD_TYPE_MUTEX);

        /* Initialize stacking message buffers */
        vtss_sem_init(&DHCP6C_globals.stksem, 1);
        for (idx = DHCP6C_STKMSG_ID_CFG_DEFAULT; idx < DHCP6C_STKMSG_MAX_ID; ++idx) {
            switch ( idx ) {
            case DHCP6C_STKMSG_ID_CFG_DEFAULT:
                DHCP6C_globals.stkmsize[idx] = sizeof(dhcp6c_stkmsg_cfg_default_req_t);
                break;
            case DHCP6C_STKMSG_ID_GLOBAL_PURGE_REQ:
                DHCP6C_globals.stkmsize[idx] = sizeof(dhcp6c_stkmsg_purge_req_t);
                break;
            case DHCP6C_STKMSG_ID_SYS_MGMT_SET_REQ:
            default:
                /* Give the MAX */
                DHCP6C_globals.stkmsize[idx] = sizeof(dhcp6c_stkmsg_sysmgmt_set_req_t);
                break;
            }

            if (VTSS_MALLOC_CAST(DHCP6C_globals.stkmsg[idx], DHCP6C_globals.stkmsize[idx]) == NULL) {
                T_E("DHCP6C_ASSERT(INIT_CMD_INIT)");
                for (;;) {}
            }
        }

        if (!vtss_bip_buffer_init(&DHCP6C_bip, DHCP6_BIP_BUF_SZ_B)) {
            T_E("vtss_bip_buffer_init failed!");
        }

#ifdef VTSS_SW_OPTION_ICFG
        if (dhcp6_client_icfg_init() != VTSS_RC_OK) {
            T_E("dhcp6_client_icfg_init failed!");
        }
#endif /* VTSS_SW_OPTION_ICFG */

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        vtss_appl_dhcp6_client_mib_init();  /* Register DHCPv6 Client Private-MIB */
#endif /* VTSS_SW_OPTION_SNMP */
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_dhcp6_client_json_init(); /* Register DHCPv6 Client JSON-RPC */
#endif /* VTSS_SW_OPTION_JSON_RPC */
        dhcp6_client_icli_cmd_register();
        vtss::dhcp6::client::initialize(DHCP6C_MAX_INTERFACES);
        break;

    case INIT_CMD_START:
        T_I("START");
        DHCP6C_CRIT_ENTER();
        utils::system_mac_clr();
        utils::device_mac_clr();
        DHCP6C_CRIT_EXIT();
        DHCP6C_MSG_CRIT_ENTER();
        vtss_clear(dhcp6c_msg);
        DHCP6C_MSG_CRIT_EXIT();
        (void) DHCP6C_conf_default();
        porting::os::thread_create(
            dhcp6c_thread,
            "DHCP6C",
            nullptr,
            0,
            &DHCP6C_thread_handle,
            &DHCP6C_thread_block);
        break;

    case INIT_CMD_CONF_DEF:
        T_I("CONF_DEF->isid: %u", isid);
        if (isid == VTSS_ISID_GLOBAL) {
            (void)DHCP6C_conf_default();

            if (msg_switch_is_primary()) {
                porting::os::event_set(&DHCP6C_thread_flag, DHCP6C_EVENT_DEFAULT);
            }
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_I("ICFG_LOADING_PRE");
        porting::os::event_set(&DHCP6C_thread_flag, DHCP6C_EVENT_RESUME);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_I("ICFG_LOADING_POST");
        if (!msg_switch_is_local(isid)) {
            DHCP6C_CRIT_ENTER();
            DHCP6C_globals.switch_event_value[isid] |= DHCP6C_EVENT_ICFG_LOADING_POST;
            DHCP6C_CRIT_EXIT();
        }
        porting::os::event_set(&DHCP6C_thread_flag, DHCP6C_EVENT_ICFG_LOADING_POST);
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

} /* namespace dhcp6c */
} /* namespace vtss */

/*****************************************************************************
    Public API section for DHCPv6 Client
    from vtss_appl/include/vtss/appl/dhcp6_client.h
*****************************************************************************/
/**
 * \brief Get the capabilities of DHCPv6 client.
 *
 * \param cap       [OUT]   The capability properties of the DHCPv6 client module.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_capabilities_get(
    vtss_appl_dhcp6c_capabilities_t *const cap
)
{
    if (!cap) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    cap->max_number_of_interfaces = DHCP6C_MAX_INTERFACES;

    return VTSS_RC_OK;
}

#define DHCP6C_EXPIRED_TIME(w, x, y, z) do {   \
if ((w)) {                                     \
    (y) = VTSS_OS_MSEC2TICK((y) * 1000);       \
    (y) += (x);                                \
    if ((z) < (y)) {                           \
        (y) -= (z);                            \
    } else {                                   \
        (y) = 0;                               \
    }                                          \
    (y) = VTSS_OS_TICK2MSEC(y);                \
    (w)->seconds = (y) / 1000;                 \
    (w)->nanoseconds = ((y) % 1000) * 1000000; \
}                                              \
} while (0)

/**
 * \brief Iterator for retrieving DHCPv6 client interface table key/index
 *
 * To walk information (configuration and status) index of the DHCPv6 client interface.
 *
 * \param prev      [IN]    Interface index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_itr(
    const vtss_ifindex_t                *const prev,
    vtss_ifindex_t                      *const next
)
{
    mesa_vid_t          vidx;
    dhcp6c_conf_intf_t  *intf;

    if (!next || !msg_switch_is_primary()) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    if (VTSS_MALLOC_CAST(intf, sizeof(dhcp6c_conf_intf_t)) == NULL) {
        T_D("Not enough memory!");
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    vidx = VTSS_VID_NULL;
    if (prev && *prev >= VTSS_IFINDEX_VLAN_OFFSET) {
        vtss_ifindex_elm_t  ife;

        /* get next */
        T_D("Find next IfIndex");

        /* get VID from given IfIndex */
        if (vtss_ifindex_decompose(*prev, &ife) != VTSS_RC_OK ||
            ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
            T_D("Failed to decompose IfIndex %u!\n\r", VTSS_IFINDEX_PRINTF_ARG(*prev));
            VTSS_FREE(intf);
            return VTSS_APPL_DHCP6C_ERROR_PARM;
        }

        vidx = (mesa_vid_t)ife.ordinal;
    } else {
        /* get first */
        T_D("Find first IfIndex");
    }

    if (vtss::dhcp6c::DHCP6C_intf_conf_itr(vidx, intf) != VTSS_RC_OK) {
        T_D("No next VIDX w.r.t %u", vidx);
        VTSS_FREE(intf);
        return VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND;
    } else {
        vidx = intf->index;
    }

    VTSS_FREE(intf);
    T_I("Found next VIDX-%u", vidx);
    /* convert VIDX to IfIndex */
    if (vtss_ifindex_from_vlan(vidx, next) != VTSS_RC_OK) {
        T_D("Failed to convert IfIndex from %u!\n\r", vidx);
        return VTSS_APPL_DHCP6C_ERROR_ENTRY_INVALID;
    }

    DHCP6C_PRIV_TD_RETURN(VTSS_RC_OK);
}

/**
 * \brief Get DHCPv6 client IP interface default configuration
 *
 * To get default configuration of the DHCPv6 client interface.
 *
 * \param entry     [OUT]   The default configuration of the DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_default(
    vtss_appl_dhcp6c_intf_conf_t        *const entry
)
{
    if (!entry) {
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    entry->rapid_commit = DHCP6C_CONF_DEF_RAPID_COMMIT;

    return VTSS_RC_OK;
}

/**
 * \brief Get DHCPv6 client specific IP interface configuration
 *
 * To get configuration of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 *
 * \param entry     [OUT]   The current configuration of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_get(
    const vtss_ifindex_t                *const ifindex,
    vtss_appl_dhcp6c_intf_conf_t        *const entry
)
{
    vtss_ifindex_elm_t  ife;
    mesa_vid_t          vidx;
    dhcp6c_conf_intf_t  *intf;
    mesa_rc             rc;

    if (!ifindex || !entry || !msg_switch_is_primary() ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    if (VTSS_MALLOC_CAST(intf, sizeof(dhcp6c_conf_intf_t)) == NULL) {
        T_D("Not enough memory!");
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    vidx = (mesa_vid_t)ife.ordinal;
    if ((rc = vtss::dhcp6c::DHCP6C_intf_conf_get(vidx, intf)) != VTSS_RC_OK) {
        T_D("No such VIDX %u in DHCPv6C", vidx);
        VTSS_FREE(intf);
        return rc;
    }

    entry->rapid_commit = intf->rapid_commit;

    VTSS_FREE(intf);
    DHCP6C_PRIV_TD_RETURN(rc);
}

/**
 * \brief Add DHCPv6 client specific IP interface configuration
 *
 * To create configuration of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 * \param entry     [IN]    The revised configuration of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_add(
    const vtss_ifindex_t                *const ifindex,
    const vtss_appl_dhcp6c_intf_conf_t  *const entry
)
{
    vtss_ifindex_elm_t  ife;
    mesa_vid_t          vidx;
    dhcp6c_conf_intf_t  *intf;
    mesa_rc             rc;

    if (!ifindex || !entry || !msg_switch_is_primary() ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }
    vidx = (mesa_vid_t)ife.ordinal;

#if defined(VTSS_SW_OPTION_IP)
    if (!vtss_appl_ip_if_exists(*ifindex)) {
        T_D("No such VIDX %u IP interface", vidx);
        return VTSS_APPL_DHCP6C_ERROR_IPIF_NOT_EXISTING;
    }
#endif /* defined(VTSS_SW_OPTION_IP) */

    if (VTSS_MALLOC_CAST(intf, sizeof(dhcp6c_conf_intf_t)) == NULL) {
        T_D("Not enough memory!");
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    if ((rc = vtss::dhcp6c::DHCP6C_intf_conf_get(vidx, intf)) == VTSS_RC_OK) {
        T_D("Existing such VIDX %u in DHCPv6C", vidx);
        VTSS_FREE(intf);
        return VTSS_APPL_DHCP6C_ERROR_ENTRY_EXISTING;
    }

    memset(intf, 0x0, sizeof(dhcp6c_conf_intf_t));
    intf->index = vidx;
    intf->rapid_commit = entry->rapid_commit;
    intf->dhcp6 = TRUE;
    rc = vtss::dhcp6c::DHCP6C_intf_conf_set(DHCP6_CONF_OP_ADD, vidx, intf);

    VTSS_FREE(intf);
    DHCP6C_PRIV_TD_RETURN(rc);
}

/**
 * \brief Set/Update DHCPv6 client specific IP interface configuration
 *
 * To modify configuration of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 * \param entry     [IN]    The revised configuration of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_set(
    const vtss_ifindex_t                *const ifindex,
    const vtss_appl_dhcp6c_intf_conf_t  *const entry
)
{
    vtss_ifindex_elm_t  ife;
    mesa_vid_t          vidx;
    dhcp6c_conf_intf_t  *intf;
    mesa_rc             rc;

    if (!ifindex || !entry || !msg_switch_is_primary() ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    if (VTSS_MALLOC_CAST(intf, sizeof(dhcp6c_conf_intf_t)) == NULL) {
        T_D("Not enough memory!");
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    vidx = (mesa_vid_t)ife.ordinal;
    if ((rc = vtss::dhcp6c::DHCP6C_intf_conf_get(vidx, intf)) != VTSS_RC_OK) {
        T_D("No such VIDX %u in DHCPv6C", vidx);
        VTSS_FREE(intf);
        return VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND;
    }

    intf->index = vidx;
    intf->rapid_commit = entry->rapid_commit;
    intf->dhcp6 = TRUE;
    rc = vtss::dhcp6c::DHCP6C_intf_conf_set(DHCP6_CONF_OP_UPD, vidx, intf);

    VTSS_FREE(intf);
    DHCP6C_PRIV_TD_RETURN(rc);
}

/**
 * \brief Delete DHCPv6 client specific IP interface configuration
 *
 * To remove configuration of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_del(
    const vtss_ifindex_t                *const ifindex
)
{
    vtss_ifindex_elm_t  ife;
    mesa_vid_t          vidx;
    dhcp6c_conf_intf_t  *intf;
    mesa_rc             rc;

    if (!ifindex || !msg_switch_is_primary() ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    if (VTSS_MALLOC_CAST(intf, sizeof(dhcp6c_conf_intf_t)) == NULL) {
        T_D("Not enough memory!");
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    vidx = (mesa_vid_t)ife.ordinal;
    if ((rc = vtss::dhcp6c::DHCP6C_intf_conf_get(vidx, intf)) != VTSS_RC_OK) {
        T_D("No such VIDX %u in DHCPv6C", vidx);
        VTSS_FREE(intf);
        return VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND;
    }

    rc = vtss::dhcp6c::DHCP6C_intf_conf_set(DHCP6_CONF_OP_DEL, vidx, intf);

    VTSS_FREE(intf);
    DHCP6C_PRIV_TD_RETURN(rc);
}

/**
 * \brief Get DHCPv6 client specific IP interface stsatus
 *
 * To get running status of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 *
 * \param entry     [OUT]   The running status of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_status_get(
    const vtss_ifindex_t                *const ifindex,
    vtss_appl_dhcp6c_interface_t        *const entry
)
{
    vtss_ifindex_elm_t  ife;
    mesa_vid_t          vidx;
    Dhcp6cInterface     *ifce;
    mesa_rc             rc;

    if (!entry || !ifindex || !msg_switch_is_primary() ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    if (VTSS_MALLOC_CAST(ifce, sizeof(Dhcp6cInterface)) == NULL) {
        T_D("Not enough memory!");
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    vidx = (mesa_vid_t)ife.ordinal;
    if ((rc = vtss::dhcp6c::dhcp6_client_interface_get(vidx, ifce)) != VTSS_RC_OK) {
        T_D("Cannot determine VIDX %u status in DHCPv6C", vidx);
    } else {
        vtss_tick_count_t  tc, ts = vtss_current_time();

        DHCP6_ADRS_CPY(&entry->address, &ifce->address);
        DHCP6_ADRS_CPY(&entry->srv_addr, &ifce->srv_addr);
        DHCP6_ADRS_CPY(&entry->dns_srv_addr, &ifce->dns_srv_addr);

        /* calculate expired time */
        memset(&entry->timers, 0x0, sizeof(vtss_appl_dhcp6c_intf_timer_t));
        tc = ifce->t1;
        DHCP6C_EXPIRED_TIME(&entry->timers.t1, ifce->refresh_ts, tc, ts);
        tc = ifce->t2;
        DHCP6C_EXPIRED_TIME(&entry->timers.t2, ifce->refresh_ts, tc, ts);
        if ((tc = ifce->preferred_lifetime) != VTSS_DHCP6_LIFETIME_INFINITY) {
            DHCP6C_EXPIRED_TIME(&entry->timers.preferred_lifetime, ifce->refresh_ts, tc, ts);
        } else {
            entry->timers.preferred_lifetime.seconds = VTSS_DHCP6_LIFETIME_INFINITY;
            entry->timers.preferred_lifetime.nanoseconds = VTSS_DHCP6_LIFETIME_INFINITY;
        }
        if ((tc = ifce->valid_lifetime) != VTSS_DHCP6_LIFETIME_INFINITY) {
            DHCP6C_EXPIRED_TIME(&entry->timers.valid_lifetime, ifce->refresh_ts, tc, ts);
        } else {
            entry->timers.valid_lifetime.seconds = VTSS_DHCP6_LIFETIME_INFINITY;
            entry->timers.valid_lifetime.nanoseconds = VTSS_DHCP6_LIFETIME_INFINITY;
        }

        DHCP6C_INTF_CNTR_ASSIGN(&entry->counters, &ifce->cntrs);
    }

    VTSS_FREE(ifce);
    DHCP6C_PRIV_TD_RETURN(rc);
}

/**
 * \brief Get DHCPv6 client specific IP interface statistics
 *
 * To get DHCPv6 control message statistics of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 *
 * \param entry     [OUT]   The packet counter of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_statistics_get(
    const vtss_ifindex_t                *const ifindex,
    vtss_appl_dhcp6c_intf_cntr_t        *const entry
)
{
    vtss_ifindex_elm_t  ife;
    mesa_vid_t          vidx;
    Dhcp6cInterface     *ifce;
    mesa_rc             rc;

    if (!entry || !ifindex || !msg_switch_is_primary() ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    if (VTSS_MALLOC_CAST(ifce, sizeof(Dhcp6cInterface)) == NULL) {
        T_D("Not enough memory!");
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    vidx = (mesa_vid_t)ife.ordinal;
    if ((rc = vtss::dhcp6c::dhcp6_client_interface_get(vidx, ifce)) != VTSS_RC_OK) {
        T_D("Cannot determine VIDX %u status in DHCPv6C", vidx);
    } else {
        DHCP6C_INTF_CNTR_ASSIGN(entry, &ifce->cntrs);
    }

    VTSS_FREE(ifce);
    DHCP6C_PRIV_TD_RETURN(rc);
}

/**
 * \brief Control action for restarting DHCPv6 client on a specific IP interface
 *
 * To restart the DHCPv6 client service on a specific IP interface.
 *
 * \param ifindex   [IN]    Interface index - the logical interface index of the IP interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_restart_act(
    const vtss_ifindex_t                *const ifindex
)
{
    vtss_ifindex_elm_t  ife;
    mesa_vid_t          vidx;
    dhcp6c_conf_intf_t  *intf;
    mesa_rc             rc;

    if (!ifindex || !msg_switch_is_primary()) {
        T_D("Invalid Input!");
        return VTSS_APPL_DHCP6C_ERROR_PARM;
    }

    if (*ifindex == VTSS_IFINDEX_VLAN_OFFSET) {
        T_D("Do nothing for VTSS_IFINDEX_VLAN_OFFSET(%u)!", VTSS_IFINDEX_PRINTF_ARG(*ifindex));
        return VTSS_RC_OK;
    } else {
        if (vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
            ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
            T_D("Invalid IfIndex(%u)!", VTSS_IFINDEX_PRINTF_ARG(*ifindex));
            return VTSS_APPL_DHCP6C_ERROR_PARM;
        }
    }

    if (VTSS_MALLOC_CAST(intf, sizeof(dhcp6c_conf_intf_t)) == NULL) {
        T_D("Not enough memory!");
        return VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
    }

    vidx = (mesa_vid_t)ife.ordinal;
    if ((rc = vtss::dhcp6c::DHCP6C_intf_conf_get(vidx, intf)) != VTSS_RC_OK) {
        T_D("No such VIDX %u in DHCPv6C", vidx);
        VTSS_FREE(intf);
        return VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND;
    }

    rc = vtss::dhcp6c::DHCP6C_intf_restart(vidx);

    VTSS_FREE(intf);
    DHCP6C_PRIV_TD_RETURN(rc);
}

extern "C" {    /* For C Access */
    BOOL dhcp6_client_if_exists(mesa_vid_t vidx)
    {
        dhcp6c_conf_intf_t  *intf;
        mesa_rc             rc;

        if (VTSS_MALLOC_CAST(intf, sizeof(dhcp6c_conf_intf_t)) != NULL) {
            rc = vtss::dhcp6c::DHCP6C_intf_conf_get(vidx, intf);
            VTSS_FREE(intf);
        } else {
            rc = VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
        }

        T_N("Check VID: %u -> %s", vidx, error_txt(rc));
        return  (rc == VTSS_RC_OK);
    }

    const char *dhcp6_client_error_txt(mesa_rc rc)
    {
        switch ( rc ) {
        case VTSS_APPL_DHCP6C_ERROR_GEN:
            return "DHCP6C: Generic error";
        case VTSS_APPL_DHCP6C_ERROR_PARM:
            return "DHCP6C: Invalid given parameter";
        case VTSS_APPL_DHCP6C_ERROR_IPIF_NOT_EXISTING:
            return "DHCP6C: IP interface is not created in advance";
        case VTSS_APPL_DHCP6C_ERROR_TABLE_FULL:
            return "DHCP6C: Table is full";
        case VTSS_APPL_DHCP6C_ERROR_MEMORY_NG:
            return "DHCP6C: Memory access failure";
        case VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND:
            return "DHCP6C: Entry is not found";
        case VTSS_APPL_DHCP6C_ERROR_ENTRY_INVALID:
            return "DHCP6C: Entry is invalid";
        case VTSS_APPL_DHCP6C_ERROR_ENTRY_EXISTING:
            return "DHCP6C: Entry is existing";
        case VTSS_APPL_DHCP6C_ERROR_PKT_GEN:
            return "DHCP6C: Control frame error";
        case VTSS_APPL_DHCP6C_ERROR_PKT_CONTENT:
            return "DHCP6C: Incorrect packet content";
        case VTSS_APPL_DHCP6C_ERROR_PKT_FORMAT:
            return "DHCP6C: Incorrect packet format";
        case VTSS_APPL_DHCP6C_ERROR_PKT_ADDRESS:
            return "DHCP6C: Incorrect address used in packet";
        default:
            return "DHCP6C: Unknown Error!";
        }
    }

    mesa_rc dhcp6_client_init(vtss_init_data_t *data)
    {
        if (!data) {
            return VTSS_APPL_DHCP6C_ERROR_GEN;
        }

        return vtss::dhcp6c::client_init(data);
    }
}
