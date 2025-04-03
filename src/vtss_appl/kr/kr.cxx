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

#include "main.h"
#include "port_api.h"
#include "kr_api.h"
#include "interrupt_api.h"
#include <sys/time.h>
#include <unistd.h>
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

// IRQ-based is now the default. Define it to 0 if you want the poll-based
#define KR_IRQ_BASED 1

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_KR

#include <vtss_trace_api.h>
#include "critd_api.h"
#include "microchip/ethernet/switch/api.h"

static vtss_trace_reg_t KR_trace_reg = {
    VTSS_TRACE_MODULE_ID, "kr", "KR"
};

#define KR_TRACE_GRP_BASE 1

static vtss_trace_grp_t KR_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [KR_TRACE_GRP_BASE] = {
        "base",
        "Base",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&KR_trace_reg, KR_trace_grps);

static critd_t       KR_crit;
static vtss_flag_t   KR_flag;
static uint32_t      KR_cap_port_cnt;
static bool          KR_cap_v2; // JR2
static bool          KR_cap_v3; // SparX-5
static vtss_handle_t KR_thread_handle;
static vtss_thread_t KR_thread_block;

#define KR_FLAG_KICK_THREAD (1 << 0)
#define KR_HIST_NUM 1000

struct KR_Lock {
    KR_Lock(const char *func, int line)
    {
        critd_enter(&KR_crit, func, line);
    }

    ~KR_Lock()
    {
        critd_exit(&KR_crit, __FILE__, 0);
    }
};

#define KR_LOCK_SCOPE() KR_Lock __lock_guard__(__FUNCTION__, __LINE__);

typedef struct {
    mesa_port_kr_state_t  state;
    struct timeval        time_start_aneg;
    struct timeval        time_start_train;
    uint32_t              time_ld;
    uint32_t              time_lp;
    uint32_t              time_since_restart;
    mesa_port_kr_status_t status;
    kr_coef_t             eq_hist[KR_HIST_NUM];
    uint16_t              eq_hist_index;
    kr_ber_t              ber_hist[KR_HIST_NUM];
    uint16_t              ber_hist_index;
    kr_irq_t              irq_hist[KR_HIST_NUM];
    uint16_t              irq_hist_index;
} kr_appl_train_t;

typedef struct {
    // Port number that this instance belongs to (to be able to identify itself)
    mesa_port_no_t port_no;

    // Port capabilities as reported by MEBA. Write once, read many
    meba_port_cap_t port_cap;

    // Is 10G port with KR capabilities? Write once, read many
    bool cap_10g;

    // Is 25G port with KR capabilities? Write once, read many
    bool cap_25g;

    // Configuration
    mesa_port_kr_conf_t conf;

    // Status
    mesa_port_kr_status_t status;

    kr_appl_train_t   tr;

    // Parallel detect
    mesa_bool_t pd;
    mesa_port_speed_t next_parallel_spd;

    // Debug
    uint32_t aneg_sm_state;
    uint32_t aneg_sm_deb;
    bool     train_done;
    bool     compl_ack_done;
    bool     pcs_flap;
} kr_state_t;

/* Global structure */
typedef struct {
    ulong          value;    /* Read value */
    mesa_chip_no_t chip_no;  /* Chip number context */

    uint32_t       kr_thread_ms_poll;
    bool           kr_thread_disable;

    bool           kr_debug;
} kr_global_t;

static CapArray<kr_state_t, MEBA_CAP_BOARD_PORT_COUNT> KR_states;
static kr_global_t kr;

/******************************************************************************/
// KR_irq_to_spd()
/******************************************************************************/
static mesa_port_speed_t KR_irq_to_spd(uint32_t irq)
{
    switch (irq) {
    case MESA_KR_ANEG_RATE_25G:
        return MESA_SPEED_25G;

    case MESA_KR_ANEG_RATE_10G:
        return MESA_SPEED_10G;

    case MESA_KR_ANEG_RATE_5G:
        return MESA_SPEED_5G;

    case MESA_KR_ANEG_RATE_2G5:
        return MESA_SPEED_2500M;

    case MESA_KR_ANEG_RATE_1G:
        return MESA_SPEED_1G;

    default:
        T_E("KR speed (%u) not supported", irq);
    }

    return MESA_SPEED_10G;
}

/******************************************************************************/
// kr_printf()
/******************************************************************************/
static void kr_printf(const char *fmt, ...)
{
    if (!kr.kr_debug) {
        return;
    }

    va_list ap;
    char local_buf[1024];

    va_start(ap, fmt);
    vsnprintf(local_buf, 1024, fmt, ap);
    va_end(ap);
    printf("%s", local_buf);
}

/******************************************************************************/
// KR_time_start()
/******************************************************************************/
static void KR_time_start(struct timeval *store)
{
    (void)gettimeofday(store, NULL);
}

/******************************************************************************/
// KR_get_time_us()
/******************************************************************************/
static uint32_t KR_get_time_us(struct timeval *store)
{
    struct timeval stop;
    (void)gettimeofday(&stop, NULL);
    return ((stop.tv_sec - store->tv_sec) * 1000000 + stop.tv_usec - store->tv_usec);
}

/******************************************************************************/
// KR_get_time_ms()
/******************************************************************************/
static uint32_t KR_get_time_ms(struct timeval *store)
{
    struct timeval stop;
    (void)gettimeofday(&stop, NULL);
    return ((stop.tv_sec - store->tv_sec) * 1000000 + stop.tv_usec - store->tv_usec) / 1000;
}

/******************************************************************************/
// KR_get_time_sec()
/******************************************************************************/
static uint32_t KR_get_time_sec(struct timeval *store)
{
    struct timeval stop;
    (void)gettimeofday(&stop, NULL);
    return ((stop.tv_sec - store->tv_sec) * 1000000 + stop.tv_usec - store->tv_usec) / 1000000;
}

/******************************************************************************/
// KR_state_init()
/******************************************************************************/
static void KR_state_init(kr_state_t *kr_state)
{
    vtss_clear(kr_state->status);
    vtss_clear(kr_state->tr);

    kr_state->train_done        = false;
    kr_state->compl_ack_done    = false;
    kr_state->aneg_sm_deb       = 0;
    kr_state->next_parallel_spd = MESA_SPEED_2500M;
    KR_time_start(&kr_state->tr.time_start_aneg);
}

/******************************************************************************/
// KR_next_parallel_spd_get()
/******************************************************************************/
static mesa_port_speed_t KR_next_parallel_spd_get(kr_state_t *kr_state)
{
    if (kr_state->next_parallel_spd == MESA_SPEED_25G) {
        kr_state->next_parallel_spd = MESA_SPEED_10G;
        return MESA_SPEED_25G;
    } else if (kr_state->next_parallel_spd == MESA_SPEED_10G) {
        kr_state->next_parallel_spd = MESA_SPEED_5G;
        return MESA_SPEED_10G;
    } else if (kr_state->next_parallel_spd == MESA_SPEED_5G) {
        kr_state->next_parallel_spd = MESA_SPEED_2500M;
        return MESA_SPEED_5G;
    } else if (kr_state->next_parallel_spd == MESA_SPEED_2500M) {
        kr_state->next_parallel_spd = MESA_SPEED_1G;
        return MESA_SPEED_2500M;
    } else  {
        kr_state->next_parallel_spd = kr_state->cap_25g ? MESA_SPEED_25G : MESA_SPEED_10G;
        return MESA_SPEED_1G;
    }
}

/******************************************************************************/
// KR_add_to_irq_history()
/******************************************************************************/
static void KR_add_to_irq_history(mesa_port_no_t p, uint32_t irq, mesa_port_kr_status_t *status)
{
    kr_appl_train_t *krs = &KR_states[p].tr;

    if (krs->irq_hist_index == KR_HIST_NUM) {
        krs->irq_hist_index = 0;
    }

    if (krs->irq_hist_index < KR_HIST_NUM) {
        krs->irq_hist[krs->irq_hist_index].time = KR_get_time_us(&krs->time_start_aneg);
        krs->irq_hist[krs->irq_hist_index].irq = irq;
        krs->irq_hist[krs->irq_hist_index].sm = status->aneg.sm;
        krs->irq_hist[krs->irq_hist_index].lp_bp0 = status->aneg.lp_bp0;
        krs->irq_hist[krs->irq_hist_index].lp_bp1 = status->aneg.lp_bp1;
        krs->irq_hist[krs->irq_hist_index].lp_bp2 = status->aneg.lp_bp2;
        krs->irq_hist[krs->irq_hist_index].lp_np0 = status->aneg.lp_np0;
        krs->irq_hist[krs->irq_hist_index].lp_np1 = status->aneg.lp_np1;
        krs->irq_hist[krs->irq_hist_index].lp_np2 = status->aneg.lp_np2;
        krs->irq_hist_index++;
    }
}

/******************************************************************************/
// KR_add_to_eq_history()
/******************************************************************************/
static void KR_add_to_eq_history(mesa_port_no_t p, mesa_kr_status_results_t res)
{
    kr_appl_train_t *krs = &KR_states[p].tr;

    if (krs->eq_hist_index == KR_HIST_NUM) {
        krs->eq_hist_index = 0;
    }

    if (krs->eq_hist_index < KR_HIST_NUM) {
        krs->eq_hist[krs->eq_hist_index].time = KR_get_time_ms(&krs->time_start_train);
        krs->eq_hist[krs->eq_hist_index].res = res;
        krs->eq_hist_index++;
    }
}

/******************************************************************************/
// KR_add_to_ber_history()
/******************************************************************************/
static void KR_add_to_ber_history(mesa_port_no_t p, uint32_t irq)
{
    kr_appl_train_t *kr = &KR_states[p].tr;
    mesa_port_kr_state_t *krs = &KR_states[p].tr.state;

    if (kr->ber_hist_index == KR_HIST_NUM) {
        kr->ber_hist_index = 0;
    }

    if (kr->ber_hist_index < KR_HIST_NUM) {
        kr->ber_hist[kr->ber_hist_index].time = KR_get_time_ms(&kr->time_start_train);
        if (irq & MESA_KR_LPSVALID) {
            kr->ber_hist[kr->ber_hist_index].ber_coef_frm = krs->ber_coef_frm;
            kr->ber_hist[kr->ber_hist_index].ber_status_frm = krs->ber_status_frm;
        } else if ((irq & MESA_KR_TRAIN) || (irq & MESA_KR_BER_BUSY_0)) {
            kr->ber_hist[kr->ber_hist_index].ber_coef_frm = krs->ber_coef_frm;
            kr->ber_hist[kr->ber_hist_index].ber_status_frm = 0xdead;
        }
        kr->ber_hist[kr->ber_hist_index].ber_training_stage = krs->ber_training_stage;
        kr->ber_hist[kr->ber_hist_index].irq = irq;
        kr->ber_hist_index++;
    }
}

/******************************************************************************/
// KR_port_kr_conf_get()
/******************************************************************************/
static mesa_rc KR_port_kr_conf_get(mesa_port_no_t port_no, mesa_port_kr_conf_t *conf)
{
    mesa_rc rc;

    if ((rc = mesa_port_kr_conf_get(NULL, port_no, conf)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_conf_get() failed: %s", error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_conf_get() => enable = %d", conf->aneg.enable);
    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_port_kr_conf_set()
/******************************************************************************/
static mesa_rc KR_port_kr_conf_set(mesa_port_no_t port_no, const mesa_port_kr_conf_t *conf)
{
    mesa_rc rc;

    T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_conf_set(enable = %d)", conf->aneg.enable);
    if ((rc = mesa_port_kr_conf_set(NULL, port_no, conf)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_conf_set(enable = %d) failed: %s", conf->aneg.enable, error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_port_kr_status_get()
// Only invoked if kr_state->conf.aneg.enable is true.
/******************************************************************************/
static mesa_rc KR_port_kr_status_get(kr_state_t *kr_state)
{
    mesa_rc rc;

    // 10G KR surveillance
    if ((rc = mesa_port_kr_status_get(NULL, kr_state->port_no, &kr_state->status)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, kr_state->port_no, "mesa_port_kr_status_get() failed: %s", error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    T_DG_PORT(KR_TRACE_GRP_BASE, kr_state->port_no, "mesa_port_kr_status_get() => r_fec = %d, rs_fec = %d", kr_state->status.aneg.r_fec_enable, kr_state->status.aneg.rs_fec_enable);

    if (KR_cap_v2 && !kr_state->status.aneg.complete) {
        KR_time_start(&kr_state->tr.time_start_aneg);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_port_kr_state_get()
/******************************************************************************/
static mesa_rc KR_port_kr_state_get(mesa_port_no_t port_no, mesa_port_kr_state_t *state)
{
    mesa_rc rc;

    if ((rc = mesa_port_kr_state_get(NULL, port_no, state)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_state_get() failed: %s", error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_state_get() => started = %d, cur-state = %s", state->training_started, kr_util_state_to_txt(state->current_state));
    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_port_kr_irq_get()
/******************************************************************************/
static mesa_rc KR_port_kr_irq_get(mesa_port_no_t port_no, uint32_t *irq)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    char    buf[1000];
#endif
    mesa_rc rc;

    if ((rc = mesa_port_kr_irq_get(NULL, port_no, irq)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_irq_get() failed: %s", error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_irq_get() => irq = 0x%x = %s", *irq, kr_util_irq_mask_to_txt(*irq, buf));
    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_port_kr_irq_apply()
/******************************************************************************/
static mesa_rc KR_port_kr_irq_apply(mesa_port_no_t port_no, const uint32_t *irq)
{
    char    buf[1000];
    mesa_rc rc;

    T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_irq_apply(irq = 0x%x = %s)", *irq, kr_util_irq_mask_to_txt(*irq, buf));
    if ((rc = mesa_port_kr_irq_apply(NULL, port_no, irq)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_irq_apply(irq = 0x%x = %s) failed: %s", *irq, kr_util_irq_mask_to_txt(*irq, buf), error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_port_kr_fec_set()
/******************************************************************************/
static mesa_rc KR_port_kr_fec_set(mesa_port_no_t port_no, const mesa_port_kr_fec_t *fec)
{
    mesa_rc rc;

    T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_fec_set(r_fec = %d, rs_fec = %d)", fec->r_fec, fec->rs_fec);
    if ((rc = mesa_port_kr_fec_set(NULL, port_no, fec)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_fec_set(r_fec = %d, rs_fec = %d) failed: %s", fec->r_fec, fec->rs_fec, error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_port_conf_get()
/******************************************************************************/
static mesa_rc KR_port_conf_get(mesa_port_no_t port_no, mesa_port_conf_t *pconf)
{
    mesa_rc rc;

    if ((rc = mesa_port_conf_get(NULL, port_no, pconf)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_conf_get() failed: %s", error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_conf_get() => %s", *pconf);
    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_port_conf_set()
/******************************************************************************/
static mesa_rc KR_port_conf_set(mesa_port_no_t port_no, const mesa_port_conf_t *pconf, const char *func, int line)
{
    mesa_rc rc;

    T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "%s#%d: mesa_port_conf_set(%s)", func, line, *pconf);
    if ((rc = mesa_port_conf_set(NULL, port_no, pconf)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "%s#%d: mesa_port_conf_set(%s) failed: %s", func, line, *pconf, error_txt(rc));

        // Convert to something sensible
        return VTSS_APPL_KR_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// KR_poll_v3()
// SparX-5 (FA) KR version
//
// kr_state->status is already updated.
//
// Notice that KR aneg will not complete if debug trace is enabled. Use info
// trace if you need some (limited) output.
/******************************************************************************/
static mesa_rc KR_poll_v3(kr_state_t *kr_state)
{
    mesa_port_conf_t     pconf;
    mesa_port_kr_state_t *krs;
    uint32_t             irq;
    kr_appl_train_t      *krt;
    mesa_port_kr_fec_t   fec;

    krt = &kr_state->tr;
    krs = &krt->state;

    // Private function to indicate to the port module the completion state of
    // aneg/training
    void port_kr_completeness_set(mesa_port_no_t port_no, bool complete);

    // Poll the IRQs
    VTSS_RC(KR_port_kr_irq_get(kr_state->port_no, &irq));

    if (irq == 0) {
        return VTSS_RC_OK;
    }

    // Re-start the aneg timer if AN_XMIT_DISABLE
    if ((irq & MESA_KR_AN_XMIT_DISABLE) || (irq & MESA_KR_LINK_FAIL)) {
        // This is the first state after link loss or enable. Tell the port
        // module that we are going to aneg and possibly train.
        port_kr_completeness_set(kr_state->port_no, false);

        // Before we start messing with the port configuration, better bring
        // down switching to/from this port.
        // Upon the next poll, the port module will also call it with link-down,
        // because we've already said that KR aneg is not complete.
        (void)mesa_port_state_set(nullptr, kr_state->port_no, false);

        kr_state->compl_ack_done = false;
        kr_state->train_done = false;
        KR_time_start(&krt->time_start_aneg);
    }

    // Start the train timer if KR_TRAIN IRQ
    if (irq & MESA_KR_TRAIN) {
        kr_state->train_done = false;
        KR_time_start(&krt->time_start_train);
        // Adjust CTLE Rx setings
        int cnt = 0;
        while (mesa_port_kr_ctle_adjust(NULL, kr_state->port_no) != MESA_RC_OK && cnt < 3) {
            usleep(1000); // 1ms
            cnt++;
        }
    }

    if (irq & MESA_KR_CMPL_ACK) {
        kr_state->compl_ack_done = true;
    }

    // KR_AN_RATE (Change to the speed/FEC requested)
    if (irq & MESA_KR_AN_RATE) {
        VTSS_RC(KR_port_conf_get(kr_state->port_no, &pconf));
        if (pconf.power_down) {
            return VTSS_RC_OK;
        }
        // Aneg speed change request
        if (pconf.speed != KR_irq_to_spd(irq & 0xf)) {
            pconf.speed = KR_irq_to_spd(irq & 0xf);
            pconf.if_type = pconf.speed > MESA_SPEED_2500M ? MESA_PORT_INTERFACE_SFI : MESA_PORT_INTERFACE_SERDES;
            VTSS_RC(KR_port_conf_set(kr_state->port_no, &pconf, __FUNCTION__, __LINE__));
        }
        kr_printf("Port %u: Aneg speed is %s (%u ms). ", kr_state->port_no, kr_util_spd_to_txt(pconf.speed), KR_get_time_ms(&krt->time_start_aneg));
        // Aneg FEC change request
        if (kr_state->status.aneg.request_fec_change) {
            vtss_clear(fec);
            fec.r_fec  = kr_state->status.aneg.r_fec_enable;
            fec.rs_fec = kr_state->status.aneg.rs_fec_enable;
            kr_printf("Port %u: R-FEC = %d, RS-FEC = %d\n", kr_state->port_no, fec.r_fec, fec.rs_fec);
            VTSS_RC(KR_port_kr_fec_set(kr_state->port_no, &fec));
        }

        if (kr_state->compl_ack_done) {
            if (kr_state->conf.train.enable) {
                kr_printf("Aneg is complete.  Now start training\n");
            } else {
                kr_printf("Aneg is complete.  Training is disabled\n");
            }
        } else {
            kr_printf("Now complete Aneg.\n");
        }
    }

    // KR_RATE_DET (Parallel detect: link partner does not have Aneg support)
    if (irq & MESA_KR_RATE_DET && kr_state->pd) {
        // Disable FEC for paralled detect modes
        VTSS_RC(KR_port_conf_get(kr_state->port_no, &pconf));
        if (pconf.power_down) {
            return VTSS_RC_OK;
        }
        vtss_clear(fec);
        VTSS_RC(KR_port_kr_fec_set(kr_state->port_no, &fec));

        pconf.speed = KR_next_parallel_spd_get(kr_state);
        pconf.if_type = pconf.speed > MESA_SPEED_2500M ? MESA_PORT_INTERFACE_SFI : MESA_PORT_INTERFACE_SERDES;
        VTSS_RC(KR_port_conf_set(kr_state->port_no, &pconf, __FUNCTION__, __LINE__));
        kr_printf("Port %u: Parallel detect speed is %s (%u ms)\n", kr_state->port_no, kr_util_spd_to_txt(pconf.speed), KR_get_time_ms(&krt->time_start_aneg));
    }

    // Apply the IRQs
    VTSS_RC(KR_port_kr_irq_apply(kr_state->port_no, &irq));

    // Get the internal training state for debug purposes
    VTSS_RC(KR_port_kr_state_get(kr_state->port_no, krs));

    // Add IRQs to history
    KR_add_to_irq_history(kr_state->port_no, irq, &kr_state->status);

    // Add BER state to history
    if (irq & MESA_KR_LPSVALID || irq & MESA_KR_TRAIN || irq & MESA_KR_BER_BUSY_0) {
        KR_add_to_ber_history(kr_state->port_no, irq);
    }

    // Add Equalizer state to history
    if (irq & MESA_KR_LPCVALID) {
        KR_add_to_eq_history(kr_state->port_no, krs->tr_res);
    }

    // Training completed
    if ((irq & MESA_KR_WT_DONE) && krs->current_state == MESA_TR_SEND_DATA) {
        krt->time_ld = KR_get_time_ms(&krt->time_start_train);
        kr_printf("Port %u: Training completed (%u ms)\n", kr_state->port_no, KR_get_time_ms(&krt->time_start_train));

        if (!kr.kr_debug) {
            T_IG_PORT(KR_TRACE_GRP_BASE, kr_state->port_no, "Training completed (%u ms)", KR_get_time_ms(&krt->time_start_train));
        }
        kr_state->train_done = true;
    }

    // Aneg completed
    if (irq & MESA_KR_AN_GOOD) {
        VTSS_RC(KR_port_conf_get(kr_state->port_no, &pconf));
        kr_printf("Port:%d - AN_GOOD (%s) in %d ms\n", kr_state->port_no, kr_util_spd_to_txt(pconf.speed), KR_get_time_ms(&krt->time_start_aneg));
        // Tell port module that we're done - with or without training - now
        port_kr_completeness_set(kr_state->port_no, true);
        kr_state->next_parallel_spd = MESA_SPEED_2500M;
    }

    return VTSS_RC_OK;
}

#if KR_IRQ_BASED
/******************************************************************************/
// KR_interrupt_callback()
/******************************************************************************/
static void KR_interrupt_callback(meba_event_t source_id, uint32_t port_no)
{
    kr_state_t     *kr_state;
    mesa_port_no_t port_itr;
    uint32_t       mask;
    mesa_rc        rc;

    if (!kr_mgmt_port_capable(port_no)) {
        T_E_PORT(port_no, "Not KR capable");
    }

    // Re-register for KR interrupts
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_KR, KR_interrupt_callback, MEBA_EVENT_KR, INTERRUPT_PRIORITY_HIGHEST)) != VTSS_RC_OK) {
        T_E_PORT(port_no, "vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }

    KR_LOCK_SCOPE();

    // Get the IRQ actvity mask
    (void)mesa_port_kr_irq_activity(NULL, &mask);

    if (!mask) {
        // Nothing to do.
        return;
    }

    // If any activity run through the ports
    for (port_itr = 0; port_itr < KR_cap_port_cnt; port_itr++) {
        kr_state = &KR_states[port_itr];

        if (!kr_state->conf.aneg.enable) {
            continue;
        }

        (void)KR_port_kr_status_get(kr_state);
        (void)KR_poll_v3(kr_state);
    }
}
#endif

/******************************************************************************/
// KR_thread()
// Will be running whenever at least one port is aneg enabled.
/******************************************************************************/
static void KR_thread(vtss_addrword_t data)
{
    kr_state_t     *kr_state;
    mesa_port_no_t port_no;
    uint32_t       poll_time_us;
    bool           keep_running;

    if (KR_cap_v2) {
        // JR2 has H/W-based KR training, so just poll status every 50 ms.
        poll_time_us = 50000;
    } else {
        // SparX-5 has S/W-based KR training. It may use IRQ or polled.
        // If running IRQ-based, we just poll the status every 50 ms, as for
        // JR2, but if it is poll-based, we must poll every 200 usecs (SIC!).
#if KR_IRQ_BASED
        poll_time_us = 50000;
#else
        poll_time_us = 200;
#endif
    }

    // Controls whether we wait for another "go" from kr_mgmt_port_conf_set()
    // or we are free running.
    keep_running = false;

    while (1) {
        if (keep_running) {
            usleep(poll_time_us);
        } else {
            // No ports in KR mode. Wait for one to get configured to that.
            (void)vtss_flag_wait(&KR_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR);
        }

        KR_LOCK_SCOPE();

        keep_running = false;
        for (port_no = 0; port_no < KR_cap_port_cnt; port_no++) {
            kr_state = &KR_states[port_no];

            if (!kr_state->conf.aneg.enable) {
                continue;
            }

            // At least one port has aneg enabled. Keep going.
            keep_running = true;

            // Get status. Required on JR2 in order to keep the state machine
            // in the API running.
            (void)KR_port_kr_status_get(kr_state);

#if !KR_IRQ_BASED
            if (KR_cap_v3) {
                // SparX-5 and we are not using IRQs to do the training.
                (void)KR_poll_v3(kr_state);
            }
#endif
        }
    }
}

/******************************************************************************/
// kr_mgmt_capable()
/******************************************************************************/
bool kr_mgmt_capable(bool *v3)
{
    if (v3) {
        if (KR_cap_v3) {
            *v3 = true;
        } else {
            *v3 = false;
        }
    }

    return KR_cap_v2 || KR_cap_v3;
}

/******************************************************************************/
// kr_mgmt_port_capable()
/******************************************************************************/
bool kr_mgmt_port_capable(mesa_port_no_t port_no)
{
    if (port_no >= KR_cap_port_cnt) {
        T_E("Illegal port_no: %u ", port_no);
        return false;
    }

    return KR_states[port_no].cap_10g || KR_states[port_no].cap_25g;
}

/******************************************************************************/
// kr_mgmt_port_conf_check()
/******************************************************************************/
mesa_rc kr_mgmt_port_conf_check(mesa_port_no_t port_no, const mesa_port_kr_conf_t *conf)
{
    meba_port_cap_t port_cap;
    bool            at_least_one_speed_advertised;

    if (conf == nullptr) {
        return VTSS_APPL_KR_RC_INVALID_PARAMETER;
    }

    T_D_PORT(port_no, "%s", *conf);

    if (!conf->aneg.enable) {
        // Nothing to check unless enabled.
        return VTSS_RC_OK;
    }

    if (!kr_mgmt_capable()) {
        // Use the RC from the port module, which is a generic form of saying:
        // "This platform does not support KR at all".
        return VTSS_APPL_PORT_RC_KR_NOT_SUPPORTED_ON_THIS_PLATFORM;
    }

    if (!kr_mgmt_port_capable(port_no)) {
        return VTSS_APPL_KR_RC_PORT_NOT_CAPABLE;
    }

    port_cap = KR_states[port_no].port_cap;

    // Check that the advertise members are correct.
    at_least_one_speed_advertised = false;
    if (conf->aneg.adv_25g) {
        if (!KR_cap_v3 || (port_cap & MEBA_PORT_CAP_25G_FDX) == 0) {
            // Must be running v3 AND the port must be supporting 25G
            T_I_PORT(port_no, "25G", port_no);
            return VTSS_APPL_KR_RC_ADV_25G;
        }

        at_least_one_speed_advertised = true;
    }

    if (conf->aneg.adv_10g) {
        if ((port_cap & MEBA_PORT_CAP_10G_FDX) == 0) {
            // The port must be supporting 10G
            T_I_PORT(port_no, "10G");
            return VTSS_APPL_KR_RC_ADV_10G;
        }

        at_least_one_speed_advertised = true;
    }

    if (conf->aneg.adv_5g) {
        if (!KR_cap_v3 || (port_cap & MEBA_PORT_CAP_5G_FDX) == 0) {
            // Must be running v3 AND the port must be supporting 5G
            T_I_PORT(port_no, "5G");
            return VTSS_APPL_KR_RC_ADV_5G;
        }

        at_least_one_speed_advertised = true;
    }

    if (conf->aneg.adv_2g5) {
        if (!KR_cap_v3 || (port_cap & MEBA_PORT_CAP_2_5G_FDX) == 0) {
            // Must be running v3 AND the port must be supporting 2.5G
            T_I_PORT(port_no, "2.5G");
            return VTSS_APPL_KR_RC_ADV_2G5;
        }

        at_least_one_speed_advertised = true;
    }

    if (conf->aneg.adv_1g) {
        if (!KR_cap_v3 || (port_cap & MEBA_PORT_CAP_1G_FDX) == 0) {
            // Must be running v3 AND the port must be supporting 1G
            T_I_PORT(port_no, "1G");
            return VTSS_APPL_KR_RC_ADV_1G;
        }

        at_least_one_speed_advertised = true;
    }

    if (!at_least_one_speed_advertised) {
        T_I_PORT(port_no, "No speeds advertised");
        return VTSS_APPL_KR_RC_ADV_NONE_SELECTED;
    }

    if (conf->aneg.r_fec_req) {
        // When r_fec is requested, so must at least one of 5G, 10G or 25G.
        if (!conf->aneg.adv_5g && !conf->aneg.adv_10g && !conf->aneg.adv_25g) {
            T_I_PORT(port_no, "R-FEC");
            return VTSS_APPL_KR_RC_R_FEC;
        }
    }

    if (conf->aneg.rs_fec_req) {
        // When rs_fec is requested, so must 25G.
        if (!conf->aneg.adv_25g) {
            T_I_PORT(port_no, "RS-FEC");
            return VTSS_APPL_KR_RC_RS_FEC;
        }
    }

    // Just a note: It doesn't make sense to train without also anegging.

    if (!conf->train.enable || KR_cap_v3) {
        // Training is not enabled or we are running V3, which supports all
        // sub-parameters.
        return VTSS_RC_OK;
    }

    // Training is enabled and we are running V2, which doesn't support any
    // sub-parameters.
    if (conf->train.no_remote) {
        T_I_PORT(port_no, "No remote");
        return VTSS_APPL_KR_RC_TRAIN_NO_REMOTE;
    }

    if (conf->train.use_ber_cnt) {
        T_I_PORT(port_no, "Use BER cnt");
        return VTSS_APPL_KR_RC_TRAIN_BER_CNT;
    }

    if (conf->train.test_mode) {
        T_I_PORT(port_no, "Test mode");
        return VTSS_APPL_KR_RC_TRAIN_TEST_MODE;
    }

    if (conf->train.test_repeat) {
        T_I_PORT(port_no, "No remote");
        return VTSS_APPL_KR_RC_TRAIN_TEST_REPEAT;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// kr_mgmt_port_conf_default_get()
/******************************************************************************/
mesa_rc kr_mgmt_port_conf_default_get(mesa_port_no_t port_no, mesa_port_kr_conf_t *conf)
{
    meba_port_cap_t port_cap;

    if (conf == nullptr) {
        return VTSS_APPL_KR_RC_INVALID_PARAMETER;
    }

    vtss_clear(*conf);

    if (!kr_mgmt_port_capable(port_no)) {
        // Nothing else to do.
        return VTSS_RC_OK;
    }

    port_cap = KR_states[port_no].port_cap;

    // Default to advertise everything we can advertise on this port.
    if (KR_cap_v3 && (port_cap & MEBA_PORT_CAP_25G_FDX)) {
        conf->aneg.adv_25g = true;
    }

    if (port_cap & MEBA_PORT_CAP_10G_FDX) {
        conf->aneg.adv_10g = true;
    }

    if (KR_cap_v3 && (port_cap & MEBA_PORT_CAP_5G_FDX)) {
        conf->aneg.adv_5g = true;
    }

    if (KR_cap_v3 && (port_cap & MEBA_PORT_CAP_2_5G_FDX)) {
        conf->aneg.adv_2g5 = true;
    }

    if (KR_cap_v3 && (port_cap & MEBA_PORT_CAP_1G_FDX)) {
        conf->aneg.adv_1g = true;
    }

    conf->aneg.r_fec_req  = conf->aneg.adv_10g;
    conf->aneg.rs_fec_req = conf->aneg.adv_25g;

    conf->train.enable = true;

    return VTSS_RC_OK;
}

/******************************************************************************/
// kr_mgmt_port_conf_get()
/******************************************************************************/
mesa_rc kr_mgmt_port_conf_get(mesa_port_no_t port_no, mesa_port_kr_conf_t *conf)
{
    if (conf == nullptr) {
        return VTSS_APPL_KR_RC_INVALID_PARAMETER;
    }

    if (!kr_mgmt_port_capable(port_no)) {
        return VTSS_APPL_KR_RC_PORT_NOT_CAPABLE;
    }

    return KR_port_kr_conf_get(port_no, conf);
}

/******************************************************************************/
// kr_mgmt_port_conf_set()
/******************************************************************************/
mesa_rc kr_mgmt_port_conf_set(mesa_port_no_t port_no, const mesa_port_kr_conf_t *conf)
{
    mesa_port_kr_conf_t local_conf;
    mesa_port_conf_t    pconf;
    mesa_port_kr_fec_t  fec;
    kr_state_t          *kr_state;
    bool                irq_enable = false;
    mesa_rc             rc;

    // Check the configuration.
    VTSS_RC(kr_mgmt_port_conf_check(port_no, conf));

    kr_state = &KR_states[port_no];

    KR_LOCK_SCOPE();

    // If going from disable to disabled, do nothing.
    if (!conf->aneg.enable && !kr_state->conf.aneg.enable) {
        return VTSS_RC_OK;
    }

    // If going from the same configuration to the same configuration, do
    // nothing.
    if (memcmp(conf, &kr_state->conf, sizeof(*conf)) == 0) {
        return VTSS_RC_OK;
    }

    // Start over whether enabling or disabling.
    KR_state_init(kr_state);

    if (!conf->aneg.enable) {
        // Disabling ourselves. Gotta clear all fields in the configuration
        // for a subsequent forced speed to work (I think it mainly is the
        // train.enable flag that needs to be cleared).
        vtss_clear(local_conf);
        VTSS_RC(KR_port_kr_conf_set(port_no, &local_conf));

        vtss_clear(fec);
        VTSS_RC(KR_port_kr_fec_set(port_no, &fec));
    }

    // Before leaving control to caller (expected to be port_instance.cxx),
    // force another speed onto the port, so that we are sure that serdes etc.
    // get a refresh when returning - if required.
    if (KR_cap_v3) {
        // Whether we disable or enable KR, force a new port configuration.
        // In order to ensure this, we first set the port configuration to 5G,
        // and then to the required speed.
        VTSS_RC(KR_port_conf_get(port_no, &pconf));
        // Force a port/serdes update
        pconf.if_type = MESA_PORT_INTERFACE_SFI;
        pconf.speed = MESA_SPEED_5G;
        VTSS_RC(KR_port_conf_set(port_no, &pconf, __FUNCTION__, __LINE__));

        // Apply the correct speed
        pconf.speed = kr_state->cap_25g ? MESA_SPEED_25G : MESA_SPEED_10G;
        VTSS_RC(KR_port_conf_set(port_no, &pconf, __FUNCTION__, __LINE__));
    }

    if (KR_cap_v3) {
#if KR_IRQ_BASED
        irq_enable = conf->aneg.enable;
#endif

        // Enable or disable IRQs
        T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_event_enable(irq_enabled = %d)", irq_enable);
        if ((rc = mesa_port_kr_event_enable(NULL, port_no, irq_enable)) != VTSS_RC_OK) {
            T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_event_enable(%d) failed: %s", irq_enable, error_txt(rc));
            return rc;
        }
    }
    if (conf->aneg.enable) {
        VTSS_RC(KR_port_kr_conf_set(port_no, conf));

        if (!conf->train.pcs_flap && KR_states[port_no].pcs_flap) {
            kr_debug_pcs_flap_set(port_no, KR_states[port_no].pcs_flap);
        }

        if (!irq_enable) {
            // Either we are running KR v2 or polled, so time to kick-start the
            // thread (if not already started).
            vtss_flag_setbits(&KR_flag, KR_FLAG_KICK_THREAD);
        }
    }

    kr_state->conf = *conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// kr_mgmt_port_status_get()
/******************************************************************************/
mesa_rc kr_mgmt_port_status_get(mesa_port_no_t port_no, kr_mgmt_status_t *status)
{
    mesa_port_kr_eye_dim_t eye;
    kr_appl_train_t        *krt;
    mesa_rc                rc;
    mesa_port_ctle_t       ctle;

    if (status == nullptr) {
        return VTSS_APPL_KR_RC_INVALID_PARAMETER;
    }

    if (!kr_mgmt_port_capable(port_no)) {
        return VTSS_APPL_KR_RC_PORT_NOT_CAPABLE;
    }

    KR_LOCK_SCOPE();

    krt = &KR_states[port_no].tr;
    if ((rc = mesa_port_kr_status_get(NULL, port_no, &status->status)) != VTSS_RC_OK) {
        T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_status_get() failed: %s", error_txt(rc));
        return rc;
    }

    status->time_since_restart = KR_get_time_sec(&krt->time_start_aneg);
    if (KR_cap_v3) {
        VTSS_RC(KR_port_kr_state_get(port_no, &status->state));

        status->time_ld = krt->time_ld;
        status->time_lp = krt->time_lp;

        if ((rc = mesa_port_kr_eye_get(NULL, port_no, &eye)) != VTSS_RC_OK) {
            T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_eye_get() failed: %s", error_txt(rc));

            // Convert to something sensible
            return VTSS_APPL_KR_RC_INTERNAL_ERROR;
        }

        if ((rc = mesa_port_kr_ctle_get(NULL, port_no, &ctle)) != VTSS_RC_OK) {
            T_EG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_eye_get() failed: %s", error_txt(rc));

            // Convert to something sensible
            return VTSS_APPL_KR_RC_INTERNAL_ERROR;
        }

        T_DG_PORT(KR_TRACE_GRP_BASE, port_no, "mesa_port_kr_eye_get() => height = %u", eye.height);
        status->eye_height = eye.height;
        status->ctle = ctle;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// kr_debug_set()
/******************************************************************************/
void kr_debug_set(bool enable)
{
    kr.kr_debug = enable;
}

/******************************************************************************/
// kr_debug_pd_set()
/******************************************************************************/
void kr_debug_pd_set(mesa_port_no_t port_no, bool enable)
{
    if (!kr_mgmt_port_capable(port_no)) {
        return;
    }

    KR_states[port_no].pd = enable;
}

/******************************************************************************/
// kr_debug_pd_set()
/******************************************************************************/
void kr_debug_pcs_flap_set(mesa_port_no_t port_no, bool enable)
{
    if (!kr_mgmt_port_capable(port_no)) {
        return;
    }
    mesa_port_kr_conf_t conf;
    KR_states[port_no].pcs_flap = enable;
    mesa_port_kr_conf_get(NULL, port_no, &conf);
    conf.train.pcs_flap = enable;
    mesa_port_kr_conf_set(NULL, port_no, &conf);
}


/******************************************************************************/
// kr_debug_state_eq_hist_get()
/******************************************************************************/
mesa_rc kr_debug_state_eq_hist_get(mesa_port_no_t port_no, kr_coef_t *coef, uint32_t index)
{
    if (coef == nullptr) {
        return VTSS_APPL_KR_RC_INVALID_PARAMETER;
    }

    if (!kr_mgmt_port_capable(port_no)) {
        return VTSS_APPL_KR_RC_PORT_NOT_CAPABLE;
    }

    if (index >= KR_states[port_no].tr.eq_hist_index) {
        return MESA_RC_INCOMPLETE;
    }

    *coef = KR_states[port_no].tr.eq_hist[index];

    return VTSS_RC_OK;
}

/******************************************************************************/
// kr_debug_state_ber_hist_get()
/******************************************************************************/
mesa_rc kr_debug_state_ber_hist_get(mesa_port_no_t port_no, kr_ber_t *ber, uint32_t index)
{
    if (ber == nullptr) {
        return VTSS_APPL_KR_RC_INVALID_PARAMETER;
    }

    if (!kr_mgmt_port_capable(port_no)) {
        return VTSS_APPL_KR_RC_PORT_NOT_CAPABLE;
    }

    KR_LOCK_SCOPE();

    if (index >= KR_states[port_no].tr.ber_hist_index) {
        return MESA_RC_INCOMPLETE;
    }

    *ber = KR_states[port_no].tr.ber_hist[index];

    return VTSS_RC_OK;
}

/******************************************************************************/
// kr_debug_state_irq_hist_get()
/******************************************************************************/
mesa_rc kr_debug_state_irq_hist_get(mesa_port_no_t port_no, kr_irq_t *irq, uint32_t index)
{
    if (irq == nullptr) {
        return VTSS_APPL_KR_RC_INVALID_PARAMETER;
    }

    if (!kr_mgmt_port_capable(port_no)) {
        return VTSS_APPL_KR_RC_PORT_NOT_CAPABLE;
    }

    KR_LOCK_SCOPE();

    if (index >= KR_states[port_no].tr.irq_hist_index) {
        return MESA_RC_INCOMPLETE;
    }

    *irq = KR_states[port_no].tr.irq_hist[index];

    return VTSS_RC_OK;
}

/******************************************************************************/
// kr_util_irq_to_txt()
/******************************************************************************/
const char *kr_util_irq_to_txt(uint32_t irq)
{
    switch (irq) {
    case MESA_KR_ACTV:
        return "KR_ACTV";

    case MESA_KR_LPSVALID:
        return "KR_LPS";

    case MESA_KR_LPCVALID:
        return "KR_LPC";

    case MESA_KR_WT_DONE:
        return "WT_DONE";

    case MESA_KR_MW_DONE:
        return "MW_DONE";

    case MESA_KR_BER_BUSY_0:
        return "BER_BUSY_0";

    case MESA_KR_BER_BUSY_1:
        return "BER_BUSY_1";

    case MESA_KR_REM_RDY_0:
        return "REM_RDY_0";

    case MESA_KR_REM_RDY_1:
        return "REM_RDY_1";

    case MESA_KR_FRLOCK_0:
        return "FRLOCK_0";

    case MESA_KR_FRLOCK_1:
        return "FRLOCK_1";

    case MESA_KR_DME_VIOL_0:
        return "DME_VIOL_0";

    case MESA_KR_DME_VIOL_1:
        return "DME_VIOL_1";

    case MESA_KR_AN_XMIT_DISABLE:
        return "AN_XMIT_DISABLE";

    case MESA_KR_TRAIN:
        return "TRAIN";

    case MESA_KR_RATE_DET:
        return "RATE_DET";

    case MESA_KR_CMPL_ACK:
        return "CMPL_ACK";

    case MESA_KR_AN_GOOD:
        return "AN_GOOD";

    case MESA_KR_LINK_FAIL:
        return "LINK_FAIL";

    case MESA_KR_ABD_FAIL:
        return "ABD_FAIL";

    case MESA_KR_ACK_FAIL:
        return "ACK_FAIL";

    case MESA_KR_NP_FAIL:
        return "NP_FAIL";

    case MESA_KR_NP_RX:
        return "NP_RX";

    case MESA_KR_INCP_LINK:
        return "INCP_LINK";

    case MESA_KR_GEN0_DONE:
        return "GEN0_DONE";

    case MESA_KR_GEN1_DONE:
        return "GEN1_DONE";

    case VTSS_BIT(3):
        return "AN_RATE_3";

    case VTSS_BIT(2):
        return "AN_RATE_2";

    case VTSS_BIT(1):
        return "AN_RATE_1";

    case VTSS_BIT(0):
        return "AN_RATE_0";

    case 0:
        return "";

    default:
        return "ILLEGAL";
    }
}
/******************************************************************************/
// kr_util_aneg_sm_to_txt()
/******************************************************************************/
const char *kr_util_aneg_sm_to_txt(uint32_t reg)
{
    switch (reg) {
    case 0:
        return "AN_ENABLE";

    case 1:
        return "XMI_DISABLE";

    case 2:
        return "ABILITY_DET";

    case 3:
        return "ACK_DET";

    case 4:
        return "COMPLETE_ACK";

    case 5:
        return "TRAIN";

    case 6:
        return "AN_GOOD_CHK";

    case 7:
        return "AN_GOOD";

    case 8:
        return "RATE_DET";

    case 11:
        return "LINK_STAT_CHK";

    case 12:
        return "PARLL_DET_FAULT";

    case 13:
        return "WAIT_RATE_DONE";

    case 14:
        return "NXTPG_WAIT";

    default:
        return "?";
    }
}

/******************************************************************************/
// kr_util_irq_mask_to_txt()
// buf must be at least 1000 long
/******************************************************************************/
char *kr_util_irq_mask_to_txt(uint32_t irq_mask, char *buf)
{
    int      i;
    uint32_t irq;
    bool     first = true;

    strcpy(buf, "<");

    for (i = 31; i >= 0; i--) {
        irq = irq_mask & VTSS_BIT(i);

        if (irq == 0) {
            continue;
        }

        if (!first) {
            strcat(buf, " ");
        }

        strcat(buf, kr_util_irq_to_txt(irq));
        first = false;
    }

    strcat(buf, ">");
    return buf;
}

/******************************************************************************/
// kr_util_raw_coef_to_txt()
/******************************************************************************/
void kr_util_raw_coef_to_txt(uint32_t frm_in, char *tap_out, char *action_out)
{
    uint32_t action = 0;

    if (frm_in == 0xdead) {
        sprintf(tap_out, "-       ");
        sprintf(action_out, "-  ");
        return;
    }

    if (VTSS_BIT(13) & frm_in) {
        sprintf(tap_out, "PRESET ");
        sprintf(action_out, "PRESET ");
        return;
    }

    if (VTSS_BIT(12) & frm_in) {
        tap_out += sprintf(tap_out, "INIT ");
        action_out += sprintf(action_out, "INIT ");
        return;
    }

    if ((frm_in & 0x3) > 0) {
        tap_out += sprintf(tap_out, "CM1 ");
        action = frm_in & 0x3;
    }

    if ((frm_in & 0xc) > 0) {
        tap_out += sprintf(tap_out, "C0 ");
        action = (frm_in >> 2) & 3;
    }

    if ((frm_in & 0x30) > 0) {
        tap_out += sprintf(tap_out, "CP1 ");
        action = (frm_in >> 4) & 3;
    }

    if ((frm_in & 0x3f) == 0) {
        tap_out += sprintf(tap_out, "ANY ");
        action = 0;
    }

    if (action == 1) {
        sprintf(action_out, "INCR");
    } else if (action == 2) {
        sprintf(action_out, "DECR");
    } else {
        sprintf(action_out, "HOLD");
    }
}

/******************************************************************************/
// kr_util_aneg_rate_to_txt()
/******************************************************************************/
const char *kr_util_aneg_rate_to_txt(uint32_t reg)
{
    switch (reg) {
    case 0:
        return "No Change";

    case 7:
        return "25G-KR";

    case 8:
        return "25G-KR-S";

    case 9:
        return "10G-KR";

    case 10:
        return "10G-KX4";

    case 11:
        return "5G-KR";

    case 12:
        return "2.5G-KX";

    case 13:
        return "1G-KX";

    default:
        return "other";
    }
}

/******************************************************************************/
// kr_util_spd_to_txt()
/******************************************************************************/
const char *kr_util_spd_to_txt(mesa_port_speed_t speed)
{
    switch (speed) {
    case MESA_SPEED_UNDEFINED:
        return "Undefined";

    case MESA_SPEED_10M:
        return "10M";

    case MESA_SPEED_100M:
        return "100M";

    case MESA_SPEED_1G:
        return "1G";

    case MESA_SPEED_2500M:
        return "2G5";

    case MESA_SPEED_5G:
        return "5G";

    case MESA_SPEED_10G:
        return "10G";

    case MESA_SPEED_12G:
        return "12G";

    case MESA_SPEED_25G:
        return "25G";

    case MESA_SPEED_AUTO:
        return "Auto";

    default:
        return "?";
    }
}

/******************************************************************************/
// kr_util_tap_to_txt()
/******************************************************************************/
const char *kr_util_tap_to_txt(mesa_kr_tap_t st)
{
    switch (st) {
    case MESA_TAP_CM1:
        return "CM1";

    case MESA_TAP_CP1:
        return "CP1";

    case MESA_TAP_C0:
        return "C0";

    default:
        return "ILLEGAL";
    }
}

/******************************************************************************/
// kr_util_state_to_txt()
/******************************************************************************/
const char *kr_util_state_to_txt(mesa_train_state_t st)
{
    switch (st) {
    case MESA_TR_INITILIZE:
        return "INITIALIZE";

    case MESA_TR_SEND_TRAINING:
        return "SEND_TRAINING";

    case MESA_TR_TRAIN_LOCAL:
        return "TRAIN_LOCAL";

    case MESA_TR_TRAIN_REMOTE:
        return "TRAIN_REMOTE";

    case MESA_TR_SEND_DATA:
        return "SEND_DATA";

    case MESA_TR_TRAINING_FAILURE:
        return "TRAINING_FAILURE";

    case MESA_TR_LINK_READY:
        return "LINK_READY";

    default:
        return "ILLEGAL";
    }
}

/******************************************************************************/
// kr_util_ber_to_txt()
/******************************************************************************/
const char *kr_util_ber_to_txt(mesa_ber_stage_t st)
{
    switch (st) {
    case MESA_BER_GO_TO_MIN:
        return "GO_TO_MIN";

    case MESA_BER_CALCULATE_BER:
        return "CALCULATE_BER";

    case MESA_BER_MOVE_TO_MID_MARK:
        return "MOVE_TO_MID_MARK";

    case MESA_BER_LOCAL_RX_TRAINED:
        return "LOCAL_RX_TRAINED";

    default:
        return "ILLEGAL";
    }
}

/******************************************************************************/
// kr_util_raw_status_to_txt()
/******************************************************************************/
void kr_util_raw_status_to_txt(uint32_t frm_in, char *tap_out, char *action_out)
{
    uint32_t action = 0;

    if (frm_in == 0xdead) {
        sprintf(tap_out, "-  ");
        sprintf(action_out, "-  ");
        return;
    }

    if (VTSS_BIT(15) & frm_in) {
        sprintf(tap_out, "RX READY ");
    }

    if ((frm_in & 0x3) > 0) {
        tap_out += sprintf(tap_out, "CM1 ");
        action = frm_in & 0x3;
    }

    if ((frm_in & 0xc) > 0) {
        tap_out += sprintf(tap_out, "C0 ");
        action = (frm_in >> 2) & 3;
    }

    if ((frm_in & 0x30) > 0) {
        tap_out += sprintf(tap_out, "CP1 ");
        action = (frm_in >> 4) & 3;
    }

    if ((frm_in & 0x3f) == 0) {
        tap_out += sprintf(tap_out, "ANY ");
        action = 0;
    }

    if (action == 0) {
        sprintf(action_out, "NOT_UPDATED");
    } else if (action == 1) {
        sprintf(action_out, "UPDATED");
    } else if (action == 2) {
        sprintf(action_out, "MIN");
    } else if (action == 3) {
        sprintf(action_out, "MAX");
    }
}

/******************************************************************************/
// kr_error_txt()
/******************************************************************************/
const char *kr_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_KR_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_KR_RC_INTERNAL_ERROR:
        return "Internal Error: A code-update is required. See console or crashlog for details";

    case VTSS_APPL_KR_RC_PORT_NOT_CAPABLE:
        return "Selected port does not have clause 73 capabilities";

    case VTSS_APPL_KR_RC_ADV_25G:
        return "Unable to advertise 25G on this port or platform";

    case VTSS_APPL_KR_RC_ADV_10G:
        return "Unable to advertise 10G on this port";

    case VTSS_APPL_KR_RC_ADV_5G:
        return "Unable to advertise 5G on this port or platform";

    case VTSS_APPL_KR_RC_ADV_2G5:
        return "Unable to advertise 2.5G on this port or platform";

    case VTSS_APPL_KR_RC_ADV_1G:
        return "Unable to advertise 1G on this port or platform";

    case VTSS_APPL_KR_RC_ADV_NONE_SELECTED:
        return "No speeds are selected for advertisement";

    case VTSS_APPL_KR_RC_R_FEC:
        return "When R-FEC is requested, either 5G, 10G, or 25G must be advertised";

    case VTSS_APPL_KR_RC_RS_FEC:
        return "When RS-FEC is requested, 25G must be advertised";

    case VTSS_APPL_KR_RC_TRAIN_NO_REMOTE:
        return "Training's \"no-remote\" is not supported on this platform";

    case VTSS_APPL_KR_RC_TRAIN_BER_CNT:
        return "Training's \"Use BER count\", is not supported on this platform";

    case VTSS_APPL_KR_RC_TRAIN_TEST_MODE:
        return "Training's \"Test Mode\", is not supported on this platform";

    case VTSS_APPL_KR_RC_TRAIN_TEST_REPEAT:
        return "Training's \"Test Repeat\", is not supported on this platform";

    default:
        T_E("Unknown error code (%u)", rc);
        return "KR: Unknown error code";
    }
}

extern "C" int kr_icli_cmd_register();

/******************************************************************************/
// kr_init()
/******************************************************************************/
mesa_rc kr_init(vtss_init_data_t *data)
{
    bool           create_thread;
    mesa_port_no_t port_no;
    mesa_rc        rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        KR_cap_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);
        critd_init(&KR_crit, "KR", VTSS_MODULE_ID_KR, CRITD_TYPE_MUTEX);
        vtss_flag_init(&KR_flag);
        break;

    case INIT_CMD_START:
        if (fast_cap(MESA_CAP_PORT_KR_IRQ)) {
            KR_cap_v3 = true;
        } else if (fast_cap(MESA_CAP_PORT_KR)) {
            KR_cap_v2 = true;
        }

        if (!KR_cap_v2 && !KR_cap_v3) {
            // Nothing more to do, because we don't support KR on this platform.
            break;
        }

        for (port_no = 0; port_no < KR_cap_port_cnt; port_no++) {
            kr_state_t *kr_state = &KR_states[port_no];

            // Let it be able to identify itself.
            kr_state->port_no = port_no;

            if ((rc = port_cap_get(port_no, &kr_state->port_cap)) != VTSS_RC_OK) {
                T_E_PORT(port_no, "port_cap_get() failed: %s", error_txt(rc));
            }

            kr_state->cap_10g = (kr_state->port_cap & MEBA_PORT_CAP_10G_FDX) != 0 && !port_is_10g_copper_phy(port_no);
            kr_state->cap_25g = (kr_state->port_cap & MEBA_PORT_CAP_25G_FDX) != 0;

            KR_state_init(kr_state);
            kr_state->pd = true;
        }

        kr_icli_cmd_register();

        if (KR_cap_v2) {
            // Here, we always need to create the thread, because it makes
            // status calls into the API that keeps the statemachine alive.
            create_thread = true;
        } else {
            // For KR_cap_v3, we don't create the thread if we are running IRQ
            // based.
#if KR_IRQ_BASED
            create_thread = false;
#else
            create_thread = true;
#endif
        }

        if (create_thread) {
            vtss_thread_create(VTSS_THREAD_PRIO_HIGHER,
                               KR_thread,
                               0,
                               "KR",
                               nullptr,
                               0,
                               &KR_thread_handle,
                               &KR_thread_block);
        }

#if KR_IRQ_BASED
        if (KR_cap_v3) {
            // Subscribe to KR interrupts
            if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_KR, KR_interrupt_callback, MEBA_EVENT_KR, INTERRUPT_PRIORITY_HIGHEST)) != VTSS_RC_OK) {
                T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
            }
        }
#endif

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}
