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

#include <vtss/appl/types.hxx>
#include "main.h"
#include "misc_api.h"
#include "msg_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "critd_api.h"
#include "qos_api.h"
#include "vtss_vcap_api.h"
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/basics/expose/struct-status.hxx" // For vtss::expose::StructStatus
#include "vtss/basics/memcmp-operator.hxx"      // For VTSS_BASICS_MEMCMP_OPERATOR

#include "vtss_common_iterator.hxx"
#include "topo_api.h"

#if defined(VTSS_SW_OPTION_LLDP)
#include "lldp_api.h"
#endif /* VTSS_SW_OPTION_LLDP */

#if defined(VTSS_SW_OPTION_ICFG)
#include "qos_icfg.h"
#endif

#include "vtss_timer_api.h"      /* For vtss_timer_XXX() functions */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_QOS

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_QOS
#include <vtss_trace_api.h>

/* =================================================================
 *  QCE ID definitions.
 *  We use a 16-bit user ID and a 16-bit QCE ID to identify an QCE.
 *  The VTSS API used a 32-bit QCE ID. These two 16-bit values are
 *  combined when accessing the switch API. This gives each QCL
 *  user a separate ID space used for its own entries.
 * ================================================================= */
#define QCL_USER_ID_GET(qce_id)               ((vtss_appl_qos_qcl_user_t)((qce_id) >> 16))
#define QCL_QCE_ID_GET(qce_id)                ((mesa_qce_id_t)((qce_id) & 0xFFFF))
#define QCL_COMBINED_ID_SET(user_id, qce_id)  ((mesa_qce_id_t)((user_id) << 16) + ((qce_id) & 0xFFFF))

/* QCL entry */
typedef struct qos_qce_t {
    struct qos_qce_t                *next; /* Next in list */
    vtss_appl_qos_qce_intern_conf_t conf;  /* Configuration */
} qcl_qce_t;

typedef struct {
    qcl_qce_t *qce_used_list;     /* a link list for used QCEs */
    qcl_qce_t *qce_free_list;     /* a link list for unused QCEs */
    qcl_qce_t *qce_list;          /* the body of whole QCE table: since the size
                                     of stack and switch qce table are different
                                     we will create the space at sys init time */
} qcl_qce_list_table_t;


/* ================================================================= *
 *  QOS definitions
 * ================================================================= */

#define QOS_PORT_CONF_CHANGE_REG_MAX 2

/* QoS port configuration change registration table */
typedef struct {
    critd_t                    crit;
    int                        count;
    qos_port_conf_change_reg_t reg[QOS_PORT_CONF_CHANGE_REG_MAX];
} qos_change_reg_table_t;

/* QoS QCE frame union */
typedef union {
    vtss_appl_qos_qce_frame_etype_t etype;
    vtss_appl_qos_qce_frame_llc_t   llc;
    vtss_appl_qos_qce_frame_snap_t  snap;
    vtss_appl_qos_qce_frame_ipv4_t  ipv4;
    vtss_appl_qos_qce_frame_ipv6_t  ipv6;
} qos_frame_union_t;


/* JSON notifications  */
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_qos_status_t);
vtss::expose::StructStatus<
    vtss::expose::ParamVal<vtss_appl_qos_status_t *>
> qos_status_update;

#if defined(VTSS_SW_OPTION_QOS_ADV)
/* QoS frame preemption run-time link and LLDP remote info */
typedef struct {
    BOOL link;
    BOOL supported;
    BOOL enabled;
    BOOL active;
    u8   add_frag_size;
} qos_preempt_info_t;

#endif /* VTSS_SW_OPTION_QOS_ADV */

/* ================================================================= *
 *  QOS global structure
 * ================================================================= */

/* QOS capabilities structure and global const pointer */
static vtss_appl_qos_capabilities_t qos_capabilities;
const vtss_appl_qos_capabilities_t * const vtss_appl_qos_capabilities = &qos_capabilities;
#define CAPA vtss_appl_qos_capabilities

/* QOS global structure */
typedef struct {
    critd_t                         qos_crit;
    vtss_appl_qos_conf_t            qos_conf;
    CapArray<vtss_appl_qos_port_conf_t, VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> qos_port_conf; /* Indexed with isid in order to make room for VTSS_ISID_LOCAL as first entry */
    CapArray<mesa_prio_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> volatile_default_prio;
    qcl_qce_list_table_t            qcl_qce_stack_list;
    qcl_qce_list_table_t            qcl_qce_switch_list;
    qos_change_reg_table_t          rt;
    vtss::Timer                     shaper_calibrate_timer;                   /* Timer for shaper calibration */
    vtss::Timer                     status_update_timer;                      /* Timer for updating qos status */
    vtss_appl_qos_status_t          qos_status;
#if defined(VTSS_SW_OPTION_QOS_ADV)
    CapArray<vtss_appl_qos_imap_entry_t, MESA_CAP_QOS_INGRESS_MAP_CNT> qos_imap;
    CapArray<vtss_appl_qos_emap_entry_t, MESA_CAP_QOS_EGRESS_MAP_CNT>  qos_emap;
    CapArray<qos_preempt_info_t,         MEBA_CAP_BOARD_PORT_MAP_COUNT>            preempt_info;
#endif /* VTSS_SW_OPTION_QOS_ADV */
} qos_global_t;

/* Global structure */
static qos_global_t qos;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "QOS", "QoS Control Lists"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define QOS_CRIT_ENTER()         critd_enter(        &qos.qos_crit, __FILE__, __LINE__)
#define QOS_CRIT_EXIT()          critd_exit(         &qos.qos_crit, __FILE__, __LINE__)
#define QOS_CRIT_ASSERT_LOCKED() critd_assert_locked(&qos.qos_crit, __FILE__, __LINE__)
#define QOS_CB_CRIT_ENTER()      critd_enter(        &qos.rt.crit,  __FILE__, __LINE__)
#define QOS_CB_CRIT_EXIT()       critd_exit(         &qos.rt.crit,  __FILE__, __LINE__)

#define QOS_PORT_POLICER_IX             0 /* Use policer #0 as port policer */

#define QOS_STORM_POLICER_UNICAST_IX    1 /* Use policer #1 as storm policer for unicast frames */
#define QOS_STORM_POLICER_BROADCAST_IX  2 /* Use policer #2 as storm policer for broadcast frames */
#define QOS_STORM_POLICER_UNKNOWN_IX    3 /* Use policer #3 as storm policer for unknown frames */

/****************************************************************************/
/*  Forward declarations                                                    */
/****************************************************************************/
static void QOS_stack_conf_set(vtss_isid_t isid_add);
static BOOL QOS_port_isid_invalid(vtss_isid_t isid, mesa_port_no_t port_no, BOOL set);
static void QOS_stack_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no);

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Default mapping of PCP to CoS */
/* Can also be used for default mapping of CoS to PCP */
/* This is the IEEE802.1Q-2011 recommended priority to traffic class mappings */
static u32 QOS_pcp2cos(u32 pcp)
{
    switch (pcp) {
    case 0:
        return 1;
    case 1:
        return 0;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        return pcp;
    default:
        T_E("Invalid PCP (%u)", pcp);
        return 0;
    }
}

/*
 * Calculate the actual weight in percent.
 * This calculation includes the round off errors that is caused by the
 * conversion from weight to cost that is done in the API.
 */
#define QOS_COST_MAX 32   // 5 bit cost
static mesa_rc QOS_weight2pct(const u8                                cnt,     // IN  Number of configured weights
                              const vtss_appl_qos_scheduler_t  *const sch_in,  // IN  Configured weights
                              vtss_appl_qos_scheduler_status_t *const sch_out) // OUT Calculated percents
{
    int        queue;
    mesa_pct_t w_min = 100;
    u8         cost[VTSS_APPL_QOS_PORT_QUEUE_CNT];
    u8         c_max = 0;
    mesa_pct_t new_weight[VTSS_APPL_QOS_PORT_QUEUE_CNT];
    int        w_sum = 0;

    if (cnt > VTSS_APPL_QOS_PORT_QUEUE_CNT) {
        return VTSS_RC_ERROR;
    }

    for (queue = 0; queue < cnt; queue++) {
        w_min = MIN(w_min, sch_in[queue].weight); // Save the lowest weight for use in next round
    }
    for (queue = 0; queue < cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        u32 c = ((((QOS_COST_MAX << 4) * w_min) / sch_in[queue].weight) + 8) >> 4;
        cost[queue] = MAX(1, c); // Force cost to be in range 1..QOS_COST_MAX
        c_max = MAX(c_max, cost[queue]); // Save the highest cost for use in next round
    }
    for (queue = 0; queue < cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        new_weight[queue] = (((c_max << 4) / cost[queue]) + 8) >> 4;
        w_sum += new_weight[queue]; // Calculate the sum of weights for use in next round
    }
    for (queue = 0; queue < cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        mesa_pct_t p =  ((((new_weight[queue] << 4) * 100) / w_sum) + 8) >> 4;
        sch_out[queue].weight = MAX(1, p); // Force pct to be in range 1..100
    }
    return VTSS_RC_OK;
}

/* Call registered QoS port configuration change callbacks */
static void QOS_port_conf_change_event(vtss_isid_t isid, mesa_port_no_t iport, const vtss_appl_qos_port_conf_t *const conf)
{
    int                        i;
    qos_port_conf_change_reg_t *reg;
    vtss_tick_count_t           ticks;

    QOS_CB_CRIT_ENTER();
    for (i = 0; i < qos.rt.count; i++) {
        reg = &qos.rt.reg[i];
        if (reg->global == (isid != VTSS_ISID_LOCAL)) {
            T_D("callback, i: %d (%s), isid: %d, iport: %u", i, vtss_module_names[reg->module_id], isid, iport);
            ticks = vtss_current_time();
            reg->callback(isid, iport, conf);
            ticks = (vtss_current_time() - ticks);
            if (ticks > reg->max_ticks) {
                reg->max_ticks = ticks;
            }
            T_D("callback done, i: %d (%s), isid: %d, iport: %u, " VPRI64u" ticks, " VPRI64u" msec", i, vtss_module_names[reg->module_id], isid, iport, ticks, VTSS_OS_TICK2MSEC(ticks));
        }
    }
    QOS_CB_CRIT_EXIT();
}

/* Set global QOS configuration to defaults */
static void QOS_conf_default_set(vtss_appl_qos_conf_t *conf)
{
    memset(conf, 0, sizeof(*conf));
    conf->global.prio_levels = VTSS_APPL_QOS_PRIO_LEVELS_8;

    conf->uc_policer.enable     = FALSE;
    conf->uc_policer.rate       = CAPA->global_storm_frame_rate_min;
    conf->uc_policer.frame_rate = TRUE;
    conf->mc_policer.enable     = FALSE;
    conf->mc_policer.rate       = CAPA->global_storm_frame_rate_min;
    conf->mc_policer.frame_rate = TRUE;
    conf->bc_policer.enable     = FALSE;
    conf->bc_policer.rate       = CAPA->global_storm_frame_rate_min;
    conf->bc_policer.frame_rate = TRUE;

    if (CAPA->has_wred2_or_wred3) {
        int group, queue, dpl;
        for (group = 0; group < CAPA->wred_group_max; group++) {
            for (queue = 0; queue < VTSS_APPL_QOS_PORT_QUEUE_CNT; queue++) {
                for (dpl = 0; dpl < CAPA->wred_dpl_max; dpl++) {
                    vtss_appl_qos_wred_t *wred = &conf->wred[queue][dpl][group];

                    wred->enable   = FALSE;
                    wred->min      = 0;
                    wred->max      = 50;
                    wred->max_unit = VTSS_APPL_QOS_WRED_MAX_DP; /* Defaults to 50% drop probability at 100% fill level */
                }
            }
        }
    }

#if defined(VTSS_SW_OPTION_QOS_ADV)
    {
        int i;
        for (i = 0; i < 64; i++) {
            conf->dscp_map[i].trust = FALSE;
            conf->dscp_map[i].cos   = 0;
            conf->dscp_map[i].dpl   = 0;
            conf->dscp_map[i].dscp = i;
            conf->dscp_map[i].remark = FALSE;
            conf->dscp_map[i].dscp_remap = i;
            conf->dscp_map[i].dscp_remap_dp1 = i;
        }
        /* conf->cos_dscp_map is already initialized to all zeroes */
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

/* Check global QOS configuration */
static mesa_rc QOS_conf_check(const vtss_appl_qos_conf_t *conf)
{
    if (conf->global.prio_levels >= VTSS_APPL_QOS_PRIO_LEVELS_LAST) {
        T_I("Invalid prio_levels");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    {
        u32 min, max;

        if (((conf->uc_policer.frame_rate == TRUE)  && (CAPA->global_storm_frame_rate_max == 0)) ||
            ((conf->uc_policer.frame_rate == FALSE) && (CAPA->global_storm_bit_rate_max   == 0))) {
            T_I("Invalid uc frame_rate %d", conf->uc_policer.frame_rate);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if (((conf->mc_policer.frame_rate == TRUE)  && (CAPA->global_storm_frame_rate_max == 0)) ||
            ((conf->mc_policer.frame_rate == FALSE) && (CAPA->global_storm_bit_rate_max   == 0))) {
            T_I("Invalid mc frame_rate %d", conf->mc_policer.frame_rate);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if (((conf->bc_policer.frame_rate == TRUE)  && (CAPA->global_storm_frame_rate_max == 0)) ||
            ((conf->bc_policer.frame_rate == FALSE) && (CAPA->global_storm_bit_rate_max   == 0))) {
            T_I("Invalid bc frame_rate %d", conf->bc_policer.frame_rate);
            return VTSS_APPL_QOS_ERROR_PARM;
        }

        min = vtss_appl_qos_rate_min(CAPA->global_storm_bit_rate_min, CAPA->global_storm_frame_rate_min);
        max = vtss_appl_qos_rate_max(CAPA->global_storm_bit_rate_max, CAPA->global_storm_frame_rate_max);

        if ((conf->uc_policer.rate < min) || (conf->uc_policer.rate > max)) {
            T_I("Invalid uc rate %u", conf->uc_policer.rate);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if ((conf->mc_policer.rate < min) || (conf->mc_policer.rate > max)) {
            T_I("Invalid mc rate %u", conf->mc_policer.rate);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if ((conf->bc_policer.rate < min) || (conf->bc_policer.rate > max)) {
            T_I("Invalid bc rate %u", conf->bc_policer.rate);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }

    if (CAPA->has_wred2_or_wred3) {
        int group, queue, dpl;

        for (group = 0; group < CAPA->wred_group_max; group++) {
            for (queue = 0; queue < VTSS_APPL_QOS_PORT_QUEUE_CNT; queue++) {
                for (dpl = 0; dpl < CAPA->wred_dpl_max; dpl++) {
                    const vtss_appl_qos_wred_t *red = &conf->wred[queue][dpl][group];
                    if (red->min > 100) {
                        T_I("Invalid min (%d)", red->min);
                        return VTSS_APPL_QOS_ERROR_PARM;
                    }
                    if ((red->max < 1) || (red->max > 100)) {
                        T_I("Invalid max (%d) on queue %d, dpl %d", red->max, queue, dpl);
                        return VTSS_APPL_QOS_ERROR_PARM;
                    }
                    if ((red->max_unit != VTSS_APPL_QOS_WRED_MAX_DP) && (red->max_unit != VTSS_APPL_QOS_WRED_MAX_FL)) {
                        T_I("Invalid max_unit (%d) on queue %d, dpl %d", red->max_unit, queue, dpl);
                        return VTSS_APPL_QOS_ERROR_PARM;
                    }
                    if ((red->max_unit == VTSS_APPL_QOS_WRED_MAX_FL) && (red->min >= red->max)) {
                        T_I("min (%u) >= max (%u) on queue %d, dpl %d", red->min, red->max, queue, dpl);
                        return VTSS_APPL_QOS_ERROR_PARM;
                    }
                }
            }
        }
    }

#if defined(VTSS_SW_OPTION_QOS_ADV)
    {
        int i;
        for (i = 0; i < 64; i++) {
            if (conf->dscp_map[i].cos > VTSS_APPL_QOS_CLASS_MAX) {
                T_I("Invalid cos");
                return VTSS_APPL_QOS_ERROR_PARM;
            }
            if (conf->dscp_map[i].dpl > CAPA->dpl_max) {
                T_I("Invalid dpl");
                return VTSS_APPL_QOS_ERROR_PARM;
            }

            if (conf->dscp_map[i].dscp > 63) {
                T_I("Invalid dscp");
                return VTSS_APPL_QOS_ERROR_PARM;
            }
            if (conf->dscp_map[i].dscp_remap > 63) {
                T_I("Invalid dscp_remap");
                return VTSS_APPL_QOS_ERROR_PARM;
            }

            if (CAPA->has_dscp_dpl_remark && conf->dscp_map[i].dscp_remap_dp1 > 63) {
                T_I("Invalid dscp_remap_dp1");
                return VTSS_APPL_QOS_ERROR_PARM;
            }
        }
        for (i = 0; i < VTSS_PRIO_ARRAY_SIZE; i++) {
            if (conf->cos_dscp_map[i].dscp > 63) {
                T_I("Invalid dscp");
                return VTSS_APPL_QOS_ERROR_PARM;
            }
            if (conf->cos_dscp_map[i].dscp_dp1 > 63) {
                T_I("Invalid dscp_dp1");
                return VTSS_APPL_QOS_ERROR_PARM;
            }
            if (CAPA->has_dscp_dp2 && conf->cos_dscp_map[i].dscp_dp2 > 63) {
                T_I("Invalid dscp_dp2");
                return VTSS_APPL_QOS_ERROR_PARM;
            }
            if (CAPA->has_dscp_dp3 && conf->cos_dscp_map[i].dscp_dp3 > 63) {
                T_I("Invalid dscp_dp3");
                return VTSS_APPL_QOS_ERROR_PARM;
            }
        }
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    return VTSS_RC_OK;
}

/* Configuration per (DSCP, DPL) */
static void QOS_dscp_dpl_conf_apply(vtss_appl_qos_conf_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    CapArray<mesa_qos_dscp_dpl_conf_t, MESA_CAP_DSCP_CNT, MESA_CAP_QOS_DPL_CNT> mesa_conf;
    mesa_rc rc;
    int     dscp;

    if ((rc = mesa_qos_dscp_dpl_conf_get(NULL, fast_cap(MESA_CAP_QOS_DPL_CNT), &mesa_conf[0][0])) != VTSS_RC_OK) {
        T_E("Error getting QoS DSCP/DPL configuration - %s\n", error_txt(rc));
    } else {
        for (dscp = 0; dscp < MESA_DSCP_CNT; dscp++) {
            mesa_conf[dscp][0].dscp = conf->dscp_map[dscp].dscp_remap;
            mesa_conf[dscp][1].dscp = conf->dscp_map[dscp].dscp_remap_dp1;
        }
        if ((rc = mesa_qos_dscp_dpl_conf_set(NULL, fast_cap(MESA_CAP_QOS_DPL_CNT), &mesa_conf[0][0])) != VTSS_RC_OK) {
            T_E("Error setting QoS DSCP/DPL configuration - %s\n", error_txt(rc));
        }
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

/* Configuration per DPL */
static void QOS_dpl_conf_apply(vtss_appl_qos_conf_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    CapArray<mesa_qos_dpl_conf_t, MESA_CAP_QOS_DPL_CNT> mesa_conf;
    mesa_rc rc;
    u32     dpl_cnt = fast_cap(MESA_CAP_QOS_DPL_CNT);
    int     prio;

    if ((rc = mesa_qos_dpl_conf_get(NULL, dpl_cnt, mesa_conf.data())) != VTSS_RC_OK) {
        T_E("Error getting QoS DPL configuration - %s\n", error_txt(rc));
    } else {
        for (prio = 0; prio < VTSS_PRIO_ARRAY_SIZE; prio++) {
            mesa_conf[0].dscp[prio] = conf->cos_dscp_map[prio].dscp;
            mesa_conf[1].dscp[prio] = conf->cos_dscp_map[prio].dscp_dp1;
            if (dpl_cnt > 2) {
                mesa_conf[2].dscp[prio] = conf->cos_dscp_map[prio].dscp_dp2;
                mesa_conf[3].dscp[prio] = conf->cos_dscp_map[prio].dscp_dp3;
            }
        }
        if ((rc = mesa_qos_dpl_conf_set(NULL, dpl_cnt, mesa_conf.data())) != VTSS_RC_OK) {
            T_E("Error setting QoS DPL configuration - %s\n", error_txt(rc));
        }
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

/* Configuration per (DPL, GROUP) */
static void QOS_dpl_group_conf_apply(vtss_appl_qos_conf_t *conf)
{
    CapArray<mesa_qos_dpl_group_conf_t, MESA_CAP_QOS_DPL_CNT, MESA_CAP_QOS_WRED_GROUP_CNT> mesa_conf;
    mesa_rc              rc;
    int                  group, queue, dpl;
    u32                  dpl_cnt = fast_cap(MESA_CAP_QOS_DPL_CNT);
    u32                  grp_cnt = fast_cap(MESA_CAP_QOS_WRED_GROUP_CNT);
    vtss_appl_qos_wred_t *appl_wred;
    mesa_wred_conf_t     *mesa_wred;

    if (!CAPA->has_wred2_or_wred3) {
        /* No WRED */
    } else if ((rc = mesa_qos_dpl_group_conf_get(NULL, dpl_cnt, grp_cnt, mesa_conf.data())) != VTSS_RC_OK) {
        T_E("Error getting QoS DPL/GRP configuration - %s\n", error_txt(rc));
    } else {
        for (dpl = 1; dpl < dpl_cnt; dpl++) {
            /* DPL zero is not used */
            for (group = 0; group < grp_cnt; group++) {
                for (queue = 0; queue < VTSS_APPL_QOS_PORT_QUEUE_CNT; queue++) {
                    mesa_wred = &mesa_conf[dpl][group].wred[queue];
                    appl_wred = &conf->wred[queue][dpl - 1][group];
                    mesa_wred->enable = appl_wred->enable;
                    mesa_wred->min_fl = appl_wred->min;
                    mesa_wred->max = appl_wred->max;
                    mesa_wred->max_unit = (appl_wred->max_unit == VTSS_APPL_QOS_WRED_MAX_FL ? MESA_WRED_MAX_FL : MESA_WRED_MAX_DP);
                }
            }
        }
        if ((rc = mesa_qos_dpl_group_conf_set(NULL, dpl_cnt, grp_cnt, mesa_conf.data())) != VTSS_RC_OK) {
            T_E("Error setting QoS DPL/GRP configuration - %s\n", error_txt(rc));
        }
    }
}

/* Apply global QOS configuration to API */
static void QOS_conf_apply(vtss_appl_qos_conf_t *conf)
{
    mesa_qos_conf_t mesa_conf;
    mesa_rc         rc;

    if ((rc = mesa_qos_conf_get(NULL, &mesa_conf)) != VTSS_RC_OK) {
        T_E("Error getting QoS configuration - %s\n", error_txt(rc));
    } else {
        if (conf->global.prio_levels < VTSS_APPL_QOS_PRIO_LEVELS_LAST) {
            mesa_conf.prios = 1 << conf->global.prio_levels;
        } else {
            mesa_conf.prios = 8;
        }

        mesa_conf.policer_uc.rate       = (conf->uc_policer.enable ? conf->uc_policer.rate : VTSS_PACKET_RATE_DISABLED);
        mesa_conf.policer_uc.frame_rate = conf->uc_policer.frame_rate;
        mesa_conf.policer_uc.mode       = MESA_STORM_POLICER_MODE_PORTS_ONLY;

        mesa_conf.policer_mc.rate       = (conf->mc_policer.enable ? conf->mc_policer.rate : VTSS_PACKET_RATE_DISABLED);
        mesa_conf.policer_mc.frame_rate = conf->mc_policer.frame_rate;
        mesa_conf.policer_mc.mode       = MESA_STORM_POLICER_MODE_PORTS_ONLY;

        mesa_conf.policer_bc.rate       = (conf->bc_policer.enable ? conf->bc_policer.rate : VTSS_PACKET_RATE_DISABLED);
        mesa_conf.policer_bc.frame_rate = conf->bc_policer.frame_rate;
        mesa_conf.policer_bc.mode       = MESA_STORM_POLICER_MODE_PORTS_ONLY;

#if defined(VTSS_SW_OPTION_QOS_ADV)
        {
            int i;

            for (i = 0; i < 64; i++) {
                mesa_qos_dscp_conf_t *dscp_conf = &mesa_conf.dscp[i];
                dscp_conf->trust  = conf->dscp_map[i].trust;
                dscp_conf->prio   = conf->dscp_map[i].cos;
                dscp_conf->dpl    = conf->dscp_map[i].dpl;
                dscp_conf->remark = conf->dscp_map[i].remark;
                dscp_conf->dscp   = conf->dscp_map[i].dscp;
            }
        }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

        /* apply new data to driver layer */
        if ((rc = mesa_qos_conf_set(NULL, &mesa_conf)) != VTSS_RC_OK) {
            T_E("Error setting QoS configuration - %s\n", error_txt(rc));
        }
    }

    /* Configuration per (DSCP, DPL) */
    QOS_dscp_dpl_conf_apply(conf);

    /* Configuration per DPL */
    QOS_dpl_conf_apply(conf);

    /* Configuration per (DPL, GROUP) */
    QOS_dpl_group_conf_apply(conf);
}

/* Get QoS global configuration  */
static mesa_rc QOS_conf_get(vtss_appl_qos_conf_t *conf)
{
    QOS_CRIT_ASSERT_LOCKED();
    T_N("enter");
    *conf = qos.qos_conf;
    T_N("exit");
    return VTSS_RC_OK;
}

/* Set QoS global configuration  */
static mesa_rc QOS_conf_set(const vtss_appl_qos_conf_t *conf)
{
    mesa_rc rc;

    QOS_CRIT_ASSERT_LOCKED();
    T_N("enter");
    if ((rc = QOS_conf_check(conf)) != VTSS_RC_OK) {
        return rc;
    }
    qos.qos_conf = *conf;
    QOS_stack_conf_set(VTSS_ISID_GLOBAL);
    T_N("exit");
    return rc;
}

/* Set QOS port configuration to defaults */
static void QOS_port_conf_default_set(vtss_appl_qos_port_conf_t *conf)
{
    int i;

    vtss_clear(*conf);

    /* Ingress classification */
    conf->port.default_cos = 0;
    conf->port.default_pcp = 0;
    conf->port.default_dpl = 0;

    conf->port.default_dei = 0;
    conf->port.trust_tag   = FALSE;
    for (i = VTSS_PCP_START; i < VTSS_PCP_ARRAY_SIZE; i++) {
        int dei;
        for (dei = VTSS_DEI_START; dei < VTSS_DEI_ARRAY_SIZE; dei++) {
            conf->tag_cos_map[i][dei].cos = QOS_pcp2cos(i);
            conf->tag_cos_map[i][dei].dpl = dei; // Defaults to same value as DEI
        }
    }
#if defined(VTSS_SW_OPTION_QOS_ADV)
    conf->port.trust_dscp = FALSE; /* Default: no DSCP based classification */
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    /* Policers */
    conf->port_policer.cir = VTSS_APPL_QOS_BITRATE_DEF;
    conf->port_policer.cbs = VTSS_APPL_QOS_BURSTSIZE_DEF;

#if defined(VTSS_SW_OPTION_QOS_ADV)
    for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
        conf->queue_policer[i].cir = VTSS_APPL_QOS_BITRATE_DEF;
        conf->queue_policer[i].cbs = VTSS_APPL_QOS_BURSTSIZE_DEF;
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    conf->uc_policer.cir = VTSS_APPL_QOS_BITRATE_DEF;
    conf->uc_policer.cbs = VTSS_APPL_QOS_BURSTSIZE_DEF;
    conf->bc_policer.cir = VTSS_APPL_QOS_BITRATE_DEF;
    conf->bc_policer.cbs = VTSS_APPL_QOS_BURSTSIZE_DEF;
    conf->un_policer.cir = VTSS_APPL_QOS_BITRATE_DEF;
    conf->un_policer.cbs = VTSS_APPL_QOS_BURSTSIZE_DEF;

    /* Shapers */
    conf->port_shaper.enable = FALSE;
    conf->port_shaper.mode   = VTSS_APPL_QOS_SHAPER_MODE_LINE;
    conf->port_shaper.cir    = VTSS_APPL_QOS_BITRATE_DEF;
    conf->port_shaper.cbs    = VTSS_APPL_QOS_BURSTSIZE_DEF;
    conf->port_shaper.dlb    = FALSE;
    conf->port_shaper.eir    = VTSS_APPL_QOS_BITRATE_DEF;
    conf->port_shaper.ebs    = VTSS_APPL_QOS_BURSTSIZE_DEF;

    for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
        conf->queue_shaper[i].enable = FALSE;
        conf->queue_shaper[i].mode   = VTSS_APPL_QOS_SHAPER_MODE_LINE;
        conf->queue_shaper[i].excess = FALSE;
        conf->queue_shaper[i].credit = FALSE;
        conf->queue_shaper[i].cir    = VTSS_APPL_QOS_BITRATE_DEF;
        conf->queue_shaper[i].cbs    = VTSS_APPL_QOS_BURSTSIZE_DEF;
        conf->queue_shaper[i].dlb    = FALSE;
        conf->queue_shaper[i].eir    = VTSS_APPL_QOS_BITRATE_DEF;
        conf->queue_shaper[i].ebs    = VTSS_APPL_QOS_BURSTSIZE_DEF;
    }

    /* Scheduler */
    conf->port.dwrr_cnt = 0; /* Strict */
    for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
        conf->scheduler[i].weight           = 17;  /* This will give each queue an equal initial weight */
        conf->scheduler[i].cut_through      = FALSE;
        conf->scheduler[i].frame_preemption = FALSE;
    }

    /* Tag remarking */
    conf->port.tag_remark_mode = VTSS_APPL_QOS_TAG_REMARK_MODE_CLASSIFIED;
    conf->port.tag_default_pcp = 0;
    conf->port.tag_default_dei = 0;
    for (i = VTSS_PRIO_START; i < VTSS_APPL_QOS_PORT_PRIO_CNT; i++) {
        int dpl;
        for (dpl = 0; dpl < 2; dpl++) {
            conf->cos_tag_map[i][dpl].pcp = QOS_pcp2cos(i);
            conf->cos_tag_map[i][dpl].dei = dpl; // Defaults to same value as DP level
        }
    }

    /* DSCP */
#if defined(VTSS_SW_OPTION_QOS_ADV)
    conf->port.dscp_translate = FALSE; /* Default: No Ingress DSCP translate */
    conf->port.dscp_imode = VTSS_APPL_QOS_DSCP_MODE_NONE; /* No DSCP ingress classification */
    conf->port.dscp_emode = VTSS_APPL_QOS_DSCP_EMODE_DISABLE; /* NO DSCP egress remark */
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    conf->port.key_type = MESA_VCAP_KEY_TYPE_NORMAL;

    conf->port.wred_group = CAPA->wred_group_min;

    conf->port.ingress_map = MESA_QOS_MAP_ID_NONE;
    conf->port.egress_map = MESA_QOS_MAP_ID_NONE;

}

/* Check port QOS configuration */
static mesa_rc QOS_port_conf_check(const vtss_appl_qos_port_conf_t *conf)
{
    int i;
    u32 rate_min, rate_max, burst_min, burst_max;

    /* Ingress classification */
    if (conf->port.default_cos > iprio2uprio(VTSS_APPL_QOS_PORT_PRIO_CNT - 1)) {
        T_I("Invalid cos");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (conf->port.default_pcp > (VTSS_PCPS - 1)) {
        T_I("Invalid pcp");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (conf->port.default_dpl > CAPA->dpl_max) {
        T_I("Invalid dpl");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (conf->port.default_dei > (VTSS_DEIS - 1)) {
        T_I("Invalid dei");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    for (i = VTSS_PCP_START; i < VTSS_PCP_ARRAY_SIZE; i++) {
        int dei;
        for (dei = VTSS_DEI_START; dei < VTSS_DEI_ARRAY_SIZE; dei++) {
            if (conf->tag_cos_map[i][dei].cos > VTSS_APPL_QOS_CLASS_MAX) {
                T_I("Invalid tag_cos_map[%d][%d].cos", i, dei);
                return VTSS_APPL_QOS_ERROR_PARM;
            }
            if (conf->tag_cos_map[i][dei].dpl > CAPA->dpl_max) {
                T_I("Invalid tag_cos_map[%d][%d].dpl", i, dei);
                return VTSS_APPL_QOS_ERROR_PARM;
            }
        }
    }

    /* Policers */
    rate_min  = vtss_appl_qos_rate_min(CAPA->port_policer_bit_rate_min,  CAPA->port_policer_frame_rate_min);
    rate_max  = vtss_appl_qos_rate_max(CAPA->port_policer_bit_rate_max,  CAPA->port_policer_frame_rate_max);
    burst_min = vtss_appl_qos_rate_min(CAPA->port_policer_bit_burst_min, CAPA->port_policer_frame_burst_min);
    burst_max = vtss_appl_qos_rate_max(CAPA->port_policer_bit_burst_max, CAPA->port_policer_frame_burst_max);

    if ((conf->port_policer.cir < rate_min) || (conf->port_policer.cir > rate_max)) {
        T_I("Invalid port_policer.cir");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if ((conf->port_policer.cbs < burst_min) || (conf->port_policer.cbs > burst_max)) {
        T_I("Invalid port_policer.cbs");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

#if defined(VTSS_SW_OPTION_QOS_ADV)
    rate_min  = vtss_appl_qos_rate_min(CAPA->queue_policer_bit_rate_min,  CAPA->queue_policer_frame_rate_min);
    rate_max  = vtss_appl_qos_rate_max(CAPA->queue_policer_bit_rate_max,  CAPA->queue_policer_frame_rate_max);
    burst_min = vtss_appl_qos_rate_min(CAPA->queue_policer_bit_burst_min, CAPA->queue_policer_frame_burst_min);
    burst_max = vtss_appl_qos_rate_max(CAPA->queue_policer_bit_burst_max, CAPA->queue_policer_frame_burst_max);

    for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
        if ((conf->queue_policer[i].cir < rate_min) || (conf->queue_policer[i].cir > rate_max)) {
            T_I("Invalid queue_policer[%d].cir", i);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if ((conf->queue_policer[i].cbs < burst_min) || (conf->queue_policer[i].cbs > burst_max)) {
            T_I("Invalid queue_policer[%d].cbs", i);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
        rate_min  = vtss_appl_qos_rate_min(CAPA->port_storm_bit_rate_min,  CAPA->port_storm_frame_rate_min);
        rate_max  = vtss_appl_qos_rate_max(CAPA->port_storm_bit_rate_max,  CAPA->port_storm_frame_rate_max);
        burst_min = vtss_appl_qos_rate_min(CAPA->port_storm_bit_burst_min, CAPA->port_storm_frame_burst_min);
        burst_max = vtss_appl_qos_rate_max(CAPA->port_storm_bit_burst_max, CAPA->port_storm_frame_burst_max);

        if ((conf->uc_policer.cir < rate_min) || (conf->uc_policer.cir > rate_max)) {
            T_I("Invalid uc_policer.cir");
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if ((conf->uc_policer.cbs < burst_min) || (conf->uc_policer.cbs > burst_max)) {
            T_I("Invalid uc_policer.cbs");
            return VTSS_APPL_QOS_ERROR_PARM;
        }

        if ((conf->bc_policer.cir < rate_min) || (conf->bc_policer.cir > rate_max)) {
            T_I("Invalid bc_policer.cir");
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if ((conf->bc_policer.cbs < burst_min) || (conf->bc_policer.cbs > burst_max)) {
            T_I("Invalid bc_policer.cbs");
            return VTSS_APPL_QOS_ERROR_PARM;
        }

        if ((conf->un_policer.cir < rate_min) || (conf->un_policer.cir > rate_max)) {
            T_I("Invalid un_policer.cir");
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if ((conf->un_policer.cbs < burst_min) || (conf->un_policer.cbs > burst_max)) {
            T_I("Invalid un_policer.cbs");
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }

    /* Shapers */
    rate_min  = vtss_appl_qos_rate_min(CAPA->port_shaper_bit_rate_min,  CAPA->port_shaper_frame_rate_min);
    rate_max  = vtss_appl_qos_rate_max(CAPA->port_shaper_bit_rate_max,  CAPA->port_shaper_frame_rate_max);
    burst_min = vtss_appl_qos_rate_min(CAPA->port_shaper_bit_burst_min, CAPA->port_shaper_frame_burst_min);
    burst_max = vtss_appl_qos_rate_max(CAPA->port_shaper_bit_burst_max, CAPA->port_shaper_frame_burst_max);

    if ((conf->port_shaper.cir < rate_min) || (conf->port_shaper.cir > rate_max)) {
        T_I("Invalid port_shaper.cir");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if ((conf->port_shaper.cbs < burst_min) || (conf->port_shaper.cbs > burst_max)) {
        T_I("Invalid port_shaper.cbs");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (CAPA->has_port_shapers_dlb) {
        if ((conf->port_shaper.eir < rate_min) || (conf->port_shaper.eir > rate_max)) {
            T_I("Invalid port_shaper.eir");
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if ((conf->port_shaper.ebs < burst_min) || (conf->port_shaper.ebs > burst_max)) {
            T_I("Invalid port_shaper.ebs");
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }

    rate_min  = vtss_appl_qos_rate_min(CAPA->queue_shaper_bit_rate_min,  CAPA->queue_shaper_frame_rate_min);
    rate_max  = vtss_appl_qos_rate_max(CAPA->queue_shaper_bit_rate_max,  CAPA->queue_shaper_frame_rate_max);
    burst_min = vtss_appl_qos_rate_min(CAPA->queue_shaper_bit_burst_min, CAPA->queue_shaper_frame_burst_min);
    burst_max = vtss_appl_qos_rate_max(CAPA->queue_shaper_bit_burst_max, CAPA->queue_shaper_frame_burst_max);

    for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
        if ((conf->queue_shaper[i].cir < rate_min) || (conf->queue_shaper[i].cir > rate_max)) {
            T_I("Invalid queue_shaper[%d].cir", i);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if ((conf->queue_shaper[i].cbs < burst_min) || (conf->queue_shaper[i].cbs > burst_max)) {
            T_I("Invalid queue_shaper[%d].cbs", i);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        if (CAPA->has_queue_shapers_dlb) {
            if ((conf->queue_shaper[i].eir < rate_min) || (conf->queue_shaper[i].eir > rate_max)) {
                T_I("Invalid queue_shaper[%d].eir", i);
                return VTSS_APPL_QOS_ERROR_PARM;
            }
            if ((conf->queue_shaper[i].ebs < burst_min) || (conf->queue_shaper[i].ebs > burst_max)) {
                T_I("Invalid queue_shaper[%d].ebs", i);
                return VTSS_APPL_QOS_ERROR_PARM;
            }
        }
    }

    /* Scheduler */
    if (conf->port.dwrr_cnt) {
        if ((CAPA->dwrr_cnt_mask & (1 << (conf->port.dwrr_cnt - 1))) == 0) {
            T_I("Invalid dwrr_cnt: %u", conf->port.dwrr_cnt);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }
    for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
        if ((conf->scheduler[i].weight < 1) || (conf->scheduler[i].weight > 100)) {
            T_I("Invalid queue_pct[%d]", i);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }

    /* Tag remarking */
    if ((conf->port.tag_remark_mode != VTSS_APPL_QOS_TAG_REMARK_MODE_CLASSIFIED) &&
        (conf->port.tag_remark_mode != VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT) &&
        (conf->port.tag_remark_mode != VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED)) {
        T_I("Invalid tag_remark_mode");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (conf->port.tag_default_pcp > (VTSS_PCPS - 1)) {
        T_I("Invalid default_pcp");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (conf->port.tag_default_dei > (VTSS_DEIS - 1)) {
        T_I("Invalid default_dei");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    for (i = VTSS_PRIO_START; i < VTSS_APPL_QOS_PORT_PRIO_CNT; i++) {
        int dpl;
        for (dpl = 0; dpl < 2; dpl++) {
            if (conf->cos_tag_map[i][dpl].pcp > (VTSS_PCPS - 1)) {
                T_I("Invalid cos_tag_map[%d][%d].pcp", i, dpl);
                return VTSS_APPL_QOS_ERROR_PARM;
            }
            if (conf->cos_tag_map[i][dpl].dei > (VTSS_DEIS - 1)) {
                T_I("Invalid cos_tag_map[%d][%d].dei", i, dpl);
                return VTSS_APPL_QOS_ERROR_PARM;
            }
        }
    }

    /* DSCP */
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if ((conf->port.dscp_imode != VTSS_APPL_QOS_DSCP_MODE_NONE) &&
        (conf->port.dscp_imode != VTSS_APPL_QOS_DSCP_MODE_ZERO) &&
        (conf->port.dscp_imode != VTSS_APPL_QOS_DSCP_MODE_SEL) &&
        (conf->port.dscp_imode != VTSS_APPL_QOS_DSCP_MODE_ALL)) {
        T_I("Invalid dscp_imode");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    switch (conf->port.dscp_emode) {
    case VTSS_APPL_QOS_DSCP_EMODE_DISABLE:
    case VTSS_APPL_QOS_DSCP_EMODE_REMARK:
    case VTSS_APPL_QOS_DSCP_EMODE_REMAP:
        break;
    case VTSS_APPL_QOS_DSCP_EMODE_REMAP_DPA:
        if (CAPA->has_dscp_dpl_remark) {
            break;
        }
        /* Fall through */
    default:
        T_I("Invalid dscp_emode");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    if (CAPA->has_qce_key_type &&
        (conf->port.key_type != MESA_VCAP_KEY_TYPE_NORMAL) &&
        (conf->port.key_type != MESA_VCAP_KEY_TYPE_DOUBLE_TAG) &&
        (conf->port.key_type != MESA_VCAP_KEY_TYPE_IP_ADDR) &&
        (conf->port.key_type != MESA_VCAP_KEY_TYPE_MAC_IP_ADDR)) {
        T_I("Invalid key_type");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((CAPA->wred_group_max > 1) &&
        (conf->port.wred_group < CAPA->wred_group_min ||
         conf->port.wred_group > CAPA->wred_group_max)) {
        T_I("Invalid wred group");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (CAPA->has_cosid_classification && conf->port.default_cosid > CAPA->cosid_max) {
        T_I("Invalid cosid");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (CAPA->has_ingress_map &&
        (conf->port.ingress_map != MESA_QOS_MAP_ID_NONE) &&
        (conf->port.ingress_map > CAPA->ingress_map_id_max)) {
        T_I("Invalid ingress map id");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (CAPA->has_egress_map &&
        (conf->port.egress_map != MESA_QOS_MAP_ID_NONE) &&
        (conf->port.egress_map > CAPA->egress_map_id_max)) {
        T_I("Invalid egress map id");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    return VTSS_RC_OK;
}


/* Port policers */
static void QOS_port_policer_conf_apply(mesa_port_no_t port_no, vtss_appl_qos_port_conf_t *conf)
{
    CapArray<mesa_qos_port_policer_conf_t, MESA_CAP_QOS_PORT_POLICER_CNT> mesa_conf;
    u32                          pol_cnt = fast_cap(MESA_CAP_QOS_PORT_POLICER_CNT);
    mesa_rc                      rc;
    mesa_qos_port_policer_conf_t *pol;
    int                          i = 0;

    if ((rc = mesa_qos_port_policer_conf_get(NULL, port_no, pol_cnt, mesa_conf.data())) != VTSS_RC_OK) {
        T_E("Error getting QoS port %u policer configuration - %s\n", port_no, error_txt(rc));
    } else {
        /* Index zero is the main port policer */
        pol = &mesa_conf[i];
        if (conf->port_policer.enable) {
            pol->policer.rate = conf->port_policer.cir;
            pol->policer.level = conf->port_policer.cbs;
        } else {
            pol->policer.rate = VTSS_BITRATE_DISABLED;
        }
        pol->frame_rate = conf->port_policer.frame_rate;
        pol->flow_control = conf->port_policer.flow_control;
        pol->known_unicast        = TRUE;
        pol->unknown_unicast      = TRUE;
        pol->known_multicast      = TRUE;
        pol->unknown_multicast    = TRUE;
        pol->known_broadcast      = TRUE;
        pol->unknown_broadcast    = TRUE;
        pol->learning             = TRUE;
        pol->limit_noncpu_traffic = TRUE;
        pol->limit_cpu_traffic    = FALSE;

        if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
            /* The following indices are used for storm policers */
            for (i = QOS_STORM_POLICER_UNICAST_IX; i < pol_cnt; i++) {
                vtss_appl_qos_port_storm_policer_t *st_pol;

                pol = &mesa_conf[i];
                memset(pol, 0, sizeof(*pol));
                if (i == QOS_STORM_POLICER_UNICAST_IX) {
                    st_pol = &conf->uc_policer;
                    pol->known_unicast     = TRUE;
                    pol->unknown_unicast   = TRUE;
                } else if (i == QOS_STORM_POLICER_BROADCAST_IX) {
                    st_pol = &conf->bc_policer;
                    pol->known_broadcast   = TRUE;
                    pol->unknown_broadcast = TRUE;
                } else {
                    st_pol = &conf->un_policer;
                    pol->unknown_unicast   = TRUE;
                    pol->unknown_multicast = TRUE;
                    pol->unknown_broadcast = TRUE;
                }
                if (st_pol->enable) {
                    pol->policer.rate = st_pol->cir;
                    pol->policer.level = st_pol->cbs;
                } else {
                    pol->policer.rate = VTSS_BITRATE_DISABLED;
                }
                pol->frame_rate = st_pol->frame_rate;
                pol->limit_noncpu_traffic = TRUE;
                pol->limit_cpu_traffic    = TRUE;
            }
        }
        if ((rc = mesa_qos_port_policer_conf_set(NULL, port_no, pol_cnt, mesa_conf.data())) != VTSS_RC_OK) {
            T_E("Error setting QoS port %u policer configuration - %s\n", port_no, error_txt(rc));
        }
    }
}

/* DPL configuration */
static void QOS_port_dpl_conf_apply(mesa_port_no_t port_no, vtss_appl_qos_port_conf_t *qos_port_conf)
{
    CapArray<mesa_qos_port_dpl_conf_t, MESA_CAP_QOS_DPL_CNT> dpl_conf;
    u32     dpl_cnt = fast_cap(MESA_CAP_QOS_DPL_CNT);
    mesa_rc rc;
    int     prio;

    if ((rc = mesa_qos_port_dpl_conf_get(NULL, port_no, dpl_cnt, dpl_conf.data())) != VTSS_RC_OK) {
        T_E("Error getting QoS port %u DPL configuration - %s\n", port_no, error_txt(rc));
    } else {
        for (prio = 0; prio < VTSS_APPL_QOS_PORT_PRIO_CNT; prio++) {
            int dpl;
            for (dpl = 0; dpl < 2; dpl++) {
                /* Only DPL 0 and 1 are currently used */
                dpl_conf[dpl].pcp[prio] = qos_port_conf->cos_tag_map[prio][dpl].pcp;
                dpl_conf[dpl].dei[prio] = qos_port_conf->cos_tag_map[prio][dpl].dei;
            }
        }
        if ((rc = mesa_qos_port_dpl_conf_set(NULL, port_no, dpl_cnt, dpl_conf.data())) != VTSS_RC_OK) {
            T_E("Error setting QoS port %u DPL configuration - %s\n", port_no, error_txt(rc));
        }
    }
}

/* Apply QOS port configuration to API */
static void QOS_port_conf_apply(mesa_port_no_t port_no, vtss_appl_qos_port_conf_t *conf)
{
    mesa_qos_port_conf_t mesa_conf;
    mesa_rc              rc;
    int                  i;

    QOS_port_conf_change_event(VTSS_ISID_LOCAL, port_no, conf); // Call local callbacks

    /* Port policers */
    QOS_port_policer_conf_apply(port_no, conf);

    /* DPL configuration */
    QOS_port_dpl_conf_apply(port_no, conf);

    if ((rc = mesa_qos_port_conf_get(NULL, port_no, &mesa_conf)) != VTSS_RC_OK) {
        T_E("Error getting QoS port %u configuration - %s\n", port_no, error_txt(rc));
    } else {
        mesa_conf.default_prio     = conf->port.default_cos;
        mesa_conf.default_dpl      = conf->port.default_dpl;
        mesa_conf.tag.pcp          = conf->port.default_pcp;
        mesa_conf.tag.dei          = conf->port.default_dei;
        mesa_conf.tag.class_enable = conf->port.trust_tag;
        for (i = VTSS_PCP_START; i < VTSS_PCP_ARRAY_SIZE; i++) {
            int dei;
            for (dei = VTSS_DEI_START; dei < VTSS_DEI_ARRAY_SIZE; dei++) {
                mesa_conf.tag.pcp_dei_map[i][dei].prio = conf->tag_cos_map[i][dei].cos;
                mesa_conf.tag.pcp_dei_map[i][dei].dpl  = conf->tag_cos_map[i][dei].dpl;
            }
        }

        /* Shapers */
        if (conf->port_shaper.enable) {
            mesa_conf.shaper.mode = (mesa_shaper_mode_t)conf->port_shaper.mode;
            mesa_conf.shaper.rate = conf->port_shaper.cir;
            mesa_conf.shaper.level = conf->port_shaper.cbs;
            if (conf->port_shaper.dlb) {
                mesa_conf.shaper.eir = conf->port_shaper.eir;
                mesa_conf.shaper.ebs = conf->port_shaper.ebs;
            } else {
                mesa_conf.shaper.eir = VTSS_BITRATE_DISABLED;
            }
        } else {
            mesa_conf.shaper.mode = MESA_SHAPER_MODE_LINE;
            mesa_conf.shaper.rate = VTSS_BITRATE_DISABLED;
            mesa_conf.shaper.eir = VTSS_BITRATE_DISABLED;
        }

        for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
            mesa_qos_port_queue_conf_t    *queue_conf = &mesa_conf.queue[i];
            vtss_appl_qos_queue_shaper_t  *shaper = &conf->queue_shaper[i];

#if defined(VTSS_SW_OPTION_QOS_ADV)
            /* Queue policers */
            vtss_appl_qos_queue_policer_t *policer = &conf->queue_policer[i];

            if (policer->enable) {
                queue_conf->policer.rate = policer->cir;
                queue_conf->policer.level = policer->cbs;
            } else {
                queue_conf->policer.rate = VTSS_BITRATE_DISABLED;
            }
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

            /* Queue shapers */
            if (shaper->enable) {
                queue_conf->shaper.mode = (mesa_shaper_mode_t)shaper->mode;
                queue_conf->shaper.rate = shaper->cir;
                queue_conf->shaper.level = shaper->cbs;
                queue_conf->shaper.credit_enable = shaper->credit;
                if (conf->queue_shaper[i].dlb) {
                    queue_conf->shaper.eir = shaper->eir;
                    queue_conf->shaper.ebs = shaper->ebs;
                } else {
                    queue_conf->shaper.eir = VTSS_BITRATE_DISABLED;
                }
            } else {
                queue_conf->shaper.mode = MESA_SHAPER_MODE_LINE;
                queue_conf->shaper.rate = VTSS_BITRATE_DISABLED;
                queue_conf->shaper.eir = VTSS_BITRATE_DISABLED;
            }
            queue_conf->excess_enable = shaper->excess;
        }

        /* Scheduler */
        mesa_conf.dwrr_enable = (conf->port.dwrr_cnt != 0);
        mesa_conf.dwrr_cnt = conf->port.dwrr_cnt;
        for (i = 0; i < VTSS_APPL_QOS_PORT_QUEUE_CNT; i++) {
            mesa_conf.queue[i].pct = conf->scheduler[i].weight;
            mesa_conf.queue[i].cut_through_enable = conf->scheduler[i].cut_through;
        }

        /* Tag remarking */
        mesa_conf.tag.remark_mode = (conf->port.tag_remark_mode == VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT ? MESA_TAG_REMARK_MODE_DEFAULT :
                                            conf->port.tag_remark_mode == VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED ? MESA_TAG_REMARK_MODE_MAPPED :
                                            MESA_TAG_REMARK_MODE_CLASSIFIED);
        mesa_conf.tag.egress_pcp = conf->port.tag_default_pcp;
        mesa_conf.tag.egress_dei = conf->port.tag_default_dei;

        mesa_conf.dmac_dip = conf->port.dmac_dip;

        mesa_conf.key_type = conf->port.key_type;

        mesa_conf.wred_group = conf->port.wred_group - 1; // Convert from 1-based to 0-based

        mesa_conf.cosid = conf->port.default_cosid;

#if defined(VTSS_SW_OPTION_QOS_ADV)
        /* DSCP */
        mesa_conf.dscp.class_enable = conf->port.trust_dscp;
        mesa_conf.dscp.translate = conf->port.dscp_translate;
        mesa_conf.dscp.mode = (conf->port.dscp_imode == VTSS_APPL_QOS_DSCP_MODE_ZERO ? MESA_DSCP_MODE_ZERO :
                               conf->port.dscp_imode == VTSS_APPL_QOS_DSCP_MODE_SEL ? MESA_DSCP_MODE_SEL :
                               conf->port.dscp_imode == VTSS_APPL_QOS_DSCP_MODE_ALL ? MESA_DSCP_MODE_ALL :
                               MESA_DSCP_MODE_NONE);
        mesa_conf.dscp.emode = (conf->port.dscp_emode == VTSS_APPL_QOS_DSCP_EMODE_REMARK ? MESA_DSCP_EMODE_REMARK :
                                conf->port.dscp_emode == VTSS_APPL_QOS_DSCP_EMODE_REMAP ? MESA_DSCP_EMODE_REMAP :
                                conf->port.dscp_emode == VTSS_APPL_QOS_DSCP_EMODE_REMAP_DPA ? MESA_DSCP_EMODE_REMAP_DPA :
                                MESA_DSCP_EMODE_DISABLE);

        mesa_conf.ingress_map = conf->port.ingress_map;
        mesa_conf.egress_map = conf->port.egress_map;

#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

        if ((rc = mesa_qos_port_conf_set(NULL, port_no, &mesa_conf)) != VTSS_RC_OK) {
            T_E("Error setting QoS port %u configuration - %s\n", port_no, error_txt(rc));
        } else {
            // On some chips, there is a bug that requires periodic calls into the
            // API in order to achieve accurate shaper rates. We need to kick-start
            // the timer that performs these periodic starts. It will auto-disable
            // itself whenever it's not required anymore.
            qos_shaper_calibration_timer_start();
        }
    }
}

/* Get port QoS configuration  */
static mesa_rc QOS_port_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_qos_port_conf_t *conf)
{
    QOS_CRIT_ASSERT_LOCKED();
    T_N("enter, isid: %u, port_no: %u", isid, port_no);

    if (QOS_port_isid_invalid(isid, port_no, 0)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    *conf = qos.qos_port_conf[isid][port_no - VTSS_PORT_NO_START];
    T_N("exit");
    return VTSS_RC_OK;
}

// Wrapper for converting from ifindex to isid,iport for configuration get
static mesa_rc QOS_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_conf_t *conf)
{
    vtss_ifindex_elm_t ife;

    if (!vtss_ifindex_is_port(ifindex)) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    return QOS_port_conf_get(ife.isid, ife.ordinal, conf);
}

/* Set port QoS configuration  */
static mesa_rc QOS_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_qos_port_conf_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;

    QOS_CRIT_ASSERT_LOCKED();
    T_N("enter, isid: %u, port_no: %u", isid, port_no);

    if (QOS_port_isid_invalid(isid, port_no, 1)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((rc = QOS_port_conf_check(conf)) != VTSS_RC_OK) {
        return rc;
    }

    if (vtss_memcmp(qos.qos_port_conf[isid][port_no - VTSS_PORT_NO_START], *conf)) {
        qos.qos_port_conf[isid][port_no - VTSS_PORT_NO_START] = *conf;
        QOS_stack_port_conf_set(isid, port_no);
    }
    T_N("exit");
    return rc;
}

// Wrapper for converting from ifindex to isid,iport for configuration set
static mesa_rc QOS_interface_conf_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_conf_t *conf)
{
    vtss_ifindex_elm_t ife;

    if (!vtss_ifindex_is_port(ifindex)) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    return QOS_port_conf_set(ife.isid, ife.ordinal, conf);
}

/* Validate QCE configuration */
static mesa_rc QOS_qce_check(mesa_qce_id_t next_qce_id, mesa_qce_t *qce)
{
    mesa_qce_key_t      *const k = &qce->key;
    mesa_qce_action_t   *const a = &qce->action;
    int                 i;
    BOOL                port_found = FALSE;

    /* Check ext QCE ID */
    if (next_qce_id > VTSS_APPL_QOS_QCE_ID_END) {
        T_I("Invalid qce_id: %u", next_qce_id);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    /* Check QCE ID */
    if (qce->id > VTSS_APPL_QOS_QCE_ID_END) {
        T_I("Invalid qce->id: %u", qce->id);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    /* Check next_qce_id != qce_id */
    if (next_qce_id != VTSS_APPL_QOS_QCE_ID_NONE && (next_qce_id == qce->id)) {
        T_I("next_qce_id (%u) == qce->id (%u)", next_qce_id, qce->id);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    /* Check that port_list contains at least one port */
    for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        if (k->port_list[i]) {
            port_found = TRUE;
            break;
        }
    }
    if (!port_found) {
        T_I("no ports in port_list");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (k->mac.dmac_mc > MESA_VCAP_BIT_1) {
        T_I("invalid dmac_mc (%d)", k->mac.dmac_mc);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (k->mac.dmac_bc > MESA_VCAP_BIT_1) {
        T_I("invalid dmac_bc (%d)", k->mac.dmac_bc);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (k->tag.dei > MESA_VCAP_BIT_1) {
        T_I("invalid tag.dei (%d)", k->tag.dei);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (k->tag.tagged > MESA_VCAP_BIT_1) {
        T_I("invalid tag.tagged (%d)", k->tag.tagged);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (k->tag.s_tag > MESA_VCAP_BIT_1) {
        T_I("invalid tag.s_tag (%d)", k->tag.s_tag);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (CAPA->has_qce_inner_tag) {
        if (k->inner_tag.dei > MESA_VCAP_BIT_1) {
            T_I("invalid inner_tag.dei (%d)", k->inner_tag.dei);
            return VTSS_APPL_QOS_ERROR_PARM;
        }

        if (k->inner_tag.tagged > MESA_VCAP_BIT_1) {
            T_I("invalid inner_tag.tagged (%d)", k->inner_tag.tagged);
            return VTSS_APPL_QOS_ERROR_PARM;
        }

        if (k->inner_tag.s_tag > MESA_VCAP_BIT_1) {
            T_I("invalid inner_tag.s_tag (%d)", k->inner_tag.s_tag);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }

    switch (k->type) {
    case MESA_QCE_TYPE_ANY:
    case MESA_QCE_TYPE_ETYPE:
    case MESA_QCE_TYPE_LLC:
    case MESA_QCE_TYPE_SNAP:
    case MESA_QCE_TYPE_IPV6:
        /* no extra checks here */
        break;
    case MESA_QCE_TYPE_IPV4:
        if (k->frame.ipv4.fragment > MESA_VCAP_BIT_1) {
            T_I("invalid ipv4.fragment (%d)", k->frame.ipv4.fragment);
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        break;
    default:
        T_I("invalid frame type (%d)", k->type);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    /* BZ#14304-14306: The table of "SNMP QceTableRowEditor" accepts invalid range input.
       Solution: Always check the valid range whatever the feature is enabled or not */
    if (/* a->prio_enable && */ a->prio >= VTSS_PRIO_END) {
        T_I("invalid cos (%u)", a->prio);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (/* a->dp_enable && */ a->dp > CAPA->dpl_max) {
        T_I("invalid dpl (%u)", a->dp);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (/* a->dscp_enable && */ a->dscp > 63) {
        T_I("invalid dscp (%u)", a->dscp);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (/* a->pcp_dei_enable && */ a->pcp > 7) {
        T_I("invalid pcp (%u)", a->pcp);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (/* a->pcp_dei_enable && */ a->dei > 1) {
        T_I("invalid dei (%u)", a->dei);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (/* a->policy_no_enable && */ a->policy_no >= fast_cap(MESA_CAP_ACL_POLICY_CNT)) {
        T_I("invalid policy_no (%u)", a->policy_no);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (CAPA->has_ingress_map && a->map_id > CAPA->ingress_map_id_max) {
        T_I("invalid map_id (%u)", a->map_id);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    return VTSS_RC_OK;
}

/* Add QCE to API after doing some required modifications */
static mesa_rc QOS_qce_add(mesa_qce_id_t next_qce_id, const mesa_qce_t *qce)
{
    mesa_qce_t qce_copy = *qce; // Take a copy we are allowed to modify
    port_iter_t pit;

    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT_ALL, PORT_ITER_FLAGS_ALL);
    while (port_iter_getnext(&pit)) {
        if (pit.type != PORT_ITER_TYPE_FRONT) {
            qce_copy.key.port_list[pit.iport] = FALSE; // Remove all non front ports
        }
    }

    return mesa_qce_add(NULL, next_qce_id, &qce_copy);
}

/* Resolve conflicting QCEs by trying to add them again */
static void QOS_qce_conflict_resolve(void)
{
    qcl_qce_list_table_t *list;
    qcl_qce_t           *qce, *qce_next;
    mesa_qce_id_t       next_id;

    QOS_CRIT_ASSERT_LOCKED();
    T_D("enter");

    list = &qos.qcl_qce_switch_list;
    /* multiple QCE can be added into H/W, specially if those use
       same range checker */
    for (qce = list->qce_used_list; qce != NULL; qce = qce->next) {
        if (qce->conf.conflict == TRUE) {
            /* Next_id (i.e. qce_id) should exist in ASIC i.e. find the first id
               starting from the current location id with non-conflict status */
            next_id = VTSS_APPL_QOS_QCE_ID_NONE;
            for (qce_next = qce->next; qce_next != NULL; qce_next = qce_next->next) {
                if (!qce_next->conf.conflict) {
                    next_id = qce_next->conf.qce.id;
                    break;
                }
            }

            /* Set this entry to ASIC layer */
            if (QOS_qce_add(next_id, &qce->conf.qce) == VTSS_RC_OK) {
                qce->conf.conflict = FALSE;
            }
        }
    }

    T_D("exit");
}

/* Add QCE to the local switch */
/**
 * \brief QOS_switch_qce_entry_add
 * Adds a qce entry to the local switch.
 *
 * \param next_qce_id [IN] The new qce will be added before this qce.
 * \param conf        [IN] The new qce to be added.
 * \param found       [IN] TRUE if entry is already in the list or FALSE if not exist.
 * \param conflict    [IN] TRUE if entry is already in the list with conflict state; FALSE otherwise.
 *
 * \return : Return code
 */
static void QOS_switch_qce_entry_add(mesa_qce_id_t next_qce_id, vtss_appl_qos_qce_intern_conf_t *conf, BOOL found, BOOL conflict)
{
    qcl_qce_list_table_t *list = &qos.qcl_qce_switch_list;
    qcl_qce_t *qce;

    QOS_CRIT_ASSERT_LOCKED();
    /*
     * If qce_id != QCE_ID_NONE, it should exist in ASIC.
     * Find the first qce_id starting from the mentioned id with non-conflict status
     */
    if (QCL_QCE_ID_GET(next_qce_id) != VTSS_APPL_QOS_QCE_ID_NONE) {
        BOOL          flag = FALSE;
        mesa_qce_id_t next_id = VTSS_APPL_QOS_QCE_ID_NONE;
        for (qce = list->qce_used_list; qce != NULL; qce = qce->next) {
            if (!flag && qce->conf.qce.id == next_qce_id) {
                flag = TRUE;
            }
            if (flag && !qce->conf.conflict) {
                next_id = qce->conf.qce.id;
                break;
            }
        }
        next_qce_id = next_id;
    }

    if (QOS_qce_add(next_qce_id, &conf->qce) == VTSS_RC_OK) {
        conf->conflict = FALSE;
    } else {
        /* Check if the entry already exists with non-conflict status.
           In that case if add fails, old entry should be deleted */
        if (found && !conflict) {
            if (mesa_qce_del(NULL, conf->qce.id) != VTSS_RC_OK) {
                T_W("Calling mesa_qce_del() failed\n");
            }
        }
        conf->conflict = TRUE;
    }

    /* Update QCE's conflict status in list */
    for (qce = list->qce_used_list; qce != NULL; qce = qce->next) {
        if (qce->conf.qce.id == conf->qce.id) { /* Found existing entry */
            qce->conf.conflict = conf->conflict;
            break;
        }
    }
}

/*
 * Local functions for manipulating qce lists.
 * They are modelled after the corresponding functions in the API
 */

/*!
 * \brief QOS_list_qce_entry_add
 *
 * Add a QCE entry to a list. If an entry with the same id exists, it is reused and probably moved.
 *
 * \param list        [IN]     The list to operate on.
 * \param next_qce_id [IN/OUT] Next QCE ID. The QCE will be added before next_qce_id or last if next_qce_id == QCE_ID_NONE.
 * \param conf        [IN]     The new QCE to be added. if conf->id == QCE_ID_NONE, it is assigned to the lowest unused id.
 * \param isid_del    [OUT]    Switch ID from where the QCE should be deleted.
 * \param found       [OUT]    TRUE if entry is already in the list or FALSE if not exist.
 * \param conflict    [OUT]    TRUE if entry is already in the list with conflict state; FALSE otherwise.
 * \param local       [IN]     TRUE if entry is added in the local (switch) list.
 *
 * \return : Return code
 */
/*lint -e{593} */
/* There is a lint error message: Custodial pointer 'new' possibly not freed or returned. We skip the lint error cause we freed it in QOS_list_qce_entry_del() */
static mesa_rc QOS_list_qce_entry_add(qcl_qce_list_table_t            *list,
                                      mesa_qce_id_t                   *next_qce_id,
                                      vtss_appl_qos_qce_intern_conf_t *conf,
                                      vtss_isid_t                     *isid_del,
                                      BOOL                            *found,
                                      BOOL                            *conflict,
                                      BOOL                            local)
{
    mesa_rc       rc = VTSS_RC_OK;
    qcl_qce_t     *qce, *prev, *ins = NULL, *ins_prev = NULL;
    qcl_qce_t     *new_ = NULL, *new_prev = NULL;
    uchar         *id_used;
    mesa_qce_id_t i;
    vtss_isid_t   isid;
    int           isid_count[VTSS_ISID_END + 1];
    BOOL          found_insertion_place = FALSE;

    QOS_CRIT_ASSERT_LOCKED();
    T_D("enter, next_qce_id: %u, qce_id: %u", *next_qce_id, conf->qce.id);

    if (VTSS_MALLOC_CAST(id_used, VTSS_APPL_QOS_QCE_ID_END) == NULL) {
        T_W("malloc failed");
        return VTSS_APPL_QOS_ERROR_GEN;
    }
    memset(id_used, 0, VTSS_APPL_QOS_QCE_ID_END);

    memset(isid_count, 0, sizeof(isid_count));
    isid_count[conf->isid]++;

    *found = FALSE;
    *conflict = FALSE;
    /* Search for existing entry and place to add */
    for (qce = list->qce_used_list, prev = NULL; qce != NULL; prev = qce, qce = qce->next) {
        T_N("check, isid: %u, qce_id: %u", qce->conf.isid, qce->conf.qce.id);
        if (qce->conf.qce.id == conf->qce.id) {
            /* Entry already exists */
            *found = TRUE;
            *conflict = qce->conf.conflict;
            new_ = qce;
            new_prev = prev;
            T_N("Entry exists");
        } else {
            isid_count[qce->conf.isid]++;
        }

        if (qce->conf.qce.id == *next_qce_id) {
            /* Found insertion point */
            ins = qce;
            ins_prev = prev;
            T_N("Found insertion point");
        } else if (QCL_QCE_ID_GET(*next_qce_id) == VTSS_APPL_QOS_QCE_ID_NONE) {
            if ((found_insertion_place == 0) && (QCL_USER_ID_GET(conf->qce.id) < QCL_USER_ID_GET(qce->conf.qce.id))) {
                /* Found insertion place by ordered priority */
                ins = qce;
                ins_prev = prev;
                found_insertion_place = 1;
                T_N("Found insertion point by ordered priority");
            }
        }

        /* Mark ID as used */
        if (QCL_USER_ID_GET(conf->qce.id) == QCL_USER_ID_GET(qce->conf.qce.id)) {
            id_used[QCL_QCE_ID_GET(qce->conf.qce.id) - VTSS_APPL_QOS_QCE_ID_START] = 1;
        }
    }

    if (QCL_QCE_ID_GET(*next_qce_id) == VTSS_APPL_QOS_QCE_ID_NONE && found_insertion_place == 0) {
        ins_prev = prev;
        T_N("ins_prev = prev");
    }

    /* Check if the place to insert was found */
    if (QCL_QCE_ID_GET(*next_qce_id) != VTSS_APPL_QOS_QCE_ID_NONE && ins == NULL) {
        T_I("qce_id: %d not found", *next_qce_id);
        rc = VTSS_APPL_QOS_ERROR_QCE_NOT_FOUND;
    }

    /* Check that the QCL is not full for any switch: Since for volatile QCE
       it keeps extra space, static entry is added through primary switch and it
       updates the primary switch stack list, switch list for any secondary
       switch can not be full unless primary switch stack list is full */
    if ((local == FALSE) && (QCL_USER_ID_GET(conf->qce.id) == VTSS_APPL_QOS_QCL_USER_STATIC)) {
        for (isid = VTSS_ISID_START; (rc == VTSS_RC_OK) && (isid < VTSS_ISID_END); isid++) {
            if ((isid_count[isid] + isid_count[VTSS_ISID_GLOBAL]) > VTSS_APPL_QOS_QCE_MAX) {
                T_W("table is full, isid %d", isid);
                rc = VTSS_APPL_QOS_ERROR_QCE_TABLE_FULL;
            }
        }
    }

    if (rc == VTSS_RC_OK) {
        T_D("using from %s list", new_ == NULL ? "free" : "used");
        *isid_del = VTSS_ISID_LOCAL;
        if (new_ == NULL) {
            /* Use first entry in free list */
            new_ = list->qce_free_list;
            /* 'new_' can not be NULL as qce_free_list never be NULL */
            if (new_ == NULL) {
                T_W("QCE table is full, isid %d", conf->isid);
                rc = VTSS_APPL_QOS_ERROR_QCE_TABLE_FULL;
                goto exit;
            }
            new_->conf.conflict = FALSE;
            list->qce_free_list = new_->next;
        } else {
            /* Take existing entry out of used list */
            if (ins_prev == new_) {
                ins_prev = new_prev;
            }
            if (new_prev == NULL) {
                list->qce_used_list = new_->next;
            } else {
                new_prev->next = new_->next;
            }

            /* If changing to a specific SID, delete QCE on old SIDs */
            if (new_->conf.isid != conf->isid && conf->isid != VTSS_ISID_GLOBAL) {
                *isid_del = new_->conf.isid;
            }
        }

        /* assign ID */
        if (QCL_QCE_ID_GET(conf->qce.id) == VTSS_APPL_QOS_QCE_ID_NONE) {
            /* Use next available ID */
            for (i = VTSS_APPL_QOS_QCE_ID_START; i <= VTSS_APPL_QOS_QCE_ID_END; i++) {
                if (!id_used[i - VTSS_APPL_QOS_QCE_ID_START]) {
                    conf->qce.id =  QCL_COMBINED_ID_SET(QCL_USER_ID_GET(conf->qce.id), i);;
                    break;
                }
            }
            if (i > VTSS_APPL_QOS_QCE_ID_END) {
                /* This will never happen */
                T_W("QCE Auto-assigned fail");
                rc = VTSS_APPL_QOS_ERROR_QCE_TABLE_FULL;
                goto exit;
            }
        }

        conf->conflict = new_->conf.conflict;
        new_->conf = *conf;

        if (ins_prev == NULL) {
            T_D("inserting first");
            new_->next = list->qce_used_list;
            list->qce_used_list = new_;
        } else {
            T_D("inserting after ID %d", ins_prev->conf.qce.id);
            new_->next = ins_prev->next;
            ins_prev->next = new_;
        }

        /* Update the next_id */
        *next_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
        if (new_->next) {
            *next_qce_id = new_->next->conf.qce.id;
        }
        T_D("exit, next_qce_id: %u, qce_id: %u", *next_qce_id, conf->qce.id);
    }

exit:
    VTSS_FREE(id_used);
    return rc;
}

/*!
 * \brief QOS_list_qce_entry_del
 *
 * Delete a QCE entry from a list.
 *
 * \param list     [IN]  The list to operate on.
 * \param qce_id   [IN]  The QCE to delete. NOTE: This is a combined id!
 * \param isid_del [OUT] The isid contained in the deleted entry
 * \param conflict [OUT] True if the deleted entry had a conflict
 *
 * \return : Return code
 */
static mesa_rc QOS_list_qce_entry_del(qcl_qce_list_table_t *list,
                                      mesa_qce_id_t        qce_id,
                                      vtss_isid_t          *isid_del,
                                      BOOL                 *conflict)
{
    qcl_qce_t *qce, *prev;
    mesa_rc   rc = VTSS_APPL_QOS_ERROR_QCE_NOT_FOUND;

    QOS_CRIT_ASSERT_LOCKED();
    T_D("enter, qce_id: %u", qce_id);

    for (qce = list->qce_used_list, prev = NULL; qce != NULL; prev = qce, qce = qce->next) {
        T_N("check, isid: %u, user_id: %u, qce_id: %u", qce->conf.isid, qce->conf.user_id, qce->conf.qce.id);
        if (qce->conf.qce.id == qce_id) {
            *isid_del = qce->conf.isid;
            *conflict = qce->conf.conflict;

            /* Remove entry from used list */
            if (prev == NULL) {
                list->qce_used_list = qce->next;
            } else {
                prev->next = qce->next;
            }

            /* Add entry to free list */
            qce->next = list->qce_free_list;
            list->qce_free_list = qce;

            rc = VTSS_RC_OK;
            break;
        }
    }

    T_D("exit, qce_id: %u %s", qce_id, (rc == VTSS_RC_OK) ? "deleted" : "not found");
    return rc;
}

/*!
 * \brief QOS_list_qce_entry_get
 *
 * Get a QCE entry from a list.
 *
 * \param isid    [IN]  Internal switch ID.
 * \param qce_id  [IN]  QCE ID. If QCE_ID_NONE, the first existing entry is returned and next is ignored.
 * \param conf    [OUT] The QCE.
 * \param next    [IN]  Next. If TRUE, get next QCE after qce_id. Otherwise get specific QCE
 *
 * \return : Return code
 */
static mesa_rc QOS_list_qce_entry_get(vtss_isid_t                     isid,
                                      mesa_qce_id_t                   qce_id,
                                      vtss_appl_qos_qce_intern_conf_t *conf,
                                      BOOL                            next)
{
    qcl_qce_t            *qce;
    qcl_qce_list_table_t *list;
    BOOL                 return_next = 0;
    mesa_rc              rc = VTSS_APPL_QOS_ERROR_QCE_NOT_FOUND;

    T_D("enter, isid: %u, qce_id: %u, next: %d", isid, qce_id, next);

    QOS_CRIT_ENTER();
    list = (isid == VTSS_ISID_LOCAL ? &qos.qcl_qce_switch_list : &qos.qcl_qce_stack_list);
    for (qce = list->qce_used_list; qce != NULL; qce = qce->next) {
        T_N("check, isid: %u, qce_id: %u", qce->conf.isid, qce->conf.qce.id);

        if ((VTSS_ISID_LEGAL(isid) && qce->conf.isid != isid)) {
            continue; /* Skip QCEs for other switches */
        }

        if (QCL_USER_ID_GET(qce->conf.qce.id) != QCL_USER_ID_GET(qce_id)) {
            continue; /* Skip QCEs for other user id's */
        }

        if (QCL_QCE_ID_GET(qce_id) == VTSS_APPL_QOS_QCE_ID_NONE) {
            break; /* Return first found QCE in list */
        }

        if (return_next) {
            break; /* Return next QCE */
        }

        if (qce->conf.qce.id == qce_id) { /* Found matching QCE */
            if (next) {
                return_next = 1; /* Prepare to return next QCE (if any) */
            } else {
                break; /* Return this QCE */
            }
        }
    }

    if (qce) { /* Found a QCE */
        *conf = qce->conf;
        rc = VTSS_RC_OK;
        T_D("exit, user_id %u, qce_id %u found", QCL_USER_ID_GET(qce->conf.qce.id), QCL_QCE_ID_GET(qce->conf.qce.id));
    } else {
        T_D("exit, user_id %u, qce_id %s%u NOT found", QCL_USER_ID_GET(qce_id), (next) ? "after " : "", QCL_QCE_ID_GET(qce_id));
    }

    QOS_CRIT_EXIT();

    return rc;
}

/*!
* \brief Get QCE configuration/status by n'th index starting from 1.
*
* \param isid    [IN]  Internal switch ID.
* \param index   [IN]  Index in list (1..n).
* \param conf    [OUT] The QCE.
*
* \return Return code.
**/
static mesa_rc QOS_list_qce_entry_get_nth(vtss_isid_t                     isid,
                                          u32                             ix,
                                          vtss_appl_qos_qce_intern_conf_t *conf)
{
    qcl_qce_list_table_t *list;
    qcl_qce_t            *qce;
    u32                  count = 0;
    mesa_rc              rc = VTSS_APPL_QOS_ERROR_QCE_NOT_FOUND;

    T_D("enter, isid: %u, index: %u", isid, ix);

    QOS_CRIT_ENTER();
    list = (isid == VTSS_ISID_LOCAL ? &qos.qcl_qce_switch_list : &qos.qcl_qce_stack_list);
    for (qce = list->qce_used_list; qce != NULL; qce = qce->next) {
        if (++count == ix) { /* Bingo */
            *conf = qce->conf;
            rc = VTSS_RC_OK;
            break;
        }
    }
    QOS_CRIT_EXIT();

    if (qce) {
        T_D("exit, user %u, id %u found", QCL_USER_ID_GET(qce->conf.qce.id), QCL_QCE_ID_GET(qce->conf.qce.id));
    } else {
        T_D("exit, NOT found");
    }

    return rc;
}

/**
 * \brief QOS_list_qce_get_next_id
 *
 * Get next QCE ID from global stack list in ID order.
 * This is only valid on a primary switch.
 *
 * \param id      [IN]   The ID to get the next from.
 * \param id_next [OUT]  Next ID.
 *
 * \return : Return code
 */
mesa_rc QOS_list_qce_get_next_id(mesa_qce_id_t id, mesa_qce_id_t *id_next)
{
    mesa_rc       rc = VTSS_RC_ERROR;
    qcl_qce_t     *qce;
    mesa_qce_id_t cur_id;

    *id_next = 0;

    /* Enter critical region */
    QOS_CRIT_ENTER();

    T_D("id: %u", id);

    for (qce = qos.qcl_qce_switch_list.qce_used_list; qce != NULL; qce = qce->next) {
        if ((cur_id = qce->conf.qce.id) > id) {
            /* Found greater ID */
            if (rc == VTSS_RC_ERROR || cur_id < *id_next) {
                *id_next = cur_id;
            }
            rc = VTSS_RC_OK;
        }
    }

    /* Exit critical region */
    QOS_CRIT_EXIT();

    return rc;
}

/****************************************************************************/
/*  Shaper calibration support                                              */
/****************************************************************************/

/* Shaper calibration timer callback function */
static void QOS_shaper_calibrate_timeout(vtss::Timer *timer)
{
    mesa_rc rc = mesa_qos_shaper_calibrate(NULL);

    switch (rc) {
    case MESA_RC_OK:
        // The shaper calibration function no longer needs periodic invocations.
        T_D("Cancelling shaper calibration timer");
        vtss_timer_cancel(timer);
        break;

    case MESA_RC_INCOMPLETE:
        // The shaper calibration function still needs periodic invocations.
        T_R("Shaper calibration timer still needs to run");
        break;

    default:
        T_E("mesa_qos_shaper_calibrate() returned rc = %d = %s", rc, error_txt(rc));
        break;
    }
}

/* Start shaper calibration timer */
void qos_shaper_calibration_timer_start(void)
{
    if (!fast_cap(MESA_CAP_QOS_SHAPER_CALIBRATE)) {
        return;
    }

    T_D("enter");
    (void)vtss_timer_cancel(&qos.shaper_calibrate_timer);
    if (vtss_timer_start(&qos.shaper_calibrate_timer) != VTSS_RC_OK) {
        T_E("Unable to start timer");
    }
}

/* Initialize shaper calibration timer */
static void QOS_shaper_calibrate_init(void)
{
    T_D("enter");

    qos.shaper_calibrate_timer.modid     = VTSS_MODULE_ID_QOS;
    qos.shaper_calibrate_timer.set_repeat(TRUE);  // Never-ending.
    qos.shaper_calibrate_timer.set_period(vtss::milliseconds(20)); // 50 times per second.
    qos.shaper_calibrate_timer.callback  = QOS_shaper_calibrate_timeout;
}

/****************************************************************************/
/*  Status update support                                                   */
/****************************************************************************/
/* Qos status timer callback function */
static void QOS_status_update_timeout(vtss::Timer *timer)
{
    mesa_qos_status_t status_api;
    T_N("enter");
    (void)mesa_qos_status_get(NULL, &status_api);
    QOS_CRIT_ENTER();
    qos.qos_status.storm_active = status_api.storm;
    QOS_CRIT_EXIT();
    qos_status_update.set(&qos.qos_status);
    T_N("exit");
}

/* Start Qos status timer */
static void QOS_status_update_start(void)
{
    T_D("enter");
    (void)vtss_timer_cancel(&qos.status_update_timer);
    if (vtss_timer_start(&qos.status_update_timer) != VTSS_RC_OK) {
        T_E("Unable to start timer");
    }
}

/* Initialize Qos status timer */
static void QOS_status_update_init(void)
{
    T_D("enter");
    qos.status_update_timer.modid = VTSS_MODULE_ID_QOS;
    qos.status_update_timer.set_repeat(TRUE);  // Never-ending.
    qos.status_update_timer.set_period(vtss::seconds(SECONDS_BETWEEN_STATUS_UPDATE));
    qos.status_update_timer.callback = QOS_status_update_timeout;
}

/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/

/* Send QOS configuration to API */
static void QOS_stack_conf_set(vtss_isid_t isid_add)
{
    vtss_appl_qos_conf_t *conf;

    QOS_CRIT_ASSERT_LOCKED();
    T_D("enter, isid: %d", isid_add);

    conf = &qos.qos_conf;
    QOS_conf_apply(conf);
    T_D("exit");
}

/* Send QOS port configuration to API */
/* port_no == VTSS_PORT_NO_NONE means all ports */
static void QOS_stack_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no)
{
    vtss_appl_qos_port_conf_t *qos_port_conf;


    // Lint is confused by the unlock/lock of critd here, so we help it a little.
    /*lint --e{454, 455, 456} */
    QOS_CRIT_ASSERT_LOCKED();
    T_D("enter, isid: %d", isid);

    if (port_no == VTSS_PORT_NO_NONE) { /* all ports */
        port_iter_t pit;
        (void) port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            qos_port_conf = &qos.qos_port_conf[VTSS_ISID_LOCAL][pit.iport - VTSS_PORT_NO_START]; // Point to local storage and
            *qos_port_conf = qos.qos_port_conf[VTSS_ISID_START][pit.iport - VTSS_PORT_NO_START]; // make a temporary modifiable copy
            /* override default prio if volatile is set */
            if (qos.volatile_default_prio[pit.iport - VTSS_PORT_NO_START] != VTSS_PRIO_NO_NONE) {
                qos_port_conf->port.default_cos = qos.volatile_default_prio[pit.iport - VTSS_PORT_NO_START];
            }
            QOS_CRIT_EXIT(); // Temporary leave critical section before doing callbacks
            QOS_port_conf_change_event(VTSS_ISID_START, pit.iport, qos_port_conf); // Call global callbacks
            QOS_CRIT_ENTER(); // Enter critical section again
            QOS_port_conf_apply(pit.iport, qos_port_conf);
        }

    } else { /* specific port */
        if (port_is_front_port(port_no)) {
            qos_port_conf = &qos.qos_port_conf[VTSS_ISID_LOCAL][port_no - VTSS_PORT_NO_START]; // Point to local storage and
            *qos_port_conf = qos.qos_port_conf[VTSS_ISID_START][port_no - VTSS_PORT_NO_START]; // make a temporary modifiable copy
            /* override default prio if volatile is set */
            if (qos.volatile_default_prio[port_no - VTSS_PORT_NO_START] != VTSS_PRIO_NO_NONE) {
                qos_port_conf->port.default_cos = qos.volatile_default_prio[port_no - VTSS_PORT_NO_START];
            }
            QOS_CRIT_EXIT(); // Temporary leave critical section before doing callbacks
            QOS_port_conf_change_event(VTSS_ISID_START, port_no, qos_port_conf); // Call global callbacks
            QOS_CRIT_ENTER(); // Enter critical section again
            QOS_port_conf_apply(port_no, qos_port_conf);
        }
    }
    T_D("exit");
}

/* Add QCE to the stack */
static void QOS_stack_qce_entry_add(vtss_isid_t isid_add, vtss_appl_qos_qce_intern_conf_t *conf, BOOL is_next)
{
    qcl_qce_t            *qce, *stack_pos = NULL;
    mesa_qce_id_t        next_qce_id;
    qcl_qce_list_table_t *list;
    mesa_rc              rc;
    vtss_isid_t          isid_del;
    BOOL                 found;
    BOOL                 conflict;

    QOS_CRIT_ASSERT_LOCKED();
    T_N("enter, isid: %d", isid_add);
    /* find the conf location in used_list, this should be the starting point
       to look next qce for a stack */
    if (is_next) {
        for (stack_pos = qos.qcl_qce_stack_list.qce_used_list;
             stack_pos != NULL; stack_pos = stack_pos->next) {
            if (stack_pos->conf.qce.id == conf->qce.id) {
                break;
            }
        }
    }

    /* default next qce is NONE */
    next_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
    /* start from conf location, find first qce for isid */
    if (is_next && stack_pos) {
        for (qce = stack_pos->next; qce != NULL; qce = qce->next) {
            if (qce->conf.isid == VTSS_ISID_GLOBAL || qce->conf.isid == isid_add) {
                next_qce_id = qce->conf.qce.id;
                break;
            }
        }
    }

    list = &qos.qcl_qce_switch_list;
    rc = QOS_list_qce_entry_add(list, &next_qce_id, conf, &isid_del, &found, &conflict, 1);
    if (rc == VTSS_RC_OK) {
        QOS_switch_qce_entry_add(next_qce_id, conf, found, conflict);
    }
    T_N("exit");
}

/* Delele QCE in the stack */
static void QOS_stack_qce_entry_del(vtss_isid_t isid_add, mesa_qce_id_t qce_id)
{
    qcl_qce_list_table_t *list;
    vtss_isid_t          dummy;
    BOOL                 conflict;
    mesa_rc              rc;

    QOS_CRIT_ASSERT_LOCKED();
    T_N("enter, isid: %d, qce_id: %u", isid_add, qce_id);

    list = &qos.qcl_qce_switch_list;
    rc = QOS_list_qce_entry_del(list, qce_id, &dummy, &conflict);

    if (rc == VTSS_RC_OK && conflict == FALSE) {
        /* Remove this entry from ASIC layer */
        if (mesa_qce_del(NULL, qce_id) != VTSS_RC_OK) {
            T_W("Calling mesa_qce_del() failed\n");
        }
        QOS_qce_conflict_resolve();
    }
    T_N("exit");
}

/* Clear all QCE configuration on a specific switch */
static void QOS_stack_qce_clear_all(vtss_isid_t isid_add)
{
    qcl_qce_t            *qce;
    qcl_qce_list_table_t *list;

    QOS_CRIT_ASSERT_LOCKED();
    T_N("enter, isid: %d", isid_add);

    list = &qos.qcl_qce_switch_list;
    while (list->qce_used_list != NULL) {
        qce = list->qce_used_list;
        list->qce_used_list = qce->next;
        if (qce->conf.conflict == FALSE) {
            /* Remove this entry from ASIC layer */
            if (mesa_qce_del(NULL, qce->conf.qce.id) != VTSS_RC_OK) {
                T_W("Calling mesa_qce_del() failed\n");
            }
        }
        /* Move to free list */
        qce->next = list->qce_free_list;
        list->qce_free_list = qce;
    }
    T_N("exit");
}

/* Resolve conflicts on all switches in the stack */
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
static mesa_rc QOS_stack_qce_conflict_resolve(void)
{
    T_N("enter");
    QOS_qce_conflict_resolve();
    T_N("exit");
    return VTSS_RC_OK;
}
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */

/* Determine if port and isid are valid */
static BOOL QOS_port_isid_invalid(vtss_isid_t isid, mesa_port_no_t port_no, BOOL set)
{
    /* Check ISID */
    if (isid >= VTSS_ISID_END) {
        T_I("illegal isid: %d", isid);
        return 1;
    }
    if (port_no >= port_count_max()) {
        T_I("illegal port_no: %u, isid: %u", port_no, isid);
        return 1;
    }

    if (set && isid == VTSS_ISID_LOCAL) {
        T_I("SET not allowed, isid: %d", isid);
        return 1;
    }

    return 0;
}

/* Determine if isid is valid */
static BOOL QOS_isid_invalid(vtss_isid_t isid)
{
    if (isid > VTSS_ISID_END) {
        T_I("illegal isid: %d", isid);
        return 1;
    }

    return 0;
}

#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
static vtss_appl_qos_qce_type_t QOS_qce_type_api2appl(mesa_qce_type_t qce_type)
{
    return (qce_type == MESA_QCE_TYPE_ETYPE ? VTSS_APPL_QOS_QCE_TYPE_ETYPE :
            qce_type == MESA_QCE_TYPE_LLC   ? VTSS_APPL_QOS_QCE_TYPE_LLC   :
            qce_type == MESA_QCE_TYPE_SNAP  ? VTSS_APPL_QOS_QCE_TYPE_SNAP  :
            qce_type == MESA_QCE_TYPE_IPV4  ? VTSS_APPL_QOS_QCE_TYPE_IPV4  :
            qce_type == MESA_QCE_TYPE_IPV6  ? VTSS_APPL_QOS_QCE_TYPE_IPV6  :
            VTSS_APPL_QOS_QCE_TYPE_ANY);
}

/*!
 * \brief Convert intern_conf configuration to conf.
 *
 * NOTE: Always update conf->next_qce_id after this function is called
 *
 * \param iconf  [IN]  The intern conf configuration.
 * \param conf   [OUT] The conf configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
static mesa_rc QOS_qce_intern_conf2conf(const vtss_appl_qos_qce_intern_conf_t *iconf, vtss_appl_qos_qce_conf_t *conf)
{
    int                     i;
    const mesa_qce_key_t    *const ik = &iconf->qce.key;
    vtss_appl_qos_qce_key_t *const ck = &conf->key;

    memset(conf, 0, sizeof(*conf));

    conf->qce_id = iconf->qce.id;
    conf->usid = topo_isid2usid(iconf->isid);
    conf->user_id = iconf->user_id;

    // port_list
    vtss::PortListStackable &pls = (vtss::PortListStackable &)ck->port_list;
    pls.clear_all();
    for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        if (ik->port_list[i]) {
            pls.set(VTSS_ISID_START, i);
        }
    }

    // dmac
    if ((ik->mac.dmac_bc != MESA_VCAP_BIT_ANY) || (ik->mac.dmac_mc != MESA_VCAP_BIT_ANY)) {
        ck->mac.dmac_type = (ik->mac.dmac_bc == MESA_VCAP_BIT_1 ? VTSS_APPL_VCAP_DMAC_TYPE_BROADCAST :
                             ik->mac.dmac_mc == MESA_VCAP_BIT_1 ? VTSS_APPL_VCAP_DMAC_TYPE_MULTICAST :
                             ik->mac.dmac_mc == MESA_VCAP_BIT_0 ? VTSS_APPL_VCAP_DMAC_TYPE_UNICAST :
                             VTSS_APPL_VCAP_DMAC_TYPE_ANY);
    }
    if (ck->mac.dmac_type == VTSS_APPL_VCAP_DMAC_TYPE_ANY) {
        memcpy(&ck->mac.dmac.value, ik->mac.dmac.value, sizeof(ck->mac.dmac.value));
        memcpy(&ck->mac.dmac.mask,  ik->mac.dmac.mask,  sizeof(ck->mac.dmac.mask));
    }

    // smac
    memcpy(&ck->mac.smac.value, ik->mac.smac.value, sizeof(ck->mac.smac.value));
    memcpy(&ck->mac.smac.mask,  ik->mac.smac.mask,  sizeof(ck->mac.smac.mask));

    // tag
    ck->tag.tag_type = vtss_appl_vcap_tag_bits2tag_type(ik->tag.tagged, ik->tag.s_tag);
    vtss_appl_vcap_vr2asr(&ik->tag.vid, &ck->tag.vid);
    ck->tag.pcp = vtss_appl_vcap_vlan_pri2pri_type(ik->tag.pcp);
    T_D("pcp.v: %d, pcp.m: %d -> pcp: %d", ik->tag.pcp.value, ik->tag.pcp.mask, ck->tag.pcp);
    ck->tag.dei = ik->tag.dei;

    // inner tag
    ck->inner_tag.tag_type = vtss_appl_vcap_tag_bits2tag_type(ik->inner_tag.tagged, ik->inner_tag.s_tag);
    vtss_appl_vcap_vr2asr(&ik->inner_tag.vid, &ck->inner_tag.vid);
    ck->inner_tag.pcp = vtss_appl_vcap_vlan_pri2pri_type(ik->inner_tag.pcp);
    ck->inner_tag.dei = ik->inner_tag.dei;

    // frame type
    ck->type = QOS_qce_type_api2appl(ik->type);

    switch (ik->type) {
    case MESA_QCE_TYPE_ETYPE: {
        const mesa_qce_frame_etype_t    *const etype   = &ik->frame.etype;
        vtss_appl_qos_qce_frame_etype_t *const etype_2 = &ck->frame.etype;

        if (etype->etype.mask[0] || etype->etype.mask[1]) {
            etype_2->etype = (etype->etype.value[0] << 8) | etype->etype.value[1];
        }
        break;
    }
    case MESA_QCE_TYPE_LLC: {
        const mesa_qce_frame_llc_t    *const llc   = &ik->frame.llc;
        vtss_appl_qos_qce_frame_llc_t *const llc_2 = &ck->frame.llc;

        llc_2->dsap.value    = llc->data.value[0];
        llc_2->dsap.mask     = llc->data.mask[0];
        llc_2->ssap.value    = llc->data.value[1];
        llc_2->ssap.mask     = llc->data.mask[1];
        llc_2->control.value = llc->data.value[2];
        llc_2->control.mask  = llc->data.mask[2];
        break;
    }
    case MESA_QCE_TYPE_SNAP: {
        const mesa_qce_frame_snap_t    *const snap   = &ik->frame.snap;
        vtss_appl_qos_qce_frame_snap_t *const snap_2 = &ck->frame.snap;

        snap_2->pid.value = (snap->data.value[3] << 8) | snap->data.value[4];
        snap_2->pid.mask  = (snap->data.mask[3] << 8)  | snap->data.mask[4];
        break;
    }
    case MESA_QCE_TYPE_IPV4: {
        const mesa_qce_frame_ipv4_t    *const i4   = &ik->frame.ipv4;
        vtss_appl_qos_qce_frame_ipv4_t *const i4_2 = &ck->frame.ipv4;

        i4_2->fragment = i4->fragment;
        vtss_appl_vcap_vr2asr(&i4->dscp, &i4_2->dscp);
        i4_2->proto = i4->proto;
        i4_2->sip = i4->sip;
        i4_2->dip = i4->dip;
        vtss_appl_vcap_vr2asr(&i4->sport, &i4_2->sport);
        vtss_appl_vcap_vr2asr(&i4->dport, &i4_2->dport);
        break;
    }
    case MESA_QCE_TYPE_IPV6: {
        const mesa_qce_frame_ipv6_t    *const i6   = &ik->frame.ipv6;
        vtss_appl_qos_qce_frame_ipv6_t *const i6_2 = &ck->frame.ipv6;

        vtss_appl_vcap_vr2asr(&i6->dscp, &i6_2->dscp);
        i6_2->proto = i6->proto;
        for (i = 0; i < 16; i++) {
            i6_2->sip.value.addr[i] = i6->sip.value[i];
            i6_2->sip.mask.addr[i]  = i6->sip.mask[i];
            i6_2->dip.value.addr[i] = i6->dip.value[i];
            i6_2->dip.mask.addr[i]  = i6->dip.mask[i];
        }
        vtss_appl_vcap_vr2asr(&i6->sport, &i6_2->sport);
        vtss_appl_vcap_vr2asr(&i6->dport, &i6_2->dport);
        break;
    }
    default:
        break;
    }

    // action
    conf->action.prio_enable      = iconf->qce.action.prio_enable;
    conf->action.prio             = iconf->qce.action.prio;
    conf->action.dp_enable        = iconf->qce.action.dp_enable;
    conf->action.dp               = iconf->qce.action.dp;
    conf->action.dscp_enable      = iconf->qce.action.dscp_enable;
    conf->action.dscp             = iconf->qce.action.dscp;
    conf->action.pcp_dei_enable   = iconf->qce.action.pcp_dei_enable;
    conf->action.pcp              = iconf->qce.action.pcp;
    conf->action.dei              = iconf->qce.action.dei;
    conf->action.policy_no_enable = iconf->qce.action.policy_no_enable;
    conf->action.policy_no        = iconf->qce.action.policy_no;
    conf->action.map_id_enable    = iconf->qce.action.map_id_enable;
    conf->action.map_id           = iconf->qce.action.map_id;

    // conflict
    conf->conflict = iconf->conflict;

    return VTSS_RC_OK;
}

static mesa_qce_type_t QOS_qce_type_appl2api(vtss_appl_qos_qce_type_t qce_type)
{
    return (qce_type == VTSS_APPL_QOS_QCE_TYPE_ETYPE ? MESA_QCE_TYPE_ETYPE :
            qce_type == VTSS_APPL_QOS_QCE_TYPE_LLC   ? MESA_QCE_TYPE_LLC   :
            qce_type == VTSS_APPL_QOS_QCE_TYPE_SNAP  ? MESA_QCE_TYPE_SNAP  :
            qce_type == VTSS_APPL_QOS_QCE_TYPE_IPV4  ? MESA_QCE_TYPE_IPV4  :
            qce_type == VTSS_APPL_QOS_QCE_TYPE_IPV6  ? MESA_QCE_TYPE_IPV6  :
            MESA_QCE_TYPE_ANY);
}

/*!
 * \brief Convert conf configuration to intern_conf.
 *
 * \param conf  [IN]  The conf configuration.
 * \param iconf [OUT] The intern conf  configuration.
 * \param clear [IN]  Clear all frame keys.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
static mesa_rc QOS_qce_conf2intern_conf(const vtss_appl_qos_qce_conf_t *conf, vtss_appl_qos_qce_intern_conf_t *iconf, BOOL clear)
{
    int                            i;
    const vtss_appl_qos_qce_key_t  *const mk = &conf->key;
    mesa_qce_key_t                 *const k  = &iconf->qce.key;
    mesa_qce_action_t              *const a  = &iconf->qce.action;
    static const qos_frame_union_t zffu = {{0}};                                  /* Zero Filled Frame Union for all zeros comparison */
    static const mesa_mac_t        zfm  = {{0}};                                  /* Zero Filled Mac for all zeros comparison */
    static const mesa_mac_t        ofm  = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}; /* One Filled Mac for all ones comparison */

    VTSS_RC(vtss_appl_qos_qce_intern_get_default(iconf));

    iconf->isid = topo_usid2isid(conf->usid);
    iconf->user_id = conf->user_id;
    iconf->qce.id = conf->qce_id;

    // port_list
    vtss::PortListStackable &pls = (vtss::PortListStackable &)mk->port_list;
    memset(k->port_list, 0, sizeof(k->port_list));
    for (const auto &ifindex : pls) {
        vtss_ifindex_elm_t ife;
        VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
        if ((ife.iftype == VTSS_IFINDEX_TYPE_PORT) && (ife.usid == 1) && (ife.ordinal < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT))) {
            k->port_list[ife.ordinal] = TRUE;
        }
    }

    // dmac
    switch (mk->mac.dmac_type) {
    case VTSS_APPL_VCAP_DMAC_TYPE_UNICAST:
        k->mac.dmac_bc = MESA_VCAP_BIT_0;
        k->mac.dmac_mc = MESA_VCAP_BIT_0;
        break;
    case VTSS_APPL_VCAP_DMAC_TYPE_MULTICAST:
        k->mac.dmac_bc = MESA_VCAP_BIT_0;
        k->mac.dmac_mc = MESA_VCAP_BIT_1;
        break;
    case VTSS_APPL_VCAP_DMAC_TYPE_BROADCAST:
        k->mac.dmac_bc = MESA_VCAP_BIT_1;
        k->mac.dmac_mc = MESA_VCAP_BIT_1;
        break;
    default:
        if (CAPA->has_qce_dmac) {
            if (memcmp(&mk->mac.dmac.mask, &zfm, sizeof(mk->mac.dmac.mask)) &&
                memcmp(&mk->mac.dmac.mask, &ofm, sizeof(mk->mac.dmac.mask))) {
                T_I("Invalid DMAC mask");
                return VTSS_RC_ERROR;
            }
            memcpy(k->mac.dmac.value, &mk->mac.dmac.value, sizeof(k->mac.dmac.value));
            memcpy(k->mac.dmac.mask,  &mk->mac.dmac.mask,  sizeof(k->mac.dmac.mask));
        }
        break;
    }

    // smac
    if (memcmp(&mk->mac.smac.mask, &zfm, sizeof(mk->mac.smac.mask)) &&
        memcmp(&mk->mac.smac.mask, &ofm, sizeof(mk->mac.smac.mask))) {
        T_I("Invalid SMAC mask");
        return VTSS_RC_ERROR;
    }
    memcpy(k->mac.smac.value, &mk->mac.smac.value, sizeof(k->mac.smac.value));
    memcpy(k->mac.smac.mask,  &mk->mac.smac.mask,  sizeof(k->mac.smac.mask));

    // tag
    if ((mk->tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_ANY) &&
        (mk->tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_UNTAGGED) &&
        (mk->tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_TAGGED) &&
        (mk->tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_C_TAGGED) &&
        (mk->tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_S_TAGGED)) {
        T_I("Invalid tag.tag_type");
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    vtss_appl_vcap_tag_type2tag_bits(mk->tag.tag_type, &k->tag.tagged, &k->tag.s_tag);
    VTSS_RC(vtss_appl_vcap_asr2vr(&mk->tag.vid, &k->tag.vid, 0xFFF));
    vtss_appl_vcap_pri_type2vlan_pri(mk->tag.pcp, &k->tag.pcp);
    T_D("pcp: %d -> pcp.v: %d, pcp.m: %d", mk->tag.pcp, k->tag.pcp.value, k->tag.pcp.mask);
    k->tag.dei = mk->tag.dei;

    // inner tag
    if (CAPA->has_qce_inner_tag) {
        if ((mk->inner_tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_ANY) &&
            (mk->inner_tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_UNTAGGED) &&
            (mk->inner_tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_TAGGED) &&
            (mk->inner_tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_C_TAGGED) &&
            (mk->inner_tag.tag_type != VTSS_APPL_VCAP_VLAN_TAG_TYPE_S_TAGGED)) {
            T_I("Invalid inner_tag.tag_type");
            return VTSS_APPL_QOS_ERROR_PARM;
        }
        vtss_appl_vcap_tag_type2tag_bits(mk->inner_tag.tag_type, &k->inner_tag.tagged, &k->inner_tag.s_tag);
        VTSS_RC(vtss_appl_vcap_asr2vr(&mk->inner_tag.vid, &k->inner_tag.vid, 0xFFF));
        vtss_appl_vcap_pri_type2vlan_pri(mk->inner_tag.pcp, &k->inner_tag.pcp);
        k->inner_tag.dei = mk->inner_tag.dei;
    }

    // frame type
    k->type = QOS_qce_type_appl2api(mk->type);

    if (!clear) { // Copy frame keys also
        // Check for frame keys in other frame types that the current one
        if ((mk->type != VTSS_APPL_QOS_QCE_TYPE_ETYPE) && memcmp(&mk->frame.etype, &zffu.etype, sizeof(mk->frame.etype))) {
            T_I("Unexpected data in EtherType frame keys");
            return VTSS_RC_ERROR;
        }

        if ((mk->type != VTSS_APPL_QOS_QCE_TYPE_LLC) && memcmp(&mk->frame.llc, &zffu.llc, sizeof(mk->frame.llc))) {
            T_I("Unexpected data in LLC frame keys");
            return VTSS_RC_ERROR;
        }

        if ((mk->type != VTSS_APPL_QOS_QCE_TYPE_SNAP) && memcmp(&mk->frame.snap, &zffu.snap, sizeof(mk->frame.snap))) {
            T_I("Unexpected data in SNAP frame keys");
            return VTSS_RC_ERROR;
        }

        if ((mk->type != VTSS_APPL_QOS_QCE_TYPE_IPV4) && memcmp(&mk->frame.ipv4, &zffu.ipv4, sizeof(mk->frame.ipv4))) {
            T_I("Unexpected data in IPv4 frame keys");
            return VTSS_RC_ERROR;
        }

        if ((mk->type != VTSS_APPL_QOS_QCE_TYPE_IPV6) && memcmp(&mk->frame.ipv6, &zffu.ipv6, sizeof(mk->frame.ipv6))) {
            T_I("Unexpected data in IPv6 frame keys");
            return VTSS_RC_ERROR;
        }

        switch (k->type) {
        case MESA_QCE_TYPE_ETYPE: {
            const vtss_appl_qos_qce_frame_etype_t *const etype_2 = &mk->frame.etype;
            mesa_qce_frame_etype_t                *const etype   = &k->frame.etype;

            if (etype_2->etype) {
                etype->etype.value[0] = (etype_2->etype >> 8) & 0xFF;
                etype->etype.mask[0] =  0xFF;
                etype->etype.value[1] = etype_2->etype & 0xFF;
                etype->etype.mask[1] =  0xFF;
            }
            break;
        }
        case MESA_QCE_TYPE_LLC: {
            const vtss_appl_qos_qce_frame_llc_t *const llc_2 = &mk->frame.llc;
            mesa_qce_frame_llc_t                *const llc   = &k->frame.llc;

            if (llc_2->dsap.mask && (llc_2->dsap.mask != 0xFF)) {
                T_I("Invalid LLC DSAP mask");
                return VTSS_RC_ERROR;
            }
            llc->data.value[0] = llc_2->dsap.value;
            llc->data.mask[0]  = llc_2->dsap.mask;

            if (llc_2->ssap.mask && (llc_2->ssap.mask != 0xFF)) {
                T_I("Invalid LLC SSAP mask");
                return VTSS_RC_ERROR;
            }
            llc->data.value[1] = llc_2->ssap.value;
            llc->data.mask[1]  = llc_2->ssap.mask;


            if (llc_2->control.mask && (llc_2->control.mask != 0xFF)) {
                T_I("Invalid LLC Control mask");
                return VTSS_RC_ERROR;
            }
            llc->data.value[2] = llc_2->control.value;
            llc->data.mask[2]  = llc_2->control.mask;
            break;
        }
        case MESA_QCE_TYPE_SNAP: {
            const vtss_appl_qos_qce_frame_snap_t *const snap_2 = &mk->frame.snap;
            mesa_qce_frame_snap_t                *const snap   = &k->frame.snap;

            if (snap_2->pid.mask && (snap_2->pid.mask != 0xFFFF)) {
                T_I("Invalid SNAP PID mask");
                return VTSS_RC_ERROR;
            }
            snap->data.value[3] = (snap_2->pid.value >> 8) & 0xFF;
            snap->data.mask[3]  = (snap_2->pid.mask  >> 8) & 0xFF;
            snap->data.value[4] = snap_2->pid.value & 0xFF;
            snap->data.mask[4]  = snap_2->pid.mask & 0xFF;
            break;
        }
        case MESA_QCE_TYPE_IPV4: {
            const vtss_appl_qos_qce_frame_ipv4_t *const i4_2 = &mk->frame.ipv4;
            mesa_qce_frame_ipv4_t                *const i4   = &k->frame.ipv4;

            i4->fragment = i4_2->fragment;
            VTSS_RC(vtss_appl_vcap_asr2vr(&i4_2->dscp, &i4->dscp, 0x3F));
            i4->proto = i4_2->proto;
            i4->sip = i4_2->sip;
            i4->dip = i4_2->dip;
            VTSS_RC(vtss_appl_vcap_asr2vr(&i4_2->sport, &i4->sport, 0xFFFF));
            VTSS_RC(vtss_appl_vcap_asr2vr(&i4_2->dport, &i4->dport, 0xFFFF));
            break;
        }
        case MESA_QCE_TYPE_IPV6: {
            const vtss_appl_qos_qce_frame_ipv6_t *const i6_2 = &mk->frame.ipv6;
            mesa_qce_frame_ipv6_t                *const i6   = &k->frame.ipv6;

            VTSS_RC(vtss_appl_vcap_asr2vr(&i6_2->dscp, &i6->dscp, 0x3F));
            i6->proto = i6_2->proto;
            for (i = 0; i < 16; i++) { 
                i6->sip.value[i] = i6_2->sip.value.addr[i];
                i6->sip.mask[i]  = i6_2->sip.mask.addr[i];
                i6->dip.value[i] = i6_2->dip.value.addr[i];
                i6->dip.mask[i]  = i6_2->dip.mask.addr[i];
            }
            VTSS_RC(vtss_appl_vcap_asr2vr(&i6_2->sport, &i6->sport, 0xFFFF));
            VTSS_RC(vtss_appl_vcap_asr2vr(&i6_2->dport, &i6->dport, 0xFFFF));
            break;
        }
        default:
            break;
        }
    } // if (!clear)

    // action
    a->prio_enable      = conf->action.prio_enable;
    a->prio             = conf->action.prio;
    a->dp_enable        = conf->action.dp_enable;
    a->dp               = conf->action.dp;
    a->dscp_enable      = conf->action.dscp_enable;
    a->dscp             = conf->action.dscp;
    a->pcp_dei_enable   = conf->action.pcp_dei_enable;
    a->pcp              = conf->action.pcp;
    a->dei              = conf->action.dei;
    a->policy_no_enable = conf->action.policy_no_enable;
    a->policy_no        = conf->action.policy_no;
    a->map_id_enable    = conf->action.map_id_enable;
    a->map_id           = conf->action.map_id;

    return VTSS_RC_OK;
}
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */

/****************************************************************************/
/*  Global management functions                                             */
/****************************************************************************/

/* Get QoS capabilities */
/* Attention!! Defines in initialization of capabilities structure must be  */
/* coordinated with similar defines in qos_api.h                            */
mesa_rc vtss_appl_qos_capabilities_get(vtss_appl_qos_capabilities_t *c)
{

    memset(c, 0, sizeof(*c)); /* Default: All has_* are false and all min/max are zero */

    c->class_min      = VTSS_APPL_QOS_CLASS_MIN;
    c->class_max      = VTSS_APPL_QOS_CLASS_MAX;

    c->dpl_min        = 0;
    c->dpl_max        = (fast_cap(MESA_CAP_QOS_DPL_CNT) - 1);

    c->wred_group_min = (fast_cap(MESA_CAP_QOS_WRED_GROUP_CNT) ? 1 : 0);
    c->wred_group_max = fast_cap(MESA_CAP_QOS_WRED_GROUP_CNT);
    c->wred_dpl_min   = c->wred_group_min;
    c->wred_dpl_max   = c->wred_group_max;
    if (c->wred_group_max > 1) {
        c->has_wred_v3    = TRUE;
    } else if (c->wred_group_max) {
        c->has_wred_v2    = TRUE;
    }
    c->has_wred2_or_wred3 = fast_cap(MESA_CAP_QOS_WRED);

    if (c->dpl_max > 1) {
        c->has_dscp_dp2 = TRUE;
    }

    if (c->dpl_max > 2) {
        c->has_dscp_dp3 = TRUE;
    }

    c->port_policer_bit_rate_min     = fast_cap(MESA_CAP_QOS_PORT_POLICER_BIT_RATE_MIN);
    c->port_policer_bit_rate_max     = fast_cap(MESA_CAP_QOS_PORT_POLICER_BIT_RATE_MAX);
    c->port_policer_bit_burst_min    = fast_cap(MESA_CAP_QOS_PORT_POLICER_BIT_BURST_MIN);
    c->port_policer_bit_burst_max    = fast_cap(MESA_CAP_QOS_PORT_POLICER_BIT_BURST_MAX);
    c->port_policer_frame_rate_min   = fast_cap(MESA_CAP_QOS_PORT_POLICER_FRAME_RATE_MIN);
    c->port_policer_frame_rate_max   = fast_cap(MESA_CAP_QOS_PORT_POLICER_FRAME_RATE_MAX);
    c->port_policer_frame_burst_min  = fast_cap(MESA_CAP_QOS_PORT_POLICER_FRAME_BURST_MIN);
    c->port_policer_frame_burst_max  = fast_cap(MESA_CAP_QOS_PORT_POLICER_FRAME_BURST_MAX);

    c->queue_policer_bit_rate_min    = fast_cap(MESA_CAP_QOS_QUEUE_POLICER_BIT_RATE_MIN);
    c->queue_policer_bit_rate_max    = fast_cap(MESA_CAP_QOS_QUEUE_POLICER_BIT_RATE_MAX);
    c->queue_policer_bit_burst_min   = fast_cap(MESA_CAP_QOS_QUEUE_POLICER_BIT_BURST_MIN);
    c->queue_policer_bit_burst_max   = fast_cap(MESA_CAP_QOS_QUEUE_POLICER_BIT_BURST_MAX);
    c->queue_policer_frame_rate_min  = fast_cap(MESA_CAP_QOS_QUEUE_POLICER_FRAME_RATE_MIN);
    c->queue_policer_frame_rate_max  = fast_cap(MESA_CAP_QOS_QUEUE_POLICER_FRAME_RATE_MAX);
    c->queue_policer_frame_burst_min = fast_cap(MESA_CAP_QOS_QUEUE_POLICER_FRAME_BURST_MIN);
    c->queue_policer_frame_burst_max = fast_cap(MESA_CAP_QOS_QUEUE_POLICER_FRAME_BURST_MAX);

    c->port_shaper_bit_rate_min      = fast_cap(MESA_CAP_QOS_PORT_SHAPER_BIT_RATE_MIN);
    c->port_shaper_bit_rate_max      = fast_cap(MESA_CAP_QOS_PORT_SHAPER_BIT_RATE_MAX);
    c->port_shaper_bit_burst_min     = fast_cap(MESA_CAP_QOS_PORT_SHAPER_BIT_BURST_MIN);
    c->port_shaper_bit_burst_max     = fast_cap(MESA_CAP_QOS_PORT_SHAPER_BIT_BURST_MAX);
    c->port_shaper_frame_rate_min    = fast_cap(MESA_CAP_QOS_PORT_SHAPER_FRAME_RATE_MIN);
    c->port_shaper_frame_rate_max    = fast_cap(MESA_CAP_QOS_PORT_SHAPER_FRAME_RATE_MAX);
    c->port_shaper_frame_burst_min   = fast_cap(MESA_CAP_QOS_PORT_SHAPER_FRAME_BURST_MIN);
    c->port_shaper_frame_burst_max   = fast_cap(MESA_CAP_QOS_PORT_SHAPER_FRAME_BURST_MAX);

    c->queue_shaper_bit_rate_min     = fast_cap(MESA_CAP_QOS_QUEUE_SHAPER_BIT_RATE_MIN);
    c->queue_shaper_bit_rate_max     = fast_cap(MESA_CAP_QOS_QUEUE_SHAPER_BIT_RATE_MAX);
    c->queue_shaper_bit_burst_min    = fast_cap(MESA_CAP_QOS_QUEUE_SHAPER_BIT_BURST_MIN);
    c->queue_shaper_bit_burst_max    = fast_cap(MESA_CAP_QOS_QUEUE_SHAPER_BIT_BURST_MAX);
    c->queue_shaper_frame_rate_min   = fast_cap(MESA_CAP_QOS_QUEUE_SHAPER_FRAME_RATE_MIN);
    c->queue_shaper_frame_rate_max   = fast_cap(MESA_CAP_QOS_QUEUE_SHAPER_FRAME_RATE_MAX);
    c->queue_shaper_frame_burst_min  = fast_cap(MESA_CAP_QOS_QUEUE_SHAPER_FRAME_BURST_MIN);
    c->queue_shaper_frame_burst_max  = fast_cap(MESA_CAP_QOS_QUEUE_SHAPER_FRAME_BURST_MAX);

    c->global_storm_bit_rate_min     = fast_cap(MESA_CAP_QOS_GLOBAL_STORM_BIT_RATE_MIN);
    c->global_storm_bit_rate_max     = fast_cap(MESA_CAP_QOS_GLOBAL_STORM_BIT_RATE_MAX);
    c->global_storm_bit_burst_min    = fast_cap(MESA_CAP_QOS_GLOBAL_STORM_BIT_BURST_MIN);
    c->global_storm_bit_burst_max    = fast_cap(MESA_CAP_QOS_GLOBAL_STORM_BIT_BURST_MAX);
    c->global_storm_frame_rate_min   = fast_cap(MESA_CAP_QOS_GLOBAL_STORM_FRAME_RATE_MIN);
    c->global_storm_frame_rate_max   = fast_cap(MESA_CAP_QOS_GLOBAL_STORM_FRAME_RATE_MAX);
    c->global_storm_frame_burst_min  = fast_cap(MESA_CAP_QOS_GLOBAL_STORM_FRAME_BURST_MIN);
    c->global_storm_frame_burst_max  = fast_cap(MESA_CAP_QOS_GLOBAL_STORM_FRAME_BURST_MAX);

    c->port_storm_bit_rate_min       = fast_cap(MESA_CAP_QOS_PORT_STORM_BIT_RATE_MIN);
    c->port_storm_bit_rate_max       = fast_cap(MESA_CAP_QOS_PORT_STORM_BIT_RATE_MAX);
    c->port_storm_bit_burst_min      = fast_cap(MESA_CAP_QOS_PORT_STORM_BIT_BURST_MIN);
    c->port_storm_bit_burst_max      = fast_cap(MESA_CAP_QOS_PORT_STORM_BIT_BURST_MAX);
    c->port_storm_frame_rate_min     = fast_cap(MESA_CAP_QOS_PORT_STORM_FRAME_RATE_MIN);
    c->port_storm_frame_rate_max     = fast_cap(MESA_CAP_QOS_PORT_STORM_FRAME_RATE_MAX);
    c->port_storm_frame_burst_min    = fast_cap(MESA_CAP_QOS_PORT_STORM_FRAME_BURST_MIN);
    c->port_storm_frame_burst_max    = fast_cap(MESA_CAP_QOS_PORT_STORM_FRAME_BURST_MAX);

    c->qce_id_min = VTSS_APPL_QOS_QCE_ID_START;
    c->qce_id_max = 256;

    if (fast_cap(MESA_CAP_QOS_SCHEDULER_CNT_DWRR)) {
        c->dwrr_cnt_mask  = 0xfe; /* 0b1111.1110 == 2-8 and 0 (zero is always allowed) */
        /*                             ^^^^-^^^-----bit 8, 7, 6, 5, 4, 3 and 2         */
    } else {
        c->dwrr_cnt_mask  = 0x20; /* 0b0010.0000 == 6 and 0 (zero is always allowed)   */
        /*                               ^----------bit 6                              */
    }

    c->has_global_storm_policers = TRUE;
    c->has_port_storm_policers = VTSS_APPL_QOS_PORT_STORM_POLICER;

#if defined(VTSS_SW_OPTION_QOS_ADV)
    c->has_port_queue_policers = TRUE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

#if defined(VTSS_SW_OPTION_QOS_ADV)
    c->has_tag_classification = TRUE;
    c->has_tag_remarking = TRUE;

    c->has_dscp = TRUE;
    c->has_dscp_dpl_class = TRUE;
    c->has_dscp_dpl_remark = fast_cap(MESA_CAP_QOS_DSCP_REMARK_DP_AWARE);
    c->has_cosid_classification = fast_cap(MESA_CAP_QOS_COSID_CLASSIFICATION);
    c->cosid_min      = 0;
    c->cosid_max      = (VTSS_COSIDS - 1);
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

    // Use cached value
    c->has_port_policers_fc = qos_capabilities.has_port_policers_fc;

    if (fast_cap(MESA_CAP_QOS_EGRESS_SHAPERS_DLB)) {
        c->has_port_shapers_dlb = TRUE;
        c->has_queue_shapers_dlb = TRUE;
    }
    c->has_queue_shapers_eb = fast_cap(MESA_CAP_QOS_EGRESS_QUEUE_SHAPERS_EB);
    c->has_queue_shapers_crb = fast_cap(MESA_CAP_QOS_EGRESS_QUEUE_SHAPERS_CRB);
    c->has_shapers_rt = fast_cap(MESA_CAP_QOS_EGRESS_SHAPERS_RT);

    if (fast_cap(MESA_CAP_QOS_EGRESS_SHAPER_FRAME)) {
        c->has_queue_shapers_frame_dlb = TRUE;
    }

#if defined(VTSS_SW_OPTION_QOS_ADV)
    c->has_queue_cut_through = fast_cap(MESA_CAP_QOS_EGRESS_QUEUE_CUT_THROUGH);
    c->has_queue_frame_preemption = false; // Moved to TSN module
    c->has_qbv = false;  // Moved to TSN module
    c->has_psfp = false; // Moved to TSN module

    c->ingress_map_id_min = 0;
    c->ingress_map_id_max = fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT);
    if (c->ingress_map_id_max) {
        c->has_ingress_map = TRUE;
        c->ingress_map_id_max--;
    }

    c->egress_map_id_min = 0;
    c->egress_map_id_max = fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT);
    if (c->egress_map_id_max) {
        c->has_egress_map = TRUE;
        c->egress_map_id_max--;
    }
#endif

   c->has_default_pcp_and_dei = TRUE;

#if defined(VTSS_SW_OPTION_QOS_ADV)
   c->has_trust_tag = TRUE;
#endif

#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    c->has_qce = TRUE;
    c->has_qce_address_mode = fast_cap(MESA_CAP_QOS_QCL_DMAC_DIP);
    c->has_qce_key_type = fast_cap(MESA_CAP_QOS_QCL_KEY_TYPE);
    c->has_qce_dmac = fast_cap(MESA_CAP_QOS_QCL_KEY_DMAC);
    c->has_qce_dip = fast_cap(MESA_CAP_QOS_QCL_KEY_DIP);
    c->has_qce_ctag = TRUE;
    c->has_qce_stag = TRUE;
    c->has_qce_inner_tag = fast_cap(MESA_CAP_QOS_QCL_KEY_INNER_TAG);
    c->has_qce_action_pcp_dei = TRUE;
    c->has_qce_action_policy = TRUE;
    c->has_qce_action_map = c->has_ingress_map;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */

    return VTSS_RC_OK;
}

/* Get default QoS General configuration  */
mesa_rc vtss_appl_qos_conf_get_default(vtss_appl_qos_conf_t *conf)
{
    VTSS_ASSERT(conf);
    QOS_conf_default_set(conf);
    return VTSS_RC_OK;
}

/* Get QoS General configuration  */
mesa_rc vtss_appl_qos_conf_get(vtss_appl_qos_conf_t *conf)
{
    mesa_rc rc;

    QOS_CRIT_ENTER();
    rc = QOS_conf_get(conf);
    QOS_CRIT_EXIT();
    return rc;
}

/* Set QoS General configuration  */
mesa_rc vtss_appl_qos_conf_set(const vtss_appl_qos_conf_t *conf)
{
    mesa_rc rc;

    QOS_CRIT_ENTER();
    rc = QOS_conf_set(conf);
    QOS_CRIT_EXIT();
    return rc;
}

/* QoS Global Storm Policers get/set */
mesa_rc vtss_appl_qos_global_uc_policer_get(vtss_appl_qos_global_storm_policer_t *conf)
{
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        *conf = c.uc_policer;
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_global_uc_policer_set(const vtss_appl_qos_global_storm_policer_t *conf)
{
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        c.uc_policer = *conf;
        rc = QOS_conf_set(&c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_global_mc_policer_get(vtss_appl_qos_global_storm_policer_t *conf)
{
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        *conf = c.mc_policer;
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_global_mc_policer_set(const vtss_appl_qos_global_storm_policer_t *conf)
{
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        c.mc_policer = *conf;
        rc = QOS_conf_set(&c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_global_bc_policer_get(vtss_appl_qos_global_storm_policer_t *conf)
{
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        *conf = c.bc_policer;
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_global_bc_policer_set(const vtss_appl_qos_global_storm_policer_t *conf)
{
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        c.bc_policer = *conf;
        rc = QOS_conf_set(&c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

static mesa_rc vtss_appl_qos_wred_group_itr(const mesa_wred_group_t *prev, mesa_wred_group_t *next)
{
    vtss::expose::snmp::IteratorComposeRange<mesa_wred_group_t> itr(CAPA->wred_group_min, CAPA->wred_group_max);
    return itr(prev, next);
}

static mesa_rc vtss_appl_qos_scheduler_queue_itr(const mesa_prio_t *prev, mesa_prio_t *next)
{
    vtss::expose::snmp::IteratorComposeRange<mesa_prio_t> itr(0, VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_MAX);
    return itr(prev, next);
}

static mesa_rc vtss_appl_qos_wred_dpl_itr(const mesa_dp_level_t *prev, mesa_dp_level_t *next)
{
    vtss::expose::snmp::IteratorComposeRange<mesa_dp_level_t> itr(CAPA->wred_dpl_min, CAPA->wred_dpl_max);
    return itr(prev, next);
}

/* QoS WRED itr/get/set */
mesa_rc vtss_appl_qos_wred_itr(const mesa_wred_group_t *prev_group, mesa_wred_group_t *next_group,
                               const mesa_prio_t       *prev_queue, mesa_prio_t       *next_queue,
                               const mesa_dp_level_t   *prev_dpl,   mesa_dp_level_t   *next_dpl)
{
    if (CAPA->has_wred2_or_wred3) {
        vtss::IteratorComposeN<u32, mesa_prio_t, mesa_dp_level_t> itr(&vtss_appl_qos_wred_group_itr,
                                                                      &vtss_appl_qos_scheduler_queue_itr,
                                                                      &vtss_appl_qos_wred_dpl_itr);
        return itr(prev_group, next_group, prev_queue, next_queue, prev_dpl, next_dpl);
    }
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

static mesa_rc vtss_appl_qos_wred_check(mesa_wred_group_t group, mesa_prio_t queue, mesa_dp_level_t dpl)
{
    if (!CAPA->has_wred2_or_wred3) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }
    if ((group < CAPA->wred_group_min) || (group > CAPA->wred_group_max)) {
        return VTSS_RC_ERROR;
    }
    if (queue > VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_MAX) {
        return VTSS_RC_ERROR;
    }
    if ((dpl < CAPA->wred_dpl_min) || (dpl > CAPA->wred_dpl_max)) {
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_qos_wred_get(mesa_wred_group_t group, mesa_prio_t queue, mesa_dp_level_t dpl, vtss_appl_qos_wred_t *conf)
{
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    VTSS_RC(vtss_appl_qos_wred_check(group, queue, dpl));
    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        *conf = c.wred[queue][dpl - CAPA->wred_dpl_min][group - CAPA->wred_group_min];
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_wred_set(mesa_wred_group_t group, mesa_prio_t queue, mesa_dp_level_t dpl, const vtss_appl_qos_wred_t *conf)
{
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    VTSS_RC(vtss_appl_qos_wred_check(group, queue, dpl));
    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        c.wred[queue][dpl - CAPA->wred_dpl_min][group - CAPA->wred_group_min] = *conf;
        rc = QOS_conf_set(&c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_dscp_itr(const mesa_dscp_t *prev, mesa_dscp_t *next)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    vtss::expose::snmp::IteratorComposeRange<mesa_dscp_t> itr(0, 63);
    return itr(prev, next);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_dscp_get(mesa_dscp_t dscp, vtss_appl_qos_dscp_entry_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    if (dscp > 63) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        *conf = c.dscp_map[dscp];
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_dscp_set(mesa_dscp_t dscp, const vtss_appl_qos_dscp_entry_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    if (dscp > 63) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        c.dscp_map[dscp] = *conf;
        rc = QOS_conf_set(&c);
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_cos_dscp_itr(const mesa_prio_t *prev, mesa_prio_t *next)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    vtss::expose::snmp::IteratorComposeRange<mesa_prio_t> itr(0, VTSS_APPL_QOS_PORT_QUEUE_CNT - 1);
    return itr(prev, next);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_cos_dscp_get(mesa_prio_t queue, vtss_appl_qos_cos_dscp_entry_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    if (queue >= VTSS_APPL_QOS_PORT_QUEUE_CNT) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        *conf = c.cos_dscp_map[queue];
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_cos_dscp_set(mesa_prio_t queue, const vtss_appl_qos_cos_dscp_entry_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc              rc;
    vtss_appl_qos_conf_t c;

    if (queue >= VTSS_APPL_QOS_PORT_QUEUE_CNT) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_conf_get(&c)) == VTSS_RC_OK) {
        c.cos_dscp_map[queue] = *conf;
        rc = QOS_conf_set(&c);
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

/****************************************************************************/
/*  Port management functions                                               */
/****************************************************************************/
/* Get default port QoS configuration  */
mesa_rc vtss_appl_qos_port_conf_get_default(vtss_appl_qos_port_conf_t *conf)
{
    VTSS_ASSERT(conf);
    QOS_port_conf_default_set(conf);
    return VTSS_RC_OK;
}

/* Get port QoS configuration  */
mesa_rc vtss_appl_qos_port_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_qos_port_conf_t *conf)
{
    mesa_rc rc;

    QOS_CRIT_ENTER();
    rc = QOS_port_conf_get(isid, port_no, conf);
    QOS_CRIT_EXIT();
    return rc;
}

/* Set port QoS configuration  */
mesa_rc vtss_appl_qos_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_qos_port_conf_t *conf)
{
    mesa_rc rc;

    QOS_CRIT_ENTER();
    rc = QOS_port_conf_set(isid, port_no, conf);
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_interface_conf_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next)
{
    return vtss_appl_iterator_ifindex_front_port(prev, next);
}

mesa_rc vtss_appl_qos_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_qos_if_conf_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        *conf = c.port;
    }
    QOS_CRIT_EXIT();
    return rc;
}

// Wrapper for converting from ifindex to isid,iport for configuration set
mesa_rc vtss_appl_qos_interface_conf_set(vtss_ifindex_t ifindex, const vtss_appl_qos_if_conf_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        c.port = *conf;
        rc = QOS_interface_conf_set(ifindex, &c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_tag_cos_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                  const mesa_tagprio_t *prev_pcp,     mesa_tagprio_t *next_pcp,
                                  const mesa_dei_t     *prev_dei,     mesa_dei_t     *next_dei)
{
#if (defined(VTSS_SW_OPTION_QOS_ADV))
    vtss::IteratorComposeN<vtss_ifindex_t, mesa_tagprio_t, mesa_dei_t> itr(&vtss_appl_iterator_ifindex_front_port,
                                                                           &vtss::expose::snmp::IteratorComposeStaticRange < mesa_tagprio_t, 0, VTSS_PCPS - 1 > ,
                                                                           &vtss::expose::snmp::IteratorComposeStaticRange < mesa_dei_t,     0, VTSS_DEIS - 1 > );
    return itr(prev_ifindex, next_ifindex, prev_pcp, next_pcp, prev_dei, next_dei);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* (defined(VTSS_SW_OPTION_QOS_ADV)) */
}

mesa_rc vtss_appl_qos_tag_cos_get(vtss_ifindex_t ifindex, mesa_tagprio_t pcp, mesa_dei_t dei, vtss_appl_qos_tag_cos_entry_t *conf)
{
#if (defined(VTSS_SW_OPTION_QOS_ADV))
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (pcp > 7) {
        return VTSS_RC_ERROR;
    }
    if (dei > 1) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        *conf = c.tag_cos_map[pcp][dei];
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* (defined(VTSS_SW_OPTION_QOS_ADV)) */
}

mesa_rc vtss_appl_qos_tag_cos_set(vtss_ifindex_t ifindex, mesa_tagprio_t pcp, mesa_dei_t dei, const vtss_appl_qos_tag_cos_entry_t *conf)
{
#if (defined(VTSS_SW_OPTION_QOS_ADV))
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (pcp > 7) {
        return VTSS_RC_ERROR;
    }
    if (dei > 1) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        c.tag_cos_map[pcp][dei] = *conf;
        rc = QOS_interface_conf_set(ifindex, &c);
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* (defined(VTSS_SW_OPTION_QOS_ADV)) */
}

#if defined(VTSS_SW_OPTION_QOS_ADV)
static mesa_rc vtss_appl_qos_dpl_itr(const mesa_dp_level_t *prev, mesa_dp_level_t *next)
{
    vtss::expose::snmp::IteratorComposeRange<mesa_dp_level_t> itr(CAPA->dpl_min, CAPA->dpl_max);
    return itr(prev, next);
}
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

mesa_rc vtss_appl_qos_cos_tag_itr(const vtss_ifindex_t  *prev_ifindex, vtss_ifindex_t  *next_ifindex,
                                  const mesa_prio_t     *prev_cos,     mesa_prio_t     *next_cos,
                                  const mesa_dp_level_t *prev_dpl,     mesa_dp_level_t *next_dpl)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    vtss::IteratorComposeN<vtss_ifindex_t, mesa_prio_t, mesa_dp_level_t> itr(&vtss_appl_iterator_ifindex_front_port,
                                                                             &vtss::expose::snmp::IteratorComposeStaticRange<mesa_prio_t, 0, VTSS_APPL_QOS_CLASS_MAX>,
                                                                             &vtss_appl_qos_dpl_itr);
    return itr(prev_ifindex, next_ifindex, prev_cos, next_cos, prev_dpl, next_dpl);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_cos_tag_get(vtss_ifindex_t ifindex, mesa_prio_t cos, mesa_dp_level_t dpl, vtss_appl_qos_cos_tag_entry_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (cos > VTSS_APPL_QOS_CLASS_MAX) {
        return VTSS_RC_ERROR;
    }
    if (dpl > 1) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        *conf = c.cos_tag_map[cos][dpl];
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_cos_tag_set(vtss_ifindex_t ifindex, mesa_prio_t cos, mesa_dp_level_t dpl, const vtss_appl_qos_cos_tag_entry_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (cos > VTSS_APPL_QOS_CLASS_MAX) {
        return VTSS_RC_ERROR;
    }
    if (dpl > 1) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        c.cos_tag_map[cos][dpl] = *conf;
        rc = QOS_interface_conf_set(ifindex, &c);
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_port_policer_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next)
{
    return vtss_appl_iterator_ifindex_front_port(prev, next);
}

mesa_rc vtss_appl_qos_port_policer_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_policer_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        *conf = c.port_policer;
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_port_policer_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_policer_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        /* Copy parameters one by one. We not yet allow them all. */
        c.port_policer.enable       = conf->enable;
        c.port_policer.frame_rate   = conf->frame_rate;
        c.port_policer.flow_control = conf->flow_control;
        c.port_policer.cir          = conf->cir;
        rc = QOS_interface_conf_set(ifindex, &c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

// Generic iterator for ifIndex,queue
static mesa_rc vtss_appl_qos_ifindex_queue_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                               const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue)
{
    vtss::IteratorComposeN<vtss_ifindex_t, mesa_prio_t> itr(&vtss_appl_iterator_ifindex_front_port,
                                                            &vtss::expose::snmp::IteratorComposeStaticRange < mesa_prio_t, 0, VTSS_APPL_QOS_PORT_QUEUE_CNT - 1 > );
    return itr(prev_ifindex, next_ifindex, prev_queue, next_queue);
}

mesa_rc vtss_appl_qos_queue_policer_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                        const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    return vtss_appl_qos_ifindex_queue_itr(prev_ifindex, next_ifindex, prev_queue, next_queue);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_queue_policer_get(vtss_ifindex_t ifindex, mesa_prio_t queue, vtss_appl_qos_queue_policer_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (queue >= VTSS_APPL_QOS_PORT_QUEUE_CNT) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        *conf = c.queue_policer[queue];
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_queue_policer_set(vtss_ifindex_t ifindex, mesa_prio_t queue, const vtss_appl_qos_queue_policer_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (queue >= VTSS_APPL_QOS_PORT_QUEUE_CNT) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        /* Copy parameters one by one. We not yet allow them all. */
        c.queue_policer[queue].enable = conf->enable;
        c.queue_policer[queue].cir    = conf->cir;
        rc = QOS_interface_conf_set(ifindex, &c);
    }
    QOS_CRIT_EXIT();
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
}

mesa_rc vtss_appl_qos_port_shaper_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next)
{
    return vtss_appl_iterator_ifindex_front_port(prev, next);
}

mesa_rc vtss_appl_qos_port_shaper_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_shaper_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        *conf = c.port_shaper;
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_port_shaper_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_shaper_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        /* Copy parameters one by one. We not yet allow them all. */
        c.port_shaper.enable = conf->enable;
        c.port_shaper.cir    = conf->cir;
        c.port_shaper.mode   = conf->mode;
        rc = QOS_interface_conf_set(ifindex, &c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_queue_shaper_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                       const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue)
{
    return vtss_appl_qos_ifindex_queue_itr(prev_ifindex, next_ifindex, prev_queue, next_queue);
}

mesa_rc vtss_appl_qos_queue_shaper_get(vtss_ifindex_t ifindex, mesa_prio_t queue, vtss_appl_qos_queue_shaper_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (queue >= VTSS_APPL_QOS_PORT_QUEUE_CNT) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        *conf = c.queue_shaper[queue];
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_queue_shaper_set(vtss_ifindex_t ifindex, mesa_prio_t queue, const vtss_appl_qos_queue_shaper_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (queue >= VTSS_APPL_QOS_PORT_QUEUE_CNT) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        /* Copy parameters one by one. We not yet allow them all. */
        c.queue_shaper[queue].enable = conf->enable;
        c.queue_shaper[queue].excess = conf->excess;
        c.queue_shaper[queue].credit = conf->credit;
        c.queue_shaper[queue].cir    = conf->cir;
        c.queue_shaper[queue].mode   = conf->mode;
        rc = QOS_interface_conf_set(ifindex, &c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_scheduler_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                    const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue)
{
    vtss::IteratorComposeN<vtss_ifindex_t, mesa_prio_t> itr(&vtss_appl_iterator_ifindex_front_port,
                                                            &vtss_appl_qos_scheduler_queue_itr);
    return itr(prev_ifindex, next_ifindex, prev_queue, next_queue);
}

mesa_rc vtss_appl_qos_scheduler_get(vtss_ifindex_t ifindex, mesa_prio_t queue, vtss_appl_qos_scheduler_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (queue > VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_MAX) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        *conf = c.scheduler[queue];
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_scheduler_set(vtss_ifindex_t ifindex, mesa_prio_t queue, const vtss_appl_qos_scheduler_t *conf)
{
    mesa_rc                   rc;
    vtss_appl_qos_port_conf_t c;

    if (queue > VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_MAX) {
        return VTSS_RC_ERROR;
    }
    QOS_CRIT_ENTER();
    if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
        c.scheduler[queue] = *conf;
        rc = QOS_interface_conf_set(ifindex, &c);
    }
    QOS_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_qos_port_storm_policer_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next)
{
    if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
        return vtss_appl_iterator_ifindex_front_port(prev, next);
    }
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_port_uc_policer_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_storm_policer_t *conf)
{
    mesa_rc                   rc = VTSS_APPL_QOS_ERROR_FEATURE;
    vtss_appl_qos_port_conf_t c;

    if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
        QOS_CRIT_ENTER();
        if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
            *conf = c.uc_policer;
        }
        QOS_CRIT_EXIT();
    }
    return rc;
}

mesa_rc vtss_appl_qos_port_uc_policer_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_storm_policer_t *conf)
{
    mesa_rc                   rc = VTSS_APPL_QOS_ERROR_FEATURE;
    vtss_appl_qos_port_conf_t c;

    if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
        QOS_CRIT_ENTER();
        if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
            mesa_burst_level_t cbs = c.uc_policer.cbs; /* Save old cbs. We do not yet allow it to be modified from the public interface */
            c.uc_policer = *conf;
            c.uc_policer.cbs = cbs; /* Restore old cbs */
            rc = QOS_interface_conf_set(ifindex, &c);
        }
        QOS_CRIT_EXIT();
    }
    return rc;
}

mesa_rc vtss_appl_qos_port_bc_policer_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_storm_policer_t *conf)
{
    mesa_rc                   rc = VTSS_APPL_QOS_ERROR_FEATURE;
    vtss_appl_qos_port_conf_t c;

    if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
        QOS_CRIT_ENTER();
        if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
            *conf = c.bc_policer;
        }
        QOS_CRIT_EXIT();
    }
    return rc;
}

mesa_rc vtss_appl_qos_port_bc_policer_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_storm_policer_t *conf)
{
    mesa_rc                   rc = VTSS_APPL_QOS_ERROR_FEATURE;
    vtss_appl_qos_port_conf_t c;

    if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
        QOS_CRIT_ENTER();
        if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
            mesa_burst_level_t cbs = c.bc_policer.cbs; /* Save old cbs. We do not yet allow it to be modified from the public interface */
            c.bc_policer = *conf;
            c.bc_policer.cbs = cbs; /* Restore old cbs */
            rc = QOS_interface_conf_set(ifindex, &c);
        }
        QOS_CRIT_EXIT();
    }
    return rc;
}

mesa_rc vtss_appl_qos_port_un_policer_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_storm_policer_t *conf)
{
    mesa_rc                   rc = VTSS_APPL_QOS_ERROR_FEATURE;
    vtss_appl_qos_port_conf_t c;

    if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
        QOS_CRIT_ENTER();
        if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
            *conf = c.un_policer;
        }
        QOS_CRIT_EXIT();
    }
    return rc;
}

mesa_rc vtss_appl_qos_port_un_policer_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_storm_policer_t *conf)
{
    mesa_rc                   rc = VTSS_APPL_QOS_ERROR_FEATURE;
    vtss_appl_qos_port_conf_t c;

    if (VTSS_APPL_QOS_PORT_STORM_POLICER) {
        QOS_CRIT_ENTER();
        if ((rc = QOS_interface_conf_get(ifindex, &c)) == VTSS_RC_OK) {
            mesa_burst_level_t cbs = c.un_policer.cbs; /* Save old cbs. We do not yet allow it to be modified from the public interface */
            c.un_policer = *conf;
            c.un_policer.cbs = cbs; /* Restore old cbs */
            rc = QOS_interface_conf_set(ifindex, &c);
        }
        QOS_CRIT_EXIT();
    }
    return rc;
}

mesa_rc vtss_appl_qos_status_get(vtss_appl_qos_status_t *status)
{
    QOS_CRIT_ENTER();
    *status = qos.qos_status;
    QOS_CRIT_EXIT();
    return VTSS_RC_OK;
}


mesa_rc vtss_appl_qos_port_status_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_qos_port_status_t *status)
{
    mesa_rc rc          = VTSS_RC_OK;
    int     port_no_idx = port_no - VTSS_PORT_NO_START;

    T_N("enter, isid: %u, port_no: %u", isid, port_no);

    if (QOS_port_isid_invalid(isid, port_no, 0)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    memset(status, 0, sizeof(*status));

    QOS_CRIT_ENTER();
    if (qos.volatile_default_prio[port_no_idx] != VTSS_PRIO_NO_NONE) {
        status->default_cos = qos.volatile_default_prio[port_no_idx];
    } else {
        status->default_cos = qos.qos_port_conf[isid][port_no_idx].port.default_cos;
    }
    rc = QOS_weight2pct(qos.qos_port_conf[isid][port_no_idx].port.dwrr_cnt,
                        qos.qos_port_conf[isid][port_no_idx].scheduler,
                        status->scheduler);
    QOS_CRIT_EXIT();

    T_N("exit");
    return rc;
}

mesa_rc vtss_appl_qos_interface_status_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next)
{
    return vtss_appl_iterator_ifindex_front_port(prev, next);
}

mesa_rc vtss_appl_qos_interface_status_get(vtss_ifindex_t ifindex, vtss_appl_qos_if_status_t *status)
{
    vtss_ifindex_elm_t ife;
    vtss_appl_qos_port_status_t s;

    if (!vtss_ifindex_is_port(ifindex)) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    VTSS_RC(vtss_appl_qos_port_status_get(ife.isid, ife.ordinal, &s));
    status->default_cos = s.default_cos;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_qos_scheduler_status_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                           const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue)
{
    return vtss_appl_qos_scheduler_itr(prev_ifindex, next_ifindex, prev_queue, next_queue);
}

mesa_rc vtss_appl_qos_scheduler_status_get(vtss_ifindex_t ifindex, mesa_prio_t queue, vtss_appl_qos_scheduler_status_t *status)
{
    vtss_ifindex_elm_t ife;
    vtss_appl_qos_port_status_t s;

    if (!vtss_ifindex_is_port(ifindex)) {
        return VTSS_RC_ERROR;
    }

    if (queue > VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_MAX) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    VTSS_RC(vtss_appl_qos_port_status_get(ife.isid, ife.ordinal, &s));
    *status = s.scheduler[queue];
    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Ingress map functions                                                   */
/****************************************************************************/

#if defined(VTSS_SW_OPTION_QOS_ADV)

/* Get QoS ingress map default configuration */
static mesa_rc QOS_imap_get_default(mesa_qos_ingress_map_id_t id, vtss_appl_qos_imap_entry_t *entry)
{
    memset(entry, 0, sizeof(*entry)); // Default everything to zero.
    return VTSS_RC_OK;
}

/* Check QoS ingress map configuration */
static mesa_rc QOS_imap_conf_check(const vtss_appl_qos_ingress_map_conf_t *conf)
{
    if ((conf->key != VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP)          &&
        (conf->key != VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP_DEI)      &&
        (conf->key != VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP)         &&
        (conf->key != VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP_PCP_DEI)) {
        T_I("Invalid ingress map key: %u!", conf->key);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    return VTSS_RC_OK;
}

/* Check QoS ingress map values */
static mesa_rc QOS_imap_values_check(const vtss_appl_qos_ingress_map_values_t *values)
{
    if (values->cos >= VTSS_PRIO_END) {
        T_I("Invalid cos: %u!", values->cos);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (values->dpl >= fast_cap(MESA_CAP_QOS_DPL_CNT)) {
        T_I("Invalid dpl: %u!", values->dpl);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (values->pcp >= VTSS_PCP_END) {
        T_I("Invalid pcp: %u!", values->pcp);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (values->dei >= VTSS_DEI_END) {
        T_I("Invalid dei: %u!", values->dei);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (values->dscp > 63) {
        T_I("Invalid dscp: %u!", values->dscp);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (values->cosid >= VTSS_COSIDS) {
        T_I("Invalid cosid: %u!", values->cosid);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    return VTSS_RC_OK;
}

static mesa_rc QOS_imap_add(mesa_qos_ingress_map_id_t        id,
                            const vtss_appl_qos_imap_entry_t *entry)
{
    int                    pcp, dei, dscp;
    mesa_qos_ingress_map_t api_conf = {0};

    api_conf.id = id;

    switch (entry->conf.key) {
    case VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP:
        api_conf.key = MESA_QOS_INGRESS_MAP_KEY_PCP;
        for (pcp = 0; pcp < VTSS_PCP_END; pcp++) {
            api_conf.maps.pcp[pcp].cos        = entry->pcp_dei[pcp][0].cos;
            api_conf.maps.pcp[pcp].dpl        = entry->pcp_dei[pcp][0].dpl;
            api_conf.maps.pcp[pcp].pcp        = entry->pcp_dei[pcp][0].pcp;
            api_conf.maps.pcp[pcp].dei        = entry->pcp_dei[pcp][0].dei;
            api_conf.maps.pcp[pcp].dscp       = entry->pcp_dei[pcp][0].dscp;
            api_conf.maps.pcp[pcp].cosid      = entry->pcp_dei[pcp][0].cosid;
        }
        break;
    case VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP_DEI:
        api_conf.key = MESA_QOS_INGRESS_MAP_KEY_PCP_DEI;
        for (pcp = 0; pcp < VTSS_PCP_END; pcp++) {
            for (dei = 0; dei < VTSS_DEI_END; dei++) {
                api_conf.maps.pcp_dei[pcp][dei].cos        = entry->pcp_dei[pcp][dei].cos;
                api_conf.maps.pcp_dei[pcp][dei].dpl        = entry->pcp_dei[pcp][dei].dpl;
                api_conf.maps.pcp_dei[pcp][dei].pcp        = entry->pcp_dei[pcp][dei].pcp;
                api_conf.maps.pcp_dei[pcp][dei].dei        = entry->pcp_dei[pcp][dei].dei;
                api_conf.maps.pcp_dei[pcp][dei].dscp       = entry->pcp_dei[pcp][dei].dscp;
                api_conf.maps.pcp_dei[pcp][dei].cosid      = entry->pcp_dei[pcp][dei].cosid;
            }
        }
        break;
    case VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP:
        api_conf.key = MESA_QOS_INGRESS_MAP_KEY_DSCP;
        for (dscp = 0; dscp < 64; dscp++) {
            api_conf.maps.dscp[dscp].cos        = entry->dscp[dscp].cos;
            api_conf.maps.dscp[dscp].dpl        = entry->dscp[dscp].dpl;
            api_conf.maps.dscp[dscp].pcp        = entry->dscp[dscp].pcp;
            api_conf.maps.dscp[dscp].dei        = entry->dscp[dscp].dei;
            api_conf.maps.dscp[dscp].dscp       = entry->dscp[dscp].dscp;
            api_conf.maps.dscp[dscp].cosid      = entry->dscp[dscp].cosid;
        }
        break;
    case VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP_PCP_DEI:
        api_conf.key = MESA_QOS_INGRESS_MAP_KEY_DSCP_PCP_DEI;
        for (dscp = 0; dscp < 64; dscp++) {
            api_conf.maps.dpd.dscp[dscp].cos        = entry->dscp[dscp].cos;
            api_conf.maps.dpd.dscp[dscp].dpl        = entry->dscp[dscp].dpl;
            api_conf.maps.dpd.dscp[dscp].pcp        = entry->dscp[dscp].pcp;
            api_conf.maps.dpd.dscp[dscp].dei        = entry->dscp[dscp].dei;
            api_conf.maps.dpd.dscp[dscp].dscp       = entry->dscp[dscp].dscp;
            api_conf.maps.dpd.dscp[dscp].cosid      = entry->dscp[dscp].cosid;
        }
        for (pcp = 0; pcp < VTSS_PCP_END; pcp++) {
            for (dei = 0; dei < VTSS_DEI_END; dei++) {
                api_conf.maps.dpd.pcp_dei[pcp][dei].cos        = entry->pcp_dei[pcp][dei].cos;
                api_conf.maps.dpd.pcp_dei[pcp][dei].dpl        = entry->pcp_dei[pcp][dei].dpl;
                api_conf.maps.dpd.pcp_dei[pcp][dei].pcp        = entry->pcp_dei[pcp][dei].pcp;
                api_conf.maps.dpd.pcp_dei[pcp][dei].dei        = entry->pcp_dei[pcp][dei].dei;
                api_conf.maps.dpd.pcp_dei[pcp][dei].dscp       = entry->pcp_dei[pcp][dei].dscp;
                api_conf.maps.dpd.pcp_dei[pcp][dei].cosid      = entry->pcp_dei[pcp][dei].cosid;
            }
        }
        break;
    }

    api_conf.action.cos     = entry->conf.action.cos;
    api_conf.action.dpl     = entry->conf.action.dpl;
    api_conf.action.pcp     = entry->conf.action.pcp;
    api_conf.action.dei     = entry->conf.action.dei;
    api_conf.action.dscp    = entry->conf.action.dscp;
    api_conf.action.cosid   = entry->conf.action.cosid;

    return mesa_qos_ingress_map_add(NULL, &api_conf);
}

static mesa_rc QOS_ingress_map_conf_add_or_set(mesa_qos_ingress_map_id_t              id,
                                               const vtss_appl_qos_ingress_map_conf_t *conf,
                                               BOOL add)
{
    mesa_rc                    rc = VTSS_RC_OK;
    vtss_appl_qos_imap_entry_t new_conf = {0};

    T_D("enter, id: %u", id);

    VTSS_ASSERT(conf);

    if (id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((rc = QOS_imap_conf_check(conf)) != VTSS_RC_OK) {
        return rc;
    }

    QOS_CRIT_ENTER();
    if (add || qos.qos_imap[id].used) {
        new_conf      = qos.qos_imap[id];
        new_conf.used = TRUE;
        new_conf.conf = *conf;
        if (memcmp(&new_conf, &qos.qos_imap[id], sizeof(new_conf)) ) { // Only if new_conf differs from current conf
            if ((rc = QOS_imap_add(id, &new_conf)) == VTSS_RC_OK) {
                qos.qos_imap[id] = new_conf; // Update current conf
            }
        }
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'set' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
}

mesa_rc vtss_appl_qos_imap_entry_get(mesa_qos_ingress_map_id_t  id,
                                     vtss_appl_qos_imap_entry_t *entry)
{
    T_D("enter, id: %u", id);

    VTSS_ASSERT(entry);

    if (id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    *entry = qos.qos_imap[id];
    QOS_CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_qos_imap_entry_get_default(mesa_qos_ingress_map_id_t  id,
                                             vtss_appl_qos_imap_entry_t *entry)
{
    T_D("enter, id: %u", id);

    VTSS_ASSERT(entry);

    if (id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    return QOS_imap_get_default(id, entry);
}
#endif /* VTSS_SW_OPTION_QOS_ADV */

mesa_rc vtss_appl_qos_ingress_map_preset(mesa_qos_ingress_map_id_t id,
                                         u8                        classes,
                                         BOOL                      color_aware)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    int                        pcp, dei, dei_end;
    mesa_rc                    rc = VTSS_RC_OK;
    vtss_appl_qos_imap_entry_t new_conf = {0};

    T_D("enter, id: %u, classes: %u, color_aware: %d", id, classes, color_aware);

    if (!CAPA->has_ingress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    if (id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((classes < 1) || (classes > VTSS_COSIDS)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    new_conf.used = TRUE;

    new_conf.conf.key = color_aware ? VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP_DEI : VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP;

    new_conf.conf.action.cos = new_conf.conf.action.dpl = new_conf.conf.action.cosid = TRUE;

    dei_end = color_aware ? VTSS_DEI_END : 1;

    /* See DS1198 section 9.1.1 for details regarding the mapping */
    for (dei = 0; dei < dei_end; dei++) {
        switch (classes) {
        case 1:
            /* Everything it zero here */
            break;
        case 2:
            new_conf.pcp_dei[4][dei].cos = new_conf.pcp_dei[4][dei].cosid = 1;
            new_conf.pcp_dei[5][dei].cos = new_conf.pcp_dei[5][dei].cosid = 1;
            new_conf.pcp_dei[6][dei].cos = new_conf.pcp_dei[6][dei].cosid = 1;
            new_conf.pcp_dei[7][dei].cos = new_conf.pcp_dei[7][dei].cosid = 1;
            break;
        case 3:
            new_conf.pcp_dei[4][dei].cos = new_conf.pcp_dei[4][dei].cosid = 1;
            new_conf.pcp_dei[5][dei].cos = new_conf.pcp_dei[5][dei].cosid = 1;
            new_conf.pcp_dei[6][dei].cos = new_conf.pcp_dei[6][dei].cosid = 2;
            new_conf.pcp_dei[7][dei].cos = new_conf.pcp_dei[7][dei].cosid = 2;
            break;
        case 4:
            new_conf.pcp_dei[2][dei].cos = new_conf.pcp_dei[2][dei].cosid = 1;
            new_conf.pcp_dei[3][dei].cos = new_conf.pcp_dei[3][dei].cosid = 1;
            new_conf.pcp_dei[4][dei].cos = new_conf.pcp_dei[4][dei].cosid = 2;
            new_conf.pcp_dei[5][dei].cos = new_conf.pcp_dei[5][dei].cosid = 2;
            new_conf.pcp_dei[6][dei].cos = new_conf.pcp_dei[6][dei].cosid = 3;
            new_conf.pcp_dei[7][dei].cos = new_conf.pcp_dei[7][dei].cosid = 3;
            break;
        case 5:
            new_conf.pcp_dei[2][dei].cos = new_conf.pcp_dei[2][dei].cosid = 1;
            new_conf.pcp_dei[3][dei].cos = new_conf.pcp_dei[3][dei].cosid = 1;
            new_conf.pcp_dei[4][dei].cos = new_conf.pcp_dei[4][dei].cosid = 2;
            new_conf.pcp_dei[5][dei].cos = new_conf.pcp_dei[5][dei].cosid = 2;
            new_conf.pcp_dei[6][dei].cos = new_conf.pcp_dei[6][dei].cosid = 3;
            new_conf.pcp_dei[7][dei].cos = new_conf.pcp_dei[7][dei].cosid = 4;
            break;
        case 6:
            new_conf.pcp_dei[0][dei].cos = new_conf.pcp_dei[0][dei].cosid = 1;
            new_conf.pcp_dei[1][dei].cos = new_conf.pcp_dei[1][dei].cosid = 0;
            new_conf.pcp_dei[2][dei].cos = new_conf.pcp_dei[2][dei].cosid = 2;
            new_conf.pcp_dei[3][dei].cos = new_conf.pcp_dei[3][dei].cosid = 2;
            new_conf.pcp_dei[4][dei].cos = new_conf.pcp_dei[4][dei].cosid = 3;
            new_conf.pcp_dei[5][dei].cos = new_conf.pcp_dei[5][dei].cosid = 3;
            new_conf.pcp_dei[6][dei].cos = new_conf.pcp_dei[6][dei].cosid = 4;
            new_conf.pcp_dei[7][dei].cos = new_conf.pcp_dei[7][dei].cosid = 5;
            break;
        case 7:
            new_conf.pcp_dei[0][dei].cos = new_conf.pcp_dei[0][dei].cosid = 1;
            new_conf.pcp_dei[1][dei].cos = new_conf.pcp_dei[1][dei].cosid = 0;
            new_conf.pcp_dei[2][dei].cos = new_conf.pcp_dei[2][dei].cosid = 2;
            new_conf.pcp_dei[3][dei].cos = new_conf.pcp_dei[3][dei].cosid = 3;
            new_conf.pcp_dei[4][dei].cos = new_conf.pcp_dei[4][dei].cosid = 4;
            new_conf.pcp_dei[5][dei].cos = new_conf.pcp_dei[5][dei].cosid = 4;
            new_conf.pcp_dei[6][dei].cos = new_conf.pcp_dei[6][dei].cosid = 5;
            new_conf.pcp_dei[7][dei].cos = new_conf.pcp_dei[7][dei].cosid = 6;
            break;
        case 8:
            new_conf.pcp_dei[0][dei].cos = new_conf.pcp_dei[0][dei].cosid = 1;
            new_conf.pcp_dei[1][dei].cos = new_conf.pcp_dei[1][dei].cosid = 0;
            new_conf.pcp_dei[2][dei].cos = new_conf.pcp_dei[2][dei].cosid = 2;
            new_conf.pcp_dei[3][dei].cos = new_conf.pcp_dei[3][dei].cosid = 3;
            new_conf.pcp_dei[4][dei].cos = new_conf.pcp_dei[4][dei].cosid = 4;
            new_conf.pcp_dei[5][dei].cos = new_conf.pcp_dei[5][dei].cosid = 5;
            new_conf.pcp_dei[6][dei].cos = new_conf.pcp_dei[6][dei].cosid = 6;
            new_conf.pcp_dei[7][dei].cos = new_conf.pcp_dei[7][dei].cosid = 7;
            break;
        default:
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }

    if (color_aware) {
        for (pcp = 0; pcp < VTSS_PCP_END; pcp++) {
            new_conf.pcp_dei[pcp][1].dpl = 1;
        }
    }

    QOS_CRIT_ENTER();
    if (memcmp(&new_conf, &qos.qos_imap[id], sizeof(new_conf)) ) { // Only if new_conf differs from current conf
        if ((rc = QOS_imap_add(id, &new_conf)) == VTSS_RC_OK) {
            qos.qos_imap[id] = new_conf; // Update current conf
        }
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_ingress_map_conf_itr(const mesa_qos_ingress_map_id_t *prev_id, mesa_qos_ingress_map_id_t *next_id)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_qos_ingress_map_id_t id, id_start = (prev_id == NULL) ? 0 : *prev_id + 1;
    mesa_rc rc = VTSS_RC_ERROR;

    if (!CAPA->has_ingress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    QOS_CRIT_ENTER();
    for (id = id_start; id < fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT); id++) {
        if (qos.qos_imap[id].used) {
            *next_id = id;
            rc = VTSS_RC_OK;
            break;
        }
    }
    QOS_CRIT_EXIT();
    T_D("exit %s, prev_id: %d, next_id: %d", (rc == VTSS_RC_OK) ? "OK" : "ERROR", (prev_id == NULL) ? -1 : *prev_id, (next_id == NULL) ? -1 : *next_id);
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_ingress_map_conf_add(mesa_qos_ingress_map_id_t              id,
                                           const vtss_appl_qos_ingress_map_conf_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_ingress_map) {
        return QOS_ingress_map_conf_add_or_set(id, conf, TRUE);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_ingress_map_conf_del(const mesa_qos_ingress_map_id_t id)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, id: %u", id);

    if (!CAPA->has_ingress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    if (id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_imap[id].used) {
        QOS_imap_get_default(id, &qos.qos_imap[id]);
        qos.qos_imap[id].used = FALSE;
        rc = mesa_qos_ingress_map_del(NULL, id);
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'del' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_ingress_map_conf_get(mesa_qos_ingress_map_id_t        id,
                                           vtss_appl_qos_ingress_map_conf_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, id: %u", id);

    if (!CAPA->has_ingress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if (id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_imap[id].used) {
        *conf = qos.qos_imap[id].conf;
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'get' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_ingress_map_conf_set(mesa_qos_ingress_map_id_t              id,
                                           const vtss_appl_qos_ingress_map_conf_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_ingress_map) {
        return QOS_ingress_map_conf_add_or_set(id, conf, FALSE);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_ingress_map_pcp_dei_conf_itr(const mesa_qos_ingress_map_id_t *prev_id,  mesa_qos_ingress_map_id_t *next_id,
                                                   const mesa_tagprio_t            *prev_pcp, mesa_tagprio_t            *next_pcp,
                                                   const mesa_dei_t                *prev_dei, mesa_dei_t                *next_dei)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_ingress_map) {
        vtss::IteratorComposeN<mesa_qos_ingress_map_id_t, mesa_tagprio_t, mesa_dei_t> itr(&vtss_appl_qos_ingress_map_conf_itr,
                                                                                          &vtss::expose::snmp::IteratorComposeStaticRange<mesa_tagprio_t, 0, VTSS_PCPS - 1 >,
                                                                                          &vtss::expose::snmp::IteratorComposeStaticRange<mesa_dei_t,     0, VTSS_DEIS - 1 >);
        return itr(prev_id, next_id, prev_pcp, next_pcp, prev_dei, next_dei);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_ingress_map_pcp_dei_conf_get(mesa_qos_ingress_map_id_t          id,
                                                   mesa_tagprio_t                     pcp,
                                                   mesa_dei_t                         dei,
                                                   vtss_appl_qos_ingress_map_values_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, id: %u, pcp: %u, dei: %u", id, pcp, dei);

    if (!CAPA->has_ingress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if ((id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) ||
        (pcp >= VTSS_PCP_END) || (dei >= VTSS_DEI_END)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_imap[id].used) {
        *conf = qos.qos_imap[id].pcp_dei[pcp][dei];
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'get' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_ingress_map_pcp_dei_conf_set(mesa_qos_ingress_map_id_t                id,
                                                   mesa_tagprio_t                           pcp,
                                                   mesa_dei_t                               dei,
                                                   const vtss_appl_qos_ingress_map_values_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc                    rc = VTSS_RC_OK;
    vtss_appl_qos_imap_entry_t new_conf = {0};

    T_D("enter, id: %u, pcp: %u, dei: %u", id, pcp, dei);

    if (!CAPA->has_ingress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if ((id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) ||
        (pcp >= VTSS_PCP_END) || (dei >= VTSS_DEI_END)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((rc = QOS_imap_values_check(conf)) != VTSS_RC_OK) {
        return rc;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_imap[id].used) {
        new_conf                   = qos.qos_imap[id];
        new_conf.pcp_dei[pcp][dei] = *conf;
        if (memcmp(&new_conf, &qos.qos_imap[id], sizeof(new_conf)) ) { // Only if new_conf differs from current conf
            if ((rc = QOS_imap_add(id, &new_conf)) == VTSS_RC_OK) {
                qos.qos_imap[id] = new_conf; // Update current conf
            }
        }
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'set' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_ingress_map_dscp_conf_itr(const mesa_qos_ingress_map_id_t *prev_id,   mesa_qos_ingress_map_id_t *next_id,
                                                const mesa_dscp_t               *prev_dscp, mesa_dscp_t               *next_dscp)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_ingress_map) {
        vtss::IteratorComposeN<mesa_qos_ingress_map_id_t, mesa_dscp_t> itr(&vtss_appl_qos_ingress_map_conf_itr,
                                                                           &vtss::expose::snmp::IteratorComposeStaticRange < mesa_dscp_t, 0, 63 > );
        return itr(prev_id, next_id, prev_dscp, next_dscp);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_ingress_map_dscp_conf_get(mesa_qos_ingress_map_id_t          id,
                                                mesa_dscp_t                        dscp,
                                                vtss_appl_qos_ingress_map_values_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, id: %u, dscp: %u", id, dscp);

    if (!CAPA->has_ingress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if ((id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) ||
        (dscp >= 64)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_imap[id].used) {
        *conf = qos.qos_imap[id].dscp[dscp];
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'get' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_ingress_map_dscp_conf_set(mesa_qos_ingress_map_id_t                id,
                                                mesa_dscp_t                              dscp,
                                                const vtss_appl_qos_ingress_map_values_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc                    rc = VTSS_RC_OK;
    vtss_appl_qos_imap_entry_t new_conf = {0};

    T_D("enter, id: %u, dscp: %u", id, dscp);

    if (!CAPA->has_ingress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if ((id >= fast_cap(MESA_CAP_QOS_INGRESS_MAP_CNT)) ||
        (dscp >= 64)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((rc = QOS_imap_values_check(conf)) != VTSS_RC_OK) {
        return rc;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_imap[id].used) {
        new_conf            = qos.qos_imap[id];
        new_conf.dscp[dscp] = *conf;
        if (memcmp(&new_conf, &qos.qos_imap[id], sizeof(new_conf)) ) { // Only if new_conf differs from current conf
            if ((rc = QOS_imap_add(id, &new_conf)) == VTSS_RC_OK) {
                qos.qos_imap[id] = new_conf; // Update current conf
            }
        }
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'set' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

/****************************************************************************/
/*  Egress map functions                                                   */
/****************************************************************************/

#if defined(VTSS_SW_OPTION_QOS_ADV)
/* Get QoS egress map default configuration */
static mesa_rc QOS_emap_get_default(mesa_qos_egress_map_id_t id, vtss_appl_qos_emap_entry_t *entry)
{
    memset(entry, 0, sizeof(*entry)); // Default everything to zero.
    return VTSS_RC_OK;
}

/* Check QoS egress map configuration */
static mesa_rc QOS_emap_conf_check(const vtss_appl_qos_egress_map_conf_t *conf)
{
    if ((conf->key != VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID)       &&
        (conf->key != VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID_DPL)   &&
        (conf->key != VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP)        &&
        (conf->key != VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP_DPL)) {
        T_I("Invalid ingress map key: %u!", conf->key);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    return VTSS_RC_OK;
}

/* Check QoS egress map values */
static mesa_rc QOS_emap_values_check(const vtss_appl_qos_egress_map_values_t *values)
{
    if (values->pcp >= VTSS_PCP_END) {
        T_I("Invalid pcp: %u!", values->pcp);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (values->dei >= VTSS_DEI_END) {
        T_I("Invalid dei: %u!", values->dei);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    if (values->dscp > 63) {
        T_I("Invalid dscp: %u!", values->dscp);
        return VTSS_APPL_QOS_ERROR_PARM;
    }
    return VTSS_RC_OK;
}

static mesa_rc QOS_emap_add(mesa_qos_egress_map_id_t         id,
                            const vtss_appl_qos_emap_entry_t *entry)
{
    int                   cosid, dpl, dscp;
    mesa_qos_egress_map_t api_conf = {0};

    api_conf.id = id;

    switch (entry->conf.key) {
    case VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID:
        api_conf.key = MESA_QOS_EGRESS_MAP_KEY_COSID;
        for (cosid = 0; cosid < VTSS_COSIDS; cosid++) {
            api_conf.maps.cosid[cosid].pcp        = entry->cosid_dpl[cosid][0].pcp;
            api_conf.maps.cosid[cosid].dei        = entry->cosid_dpl[cosid][0].dei;
            api_conf.maps.cosid[cosid].dscp       = entry->cosid_dpl[cosid][0].dscp;
        }
        break;
    case VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID_DPL:
        api_conf.key = MESA_QOS_EGRESS_MAP_KEY_COSID_DPL;
        for (cosid = 0; cosid < VTSS_COSIDS; cosid++) {
            for (dpl = 0; dpl < fast_cap(MESA_CAP_QOS_DPL_CNT); dpl++) {
                api_conf.maps.cosid_dpl[cosid][dpl].pcp        = entry->cosid_dpl[cosid][dpl].pcp;
                api_conf.maps.cosid_dpl[cosid][dpl].dei        = entry->cosid_dpl[cosid][dpl].dei;
                api_conf.maps.cosid_dpl[cosid][dpl].dscp       = entry->cosid_dpl[cosid][dpl].dscp;
            }
        }
        break;
    case VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP:
        api_conf.key = MESA_QOS_EGRESS_MAP_KEY_DSCP;
        for (dscp = 0; dscp < 64; dscp++) {
            api_conf.maps.dscp[dscp].pcp        = entry->dscp_dpl[dscp][0].pcp;
            api_conf.maps.dscp[dscp].dei        = entry->dscp_dpl[dscp][0].dei;
            api_conf.maps.dscp[dscp].dscp       = entry->dscp_dpl[dscp][0].dscp;
        }
        break;
    case VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP_DPL:
        api_conf.key = MESA_QOS_EGRESS_MAP_KEY_DSCP_DPL;
        for (dscp = 0; dscp < 64; dscp++) {
            for (dpl = 0; dpl < fast_cap(MESA_CAP_QOS_DPL_CNT); dpl++) {
                api_conf.maps.dscp_dpl[dscp][dpl].pcp        = entry->dscp_dpl[dscp][dpl].pcp;
                api_conf.maps.dscp_dpl[dscp][dpl].dei        = entry->dscp_dpl[dscp][dpl].dei;
                api_conf.maps.dscp_dpl[dscp][dpl].dscp       = entry->dscp_dpl[dscp][dpl].dscp;
            }
        }
        break;
    }

    api_conf.action.pcp     = entry->conf.action.pcp;
    api_conf.action.dei     = entry->conf.action.dei;
    api_conf.action.dscp    = entry->conf.action.dscp;

    return mesa_qos_egress_map_add(NULL, &api_conf);
}

static mesa_rc QOS_egress_map_conf_add_or_set(mesa_qos_egress_map_id_t              id,
                                              const vtss_appl_qos_egress_map_conf_t *conf,
                                              BOOL add)
{
    mesa_rc                    rc = VTSS_RC_OK;
    vtss_appl_qos_emap_entry_t new_conf = {0};

    T_D("enter, id: %u", id);

    VTSS_ASSERT(conf);

    if (id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((rc = QOS_emap_conf_check(conf)) != VTSS_RC_OK) {
        return rc;
    }

    QOS_CRIT_ENTER();
    if (add || qos.qos_emap[id].used) {
        new_conf      = qos.qos_emap[id];
        new_conf.used = TRUE;
        new_conf.conf = *conf;
        if (memcmp(&new_conf, &qos.qos_emap[id], sizeof(new_conf)) ) { // Only if new_conf differs from current conf
            if ((rc = QOS_emap_add(id, &new_conf)) == VTSS_RC_OK) {
                qos.qos_emap[id] = new_conf; // Update current conf
            }
        }
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'set' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
}

mesa_rc vtss_appl_qos_emap_entry_get(mesa_qos_egress_map_id_t   id,
                                     vtss_appl_qos_emap_entry_t *entry)
{
    T_D("enter, id: %u", id);

    VTSS_ASSERT(entry);

    if (id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    *entry = qos.qos_emap[id];
    QOS_CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_qos_emap_entry_get_default(mesa_qos_egress_map_id_t  id,
                                             vtss_appl_qos_emap_entry_t *entry)
{
    T_D("enter, id: %u", id);

    VTSS_ASSERT(entry);

    if (id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    return QOS_emap_get_default(id, entry);
}
#endif /* VTSS_SW_OPTION_QOS_ADV */

mesa_rc vtss_appl_qos_egress_map_preset(mesa_qos_egress_map_id_t id,
                                        u8                       classes,
                                        BOOL                     color_aware)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    int                        cosid, dpl;
    mesa_rc                    rc = VTSS_RC_OK;
    vtss_appl_qos_emap_entry_t new_conf = {0};

    T_D("enter, id: %u, classes: %u, color_aware: %d", id, classes, color_aware);

    if (!CAPA->has_egress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    if (id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((classes < 1) || (classes > VTSS_COSIDS)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    new_conf.used = TRUE;

    new_conf.conf.key = color_aware ? VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID_DPL : VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID;

    new_conf.conf.action.pcp = new_conf.conf.action.dei = TRUE;

    /* See DS1198 section 9.1.2 for details regarding the mapping */
    for (dpl = 0; dpl < fast_cap(MESA_CAP_QOS_DPL_CNT); dpl++) {
        switch (classes) {
        case 1:
            new_conf.cosid_dpl[0][dpl].pcp = 0;
            new_conf.cosid_dpl[1][dpl].pcp = 7;
            new_conf.cosid_dpl[2][dpl].pcp = 7;
            new_conf.cosid_dpl[3][dpl].pcp = 7;
            new_conf.cosid_dpl[4][dpl].pcp = 7;
            new_conf.cosid_dpl[5][dpl].pcp = 7;
            new_conf.cosid_dpl[6][dpl].pcp = 7;
            new_conf.cosid_dpl[7][dpl].pcp = 7;
            break;
        case 2:
            new_conf.cosid_dpl[0][dpl].pcp = 0;
            new_conf.cosid_dpl[1][dpl].pcp = 4;
            new_conf.cosid_dpl[2][dpl].pcp = 7;
            new_conf.cosid_dpl[3][dpl].pcp = 7;
            new_conf.cosid_dpl[4][dpl].pcp = 7;
            new_conf.cosid_dpl[5][dpl].pcp = 7;
            new_conf.cosid_dpl[6][dpl].pcp = 7;
            new_conf.cosid_dpl[7][dpl].pcp = 7;
            break;
        case 3:
            new_conf.cosid_dpl[0][dpl].pcp = 0;
            new_conf.cosid_dpl[1][dpl].pcp = 4;
            new_conf.cosid_dpl[2][dpl].pcp = 6;
            new_conf.cosid_dpl[3][dpl].pcp = 7;
            new_conf.cosid_dpl[4][dpl].pcp = 7;
            new_conf.cosid_dpl[5][dpl].pcp = 7;
            new_conf.cosid_dpl[6][dpl].pcp = 7;
            new_conf.cosid_dpl[7][dpl].pcp = 7;
            break;
        case 4:
            new_conf.cosid_dpl[0][dpl].pcp = 0;
            new_conf.cosid_dpl[1][dpl].pcp = 2;
            new_conf.cosid_dpl[2][dpl].pcp = 4;
            new_conf.cosid_dpl[3][dpl].pcp = 6;
            new_conf.cosid_dpl[4][dpl].pcp = 7;
            new_conf.cosid_dpl[5][dpl].pcp = 7;
            new_conf.cosid_dpl[6][dpl].pcp = 7;
            new_conf.cosid_dpl[7][dpl].pcp = 7;
            break;
        case 5:
            new_conf.cosid_dpl[0][dpl].pcp = 0;
            new_conf.cosid_dpl[1][dpl].pcp = 2;
            new_conf.cosid_dpl[2][dpl].pcp = 4;
            new_conf.cosid_dpl[3][dpl].pcp = 6;
            new_conf.cosid_dpl[4][dpl].pcp = 7;
            new_conf.cosid_dpl[5][dpl].pcp = 7;
            new_conf.cosid_dpl[6][dpl].pcp = 7;
            new_conf.cosid_dpl[7][dpl].pcp = 7;
            break;
        case 6:
            new_conf.cosid_dpl[0][dpl].pcp = 1;
            new_conf.cosid_dpl[1][dpl].pcp = 0;
            new_conf.cosid_dpl[2][dpl].pcp = 2;
            new_conf.cosid_dpl[3][dpl].pcp = 4;
            new_conf.cosid_dpl[4][dpl].pcp = 6;
            new_conf.cosid_dpl[5][dpl].pcp = 7;
            new_conf.cosid_dpl[6][dpl].pcp = 7;
            new_conf.cosid_dpl[7][dpl].pcp = 7;
            break;
        case 7:
            new_conf.cosid_dpl[0][dpl].pcp = 1;
            new_conf.cosid_dpl[1][dpl].pcp = 0;
            new_conf.cosid_dpl[2][dpl].pcp = 2;
            new_conf.cosid_dpl[3][dpl].pcp = 3;
            new_conf.cosid_dpl[4][dpl].pcp = 4;
            new_conf.cosid_dpl[5][dpl].pcp = 6;
            new_conf.cosid_dpl[6][dpl].pcp = 7;
            new_conf.cosid_dpl[7][dpl].pcp = 7;
            break;
        case 8:
            new_conf.cosid_dpl[0][dpl].pcp = 1;
            new_conf.cosid_dpl[1][dpl].pcp = 0;
            new_conf.cosid_dpl[2][dpl].pcp = 2;
            new_conf.cosid_dpl[3][dpl].pcp = 3;
            new_conf.cosid_dpl[4][dpl].pcp = 4;
            new_conf.cosid_dpl[5][dpl].pcp = 5;
            new_conf.cosid_dpl[6][dpl].pcp = 6;
            new_conf.cosid_dpl[7][dpl].pcp = 7;
            break;
        default:
            return VTSS_APPL_QOS_ERROR_PARM;
        }
    }

    if (color_aware) {
        for (cosid = 0; cosid < VTSS_COSIDS; cosid++) {
            for (dpl = 1; dpl < fast_cap(MESA_CAP_QOS_DPL_CNT); dpl++) {
                new_conf.cosid_dpl[cosid][dpl].dei = 1;
            }
        }
    }

    QOS_CRIT_ENTER();
    if (memcmp(&new_conf, &qos.qos_emap[id], sizeof(new_conf)) ) { // Only if new_conf differs from current conf
        if ((rc = QOS_emap_add(id, &new_conf)) == VTSS_RC_OK) {
            qos.qos_emap[id] = new_conf; // Update current conf
        }
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_egress_map_conf_itr(const mesa_qos_egress_map_id_t *prev_id, mesa_qos_egress_map_id_t *next_id)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_qos_egress_map_id_t id, id_start = (prev_id == NULL) ? 0 : *prev_id + 1;
    mesa_rc rc = VTSS_RC_ERROR;

    if (!CAPA->has_egress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    QOS_CRIT_ENTER();
    for (id = id_start; id < fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT); id++) {
        if (qos.qos_emap[id].used) {
            *next_id = id;
            rc = VTSS_RC_OK;
            break;
        }
    }
    QOS_CRIT_EXIT();
    T_D("exit %s, prev_id: %d, next_id: %d", (rc == VTSS_RC_OK) ? "OK" : "ERROR", (prev_id == NULL) ? -1 : *prev_id, (next_id == NULL) ? -1 : *next_id);
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_egress_map_conf_add(mesa_qos_egress_map_id_t              id,
                                          const vtss_appl_qos_egress_map_conf_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_egress_map) {
        return QOS_egress_map_conf_add_or_set(id, conf, TRUE);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_egress_map_conf_del(const mesa_qos_egress_map_id_t id)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, id: %u", id);

    if (!CAPA->has_egress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    if (id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_emap[id].used) {
        QOS_emap_get_default(id, &qos.qos_emap[id]);
        qos.qos_emap[id].used = FALSE;
        rc = mesa_qos_egress_map_del(NULL, id);
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'del' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_egress_map_conf_get(mesa_qos_egress_map_id_t        id,
                                          vtss_appl_qos_egress_map_conf_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, id: %u", id);

    if (!CAPA->has_egress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if (id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_emap[id].used) {
        *conf = qos.qos_emap[id].conf;
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'get' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_egress_map_conf_set(mesa_qos_egress_map_id_t              id,
                                          const vtss_appl_qos_egress_map_conf_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_egress_map) {
        return QOS_egress_map_conf_add_or_set(id, conf, FALSE);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_egress_map_cosid_dpl_conf_itr(const mesa_qos_egress_map_id_t *prev_id,    mesa_qos_egress_map_id_t *next_id,
                                                    const mesa_cosid_t             *prev_cosid, mesa_cosid_t             *next_cosid,
                                                    const mesa_dpl_t               *prev_dpl,   mesa_dpl_t               *next_dpl)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_egress_map) {
        vtss::IteratorComposeN<mesa_qos_egress_map_id_t, mesa_cosid_t, mesa_dpl_t> itr(&vtss_appl_qos_egress_map_conf_itr,
                                                                                       &vtss::expose::snmp::IteratorComposeStaticRange<mesa_cosid_t, 0, VTSS_COSIDS - 1 >,
                                                                                       &vtss_appl_qos_dpl_itr);
        return itr(prev_id, next_id, prev_cosid, next_cosid, prev_dpl, next_dpl);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_egress_map_cosid_dpl_conf_get(mesa_qos_egress_map_id_t          id,
                                                     mesa_cosid_t                      cosid,
                                                     mesa_dpl_t                        dpl,
                                                     vtss_appl_qos_egress_map_values_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, id: %u, cosid: %u, dpl: %u", id, cosid, dpl);

    if (!CAPA->has_egress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if ((id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) ||
        (cosid >= VTSS_COSIDS) || (dpl >= fast_cap(MESA_CAP_QOS_DPL_CNT))) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_emap[id].used) {
        *conf = qos.qos_emap[id].cosid_dpl[cosid][dpl];
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'get' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_egress_map_cosid_dpl_conf_set(mesa_qos_egress_map_id_t                id,
                                                    mesa_cosid_t                            cosid,
                                                    mesa_dpl_t                              dpl,
                                                    const vtss_appl_qos_egress_map_values_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc                    rc = VTSS_RC_OK;
    vtss_appl_qos_emap_entry_t new_conf = {0};

    T_D("enter, id: %u, cosid: %u, dpl: %u", id, cosid, dpl);

    if (!CAPA->has_egress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if ((id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) ||
        (cosid >= VTSS_COSIDS) || (dpl >= fast_cap(MESA_CAP_QOS_DPL_CNT))) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((rc = QOS_emap_values_check(conf)) != VTSS_RC_OK) {
        return rc;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_emap[id].used) {
        new_conf                       = qos.qos_emap[id];
        new_conf.cosid_dpl[cosid][dpl] = *conf;
        if (memcmp(&new_conf, &qos.qos_emap[id], sizeof(new_conf)) ) { // Only if new_conf differs from current conf
            if ((rc = QOS_emap_add(id, &new_conf)) == VTSS_RC_OK) {
                qos.qos_emap[id] = new_conf; // Update current conf
            }
        }
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'set' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_egress_map_dscp_dpl_conf_itr(const mesa_qos_egress_map_id_t *prev_id,   mesa_qos_egress_map_id_t *next_id,
                                                   const mesa_dscp_t              *prev_dscp, mesa_dscp_t              *next_dscp,
                                                   const mesa_dpl_t               *prev_dpl,  mesa_dpl_t               *next_dpl)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_egress_map) {
        vtss::IteratorComposeN<mesa_qos_egress_map_id_t, mesa_dscp_t, mesa_dpl_t> itr(&vtss_appl_qos_egress_map_conf_itr,
                                                                                      &vtss::expose::snmp::IteratorComposeStaticRange<mesa_dscp_t, 0, 63 >,
                                                                                      &vtss_appl_qos_dpl_itr);
        return itr(prev_id, next_id, prev_dscp, next_dscp, prev_dpl, next_dpl);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */
    return VTSS_APPL_QOS_ERROR_FEATURE;
}

mesa_rc vtss_appl_qos_egress_map_dscp_dpl_conf_get(mesa_qos_egress_map_id_t          id,
                                                   mesa_dscp_t                       dscp,
                                                   mesa_dpl_t                        dpl,
                                                   vtss_appl_qos_egress_map_values_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, id: %u, dscp: %u, dpl: %u", id, dscp, dpl);

    if (!CAPA->has_egress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    VTSS_ASSERT(conf);

    if ((id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) ||
        (dscp >= 64) || (dpl >= fast_cap(MESA_CAP_QOS_DPL_CNT))) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_emap[id].used) {
        *conf = qos.qos_emap[id].dscp_dpl[dscp][dpl];
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'get' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

mesa_rc vtss_appl_qos_egress_map_dscp_dpl_conf_set(mesa_qos_egress_map_id_t                id,
                                                   mesa_dscp_t                             dscp,
                                                   mesa_dpl_t                              dpl,
                                                   const vtss_appl_qos_egress_map_values_t *conf)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_rc                    rc = VTSS_RC_OK;
    vtss_appl_qos_emap_entry_t new_conf = {0};

    T_D("enter, id: %u, dscp: %u, dpl: %u", id, dscp, dpl);

    VTSS_ASSERT(conf);

    if (!CAPA->has_egress_map) {
        return VTSS_APPL_QOS_ERROR_FEATURE;
    }

    if ((id >= fast_cap(MESA_CAP_QOS_EGRESS_MAP_CNT)) ||
        (dscp >= 64) || (dpl >= fast_cap(MESA_CAP_QOS_DPL_CNT))) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if ((rc = QOS_emap_values_check(conf)) != VTSS_RC_OK) {
        return rc;
    }

    QOS_CRIT_ENTER();
    if (qos.qos_emap[id].used) {
        new_conf                     = qos.qos_emap[id];
        new_conf.dscp_dpl[dscp][dpl] = *conf;
        if (memcmp(&new_conf, &qos.qos_emap[id], sizeof(new_conf)) ) { // Only if new_conf differs from current conf
            if ((rc = QOS_emap_add(id, &new_conf)) == VTSS_RC_OK) {
                qos.qos_emap[id] = new_conf; // Update current conf
            }
        }
    } else {
        rc = VTSS_RC_ERROR; // Tried to do a 'set' on an unused entry
    }
    QOS_CRIT_EXIT();

    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* VTSS_SW_OPTION_QOS_ADV */
}

/****************************************************************************/
/*  QCE functions                                                           */
/****************************************************************************/
mesa_rc vtss_appl_qos_qce_precedence_itr(const u32 *prev_index, u32 *next_index)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    mesa_rc                         rc   = VTSS_RC_OK;
    u32                             ix   = (prev_index) ? *prev_index + 1 : 1;
    vtss_appl_qos_qce_intern_conf_t conf;

    T_D("enter, prev_index: %d", (prev_index) ? *prev_index : -1);

    if (vtss_appl_qos_qce_intern_get_nth(VTSS_ISID_GLOBAL, ix, &conf) == VTSS_RC_OK) {
        *next_index = ix;
    } else { // Done
        rc = VTSS_RC_ERROR;
    }
    if (rc == VTSS_RC_OK) {
        T_D("exit, next_index: %d", (next_index) ? *next_index : -1);
    } else {
        T_D("exit, no more entries");
    }
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_precedence_get(u32 ix, vtss_appl_qos_qce_conf_t *conf)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    vtss_appl_qos_qce_intern_conf_t iconf;

    T_D("enter, ix: %u", ix);
    VTSS_RC(vtss_appl_qos_qce_intern_get_nth(VTSS_ISID_GLOBAL, ix, &iconf));
    VTSS_RC(QOS_qce_intern_conf2conf(&iconf, conf));
    return VTSS_RC_OK;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
/*!
 * \brief Get next usid.
 *
 * \param prev_usid [IN]  The precious usid. Use 0 to get first.
 * \return Returns next usid or 0 if no next usid.
 **/
static vtss_usid_t QOS_usid_get_next(vtss_usid_t prev_usid)
{
    vtss_usid_t s, next_usid = 0;
    u32         exists_mask = msg_existing_switches();

    T_N("enter, prev_usid: %u", prev_usid);
    for (s = (prev_usid + 1); s < VTSS_USID_END; s++) {
        if (exists_mask & (1LU << topo_usid2isid(s))) {
            next_usid = s;
            break;
        }
    }
    T_N("exit, next_usid: %u", next_usid);
    return next_usid;
}
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */

mesa_rc vtss_appl_qos_qce_status_itr(const vtss_usid_t *prev_usid,  vtss_usid_t *next_usid,
                                     const u32         *prev_index, u32         *next_index)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    mesa_rc                         rc   = VTSS_RC_OK;
    vtss_usid_t                     usid = (prev_usid)  ? *prev_usid  : QOS_usid_get_next(0);
    u32                             ix   = (prev_index) ? *prev_index + 1 : 1;
    vtss_appl_qos_qce_intern_conf_t conf;

    T_D("enter, prev_usid: %d, prev_index: %d", (prev_usid) ? *prev_usid : -1, (prev_index) ? *prev_index : -1);

    if (vtss_appl_qos_qce_intern_get_nth(topo_usid2isid(usid), ix, &conf) == VTSS_RC_OK) {
        *next_usid = usid;
        *next_index = ix;
    } else { // Switch to next usid
        *next_index = 1;
        usid = QOS_usid_get_next(usid);
        if (usid) {
            *next_usid = usid;
        } else {
            rc = VTSS_RC_ERROR;
        }
    }
    if (rc == VTSS_RC_OK) {
        T_D("exit, next_usid: %d, next_index: %d", (next_usid) ? *next_usid : -1, (next_index) ? *next_index : -1);
    } else {
        T_D("exit, no more entries");
    }
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_status_get(vtss_usid_t usid, u32 ix, vtss_appl_qos_qce_conf_t *status)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    vtss_appl_qos_qce_intern_conf_t conf;

    T_D("enter, usid: %u, ix: %u", usid, ix);
    VTSS_RC(vtss_appl_qos_qce_intern_get_nth(topo_usid2isid(usid), ix, &conf));
    VTSS_RC(QOS_qce_intern_conf2conf(&conf, status));
    return VTSS_RC_OK;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_intern_get(vtss_isid_t                     isid,
                                     vtss_appl_qos_qcl_user_t        user_id,
                                     mesa_qce_id_t                   qce_id,
                                     vtss_appl_qos_qce_intern_conf_t *conf,
                                     BOOL                            next)
{
    mesa_rc rc = VTSS_RC_OK;

    T_N("enter, isid: %u, user_id: %d, qce_id: %u, next: %d", isid, user_id, qce_id, next);

    /* Check switch id */
    if (QOS_isid_invalid(isid)) {
        return VTSS_APPL_QOS_ERROR_STACK_STATE;
    }

    /* Check user id */
    if ((user_id < 0) || (user_id >= VTSS_APPL_QOS_QCL_USER_CNT)) {
        T_I("Invalid user_id: %d", user_id);
        return VTSS_APPL_QOS_ERROR_QCL_USER_NOT_FOUND;
    }

    /* Check QCE id */
    if (qce_id > VTSS_APPL_QOS_QCE_ID_END) {
        T_I("Invalid qce_id: %d", qce_id);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    if (msg_switch_is_local(isid)) {
        isid = VTSS_ISID_LOCAL;
    }

    rc = QOS_list_qce_entry_get(isid, QCL_COMBINED_ID_SET(user_id, qce_id), conf, next);

    conf->qce.id = QCL_QCE_ID_GET(conf->qce.id); /* Restore independent QCE ID */

    T_N("exit");
    return rc;
}

mesa_rc vtss_appl_qos_qce_intern_get_nth(vtss_isid_t                     isid,
                                         u32                             ix,
                                         vtss_appl_qos_qce_intern_conf_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;

    T_N("enter, isid: %u, index: %u", isid, ix);

    /* Check switch id */
    if (QOS_isid_invalid(isid)) {
        return VTSS_APPL_QOS_ERROR_STACK_STATE;
    }

    if (msg_switch_is_local(isid)) {
        isid = VTSS_ISID_LOCAL;
    }

    rc = QOS_list_qce_entry_get_nth(isid, ix, conf);

    conf->qce.id = QCL_QCE_ID_GET(conf->qce.id); /* Restore independent QCE ID */

    T_N("exit");
    return rc;
}

mesa_rc vtss_appl_qos_qce_intern_add(mesa_qce_id_t                   next_qce_id,
                                     vtss_appl_qos_qce_intern_conf_t *conf)
{
    qcl_qce_list_table_t *list;
    mesa_rc              rc;
    vtss_isid_t          isid_del;
    BOOL                 found;
    BOOL                 conflict;

    T_N("enter, next_qce_id: %u, isid: %u, user_id: %d, qce_id: %u", next_qce_id, conf->isid, conf->user_id, conf->qce.id);

    /* Check switch id */
    if (QOS_isid_invalid(conf->isid)) {
        return VTSS_APPL_QOS_ERROR_STACK_STATE;
    }

    /* Check user id */
    /* Suppress Lint Warning 568: non-negative quantity is never less than zero */
    /*lint -e{568} */
    if ((conf->user_id < 0) || (conf->user_id >= VTSS_APPL_QOS_QCL_USER_CNT)) {
        T_I("Invalid user_id: %d", conf->user_id);
        return VTSS_APPL_QOS_ERROR_QCL_USER_NOT_FOUND;
    }

    /* Check QCE */
    if ((rc = QOS_qce_check(next_qce_id, &conf->qce)) != VTSS_RC_OK) {
        return rc;
    }

    next_qce_id = QCL_COMBINED_ID_SET(conf->user_id, next_qce_id); /* Convert to combined ID */
    conf->qce.id = QCL_COMBINED_ID_SET(conf->user_id, conf->qce.id); /* Convert to combined ID */

    if (conf->isid == VTSS_ISID_LOCAL) {
        QOS_CRIT_ENTER();
        list = &qos.qcl_qce_switch_list; /* Use local switch list */
        rc = QOS_list_qce_entry_add(list, &next_qce_id, conf, &isid_del, &found, &conflict, 1);
        if (rc == VTSS_RC_OK) {
            QOS_switch_qce_entry_add(next_qce_id, conf, found, conflict);
        }
        QOS_CRIT_EXIT();
    } else {
        QOS_CRIT_ENTER();
        list = &qos.qcl_qce_stack_list; /* Use global stack list */
        rc = QOS_list_qce_entry_add(list, &next_qce_id, conf, &isid_del, &found, &conflict, 0);
        if (rc == VTSS_RC_OK) {
            if (isid_del != VTSS_ISID_LOCAL) {
                QOS_stack_qce_entry_del(isid_del, conf->qce.id);
            }
            QOS_stack_qce_entry_add(conf->isid, conf, TRUE);
        }
        QOS_CRIT_EXIT();
    }

    conf->qce.id = QCL_QCE_ID_GET(conf->qce.id); /* Restore independent QCE ID */

    T_N("exit");
    return rc;
}

mesa_rc vtss_appl_qos_qce_intern_del(vtss_isid_t              isid,
                                     vtss_appl_qos_qcl_user_t user_id,
                                     mesa_qce_id_t            qce_id)
{
    qcl_qce_list_table_t *list;
    mesa_rc               rc;
    vtss_isid_t           isid_del;
    BOOL                  conflict = TRUE;

    T_N("enter, isid: %d, user_id: %d, qce_id: %d", isid, user_id, qce_id);

    /* Check switch id */
    if (QOS_isid_invalid(isid)) {
        return VTSS_APPL_QOS_ERROR_STACK_STATE;
    }

    /* Check user id */
    if ((user_id < 0) || (user_id >= VTSS_APPL_QOS_QCL_USER_CNT)) {
        T_I("Invalid user_id: %d", user_id);
        return VTSS_APPL_QOS_ERROR_QCL_USER_NOT_FOUND;
    }

    /* Check QCE id */
    if (qce_id > VTSS_APPL_QOS_QCE_ID_END) {
        T_I("Invalid qce_id: %d", qce_id);
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    qce_id = QCL_COMBINED_ID_SET(user_id, qce_id); /* Convert to combined ID */

    if (isid == VTSS_ISID_LOCAL) {
        QOS_CRIT_ENTER();
        list = &qos.qcl_qce_switch_list; /* Use local switch list */
        rc = QOS_list_qce_entry_del(list, qce_id, &isid_del, &conflict);

        if (rc == VTSS_RC_OK && conflict == FALSE) {
            /* Remove this entry from ASIC layer */
            if ((rc = mesa_qce_del(NULL, qce_id)) != VTSS_RC_OK) {
                T_W("Calling mesa_qce_del() failed\n");
            }
            QOS_qce_conflict_resolve();
        }
        QOS_CRIT_EXIT();
    } else {
        QOS_CRIT_ENTER();
        list = &qos.qcl_qce_stack_list; /* Use global stack list */
        rc = QOS_list_qce_entry_del(list, qce_id, &isid_del, &conflict);
        if (rc == VTSS_RC_OK) {
            QOS_stack_qce_entry_del(isid_del, qce_id);
        }
        QOS_CRIT_EXIT();
    }

    T_N("exit");
    return rc;
}

mesa_rc vtss_appl_qos_qce_intern_get_default(vtss_appl_qos_qce_intern_conf_t *conf)
{
    vtss_clear(*conf);
    conf->isid    = VTSS_ISID_GLOBAL;
    conf->user_id = VTSS_APPL_QOS_QCL_USER_STATIC;
    return mesa_qce_init(NULL, MESA_QCE_TYPE_ANY, &conf->qce);
}

/* Resolve QCE conflict */
mesa_rc vtss_appl_qos_qce_conflict_resolve(void)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    mesa_rc rc;

    T_N("enter");

    QOS_CRIT_ENTER();
    rc = QOS_stack_qce_conflict_resolve();
    QOS_CRIT_EXIT();

    T_N("exit");
    return rc;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_conflict_resolve_get(BOOL *conf)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    *conf = FALSE;
    return VTSS_RC_OK;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_conflict_resolve_set(const BOOL *conf)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    if (*conf) {
        return vtss_appl_qos_qce_conflict_resolve();
    } else {
        return VTSS_RC_OK;
    }
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_conf_itr(const mesa_qce_id_t *const prev_id,
                                    mesa_qce_id_t       *const next_id)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    if (next_id == NULL) {
        T_E("Invalid next_id!");
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    return QOS_list_qce_get_next_id(prev_id == NULL ? 0 : *prev_id, next_id);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_conf_set(mesa_qce_id_t                  qce_id,
                                   const vtss_appl_qos_qce_conf_t *conf)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    vtss_appl_qos_qce_intern_conf_t iconf;

    T_N("enter qce_id: %u", qce_id);

    VTSS_RC(vtss_appl_qos_qce_intern_get(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &iconf, 0));
    VTSS_RC(QOS_qce_conf2intern_conf(conf, &iconf, (conf->key.type != QOS_qce_type_api2appl(iconf.qce.key.type))));
    return vtss_appl_qos_qce_intern_add(conf->next_qce_id, &iconf);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_conf_add(mesa_qce_id_t                  qce_id,
                                   const vtss_appl_qos_qce_conf_t *conf)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    vtss_appl_qos_qce_intern_conf_t iconf;

    T_N("enter qce_id: %u, next_qce_id: %u", qce_id, conf->next_qce_id);

    VTSS_RC(vtss_appl_qos_qce_intern_get_default(&iconf));
    VTSS_RC(QOS_qce_conf2intern_conf(conf, &iconf, FALSE));
    iconf.qce.id = qce_id; /* Add the index key (qce_id) manually here */

    if (!VTSS_ISID_LEGAL(iconf.isid)) {
        iconf.isid = VTSS_ISID_GLOBAL; /* Add QCE on all switches */
    }
    return vtss_appl_qos_qce_intern_add(conf->next_qce_id, &iconf);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_conf_del(mesa_qce_id_t qce_id)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    T_N("enter qce_id: %u", qce_id);
    return vtss_appl_qos_qce_intern_del(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id);
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_conf_get(mesa_qce_id_t            qce_id,
                                   vtss_appl_qos_qce_conf_t *conf)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    vtss_appl_qos_qce_intern_conf_t iconf;
    mesa_qce_id_t                   next_qce_id;

    T_N("enter qce_id: %u", qce_id);

    next_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
    if (vtss_appl_qos_qce_intern_get(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &iconf, 1) == VTSS_RC_OK) {
        next_qce_id = iconf.qce.id;
    }
    VTSS_RC(vtss_appl_qos_qce_intern_get(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &iconf, 0));
    VTSS_RC(QOS_qce_intern_conf2conf(&iconf, conf));
    conf->next_qce_id = next_qce_id;
    return VTSS_RC_OK;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

mesa_rc vtss_appl_qos_qce_conf_get_default(vtss_appl_qos_qce_conf_t *conf)
{
#if defined(VTSS_APPL_QOS_QCL_INCLUDE)
    vtss_appl_qos_qce_intern_conf_t iconf;

    VTSS_RC(vtss_appl_qos_qce_intern_get_default(&iconf));
    VTSS_RC(QOS_qce_intern_conf2conf(&iconf, conf));
    conf->next_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
    return VTSS_RC_OK;
#else
    return VTSS_APPL_QOS_ERROR_FEATURE;
#endif /* defined(VTSS_APPL_QOS_QCL_INCLUDE) */
}

/****************************************************************************/
/*  Inter module support functions                                          */
/****************************************************************************/
/* Set port QoS volatile default priority. Use VTSS_PRIO_NO_NONE to disable */
mesa_rc qos_port_volatile_default_prio_set(vtss_isid_t isid, mesa_port_no_t port_no, mesa_prio_t default_prio)
{
    mesa_rc rc          = VTSS_RC_OK;
    int     port_no_idx = port_no - VTSS_PORT_NO_START;

    T_N("enter, isid: %u, port_no: %u, default_prio: %u", isid, port_no, default_prio);

    if (QOS_port_isid_invalid(isid, port_no, 1)) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CRIT_ENTER();

    /* Check if anything has changed */
    if (qos.volatile_default_prio[port_no_idx] == default_prio) {
        goto exit_func; // Nothing to do
    }
    qos.volatile_default_prio[port_no_idx] = default_prio;

    /* Apply new data to msg layer */
    QOS_stack_port_conf_set(isid, port_no);

exit_func:
    QOS_CRIT_EXIT();
    T_N("exit");
    return rc;
}

mesa_rc qos_port_conf_change_register(BOOL global, vtss_module_id_t module_id, qos_port_conf_change_cb_t callback)
{
    mesa_rc rc = VTSS_RC_OK;

    VTSS_ASSERT(callback);

    if (module_id >= VTSS_MODULE_ID_NONE) {
        return VTSS_APPL_QOS_ERROR_PARM;
    }

    QOS_CB_CRIT_ENTER();
    if (qos.rt.count < QOS_PORT_CONF_CHANGE_REG_MAX) {
        qos.rt.reg[qos.rt.count].global    = global;
        qos.rt.reg[qos.rt.count].module_id = module_id;
        qos.rt.reg[qos.rt.count].callback  = callback;
        qos.rt.count++;
    } else {
        T_E("qos port change table full!");
        rc = VTSS_APPL_QOS_ERROR_GEN;
    }
    QOS_CB_CRIT_EXIT();

    return rc;
}

mesa_rc qos_port_conf_change_reg_get(qos_port_conf_change_reg_t *entry, BOOL clear)
{
    mesa_rc rc = VTSS_APPL_QOS_ERROR_GEN;
    int     i;

    QOS_CB_CRIT_ENTER();
    for (i = 0; i < qos.rt.count; i++) {
        qos_port_conf_change_reg_t *reg = &qos.rt.reg[i];
        if (clear) {
            /* Clear all entries */
            reg->max_ticks = 0;
        } else if (entry->module_id == VTSS_MODULE_ID_NONE) {
            /* Get first */
            *entry = *reg;
            rc = VTSS_RC_OK;
            break;
        } else if (entry->global == reg->global && entry->module_id == reg->module_id && entry->callback == reg->callback) {
            /* Get next */
            entry->module_id = VTSS_MODULE_ID_NONE;
        }
    }
    QOS_CB_CRIT_EXIT();

    return rc;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
static void QOS_conf_default_all(void)
{
    port_iter_t               pit;
    vtss_appl_qos_port_conf_t qos_port_conf_new;
    qcl_qce_t                 *qce, *prev;

    QOS_CRIT_ENTER();

    // Global config
    QOS_conf_default_set(&qos.qos_conf);

    // Port config
    QOS_port_conf_default_set(&qos_port_conf_new);
    (void) port_iter_init_local_all(&pit);
    while (port_iter_getnext(&pit)) {
        qos.qos_port_conf[VTSS_ISID_START][pit.iport - VTSS_PORT_NO_START] = qos_port_conf_new;
    }

    // QCE config
    for (qce = qos.qcl_qce_stack_list.qce_used_list, prev = NULL; qce != NULL; prev = qce, qce = qce->next) {
    }
    /* prev now points to the last entry in qce_used_list (if any) */
    /* Insert qce_used_list to front of qce_free_list and mark qce_used_list */
    if (prev != NULL) {
        prev->next = qos.qcl_qce_stack_list.qce_free_list;
        qos.qcl_qce_stack_list.qce_free_list = qos.qcl_qce_stack_list.qce_used_list;
        qos.qcl_qce_stack_list.qce_used_list = NULL;
    }

#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (CAPA->has_ingress_map) {
        for (int id = 0; id <= CAPA->ingress_map_id_max; id++) {
            QOS_imap_get_default(id, &qos.qos_imap[id]);
        }
    }

    if (CAPA->has_egress_map) {
        for (int id = 0; id <= CAPA->egress_map_id_max; id++) {
            QOS_emap_get_default(id, &qos.qos_emap[id]);
        }
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */

    QOS_CRIT_EXIT();
}

/* Initialize QOS port volatile default prio to undefined */
static void QOS_port_volatile_default_prio_init(void)
{
    port_iter_t   pit;

    (void) port_iter_init_local_all(&pit);
    while (port_iter_getnext(&pit)) {
        qos.volatile_default_prio[pit.iport - VTSS_PORT_NO_START] = VTSS_PRIO_NO_NONE;
    }
}

/* Initialize QoS module */
static void QOS_local_init(void)
{
    int                           i;
    qcl_qce_t                     *qce;
    vtss_appl_port_capabilities_t cap;

    T_N("enter");

    /* Initialize capabilities */
    if (vtss_appl_qos_capabilities_get(&qos_capabilities) != VTSS_RC_OK) {
        T_E("Unable to initialize capabilities");
    }

    (void)vtss_appl_port_capabilities_get(&cap);
    qos_capabilities.has_port_policers_fc = (cap.aggr_caps & MEBA_PORT_CAP_FLOW_CTRL) != 0;

    /* Put all QCEs to the unused stack link list */
    qos.qcl_qce_stack_list.qce_free_list = NULL;
    /* allocate the memory at the beginning for MAX QCE in stack list */
    if ((VTSS_MALLOC_CAST(qos.qcl_qce_stack_list.qce_list, sizeof(qcl_qce_t) * VTSS_APPL_QOS_QCE_ID_END))) {
        for (i = 0; i < VTSS_APPL_QOS_QCE_ID_END; i++) {
            qce = qos.qcl_qce_stack_list.qce_list + i;
            qce->next = qos.qcl_qce_stack_list.qce_free_list;
            qos.qcl_qce_stack_list.qce_free_list = qce;
        }
    }
    qos.qcl_qce_stack_list.qce_used_list = NULL;

    /* Put all QCEs to the unused switch link list */
    qos.qcl_qce_switch_list.qce_free_list = NULL;
    /* allocate the memory at the beginning for MAX QCE in switch list */
    if ((VTSS_MALLOC_CAST(qos.qcl_qce_switch_list.qce_list, sizeof(qcl_qce_t) * (VTSS_APPL_QOS_QCE_MAX + VTSS_APPL_QOS_RESERVED_QCE_CNT)))) {
        for (i = 0; i < (VTSS_APPL_QOS_QCE_MAX + VTSS_APPL_QOS_RESERVED_QCE_CNT); i++) {
            qce = qos.qcl_qce_switch_list.qce_list + i;
            qce->next = qos.qcl_qce_switch_list.qce_free_list;
            qos.qcl_qce_switch_list.qce_free_list = qce;
        }
    }
    qos.qcl_qce_switch_list.qce_used_list = NULL;

    /* Create mutex for critical regions */
    critd_init(&qos.qos_crit, "qos", VTSS_MODULE_ID_QOS, CRITD_TYPE_MUTEX);

    /* Create mutex for change callback structure */
    critd_init(&qos.rt.crit, "qos.cb", VTSS_MODULE_ID_QOS, CRITD_TYPE_MUTEX);

    T_N("exit");
}

/* Initialize module */
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void qos_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_qos_json_init(void);
#endif
extern "C" int qos_icli_cmd_register();

mesa_rc qos_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        QOS_local_init();
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        qos_mib_init();  /* Register our private mib */
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_qos_json_init();
#endif
        qos_icli_cmd_register();
#ifdef VTSS_SW_OPTION_ICFG
        if (qos_icfg_init() != VTSS_RC_OK) {
            T_E("ICFG not initialized correctly");
        }
#endif
        break;

    case INIT_CMD_START:
        T_D("START");
        if (fast_cap(MESA_CAP_QOS_SHAPER_CALIBRATE)) {
            QOS_shaper_calibrate_init();
        }
        QOS_status_update_init();

        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_GLOBAL) {
            /* Reset configuration to default and apply it to switch */
            QOS_conf_default_all();
            QOS_CRIT_ENTER();
            QOS_stack_conf_set(VTSS_ISID_GLOBAL);
            QOS_stack_port_conf_set(VTSS_ISID_GLOBAL, VTSS_PORT_NO_NONE);
            QOS_stack_qce_clear_all(VTSS_ISID_GLOBAL);
#if defined(VTSS_SW_OPTION_QOS_ADV)
            if (CAPA->has_ingress_map) {
                (void)mesa_qos_ingress_map_del_all(NULL);
            }
            if (CAPA->has_egress_map) {
                (void)mesa_qos_egress_map_del_all(NULL);
            }
#endif /* VTSS_SW_OPTION_QOS_ADV */
            QOS_CRIT_EXIT();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        /* Initialize volatile default prio to default (disabled) */
        QOS_port_volatile_default_prio_init();
        /* Reset configuration to default and apply it to switch */
        QOS_conf_default_all();
        QOS_CRIT_ENTER();
        QOS_stack_conf_set(VTSS_ISID_GLOBAL);
        QOS_stack_port_conf_set(VTSS_ISID_GLOBAL, VTSS_PORT_NO_NONE);
        QOS_CRIT_EXIT();
        QOS_status_update_start();
        break;

    default:
        break;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

