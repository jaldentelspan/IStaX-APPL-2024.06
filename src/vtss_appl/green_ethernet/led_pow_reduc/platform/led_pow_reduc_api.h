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

#ifndef _LED_POW_REDUC_API_H_
#define _LED_POW_REDUC_API_H_
#include "vtss_module_id.h"
#include "critd_api.h"
#include "main.h"
#include "msg_api.h"
#include "vtss/appl/led_power_reduction.h"
#ifdef __cplusplus
extern "C" {
#endif
//************************************************
// Definition of rc errors - See also led_pow_reduc_error_txt in led_pow_reduc.c
//************************************************
enum {
    LED_POW_REDUC_ERROR_NOT_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_LED_POW_REDUC),
};
const char *led_pow_reduc_error_txt(mesa_rc rc);


// Struct used for looping through all timer intervals
typedef struct {
    u8 start_index; // Start of interval
    u8 end_index;   // End of interval
    u8 first_index; // Very first index containing a change in intensity change.
    BOOL start_new_search; // Used to signal that a new loop through is started.
} led_pow_reduc_timer_t;


//************************************************
// Constants
//************************************************

// Defines the number of LED intensity timers
#define LED_POW_REDUC_TIMERS_CNT 24
#define LED_POW_REDUC_TIMERS_MAX LED_POW_REDUC_TIMERS_CNT -1
#define LED_POW_REDUC_TIMERS_MIN 0


// Defines hours of day.
#define LED_POW_REDUC_TIME_MAX 23
#define LED_POW_REDUC_TIME_MIN 0


// Defines the number of LED intensity (0 - 100 %)
#define LED_POW_REDUC_INTENSITY_MAX 100
#define LED_POW_REDUC_INTENSITY_MIN 0
#define LED_POW_REDUC_INTENSITY_DEFAULT 20


// Defines maintenance time (0-65535 seconds ( 16 bits))
#define LED_POW_REDUC_MAINTENANCE_TIME_MAX 65535
#define LED_POW_REDUC_MAINTENANCE_TIME_MIN 0
#define LED_POW_REDUC_MAINTENANCE_TIME_DEFAULT 10


// Default configuration value for the on_at_err parameter.
#define LED_POW_REDUC_ON_AT_ERR_DEFAULT FALSE


//************************************************
// Configuration definition
//************************************************
//#define LED_POW_REDUC_TIMER_UNUSED 101 // Defines if a timer is used

// Global configuration (configuration that are shared for all switches in the stack)
typedef struct {
    BOOL on_at_err;
    u16  maintenance_time; // The time that the LEDs shall be turned on when any port changes link state
    u8   led_timer_intensity[LED_POW_REDUC_TIMERS_CNT]; // The LED intensity at the corresponding time_interval[LED_POW_REDUC_TIME_INTERVALS_CNT]
} led_pow_reduc_glbl_conf_t;

// Switch configuration (configurations that are local for a switch in the stack)
typedef struct {
    led_pow_reduc_glbl_conf_t glbl_conf;
} led_pow_reduc_local_conf_t;


//************************************************
// Functions
//************************************************
void    led_pow_reduc_mgmt_get_switch_conf(led_pow_reduc_local_conf_t *local_conf);
mesa_rc led_pow_reduc_mgmt_set_switch_conf(led_pow_reduc_local_conf_t *new_local_conf);
void led_pow_reduc_mgmt_err_detected(void) ;
mesa_rc led_pow_reduc_init(vtss_init_data_t *data);
void led_pow_reduc_mgmt_timer_get_init(led_pow_reduc_timer_t *current_timer);
BOOL led_pow_reduc_mgmt_timer_get(led_pow_reduc_timer_t *current_timer);
u8 led_pow_reduc_mgmt_next_change_get(u8 start_index);
mesa_rc led_pow_reduc_mgmt_timer_set(u8 start_index, u8 end_index, u8 intensity);
bool led_pow_reduc_module_enabled(void);

#ifdef __cplusplus
}
#endif
#endif // _LED_POW_REDUC_API_H_

