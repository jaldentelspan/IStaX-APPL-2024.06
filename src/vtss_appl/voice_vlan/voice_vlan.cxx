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
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "vtss/appl/voice_vlan.h"
#include "voice_vlan_api.h"
#include "voice_vlan.h"
#include "vlan_api.h"
#include "mac_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#if defined(VOICE_VLAN_CLASS_SUPPORTED)
#include "qos_api.h"
#endif /* VOICE_VLAN_CLASS_SUPPORTED */
#include "psec_api.h"
#if defined(VTSS_SW_OPTION_LLDP)
#include "lldp_api.h"
#include "lldp_remote.h"
#endif /* VTSS_SW_OPTION_LLDP */
#if defined(VTSS_SW_OPTION_MVR)
#include <vtss/appl/mvr.h>
#endif /* VTSS_SW_OPTION_MVR */

#ifdef VTSS_SW_OPTION_ICFG
#include "voice_vlan_icfg.h"
#endif

#define VOICE_VLAN_CONF_CHANGE_MODE             0x1
#define VOICE_VLAN_CONF_CHANGE_VID              0x2
#define VOICE_VLAN_CONF_CHANGE_AGE_TIME         0x4
#define VOICE_VLAN_CONF_CHANGE_TRAFFIC_CLASS    0x8

#define VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE       0x1
#define VOICE_VLAN_PORT_CONF_CHANGE_SECURITY        0x2
#define VOICE_VLAN_PORT_CONF_CHANGE_PROTOCOL        0x4

/* Set VOICE_VLAN reserved QCE */
#define VOICE_VLAN_RESERVED_QCE_ID  1

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_VOICE_VLAN

/****************************************************************************/
/*  Private global variables                                                */
/****************************************************************************/

/* Private global structure */
static voice_vlan_global_t VOICE_VLAN_global;

static vtss_trace_reg_t VOICE_VLAN_trace_reg = {
    VTSS_TRACE_MODULE_ID, "voice_vlan", "VOICE_VLAN"
};

static vtss_trace_grp_t VOICE_VLAN_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&VOICE_VLAN_trace_reg, VOICE_VLAN_trace_grps);

#define VOICE_VLAN_CRIT_ENTER() critd_enter(&VOICE_VLAN_global.crit, __FILE__, __LINE__)
#define VOICE_VLAN_CRIT_EXIT()  critd_exit( &VOICE_VLAN_global.crit, __FILE__, __LINE__)

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/
static mesa_rc VOICE_VLAN_mgmt_oui_conf_set(voice_vlan_oui_conf_t *conf, BOOL single_oui_chg);
static mesa_rc VOICE_VLAN_join(vtss_isid_t isid, mesa_port_no_t iport, mesa_vid_t vid);
static mesa_rc VOICE_VLAN_disjoin(vtss_isid_t isid, mesa_port_no_t iport, mesa_vid_t vid);

/****************************************************************************/
// Voice VLAN LLDP telephony MAC entry functions
/****************************************************************************/

/* Get VOICE_VLAN LLDP telephony MAC entry
 * The entry key is MAC address.
 * Use null MAC address to get first entry. */
mesa_rc voice_vlan_lldp_telephony_mac_entry_get(voice_vlan_lldp_telephony_mac_entry_t *entry, BOOL next)
{
    u32 i, num, found = 0;
    u8  null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    T_D("enter");

    if (entry == NULL) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    VOICE_VLAN_CRIT_ENTER();

    /* Lookup this entry */
    for (i = 0, num = 0;
         i < fast_cap(VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT) && num < VOICE_VLAN_global.lldp_telephony_mac.entry_num;
         i++) {
        if (!VOICE_VLAN_global.lldp_telephony_mac.entry[i].valid) {
            continue;
        }
        num++;

        /* Get first entry */
        if (memcmp(null_mac, entry->mac, 6) == 0 && next) {
            *entry = VOICE_VLAN_global.lldp_telephony_mac.entry[i];
            found = 1;
            break;
        }

        /* Lookup this entry */
        if (!memcmp(VOICE_VLAN_global.lldp_telephony_mac.entry[i].mac, entry->mac, 6)) {
            if (next) { /* Get next entry */
                if (num == VOICE_VLAN_global.lldp_telephony_mac.entry_num) {
                    break;
                }
                i++;
                while (i < fast_cap(VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT)) {
                    if (VOICE_VLAN_global.lldp_telephony_mac.entry[i].valid) {
                        *entry = VOICE_VLAN_global.lldp_telephony_mac.entry[i];
                        found = 1;
                        break;
                    }
                    i++;
                }
                break;
            } else { /* Get this entry */
                *entry = VOICE_VLAN_global.lldp_telephony_mac.entry[i];
                found = 1;
            }
            break;
        }
    }

    VOICE_VLAN_CRIT_EXIT();
    T_D("exit");

    if (found) {
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

/* Get VOICE_VLAN LLDP telephony MAC entry
 * fill null oui_address will get first entry */
static mesa_rc VOICE_VLAN_lldp_telephony_mac_entry_get_by_port(vtss_isid_t isid, mesa_port_no_t iport, voice_vlan_lldp_telephony_mac_entry_t *entry, BOOL next)
{
    u32 i, num, found = 0;
    u8  null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    T_D("enter");

    /* Lookup this entry */
    for (i = 0, num = 0;
         i < fast_cap(VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT) && num < VOICE_VLAN_global.lldp_telephony_mac.entry_num;
         i++) {
        if (!VOICE_VLAN_global.lldp_telephony_mac.entry[i].valid) {
            continue;
        }
        num++;

        if (VOICE_VLAN_global.lldp_telephony_mac.entry[i].isid != isid ||
            VOICE_VLAN_global.lldp_telephony_mac.entry[i].port_no != iport) {
            continue;
        }

        /* Get first entry */
        if (memcmp(null_mac, entry->mac, 6) == 0 && next) {
            *entry = VOICE_VLAN_global.lldp_telephony_mac.entry[i];
            found = 1;
            break;
        }

        /* Lookup this entry */
        if (!memcmp(VOICE_VLAN_global.lldp_telephony_mac.entry[i].mac, entry->mac, 6)) {
            if (next) { /* Get next entry */
                if (num == VOICE_VLAN_global.lldp_telephony_mac.entry_num) {
                    break;
                }
                i++;
                while (i < fast_cap(VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT)) {
                    if (VOICE_VLAN_global.lldp_telephony_mac.entry[i].valid &&
                        VOICE_VLAN_global.lldp_telephony_mac.entry[i].isid == isid &&
                        VOICE_VLAN_global.lldp_telephony_mac.entry[i].port_no == iport) {
                        *entry = VOICE_VLAN_global.lldp_telephony_mac.entry[i];
                        found = 1;
                        break;
                    }
                    i++;
                }
                break;
            } else { /* Get this entry */
                *entry = VOICE_VLAN_global.lldp_telephony_mac.entry[i];
                found = 1;
            }
            break;
        }
    }

    T_D("exit");

    if (found) {
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

#if defined(VTSS_SW_OPTION_LLDP)
/* Add/Set VOICE_VLAN LLDP telephony MAC entry */
static mesa_rc VOICE_VLAN_lldp_telephony_mac_entry_add(voice_vlan_lldp_telephony_mac_entry_t *entry)
{
    mesa_rc rc = VTSS_RC_ERROR;
    u32     i, num, found = 0;
    u8      null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    T_D("enter");
    if (memcmp(null_mac, entry->mac, 6) == 0) {
        return VTSS_RC_ERROR;
    }

    /* Lookup this entry */
    for (i = 0, num = 0;
         i < fast_cap(VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT) && num < VOICE_VLAN_global.lldp_telephony_mac.entry_num;
         i++) {
        if (!VOICE_VLAN_global.lldp_telephony_mac.entry[i].valid) {
            continue;
        }
        num++;

        if (!memcmp(VOICE_VLAN_global.lldp_telephony_mac.entry[i].mac, entry->mac, 6)) {
            found = 1;
            break;
        }
    }

    if (found) {
        VOICE_VLAN_global.lldp_telephony_mac.entry[i] = *entry;
        rc = VTSS_RC_OK;
    } else {
        /* Lookup a empty entry for using */
        for (i = 0; i < fast_cap(VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT); i++) {
            if (!VOICE_VLAN_global.lldp_telephony_mac.entry[i].valid) {
                found = 1;
                break;
            }
        }

        if (found) { /* Fill this entry */
            VOICE_VLAN_global.lldp_telephony_mac.entry[i] = *entry;
            VOICE_VLAN_global.lldp_telephony_mac.entry_num++;
            rc = VTSS_RC_OK;
        }
    }

    T_D("exit");

    return rc;
}

/* Delete VOICE_VLAN LLDP telephony MAC entry */
static mesa_rc VOICE_VLAN_lldp_telephony_mac_entry_del(voice_vlan_lldp_telephony_mac_entry_t *entry)
{
    u32     i, num;
    u8      null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    T_D("enter");
    if (memcmp(null_mac, entry->mac, 6) == 0) {
        T_D("exit");
        return VTSS_RC_ERROR;
    }

    /* Lookup this entry */
    for (i = 0, num = 0;
         i < fast_cap(VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT) && num < VOICE_VLAN_global.lldp_telephony_mac.entry_num;
         i++) {
        if (!VOICE_VLAN_global.lldp_telephony_mac.entry[i].valid) {
            continue;
        }
        num++;

        if (!memcmp(VOICE_VLAN_global.lldp_telephony_mac.entry[i].mac, entry->mac, 6)) {
            if (VOICE_VLAN_global.lldp_telephony_mac.entry_num > 0) {
                VOICE_VLAN_global.lldp_telephony_mac.entry_num--;
            }
            memset(&VOICE_VLAN_global.lldp_telephony_mac.entry[i], 0x0, sizeof(VOICE_VLAN_global.lldp_telephony_mac.entry[i]));
            break;
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_LLDP */

/* Clear VOICE_VLAN LLDP telephony MAC entry by port */
static void VOICE_VLAN_lldp_telephony_mac_entry_clear_by_port(vtss_isid_t isid, mesa_port_no_t iport)
{
    u32 i, num;

    T_D("enter");

    /* Lookup this entry */
    for (i = 0, num = 0;
         i < fast_cap(VTSS_APPL_CAP_VOICE_VLAN_LLDP_TELEPHONY_MAC_ENTRIES_CNT) && num < VOICE_VLAN_global.lldp_telephony_mac.entry_num;
         i++) {
        if (!VOICE_VLAN_global.lldp_telephony_mac.entry[i].valid) {
            continue;
        }
        num++;

        if (VOICE_VLAN_global.lldp_telephony_mac.entry[i].isid != isid ||
            VOICE_VLAN_global.lldp_telephony_mac.entry[i].port_no != iport) {
            continue;
        }
        if (VOICE_VLAN_global.lldp_telephony_mac.entry_num > 0) {
            VOICE_VLAN_global.lldp_telephony_mac.entry_num--;
        }
        memset(&VOICE_VLAN_global.lldp_telephony_mac.entry[i], 0x0, sizeof(VOICE_VLAN_global.lldp_telephony_mac.entry[i]));
    }

    T_D("exit");
}

/* Clear VOICE_VLAN LLDP telephony MAC entry */
static void VOICE_VLAN_lldp_telephony_mac_entry_clear(void)
{
    vtss_clear(VOICE_VLAN_global.lldp_telephony_mac);
}

static BOOL VOICE_VLAN_is_lldp_telephony_addr(vtss_isid_t isid, mesa_port_no_t iport, u8 addr[6])
{
    voice_vlan_lldp_telephony_mac_entry_t entry;

    memset(&entry, 0x0, sizeof(entry));
    while (VOICE_VLAN_lldp_telephony_mac_entry_get_by_port(isid, iport, &entry, TRUE) == VTSS_RC_OK) {
        if (!memcmp(entry.mac, addr, 6)) {
            return TRUE;
        }
    };

    return FALSE;
}

static BOOL VOICE_VLAN_exist_telephony_lldp_device(vtss_isid_t isid, mesa_port_no_t iport)
{
    voice_vlan_lldp_telephony_mac_entry_t entry;

    memset(&entry, 0x0, sizeof(entry));
    while (VOICE_VLAN_lldp_telephony_mac_entry_get_by_port(isid, iport, &entry, TRUE) == VTSS_RC_OK) {
        return TRUE;
    };
    return FALSE;
}


/****************************************************************************/
// VOICE_VLAN LLDP OUI entry functions
/****************************************************************************/

/* Get VOICE_VLAN OUI entry
 * The entry key is OUI address.
 * Use zero OUI address to get first entry. */
mesa_rc voice_vlan_oui_entry_get(voice_vlan_oui_entry_t *entry, BOOL next)
{
    u8                      i;
    voice_vlan_oui_entry_t  *chk, *nxt;

    T_D("enter(%s(%s)->%X-%X-%X)",
        next ? "NXT" : "GET",
        entry ? "PTR" : "NIL",
        entry ? entry->oui_addr[0] : 0,
        entry ? entry->oui_addr[1] : 0,
        entry ? entry->oui_addr[2] : 0);

    if (entry == NULL) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    nxt = NULL;
    VOICE_VLAN_CRIT_ENTER();
    for (i = 0; i < VOICE_VLAN_OUI_ENTRIES_CNT; ++i) {
        chk = &VOICE_VLAN_global.oui_conf.entry[i];
        if (!chk->valid) {
            continue;
        }

        if (next) {
            if (memcmp(chk->oui_addr, entry->oui_addr, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN) > 0) {
                if (!nxt) {
                    nxt = chk;
                    T_D("Assign: %X-%X-%X", nxt->oui_addr[0], nxt->oui_addr[1], nxt->oui_addr[2]);
                } else {
                    if (memcmp(chk->oui_addr, nxt->oui_addr, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN) < 0) {
                        nxt = chk;
                        T_D("Replace: %X-%X-%X", nxt->oui_addr[0], nxt->oui_addr[1], nxt->oui_addr[2]);
                    }
                }
            }
        } else {
            if (!memcmp(chk->oui_addr, entry->oui_addr, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN)) {
                nxt = chk;
                break;
            }
        }
    }
    if (nxt) {
        *entry = *nxt;
    }
    VOICE_VLAN_CRIT_EXIT();

    T_D("exit(%s->%X-%X-%X)",
        nxt ? "OK" : "NG",
        entry->oui_addr[0],
        entry->oui_addr[1],
        entry->oui_addr[2]);

    return nxt ? VTSS_RC_OK : VTSS_RC_ERROR;
}

static void VOICE_VLAN_psec_mac_update_by_single_oui_chg(BOOL is_add, voice_vlan_oui_entry_t *entry)
{
    mesa_rc         rc;
    switch_iter_t   sit;
    port_iter_t     pit;
    u32             port_oui_cnt;
    mesa_vid_t      voice_vlan_vid;
    int             discovery_protocol;
    BOOL            sw_learn, security_mode;

    T_D("enter");

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            VOICE_VLAN_CRIT_ENTER();
            port_oui_cnt        = VOICE_VLAN_global.oui_cnt[sit.isid][pit.iport];
            voice_vlan_vid      = VOICE_VLAN_global.conf.vid;
            discovery_protocol  = VOICE_VLAN_global.port_conf[sit.isid].discovery_protocol[pit.iport];
            sw_learn            = VOICE_VLAN_global.sw_learn[sit.isid][pit.iport];
            security_mode       = VOICE_VLAN_global.port_conf[sit.isid].security[pit.iport] == VOICE_VLAN_MGMT_ENABLED ? TRUE : FALSE;
            VOICE_VLAN_CRIT_EXIT();

            if (sw_learn) {
                vtss_appl_psec_interface_status_t   port_status;
                vtss_appl_psec_mac_status_t         mac_status;
                BOOL                                first;
                vtss_ifindex_t                      prev_ifindex, next_ifindex;
                mesa_vid_t                          prev_vid, next_vid;
                mesa_mac_t                          prev_mac, next_mac;

                if (vtss_ifindex_from_port(sit.isid, pit.iport, &prev_ifindex) != VTSS_RC_OK || vtss_appl_psec_interface_status_get(prev_ifindex, &port_status) != VTSS_RC_OK) {
                    continue;
                }

                first = TRUE;
                while (vtss_appl_psec_interface_status_mac_itr(&prev_ifindex, &next_ifindex, first ? NULL : &prev_vid, &next_vid, first ? NULL : &prev_mac, &next_mac) == VTSS_RC_OK) {
                    BOOL is_oui_addr, mac_forwarding;

                    if (next_ifindex != prev_ifindex) {
                        // Iterating into next port. Done.
                        break;
                    }

                    first = FALSE;
                    prev_vid = next_vid;
                    prev_mac = next_mac;

                    if (vtss_appl_psec_interface_status_mac_get(next_ifindex, next_vid, next_mac, &mac_status) != VTSS_RC_OK) {
                        // Someone deleted the MAC address in between the iterator and now. That's not an error.
                        break;
                    }

                    is_oui_addr = memcmp(mac_status.vid_mac.mac.addr, entry->oui_addr, sizeof(entry->oui_addr)) == 0 ? TRUE : FALSE;
                    mac_forwarding = (mac_status.blocked || mac_status.kept_blocked) ? FALSE : TRUE;

                    // When Voice OUI discovery protocol is enabled, we need to change the VOICE_VLAN memberset
                    // 1. Join to VOICE_VLAN when port_oui_cnt == 0
                    // 2. Disjoin from VOICE_VLAN when port_oui_cnt == 1
                    if (is_oui_addr && discovery_protocol != VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP /* OUI or BOTH */) {
                        // Check if need to join to VOICE_VLAN when OUI adding
                        if (is_add) { // no matter 'forwarding or blocking state'
                            /* Join to VOICE_VLAN when matched the new OUI address and the port is not in the memberset */
                            if (port_oui_cnt == 0 &&
                                (rc = VOICE_VLAN_join(sit.isid, pit.iport, voice_vlan_vid)) != VTSS_RC_OK) {
                                T_W("VOICE_VLAN_join(%u, %d, %d): failed rc = %d", sit.isid, pit.iport, voice_vlan_vid, rc);
                            }

                            port_oui_cnt++;
                            VOICE_VLAN_CRIT_ENTER();
                            VOICE_VLAN_global.oui_cnt[sit.isid][pit.iport]++;
                            VOICE_VLAN_CRIT_EXIT();

                            // Check if need to disjoin from VOICE_VLAN when OUI deleting
                        } else if (!is_add && mac_forwarding && port_oui_cnt > 0) {
                            port_oui_cnt--;
                            VOICE_VLAN_CRIT_ENTER();
                            VOICE_VLAN_global.oui_cnt[sit.isid][pit.iport]--;
                            VOICE_VLAN_CRIT_EXIT();

                            /* Disjoin from VOICE_VLAN when last OUI address deleting */
                            if (port_oui_cnt == 0 &&
                                (discovery_protocol == VOICE_VLAN_DISCOVERY_PROTOCOL_OUI ||
                                 (discovery_protocol == VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH && !VOICE_VLAN_exist_telephony_lldp_device(sit.isid, pit.iport)))) {
                                if ((rc = VOICE_VLAN_disjoin(sit.isid, pit.iport, voice_vlan_vid)) != VTSS_RC_OK) {
                                    T_W("VOICE_VLAN_disjoin(%u, %d, %d): failed rc = %d", sit.isid, pit.iport, voice_vlan_vid, rc);
                                }
                            }
                        }
                    }

                    // When Voice VLAN port security mode is enabled, we need to change the specific psec mac state
                    // 1. Change 'forwarding' to 'blocked' state when OUI deleting
                    // 2. Change 'blocked' to 'forwarding' state when OUI adding
                    if (security_mode &&
                        mac_status.vid_mac.vid == voice_vlan_vid && is_oui_addr &&
                        ((is_add && !mac_forwarding) || (!is_add && mac_forwarding))) {
                        psec_add_method_t new_method = (is_add && !mac_forwarding) ? PSEC_ADD_METHOD_FORWARD : PSEC_ADD_METHOD_BLOCK;
                        mesa_vid_mac_t new_vid_mac;

                        new_vid_mac.vid = mac_status.vid_mac.vid;
                        memcpy(new_vid_mac.mac.addr, mac_status.vid_mac.mac.addr, sizeof(mac_status.vid_mac.mac.addr));

                        if (psec_mgmt_mac_chg(VTSS_APPL_PSEC_USER_VOICE_VLAN, sit.isid, pit.iport, &new_vid_mac, new_method) == VTSS_RC_OK) {
                            T_D("Change OUI MAC address %2X-%2X-%2X-%2X-%2X-%2X to %s mode", new_vid_mac.mac.addr[0], new_vid_mac.mac.addr[1], new_vid_mac.mac.addr[2], new_vid_mac.mac.addr[3], new_vid_mac.mac.addr[4], new_vid_mac.mac.addr[5], new_method == PSEC_ADD_METHOD_FORWARD ? "forwarding" : "blocked");
                        }
                    }
                }
            }
        }
    }

    T_D("exit");
}

/* Add/Set VOICE_VLAN OUI entry */
mesa_rc voice_vlan_oui_entry_add(voice_vlan_oui_entry_t *entry)
{
    mesa_rc                 rc;
    u32                     i, num;
    BOOL                    existing = FALSE, found_empty = FALSE;
    voice_vlan_oui_conf_t   conf;
    u8                      null_oui_addr[3] = {0x0, 0x0, 0x0};

    T_D("enter");
    if (entry == NULL) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    if (memcmp(null_oui_addr, entry->oui_addr, 3) == 0) {
        T_D("exit");
        return VOICE_VLAN_ERROR_PARM_NULL_OUI_ADDR;
    }

    VOICE_VLAN_CRIT_ENTER();

    /* Lookup this entry */
    for (i = 0, num = 0;
         i < VOICE_VLAN_OUI_ENTRIES_CNT && num < VOICE_VLAN_global.oui_conf.entry_num;
         i++) {
        if (!VOICE_VLAN_global.oui_conf.entry[i].valid) {
            continue;
        }
        num++;

        if (!memcmp(VOICE_VLAN_global.oui_conf.entry[i].oui_addr, entry->oui_addr, 3)) {
            existing = TRUE;
            break;
        }
    }

    if (existing && i < VOICE_VLAN_OUI_ENTRIES_CNT) {
        conf = VOICE_VLAN_global.oui_conf;
        conf.entry[i] = *entry;
        conf.entry[i].valid = TRUE;
    } else {
        /* Lookup a empty entry for using */
        for (i = 0; i < VOICE_VLAN_OUI_ENTRIES_CNT; i++) {
            if (!VOICE_VLAN_global.oui_conf.entry[i].valid) {
                found_empty = TRUE;
                break;
            }
        }

        if (found_empty && i < VOICE_VLAN_OUI_ENTRIES_CNT) { /* Fill this entry */
            conf = VOICE_VLAN_global.oui_conf;
            conf.entry_num++;
            conf.entry[i] = *entry;
            conf.entry[i].valid = TRUE;
        } else {
            T_D("exit: out of max. count");
            VOICE_VLAN_CRIT_EXIT();
            return VOICE_VLAN_ERROR_REACH_MAX_OUI_ENTRY;
        }
    }

    VOICE_VLAN_CRIT_EXIT();

    if (!existing) { // new OUI entry
        VOICE_VLAN_psec_mac_update_by_single_oui_chg(TRUE, entry);
    }

    rc = VOICE_VLAN_mgmt_oui_conf_set(&conf, TRUE);

    T_D("exit");
    return rc;
}

/* Delete VOICE_VLAN OUI entry */
mesa_rc voice_vlan_oui_entry_del(voice_vlan_oui_entry_t *entry)
{
    mesa_rc                 rc = VOICE_VLAN_ERROR_ENTRY_NOT_EXIST;
    u32                     i, num, found = 0;
    voice_vlan_oui_conf_t   conf;

    T_D("enter");

    if (entry == NULL) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    VOICE_VLAN_CRIT_ENTER();

    /* Lookup this entry */
    for (i = 0, num = 0;
         i < VOICE_VLAN_OUI_ENTRIES_CNT && num < VOICE_VLAN_global.oui_conf.entry_num;
         i++) {
        if (!VOICE_VLAN_global.oui_conf.entry[i].valid) {
            continue;
        }
        num++;

        if (!memcmp(VOICE_VLAN_global.oui_conf.entry[i].oui_addr, entry->oui_addr, 3)) {
            found = 1;
            break;
        }
    }

    VOICE_VLAN_CRIT_EXIT();

    if (found && i < VOICE_VLAN_OUI_ENTRIES_CNT) { /* Delete this entry */
        VOICE_VLAN_psec_mac_update_by_single_oui_chg(FALSE, entry);

        VOICE_VLAN_CRIT_ENTER();
        conf = VOICE_VLAN_global.oui_conf;
        VOICE_VLAN_CRIT_EXIT();

        if (conf.entry_num > 0) {
            conf.entry_num--;
        }
        memset(&conf.entry[i], 0x0, sizeof(conf.entry[i]));
        rc = VOICE_VLAN_mgmt_oui_conf_set(&conf, TRUE);
    }

    T_D("exit");
    return rc;
}

/* Clear VOICE_VLAN OUI entry */
mesa_rc voice_vlan_oui_entry_clear(void)
{
    voice_vlan_oui_conf_t   conf;
    mesa_rc                 rc;

    T_D("enter");
    memset(&conf, 0x0, sizeof(conf));
    rc = VOICE_VLAN_mgmt_oui_conf_set(&conf, FALSE);
    T_D("exit");
    return rc;
}

static BOOL VOICE_VLAN_is_oui_addr(const u8 addr[6])
{
    voice_vlan_oui_entry_t  entry;

    memset(&entry, 0x0, sizeof(entry));
    while (voice_vlan_oui_entry_get(&entry, TRUE) == VTSS_RC_OK) {
        if (entry.oui_addr[0] == addr[0] && entry.oui_addr[1] == addr[1] && entry.oui_addr[2] == addr[2]) {
            return TRUE;
        }
    };

    return FALSE;
}


/****************************************************************************/
// Voice VLAN QCE functions
/****************************************************************************/
// Avoid "Custodual pointer 'msg' has not been freed or returned, since
// the msg is freed by the message module.
/*lint -e{429} */
static void VOICE_VLAN_local_qce_set(vtss_isid_t isid, voice_vlan_msg_req_t *msg_reg)
{
    size_t                  msg_len = sizeof(voice_vlan_msg_req_t);
    voice_vlan_msg_req_t    *msg;

    VTSS_MALLOC_CAST(msg, msg_len);
    if (msg) {
        *msg = *msg_reg;
        msg->msg_id = VOICE_VLAN_MSG_ID_QCE_SET_REQ;
        if (!msg_reg->req.qce_conf_set.change_traffic_class) {
            msg->req.qce_conf_set.traffic_class = VOICE_VLAN_global.conf.traffic_class;
        }
        if (!msg_reg->req.qce_conf_set.change_vid) {
            msg->req.qce_conf_set.vid = VOICE_VLAN_global.conf.vid;
        }
        msg_tx(VTSS_MODULE_ID_VOICE_VLAN, isid, msg, msg_len);
    }
}

/* Create a reserved QCE for Voice VLAN */
static void VOICE_VLAN_reserved_qce_del(void)
{
    voice_vlan_msg_req_t    msg_reg;
    vtss_isid_t             isid;

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (!msg_switch_exists(isid)) {
            continue;
        }
        memset(&msg_reg, 0x0, sizeof(msg_reg));
        msg_reg.req.qce_conf_set.del_qce = TRUE;
        VOICE_VLAN_local_qce_set(isid, &msg_reg);
    }
}


/****************************************************************************/
// Voice VLAN functions
/****************************************************************************/

static mesa_rc VOICE_VLAN_join(vtss_isid_t isid, mesa_port_no_t iport, mesa_vid_t vid)
{
    mesa_rc                rc = VTSS_RC_OK;
    vtss_appl_vlan_entry_t vlan_member;
    voice_vlan_msg_req_t   msg_reg;

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    /* Add to VOICE_VLAN */
    if (vtss_appl_vlan_get(isid, vid, &vlan_member, FALSE, VTSS_APPL_VLAN_USER_VOICE_VLAN) != VTSS_RC_OK) {
        vtss_clear(vlan_member);
        vlan_member.vid = vid;
    }
    if (vlan_member.ports[iport] == 0) {
        vlan_member.ports[iport] = 1;
        rc = vlan_mgmt_vlan_add(isid, &vlan_member, VTSS_APPL_VLAN_USER_VOICE_VLAN);
    }

    /* Set port default QCL to VOICE_VLAN reserved QCL */
    memset(&msg_reg, 0x0, sizeof(msg_reg));
    msg_reg.req.qce_conf_set.is_port_add = TRUE;
    msg_reg.req.qce_conf_set.iport = iport;
    VOICE_VLAN_local_qce_set(isid, &msg_reg);

    return rc;
}

static mesa_rc VOICE_VLAN_disjoin(vtss_isid_t isid, mesa_port_no_t iport, mesa_vid_t vid)
{
    mesa_rc                rc = VTSS_RC_OK;
    vtss_appl_vlan_entry_t vlan_member;
    voice_vlan_msg_req_t   msg_reg;

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    /* Remove from VOICE_VLAN */
    if (vtss_appl_vlan_get(isid, vid, &vlan_member, FALSE, VTSS_APPL_VLAN_USER_VOICE_VLAN) != VTSS_RC_OK) {
        vtss_clear(vlan_member);
        vlan_member.vid = vid;
    }
    if (vlan_member.ports[iport] == 1) {
        vlan_member.ports[iport] = 0;
        rc = vlan_mgmt_vlan_add(isid, &vlan_member, VTSS_APPL_VLAN_USER_VOICE_VLAN);
    }

    /* Restore port default QCL */
    memset(&msg_reg, 0x0, sizeof(msg_reg));
    msg_reg.req.qce_conf_set.is_port_del = TRUE;
    msg_reg.req.qce_conf_set.iport = iport;
    VOICE_VLAN_local_qce_set(isid, &msg_reg);

    return rc;
}


/****************************************************************************/
/* Callback functions                                                       */
/****************************************************************************/

/* Process port change status */
static void VOICE_VLAN_porcess_port_change_cb(vtss_isid_t isid, mesa_port_no_t iport, BOOL link)
{
    mesa_rc rc;

    /* do nothing when port link-on, since everything should be no problem at this moment */
    if (!link) {
        VOICE_VLAN_CRIT_ENTER();
        /* Disjoin from VOice VLAN when port link-down */
        if (VOICE_VLAN_global.conf.mode == VOICE_VLAN_MGMT_ENABLED &&
            VOICE_VLAN_global.port_conf[isid].port_mode[iport] == VOICE_VLAN_PORT_MODE_AUTO) {
            if ((rc = VOICE_VLAN_disjoin(isid, iport, VOICE_VLAN_global.conf.vid)) != VTSS_RC_OK) {
                T_W("VOICE_VLAN_disjoin(%u, %d, %d): failed rc = %d", isid, iport, VOICE_VLAN_global.conf.vid, rc);
            }
        }

        /* Clear LLDP telephony MAC entries on this port */
        VOICE_VLAN_lldp_telephony_mac_entry_clear_by_port(isid, iport);

        /* Clear OUI counter on this port */
        memset(&VOICE_VLAN_global.oui_cnt[isid][iport], 0x0, sizeof(VOICE_VLAN_global.oui_cnt[isid][iport]));
        VOICE_VLAN_CRIT_EXIT();
    }
}

/* Port change link status callback function */
static void VOICE_VLAN_port_change_cb(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_isid_t isid = VTSS_ISID_START;

    if (msg_switch_is_primary()) {
        T_D("port_no: [%d,%u] link %s", isid, port_no, status->link ? "up" : "down");
        if (!msg_switch_exists(isid)) {
            return;
        }
        VOICE_VLAN_porcess_port_change_cb(isid, port_no, status->link);
    }
}

#if defined(VTSS_SW_OPTION_LLDP)
/* Process LLDP entry change */
static void VOICE_VLAN_porcess_lldp_cb(vtss_isid_t isid, mesa_port_no_t iport, vtss_appl_lldp_remote_entry_t *lldp_entry)
{
    mesa_rc                                 rc;
    int                                     del_flag = 0, port_entry_cnt = 0;
    voice_vlan_lldp_telephony_mac_entry_t   lldp_telephony_mac_entry;

    T_D("enter, isid: %d, iport: %d, LLDP entry: %s, telphone capability: %s, capability:%s",
        isid,
        iport,
        lldp_entry->in_use ? "Del" : "Add/Change",
        (lldp_entry->system_capabilities[1] & 0x20) ? "Yes" : "No",
        (lldp_entry->system_capabilities[3] & 0x20) ? "Enabled" : "Disabled");

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return;
    }

    VOICE_VLAN_CRIT_ENTER();

    if (VOICE_VLAN_global.conf.mode == VOICE_VLAN_MGMT_ENABLED &&
        VOICE_VLAN_global.port_conf[isid].port_mode[iport] == VOICE_VLAN_PORT_MODE_AUTO &&
        VOICE_VLAN_global.port_conf[isid].discovery_protocol[iport] != VOICE_VLAN_DISCOVERY_PROTOCOL_OUI) {
        lldp_telephony_mac_entry.isid = isid;
        lldp_telephony_mac_entry.port_no = iport;
        memcpy(lldp_telephony_mac_entry.mac, lldp_entry->smac, 6);
        if (lldp_entry->in_use &&
            (lldp_entry->system_capabilities[1] & 0x20) &&
            (lldp_entry->system_capabilities[3] & 0x20)) { //bit5 is telephone capability (has telephone capability and is enabled)

            /* Add LLDP telephony MAC entry */
            T_D("Add LLDP telephony MAC entry");
            lldp_telephony_mac_entry.valid = 1;
            if ((rc = VOICE_VLAN_lldp_telephony_mac_entry_add(&lldp_telephony_mac_entry)) != VTSS_RC_OK) {
                T_W("VOICE_VLAN_lldp_telephony_mac_entry_add(): failed rc = %d", rc);
            }

            /* Join to VOICE_VLAN when LLDP device provide telephone capability */
            if ((rc = VOICE_VLAN_join(isid, iport, VOICE_VLAN_global.conf.vid)) != VTSS_RC_OK) {
                T_W("VOICE_VLAN_join(%u, %d, %d): failed rc = %d", isid, iport, VOICE_VLAN_global.conf.vid, rc);
            }
        } else {
            /* Delete LLDP telephony MAC entry */
            T_D("Delete LLDP telephony MAC entry");
            if (VOICE_VLAN_lldp_telephony_mac_entry_del(&lldp_telephony_mac_entry) != VTSS_RC_OK) {
                T_D("Calling VOICE_VLAN_lldp_telephony_mac_entry_del() failed\n");
            }
            del_flag = 1;
        }
    }

    if (del_flag) {
        memset(&lldp_telephony_mac_entry, 0x0, sizeof(lldp_telephony_mac_entry));
        while (VOICE_VLAN_lldp_telephony_mac_entry_get_by_port(isid, iport, &lldp_telephony_mac_entry, TRUE) == VTSS_RC_OK) {
            port_entry_cnt++;
        }

        if (port_entry_cnt == 0) {
            /* Remove from VOICE_VLAN when
               1. In LLDP mode, last telephony LLDP device leaving
               2. In both mode, disjoin from Voice VLAN if there's no any mac entry exist in Voice VLAN */
            if (VOICE_VLAN_global.port_conf[isid].discovery_protocol[iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP ||
                (VOICE_VLAN_global.port_conf[isid].discovery_protocol[iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH && VOICE_VLAN_global.oui_cnt[isid][iport] == 0)) {
                if ((rc = VOICE_VLAN_disjoin(isid, iport, VOICE_VLAN_global.conf.vid)) != VTSS_RC_OK) {
                    T_W("VOICE_VLAN_disjoin(%u, %d, %d): failed rc = %d", isid, iport, VOICE_VLAN_global.conf.vid, rc);
                }
            }
        }
    }

    VOICE_VLAN_CRIT_EXIT();
    T_D("exit");
}
#endif /* VTSS_SW_OPTION_LLDP */

/* LLDP entry change callback function */
// Avoid "Custodual pointer 'msg' has not been freed or returned, since
// the msg is freed by the message module.
/*lint -e{429} */
#if defined(VTSS_SW_OPTION_LLDP)
static void VOICE_VLAN_lldp_entry_cb(mesa_port_no_t iport, vtss_appl_lldp_remote_entry_t *entry)
{
    size_t msg_len = sizeof(voice_vlan_msg_req_t);
    voice_vlan_msg_req_t *msg = (voice_vlan_msg_req_t *)VTSS_MALLOC(msg_len);
    if (msg) {
        msg->msg_id = VOICE_VLAN_MSG_ID_LLDP_CB_IND;
        msg->req.lldp_cb_ind.port_no = iport;
        msg->req.lldp_cb_ind.entry = *entry;
        msg_tx(VTSS_MODULE_ID_VOICE_VLAN, 0, msg, msg_len);
    }
}
#endif /* VTSS_SW_OPTION_LLDP */

/* Port security module MAC add callback function */
static psec_add_method_t VOICE_VLAN_psec_mac_add_cb(vtss_isid_t isid, mesa_port_no_t iport, mesa_vid_mac_t *vid_mac, u32 mac_cnt_before_callback, vtss_appl_psec_user_t originating_user, psec_add_action_t *action)
{
    mesa_rc             rc;
    BOOL                is_oui_addr = 0;
    psec_add_method_t   psec_rc = PSEC_ADD_METHOD_FORWARD;

    is_oui_addr = VOICE_VLAN_is_oui_addr(vid_mac->mac.addr);

    VOICE_VLAN_CRIT_ENTER();

    if (is_oui_addr) {
        if (VOICE_VLAN_global.port_conf[isid].port_mode[iport] != VOICE_VLAN_PORT_MODE_AUTO ||
            VOICE_VLAN_global.port_conf[isid].discovery_protocol[iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP) {
            VOICE_VLAN_CRIT_EXIT();
            return PSEC_ADD_METHOD_FORWARD;
        }
        /* Join to VOICE_VLAN when incoming packet's source MAC address is OUI address */
        if (VOICE_VLAN_global.oui_cnt[isid][iport] == 0 && (rc = VOICE_VLAN_join(isid, iport, VOICE_VLAN_global.conf.vid)) != VTSS_RC_OK) {
            T_W("VOICE_VLAN_join(%u, %d, %d): failed rc = %d", isid, iport, VOICE_VLAN_global.conf.vid, rc);
        }

        VOICE_VLAN_global.oui_cnt[isid][iport]++;
    } else if (VOICE_VLAN_global.port_conf[isid].security[iport] == VOICE_VLAN_MGMT_ENABLED && vid_mac->vid == VOICE_VLAN_global.conf.vid) {
        if (!VOICE_VLAN_is_lldp_telephony_addr(isid, iport, vid_mac->mac.addr)) {
            /* Block non-OUI address in VOICE_VLAN */
            psec_rc = PSEC_ADD_METHOD_BLOCK;
        }
    }

    VOICE_VLAN_CRIT_EXIT();

    return psec_rc;
}

/* Port security module MAC delete callback function */
static void VOICE_VLAN_psec_mac_del_cb(vtss_isid_t isid, mesa_port_no_t iport, const mesa_vid_mac_t *vid_mac, psec_del_reason_t reason, psec_add_method_t add_method, vtss_appl_psec_user_t originating_user)
{
    mesa_rc rc;
    BOOL    is_oui_addr = 0;

    switch (reason) {
    case PSEC_DEL_REASON_PORT_LINK_DOWN:
        /* Do nothing, VOICE_VLAN_porcess_port_change_cb() will handle it */
        break;
    case PSEC_DEL_REASON_STATION_MOVED:
    case PSEC_DEL_REASON_AGED_OUT:
    case PSEC_DEL_REASON_USER_DELETED:
        is_oui_addr = VOICE_VLAN_is_oui_addr(vid_mac->mac.addr);
        VOICE_VLAN_CRIT_ENTER();
        if (VOICE_VLAN_global.port_conf[isid].security[iport] == VOICE_VLAN_MGMT_ENABLED &&
            VOICE_VLAN_global.oui_cnt[isid][iport] > 0 &&
            add_method == PSEC_ADD_METHOD_FORWARD &&
            is_oui_addr) {
            VOICE_VLAN_global.oui_cnt[isid][iport]--;

            /* Disjoin from VOICE_VLAN when last OUI address aged out */
            if (VOICE_VLAN_global.oui_cnt[isid][iport] == 0 &&
                (VOICE_VLAN_global.port_conf[isid].discovery_protocol[iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_OUI ||
                 (VOICE_VLAN_global.port_conf[isid].discovery_protocol[iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH && !VOICE_VLAN_exist_telephony_lldp_device(isid, iport)))) {
                if ((rc = VOICE_VLAN_disjoin(isid, iport, VOICE_VLAN_global.conf.vid)) != VTSS_RC_OK) {
                    T_W("VOICE_VLAN_disjoin(%u, %d, %d): failed rc = %d", isid, iport, VOICE_VLAN_global.conf.vid, rc);
                }
            }
        }
        VOICE_VLAN_CRIT_EXIT();
        break;
    case PSEC_DEL_REASON_PORT_SHUT_DOWN:
        /* Process the condition the same as link-down */
        VOICE_VLAN_porcess_port_change_cb(isid, iport, 0);
        break;
    default:
        return;
    }
}


/****************************************************************************/
// Voice VLAN configuration changed apply functions
/****************************************************************************/

#if defined(VTSS_SW_OPTION_LLDP)
static void VOICE_VLAN_update_lldp_info(vtss_isid_t isid, mesa_port_no_t port_no, voice_vlan_port_conf_t *new_conf)
{
    port_iter_t         pit;
    vtss_appl_lldp_remote_entry_t *lldp_table = NULL, *lldp_entry = NULL;
    int                 i, count = 0;
    BOOL                need_process = FALSE;
    vtss_appl_lldp_cap_t cap;
    if (vtss_appl_lldp_cap_get(&cap) != VTSS_RC_OK) {
        T_W("Calling vtss_appl_lldp_cap_get() failed");
    }

    if (isid != VTSS_ISID_START) {
        return;
    }
    /* Only process it when port mode is enabled and discovery protocol is LLDP/Both */
    if (port_no == VTSS_PORT_NO_NONE) {
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO &&
                new_conf->discovery_protocol[pit.iport] != VOICE_VLAN_DISCOVERY_PROTOCOL_OUI) {
                need_process = TRUE;
                break;
            }
        }
        if (!need_process) {
            return;
        }
    }

    vtss_appl_lldp_mutex_lock();

    count = cap.remote_entries_cnt;
    lldp_table = vtss_appl_lldp_entries_get(); // Get the LLDP entries for the switch in question.
    for (i = 0, lldp_entry = lldp_table; (lldp_entry && i < count); i++) {
        if (lldp_entry->in_use &&
            (lldp_entry->system_capabilities[1] & 0x20) &&
            (lldp_entry->system_capabilities[3] & 0x20) &&
            new_conf->port_mode[lldp_entry->receive_port] == VOICE_VLAN_PORT_MODE_AUTO &&
            new_conf->discovery_protocol[lldp_entry->receive_port] != VOICE_VLAN_DISCOVERY_PROTOCOL_OUI) {
            if (port_no == VTSS_PORT_NO_NONE) {
                VOICE_VLAN_porcess_lldp_cb(isid, lldp_entry->receive_port, lldp_entry);
            } else if (port_no == lldp_entry->receive_port) {
                VOICE_VLAN_porcess_lldp_cb(isid, lldp_entry->receive_port, lldp_entry);
                break;
            }
        }
        lldp_entry++;
    }

    vtss_appl_lldp_mutex_unlock();
}
#endif /* VTSS_SW_OPTION_LLDP */

/* Apply VOICE_VLAN configuration */
static mesa_rc VOICE_VLAN_conf_apply(BOOL init, vtss_isid_t isid)
{
    mesa_rc                    rc = VTSS_RC_OK;
    port_iter_t                pit;
    vtss_appl_vlan_entry_t     vlan_member;
    vtss_appl_vlan_port_conf_t vlan_port_conf;
    int                        vlan_member_cnt = 0;
    voice_vlan_msg_req_t       msg_reg;

    T_D("enter, isid: %d", isid);

    if (!msg_switch_is_primary()) {
        T_D("not primary switch");
        T_D("exit, isid: %d", isid);
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_exists(isid)) {
        T_D("isid: %d not exist", isid);
        T_D("exit, isid: %d", isid);
        return VOICE_VLAN_ERROR_ISID;
    }

    if (init) {
        /* Set age time */
        VOICE_VLAN_CRIT_ENTER();
        rc = psec_mgmt_time_conf_set(VTSS_APPL_PSEC_USER_VOICE_VLAN, VOICE_VLAN_global.conf.age_time, VOICE_VLAN_MGMT_DEFAULT_HOLD_TIME);
        VOICE_VLAN_CRIT_EXIT();
    }

    VOICE_VLAN_CRIT_ENTER();
    if (VOICE_VLAN_global.conf.mode == VOICE_VLAN_MGMT_ENABLED) {
        if (vtss_appl_vlan_get(isid, VOICE_VLAN_global.conf.vid, &vlan_member, FALSE, VTSS_APPL_VLAN_USER_VOICE_VLAN) != VTSS_RC_OK) {
            /* Create Voice VLAN */
            vtss_clear(vlan_member);
            vlan_member.vid = VOICE_VLAN_global.conf.vid;
            if ((rc = vlan_mgmt_vlan_add(isid, &vlan_member, VTSS_APPL_VLAN_USER_VOICE_VLAN)) != VTSS_RC_OK) {
                T_W("vlan_mgmt_vlan_add(%u): failed rc = %d", isid, rc);
            }
        }
    }
    VOICE_VLAN_CRIT_EXIT();

    if (rc != VTSS_RC_OK || VOICE_VLAN_global.conf.mode == VOICE_VLAN_MGMT_DISABLED) {
        T_D("exit, isid: %d", isid);
        return rc;
    }

    VOICE_VLAN_CRIT_ENTER();
    vtss_clear(vlan_member);
    vlan_member.vid = VOICE_VLAN_global.conf.vid;

    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
    while (port_iter_getnext(&pit)) {
        if (VOICE_VLAN_global.port_conf[isid].port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_DISABLED) {
            /* Un-override any parameter */
            memset(&vlan_port_conf, 0x0, sizeof(vlan_port_conf));
            if ((rc = vlan_mgmt_port_conf_set(isid, pit.iport, &vlan_port_conf, VTSS_APPL_VLAN_USER_VOICE_VLAN)) != VTSS_RC_OK) {
                T_E("vlan_mgmt_port_conf_set(%u, %d): failed rc = %s", isid, pit.iport, error_txt(rc));
            }
        } else {
            /* Disable ingress filter when port mode isn't disabled */
            if (vlan_mgmt_port_conf_get(isid, pit.iport, &vlan_port_conf, VTSS_APPL_VLAN_USER_VOICE_VLAN, TRUE) == VTSS_RC_OK) {
                vlan_port_conf.hybrid.ingress_filter = FALSE;
                vlan_port_conf.hybrid.port_type = VTSS_APPL_VLAN_PORT_TYPE_C;

                // Force port to tag Voice VLAN on egress
                vlan_port_conf.hybrid.tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS;
                vlan_port_conf.hybrid.untagged_vid = VOICE_VLAN_global.conf.vid;

                vlan_port_conf.hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT | VTSS_APPL_VLAN_PORT_FLAGS_AWARE | VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE;
                if ((rc = vlan_mgmt_port_conf_set(isid, pit.iport, &vlan_port_conf, VTSS_APPL_VLAN_USER_VOICE_VLAN)) != VTSS_RC_OK) {
                    T_E("vlan_mgmt_port_conf_set(%u, %d): failed rc = %s", isid, pit.iport, error_txt(rc));
                }
            }
        }

        /* Set VOICE_VLAN member */
        if (VOICE_VLAN_global.port_conf[isid].port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_FORCED) {
            vlan_member.ports[pit.iport] = 1;
            vlan_member_cnt++;
        }

        /* Enable software-based learning when:
         * 1. Voice VLAN and port security mode are enabled
         * 2. Support auto OUI discovery protocol */
        if (VOICE_VLAN_global.conf.mode == VOICE_VLAN_MGMT_ENABLED) {
            if ((VOICE_VLAN_global.port_conf[isid].port_mode[pit.iport] != VOICE_VLAN_PORT_MODE_DISABLED && VOICE_VLAN_global.port_conf[isid].security[pit.iport] == VOICE_VLAN_MGMT_ENABLED) ||
                (VOICE_VLAN_global.port_conf[isid].port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO && VOICE_VLAN_global.port_conf[isid].discovery_protocol[pit.iport] != VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP)) {
                if (VOICE_VLAN_global.sw_learn[isid][pit.iport] == 0) {
                    if ((rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_VOICE_VLAN, isid, pit.iport, TRUE, PSEC_PORT_MODE_NORMAL)) == VTSS_RC_OK) {
                        VOICE_VLAN_global.sw_learn[isid][pit.iport] = 1;
                    }
                }
            }
        } else if (VOICE_VLAN_global.sw_learn[isid][pit.iport]) {
            /* Disable software-based learning */
            if ((rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_VOICE_VLAN, isid, pit.iport, FALSE, PSEC_PORT_MODE_NORMAL)) == VTSS_RC_OK) {
                VOICE_VLAN_global.sw_learn[isid][pit.iport] = 0;
            }
        }
    }

    if (vlan_member_cnt) {
        rc = vlan_mgmt_vlan_add(isid, &vlan_member, VTSS_APPL_VLAN_USER_VOICE_VLAN);
    }

    memset(&msg_reg, 0x0, sizeof(msg_reg));
    msg_reg.req.qce_conf_set.is_port_list = TRUE;
    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        msg_reg.req.qce_conf_set.port_list[pit.iport] = vlan_member.ports[pit.iport];
    }
    VOICE_VLAN_local_qce_set(isid, &msg_reg);

    VOICE_VLAN_CRIT_EXIT();
    T_D("exit, isid: %d", isid);
    return rc;
}

/* Apply VOICE_VLAN configuration pre-changed */
static mesa_rc VOICE_VLAN_conf_pre_changed_apply(int change_field, voice_vlan_conf_t *old_conf)
{
    mesa_rc                    rc = VTSS_RC_OK;
    switch_iter_t              sit;
    port_iter_t                pit;
    vtss_appl_vlan_port_conf_t vlan_port_conf;
#if VOICE_VLAN_CHECK_CONFLICT_CONF
#if defined(VTSS_SW_OPTION_LLDP)
    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_conf;
#endif /* VTSS_SW_OPTION_LLDP */
#endif /* VOICE_VLAN_CHECK_CONFLICT_CONF */

    /* 1. Change mode from enabled to disabled
     * 2. Change VOICE_VLAN ID when mode is enabled */
    if (old_conf->mode == VOICE_VLAN_MGMT_ENABLED &&
        ((change_field & VOICE_VLAN_CONF_CHANGE_MODE) || (change_field & VOICE_VLAN_CONF_CHANGE_VID))) {

        if (old_conf->mode == VOICE_VLAN_MGMT_ENABLED &&
            (change_field & VOICE_VLAN_CONF_CHANGE_MODE)) {
            /* Delete reserved QCE */
            VOICE_VLAN_reserved_qce_del();
        }

        /* Delete original VOICE_VLAN */
        if ((rc = vlan_mgmt_vlan_del(VTSS_ISID_GLOBAL, old_conf->vid, VTSS_APPL_VLAN_USER_VOICE_VLAN)) != VTSS_RC_OK) {
            return rc;
        }

        /* Clear LLDP telephony MAC entries */
        if (change_field & VOICE_VLAN_CONF_CHANGE_MODE) {
            VOICE_VLAN_lldp_telephony_mac_entry_clear();
        }

        /* Clear "oui_cnt" */
        VOICE_VLAN_global.oui_cnt.clear();

        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (change_field & VOICE_VLAN_CONF_CHANGE_MODE) {
                    /* Un-override any parameter when change mode from enabled to disabled */
                    memset(&vlan_port_conf, 0x0, sizeof(vlan_port_conf));
                    if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &vlan_port_conf, VTSS_APPL_VLAN_USER_VOICE_VLAN)) != VTSS_RC_OK) {
                        T_E("vlan_mgmt_port_conf_set(%u, %d): failed rc = %s", sit.isid, pit.iport, error_txt(rc));
                    }

                    /* Disable software-based learning */
                    if (VOICE_VLAN_global.sw_learn[sit.isid][pit.iport]) {
                        if ((rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_VOICE_VLAN, sit.isid, pit.iport, FALSE, PSEC_PORT_MODE_NORMAL)) == VTSS_RC_OK) {
                            VOICE_VLAN_global.sw_learn[sit.isid][pit.iport] = 0;
                        }
                    }
                }
            }
        }
    }

#if VOICE_VLAN_CHECK_CONFLICT_CONF
    /* Check LLDP port mode when change mode from disabled to enabled */
    if (old_conf->mode == VOICE_VLAN_MGMT_DISABLED &&
        (change_field & VOICE_VLAN_CONF_CHANGE_MODE)) {
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
#if defined(VTSS_SW_OPTION_LLDP)
            if ((sit.isid != VTSS_ISID_START) || (rc = lldp_mgmt_conf_get(&lldp_conf[0])) != VTSS_RC_OK) {
                T_W("Calling lldp_mgmt_conf_get() failed, isid=%d, rc = %s", sit.isid, error_txt(rc));
                break;
            }
            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (VOICE_VLAN_global.port_conf[sit.isid].port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO &&
                    VOICE_VLAN_global.port_conf[sit.isid].discovery_protocol[pit.iport] != VOICE_VLAN_DISCOVERY_PROTOCOL_OUI &&
                    (lldp_conf[pit.iport].admin_states == (vtss_appl_lldp_admin_state_t)VTSS_APPL_LLDP_DISABLED || lldp_conf[pit.iport].admin_states == (vtss_appl_lldp_admin_state_t)VTSS_APPL_LLDP_ENABLED_TX_ONLY)) {
                    return VOICE_VLAN_ERROR_IS_CONFLICT_WITH_LLDP;
                }
            }
#endif /* VTSS_SW_OPTION_LLDP */
        }
    }
#endif /* VOICE_VLAN_CHECK_CONFLICT_CONF */

    return rc;
}

/* Apply VOICE_VLAN configuration post-changed */
static mesa_rc VOICE_VLAN_conf_post_changed_apply(int change_field, voice_vlan_conf_t *new_conf)
{
    mesa_rc     rc = VTSS_RC_OK;
    vtss_isid_t isid;

    /* Change mode from disabled to enabled */
    if (new_conf->mode == VOICE_VLAN_MGMT_ENABLED &&
        ((change_field & VOICE_VLAN_CONF_CHANGE_MODE) || (change_field & VOICE_VLAN_CONF_CHANGE_VID))) {
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!msg_switch_exists(isid)) {
                continue;
            }
            rc = VOICE_VLAN_conf_apply(0, isid);
        }
    }

    if (change_field & VOICE_VLAN_CONF_CHANGE_AGE_TIME) {
        rc = psec_mgmt_time_conf_set(VTSS_APPL_PSEC_USER_VOICE_VLAN, new_conf->age_time, PSEC_ZOMBIE_HOLD_TIME_SECS);
    }

    if (new_conf->mode == VOICE_VLAN_MGMT_ENABLED &&
        ((change_field & VOICE_VLAN_CONF_CHANGE_TRAFFIC_CLASS) || (change_field & VOICE_VLAN_CONF_CHANGE_VID))) {
        voice_vlan_msg_req_t msg_reg;

        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!msg_switch_exists(isid)) {
                continue;
            }
            memset(&msg_reg, 0x0, sizeof(msg_reg));
            if (change_field & VOICE_VLAN_CONF_CHANGE_TRAFFIC_CLASS) {
                msg_reg.req.qce_conf_set.change_traffic_class = TRUE;
                msg_reg.req.qce_conf_set.traffic_class = new_conf->traffic_class;
            }
            if (change_field & VOICE_VLAN_CONF_CHANGE_VID) {
                msg_reg.req.qce_conf_set.change_vid = TRUE;
                msg_reg.req.qce_conf_set.vid = new_conf->vid;
            }
            VOICE_VLAN_CRIT_ENTER();
            VOICE_VLAN_local_qce_set(isid, &msg_reg);
            VOICE_VLAN_CRIT_EXIT();
        }
    }

#if defined(VTSS_SW_OPTION_LLDP)
    /* Only process it when global mode changed from disabled to enabled */
    if ((change_field & VOICE_VLAN_CONF_CHANGE_MODE) && new_conf->mode == VOICE_VLAN_MGMT_ENABLED) {
        voice_vlan_port_conf_t port_conf;

        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!msg_switch_exists(isid)) {
                continue;
            }
            VOICE_VLAN_CRIT_ENTER();
            port_conf = VOICE_VLAN_global.port_conf[isid];
            VOICE_VLAN_CRIT_EXIT();
            VOICE_VLAN_update_lldp_info(isid, VTSS_PORT_NO_NONE, &port_conf);
        }
    }
#endif /* VTSS_SW_OPTION_LLDP */

    return rc;
}

/* Apply VOICE_VLAN port configuration post-changed */
static mesa_rc VOICE_VLAN_port_conf_post_changed_apply(u32 *change_field, vtss_isid_t isid, voice_vlan_port_conf_t *new_conf)
{
    mesa_rc                    rc = VTSS_RC_OK;
    port_iter_t                pit;
    vtss_appl_vlan_entry_t     vlan_member;
    vtss_appl_vlan_port_conf_t vlan_port_conf;
    voice_vlan_msg_req_t       msg_reg;
    BOOL                       reopen1, reopen2, reopen3;
#if defined(VTSS_SW_OPTION_LLDP)
    BOOL                       global_mode;
#endif /* VTSS_SW_OPTION_LLDP */

    T_D("enter");

    if (!msg_switch_exists(isid)) {
        T_D("exit");
        return rc;
    }

    VOICE_VLAN_CRIT_ENTER();

    if (VOICE_VLAN_global.conf.mode == VOICE_VLAN_MGMT_ENABLED) {
        if (vtss_appl_vlan_get(isid, VOICE_VLAN_global.conf.vid, &vlan_member, FALSE, VTSS_APPL_VLAN_USER_VOICE_VLAN) != VTSS_RC_OK) {
            vtss_clear(vlan_member);
            vlan_member.vid = VOICE_VLAN_global.conf.vid;
        }
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (change_field[pit.iport] == 0) {
                continue;
            }
            reopen1 = reopen2 = reopen3 = FALSE;

            /* Change VOICE_VLAN members */
            if ((change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE) ||
                (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_SECURITY) ||
                (new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO && (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PROTOCOL))) {
                reopen1 = (new_conf->security[pit.iport] == VOICE_VLAN_MGMT_ENABLED &&
                           (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE) && new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO);

                if ((change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE) ||
                    (new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO && (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PROTOCOL))) {
                    if (new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_FORCED) {
                        /* Join to VOICE_VLAN when port mode is forced */
                        vlan_member.ports[pit.iport] = 1;
                    } else {
                        /* Disjoin to VOICE_VLAN when port mode is disabled or auto */
                        vlan_member.ports[pit.iport] = 0;
                    }
                }

                /* Clear LLDP telephony MAC entries on this port when:
                 * 1. Change port mode
                 * 2. Change port discovery protocol to OUI */
                reopen2 = ((!(change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE)) && new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO && (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PROTOCOL) && (new_conf->discovery_protocol[pit.iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_OUI));
                if (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE || reopen2) {
                    VOICE_VLAN_lldp_telephony_mac_entry_clear_by_port(isid, pit.iport);
                }

                /* Clear "oui_-cnt" on this port when:
                 * 1. Change port mode
                 * 2. Change port discovery protocol to LLDP
                 * 3. Change port security mode */
                reopen3 = ((!(change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE)) && new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO && (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PROTOCOL) && (new_conf->discovery_protocol[pit.iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP)) ||
                          ((change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_SECURITY) && new_conf->security[pit.iport] == VOICE_VLAN_MGMT_ENABLED);
                if (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE || reopen3) {
                    VOICE_VLAN_global.oui_cnt[isid][pit.iport] = 0;
                }
            }

            /* Change MAC learning mode */
            /* Enable software-based learning when:
             * 1. Voice VLAN and port security mode are enabled
             * 2. Support auto OUI discovery protocol */
            if ((new_conf->port_mode[pit.iport] != VOICE_VLAN_PORT_MODE_DISABLED && new_conf->security[pit.iport] == VOICE_VLAN_MGMT_ENABLED) ||
                (new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO && new_conf->discovery_protocol[pit.iport] != VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP)) {
                if (VOICE_VLAN_global.sw_learn[isid][pit.iport] == 0 || reopen1 || reopen2 || reopen3) {
                    if ((rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_VOICE_VLAN, isid, pit.iport, TRUE, PSEC_PORT_MODE_NORMAL)) == VTSS_RC_OK) {
                        VOICE_VLAN_global.sw_learn[isid][pit.iport] = 1;
                    }
                }
            } else if (VOICE_VLAN_global.sw_learn[isid][pit.iport]) {
                /* Disable software-based learning */
                if ((rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_VOICE_VLAN, isid, pit.iport, FALSE, PSEC_PORT_MODE_NORMAL)) == VTSS_RC_OK) {
                    VOICE_VLAN_global.sw_learn[isid][pit.iport] = 0;
                }
            }

            /* Change ingress filter */
            if (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE) {
                if (new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_DISABLED) {
                    /* Un-override any parameter */
                    memset(&vlan_port_conf, 0x0, sizeof(vlan_port_conf));
                    if ((rc = vlan_mgmt_port_conf_set(isid, pit.iport, &vlan_port_conf, VTSS_APPL_VLAN_USER_VOICE_VLAN)) != VTSS_RC_OK) {
                        T_E("vlan_mgmt_port_conf_set(%u, %d): failed rc = %s", isid, pit.iport, error_txt(rc));
                    }
                } else {
                    /* Disable ingress filter when port mode isn't disabled */
                    if (vlan_mgmt_port_conf_get(isid, pit.iport, &vlan_port_conf, VTSS_APPL_VLAN_USER_VOICE_VLAN, TRUE) == VTSS_RC_OK) {
                        vlan_port_conf.hybrid.ingress_filter = FALSE;
                        vlan_port_conf.hybrid.port_type = VTSS_APPL_VLAN_PORT_TYPE_C;

                        // Force port to tag Voice VLAN on egress
                        vlan_port_conf.hybrid.tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS;
                        vlan_port_conf.hybrid.untagged_vid = VOICE_VLAN_global.conf.vid;

                        vlan_port_conf.hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT | VTSS_APPL_VLAN_PORT_FLAGS_AWARE | VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE;
                        if ((rc = vlan_mgmt_port_conf_set(isid, pit.iport, &vlan_port_conf, VTSS_APPL_VLAN_USER_VOICE_VLAN)) != VTSS_RC_OK) {
                            T_E("vlan_mgmt_port_conf_set(%u, %d): failed rc = %s", isid, pit.iport, error_txt(rc));
                        }
                    }
                }
            }
        }

        rc = vlan_mgmt_vlan_add(isid, &vlan_member, VTSS_APPL_VLAN_USER_VOICE_VLAN);

        memset(&msg_reg, 0x0, sizeof(msg_reg));
        msg_reg.req.qce_conf_set.is_port_list = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            msg_reg.req.qce_conf_set.port_list[pit.iport] = vlan_member.ports[pit.iport];
        }
        VOICE_VLAN_local_qce_set(isid, &msg_reg);
    }

    VOICE_VLAN_CRIT_EXIT();

#if defined(VTSS_SW_OPTION_LLDP)
    VOICE_VLAN_CRIT_ENTER();
    global_mode = VOICE_VLAN_global.conf.mode;
    VOICE_VLAN_CRIT_EXIT();

    if (global_mode == VOICE_VLAN_MGMT_ENABLED) {
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (((change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE) && new_conf->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO) ||
                ((change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PROTOCOL) && new_conf->port_mode[pit.iport] != VOICE_VLAN_DISCOVERY_PROTOCOL_OUI)) {
                VOICE_VLAN_update_lldp_info(isid, pit.iport, new_conf);
            }
        }
    }
#endif /* VTSS_SW_OPTION_LLDP */

    T_D("exit");
    return rc;
}


/****************************************************************************/
/*  Default and configuration chagned functions                             */
/****************************************************************************/

/* Determine if VOICE_VLAN configuration has changed */
static int VOICE_VLAN_conf_changed(voice_vlan_conf_t *old, voice_vlan_conf_t *new_)
{
    int change_field = 0;

    if (old->mode != new_->mode) {
        change_field |= VOICE_VLAN_CONF_CHANGE_MODE;
    }
    if (old->vid != new_->vid) {
        change_field |= VOICE_VLAN_CONF_CHANGE_VID;
    }
    if (old->age_time != new_->age_time) {
        change_field |= VOICE_VLAN_CONF_CHANGE_AGE_TIME;
    }
    if (old->traffic_class != new_->traffic_class) {
        change_field |= VOICE_VLAN_CONF_CHANGE_TRAFFIC_CLASS;
    }

    return change_field;
}

/* Set VOICE_VLAN defaults */
static void VOICE_VLAN_default_set(voice_vlan_conf_t *conf)
{
    conf->mode = VOICE_VLAN_MGMT_DEFAULT_MODE;
    conf->vid = VOICE_VLAN_MGMT_DEFAULT_VID;
    conf->age_time = VOICE_VLAN_MGMT_DEFAULT_AGE_TIME;
    conf->traffic_class = uprio2iprio(VOICE_VLAN_MGMT_DEFAULT_TRAFFIC_CLASS);
}

/* Determine if VOICE_VLAN port configuration has changed */
static int VOICE_VLAN_port_conf_changed(vtss_isid_t isid, voice_vlan_port_conf_t *old, voice_vlan_port_conf_t *new_, u32 *change_field)
{
    port_iter_t pit;

    if (change_field != NULL) {
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (old->port_mode[pit.iport] != new_->port_mode[pit.iport]) {
                change_field[pit.iport] |= VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE;
            }
            if (old->security[pit.iport] != new_->security[pit.iport]) {
                change_field[pit.iport] |= VOICE_VLAN_PORT_CONF_CHANGE_SECURITY;
            }
            if (old->discovery_protocol[pit.iport] != new_->discovery_protocol[pit.iport]) {
                change_field[pit.iport] |= VOICE_VLAN_PORT_CONF_CHANGE_PROTOCOL;
            }
        }

    }

    return vtss_memcmp(*new_, *old);
}

/* Set VOICE_VLAN port defaults */
static void VOICE_VLAN_port_default_set(vtss_isid_t isid, voice_vlan_port_conf_t *conf)
{
    mesa_port_no_t  port_no;

    if (isid == VTSS_ISID_GLOBAL) {
        return;
    }

    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        conf->port_mode[port_no] = VOICE_VLAN_MGMT_DEFAULT_PORT_MODE;
        conf->security[port_no] = VOICE_VLAN_MGMT_DEFAULT_SECURITY;
        conf->discovery_protocol[port_no] = VOICE_VLAN_MGMT_DEFAULT_DISCOVERY_PROTOCOL;
    }
}

/* Determine if VOICE_VLAN OUI configuration has changed */
static int VOICE_VLAN_oui_conf_changed(voice_vlan_oui_conf_t *old, voice_vlan_oui_conf_t *new_)
{
    return (memcmp(new_, old, sizeof(*new_)));
}

/* Set VOICE_VLAN OUI defaults */
static void VOICE_VLAN_oui_default_set(voice_vlan_oui_conf_t *conf)
{
    memset(conf, 0x0, sizeof(*conf));

    // Default set of phone OUIs is no longer set here; it has been moved to
    // icfg/icfg-default-config.txt. Otherwise it is not possible to delete
    // OUIs across a reboot: startup-config won't list the deleted OUIs, but
    // they would be added here.
}


/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
static const char *VOICE_VLAN_msg_id_txt(voice_vlan_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case VOICE_VLAN_MSG_ID_LLDP_CB_IND:
        txt = "VOICE_VLAN_MSG_ID_LLDP_CB_IND";
        break;
    case VOICE_VLAN_MSG_ID_QCE_SET_REQ:
        txt = "VOICE_VLAN_MSG_ID_QCE_SET_REQ";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}
#endif /* VTSS_TRACE_LVL_DEBUG */

static BOOL VOICE_VLAN_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    voice_vlan_msg_id_t msg_id = *(voice_vlan_msg_id_t *)rx_msg;

    T_D("msg_id: %d, %s, len: %zd, isid: %u", msg_id, VOICE_VLAN_msg_id_txt(msg_id), len, isid);

    switch (msg_id) {
    case VOICE_VLAN_MSG_ID_LLDP_CB_IND: {
#if defined(VTSS_SW_OPTION_LLDP)
        voice_vlan_msg_req_t *msg = (voice_vlan_msg_req_t *)rx_msg;
        VOICE_VLAN_porcess_lldp_cb(isid, msg->req.lldp_cb_ind.port_no, &msg->req.lldp_cb_ind.entry);
#endif /* VTSS_SW_OPTION_LLDP */
        break;
    }
    case VOICE_VLAN_MSG_ID_QCE_SET_REQ: {
        voice_vlan_msg_req_t *msg = (voice_vlan_msg_req_t *)rx_msg;

        if (msg->req.qce_conf_set.del_qce) {
            (void) vtss_appl_qos_qce_intern_del(VTSS_ISID_LOCAL, VTSS_APPL_QOS_QCL_USER_VOICE_VLAN, VOICE_VLAN_RESERVED_QCE_ID);
        } else {
            vtss_appl_qos_qce_intern_conf_t qce_conf;
            mesa_qce_t                      *const q = &qce_conf.qce;
            mesa_qce_key_t                  *const k = &q->key;
            mesa_qce_action_t               *const a = &q->action;

            if (vtss_appl_qos_qce_intern_get(VTSS_ISID_LOCAL, VTSS_APPL_QOS_QCL_USER_VOICE_VLAN, VOICE_VLAN_RESERVED_QCE_ID, &qce_conf, FALSE) != VTSS_RC_OK) {
                (void)vtss_appl_qos_qce_intern_get_default(&qce_conf);
                q->id = VOICE_VLAN_RESERVED_QCE_ID;
                qce_conf.isid = VTSS_ISID_LOCAL;
                qce_conf.user_id = VTSS_APPL_QOS_QCL_USER_VOICE_VLAN;
            }

            a->prio = msg->req.qce_conf_set.traffic_class;
            a->prio_enable = TRUE;
            k->tag.vid.type = MESA_VCAP_VR_TYPE_VALUE_MASK;
            k->tag.vid.vr.v.value = msg->req.qce_conf_set.vid;
            k->tag.vid.vr.v.mask = 0xFFF;

            if (msg->req.qce_conf_set.is_port_list) {
                mesa_port_no_t iport;
                for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
                    k->port_list[iport] = msg->req.qce_conf_set.port_list[iport];
                }
            } else if (msg->req.qce_conf_set.is_port_add) {
                k->port_list[msg->req.qce_conf_set.iport] = TRUE;
            } else if (msg->req.qce_conf_set.is_port_del) {
                k->port_list[msg->req.qce_conf_set.iport] = FALSE;
            }
            (void) vtss_appl_qos_qce_intern_add(VTSS_APPL_QOS_QCE_ID_NONE, &qce_conf);
        }
        break;
    }
    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }
    return TRUE;
}

static mesa_rc VOICE_VLAN_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = VOICE_VLAN_msg_rx;
    filter.modid = VTSS_MODULE_ID_VOICE_VLAN;
    return msg_rx_filter_register(&filter);
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* VOICE_VLAN error text */
const char *voice_vlan_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "Operation only valid on primary switch";

    case VOICE_VLAN_ERROR_ISID:
        return "Invalid Switch ID";

    case VOICE_VLAN_ERROR_ISID_NON_EXISTING:
        return "Switch ID is non-existing";

    case VOICE_VLAN_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    case VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_MGMT_VID:
        return "Voice VID is conflict with managed VID";

    case VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_MVR_VID:
        return "Voice VID is conflict with MVR VID";

    case VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_STATIC_VID:
        return "Voice VID is conflict with static VID";

    case VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_PVID:
        return "Voice VID is conflict with PVID";

    case VOICE_VLAN_ERROR_IS_CONFLICT_WITH_LLDP:
        return "Voice VLAN port configuration is conflict with LLDP";

    case VOICE_VLAN_ERROR_PARM_NULL_OUI_ADDR:
        return "Voice VLAN module parameter error of null OUI address";

    case VOICE_VLAN_ERROR_ENTRY_NOT_EXIST:
        return "Voice VLAN table entry not exist";

    case VOICE_VLAN_ERROR_ENTRY_ALREADY_EXIST:
        return "Voice VLAN table entry already exist";

    case VOICE_VLAN_ERROR_REACH_MAX_OUI_ENTRY:
        return "Voice VLAN OUI table reach max entries";

    default:
        return "Voice VLAN: Unknown error code";
    }
}

/* Get VOICE_VLAN configuration */
mesa_rc voice_vlan_mgmt_conf_get(voice_vlan_conf_t *glbl_cfg)
{
    T_D("enter");

    if (glbl_cfg == NULL) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    VOICE_VLAN_CRIT_ENTER();
    *glbl_cfg = VOICE_VLAN_global.conf;
    VOICE_VLAN_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set VOICE_VLAN configuration */
mesa_rc voice_vlan_mgmt_conf_set(voice_vlan_conf_t *glbl_cfg)
{
    mesa_rc                 rc      = VTSS_RC_OK;
    int                     changed = 0;

    T_D("enter, mode: %d, vid: %d age_time: %d traffic_class: %d",
        glbl_cfg->mode, glbl_cfg->vid, glbl_cfg->age_time, glbl_cfg->traffic_class);

    if (glbl_cfg == NULL) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    /* Check illegal parameter */
    if ((glbl_cfg->mode != VOICE_VLAN_MGMT_ENABLED && glbl_cfg->mode != VOICE_VLAN_MGMT_DISABLED) ||
        (glbl_cfg->vid < VTSS_APPL_VLAN_ID_MIN || glbl_cfg->vid > VTSS_APPL_VLAN_ID_MAX) ||
        (glbl_cfg->age_time < VOICE_VLAN_MIN_AGE_TIME || glbl_cfg->age_time > VOICE_VLAN_MAX_AGE_TIME) ||
        (glbl_cfg->traffic_class > uprio2iprio(VOICE_VLAN_MAX_TRAFFIC_CLASS)) ||
        VOICE_VLAN_is_valid_voice_vid(glbl_cfg->vid)) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }

    VOICE_VLAN_CRIT_ENTER();
    changed = VOICE_VLAN_conf_changed(&VOICE_VLAN_global.conf, glbl_cfg);
    if (changed) {
        if ((rc = VOICE_VLAN_conf_pre_changed_apply(changed, &VOICE_VLAN_global.conf)) != VTSS_RC_OK) {
            VOICE_VLAN_CRIT_EXIT();
            T_D("exit");
            return rc;
        }
    }
    VOICE_VLAN_global.conf = *glbl_cfg;
    VOICE_VLAN_CRIT_EXIT();

    if (changed) {
        /* Apply configuration post-changed */
        rc = VOICE_VLAN_conf_post_changed_apply(changed, glbl_cfg);
    }

    T_D("exit");
    return rc;
}

/* Get VOICE_VLAN port configuration */
mesa_rc voice_vlan_mgmt_port_conf_get(vtss_isid_t isid, voice_vlan_port_conf_t *switch_cfg)
{
    T_D("enter");

    if (switch_cfg == NULL) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return VOICE_VLAN_ERROR_ISID_NON_EXISTING;
    }

    VOICE_VLAN_CRIT_ENTER();
    *switch_cfg = VOICE_VLAN_global.port_conf[isid];
    VOICE_VLAN_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set VOICE_VLAN port configuration */
mesa_rc voice_vlan_mgmt_port_conf_set(vtss_isid_t isid, voice_vlan_port_conf_t *switch_cfg)
{
    mesa_rc                     rc      = VTSS_RC_OK;
    int                         changed = 0;
    port_iter_t                 pit;
    CapArray<u32, MEBA_CAP_BOARD_PORT_MAP_COUNT> change_field;
#if VOICE_VLAN_CHECK_CONFLICT_CONF
#if defined(VTSS_SW_OPTION_LLDP)
    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_conf;
#endif /* VTSS_SW_OPTION_LLDP */
#endif /* VOICE_VLAN_CHECK_CONFLICT_CONF */

    T_D("enter");

    if (switch_cfg == NULL) {
        T_D("exit");
        return VOICE_VLAN_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    if (!msg_switch_configurable(isid)) {
        T_W("isid: %d isn't configurable switch", isid);
        T_D("exit");
        return VOICE_VLAN_ERROR_ISID_NON_EXISTING;
    }

    /* Check illegal parameter */
    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if ((switch_cfg->port_mode[pit.iport] > VOICE_VLAN_PORT_MODE_FORCED) ||
            (switch_cfg->security[pit.iport] != VOICE_VLAN_MGMT_ENABLED && switch_cfg->security[pit.iport] != VOICE_VLAN_MGMT_DISABLED) ||
            (switch_cfg->discovery_protocol[pit.iport] > VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH)) {
            T_D("exit");
            return VOICE_VLAN_ERROR_INV_PARAM;
        }
#ifndef VTSS_SW_OPTION_LLDP
        switch_cfg->discovery_protocol[pit.iport] = VOICE_VLAN_DISCOVERY_PROTOCOL_OUI;
#endif /* VTSS_SW_OPTION_LLDP */
    }

    VOICE_VLAN_CRIT_ENTER();
    change_field.clear();
    changed = VOICE_VLAN_port_conf_changed(isid, &VOICE_VLAN_global.port_conf[isid], switch_cfg, change_field.data());

#if VOICE_VLAN_CHECK_CONFLICT_CONF
#if defined(VTSS_SW_OPTION_LLDP)
    if ((isid != VTSS_ISID_START) || (rc = lldp_mgmt_conf_get(&lldp_conf[0])) != VTSS_RC_OK) {
        T_W("Calling lldp_mgmt_conf_get() failed, isid=%d, rc = %s", isid, error_txt(rc));
    } else if (VOICE_VLAN_global.conf.mode == VOICE_VLAN_MGMT_ENABLED) {
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (change_field[pit.iport] == 0) {
                continue;
            }
            if (changed) {
                /* Check LLDP port mode */
                if ((change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PORT_MODE) || (change_field[pit.iport] & VOICE_VLAN_PORT_CONF_CHANGE_PROTOCOL)) {
                    if (switch_cfg->port_mode[pit.iport] == VOICE_VLAN_PORT_MODE_AUTO &&
                        switch_cfg->discovery_protocol[pit.iport] != VOICE_VLAN_DISCOVERY_PROTOCOL_OUI &&
                        (lldp_conf[pit.iport].admin_states == (vtss_appl_lldp_admin_state_t)VTSS_APPL_LLDP_DISABLED || lldp_conf[pit.iport].admin_states == (vtss_appl_lldp_admin_state_t)VTSS_APPL_LLDP_ENABLED_TX_ONLY)) {
                        VOICE_VLAN_CRIT_EXIT();
                        T_D("exit");
                        return VOICE_VLAN_ERROR_IS_CONFLICT_WITH_LLDP;
                    }
                }
            }
        }
    }
#endif /* VTSS_SW_OPTION_LLDP */
#endif /* VOICE_VLAN_CHECK_CONFLICT_CONF */

    VOICE_VLAN_global.port_conf[isid] = *switch_cfg;
    VOICE_VLAN_CRIT_EXIT();

    if (changed) {
        /* Apply configuration post-changed */
        rc = VOICE_VLAN_port_conf_post_changed_apply(change_field.data(), isid, switch_cfg);
    }

    T_D("exit");
    return rc;
}

/* Apply VOICE_VLAN OUI configuration post-changed */
static mesa_rc VOICE_VLAN_oui_conf_post_changed_apply(void)
{
    mesa_rc         rc = VTSS_RC_OK;
    switch_iter_t   sit;
    port_iter_t     pit;

    T_D("enter");

    if (!msg_switch_is_primary()) {
        T_D("not primary switch");
        T_D("exit");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    VOICE_VLAN_CRIT_ENTER();
    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (VOICE_VLAN_global.sw_learn[sit.isid][pit.iport]) {
                if ((rc = psec_mgmt_port_conf_set(VTSS_APPL_PSEC_USER_VOICE_VLAN, sit.isid, pit.iport, TRUE, PSEC_PORT_MODE_NORMAL)) == VTSS_RC_OK) {
                    VOICE_VLAN_global.oui_cnt[sit.isid][pit.iport] = 0;

                    /* Change Voice VLAN member set */
                    if ((VOICE_VLAN_global.port_conf[sit.isid].discovery_protocol[pit.iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_OUI ||
                         (VOICE_VLAN_global.port_conf[sit.isid].discovery_protocol[pit.iport] == VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH && !VOICE_VLAN_exist_telephony_lldp_device(sit.isid, pit.iport)))) {
                        if ((rc = VOICE_VLAN_disjoin(sit.isid, pit.iport, VOICE_VLAN_global.conf.vid)) != VTSS_RC_OK) {
                            T_W("VOICE_VLAN_disjoin(%u, %d, %d): failed rc = %d", sit.isid, pit.iport, VOICE_VLAN_global.conf.vid, rc);
                        }
                    }
                }
            }
        }
    }
    VOICE_VLAN_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Set VOICE_VLAN OUI configuration */
static mesa_rc VOICE_VLAN_mgmt_oui_conf_set(voice_vlan_oui_conf_t *conf, BOOL single_oui_chg)
{
    mesa_rc                     rc      = VTSS_RC_OK;
    int                         changed = 0;

    T_D("enter");

    VOICE_VLAN_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        changed = VOICE_VLAN_oui_conf_changed(&VOICE_VLAN_global.oui_conf, conf);
        VOICE_VLAN_global.oui_conf = *conf;
    } else {
        T_W("not primary switch");
        rc = VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    VOICE_VLAN_CRIT_EXIT();

    if (changed && !single_oui_chg) {
        /* Apply configuration post-changed */
        rc = VOICE_VLAN_oui_conf_post_changed_apply();
    }

    T_D("exit");
    return rc;
}


/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Restore VOICE_VLAN configuration */
static mesa_rc VOICE_VLAN_conf_reset(void)
{
    mesa_rc                 rc = VTSS_RC_OK;
    vtss_isid_t             isid;
    voice_vlan_conf_t       conf;
    voice_vlan_port_conf_t  port_conf;
    voice_vlan_oui_conf_t   oui_conf;

    T_D("enter");

    /* Initialize VOICE_VLAN configuration */
    VOICE_VLAN_default_set(&conf);
    if ((rc = voice_vlan_mgmt_conf_set(&conf)) != VTSS_RC_OK) {
        T_D("exit");
        return rc;
    }

    /* Initialize VOICE_VLAN port configuration */
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (!msg_switch_exists(isid)) {
            continue;
        }
        vtss_clear(port_conf);
        VOICE_VLAN_port_default_set(isid, &port_conf);
        rc = voice_vlan_mgmt_port_conf_set(isid, &port_conf);
    }

    /* Initialize VOICE_VLAN OUI configuration */
    if (rc == VTSS_RC_OK) {
        VOICE_VLAN_oui_default_set(&oui_conf);
        rc = VOICE_VLAN_mgmt_oui_conf_set(&oui_conf, FALSE);
    }

    T_D("exit");
    return rc;
}

/* Read/create VOICE_VLAN switch configuration */
static void VOICE_VLAN_conf_read_switch(vtss_isid_t isid_add)
{
    voice_vlan_port_conf_t      new_port_conf;
    vtss_isid_t                 isid;

    T_D("enter, isid_add: %d", isid_add);

    vtss_clear(new_port_conf);

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (isid_add != VTSS_ISID_GLOBAL && isid_add != isid) {
            continue;
        }

        VOICE_VLAN_CRIT_ENTER();
        /* Use default values */
        VOICE_VLAN_port_default_set(isid, &new_port_conf);
        VOICE_VLAN_global.port_conf[isid] = new_port_conf;
        VOICE_VLAN_CRIT_EXIT();
    }
    T_D("exit");
}

/* Read/create VOICE_VLAN stack configuration */
static mesa_rc VOICE_VLAN_conf_read_stack(BOOL create)
{
    voice_vlan_conf_t           new_voice_vlan_conf;
    voice_vlan_oui_conf_t       new_voice_vlan_oui_conf;
    mesa_rc                     rc = VTSS_RC_OK;

    T_D("enter, create: %d", create);

    VOICE_VLAN_CRIT_ENTER();
    /* Use default values */
    VOICE_VLAN_default_set(&new_voice_vlan_conf);
    VOICE_VLAN_lldp_telephony_mac_entry_clear();
    VOICE_VLAN_global.oui_cnt.clear();
    VOICE_VLAN_global.sw_learn.clear();
    VOICE_VLAN_global.conf = new_voice_vlan_conf;
    VOICE_VLAN_CRIT_EXIT();

    VOICE_VLAN_CRIT_ENTER();
    /* Use default values */
    VOICE_VLAN_oui_default_set(&new_voice_vlan_oui_conf);
    VOICE_VLAN_global.oui_conf = new_voice_vlan_oui_conf;
    VOICE_VLAN_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Module start */
static void VOICE_VLAN_start(BOOL init)
{
    voice_vlan_conf_t       *conf_p;
    voice_vlan_port_conf_t  *port_conf_p;
    voice_vlan_oui_conf_t   *oui_conf_p;
    vtss_isid_t             isid;
    mesa_rc                 rc;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize VOICE_VLAN configuration */
        conf_p = &VOICE_VLAN_global.conf;
        VOICE_VLAN_default_set(conf_p);

        /* Initialize VOICE_VLAN port configuration */
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            port_conf_p = &VOICE_VLAN_global.port_conf[isid];
            VOICE_VLAN_port_default_set(isid, port_conf_p);
        }

        /* Initialize VOICE_VLAN OUI configuration */
        oui_conf_p = &VOICE_VLAN_global.oui_conf;
        VOICE_VLAN_oui_default_set(oui_conf_p);

        /* Initialize LLDP telephony MAC entry */
        VOICE_VLAN_lldp_telephony_mac_entry_clear();

        /* Initialize counters */
        VOICE_VLAN_global.oui_cnt.clear();
        VOICE_VLAN_global.sw_learn.clear();

        /* Initialize message buffers */
        vtss_sem_init(&VOICE_VLAN_global.request.sem, 1);

        /* Create semaphore for critical regions */
        critd_init(&VOICE_VLAN_global.crit, "voice_vlan", VTSS_MODULE_ID_VOICE_VLAN, CRITD_TYPE_MUTEX);
    } else {
        /* Register for stack messages */
        if ((rc = VOICE_VLAN_stack_register()) != VTSS_RC_OK) {
            T_W("VOICE_VLAN_stack_register(): failed rc = %d", rc);
        }

        /* Register for port linkup/link-down status changed */
        if ((rc = port_change_register(VTSS_MODULE_ID_VOICE_VLAN, VOICE_VLAN_port_change_cb)) != VTSS_RC_OK) {
            T_W("port_change_register(Voice VLAN): failed rc = %d", rc);
        }

        /* Register to port security module */
        if ((rc = psec_mgmt_register_callbacks(VTSS_APPL_PSEC_USER_VOICE_VLAN, VOICE_VLAN_psec_mac_add_cb, VOICE_VLAN_psec_mac_del_cb)) != VTSS_RC_OK) {
            T_W("psec_mgmt_register_callbacks(Voice VLAN): failed rc = %d", rc);
        }

#if defined(VTSS_SW_OPTION_LLDP)
        /* Register to LLDP module */
        lldp_mgmt_entry_updated_callback_register(VOICE_VLAN_lldp_entry_cb);
#endif /* VTSS_SW_OPTION_LLDP */
    }
    T_D("exit");
}

/****************************************************************************/
// Initialize module
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void vtss_appl_voice_vlan_mib_init(void);
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_voice_vlan_json_init(void);
#endif /* VTSS_SW_OPTION_JSON_RPC */
extern "C" int voice_vlan_icli_cmd_register();

/****************************************************************************/
// voice_vlan_init()
/****************************************************************************/
mesa_rc voice_vlan_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc     rc   = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        VOICE_VLAN_start(1);

#ifdef VTSS_SW_OPTION_ICFG
        rc = voice_vlan_icfg_init();
        if (rc != VTSS_RC_OK) {
            T_D("fail to init icfg registration, rc = %s", error_txt(rc));
        }
#endif
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        vtss_appl_voice_vlan_mib_init();    /* Register Voice VLAN Private-MIB */
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_voice_vlan_json_init();   /* Register Voice VLAN JSON-RPC */
#endif /* VTSS_SW_OPTION_JSON_RPC */
        voice_vlan_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");
        VOICE_VLAN_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            if ((rc = VOICE_VLAN_conf_reset()) != VTSS_RC_OK) {
                T_D("Calling VOICE_VLAN_conf_reset(): failed rc = %s", voice_vlan_error_txt(rc));
            }
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        /* Read stack and switch configuration */
        if ((rc = VOICE_VLAN_conf_read_stack(0)) != VTSS_RC_OK) {
            T_W("Calling VOICE_VLAN_conf_read_stack(0): failed rc = %s", voice_vlan_error_txt(rc));
        }

        VOICE_VLAN_conf_read_switch(VTSS_ISID_GLOBAL);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");

        /* Apply VOICE_VLAN initial configuration */
        if ((rc = VOICE_VLAN_conf_apply(1, isid)) != VTSS_RC_OK) {
            T_D("Calling VOICE_VLAN_conf_apply(isid = %u): failed rc = %s", isid, voice_vlan_error_txt(rc));
        }

        break;

    default:
        break;
    }

    return rc;
}

/****************************************************************************/
/*  Other management functions                                              */
/****************************************************************************/

/* The API uses for checking conflicted configuration with LLDP module.
 * User cannot set LLDP port mode to disabled or TX only when Voice-VLAN
 * support LLDP discovery protocol. */
BOOL voice_vlan_is_supported_LLDP_discovery(vtss_isid_t isid, mesa_port_no_t port_no)
{
    T_D("enter");

    VOICE_VLAN_CRIT_ENTER();
    if (VOICE_VLAN_global.conf.mode == VOICE_VLAN_MGMT_ENABLED &&
        VOICE_VLAN_global.port_conf[isid].port_mode[port_no] == VOICE_VLAN_PORT_MODE_AUTO &&
        VOICE_VLAN_global.port_conf[isid].discovery_protocol[port_no] != VOICE_VLAN_DISCOVERY_PROTOCOL_OUI) {
        VOICE_VLAN_CRIT_EXIT();
        T_D("exit");
        return TRUE;
    }
    VOICE_VLAN_CRIT_EXIT();

    T_D("exit");
    return FALSE;
}

/* Check Voice VLAN ID is conflict with other configurations */
mesa_rc VOICE_VLAN_is_valid_voice_vid(mesa_vid_t voice_vid)
{
#if VOICE_VLAN_CHECK_CONFLICT_CONF
#if defined(VTSS_SW_OPTION_MVR)
    vtss_appl_ipmc_lib_vlan_key_t  vlan_key;
    vtss_appl_ipmc_lib_vlan_conf_t mvr_vlan_conf;
#endif

    mesa_rc rc = VTSS_RC_OK;

    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        return VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

#if defined(VTSS_SW_OPTION_MVR)
    /* Check conflict with MVR VID */
    vlan_key.is_mvr  = true; // Check to see if this is an MVR VLAN
    vlan_key.is_ipv4 = true; // Doesn't matter, since IGMP and MLD confs are identical for a given VLAN.
    vlan_key.vid     = voice_vid;
    if (vtss_appl_ipmc_lib_vlan_conf_get(vlan_key, &mvr_vlan_conf) == VTSS_RC_OK) {
        return VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_MVR_VID;
    }
#endif /* VTSS_SW_OPTION_MVR */

    return rc;
#else
    return VTSS_RC_OK;
#endif /* VOICE_VLAN_CHECK_CONFLICT_CONF */
}

/*****************************************************************************
    Public API section for Private VLAN
    from vtss_appl/include/vtss/appl/pvlan.h
*****************************************************************************/
#include "vtss_common_iterator.hxx"

/**
 * \brief Get the capabilities of Voice VLAN.
 *
 * \param cap       [OUT]   The capability properties of the Voice VLAN module.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_capabilities_get(vtss_appl_voice_vlan_capabilities_t *const cap)
{
    if (!cap) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    cap->max_oui_registration_entry_count = VOICE_VLAN_OUI_ENTRIES_CNT;
    cap->max_forwarding_traffic_class = VOICE_VLAN_MAX_TRAFFIC_CLASS;
    cap->max_oui_learning_aging_time = VOICE_VLAN_MAX_AGE_TIME;
    cap->min_oui_learning_aging_time = VOICE_VLAN_MIN_AGE_TIME;
#ifdef VTSS_SW_OPTION_LLDP
    cap->support_lldp_discovery_notification = TRUE;
#else
    cap->support_lldp_discovery_notification = FALSE;
#endif /* VTSS_SW_OPTION_LLDP */

    return VTSS_RC_OK;
}

/**
 * \brief Get Voice VLAN global default configuration.
 *
 * Get default configuration of the Voice VLAN global setting.
 *
 * \param entry     [OUT]   The default configuration of the Voice VLAN global setting.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_global_config_default(
    vtss_appl_voice_vlan_global_conf_t              *const entry
)
{
    voice_vlan_conf_t   ety;

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    VOICE_VLAN_default_set(&ety);
    entry->admin_state = ety.mode;
    entry->voice_vlan_id = ety.vid;
    entry->aging_time = ety.age_time;
    entry->traffic_class = ety.traffic_class;

    return VTSS_RC_OK;
}

/**
 * \brief Get Voice VLAN global configuration.
 *
 * Get configuration of the Voice VLAN global setting.
 *
 * \param entry     [OUT]   The current configuration of the Voice VLAN global setting.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_global_config_get(
    vtss_appl_voice_vlan_global_conf_t              *const entry
)
{
    mesa_rc             vrc;
    voice_vlan_conf_t   ety;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if ((vrc = voice_vlan_mgmt_conf_get(&ety)) == VTSS_RC_OK) {
        entry->admin_state = ety.mode;
        entry->voice_vlan_id = ety.vid;
        entry->aging_time = ety.age_time;
        entry->traffic_class = ety.traffic_class;
    }

    T_D("DONE(%s)", error_txt(vrc));
    return vrc;
}

/**
 * \brief Set/Update Voice VLAN global configuration.
 *
 * Modify configuration of the Voice VLAN global setting.
 *
 * \param entry     [IN]    The revised configuration of the Voice VLAN global setting.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_global_config_set(
    const vtss_appl_voice_vlan_global_conf_t        *const entry
)
{
    mesa_rc             vrc;
    voice_vlan_conf_t   ety;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if ((vrc = voice_vlan_mgmt_conf_get(&ety)) == VTSS_RC_OK) {
        ety.mode = entry->admin_state;
        ety.vid = entry->voice_vlan_id;
        ety.age_time = entry->aging_time;
        ety.traffic_class = entry->traffic_class;
        vrc = voice_vlan_mgmt_conf_set(&ety);
    }

    T_D("DONE(%s)", error_txt(vrc));
    return vrc;
}

/**
 * \brief Iterator for retrieving Voice VLAN port configuration table index.
 *
 * Retrieve the 'next' configuration index of the Voice VLAN port configuration table
 * according to the given 'prev'.
 *
 * \param prev      [IN]    Porinter of port interface index to be used for index determination.
 *
 * \param next      [OUT]   The next index should be used for the table entry.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Error code.      VTSS_RC_OK for success operation and the value in 'next' is valid,
 *                          other error code means that no "next" index and its corresponding
 *                          entry exists, and the end has been reached.
 */
mesa_rc vtss_appl_voice_vlan_port_itr(
    const vtss_ifindex_t                            *const prev,
    vtss_ifindex_t                                  *const next
)
{
    if (!next) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    return vtss_appl_iterator_ifindex_front_port_exist(prev, next);
}

/**
 * \brief Get Voice VLAN port interface default configuration.
 *
 * Get default configuration of the Voice VLAN port interface.
 *
 * \param ifindex   [OUT]   The logical interface index of Voice VLAN port to be used.
 * \param entry     [OUT]   The default configuration of the Voice VLAN port interface.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_port_config_default(
    vtss_ifindex_t                                  *const ifindex,
    vtss_appl_voice_vlan_port_conf_t                *const entry
)
{
    if (!ifindex || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if (vtss_ifindex_from_port(VTSS_ISID_START, VTSS_PORT_NO_START, ifindex) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    vtss_clear(*entry);

#if   VOICE_VLAN_MGMT_DEFAULT_PORT_MODE == VOICE_VLAN_PORT_MODE_DISABLED
    entry->management = VTSS_APPL_VOICE_VLAN_MANAGEMENT_DISABLE;
#elif VOICE_VLAN_MGMT_DEFAULT_PORT_MODE == VOICE_VLAN_PORT_MODE_AUTO
    entry->management = VTSS_APPL_VOICE_VLAN_MANAGEMENT_AUTOMATIC;
#else
    entry->management = VTSS_APPL_VOICE_VLAN_MANAGEMENT_FORCE_MEMBER;
#endif

#if   VOICE_VLAN_MGMT_DEFAULT_DISCOVERY_PROTOCOL == VOICE_VLAN_DISCOVERY_PROTOCOL_OUI
    entry->protocol = VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI;
#elif VOICE_VLAN_MGMT_DEFAULT_DISCOVERY_PROTOCOL == VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP
    entry->protocol = VTSS_APPL_VOICE_VLAN_DISCOVER_BY_LLDP;
#elif VOICE_VLAN_MGMT_DEFAULT_DISCOVERY_PROTOCOL == VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH
    entry->protocol = VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI_OR_LLDP;
#endif

#if   VOICE_VLAN_MGMT_DEFAULT_SECURITY == VOICE_VLAN_MGMT_DISABLED
    entry->secured = FALSE;
#else
    entry->secured = TRUE;
#endif

    return VTSS_RC_OK;
}

/**
 * \brief Get Voice VLAN specific port interface configuration.
 *
 * Get configuration of the specific Voice VLAN port.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of Voice VLAN port.
 *
 * \param entry     [OUT]   The current configuration of the specific Voice VLAN port.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_port_config_get(
    const vtss_ifindex_t                            *const ifindex,
    vtss_appl_voice_vlan_port_conf_t                *const entry
)
{
    vtss_ifindex_elm_t      ife;
    vtss_isid_t             isid;
    u32                     ptx;
    voice_vlan_port_conf_t  ety;
    mesa_rc                 vrc;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from given IfIndex */
    if (!ifindex || !entry ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT ||
        (ptx = ife.ordinal) >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }
    isid = ife.isid;
    T_D("ISID:%u PORT:%u", isid, ptx);

    vtss_clear(ety);
    if ((vrc = voice_vlan_mgmt_port_conf_get(isid, &ety)) == VTSS_RC_OK) {
        switch ( ety.port_mode[ptx] ) {
        case VOICE_VLAN_PORT_MODE_AUTO:
            entry->management = VTSS_APPL_VOICE_VLAN_MANAGEMENT_AUTOMATIC;
            break;
        case VOICE_VLAN_PORT_MODE_FORCED:
            entry->management = VTSS_APPL_VOICE_VLAN_MANAGEMENT_FORCE_MEMBER;
            break;
        case VOICE_VLAN_PORT_MODE_DISABLED:
            entry->management = VTSS_APPL_VOICE_VLAN_MANAGEMENT_DISABLE;
            break;
        default:
            T_D("Invalid Management:%d", ety.port_mode[ptx]);
            vrc = VOICE_VLAN_ERROR_INV_PARAM;
            break;
        }

        if (vrc == VTSS_RC_OK) {
            switch ( ety.discovery_protocol[ptx] ) {
            case VOICE_VLAN_DISCOVERY_PROTOCOL_OUI:
                entry->protocol = VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI;
                break;
            case VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP:
                entry->protocol = VTSS_APPL_VOICE_VLAN_DISCOVER_BY_LLDP;
                break;
            case VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH:
                entry->protocol = VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI_OR_LLDP;
                break;
            default:
                T_D("Invalid Protocol:%d", ety.discovery_protocol[ptx]);
                vrc = VOICE_VLAN_ERROR_INV_PARAM;
                break;
            }
        }

        if (vrc == VTSS_RC_OK) {
            entry->secured = ety.security[ptx];
        }
    }

    T_D("GET(%s:%d:%u)-%d/%d/%s",
        vrc == VTSS_RC_OK ? "OK" : "NG", isid, ptx,
        entry->management, entry->protocol,
        entry->secured ? "Secured" : "Insecure");
    return vrc;
}

/**
 * \brief Set/Update Voice VLAN specific port interface configuration.
 *
 * Modify configuration of the specific Voice VLAN port.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of Voice VLAN port.
 * \param entry     [IN]    The revised configuration of the specific Voice VLAN port.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_port_config_set(
    const vtss_ifindex_t                            *const ifindex,
    const vtss_appl_voice_vlan_port_conf_t          *const entry
)
{
    vtss_ifindex_elm_t      ife;
    vtss_isid_t             isid;
    u32                     ptx;
    voice_vlan_port_conf_t  ety;
    mesa_rc                 vrc;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from given IfIndex */
    if (!ifindex || !entry ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT ||
        (ptx = ife.ordinal) >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }
    isid = ife.isid;
    T_D("ISID:%u PORT:%u", isid, ptx);

    vtss_clear(ety);
    if ((vrc = voice_vlan_mgmt_port_conf_get(isid, &ety)) == VTSS_RC_OK) {
        switch ( entry->management ) {
        case VTSS_APPL_VOICE_VLAN_MANAGEMENT_AUTOMATIC:
            ety.port_mode[ptx] = VOICE_VLAN_PORT_MODE_AUTO;
            break;
        case VTSS_APPL_VOICE_VLAN_MANAGEMENT_FORCE_MEMBER:
            ety.port_mode[ptx] = VOICE_VLAN_PORT_MODE_FORCED;
            break;
        case VTSS_APPL_VOICE_VLAN_MANAGEMENT_DISABLE:
            ety.port_mode[ptx] = VOICE_VLAN_PORT_MODE_DISABLED;
            break;
        default:
            T_D("Invalid Management:%d", entry->management);
            vrc = VOICE_VLAN_ERROR_INV_PARAM;
            break;
        }

        if (vrc == VTSS_RC_OK) {
            switch ( entry->protocol ) {
            case VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI:
                ety.discovery_protocol[ptx] = VOICE_VLAN_DISCOVERY_PROTOCOL_OUI;
                break;
            case VTSS_APPL_VOICE_VLAN_DISCOVER_BY_LLDP:
                ety.discovery_protocol[ptx] = VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP;
                break;
            case VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI_OR_LLDP:
                ety.discovery_protocol[ptx] = VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH;
                break;
            default:
                T_D("Invalid Protocol:%d", entry->protocol);
                vrc = VOICE_VLAN_ERROR_INV_PARAM;
                break;
            }
        }

        if (vrc == VTSS_RC_OK) {
            ety.security[ptx] = entry->secured;
            vrc = voice_vlan_mgmt_port_conf_set(isid, &ety);
        }
    }

    T_D("SET(%s:%d:%u)-%d/%d/%s",
        vrc == VTSS_RC_OK ? "OK" : "NG", isid, ptx,
        entry->management, entry->protocol,
        entry->secured ? "Secured" : "Insecure");
    return vrc;
}

/**
 * \brief Iterator for retrieving Voice VLAN telephony OUI configuration table index.
 *
 * Retrieve the 'next' configuration index of the Voice VLAN telephony OUI table
 * according to the given 'prev'.
 *
 * \param prev      [IN]    Porinter of OUI configuration index to be used for index determination.
 *
 * \param next      [OUT]   The next index should be used for the table entry.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Error code.      VTSS_RC_OK for success operation and the value in 'next' is valid,
 *                          other error code means that no "next" index and its corresponding
 *                          entry exists, and the end has been reached.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_itr(
    const vtss_appl_voice_vlan_oui_index_t          *const prev,
    vtss_appl_voice_vlan_oui_index_t                *const next
)
{
    mesa_rc                 vrc;
    voice_vlan_oui_entry_t  ety;

    if (!next) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(&ety, 0x0, sizeof(voice_vlan_oui_entry_t));
    if (prev) {
        memcpy(ety.oui_addr, prev, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN);
    }
    T_D("OUI NXT %X-%X-%X", ety.oui_addr[0], ety.oui_addr[1], ety.oui_addr[2]);
    if ((vrc = voice_vlan_oui_entry_get(&ety, TRUE)) == VTSS_RC_OK) {
        T_D("GOT OUI %X-%X-%X", ety.oui_addr[0], ety.oui_addr[1], ety.oui_addr[2]);
        memcpy(next, ety.oui_addr, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN);
    }

    T_D("DONE(%s)", error_txt(vrc));
    return vrc;
}

/**
 * \brief Get Voice VLAN telephony OUI entry's default configuration.
 *
 * Get default configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [OUT]   Telephony OUI prefix to be used in Voice VLAN.
 * \param entry     [OUT]   The default configuration of the Voice VLAN telephony OUI entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_default(
    vtss_appl_voice_vlan_oui_index_t                *const ouiindex,
    vtss_appl_voice_vlan_telephony_oui_conf_t       *const entry
)
{
    if (!ouiindex || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(ouiindex, 0x0, sizeof(vtss_appl_voice_vlan_oui_index_t));
    memset(entry, 0x0, sizeof(vtss_appl_voice_vlan_telephony_oui_conf_t));

    return VTSS_RC_OK;
}

/**
 * \brief Get Voice VLAN specific telephony OUI entry's configuration.
 *
 * Get configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [IN]    (key) OUI index - Telephony OUI prefix index used in Voice VLAN.
 *
 * \param entry     [OUT]   The current configuration of the specific Voice VLAN telephony OUI entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_get(
    const vtss_appl_voice_vlan_oui_index_t          *const ouiindex,
    vtss_appl_voice_vlan_telephony_oui_conf_t       *const entry
)
{
    mesa_rc                 vrc;
    voice_vlan_oui_entry_t  ety;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!ouiindex || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(&ety, 0x0, sizeof(voice_vlan_oui_entry_t));
    memcpy(ety.oui_addr, ouiindex, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN);
    vrc = voice_vlan_oui_entry_get(&ety, FALSE);
    if (vrc != VTSS_RC_OK) {
        T_D("NonExisting OUI %X-%X-%X", ety.oui_addr[0], ety.oui_addr[1], ety.oui_addr[2]);
    } else {
        int desc_len = strlen(ety.description);

        memset(entry->description, 0x0, sizeof(entry->description));
        if (desc_len > 0) {
            strncpy(entry->description, ety.description, desc_len);
        }
    }

    T_D("DONE(%s)", error_txt(vrc));
    return vrc;
}

/**
 * \brief Add Voice VLAN specific telephony OUI entry's configuration.
 *
 * Create configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [IN]    (key) OUI index - Telephony OUI prefix index used in Voice VLAN.
 * \param entry     [IN]    The new configuration of the specific Voice VLAN telephony OUI entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_add(
    const vtss_appl_voice_vlan_oui_index_t          *const ouiindex,
    const vtss_appl_voice_vlan_telephony_oui_conf_t *const entry
)
{
    mesa_rc                 vrc;
    voice_vlan_oui_entry_t  ety;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!ouiindex || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(&ety, 0x0, sizeof(voice_vlan_oui_entry_t));
    memcpy(ety.oui_addr, ouiindex, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN);
    vrc = voice_vlan_oui_entry_get(&ety, FALSE);
    if (vrc == VTSS_RC_OK) {
        T_D("Existing OUI %X-%X-%X", ety.oui_addr[0], ety.oui_addr[1], ety.oui_addr[2]);
        vrc = VOICE_VLAN_ERROR_ENTRY_ALREADY_EXIST;
    } else {
        int desc_len = strlen(entry->description);

        memset(ety.description, 0x0, sizeof(ety.description));
        if (desc_len > 0) {
            strncpy(ety.description, entry->description, desc_len);
        }
        vrc = voice_vlan_oui_entry_add(&ety);
    }

    T_D("DONE(%s)", error_txt(vrc));
    return vrc;
}

/**
 * \brief Set/Update Voice VLAN specific telephony OUI entry's configuration.
 *
 * Modify configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [IN]    (key) OUI index - Telephony OUI prefix index used in Voice VLAN.
 * \param entry     [IN]    The revised configuration of the specific Voice VLAN telephony OUI entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_set(
    const vtss_appl_voice_vlan_oui_index_t          *const ouiindex,
    const vtss_appl_voice_vlan_telephony_oui_conf_t *const entry
)
{
    mesa_rc                 vrc;
    voice_vlan_oui_entry_t  ety;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!ouiindex || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(&ety, 0x0, sizeof(voice_vlan_oui_entry_t));
    memcpy(ety.oui_addr, ouiindex, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN);
    vrc = voice_vlan_oui_entry_get(&ety, FALSE);
    if (vrc != VTSS_RC_OK) {
        T_D("NonExisting OUI %X-%X-%X", ety.oui_addr[0], ety.oui_addr[1], ety.oui_addr[2]);
    } else {
        int desc_len = strlen(entry->description);

        memset(ety.description, 0x0, sizeof(ety.description));
        if (desc_len > 0) {
            strncpy(ety.description, entry->description, desc_len);
        }
        vrc = voice_vlan_oui_entry_add(&ety);
    }

    T_D("DONE(%s)", error_txt(vrc));
    return vrc;
}

/**
 * \brief Delete Voice VLAN specific telephony OUI entry's configuration.
 *
 * Remove configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [IN]    (key) OUI index - Telephony OUI prefix index used in Voice VLAN.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_del(
    const vtss_appl_voice_vlan_oui_index_t          *const ouiindex
)
{
    mesa_rc                 vrc;
    voice_vlan_oui_entry_t  ety;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!ouiindex) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(&ety, 0x0, sizeof(voice_vlan_oui_entry_t));
    memcpy(ety.oui_addr, ouiindex, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN);
    vrc = voice_vlan_oui_entry_get(&ety, FALSE);
    if (vrc != VTSS_RC_OK) {
        T_D("NonExisting OUI %X-%X-%X", ety.oui_addr[0], ety.oui_addr[1], ety.oui_addr[2]);
    } else {
        vrc = voice_vlan_oui_entry_del(&ety);
    }

    T_D("DONE(%s)", error_txt(vrc));
    return vrc;
}

