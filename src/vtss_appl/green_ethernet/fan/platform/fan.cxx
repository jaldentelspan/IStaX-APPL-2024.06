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

#include "critd_api.h"
#include "main.h"
#include "msg_api.h"

#include "fan.h"
#include "fan_api.h"
#include "microchip/ethernet/switch/api.h"
#include "misc_api.h"
#include "board_if.h"
#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#ifdef VTSS_SW_OPTION_ICFG
#include "fan_icli_functions.h"
#endif
#include "vtss_cmdline.hxx"

#include "port_api.h"
#include <vtss_common_iterator.hxx>
#include <vtss/basics/expose/snmp/iterator-compose-range.hxx>
#include <vtss/basics/expose/snmp/iterator-compose-N.hxx>
#include <vtss/basics/expose/snmp/iterator-compose-depend-N.hxx>
#include <vtss/appl/fan.h>

static fan_msg_t msg_conf; // Mutex-protected message transmission buffer(s).

//****************************************
// TRACE
//****************************************
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "fan", "Fan control"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CONF] = {
        "conf",
        "FAN configuration",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CLI] = {
        "cli",
        "Command line interface",
        VTSS_TRACE_LVL_INFO
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* Critical region protection protecting the following block of variables */

static critd_t    crit;
#define FAN_CRIT_ENTER()         critd_enter(        &crit, __FILE__, __LINE__)
#define FAN_CRIT_EXIT()          critd_exit(         &crit, __FILE__, __LINE__)
#define FAN_CRIT_ASSERT_LOCKED() critd_assert_locked(&crit, __FILE__, __LINE__)

//***********************************************
// MISC
//***********************************************

/* Thread variables */
static vtss_handle_t fan_thread_handle;
static vtss_thread_t fan_thread_block;

const vtss_enum_descriptor_t vtss_appl_fan_pwm_freq_type_txt[] = {
    {MESA_FAN_PWM_FREQ_25KHZ,   "pwm25khz"},
    {MESA_FAN_PWM_FREQ_120HZ,   "pwm120hz"},
    {MESA_FAN_PWM_FREQ_100HZ,   "pwm100hz"},
    {MESA_FAN_PWM_FREQ_80HZ,    "pwm80hz"},
    {MESA_FAN_PWM_FREQ_60HZ,    "pwm60hz"},
    {MESA_FAN_PWM_FREQ_40HZ,    "pwm40hz"},
    {MESA_FAN_PWM_FREQ_20HZ,    "pwm20hz"},
    {0, 0},
};

//************************************************
// Function declartions
//************************************************

//************************************************
// Global Variables
//************************************************
static vtss_appl_fan_conf_t   fan_stack_conf; // Configuration for whole stack (used when we're the primary switch, only).
static vtss_appl_fan_conf_t   fan_local_conf; // Current configuration for this switch.
static vtss_appl_fan_status_t switch_status;  // Status from a secondary switch.
static vtss_flag_t            status_flag;    // Flag for signaling that status data from a secondary switch has been received.
static vtss_mtimer_t          timer;          // Timer for timeout
static BOOL                   fan_init_done;
static BOOL                   fan_speed_valid;
// 0 = fan off, 255 = Fan at full speed
static u8                     fan_speed_lvl = 255; // The fan speed level ( PWM duty cycle). Start at full speed.
static mesa_fan_conf_t        meba_fan_spec = {}; // Fan specification as obtained from meba, possibly overlayed by command line parameter "fan_pwm"

//************************************************
// Misc. functions
//************************************************

const char *mesa_fan_pwd_freq_t_to_str(mesa_fan_pwd_freq_t value)
{
    switch (value) {
    case MESA_FAN_PWM_FREQ_20HZ:
        return "20hz";
    case MESA_FAN_PWM_FREQ_40HZ:
        return "40hz";
    case MESA_FAN_PWM_FREQ_60HZ:
        return "60hz";
    case MESA_FAN_PWM_FREQ_80HZ:
        return "80hz";
    case MESA_FAN_PWM_FREQ_100HZ:
        return "100hz";
    case MESA_FAN_PWM_FREQ_120HZ:
        return "120hz";
    case MESA_FAN_PWM_FREQ_25KHZ:
        return "25khz";
    default:
        T_E("Unknow value: %d", value);
        return "";
    }
}

mesa_fan_pwd_freq_t str_to_mesa_fan_pwd_freq_t(const char *value)
{
    if (strncmp(value, "20hz", strlen("20hz")) == 0) {
        return MESA_FAN_PWM_FREQ_20HZ;
    }
    if (strncmp(value, "40hz", strlen("40hz")) == 0) {
        return MESA_FAN_PWM_FREQ_40HZ;
    }
    if (strncmp(value, "60hz", strlen("60hz")) == 0) {
        return MESA_FAN_PWM_FREQ_60HZ;
    }
    if (strncmp(value, "80hz", strlen("80hz")) == 0) {
        return MESA_FAN_PWM_FREQ_80HZ;
    }
    if (strncmp(value, "100hz", strlen("100hz")) == 0) {
        return MESA_FAN_PWM_FREQ_100HZ;
    }
    if (strncmp(value, "120hz", strlen("120hz")) == 0) {
        return MESA_FAN_PWM_FREQ_120HZ;
    }
    if (strncmp(value, "25khz", strlen("25khz")) == 0) {
        return MESA_FAN_PWM_FREQ_25KHZ;
    }
    return MESA_FAN_PWM_FREQ_20HZ;
}

static uint32_t fan_temp_sensor_cnt_get(void)
{
    static uint32_t val;

    // Cache value of MEBA_CAP_TEMP_SENSORS, because the call to fast_cap() is
    // pretty expensive in terms of time.
    if (!val) {
        val = fast_cap(MEBA_CAP_TEMP_SENSORS);
    }

    return val;
}

//
// Converts error to printable text
//
// In : rc - The error type
//
// Return : Error text
//
const char *fan_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_FAN_ERROR_ISID:
        return "Invalid Switch ID";

    case VTSS_APPL_FAN_ERROR_SECONDARY_SWITCH:
        return "Could not get data from secondary switch";

    case VTSS_APPL_FAN_ERROR_NOT_PRIMARY_SWITCH:
        return "Switch must to be the primary";

    case VTSS_APPL_FAN_ERROR_VALUE:
        return "Invalid value";

    case VTSS_APPL_FAN_ERROR_T_ON_LOW:
        return "Max. Temperature must be lower than 'On' temperature";

    case VTSS_APPL_FAN_ERROR_T_CONF:
        return "Max. Temperature must be higher than 'On' temperature";

    case VTSS_APPL_FAN_ERROR_FAN_NOT_RUNNING:
        return "Fan is supposed to be running, but fan speed is 0 rpm. Please make sure that the fan isn't blocked";

    default:
        T_D("Default");
        return "";
    }

}


// Checks if the switch is the primary and that the isid is a valid isid.
//
// In : isid - The isid to be checked
//
// Return : If isid is valid and switch is the primary then return VTSS_RC_OK, else return error code
mesa_rc fan_is_primary_switch_and_isid_legal(vtss_isid_t isid)
{
    if (!msg_switch_is_primary()) {
        T_W("Configuration change only allowed from primary switch");
        return VTSS_APPL_FAN_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (!VTSS_ISID_LEGAL(isid)) {
        return VTSS_APPL_FAN_ERROR_ISID;
    }

    return VTSS_RC_OK;
}

//************************************************
// Fan control
//************************************************

static mesa_rc sensor_iterator(
    const u8  *const prev_sensor_idx,
    u8 *const  next_sensor_idx
)
{
    vtss::expose::snmp::IteratorComposeRange<u8> itr(1, VTSS_APPL_FAN_TEMPERATURE_SENSOR_CNT_MAX);
    return itr(prev_sensor_idx, next_sensor_idx);
}

static mesa_rc fan_speed_level_get(vtss_isid_t isid, u8 *duty_cycle)
{
    return mesa_fan_cool_lvl_get(NULL, duty_cycle);
}

static mesa_rc fan_speed_level_set(vtss_isid_t isid, u8 duty_cycle)
{
    fan_speed_valid = TRUE;
    return mesa_fan_cool_lvl_set(NULL, duty_cycle);
}

// Function that returns this switch's status
//
// Out : status - Pointer to where to put chip status
static void fan_get_local_status(vtss_appl_fan_status_t *status)
{
    u32             rotation_count = 0;
    mesa_rc rc;
    u32 rpm;

    FAN_CRIT_ENTER();
    u16 fan_speed = 0;
    int i, chip_temp = 0;

    memset(status->chip_temp, 0, sizeof(status->chip_temp));

    // Get temperature
    for (i = 0; i < fan_temp_sensor_cnt_get(); i++) {
        if (meba_sensor_get(board_instance, MEBA_SENSOR_BOARD_TEMP, i, &chip_temp) != VTSS_RC_OK) {
            T_E("Could not get chip temperature %d", i);
            FAN_CRIT_EXIT();
            return;
        } else {
            status->chip_temp[i] = chip_temp;
        }
    }

    // Get fan speed
    status->fan_speed_setting_pct = fan_speed_lvl * 100 / 255;

    if ((rc = mesa_fan_rotation_get(NULL, &rotation_count)) != VTSS_RC_OK) {
        rotation_count = 0;
        T_R("%s", fan_error_txt(rc));
    }
    rpm = rotation_count * 60; // Give it back a rounds per minute.
    T_D("rotation_count:%d, rpm:%d", rotation_count, rpm);
    FAN_CRIT_EXIT();

    // Check compile customization is valid
    if (meba_fan_spec.ppr == 0) {
        T_E("FAN PPR must not to set to zero");
    } else {
        // Adjust for PPR (pulses per rotation)
        if (meba_fan_spec.type == MESA_FAN_3_WIRE_TYPE) {
            // If the fan is a 3-wire type, the pulses are only valid when PWM pulse is high.
            // We need to take that into account. See AN0xxx for how the calculation is done.

            // Get PWM duty cycle
            u8 duty_cycle = 0;
            if (fan_speed_level_get(VTSS_ISID_LOCAL, &duty_cycle) != VTSS_RC_OK) {
                duty_cycle = 0;
            }

            if (duty_cycle == 0 || meba_fan_spec.ppr == 0) {
                // Avoid divide by zero.
                T_N("Setting Fan Speed to 0");
                fan_speed  = 0;
            } else {
                fan_speed = rpm * duty_cycle / 0xFF / meba_fan_spec.ppr;
                T_N("Calculating Fan Speed. fan_speed:%d, rpm:%u, duty_cycle:%d, ppr:%u", fan_speed, rpm, duty_cycle, meba_fan_spec.ppr);
            }
        } else {
            fan_speed = rpm / meba_fan_spec.ppr;
        }
    }

    // Round up/down to the nearest 100.
    if (fan_speed > 0 && fan_speed < 100) {
        fan_speed = 100;
    } else {
        fan_speed = fan_speed / 100;
        fan_speed = fan_speed * 100;
    }

    // Return the fan speed
    status->fan_speed = fan_speed;
}

// Function for finding the highest system temperature in case there are multiple temperature sensors.
//
// In : temperature_array : Array with all the sensors readings
//
// Return : The value of the sensor with the highest temperature

i16 fan_find_highest_temp(i16 *temperature_array)
{
    u8 sensor;
    i16 max_temp = temperature_array[0];

    // Loop through all temperature sensors values and find the highest temperature.
    int sensor_cnt = fan_temp_sensor_cnt_get();
    for (sensor = 0; sensor < sensor_cnt; sensor++) {
        if (temperature_array[sensor] > max_temp) {
            max_temp = temperature_array[sensor];
        }
    }

    return max_temp;
}

// See Section 4 in AN0xxxx
// In : reset_last_temp - The Fan speed is only updated when the temperature has changed.
//                        If fan_control is call with "reset_last_temp" set to TRUE, the temperature
//                        "memory" is cleared, and the fan speed will be re-configured the next time fan_control is called.
static void fan_control(BOOL reset_last_temp)
{
    u8 new_fan_speed_lvl = 0;

    T_R("Entering fan_control");

    // Make sure that we done try and adjust fan before the controller is initialised.
    if (!fan_init_done) {
        return;
    }

    FAN_CRIT_ENTER();
    static i8 last_temp = 0; // The Temperature the last time the fan speed was adjusted

    if (reset_last_temp) {
        last_temp = 0;
        FAN_CRIT_EXIT();
        return;
    }
    FAN_CRIT_EXIT();

    //Because some result from the calculations below will be lower the zero, and we don't
    //to use floating point operations, we multiply with the resolution constant.
    const u16 resolution = 1000;
    //
    i32 fan_speed_lvl_pct; // The fan speed in percent. ( Negative number = fan stopped )
    const u8 fan_level_steps_pct = 10;  // How much the fan shall change for every adjustment.

    // Get the chip temperature
    vtss_appl_fan_status_t status;
    fan_get_local_status(&status); // Get chip temperature

    FAN_CRIT_ENTER();
    // Add some hysteresis to avoid that the fan is adjusted all the time.
    if ((last_temp < VTSS_APPL_FAN_TEMP_MAX && fan_find_highest_temp(&status.chip_temp[0]) > last_temp + 1) ||
        (last_temp > VTSS_APPL_FAN_TEMP_MIN && fan_find_highest_temp(&status.chip_temp[0]) < last_temp - 1)) {

        // Figure 4 in AN0xxx shows a state machine, but instead of a state machine I have
        // use the following which does the same. In this way we don't have to have fixed
        // number of states (Each fan speed level corresponds to a state).
        i32 delta_t = (fan_local_conf.glbl_conf.t_max - fan_local_conf.glbl_conf.t_on) * resolution *  fan_level_steps_pct / 100; // delta_t is described in AN0xxx section 4.

        // Calculate the fan speed in percent
        if (delta_t == 0) {
            // avoid divide by zero
            fan_speed_lvl_pct = 0;
        } else {
            fan_speed_lvl_pct = (fan_find_highest_temp(&status.chip_temp[0]) - fan_local_conf.glbl_conf.t_on) * resolution * fan_level_steps_pct / delta_t ;
            if (fan_speed_lvl_pct < 0) {
                fan_speed_lvl_pct = 0;
            }
        }

        meba_fan_param_t desc = {};
        if (meba_fan_param_get(board_instance, &desc) != VTSS_RC_OK) {
            T_E("Could not get fan parameters");
            FAN_CRIT_EXIT();
            return;
        }

        // Make sure that fan that the fan speed doesn't get below the PWM need to keep the fan running
        if ((fan_speed_lvl_pct < desc.min_pwm) && (fan_speed_lvl_pct > 0)) {
            fan_speed_lvl_pct = desc.min_pwm;
        }

        if (fan_find_highest_temp(&status.chip_temp[0]) > fan_local_conf.glbl_conf.t_on) {
            if (fan_speed_lvl_pct > 100) {
                new_fan_speed_lvl = 255;
            } else {
                new_fan_speed_lvl = 255 * fan_speed_lvl_pct / 100;
            }

        } else {
            new_fan_speed_lvl = 0;
        }
        T_I("new_fan_speed_lvl = %d, chip_temp =%d, delta_t = %u, temp_fan_speed_lvl_pct = %u, fan_speed_lvl = %d",
            new_fan_speed_lvl, fan_find_highest_temp(&status.chip_temp[0]), delta_t, fan_speed_lvl_pct, fan_speed_lvl);


        // Set new fan speed level
        if (new_fan_speed_lvl != fan_speed_lvl) {

            // Some fans can not start at a low PWM pulse width and needs to be kick started at full speed.
            if (fan_speed_lvl == 0) {
                T_I("Start fan at full speed for %d seconds", desc.start_time);
                // Start fan at full speed
                if (fan_speed_level_set(VTSS_ISID_LOCAL, 255) != VTSS_RC_OK) {
                    T_E("Could not set fan cooling level");
                }
                VTSS_OS_MSLEEP(desc.start_time * 1000);
            }

            T_I("Setting fan_speed_lvl %d", new_fan_speed_lvl);
            if (fan_speed_level_set(VTSS_ISID_LOCAL, new_fan_speed_lvl) != VTSS_RC_OK) {
                T_E("Could not set fan cooling level");
            }
        }

        fan_speed_lvl = new_fan_speed_lvl;
        last_temp = fan_find_highest_temp(&status.chip_temp[0]);
    }
    FAN_CRIT_EXIT();
}


/*************************************************************************
** Message module functions
*************************************************************************/

/* Allocate request/reply buffer */
static void fan_msg_alloc(fan_msg_buf_t *buf, BOOL request)
{
    FAN_CRIT_ENTER();
    buf->sem = (request ? &msg_conf.request.sem : &msg_conf.reply.sem);
    buf->msg = (request ? &msg_conf.request.msg[0] : &msg_conf.reply.msg[0]);
    FAN_CRIT_EXIT();
    vtss_sem_wait(buf->sem);
}

/* Release message buffer */
static void fan_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    vtss_sem_post((vtss_sem_t *)contxt);
}

/* Send message */
static void fan_msg_tx(fan_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
    msg_tx_adv(buf->sem, fan_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_TRACE_MODULE_ID, isid, buf->msg, len);
}

// Transmits status from a secondary switch to the primary
//
// In : primary_switch_id - The primary switch's switch id
static void fan_msg_tx_switch_status (vtss_isid_t primary_switch_id)
{
    // Send the new configuration to the switch in question
    fan_msg_buf_t      buf;
    fan_msg_local_switch_status_t  *msg;

    vtss_appl_fan_status_t status;


    fan_get_local_status(&status); // Get chip temperature and fan rotation count
    T_I("switch_status.fan_speed:%d", switch_status.fan_speed);
    fan_msg_alloc(&buf, 1);
    msg = (fan_msg_local_switch_status_t *)buf.msg;
    msg->status = status;

    // Do the transmission
    msg->msg_id = FAN_MSG_ID_STATUS_REP; // Set msg ID
    fan_msg_tx(&buf, primary_switch_id, sizeof(*msg)); //  Send the msg
}

// Getting message from the message module.
static BOOL fan_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    mesa_fan_conf_t fan_spec;
    fan_msg_id_t msg_id = *(fan_msg_id_t *)rx_msg;
    T_R("msg_id: %d,  len: %zd, isid: %u", msg_id, len, isid);

    switch (msg_id) {
    case FAN_MSG_ID_CONF_SET_REQ: {
        // Update switch's configuration
        // Got new configuration
        T_DG(TRACE_GRP_CONF, "msg_id = FAN_MSG_ID_CONF_SET_REQ");
        fan_msg_local_switch_conf_t *msg;
        msg = (fan_msg_local_switch_conf_t *)rx_msg;
        FAN_CRIT_ENTER();
        fan_local_conf  =  (msg->local_conf); // Update configuration for this switch.
        fan_spec = meba_fan_spec;
        fan_spec.fan_pwm_freq = fan_local_conf.glbl_conf.pwm;
        if (mesa_fan_controller_init(NULL, &fan_spec) != VTSS_RC_OK) {
            T_E("Failed setting new fan PWM value: %s", mesa_fan_pwd_freq_t_to_str(fan_spec.fan_pwm_freq));
        }

        FAN_CRIT_EXIT();

        // Reset last temperature reading in order to take new configuration into account the next time the fan speed is updated.
        fan_control(TRUE);


        break;
    }

    // Primary switch has requested status
    case FAN_MSG_ID_STATUS_REQ:
        T_N("msg_id = FAN_MSG_ID_STATUS_REQ");
        fan_msg_tx_switch_status(isid); // Transmit status back to the primary switch.
        break;

    case FAN_MSG_ID_STATUS_REP:
        // Got status from a secondary switch

        fan_msg_local_switch_status_t *msg;
        msg = (fan_msg_local_switch_status_t *)rx_msg;
        FAN_CRIT_ENTER();
        switch_status =  (msg->status); // Update status for switch.
        vtss_flag_setbits(&status_flag, 1 << isid); // Signal that the message has been received
        FAN_CRIT_EXIT();
        break;

    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }

    return TRUE;
}



// Transmits a new configuration to a secondary switch via the message protocol
//
// In : secondary_switch_id - The id of the switch to receive the new configuration.
static void fan_msg_tx_switch_conf (vtss_isid_t secondary_switch_id)
{
    // Send the new configuration to the switch in question
    fan_msg_buf_t      buf;
    fan_msg_local_switch_conf_t  *msg;
    fan_msg_alloc(&buf, 1);
    msg = (fan_msg_local_switch_conf_t *)buf.msg;

    FAN_CRIT_ENTER();
    msg->local_conf.glbl_conf = fan_stack_conf.glbl_conf;
    FAN_CRIT_EXIT();
    T_DG(TRACE_GRP_CONF, "Transmit FAN_MSG_ID_CONF_SET_REQ");
    // Do the transmission
    msg->msg_id = FAN_MSG_ID_CONF_SET_REQ; // Set msg ID
    fan_msg_tx(&buf, secondary_switch_id, sizeof(*msg)); //  Send the msg
}

// Transmits a status request to a secondary switch, and wait for the reply.
//
// In : secondary_switch_id - The secondary switch's switch id.
//
// Return : True if NO reply from secondary switch.
static BOOL fan_msg_tx_switch_status_req (vtss_isid_t secondary_switch_id)
{
    BOOL             timeout;
    vtss_flag_value_t flag;
    vtss_tick_count_t time_tick;

    FAN_CRIT_ENTER();
    timeout = VTSS_MTIMER_TIMEOUT(&timer);
    FAN_CRIT_EXIT();

    if (timeout) {
        // Setup sync flag.
        flag = (1 << secondary_switch_id);
        vtss_flag_maskbits(&status_flag, ~flag);



        // Send the status request to the switch in question
        fan_msg_buf_t      buf;
        fan_msg_alloc(&buf, 1);
        fan_msg_id_req_t *msg = (fan_msg_id_req_t *)buf.msg;
        // Do the transmission
        msg->msg_id = FAN_MSG_ID_STATUS_REQ; // Set msg ID

        T_D("secondary_switch_id = %d", secondary_switch_id);
        if (msg_switch_exists(secondary_switch_id)) {
            T_D("Transmitting FAN_MSG_ID_STATUS_REQ");
            fan_msg_tx(&buf, secondary_switch_id, sizeof(*msg)); //  Send the Mag
        } else {
            T_W("Skipped fan_msg_tx due to isid:%d msg switch doesn't exist", secondary_switch_id);
            return TRUE; // Signal status get failure.
        }


        // Wait for timeout or synch. flag to be set. Timeout set to 5 sec
        time_tick = vtss_current_time() + VTSS_OS_MSEC2TICK(5000);
        return (vtss_flag_timed_wait(&status_flag, flag, VTSS_FLAG_WAITMODE_OR, time_tick) & flag ? 0 : 1);
    }

    T_DG(TRACE_GRP_CONF, "timeout not set");
    return TRUE; // Signal status get failure
}

// Initializes the message protocol
static void fan_msg_init(void)
{
    /* Register for stack messages */
    msg_rx_filter_t filter;

    /* Initialize message buffers */
    vtss_sem_init(&msg_conf.request.sem, 1);
    vtss_sem_init(&msg_conf.request.sem, 1);

    memset(&filter, 0, sizeof(filter));
    filter.cb = fan_msg_rx;
    filter.modid = VTSS_TRACE_MODULE_ID;
    (void) msg_rx_filter_register(&filter);
}

//************************************************
// Configuration
//************************************************
// Function for updating all switches in a stack with the configuration
static void update_all_switches(void)
{
    // loop through all isids and send new configuration to secondary switch if it exist.
    vtss_isid_t isid;
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (msg_switch_exists(isid)) {
            fan_msg_tx_switch_conf(isid);
        }
    }

}

// Function for setting configuration to default
static void  fan_conf_default_set()
{
    FAN_CRIT_ENTER();
    //Set default configuration
    memset(&fan_stack_conf, 0, sizeof(fan_stack_conf)); // Set everything to 0. Non-zero default values will be set below.
    vtss_appl_fan_default_conf_get(&fan_stack_conf);
    FAN_CRIT_EXIT();

    update_all_switches(); // Send configuration to all switches in the stack
}

/****************************************************************************
* Module thread
****************************************************************************/
static void fan_thread(vtss_addrword_t data)
{
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_TRACE_MODULE_ID);
    T_R("Entering fan_thread");

    // This will block this thread from running further until the PHYs are initialized.
    port_phy_wait_until_ready();
    FAN_CRIT_ENTER();
    VTSS_MTIMER_START(&timer, 1);
    FAN_CRIT_EXIT();

    // Initialize temperature sensor.
    if (meba_reset(board_instance, MEBA_SENSOR_INITIALIZE) != VTSS_RC_OK) {  // initialize the temperature sensor
        T_E("Could not initialize temperature sensor, fan thread terminated");
        return;
    }
    // Initialize board fan(s).
    if (meba_reset(board_instance, MEBA_FAN_INITIALIZE) != VTSS_RC_OK) {
        T_E("Could not initialize board fan(s), fan thread terminated");
        return;
    }

    // Initialize icfg
#ifdef VTSS_SW_OPTION_ICFG
    FAN_CRIT_ENTER();
    if (fan_icfg_init() != VTSS_RC_OK) {
        T_E("ICFG not initialized correctly");
    }
    FAN_CRIT_EXIT();
#endif

    fan_init_done = TRUE;

    // ***** Go into loop **** //
    T_R("Entering fan_thread Loop");
    for (;;) {
        VTSS_OS_MSLEEP(1000);
        fan_control(FALSE); // Control fan speed
    }
}
/****************************************************************************/
/*  API functions (management  functions)                                   */
/****************************************************************************/

/*
 * Fan speed get function, it is used to get fan speed.
 * param usid    [IN]:  switch ID.
 * param speed   [OUT]: fan speed.
 * return VTSS_RC_OK if the operation succeeded.
 */

mesa_rc vtss_appl_fan_speed_get(
    vtss_usid_t           usid,
    vtss_appl_fan_speed_t *const speed
)
{
    vtss_isid_t            isid = VTSS_ISID_START;
    vtss_appl_fan_status_t status;

    /* Check illegal parameters */
    if (usid != VTSS_USID_START) {
        T_D("exit: Invalid USID = %d", usid);
        return VTSS_RC_ERROR;
    }
    if (speed == NULL) {
        T_E("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    if ( vtss_appl_fan_status_get(&status, isid) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    speed->fan_speed = status.fan_speed;
    return VTSS_RC_OK;
}



/*
* Fan sensors iterate function, it is used to get first and get next indexes.
* param prev_swid_idx   [IN]:  previous switch ID.
* param next_swid_idx   [OUT]: next switch ID.
* param prev_sensor_idx [IN]:  previous sensor ID.
* param next_sensor_idx [OUT]: next sensor ID.
* return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_fan_sensors_itr(
    const vtss_usid_t *const prev_swid_idx,
    vtss_usid_t       *const next_swid_idx,
    const u8          *const prev_sensor_idx,
    u8                *const next_sensor_idx
)
{
    vtss::IteratorComposeN<vtss_usid_t, u8>
    itr(&vtss_appl_iterator_switch, &sensor_iterator);
    return itr(prev_swid_idx, next_swid_idx, prev_sensor_idx, next_sensor_idx);
}


/*
 * Get Chip Temperature
 * param usid          [IN]: Switch ID for user view (The valid value starts from 1)
 * param sensorId      [IN]: Sensor ID
 * param chipTemp      [OUT]: The chip temp
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_fan_sensor_temp_get(
    vtss_usid_t                usid,
    u8                         sensorId,
    vtss_appl_fan_chip_temp_t  *const chipTemp
)
{
    vtss_isid_t            isid = VTSS_ISID_START;
    u8                     sensor_cnt;
    vtss_appl_fan_status_t status;

    T_D("enter : usid = %u sensorId = %u", usid, sensorId);
    /* Check illegal parameters */
    if (usid != VTSS_USID_START) {
        T_D("exit: Invalid USID = %d", usid);
        return VTSS_RC_ERROR;
    }
    sensor_cnt = fan_temp_sensor_cnt_get();
    if (!sensor_cnt) {
        T_D("exit: no temp sensors exist in this switch");
        return VTSS_RC_ERROR;
    }
    if ((chipTemp == NULL) || (sensorId > sensor_cnt) || (sensorId < 1)) {
        T_D("exit: Input parameter is NULL, sensor_id = %u", sensorId);
        return VTSS_RC_ERROR;
    }
    if ( vtss_appl_fan_status_get(&status, isid) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    chipTemp->chip_temp = status.chip_temp[sensorId - 1]; //array index start from 0
    return VTSS_RC_OK;
}

/*
 * Function returns the temperature sensors count.
 * param [IN] switch id
 * param [OUT] count
 */
mesa_rc vtss_appl_fan_temperature_sensors_count_get(
    vtss_isid_t isid,
    u8          *cnt
)
{
    *cnt = fan_temp_sensor_cnt_get();
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_fan_default_conf_get(vtss_appl_fan_conf_t *conf)
{
    conf->glbl_conf.t_max = VTSS_APPL_FAN_CONF_T_MAX_DEFAULT;
    conf->glbl_conf.t_on  = VTSS_APPL_FAN_CONF_T_ON_DEFAULT;
    conf->glbl_conf.pwm = meba_fan_spec.fan_pwm_freq;
    return VTSS_RC_OK;
}
/*
 * Function that returns the current configuration for a switch.
 * param In/out : switch_conf - Pointer to configuration struct where the current configuration is copied to.
 */
mesa_rc vtss_appl_fan_conf_get(vtss_appl_fan_conf_t *switch_conf)
{
    // All switches have the same configuration
    FAN_CRIT_ENTER();
    memcpy(&switch_conf->glbl_conf, &fan_stack_conf.glbl_conf, sizeof(vtss_appl_fan_conf_struct_t));
    FAN_CRIT_EXIT();
    return VTSS_RC_OK;
}

/*
  * Function for setting the current configuration for a switch.
  * param [IN] : switch_conf - Pointer to configuration struct with the new configuration.
  * Return : VTSS error code
  */
mesa_rc vtss_appl_fan_conf_set(const vtss_appl_fan_conf_t *new_switch_conf)
{
    if (!new_switch_conf) {
        T_D(" null new_switch_conf");
        return VTSS_APPL_FAN_ERROR_T_CONF;
    }
    T_D("t_max = %d, t_on = %d", new_switch_conf->glbl_conf.t_max, new_switch_conf->glbl_conf.t_on);
    if (( new_switch_conf->glbl_conf.t_max < VTSS_APPL_FAN_TEMP_MIN) || (new_switch_conf->glbl_conf.t_max > VTSS_APPL_FAN_TEMP_MAX)) {
        T_W("T_MAX should be with in range, Range :<%d to %d>", VTSS_APPL_FAN_TEMP_MIN, VTSS_APPL_FAN_TEMP_MAX);
        return VTSS_APPL_FAN_ERROR_T_CONF;
    }
    if ((new_switch_conf->glbl_conf.t_on < VTSS_APPL_FAN_TEMP_MIN) || (new_switch_conf->glbl_conf.t_on > VTSS_APPL_FAN_TEMP_MAX)) {
        T_W("T_ON should be with in range, Range :<%d to %d>", VTSS_APPL_FAN_TEMP_MIN, VTSS_APPL_FAN_TEMP_MAX);
        return VTSS_APPL_FAN_ERROR_T_CONF;
    }
    // Configuration changes only allowed by primary switch
    if (!msg_switch_is_primary()) {
        T_D("Configuration change only allowed from primary switch");
        return VTSS_APPL_FAN_ERROR_NOT_PRIMARY_SWITCH;
    }
    // It doesn't make any sense to have a t_max that is lower than T_on.
    if (new_switch_conf->glbl_conf.t_max <= new_switch_conf->glbl_conf.t_on) {
        T_W("T_MAX(fan in full speed) should be more than T_ON");
        return VTSS_APPL_FAN_ERROR_T_CONF;
    }
    // Ok now we can do the configuration
    FAN_CRIT_ENTER();
    memcpy(&fan_stack_conf.glbl_conf, &new_switch_conf->glbl_conf, sizeof(vtss_appl_fan_conf_struct_t)); // Update the configuration for the switch
    FAN_CRIT_EXIT();

    T_DG(TRACE_GRP_CONF, "Conf. changed");
    // Transfer new configuration to the switch in question.
    update_all_switches();
    return VTSS_RC_OK;
}

//
// Function that returns status for a switch (e.g. chip temperature).
//
// In : isid - isid for the switch the shall return its chip temperature
//
// In/out : status - Pointer to status struct where the switch's status is copied to.
//
mesa_rc vtss_appl_fan_status_get(vtss_appl_fan_status_t *status, vtss_isid_t isid)
{
    T_D("isid: %u", isid);
    // Configuration changes only allowed by primary switch
    if (fan_is_primary_switch_and_isid_legal(isid) != VTSS_RC_OK) {
        return VTSS_APPL_FAN_ERROR_ISID;
    }
    if (fan_msg_tx_switch_status_req(isid)) {
        T_D("Communication problem with secondary switch");
        memset(status, 0, sizeof(vtss_appl_fan_status_t)); // We have no real data, so we resets everything to 0.
        return VTSS_APPL_FAN_ERROR_SECONDARY_SWITCH;
    } else {
        FAN_CRIT_ENTER();
        memcpy(status, &switch_status, sizeof(vtss_appl_fan_status_t));
        FAN_CRIT_EXIT();
        // Give a warning if the FAN is not running (when it is supposed to run)
        if (meba_fan_spec.type != MESA_FAN_2_WIRE_TYPE) {
            if (fan_speed_valid && switch_status.fan_speed == 0 && switch_status.fan_speed_setting_pct > 0) {
                return VTSS_APPL_FAN_ERROR_FAN_NOT_RUNNING;
            }
        }
    }
    return VTSS_RC_OK;
}
//Function returns the capabilities supported in our switch
mesa_rc vtss_appl_fan_capabilities_get(vtss_appl_fan_capabilities_t *const capabilities)
{
    capabilities->sensor_count = VTSS_APPL_FAN_TEMPERATURE_SENSOR_CNT_MAX;
    return VTSS_RC_OK;
}

bool fan_module_enabled(void)
{
    return fast_cap(MEBA_CAP_FAN_SUPPORT) && (fast_cap(MEBA_CAP_TEMP_SENSORS) > 0);
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void fan_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_fan_json_init(void);
#endif
extern "C" int fan_icli_cmd_register();

mesa_rc fan_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid; // Get switch id

    switch (data->cmd) {
    case INIT_CMD_INIT:
        vtss_flag_init(&status_flag);
        critd_init(&crit, "fan", VTSS_MODULE_ID_FAN, CRITD_TYPE_MUTEX);

        FAN_CRIT_ENTER();

        if (fan_module_enabled()) {
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
            fan_mib_init();  /* Register our private mib */
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
            vtss_appl_fan_json_init();
#endif
            fan_icli_cmd_register();
        }

        FAN_CRIT_EXIT();
        if (fan_module_enabled() &&
            meba_fan_conf_get(board_instance, &meba_fan_spec) == VTSS_RC_OK) {

            const char *cmd_line_pwm = vtss_cmdline_get("fan_pwm");
            if (cmd_line_pwm) {
                meba_fan_spec.fan_pwm_freq = str_to_mesa_fan_pwd_freq_t(cmd_line_pwm);
            }

            /* Initialize and register trace resource's */
            vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                               fan_thread,
                               0,
                               "FAN",
                               nullptr,
                               0,
                               &fan_thread_handle,
                               &fan_thread_block);
            T_D("enter, cmd=INIT");
        }

        break;

    case INIT_CMD_START:
        if (fan_module_enabled()) {
            fan_msg_init();
        }
        break;

    case INIT_CMD_CONF_DEF:
        if (fan_module_enabled()) {
            if (isid == VTSS_ISID_LOCAL) {
                /* Reset local configuration */
                T_D("isid local");
            } else if (VTSS_ISID_LEGAL(isid)) {
                /* Reset configuration (specific switch or all switches) */
                T_D("Restore to default");
                fan_conf_default_set();
            }
        }

        T_D("enter, cmd=INIT_CMD_CONF_DEF");
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        if (fan_module_enabled()) {
            fan_conf_default_set();
        }

        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        if (fan_module_enabled()) {
            fan_msg_tx_switch_conf(isid);
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

