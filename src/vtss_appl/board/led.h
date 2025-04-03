/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _LED_H_
#define _LED_H_

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_LED

#define VTSS_TRACE_GRP_DEFAULT 0

#include <vtss_trace_api.h>
/* ============== */

#define LED_FRONT_LED_MAX_SUBSTATES 3 /* Defines the maximum number of sub-states one state can undergo before starting over */

/****************************************************************************/
// This defines the configurations of the LED, i.e. the color(s) and
// blinking rate.
/****************************************************************************/
typedef struct {
  LED_led_colors_t      colors[LED_FRONT_LED_MAX_SUBSTATES+1];
  unsigned long         timeout_ms[LED_FRONT_LED_MAX_SUBSTATES];
  led_front_led_state_t least_next_state;

  /* Save this state permanently. It is used to back current LED state to 
     previous state. For example, if current LED state is software error, when
     the error condition is removed (e.g. syslog is cleared), then the system
     LED may be back to green (if no other module indicates an error). */
  BOOL                  permanent_state;
} LED_front_led_state_cfg_t;

#endif /* _LED_H_ */

