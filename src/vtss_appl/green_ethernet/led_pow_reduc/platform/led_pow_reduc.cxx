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

#ifdef VTSS_SW_OPTION_PHY
#include "misc_api.h"
#include "phy_api.h" // For PHY_INST
#endif

#include "critd_api.h"
#include "main.h"
#include "msg_api.h"
#include "led_api.h"
#include "led_pow_reduc.h"
#include "led_pow_reduc_api.h"
#include "port_api.h"

#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#include "misc_api.h"

#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
#include "daylight_saving_api.h"
#endif

#include <sysutil_api.h> // For system_get_tz_off
#include "microchip/ethernet/switch/api.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "led_pow_reduc_icli_functions.h"
#endif
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"

static led_pow_reduc_msg_t msg_conf; // semaphore-protected message transmission buffer(s).

//****************************************
// TRACE
//****************************************
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "led_pwr", "Led_Pow_Reduc control"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CONF] = {
        "conf",
        "LED_POW_REDUC configuration",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_CLI] = {
        "cli",
        "Command line interface",
        VTSS_TRACE_LVL_INFO
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* Critical region protection protecting the following block of variables */
static critd_t crit;

#define LED_POW_REDUC_CRIT_ENTER()         critd_enter(        &crit, __FILE__, __LINE__)
#define LED_POW_REDUC_CRIT_EXIT()          critd_exit(         &crit, __FILE__, __LINE__)
#define LED_POW_REDUC_CRIT_ASSERT_LOCKED() critd_assert_locked(&crit, __FILE__, __LINE__)

//***********************************************
// MISC
//***********************************************


/* Thread variables */
static vtss_handle_t led_pow_reduc_thread_handle;
static vtss_thread_t led_pow_reduc_thread_block;

//************************************************
// Function declartions
//************************************************

//************************************************
// Global Variables
//************************************************
static led_pow_reduc_stack_conf_t led_pow_reduc_stack_conf; // Configuration for whole stack (used when we're the primary switch, only).
static led_pow_reduc_local_conf_t led_pow_reduc_local_conf; // Current configuration for this switch.
static vtss_flag_t                status_flag;              // Flag for signaling that status data from a secondary switch has been received.
static u8                         led_intensity_current_value = 100;
static BOOL                       conf_read_done;           // We can not start the thread stuff before the configuration is read, so we use this variable to signal that configuration is read.

//************************************************
// Misc. functions
//************************************************

//
// Converts error to printable text
//
// In : rc - The error type
//
// Return : Error text
//
const char *led_pow_reduc_error_txt(mesa_rc rc)
{
    switch (rc) {
    case LED_POW_REDUC_ERROR_NOT_PRIMARY_SWITCH:
        return "Switch must be the primary switch";

    default:
        T_D("Default");
        return "";
    }
}

/*************************************************************************
** Message module functions
*************************************************************************/

/* Allocate request/reply buffer */
static void led_pow_reduc_msg_alloc(led_pow_reduc_msg_buf_t *buf, BOOL request)
{
    LED_POW_REDUC_CRIT_ENTER();
    buf->sem = (request ? &msg_conf.request.sem : &msg_conf.reply.sem);
    buf->msg = (request ? &msg_conf.request.msg[0] : &msg_conf.reply.msg[0]);
    LED_POW_REDUC_CRIT_EXIT();
    vtss_sem_wait(buf->sem);
}

/* Release message buffer */
static void led_pow_reduc_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    vtss_sem_post((vtss_sem_t *)contxt);
}

/* Send message */
static void led_pow_reduc_msg_tx(led_pow_reduc_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
    msg_tx_adv(buf->sem, led_pow_reduc_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_TRACE_MODULE_ID, isid, buf->msg, len);
}

// Getting message from the message module.
static BOOL led_pow_reduc_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    led_pow_reduc_msg_id_t msg_id = *(led_pow_reduc_msg_id_t *)rx_msg;
    T_R("Entering led_pow_reduc_msg_rx");
    T_N("msg_id: %d,  len: %zd, isid: %u", msg_id, len, isid);

    switch (msg_id) {

    // Update switch's configuration
    case LED_POW_REDUC_MSG_ID_CONF_SET_REQ: {
        // Got new configuration
        T_DG(TRACE_GRP_CONF, "msg_id = LED_POW_REDUC_MSG_ID_CONF_SET_REQ");
        led_pow_reduc_msg_local_switch_conf_t *msg;
        msg = (led_pow_reduc_msg_local_switch_conf_t *)rx_msg;
        LED_POW_REDUC_CRIT_ENTER();
        led_pow_reduc_local_conf  =  (msg->local_conf); // Update configuration for this switch.
        LED_POW_REDUC_CRIT_EXIT();
        break;
    }


    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }

    return TRUE;
}

// Transmits a new configuration to a secondary switch via the message protocol
//
// In : secondary_switch_id - The id of the switch to receive the new configuration.
static void led_pow_reduc_msg_tx_switch_conf (vtss_isid_t secondary_switch_id)
{
    // Send the new configuration to the switch in question
    led_pow_reduc_msg_buf_t      buf;
    led_pow_reduc_msg_local_switch_conf_t  *msg;
    led_pow_reduc_msg_alloc(&buf, 1);
    msg = (led_pow_reduc_msg_local_switch_conf_t *)buf.msg;
    LED_POW_REDUC_CRIT_ENTER(); // Protect led_pow_reduc_stack_conf
    msg->local_conf.glbl_conf = led_pow_reduc_stack_conf.glbl_conf;
    LED_POW_REDUC_CRIT_EXIT();
    T_DG(TRACE_GRP_CONF, "Transmit LED_POW_REDUC_MSG_ID_CONF_SET_REQ");
    // Do the transmission
    msg->msg_id = LED_POW_REDUC_MSG_ID_CONF_SET_REQ; // Set msg ID
    led_pow_reduc_msg_tx(&buf, secondary_switch_id, sizeof(*msg)); //  Send the msg
}



// Initializes the message protocol
static void led_pow_reduc_msg_init(void)
{
    /* Register for stack messages */
    msg_rx_filter_t filter;

    /* Initialize message buffers */
    vtss_sem_init(&msg_conf.request.sem, 1);
    vtss_sem_init(&msg_conf.request.sem, 1);

    memset(&filter, 0, sizeof(filter));
    filter.cb = led_pow_reduc_msg_rx;
    filter.modid = VTSS_TRACE_MODULE_ID;
    (void) msg_rx_filter_register(&filter);
}


//************************************************
// Configuration
//************************************************

// Function for sending configuration to all switches in the stack
void led_pow_reduc_conf_to_all (void)
{
    // Loop through all isids and send new configuration to secondary switch if
    // it exist.
    vtss_isid_t isid;
    for (isid = 1; isid < VTSS_ISID_END; isid++) {
        if (msg_switch_exists(isid)) {
            led_pow_reduc_msg_tx_switch_conf(isid);
        }
    }
}

// Setting configuration to default
static void  led_pow_conf_default_set(void)
{
    T_RG(TRACE_GRP_CONF, "Entering");

    LED_POW_REDUC_CRIT_ENTER();

    //Set default configuration
    int timer_index;
    memset(&led_pow_reduc_stack_conf, 0, sizeof(led_pow_reduc_stack_conf));
    led_pow_reduc_stack_conf.glbl_conf.maintenance_time = LED_POW_REDUC_MAINTENANCE_TIME_DEFAULT;
    led_pow_reduc_stack_conf.glbl_conf.on_at_err = LED_POW_REDUC_ON_AT_ERR_DEFAULT;
    for (timer_index = LED_POW_REDUC_TIMERS_MIN; timer_index <= LED_POW_REDUC_TIMERS_MAX; timer_index++) {
        led_pow_reduc_stack_conf.glbl_conf.led_timer_intensity[timer_index] = LED_POW_REDUC_INTENSITY_DEFAULT;
    }
    LED_POW_REDUC_CRIT_EXIT();

    // loop through all isids and send new configuration to all secondary
    // switches that exist.
    led_pow_reduc_conf_to_all();
}

// Check the maintenance timer and set the LED intensity (MUST be crit_region protected for protecting led_pow_reduc_local_conf )
//
// IN: new_maintenance_time - Value that the maintenance_timer should be set to if new_maintenance_time_vld = TRUE
//     new_maintenance_time_vld - Indicates if new_maintenance_time is valid
static void led_pow_reduc_maintenance_timer(u32 new_maintenance_time, BOOL new_maintenance_time_vld)
{
    static u16 maintenance_timer = 0;
    mesa_rc    rc;

    if (new_maintenance_time_vld) {
        T_D("Setting maintenance_timer to %d", maintenance_timer);
        maintenance_timer = new_maintenance_time;
    } else {
        if (maintenance_timer > 0) {
            maintenance_timer--;
            T_D("Decreasing maintenance_timer to %d", maintenance_timer);
        }
    }


    LED_POW_REDUC_CRIT_ASSERT_LOCKED(); // Make sure that we are crit_region protected.

    // Turn on LEDs at full power
    if (maintenance_timer > 0 || (led_front_led_in_error_state() && led_pow_reduc_local_conf.glbl_conf.on_at_err)) {
        T_D("Setting intensity to 100 pct");
        rc = meba_led_intensity_set(board_instance, 100);
    } else {
        T_D("Setting intensity to %d", led_intensity_current_value);
        rc = meba_led_intensity_set(board_instance, led_intensity_current_value);
    }
    if (rc != VTSS_RC_OK) {
        T_I("Failed to set led intensity, rc = %d", rc);
    }
}

static void led_pow_reduc_port_change(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    LED_POW_REDUC_CRIT_ENTER();
    T_D_PORT(port_no, "maintenance_time:%d", led_pow_reduc_local_conf.glbl_conf.maintenance_time);
    led_pow_reduc_maintenance_timer(led_pow_reduc_local_conf.glbl_conf.maintenance_time, TRUE);
    LED_POW_REDUC_CRIT_EXIT();
}

// Function that checks the LED intensity timers and updates the LED intensity accordingly
static void led_pow_reduc_chk_timers (void)
{
    time_t t = time(NULL);
    struct tm *timeinfo_p;
    struct tm timeinfo;


#if defined(VTSS_SW_OPTION_SYSUTIL)
    /* Correct for timezone */
    t += (system_get_tz_off() * 60);
#endif
#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
    /* Correct for DST */
    t += (time_dst_get_offset() * 60);
#endif
    timeinfo_p = localtime_r(&t, &timeinfo);

    T_N("Time %02d:%02d:%02d",
        timeinfo_p->tm_hour,
        timeinfo_p->tm_min,
        timeinfo_p->tm_sec);

    int timer_index;
    LED_POW_REDUC_CRIT_ENTER();
    for (timer_index = LED_POW_REDUC_TIMERS_MIN; timer_index <= LED_POW_REDUC_TIMERS_MAX; timer_index++) {
        T_R("Time %02d:%02d:%02d, intensity:%d, index:%d ",
            timeinfo_p->tm_hour,
            timeinfo_p->tm_min,
            timeinfo_p->tm_sec,
            led_pow_reduc_local_conf.glbl_conf.led_timer_intensity[timer_index], timer_index);

        T_R("intensity[%d]:%d", timer_index, led_pow_reduc_local_conf.glbl_conf.led_timer_intensity[timer_index]);

        // Set intensity corresponding to the current hour
        if (timer_index == timeinfo_p->tm_hour) {
            T_D("Setting LED led_intensity_current_value to %d ", led_pow_reduc_local_conf.glbl_conf.led_timer_intensity[timer_index]);
            led_intensity_current_value = led_pow_reduc_local_conf.glbl_conf.led_timer_intensity[timer_index];
        }
    }
    LED_POW_REDUC_CRIT_EXIT();

}

/****************************************************************************
* Module thread
****************************************************************************/
static void led_pow_reduc_thread(vtss_addrword_t data)
{
    // ***** Go into loop **** //
    T_R("Entering led_pow_reduc_thread");

    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_LED_POW_REDUC);

    // This will block this thread from running further until the PHYs are initialized.
    port_phy_wait_until_ready();

    // Initializing the LED power reduction API
    meba_reset(board_instance, MEBA_PORT_LED_INITIALIZE);

    // Wait for configuration to be done
    while (!conf_read_done) {
        VTSS_OS_MSLEEP(1000);
    }

    for (;;) {
        VTSS_OS_MSLEEP(1000);
        led_pow_reduc_chk_timers();
        LED_POW_REDUC_CRIT_ENTER();
        led_pow_reduc_maintenance_timer(0, FALSE);
        LED_POW_REDUC_CRIT_EXIT();
    }
}

/****************************************************************************/
/*  API functions (management  functions)                                   */
/****************************************************************************/

/**
 * Associate a LED intensity level with a clock time of the day.
 * It is used to set LEDs intensity level for a given clock time of the day.
 * clockTime [IN]: Clock time based on 24-hour time notation
 * intensity  [IN]: LEDs Intensity (in percentage)
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_led_power_reduction_clock_time_intensity_set(
    vtss_appl_led_power_reduction_led_glow_start_time_t clockTime,
    const vtss_appl_led_power_reduction_intensity_t  *const intensity)
{
    if (!led_pow_reduc_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    if (clockTime.hour > LED_POW_REDUC_TIMERS_MAX) {
        T_W("Invalid input, clock timer is more than max availabe timer");
        return VTSS_RC_ERROR;
    }
    if ((intensity == NULL) || (intensity->intensity > LED_POW_REDUC_INTENSITY_MAX)) {
        T_W("Invalid input, intensity is more than max intensity");
        return VTSS_RC_ERROR;
    }
    VTSS_RC(led_pow_reduc_mgmt_timer_set(clockTime.hour, clockTime.hour + 1, intensity->intensity));

    return VTSS_RC_OK;
}

/**
 * Get associated LEDs intensity level for a given clock time of the day.
 * clockTime [IN]: Clock time based on 24-hour time notation
 * intensity [OUT]: LEDs Intensity level(in percentage)
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_led_power_reduction_clock_time_intensity_get(
    vtss_appl_led_power_reduction_led_glow_start_time_t clockTime,
    vtss_appl_led_power_reduction_intensity_t  *const intensity)
{
    if (!led_pow_reduc_module_enabled()) {
        return VTSS_RC_ERROR;
    }
    if (clockTime.hour > LED_POW_REDUC_TIMERS_MAX) {
        T_W("Invalid input, clock timer is more than max available timer");
        return VTSS_RC_ERROR;
    }
    if (!intensity) {
        T_W("Invalid input, intensity is null");
        return VTSS_RC_ERROR;
    }
    LED_POW_REDUC_CRIT_ENTER();
    intensity->intensity = led_pow_reduc_stack_conf.glbl_conf.led_timer_intensity[clockTime.hour];
    LED_POW_REDUC_CRIT_EXIT();
    return VTSS_RC_OK;
}

/**
 * Set parameters to glow LEDs in full brightness(100% intensity).
 * conf    [IN]: Configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_led_power_reduction_full_led_brightness_conf_set(
    const vtss_appl_led_power_reduction_led_full_brightness_conf_t *const conf)
{
    led_pow_reduc_local_conf_t  led_local_conf;
    if (!led_pow_reduc_module_enabled()) {
        return VTSS_RC_ERROR;
    }
    if (!conf) {
        T_W("Invalid input, conf is null");
        return VTSS_RC_ERROR;
    }
    led_pow_reduc_mgmt_get_switch_conf(&led_local_conf);
    led_local_conf.glbl_conf.maintenance_time = conf->maintenance_duration;
    led_local_conf.glbl_conf.on_at_err        = conf->error_enable;
    VTSS_RC(led_pow_reduc_mgmt_set_switch_conf(&led_local_conf));
    return VTSS_RC_OK;
}

/**
 * Get parameters to glow LEDs in full brightness(100% intensity).
 * conf [OUT]: The configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_led_power_reduction_full_led_brightness_conf_get(
    vtss_appl_led_power_reduction_led_full_brightness_conf_t *const conf)
{
    led_pow_reduc_local_conf_t     led_local_conf;
    if (!led_pow_reduc_module_enabled()) {
        return VTSS_RC_ERROR;
    }
    if (!conf) {
        T_W("Invalid input, conf is null");
        return VTSS_RC_ERROR;
    }
    led_pow_reduc_mgmt_get_switch_conf(&led_local_conf);
    conf->maintenance_duration = led_local_conf.glbl_conf.maintenance_time;
    conf->error_enable         = led_local_conf.glbl_conf.on_at_err;
    return VTSS_RC_OK;
}

/**
 * Intensity clock time iterator function, it is used to get first or next clock time.
 * prev_clock_time [IN]:Previous clock time
 * next_clock_time [OUT]:Next clock time
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_led_power_reduction_clock_time_iterator(
    const vtss_appl_led_power_reduction_led_glow_start_time_t *const prev_clock_time,
    vtss_appl_led_power_reduction_led_glow_start_time_t *const next_clock_time)
{
    if (!led_pow_reduc_module_enabled()) {
        return VTSS_RC_ERROR;
    }
    return vtss::expose::snmp::IteratorComposeStaticRange<u8, 0, LED_POW_REDUC_TIMERS_MAX>
           (&(prev_clock_time->hour), &(next_clock_time->hour));
}
// Function that returns the next index where the intensity changes
//
// In/out : switch_conf - Pointer to configuration struct where the current configuration is copied to.
//
u8 led_pow_reduc_mgmt_next_change_get(u8 start_index)
{
    u8 timer_index;
    u8 intensity;
    u8 start_intensity;

    // Find if there is an intensity change from starting time until midnight
    for (timer_index = start_index; timer_index <= LED_POW_REDUC_TIMERS_MAX; timer_index++) {

        LED_POW_REDUC_CRIT_ENTER(); // Protect led_pow_reduc_local_conf
        intensity = led_pow_reduc_local_conf.glbl_conf.led_timer_intensity[timer_index];
        start_intensity = led_pow_reduc_local_conf.glbl_conf.led_timer_intensity[start_index];
        LED_POW_REDUC_CRIT_EXIT();

        T_I("start_intensity:%d, intensity:%d, timer_index:%d", start_intensity, intensity, timer_index);
        if (intensity != start_intensity) {
            return timer_index;
        }

    }
    // Find if there is an intensity change from midnight until start time
    for (timer_index = LED_POW_REDUC_TIMERS_MIN; timer_index <= start_index; timer_index++) {

        LED_POW_REDUC_CRIT_ENTER(); // Protect led_pow_reduc_local_conf
        intensity = led_pow_reduc_local_conf.glbl_conf.led_timer_intensity[timer_index];
        start_intensity = led_pow_reduc_local_conf.glbl_conf.led_timer_intensity[start_index];
        LED_POW_REDUC_CRIT_EXIT();

        T_I("start_intensity:%d, intensity:%d, timer_index:%d", start_intensity, intensity, timer_index);
        if (intensity != start_intensity) {
            return timer_index;
        }
    }

    // There is no change in intensity, so we signal that by returning the start_index
    return start_index;
}

// Function for setting the led timer interval
mesa_rc led_pow_reduc_mgmt_timer_set(u8 start_index, u8 end_index, u8 intensity)
{
    u8 led_index;

    if (!led_pow_reduc_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    // We allow both 00:00 and 24:00 as midnight, but in code midnight is given as index 0
    if (end_index == 24) {
        end_index = 0;
    }
    if (start_index == 24) {
        start_index = 0;
    }


    T_I("end_index:%d, start_index:%d, intensity:%d", end_index, start_index, intensity);

    // If start index = end_index then it mean set all timers to same value.
    if (start_index == end_index) {
        start_index = LED_POW_REDUC_TIMERS_MIN;
        end_index = LED_POW_REDUC_TIMERS_CNT;
    }

    // Crossing midnight
    if (start_index > end_index) {
        for (led_index = start_index; led_index <= 23; led_index++) {
            LED_POW_REDUC_CRIT_ENTER();
            led_pow_reduc_stack_conf.glbl_conf.led_timer_intensity[led_index] = intensity;
            LED_POW_REDUC_CRIT_EXIT();
        }
        start_index = 0; // Continue from midnight
    }

    for (led_index = start_index; led_index < end_index; led_index++) {
        T_I("led_index:%d, intensity:%d", led_index, intensity);
        LED_POW_REDUC_CRIT_ENTER();
        led_pow_reduc_stack_conf.glbl_conf.led_timer_intensity[led_index] = intensity;
        LED_POW_REDUC_CRIT_EXIT();
    }

    // Transfer new configuration to the switch in question.
    led_pow_reduc_conf_to_all();
    return VTSS_RC_OK;
}

// Function for initializing the timer struct used for looping through all timers for find the timer intervals
//
// In/out : current_timer - Pointer to a timer record containing the timer information.
void led_pow_reduc_mgmt_timer_get_init(led_pow_reduc_timer_t *current_timer)
{
    current_timer->start_index = 0;
    current_timer->end_index = 0;
    current_timer->first_index = 0;
    current_timer->start_new_search = TRUE;
}


// Function that can be used for looping through all timers for find the timer intervals
//
// In/out : current_timer - Pointer to a timer record containing the timer information.
// Return : FALSE when all timer has been "passed" else TRUE
BOOL led_pow_reduc_mgmt_timer_get(led_pow_reduc_timer_t *current_timer)
{
    T_I("end_index:%d, first_index:%d, start_index:%d, search:%d",
        current_timer->end_index, current_timer->first_index, current_timer->start_index, current_timer->start_new_search);
    if (!current_timer->start_new_search && (current_timer->end_index == current_timer->first_index)) {
        return FALSE; // OK, now we have been through all timers
    }

    // Starting a new loop through all timer intervals
    if (current_timer->start_new_search) {
        current_timer->first_index = led_pow_reduc_mgmt_next_change_get(0);
        current_timer->start_index = current_timer->first_index;
        current_timer->start_new_search = FALSE;
    } else {
        current_timer->start_index = current_timer->end_index; // Continue from where we left off, then last time the function was called.
    }

    if (current_timer->first_index == 0) {
        // Intensity is always the same.
        current_timer->start_index = 0;
        current_timer->end_index = 0;
    } else {
        current_timer->end_index = led_pow_reduc_mgmt_next_change_get(current_timer->start_index);
        T_I("end_index:%d, first_index:%d, start_index:%d", current_timer->end_index, current_timer->first_index, current_timer->start_index);
    }

    return TRUE; // More timer intervals exists.
}

//
// Function that returns the current configuration for a switch.
//
// In/out : switch_conf - Pointer to configuration struct where the current configuration is copied to.
//
void led_pow_reduc_mgmt_get_switch_conf(led_pow_reduc_local_conf_t *switch_conf)
{
    LED_POW_REDUC_CRIT_ENTER();
    // All switches have the same configuration
    memcpy(&switch_conf->glbl_conf, &led_pow_reduc_stack_conf.glbl_conf, sizeof(led_pow_reduc_stack_conf.glbl_conf));
    LED_POW_REDUC_CRIT_EXIT();
    T_IG(TRACE_GRP_CONF, "led_local_conf.glbl_conf.on_at_err:%d", switch_conf->glbl_conf.on_at_err);
}


// Function for setting the current configuration for a switch.
//
// In : switch_conf - Pointer to configuration struct with the new configuration.
//
// Return : VTSS error code
mesa_rc led_pow_reduc_mgmt_set_switch_conf(led_pow_reduc_local_conf_t  *new_switch_conf)
{
    BOOL change = (led_pow_reduc_stack_conf.glbl_conf.maintenance_time != new_switch_conf->glbl_conf.maintenance_time) ||
                  (led_pow_reduc_stack_conf.glbl_conf.on_at_err != new_switch_conf->glbl_conf.on_at_err); // Store if maintenance time is changed

    if (!led_pow_reduc_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    T_DG(TRACE_GRP_CONF, "Conf. changed");
    // Configuration changes only allowed by primary switch
    if (!msg_switch_is_primary()) {
        return LED_POW_REDUC_ERROR_NOT_PRIMARY_SWITCH;
    }

    // Ok now we can do the configuration
    LED_POW_REDUC_CRIT_ENTER();
    memcpy(&led_pow_reduc_stack_conf.glbl_conf, &new_switch_conf->glbl_conf, sizeof(led_pow_reduc_glbl_conf_t)); // Update the configuration for the switch
    LED_POW_REDUC_CRIT_EXIT();

    // Transfer new configuration to the switch in question.
    led_pow_reduc_conf_to_all();

    LED_POW_REDUC_CRIT_ENTER();
    // Activate maintenance_timer when LED power reduction configuration is changed.
    if (change) {
        led_pow_reduc_maintenance_timer(led_pow_reduc_stack_conf.glbl_conf.maintenance_time, TRUE);
    }
    LED_POW_REDUC_CRIT_EXIT();
    return VTSS_RC_OK;
}

bool led_pow_reduc_module_enabled(void)
{
    return fast_cap(MEBA_CAP_LED_DIM_SUPPORT);
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void vtss_appl_led_power_reduction_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_led_power_reduction_json_init(void);
#endif
extern "C" int led_pow_reduc_icli_cmd_register();

mesa_rc led_pow_reduc_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    T_D("led_pow_reduc_init enter, cmd=%d", data->cmd);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        vtss_flag_init(&status_flag);
        critd_init(&crit, "led_pow_reduc", VTSS_MODULE_ID_LED_POW_REDUC, CRITD_TYPE_MUTEX);
        LED_POW_REDUC_CRIT_ENTER();

        if (led_pow_reduc_module_enabled()) {
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
            /* Register private mib */
            vtss_appl_led_power_reduction_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
            vtss_appl_led_power_reduction_json_init();
#endif
            led_pow_reduc_icli_cmd_register();
        }

        LED_POW_REDUC_CRIT_EXIT();

        if (led_pow_reduc_module_enabled()) {
            vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                               led_pow_reduc_thread,
                               0,
                               "LED_POW_REDUC",
                               nullptr,
                               0,
                               &led_pow_reduc_thread_handle,
                               &led_pow_reduc_thread_block);
        }

        T_D("enter, cmd=INIT");

        // Initialize icfg
        if (led_pow_reduc_module_enabled()) {
#ifdef VTSS_SW_OPTION_ICFG
            if (led_pow_reduc_icfg_init() != VTSS_RC_OK) {
                T_E("ICFG not initialized correctly");
            }
#endif
        }

        break;

    case INIT_CMD_START:
        if (led_pow_reduc_module_enabled()) {
            led_pow_reduc_msg_init();
            (void)port_change_register(VTSS_MODULE_ID_LED_POW_REDUC, led_pow_reduc_port_change); // Prepare callback function for link up/down for ports
        }

        break;

    case INIT_CMD_CONF_DEF:
        if (led_pow_reduc_module_enabled()) {
            if (isid == VTSS_ISID_LOCAL) {
                /* Reset local configuration */
                T_D("isid local");
            } else if (VTSS_ISID_LEGAL(isid)) {
                /* Reset configuration (specific switch or all switches) */
                T_D("Restore to default");
                led_pow_conf_default_set();
            }
            T_D("enter, cmd=INIT_CMD_CONF_DEF");
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        if (led_pow_reduc_module_enabled()) {
            led_pow_conf_default_set();
            conf_read_done = TRUE; // Signal that reading of configuration is done.
        }
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        if (led_pow_reduc_module_enabled()) {
            led_pow_reduc_msg_tx_switch_conf(isid);
        }
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

