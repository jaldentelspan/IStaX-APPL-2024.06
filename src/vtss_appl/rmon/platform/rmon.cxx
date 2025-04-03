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

/**************************************************************
 * Copyright (C) 2001 Alex Rozin, Optical Access
 *
 *                     All Rights Reserved
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * ALEX ROZIN DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * ALEX ROZIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 ******************************************************************/

#include <main.h>
#include "conf_api.h"
#include "rmon_api.h"
#include "misc_api.h"
#include "msg_api.h"  // msg_switch_exists(), msg_switch_configurable()
#include "port_api.h" // port_count_max()
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#include "rmon_trace.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "rmon_icfg.h"
#endif

#ifdef VTSS_SW_OPTION_SNMP
#include "vtss_snmp_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_RMON

#ifdef EXTEND_RMON_TO_WEB_CLI
static rmon_global_t rmon_global;
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "RMON", "RMON"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define RMON_CRIT_ENTER(table_ptr) critd_enter(&((table_ptr)->crit), __FILE__, __LINE__)
#define RMON_CRIT_EXIT(table_ptr)  critd_exit( &((table_ptr)->crit), __FILE__, __LINE__)
#define RMON_GLOBAL_CRIT_ENTER()   critd_enter(&rmon_global.crit,    __FILE__, __LINE__)
#define RMON_GLOBAL_CRIT_EXIT()    critd_exit( &rmon_global.crit,    __FILE__, __LINE__)

#define MIB_DESCR   "Startup Mgmt"
typedef vtss_stat_ctrl_entry_t STAT_CRTL_ENTRY_T;
typedef struct {
    RMON_TABLE_INDEX_T tbl_index;
    TABLE_DEFINTION_T *tbl;
} rmonTable_t;

static TABLE_DEFINTION_T StatCtrlTable;
static TABLE_DEFINTION_T *stat_table_ptr = NULL;
static TABLE_DEFINTION_T HistoryCtrlTable;
static TABLE_DEFINTION_T *history_table_ptr = NULL;
static TABLE_DEFINTION_T AlarmCtrlTable;
static TABLE_DEFINTION_T *alarm_table_ptr = NULL;
static TABLE_DEFINTION_T EventCtrlTable;
static TABLE_DEFINTION_T *event_table_ptr = NULL;

/*lint -esym(459,etherStatusIndex)*/
/* This table is read only. */
static VAR_OID_T etherStatusIndex = { 11, {1, 3, 6, 1, 2, 1, 2, 2, 1, 1, 1} };
static BOOL stat_delete_all_flag = FALSE;

/*lint -esym(459,rmonTable)*/
/* This table is read only. */
static rmonTable_t rmonTable[] = {
    {RMON_STATS_TABLE_INDEX, &StatCtrlTable},
    {RMON_HISTORY_TABLE_INDEX, &HistoryCtrlTable},
    {RMON_ALARM_TABLE_INDEX, &AlarmCtrlTable},
    {RMON_EVENT_TABLE_INDEX, &EventCtrlTable}
};

static TABLE_DEFINTION_T *rmonTable_find(RMON_TABLE_INDEX_T idx)
{
    int i = 0;
    for (i = 0; i < ( int )(sizeof(rmonTable) / sizeof(rmonTable_t)); i++) {
        if (idx == rmonTable[i].tbl_index) {
            return rmonTable[i].tbl;
        }
    }
    return NULL;
}

/*
 * Control Table RowApi Callbacks
 */
static int
stat_Create(RMON_ENTRY_T *eptr)
{
    /* create the body: alloc it and set defaults */
    STAT_CRTL_ENTRY_T   *body;

    eptr->body = AGMALLOC(sizeof(STAT_CRTL_ENTRY_T));
    if (!eptr->body) {
        return -3;
    }
    body = (STAT_CRTL_ENTRY_T *) eptr->body;

    /*
     * set defaults
     */
    /* default data source
    memcpy(&body->data_source, &etherStatusIndex, sizeof(VAR_OID_T));
    body->data_source.objid[body->data_source.length - 1] = eptr->ctrl_index; */
    memset(body->data_source.objid, 0x0, sizeof(body->data_source.objid));
    body->data_source.length = 2;
    eptr->owner = AGSTRDUP(MIB_DESCR);
    memset(&body->eth, 0, sizeof(ETH_STATS_T));

    return 0;
}

BOOL get_datasource_info(int if_index, datasourceTable_info_t *table_info_p)
{
    table_info_p->ifIndex = if_index;

    return ifIndex_get_valid(table_info_p);
}





static int
stat_Validate(RMON_ENTRY_T *eptr)
{
    /*
     * T.B.D. (system dependent) check valid inteface in body->data_source;
     */
    STAT_CRTL_ENTRY_T   *body;
    datasourceTable_info_t      table_info;
    int                 if_index;

    body = (STAT_CRTL_ENTRY_T *) eptr->body;

    /* check etherStatsIndex length */
    if (body->data_source.length != etherStatusIndex.length) {
        return -1;
    }

    /* check etherStatsIndex OID */
    if (snmp_oid_compare
        (body->data_source.objid, etherStatusIndex.length - 1,
         etherStatusIndex.objid, etherStatusIndex.length - 1)) {
        return -1;
    }

    /* check entry is valid */
    if_index = body->data_source.objid[etherStatusIndex.length - 1];

    if (get_datasource_info(if_index, &table_info)) {
        if (table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_PORT ||
            table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_LLAG ||
            table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_GLAG) {
            /* save SNMP RMON statistics valid entry */
            rmon_stat_entry_t entry;

            entry.valid = 1;
            entry.ctrl_index = eptr->ctrl_index;
            entry.if_index = body->data_source.objid[body->data_source.length - 1];
            if (snmp_mgmt_rmon_stat_entry_set(&entry) != VTSS_RC_OK) {
                T_W("snmp_mgmt_rmon_stat_entry_set fail");
                return -1;
            }

            return 0;
        }
    }

    return -1;
}

static int
stat_Activate(RMON_ENTRY_T *eptr)
{
    STAT_CRTL_ENTRY_T   *body = (STAT_CRTL_ENTRY_T *) eptr->body;

    body->etherStatsCreateTime = AGUTIL_sys_up_time();

    return 0;
}

static int
stat_Copy(RMON_ENTRY_T *eptr)
{
    STAT_CRTL_ENTRY_T   *body = (STAT_CRTL_ENTRY_T *) eptr->body;
    STAT_CRTL_ENTRY_T   *clone = (STAT_CRTL_ENTRY_T *) eptr->tmp;

    if (snmp_oid_compare
        (clone->data_source.objid, clone->data_source.length,
         body->data_source.objid, body->data_source.length)) {
        memcpy(&body->data_source, &clone->data_source, sizeof(VAR_OID_T));
    }

#ifdef EXTEND_RMON_TO_WEB_CLI
    body->id = clone->id;
#endif
    return 0;
}

static int
stat_Deactivate(RMON_ENTRY_T *eptr)
{
    STAT_CRTL_ENTRY_T      *body = (STAT_CRTL_ENTRY_T *) eptr->body;
    rmon_stat_entry_t entry;

    memset(&body->eth, 0, sizeof(ETH_STATS_T));

    /* delete SNMP RMON statistics entry */
    if (stat_delete_all_flag) {
        return 0;
    }

    entry.ctrl_index = eptr->ctrl_index;
    if (snmp_mgmt_rmon_stat_entry_get(&entry, FALSE) != VTSS_RC_OK) {
        return -1;
    }
    entry.valid = 0;
    if (snmp_mgmt_rmon_stat_entry_set(&entry) != VTSS_RC_OK) {
        return -1;
    }

    return 0;

}

int
add_statistics_entry(int ctrl_index, int ifIndex)
{
    int             ierr;

    ierr = ROWAPI_new(stat_table_ptr, ctrl_index);
    switch (ierr) {
    case -1:
        ag_trace("max. number exedes\n");
        break;
    case -2:
        ag_trace("VTSS_MALLOC() failed");
        break;
    case -3:
        ag_trace("ClbkCreate failed");
        break;
    case 0:
        break;
    default:
        ag_trace("Unknown code %d", ierr);
        break;
    }

    if (!ierr) {
        RMON_ENTRY_T *eptr = ROWAPI_find(stat_table_ptr, ctrl_index);
        if (!eptr) {
            ag_trace("cannot find statistics entry %ld", ctrl_index);
            ierr = -4;
        } else {
            STAT_CRTL_ENTRY_T *body = (STAT_CRTL_ENTRY_T *) eptr->body;

            memcpy(&body->data_source, &etherStatusIndex, sizeof(VAR_OID_T));
            body->data_source.objid[body->data_source.length - 1] = ifIndex;
#ifdef EXTEND_RMON_TO_WEB_CLI
            body->id = ctrl_index;
#endif

            eptr->new_status = RMON1_ENTRY_VALID;
            ierr = ROWAPI_commit(stat_table_ptr, ctrl_index);
            if (ierr) {
                ag_trace("ROWAPI_commit returned %d", ierr);
            }
        }
    }

    return ierr;
}

void rmon_stat_DeleteAllRow(void)
{
    RMON_ENTRY_T *eptr;
    u_long   idx = 0, entry_num;
    u_long   *buffer;

    if (!stat_table_ptr) {
        return;
    }
    entry_num = stat_table_ptr->current_number_of_entries;
    if (!entry_num) {
        return;
    }
    if (!(VTSS_MALLOC_CAST(buffer, entry_num * sizeof(u_long)))) {
        return;
    }

    for (eptr = stat_table_ptr->first; eptr; eptr = eptr->next) {
        buffer[idx++] = eptr->ctrl_index;
        if (idx > stat_table_ptr->current_number_of_entries) {
            VTSS_FREE(buffer);
            return;
        }
    }

    stat_delete_all_flag = TRUE;
    for (idx = 0; idx < entry_num; idx++) {
        eptr = ROWAPI_find(stat_table_ptr, buffer[idx]);
        if (eptr) {
            ROWAPI_delete_clone(stat_table_ptr, buffer[idx]);
            rowapi_delete(eptr);
        }
    }
    stat_delete_all_flag = FALSE;

    VTSS_FREE(buffer);
}

void rmon_create_stat_default_entry(void)
{
    rmon_stat_entry_t entry;

    if (stat_table_ptr) {
        rmon_stat_DeleteAllRow();
    } else {
        return;
    }

    /* Create default RMON statistics row entries form SNMP manager module */
    entry.ctrl_index = 0;
    while (snmp_mgmt_rmon_stat_entry_get(&entry, TRUE) == VTSS_RC_OK) {
        (void)add_statistics_entry(entry.ctrl_index, entry.if_index);
    }
}

static BOOL update_etherStatsTable_entry(vtss_isid_t isid, mesa_port_no_t port_idx, ETH_STATS_T *table_entry_p)
{
    mesa_port_counters_t counters;

    if (!msg_switch_configurable(isid)) {
        /* if the switch is not configurable, the counters always equal 0 */
        memset(table_entry_p, 0, sizeof(*table_entry_p));
        return FALSE;
    }

    vtss_ifindex_t ifindex;
    if (vtss_ifindex_from_port(isid, port_idx, &ifindex) != VTSS_RC_OK) {
        return FALSE;
    };

    if (!msg_switch_exists(isid) || vtss_appl_port_statistics_get(ifindex, &counters) != VTSS_RC_OK) {
        return FALSE;
    }

    table_entry_p->drop_events    += counters.rmon.rx_etherStatsDropEvents           + counters.rmon.tx_etherStatsDropEvents;
    table_entry_p->octets         += counters.rmon.rx_etherStatsOctets               + counters.rmon.tx_etherStatsOctets;
    table_entry_p->packets        += counters.rmon.rx_etherStatsPkts                 + counters.rmon.tx_etherStatsPkts;
    table_entry_p->bcast_pkts     += counters.rmon.rx_etherStatsBroadcastPkts        + counters.rmon.tx_etherStatsBroadcastPkts;
    table_entry_p->mcast_pkts     += counters.rmon.rx_etherStatsMulticastPkts        + counters.rmon.tx_etherStatsMulticastPkts;
    table_entry_p->crc_align      += counters.rmon.rx_etherStatsCRCAlignErrors;
    table_entry_p->undersize      += counters.rmon.rx_etherStatsUndersizePkts;
    table_entry_p->oversize       += counters.rmon.rx_etherStatsOversizePkts;
    table_entry_p->fragments      += counters.rmon.rx_etherStatsFragments;
    table_entry_p->jabbers        += counters.rmon.rx_etherStatsJabbers;
    table_entry_p->collisions     +=                                                   counters.rmon.tx_etherStatsCollisions;
    table_entry_p->pkts_64        += counters.rmon.rx_etherStatsPkts64Octets         + counters.rmon.tx_etherStatsPkts64Octets;
    table_entry_p->pkts_65_127    += counters.rmon.rx_etherStatsPkts65to127Octets    + counters.rmon.tx_etherStatsPkts65to127Octets;
    table_entry_p->pkts_128_255   += counters.rmon.rx_etherStatsPkts128to255Octets   + counters.rmon.tx_etherStatsPkts128to255Octets;
    table_entry_p->pkts_256_511   += counters.rmon.rx_etherStatsPkts256to511Octets   + counters.rmon.tx_etherStatsPkts256to511Octets;
    table_entry_p->pkts_512_1023  += counters.rmon.rx_etherStatsPkts512to1023Octets  + counters.rmon.tx_etherStatsPkts512to1023Octets;
    table_entry_p->pkts_1024_1518 += counters.rmon.rx_etherStatsPkts1024to1518Octets + counters.rmon.tx_etherStatsPkts1024to1518Octets;

    return TRUE;
}

BOOL get_etherStatsTable_entry(VAR_OID_T *data_source, ETH_STATS_T *table_entry_p)
{
#ifdef VTSS_SW_OPTION_AGGR
    vtss_isid_t              isid;
    mesa_port_no_t           port_idx;
    aggr_mgmt_group_member_t aggr_members;
#endif /* VTSS_SW_OPTION_AGGR */
    datasourceTable_info_t   table_info;
    u32                      port_count;

    memset(table_entry_p, 0x0, sizeof(ETH_STATS_T));

    table_entry_p->ifIndex = vtss_ifindex_cast_from_u32_0(data_source->objid[data_source->length - 1]);
    if (get_datasource_info(vtss_ifindex_cast_to_u32(table_entry_p->ifIndex), &table_info) == FALSE) {
        return FALSE;
    }

    port_count = port_count_max();

    switch (table_info.type) {
    case IFTABLE_IFINDEX_TYPE_PORT:
        if (update_etherStatsTable_entry(table_info.isid, table_info.if_id, table_entry_p) == FALSE) {
            return FALSE;
        }
        break;
#ifdef VTSS_SW_OPTION_AGGR
    case IFTABLE_IFINDEX_TYPE_LLAG:
        if ((aggr_mgmt_port_members_get(table_info.isid, table_info.if_id, &aggr_members, FALSE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
            && (aggr_mgmt_lacp_members_get(table_info.isid, table_info.if_id, &aggr_members, FALSE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
           ) {
            return FALSE;
        }

        for (port_idx = VTSS_PORT_NO_START ; port_idx < port_count; port_idx++) {
            if (!aggr_members.entry.member[port_idx]) {
                continue;
            }
            if (update_etherStatsTable_entry(table_info.isid, port_idx, table_entry_p) == FALSE) {
                return FALSE;
            }
        }
        break;
    case IFTABLE_IFINDEX_TYPE_GLAG:
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!msg_switch_exists(isid)) {
                continue;
            }

            if (aggr_mgmt_port_members_get(isid, table_info.if_id, &aggr_members, FALSE) != VTSS_RC_OK) {
                return FALSE;
            }
            for (port_idx = VTSS_PORT_NO_START ; port_idx < port_count; port_idx++) {
                if (!aggr_members.entry.member[port_idx]) {
                    continue;
                }
                if (update_etherStatsTable_entry(isid, port_idx, table_entry_p) == FALSE) {
                    return FALSE;
                }
            }
        }
        break;
#endif /* VTSS_SW_OPTION_AGGR */
    case IFTABLE_IFINDEX_TYPE_VLAN:
        break;
    default:
        return FALSE;
    }

    return TRUE;
}


#endif /* RFC2819_SUPPORTED_STATISTICS */

/* history ----------------------------------------------------------*/


/*
 * File scope definitions section
 */

/*
 * defaults & limitations
 */

#define HIST_DEF_INTERVAL 1800


#ifndef EXTEND_RMON_TO_WEB_CLI
typedef struct history_data_struct_t {
    struct data_struct_t *next;
    u_long          data_index;
    u_long          start_interval;
    u_long          utilization;
    ETH_STATS_T     EthData;
} HISTORY_DATA_ENTRY_T;

typedef struct {
    u_long               interval;
    u_long               timer_id;
    VAR_OID_T            data_source;
    u_long               coeff;
    HISTORY_DATA_ENTRY_T previous_bucket;
    SCROLLER_T           scrlr;
} HISTORY_CRTL_ENTRY_T;
#else
typedef vtss_history_ctrl_entry_t HISTORY_CRTL_ENTRY_T;
typedef vtss_history_data_entry_t HISTORY_DATA_ENTRY_T;
#endif

/*lint -esym(459,historyControlIndex)*/
/* This table is read only. */
static VAR_OID_T historyControlIndex = { 11, {1, 3, 6, 1, 2, 1, 2, 2, 1, 1, 1} };
static BOOL history_delete_all_flag = FALSE;

/*
 * history row management control callbacks
 */

static void
compute_delta(ETH_STATS_T *delta,
              ETH_STATS_T *newval, ETH_STATS_T *prevval)
{
#define CNT_DIF(X) delta->X = (newval->X >= prevval->X) ? (newval->X - prevval->X) : newval->X

    CNT_DIF(octets);
    CNT_DIF(packets);
    CNT_DIF(bcast_pkts);
    CNT_DIF(mcast_pkts);
    CNT_DIF(crc_align);
    CNT_DIF(undersize);
    CNT_DIF(oversize);
    CNT_DIF(fragments);
    CNT_DIF(jabbers);
    CNT_DIF(collisions);
}

static void
history_get_backet(unsigned int clientreg, void *clientarg)
{
    RMON_ENTRY_T         *hdr_ptr;
    HISTORY_CRTL_ENTRY_T *body;
    HISTORY_DATA_ENTRY_T *bptr;
    ETH_STATS_T          newSample;

    /*
     * ag_trace ("history_get_backet: timer_id=%d", (int) clientreg);
     */
    hdr_ptr = (RMON_ENTRY_T *) clientarg;
    if (!hdr_ptr) {
        ag_trace
        ("Err: history_get_backet: hdr_ptr=NULL ? (Inserted in shock)");
        return;
    }

    body = (HISTORY_CRTL_ENTRY_T *) hdr_ptr->body;
    if (!body) {
        ag_trace
        ("Err: history_get_backet: body=NULL ? (Inserted in shock)");
        return;
    }

    if (RMON1_ENTRY_VALID != hdr_ptr->status) {
        ag_trace("Err: history_get_backet when entry %d is not valid ?!!",
                 (int) hdr_ptr->ctrl_index);
        /*
         * snmp_alarm_print_list ();
         */
        ROWAPI_alarm_unregister(body->timer_id);
        ag_trace("Err: unregistered %ld", (long) body->timer_id);
        return;
    }

    /* SYSTEM_get_eth_statistics(&body->data_source, &newSample); */
    if (!get_etherStatsTable_entry(&body->data_source, &newSample)) {
        return;
    }

    bptr = (HISTORY_DATA_ENTRY_T *)ROWDATAAPI_locate_new_data(&body->scrlr);
    if (!bptr) {
        ag_trace
        ("Err: history_get_backet for %d: empty bucket's list !\n",
         (int) hdr_ptr->ctrl_index);
        return;
    }

    bptr->data_index = ROWDATAAPI_get_total_number(&body->scrlr);
#if 0
#ifdef EXTEND_RMON_TO_WEB_CLI
    bptr->ctrl_index = body->id;
#endif
#endif

    bptr->start_interval = body->previous_bucket.start_interval;

    compute_delta(&bptr->EthData, &newSample,
                  &body->previous_bucket.EthData);

    bptr->utilization =
        bptr->EthData.octets * 8 + bptr->EthData.packets * (96 + 64);
    bptr->utilization /= body->coeff;

    /*
     * update previous_bucket
     */
    body->previous_bucket.start_interval = AGUTIL_sys_up_time();
    memcpy(&body->previous_bucket.EthData, &newSample,
           sizeof(ETH_STATS_T));
}

/*
 * Control Table RowApi Callbacks
 */

int
history_Create(RMON_ENTRY_T *eptr)
{
    /* create the body: alloc it and set defaults */
    HISTORY_CRTL_ENTRY_T   *body;

    eptr->body = AGMALLOC(sizeof(HISTORY_CRTL_ENTRY_T));
    if (!eptr->body) {
        return -3;
    }
    body = (HISTORY_CRTL_ENTRY_T *) eptr->body;

    /*
     * set defaults
     */
    body->interval = HIST_DEF_INTERVAL;
    body->timer_id = 0;
    /* default data source
    memcpy(&body->data_source, &historyControlIndex, sizeof(VAR_OID_T));
    body->data_source.objid[body->data_source.length - 1] = eptr->ctrl_index; */
    memset(body->data_source.objid, 0x0, sizeof(body->data_source.objid));
    body->data_source.length = 2;
    (void)ROWDATAAPI_init(&body->scrlr, RMON_BUCKET_CNT_DEF,
                          RMON_BUCKET_CNT_MAX, sizeof(HISTORY_DATA_ENTRY_T), NULL);

    return 0;
}

int
history_Validate(RMON_ENTRY_T *eptr)
{
    /*
     * T.B.D. (system dependent) check valid inteface in body->data_source;
     */
    HISTORY_CRTL_ENTRY_T *body;
    datasourceTable_info_t       table_info;
    int                  if_index;

    body = (HISTORY_CRTL_ENTRY_T *) eptr->body;

    /* check historyControlIndex length */
    if (body->data_source.length != historyControlIndex.length) {
        return -1;
    }

    /* check historyControlIndex OID */
    if (snmp_oid_compare
        (body->data_source.objid, historyControlIndex.length - 1,
         historyControlIndex.objid, historyControlIndex.length - 1)) {
        return -1;
    }

    /* check entry is valid */
    if_index = body->data_source.objid[historyControlIndex.length - 1];

    if (get_datasource_info(if_index, &table_info)) {
        if (table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_PORT ||
            table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_LLAG ||
            table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_GLAG) {
            /* save SNMP RMON history valid entry */
            rmon_history_entry_t entry;

            entry.valid = 1;
            entry.ctrl_index = eptr->ctrl_index;
            entry.if_index = body->data_source.objid[body->data_source.length - 1];
            entry.interval = body->interval;
            entry.requested = body->scrlr.data_requested;
            if (snmp_mgmt_rmon_history_entry_set(&entry) != VTSS_RC_OK) {
                return -1;
            }

            return 0;
        }
    }

    return -1;
}

int
history_Activate(RMON_ENTRY_T *eptr)
{
    HISTORY_CRTL_ENTRY_T   *body = (HISTORY_CRTL_ENTRY_T *) eptr->body;

    body->coeff = 100000L * (long) body->interval;

    ROWDATAAPI_set_size(&body->scrlr,
                        body->scrlr.data_requested,
                        (u_char)(RMON1_ENTRY_VALID == eptr->status) );

    /* SYSTEM_get_eth_statistics(&body->data_source,
                              &body->previous_bucket.EthData); */
    (void)get_etherStatsTable_entry(&body->data_source,
                                    &body->previous_bucket.EthData);
    body->previous_bucket.start_interval = AGUTIL_sys_up_time();

    body->scrlr.current_data_ptr = body->scrlr.first_data_ptr;
    /*
     * ag_trace ("Dbg:   registered in history_Activate");
     */
    body->timer_id = ROWAPI_alarm_register(body->interval, RMON_TIMER_REPEAT,
                                           history_get_backet, eptr);
    return 0;
}

int
history_Deactivate(RMON_ENTRY_T *eptr)
{
    HISTORY_CRTL_ENTRY_T      *body = (HISTORY_CRTL_ENTRY_T *) eptr->body;
    rmon_history_entry_t entry;

    if (body->timer_id != 0) {
        ROWAPI_alarm_unregister(body->timer_id);
    }
    /*
     * ag_trace ("Dbg: unregistered in history_Deactivate timer_id=%d",
     * (int) body->timer_id);
     */

    /*
     * free data list
     */
    ROWDATAAPI_descructor(&body->scrlr);

    /* delete SNMP RMON history entry */
    if (history_delete_all_flag) {
        return 0;
    }
    entry.ctrl_index = eptr->ctrl_index;
    if (snmp_mgmt_rmon_history_entry_get(&entry, FALSE) != VTSS_RC_OK) {
        return -1;
    }
    entry.valid = 0;
    if (snmp_mgmt_rmon_history_entry_set(&entry) != VTSS_RC_OK) {
        return -1;
    }

    return 0;
}

int
history_Copy(RMON_ENTRY_T *eptr)
{
    HISTORY_CRTL_ENTRY_T   *body = (HISTORY_CRTL_ENTRY_T *) eptr->body;
    HISTORY_CRTL_ENTRY_T   *clone = (HISTORY_CRTL_ENTRY_T *) eptr->tmp;

    if (body->scrlr.data_requested != clone->scrlr.data_requested) {
        ROWDATAAPI_set_size(&body->scrlr, clone->scrlr.data_requested,
                            (u_char)(RMON1_ENTRY_VALID == eptr->status) );
    }

    if (body->interval != clone->interval) {
        if (RMON1_ENTRY_VALID == eptr->status) {
            if (body->timer_id != 0) {
                ROWAPI_alarm_unregister(body->timer_id);
            }
            body->timer_id =
                ROWAPI_alarm_register(clone->interval, RMON_TIMER_REPEAT,
                                      history_get_backet, eptr);
        }

        body->interval = clone->interval;
    }

#ifdef EXTEND_RMON_TO_WEB_CLI
    body->id = clone->id;
#endif

    if (snmp_oid_compare
        (clone->data_source.objid, clone->data_source.length,
         body->data_source.objid, body->data_source.length)) {
        memcpy(&body->data_source, &clone->data_source, sizeof(VAR_OID_T));
    }

    return 0;
}

static SCROLLER_T *
history_extract_scroller(void *v_body)
{
    HISTORY_CRTL_ENTRY_T   *body = (HISTORY_CRTL_ENTRY_T *) v_body;
    return &body->scrlr;
}

int
add_history_entry(int ctrl_index, int ifIndex,
                  u_long interval, u_long requested)
{
    int             ierr;

    ierr = ROWAPI_new(history_table_ptr, ctrl_index);
    switch (ierr) {
    case -1:
        ag_trace("max. number exedes\n");
        break;
    case -2:
        ag_trace("VTSS_MALLOC() failed");
        break;
    case -3:
        ag_trace("ClbkCreate failed");
        break;
    case 0:
        break;
    default:
        ag_trace("Unknown code %d", ierr);
        break;
    }

    if (!ierr) {
        RMON_ENTRY_T *eptr = ROWAPI_find(history_table_ptr, ctrl_index);
        if (!eptr) {
            ag_trace("cannot find history entry %ld", ctrl_index);
            ierr = -4;
        } else {
            HISTORY_CRTL_ENTRY_T *body = (HISTORY_CRTL_ENTRY_T *) eptr->body;

            memcpy(&body->data_source, &etherStatusIndex, sizeof(VAR_OID_T));
            body->data_source.objid[body->data_source.length - 1] = ifIndex;
            body->interval = interval;
            body->scrlr.data_requested = requested;
#ifdef EXTEND_RMON_TO_WEB_CLI
            body->id = ctrl_index;
#endif

            eptr->new_status = RMON1_ENTRY_VALID;
            ierr = ROWAPI_commit(history_table_ptr, ctrl_index);
            if (ierr) {
                ag_trace("ROWAPI_commit returned %d", ierr);
            }
        }
    }

    return ierr;
}

void rmon_history_DeleteAllRow(void)
{
    RMON_ENTRY_T *eptr;
    u_long   idx = 0, entry_num;
    u_long   *buffer;

    if (!history_table_ptr) {
        return;
    }
    entry_num = history_table_ptr->current_number_of_entries;
    if (!entry_num) {
        return;
    }
    if (!(VTSS_MALLOC_CAST(buffer, entry_num * sizeof(u_long)))) {
        return;
    }

    for (eptr = history_table_ptr->first; eptr; eptr = eptr->next) {
        buffer[idx++] = eptr->ctrl_index;
        if (idx > history_table_ptr->current_number_of_entries) {
            VTSS_FREE(buffer);
            return;
        }
    }

    history_delete_all_flag = TRUE;
    for (idx = 0; idx < entry_num; idx++) {
        eptr = ROWAPI_find(history_table_ptr, buffer[idx]);
        if (eptr) {
            ROWAPI_delete_clone(history_table_ptr, buffer[idx]);
            rowapi_delete(eptr);
        }
    }
    history_delete_all_flag = FALSE;

    VTSS_FREE(buffer);
}

void rmon_create_history_default_entry(void)
{
    rmon_history_entry_t entry;

    if (history_table_ptr) {
        rmon_history_DeleteAllRow();
    } else {
        return;
    }

    /* Create default RMON history row entries form SNMP manager module */
    entry.ctrl_index = 0;
    while (snmp_mgmt_rmon_history_entry_get(&entry, TRUE) == VTSS_RC_OK) {
        (void)add_history_entry(entry.ctrl_index, entry.if_index, entry.interval, entry.requested);
    }
}


/* alram ----------------------------------------------------------*/



#ifndef EXTEND_RMON_TO_WEB_CLI
typedef enum {
    VTSS_APPL_RMON_SAMPLE_TYPE_ABSOLUTE =
        1,
    VTSS_APPL_RMON_SAMPLE_TYPE_DELTA,
} ALARM_SAMPLE_TYPE_T;

typedef enum {
    VTSS_APPL_RMON_ALARM_NOTHING =
        0,
    VTSS_APPL_RMON_ALARM_RISING,
    VTSS_APPL_RMON_ALARM_FALLING,
    VTSS_APPL_RMON_ALARM_BOTH
} ALARM_TYPE_T;

typedef struct {
    u_long
    interval;
    u_long
    timer_id;
    VAR_OID_T
    var_name;
    ALARM_SAMPLE_TYPE_T
    sample_type;
    ALARM_TYPE_T
    startup_type;      /* RISING | FALLING | BOTH */

    long
    rising_threshold;
    long
    falling_threshold;
    u_long
    rising_event_index;
    u_long
    falling_event_index;

    u_long
    last_abs_value;
    long
    value;
    ALARM_TYPE_T
    prev_alarm;        /* NOTHING | RISING | FALLING */
} ALARM_CRTL_ENTRY_T;

#else
typedef vtss_alarm_sample_type_t ALARM_SAMPLE_TYPE_T;
typedef vtss_alarm_type_t ALARM_TYPE_T;
typedef vtss_alarm_ctrl_entry_t ALARM_CRTL_ENTRY_T;
#endif


/*
 * Main section
 */

//static VAR_OID_T alarmIndex = { 12, {1, 3, 6, 1, 2, 1, 16, 1, 1, 1, 5, 1} };
static BOOL alarm_delete_all_flag = FALSE;

#if 0                           /* KUKU */
static u_long
kuku_sum =
    0,
    kuku_cnt =
        0;
#endif

/*
 * find & enjoy it in event.c
 */
extern int
event_api_send_alarm(u_char is_rising,
                     u_long alarm_index,
                     u_long event_index,
                     oid *alarmed_var,
                     size_t alarmed_var_length,
                     u_long sample_type,
                     u_long value,
                     u_long the_threshold, char *alarm_descr);

static int
fetch_var_val(oid *name, size_t namelen, u_long *new_value)
{
    netsnmp_subtree *tree_ptr;
    size_t          var_len;
    WriteMethod    *write_method;
    struct variable called_var;
    struct variable *s_var_ptr = NULL;
    u_char *access;


    tree_ptr = netsnmp_subtree_find(name, namelen, NULL, "");
    if (!tree_ptr) {
        ag_trace("tree_ptr is NULL");
        return ROW_ERR_NOSUCHNAME;
    }


    memcpy(called_var.name, tree_ptr->name_a,
           tree_ptr->namelen * sizeof(oid));

    if (tree_ptr->reginfo &&
        tree_ptr->reginfo->handler &&
        tree_ptr->reginfo->handler->next &&
        tree_ptr->reginfo->handler->next->myvoid) {
        s_var_ptr = (struct variable *)tree_ptr->reginfo->handler->next->myvoid;
    }

    if (s_var_ptr) {
        if (s_var_ptr->namelen) {
            called_var.namelen =
                tree_ptr->namelen;
            called_var.type = s_var_ptr->type;
            called_var.magic = s_var_ptr->magic;
            called_var.acl = s_var_ptr->acl;
            called_var.findVar = s_var_ptr->findVar;
            access =
                (*(s_var_ptr->findVar)) (&called_var, name, &namelen,
                                         1, &var_len, &write_method);

            if (access
                && snmp_oid_compare(name, namelen, tree_ptr->end_a,
                                    tree_ptr->end_len) > 0) {
                memcpy(name, tree_ptr->end_a, tree_ptr->end_len);
                access = 0;
                ag_trace("access := 0");
            }

            if (access) {

                /*
                 * check 'var_len' ?
                 */

                /*
                 * check type
                 */
                switch (called_var.type) {
                case ASN_INTEGER:
                case ASN_COUNTER:
                case ASN_TIMETICKS:
                case ASN_GAUGE:
                case ASN_COUNTER64:
                    break;
                default:
                    ag_trace("invalid type: %d",
                             (int) called_var.type);
                    return ROW_ERR_GENERR;
                }
                *new_value = *(u_long *) access;
                return ROW_ERR_NOERROR;
            }
        }
    }

    return ROW_ERR_NOSUCHNAME;
}

static void
alarm_check_var(unsigned int clientreg, void *clientarg)
{
    RMON_ENTRY_T       *hdr_ptr;
    ALARM_CRTL_ENTRY_T *body;
    u_long             new_value;
    int                ierr;

    T_D("enter");
    hdr_ptr = (RMON_ENTRY_T *) clientarg;
    if (!hdr_ptr) {
        T_D
        ("Err: history_get_backet: hdr_ptr=NULL ? (Inserted in shock)");
        return;
    }

    body = (ALARM_CRTL_ENTRY_T *) hdr_ptr->body;
    if (!body) {
        T_D
        ("Err: history_get_backet: body=NULL ? (Inserted in shock)");
        return;
    }

    if (RMON1_ENTRY_VALID != hdr_ptr->status) {
        T_D("Err: history_get_backet when entry %d is not valid ?!!",
            (int) hdr_ptr->ctrl_index);
        ROWAPI_alarm_unregister(body->timer_id);
        return;
    }

    ierr = fetch_var_val(body->var_name.objid,
                         body->var_name.length, &new_value);
    if (ROW_ERR_NOERROR != ierr) {
        T_D("Err(%d): Can't fetch var_name", ierr);
        return;
    }
    T_D("new_value = %lu", new_value);

    body->value = (VTSS_APPL_RMON_SAMPLE_TYPE_ABSOLUTE == body->sample_type) ?
                  new_value : new_value - body->last_abs_value;

    body->last_abs_value = new_value;
    /*
     * ag_trace ("fetched value=%ld check %ld", (long) new_value, (long) body->value);
     */
#if 0                           /* KUKU */
    kuku_sum += body->value;
    kuku_cnt++;
#endif
    static char descr[] = "Rising";
    static char descr2[] = "Falling";
    if (VTSS_APPL_RMON_ALARM_RISING != body->prev_alarm &&
        body->value >= body->rising_threshold &&
        ROW_ERR_NOERROR == event_api_send_alarm(1, hdr_ptr->ctrl_index,
                                                body->rising_event_index,
                                                body->var_name.objid,
                                                body->var_name.length,
                                                body->sample_type, body->value,
                                                body->rising_threshold,
                                                descr)) {
        body->prev_alarm = VTSS_APPL_RMON_ALARM_RISING;
    } else if (VTSS_APPL_RMON_ALARM_FALLING != body->prev_alarm &&
               body->value <= body->falling_threshold &&
               ROW_ERR_NOERROR == event_api_send_alarm(0,
                                                       hdr_ptr->ctrl_index,
                                                       body->
                                                       falling_event_index,
                                                       body->var_name.objid,
                                                       body->var_name.
                                                       length, body->sample_type,
                                                       body->value,
                                                       body->
                                                       falling_threshold,
                                                       descr2)) {
        body->prev_alarm = VTSS_APPL_RMON_ALARM_FALLING;
    }
}

/*
 * Control Table RowApi Callbacks
 */

int
alarm_Create(RMON_ENTRY_T *eptr)
{
    /* create the body: alloc it and set defaults */
    ALARM_CRTL_ENTRY_T   *body;

    eptr->body = AGMALLOC(sizeof(ALARM_CRTL_ENTRY_T));
    if (!eptr->body) {
        return -3;
    }
    body = (ALARM_CRTL_ENTRY_T *) eptr->body;

    /*
     * set defaults
     */
    body->interval = 30;
    /* default alarm variable
    memcpy(&body->var_name, &alarmIndex, sizeof(VAR_OID_T));
    body->var_name.objid[body->var_name.length - 1] = eptr->ctrl_index; */
    memset(body->var_name.objid, 0x0, sizeof(body->var_name.objid));
    body->var_name.length = 2;
    body->sample_type = VTSS_APPL_RMON_SAMPLE_TYPE_DELTA;
    body->startup_type = VTSS_APPL_RMON_ALARM_BOTH;
    body->rising_threshold = 0;
    body->falling_threshold = 0;
    body->rising_event_index = body->falling_event_index = 0;
    body->value = 0;

    body->prev_alarm = VTSS_APPL_RMON_ALARM_NOTHING;

    return 0;
}

int
alarm_Validate(RMON_ENTRY_T *eptr)
{
    ALARM_CRTL_ENTRY_T *body = (ALARM_CRTL_ENTRY_T *) eptr->body;
    datasourceTable_info_t     table_info;
    int                if_index;
    oid                temp_objid[MAX_OID_LEN];

    if (body->rising_threshold <= body->falling_threshold) {
        ag_trace("alarm_Validate failed: %lu must be > %lu",
                 body->rising_threshold, body->falling_threshold);
        return ROW_ERR_BADVALUE;
    }

#if 0 /* allowed the 'variable' accept more than RMON statistics OID */
    /* check alarm variable length */
    if (body->var_name.length != alarmIndex.length) {
        return -1;
    }

    /* check alarm OID */
    if (snmp_oid_compare
        (body->var_name.objid, body->var_name.length - 2,
         alarmIndex.objid, alarmIndex.length - 2)) {
        return -1;
    }
#endif

    if (body->var_name.length > SNMP_MAX_ALRAM_VARIABLE_LEN) {
        return -1;
    }

    memcpy(temp_objid, body->var_name.objid, sizeof(oid) * body->var_name.length);
    /* check entry is valid */
    if_index = body->var_name.objid[body->var_name.length - 1];

    if (get_datasource_info(if_index, &table_info)) {
        if (table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_PORT ||
            table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_LLAG ||
            table_info.type == (ifIndex_type_t)DATASOURCE_IFINDEX_TYPE_GLAG) {
            /* save SNMP RMON alarm valid entry */
            rmon_alarm_entry_t entry;

            entry.ctrl_index = eptr->ctrl_index;
            (void)snmp_mgmt_rmon_alarm_entry_get(&entry, FALSE);
            entry.valid = 1;
            entry.interval = body->interval;
            memcpy(entry.variable, body->var_name.objid, sizeof(oid) * body->var_name.length);
            entry.variable_len = body->var_name.length;
            entry.sample_type = body->sample_type;
            entry.startup_alarm =  body->startup_type;
            entry.rising_threshold = body->rising_threshold;
            entry.falling_threshold = body->falling_threshold;
            entry.rising_event_index = body->rising_event_index;
            entry.falling_event_index = body->falling_event_index;
            if (snmp_mgmt_rmon_alarm_entry_set(&entry) != VTSS_RC_OK) {
                return -1;
            }

            return 0;
        } else {
            return RMON_ERROR_INVALID_PARAM;
        }
    }

    return 0;
}

int
alarm_Activate(RMON_ENTRY_T *eptr)
{
    ALARM_CRTL_ENTRY_T   *body = (ALARM_CRTL_ENTRY_T *) eptr->body;
    int             ierr;
    u_long          tmp_last_abs_value;

    ierr = fetch_var_val(body->var_name.objid,
                         body->var_name.length, &tmp_last_abs_value);
    if (ROW_ERR_NOERROR != ierr) {
        T_D("Can't fetch var_name");
        body->last_abs_value = 0;
    } else {
        body->last_abs_value = tmp_last_abs_value;
    }

    if (VTSS_APPL_RMON_SAMPLE_TYPE_ABSOLUTE != body->sample_type) {
        /*
         * check startup alarm
         */
        static char descr3[] = "Startup Rising";
        static char descr4[] = "Startup Falling";
        if (VTSS_APPL_RMON_ALARM_RISING == body->startup_type ||
            VTSS_APPL_RMON_ALARM_BOTH == body->startup_type) {
            if ( body->rising_threshold < 0 || body->last_abs_value >= ( unsigned int )body->rising_threshold ) {
                (void)event_api_send_alarm(1, eptr->ctrl_index,
                                           body->rising_event_index,
                                           body->var_name.objid,
                                           body->var_name.length,
                                           body->sample_type, body->value,
                                           body->rising_threshold,
                                           descr3);
            }
        }

        if (VTSS_APPL_RMON_ALARM_FALLING == body->startup_type ||
            VTSS_APPL_RMON_ALARM_BOTH == body->startup_type) {
            if ( body->last_abs_value < 0x80000000 && (  (int ) body->last_abs_value <= body->falling_threshold ) ) {
                (void)event_api_send_alarm(0, eptr->ctrl_index,
                                           body->falling_event_index,
                                           body->var_name.objid,
                                           body->var_name.length,
                                           body->sample_type, body->value,
                                           body->falling_threshold,
                                           descr4);
            }
        }

    }

    body->timer_id = ROWAPI_alarm_register(body->interval, RMON_TIMER_REPEAT,
                                           alarm_check_var, eptr);
    return 0;
}

int
alarm_Deactivate(RMON_ENTRY_T *eptr)
{
    ALARM_CRTL_ENTRY_T      *body = (ALARM_CRTL_ENTRY_T *) eptr->body;
    rmon_alarm_entry_t entry;

    ROWAPI_alarm_unregister(body->timer_id);

    /* delete SNMP RMON alarm entry */
    if (alarm_delete_all_flag) {
        return 0;
    }

    entry.ctrl_index = eptr->ctrl_index;

    if (snmp_mgmt_rmon_alarm_entry_get(&entry, FALSE) != VTSS_RC_OK) {
        return -1;
    }
    entry.valid = 0;
    if (snmp_mgmt_rmon_alarm_entry_set(&entry) != VTSS_RC_OK) {
        return -1;
    }

    return 0;
}

int
alarm_Copy(RMON_ENTRY_T *eptr)
{
    ALARM_CRTL_ENTRY_T   *body = (ALARM_CRTL_ENTRY_T *) eptr->body;
    ALARM_CRTL_ENTRY_T   *clone = (ALARM_CRTL_ENTRY_T *) eptr->tmp;

    if (RMON1_ENTRY_VALID == eptr->status &&
        clone->rising_threshold <= clone->falling_threshold) {
        ag_trace("alarm_Copy failed: invalid thresholds");
        return ROW_ERR_BADVALUE;
    }

    if (clone->interval != body->interval) {
        if (RMON1_ENTRY_VALID == eptr->status) {
            ROWAPI_alarm_unregister(body->timer_id);
            body->timer_id =
                ROWAPI_alarm_register(clone->interval, RMON_TIMER_REPEAT,
                                      alarm_check_var, eptr);
        }
        body->interval = clone->interval;
    }

#ifdef EXTEND_RMON_TO_WEB_CLI
    body->id = clone->id;
#endif

    if (snmp_oid_compare(clone->var_name.objid, clone->var_name.length,
                         body->var_name.objid, body->var_name.length)) {
        memcpy(&body->var_name, &clone->var_name, sizeof(VAR_OID_T));
    }

    body->sample_type = clone->sample_type;
    body->startup_type = clone->startup_type;
    body->sample_type = clone->sample_type;
    body->rising_threshold = clone->rising_threshold;
    body->falling_threshold = clone->falling_threshold;
    body->rising_event_index = clone->rising_event_index;
    body->falling_event_index = clone->falling_event_index;
    /*
     * ag_trace ("alarm_Copy: rising_threshold=%lu falling_threshold=%lu",
     * body->rising_threshold, body->falling_threshold);
     */
    return 0;
}

int
add_alarm_entry(int ctrl_index,
                u_long interval,
                ulong variable[SNMP_MAX_ALRAM_VARIABLE_LEN],
                u_long variable_len,
                u_long sample_type,
                u_long startup_alarm,
                u_long rising_threshold,
                u_long falling_threshold,
                u_long rising_event_index,
                u_long falling_event_index)
{
    int             ierr;

    ierr = ROWAPI_new(alarm_table_ptr, ctrl_index);
    switch (ierr) {
    case -1:
        ag_trace("max. number exedes\n");
        break;
    case -2:
        ag_trace("VTSS_MALLOC() failed");
        break;
    case -3:
        ag_trace("ClbkCreate failed");
        break;
    case 0:
        break;
    default:
        ag_trace("Unknown code %d", ierr);
        break;
    }

    if (!ierr) {
        RMON_ENTRY_T *eptr = ROWAPI_find(alarm_table_ptr, ctrl_index);
        if (!eptr) {
            ag_trace("cannot find alarm entry %ld", ctrl_index);
            ierr = -4;
        } else {
            ALARM_CRTL_ENTRY_T *body = (ALARM_CRTL_ENTRY_T *) eptr->body;

            memcpy(body->var_name.objid, variable, sizeof(oid) * variable_len);
            body->var_name.length = variable_len;
            body->interval = interval;
            body->sample_type = (vtss_alarm_sample_type_t)sample_type;
            body->startup_type = (vtss_alarm_type_t)startup_alarm;
            body->rising_threshold = rising_threshold;
            body->falling_threshold = falling_threshold;
            body->rising_event_index = rising_event_index;
            body->falling_event_index = falling_event_index;
#ifdef EXTEND_RMON_TO_WEB_CLI
            body->id = ctrl_index;
#endif

            eptr->new_status = RMON1_ENTRY_VALID;
            ierr = ROWAPI_commit(alarm_table_ptr, ctrl_index);
            if (ierr) {
                ag_trace("ROWAPI_commit returned %d", ierr);
            }
        }
    }

    return ierr;
}

void rmon_alarm_DeleteAllRow(void)
{
    RMON_ENTRY_T *eptr;
    u_long   idx = 0, entry_num;
    u_long   *buffer;

    if (!alarm_table_ptr) {
        return;
    }

    entry_num = alarm_table_ptr->current_number_of_entries;
    if (!entry_num) {
        return;
    }
    if (!(VTSS_MALLOC_CAST(buffer, entry_num * sizeof(u_long)))) {
        return;
    }

    for (eptr = alarm_table_ptr->first; eptr; eptr = eptr->next) {
        buffer[idx++] = eptr->ctrl_index;
        if (idx > alarm_table_ptr->current_number_of_entries) {
            VTSS_FREE(buffer);
            return;
        }
    }

    alarm_delete_all_flag = TRUE;
    for (idx = 0; idx < entry_num; idx++) {
        eptr = ROWAPI_find(alarm_table_ptr, buffer[idx]);
        if (eptr) {
            ROWAPI_delete_clone(alarm_table_ptr, buffer[idx]);
            rowapi_delete(eptr);
        }
    }
    alarm_delete_all_flag = FALSE;

    VTSS_FREE(buffer);
}

void rmon_create_alarm_default_entry(void)
{
    rmon_alarm_entry_t entry;

    if (alarm_table_ptr) {
        rmon_alarm_DeleteAllRow();
    } else {
        return;
    }

    /* Create default RMON alarm row entries form SNMP manager module */
    entry.ctrl_index = 0;
    while (snmp_mgmt_rmon_alarm_entry_get(&entry, TRUE) == VTSS_RC_OK) {
        (void)add_alarm_entry(entry.ctrl_index,
                              entry.interval,
                              entry.variable,
                              entry.variable_len,
                              entry.sample_type,
                              entry.startup_alarm,
                              entry.rising_threshold,
                              entry.falling_threshold,
                              entry.rising_event_index,
                              entry.falling_event_index);
    }
}

/* event ----------------------------------------------------------*/

/*
 * File scope definitions section
 */


/*
 * defaults & limitations
 */

#define MAX_LOG_ENTRIES_PER_CTRL    200

#ifndef EXTEND_RMON_TO_WEB_CLI
typedef struct event_data_struct_t {
    struct data_struct_t *next;
    u_long          data_index;
    u_long          log_time;
    char           *log_description;
} EVENT_DATA_ENTRY_T;

typedef enum {
    VTSS_APPL_RMON_EVENT_NONE = 1,
    VTSS_APPL_RMON_EVENT_LOG,
    VTSS_APPL_RMON_EVENT_TRAP,
    VTSS_APPL_RMON_EVENT_LOG_AND_TRAP
} EVENT_TYPE_T;

typedef struct {
    char           *event_description;
    EVENT_TYPE_T    event_type;
    u_long          event_last_time_sent;

    SCROLLER_T      scrlr;
#if 0
    u_long          event_last_logged_index;
    u_long          event_number_of_log_entries;
    EVENT_DATA_ENTRY_T   *log_list;
    EVENT_DATA_ENTRY_T   *last_log_ptr;
#endif
} EVENT_CRTL_ENTRY_T;

#else
typedef vtss_event_data_entry_t EVENT_DATA_ENTRY_T;
typedef vtss_event_type_t EVENT_TYPE_T;
typedef vtss_event_ctrl_entry_t EVENT_CRTL_ENTRY_T;
#endif
/*
 * Main section
 */

static BOOL event_delete_all_flag = FALSE;

/*
 * Control Table RowApi Callbacks
 */

static int
data_destructor(SCROLLER_T *scrlr, void *free_me)
{
    EVENT_DATA_ENTRY_T   *lptr = (EVENT_DATA_ENTRY_T *)free_me;

    if (lptr->log_description) {
        AGFREE(lptr->log_description);
    }

    return 0;
}

int
event_Create(RMON_ENTRY_T *eptr)
{
    /* create the body: alloc it and set defaults */
    EVENT_CRTL_ENTRY_T   *body;

    eptr->body = AGMALLOC(sizeof(EVENT_CRTL_ENTRY_T));
    if (!eptr->body) {
        return -3;
    }
    body = (EVENT_CRTL_ENTRY_T *) eptr->body;

    /*
     * set defaults
     */

    body->event_description = NULL;
    body->event_type = VTSS_APPL_RMON_EVENT_NONE;
    body->event_last_time_sent = 0;
    (void)ROWDATAAPI_init(&body->scrlr,
                          MAX_LOG_ENTRIES_PER_CTRL,
                          MAX_LOG_ENTRIES_PER_CTRL,
                          sizeof(EVENT_DATA_ENTRY_T), data_destructor);


    return 0;
}

int
event_Clone(RMON_ENTRY_T *eptr)
{
    /* copy entry_bod -> clone */
    EVENT_CRTL_ENTRY_T   *body = (EVENT_CRTL_ENTRY_T *) eptr->body;
    EVENT_CRTL_ENTRY_T   *clone = (EVENT_CRTL_ENTRY_T *) eptr->tmp;

    if (body->event_description) {
        clone->event_description = AGSTRDUP(body->event_description);
    }

    return 0;
}

int
event_Copy(RMON_ENTRY_T *eptr)
{
    EVENT_CRTL_ENTRY_T   *body = (EVENT_CRTL_ENTRY_T *) eptr->body;
    EVENT_CRTL_ENTRY_T   *clone = (EVENT_CRTL_ENTRY_T *) eptr->tmp;

#ifdef EXTEND_RMON_TO_WEB_CLI
    body->id = clone->id;
#endif

    if (body->event_type != clone->event_type) {
        body->event_type = clone->event_type;
    }

    if (clone->event_description) {
        if (body->event_description) {
            AGFREE(body->event_description);
        }
        body->event_description = AGSTRDUP(clone->event_description);
    }

    return 0;
}

int
event_Delete(RMON_ENTRY_T *eptr)
{
    EVENT_CRTL_ENTRY_T   *body = (EVENT_CRTL_ENTRY_T *) eptr;

    if (body->event_description) {
        AGFREE(body->event_description);
    }

    return 0;
}

int
event_Validate(RMON_ENTRY_T *eptr)
{
    EVENT_CRTL_ENTRY_T *body;
    rmon_event_entry_t entry;

    /* save SNMP RMON event valid entry */
    body = (EVENT_CRTL_ENTRY_T *) eptr->body;
    entry.ctrl_index = eptr->ctrl_index;
    (void)snmp_mgmt_rmon_event_entry_get(&entry, FALSE);
    entry.valid = 1;
    if (body->event_description) {
        strcpy(entry.description, body->event_description);
    } else {
        strcpy(entry.description, "");
    }
    entry.type = body->event_type;
    if (snmp_mgmt_rmon_event_entry_set(&entry) != VTSS_RC_OK) {
        return -1;
    }

    return 0;
}

int
event_Activate(RMON_ENTRY_T *eptr)
{
    /* init logTable */
    EVENT_CRTL_ENTRY_T   *body = (EVENT_CRTL_ENTRY_T *) eptr->body;

    ROWDATAAPI_set_size(&body->scrlr,
                        body->scrlr.data_requested,
                        (u_char)(RMON1_ENTRY_VALID == eptr->status) );

    return 0;
}

int
event_Deactivate(RMON_ENTRY_T *eptr)
{
    /* free logTable */
    EVENT_CRTL_ENTRY_T      *body = (EVENT_CRTL_ENTRY_T *) eptr->body;
    rmon_event_entry_t entry;

    /*
     * free data list
     */
    ROWDATAAPI_descructor(&body->scrlr);

    /* delete SNMP RMON event entry */
    if (event_delete_all_flag) {
        return 0;
    }

    entry.ctrl_index = eptr->ctrl_index;

    if (snmp_mgmt_rmon_event_entry_get(&entry, FALSE) != VTSS_RC_OK) {
        return -1;
    }
    entry.valid = 0;
    if (snmp_mgmt_rmon_event_entry_set(&entry) != VTSS_RC_OK) {
        return -1;
    }

    return 0;
}

static SCROLLER_T *
event_extract_scroller(void *v_body)
{
    EVENT_CRTL_ENTRY_T   *body = (EVENT_CRTL_ENTRY_T *) v_body;
    return &body->scrlr;
}

static void
event_save_log(EVENT_CRTL_ENTRY_T *body, char *event_descr)
{
    EVENT_DATA_ENTRY_T *lptr;

    lptr = (EVENT_DATA_ENTRY_T *)ROWDATAAPI_locate_new_data(&body->scrlr);
    if (!lptr) {
        ag_trace("Err: event_save_log:cannot locate ?");
        return;
    }

    lptr->log_time = body->event_last_time_sent;
    if (lptr->log_description) {
        AGFREE(lptr->log_description);
    }
    lptr->log_description = AGSTRDUP(event_descr);
    lptr->data_index = ROWDATAAPI_get_total_number(&body->scrlr);

    /*
     * ag_trace ("log has been saved, data_index=%d", (int) lptr->data_index);
     */
}

static char    *
create_explanaition(EVENT_CRTL_ENTRY_T *evptr, u_char is_rising,
                    u_long alarm_index, u_long event_index,
                    oid *alarmed_var,
                    size_t alarmed_var_length,
                    u_long value, u_long the_threshold,
                    u_long sample_type, char *alarm_descr)
{
#define UNEQ_LENGTH (1 + 11 + 4 + 11 + 1 + 20)
    char            expl[UNEQ_LENGTH];

    /*lint -esym(459,c_oid)*/
    static char     c_oid[SPRINT_MAX_LEN];
    size_t          sz;
    char           *descr;
    char  *pch;
    char  *tmp;

    /* it's only using for newer NET-SNMP version
    snprint_objid(c_oid, sizeof(c_oid)-1, alarmed_var, alarmed_var_length); */
    (void)sprint_objid(c_oid, alarmed_var, alarmed_var_length);
    c_oid[sizeof(c_oid) - 1] = '\0';
    for (pch = c_oid;;) {
        tmp = strchr(pch, '.');
        if (!tmp) {
            break;
        }
        if (isdigit(tmp[1]) || '"' == tmp[1]) {
            break;
        }
        pch = tmp + 1;
    }

    (void)snprintf(expl, UNEQ_LENGTH, "=%ld %s= %ld :%ld, %ld",
                   (unsigned long) value,
                   is_rising ? ">" : "<",
                   (unsigned long) the_threshold,
                   (long) alarm_index, (long) event_index);
    sz = 3 + strlen(expl) + strlen(pch);
    if (alarm_descr) {
        sz += strlen(alarm_descr);
    }

    descr = (char *)AGMALLOC(sz);
    if (!descr) {
        ag_trace("Can't allocate event description");
        return NULL;
    }

    if (alarm_descr) {
        strcpy(descr, alarm_descr);
        strcat(descr, ":");
    } else {
        *descr = '\0';
    }

    strcat(descr, pch);
    strcat(descr, expl);

    (void)event_save_log(evptr, descr);

    if (descr) {
        AGFREE(descr);
    }

    descr = alarm_descr;
    return descr;
}

void
event_send_trap(u_char is_rising,
                u_int alarm_index,
                u_int value, u_int the_threshold,
                oid *alarmed_var, size_t alarmed_var_length,
                u_int sample_type)
{
    oid      rmon1_trap_oid[]  = { 1, 3, 6, 1, 2, 1, 16, 0, 0 };
    oid      alarm_index_oid[] = { 1, 3, 6, 1, 2, 1, 16, 3, 1, 1, 1 };
    oid      alarmed_var_oid[] = { 1, 3, 6, 1, 2, 1, 16, 3, 1, 1, 3 };
    oid      sample_type_oid[] = { 1, 3, 6, 1, 2, 1, 16, 3, 1, 1, 4 };
    oid      value_oid[]       = { 1, 3, 6, 1, 2, 1, 16, 3, 1, 1, 5 };
    oid      threshold_oid[]   = { 1, 3, 6, 1, 2, 1, 16, 3, 1, 1, 7 };     /* rising case */
    struct variable_list *top = NULL;
    snmp_vars_trap_entry_t  trap_entry;
    int    iii;

    /*
     * set the last 'oid' : risingAlarm or fallingAlarm
     */
    memset (&trap_entry, 0x0, sizeof(trap_entry));
    if (is_rising) {
        strncpy(trap_entry.name, TRAP_NAME_RMON_RISING, TRAP_MAX_TABLE_NAME_LEN);
        iii = OID_LENGTH(rmon1_trap_oid);
        rmon1_trap_oid[iii - 1] = 1;
        iii = OID_LENGTH(threshold_oid);
        threshold_oid[iii - 1] = 7;
    } else {
        strncpy(trap_entry.name, TRAP_NAME_RMON_FALLING, TRAP_MAX_TABLE_NAME_LEN);
        iii = OID_LENGTH(rmon1_trap_oid);
        rmon1_trap_oid[iii - 1] = 2;
        iii = OID_LENGTH(threshold_oid);
        threshold_oid[iii - 1] = 8;
    }

    trap_entry.oid_len = OID_LENGTH(rmon1_trap_oid);
    memcpy(trap_entry.oid, rmon1_trap_oid,
           sizeof(oid) * trap_entry.oid_len);

    /*
     * build the var list
     */
    top = snmp_bind_var(top, &alarm_index, ASN_INTEGER, sizeof(u_int),
                        alarm_index_oid, OID_LENGTH(alarm_index_oid));

    top =
        snmp_bind_var(top, alarmed_var, ASN_OBJECT_ID,
                      sizeof(oid) * alarmed_var_length, alarmed_var_oid,
                      OID_LENGTH(alarmed_var_oid));

    top = snmp_bind_var(top, &sample_type, ASN_INTEGER, sizeof(u_int),
                        sample_type_oid, OID_LENGTH(sample_type_oid));

    top = snmp_bind_var(top, &value, ASN_INTEGER, sizeof(u_int),
                        value_oid, OID_LENGTH(value_oid));

    top = snmp_bind_var(top, &the_threshold, ASN_INTEGER, sizeof(u_int),
                        threshold_oid, OID_LENGTH(threshold_oid));

    trap_entry.vars = top;

    snmp_send_vars_trap(&trap_entry);

}

int
event_api_send_alarm(u_char is_rising,
                     u_long alarm_index,
                     u_long event_index,
                     oid *alarmed_var,
                     size_t alarmed_var_length,
                     u_long sample_type,
                     u_long value, u_long the_threshold, char *alarm_descr)
{
    RMON_ENTRY_T   *eptr;
    EVENT_CRTL_ENTRY_T   *evptr;

    if (!event_index) {
        return ROW_ERR_NOSUCHNAME;
    }

#if 0
    ag_trace("event_api_send_alarm(%d,%d,%d,'%s')",
             (int) is_rising, (int) alarm_index, (int) event_index,
             alarm_descr);
#endif
    eptr = ROWAPI_find(event_table_ptr, event_index);
    if (!eptr) {
        /*
         * ag_trace ("event cannot find entry %ld", event_index);
         */
        return ROW_ERR_NOSUCHNAME;
    }

    evptr = (EVENT_CRTL_ENTRY_T *) eptr->body;
    evptr->event_last_time_sent = AGUTIL_sys_up_time();


    if (VTSS_APPL_RMON_EVENT_TRAP == evptr->event_type
        || VTSS_APPL_RMON_EVENT_LOG_AND_TRAP == evptr->event_type) {
        /* peter, 2007/8,
        event_send_trap(evptr, is_rising, alarm_index, value,
                        the_threshold, alarmed_var, alarmed_var_length,
                        sample_type); */
#ifdef VTSS_SW_OPTION_SNMP
        event_send_trap(is_rising, alarm_index, value,
                        the_threshold, alarmed_var, alarmed_var_length,
                        sample_type);

#endif
    }

    if (VTSS_APPL_RMON_EVENT_LOG == evptr->event_type
        || VTSS_APPL_RMON_EVENT_LOG_AND_TRAP == evptr->event_type) {
//        register char  *explain;

        (void) create_explanaition(evptr, is_rising,
                                   alarm_index, event_index,
                                   alarmed_var, alarmed_var_length,
                                   value, the_threshold,
                                   sample_type, alarm_descr);

#if 0
        (void)event_save_log(evptr, explain);

        if (explain != NULL) {
            AGFREE(explain);
        }
#endif
    }

    return ROW_ERR_NOERROR;
}

int
add_event_entry(int ctrl_index,
                char *event_description,
                EVENT_TYPE_T event_type)
{
    int             ierr;

    ierr = ROWAPI_new(event_table_ptr, ctrl_index);
    switch (ierr) {
    case -1:
        ag_trace("max. number exedes\n");
        break;
    case -2:
        ag_trace("VTSS_MALLOC() failed");
        break;
    case -3:
        ag_trace("ClbkCreate failed");
        break;
    case 0:
        break;
    default:
        ag_trace("Unknown code %d", ierr);
        break;
    }

    if (!ierr) {
        RMON_ENTRY_T *eptr = ROWAPI_find(event_table_ptr, ctrl_index);
        if (!eptr) {
            ag_trace("cannot find event entry %ld", ctrl_index);
            ierr = -4;
        } else {
            EVENT_CRTL_ENTRY_T *body = (EVENT_CRTL_ENTRY_T *) eptr->body;

            if (event_description) {
                if (body->event_description) {
                    AGFREE(body->event_description);
                }
                body->event_description = AGSTRDUP(event_description);
            }
            body->event_type = event_type;
#ifdef EXTEND_RMON_TO_WEB_CLI
            body->id = ctrl_index;
#endif

            eptr->new_status = RMON1_ENTRY_VALID;
            ierr = ROWAPI_commit(event_table_ptr, ctrl_index);
            if (ierr) {
                ag_trace("ROWAPI_commit returned %d", ierr);
            }
        }
    }

    return ierr;
}

void rmon_event_DeleteAllRow(void)
{
    RMON_ENTRY_T *eptr;
    u_long   idx = 0, entry_num;
    u_long   *buffer;

    if (!event_table_ptr) {
        return;
    }
    entry_num = event_table_ptr->current_number_of_entries;
    if (!entry_num) {
        return;
    }
    if (!(VTSS_MALLOC_CAST(buffer, entry_num * sizeof(u_long)))) {
        return;
    }

    for (eptr = event_table_ptr->first; eptr; eptr = eptr->next) {
        buffer[idx++] = eptr->ctrl_index;
        if (idx > event_table_ptr->current_number_of_entries) {
            VTSS_FREE(buffer);
            return;
        }
    }

    event_delete_all_flag = TRUE;
    for (idx = 0; idx < entry_num; idx++) {
        eptr = ROWAPI_find(event_table_ptr, buffer[idx]);
        if (eptr) {
            ROWAPI_delete_clone(event_table_ptr, buffer[idx]);
            rowapi_delete(eptr);
        }
    }
    event_delete_all_flag = FALSE;

    VTSS_FREE(buffer);
}

void rmon_create_event_default_entry(void)
{
    rmon_event_entry_t entry;

    if (event_table_ptr) {
        rmon_event_DeleteAllRow();
    } else {
        return;
    }

    /* Create default RMON event row entries form SNMP manager module */
    entry.ctrl_index = 0;
    while (snmp_mgmt_rmon_event_entry_get(&entry, TRUE) == VTSS_RC_OK) {
        (void)add_event_entry(entry.ctrl_index, entry.description, (EVENT_TYPE_T)entry.type);
    }
}


RMON_ENTRY_T   *RMON_header_ControlEntry(struct variable *vp, oid *name,
                                         size_t *length, int exact,
                                         size_t *var_len,
                                         RMON_TABLE_INDEX_T table_index,
                                         void *entry_ptr,
                                         size_t entry_size)
{
    RMON_ENTRY_T *entry = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    RMON_CRIT_ENTER(table_ptr);

    entry =  ROWAPI_header_ControlEntry(vp, name,
                                        length, exact,
                                        var_len,
                                        table_ptr,
                                        entry_ptr,
                                        entry_size);
    RMON_CRIT_EXIT(table_ptr);
    return entry;
}

RMON_ENTRY_T   *RMON_header_DataEntry(struct variable *vp,
                                      oid *name, size_t *length,
                                      int exact, size_t *var_len,
                                      RMON_TABLE_INDEX_T table_index,
                                      size_t data_size,
                                      void *entry_ptr)
{
    RMON_ENTRY_T *entry = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    RMON_CRIT_ENTER(table_ptr);

    entry =  ROWDATAAPI_header_DataEntry(vp,
                                         name, length,
                                         exact, var_len,
                                         table_ptr,
                                         table_ptr->extract_scroller,
                                         data_size,
                                         entry_ptr);
    RMON_CRIT_EXIT(table_ptr);
    return entry;
}

int             RMON_do_another_action(oid *name,
                                       int tbl_first_index_begin,
                                       int action, int *prev_action,
                                       RMON_TABLE_INDEX_T table_index,
                                       size_t entry_size)
{
    int rc = 0;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    RMON_CRIT_ENTER(table_ptr);
    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   action, prev_action,
                                   table_ptr,
                                   entry_size);
    RMON_CRIT_EXIT(table_ptr);
    return rc;

}





RMON_ENTRY_T   *RMON_find(RMON_TABLE_INDEX_T table_index,
                          u_long ctrl_index)
{
    RMON_ENTRY_T *entry = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    RMON_CRIT_ENTER(table_ptr);
    entry =  ROWAPI_find(table_ptr, ctrl_index);
    RMON_CRIT_EXIT(table_ptr);
    return entry;
}

RMON_ENTRY_T   *RMON_next(RMON_TABLE_INDEX_T table_index,
                          u_long ctrl_index)
{
    RMON_ENTRY_T *entry = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    RMON_CRIT_ENTER(table_ptr);
    entry =  ROWAPI_next(table_ptr, ctrl_index);
    RMON_CRIT_EXIT(table_ptr);
    return entry;
}



void            RMON_delete_clone(RMON_TABLE_INDEX_T table_index,
                                  u_long ctrl_index)
{
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    RMON_CRIT_ENTER(table_ptr);
    ROWAPI_delete_clone(table_ptr,
                        ctrl_index);
    RMON_CRIT_EXIT(table_ptr);

}

/* peter, 2007/8, modify for dynamic row entry delete */
void RMON_delete(RMON_ENTRY_T *eold)
{
    rowapi_delete(eold);
}

int             RMON_new(RMON_TABLE_INDEX_T table_index,
                         u_long ctrl_index)
{
    int rc = 0;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    RMON_CRIT_ENTER(table_ptr);
    rc =  ROWAPI_new(table_ptr,
                     ctrl_index);
    RMON_CRIT_EXIT(table_ptr);
    return rc;
}

int             RMON_commit(RMON_TABLE_INDEX_T table_index,
                            u_long ctrl_index)
{
    int rc = 0;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    RMON_CRIT_ENTER(table_ptr);
    rc =  ROWAPI_commit(table_ptr,
                        ctrl_index);
    RMON_CRIT_EXIT(table_ptr);
    return rc;
}





static  void            RMON_init_table(RMON_TABLE_INDEX_T table_index,
                                        const char *name,
                                        u_long max_number_of_entries,
                                        ENTRY_CALLBACK_T *ClbkCreate,
                                        ENTRY_CALLBACK_T *ClbkClone,
                                        ENTRY_CALLBACK_T *ClbkDelete,
                                        ENTRY_CALLBACK_T *ClbkValidate,
                                        ENTRY_CALLBACK_T *ClbkActivate,
                                        ENTRY_CALLBACK_T *ClbkDeactivate,
                                        ENTRY_CALLBACK_T *ClbkCopy,
                                        SCROLLER_T * (*extract_scroller) (void *body))
{

    TABLE_DEFINTION_T *table_ptr = rmonTable_find(table_index);
    critd_init(&table_ptr->crit, name, VTSS_MODULE_ID_RMON, CRITD_TYPE_MUTEX);

    ROWAPI_init_table(table_ptr,
                      name,
                      max_number_of_entries,
                      ClbkCreate,
                      ClbkClone,
                      ClbkDelete,
                      ClbkValidate,
                      ClbkActivate,
                      ClbkDeactivate,
                      ClbkCopy,
                      extract_scroller);
}

void RMON_init_stats_table(void)
{
    RMON_init_table(RMON_STATS_TABLE_INDEX, MIB_DESCR, RMON_STAT_MAX_ROW_SIZE, stat_Create, NULL, /* &stat_Clone, */
                    NULL,     /* &stat_Delete, */
                    stat_Validate,
                    stat_Activate, stat_Deactivate, stat_Copy, NULL);

    stat_table_ptr = rmonTable_find(RMON_STATS_TABLE_INDEX);

}

void RMON_init_history_table(void)
{
    (void)RMON_init_table(RMON_HISTORY_TABLE_INDEX, "History", RMON_HISTORY_MAX_ROW_SIZE, history_Create, NULL,   /* &history_Clone, */
                          NULL,     /* &history_Delete, */
                          history_Validate,
                          history_Activate,
                          history_Deactivate, history_Copy, history_extract_scroller);
    history_table_ptr  = rmonTable_find(RMON_HISTORY_TABLE_INDEX);

}

void RMON_init_alarm_table(void)
{
    (void)RMON_init_table(RMON_ALARM_TABLE_INDEX, "Alarm", RMON_ALARM_MAX_ROW_SIZE, alarm_Create, NULL, /* &alarm_Clone, */
                          NULL,     /* &alarm_Delete, */
                          alarm_Validate,
                          alarm_Activate, alarm_Deactivate, alarm_Copy, NULL);
    alarm_table_ptr  = rmonTable_find(RMON_ALARM_TABLE_INDEX);

}

void RMON_init_event_table(void)
{
    (void)RMON_init_table(RMON_EVENT_TABLE_INDEX, "Event", RMON_EVENT_MAX_ROW_SIZE, event_Create, event_Clone,
                          event_Delete,
                          event_Validate,
                          event_Activate,
                          event_Deactivate, event_Copy, event_extract_scroller);
    event_table_ptr  = rmonTable_find(RMON_EVENT_TABLE_INDEX);

}


void           *RMON_locate_new_data(SCROLLER_T *scrlr)
{
    return ROWDATAAPI_locate_new_data(scrlr);
}
void            RMON_descructor(SCROLLER_T *scrlr)
{
    ROWDATAAPI_descructor(scrlr);
}
void
RMON_set_size(SCROLLER_T *scrlr,
              u_long data_requested,
              u_char do_allocation)
{
    ROWDATAAPI_set_size(scrlr, data_requested, do_allocation);
}

u_long          RMON_get_total_number(SCROLLER_T *scrlr)
{
    return ROWDATAAPI_get_total_number(scrlr);
}

int             RMON_data_init(SCROLLER_T *scrlr,
                               u_long max_number_of_entries,
                               u_long data_requested,
                               size_t data_size,
                               int (*data_destructor_f) (struct
                                                         data_scroller *,
                                                         void *))
{
    return ROWDATAAPI_init(scrlr,
                           max_number_of_entries,
                           data_requested,
                           data_size,
                           data_destructor_f);
}

void            init_rmon_statistics(void)
{
    /*
     * place any other initialization junk you need here
     */
    rmon_stat_DeleteAllRow();

    RMON_init_stats_table();

    /* Create default RMON statistics row entries form SNMP manager module */
    {
        rmon_stat_entry_t entry;

        entry.ctrl_index = 0;
        while (snmp_mgmt_rmon_stat_entry_get(&entry, TRUE) == VTSS_RC_OK) {
            (void)add_statistics_entry(entry.ctrl_index, entry.if_index);
        }
    }

}
void            init_rmon_history(void)
{
    /*
     * place any other initialization junk you need here
     */
    rmon_history_DeleteAllRow();

    RMON_init_history_table();

    /* Create default RMON history row entries form SNMP manager module */
    {
        rmon_history_entry_t entry;

        entry.ctrl_index = 0;
        while (snmp_mgmt_rmon_history_entry_get(&entry, TRUE) == VTSS_RC_OK) {
            (void)add_history_entry(entry.ctrl_index, entry.if_index, entry.interval, entry.requested);
        }
    }

}
void            init_rmon_alarm(void)
{
    /*
     * place any other initialization junk you need here
     */
    rmon_alarm_DeleteAllRow();

    RMON_init_alarm_table();

    /* Create default RMON alarm row entries form SNMP manager module */
    {
        rmon_alarm_entry_t entry;

        entry.ctrl_index = 0;
        while (snmp_mgmt_rmon_alarm_entry_get(&entry, TRUE) == VTSS_RC_OK) {
            (void)add_alarm_entry(entry.ctrl_index,
                                  entry.interval,
                                  entry.variable,
                                  entry.variable_len,
                                  entry.sample_type,
                                  entry.startup_alarm,
                                  entry.rising_threshold,
                                  entry.falling_threshold,
                                  entry.rising_event_index,
                                  entry.falling_event_index);
        }
    }

}
void            init_rmon_event(void)
{
    /*
     * place any other initialization junk you need here
     */
    rmon_event_DeleteAllRow();

    RMON_init_event_table();

    /* Create default RMON event row entries form SNMP manager module */
    {
        rmon_event_entry_t entry;

        entry.ctrl_index = 0;
        while (snmp_mgmt_rmon_event_entry_get(&entry, TRUE) == VTSS_RC_OK) {
            (void)add_event_entry(entry.ctrl_index, entry.description, (EVENT_TYPE_T)entry.type);
        }
    }

}

BOOL rmon_etherStatsTable_entry_update(vtss_var_oid_t *data_source, vtss_eth_stats_t *table_entry_p)
{
    return get_etherStatsTable_entry(data_source, table_entry_p);
}

/*lint -esym(578,strchr)*/
mesa_rc vtss_appl_rmon_statistics_entry_add(u32 index, const vtss_appl_stats_ctrl_entry_t *const entry)
{
    vtss_stat_ctrl_entry_t  conf;
    u32                     i;
    oid                     if_entry_index_oid[] = IF_ENTRY_INDEX_OID;
    vtss_ifindex_elm_t      ife;

    if (entry) {
        if (vtss_ifindex_decompose(entry->ifIndex, &ife) != VTSS_RC_OK ||
            (ife.iftype != VTSS_IFINDEX_TYPE_PORT &&
             ife.iftype != VTSS_IFINDEX_TYPE_LLAG &&
             ife.iftype != VTSS_IFINDEX_TYPE_GLAG)) {
            T_D("%% Fail: the corresponding ifindex dosen't exist\n");
            return VTSS_RC_ERROR;
        }

        memset(&conf, 0, sizeof(conf));
        conf.id = index;
        (void)rmon_mgmt_statistics_entry_get(&conf, FALSE);

        conf.data_source.length = 11;
        for ( i = 0; i < 10; i++ ) {
            conf.data_source.objid[i] = if_entry_index_oid[i];
        }
        conf.data_source.objid[10] = (oid)(vtss_ifindex_cast_to_u32(entry->ifIndex));

        if (rmon_mgmt_statistics_entry_add(&conf) != VTSS_RC_OK) {
            T_D("%% Fail to add statistics entry\n");
            return VTSS_RC_ERROR;
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc rmon_mgmt_statistics_entry_add (vtss_stat_ctrl_entry_t *entry)
{
    mesa_rc rc = RMON_ERROR_GEN;
    int tbl_first_index_begin = 0;
    long                   long_temp = 0;
    RMON_ENTRY_T           *hdr = NULL;
    vtss_stat_ctrl_entry_t      *cloned_body = NULL;
    vtss_stat_ctrl_entry_t      *body = NULL;
    oid name[] = {1, 3, 6, 1, 2, 1, 16, 1, 1, 1, 1, 0};
    size_t entry_size = sizeof(vtss_stat_ctrl_entry_t);
    int prev_action = COMMIT;

    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_STATS_TABLE_INDEX);
    if (!entry) {
        return rc;
    }
    tbl_first_index_begin = sizeof(name) / sizeof(oid) - 1;
    name[tbl_first_index_begin] = entry->id;

    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);

    if (table_ptr->current_number_of_entries >=
        table_ptr->max_number_of_entries) {
        rc = RMON_ERROR_STAT_TABLE_FULL;
        goto exit_crit_section;
    }

    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   RESERVE1, &prev_action,
                                   table_ptr,
                                   entry_size);
    /*
     * get values from PDU, check them and save them in the cloned entry
     */
    long_temp = name[tbl_first_index_begin];
    hdr = ROWAPI_find(table_ptr, long_temp);        /* it MUST be OK */
    if (!hdr) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    cloned_body = (vtss_stat_ctrl_entry_t *) hdr->tmp;
    body = (vtss_stat_ctrl_entry_t *) hdr->body;

    if (!body || !cloned_body) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    memcpy(&cloned_body->data_source, &entry->data_source, sizeof(entry->data_source));
    cloned_body->id = entry->id;

    hdr->new_status = RMON1_ENTRY_UNDER_CREATION;

    rc =  ROWAPI_commit_status(name,
                               tbl_first_index_begin,
                               &prev_action,
                               table_ptr,
                               entry_size);

    if (rc) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   RESERVE1, &prev_action,
                                   table_ptr,
                                   entry_size);



    hdr->new_status = RMON1_ENTRY_VALID;

    rc =  ROWAPI_commit_status(name,
                               tbl_first_index_begin,
                               &prev_action,
                               table_ptr,
                               entry_size);

    if (rc) {
        rc = RMON_ERROR_GEN;
    }

exit_crit_section:
    RMON_CRIT_EXIT(table_ptr);
    return rc;
}

mesa_rc vtss_appl_rmon_statistics_entry_del(u32 index)
{
    vtss_stat_ctrl_entry_t entry;
    entry.id = index;

    return rmon_mgmt_statistics_entry_del(&entry);
}

mesa_rc rmon_mgmt_statistics_entry_del (vtss_stat_ctrl_entry_t *entry)
{
    mesa_rc rc = RMON_ERROR_GEN;
    RMON_ENTRY_T           *hdr = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_STATS_TABLE_INDEX);
    if (!entry) {
        return rc;
    }
    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);
    hdr = ROWAPI_find(table_ptr, entry->id);        /* it MUST be OK */
    if (!hdr) {
        rc = RMON_ERROR_STAT_ENTRY_NOT_FOUND;
        goto exit_crit_section;
    }

    hdr->new_status = RMON1_ENTRY_INVALID;
    rc = ROWAPI_commit(table_ptr, entry->id);
    if (rc) {
        rc = RMON_ERROR_GEN;
    }

exit_crit_section:
    RMON_CRIT_EXIT(table_ptr);
    return rc;

}

mesa_rc vtss_appl_rmon_statistics_stats_get(u32 index, vtss_appl_stats_statistics_entry_t *const entry)
{
    mesa_rc                 rc = VTSS_RC_ERROR;
    vtss_stat_ctrl_entry_t  conf;

    memset(&conf, 0, sizeof(vtss_stat_ctrl_entry_t));
    conf.id = index;

    if ((rc = rmon_mgmt_statistics_entry_get(&conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    if (!rmon_etherStatsTable_entry_update(&conf.data_source, &conf.eth)) {
        T_E("why is it(%s) returning false?", __FUNCTION__);
        return VTSS_RC_ERROR;
    }
    entry->eth = conf.eth;
    entry->etherStatsCreateTime = conf.etherStatsCreateTime;

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rmon_statistics_entry_get(u32 index, vtss_appl_stats_ctrl_entry_t *const entry)
{
    vtss_stat_ctrl_entry_t find_entry;
    find_entry.id = index;

    if (entry) {
        if (rmon_mgmt_statistics_entry_get(&find_entry, FALSE) == VTSS_RC_OK) {

            entry->ifIndex = vtss_ifindex_cast_from_u32_0(find_entry.data_source.objid[10]);
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc rmon_mgmt_statistics_entry_get (vtss_stat_ctrl_entry_t *entry, BOOL next)
{
    mesa_rc rc = RMON_ERROR_GEN;
    RMON_ENTRY_T *hdr = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_STATS_TABLE_INDEX);
    if (!entry) {
        return rc;
    }
    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);
    hdr = (next == TRUE ? ROWAPI_next(table_ptr, entry->id) : ROWAPI_find(table_ptr, entry->id));

    if (hdr) {
        memcpy(entry, hdr->body, sizeof(vtss_stat_ctrl_entry_t));
        RMON_CRIT_EXIT(table_ptr);
        return VTSS_RC_OK;
    }
    RMON_CRIT_EXIT(table_ptr);
    return RMON_ERROR_STAT_ENTRY_NOT_FOUND;
}

/* Local function to create or update a new RMON history entry
   Must under semaphore protection when calling the function. */
static mesa_rc RMON_history_entry_add(TABLE_DEFINTION_T *table_ptr, vtss_history_ctrl_entry_t *entry)
{
    mesa_rc rc = RMON_ERROR_GEN;
    int tbl_first_index_begin = 0;
    long                   long_temp = 0;
    RMON_ENTRY_T           *hdr = NULL;
    vtss_history_ctrl_entry_t      *cloned_body = NULL;
    vtss_history_ctrl_entry_t      *body = NULL;
    oid name[] = {1, 3, 6, 1, 2, 1, 16, 2, 1, 1, 1, 0};
    size_t entry_size = sizeof(vtss_history_ctrl_entry_t);
    int prev_action = COMMIT;

    tbl_first_index_begin = sizeof(name) / sizeof(oid) - 1;
    name[tbl_first_index_begin] = entry->id;

    if (table_ptr->current_number_of_entries >=
        table_ptr->max_number_of_entries) {
        rc = RMON_ERROR_HISTORY_TABLE_FULL;
        goto exit_crit_section;
    }

    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   RESERVE1, &prev_action,
                                   table_ptr,
                                   entry_size);
    /*
     * get values from PDU, check them and save them in the cloned entry
     */
    long_temp = name[tbl_first_index_begin];
    hdr = ROWAPI_find(table_ptr, long_temp);        /* it MUST be OK */
    if (!hdr) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    cloned_body = (vtss_history_ctrl_entry_t *) hdr->tmp;
    body = (vtss_history_ctrl_entry_t *) hdr->body;

    if (!body || !cloned_body) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    cloned_body->id = entry->id;
    memcpy(&cloned_body->data_source, &entry->data_source, sizeof(entry->data_source));
    cloned_body->interval = entry->interval;
    cloned_body->scrlr.data_requested = entry->scrlr.data_requested;

    hdr->new_status = RMON1_ENTRY_UNDER_CREATION;

    rc =  ROWAPI_commit_status(name,
                               tbl_first_index_begin,
                               &prev_action,
                               table_ptr,
                               entry_size);

    if (rc) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   RESERVE1, &prev_action,
                                   table_ptr,
                                   entry_size);

    hdr->new_status = RMON1_ENTRY_VALID;

    rc =  ROWAPI_commit_status(name,
                               tbl_first_index_begin,
                               &prev_action,
                               table_ptr,
                               entry_size);

    if (rc) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

exit_crit_section:
    return rc;
}

mesa_rc vtss_appl_rmon_history_entry_add(u32 index, const vtss_appl_history_entry_t *const entry)
{
    vtss_history_ctrl_entry_t   conf;
    u32                         i;
    oid                         if_entry_index_oid[] = IF_ENTRY_INDEX_OID;
    vtss_ifindex_elm_t          ife;

    if (vtss_ifindex_decompose(entry->ifIndex, &ife) != VTSS_RC_OK ||
        (ife.iftype != VTSS_IFINDEX_TYPE_PORT &&
         ife.iftype != VTSS_IFINDEX_TYPE_LLAG &&
         ife.iftype != VTSS_IFINDEX_TYPE_GLAG)) {
        T_D("%% Fail: the corresponding ifindex dosen't exist\n");
        return VTSS_RC_ERROR;
    }

    memset(&conf, 0x0, sizeof(conf));
    conf.id = index;

    conf.data_source.length = 11;
    for ( i = 0; i < 10; i++ ) {
        conf.data_source.objid[i] = if_entry_index_oid[i];
    }
    conf.data_source.objid[10] = (oid)(vtss_ifindex_cast_to_u32(entry->ifIndex));

    conf.scrlr.data_requested = entry->data_requested;

    conf.interval = entry->interval;

    return rmon_mgmt_history_entry_add(&conf);
}

mesa_rc rmon_mgmt_history_entry_add(vtss_history_ctrl_entry_t *entry)
{
    mesa_rc                     rc = RMON_ERROR_GEN;
    TABLE_DEFINTION_T           *table_ptr = rmonTable_find(RMON_HISTORY_TABLE_INDEX);
    vtss_history_ctrl_entry_t   exist_entry;

    /* Check input parameter */
    if (!entry) {
        return RMON_ERROR_GEN;
    }

    if (entry->interval < MIN_historyControlInterval ||
        entry->interval > MAX_historyControlInterval ||
        entry->scrlr.data_requested < 1              ||
        entry->scrlr.data_requested > RMON_BUCKET_CNT_MAX) {
        return RMON_ERROR_INVALID_PARAM;
    }

    /* Delete it first if the entry data source is changed. */
    if (rmon_mgmt_history_entry_get(&exist_entry, FALSE) == VTSS_RC_OK &&
        memcmp(&exist_entry.data_source, &entry->data_source, sizeof(exist_entry.data_source))) {
        (void)rmon_mgmt_history_entry_del(&exist_entry);
    }

    /* Create or update entry */
    if (table_ptr) {
        RMON_CRIT_ENTER(table_ptr);
        rc = RMON_history_entry_add(table_ptr, entry);
        RMON_CRIT_EXIT(table_ptr);
    }

    return rc;
}


mesa_rc vtss_appl_rmon_history_entry_del(u32 index)
{
    vtss_history_ctrl_entry_t entry;
    entry.id = index;

    return rmon_mgmt_history_entry_del(&entry);
}

mesa_rc rmon_mgmt_history_entry_del ( vtss_history_ctrl_entry_t *entry )
{
    mesa_rc rc = RMON_ERROR_GEN;
    RMON_ENTRY_T           *hdr = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_HISTORY_TABLE_INDEX);
    if (!entry) {
        return rc;
    }

    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);
    hdr = ROWAPI_find(table_ptr, entry->id);        /* it MUST be OK */
    if (!hdr) {
        rc = RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
        goto exit_crit_section;
    }
    hdr->new_status = RMON1_ENTRY_INVALID;
    rc = ROWAPI_commit(table_ptr, entry->id);
    if (rc) {
        rc = RMON_ERROR_GEN;
    }

exit_crit_section:
    RMON_CRIT_EXIT(table_ptr);
    return rc;

}

mesa_rc vtss_appl_rmon_history_stats_get(u32 index, u32 data_index, vtss_appl_history_statistics_t *const entry)
{
    mesa_rc                     rc = VTSS_RC_ERROR;
    vtss_history_ctrl_entry_t   conf;
    vtss_history_data_entry_t   data;

    memset(&conf, 0, sizeof(vtss_history_ctrl_entry_t));

    conf.id = index;
    if ((rc = rmon_mgmt_history_entry_get(&conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    memset(&data, 0, sizeof(vtss_history_data_entry_t));
    data.data_index = data_index;

    if ((rc = rmon_mgmt_history_data_get(conf.id, &data, FALSE)) == VTSS_RC_OK) {
        entry->EthData = data.EthData;
        entry->utilization = data.utilization;
        entry->start_interval = data.start_interval / 100;
    }
    return rc;
}

mesa_rc vtss_appl_rmon_history_status_get(u32 index, vtss_appl_history_status_t *const entry)
{
    mesa_rc                     rc = VTSS_RC_ERROR;
    vtss_history_ctrl_entry_t   conf;

    memset(&conf, 0, sizeof(vtss_history_ctrl_entry_t));

    conf.id = index;
    if ((rc = rmon_mgmt_history_entry_get(&conf, FALSE)) != VTSS_RC_OK) {
        return rc;
    }
    entry->data_granted   = conf.scrlr.data_granted;
    return rc;
}

mesa_rc vtss_appl_rmon_history_entry_get(u32 index, vtss_appl_history_entry_t *const entry)
{
    vtss_history_ctrl_entry_t   conf;

    memset(&conf, 0, sizeof(vtss_history_ctrl_entry_t));

    conf.id = index;
    if (rmon_mgmt_history_entry_get(&conf, FALSE) == VTSS_RC_OK) {
        entry->ifIndex      = vtss_ifindex_cast_from_u32_0(conf.data_source.objid[10]);
        entry->interval = conf.interval;
        entry->data_requested = conf.scrlr.data_requested;
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc rmon_mgmt_history_entry_get ( vtss_history_ctrl_entry_t *entry, BOOL next )
{
    mesa_rc rc = RMON_ERROR_GEN;
    RMON_ENTRY_T *hdr = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_HISTORY_TABLE_INDEX);
    if (!entry) {
        return rc;
    }
    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);
    hdr = (next == TRUE ? ROWAPI_next(table_ptr, entry->id) : ROWAPI_find(table_ptr, entry->id));

    if (hdr) {
        memcpy(entry, hdr->body, sizeof(vtss_history_ctrl_entry_t));
        RMON_CRIT_EXIT(table_ptr);
        return VTSS_RC_OK;
    }
    RMON_CRIT_EXIT(table_ptr);
    return RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
}

static SCROLLER_T *
vtss_history_extract_scroller(void *v_body)
{
    vtss_history_ctrl_entry_t   *body = (vtss_history_ctrl_entry_t *) v_body;
    return &body->scrlr;
}

#if 1
mesa_rc rmon_mgmt_history_data_get ( ulong ctrl_index, vtss_history_data_entry_t *entry, BOOL next )
{
    RMON_ENTRY_T *hdr = NULL;
    int first_index_begin = 0;
    oid name[] = {1, 3, 6, 1, 2, 1, 16, 2, 2, 1, 1, 0, 0};
    size_t length = sizeof(name) / sizeof(oid);
    first_index_begin = length - 2;
    name[first_index_begin] = ctrl_index;
    name[first_index_begin + 1] = entry->data_index;

    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_HISTORY_TABLE_INDEX);
    RMON_CRIT_ENTER(table_ptr);
    hdr = ROWDATAAPI_header_DataEntry(NULL, name,
                                      &length, !next,
                                      NULL,
                                      table_ptr,
                                      vtss_history_extract_scroller,
                                      sizeof(vtss_history_data_entry_t), entry);
    RMON_CRIT_EXIT(table_ptr);
    return hdr && name[first_index_begin] == ctrl_index ? 0 : RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;

}
#else

mesa_rc rmon_mgmt_history_data_get ( vtss_history_data_entry_t *entry, BOOL next )
{
    mesa_rc rc = RMON_ERROR_GEN;
    RMON_ENTRY_T *hdr = NULL;
    SCROLLER_T     *scrlr = NULL;
    NEXTED_PTR_T   *bptr = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_HISTORY_TABLE_INDEX);
    int i = 0;
    if (!entry) {
        return rc;
    }

    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);

    hdr = ROWAPI_find(table_ptr, entry->ctrl_index);

    if (hdr) {
        scrlr = &((vtss_history_ctrl_entry_t *)hdr->body)->scrlr;
        bptr = scrlr->first_data_ptr;
        printf("find ctrl_index = %ld, tatol_num = %ld\n", ((vtss_history_ctrl_entry_t *)hdr->body)->id, scrlr->data_total_number);

        if (next) {
            for (i = 0; i < scrlr->data_stored && bptr;
                 i++, bptr = bptr->next) {
                printf( "for loop: data_index = %ld, data_stored = %ld, entry_index = %ld, next = 0x%p\n",
                        bptr->data_index, scrlr->data_stored, entry->data_index, bptr->next );

                if (bptr->data_index && (long)bptr->data_index > entry->data_index) {
                    rc = 0;
                    break;
                }
            }

            if (bptr && (long)bptr->data_index <= entry->data_index) {
                rc = RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
                bptr = NULL;
            }

        } else {
            /*  there no data entry in the specific cotrol index    */
            if (hdr->ctrl_index != entry->ctrl_index) {
                rc = RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
                bptr = NULL;
                goto exit_crit_section;
            }

            for (i = 0; i < scrlr->data_stored && bptr;
                 i++, bptr = bptr->next) {
                if ((long)bptr->data_index == entry->data_index) {
                    rc = 0;
                    break;
                }
            }

            if (!bptr) {
                rc = RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
            }
        }
    } else {
        rc = RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
        goto exit_crit_section;

    }

    if (bptr) {
        memcpy(entry, bptr, sizeof(vtss_history_data_entry_t));
    }
exit_crit_section:
    RMON_CRIT_EXIT(table_ptr);
    return rc;

}
#endif

/* Local function to create or update a new RMON alarm entry
   Must under semaphore protection when calling the function. */
static mesa_rc RMON_alarm_entry_add(TABLE_DEFINTION_T *table_ptr, vtss_alarm_ctrl_entry_t *entry)
{
    mesa_rc rc = RMON_ERROR_GEN;
    int tbl_first_index_begin = 0;
    long                   long_temp = 0;
    RMON_ENTRY_T           *hdr = NULL;
    vtss_alarm_ctrl_entry_t      *cloned_body = NULL;
    vtss_alarm_ctrl_entry_t      *body = NULL;
    oid name[] = {1, 3, 6, 1, 2, 1, 16, 3, 1, 1, 1, 0};
    size_t entry_size = sizeof(vtss_alarm_ctrl_entry_t);
    int prev_action = COMMIT;

    tbl_first_index_begin = sizeof(name) / sizeof(oid) - 1;
    name[tbl_first_index_begin] = entry->id;

    if (table_ptr->current_number_of_entries >=
        table_ptr->max_number_of_entries) {
        rc = RMON_ERROR_HISTORY_TABLE_FULL;
        goto exit_crit_section;
    }

    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   RESERVE1, &prev_action,
                                   table_ptr,
                                   entry_size);
    /*
     * get values from PDU, check them and save them in the cloned entry
     */
    long_temp = name[tbl_first_index_begin];
    hdr = ROWAPI_find(table_ptr, long_temp);        /* it MUST be OK */
    if (!hdr) {
        T_E("not found\n");
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    cloned_body = (vtss_alarm_ctrl_entry_t *) hdr->tmp;
    body = (vtss_alarm_ctrl_entry_t *) hdr->body;

    if (!body || !cloned_body) {
        T_E("body or cloned_body NULL\n");
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    memcpy (cloned_body, entry, sizeof(vtss_alarm_ctrl_entry_t));

    hdr->new_status = RMON1_ENTRY_UNDER_CREATION;
    rc =  ROWAPI_commit_status(name,
                               tbl_first_index_begin,
                               &prev_action,
                               table_ptr,
                               entry_size);
    if (rc) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   RESERVE1, &prev_action,
                                   table_ptr,
                                   entry_size);

    hdr->new_status = RMON1_ENTRY_VALID;
    rc =  ROWAPI_commit_status(name,
                               tbl_first_index_begin,
                               &prev_action,
                               table_ptr,
                               entry_size);
    if (rc) {
        rc = RMON_ERROR_GEN;
    }

exit_crit_section:
    return rc;
}

mesa_rc vtss_appl_rmon_alarm_entry_add(u32 index, const vtss_appl_alarm_ctrl_entry_t *const entry)
{
    vtss_alarm_ctrl_entry_t     conf;
    iftable_info_t              table_info;
    u32                         i;
    oid                         if_entry_oid[] = IF_ENTRY_INST;

    table_info.ifIndex = vtss_ifindex_cast_to_u32(entry->ifIndex);

    if ( ! ifIndex_get_valid(&table_info) ) {
        T_E("%% Fail: ifindex doesn't exist\n");
        return VTSS_RC_ERROR;
    }

    if_entry_oid[9] = entry->var_name;
    if_entry_oid[10] = vtss_ifindex_cast_to_u32(entry->ifIndex);


    memset(&conf, 0x0, sizeof(conf));
    conf.id = index;

    (void)rmon_mgmt_alarm_entry_get(&conf, FALSE);

    conf.var_name.length = 11;
    for (i = 0; i < 11; i ++) {
        conf.var_name.objid[i] = if_entry_oid[i];
    }

    if (entry->rising_threshold <= entry->falling_threshold) {
        T_D("%% Invalid: rising threshold must be larger than falling threshold\n");
        return VTSS_RC_ERROR;
    }

    conf.sample_type  = entry->sample_type;
    conf.startup_type = entry->startup_type;
    conf.interval     = entry->interval;

    conf.rising_threshold    = entry->rising_threshold;
    conf.falling_threshold   = entry->falling_threshold;
    conf.rising_event_index  = entry->rising_event_index;
    conf.falling_event_index = entry->falling_event_index;

    return rmon_mgmt_alarm_entry_add(&conf);
}

mesa_rc rmon_mgmt_alarm_entry_add(vtss_alarm_ctrl_entry_t *entry)
{
    mesa_rc                 rc = RMON_ERROR_GEN;
    TABLE_DEFINTION_T       *table_ptr = rmonTable_find(RMON_ALARM_TABLE_INDEX);
    vtss_alarm_ctrl_entry_t exist_entry;

    /* Check input parameter */
    if (!entry) {
        return RMON_ERROR_GEN;
    }

    if (entry->interval < 1 ||
        entry->interval > 0x7fffffff ||
        entry->rising_event_index > MAX_alarmEventIndex ||
        entry->rising_event_index < MIN_alarmEventIndex ||
        entry->falling_event_index > MAX_alarmEventIndex ||
        entry->falling_event_index < MIN_alarmEventIndex
       ) {
        return RMON_ERROR_INVALID_PARAM;
    }
    exist_entry.id = entry->id;
    /* Delete it first if the entry data source is changed. */
    if (rmon_mgmt_alarm_entry_get(&exist_entry, FALSE) == VTSS_RC_OK &&
        memcmp(&exist_entry.var_name, &entry->var_name, sizeof(exist_entry.var_name))) {
        (void) rmon_mgmt_alarm_entry_del(&exist_entry);
    }

    /* Create or update entry */
    if (table_ptr) {
        RMON_CRIT_ENTER(table_ptr);
        rc = RMON_alarm_entry_add(table_ptr, entry);
        RMON_CRIT_EXIT(table_ptr);
    }

    return rc;
}

mesa_rc vtss_appl_rmon_alarm_entry_del(u32 index)
{
    vtss_alarm_ctrl_entry_t entry;
    entry.id = index;

    return rmon_mgmt_alarm_entry_del(&entry);
}

mesa_rc rmon_mgmt_alarm_entry_del ( vtss_alarm_ctrl_entry_t *entry )
{
    mesa_rc rc = RMON_ERROR_GEN;
    RMON_ENTRY_T           *hdr = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_ALARM_TABLE_INDEX);
    if (!entry) {
        return rc;
    }

    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);
    hdr = ROWAPI_find(table_ptr, entry->id);        /* it MUST be OK */
    if (!hdr) {
        rc = RMON_ERROR_ALARM_ENTRY_NOT_FOUND;
        goto exit_crit_section;
    }
    hdr->new_status = RMON1_ENTRY_INVALID;
    rc = ROWAPI_commit(table_ptr, entry->id);
    if (rc) {
        rc = RMON_ERROR_GEN;
    }

exit_crit_section:
    RMON_CRIT_EXIT(table_ptr);
    return rc;

}

mesa_rc vtss_appl_rmon_alarm_status_get(u32 index, vtss_appl_alarm_value_t *const value)
{
    vtss_alarm_ctrl_entry_t tmp_entry;

    memset(&tmp_entry, 0, sizeof(vtss_alarm_ctrl_entry_t));

    tmp_entry.id = index;

    if (rmon_mgmt_alarm_entry_get(&tmp_entry, FALSE) == VTSS_RC_OK) {
        value->value = tmp_entry.value;
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_rmon_alarm_entry_get(u32 index, vtss_appl_alarm_ctrl_entry_t *const entry)
{
    vtss_alarm_ctrl_entry_t tmp_entry;

    memset(&tmp_entry, 0, sizeof(vtss_alarm_ctrl_entry_t));

    tmp_entry.id = index;

    if (rmon_mgmt_alarm_entry_get(&tmp_entry, FALSE) == VTSS_RC_OK) {
        entry->var_name     = (vtss_appl_rmon_alarm_if_t)(tmp_entry.var_name.objid[9]);
        entry->ifIndex      = vtss_ifindex_cast_from_u32_0(tmp_entry.var_name.objid[10]);
        entry->sample_type  = tmp_entry.sample_type;
        entry->startup_type = tmp_entry.startup_type;
        entry->interval     = tmp_entry.interval;
        entry->rising_threshold    = tmp_entry.rising_threshold;
        entry->falling_threshold   = tmp_entry.falling_threshold;
        entry->rising_event_index  = tmp_entry.rising_event_index;
        entry->falling_event_index = tmp_entry.falling_event_index;
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc rmon_mgmt_alarm_entry_get ( vtss_alarm_ctrl_entry_t *entry, BOOL next )
{
    RMON_ENTRY_T *hdr = NULL;
    int first_index_begin = 0;
    oid name[] = {1, 3, 6, 1, 2, 1, 16, 3, 1, 1, 1, 0};
    size_t length = sizeof(name) / sizeof(oid);
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_ALARM_TABLE_INDEX);
    first_index_begin = length - 1;
    name[first_index_begin] = entry->id;

    RMON_CRIT_ENTER(table_ptr);
    hdr = ROWAPI_header_ControlEntry(NULL, name,
                                     &length, !next,
                                     NULL,
                                     table_ptr,
                                     entry, sizeof(vtss_alarm_ctrl_entry_t));
    RMON_CRIT_EXIT(table_ptr);
    return hdr ? 0 : RMON_ERROR_ALARM_ENTRY_NOT_FOUND;

}

mesa_rc vtss_appl_rmon_event_entry_add(u32 index, const vtss_appl_event_entry_t *const entry)
{
    vtss_event_ctrl_entry_t conf;

    memset(&conf, 0x0, sizeof(conf));
    conf.id = index;
    (void)rmon_mgmt_event_entry_get(&conf, FALSE);
    conf.event_type = entry->event_type;

    conf.event_description = VTSS_STRDUP(entry->event_description);
    if (rmon_mgmt_event_entry_add(&conf) != VTSS_RC_OK) {
        T_E("%% Fail to add event entry\n");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

mesa_rc     rmon_mgmt_event_entry_add ( vtss_event_ctrl_entry_t *entry )
{
    mesa_rc rc = RMON_ERROR_GEN;
    int tbl_first_index_begin = 0;
    long                   long_temp = 0;
    RMON_ENTRY_T           *hdr = NULL;
    vtss_event_ctrl_entry_t      *cloned_body = NULL;
    vtss_event_ctrl_entry_t      *body = NULL;
    oid name[] = {1, 3, 6, 1, 2, 1, 16, 9, 1, 1, 1, 0};
    size_t entry_size = sizeof(vtss_event_ctrl_entry_t);
    int prev_action = COMMIT;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_EVENT_TABLE_INDEX);

    if (!entry) {
        return rc;
    }
    tbl_first_index_begin = sizeof(name) / sizeof(oid) - 1;
    name[tbl_first_index_begin] = entry->id;

    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);

    if (table_ptr->current_number_of_entries >=
        table_ptr->max_number_of_entries) {
        rc = RMON_ERROR_HISTORY_TABLE_FULL;
        goto exit_crit_section;
    }

    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   RESERVE1, &prev_action,
                                   table_ptr,
                                   entry_size);
    /*
     * get values from PDU, check them and save them in the cloned entry
     */

    long_temp = name[tbl_first_index_begin];
    hdr = ROWAPI_find(table_ptr, long_temp);        /* it MUST be OK */
    if (!hdr) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    cloned_body = (vtss_event_ctrl_entry_t *) hdr->tmp;
    body = (vtss_event_ctrl_entry_t *) hdr->body;

    if (!body || !cloned_body) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

    cloned_body->id = entry->id;
    cloned_body->event_description = entry->event_description ? VTSS_STRDUP(entry->event_description) : NULL;
    cloned_body->event_type = entry->event_type;

    if (entry->event_description != body->event_description) {
        VTSS_FREE(entry->event_description);
        entry->event_description = NULL;
    }

    hdr->new_status = RMON1_ENTRY_UNDER_CREATION;

    rc =  ROWAPI_commit_status(name,
                               tbl_first_index_begin,
                               &prev_action,
                               table_ptr,
                               entry_size);

    if (rc) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }
    rc =  ROWAPI_do_another_action(name,
                                   tbl_first_index_begin,
                                   RESERVE1, &prev_action,
                                   table_ptr,
                                   entry_size);

    hdr->new_status = RMON1_ENTRY_VALID;

    rc =  ROWAPI_commit_status(name,
                               tbl_first_index_begin,
                               &prev_action,
                               table_ptr,
                               entry_size);

    if (rc) {
        rc = RMON_ERROR_GEN;
        goto exit_crit_section;
    }

exit_crit_section:
    RMON_CRIT_EXIT(table_ptr);
    return rc;

}

mesa_rc vtss_appl_rmon_event_entry_del(u32 index)
{
    vtss_event_ctrl_entry_t entry;
    entry.id = index;

    return rmon_mgmt_event_entry_del(&entry);
}

mesa_rc     rmon_mgmt_event_entry_del ( vtss_event_ctrl_entry_t *entry )
{
    mesa_rc rc = RMON_ERROR_GEN;
    RMON_ENTRY_T           *hdr = NULL;
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_EVENT_TABLE_INDEX);
    if (!entry) {
        return rc;
    }

    if (!table_ptr) {
        return rc;
    }

    RMON_CRIT_ENTER(table_ptr);
    hdr = ROWAPI_find(table_ptr, entry->id);        /* it MUST be OK */
    if (!hdr) {
        rc = RMON_ERROR_EVENT_ENTRY_NOT_FOUND;
        goto exit_crit_section;
    }
    hdr->new_status = RMON1_ENTRY_INVALID;
    rc = ROWAPI_commit(table_ptr, entry->id);
    if (rc) {
        rc = RMON_ERROR_GEN;
    }

exit_crit_section:
    RMON_CRIT_EXIT(table_ptr);
    return rc;

}
mesa_rc vtss_appl_rmon_event_status_last_sent(u32 index, u32 *const event_last_time_sent)
{
    vtss_event_ctrl_entry_t conf;
    vtss_event_data_entry_t data;
    BOOL found = FALSE;

    memset(&conf, 0, sizeof(vtss_event_ctrl_entry_t));
    memset(&data, 0, sizeof(vtss_event_data_entry_t));

    conf.id = index;

    if (rmon_mgmt_event_entry_get(&conf, FALSE) == VTSS_RC_OK) {

        while (rmon_mgmt_event_data_get(conf.id, &data, TRUE) == VTSS_RC_OK) {
            found = TRUE;
            *event_last_time_sent = conf.event_last_time_sent / 100;
        }

    }
    return (found) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

mesa_rc vtss_appl_rmon_event_status_get(u32 index, u32 data_index, vtss_appl_event_data_t *const entry)
{
    vtss_event_ctrl_entry_t conf;
    vtss_event_data_entry_t data;

    memset(&conf, 0, sizeof(vtss_event_ctrl_entry_t));
    memset(&data, 0, sizeof(vtss_event_data_entry_t));
    memset(entry, 0, sizeof(*entry));

    conf.id = index;
    data.data_index = data_index;

    T_D("idx = %du, data_idx = %lu", conf.id, data.data_index);

    if (rmon_mgmt_event_entry_get(&conf, FALSE) == VTSS_RC_OK) {

        T_D("rmon_mgmt_event_data_get:idx = %du, data_idx = %lu", conf.id, data.data_index);
        if (rmon_mgmt_event_data_get(conf.id, &data, FALSE) == VTSS_RC_OK) {
            entry->log_time = data.log_time / 100;

            if (strlen(data.log_description)) {
                memcpy(entry->log_description, data.log_description, strlen(data.log_description));
            }
            T_D("VTSS_RC_OK");
            return VTSS_RC_OK;
        }
    }
    T_D("VTSS_RC_ERROR");
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_rmon_event_entry_get(u32 index, vtss_appl_event_entry_t *const entry)
{
    vtss_event_ctrl_entry_t conf;

    memset(&conf, 0x0, sizeof(conf));
    memset(entry, 0, sizeof(*entry));

    conf.id = index;

    if (rmon_mgmt_event_entry_get(&conf, FALSE) == VTSS_RC_OK) {
        entry->event_type = conf.event_type;
        if (conf.event_description) {
            strncpy(entry->event_description, conf.event_description, VTSS_APPL_RMON_LOG_STRING_LENGTH - 1);
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc         rmon_mgmt_event_entry_get ( vtss_event_ctrl_entry_t *entry, BOOL next )
{
    RMON_ENTRY_T *hdr = NULL;
    int first_index_begin = 0;
    oid name[] = {1, 3, 6, 1, 2, 1, 16, 9, 1, 1, 1, 0};
    size_t length = sizeof(name) / sizeof(oid);
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_EVENT_TABLE_INDEX);
    first_index_begin = length - 1;
    name[first_index_begin] = entry->id;

    RMON_CRIT_ENTER(table_ptr);
    hdr = ROWAPI_header_ControlEntry(NULL, name,
                                     &length, !next,
                                     NULL,
                                     table_ptr,
                                     entry, sizeof(vtss_event_ctrl_entry_t));
    RMON_CRIT_EXIT(table_ptr);
    return hdr ? 0 : RMON_ERROR_EVENT_ENTRY_NOT_FOUND;

}

static SCROLLER_T *
vtss_event_extract_scroller(void *v_body)
{
    vtss_event_ctrl_entry_t   *body = (vtss_event_ctrl_entry_t *) v_body;
    return &body->scrlr;
}

mesa_rc rmon_mgmt_event_data_get ( ulong ctrl_index, vtss_event_data_entry_t *entry, BOOL next )
{
    RMON_ENTRY_T *hdr = NULL;
    int first_index_begin = 0;
    oid name[] = {1, 3, 6, 1, 2, 1, 16, 9, 2, 1, 1, 0, 0};
    size_t length = sizeof(name) / sizeof(oid);
    TABLE_DEFINTION_T *table_ptr = rmonTable_find(RMON_EVENT_TABLE_INDEX);
    first_index_begin = length - 2;
    name[first_index_begin] = ctrl_index;
    name[first_index_begin + 1] = entry->data_index;

    RMON_CRIT_ENTER(table_ptr);
    hdr = ROWDATAAPI_header_DataEntry(NULL, name,
                                      &length, !next,
                                      NULL,
                                      table_ptr,
                                      vtss_event_extract_scroller,
                                      sizeof(vtss_event_data_entry_t), entry);
    RMON_CRIT_EXIT(table_ptr);
    return (hdr && name[first_index_begin] == ctrl_index) ? 0 : RMON_ERROR_EVENT_ENTRY_NOT_FOUND;

}


/* Determine if SNMP RMON statistics entry has changed */
static int rmon_stat_entry_changed(rmon_stat_entry_t *old, rmon_stat_entry_t *new_)
{
    return (new_->valid != old->valid ||
            new_->ctrl_index != old->ctrl_index ||
            new_->if_index != old->if_index);
}

/* Determine if SNMP RMON history entry has changed */
static int rmon_history_entry_changed(rmon_history_entry_t *old, rmon_history_entry_t *new_)
{
    return (new_->valid != old->valid ||
            new_->ctrl_index != old->ctrl_index ||
            new_->if_index != old->if_index ||
            new_->interval != old->interval ||
            new_->requested != old->requested);
}

/* Determine if SNMP RMON alarm entry has changed */
static int rmon_alarm_entry_changed(rmon_alarm_entry_t *old, rmon_alarm_entry_t *new_)
{
    return (new_->valid != old->valid ||
            new_->ctrl_index != old->ctrl_index ||
            new_->interval != old->interval ||
            memcmp(new_->variable, old->variable, new_->variable_len > old->variable_len ? (sizeof(ulong) * new_->variable_len) : (sizeof(ulong) * old->variable_len)) ||
            new_->sample_type != old->sample_type ||
            new_->rising_threshold != old->rising_threshold ||
            new_->falling_threshold != old->falling_threshold ||
            new_->rising_event_index != old->rising_event_index ||
            new_->falling_event_index != old->falling_event_index);
}

/* Determine if SNMP RMON evnet entry has changed */
static int rmon_evnet_entry_changed(rmon_event_entry_t *old, rmon_event_entry_t *new_)
{
    return (new_->valid != old->valid ||
            new_->ctrl_index != old->ctrl_index ||
            strcmp(new_->description, old->description) ||
            new_->type != old->type);
}

/* Get SNMP RMON statistics row entry */
mesa_rc snmp_mgmt_rmon_stat_entry_get(rmon_stat_entry_t *entry, BOOL next)
{
    ulong i, num, found = 0;

    T_D("enter");
    RMON_GLOBAL_CRIT_ENTER();
    for (i = 0, num = 0;
         i < RMON_STAT_MAX_ROW_SIZE && num < rmon_global.rmon_stat_entry_num;
         i++) {
        if (!rmon_global.rmon_stat_entry[i].valid) {
            continue;
        }
        num++;
        if (entry->ctrl_index == 0 && next) {
            *entry = rmon_global.rmon_stat_entry[i];
            found = 1;
            break;
        }
        if (rmon_global.rmon_stat_entry[i].ctrl_index == entry->ctrl_index) {
            if (next) {
                if (num == rmon_global.rmon_stat_entry_num) {
                    break;
                }
                i++;
                while (i < RMON_STAT_MAX_ROW_SIZE ) {
                    if (rmon_global.rmon_stat_entry[i].valid) {
                        *entry = rmon_global.rmon_stat_entry[i];
                        found = 1;
                        break;
                    }
                    i++;
                }
                break;
            } else {
                *entry = rmon_global.rmon_stat_entry[i];
                found = 1;
            }
            break;
        }
    }
    RMON_GLOBAL_CRIT_EXIT();
    T_D("exit");

    if (found) {
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

/* Set SNMP RMON statistics row entry */
mesa_rc snmp_mgmt_rmon_stat_entry_set(rmon_stat_entry_t *entry)
{
    mesa_rc              rc = VTSS_RC_OK;
    int                  changed = 0, found_flag = 0;
    ulong                i = 0, num;

    T_D("enter");
    RMON_GLOBAL_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        for (i = 0, num = 0;
             i < RMON_STAT_MAX_ROW_SIZE && num < rmon_global.rmon_stat_entry_num;
             i++) {
            if (!rmon_global.rmon_stat_entry[i].valid) {
                continue;
            }
            num++;
            if (rmon_global.rmon_stat_entry[i].ctrl_index == entry->ctrl_index) {
                found_flag = 1;
                break;
            }
        }

        if (i < RMON_STAT_MAX_ROW_SIZE && found_flag) {
            changed = rmon_stat_entry_changed(&rmon_global.rmon_stat_entry[i], entry);
            if (!entry->valid) {
                rmon_global.rmon_stat_entry_num--;
            }
            if (changed) {
                rmon_global.rmon_stat_entry[i] = *entry;
            }
        } else if (entry->valid) {
            /* add new entry */
            for (i = 0; i < RMON_STAT_MAX_ROW_SIZE; i++) {
                if (rmon_global.rmon_stat_entry[i].valid) {
                    continue;
                }
                rmon_global.rmon_stat_entry_num++;
                rmon_global.rmon_stat_entry[i] = *entry;
                break;
            }
            if (i == RMON_STAT_MAX_ROW_SIZE) {
                rc = (mesa_rc) RMON_ERROR_STAT_TABLE_FULL;
            }
        }
    } else {
        T_W("not primary switch");
        rc = (mesa_rc) RMON_ERROR_STACK_STATE;
    }
    RMON_GLOBAL_CRIT_EXIT();

    T_D("exit, rc = %d, entry = " VPRIlu", rmon_stat_entry_num = %lu", rc, i, rmon_global.rmon_stat_entry_num);

    return rc;
}

/* Get SNMP RMON history row entry */
mesa_rc snmp_mgmt_rmon_history_entry_get(rmon_history_entry_t *entry, BOOL next)
{
    ulong i, num, found = 0;

    RMON_GLOBAL_CRIT_ENTER();
    for (i = 0, num = 0;
         i < RMON_HISTORY_MAX_ROW_SIZE && num < rmon_global.rmon_history_entry_num;
         i++) {
        if (!rmon_global.rmon_history_entry[i].valid) {
            continue;
        }
        num++;
        if (entry->ctrl_index == 0 && next) {
            *entry = rmon_global.rmon_history_entry[i];
            found = 1;
            break;
        }
        if (rmon_global.rmon_history_entry[i].ctrl_index == entry->ctrl_index) {
            if (next) {
                if (num == rmon_global.rmon_history_entry_num) {
                    break;
                }
                i++;
                while (i < RMON_HISTORY_MAX_ROW_SIZE) {
                    if (rmon_global.rmon_history_entry[i].valid) {
                        *entry = rmon_global.rmon_history_entry[i];
                        found = 1;
                        break;
                    }
                    i++;
                }
                break;
            } else {
                *entry = rmon_global.rmon_history_entry[i];
                found = 1;
            }
            break;
        }
    }
    RMON_GLOBAL_CRIT_EXIT();
    if (found) {
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

/* Set SNMP RMON history row entry */
mesa_rc snmp_mgmt_rmon_history_entry_set(rmon_history_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    int                     changed = 0, found_flag = 0;
    ulong                   i, num;

    T_D("enter");
    RMON_GLOBAL_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        for (i = 0, num = 0;
             i < RMON_HISTORY_MAX_ROW_SIZE && num < rmon_global.rmon_history_entry_num;
             i++) {
            if (!rmon_global.rmon_history_entry[i].valid) {
                continue;
            }
            num++;
            if (rmon_global.rmon_history_entry[i].ctrl_index == entry->ctrl_index) {
                found_flag = 1;
                break;
            }
        }

        if (i < RMON_HISTORY_MAX_ROW_SIZE && found_flag) {
            changed = rmon_history_entry_changed(&rmon_global.rmon_history_entry[i], entry);
            if (!entry->valid) {
                rmon_global.rmon_history_entry_num--;
            }
            if (changed) {
                rmon_global.rmon_history_entry[i] = *entry;
            }
        } else if (entry->valid) {
            /* add new entry */
            for (i = 0; i < RMON_HISTORY_MAX_ROW_SIZE; i++) {
                if (rmon_global.rmon_history_entry[i].valid) {
                    continue;
                }
                rmon_global.rmon_history_entry_num++;
                rmon_global.rmon_history_entry[i] = *entry;
                break;
            }
            if (i == RMON_HISTORY_MAX_ROW_SIZE) {
                rc = (mesa_rc) RMON_ERROR_HISTORY_TABLE_FULL;
            }
        }
    } else {
        T_W("not primary switch");
        rc = (mesa_rc) RMON_ERROR_STACK_STATE;
    }
    RMON_GLOBAL_CRIT_EXIT();

    T_D("exit");

    return rc;
}

/* Get SNMP RMON alarm row entry */
mesa_rc snmp_mgmt_rmon_alarm_entry_get(rmon_alarm_entry_t *entry, BOOL next)
{
    ulong i, num, found = 0;

    RMON_GLOBAL_CRIT_ENTER();
    for (i = 0, num = 0;
         i < RMON_ALARM_MAX_ROW_SIZE && num < rmon_global.rmon_alarm_entry_num;
         i++) {
        if (!rmon_global.rmon_alarm_entry[i].valid) {
            continue;
        }
        num++;
        if (entry->ctrl_index == 0 && next) {
            *entry = rmon_global.rmon_alarm_entry[i];
            found = 1;
            break;
        }
        if (rmon_global.rmon_alarm_entry[i].ctrl_index == entry->ctrl_index) {
            if (next) {
                if (num == rmon_global.rmon_alarm_entry_num) {
                    break;
                }
                i++;
                while (i < RMON_ALARM_MAX_ROW_SIZE) {
                    if (rmon_global.rmon_alarm_entry[i].valid) {
                        *entry = rmon_global.rmon_alarm_entry[i];
                        found = 1;
                        break;
                    }
                    i++;
                }
                break;
            } else {
                *entry = rmon_global.rmon_alarm_entry[i];
                found = 1;
            }
            break;
        }
    }
    RMON_GLOBAL_CRIT_EXIT();

    if (found) {
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

/* Set SNMP RMON alarm row entry */
mesa_rc snmp_mgmt_rmon_alarm_entry_set(rmon_alarm_entry_t *entry)
{
    mesa_rc                 rc = VTSS_RC_OK;
    int                     changed = 0, found_flag = 0;
    ulong                   i, num;

    RMON_GLOBAL_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        for (i = 0, num = 0;
             i < RMON_ALARM_MAX_ROW_SIZE && num < rmon_global.rmon_alarm_entry_num;
             i++) {
            if (!rmon_global.rmon_alarm_entry[i].valid) {
                continue;
            }
            num++;
            if (rmon_global.rmon_alarm_entry[i].ctrl_index == entry->ctrl_index) {
                found_flag = 1;
                break;
            }
        }

        if (i < RMON_ALARM_MAX_ROW_SIZE && found_flag) {
            changed = rmon_alarm_entry_changed(&rmon_global.rmon_alarm_entry[i], entry);
            if (!entry->valid) {
                rmon_global.rmon_alarm_entry_num--;
            }
            if (changed) {
                rmon_global.rmon_alarm_entry[i] = *entry;
            }
        } else if (entry->valid) {
            /* add new entry */
            for (i = 0; i < RMON_ALARM_MAX_ROW_SIZE; i++) {
                if (rmon_global.rmon_alarm_entry[i].valid) {
                    continue;
                }
                rmon_global.rmon_alarm_entry_num++;
                rmon_global.rmon_alarm_entry[i] = *entry;
                break;
            }
            if (i == RMON_ALARM_MAX_ROW_SIZE) {
                rc = (mesa_rc) RMON_ERROR_ALARM_TABLE_FULL;
            }
        }
    } else {
        T_W("not primary switch");
        rc = (mesa_rc) RMON_ERROR_STACK_STATE;
    }
    RMON_GLOBAL_CRIT_EXIT();

    return rc;
}

/* Get SNMP RMON event row entry */
mesa_rc snmp_mgmt_rmon_event_entry_get(rmon_event_entry_t *entry, BOOL next)
{
    ulong i, num, found = 0;

    RMON_GLOBAL_CRIT_ENTER();
    for (i = 0, num = 0;
         i < RMON_EVENT_MAX_ROW_SIZE && num < rmon_global.rmon_event_entry_num;
         i++) {
        if (!rmon_global.rmon_event_entry[i].valid) {
            continue;
        }
        num++;
        if (entry->ctrl_index == 0 && next) {
            *entry = rmon_global.rmon_event_entry[i];
            found = 1;
            break;
        }
        if (rmon_global.rmon_event_entry[i].ctrl_index == entry->ctrl_index) {
            if (next) {
                if (num == rmon_global.rmon_event_entry_num) {
                    break;
                }
                i++;
                while (i < RMON_EVENT_MAX_ROW_SIZE) {
                    if (rmon_global.rmon_event_entry[i].valid) {
                        *entry = rmon_global.rmon_event_entry[i];
                        found = 1;
                        break;
                    }
                    i++;
                }
                break;
            } else {
                *entry = rmon_global.rmon_event_entry[i];
                found = 1;
            }
            break;
        }
    }
    RMON_GLOBAL_CRIT_EXIT();

    if (found) {
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

/* Set SNMP RMON event row entry */
mesa_rc snmp_mgmt_rmon_event_entry_set(rmon_event_entry_t *entry)
{
    mesa_rc               rc = VTSS_RC_OK;
    int                   changed = 0, found_flag = 0;
    ulong                 i, num;

    RMON_GLOBAL_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        for (i = 0, num = 0;
             i < RMON_EVENT_MAX_ROW_SIZE && num < rmon_global.rmon_event_entry_num;
             i++) {
            if (!rmon_global.rmon_event_entry[i].valid) {
                continue;
            }
            num++;
            if (rmon_global.rmon_event_entry[i].ctrl_index == entry->ctrl_index) {
                found_flag = 1;
                break;
            }
        }

        if (i < RMON_EVENT_MAX_ROW_SIZE && found_flag) {
            changed = rmon_evnet_entry_changed(&rmon_global.rmon_event_entry[i], entry);
            if (!entry->valid) {
                rmon_global.rmon_event_entry_num--;
            }
            if (changed) {
                rmon_global.rmon_event_entry[i] = *entry;
            }
        } else if (entry->valid) {
            /* add new entry */
            for (i = 0; i < RMON_EVENT_MAX_ROW_SIZE; i++) {
                if (rmon_global.rmon_event_entry[i].valid) {
                    continue;
                }
                rmon_global.rmon_event_entry_num++;
                rmon_global.rmon_event_entry[i] = *entry;
                break;
            }
            if (i == RMON_EVENT_MAX_ROW_SIZE) {
                rc = (mesa_rc) RMON_ERROR_EVENT_TABLE_FULL;
            }
        }
    } else {
        T_W("not primary switch");
        rc = (mesa_rc) RMON_ERROR_STACK_STATE;
    }
    RMON_GLOBAL_CRIT_EXIT();

    return rc;
}





#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
static const char *rmon_msg_id_txt(rmon_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case RMON_MSG_ID_RMON_CONF_SET_REQ:
        txt = "RMON_MSG_ID_RMON_CONF_SET_REQ";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}
#endif /* VTSS_TRACE_LVL_DEBUG */

#if 0
/* Allocate request buffer */
static rmon_msg_req_t *rmon_msg_req_alloc(rmon_msg_buf_t *buf, rmon_msg_id_t msg_id)
{
    rmon_msg_req_t *msg = &rmon_global.request.msg;

    buf->sem = &rmon_global.request.sem;
    buf->msg = msg;
    vtss_sem_wait(buf->sem);
    msg->msg_id = msg_id;
    return msg;
}

/* Free request/reply buffer */
static void rmon_msg_free(vtss_sem_t *sem)
{
    vtss_sem_post(sem);
}

static void rmon_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    rmon_msg_id_t msg_id = *(rmon_msg_id_t *)msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s", msg_id, rmon_msg_id_txt(msg_id));
    rmon_msg_free(contxt);
}

static void rmon_msg_tx(rmon_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    rmon_msg_id_t msg_id = *(rmon_msg_id_t *)buf->msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s, len: %zu, isid: %d", msg_id, rmon_msg_id_txt(msg_id), len, isid);
    msg_tx_adv(buf->sem, rmon_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_MODULE_ID_RMON, isid, buf->msg, len + MSG_TX_DATA_HDR_LEN(rmon_msg_req_t, req));
}
#endif

static BOOL rmon_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    rmon_msg_id_t msg_id = *(rmon_msg_id_t *)rx_msg;

    T_D("msg_id: %d, %s, len: %zu, isid: %u", msg_id, rmon_msg_id_txt(msg_id), len, isid);

    switch (msg_id) {
    case RMON_MSG_ID_RMON_CONF_SET_REQ: {
        break;
    }
    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }
    return TRUE;
}

static mesa_rc rmon_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = rmon_msg_rx;
    filter.modid = VTSS_MODULE_ID_RMON;
    return msg_rx_filter_register(&filter);
}


/* Set stack RMON configuration */
static void rmon_stack_rmon_conf_set(vtss_isid_t isid_add)
{
//    rmon_msg_req_t  *msg;
//    rmon_msg_buf_t  buf;
    vtss_isid_t     isid;

    T_D("enter, isid_add: %d", isid_add);
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if ((isid_add != VTSS_ISID_GLOBAL && isid_add != isid) ||
            !msg_switch_exists(isid)) {
            continue;
        }
#if 0
        RMON_GLOBAL_CRIT_ENTER();
        msg = rmon_msg_req_alloc(&buf, RMON_MSG_ID_RMON_CONF_SET_REQ);
        RMON_GLOBAL_CRIT_EXIT();
        (void)rmon_msg_tx(&buf, isid, sizeof(msg->req.conf_set.conf));
#endif
    }

    T_D("exit, isid_add: %d", isid_add);
}





/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create SNMP switch configuration */
static void rmon_conf_read_switch(vtss_isid_t isid_add)
{

    T_D("enter, isid_add: %d", isid_add);

    T_D("exit");
}

/* Read/create SNMP stack configuration */
static void rmon_conf_read_stack(BOOL create)
{

    T_D("enter, create: %d", create);

    RMON_GLOBAL_CRIT_ENTER();

    /* Use default values */
    rmon_global.rmon_stat_entry_num = 0;
    memset(rmon_global.rmon_stat_entry, 0x0, sizeof(rmon_global.rmon_stat_entry));

    /* Use default values */
    rmon_global.rmon_history_entry_num = 0;
    memset(rmon_global.rmon_history_entry, 0x0, sizeof(rmon_global.rmon_history_entry));

    /* Use default values */
    rmon_global.rmon_alarm_entry_num = 0;
    memset(rmon_global.rmon_alarm_entry, 0x0, sizeof(rmon_global.rmon_alarm_entry));

    /* Use default values */
    rmon_global.rmon_event_entry_num = 0;
    memset(rmon_global.rmon_event_entry, 0x0, sizeof(rmon_global.rmon_event_entry));

    RMON_GLOBAL_CRIT_EXIT();

    T_D("exit");
}

/* Set RMON configuration to engine */
static void rmon_conf_engine_set(void)
{
    /* Set to the snmp engine only when stack role is primary switch */
    if (!msg_switch_is_primary()) {
        T_D("NOT primary switch");
        return;
    }

    rmon_create_stat_default_entry();
    rmon_create_history_default_entry();
    rmon_create_event_default_entry();
    rmon_create_alarm_default_entry();
}

/* Module start */
static void rmon_start(BOOL init)
{
    mesa_rc             rc;

    T_D("enter, init: %d", init);

    if (init) {

        /* Initialize message buffers */
        vtss_sem_init(&rmon_global.request.sem, 1);

        /* Create semaphore for critical regions */
        critd_init(&rmon_global.crit, "rmon", VTSS_MODULE_ID_RMON, CRITD_TYPE_MUTEX);

        /* Initialize SNMP configuration */
        init_rmon_statistics();
        init_rmon_history();
        init_rmon_alarm();
        init_rmon_event();
    } else {
        static vtss::Timer RMON_timer;
        RMON_timer.set_repeat(true);
        RMON_timer.set_period(vtss::milliseconds(1000));
        RMON_timer.callback = run_rmon_timer;
        RMON_timer.modid = VTSS_MODULE_ID_RMON;
        if (vtss_timer_start(&RMON_timer) != VTSS_RC_OK) {
            T_E("vtss_timer_start() failed");
        }

        /* Register for stack messages */
        if ((rc = rmon_stack_register()) != VTSS_RC_OK) {
            T_W("rmon_stack_register(): failed rc = %d", rc);
        }

    }
    T_D("exit");
}

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_rmon_json_init();
#endif
extern "C" int rmon_icli_cmd_register();

/* Initialize module */
mesa_rc rmon_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    mesa_rc     rc = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
#ifndef VTSS_SW_OPTION_SNMP
        if (ifIndex_init()) {
            T_E("RMON SNMP ifIndex initialization failed");
        }
#endif

        rmon_start(1);
#ifdef VTSS_SW_OPTION_ICFG
        rc = rmon_icfg_init();
        if (rc != VTSS_RC_OK) {
            T_D("fail to init icfg registration, rc = %s", error_txt(rc));
        }
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_rmon_json_init();
#endif
        rmon_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");
        rmon_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            rmon_conf_read_stack(1);
            rmon_conf_engine_set();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE: {
        T_D("ICFG_LOADING_PRE");

        /* Read stack and switch configuration */
        rmon_conf_read_stack(0);
        rmon_conf_read_switch(VTSS_ISID_GLOBAL);
        rmon_conf_engine_set();
        break;
    }

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        /* Apply all configuration to switch */
        rmon_stack_rmon_conf_set(isid);
        break;

    default:
        break;
    }

    return rc;
}

#ifdef VTSS_SW_OPTION_AGGR
/**
  * \brief Get the start ifIndex of LLAG.
  *
  * \return
  *    the start ifIndex of LLAG\n
  */

int rmon_mgmt_llag_start_valid_get ( void )
{
    int tmp;
    iftable_info_t info;
    info.type = IFTABLE_IFINDEX_TYPE_PORT;
    /* get the first existent switch ISID */
    if ( FALSE == ifIndex_get_first_by_type( &info )) {
        return -1;
    }
    tmp = (int)info.ifIndex;
    info.type = IFTABLE_IFINDEX_TYPE_LLAG;
    info.if_id = AGGR_MGMT_GROUP_NO_START;

    if ( FALSE == ifIndex_get_by_interface( &info )) {
        return -1;
    }
    tmp = (int)info.ifIndex - tmp + AGGR_MGMT_GROUP_NO_START;
    return tmp;
}

/**
  * \brief Get the start ifIndex of GLAG.
  *
  * \return
  *    the start ifIndex of GLAG\n
  */
int rmon_mgmt_ifIndex_glag_start_valid_get ( void )
{

    iftable_info_t info;
    info.type = IFTABLE_IFINDEX_TYPE_GLAG;
    info.if_id = AGGR_MGMT_GLAG_START_;
    if ( FALSE == ifIndex_get_by_interface( &info )) {
        return -1;
    }

    return ( int ) info.ifIndex;

}
#endif /* VTSS_SW_OPTION_AGGR */

#ifndef VTSS_SW_OPTION_SNMP
/* Convert text (e.g. .1.2.*.3) to OID */
mesa_rc mgmt_txt2oid(char *buf, int len,
                     ulong *oid, uchar *oid_mask, ulong *oid_len)
{
    ulong count = 0;
    int   i, j = 0, c = 0, prev;
    uchar mask = 0;

    for (i = 0; i < len; i++) {
        prev = c;
        c = buf[i];
        if (c == '.') {
            if (prev == '.' || i == (len - 1) || count == 128) {
                return VTSS_UNSPECIFIED_ERROR;
            }
            j = count;
            mask = (1 << (7 - (count % 8)));
            count++;
        } else if (c == '*') {
            if (prev != '.') {
                return VTSS_UNSPECIFIED_ERROR;
            }
            oid_mask[j / 8] &= ~mask;
        } else if (isdigit(c)) {
            if (prev == 0 || prev == '*') {
                return VTSS_UNSPECIFIED_ERROR;
            }
            if (prev == '.') {
                oid[j] = 0;
            }
            oid[j] = (oid[j] * 10 + c - '0');
            oid_mask[j / 8] |= mask;
        } else {
            return VTSS_UNSPECIFIED_ERROR;
        }
    }

    *oid_len = count;

    return (count ? VTSS_RC_OK : VTSS_UNSPECIFIED_ERROR);
}
#endif

void *rmon_callout_malloc(size_t sz)
{
    return VTSS_MALLOC(sz);
}

char *rmon_callout_strdup(const char *str)
{
    return VTSS_STRDUP(str);
}

void rmon_callout_free(void *ptr)
{
    VTSS_FREE(ptr);
}
