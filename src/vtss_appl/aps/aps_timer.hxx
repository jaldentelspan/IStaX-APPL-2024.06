/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _APS_TIMER_HXX_
#define _APS_TIMER_HXX_

#include "main_types.h"

struct aps_timer_s;
typedef void (*aps_timer_callback_t)(struct aps_timer_s &timer, void *context);

typedef struct aps_timer_s {
    // Function to call upon timeout
    aps_timer_callback_t callback;

    // User-defined context passed back in the callback
    void *context;

    // A name for this timer (only used in trace output and 'debug aps timers')
    const char *name;

    // Some timers are replicated several times with the same name. This allows
    // for distinguishing them. Set to -1 if not replicated.
    int instance;

    // If true, the timer re-initializes itself upon timeout. If false, it must
    // be restarted by the user.
    bool periodic;

    // Only used if #periodic is true. Determines the period (measured in
    // milliseconds) of the timer.
    uint32_t period_ms;

    // Absolute time (in milliseconds) since boot where we want to time out.
    uint64_t timeout_ms;

    // Number of times the callback has been invoked.
    uint32_t callback_cnt;

    // Number of times we've lost a callback because we can't keep up the pace
    uint32_t callback_loss_cnt;

    // Points to prev active timer. NULL if this timer is not active. If it's
    // the first, it points to a placeholder.
    struct aps_timer_s *prev;

    // Points to next active timer. NULL if last or this timer is not active.
    struct aps_timer_s *next;
} aps_timer_t;

// User operations.
// APS mutex must be taken upon calls to these functions, and it is already
// taken upon callback.

// Initialize a timer with a name, a callback and some user-specified context.
void aps_timer_init(aps_timer_t &t, const char *name, int instance, aps_timer_callback_t callback, void *context);

// Start a timer with a given timeout (in milliseconds) while choosing whether
// to let it repeat upon timeout.
void aps_timer_start(aps_timer_t &t, uint32_t period_ms, bool repeat);

// Returns true if timer is currently active, false otherwise.
bool aps_timer_active(aps_timer_t &t);

// Possibly extend a given timeout.
// The function works as follows:
//   If timer is started and time left until timeout is greater than or equal to
//   requested #timeout_ms, do nothing.
//   Otherwise set the timeout to #timeout_ms.
// The function will fail if the timer is currently set to repeat.
void aps_timer_extend(aps_timer_t &t, uint32_t timeout_ms);

// Stop a timer. May be called whether or not the timer is currently active.
void aps_timer_stop(aps_timer_t &t);

// System operations
void aps_timer_init(void);

#endif /* _APS_TIMER_HXX_ */

