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

#include "vtss/appl/pvlan.h"
#include "pvlan.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "pvlan_icfg.h"
#endif
#include "misc_api.h"
#include "port_api.h"
#include "port_iter.hxx"

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static pvlan_global_t pvlan;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "pvlan", "PVLAN table"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define PVLAN_CRIT_ENTER() critd_enter(&pvlan.crit, __FUNCTION__, __LINE__)
#define PVLAN_CRIT_EXIT()  critd_exit( &pvlan.crit, __FUNCTION__, __LINE__)

/****************************************************************************/
/*  NAMING CONVENTION FOR INTERNAL FUNCTIONS:                               */
/*    SM_<function_name>   : Functions related to SrcMask-based VLAN        */
/*                           (standalone, only).                            */
/*    PI_<function_name>   : Functions related to Port Isolation            */
/*                           (standalone and stacking).                     */
/*    PVLAN_<function_name>: Functions common to SM_xxx and PI_xxx.         */
/*                                                                          */
/*  NAMING CONVENTION FOR EXTERNAL (API) FUNCTIONS:                         */
/*    pvlan_mgmt_pvlan_XXX  : Functions related to SrcMask-based VLAN.      */
/*    pvlan_mgmt_isolate_XXX: Functions related to Port Isolation.          */
/*                                                                          */
/* Notice, that in the Datasheet, Port Isolation is known as Private VLANs. */
/****************************************************************************/

/****************************************************************************/
//
//  SM_xxx PRIVATE FUNCTIONS
//
/****************************************************************************/

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// SM_entry_add()
// Description:Add Private VLAN entry via switch API
// - Calling API function mesa_pvlan_port_members_set()
// Input:
// Output: Return VTSS_RC_OK if success
/****************************************************************************/
static mesa_rc SM_entry_add(BOOL privatevid_delete, SM_entry_conf_t *conf)
{
    mesa_port_list_t member;
    port_iter_t      pit;
    uint32_t         loop_port_up_inj = fast_cap(VTSS_APPL_CAP_LOOP_PORT_UP_INJ);

    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        member[pit.iport] = ((!privatevid_delete) && conf->ports[VTSS_ISID_LOCAL][pit.iport]) ? 1 : 0;
    }

    /* Loop ports are members of all PVLANs */
    if (loop_port_up_inj != MESA_PORT_NO_NONE) {
        member[loop_port_up_inj] = 1;
    }
#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    member[VTSS_SW_OPTION_MIRROR_LOOP_PORT] = 1;
#endif /* VTSS_SW_OPTION_MIRROR_LOOP_PORT */

    // T_I("Private VLAN Add: private privatevid %u, privatevid_delete %d, ports 0x%x",
    //    conf->privatevid,
    //    privatevid_delete,
    //    conf->ports[VTSS_ISID_LOCAL][0]);

    return mesa_pvlan_port_members_set(NULL, conf->privatevid, &member);
}
#endif /* PVLAN_SRC_MASK_ENA */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// SM_list_pvlan_del()
// If @local is TRUE, modifies pvlan.switch_pvlan, i.e. this switch's
// configuration. Otherwise modifies pvlan.stack_pvlan.
// Deleting a PVLAN on the stack works globally. All ISIDs get it removed.
/****************************************************************************/
static mesa_rc SM_list_pvlan_del(BOOL local, mesa_pvlan_no_t privatevid)
{
    SM_pvlan_t      *pvlan_p, *prev;
    SM_pvlan_list_t *list;
    mesa_rc         rc = VTSS_RC_OK;

    T_I("Delete old PVLAN privatevid: %u", privatevid);

    PVLAN_CRIT_ENTER();

    list = (local ? &pvlan.switch_pvlan : &pvlan.stack_pvlan);

    if (list->used == NULL) {
        rc = PVLAN_ERROR_PVLAN_TABLE_EMPTY;
        goto exit_func;
    }

    /* Delete old PVLAN */
    pvlan_p = list->used;
    prev = NULL;
    while (pvlan_p) {
        if (pvlan_p->conf.privatevid == privatevid) {
            break;
        }
        prev = pvlan_p;
        pvlan_p = pvlan_p->next;
    }

    if (!pvlan_p) {
        rc = PVLAN_ERROR_ENTRY_NOT_FOUND;
        goto exit_func;
    }

    // Concatenate the used list.
    if (prev) {
        prev->next = pvlan_p->next;
    } else {
        list->used = pvlan_p->next;
    }

    // Move it to free list
    pvlan_p->next = list->free;
    list->free = pvlan_p;

exit_func:
    PVLAN_CRIT_EXIT();
    return rc;
}
#endif /* PVLAN_SRC_MASK_ENA */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// SM_list_pvlan_add()
// Adds or overwrites a private vlan to @isid_add.
// @isid_add may be either
//   VTSS_ISID_LOCAL: Called from msg_rx, i.e. we modify the switch-specific list.
//   VTSS_ISID_LEGAL: Any legal switch ID, i.e. we modify the stack-specific list.
//   VTSS_ISID_GLOBAL: All legal switch IDs, i.e. we modify the stack-specific list.
/****************************************************************************/
static mesa_rc SM_list_pvlan_add(SM_entry_conf_t *pvlan_entry, vtss_isid_t isid_add, BOOL *privatevid_is_new)
{
    SM_pvlan_t      *temp_pvlan, *prev_pvlan, *new_pvlan;
    SM_pvlan_list_t *list;
    mesa_rc         rc = VTSS_RC_OK;
#if VTSS_SWITCH_STANDALONE
    BOOL no_ports_in_pvlan;
    mesa_port_no_t  iport;
#endif

    T_I("enter: privatevid %u isid_add %d", pvlan_entry->privatevid, isid_add);

    PVLAN_CRIT_ENTER();

    list = (isid_add == VTSS_ISID_LOCAL ? &pvlan.switch_pvlan : &pvlan.stack_pvlan);

    // Find insertion/overwrite point
    temp_pvlan = list->used;
    prev_pvlan = NULL;
    while (temp_pvlan) {
        // The Private PRIVATEVIDs are inserted in numeric, inceasing order.
        if (temp_pvlan->conf.privatevid > pvlan_entry->privatevid) {
            break;
        }
        prev_pvlan = temp_pvlan;
        temp_pvlan = temp_pvlan->next;
    }

    // Now, prev_pvlan points to the entry to which to append the new entry, or
    // in case of an overwrite, the entry to overwrite. The remainder of the
    // list is given by temp_pvlan.

    // Better tell the caller whether we're overwriting an existing or not.
    *privatevid_is_new = (prev_pvlan == NULL || prev_pvlan->conf.privatevid != pvlan_entry->privatevid);

    if (*privatevid_is_new) {
        T_D("Adding PRIVATEVID=%u to used list", pvlan_entry->privatevid);

        // Attempt to add a new entry.
        if (list->free == NULL) {
            rc = PVLAN_ERROR_PVLAN_TABLE_FULL;
            goto exit_func; // goto statements are quite alright when we always need to do something special before exiting a function.
        }

        // Pick the first item from the free list.
        new_pvlan = list->free;
        list->free = list->free->next;

        // Insert the new entry in the appropriate position in list->used.
        new_pvlan->next = temp_pvlan; // Attach remainder of list.

        if (prev_pvlan) {
            // Entries before this exist.
            prev_pvlan->next = new_pvlan;
        } else {
            // This goes first in the list.
            list->used = new_pvlan;
        }
    } else {
        // Overwrite existing
        T_D("Replacing PRIVATEVID=%u in used list for isid = %u", pvlan_entry->privatevid, isid_add);
        new_pvlan = prev_pvlan;
    }

    /* This check is to make lint happy */
    if (new_pvlan == NULL) {
        rc = PVLAN_ERROR_PVLAN_TABLE_FULL;
        goto exit_func;
    }

    // Populate new_pvlan entry.
    if (*privatevid_is_new || isid_add == VTSS_ISID_GLOBAL) {
        // Overwrite everything of our current configuration, assuming
        // that pvlan_entry is properly initialized.
        new_pvlan->conf = *pvlan_entry;
    } else {
        // Only overwrite the portion that has changed.
        new_pvlan->conf.ports[isid_add] = pvlan_entry->ports[isid_add];
    }

#if VTSS_SWITCH_STANDALONE
    no_ports_in_pvlan = 1;
    /* If the user specifies a PVLAN with no ports this shall be treated as if the PVLAN was deleted. */
    T_I("Checking if all ports are deselected");
    if (isid_add == VTSS_ISID_GLOBAL) {
        // Safe, since we're in standalone mode
        isid_add = VTSS_ISID_START;
    }

    for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
        // Check if we have reached the port vector that includes the stack ports.
        if (new_pvlan->conf.ports[isid_add][iport]) {
            no_ports_in_pvlan = 0;
            break;
        }
    }

    PVLAN_CRIT_EXIT();

    // If no ports were added for this VLAN then delete the VLAN.
    if (no_ports_in_pvlan) {
        T_I("No ports in vlan - deleting");
        (void)SM_list_pvlan_del(FALSE, new_pvlan->conf.privatevid);
        rc = PVLAN_ERROR_DEL_INSTEAD_OF_ADD;
    }
    return rc;
#endif /* VTSS_SWITCH_STANDALONE */

exit_func:
    PVLAN_CRIT_EXIT();
    return rc;
}
#endif /* PVLAN_SRC_MASK_ENA */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// Set PVLAN configuration to switch
/****************************************************************************/
static mesa_rc SM_stack_conf_set(vtss_isid_t isid_add)
{
    vtss_isid_t     isid;
    SM_pvlan_t      *pvlan_stack, *pvlan_switch;
    SM_pvlan_list_t *switch_list;
    SM_entry_conf_t *conf;
    CapArray<SM_entry_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> conf_table;
    BOOL            privatevid_is_new;
    u32             i, pvlan_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    T_D("enter, isid_add: %d", isid_add);

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if ((isid_add != VTSS_ISID_GLOBAL && isid_add != isid) ||
            !msg_switch_exists(isid)) {
            continue;
        }

        // Sets the whole configuration - not just one PVLAN entry.
        // We need to delete all currently configured PVLANs and
        // possibly create some new ones. This will shortly give
        // rise to no configured PVLANs, but that's ignorable.

        PVLAN_CRIT_ENTER();
        switch_list = &pvlan.switch_pvlan; // Select the actual configuration

        // Always delete default PVLAN, since it is initialized by the Switch API to contain
        // all ports upon boot, and if it's not included in the list of PVLANs loaded
        // from startup-config, all ports will always be members of all PVLANs, independent of how
        // the remaining PVLANs are configured.
        conf = &conf_table[0];
        conf->privatevid = 0;
        (void)SM_entry_add(PVLAN_DELETE, conf);

        /* Delete current PVLAN */
        for (pvlan_switch = switch_list->used; pvlan_switch != NULL; pvlan_switch = pvlan_switch->next) {
            T_D("Calling SM_entry_add - PVLAN_DELETE privatevid %u", pvlan_switch->conf.privatevid);
            (void)SM_entry_add(PVLAN_DELETE, &pvlan_switch->conf);
            if (pvlan_switch->next == NULL) {
                /* Last entry found, move used list to free list */
                pvlan_switch->next = switch_list->free;
                switch_list->free = switch_list->used;
                switch_list->used = NULL;
                break;
            }
        }

        /* Add new PVLANs */
        for (pvlan_stack = pvlan.stack_pvlan.used, i = 0; pvlan_stack != NULL && i < pvlan_cnt; pvlan_stack = pvlan_stack->next, i++) {
            conf_table[i] = pvlan_stack->conf;
        }
        PVLAN_CRIT_EXIT();

        pvlan_cnt = i;
        for (i = 0; i < pvlan_cnt; i++) {
            conf = &conf_table[i];
            conf->ports[VTSS_ISID_LOCAL] = conf->ports[isid];
            if (SM_list_pvlan_add(conf, VTSS_ISID_LOCAL, &privatevid_is_new) == VTSS_RC_OK) {
                T_D("Calling SM_entry_add - PVLAN_ADD privatevid %u", conf->privatevid);
                (void)SM_entry_add(PVLAN_ADD, conf);
            }
        }
    }

    T_D("exit, isid: %d", isid_add);
    return VTSS_RC_OK;
}
#endif /* PVLAN_SRC_MASK_ENA */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// SM_stack_conf_add()
// Transmit the Private VLAN configuration to @isid_add for privatevid = @privatevid.
// @isid may be VTSS_ISID_GLOBAL if the configuration for @privatevid
// should be sent to all existing switches in the stack. Otherwise
// @isid is just a legal SID for which the PRIVATEVID's port configuration
// has changed (i.e. the PRIVATEVID already existed).
/****************************************************************************/
static void SM_stack_conf_add(vtss_isid_t isid_add, mesa_pvlan_no_t privatevid)
{
    vtss_isid_t     isid_min, isid_max, isid;
    SM_pvlan_t      *pvlan_stack;
    SM_entry_conf_t conf;
    BOOL            found;
    BOOL            privatevid_is_new;

    T_D("enter, isid_add: %d, privatevid: %u", isid_add, privatevid);
    vtss_clear(conf);

    PVLAN_CRIT_ENTER();

    // Find the PRIVATEVID entry that matches @privatevid
    for (found = FALSE, pvlan_stack = pvlan.stack_pvlan.used; pvlan_stack != NULL; pvlan_stack = pvlan_stack->next) {
        if (pvlan_stack->conf.privatevid == privatevid) {
            // Make a copy of the configuration, so that we can release PVLAN_CRIT
            conf = pvlan_stack->conf;
            found = TRUE;
            break;
        }
    }

    PVLAN_CRIT_EXIT();

    if (!found) {
        T_W("PRIVATEVID %u not found, isid_add: %d", privatevid, isid_add);
        return;
    }

    // Find boundaries
    if (isid_add == VTSS_ISID_GLOBAL) {
        isid_min = VTSS_ISID_START;
        isid_max = VTSS_ISID_END;
    } else {
        isid_min = isid_add;
        isid_max = isid_add + 1;
    }

    for (isid = isid_min; isid < isid_max; isid++) {
        if (!msg_switch_exists(isid)) {
            continue;
        }

        conf.ports[VTSS_ISID_LOCAL] = conf.ports[isid];
        if (SM_list_pvlan_add(&conf, VTSS_ISID_LOCAL, &privatevid_is_new) == VTSS_RC_OK) {
            (void)SM_entry_add(PVLAN_ADD, &conf);
        }
    }

    T_D("exit, isid: %d, privatevid: %u", isid_add, privatevid);
    return;
}
#endif /* PVLAN_SRC_MASK_ENA */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// Delete PVLAN from switch
/****************************************************************************/
static mesa_rc SM_stack_conf_del(mesa_pvlan_no_t privatevid)
{
    SM_entry_conf_t conf;
    vtss_isid_t     isid;
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    vtss_isid_t     isid_del = VTSS_ISID_GLOBAL;
#endif

    T_D("enter, isid: %d, privatevid: %u", isid_del, privatevid);

    vtss_clear(conf);
    conf.privatevid = privatevid;
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (!msg_switch_exists(isid)) {
            continue;
        }

        if (SM_list_pvlan_del(TRUE, privatevid) == VTSS_RC_OK) {
            (void)SM_entry_add(PVLAN_DELETE, &conf);
        }
    }

    T_D("exit, isid: %d, privatevid: %u", isid_del, privatevid);
    return VTSS_RC_OK;
}
#endif /* PVLAN_SRC_MASK_ENA */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
/****************************************************************************/
static BOOL SM_isid_invalid(vtss_isid_t isid)
{
    if (isid > VTSS_ISID_END) {
        T_W("illegal isid: %d", isid);
        return 1;
    }

    if (isid != VTSS_ISID_LOCAL && !msg_switch_is_primary()) {
        T_W("not primary switch");
        return 1;
    }

    if (VTSS_ISID_LEGAL(isid) && !msg_switch_configurable(isid)) {
        T_W("isid %d not active", isid);
        return 1;
    }

    return 0;
}
#endif /* PVLAN_SRC_MASK_ENA */

/****************************************************************************/
//
//  PI_xxx PRIVATE FUNCTIONS
//
/****************************************************************************/

/*****************************************************************/
/* Description: Setup default pvlan port configuration           */
/*                                                               */
/* Input:  Pointer to pvlan port entry                           */
/* Output:                                                       */
/*****************************************************************/
static inline void PI_default_set(mesa_port_list_t *PI_conf)
{
    /* Default to no port being isolated */
    PI_conf->clear_all();
}

/****************************************************************************/
// Set port configuration
/****************************************************************************/
static mesa_rc PI_stack_conf_set(vtss_isid_t isid)
{
    mesa_port_list_t *port_list;

    T_D("enter, isid: %d", isid);
    PVLAN_CRIT_ENTER();
    port_list = &pvlan.PI_conf[VTSS_ISID_LOCAL];
    if (pvlan.PI_conf[isid] != *port_list) {
        T_D("Calling mesa_isolated_port_members_set() with isid %u", isid);
        *port_list = pvlan.PI_conf[isid];
        (void)mesa_isolated_port_members_set(NULL, port_list);
    }
    PVLAN_CRIT_EXIT();
    T_D("exit, isid: %d", isid);

    return VTSS_RC_OK;
}

/****************************************************************************/
//
//  SHARED PRIVATE FUNCTIONS
//
/****************************************************************************/

/****************************************************************************/
// PVLAN_default_switch()
// Set defaults of all or one given switch.
// This only concerns the port-isolation PVLANs.
/****************************************************************************/
static void PVLAN_default_switch(vtss_isid_t isid_add)
{
    mesa_port_list_t new_conf;
    vtss_isid_t      isid;
    BOOL             changed;

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (isid_add != VTSS_ISID_GLOBAL && isid_add != isid) {
            continue;
        }

        // Use default values
        PI_default_set(&new_conf);

        PVLAN_CRIT_ENTER();
        changed = (new_conf != pvlan.PI_conf[isid]);
        pvlan.PI_conf[isid] = new_conf;
        PVLAN_CRIT_EXIT();

        if (isid_add != VTSS_ISID_GLOBAL && changed && msg_switch_exists(isid)) {
            (void)PI_stack_conf_set(isid);
        }
    }
}

/****************************************************************************/
// PVLAN_default_stack()
// Set defaults of the whole stack.
// For the PVLAN module, this only
// concerns the srcmask-based PVLANs (which are not enabled for the stack,
// which sounds like a contradiction, but we could enable it in the future).
/****************************************************************************/
#if defined(PVLAN_SRC_MASK_ENA)
static void PVLAN_default_stack(void)
{
    SM_pvlan_list_t *list;
    SM_pvlan_t      *pvlan_stack;
    vtss_isid_t     isid;

    PVLAN_CRIT_ENTER();

    list = &pvlan.stack_pvlan;

    /* Free old PVLANs */
    for (pvlan_stack = list->used; pvlan_stack != NULL; pvlan_stack = pvlan_stack->next) {
        if (pvlan_stack->next == NULL) {
            /* Last entry found, move used list to free list */
            pvlan_stack->next = list->free;
            list->free = list->used;
            list->used = NULL;
            break;
        }
    }

    /* Move entry from free list to used list */
    if ((pvlan_stack = list->free) == NULL) {
        T_W("This really wasn't expected. Bug.");
    } else {
        list->free = pvlan_stack->next;
        pvlan_stack->next = list->used;
        list->used = pvlan_stack;
        pvlan_stack->conf.privatevid = 0;
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            pvlan_stack->conf.ports[isid].set_all();
        }
    }

    PVLAN_CRIT_EXIT();

    (void)SM_stack_conf_set(VTSS_ISID_GLOBAL);
}
#endif /* PVLAN_SRC_MASK_ENA */

/****************************************************************************/
// Module start
/****************************************************************************/
static void PVLAN_start(void)
{
    vtss_isid_t     isid;

    T_D("enter");
#if defined(PVLAN_SRC_MASK_ENA)
    {
        int             i;
        SM_pvlan_t      *pvlan_p;
        SM_pvlan_list_t *list;
        u32             pvlan_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

        /* Initialize Private VLAN for stack: All free */
        list = &pvlan.stack_pvlan;
        list->free = NULL;
        list->used = NULL;
        for (i = 0; i < pvlan_cnt; i++) {
            pvlan_p = &pvlan.pvlan_stack_table[i];
            pvlan_p->next = list->free;
            list->free = pvlan_p;
        }

        /* Initialize Private VLAN for this switch: All free */
        list = &pvlan.switch_pvlan;
        list->free = NULL;
        list->used = NULL;
        for (i = 0; i < pvlan_cnt; i++) {
            pvlan_p = &pvlan.pvlan_switch_table[i];
            pvlan_p->next = list->free;
            list->free = pvlan_p;
        }
    }
#endif /* PVLAN_SRC_MASK_ENA */

    /* Initialize port isolation configuration */
    for (isid = VTSS_ISID_LOCAL; isid < VTSS_ISID_END; isid++) {
        PI_default_set(&pvlan.PI_conf[isid]);
    }

    /* Create semaphore for critical regions */
    critd_init(&pvlan.crit, "pvlan", VTSS_MODULE_ID_PVLAN, CRITD_TYPE_MUTEX);
    T_D("exit");
}

/****************************************************************************/
// Determine if port and ISID are valid
/****************************************************************************/
static BOOL PI_isid_invalid(vtss_isid_t isid, BOOL set)
{
    /* Check ISID */
    if (isid >= VTSS_ISID_END) {
        T_W("illegal isid: %d", isid);
        return TRUE;
    }

    if (set && isid == VTSS_ISID_LOCAL) {
        T_W("SET not allowed, isid: %d", isid);
        return TRUE;
    }

    return FALSE;
}

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// PVLAN_portlist_index_get()
/****************************************************************************/
static BOOL PVLAN_portlist_index_get(u32 i, const vtss_port_list_stackable_t *const pl)
{
    if (i >= 8 * 128 || !pl) {
        return false;
    }

    u32 idx_bit = i & 7;
    u32 idx = i >> 3;
    return (pl->data[idx] >> idx_bit) & 1;
}
#endif /* defined(PVLAN_SRC_MASK_ENA) */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// PVLAN_portlist_index_set()
/****************************************************************************/
static BOOL PVLAN_portlist_index_set(u32 i, vtss_port_list_stackable_t *const pl)
{
    if (i >= 8 * 128 || !pl) {
        return false;
    }

    u8 val = 1;
    u32 idx_bit = i & 7;
    u32 idx = i >> 3;
    val <<= idx_bit;
    pl->data[idx] |= val;
    return true;
}
#endif /* defined(PVLAN_SRC_MASK_ENA) */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// PVLAN_portlist_index_clear()
/****************************************************************************/
static BOOL PVLAN_portlist_index_clear(u32 i, vtss_port_list_stackable_t *const pl)
{
    if (i >= 8 * 128 || !pl) {
        return false;
    }

    u8 val = 1;
    u32 idx_bit = i & 7;
    u32 idx = i >> 3;
    val <<= idx_bit;
    pl->data[idx] &= (~val);
    return true;
}
#endif /* defined(PVLAN_SRC_MASK_ENA) */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// PVLAN_isid_port_to_index()
/****************************************************************************/
static u32 PVLAN_isid_port_to_index(vtss_isid_t i, mesa_port_no_t p)
{
    VTSS_ASSERT(VTSS_ISID_START == 1);
    VTSS_ASSERT((fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1) <= 128);
    return (i - 1) * 128 + iport2uport(p);
}
#endif /* defined(PVLAN_SRC_MASK_ENA) */

/****************************************************************************/
/*  Public functions                                                        */
/****************************************************************************/

/****************************************************************************/
// PVLAN error text
/****************************************************************************/
const char *pvlan_error_txt(mesa_rc rc)
{
    const char *txt;

    switch (rc) {
    case PVLAN_ERROR_GEN:
        txt = "PVLAN generic error";
        break;
    case PVLAN_ERROR_PARM:
        txt = "PVLAN parameter error";
        break;
    case PVLAN_ERROR_ENTRY_NOT_FOUND:
        txt = "Entry not found";
        break;
    case PVLAN_ERROR_PVLAN_TABLE_FULL:
        txt = "PVLAN table full";
        break;
    case PVLAN_ERROR_PVLAN_TABLE_EMPTY:
        txt = "PVLAN table empty";
        break;
    case PVLAN_ERROR_STACK_STATE:
        txt = "Invalid stack state";
        break;
    case PVLAN_ERROR_UNSUPPORTED:
        txt = "Unsupported feature";
        break;
    default:
        txt = "PVLAN unknown error";
        break;
    }
    return txt;
}

/****************************************************************************/
// Description:Management PVLAN isolate get function
// - Only accessing switch module linked list
//
// Input: isid and port isolation configuration
// Output: Return VTSS_RC_OK if success
/****************************************************************************/
mesa_rc pvlan_mgmt_isolate_conf_get(vtss_isid_t isid, mesa_port_list_t &member)
{
    T_D("enter, isid=%d", isid);

    if (PI_isid_invalid(isid, FALSE)) {
        return PVLAN_ERROR_PARM;
    }

    PVLAN_CRIT_ENTER();
    member = pvlan.PI_conf[isid];
    PVLAN_CRIT_EXIT();
    T_D("exit");

    return VTSS_RC_OK;
}

/****************************************************************************/
// pvlan_mgmt_isolate_conf_set()
// Management PVLAN port isolation set function
// - Both configuration update and chip update through message module and
// switch API
// Input: isid, port number and port isolation struct. Must be LEGAL.
// Output: Return VTSS_RC_OK if success
/****************************************************************************/
mesa_rc pvlan_mgmt_isolate_conf_set(vtss_isid_t isid, mesa_port_list_t &member)
{
    mesa_rc rc = VTSS_RC_OK;
    int     changed = 0;

    T_D("enter, isid: %d", isid);

    // We allow configuration where stack ports (in stackable builds) are
    // marked for isolation. Only when applying the config to real
    // hardware, will possible stack ports get unmarked.

    if (PI_isid_invalid(isid, TRUE)) {
        return PVLAN_ERROR_PARM;
    }

    if (msg_switch_configurable(isid)) {
        PVLAN_CRIT_ENTER();
        changed = (pvlan.PI_conf[isid] != member);
        pvlan.PI_conf[isid] = member;
        PVLAN_CRIT_EXIT();
    } else {
        T_W("isid %d not active", isid);
        return PVLAN_ERROR_STACK_STATE;
    }

    if (changed) {
        /* Activate changed configuration in stack */
        rc = PI_stack_conf_set(isid);
    }

    T_D("exit, isid: %d", isid);
    return rc;
}

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// Description:Management PVLAN list get function
// - Only accessing internal linked lists
//
// Input:Note pvlan members as boolean port list.
//       If next and privatevid = 0 then will return first PVLAN entry
// Output: Return VTSS_RC_OK if success
/****************************************************************************/
mesa_rc pvlan_mgmt_pvlan_get(vtss_isid_t isid, mesa_pvlan_no_t privatevid, pvlan_mgmt_entry_t *pvlan_mgmt_entry, BOOL next)
{
    mesa_rc         rc = VTSS_RC_OK;
    SM_pvlan_t      *pvlan_p;
    BOOL            found = FALSE;
    port_iter_t     pit;
    SM_pvlan_list_t *list;

    T_D("isid=%d, privatevid=%u, next=%d", isid, privatevid, next);
    PVLAN_CRIT_ENTER();
    list = isid == VTSS_ISID_LOCAL ? &pvlan.switch_pvlan : &pvlan.stack_pvlan;
    if (SM_isid_invalid(isid)) {
        PVLAN_CRIT_EXIT();
        return PVLAN_ERROR_STACK_STATE;
    }
    memset(pvlan_mgmt_entry->ports, 0, sizeof(pvlan_mgmt_entry->ports));
    for (pvlan_p = list->used; pvlan_p != NULL; pvlan_p = pvlan_p->next) {
        if ((next && pvlan_p->conf.privatevid > privatevid) ||
            (!next && pvlan_p->conf.privatevid == privatevid)) {
            found = TRUE;
            pvlan_mgmt_entry->privatevid = pvlan_p->conf.privatevid;
            (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                pvlan_mgmt_entry->ports[pit.iport] = pvlan_p->conf.ports[isid][pit.iport];
            }
            break;
        }
    }
    PVLAN_CRIT_EXIT();

    if (!found) {
        rc = PVLAN_ERROR_ENTRY_NOT_FOUND;
        T_D("PVLAN_ERROR_ENTRY_NOT_FOUND - Called with privatevid = %u, next=%d",
            privatevid, next);
    }
    T_D("return %d \n", rc);

    return rc;
}
#endif /* PVLAN_SRC_MASK_ENA */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// Description:Management PVLAN list add function
// - Both configuration update and chip update through message
// - module and API
//
// Input: Note pvlan members as boolean port list
//        @isid must be LEGAL or GLOBAL
// Output: Return VTSS_RC_OK if success
/****************************************************************************/
mesa_rc pvlan_mgmt_pvlan_add(vtss_isid_t isid_add, pvlan_mgmt_entry_t *pvlan_mgmt_entry)
{
    mesa_rc             rc = VTSS_RC_OK;
    BOOL                privatevid_is_new;
    SM_entry_conf_t     pvlan_entry;
    vtss_isid_t         isid, isid_min, isid_max;
    int                 max_pvlan_count = port_count_max();
    int                 pvid;

    T_D("enter, isid: %u, privatevid: %u", isid_add, pvlan_mgmt_entry->privatevid);

    if (!msg_switch_is_primary()) {
        T_W("Not primary switch");
        return PVLAN_ERROR_STACK_STATE;
    }

    if (isid_add != VTSS_ISID_GLOBAL && !VTSS_ISID_LEGAL(isid_add)) {
        T_E("Invalid ISID (%u). LEGAL or GLOBAL expected", isid_add);
        return PVLAN_ERROR_PARM;
    }

    pvid = pvlan_mgmt_entry->privatevid + 1;
    if ((pvid < PVLAN_ID_START) || (pvid > max_pvlan_count)) {
        T_D("Invalid PVLAN ID (%u). Valid range is %u - %u.", pvid,
            PVLAN_ID_START, max_pvlan_count);
        return PVLAN_ERROR_PARM;
    }

    // Convert the pvlan_mgmt_entry_t to an SM_entry_conf_t
    if (isid_add == VTSS_ISID_GLOBAL) {
        isid_min = VTSS_ISID_START;
        isid_max = VTSS_ISID_END;
    } else {
        isid_min = isid_add;
        isid_max = isid_add + 1;
    }

    pvlan_entry.privatevid = pvlan_mgmt_entry->privatevid;
    vtss_clear(pvlan_entry.ports);

    // First set the front ports for selected ISIDs.
    for (isid = isid_min; isid < isid_max; isid++) {
        pvlan_entry.ports[isid] = pvlan_mgmt_entry->ports;
    }


    // Add/modify PRIVATEVID. It may return rc != VTSS_RC_OK if table is full.
    rc = SM_list_pvlan_add(&pvlan_entry, isid_add, &privatevid_is_new);

    // And transmit it to @isid_add. If @privatevid_is_new, transmit it to all switches,
    // since all switches must include the PVLAN on their stack ports.
    if (rc == VTSS_RC_OK) {
        SM_stack_conf_add(privatevid_is_new ? VTSS_ISID_GLOBAL : isid_add, pvlan_mgmt_entry->privatevid);
    }

    return rc;
}
#endif /* PVLAN_SRC_MASK_ENA */

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
// Delete Private VLAN
// Description:Management PVLAN list delete function
// - Both configuration update and chip update through API
// Deletes globally. Use "cli: pvlan add <privatevid> <stack_port>" to
// delete frontports locally on a switch.
//
// Input:
// Output: Return VTSS_RC_OK if success
/****************************************************************************/
mesa_rc pvlan_mgmt_pvlan_del(mesa_pvlan_no_t privatevid)
{
    mesa_rc     rc;
    int         max_pvlan_count = port_count_max();
    int         pvid;

    T_D("enter, privatevid: %u", privatevid);

    pvid = privatevid + 1;
    if ((pvid < PVLAN_ID_START) || (pvid > max_pvlan_count)) {
        T_D("Invalid PVLAN ID (%u). Valid range is %u - %u.", pvid,
            PVLAN_ID_START, max_pvlan_count);
        return PVLAN_ERROR_PARM;
    }

    // Delete the entry from the stack configuration
    rc = SM_list_pvlan_del(FALSE, privatevid);

    // Tell it to all switches in the stack.
    if (rc == VTSS_RC_OK) {
        rc = SM_stack_conf_del(privatevid);
    }

    T_D("exit, id: %u", privatevid);

    return rc;
}
#endif /* PVLAN_SRC_MASK_ENA */

/*****************************************************************************
    Public API section for Private VLAN
    from vtss_appl/include/vtss/appl/pvlan.h
*****************************************************************************/
#include "vtss_common_iterator.hxx"

/**
 * \brief Get the capabilities of PVLAN.
 *
 * \param cap       [OUT]   The capability properties of the PVLAN module.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_capabilities_get(vtss_appl_pvlan_capabilities_t *const cap)
{
    if (!cap) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

#if defined(PVLAN_SRC_MASK_ENA)
    u32 max_pvlan_count = port_count_max();
    cap->support_pvlan_membership_mgmt = TRUE;
    cap->min_pvlan_membership_vlan_id = iport2uport(0);
    cap->max_pvlan_membership_vlan_id = iport2uport(max_pvlan_count - 1);
#else
    memset(cap, 0x0, sizeof(vtss_appl_pvlan_capabilities_t));
#endif /* defined(PVLAN_SRC_MASK_ENA) */

    return VTSS_RC_OK;
}

/**
 * \brief Iterator for retrieving PVLAN membership table key/index
 *
 * To walk information (configuration) index of PVLAN membership table.
 *
 * \param prev      [IN]    PVLAN index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_itr(const u32 *const prev, u32 *const next)
{
#if defined(PVLAN_SRC_MASK_ENA)
    vtss_isid_t         isid;
    switch_iter_t       sit;
    mesa_pvlan_no_t     pvx;
    pvlan_mgmt_entry_t  ety;
    mesa_rc             rc;
#endif /* defined(PVLAN_SRC_MASK_ENA) */
    if (!next) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

#if defined(PVLAN_SRC_MASK_ENA)
    T_D("enter: PREV=%s->%u", prev ? "PTR" : "NUL", prev ? *prev : 0);

    rc = PVLAN_ERROR_GEN;
    isid = VTSS_ISID_GLOBAL;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        if (msg_switch_is_local(sit.isid)) {
            isid = sit.isid;
            break;
        }
    }
    if (isid == VTSS_ISID_GLOBAL) {
        T_D("Cannot determine ISID");
        return rc;
    }

    if (!prev) {
        pvx = 0;
        rc = pvlan_mgmt_pvlan_get(isid, pvx, &ety, FALSE);
    } else {
        pvx = uport2iport(*prev);
        rc = pvlan_mgmt_pvlan_get(isid, pvx, &ety, TRUE);
    }

    if (rc == VTSS_RC_OK) {
        *next = iport2uport(ety.privatevid);
    }

    T_D("exit(%s): NEXT=%u", rc == VTSS_RC_OK ? "OK" : "NG", *next);
    return rc;
#else
    return PVLAN_ERROR_UNSUPPORTED;
#endif /* defined(PVLAN_SRC_MASK_ENA) */
}

/**
 * \brief Iterator for retrieving PVLAN isolation table key/index
 *
 * To walk information (configuration) port index of PVLAN isolation table.
 *
 * \param prev      [IN]    Interface index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_isolation_itr(const vtss_ifindex_t *const prev, vtss_ifindex_t *const next)
{
    return vtss_appl_iterator_ifindex_front_port_exist(prev, next);
}

/**
 * \brief Get PVLAN specific membership configuration
 *
 * To get configuration of the specific PVLAN membership entry.
 *
 * \param idx       [IN]    (key) PVLAN index - the index of PVLAN membership.
 *
 * \param entry     [OUT]   The current configuration of the specific PVLAN membership.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_config_get(const u32 *const idx, vtss_appl_pvlan_membership_conf_t *const entry)
{
#if defined(PVLAN_SRC_MASK_ENA)
    switch_iter_t      sit;
    vtss_isid_t        isid;
    pvlan_mgmt_entry_t ety;
    mesa_pvlan_no_t    zero_based_pvlan_id;
    mesa_rc            rc;
#endif /* defined(PVLAN_SRC_MASK_ENA) */

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!idx || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

#if defined(PVLAN_SRC_MASK_ENA)
    if (!PVLAN_ID_IS_LEGAL(*idx)) {
        T_D("Invalid PVLAN Index:%u", *idx);
        return VTSS_RC_ERROR;
    }

    rc = PVLAN_ERROR_GEN;
    zero_based_pvlan_id = uport2iport(*idx);
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        isid = sit.isid;
        if ((rc = pvlan_mgmt_pvlan_get(isid, zero_based_pvlan_id, &ety, FALSE)) == VTSS_RC_OK) {
            u32         port;
            port_iter_t pit;

            memset(entry, 0x0, sizeof(vtss_appl_pvlan_membership_conf_t));
            (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                port = pit.iport;
                if (port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                    if (ety.ports[port] && port_is_front_port(port)) {
                        (void)PVLAN_portlist_index_set(PVLAN_isid_port_to_index(isid, port), &entry->member_ports);
                    } else {
                        (void)PVLAN_portlist_index_clear(PVLAN_isid_port_to_index(isid, port), &entry->member_ports);
                    }
                }
            }
        }
    }

    return rc;
#else
    return PVLAN_ERROR_UNSUPPORTED;
#endif /* defined(PVLAN_SRC_MASK_ENA) */
}

/**
 * \brief Add PVLAN specific membership configuration
 *
 * To create configuration of the specific PVLAN membership entry.
 *
 * \param idx       [IN]    (key) PVLAN index - the index of PVLAN membership.
 * \param entry     [IN]    The new configuration of the specific PVLAN membership entry.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_config_add(const u32 *const idx, const vtss_appl_pvlan_membership_conf_t *const entry)
{
#if defined(PVLAN_SRC_MASK_ENA)
    vtss_isid_t        isid;
    switch_iter_t      sit;
    port_iter_t        pit;
    pvlan_mgmt_entry_t ety;
    mesa_pvlan_no_t    zero_based_pvlan_id;
    mesa_rc            rc;
    BOOL               add_ok;
    u32                max_pvlan_count = port_count_max();
#endif /* defined(PVLAN_SRC_MASK_ENA) */

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!idx || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

#if defined(PVLAN_SRC_MASK_ENA)
    if ((*idx < PVLAN_ID_START) || (*idx > max_pvlan_count)) {
        T_D("Invalid PVLAN Index:%u", *idx);
        return VTSS_RC_ERROR;
    }

    isid = VTSS_ISID_GLOBAL;
    zero_based_pvlan_id = uport2iport(*idx);
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        if (msg_switch_is_local(sit.isid)) {
            isid = sit.isid;
            break;
        }
    }
    if (isid == VTSS_ISID_GLOBAL) {
        T_D("Cannot determine ISID");
        return PVLAN_ERROR_GEN;
    }

    rc = pvlan_mgmt_pvlan_get(isid, zero_based_pvlan_id, &ety, FALSE);
    if (rc == VTSS_RC_OK) {
        T_D("Existing PVLAN %u", *idx);
        return PVLAN_ERROR_PARM;
    }

    add_ok = TRUE;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        isid = sit.isid;
        vtss_clear(ety);
        ety.privatevid = zero_based_pvlan_id;
        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (pit.iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                ety.ports[pit.iport] = PVLAN_portlist_index_get(PVLAN_isid_port_to_index(isid, pit.iport), &entry->member_ports);
            }
        }

        if ((rc = pvlan_mgmt_pvlan_add(isid, &ety)) != VTSS_RC_OK) {
            add_ok = FALSE;
        }
        T_D("Set ISID:%u/RC=(%s)", isid, error_txt(rc));
    }

    T_D("DONE(%s)", add_ok ? "OK" : "NG");
    return (add_ok ? VTSS_RC_OK : VTSS_RC_ERROR);
#else
    return PVLAN_ERROR_UNSUPPORTED;
#endif /* defined(PVLAN_SRC_MASK_ENA) */
}

/**
 * \brief Set/Update PVLAN specific membership configuration
 *
 * To modify configuration of the specific PVLAN membership entry.
 *
 * \param idx       [IN]    (key) PVLAN index - the index of PVLAN membership.
 * \param entry     [IN]    The revised configuration of the specific PVLAN membership entry.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_config_set(const u32 *const idx, const vtss_appl_pvlan_membership_conf_t *const entry)
{
#if defined(PVLAN_SRC_MASK_ENA)
    vtss_isid_t        isid;
    switch_iter_t      sit;
    port_iter_t        pit;
    pvlan_mgmt_entry_t ety;
    mesa_pvlan_no_t    zero_based_pvlan_id;
    mesa_rc            rc;
    BOOL               set_ok;
    u32                max_pvlan_count = port_count_max();
#endif /* defined(PVLAN_SRC_MASK_ENA) */

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!idx || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

#if defined(PVLAN_SRC_MASK_ENA)
    if ((*idx < PVLAN_ID_START) || (*idx > max_pvlan_count)) {
        T_D("Invalid PVLAN Index:%u", *idx);
        return VTSS_RC_ERROR;
    }

    isid = VTSS_ISID_GLOBAL;
    zero_based_pvlan_id = uport2iport(*idx);
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        if (msg_switch_is_local(sit.isid)) {
            isid = sit.isid;
            break;
        }
    }
    if (isid == VTSS_ISID_GLOBAL) {
        T_D("Cannot determine ISID");
        return PVLAN_ERROR_GEN;
    }

    rc = pvlan_mgmt_pvlan_get(isid, zero_based_pvlan_id, &ety, FALSE);
    if (rc != VTSS_RC_OK) {
        T_D("No valid PVLAN %u", *idx);
        return PVLAN_ERROR_ENTRY_NOT_FOUND;
    }

    set_ok = TRUE;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        isid = sit.isid;
        vtss_clear(ety);
        if (pvlan_mgmt_pvlan_get(isid, zero_based_pvlan_id, &ety, FALSE) == VTSS_RC_OK) {
            (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (pit.iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                    ety.ports[pit.iport] = PVLAN_portlist_index_get(PVLAN_isid_port_to_index(isid, pit.iport), &entry->member_ports);
                }
            }

            if ((rc = pvlan_mgmt_pvlan_add(isid, &ety)) != VTSS_RC_OK) {
                set_ok = FALSE;
            }

            T_D("Set ISID:%u/RC=(%s)", isid, error_txt(rc));
        }
    }

    T_D("DONE(%s)", set_ok ? "OK" : "NG");
    return (set_ok ? VTSS_RC_OK : VTSS_RC_ERROR);
#else
    return PVLAN_ERROR_UNSUPPORTED;
#endif /* defined(PVLAN_SRC_MASK_ENA) */
}

/**
 * \brief Delete PVLAN specific membership configuration
 *
 * To remove configuration of the specific PVLAN membership entry.
 *
 * \param idx     [IN]    (key) PVLAN index - the index of PVLAN membership.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_config_del(const u32 *const idx)
{
#if defined(PVLAN_SRC_MASK_ENA)
    vtss_isid_t         isid;
    switch_iter_t       sit;
    pvlan_mgmt_entry_t  ety;
    mesa_pvlan_no_t     zero_based_pvlan_id;
    mesa_rc             rc;
    u32                 max_pvlan_count = port_count_max();
#endif /* defined(PVLAN_SRC_MASK_ENA) */

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return VTSS_RC_ERROR;
    }

    if (!idx) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

#if defined(PVLAN_SRC_MASK_ENA)
    if ((*idx < PVLAN_ID_START) || (*idx > max_pvlan_count)) {
        T_D("Invalid PVLAN Index:%u", *idx);
        return VTSS_RC_ERROR;
    }

    isid = VTSS_ISID_GLOBAL;
    zero_based_pvlan_id = uport2iport(*idx);
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        if (msg_switch_is_local(sit.isid)) {
            isid = sit.isid;
            break;
        }
    }
    if (isid == VTSS_ISID_GLOBAL) {
        T_D("Cannot determine ISID");
        return PVLAN_ERROR_GEN;
    }

    vtss_clear(ety);
    rc = pvlan_mgmt_pvlan_get(isid, zero_based_pvlan_id, &ety, FALSE);
    if (rc != VTSS_RC_OK) {
        T_D("No valid PVLAN %u", *idx);
        return PVLAN_ERROR_ENTRY_NOT_FOUND;
    }

    rc = pvlan_mgmt_pvlan_del(ety.privatevid);
    T_D("DONE(%s)", error_txt(rc));
    return rc;
#else
    return PVLAN_ERROR_UNSUPPORTED;
#endif /* defined(PVLAN_SRC_MASK_ENA) */
}

/**
 * \brief Get PVLAN port isolation configuration
 *
 * To get configuration of the PVLAN port isolation.
 *
 * \param ifindex   [IN]    (key) Port index - the port index of PVLAN isolation.
 *
 * \param entry     [OUT]   The current isolation configuration of the specific PVLAN port.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_isolation_config_get(const vtss_ifindex_t *const ifindex, vtss_appl_pvlan_isolation_conf_t *const entry)
{
    vtss_ifindex_elm_t ife;
    vtss_isid_t        isid;
    mesa_port_no_t     iport;
    mesa_port_list_t   member;
    mesa_rc            rc = VTSS_RC_ERROR;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return rc;
    }

    /* get isid/iport from given IfIndex */
    if (!ifindex || !entry ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT ||
        (iport = ife.ordinal) >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_D("Invalid Input!");
        return rc;
    }

    isid = ife.isid;
    T_D("ISID:%u PORT:%u", isid, iport);

    if ((rc = pvlan_mgmt_isolate_conf_get(isid, member)) == VTSS_RC_OK) {
        entry->isolated = member[iport - VTSS_PORT_NO_START];
    }

    T_D("GET(%s:%u:%u)->%sIsolated", rc == VTSS_RC_OK ? "OK" : "NG", isid, iport, entry->isolated ? "" : "Not");

    return rc;
}

/**
 * \brief Set/Update PVLAN port isolation configuration
 *
 * To modify configuration of the PVLAN port isolation.
 *
 * \param ifindex   [IN]    (key) Port index - the port index of PVLAN isolation.
 *
 * \param entry     [OUT]   The current isolation configuration of the specific PVLAN port.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_isolation_config_set(const vtss_ifindex_t *const ifindex, const vtss_appl_pvlan_isolation_conf_t *const entry)
{
    vtss_ifindex_elm_t ife;
    vtss_isid_t        isid;
    mesa_port_no_t     iport;
    mesa_port_list_t   member;
    mesa_rc            rc = VTSS_RC_ERROR;

    if (!msg_switch_is_primary()) {
        T_D("Not primary switch!");
        return rc;
    }

    /* get isid/iport from given IfIndex */
    if (!ifindex || !entry ||
        vtss_ifindex_decompose(*ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT ||
        (iport = ife.ordinal) >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_D("Invalid Input!");
        return rc;
    }

    isid = ife.isid;
    T_D("ISID:%u PORT:%u", isid, iport);

    if ((rc = pvlan_mgmt_isolate_conf_get(isid, member)) != VTSS_RC_OK) {
        T_D("No valid PVLAN Isolation on ISID:%u", isid);
        return PVLAN_ERROR_ENTRY_NOT_FOUND;
    }

    member[iport] = entry->isolated;
    rc = pvlan_mgmt_isolate_conf_set(isid, member);

    T_D("SET(%s:%u:%u)->%sIsolated", rc == VTSS_RC_OK ? "OK" : "NG", isid, iport, entry->isolated ? "" : "Not");

    return rc;
}

/****************************************************************************/
// Initialize module
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void vtss_appl_pvlan_mib_init(void);
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_pvlan_json_init(void);
#endif /* VTSS_SW_OPTION_JSON_RPC */
extern "C" int pvlan_icli_cmd_register();

/****************************************************************************/
// pvlan_init()
/****************************************************************************/
mesa_rc pvlan_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
#ifdef VTSS_SW_OPTION_ICFG
    mesa_rc     rc;
#endif

    switch (data->cmd) {
    case INIT_CMD_INIT:
        PVLAN_start();
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = PVLAN_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling pvlan_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        vtss_appl_pvlan_mib_init();     /* Register Private VLAN Private-MIB */
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_pvlan_json_init();    /* Register Private VLAN JSON-RPC */
#endif /* VTSS_SW_OPTION_JSON_RPC */
        pvlan_icli_cmd_register();
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
#if defined(PVLAN_SRC_MASK_ENA)
            /* Reset stack configuration */
            PVLAN_default_stack();
#endif
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
            PVLAN_default_switch(isid);
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");

        // Default the stack and switch configuration
#if defined(PVLAN_SRC_MASK_ENA)
        PVLAN_default_stack();
#endif

        PVLAN_default_switch(VTSS_ISID_GLOBAL);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        /* Apply all configuration to switch */
        (void)PI_stack_conf_set(isid);

#if defined(PVLAN_SRC_MASK_ENA)
        (void)SM_stack_conf_set(isid);
#endif /* PVLAN_SRC_MASK_ENA */
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

