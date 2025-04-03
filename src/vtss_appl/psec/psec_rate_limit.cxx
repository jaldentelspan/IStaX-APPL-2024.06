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

/******************************************************************************/
//
// Rate limiting filter needed to prevent DoS attacks.
//
/******************************************************************************/

#include "main.h"            /* For ARRSZ()                 */
#include "psec_rate_limit.h" /* Ourselves                   */
#include "psec_trace.h"      /* For T_x() trace macros      */
#include "critd_api.h"       /* For mutex wrapper           */
#include <vtss/appl/psec.h>  /* For VTSS_APPL_PSEC_RC_xxx   */
#include "msg.h"             /* For msg_switch_is_primary() */

/******************************************************************************/
// Mutex stuff.
/******************************************************************************/
static critd_t crit_psec_rate_limit;

// Macros for accessing mutex functions
// ------------------------------------
#define PSEC_RATE_LIMIT_CRIT_ENTER()         critd_enter(        &crit_psec_rate_limit, __FILE__, __LINE__)
#define PSEC_RATE_LIMIT_CRIT_EXIT()          critd_exit(         &crit_psec_rate_limit, __FILE__, __LINE__)
#define PSEC_RATE_LIMIT_CRIT_ASSERT_LOCKED() critd_assert_locked(&crit_psec_rate_limit, __FILE__, __LINE__)

// We need to shape the learn frames against the primary switch for the
// following reason: If any secondary switch could send an unlimited amount of
// learn frames to the primary switch, the message queue on the primary switch
// would queue them up, and send them to the DOT1X module one by one. The
// message queue is of unlimited size, so if the DOT1X module can't keep up the
// pace, it would just grow and grow until the DOT1X module finds out that all
// state machines are in use, or it's out of RADIUS identifiers. After this, it
// would send a message to the secondary switch and tell it to stop copying
// learn frames to the CPU. The bad thing at this point is that if the MAC
// module uses the message protocol to tell this, then a real secondary switch
// would get the message quite fast, but if it is sent to the primary switch
// itself, it would be looped back and end up at the back of the message queue,
// and not get processed until all the learn frames that the message module has
// received thus far are processed. Therefore, it is of importance that the MAC
// module doesn't use the message protocol to send such requests to itself, but
// shortcut the message protocol and write it directly - when it's primary
// switch and the message is also for the primary switch.
// The policer is implemented as a leaky bucket with some burst capacity and
// an emptying at a steady pace. Since the DOT1X_thread() is suspended when
// we're a secondary switch and the policer still needs to work, we can't use
// the thread to empty the leaky bucket, so we'll have to do it based on the
// current uptime whenever we're about to judge wether to drop the learn frame
// or forward it to the primary switch. For that we need a current fill level,
// a burst capacity (constant), an emptying rate (constant) and the time of the
// last update of the bucket.
// One could ask: Why not use the learn frame policer, which is already enabled,
// when multi-clients are supported? The reason is that this policer affects
// wire-speed learning, and doesn't protect when a given MAC address has already
// been learned, and it is going to be aged.
static u64               PSEC_rate_limit_fill_level;
static vtss_tick_count_t PSEC_rate_limit_uptime_of_last_update_ms;
static BOOL              PSEC_rate_limit_burst_capacity_reached;

// And then the statistics we keep track of in here
static psec_rate_limit_stat_t PSEC_rate_limit_stat;

// Rate-limiter configuration. This can be changed by a debug command.
static psec_rate_limit_conf_t PSEC_conf;

/******************************************************************************/
// List of last seen MAC addresses on this switch.
/******************************************************************************/
typedef struct {
    vtss_tick_count_t last_transmission_time_ms;
    mesa_vid_mac_t   vid_mac;
    mesa_port_no_t   port;
    BOOL             in_use;
} psec_rate_limit_filter_t;

static psec_rate_limit_filter_t PSEC_rate_limit_filter[PSEC_RATE_LIMIT_FILTER_CNT];

/******************************************************************************/
// PSEC_rate_limit_filter_drop()
/******************************************************************************/
static inline BOOL PSEC_rate_limit_filter_drop(mesa_port_no_t port, mesa_vid_mac_t *vid_mac, vtss_tick_count_t now_ms)
{
    // Scenario: Suppose two clients are attached to the same switch, and one is
    // transmitting a lot of data, while the other (the friendly) is
    // transmitting only a little bit, and suppose we're currently
    // software-aging both clients.
    // Both users' data would end up in the same extraction queue, but if the
    // one sending a lot of data saturates the rate-limiter, the one sending not
    // so much data would not get a chance to get its frames through to the
    // primary switch. Therefore, we need to build a filter on top of the
    // rate-limiter, so that a given user cannot send frames to the primary
    // switch unless it hasn't sent anything the last, say, 3 seconds. We can't
    // have an array that is infinitely long, so we just save the last X MAC
    // addresses.
    psec_rate_limit_filter_t *mf, *temp_mf = &PSEC_rate_limit_filter[0];
    vtss_tick_count_t         time_of_oldest = PSEC_rate_limit_filter[0].last_transmission_time_ms;
    int                      i;

    // In case of a stack-topology change or configuration change, the mac filter gets cleared.
    for (i = 0; i < (int)ARRSZ(PSEC_rate_limit_filter); i++) {
        mf = &PSEC_rate_limit_filter[i];
        if (mf->in_use && memcmp(&mf->vid_mac, vid_mac, sizeof(mf->vid_mac)) == 0 && mf->port == port) {
            // Received this MAC address before. Check to see if we should drop it.
            if (now_ms - mf->last_transmission_time_ms >= PSEC_conf.drop_age_ms) {
                mf->last_transmission_time_ms = now_ms;
                return FALSE; // Don't drop it.
            } else {
                return TRUE; // Drop it. It's not old enough
            }
        }

        if (mf->last_transmission_time_ms < time_of_oldest) {
            temp_mf        = mf;
            time_of_oldest = mf->last_transmission_time_ms;
        }
    }

    // New MAC address. Save it by overwriting the one that has been longest in the array
    temp_mf->in_use                    = TRUE; // Can never get cleared.
    temp_mf->last_transmission_time_ms = now_ms;
    temp_mf->vid_mac                   = *vid_mac;
    temp_mf->port                      = port;
    temp_mf->in_use                    = TRUE;
    return FALSE; // Don't drop it.
}

//******************************************************************************
//
// Public functions
//
//******************************************************************************

/******************************************************************************/
// psec_rate_limit_drop()
// Check to see if we should drop this frame.
/******************************************************************************/
BOOL psec_rate_limit_drop(mesa_port_no_t port, mesa_vid_mac_t *vid_mac)
{
    // This function is not re-entrant, and since it's only called from the Packet Rx
    // thread, it's ensured that it's not called twice.
    vtss_tick_count_t now_ms, diff_ms;
    u64               subtract_from_fill_level;
    BOOL              result;

    PSEC_RATE_LIMIT_CRIT_ENTER();

    // NB:
    // Due to the granularity of a tick, all fill-levels are multiplied
    // by 1024, so that we count up and down more accurately.
    // If we didn't, the fill-level-subtraction would occur in too high steps,
    // or not at all if two frames arrived within the same tick.

    // Update the rate-limiter's current fill level
    // @now_ms is the current uptime measured in milliseconds.
    now_ms = VTSS_OS_TICK2MSEC(vtss_current_time());

    // @diff_ms is the amount of time elapsed since the last call of this
    // function (notice that we don't take into account if the stack topology
    // changes, which is fine, I think).
    diff_ms = now_ms - PSEC_rate_limit_uptime_of_last_update_ms;

    // In reality we should have divided the following by 1000 and multiplied by the
    // fill-level granularity (1024), but to speed things up, we don't do this, which
    // causes the emptying of the leaky bucket to happen 1 - 1000/1024 = 2.3% slower.
    // Stay in the 64-bit world (vtss_tick_count_t) when doing these computations.
    subtract_from_fill_level = (diff_ms * PSEC_conf.rate);

    // Update the rate-limiter's current fill-level
    if (subtract_from_fill_level >= PSEC_rate_limit_fill_level) {
        PSEC_rate_limit_fill_level = 0;
    } else {
        PSEC_rate_limit_fill_level -= (ulong)subtract_from_fill_level;
    }

    PSEC_rate_limit_uptime_of_last_update_ms = now_ms;

    // Hysteresis: If the rate-limiter has hit it's maximum, wait until it's below it's minimum
    // before allowing frames again.
    // The 1024 below is due to the fill-level granularity.
    if ((PSEC_rate_limit_burst_capacity_reached && PSEC_rate_limit_fill_level > (PSEC_conf.fill_level_min * 1024ULL)) || (PSEC_rate_limit_fill_level >= (PSEC_conf.fill_level_max * 1024ULL))) {
        if (PSEC_rate_limit_burst_capacity_reached == FALSE) {
            T_D("Turning on rate-limiter");
        }

        // We could've stopped all traffic from multi-client ports here, but I
        // think it's better to leave that decision to the primary switch, also
        // because we have no way to trigger the port(s) to start again, since
        // we're only called when new frames arrive.
        PSEC_rate_limit_burst_capacity_reached = TRUE;
        PSEC_rate_limit_stat.drop_cnt++;
        result = TRUE;
        goto do_exit;
    }

    if (PSEC_rate_limit_burst_capacity_reached == TRUE) {
        T_D("Turning off rate-limiter");
    }

    // Check to see if we need to filter this one out.
    if (PSEC_rate_limit_filter_drop(port, vid_mac, now_ms)) {
        // We do. Don't count it in the rate-limiter.
        PSEC_rate_limit_stat.filter_drop_cnt++;
        result = TRUE;
        goto do_exit;
    }

    // Fill-level granularity is 1024 per frame.
    PSEC_rate_limit_fill_level             += 1024;
    PSEC_rate_limit_burst_capacity_reached  = FALSE;
    PSEC_rate_limit_stat.forward_cnt++;

    result = FALSE; // Don't drop the frame - forward it to the primary switch.

do_exit:
    PSEC_RATE_LIMIT_CRIT_EXIT();
    return result;
}

/******************************************************************************/
// psec_rate_limit_conf_get()
/******************************************************************************/
mesa_rc psec_rate_limit_conf_get(psec_rate_limit_conf_t *conf)
{
    if (!conf) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    PSEC_RATE_LIMIT_CRIT_ENTER();
    *conf = PSEC_conf;
    PSEC_RATE_LIMIT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_rate_limit_conf_set()
/******************************************************************************/
mesa_rc psec_rate_limit_conf_set(vtss_isid_t isid, psec_rate_limit_conf_t *conf)
{
    if (isid != VTSS_ISID_LOCAL && isid != VTSS_ISID_GLOBAL) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    if (!conf) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    if (conf->fill_level_min >= conf->fill_level_max) {
        return VTSS_APPL_PSEC_RC_INV_RATE_LIMITER_FILL_LEVEL;
    }

    if (conf->rate == 0) {
        return VTSS_APPL_PSEC_RC_INV_RATE_LIMITER_RATE;
    }

    if (isid == VTSS_ISID_GLOBAL) {
        if (msg_switch_is_primary()) {
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                psec_msg_tx_rate_limit_conf(isid, conf);
            }
        }
    } else {
        PSEC_RATE_LIMIT_CRIT_ENTER();
        PSEC_conf = *conf;
        PSEC_RATE_LIMIT_CRIT_EXIT();
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_rate_limit_stat_get()
/******************************************************************************/
mesa_rc psec_rate_limit_stat_get(psec_rate_limit_stat_t *stat, u64 *rate_limit_fill_level)
{
    if (!stat || !rate_limit_fill_level) {
        return VTSS_APPL_PSEC_RC_INV_PARAM;
    }

    PSEC_RATE_LIMIT_CRIT_ENTER();
    *stat                  = PSEC_rate_limit_stat;
    *rate_limit_fill_level = PSEC_rate_limit_fill_level / 1024;
    PSEC_RATE_LIMIT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_rate_limit_stat_clr()
/******************************************************************************/
mesa_rc psec_rate_limit_stat_clr(void)
{
    PSEC_RATE_LIMIT_CRIT_ENTER();
    memset(&PSEC_rate_limit_stat, 0, sizeof(PSEC_rate_limit_stat));
    PSEC_RATE_LIMIT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_rate_limit_filter_clr()
// Set port to fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) to clear the whole filter.
/******************************************************************************/
mesa_rc psec_rate_limit_filter_clr(mesa_port_no_t port)
{
    int i;

    PSEC_RATE_LIMIT_CRIT_ENTER();
    if (port >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        memset(PSEC_rate_limit_filter, 0, sizeof(PSEC_rate_limit_filter));
    } else {
        for (i = 0; i < (int)ARRSZ(PSEC_rate_limit_filter); i++) {
            psec_rate_limit_filter_t *mf = &PSEC_rate_limit_filter[i];
            if (mf->in_use && mf->port == port) {
                mf->in_use = FALSE;
                mf->last_transmission_time_ms = 0;
            }
        }
    }

    PSEC_RATE_LIMIT_CRIT_EXIT();

    return VTSS_RC_OK;
}

/******************************************************************************/
// psec_rate_limit_init()
/******************************************************************************/
void psec_rate_limit_init(void)
{
    PSEC_rate_limit_fill_level               =    0;
    PSEC_rate_limit_uptime_of_last_update_ms =    0;
    PSEC_rate_limit_burst_capacity_reached   = FALSE;
    PSEC_conf.fill_level_max                 =  200;    // Burst capacity. At most this amount of frames in a burst
    PSEC_conf.fill_level_min                 =   50;    // Hysteresis: After the burst capacity has been reached, do not allow new frames until it reaches 50
    PSEC_conf.rate                           =   32;    // At most 32 frames per second over time (better to pick a power-of-two for this number)
    PSEC_conf.drop_age_ms                    = 3000ULL; // 3 seconds per default.

    // Initialize mutex.
    critd_init(&crit_psec_rate_limit, "psec_rate_limit", VTSS_MODULE_ID_PSEC, CRITD_TYPE_MUTEX);

    psec_rate_limit_filter_clr(fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT));
    psec_rate_limit_stat_clr();
}

