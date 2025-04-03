/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "interrupt_api.h"
#include "main_conf.hxx"     // For vtss::appl::main::module_conf_get()
#include "msg_api.h"         // For msg_wait()
#include "port_api.h"
#include "port_instance.hxx"
#include "port_listener.hxx"
#include "port_lock.hxx"
#include "port_trace.h"
#include "vtss_api_if_api.h"
#include <vtss/appl/port.h>
#include "lock.hxx"          // For vtss::Lock
#include "control_api.h"     // For control_system_reset_register()

#ifdef VTSS_SW_OPTION_CPUPORT
#include <vtss/appl/cpuport.h>
#endif





#ifdef VTSS_SW_OPTION_KR
#include "kr_api.h"
#endif

#ifdef VTSS_SW_OPTION_PHY
#include "phy_api.h"
#endif

#ifdef VTSS_SW_OPTION_POE
#include "poe_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PORT

// Port module state
typedef enum {
    PORT_MODULE_STATE_INIT, /* Initial state */
    PORT_MODULE_STATE_CONF, /* Configuration applied after INIT_CMD_ICFG_LOADING_POST */
    PORT_MODULE_STATE_ANEG, /* Auto negotiating ports (some seconds) */
    PORT_MODULE_STATE_POLL, /* Polling all ports */
    PORT_MODULE_STATE_READY /* Warm start ready  */
} port_module_state_t;

/* Structure for global variables */
static struct {
    /* Thread variables */
    vtss_handle_t              thread_handle;
    vtss_thread_t              thread_block;

    /* Critical region protection protecting callbacks, i.e.
       #change_table, #global_change_table, and #shutdown_table */
    critd_t                    cb_crit;

    /* Critical region protection protecting the remaining variables in this struct */
    critd_t                    crit;

    // Sum of PHY capabilities
    mepa_phy_cap_t             phy_cap;

    // Make it possible to pause/resume polling of ports.
    vtss::Lock                 thread_lock;
} port;

// Bugzilla#8911: VeriPHY sometimes gives wrong result, so we repeat the process
// VERIPHY_REPEAT_CNT times
#ifdef VTSS_SW_OPTION_POE
#define VERIPHY_REPEAT_CNT 5
#else
#define VERIPHY_REPEAT_CNT 2
#endif

#define PORT_FLAG_PHY_INTERRUPT 0x01
static vtss_flag_t                                       interrupt_wait_flag;
static mesa_port_list_t                                  fast_link_int;    // Indicates that a PHY fast link failure interrupt has been received
static mesa_port_list_t                                  sfp_los_int;      // Indicates that an SFP Loss of Signal interrupt has been received
vtss_appl_port_capabilities_t                            PORT_cap;         // Not static, because it's also used by port_instance.cxx
CapArray<port_instance_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_instances;
static meba_sfp_driver_t                                 *sfp_drivers = NULL;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "port", "Port module."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PORT_TRACE_GRP_ITER] = {
        "iter",
        "Iterations ",
        VTSS_TRACE_LVL_ERROR
    },
    [PORT_TRACE_GRP_PHY] = {
        "phy",
        "PHY",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PORT_TRACE_GRP_SFP] = {
        "sfp",
        "SFP",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PORT_TRACE_GRP_KR] = {
        "kr",
        "KR (clause 73)",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PORT_TRACE_GRP_CONF] = {
        "conf",
        "Configuration",
        VTSS_TRACE_LVL_ERROR
    },
    [PORT_TRACE_GRP_STATUS] = {
        "status",
        "Status",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PORT_TRACE_GRP_ICLI] = {
        "iCLI",
        "ICLI",
        VTSS_TRACE_LVL_ERROR
    },
    [PORT_TRACE_GRP_MIB] = {
        "mib",
        "MIB",
        VTSS_TRACE_LVL_ERROR
    },
    [PORT_TRACE_GRP_LINK_FLAP_DETECT] = {
        "link_flap_det",
        "Link Flap Detection",
        VTSS_TRACE_LVL_ERROR
    },
    [PORT_TRACE_GRP_CALLBACK] = {
        "callbacks",
        "Port status/link change callbacks",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

// Critd stuff
PortLockScope::PortLockScope(const char *file, int line)
    : file(file), line(line)
{
    critd_enter(&port.crit, file, line);
}

PortLockScope::~PortLockScope(void)
{
    critd_exit(&port.crit, file, line);
}

PortUnlockScope::PortUnlockScope(const char *file, int line)
    : file(file), line(line)
{
    critd_exit(&port.crit, file, line);
}

PortUnlockScope::~PortUnlockScope(void)
{
    critd_enter(&port.crit, file, line);
}

PortCallbackLockScope::PortCallbackLockScope(const char *file, int line)
    : file(file), line(line)
{
    critd_enter(&port.cb_crit, file, line);
}

PortCallbackLockScope::~PortCallbackLockScope(void)
{
    critd_exit(&port.cb_crit, file, line);
}

/******************************************************************************/
// PORT_capabilities_set()
// Once and for all, create this module's capabilities.
/******************************************************************************/
static void PORT_capabilities_set(void)
{
    mesa_port_no_t  port_no;
    meba_port_cap_t port_cap;

    PORT_cap.port_cnt               = fast_cap(MEBA_CAP_BOARD_PORT_COUNT) - fast_cap(MEBA_CAP_CPU_PORTS_COUNT);
    PORT_cap.last_pktsize_threshold = fast_cap(MESA_CAP_PORT_LAST_FRAME_LEN_THRESHOLD);
    PORT_cap.frame_length_max_min   = MESA_MAX_FRAME_LENGTH_STANDARD;
    PORT_cap.frame_length_max_max   = fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX);

#if defined(VTSS_SW_OPTION_KR)
    // Don't call kr_mgmt_capable(), because it hasn't initialized its members
    // yet.
    PORT_cap.has_kr_v2 = fast_cap(MESA_CAP_PORT_KR);
    PORT_cap.has_kr_v3 = fast_cap(MESA_CAP_PORT_KR_IRQ);
    PORT_cap.has_kr    = PORT_cap.has_kr_v2 || PORT_cap.has_kr_v3;
#else
    PORT_cap.has_kr_v2 = false;
    PORT_cap.has_kr_v3 = false;
    PORT_cap.has_kr    = false;
#endif

    for (port_no = 0; port_no < PORT_cap.port_cnt; port_no++) {
        VTSS_RC_ERR_PRINT(port_cap_get(port_no, &port_cap));
        PORT_cap.aggr_caps |= port_cap;
    }

    // Priority-based flow control support on this chip?
    PORT_cap.has_pfc = fast_cap(MESA_CAP_PORT_PFC);
}

/******************************************************************************/
// PORT_sfp_drivers_prepend()
/******************************************************************************/
static void PORT_sfp_drivers_prepend(meba_sfp_drivers_t drivers)
{
    // add all drivers in reverse order at the beginning of the list
    for (int i = 0; i < drivers.count; ++i) {
        drivers.sfp_drv[i].next = sfp_drivers;
        sfp_drivers = &drivers.sfp_drv[i];
    }
}

/******************************************************************************/
// PORT_sfp_driver_prepend()
/******************************************************************************/
static void PORT_sfp_driver_prepend(meba_sfp_driver_t *driver)
{
    driver->next = sfp_drivers;
    sfp_drivers = driver;
}

/******************************************************************************/
// PORT_no_valid()
/******************************************************************************/
static mesa_rc PORT_no_valid(mesa_port_no_t port_no, bool trace_error = true)
{
    if (port_no >= PORT_cap.port_cnt) {
        if (trace_error) {
            T_E("Illegal port_no (%u)", port_no);
        } else {
            // Don't throw trace error, because some modules (e.g. PTP) rely on
            // the port module to return an error if the port doesn't exist when
            // calling vtss_appl_port_status_get().
            T_I("Illegal port_no (%u)", port_no);
        }

        return VTSS_APPL_PORT_RC_INVALID_PORT;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PORT_ifindex_to_port()
/******************************************************************************/
static mesa_rc PORT_ifindex_to_port(vtss_ifindex_t ifindex, mesa_port_no_t &port_no)
{
    vtss_ifindex_elm_t ife;

    // Check that we can decompose the ifindex and that it's a port.
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
        port_no = VTSS_PORT_NO_NONE;
        return VTSS_APPL_PORT_RC_IFINDEX_NOT_PORT;
    }

    if (ife.ordinal >= PORT_cap.port_cnt) {
        return VTSS_APPL_PORT_RC_INVALID_PORT;
    }

    port_no = ife.ordinal;

    return VTSS_RC_OK;
}

/******************************************************************************/
// PORT_has_phy()
// Returns true if the port has a PHY attached to it. Both internal and external
// PHYs on the board count, but not PHYs part of an inserted CuSFP. Dual-media
// ports count as well.
// Usage: to determine that the port has a PHY so we can initialize it and its
// driver.
/******************************************************************************/
static bool PORT_has_phy(mesa_port_no_t port_no)
{
    meba_port_cap_t port_cap;

    if (port_no >= PORT_cap.port_cnt) {
        return false;
    }

    port_cap = port_custom_table[port_no].cap;

    return (port_cap & (MEBA_PORT_CAP_COPPER | MEBA_PORT_CAP_DUAL_COPPER)) || (port_cap & MEBA_PORT_CAP_VTSS_10G_PHY);
}

/******************************************************************************/
// PORT_sfp_detectable()
/******************************************************************************/
static bool PORT_sfp_detectable(mesa_port_no_t port_no)
{
    if (port_custom_table[port_no].cap & MEBA_PORT_CAP_SFP_DETECT  ||
        port_custom_table[port_no].cap & MEBA_PORT_CAP_DUAL_SFP_DETECT) {
        T_RG_PORT(PORT_TRACE_GRP_SFP, port_no, "Detection");
        return true;
    }

    T_RG_PORT(PORT_TRACE_GRP_SFP, port_no, "No detection");
    return false;
}

/******************************************************************************/
// PORT_los_interrupt_function()
/******************************************************************************/
static void PORT_los_interrupt_function(meba_event_t source_id, mesa_port_no_t port_no)
{
    bool    wake_up = false;
    mesa_rc rc;

    // Check port number
    if (port_no >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        T_E("illegal port_no: %u from interrupt routine (0x%x)", port_no, source_id);
        return;
    }

    T_I("Interrupt %s to port_no %u", source_id == MEBA_EVENT_LOS ? "SFP-LOS" : source_id == MEBA_EVENT_FLNK ? "PHY-FLNK" : "?!?", port_no);

    // Hook up for the next interrupt
    // To achieve backpressure, this should be done in the thread just before
    // taking action (reading the actual status) on the interrupt
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PORT, PORT_los_interrupt_function, source_id, INTERRUPT_PRIORITY_NORMAL)) != VTSS_RC_OK) {
        // If this function has succeeded before, it must again, hence T_E()
        T_E("vtss_interrupt_source_hook_set(%d) failed: %s", source_id, error_txt(rc));
    }

    if (source_id == MEBA_EVENT_LOS) {
        // SFP Loss of Signal event
        if (PORT_sfp_detectable(port_no)) {
            // Only for detectable SFP ports
            // In case of a fast shutdown/no shutdown we will always get this
            // interupt which means that the partner was shutdown. Therefore we
            // need to poll the status of the port to detect that the port
            // went down before the partner runs the no shutdown command.
            sfp_los_int[port_no] = true;
            wake_up = true;
        }
    } else if (source_id == MEBA_EVENT_FLNK) {
        // PHY Fast link (down) interrupt
        fast_link_int[port_no] = true;
        wake_up = true;
    } else {
        T_E("Unknown source_id (%d). Not one we have subscribed for", source_id);
    }

    if (wake_up) {
        vtss_flag_setbits(&interrupt_wait_flag, PORT_FLAG_PHY_INTERRUPT);
    }
}

/******************************************************************************/
// PORT_ams_interrupt_function()
/******************************************************************************/
static void PORT_ams_interrupt_function(meba_event_t source_id, uint32_t port_no)
{
    mesa_rc rc;

    // Hook up for the next interrupt
    // To achieve backpressure, this should be done in the thread just before
    // taking action (reading the actual status) on the interrupt
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PORT, PORT_ams_interrupt_function, MEBA_EVENT_AMS, INTERRUPT_PRIORITY_NORMAL)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set(%d) failed: %s", source_id, error_txt(rc));
    }

    // Wake up the thread
    vtss_flag_setbits(&interrupt_wait_flag, PORT_FLAG_PHY_INTERRUPT);
}

/******************************************************************************/
// PORT_reboot_handler()
/******************************************************************************/
static void PORT_reboot_handler(mesa_restart_t restart)
{
    mesa_port_no_t port_no;

    // Shut down all ports just before a reboot occurs. This handler is
    // installed with low priority to allow other modules to send e.g. dying
    // gasp frames before the ports are shut down.
    PORT_LOCK_SCOPE();

    for (port_no = 0; port_no < PORT_cap.port_cnt; port_no++) {
        // Call a function that takes down the port immediately rather than
        // waiting for port_instance::poll() to be called.
        port_instances[port_no].shutdown_now();
    }
}

/******************************************************************************/
// PORT_syslog_admin_state_changed()
/******************************************************************************/
static void PORT_syslog_admin_state_changed(port_user_t user, mesa_port_no_t port_no, bool new_admin_state)
{
#if defined(VTSS_SW_OPTION_SYSLOG)
    char buf[128], *p = buf;

    p += sprintf(p, "LINK-CHANGED: ");
    p += sprintf(p, "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);

    if (user == PORT_USER_STATIC) {
        p += sprintf(p, ", changed state to administratively %s.", new_admin_state ? "up" : "down");
    } else {
        p += sprintf(p, ", changed state to %s (%s).", new_admin_state ? "up" : "down", port_instance_vol_user_txt(user));
    }

    S_PORT_N(VTSS_ISID_START, port_no, "%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */
}

/******************************************************************************/
// PORT_syslog_link_state_changed()
/******************************************************************************/
static void PORT_syslog_link_state_changed(mesa_port_no_t port_no, bool new_link)
{
#ifdef VTSS_SW_OPTION_SYSLOG
    char buf[128], *p = buf;

    p += sprintf(p, "LINK-UPDOWN: ");
    p += sprintf(p, "Interface %s", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
    p += sprintf(p, ", changed state to %s.", new_link ? "up" : "down");
    S_PORT_N(VTSS_ISID_START, port_no, "%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */
}

/******************************************************************************/
// PORT_change_events_handle()
/******************************************************************************/
static void PORT_change_events_handle(mesa_port_no_t port_no, vtss_appl_port_status_t &new_port_status, bool link_down, bool link_up)
{
    if (link_down) {
        T_I_PORT(port_no, "Link down");
        // Gotta override this member, because this function may be called with
        // _port_status.link == true, e.g. when changing media.
        new_port_status.link = false;
        port_listener_change_notify(port_no, new_port_status);
        PORT_syslog_link_state_changed(port_no, new_port_status.link);
    }

    if (link_up) {
        T_I_PORT(port_no, "Link up");
        new_port_status.link = true;
        port_listener_change_notify(port_no, new_port_status);
        PORT_syslog_link_state_changed(port_no, new_port_status.link);
    }

    if (!link_up && !link_down) {
        // For new listeners, we need to tell them the current port state.
        port_listener_initial_change_notify(port_no, new_port_status);
    }
}

/******************************************************************************/
// PORT_vol_valid()
/******************************************************************************/
static mesa_rc PORT_vol_valid(port_user_t user, mesa_port_no_t port_no, bool config)
{
    /* Check user */
    if (user > PORT_USER_CNT || (config && user == PORT_USER_CNT)) {
        T_E("Illegal user: %d", user);
        return VTSS_APPL_PORT_RC_PARM;
    }

    /* Check port */
    if (PORT_no_valid(port_no) != VTSS_RC_OK) {
        T_E("Illegal port_no: %u", port_no);
        return VTSS_APPL_PORT_RC_PARM;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PORT_veriphy_start()
/******************************************************************************/
static mesa_rc PORT_veriphy_start(mesa_port_no_t port_no, port_veriphy_mode_t mode)
{
    meba_port_cap_t cap = port_instances[port_no].static_caps_get();

    // If the port doesn't support the veriphy just exit
    if (!(cap & MEBA_PORT_CAP_1G_PHY)) {
        return VTSS_RC_OK;
    }

    port_veriphy_t &veriphy = port_instances[port_no].get_veriphy();

    veriphy.running = 1;
    veriphy.valid = 0;
    veriphy.mode = mode;

    // Bugzilla#8911, Sometimes cable length is measured too long (but never too short),
    // so veriphy is done multiple times and the shortest length is found.
    // Start with the longest possible length.
    for (uint8_t i = 0; i  < 4; ++i) {
        veriphy.result.status[i] = MEPA_CABLE_DIAG_STATUS_UNKNOWN;
        veriphy.result.length[i] = 255;
        veriphy.variate_cnt = 0;
        T_D_PORT(port_no, "Len:%d",  veriphy.result.length[i]);
    }

    veriphy.repeat_cnt = VERIPHY_REPEAT_CNT;

    return port_instances[port_no].start_veriphy(mode == PORT_VERIPHY_MODE_BASIC ? 2 : mode == PORT_VERIPHY_MODE_NO_LENGTH ? 1 : 0);
}

/******************************************************************************/
// PORT_veriphy_run()
/******************************************************************************/
static void PORT_veriphy_run(void)
{
    // VeriPHY processing for all ports
#ifdef VTSS_SW_OPTION_POE
    // Needed due to bugzilla#8911, see the other comments regarding this.
    poe_status_t poe_port_status;
    poe_mgmt_get_status(&poe_port_status);
#endif

    for (mesa_port_no_t p = 0; p < PORT_cap.port_cnt; ++p) {
        port_veriphy_t &veriphy = port_instances[p].get_veriphy();
        if (veriphy.running) {
            mepa_cable_diag_result_t temp_result;
            mesa_rc rc = port_instances[p].get_veriphy(&temp_result);
            if (rc != VTSS_RC_INCOMPLETE) {

                veriphy.result.link = temp_result.link;
                for (int i = 0; i < 4; ++i) {
                    T_I_PORT(p, "status[%d]:0x%X, temp_status:0x%X, length:0x%X, temp_length:0x%X\n",
                             i, veriphy.result.status[i], temp_result.status[i], veriphy.result.length[i], temp_result.length[i]);

                    // Bugzilla#8911, Sometimes cable length is measured too long (but never too short),
                    // so veriphy is done multiple times and the shortest length is stored.

                    // First time we simply use the result we got from the PHY
                    if (veriphy.repeat_cnt < VERIPHY_REPEAT_CNT) {
                        // We have seen that the results variates when no cable is plugged in so if that
                        // happens we say that the port is unconnected. (adding 3 because the resolution is 3 m, so that variation is OK).
                        if ((veriphy.result.length[i] + 3) < temp_result.length[i]) {
                            veriphy.variate_cnt++;
                        } else {
                            veriphy.result.length[i] = temp_result.length[i];
                            veriphy.result.status[i] = temp_result.status[i];
                        }
                    } else {
                        veriphy.result.length[i] = temp_result.length[i];
                        veriphy.result.status[i] = temp_result.status[i];
                    }

                    T_I_PORT(p, "status[%d]:0x%X, temp_status:0x%X, length:0x%X, temp_length:0x%X",
                             i, veriphy.result.status[i], temp_result.status[i], veriphy.result.length[i], temp_result.length[i]);

#ifdef VTSS_SW_OPTION_POE
                    // Bugzilla#8911, we have that PoE boards can "confuse" VeriPhy, to wrongly report ABNORMAL, SHORT and OPEN, so when we have a PD, we only report OK (else it is set as unknown)
                    if (poe_port_status.port_status[p].pd_status != VTSS_APPL_POE_NO_PD_DETECTED &&
                        poe_port_status.port_status[p].pd_status != VTSS_APPL_POE_NOT_SUPPORTED &&
                        poe_port_status.port_status[p].pd_status != VTSS_APPL_POE_PD_FAULT &&
                        poe_port_status.port_status[p].pd_status != VTSS_APPL_POE_PSE_FAULT &&
                        poe_port_status.port_status[p].pd_status != VTSS_APPL_POE_DISABLED) {
                        T_I_PORT(p, "PoE Port status:%d", poe_port_status.port_status[p].pd_status);
                        if (temp_result.status[i] != MEPA_CABLE_DIAG_STATUS_OK) {
                            T_I_PORT(p, "temp_result.status[%d]:%d", i, temp_result.status[i]);
                            veriphy.result.status[i] = MEPA_CABLE_DIAG_STATUS_UNKNOWN;
                        }
                    }
#endif
                    T_I_PORT(p, "status[%d]:0x%X, length:0x%X variate_cnt:%d", i, veriphy.result.status[i], veriphy.result.length[i], veriphy.variate_cnt);
                }

                T_N_PORT(p, "veriPHY done, rc = %d", rc);

                // Work-around of bugzilla#8911 - If the result variates it is an indication of that the port in not connected
                if (veriphy.variate_cnt > 1) {
                    for (int i = 0; i < 4; i++) {
                        if (veriphy.result.status[i] != MEPA_CABLE_DIAG_STATUS_UNKNOWN) {
                            veriphy.result.status[i] = MEPA_CABLE_DIAG_STATUS_OPEN;
                            veriphy.result.length[i] = 0;
                        }
                    }

                    T_D_PORT(p, "Forcing status to open, vaiate_cnt:%d", veriphy.variate_cnt);
                }

                if (veriphy.repeat_cnt > 1) {
                    // Bugzilla#8911- Repeat veriphy a number of times in order to get correct result.
                    veriphy.repeat_cnt--;
                    port_instances[p].start_veriphy(veriphy.mode == PORT_VERIPHY_MODE_BASIC ? 2 :
                                                    veriphy.mode == PORT_VERIPHY_MODE_NO_LENGTH ? 1 : 0);
                } else {
                    veriphy.repeat_cnt = 0;
                    veriphy.running = 0;
                    veriphy.valid = (rc == VTSS_RC_OK ? 1 : 0);
                }
            }
        }
    }
}

/******************************************************************************/
// PORT_phy_driver_set()
/******************************************************************************/
static void PORT_phy_driver_set(mesa_port_no_t port_no)
{
    port_instances[port_no].phy_device_set();
}

/******************************************************************************/
// PORT_reload_defaults()
/******************************************************************************/
static void PORT_reload_defaults(void)
{
    mesa_port_no_t port_no;

    PORT_LOCK_SCOPE();
    for (port_no = 0; port_no < PORT_cap.port_cnt; port_no++) {
        port_instances[port_no].reload_defaults(port_no);
    }
}

/******************************************************************************/
// PORT_init_ports()
/******************************************************************************/
static void PORT_init_ports(void)
{
    mesa_port_no_t port_no;
    bool           reset_phys = true;
    uint32_t       loop_port_up_inj = fast_cap(VTSS_APPL_CAP_LOOP_PORT_UP_INJ);

    // Need to create a copy of the PORT_cap.port_cnt, otherwise it is
    // increased for some cases (LOOP_PORT_UP_INJ) and it is not updated
    uint32_t port_count = PORT_cap.port_cnt;













    // Initialize PHY API
    meba_reset(board_instance, MEBA_PHY_INITIALIZE);

    // Get sum of PHY capabilities
    for (port_no = 0; port_no < port_count; port_no++) {
        mepa_phy_info_t phy_info;
        if (meba_phy_info_get(board_instance, port_no, &phy_info) == MESA_RC_OK) {
            port.phy_cap = (mepa_phy_cap_t)(port.phy_cap | phy_info.cap);
        }
    }

    /* Release ports from reset */
    if (reset_phys) {
        meba_reset(board_instance, MEBA_PORT_RESET);
    }

    /* Remove loop ports from default VLAN before we potentially enable forwarding */
    mesa_port_list_t member = {};
    if (mesa_vlan_port_members_get(NULL, VTSS_VID_DEFAULT, &member) == VTSS_RC_OK) {
        for (port_no = port_count; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            member[port_no] = 0;
        }
        (void)mesa_vlan_port_members_set(NULL, VTSS_VID_DEFAULT, &member);
    }

    if (loop_port_up_inj != MESA_PORT_NO_NONE) {
        /* Include up-injection loop port */
        port_count++;
    }

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    /* Include mirror loop port */
    port_count++;
#endif /* VTSS_SW_OPTION_MIRROR_LOOP_PORT */

    // Initialization of ports need to happen before PHY/SPFs are initialized
    port_count = MIN(port_count, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT));
    for (port_no = 0; port_no < port_count; port_no++) {
        meba_port_cap_t port_cap = 0;
        VTSS_RC_ERR_PRINT(port_cap_get(port_no, &port_cap));
        if (port_cap == 0) {
            // Skip ports without capabilities
            continue;
        }

        // Fill in the members of the local port instance
        port_instances[port_no].init_members(port_no);
        T_DG_PORT(VTSS_TRACE_GRP_DEFAULT, port_no, "Initialized port instance");

        // Identify and initialize PHYs (no CuSFPs at this point)
        // This PHY initialization needs to take place before the
        // port interface is initialized.
        // Reason for that is that some PHYs perform calibration
        // steps during their initialization that require no
        // activity on the MAC interface as it interferes with
        // the calibration.
        if (PORT_has_phy(port_no)) {
            PORT_phy_driver_set(port_no);
            T_DG_PORT(VTSS_TRACE_GRP_DEFAULT, port_no, "Initialized port PHY");
        }

        /* Setup port with default configuration */
        bool loop_port = false;
        if (loop_port_up_inj != MESA_PORT_NO_NONE) {
            if (port_no == loop_port_up_inj) {
                /* Enable injection header */
                mesa_port_ifh_t ifh_conf = {};
                ifh_conf.ena_inj_header = true;
                ifh_conf.ena_xtr_header = false;
                (void)mesa_port_ifh_conf_set(NULL, port_no, &ifh_conf);
                loop_port = true;
            }
        }

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
        if (port_no == VTSS_SW_OPTION_MIRROR_LOOP_PORT) {
            mesa_learn_mode_t mode;

            /* Disable learning on loop port */
            memset(&mode, 0, sizeof(mode));
            (void)mesa_learn_port_mode_set(NULL, port_no, &mode);

            loop_port = true;
        }
#endif /* VTSS_SW_OPTION_MIRROR_LOOP_PORT */
        if (loop_port) {
            mesa_packet_rx_port_conf_t packet_port_conf;

            /* Disable CPU copy/redirect on loop port */
            if (mesa_packet_rx_port_conf_get(NULL, port_no, &packet_port_conf) == VTSS_RC_OK) {
                packet_port_conf.ipmc_ctrl_reg = MESA_PACKET_REG_FORWARD;
                packet_port_conf.igmp_reg = MESA_PACKET_REG_FORWARD;
                packet_port_conf.mld_reg = MESA_PACKET_REG_FORWARD;
                for (int j = 0; j < 16; j++) {
                    packet_port_conf.bpdu_reg[j] = MESA_PACKET_REG_FORWARD;
                    packet_port_conf.garp_reg[j] = MESA_PACKET_REG_FORWARD;
                }
                (void)mesa_packet_rx_port_conf_set(NULL, port_no, &packet_port_conf);
            }
        }
    }

    // Do post reset
    meba_reset(board_instance, MEBA_PORT_RESET_POST);

    /* Initialize port LEDs */
    meba_reset(board_instance, MEBA_PORT_LED_INITIALIZE);

    /* Open up port API only after init */
    // THIS IS THE FIRST UNLOCK OF PORT_CRIT
    T_I("Unlocking port mutex for the first time");
    critd_exit(&port.crit, __FILE__, __LINE__);
}

/******************************************************************************/
// PORT_sfp_driver_create()
/******************************************************************************/
static bool PORT_sfp_driver_create(mesa_port_no_t port_no, meba_sfp_driver_t *driver, meba_sfp_device_info_t &device_info)
{
    bool result;

    if ((result = meba_fill_driver(board_instance, port_no, driver, &device_info)) == false) {
        // Since we have already read the SFP ROM successfully with
        // meba_sfp_device_info_get(), we know that MEBA will be able to do it
        // again, so it shouldn't be able to fail, so we can safely throw a
        // trace error here.
        T_EG_PORT(PORT_TRACE_GRP_SFP, port_no, "meba_fill_driver(\"%s\") failed", device_info.vendor_pn);
    } else {
        T_IG_PORT(PORT_TRACE_GRP_SFP, port_no, "Successfully created driver for \"%s\"", device_info.vendor_pn);
    }

    return result;
}

/******************************************************************************/
// PORT_sfp_device_create()
/******************************************************************************/
static bool PORT_sfp_device_create(mesa_port_no_t port_no, meba_sfp_driver_t *driver, meba_sfp_device_info_t &device_info)
{
    meba_sfp_driver_address_t address_mode;
    meba_sfp_device_t         *device;

    address_mode.mode                       = mscc_sfp_driver_address_mode;
    address_mode.val.mscc_address.inst      = nullptr;
    address_mode.val.mscc_address.port_no   = port_no;
    address_mode.val.mscc_address.meba_inst = board_instance;

    // If the SFP was pre-provisioned in MEBA, we found the driver by its name
    // and didn't create it with meba_fill_driver(). In these cases, we need to
    // overwrite whatever is in device_info.transceiver (coming from the SFP
    // ROM) with what the pre-provisioned driver says it is (in case the driver
    // is created with meba_fill_driver(), meba_sfp_driver_tr_get() returns the
    // same value as is already in device_info.transceiver.
    if (driver->meba_sfp_driver_tr_get) {
        (void)driver->meba_sfp_driver_tr_get(NULL, &device_info.transceiver);
    }

    // This function allocates a device and assigns the driver and the
    // device_info we pass in the call to the device structure members.
    if ((device = driver->meba_sfp_driver_probe(driver, &address_mode, &device_info)) == nullptr) {
        T_EG(PORT_TRACE_GRP_SFP, "%u: meba_sfp_driver_probe() failed for \"%s\"", port_no, device_info.vendor_pn);
        return false;
    }

    // Assign the SFP device to the port instance and initialize it.
    port_instances[port_no].sfp_device_set(device);

    T_IG(PORT_TRACE_GRP_SFP, "%u: Created device for \"%s\"", port_no, device_info.vendor_pn);
    return true;
}

/******************************************************************************/
// PORT_sfp_driver_search()
/******************************************************************************/
static meba_sfp_driver_t *PORT_sfp_driver_search(meba_sfp_device_info_t &device_info)
{
    meba_sfp_driver_t *driver = sfp_drivers;

    while (driver) {
        if (strcmp(device_info.vendor_pn, driver->product_name)) {
            driver = driver->next;
            continue;
        }

        T_IG(PORT_TRACE_GRP_SFP, "Found existing driver for \"%s\"", device_info.vendor_pn);
        return driver;
    }

    // No existing driver found
    T_IG(PORT_TRACE_GRP_SFP, "No existing driver found for \"%s\"", device_info.vendor_pn);
    return nullptr;
}

/******************************************************************************/
// PORT_sfp_mac_to_mac_driver_name_get()
/******************************************************************************/
static const char *PORT_sfp_mac_to_mac_driver_name_get(meba_port_cap_t cap)
{
    if ((cap & MEBA_PORT_CAP_25G_FDX) != 0) {
        return "MAC-to-MAC-25G";
    }

    if ((cap & MEBA_PORT_CAP_10G_FDX) != 0) {
        return "MAC-to-MAC-10G";
    }

    if ((cap & MEBA_PORT_CAP_2_5G_FDX) != 0) {
        return "MAC-to-MAC-2.5G";
    }

    return "MAC-to-MAC-1G";
}

enum class SFPStatus {
    Inserted,
    Removed,
    NoChange,
    Invalid,
};

/******************************************************************************/
// PORT_sfp_mac_to_mac_driver_create()
// Creates a MAC-to-MAC driver, that is, a driver for backplane Cu, where no
// PHY and no SFP is connected.
/******************************************************************************/
static SFPStatus PORT_sfp_mac_to_mac_driver_create(mesa_port_no_t port_no)
{
    meba_sfp_device_info_t device_info;
    meba_sfp_driver_t      *driver;
    meba_port_cap_t        port_cap;

    if (port_instances[port_no].has_sfp_dev()) {
        // Driver already attached. Nothing to do.
        return SFPStatus::NoChange;
    }

    // We use a special "product name" when searching for such a driver. The
    // supported drivers have already been loaded (meba_mac_to_mac_driver_init).

    // The selected driver is based on the port's speed capabilities.
    VTSS_RC_ERR_PRINT(port_cap_get(port_no, &port_cap));

    // Fill in vendor_pn of device_info, because that's the field we use when
    // searching for an already loaded driver.
    vtss_clear(device_info);
    strncpy(device_info.vendor_pn, PORT_sfp_mac_to_mac_driver_name_get(port_cap), sizeof(device_info.vendor_pn));
    device_info.vendor_pn[sizeof(device_info.vendor_pn) - 1] = '\0';

    // Time to find a driver.
    // Search the already-loaded drivers for a driver with that product name.
    if ((driver = PORT_sfp_driver_search(device_info)) == nullptr) {
        // Unlike for normal SFPs, we cannot ask MEBA to create a device for a
        // backplane Cu, because there's no SFP ROM to read properties from.
        T_EG(PORT_TRACE_GRP_SFP, "%u: Unable to find a driver for \"%s\"", port_no, device_info.vendor_pn);
        return SFPStatus::Invalid;
    }

    // Time to create the device for it.
    if (!PORT_sfp_device_create(port_no, driver, device_info)) {
        return SFPStatus::Invalid;
    }

    return SFPStatus::Inserted;
}

/******************************************************************************/
// PORT_sfp_drv_status_check()
/******************************************************************************/
static SFPStatus PORT_sfp_drv_status_check(mesa_port_no_t port_no, bool sfp_is_inserted)
{
    meba_sfp_device_info_t device_info;
    meba_sfp_driver_t      *driver;

    if (port_instances[port_no].has_sfp_dev() || !port_instances[port_no].may_load_sfp()) {
        // Either the port instance has an SFP device present already or it
        // tells us that the SFP is unusuable (has_sfp_dev() == false and
        // may_load_sfp() == false). Therefore, we can remove it if it's no
        // longer inserted.
        if (!sfp_is_inserted) {
            // MEBA tells us that the SFP is no longer inserted
            T_IG(PORT_TRACE_GRP_SFP, "%u: SFP removed", port_no);
            port_instances[port_no].sfp_device_del();

            // On purpose we don't remove the driver that we created manually
            // because it might be possible to add it again, so we don't need
            // to do the detection again.
            return SFPStatus::Removed;
        }

        return SFPStatus::NoChange;
    }

    if (!sfp_is_inserted) {
        // No change in SFP presence state
        return SFPStatus::NoChange;
    }

    // SFP is inserted.

    // First get the SFP product name
    if ((port_custom_table[port_no].cap & MEBA_PORT_CAP_SFP_INACCESSIBLE) || !meba_sfp_device_info_get(board_instance, port_no, &device_info)) {
        T_IG(PORT_TRACE_GRP_SFP, "%u: Unable to read SFP ROM. Defaulting to MAC-to-MAC", port_no);
        return PORT_sfp_mac_to_mac_driver_create(port_no);
    }

    // Time to find a driver.
    // Search the already-loaded drivers for a driver with that product name.
    if ((driver = PORT_sfp_driver_search(device_info)) != nullptr) {
        if (!PORT_sfp_device_create(port_no, driver, device_info)) {
            return SFPStatus::Invalid;
        }

        return SFPStatus::Inserted;
    }

    // If we get here, no existing driver was found, so we need to ask MEBA to
    // create one for us.
    if ((driver = (meba_sfp_driver_t *)VTSS_CALLOC(1, sizeof(meba_sfp_driver_t))) == nullptr) {
        T_EG(PORT_TRACE_GRP_SFP, "%u: Unable to allocate %zu bytes for SFP \"%s\"", port_no, sizeof(meba_sfp_driver_t), device_info.vendor_pn);
        return SFPStatus::Invalid;
    }

    // MEBA will re-read the ROM and fill in required functions and device_info.
    if (!PORT_sfp_driver_create(port_no, driver, device_info)) {
        return SFPStatus::Invalid;
    }

    // We succeeded in creating a driver for the SFP.
    // Prepend it to the existing list of drivers, so that we won't need to have
    // MEBA create a driver next time this SFP is inserted.
    PORT_sfp_driver_prepend(driver);

    if (!PORT_sfp_device_create(port_no, driver, device_info)) {
        return SFPStatus::Invalid;
    }

    return SFPStatus::Inserted;
}

/******************************************************************************/
// PORT_sfp_drivers_init()
/******************************************************************************/
static void PORT_sfp_drivers_init(void)
{
    // SFP drivers
    PORT_sfp_drivers_prepend(meba_cisco_driver_init());
    PORT_sfp_drivers_prepend(meba_axcen_driver_init());
    PORT_sfp_drivers_prepend(meba_finisar_driver_init());
    PORT_sfp_drivers_prepend(meba_hp_driver_init());
    PORT_sfp_drivers_prepend(meba_d_link_driver_init());
    PORT_sfp_drivers_prepend(meba_oem_driver_init());
    PORT_sfp_drivers_prepend(meba_wavesplitter_driver_init());
    PORT_sfp_drivers_prepend(meba_avago_driver_init());
    PORT_sfp_drivers_prepend(meba_excom_driver_init());
    PORT_sfp_drivers_prepend(meba_mac_to_mac_driver_init());
    PORT_sfp_drivers_prepend(meba_fs_driver_init());
}

/******************************************************************************/
// PORT_thread()
/******************************************************************************/
static void PORT_thread(vtss_addrword_t data)
{
    vtss_appl_port_status_t new_port_status;
    mesa_port_no_t          port_no, resume_port_no;
    static const int        mesa_poll_timer_ticks = VTSS_OS_MSEC2TICK(1000);
    vtss_mtimer_t           mesa_poll_timer;
    bool                    link_down, link_up;
    mesa_port_list_t        sfp_insertion_status, old_sfp_insertion_status = {};
    mesa_rc                 rc;

    // Start MESA poll timer
    VTSS_MTIMER_START(&mesa_poll_timer, mesa_poll_timer_ticks);

    // This is used to indicate that a fast link failure interrupt has
    // requested a full run through all ports
    resume_port_no = MESA_PORT_NO_NONE;

    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PORT, PORT_los_interrupt_function, MEBA_EVENT_FLNK, INTERRUPT_PRIORITY_NORMAL)) != VTSS_RC_OK) {
        T_I("vtss_interrupt_source_hook_set(%d) failed: %s", MEBA_EVENT_FLNK, error_txt(rc));
    }

    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PORT, PORT_los_interrupt_function, MEBA_EVENT_LOS, INTERRUPT_PRIORITY_NORMAL)) != VTSS_RC_OK) {
        T_I("vtss_interrupt_source_hook_set(%d) failed: %s", MEBA_EVENT_LOS, error_txt(rc));
    }

    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PORT, PORT_ams_interrupt_function, MEBA_EVENT_AMS, INTERRUPT_PRIORITY_NORMAL)) != VTSS_RC_OK) {
        T_I("vtss_interrupt_source_hook_set(%d) failed: %s", MEBA_EVENT_AMS, error_txt(rc));
    }

    // Register a reboot handler, which allows us to shut down all ports before
    // the actual reboot occurs. We install it with a low priority in order for
    // other modules to get called back before we shut down the ports.
    control_system_reset_register(PORT_reboot_handler, VTSS_MODULE_ID_PORT, CONTROL_SYSTEM_RESET_PRIORITY_LOW);

    // Wait until ICFG has applied all configuration. If we didn't do this, we
    // could happen to get link and switch traffic on ports that are shut down
    // in the startup-config. The down-side is that it takes a bit longer to get
    // link and up and running.
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_PORT);
    T_I("ICFG has been applied. Time to move on");

    while (1) {
        port.thread_lock.wait();

        if (meba_sfp_insertion_status_get(board_instance, &sfp_insertion_status) != VTSS_RC_OK) {
            T_E("Could not perform a SFP module detect");
        }

        if (sfp_insertion_status != old_sfp_insertion_status) {
            T_IG(PORT_TRACE_GRP_SFP, "SFP insertion status changed: %s -> %s", old_sfp_insertion_status, sfp_insertion_status);
            old_sfp_insertion_status = sfp_insertion_status;
        }

        for (port_no = 0; port_no < PORT_cap.port_cnt; port_no++) {
            meba_port_cap_t port_cap = 0;
            VTSS_RC_ERR_PRINT(port_cap_get(port_no, &port_cap));
            if (port_cap == 0) {
                // Skip ports without capabilities
                continue;
            }

            if (resume_port_no == MESA_PORT_NO_NONE || fast_link_int[port_no] || sfp_los_int[port_no]) {
                // Either polling normally or a fast link interrupt occurred on
                // this port.
                if (fast_link_int[port_no] || sfp_los_int[port_no]) {
                    T_I_PORT(port_no, "Fast polling: PHY-link-down = %d, SFP-LoS = %d", fast_link_int.get(port_no), sfp_los_int.get(port_no));
                    // The port has gone down. Make sure the port instance gets
                    // to know about it if it's too fast for it to be  reflected
                    // in the polled port status, since we do want port link
                    // state change events when this happens.
                    port_instances[port_no].notify_link_down_set(fast_link_int[port_no], sfp_los_int[port_no]);
                    fast_link_int[port_no] = false;
                    sfp_los_int[port_no] = false;
                }

                {
                    // Poll port status
                    PORT_LOCK_SCOPE();
                    if (PORT_sfp_detectable(port_no)) {
                        (void)PORT_sfp_drv_status_check(port_no, sfp_insertion_status.get(port_no));
                    } else {
                        if (!port_instances[port_no].has_phy_dev()) {
                            // If it is not possible to detect the SFP and there
                            // is no PHY then we know this is a hardware
                            // connection (MAC to MAC, Backplane Cu). This
                            // connection is interpreted like an SFP, using a
                            // special SFP MAC-to-MAC driver.
                            (void)PORT_sfp_mac_to_mac_driver_create(port_no);
                        }
                    }

                    port_instances[port_no].poll(new_port_status, link_down, link_up);
                    (void)port_instances[port_no].led_update();
                    PORT_veriphy_run();
                }

                // Here' we don't have the scope lock, so it's time to call
                // potential listeners with link up/down events.
                // We can have both a link down and a link up event in stove.
                PORT_change_events_handle(port_no, new_port_status, link_down, link_up);
            }

            if (resume_port_no == port_no || resume_port_no == MESA_PORT_NO_NONE) {
                /* Either a fast link fail run is not activated or it has been done */
                resume_port_no = MESA_PORT_NO_NONE; /* Fast link fail run has been done */
                vtss_flag_value_t flag_value = vtss_flag_timed_waitfor(&interrupt_wait_flag,
                                                                       PORT_FLAG_PHY_INTERRUPT,
                                                                       VTSS_FLAG_WAITMODE_OR_CLR,
                                                                       VTSS_OS_MSEC2TICK(1000 / PORT_cap.port_cnt));
                if (flag_value == PORT_FLAG_PHY_INTERRUPT) {
                    // When an interrupt is received, the loop has to take a
                    // fast link failure run and when it gets back to the port
                    // number where it got interrupted, it must resume its
                    // normal polling from there.
                    resume_port_no = port_no;
                }
            }

            // Periodically poll MESA to help it consolidate counters
            if (VTSS_MTIMER_TIMEOUT(&mesa_poll_timer)) {
                (void)mesa_poll_1sec(NULL);
                VTSS_MTIMER_START(&mesa_poll_timer, mesa_poll_timer_ticks);
            }
        }
    }
}

/******************************************************************************/
// vtss_appl_port_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_port_capabilities_get(vtss_appl_port_capabilities_t *cap)
{
    if (cap == nullptr) {
        return VTSS_APPL_PORT_RC_PARM;
    }

    *cap = PORT_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_port_interface_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_port_interface_capabilities_get(vtss_ifindex_t ifindex, vtss_appl_port_interface_capabilities_t *if_caps)
{
#if defined(VTSS_SW_OPTION_KR)
    vtss_appl_port_status_t port_status;
#endif
    mesa_port_no_t          port_no;

    if (if_caps == nullptr) {
        return VTSS_APPL_PORT_RC_PARM;
    }

    VTSS_RC(port_ifindex_valid(ifindex));

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        // PALLETBD. Where do we get the CPU ports' capabilities from?
        return VTSS_RC_ERROR;
    }
#endif //VTSS_SW_OPTION_CPUPORT

    // Normal switch port
    VTSS_RC(PORT_ifindex_to_port(ifindex, port_no));

    PORT_LOCK_SCOPE();

    if_caps->static_caps = port_instances[port_no].static_caps_get();
    if_caps->sfp_caps    = port_instances[port_no].sfp_caps_get();

#if defined(VTSS_SW_OPTION_KR)
    port_instances[port_no].port_status_get(port_status);
    if_caps->has_kr = port_status.has_kr;
#else
    if_caps->has_kr = false;
#endif

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_port_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_port_conf_default_get(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *conf)
{
    mesa_port_no_t port_no;

    if (conf == nullptr) {
        return VTSS_APPL_PORT_RC_PARM;
    }

    VTSS_RC(port_ifindex_valid(ifindex));

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        return vtss_appl_cpuport_conf_default_get(ifindex, conf);
    }
#endif //VTSS_SW_OPTION_CPUPORT

    // Normal switch port
    VTSS_RC(PORT_ifindex_to_port(ifindex, port_no));

    // No need to take lock
    port_instances[port_no].conf_default_get(*conf);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_port_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_port_conf_get(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *conf)
{
    mesa_port_no_t port_no;

    if (conf == nullptr) {
        return VTSS_APPL_PORT_RC_PARM;
    }

    VTSS_RC(port_ifindex_valid(ifindex));

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        return vtss_appl_cpuport_conf_get(ifindex, conf);
    }
#endif //VTSS_SW_OPTION_CPUPORT

    // Normal switch port
    VTSS_RC(PORT_ifindex_to_port(ifindex, port_no));

    PORT_LOCK_SCOPE();
    port_instances[port_no].conf_get(*conf);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_port_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_port_conf_set(vtss_ifindex_t ifindex, const vtss_appl_port_conf_t *conf)
{
    mesa_port_no_t        port_no;
    vtss_appl_port_conf_t old_conf;
    mesa_rc               rc;

    if (conf == nullptr) {
        return VTSS_APPL_PORT_RC_PARM;
    }

    VTSS_RC(port_ifindex_valid(ifindex));

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        return vtss_appl_cpuport_conf_set(ifindex, conf);
    }
#endif //VTSS_SW_OPTION_CPUPORT

    // Normal switch port
    VTSS_RC(PORT_ifindex_to_port(ifindex, port_no));

    PORT_LOCK_SCOPE();
    port_instances[port_no].conf_get(old_conf);

    if ((rc = port_instances[port_no].conf_set(*conf)) == VTSS_RC_OK) {
#if defined(VTSS_SW_OPTION_SYSLOG) && defined(VTSS_SW_OPTION_ICLI)
        if (old_conf.admin.enable != conf->admin.enable) {
            PORT_syslog_admin_state_changed(PORT_USER_STATIC, port_no, conf->admin.enable);
        }
#endif /* VTSS_SW_OPTION_SYSLOG && VTSS_SW_OPTION_ICLI */
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_port_status_get()
/******************************************************************************/
mesa_rc vtss_appl_port_status_get(vtss_ifindex_t ifindex, vtss_appl_port_status_t *status)
{
    mesa_port_no_t port_no;

    VTSS_RC(port_ifindex_valid(ifindex));

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        return vtss_appl_cpuport_status_get(ifindex, status);
    }
#endif // VTSS_SW_OPTION_CPUPORT

    // Normal switch port
    VTSS_RC(PORT_ifindex_to_port(ifindex, port_no));

    PORT_LOCK_SCOPE();
    port_instances[port_no].port_status_get(*status);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_port_statistics_get()
/******************************************************************************/
mesa_rc vtss_appl_port_statistics_get(vtss_ifindex_t ifindex, mesa_port_counters_t *statistics)
{
    mesa_port_no_t port_no;

    VTSS_RC(port_ifindex_valid(ifindex));

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        return vtss_appl_cpuport_statistics_get(ifindex, statistics);
    }
#endif // VTSS_SW_OPTION_CPUPORT

    // Normal switch port
    VTSS_RC(PORT_ifindex_to_port(ifindex, port_no));

    return mesa_port_counters_get(NULL, port_no, statistics);
}

/******************************************************************************/
// vtss_appl_port_statistics_clear()
/******************************************************************************/
mesa_rc vtss_appl_port_statistics_clear(vtss_ifindex_t ifindex)
{
    mesa_port_no_t port_no;

    VTSS_RC(port_ifindex_valid(ifindex));

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        return vtss_appl_cpuport_statistics_clear(ifindex);
    }
#endif // VTSS_SW_OPTION_CPUPORT

    // Normal switch port
    VTSS_RC(PORT_ifindex_to_port(ifindex, port_no));

    {
        PORT_LOCK_SCOPE();

        // Also clear the link up/down counts held in port status.
        port_instances[port_no].link_up_down_cnt_clear();
    }

    return mesa_port_counters_clear(NULL, port_no);
}

/******************************************************************************/
// port_veriphy_start()
/******************************************************************************/
mesa_rc port_veriphy_start(port_veriphy_mode_t *mode)
{
    mesa_port_no_t  port_no;
    meba_port_cap_t port_cap;

    // RBNTBD: Rename to vtss_appl_port_veriphy_start()
    T_D("enter");

    PORT_LOCK_SCOPE();
    for (port_no = 0; port_no < PORT_cap.port_cnt; ++port_no) {
        VTSS_RC_ERR_PRINT(port_cap_get(port_no, &port_cap));

        if (port_cap == 0) {
            // Skip ports without capabilities
            continue;
        }

        port_veriphy_t &veriphy = port_instances[port_no].get_veriphy();
        if (mode[port_no] != PORT_VERIPHY_MODE_NONE && !veriphy.running) {
            T_I("starting VeriPHY on port_no %u", port_no);
            PORT_veriphy_start(port_no, mode[port_no]);
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/******************************************************************************/
// port_veriphy_result_get()
/******************************************************************************/
mesa_rc port_veriphy_result_get(mesa_port_no_t port_no, mepa_cable_diag_result_t *result, unsigned int timeout)
{
    unsigned int timer;

    // RBNTBD: Rename to vtss_appl_port_veriphy_result_get()

    T_N_PORT(port_no, "enter, %u", port_no);
    VTSS_RC(PORT_no_valid(port_no));

    mesa_rc rc = VTSS_APPL_PORT_RC_VERIPHY_RUNNING;
    for (timer = 0; ; timer++) {
        port_veriphy_t &veriphy = port_instances[port_no].get_veriphy();

        {
            PORT_LOCK_SCOPE();
            if (!veriphy.running) {
                *result = veriphy.result;
                T_D_PORT(port_no, "veriphy->valid =%d", veriphy.valid);
                if (veriphy.valid) {
                    rc = (mesa_rc)VTSS_RC_OK;
                } else {
                    rc = (mesa_rc)VTSS_APPL_PORT_RC_GEN;
                    result->status[0] = MEPA_CABLE_DIAG_STATUS_UNKNOWN;
                    result->status[1] = MEPA_CABLE_DIAG_STATUS_UNKNOWN;
                    result->status[2] = MEPA_CABLE_DIAG_STATUS_UNKNOWN;
                    result->status[3] = MEPA_CABLE_DIAG_STATUS_UNKNOWN;
                }
            } else {
                result->status[0] = MEPA_CABLE_DIAG_STATUS_RUNNING;
                result->status[1] = MEPA_CABLE_DIAG_STATUS_RUNNING;
                result->status[2] = MEPA_CABLE_DIAG_STATUS_RUNNING;
                result->status[3] = MEPA_CABLE_DIAG_STATUS_RUNNING;
            }
        }

        if (rc != VTSS_APPL_PORT_RC_VERIPHY_RUNNING || timer >= timeout) {
            break;
        }

        VTSS_OS_MSLEEP(1000);
    }

    T_D("exit");

    return rc;
}

/******************************************************************************/
// port_vol_conf_get()
/******************************************************************************/
mesa_rc port_vol_conf_get(port_user_t user, mesa_port_no_t port_no, port_vol_conf_t *conf)
{
    VTSS_RC(PORT_vol_valid(user, port_no, false));

    if (conf == nullptr) {
        T_E("%u: %s. Conf is null", port_no, port_instance_vol_user_txt(user));
        return VTSS_APPL_PORT_RC_PARM;
    }

    PORT_LOCK_SCOPE();
    return port_instances[port_no].vol_conf_get(user, *conf);
}

/******************************************************************************/
// port_vol_conf_set()
/******************************************************************************/
mesa_rc port_vol_conf_set(port_user_t user, mesa_port_no_t port_no, const port_vol_conf_t *new_conf)
{
    bool            old_admin_state, new_admin_state;
    port_vol_conf_t old_conf;

    VTSS_RC(PORT_vol_valid(user, port_no, true));

    if (new_conf == nullptr) {
        T_E("%u: User %s. conf is null", port_no, port_instance_vol_user_txt(user));
        return VTSS_APPL_PORT_RC_PARM;
    }

    if (port_instances[port_no].vol_conf_get(user, old_conf) != VTSS_RC_OK) {
        T_E("%u: User %s. Unable to get old conf", port_no, port_instance_vol_user_txt(user));
        return VTSS_APPL_PORT_RC_INVALID_PORT;
    }

    T_I("port_no: %u, user: %s, disable: %u, oper_up: %u, oper_down: %u, disable w/ recover: %u",
        port_no, port_instance_vol_user_txt(user), new_conf->disable,
        new_conf->oper_up, new_conf->oper_down, new_conf->disable_adm_recover);

    {
        PORT_LOCK_SCOPE();

        // This will set it in the port_instance and cause it to be applied
        // asynchronously to avoid mutex deadlocks.
        VTSS_RC(port_instances[port_no].vol_conf_set(user, *new_conf));
    }

    old_admin_state = !(old_conf.disable  || old_conf.disable_adm_recover);
    new_admin_state = !(new_conf->disable || new_conf->disable_adm_recover);

    if (old_admin_state != new_admin_state) {
        PORT_syslog_admin_state_changed(user, port_no, new_admin_state);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_vol_status_get()
/******************************************************************************/
mesa_rc port_vol_status_get(port_user_t user, mesa_port_no_t port_no, port_vol_status_t *status)
{
    port_vol_conf_t vol_conf;
    uint32_t        usr;
    bool            disable, disable_adm_recover, oper_up, oper_down;

    VTSS_RC(PORT_vol_valid(user, port_no, false));

    if (status == nullptr) {
        return VTSS_APPL_PORT_RC_PARM;
    }

    vtss_clear(*status);
    status->user = PORT_USER_STATIC;
    strcpy(status->name, port_instance_vol_user_txt(PORT_USER_STATIC));

    PORT_LOCK_SCOPE();
    for (usr = PORT_USER_STATIC; usr < PORT_USER_CNT; usr++) {
        if (usr != user && user != PORT_USER_CNT) {
            continue;
        }

        port_instances[port_no].vol_conf_get(static_cast<port_user_t>(usr), vol_conf);

        disable             = vol_conf.disable;
        disable_adm_recover = vol_conf.disable_adm_recover;
        oper_up             = vol_conf.oper_up;
        oper_down           = vol_conf.oper_down;

        // If user matches or port has been disabled, use admin status
        if (usr == user || (status->conf.disable == 0 && disable) || (status->conf.disable_adm_recover == 0 && disable_adm_recover)) {
            status->conf.disable = disable;
            status->conf.disable_adm_recover = disable_adm_recover;
            status->user = (port_user_t)usr;
            strcpy(status->name, port_instance_vol_user_txt((port_user_t)usr));
        }

        // If user matches or operational mode is forced, use operational mode
        if (usr == user || oper_up) {
            status->conf.oper_up = oper_up;
        }

        if (usr == user || oper_down) {
            status->conf.oper_down = oper_down;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_ifindex_valid()
/******************************************************************************/
mesa_rc port_ifindex_valid(vtss_ifindex_t ifindex)
{
    vtss_ifindex_elm_t ife;

    if (vtss_ifindex_is_port(ifindex)) {
        // Check that port is within valid range.
        VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
        VTSS_RC(PORT_no_valid(ife.ordinal, false));
        return VTSS_RC_OK;
    }

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
        if (ife.ordinal >= 0 && ife.ordinal < fast_cap(MEBA_CAP_CPU_PORTS_COUNT)) {
            return VTSS_RC_OK;
        }
    }
#endif // VTSS_SW_OPTION_CPUPORT

    return VTSS_APPL_PORT_RC_IFINDEX_NOT_PORT;
}

/******************************************************************************/
// port_is_front_port()
/******************************************************************************/
bool port_is_front_port(mesa_port_no_t port_no)
{
    // Check if port is within the number of ports for the switch.
    return (port_no < port_count_max()          &&
            port_custom_table[port_no].cap != 0 &&
            port_custom_table[port_no].map.chip_port != CHIP_PORT_UNUSED);
}

/******************************************************************************/
// port_count_max()
/******************************************************************************/
uint32_t port_count_max(void)
{
    return PORT_cap.port_cnt;
}

/******************************************************************************/
// port_speed_to_txt()
// Max return if can't be MESA_SPEED_UNDEFINED or unknown: 4 chars.
/******************************************************************************/
const char *port_speed_to_txt(mesa_port_speed_t speed, bool capitals)
{
    switch (speed) {
    case MESA_SPEED_UNDEFINED:
        return capitals ? "UNDEFINED" : "undefined";

    case MESA_SPEED_10M:
        return capitals ? "10M"       : "10";

    case MESA_SPEED_100M:
        return capitals ? "100M"      : "100";

    case MESA_SPEED_1G:
        return capitals ? "1G"        : "1000";

    case MESA_SPEED_2500M:
        return capitals ? "2.5G"      : "2500";

    case MESA_SPEED_5G:
        return capitals ? "5G"        : "5g";

    case MESA_SPEED_10G:
        return capitals ? "10G"       : "10g";

    case MESA_SPEED_12G:
        return capitals ? "12G"       : "12g";

    case MESA_SPEED_25G:
        return capitals ? "25G"       : "25g";

    case MESA_SPEED_AUTO:
        return capitals ? "AUTO"      : "auto";

    default:
        T_E("Unknown speed (%d)", speed);
        return capitals ? "UNKNOWN"   : "unknown";
    }
}

/******************************************************************************/
// port_aneg_method_to_txt()
/******************************************************************************/
const char *port_aneg_method_to_txt(vtss_appl_port_aneg_method_t aneg_method, bool use_consolidated_name)
{
    switch (aneg_method) {
    case VTSS_APPL_PORT_ANEG_METHOD_UNKNOWN:
        // No SFP plugged in.
        return "Unknown";

    case VTSS_APPL_PORT_ANEG_METHOD_NONE:
        return "No";

    case VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_28:
        return use_consolidated_name ? "Yes" : "Clause 28";

    case VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_37:
        return use_consolidated_name ? "Yes" : "Clause 37";

    case VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_73:
        return use_consolidated_name ? "Yes (Cl73)" : "Clause 73";

    default:
        T_E("Unknown aneg_method (%d)", aneg_method);
        return "INVALID";
    }
}

/******************************************************************************/
// port_media_type_to_txt()
/******************************************************************************/
const char *port_media_type_to_txt(vtss_appl_port_media_t media_type, bool capitals)
{
    switch (media_type) {
    case VTSS_APPL_PORT_MEDIA_CU:
        return capitals ? "RJ45"   : "rj45";

    case VTSS_APPL_PORT_MEDIA_SFP:
        return capitals ? "SFP"    : "sfp";

    case VTSS_APPL_PORT_MEDIA_DUAL:
        return capitals ? "Dual"   : "dual";

    default:
        T_E("Unknown media_type (%d)", media_type);
        return capitals ? "UNKNOWN" : "unknown";
    }
}

/******************************************************************************/
// port_fec_mode_to_txt()
/******************************************************************************/
const char *port_fec_mode_to_txt(vtss_appl_port_fec_mode_t fec_mode, bool capitals)
{
    switch (fec_mode) {
    case VTSS_APPL_PORT_FEC_MODE_NONE:
        return capitals ? "None" : "none";

    case VTSS_APPL_PORT_FEC_MODE_R_FEC:
        return capitals ? "R-FEC (Firecode/Clause 74)" : "r-fec";

    case VTSS_APPL_PORT_FEC_MODE_RS_FEC:
        return capitals ? "RS-FEC (Reed-Solomon/Clause 108)" : "rs-fec";

    case VTSS_APPL_PORT_FEC_MODE_AUTO:
        return capitals ? "Auto" : "auto";

    default:
        T_E("Unknown fec_mode (%d)", fec_mode);
        return capitals ? "UNKNOWN" : "unknown";
    }
}

/******************************************************************************/
// port_speed_duplex_to_txt_from_conf()
// strlen(longest) == 7
/******************************************************************************/
const char *port_speed_duplex_to_txt_from_conf(const vtss_appl_port_conf_t &conf)
{
    switch (conf.speed) {
    case MESA_SPEED_10M:
        return conf.fdx ? "10fdx"  : "10hdx";

    case MESA_SPEED_100M:
        return conf.fdx ? "100fdx" : "100hdx";

    case MESA_SPEED_1G:
        return "1Gfdx";

    case MESA_SPEED_2500M:
        return "2.5Gfdx";

    case MESA_SPEED_5G:
        return "5Gfdx";

    case MESA_SPEED_10G:
        return "10Gfdx";

    case MESA_SPEED_25G:
        return "25Gfdx";

    case MESA_SPEED_AUTO:
        return conf.force_clause_73 ? "Cl. 73" : "Auto";

    default:
        T_E("Unknown speed (%d)", conf.speed);
        return "?";
    }
}

/******************************************************************************/
// port_speed_duplex_to_txt_from_status()
/******************************************************************************/
const char *port_speed_duplex_to_txt_from_status(const vtss_appl_port_status_t &status)
{
    switch (status.speed) {
    case MESA_SPEED_10M:
        return status.fiber || status.fdx ? "10fdx"   : "10hdx";

    case MESA_SPEED_100M:
        return status.fiber || status.fdx ? "100fdx"  : "100hdx";

    case MESA_SPEED_1G:
        return status.fiber || status.fdx ? "1Gfdx"   : "1Ghdx";

    case MESA_SPEED_2500M:
        return status.fdx                 ? "2.5Gfdx" : "2.5Ghdx";

    case MESA_SPEED_5G:
        return status.fdx                 ? "5Gfdx"   : "5Ghdx";

    case MESA_SPEED_10G:
        return status.fdx                 ? "10Gfdx"  : "10Ghdx";

    case MESA_SPEED_25G:
        return status.fdx                 ? "25Gfdx"  : "25Ghdx";

    default:
        T_E("Unknown speed (%d)", status.speed);
        return "?";
    }
}

/******************************************************************************/
// port_speed_duplex_link_to_txt()
// strlen(longest) == 7
/******************************************************************************/
const char *port_speed_duplex_link_to_txt(const vtss_appl_port_status_t &status)
{
    switch (status.speed) {
    case MESA_SPEED_10M:
        return status.fdx ? "10fdx" : "10hdx";

    case MESA_SPEED_100M:
        return status.fdx ? "100fdx" : "100hdx";

    case MESA_SPEED_1G:
        return status.fdx ? "1Gfdx" : "1Ghdx";

    case MESA_SPEED_2500M:
        return status.fdx ? "2.5Gfdx" : "2.5Ghdx";

    case MESA_SPEED_5G:
        return status.fdx ? "5Gfdx" : "5Ghdx";

    case MESA_SPEED_10G:
        return status.fdx ? "10Gfdx" : "10Ghdx";

    case MESA_SPEED_25G:
        return status.fdx ? "25Gfdx" : "25Ghdx";

    default:
        T_E("Unknown speed (%d)", status.speed);
        return "?";
    }
}

/******************************************************************************/
// port_veriphy_status_to_txt()
/******************************************************************************/
const char *port_veriphy_status_to_txt(mepa_cable_diag_status_t status)
{
    switch (status) {
    case MEPA_CABLE_DIAG_STATUS_OK:
        return "OK";

    case MEPA_CABLE_DIAG_STATUS_OPEN:
        return "Open";

    case MEPA_CABLE_DIAG_STATUS_SHORT:
        return "Short";

    case MEPA_CABLE_DIAG_STATUS_ABNORM:
        return "Abnormal";

    case MEPA_CABLE_DIAG_STATUS_SHORT_A:
        return "Short A";

    case MEPA_CABLE_DIAG_STATUS_SHORT_B:
        return "Short B";

    case MEPA_CABLE_DIAG_STATUS_SHORT_C:
        return "Short C";

    case MEPA_CABLE_DIAG_STATUS_SHORT_D:
        return "Short D";

    case MEPA_CABLE_DIAG_STATUS_COUPL_A:
        return "Cross A";

    case MEPA_CABLE_DIAG_STATUS_COUPL_B:
        return "Cross B";

    case MEPA_CABLE_DIAG_STATUS_COUPL_C:
        return "Cross C";

    case MEPA_CABLE_DIAG_STATUS_COUPL_D:
        return "Cross D";

    case MEPA_CABLE_DIAG_STATUS_UNKNOWN:
        return "N/A";

    case MEPA_CABLE_DIAG_STATUS_RUNNING:
        return "Still running!";

    default:
        return "?";
    }
}

/******************************************************************************/
// port_sfp_transceiver_to_txt()
/******************************************************************************/
const char *port_sfp_transceiver_to_txt(meba_sfp_transreceiver_t tr)
{
    switch (tr) {
    case MEBA_SFP_TRANSRECEIVER_NONE:
        return "None";

    case MEBA_SFP_TRANSRECEIVER_NOT_SUPPORTED:
        return "Not supported";

    case MEBA_SFP_TRANSRECEIVER_100FX:
        return "100BASE-FX";

    case MEBA_SFP_TRANSRECEIVER_100BASE_LX:
        return "100BASE-LX";

    case MEBA_SFP_TRANSRECEIVER_100BASE_ZX:
        return "100BASE-ZX";

    case MEBA_SFP_TRANSRECEIVER_100BASE_SX:
        return "100BASE-SX";

    case MEBA_SFP_TRANSRECEIVER_100BASE_BX10:
        return "100BASE-BX10";

    case MEBA_SFP_TRANSRECEIVER_100BASE_T:
        return "100BASE-T";

    case MEBA_SFP_TRANSRECEIVER_1000BASE_BX10:
        return "1000BASE-BX10";

    case MEBA_SFP_TRANSRECEIVER_1000BASE_T:
        return "1000BASE-T";

    case MEBA_SFP_TRANSRECEIVER_1000BASE_CX:
        return "1000BASE-CX";

    case MEBA_SFP_TRANSRECEIVER_1000BASE_SX:
        return "1000BASE-SX";

    case MEBA_SFP_TRANSRECEIVER_1000BASE_LX:
        return "1000BASE-LX";

    case MEBA_SFP_TRANSRECEIVER_1000BASE_ZX:
        return "1000BASE-ZX";

    case MEBA_SFP_TRANSRECEIVER_1000BASE_LR:
        return "1000BASE-LR";

    case MEBA_SFP_TRANSRECEIVER_1000BASE_X:
        return "1000BASE-X";

    case MEBA_SFP_TRANSRECEIVER_2G5:
        return "2.5G";

    case MEBA_SFP_TRANSRECEIVER_5G:
        return "5G";

    case MEBA_SFP_TRANSRECEIVER_10G:
        return "10G";

    case MEBA_SFP_TRANSRECEIVER_10G_SR:
        return "10GBASE-SR";

    case MEBA_SFP_TRANSRECEIVER_10G_LR:
        return "10GBASE-LR";

    case MEBA_SFP_TRANSRECEIVER_10G_LRM:
        return "10GBASE-LRM";

    case MEBA_SFP_TRANSRECEIVER_10G_ER:
        return "10GBASE-ER";

    case MEBA_SFP_TRANSRECEIVER_10G_DAC:
        return "10GBASE-CR";

    case MEBA_SFP_TRANSRECEIVER_25G:
        return "25G";

    case MEBA_SFP_TRANSRECEIVER_25G_SR:
        return "25GBASE-SR";

    case MEBA_SFP_TRANSRECEIVER_25G_LR:
        return "25GBASE-LR";

    case MEBA_SFP_TRANSRECEIVER_25G_LRM:
        return "25GBASE-LRM";

    case MEBA_SFP_TRANSRECEIVER_25G_ER:
        return "25GBASE-ER";

    case MEBA_SFP_TRANSRECEIVER_25G_DAC:
        return "25GBASE-CR(-S)";

    default:
        return "Unknown transceiver";
    }
}

/******************************************************************************/
// port_sfp_connector_to_txt()
/******************************************************************************/
const char *port_sfp_connector_to_txt(meba_sfp_connector_t conn)
{
    switch (conn) {
    case MEBA_SFP_CONNECTOR_NONE:
        return "None";
    case MEBA_SFP_CONNECTOR_SC:
        return "Subscriber Conector";
    case MEBA_SFP_CONNECTOR_FC_STYLE_1:
        return "Fiber Channel Style 1 Cu";
    case MEBA_SFP_CONNECTOR_FC_STYLE_2:
        return "Fiber Channel Style 2 Cu";
    case MEBA_SFP_CONNECTOR_BNC_TNC:
        return "Bayonet/Threaded Neill-Concelman";
    case MEBA_SFP_CONNECTOR_FC_COAX:
        return "Fiber Channel Coax headers";
    case MEBA_SFP_CONNECTOR_FJ:
        return "Fiber Jack";
    case MEBA_SFP_CONNECTOR_LC:
        return "Lucent Connector";
    case MEBA_SFP_CONNECTOR_MT_RJ:
        return "Mechanical transfer - RJ";
    case MEBA_SFP_CONNECTOR_MU:
        return "Multiple Optical";
    case MEBA_SFP_CONNECTOR_SG:
        return "SG";
    case MEBA_SFP_CONNECTOR_OP:
        return "Optical Pigtail";
    case MEBA_SFP_CONNECTOR_MPO_1X12:
        return "Multifiber Parallel Optic 1x12";
    case MEBA_SFP_CONNECTOR_MPO_2X16:
        return "Multifiber Parallel Optic 2x16";
    case MEBA_SFP_CONNECTOR_HSSDC_II:
        return "High Speed Serial Data Connector";
    case MEBA_SFP_CONNECTOR_CP:
        return "Copper Pigtail (DAC)";
    case MEBA_SFP_CONNECTOR_RJ45:
        return "RJ45";
    case MEBA_SFP_CONNECTOR_NO_SEP:
        return "No separable connector";
    case MEBA_SFP_CONNECTOR_MXC_2X16:
        return "MXC 2x16";
    case MEBA_SFP_CONNECTOR_CS:
        return "CS optical connector";
    case MEBA_SFP_CONNECTOR_SN:
        return "SN (previously Mini CS) optical connector";
    case MEBA_SFP_CONNECTOR_MPO_2X12:
        return "Multifiber Parallel Optic 2x12";
    case MEBA_SFP_CONNECTOR_MPO_1X16:
        return "Multifiber Parallel Optic 2x16";

    default:
        return "Unknown!";
    }
}

/******************************************************************************/
// port_sfp_type_to_txt()
// Max return = 7 chars
/******************************************************************************/
const char *port_sfp_type_to_txt(vtss_appl_port_sfp_type_t sfp_type)
{
    switch (sfp_type) {
    case VTSS_APPL_PORT_SFP_TYPE_NONE:
        return "None";

    case VTSS_APPL_PORT_SFP_TYPE_UNKNOWN:
        return "Unknown";

    case VTSS_APPL_PORT_SFP_TYPE_OPTICAL:
        return "Optical";

    case VTSS_APPL_PORT_SFP_TYPE_CU:
        return "CuSFP";

    case VTSS_APPL_PORT_SFP_TYPE_CU_BP:
        return "CuBP";

    case VTSS_APPL_PORT_SFP_TYPE_DAC:
        return "DAC";

    default:
        return "ERROR!";
    }
}

/******************************************************************************/
// port_sfp_type_speed_to_txt()
// sizeof(buf) >= 4 + 1 + 7 = 12 + terminating null = 13.
/******************************************************************************/
char *port_sfp_type_speed_to_txt(char *buf, size_t size, vtss_appl_port_status_t &status)
{
    if (status.sfp_type == VTSS_APPL_PORT_SFP_TYPE_NONE) {
        // SFP type is not applicable on pure copper ports.
        strncpy(buf, status.static_caps & MEBA_PORT_CAP_COPPER ? "N/A" : "None", size);
    } else {
        snprintf(buf, size, "%s %s", port_speed_to_txt(status.sfp_speed_max), port_sfp_type_to_txt(status.sfp_type));
    }

    return buf;
}

/******************************************************************************/
// port_cap_to_txt()
// Buf must be ~400 bytes long if all bits are set.
/******************************************************************************/
char *port_cap_to_txt(char *buf, size_t size, meba_port_cap_t cap)
{
    int  s = 0;
    bool first = true;

#define P(...)                                              \
    if (size - s > 0) {                                     \
        int res = snprintf(buf + s, size - s, __VA_ARGS__); \
        if (res > 0) {                                      \
            s += res;                                       \
        }                                                   \
    }

#define F(X)                       \
    if (cap & MEBA_PORT_CAP_##X) { \
        cap &= ~MEBA_PORT_CAP_##X; \
        if (first) {               \
            first = false;         \
            P(#X);                 \
        } else {                   \
            P(" " #X);             \
        }                          \
    }

    *buf = 0;
    F(AUTONEG);
    F(10M_HDX);
    F(10M_FDX);
    F(100M_HDX);
    F(100M_FDX);
    F(1G_FDX);
    F(2_5G_FDX);
    F(5G_FDX);
    F(10G_FDX);
    F(25G_FDX);
    F(FLOW_CTRL);
    F(COPPER);
    F(FIBER);
    F(DUAL_COPPER);
    F(DUAL_FIBER);
    F(SD_ENABLE);
    F(SD_HIGH);
    F(SD_INTERNAL);
    F(XAUI_LANE_FLIP);
    F(VTSS_10G_PHY);
    F(SFP_DETECT);
    F(STACKING);
    F(DUAL_SFP_DETECT);
    F(SFP_ONLY);
    F(DUAL_NO_COPPER);
    F(SERDES_RX_INVERT);
    F(SERDES_TX_INVERT);
    F(INT_PHY);
    F(NO_FORCE);
    F(CPU);
    F(SFP_INACCESSIBLE);

#undef F
#undef P

    if (cap != 0) {
        T_E("Not all port capabilities are handled. Missing = 0x%x", cap);
    }

    buf[MIN(size - 1, s)] = 0;
    return buf;
}

/******************************************************************************/
// port_adv_dis_to_txt()
// Buf must be ~50 bytes long if all bits are set.
/******************************************************************************/
char *port_adv_dis_to_txt(char *buf, size_t size, mepa_adv_dis_t adv_dis)
{
    int  s = 0;
    bool first = true;

#define P(...)                                              \
    if (size - s > 0) {                                     \
        int res = snprintf(buf + s, size - s, __VA_ARGS__); \
        if (res > 0) {                                      \
            s += res;                                       \
        }                                                   \
    }

#define F(X)                          \
    if (adv_dis & MEPA_ADV_DIS_##X) { \
        if (first) {                  \
            first = false;            \
            P(#X);                    \
        } else {                      \
            P(" " #X);                \
        }                             \
    }

    *buf = 0;
    F(HDX);
    F(FDX);
    F(10M);
    F(100M);
    F(1G);
    F(2500M);
    F(5G);
    F(10G)
    F(RESTART_ANEG)
#undef F
#undef P

    buf[MIN(size - 1, s)] = 0;
    return buf;
}

/******************************************************************************/
// port_oper_warnings_to_txt()
// Buf must be ~200 bytes long if all bits are set.
/******************************************************************************/
char *port_oper_warnings_to_txt(char *buf, size_t size, vtss_appl_port_oper_warnings_t oper_warnings)
{
    int  s = 0, res;
    bool first = true;

#define P(_str_)                                        \
    if (size - s > 0) {                                 \
        res = snprintf(buf + s, size - s, "%s", _str_); \
        if (res > 0) {                                  \
            s += res;                                   \
        }                                               \
    }

#define F(X, _name_)                                       \
    if (oper_warnings & VTSS_APPL_PORT_OPER_WARNING_##X) { \
        oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_##X; \
        if (first) {                                       \
            first = false;                                 \
            P(_name_);                                     \
        } else {                                           \
            P(", " _name_);                                \
        }                                                  \
    }

    buf[0] = 0;
    s = 0;
    // Example of a field name (just so that we can search for this function):
    // VTSS_APPL_PORT_OPER_WARNING_CU_SFP_IN_DUAL_MEDIA_SLOT
    F(CU_SFP_IN_DUAL_MEDIA_SLOT,                "This dual media port does not support CuSFPs");
    F(CU_SFP_SPEED_AUTO,                        "CuSFPs require speed auto");
    F(FORCED_SPEED_NOT_SUPPORTED_BY_SFP,        "This SFP does not support the configured, forced speed");
    F(PORT_DOES_NOT_SUPPORT_SFP,                "The port's minimum speed is higher than the SFP's maximum speed");
    F(SFP_NOMINAL_SPEED_HIGHER_THAN_PORT_SPEED, "SFP's nominal speed is higher than actual speed, which may cause instability");
    F(SFP_CANNOT_RUN_CLAUSE_73,                 "This SFP cannot run clause 73 aneg (which is forced)");
    F(SFP_UNREADABLE_IN_THIS_PORT,              "SFP type cannot be determined on this interface. Instability can be expected");
    F(SFP_READ_FAILED,                          "SFP is not readable. Please replace or expect instability");
    F(SFP_DOES_NOT_SUPPORT_HDX,                 "SFP does not support half duplex");
    F(CANNOT_OBEY_ANEG_FC_WHEN_PFC_ENABLED,     "Cannot obey aneg-enabled flowcontrol when priority-based flowcontrol is enabled");
    buf[MIN(size - 1, s)] = 0;
#undef F
#undef P

    if (oper_warnings != 0) {
        T_E("Not all operational warnings are handled. Missing = 0x%x", oper_warnings);
    }

    return buf;
}

/******************************************************************************/
// port_error_txt()
/******************************************************************************/
const char *port_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_PORT_RC_VERIPHY_RUNNING:
        return "VeriPHY still running";

    case VTSS_APPL_PORT_RC_PARM:
        return "Illegal parameter";

    case VTSS_APPL_PORT_RC_REG_TABLE_FULL:
        return "Registration table full";

    case VTSS_APPL_PORT_RC_REQ_TIMEOUT:
        return "Timeout on message request";

    case VTSS_APPL_PORT_RC_MUST_BE_PRIMARY_SWITCH:
        return "This is not allow at secondary switch. Switch must be the primary";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_MASK:
        return "Illegal advertise disable bitmask (bits set outside of valid bits)";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_HDX_AND_FDX:
        return "Advertisement of both half and full duplex cannot be disabled simultaneously";

    case VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_HDX:
        return "Advertisement of half duplex cannot be enabled, because port doesn't support half duplex";

    case VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_10M:
        return "Advertisement of 10 Mbps cannot be enabled, because port doesn't support that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_100M:
        return "Advertisement of 100 Mbps cannot be enabled, because port doesn't support that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_1G:
        return "Advertisement of 1 Gbps cannot be enabled, because port doesn't support that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_2500M:
        return "Advertisement of 2.5 Gbps cannot be enabled, because port doesn't support that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_5G:
        return "Advertisement of 5 Gbps cannot be enabled, because port doesn't support that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_10G:
        return "Advertisement of 5 Gbps cannot be enabled, because port doesn't support that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_HDX:
        return "Advertisement of half duplex cannot be disabled on SFP and dual media ports when the port supports half duplex";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_FDX:
        return "Advertisement of full duplex cannot be disabled on SFP and dual media ports";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_10M:
        return "Advertisement of 10 Mbps cannot be disabled on SFP and dual media ports when the port supports that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_100M:
        return "Advertisement of 100 Mbps cannot be disabled on SFP and dual media ports when the port supports that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_1G:
        return "Advertisement of 1 Gbps cannot be disabled on SFP and dual media ports when the port supports that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_2500M:
        return "Advertisement of 2.5 Gbps cannot be disabled on SFP and dual media ports when the port supports that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_5G:
        return "Advertisement of 5 Gbps cannot be disabled on SFP and dual media ports when the port supports that speed";

    case VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_10G:
        return "Advertisement of 10 Gbps cannot be disabled on SFP and dual media ports when the port supports that speed";

    case VTSS_APPL_PORT_RC_MTU:
        return "MTU is outside the valid range";

    case VTSS_APPL_PORT_RC_EXCESSIVE_COLLISSION:
        return "Excessive collission restart cannot be enabled because the interface does not support half duplex";

    case VTSS_APPL_PORT_RC_INVALID_PORT:
        return "Port number is out of range";

    case VTSS_APPL_PORT_RC_LOOP_PORT:
        return "Loop ports cannot be configured";

    case VTSS_APPL_PORT_RC_MEDIA_TYPE_CU:
        return "Copper media type is not supported on this interface";

    case VTSS_APPL_PORT_RC_MEDIA_TYPE_SFP:
        return "SFP media type not supported on this interface";

    case VTSS_APPL_PORT_RC_MEDIA_TYPE_DUAL:
        return "Interface is not a dual media interface";

    case VTSS_APPL_PORT_RC_MEDIA_TYPE:
        return "Unknown media type enumeration";

    case VTSS_APPL_PORT_RC_SPEED_DUAL_MEDIA_SFP:
        return "Dual media ports configured as SFP ports must have a speed higher than 10 Mbps";

    case VTSS_APPL_PORT_RC_SPEED_DUAL_MEDIA_AUTO:
        return "Dual media ports configured as dual media must be set to auto speed with all options advertised";

    case VTSS_APPL_PORT_RC_DUPLEX_HALF:
        return "Port does not support half duplex";

    case VTSS_APPL_PORT_RC_SPEED_NO_FORCE:
        return "Port does not support forced speed, only auto";

    case VTSS_APPL_PORT_RC_SPEED_NO_AUTO:
        return "Port does not support auto, only forced speed";

    case VTSS_APPL_PORT_RC_SPEED_10M_FDX:
        return "10 Mbps full duplex not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_10M_HDX:
        return "10 Mbps half duplex not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_100M_FDX:
        return "100 Mbps full duplex not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_100M_HDX:
        return "100 Mbps half duplex not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_1G:
        return "1 Gbps not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_1G_HDX:
        return "1 Gbps ports do not support half duplex";

    case VTSS_APPL_PORT_RC_SPEED_2G5:
        return "1 Gbps not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_2G5_HDX:
        return "2.5 Gbps ports do not support half duplex";

    case VTSS_APPL_PORT_RC_SPEED_5G:
        return "5 Gbps not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_5G_HDX:
        return "5 Gbps ports do not support half duplex";

    case VTSS_APPL_PORT_RC_SPEED_10G:
        return "10 Gbps not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_10G_HDX:
        return "10 Gbps ports do not support half duplex";

    case VTSS_APPL_PORT_RC_SPEED_25G:
        return "25 Gbps not supported on this interface";

    case VTSS_APPL_PORT_RC_SPEED_25G_HDX:
        return "25 Gbps ports do not support half duplex";

    case VTSS_APPL_PORT_RC_SPEED:
        return "Unknown speed enumeration";

    case VTSS_APPL_PORT_RC_IFINDEX_NOT_PORT:
        return "Interface is not a port interface";

    case VTSS_APPL_PORT_RC_FLOWCONTROL:
        return "Flow control is not supported on this interface";

    case VTSS_APPL_PORT_RC_FLOWCONTROL_WHILE_PFC:
        return "Standard and priority flowcontrol cannot both be enabled";

    case VTSS_APPL_PORT_RC_FLOWCONTROL_PFC:
        return "Priority flowcontrol is not supported on this interface";

    case VTSS_APPL_PORT_RC_KR_NOT_SUPPORTED_ON_THIS_PLATFORM:
        return "This platform does not support clause 73 aneg (KR)";

    case VTSS_APPL_PORT_RC_KR_NOT_SUPPORTED_ON_THIS_INTERFACE:
        return "This interface does not support clause 73 aneg (KR)";

    case VTSS_APPL_PORT_RC_KR_FORCED_REQUIRES_SPEED_AUTO:
        return "When interface is forced into clause 73 aneg (KR), the speed must be set to 'AUTO'";

    case VTSS_APPL_PORT_RC_FEC_ILLEGAL:
        return "Illegal/unknown FEC mode selected";

    case VTSS_APPL_PORT_RC_FEC_R_NOT_SUPPORTED_ON_THIS_PLATFORM:
        return "This platform does not support clause 74 Forward Error Correction (R-FEC/Firecode)";

    case VTSS_APPL_PORT_RC_FEC_RS_NOT_SUPPORTED_ON_THIS_PLATFORM:
        return "This platform does not support clause 108 Forward Error Correction (Reed-Solomon-FEC)";

    case VTSS_APPL_PORT_RC_FEC_R_NOT_SUPPORTED_ON_THIS_INTERFACE:
        return "This interface does not support clause 74 Forward Error Correction (R-FEC/Firecode)";

    case VTSS_APPL_PORT_RC_FEC_RS_NOT_SUPPORTED_ON_THIS_INTERFACE:
        return "This interface does not support clause 108 Forward Error Correction (Reed-Solomon-FEC)";

    case VTSS_APPL_PORT_RC_ICLI_DUPLEX_AUTO_OBSOLETE:
        return "'auto' keyword no longer supported. Use 'speed auto {[no-hdx] | [no-fdx]}' instead";

    default:
        T_E("Unknown error code (0x%x)", rc);
        return "Unknown port error";
    }
}

/******************************************************************************/
// port_phy_reset()
/******************************************************************************/
void port_phy_reset(mesa_port_no_t port_no)
{
    if (PORT_no_valid(port_no) != VTSS_RC_OK) {
        return;
    }

    port_instances[port_no].phy_reset();
}

/******************************************************************************/
// port_has_phy()
// Determines if port has a PHY.
/******************************************************************************/
bool port_has_phy(mesa_port_no_t port_no)
{
    if (PORT_no_valid(port_no) != VTSS_RC_OK) {
        return false;
    }

    return port_instances[port_no].has_phy();
}

/******************************************************************************/
// port_is_phy()
// Returns if a port is PHY (if the PHY is in pass through mode, it shall act as
// it is not a PHY).
/******************************************************************************/
bool port_is_phy(mesa_port_no_t port_no)
{
    if (PORT_no_valid(port_no) != VTSS_RC_OK) {
        return false;
    }

    return port_instances[port_no].is_phy();
}

/******************************************************************************/
// port_is_10g_copper_phy()
/******************************************************************************/
bool port_is_10g_copper_phy(mesa_port_no_t port_no)
{
    meba_port_cap_t cap;

    if (PORT_no_valid(port_no) != VTSS_RC_OK) {
        return false;
    }

    VTSS_RC_ERR_PRINT(port_cap_get(port_no, &cap));
    if (((cap & (MEBA_PORT_CAP_10G_FDX | MEBA_PORT_CAP_5G_FDX)) && (cap & MEBA_PORT_CAP_COPPER))
        || ((cap & MEBA_PORT_CAP_2_5G_FDX) && (cap & MEBA_PORT_CAP_COPPER)
            && port_custom_table[port_no].mac_if == MESA_PORT_INTERFACE_SFI)) {
        /* we support 2 kind of 2.5G PHY: one use SFI interface,
         another uses SGMII, we only care about the PHY using SFI interface here. */
        return true;
    }

    return false;
}

/******************************************************************************/
// port_phy_wait_until_ready()
/******************************************************************************/
void port_phy_wait_until_ready(void)
{
    // All PHYs have been reset when the port module is ready
    PORT_LOCK_SCOPE();
}

/******************************************************************************/
// port_cap_get()
/******************************************************************************/
mesa_rc port_cap_get(mesa_port_no_t port_no, meba_port_cap_t *cap)
{
    if (port_no >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        return VTSS_APPL_PORT_RC_PARM;
    }

    if (cap == nullptr) {
        return VTSS_APPL_PORT_RC_PARM;
    }

    *cap = port_instances[port_no].static_caps_get() != 0 ? port_instances[port_no].static_caps_get() : port_custom_table[port_no].cap;
    return MESA_RC_OK;
}

/******************************************************************************/
// port_phy_cap_check()
/******************************************************************************/
bool port_phy_cap_check(mepa_phy_cap_t cap)
{
    return (port.phy_cap & cap ? true : false);
}

/******************************************************************************/
// port_cap_to_advertise_dis()
/******************************************************************************/
mepa_adv_dis_t port_cap_to_advertise_dis(meba_port_cap_t port_cap)
{
    mepa_adv_dis_t adv_dis = (mepa_adv_dis_t)0;

    if (!(port_cap & MEBA_PORT_CAP_HDX)) {
        // Disable half duplex advertisement as the port does not support it
        adv_dis |= MEPA_ADV_DIS_HDX;
    }

    if (!(port_cap & (MEBA_PORT_CAP_10M_FDX | MEBA_PORT_CAP_10M_HDX))) {
        // Disable 10M advertisement as the port does not support it
        adv_dis |= MEPA_ADV_DIS_10M;
    }

    if (!(port_cap & (MEBA_PORT_CAP_100M_FDX | MEBA_PORT_CAP_100M_HDX))) {
        // Disable 100M advertisement as the port does not support it
        adv_dis |= MEPA_ADV_DIS_100M;
    }

    if (!(port_cap & MEBA_PORT_CAP_1G_FDX)) {
        // Disable 1G advertisement as the port does not support it
        adv_dis |= MEPA_ADV_DIS_1G;
    }

    if (!(port_cap & MEBA_PORT_CAP_2_5G_FDX)) {
        // Disable 2.5G advertisement as the port does not support it
        adv_dis |= MEPA_ADV_DIS_2500M;
    }

    if (!(port_cap & MEBA_PORT_CAP_5G_FDX)) {
        // Disable 5G advertisement as the port does not support it
        adv_dis |= MEPA_ADV_DIS_5G;
    }

    if (!(port_cap & MEBA_PORT_CAP_10G_FDX)) {
        // Disable 10G advertisement as the port does not support it
        adv_dis |= MEPA_ADV_DIS_10G;
    }

    return adv_dis;
}

/******************************************************************************/
// port_kr_completeness_set()
// Called by KR module whenever a state change on Aneg completion occurs.
/******************************************************************************/
void port_kr_completeness_set(mesa_port_no_t port_no, bool complete)
{
    // Do NOT take port lock, because it may happen that the port instance is
    // now calling into KR to e.g. stop KR, and KR then calls back into the
    // port module. Since we only set a boolean, I don't consider this a risk.
    port_instances[port_no].kr_completeness_set(complete);
}

/******************************************************************************/
// port_debug_state_dump()
/******************************************************************************/
mesa_rc port_debug_state_dump(vtss_ifindex_t ifindex, uint32_t session_id, port_icli_pr_t pr)
{
    mesa_port_no_t port_no;

    VTSS_RC(port_ifindex_valid(ifindex));

#ifdef VTSS_SW_OPTION_CPUPORT
    if (vtss_ifindex_is_cpu(ifindex)) {
        return VTSS_RC_ERROR;
    }
#endif // VTSS_SW_OPTION_CPUPORT

    // Normal switch port
    VTSS_RC(PORT_ifindex_to_port(ifindex, port_no));

    // The function locks the port mutex itself
    port_instances[port_no].debug_state_dump(session_id, pr);
    return VTSS_RC_OK;
}

mesa_rc port_man_neg_set(mesa_port_no_t port_no, mepa_manual_neg_t man_neg)
{
    PORT_LOCK_SCOPE();

    return port_instances[port_no].phy_man_neg_set(__FUNCTION__, __LINE__, man_neg);
}

mesa_rc port_man_neg_get(mesa_port_no_t port_no, mepa_manual_neg_t *man_neg)
{
    PORT_LOCK_SCOPE();

    *man_neg = port_instances[port_no].phy_man_neg_get(__FUNCTION__, __LINE__);
    return VTSS_RC_OK;
}

extern "C" {
    int  port_icli_cmd_register(void);
    void vtss_appl_port_json_init(void);
    void port_mib_init(void);
    int  port_power_savings_icli_cmd_register(void);
    void vtss_appl_port_power_savings_mib_init(void);
    void vtss_appl_port_power_savings_json_init(void);
}

mesa_rc port_icfg_init(void);

/******************************************************************************/
// port_init()
// Initialize module
/******************************************************************************/
mesa_rc port_init(vtss_init_data_t *data)
{
    static bool init_cmd_start_already_invoked;





    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_I("INIT");

        // Initialize the constant port module capabilities.
        PORT_capabilities_set();

        PORT_sfp_drivers_init();

        // Create main mutex, and create it locked (hence _legacy()).
        critd_init_legacy(&port.crit, "port", VTSS_MODULE_ID_PORT, CRITD_TYPE_MUTEX);

        /* Create critical region protecting callbacks and their registrations  */
        critd_init(&port.cb_crit, "port.cb", VTSS_MODULE_ID_PORT, CRITD_TYPE_MUTEX);

        /* Create event flag for interrupt to signal and PORT_thread to wait for */
        vtss_flag_init(&interrupt_wait_flag);

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register our private mib */
        port_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_port_json_init();
#endif

        port_icli_cmd_register();

#ifdef VTSS_SW_OPTION_PORT_POWER_SAVINGS

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register our private mib */
        vtss_appl_port_power_savings_mib_init();
#endif // endif VTSS_SW_OPTION_PRIVATE_MIB

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_port_power_savings_json_init();
#endif //endif VTSS_SW_OPTION_JSON_RPC

        port_power_savings_icli_cmd_register();

#endif //endif VTSS_SW_OPTION_PORT_POWER_SAVINGS

#ifdef VTSS_SW_OPTION_ICFG
        // Initialize ICLI "show running" configuration
        VTSS_RC(port_icfg_init());

#if defined(VTSS_SW_OPTION_PORT_POWER_SAVINGS)
        mesa_rc port_power_savings_icfg_init(void);
        VTSS_RC(port_power_savings_icfg_init());
#endif
#endif
        break;

    case INIT_CMD_START:
        T_I("START");

        if (init_cmd_start_already_invoked) {
            T_E("INIT_CMD_START is already invoked");
            break;
        }

        init_cmd_start_already_invoked = true;

        port.thread_lock.lock(false);

        /* The switch API has been set up by the vtss_api module */
        VTSS_ASSERT(vtss_board_type() != VTSS_BOARD_UNKNOWN);
        PORT_init_ports();

        /* Create thread */
        VTSS_ASSERT(PORT_cap.port_cnt > 0);
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           PORT_thread,
                           0,
                           "Port Control",
                           nullptr,
                           0,
                           &port.thread_handle,
                           &port.thread_block);

        break;

    case INIT_CMD_SUSPEND_RESUME:
        // This is a feature that allows for suspending polling of PHYs and
        // SFPs through the use of 'debug suspend' and 'debug resume' CLI
        // commands.
        port.thread_lock.lock(!data->resume);
        break;

    case INIT_CMD_CONF_DEF:
        T_I("CONF_DEF");

        if (data->isid == VTSS_ISID_GLOBAL) {
            PORT_reload_defaults();
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}
