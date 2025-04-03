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

// Parts of this file is derived from code with the following copyright:

/***********************************************************
    Copyright 1988, 1989 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/
/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "main.h"
#include "main_conf.hxx"
#include "conf_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#include "eth_link_oam_api.h"
#endif

#include "vtss_snmp_api.h"
#include "vtss_snmp.h"
#include "vtss_snmp_mibs_init.h"
#include "vtss_snmp_linux.h"
#include "vtss_private_trap.hxx"
#include "port_api.h"    /* For port_change_register() */

#include "l2proto_api.h"
#include "control_api.h" /* For control_system_restart_to_str() */
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

//#include "vtss_free_list_api.h"
#include "vtss_avl_tree_api.h"

#include "vtss_os_wrapper_network.h"
#include <arpa/inet.h>

#include "vtss_os_wrapper_snmp.h"

#include "snmp_custom_api.h"

#include "mibContextTable.h"
#include "snmp_mib_redefine.h"
#include "rfc1213_mib2.h"
#ifdef VTSS_SW_OPTION_RMON
#include "rfc2819_rmon.h"
#include "rmon_api.h"
#endif
#ifdef VTSS_SW_OPTION_SMON
#include "rfc2613_smon.h"
#endif
#include "rfc4188_bridge.h"
#include "rfc3635_etherlike.h"

#include "rfc3411_framework.h"
#include "rfc3412_mpd.h"
#ifdef VTSS_SW_OPTION_SMB_SNMP
#include "rfc3414_usm.h"
#include "rfc3415_vacm.h"
#endif /* VTSS_SW_OPTION_SMB_SNMP */

#ifdef VTSS_SW_OPTION_LLDP
#include "dot1ab_lldp.h"
#include "lldp_api.h"
#endif

#include "sysORTable.h"
#ifdef VTSS_SW_OPTION_SMB_SNMP
#include "rfc2674_q_bridge.h"
#ifdef VTSS_SW_OPTION_QOS
#include "rfc4363_p_bridge.h"
#endif /* VTSS_SW_OPTION_QOS */
#include "ieee8021BridgeMib.h"
#include "ieee8021QBridgeMib.h"
#ifdef VTSS_SW_OPTION_DOT1X_ACCT
#include "rfc4670_radiusclient.h"
#endif /* SW_OPTION_DOT1X_ACCT */
#ifdef VTSS_SW_OPTION_LACP
#include "ieee8023_lag_mib.h"
#endif /* VTSS_SW_OPTION_LACP */
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#include "dot3OamMIB.h"
#include "ping_api.h"
#endif
#ifdef VTSS_SW_OPTION_IGMPS
#include "rfc2933_igmp.h"
#endif /* VTSS_SW_OPTION_IGMPS */
#ifdef VTSS_SW_OPTION_IPMC
#include "mgmdMIBObjects.h"
#endif /* VTSS_SW_OPTION_IPMC */
#include "rfc2863_ifmib.h"
#ifdef VTSS_SW_OPTION_DOT1X
#include "ieee8021x_mib.h"
#ifdef VTSS_SW_OPTION_RADIUS
#include "rfc4668_radiusclient.h"
#endif /* VTSS_SW_OPTION_RADIUS */
#endif /* VTSS_SW_OPTION_DOT1X */
#if defined(VTSS_SW_OPTION_POE) || defined(VTSS_SW_OPTION_LLDP_MED)
#include "lldpXMedMIB.h"
#endif // VTSS_SW_OPTION_POE
#include "rfc4133_entity.h"
#include "rfc3636_mau.h"
#endif //VTSS_SW_OPTION_SMB_SNMP

#ifdef VTSS_SW_OPTION_SFLOW
#include "sflow_snmp.h"
#endif

#ifdef VTSS_SW_OPTION_ICFG
#include "snmp_icfg.h"
#include "trap_icfg.h"
#endif

#if defined(VTSS_SW_OPTION_IP)
#include "rfc4292_ip_forward.h"
#include "rfc4293_ip.h"
#endif /* VTSS_SW_OPTION_IP */

#ifdef VTSS_SW_OPTION_MSTP
#include "mstp_api.h"
#include "ieee8021MstpMib.h"
#endif

#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif /* VTSS_SW_OPTION_WEB */

#ifdef VTSS_SW_OPTION_P802_1_AS
#if defined(VTSS_SW_OPTION_PTP)
#include "ieee8021AsTimeSyncMib.h"
#else
#error "Option P802_1_AS not possible without option PTP"
#endif
#endif /* VTSS_SW_OPTION_P802_1_AS */

#ifdef VTSS_SW_OPTION_TSN
#include "tsn_api.h"
#include "ieee8021STMib.h"
#endif

#ifdef VTSS_SW_OPTION_CFM
#include "ieee8021CfmMib.h"
#endif

#include "ieee8021PreemptionMib.h"

#ifdef VTSS_SW_OPTION_QOS_ADV
#include "ieee8021PSFPMib.h"
#endif

#ifdef VTSS_SW_OPTION_FRR
#include "frr_daemon.hxx" /* For frr_has_XXX() */
#endif

#ifdef VTSS_SW_OPTION_FRR_OSPF
#include "rfc4750_ospf.h"
#endif

#ifdef VTSS_SW_OPTION_FRR_RIP
#include "rfc1724_rip2.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
void vtss_snmp_reg_mib_tree();
#ifdef __cplusplus
}
#endif

#include "ifIndex_api.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SNMP

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
snmp_global_t snmp_global;
netsnmp_log_handler *snmp_loghandler;
static vtss_trap_sys_conf_t trap_global;
static bool any_ip4_interface = false;
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
static mesa_rc loam_trap_entry_add(const DGMappingKey key, const DGValue *const value);
static mesa_rc loam_trap_entry_get(const DGMappingKey key, DGValue *value);
static mesa_rc loam_trap_entry_del(const DGMappingKey key);
static dying_gasp_db_trap_entry_t dg_data;
#endif

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "snmp", "SNMP"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_USERS] = {
        "users",
        "Users ",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_TRAP] = {
        "trap",
        "Trap ",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define SNMP_CRIT_ENTER(line) critd_enter(&snmp_global.crit, __FILE__, line)
#define SNMP_CRIT_EXIT()      critd_exit( &snmp_global.crit, __FILE__, __LINE__)

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
struct VtssDgTrapScopeGuard {
    VtssDgTrapScopeGuard(int line)
    {
        critd_enter(&dg_data.crit, __FUNCTION__, line);
    }

    ~VtssDgTrapScopeGuard()
    {
        critd_exit(&dg_data.crit, __FUNCTION__, 0);
    }
};
#endif /* VTSS_SW_OPTION_ETH_LINK_OAM */

struct SnmpLock {
    SnmpLock(int line)
    {
        T_N("Enter line:%d", line);
        SNMP_CRIT_ENTER(line);
    }
    ~SnmpLock()
    {
        T_N("Exit Crit");
        SNMP_CRIT_EXIT();
    }
};

#define CRIT_SCOPE() SnmpLock __lock_guard__(__LINE__)

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#define DG_CRIT_SCOPE() VtssDgTrapScopeGuard __lock_guard__(__LINE__)
#endif

/***************************/
/* Linux Thread variables  */
/***************************/
static vtss_handle_t snmp_agent_thread_handle;
static vtss_thread_t snmp_agent_thread_block;

BOOL SnmpdGetAgentState(void)
{
    BOOL state;
    SNMP_CRIT_ENTER(__LINE__);
    state = snmp_global.mode;
    SNMP_CRIT_EXIT();
    return state;
}

#ifdef SNMP_PRIVATE_MIB_MGMT
oid                 snmp_private_mib_oid[] = {1, 3, 6, 1, 4, 1, SNMP_PRIVATE_MIB_ENTERPRISE, SNMP_PRIVATE_MIB_MGMT, SNMP_PRIVATE_MIB_PRODUCT_ID};
#else
oid                 snmp_private_mib_oid[] = {1, 3, 6, 1, 4, 1, SNMP_PRIVATE_MIB_ENTERPRISE, SNMP_PRIVATE_MIB_PRODUCT_ID /* Placeholder for product ID */};
#endif // SNMP_PRIVATE_MIB_MGMT

/* Thread variables */
static vtss_handle_t snmp_trap_thread_handle;
static vtss_thread_t snmp_trap_thread_block;

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#include "chrono.hxx"
static vtss_handle_t   dying_gasp_timer_thread_handle;
static vtss_thread_t   dying_gasp_timer_thread_block;
static mesa_rc         make_oam_dying_gasp_trap_pdu(vtss_trap_entry_t  *trap_entry);
static mesa_rc         dying_gasp_get_ready_to_send_trap_to_this_entry(vtss_trap_entry_t *trap_entry, BOOL do_rewrite);
#endif

/* values of the generic-trap field in trap PDUs */
typedef enum {
    SNMP_TRAP_TYPE_COLDSTART,
    SNMP_TRAP_TYPE_WARMSTART,
    SNMP_TRAP_TYPE_LINKDOWN,
    SNMP_TRAP_TYPE_LINKUP,
    SNMP_TRAP_TYPE_AUTHFAIL,
    SNMP_TRAP_TYPE_END
} snmp_trap_type_t;

/* Trap interface information */
typedef struct {
    i32    if_idx;     /* ifIndex */
    i32    if_admin;   /* ifAdminStatus */
    i32    if_oper;    /* ifOperStatus */
} snmp_trap_if_info_t;

typedef struct {
    snmp_trap_type_t                trap_type;
    snmp_trap_if_info_t             trap_if_info;
} snmp_trap_buff_t;

typedef struct {
    snmp_vars_trap_entry_t          trap_entry;
} snmp_trap_vars_buff_t;

#define SNMP_TRAP_BUFF_MAX_CNT      (VTSS_ISID_CNT * fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + 2) /* +1 means cold start or warm start, +1 means null buffer */
#define SNMP_TRAP_BUFF_CNT(r,w)     ((r) > (w) ? ((SNMP_TRAP_BUFF_MAX_CNT) - ((r)-(w))) : ((w)-(r)))

static snmp_trap_buff_t snmp_trap_buff[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];
static snmp_trap_vars_buff_t snmp_trap_vars_buff[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];
static int trap_buff_read_idx = 0, trap_buff_write_idx = 0;
static int trap_vars_buff_read_idx = 0, trap_vars_buff_write_idx = 0;

typedef enum {
    EVENT_TYPE_WARM_START,
    EVENT_TYPE_COLD_START,
    EVENT_TYPE_LINK_UP,
    EVENT_TYPE_LINK_DOWN,
    EVENT_TYPE_LLDP,
    EVENT_TYPE_AUTH_FAIL,
    EVENT_TYPE_STP,
    EVENT_TYPE_RMON,
    EVENT_TYPE_ENTITY,
    EVENT_TYPE_DYINGGASP,
    EVENT_TYPE_PTP,
    EVENT_TYPE_END
} event_type_e;

typedef struct {
    snmp_vars_trap_entry_t trap_entry;
    event_type_e event_type;
} event_tbl_t;

#define SNMP_TRAP_OID   {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0}
#define COLD_START_OID  {1, 3, 6, 1, 6, 3, 1, 1, 5, 1}
#define WARM_START_OID  {1, 3, 6, 1, 6, 3, 1, 1, 5, 2}
#define LINKDOWN_OID    {1, 3, 6, 1, 6, 3, 1, 1, 5, 3}
#define LINKUP_OID      {1, 3, 6, 1, 6, 3, 1, 1, 5, 4}
#define AUTH_FAIL_OID   {1, 3, 6, 1, 6, 3, 1, 1, 5, 5}
#define IFINDEX_OID {1, 3, 6, 1, 2, 1, 2, 2, 1, 1, 0}
#define IFADMIN_OID {1, 3, 6, 1, 2, 1, 2, 2, 1, 7, 0}
#define IFOPER_OID  {1, 3, 6, 1, 2, 1, 2, 2, 1, 8, 0}

static bool snmpd_communities_updated = false;
bool is_snmpd_communities_updated()
{
    return snmpd_communities_updated;
}
void set_snmpd_communities_updated(bool v)
{
    snmpd_communities_updated = v;
}

static bool snmpd_traps_updated = false;
bool is_snmpd_traps_updated()
{
    return snmpd_traps_updated;
}
void set_snmpd_traps_updated(bool v)
{
    snmpd_traps_updated = v;
}

static bool snmpd_boot_finished = false;
bool is_snmpd_boot_finished()
{
    CRIT_SCOPE();
    return snmpd_boot_finished;
}
void set_snmpd_boot_finished()
{
    CRIT_SCOPE();
    snmpd_boot_finished = true;
}

static vtss_trap_entry_t *get_next_trap_entry_point(vtss_trap_entry_t *trap_entry);
static vtss_trap_source_t *get_trap_source_point(vtss_trap_source_t *trap_source);

static mesa_rc trap_event_send_snmpTraps(snmp_trap_buff_t *trap_buff)
{
    char name[10] = "UNKNOWN";
    oid ifIndex_oid[] = IFINDEX_OID;
    oid ifAdmin_oid[] = IFADMIN_OID;
    oid ifOper_oid[] = IFOPER_OID;
    int ifTable_len = sizeof(ifIndex_oid) / sizeof(oid);
    struct variable_list trap_var, ifIndex_var, ifAdmin_var, ifOper_var;
    iftable_info_t info;
    snmp_trap_type_t trap_type = trap_buff->trap_type;

    memset(&trap_var, 0, sizeof(trap_var));
    memset(&ifIndex_var, 0, sizeof(struct variable_list));
    memset(&ifAdmin_var, 0, sizeof(struct variable_list));
    memset(&ifOper_var, 0, sizeof(struct variable_list));

    info.ifIndex = trap_buff->trap_if_info.if_idx;

    ifIndex_var.next_variable  = &ifAdmin_var;
    ifAdmin_var.next_variable  = &ifOper_var;
    ifOper_var.next_variable  = NULL;

    ifIndex_oid[ifTable_len - 1] = info.ifIndex;
    ifAdmin_oid[ifTable_len - 1] = info.ifIndex;
    ifOper_oid[ifTable_len - 1]  = info.ifIndex;

    ( void )snmp_set_var_objid( &ifIndex_var, ifIndex_oid, OID_LENGTH(ifIndex_oid));
    ( void )snmp_set_var_objid( &ifAdmin_var, ifAdmin_oid, OID_LENGTH(ifAdmin_oid));
    ( void )snmp_set_var_objid( &ifOper_var, ifOper_oid, OID_LENGTH(ifOper_oid));

    ifIndex_var.type = ASN_INTEGER;
    ifAdmin_var.type = ASN_INTEGER;
    ifOper_var.type  = ASN_INTEGER;

    (void)snmp_set_var_value( &ifIndex_var, (u_char *)&trap_buff->trap_if_info.if_idx,   sizeof(trap_buff->trap_if_info.if_idx));
    (void)snmp_set_var_value( &ifAdmin_var, (u_char *)&trap_buff->trap_if_info.if_admin, sizeof(trap_buff->trap_if_info.if_admin));
    (void)snmp_set_var_value( &ifOper_var,  (u_char *)&trap_buff->trap_if_info.if_oper,  sizeof(trap_buff->trap_if_info.if_oper));

    if ( (trap_type == SNMP_TRAP_TYPE_LINKUP || trap_type == SNMP_TRAP_TYPE_LINKDOWN ) &&
         (FALSE == ifIndex_get(&info) || info.type != IFTABLE_IFINDEX_TYPE_PORT)) {
        return VTSS_RC_ERROR;
    }
    u32 index_len = 0;
    oid index = info.ifIndex;

    /* In the notification, we have to assign our notification OID to
     * the snmpTrapOID.0 object.
     */
    oid objid_snmptrap[] = SNMP_TRAP_OID;
    (void)snmp_set_var_objid(&trap_var, objid_snmptrap, OID_LENGTH(objid_snmptrap));
    trap_var.type = ASN_OBJECT_ID;

    switch (trap_type) {
    case SNMP_TRAP_TYPE_COLDSTART: {
        T_D("SNMP_TRAP_TYPE_COLDSTART");
        strcpy(name, TRAP_NAME_COLD_START);
        oid val_coldstart[] = COLD_START_OID;
        (void)snmp_set_var_value(&trap_var, (u_char *)val_coldstart, OID_LENGTH(val_coldstart) * sizeof(oid));
    }
    break;
    case SNMP_TRAP_TYPE_WARMSTART: {
        T_D("SNMP_TRAP_TYPE_WARMSTART");
        strcpy(name, TRAP_NAME_WARM_START);
        oid val_warmstart[] = WARM_START_OID;
        (void)snmp_set_var_value(&trap_var, (u_char *)val_warmstart, OID_LENGTH(val_warmstart) * sizeof(oid));
    }
    break;
    case SNMP_TRAP_TYPE_LINKUP: {
        T_D("SNMP_TRAP_TYPE_LINKUP");
        strcpy(name, TRAP_NAME_LINK_UP);
        index_len = 1;
        oid val_linkup[] = LINKUP_OID;
        (void)snmp_set_var_value(&trap_var, (u_char *)val_linkup, OID_LENGTH(val_linkup) * sizeof(oid));
        trap_var.next_variable = &ifIndex_var;
    }
    break;
    case SNMP_TRAP_TYPE_LINKDOWN: {
        T_D("SNMP_TRAP_TYPE_LINKDOWN");
        strcpy(name, TRAP_NAME_LINK_DOWN);
        index_len = 1;
        oid val_linkdown[] = LINKDOWN_OID;
        (void)snmp_set_var_value(&trap_var, (u_char *)val_linkdown, OID_LENGTH(val_linkdown) * sizeof(oid));
        trap_var.next_variable = &ifIndex_var;
    }
    break;
    default: {
        T_D("other trap_type:%d", trap_type);
    }
    break;
    }

    vtss_send_v2trap(name, index_len, &index, &trap_var);
    return VTSS_RC_OK;
}

static mesa_rc trap_event_send_vars(snmp_vars_trap_entry_t *trap_entry_vars)
{
    struct variable_list *event_var;
    oid trap_oid[] = SNMP_TRAP_OID;

    // snmp_free_varbind() is used to free this pointer, once used.
    // Therefore we use an SNMP allocation function, rather than VTSS_MALLOC().
    // SNMP_MALLOC_STRUCT() also zeroes out the allocated structure.
    event_var = SNMP_MALLOC_STRUCT(variable_list);

    if (!event_var) {
        T_E("out of memory");
        return VTSS_RC_ERROR;
    }

    event_var->next_variable  = trap_entry_vars->vars;
    trap_entry_vars->vars = event_var;

    /* In the notification, we have to assign our notification OID to
     * the snmpTrapOID.0 object.
     */
    ( void )snmp_set_var_objid( event_var, trap_oid, OID_LENGTH(trap_oid));
    event_var->type           = ASN_OBJECT_ID;
    ( void )snmp_set_var_value( event_var, (u_char *)trap_entry_vars->oid, trap_entry_vars->oid_len * sizeof(oid));

    vtss_send_v2trap(trap_entry_vars->name, trap_entry_vars->index_len, trap_entry_vars->index, event_var);
    return VTSS_RC_OK;
}

void vtss_send_v2trap(const char *name, u32 index_len, oid *index, netsnmp_variable_list *t)
{
    u32 i = 0;
    /* Debug print start */
    char buf[512] = "none";
    int len = 0;
    while (i < index_len && len < sizeof(buf)) {
        len += snprintf(buf + len, sizeof(buf) - len, ".%lu", index[i]);
        i++;
    }
    T_D("Sending trap %s with index %s", name, buf);
    netsnmp_variable_list *p;
    snprint_variable(buf, sizeof(buf), t->name, t->name_length, t);
    p = t->next_variable;
    T_D("%u: %s", i++, buf);
    while (NULL != p) {
        snprint_variable(buf, sizeof(buf), p->name, p->name_length, p);
        p = p->next_variable;
        T_D("%u: %s", i++, buf);
    }
    /* Debug print end */
    if (!SnmpdGetAgentState()) {
        T_D("Trap send ignored as SNMP server disabled");
        return;
    }
    CRIT_SCOPE();
    vtss_trap_source_t trap_source;
    strncpy(trap_source.source_name, name, TRAP_MAX_TABLE_NAME_LEN);
    vtss_trap_source_t *tmp = get_trap_source_point(&trap_source);
    if (!tmp) {
        T_D("Trap filter not configured");
        return;
    }
    u32 a = 0, f = 0;
    vtss_trap_filter_item_t *item;
    BOOL include = FALSE, exclude = FALSE, match;
    u32 mask, maskpos;
    for (i = 0; i < VTSS_TRAP_FILTER_MAX && a < tmp->trap_filter.active_cnt; i++) {
        item = tmp->trap_filter.item[i];
        if (!item) {
            break;
        }
        a++;
        match = FALSE;
        mask = 0x80;
        maskpos = 0;
        if (include && (item->filter_type == SNMPV3_MGMT_VIEW_INCLUDED)) {
            // Already included and type=include; no need to check
            break;
        }
        if (item->index_filter_len <= 1) {
            // Match all filter
            match = TRUE;
        } else {
            match = TRUE;
            for (f = 0; f < item->index_filter_len - 1; f++) {
                if (f >= index_len) {
                    // Filter longer than index; do not match
                    match = FALSE;
                    break;
                }
                if ((item->index_filter[f + 1] == index[f]) || ((maskpos < item->index_mask_len) && ((item->index_mask[maskpos] & mask) == 0))) {
                    // OID item matches - continue checking...
                    if (mask == 1) {
                        mask = 0x80;
                        maskpos++;
                    } else {
                        mask >>= 1;
                    }
                    continue;
                }
                // No match for OID item
                match = FALSE;
                break;
            }
        }
        if (match) {
            if (item->filter_type == SNMPV3_MGMT_VIEW_INCLUDED) {
                // Mark as included by a filter
                include = TRUE;
            } else {
                // Mark as excluded by a filter; stop checking, as this trap will not be sent
                exclude = TRUE;
                break;
            }
        }
    }
    // Send if filter is included by at least one and excluded by none
    if (include && !exclude) {
        T_D("Trap filter ok - sending now!");
        send_v2trap(t);
    } else {
        T_D("Trap filter prevents sending");
    }
}

/**
 * Get OID length (number of items) of private MIB OID
 */
u32 snmp_private_mib_oid_len_get(void)
{
    return OID_LENGTH(snmp_private_mib_oid);
}


/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/
/* Determine if IP address valid */
static BOOL snmp_is_ip_valid(mesa_ip_network_t ip)
{
    if (ip.address.type == MESA_IP_TYPE_NONE) {

        return FALSE;

    } else if (ip.address.type == MESA_IP_TYPE_IPV4) {

        ulong temp_ip_addr = htonl(ip.address.addr.ipv4);
        uchar temp_ip[4];
        mesa_prefix_size_t prefix_temp = ip.prefix_size;

        temp_ip[0] = temp_ip_addr & 0xFF;
        temp_ip[1] = (temp_ip_addr >> 8) & 0xFF;
        temp_ip[2] = (temp_ip_addr >> 16) & 0xFF;
        temp_ip[3] = (temp_ip_addr >> 24) & 0xFF;

        if (temp_ip[0] == 127 || temp_ip[0] > 223) {
            return FALSE;
        }

        for (u8 i = 0; i < 4; i++) {
            if (prefix_temp >= 8) {
                prefix_temp -= 8;
            } else if (prefix_temp > 0) {
                if ((temp_ip[i] & ((0x100 >> prefix_temp) - 1)) > 0) {
                    return FALSE;
                }
                prefix_temp = 0;
            } else {
                if (temp_ip[i] > 0) {
                    return FALSE;
                }
            }
        }

        if (ip.prefix_size > 32) {
            return FALSE;
        }

        if (ip.address.addr.ipv4 == 0 && ip.prefix_size > 0) {
            return FALSE;
        }

        return TRUE;

#ifdef VTSS_SW_OPTION_IPV6
    } else if (ip.address.type == MESA_IP_TYPE_IPV6) {

        BOOL is_zero = TRUE;
        mesa_prefix_size_t prefix_temp = ip.prefix_size;

        if (ip.address.addr.ipv6.addr[0] > 254) {
            return FALSE;
        }

        for (u8 i = 0; i < 16; i++) {
            if (i == 15) {
                if (ip.address.addr.ipv6.addr[i] == 1) {
                    return FALSE;
                }
            }
            if (ip.address.addr.ipv6.addr[i] > 0) {
                is_zero = FALSE;
            }
            if (prefix_temp >= 8) {
                prefix_temp -= 8;
            } else if (prefix_temp > 0) {
                if ((ip.address.addr.ipv6.addr[i] & ((0x100 >> prefix_temp) - 1)) > 0) {
                    return FALSE;
                }
                prefix_temp = 0;
            } else {
                if (ip.address.addr.ipv6.addr[i] > 0) {
                    return FALSE;
                }
            }
        }

        if (ip.prefix_size > 128) {
            return FALSE;
        }

        if (is_zero && ip.prefix_size > 0) {
            return FALSE;
        }

        return TRUE;

#endif /* VTSS_SW_OPTION_IPV6 */
    }

    /* Unknown IP type */
    return FALSE;
}

static int snmp_str_cmp(char *data, char *key)
{
    int cmp;

    if ((cmp = strlen(data) - strlen(key)) != 0) {
        return cmp;
    }

    return strcmp(data, key);
}

static int snmp_ip_cmp(mesa_ip_network_t *data, mesa_ip_network_t *key)
{
    if (data->address.type > key->address.type) {
        return 1;
    } else if (data->address.type < key->address.type) {
        return -1;
    }

    if (data->address.type == MESA_IP_TYPE_NONE) {
        /* ADDR/PREFIX not relevant */
        return 0;
    }

    if (data->address.type == MESA_IP_TYPE_IPV4) {
        if (data->address.addr.ipv4 > key->address.addr.ipv4) {
            return 1;
        } else if (data->address.addr.ipv4 < key->address.addr.ipv4) {
            return -1;
        }
    }

#ifdef VTSS_SW_OPTION_IPV6
    if (data->address.type == MESA_IP_TYPE_IPV6) {
        for (u8 i = 0; i < 16; i++) {
            if (data->address.addr.ipv6.addr[i] > key->address.addr.ipv6.addr[i]) {
                return 1;
            } else if (data->address.addr.ipv6.addr[i] < key->address.addr.ipv6.addr[i]) {
                return -1;
            }
        }
    }
#endif /* VTSS_SW_OPTION_IPV6 */

    /* PREFIX compare is common for IPv4/IPv6 */
    return data->prefix_size > key->prefix_size ? 1 : data->prefix_size < key->prefix_size ? -1 : 0;
}

static BOOL snmp_ip_overlap(mesa_ip_network_t *data, mesa_ip_network_t *key)
{
    if (data->address.type != key->address.type) {
        if (data->address.type == MESA_IP_TYPE_NONE) {
            return TRUE;
        }
        if (key->address.type == MESA_IP_TYPE_NONE) {
            return TRUE;
        }
        return FALSE;
    }

    mesa_prefix_size_t prefix_temp = 0;

    if (data->prefix_size < key->prefix_size) {
        prefix_temp = data->prefix_size;
    } else {
        prefix_temp = key->prefix_size;
    }

    if (data->address.type == MESA_IP_TYPE_IPV4) {
        ulong data_ip_addr = htonl(data->address.addr.ipv4);
        uchar data_ip[4];
        data_ip[0] = data_ip_addr & 0xFF;
        data_ip[1] = (data_ip_addr >> 8) & 0xFF;
        data_ip[2] = (data_ip_addr >> 16) & 0xFF;
        data_ip[3] = (data_ip_addr >> 24) & 0xFF;
        ulong key_ip_addr = htonl(key->address.addr.ipv4);
        uchar key_ip[4];
        key_ip[0] = key_ip_addr & 0xFF;
        key_ip[1] = (key_ip_addr >> 8) & 0xFF;
        key_ip[2] = (key_ip_addr >> 16) & 0xFF;
        key_ip[3] = (key_ip_addr >> 24) & 0xFF;

        for (u8 i = 0; i < 4; i++) {
            if (prefix_temp >= 8) {
                if (data_ip[i] != key_ip[i]) {
                    return FALSE;
                }
                prefix_temp -= 8;
            } else if (prefix_temp > 0) {
                if ((data_ip[i] & ((0x100 >> prefix_temp) - 1)) != (key_ip[i] & ((0x100 >> prefix_temp) - 1))) {
                    return FALSE;
                }
                prefix_temp = 0;
            } else {
                break;
            }
        }
    }

#ifdef VTSS_SW_OPTION_IPV6
    if (data->address.type == MESA_IP_TYPE_IPV6) {
        for (u8 i = 0; i < 16; i++) {
            if (prefix_temp >= 8) {
                if (data->address.addr.ipv6.addr[i] != key->address.addr.ipv6.addr[i]) {
                    return FALSE;
                }
                prefix_temp -= 8;
            } else if (prefix_temp > 0) {
                if ((data->address.addr.ipv6.addr[i] & ((0x100 >> prefix_temp) - 1)) != (key->address.addr.ipv6.addr[i] & ((0x100 >> prefix_temp) - 1))) {
                    return FALSE;
                }
                prefix_temp = 0;
            } else {
                break;
            }
        }
    }
#endif /* VTSS_SW_OPTION_IPV6 */

    return TRUE;
}

static int snmp_smon_stat_entry_changed(snmp_rmon_stat_entry_t *old, snmp_rmon_stat_entry_t *new_)
{
    return (new_->valid != old->valid ||
            new_->ctrl_index != old->ctrl_index ||
            new_->if_index != old->if_index);
}

/* Determine if SNMPv3 communities configuration has changed */
int snmpv3_communities_conf_changed(snmpv3_communities_conf_t *old, snmpv3_communities_conf_t *new_)
{
    return (new_->valid != old->valid ||
            strcmp(new_->community, old->community) ||
            strcmp(new_->communitySecret, old->communitySecret) ||
            snmp_ip_cmp(&new_->sip, &old->sip) != 0);
}

/* Determine if SNMPv3 users configuration has changed */
int snmpv3_users_conf_changed(snmpv3_users_conf_t *old, snmpv3_users_conf_t *new_)
{
    return (new_->valid != old->valid ||
            new_->engineid_len != old->engineid_len ||
            memcmp(new_->engineid, old->engineid, new_->engineid_len > old->engineid_len ? new_->engineid_len : old->engineid_len) ||
            strcmp(new_->user_name, old->user_name) ||
            new_->security_level != old->security_level ||
            new_->auth_protocol != old->auth_protocol ||
            new_->auth_password_encrypted != old->auth_password_encrypted ||
            strcmp(new_->auth_password, old->auth_password) ||
            new_->priv_protocol != old->priv_protocol ||
            new_->priv_password_encrypted != old->priv_password_encrypted ||
            strcmp(new_->priv_password, old->priv_password) ||
            new_->storage_type != old->storage_type ||
            new_->status != old->status);
}

/* Determine if SNMPv3 groups configuration has changed */
int snmpv3_groups_conf_changed(snmpv3_groups_conf_t *old, snmpv3_groups_conf_t *new_)
{
    return (new_->valid != old->valid ||
            strcmp(new_->group_name, old->group_name) ||
            new_->security_model != old->security_model ||
            strcmp(new_->security_name, old->security_name) ||
            new_->storage_type != old->storage_type ||
            new_->status != old->status);
}

/* Determine if SNMPv3 views configuration has changed */
int snmpv3_views_conf_changed(snmpv3_views_conf_t *old, snmpv3_views_conf_t *new_)
{
    return (new_->valid != old->valid ||
            strcmp(new_->view_name, old->view_name) ||
            strcmp(new_->subtree, old->subtree) ||
            new_->view_type != old->view_type ||
            new_->storage_type != old->storage_type ||
            new_->status != old->status);
}

/* Determine if SNMPv3 accesses configuration has changed */
int snmpv3_accesses_conf_changed(snmpv3_accesses_conf_t *old, snmpv3_accesses_conf_t *new_)
{
    return (new_->valid != old->valid ||
            strcmp(new_->group_name, old->group_name) ||
            strcmp(new_->context_prefix, old->context_prefix) ||
            new_->security_model != old->security_model ||
            new_->security_level != old->security_level ||
            new_->context_match != old->context_match ||
            strcmp(new_->read_view_name, old->read_view_name) ||
            strcmp(new_->write_view_name, old->write_view_name) ||
            strcmp(new_->notify_view_name, old->notify_view_name) ||
            new_->storage_type != old->storage_type ||
            new_->status != old->status);
}

/* Get SNMP configuration */
mesa_rc snmp_engine_conf_get(snmp_conf_t *conf)
{
    T_D("enter");
    conf->mode = snmp_global.mode;
    conf->engineid_len = snmpv3_get_engineID(conf->engineid, SNMPV3_MAX_ENGINE_ID_LEN);
    T_D("exit");
    return VTSS_RC_OK;
}

const uchar *default_engine_id(void)
{
    static uchar *the_default_engine_id = NULL;
    if (NULL == the_default_engine_id) {
        mesa_mac_t mac_address;
        (void) conf_mgmt_mac_addr_get((uchar *)&mac_address.addr, 0);
        the_default_engine_id = (uchar *)VTSS_MALLOC(SNMPV3_DEFAULT_ENGINE_ID_LEN);
        the_default_engine_id[0] = 0x80 | ((SNMP_PRIVATE_MIB_ENTERPRISE & 0xff000000) >> 24);
        the_default_engine_id[1]  = (SNMP_PRIVATE_MIB_ENTERPRISE & 0xff0000) >> 16;
        the_default_engine_id[2]  = (SNMP_PRIVATE_MIB_ENTERPRISE & 0xff00) >> 8;
        the_default_engine_id[3]  = (SNMP_PRIVATE_MIB_ENTERPRISE & 0xff);
        the_default_engine_id[4]  = 3; // The engine Id is built from a MAC address (RFC3411)
        the_default_engine_id[5]  = mac_address.addr[0];
        the_default_engine_id[6]  = mac_address.addr[1];
        the_default_engine_id[7]  = mac_address.addr[2];
        the_default_engine_id[8]  = mac_address.addr[3];
        the_default_engine_id[9]  = mac_address.addr[4];
        the_default_engine_id[10] = mac_address.addr[5];
    }
    return the_default_engine_id;
}

/* Get SNMP defaults */
void snmp_default_get(snmp_conf_t *conf)
{
    conf->mode = SNMP_DEFAULT_MODE;
    conf->engineid_len = SNMPV3_DEFAULT_ENGINE_ID_LEN;
    memcpy(conf->engineid, default_engine_id(), conf->engineid_len);
}

/* Set SNMPv3 communities defaults */
void snmpv3_default_communities_get(u32 *conf_num, snmpv3_communities_conf_t *conf)
{
    if (conf_num) {
        *conf_num = 0;
    }
}

/* Set SNMPv3 communities defaults */
void snmpv3_default_communities(void)
{
    /* Read/create SNMPv3 communities configuration */
    ulong                         idx;

    /* Use default values */
    memset(snmp_global.communities_conf, 0x0, sizeof(snmp_global.communities_conf));

    snmpv3_default_communities_get(&snmp_global.communities_conf_num, &snmp_global.communities_conf[0]);
    for (idx = 0; idx < SNMPV3_MAX_COMMUNITIES; idx++) {
        if (snmp_global.communities_conf[idx].valid) {
            if (snmp_global.communities_conf[idx].storage_type != SNMP_MGMT_STORAGE_NONVOLATILE ||
                snmp_global.communities_conf[idx].status != SNMP_MGMT_ROW_ACTIVE) {
                snmp_global.communities_conf[idx].valid = 0;
                snmp_global.communities_conf_num--;
            }
        }
    }
    set_snmpd_communities_updated(true);
}

/* Get SNMPv3 users defaults */
void snmpv3_default_users_get(u32 *conf_num, snmpv3_users_conf_t *conf)
{
    if ( conf_num ) {
#if defined(VTSS_APPL_SNMP_DEFAULT_V3)
        *conf_num = 1;
#else
        *conf_num = 0;
#endif
    }

    if ( conf ) {
#if defined(VTSS_APPL_SNMP_DEFAULT_V3)
        conf->idx = 1;
        conf->valid = 1;
        conf->engineid_len = snmpv3_get_engineID(conf->engineid, SNMPV3_MAX_ENGINE_ID_LEN);
        strcpy(conf->user_name, SNMPV3_DEFAULT_USER);
        conf->security_level = SNMP_MGMT_SEC_LEVEL_NOAUTH;
        conf->auth_protocol = SNMP_MGMT_AUTH_PROTO_NONE;
        conf->auth_password_encrypted = TRUE;
        strcpy(conf->auth_password, "");
        conf->auth_password_len = 0;
        conf->priv_protocol = SNMP_MGMT_PRIV_PROTO_NONE;
        conf->priv_password_encrypted = TRUE;
        strcpy(conf->priv_password, "");
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;
#endif
    }
}

// Getting auth protocol from an NET-SNMP user in the MSCC format
// In    : usmUser - NET-SNMP user entry
// Return: Auth protocol in MSCC format
static vtss_appl_snmp_auth_protocol_t snmpv3_auth_protocol_get(usmUser *usmUser)
{
    if (usmUser == NULL) {
        T_E("Unexpected NULL pointer");
        return SNMP_MGMT_AUTH_PROTO_NONE;
    }

    if (memcmp(usmUser->authProtocol, usmHMACSHA1AuthProtocol, sizeof(usmHMACSHA1AuthProtocol)) == 0) {
        return SNMP_MGMT_AUTH_PROTO_SHA;
    }

    if (memcmp(usmUser->authProtocol, usmHMACMD5AuthProtocol, sizeof(usmHMACMD5AuthProtocol)) == 0) {
        return SNMP_MGMT_AUTH_PROTO_MD5;
    }

    if (memcmp(usmUser->authProtocol, usmNoAuthProtocol, sizeof(usmNoAuthProtocol)) == 0) {
        return SNMP_MGMT_AUTH_PROTO_NONE;
    }

    return SNMP_MGMT_AUTH_PROTO_NONE;
}


// Getting Priv protocol from an NET-SNMP user in the MSCC format
// In    : usmUser - NET-SNMP user entry
// Return: Priv protocol in MSCC format
static vtss_appl_snmp_priv_protocol_t snmpv3_priv_protocol_get(usmUser *usmUser)
{
    if (usmUser == NULL) {
        T_E("Unexpected NULL pointer");
        return SNMP_MGMT_PRIV_PROTO_NONE;
    }

    if (memcmp(usmUser->privProtocol, usmAESPrivProtocol, sizeof(usmAESPrivProtocol)) == 0) {
        return SNMP_MGMT_PRIV_PROTO_AES;
    }

    if (memcmp(usmUser->privProtocol, usmDESPrivProtocol, sizeof(usmDESPrivProtocol)) == 0) {
        return SNMP_MGMT_PRIV_PROTO_DES;
    }

    if (memcmp(usmUser->privProtocol, usmNoPrivProtocol, sizeof(usmNoPrivProtocol)) == 0) {
        return SNMP_MGMT_PRIV_PROTO_NONE;
    }

    return SNMP_MGMT_PRIV_PROTO_NONE;
}

/*******************************************************************-o-******
 * check_secLevel
 *
 * Parameters:
 *   level
 *  *user
 *
 * Returns:
 *  0   On success,
 *  -1  Otherwise.
 *
 * Checks that a given security level is valid for a given user.
 */
static int
check_secLevel(int level, struct usmUser *user)
{

    if (user->userStatus != RS_ACTIVE) {
        return -1;
    }

    if (level == SNMP_SEC_LEVEL_AUTHPRIV
        && (netsnmp_oid_equals(user->privProtocol, user->privProtocolLen,
                               usmNoPrivProtocol,
                               sizeof(usmNoPrivProtocol) / sizeof(oid)) ==
            0)) {
        return 1;
    }
    if ((level == SNMP_SEC_LEVEL_AUTHPRIV
         || level == SNMP_SEC_LEVEL_AUTHNOPRIV)
        &&
        (netsnmp_oid_equals
         (user->authProtocol, user->authProtocolLen, usmNoAuthProtocol,
          sizeof(usmNoAuthProtocol) / sizeof(oid)) == 0)) {
        return 1;
    }

    return 0;
}                               /* end check_secLevel() */

// Convert a NET-SNMP user to MSCC format
// In     : usmUser - NET-SNMP user entry
// In/Out : users_conf - Pointer to where to put the MSCC result
// Return : VTSS_RC_ERROR if user convert failed
static mesa_rc snmpv3_usmUser2user_conf(snmpv3_users_conf_t *users_conf, usmUser *usmUser)
{
    if (users_conf == NULL || usmUser == NULL) {
        T_IG(TRACE_GRP_USERS, "Null");
        return SNMP_ERROR_NULL;
    }

    memset(users_conf, 0, sizeof(snmpv3_users_conf_t));

    // EngineID
    T_NG(TRACE_GRP_USERS, "Engine len:%zu", (usmUser->engineIDLen));
    if (usmUser->engineIDLen >= SNMPV3_MAX_ENGINE_ID_LEN) {
        return SNMPV3_ERROR_ENGINE_ID_LEN_EXCEEDED;
    }
    users_conf->engineid_len = usmUser->engineIDLen;
    memcpy(&users_conf->engineid[0], usmUser->engineID, usmUser->engineIDLen);

    // User Name
    u32 userNameLen = strlen(usmUser->name);
    T_IG(TRACE_GRP_USERS, "User name len:%d", userNameLen);
    if (userNameLen > SNMPV3_MAX_NAME_LEN) {
        return SNMPV3_ERROR_USER_NAME_LEN_EXCEEDED;
    }

    T_IG(TRACE_GRP_USERS, "User name:%s", usmUser->name);
    strncpy(&users_conf->user_name[0], usmUser->name, userNameLen);

    // Auth Protocol
    T_IG(TRACE_GRP_USERS, "AuthKey len:%zu", usmUser->authKeyLen);
    if (usmUser->authKeyLen * 2 > SNMPV3_MAX_SHA_PASSWORD_LEN) {
        return SNMPV3_ERROR_PASSWORD_LEN;
    }

    users_conf->auth_protocol = (u32)snmpv3_auth_protocol_get(usmUser);
    T_IG(TRACE_GRP_USERS, "Protocol get:%d", users_conf->auth_protocol);

    strcpy(&users_conf->auth_password[0], "");
    users_conf->auth_password_len = 0;

    if (users_conf->auth_protocol != SNMP_MGMT_AUTH_PROTO_NONE) {
        if (usmUser->authKey == NULL && (usmUser->authKeyLen != 0)) {
            T_EG(TRACE_GRP_USERS, "NULL");
            return SNMP_ERROR_NULL;
        } else {
            password_as_hex_string((char *) usmUser->authKey, usmUser->authKeyLen, &users_conf->auth_password[0], SNMPV3_MAX_SHA_PASSWORD_LEN);
            users_conf->auth_password_len = usmUser->authKeyLen * 2;
            T_IG(TRACE_GRP_USERS, "AuthKey len:%zu", usmUser->authKeyLen);
            users_conf->auth_password_encrypted = TRUE;
            T_IG(TRACE_GRP_USERS, "Not NULL, pass:%s, len:%d", &users_conf->auth_password[0], users_conf->auth_password_len);
        }
    }

    // Priv Protocol
    T_IG(TRACE_GRP_USERS, "PrivKey len:%zu", usmUser->privKeyLen);
    if (usmUser->privKeyLen * 2 > SNMPV3_MAX_DES_PASSWORD_LEN) {
        return SNMPV3_ERROR_PASSWORD_LEN;
    }

    users_conf->priv_protocol = (u32)snmpv3_priv_protocol_get(usmUser);
    T_IG(TRACE_GRP_USERS, "Protocol get:%d", users_conf->priv_protocol);

    strcpy(&users_conf->priv_password[0], "");
    users_conf->priv_password_len = 0;

    if (users_conf->priv_protocol != SNMP_MGMT_PRIV_PROTO_NONE) {
        if (usmUser->privKey == NULL && (usmUser->privKeyLen != 0)) {
            T_EG(TRACE_GRP_USERS, "NULL");
            return SNMP_ERROR_NULL;
        } else {
            password_as_hex_string((char *) usmUser->privKey, usmUser->privKeyLen, &users_conf->priv_password[0], SNMPV3_MAX_DES_PASSWORD_LEN);
            users_conf->priv_password_len = usmUser->privKeyLen * 2;
            T_IG(TRACE_GRP_USERS, "PrivKey len:%zu", usmUser->privKeyLen);
            users_conf->priv_password_encrypted = TRUE;
            T_IG(TRACE_GRP_USERS, "Not NULL, pass:%s, len:%d", &users_conf->priv_password[0], users_conf->priv_password_len);
        }
    }

    // Storage Type
    users_conf->storage_type = usmUser->userStorageType;

    // Status
    users_conf->status = usmUser->userStatus;

    // Security level
    if (check_secLevel(SNMP_SEC_LEVEL_AUTHPRIV, usmUser) == 0) {
        users_conf->security_level = SNMP_MGMT_SEC_LEVEL_AUTHPRIV;
    } else if (check_secLevel(SNMP_SEC_LEVEL_AUTHNOPRIV, usmUser) == 0) {
        users_conf->security_level = SNMP_MGMT_SEC_LEVEL_AUTHNOPRIV;
    } else {
        users_conf->security_level = SNMP_MGMT_SEC_LEVEL_NOAUTH;
    }

    // OK - Now it should be valid
    users_conf->valid = TRUE;

    return VTSS_RC_OK;
}

mesa_rc snmpv3_engine_users_get(snmpv3_users_conf_t *conf, BOOL next)
{
    usmUser *usmUser;
    usmUser = usm_get_userList(); // Get NET-SNMP users in linked list

    if (usmUser == NULL) {
        T_IG(TRACE_GRP_USERS, "User list is empty");
        return SNMPV3_ERROR_NO_USER_FOUND; // Signal no user found
    }

    // Get this requested entry
    if (!next) {
        T_IG(TRACE_GRP_USERS, "Get the requested user - name:%s", conf->user_name);
        usmUser = usm_get_user(conf->engineid, conf->engineid_len, conf->user_name);
        if (usmUser == NULL) {
            T_IG(TRACE_GRP_USERS, "User Data is NULL");
            return SNMPV3_ERROR_NO_USER_FOUND; // Signal no user found
        }
        return snmpv3_usmUser2user_conf(conf, usmUser);
    }

    // Get next
    int compare = 0;
    u32 usmUserNameLen, confUserNameLen;
    confUserNameLen = strlen(conf->user_name);

    if (!strcmp(conf->user_name, SNMPV3_CONF_ACESS_GETFIRST)) {
        T_IG(TRACE_GRP_USERS, "Return user:%s", usmUser->name);
        return snmpv3_usmUser2user_conf(conf, usmUser);
    }
    do  {
        if (usmUser->engineIDLen < conf->engineid_len) {
            continue;
        }
        if (usmUser->engineIDLen == conf->engineid_len) {
            if ((compare = memcmp(usmUser->engineID, conf->engineid, conf->engineid_len)) < 0) {
                continue;
            }
            if (compare == 0) {
                usmUserNameLen = strlen(usmUser->name);
                if (usmUserNameLen < confUserNameLen) {
                    continue;
                }
                if (usmUserNameLen == confUserNameLen) {
                    if ((compare = strcmp(usmUser->name, conf->user_name)) < 0) {
                        continue;
                    }
                    if (compare == 0) {
                        // Return next
                        continue;
                    }
                }
            }
        }
        break;
    } while ((usmUser = usmUser->next));

    if (usmUser) {
        T_IG(TRACE_GRP_USERS, "Return user:%s", usmUser->name);
        return snmpv3_usmUser2user_conf(conf, usmUser);
    } else {
        return SNMPV3_ERROR_NO_USER_FOUND; // Signal no user found
    }
}

// Add/Remove/Modify a net-snmp user
// IN : conf - User configuration
//      add  - true to add user conf to user list.
static mesa_rc snmpv3_engine_users_set(snmpv3_users_conf_t *conf, BOOL is_add)
{
    mesa_rc rc = VTSS_RC_OK;
    if (is_add && (conf->status != SNMP_MGMT_ROW_ACTIVE)) {
        return ((mesa_rc) SNMP_ERROR_ROW_STATUS_INCONSISTENT);
    }

    T_DG(TRACE_GRP_USERS, "is_add: %d", is_add);
    usmUser *usmUser;

#ifdef __TODO__ //What is this doing??
#ifdef VTSS_SW_OPTION_SMON
    smon_create_stat_default_entry();
#endif
#endif

    if (is_add) {
        // Add the user to list
        char buf[512];
        char engineid_txt[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];

        //
        // Auth password
        //
        char auth_password_buf[128];
        strcpy(auth_password_buf, "");
        T_DG(TRACE_GRP_USERS, "conf->auth_protocol: %d", conf->auth_protocol);
        if (conf->auth_protocol != SNMP_MGMT_AUTH_PROTO_NONE) {
            if (conf->auth_protocol == SNMP_MGMT_AUTH_PROTO_MD5) {
                strncat(auth_password_buf, " MD5 ", sizeof(auth_password_buf) - 1);
            }

            if (conf->auth_protocol == SNMP_MGMT_AUTH_PROTO_SHA) {
                strncat(auth_password_buf, " SHA ", sizeof(auth_password_buf) - 1);
            }

            if (conf->auth_password_encrypted) {
                //  See http://net-snmp.sourceforge.net/docs/man/snmpd.conf.html
                strncat(auth_password_buf, "-l 0x", sizeof(auth_password_buf) - 1);
                strncat(auth_password_buf, conf->auth_password, sizeof(auth_password_buf) - 1);
            } else {
                strncat(auth_password_buf, "'", sizeof(auth_password_buf) - 1);
                strncat(auth_password_buf, conf->auth_password, sizeof(auth_password_buf) - 1);
                strncat(auth_password_buf, "'", sizeof(auth_password_buf) - 1);
            }
        }

        //
        // priv password
        //
        char priv_password_buf[128];
        strcpy(priv_password_buf, "");
        T_DG(TRACE_GRP_USERS, "conf->priv_protocol: %d", conf->priv_protocol);
        if (conf->priv_protocol != SNMP_MGMT_PRIV_PROTO_NONE) {
            if (conf->priv_protocol == SNMP_MGMT_PRIV_PROTO_DES) {
                strncat(priv_password_buf, " DES ", sizeof(priv_password_buf) - 1);
            }

            if (conf->priv_protocol == SNMP_MGMT_PRIV_PROTO_AES) {
                strncat(priv_password_buf, " AES ", sizeof(priv_password_buf) - 1);
            }

            if (conf->priv_password_encrypted) {
                // See http://net-snmp.sourceforge.net/docs/man/snmpd.conf.html
                strncat(priv_password_buf, "-l 0x", sizeof(priv_password_buf) - 1);
                strncat(priv_password_buf, conf->priv_password, sizeof(priv_password_buf) - 1);
            } else {
                strncat(priv_password_buf, "'", sizeof(priv_password_buf) - 1);
                strncat(priv_password_buf, conf->priv_password, sizeof(priv_password_buf) - 1);
                strncat(priv_password_buf, "'", sizeof(priv_password_buf) - 1);
            }
        }

        /* Format: -e engineid username (MD5|SHA) passphrase (DES|AES) passphrase */
        snprintf(buf, sizeof(buf) - 1, "-e 0x%s %s%s%s",
                 misc_engineid2str(engineid_txt, conf->engineid, conf->engineid_len),
                 conf->user_name,
                 auth_password_buf,
                 priv_password_buf);
        T_IG(TRACE_GRP_USERS, "Buf:%s", buf);
        usm_parse_create_usmUser(NULL, buf);
    } else {
        // Remove the user from the list
        usmUser = usm_get_user(conf->engineid, conf->engineid_len, conf->user_name);
        if (usmUser != NULL) {
            T_IG(TRACE_GRP_USERS, "User found - removing from list, name:%s", usmUser->name);
            (void) usm_remove_user(usmUser);
            usmUser->prev = NULL;
            usmUser->next = NULL;
            (void) usm_free_user(usmUser);
        } else {
            rc = SNMPV3_ERROR_USER_NOT_EXIST;
        }
    }

    /* SNMPv3 users used by SNMPv3 traps, so needs update when they are changed */
    set_snmpd_traps_updated(true);

    T_DG(TRACE_GRP_USERS, "exit");
    return rc;
}

// Add default use to net-snmp
mesa_rc snmpv3_default_users()
{
    u32        num;
    // Add the default user
    T_D("Adding default user");
    snmpv3_default_users_get(&num, NULL);
    if (num > 0) {
        snmpv3_users_conf_t user;
        snmpv3_default_users_get(NULL, &user);
        VTSS_RC(snmpv3_engine_users_set(&user, TRUE));
    }
    return VTSS_RC_OK;
}

/* Get SNMPv3 groups defaults */
void snmpv3_default_groups_get(u32 *conf_num, snmpv3_groups_conf_t *conf)
{
    if ( conf_num ) {
#if defined(VTSS_APPL_SNMP_DEFAULT_V3)
        *conf_num = 5;
#else
        *conf_num = 4;
#endif
    }

    if (conf) {
        conf->valid = 1;
        conf->security_model = SNMP_MGMT_SEC_MODEL_SNMPV1;
        strcpy(conf->security_name, SNMP_DEFAULT_RO_COMMUNITY);
        strcpy(conf->group_name, SNMPV3_DEFAULT_RO_GROUP);
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;

        conf++;
        conf->valid = 1;
        conf->security_model = SNMP_MGMT_SEC_MODEL_SNMPV1;
        strcpy(conf->security_name, SNMP_DEFAULT_RW_COMMUNITY);
        strcpy(conf->group_name, SNMPV3_DEFAULT_RW_GROUP);
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;

        conf++;
        conf->valid = 1;
        conf->security_model = SNMP_MGMT_SEC_MODEL_SNMPV2C;
        strcpy(conf->security_name, SNMP_DEFAULT_RO_COMMUNITY);
        strcpy(conf->group_name, SNMPV3_DEFAULT_RO_GROUP);
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;

        conf++;
        conf->valid = 1;
        conf->security_model = SNMP_MGMT_SEC_MODEL_SNMPV2C;
        strcpy(conf->security_name, SNMP_DEFAULT_RW_COMMUNITY);
        strcpy(conf->group_name, SNMPV3_DEFAULT_RW_GROUP);
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;

#if defined(VTSS_APPL_SNMP_DEFAULT_V3)
        conf++;
        conf->valid = 1;
        conf->security_model = SNMP_MGMT_SEC_MODEL_USM;
        strcpy(conf->security_name, SNMPV3_DEFAULT_USER);
        strcpy(conf->group_name, SNMPV3_DEFAULT_RW_GROUP);
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;
#endif
    }
}

mesa_rc snmpv3_engine_groups_get(snmpv3_groups_conf_t *conf, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;
    int     compare = 0;
    BOOL    found = FALSE;
    int     namelen = strlen(conf->security_name);

    T_D("enter %s %d", conf->security_name, next);

    vacm_scanGroupInit();

    struct vacm_groupEntry *gp;

    while ((gp = vacm_scanGroupNext())) {
        if (next && (!strcmp(conf->security_name, SNMPV3_CONF_ACESS_GETFIRST))) {
            break;
        }
        if (gp->securityModel < conf->security_model) {
            continue;
        }
        if (gp->securityModel == conf->security_model) {
            if (gp->securityName[0] < namelen) {
                continue;
            }
            if (gp->securityName[0] == namelen) {
                if ((compare = strcmp(gp->securityName + 1, conf->security_name)) < 0) {
                    continue;
                }
                if (compare == 0) {
                    found = TRUE;
                    if (next) {
                        continue;
                    }
                }
            }
        }
        break;
    }

    if (gp && (next || found)) {
        T_D("found %s %d", gp->securityName + 1, next);
        conf->valid = true;
        conf->security_model = gp->securityModel;
        strcpy(conf->security_name, gp->securityName + 1);
        strcpy(conf->group_name, gp->groupName);
        conf->storage_type = gp->storageType;
        conf->status = gp->status;
    } else {
        rc = VTSS_RC_ERROR;
    }
    T_D("exit %d", rc);
    return rc;
}

mesa_rc snmpv3_engine_groups_set(snmpv3_groups_conf_t *conf, BOOL is_add)
{
    struct vacm_groupEntry  *gp;

    if (is_add && (conf->status != SNMP_MGMT_ROW_ACTIVE)) {
        return ((mesa_rc) SNMP_ERROR_ROW_STATUS_INCONSISTENT);
    }

    if (is_add) {
        gp = vacm_createGroupEntry(conf->security_model, conf->security_name);
        if (gp) {
            strcpy(gp->groupName, conf->group_name);
            gp->storageType = conf->storage_type;
            gp->status = conf->status;
            free(gp->reserved); // Was created with calloc() in call to vacm_createGroupEntry(), so don't use VTSS_FREE()
            gp->reserved = NULL;
        } else {
            return VTSS_RC_ERROR;
        }
    } else {
        vacm_destroyGroupEntry(conf->security_model, conf->security_name);
    }

    return VTSS_RC_OK;
}

void snmpv3_default_groups(void)
{
    snmpv3_groups_conf_t conf, *def_conf, *ptr;
    u32        num;
    i32        i = 0;

    snmpv3_default_groups_get( &num, NULL);
    if ((VTSS_MALLOC_CAST(def_conf, sizeof(conf) * num)) == NULL) {
        T_D("Malloc error snmpv3_groups_conf_t");
        return;
    }
    snmpv3_default_groups_get ( NULL, def_conf);
    for (i = 0, ptr = def_conf; i < (i32)num && ptr != NULL; i++, ptr++) {
        if (ptr->valid) {
            if (ptr->storage_type == SNMP_MGMT_STORAGE_NONVOLATILE &&
                ptr->status == SNMP_MGMT_ROW_ACTIVE) {
                snmpv3_engine_groups_set(ptr, TRUE);
            }
        }
    }
    VTSS_FREE(def_conf);
}

/* Set SNMPv3 views defaults */
void snmpv3_default_views_get(u32 *conf_num, snmpv3_views_conf_t *conf)
{
    char default_subtree[] = ".1";

    if ( conf_num ) {
        *conf_num = 1;
    }

    if ( conf ) {
        conf->valid = 1;
        strcpy(conf->view_name, SNMPV3_DEFAULT_VIEW);
        strcpy(conf->subtree, default_subtree);
        conf->view_type = SNMPV3_MGMT_VIEW_INCLUDED;
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;
    }
}

BOOL str2oid(const char *name, oid *oidSubTree,  u32 *oid_len, u8 *oid_mask, u32 *oid_mask_len)
{
    size_t      len = strlen(name);
    char const  *value_char;
    int         num = 0, dot_flag = FALSE;
    u32         i, mask = 0x80, maskpos = 0;

    value_char = name;
    *oid_len = *oid_mask_len = 0;

    //check if OID format .x.x.x
    for (i = 0; i < len; i++) {
        if (((value_char[i] != '.') && (value_char[i] != '*')) &&
            (value_char[i] < '0' || value_char[i] > '9')) {
            return FALSE;
        }
        if (value_char[i] == '*') {
            if (i == 0 || value_char[i - 1] != '.') {
                return FALSE;
            }
        }
        if (value_char[i] == '.') {
            if (dot_flag) { //double dot
                return FALSE;
            }
            dot_flag = TRUE;
            num++;
            if (num > SNMP_MGMT_MAX_OID_LEN - 1) { // Only 127 can be handled by net-snmp as it is prepended with length
                return FALSE;
            }
        } else {
            dot_flag = FALSE;
        }
    }
    *oid_mask_len = (num + 7) / 8;
    *oid_len = num + 1;
    oid_mask[maskpos] = 0;

    /* convert OID string (RFC1447)
       Each bit of this bit mask corresponds to the (8*i - 7)-th
       sub-identifier, and the least significant bit of the i-th
       octet of this octet string corresponding to the (8*i)-th
       sub-identifier, where i is in the range 1 through 16. */
    oidSubTree[0] = num;
    for (i = 0; i < num; i++) {
        if (!memcmp(value_char, ".*", 2)) {
            oidSubTree[i + 1] = 0;
            oid_mask[maskpos] &= (~mask);
            value_char = value_char + 2;
        } else {
            oid_mask[maskpos] |= mask;
            (void) sscanf(value_char++, "." VPRIlu, &oidSubTree[i + 1]);
        }
        T_I("value_char:%s, oid_mask:0x%X, maskpos:%d, mask:0x%X", value_char, oid_mask[maskpos], maskpos, mask);

        if (i == num - 1) {
            break; //last OID node
        }
        while (*value_char != '.') {
            value_char++;
        }

        if (mask == 1) {
            mask = 0x80;
            maskpos++;
            oid_mask[maskpos] = 0;
        } else {
            mask >>= 1;
        }
    }
    T_I("mask:0x%X", mask);
    return TRUE;
}

static BOOL parse2oid(char *subtree, oid *oidSubTree,  u32 *oid_len, u8 *oid_mask, u32 *oid_mask_len)
{
    char                        num_oid[256], *cp = num_oid;
    mibContextTable_entry_t     entry;

    T_D("subtree = %s", subtree);
    if (isalpha(subtree[1]))  {
        memset(&entry, 0, sizeof(entry));
        (void)misc_parse_oid_prefix(subtree, entry.descr);
        T_D("prefix = %s", 0 == entry.descr[0] ? "no prefix" : entry.descr);
        if (entry.descr[0]) {
            cp = subtree + 1 + strlen(entry.descr);
            if ( 0 != mibPrefixTableEntry_get_by_descr(&entry)) {
                return FALSE;
            }
            (void)snprintf(num_oid, sizeof(num_oid), "%s%s",
                           misc_oid2str((ulong *)entry.oid_, entry.oid_len, NULL, 0), cp);
        } else {
            return FALSE;
        }
    } else {
        (void)snprintf(num_oid, sizeof(num_oid), "%s", subtree);
    }

    return str2oid(num_oid, oidSubTree, oid_len, oid_mask, oid_mask_len);
}

mesa_rc snmpv3_engine_views_get(snmpv3_views_conf_t *conf, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;
    int     compare = 0;
    BOOL    found = FALSE;
    int     namelen = strlen(conf->view_name);
    oid     oidSubTree[SNMP_MGMT_MAX_OID_LEN];
    u32     oid_len = 0, oid_mask_len = 0;
    u8      oid_mask[SNMP_MGMT_MAX_SUBTREE_LEN];

    T_D("enter %s %d", conf->view_name, next);

    if (strcmp(conf->view_name, SNMPV3_CONF_ACESS_GETFIRST) &&
        (!parse2oid(conf->subtree, oidSubTree, &oid_len, oid_mask, &oid_mask_len))) {
        T_I("SNMP_ERROR_PARM");
        return SNMP_ERROR_PARM;
    }

    vacm_scanViewInit();

    struct vacm_viewEntry *vp;
    while ((vp = vacm_scanViewNext())) {
        if (next && (!strcmp(conf->view_name, SNMPV3_CONF_ACESS_GETFIRST))) {
            break;
        }
        if (vp->viewName[0] < namelen) {
            continue;
        }
        if (vp->viewName[0] == namelen) {
            if ((compare = strcmp(vp->viewName + 1, conf->view_name)) < 0) {
                continue;
            }
            if (compare == 0) {
                if (vp->viewSubtreeLen < oid_len) {
                    continue;
                }
                if (vp->viewSubtreeLen == oid_len) {
                    char subtree_str[SNMP_MGMT_MAX_SUBTREE_STR_LEN + 1];
                    strcpy(subtree_str, misc_oid2str(vp->viewSubtree + 1, vp->viewSubtreeLen - 1, oid_mask, oid_mask_len));
                    if ((compare = strcmp(subtree_str, conf->subtree)) < 0) {
                        continue;
                    }
                    T_N("Compare:%d", compare);
                    if (compare == 0) {
                        found = TRUE;
                        if (next) {
                            continue;
                        }
                    }
                }
            }
        }
        break;
    }

    T_D("found %d, next:%d", found, next);
    if (vp && (next || found)) {
        T_D("found %s %d", vp->viewName + 1, next);
        conf->valid = true;
        strcpy(conf->view_name, vp->viewName + 1);
        strcpy(conf->subtree, misc_oid2str(vp->viewSubtree + 1, vp->viewSubtreeLen - 1, vp->viewMask, vp->viewMaskLen));
        conf->view_type = vp->viewType == SNMP_VIEW_INCLUDED ? SNMPV3_MGMT_VIEW_INCLUDED : SNMPV3_MGMT_VIEW_EXCLUDED;
        conf->storage_type = vp->viewStorageType;
        conf->status = vp->viewStatus;
    } else {
        rc = VTSS_RC_ERROR;
    }
    T_D("exit %d", rc);
    return rc;
}

static mesa_rc snmpv3_engine_views_set(snmpv3_views_conf_t *conf, BOOL is_add)
{
    oid                         oidSubTree[SNMP_MGMT_MAX_OID_LEN];
    u32                         oid_len, oid_mask_len;
    u8                          oid_mask[SNMP_MGMT_MAX_SUBTREE_LEN];
    struct vacm_viewEntry       *vp;

    if (is_add && (conf->status != SNMP_MGMT_ROW_ACTIVE)) {
        return ((mesa_rc) SNMP_ERROR_ROW_STATUS_INCONSISTENT);
    }

    if (!parse2oid(conf->subtree, oidSubTree, &oid_len, oid_mask, &oid_mask_len)) {
        return SNMP_ERROR_PARM;
    }

    if (is_add) {
        vp = vacm_createViewEntry(conf->view_name, oidSubTree + 1, oid_len - 1);
        if (vp) {
            memcpy(vp->viewMask, oid_mask, oid_mask_len);
            vp->viewMaskLen = oid_mask_len;
            vp->viewType = SNMPV3_MGMT_VIEW_INCLUDED == conf->view_type ? SNMP_VIEW_INCLUDED : SNMP_VIEW_EXCLUDED;
            vp->viewStorageType = conf->storage_type;
            vp->viewStatus = conf->status;
            free(vp->reserved); // Was created with calloc() in call to vacm_createViewEntry(), so don't use VTSS_FREE()
            vp->reserved = NULL;
        } else {
            return VTSS_RC_ERROR;
        }
    } else {
        vacm_destroyViewEntry(conf->view_name, oidSubTree, oid_len);
    }

    return VTSS_RC_OK;
}

void snmpv3_default_views(void)
{
    snmpv3_views_conf_t conf, *def_conf, *ptr;
    u32        num;
    u32        i = 0;
    snmpv3_default_views_get( &num, NULL);
    if ((VTSS_MALLOC_CAST(def_conf, sizeof(conf) * num)) == NULL) {
        T_D("Malloc error snmpv3_groups_conf_t");
        return;
    }
    snmpv3_default_views_get ( NULL, def_conf);
    for (i = 0, ptr = def_conf; i < (i32)num && ptr != NULL; i++, ptr++) {
        if (ptr->valid) {
            if (ptr->storage_type == SNMP_MGMT_STORAGE_NONVOLATILE &&
                ptr->status == SNMP_MGMT_ROW_ACTIVE) {
                snmpv3_engine_views_set(ptr, TRUE);
            }
        }
    }

    VTSS_FREE(def_conf);
}

/* Set SNMPv3 accesses defaults */
void snmpv3_default_accesses_get(u32 *conf_num, snmpv3_accesses_conf_t *conf)
{
    if ( conf_num ) {
        *conf_num = 2;
    }

    if ( conf ) {
        conf->valid = 1;
        strcpy(conf->group_name, SNMPV3_DEFAULT_RO_GROUP);
        strcpy(conf->context_prefix, "");
        conf->security_model = SNMP_MGMT_SEC_MODEL_ANY;
        conf->security_level = SNMP_MGMT_SEC_LEVEL_NOAUTH;
        conf->context_match = SNMPV3_MGMT_CONTEX_MATCH_EXACT;
        strcpy(conf->read_view_name, SNMPV3_DEFAULT_VIEW);
        strcpy(conf->write_view_name, SNMPV3_NONAME);
        strcpy(conf->notify_view_name, SNMPV3_NONAME);
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;

        conf++;
        conf->valid = 1;
        strcpy(conf->group_name, SNMPV3_DEFAULT_RW_GROUP);
        strcpy(conf->context_prefix, "");
        conf->security_model = SNMP_MGMT_SEC_MODEL_ANY;
        conf->security_level = SNMP_MGMT_SEC_LEVEL_NOAUTH;
        conf->context_match = SNMPV3_MGMT_CONTEX_MATCH_EXACT;
        strcpy(conf->read_view_name, SNMPV3_DEFAULT_VIEW);
        strcpy(conf->write_view_name, SNMPV3_DEFAULT_VIEW);
        strcpy(conf->notify_view_name, SNMPV3_NONAME);
        conf->storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
        conf->status = SNMP_MGMT_ROW_ACTIVE;
    }
}

mesa_rc snmpv3_engine_accesses_get(snmpv3_accesses_conf_t *conf, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;
    int     compare = 0;
    BOOL    found = FALSE;
    int     namelen = strlen(conf->group_name);
    int     prefixlen = strlen(conf->context_prefix);

    T_D("enter %s %d", conf->group_name, next);

    vacm_scanAccessInit();

    struct vacm_accessEntry *ap;
    while ((ap = vacm_scanAccessNext())) {
        if (next && (!strcmp(conf->group_name, SNMPV3_CONF_ACESS_GETFIRST))) {
            break;
        }
        if (ap->groupName[0] < namelen) {
            continue;
        }
        if (ap->groupName[0] == namelen) {
            if ((compare = strcmp(ap->groupName + 1, conf->group_name)) < 0) {
                continue;
            }
            if (compare == 0) {
                if (ap->contextPrefix[0] < prefixlen) {
                    continue;
                }
                if (ap->contextPrefix[0] == prefixlen) {
                    if ((compare = strcmp(ap->contextPrefix + 1, conf->context_prefix)) < 0) {
                        continue;
                    }
                    if (compare == 0) {
                        if (ap->securityModel < conf->security_model) {
                            continue;
                        }
                        if (ap->securityModel == conf->security_model) {
                            if (ap->securityLevel < conf->security_level) {
                                continue;
                            }
                            if (ap->securityLevel == conf->security_level) {
                                found = TRUE;
                                if (next) {
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
    }

    if (ap && (next || found)) {
        T_D("found %s %d", ap->groupName + 1, next);
        conf->valid = true;
        strcpy(conf->group_name, ap->groupName + 1);
        strcpy(conf->context_prefix, ap->contextPrefix + 1);
        conf->security_model = ap->securityModel;
        conf->security_level = ap->securityLevel;
        conf->context_match = ap->contextMatch;
        strcpy(conf->read_view_name, ap->views[VACM_VIEW_READ]);
        strcpy(conf->write_view_name, ap->views[VACM_VIEW_WRITE]);
        strcpy(conf->notify_view_name, ap->views[VACM_VIEW_NOTIFY]);
        conf->storage_type = ap->storageType;
        conf->status = ap->status;
    } else {
        rc = VTSS_RC_ERROR;
    }
    T_D("exit %d", rc);
    return rc;
}

mesa_rc snmpv3_engine_accesses_set(snmpv3_accesses_conf_t *conf, BOOL is_add)
{
    struct vacm_accessEntry     *ap;

    if (is_add && (conf->status != SNMP_MGMT_ROW_ACTIVE)) {
        return ((mesa_rc) SNMP_ERROR_ROW_STATUS_INCONSISTENT);
    }

    if (is_add) {
        ap = vacm_createAccessEntry(conf->group_name, conf->context_prefix,
                                    conf->security_model, conf->security_level);
        if (ap) {
            ap->contextMatch = conf->context_match;
            strcpy(ap->views[VACM_VIEW_READ], conf->read_view_name);
            strcpy(ap->views[VACM_VIEW_WRITE], conf->write_view_name);
            strcpy(ap->views[VACM_VIEW_NOTIFY], conf->notify_view_name);
            ap->storageType = conf->storage_type;
            ap->status = conf->status;
            free(ap->reserved); // Was created with calloc() in call to vacm_createAccessEntry(), so don't use VTSS_FREE()
            ap->reserved = NULL;
        } else {
            return VTSS_RC_ERROR;
        }
    } else {
        vacm_destroyAccessEntry(conf->group_name, conf->context_prefix,
                                conf->security_model, conf->security_level);
    }

    return VTSS_RC_OK;
}

void snmpv3_default_accesses(void)
{
    snmpv3_accesses_conf_t conf, *def_conf, *ptr;
    u32        num;
    i32        i = 0;

    snmpv3_default_accesses_get( &num, NULL);
    if ((VTSS_MALLOC_CAST(def_conf, sizeof(conf) * num)) == NULL) {
        T_D("Malloc error snmpv3_accesses_conf_t");
        return;
    }
    snmpv3_default_accesses_get ( NULL, def_conf);
    for (i = 0, ptr = def_conf; i < (i32)num && ptr != NULL; i++, ptr++) {
        if (ptr->valid) {
            if (ptr->storage_type == SNMP_MGMT_STORAGE_NONVOLATILE &&
                ptr->status == SNMP_MGMT_ROW_ACTIVE) {
                snmpv3_engine_accesses_set(ptr, TRUE);
            }
        }
    }
    VTSS_FREE(def_conf);
}

/* Determine if SNMP configuration has changed */
int snmp_conf_changed(snmp_conf_t *old, snmp_conf_t *new_)
{
    return (new_->mode != old->mode
            || new_->engineid_len != old->engineid_len
            || memcmp(new_->engineid, old->engineid, new_->engineid_len > old->engineid_len ? new_->engineid_len : old->engineid_len)
           );
}

static void snmp_smon_conf_engine_set(void)
{
    /* Set to the snmp engine only when stack role is primary switch */
    if (!msg_switch_is_primary()) {
        return;
    }

#ifdef VTSS_SW_OPTION_SMON
    smon_create_stat_default_entry();
#endif
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/
/* SNMP error text */
extern "C"
const char *snmp_error_txt(snmp_error_t rc)
{
    switch (rc) {
    case SNMP_ERROR_GEN:
        return "SNMP generic error";

    case SNMP_ERROR_PARM:
        return "SNMP parameter error";

    case SNMP_ERROR_STACK_STATE:
        return "SNMP stack state error";

    case SNMP_ERROR_SMON_STAT_TABLE_FULL:
        return "SNMP statistics table full";

    case SNMP_ERROR_ROW_STATUS_INCONSISTENT:
        return "SNMP Row status inconsistent";

    case SNMPV3_ERROR_ENGINE_ID_LEN_EXCEEDED:
        return "Engine ID exceeds maximum length.";

    case SNMPV3_ERROR_USER_NAME_LEN_EXCEEDED:
        return "User name exceeds maximum length.";

    case SNMPV3_ERROR_PASSWORD_LEN:
        return "Invalid password length.";

    case SNMPV3_ERROR_NO_USER_FOUND:
        return "No such user found.";

    case SNMPV3_ERROR_COMMUNITY_TOO_LONG:
        return "Community name too long.";

    case SNMPV3_ERROR_SEC_NAME_TOO_LONG:
        return "Security name too long.";

    case SNMPV3_ERROR_COMMUNITIES_IP_OVERLAP:
        return "Community has overlapping IP.";

    case SNMPV3_ERROR_COMMUNITIES_IP_INVALID:
        return "Community has invalid IP address or prefix length.";

    case SNMP_ERROR_NULL:
        return "SNMP unexpected NULL pointer";

    case SNMP_TRAP_NO_SUCH_TRAP:
        return "SNMP-TRAP: No such trap";

    case SNMP_TRAP_NO_SUCH_SUBSCRIPTION:
        return "SNMP-TRAP: No such trap subscription";

    case SNMP_TRAP_FILTER_TABLE_FULL:
        return "SNMP-TRAP: Filter table is full for trap source.";

    case SNMPV3_ERROR_COMMUNITY_ALREADY_EXIST:
        return "Community already exist.";

    case SNMPV3_ERROR_USER_ALREADY_EXIST:
        return "User already exist.";

    case SNMPV3_ERROR_GROUP_ALREADY_EXIST:
        return "Group already exist.";

    case SNMPV3_ERROR_VIEW_ALREADY_EXIST:
        return "View already exist.";

    case SNMPV3_ERROR_ACCESS_ALREADY_EXIST:
        return "Access already exist.";

    case SNMP_ERROR_TRAP_RECV_ALREADY_EXIST:
        return "Trap receiver already exist.";

    case SNMP_ERROR_TRAP_SOURCE_ALREADY_EXIST:
        return "Trap source already exist.";

    case SNMP_ERROR_ENGINE_FAIL:
        return "SNMP engine occur fail.";

    case SNMPV3_ERROR_COMMUNITIES_TABLE_FULL:
        return "Communities table full";

    case SNMPV3_ERROR_USERS_TABLE_FULL:
        return "Users table full.";

    case SNMPV3_ERROR_GROUPS_TABLE_FULL:
        return "Groups table full.";

    case SNMPV3_ERROR_VIEWS_TABLE_FULL:
        return "Views table full.";

    case SNMPV3_ERROR_ACCESSES_TABLE_FULL:
        return "Accesses table full.";

    case SNMPV3_ERROR_COMMUNITY_NOT_EXIST:
        return "Community does not exist";

    case SNMPV3_ERROR_USER_NOT_EXIST:
        return "User does not exist";

    case SNMPV3_ERROR_GROUP_NOT_EXIST:
        return "Group does not exist";

    case SNMPV3_ERROR_VIEW_NOT_EXIST:
        return "View does not exist";

    case SNMPV3_ERROR_ACCESS_NOT_EXIST:
        return "Access does not exist";

    case SNMP_ERROR_TRAP_RECV_NOT_EXIST:
        return "Trap receiver does not exist";

    case SNMP_ERROR_TRAP_SOURCE_NOT_EXIST:
        return "Trap source does not exist";
    }

    return "SNMP unknown error";
}

/* Get SNMP configuration */
mesa_rc snmp_mgmt_snmp_conf_get(snmp_conf_t *conf)
{
    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter");
    CRIT_SCOPE();
    VTSS_RC(snmp_engine_conf_get(conf));

    T_D("exit");
    return VTSS_RC_OK;
}

/* Set SNMP configuration */
mesa_rc snmp_mgmt_snmp_conf_set(snmp_conf_t *conf)
{
    mesa_rc         rc      = VTSS_RC_OK;
    int             changed = 0;
    snmp_conf_t     snmp_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter, mode: %d",
        conf->mode);

    /* check illegal parameter */
    if (conf->mode != SNMP_MGMT_ENABLED && conf->mode != SNMP_MGMT_DISABLED) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    if (!snmpv3_is_valid_engineid(conf->engineid, conf->engineid_len)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    CRIT_SCOPE();
    if (msg_switch_is_primary()) {
        rc = snmp_engine_conf_get(&snmp_conf);
        changed = snmp_conf_changed(&snmp_conf, conf);
    } else {
        T_W("not primary switch");
        rc = (mesa_rc)SNMP_ERROR_STACK_STATE;
    }

    if (changed) {
        /* Activate changed configuration */
        snmp_global.mode = conf->mode;
        SnmpdSetEngineId((char *)conf->engineid, conf->engineid_len);
    }

    T_D("exit");
    return rc;
}

/* Determine if port and ISID are valid */
static BOOL snmp_mgmt_port_sid_invalid(vtss_isid_t isid, mesa_port_no_t port_no, BOOL is_set)
{
    if (port_no >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_W("illegal port_no: %u", port_no);
        return TRUE;
    }

    /* Check ISID */
    if (isid >= VTSS_ISID_END) {
        T_W("illegal isid: %d", isid);
        return TRUE;
    }

    if (is_set && isid == VTSS_ISID_LOCAL) {
        T_W("SET not allowed, isid: %d", isid);
        return TRUE;
    }

    return FALSE;
}

/* Get SNMP port configuration */
mesa_rc snmp_mgmt_snmp_port_conf_get(vtss_isid_t isid,
                                     mesa_port_no_t port_no,
                                     snmp_port_conf_t *conf)
{
    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter, isid: %d, port_no: %u", isid, port_no);

    /* check illegal parameter */
    if (snmp_mgmt_port_sid_invalid(isid, port_no, FALSE)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    CRIT_SCOPE();
    *conf = snmp_global.snmp_port_conf[isid][port_no - VTSS_PORT_NO_START];

    T_D("exit");
    return VTSS_RC_OK;
}

/* Get SNMP SMON statistics row entry */
mesa_rc snmp_mgmt_smon_stat_entry_get(snmp_rmon_stat_entry_t *entry, BOOL next)
{
    ulong i, num, found = 0;

    if (entry == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter");
    CRIT_SCOPE();
    for (i = 0, num = 0;
         i < SNMP_SMON_STAT_MAX_ROW_SIZE && num < snmp_global.snmp_smon_stat_entry_num;
         i++) {
        if (!snmp_global.snmp_smon_stat_entry[i].valid) {
            continue;
        }
        num++;
        if (entry->ctrl_index == 0 && next) {
            *entry = snmp_global.snmp_smon_stat_entry[i];
            found = 1;
            break;
        }
        if (snmp_global.snmp_smon_stat_entry[i].ctrl_index == entry->ctrl_index) {
            if (next) {
                if (num == snmp_global.snmp_smon_stat_entry_num) {
                    break;
                }
                i++;
                while (i < SNMP_SMON_STAT_MAX_ROW_SIZE ) {
                    if (snmp_global.snmp_smon_stat_entry[i].valid) {
                        *entry = snmp_global.snmp_smon_stat_entry[i];
                        found = 1;
                        break;
                    }
                    i++;
                }
                break;
            } else {
                *entry = snmp_global.snmp_smon_stat_entry[i];
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

mesa_rc snmp_mgmt_smon_stat_entry_set(snmp_rmon_stat_entry_t *entry)
{
    mesa_rc              rc = VTSS_RC_OK;
    int                  changed = 0, found_flag = 0;
    ulong                i, num;

    if (entry == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter");
    CRIT_SCOPE();
    if (msg_switch_is_primary()) {
        for (i = 0, num = 0;
             i < SNMP_SMON_STAT_MAX_ROW_SIZE && num < snmp_global.snmp_smon_stat_entry_num;
             i++) {
            if (!snmp_global.snmp_smon_stat_entry[i].valid) {
                continue;
            }
            num++;
            if (snmp_global.snmp_smon_stat_entry[i].ctrl_index == entry->ctrl_index) {
                found_flag = 1;
                break;
            }
        }

        if (i < SNMP_SMON_STAT_MAX_ROW_SIZE && found_flag) {
            changed = snmp_smon_stat_entry_changed(&snmp_global.snmp_smon_stat_entry[i], entry);
            if (!entry->valid) {
                snmp_global.snmp_smon_stat_entry_num--;
            }
            snmp_global.snmp_smon_stat_entry[i] = *entry;
        } else if (entry->valid) {
            /* add new entry */
            for (i = 0; i < SNMP_SMON_STAT_MAX_ROW_SIZE; i++) {
                if (snmp_global.snmp_smon_stat_entry[i].valid) {
                    continue;
                }
                snmp_global.snmp_smon_stat_entry_num++;
                snmp_global.snmp_smon_stat_entry[i] = *entry;
                break;
            }
            if (i < SNMP_SMON_STAT_MAX_ROW_SIZE) {
                changed = 1;
            } else {
                rc = (mesa_rc) SNMP_ERROR_SMON_STAT_TABLE_FULL;
            }
        }
    } else {
        T_W("not primary switch");
        rc = (mesa_rc)SNMP_ERROR_STACK_STATE;
    }

    (void) changed;  // Quiet lint

    T_D("exit");
    return rc;
}

/* check is SNMP admin string format */
BOOL snmpv3_is_admin_string(const char *str)
{
    uint idx;

    /* check illegal parameter,
       admin string length is restricted to 1 - 32,
       admin string is restricted to the UTF-8 octet string (33 - 126) */
    if (strlen(str) < SNMPV3_MIN_NAME_LEN || strlen(str) > SNMPV3_MAX_NAME_LEN) {
        return FALSE;
    }
    for (idx = 0; idx < strlen(str); idx++) {
        if (str[idx] < 33 || str[idx] > 126) {
            return FALSE;
        }
    }

    return TRUE;
}

/* check is valid engine ID */
BOOL snmpv3_is_valid_engineid(uchar *engineid, ulong engineid_len)
{
    uint idx, val_0_cnt = 0, val_ff_cnt = 0;

    /* The format of 'Engine ID' may not be all zeros or all 'ff'H
       and is restricted to 5 - 32 octet string */

    if (engineid_len < SNMPV3_MIN_ENGINE_ID_LEN || engineid_len > SNMPV3_MAX_ENGINE_ID_LEN) {
        return FALSE;
    }

    for (idx = 0; idx < engineid_len; idx++) {
        if (engineid[idx] == 0x0) {
            val_0_cnt++;
        } else if (engineid[idx] == 0xff) {
            val_ff_cnt++;
        }
    }
    if (val_0_cnt == engineid_len || val_ff_cnt == engineid_len) {
        return FALSE;
    }

    return TRUE;
}

/* check SNMPv3 communities configuration */
mesa_rc snmpv3_mgmt_communities_conf_check(snmpv3_communities_conf_t *conf)
{
    T_D("enter");

    /* check illegal parameter */
    if (!snmpv3_is_admin_string(conf->community)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!snmpv3_is_admin_string(conf->communitySecret)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!snmp_is_ip_valid(conf->sip)) {
        return ((mesa_rc) SNMPV3_ERROR_COMMUNITIES_IP_INVALID);
    }
    if (conf->storage_type < SNMP_MGMT_STORAGE_OTHER ||
        conf->storage_type > SNMP_MGMT_STORAGE_READONLY) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->status > SNMP_MGMT_ROW_DESTROY) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    T_D("exit");
    return VTSS_RC_OK;
}

static int snmp_community_cmp(snmpv3_communities_conf_t *data, snmpv3_communities_conf_t *key)
{
    int cmp;
    if ((cmp = snmp_str_cmp(data->community, key->community)) != 0) {
        return cmp;
    }

    return snmp_ip_cmp(&data->sip, &key->sip);
}


/* Get SNMPv3 communities configuration,
fill community = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_communities_conf_get(snmpv3_communities_conf_t *conf, BOOL next)
{
    uint i, num, found = 0;
    snmpv3_communities_conf_t *tmp = NULL;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter");
    CRIT_SCOPE();

    for (i = 0, num = 0;
         i < SNMPV3_MAX_COMMUNITIES && num < snmp_global.communities_conf_num;
         i++) {
        if (!snmp_global.communities_conf[i].valid) {
            continue;
        }
        num++;

        if (next) {
            if (snmp_community_cmp(&snmp_global.communities_conf[i], conf) > 0) {
                if (tmp) {
                    if (snmp_community_cmp(&snmp_global.communities_conf[i], tmp) >= 0) {
                        continue;
                    }
                }
                tmp = &snmp_global.communities_conf[i];
                found = 1;
            }
        } else if (!snmp_community_cmp(conf, &snmp_global.communities_conf[i])) {
            tmp = &snmp_global.communities_conf[i];
            found = 1;
            break;
        }
    }

    T_D("exit");

    if (found) {
        /* make lint happy */
        if (tmp) {
            *conf = *tmp;
        }
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

/* Set SNMPv3 communities configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_communities_conf_set(snmpv3_communities_conf_t *conf)
{
    mesa_rc                 rc = VTSS_RC_OK;
    int                     changed = 0, found_flag = 0, overlap_flag = 0;
    uint                    i, num, found_num = 0;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter");

    if (conf->valid && conf->status == SNMP_MGMT_ROW_ACTIVE) {
        if ((rc = snmpv3_mgmt_communities_conf_check(conf)) != VTSS_RC_OK) {
            T_D("exit");
            return rc;
        }
    }

    if (msg_switch_is_primary()) {
        CRIT_SCOPE();
        for (i = 0, num = 0;
             i < SNMPV3_MAX_COMMUNITIES && num < snmp_global.communities_conf_num;
             i++) {
            if (!snmp_global.communities_conf[i].valid) {
                continue;
            }
            num++;
            if (!strcmp(conf->community, snmp_global.communities_conf[i].community)) {
                if (!snmp_ip_cmp(&conf->sip, &snmp_global.communities_conf[i].sip)) {
                    found_flag = 1;
                    found_num = i;
                    if (!conf->valid) {
                        break;
                    }
                    // Continue checking for overlap...
                }
            } else {
                if (conf->valid && !strcmp(conf->communitySecret, snmp_global.communities_conf[i].communitySecret) &&
                    snmp_ip_overlap(&conf->sip, &snmp_global.communities_conf[i].sip)) {
                    overlap_flag = 1;
                    break;
                }
            }
        }
        i = found_num;

        if (overlap_flag) {
            rc = (mesa_rc) SNMPV3_ERROR_COMMUNITIES_IP_OVERLAP;
        } else if (found_flag && i < SNMPV3_MAX_COMMUNITIES) {
            if (!conf->valid || conf->status == SNMP_MGMT_ROW_DESTROY) {
                snmp_global.communities_conf_num--;
                conf->valid = 0;
            }
            conf->idx = snmp_global.communities_conf[i].idx;
            changed = snmpv3_communities_conf_changed(&snmp_global.communities_conf[i], conf);
            snmp_global.communities_conf[i] = *conf;
        } else if (conf->valid) {
            /* add new entry */
            for (i = 0; i < SNMPV3_MAX_COMMUNITIES; i++) {
                if (snmp_global.communities_conf[i].valid) {
                    continue;
                }
                conf->idx = i + 1;
                snmp_global.communities_conf_num++;
                snmp_global.communities_conf[i] = *conf;
                break;
            }
            if (i < SNMPV3_MAX_COMMUNITIES) {
                changed = 1;
            } else {
                rc = (mesa_rc) SNMPV3_ERROR_COMMUNITIES_TABLE_FULL;
            }
        } else if (!conf->valid || conf->status == SNMP_MGMT_ROW_DESTROY) {
            rc = (mesa_rc) SNMPV3_ERROR_COMMUNITY_NOT_EXIST;
        }
    } else {
        T_W("not primary switch");
        rc = (mesa_rc) SNMP_ERROR_STACK_STATE;
    }

    if (changed) {
        set_snmpd_communities_updated(true);
    }

    T_D("exit");
    return rc;
}

char *password_as_hex_string(const char *password, u32 len, char *password_hex, u8 max_len)
{
    char hex[3];
    strcpy(password_hex, "");

    if (len == 0) {
        strcpy(password_hex, "ERROR");
        return password_hex;
    }

    for (auto i = 0; (i < len) && (i < max_len / 2); i++) {
        snprintf(hex, 3, "%02X", (u8)password[i]);
        strncat(password_hex, hex, max_len);
    }
    return password_hex;
}

void snmpv3_engine_communities_write(void)
{
    char ip_buf[64];
    char line[SPRINT_MAX_LEN];
    uint i, num;

    T_D("enter");
#if defined(MSCC_BRSDK)
    netsnmp_udp_com2SecList_free(); // This requires a patched netsnmp, which is not available on Ubuntu
#ifdef VTSS_SW_OPTION_IPV6
    netsnmp_udp6_com2Sec6List_free(); // This requires a patched netsnmp, which is not available on Ubuntu
#endif /* VTSS_SW_OPTION_IPV6 */
#endif
    for (i = 0, num = 0;
         i < SNMPV3_MAX_COMMUNITIES && num < snmp_global.communities_conf_num;
         i++) {
        if (!snmp_global.communities_conf[i].valid) {
            continue;
        }
        num++;
        if (snmp_global.communities_conf[i].sip.address.type == MESA_IP_TYPE_IPV4) {
            /* IPv4 */
            snprintf(line, sizeof(line), "'%s' %s/%u '%s'",
                     snmp_global.communities_conf[i].community,
                     misc_ipv4_txt(snmp_global.communities_conf[i].sip.address.addr.ipv4, ip_buf),
                     snmp_global.communities_conf[i].sip.prefix_size,
                     snmp_global.communities_conf[i].communitySecret);
            netsnmp_udp_parse_security(NULL, line);
#ifdef VTSS_SW_OPTION_IPV6
        } else if (snmp_global.communities_conf[i].sip.address.type == MESA_IP_TYPE_IPV6) {
            /* IPv6 */
            snprintf(line, sizeof(line), "'%s' %s/%u '%s'",
                     snmp_global.communities_conf[i].community,
                     misc_ipv6_txt(&snmp_global.communities_conf[i].sip.address.addr.ipv6, ip_buf),
                     snmp_global.communities_conf[i].sip.prefix_size,
                     snmp_global.communities_conf[i].communitySecret);
            netsnmp_udp6_parse_security(NULL, line);
#endif
        }
    }
    T_D("exit");
}

/* check SNMPv3 users configuration */
mesa_rc snmpv3_mgmt_users_conf_check(snmpv3_users_conf_t *conf)
{
    T_D("enter");

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    /* check illegal parameter */
    if (!snmpv3_is_valid_engineid(conf->engineid, conf->engineid_len)) {
        T_I("Invalid engineid");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!snmpv3_is_admin_string(conf->user_name)) {
        T_I("Invalid user name");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!strcmp(conf->user_name, SNMPV3_NONAME)) {
        T_I("Invalid no-name");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->security_level < SNMP_MGMT_SEC_LEVEL_NOAUTH ||
        conf->security_level > SNMP_MGMT_SEC_LEVEL_AUTHPRIV) {
        T_I("Invalid security level");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->auth_protocol > SNMP_MGMT_AUTH_PROTO_SHA) {
        T_I("Invalid SHA");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
#if defined(NETSNMP_USE_OPENSSL)
    if (conf->priv_protocol > SNMP_MGMT_PRIV_PROTO_AES) {
#else
    if (conf->priv_protocol > SNMP_MGMT_PRIV_PROTO_DES) {
#endif
        T_I("Invalid privacy protocol");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    if (((conf->security_level == SNMP_MGMT_SEC_LEVEL_NOAUTH) &&
         (conf->auth_protocol != SNMP_MGMT_AUTH_PROTO_NONE || conf->priv_protocol != SNMP_MGMT_PRIV_PROTO_NONE)) ||
        ((conf->security_level == SNMP_MGMT_SEC_LEVEL_AUTHNOPRIV) &&
         ((conf->auth_protocol == SNMP_MGMT_AUTH_PROTO_NONE || conf->priv_protocol != SNMP_MGMT_PRIV_PROTO_NONE))) ||
        ((conf->security_level == SNMP_MGMT_SEC_LEVEL_AUTHPRIV) &&
         ((conf->auth_protocol == SNMP_MGMT_AUTH_PROTO_NONE || conf->priv_protocol == SNMP_MGMT_PRIV_PROTO_NONE)))) {
        T_I("Invalid Protocol");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->security_level != SNMP_MGMT_SEC_LEVEL_NOAUTH) {
        auto max_len = 0;
        if (conf->auth_protocol == SNMP_MGMT_AUTH_PROTO_MD5) {
            max_len = conf->auth_password_encrypted ? SNMPV3_MAX_MD5_PASSWORD_LEN * 2 : SNMPV3_MAX_MD5_PASSWORD_LEN;
        } else { //SHA
            max_len = conf->auth_password_encrypted ? SNMPV3_MAX_SHA_PASSWORD_LEN * 2 : SNMPV3_MAX_SHA_PASSWORD_LEN;
        }

        conf->auth_password_len = strlen(conf->auth_password);

        if (conf->auth_password_len < SNMPV3_MIN_PASSWORD_LEN || conf->auth_password_len  > max_len) {
            T_I("Invalid auth password length:%d, min:%d, max:%d", conf->auth_password_len, SNMPV3_MIN_PASSWORD_LEN, max_len);
            return SNMPV3_ERROR_PASSWORD_LEN;
        }

        if (conf->security_level == SNMP_MGMT_SEC_LEVEL_AUTHPRIV) {
            conf->priv_password_len = strlen(conf->priv_password);

            if (conf->priv_password_len < SNMPV3_MIN_PASSWORD_LEN || conf->priv_password_len > SNMPV3_MAX_DES_PASSWORD_LEN) {
                T_I("Invalid priv password length:%d, min:%d, max:%d", conf->priv_password_len, SNMPV3_MIN_PASSWORD_LEN, SNMPV3_MAX_DES_PASSWORD_LEN);
                return SNMPV3_ERROR_PASSWORD_LEN;
            }
        }
    }
    if (conf->storage_type < SNMP_MGMT_STORAGE_OTHER ||
        conf->storage_type > SNMP_MGMT_STORAGE_READONLY) {
        T_I("Invalid storage type");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->status > SNMP_MGMT_ROW_DESTROY) {
        T_I("Invalid status");
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* Get SNMPv3 users configuration,
fill user_name = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_users_conf_get(snmpv3_users_conf_t *conf, BOOL next)
{
    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter %s %d", conf->user_name, next);
    CRIT_SCOPE();
    return snmpv3_engine_users_get(conf, next);
}

/* Set SNMPv3 users configuration */
mesa_rc snmpv3_mgmt_users_conf_set(snmpv3_users_conf_t *conf)
{
    mesa_rc                 rc = VTSS_RC_OK;
    snmpv3_users_conf_t     found_conf;

    T_D("enter");

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    if (conf->valid && conf->status == SNMP_MGMT_ROW_ACTIVE) {
        if ((rc = snmpv3_mgmt_users_conf_check(conf)) != VTSS_RC_OK) {
            T_D("exit");
            return rc;
        }
    }

    if (!msg_switch_is_primary()) {
        return SNMP_ERROR_STACK_STATE;
    }

    CRIT_SCOPE();
    if (conf->valid) {
        // Check the operation is 'ADD' or 'SET'
        memcpy(found_conf.engineid, conf->engineid, conf->engineid_len);
        found_conf.engineid_len = conf->engineid_len;
        strcpy(found_conf.user_name, conf->user_name);
        if (snmpv3_engine_users_get(&found_conf, FALSE) != VTSS_RC_OK) {
#if !defined(SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT)
            u32 curr_entry_count = 0;

            /* It is an 'ADD' operation */
            /* Get the current entry count */
            memset(&found_conf, 0, sizeof(found_conf));
            while (snmpv3_engine_users_get(&found_conf, TRUE) == VTSS_RC_OK) {
                curr_entry_count++;
            }
            if (curr_entry_count >= SNMPV3_MAX_USERS) {
                rc = SNMPV3_ERROR_USERS_TABLE_FULL;
            }
#endif /* SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT */
        } else {
            /* It is an 'MOD' operation - remove and re-add */
            rc = snmpv3_engine_users_set(conf, FALSE);
        }
    }

    if (rc == VTSS_RC_OK) {
        rc = snmpv3_engine_users_set(conf, conf->valid); // Not valid is same as remove
    }

    T_D("exit");
    return rc;
}

/* check SNMPv3 groups configuration */
mesa_rc snmpv3_mgmt_groups_conf_check(snmpv3_groups_conf_t *conf)
{
    T_D("enter");

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    /* check illegal parameter */
    if (conf->security_model < SNMP_MGMT_SEC_MODEL_SNMPV1 ||
        conf->security_model > SNMP_MGMT_SEC_MODEL_USM) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!snmpv3_is_admin_string(conf->security_name)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!snmpv3_is_admin_string(conf->group_name)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->storage_type < SNMP_MGMT_STORAGE_OTHER ||
        conf->storage_type > SNMP_MGMT_STORAGE_READONLY) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->status > SNMP_MGMT_ROW_DESTROY) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* Get SNMPv3 groups configuration,
fill security_name = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_groups_conf_get(snmpv3_groups_conf_t *conf, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter %s %d", conf->security_name, next);
    CRIT_SCOPE();
    rc = snmpv3_engine_groups_get(conf, next);

    T_D("exit %d", rc);
    return rc;
}

/* Set SNMPv3 groups configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_groups_conf_set(snmpv3_groups_conf_t *conf)
{
    mesa_rc                 rc = VTSS_RC_OK;
    int                     changed = 0;
    snmpv3_groups_conf_t    found_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    if (conf->valid && conf->status == SNMP_MGMT_ROW_ACTIVE) {
        if ((rc = snmpv3_mgmt_groups_conf_check(conf)) != VTSS_RC_OK) {
            T_D("exit");
            return rc;
        }
    }

    if (msg_switch_is_primary()) {
        CRIT_SCOPE();
        found_conf.security_model = conf->security_model;
        strcpy(found_conf.security_name, conf->security_name);
        if (snmpv3_engine_groups_get(&found_conf, FALSE) == VTSS_RC_OK) {
            if (!conf->valid || conf->status == SNMP_MGMT_ROW_DESTROY) {
                /* delete entry */
                rc = snmpv3_engine_groups_set(conf, FALSE);
            } else {
                /* change entry */
                changed = snmpv3_groups_conf_changed(&found_conf, conf);
                if (changed) {
                    rc = snmpv3_engine_groups_set(conf, FALSE);
                    if (rc == VTSS_RC_OK) {
                        rc = snmpv3_engine_groups_set(conf, TRUE);
                    }
                }
            }
        } else if (conf->valid) {
#if defined(SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT)
            rc = snmpv3_engine_groups_set(conf, TRUE);
            changed = VTSS_RC_OK == rc ? 1 : 0;
#else
            u32 curr_entry_count = 0;

            /* Get the current entry count */
            memset(&found_conf, 0, sizeof(found_conf));
            while (snmpv3_engine_groups_get(&found_conf, TRUE) == VTSS_RC_OK) {
                curr_entry_count++;
            }
            if (curr_entry_count >= SNMPV3_MAX_GROUPS) {
                rc = SNMPV3_ERROR_GROUPS_TABLE_FULL;
            } else {
                /* add new entry */
                rc = snmpv3_engine_groups_set(conf, TRUE);
                changed = VTSS_RC_OK == rc ? 1 : 0;
            }
#endif /* SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT */
        } else if (!conf->valid || conf->status == SNMP_MGMT_ROW_DESTROY) {
            rc = SNMPV3_ERROR_GROUP_NOT_EXIST;
        }

    } else {
        T_W("not primary switch");
        rc = (mesa_rc) SNMP_ERROR_STACK_STATE;
    }

    T_D("exit");
    return rc;
}

/* check SNMPv3 views configuration */
mesa_rc snmpv3_mgmt_views_conf_check(snmpv3_views_conf_t *conf)
{
    T_D("enter");

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    /* check illegal parameter */
    if (!snmpv3_is_admin_string(conf->view_name)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!strcmp(conf->view_name, SNMPV3_NONAME)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!strlen(conf->subtree) || strlen(conf->subtree) > SNMP_MGMT_MAX_SUBTREE_STR_LEN) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->view_type != SNMPV3_MGMT_VIEW_INCLUDED &&
        conf->view_type != SNMPV3_MGMT_VIEW_EXCLUDED) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->storage_type < SNMP_MGMT_STORAGE_OTHER ||
        conf->storage_type > SNMP_MGMT_STORAGE_READONLY) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->status > SNMP_MGMT_ROW_DESTROY) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* Get SNMPv3 views configuration,
fill view_name = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_views_conf_get(snmpv3_views_conf_t *conf, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter %s %d", conf->view_name, next);
    CRIT_SCOPE();
    rc = snmpv3_engine_views_get(conf, next);

    T_D("exit %d", rc);
    return rc;
}

/* Set SNMPv3 views configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_views_conf_set(snmpv3_views_conf_t *conf)
{
    mesa_rc                 rc = VTSS_RC_OK;
    int                     changed = 0;
    snmpv3_views_conf_t     found_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    if (conf->valid && conf->status == SNMP_MGMT_ROW_ACTIVE) {
        if ((rc = snmpv3_mgmt_views_conf_check(conf)) != VTSS_RC_OK) {
            T_D("exit");
            return rc;
        }
    }

    if (msg_switch_is_primary()) {
        CRIT_SCOPE();
        strcpy(found_conf.view_name, conf->view_name);
        strcpy(found_conf.subtree, conf->subtree);
        if (snmpv3_engine_views_get(&found_conf, FALSE) == VTSS_RC_OK) {
            if (!conf->valid || conf->status == SNMP_MGMT_ROW_DESTROY) {
                /* delete entry */
                rc = snmpv3_engine_views_set(conf, FALSE);
            } else {
                /* change entry */
                changed = snmpv3_views_conf_changed(&found_conf, conf);
                if (changed) {
                    rc = snmpv3_engine_views_set(conf, FALSE);
                    if (rc == VTSS_RC_OK) {
                        rc = snmpv3_engine_views_set(conf, TRUE);
                    }
                }
            }
        } else if (conf->valid) {
#if defined(SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT)
            rc = snmpv3_engine_views_set(conf, TRUE);
            changed = VTSS_RC_OK == rc ? 1 : 0;
#else
            u32 curr_entry_count = 0;

            /* Get the current entry count */
            memset(&found_conf, 0, sizeof(found_conf));
            while (snmpv3_engine_views_get(&found_conf, TRUE) == VTSS_RC_OK) {
                curr_entry_count++;
            }
            if (curr_entry_count >= SNMPV3_MAX_VIEWS) {
                rc = SNMPV3_ERROR_VIEWS_TABLE_FULL;
            } else {
                /* add new entry */
                rc = snmpv3_engine_views_set(conf, TRUE);
                changed = VTSS_RC_OK == rc ? 1 : 0;
            }
#endif /* SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT */
        } else if (!conf->valid || conf->status == SNMP_MGMT_ROW_DESTROY) {
            rc = SNMPV3_ERROR_VIEW_NOT_EXIST;
        }

    } else {
        T_W("not primary switch");
        rc = (mesa_rc) SNMP_ERROR_STACK_STATE;
    }

    T_D("exit");
    return rc;
}

/* check SNMPv3 accesses configuration */
mesa_rc snmpv3_mgmt_accesses_conf_check(snmpv3_accesses_conf_t *conf)
{
    T_D("enter");

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    /* check illegal parameter */
    if (!snmpv3_is_admin_string(conf->group_name)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->security_model > SNMP_MGMT_SEC_MODEL_USM) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->security_level < SNMP_MGMT_SEC_LEVEL_NOAUTH ||
        conf->security_level > SNMP_MGMT_SEC_LEVEL_AUTHPRIV) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->context_match != SNMPV3_MGMT_CONTEX_MATCH_EXACT &&
        conf->context_match != SNMPV3_MGMT_CONTEX_MATCH_PREFIX) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!snmpv3_is_admin_string(conf->read_view_name)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!snmpv3_is_admin_string(conf->write_view_name)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (!snmpv3_is_admin_string(conf->notify_view_name)) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->storage_type < SNMP_MGMT_STORAGE_OTHER ||
        conf->storage_type > SNMP_MGMT_STORAGE_READONLY) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }
    if (conf->status > SNMP_MGMT_ROW_DESTROY) {
        return ((mesa_rc) SNMP_ERROR_PARM);
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/* Get SNMPv3 accesses configuration,
fill group_name = SNMPV3_CONF_ACESS_GETFIRST will get first entry */
mesa_rc snmpv3_mgmt_accesses_conf_get(snmpv3_accesses_conf_t *conf, BOOL next)
{
    mesa_rc rc = VTSS_RC_OK;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter %s %d", conf->group_name, next);
    CRIT_SCOPE();
    rc = snmpv3_engine_accesses_get(conf, next);

    T_D("exit %d", rc);
    return rc;
}

/* Set SNMPv3 accesses configuration,
fill valid = 0 or status = SNMP_MGMT_ROW_DESTROY will destroy the entry */
mesa_rc snmpv3_mgmt_accesses_conf_set(snmpv3_accesses_conf_t *conf)
{
    mesa_rc                 rc = VTSS_RC_OK;
    int                     changed = 0;
    snmpv3_accesses_conf_t  found_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    if (conf->valid && conf->status == SNMP_MGMT_ROW_ACTIVE) {
        if ((rc = snmpv3_mgmt_accesses_conf_check(conf)) != VTSS_RC_OK) {
            T_D("exit");
            return rc;
        }
    }

    if (msg_switch_is_primary()) {
        CRIT_SCOPE();
        strcpy(found_conf.group_name, conf->group_name);
        strcpy(found_conf.context_prefix, conf->context_prefix);
        found_conf.security_model = conf->security_model;
        found_conf.security_level = conf->security_level;
        if (snmpv3_engine_accesses_get(&found_conf, FALSE) == VTSS_RC_OK) {
            if (!conf->valid || conf->status == SNMP_MGMT_ROW_DESTROY) {
                /* delete entry */
                rc = snmpv3_engine_accesses_set(conf, FALSE);
            } else {
                /* change entry */
                changed = snmpv3_accesses_conf_changed(&found_conf, conf);
                if (changed) {
                    rc = snmpv3_engine_accesses_set(conf, FALSE);
                    if (rc == VTSS_RC_OK) {
                        rc = snmpv3_engine_accesses_set(conf, TRUE);
                    }
                }
            }
        } else if (conf->valid) {
#if defined(SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT)
            rc = snmpv3_engine_accesses_set(conf, TRUE);
            changed = VTSS_RC_OK == rc ? 1 : 0;
#else
            u32 curr_entry_count = 0;

            /* Get the current entry count */
            memset(&found_conf, 0, sizeof(found_conf));
            while (snmpv3_engine_accesses_get(&found_conf, TRUE) == VTSS_RC_OK) {
                curr_entry_count++;
            }
            if (curr_entry_count >= SNMPV3_MAX_ACCESSES) {
                rc = SNMPV3_ERROR_ACCESSES_TABLE_FULL;
            } else {
                /* add new entry */
                rc = snmpv3_engine_accesses_set(conf, TRUE);
                changed = VTSS_RC_OK == rc ? 1 : 0;
            }
#endif /* SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT */
        } else if (!conf->valid || conf->status == SNMP_MGMT_ROW_DESTROY) {
            rc = SNMPV3_ERROR_ACCESS_NOT_EXIST;
        }

    } else {
        T_W("not primary switch");
        rc = (mesa_rc) SNMP_ERROR_STACK_STATE;
    }

    T_D("exit");
    return rc;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

/* Send trap packet */
static void snmp_mgmt_send_trap(snmp_trap_type_t trap_type, snmp_trap_if_info_t trap_if_info)
{
    if (msg_switch_is_primary() && (SNMP_TRAP_BUFF_CNT(trap_buff_read_idx, trap_buff_write_idx) < (SNMP_TRAP_BUFF_MAX_CNT - 1))) {
        CRIT_SCOPE();
        snmp_trap_buff[trap_buff_write_idx].trap_type = trap_type;
        snmp_trap_buff[trap_buff_write_idx].trap_if_info = trap_if_info;
        trap_buff_write_idx = (trap_buff_write_idx + 1) % SNMP_TRAP_BUFF_MAX_CNT;

    }
}

mesa_rc trap_engine_source_config(char *source, BOOL enable)
{
    if ((strcmp(source, TRAP_NAME_COLD_START) == 0) ||
        (strcmp(source, TRAP_NAME_WARM_START) == 0) ||
        (strcmp(source, TRAP_NAME_LINK_UP) == 0) ||
        (strcmp(source, TRAP_NAME_LINK_DOWN) == 0)) {
        // Do nothing
        return VTSS_RC_OK;
    }
    if ((strcmp(source, TRAP_NAME_AUTH_FAIL) == 0)) {
        char buf[2];
        if (enable) {
            buf[0] = '1';
        } else {
            buf[0] = '2';
        }
        buf[1] = '\0';
        snmpd_parse_config_authtrap((char *) "authtrapenable", buf);
        return VTSS_RC_OK;
    }
#ifdef VTSS_SW_OPTION_MSTP
    if ((strcmp(source, TRAP_NAME_NEW_ROOT) == 0) ||
        (strcmp(source, TRAP_NAME_TOPO_CHNG) == 0)) {
        // Do nothing
        return VTSS_RC_OK;
    }
#endif
#ifdef VTSS_SW_OPTION_LLDP
    if ((strcmp(source, TRAP_NAME_LLDP_REM_TBL_CHNG) == 0)) {
        // Do nothing
        return VTSS_RC_OK;
    }
#endif
#ifdef VTSS_SW_OPTION_RMON
    if ((strcmp(source, TRAP_NAME_RMON_RISING) == 0) ||
        (strcmp(source, TRAP_NAME_RMON_FALLING) == 0)) {
        // Do nothing
        return VTSS_RC_OK;
    }
#endif
#if RFC4133_SUPPORTED_ENTITY
#if RFC4133_SUPPORTED_TRAPS
    if ((strcmp(source, TRAP_NAME_ENTITY_CONF_CHNG) == 0)) {
        // Do nothing
        return VTSS_RC_OK;
    }
#endif
#endif
    if (enable) {
        return vtss_appl_snmp_private_trap_listen_add(source);
    } else {
        return vtss_appl_snmp_private_trap_listen_del(source);
    }
}

/* Write trapsess information to net-snmp */
void snmpv3_mgmt_trapsess_write()
{
    T_N("Enter");

    char engineid_txt[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];

    char line[SPRINT_MAX_LEN];
    char inform[20];
    char version[6];
    snmpv3_users_conf_t user_conf;
    char engineid[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 8];
    char user[SNMPV3_MAX_NAME_LEN + 6];
    char auth_type[18];
    char community[SNMP_MGMT_INPUT_COMMUNITY_LEN + 6];
    char addr[512];
    char port[10];

    snmpd_free_trapsinks();

    vtss_trap_entry_t trap_entry;
    memset(&trap_entry, 0, sizeof(trap_entry));

    vtss_trap_entry_t *tmp;
    while ((tmp = get_next_trap_entry_point(&trap_entry)) != NULL) {
        memcpy(&trap_entry, tmp, sizeof(vtss_trap_entry_t));
        if (!trap_entry.trap_conf.enable) {
            continue;
        }
        if (trap_entry.trap_conf.trap_inform_mode) {
            snprintf(inform, sizeof(inform), "-Ci -r %d -t %d ", trap_entry.trap_conf.trap_inform_retries, trap_entry.trap_conf.trap_inform_timeout);
        } else {
            strcpy(inform, "");
        }

        switch (trap_entry.trap_conf.trap_version) {
        case SNMP_MGMT_VERSION_1:  // Version 1
            snprintf(version, sizeof(version), "-v1");
            strcpy(engineid, "");
            strcpy(user, "");
            strcpy(auth_type, "");
            // COMMUNITY
            snprintf(community, sizeof(community), " -c %s", trap_entry.trap_conf.trap_communitySecret);
            break;
        case SNMP_MGMT_VERSION_2C:  // Version 2
            snprintf(version, sizeof(version), "-v2c");
            strcpy(engineid, "");
            strcpy(user, "");
            strcpy(auth_type, "");
            // COMMUNITY
            snprintf(community, sizeof(community), " -c %s", trap_entry.trap_conf.trap_communitySecret);
            break;
        case SNMP_MGMT_VERSION_3:  // Version 3
            snprintf(version, sizeof(version), "-v3");
            snprintf(engineid, sizeof(engineid), " -e 0x%s", misc_engineid2str(engineid_txt, trap_entry.trap_conf.trap_engineid, trap_entry.trap_conf.trap_engineid_len));
            memcpy(user_conf.engineid, trap_entry.trap_conf.trap_engineid, trap_entry.trap_conf.trap_engineid_len);
            user_conf.engineid_len = trap_entry.trap_conf.trap_engineid_len;
            if (strlen(trap_entry.trap_conf.trap_security_name) > 0) {
                snprintf(user, sizeof(user), " -u %s", trap_entry.trap_conf.trap_security_name);
            }
            strcpy(user_conf.user_name, trap_entry.trap_conf.trap_security_name);
            if (snmpv3_engine_users_get(&user_conf, FALSE) == VTSS_RC_OK) {
                snprintf(auth_type, sizeof(auth_type), " -l %s", user_conf.security_level == SNMP_MGMT_SEC_LEVEL_NOAUTH ? "noAuthNoPriv" : user_conf.security_level == SNMP_MGMT_SEC_LEVEL_AUTHNOPRIV ? "authNoPriv" : "authPriv");
            } else {
                T_I("Unknown trap v3 user: '%s'. Ignoring trap entry.", trap_entry.trap_conf.trap_security_name);
                continue;
            }
            strcpy(community, "");
            break;
        default:
            T_E("Unknown trap version");
            continue;
        }

        // HOST
        char buf[64];
        if (trap_entry.trap_conf.dip.type == VTSS_INET_ADDRESS_TYPE_IPV4) {
            snprintf(addr, sizeof(addr), " %s", misc_ipv4_txt(trap_entry.trap_conf.dip.address.ipv4, buf));
        } else if (trap_entry.trap_conf.dip.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
#ifdef VTSS_SW_OPTION_IPV6
            snprintf(addr, sizeof(addr), " [%s]", misc_ipv6_txt(&trap_entry.trap_conf.dip.address.ipv6, buf));
#else
            continue;
#endif
        } else if (trap_entry.trap_conf.dip.type == VTSS_INET_ADDRESS_TYPE_DNS) {
            snprintf(addr, sizeof(addr), " %s", trap_entry.trap_conf.dip.address.domain_name.name);
            addr[sizeof(addr) - 1] = 0;
        } else {
            T_I("Unknown IP address type");
            continue;
        }

        // PORT
        snprintf(port, sizeof(port), ":%d\n", trap_entry.trap_conf.trap_port);

        snprintf(line, sizeof(line), "%s%s%s%s%s%s%s%s",
                 inform,
                 version,
                 engineid,
                 user,
                 auth_type,
                 community,
                 addr,
                 port);
        T_D("trapsess %s", line);
        snmpd_parse_config_trapsess(NULL, line);
    }

    T_D("Done");
    return;
}

static vtss_trap_entry_t *alloc_trap_entry(void)
{
    int i = 0;
    BOOL found = FALSE;
    vtss_trap_entry_t *tmp;

    for (i = 0, tmp = &trap_global.trap_entry[i]; i < VTSS_TRAP_CONF_MAX; i++, tmp++) {
        if ( FALSE == tmp->valid) {
            found = TRUE;
            break;
        }
    }

    if (FALSE == found ) {
        return NULL;
    }

    tmp->valid = TRUE;
    tmp->trap_conf.conf_id = i;
    return tmp;
}

static void free_trap_entry(vtss_trap_entry_t *entry)
{
    memset(entry, 0, sizeof(vtss_trap_entry_t));
}
/* if elm1 lager than elm2, return 1, else if elm1 smaller than elm2, return -1, otherwise return 0 */
static i32 trap_conf_entry_compare_func(void *elm1, void *elm2)
{
    vtss_trap_entry_t *in_list = (vtss_trap_entry_t *)elm1;
    vtss_trap_entry_t *new_entry = (vtss_trap_entry_t *)elm2;
    int cmp1 = strlen(in_list->trap_conf_name) - strlen(new_entry->trap_conf_name);
    int cmp2 = strcmp(in_list->trap_conf_name, new_entry->trap_conf_name);

    if ( cmp1 > 0 || (cmp1 == 0 && cmp2 > 0) ) {
        return 1;
    } else if ( cmp1 < 0 || (cmp1 == 0 && cmp2 < 0) ) {
        return -1;
    } else {
        return 0;
    }

}

VTSS_AVL_TREE(trap_entry_avl, "TRAP_conf_entry", VTSS_MODULE_ID_SNMP, trap_conf_entry_compare_func, VTSS_TRAP_CONF_MAX)

static vtss_trap_entry_t *get_trap_entry_point(vtss_trap_entry_t *trap_entry)
{
    vtss_trap_entry_t *tmp = trap_entry;

    if (vtss_avl_tree_get(&trap_entry_avl, (void **) &tmp, VTSS_AVL_TREE_GET) != TRUE) { // entry not existing
        T_I("Could not get trap entry");
        return NULL;
    }
    return tmp;
}

static vtss_trap_entry_t *get_next_trap_entry_point(vtss_trap_entry_t *trap_entry)
{
    vtss_trap_entry_t *tmp = trap_entry;
    if (vtss_avl_tree_get(&trap_entry_avl, (void **) &tmp, VTSS_AVL_TREE_GET_NEXT) != TRUE) { // entry not existing
        T_N("Could not get trap entry");
        return NULL;
    }
    return tmp;
}

/* if elm1 lager than elm2, return 1, else if elm1 smaller than elm2, return -1, otherwise return 0 */
static i32 trap_filter_compare_func(vtss_trap_filter_item_t *elm1, vtss_trap_filter_item_t *elm2)
{
    int type_cmp = elm1->filter_type - elm2->filter_type;
    if ( type_cmp > 0 ) {
        return 1;
    } else if ( type_cmp < 0 ) {
        return -1;
    }

    int filter_cmp = elm1->index_filter_len - elm2->index_filter_len;
    if (filter_cmp == 0) {
        filter_cmp = memcmp(elm1->index_filter, elm2->index_filter, elm1->index_filter_len * sizeof(oid));
    }
    if ( filter_cmp > 0 ) {
        return 1;
    } else if ( filter_cmp < 0 ) {
        return -1;
    }

    int mask_cmp = elm1->index_mask_len - elm2->index_mask_len;
    if (mask_cmp == 0) {
        mask_cmp = memcmp(elm1->index_mask, elm2->index_mask, elm1->index_mask_len);
    }
    if ( mask_cmp > 0 ) {
        return 1;
    } else if ( mask_cmp < 0 ) {
        return -1;
    }
    return 0;

}

static vtss_trap_filter_item_t *alloc_trap_filter(vtss_trap_source_t *entry)
{
    int i = 0;
    for (i = 0; i < VTSS_TRAP_FILTER_MAX; i++) {
        if (!entry->trap_filter.item[i]) {
            entry->trap_filter.item[i] = (vtss_trap_filter_item_t *)VTSS_MALLOC(sizeof(vtss_trap_filter_item_t));
            if (entry->trap_filter.item[i]) {
                entry->trap_filter.active_cnt++;
            }
            return entry->trap_filter.item[i];
        }
    }
    return NULL;
}

static vtss_trap_filter_item_t *alloc_specific_trap_filter(vtss_trap_source_t *entry, int i)
{
    if (i >= VTSS_TRAP_FILTER_MAX) {
        return NULL;
    }
    if (!entry->trap_filter.item[i]) {
        entry->trap_filter.item[i] = (vtss_trap_filter_item_t *)VTSS_MALLOC(sizeof(vtss_trap_filter_item_t));
        if (entry->trap_filter.item[i]) {
            entry->trap_filter.active_cnt++;
        }
    }
    return entry->trap_filter.item[i];
}

static vtss_trap_filter_item_t *get_trap_filter(vtss_trap_source_t *entry, vtss_trap_filter_item_t *item)
{
    int i = 0;
    for (i = 0; i < VTSS_TRAP_FILTER_MAX; i++) {
        if (entry->trap_filter.item[i]) {
            if (trap_filter_compare_func(entry->trap_filter.item[i], item) == 0) {
                return entry->trap_filter.item[i];
            }
        }
    }
    return NULL;
}

static u32 free_trap_filter(vtss_trap_source_t *entry, vtss_trap_filter_item_t *item)
{
    int i = 0;
    for (i = 0; i < VTSS_TRAP_FILTER_MAX; i++) {
        if (entry->trap_filter.item[i]) {
            if (trap_filter_compare_func(entry->trap_filter.item[i], item) == 0) {
                VTSS_FREE(entry->trap_filter.item[i]);
                entry->trap_filter.item[i] = NULL;
                entry->trap_filter.active_cnt--;
                break;
            }
        }
    }
    return entry->trap_filter.active_cnt;
}

static u32 free_specific_trap_filter(vtss_trap_source_t *entry, int i)
{
    if (i < VTSS_TRAP_FILTER_MAX) {
        if (entry->trap_filter.item[i]) {
            VTSS_FREE(entry->trap_filter.item[i]);
            entry->trap_filter.item[i] = NULL;
            entry->trap_filter.active_cnt--;
        }
    }
    return entry->trap_filter.active_cnt;
}

static u32 trap_filter_cnt(vtss_trap_source_t *entry)
{
    return entry->trap_filter.active_cnt;
}

static vtss_trap_source_t *alloc_trap_source(void)
{
    int i = 0;
    BOOL found = FALSE;
    vtss_trap_source_t *tmp;

    for (i = 0, tmp = &trap_global.trap_source[i]; i < VTSS_TRAP_SOURCE_MAX; i++, tmp++) {
        if ( FALSE == tmp->valid) {
            found = TRUE;
            break;
        }
    }

    if (FALSE == found ) {
        return NULL;
    }

    tmp->valid = TRUE;
    tmp->trap_filter.conf_id = i;
    return tmp;
}

static void free_trap_source(vtss_trap_source_t *entry)
{
    int i = 0;
    for (i = 0; i < VTSS_TRAP_FILTER_MAX; i++) {
        if (entry->trap_filter.item[i]) {
            VTSS_FREE(entry->trap_filter.item[i]);
        }
    }
    (void) trap_engine_source_config(entry->source_name, FALSE);
    memset(entry, 0, sizeof(vtss_trap_source_t));
}

/* if elm1 lager than elm2, return 1, else if elm1 smaller than elm2, return -1, otherwise return 0 */
static i32 trap_conf_source_compare_func(void *elm1, void *elm2)
{
    vtss_trap_source_t *in_list = (vtss_trap_source_t *)elm1;
    vtss_trap_source_t *new_entry = (vtss_trap_source_t *)elm2;
    int cmp1 = strlen(in_list->source_name) - strlen(new_entry->source_name);
    int cmp2 = strcmp(in_list->source_name, new_entry->source_name);

    if ( cmp1 > 0 || (cmp1 == 0 && cmp2 > 0) ) {
        return 1;
    } else if ( cmp1 < 0 || (cmp1 == 0 && cmp2 < 0) ) {
        return -1;
    } else {
        return 0;
    }

}

VTSS_AVL_TREE(trap_source_avl, "TRAP_conf_source", VTSS_MODULE_ID_SNMP, trap_conf_source_compare_func, VTSS_TRAP_SOURCE_MAX)

static vtss_trap_source_t *get_trap_source_point(vtss_trap_source_t *trap_source)
{
    vtss_trap_source_t *tmp = trap_source;

    if (vtss_avl_tree_get(&trap_source_avl, (void **) &tmp, VTSS_AVL_TREE_GET) != TRUE) { // entry not existing
        T_I("Could not get trap entry");
        return NULL;
    }
    return tmp;
}

static vtss_trap_source_t *get_next_trap_source_point(vtss_trap_source_t *trap_source)
{
    vtss_trap_source_t *tmp = trap_source;
    if (vtss_avl_tree_get(&trap_source_avl, (void **) &tmp, VTSS_AVL_TREE_GET_NEXT) != TRUE) { // entry not existing
        T_N("Could not get trap entry");
        return NULL;
    }
    return tmp;
}

mesa_rc trap_init(void)
{

    CRIT_SCOPE();
    memset( &trap_global, 0, sizeof(trap_global));


    if (FALSE == vtss_avl_tree_init(&trap_entry_avl)) {
        T_E("Init trap_entry AVL fail");
    }
    if (FALSE == vtss_avl_tree_init(&trap_source_avl)) {
        T_E("Init trap_source AVL fail");
    }
    return VTSS_RC_OK;
}

/**
  * \brief Get trap configuration entry
  *
  * \param trap_entry   [IN] trap_conf_name: Name of the trap configuration
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_conf_get (vtss_trap_entry_t  *trap_entry)
{
    vtss_trap_entry_t *tmp;

    if (trap_entry == NULL) {
        return SNMP_ERROR_NULL;
    }

    CRIT_SCOPE();
    tmp = get_trap_entry_point(trap_entry);

    if ( !tmp ) {
        return VTSS_RC_ERROR;
    }
    memcpy(trap_entry, tmp, sizeof(vtss_trap_entry_t));

    return VTSS_RC_OK;
}

/**
  * \brief Get next trap configuration entry
  *
  * \param trap_entry   [INOUT] trap_conf_name: Name of the trap configuration
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_conf_get_next (vtss_trap_entry_t  *trap_entry)
{
    if (trap_entry == NULL) {
        return SNMP_ERROR_NULL;
    }

    CRIT_SCOPE();
    vtss_trap_entry_t *tmp = get_next_trap_entry_point(trap_entry);
    if ( !tmp ) {
        T_N("No more entries");
        return VTSS_RC_ERROR;
    }
    memcpy(trap_entry, tmp, sizeof(vtss_trap_entry_t));

    T_I("Entry found");
    return VTSS_RC_OK;
}

/**
  * \brief Set trap configuration entry
  *
  * \param trap_entry   [IN] : The trap configuration
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_conf_set (vtss_trap_entry_t *trap_entry)
{
    int  conf_id;
    BOOL delete_ = FALSE, update = FALSE;
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
    u32  dying_gasp = fast_cap(MEBA_CAP_DYING_GASP);
#endif

    if (trap_entry == NULL) {
        return SNMP_ERROR_NULL;
    }

    CRIT_SCOPE();
    vtss_trap_entry_t *tmp = get_trap_entry_point(trap_entry);
    if (NULL == tmp && FALSE == trap_entry->valid) {
        return SNMP_ERROR_TRAP_RECV_NOT_EXIST;
    } else if ( tmp ) {
        if (TRUE == trap_entry->valid) {
            update = TRUE;
        }
    } else if ( NULL == (tmp = alloc_trap_entry())) {
        T_D("reach the max conf");
        return VTSS_RC_ERROR;
    }

    if (FALSE == trap_entry->valid) {
        delete_ = TRUE;
    }
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
    if (dying_gasp && delete_) {
        struct DGMappingKey key;
        strcpy(key.trap_name, trap_entry->trap_conf_name);
        loam_trap_entry_del(key);
    }
#endif
    conf_id = tmp->trap_conf.conf_id;
    if ( TRUE == delete_ ) {
        (void) vtss_avl_tree_delete(&trap_entry_avl, (void **)&tmp);
        free_trap_entry(tmp);
    } else {
        memcpy(tmp, trap_entry, sizeof(vtss_trap_entry_t));
        tmp->trap_conf.conf_id = conf_id;
        if ( FALSE == update ) {
            if (vtss_avl_tree_add (&trap_entry_avl, tmp) == FALSE) {
                T_E("Could not add trap entry");
            }
        }
    }

    set_snmpd_traps_updated(true);

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
    if (dying_gasp && !delete_) {
        //create new dying trap whenever conf change
        if (make_oam_dying_gasp_trap_pdu(trap_entry) == VTSS_RC_OK) {
            dying_gasp_get_ready_to_send_trap_to_this_entry(trap_entry, TRUE);
        } else {
            struct DGMappingKey key;
            strcpy(key.trap_name, trap_entry->trap_conf_name);
            loam_trap_entry_del(key);
        }
    }
#endif

    return VTSS_RC_OK;
}

/**
  * \brief Get trap source entry
  *
  * \param trap_source  [INOUT] source_name: Name of the trap source
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_source_get (vtss_trap_source_t *trap_source)
{
    vtss_trap_source_t *tmp;

    if (trap_source == NULL) {
        return SNMP_ERROR_NULL;
    }

    CRIT_SCOPE();
    tmp = get_trap_source_point(trap_source);

    if ( !tmp ) {
        return VTSS_RC_ERROR;
    }
    memcpy(trap_source, tmp, sizeof(vtss_trap_source_t));

    return VTSS_RC_OK;
}

/**
  * \brief Get next trap source entry
  *
  * \param trap_source  [INOUT] source_name: Name of the trap source
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_source_get_next (vtss_trap_source_t *trap_source)
{
    if (trap_source == NULL) {
        return SNMP_ERROR_NULL;
    }

    CRIT_SCOPE();
    vtss_trap_source_t *tmp = get_next_trap_source_point(trap_source);
    if ( !tmp ) {
        T_N("No more entries");
        return VTSS_RC_ERROR;
    }
    memcpy(trap_source, tmp, sizeof(vtss_trap_source_t));

    T_I("Entry found");
    return VTSS_RC_OK;
}

/**
  * \brief Get next filter entry
  *
  * \param trap_source  [INOUT] : The trap source
  * \param filter_id    [INOUT] : Filter ID
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_filter_get_next(vtss_trap_source_t *trap_source, int *filter_id)
{
    T_N("enter");
    if (trap_source == NULL || filter_id == NULL) {
        return SNMP_ERROR_NULL;
    }

    CRIT_SCOPE();
    vtss_trap_source_t *tmp = get_trap_source_point(trap_source);
    (*filter_id)++;
    if ( !tmp ) {
        tmp = get_next_trap_source_point(trap_source);
        *filter_id = 0;
    }
    while (tmp) {
        memcpy(trap_source, tmp, sizeof(vtss_trap_source_t));
        while (*filter_id < VTSS_TRAP_FILTER_MAX) {
            if (trap_source->trap_filter.item[*filter_id]) {
                T_N("exit");
                return VTSS_RC_OK;
            }
            (*filter_id)++;
        };
        tmp = get_next_trap_source_point(trap_source);
        *filter_id = 0;
    };
    T_N("exit");
    return VTSS_RC_ERROR;
}

/**
  * \brief Get filter entry
  *
  * \param trap_source  [IN]  : The trap source
  * \param filter_id    [IN]  : Filter ID
  * \param trap_filter  [OUT] : The trap filter item
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_filter_get(vtss_trap_source_t *trap_source, int filter_id, vtss_trap_filter_item_t *trap_filter)
{
    T_N("enter");
    if (trap_source == NULL) {
        return SNMP_ERROR_NULL;
    }

    CRIT_SCOPE();
    vtss_trap_source_t *tmp = get_trap_source_point(trap_source);
    if ( !tmp ) {
        return VTSS_RC_ERROR;
    }
    memcpy(trap_source, tmp, sizeof(vtss_trap_source_t));
    if (tmp->trap_filter.item[filter_id]) {
        memcpy(trap_filter, tmp->trap_filter.item[filter_id], sizeof(vtss_trap_filter_item_t));
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

/**
  * \brief Set trap source entry
  *
  * \param trap_source  [IN] : The trap source
  * \param trap_filter  [IN] : The trap filter item
  * \param filter_id    [IN] : Filter ID, use -1 for auto
  * \return
  *    VTSS_RC_OK indicates it is successful, otherwise is failed.
  */
mesa_rc trap_mgmt_source_set(vtss_trap_source_t *trap_source, vtss_trap_filter_item_t *trap_filter, int filter_id)
{
    BOOL                        delete_ = FALSE, update = FALSE;
    mesa_rc                     rc;

    if (trap_source == NULL) {
        return SNMP_ERROR_NULL;
    }

    CRIT_SCOPE();
    vtss_trap_source_t *tmp = get_trap_source_point(trap_source);
    if (NULL == tmp && FALSE == trap_source->valid) {
        return SNMP_ERROR_TRAP_SOURCE_NOT_EXIST;
    } else if (tmp) {
        if (TRUE == trap_source->valid) {
            update = TRUE;
        }
    } else if (NULL == (tmp = alloc_trap_source())) {
        T_D("reach the max conf");
        return VTSS_RC_ERROR;
    }

    if (FALSE == trap_source->valid) {
        delete_ = TRUE;
    }
    if (TRUE == delete_) {
        // Remove filter
        u32 cnt, oldcnt = trap_filter_cnt(tmp);;
        if (filter_id < 0) { // Ignore ID
            cnt = free_trap_filter(tmp, trap_filter);
        } else {
            cnt = free_specific_trap_filter(tmp, filter_id);
        }
        if (cnt == 0) { // Remove source
            (void) vtss_avl_tree_delete(&trap_source_avl, (void **)&tmp);
            free_trap_source(tmp);
        }
        if (cnt == oldcnt) { // None removed
            return SNMP_ERROR_TRAP_SOURCE_NOT_EXIST;
        }
    } else {
        if (trap_filter == NULL) {
            return SNMP_ERROR_NULL;
        }
        if (FALSE == update) { // New source
            if ((rc = trap_engine_source_config(trap_source->source_name, TRUE)) != VTSS_RC_OK) {
                free_trap_source(tmp);
                return rc;
            }
            strcpy(tmp->source_name, trap_source->source_name);
            if (vtss_avl_tree_add (&trap_source_avl, tmp) == FALSE) {
                T_E("Could not add trap entry");
            }
        }
        // Add filter
        vtss_trap_filter_item_t *item;
        if (filter_id < 0) { // Ignore ID
            item = get_trap_filter(tmp, trap_filter);
            if (!item) {
                item = alloc_trap_filter(tmp);
            }
        } else {
            item = alloc_specific_trap_filter(tmp, filter_id);
        }
        if (!item) {
            return SNMP_TRAP_FILTER_TABLE_FULL;
        }
        memcpy(item, trap_filter, sizeof(vtss_trap_filter_item_t));
    }

    return VTSS_RC_OK;
}

/**
  * \brief Get trap default configuration entry
  *
  * \param trap_entry   [OUT] : The trap configuration
  */

void trap_mgmt_conf_default_get(vtss_trap_entry_t  *trap_entry)
{

    CRIT_SCOPE();
    vtss_trap_conf_t          *conf = &trap_entry->trap_conf;

    conf->enable = TRAP_CONF_DEFAULT_ENABLE;
    conf->dip.type = VTSS_INET_ADDRESS_TYPE_NONE;
    conf->trap_port = TRAP_CONF_DEFAULT_DPORT;
    conf->trap_version = TRAP_CONF_DEFAULT_VER;
    strcpy(conf->trap_communitySecret, TRAP_CONF_DEFAULT_COMM);
    (void)AUTH_secret_key_cryptography(TRUE, conf->trap_communitySecret, conf->trap_encryptedSecret);
    conf->trap_inform_mode = TRAP_CONF_DEFAULT_INFORM_MODE;
    conf->trap_inform_timeout = TRAP_CONF_DEFAULT_INFORM_TIMEOUT;
    conf->trap_inform_retries = TRAP_CONF_DEFAULT_INFORM_RETRIES;
    conf->trap_engineid_len = snmpv3_get_engineID(conf->trap_engineid, SNMPV3_MAX_ENGINE_ID_LEN);
    strcpy(conf->trap_security_name, TRAP_CONF_DEFAULT_SEC_NAME);
}

struct variable_list *
snmp_bind_var(struct variable_list *prev,
              void *value, int type, size_t sz_val, oid *oidVar, size_t sz_oid)
{
    struct variable_list *var;

    // snmp_free_varbind() is used to free this pointer, once used.
    // Therefore we use an SNMP allocation function, rather than VTSS_MALLOC().
    // SNMP_MALLOC_STRUCT() also zeroes out the allocated structure.
    var = SNMP_MALLOC_STRUCT(variable_list);
    if (!var) {
        T_E("FATAL: cannot VTSS_MALLOC in snmp_bind_var");
        exit(-1);               /* Sorry :( */
    }
    memset(var, 0, sizeof(struct variable_list));
    var->next_variable = prev;
    (void)snmp_set_var_objid(var, oidVar, sz_oid);
    var->type = type;
    (void)snmp_set_var_value(var, (u_char *) value, sz_val);

    return var;
}

/**
  * \brief Send SNMP vars trap
  *
  * \param entry         [IN]: the event OID and variable binding
  *
 */

void snmp_send_vars_trap(snmp_vars_trap_entry_t *entry)
{
    CRIT_SCOPE();
    if (msg_switch_is_primary() && (SNMP_TRAP_BUFF_CNT(trap_vars_buff_read_idx, trap_vars_buff_write_idx) < (SNMP_TRAP_BUFF_MAX_CNT - 1))) {
        snmp_trap_vars_buff[trap_vars_buff_write_idx].trap_entry = *entry;
        trap_vars_buff_write_idx = (trap_vars_buff_write_idx + 1) % SNMP_TRAP_BUFF_MAX_CNT;
    }
}

/*
 * Port state change indication
 */
#ifdef RFC3636_SUPPORTED_MAU
#if RFC3636_SUPPORTED_MAU
extern CapArray<u_long, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> linkdown_counter;
#endif      /*RFC3636_SUPPORTED_MAU*/
#endif // end ifdef
static void snmp_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_isid_t           isid = VTSS_ISID_START;
    snmp_port_conf_t      snmp_port_conf;
    vtss_appl_port_conf_t port_conf;
    snmp_trap_if_info_t   trap_if_info;
    iftable_info_t        table_info;

    if (msg_switch_is_primary()) {
        T_D("port_no: [%d,%u] link %s", isid, port_no, status->link ? "up" : "down");
        if (!msg_switch_exists(isid)) { /* IP interface maybe change, don't send trap */
            return;
        }
        if (snmp_mgmt_snmp_port_conf_get(isid, port_no, &snmp_port_conf) != VTSS_RC_OK) {
            return;
        }

        vtss_ifindex_t ifindex;
        if (vtss_ifindex_from_port(isid, port_no, &ifindex) != VTSS_RC_OK) {
            T_E("Could not get ifindex");
            return;
        };

        if (vtss_appl_port_conf_get(ifindex, &port_conf) != VTSS_RC_OK) {
            return;
        }
        table_info.if_id = port_no;
        table_info.type = IFTABLE_IFINDEX_TYPE_PORT;
        table_info.isid = isid;
        (void) ifIndex_get_by_interface(&table_info);
        trap_if_info.if_idx = table_info.ifIndex;
        trap_if_info.if_admin = port_conf.admin.enable ? 1 : 2;
        trap_if_info.if_oper = status->link ? 1 : 2;
        snmp_mgmt_send_trap(status->link ? SNMP_TRAP_TYPE_LINKUP : SNMP_TRAP_TYPE_LINKDOWN, trap_if_info);
#ifdef RFC3636_SUPPORTED_MAU
#if RFC3636_SUPPORTED_MAU
        if (!status->link) {
            CRIT_SCOPE();
            linkdown_counter[isid - VTSS_ISID_START][port_no]++;

        }
#endif      /*RFC3636_SUPPORTED_MAU*/
#endif // end ifdef
    }
}

/****************************************************************************
 * Module thread
 ****************************************************************************/

static void snmp_trap_thread(vtss_addrword_t data)
{
#ifdef VTSS_SW_OPTION_MSTP
    mstp_bridge_param_t sc;
#endif /* VTSS_SW_OPTION_MSTP */

    // Wait until snmp_agent startup is done
    while (!is_snmpd_boot_finished()) {
        VTSS_OS_MSLEEP(1000);
    }

    /* The stackable device booting will take 3~5 seconds. */
    VTSS_OS_MSLEEP(5000);

#ifdef VTSS_SW_OPTION_MSTP
    if (vtss_appl_mstp_system_config_get(&sc) == VTSS_RC_OK && sc.forceVersion == VTSS_APPL_MSTP_PROTOCOL_VERSION_COMPAT) {
        /* If STP mode is enabled, the port state became forwarding state
           will take about 30 seconds. (2 forwarding delay time)
           In order to prevent trap packet lossing during system booting state.
           We make a simple way to do it.
           Always waiting system booting passed 30 seconds then the SNMP module
           starting sendout trap packets. */
        VTSS_OS_MSLEEP(30000);
    }
#endif /* VTSS_SW_OPTION_MSTP */
    while (!any_ip4_interface) {
        // wait for an IP interface before trying to send any traps
        VTSS_OS_MSLEEP(1000);
    }

    while (1) {
        while (msg_switch_is_primary()) {
            while (trap_buff_read_idx != trap_buff_write_idx) {
                SNMP_CRIT_ENTER(__LINE__);
                snmp_trap_buff_t   trap_buf = snmp_trap_buff[trap_buff_read_idx];
                SNMP_CRIT_EXIT();
                (void)trap_event_send_snmpTraps(&trap_buf);
                SNMP_CRIT_ENTER(__LINE__);
                trap_buff_read_idx = (trap_buff_read_idx + 1) % SNMP_TRAP_BUFF_MAX_CNT;
                SNMP_CRIT_EXIT();
                VTSS_OS_MSLEEP(100);
            }
            while (trap_vars_buff_read_idx != trap_vars_buff_write_idx) {
                SNMP_CRIT_ENTER(__LINE__);
                snmp_vars_trap_entry_t trap_entry = snmp_trap_vars_buff[trap_vars_buff_read_idx].trap_entry;
                SNMP_CRIT_EXIT();
                (void)trap_event_send_vars(&trap_entry);
                snmp_free_varbind(trap_entry.vars);
                SNMP_CRIT_ENTER(__LINE__);
                trap_vars_buff_read_idx = (trap_vars_buff_read_idx + 1) % SNMP_TRAP_BUFF_MAX_CNT;
                SNMP_CRIT_EXIT();
                VTSS_OS_MSLEEP(100);
            }
            VTSS_OS_MSLEEP(1000);
        } // while(msg_switch_is_primary())

        // No reason for using CPU ressources when we're a secondary switch
        T_D("Suspending SNMP trap thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_PRE, VTSS_MODULE_ID_SNMP);
        T_D("Resumed SNMP trap thread");
    } // while(1)
}

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
static void loam_trap_entry_default_set(void)
{
    DG_CRIT_SCOPE();
    T_D("Delete buffered pkt in kernel LIFO (only in Linux) & then delete the loam trap entry cache");
    for (auto it = dg_data.map.begin(); it != dg_data.map.end(); it++) {
        if (it->second.id) {
            vtss_eth_link_oam_del_dying_gasp_pdu(it->second.id);
        }
    }
    dg_data.map.clear();
}
static mesa_rc loam_trap_entry_add(const DGMappingKey key, const DGValue *const value)
{
    if (!value) {
        T_E("value must be preallocated memory");
        return VTSS_RC_ERROR;
    }

    DG_CRIT_SCOPE();
    auto it = dg_data.map.find(key);
    if ((it == dg_data.map.end()) && (dg_data.map.size() == VTSS_TRAP_CONF_MAX)) {
        return VTSS_RC_ERROR;
    }
    auto ret = dg_data.map.insert(DGMap::value_type(key, *value));
    if (ret.second == false) {
        dg_data.map.set(key, *value);
        T_D("Mapping was already present in db & updated");
    } else {
        T_D("Mapping is added to switch");
    }
    return VTSS_RC_OK;
}
static mesa_rc loam_trap_entry_del(const DGMappingKey key)
{
    T_D("Deleting loam trap entry configuration with key:%s", key.trap_name);
    T_D("Delete the kernel buff for this entry");
    DGValue val;
    if (loam_trap_entry_get(key, &val) == VTSS_RC_OK) {
        if (val.id) {
            vtss_eth_link_oam_del_dying_gasp_pdu(val.id);
        }
    }
    DG_CRIT_SCOPE();
    auto del = dg_data.map.erase(key);
    if (del) {
        T_D("Mapping was successfully deleted from the map");
    }

    return del == 1 ? VTSS_RC_OK : VTSS_RC_ERROR;
}
static mesa_rc loam_trap_entry_get(const DGMappingKey key, DGValue *value)
{
    T_D("Switch fetching mapping from the map with key: %s", key.trap_name);

    if (!value) {
        T_E("value must be preallocated memory");
        return VTSS_RC_ERROR;
    }

    DG_CRIT_SCOPE();
    auto it = dg_data.map.find(key);
    if (it != dg_data.map.end()) {
        *value = it->second;
        T_D("Found the requested mapping");
    } else {
        T_D("The requested mapping is not in map.");
    }
    return it != dg_data.map.end() ? VTSS_RC_OK : VTSS_RC_ERROR;
}
#define VTSS_TRAP_DEST_GET(trap_entry_ptr, dest_out) \
    do {\
        vtss_trap_conf_t *cfg = &trap_entry_ptr->trap_conf;\
        memset(dest_out, 0x0, sizeof(dest_out));\
        if (cfg->dip.type == VTSS_INET_ADDRESS_TYPE_IPV4) {\
            (void) misc_ipv4_txt(cfg->dip.address.ipv4, dest_out);\
        } else if (cfg->dip.type == VTSS_INET_ADDRESS_TYPE_IPV6) {\
            (void) misc_ipv6_txt(&(cfg->dip.address.ipv6), dest_out);\
        } else if (cfg->dip.type == VTSS_INET_ADDRESS_TYPE_DNS) {\
            if (strlen(cfg->dip.address.domain_name.name) > 0) {\
                strncpy(dest_out, cfg->dip.address.domain_name.name, 254);\
                dest_out[254] = 0;\
            } else {\
                T_I("Empty string");\
            }\
        } else {\
            T_I("Unknown type");\
        }\
        T_D("Destination Host: %s", dest_out);\
    } while (0)

static mesa_rc vtss_make_l2_to_l4_packet_headers(vtss_trap_entry_t *trap_entry, DGValue *Id);
static mesa_rc loam_new_pkt_to_kernel_add_private(vtss_trap_entry_t *trap_entry, BOOL do_rewrite)
{
    struct DGMappingKey key;
    struct DGValue Id;

    T_I("Creating headers for trap_conf_name:%s rewrite=%i\n", trap_entry->trap_conf_name, do_rewrite);

    strcpy(key.trap_name, trap_entry->trap_conf_name);

    if (loam_trap_entry_get(key, &Id) == VTSS_RC_OK) {
        BOOL is_pingable = FALSE;
        char icmp_dest[255];
        if (do_rewrite || Id.update) {
            VTSS_TRAP_DEST_GET(trap_entry, icmp_dest);
            is_pingable = ping_test_trap_server_exist((trap_entry->trap_conf.dip.type == VTSS_INET_ADDRESS_TYPE_IPV6), icmp_dest, 1);
        }
        if (Id.id) {
            if (is_pingable || do_rewrite) {
                vtss_eth_link_oam_del_dying_gasp_pdu(Id.id);
            } else {
                if (Id.update && !is_pingable) {
                    return VTSS_RC_ERROR;
                }
                return VTSS_RC_OK;
            }
        }
        if (Id.snmp_pdu_length > 0) {
            VTSS_RC(vtss_make_l2_to_l4_packet_headers(trap_entry, &Id));
            if (is_pingable) {
                Id.update = FALSE;
            }
            VTSS_RC(loam_trap_entry_add(key, &Id));
        }
    }

    return VTSS_RC_OK;
}

static mesa_rc dying_gasp_get_ready_to_send_trap_to_this_entry(vtss_trap_entry_t *trap_entry, BOOL do_rewrite)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (!trap_entry) {
        T_E("trap entry ptr must not be null");
        return rc;
    }
    rc = loam_new_pkt_to_kernel_add_private(trap_entry, do_rewrite);
    return rc;
}
static void dying_gasp_timer_thread(vtss_addrword_t data)
{
    BOOL trap_entry_pending = FALSE, do_rewrite = FALSE;
    u32  rewrite = 0;

    while (1) {
        vtss_trap_entry_t        trap_entry;
        // Wait until ICFG_LOADING_PRE event.
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_PRE, VTSS_MODULE_ID_SNMP);
        memset(&trap_entry, 0x0, sizeof(vtss_trap_entry_t));
        strcpy(trap_entry.trap_conf_name, "");
        trap_entry_pending = FALSE;
        while ((trap_mgmt_conf_get_next( &trap_entry) == VTSS_RC_OK)) {
            if (dying_gasp_get_ready_to_send_trap_to_this_entry(&trap_entry, do_rewrite) != VTSS_RC_OK) {
                trap_entry_pending = TRUE;
            }
        }
        do_rewrite = FALSE;
        if (!trap_entry_pending) {
            VTSS_OS_MSLEEP(10000);
            rewrite += 10;
        } else {
            VTSS_OS_MSLEEP(2000);
            rewrite += 2;
        }
        if (rewrite > 180) {
            rewrite = 0;
            do_rewrite = TRUE;
        }
    }
}
#endif // VTSS_SW_OPTION_ETH_LINK_OAM
/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

static void del_all_avl_trap_entry(void)
{
    vtss_trap_entry_t *tmp;

    if (vtss_avl_tree_get(&trap_entry_avl, (void **) &tmp, VTSS_AVL_TREE_GET_FIRST) != TRUE) { // entry not existing
        return;
    }

    do {
        if ( FALSE == vtss_avl_tree_delete (&trap_entry_avl, (void **)&tmp) ) {
            T_E("vtss_avl_tree_delete fail(conf_name = %s)", tmp->trap_conf_name);
            break;
        }
    } while (vtss_avl_tree_get(&trap_entry_avl, (void **) &tmp, VTSS_AVL_TREE_GET_NEXT) == TRUE);

}

static void del_all_avl_trap_source(void)
{
    vtss_trap_source_t *tmp;

    if (vtss_avl_tree_get(&trap_source_avl, (void **) &tmp, VTSS_AVL_TREE_GET_FIRST) != TRUE) { // entry not existing
        return;
    }

    do {
        if ( FALSE == vtss_avl_tree_delete (&trap_source_avl, (void **)&tmp) ) {
            T_E("vtss_avl_tree_delete fail(conf_name = %s)", tmp->source_name);
            break;
        }
    } while (vtss_avl_tree_get(&trap_source_avl, (void **) &tmp, VTSS_AVL_TREE_GET_NEXT) == TRUE);

}

/* Read/create SNMP stack configuration */
static void snmp_conf_read_stack()
{
    snmp_conf_t new_snmp_conf;

    T_D("enter");

    /* Use default values */
    snmp_default_get(&new_snmp_conf);
    CRIT_SCOPE();
    snmp_global.mode = new_snmp_conf.mode;
    SnmpdSetEngineId((char *)new_snmp_conf.engineid, new_snmp_conf.engineid_len);
    set_snmpd_communities_updated(true);

    /* Use default values */
    snmp_global.snmp_smon_stat_entry_num = 0;
    memset(snmp_global.snmp_smon_stat_entry, 0x0, sizeof(snmp_global.snmp_smon_stat_entry));

    /* Clear net-snmp lists */
    struct usmUser *tmp = usm_get_userList();
    while (tmp) {
        tmp = usm_remove_user(tmp);
    }
    vacm_destroyAllGroupEntries();
    vacm_destroyAllAccessEntries();
    vacm_destroyAllViewEntries();
    // Save vtss_snmpd.conf now!
    snmp_store("vtss_snmpd");

    /* Use default values */
    snmpv3_default_communities();
    snmpv3_default_users();
    snmpv3_default_groups();
    snmpv3_default_accesses();
    snmpv3_default_views();

    /* Use default values */
    del_all_avl_trap_entry();
    del_all_avl_trap_source();
    memset(&trap_global, 0, sizeof(trap_global));

    int i;
    vtss_trap_entry_t *tmpe;
    for (i = 0, tmpe = &trap_global.trap_entry[i]; i < VTSS_TRAP_CONF_MAX; i++, tmpe++) {
        if ( TRUE == tmpe->valid) {
            (void) vtss_avl_tree_add(&trap_entry_avl, tmpe);
        }
    }
    vtss_trap_source_t *tmps;
    for (i = 0, tmps = &trap_global.trap_source[i]; i < VTSS_TRAP_SOURCE_MAX; i++, tmps++) {
        if ( TRUE == tmps->valid) {
            (void) vtss_avl_tree_add(&trap_source_avl, tmps);
        }
    }
    T_D("exit");
}

/****************************************************************************
* Linux snmp_agent thread
****************************************************************************/
int vtss_agent_check_and_process(int block, int t)
{
    int             numfds;
    fd_set          fdset;
    struct timeval  timeout = { t, 0 }, *tvp = &timeout;
    int             count;
    int             fakeblock = 0;

    numfds = 0;
    FD_ZERO(&fdset);
    snmp_select_info(&numfds, &fdset, tvp, &fakeblock);
    if (block != 0 && fakeblock != 0) {
        /*
         * There are no alarms registered, and the caller asked for blocking, so
         * let select() block forever.
         */

    } else if (block != 0 && fakeblock == 0) {
        /*
         * The caller asked for blocking, but there is an alarm due sooner than
         * LONG_MAX seconds from now, so use the modified timeout returned by
         * snmp_select_info as the timeout for select().
         */

    } else if (block == 0) {
        /*
         * The caller does not want us to block at all.
         */

        timerclear(tvp);
    }

    count = select(numfds, &fdset, NULL, NULL, tvp);

    if (count > 0) {
        /*
         * packets found, process them
         */
        snmp_read(&fdset);
    } else
        switch (count) {
        case 0:
            snmp_timeout();
            break;
        case -1:
            if (errno != EINTR) {
                snmp_log_perror("select");
            }
            return -1;
        default:
            snmp_log(LOG_ERR, "select returned %d\n", count);
            return -1;
        }                       /* endif -- count>0 */

    /*
     * see if persistent store needs to be saved
     */
    snmp_store_if_needed();

    /*
     * Run requested alarms.
     */
    run_alarms();

    netsnmp_check_outstanding_agent_requests();

    return count;
}

static void snmp_agent_thread(vtss_addrword_t data)
{
    BOOL master_agent_running = false;
    BOOL send_warm_start = false;
    snmp_trap_if_info_t   trap_if_info;
    memset(&trap_if_info, 0x0, sizeof(trap_if_info));
    // Wait until INIT_CMD_INIT is complete.
    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_SNMP);
    while (1) {
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_SNMP);

        if (!any_ip4_interface) {
            T_D("Wait for ip address");
            sleep(1);
            continue;
        }
        /* init our own mibs */
        vtss_snmp_mibs_init();

        /* initialize tcpip */
        SOCK_STARTUP;

        T_I("Starting up");
        /* Remove comments from below two lines to enable debug in net-snmp */
        //snmp_set_do_debugging(1);
        //netsnmp_disable_this_loghandler(snmp_loghandler);

        while (msg_switch_is_primary()) {
            SNMP_CRIT_ENTER(__LINE__);
            if (is_snmpd_communities_updated()) {
                snmpv3_engine_communities_write();
                set_snmpd_communities_updated(false);
            }
            if (is_snmpd_traps_updated()) {
                snmpv3_mgmt_trapsess_write();
                set_snmpd_traps_updated(false);
            }
            if (snmp_global.mode != master_agent_running) {
                if (snmp_global.mode) {
                    (void)init_master_agent();
                    master_agent_running = true;
                    send_warm_start = true;
                } else {
                    shutdown_master_agent();
                    master_agent_running = false;
                }
            }
            SNMP_CRIT_EXIT();
            if (send_warm_start) {
                send_warm_start = false;
                if (is_snmpd_boot_finished()) {
                    snmp_mgmt_send_trap(SNMP_TRAP_TYPE_WARMSTART, trap_if_info);
                }
            }
            set_snmpd_boot_finished();
            vtss_agent_check_and_process(1, 1); /* 0 == don't block */
        }
        snmp_shutdown("vtss_snmpd");
        SOCK_CLEANUP;
        if (master_agent_running) {
            shutdown_master_agent();
            master_agent_running = false;
        }
        shutdown_agent();
    }
}

void snmp_ip_state_change_callback(vtss_ifindex_t if_id)
{
    char buf[256];
    vtss_appl_ip_if_key_ipv4_t key = {};

    any_ip4_interface = (vtss_appl_ip_if_status_ipv4_itr(&key, &key) == VTSS_RC_OK);
    T_I("%s : %s", vtss_ifindex2str(buf, sizeof(buf), key.ifindex), any_ip4_interface ? "IPv4-UP" : "IPv4-DOWN");
}

/* Module start */
static void snmp_start(BOOL init)
{
    mesa_rc      rc;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize SNMP configuration */
        snmp_global.mode = SNMP_DEFAULT_MODE;
        set_snmpd_communities_updated(true);

        /* Initialize message buffers */
        vtss_sem_init(&snmp_global.request.sem, 1);

        /* Create semaphore for critical regions */
        critd_init(&snmp_global.crit, "snmp", VTSS_MODULE_ID_SNMP, CRITD_TYPE_MUTEX);

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
        critd_init(&dg_data.crit, "dying_gasp_data", VTSS_MODULE_ID_SNMP, CRITD_TYPE_MUTEX);
#endif /* VTSS_SW_OPTION_ETH_LINK_OAM */


        /* Initialize mibContextTable semaphore for critical regions */
        mibContextTable_init();
        /* Create SNMP thread */
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           snmp_trap_thread,
                           0,
                           "SNMP Trap",
                           nullptr,
                           0,
                           &snmp_trap_thread_handle,
                           &snmp_trap_thread_block);
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
        if (fast_cap(MEBA_CAP_DYING_GASP)) {
            vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                               dying_gasp_timer_thread,
                               0,
                               "SNMP DYING GASP TIMER",
                               nullptr,
                               0,
                               &dying_gasp_timer_thread_handle,
                               &dying_gasp_timer_thread_block);
        }
#endif

        /* netsnmp static config */
        netsnmp_ds_set_string(NETSNMP_DS_LIBRARY_ID,
                              NETSNMP_DS_LIB_CONFIGURATION_DIR, "/tmp");
        netsnmp_ds_set_string(NETSNMP_DS_LIBRARY_ID,
                              NETSNMP_DS_LIB_PERSISTENT_DIR, "/switch");
#ifdef VTSS_SW_OPTION_IPV6
        netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
                              NETSNMP_DS_AGENT_PORTS, "udp:161,udp6:161");
#endif
        snmp_loghandler = netsnmp_register_loghandler(NETSNMP_LOGHANDLER_STDOUT, LOG_ALERT);
        netsnmp_ds_set_int(NETSNMP_DS_APPLICATION_ID,
                           NETSNMP_DS_AGENT_AGENTX_TIMEOUT, 5);
        netsnmp_ds_set_int(NETSNMP_DS_APPLICATION_ID,
                           NETSNMP_DS_AGENT_AGENTX_RETRIES, 1);

        /* initialize the agent library */
        init_agent("vtss_snmpd");

        /* initialize mib code here */
        init_vacm();
        init_usm();
        init_mib_modules();
        init_snmp("vtss_snmpd");

        vtss_thread_create(VTSS_THREAD_PRIO_BELOW_NORMAL,
                           snmp_agent_thread,
                           0,
                           "SNMP_AGENT",
                           nullptr,
                           0,
                           &snmp_agent_thread_handle,
                           &snmp_agent_thread_block);
    } else {
        /* Register port change callback */
        if ((rc = port_change_register(VTSS_MODULE_ID_SNMP, snmp_port_state_change_callback)) != VTSS_RC_OK) {
            T_E("port_change_register(): failed rc = %d", rc);
        }
        if ((rc = vtss_ip_if_callback_add(snmp_ip_state_change_callback)) != VTSS_RC_OK) {
            T_E("vtss_ip_if_callback_add(): failed rc = %d", rc);
        }

        CRIT_SCOPE();
#if defined(MSCC_BRSDK)
        update_private_enterprise_trap(snmp_private_mib_oid, snmp_private_mib_oid_len_get()); // This requires a patched netsnmp, which is not available on Ubuntu
#endif
    }
    T_D("exit");
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void snmp_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_snmp_json_init();
#endif

extern "C" mesa_rc vtss_appl_snmp_private_trap_init(vtss_init_data_t *data);

static bool snmp_enabled;

bool snmp_module_enabled(void)
{
    return snmp_enabled;
}

extern "C" int trap_icli_cmd_register();
extern "C" int snmp_icli_cmd_register();

/* Initialize module */
mesa_rc snmp_init(vtss_init_data_t *data)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_isid_t isid = data->isid;

    if (data->cmd == INIT_CMD_INIT) {
        snmp_enabled = vtss::appl::main::module_enabled("snmp");
#ifdef VTSS_SW_OPTION_WEB
        if (!snmp_enabled) {
            web_page_disable("snmp_menu");
            web_css_filter_add("snmp");
        }
#endif /* VTSS_SW_OPTION_WEB */
    }
    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    if (!snmp_enabled) {
        T_D("module disabled");
        return VTSS_RC_OK;
    }

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");

        if (ifIndex_init()) {
            T_E("SNMP ifIndex initialization failed");
        }

        snmp_start(1);
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register our private mib */
        snmp_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_snmp_json_init();
#endif
        trap_icli_cmd_register();
        snmp_icli_cmd_register();
        ( void ) trap_init();
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = snmp_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling snmp_icfg_init() failed rc = %s", error_txt(rc));
        }
        if ((rc = trap_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling snmp_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif // defined VTSS_SW_OPTION_ICFG

        break;

    case INIT_CMD_START:
        T_D("START");
        snmp_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            snmp_conf_read_stack();
            snmp_smon_conf_engine_set();
            CRIT_SCOPE();
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
            if (fast_cap(MEBA_CAP_DYING_GASP)) {
                loam_trap_entry_default_set();
            }
#endif /* VTSS_SW_OPTION_ETH_LINK_OAM */
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE: {
        mesa_restart_status_t status;
        mesa_restart_t        restart;
        snmp_trap_if_info_t   trap_if_info;

        T_D("ICFG_LOADING_PRE");

        /* Read stack and switch configuration */
        snmp_conf_read_stack();

        memset(&trap_if_info, 0x0, sizeof(trap_if_info));
        restart = (control_system_restart_status_get(NULL, &status) == VTSS_RC_OK ? status.restart : MESA_RESTART_COLD);
#ifdef VTSS_SW_OPTION_SYSLOG
        S_I("SYS-BOOTING: Switch just made a %s boot.", control_system_restart_to_str(restart));
#endif
        snmp_mgmt_send_trap(restart == MESA_RESTART_COLD ? SNMP_TRAP_TYPE_COLDSTART : SNMP_TRAP_TYPE_WARMSTART, trap_if_info);
        break;
    }

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
#ifdef RFC4133_SUPPORTED_ENTITY
#if RFC4133_SUPPORTED_ENTITY
        rc = entConfigChange(isid);
#endif  /*RFC4133_SUPPORTED_ENTITY*/
#endif // RFC4133_SUPPORTED_ENTITY ifdef
        break;

    default:
        break;
    }

    (void) vtss_appl_snmp_private_trap_init(data);
    return rc;
}

/*lint -esym(459, vtss_snmp_mibs_init) ... We're thread-safe */
void vtss_snmp_mibs_init(void)
{
    // The sysORTable must be initialized first because the rest of MIB inital
    // function will register to sysORTable.
    sysORTable_init();

    // Initialize SNMP MIB redefine table
    snmp_mib_redefine_init();

#if RFC1213_SUPPORTED_SYSTEM
    init_mib2_system();
#endif /* RFC1213_SUPPORTED_SYSTEM */

#if RFC1213_SUPPORTED_INTERFACES
    init_mib2_interfaces();
#endif /* RFC1213_SUPPORTED_INTERFACES */

#if RFC1213_SUPPORTED_IP
    init_mib2_ip();
#endif /* RFC1213_SUPPORTED_IP */

#ifdef RFC2674_SUPPORTED_Q_BRIDGE
    init_rfc2674_q_bridge();
#endif  /* RFC2674_Q_BRIDGE  */

#ifdef RFC4363_SUPPORTED_P_BRIDGE
    init_rfc4363_p_bridge();
#endif  /* RFC2674_Q_BRIDGE  */

#ifdef IEEE8021_Q_BRIDGE
    ieee8021BridgeMib_init(); /* ieee8021QBridgeMib needs to reference ieee8021BridgeBasePortTable */
    ieee8021QBridgeMib_init();
#endif

#if RFC1213_SUPPORTED_ICMP
    init_mib2_icmp();
#endif /* RFC1213_SUPPORTED_ICMP */

#if RFC1213_SUPPORTED_TCP
    init_mib2_tcp();
#endif /* RFC1213_SUPPORTED_TCP */

#if RFC1213_SUPPORTED_UDP
    init_mib2_udp();
#endif /* RFC1213_SUPPORTED_UDP */

#if RFC1213_SUPPORTED_SNMP
    init_mib2_snmp();
#endif /* RFC1213_SUPPORTED_SNMP */

#ifdef RFC2819_SUPPORTED_STATISTICS
    init_rmon_statisticsMIB();
#endif /* RFC2819_SUPPORTED_STATISTICS */

#ifdef RFC2819_SUPPORTED_HISTORY
    init_rmon_historyMIB();
#endif /* RFC2819_SUPPORTED_HISTORY */

#ifdef RFC2819_SUPPORTED_EVENT
    init_rmon_eventMIB();
#endif /* RFC2819_SUPPORTED_EVENT */

#ifdef RFC2819_SUPPORTED_AlARM
    init_rmon_alarmMIB();
#endif /* RFC2819_SUPPORTED_AlARM */

#ifdef VTSS_SW_OPTION_SMON
    init_switchRMON();
#endif /* VTSS_SW_OPTION_SMON */

#if RFC4188_SUPPORTED_DOT1D_BRIDGE
    init_dot1dBridge();
#endif /* RFC4188_SUPPORTED_DOT1D_BRIDGE */

#ifdef VTSS_SW_OPTION_LLDP
#ifdef DOT1AB_LLDP
    init_lldpObjects();
#endif /* DOT1AB_LLDP */
#endif /* VTSS_SW_OPTION_LLDP */

#if RFC3635_SUPPORTED_TRANSMISSION
    init_transmission();
#endif /* RFC36353_SUPPORTED_TRANSMISSION */

#ifdef VTSS_SW_OPTION_DOT1X
#ifdef IEEE8021X_SUPPORTED_MIB
#if IEEE8021X_SUPPORTED_MIB
    init_ieee8021paeMIB();
#endif /* IEE8021X_SUPPORTED_MIB */
#endif /* IEEE8021X_SUPPORTED_MIB */
#endif /* VTSS_SW_OPTION_DOT1X */

#ifdef RFC4133_SUPPORTED_ENTITY
    init_entityMIB();
#endif /* RFC4133_SUPPORTED_ENTITY */

#ifdef RFC3636_SUPPORTED_MAU
#if RFC3636_SUPPORTED_MAU
    init_snmpDot3MauMgt();
#endif /* RFC3636_SUPPORTED_MAU */
#endif /* RFC3636_SUPPORTED_MAU */

#ifdef VTSS_SW_OPTION_SFLOW
    sflow_snmp_init();
#endif /* VTSS_SW_OPTION_SFLOW */

    vtss_snmp_reg_mib_tree();

#if defined(VTSS_SW_OPTION_POE)
#ifdef VTSS_POE_SUPPORT_RFC3621_ENABLE
#endif /* VTSS_SW_OPTION_POE */
#endif /* VTSS_POE_SUPPORT_RFC3621_ENABLE */

#if defined(VTSS_SW_OPTION_IP)
#if defined(RFC4292_SUPPORTED_IPFORWARDMIB)
    init_ipForward();
#endif /*   RFC4292_SUPPORTED_IPFORWARDMIB */
#if defined(RFC4293_SUPPORTED_IPMIB)
    init_ip();
#endif /*   RFC4293_SUPPORTED_IPMIB */
#endif /* VTSS_SW_OPTION_IP */

    /* net-snmp doesn't use this */
    init_snmpMPDStats();
#if RFC3411_SUPPORTED_FRAMEWORK_SNMPENGINE
    init_snmpEngine();
#endif /* RFC3411_SUPPORTED_FRAMEWORK_SNMPENGINE */

#if defined(VTSS_SW_OPTION_CFM)
    ieee8021CfmMib_init();
#endif /* VTSS_SW_OPTION_CFM */

//SMB_MIBs
#ifdef VTSS_SW_OPTION_SMB_SNMP

#ifdef RFC2863_SUPPORTED_IFMIB
#if RFC2863_SUPPORTED_IFMIB
    init_ifMIB();
#endif /* RFC2863_SUPPORTED_IFMIB */
#endif /* RFC2863_SUPPORTED_IFMIB */

#ifdef VTSS_SW_OPTION_LLDP
#ifdef DOT1AB_LLDP
#if defined(VTSS_SW_OPTION_POE) || defined(VTSS_SW_OPTION_LLDP_MED)
    init_lldpXMedMIB();
#endif /* TSS_SW_OPTION_POE */
#endif /* DOT1AB_LLDP */
#endif /* VTSS_SW_OPTION_LLDP */

#ifdef VTSS_SW_OPTION_IGMPS
#ifdef RFC2933_SUPPORTED_IGMP
    init_igmpInterfaceTable();
#endif /* RFC2933_SUPPORTED_IGMP */
#endif /* VTSS_SW_OPTION_IGMPS */

#ifdef VTSS_SW_OPTION_IPMC
#ifdef RFC5519_SUPPORTED_MGMD
    init_mgmdMIBObjects();
#endif /* RFC5519_SUPPORTED_MGMD */
#endif /* VTSS_SW_OPTION_IPMC */

#ifdef RFC4668_SUPPORTED_RADIUS
#if RFC4668_SUPPORTED_RADIUS
    init_radiusAuthClientMIBObjects();
#endif /* RFC4668_SUPPORTED_RADIUS */
#endif /* RFC4668_SUPPORTED_RADIUS */

#ifdef VTSS_SW_OPTION_LACP
    init_lagMIBObjects();
#endif /* VTSS_SW_OPTION_LACP */

#ifdef RFC4670_SUPPORTED_RADIUS
#if RFC4670_SUPPORTED_RADIUS
    init_radiusAccClientMIBObjects();
#endif /* RFC4670_SUPPORTED_RADIUS */
#endif /* RFC4670_SUPPORTED_RADIUS */

#ifdef VTSS_SW_OPTION_MSTP
#if IEEE8021_MSTP
    ieee8021MstpMib_init();
#endif
#endif /* VTSS_SW_OPTION_MSTP */

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#if RFC4878_SUPPORTED_ETH_LINK_OAM
    init_dot3OamMIB();
#endif /* VTSS_SW_OPTION_ETH_LINK_OAM */
#endif  /* RFC4878_SUPPORTED_ETH_LINK_OAM */

#if RFC3414_SUPPORTED_USMSTATS
    init_usmStats();
#endif /* RFC3414_SUPPORTED_USMSTATS */
#if RFC3414_SUPPORTED_USMUSER
    init_usmUser();
#endif /* RFC3414_SUPPORTED_USMUSER */
#if RFC3415_SUPPORTED_VACMCONTEXTTABLE
    init_vacmContextTable();
#endif /* RFC3415_SUPPORTED_VACMCONTEXTTABLE */
#if RFC3415_SUPPORTED_VACMSECURITYTOGROUPTABLE
    init_vacmSecurityToGroupTable();
#endif /* RFC3415_SUPPORTED_VACMSECURITYTOGROUPTABLE */
#if RFC3415_SUPPORTED_VACMACCESSTABLE
    init_vacmAccessTable();
#endif /* RFC3415_SUPPORTED_VACMACCESSTABLE */
#if RFC3415_SUPPORTED_VACMMIBVIEWS
    init_vacmMIBViews();
#endif /* RFC3415_SUPPORTED_VACMMIBVIEWS */

#ifdef VTSS_SW_OPTION_P802_1_AS
    ieee8021AsTimeSyncMib_init();
#endif /* VTSS_SW_OPTION_P802_1_AS */

#if defined(VTSS_SW_OPTION_TSN)
    if (vtss_appl_tsn_capabilities->has_tas) {
        ieee8021STMib_init();
    }
#if defined(VKTBD)
    if (vtss_appl_qos_capabilities->has_psfp) {
        ieee8021PSFPMib_init();
    }
#endif /* defined(VKTBD) */
#endif

#if defined(VTSS_SW_OPTION_TSN)
    if (fast_cap(MESA_CAP_QOS_FRAME_PREEMPTION)) {
        ieee8021PreemptionMib_init();
    }
#endif

#ifdef VTSS_SW_OPTION_FRR_OSPF
    if (frr_has_ospfd()) {
        rfc4750_ospf_init();
    }
#endif

#ifdef VTSS_SW_OPTION_FRR_RIP
    if (frr_has_ripd()) {
        rfc1724_rip2_init();
    }
#endif

#endif /* VTSS_SW_OPTION_SMB_SNMP */
}

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#define OUT_BUF_MAX_SIZE (((sizeof(u32)*8)/7) + 1) /* i.e. 5 */
u8 encode_base128(const u32 value, u8 *out)
{
    u8  octet = 0;
    u8  size  = 0;
    u8  buf[OUT_BUF_MAX_SIZE];
    u8  *buf_p = &buf[0];

    memset(&buf, 0, sizeof(buf));

    u32 temp = htonl(value);

    /* Convert to Little Endian */
    temp = ((temp & 0xff000000) >> 24) |
           ((temp & 0x00ff0000) >> 8)  |
           ((temp & 0x0000ff00) << 8)  |
           ((temp & 0xff) << 24);

    bool set_msb = false;
    do {
        octet = temp & 0x7f;
        temp >>= 7;
        *buf_p++    = (set_msb == true) ? (octet | 0x80) : octet;
        set_msb     = (temp != 0) ? true : false;
        size++;
    } while (temp != 0);

    VTSS_ASSERT(size <= OUT_BUF_MAX_SIZE);
    for (int i = size - 1; i >= 0; i--) {
        if (buf[i] != 0) {
            *out++ = buf[i];
        }
    }
    return size;
}

#include "vtss_dns.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include "packet_api.h"
#include<netinet/if_ether.h>
#include <linux/if_arp.h>


#define PKT_BUFSIZE     1514
#define PKT_LLH_LEN     14
#define PKT_IP_LEN      20
#define PKT_UDP_LEN     sizeof(dns_udp_hdr)
#define PKT_ETHTYPE_IP  0x0800
#define PKT_PROTO_UDP   0x11

static mesa_rc vtss_get_my_ip_conf(u32 *ip, int *vlan)
{
    vtss_appl_ip_if_status_t ipv4;
    vtss_ifindex_t           prev_ifindex, ifindex;
    mesa_rc                  rc = VTSS_RC_ERROR;

    if (!ip || !vlan) {
        return rc;
    }

    prev_ifindex = VTSS_IFINDEX_NONE;
    while (vtss_appl_ip_if_itr(&prev_ifindex, &ifindex, true /* only VLAN interfaces */) == VTSS_RC_OK) {
        prev_ifindex = ifindex;

        *vlan = vtss_ifindex_as_vlan(ifindex);

        if ((rc = vtss_appl_ip_if_status_get(ifindex, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &ipv4)) == VTSS_RC_OK) {
            *ip = ipv4.u.ipv4.net.address;
            break;
        }
    }

    return rc;
}

// Find gateway MAC address
static mesa_rc vtss_get_gateway_mac_addr(uchar *mac_addr)
{
#define RT_BUF_SIZE 2048
    static mesa_ipv4_network_t          zero_network;
    vtss_appl_ip_neighbor_key_t         prev_nb_key, nb_key;
    vtss_appl_ip_neighbor_status_t      nb_status;
    vtss_appl_ip_route_status_map_t     rts;
    vtss_appl_ip_route_status_map_itr_t itr;
    u32                                 j;
    char                                ip_str_buf[16];
    mesa_ip_addr_t                      dest_ip;
    bool                                found = false, first;

    VTSS_RC(vtss_appl_ip_route_status_get_all(rts, VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC));

    for (itr = rts.begin(); itr != rts.end(); ++itr) {
        if (itr->first.route.route.ipv4_uc.network == zero_network && itr->first.route.route.ipv4_uc.destination != 0) {
            T_D("Found gateway IPv4 %s", itr->first.route.route.ipv4_uc);
            dest_ip.addr.ipv4 = itr->first.route.route.ipv4_uc.destination;
            found = true;
            break;
        }
    }

    if (!found) {
        return VTSS_RC_ERROR;
    }

    // Lookup MAC address in ARP table
    found = false;
    first = true;
    while (vtss_appl_ip_neighbor_itr(first ? nullptr : &prev_nb_key, &nb_key, MESA_IP_TYPE_IPV4) == VTSS_RC_OK) {
        first       = false;
        prev_nb_key = nb_key;

        if (nb_key.dip.addr.ipv4 != dest_ip.addr.ipv4) {
            continue;
        }

        if (vtss_appl_ip_neighbor_status_get(&nb_key, &nb_status) != VTSS_RC_OK) {
            continue;
        }

        T_D("Found gateway IP address %s MAC address %s", misc_ipv4_txt((mesa_ipv4_t)nb_key.dip.addr.ipv4, ip_str_buf), misc_mac2str(nb_status.dmac.addr));
        for (j = 0; j < 6; j++) {
            mac_addr[j] = nb_status.dmac.addr[j];
        }

        found = true;
        break;
    }

    if (!found) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

static mesa_rc vtss_make_l2_to_l4_packet_headers(vtss_trap_entry_t *trap_entry, DGValue *Id)
{
    static uchar            t = 0;
    u8                      pkt_buf[PKT_BUFSIZE + 2];
    size_t                  pkt_len;
    dns_eth_hdr             *eth_hdr;
    dns_ip_hdr              *ip_hdr;
    dns_udp_hdr             *udp_hdr;
    uchar                   mac_addr[6];
    int                     vlan;
    mesa_rc                 rc;
    mesa_ip_addr_t          sip, dip;
    uint16_t                udp_len;

    if (!trap_entry || !Id) {
        return VTSS_RC_ERROR;
    }

    vtss_trap_conf_t         *cfg = &trap_entry->trap_conf;
    char                     dip_name[255];

    //get destination gateway mac address
    rc = vtss_get_gateway_mac_addr(mac_addr);
    if (rc != VTSS_RC_OK) {
        T_D("vtss_get_gateway_mac_addr failed");
        return rc;
    }

    eth_hdr = (dns_eth_hdr *)pkt_buf;
    ip_hdr = (dns_ip_hdr *)&pkt_buf[PKT_LLH_LEN];

    eth_hdr->dest.addr[0] = mac_addr[0];
    eth_hdr->dest.addr[1] = mac_addr[1];
    eth_hdr->dest.addr[2] = mac_addr[2];
    eth_hdr->dest.addr[3] = mac_addr[3];
    eth_hdr->dest.addr[4] = mac_addr[4];
    eth_hdr->dest.addr[5] = mac_addr[5];

    (void) conf_mgmt_mac_addr_get(mac_addr, 0);
    memcpy(eth_hdr->src.addr, mac_addr, sizeof(uchar) * 6);
    eth_hdr->type = htons(PKT_ETHTYPE_IP);

    ip_hdr->vhl = 0x45;
    ip_hdr->tos = 0x00;
    ip_hdr->ipid[0] = 0x00;
    ip_hdr->ipid[1] = ++t;
    ip_hdr->ipoffset[0] = 0x00;
    ip_hdr->ipoffset[1] = 0x00;
    ip_hdr->ttl = 128;
    ip_hdr->proto = PKT_PROTO_UDP;

    sip.type = MESA_IP_TYPE_IPV4;
    if (vtss_get_my_ip_conf(&sip.addr.ipv4, &vlan) != VTSS_RC_OK) {
        T_I("Not able to get my IP, can't make packet");
        return VTSS_RC_ERROR;
    }

    ip_hdr->srcipaddr = htonl(sip.addr.ipv4);

    VTSS_TRAP_DEST_GET(trap_entry, dip_name);
    dip.type = MESA_IP_TYPE_IPV4;
    dip.addr.ipv4 = ntohl(inet_addr(dip_name));

    ip_hdr->destipaddr = htonl(dip.addr.ipv4);
    T_D("Hostname(of trap destination): %s\n", dip_name);

    /* UDP Header */
    udp_hdr = (dns_udp_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN];
    udp_hdr->sport = htons(1260);//randomly picked
    udp_hdr->dport = htons(cfg->trap_port);

    /* Payload */
    u8 *payload = &pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN];

    int payload_len = Id->snmp_pdu_length;
    memcpy(payload, Id->snmp_pdu_buf, payload_len);

    ip_hdr->len = htons(PKT_IP_LEN + PKT_UDP_LEN + payload_len);
    ip_hdr->ipchksum = 0;
    ip_hdr->ipchksum = htons(vtss_ip_checksum(&pkt_buf[PKT_LLH_LEN], PKT_IP_LEN));

    /* UDP checksum related  */
    udp_len = PKT_UDP_LEN + payload_len;
    udp_hdr->ulen = htons(udp_len);
    udp_hdr->csum = 0;
    udp_hdr->csum = htons(vtss_ip_pseudo_header_checksum(&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN], udp_len, sip, dip, IP_PROTO_UDP));

    pkt_len = PKT_LLH_LEN + PKT_IP_LEN + udp_len;

    /* Send as raw pkt over management vlan */
    if ((Id->id = vtss_eth_link_oam_add_dying_gasp_trap(vlan, pkt_buf, pkt_len)) < 0) {
        T_E("Invalid id:%d\n", Id->id);
        return VTSS_RC_ERROR;
    }

    T_D("Packet Added frame to kernel buffer(vlan:%d) - OK, TRAP ID: %d\n", vlan, Id->id);
    return VTSS_RC_OK;
}

static mesa_rc make_oam_dying_gasp_trap_pdu(vtss_trap_entry_t *trap_entry)
{

    T_D("Make a new DG SNMP TRAP PDU for trap entry: %s", trap_entry->trap_conf_name);

    if (!trap_entry->trap_conf.enable) {
        T_D("DG: Trap entry not enabled");
        return VTSS_RC_ERROR;
    }
    if (trap_entry->trap_conf.trap_version > SNMP_MGMT_VERSION_2C) {
        T_D("DG: V3 traps not supported yet");
        return VTSS_RC_ERROR;
    }
    if (trap_entry->trap_conf.dip.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
        T_D("DG: IPv6 not supported yet");
        return VTSS_RC_ERROR;
    }

    u32 trapOid[]                   = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};/* 11 */
    u32 dyGaspTrapOid[]             = {1, 3, 6, 1, 2, 1, 158, 0, 2}; /* 9 */
    u32 dot3OamEventLogOui_oid[]    = {1, 3, 6, 1, 2, 1, 158, 1, 6, 1, 3};
    u32 eventTypeOid[]              = {1, 3, 6, 1, 2, 1, 158, 1, 6, 1, 4};/* 11 */
    u32 sysUpTimeOid[]              = {1, 3, 6, 1, 2, 1, 1, 3, 0};
    int sysUpTimeValIdx             = 0;

    const char *community = trap_entry->trap_conf.trap_communitySecret;
    u8 snmp_version       = SNMP_VERSION_2c;/*Only v2c is supported as of now */
    u8 mac[6]             = {0};
    u8 oui[3]             = {0};

    (void) conf_mgmt_mac_addr_get(mac, 0);
    oui[0] = mac[0];
    oui[1] = mac[1];
    oui[2] = mac[2];
    u16 eventType = htons(257);

    u32 length = 0;
    u8  payload[256];
    memset(payload, '\0', sizeof(payload));

    {/* Seq TAG: 0 */
        VTSS_SEQ_TAG_START(payload, length, SEQUENCE_TAG);
        {/* TAG: 0, payload: 0*/
            VTSS_VAR_BIND_TLV(payload, length, ASN_INTEGER, 1, &snmp_version);
            VTSS_VAR_BIND_TLV(payload, length, ASN_OCTET_STR, strlen(community), community);

            VTSS_SEQ_TAG_START(payload, length, SNMP_MSG_TRAP2);
            {
                u8 request_id = 1, err_status = 0, err_index = 0;
                VTSS_VAR_BIND_TLV(payload, length, ASN_INTEGER, sizeof(request_id), &request_id);
                VTSS_VAR_BIND_TLV(payload, length, ASN_INTEGER, sizeof(err_status), &err_status);
                VTSS_VAR_BIND_TLV(payload, length, ASN_INTEGER, sizeof(err_index), &err_index);

                /* Sequence tag to hold varbindings */
                VTSS_SNMP_SEQ_BLOCK(payload, length, SEQUENCE_TAG,
                                    /* varbindings */
                                    u32 sysUpTimeValue    = htonl(vtss::uptime_milliseconds() / 10);
                                    VTSS_VAR_BIND(payload, length, sysUpTimeOid, TimeTicks_TAG, sizeof(sysUpTimeValue), &sysUpTimeValue);
                                    sysUpTimeValIdx = (length - sizeof(sysUpTimeValue));
                                    VTSS_OID_BIND(payload, length, trapOid, dyGaspTrapOid);
                                    VTSS_VAR_BIND(payload, length, eventTypeOid, ASN_UNSIGNED, sizeof(eventType), &eventType);
                                    VTSS_VAR_BIND(payload, length, dot3OamEventLogOui_oid, ASN_OCTET_STR, sizeof(oui), oui);
                                   );
            }
            VTSS_SEQ_TAG_END(payload, length);
        }
        VTSS_SEQ_TAG_END(payload, length);
    }

    struct DGMappingKey key;
    struct DGValue Id;
    memset(&Id, 0, sizeof(DGValue));
    strcpy(key.trap_name, trap_entry->trap_conf_name);
    loam_trap_entry_get(key, &Id);
    Id.sysUpTimeValueOffset = sysUpTimeValIdx;
    Id.snmp_pdu_length = length;
    Id.update = TRUE;
    memcpy(Id.snmp_pdu_buf, payload, sizeof(payload));
    loam_trap_entry_add(key, &Id);

    T_D("Created PDU of length:%d, for entry: %s\n", length, trap_entry->trap_conf_name);
    return VTSS_RC_OK;
}
void vtss_snmp_dying_gasp_trap_send_handler(void)
{
    printf("%%Note: Not supported for linux, use following commands for linux debuggging ->\n");
    printf("debug sym write icpu_cfg:intr:intr_force 4\n");
    printf("%%Note: If you want to see kernel dyeing gasp buffer content, then use commands below ->\n");
    printf("debug system shell\ncat /proc/vtss_dying_gasp\n");
}
#endif /* VTSS_SW_OPTION_ETH_LINK_OAM */

/**
 * \brief Set SNMP configuration
 *
 * To Set the SNMP global configuration.
 *
 * \param conf      [IN]    The global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_conf_set(const vtss_appl_snmp_conf_t *const conf)
{
    snmp_conf_t snmp_conf;
    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    snmp_conf = *(snmp_conf_t *)conf;
    return snmp_mgmt_snmp_conf_set(&snmp_conf);
}

/**
 * \brief Get SNMP configuration
 *
 * To Get the SNMP global configuration.
 *
 * \param conf      [OUT]    The global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_conf_get(vtss_appl_snmp_conf_t *const conf)
{
    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    return snmp_mgmt_snmp_conf_get((snmp_conf_t *)conf);
}

#define CHECK_SNMP_COMMUNITY_PARAM(CONF_NAME, CONF) \
    do {\
        if (!(CONF)) { \
            T_W("%s is NULL", #CONF); \
            return SNMP_ERROR_NULL; \
        }\
        CHECK_SNMP_COMMUNITY_IDX(CONF_NAME);\
        if (strlen((CONF)->secret) > VTSS_APPL_SNMP_MAX_NAME_LEN) {\
            return SNMPV3_ERROR_SEC_NAME_TOO_LONG;\
        }\
    } while(0)

#define CHECK_SNMP_COMMUNITY_IDX(CONF_NAME) \
    do {\
        if (strlen((CONF_NAME)->name) > VTSS_APPL_SNMP_MAX_NAME_LEN) {\
            return SNMPV3_ERROR_COMMUNITY_TOO_LONG;\
        }\
    } while(0)

/**
 * \brief Iterate function of SNMP community table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_itr(
    const vtss_appl_snmp_community_index_t   *const prev_index,
    vtss_appl_snmp_community_index_t         *const next_index
)
{
    mesa_rc                     rc = VTSS_RC_OK;
    snmpv3_communities_conf_t   snmp_conf;

    if (!next_index) {
        return SNMP_ERROR_NULL;
    }

    memset(&snmp_conf, 0, sizeof(snmp_conf));
    if (prev_index) {
        CHECK_SNMP_COMMUNITY_IDX(prev_index);
        strcpy(snmp_conf.community, prev_index->name);
        snmp_conf.sip.address.type = MESA_IP_TYPE_IPV4;
        snmp_conf.sip.address.addr.ipv4 = prev_index->sip.address;
        snmp_conf.sip.prefix_size = prev_index->sip.prefix_size;
    }

    do {
        if ((rc = snmpv3_mgmt_communities_conf_get(&snmp_conf, TRUE)) != VTSS_RC_OK) {
            return rc;
        }
    } while (snmp_conf.sip.address.type != MESA_IP_TYPE_IPV4);
    strcpy(next_index->name, snmp_conf.community);
    next_index->sip.address = snmp_conf.sip.address.addr.ipv4;
    next_index->sip.prefix_size = snmp_conf.sip.prefix_size;
    T_D("name = %s", next_index->name);
    return rc;
}

/**
 * \brief Get the entry of SNMP community table
 *
 * To get the specific entry in SNMP community table.
 *
 * \param conf_name     [IN]    (key) Index of the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_get(
    const vtss_appl_snmp_community_index_t   conf_index,
    vtss_appl_snmp_community_conf_t          *const conf
)
{
    mesa_rc                     rc;
    snmpv3_communities_conf_t   snmp_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter");
    CHECK_SNMP_COMMUNITY_IDX(&conf_index);

    strcpy(snmp_conf.community, conf_index.name);
    snmp_conf.sip.address.type = MESA_IP_TYPE_IPV4;
    snmp_conf.sip.address.addr.ipv4 = conf_index.sip.address;
    snmp_conf.sip.prefix_size = conf_index.sip.prefix_size;
    T_D("key = %s", snmp_conf.community);
    if ((rc = snmpv3_mgmt_communities_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        T_D("entry not found");
        return rc;
    }

    strcpy(conf->secret, snmp_conf.communitySecret);

    return VTSS_RC_OK;
}

/**
 * \brief Set the entry of SNMP community table
 *
 * To modify the specific entry in SNMP community table.
 *
 * \param conf_index     [IN]    (key) Index of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_community_set(
    const vtss_appl_snmp_community_index_t   conf_index,
    const vtss_appl_snmp_community_conf_t    *const conf
)
{
    mesa_rc                     rc;
    snmpv3_communities_conf_t   snmp_conf;

    CHECK_SNMP_COMMUNITY_PARAM(&conf_index, conf);

    strcpy(snmp_conf.community, conf_index.name);
    snmp_conf.sip.address.type = MESA_IP_TYPE_IPV4;
    snmp_conf.sip.address.addr.ipv4 = conf_index.sip.address;
    snmp_conf.sip.prefix_size = conf_index.sip.prefix_size;
    if ((rc = snmpv3_mgmt_communities_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    if (strlen(conf->secret) == 0) {
        strcpy(snmp_conf.communitySecret, conf_index.name);
    } else {
        strcpy(snmp_conf.communitySecret, conf->secret);
    }
    if ((rc = AUTH_secret_key_cryptography(TRUE, snmp_conf.communitySecret, snmp_conf.encryptedSecret)) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    return snmpv3_mgmt_communities_conf_set(&snmp_conf);
}

/**
 * \brief Delete the entry of SNMP community table
 *
 * To delete the specific entry in SNMP community table.
 *
 * \param conf_index      [IN]    (key) Index of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_community_del(
    const vtss_appl_snmp_community_index_t   conf_index
)
{
    snmpv3_communities_conf_t snmp_conf;

    T_D("enter");
    CHECK_SNMP_COMMUNITY_IDX(&conf_index);

    T_D("key = %s", conf_index.name);
    strcpy(snmp_conf.community, conf_index.name);
    snmp_conf.sip.address.type = MESA_IP_TYPE_IPV4;
    snmp_conf.sip.address.addr.ipv4 = conf_index.sip.address;
    snmp_conf.sip.prefix_size = conf_index.sip.prefix_size;

    if (snmpv3_mgmt_communities_conf_get(&snmp_conf, FALSE) != VTSS_RC_OK) {
        T_D("entry not found");
        return VTSS_RC_OK;
    }

    snmp_conf.valid         = FALSE;
    return snmpv3_mgmt_communities_conf_set(&snmp_conf);
}

/**
 * \brief Add new entry of SNMP community table
 *
 * To Add new entry in SNMP community table.
 *
 * \param conf_index     [IN]    (key) Index of the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_community_add(
    const vtss_appl_snmp_community_index_t   conf_index,
    const vtss_appl_snmp_community_conf_t    *const conf
)
{
    snmpv3_communities_conf_t   snmp_conf;

    T_D("enter");
    CHECK_SNMP_COMMUNITY_PARAM(&conf_index, conf);

    memset(&snmp_conf, 0, sizeof(snmp_conf));
    strcpy(snmp_conf.community, conf_index.name);
    snmp_conf.sip.address.type = MESA_IP_TYPE_IPV4;
    snmp_conf.sip.address.addr.ipv4 = conf_index.sip.address;
    snmp_conf.sip.prefix_size = conf_index.sip.prefix_size;

    T_D("key = %s", snmp_conf.community);

    if (snmpv3_mgmt_communities_conf_get(&snmp_conf, FALSE) == VTSS_RC_OK) {
        T_D("entry exists");
        return SNMPV3_ERROR_COMMUNITY_ALREADY_EXIST;
    }

    if (strlen(conf->secret) == 0) {
        strcpy(snmp_conf.communitySecret, conf_index.name);
    } else {
        strcpy(snmp_conf.communitySecret, conf->secret);
    }
    if (AUTH_secret_key_cryptography(TRUE, snmp_conf.communitySecret, snmp_conf.encryptedSecret) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    snmp_conf.valid         = TRUE;
    snmp_conf.storage_type  = SNMP_MGMT_STORAGE_NONVOLATILE;
    snmp_conf.status        = SNMP_MGMT_ROW_ACTIVE;
    return snmpv3_mgmt_communities_conf_set(&snmp_conf);
}

/**
 * \brief Get default value of SNMP community table
 *
 * To add new entry in SNMP community table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP community entry.
 * \param conf          [OUT]    The new entry of SNMP community table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_default(
    vtss_appl_snmp_community_index_t  *const conf_index,
    vtss_appl_snmp_community_conf_t   *const conf
)
{

    if (!conf_index || !conf) {
        T_D("conf_index or conf is NULL");
        return SNMP_ERROR_NULL;
    }
    memset(conf_index, 0, sizeof(*conf_index));
    strcpy(conf->secret, "");
    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of SNMP community table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_itr(
    const vtss_appl_snmp_community6_index_t   *const prev_index,
    vtss_appl_snmp_community6_index_t         *const next_index
)
{
    mesa_rc                     rc = VTSS_RC_OK;
    snmpv3_communities_conf_t   snmp_conf;

    if (!next_index) {
        return SNMP_ERROR_NULL;
    }

    memset(&snmp_conf, 0, sizeof(snmp_conf));
    if (prev_index) {
        CHECK_SNMP_COMMUNITY_IDX(prev_index);
        strcpy(snmp_conf.community, prev_index->name);
        snmp_conf.sip.address.type = MESA_IP_TYPE_IPV6;
        snmp_conf.sip.address.addr.ipv6 = prev_index->sip_ipv6.address;
        snmp_conf.sip.prefix_size = prev_index->sip_ipv6.prefix_size;
    }

    do {
        if ((rc = snmpv3_mgmt_communities_conf_get(&snmp_conf, TRUE)) != VTSS_RC_OK) {
            return rc;
        }
    } while (snmp_conf.sip.address.type != MESA_IP_TYPE_IPV6);
    strcpy(next_index->name, snmp_conf.community);
    next_index->sip_ipv6.address = snmp_conf.sip.address.addr.ipv6;
    next_index->sip_ipv6.prefix_size = snmp_conf.sip.prefix_size;
    T_D("name = %s", next_index->name);
    return rc;
}

/**
 * \brief Get the entry of SNMP community table
 *
 * To get the specific entry in SNMP community table.
 *
 * \param conf_name     [IN]    (key) Index of the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_get(
    const vtss_appl_snmp_community6_index_t   conf_index,
    vtss_appl_snmp_community_conf_t          *const conf
)
{
    mesa_rc                     rc;
    snmpv3_communities_conf_t   snmp_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    T_D("enter");
    CHECK_SNMP_COMMUNITY_IDX(&conf_index);

    strcpy(snmp_conf.community, conf_index.name);
    snmp_conf.sip.address.type = MESA_IP_TYPE_IPV6;
    snmp_conf.sip.address.addr.ipv6 = conf_index.sip_ipv6.address;
    snmp_conf.sip.prefix_size = conf_index.sip_ipv6.prefix_size;
    T_D("key = %s", snmp_conf.community);
    if ((rc = snmpv3_mgmt_communities_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        T_D("entry not found");
        return rc;
    }

    strcpy(conf->secret, snmp_conf.communitySecret);

    return VTSS_RC_OK;
}

/**
 * \brief Set the entry of SNMP community table
 *
 * To modify the specific entry in SNMP community table.
 *
 * \param conf_index     [IN]    (key) Index of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_community6_set(
    const vtss_appl_snmp_community6_index_t   conf_index,
    const vtss_appl_snmp_community_conf_t    *const conf
)
{
    mesa_rc                     rc;
    snmpv3_communities_conf_t   snmp_conf;

    CHECK_SNMP_COMMUNITY_PARAM(&conf_index, conf);

    strcpy(snmp_conf.community, conf_index.name);
    snmp_conf.sip.address.type = MESA_IP_TYPE_IPV6;
    snmp_conf.sip.address.addr.ipv6 = conf_index.sip_ipv6.address;
    snmp_conf.sip.prefix_size = conf_index.sip_ipv6.prefix_size;
    if ((rc = snmpv3_mgmt_communities_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    if (strlen(conf->secret) == 0) {
        strcpy(snmp_conf.communitySecret, conf_index.name);
    } else {
        strcpy(snmp_conf.communitySecret, conf->secret);
    }
    if (AUTH_secret_key_cryptography(TRUE, snmp_conf.communitySecret, snmp_conf.encryptedSecret) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    return snmpv3_mgmt_communities_conf_set(&snmp_conf);
}

/**
 * \brief Delete the entry of SNMP community table
 *
 * To delete the specific entry in SNMP community table.
 *
 * \param conf_index      [IN]    (key) Index of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_community6_del(
    const vtss_appl_snmp_community6_index_t   conf_index
)
{
    snmpv3_communities_conf_t snmp_conf;

    T_D("enter");
    CHECK_SNMP_COMMUNITY_IDX(&conf_index);

    T_D("key = %s", conf_index.name);
    strcpy(snmp_conf.community, conf_index.name);
    snmp_conf.sip.address.type = MESA_IP_TYPE_IPV6;
    snmp_conf.sip.address.addr.ipv6 = conf_index.sip_ipv6.address;
    snmp_conf.sip.prefix_size = conf_index.sip_ipv6.prefix_size;

    if (snmpv3_mgmt_communities_conf_get(&snmp_conf, FALSE) != VTSS_RC_OK) {
        T_D("entry not found");
        return VTSS_RC_OK;
    }

    snmp_conf.valid         = FALSE;
    return snmpv3_mgmt_communities_conf_set(&snmp_conf);
}

/**
 * \brief Add new entry of SNMP community table
 *
 * To Add new entry in SNMP community table.
 *
 * \param conf_index     [IN]    (key) Index of the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_community6_add(
    const vtss_appl_snmp_community6_index_t   conf_index,
    const vtss_appl_snmp_community_conf_t    *const conf
)
{
    snmpv3_communities_conf_t   snmp_conf;

    T_D("enter");
    CHECK_SNMP_COMMUNITY_PARAM(&conf_index, conf);

    memset(&snmp_conf, 0, sizeof(snmp_conf));
    strcpy(snmp_conf.community, conf_index.name);
    snmp_conf.sip.address.type = MESA_IP_TYPE_IPV6;
    snmp_conf.sip.address.addr.ipv6 = conf_index.sip_ipv6.address;
    snmp_conf.sip.prefix_size = conf_index.sip_ipv6.prefix_size;

    T_D("key = %s", snmp_conf.community);

    if (snmpv3_mgmt_communities_conf_get(&snmp_conf, FALSE) == VTSS_RC_OK) {
        T_D("entry exists");
        return SNMPV3_ERROR_COMMUNITY_ALREADY_EXIST;
    }

    if (strlen(conf->secret) == 0) {
        strcpy(snmp_conf.communitySecret, conf_index.name);
    } else {
        strcpy(snmp_conf.communitySecret, conf->secret);
    }
    if (AUTH_secret_key_cryptography(TRUE, snmp_conf.communitySecret, snmp_conf.encryptedSecret) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    snmp_conf.valid         = TRUE;
    snmp_conf.storage_type  = SNMP_MGMT_STORAGE_NONVOLATILE;
    snmp_conf.status        = SNMP_MGMT_ROW_ACTIVE;
    return snmpv3_mgmt_communities_conf_set(&snmp_conf);
}

/**
 * \brief Get default value of SNMP community table
 *
 * To add new entry in SNMP community table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP community entry.
 * \param conf          [OUT]    The new entry of SNMP community table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_default(
    vtss_appl_snmp_community6_index_t  *const conf_index,
    vtss_appl_snmp_community_conf_t   *const conf
)
{

    if (!conf_index || !conf) {
        T_D("conf_index or conf is NULL");
        return SNMP_ERROR_NULL;
    }
    memset(conf_index, 0, sizeof(*conf_index));
    strcpy(conf->secret, "");
    return VTSS_RC_OK;
}

#define CHECK_SNMP_USER_PARAM(CONF_IDX, CONF) \
    do {\
        if (!(CONF)) { \
            T_W("%s is NULL", #CONF); \
            return SNMP_ERROR_NULL;   \
        }\
        CHECK_SNMP_USER_IDX(CONF_IDX);\
    } while(0)

#define CHECK_SNMP_USER_IDX(CONF_IDX) \
    do {\
        if ((CONF_IDX)->engineid_len > VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN || \
            strlen((CONF_IDX)->user_name) > VTSS_APPL_SNMP_MAX_NAME_LEN) { \
            return SNMP_ERROR_PARM; \
        }\
    } while(0)

/**
 * \brief Iterate function of SNMP user table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_itr(
    const vtss_appl_snmp_user_index_t    *const prev_index,
    vtss_appl_snmp_user_index_t          *const next_index
)
{
    snmpv3_users_conf_t    snmp_conf;

    if (!next_index) {
        return SNMP_ERROR_NULL;
    }

    if (prev_index) {
        CHECK_SNMP_USER_IDX(prev_index);
        snmp_conf.engineid_len = prev_index->engineid_len;
        memcpy(snmp_conf.engineid, prev_index->engineid, snmp_conf.engineid_len);
        strcpy(snmp_conf.user_name, prev_index->user_name);
    } else {
        memset(&snmp_conf, 0, sizeof(snmp_conf));
    }

    VTSS_RC(snmpv3_mgmt_users_conf_get(&snmp_conf, TRUE));

    next_index->engineid_len = snmp_conf.engineid_len;
    memcpy(next_index->engineid, snmp_conf.engineid, snmp_conf.engineid_len);
    strcpy(next_index->user_name, snmp_conf.user_name);

    return VTSS_RC_OK;
}

/**
 * \brief Get the entry of SNMP user table
 *
 * To get the specific entry in SNMP user table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP user entry.
 * \param conf          [OUT]   The current entry of the SNMP user entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_user_get(
    const vtss_appl_snmp_user_index_t  conf_index,
    vtss_appl_snmp_user_conf_t         *const conf
)
{
    snmpv3_users_conf_t    snmp_conf;
    T_NG(TRACE_GRP_USERS, "conf_index.engineid_len:%d", conf_index.engineid_len);

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    CHECK_SNMP_USER_IDX(&conf_index);

    snmp_conf.engineid_len = conf_index.engineid_len;
    memcpy(snmp_conf.engineid, conf_index.engineid, snmp_conf.engineid_len);
    strcpy(snmp_conf.user_name, conf_index.user_name);

    VTSS_RC(snmpv3_mgmt_users_conf_get(&snmp_conf, FALSE));

    conf->security_level = (vtss_appl_snmp_security_level_t)snmp_conf.security_level;
    conf->auth_protocol = (vtss_appl_snmp_auth_protocol_t)snmp_conf.auth_protocol;
    strcpy(conf->auth_password, "");
    conf->priv_protocol = (vtss_appl_snmp_priv_protocol_t)snmp_conf.priv_protocol;
    strcpy(conf->priv_password, "");
    return VTSS_RC_OK;
}

/**
 * \brief Set the entry of SNMP user table
 *
 * To modify the specific entry in SNMP user table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP user entry.
 * \param conf          [IN]    The revised the SNMP user entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_user_set(
    const vtss_appl_snmp_user_index_t  conf_index,
    const vtss_appl_snmp_user_conf_t   *const conf
)
{
    snmpv3_users_conf_t    snmp_conf;

    T_D("enter");
    CHECK_SNMP_USER_PARAM(&conf_index, conf);

    snmp_conf.engineid_len = conf_index.engineid_len;
    memcpy(snmp_conf.engineid, conf_index.engineid, snmp_conf.engineid_len);
    strcpy(snmp_conf.user_name, conf_index.user_name);

    VTSS_RC(snmpv3_mgmt_users_conf_get(&snmp_conf, FALSE));

    snmp_conf.security_level = conf->security_level;
    snmp_conf.auth_protocol = conf->auth_protocol;
    if (strlen(conf->auth_password) > 0) {
        snmp_conf.auth_password_encrypted = FALSE;
        strcpy(snmp_conf.auth_password, conf->auth_password);
    }
    snmp_conf.priv_protocol = conf->priv_protocol;
    if (strlen(conf->priv_password) > 0) {
        snmp_conf.priv_password_encrypted = FALSE;
        strcpy(snmp_conf.priv_password, conf->priv_password);
    }
    T_D("exit");
    return snmpv3_mgmt_users_conf_set(&snmp_conf);
}

/**
 * \brief Delete the entry of SNMP user table
 *
 * To delete the specific entry in SNMP user table.
 *
 * \param conf_index      [IN]    (key) Name of the SNMP user entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_user_del(
    const vtss_appl_snmp_user_index_t  conf_index
)
{
    snmpv3_users_conf_t    snmp_conf;

    T_NG(TRACE_GRP_USERS, "conf_index.engineid_len:%d", conf_index.engineid_len);

    CHECK_SNMP_USER_IDX(&conf_index);

    snmp_conf.engineid_len = conf_index.engineid_len;
    memcpy(snmp_conf.engineid, conf_index.engineid, snmp_conf.engineid_len);
    strcpy(snmp_conf.user_name, conf_index.user_name);

    VTSS_RC(snmpv3_mgmt_users_conf_get(&snmp_conf, FALSE)); // Get the user requested (return user not found is the user doesn't exist)

    snmp_conf.valid = FALSE; // Mark for deletion

    return snmpv3_mgmt_users_conf_set(&snmp_conf);
}

/**
 * \brief Add new entry of SNMP user table
 *
 * To add new entry in SNMP user table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP user entry.
 * \param conf          [IN]    The new entry of SNMP user table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_user_add(
    const vtss_appl_snmp_user_index_t  conf_index,
    const vtss_appl_snmp_user_conf_t   *const conf
)
{
    snmpv3_users_conf_t    snmp_conf;

    CHECK_SNMP_USER_PARAM(&conf_index, conf);

    memset(&snmp_conf, 0, sizeof(snmp_conf)); // Clear to make sure we are at a know state.

    snmp_conf.engineid_len = conf_index.engineid_len;
    memcpy(snmp_conf.engineid, conf_index.engineid, snmp_conf.engineid_len);
    strcpy(snmp_conf.user_name, conf_index.user_name);

    if (snmpv3_mgmt_users_conf_get(&snmp_conf, FALSE) == VTSS_RC_OK) {
        return SNMPV3_ERROR_USER_ALREADY_EXIST;
    }

    snmp_conf.security_level = conf->security_level;
    snmp_conf.auth_protocol = conf->auth_protocol;
    snmp_conf.auth_password_encrypted = FALSE;
    strcpy(snmp_conf.auth_password, conf->auth_password);
    snmp_conf.priv_protocol = conf->priv_protocol;
    snmp_conf.priv_password_encrypted = FALSE;
    strcpy(snmp_conf.priv_password, conf->priv_password);
    snmp_conf.storage_type  = SNMP_MGMT_STORAGE_NONVOLATILE;
    snmp_conf.status        = SNMP_MGMT_ROW_ACTIVE;
    snmp_conf.valid         = TRUE;

    return snmpv3_mgmt_users_conf_set(&snmp_conf);
}

/**
 * \brief Get default value of SNMP user table
 *
 * To add new entry in SNMP user table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP user entry.
 * \param conf          [OUT]    The new entry of SNMP user table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_default(
    vtss_appl_snmp_user_index_t  *const conf_index,
    vtss_appl_snmp_user_conf_t   *const conf
)
{

    if (!conf_index || !conf) {
        T_D("conf_index or conf is NULL");
        return SNMP_ERROR_NULL;
    }
    memset(conf_index, 0, sizeof(*conf_index));
    conf_index->engineid_len = VTSS_APPL_SNMP_ENGINE_ID_MIN_LEN;
    conf->security_level = VTSS_APPL_SNMP_SECURITY_LEVEL_NOAUTH;
    conf->auth_protocol = VTSS_APPL_SNMP_AUTH_PROTOCOL_NONE;
    strcpy(conf->auth_password, "");
    conf->priv_protocol = VTSS_APPL_SNMP_PRIV_PROTOCOL_NONE;
    strcpy(conf->priv_password, "");
    return VTSS_RC_OK;
}

#define CHECK_SNMP_GROUP_PARAM(CONF_IDX, CONF) \
    do {\
        if (!(CONF)) { \
            T_W("%s is NULL", #CONF); \
            return SNMP_ERROR_NULL; \
        }\
        CHECK_SNMP_GROUP_IDX(CONF_IDX);\
        if (strlen((CONF)->access_group_name) > VTSS_APPL_SNMP_MAX_NAME_LEN) { \
            return SNMP_ERROR_PARM; \
        }\
    } while(0)

#define CHECK_SNMP_GROUP_IDX(CONF_IDX) \
    do {\
        if (strlen((CONF_IDX)->user_or_community) > VTSS_APPL_SNMP_MAX_NAME_LEN) { \
            return SNMP_ERROR_PARM; \
        }\
    } while(0)

/**
 * \brief Iterate function of SNMP group table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN] vtss_appl_snmp_user_to_access_group_get previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_to_access_group_itr(
    const vtss_appl_snmp_user_to_access_group_index_t  *const prev_index,
    vtss_appl_snmp_user_to_access_group_index_t        *const next_index
)
{
    mesa_rc                 rc;
    snmpv3_groups_conf_t    snmp_conf;

    T_D("enter");
    if (!next_index) {
        return SNMP_ERROR_NULL;
    }

    if (prev_index) {
        CHECK_SNMP_GROUP_IDX(prev_index);
        snmp_conf.security_model = prev_index->security_model;
        strcpy(snmp_conf.security_name, prev_index->user_or_community);
    } else {
        memset(&snmp_conf, 0, sizeof(snmp_conf));
    }

    if ((rc = snmpv3_mgmt_groups_conf_get(&snmp_conf, true)) != VTSS_RC_OK) {
        return rc;
    }

    next_index->security_model = (vtss_appl_snmp_security_model_t)snmp_conf.security_model;
    strcpy(next_index->user_or_community, snmp_conf.security_name);
    return rc;
}

/**
 * \brief Get the entry of SNMP group table
 *
 * To get the specific entry in SNMP group table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP group entry.
 * \param conf          [OUT]   The current entry of the SNMP group entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_user_to_access_group_get(
    const vtss_appl_snmp_user_to_access_group_index_t  conf_index,
    vtss_appl_snmp_user_to_access_group_conf_t         *const conf
)
{
    mesa_rc                 rc;
    snmpv3_groups_conf_t    snmp_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    CHECK_SNMP_GROUP_IDX(&conf_index);

    snmp_conf.security_model = conf_index.security_model;
    strcpy(snmp_conf.security_name, conf_index.user_or_community);

    if ((rc = snmpv3_mgmt_groups_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    strcpy(conf->access_group_name, snmp_conf.group_name);
    return rc;
}

/**
 * \brief Set the entry of SNMP group table
 *
 * To modify the specific entry in SNMP group table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP group entry.
 * \param conf          [IN]    The revised the SNMP group entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_user_to_access_group_set(
    const vtss_appl_snmp_user_to_access_group_index_t  conf_index,
    const vtss_appl_snmp_user_to_access_group_conf_t   *const conf
)
{
    mesa_rc                 rc;
    snmpv3_groups_conf_t    snmp_conf;

    CHECK_SNMP_GROUP_PARAM(&conf_index, conf);

    snmp_conf.security_model = conf_index.security_model;
    strcpy(snmp_conf.security_name, conf_index.user_or_community);

    if ((rc = snmpv3_mgmt_groups_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    strcpy(snmp_conf.group_name, conf->access_group_name);
    return snmpv3_mgmt_groups_conf_set(&snmp_conf);
}

/**
 * \brief Delete the entry of SNMP group table
 *
 * To delete the specific entry in SNMP group table.
 *
 * \param conf_index      [IN]    (key) Name of the SNMP group entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_user_to_access_group_del(
    const vtss_appl_snmp_user_to_access_group_index_t  conf_index
)
{
    snmpv3_groups_conf_t    snmp_conf;

    CHECK_SNMP_GROUP_IDX(&conf_index);

    snmp_conf.security_model = conf_index.security_model;
    strcpy(snmp_conf.security_name, conf_index.user_or_community);
    snmp_conf.valid = FALSE;
    return snmpv3_mgmt_groups_conf_set(&snmp_conf);
}

/**
 * \brief Add new entry of SNMP group table
 *
 * To add new entry in SNMP group table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP group entry.
 * \param conf          [IN]    The new entry of SNMP group table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_user_to_access_group_add(
    const vtss_appl_snmp_user_to_access_group_index_t  conf_index,
    const vtss_appl_snmp_user_to_access_group_conf_t   *const conf
)
{
    snmpv3_groups_conf_t    snmp_conf;

    CHECK_SNMP_GROUP_PARAM(&conf_index, conf);

    memset(&snmp_conf, 0, sizeof(snmp_conf)); // Clear to make sure we are at a know state.

    snmp_conf.security_model = conf_index.security_model;
    strcpy(snmp_conf.security_name, conf_index.user_or_community);

    if (snmpv3_mgmt_groups_conf_get(&snmp_conf, FALSE) == VTSS_RC_OK) {
        return SNMPV3_ERROR_GROUP_ALREADY_EXIST;
    }

    strcpy(snmp_conf.group_name, conf->access_group_name);
    snmp_conf.storage_type  = SNMP_MGMT_STORAGE_NONVOLATILE;
    snmp_conf.status        = SNMP_MGMT_ROW_ACTIVE;
    snmp_conf.valid         = TRUE;

    return snmpv3_mgmt_groups_conf_set(&snmp_conf);
}

/**
 * \brief Get default value of SNMP group table
 *
 * To add new entry in SNMP user table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP group entry.
 * \param conf          [OUT]    The new entry of SNMP group table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */

mesa_rc
vtss_appl_snmp_user_to_access_group_default(
    vtss_appl_snmp_user_to_access_group_index_t  *const conf_index,
    vtss_appl_snmp_user_to_access_group_conf_t   *const conf
)
{
    if (!conf_index || !conf) {
        T_D("conf_index or conf is NULL");
        return SNMP_ERROR_NULL;
    }

    memset(conf_index, 0, sizeof(*conf_index));
    strcpy(conf->access_group_name, SNMPV3_DEFAULT_RO_GROUP);
    return VTSS_RC_OK;
}

#define CHECK_SNMP_VIEW_PARAM(CONF_IDX, CONF) \
    do {\
        if (!(CONF)) { \
            T_W("%s is NULL", #CONF); \
            return SNMP_ERROR_NULL; \
        }\
        CHECK_SNMP_VIEW_IDX(CONF_IDX); \
    } while(0)

#define CHECK_SNMP_VIEW_IDX(CONF_IDX) \
    do {\
        if (strlen((CONF_IDX)->view_name) > VTSS_APPL_SNMP_MAX_NAME_LEN || \
            strlen((CONF_IDX)->subtree) > VTSS_APPL_SNMP_MAX_SUBTREE_STR_LEN || \
            FALSE == misc_parse_oid_prefix((CONF_IDX)->subtree, NULL)) { \
            return SNMP_ERROR_PARM; \
        }\
    } while(0)

/**
 * \brief Iterate function of SNMP view table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_view_itr(
    const vtss_appl_snmp_view_index_t  *const prev_index,
    vtss_appl_snmp_view_index_t        *const next_index
)
{
    mesa_rc                rc = VTSS_RC_OK;
    snmpv3_views_conf_t    snmp_conf;

    if (!next_index) {
        return SNMP_ERROR_NULL;
    }

    if (prev_index) {
        T_D("(%s, %s)", prev_index->view_name, prev_index->subtree);
        CHECK_SNMP_VIEW_IDX(prev_index);

        strcpy(snmp_conf.view_name, prev_index->view_name);
        strcpy(snmp_conf.subtree, prev_index->subtree);
    } else {
        memset(&snmp_conf, 0, sizeof(snmp_conf));
    }

    T_D("(%s, %s)", snmp_conf.view_name, snmp_conf.subtree);

    if ((rc = snmpv3_mgmt_views_conf_get(&snmp_conf, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    T_D("(%s, %s)", snmp_conf.view_name, snmp_conf.subtree);
    strcpy(next_index->view_name, snmp_conf.view_name);
    strcpy(next_index->subtree, snmp_conf.subtree);
    return rc;
}

/**
 * \brief Get the entry of SNMP view table
 *
 * To get the specific entry in SNMP view table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP view entry.
 * \param conf          [OUT]   The current entry of the SNMP view entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_view_get(
    const vtss_appl_snmp_view_index_t  conf_index,
    vtss_appl_snmp_view_conf_t         *const conf
)
{
    mesa_rc                rc;
    snmpv3_views_conf_t    snmp_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    CHECK_SNMP_VIEW_IDX(&conf_index);

    strcpy(snmp_conf.view_name, conf_index.view_name);
    strcpy(snmp_conf.subtree, conf_index.subtree);

    if ((rc = snmpv3_mgmt_views_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    conf->view_type = (vtss_appl_snmp_view_type_t)snmp_conf.view_type;
    return rc;
}

/**
 * \brief Set the entry of SNMP view table
 *
 * To modify the specific entry in SNMP view table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP view entry.
 * \param conf          [IN]    The revised the SNMP view entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_view_set(
    const vtss_appl_snmp_view_index_t  conf_index,
    const vtss_appl_snmp_view_conf_t   *const conf
)
{
    mesa_rc                rc;
    snmpv3_views_conf_t    snmp_conf;

    CHECK_SNMP_VIEW_PARAM(&conf_index, conf);

    strcpy(snmp_conf.view_name, conf_index.view_name);
    strcpy(snmp_conf.subtree, conf_index.subtree);

    if ((rc = snmpv3_mgmt_views_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    snmp_conf.view_type = conf->view_type;
    return snmpv3_mgmt_views_conf_set(&snmp_conf);
}

/**
 * \brief Delete the entry of SNMP view table
 *
 * To delete the specific entry in SNMP view table.
 *
 * \param conf_index      [IN]    (key) Name of the SNMP view entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_view_del(
    const vtss_appl_snmp_view_index_t  conf_index
)
{
    snmpv3_views_conf_t    snmp_conf;

    CHECK_SNMP_VIEW_IDX(&conf_index);

    if (FALSE == misc_parse_oid_prefix(conf_index.subtree, NULL)) {
        return SNMP_ERROR_PARM;
    }

    strcpy(snmp_conf.view_name, conf_index.view_name);
    strcpy(snmp_conf.subtree, conf_index.subtree);
    snmp_conf.valid = FALSE;

    return snmpv3_mgmt_views_conf_set(&snmp_conf);
}

/**
 * \brief Add new entry of SNMP view table
 *
 * To add new entry in SNMP view table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP view entry.
 * \param conf          [IN]    The new entry of SNMP view table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_view_add(
    const vtss_appl_snmp_view_index_t  conf_index,
    const vtss_appl_snmp_view_conf_t   *const conf
)
{
    snmpv3_views_conf_t    snmp_conf;

    CHECK_SNMP_VIEW_PARAM(&conf_index, conf);

    memset(&snmp_conf, 0, sizeof(snmp_conf)); // Clear to make sure we are at a know state.

    strcpy(snmp_conf.view_name, conf_index.view_name);
    strcpy(snmp_conf.subtree, conf_index.subtree);

    if (snmpv3_mgmt_views_conf_get(&snmp_conf, FALSE) == VTSS_RC_OK) {
        return SNMPV3_ERROR_VIEW_ALREADY_EXIST;
    }
    snmp_conf.view_type     = conf->view_type;
    snmp_conf.storage_type  = SNMP_MGMT_STORAGE_NONVOLATILE;
    snmp_conf.status        = SNMP_MGMT_ROW_ACTIVE;
    snmp_conf.valid         = TRUE;
    return snmpv3_mgmt_views_conf_set(&snmp_conf);
}

/**
 * \brief Get default value of SNMP view table
 *
 * To add new entry in SNMP view table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP view entry.
 * \param conf          [OUT]    The new entry of SNMP view table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_view_default(
    vtss_appl_snmp_view_index_t  *const conf_index,
    vtss_appl_snmp_view_conf_t   *const conf
)
{
    if (!conf_index || !conf) {
        T_D("conf_index or conf is NULL");
        return SNMP_ERROR_NULL;
    }
    memset(conf_index, 0, sizeof(*conf_index));
    conf->view_type = VTSS_APPL_SNMP_VIEW_TYPE_INCLUDED;
    return VTSS_RC_OK;
}

#define CHECK_SNMP_ACCESS_PARAM(CONF_IDX, CONF) \
    do {\
        if (!(CONF)) { \
            T_W("%s is NULL", #CONF); \
            return SNMP_ERROR_NULL; \
        }\
        CHECK_SNMP_ACCESS_IDX(CONF_IDX); \
    } while(0)

#define CHECK_SNMP_ACCESS_IDX(CONF_IDX) \
    do {\
        if (strlen((CONF_IDX)->access_group_name) > SNMPV3_MAX_NAME_LEN || \
            (CONF_IDX)->security_model > VTSS_APPL_SNMP_SECURITY_MODEL_USM || \
            (CONF_IDX)->security_level > VTSS_APPL_SNMP_SECURITY_LEVEL_AUTHPRIV) { \
            return SNMP_ERROR_PARM; \
        }\
    } while(0)

/**
 * \brief Iterate function of SNMP access table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_access_group_itr(
    const vtss_appl_snmp_access_group_index_t    *const prev_index,
    vtss_appl_snmp_access_group_index_t          *const next_index
)
{
    mesa_rc                 rc = VTSS_RC_OK;
    snmpv3_accesses_conf_t  snmp_conf;

    if (!next_index) {
        return SNMP_ERROR_NULL;
    }

    if (prev_index) {
        CHECK_SNMP_ACCESS_IDX(prev_index);
        strcpy(snmp_conf.group_name, prev_index->access_group_name);
        snmp_conf.security_model = prev_index->security_model;
        snmp_conf.security_level = prev_index->security_level;
    } else {
        memset(&snmp_conf, 0, sizeof(snmp_conf));
    }

    if ((rc = snmpv3_mgmt_accesses_conf_get(&snmp_conf, TRUE)) != VTSS_RC_OK) {
        return rc;
    }

    strcpy(next_index->access_group_name, snmp_conf.group_name);
    next_index->security_model = (vtss_appl_snmp_security_model_t)snmp_conf.security_model;
    next_index->security_level = (vtss_appl_snmp_security_level_t)snmp_conf.security_level;
    return rc;

}

/**
 * \brief Get the entry of SNMP access table
 *
 * To get the specific entry in SNMP access table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP access entry.
 * \param conf          [OUT]   The current entry of the SNMP access entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_access_group_get(
    const vtss_appl_snmp_access_group_index_t  conf_index,
    vtss_appl_snmp_access_group_conf_t         *const conf
)
{
    mesa_rc                 rc;
    snmpv3_accesses_conf_t  snmp_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    CHECK_SNMP_ACCESS_IDX(&conf_index);

    strcpy(snmp_conf.group_name, conf_index.access_group_name);
    snmp_conf.security_model = conf_index.security_model;
    snmp_conf.security_level = conf_index.security_level;

    if ((rc = snmpv3_mgmt_accesses_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    strcpy(conf->read_view_name, snmp_conf.read_view_name);
    strcpy(conf->write_view_name, snmp_conf.write_view_name);
    return rc;
}

/**
 * \brief Set the entry of SNMP access table
 *
 * To modify the specific entry in SNMP access table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP access entry.
 * \param conf          [IN]    The revised the SNMP access entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_access_group_set(
    const vtss_appl_snmp_access_group_index_t  conf_index,
    const vtss_appl_snmp_access_group_conf_t   *const conf
)
{
    mesa_rc                 rc;
    snmpv3_accesses_conf_t    snmp_conf;

    CHECK_SNMP_ACCESS_PARAM(&conf_index, conf);

    strcpy(snmp_conf.group_name, conf_index.access_group_name);
    snmp_conf.security_model = conf_index.security_model;
    snmp_conf.security_level = conf_index.security_level;

    if ((rc = snmpv3_mgmt_accesses_conf_get(&snmp_conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    strcpy(snmp_conf.read_view_name, conf->read_view_name);
    strcpy(snmp_conf.write_view_name, conf->write_view_name);
    return snmpv3_mgmt_accesses_conf_set(&snmp_conf);
}


/**
 * \brief Delete the entry of SNMP access table
 *
 * To delete the specific entry in SNMP access table.
 *
 * \param conf_index      [IN]    (key) Name of the SNMP access entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_access_group_del(
    const vtss_appl_snmp_access_group_index_t  conf_index
)
{
    snmpv3_accesses_conf_t    snmp_conf;

    CHECK_SNMP_ACCESS_IDX(&conf_index);

    strcpy(snmp_conf.group_name, conf_index.access_group_name);
    snmp_conf.security_model = conf_index.security_model;
    snmp_conf.security_level = conf_index.security_level;
    snmp_conf.valid = FALSE;

    return snmpv3_mgmt_accesses_conf_set(&snmp_conf);
}

/**
 * \brief Add new entry of SNMP access table
 *
 * To add new entry in SNMP access table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP access entry.
 * \param conf          [IN]    The new entry of SNMP access table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_snmp_access_group_add(
    const vtss_appl_snmp_access_group_index_t  conf_index,
    const vtss_appl_snmp_access_group_conf_t   *const conf
)
{
    snmpv3_accesses_conf_t    snmp_conf;

    CHECK_SNMP_ACCESS_PARAM(&conf_index, conf);

    memset(&snmp_conf, 0, sizeof(snmp_conf)); // Clear to make sure we are at a know state.

    strcpy(snmp_conf.group_name, conf_index.access_group_name);
    snmp_conf.security_model = conf_index.security_model;
    snmp_conf.security_level = conf_index.security_level;

    if (snmpv3_mgmt_accesses_conf_get(&snmp_conf, FALSE) == VTSS_RC_OK) {
        return SNMPV3_ERROR_ACCESS_ALREADY_EXIST;
    }

    strcpy(snmp_conf.read_view_name, conf->read_view_name);
    strcpy(snmp_conf.write_view_name, conf->write_view_name);

    strcpy(snmp_conf.notify_view_name, SNMPV3_NONAME);
    snmp_conf.context_match = SNMPV3_MGMT_CONTEX_MATCH_EXACT;
    snmp_conf.storage_type  = SNMP_MGMT_STORAGE_NONVOLATILE;
    snmp_conf.status        = SNMP_MGMT_ROW_ACTIVE;
    snmp_conf.valid         = TRUE;

    return snmpv3_mgmt_accesses_conf_set(&snmp_conf);
}

/**
 * \brief Get default value of SNMP access table
 *
 * To add new entry in SNMP user table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP access entry.
 * \param conf          [OUT]    The new entry of SNMP access table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */

mesa_rc
vtss_appl_snmp_access_group_default(
    vtss_appl_snmp_access_group_index_t  *const conf_index,
    vtss_appl_snmp_access_group_conf_t   *const conf
)
{
    if (!conf_index || !conf) {
        T_D("conf_index or conf is NULL");
        return SNMP_ERROR_NULL;
    }

    memset(conf_index, 0, sizeof(*conf_index));
    conf_index->security_level = VTSS_APPL_SNMP_SECURITY_LEVEL_NOAUTH;
    strcpy(conf->read_view_name, SNMPV3_DEFAULT_VIEW);
    strcpy(conf->write_view_name, SNMPV3_NONAME);
    return VTSS_RC_OK;
}

#define CHECK_TRAP_RECEIVER_PARAM(CONF_IDX, CONF) \
    do {\
        if (!(CONF)) { \
            T_W("%s is NULL", #CONF); \
            return SNMP_ERROR_NULL; \
        }\
        if ((CONF)->version > VTSS_APPL_SNMP_VERSION_3 || \
            (CONF)->dest_addr.type == VTSS_INET_ADDRESS_TYPE_NONE || \
            (CONF)->dest_addr.type > VTSS_INET_ADDRESS_TYPE_DNS || \
            (((CONF)->dest_addr.type == VTSS_INET_ADDRESS_TYPE_DNS) && (misc_str_is_domainname((CONF)->dest_addr.address.domain_name.name) != VTSS_RC_OK)) || \
            strlen((CONF)->community) > VTSS_APPL_SNMP_COMMUNITY_LEN || \
            (CONF)->notify_type > VTSS_APPL_TRAP_NOTIFY_INFORM || \
            (CONF)->timeout > VTSS_APPL_SNMP_TRAP_TIMEOUT_MAX || \
            (CONF)->retries > VTSS_APPL_SNMP_TRAP_RETRIES_MAX || \
            (CONF)->engineid_len > VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN || \
            strlen((CONF)->user_name) > VTSS_APPL_SNMP_MAX_NAME_LEN) { \
            return SNMP_ERROR_PARM; \
        }\
        CHECK_TRAP_RECEIVER_IDX(CONF_IDX); \
    } while(0)

#define CHECK_TRAP_RECEIVER_IDX(CONF_IDX) \
    do {\
        if (strlen((CONF_IDX)->name) > VTSS_APPL_SNMP_MAX_NAME_LEN) { \
            return SNMP_ERROR_PARM; \
        }\
    } while(0)

/**
 * \brief Iterate function of SNMP TRAP receiver table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_trap_receiver_itr(
    const vtss_appl_trap_receiver_index_t    *const prev_index,
    vtss_appl_trap_receiver_index_t          *const next_index
)
{
    mesa_rc                 rc = VTSS_RC_OK;
    vtss_trap_entry_t       snmp_conf;

    if (!next_index) {
        return SNMP_ERROR_NULL;
    }

    if (prev_index) {
        CHECK_TRAP_RECEIVER_IDX(prev_index);
        strcpy(snmp_conf.trap_conf_name, prev_index->name);
    } else {
        memset(&snmp_conf, 0, sizeof(snmp_conf));
    }

    if ((rc = trap_mgmt_conf_get_next(&snmp_conf)) != VTSS_RC_OK) {
        return rc;
    }

    strcpy(next_index->name, snmp_conf.trap_conf_name);
    return rc;

}

/**
 * \brief Get the entry of SNMP TRAP receiver table
 *
 * To get the specific entry in SNMP TRAP receiver table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP TRAP receiver entry.
 * \param conf          [OUT]   The current entry of the SNMP TRAP receiver entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_receiver_get(
    const vtss_appl_trap_receiver_index_t  conf_index,
    vtss_appl_trap_receiver_conf_t         *const conf
)
{
    mesa_rc                 rc;
    vtss_trap_entry_t       snmp_conf;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    CHECK_TRAP_RECEIVER_IDX(&conf_index);

    strcpy(snmp_conf.trap_conf_name, conf_index.name);

    if ((rc = trap_mgmt_conf_get(&snmp_conf)) != VTSS_RC_OK) {
        return rc;
    }

    conf->enable = snmp_conf.trap_conf.enable;
    memcpy(&conf->dest_addr, &snmp_conf.trap_conf.dip, sizeof(vtss_inet_address_t));
    conf->port = snmp_conf.trap_conf.trap_port;
    conf->version = (vtss_appl_snmp_version_t) snmp_conf.trap_conf.trap_version;
    strcpy(conf->community, snmp_conf.trap_conf.trap_communitySecret);
    conf->notify_type = (vtss_appl_trap_notify_type_t) snmp_conf.trap_conf.trap_inform_mode;
    conf->timeout = snmp_conf.trap_conf.trap_inform_timeout;
    conf->retries = snmp_conf.trap_conf.trap_inform_retries;
    conf->engineid_len = snmp_conf.trap_conf.trap_engineid_len;
    memcpy(conf->engineid, snmp_conf.trap_conf.trap_engineid, snmp_conf.trap_conf.trap_engineid_len);
    strcpy(conf->user_name, snmp_conf.trap_conf.trap_security_name);
    return rc;
}

/**
 * \brief Set the entry of SNMP TRAP receiver table
 *
 * To modify the specific entry in SNMP TRAP receiver table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP TRAP receiver entry.
 * \param conf          [IN]    The revised the SNMP TRAP receiver entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_receiver_set(
    const vtss_appl_trap_receiver_index_t  conf_index,
    const vtss_appl_trap_receiver_conf_t   *const conf
)
{
    mesa_rc                 rc;
    vtss_trap_entry_t       snmp_conf;

    CHECK_TRAP_RECEIVER_PARAM(&conf_index, conf);

    strcpy(snmp_conf.trap_conf_name, conf_index.name);

    if ((rc = trap_mgmt_conf_get(&snmp_conf)) != VTSS_RC_OK) {
        return rc;
    }

    snmp_conf.trap_conf.enable = conf->enable;
    memcpy(&snmp_conf.trap_conf.dip, &conf->dest_addr, sizeof(vtss_inet_address_t));;
    snmp_conf.trap_conf.trap_port = conf->port;
    snmp_conf.trap_conf.trap_version = conf->version;
    strcpy(snmp_conf.trap_conf.trap_communitySecret, conf->community);
    (void)AUTH_secret_key_cryptography(TRUE, snmp_conf.trap_conf.trap_communitySecret, snmp_conf.trap_conf.trap_encryptedSecret);
    snmp_conf.trap_conf.trap_inform_mode = conf->notify_type;
    snmp_conf.trap_conf.trap_inform_timeout = conf->timeout;
    snmp_conf.trap_conf.trap_inform_retries = conf->retries;
    snmp_conf.trap_conf.trap_engineid_len = conf->engineid_len;
    memcpy(snmp_conf.trap_conf.trap_engineid, conf->engineid, snmp_conf.trap_conf.trap_engineid_len);
    strcpy(snmp_conf.trap_conf.trap_security_name, conf->user_name);
    return trap_mgmt_conf_set(&snmp_conf);
}


/**
 * \brief Delete the entry of SNMP TRAP receiver table
 *
 * To delete the specific entry in SNMP TRAP receiver table.
 *
 * \param conf_index      [IN]    (key) Name of the SNMP TRAP receiver entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_receiver_del(
    const vtss_appl_trap_receiver_index_t  conf_index
)
{
    vtss_trap_entry_t snmp_conf;

    CHECK_TRAP_RECEIVER_IDX(&conf_index);

    strcpy(snmp_conf.trap_conf_name, conf_index.name);
    snmp_conf.valid = FALSE;

    return trap_mgmt_conf_set(&snmp_conf);
}

/**
 * \brief Add new entry of SNMP TRAP receiver table
 *
 * To add new entry in SNMP TRAP receiver table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP TRAP receiver entry.
 * \param conf          [IN]    The new entry of SNMP TRAP receiver table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_receiver_add(
    const vtss_appl_trap_receiver_index_t  conf_index,
    const vtss_appl_trap_receiver_conf_t   *const conf
)
{
    vtss_trap_entry_t snmp_conf;

    CHECK_TRAP_RECEIVER_PARAM(&conf_index, conf);

    memset(&snmp_conf, 0, sizeof(snmp_conf)); // Clear to make sure we are at a know state.

    strcpy(snmp_conf.trap_conf_name, conf_index.name);

    if (trap_mgmt_conf_get(&snmp_conf) == VTSS_RC_OK) {
        return SNMP_ERROR_TRAP_RECV_ALREADY_EXIST;
    }

    snmp_conf.trap_conf.enable = conf->enable;
    memcpy(&snmp_conf.trap_conf.dip, &conf->dest_addr, sizeof(vtss_inet_address_t));;
    snmp_conf.trap_conf.trap_port = conf->port;
    snmp_conf.trap_conf.trap_version = conf->version;
    strcpy(snmp_conf.trap_conf.trap_communitySecret, conf->community);
    (void)AUTH_secret_key_cryptography(TRUE, snmp_conf.trap_conf.trap_communitySecret, snmp_conf.trap_conf.trap_encryptedSecret);
    snmp_conf.trap_conf.trap_inform_mode = conf->notify_type;
    snmp_conf.trap_conf.trap_inform_timeout = conf->timeout;
    snmp_conf.trap_conf.trap_inform_retries = conf->retries;
    snmp_conf.trap_conf.trap_engineid_len = conf->engineid_len;
    memcpy(snmp_conf.trap_conf.trap_engineid, conf->engineid, snmp_conf.trap_conf.trap_engineid_len);
    strcpy(snmp_conf.trap_conf.trap_security_name, conf->user_name);

    snmp_conf.valid         = TRUE;

    return trap_mgmt_conf_set(&snmp_conf);
}

/**
 * \brief Get default value of SNMP TRAP receiver table
 *
 * To get default values for the new entry in Trap receiver table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP TRAP receiver entry.
 * \param conf          [OUT]    The new entry of SNMP TRAP receiver table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */

mesa_rc
vtss_appl_trap_receiver_default(
    vtss_appl_trap_receiver_index_t  *const conf_index,
    vtss_appl_trap_receiver_conf_t   *const conf
)
{
    if (!conf_index || !conf) {
        T_D("conf_index or conf is NULL");
        return SNMP_ERROR_NULL;
    }
    memset(conf_index, 0, sizeof(*conf_index));
    memset(conf, 0, sizeof(*conf));

    conf->enable = TRAP_CONF_DEFAULT_ENABLE;
    conf->dest_addr.type = VTSS_INET_ADDRESS_TYPE_IPV4;
    conf->port = TRAP_CONF_DEFAULT_DPORT;
    conf->version = VTSS_APPL_SNMP_VERSION_2C;
    strcpy(conf->community, TRAP_CONF_DEFAULT_COMM);
    conf->notify_type = VTSS_APPL_TRAP_NOTIFY_TRAP;
    conf->timeout = TRAP_CONF_DEFAULT_INFORM_TIMEOUT;
    conf->retries = TRAP_CONF_DEFAULT_INFORM_RETRIES;
    conf->engineid_len = VTSS_APPL_SNMP_ENGINE_ID_MIN_LEN;
    strcpy(conf->user_name, TRAP_CONF_DEFAULT_SEC_NAME);
    return VTSS_RC_OK;
}

#define CHECK_TRAP_SOURCE_PARAM(CONF_IDX, CONF, is_add) \
    do {\
        if (!(CONF)) { \
            T_W("%s is NULL", #CONF); \
            return SNMP_ERROR_NULL; \
        }\
        if ((CONF)->index_filter_len > VTSS_APPL_SNMP_MAX_OID_LEN || \
            (CONF)->index_mask_len > VTSS_APPL_SNMP_MAX_SUBTREE_LEN) { \
            return SNMP_ERROR_PARM; \
        }\
        CHECK_TRAP_SOURCE_IDX(CONF_IDX, is_add); \
    } while(0)

#define CHECK_TRAP_SOURCE_IDX(CONF_IDX, is_add) \
    do {\
        if (strlen((CONF_IDX)->name) > VTSS_APPL_TRAP_TABLE_NAME_SIZE || \
            ((CONF_IDX)->index_filter_id >= VTSS_TRAP_FILTER_MAX && !is_add) || \
            ((CONF_IDX)->index_filter_id > VTSS_TRAP_FILTER_MAX && is_add)) { \
            return SNMP_ERROR_PARM; \
        }\
    } while(0)

/**
 * \brief Iterate function of SNMP TRAP source table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_trap_source_itr(
    const vtss_appl_trap_source_index_t    *const prev_index,
    vtss_appl_trap_source_index_t          *const next_index
)
{
    T_N("enter");
    mesa_rc                 rc = VTSS_RC_OK;
    vtss_trap_source_t      snmp_conf;
    int                     filter_id = 0;

    if (!next_index) {
        return SNMP_ERROR_NULL;
    }

    if (prev_index) {
        T_N("prev %s %d", prev_index->name, prev_index->index_filter_id);
        CHECK_TRAP_SOURCE_IDX(prev_index, FALSE);
        strcpy(snmp_conf.source_name, prev_index->name);
        filter_id = prev_index->index_filter_id;
    } else {
        memset(&snmp_conf, 0, sizeof(snmp_conf));
    }

    if ((rc = trap_mgmt_filter_get_next(&snmp_conf, &filter_id)) != VTSS_RC_OK) {
        T_N("exit");
        return rc;
    }

    strcpy(next_index->name, snmp_conf.source_name);
    next_index->index_filter_id = filter_id;
    T_N("exit next %s %d", next_index->name, next_index->index_filter_id);
    return rc;

}

/**
 * \brief Get the entry of SNMP TRAP source table
 *
 * To get the specific entry in SNMP TRAP source table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP TRAP source entry.
 * \param conf          [OUT]   The current entry of the SNMP TRAP source entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_source_get(
    const vtss_appl_trap_source_index_t  conf_index,
    vtss_appl_trap_source_conf_t         *const conf
)
{
    T_D("enter %s %d", conf_index.name, conf_index.index_filter_id);
    mesa_rc                 rc = VTSS_RC_OK;
    vtss_trap_source_t      snmp_conf;
    vtss_trap_filter_item_t snmp_filter;
    int                     filter_id;

    if (conf == NULL) {
        return SNMP_ERROR_NULL;
    }

    CHECK_TRAP_SOURCE_IDX(&conf_index, FALSE);

    strcpy(snmp_conf.source_name, conf_index.name);
    filter_id = conf_index.index_filter_id;

    if ((rc = trap_mgmt_filter_get(&snmp_conf, filter_id, &snmp_filter)) != VTSS_RC_OK) {
        T_N("exit");
        return rc;
    }

    conf->filter_type = (vtss_appl_snmp_view_type_t) snmp_filter.filter_type;
    conf->index_filter_len = snmp_filter.index_filter_len - 1;
    memcpy(conf->index_filter, snmp_filter.index_filter + 1, sizeof(oid) * snmp_filter.index_filter_len);
    conf->index_mask_len = snmp_filter.index_mask_len;
    memcpy(conf->index_mask, snmp_filter.index_mask, conf->index_mask_len);
    T_D("debug %s %d %d/%s", conf_index.name, conf_index.index_filter_id,
        conf->index_filter_len, misc_oid2txt((const ulong *)conf->index_filter, conf->index_filter_len));
    T_D("exit");
    return rc;
}

/**
 * \brief Set the entry of SNMP TRAP source table
 *
 * To modify the specific entry in SNMP TRAP source table.
 *
 * \param conf_index     [IN]    (key) Name of the SNMP TRAP source entry.
 * \param conf          [IN]    The revised the SNMP TRAP source entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_source_set(
    const vtss_appl_trap_source_index_t  conf_index,
    const vtss_appl_trap_source_conf_t   *const conf
)
{
    T_D("enter %s %d", conf_index.name, conf_index.index_filter_id);
    mesa_rc                 rc = VTSS_RC_OK;
    vtss_trap_source_t      snmp_conf;
    vtss_trap_filter_item_t snmp_filter;
    int                     filter_id;

    CHECK_TRAP_SOURCE_PARAM(&conf_index, conf, FALSE);

    strcpy(snmp_conf.source_name, conf_index.name);
    filter_id = conf_index.index_filter_id;

    if ((rc = trap_mgmt_filter_get(&snmp_conf, filter_id, &snmp_filter)) != VTSS_RC_OK) {
        T_N("exit");
        return rc;
    }

    snmp_filter.filter_type = conf->filter_type;
    snmp_filter.index_filter_len = conf->index_filter_len + 1;
    snmp_filter.index_filter[0] = conf->index_filter_len;
    memcpy(snmp_filter.index_filter + 1, conf->index_filter, sizeof(oid) * snmp_filter.index_filter_len);
    T_D("debug %s %d %d/%s", conf_index.name, conf_index.index_filter_id,
        conf->index_filter_len, misc_oid2txt((const ulong *)conf->index_filter, conf->index_filter_len));
    snmp_filter.index_mask_len = conf->index_mask_len;
    memcpy(snmp_filter.index_mask, conf->index_mask, conf->index_mask_len);
    T_D("call set");
    return trap_mgmt_source_set(&snmp_conf, &snmp_filter, filter_id);
}


/**
 * \brief Delete the entry of SNMP TRAP source table
 *
 * To delete the specific entry in SNMP TRAP source table.
 *
 * \param conf_index      [IN]    (key) Name of the SNMP TRAP source entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_source_del(
    const vtss_appl_trap_source_index_t  conf_index
)
{
    T_D("enter %s %d", conf_index.name, conf_index.index_filter_id);
    vtss_trap_source_t      snmp_conf;
    int                     filter_id;

    CHECK_TRAP_SOURCE_IDX(&conf_index, FALSE);

    strcpy(snmp_conf.source_name, conf_index.name);
    filter_id = conf_index.index_filter_id;
    snmp_conf.valid = FALSE;

    T_D("call set");
    return trap_mgmt_source_set(&snmp_conf, NULL, filter_id);
}

/**
 * \brief Add new entry of SNMP TRAP source table
 *
 * To add new entry in SNMP TRAP source table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP TRAP source entry.
 * \param conf          [IN]    The new entry of SNMP TRAP source table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_source_add(
    const vtss_appl_trap_source_index_t  conf_index,
    const vtss_appl_trap_source_conf_t   *const conf
)
{
    T_D("enter %s %d", conf_index.name, conf_index.index_filter_id);
    vtss_trap_source_t      snmp_conf;
    vtss_trap_filter_item_t snmp_filter;
    int                     filter_id;

    CHECK_TRAP_SOURCE_PARAM(&conf_index, conf, TRUE);

    memset(&snmp_conf, 0, sizeof(snmp_conf)); // Clear to make sure we are at a know state.

    strcpy(snmp_conf.source_name, conf_index.name);

    if (conf_index.index_filter_id == VTSS_TRAP_FILTER_MAX) {
        filter_id = -1;
    } else {
        filter_id = conf_index.index_filter_id;
        if (trap_mgmt_filter_get(&snmp_conf, filter_id, &snmp_filter) == VTSS_RC_OK) {
            T_N("exit");
            return SNMP_ERROR_TRAP_SOURCE_ALREADY_EXIST;
        }
    }

    snmp_filter.filter_type = conf->filter_type;
    snmp_filter.index_filter_len = conf->index_filter_len + 1;
    snmp_filter.index_filter[0] = conf->index_filter_len;
    memcpy(snmp_filter.index_filter + 1, conf->index_filter, sizeof(oid) * conf->index_filter_len);
    T_D("debug %s %d len=%d", conf_index.name, conf_index.index_filter_id, conf->index_filter_len);
    snmp_filter.index_mask_len = conf->index_mask_len;
    memcpy(snmp_filter.index_mask, conf->index_mask, conf->index_mask_len);

    snmp_conf.valid         = TRUE;
    T_D("call set");
    return trap_mgmt_source_set(&snmp_conf, &snmp_filter, filter_id);
}

/**
 * \brief Get default value of SNMP TRAP source table
 *
 * To get default values for the new entry in Trap receiver table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP TRAP source entry.
 * \param conf          [OUT]    The new entry of SNMP TRAP source table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */

mesa_rc
vtss_appl_trap_source_default(
    vtss_appl_trap_source_index_t  *const conf_index,
    vtss_appl_trap_source_conf_t   *const conf
)
{
    if (!conf_index || !conf) {
        T_D("conf_index or conf is NULL");
        return SNMP_ERROR_NULL;
    }
    memset(conf_index, 0, sizeof(*conf_index));
    memset(conf, 0, sizeof(*conf));

    conf->filter_type = VTSS_APPL_SNMP_VIEW_TYPE_INCLUDED;
    return VTSS_RC_OK;
}


void vtss_debug_send_trap()
{
    snmp_trap_if_info_t   trap_if_info;
    memset(&trap_if_info, 0x0, sizeof(trap_if_info));
    snmp_mgmt_send_trap(SNMP_TRAP_TYPE_WARMSTART, trap_if_info);
}

void vtss_snmp_u64_to_counter64(struct counter64 *c64, uint64_t uint64)
{
    if (!c64) {
        return;
    }

    c64->high = (uint64 >> 32) & 0xFFFFFFFF;
    c64->low  = (uint64 >>  0) & 0xFFFFFFFF;
}

