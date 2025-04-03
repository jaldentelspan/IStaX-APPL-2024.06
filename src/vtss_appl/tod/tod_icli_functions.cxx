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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "main.h"
#include "icli_porting_util.h"
#include "tod_icli_functions.h"
#include "tod_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "vtss_tod_api.h"
#include "vtss_tod_mod_man.h"
#include "misc_api.h"
#if defined(VTSS_SW_OPTION_PTP)
#include "ptp_api.h"
#endif

/***************************************************************************/
/*  Internal types                                                         */
/***************************************************************************/
/*
 * phy_ts configuration.
 */
typedef struct {
    BOOL enable;
    BOOL disable;
} vtss_tod_phy_ts_conf_t;

/*
 * engine configuration.
 */
typedef struct {
    BOOL set;
} vtss_tod_engine_conf_t;

/*
 * phy monitor configuration.
 */
typedef struct {
    BOOL enable;
    BOOL disable;
} vtss_tod_phy_monitor_conf_t;


/***************************************************************************/
/*  Internal functions                                                     */
/***************************************************************************/

static icli_rc_t tod_icli_config_traverse_ports(i32 session_id, int clockinst,
                                           icli_stack_port_range_t *port_type_list_p, void *port_cfg,
                                           icli_rc_t (*cfg_function)(i32 session_id, int inst, mesa_port_no_t uport, void *cfg))
{
    icli_rc_t      rc = ICLI_RC_OK;
    u32            range_idx, cnt_idx;
    mesa_port_no_t uport;
    switch_iter_t  sit;
    port_iter_t    pit;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                if (ICLI_RC_OK != cfg_function(session_id, clockinst, uport, port_cfg)) {
                    rc = ICLI_RC_ERROR;
                }
            }
        }
    } else { //show all port configuration
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);
        while (switch_iter_getnext(&sit)) {
            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (ICLI_RC_OK != cfg_function(session_id, clockinst, pit.uport, port_cfg)) {
                    rc = ICLI_RC_ERROR;
                }
            }
        }
    }
    return rc;
}

/***************************************************************************/
/*  Functions called by iCLI                                               */
/***************************************************************************/

BOOL tod_icli_runtime_phy_timestamping_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (mepa_phy_ts_cap()) {
                runtime->present = TRUE;
            } else {
                runtime->present = FALSE;
            }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

static icli_rc_t my_tod_icli_debug_phy_ts(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_tod_phy_ts_conf_t *conf = (vtss_tod_phy_ts_conf_t *)cfg;
    mesa_port_no_t     port_no;
    BOOL enabled;

    port_no = uport2iport(uport);
    if(conf->enable || conf->disable) {
        if(!tod_port_phy_ts_set(&conf->enable, port_no)) {
            ICLI_PRINTF("Failed to set the phy ts mode for port %d\n", uport);
        } else {
            ICLI_PRINTF("Port %d: Successfully %s the phy ts mode\n", uport, conf->enable ? "enable" : "disable");
        }
    } else {
        if(!tod_port_phy_ts_get(&enabled, port_no)) {
            ICLI_PRINTF("Failed to get the the phy ts mode for port %d\n", uport);
        } else {
            ICLI_PRINTF("Phy ts mode port %d = %s\n", uport, enabled ? "enable" : "disable");
        }
    }
    return rc;
}

icli_rc_t tod_icli_debug_phy_ts(i32 session_id, BOOL has_enable, BOOL has_disable, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_tod_phy_ts_conf_t conf;
    conf.enable = has_enable;
    conf.disable = has_disable;
    rc = tod_icli_config_traverse_ports(session_id, 0, v_port_type_list, &conf, my_tod_icli_debug_phy_ts);
    return rc;
}


static icli_rc_t my_tod_icli_debug_tod_monitor(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_tod_phy_monitor_conf_t *conf = (vtss_tod_phy_monitor_conf_t *)cfg;
    mesa_port_no_t     port_no;

    port_no = uport2iport(uport);
    if(conf->enable || conf->disable) {
        if (VTSS_RC_OK == tod_mod_man_force_slave_state(port_no, conf->enable)) {
            ICLI_PRINTF("%s monitoring of port %d\n", conf->enable ? "Enable" : "Disable", uport);
        } else {
            ICLI_PRINTF("Enable/disable failed\n");
        }
    }
    return rc;
}

icli_rc_t tod_icli_debug_tod_monitor(i32 session_id, BOOL has_enable, BOOL has_disable, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_tod_phy_monitor_conf_t conf;
    conf.enable = has_enable;
    conf.disable = has_disable;
    rc = tod_icli_config_traverse_ports(session_id, 0, v_port_type_list, &conf, my_tod_icli_debug_tod_monitor);
    return rc;
}

static char* _tod_icli_debug_state_2_string(mod_man_state_t state)
{
    if (state == MOD_MAN_NOT_STARTED) {
        return ("INACTIVE");
    } else if (state == MOD_MAN_DISABLED) {
        return ("DISABLED");
    } else if (state == MOD_MAN_READ_RTC_FROM_SWITCH) {
        return ("RD_MSTR");
    } else if (state == MOD_MAN_SET_ABS_TOD) {
        return ("SETPHY_0");
    } else if (state == MOD_MAN_SET_ABS_TOD_DONE) {
        return ("SETPHY_1");
    } else if (state == MOD_MAN_READ_RTC_FROM_PHY_AND_COMPARE) {
        return ("RDPHY*");
    } else {
        return ("N/A");
    }
}

icli_rc_t tod_icli_debug_status_show(i32 session_id, BOOL has_interface, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;

    mepa_timestamp_t slave_time;
    mepa_timestamp_t next_pps;
    i64 offset;
    bool in_sync;
    mesa_port_no_t port_no;
    mod_man_state_t state;
    port_iter_t          pit;
    int remain_time_until_start;
    int max_offset;
    int mod_man_running;
    uint32_t oos_cnt;
    int64_t  oos_skew;

    (void) tod_mod_man_gen_status_get(&mod_man_running, &remain_time_until_start, &max_offset);

    if (mod_man_running == MOD_MAN_NOT_INSTALLED) {
        ICLI_PRINTF("TOD module manager not running (PHY is not ts capable)\n");
    } else if (mod_man_running == MOD_MAN_INSTALLED_BUT_NOT_RUNNING) {
        ICLI_PRINTF("TOD module-manager inactive for next %d seconds\n", (int)(remain_time_until_start/1000));
    } else if (mod_man_running == MOD_MAN_INSTALLED_AND_RUNNING) {
        ICLI_PRINTF("Port State    Phy [s:ns]           Mstr [s:ns]          In-sync Offset[ns] OutOfSyncCnt Last OOS Skew\n");
        ICLI_PRINTF("---- -------- -------------------- -------------------- ------- ---------- ------------ -------------\n");
        (void) port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
        while (icli_port_iter_getnext(&pit, has_interface ? v_port_type_list : 0)) {
            port_no = (mesa_port_no_t)pit.uport;
            if (tod_mod_man_port_status_get(uport2iport(port_no), &slave_time, &next_pps, &offset, &in_sync, &state, &oos_cnt, &oos_skew) == VTSS_RC_OK) {
                ICLI_PRINTF("%4d %-8s %10d:%09d %10d:%09d %-7s " VPRI64Fd("10") " %12u " VPRI64Fd("13") "\n",
                            port_no,
                            _tod_icli_debug_state_2_string(state),
                            ((slave_time.seconds.high << 16) + slave_time.seconds.low), slave_time.nanoseconds,
                            ((next_pps.seconds.high << 16) + next_pps.seconds.low), next_pps.nanoseconds,
                            in_sync ? "TRUE" : "FALSE", offset, oos_cnt, oos_skew);
            }
        }
        ICLI_PRINTF("Max offset allowed: +/-%d ns\n", max_offset);
    } else {
        // invalid state (should not happen)
        rc = ICLI_RC_ERROR;
    }

    return rc;

}

