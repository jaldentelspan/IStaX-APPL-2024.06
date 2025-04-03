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

#ifndef VTSS_PTP_SYS_TIMER_H
#define VTSS_PTP_SYS_TIMER_H

#include "main_types.h"

struct vtss_ptp_sys_timer_s;
typedef void (*vtss_ptp_sys_timer_callout_t)(struct vtss_ptp_sys_timer_s *timer, void *context);

// The PTP timer system is tick-based. Unless VTSS_MAX_LOG_MESSAGE_RATE is
// defined and set to a value in range [-7; 0], a tick is 7.8125 ms.
typedef struct vtss_ptp_sys_timer_s {
    // Function to call upon timeout
    vtss_ptp_sys_timer_callout_t callout;

    // User-defined context passed back in the callout
    void *context;

    // A name for this timer (only used in trace output and 'debug ptp timers')
    const char *name;

    // Some timers are replicated several times with the same name. This allows
    // for distinguishing them. Set to -1 if not replicated.
    int instance;

    // If true, the timer re-initializes itself upon timeout. If false, it must
    // be restarted by the user.
    bool periodic;

    // Only used if #periodic is true. Determines the period (measured in
    // microseconds) of the timer. We compute everything in microseconds rather
    // than milliseconds, because one tick is not an integral number of
    // milliseconds, so in this way, we avoid drifts.
    // Doing so will cause us to roll over at:
    //  2^64 / (1000000 * 86400 * 365) years = 584942 years, which I think is
    // good enough for this product.
    u32 period_us;

    // Absolute time since boot where we want to time out.
    u64 timeout_us;

    // Number of times the callback has been invoked.
    u32 callback_cnt;

    // Number of times we've lost a callback because we can't keep up the pace
    u32 callback_loss_cnt;

    // Delay unlinking the timer and execute it after callout is called so that
    // list neighbors are clear.
    bool delay_unlink;

    // Points to prev active timer. NULL if this timer is not active. If it's
    // the first, it points to a placeholder.
    struct vtss_ptp_sys_timer_s *prev;

    // Points to next active timer. NULL if last or this timer is not active.
    struct vtss_ptp_sys_timer_s *next;
} vtss_ptp_sys_timer_t;

#ifdef __cplusplus
extern "C" {
#endif

// User operations
void vtss_ptp_timer_init (vtss_ptp_sys_timer_t *t, const char *name, int instance, vtss_ptp_sys_timer_callout_t callout, void *context);
void vtss_ptp_timer_start(vtss_ptp_sys_timer_t *t, u32 period_ticks, bool repeat);
void vtss_ptp_timer_stop (vtss_ptp_sys_timer_t *t);

// System operations
u64  vtss_ptp_timer_tick(u64 now_us);
void vtss_ptp_timer_initialize(void);

// Round x divided by y to nearest higher integer. x and y are integers
#define PTP_DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))

#ifdef __cplusplus
}
#endif

#endif // VTSS_PTP_SYS_TIMER_H

