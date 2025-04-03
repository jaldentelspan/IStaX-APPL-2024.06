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

#ifndef _LED_API_H_
#define _LED_API_H_

#include <main.h>
#include <vtss/basics/enum_macros.hxx>
#include "topo_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Base colors */
typedef enum {
  lc_NULL  = MEBA_LED_COLOR_COUNT,
  lc_OFF   = MEBA_LED_COLOR_OFF,
  lc_GREEN = MEBA_LED_COLOR_GREEN,
  lc_RED   = MEBA_LED_COLOR_RED,
  lc_YELLOW = MEBA_LED_COLOR_YELLOW,
} LED_led_colors_t;

/****************************************************************************/
// The led_front_led_state() function takes an argument that tells this
// module how to exercise the front LED. This argument is one of the
// following.
// NOTE 1: If you sometime in the future need to add another state, make sure
// you also add a state configuration, which is set in the SL_led_state_cfg[]
// array within led.c.
// NOTE 2: Future states MUST be added after LED_FRONT_LED_NORMAL and before
// LED_FRONT_LED_ERROR due to an implicit severity/stickyness.
/****************************************************************************/
typedef enum {
  LED_FRONT_LED_NORMAL = 0,
  // New future states MUST be added after LED_FRONT_LED_NORMAL and notice the precedence

#if defined(VTSS_SW_OPTION_PSU)
    LED_FRONT_LED_MAIN_PSU,
    LED_FRONT_LED_REDUNDANT_PSU,
#endif /* VTSS_SW_OPTION_PSU */

  LED_FRONT_LED_ZTP_DOWNLOAD,
  LED_FRONT_LED_ZTP_CONF,
  LED_FRONT_LED_ZTP_ERROR,  // Downloading or configuring process is failed during ZTP(Zero Touch Provisioning).

  LED_FRONT_LED_FLASHING_BOARD,

  LED_FRONT_LED_STACK_FW_CHK_ERROR, // Firmware version checking error while stack neighbor has incompatible firmware version.

  LED_FRONT_LED_POST_ERROR, // Any one of self-test item is failed.

  // New future states MUST be added before LED_FRONT_LED_ERROR and notice the precedence
  // Total count *MUST* less than 31 (because we use bit field manipulation for the permanent states)
  LED_FRONT_LED_ERROR,  // Generic software error. T_E() or S_E() event is triggered.
  LED_FRONT_LED_FATAL   // Fatal error, e.g. system assertion.
} led_front_led_state_t;

/*
 * Overridable (board) functions
 */
void LED_led_init(void);
void LED_front_led_set(LED_led_colors_t color);
void LED_led_test(void);

/****************************************************************************/
// led_front_led_state()
// Change the state of the front LED on boards that support this function.
// Note that some of the states are sticky in the sense that you cannot
// return to a state with a smaller significance, when once entered. E.g.
// you cannot change from LED_FRONT_LED_FATAL to LED_FRONT_LED_NORMAL (unless
// the force flag is set).
/****************************************************************************/
void led_front_led_state(led_front_led_state_t state, BOOL force);

/****************************************************************************/
// led_front_led_state_clear()
// Change the state of the front LED. It is used to back current LED state to 
// previous state. For example, if current LED state is software error, when
// the error condition is removed (e.g. syslog is cleared), then the system
// LED may be back to green (if no other module indicates an error).
/****************************************************************************/
void led_front_led_state_clear(led_front_led_state_t state);

/****************************************************************************/
// led_front_led_state_clear_all()
// Clear all error state of the front LED and back to normal indication.
/****************************************************************************/
void led_front_led_state_clear_all(void);

/****************************************************************************/
// Get latest front LED state.
/****************************************************************************/
void led_front_led_state_get(led_front_led_state_t *state);

/****************************************************************************/
// led_init()
// Module initialization function.
/****************************************************************************/
mesa_rc led_init(vtss_init_data_t *data);

/****************************************************************************/
// led_front_led_in_error_state()
// Function for checking if the front LED is indicating error/fatal.
/****************************************************************************/
BOOL led_front_led_in_error_state(void);

/****************************************************************************/
// led_front_led_state_txt()
// Get the front LED state text.
/****************************************************************************/
const char *led_front_led_state_txt(led_front_led_state_t state);

/****************************************************************************/
// Debug functions
/****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _LED_API_H_ */

