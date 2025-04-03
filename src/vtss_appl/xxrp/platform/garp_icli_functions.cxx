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
#ifdef VTSS_SW_OPTION_GVRP

#include "xxrp_api.h"
#include "icli_api.h"
#include "../base/vtss_gvrp.h"
#include <vtss_xxrp_callout.h>
#include "garp_icli_functions.h"
#include "xxrp_trace.h"
#include "critd_api.h"

VTSS_CRIT_SCOPE_CLASS_EXTERN(xxrp_appl_crit, VtssXxrpApplCritdGuard);

#define XXRP_APPL_CRIT_SCOPE() VtssXxrpApplCritdGuard __lock_guard__(__LINE__)

static void f_header(u32 session_id)
{
    ICLI_PRINTF("             |<-------- State of: ------->||<--- Timer [cs]: -->|\n");
    ICLI_PRINTF("Sw Port VLan  Applicant Registrar LeaveAll  txPDU leave leaveall  GIP-Context\n");
}

u32 leaveall_state;

static void f(u32 session_id, u16 usid, u16 isid, u16 iport, u16 port, unsigned int vlan)
{
    int rc;

    int port_no;
    struct garp_gid_instance *gid;
    int leave;
    u32 txPDU_timeout = -1;
    int gip_context;
    u32  leaveall_timeout = -1;

    garp_applicant_state_t  applicant_state;
    garp_registrar_state_t  registrar_state;

    port_no = L2PORT2PORT(isid, iport);

    GVRP_CRIT_ENTER();

    rc = vtss_gvrp_gid(port_no, vlan, &gid);
    if (rc) {
        GVRP_CRIT_EXIT();
        return;
    }

    leave = vtss_gvrp_leave_timeout(gid);
    gip_context = vtss_gvrp_gip_context(gid);
    leaveall_timeout = vtss_gvrp_leaveall_timeout(port_no);

    if (IS_GID_IN_TXPDU_QUEUE(gid)) {
        txPDU_timeout = vtss_gvrp_txPDU_timeout(gid);
    }

    applicant_state = (garp_applicant_state_t)gid->applicant_state;
    registrar_state = (garp_registrar_state_t)gid->registrar_state;

    vtss_gvrp_gid_done(&gid);
    GVRP_CRIT_EXIT();


    ICLI_PRINTF("%2hu %4hu %4u  ", usid, port, vlan);
    ICLI_PRINTF("%6s %9s %11s",
                applicant_state_name(applicant_state),
                registrar_state_name(registrar_state),
                leaveall_state_name((garp_leaveall_state_t)leaveall_state));

    // --- If not in txPDU queue, then print dash
    if (txPDU_timeout == -1) {
        ICLI_PRINTF("     - ");
    }  else {
        ICLI_PRINTF("%7u", txPDU_timeout);
    }

    // --- If not in leave queue, then print dash
    if (leave == -1) {
        ICLI_PRINTF("   -   ");
    } else {
        ICLI_PRINTF(" %5d ", leave);
    }

    ICLI_PRINTF("%6u", leaveall_timeout);

    if (gip_context < 0) {
        ICLI_PRINTF("     -\n");
    } else {
        ICLI_PRINTF("     %1d\n", gip_context);
    }
}


void gvrp_protocol_state(u32 session_id, icli_stack_port_range_t *plist, icli_unsigned_range_t *v_vlan_list)
{
    u32 i;
    int rc;

    u16 usid, isid;

    if (!plist || !v_vlan_list) {
        T_E("plist=%p,  v_vlan_list=%p", plist, v_vlan_list);
        return;
    }

    f_header(session_id);


    //  Loop over switch, port, vlans
    for (i = 0; i < plist->cnt; ++i) {

        usid = plist->switch_range[i].usid;
        isid = plist->switch_range[i].isid;

        u16 portBegin = plist->switch_range[i].begin_port;
        u16 iportBegin = plist->switch_range[i].begin_iport;
        u16 k;

        for (k = 0; k < plist->switch_range[i].port_cnt; ++k) {
            u32 j;
            int port_no;
            struct garp_participant *p;

            port_no = L2PORT2PORT(isid, k + iportBegin);

            GVRP_CRIT_ENTER();
            rc = vtss_gvrp_participant(port_no, &p);
            GVRP_CRIT_EXIT();

            if (rc) {
                T_N("GVRP protocol-state.1: port_no=%d do not exist", port_no);
                continue;
            }
            leaveall_state = p->leaveall_state;

            for (j = 0; j < v_vlan_list->cnt; ++j) {

                unsigned int vlan_min = v_vlan_list->range[j].min;
                unsigned int vlan_max = v_vlan_list->range[j].max;
                unsigned int vlan;

                for (vlan = vlan_min; vlan <= vlan_max; ++vlan) {

                    f(session_id, usid, isid, k + iportBegin, k + portBegin, vlan);

                }
            }
        }
    }

    return;
}

mesa_rc gvrp_global_enable(int enable, int max_vlans)
{
    mesa_rc rc = VTSS_RC_OK;
    int     appl_exclusive = 1;

    T_D("GVRP global enable.1, enable=%d max_vlans=%d", enable, max_vlans);

    XXRP_APPL_CRIT_SCOPE();

    if (enable == vtss_gvrp_is_enabled() && max_vlans == vtss_gvrp_max_vlans()) {
        /* Config didn't changed, do nothing */
        return VTSS_RC_OK;
    }

    /* Check that no other MRP/GARP application is currently enabled */
    if (xxrp_mgmt_appl_exclusion(VTSS_GARP_APPL_GVRP)) {
        appl_exclusive = 0;
    }

    if (appl_exclusive) {
        if (enable) {

            rc = vtss_gvrp_construct(-1, max_vlans);
            if (VTSS_RC_OK != rc) {
                T_W("Operation failed. Try to disable GVRP first");
                return rc;
            }
            rc = xxrp_mgmt_global_enabled_set(VTSS_GARP_APPL_GVRP, 1);
        } else {
            rc = xxrp_mgmt_global_enabled_set(VTSS_GARP_APPL_GVRP, 0);
            GVRP_CRIT_ENTER();
            vtss_gvrp_destruct(FALSE);
            GVRP_CRIT_EXIT();
        }
    } else {
        if (enable) {
            return XXRP_ERROR_APPL_OVERLAP;
        }
    }

    if (rc) {
        T_D("GVRP global enable.2 failed, rc=%d", rc);
        return rc;
    }

    T_N("GVRP global enable.2: Exit OK");
    return VTSS_RC_OK;
}

mesa_rc gvrp_max_vlans_set(int max_vlans)
{
    return vtss_gvrp_max_vlans_set(max_vlans);
}

int gvrp_max_vlans_get(void)
{
    return vtss_gvrp_max_vlans();
}

int gvrp_enable_get(void)
{
    return vtss_gvrp_is_enabled();
}

static void funktor_portvlan(icli_stack_port_range_t *plist,
                             icli_unsigned_range_t *v_vlan_list,
                             mesa_rc (*F)(int, u16))
{
    u32 i;
    mesa_rc rc;

    //  Loop over switch, port, vlans
    for (i = 0; i < plist->cnt; ++i) {

        u16 isid = plist->switch_range[i].switch_id;

        u16 portBegin = plist->switch_range[i].begin_iport;
        u16 portEnd = portBegin + plist->switch_range[i].port_cnt;
        u16 iport;

        for (iport = portBegin; iport < portEnd; ++iport) {

            u32 j;
            int port_no;

            port_no = L2PORT2PORT(isid, iport);

            for (j = 0; j < v_vlan_list->cnt; ++j) {

                unsigned int vlan_min = v_vlan_list->range[j].min;
                unsigned int vlan_max = v_vlan_list->range[j].max;
                unsigned int vlan;

                for (vlan = vlan_min; vlan <= vlan_max; ++vlan) {

                    rc = F(port_no, vlan);
                    if (rc) {
                        T_E("rc=%d", rc);
                    }
                }
            }
        }
    }

    return;
}

static void funktor_port(icli_stack_port_range_t *plist,
                         void (*F)(vtss_isid_t, mesa_port_no_t, int), int enable)
{
    u32 i;

    //  Loop over switch, port, vlans
    for (i = 0; i < plist->cnt; ++i) {

        u16 portBegin = plist->switch_range[i].begin_iport;
        u16 portEnd = portBegin + plist->switch_range[i].port_cnt;
        u16 iport;

        for (iport = portBegin; iport < portEnd; ++iport) {
            F(plist->switch_range[i].isid, (mesa_port_no_t)iport, enable);
        }
    }

    return;
}



void gvrp_join_request(icli_stack_port_range_t *plist, icli_unsigned_range_t *v_vlan_list)
{
    GVRP_CRIT_ENTER();
    funktor_portvlan(plist, v_vlan_list, vtss_gvrp_join_request);
    GVRP_CRIT_EXIT();
}



void gvrp_leave_request(icli_stack_port_range_t *plist, icli_unsigned_range_t *v_vlan_list)
{
    GVRP_CRIT_ENTER();
    funktor_portvlan(plist, v_vlan_list, vtss_gvrp_leave_request);
    GVRP_CRIT_EXIT();
}

#if 0
static void port_enable(int port, int enable)
{
    vtss_isid_t isid = 1;
    mesa_port_no_t iport = port;
    mesa_rc rc;


    rc = xxrp_mgmt_enabled_set(isid, iport, VTSS_GARP_APPL_GVRP, enable);
    T_N("port_enable(%d, %d) rc=%d", port, enable, rc);
}
#endif

static void port_enable(vtss_isid_t isid, mesa_port_no_t iport, int enable)
{
    mesa_rc rc;

    rc = xxrp_mgmt_enabled_set(isid, iport, VTSS_GARP_APPL_GVRP, enable);
    if (rc) {
        T_E("rc=%d", rc);
    }

    T_N("port_enable(%d, %d) rc=%d", iport, enable, rc);
}

void gvrp_port_enable(icli_stack_port_range_t *plist, int enable)
{
    funktor_port(plist, port_enable, enable);
}

mesa_rc gvrp_icli_debug_global_print(const i32 session_id)
{
    BOOL                     enable;
    icli_switch_port_range_t spr;

    VTSS_RC(xxrp_mgmt_global_enabled_get(VTSS_GARP_APPL_GVRP, &enable));
    ICLI_PRINTF("Global enable : ");
    ICLI_PRINTF("%s\n", enable ? "TRUE" : "FALSE");
    ICLI_PRINTF("Max VLANs : %d\n", vtss_gvrp_max_vlans());
    ICLI_PRINTF("Join time : %u\n", vtss_gvrp_get_timer(GARP_TC__transmitPDU));
    ICLI_PRINTF("Leave time : %u\n", vtss_gvrp_get_timer(GARP_TC__leavetimer));
    ICLI_PRINTF("Leave all time : %u\n", vtss_gvrp_get_timer(GARP_TC__leavealltimer));
    ICLI_PRINTF("\n");

    memset(&spr, 0, sizeof(spr));
    while (icli_port_get_next(&spr)) {
        if (xxrp_mgmt_enabled_get(spr.isid, spr.begin_iport, VTSS_GARP_APPL_GVRP, &enable) == VTSS_RC_OK) {
            ICLI_PRINTF("%s %u/%u : ", icli_port_type_get_name(spr.port_type), spr.switch_id, spr.begin_port);
            ICLI_PRINTF("%s\n", enable ? "TRUE" : "FALSE");
        }
    }
    return VTSS_RC_OK;
}

mesa_rc gvrp_icli_debug_msti(const i32 session_id)
{
    gvrp_dump_msti_state();

    return VTSS_RC_OK;
}

mesa_rc gvrp_icli_debug_internal_statistic(const i32 session_id)
{
    vtss_gvrp_internal_statistic();

    return VTSS_RC_OK;
}

mesa_rc gvrp_icli_timer_conf_set(const i32 session_id, bool has_join_time, u32 join_time,
                                 bool has_leave_time, u32 leave_time,
                                 bool has_leave_all_time, u32 leave_all_time)
{
    mesa_rc rc;

    if (has_join_time) {
        rc = vtss_gvrp_set_timer(GARP_TC__transmitPDU, join_time);
        if (rc) {
            ICLI_PRINTF("Failed: Set join-time");
        }
    }
    if (has_leave_time) {
        rc = vtss_gvrp_set_timer(GARP_TC__leavetimer, leave_time);
        if (rc) {
            ICLI_PRINTF("Failed: Set Leave-time");
        }
    }
    if (has_leave_all_time) {
        rc = vtss_gvrp_set_timer(GARP_TC__leavealltimer, leave_all_time);
        if (rc) {
            ICLI_PRINTF("Failed: Set LeaveAll-time");
        }
    }

    return VTSS_RC_OK;
}

mesa_rc gvrp_icli_timer_conf_def(const i32 session_id)
{
    mesa_rc rc;

    rc = vtss_gvrp_set_timer(GARP_TC__transmitPDU, 20);
    if (rc) {
        ICLI_PRINTF("Failed: Set join-time");
    }
    rc = vtss_gvrp_set_timer(GARP_TC__leavetimer, 60);
    if (rc) {
        ICLI_PRINTF("Failed: Set Leave-time");
    }
    rc = vtss_gvrp_set_timer(GARP_TC__leavealltimer, 1000);
    if (rc) {
        ICLI_PRINTF("Failed: Set LeaveAll-time");
    }

    return VTSS_RC_OK;
}
#endif
