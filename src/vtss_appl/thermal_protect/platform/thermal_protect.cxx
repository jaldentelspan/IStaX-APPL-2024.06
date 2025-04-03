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

// ***************************************************************
// * Thermal protection is used for protecting the chip from
// * getting overheated. It is done by turning off ports when
// * a configurable temperature is reached. It is possible to map
// * the ports to a group  for when to shut down. Each group has
// * a configurable temperature at which the ports with the given
// * priorities are shut down.
// ***************************************************************
#include "critd_api.h"
#include "main.h"
#include "msg_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "thermal_protect.h"
#include "thermal_protect_api.h"
#include "microchip/ethernet/switch/api.h"
#if defined(VTSS_SW_OPTION_PHY)
#include "microchip/ethernet/switch/api.h"
#endif

#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#include "misc_api.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "thermal_protect_icli_functions.h" // For thermal_protect_icfg_init
#endif // VTSS_SW_OPTION_ICFG

#ifdef __cplusplus
extern "C" {
#endif

//****************************************
// TRACE
//****************************************
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "thermal", "Thermal_Protect control"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CONF] = {
        "conf",
        "THERMAL_PROTECT configuration",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CLI] = {
        "cli",
        "Command line interface",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* Critical region protection protecting the following block of variables */
static critd_t    crit;

#define THERMAL_PROTECT_CRIT_ENTER()         critd_enter(&crit,         __FILE__, __LINE__)
#define THERMAL_PROTECT_CRIT_EXIT()          critd_exit( &crit,         __FILE__, __LINE__)
#define THERMAL_PROTECT_CRIT_ASSERT_LOCKED() critd_assert_locked(&crit, __FILE__, __LINE__)

//***********************************************
// MISC
//***********************************************
/* Thread variables */
static vtss_handle_t thermal_protect_thread_handle;
static vtss_thread_t thermal_protect_thread_block;

//************************************************
// Global Variables
//************************************************
static thermal_protect_msg_t                    msg_conf; // semaphore-protected message transmission buffer(s).
static thermal_protect_stack_conf_t             thermal_protect_stack_conf;  // Configuration for whole stack (used when we're primary switch, only).
static vtss_appl_thermal_protect_switch_conf_t  thermal_protect_switch_conf; // Configuration for this switch.
static vtss_appl_thermal_protect_local_status_t switch_status; // Status from a secondary switch.
static vtss_flag_t                              status_flag; // Flag for signaling that status data from a secondary switch has been received.
static vtss_mtimer_t                            timer; // Timer for timeout
static BOOL                                     init_done = FALSE; // Signaling the initialization is done, because we can't start thermal protection before we have a valid
// configuration, else we risk that ports are shut down due to "random" configuration. It is OK not to have
// any mutex protection, since init_done is only changed once (during boot)

//************************************************
// Misc. functions
//************************************************
static mesa_rc thermal_protect_if2ife(vtss_ifindex_t ifindex,
                                      vtss_ifindex_elm_t *ife)
{
    mesa_rc rc;
    if (!vtss_ifindex_is_port(ifindex)) {
        rc = VTSS_APPL_THERMAL_PROTECT_ERROR_IFINDEX;
    } else {
        rc = vtss_ifindex_decompose(ifindex, ife);
    }
    return rc;
}
//
// Converts error to printable text
//
// In : rc - The error type
//
// Return : Error text
//
char *thermal_protect_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_THERMAL_PROTECT_ERROR_ISID:
        return (char *)"Invalid Switch ID";

    case VTSS_APPL_THERMAL_PROTECT_ERROR_SECONDARY_SWITCH:
        return (char *)"Could not get data from a secondary switch";

    case VTSS_APPL_THERMAL_PROTECT_ERROR_NOT_PRIMARY_SWITCH:
        return (char *)"Switch must to be primary switch";

    case VTSS_APPL_THERMAL_PROTECT_ERROR_VALUE:
        return (char *)"Invalid value";

    default:
        T_D("Default");
        return (char *)"";
    }
}

// Converts power down status  to printable text
//
// In : power down status
//
// Return : Printable text
//
char *thermal_protect_power_down2txt(BOOL powered_down)
{
    if (powered_down) {
        return (char *)"Port link is thermal protected (Link is down)";
    } else {
        return (char *)"Port link operating normally";
    }
}

// Function for keeping status off if a port is powered down due to thermal protection.
// IN : iport - The port number starting from 0
//      set   - TRUE if the port power down status shall be updated. FALSE is the port power status shall not be updated (For getting the status).
//      value - The new value for the port power status. Only valid if the parameter "set" is set to TRUE.
//     Return - TRUE if port is powered down due to thermal protection else FALSE
static BOOL is_port_down(mesa_port_no_t iport, BOOL set, BOOL value)
{
    THERMAL_PROTECT_CRIT_ENTER(); // Protect thermal_protect_local_conf
    static BOOL port_down_due_to_thermal_protection_init = FALSE;
    static mesa_port_list_t port_down_due_to_thermal_protection;
    port_iter_t  pit;
    BOOL result;

    // Initialize the port down array.
    if (port_down_due_to_thermal_protection_init == FALSE) {
        port_down_due_to_thermal_protection_init = TRUE;
        // Loop through all front ports
        if (port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                port_down_due_to_thermal_protection[pit.iport] = FALSE;
            }
        }
    }

    if (set) {
        // Setting new value
        port_down_due_to_thermal_protection[iport] = value;
    }

    result =  port_down_due_to_thermal_protection[iport]; // Getting current port status
    THERMAL_PROTECT_CRIT_EXIT();

    return result; // Return current status
}

// Function that returns this switch's temperature status
//
// Out : status - Pointer to where to put chip status
static void thermal_protect_get_local_status(vtss_appl_thermal_protect_local_status_t *status)
{
    int temp;
    port_iter_t  pit;

    // Loop through all front ports
    if (port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI) == VTSS_RC_OK) {
        while (port_iter_getnext(&pit)) {
            if (pit.iport >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                T_E("iport range ???");
                return;
            }
            status->port_powered_down[pit.iport] = is_port_down(pit.iport, FALSE, FALSE);
            if (meba_sensor_get(board_instance, MEBA_SENSOR_PORT_TEMP, pit.iport, &temp) != VTSS_RC_OK) {
                T_E("Could not read chip temperature, setting temperature to MAX");
                status->port_temp[pit.iport] = VTSS_APPL_THERMAL_PROTECT_TEMP_MAX;
            } else {
                status->port_temp[pit.iport] = temp;
            }
        }
    }
}

/*************************************************************************
** Message module functions
*************************************************************************/

/* Allocate request/reply buffer */
static void thermal_protect_msg_alloc(thermal_protect_msg_buf_t *buf, BOOL request)
{
    THERMAL_PROTECT_CRIT_ENTER();
    buf->sem = (request ? &msg_conf.request.sem : &msg_conf.reply.sem);
    buf->msg = (request ? &msg_conf.request.msg[0] : &msg_conf.reply.msg[0]);
    THERMAL_PROTECT_CRIT_EXIT();
    vtss_sem_wait(buf->sem);
}

/* Release message buffer */
static void thermal_protect_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    vtss_sem_post((vtss_sem_t *)contxt);
}

/* Send message */
static void thermal_protect_msg_tx(thermal_protect_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
    msg_tx_adv(buf->sem, thermal_protect_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_TRACE_MODULE_ID, isid, buf->msg, len);
}

// Transmits status from a secondary to the primary switch
//
// In : primary_switch_id - The primary switch's id
static void thermal_protect_msg_tx_switch_status (vtss_isid_t primary_switch_id)
{
    // Send the new configuration to the switch in question
    thermal_protect_msg_buf_t      buf;
    thermal_protect_msg_local_switch_status_t  *msg;

    vtss_appl_thermal_protect_local_status_t status;
    thermal_protect_get_local_status(&status); // Get chip temperature and thermal_protect rotation count

    thermal_protect_msg_alloc(&buf, 1);
    msg = (thermal_protect_msg_local_switch_status_t *)buf.msg;
    msg->status = status;

    // Do the transmission
    msg->msg_id = THERMAL_PROTECT_MSG_ID_STATUS_REP; // Set msg ID
    thermal_protect_msg_tx(&buf, primary_switch_id, sizeof(*msg)); //  Send the msg
}

// Getting message from the message module.
static BOOL thermal_protect_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    mesa_rc        rc;
    mesa_port_no_t port_index;
    thermal_protect_msg_id_t msg_id = *(thermal_protect_msg_id_t *)rx_msg;
    T_R("Entering thermal_protect_msg_rx");

    switch (msg_id) {

    // Update switch's configuration
    case THERMAL_PROTECT_MSG_ID_CONF_SET_REQ: {
        // Got new configuration
        T_DG(TRACE_GRP_CONF, "rx: msg_id = THERMAL_PROTECT_MSG_ID_CONF_SET_REQ");
        thermal_protect_msg_local_switch_conf_t *msg;
        msg = (thermal_protect_msg_local_switch_conf_t *)rx_msg;
        THERMAL_PROTECT_CRIT_ENTER();
        thermal_protect_switch_conf  =  (msg->switch_conf); // Update configuration for this switch.
        T_DG(TRACE_GRP_CONF, "grp_temperatures[0] = %d",
             thermal_protect_switch_conf.glbl_conf.grp_temperatures[0]);
        THERMAL_PROTECT_CRIT_EXIT();

        // Now that we have received a good configuration, we can safely resume our thread.
        init_done = TRUE;
        break;
    }

    // Port shutdown requested
    case THERMAL_PROTECT_MSG_ID_PORT_SHUTDOWN_REQ: {
        T_N("msg_id = THERMAL_PROTECT_MSG_ID_PORT_SHUTDOW_REQ");
        port_vol_conf_t conf;
        port_user_t     user = PORT_USER_THERMAL_PROTECT;
        thermal_protect_msg_port_shutdown_t *msg;
        msg = (thermal_protect_msg_port_shutdown_t *)rx_msg;
        port_index = msg->port_index;

        T_D_PORT(port_index, "Thermal protection - Shutting down port %u", port_index);
        if ((rc = port_vol_conf_get(user, port_index, &conf)) != VTSS_RC_OK) {
            T_E("%s", error_txt(rc));
        } else {
            conf.disable = msg->link_down;
            if ((rc = port_vol_conf_set(user, port_index, &conf)) != VTSS_RC_OK) {
                T_E("%s", error_txt(rc));
            }
        }
    }
    break;

    // Primary switch has requested status
    case THERMAL_PROTECT_MSG_ID_STATUS_REQ:
        T_N("msg_id = THERMAL_PROTECT_MSG_ID_STATUS_REQ");
        thermal_protect_msg_tx_switch_status(isid); // Transmit status back to primary switch.
        break;

    // Got status from a secondary switch
    case THERMAL_PROTECT_MSG_ID_STATUS_REP:
        T_N("msg_id = THERMAL_PROTECT_MSG_ID_STATUS_REP");
        thermal_protect_msg_local_switch_status_t *msg1;
        msg1 = (thermal_protect_msg_local_switch_status_t *)rx_msg;
        THERMAL_PROTECT_CRIT_ENTER();
        switch_status =  (msg1->status); // Update status for switch.
        vtss_flag_setbits(&status_flag, 1 << isid); // Signal that the message has been received
        THERMAL_PROTECT_CRIT_EXIT();
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
static void thermal_protect_msg_tx_switch_conf (vtss_isid_t secondary_switch_id)
{
    // Send the new configuration to the switch in question
    thermal_protect_msg_buf_t      buf;
    thermal_protect_msg_local_switch_conf_t  *msg;
    thermal_protect_msg_alloc(&buf, 1);
    msg = (thermal_protect_msg_local_switch_conf_t *)buf.msg;

    THERMAL_PROTECT_CRIT_ENTER();
    msg->switch_conf.local_conf = thermal_protect_stack_conf.local_conf[secondary_switch_id];
    msg->switch_conf.glbl_conf  = thermal_protect_stack_conf.glbl_conf;
    T_R("temp[0] = %d, isid = %d", thermal_protect_stack_conf.glbl_conf.grp_temperatures[0], secondary_switch_id);

    THERMAL_PROTECT_CRIT_EXIT();
    T_DG(TRACE_GRP_CONF, "Transmit THERMAL_PROTECT_MSG_ID_CONF_SET_REQ");
    // Do the transmission
    msg->msg_id = THERMAL_PROTECT_MSG_ID_CONF_SET_REQ; // Set msg ID
    thermal_protect_msg_tx(&buf, secondary_switch_id, sizeof(*msg)); //  Send the msg
}

// Transmit port shutdown from secondary to the primary switch. This is needed because we volatile need to shut down the port, and that needs to be done by the primary switch (because the secondary doesn't know it's own isid)
//
// In : port_index - Port to shut down
//      link_down  - TRUE to volatile shut down the port. FALSE to volatile power up.
static void thermal_protect_msg_tx_port_shutdown(mesa_port_no_t port_index, BOOL link_down)
{
    // Send the port shutdown to primary switch
    thermal_protect_msg_buf_t      buf;
    thermal_protect_msg_port_shutdown_t *msg;
    thermal_protect_msg_alloc(&buf, 1);
    msg = (thermal_protect_msg_port_shutdown_t *)buf.msg;
    msg->port_index = port_index;
    msg->link_down = link_down;
    T_RG(TRACE_GRP_CONF, "Transmit THERMAL_PROTECT_MSG_ID_PORT_SHUTDOW_REQ");
    // Do the transmission
    msg->msg_id = THERMAL_PROTECT_MSG_ID_PORT_SHUTDOWN_REQ; // Set msg ID
    thermal_protect_msg_tx(&buf, 0, sizeof(*msg)); //  Send the msg
}

// Transmits a status request to a secondary switch, and wait for the reply.
//
// In : secondary_switch_id - The secondary switch id.
//
// Return : True if NO reply from secondary switch.
static BOOL thermal_protect_msg_tx_switch_status_req (vtss_isid_t secondary_switch_id)
{
    BOOL              timeout;
    vtss_flag_value_t flag;
    vtss_tick_count_t time_tick;

    THERMAL_PROTECT_CRIT_ENTER();
    timeout = VTSS_MTIMER_TIMEOUT(&timer);
    THERMAL_PROTECT_CRIT_EXIT();

    if (timeout) {
        // Setup sync flag.
        flag = (1 << secondary_switch_id);
        vtss_flag_maskbits(&status_flag, ~flag);

        // Send the status request to the switch in question
        thermal_protect_msg_buf_t      buf;
        thermal_protect_msg_alloc(&buf, 1);
        thermal_protect_msg_id_req_t *msg = (thermal_protect_msg_id_req_t *)buf.msg;
        // Do the transmission
        msg->msg_id = THERMAL_PROTECT_MSG_ID_STATUS_REQ; // Set msg ID

        if (msg_switch_exists(secondary_switch_id)) {
            T_D("Transmitting THERMAL_PROTECT_MSG_ID_STATUS_REQ");
            thermal_protect_msg_tx(&buf, secondary_switch_id, sizeof(*msg)); //  Send the Mag
        } else {
            T_D("Skipped thermal_protect_msg_tx due to isid:%d msg switch doesn't exist", secondary_switch_id);
            return TRUE; // Signal status get failure.
        }


        // Wait for timeout or synch. flag to be set.
        time_tick = vtss_current_time() + VTSS_OS_MSEC2TICK(1000);
        return (vtss_flag_timed_wait(&status_flag, flag, VTSS_FLAG_WAITMODE_OR, time_tick) & flag ? 0 : 1);
    }

    T_DG(TRACE_GRP_CONF, "timeout not set");
    return TRUE; // Signal status get failure
}

// Initializes the message protocol
static void thermal_protect_msg_init(void)
{
    /* Register for stack messages */
    msg_rx_filter_t filter;

    /* Initialize message buffers */
    vtss_sem_init(&msg_conf.request.sem, 1);
    vtss_sem_init(&msg_conf.request.sem, 1);

    memset(&filter, 0, sizeof(filter));
    filter.cb = thermal_protect_msg_rx;
    filter.modid = VTSS_TRACE_MODULE_ID;
    (void) msg_rx_filter_register(&filter);
}

//************************************************
// Main functions
//************************************************


static void thermal_protect_shutdown(mesa_port_no_t iport, BOOL shutdown)
{
    // If the port is already in the correct state then don't do anything
    if ((is_port_down(iport, FALSE, TRUE) && shutdown) ||  (!is_port_down(iport, FALSE, TRUE) && !shutdown)) {
        return;
    }

    thermal_protect_msg_tx_port_shutdown(iport, shutdown);
    (void) is_port_down(iport, TRUE, shutdown);  // Store that port shutdown mode
}

// Function that checks if ports shall be shut down due to the chip temperature
// is above the configured levels.
static void thermal_protect_chk(void)
{
    u8 port_grp;
    i16 grp_temp;
    port_iter_t  pit;

    // The chip temperature
    vtss_appl_thermal_protect_local_status_t status;
    thermal_protect_get_local_status(&status);

    // Loop through all groups and ports to check if
    // any ports should be shut down
    int  grp_index;
    for (grp_index = 0; grp_index < VTSS_APPL_THERMAL_PROTECT_GROUP_CNT; grp_index++) {
        // Loop through all front ports
        if (port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI) == VTSS_RC_OK) {
            while (port_iter_getnext(&pit)) {
                THERMAL_PROTECT_CRIT_ENTER(); // Protect thermal_protect_switch_conf
                port_grp = thermal_protect_switch_conf.local_conf.port_grp[pit.iport];
                grp_temp = thermal_protect_switch_conf.glbl_conf.grp_temperatures[grp_index];
                THERMAL_PROTECT_CRIT_EXIT();
                if (port_grp == VTSS_APPL_THERMAL_PROTECT_GROUP_DISABLED) {
                    thermal_protect_shutdown(pit.iport, FALSE); // Turn on port
                    continue;
                }

                if (port_grp == grp_index) {
                    T_D_PORT(pit.iport, "grp_index = %d, port_grp= %d, Temp = %d, port_temp = %d",
                             grp_index, port_grp, grp_temp, status.port_temp[pit.iport]);

                    if (grp_temp <= status.port_temp[pit.iport])  {
                        thermal_protect_shutdown(pit.iport, TRUE);  // Shut down port
                    } else if (is_port_down(pit.iport, FALSE, TRUE)) {
                        thermal_protect_shutdown(pit.iport, FALSE); // Turn on port
                    }
                }
            }
        }
    }
}

//************************************************
// Configuration
//************************************************
// Function transmitting configuration to all switches in the stack
static void thermal_protect_msg_tx_conf_to_all(void)
{
    vtss_isid_t       isid;
    // loop through all isids and send new configuration to secondary switch if it exist.
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (msg_switch_exists(isid)) {
            thermal_protect_msg_tx_switch_conf(isid);
        }
    }
}

// Function for getting the default configuration for a single switch
// Out - switch_conf - Pointer to where to put the default configuration.
void vtss_appl_thermal_protect_switch_conf_default_get(vtss_appl_thermal_protect_switch_conf_t *switch_conf)
{
    //Set default configuration
    memset(switch_conf, 0, sizeof(*switch_conf)); // Set everything to 0. Non-zero default values will be set below.
    int grp_index;
    for (grp_index = 0; grp_index < VTSS_APPL_THERMAL_PROTECT_GROUP_CNT; grp_index++) {
        switch_conf->glbl_conf.grp_temperatures[grp_index] = VTSS_APPL_THERMAL_PROTECT_TEMP_MAX;
    }
    port_iter_t  pit;
    if (port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI) == VTSS_RC_OK) {
        while (port_iter_getnext(&pit)) {
            switch_conf->local_conf.port_grp[pit.iport] = VTSS_APPL_THERMAL_PROTECT_GROUP_DISABLED;
        }
    }
}

// Function for setting configuration to default
static void  thermal_protect_conf_default_set(void)
{
    T_RG(TRACE_GRP_CONF, "Entering");

    THERMAL_PROTECT_CRIT_ENTER();

    vtss_appl_thermal_protect_switch_conf_t switch_conf;             // Configuration for a single switch
    vtss_appl_thermal_protect_switch_conf_default_get(&switch_conf); // Get default configuration

    thermal_protect_stack_conf.glbl_conf = switch_conf.glbl_conf; // Common configuration for all switches

    // Set the configuration that is local for each switch
    vtss_isid_t isid;
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        thermal_protect_stack_conf.local_conf[isid] = switch_conf.local_conf;
    }

    THERMAL_PROTECT_CRIT_EXIT();

    // loop through all isids and send new configuration to secondary switch if it exist.
    thermal_protect_msg_tx_conf_to_all();
}

/****************************************************************************
* Module thread
****************************************************************************/
static void thermal_protect_thread(vtss_addrword_t data)
{
    // This will block this thread from running further until the PHYs are initialized.
    port_phy_wait_until_ready();

    THERMAL_PROTECT_CRIT_ENTER();
    VTSS_MTIMER_START(&timer, 1);
    THERMAL_PROTECT_CRIT_EXIT();

    if (meba_reset(board_instance, MEBA_SENSOR_INITIALIZE) != VTSS_RC_OK) {
        T_E("Could not initialize temperature sensor controller. Terminating thermal protection thread.");
        return;
    }

    // ***** Go into loop **** //
    T_R("Entering thermal_protect_thread");
    for (;;) {
        VTSS_OS_MSLEEP(5000);
        if (init_done) {
            thermal_protect_chk();
        }
    }
}

/****************************************************************************/
/*  API functions (management  functions)                                   */
/****************************************************************************/

// iterator function, it is used to get first and get next group index.
mesa_rc vtss_appl_thermal_protect_group_iterator(const vtss_appl_thermal_protect_group_t *const prev_group,
                                                 vtss_appl_thermal_protect_group_t *const next_group)
{
    if (!next_group) {
        return VTSS_RC_ERROR;
    }
    if (!prev_group) {
        // Get first
        next_group->group = VTSS_APPL_THERMAL_PROTECT_GROUP_MIN_VALUE;
    } else if (prev_group->group < VTSS_APPL_THERMAL_PROTECT_GROUP_MAX_VALUE) {
        // Get next
        next_group->group = prev_group->group + 1;
    } else {
        // Overflow
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

// Set group to temperature, it is used to set temperature value for a given group
// grp [IN]: group
// temp   [IN]: temperature
// VTSS_RC_OK if the operation succeeded.
mesa_rc vtss_appl_thermal_protect_group_temp_set(vtss_appl_thermal_protect_group_t grp,
                                                 const vtss_appl_thermal_protect_group_temperature_t  *temp)
{
    T_D("group: %u", grp.group);
    mesa_rc                                     rc = VTSS_APPL_THERMAL_PROTECT_ERROR_VALUE;
    vtss_appl_thermal_protect_switch_conf_t     switch_conf;
    switch_iter_t                               sit;

    if (!thermal_protect_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    if (!temp ||
        (temp->group_temperature > VTSS_APPL_THERMAL_PROTECT_TEMP_MAX) ||
        (temp->group_temperature < VTSS_APPL_THERMAL_PROTECT_TEMP_MIN) ||
        (grp.group > VTSS_APPL_THERMAL_PROTECT_GROUP_MAX_VALUE)) {
        T_W("Invalid param");
        return rc;
    }
    // Loop through all switches in stack
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG));
    while (switch_iter_getnext(&sit)) {
        vtss_appl_thermal_protect_switch_conf_get(&switch_conf, sit.isid);
        switch_conf.glbl_conf.grp_temperatures[grp.group] = temp->group_temperature;
        VTSS_RC(vtss_appl_thermal_protect_switch_conf_set(&switch_conf, sit.isid));
    }
    return VTSS_RC_OK;
}

// Get group to temperature configuration, it is used to set temperature value for a given group
// grp [IN]: group number
// param temp   [OUT]: temperature
// return VTSS_RC_OK if the operation succeeded.
mesa_rc vtss_appl_thermal_protect_group_temp_get(vtss_appl_thermal_protect_group_t grp,
                                                 vtss_appl_thermal_protect_group_temperature_t *temp)
{
    T_W("grp: %u", grp.group);
    mesa_rc                                 rc = VTSS_APPL_THERMAL_PROTECT_ERROR_VALUE;
    vtss_appl_thermal_protect_switch_conf_t switch_conf;

    if (!thermal_protect_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    if (!temp || (grp.group > VTSS_APPL_THERMAL_PROTECT_GROUP_MAX_VALUE)) {
        T_W("Invalid param");
        return rc;
    }
    vtss_appl_thermal_protect_switch_conf_get(&switch_conf, VTSS_ISID_LOCAL);
    temp->group_temperature = switch_conf.glbl_conf.grp_temperatures[grp.group];

    return VTSS_RC_OK;
}

// Set port group
// ifIndex  [IN]: Interface index
// group    [IN]: The port group value
// return VTSS_RC_OK if the operation succeeded.
mesa_rc vtss_appl_thermal_protect_port_group_set(vtss_ifindex_t ifIndex,
                                                 const vtss_appl_thermal_protect_group_t *group)
{
    mesa_rc                                 rc = VTSS_APPL_THERMAL_PROTECT_ERROR_VALUE;
    vtss_ifindex_elm_t                      ife;
    vtss_appl_thermal_protect_switch_conf_t switch_conf;

    if (!thermal_protect_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    if (!group || (group->group > VTSS_APPL_THERMAL_PROTECT_GROUP_CNT)) {
        T_W("Invalid params");
        return rc;
    }
    if ((rc = thermal_protect_if2ife(ifIndex, &ife)) != VTSS_RC_OK) {
        T_W("Invalid interface index, ifIndex: %u", VTSS_IFINDEX_PRINTF_ARG(ifIndex));
        return rc;
    }
    vtss_appl_thermal_protect_switch_conf_get(&switch_conf, ife.isid);
    switch_conf.local_conf.port_grp[ife.ordinal] = group->group;

    return vtss_appl_thermal_protect_switch_conf_set(&switch_conf, ife.isid);
}

// Get port grprity
// ifIndex  [IN]:  Interface index
// grp     [OUT]: The port grprity value
// return VTSS_RC_OK if the operation succeeded.
mesa_rc vtss_appl_thermal_protect_port_group_get(vtss_ifindex_t ifIndex,
                                                 vtss_appl_thermal_protect_group_t *group)
{
    mesa_rc                                 rc = VTSS_APPL_THERMAL_PROTECT_ERROR_VALUE;
    vtss_ifindex_elm_t                      ife;
    vtss_appl_thermal_protect_switch_conf_t switch_conf;

    if (!thermal_protect_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    if (!group) {
        T_W("Invalid param");
        return rc;
    }
    if ((rc = thermal_protect_if2ife(ifIndex, &ife)) != VTSS_RC_OK) {
        T_W("Invalid interface index, ifIndex: %u", VTSS_IFINDEX_PRINTF_ARG(ifIndex));
        return rc;
    }
    vtss_appl_thermal_protect_switch_conf_get(&switch_conf, ife.isid);
    group->group = switch_conf.local_conf.port_grp[ife.ordinal];

    return VTSS_RC_OK;
}

// Get port status
// ifIndex   [IN]: Iinterface index
// status   [OUT]: The port status data
// return VTSS_RC_OK if the operation succeeded.
mesa_rc vtss_appl_thermal_protect_port_status_get(vtss_ifindex_t ifIndex,
                                                  vtss_appl_thermal_protect_port_status_t *status)
{
    mesa_rc                                  rc = VTSS_APPL_THERMAL_PROTECT_ERROR_VALUE;
    vtss_ifindex_elm_t                       ife;
    vtss_appl_thermal_protect_local_status_t switch_local_status;

    if (!thermal_protect_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    if (!status) {
        T_W("Invalid params");
        return rc;
    }
    if ((rc = thermal_protect_if2ife(ifIndex, &ife)) != VTSS_RC_OK) {
        T_W("Invalid interface index, ifIndex: %u", VTSS_IFINDEX_PRINTF_ARG(ifIndex));
        return rc;
    }
    if ((rc = vtss_appl_thermal_protect_get_switch_status(&switch_local_status, ife.isid)) == VTSS_RC_OK) {
        status->temperature         = switch_local_status.port_temp[ife.ordinal];
        status->power_status        = switch_local_status.port_powered_down[ife.ordinal];
    }
    return rc;
}
//Function returns the capabilities supported in our switch
mesa_rc vtss_appl_thermal_protect_capabilities_get(vtss_appl_thermal_protect_capabilities_t
                                                   *const capabilities)
{
    capabilities->max_supported_group = VTSS_APPL_THERMAL_PROTECT_GROUP_CNT;
    return VTSS_RC_OK;
}
// Function that returns the current configuration for a switch.
// In : isid - isid for the switch the shall return its configuration
// In/out : switch_conf - Pointer to configuration struct where the current configuration is copied to.
void vtss_appl_thermal_protect_switch_conf_get(vtss_appl_thermal_protect_switch_conf_t *switch_conf,
                                               vtss_isid_t isid)
{
    if (isid != VTSS_ISID_LOCAL) {
        // Get this switch's configuration
        THERMAL_PROTECT_CRIT_ENTER();
        // Update the configuration for the switch
        memcpy(&switch_conf->local_conf, &thermal_protect_stack_conf.local_conf[isid], sizeof(vtss_appl_thermal_protect_local_conf_t));
        memcpy(&switch_conf->glbl_conf, &thermal_protect_stack_conf.glbl_conf, sizeof(vtss_appl_thermal_protect_glbl_conf_t));
        THERMAL_PROTECT_CRIT_EXIT();
    } else {
        THERMAL_PROTECT_CRIT_ENTER(); // Protect  thermal_protect_switch_conf
        *switch_conf = thermal_protect_switch_conf;
        T_D("grp 0 temp = %d", thermal_protect_switch_conf.glbl_conf.grp_temperatures[0]);
        THERMAL_PROTECT_CRIT_EXIT();
    }
}

// Function for setting the current configuration for a switch.
// In : isid - isid for the switch the shall return its configuration
// switch_conf - Pointer to configuration struct with the new configuration.
// Return : VTSS error code
mesa_rc vtss_appl_thermal_protect_switch_conf_set(vtss_appl_thermal_protect_switch_conf_t *new_switch_conf,
                                                  vtss_isid_t isid)
{
    if (!thermal_protect_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    // Configuration changes only allowed by primary switch
    if (!msg_switch_is_primary()) {
        T_W("Configuration change only allowed from primary switch");
        return VTSS_APPL_THERMAL_PROTECT_ERROR_NOT_PRIMARY_SWITCH;
    }

    // Ok now we can do the configuration
    THERMAL_PROTECT_CRIT_ENTER();
    // Update the configuration for the switch. Both common configuration for all switches (global), and the local configuration
    // for the switch in question.
    memcpy(&thermal_protect_stack_conf.glbl_conf, &new_switch_conf->glbl_conf, sizeof(vtss_appl_thermal_protect_glbl_conf_t));
    memcpy(&thermal_protect_stack_conf.local_conf[isid], &new_switch_conf->local_conf, sizeof(vtss_appl_thermal_protect_local_conf_t));
    THERMAL_PROTECT_CRIT_EXIT();

    if (isid == VTSS_ISID_LOCAL) {
        thermal_protect_msg_tx_conf_to_all();
    } else {
        // Transfer new configuration to the switch in question.
        thermal_protect_msg_tx_switch_conf(isid);
    }
    return VTSS_RC_OK;
}

// Function that returns status for a switch (e.g. chip temperature).
// In : isid - isid for the switch the shall return its chip temperature
// In/out : status - Pointer to status struct where the switch's status is copied to.
mesa_rc vtss_appl_thermal_protect_get_switch_status(vtss_appl_thermal_protect_local_status_t *status,
                                                    vtss_isid_t isid)
{
    if (!thermal_protect_module_enabled()) {
        return VTSS_RC_ERROR;
    }

    // All switches have the same configuration
    if (thermal_protect_msg_tx_switch_status_req(isid)) {
        T_D("Communication problem with secondary switch");
        memset(status, 0, sizeof(vtss_appl_thermal_protect_local_status_t)); // We have no real data, so we reset everything to 0.
        return VTSS_APPL_THERMAL_PROTECT_ERROR_SECONDARY_SWITCH;
    } else {
        THERMAL_PROTECT_CRIT_ENTER();
        memcpy(status, &switch_status, sizeof(vtss_appl_thermal_protect_local_status_t));
        THERMAL_PROTECT_CRIT_EXIT();
        return VTSS_RC_OK;
    }
}

bool thermal_protect_module_enabled(void)
{
    return fast_cap(MEBA_CAP_TEMP_SENSORS) > 0;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void Thermal_protection_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_thermal_protect_json_init(void);
#endif
extern "C" int thermal_protect_icli_cmd_register();

mesa_rc thermal_protect_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid; // Get switch id
    mesa_rc rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        vtss_flag_init(&status_flag);
        critd_init(&crit, "thermal_protect", VTSS_MODULE_ID_THERMAL_PROTECT, CRITD_TYPE_MUTEX);

        THERMAL_PROTECT_CRIT_ENTER();

        if (thermal_protect_module_enabled()) {
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
            /* Register private mib */
            Thermal_protection_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
            vtss_appl_thermal_protect_json_init();
#endif
            thermal_protect_icli_cmd_register();
        }

        THERMAL_PROTECT_CRIT_EXIT();

        /* Initialize and register trace resource's */
        // Create our poller thread. It is resumed once we receive
        // some configuration (in thermal_protect_msg_rx()).
        if (thermal_protect_module_enabled()) {
            vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                               thermal_protect_thread,
                               0,
                               "THERMAL_PROTECT",
                               nullptr,
                               0,
                               &thermal_protect_thread_handle,
                               &thermal_protect_thread_block);
        }

        break;

    case INIT_CMD_START:
        if (thermal_protect_module_enabled()) {
#ifdef VTSS_SW_OPTION_ICFG
            // Initialize ICLI config
            if ((rc = thermal_protect_icfg_init()) != VTSS_RC_OK) {
                T_E("%s", error_txt(rc));
            }
#endif
            thermal_protect_msg_init();
        }

        break;

    case INIT_CMD_CONF_DEF:
        if (thermal_protect_module_enabled()) {
            if (isid == VTSS_ISID_LOCAL) {
                /* Reset local configuration */
                T_D("isid local");
            } else if (VTSS_ISID_LEGAL(isid)) {
                /* Reset configuration (specific switch or all switches) */
                T_D("Restore to default");
                thermal_protect_conf_default_set();
            }
            T_D("enter, cmd=INIT_CMD_CONF_DEF");
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        if (thermal_protect_module_enabled()) {
            thermal_protect_conf_default_set();
        }

        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        if (thermal_protect_module_enabled()) {
            thermal_protect_msg_tx_switch_conf(isid); // Update configuration
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}
#ifdef __cplusplus
}
#endif

