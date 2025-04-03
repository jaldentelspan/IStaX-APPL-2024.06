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

#include "microchip/ethernet/switch/api.h"
#include "main.h"
#include "led_api.h"
#include "led.h"
#include "port_api.h"  /* For port_phy_wait_until_ready() */
#include "msg_api.h"   /* For msg_wait()                  */
#include "interrupt_api.h"
#if defined(VTSS_SW_OPTION_PSU)
#include "sysutil_api.h"
#endif /* VTSS_SW_OPTION_PSU */
#include "critd_api.h"

// ===========================================================================
// Bit manipulation macros
// ---------------------------------------------------------------------------
#define LED_BIT_SET(var, bit)     ((var) |= (1 << (bit)))
#define LED_BIT_CLEAR(var, bit)   ((var) &= ~(1 << (bit)))
#define LED_BIT_TEST(var, bit)    ((var) & (1 << (bit)))

#define LED_THREAD_FLAG_PUSH_BUTTON_INTERRUPT 0x01
#define LED_THREAD_FLAG_ANY                   0xFFFFFFFF  /* Any possible bit */
// ===========================================================================
// Trace
// ---------------------------------------------------------------------------

static vtss_trace_reg_t trace_reg =
{
    VTSS_TRACE_MODULE_ID, "led", "LED module"
};

static vtss_trace_grp_t trace_grps[] =
{
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define LED_CRIT_ENTER(crit) critd_enter(crit, __FILE__, __LINE__)
#define LED_CRIT_EXIT(crit)  critd_exit( crit, __FILE__, __LINE__)

/****************************************************************************/
/*                                                                          */
/*  NAMING CONVENTION FOR INTERNAL FUNCTIONS:                               */
/*    LED_<function_name>                                                   */
/*                                                                          */
/*  NAMING CONVENTION FOR EXTERNAL (API) FUNCTIONS:                         */
/*    led_<function_name>                                                   */
/*                                                                          */
/****************************************************************************/

// LED thread variables
static vtss_handle_t         LED_thread_handle;
static vtss_thread_t         LED_thread_state;  // Contains space for the scheduler to hold the current thread state.
static led_front_led_state_t LED_next_front_led_state;
static critd_t               LED_front_led_crit;
static critd_t               LED_int_crit;
static u32                   LED_permanent_state = 0;

/****************************************************************************/
// The .colors member must end with an lc_NULL, since that tells the number
// of sub-states that the LED undergo before starting all over.
// The .timeout member must be one shorter than the .colors. It contains
// the number of milliseconds (>= 10) to wait when going from one sub-state to
// the next.
// For instance: In the LED_FRONT_LED_NORMAL state the first color displayed
// is lc_GREEN, then it waits 500 ms (first entry in .timeout), then it
// turns off the led (lc_OFF) and waits another 500 ms (second entry in
// .timeout). Now, since the next entry is lc_NULL, the state machine goes back
// and turns the LED green again.
/****************************************************************************/
static LED_front_led_state_cfg_t LED_front_led_state_cfg[] = {
  [LED_FRONT_LED_NORMAL] = {
    .colors           = {lc_GREEN, lc_NULL},
    .timeout_ms       = {200},
    .least_next_state = LED_FRONT_LED_NORMAL,
    .permanent_state  = TRUE
  },

#if defined(VTSS_SW_OPTION_PSU)
  [LED_FRONT_LED_MAIN_PSU] = {
    .colors           = {lc_GREEN, lc_NULL},
    .timeout_ms       = {2000},
    .least_next_state = LED_FRONT_LED_NORMAL,
    .permanent_state  = TRUE
  },
  [LED_FRONT_LED_REDUNDANT_PSU] = {
    .colors           = {lc_YELLOW, lc_NULL},
    .timeout_ms       = {2000},
    .least_next_state = LED_FRONT_LED_NORMAL,
    .permanent_state  = TRUE
  },
#endif /* VTSS_SW_OPTION_PSU */

  [LED_FRONT_LED_ZTP_DOWNLOAD] = {
    .colors           = {lc_GREEN, lc_OFF, lc_NULL},
    .timeout_ms       = {100, 100},
    .least_next_state = LED_FRONT_LED_NORMAL,
    .permanent_state  = FALSE
  },
  [LED_FRONT_LED_ZTP_CONF] = {
    .colors           = {lc_YELLOW, lc_OFF, lc_NULL},
    .timeout_ms       = {100, 100},
    .least_next_state = LED_FRONT_LED_NORMAL,
    .permanent_state  = FALSE
  },
  [LED_FRONT_LED_ZTP_ERROR] = {
    .colors           = {lc_RED, lc_OFF, lc_NULL},
    .timeout_ms       = {100, 100},
    .least_next_state = LED_FRONT_LED_ZTP_ERROR,
    .permanent_state  = TRUE
  },

  [LED_FRONT_LED_FLASHING_BOARD] = {
    .colors           = {lc_GREEN, lc_OFF, lc_NULL},
    .timeout_ms       = {100, 100},
    .least_next_state = LED_FRONT_LED_NORMAL,
    .permanent_state  = FALSE
  },

  [LED_FRONT_LED_STACK_FW_CHK_ERROR] = {
    .colors           = {lc_GREEN , lc_RED, lc_NULL},
    .timeout_ms       = {100, 100},
    .least_next_state = LED_FRONT_LED_STACK_FW_CHK_ERROR,
    .permanent_state  = TRUE
  },

  [LED_FRONT_LED_POST_ERROR] = {
    .colors           = {lc_RED, lc_NULL},
    .timeout_ms       = {2000},
    .least_next_state = LED_FRONT_LED_POST_ERROR,
    .permanent_state  = TRUE
  },

  [LED_FRONT_LED_ERROR] = {
    .colors           = {lc_RED, lc_OFF, lc_NULL},
    .timeout_ms       = {500, 500},
    .least_next_state = LED_FRONT_LED_ERROR, // Cannot go below LED_FRONT_LED_ERROR when first set.
    .permanent_state  = TRUE
  },

  [LED_FRONT_LED_FATAL] = {
    .colors           = {lc_RED, lc_NULL},   // The FATAL state should only contain one single color, since it may be that we never enter the LED_thread(), which is used to toggle colors, again.
    .timeout_ms       = {2000},              // Even when only one color is displayed, we must be able to wake up the thread
    .least_next_state = LED_FRONT_LED_FATAL, // Cannot go below LED_FRONT_LED_FATAL when first set.
    .permanent_state  = TRUE
  }
};

/****************************************************************************/
/*                                                                          */
/*  MODULE INTERNAL FUNCTIONS                                               */
/*                                                                          */
/****************************************************************************/

/* The MEBA implementation */
void LED_front_led_set(LED_led_colors_t color)
{
  static LED_led_colors_t cur_color;
  LED_CRIT_ENTER(&LED_int_crit);
  if (cur_color != color) {
    meba_status_led_set(board_instance, MEBA_LED_TYPE_FRONT, (meba_led_color_t) color);
    cur_color = color;
  }
  LED_CRIT_EXIT(&LED_int_crit);
}

/****************************************************************************/
/****************************************************************************/
static void LED_front_led_state_cfg_sanity_check(LED_front_led_state_cfg_t *cfg)
{
  int i;

  BOOL null_color_seen = FALSE;
  // The first color cannot be NULL, since that's the "color" used to designate the last
  // color before starting over.
  VTSS_ASSERT(cfg->colors[0] != lc_NULL);

  for (i = 1; i <= LED_FRONT_LED_MAX_SUBSTATES; i++) {
    // The timeout between two consecutive sub-states must
    // be >= 10 ms and less than or equal to, say, 2 seconds.
    VTSS_ASSERT(cfg->timeout_ms[i-1] >= 10 && cfg->timeout_ms[i-1] <= 2000);
    if(cfg->colors[i]==lc_NULL) {
      null_color_seen = TRUE;
      break;
    }
  }

  // The array must have been terminated with an lc_NULL before the end of the sub-states
  VTSS_ASSERT(null_color_seen);
}

/****************************************************************************/
/****************************************************************************/
static vtss_flag_t interrupt_flag;
static BOOL LED_do_update_tower = TRUE;
// Function used if push button is interrupt driven.
// In : None of the inputs are used at the moment,
static void LED_push_button_interrupt(meba_event_t     source_id,
                                      u32                         instance_id)
{
  BOOL update = TRUE;

  T_I("LED_push_button_interrupt");
  vtss_flag_setbits(&interrupt_flag, LED_THREAD_FLAG_PUSH_BUTTON_INTERRUPT); // Signal to thread to update the tower LEDs if needed

  T_D("%sUPDATE TOWER LED", update ? "" : "DO NOT ");

  LED_do_update_tower = update;
}

static void LED_check_for_tower_update(uint32_t cur_mode) {
  T_N("LED_do_update_tower:%d", LED_do_update_tower);
  if (LED_do_update_tower) {
      meba_led_mode_set(board_instance, cur_mode);

      LED_do_update_tower = FALSE;

      // Wait 200 mSec between each interrupt in order to avoid that short "pushes" are detected as multiple "pushes"
      VTSS_OS_MSLEEP(200);

  }

  // Hook up for new interrupts
  T_D("Hooking up");
  (void) vtss_interrupt_source_hook_set(VTSS_MODULE_ID_LED,
                                        LED_push_button_interrupt,
                                        MEBA_EVENT_PUSH_BUTTON,
                                        INTERRUPT_PRIORITY_NORMAL);
}


static void LED_thread(vtss_addrword_t data)
{
  led_front_led_state_t cur_led_state = (led_front_led_state_t)-1;
  int                   substate_idx = 0;
  BOOL                  initial;
  vtss_tick_count_t     time_tick;
  vtss_flag_value_t     wait_flag;
  uint32_t              cur_mode = 0, n_modes = fast_cap(MEBA_CAP_LED_MODES);

  msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_LED);

  vtss_flag_init(&interrupt_flag);

  // This will block this thread from running further until the PHYs are initialized.
  port_phy_wait_until_ready();

  meba_reset(board_instance, MEBA_STATUS_LED_INITIALIZE);

  /* Initialize mode and hook IRQ */
  LED_check_for_tower_update(cur_mode);

  initial = TRUE;
  while (1) {
    LED_CRIT_ENTER(&LED_front_led_crit);
    if(initial || cur_led_state != LED_next_front_led_state) {
      cur_led_state = LED_next_front_led_state;
      substate_idx=0;
      initial = FALSE;
    }
    LED_CRIT_EXIT(&LED_front_led_crit);

    LED_front_led_set(LED_front_led_state_cfg[cur_led_state].colors[substate_idx]);

    time_tick = vtss_current_time() + VTSS_OS_MSEC2TICK(LED_front_led_state_cfg[cur_led_state].timeout_ms[substate_idx]);
    wait_flag = vtss_flag_timed_wait(&interrupt_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR, time_tick);
    T_N("wait_flag is 0x%04x", wait_flag);
    if (wait_flag & LED_THREAD_FLAG_PUSH_BUTTON_INTERRUPT) {
        T_I("LED_check_for_tower_update");
        if (n_modes > 1) {
          cur_mode = (cur_mode + 1) % n_modes;
          LED_check_for_tower_update(cur_mode);
        }
    }

    if (!LED_do_update_tower)
      if (LED_front_led_state_cfg[cur_led_state].colors[++substate_idx] == lc_NULL)
        substate_idx = 0;
  }
}

/****************************************************************************/
/*                                                                          */
/*  MODULE EXTERNAL FUNCTIONS                                               */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
// Function for checking if the front LED is indicating error/fatal.
// Return : TRUE if LED is in error/fatal state (LED flashing red) else FALSE
/****************************************************************************/
BOOL led_front_led_in_error_state(void) {
  T_N("LED_next_front_led_state:%d", LED_next_front_led_state);
  return  (LED_next_front_led_state == LED_FRONT_LED_ERROR || LED_next_front_led_state == LED_FRONT_LED_FATAL);
}

/****************************************************************************/
// led_front_led_state_txt()
// Get the front LED state text.
/****************************************************************************/

const char *led_front_led_state_txt(led_front_led_state_t state)
{
    switch (state) {
    case LED_FRONT_LED_NORMAL:
        return "System LED: green, solid, normal indication.";
    case LED_FRONT_LED_FLASHING_BOARD:
        return "System LED: green, blinking, firmware flashing.";

    case LED_FRONT_LED_STACK_FW_CHK_ERROR:
        return "System LED: green/red, blinking, stack neighbor has incompatible FW version.";

    case LED_FRONT_LED_ZTP_DOWNLOAD:
        return "System LED: green, blinking, ZTP downloading.";
    case LED_FRONT_LED_ZTP_CONF:
        return "System LED: amber, blinking, ZTP configuring.";
    case LED_FRONT_LED_ZTP_ERROR:
        return "System LED: red, blinking, ZTP failed.";

#if defined(VTSS_SW_OPTION_PSU)
    case LED_FRONT_LED_MAIN_PSU:
        return "System LED: green, solid, normal indication.";
    case LED_FRONT_LED_REDUNDANT_PSU:
        return "System LED: amber, solid, redundant power supply.";
#endif /* VTSS_SW_OPTION_PSU */

    case LED_FRONT_LED_POST_ERROR:
        return "System LED: red, solid, POST error occurred.";

    case LED_FRONT_LED_ERROR:
        return "System LED: red, blinking, a software error occurred.";

    case LED_FRONT_LED_FATAL:
        return "System LED: red, solid, a fatal error occurred.";

    default:
        return "System LED: unknown state.";
    }
}

/****************************************************************************/
// led_front_led_state()
/****************************************************************************/
void led_front_led_state(led_front_led_state_t state, BOOL force)
{
  // Guard against someone having added a new state to led_front_led_state_t
  // without having added a configuration to the LED_front_led_state_cfg[] array.
  VTSS_ASSERT(state < sizeof(LED_front_led_state_cfg) / sizeof(LED_front_led_state_cfg[0]));

  if (LED_front_led_state_cfg[state].permanent_state) {
#if defined(VTSS_SW_OPTION_PSU)
    if (state == LED_FRONT_LED_MAIN_PSU || state == LED_FRONT_LED_REDUNDANT_PSU) {
      // Clear all power states first
      LED_BIT_CLEAR(LED_permanent_state, LED_FRONT_LED_REDUNDANT_PSU);
      LED_BIT_CLEAR(LED_permanent_state, LED_FRONT_LED_MAIN_PSU);
    }
#endif /* VTSS_SW_OPTION_PSU */
    LED_BIT_SET(LED_permanent_state, state);
  }

  LED_CRIT_ENTER(&LED_front_led_crit);

#if defined(VTSS_SW_OPTION_PSU)
  // Keep the original power LED state
  if (state == LED_FRONT_LED_NORMAL &&
      (LED_next_front_led_state == LED_FRONT_LED_MAIN_PSU || LED_next_front_led_state == LED_FRONT_LED_REDUNDANT_PSU)) {
    goto exit_func;
  }
#endif /* VTSS_SW_OPTION_PSU */

  // Don't allow a smaller new state than the current state's least_next_state member.
  if (
#if defined(VTSS_SW_OPTION_PSU)
      // Specific case: allow state changed between LED_FRONT_LED_MAIN_PSU <--> LED_FRONT_LED_REDUNDANT_PSU
      (!((state == LED_FRONT_LED_MAIN_PSU && LED_next_front_led_state == LED_FRONT_LED_REDUNDANT_PSU) ||
      (state == LED_FRONT_LED_REDUNDANT_PSU && LED_next_front_led_state == LED_FRONT_LED_MAIN_PSU))) &&
#endif /* VTSS_SW_OPTION_PSU */
      !force && state < LED_front_led_state_cfg[LED_next_front_led_state].least_next_state) {
    goto exit_func;
  }

  LED_next_front_led_state = state;

  // In case of a fatal, we may end up with not entering the LED_thread()
  // again, so we better force the LED to the color of fatality
  // right away
  if (state == LED_FRONT_LED_FATAL) {
    LED_front_led_set(LED_front_led_state_cfg[LED_FRONT_LED_FATAL].colors[0]);
  }

exit_func:
  LED_CRIT_EXIT(&LED_front_led_crit);
}

/****************************************************************************/
// led_front_led_state_clear()
/****************************************************************************/
void led_front_led_state_clear(led_front_led_state_t state)
{
    led_front_led_state_t new_state = LED_next_front_led_state;

    // Guard against someone having added a new state to led_front_led_state_t
    // without having added a configuration to the LED_front_led_state_cfg[] array.
    VTSS_ASSERT(state < sizeof(LED_front_led_state_cfg) / sizeof(LED_front_led_state_cfg[0]));

    if (state == LED_FRONT_LED_NORMAL
#if defined(VTSS_SW_OPTION_PSU)
        || state == LED_FRONT_LED_MAIN_PSU
        || state == LED_FRONT_LED_REDUNDANT_PSU
#endif /* VTSS_SW_OPTION_PSU */
       ) {
        // Ignore normal indication
        return;
    }

    LED_BIT_CLEAR(LED_permanent_state, state);
    if (new_state == state) {
        // Back to previous permanent state
        int bit;
        for (bit = (int) LED_FRONT_LED_FATAL; bit >= 0; bit--) {
            if (LED_BIT_TEST(LED_permanent_state, bit)) {
              new_state = (led_front_led_state_t) bit;
              break;
            }
        }
    }

    // Update new LED state
    if (new_state != LED_next_front_led_state) {
        led_front_led_state(new_state, TRUE);
    }
}

/****************************************************************************/
// led_front_led_state_clear_all()
/****************************************************************************/
void led_front_led_state_clear_all(void)
{
    led_front_led_state_t new_state = LED_next_front_led_state;

    // Back to power LED state
#if defined(VTSS_SW_OPTION_PSU)
    if (LED_BIT_TEST(LED_permanent_state, LED_FRONT_LED_REDUNDANT_PSU)) {
        new_state = LED_FRONT_LED_REDUNDANT_PSU;
    } else {
        new_state = LED_FRONT_LED_MAIN_PSU;
    }
    LED_permanent_state = 0;
    LED_BIT_SET(LED_permanent_state, LED_FRONT_LED_NORMAL);
#else
    LED_permanent_state = 0;
    new_state = LED_FRONT_LED_NORMAL;
#endif /* VTSS_SW_OPTION_PSU */
    LED_BIT_SET(LED_permanent_state, new_state);

    if (new_state != LED_next_front_led_state) {
        led_front_led_state(new_state, TRUE);
    }
}

/****************************************************************************/
// Get latest front LED state.
/****************************************************************************/
void led_front_led_state_get(led_front_led_state_t *state)
{
    *state = LED_next_front_led_state;
}

/****************************************************************************/
// Test LEDs
/****************************************************************************/
static void set_port_led(int port, mesa_bool_t on)
{
  mesa_port_status_t status = { 0 } ;
  static const mesa_port_counters_t counters = { 0 };
  meba_port_admin_state_t state = { 0 };
  status.link = on;
  status.link_down = !on;
  status.speed = on ? MESA_SPEED_1G : MESA_SPEED_UNDEFINED;
  status.fdx = on;
  state.enable = on;

  // meba_port_led_update() calls mesa_sgpio_conf_get()/set(), and these two
  // calls must be invoked without interference.
  VTSS_APPL_API_LOCK_SCOPE();
  meba_port_led_update(board_instance, port, &status, &counters, &state);
}

void LED_led_test(void)
{
  int port, max = meba_capability(board_instance, MEBA_CAP_BOARD_PORT_COUNT);
  T_I("LED_test start");
  meba_status_led_set(board_instance, MEBA_LED_TYPE_FRONT, MEBA_LED_COLOR_YELLOW);
  for (port = 0; port < max; port++)
    set_port_led(port, true);
  T_I("LED_test LEDs ON");
  VTSS_MSLEEP(2000);
  meba_status_led_set(board_instance, MEBA_LED_TYPE_FRONT, MEBA_LED_COLOR_OFF);
  for (port = 0; port < max; port++)
    set_port_led(port, false);
  T_I("LED_test LEDs OFF");
  VTSS_MSLEEP(2000);
  meba_status_led_set(board_instance, MEBA_LED_TYPE_FRONT, MEBA_LED_COLOR_GREEN);
  T_I("LED_test done");
}

static void LED_assert_cb(const char *file_name, const unsigned long line_num, const char *msg)
{
    // Enter the front led fatal state
    led_front_led_state(LED_FRONT_LED_FATAL, FALSE);
}

/****************************************************************************/
// led_init()
/****************************************************************************/
mesa_rc led_init(vtss_init_data_t *data)
{
  size_t i;

  switch(data->cmd) {
  case INIT_CMD_EARLY_INIT:
    // The front led state must also be protected by a critical region.
    critd_init(&LED_front_led_crit, "LED_front", VTSS_MODULE_ID_LED, CRITD_TYPE_MUTEX);
    critd_init(&LED_int_crit,       "LED_int",   VTSS_MODULE_ID_LED, CRITD_TYPE_MUTEX);
    break;

  case INIT_CMD_INIT:
    // Sanity check of LED_front_led_state_cfg[] and primary/secondary switch dittos
    for(i = 0; i < sizeof(LED_front_led_state_cfg)/sizeof(LED_front_led_state_cfg[0]); i++) {
      LED_front_led_state_cfg_sanity_check(&LED_front_led_state_cfg[i]);
    }

    LED_next_front_led_state = LED_FRONT_LED_NORMAL;
    LED_BIT_SET(LED_permanent_state, LED_FRONT_LED_NORMAL);

    // Create thread that receives "tx done" messages and callback packet txers in thread context
    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       LED_thread,
                       0,
                       "LED",
                       nullptr,
                       0,
                       &LED_thread_handle,
                       &LED_thread_state);

    // Hook into the VTSS_ASSERT macro.
    vtss_common_assert_cb_set(LED_assert_cb);
    break;

  default:
    break;
  }

  return VTSS_RC_OK;
}

