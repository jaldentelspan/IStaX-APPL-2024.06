/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "critd_api.h"
#include "port_api.h"
#include "msg_api.h"
#include "mac_api.h"
#include "mac.h"
#include <vtss/appl/mac.h>
#include "packet_api.h"
#include "misc_api.h"
#include "l2proto_api.h"
#ifdef VTSS_SW_OPTION_ICLI
#include "mac_icfg.h"
#endif

#ifdef VTSS_SW_OPTION_ALARM
void mac_any_init();
#endif

/*lint -sem(mac_local_learn_mode_set, thread_protected) Well, not really, but we live with it. */

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Thread variables */
static vtss_handle_t      mac_thread_handle;
static vtss_thread_t      mac_thread_block;
static vtss_flag_t        age_flags, getnext_flags, stats_flags, mac_table_flags;
static void               *mac_frame_rx_filter_id = NULL;
static packet_rx_filter_t mac_frame_rx_filter;

#define MACFLAG_ABORT_AGE                (1 << 0)
#define MACFLAG_START_AGE                (1 << 1)
#define MACFLAG_WAIT_GETNEXT_DONE        (1 << 2)
#define MACFLAG_WAIT_STATS_DONE          (1 << 3)
#define MACFLAG_WAIT_GETNEXTSTACK_DONE   (1 << 4)
#define MACFLAG_WAIT_DONE                (1 << 5)
#define MACFLAG_COULD_NOT_TX             (1 << 8)

/* Global buffer used for exhanging configuration */
static mac_global_t              mac_config;

/* Static-Mac list for non-volatile configured mac-addresses - saved in flash  */
static mac_static_t       *mac_used, *mac_free, mac_list[MAC_ADDR_NON_VOLATILE_MAX];
/* Static-Mac list for volatile configured mac-addresses - not saved in flash */
static mac_static_t       *mac_used_vol, *mac_free_vol, mac_list_vol[MAC_ADDR_VOLATILE_MAX];

/* Temporary and stored age time */
static age_time_temp_t           age_time_temp;

/* Learn mode info - kept in flash  */
static mac_conf_learn_mode_t       mac_learn_mode;

/* Learning-disabled VLANS bitmap */
static u8   lrn_disabled_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

/* Forced learning mode - only kept in memory */
static struct {
    BOOL enable[VTSS_ISID_END][VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD + 1];
    mesa_learn_mode_t learn_mode;
} learn_force;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "mac", "MAC"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define MAC_CRIT_ENTER()     critd_enter(&mac_config.crit,     __FILE__, __LINE__)
#define MAC_CRIT_EXIT()      critd_exit( &mac_config.crit,     __FILE__, __LINE__)
#define MAC_RAM_CRIT_ENTER() critd_enter(&mac_config.ram_crit, __FILE__, __LINE__)
#define MAC_RAM_CRIT_EXIT()  critd_exit( &mac_config.ram_crit, __FILE__, __LINE__)

#define MAC_MGMT_ASSERT(x,_txt) if((x)) { T_E("%s",_txt); return MAC_ERROR_GEN;}

static mesa_rc mac_table_add(vtss_isid_t isid, mac_mgmt_addr_entry_t *entry);
static mesa_rc mac_table_del(vtss_isid_t isid, mesa_vid_mac_t *conf, BOOL vol);

/****************************************************************************/
/*  Chip API functions and various local functions                          */
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
static mesa_rc age_time_set(mac_age_conf_t *conf)
{
    mesa_rc           rc;
    T_D("enter, agetime set:%u", conf->mac_age_time);
    // Set the age time in the chip
    rc = mesa_mac_table_age_time_set(NULL, conf->mac_age_time);
    // Keep the agetime in the API

    MAC_CRIT_ENTER();
    mac_config.conf = *conf;
    MAC_CRIT_EXIT();
    T_D("exit, agetime set:%u", conf->mac_age_time);
    return rc;
}

/****************************************************************************/
// Determine if mac configuration has changed
/****************************************************************************/
static int mac_age_changed(const mac_age_conf_t *const old, const mac_age_conf_t *const new_conf)
{
    return (new_conf->mac_age_time != old->mac_age_time);
}

/****************************************************************************/
/****************************************************************************/
static vtss_isid_t get_local_isid(void)
{
    return VTSS_ISID_START;
}

/****************************************************************************/
// Temp aging thread
/****************************************************************************/
static void mac_thread(vtss_addrword_t data)
{
    age_time_temp_t local_temp;
    mac_age_conf_t age_tmp;

    T_D("Aging thread start");
    while (1) {
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_MAC);

        T_D("Aging thread waiting for start signal");
        vtss_flag_value_t flags = vtss_flag_wait( &age_flags,
                                                  MACFLAG_START_AGE | MACFLAG_ABORT_AGE,
                                                  VTSS_FLAG_WAITMODE_OR_CLR);
        if (flags & MACFLAG_START_AGE) {
            /* Store current age time */
            if (mac_mgmt_age_time_get(&age_tmp) != VTSS_RC_OK) {
                T_E("Could not store age time");
            }
            MAC_CRIT_ENTER();
            /* Take a local copy */
            local_temp = age_time_temp;
            age_time_temp.stored_conf = age_tmp;
            MAC_CRIT_EXIT();
            /* Modify aging time in chip */
            if (age_time_set(&local_temp.temp_conf) != VTSS_RC_OK) {
                T_E("Could not set age time");
            }

            T_D(VPRI64u": Changed age timer to %u sec. Will change it back after " VPRIlu" sec.",
                vtss_current_time(), local_temp.temp_conf.mac_age_time, local_temp.age_period);

            /* Wait until timeout or aborted */
            while (local_temp.age_period > 0) {
                flags = vtss_flag_timed_wait( &age_flags,
                                              MACFLAG_ABORT_AGE,
                                              VTSS_FLAG_WAITMODE_OR,
                                              vtss_current_time() + VTSS_OS_MSEC2TICK(1000));
                if (flags & MACFLAG_ABORT_AGE) {
                    T_I(VPRI64u": Aging aborted", vtss_current_time());
                    break;
                } else {
                    local_temp.age_period--;
                }
            }


            MAC_CRIT_ENTER();
            age_tmp = age_time_temp.stored_conf;
            MAC_CRIT_EXIT();
            /* Back to orginal aging value */
            if (age_time_set(&age_tmp) != VTSS_RC_OK) {
                T_E("Could not set age time");
            }
        }
    }

    /* NOTREACHED */
}

BOOL portlist_index_get(u32 i, const vtss_port_list_stackable_t *pl)
{
    if (i >= 8 * 128) {
        return false;
    }

    u32 idx_bit = i & 7;
    u32 idx = i >> 3;
    return (pl->data[idx] >> idx_bit) & 1;
}

BOOL portlist_index_set(u32 i, vtss_port_list_stackable_t *pl)
{
    if (i >= 8 * 128) {
        return false;
    }

    u8 val = 1;
    u32 idx_bit = i & 7;
    u32 idx = i >> 3;
    val <<= idx_bit;
    pl->data[idx] |= val;
    return true;
}

BOOL portlist_index_clear(u32 i, vtss_port_list_stackable_t *pl)
{
    if (i >= 8 * 128) {
        return false;
    }

    u8 val = 1;
    u32 idx_bit = i & 7;
    u32 idx = i >> 3;
    val <<= idx_bit;
    pl->data[idx] &= (~val);
    return true;
}

u32 isid_port_to_index(vtss_isid_t i,  mesa_port_no_t p)
{
    VTSS_ASSERT(VTSS_PORT_NO_START == 0);
    VTSS_ASSERT(VTSS_ISID_START == 1);
    VTSS_ASSERT(fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1 <= 64);
    return (i - 1) * 64 + iport2uport(p);
}

/****************************************************************************/
/****************************************************************************/
static void pr_entry(const char name[100], mac_mgmt_addr_entry_t *entry)
{
    mesa_port_no_t port_no;
    BOOL first = 1;
    char dest[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD * 4];
    int used = 0, sz = sizeof(dest) - 1;
    dest[sz] = '\0';

    memset(dest, 0, sizeof(dest));
    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        if (entry->destination[port_no]) {
            if (used >= sz) {
                T_E("dest too short");
                break;
            }

            used += snprintf(dest + used, sz - used, "%s%u", first ? "" : "-", port_no);
            first = 0;
        }
    }
    T_D("(%s:)Entry:%02x-%02x-%02x-%02x-%02x-%02x vid:%d Ports:%s", name,
        entry->vid_mac.mac.addr[0], entry->vid_mac.mac.addr[1], entry->vid_mac.mac.addr[2],
        entry->vid_mac.mac.addr[3], entry->vid_mac.mac.addr[4], entry->vid_mac.mac.addr[5], entry->vid_mac.vid, dest);
}

/****************************************************************************/
/****************************************************************************/
static BOOL mac_vid_compare(mesa_vid_mac_t *entry1, mesa_vid_mac_t *entry2)
{
    if (entry1->vid != entry2->vid) {
        return 0;
    }

    return memcmp(entry1->mac.addr, entry2->mac.addr, 6) == 0;
}

/****************************************************************************/
// Larger = 1, smaller = -1, equal = 0
/****************************************************************************/
static int vid_mac_cmp(mesa_vid_mac_t *entry1, mesa_vid_mac_t *entry2)
{
    int i;

    if (entry1->vid > entry2->vid) {
        return 1;
    } else if (entry1->vid < entry2->vid) {
        return -1;
    }

    for (i = 0; i < 6; i++) {
        if (entry1->mac.addr[i] > entry2->mac.addr[i]) {
            return 1;
        } else if (entry1->mac.addr[i] < entry2->mac.addr[i]) {
            return -1;
        }
    }
    return 0;
}

/****************************************************************************/
/****************************************************************************/
static bool vid_mac_bigger(mesa_vid_mac_t *entry1, mesa_vid_mac_t *entry2)
{
    return ((vid_mac_cmp(entry1, entry2) == 1) ? 1 : 0);
}

/****************************************************************************/
/****************************************************************************/
/* A port in a Secure mode (learn frames are killed) will send packet to the CPU even though the SMAC is not learned  */
/* As a workaround we grab those frames and if they are not known they get filtered.                                  */
static BOOL mac_filter_rx_callback(void *contxt,  const u8 *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    mesa_vid_mac_t search_mac;
    mac_mgmt_addr_entry_t return_mac;

    if (!frm || !rx_info) {
        return FALSE;
    }
    /* Get the SMAC and VID of the incoming frame */
    memcpy(search_mac.mac.addr, &frm[6], sizeof(u8) * 6);
    search_mac.vid = rx_info->tag.vid;

    /* ...and look it up in the static MAC table */
    if ((mac_mgmt_static_get_next(VTSS_ISID_LOCAL, &search_mac, &return_mac, FALSE, FALSE) == VTSS_RC_OK) &&
        return_mac.destination[rx_info->port_no]) {
        return FALSE; // Address is known, let the frame pass
    }
    return TRUE; // Not in table, filter the frame
}

/****************************************************************************/
/****************************************************************************/
static mesa_rc mac_register_packet_callback(vtss_isid_t isid, mesa_port_no_t port_no)
{
    BOOL  change = FALSE, enable_filter = FALSE, new_cfg, old_cfg;

    if (!msg_switch_is_local(isid)) {
        // Currently there is no secondary switch support
        return VTSS_RC_OK;
    }
    MAC_CRIT_ENTER();
    new_cfg = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_DISCARD);
    old_cfg = VTSS_BF_GET(mac_frame_rx_filter.src_port_mask, port_no);

    if (new_cfg != old_cfg) {
        VTSS_PORT_BF_SET(mac_frame_rx_filter.src_port_mask, port_no, new_cfg);
        change = TRUE;
    }
    MAC_CRIT_EXIT();
    if (new_cfg) {
        enable_filter = TRUE;
    }

    /* Register, change or unregister callbacks from the packet module */
    if (change) {
        if (enable_filter) {
            if (mac_frame_rx_filter_id) {
                VTSS_RC(packet_rx_filter_change(&mac_frame_rx_filter, &mac_frame_rx_filter_id));
            } else {
                VTSS_RC(packet_rx_filter_register(&mac_frame_rx_filter, &mac_frame_rx_filter_id));
            }
        } else {
            if (mac_frame_rx_filter_id) {
                VTSS_RC(packet_rx_filter_unregister(mac_frame_rx_filter_id));
                mac_frame_rx_filter_id = NULL;
            }
        }
    }
    return VTSS_RC_OK;
}


/****************************************************************************/
// Add MAC address via switch API
/****************************************************************************/
static mesa_rc mac_entry_add(mac_mgmt_addr_entry_t *entry)
{
    mesa_mac_table_entry_t mac_entry;
    mesa_rc                rc;
    mesa_port_no_t         port_no;
    u32                    port_count = port_count_max();

    /* Convert the mac mgmt data type (smaller) to the Switch API data type (larger)*/
    vtss_clear(mac_entry);
    mac_entry.copy_to_cpu = entry->copy_to_cpu;
    mac_entry.cpu_queue = PACKET_XTR_QU_MAC;
    mac_entry.locked = 1;
    mac_entry.vid_mac = entry->vid_mac;
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        mac_entry.destination[port_no] = entry->destination[port_no];
    }

    rc = mesa_mac_table_add(NULL, &mac_entry);

    /*  Add the address to the  local table    */
    if (!msg_switch_is_primary()) {

        (void)(mac_table_add(VTSS_ISID_LOCAL, entry));
    }

    /* Notify the mac memory thread */
    vtss_flag_setbits(&mac_table_flags, 1);
    return rc;
}

/****************************************************************************/
// Delete MAC address via switch API
/****************************************************************************/
static mesa_rc mac_entry_del(mesa_vid_mac_t *vid_mac, BOOL vol)
{
    mesa_rc                rc;
    mesa_vid_mac_t         vid_mac_tmp;
    mesa_mac_table_entry_t entry;

    T_N("Deleting mac entry through API:%s", misc_mac2str(vid_mac->mac.addr));

    if (vid_mac->vid == MAC_ALL_VLANS) {
        /* ignore Vlans */
        memset(&vid_mac_tmp, 0, sizeof(vid_mac_tmp));
        while (1) {
            rc = mesa_mac_table_get_next(NULL, &vid_mac_tmp, &entry);
            if (rc != VTSS_RC_OK) {
                break;
            }

            if (mac_vid_compare(&entry.vid_mac, vid_mac)) {
                rc = mesa_mac_table_del(NULL, &entry.vid_mac);
                if (rc != VTSS_RC_OK) {
                    T_N("(mac_msg_rx)Could not del entry,rc:%d", rc);
                }
            }

            vid_mac_tmp = entry.vid_mac;
        }
    } else {
        rc = mesa_mac_table_del(NULL, vid_mac);
        if (rc != VTSS_RC_OK) {
            T_N("(mac_msg_rx)Could not del entry,rc:%d", rc);
        }
    }


    /*  Delete the address from the local table    */
    if (!msg_switch_is_primary()) {

        if (mac_table_del(VTSS_ISID_LOCAL, vid_mac, vol) != VTSS_RC_OK) {
            T_E("Could not add entry");
        }
    }

    /* Notify the mac memory thread */
    vtss_flag_setbits(&mac_table_flags, 1);

    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/
static void build_mac_list(void)
{
    mac_static_t     *mac;
    int              i;

    /* Build list of free static MACs */
    MAC_CRIT_ENTER();
    mac_free = NULL;
    for (i = 0; i < MAC_ADDR_NON_VOLATILE_MAX; i++) {
        mac = &mac_list[i];
        mac->next = mac_free;
        mac_free = mac;
    }
    mac_used = NULL;

    /* Build list of free volatile MACs */

    mac_free_vol = NULL;
    for (i = 0; i < MAC_ADDR_VOLATILE_MAX; i++) {
        mac = &mac_list_vol[i];
        mac->next = mac_free_vol;
        mac_free_vol = mac;
    }
    mac_used_vol = NULL;
    MAC_CRIT_EXIT();
}

/****************************************************************************/
// Get the next MAC address on the local switch
/****************************************************************************/
static mesa_rc mac_local_table_get_next(mesa_vid_mac_t *vid_mac,
                                        mesa_mac_table_entry_t *entry, BOOL next)
{
    mesa_rc rc;
    mesa_vid_mac_t         vid_mac_tmp;

    if (next) {
        rc = mesa_mac_table_get_next(NULL, vid_mac, entry);
    } else {
        if (vid_mac->vid == MAC_ALL_VLANS) {
            memset(&vid_mac_tmp, 0, sizeof(vid_mac_tmp));

            while (1) {
                rc = mesa_mac_table_get_next(NULL, &vid_mac_tmp, entry);
                if (rc != VTSS_RC_OK) {
                    break;
                }
                if (mac_vid_compare(&entry->vid_mac, vid_mac)) {
                    return rc;
                }
                vid_mac_tmp = entry->vid_mac;
            }


        } else {
            rc = mesa_mac_table_get(NULL, vid_mac, entry);
        }
    }

    entry->copy_to_cpu |= entry->copy_to_cpu_smac;

    T_N("Return: vid: %d, mac: %02x-%02x-%02x-%02x-%02x-%02x Next:%d, rc:%d\n",
        entry->vid_mac.vid,
        entry->vid_mac.mac.addr[0], entry->vid_mac.mac.addr[1], entry->vid_mac.mac.addr[2],
        entry->vid_mac.mac.addr[3], entry->vid_mac.mac.addr[4], entry->vid_mac.mac.addr[5], next, rc);
    return rc;
}

/****************************************************************************/
/****************************************************************************/
static mesa_rc mac_local_learn_mode_set(mesa_port_no_t port_no, const mesa_learn_mode_t *const learn_mode)
{
    T_D("Local. Port:%d auto:%d, cpu:%d, discard:%d", port_no, learn_mode->automatic, learn_mode->cpu, learn_mode->discard);
    return mesa_learn_port_mode_set(NULL, port_no,  learn_mode);
}

/****************************************************************************/
// Get MAC address statistics
/****************************************************************************/
static mesa_rc mac_local_table_get_stats(mac_table_stats_t *stats)
{
    int                    i;
    mesa_vid_mac_t         vid_mac;
    mesa_mac_table_entry_t mac_entry;
    mesa_port_no_t         port_no;
    mesa_rc                rc;
    BOOL                   next = 1;
    u32                    port_count = port_count_max();

    T_N("enter mac_local_table_get_stats");

    // initial search address is 00-00-00-00-00-00, VID: 1
    vid_mac.vid = 1;
    for (i = 0; i < 6; i++) {
        vid_mac.mac.addr[i] = 0;
    }

    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        stats->learned[port_no] = 0;
    }
    stats->static_total = 0;
    stats->learned_total = 0;;

    // Run through all existing entries and gather stats
    for (;;) {
        rc = mac_local_table_get_next(&vid_mac, &mac_entry, next);
        if (rc != VTSS_RC_OK) {
            break;
        }

        // Search again from the found entry
        vid_mac = mac_entry.vid_mac;

        if (mac_entry.locked) {
            stats->static_total++;
        } else {
            stats->learned_total++;
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (mac_entry.destination[port_no]) {
                    stats->learned[port_no]++;
                    /* Count only for the 1.first port (to avoid counting aggregation destinations) */
                    break;
                }
            }
        }

        if (stats->static_total + stats->learned_total > fast_cap(MESA_CAP_L2_MAC_ADDR_CNT)) {
            break;
        }

    }

    T_N("exit mac_local_table_get_stats");
    return VTSS_RC_OK;
}

/****************************************************************************/
// Get MAC address VLAN statistics
/****************************************************************************/
static mesa_rc mac_local_table_get_vlan_stats(mesa_vid_t vlan, mac_table_stats_t *stats)
{

    int                    i;
    mesa_vid_mac_t         vid_mac;
    mesa_mac_table_entry_t mac_entry;
    mesa_port_no_t         port_no;
    mesa_rc                rc;
    BOOL                   next = 1;
    u32                    port_count = port_count_max();

    T_N("enter mac_local_table_get_vlan_stats");

    // initial search address is 00-00-00-00-00-00, VID: 1
    vid_mac.vid = vlan;
    for (i = 0; i < 6; i++) {
        vid_mac.mac.addr[i] = 0;
    }

    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        stats->learned[port_no] = 0;
    }
    stats->static_total = 0;
    stats->learned_total = 0;;

    // Run through all existing entries and gather stats
    for (;;) {
        rc = mac_local_table_get_next(&vid_mac, &mac_entry, next);
        if (rc != VTSS_RC_OK || (mac_entry.vid_mac.vid != vlan)) {
            break;
        }

        if (mac_entry.locked) {
            stats->static_total++;
        } else {
            stats->learned_total++;
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (mac_entry.destination[port_no]) {
                    stats->learned[port_no]++;
                    /* Count only for the 1.first port (to avoid counting aggregation destinations) */
                    break;
                }
            }
        }

        if (stats->static_total + stats->learned_total > fast_cap(MESA_CAP_L2_MAC_ADDR_CNT)) {
            break;
        }

        // Search again from the found entry
        vid_mac = mac_entry.vid_mac;
    }

    T_N("exit mac_local_table_get_vlan_stats");
    return VTSS_RC_OK;
}

/****************************************************************************/
// Flush the RAM address table
/****************************************************************************/
static mesa_rc mac_local_del_locked(void)
{

    mac_static_t           *mac;

    if (msg_switch_is_primary()) {
        return VTSS_RC_OK;
    }

    T_N("enter mac_local_table_del_locked");

    MAC_CRIT_ENTER();
    /*  Volatile      */
    for (mac = mac_used_vol; mac != NULL; mac = mac->next) {
        T_N("Deleting mac's:%02x-%02x-%02x-%02x-%02x-%02x vid:%d",
            mac->conf.vid_mac.mac.addr[0], mac->conf.vid_mac.mac.addr[1], mac->conf.vid_mac.mac.addr[2],
            mac->conf.vid_mac.mac.addr[3], mac->conf.vid_mac.mac.addr[4], mac->conf.vid_mac.mac.addr[5], mac->conf.vid_mac.vid);

        (void)mesa_mac_table_del(NULL, &mac->conf.vid_mac);
    }

    /*  .. and non Volatile      */
    for (mac = mac_used; mac != NULL; mac = mac->next) {
        T_N("Deleting mac's:%02x-%02x-%02x-%02x-%02x-%02x vid:%d",
            mac->conf.vid_mac.mac.addr[0], mac->conf.vid_mac.mac.addr[1], mac->conf.vid_mac.mac.addr[2],
            mac->conf.vid_mac.mac.addr[3], mac->conf.vid_mac.mac.addr[4], mac->conf.vid_mac.mac.addr[5], mac->conf.vid_mac.vid);

        (void)mesa_mac_table_del(NULL, &mac->conf.vid_mac);
    }
    MAC_CRIT_EXIT();

    /* Reset the linked MAC list */
    build_mac_list();

    T_N("exit mac_local_table_del_locked");
    return VTSS_RC_OK;
}

/****************************************************************************/
// Flush all static addresses we know of from chip
/****************************************************************************/
static void flush_mac_list(BOOL force)
{
    mac_static_t           *mac;
    /* Flush the primary switch's MAC table   */
    if (msg_switch_is_primary() || force) {

        /*  Volatile      */
        for (mac = mac_used_vol; mac != NULL; mac = mac->next) {
            (void)mac_entry_del(&mac->conf.vid_mac, 1);
        }

        /*  .. and non Volatile      */
        for (mac = mac_used; mac != NULL; mac = mac->next) {
            (void)mac_entry_del(&mac->conf.vid_mac, 0);
        }
    }
}

/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
static const char *mac_msg_id_txt(mac_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case MAC_MSG_ID_AGE_SET_REQ:
        txt = "MAC_MSG_ID_AGE_SET_REQ";
        break;
    case MAC_MSG_ID_GET_NEXT_REQ:
        txt = "MAC_MSG_ID_GET_NEXT_REQ";
        break;
    case MAC_MSG_ID_GET_NEXT_REP:
        txt = "MAC_MSG_ID_GET_NEXT_REP";
        break;
    case MAC_MSG_ID_GET_NEXT_STACK_REQ:
        txt = "MAC_MSG_ID_GET_NEXT_STACK_REQ";
        break;
    case MAC_MSG_ID_GET_STATS_REQ:
        txt = "MAC_MSG_ID_GET_STATS_REQ";
        break;
    case MAC_MSG_ID_GET_STATS_REP:
        txt = "MAC_MSG_ID_GET_STATS_REP";
        break;
    case MAC_MSG_ID_ADD_REQ:
        txt = "MAC_MSG_ID_ADD_REQ";
        break;
    case MAC_MSG_ID_DEL_REQ:
        txt = "MAC_MSG_ID_DEL_REQ";
        break;
    case MAC_MSG_ID_LEARN_REQ:
        txt = "MAC_MSG_ID_LEARN_REQ";
        break;
    case MAC_MSG_ID_FLUSH_REQ:
        txt = "MAC_MSG_ID_FLUSH_REQ";
        break;
    case MAC_MSG_ID_LOCKED_DEL_REQ:
        txt = "MAC_MSG_ID_LOCKED_DEL_REQ";
        break;
    case MAC_MSG_ID_UPSID_FLUSH_REQ:
        txt = "MAC_MSG_ID_UPSID_FLUSH_REQ";
        break;
    case MAC_MSG_ID_UPSID_UPSPN_FLUSH_REQ:
        txt = "MAC_MSG_ID_UPSID_UPSPN_FLUSH_REQ";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}

/****************************************************************************/
// Allocate request/reply buffer
/****************************************************************************/
static mac_msg_req_t *mac_msg_req_alloc(mac_msg_id_t msg_id, u32 ref_cnt)
{
    mac_msg_req_t *msg;

    if (ref_cnt == 0) {
        return NULL;
    }

    msg = (mac_msg_req_t *)msg_buf_pool_get(mac_config.request);
    VTSS_ASSERT(msg);
    if (ref_cnt > 1) {
        msg_buf_pool_ref_cnt_set(msg, ref_cnt);
    }

    msg->msg_id = msg_id;
    return msg;
}

/****************************************************************************/
/****************************************************************************/
static mac_msg_rep_t *mac_msg_rep_alloc(mac_msg_id_t msg_id)
{
    mac_msg_rep_t *msg = (mac_msg_rep_t *)msg_buf_pool_get(mac_config.reply);
    VTSS_ASSERT(msg);
    msg->msg_id = msg_id;
    return msg;
}

/****************************************************************************/
// Release mac message buffer
/****************************************************************************/
static void mac_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    mac_msg_id_t msg_id = *(mac_msg_id_t *)msg;
    (void)msg_buf_pool_put(msg);

    if (rc != MSG_TX_RC_OK) {
        T_D("Could not transmit %s. Got return code %d from msg module", mac_msg_id_txt(msg_id), rc);

        /* Release the sync flag */
        if (msg_id == MAC_MSG_ID_GET_NEXT_REQ ) {
            vtss_flag_setbits(&getnext_flags, MACFLAG_COULD_NOT_TX);
        } else if (msg_id == MAC_MSG_ID_LEARN_REQ) {
            vtss_flag_setbits(&stats_flags, MACFLAG_COULD_NOT_TX);
        }
    }
}

/****************************************************************************/
// Send mac message
/****************************************************************************/
static void mac_msg_tx(void *msg, vtss_isid_t isid, size_t len)
{
    mac_msg_id_t msg_id = *(mac_msg_id_t *)msg;

    T_N("Tx: %s, len: %zd, isid: %d", mac_msg_id_txt(msg_id), len, isid);
    // Avoid "Warning -- Constant value Boolean" Lint warning, due to the use of MSG_TX_DATA_HDR_LEN_MAX() below
    /*lint -e(506) */
    msg_tx_adv(NULL, mac_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_MAC, isid, msg, len + MSG_TX_DATA_HDR_LEN_MAX(mac_msg_req_t, req, mac_msg_rep_t, rep));
}

/****************************************************************************/
/****************************************************************************/
static mesa_rc mac_do_set_local_learn_mode(mesa_port_no_t port_no, const mesa_learn_mode_t *const learn_mode)
{
    mesa_rc rc = VTSS_RC_OK;
    u32     port_count = port_count_max();

    if (port_no == MAC_ALL_PORTS) {
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            rc = mac_local_learn_mode_set(port_no, learn_mode);
        }
    } else {
        rc = mac_local_learn_mode_set(port_no, learn_mode);
    }

    return rc;
}

/****************************************************************************/
/****************************************************************************/
static mesa_rc mac_do_set_local_vlan_learn_mode(mesa_vid_t vid, const vtss_appl_mac_vid_learn_mode_t *const learn_mode)
{
    mesa_vlan_vid_conf_t conf;
    u32                  min, max;
    mesa_rc              rc = VTSS_RC_OK, rc2;

    if (vid == VTSS_VID_ALL) {
        min = 1;
        max = 4095;
    } else {
        min = vid;
        max = vid;
    }

    for (vid = min; vid <= max; vid++) {
        {
            // Make sure that the calls to get() and set() are undivided
            VTSS_APPL_API_LOCK_SCOPE();
            if ((rc2 = mesa_vlan_vid_conf_get(NULL, vid, &conf)) == VTSS_RC_OK) {
                conf.learning = learn_mode->learning;
                rc2 = mesa_vlan_vid_conf_set(NULL, vid, &conf);
            }
        }

        if (rc2 != VTSS_RC_OK) {
            rc = rc2;
        } else {
            MAC_CRIT_ENTER();
            VTSS_BF_SET(lrn_disabled_vids, vid, !learn_mode->learning);
            MAC_CRIT_EXIT();

        }
    }
    return rc;
}

/****************************************************************************/
/****************************************************************************/
static BOOL mac_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    mac_msg_id_t   msg_id  = *(mac_msg_id_t *)rx_msg;

    T_N("Rx: %s, len: %zd, isid: %u", mac_msg_id_txt(msg_id), len, isid);

    switch (msg_id) {
    case MAC_MSG_ID_AGE_SET_REQ: {
        mac_msg_req_t *req_msg = (mac_msg_req_t *)rx_msg;

        T_D("Got message: change aging to %u", req_msg->req.age_set.conf.mac_age_time);
        if (age_time_set(&req_msg->req.age_set.conf) != VTSS_RC_OK) {
            T_E("Could not set time");
        }
        break;
    }

    case MAC_MSG_ID_GET_NEXT_REQ: {
        mac_msg_req_t *req_msg = (mac_msg_req_t *)rx_msg;
        mac_msg_rep_t *rep_msg = mac_msg_rep_alloc(MAC_MSG_ID_GET_NEXT_REP);

        /* Do a local Get Next */
        rep_msg->rep.get_next.rc = mac_local_table_get_next(&req_msg->req.get_next.vid_mac, &rep_msg->rep.get_next.entry, req_msg->req.get_next.next);

        T_N("Return mac entry to primary switch: %02x-%02x-%02x-%02x-%02x-%02x vid: %d",
            rep_msg->rep.get_next.entry.vid_mac.mac.addr[0], rep_msg->rep.get_next.entry.vid_mac.mac.addr[1], rep_msg->rep.get_next.entry.vid_mac.mac.addr[2],
            rep_msg->rep.get_next.entry.vid_mac.mac.addr[3], rep_msg->rep.get_next.entry.vid_mac.mac.addr[4], rep_msg->rep.get_next.entry.vid_mac.mac.addr[5],
            rep_msg->rep.get_next.entry.vid_mac.vid);

        /* Transmit the get_next reply */
        mac_msg_tx(rep_msg, isid, sizeof(rep_msg->rep.get_next));
        break;
    }

    case MAC_MSG_ID_GET_NEXT_REP: {
        mac_msg_rep_t *rep_msg = (mac_msg_rep_t *)rx_msg;

        MAC_CRIT_ENTER();
        /* Store the info in the global struct */
        mac_config.get_next.entry = rep_msg->rep.get_next.entry;
        mac_config.get_next.rc    = rep_msg->rep.get_next.rc;
        MAC_CRIT_EXIT();

        /* Release the sync flag */
        vtss_flag_setbits(&getnext_flags, MACFLAG_WAIT_GETNEXT_DONE);
        break;
    }

    case MAC_MSG_ID_GET_STATS_REQ: {
        mac_msg_req_t *req_msg = (mac_msg_req_t *)rx_msg;
        mac_msg_rep_t *rep_msg = mac_msg_rep_alloc(MAC_MSG_ID_GET_STATS_REP);

        /* Do a local Get Stats */
        if (req_msg->req.stats.vlan > 0) {
            rep_msg->rep.stats.rc = mac_local_table_get_vlan_stats(req_msg->req.stats.vlan, &rep_msg->rep.stats.stats);
        } else {
            rep_msg->rep.stats.rc = mac_local_table_get_stats(&rep_msg->rep.stats.stats);
        }

        /* Transmit the get_next reply */
        mac_msg_tx(rep_msg, isid, sizeof(rep_msg->rep.stats));
        break;
    }

    case MAC_MSG_ID_GET_STATS_REP: {
        mac_msg_rep_t *rep_msg = (mac_msg_rep_t *)rx_msg;

        /* Store the info in the global struct */
        MAC_CRIT_ENTER();
        mac_config.get_stats.stats = rep_msg->rep.stats.stats;
        mac_config.get_stats.rc    = rep_msg->rep.stats.rc;
        MAC_CRIT_EXIT();

        /* Release the sync flag */
        vtss_flag_setbits(&stats_flags, MACFLAG_WAIT_STATS_DONE);
        break;
    }

    case MAC_MSG_ID_ADD_REQ: {
        mac_msg_req_t *req_msg = (mac_msg_req_t *)rx_msg;
        mesa_rc       rc;

        /* Do a local add mac */
        if ((rc = mac_entry_add(&req_msg->req.add.entry)) != VTSS_RC_OK) {
            T_W("(mac_msg_rx)Could not add entry, rc = %d", rc);
        }
        break;
    }

    case MAC_MSG_ID_DEL_REQ: {
        mac_msg_req_t *req_msg = (mac_msg_req_t *)rx_msg;
        mesa_rc       rc;

        /* Do a local del mac */
        if ((rc = mac_entry_del(&req_msg->req.del.vid_mac, req_msg->req.del.vol)) != VTSS_RC_OK) {
            T_W("(mac_msg_rx)Could not del entry, rc = %d", rc);
        }
        break;
    }

    case MAC_MSG_ID_LEARN_REQ: {
        mac_msg_req_t *req_msg = (mac_msg_req_t *)rx_msg;
        mesa_rc       rc;

        /* Do a local learnmode */
        if ((rc = mac_do_set_local_learn_mode(req_msg->req.learn.port_no, &req_msg->req.learn.learn_mode)) != VTSS_RC_OK) {
            T_W("(mac_msg_rx)Could set learn mode, rc = %d", rc);
        }

        break;
    }

    case MAC_MSG_ID_VLAN_LEARN_REQ: {
        mac_msg_req_t *req_msg = (mac_msg_req_t *)rx_msg;
        mesa_rc       rc;

        /* Do a local learnmode */
        if ((rc = mac_do_set_local_vlan_learn_mode(req_msg->req.vlan_learn.vid, &req_msg->req.vlan_learn.learn_mode)) != VTSS_RC_OK) {
            T_W("(mac_msg_rx)Could set learn mode, rc = %d", rc);
        }

        break;
    }

    case MAC_MSG_ID_FLUSH_REQ: {
        /* Do a local flush mac */
        if (mesa_mac_table_flush(NULL) != VTSS_RC_OK) {
            T_W("Could not flush");
        }
        break;
    }

    case MAC_MSG_ID_LOCKED_DEL_REQ: {
        T_D("Got message: Remove all static addresses");
        if (mac_local_del_locked() != VTSS_RC_OK) {
            T_E("Could not delete addresses");
        }
        break;
    }

    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }
    return TRUE;
}

/****************************************************************************/
/****************************************************************************/
static mesa_rc mac_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = mac_msg_rx;
    filter.modid = VTSS_MODULE_ID_MAC;
    return msg_rx_filter_register(&filter);
}

/****************************************************************************/
// Remove any old static mac addresses
/****************************************************************************/
static mesa_rc mac_remove_locked(vtss_isid_t isid)
{
    mac_msg_req_t *msg;

    T_D("Enter mac_remove_locked isid:%d", isid);
    if (msg_switch_exists(isid)) {
        msg = mac_msg_req_alloc(MAC_MSG_ID_LOCKED_DEL_REQ, 1);
        mac_msg_tx(msg, isid, 0);
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
// Set mac age configuration
/****************************************************************************/
static mesa_rc mac_age_stack_conf_set(vtss_isid_t isid)
{
    mac_msg_req_t *msg;

    if (msg_switch_exists(isid)) {
        /* Set MAC aging  */
        T_D("isid: %d. set age_time to %u", isid, mac_config.conf.mac_age_time);

        msg = mac_msg_req_alloc(MAC_MSG_ID_AGE_SET_REQ, 1);

        /* Use the stored value in case of the sprout is doing a temp aging */
        MAC_CRIT_ENTER();
        msg->req.age_set.conf = age_time_temp.stored_conf;
        MAC_CRIT_EXIT();

        mac_msg_tx(msg, isid, sizeof(msg->req.age_set));
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/
static BOOL mac_mgmt_port_sid_invalid(vtss_isid_t isid, mesa_port_no_t port_no, BOOL set, BOOL check_port)
{
    if (isid != VTSS_ISID_LOCAL && !msg_switch_is_primary()) {
        T_D("not primary switch");
        return 1;
    }

    if (port_no >= port_count_max()) {
        T_D("illegal port_no: %u", port_no);
        return 1;
    }

    if (isid >= VTSS_ISID_END) {
        T_D("illegal isid: %d", isid);
        return 1;
    }

    if (set && isid == VTSS_ISID_LOCAL) {
        T_D("SET not allowed, isid: %d", isid);
        return 1;
    }

    return 0;
}

/****************************************************************************/
// Set the learn mode to a switch in the stack
/****************************************************************************/
static mesa_rc mac_stack_learn_mode_set(vtss_isid_t isid, mesa_port_no_t port_no, const mesa_learn_mode_t *const learn_mode)
{
    mac_msg_req_t *msg;
    mesa_rc       rc = VTSS_RC_OK;

    if (isid == VTSS_ISID_LOCAL) {
        isid = get_local_isid();
        MAC_MGMT_ASSERT(!VTSS_ISID_LEGAL(isid), "Failed getting local ISID id");
    }

    if (msg_switch_exists(isid)) {
        if (msg_switch_is_local(isid)) {
            // Bypass the message module for fast setting of learn mode (needed by DOT1X module).
            rc = mac_do_set_local_learn_mode(port_no, learn_mode);
        } else {
            msg = mac_msg_req_alloc(MAC_MSG_ID_LEARN_REQ, 1);
            msg->req.learn.port_no = port_no;
            msg->req.learn.learn_mode = *learn_mode;
            mac_msg_tx(msg, isid, sizeof(msg->req.learn));
        }
    }
    return rc;
}

/****************************************************************************/
/* Add a mac entry to the local switch */
/****************************************************************************/
static mesa_rc mac_add_stack(vtss_isid_t isid_add, mac_mgmt_addr_entry_t *entry)
{
    return mac_entry_add(entry);
}

/****************************************************************************/
// Delete a mac entry from a switch (isid_del) in the stack
/****************************************************************************/
static mesa_rc mac_del_stack(vtss_isid_t isid_del, mesa_vid_mac_t *entry, BOOL vol)
{
    BOOL                  mac_exists_in_stack = 0;
    int                   port_no;
    mac_mgmt_addr_entry_t del_entry, mac_entry;
    switch_iter_t         sit;

    /* Run through the switch list and check if the address
       is known on other frontports in the stack*/

    if (mac_mgmt_port_sid_invalid(isid_del, VTSS_PORT_NO_START, 1, FALSE)) {
        return MAC_ERROR_GEN;
    }

    vtss_clear(del_entry);
    vtss_clear(mac_entry);

    // Skip this part for volatile entries.
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit) && !vol) {
        if (sit.isid != isid_del) {
            if (mac_mgmt_static_get_next(sit.isid, entry, &mac_entry, 0, vol) == VTSS_RC_OK) {
                mac_exists_in_stack = 1;
                break;
            }
        }
    }

    if (mac_exists_in_stack) {
        mac_msg_req_t *msg;

        if (!msg_switch_exists(isid_del)) {
            return VTSS_RC_OK;
        }

        T_D("Only deleting from isid %d\n", isid_del);

        /* Mac also exists elsewhere in the stack. Only delete from isid's frontports */
        del_entry.vid_mac = *entry;
        for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            del_entry.destination[port_no] = 0;
        }
        msg = mac_msg_req_alloc(MAC_MSG_ID_ADD_REQ, 1); /* Using Add to modify portmask */
        msg->req.add.entry = del_entry;
        mac_msg_tx(msg, isid_del, sizeof(msg->req.add));
    } else {
        mac_msg_req_t *msg;

        /* Mac only exists at ISIDs frontports. Delete from all switches */
        T_D("Mac only exists at isid:%d frontports. Deleting from whole stack\n", isid_del);

        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);

        /* Allocate a message with a ref-count corresponding to the number of times switch_iter_getnext() will return TRUE. */
        if ((msg = mac_msg_req_alloc(MAC_MSG_ID_DEL_REQ, sit.remaining)) != NULL) {
            msg->req.del.vid_mac = *entry;
            msg->req.del.vol = vol;

            while (switch_iter_getnext(&sit)) {
                mac_msg_tx(msg, sit.isid, sizeof(msg->req.del));
            }
        }
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
// Set the learn mode to the new switch in the stack
/****************************************************************************/
static void mac_learnmode_conf_set(vtss_isid_t isid)
{
    mesa_learn_mode_t mode;
    mesa_learn_mode_t mode_cmp;
    mesa_port_no_t    port_no;
    u32               port_count = port_count_max();
    BOOL              lf;

    /* Need to convert to the larger 'mesa_learn_mode_t' to chip api struct */
    MAC_CRIT_ENTER();
    mode.automatic = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][1], LEARN_AUTOMATIC);
    mode.cpu = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][1], LEARN_CPU);
    mode.discard = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][1], LEARN_DISCARD);
    MAC_CRIT_EXIT();

    /* Most of the time all ports have the same learning mode */
    /* To save time, all ports are set to port 1's mode and then only  ports that differs from this mode are changed */
    if (mac_stack_learn_mode_set(isid, MAC_ALL_PORTS, &mode) != VTSS_RC_OK) {
        T_W("Could not set stack learn mode\n");
    }

    /* Check if any port have a different value than already configured with  */
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (mac_register_packet_callback(isid, port_no) != VTSS_RC_OK) {
            T_W("mac_register_packet_callback failed\n");
        }
        MAC_CRIT_ENTER();
        lf = learn_force.enable[isid][port_no];
        MAC_CRIT_EXIT();
        if (lf) {
            mode_cmp.automatic = 0;
            mode_cmp.cpu = 1;
            mode_cmp.discard = 1;
            if ((mac_stack_learn_mode_set(isid, port_no, &mode_cmp)) != VTSS_RC_OK) {
                T_W("Could not set stack learn mode\n");
            }
        } else {
            MAC_CRIT_ENTER();
            mode_cmp.automatic = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_AUTOMATIC);
            mode_cmp.cpu = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_CPU);
            mode_cmp.discard = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_DISCARD);
            MAC_CRIT_EXIT();
            if (memcmp(&mode, &mode_cmp, sizeof(mode)) != 0) {
                /* Configure this port */
                if ((mac_stack_learn_mode_set(isid, port_no, &mode_cmp)) != VTSS_RC_OK) {
                    T_W("Could not set stack learn mode\n");
                }
            }
        }
    }
}

/****************************************************************************/
// Add a static mac address to a switch in the stack
/****************************************************************************/
static void mac_static_conf_set(vtss_isid_t isid_add)
{
    mac_mgmt_addr_entry_t mac_entry;
    mesa_vid_mac_t        vid_mac;
    int                   vol;
    switch_iter_t         sit;

    T_I("Enter %d", isid_add);
    /* Add static address to the new unit, both volatile and non-volatile */
    /* All addresses must be refreshed as all addresses must be known in every unit.*/
    for (vol = 0; vol <= 1; vol++) {
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            memset(&vid_mac, 0, sizeof(vid_mac));
            while (mac_mgmt_static_get_next(sit.isid, &vid_mac, &mac_entry, 1, vol) == VTSS_RC_OK) {
                vid_mac = mac_entry.vid_mac;
                if (mac_add_stack(sit.isid, &mac_entry) != VTSS_RC_OK) {
                    T_W("Could not add mac-address to ISID:%u", sit.isid);
                }
            }
        }
    }

    T_I("Exit %d", isid_add);
}

/****************************************************************************/
/****************************************************************************/
static mesa_rc mac_table_add(vtss_isid_t isid, mac_mgmt_addr_entry_t *entry)
{
    mac_static_t           *mac, *prev, *new_mac = NULL, **mac_used_p, **mac_free_p;
    mac_entry_conf_t       conf;
    mac_mgmt_addr_entry_t  return_mac;
    mesa_mac_table_entry_t found_mac;
    uint                   i = 0;
    BOOL                   place_found = 0, newentry = 0, vol = entry->volatil, own_by_mac_module = 0;
    u32                    port_count = port_count_max();
    switch_iter_t          sit;

    T_N("Doing a table add at switch isid:%d", isid);

    if (isid == VTSS_ISID_LOCAL) {
        isid = get_local_isid();
        MAC_MGMT_ASSERT(!VTSS_ISID_LEGAL(isid), "Failed getting local ISID id");
    }

    if (entry->vid_mac.vid < MAC_MGMT_VLAN_ID_MIN || entry->vid_mac.vid > MAC_MGMT_VLAN_ID_MAX) {
        T_E("Invalid VID (%u)", entry->vid_mac.vid);
        return MAC_ERROR_VLAN_INVALID;
    }

    /* If the address exist then exit with an proper error code */
    if (vol) {
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            if ( mac_mgmt_static_get_next(sit.isid, &entry->vid_mac, &return_mac, 0, MAC_NON_VOLATILE) == VTSS_RC_OK) {  // non-volatile
                return MAC_ERROR_MAC_EXIST;
            }
        }
    }

    if (!vol) {
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            if ( mac_mgmt_static_get_next(sit.isid, &entry->vid_mac, &return_mac, 0, MAC_VOLATILE) == VTSS_RC_OK) {  // volatile
                return MAC_ERROR_MAC_VOL_EXIST;
            }
        }
    }

    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        if ( mac_mgmt_static_get_next(sit.isid, &entry->vid_mac, &return_mac, 0, MAC_NON_VOLATILE) == VTSS_RC_OK) {
            own_by_mac_module = 1;
            break;
        }
    }

    /* Adresses added by other modules may not be changed, unless it's a volatile
     * entry, so that we know it's for debug purposes and not end-user controlled. */
    if (!own_by_mac_module && !vol) {
        if (mac_mgmt_table_get_next(isid, &entry->vid_mac, &found_mac, 0) == VTSS_RC_OK) {
            if (found_mac.locked) {
                return MAC_ERROR_MAC_SYSTEM_EXIST;
            }
        }
    }

    MAC_CRIT_ENTER();
    if (vol) {
        mac_used_p = &mac_used_vol;
        mac_free_p = &mac_free_vol;
    } else {
        mac_used_p = &mac_used;
        mac_free_p = &mac_free;
    }

    for (mac = *mac_used_p; mac != NULL; mac = mac->next) {
        T_N("In list before add:%02x-%02x-%02x-%02x-%02x-%02x vid:%d",
            mac->conf.vid_mac.mac.addr[0], mac->conf.vid_mac.mac.addr[1], mac->conf.vid_mac.mac.addr[2],
            mac->conf.vid_mac.mac.addr[3], mac->conf.vid_mac.mac.addr[4], mac->conf.vid_mac.mac.addr[5], mac->conf.vid_mac.vid);
    }

    /*  Search for existing entry and if found replace with new entry data */
    for (mac = *mac_used_p; mac != NULL; mac = mac->next) {
        if (mac_vid_compare(&mac->conf.vid_mac, &entry->vid_mac) && mac->conf.isid == isid) {
            new_mac = mac;
            break;
        }
    }

    /* Convert datatype to the smaller Conf API */
    memset(&conf, 0, sizeof(conf));
    conf.vid_mac = entry->vid_mac;
    conf.isid = isid;
    conf.copy_to_cpu = entry->copy_to_cpu;

    if (entry->dynamic) {
        conf.mode = MAC_ADDR_DYNAMIC;
    }

    for (i = 0; i < port_count; i++) {
        VTSS_BF_SET(conf.destination, i, entry->destination[i + VTSS_PORT_NO_START]);
    }

    if (new_mac == NULL && *mac_free_p == NULL) {
        T_W("Static MAC Table is full\n");
        MAC_CRIT_EXIT();
        return MAC_ERROR_REG_TABLE_FULL;
    } else {
        /* Add the new entry to the used list */
        if (new_mac == NULL) {
            new_mac = *mac_free_p;
            *mac_free_p = new_mac->next;
            new_mac->next = *mac_used_p;
            *mac_used_p = new_mac;
            newentry = 1;
            T_N("Added new entry to local MAC Table");
        }
        new_mac->conf = conf;

        if (newentry) {
            /* Find the right placement for the address in the pointerlist, lowest to highest */
            for (mac = *mac_used_p, prev = NULL; mac != NULL; prev = mac, mac = mac->next) {
                if (vid_mac_bigger(&mac->conf.vid_mac, &new_mac->conf.vid_mac )) {
                    place_found = 1;
                    break;
                }
            }
            if (place_found && (prev != new_mac) && (prev != NULL)) {
                T_N("Not first and not last in the pointer list.");
                *mac_used_p = new_mac->next;
                new_mac->next = prev->next;
                prev->next = new_mac;
            } else if (!place_found && (*mac_used_p)->next != NULL && (prev != NULL)) {
                T_N("Using last place");
                *mac_used_p = new_mac->next;
                prev->next = new_mac;
                new_mac->next = NULL;
            } else {
                T_N("Using the default 1.place");
            }
        }
    }

    for (mac = *mac_used_p; mac != NULL; mac = mac->next) {
        T_N("In list after add:%02x-%02x-%02x-%02x-%02x-%02x vid:%d",
            mac->conf.vid_mac.mac.addr[0], mac->conf.vid_mac.mac.addr[1], mac->conf.vid_mac.mac.addr[2],
            mac->conf.vid_mac.mac.addr[3], mac->conf.vid_mac.mac.addr[4], mac->conf.vid_mac.mac.addr[5], mac->conf.vid_mac.vid);
    }
    MAC_CRIT_EXIT();

    /* If adding to the local secondary switch table the return now  */
    if (msg_switch_is_primary()) {
        if (msg_switch_configurable(isid)) {
            pr_entry("mac_table_add", entry);
            if (mac_add_stack(isid, entry) != VTSS_RC_OK) {
                T_W("Could not add mac-address to ISID:%u", isid);
            }
            T_N("Added new entry to chip (ISID:%d)", isid);
        }
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
// Remove volatile or non-volatile static mac address from chip, flash and local configuration
/****************************************************************************/
static mesa_rc mac_table_del(vtss_isid_t isid, mesa_vid_mac_t *conf, BOOL vol)
{

    mac_static_t       *mac, *prev, **mac_used_p, **mac_free_p;
    mac_mgmt_addr_entry_t return_mac;
    BOOL  entry_found = 0;

    if (conf->vid < MAC_MGMT_VLAN_ID_MIN || conf->vid > MAC_MGMT_VLAN_ID_MAX) {
        T_E("Invalid VID (%u)", conf->vid);
        return MAC_ERROR_VLAN_INVALID;
    }

    if (isid == VTSS_ISID_LOCAL) {
        isid = get_local_isid();
        MAC_MGMT_ASSERT(!VTSS_ISID_LEGAL(isid), "Failed getting local ISID id");
    }

    MAC_CRIT_ENTER();
    if (vol) {
        mac_used_p = &mac_used_vol;
        mac_free_p = &mac_free_vol;
    } else {
        mac_used_p = &mac_used;
        mac_free_p = &mac_free;
    }
    MAC_CRIT_EXIT();

    if (msg_switch_is_primary()) {
        /* Remove the entry from the chip through the API */
        if ( mac_mgmt_static_get_next(isid, conf, &return_mac, 0, vol) == VTSS_RC_OK)
            if (mac_del_stack(isid, conf, vol) != VTSS_RC_OK) {
                T_W("Could not del address");
            }
    }

    MAC_CRIT_ENTER();
    /* Find and remove the entry from the local list */
    for (mac = *mac_used_p, prev = NULL; mac != NULL; prev = mac, mac = mac->next) {

        if (mac_vid_compare(&mac->conf.vid_mac, conf) && mac->conf.isid == isid) {
            /* Found entry */
            entry_found = 1;
            if (prev == NULL) { /* if first entry */
                *mac_used_p = mac->next;
                mac->next = *mac_free_p;
                *mac_free_p = mac;
                T_N("Deleted first entry.");
            } else { /* Remove entry and add it to the free list */
                prev->next = mac->next;
                mac->next = *mac_free_p;
                *mac_free_p = mac;
                T_N("Deleted one entry.");
            }
            break;
        }
    }

    /* If deleting from the local secondary switch table the return now  */
    if (msg_switch_is_primary()) {
        /* If non-volatile update flash configuration */
        if (entry_found) {
            // do nothing
        } else {
            T_D("Entry for removal not found.");
            MAC_CRIT_EXIT();
            return MAC_ERROR_MAC_NOT_EXIST;
        }

        T_N("Deleted one entry from chip.");
    }

    MAC_CRIT_EXIT();
    return VTSS_RC_OK;
}

/****************************************************************************/
// Read/create mac age configuration
/****************************************************************************/
static void mac_conf_age_default(void)
{
    mac_age_conf_t conf;
    vtss_isid_t    isid;


    /* Use default values */
    conf.mac_age_time = MAC_AGE_TIME_DEFAULT;

    MAC_CRIT_ENTER();
    mac_config.conf = conf;
    age_time_temp.stored_conf = conf;
    MAC_CRIT_EXIT();

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++)  {
        if (!msg_switch_exists(isid)) {
            continue;
        }

        if (mac_age_stack_conf_set(isid) != VTSS_RC_OK) {
            T_W("Could not set aging");
        }
    }
}

/****************************************************************************/
// Read/create static MAC configuration
/****************************************************************************/
static void mac_conf_static_default(void)
{
    switch_iter_t      sit;

    /* Flush the primary switch list */
    flush_mac_list(TRUE);

    /* Let each existing switch flush their addresses locally */
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        if (mac_remove_locked(sit.isid) != VTSS_RC_OK) {
            T_W("Could not remove locked addresses isid: %d", sit.isid);
        }
    }

    /* Reset the linked mac list */
    build_mac_list();
}

/****************************************************************************/
// Read/create mac learn configuration
/****************************************************************************/
static void mac_conf_learn_default(void)
{
    mesa_learn_mode_t     learn_mode;
    BOOL                  learn_force_en = FALSE;
    mesa_port_no_t        port_no;
    u32                   port_count;
    switch_iter_t         sit;

    MAC_CRIT_ENTER();

    /* Automatic learning mode is default */
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    while (switch_iter_getnext(&sit)) {
        for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            VTSS_BF_SET(mac_learn_mode.learn_mode[sit.isid][port_no], LEARN_AUTOMATIC, 1);
            VTSS_BF_SET(mac_learn_mode.learn_mode[sit.isid][port_no], LEARN_CPU, 0);
            VTSS_BF_SET(mac_learn_mode.learn_mode[sit.isid][port_no], LEARN_DISCARD, 0);
        }
    }

    T_D("Setting mac learn mode to default");

    /* Default all switches in the stack */
    learn_mode.automatic = 1;
    learn_mode.cpu       = 0;
    learn_mode.discard   = 0;

    /* If 'learn_force' is enabled then that port must not be defaulted */
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    while (switch_iter_getnext(&sit)) {
        for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            if (learn_force.enable[sit.isid][port_no]) {
                learn_force_en = TRUE;
                break;
            }
        }
    }

    if (learn_force_en) {
        // Send configuration to all existing switches.
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            port_count = port_count_max();
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (!learn_force.enable[sit.isid][port_no]) {
                    MAC_CRIT_EXIT();
                    if (mac_stack_learn_mode_set(sit.isid, port_no, &learn_mode) != VTSS_RC_OK) {
                        T_W("Could not set learn mode");
                    }
                    MAC_CRIT_ENTER();
                }
            }
        }
    } else {
        // Send configuration to all existing switches.
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            MAC_CRIT_EXIT();
            if (mac_stack_learn_mode_set(sit.isid, MAC_ALL_PORTS, &learn_mode) != VTSS_RC_OK) {
                T_W("Could not set learn mode");
            }
            MAC_CRIT_ENTER();
        }
    }
    MAC_CRIT_EXIT();
}

/****************************************************************************/
// Default mac module configuration
/****************************************************************************/
static void mac_conf_default(void)
{
    vtss_appl_mac_vid_learn_mode_t mode;
    T_D("Enter");
    mac_conf_age_default();
    mac_conf_static_default();
    mac_conf_learn_default();
    mode.learning = 1;
    (void)mac_mgmt_vlan_learn_mode_set(VTSS_VID_ALL, &mode);

    /* Flush the dynamic MAC table */
    if (mac_mgmt_table_flush() != VTSS_RC_OK) {
        T_W("Could not flush");
    }
    /* enable normal mac flushing */
    MAC_CRIT_ENTER();
    memset(mac_config.flush_port_ignore, 0, sizeof(mac_config.flush_port_ignore));
    MAC_CRIT_EXIT();
    T_D("Exit");
}

/****************************************************************************/
/****************************************************************************/
static void mac_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    if (!status->link) {
        vtss_isid_t isid = get_local_isid();
        if (!VTSS_ISID_LEGAL(isid)) {
            T_E("ISID Id is not legal");
            return;
        }
        if (mac_config.flush_port_ignore[isid][port_no - VTSS_PORT_NO_START]) {
            return; // The port is bypassed
        }
        /* Port is down, flush the MAC table for that port */
        (void)mesa_mac_table_port_flush(NULL, port_no);
    }
}

/* Module start */
static void mac_start(void)
{
    T_D("Enter Mac start");

    /* Register for stack messages */
    if (mac_stack_register() != VTSS_RC_OK) {
        T_W("Could not register for stack messages");
    }

    /* Register for Port LOCAL change callback */
    (void)port_change_register(VTSS_MODULE_ID_MAC, mac_port_state_change_callback);

    /* Disable the force secure system mode */
    MAC_CRIT_ENTER();
    memset(&learn_force, 0, sizeof(learn_force));
    MAC_CRIT_EXIT();

    /* Reset the linked mac list */
    build_mac_list();

    /* For packet module registration. Those fields will not change */
    packet_rx_filter_init(&mac_frame_rx_filter);
    mac_frame_rx_filter.modid = VTSS_MODULE_ID_MAC;
    mac_frame_rx_filter.match = PACKET_RX_FILTER_MATCH_SRC_PORT;
    mac_frame_rx_filter.prio  = PACKET_RX_FILTER_PRIO_BELOW_NORMAL;
    mac_frame_rx_filter.cb    = mac_filter_rx_callback;

    T_D("exit");
}

static mesa_rc vtss_appl_mac_table_get_priv(mesa_vid_t vid, mesa_mac_t mac,
                                            vtss_appl_mac_stack_addr_entry_t *const return_mac, mac_mgmt_addr_type_t *const type)
{
    port_iter_t            pit;
    switch_iter_t          sit;
    mesa_vid_mac_t         indx;
    mac_mgmt_table_stack_t stack_entry;
    mesa_rc                rc = VTSS_RC_ERROR;
//    vtss::PortListStackable  &destination = (vtss::PortListStackable &)return_mac->destination;


    T_I("vtss_appl_mac_table_get_priv: only vlan:%d only conf:%d not dynamic:%d not static:%d not cpu:%d\n",
        type->only_this_vlan, type->only_in_conf, type->not_dynamic, type->not_static, type->not_cpu);
    indx.vid = vid;
    indx.mac = mac;
    if ((rc = mac_mgmt_stack_get_next(&indx, &stack_entry, type, 0)) == VTSS_RC_OK) {
        return_mac->vid_mac = indx;
        return_mac->copy_to_cpu = stack_entry.copy_to_cpu;
        return_mac->dynamic = !stack_entry.locked;
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
        while (switch_iter_getnext(&sit)) {
            (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (stack_entry.destination[sit.isid][pit.iport]) {
                    (void)portlist_index_set(isid_port_to_index(sit.isid, pit.iport), &return_mac->destination);
                } else {
                    (void)portlist_index_clear(isid_port_to_index(sit.isid, pit.iport), &return_mac->destination);
                }
            }
        }
    }
    return rc;
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/****************************************************************************/
// Get the age time locally
/****************************************************************************/
mesa_rc mac_mgmt_age_time_get(mac_age_conf_t *const conf)
{

    MAC_CRIT_ENTER();
    *conf = mac_config.conf;
    MAC_CRIT_EXIT();
    return VTSS_RC_OK;
}

/****************************************************************************/
// Set the age time to all swithces in the stack
/****************************************************************************/
mesa_rc mac_mgmt_age_time_set(const mac_age_conf_t *const conf)
{
    int                  changed = 0;
    mesa_rc              rc = VTSS_RC_OK;
    vtss_isid_t          isid;

    T_D("enter, age time set %u", conf->mac_age_time);

    if (!msg_switch_is_primary()) {
        T_D("not primary switch");
        return MAC_ERROR_STACK_STATE;
    }
    if ((conf->mac_age_time >= MAC_AGE_TIME_MIN &&
         conf->mac_age_time <= MAC_AGE_TIME_MAX) ||
        conf->mac_age_time == MAC_AGE_TIME_DISABLE  ) {

        MAC_CRIT_ENTER();
        changed = mac_age_changed(&mac_config.conf, conf);
        mac_config.conf = *conf;
        age_time_temp.stored_conf = *conf;;
        MAC_CRIT_EXIT();

        if (changed) {
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                if (!msg_switch_exists(isid)) {
                    continue;
                }
                rc = mac_age_stack_conf_set(isid);
                T_D("Updated agetime in switch usid:%d", isid);
            }
        }
    } else {
        rc = VTSS_INVALID_PARAMETER;
    }
    return rc;
}

/****************************************************************************/
// Use another age time for specified time
/****************************************************************************/
mesa_rc mac_age_time_set(ulong mac_age_time, ulong age_period)
{
    MAC_CRIT_ENTER();
    age_time_temp.temp_conf.mac_age_time = mac_age_time;
    age_time_temp.age_period = age_period;
    /* Abort any aging + restart */
    T_D("%s(" VPRIlu", " VPRIlu")", __FUNCTION__, mac_age_time, age_period);
    vtss_flag_setbits( &age_flags, MACFLAG_ABORT_AGE | MACFLAG_START_AGE);
    MAC_CRIT_EXIT();

    return VTSS_RC_OK;
}

/****************************************************************************/
// Disable / enable auto mac address flushing
/****************************************************************************/
mesa_rc mac_mgmt_port_mac_table_auto_flush(vtss_isid_t isid, mesa_port_no_t port_no, BOOL enable)
{
    if (mac_mgmt_port_sid_invalid(isid, port_no, 1, TRUE)) {
        return MAC_ERROR_STACK_STATE;
    }
    MAC_CRIT_ENTER();
    mac_config.flush_port_ignore[isid][port_no - VTSS_PORT_NO_START] = !enable;
    MAC_CRIT_EXIT();
    return VTSS_RC_OK;
}

/****************************************************************************/
// Get the next address from a switch in the Stack
/****************************************************************************/
mesa_rc mac_mgmt_table_get_next(vtss_isid_t isid, mesa_vid_mac_t *vid_mac, mesa_mac_table_entry_t *const entry, BOOL next)
{
    T_N("Doing a getnext at switch isid:%d", isid );

    if (mac_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, 0, FALSE)) {
        return MAC_ERROR_GEN;
    }

    return mac_local_table_get_next(vid_mac, entry, next);
}


/****************************************************************************/
/****************************************************************************/

mesa_rc mac_mgmt_stack_get_next(mesa_vid_mac_t *vid_mac, mac_mgmt_table_stack_t *entry, mac_mgmt_addr_type_t *type, BOOL next)
{
    mesa_mac_table_entry_t       next_entry;
    vtss_isid_t                  local_isid;
    u32                          i;
    switch_iter_t                sit;

    if (!msg_switch_is_primary()) {
        T_D("not primary switch");
        return MAC_ERROR_STACK_STATE;
    }

    vtss_clear(*entry);

    local_isid = get_local_isid();
    MAC_MGMT_ASSERT(!VTSS_ISID_LEGAL(local_isid), "Failed getting local ISID id");


    while (1) {
        if (!type->only_in_conf) {
            if (next) {
                if (mesa_mac_table_get_next(NULL, vid_mac, &next_entry) != VTSS_RC_OK) {
                    break;
                }
            } else {
                if (mesa_mac_table_get(NULL, vid_mac, &next_entry) != VTSS_RC_OK) {
                    break;
                }
            }

            next_entry.copy_to_cpu |= next_entry.copy_to_cpu_smac;

            T_D("vid = %d, mac = %x:%x:%x:%x:%x:%x Next:%d", next_entry.vid_mac.vid,
                next_entry.vid_mac.mac.addr[1], next_entry.vid_mac.mac.addr[2], next_entry.vid_mac.mac.addr[3], next_entry.vid_mac.mac.addr[4],
                next_entry.vid_mac.mac.addr[4], next_entry.vid_mac.mac.addr[5], next);
        } else {
            // Look in the conf table
            mac_mgmt_addr_entry_t  mac_entry;
            mesa_vid_mac_t         vid_mac_tmp;
            memset(&vid_mac_tmp, 0, sizeof(vid_mac_tmp));
            T_D("vid = %d, mac = %x:%x:%x:%x:%x:%x Next:%d", vid_mac->vid,
                vid_mac->mac.addr[1], vid_mac->mac.addr[2], vid_mac->mac.addr[3], vid_mac->mac.addr[4],
                vid_mac->mac.addr[4], vid_mac->mac.addr[5], next);

            (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
            while (switch_iter_getnext(&sit)) {
                if (mac_mgmt_static_get_next(sit.isid, vid_mac, &mac_entry, next, MAC_NON_VOLATILE) == VTSS_RC_OK) {
                    /* Must return the addresses lowest to highest regardless of ISID */
                    if (vid_mac_bigger(&entry->vid_mac, &mac_entry.vid_mac) || !entry->locked) {
                        entry->vid_mac = mac_entry.vid_mac;
                        for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
                            entry->destination[sit.isid][i] = mac_entry.destination[i];
                        }
                        entry->copy_to_cpu = mac_entry.copy_to_cpu;
                        entry->locked = 1;
                    }
                }
            }
            if (entry->locked) { /* i.e. found */
                return VTSS_RC_OK;
            } else {
                return MAC_ERROR_NOT_FOUND;
            }
        }
        /* These we don't want */
        if ((type->only_this_vlan && (next_entry.vid_mac.vid != vid_mac->vid)) ||
            (type->not_dynamic    && !next_entry.locked) ||
            (type->not_static     && next_entry.locked) ||
            (type->not_cpu        && next_entry.copy_to_cpu) ||
            (type->not_mc         && (next_entry.vid_mac.mac.addr[0] & 0x1)) ||
            (type->not_uc         && !(next_entry.vid_mac.mac.addr[0] & 0x1))) {
            if (!next) {
                return MAC_ERROR_NOT_FOUND;
            }
            *vid_mac = next_entry.vid_mac;
            continue;
        }

        entry->vid_mac = next_entry.vid_mac;
        for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
            entry->destination[local_isid][i] = next_entry.destination[i];
        }
        entry->copy_to_cpu = next_entry.copy_to_cpu;
        entry->locked = next_entry.locked;
        return VTSS_RC_OK;
    }

    return MAC_ERROR_NOT_FOUND;
}

/****************************************************************************/
// Flush MAC address table
/****************************************************************************/
mesa_rc mac_mgmt_table_flush(void)
{
    mac_msg_req_t *msg;
    switch_iter_t sit;

    T_D("Flush mac table");

    if (!msg_switch_is_primary()) {
        if (mesa_mac_table_flush(NULL) != VTSS_RC_OK) {
            T_W("Could not flush");
        }
        return VTSS_RC_OK;
    }

    /* Run through all switches and flush the MAC table */
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);

    if ((msg = mac_msg_req_alloc(MAC_MSG_ID_FLUSH_REQ, sit.remaining)) != NULL) {
        while (switch_iter_getnext(&sit)) {
            mac_msg_tx(msg, sit.isid, 0);
        }
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_table_vlan_stats_get(vtss_isid_t isid, mesa_vid_t vlan, mac_table_stats_t *stats)
{
    mac_msg_req_t *msg;
    mesa_rc       rc = VTSS_RC_OK;

    T_N("Doing a get vlan stats at switch isid:%d", isid);

    if (mac_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, 0, FALSE)) {
        return MAC_ERROR_STACK_STATE;
    }
    if (vlan < MAC_MGMT_VLAN_ID_MIN || vlan > MAC_MGMT_VLAN_ID_MAX) {
        T_E("Invalid VID (%u)", vlan);
        return MAC_ERROR_VLAN_INVALID;
    }

    if (msg_switch_is_local(isid)) {
        rc = mac_local_table_get_vlan_stats(vlan, stats);
    } else {
        msg = mac_msg_req_alloc(MAC_MSG_ID_GET_STATS_REQ, 1);
        msg->req.stats.vlan = vlan;
        mac_msg_tx(msg, isid, sizeof(msg->req.stats));

        /* Wait for reply from stack */
        vtss_flag_value_t flags = vtss_flag_timed_wait( &stats_flags, MACFLAG_WAIT_STATS_DONE | MACFLAG_COULD_NOT_TX,
                                                        VTSS_FLAG_WAITMODE_OR_CLR,
                                                        vtss_current_time() + VTSS_OS_MSEC2TICK(MAC_REQ_TIMEOUT * 1000));
        if (flags == MACFLAG_COULD_NOT_TX) {
            return MAC_ERROR_STACK_STATE;
        }
        if (flags == 0) {
            T_W("Timeout or could not tx, MAC_MSG_ID_GET_NEXT_REQ ISID:%d", isid);
            return MAC_ERROR_REQ_TIMEOUT;
        }

        MAC_CRIT_ENTER();
        *stats = mac_config.get_stats.stats;
        rc = mac_config.get_stats.rc;
        MAC_CRIT_EXIT();
    }
    return rc;
}

/****************************************************************************/
// Get the MAC statistics from a switch (isid) in the stack
/****************************************************************************/
mesa_rc mac_mgmt_table_stats_get(vtss_isid_t isid, mac_table_stats_t *stats)
{
    T_N("Doing a get stats at switch isid:%d", isid );

    if (mac_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, 0, FALSE)) {
        return MAC_ERROR_STACK_STATE;
    }

    return mac_local_table_get_stats(stats);
}

/****************************************************************************/
// Add volatile or non-volatile static mac address to chip
/****************************************************************************/

mesa_rc mac_mgmt_table_add(vtss_isid_t isid, mac_mgmt_addr_entry_t *entry)
{
    if (isid == VTSS_ISID_LOCAL) {
        isid = get_local_isid();
        MAC_MGMT_ASSERT(!VTSS_ISID_LEGAL(isid), "Failed getting local ISID id");
    }

    if (msg_switch_is_primary()) {
        T_D("Mac add %s, VLAN %u, vol:%u", misc_mac2str(entry->vid_mac.mac.addr), entry->vid_mac.vid, entry->volatil);
        return mac_table_add(isid, entry);
    } else {
        T_D("not primary switch");
        return MAC_ERROR_STACK_STATE;
    }
}

/****************************************************************************/
// Delete all volatile or non-volatile static mac addresses from the port. Per switch.
/****************************************************************************/
mesa_rc mac_mgmt_table_port_del(vtss_isid_t isid, mesa_port_no_t port_no, BOOL vol)
{
    mac_mgmt_addr_entry_t mac_entry;
    mesa_vid_mac_t        vid_mac;
    mesa_rc               rc = VTSS_RC_OK;

    if (mac_mgmt_port_sid_invalid(isid, port_no, 1, TRUE)) {
        return MAC_ERROR_GEN;
    }
    T_D("Port mac del, Port %u, vol:%u", port_no, vol);
    memset(&vid_mac, 0, sizeof(vid_mac));
    while (mac_mgmt_static_get_next(isid, &vid_mac, &mac_entry, 1, vol) == VTSS_RC_OK) {
        if (mac_entry.destination[port_no]) {
            rc = mac_mgmt_table_del(isid, &mac_entry.vid_mac, vol);
        }

        vid_mac = mac_entry.vid_mac;
    }

    return rc;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_table_del(vtss_isid_t isid, mesa_vid_mac_t *conf, BOOL vol)
{
    if (mac_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, 1, FALSE)) {
        return MAC_ERROR_GEN;
    }
    T_D("Mac del %s, VLAN %u, vol:%u", misc_mac2str(conf->mac.addr), conf->vid, vol);
    return mac_table_del(isid, conf, vol);
}

/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_static_get_next(vtss_isid_t isid, mesa_vid_mac_t *search_mac, mac_mgmt_addr_entry_t *return_mac, BOOL next, BOOL vol)
{
    mac_static_t       *mac, *temp, *mac_used_p, *mac_prev;
    uint               port_no, i;
    mesa_rc            rc;


    if (isid == VTSS_ISID_LOCAL) {
        isid = get_local_isid();
        MAC_MGMT_ASSERT(!VTSS_ISID_LEGAL(isid), "Failed getting local ISID id");
    }

    MAC_CRIT_ENTER();
    if (vol) {
        mac_used_p = mac_used_vol;
    } else {
        mac_used_p = mac_used;
    }

    /* Flashed addresses are always locked, with stack ports included  */
    return_mac->volatil = vol;
    T_D("getting mac entry:%02x-%02x-%02x-%02x-%02x-%02x vid:%d, Next:%d, Volatile:%d, isid:%d",
        search_mac->mac.addr[0], search_mac->mac.addr[1], search_mac->mac.addr[2],
        search_mac->mac.addr[3], search_mac->mac.addr[4], search_mac->mac.addr[5], search_mac->vid, next, vol, isid);

    if (next) {
        T_N("Doing a get next at switch isid:%d. Volatile:%d", isid, vol);

        /* If the table is empty */
        if (mac_used_p == NULL) {
            rc = VTSS_UNSPECIFIED_ERROR;
            goto exit_func;
        }

        for (mac = mac_used_p, i = 1; mac != NULL; mac = mac->next, i++) {
            T_N("Entry %d Isid:%d:%02x-%02x-%02x-%02x-%02x-%02x vid:%d", i, mac->conf.isid,
                mac->conf.vid_mac.mac.addr[0], mac->conf.vid_mac.mac.addr[1], mac->conf.vid_mac.mac.addr[2],
                mac->conf.vid_mac.mac.addr[3], mac->conf.vid_mac.mac.addr[4], mac->conf.vid_mac.mac.addr[5], mac->conf.vid_mac.vid);
        }

        /* Search the list for a matching entry and return the next entry */
        for (mac = mac_used_p; mac != NULL; mac = mac->next) {
            if (mac_vid_compare(&mac->conf.vid_mac, search_mac) && mac->conf.isid == isid) {

                if (mac->next == NULL) {
                    rc = VTSS_UNSPECIFIED_ERROR;
                    goto exit_func;
                }

                /* Look for the next entry with the same isid */
                for (mac_prev = mac, mac = mac->next; mac != NULL; mac_prev = mac, mac = mac->next) {
                    if (mac->conf.isid == isid) {
                        mac = mac_prev;
                        break;
                    }
                }

                if (mac == NULL) {
                    rc = VTSS_UNSPECIFIED_ERROR;
                    goto exit_func;
                }

                if (mac->next != NULL) {
                    temp = mac->next;
                    return_mac->vid_mac = temp->conf.vid_mac;
                    return_mac->dynamic = (temp->conf.mode & MAC_ADDR_DYNAMIC) ? 1 : 0;
                    return_mac->copy_to_cpu = temp->conf.copy_to_cpu;
                    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
                        return_mac->destination[port_no] = VTSS_PORT_BF_GET(temp->conf.destination, port_no);
                    }

                    T_D("Found entry. Isid:%d. Returning next mac entry:%02x-%02x-%02x-%02x-%02x-%02x vid:%d",
                        temp->conf.isid,
                        return_mac->vid_mac.mac.addr[0], return_mac->vid_mac.mac.addr[1], return_mac->vid_mac.mac.addr[2],
                        return_mac->vid_mac.mac.addr[3], return_mac->vid_mac.mac.addr[4], return_mac->vid_mac.mac.addr[5],
                        return_mac->vid_mac.vid);

                    rc = VTSS_RC_OK;
                    goto exit_func;
                } else {
                    /* last entry */
                    rc = VTSS_UNSPECIFIED_ERROR;
                    goto exit_func;
                }
            } else {
                /* if this address is larger than the search address then return it */
                if (vid_mac_bigger(&mac->conf.vid_mac, search_mac) && mac->conf.isid == isid) {
                    return_mac->vid_mac = mac->conf.vid_mac;
                    return_mac->dynamic = (mac->conf.mode & MAC_ADDR_DYNAMIC) ? 1 : 0;
                    return_mac->copy_to_cpu = mac->conf.copy_to_cpu;
                    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
                        return_mac->destination[port_no] = VTSS_PORT_BF_GET(mac->conf.destination, port_no);
                    }

                    T_D("Did not find entry. Return the next mac entry:%02x-%02x-%02x-%02x-%02x-%02x vid:%d",
                        return_mac->vid_mac.mac.addr[0], return_mac->vid_mac.mac.addr[1], return_mac->vid_mac.mac.addr[2],
                        return_mac->vid_mac.mac.addr[3], return_mac->vid_mac.mac.addr[4], return_mac->vid_mac.mac.addr[5],
                        return_mac->vid_mac.vid);

                    rc = VTSS_RC_OK;
                    goto exit_func;
                }
            }
        }
        rc = VTSS_UNSPECIFIED_ERROR;
        goto exit_func;
    } else {
        /* Do a Lookup */
        T_N("Do a lookup");
        for (mac = mac_used_p; mac != NULL; mac = mac->next) {
            if (mac_vid_compare(&mac->conf.vid_mac, search_mac) && mac->conf.isid == isid) {
                return_mac->vid_mac = mac->conf.vid_mac;
                return_mac->dynamic = (mac->conf.mode & MAC_ADDR_DYNAMIC) ? 1 : 0;
                return_mac->copy_to_cpu = mac->conf.copy_to_cpu;
                for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
                    return_mac->destination[port_no] = VTSS_PORT_BF_GET(mac->conf.destination, port_no);
                }
                T_D("Lookup Found");
                rc = VTSS_RC_OK;
                goto exit_func;
            }
        }
        T_D("Lookup NOT Found");
        rc = VTSS_UNSPECIFIED_ERROR;
        goto exit_func;
    }

exit_func:
    MAC_CRIT_EXIT();
    return rc;
}
/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_vlan_learn_mode_set(mesa_vid_t vid, const vtss_appl_mac_vid_learn_mode_t *const mode)
{
    mesa_rc            rc = VTSS_RC_OK;

    if ((rc = mac_do_set_local_vlan_learn_mode(vid, mode)) != VTSS_RC_OK) {
        T_W("Could set learn mode, rc = %d", rc);
    }
    return rc;
}
/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_vlan_learn_mode_get(mesa_vid_t vid, vtss_appl_mac_vid_learn_mode_t *const mode)
{
    mesa_rc              rc = VTSS_RC_OK;
    mesa_vlan_vid_conf_t api_mode;
    VTSS_RC(mesa_vlan_vid_conf_get(NULL, vid, &api_mode));
    mode->learning = api_mode.learning;
    return rc;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_learning_disabled_vids_get(u8 vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    MAC_CRIT_ENTER();
    memcpy(vids, lrn_disabled_vids, sizeof(lrn_disabled_vids));
    MAC_CRIT_EXIT();
    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_learning_disabled_vids_set(u8 vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    u8                              old_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    mesa_vid_t                      vid;
    vtss_appl_mac_vid_learn_mode_t  mode;
    mesa_rc                         rc = VTSS_RC_OK;

    (void)mac_mgmt_learning_disabled_vids_get(old_vids);

    (void)vlan_bulk_update_begin();

    // Create or delete VIDs as requested.
    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        BOOL old_learning_disabled = VTSS_BF_GET(old_vids, vid);
        if (old_learning_disabled != VTSS_BF_GET(vids, vid)) {
            mode.learning = old_learning_disabled;
            T_D("%s VLAN %d learning", mode.learning ? "ENABLE" : "DIABLE", vid);
            if ((rc = mac_mgmt_vlan_learn_mode_set(vid, &mode)) != VTSS_RC_OK) {
                break;
            }
        }
    }

    (void)vlan_bulk_update_end();

    return rc;
}


/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_learn_mode_set(vtss_isid_t isid, mesa_port_no_t port_no, const mesa_learn_mode_t *const learn_mode)
{
    mesa_rc rc = VTSS_RC_OK;
    BOOL    lf;

    T_D("Learn mode set, port:%u auto:%d cpu:%d discard:%d", port_no, learn_mode->automatic, learn_mode->cpu, learn_mode->discard);

    if (mac_mgmt_port_sid_invalid(isid, port_no, 1, TRUE)) {
        return MAC_ERROR_STACK_STATE;
    }

    MAC_CRIT_ENTER();
    lf = learn_force.enable[isid][port_no];

    if (lf) {
        // We need to check that the new learn mode is not different from the old, since
        // the user shouldn't be able to change it when a particular learn mode is forced
        // by another module (notably the Port Security (PSEC) module).
        if (VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_AUTOMATIC) != learn_mode->automatic ||
            VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_CPU      ) != learn_mode->cpu       ||
            VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_DISCARD  ) != learn_mode->discard) {
            MAC_CRIT_EXIT();
            return MAC_ERROR_LEARN_FORCE_SECURE;
        } else {
            MAC_CRIT_EXIT();
            return VTSS_RC_OK; // No changes.
        }
    }
    MAC_CRIT_EXIT();

    if (isid == VTSS_ISID_LOCAL) {
        isid = get_local_isid();
        MAC_MGMT_ASSERT(!VTSS_ISID_LEGAL(isid), "Failed getting local ISID id");
    }

    rc = mac_stack_learn_mode_set(isid, port_no, learn_mode);

    if (rc == VTSS_RC_OK && !lf) {
        MAC_CRIT_ENTER();
        VTSS_BF_SET(mac_learn_mode.learn_mode[isid][port_no], LEARN_AUTOMATIC, learn_mode->automatic);
        VTSS_BF_SET(mac_learn_mode.learn_mode[isid][port_no], LEARN_CPU,       learn_mode->cpu);
        VTSS_BF_SET(mac_learn_mode.learn_mode[isid][port_no], LEARN_DISCARD,   learn_mode->discard);
        MAC_CRIT_EXIT();
    }

    VTSS_RC(mac_register_packet_callback(isid, port_no));

    return rc;
}

/****************************************************************************/
/****************************************************************************/
void mac_mgmt_learn_mode_get(vtss_isid_t isid, mesa_port_no_t port_no, mesa_learn_mode_t *const learn_mode, BOOL *chg_allowed)
{
    /* Return the learn mode, as we know it from our conf. */
    *chg_allowed = 1;

    if (VTSS_ISID_LEGAL(isid)) {
        // Always return the user's preferences, i.e. if some Module
        // has overridden the current mode, then just set the chg_allowed to FALSE.
        MAC_CRIT_ENTER();
        if (learn_force.enable[isid][port_no]) {
            *chg_allowed = 0;
        }
        learn_mode->automatic = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_AUTOMATIC);
        learn_mode->cpu =       VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_CPU);
        learn_mode->discard =   VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_DISCARD);
        MAC_CRIT_EXIT();
    } else {
        T_D("isid:%d is not a legal isid", isid);
    }
}

/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_learn_mode_force_secure(vtss_isid_t isid, mesa_port_no_t port_no, BOOL cpu_copy)
{
    T_D("Setting isid:%d port:%d t forced secure learning cpu and discard = 1", isid, port_no);

    if (mac_mgmt_port_sid_invalid(isid, port_no, 1, TRUE)) {
        return MAC_ERROR_STACK_STATE;
    }
    MAC_CRIT_ENTER();
    learn_force.enable[isid][port_no] = 1;
    learn_force.learn_mode.automatic = 0;
    learn_force.learn_mode.discard = 1;
    learn_force.learn_mode.cpu = cpu_copy;
    MAC_CRIT_EXIT();
    if (mac_stack_learn_mode_set(isid, port_no, &learn_force.learn_mode) != VTSS_RC_OK) {
        T_W("Could not set learn mode mode force");
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/
mesa_rc mac_mgmt_learn_mode_revert(vtss_isid_t isid, mesa_port_no_t port_no)
{
    mesa_learn_mode_t learn_mode;

    T_D("Learn mode revert, port:%u", port_no);

    if (mac_mgmt_port_sid_invalid(isid, port_no, 1, TRUE)) {
        return MAC_ERROR_STACK_STATE;
    }

    MAC_CRIT_ENTER();
    if (!learn_force.enable[isid][port_no]) {
        MAC_CRIT_EXIT();
        return VTSS_RC_OK;
    }
    learn_force.enable[isid][port_no] = 0;

    /* Get the saved learn mode */
    learn_mode.automatic = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_AUTOMATIC);
    learn_mode.cpu = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_CPU);
    learn_mode.discard = VTSS_BF_GET(mac_learn_mode.learn_mode[isid][port_no], LEARN_DISCARD);
    MAC_CRIT_EXIT();
    if (mac_stack_learn_mode_set(isid, port_no, &learn_mode) != VTSS_RC_OK) {
        T_W("Could not set learn mode");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}


/****************************************************************************/
// Return an error text string based on a return code.
/****************************************************************************/
const char *mac_error_txt(mesa_rc rc)
{
    switch (rc) {
    case MAC_ERROR_GEN:
        return "General error";

    case MAC_ERROR_MAC_RESERVED:
        return "MAC address is reserved";

    case MAC_ERROR_REG_TABLE_FULL:
        return "Registration table full";

    case MAC_ERROR_REQ_TIMEOUT:
        return "Timeout on message request";

    case MAC_ERROR_STACK_STATE:
        return "Illegal primary/secondary switch state";

    case MAC_ERROR_MAC_EXIST:
        return "MAC address already exists";

    case MAC_ERROR_MAC_SYSTEM_EXIST:
        return "MAC address exists (system address)";

    case MAC_ERROR_MAC_VOL_EXIST:
        return "Volatile mac address already exists";

    case MAC_ERROR_MAC_ONE_DESTINATION_ALLOWED:
        return "Only one destination is allowed";

    case MAC_ERROR_VLAN_INVALID:
        return "Invalid VLAN";

    default:
        return "MAC: Unknown error code";
    }
}

/****************************************************************************/
// Public headers exposed in vtss_appl/include/vtss/appl/mac.h
/****************************************************************************/
mesa_rc vtss_appl_mac_learn_mode_get(vtss_ifindex_t    ifindex,
                                     vtss_appl_mac_learn_mode_t *const mode)
{
    vtss_ifindex_elm_t ife;
    BOOL chg_allowed;
    mesa_learn_mode_t lm;

    T_I("vtss_appl_mac_learn_mode_get");
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        return VTSS_RC_ERROR;
    }

    (void)mac_mgmt_learn_mode_get(ife.isid, ife.ordinal, &lm, &chg_allowed);
    mode->chg_allowed = chg_allowed;
    if (lm.discard) {
        mode->learn_mode = VTSS_APPL_MAC_LEARNING_SECURE;
    } else if (lm.automatic) {
        mode->learn_mode = VTSS_APPL_MAC_LEARNING_AUTO;
    } else {
        mode->learn_mode = VTSS_APPL_MAC_LEARNING_DISABLE;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mac_learn_mode_set(vtss_ifindex_t    ifindex,
                                     const vtss_appl_mac_learn_mode_t *const mode)
{
    vtss_ifindex_elm_t ife;
    mesa_learn_mode_t lm = {0};

    T_I("vtss_appl_mac_learn_mode_set");
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        return VTSS_RC_ERROR;
    }

    if (mode->learn_mode == VTSS_APPL_MAC_LEARNING_SECURE) {
        lm.discard = 1;
    } else if (mode->learn_mode == VTSS_APPL_MAC_LEARNING_AUTO) {
        lm.automatic = 1;
    } else if (mode->learn_mode == VTSS_APPL_MAC_LEARNING_DISABLE) {
        lm.automatic = 0;
    } else {
        return VTSS_RC_ERROR;
    }

    return mac_mgmt_learn_mode_set(ife.isid, ife.ordinal, &lm);
}

mesa_rc vtss_appl_mac_vlan_learn_mode_set(mesa_vid_t     vid,
                                          const vtss_appl_mac_vid_learn_mode_t *const mode)
{
    if (vid < MAC_MGMT_VLAN_ID_MIN || vid > MAC_MGMT_VLAN_ID_MAX) {
        return MAC_ERROR_VLAN_INVALID;
    }
    return mac_mgmt_vlan_learn_mode_set(vid, mode);
}

mesa_rc vtss_appl_mac_vlan_learn_mode_get(mesa_vid_t     vid,
                                          vtss_appl_mac_vid_learn_mode_t *const mode)
{
    if (vid < MAC_MGMT_VLAN_ID_MIN || vid > MAC_MGMT_VLAN_ID_MAX) {
        return MAC_ERROR_VLAN_INVALID;
    }
    return mac_mgmt_vlan_learn_mode_get(vid, mode);
}


mesa_rc vtss_appl_mac_age_time_get(vtss_appl_mac_age_conf_t *const conf)
{
    T_I("vtss_appl_mac_age_time_get");
    return  mac_mgmt_age_time_get(conf);
}

mesa_rc vtss_appl_mac_age_time_set(const vtss_appl_mac_age_conf_t *const conf)
{
    T_I("vtss_appl_mac_age_time_set");
    return  mac_mgmt_age_time_set(conf);
}

mesa_rc vtss_appl_mac_table_conf_set(mesa_vid_t vid, mesa_mac_t mac,
                                     const vtss_appl_mac_stack_addr_entry_conf_t *const entry)
{
    mac_mgmt_addr_entry_t      stack_entry = {{0}};
    port_iter_t                pit;
    switch_iter_t              sit;
    mesa_rc                    rc;
    BOOL                       dest_found = 0;
//    vtss::PortListStackable  &destination = (vtss::PortListStackable &)entry->destination;

    T_I("vtss_appl_mac_table_set");
    stack_entry.vid_mac.vid = vid;
    stack_entry.vid_mac.mac = mac;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            stack_entry.destination[pit.iport] = portlist_index_get(isid_port_to_index(sit.isid, pit.iport), &entry->destination);
            if (stack_entry.destination[pit.iport]) {
                dest_found = 1;
            }
        }
        if (dest_found) {
            if ((rc = mac_mgmt_table_add(sit.isid, &stack_entry)) != VTSS_RC_OK) {
                T_W("Could not add mac address. Reason:%s", mac_error_txt(rc));
                return rc;
            }
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_mac_table_conf_del(mesa_vid_t vid, mesa_mac_t mac)
{
    mesa_vid_mac_t indx;
    mac_mgmt_addr_entry_t  return_mac;
    switch_iter_t sit;

    T_I("vtss_appl_mac_table_del");
    indx.vid = vid;
    indx.mac = mac;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        if (mac_mgmt_static_get_next(sit.isid, &indx, &return_mac, 0, MAC_NON_VOLATILE) == VTSS_RC_OK) {
            return mac_mgmt_table_del(sit.isid, &indx, 0);
        }
    }
    return MAC_ERROR_NOT_FOUND;
}

mesa_rc vtss_appl_mac_table_conf_default(mesa_vid_t *vid, mesa_mac_t *mac,
                                         vtss_appl_mac_stack_addr_entry_conf_t *const entry)
{
    memset(entry, 0, sizeof(*entry));
    *vid = MESA_VID_DEFAULT;
    entry->vid_mac.vid = MESA_VID_DEFAULT;
    return VTSS_RC_OK;
}
mesa_rc vtss_appl_mac_table_get(mesa_vid_t vid, mesa_mac_t mac,
                                vtss_appl_mac_stack_addr_entry_t *const entry)
{
    mac_mgmt_addr_type_t type = {0, 0, 0, 0, 0, 0, 0}; // all of them
    memset(entry, 0, sizeof(*entry));
    return vtss_appl_mac_table_get_priv(vid, mac, entry, &type);
}

mesa_rc vtss_appl_mac_table_static_get(mesa_vid_t vid, mesa_mac_t mac,
                                       vtss_appl_mac_stack_addr_entry_t *const entry)
{
    mac_mgmt_addr_type_t type = {0, 0, 1, 0, 0, 0, 0}; // only static (not dynamic)
    memset(entry, 0, sizeof(*entry));
    return vtss_appl_mac_table_get_priv(vid, mac, entry, &type);
}

mesa_rc vtss_appl_mac_table_conf_get(mesa_vid_t vid, mesa_mac_t mac,
                                     vtss_appl_mac_stack_addr_entry_conf_t *const entry)
{
    vtss_appl_mac_stack_addr_entry_t return_mac = {{0}};
    mac_mgmt_addr_type_t type = {0, 1, 0, 0, 0, 0, 0}; // only in conf
    mesa_rc                rc = VTSS_RC_ERROR;

    T_I("vtss_appl_mac_table_conf_get");
    memset(entry, 0, sizeof(*entry));
    if ((rc = vtss_appl_mac_table_get_priv(vid, mac, &return_mac, &type)) == VTSS_RC_OK) {
        entry->vid_mac = return_mac.vid_mac;
        entry->destination = return_mac.destination;
    }
    return rc;
}

mesa_rc vtss_appl_mac_table_flush(const vtss_appl_mac_flush_t *const flush)
{
    if (flush->flush_all) {
        return mac_mgmt_table_flush();
    }
    return VTSS_RC_OK;
}

static mesa_rc mac_mgmt_table_itr(const mesa_vid_t *const prev_vid,
                                  mesa_vid_t       *const next_vid,
                                  const mesa_mac_t *const prev_mac,
                                  mesa_mac_t       *const next_mac,
                                  mac_mgmt_addr_type_t *type)
{

    mesa_rc rc;
    mesa_vid_mac_t prev = {0};
    mac_mgmt_table_stack_t result;

    T_I("vtss_appl_mac_table_itr");

    if (prev_vid == NULL) {
        // Get first address
        if ((rc = mac_mgmt_stack_get_next(&prev, &result, type, 0)) != VTSS_RC_OK) {
            rc = mac_mgmt_stack_get_next(&prev, &result, type, 1);
        }
    } else {
        // Have previous VID, get next MAC
        prev.vid = *prev_vid;
        if (prev_mac != NULL) {
            prev.mac = *prev_mac;
        }
        rc = mac_mgmt_stack_get_next(&prev, &result, type, 1);
    }
    *next_vid = result.vid_mac.vid;
    *next_mac = result.vid_mac.mac;
    return rc;
}



mesa_rc vtss_appl_mac_table_all_itr(const mesa_vid_t *const prev_vid,
                                    mesa_vid_t       *const next_vid,
                                    const mesa_mac_t *const prev_mac,
                                    mesa_mac_t       *const next_mac)
{
    mac_mgmt_addr_type_t type = {0, 0, 0, 0, 0, 0, 0}; // all
    return mac_mgmt_table_itr(prev_vid, next_vid, prev_mac, next_mac, &type);
}

mesa_rc vtss_appl_mac_table_conf_itr(const mesa_vid_t *const prev_vid,
                                     mesa_vid_t       *const next_vid,
                                     const mesa_mac_t *const prev_mac,
                                     mesa_mac_t       *const next_mac)
{
    mac_mgmt_addr_type_t type = {0, 1, 0, 0, 0, 0, 0}; // only_in_conf
    return mac_mgmt_table_itr(prev_vid, next_vid, prev_mac, next_mac, &type);
}

mesa_rc vtss_appl_mac_table_static_itr(const mesa_vid_t *const prev_vid,
                                       mesa_vid_t       *const next_vid,
                                       const mesa_mac_t *const prev_mac,
                                       mesa_mac_t       *const next_mac)

{
    mac_mgmt_addr_type_t type = {0, 0, 1, 0, 0, 0, 0}; // not_dynamic
    return mac_mgmt_table_itr(prev_vid, next_vid, prev_mac, next_mac, &type);
}

mesa_rc vtss_appl_mac_table_stats_get(vtss_appl_mac_table_stats_t *const stats)
{
    mesa_rc rc;
    mac_table_stats_t pstats;
    T_I("vtss_appl_mac_table_stats_get");
    if ((rc = mac_mgmt_table_stats_get(get_local_isid(), &pstats)) == VTSS_RC_OK) {
        stats->dynamic_total = pstats.learned_total;
        stats->static_total = pstats.static_total;
    }
    return rc;
}

mesa_rc vtss_appl_mac_port_stats_get(vtss_ifindex_t    ifindex,
                                     vtss_appl_mac_port_stats_t *const stats)
{
    mesa_rc rc;
    vtss_ifindex_elm_t ife;
    mac_table_stats_t pstats;

    T_I("vtss_appl_mac_port_stats_get %u", VTSS_IFINDEX_PRINTF_ARG(ifindex));
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        return VTSS_RC_ERROR;
    }

    if ((rc = mac_mgmt_table_stats_get(ife.isid, &pstats)) == VTSS_RC_OK) {
        stats->dynamic = pstats.learned[ife.ordinal];
    }
    return rc;
}

mesa_rc vtss_appl_mac_capabilities_get(vtss_appl_mac_capabilities_t *const cap)
{
    cap->mac_addr_non_volatile_max = MAC_ADDR_NON_VOLATILE_MAX;
    return VTSS_RC_OK;
}

extern "C" int mac_icli_cmd_register();

/****************************************************************************/
// Initialize module
/****************************************************************************/
mesa_rc mac_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

#ifdef VTSS_SW_OPTION_ALARM
    mac_any_init();
#endif

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT_CMD_INIT");
        critd_init(&mac_config.crit,     "mac_config",     VTSS_MODULE_ID_MAC, CRITD_TYPE_MUTEX);
        critd_init(&mac_config.ram_crit, "mac_config.ram", VTSS_MODULE_ID_MAC, CRITD_TYPE_MUTEX);

        MAC_RAM_CRIT_ENTER();
        MAC_CRIT_ENTER();

        /* The request buffer pool consists of the greater of VTSS_ISID_CNT or
           four buffers because of the four requests sent upon an
           INIT_CMD_ICFG_LOADING_POST event, and because of the heavy use of
           request buffers to all possible switches whenever a port has
           link-down or a switch leaves the stack
         */

        // Avoid "Warning -- Constant value Boolean" Lint warning, due to the use of MSG_TX_DATA_HDR_LEN_MAX() below
        /*lint -e{506} */
        mac_config.request = msg_buf_pool_create(VTSS_MODULE_ID_MAC, "Request", VTSS_ISID_CNT > 4 ? VTSS_ISID_CNT : 4, sizeof(mac_msg_req_t));
        mac_config.reply   = msg_buf_pool_create(VTSS_MODULE_ID_MAC, "Reply",   1, sizeof(mac_msg_rep_t));

#ifdef VTSS_SW_OPTION_ICFG
        if (mac_icfg_init() != VTSS_RC_OK) {
            T_D("Calling mac_icfg_init() failed");
        }
#endif

        vtss_flag_init(&age_flags);
        vtss_flag_init(&getnext_flags);
        vtss_flag_init(&stats_flags);
        vtss_flag_init(&mac_table_flags);
        MAC_CRIT_EXIT();
        MAC_RAM_CRIT_EXIT();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        mac_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_mac_json_init();
#endif

        mac_icli_cmd_register();

        /* Create aging thread */
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           mac_thread,
                           0,
                           "Mac Age Control",
                           nullptr,
                           0,
                           &mac_thread_handle,
                           &mac_thread_block);
        break;

    case INIT_CMD_START:
        T_D("INIT_CMD_START");
        mac_start();
        break;

    case INIT_CMD_CONF_DEF:
        T_D("INIT_CMD_CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_GLOBAL) {
            mac_conf_default();
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_I("ICFG_LOADING_PRE");
        flush_mac_list(TRUE);
        build_mac_list();
        mac_conf_default();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        /* Configure the Age time */
        (void)mac_age_stack_conf_set(isid);
        /* Add static mac addresses */
        mac_static_conf_set(isid);
        /* Set the learn mode */
        mac_learnmode_conf_set(isid);
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

