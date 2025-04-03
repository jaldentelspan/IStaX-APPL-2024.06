/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <vtss/appl/psec.h>
#include <vtss/basics/enum_macros.hxx>
#include "icli_api.h"
#include "icli_porting_util.h"
#include "misc_api.h"
#include "psec_api.h"
#include "psec_icli_functions.h"
#include "psec_util.h"
#include "psec_rate_limit.h"
#include "msg_api.h"             /* For msg_abstime_get() */
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif

// Allow operator++ on vtss_appl_psec_user_t
VTSS_ENUM_INC(vtss_appl_psec_user_t);

//******************************************************************************
//
// Internal functions
//
//******************************************************************************

//******************************************************************************
// PSEC_ICLI_users_print()
//******************************************************************************
static void PSEC_ICLI_users_print(u32 session_id)
{
    vtss_appl_psec_user_t user;
    u32                   included_in_build_mask = psec_util_users_included_get();

    ICLI_PRINTF("Users:\n");

    for (user = (vtss_appl_psec_user_t)0; user < VTSS_APPL_PSEC_USER_LAST; user++) {
        if ((included_in_build_mask & VTSS_BIT(user)) == 0) {
            continue;
        }

        ICLI_PRINTF("  %c = %s\n", psec_util_user_abbr_get(user), psec_util_user_name_get(user));
    }

    ICLI_PRINTF("\n");
}

//******************************************************************************
// PSEC_ICLI_print_overview()
//******************************************************************************
static mesa_rc PSEC_ICLI_print_overview(i32 session_id, const switch_iter_t *sit, const port_iter_t *pit, BOOL debug, vtss_appl_psec_mac_status_map_t *unused, icli_stack_port_range_t *plist)
{
    char                                           buf[50], ena_str[VTSS_APPL_PSEC_USER_LAST + 1], limit_buf[20], viol_buf[20], violation_mode_buf[20], flags_buf[20];
    vtss_appl_psec_interface_status_t              port_status;
    vtss_appl_psec_interface_notification_status_t port_notif_status;
    vtss_appl_psec_interface_conf_t                port_conf;
    vtss_ifindex_t                                 ifindex;
    mesa_rc                                        rc;

    VTSS_RC(vtss_ifindex_from_port(sit->isid, pit->iport, &ifindex));

    // Print headers
    if (pit->first) {
        // First time, print PSEC users. We only print those that are included in the build.
        PSEC_ICLI_users_print(session_id);

        if (debug) {
            ICLI_PRINTF("Flags:\n"
                        "  D = Port is shut down\n"
                        "  L = Limit is reached\n"
                        "  E = Secure learning enabled\n"
                        "  C = CPU copying is enabled\n"
                        "  U = Link is up\n"
                        "  M = STP instance is discarding\n"
                        "  H = H/W add failed\n"
                        "  S = S/W add failed\n\n");
        }

        ICLI_PRINTF("Interface  Users Limit Current Violating Violation Mode Sticky State%s\n",         debug ? "         Flags" : "");
        ICLI_PRINTF("---------- ----- ----- ------- --------- -------------- ------ -------------%s\n", debug ? " --------"      : "");
    }

    if ((rc = vtss_appl_psec_interface_status_get(ifindex, &port_status)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Unable to obtain interface status: %s\n", error_txt(rc));
        return rc;
    }

    if ((rc = vtss_appl_psec_interface_notification_status_get(ifindex, &port_notif_status)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Unable to obtain interface notification status: %s\n", error_txt(rc));
        return rc;
    }

    if ((rc = vtss_appl_psec_interface_conf_get(ifindex, &port_conf)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Unable to obtain interface configuration: %s\n", error_txt(rc));
        return rc;
    }

    // If invoked without a port-list, only print info for enabled ports.
    if (plist == NULL && port_status.users == 0) {
        return VTSS_RC_OK;
    }

    if (debug) {
        sprintf(flags_buf, " %c%c%c%c%c%c%c%c",
                port_notif_status.shut_down ? 'D' : '-',
                port_status.limit_reached   ? 'L' : '-',
                port_status.sec_learning    ? 'E' : '-',
                port_status.cpu_copying     ? 'C' : '-',
                port_status.link_is_up      ? 'U' : '-',
                port_status.stp_discarding  ? 'M' : '-',
                port_status.hw_add_failed   ? 'H' : '-',
                port_status.sw_add_failed   ? 'S' : '-');
    } else {
        flags_buf[0] = '\0';
    }

    if (port_conf.enabled) {
        sprintf(limit_buf, "%u", port_conf.limit);
        sprintf(viol_buf,  "%u", port_status.cur_violate_cnt);
    } else {
        strcpy(limit_buf, "N/A");
        strcpy(viol_buf,  "N/A");
    }

    sprintf(violation_mode_buf, "%s", !port_conf.enabled ? "Disabled" :
            port_conf.violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_PROTECT  ? "Protect"  :
            port_conf.violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT ? "Restrict" :
            port_conf.violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN ? "Shutdown" : "Unknown");

    ICLI_PRINTF("%-10s %-5s %5s %7u %9s %-14s %-6s %-13s%s\n",
                icli_port_info_txt_short(sit->usid, pit->uport, buf),
                psec_util_user_abbr_str_populate(ena_str, port_status.users),
                limit_buf,
                port_status.mac_cnt,
                viol_buf,
                violation_mode_buf,
                port_status.sticky ? "Yes" : "No",
                port_status.users == 0 ? "No users" : port_notif_status.shut_down ? "Shut Down" : port_status.limit_reached ? "Limit Reached" : "Ready",
                flags_buf);

    return VTSS_RC_OK;
}

//******************************************************************************
// PSEC_ICLI_print_address()
//******************************************************************************
static mesa_rc PSEC_ICLI_print_address(i32 session_id, const switch_iter_t *sit, const port_iter_t *pit, BOOL debug, vtss_appl_psec_mac_status_map_t *mac_status_map, icli_stack_port_range_t *plist)
{
    char                                            buf[200], buf1[20], buf2[20], buf3[50];
    char                                            buf4[VTSS_APPL_PSEC_USER_LAST + 1], buf5[VTSS_APPL_PSEC_USER_LAST + 1], buf6[VTSS_APPL_PSEC_USER_LAST + 1];
    vtss_appl_psec_interface_status_t               port_status;
    vtss_appl_psec_mac_status_map_t::const_iterator mac_itr;
    vtss_ifindex_t                                  ifindex;
    mesa_rc                                         rc;

    VTSS_RC(vtss_ifindex_from_port(sit->isid, pit->iport, &ifindex));

    if ((rc = vtss_appl_psec_interface_status_get(ifindex, &port_status)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Error: %s\n", error_txt(rc));
        return rc;
    }

    // Print headers
    if (pit->first) {
        if (debug) {
            PSEC_ICLI_users_print(session_id);
            ICLI_PRINTF("Flags:\n"
                        "  F = Entry is forwarding\n"
                        "  B = Entry is blocked but subject to 'aging'\n"
                        "  K = Entry is kept blocked (not subject to 'aging')\n"
                        "  V = Entry is violating\n"
                        "  C = Entry is being aged (copying to CPU)\n"
                        "  A = An age frame was seen\n"
                        "Per-user forward decisions:\n"
                        "  F = Forward by\n"
                        "  B = Block by\n"
                        "  K = Kept blocked by\n\n");
        }

        ICLI_PRINTF("VLAN MAC Address       Type    State      Port       Age/Hold Time%s\n", !debug ? "" : " Flags F    B    K    Added                     Changed");
        ICLI_PRINTF("---- ----------------- ------- ---------- ---------- -------------%s\n", !debug ? "" : " ----- ---- ---- ---- ------------------------- -------------------------");
    }

    // If invoked without a port-list, only print port info for enabled ports.
    if (plist == NULL && port_status.users == 0) {
        return VTSS_RC_OK;
    }

    // Loop through all MAC addresses attached to this port
    for (mac_itr = mac_status_map->cbegin(); mac_itr != mac_status_map->cend(); ++mac_itr) {
        const vtss_appl_psec_mac_status_t *mac_status;

        if (mac_itr->first.ifindex < ifindex) {
            // Not gotten to the port we're looking for yet.
            continue;
        }

        if (mac_itr->first.ifindex > ifindex) {
            // Iterating into next port. Done.
            break;
        }

        mac_status = &mac_itr->second;

        sprintf(buf1, "%s", mac_status->violating ? "Violating" : (mac_status->blocked || mac_status->kept_blocked) ? "Blocked" : "Forwarding");

        if (mac_status->age_or_hold_time_secs == 0) {
            sprintf(buf2, "%s", "N/A");
        } else {
            sprintf(buf2, "%13u", mac_status->age_or_hold_time_secs);
        }

        ICLI_PRINTF("%4u %-17s %-7s %-10s %-10s %13s%s",
                    mac_status->vid_mac.vid,
                    misc_mac_txt(mac_status->vid_mac.mac.addr, buf),
                    psec_util_mac_type_to_str(mac_status->mac_type),
                    buf1,
                    icli_port_info_txt_short(sit->usid, pit->uport, buf3),
                    buf2,
                    debug ? " " : "\n");

        if (debug) {
            sprintf(buf1, "%c%c%c%c",
                    mac_status->blocked ? mac_status->kept_blocked ? 'K' : 'B' : 'F',
                    mac_status->violating                          ? 'V' : '-',
                    mac_status->cpu_copying                        ? 'C' : '-',
                    mac_status->age_frame_seen                     ? 'A' : '-');

            ICLI_PRINTF("%-5s %-4s %-4s %-4s %-25s %-25s\n",
                        buf1,
                        psec_util_user_abbr_str_populate(buf4, port_status.users & mac_status->users_forward),
                        psec_util_user_abbr_str_populate(buf5, port_status.users & mac_status->users_block),
                        psec_util_user_abbr_str_populate(buf6, port_status.users & mac_status->users_keep_blocked),
                        mac_status->creation_time,
                        mac_status->changed_time);
        }
    }

    return VTSS_RC_OK;
}

//******************************************************************************
// PSEC_ICLI_sit_pit_loop()
//******************************************************************************
static mesa_rc PSEC_ICLI_sit_pit_loop(i32 session_id, icli_stack_port_range_t *plist, mesa_rc (*cmd)(i32 session_id, const switch_iter_t *sit, const port_iter_t *pit, BOOL debug, vtss_appl_psec_mac_status_map_t *mac_status_map, icli_stack_port_range_t *plist), BOOL debug, vtss_appl_psec_mac_status_map_t *mac_status_map)
{
    switch_iter_t sit;

    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID));

    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all ports
        port_iter_t pit;

        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL));

        while (icli_port_iter_getnext(&pit, plist)) {
            VTSS_RC(cmd(session_id, &sit, &pit, debug, mac_status_map, plist));
        }
    }

    return VTSS_RC_OK;
}

//******************************************************************************
//
// Public functions
//
//******************************************************************************

//******************************************************************************
// psec_icli_show()
//******************************************************************************
mesa_rc psec_icli_show(i32 session_id, icli_stack_port_range_t *plist, BOOL debug)
{
    vtss_appl_psec_global_conf_t global_conf;

    ICLI_RC_CHECK_PRINT_RC(PSEC_ICLI_sit_pit_loop(session_id, plist, PSEC_ICLI_print_overview, debug, NULL));

    // Also print whether aging is enabled and if so, the configured aging time, and the configured hold time.
    VTSS_RC(vtss_appl_psec_global_conf_get(&global_conf));

    if (global_conf.enable_aging) {
        ICLI_PRINTF("\nAging time: %u seconds", global_conf.aging_period_secs);
    } else {
        ICLI_PRINTF("\nAging disabled");
    }

    ICLI_PRINTF("\nHold time: %u seconds\n\n", global_conf.hold_time_secs);

    return VTSS_RC_OK;
}

//******************************************************************************
// psec_icli_address_show()
//******************************************************************************
mesa_rc psec_icli_address_show(i32 session_id, icli_stack_port_range_t *plist, BOOL debug)
{
    vtss_appl_psec_global_status_t  global_status;
    vtss_appl_psec_mac_status_map_t mac_status_map;
    mesa_rc                         rc;

    // Get all MAC addresses learned by PSEC.
    if ((rc = vtss_appl_psec_interface_status_mac_get_all(mac_status_map)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Error: %s\n", error_txt(rc));
        return rc;
    }

    ICLI_RC_CHECK_PRINT_RC(PSEC_ICLI_sit_pit_loop(session_id, plist, PSEC_ICLI_print_address, debug, &mac_status_map));

    // Also print total number of MAC addresses and currently used MAC addresses.
    if ((rc = vtss_appl_psec_global_status_get(&global_status)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Error: %s\n", error_txt(rc));
        return rc;
    }

    ICLI_PRINTF("\nNumber of MAC addresses manageable by port-security in the system: %u\n", global_status.total_mac_cnt);
    ICLI_PRINTF(  "Number of MAC addresses currently used by port-security in the system: %zu\n\n", mac_status_map.size());

    return VTSS_RC_OK;
}

//******************************************************************************
// psec_icli_mac_clear()
//******************************************************************************
mesa_rc psec_icli_mac_clear(i32 session_id, BOOL has_mac, BOOL has_vlan, mesa_mac_t mac, icli_stack_port_range_t *plist, mesa_vid_t vlan)
{
    vtss_appl_psec_global_control_mac_clear_t info;
    switch_iter_t                             sit;
    mesa_rc                                   rc;

    memset(&info, 0, sizeof(info));
    info.specific_ifindex = plist != NULL;
    info.specific_vlan    = has_vlan;
    info.specific_mac     = has_mac;
    info.vlan             = vlan;
    info.mac              = mac;

    if (!plist) {
        // User hasn't requested us to remove on a specific interface.
        // Call once and exit.
        if ((rc = vtss_appl_psec_global_control_mac_clear(&info)) != VTSS_RC_OK) {
            ICLI_PRINTF("%% Unable to clear MAC address%s. Error code: %s\n", info.specific_mac && info.specific_vlan ? "" : "es", error_txt(rc));
        }

        return rc;
    }

    // Here, we need to iterate over all ports in plist.
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID));

    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all ports
        port_iter_t pit;

        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_NORMAL));

        while (icli_port_iter_getnext(&pit, plist)) {
            char buf[50];

            VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &info.ifindex));

            if ((rc = vtss_appl_psec_global_control_mac_clear(&info)) != VTSS_RC_OK) {
                ICLI_PRINTF("%% Unable to clear MAC addresses on %s. Error code: %s\n", icli_port_info_txt_short(sit.usid, pit.uport, buf), error_txt(rc));
                return rc;
            }
        }
    }

    return VTSS_RC_OK;
}

//******************************************************************************
// psec_icli_rate_limit_config_show()
//******************************************************************************
mesa_rc psec_icli_rate_limit_config_show(i32 session_id)
{
    psec_rate_limit_conf_t rate_limit_conf;
    mesa_rc                rc;

    if ((rc = psec_rate_limit_conf_get(&rate_limit_conf)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Unable to get rate-limit config: %s\n", error_txt(rc));
        return rc;
    }

    // Print current settings
    ICLI_PRINTF("Min Fill Level: %10u frames\n"
                "Max Fill Level: %10u frames\n"
                "Rate          : %10u frames/second\n"
                "Drop age      : " VPRI64Fu("10") " msecs\n\n",
                rate_limit_conf.fill_level_min,
                rate_limit_conf.fill_level_max,
                rate_limit_conf.rate,
                rate_limit_conf.drop_age_ms);

    return VTSS_RC_OK;
}

//******************************************************************************
// psec_icli_rate_limit_config_set()
//******************************************************************************
mesa_rc psec_icli_rate_limit_config_set(i32 session_id, BOOL has_fill_level_min, u32 fill_level_min, BOOL has_fill_level_max, u32 fill_level_max, BOOL has_rate, u32 rate, BOOL has_drop_age, u32 drop_age)
{
    psec_rate_limit_conf_t rate_limit_conf;
    mesa_rc                rc;

    if ((rc = psec_rate_limit_conf_get(&rate_limit_conf)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Unable to get rate-limit config: %s\n", error_txt(rc));
        return rc;
    }

    if (has_fill_level_min) {
        rate_limit_conf.fill_level_min = fill_level_min;
    }

    if (has_fill_level_max) {
        rate_limit_conf.fill_level_max = fill_level_max;
    }

    if (has_rate) {
        rate_limit_conf.rate = rate;
    }

    if (has_drop_age) {
        rate_limit_conf.drop_age_ms = drop_age;
    }

    if ((rc = psec_rate_limit_conf_set(VTSS_ISID_GLOBAL, &rate_limit_conf)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Unable to set rate-limit config: %s\n", error_txt(rc));
    }

    return rc;
}

//******************************************************************************
// psec_icli_rate_limit_statistics_show()
//******************************************************************************
mesa_rc psec_icli_rate_limit_statistics_show(i32 session_id)
{
    mesa_rc                rc;
    psec_rate_limit_stat_t statistics;
    u64                    cur_fill_level;

    if ((rc = psec_rate_limit_stat_get(&statistics, &cur_fill_level)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Unable to obtain rate-limit statistics: %s\n", error_txt(rc));
        return rc;
    }

    ICLI_PRINTF("Frame Counters:\n"
                "  Forwarded  : " VPRI64Fu("12") "\n"
                "  Dropped    : " VPRI64Fu("12") "\n"
                "  Filtered   : " VPRI64Fu("12") "\n"
                "  Fill-level : " VPRI64Fu("12") "\n\n",
                statistics.forward_cnt,
                statistics.drop_cnt,
                statistics.filter_drop_cnt,
                cur_fill_level);

    return VTSS_RC_OK;
}

//******************************************************************************
// psec_icli_rate_limit_statistics_clear()
//******************************************************************************
mesa_rc psec_icli_rate_limit_statistics_clear(i32 session_id)
{
    mesa_rc rc;

    if ((rc = psec_rate_limit_stat_clr()) != VTSS_RC_OK) {
        ICLI_PRINTF("Unable to clear rate-limiter statistics: %s\n", error_txt(rc));
    }

    return rc;
}
