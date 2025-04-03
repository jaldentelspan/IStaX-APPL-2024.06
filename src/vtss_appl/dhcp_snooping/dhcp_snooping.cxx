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

#include "main.h"
#include "port_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "dhcp_snooping_api.h"
#include "dhcp_snooping.h"
#include "dhcp_helper_api.h"
#include "packet_api.h"
#include "vlan_api.h"
#include "mac_api.h"
#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif /* VTSS_SW_OPTION_SYSLOG */
#ifdef VTSS_SW_OPTION_ICFG
#include "dhcp_snooping_icfg.h"
#endif




// The following is controlled from outside.
#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
#include "psec_api.h"
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */

// For public header
#include "vtss_common_iterator.hxx"
//#include "ifIndex_api.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_DHCP_SNOOPING

#ifndef IP_VHL_HL
#define IP_VHL_HL(vhl)      ((vhl) & 0x0f)
#endif /* IP_VHL_HL */

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static dhcp_snooping_global_t DHCP_SNOOPING_global;

static vtss_trace_reg_t DHCP_SNOOPING_trace_reg = {
    VTSS_TRACE_MODULE_ID, "dhcp_snoop", "DHCP Snooping"
};

static vtss_trace_grp_t DHCP_SNOOPING_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&DHCP_SNOOPING_trace_reg, DHCP_SNOOPING_trace_grps);

#define DHCP_SNOOPING_CRIT_ENTER() critd_enter(&DHCP_SNOOPING_global.crit, __FILE__, __LINE__)
#define DHCP_SNOOPING_CRIT_EXIT()  critd_exit( &DHCP_SNOOPING_global.crit, __FILE__, __LINE__)

/* Thread variables */
static vtss_handle_t DHCP_SNOOPING_thread_handle;
static vtss_thread_t DHCP_SNOOPING_thread_block;

struct DHCP_SNOOPING_ip_assigned_info_list {
    struct DHCP_SNOOPING_ip_assigned_info_list  *next;
    dhcp_snooping_ip_assigned_info_t            info;
} *DHCP_SNOOPING_ip_assigned_info;


/******************************************************************************/
/*  IP Assignment callback function                                           */
/******************************************************************************/
#define DHCP_SNOOPING_IP_ASSIGNED_INFO_MAX_CALLBACK_NUM  2

typedef struct {
    int                                         active;
    dhcp_snooping_ip_assigned_info_callback_t   callback;
} dhcp_snooping_ip_assigned_info_callback_list_t;

static dhcp_snooping_ip_assigned_info_callback_list_t DHCP_SNOOPING_ip_assigned_info_callback_list[DHCP_SNOOPING_IP_ASSIGNED_INFO_MAX_CALLBACK_NUM];

/* Register IP assigned information */
void dhcp_snooping_ip_assigned_info_register(dhcp_snooping_ip_assigned_info_callback_t cb)
{
    uint idx, inactive_idx = ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list), found_inactive_flag = 0;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    for (idx = 0; idx < ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list); idx++) {
        if (DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active && DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback == cb) {
            //found the same callback, do nothing and return
            DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active = 1;
            DHCP_SNOOPING_CRIT_EXIT();
            return;
        }
        if (found_inactive_flag == 0 && DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active == 0) {
            //get the first incative index
            inactive_idx = idx;
            found_inactive_flag = 1;
        }
    }

    if (inactive_idx == ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list)) { //callback list full
        DHCP_SNOOPING_CRIT_EXIT();
        return;
    }

    //active the callback to callback list[found_inactive_flag]
    DHCP_SNOOPING_ip_assigned_info_callback_list[inactive_idx].active = 1;
    DHCP_SNOOPING_ip_assigned_info_callback_list[inactive_idx].callback = cb;
    DHCP_SNOOPING_CRIT_EXIT();
}

/* Unregister IP assigned information */
void dhcp_snooping_ip_assigned_info_unregister(dhcp_snooping_ip_assigned_info_callback_t cb)
{
    uint idx;

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    for (idx = 0; idx < ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list); idx++) {
        if (DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active && DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback == cb) {
            DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active = 0;
            DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback = NULL;
            break;
        }
    }
    DHCP_SNOOPING_CRIT_EXIT();
}


/****************************************************************************/
/*  Volatile MAC add/delete functions                                       */
/****************************************************************************/
#if 0
/* Add volatile MAC entry */
static mesa_rc DHCP_SNOOPING_volatile_mac_add(vtss_isid_t isid, mesa_port_no_t port_no, mesa_vid_t vid, u8 *mac)
{
    mac_mgmt_addr_entry_t entry;
    mesa_rc               rc;

    memset(&entry, 0, sizeof(entry));
    memcpy(entry.vid_mac.mac.addr, mac, 6);
    entry.vid_mac.vid       = vid;
    entry.not_stack_ports   = 1; // Always
    entry.volatil           = 1; // Always
    entry.copy_to_cpu       = 0;

    entry.destination[port_no] = 1;
    rc = mac_mgmt_table_add(isid, &entry);

    return rc;
}

/* Delete volatile MAC entry */
static mesa_rc DHCP_SNOOPING_volatile_mac_del(vtss_isid_t isid, mesa_vid_t vid, u8 *mac)
{
    mesa_vid_mac_t  entry;
    mesa_rc         rc;

    entry.vid = vid;
    memcpy(entry.mac.addr, mac, 6);

    rc = mac_mgmt_table_del(isid, &entry, 1);

    return rc;
}
#endif

/****************************************************************************/
/*  IP assigned information entry control functions                         */
/****************************************************************************/
/*lint -e{593} */
/* There is a lint error message: Custodial pointer 'sp' possibly not freed or returned.
   We skip the lint error cause we freed it in DHCP_SNOOPING_ip_assigned_info_del() */
/* Add DHCP snooping IP assigned information entry */
static void DHCP_SNOOPING_ip_assigned_info_add(dhcp_snooping_ip_assigned_info_t *info)
{
    struct DHCP_SNOOPING_ip_assigned_info_list  *sp, *search_sp, *prev_sp = NULL;
    int                                         rc;
    BOOL                                        insert_first = FALSE;

    T_D("enter");

    DHCP_SNOOPING_CRIT_ENTER();
    search_sp = DHCP_SNOOPING_ip_assigned_info;
    if (!search_sp ||
        (search_sp && (((rc = memcmp(info->mac, search_sp->info.mac, 6)) < 0) || (rc == 0 && info->vid < search_sp->info.vid)))) {
        insert_first = TRUE;
        T_D("Entry will be added to the front of the table");
    } else {
        /* search entry exist? */
        for (; search_sp; search_sp = search_sp->next) {
            rc = memcmp(info->mac, search_sp->info.mac, 6);
            if (rc == 0 && info->vid == search_sp->info.vid) { //exact match
                /* found it, update content */
                search_sp->info = *info;
                DHCP_SNOOPING_CRIT_EXIT();
                T_D("exit: snooping table entry updated.");
                return;
            } else if (rc < 0 || (rc == 0 && info->vid < search_sp->info.vid)) { //insert after the current entry
                break;
            }
            prev_sp = search_sp;
        }
    }

    if (DHCP_SNOOPING_global.frame_info_cnt > DHCP_HELPER_FRAME_INFO_MAX_CNT) {
#if defined(VTSS_SW_OPTION_SYSLOG)
        S_I("DHCP_SNOOPING-REACHED_LIMIT: Reach the DHCP Snooping frame information the maximum count");
#endif /* VTSS_SW_OPTION_SYSLOG */
    } else if ((sp = (struct DHCP_SNOOPING_ip_assigned_info_list *)VTSS_MALLOC(sizeof(*sp))) == NULL) {
        T_W("Unable to allocate %zu bytes for DHCP frame information", sizeof(*sp));
    } else {
        sp->info = *info;
        if (insert_first) {
            //insert to first entry
            sp->next = DHCP_SNOOPING_ip_assigned_info;
            DHCP_SNOOPING_ip_assigned_info = sp;
        } else if (prev_sp) {
            sp->next = prev_sp->next;
            prev_sp->next = sp;
        }
        DHCP_SNOOPING_global.frame_info_cnt++;
    }
    DHCP_SNOOPING_CRIT_EXIT();
    T_D("exit");
}

/* Delete DHCP snooping IP assigned information entry */
static BOOL DHCP_SNOOPING_ip_assigned_info_del(u8 *mac, mesa_vid_t vid)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp, *prev_sp = NULL;

    DHCP_SNOOPING_CRIT_ENTER();
    for (sp = DHCP_SNOOPING_ip_assigned_info; sp; sp = sp->next) {
        if (!memcmp(sp->info.mac, mac, sizeof(sp->info.mac)) && sp->info.vid == vid) {
            if (prev_sp == NULL) {
                DHCP_SNOOPING_ip_assigned_info = sp->next;
            } else {
                prev_sp->next = sp->next;
            }
            VTSS_FREE(sp);
            if (DHCP_SNOOPING_global.frame_info_cnt) {
                DHCP_SNOOPING_global.frame_info_cnt--;
            }
            DHCP_SNOOPING_CRIT_EXIT();
            return TRUE;
        }
        prev_sp = sp;
    }
    DHCP_SNOOPING_CRIT_EXIT();
    return FALSE;
}

/* Lookup DHCP snooping IP assigned information entry */
static BOOL DHCP_SNOOPING_ip_assigned_info_lookup(u8 *mac, mesa_vid_t vid, dhcp_snooping_ip_assigned_info_t *info)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;
    while (sp) {
        if (!memcmp(sp->info.mac, mac, 6) && sp->info.vid == vid) {
            *info = sp->info;
            DHCP_SNOOPING_CRIT_EXIT();
            return TRUE;
        }
        sp = sp->next;
    }

    DHCP_SNOOPING_CRIT_EXIT();
    return FALSE;
}

/* Getnext DHCP snooping IP assigned information entry */
BOOL dhcp_snooping_ip_assigned_info_getnext(u8 *mac, mesa_vid_t vid, dhcp_snooping_ip_assigned_info_t *info)
{
    u8 null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    DHCP_SNOOPING_CRIT_ENTER();
    if (!DHCP_SNOOPING_ip_assigned_info) {
        DHCP_SNOOPING_CRIT_EXIT();
        return FALSE;
    }

    if (!memcmp(null_mac, mac, 6) && vid == 0) {
        *info = DHCP_SNOOPING_ip_assigned_info->info;
        DHCP_SNOOPING_CRIT_EXIT();
        return TRUE;
    } else {
        struct DHCP_SNOOPING_ip_assigned_info_list  *sp;
        dhcp_snooping_ip_assigned_info_t            search_entry;
        int                                         rc;

        memcpy(search_entry.mac, mac, 6);
        search_entry.vid = vid;

        sp = DHCP_SNOOPING_ip_assigned_info;
        while (sp) {
            if ((rc = memcmp(sp->info.mac, search_entry.mac, 6)) > 0 || (rc == 0 && sp->info.vid > search_entry.vid)) {
                *info = sp->info;
                DHCP_SNOOPING_CRIT_EXIT();
                return TRUE;
            }
            sp = sp->next;
        }

        DHCP_SNOOPING_CRIT_EXIT();
        return FALSE;
    }
}

/* Clear DHCP snooping IP assigned information entry */
static void DHCP_SNOOPING_ip_assigned_info_clear(void)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;
    while (sp) {
        DHCP_SNOOPING_ip_assigned_info = sp->next;
        VTSS_FREE(sp);
        sp = DHCP_SNOOPING_ip_assigned_info;
    }
    DHCP_SNOOPING_global.frame_info_cnt = 0;
    DHCP_SNOOPING_CRIT_EXIT();
}

/* Clear DHCP snooping IP assigned information entry by port */
static void DHCP_SNOOPING_ip_assigned_info_clear_port(vtss_isid_t isid, mesa_port_no_t port_no)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp, *prev_sp = NULL;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;

    /* clear entries by mac */
    while (sp) {
        if (isid == sp->info.isid && port_no == sp->info.port_no) {
            if (prev_sp) {
                prev_sp->next = sp->next;
                VTSS_FREE(sp);
                sp = prev_sp->next;
            } else {
                DHCP_SNOOPING_ip_assigned_info = sp->next;
                VTSS_FREE(sp);
                sp = DHCP_SNOOPING_ip_assigned_info;
            }
            if (DHCP_SNOOPING_global.frame_info_cnt) {
                DHCP_SNOOPING_global.frame_info_cnt--;
            }
            continue;
        }
        prev_sp = sp;
        sp = sp->next;
    }
    DHCP_SNOOPING_CRIT_EXIT();
}

/* DHCP snooping IP assigned information callback */
void dhcp_snooping_ip_assigned_info_callback(dhcp_helper_frame_info_t *info, dhcp_snooping_info_reason_t reason)
{
    uint idx;

    DHCP_SNOOPING_CRIT_ENTER();
#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
    if (DHCP_SNOOPING_global.conf.snooping_mode == DHCP_SNOOPING_MGMT_ENABLED) {
        if (DHCP_SNOOPING_global.port_conf[info->isid].port_mode[info->port_no] == DHCP_SNOOPING_PORT_MODE_UNTRUSTED &&
            DHCP_SNOOPING_global.port_conf[info->isid].veri_mode[info->port_no] == DHCP_SNOOPING_MGMT_ENABLED) {
            if (reason == DHCP_SNOOPING_INFO_REASON_ASSIGN_COMPLETED) {
                mesa_vid_mac_t vid_mac;
                vid_mac.vid = info->vid;
                memcpy(vid_mac.mac.addr, info->mac, 6);
                if (psec_mgmt_mac_chg(VTSS_APPL_PSEC_USER_DHCP_SNOOPING, info->isid, info->port_no, &vid_mac, PSEC_ADD_METHOD_FORWARD) != VTSS_RC_OK) {
                    T_D("psec_mgmt_mac_chg()failed.");
                }
            }
        }
    }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */

    for (idx = 0; idx < ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list); idx++) {
        if (DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active && DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback) {
            DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback(info, reason);
        }
    }
    DHCP_SNOOPING_CRIT_EXIT();
}

/* The callback function for DHCP client get dynamic IP address completed */
void dhcp_snooping_ip_addr_obtained_callback(dhcp_helper_frame_info_t *info)
{
    dhcp_snooping_ip_assigned_info_t lookup_info;

    if (info->vid > VTSS_APPL_VLAN_ID_MAX || !msg_switch_exists(info->isid) || !info->assigned_ip || !info->lease_time) {
        return;
    }
    if (DHCP_SNOOPING_ip_assigned_info_lookup(info->mac, info->vid, &lookup_info)) {
        dhcp_snooping_ip_assigned_info_callback(&lookup_info, DHCP_SNOOPING_INFO_REASON_ENTRY_DUPLEXED);
    }
    DHCP_SNOOPING_ip_assigned_info_add(info);
    dhcp_snooping_ip_assigned_info_callback(info, DHCP_SNOOPING_INFO_REASON_ASSIGN_COMPLETED);
}

/* The callback function DHCP client IP address released */
void dhcp_snooping_release_ip_addr_callback(dhcp_helper_frame_info_t *info)
{
    if (DHCP_SNOOPING_ip_assigned_info_del(info->mac, info->vid)) {
        dhcp_snooping_ip_assigned_info_callback(info, DHCP_SNOOPING_INFO_REASON_RELEASE);
    }
}

static void DHCP_SNOOPING_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    u8                               mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    mesa_vid_t                       vid = 0;
    dhcp_snooping_ip_assigned_info_t ip_assigned_info;
    vtss_isid_t                      isid = VTSS_ISID_START;

    if (msg_switch_is_primary()) {
        T_D("port_no: [%d,%u] link %s", isid, port_no, status->link ? "up" : "down");
        if (!msg_switch_exists(isid)) {
            return;
        }
        if (!status->link) {
            while (dhcp_snooping_ip_assigned_info_getnext(mac, vid, &ip_assigned_info)) {
                memcpy(mac, ip_assigned_info.mac, 6);
                vid = ip_assigned_info.vid;
                if (ip_assigned_info.isid == isid && ip_assigned_info.port_no == port_no) {
                    dhcp_snooping_ip_assigned_info_callback(&ip_assigned_info, DHCP_SNOOPING_INFO_REASON_PORT_LINK_DOWN);
                }
            }
            DHCP_SNOOPING_ip_assigned_info_clear_port(isid, port_no);
        }
    }
}

/****************************************************************************/
/*  compare functions                                                       */
/****************************************************************************/

/* Get DHCP snooping defaults */
void dhcp_snooping_mgmt_conf_get_default(dhcp_snooping_conf_t *conf)
{
    conf->snooping_mode = DHCP_SNOOPING_MGMT_DISABLED;
}

/* Determine if DHCP snooping configuration has changed */
int dhcp_snooping_mgmt_conf_changed(dhcp_snooping_conf_t *old, dhcp_snooping_conf_t *new_conf)
{
    return (memcmp(new_conf, old, sizeof(*new_conf)));
}

/* Get DHCP snooping port defaults */
void dhcp_snooping_mgmt_port_get_default(vtss_isid_t isid, dhcp_snooping_port_conf_t *conf)
{
    mesa_port_no_t  port_no;

    if (isid == VTSS_ISID_GLOBAL) {
        return;
    }

    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        conf->port_mode[port_no] = DHCP_SNOOPING_DEFAULT_PORT_MODE;
#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
        conf->veri_mode[port_no] = DHCP_SNOOPING_MGMT_ENABLED;
#else
        conf->veri_mode[port_no] = DHCP_SNOOPING_MGMT_DISABLED;
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */
    }
}

/* Determine if DHCP snooping port configuration has changed */
int dhcp_snooping_mgmt_port_conf_changed(dhcp_snooping_port_conf_t *old, dhcp_snooping_port_conf_t *new_conf)
{
    return vtss_memcmp(*new_conf, *old);
}

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

























































/*  Primary switch only. Receive packet from DHCP helper - send it to TCP/IP stack  */
static BOOL DHCP_SNOOPING_stack_receive_callback(const u8 *const packet, size_t len,
                                                 const dhcp_helper_frame_info_t *helper_info,
                                                 const dhcp_helper_rx_cb_flag_t flag)
{
    u8                          dhcp_message = helper_info->op_code;
    dhcp_helper_frame_info_t    record_info;

    T_D("enter, RX port %d glag %d len %zd", helper_info->port_no, helper_info->glag_no, len);

    if (!msg_switch_is_primary()) {
        return FALSE;
    }













































    if (DHCP_HELPER_MSG_FROM_SERVER(dhcp_message)) {
        /* Only send reply DHCP message to correct client port */
        record_info = *helper_info;

        //lookup source port, flooding it if cannot find it.
        if (dhcp_helper_frame_info_lookup(record_info.mac, record_info.vid, record_info.transaction_id, &record_info)) {
            //filter source port
            if (record_info.isid == helper_info->isid && record_info.vid == helper_info->vid && record_info.port_no == helper_info->port_no) {
                T_D("exit: Filter source port");
                return FALSE;
            }

            //transmit DHCP packet
            T_D("Transmitting packet from server to client port %u", record_info.port_no);
            if (dhcp_helper_xmit(DHCP_HELPER_USER_SNOOPING, packet, len, record_info.vid, record_info.isid,
                                 VTSS_BIT64(record_info.port_no), TRUE, VTSS_ISID_END, VTSS_PORT_NO_NONE, VTSS_GLAG_NO_NONE, NULL)) {
                T_W("Calling dhcp_helper_xmit() failed.\n");
            }
            T_D("exit: To source port");
            return TRUE;
        } else {
            T_D("frame_info_lookup failed");
        }
    }

    /* Forward to other trusted front ports. (DHCP Helper will do it) */
    if (dhcp_helper_xmit(DHCP_HELPER_USER_SNOOPING, packet, len, helper_info->vid, VTSS_ISID_GLOBAL, 0, FALSE, helper_info->isid, helper_info->port_no, helper_info->glag_no, NULL)) {
        T_D("Calling dhcp_helper_xmit() failed");
    }

    T_D("exit: Flooding");
    return TRUE;
}


/****************************************************************************/
// Port security callback functions
/****************************************************************************/

#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
/* Port security module MAC add callback function */
static psec_add_method_t DHCP_SNOOPING_psec_mac_add_cb(vtss_isid_t isid, mesa_port_no_t port_no, mesa_vid_mac_t *vid_mac, u32 mac_cnt_before_callback, vtss_appl_psec_user_t originating_user, psec_add_action_t *action)
{
    psec_add_method_t                           rc = PSEC_ADD_METHOD_BLOCK;
    struct DHCP_SNOOPING_ip_assigned_info_list  *sp;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;
    while (sp) {
        if (sp->info.isid == isid && sp->info.port_no == port_no) {
            rc = PSEC_ADD_METHOD_FORWARD;
            break;
        }
        sp = sp->next;
    }
    DHCP_SNOOPING_CRIT_EXIT();

    return rc;
}

/* Proess DHCP snooping MAC verification */
static mesa_rc DHCP_SNOOPING_veri_process(vtss_isid_t isid, mesa_port_no_t port_no, u32 enable)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_exists(isid)) {
        //Do nothing when the switch is non-existing
        T_D("exit");
        return VTSS_RC_OK;
    }

    if (enable == DHCP_SNOOPING_MGMT_ENABLED) {
        /* Set untrust port to MAC sercure learning */
        rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_DHCP_SNOOPING, isid, port_no, TRUE,  PSEC_PORT_MODE_NORMAL);
    } else {
        /* Revert port state */
        rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_DHCP_SNOOPING, isid, port_no, FALSE, PSEC_PORT_MODE_NORMAL);
    }

    T_D("exit");
    return rc;
}

/* Start proess DHCP snooping MAC verification */
static mesa_rc DHCP_SNOOPING_veri_process_start(vtss_isid_t isid, BOOL is_restart)
{
    mesa_rc                     rc = VTSS_RC_OK;
    port_iter_t                 pit;
    dhcp_snooping_conf_t        conf;
    u8                          port_mode, veri_mode;
    dhcp_snooping_port_conf_t   port_conf;

    T_D("enter, isid: %d, is_restart: %d", isid, is_restart);

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_exists(isid)) {
        //Do nothing when the switch is non-existing
        T_D("exit");
        return VTSS_RC_OK;
    }

    if (is_restart) {
        /* Initialize DHCP snooping port configuration */
        dhcp_snooping_mgmt_port_get_default(isid, &port_conf);
        (void) dhcp_snooping_mgmt_port_conf_set(isid, &port_conf);
    } else {
        DHCP_SNOOPING_CRIT_ENTER();
        conf = DHCP_SNOOPING_global.conf;

        /* Set DHCP snooping MAC verification configuration */
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            port_mode = DHCP_SNOOPING_global.port_conf[isid].port_mode[pit.iport];
            veri_mode = DHCP_SNOOPING_global.port_conf[isid].veri_mode[pit.iport];
            if (conf.snooping_mode == DHCP_SNOOPING_MGMT_ENABLED &&
                port_mode == DHCP_SNOOPING_PORT_MODE_UNTRUSTED &&
                veri_mode == DHCP_SNOOPING_MGMT_ENABLED) {
                rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_ENABLED);
            }
        }
        DHCP_SNOOPING_CRIT_EXIT();
    }

    T_D("exit, isid: %d", isid);
    return rc;
}
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */

static void DHCP_SNOOPING_conf_apply(void)
{
    mesa_rc                             rc;
    u8                                  mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    mesa_vid_t                          vid = 0;
    dhcp_snooping_ip_assigned_info_t    ip_assigned_info;
    u32                                 snooping_mode;
    vtss_isid_t                         isid;
    mesa_port_no_t                      port_no;
    dhcp_helper_port_conf_t             dhcp_helper_port_conf;

    if (!msg_switch_is_primary()) {
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    snooping_mode = DHCP_SNOOPING_global.conf.snooping_mode;
    DHCP_SNOOPING_CRIT_EXIT();
    if (snooping_mode) {
        //Add ACEs again when topology change
        dhcp_helper_user_receive_register(DHCP_HELPER_USER_SNOOPING, DHCP_SNOOPING_stack_receive_callback);
    } else {
        dhcp_helper_user_receive_unregister(DHCP_HELPER_USER_SNOOPING);

        while (dhcp_snooping_ip_assigned_info_getnext(mac, vid, &ip_assigned_info)) {
            memcpy(mac, ip_assigned_info.mac, 6);
            vid = ip_assigned_info.vid;
            dhcp_snooping_ip_assigned_info_callback(&ip_assigned_info, DHCP_SNOOPING_INFO_REASON_MODE_DISABLED);
        }

        DHCP_SNOOPING_ip_assigned_info_clear();
    }

    /* If the snooping mode is disabled, set all ports to trusted mode. */
    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        dhcp_helper_port_conf.port_mode[port_no] = DHCP_SNOOPING_PORT_MODE_TRUSTED;
    }

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (!msg_switch_configurable(isid)) {
            continue;
        }

        if (snooping_mode) {
            DHCP_SNOOPING_CRIT_ENTER();
            if ((rc = dhcp_helper_mgmt_port_conf_set(isid, (dhcp_helper_port_conf_t *) &DHCP_SNOOPING_global.port_conf[isid])) != VTSS_RC_OK) {
                T_W("Calling dhcp_helper_mgmt_port_conf_set(isid = %u): failed rc = %s", isid, dhcp_helper_error_txt(rc));
            }
            DHCP_SNOOPING_CRIT_EXIT();
        } else {
            if ((rc = dhcp_helper_mgmt_port_conf_set(isid, &dhcp_helper_port_conf)) != VTSS_RC_OK) {
                T_W("Calling dhcp_helper_mgmt_port_conf_set(isid = %u): failed rc = %s", isid, dhcp_helper_error_txt(rc));
            }
        }
    }
}

static void DHCP_SNOOPING_port_conf_apply(vtss_isid_t isid)
{
    mesa_rc rc;

    if (!msg_switch_is_primary() || !msg_switch_configurable(isid)) {
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    if (DHCP_SNOOPING_global.conf.snooping_mode) {
        if ((rc = dhcp_helper_mgmt_port_conf_set(isid, (dhcp_helper_port_conf_t *) &DHCP_SNOOPING_global.port_conf[isid])) != VTSS_RC_OK) {
            T_W("Calling dhcp_helper_mgmt_port_conf_set(isid = %u): failed rc = %s", isid, dhcp_helper_error_txt(rc));
        }
    }
    DHCP_SNOOPING_CRIT_EXIT();
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* DHCP snooping error text */
const char *dhcp_snooping_error_txt(mesa_rc rc)
{
    switch (rc) {
    case DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on the primary switch";

    case DHCP_SNOOPING_ERROR_ISID:
        return "Invalid Switch ID";

    case DHCP_SNOOPING_ERROR_ISID_NON_EXISTING:
        return "Switch ID is non-existing";

    case DHCP_SNOOPING_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    default:
        return "DHCP Snooping: Unknown error code";
    }
}

/* Get DHCP snooping configuration */
mesa_rc dhcp_snooping_mgmt_conf_get(dhcp_snooping_conf_t *glbl_cfg)
{
    T_D("enter");

    if (glbl_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    *glbl_cfg = DHCP_SNOOPING_global.conf;
    DHCP_SNOOPING_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set DHCP snooping configuration */
mesa_rc dhcp_snooping_mgmt_conf_set(dhcp_snooping_conf_t *glbl_cfg)
{
    mesa_rc       rc      = VTSS_RC_OK;
    int           changed = 0;
#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
    vtss_isid_t   isid;
    port_iter_t   pit;
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */

    T_D("enter, mode: %d", glbl_cfg->snooping_mode);

    if (glbl_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    /* Check illegal parameter */
    if (glbl_cfg->snooping_mode != DHCP_SNOOPING_MGMT_ENABLED && glbl_cfg->snooping_mode != DHCP_SNOOPING_MGMT_DISABLED) {
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    changed = dhcp_snooping_mgmt_conf_changed(&DHCP_SNOOPING_global.conf, glbl_cfg);
    DHCP_SNOOPING_global.conf = *glbl_cfg;
    DHCP_SNOOPING_CRIT_EXIT();

    if (changed) {
        /* Activate changed configuration */
        DHCP_SNOOPING_conf_apply();

#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!msg_switch_configurable(isid)) {
                continue;
            }
            DHCP_SNOOPING_CRIT_ENTER();
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (DHCP_SNOOPING_global.port_conf[isid].veri_mode[pit.iport] == DHCP_SNOOPING_MGMT_DISABLED ||
                    DHCP_SNOOPING_global.port_conf[isid].port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED) {
                    continue;
                }
                if (DHCP_SNOOPING_global.conf.snooping_mode == DHCP_SNOOPING_MGMT_DISABLED) {
                    //enabled to disabled
                    if ((rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_DISABLED)) != VTSS_RC_OK) {
                        T_W("Calling DHCP_SNOOPING_veri_process(isid = %u): failed rc = %s", isid, dhcp_snooping_error_txt(rc));
                    }
                } else {
                    //disabled to enabled
                    if ((rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_ENABLED)) != VTSS_RC_OK) {
                        T_W("Calling DHCP_SNOOPING_veri_process(isid = %u): failed rc = %s", isid, dhcp_snooping_error_txt(rc));
                    }
                }
            }
            DHCP_SNOOPING_CRIT_EXIT();
        }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */
    }

    T_D("exit");

    return rc;
}

/* Get DHCP snooping port configuration */
mesa_rc dhcp_snooping_mgmt_port_conf_get(vtss_isid_t isid, dhcp_snooping_port_conf_t *switch_cfg)
{
    T_D("enter");

    if (switch_cfg == NULL) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return DHCP_SNOOPING_ERROR_ISID_NON_EXISTING;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    *switch_cfg = DHCP_SNOOPING_global.port_conf[isid];
    DHCP_SNOOPING_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set DHCP snooping port configuration */
mesa_rc dhcp_snooping_mgmt_port_conf_set(vtss_isid_t isid, dhcp_snooping_port_conf_t *switch_cfg)
{
    mesa_rc                   rc      = VTSS_RC_OK;
    int                       changed = 0;
#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
    dhcp_snooping_port_conf_t original_conf;
    port_iter_t               pit;
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */

    T_D("enter");

    if (switch_cfg == NULL) {
        T_D("exit");
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return DHCP_SNOOPING_ERROR_ISID_NON_EXISTING;
    }

    DHCP_SNOOPING_CRIT_ENTER();
#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
    original_conf = DHCP_SNOOPING_global.port_conf[isid];
#else
    switch_cfg->veri_mode = DHCP_SNOOPING_global.port_conf[isid].veri_mode;
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */
    changed = dhcp_snooping_mgmt_port_conf_changed(&DHCP_SNOOPING_global.port_conf[isid], switch_cfg);
    DHCP_SNOOPING_global.port_conf[isid] = *switch_cfg;
    DHCP_SNOOPING_CRIT_EXIT();

    if (changed) {
        /* Activate changed configuration */
        DHCP_SNOOPING_port_conf_apply(isid);

#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
        DHCP_SNOOPING_CRIT_ENTER();
        if (DHCP_SNOOPING_global.conf.snooping_mode == DHCP_SNOOPING_MGMT_ENABLED) {
            /* Process MAC verification */
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (switch_cfg->veri_mode[pit.iport] != original_conf.veri_mode[pit.iport]) {
                    if (DHCP_SNOOPING_global.port_conf[isid].port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED) {
                        continue;
                    }
                    if (switch_cfg->veri_mode[pit.iport] == DHCP_SNOOPING_MGMT_ENABLED) {
                        //MAC verification disabled to enabled
                        rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_ENABLED);
                    } else {
                        //MAC verification enabled to disabled
                        rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_DISABLED);
                    }
                } else if (switch_cfg->veri_mode[pit.iport] == DHCP_SNOOPING_MGMT_ENABLED) {
                    if (switch_cfg->port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_UNTRUSTED &&
                        original_conf.port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED) {
                        //trusted to untrusted
                        rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_ENABLED);
                    } else if (switch_cfg->port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED &&
                               original_conf.port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_UNTRUSTED) {
                        //untrusted to trusted
                        rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_DISABLED);
                    }
                }
            }
        }
        DHCP_SNOOPING_CRIT_EXIT();
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */
    }

    T_D("exit");

    return rc;
}


/****************************************************************************
 * Module thread
 ****************************************************************************/
/* Caculate the lease time */
static void DHCP_SNOOPING_thread(vtss_addrword_t data)
{
    dhcp_snooping_ip_assigned_info_t    info;
    vtss_tick_count_t                   sleep_time, defalt_sleep_time = 60000, cur_time = 0;
    u8                                  mac[6], null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    mesa_vid_t                          vid;
    BOOL                                rc;
    uint                                entry_cnt;

    sleep_time = defalt_sleep_time;
    while (1) {
        if (msg_switch_is_primary()) {
            while (msg_switch_is_primary()) {
                VTSS_OS_MSLEEP(sleep_time);

                memcpy(mac, null_mac, sizeof(mac));
                vid = 0;
                cur_time = (u32) vtss_current_time();
                entry_cnt = 0;

                do {
                    rc = dhcp_snooping_ip_assigned_info_getnext(mac, vid, &info);
                    if (rc) {
                        entry_cnt++;

                        if ((VTSS_OS_TICK2MSEC(cur_time - info.timestamp) / 1000 /* seconds */) >= info.lease_time) {
                            if (DHCP_SNOOPING_ip_assigned_info_del(info.mac, info.vid)) {
                                entry_cnt--;
                                dhcp_snooping_ip_assigned_info_callback(&info, DHCP_SNOOPING_INFO_REASON_LEASE_TIMEOUT);
                            }
                        } else {
                            /* Select the minimun lease time as next sleep time */
                            vtss_tick_count_t new_sleep =
                                VTSS_OS_MSEC2TICK(info.lease_time * 1000) - (cur_time - info.timestamp);
                            if (sleep_time > new_sleep) {
                                sleep_time = new_sleep;
                            }
                            memcpy(mac, info.mac, 6);
                            vid = info.vid;
                        }
                    }

                    if (!entry_cnt) {
                        sleep_time = defalt_sleep_time;
                    }
                } while (rc);
            } // while (msg_switch_is_primary())
        } // if(msg_switch_is_primary())

        // No reason for using CPU ressources when we're not ready.
        T_D("Suspending DHCP snooping thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_DHCP_SNOOPING);
        T_D("Resumed DHCP snooping thread");
    } // while(1)
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create DHCP snooping switch configuration */
static void DHCP_SNOOPING_conf_read_switch(vtss_isid_t isid_add)
{
    dhcp_snooping_port_conf_t       *port_conf, new_port_conf;
    int                             changed;
    vtss_isid_t                     isid;

    T_D("enter, isid_add: %d", isid_add);

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (isid_add != VTSS_ISID_GLOBAL && isid_add != isid) {
            continue;
        }

        changed = 0;
        DHCP_SNOOPING_CRIT_ENTER();
        /* Use default values */
        dhcp_snooping_mgmt_port_get_default(isid, &new_port_conf);

        port_conf = &DHCP_SNOOPING_global.port_conf[isid];
        if (dhcp_snooping_mgmt_port_conf_changed(port_conf, &new_port_conf)) {
            changed = 1;
        }
        *port_conf = new_port_conf;
        DHCP_SNOOPING_CRIT_EXIT();
        if (changed && isid_add != VTSS_ISID_GLOBAL && msg_switch_configurable(isid)) {
            DHCP_SNOOPING_port_conf_apply(isid);
        }
    }

    T_D("exit");
}

/* Read/create DHCP snooping stack configuration */
static void DHCP_SNOOPING_conf_read_stack(BOOL create)
{
    int                         changed;
    dhcp_snooping_conf_t        *old_dhcp_snooping_conf_p, new_dhcp_snooping_conf;

    T_D("enter, create: %d", create);

    changed = 0;
    DHCP_SNOOPING_CRIT_ENTER();
    /* Use default values */
    dhcp_snooping_mgmt_conf_get_default(&new_dhcp_snooping_conf);

    old_dhcp_snooping_conf_p = &DHCP_SNOOPING_global.conf;
    if (dhcp_snooping_mgmt_conf_changed(old_dhcp_snooping_conf_p, &new_dhcp_snooping_conf)) {
        changed = 1;
    }
    DHCP_SNOOPING_global.conf = new_dhcp_snooping_conf;
    DHCP_SNOOPING_CRIT_EXIT();

    if (changed && create) { //Always set when topology change
        DHCP_SNOOPING_conf_apply();
    }

    T_D("exit");
}

/* Module start */
static void DHCP_SNOOPING_start(BOOL init)
{
    dhcp_snooping_conf_t      *conf_p;
    dhcp_snooping_port_conf_t *port_conf_p;
    vtss_isid_t               isid;
    mesa_rc                   rc;

    T_D("enter, init: %d", init);
    if (init) {
        /* Initialize DHCP snooping configuration */
        conf_p = &DHCP_SNOOPING_global.conf;
        dhcp_snooping_mgmt_conf_get_default(conf_p);

        /* Initialize DHCP snooping port configuration */
        for (isid = VTSS_ISID_LOCAL; isid < VTSS_ISID_END; isid++) {
            port_conf_p = &DHCP_SNOOPING_global.port_conf[isid];
            dhcp_snooping_mgmt_port_get_default(isid, port_conf_p);
        }

        /* Create semaphore for critical regions */
        critd_init(&DHCP_SNOOPING_global.crit, "dhcp_snooping", VTSS_MODULE_ID_DHCP_SNOOPING, CRITD_TYPE_MUTEX);

        /* Create DHCP helper thread */
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           DHCP_SNOOPING_thread,
                           0,
                           "DHCP Snooping",
                           nullptr,
                           0,
                           &DHCP_SNOOPING_thread_handle,
                           &DHCP_SNOOPING_thread_block);
    } else {
        /* Register for frame information */
        dhcp_helper_notify_ip_addr_obtained_register(dhcp_snooping_ip_addr_obtained_callback);
        dhcp_helper_release_ip_addr_register(dhcp_snooping_release_ip_addr_callback);
        if ((rc = port_change_register(VTSS_MODULE_ID_DHCP_SNOOPING, DHCP_SNOOPING_state_change_callback)) != VTSS_RC_OK) {
            T_E("port_change_register() failed: rc = %d", rc);
        }

#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
        /* Set timer to port security module */
        if ((rc = psec_mgmt_time_conf_set(VTSS_APPL_PSEC_USER_DHCP_SNOOPING, 300/* 5 mins */, PSEC_HOLD_TIME_MAX)) != VTSS_RC_OK) {
            T_W("psec_mgmt_time_conf_set(): failed rc = %d", rc);
        }

        /* Register to port security module */
        if ((rc = psec_mgmt_register_callbacks(VTSS_APPL_PSEC_USER_DHCP_SNOOPING, DHCP_SNOOPING_psec_mac_add_cb, NULL)) != VTSS_RC_OK) {
            T_W("psec_mgmt_register_callbacks(): failed rc = %d", rc);
        }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */













    }

    T_D("exit");
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void dhcp_snooping_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_dhcp_snooping_json_init(void);
#endif
extern "C" int dhcp_snooping_icli_cmd_register();

/* Initialize module */
mesa_rc dhcp_snooping_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc     rc   = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        DHCP_SNOOPING_start(1);
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = dhcp_snooping_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling dhcp_snooping_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        dhcp_snooping_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_dhcp_snooping_json_init();
#endif
        dhcp_snooping_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");
        DHCP_SNOOPING_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            DHCP_SNOOPING_conf_read_stack(1);
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
            if ((rc = DHCP_SNOOPING_veri_process_start(isid, 1)) != VTSS_RC_OK) {
                T_D("Calling DHCP_SNOOPING_veri_process_start(isid = %u): failed rc = %s", isid, dhcp_snooping_error_txt(rc));
            }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */
            DHCP_SNOOPING_conf_read_switch(isid);
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        /* Read stack and switch configuration */
        DHCP_SNOOPING_conf_read_stack(0);
        DHCP_SNOOPING_conf_read_switch(VTSS_ISID_GLOBAL);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");

        /* Apply all configuration to switch */
        if (msg_switch_is_local(isid)) {
            DHCP_SNOOPING_conf_apply();
        }

        DHCP_SNOOPING_port_conf_apply(isid);

#ifdef DHCP_SNOOPING_MAC_VERI_SUPPORT
        if ((rc = DHCP_SNOOPING_veri_process_start(isid, 0)) != VTSS_RC_OK) {
            T_D("Calling DHCP_SNOOPING_veri_process_start(isid = %u): failed rc = %s", isid, dhcp_snooping_error_txt(rc));
        }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORT */

        break;

    default:
        break;
    }

    return rc;
}

/****************************************************************************/
/*  Statistics functions                                                    */
/****************************************************************************/

/* Get DHCP snooping statistics
   Return 0  : Success
   Return -1 : Fail */
int dhcp_snooping_stats_get(vtss_isid_t isid, mesa_port_no_t port_no, dhcp_snooping_stats_t *stats)
{
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return -1;
    }

    return (dhcp_helper_stats_get(DHCP_HELPER_USER_SNOOPING, isid, port_no, stats));
}

/* Clear DHCP snooping statistics
   Return 0  : Success
   Return -1 : Fail */
int dhcp_snooping_stats_clear(vtss_isid_t isid, mesa_port_no_t port_no)
{
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return -1;
    }

    return (dhcp_helper_stats_clear(DHCP_HELPER_USER_SNOOPING, isid, port_no));
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\dhcp_snooping.h

==============================================================================
*/
/**
 * Get DHCP Snooping System Parameters
 *
 * To read current system parameters in DHCP Snooping.
 *
 * \param param [OUT] The DHCP Snooping system configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_system_config_get(
    vtss_appl_dhcp_snooping_param_t      *const param
)
{
    dhcp_snooping_conf_t    conf;

    /* check parameter */
    if ( param == NULL ) {
        T_W("param == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get */
    if ( dhcp_snooping_mgmt_conf_get(&conf) != VTSS_RC_OK ) {
        T_W("dhcp_snooping_mgmt_conf_get()");
        return VTSS_RC_ERROR;
    }

    /* pack output */
    param->mode = conf.snooping_mode ? TRUE : FALSE;

    return VTSS_RC_OK;
}

/**
 * Set DHCP Snooping System Parameters
 *
 * To modify current system parameters in DHCP Snooping.
 *
 * \param param [IN] The DHCP Snooping system configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_system_config_set(
    const vtss_appl_dhcp_snooping_param_t    *const param
)
{
    dhcp_snooping_conf_t    conf;

    /* check parameter */
    if ( param == NULL ) {
        T_W("param == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* set */
    conf.snooping_mode = param->mode;
    if ( dhcp_snooping_mgmt_conf_set(&conf) != VTSS_RC_OK ) {
        T_W("dhcp_snooping_mgmt_conf_set()");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * Iterate function of DHCP Snooping Port Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_ifindex [IN]  previous ifindex.
 * \param next_ifindex [OUT] next ifindex.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_config_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
)
{
    return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
}

/**
 * Get DHCP Snooping Port Configuration
 *
 * To read configuration of the port in DHCP Snooping.
 *
 * \param ifindex   [IN]  (key 1) Interface index - the logical interface
 *                                index of the physical port.
 * \param port_conf [OUT] The current configuration of the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_config_get(
    vtss_ifindex_t                          ifindex,
    vtss_appl_dhcp_snooping_port_config_t   *const port_conf
)
{
    vtss_ifindex_elm_t          ife;
    dhcp_snooping_port_conf_t   conf;

    /* check parameter */
    if (port_conf == NULL) {
        T_W("port_conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* get mode of port */
    if (dhcp_snooping_mgmt_port_conf_get(ife.isid, &conf) != VTSS_RC_OK) {
        T_W("dhcp_snooping_mgmt_port_conf_get()\n");
        return VTSS_RC_ERROR;
    }

    /* pack output */
    port_conf->trustMode = (conf.port_mode[ife.ordinal] == DHCP_SNOOPING_PORT_MODE_TRUSTED) ? TRUE : FALSE;

    return VTSS_RC_OK;
}

/**
 * Set DHCP Snooping Port Configuration
 *
 * To modify configuration of the port in DHCP Snooping.
 *
 * \param ifindex   [IN] (key 1) Interface index - the logical interface index
 *                               of the physical port.
 * \param port_conf [IN] The configuration set to the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_config_set(
    vtss_ifindex_t                                  ifindex,
    const vtss_appl_dhcp_snooping_port_config_t     *const port_conf
)
{
    vtss_ifindex_elm_t          ife;
    dhcp_snooping_port_conf_t   conf;

    /* check parameter */
    if (port_conf == NULL) {
        T_W("port_conf == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* get mode of port */
    if (dhcp_snooping_mgmt_port_conf_get(ife.isid, &conf) != VTSS_RC_OK) {
        T_W("dhcp_snooping_mgmt_port_conf_get()\n");
        return VTSS_RC_ERROR;
    }

    /* set mode of port */
    conf.port_mode[ife.ordinal] = port_conf->trustMode ? DHCP_SNOOPING_PORT_MODE_TRUSTED : DHCP_SNOOPING_PORT_MODE_UNTRUSTED;

    if (dhcp_snooping_mgmt_port_conf_set(ife.isid, &conf) != VTSS_RC_OK) {
        T_W("dhcp_snooping_mgmt_port_conf_set()\n");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * Iterate function of DHCP Snooping Assigned IP Table
 *
 * To get first and get next indexes.
 *
 * \param prev_mac [IN]  previous MAC address.
 * \param next_mac [OUT] next MAC address.
 * \param prev_vid [IN]  previous VLAN ID.
 * \param next_vid [OUT] next VLAN ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_assigned_ip_itr(
    const mesa_mac_t        *const prev_mac,
    mesa_mac_t              *const next_mac,
    const mesa_vid_t        *const prev_vid,
    mesa_vid_t              *const next_vid
)
{
    u8                                  mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    mesa_vid_t                          vid = 0;
    dhcp_snooping_ip_assigned_info_t    info;

    /* check parameter */
    if ( next_mac == NULL || next_vid == NULL ) {
        T_W("next_mac == NULL || next_vid == NULL\n");
        return VTSS_RC_ERROR;
    }

    if ( prev_mac ) {
        /*
            get next
        */
        /* key 1 */
        memcpy(mac, prev_mac, 6);

        if ( prev_vid ) {
            /* key 2 */
            vid = *prev_vid;
        }
    }

    /* get first or next */
    if ( dhcp_snooping_ip_assigned_info_getnext(mac, vid, &info) == FALSE ) {
        return VTSS_RC_ERROR;
    }

    /* key 1 */
    memcpy(next_mac, info.mac, 6);

    /* key 2 */
    *next_vid = info.vid;

    return VTSS_RC_OK;
}

/**
 * Get DHCP Snooping Assigned IP entry
 *
 * To read data of assigned IP in DHCP Snooping.
 *
 * \param mac_addr    [IN]  (key 1) MAC address
 * \param vid         [IN]  (key 2) VLAN ID
 * \param assigned_ip [OUT] The information of the IP assigned to DHCP client by DHCP server
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_assigned_ip_get(
    mesa_mac_t                              mac_addr,
    mesa_vid_t                              vid,
    vtss_appl_dhcp_snooping_assigned_ip_t   *const assigned_ip
)
{
    u8                                  mac[6];
    dhcp_snooping_ip_assigned_info_t    info;

    /* check parameter */
    if ( assigned_ip == NULL ) {
        T_W("assigned_ip == NULL\n");
        return VTSS_RC_ERROR;
    }

    memcpy(mac, &mac_addr, 6);

    if (DHCP_SNOOPING_ip_assigned_info_lookup(mac, vid, &info) == FALSE) {
        return VTSS_RC_ERROR;
    }

    // get ifindex
    if (vtss_ifindex_from_port(info.isid, info.port_no, &(assigned_ip->ifIndex)) != VTSS_RC_OK) {
        T_W("Fail to get first_ifindex from (%u, %u)\n", info.isid, info.port_no);
        return VTSS_RC_ERROR;
    }

    assigned_ip->ipAddr = info.assigned_ip;
    assigned_ip->netmask = info.assigned_mask;
    assigned_ip->dhcpServerIp = info.dhcp_server_ip;

    return VTSS_RC_OK;
}

/**
 * Iterate function of DHCP Snooping Port Statistics table
 *
 * To get first and get next indexes.
 *
 * \param prev_ifindex [IN]  previous ifindex.
 * \param next_ifindex [OUT] next ifindex.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_statistics_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
)
{
    return vtss_appl_dhcp_snooping_port_config_itr(prev_ifindex, next_ifindex);
}

/**
 * Get DHCP Snooping Port Statistics entry
 *
 * To read statistics of the port in DHCP Snooping.
 *
 * \param ifindex         [IN]  (key 1) Interface index - the logical interface
 *                                      index of the physical port.
 * \param port_statistics [OUT] The statistics of the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_statistics_get(
    vtss_ifindex_t                              ifindex,
    vtss_appl_dhcp_snooping_port_statistics_t   *const port_statistics
)
{
    vtss_ifindex_elm_t          ife;
    dhcp_snooping_stats_t       stats;

    /* check parameter */
    if (port_statistics == NULL) {
        T_W("port_statistics == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* get mode of port */
    if (dhcp_snooping_stats_get(ife.isid, ife.ordinal, &stats) != 0) {
        T_W("dhcp_snooping_stats_get()\n");
        return VTSS_RC_ERROR;
    }

    /* pack output */
    port_statistics->rxDiscover         = stats.rx_stats.discover_rx;
    port_statistics->rxOffer            = stats.rx_stats.offer_rx;
    port_statistics->rxRequest          = stats.rx_stats.request_rx;
    port_statistics->rxDecline          = stats.rx_stats.decline_rx;
    port_statistics->rxAck              = stats.rx_stats.ack_rx;
    port_statistics->rxNak              = stats.rx_stats.nak_rx;
    port_statistics->rxRelease          = stats.rx_stats.release_rx;
    port_statistics->rxInform           = stats.rx_stats.inform_rx;
    port_statistics->rxLeaseQuery       = stats.rx_stats.leasequery_rx;
    port_statistics->rxLeaseUnassigned  = stats.rx_stats.leaseunassigned_rx;
    port_statistics->rxLeaseUnknown     = stats.rx_stats.leaseunknown_rx;
    port_statistics->rxLeaseActive      = stats.rx_stats.leaseactive_rx;
    port_statistics->rxDiscardChksumErr = stats.rx_stats.discard_chksum_err_rx;
    port_statistics->rxDiscardUntrust   = stats.rx_stats.discard_untrust_rx;

    port_statistics->txDiscover         = stats.tx_stats.discover_tx;
    port_statistics->txOffer            = stats.tx_stats.offer_tx;
    port_statistics->txRequest          = stats.tx_stats.request_tx;
    port_statistics->txDecline          = stats.tx_stats.decline_tx;
    port_statistics->txAck              = stats.tx_stats.ack_tx;
    port_statistics->txNak              = stats.tx_stats.nak_tx;
    port_statistics->txRelease          = stats.tx_stats.release_tx;
    port_statistics->txInform           = stats.tx_stats.inform_tx;
    port_statistics->txLeaseQuery       = stats.tx_stats.leasequery_tx;
    port_statistics->txLeaseUnassigned  = stats.tx_stats.leaseunassigned_tx;
    port_statistics->txLeaseUnknown     = stats.tx_stats.leaseunknown_tx;
    port_statistics->txLeaseActive      = stats.tx_stats.leaseactive_tx;

    return VTSS_RC_OK;
}

/**
 * Get DHCP Snooping Clear Port Statistics Action entry
 *
 * To get status of clear statistics action of the port in DHCP Snooping.
 *
 * \param ifindex               [IN]  (key 1) Interface index - the logical interface
 *                                            index of the physical port.
 * \param clear_port_statistics [OUT] The status of clear statistics action of the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_clear_port_statistics_get(
    vtss_ifindex_t                                      ifindex,
    vtss_appl_dhcp_snooping_clear_port_statistics_t     *const clear_port_statistics
)
{
    vtss_ifindex_elm_t          ife;

    /* check parameter */
    if (clear_port_statistics == NULL) {
        T_W("clear_port_statistics == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* pack output */
    memset(clear_port_statistics, 0, sizeof(vtss_appl_dhcp_snooping_clear_port_statistics_t));

    return VTSS_RC_OK;
}

/**
 * Set DHCP Snooping Clear Port Statistics Action entry
 *
 * To set to clear statistics on the port in DHCP Snooping.
 *
 * \param ifindex               [IN] (key 1) Interface index - the logical interface
 *                                           index of the physical port.
 * \param clear_port_statistics [IN] Action to clear port statistics
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_clear_port_statistics_set(
    vtss_ifindex_t                                          ifindex,
    const vtss_appl_dhcp_snooping_clear_port_statistics_t   *const clear_port_statistics
)
{
    vtss_ifindex_elm_t          ife;

    /* check parameter */
    if (clear_port_statistics == NULL) {
        T_W("clear_port_statistics == NULL\n");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* do action */
    if (clear_port_statistics->clearPortStatistics) {
        if (dhcp_snooping_stats_clear(ife.isid, ife.ordinal) != 0) {
            T_W("dhcp_snooping_stats_clear(%u, %u)\n", ife.isid, ife.ordinal);
        }
    }

    return VTSS_RC_OK;
}

