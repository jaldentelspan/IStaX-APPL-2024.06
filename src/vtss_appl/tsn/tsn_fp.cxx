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

#include <vtss/appl/tsn.h>          /* For ourselves (public header)             */
#include "tsn_api.h"                /* For ourselves (semi-public header)        */
#include "tsn_trace.h"              /* For our own trace definitions             */
#include "tsn_expose.hxx"           /* For our own notifications                 */
#include "critd_api.h"              /* For mutex wrapper                         */
#include "tsn_timer.hxx"
#include "tsn_lock.hxx"             /* For TSN_LOCK_SCOPE()                      */
#include "misc_api.h"               /* For misc_mac_txt()                        */
#include "vlan_api.h"               /* For vlan_mgmt_XXX()                       */
#include "conf_api.h"               /* For conf_mgmt_mac_addr_get()              */
#include "l2proto_api.h"            /* For l2port2port()                         */
#include "interrupt_api.h"          /* For vtss_interrupt_source_hook_set()      */
#include "web_api.h"                /* For webCommonBufferHandler()              */
#include <vtss/basics/trace.hxx>    /* For VTSS_TRACE()                          */
#include "vtss_common_iterator.hxx" /* For vtss_appl_iterator_ifindex_front_port */
#include <unistd.h>                 /* for usleep                                */
#include "port_api.h"               /* For port_cap_get()                        */
#include <vtss/appl/qos.h>          /* For vtss_appl_qos_scheduler_get           */

#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif /* VTSS_SW_OPTION_ICFG */

#if defined(VTSS_SW_OPTION_LLDP)
#include "lldp_api.h"
#endif /* VTSS_SW_OPTION_LLDP */


/******************************************************************************/
// Global variables
/******************************************************************************/

extern uint32_t                               TSN_cap_port_cnt;
extern vtss_appl_tsn_capabilities_t           TSN_cap;

#define VERIFY_TIMER_MAX_RETRY                2
#define VERIFY_TIMER_TIMEOUT_VALUE_MS         1000

/* TSN frame preemption LLDP remote info */
typedef struct {
    bool supported;
    bool enabled;
    bool active;
    u8   add_frag_size;
    u8   debug_add_frag_size; //  can be set by cli debug command
} tsn_preempt_info_t;

/* TSN frame preemption state */
typedef struct {
    tsn_timer_t verify_timer;
    int timer_count;
    mesa_port_no_t port;
    bool link;
    mesa_port_speed_t speed;
} tsn_fp_state_t;

/* TSN global structure */
typedef struct {
    CapArray<vtss_appl_tsn_fp_cfg_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> tsn_fp_conf;
    CapArray<tsn_fp_state_t,         MEBA_CAP_BOARD_PORT_MAP_COUNT> tsn_fp_state;
    CapArray<tsn_preempt_info_t,     MEBA_CAP_BOARD_PORT_MAP_COUNT> preempt_info;
} tsn_global_t;

/* Global structure */
static tsn_global_t tsn;

//******************************************************************************
// vtss_tsn_fp_get_preempt_info_default()
//******************************************************************************
static mesa_rc vtss_tsn_fp_get_preempt_info_default(tsn_preempt_info_t *const preempt_info )
{
    VTSS_RC(TSN_ptr_check(preempt_info));

    preempt_info->supported = false;
    preempt_info->enabled = false;
    preempt_info->active = false;
    preempt_info->add_frag_size = 0;
    preempt_info->debug_add_frag_size = 0;

    return VTSS_RC_OK;
}

mesa_rc tsn_util_debug_set_add_frag_size(vtss_ifindex_t ifindex, int add_frag_size)
{
    vtss_ifindex_elm_t ife = {};
    T_D("Enter");
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_FP_INVALID_IFINDEX;
    }
    if (add_frag_size < 0 || add_frag_size > 3 ) {
        T_I("Debug frag size out of range");
        return VTSS_RC_ERROR;
    }
    TSN_LOCK_SCOPE();

    tsn.preempt_info[ife.ordinal].debug_add_frag_size = add_frag_size;

    return VTSS_RC_OK;

}

/******************************************************************************/
// TSN_fp_supported()
/******************************************************************************/
static mesa_rc TSN_fp_supported(mesa_port_no_t port)
{
    meba_port_cap_t cap;
    port_cap_get(port, &cap);

    if ((cap & MEBA_PORT_CAP_10G_FDX) && (cap & MEBA_PORT_CAP_COPPER)) {
        T_D("Preemption not supported on 10G copper port (Aquantia), capability %d ", cap );
        return VTSS_APPL_FP_10G_COPPER_PORT_NOT_SUPPORTED;
    }
    return VTSS_RC_OK;
}

/******************************************************************************/
// TSN_appl_to_mesa_fp_conf()
/******************************************************************************/
static void TSN_appl_to_mesa_fp_conf(vtss_appl_tsn_fp_cfg_t *appl, mesa_qos_fp_port_conf_t *mesa)
{
    for (int prio = 0; prio < 8; prio++) {
        mesa->admin_status[prio] = appl->admin_status[prio];
    }
    mesa->enable_tx         = appl->enable_tx;
    mesa->verify_disable_tx = appl->verify_disable_tx;
    mesa->verify_time       = appl->verify_time;
    mesa->add_frag_size     = appl->add_frag_size;
}


/******************************************************************************/
// TSN_fp_is_config_different()
/******************************************************************************/
static bool TSN_fp_is_config_different(mesa_qos_fp_port_conf_t *old_conf, mesa_qos_fp_port_conf_t *new_conf)
{
    bool diff = false;
    for (int prio = 0; prio < 8; prio++) {
        if ( old_conf->admin_status[prio] != new_conf->admin_status[prio]) {
            diff = true;
            T_DG(TSN_TRACE_GRP_CONFIG_FP, "admin_status[%u] old:%u  new:%u", prio, old_conf->admin_status[prio], new_conf->admin_status[prio]);
            return diff;
        }
    }
    if (old_conf->enable_tx != new_conf->enable_tx) {
        diff = true;
        T_DG(TSN_TRACE_GRP_CONFIG_FP, "enable_tx old:%u new:%u", old_conf->enable_tx, new_conf->enable_tx);
        return diff;
    }
    if (old_conf->verify_disable_tx != new_conf->verify_disable_tx) {
        diff = true;
        T_DG(TSN_TRACE_GRP_CONFIG_FP, "verify_disable_tx old:%u new:%u", old_conf->verify_disable_tx, new_conf->verify_disable_tx);
        return diff;
    }
    if (old_conf->verify_time != new_conf->verify_time) {
        diff = true;
        T_DG(TSN_TRACE_GRP_CONFIG_FP, "verify_time old:%u new:%u", old_conf->verify_time, new_conf->verify_time);
        return diff;
    }
    if (old_conf->add_frag_size != new_conf->add_frag_size) {
        diff = true;
        T_DG(TSN_TRACE_GRP_CONFIG_FP, "add_frag_size old:%u new:%u", old_conf->add_frag_size, new_conf->add_frag_size);
        return diff;
    }
    return diff;
}

//*****************************************************************************/
// TSN_fp_status_verify_to_str()
/******************************************************************************/
const char *TSN_fp_status_verify_to_str(mesa_mm_status_verify_t status_verify)
{
    switch (status_verify) {
    case MESA_MM_STATUS_VERIFY_INITIAL:
        return "initial";

    case MESA_MM_STATUS_VERIFY_IDLE:
        return "idle";

    case MESA_MM_STATUS_VERIFY_SEND:
        return "send";

    case MESA_MM_STATUS_VERIFY_WAIT:
        return "wait";

    case MESA_MM_STATUS_VERIFY_SUCCEEDED:
        return "succeeded";

    case MESA_MM_STATUS_VERIFY_FAILED:
        return "failed";

    case MESA_MM_STATUS_VERIFY_DISABLED:
        return "disabled";

    default:
        T_E("Invalid status_verify (%d)", status_verify);
        return "unknown";
    }
}

/******************************************************************************/
// TSN_fp_port_state_set()
/******************************************************************************/
static mesa_rc TSN_fp_port_state_set(mesa_port_no_t port_no, bool called_by_timer)
{
    TSN_LOCK_ASSERT_LOCKED("TSN_fp_port_state_set not locked");
    mesa_rc rc = VTSS_RC_OK;

    if (!TSN_is_started()) {
        // Defer changes until properly initialised after boot.
        T_DG(TSN_TRACE_GRP_CONFIG_FP, "defer configuration port %u timer %u\n", port_no, called_by_timer);
        return rc;
    }

    mesa_qos_fp_port_conf_t api_conf_new = {};
    mesa_qos_fp_port_conf_t api_conf_old = {};
    mesa_qos_fp_port_status_t st = {};
    vtss_appl_tsn_fp_cfg_t *appl_conf = &tsn.tsn_fp_conf[port_no];
    tsn_preempt_info_t *preempt_info = &tsn.preempt_info[port_no];
    tsn_fp_state_t *state = &tsn.tsn_fp_state[port_no];
    bool need_verify_refresh = false;
    bool config_changed = false;

    TSN_appl_to_mesa_fp_conf(appl_conf, &api_conf_new); // api_conf_new is now a copy of the configured values in API format

    T_DG(TSN_TRACE_GRP_CONFIG_FP, "port_no = %u: Stored LLDP info: supported: %u enable: %u  active: %u  frag_size: %u deb_frag_size: %u  link  %u, ", port_no,
         preempt_info->supported, preempt_info->enabled,  preempt_info->active, preempt_info->add_frag_size, preempt_info->debug_add_frag_size, state->link ); //  can be set by cli debug command

    if (appl_conf->ignore_lldp_tx) {
        T_IG(TSN_TRACE_GRP_CONFIG_FP, "port_no : %u Not using lldp conveyed information but configured values ", port_no);
    } else {
        if (appl_conf->enable_tx &&         // If I am configured to send frames in preemption mode
            preempt_info->supported &&      // If remote link partner supports preemption
            preempt_info->enabled ) {       // If remote link partner has preemption enabled
            api_conf_new.add_frag_size = preempt_info->add_frag_size;
            api_conf_new.enable_tx = true;
        } else {
            api_conf_new.enable_tx = false;
        };
    }

    // Check result of verify function, and if required start the retry timer: tsn_timer_start()
    if (!called_by_timer) {
        state->timer_count = VERIFY_TIMER_MAX_RETRY;
    }

    if (api_conf_new.enable_tx &&           // No need to check verify status if frame preemption is not enabled
        !api_conf_new.verify_disable_tx &&  // If verify is disabled, no need to check status
        state->link ) {                     // If there is no link, we can not perform verify
        VTSS_RC(mesa_qos_fp_port_status_get(NULL, port_no, &st));
        if (st.status_verify != MESA_MM_STATUS_VERIFY_SUCCEEDED) {
            // verify process not successful. We try again, driven by the verify timer function callback: TSN_FP_verify_timeout()
            need_verify_refresh = true;
            tsn_timer_start(state->verify_timer, VERIFY_TIMER_TIMEOUT_VALUE_MS, false /* false = one shot */);
            T_DG(TSN_TRACE_GRP_CONFIG_FP, "port_no = %u: Verify Refresh needed. Status: %s", port_no, TSN_fp_status_verify_to_str(st.status_verify));
        }
    }

    if (!need_verify_refresh) {             // No need to check configuration, mesa_qos_fp_port_conf_set will be called in any case.
        VTSS_RC(mesa_qos_fp_port_conf_get(NULL, port_no, &api_conf_old));
        config_changed = TSN_fp_is_config_different(&api_conf_old, &api_conf_new);
    }

    // We only need to call api if something changed, or if verify status need to be refreshed
    if (config_changed || need_verify_refresh) {
        T_DG(TSN_TRACE_GRP_CONFIG_FP, "port_no = %u: enable_tx: %u verify_dis_tx: %u verify_time: %u frag_size: %u queues 0-7: %u %u %u %u %u %u %u %u", port_no,
             api_conf_new.enable_tx, api_conf_new.verify_disable_tx, api_conf_new.verify_time, api_conf_new.add_frag_size,
             api_conf_new.admin_status[0], api_conf_new.admin_status[1], api_conf_new.admin_status[2], api_conf_new.admin_status[3],
             api_conf_new.admin_status[4], api_conf_new.admin_status[5], api_conf_new.admin_status[6], api_conf_new.admin_status[7]);

        if (((rc = TSN_fp_supported(port_no)) != VTSS_RC_OK)) {
            if (api_conf_new.enable_tx == true) {
#ifdef VTSS_SW_OPTION_SYSLOG
                char syslog_txt[512];
                sprintf(syslog_txt, "FRAME-PREEMPTION DISABLED: %s on Interface %s", error_txt(rc), SYSLOG_PORT_INFO_REPLACE_KEYWORD);
                S_PORT_E(VTSS_ISID_LOCAL, port_no, "%s", syslog_txt);
#endif /* VTSS_SW_OPTION_SYSLOG */
            }
            return VTSS_RC_OK;
        }
        if (!state->link || state->speed > MESA_SPEED_10G) {
            VTSS_ASSERT(state->speed != MESA_SPEED_AUTO);
            // Frame preemption not supported for link speed above 10G, no reason to configure
            // frame preemption when no link
            return VTSS_RC_OK;
        }
        VTSS_RC(mesa_qos_fp_port_conf_set(NULL, port_no, &api_conf_new));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// TSN_fp_port_state_set_all()
/******************************************************************************/
void TSN_fp_port_state_set_all(void)
{
    mesa_port_no_t  port_no;
    for (port_no = 0; port_no < TSN_cap_port_cnt; port_no++) {
        TSN_fp_port_state_set(port_no, false);
    }
}

/******************************************************************************/
// TSN_FP_verify_timeout()
// Timer == verify status
/******************************************************************************/
static void TSN_FP_verify_timeout(tsn_timer_t &timer, void *context)
{
    TSN_LOCK_SCOPE();
    tsn_fp_state_t *tsn_fp_state = (tsn_fp_state_t *)context;
    VTSS_ASSERT(tsn_fp_state);
    tsn_fp_state->timer_count--;
    if (tsn_fp_state->timer_count >= 0) {
        TSN_fp_port_state_set(tsn_fp_state->port, true);
    }
}

/******************************************************************************/
// TSN_fp_default()
/******************************************************************************/
void TSN_fp_default(void)
{
    vtss_appl_tsn_fp_cfg_t  appl_conf = {};
    tsn_preempt_info_t      default_preempt_info = {};
    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t port_status;
    mesa_port_no_t          port_no;
    bool                    link;

    (void)vtss_tsn_fp_get_preempt_info_default(&default_preempt_info);
    (void)vtss_appl_tsn_fp_cfg_get_default(&appl_conf);

    for (port_no = 0; port_no < TSN_cap_port_cnt; port_no++) {
        (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &ifindex);
        if (vtss_appl_port_status_get(ifindex, &port_status) == VTSS_RC_OK) {
            link = port_status.link;
        } else {
            T_W("Failed to get link state for port %d. Assuming link is down", port_no);
            link = false;
        }

        TSN_LOCK_SCOPE();
        tsn.tsn_fp_conf[port_no] = appl_conf;
        tsn.preempt_info[port_no] = default_preempt_info;
        tsn.tsn_fp_state[port_no].port = port_no;
        tsn.tsn_fp_state[port_no].link = link;
        tsn.tsn_fp_state[port_no].timer_count = VERIFY_TIMER_MAX_RETRY;
        tsn_timer_init(tsn.tsn_fp_state[port_no].verify_timer, "verify timer", tsn.tsn_fp_state[port_no].port, TSN_FP_verify_timeout, &tsn.tsn_fp_state[port_no]);
        TSN_fp_port_state_set(port_no, false);
    }
}

//******************************************************************************
// vtss_appl_tsn_fp_cfg_get()
//******************************************************************************
mesa_rc vtss_appl_tsn_fp_cfg_get( vtss_ifindex_t ifindex,
                                  vtss_appl_tsn_fp_cfg_t *const cfg )
{
    vtss_ifindex_elm_t ife = {};
    T_D("Enter");
    VTSS_RC(TSN_ptr_check(cfg));
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_FP_INVALID_IFINDEX;
    }

    VTSS_RC(TSN_fp_supported(ife.ordinal));

    TSN_LOCK_SCOPE();
    *cfg = tsn.tsn_fp_conf[ife.ordinal];
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_fp_cfg_set()
//******************************************************************************
mesa_rc vtss_appl_tsn_fp_cfg_set( vtss_ifindex_t ifindex,
                                  const vtss_appl_tsn_fp_cfg_t *const cfg )
{
    vtss_ifindex_elm_t ife = {};
    T_D("Enter");
    VTSS_RC(TSN_ptr_check(cfg));

    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_D("Only port type interfaces are currently supported");
        return VTSS_APPL_FP_INVALID_IFINDEX;
    }

    if (cfg->verify_time < 1 || cfg->verify_time > 128) {
        T_D("Verify time must be in range [1,128] %d", cfg->verify_time );
        return VTSS_APPL_FP_INVALID_VERIFY_TIME;
    }

    if (cfg->add_frag_size < 0 || cfg->add_frag_size > 2) {
        T_D("Frag size must be in range [0,2] %d", cfg->add_frag_size );
        return VTSS_APPL_FP_INVALID_FRAG_SIZE;
    }
    vtss_appl_qos_scheduler_t qos_schedule_conf;
    for (int i = 0; i < MESA_PRIO_CNT; i++) {
        vtss_appl_qos_scheduler_get(ifindex, i, &qos_schedule_conf);
        if (qos_schedule_conf.cut_through) {
            return VTSS_APPL_FP_CUT_THROUGH;
        }
    }

    VTSS_RC(TSN_fp_supported(ife.ordinal));

    TSN_LOCK_SCOPE();

    if (memcmp(&tsn.tsn_fp_conf[ife.ordinal], cfg, sizeof(vtss_appl_tsn_fp_cfg_t))) {
        tsn.tsn_fp_conf[ife.ordinal] = *cfg;
        return TSN_fp_port_state_set(ife.ordinal, false);
    }
    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_fp_cfg_itr()
//******************************************************************************
mesa_rc vtss_appl_tsn_fp_cfg_itr( const vtss_ifindex_t *const prev_ifindex,
                                  vtss_ifindex_t       *const next_ifindex )
{
    vtss_ifindex_elm_t ife = {};
    if (TSN_cap.has_queue_frame_preemption) {
        vtss_ifindex_t  prev = VTSS_IFINDEX_NONE;
        vtss_ifindex_t  next;

        if (prev_ifindex) {
            prev = *prev_ifindex;
        }

        while (vtss_appl_iterator_ifindex_front_port(&prev, &next) == VTSS_RC_OK) {
            if (vtss_ifindex_decompose(next, &ife) != VTSS_RC_OK) {
                return VTSS_RC_ERROR;
            }

            if (TSN_fp_supported(ife.ordinal) == VTSS_RC_OK) {
                *next_ifindex = next;
                return VTSS_RC_OK;
            } else {
                prev = next;
            }
        }
        return VTSS_RC_ERROR;
    }

    return VTSS_APPL_FP_FEATURE;
}

//******************************************************************************
// vtss_appl_tsn_fp_cfg_get_default()
//******************************************************************************
mesa_rc vtss_appl_tsn_fp_cfg_get_default( vtss_appl_tsn_fp_cfg_t *const cfg )
{
    VTSS_RC(TSN_ptr_check(cfg));

    for (int i = 0; i < MESA_QUEUE_ARRAY_SIZE; i++) {
        cfg->admin_status[i]    = false; // IEEE802.1Qbu: framePreemptionStatusTable
    }
    cfg->enable_tx              = false;
    cfg->ignore_lldp_tx         = false;
    cfg->verify_disable_tx      = false;
    cfg->verify_time            = 10; // millisecs. IEEE803.3br aMACMergeVerifyTime
    cfg->add_frag_size          = TSN_cap.min_add_frag_size;

    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_fp_status_get()
//******************************************************************************
mesa_rc vtss_appl_tsn_fp_status_get(vtss_ifindex_t ifindex, vtss_appl_tsn_fp_status_t *const status)
{
    mesa_qos_fp_port_status_t st = {};
    vtss_ifindex_elm_t        ife;
    mesa_port_no_t            port_no;
    vtss_appl_port_status_t   port_status;
    bool                      link, port_support_preemption;
    mesa_qos_fp_port_conf_t   api_conf = {};

    T_D("Enter");
    VTSS_RC(TSN_ptr_check(status));
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_FP_INVALID_IFINDEX;
    }

    port_no = ife.ordinal;
    port_support_preemption = (TSN_fp_supported(port_no) == VTSS_RC_OK);

    if (vtss_appl_port_status_get(ifindex, &port_status) == VTSS_RC_OK) {
        link = port_status.link;
    } else {
        T_W("Failed to get link state for port %d. Assuming link is down", port_no);
        link = false;
    }

    TSN_LOCK_SCOPE();

    VTSS_RC(mesa_qos_fp_port_conf_get(NULL, port_no, &api_conf));
    VTSS_RC(mesa_qos_fp_port_status_get(NULL, port_no, &st));
    status->hold_advance        =    st.hold_advance;
    status->release_advance     =    st.release_advance;
    status->preemption_active   =    api_conf.enable_tx && st.preemption_active;
    status->hold_request        =    st.hold_request;

    if (link) {
        switch (st.status_verify) {
        case MESA_MM_STATUS_VERIFY_INITIAL:
            status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_INITIAL;
            break;
        case MESA_MM_STATUS_VERIFY_IDLE:
            status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_IDLE;
            break;
        case MESA_MM_STATUS_VERIFY_SEND:
            status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_SEND;
            break;
        case MESA_MM_STATUS_VERIFY_WAIT:
            status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_WAIT;
            break;
        case MESA_MM_STATUS_VERIFY_SUCCEEDED:
            status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_SUCCEEDED;
            break;
        case MESA_MM_STATUS_VERIFY_FAILED:
            status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_FAILED;
            break;
        case MESA_MM_STATUS_VERIFY_DISABLED:
            status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_DISABLED;
            break;
        default:
            status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_UNKNOWN;
        }
    } else {
        // If port is not up, we do not trust status of verify process
        status->status_verify = VTSS_APPL_TSN_MM_STATUS_VERIFY_UNKNOWN;
    }

    // These 4 parameters (loc_) are used for generation of lldp_tx information.
    // To inform linkpartner of local conditions and requirements.
    status->loc_preempt_supported = TSN_cap.has_queue_frame_preemption && port_support_preemption;
    status->loc_preempt_enabled   = tsn.tsn_fp_conf[port_no].enable_tx; // No need to use port_support_preemption. It is not possible to configure this to enable when no preemption support.
    status->loc_preempt_active    = st.preemption_active; // TBD: The same as status->preemption_active, but we keep it like this for the time being
    status->loc_add_frag_size     = tsn.preempt_info[port_no].debug_add_frag_size > TSN_cap.min_add_frag_size ? tsn.preempt_info[port_no].debug_add_frag_size : TSN_cap.min_add_frag_size;

    return VTSS_RC_OK;
}

//******************************************************************************
// vtss_appl_tsn_fp_status_itr()
//******************************************************************************
mesa_rc vtss_appl_tsn_fp_status_itr(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex
)
{
    if (TSN_cap.has_queue_frame_preemption) {
        return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
    }

    return VTSS_APPL_FP_FEATURE;
}

//******************************************************************************
// TSN_port_change_cb()
// Callback from port link up/ link down
//******************************************************************************
void TSN_port_change_cb(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    T_DG(TSN_TRACE_GRP_CALLBACK, "port_no: %u, link: %d, speed: %d", port_no, status->link, status->speed);
    TSN_LOCK_SCOPE();
    if (tsn.tsn_fp_state[port_no].link != status->link ||
        tsn.tsn_fp_state[port_no].speed != status->speed) {
        // link status changed
        tsn.tsn_fp_state[port_no].link = status->link;
        tsn.tsn_fp_state[port_no].speed = status->speed;
        if (!status->link) {
            // if link went down, clear values received through lldp.
            // because this may mean that our link partner changed.
            tsn.preempt_info[port_no].supported = false;
            tsn.preempt_info[port_no].enabled = false;
            tsn.preempt_info[port_no].active = false;
            tsn.preempt_info[port_no].add_frag_size = 0;
        }
        TSN_fp_port_state_set(port_no, false);
    }
}

#if defined(VTSS_SW_OPTION_LLDP)
//******************************************************************************
// TSN_lldp_change_cb()
// Callback from port LLDP frame preemption value changes
//******************************************************************************
void TSN_lldp_change_cb(mesa_port_no_t port_no, vtss_appl_lldp_remote_entry_t *entry)
{
    T_DG(TSN_TRACE_GRP_CALLBACK, "port_no: %u, rx_info_ttl: %u, supported: %d, enabled: %d, active: %d, add_frag_size: %u",
         port_no, entry->rx_info_ttl,
         entry->fp.RemFramePreemptSupported, entry->fp.RemFramePreemptEnabled,
         entry->fp.RemFramePreemptActive, entry->fp.RemFrameAddFragSize);
    TSN_LOCK_SCOPE();
    if (tsn.tsn_fp_state[port_no].link) {
        T_DG(TSN_TRACE_GRP_CALLBACK, "port_no: %u LLDP LINK", port_no);
        if ((tsn.preempt_info[port_no].supported != entry->fp.RemFramePreemptSupported) ||
            (tsn.preempt_info[port_no].enabled != entry->fp.RemFramePreemptEnabled)     ||
            (tsn.preempt_info[port_no].add_frag_size != entry->fp.RemFrameAddFragSize)) { /* LLDP preempt info has changed */
            tsn.preempt_info[port_no].supported     = entry->fp.RemFramePreemptSupported;
            tsn.preempt_info[port_no].enabled       = entry->fp.RemFramePreemptEnabled;
            tsn.preempt_info[port_no].active        = entry->fp.RemFramePreemptActive; /* Not needed in the comparison above */
            tsn.preempt_info[port_no].add_frag_size = entry->fp.RemFrameAddFragSize;
        }
    } else {
        T_DG(TSN_TRACE_GRP_CALLBACK, "port_no: %u LLDP NO LINK", port_no);
        tsn.preempt_info[port_no].supported     = false;
        tsn.preempt_info[port_no].enabled       = false;
        tsn.preempt_info[port_no].active        = false;
        tsn.preempt_info[port_no].add_frag_size = 0;
    }
    TSN_fp_port_state_set(port_no, false);
}
#endif /* VTSS_SW_OPTION_LLDP */
