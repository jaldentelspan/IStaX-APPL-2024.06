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
#include "icli_api.h"
#include "icli_porting_util.h" // for icli_port_info_txt(), icli_table_header()

#include "xxrp_api.h"
#include "vtss_mrp.hxx"
#include "mrp_icli_functions.h"
#include "xxrp_trace.h"
#include "critd_api.h"
#include "misc_api.h"

VTSS_CRIT_SCOPE_CLASS_EXTERN(xxrp_appl_crit, VtssXxrpApplCritdGuard);

#define XXRP_APPL_CRIT_SCOPE() VtssXxrpApplCritdGuard __lock_guard__(__LINE__)

BOOL mrp_icli_runtime_mvrp(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
    case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_MVRP)
        runtime->present = TRUE;
#else
        runtime->present = FALSE;
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

#ifdef VTSS_SW_OPTION_MVRP
mesa_rc mvrp_icli_global_set(const i32 session_id, BOOL enable)
{
    int appl_exclusive = 1;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_ICLI, DEBUG) << "ICLI "
                                                << (enable ? "enable" : "disable")
                                                << " MVRP globally";
    XXRP_APPL_CRIT_SCOPE();
    if (xxrp_mgmt_appl_exclusion(VTSS_MRP_APPL_MVRP)) {
        appl_exclusive = 0;
    }
    if (appl_exclusive) {
        VTSS_RC(xxrp_mgmt_global_enabled_set(VTSS_MRP_APPL_MVRP, enable));
    } else {
        if (enable) {
            return XXRP_ERROR_APPL_OVERLAP;
        }
    }
    return VTSS_RC_OK;
}

mesa_rc mvrp_icli_port_set(const i32 session_id, icli_stack_port_range_t *plist, BOOL enable)
{
    switch_iter_t sit;
    port_iter_t   pit;

    // Loop through all the switches in the plist
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all the ports in the plist
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_ICLI, DEBUG) << "ICLI "
                                                        << (enable ? "enable" : "disable")
                                                        << " MVRP on port " << pit.uport;
            VTSS_RC(xxrp_mgmt_enabled_set(sit.isid, pit.iport, VTSS_MRP_APPL_MVRP, enable));
        }
    }

    return VTSS_RC_OK;
}

static void mvrp_icli_timers_set_to_default(vtss_mrp_timer_conf_t *timers)
{
    timers->join_timer = 20;
    timers->leave_timer = 60;
    timers->leave_all_timer = 1000;
}

mesa_rc mrp_icli_timers_set(const i32 session_id, icli_stack_port_range_t *plist,
                            BOOL has_join_time, u32 join_time,
                            BOOL has_leave_time, u32 leave_time,
                            BOOL has_leave_all_time, u32 leave_all_time)
{
    vtss_mrp_timer_conf_t timers;
    switch_iter_t         sit;
    port_iter_t           pit;

    // Loop through all the switches in the plist
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all the ports in the plist
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            VTSS_RC(mrp_mgmt_timers_get(sit.isid, pit.iport, &timers));
            if (has_join_time) {
                timers.join_timer = join_time;
            }
            if (has_leave_time) {
                timers.leave_timer = leave_time;
            }
            if (has_leave_all_time) {
                timers.leave_all_timer = leave_all_time;
            }
            VTSS_RC(mrp_mgmt_timers_set(sit.isid, pit.iport, &timers));
        }
    }

    return VTSS_RC_OK;
}

mesa_rc mrp_icli_timers_def(const i32 session_id, icli_stack_port_range_t *plist)
{
    vtss_mrp_timer_conf_t timers;
    switch_iter_t         sit;
    port_iter_t           pit;

    // Loop through all the switches in the plist
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all the ports in the plist
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            mvrp_icli_timers_set_to_default(&timers);
            VTSS_RC(mrp_mgmt_timers_set(sit.isid, pit.iport, &timers));
        }
    }
    return VTSS_RC_OK;
}

mesa_rc mrp_icli_periodic_set(const i32 session_id, icli_stack_port_range_t *plist,
                              bool state)
{
    switch_iter_t         sit;
    port_iter_t           pit;

    // Loop through all the switches in the plist
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all the ports in the plist
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            VTSS_RC(vtss::mrp::mgmt_periodic_state_set(sit.isid, pit.iport, state));
        }
    }
    return VTSS_RC_OK;
}

static void mvrp_icli_bitmask_update(icli_unsigned_range_t *vlan_list,
                                     vtss::VlanList &vls, bool set)
{
    mesa_vid_t vid;
    u32        idx;

    for (idx = 0; idx < vlan_list->cnt; idx++) {
        for (vid = vlan_list->range[idx].min; vid <= vlan_list->range[idx].max; vid++) {
            if (set) {
                vls.set(vid);
            } else {
                vls.clear(vid);
            }
        }
    }
}

mesa_rc mvrp_icli_vlans_set(const i32 session_id, icli_unsigned_range_t *vlan_list, BOOL has_default, BOOL has_all,
                            BOOL has_none, BOOL has_add, BOOL has_remove, BOOL has_except)
{
    vtss::VlanList vls;

    if (!vlan_list && !has_all && !has_none) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_ICLI, DEBUG) << "vlist = " << vlan_list;
        return VTSS_RC_ERROR;
    }

    if (has_default) {
        // Reset to defaults.
        vls.clear_all();
    } else if (has_all || has_except) {
        for (uint i = 1; i < VTSS_APPL_VLAN_ID_MAX; ++i) {
            vls.set(i);
        }
        if (has_except && vlan_list) {
            // Remove VIDs in the vlan_list from bitmask
            mvrp_icli_bitmask_update(vlan_list, vls, false);
        }
    } else if (has_none || (!has_remove && !has_add)) {
        // Here, either the "none" keyword or no keywords at all were used on the command line.
        vls.clear_all();
        if (!has_none) {
            /* Overwrite new vlan list to database */
            mvrp_icli_bitmask_update(vlan_list, vls, true);
        }
    } else if (vlan_list) {
        xxrp_mgmt_global_managed_vids_get(VTSS_MRP_APPL_MVRP, vls);
        if (has_add) {
            mvrp_icli_bitmask_update(vlan_list, vls, true);
        } else {
            /* This leaves only the remove keyword */
            mvrp_icli_bitmask_update(vlan_list, vls, false);
        }
    }
    VTSS_RC(xxrp_mgmt_global_managed_vids_set(VTSS_MRP_APPL_MVRP, vls));
    return VTSS_RC_OK;
}

mesa_rc mrp_icli_show_status(const i32 session_id, icli_stack_port_range_t *plist,
                             BOOL has_mvrp)
{
    switch_iter_t        sit;
    port_iter_t          pit;
    char                 buf[200];
    char                 interface_str[ICLI_PORTING_STR_BUF_SIZE];
    vtss_appl_mrp_appl_t appl;
    vtss_appl_mrp_appl_t first_appl = (vtss_appl_mrp_appl_t)0;
    vtss_appl_mrp_appl_t last_appl  = (vtss_appl_mrp_appl_t)(VTSS_APPL_MRP_APPL_LAST - 1);
    BOOL                 print_hdr  = TRUE;
    BOOL                 has_all    = TRUE;

#ifdef VTSS_SW_OPTION_MVRP
    if (has_mvrp) {
        first_appl = VTSS_APPL_MRP_APPL_MVRP;
        has_all = FALSE;
    }
#endif

    if (!has_all) {
        last_appl = first_appl;
    }

    // Loop through all switches in the stack
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID));
    while (icli_switch_iter_getnext(&sit, plist)) {
        VTSS_RC(port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL));
        while (icli_port_iter_getnext(&pit, plist)) {
            vtss::mrp::MrpApplPortStat stat;

            (void)icli_port_info_txt(sit.usid, pit.uport, interface_str);
            strcat(&interface_str[0], " :");
            icli_table_header(session_id, &interface_str[0]);

            print_hdr = TRUE;

            for (appl = first_appl; appl <= last_appl; ++appl) {
                if (vtss::mrp::mgmt_stat_port_get(sit.isid, pit.iport, appl, stat) != VTSS_RC_OK) {
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_ICLI, INFO) << "Skipping MRP appl "
                                                               << appl << " isid "
                                                               << sit.isid << " iport "
                                                               << pit.iport;
                    continue;
                }

                if (print_hdr) {
                    (void)snprintf(&buf[0], sizeof(buf), "%-10s%-21s%-13s",
                                   "MRP Appl",
                                   "FailedRegistrations",
                                   "LastPduOrigin");

                    icli_table_header(session_id, buf);
                    print_hdr = FALSE;
                }

                ICLI_PRINTF("%-10s", vtss::mrp::mgmt_appl_to_txt(appl));

                ICLI_PRINTF(VPRI64Fu("-21"), stat.failedRegistrations);

                ICLI_PRINTF("%-13s", misc_mac2str(stat.lastPduOrigin));
                ICLI_PRINTF("\n");
            }
            ICLI_PRINTF("\n");
        }
    }

    return VTSS_RC_OK;
}

mesa_rc mvrp_icli_debug_state_machines(const i32 session_id, icli_stack_port_range_t *plist, icli_unsigned_range_t *vlist)
{
    switch_iter_t sit;
    port_iter_t   pit;
    u16 j, vlan_min, vlan, vlan_max;
    char                   buf[200];
    char                   interface_str[ICLI_PORTING_STR_BUF_SIZE];
    bool                   print_hdr = true;
    vtss::mrp::MrpApplStat mvrp_stat;

    if (!plist || !vlist) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_ICLI, DEBUG) << "plist = " << plist << ", vlist = " << vlist;
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss::mrp::mgmt_stat_mvrp_get(mvrp_stat));
    if (!mvrp_stat.global_state) {
        // MRP Application is not enabled, nothing to display
        return VTSS_RC_OK;
    }
    // Loop through all the switches in the plist
    VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through all the ports in the plist
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            u32 port = L2PORT2PORT(sit.isid, pit.iport);
            if (!mvrp_stat.port_state[port]) {
                // Skip ports not currently running the MRP Application
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_ICLI, INFO) << "Skipping"
                                                           << " isid " << sit.isid
                                                           << " iport " << pit.iport;
                continue;
            }

            vtss::mrp::MrpApplPortDebug stat;
            if (vtss::mrp::mgmt_debug_mvrp_get(sit.isid, pit.iport, stat) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_ICLI, INFO) << "Skipping"
                                                           << " isid " << sit.isid
                                                           << " iport " << pit.iport;
                continue;
            }

            (void)icli_port_info_txt(sit.usid, pit.uport, interface_str);
            strcat(&interface_str[0], " :");
            icli_table_header(session_id, &interface_str[0]);

            print_hdr = true;
            (void)snprintf(&buf[0], sizeof(buf), "%-14s%-14s%-17s%-18s%-21s%-21s",
                           "LeaveAll STM",
                           "Periodic STM",
                           "Join Timer (ms)",
                           "Leave Timer (ms)",
                           "LeaveAll Timer (ms)",
                           "Periodic Timer (ms)");
            icli_table_header(session_id, buf);
            ICLI_PRINTF("%-14s", bool_state2txt(stat.leaveAllState));
            ICLI_PRINTF("%-14s", bool_state2txt(stat.periodicState));
            ICLI_PRINTF(VPRI64Fu("-17"), stat.join.raw());
            ICLI_PRINTF(VPRI64Fu("-18"), stat.leave.raw());
            ICLI_PRINTF(VPRI64Fu("-21"), stat.leaveAll.raw());
            ICLI_PRINTF(VPRI64Fu("-21"), stat.periodic.raw());
            ICLI_PRINTF("\n");

            for (j = 0; j < vlist->cnt; ++j) {
                vlan_min = vlist->range[j].min;
                vlan_max = vlist->range[j].max;
                for (vlan = vlan_min; vlan <= vlan_max; ++vlan) {
                    if (print_hdr) {
                        (void)snprintf(&buf[0], sizeof(buf), "%-6s%-15s%-15s%-15s",
                                       "VLAN",
                                       "Applicant STM",
                                       "Registrar STM",
                                       "Registrar Admin"
                                      );

                        icli_table_header(session_id, buf);
                        print_hdr = false;
                    }
                    if (!mvrp_stat.vlan_list.get(vlan)) {
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_ICLI, INFO) << "Skipping VLAN "
                                                                   << vlan;
                        continue;
                    }
                    ICLI_PRINTF("%-6u", vlan);
                    ICLI_PRINTF("%-15s", applicant_state2txt(stat.states[mvrp_stat.vlan2index[vlan]].applicant()));
                    ICLI_PRINTF("%-15s", registrar_state2txt(stat.states[mvrp_stat.vlan2index[vlan]].registrar()));
                    ICLI_PRINTF("%-15s", registrar_admin_state2txt(stat.states[mvrp_stat.vlan2index[vlan]].registrar_admin()));
                    ICLI_PRINTF("\n");
                }
            }
            ICLI_PRINTF("\n");
        }
    }
    return VTSS_RC_OK;
}

mesa_rc mvrp_icli_debug_msti_connected_ring(const i32 session_id, BOOL has_msti, u8 instance)
{
    bool                   first_port = true;
    char                   buf[200];
    vtss::mrp::MrpApplStat mvrp_stat;
    u8                     msti, msti_start, msti_end;

    VTSS_RC(vtss::mrp::mgmt_stat_mvrp_get(mvrp_stat));
    if (!mvrp_stat.global_state) {
        // MRP Application is not enabled, nothing to display
        return VTSS_RC_OK;
    }

    if (has_msti) {
        msti_start = instance;
        msti_end = instance + 1;
    } else {
        msti_start = 0;
        msti_end = MRP_MSTI_MAX;
    }
    for (msti = msti_start; msti < msti_end; msti++) {
        (void)snprintf(&buf[0], sizeof(buf), "MAP context of MSTI %u :", msti);
        icli_table_header(session_id, buf);
        first_port = true;
        for (auto it = mvrp_stat.ring[msti].cbegin(); it != mvrp_stat.ring[msti].cend(); ++it) {
            if (!first_port) {
                ICLI_PRINTF(" -> ");
            }
            ICLI_PRINTF("%-2s", (l2port2str(*it)));
            first_port = false;
        }
        ICLI_PRINTF("\n\n");
    }
    return VTSS_RC_OK;
}

#endif
