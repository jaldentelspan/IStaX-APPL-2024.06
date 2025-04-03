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
#include "lacp.h"
#include "lacp_api.h"
#include "msg_api.h"
#include "aggr_api.h"
#include "port_api.h"
#include "critd_api.h"
#include "vtss_os_wrapper.h"
#include "conf_api.h"
#include "packet_api.h"
#include "misc_api.h"
#include "mgmt_api.h"
#include "vtss_lacp.h"
#if defined(VTSS_SW_OPTION_DOT1X)
#include "dot1x_api.h"
#endif /* VTSS_SW_OPTION_DOT1X */
#if defined(VTSS_SW_OPTION_ICFG)
#include "lacp_icfg.h"
#endif /* defined(VTSS_SW_OPTION_ICFG) */
#include "vtss/appl/nas.h" // For NAS management functions

#include "topo_api.h"
#define FLAG_ABORT         (1 << 0)
#define FLAG_LINK_UP       (1 << 1)
#define FLAG_LACP_ENABLED  (1 << 2)
static vtss_flag_t lacp_flags;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "lacp", "LACP Module"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define LACP_CORE_LOCK()   critd_enter(&lacp_global.coremutex, __FUNCTION__, __LINE__)
#define LACP_CORE_UNLOCK() critd_exit( &lacp_global.coremutex, __FUNCTION__, __LINE__)
#define LACP_DATA_LOCK()   critd_enter(&lacp_global.datamutex, __FUNCTION__, __LINE__)
#define LACP_DATA_UNLOCK() critd_exit( &lacp_global.datamutex, __FUNCTION__, __LINE__)

static vtss_handle_t      lacp_thread_handle;
static vtss_thread_t      lacp_thread_block;
static BOOL               lacp_disable;
#ifndef VTSS_SW_OPTION_MSTP
static vtss_handle_t      lacp_stp_thread_handle;
static vtss_thread_t      lacp_stp_thread_block;
#endif /* VTSS_SW_OPTION_MSTP */

static CapArray<l2_port_no_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> l2port_2_usidport;
static CapArray<l2_port_no_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> l2usid_2_l2port;

typedef struct {
    vtss_lacp_system_config_t system;                          /* NOTE: Only system_prio is R/W; system_id (the MAC) is R/O */
    CapArray<vtss_lacp_port_config_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> ports;
} lacp_conf_t;

static struct {
    critd_t coremutex; // LACP core library serialization
    critd_t datamutex; // LACP data region protection
    lacp_conf_t conf;  // Current configuration
} lacp_global;

const vtss_common_macaddr_t vtss_lacp_slowmac   = {VTSS_LACP_MULTICAST_MACADDR};
const vtss_common_macaddr_t vtss_common_zeromac = {{ 0, 0, 0, 0, 0, 0 }};

/****************************************************************************/
// Port state callback function.
// This function is called if a port state change occurs.
/****************************************************************************/
static void lacp_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_isid_t isid = VTSS_ISID_START;

    if (msg_switch_is_primary()) {
        l2_port_no_t l2port = L2PORT2PORT(isid, port_no);

        T_D("port:%d %s\n", l2port, status->link ? "up" : "down");

        LACP_CORE_LOCK();
        vtss_lacp_linkstate_changed(l2port, status->link ?
                                    VTSS_COMMON_LINKSTATE_UP :
                                    VTSS_COMMON_LINKSTATE_DOWN);
        LACP_CORE_UNLOCK();

#ifndef VTSS_SW_OPTION_MSTP
        vtss_lacp_port_config_t pconf;
        /* When an LACP-enabled port get a link-up event, port is set to delayed forwarding. */
        /* This will give LACP time to form aggregation.                                     */
        (void)lacp_mgmt_port_conf_get(isid, port_no, &pconf);
        if (status->link) {
            if (pconf.enable_lacp) {
                /* Enable delayed forwarding  */
                vtss_flag_setbits(&lacp_flags, FLAG_LINK_UP);
            } else {
                vtss_os_set_stpstate(l2port, MESA_STP_STATE_FORWARDING);
            }
        } else {
            vtss_os_set_stpstate(l2port, MESA_STP_STATE_DISCARDING);
        }
#endif /* VTSS_SW_OPTION_MSTP */
    }
}

/****************************************************************************/
// Apply the configuration to the whole stack
/****************************************************************************/
static void lacp_propagate_conf(void)
{
    l2_port_no_t l2port;
    BOOL lacp_ena = 0;

    T_I("Making configuration effective");
    LACP_CORE_LOCK();
    vtss_lacp_set_config(&lacp_global.conf.system);

    for (l2port = 0; l2port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); l2port++)  {
        vtss_lacp_set_portconfig(l2port, &lacp_global.conf.ports[l2port]);
        if (lacp_global.conf.ports[l2port].enable_lacp) {
            lacp_ena = 1;
        }
    }

    if (lacp_ena) {
        /* Wake up LACP thread */
        vtss_flag_setbits(&lacp_flags, FLAG_LACP_ENABLED);
        lacp_disable = 0;
    } else {
        /* Put LACP thread back to sleep */
        vtss_flag_maskbits(&lacp_flags, ~FLAG_LACP_ENABLED);
        lacp_disable = 1;
    }

    LACP_CORE_UNLOCK();
}

/****************************************************************************/
/****************************************************************************/
static void lacp_conf_default(void)
{
    unsigned int i;

    T_D("enter, lacp_conf_default");

    LACP_DATA_LOCK();
    lacp_global.conf.system.system_prio = VTSS_LACP_DEFAULT_SYSTEMPRIO;
    memcpy(lacp_global.conf.system.system_id.macaddr, VTSS_COMMON_ZEROMAC, sizeof(VTSS_COMMON_ZEROMAC));

    for (i = 0; i < lacp_global.conf.ports.size(); i++) {
        lacp_global.conf.ports[i].port_prio = VTSS_LACP_DEFAULT_PORTPRIO;
        lacp_global.conf.ports[i].port_key = VTSS_LACP_AUTOKEY;
        lacp_global.conf.ports[i].enable_lacp = FALSE;
        lacp_global.conf.ports[i].xmit_mode = VTSS_LACP_DEFAULT_FSMODE;
        lacp_global.conf.ports[i].active_or_passive = VTSS_LACP_DEFAULT_ACTIVITY_MODE;
    }
    LACP_DATA_UNLOCK();

#ifndef VTSS_SW_OPTION_MSTP
    {
        l2port_iter_t l2pit;

        /* Need to set all ports to STP forwarding - if not done by STP module */
        (void)l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, (l2port_iter_type_t)(L2PORT_ITER_TYPE_PHYS | L2PORT_ITER_ISID_ALL));
        while (l2port_iter_getnext(&l2pit)) {
            if (lacp_global.conf.ports[l2pit.l2port].enable_lacp) {
                vtss_os_set_stpstate(l2pit.l2port, MESA_STP_STATE_DISCARDING);
            } else {
                vtss_os_set_stpstate(l2pit.l2port, MESA_STP_STATE_FORWARDING);
            }
        }
    }
#endif /* VTSS_SW_OPTION_MSTP */
    lacp_propagate_conf();

    // Apply group defaults
    vtss_lacp_groupconfig_t group;
    group.revertive = VTSS_COMMON_BOOL_TRUE;
    group.max_bundle = VTSS_LACP_MAX_PORTS_IN_AGGR;
    for (i = 1; i <= VTSS_LACP_MAX_AGGR_; i++) {
        vtss_lacp_set_groupconf((vtss_lacp_key_t)i, &group);
    }
    T_D("Exit: lacp_conf_default");
}

/****************************************************************************/
// LACP packet reception on any switch
/****************************************************************************/
static BOOL rx_lacp(void  *contxt, const uchar *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    T_R("port_no: %d len %d vid %d", rx_info->port_no, rx_info->length, rx_info->tag.vid);

    // Check that the subtype is '1'
    if (frm[14] != 1) {
        return FALSE;  // Not a LACP frame
    }

    // NB: Null out the GLAG (port is 1st in aggr)
    l2_receive_indication(VTSS_MODULE_ID_LACP, frm, rx_info->length, rx_info->port_no, rx_info->tag.vid, (mesa_glag_no_t)(VTSS_GLAG_NO_START - 1));

    return FALSE; // Allow other subscribers to receive the packet
}

/****************************************************************************/
// Primary switch only. Received packet from l2proto - send it to base
/****************************************************************************/
static void lacp_stack_receive(const void *packet,
                               size_t len,
                               mesa_vid_t vid,
                               l2_port_no_t l2port)
{
    T_R("RX port %d len %zd", l2port, len);
    if (msg_switch_is_primary()) {
        LACP_CORE_LOCK();
        vtss_lacp_receive(l2port, (const vtss_common_octet_t *)packet, len);
        LACP_CORE_UNLOCK();
    }
}

/****************************************************************************/
// The LACP thread - if the switch is the primary switch
/****************************************************************************/
static void lacp_thread(vtss_addrword_t data)
{
    void *filter_id;
    packet_rx_filter_t rx_filter;
    u32 lacp_sleep_soon = 0;

    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_LACP);
    /* Initialize LACP */
    vtss_lacp_init();

    /* Port change callback */
    (void)port_change_register(VTSS_MODULE_ID_LACP, lacp_port_state_change_callback);
    // Registration for LACP frames
    packet_rx_filter_init(&rx_filter);
    rx_filter.modid                 = VTSS_MODULE_ID_LACP;
    rx_filter.match                 = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_ETYPE;
    rx_filter.cb                    = rx_lacp;
    rx_filter.prio                  = PACKET_RX_FILTER_PRIO_NORMAL;
    memcpy(rx_filter.dmac, VTSS_LACP_LACPMAC, sizeof(rx_filter.dmac));
    rx_filter.etype                 = 0x8809; // LACP

    (void)packet_rx_filter_register(&rx_filter, &filter_id);
    l2_receive_register(VTSS_MODULE_ID_LACP, lacp_stack_receive);

    /* Tick the LACP 'VTSS_LACP_TICKS_PER_SEC' per sec   */
    while (1) {
        while (msg_switch_is_primary()) {

            /* If LACP gets disabled it goes to sleep after cleaning up the protocol states */
            if (lacp_disable) {
                lacp_sleep_soon++;
            }
            if (lacp_sleep_soon == 10) {
                vtss_flag_wait(&lacp_flags, FLAG_LACP_ENABLED, VTSS_FLAG_WAITMODE_OR);
                lacp_sleep_soon = 0;
                lacp_disable = 0;
            }

            LACP_CORE_LOCK();
            vtss_lacp_tick();
            LACP_CORE_UNLOCK();
            VTSS_OS_MSLEEP(1000 / VTSS_LACP_TICKS_PER_SEC);
        }
        LACP_CORE_LOCK();
        vtss_lacp_deinit();
        LACP_CORE_UNLOCK();
        T_I("Suspending LACP thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_LACP);
        vtss_lacp_init();
    }
}

#if !defined(VTSS_SW_OPTION_MSTP)
/****************************************************************************/
/****************************************************************************/
static void lacp_stp_thread(vtss_addrword_t data)
{
    l2port_iter_t l2pit;

    // Wait until INIT_CMD_INIT is complete.
    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_LACP);

    while (1) {
        /* Wait for a link up event */
        (void)vtss_flag_wait(&lacp_flags, FLAG_LINK_UP, VTSS_FLAG_WAITMODE_OR_CLR);

        /* Now pause for few seconds */
        (void)vtss_flag_timed_wait(&lacp_flags, FLAG_ABORT, VTSS_FLAG_WAITMODE_OR, vtss_current_time() + VTSS_OS_MSEC2TICK(4000));

        /* Set ports STP state to forwarding */
        (void)l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_PHYS);
        while (l2port_iter_getnext(&l2pit)) {
            if (vtss_os_get_stpstate(l2pit.l2port) == MESA_STP_STATE_DISCARDING) {
                T_I("Setting state to forwarding port:%d", l2pit.l2port);
                vtss_os_set_stpstate(l2pit.l2port, MESA_STP_STATE_FORWARDING);
            }
        }
    }
}
#endif /* !defined(VTSS_SW_OPTION_MSTP) */

/****************************************************************************/
// Semi-public functions
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
void vtss_os_set_hwaggr(vtss_lacp_agid_t aid, vtss_common_port_t new_port)
{
    if (!msg_switch_is_primary()) {
        return;
    }

    if (aggr_mgmt_lacp_member_add(aid, new_port, TRUE) == AGGR_ERROR_REG_TABLE_FULL)  {
        /* Not possible to aggregate this port. Disable LACP for that port */
    }
}

/****************************************************************************/
/****************************************************************************/
void vtss_os_clear_hwaggr(vtss_lacp_agid_t aid, vtss_common_port_t old_port)
{
    if (!msg_switch_is_primary()) {
        return;
    }
    aggr_mgmt_lacp_member_add(aid, old_port, FALSE);
}

/****************************************************************************/
/****************************************************************************/
const char *vtss_common_str_macaddr(const vtss_common_macaddr_t VTSS_COMMON_PTR_ATTRIB *mac)
{
    static char VTSS_COMMON_DATA_ATTRIB buf[24];

    sprintf(buf, "%02X-%02X-%02X-%02X-%02X-%02X",
            mac->macaddr[0], mac->macaddr[1], mac->macaddr[2],
            mac->macaddr[3], mac->macaddr[4], mac->macaddr[5]);
    return buf;
}

/****************************************************************************/
// vtss_os_make_key - Generate a key for a port that reflects its relationship
/****************************************************************************/
vtss_lacp_key_t vtss_os_make_key(vtss_common_port_t portno, vtss_lacp_key_t new_key)
{
    int speed;

    if (new_key == VTSS_LACP_AUTOKEY) {
        /* We don't really care what the value is when the link is down */
        speed = vtss_os_get_linkspeed(portno);

        if (speed == 12000) {
            return 7;
        } else if (speed == 10000) {
            return 6;
        } else if (speed == 5000) {
            return 5;
        } else if (speed == 2500) {
            return 4;
        } else if (speed == 1000) {
            return 3;
        } else if (speed == 100) {
            return 2;
        }  else {
            return 1;
        }
    }
    return new_key;
}

/****************************************************************************/
// Convert between l2 ports and usid ports
/****************************************************************************/
vtss_common_port_t vtss_os_translate_port(vtss_common_port_t l2port, BOOL from_core)
{
    if (l2port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        if (from_core) {
            return l2port_2_usidport[l2port] + 1;    /* l2port -> usid port, plus one to make port 0 appear as port 1 */
        } else {
            return l2usid_2_l2port[l2port] + 1;    /* usid port -> l2port */
        }
    }
    return (vtss_common_port_t) - 1;
}

/****************************************************************************/
/*  API functions                                                           */
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
const char *lacp_error_txt(mesa_rc rc)
{
    switch (rc) {
    case LACP_ERROR_GEN:
        return "LACP generic error";
    case LACP_ERROR_STATIC_AGGR_ENABLED:
        return "Static aggregation is enabled";
    case LACP_ERROR_DOT1X_ENABLED:
        return "DOT1X is enabled";
    case LACP_ERROR_INVALID_AGGR_ID:
        return "Invalid aggregation id.";
    case LACP_ERROR_INVALID_PRT_IFTYPE:
        return "Invalid physical interface type format.";
    case LACP_ERROR_INVALID_PRT_KEY:
        return "Invalid LACP port key value.";
    case LACP_ERROR_INVALID_PRIO:
        return "Invalid LACP priority value.";
    case LACP_ERROR_ENTRY_NOT_FOUND:
        return "Aggregation entry not found.";
    case LACP_ERROR_MAX_BUNDLE_OVERFLOW:
        return "Max bundle overflow";
    default:
        return "LACP unknown error";
    }
}

mesa_rc vtss_appl_lacp_globals_set(const vtss_appl_lacp_globals_t *const conf)
{
    vtss_lacp_system_config_t sysconf;
    mesa_rc                   rc = VTSS_RC_ERROR;

    if ((conf->system_prio < VTSS_APPL_LACP_PRIORITY_MINIMUM) ||
        (conf->system_prio > VTSS_APPL_LACP_PRIORITY_MAXIMUM)) {
        return LACP_ERROR_INVALID_PRIO;
    }

    if ((rc = lacp_mgmt_system_conf_get(&sysconf)) == VTSS_RC_OK) {
        sysconf.system_prio = conf->system_prio;
        rc = lacp_mgmt_system_conf_set(&sysconf);
    }
    return rc;
}

mesa_rc vtss_appl_lacp_globals_get(vtss_appl_lacp_globals_t *const conf)
{
    vtss_lacp_system_config_t sysconf;
    mesa_rc                   rc = VTSS_RC_ERROR;

    if ((rc = lacp_mgmt_system_conf_get(&sysconf)) == VTSS_RC_OK) {
        conf->system_prio = sysconf.system_prio;
    }
    return rc;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc lacp_mgmt_system_conf_get(vtss_lacp_system_config_t *conf)
{
    if (!msg_switch_is_primary()) {
        return VTSS_UNSPECIFIED_ERROR;
    }

    LACP_DATA_LOCK();
    *conf = lacp_global.conf.system;
    LACP_DATA_UNLOCK();
    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc lacp_mgmt_system_conf_set(const vtss_lacp_system_config_t *conf)
{
    if (!msg_switch_is_primary()) {
        return VTSS_UNSPECIFIED_ERROR;
    }

    LACP_DATA_LOCK();
    lacp_global.conf.system = *conf;
    LACP_DATA_UNLOCK();
    /* Make effective in LACP core */
    LACP_CORE_LOCK();
    vtss_lacp_set_config(conf);
    LACP_CORE_UNLOCK();
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_lacp_port_conf_get(vtss_ifindex_t ifindex, vtss_appl_lacp_port_conf_t *const pconf)
{
    vtss_ifindex_elm_t ife;
    vtss_lacp_port_config_t conf;
    mesa_rc                 rc = VTSS_RC_ERROR;

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    if (ife.iftype == VTSS_IFINDEX_TYPE_PORT) {
        if ((rc = lacp_mgmt_port_conf_get(ife.isid, ife.ordinal, &conf)) == VTSS_RC_OK) {
            pconf->port_prio        = conf.port_prio;
            pconf->fast_xmit_mode   = (BOOL )conf.xmit_mode;
        }
    } else {
        rc = LACP_ERROR_INVALID_PRT_IFTYPE;
    }
    return rc;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc lacp_mgmt_port_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_lacp_port_config_t *pconf)
{
    l2_port_no_t l2port = L2PORT2PORT(isid, port_no);
    if (!msg_switch_is_primary()) {
        return VTSS_UNSPECIFIED_ERROR;
    }

    LACP_DATA_LOCK();
    *pconf = lacp_global.conf.ports[l2port];
    LACP_DATA_UNLOCK();
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_lacp_group_if_conf_set(const vtss_ifindex_t ifindex, const vtss_appl_lacp_group_conf_t *const gconf)
{
    vtss_ifindex_elm_t          ife;

    VTSS_RC(validate_aggr_index(ifindex, &ife));
    if (ife.iftype == VTSS_IFINDEX_TYPE_LLAG) {
        return vtss_appl_lacp_group_conf_set(ife.ordinal, gconf);
    }

    T_E("Group not supported");
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_lacp_group_conf_set(u16 key, const vtss_appl_lacp_group_conf_t *const gconf)
{
    vtss_lacp_groupconfig_t group;
    if (gconf->max_bundle > VTSS_LACP_MAX_PORTS_IN_AGGR) {
        return LACP_ERROR_MAX_BUNDLE_OVERFLOW;
    }
    if (key > VTSS_LACP_MAX_AGGR_) {
        return LACP_ERROR_INVALID_KEY;
    }
    group.revertive = gconf->revertive;
    group.max_bundle = gconf->max_bundle;
    vtss_lacp_set_groupconf((vtss_lacp_key_t)key, &group);
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_lacp_group_if_conf_get(const vtss_ifindex_t ifindex, vtss_appl_lacp_group_conf_t *const gconf)
{
    vtss_ifindex_elm_t          ife;

    VTSS_RC(validate_aggr_index(ifindex, &ife));
    if (ife.iftype == VTSS_IFINDEX_TYPE_LLAG) {
        return vtss_appl_lacp_group_conf_get(ife.ordinal, gconf);
    }

    T_E("Group not supported");
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_lacp_group_conf_get(u16 key, vtss_appl_lacp_group_conf_t *const gconf)
{
    vtss_lacp_groupconfig_t group;
    vtss_lacp_get_groupconf((vtss_lacp_key_t)key, &group);
    gconf->revertive = group.revertive;
    gconf->max_bundle = group.max_bundle;
    return VTSS_RC_OK;
}

mesa_rc lacp_mgmt_port_conf_get_default(vtss_lacp_port_config_t *pconf)
{
    pconf->port_prio         = VTSS_LACP_DEFAULT_PORTPRIO;
    pconf->port_key          = VTSS_LACP_AUTOKEY;
    pconf->enable_lacp       = FALSE;
    pconf->xmit_mode         = VTSS_LACP_DEFAULT_FSMODE;
    pconf->active_or_passive = VTSS_LACP_DEFAULT_ACTIVITY_MODE;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_lacp_port_conf_set(vtss_ifindex_t ifindex, const vtss_appl_lacp_port_conf_t *const pconf)
{
    vtss_ifindex_elm_t ife;
    vtss_lacp_port_config_t config;
    mesa_rc                 rc = VTSS_RC_ERROR;

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    if (ife.iftype == VTSS_IFINDEX_TYPE_PORT) {
        if (lacp_mgmt_port_conf_get(ife.isid, ife.ordinal, &config) != VTSS_RC_OK) {
            return VTSS_UNSPECIFIED_ERROR;
        }
        config.port_prio    = pconf->port_prio;
        config.xmit_mode    = pconf->fast_xmit_mode;
        rc = lacp_mgmt_port_conf_set(ife.isid, ife.ordinal, &config);
    } else {
        rc = LACP_ERROR_INVALID_PRT_IFTYPE;
    }
    return rc;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc lacp_mgmt_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_lacp_port_config_t *pconf)
{
    l2_port_no_t l2port = L2PORT2PORT(isid, port_no);
    aggr_mgmt_group_no_t  aggr_no;
    aggr_mgmt_group_member_t group;
    BOOL lacp_enabled = 0;
    mesa_rc rc;

    if (!msg_switch_is_primary()) {
        return VTSS_UNSPECIFIED_ERROR;
    }

    if (pconf->enable_lacp) {
        /* Check if the ports is a member of static aggregation */
        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END_; aggr_no++) {
            if (aggr_mgmt_port_members_get(isid, aggr_no, &group, 0) == VTSS_RC_OK) {
                if (group.entry.member[port_no]) {
                    return LACP_ERROR_STATIC_AGGR_ENABLED;
                }
            }
        }
    }

#if defined(VTSS_SW_OPTION_DOT1X)
    vtss_nas_switch_cfg_t switch_cfg;

    /* Check if dot1x is enabled */
    vtss_usid_t usid = topo_isid2usid(isid);
    if (vtss_nas_switch_cfg_get(usid, &switch_cfg) != VTSS_RC_OK) {
        return VTSS_UNSPECIFIED_ERROR;
    }
    if (switch_cfg.port_cfg[port_no].admin_state != VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED) {
        return LACP_ERROR_DOT1X_ENABLED;
    }
#endif /* VTSS_SW_OPTION_DOT1X */

    /* Create placeholder for the LACP group in the aggr module */
    if (pconf->enable_lacp) {
        if ((rc = aggr_mgmt_lacp_member_set(isid, port_no, pconf->port_key)) != VTSS_RC_OK) {
            return rc;
        }
    } else {
        if ((rc = aggr_mgmt_lacp_member_set(isid, port_no, VTSS_AGGR_NO_NONE)) != VTSS_RC_OK) {
            return rc;
        }
    }

    LACP_DATA_LOCK();
    lacp_global.conf.ports[l2port] = *pconf;
    LACP_DATA_UNLOCK();
    LACP_CORE_LOCK();
    vtss_lacp_set_portconfig((l2port), pconf);
    LACP_CORE_UNLOCK();

    for (l2port = 0; l2port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); l2port++)  {
        if (lacp_global.conf.ports[l2port].enable_lacp) {
            lacp_enabled = 1;
            break;
        }
    }

    if (lacp_enabled) {
        /* Wake up LACP thread */
        vtss_flag_setbits(&lacp_flags, FLAG_LACP_ENABLED);
        lacp_disable = 0;
    } else {
        /* Put LACP thread back to sleep */
        vtss_flag_maskbits(&lacp_flags, ~FLAG_LACP_ENABLED);
        lacp_disable = 1;
    }
    return VTSS_RC_OK;
}

#ifndef VTSS_LACP_NDEBUG
mesa_rc debug_vtss_lacp_dump()
{
    vtss_lacp_dump();
    return VTSS_RC_OK;
}
#endif

/* portlist_* are defined in vtss_appl/mac/mac.c file */
BOOL portlist_index_set(u32 i, vtss_port_list_stackable_t *pl);
BOOL portlist_index_clear(u32 i, vtss_port_list_stackable_t *pl);
u32 isid_port_to_index(vtss_isid_t i,  mesa_port_no_t p);

static BOOL lacp_mgmt_aggr_status_get_by_aggr_no(unsigned int aid, vtss_lacp_aggregatorstatus_t *status)
{
    BOOL        first_search = 1, found = 0;
    int         search_aid, return_aid;

    while (aggr_mgmt_lacp_id_get_next(first_search ? NULL : &search_aid, &return_aid) == VTSS_RC_OK) {
        search_aid = return_aid;
        first_search = 0;
        if (!lacp_mgmt_aggr_status_get(return_aid, status)) {
            continue;
        }

        if (mgmt_aggr_no2id(lacp_to_aggr_id(status->aggrid)) == aid) {
            found = 1;
            break;
        }
    }
    return found;
}

mesa_rc vtss_lacp_system_status_get(vtss_ifindex_t ifindex, vtss_appl_lacp_aggregator_status_t *const stat)
{
    vtss_ifindex_elm_t ife;
    vtss_lacp_aggregatorstatus_t status;
    port_iter_t                  pit;

    memset(stat, 0, sizeof(vtss_appl_lacp_aggregator_status_t));

    if (validate_aggr_index(ifindex, &ife) != VTSS_RC_OK) {
        return LACP_ERROR_INVALID_AGGR_ID;
    }

    T_D("ordinal:%d, isid:%d, iftype:%d", ife.ordinal, ife.isid, ife.iftype);
    if (lacp_mgmt_aggr_status_get_by_aggr_no(ife.ordinal, &status)) {
        stat->aggr_id = mgmt_aggr_no2id(lacp_to_aggr_id(status.aggrid));
        memcpy(stat->partner_oper_system.addr, status.partner_oper_system.macaddr, sizeof(mesa_mac_t));
        stat->partner_oper_system_priority = status.partner_oper_system_priority;
        stat->partner_oper_key = status.partner_oper_key;
        stat->secs_since_last_change = (u32)status.secs_since_last_change;

        (void)port_iter_init(&pit, NULL, ife.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (status.port_list[pit.iport]) {
                (void)portlist_index_set(isid_port_to_index(ife.isid, pit.iport), &stat->port_list);
            } else {
                (void)portlist_index_clear(isid_port_to_index(ife.isid, pit.iport), &stat->port_list);
            }
        }

        T_D("LAG with aggrid:%d is found", stat->aggr_id);
        return VTSS_RC_OK;
    }
    return LACP_ERROR_ENTRY_NOT_FOUND;
}

/****************************************************************************/
/****************************************************************************/
vtss_common_bool_t lacp_mgmt_aggr_status_get(unsigned int aid, vtss_lacp_aggregatorstatus_t *stat)
{
    vtss_common_bool_t res;
    if (!msg_switch_is_primary()) {
        return VTSS_COMMON_BOOL_FALSE;
    }

    LACP_CORE_LOCK();
    res = vtss_lacp_get_aggr_status(aid, stat);
    LACP_CORE_UNLOCK();
    return res;
}

mesa_rc vtss_appl_lacp_port_stats_get(vtss_ifindex_t ifindex, vtss_appl_lacp_port_stats_t *const stats)
{
    vtss_lacp_portstatus_t  complete_status;
    vtss_ifindex_elm_t      ife;
    mesa_rc                 rc = VTSS_RC_ERROR;

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    if (ife.iftype == VTSS_IFINDEX_TYPE_PORT) {
        if ((rc = lacp_mgmt_port_status_get(ife.ordinal, &complete_status)) == VTSS_RC_OK) {
            stats->lacp_frame_rx = complete_status.port_stats.lacp_frame_recvs;
            stats->lacp_frame_tx = complete_status.port_stats.lacp_frame_xmits;
            stats->illegal_frame_rx = complete_status.port_stats.illegal_frame_recvs;
            stats->unknown_frame_rx = complete_status.port_stats.unknown_frame_recvs;
        }
    } else {
        rc = LACP_ERROR_INVALID_PRT_IFTYPE;
    }
    return rc;
}

mesa_rc vtss_appl_lacp_port_status_get(vtss_ifindex_t ifindex, vtss_appl_lacp_port_status_t *const status)
{
    vtss_lacp_portstatus_t  complete_status;
    vtss_lacp_port_config_t conf;
    vtss_ifindex_elm_t      ife;
    int                     search_aid, return_aid;
    BOOL                    first_search = TRUE;
    CapArray<u32, VTSS_APPL_CAP_AGGR_MGMT_GROUPS> partner_key;

    if (((vtss_ifindex_decompose(ifindex, &ife)) != VTSS_RC_OK) || (ife.iftype != VTSS_IFINDEX_TYPE_PORT)) {
        return VTSS_RC_ERROR;
    }

    while (aggr_mgmt_lacp_id_get_next(first_search ? NULL : &search_aid, &return_aid) == VTSS_RC_OK) {
        search_aid = return_aid;
        first_search = FALSE;

        vtss_lacp_aggregatorstatus_t aggr;
        if (!lacp_mgmt_aggr_status_get(return_aid, &aggr)) {
            continue;
        }

        partner_key[mgmt_aggr_no2id(lacp_to_aggr_id(aggr.aggrid))] = aggr.partner_oper_key;
    }

    if ((lacp_mgmt_port_conf_get(ife.isid, ife.ordinal, &conf) == VTSS_RC_OK) &&
        (lacp_mgmt_port_status_get(ife.ordinal, &complete_status) == VTSS_RC_OK)) {
        status->lacp_mode = complete_status.port_enabled;
        status->actor_port_prio = conf.port_prio;
        status->actor_oper_port_key = complete_status.actor_oper_port_key;
        status->actor_lacp_state = complete_status.actor_lacp_state;

        status->partner_oper_port_priority = complete_status.partner_oper_port_priority;
        status->partner_oper_port_number = complete_status.partner_oper_port_number;
        aggr_mgmt_group_no_t aggr_no = lacp_to_aggr_id(complete_status.actor_port_aggregator_identifier);
        status->partner_oper_port_key = partner_key[aggr_no];
        status->partner_lacp_state = complete_status.partner_lacp_state;

        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc lacp_mgmt_port_status_get(l2_port_no_t l2port, vtss_lacp_portstatus_t *stat)
{
    if (!msg_switch_is_primary()) {
        return VTSS_UNSPECIFIED_ERROR;
    }

    LACP_CORE_LOCK();
    vtss_lacp_get_port_status(l2port, stat);
    LACP_CORE_UNLOCK();
    return VTSS_RC_OK;
}

mesa_rc vtss_lacp_port_stats_clr(vtss_ifindex_t ifindex)
{
    vtss_ifindex_elm_t      ife;
    mesa_rc                 rc = VTSS_RC_ERROR;

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    if (ife.iftype == VTSS_IFINDEX_TYPE_PORT) {
        lacp_mgmt_statistics_clear(L2PORT2PORT(ife.isid, ife.ordinal));
        rc = VTSS_RC_OK;
    } else {
        rc = LACP_ERROR_INVALID_PRT_IFTYPE;
    }
    return rc;
}

/****************************************************************************/
/****************************************************************************/
void lacp_mgmt_statistics_clear(l2_port_no_t l2port)
{
    LACP_CORE_LOCK();
    vtss_lacp_clear_statistics(l2port);
    LACP_CORE_UNLOCK();
}

VTSS_PRE_DECLS void lacp_mib_init(void);
VTSS_PRE_DECLS void vtss_appl_lacp_json_init(void);
extern "C" int lacp_icli_cmd_register();

/****************************************************************************/
/****************************************************************************/
mesa_rc lacp_init(vtss_init_data_t *data)
{
    vtss_isid_t    isid = data->isid;
    mesa_port_no_t port_no;
    l2_port_no_t   l2port, usidport;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Create mutexes for critical regions
        critd_init(&lacp_global.datamutex, "LACP data", VTSS_MODULE_ID_LACP, CRITD_TYPE_MUTEX);
        critd_init(&lacp_global.coremutex, "LACP core", VTSS_MODULE_ID_LACP, CRITD_TYPE_MUTEX);

        vtss_flag_init(&lacp_flags);

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        lacp_mib_init();  /* Register our private mib */
#endif /* defined(VTSS_SW_OPTION_PRIVATE_MIB) */

#if defined(VTSS_SW_OPTION_JSON_RPC)
        vtss_appl_lacp_json_init();
#endif /* defined(VTSS_SW_OPTION_JSON_RPC) */

        lacp_icli_cmd_register();

#if defined(VTSS_SW_OPTION_ICFG)
        if (lacp_icfg_init() != VTSS_RC_OK) {
            T_D("Calling lacp_icfg_init() failed");
        }
#endif /* defined(VTSS_SW_OPTION_ICFG) */

        // Create thread(s).
        // This thread will start working upon completion of INIT_CMD_ICFG_LOADING_PRE event.
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           lacp_thread,
                           0,
                           "LACP Control",
                           nullptr,
                           0,
                           &lacp_thread_handle,
                           &lacp_thread_block);

#if !defined(VTSS_SW_OPTION_MSTP)
        // This thread will start working upon completion of INIT_CMD_INIT event.
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           lacp_stp_thread,
                           0,
                           "LACP Port FWD",
                           nullptr,
                           0,
                           &lacp_stp_thread_handle,
                           &lacp_stp_thread_block);
#endif /* !defined(VTSS_SW_OPTION_MSTP) */
        break;

    case INIT_CMD_CONF_DEF:
        if (isid == VTSS_ISID_GLOBAL) {
            lacp_conf_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        lacp_conf_default();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");

        /* Convert between usid and l2ports and cache it */
        for (port_no = 0; port_no < data->switch_info[isid].port_cnt; port_no++) {
            usidport = L2PORT2PORT(topo_isid2usid(isid), port_no);
            l2port = L2PORT2PORT(isid, port_no);
            l2port_2_usidport[l2port] = usidport;
            l2usid_2_l2port[usidport] = l2port;
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

