/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _IEC_MRP_TIMER_HXX_
#define _IEC_MRP_TIMER_HXX_

#include "main_types.h"

struct mrp_timer_s;
struct mrp_state_s;
typedef void (*mrp_timer_callback_t)(struct mrp_timer_s &timer, struct mrp_state_s *);

typedef struct mrp_timer_s {
    // Function to call upon timeout
    mrp_timer_callback_t callback;

    // User-defined context passed back in the callback
    struct mrp_state_s *context;

    // A name for this timer (only used in trace output and 'debug mrp timers')
    const char *name;

    // Some timers are replicated several times with the same name. This allows
    // for distinguishing them. Set to -1 if not replicated.
    int instance;

    // If true, the timer re-initializes itself upon timeout. If false, it must
    // be restarted by the user.
    bool periodic;

    // Only used if #periodic is true. Determines the period (measured in
    // microseconds) of the timer.
    uint32_t period_us;

    // Absolute time (in microseconds) since boot where we want to time out.
    uint64_t timeout_us;

    // Number of times the callback has been invoked.
    uint32_t callback_cnt;

    // Number of times we've lost a callback because we can't keep up the pace
    uint32_t callback_loss_cnt;

    // Points to prev active timer. NULL if this timer is not active. If it's
    // the first, it points to a placeholder.
    struct mrp_timer_s *prev;

    // Points to next active timer. NULL if last or this timer is not active.
    struct mrp_timer_s *next;
} mrp_timer_t;

// User operations.
// IEC_MRP mutex must be taken upon calls to these functions, and it is already
// taken upon callback.

// Initialize a timer with a name, a callback and some user-specified context.
void mrp_timer_init(mrp_timer_t &t, const char *name, int instance, mrp_timer_callback_t callback, struct mrp_state_s *context);

// Start a timer with a given timeout (in microseconds) while choosing whether
// to let it repeat upon timeout.
void mrp_timer_start(mrp_timer_t &t, uint32_t period_us, bool repeat);

// Returns true if timer is currently active, false otherwise.
bool mrp_timer_active(mrp_timer_t &t);

// Possibly extend a given timeout.
// The function works as follows:
//   If timer is started and time left until timeout is greater than or equal to
//   requested #timeout_us, do nothing.
//   Otherwise set the timeout to #timeout_us.
// The function will fail if the timer is currently set to repeat.
void mrp_timer_extend(mrp_timer_t &t, uint32_t timeout_us);

// Stop a timer. May be called whether or not the timer is currently active.
void mrp_timer_stop(mrp_timer_t &t);

// System operations
void mrp_timer_init(void);

#endif /* _IEC_MRP_TIMER_HXX_ */

