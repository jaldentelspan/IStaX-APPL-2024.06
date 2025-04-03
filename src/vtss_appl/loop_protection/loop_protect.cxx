/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss_os_wrapper.h"
#include "loop_protect.h"

#include "main.h"
#include "conf_api.h"
#include "misc_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "packet_api.h"
#include "acl_api.h"
#include "loop_protect.h"
#include "msg_api.h"
#include "topo_api.h"
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif
#ifdef VTSS_SW_OPTION_LOOP_DETECT
#include "vtss_lb_api.h"
#endif
#ifdef VTSS_SW_OPTION_ICFG
#include "loop_protect_icfg.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_LOOP_PROTECT

/* ================================================================= *
 *  LOOP_PROTECT module private data
 * ================================================================= */

/* State and config */
static lprot_conf_t lprot_conf;

typedef struct {
    uint port;                  /* Port_no or aggr_id */
    int transmit_timer;
    int shutdown_timer;
    u32 last_disable;
    int ttl_timer;
    vtss_appl_loop_protect_port_info_t info;
} lprot_port_state_t;

typedef struct {
    u32 last_refresh;
} lprot_switch_state_t;

static struct {
    vtss_usid_t usid;                      /* My usid */
    u8 primary_switch_mac[6];              /* MAC from the primary switch */
    u8 primary_switch_key[SHA1_HASH_SIZE]; /* Stack key */
    lprot_switch_state_t switches[VTSS_ISID_END];
    CapArray<lprot_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> ports;
} lprot_state;

static struct {
    CapArray<vtss_appl_loop_protect_port_info_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> ports;
} lprot_primary_switch_state[VTSS_ISID_END];

/* Frame data */
static const u8  dmac[6] = {0x01, 0x01, 0xc1, 0x00, 0x00, 0x00}; /* 01-01-c1-00-00-00 */
static       u8  switchmac[6];
static const u16 etype = 0x9003;
static void *loop_filter_id;

/* Thread variables */
static critd_t       lprot_crit;          /* module critical section */
static vtss_flag_t   lprot_control_flags; /* thread control */
static vtss_flag_t   lprot_status_flags;
static vtss_handle_t lprot_thread_handle;
static vtss_thread_t lprot_thread_block;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "lprot", "Loop Protection"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define LPROT_CRIT_ENTER()         critd_enter(        &lprot_crit, __FILE__, __LINE__)
#define LPROT_CRIT_EXIT()          critd_exit(         &lprot_crit, __FILE__, __LINE__)
#define LPROT_CRIT_ASSERT_LOCKED() critd_assert_locked(&lprot_crit, __FILE__, __LINE__)

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

static void lprot_enable(lprot_port_state_t *pstate);

static loop_protect_msg_t *
loop_protect_alloc_message(loop_protect_msg_id_t msg_id)
{
    loop_protect_msg_t *msg;
    VTSS_MALLOC_CAST(msg, sizeof(loop_protect_msg_t));
    if(msg)
        msg->msg_id = msg_id;
    T_N("msg type %d => %p", msg_id, msg);
    return msg;
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* LOOP_PROTECT error text */
const char *loop_protect_error_txt(mesa_rc rc)
{
    switch (rc) {
    case LOOP_PROTECT_ERROR_GEN:
        return "LOOP_PROTECT generic error";
    case LOOP_PROTECT_ERROR_PARM:
        return "LOOP_PROTECT parameter error";
    case LOOP_PROTECT_ERROR_TIMEOUT:
        return "LOOP_PROTECT timeout error";
    case LOOP_PROTECT_ERROR_MSGALLOC:
        return "LOOP_PROTECT message allocation error";
    default:
        return "LOOP_PROTECT unknown error";
    }
}

static void loop_protect_redir_cpu(void)
{
    static BOOL acl_installed;
    acl_entry_conf_t conf;
    port_iter_t pit;
    uint ports = 0;

    /* Init ACL structure */
    (void) acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, &conf);

    /* Count how many ports are actively protected */
    if(lprot_conf.global.enabled) {
        (void) port_iter_init(&pit, NULL, VTSS_ISID_LOCAL,
                              PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        memset(conf.port_list, 0, sizeof(conf.port_list)); /* Apply to none by default */
        while (port_iter_getnext(&pit)) {
            const vtss_appl_loop_protect_port_conf_t *pconf = &lprot_conf.ports[VTSS_ISID_LOCAL][pit.iport];
            if(pconf->enabled) { /* This port active? */
                ports++;
                conf.port_list[pit.iport] = TRUE;
            }
        }
    }

    /* Install rule if any ports are active */
    if(ports) {
        conf.id = LPROT_ACE_ID;
        conf.isid = VTSS_ISID_LOCAL;
        conf.action.force_cpu = TRUE;
        conf.action.cpu_queue = PACKET_XTR_QU_BPDU; /* High! */
        /* "Filter" action */
        conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        /* Mask/filter out all egress ports */
        memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
        memcpy(conf.frame.etype.dmac.value, dmac, sizeof(conf.frame.etype.dmac.value));
        memset(conf.frame.etype.dmac.mask, 0xFF, sizeof(conf.frame.etype.dmac.mask));
        memcpy(conf.frame.etype.smac.value, lprot_state.primary_switch_mac, sizeof(conf.frame.etype.smac.value));
        memset(conf.frame.etype.smac.mask, 0xFF, sizeof(conf.frame.etype.smac.mask));
        memset(conf.frame.etype.etype.mask, 0xFF, sizeof(conf.frame.etype.etype.mask));
        conf.frame.etype.etype.value[0] = (etype >> 8);
        conf.frame.etype.etype.value[1] = etype & 0xff;
        T_D("Setting up ACL redirect for %u ports", ports);
        if (acl_mgmt_ace_add(ACL_USER_LOOP_PROTECT, ACL_MGMT_ACE_ID_NONE, &conf) == VTSS_RC_OK)
            acl_installed = TRUE;
        else
            T_E("loop_protect_redir_cpu() failed");
    } else {
        /* Delete old ACL rule */
        if(acl_installed) {
            T_D("Delete ACL redirect ID = %d", LPROT_ACE_ID);
            if(acl_mgmt_ace_del(ACL_USER_LOOP_PROTECT, LPROT_ACE_ID) != VTSS_RC_OK)
                T_E("loop_protect_redir_cpu() ACL delete failed");
            acl_installed = FALSE;
        }
    }
}

static void lprot_conf_apply_local(void)
{
    port_iter_t pit;
    (void) port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    LPROT_CRIT_ENTER();
    while (port_iter_getnext(&pit)) {
        /* Enable disabled ports that are no longer configured protected */
        if(lprot_state.ports[pit.iport].info.loop_detect) {
            if (lprot_conf.global.shutdown_time != 0) {
                T_D("Enabling port %d", iport2uport(pit.iport));
                lprot_enable(&lprot_state.ports[pit.iport]);
            } else {
                T_D("Not enabling port %d because shutdown_time == 0", iport2uport(pit.iport));
            }
        }
    }
    loop_protect_redir_cpu();
    LPROT_CRIT_EXIT();
}

static void loop_protect_conf_send_port(vtss_isid_t isid, mesa_port_no_t port_no)
{
    loop_protect_msg_t *msg = loop_protect_alloc_message(LOOP_PROTECT_MSG_ID_CONF_PORT);
    if(msg) {
        msg->data.port_conf.port_no = port_no;
        msg->data.port_conf.port_conf = lprot_conf.ports[isid][port_no];
        msg_tx(VTSS_MODULE_ID_LOOP_PROTECT, isid, msg, sizeof(*msg));
    }
}

static void loop_protect_conf_send(vtss_isid_t isid)
{
    loop_protect_msg_t *msg = loop_protect_alloc_message(LOOP_PROTECT_MSG_ID_CONF);
    if(msg) {
        LPROT_CRIT_ENTER();
        memcpy(msg->data.unit_conf.mac, switchmac, sizeof(switchmac));
        msg->data.unit_conf.usid = topo_isid2usid(isid);
        msg->data.unit_conf.global_conf = lprot_conf.global;
        LPROT_CRIT_EXIT();
        msg_tx(VTSS_MODULE_ID_LOOP_PROTECT, isid, msg, sizeof(*msg));
    }
}

static void lprot_conf_apply(void)
{
    switch_iter_t sit;

    T_N("enter");
    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        loop_protect_conf_send(sit.isid);
    }
    T_N("exit");
}

/* Set Loop_Protect General configuration  */
mesa_rc vtss_appl_loop_protect_conf_set(const vtss_appl_loop_protect_conf_t *conf)
{
    mesa_rc rc;
    T_N("enter");

    if (conf->transmission_time < 1 || conf->transmission_time > 10 ||     /* 1-10 */
        conf->shutdown_time     > 604800) { /* 0-604800 */
        return LOOP_PROTECT_ERROR_PARM;
    }

    LPROT_CRIT_ENTER();
    lprot_conf.global = *conf;    /* Move new data to database */
    LPROT_CRIT_EXIT();

    lprot_conf_apply();         /* Apply config */
    rc = VTSS_RC_OK;

    T_N("exit");
    return rc;
}

/* Get Loop_Protect General configuration  */
mesa_rc vtss_appl_loop_protect_conf_get(vtss_appl_loop_protect_conf_t *conf)
{
    LPROT_CRIT_ENTER();
    *conf = lprot_conf.global; // Get the 'real' configuration
    LPROT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/* Set port Loop_Protect configuration  */
mesa_rc vtss_appl_loop_protect_conf_port_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_loop_protect_port_conf_t *conf)
{
    if (conf->action > VTSS_APPL_LOOP_PROTECT_ACTION_LOG_ONLY) {
        return VTSS_RC_OK;
    }

    LPROT_CRIT_ENTER();
    if (isid < VTSS_ISID_END && port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        BOOL changed = memcmp(&lprot_conf.ports[isid][port_no], conf, sizeof(*conf)) != 0;
        if (changed) {
            lprot_conf.ports[isid][port_no] = *conf;
            loop_protect_conf_send_port(isid, port_no);
        }
    }

    LPROT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/* Get port Loop_Protect configuration  */
mesa_rc vtss_appl_loop_protect_conf_port_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_loop_protect_port_conf_t *conf)
{
    mesa_rc rc = LOOP_PROTECT_ERROR_PARM;

    if (isid < VTSS_ISID_END && port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        LPROT_CRIT_ENTER();
        *conf = lprot_conf.ports[isid][port_no];
        LPROT_CRIT_EXIT();
        rc = VTSS_RC_OK;
    }

    return rc;
}

/* Get Loop_Protect port info  */
mesa_rc vtss_appl_loop_protect_port_info_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_loop_protect_port_info_t *info)
{
    mesa_rc rc = LOOP_PROTECT_ERROR_PARM;

    LPROT_CRIT_ENTER();
    if (isid < VTSS_ISID_END && port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        if (lprot_conf.ports[isid][port_no].enabled) {
            if (lprot_state.switches[isid].last_refresh != time(NULL)) {
                loop_protect_msg_t *msg = loop_protect_alloc_message(LOOP_PROTECT_MSG_ID_PORT_STATUS_REQ);
                if (msg) {
                    vtss_flag_value_t flag = (1<<isid);
                    vtss_flag_t *flags = &lprot_status_flags;
                    vtss_tick_count_t wtime = vtss_current_time() + VTSS_OS_MSEC2TICK(3000);
                    msg_tx(VTSS_MODULE_ID_LOOP_PROTECT, isid, msg, sizeof(*msg));
                    vtss_flag_maskbits(flags, ~flag);
                    LPROT_CRIT_EXIT();
                    if (vtss_flag_timed_wait(flags, flag, VTSS_FLAG_WAITMODE_OR, wtime) & flag) {
                        LPROT_CRIT_ENTER();
                        *info = lprot_primary_switch_state[isid].ports[port_no];
                        rc = VTSS_RC_OK;
                    } else {
                        LPROT_CRIT_ENTER();
                        rc = LOOP_PROTECT_ERROR_TIMEOUT;
                    }
                }
            } else {
                /* Up-to date info */
                *info = lprot_primary_switch_state[isid].ports[port_no];
                rc = VTSS_RC_OK;
            }
        } else {
            rc = LOOP_PROTECT_ERROR_INACTIVE; /* Not active */
        }
    }

    LPROT_CRIT_EXIT();

    return rc;
}

const char *loop_protect_action2string(vtss_appl_loop_protect_action_t action)
{
    static const char * const action_string[] = {
        [VTSS_APPL_LOOP_PROTECT_ACTION_SHUTDOWN] = "Shutdown",
        [VTSS_APPL_LOOP_PROTECT_ACTION_SHUT_LOG] = "Shutdown+Log",
        [VTSS_APPL_LOOP_PROTECT_ACTION_LOG_ONLY] = "Log Only",
    };
    if(action <= VTSS_APPL_LOOP_PROTECT_ACTION_LOG_ONLY)
        return action_string[action];
    return "Undefined";
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

static void
loop_protect_conf_default(void)
{
    uint i, j;

    /* Use default configuration */
    lprot_conf.global.enabled           = VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_ENABLED;       /* Disabled */
    lprot_conf.global.transmission_time = VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_TX_TIME;       /* 5 seconds */
    lprot_conf.global.shutdown_time     = VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_SHUTDOWN_TIME; /* 3 minutes */
    for(i = 0; i < VTSS_ISID_END; i++) {
        for(j = 0; j < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); j++) {
            lprot_conf.ports[i][j].enabled  = VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_ENABLED;
            lprot_conf.ports[i][j].action   = VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_ACTION;
            lprot_conf.ports[i][j].transmit = VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_TX_MODE;
        }
    }

    /* Apply config */
    lprot_conf_apply();
}

static void lprot_port_disable_primary_switch(mesa_port_no_t port_no, BOOL dis)
{
    port_vol_conf_t conf;
    port_user_t     user = PORT_USER_LOOP_PROTECT;

    T_D("%sable port: %u", dis ? "dis" : "en", port_no);
    if (port_vol_conf_get(user, port_no, &conf) == VTSS_RC_OK &&
        conf.disable != dis) {
        conf.disable = dis;
        if (port_vol_conf_set(user, port_no, &conf) != VTSS_RC_OK)
            T_E("port_vol_conf_set(%u) failed", port_no);
    }
}

static BOOL lprot_disable_ratelimit(lprot_port_state_t *pstate)
{
    u32 now = time(NULL);
    if (lprot_state.ports[pstate->port].last_disable != now) {
        lprot_state.ports[pstate->port].last_disable = now;
        return TRUE;
    }
    return FALSE;
}

static void lprot_port_disable(mesa_port_no_t port_no, BOOL disable)
{
    loop_protect_msg_t *msg = loop_protect_alloc_message(LOOP_PROTECT_MSG_ID_PORT_CTL);
    if(msg) {
        msg->data.port_ctl.port_no = port_no;
        msg->data.port_ctl.disable = disable;
        T_I("%sable port %d", disable ? "dis" : "en", iport2uport(port_no));
        msg_tx(VTSS_MODULE_ID_LOOP_PROTECT, 0, msg, sizeof(*msg));
        if (disable) {
            lprot_state.ports[port_no].last_disable = time(NULL);
        }
    } else {
        T_W("Unable to %sable port %d", disable ? "dis" : "en", iport2uport(port_no));
    }
}

static void lprot_enable(lprot_port_state_t *pstate)
{
    T_I("Re-enable %s %d", "port", iport2uport(pstate->port));
    pstate->info.disabled = FALSE;
    pstate->info.loop_detect = FALSE;
    pstate->transmit_timer = 0;
    lprot_port_disable(pstate->port, FALSE);
}

static void lprot_disable(lprot_port_state_t *pstate)
{
    const vtss_appl_loop_protect_port_conf_t *pconf;
    T_W("Loop detected: %s %d", "Port", iport2uport(pstate->port));
    pconf = &lprot_conf.ports[VTSS_ISID_LOCAL][pstate->port];
    if(pconf->action == VTSS_APPL_LOOP_PROTECT_ACTION_SHUTDOWN ||
       pconf->action == VTSS_APPL_LOOP_PROTECT_ACTION_SHUT_LOG) {
        pstate->shutdown_timer = 0;
        pstate->info.disabled = TRUE;
        if (lprot_disable_ratelimit(pstate)) {
            lprot_port_disable(pstate->port, TRUE);
        } else {
            T_N("Skip disable, already sent this very second, port %d", pstate->port);
        }
    }

#if defined(VTSS_SW_OPTION_SYSLOG)
    if(pconf->action == VTSS_APPL_LOOP_PROTECT_ACTION_SHUT_LOG ||
       pconf->action == VTSS_APPL_LOOP_PROTECT_ACTION_LOG_ONLY) {
        if(!pstate->info.loop_detect) { /* Only 1st time detected */
            char buf[256], *p = &buf[0];
            p += sprintf(p, "LOOP_PROTECTION-LOOP_BACK_DETECTED: Loop-back detected on ");
            p += sprintf(p, "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
            p += sprintf(p, "%s.",  pconf->action == VTSS_APPL_LOOP_PROTECT_ACTION_SHUT_LOG ? ", shut down" : "");
            S_PORT_W(VTSS_ISID_LOCAL, pstate->port, "%s", buf);
        }
    }
#endif /* VTSS_SW_OPTION_SYSLOG */

    pstate->info.loops++;
    pstate->info.last_loop = time(NULL);
    pstate->info.loop_detect = TRUE;
}

static BOOL lprot_egress_filter(mesa_port_no_t egress_port)
{
    mesa_vlan_port_conf_t pconf;
    mesa_packet_filter_t     filter;
    mesa_packet_frame_info_t info;

    if (mesa_vlan_port_conf_get(NULL, egress_port, &pconf) != VTSS_RC_OK) {
        T_E("Unable to get port vlan config for port %d", iport2uport(egress_port));
        return FALSE;
    }

    mesa_packet_frame_info_init(&info);
    info.vid = pconf.pvid;
    info.port_tx = egress_port; // tx filtering
    info.aggr_tx_disable = TRUE;

    if (mesa_packet_frame_filter(NULL, &info, &filter) == VTSS_RC_OK &&
        filter == MESA_PACKET_FILTER_DISCARD) {
        T_I("frame discarded by egress filtering, port: %u, vid: %u",
            iport2uport(egress_port), info.vid);
        return TRUE;
    }

    return FALSE;
}

static void lprot_transmit(vtss_usid_t usid, mesa_port_no_t egress_port, lprot_port_state_t *pstate)
{
    if (!lprot_egress_filter(egress_port)) {
        vtss_uport_no_t uport = iport2uport(pstate->port);
        loop_prot_pdu_t *pdu = (loop_prot_pdu_t*) packet_tx_alloc(sizeof(*pdu));
        T_N("transmit check %d", uport);
        if(pdu) {
            packet_tx_props_t tx_props;

            T_D("transmit link %d", uport);
            packet_tx_props_init(&tx_props);
            tx_props.packet_info.modid     = VTSS_MODULE_ID_LOOP_PROTECT;
            tx_props.packet_info.frm       = (u8 *)pdu;
            tx_props.packet_info.len       = sizeof(*pdu);
            tx_props.tx_info.dst_port_mask = VTSS_BIT64(egress_port);

            /* Frame */
            memset(pdu, 0, sizeof(*pdu));
            memcpy(pdu->dst, dmac, sizeof(pdu->dst));
            memcpy(pdu->src, lprot_state.primary_switch_mac, sizeof(pdu->dst));
            pdu->oui = htons(etype);
            pdu->version = LPROT_PROTVERSION;
            pdu->lport = egress_port;
            pdu->usid = (u16) usid;
            pdu->tstamp = vtss_current_time();
            memcpy(pdu->switchmac, switchmac, sizeof(pdu->switchmac)); /* Own MAC */
            if(packet_tx(&tx_props) != VTSS_RC_OK)
                T_E("transmit fail port %d", uport);
        } else {
            T_E("transmit malloc fail port %d", uport);
        }
    }
    pstate->transmit_timer = 0;
}

/*
 * Main thread helper
 */

static void lprot_poag_tick(lprot_port_state_t *pstate, const vtss_appl_loop_protect_port_conf_t *pconf)
{
    BOOL                    tx = pconf->transmit;
    mesa_port_no_t          egress_port = pstate->port;
    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t port_status;

    T_N("Tick: %s %d (%sabled, tx: %d), shutdown %d",
        "Port", iport2uport(pstate->port),
        pstate->info.disabled ? "En" : "Dis", tx, pstate->shutdown_timer);

    if (tx) {
        (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, pstate->port, &ifindex);

        if (vtss_appl_port_status_get(ifindex, &port_status) == VTSS_RC_OK) {
            tx = port_status.link;
        } else {
            T_W("vtss_appl_port_status_get(%u) failed, skip port tx", pstate->port);
            tx = FALSE;
        }
    }

    if (pstate->ttl_timer) {
        --pstate->ttl_timer; /* Decrement TTL nocheck timer */
    }

    if (pstate->info.disabled) { /* Has the port been shut down? */
        pstate->shutdown_timer++;
        if (lprot_conf.global.shutdown_time != 0 &&
           pstate->shutdown_timer >= (int)lprot_conf.global.shutdown_time) {
            lprot_enable(pstate);
            if (tx) {
                lprot_transmit(lprot_state.usid, egress_port, pstate); /* Immediately TX */
            }
        }
    } else {
        if (pconf->transmit) { /* Are we an active transmitter? */
            pstate->transmit_timer++;
            if (tx && pstate->transmit_timer >= (int)lprot_conf.global.transmission_time) {
                lprot_transmit(lprot_state.usid, egress_port, pstate);
            }
        }
    }
}

/*
 * Main thread
 */

static void loop_periodic(void)
{
    T_N("loop protect tick");
    if(lprot_conf.global.enabled) {         /* Do we do anything ? */
        port_iter_t pit;
        /* Ports */
        (void) port_iter_init(&pit, NULL, VTSS_ISID_LOCAL,
                              PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            const vtss_appl_loop_protect_port_conf_t *pconf = &lprot_conf.ports[VTSS_ISID_LOCAL][pit.iport];
            lprot_port_state_t *pstate = &lprot_state.ports[pit.iport];
            /* Tick active ports */
            if(pconf->enabled)
                lprot_poag_tick(pstate, pconf);
        }
    }
}

// VLAN/MSTP/ERPS/.. ingress filtering
static BOOL lprot_ingress_filter(u32 src_port, mesa_vid_t vid)
{
    mesa_packet_filter_t     filter;
    mesa_packet_frame_info_t info;

    if (src_port == VTSS_PORT_NO_NONE) {
        return TRUE;            /* Filter frames on interconnect ports */
    }

    mesa_packet_frame_info_init(&info);
    info.port_no = src_port;
    info.vid = vid;

    if (mesa_packet_frame_filter(NULL, &info, &filter) == VTSS_RC_OK &&
        filter == MESA_PACKET_FILTER_DISCARD) {
        T_I("frame discarded by ingress filtering, port: %u, vid: %u",
            iport2uport(src_port), info.vid);
        return TRUE;
    }

    return FALSE;
}

static BOOL rx_frame(void *contxt,
                     const u8 *const frame,
                     const mesa_packet_rx_info_t *rx_info)
{
    loop_prot_pdu_t *pdu = (loop_prot_pdu_t*) frame;
    u32 in_port = rx_info->port_no, out_port = pdu->lport;
    lprot_port_state_t *pstate;
    const vtss_appl_loop_protect_port_conf_t *pconf;

    /* Apply ingress filtering */
    if (lprot_ingress_filter(rx_info->port_no, rx_info->tag.vid)) {
        return FALSE;
    }

    LPROT_CRIT_ENTER();

    pstate = &lprot_state.ports[in_port];
    pconf = &lprot_conf.ports[VTSS_ISID_LOCAL][in_port];

    T_D("Rx: %02x-%02x-%02x-%02x-%02x-%02x on port %u ", frame[0], frame[1], frame[2], frame[3], frame[4], frame[5], iport2uport(in_port));

    /* Enabled for loop protection ? */
    if(!lprot_conf.global.enabled || !pconf->enabled) {
        T_D("Discard - not enabled globally or on port");
        goto discard;
    }

    if(memcmp(pdu->dst, dmac, sizeof(pdu->dst)) != 0 ||     /* Proper DST */
       memcmp(pdu->src, lprot_state.primary_switch_mac, sizeof(pdu->src)) != 0 || /* Proper SRC */
       pdu->oui != htons(etype) ||                          /* Proper OUI */
       pdu->version != LPROT_PROTVERSION) {                 /* Proper version */
        T_D("Discard - Basic PDU check fails (sda/sa, oui, protocol, version)");
    } else {
        u32 tdelta = (vtss_current_time() - pdu->tstamp);
        T_I("Rx: checking looped pdu port %u (from port %u:%u), delta(t): %u, ttl_timer %d",
            iport2uport(in_port), u16(pdu->usid), iport2uport(out_port), tdelta, pstate->ttl_timer);
        if((pstate->ttl_timer > 0) || /* Port just came up */
           pdu->usid != lprot_state.usid ||                     /* Other unit */
           (VTSS_OS_TICK2MSEC(tdelta) < LPROT_MAX_TTL_MSECS)) { /* OR recent PDU */
            BOOL valid = TRUE;  /* Assume valid until proven wrong */
#ifdef VTSS_SW_OPTION_LOOP_DETECT
            /*
             * Give loop detection time to do its work on the
             * appropriate ports in the given time.
             */
            if(valid && vtss_lb_port(in_port) && vtss_lb_port(out_port)) {
                T_D("Rx: Dropping loopback on (active) loopback detect ports [%d,%d]",
                    in_port, out_port);
                valid = FALSE;
            }
#endif
            if(valid) {
                lprot_disable(pstate);
            } else {
                T_N("Rx: Dropping frame");
            }
        } else {
            T_D("Rx: Frame too old (%u)", tdelta);
        }
    }
discard:
    LPROT_CRIT_EXIT();
    return(TRUE);
}

static void loop_protect_packet_register(void)
{
    packet_rx_filter_t filter;

    packet_rx_filter_init(&filter);
    filter.modid = VTSS_MODULE_ID_LOOP_PROTECT;
    filter.match = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_SMAC |
        PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_ACL;
    filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;
    filter.etype = etype;
    memcpy(filter.dmac, dmac, sizeof(filter.dmac));
    memcpy(filter.smac, lprot_state.primary_switch_mac, sizeof(filter.smac));
    filter.cb    = rx_frame;

    if (loop_filter_id) {
        if (packet_rx_filter_change(&filter, &loop_filter_id) != VTSS_RC_OK) {
            T_E("packet rx re-register failed");
        }
    } else if (packet_rx_filter_register(&filter, &loop_filter_id) != VTSS_RC_OK) {
        T_E("packet rx register failed");
    }
}

static void port_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    LPROT_CRIT_ENTER();
    lprot_port_state_t *pstate = &lprot_state.ports[port_no];
    const vtss_appl_loop_protect_port_conf_t *pconf = &lprot_conf.ports[VTSS_ISID_LOCAL][port_no];

#if defined(VTSS_SW_OPTION_SYSLOG)
    if (lprot_conf.global.enabled && pconf->enabled &&
        (pconf->action == VTSS_APPL_LOOP_PROTECT_ACTION_SHUT_LOG ||
         pconf->action == VTSS_APPL_LOOP_PROTECT_ACTION_LOG_ONLY)) {
        char buf[128], *p = &buf[0];
        p += sprintf(p, "LOOP_PROTECTION-UPDOWN: Loop protection on ");
        p += sprintf(p, "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
        p += sprintf(p, ", changed state to %s.", status->link ? "up" : "down");
        S_PORT_N(VTSS_ISID_LOCAL, pstate->port, "%s", buf);
    }
#endif /* VTSS_SW_OPTION_SYSLOG */

    if (status->link) {  /* Link */
        if (lprot_conf.global.enabled) {
            if (!pstate->info.disabled) {
                pstate->ttl_timer = LPROT_TTL_NOCHECK;
                if (pconf->enabled && pconf->transmit) {
                    /* Immediate transmit triggering */
                    lprot_transmit(lprot_state.usid, port_no, pstate);
                }
            }
        }
    } else {  /* No link */
        if (lprot_conf.global.enabled && pconf->enabled) {
            if (pstate->info.disabled) {
                T_D("Link down on disabled uport %d  - ignore", iport2uport(port_no));
            } else {
                /* Reset state to initial state if link down and loop was not disabled. */
                T_D("Link down on active uport %d  - reset it", iport2uport(port_no));
                if (lprot_conf.global.shutdown_time != 0) {
                    T_D("Link down on active uport %d  - reset it", iport2uport(port_no));
                    lprot_enable(pstate);
                } else {
                    T_D("Link down on active uport %d  - do not reset it because shutdown_time == 0", iport2uport(port_no));
                }
            }
        }
    }
    LPROT_CRIT_EXIT();
}

/*
 * Message indication function
 */
static BOOL loop_protect_msg_rx(void             *contxt,
                                const void       *rx_msg,
                                size_t           len,
                                vtss_module_id_t modid,
                                u32              isid)
{
    const loop_protect_msg_t *msg = (const loop_protect_msg_t *)rx_msg;
    T_D("Sid %u, rx %zd bytes, msg %d", isid, len, msg->msg_id);
    switch (msg->msg_id) {
    case LOOP_PROTECT_MSG_ID_CONF:
    {
        mesa_port_no_t port_no;
        T_D("LOOP_PROTECT_MSG_ID_CONF");
        LPROT_CRIT_ENTER();
        memcpy(lprot_state.primary_switch_key, msg->data.unit_conf.key, sizeof(lprot_state.primary_switch_key));
        memcpy(lprot_state.primary_switch_mac, msg->data.unit_conf.mac, sizeof(lprot_state.primary_switch_mac));
        lprot_state.usid = msg->data.unit_conf.usid;
        lprot_conf.global = msg->data.unit_conf.global_conf;
        loop_protect_packet_register();
        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            lprot_conf.ports[VTSS_ISID_LOCAL][port_no] = lprot_conf.ports[VTSS_ISID_START][port_no];
        }
        LPROT_CRIT_EXIT();
        lprot_conf_apply_local();
        break;
    }
    case LOOP_PROTECT_MSG_ID_CONF_PORT:
    {
        mesa_port_no_t port_no = msg->data.port_conf.port_no;
        T_D("LOOP_PROTECT_MSG_ID_CONF_PORT, port %d", port_no);
        LPROT_CRIT_ENTER();
        lprot_conf.ports[VTSS_ISID_LOCAL][port_no] = msg->data.port_conf.port_conf;
        LPROT_CRIT_EXIT();
        lprot_conf_apply_local();
        break;
    }
    case LOOP_PROTECT_MSG_ID_PORT_CTL:
    {
        mesa_port_no_t port_no = msg->data.port_ctl.port_no;
        BOOL disable = msg->data.port_ctl.disable;
        T_D("LOOP_PROTECT_MSG_ID_PORT_CTL, port %d, disable %d", port_no, disable);
        if(msg_switch_is_primary()) {
            lprot_port_disable_primary_switch(port_no, disable);
        } else {
            T_W("Skipping on secondary switch");
        }
        break;
    }
    case LOOP_PROTECT_MSG_ID_PORT_STATUS_REQ:
    {
        loop_protect_msg_t *rep = loop_protect_alloc_message(LOOP_PROTECT_MSG_ID_PORT_STATUS_RSP);
        if(rep) {
            msg_tx(VTSS_MODULE_ID_LOOP_PROTECT, isid, rep, sizeof(*rep));
        }
        break;
    }
    case LOOP_PROTECT_MSG_ID_PORT_STATUS_RSP:
    {
        if(msg_switch_is_primary()) {
            port_iter_t pit;
            (void) port_iter_init(&pit, NULL, isid,
                                  PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            LPROT_CRIT_ENTER();
            while (port_iter_getnext(&pit)) {
                lprot_primary_switch_state[isid].ports[pit.iport] = lprot_state.ports[pit.iport].info;
            }
            lprot_state.switches[isid].last_refresh = time(NULL);
            vtss_flag_setbits(&lprot_status_flags, 1<<isid);
            LPROT_CRIT_EXIT();
        } else {
            T_W("Skipping on secondary switch");
        }
        break;
    }
    default:
        T_W("Unhandled msg %d", msg->msg_id);
    }
    return TRUE;
}

/*
 * Stack Register
 */
static void
loop_protect_stack_register(void)
{
    msg_rx_filter_t filter;
    memset(&filter, 0, sizeof(filter));
    filter.cb = loop_protect_msg_rx;
    filter.modid = VTSS_MODULE_ID_LOOP_PROTECT;
    mesa_rc rc =  msg_rx_filter_register(&filter);
    VTSS_ASSERT(rc == VTSS_RC_OK);
}

static void lprot_init(void)
{
    uint i;

    /* memset(&lprot_state, 0, sizeof(lprot_state)); - this is BSS */
    for(i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        lprot_state.ports[i].port = i;
    }

    /* Initialize */
    (void) conf_mgmt_mac_addr_get(switchmac, 0);
}

static void lprot_thread(vtss_addrword_t data)
{
    // Wait until INIT_CMD_INIT is complete.
    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_LOOP_PROTECT);

    /* Port change callback */
    (void)port_change_register(VTSS_MODULE_ID_LOOP_PROTECT, port_change_callback);

    /* Message module RX */
    loop_protect_stack_register();

    vtss_flag_init(&lprot_status_flags);

    for(;;) {
        vtss_tick_count_t wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(1000);
        vtss_flag_value_t flags;
        while((flags = vtss_flag_timed_wait(&lprot_control_flags, 0xffff, VTSS_FLAG_WAITMODE_OR_CLR, wakeup))) {
            T_I("loop protect thread event, flags 0x%x", flags);
        }
        loop_periodic();
    }
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void LoopProtection_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_loop_protect_json_init(void);
#endif
extern "C" int loop_protect_icli_cmd_register();

/* Initialize module */
mesa_rc loop_protect_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

#ifdef VTSS_SW_OPTION_ICFG
    mesa_rc     rc;
#endif

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        /* Create semaphore for critical regions */
        critd_init(&lprot_crit, "lprot", VTSS_MODULE_ID_LOOP_PROTECT, CRITD_TYPE_MUTEX);

        LPROT_CRIT_ENTER();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        LoopProtection_mib_init();  /* Register our private mib */
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_loop_protect_json_init();
#endif
        loop_protect_icli_cmd_register();
        LPROT_CRIT_EXIT();

#ifdef VTSS_SW_OPTION_ICFG
        rc = loop_protect_icfg_init();
        if (rc != VTSS_RC_OK) {
            T_D("fail to init icfg registration for loop protect, rc = %s", error_txt(rc));
        }
#endif
        break;

    case INIT_CMD_START:
        T_D("START");
        lprot_init();
        vtss_flag_init(&lprot_control_flags);
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           lprot_thread,
                           0,
                           "loop_prot",
                           nullptr,
                           0,
                           &lprot_thread_handle,
                           &lprot_thread_block);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_GLOBAL) {
            /* Reset global configuration */
            loop_protect_conf_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        /* Read and apply config */
        loop_protect_conf_default();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        loop_protect_conf_send(isid);
        break;

    default:
        break;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

