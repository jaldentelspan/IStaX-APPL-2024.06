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

#include "conf_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "ip_api.h"
#include "ip_utils.hxx" // For vtss_ip_checksum() and friends.
#include "packet_api.h"
#include "dhcp_relay_api.h"
#include "dhcp_relay.h"
#include "dhcp_helper_api.h"
#include "mac_api.h"
#include "vlan_api.h"
#include "vtss/appl/dhcp_server.h"

#include "vtss_network.h"

#include <netinet/ip.h>
#include "dhcp.h"
#include <netinet/udp.h>

#define DHCP_RELAY_USING_ISCDHCP_PACKAGE      1

#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
#include "iscdhcp_istax.h"
#include "dhcp_relay_callout.h"
#endif /* DHCP_RELAY_USING_ISCDHCP_PACKAGE */

#ifdef VTSS_SW_OPTION_ICFG
#include "dhcp_relay_icfg.h"
#endif /* VTSS_SW_OPTION_ICFG */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_DHCP_RELAY

/* Callback functions */
static int  DHCP_RELAY_update_circuit_id_callback(u8 *mac, uint transaction_id, u8 *circuit_id);
static int  DHCP_RELAY_check_circuit_id_callback(u8 *mac, uint transaction_id, u8 *circuit_id);
static int  DHCP_RELAY_send_client_callback(char *raw, size_t len, struct sockaddr_in *to, u8 *mac, uint transaction_id);
static void DHCP_RELAY_send_server_callback(char *raw, size_t len, uint32_t srv_ip);
static void DHCP_RELAY_fill_giaddr_callback(u8 *mac, uint transaction_id, mesa_ipv4_t *agent_ipv4_addr);
static void dhcp_relay_local_stats_clear(void);

/* Local function */
static BOOL DHCP_RELAY_get_linkup_ipv4_interface_status(mesa_vid_t vid, vtss_appl_ip_if_status_t *status);

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static dhcp_relay_global_t DHCP_RELAY_global;

static vtss_trace_reg_t DHCP_RELAY_trace_reg = {
    VTSS_TRACE_MODULE_ID, "dhcp_relay", "DHCP relay"
};

static vtss_trace_grp_t DHCP_RELAY_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&DHCP_RELAY_trace_reg, DHCP_RELAY_trace_grps);

#define DHCP_RELAY_CRIT_ENTER() critd_enter(&DHCP_RELAY_global.crit, __FILE__, __LINE__)
#define DHCP_RELAY_CRIT_EXIT()  critd_exit( &DHCP_RELAY_global.crit, __FILE__, __LINE__)

/* Thread variables */
static vtss_handle_t DHCP_RELAY_thread_handle;
static vtss_thread_t DHCP_RELAY_thread_block;

/* Circuit ID (Sub-option 1):
   It indicates the information when agent receives DHCP message.
   The circuit ID field is 4 bytes in length and the format is <vlan_id><module_id><port_no>.
   <vlan_id>:   The first two bytes represent the VLAN ID.
   <module_id>: The third byte is the module ID(in standalone switch it always equal 0, in stackable switch it means switch ID).
   <port_no>:   The fourth byte is the port number (1-based). */

typedef struct {
    u16 vlan_id;
    u8  module_id;
    u8  port_no;
} dhcp_relay_circuit_id_t;


/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Determine if DHCP relay configuration has changed */
static int DHCP_RELAY_conf_changed(dhcp_relay_conf_t *old, dhcp_relay_conf_t *new_conf)
{
    return (new_conf->relay_mode != old->relay_mode
            || new_conf->relay_server_cnt != old->relay_server_cnt
            || memcmp(new_conf->relay_server, old->relay_server, sizeof(new_conf->relay_server))
            || new_conf->relay_info_mode != old->relay_info_mode
            || new_conf->relay_info_policy != old->relay_info_policy);
}

/* Set DHCP relay defaults */
static void DHCP_RELAY_default_set(dhcp_relay_conf_t *conf)
{
    conf->relay_mode = DHCP4R_DEF_MODE;
    conf->relay_server_cnt = DHCP4R_DEF_SRV_CNT;
    memset(conf->relay_server, 0x0, sizeof(conf->relay_server));
    conf->relay_info_mode = DHCP4R_DEF_INFO_MODE;
    conf->relay_info_policy = DHCP4R_DEF_INFO_POLICY;
}

#ifndef IP_VHL_HL
#define IP_VHL_HL(vhl)      ((vhl) & 0x0f)
#endif /* IP_VHL_HL */

// Convert subnet mask from network prefix size
#define DHCP_RELAY_SUBNET_PREFIX_2_MASK(prefix_size)    (0xFFFFFFFF << (32 - prefix_size))

/* Get the IPv4 default route */
static BOOL DHCP_RELAY_default_route_get(mesa_ipv4_uc_t *def_route)
{
    vtss_appl_ip_route_status_map_t     rts;
    vtss_appl_ip_route_status_map_itr_t itr;
    mesa_rc                             rc;

    if ((rc = vtss_appl_ip_route_status_get_all(rts, VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC, VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC)) != VTSS_RC_OK) {
        return FALSE;
    }

    for (itr = rts.begin(); itr != rts.end(); ++itr) {
        if (itr->first.route.route.ipv4_uc.network.address != 0 || itr->first.route.route.ipv4_uc.network.prefix_size != 0) {
            // Only looking for default routes
            continue;
        }

        if ((itr->second.flags & VTSS_APPL_IP_ROUTE_STATUS_FLAG_SELECTED) == 0) {
            // This is not an active and best route.
            continue;
        }

        T_D("Found IPv4 default route=0x%x\n", itr->first.route.route.ipv4_uc.destination);
        *def_route = itr->first.route.route.ipv4_uc;
        return TRUE;
    }

    return FALSE;
}

/* Lookup a valid interface which can reach the DHCP Relay server */
static BOOL DHCP_RELAY_avail_ifid_lookup(mesa_vid_t *ifid)
{
    dhcp_relay_conf_t           relay_conf;
    mesa_vid_t                  idx;
    vtss_ifindex_t              ifidx;
    BOOL                        has_def_route = FALSE, found = FALSE, fully_matched = FALSE, def_route_matched = FALSE;
    vtss_appl_ip_if_conf_ipv4_t ip_conf;
    mesa_ipv4_uc_t              def_route;
    mesa_prefix_size_t          fully_matched_prefix_size = 0, def_route_matched_prefix_size = 0;
    vtss_appl_ip_if_status_t    ifstat;

    if (dhcp_relay_mgmt_conf_get(&relay_conf) != VTSS_RC_OK) {
        T_D("Calling dhcp_relay_mgmt_conf_get() failed");
        return found;
    }

    memset(&def_route, 0, sizeof(def_route));
    has_def_route = DHCP_RELAY_default_route_get(&def_route);
    T_D("has_def_route=%s, addr=0x%x", has_def_route ? "TRUE" : "FALSE", def_route.destination);

    /* Lookup a valid interface */
    for (idx = VTSS_APPL_VLAN_ID_MIN; idx <= VTSS_APPL_VLAN_ID_MAX; idx++) {
        if (vtss_ifindex_from_vlan(idx, &ifidx) == VTSS_RC_OK &&
            vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &ifstat) == VTSS_RC_OK) {
            T_D("Found active interface vlan %d", idx);
            if (!found && !has_def_route) {
                // Found first available interface. Marked it first and looking for a better one later (fully matched)
                T_D("Found afirst available interface %d", idx);
                found = TRUE;
                *ifid = idx;
            }

            if (vtss_appl_ip_if_conf_ipv4_get(ifidx, &ip_conf) == VTSS_RC_OK) {
                // Is a fully matched interface?
                for (int i = 0; i < DHCP_RELAY_MGMT_MAX_DHCP_SERVER && i < relay_conf.relay_server_cnt; i++) {
                    if ((ip_conf.network.address & DHCP_RELAY_SUBNET_PREFIX_2_MASK(ip_conf.network.prefix_size)) ==
                        (relay_conf.relay_server[i] & DHCP_RELAY_SUBNET_PREFIX_2_MASK(ip_conf.network.prefix_size))) {
                        // Found a fully matched interface. Keep looking for a better one later (longest matched)
                        if (!fully_matched) {
                            T_D("First time found fully matched interface VLAN %d, ip_conf.network.prefix_size=%d", idx, ip_conf.network.prefix_size);
                            fully_matched = TRUE;
                            found = TRUE;
                            *ifid = idx;
                            fully_matched_prefix_size = ip_conf.network.prefix_size;
                        } else if (fully_matched_prefix_size < ip_conf.network.prefix_size) {
                            T_D("Found a better fully matched interface VLAN %d, ip_conf.network.prefix_size=%d(original=%d)", idx, ip_conf.network.prefix_size, fully_matched_prefix_size);
                            *ifid = idx;
                            fully_matched_prefix_size = ip_conf.network.prefix_size;
                        }
                        break;
                    }
                }

                // Is the same domain with the default route?
                if (!fully_matched &&
                    has_def_route &&
                    ((ip_conf.network.address & DHCP_RELAY_SUBNET_PREFIX_2_MASK(ip_conf.network.prefix_size)) ==
                     (def_route.destination & DHCP_RELAY_SUBNET_PREFIX_2_MASK(ip_conf.network.prefix_size)))) {
                    // Found a fully matched interface. Keep looking for a better one later (longest matched)
                    if (!def_route_matched) {
                        T_D("First time found interface VLAN %d matched default route, ip_conf.network.prefix_size=%d", idx, ip_conf.network.prefix_size);
                        def_route_matched = TRUE;
                        found = TRUE;
                        *ifid = idx;
                        def_route_matched_prefix_size = ip_conf.network.prefix_size;
                    } else if (def_route_matched_prefix_size < ip_conf.network.prefix_size) {
                        T_D("Found a better interface VLAN %d matched default route, ip_conf.network.prefix_size=%d(original=%d)", idx, ip_conf.network.prefix_size, def_route_matched_prefix_size);
                        *ifid = idx;
                        def_route_matched_prefix_size = ip_conf.network.prefix_size;
                    }



                }
            }
        }
    }

    if (found) { // Update to local database
        T_D("Found available interface %d", *ifid);
        DHCP_RELAY_CRIT_ENTER();
        DHCP_RELAY_global.avail_ifid = *ifid;
        DHCP_RELAY_CRIT_EXIT();
    }

    return found;
}

/*  Primary switch only. Receive packet from DHCP helper - send it to TCP/IP stack  */
static BOOL DHCP_RELAY_stack_receive_callback(const u8 *const packet, size_t len,
                                              const dhcp_helper_frame_info_t *helper_info,
                                              const dhcp_helper_rx_cb_flag_t flag)
{
    int sock_ip;
    struct sockaddr_in sin;     // Address structure used when sending packets over UDP
    int ret = -1;
    int bootp_offset = 42; // ether header 14 bytes + ip header 20 bytes + udp header 8 bytes
    struct dhcp_packet bp;
    vtss_appl_ip_if_status_t ifstat;

    T_D("enter, RX port:%d, glag_no:%d, vid:%d, len:%zd.", helper_info->port_no, helper_info->glag_no, helper_info->vid, len);
    if (helper_info->vid == 0) {
        // 0 is not a valid vlan
        return FALSE;
    }
    vtss_ifindex_t ifidx = vtss_ifindex_cast_from_u32(helper_info->vid, VTSS_IFINDEX_TYPE_VLAN);
    if (!msg_switch_is_primary()) {
        return FALSE;
    }
    if (len <= bootp_offset || len > 1514) {
        T_D("Packet length error (len=%u)", (u32)len);
        return FALSE;
    }

    // Open a socket for sending DHCP packets over UDP
    sock_ip = vtss_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ip == -1) {
        T_W("socket() - failed!");
        return FALSE;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(67);
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    memcpy(&bp, &packet[bootp_offset], sizeof(bp));
    T_D("bp giaddr %s", inet_ntoa(bp.giaddr));

    /*
     * Bugzilla #23624: We need to set the DHCP Relay address here as the information
     * about which interface the packet was received on would otherwise be lost.
     * We use the IP address for the VLAN which this request was received on.
     */
    if (bp.giaddr.s_addr == 0 && vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &ifstat) == VTSS_RC_OK) {
        bp.giaddr.s_addr = htonl(ifstat.u.ipv4.net.address);
        T_D("Adding DHCP Relay address %s for VLAN %u", inet_ntoa(bp.giaddr), helper_info->vid);
    }

    ret = sendto(sock_ip, &bp, (len - bootp_offset),  0, (struct sockaddr *)&sin, sizeof(sin));
    if (ret == -1) {
        T_W("sendto() - failed!");
    }

    close(sock_ip);

    T_D("exit");
    return TRUE;
}

#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
/* Setup DHCP relay configuration to engine */
static void DHCP_RELAY_engine_conf_set(dhcp_relay_conf_t *conf)
{
    uint idx;

    iscdhcp_relay_mode_set((conf->relay_mode && conf->relay_server_cnt) ? DHCP_RELAY_MGMT_ENABLED : DHCP_RELAY_MGMT_DISABLED);
    iscdhcp_clear_dhcp_server();
    for (idx = 0; idx < DHCP_RELAY_MGMT_MAX_DHCP_SERVER; idx++) {
        iscdhcp_add_dhcp_server(conf->relay_server[idx]);
    }
    iscdhcp_set_agent_info_mode(conf->relay_info_mode);
    iscdhcp_set_relay_info_policy(conf->relay_info_policy);
}
#endif /* DHCP_RELAY_USING_ISCDHCP_PACKAGE */

static void DHCP_RELAY_conf_apply(void)
{
    if (msg_switch_is_primary()) {
        DHCP_RELAY_CRIT_ENTER();
        if (DHCP_RELAY_global.dhcp_relay_conf.relay_mode && DHCP_RELAY_global.dhcp_relay_conf.relay_server_cnt) {
            T_D("registering dhcp relay");
            dhcp_helper_user_receive_register(DHCP_HELPER_USER_RELAY, DHCP_RELAY_stack_receive_callback);
            dhcp_helper_user_clear_local_stat_register(DHCP_HELPER_USER_RELAY, dhcp_relay_local_stats_clear);
        } else {
            T_D("unregistering dhcp relay");
            dhcp_helper_user_receive_unregister(DHCP_HELPER_USER_RELAY);
        }
#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
        DHCP_RELAY_engine_conf_set(&DHCP_RELAY_global.dhcp_relay_conf);
#endif /* DHCP_RELAY_USING_ISCDHCP_PACKAGE */
        DHCP_RELAY_CRIT_EXIT();
    }
}

// Avoid Custodial pointer 'status' has not been freed or returned,
// the all_status_p is freed by the message module.
/*lint -e{429} */
/* Get the IPv4 interface link status.
   TRUE: Link-up, FLASE:Link-down
 */
static BOOL DHCP_RELAY_get_linkup_ipv4_interface_status(mesa_vid_t vid, vtss_appl_ip_if_status_t *status)
{
    vtss_appl_ip_if_status_t ifstat;

    T_D("Enter: vid=%d", vid);
    if (vid == 0) {
        // 0 is not a valid vlan
        return FALSE;
    }
    vtss_ifindex_t ifidx = vtss_ifindex_cast_from_u32(vid, VTSS_IFINDEX_TYPE_VLAN);

    if (vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &ifstat) == VTSS_RC_OK) {
        *status = ifstat;
        T_D("Exit: Link-up");
        return TRUE;
    }

    T_D("Exit: Link-down");
    return FALSE;
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* DHCP relay error text */
const char *dhcp_relay_error_txt(mesa_rc rc)
{
    switch (rc) {
    case DHCP_RELAY_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on the primary switch";

    case DHCP_RELAY_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    default:
        return "DHCP Relay: Unknown error code";
    }
}

/* Get DHCP relay configuration */
mesa_rc dhcp_relay_mgmt_conf_get(dhcp_relay_conf_t *glbl_cfg)
{
    T_D("enter");

    if (glbl_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_RELAY_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_RELAY_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    DHCP_RELAY_CRIT_ENTER();
    *glbl_cfg = DHCP_RELAY_global.dhcp_relay_conf;
    DHCP_RELAY_CRIT_EXIT();

    T_D("enter, curr conf > relay mode: %d, relay server: %u, relay information mode: %d, relay information policy: %d, relay server count %d",
        glbl_cfg->relay_mode, glbl_cfg->relay_server[0], glbl_cfg->relay_info_mode, glbl_cfg->relay_info_policy, glbl_cfg->relay_server_cnt);

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set DHCP relay configuration */
mesa_rc dhcp_relay_mgmt_conf_set(dhcp_relay_conf_t *glbl_cfg)
{
    mesa_rc rc      = VTSS_RC_OK;
    int     changed = 0;

    T_D("enter, new conf > relay mode: %d, relay server: %u, relay information mode: %d, relay information policy: %d, relay server count %d",
        glbl_cfg->relay_mode, glbl_cfg->relay_server[0], glbl_cfg->relay_info_mode, glbl_cfg->relay_info_policy, glbl_cfg->relay_server_cnt);

    if (glbl_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_RELAY_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_RELAY_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    /* check illegal parameter */
    if (glbl_cfg->relay_info_mode == DHCP_RELAY_MGMT_DISABLED && glbl_cfg->relay_info_policy == DHCP_RELAY_INFO_POLICY_REPLACE) {
        return DHCP_RELAY_ERROR_INV_PARAM;
    }

    DHCP_RELAY_CRIT_ENTER();
    changed = DHCP_RELAY_conf_changed(&DHCP_RELAY_global.dhcp_relay_conf, glbl_cfg);
    DHCP_RELAY_CRIT_EXIT();

    /* If relay is being started, we need to check if dhcp server is running. If dhcp server is enabled on DUT, dhcp relay should not be started.
       Both open a socket on UDP port 67 and should not run at the same time to ensure that packets go to the correct module. */
    if (changed && glbl_cfg->relay_mode && glbl_cfg->relay_server_cnt) {
        vtss_appl_dhcp_server_config_globals_t server_conf;
        if (vtss_appl_dhcp_server_config_globals_get(&server_conf) != VTSS_RC_OK) {
            T_W("Could not get dhcp server configuration.");
            return VTSS_RC_ERROR;
        }
        if (server_conf.mode) {
            T_W("Can not start DHCP relay while DHCP server is active on device.");
            return VTSS_RC_ERROR;
        }
    }

    DHCP_RELAY_CRIT_ENTER();
    DHCP_RELAY_global.dhcp_relay_conf = *glbl_cfg;
    DHCP_RELAY_CRIT_EXIT();

    if (changed) {
        /* Activate changed configuration */
        DHCP_RELAY_conf_apply();
    }

    T_D("exit");

    return rc;
}

/****************************************************************************
 * Module thread
 ****************************************************************************/
/* We create a new thread to do it for instead of in 'Init Modules' thread.
   That we don't need wait a long time in 'Init Modules' thread. */
static void DHCP_RELAY_thread(vtss_addrword_t data)
{
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_DHCP_RELAY);

#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
    while (1) {
        if (msg_switch_is_primary()) {
            BOOL                        relay_mode;
            mesa_vid_t                  avail_ifid = 0;
            vtss_appl_ip_if_status_t    status;

            DHCP_RELAY_CRIT_ENTER();
            relay_mode = (DHCP_RELAY_global.dhcp_relay_conf.relay_mode && DHCP_RELAY_global.dhcp_relay_conf.relay_server_cnt) ? TRUE : FALSE;
            DHCP_RELAY_CRIT_EXIT();

            // Calling iscdhcp_init() when DHCP relay is enabled and found a available interface.
            if (relay_mode &&
                DHCP_RELAY_avail_ifid_lookup(&avail_ifid) &&
                DHCP_RELAY_get_linkup_ipv4_interface_status(avail_ifid, &status)) {
                T_D("avail_ifid=%d", avail_ifid);
                break;
            }
        } else {
            // No reason for using CPU ressources when we're a secondary switch
            T_D("Suspending DHCP relay thread");
            msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_DHCP_RELAY);
            T_D("Resumed DHCP relay thread");
        }

        VTSS_OS_MSLEEP(1000);
    }

    /* There's a forever loop in the function */
    (void) iscdhcp_init();
#else
    while (1) {
        VTSS_OS_MSLEEP(10000);
    }
#endif /* DHCP_RELAY_USING_ISCDHCP_PACKAGE */
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create DHCP relay stack configuration */
static void DHCP_RELAY_conf_read_stack(BOOL create)
{
    int                     changed;
    dhcp_relay_conf_t       *old_dhcp_relay_conf_p, new_dhcp_relay_conf;

    T_D("enter, create: %d", create);

    changed = 0;
    DHCP_RELAY_CRIT_ENTER();
    /* Use default values */
    DHCP_RELAY_default_set(&new_dhcp_relay_conf);

    old_dhcp_relay_conf_p = &DHCP_RELAY_global.dhcp_relay_conf;
    if (DHCP_RELAY_conf_changed(old_dhcp_relay_conf_p, &new_dhcp_relay_conf)) {
        changed = 1;
    }
    DHCP_RELAY_global.dhcp_relay_conf = new_dhcp_relay_conf;
    DHCP_RELAY_CRIT_EXIT();

    if (changed && create) { //Always set when topology change
        DHCP_RELAY_conf_apply();
    }

    T_D("exit");
}

/* Module start */
static void DHCP_RELAY_start(void)
{
    dhcp_relay_conf_t *conf_p;

    T_D("enter");

    /* Initialize DHCP relay configuration */
    conf_p = &DHCP_RELAY_global.dhcp_relay_conf;
    DHCP_RELAY_default_set(conf_p);

    /* Create semaphore for critical regions */
    critd_init(&DHCP_RELAY_global.crit, "dhcp_relay", VTSS_MODULE_ID_DHCP_RELAY, CRITD_TYPE_MUTEX);

    T_D("exit");
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void dhcp_relay_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_dhcp_relay_json_init(void);
#endif
extern "C" int dhcp_relay_icli_cmd_register();

/* Initialize module */
mesa_rc dhcp_relay_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    u8          mac[6];

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        DHCP_RELAY_start();
#ifdef VTSS_SW_OPTION_ICFG
        if (dhcp_relay_icfg_init() != VTSS_RC_OK) {
            T_E("dhcp_relay_icfg_init failed!");
        }
#endif /* VTSS_SW_OPTION_ICFG */

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        dhcp_relay_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_dhcp_relay_json_init();
#endif
        dhcp_relay_icli_cmd_register();
        /*Registrering to vlan interface updates*/
        (void)vtss_ip_if_callback_add(DHCP_RELAY_if_updated_callback);
        break;

    case INIT_CMD_START:
        /* Create DHCP relay thread */
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           DHCP_RELAY_thread,
                           0,
                           "DHCP Relay",
                           nullptr,
                           0,
                           &DHCP_RELAY_thread_handle,
                           &DHCP_RELAY_thread_block);
        T_D("START");
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            DHCP_RELAY_conf_read_stack(1);
            //dhcp_relay_stats_clear(); /* Don't need it since DHCP Helper already done */
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        DHCP_RELAY_CRIT_ENTER();
        DHCP_RELAY_global.avail_ifid = 0;
        DHCP_RELAY_CRIT_EXIT();
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");

        /* Read stack and switch configuration */
        DHCP_RELAY_conf_read_stack(0);

        if (conf_mgmt_mac_addr_get(mac, 0) >= 0) {
#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
            iscdhcp_set_platform_mac(mac);
            iscdhcp_set_remote_id(mac);
            iscdhcp_reply_update_circuit_id_register(DHCP_RELAY_update_circuit_id_callback);
            iscdhcp_reply_check_circuit_id_register(DHCP_RELAY_check_circuit_id_callback);
            iscdhcp_reply_send_client_register(DHCP_RELAY_send_client_callback);
            iscdhcp_reply_send_server_register(DHCP_RELAY_send_server_callback);
            iscdhcp_reply_fill_giaddr_register(DHCP_RELAY_fill_giaddr_callback);
#endif /* DHCP_RELAY_USING_ISCDHCP_PACKAGE */
        }

        dhcp_relay_stats_clear();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        /* Apply all configuration to switch */
        if (msg_switch_is_local(isid)) {
            DHCP_RELAY_conf_apply();
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Statistics functions                                                    */
/****************************************************************************/

/* Get DHCP relay statistics */
void dhcp_relay_stats_get(dhcp_relay_stats_t *stats)
{
#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
    iscdhcp_get_couters((iscdhcp_relay_counter_t *)stats);
#else
    memset(stats, 0x0, sizeof(*stats));
#endif /* DHCP_RELAY_USING_ISCDHCP_PACKAGE */
}

/* Clear DHCP relay local statistics */
static void dhcp_relay_local_stats_clear(void)
{
#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
    iscdhcp_clear_couters();
#endif /* DHCP_RELAY_USING_ISCDHCP_PACKAGE */
}

/* Clear DHCP relay statistics */
void dhcp_relay_stats_clear(void)
{
    /* Clear the local statistics and DHCP helper detail statistics */
    (void) dhcp_helper_stats_clear_by_user(DHCP_HELPER_USER_RELAY);
}


/****************************************************************************/
/*  Recored system IP address functions                                     */
/****************************************************************************/

/* Notify DHCP relay module when system IP address changed */
void dhcp_realy_sysip_changed(u32 ip_addr)
{
    struct in_addr ia;

    if (!ip_addr) {
        return;
    }
    ia.s_addr = htonl(ip_addr);
#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
    iscdhcp_change_interface_addr("vtss.vlan", &ia);
#endif /* DHCP_RELAY_USING_ISCDHCP_PACKAGE */
}


/****************************************************************************/
/*  Send to client functions                                                */
/****************************************************************************/

/* Callback function for update circut ID
   Return -1: circut ID is invalid
   Return  0: circut ID is valid */
static int DHCP_RELAY_update_circuit_id_callback(u8 *mac, uint transaction_id, u8 *circuit_id)
{
    dhcp_helper_frame_info_t    client_info;
    dhcp_relay_circuit_id_t     temp_circuit_id;

    if (!circuit_id || !dhcp_helper_frame_info_lookup(mac, 0, transaction_id, &client_info)) {
        /* Could not find the client interface */
        return -1;
    }

    /* caculate option 82 information */
    temp_circuit_id.vlan_id = htons(client_info.vid);

    temp_circuit_id.module_id = 0;

    temp_circuit_id.port_no = (u8)(iport2uport(client_info.port_no));

    memcpy(circuit_id, &temp_circuit_id, sizeof(temp_circuit_id));

    return 0;
}

/* Callback function for check circut ID
   Return -1: circut ID is invalid
   Return  0: circut ID is valid */
static int DHCP_RELAY_check_circuit_id_callback(u8 *mac, uint transaction_id, u8 *circuit_id)
{
    dhcp_helper_frame_info_t    client_info;
    dhcp_relay_circuit_id_t     temp_circuit_id;

    T_D("Enter, mac=%02x:%02x:%02x:%02x:%02x:%02x, transaction_id=0x%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], transaction_id);

    if (!circuit_id || !dhcp_helper_frame_info_lookup(mac, 0, transaction_id, &client_info)) {
        /* Could not find the client interface */
        T_D("Exit: Could not find the client interface");
        return -1;
    }

    /* caculate option 82 information */
    temp_circuit_id.vlan_id = htons(client_info.vid);

    temp_circuit_id.module_id = 0;

    temp_circuit_id.port_no = (u8)(iport2uport(client_info.port_no));

    if (memcmp(&temp_circuit_id, circuit_id, sizeof(temp_circuit_id))) {
        T_D("Exit: temp_circuit_id is different between circuit_id");
        return -1;
    }

    T_D("Exit");
    return 0;
}

/* Callback function for send DHCP message to client
   Return -1: send packet fail
   Return  0: send packet success */
static int DHCP_RELAY_send_client_callback(char *raw, size_t len, struct sockaddr_in *to, u8 *mac, uint transaction_id)
{
    u8                          *pkt_buf;
    void                        *bufref;
    int                         pkt_len = 14 + 20 + 8 + len;
    u8                          broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    struct ip                   *ip_hdr;
    struct udphdr               *udp_hdr;
    u8                          system_mac_addr[6];
    char                        buf[24];
    vtss_appl_ip_if_status_t    ifstat;
    dhcp_helper_frame_info_t    client_info;
    mesa_ip_addr_t              sip, dip;
    static u16                  DHCP_RELAY_send_to_client_ip_id = 0;
    int                         rc;

    T_D("Enter. to = %s, mac = %s, xid = 0x%08x", inet_ntoa(to->sin_addr), misc_mac_txt(mac, buf), transaction_id);

    if (!dhcp_helper_frame_info_lookup(mac, 0, transaction_id, &client_info)) {
        /* Could not find the client interface */
        return -1;
    }

    if ((pkt_buf = (u8 *)dhcp_helper_alloc_xmit(pkt_len, client_info.isid, &bufref)) == NULL) {
        /* Allocate fail */
        return -1;
    }

    //da, sa, type
    memset(pkt_buf, 0x0, pkt_len);
    memcpy(pkt_buf, broadcast_mac, 6);
    if (conf_mgmt_mac_addr_get(system_mac_addr, 0) < 0) {
        return -1;
    }
    memcpy(pkt_buf + 6, system_mac_addr, 6);
    *(pkt_buf + 12) = 0x08; //IP
    *(pkt_buf + 13) = 0x0;

    /* IP header */
    sip.type = MESA_IP_TYPE_IPV4;
    if (DHCP_RELAY_get_linkup_ipv4_interface_status(client_info.vid, &ifstat)) {
        sip.addr.ipv4 = ifstat.u.ipv4.net.address;
    } else {
        sip.addr.ipv4 = 0;
    }

    dip.type = MESA_IP_TYPE_IPV4;
    dip.addr.ipv4 = 0xffffffff; // IP broadcast

    ip_hdr = (struct ip *)(pkt_buf + 14);
    ip_hdr->ip_hl = 0x5;
    ip_hdr->ip_v = 0x4;
    ip_hdr->ip_tos = 0x0;
    DHCP_RELAY_CRIT_ENTER();
    ip_hdr->ip_id = htons((++DHCP_RELAY_send_to_client_ip_id));
    DHCP_RELAY_CRIT_EXIT();
    ip_hdr->ip_off = 0x0;
    ip_hdr->ip_ttl = 128;
    ip_hdr->ip_p   = IP_PROTO_UDP;
    ip_hdr->ip_src.s_addr = htonl(sip.addr.ipv4);
    ip_hdr->ip_dst.s_addr = htonl(dip.addr.ipv4);
    ip_hdr->ip_len = htons(20 + 8 + len);
    ip_hdr->ip_sum = 0; //clear before do checksum
    ip_hdr->ip_sum = htons(vtss_ip_checksum(&pkt_buf[14], 20));

    /* UDP header */
    udp_hdr = (struct udphdr *)(pkt_buf + 14 + 20);
    udp_hdr->uh_sport = htons(67);
    udp_hdr->uh_dport = htons(68);
    udp_hdr->uh_ulen = htons(8 + len);

    //dhcp message
    memcpy(pkt_buf + 14 + 20 + 8, raw, len);
    udp_hdr->uh_sum = 0; //clear before do checksum
    udp_hdr->uh_sum = htons(vtss_ip_pseudo_header_checksum(&pkt_buf[14 + 20], len + 8, sip, dip, IP_PROTO_UDP));

    rc = dhcp_helper_xmit(DHCP_HELPER_USER_RELAY, pkt_buf, pkt_len, client_info.vid, client_info.isid,
                          VTSS_BIT64(client_info.port_no), FALSE, VTSS_ISID_END, VTSS_PORT_NO_NONE, VTSS_GLAG_NO_NONE, bufref);
    return (rc);
}

/* Callback function for DHCP Relay base module after send out the DHCP packet successfully.
   It is used to count the per-port statistic. */
static void DHCP_RELAY_send_server_callback(char *raw, size_t len, uint32_t srv_ip)
{
    vtss_appl_ip_neighbor_key_t    prev_key, key;
    vtss_appl_ip_neighbor_status_t status;
    vtss_isid_t                    isid_idx;
    mesa_vid_mac_t                 vid_mac;
    mesa_mac_table_entry_t         mac_entry;
    char                           ip_str_buf[16];
    bool                           first = true, found = false;

    T_D("Enter. srv_ip = %s", misc_ipv4_txt(srv_ip, ip_str_buf));

    memset(&vid_mac, 0, sizeof(vid_mac));

    // Lookup MAC address in ARP table
    while (vtss_appl_ip_neighbor_itr(first ? nullptr : &prev_key, &key, MESA_IP_TYPE_IPV4) == VTSS_RC_OK) {
        first    = false;
        prev_key = key;

        if (key.dip.addr.ipv4 != srv_ip) {
            continue;
        }

        if ((vid_mac.vid = vtss_ifindex_as_vlan(key.ifindex)) == 0) {
            continue;
        }

        if (vtss_appl_ip_neighbor_status_get(&key, &status) != VTSS_RC_OK) {
            continue;
        }

        vid_mac.mac = status.dmac;

        T_D("Found DHCP Relay server IP address %s", misc_ipv4_txt((mesa_ipv4_t)srv_ip, ip_str_buf));
        found = true;
        break;
    }

    if (!found) {
        return;
    }

    /* Lookup source port in MAC address table */
    for (isid_idx = VTSS_ISID_START; isid_idx < VTSS_ISID_END; isid_idx++) {
        if (!msg_switch_exists(isid_idx)) {
            continue;
        }

        if (mac_mgmt_table_get_next(isid_idx, &vid_mac, &mac_entry, FALSE) == VTSS_RC_OK) {
            mesa_port_no_t port_idx;
            for (port_idx = VTSS_PORT_NO_START; port_idx < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_idx++) {
                if (mac_entry.destination[port_idx]) {
                    struct dhcp_packet  bp;
                    u8                  dhcp_message;

                    memcpy(&bp, raw, sizeof(bp));
                    dhcp_message = bp.options[6];
                    DHCP_HELPER_stats_add(DHCP_HELPER_USER_RELAY, isid_idx, VTSS_BIT64(port_idx), dhcp_message, DHCP_HELPER_DIRECTION_TX);
                    break;
                }
            }

            break;
        }
    }
}

static void DHCP_RELAY_fill_giaddr_callback(u8 *mac, uint transaction_id, mesa_ipv4_t *agent_ipv4_addr)
{
    mesa_vid_t                avail_ifid;
    vtss_appl_ip_if_status_t  status;

    T_D("Enter");

    // Use the available interface IP address to fill the field of "Giaddr"

    DHCP_RELAY_CRIT_ENTER();
    avail_ifid = DHCP_RELAY_global.avail_ifid;
    DHCP_RELAY_CRIT_EXIT();

    if (DHCP_RELAY_get_linkup_ipv4_interface_status(avail_ifid, &status)) {
        *agent_ipv4_addr = htonl(status.u.ipv4.net.address);
        T_D("Set server_vlan_ipv4_addr=0x%x", *agent_ipv4_addr);
    }
}

#if DHCP_RELAY_USING_ISCDHCP_PACKAGE
/****************************************************************************/
/*  DHCP Base Module callout implementations                                */
/****************************************************************************/
void *dhcp_relay_callout_malloc(size_t size)
{
    return VTSS_MALLOC(size);
}

void dhcp_relay_callout_free(void *ptr)
{
    VTSS_FREE(ptr);
}

char *dhcp_relay_callout_strdup(const char *str)
{
    return VTSS_STRDUP(str);
}
#endif

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\dhcp_relay.h

==============================================================================
*/
/**
 * Get DHCP Relay Parameters
 *
 * To read current system parameters in DHCP relay.
 *
 * \param param [OUT] The DHCP relay system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_system_config_get(
    vtss_appl_dhcp_relay_param_t     *const param
)
{
    dhcp_relay_conf_t   conf;

    /* check parameter */
    if ( param == NULL ) {
        T_W("param == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get global configuration */
    memset( &conf, 0, sizeof(conf) );

    if ( dhcp_relay_mgmt_conf_get(&conf) != VTSS_RC_OK ) {
        T_W("dhcp_relay_mgmt_conf_get()\n");
        return VTSS_RC_ERROR;
    }

    /* pack output */
    param->mode = conf.relay_mode;
    if ( conf.relay_server_cnt ) {
        param->serverIpAddr = conf.relay_server[0];
    } else {
        param->serverIpAddr = 0;
    }
    param->informationMode = conf.relay_info_mode;
    param->informationPolicy = (vtss_appl_dhcp_relay_information_policy_t)(conf.relay_info_policy);

    return VTSS_RC_OK;
}

/**
 * Set DHCP Relay Parameters
 *
 * To modify current system parameters in DHCP relay.
 *
 * \param param [IN] The DHCP relay system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_system_config_set(
    const vtss_appl_dhcp_relay_param_t   *const param
)
{
    dhcp_relay_conf_t   conf;

    /* check parameter */
    if ( param == NULL ) {
        T_W("param == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* pack input */
    memset( &conf, 0, sizeof(conf) );

    conf.relay_mode         = param->mode;
    conf.relay_server_cnt   = 1;
    conf.relay_server[0]    = param->serverIpAddr;
    conf.relay_info_mode    = param->informationMode;
    conf.relay_info_policy  = param->informationPolicy;

    /* set global configuration */
    if ( dhcp_relay_mgmt_conf_set(&conf) != VTSS_RC_OK ) {
        T_W("dhcp_relay_mgmt_conf_set()\n");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * Get DHCP Relay Statistics
 *
 * To read current statistics in DHCP relay.
 *
 * \param statistics [OUT] The DHCP relay statistics
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_statistics_get(
    vtss_appl_dhcp_relay_statistics_t   *const statistics
)
{
    /* check parameter */
    if ( statistics == NULL ) {
        T_W("statistics == NULL\n");
        return VTSS_RC_ERROR;
    }

    /*
        because the data structure is the same and dhcp_relay_stats_get()
        without return value, it is ok to reuse statistics.
    */
    memset(statistics, 0, sizeof(vtss_appl_dhcp_relay_statistics_t));
    dhcp_relay_stats_get( (dhcp_relay_stats_t *)statistics );

    return VTSS_RC_OK;
}

/**
 * Get DHCP Relay Control
 *
 * To read current action parameters in DHCP relay.
 *
 * \param control [OUT] The DHCP relay action data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_control_get(
    vtss_appl_dhcp_relay_control_t   *const control
)
{
    /* check parameter */
    if ( control == NULL ) {
        T_W("control == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* just return all 0's */
    memset( control, 0, sizeof(vtss_appl_dhcp_relay_control_t) );
    return VTSS_RC_OK;
}

/**
 * Set IP Source Guard Control
 *
 * To do the action in DHCP relay.
 *
 * \param control [IN] What to do
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_control_set(
    const vtss_appl_dhcp_relay_control_t     *const control
)
{
    /* check parameter */
    if ( control == NULL ) {
        T_W("control == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* do action */
    if ( control->clearStatistics ) {
        dhcp_relay_stats_clear();
    }

    return VTSS_RC_OK;
}

