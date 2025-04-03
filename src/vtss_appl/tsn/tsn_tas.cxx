/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <vtss/appl/tsn.h>        /* For ourselves (public header)           */
#include "tsn_api.h"              /* For ourselves (semi-public header)      */
#include "tsn_trace.h"            /* For our own trace definitions           */
#include "tsn_expose.hxx"         /* For our own notifications               */
#include "tsn_timer.hxx"          /* For tsn_timer                           */
#include "tsn_lock.hxx"           /* For TSN_LOCK_SCOPE()                    */
#include <vtss_tod_api.h>         /* vtss_tod_sub_TimeStamps()               */
#include "vtss_common_iterator.hxx" /* For vtss_appl_iterator_ifindex_front_port */
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-depend-N.hxx"

/******************************************************************************/
// Global variables
/******************************************************************************/

extern uint32_t                               TSN_cap_port_cnt;
extern vtss_appl_tsn_capabilities_t           TSN_cap;
static tsn_timer_t                            TSN_TAS_poll_timer;

// Used to hold state values
typedef struct {
    bool config_change_performed;
    vtss_appl_tsn_tas_cfg_t cfg;
    CapArray<vtss_appl_tsn_tas_gcl_t,   MESA_CAP_QOS_TAS_GCE_CNT> gcl;
} vtss_appl_tsn_tas_state_t;

// Used to hold configured values
typedef struct {
    vtss_appl_tsn_tas_cfg_t cfg;
    uint16_t max_sdu[VTSS_QUEUE_ARRAY_SIZE];
    CapArray<vtss_appl_tsn_tas_gcl_t,   MESA_CAP_QOS_TAS_GCE_CNT> gcl;
} vtss_tsn_tas_config_t;

// Used to hold oper state
typedef struct {
    uint64_t         config_change_error;
    uint32_t         oper_control_list_length;
    uint32_t         oper_cycle_time_numerator;
    uint32_t         oper_cycle_time_denominator;
    uint32_t         oper_cycle_time_extension;
    mesa_timestamp_t oper_base_time;
} vtss_tsn_tas_oper_state_t;

typedef struct {
    vtss_tsn_tas_oper_state_t status;
    CapArray<vtss_appl_tsn_tas_gcl_t,   MESA_CAP_QOS_TAS_GCE_CNT> gcl;
} vtss_tsn_tas_oper_t;

/* TSN global structure:
   The TAS configuration contains a timestamp specifying when the
   configuration should take effect. That allows matching
   configurations to be built up on several devices and allow them all
   to apply the new configuration at the same time. It also make the
   state of TAS a bit tricky:

   - There is the configuration that is being built up (tas_conf)

   - There is the configuration that has been configured in hardware
   with a config-change but not yet in effect due to admin_base_time
   being in the future (tas_state)

   - There is the configuration currently in effect. (tas_oper)

   The configuration from tas_conf is transferred to tas_state when
   config-change is applied.

   The configuration from tas_state is transferred to tas_oper, when
   admin_base_time is passed.

   When disabling tas, the configuration change has immediate effect
   and will be applied to both tas_state and tas_oper immediately.
 */
typedef struct {
    CapArray<vtss_tsn_tas_config_t,       MEBA_CAP_BOARD_PORT_MAP_COUNT> tas_conf;
    CapArray<vtss_tsn_tas_oper_t,         MEBA_CAP_BOARD_PORT_MAP_COUNT> tas_oper;
    CapArray<vtss_appl_tsn_tas_state_t,   MEBA_CAP_BOARD_PORT_MAP_COUNT> tas_state;
} tsn_tas_t;

/* Global TSN_TAS structure */
static tsn_tas_t tsn_tas;

/******************************************************************************/
// TSN_tas_data_init() Initialize internal data structure
/******************************************************************************/
void TSN_tas_data_init(void)
{
    vtss_clear(tsn_tas);
    vtss_appl_tsn_tas_cfg_t def_cfg;
    vtss_appl_tsn_tas_gcl_t def_gce;
    vtss_appl_tsn_tas_gcl_admin_get_default(&def_gce);
    vtss_appl_tsn_tas_cfg_get_default(&def_cfg);
    // oper init
    for (int i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        tsn_tas.tas_oper[i].status.config_change_error         = 0;
        tsn_tas.tas_oper[i].status.oper_control_list_length    = def_cfg.admin_control_list_length;
        tsn_tas.tas_oper[i].status.oper_cycle_time_numerator   = def_cfg.admin_cycle_time_numerator;
        tsn_tas.tas_oper[i].status.oper_cycle_time_denominator = def_cfg.admin_cycle_time_denominator;
        tsn_tas.tas_oper[i].status.oper_cycle_time_extension   = def_cfg.admin_cycle_time_extension;
        tsn_tas.tas_oper[i].status.oper_base_time              = def_cfg.admin_base_time;
        for (int gce = 0; gce < fast_cap(MESA_CAP_QOS_TAS_GCE_CNT); gce++) {
            tsn_tas.tas_oper[i].gcl[gce] = def_gce;
        }
    }
    // state init
    for (int i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        tsn_tas.tas_state[i].config_change_performed        = false;
        tsn_tas.tas_state[i].cfg                            = def_cfg;
        for (int gce = 0; gce < fast_cap(MESA_CAP_QOS_TAS_GCE_CNT); gce++) {
            tsn_tas.tas_state[i].gcl[gce]                   = def_gce;
        }
    }
    // config init
    for (int i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        tsn_tas.tas_conf[i].cfg                 = def_cfg;
        for (int sdu = 0; sdu < VTSS_QUEUE_ARRAY_SIZE; sdu++) {
            tsn_tas.tas_conf[i].max_sdu[sdu]    = MAX_SDU_DEFAULT;
        }
        for (int gce = 0; gce < fast_cap(MESA_CAP_QOS_TAS_GCE_CNT); gce++) {
            tsn_tas.tas_conf[i].gcl[gce]        = def_gce;
        }
    }
}

/******************************************************************************/
// Static helper function
/******************************************************************************/

// while stopping a gcl is ongoing, configuration cannot be changed. Keep trying
// for one second to see if it succeeds
mesa_rc tsn_tas_port_gcl_conf_set(const mesa_inst_t          inst,
                                  const mesa_port_no_t       port_no,
                                  const uint32_t             gce_cnt,
                                  const mesa_qos_tas_gce_t   *const gcl)
{
    uint64_t start = vtss::uptime_milliseconds();
    mesa_rc  rc;

    while ((rc = mesa_qos_tas_port_gcl_conf_set(inst,
                                                port_no,
                                                gce_cnt,
                                                gcl)) != MESA_RC_OK &&
           vtss::uptime_milliseconds() < start + 2000) {
        VTSS_OS_MSLEEP(100);
    }
    return rc;
}

mesa_rc tsn_tas_port_conf_set(const mesa_inst_t        inst,
                              const mesa_port_no_t     port_no,
                              mesa_qos_tas_port_conf_t *mesa_conf)
{
    uint64_t start = vtss::uptime_milliseconds();
    mesa_rc  rc;
    while ((rc = mesa_qos_tas_port_conf_set(inst,
                                            port_no,
                                            mesa_conf)) != MESA_RC_OK &&
           vtss::uptime_milliseconds() < start + 2000) {
        VTSS_OS_MSLEEP(100);
        mesa_conf->base_time = tsn_util_calculate_chip_base_time(mesa_conf->base_time, mesa_conf->cycle_time);
    }
    return rc;

}
/******************************************************************************/
// TSN_tas_default()
/******************************************************************************/
void TSN_tas_default(void)
{
    vtss_appl_tsn_tas_cfg_t def_conf = {};
    vtss_appl_tsn_tas_cfg_global_t global_cfg = {};
    vtss_ifindex_t ifindex;
    mesa_qos_tas_gce_t dummy;
    vtss_appl_tsn_tas_max_sdu_t max_sdu = {MAX_SDU_DEFAULT};
    global_cfg.always_guard_band = true;
    vtss_appl_tsn_tas_gcl_t def_gce;
    vtss_appl_tsn_tas_gcl_admin_get_default(&def_gce);
    vtss_appl_tsn_tas_cfg_get_default(&def_conf);
    // set config_change = true because default configuration must be committed in chip.
    def_conf.config_change = true;

    vtss_appl_tsn_tas_cfg_set_global(&global_cfg);
    for (int port = 0; port < TSN_cap_port_cnt; port++ ) {
        vtss_ifindex_from_port(VTSS_ISID_START, port, &ifindex);
        for (int queue = 0; queue < 8; queue++) {
            vtss_appl_tsn_tas_per_q_max_sdu_set(ifindex, queue, &max_sdu);
        }
        for (int gce = 0; gce < fast_cap(MESA_CAP_QOS_TAS_GCE_CNT); gce++) {
            vtss_appl_tsn_tas_gcl_admin_set(ifindex, gce, &def_gce);
        }
        tsn_tas_port_gcl_conf_set(NULL, port, 0, &dummy);
        vtss_appl_tsn_tas_cfg_set(ifindex, &def_conf);
    }
}

/******************************************************************************/
// TSN_tas_config_change()()
/******************************************************************************/
mesa_rc TSN_tas_config_change()
{
    mesa_rc rc;
    vtss_appl_tsn_tas_cfg_t cfg;
    vtss_ifindex_t prev_ifindex, next_ifindex;
    prev_ifindex = VTSS_IFINDEX_NONE;
    while (VTSS_RC_OK == vtss_appl_tsn_tas_cfg_itr(&prev_ifindex, &next_ifindex) ) {
        prev_ifindex = next_ifindex;
        rc = vtss_appl_tsn_tas_cfg_get(next_ifindex, &cfg);
        T_IG(TSN_TRACE_GRP_TIMER, "loop vtss_appl_tsn_tas_cfg_get rc: %u", rc);
        if (cfg.gate_enabled) {
            cfg.config_change = true;
            rc = vtss_appl_tsn_tas_cfg_set(next_ifindex, &cfg);
            T_IG(TSN_TRACE_GRP_TIMER, "called vtss_appl_tsn_tas_cfg_set rc:%u", rc);
        }
    }
    return rc;
}

//******************************************************************************
// TSN_byte_to_array()
//******************************************************************************
static void TSN_byte_to_array( uint8_t byte, mesa_bool_t a[MESA_QUEUE_ARRAY_SIZE] )
{
    for (int i = 0; i < MESA_QUEUE_ARRAY_SIZE; i++) {
        a[i] = ((1 << i) & byte) ? true : false;
    }
}

//******************************************************************************
// TSN_array_to_byte()
//******************************************************************************
static uint8_t TSN_array_to_byte( mesa_bool_t a[MESA_QUEUE_ARRAY_SIZE] )
{
    uint8_t res = 0;
    for (int i = 0; i < MESA_QUEUE_ARRAY_SIZE; i++) {
        res |=  a[i] ? (1 << i) : 0;
    }
    return res;
}

//******************************************************************************
// vtss_appl_tsn_tas_cfg_get_global()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_cfg_get_global(
    vtss_appl_tsn_tas_cfg_global_t *const cfg
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter");
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(cfg));

    mesa_qos_tas_conf_t mesa_conf;
    VTSS_RC(mesa_qos_tas_conf_get(NULL, &mesa_conf));
    cfg->always_guard_band = mesa_conf.always_guard_band;
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_cfg_set_global()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_cfg_set_global(
    const vtss_appl_tsn_tas_cfg_global_t *const cfg
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter");
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(cfg));

    mesa_qos_tas_conf_t mesa_conf;
    mesa_conf.always_guard_band = cfg->always_guard_band;
    VTSS_RC(mesa_qos_tas_conf_set(NULL, &mesa_conf));
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_per_q_max_sdu_get()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_per_q_max_sdu_get(
    vtss_ifindex_t ifindex,
    mesa_prio_t    queue,
    vtss_appl_tsn_tas_max_sdu_t *const max_sdu
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter ifindex %d queue %d", ifindex, queue);
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(max_sdu));

    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }

    if (queue >= VTSS_QUEUE_END) {
        return VTSS_APPL_TAS_INVALID_QUEUE;
    }

    TSN_LOCK_SCOPE();
    max_sdu->max_sdu = tsn_tas.tas_conf[ife.ordinal].max_sdu[queue];
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_per_q_max_sdu_set()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_per_q_max_sdu_set(
    vtss_ifindex_t ifindex,
    mesa_prio_t    queue,
    const vtss_appl_tsn_tas_max_sdu_t *const max_sdu
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter ifindex %d queue %d", ifindex, queue);
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(max_sdu));

    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }

    if (queue >= VTSS_QUEUE_END) {
        return VTSS_APPL_TAS_INVALID_QUEUE;
    }

    if (max_sdu->max_sdu > TSN_cap.tas_max_sdu_max || ( max_sdu->max_sdu < TSN_cap.tas_max_sdu_min && max_sdu->max_sdu != 0) ) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "SDU MAX  %u is out of range %u - %u ", max_sdu->max_sdu, TSN_cap.tas_max_sdu_min, TSN_cap.tas_max_sdu_max);
        return VTSS_APPL_TAS_MAX_SDU_OUT_OF_RANGE;
    }

    TSN_LOCK_SCOPE();
    tsn_tas.tas_conf[ife.ordinal].max_sdu[queue] = max_sdu->max_sdu;
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_per_q_max_sdu_itr()
//******************************************************************************
static mesa_rc tsn_tas_q_itr(
    const mesa_prio_t *const prev_q,
    mesa_prio_t *const next_q
)
{
    vtss::expose::snmp::IteratorComposeRange<mesa_prio_t> itr(VTSS_QUEUE_START, VTSS_QUEUE_END - 1);
    return itr(prev_q, next_q);
}

mesa_rc vtss_appl_tsn_tas_per_q_max_sdu_itr(
    const vtss_ifindex_t *const prev_ifindex,   vtss_ifindex_t  *const  next_ifindex,
    const mesa_prio_t    *const prev_queue,     mesa_prio_t     *const  next_queue
)
{
    if (TSN_cap.has_tas) {
        vtss::IteratorComposeN<vtss_ifindex_t, mesa_prio_t> itr(vtss_appl_iterator_ifindex_front_port, tsn_tas_q_itr);
        return itr(prev_ifindex, next_ifindex, prev_queue, next_queue);
    }
    return VTSS_APPL_TAS_FEATURE;
}

//******************************************************************************
// vtss_appl_tsn_tas_gcl_oper_get()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_gcl_oper_get(
    vtss_ifindex_t   ifindex,
    vtss_gcl_index_t gcl_index,
    vtss_appl_tsn_tas_gcl_t *const status
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter ifindex %d gcl_index %d", ifindex, gcl_index);
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(status));

    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }
    if (gcl_index >= TSN_cap.tas_max_gce_cnt) {
        return VTSS_APPL_TAS_INVALID_GCE;
    }

    *status = tsn_tas.tas_oper[ife.ordinal].gcl[gcl_index];
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_gcl_admin_get()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_gcl_admin_get(
    vtss_ifindex_t   ifindex,
    vtss_gcl_index_t gcl_index,
    vtss_appl_tsn_tas_gcl_t *const cfg
)
{
    mesa_qos_tas_gce_t  gcl[TSN_cap.tas_max_gce_cnt];
    uint32_t            gce_cnt;
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter ifindex %d gcl_index %d", ifindex, gcl_index);
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(cfg));

    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }

    if (gcl_index >= TSN_cap.tas_max_gce_cnt) {
        return VTSS_APPL_TAS_INVALID_GCE;
    }

    TSN_LOCK_SCOPE();
    if (mesa_qos_tas_port_gcl_conf_get(NULL,
                                       ife.ordinal,
                                       TSN_cap.tas_max_gce_cnt,
                                       gcl,
                                       &gce_cnt) != MESA_RC_OK) {
        return VTSS_RC_ERROR;
    }

    *cfg = tsn_tas.tas_conf[ife.ordinal].gcl[gcl_index];
    cfg->gate_operation = gcl[gcl_index].gate_operation;
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_gcl_admin_set()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_gcl_admin_set(
    vtss_ifindex_t   ifindex,
    vtss_gcl_index_t gcl_index,
    const vtss_appl_tsn_tas_gcl_t *const cfg
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter ifindex %d gcl_index %d", ifindex, gcl_index);
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(cfg));

    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }

    if (gcl_index >= TSN_cap.tas_max_gce_cnt) {
        return VTSS_APPL_TAS_INVALID_GCE;
    }

    TSN_LOCK_SCOPE();
    tsn_tas.tas_conf[ife.ordinal].gcl[gcl_index] = *cfg;
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_gcl_admin_itr()
//******************************************************************************
static mesa_rc tsn_tas_gcl_index_admin_itr(
    const vtss_gcl_index_t *const prev_gcl,
    vtss_gcl_index_t       *const next_gcl,
    vtss_ifindex_t                 ifindex
)
{
    vtss_ifindex_elm_t ife = {};
    int range_end;
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }
    range_end = tsn_tas.tas_conf[ife.ordinal].cfg.admin_control_list_length - 1;
    if (range_end < 0) {
        return VTSS_RC_ERROR;
    }
    vtss::expose::snmp::IteratorComposeRange<vtss_gcl_index_t> itr(0, range_end) ;
    return itr(prev_gcl, next_gcl);
}

mesa_rc vtss_appl_tsn_tas_gcl_admin_itr(
    const vtss_ifindex_t    *const prev_ifindex,    vtss_ifindex_t   *const next_ifindex,
    const vtss_gcl_index_t  *const prev_gcl,        vtss_gcl_index_t *const next_gcl
)
{
    VTSS_RC(TSN_ptr_check(next_ifindex));
    VTSS_RC(TSN_ptr_check(next_gcl));
    if (TSN_cap.has_tas) {
        TSN_LOCK_SCOPE();
        vtss::IteratorComposeDependN<vtss_ifindex_t, vtss_gcl_index_t> itr(vtss_appl_iterator_ifindex_front_port, tsn_tas_gcl_index_admin_itr);
        return itr(prev_ifindex, next_ifindex, prev_gcl, next_gcl);
    }
    return VTSS_APPL_TAS_FEATURE;
}

//******************************************************************************
// vtss_appl_tsn_tas_gcl_admin_get_default()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_gcl_admin_get_default(
    vtss_appl_tsn_tas_gcl_t *const cfg
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter");
    VTSS_RC(TSN_ptr_check(cfg));
    cfg->gate_state      = 0xff; // All gates open
    // According to 802.1Qbv section 8.6.9.2.1 ExecuteOperation, minimum time_interval is 1
    cfg->time_interval   = 1;
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_gcl_oper_itr()
//******************************************************************************
static mesa_rc tsn_tas_gcl_index_oper_itr(
    const vtss_gcl_index_t *const prev_gcl,
    vtss_gcl_index_t       *const next_gcl,
    vtss_ifindex_t                 ifindex
)
{
    vtss_ifindex_elm_t ife = {};
    int range_end;
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }
    range_end = tsn_tas.tas_oper[ife.ordinal].status.oper_control_list_length - 1;
    if (range_end < 0) {
        return VTSS_RC_ERROR;
    }
    vtss::expose::snmp::IteratorComposeRange<vtss_gcl_index_t> itr(0, range_end);
    return itr(prev_gcl, next_gcl);
}

mesa_rc vtss_appl_tsn_tas_gcl_oper_itr(
    const vtss_ifindex_t    *const prev_ifindex,    vtss_ifindex_t   *const next_ifindex,
    const vtss_gcl_index_t  *const prev_gcl,        vtss_gcl_index_t *const next_gcl
)
{
    VTSS_RC(TSN_ptr_check(next_ifindex));
    VTSS_RC(TSN_ptr_check(next_gcl));
    if (TSN_cap.has_tas) {
        TSN_LOCK_SCOPE();
        vtss::IteratorComposeDependN<vtss_ifindex_t, vtss_gcl_index_t> itr(vtss_appl_iterator_ifindex_front_port, tsn_tas_gcl_index_oper_itr);
        return itr(prev_ifindex, next_ifindex, prev_gcl, next_gcl);
    }
    return VTSS_APPL_TAS_FEATURE;
}

//******************************************************************************
// vtss_appl_tsn_tas_cfg_get()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_cfg_get(
    vtss_ifindex_t ifindex,
    vtss_appl_tsn_tas_cfg_t *const cfg
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter ifindex %d", ifindex);
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(cfg));

    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }

    TSN_LOCK_SCOPE();
    *cfg = tsn_tas.tas_conf[ife.ordinal].cfg;
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_cfg_set()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_cfg_set(
    vtss_ifindex_t ifindex,
    const vtss_appl_tsn_tas_cfg_t *const cfg
)
{
    bool current_gate_enable;
    u32  current_cycle_time;
    mesa_port_no_t port_no;

    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter ifindex %d", ifindex);
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(cfg));

    mesa_rc rc;
    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }
    port_no = ife.ordinal;
    if (cfg->admin_control_list_length >  TSN_cap.tas_max_gce_cnt ) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "Invalid ControlListLength %u. Must be maximum %u", cfg->admin_control_list_length,  TSN_cap.tas_max_gce_cnt);
        return VTSS_APPL_TAS_ADMIN_CTRL_LIST_TOO_LONG;
    }
    if (cfg->admin_base_time.sec_msb) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "Invalid cfg->admin_base_time.sec_msb");
        return VTSS_APPL_TAS_INVALID_ADMIN_BASE_TIME_SEC_MSB;
    }
    if (cfg->admin_base_time.nanoseconds > 999999999) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "Invalid cfg->admin_base_time.nanoseconds");
        return VTSS_APPL_TAS_ADMIN_BASE_TIME_SEC_NANO_TOO_BIG;
    }
    if (cfg->admin_cycle_time_numerator == 0) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "CycleTimeNumerator must not be zero");
        return VTSS_APPL_TAS_ADMIN_CYCLE_TIME_NUMERATOR_ZERO;
    }
    if (cfg->admin_cycle_time_denominator == 0) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "CycleTimeDenominator must not be zero");
        return VTSS_APPL_TAS_ADMIN_CYCLE_TIME_DENOMINATOR_ZERO;
    }
    if (cfg->admin_cycle_time_denominator != 1000 && cfg->admin_cycle_time_denominator != 1000000 && cfg->admin_cycle_time_denominator != 1000000000) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "CycleTimeDenominator must be either 1000, 1000000 or 1000000000.");
        return VTSS_APPL_TAS_ADMIN_CYCLE_TIME_DENOMINATOR_NOT_VALID;
    }
    // convert CycleTime to nanoseconds
    u32 cycle_time_ns = (u32)((cfg->admin_cycle_time_numerator * 1000000000ULL) / cfg->admin_cycle_time_denominator);
    if (cycle_time_ns < TSN_cap.tas_ct_min) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "Invalid CycleTime %u. Allowed range: %d <= CycleTime <= %d", cycle_time_ns, TSN_cap.tas_ct_min, TSN_cap.tas_ct_max);
        return VTSS_APPL_TAS_CYCLE_TIME_TOO_SMALL;
    }
    if (cycle_time_ns > TSN_cap.tas_ct_max) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "Invalid CycleTime %u. Allowed range: %d <= CycleTime <= %d", cycle_time_ns, TSN_cap.tas_ct_min, TSN_cap.tas_ct_max);
        return VTSS_APPL_TAS_CYCLE_TIME_TOO_BIG;
    }
    if (cfg->admin_cycle_time_extension > TSN_cap.tas_ct_max) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "CycleTimeExtension %u. Must be maximum %u", cfg->admin_cycle_time_extension, TSN_cap.tas_ct_max);
        return VTSS_APPL_TAS_CYCLE_TIME_EXTENSION_TOO_BIG;
    }
    if (cfg->admin_cycle_time_extension < TSN_cap.tas_ct_min) {
        T_IG(TSN_TRACE_GRP_CONFIG_TAS, "CycleTimeExtension %u. Must be minimum %u", cfg->admin_cycle_time_extension, TSN_cap.tas_ct_min);
        return VTSS_APPL_TAS_CYCLE_TIME_EXTENSION_TOO_SMALL;
    }


    // Fetch state of the PTP check function.
    // This is to ensure that we do not issue a config_change before the ptp time is synchronized
    vtss_appl_tsn_global_state_t global_state;
    vtss_appl_tsn_global_state_get(&global_state);
    bool time_state_ok = global_state.start;

    TSN_LOCK_SCOPE();

    if (cfg->gate_enabled && cfg->config_change && cfg->admin_control_list_length > 0) {
        u32 accumulated_time_interval = 0;
        u8 gate_state = 0;
        for (int i = 0; i < cfg->admin_control_list_length; i++) {
            accumulated_time_interval +=  tsn_tas.tas_conf[port_no].gcl[i].time_interval;
            gate_state |=  tsn_tas.tas_conf[port_no].gcl[i].gate_state;
        }
        if (gate_state != 0xff) {
            T_IG(TSN_TRACE_GRP_CONFIG_TAS, "Not all queues are open during cycle (0x%x).", gate_state);
            return VTSS_APPL_TAS_UNHANDLED_QUEUES;
        }
        if (cycle_time_ns < accumulated_time_interval) {
            T_IG(TSN_TRACE_GRP_CONFIG_TAS, "CycleTime (%u) must be equal to or greater than accumulated TimeInterval (%u).", cycle_time_ns, accumulated_time_interval);
            return VTSS_APPL_TAS_INVALID_CYCLE_TIME;
        }
    }

    // get information about frame pre-emption
    vtss_appl_tsn_fp_cfg_t fp_cfg;
    vtss_appl_tsn_fp_cfg_get(ifindex, &fp_cfg);
    u8 fp_state = 0;
    for (int q = 0; q < MESA_QUEUE_ARRAY_SIZE; ++q) {
        if (!fp_cfg.admin_status[q]) {
            fp_state |= 1 << q;
        }
    }

    // First: configure list of Gate Control Elements
    CapArray<mesa_qos_tas_gce_t, MESA_CAP_QOS_TAS_GCE_CNT> mesa_gce_conf;
    CapArray<vtss_appl_tsn_tas_gcl_t, MESA_CAP_QOS_TAS_GCE_CNT> gcl;
    uint32_t total_time_interval = 0;

    vtss_appl_tsn_tas_gcl_t def_gce;
    vtss_appl_tsn_tas_gcl_admin_get_default(&def_gce);
    int mesa_gcl_len = cfg->admin_control_list_length;
    // If a gce contains preemptable traffic, operation shall be set
    // to RELEASE_MAC to enable transmission of preemptable
    // traffic. If the gce only contains express traffic, the
    // operation shall be set to HOLD_MAC
    for (int i = 0; i < cfg->admin_control_list_length; i++) {
        if ( i >= tsn_tas.tas_conf[port_no].cfg.admin_control_list_length) {
            // if the new admin_control_list_length is longer that the
            // old, the elements in the list above
            // admin_control_list_length is unconfigured so we apply
            // the default value
            gcl[i] = def_gce;
            if (cycle_time_ns > total_time_interval) {
                gcl[i].time_interval = (cycle_time_ns - total_time_interval) / (cfg->admin_control_list_length - i);
            } else {
                gcl[i].time_interval = 0;
            }
        } else {
            gcl[i] = tsn_tas.tas_conf[port_no].gcl[i];
        }
        total_time_interval += gcl[i].time_interval;
        if (!fp_cfg.enable_tx) {
            gcl[i].gate_operation = MESA_QOS_TAS_GCO_SET_GATE_STATES;
        } else if (fp_state & ~gcl[i].gate_state) {
            gcl[i].gate_operation = MESA_QOS_TAS_GCO_SET_AND_HOLD_MAC;
        } else {
            gcl[i].gate_operation = MESA_QOS_TAS_GCO_SET_AND_RELEASE_MAC;
        }
        mesa_gce_conf[i].gate_operation = gcl[i].gate_operation;
        mesa_gce_conf[i].time_interval = gcl[i].time_interval;
        TSN_byte_to_array(gcl[i].gate_state, mesa_gce_conf[i].gate_open);
    }

    if (cfg->admin_control_list_length == 0 || !cfg->gate_enabled) {
        // Create a gce filling out the entire cycle, with gate_state
        // taken from admin_gate_states or (in case of tas being
        // disabled) all open
        if (!fp_cfg.enable_tx) {
            mesa_gce_conf[0].gate_operation = MESA_QOS_TAS_GCO_SET_GATE_STATES;
        } else if (fp_state & ~cfg->admin_gate_states) {
            mesa_gce_conf[0].gate_operation = MESA_QOS_TAS_GCO_SET_AND_HOLD_MAC;
        } else {
            mesa_gce_conf[0].gate_operation = MESA_QOS_TAS_GCO_SET_AND_RELEASE_MAC;
        }
        mesa_gce_conf[0].time_interval = cycle_time_ns;
        if (cfg->gate_enabled) {
            TSN_byte_to_array(cfg->admin_gate_states, mesa_gce_conf[0].gate_open);
        } else {
            TSN_byte_to_array(0xff, mesa_gce_conf[0].gate_open);
        }
        mesa_gcl_len = 1;
    }

    if ((rc = tsn_tas_port_gcl_conf_set(NULL, port_no, mesa_gcl_len, mesa_gce_conf.data())) !=  MESA_RC_OK ) {
        T_DG(TSN_TRACE_GRP_CONFIG_TAS, "failed calling mesa_qos_tas_port_gcl_conf_set. reason %u", rc);
        return VTSS_APPL_TAS_GCE_CONFIGURATION;
    }

    // Next: Configure TAS.
    mesa_qos_tas_port_conf_t mesa_conf;
    mesa_qos_tas_port_conf_get(NULL, port_no, &mesa_conf);
    current_gate_enable = mesa_conf.gate_enabled;
    current_cycle_time = mesa_conf.cycle_time;
    mesa_conf.gate_enabled = cfg->gate_enabled;
    for (int i = 0; i < MESA_QUEUE_ARRAY_SIZE; i++) {
        mesa_conf.max_sdu[i]    = tsn_tas.tas_conf[port_no].max_sdu[i] == 0 ? TSN_cap.tas_max_sdu_max : tsn_tas.tas_conf[port_no].max_sdu[i];
    }

    TSN_byte_to_array(cfg->admin_gate_states, mesa_conf.gate_open);
    mesa_conf.cycle_time        = tsn_util_calc_cycle_time_nsec(cfg->admin_cycle_time_numerator, cfg->admin_cycle_time_denominator);
    mesa_conf.cycle_time_ext    = cfg->admin_cycle_time_extension;
    mesa_conf.base_time         = tsn_util_calculate_chip_base_time(cfg->admin_base_time, mesa_conf.cycle_time);
    mesa_conf.config_change     = cfg->config_change || cfg->gate_enabled != current_gate_enable;

    if (!time_state_ok) {
        T_DG(TSN_TRACE_GRP_CONFIG_TAS, "Delaying config_change until timer is ready. reason %u", time_state_ok) ;
        mesa_conf.config_change = false;
    }

    T_DG(TSN_TRACE_GRP_CONFIG_TAS, "Config_change %u gate_enable %u MaxSdu %u %u %u %u %u %u %u %u. Gate open %u %u %u %u %u %u %u %u CycleTime %u ext %u basetime msb:%u sec:%u nano:%u frag:%u    ",
         mesa_conf.config_change, mesa_conf.gate_enabled,
         mesa_conf.max_sdu[0],   mesa_conf.max_sdu[1],   mesa_conf.max_sdu[2],   mesa_conf.max_sdu[3],   mesa_conf.max_sdu[4],   mesa_conf.max_sdu[5],   mesa_conf.max_sdu[6],   mesa_conf.max_sdu[7],
         mesa_conf.gate_open[0], mesa_conf.gate_open[1], mesa_conf.gate_open[2], mesa_conf.gate_open[3], mesa_conf.gate_open[4], mesa_conf.gate_open[5], mesa_conf.gate_open[6], mesa_conf.gate_open[7],
         mesa_conf.cycle_time, mesa_conf.cycle_time_ext,
         mesa_conf.base_time.sec_msb, mesa_conf.base_time.seconds, mesa_conf.base_time.nanoseconds, mesa_conf.base_time.nanosecondsfrac);

    // Fetch status to save config_change_error counter. If configuration fails, we may be able to figure out why.
    mesa_qos_tas_port_status_t mesa_status;
    mesa_qos_tas_port_status_get(NULL, port_no, &mesa_status);
    if ((rc = tsn_tas_port_conf_set(NULL, port_no, &mesa_conf)) != MESA_RC_OK ) {
        T_DG(TSN_TRACE_GRP_CONFIG_TAS, "failed calling tsn_tas_port_conf_set. reason %u", rc);
        uint64_t old_config_change_error = mesa_status.config_change_error;
        mesa_qos_tas_port_status_get(NULL, port_no, &mesa_status);
        if (old_config_change_error < mesa_status.config_change_error) {
            return VTSS_APPL_TAS_TAS_CONFIGURATION_BASETIME_NOT_FUTURE;
        } else {
            return VTSS_APPL_TAS_TAS_CONFIGURATION;
        }
    }


    mesa_timestamp_t tod;
    uint64_t         tc;
    (void)mesa_ts_timeofday_get(NULL, &tod, &tc);

    if (mesa_conf.gate_enabled &&
        current_gate_enable &&
        mesa_conf.config_change &&
        cfg->admin_base_time < tod) {
        // See section 8.6.9.3.1 SetConfigChangeTime() in 802.1Qbv-2015 for calculation
        // of ConfigChangeError
        tsn_tas.tas_oper[port_no].status.config_change_error += 1;
    }
    if (mesa_conf.config_change || (current_gate_enable && !cfg->gate_enabled)) {
        VTSS_OS_MSLEEP(current_cycle_time / 1000000); // Wait for current cycle to stop before changing configuration
    }

    // Save current configuration
    tsn_tas.tas_conf[port_no].cfg = *cfg;
    tsn_tas.tas_conf[port_no].gcl = gcl;

    // The purpose of this section is to update the oper_state variables.
    if (mesa_conf.config_change && mesa_conf.gate_enabled) { // We use mesa_conf.config_change because it may have been disabled earlier due to time_state_ok check
        // tsn_tas.tas_state values will be copied to tsn_tas.tas_oper by TSN_TAS_operstate_timeout when conditions are right.
        tsn_tas.tas_state[port_no].cfg = *cfg;
        for (int gce = 0; gce < tsn_tas.tas_conf[port_no].cfg.admin_control_list_length; gce++) {
            tsn_tas.tas_state[port_no].gcl[gce] = tsn_tas.tas_conf[port_no].gcl[gce];
        }
        tsn_tas.tas_state[port_no].config_change_performed = true;
    } else if (current_gate_enable && !mesa_conf.gate_enabled ) {
        // Going from gate enable to gate disable.
        // This will stop processing of all gcls at once and we do not have to wait for
        // config_pending and/or config_change_time so the status can be updated immediately.
        vtss_appl_tsn_tas_cfg_t def_cfg;
        vtss_appl_tsn_tas_cfg_get_default(&def_cfg);
        tsn_tas.tas_oper[port_no].status.config_change_error             = 0;
        tsn_tas.tas_oper[port_no].status.oper_control_list_length        = def_cfg.admin_control_list_length;
        tsn_tas.tas_oper[port_no].status.oper_cycle_time_numerator       = def_cfg.admin_cycle_time_numerator;
        tsn_tas.tas_oper[port_no].status.oper_cycle_time_denominator     = def_cfg.admin_cycle_time_denominator;
        tsn_tas.tas_oper[port_no].status.oper_cycle_time_extension       = def_cfg.admin_cycle_time_extension;
        tsn_tas.tas_oper[port_no].status.oper_base_time                  = def_cfg.admin_base_time;
        for (int gce = 0; gce < fast_cap(MESA_CAP_QOS_TAS_GCE_CNT); gce++) {
            tsn_tas.tas_oper[port_no].gcl[gce]                  = def_gce;
        }
    }

    tsn_tas.tas_conf[port_no].cfg.config_change = false; // config_change is a one_shot

    return VTSS_RC_OK;
}

/******************************************************************************/
// TSN_TAS_operstate_timeout()
// Timer = operstate_timer
/******************************************************************************/
static void TSN_TAS_operstate_timeout(tsn_timer_t &timer, void *context)
{
    mesa_qos_tas_port_status_t mesa_status;
    mesa_timestamp_t tod;
    uint64_t         tc;
    TSN_LOCK_SCOPE();

    for (int port = 0; port < TSN_cap_port_cnt; port++ ) {
        if (tsn_tas.tas_state[port].cfg.gate_enabled) {
            T_DG(TSN_TRACE_GRP_CONFIG_TAS, "port %u gate_enabled", port);
            if (tsn_tas.tas_state[port].config_change_performed) {
                T_DG(TSN_TRACE_GRP_CONFIG_TAS, "port %u config_change_performed", port);
                mesa_qos_tas_port_status_get(NULL, port, &mesa_status);
                (void)mesa_ts_timeofday_get(NULL, &tod, &tc);
                if (mesa_status.config_change_time < tod ) {
                    T_DG(TSN_TRACE_GRP_CONFIG_TAS, "port %u config_change_time", port);
                    if (!mesa_status.config_pending) {
                        T_DG(TSN_TRACE_GRP_CONFIG_TAS, "port %u config_pending", port);
                        tsn_tas.tas_oper[port].status.oper_control_list_length       = tsn_tas.tas_state[port].cfg.admin_control_list_length;
                        tsn_tas.tas_oper[port].status.oper_cycle_time_numerator      = tsn_tas.tas_state[port].cfg.admin_cycle_time_numerator;
                        tsn_tas.tas_oper[port].status.oper_cycle_time_denominator    = tsn_tas.tas_state[port].cfg.admin_cycle_time_denominator;
                        tsn_tas.tas_oper[port].status.oper_cycle_time_extension      = tsn_tas.tas_state[port].cfg.admin_cycle_time_extension;
                        tsn_tas.tas_oper[port].status.oper_base_time                 = tsn_tas.tas_state[port].cfg.admin_base_time;
                        for (int gce = 0; gce < tsn_tas.tas_oper[port].status.oper_control_list_length; gce++) {
                            tsn_tas.tas_oper[port].gcl[gce]  = tsn_tas.tas_state[port].gcl[gce];
                        }
                        tsn_tas.tas_state[port].config_change_performed              = false;
                    }
                }
            }
        }
    }
}

//******************************************************************************
// vtss_appl_tsn_tas_cfg_itr()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_cfg_itr(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex
)
{
    if (TSN_cap.has_tas) {
        return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
    }
    return VTSS_APPL_TAS_FEATURE;
}

//******************************************************************************
// vtss_appl_tsn_tas_cfg_get_default()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_cfg_get_default(
    vtss_appl_tsn_tas_cfg_t *const cfg
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter");
    VTSS_RC(TSN_ptr_check(cfg));

    cfg->gate_enabled                       = false;
    cfg->admin_gate_states                  = 0xff; // Set all to open
    cfg->admin_control_list_length          = 0;    // No active GCEs in Gate Control List
    cfg->admin_cycle_time_numerator         = 100;  // Default cycle time is 100 msec (100/1000 sec)
    cfg->admin_cycle_time_denominator       = 1000; // Default cycle time is 100 msec (100/1000 sec)
    cfg->admin_cycle_time_extension         = TSN_cap.tas_ct_min;
    cfg->admin_base_time.sec_msb            = 0;    // Seconds msb
    cfg->admin_base_time.seconds            = 0;    // Seconds
    cfg->admin_base_time.nanoseconds        = 0;    // nanoseconds
    cfg->admin_base_time.nanosecondsfrac    = 0;    // 16 bit fraction of nano seconds
    cfg->config_change                      = false;

    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_status_get()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_status_get(
    vtss_ifindex_t ifindex,
    vtss_appl_tsn_tas_oper_state_t *const status
)
{
    T_NG(TSN_TRACE_GRP_CONFIG_TAS, "Enter ifindex %d", ifindex);
    if (!TSN_cap.has_tas) {
        return VTSS_APPL_TAS_FEATURE;
    }
    VTSS_RC(TSN_ptr_check(status));

    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_TAS_INVALID_IFINDEX;
    }

    mesa_qos_tas_port_status_t mesa_status;
    mesa_timestamp_t ts;
    uint64_t tc;
    VTSS_RC(mesa_qos_tas_port_status_get(NULL, ife.ordinal, &mesa_status));
    mesa_ts_timeofday_get(NULL, &ts, &tc);


    status->oper_gate_states        = TSN_array_to_byte(mesa_status.gate_open);
    status->config_change_time      = mesa_status.config_change_time;
    status->config_pending          = mesa_status.config_pending;
    status->current_time            = ts;
    status->supported_list_max      = TSN_cap.tas_max_gce_cnt;
    status->tick_granularity        = 1;

    TSN_LOCK_SCOPE();

    status->config_change_error             = tsn_tas.tas_oper[ife.ordinal].status.config_change_error;
    status->oper_control_list_length        = tsn_tas.tas_oper[ife.ordinal].status.oper_control_list_length;
    status->oper_cycle_time_numerator       = tsn_tas.tas_oper[ife.ordinal].status.oper_cycle_time_numerator;
    status->oper_cycle_time_denominator     = tsn_tas.tas_oper[ife.ordinal].status.oper_cycle_time_denominator;
    status->oper_cycle_time_extension       = tsn_tas.tas_oper[ife.ordinal].status.oper_cycle_time_extension;
    status->oper_base_time                  = tsn_tas.tas_oper[ife.ordinal].status.oper_base_time;

    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_tas_status_itr()
//******************************************************************************
mesa_rc vtss_appl_tsn_tas_status_itr(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex
)
{
    if (TSN_cap.has_tas) {
        return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
    }
    return VTSS_APPL_TAS_FEATURE;
}

void TSN_tas_init()
{
    tsn_timer_init(TSN_TAS_poll_timer, "operstate timer", -1, TSN_TAS_operstate_timeout, &tsn_tas.tas_state);
    tsn_timer_start(TSN_TAS_poll_timer, 1000, true /* true = repeat */);
}
