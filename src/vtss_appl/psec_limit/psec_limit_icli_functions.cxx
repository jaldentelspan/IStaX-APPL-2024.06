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

#include "icli_api.h"
#include "icli_porting_util.h"
#include "psec_limit_icli_functions.h"
#include "psec_limit_api.h"
#include "psec_limit_trace.h"
#include "misc_api.h" /* For misc_mac_txt() */
#include <vtss/appl/interface.h>
#include <vtss/appl/psec.h>
#include "msg_api.h"

#if defined(VTSS_SW_OPTION_ICFG)
#include "icfg_api.h"
#endif

/***************************************************************************/
/*  Type defines                                                           */
/***************************************************************************/
// Enum for selecting which status or configuration to access
typedef enum {
    ENABLE,            // Enable port security
    MAXIMUM,           // Configure max. number of MAC addresses that can be learned on this set of ports
    MAXIMUM_VIOLATION, // Configure max. number of violating MAC addresses.
    VIOLATION,         // Configure the violation_mode
    AGING,             // Configure aging
    AGING_TIME,        // Configure aging time
    HOLD_TIME,         // Configure hold time
    STICKY,            // Configure stickiness
    MAC,               // Configure static MAC address
} psec_limit_cmd_type_t;

// Used to pass the configuration to be changed or status information to display
typedef struct {
    psec_limit_cmd_type_t type;          // Which configuration to access

    BOOL no; // "invert" the command (User no command)

    // New values
    union {
        u32                             limit;
        u32                             violate_limit;
        vtss_appl_psec_violation_mode_t violation_mode;
        u32                             aging_period_secs;
        u32                             hold_time_secs;
        vtss_appl_psec_mac_conf_t       mac_conf;
    } value;
} psec_limit_cmd_t;

/***************************************************************************/
/*  Internal functions                                                     */
/****************************************************************************/
// Function for configuring global configuration
// In : session_id - For printing
//      cmd        - Containing information about which function to call.
static mesa_rc psec_limit_icli_global_conf(i32 session_id, const psec_limit_cmd_t *cmd)
{
    vtss_appl_psec_global_conf_t global_conf;
    vtss_appl_psec_global_conf_t global_conf_default;

    // Get current configuration
    VTSS_RC(vtss_appl_psec_global_conf_get(&global_conf));

    // Get default configuration values.
    VTSS_RC(vtss_appl_psec_global_conf_default_get(&global_conf_default));

    switch (cmd->type) {
    case AGING:
        global_conf.enable_aging = !cmd->no;
        break;

    case AGING_TIME:
        global_conf.aging_period_secs = cmd->no ? global_conf_default.aging_period_secs : cmd->value.aging_period_secs;
        break;

    case HOLD_TIME:
        global_conf.hold_time_secs = cmd->no ? global_conf_default.hold_time_secs : cmd->value.hold_time_secs;
        break;

    default:
        T_E("Un-expected type:%d", cmd->type);
        break;
    }

    // Set new configuration
    VTSS_RC(vtss_appl_psec_global_conf_set(&global_conf));

    return VTSS_RC_OK;
}

// Function for looping over all switches and all ports a the port list, and the calling a configuration or status/statistics function.
// In : session_id - For printing
//      plist      - Containing information about which switches and ports to "access"
//      cmd        - Containing information about which function to call.
static mesa_rc psec_limit_icli_sit_port_loop(const i32 session_id, icli_stack_port_range_t *plist, const psec_limit_cmd_t *cmd)
{
    switch_iter_t                   sit;
    vtss_appl_psec_interface_conf_t port_conf;
    vtss_appl_psec_interface_conf_t port_conf_default;
    vtss_ifindex_t                  ifindex;

    // Get default configuration values.
    VTSS_RC(vtss_appl_psec_interface_conf_default_get(&port_conf_default));

    // Loop through all switches in a stack.
    // For all commands, the switch needs to be configurable.
    VTSS_RC(icli_switch_iter_init(&sit));

    while (icli_switch_iter_getnext(&sit, plist)) {
        port_iter_t pit;
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL));

        // Loop through all ports
        while (icli_port_iter_getnext(&pit, plist)) {

            VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));

            if (cmd->type == MAC) {
                if (cmd->no) {
                    return vtss_appl_psec_interface_conf_mac_del(ifindex, cmd->value.mac_conf.vlan, cmd->value.mac_conf.mac);
                } else {
                    return vtss_appl_psec_interface_conf_mac_add(ifindex, cmd->value.mac_conf.vlan, cmd->value.mac_conf.mac, &cmd->value.mac_conf);
                }
            }

            // Get current interface configuration.
            VTSS_RC(vtss_appl_psec_interface_conf_get(ifindex, &port_conf));

            switch (cmd->type) {
            case ENABLE:
                port_conf.enabled = !cmd->no;
                break;

            case MAXIMUM:
                port_conf.limit = cmd->no ? port_conf_default.limit : cmd->value.limit;
                break;

            case MAXIMUM_VIOLATION:
                port_conf.violate_limit = cmd->no ? port_conf_default.violate_limit : cmd->value.violate_limit;
                break;

            case VIOLATION:
                port_conf.violation_mode = cmd->no ? port_conf_default.violation_mode : cmd->value.violation_mode;
                break;

            case STICKY:
                port_conf.sticky = !cmd->no;
                break;

            default:
                T_E("Unexpected type: %d", cmd->type);
                break;
            }

            VTSS_RC(vtss_appl_psec_interface_conf_set(ifindex, &port_conf));
        }
    }

    return VTSS_RC_OK;
}

/***************************************************************************/
/*  Functions called by iCLI                                                */
/****************************************************************************/

// See psec_limit_icli_functions.h
mesa_rc psec_limit_icli_enable(i32 session_id, icli_stack_port_range_t *plist, BOOL no)
{
    psec_limit_cmd_t cmd;
    cmd.type = ENABLE;
    cmd.no = no;
    ICLI_RC_CHECK_PRINT_RC(psec_limit_icli_sit_port_loop(session_id, plist, &cmd));
    return VTSS_RC_OK;
}

// See psec_limit_icli_functions.h
mesa_rc psec_limit_icli_maximum(i32 session_id, icli_stack_port_range_t *plist, u32 limit, BOOL no)
{
    psec_limit_cmd_t cmd;
    cmd.type = MAXIMUM;
    cmd.no = no;
    cmd.value.limit = limit;
    ICLI_RC_CHECK_PRINT_RC(psec_limit_icli_sit_port_loop(session_id, plist, &cmd));
    return VTSS_RC_OK;
}

// See psec_limit_icli_functions.h
mesa_rc psec_limit_icli_maximum_violation(i32 session_id, icli_stack_port_range_t *plist, u32 violate_limit, BOOL no)
{
    psec_limit_cmd_t cmd;
    cmd.type = MAXIMUM_VIOLATION;
    cmd.no = no;
    cmd.value.violate_limit = violate_limit;
    ICLI_RC_CHECK_PRINT_RC(psec_limit_icli_sit_port_loop(session_id, plist, &cmd));
    return VTSS_RC_OK;
}

// See psec_limit_icli_functions.h
mesa_rc psec_limit_icli_violation(i32 session_id, BOOL has_protect, BOOL has_restrict, BOOL has_shutdown, icli_stack_port_range_t *plist, BOOL no)
{
    psec_limit_cmd_t cmd;
    cmd.type = VIOLATION;
    cmd.value.violation_mode = has_restrict ? VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT :
                               has_shutdown ? VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN : VTSS_APPL_PSEC_VIOLATION_MODE_PROTECT;
    cmd.no = no;

    ICLI_RC_CHECK_PRINT_RC(psec_limit_icli_sit_port_loop(session_id, plist, &cmd));
    return VTSS_RC_OK;
}

mesa_rc psec_limit_icli_mac_address(i32 session_id, icli_stack_port_range_t *plist, vtss_appl_psec_mac_conf_t *mac_conf, BOOL no)
{
    psec_limit_cmd_t cmd;

    if (mac_conf) {
        if (plist->cnt > 1 || plist->switch_range[0].port_cnt > 1) {
            ICLI_PRINTF("%% (Only a single interface can be selected when issuing this command)\n");
            return VTSS_RC_ERROR;
        }

        cmd.type = MAC;
        cmd.value.mac_conf = *mac_conf;
    } else {
        cmd.type = STICKY;
    }

    cmd.no = no;
    ICLI_RC_CHECK_PRINT_RC(psec_limit_icli_sit_port_loop(session_id, plist, &cmd));
    return VTSS_RC_OK;
}

// See psec_limit_icli_functions.h
mesa_rc psec_limit_icli_aging(i32 session_id, BOOL no)
{
    psec_limit_cmd_t cmd;
    cmd.type = AGING;
    cmd.no = no;
    ICLI_RC_CHECK_PRINT_RC(psec_limit_icli_global_conf(session_id, &cmd));
    return VTSS_RC_OK;
}

// See psec_limit_icli_functions.h
mesa_rc psec_limit_icli_aging_time(i32 session_id, u32 value, BOOL no)
{
    psec_limit_cmd_t cmd;
    cmd.type = AGING_TIME;
    cmd.value.aging_period_secs = value;
    cmd.no = no;
    ICLI_RC_CHECK_PRINT_RC(psec_limit_icli_global_conf(session_id, &cmd));
    return VTSS_RC_OK;
}

// See psec_limit_icli_functions.h
mesa_rc psec_limit_icli_hold_time(i32 session_id, u32 value, BOOL no)
{
    psec_limit_cmd_t cmd;
    cmd.type = HOLD_TIME;
    cmd.value.hold_time_secs = value;
    cmd.no = no;
    ICLI_RC_CHECK_PRINT_RC(psec_limit_icli_global_conf(session_id, &cmd));
    return VTSS_RC_OK;
}

//******************************************************************************
// psec_limit_icli_debug_ref_cnt()
//******************************************************************************
mesa_rc psec_limit_icli_debug_ref_cnt(i32 session_id, icli_stack_port_range_t *plist)
{
    switch_iter_t  sit;
    vtss_ifindex_t ifindex;
    u32            fwd_cnt, blk_cnt;
    mesa_rc        rc;

    // Here, we need to iterate over all ports in plist.
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID));

    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all ports
        port_iter_t pit;

        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL));

        while (icli_port_iter_getnext(&pit, plist)) {
            char buf[50];

            VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));
            VTSS_RC(psec_limit_mgmt_debug_ref_cnt_get(ifindex, &fwd_cnt, &blk_cnt));

            if ((rc = psec_limit_mgmt_debug_ref_cnt_get(ifindex, &fwd_cnt, &blk_cnt)) != VTSS_RC_OK) {
                ICLI_PRINTF("%% Unable to obtain debug info on port %s. Error code: %s\n", icli_port_info_txt_short(sit.usid, pit.uport, buf), error_txt(rc));
                return rc;
            }

            if (pit.first) {
                ICLI_PRINTF("Interface  Forwarding Blocking\n");
                ICLI_PRINTF("---------- ---------- --------\n");
            }

            ICLI_PRINTF("%-10s %10u %8u\n", icli_port_info_txt_short(sit.usid, pit.uport, buf), fwd_cnt, blk_cnt);
        }
    }

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_ICFG
/***************************************************************************/
/* ICFG callback functions */
/****************************************************************************/
static mesa_rc psec_limit_icfg_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    vtss_icfg_conf_print_t conf_print;

    vtss_icfg_conf_print_init(&conf_print, req, result);

    if (req->cmd_mode == ICLI_CMD_MODE_GLOBAL_CONFIG) {
        vtss_appl_psec_global_conf_t global_conf;
        vtss_appl_psec_global_conf_t global_conf_default;

        // Get current configuration
        VTSS_RC(vtss_appl_psec_global_conf_get(&global_conf));

        // Get default configuration values.
        VTSS_RC(vtss_appl_psec_global_conf_default_get(&global_conf_default));

        // Aging time
        conf_print.is_default = global_conf.aging_period_secs == global_conf_default.aging_period_secs;
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "port-security aging time", "%u", global_conf.aging_period_secs));

        // Aging enable
        conf_print.is_default = global_conf.enable_aging == global_conf_default.enable_aging;
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "port-security aging", "%s", ""));

        // Hold time
        conf_print.is_default = global_conf.hold_time_secs == global_conf_default.hold_time_secs;
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "port-security hold time", "%u", global_conf.hold_time_secs));

    } else if (req->cmd_mode == ICLI_CMD_MODE_INTERFACE_PORT_LIST) {
        vtss_isid_t                     isid = req->instance_id.port.isid;
        mesa_port_no_t                  iport = req->instance_id.port.begin_iport;
        vtss_appl_psec_interface_conf_t port_conf, port_conf_default;
        vtss_ifindex_t                  ifindex;
        vtss_ifindex_t                  prev_ifindex, next_ifindex;
        mesa_vid_t                      prev_vid, next_vid;
        mesa_mac_t                      prev_mac, next_mac;
        BOOL                            first = TRUE;

        VTSS_RC(vtss_ifindex_from_port(isid, iport, &ifindex));

        // Get current configuration
        VTSS_RC(vtss_appl_psec_interface_conf_get(ifindex, &port_conf));

        // Get default configuration
        VTSS_RC(vtss_appl_psec_interface_conf_default_get(&port_conf_default));

        // Limit
        conf_print.is_default = port_conf.limit == port_conf_default.limit;
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "port-security maximum", "%u", port_conf.limit));

        // violate-limit
        conf_print.is_default = port_conf.violate_limit == port_conf_default.violate_limit;
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "port-security maximum-violation", "%u", port_conf.violate_limit));

        // Violation Mode
        conf_print.is_default = port_conf.violation_mode == port_conf_default.violation_mode;
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "port-security violation", "%s",
                                     port_conf.violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_PROTECT  ? "protect"  :
                                     port_conf.violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT ? "restrict" :
                                     port_conf.violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN ? "shutdown" : "Unknown violation mode"));

        // Enable (after setting up Limit and Violation Mode)
        conf_print.is_default = port_conf.enabled == port_conf_default.enabled;
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "port-security", "%s", ""));

        // Sticky
        conf_print.is_default = port_conf.sticky == port_conf_default.sticky;
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "port-security mac-address sticky", "%s", ""));

        // Sticky and static MACs.
        prev_ifindex = ifindex;
        while (vtss_appl_psec_interface_conf_mac_itr(&prev_ifindex, &next_ifindex, first ? NULL : &prev_vid, &next_vid, first ? NULL : &prev_mac, &next_mac) == VTSS_RC_OK) {
            vtss_appl_psec_mac_conf_t mac_conf;
            mesa_vid_t                pvid;
            char                      buf1[20], buf2[20];

            if (next_ifindex != prev_ifindex) {
                // Iterating into next port. Done.
                break;
            }

            first = FALSE;
            prev_vid = next_vid;
            prev_mac = next_mac;

            if (vtss_appl_psec_interface_conf_mac_pvid_get(next_ifindex, next_vid, next_mac, &mac_conf, &pvid) != VTSS_RC_OK) {
                // Someone deleted the MAC address in between the iterator and now. That's not an error.
                break;
            }

            if (req->all_defaults || mac_conf.vlan != pvid) {
                sprintf(buf2, " vlan %u", mac_conf.vlan);
            } else {
                buf2[0] = '\0';
            }

            VTSS_RC(vtss_icfg_printf(result, " port-security mac-address%s %s%s\n",
                                     mac_conf.mac_type == VTSS_APPL_PSEC_MAC_TYPE_STICKY ? " sticky" : "",
                                     misc_mac_txt(mac_conf.mac.addr, buf1),
                                     buf2));
        }
    }

    return VTSS_RC_OK;
}
#endif /* defined(VTSS_SW_OPTION_ICFG) */

#if defined(VTSS_SW_OPTION_ICFG)
/* ICFG Initialization function */
mesa_rc psec_limit_icfg_init(void)
{
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_PSEC_LIMIT_GLOBAL_CONF,    "port-security", psec_limit_icfg_conf));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_PSEC_LIMIT_INTERFACE_CONF, "port-security", psec_limit_icfg_conf));
    return VTSS_RC_OK;
}
#endif /* defined(VTSS_SW_OPTION_ICFG) */

