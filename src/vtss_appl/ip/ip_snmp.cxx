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

#include "ip_api.h"
#include "ip_snmp.h"
#include "ip_utils.hxx"
#include "ip_trace.h"
#include "critd_api.h"
#include "vtss/basics/map.hxx"

using namespace vtss;

#ifdef VTSS_SW_OPTION_SNMP

static critd_t IP_SNMP_crit;

#define IP_SNMP_CRIT_ENTER() critd_enter(&IP_SNMP_crit, __FILE__, __LINE__)
#define IP_SNMP_CRIT_EXIT()  critd_exit( &IP_SNMP_crit, __FILE__, __LINE__)

#define IP_SNMP_CRIT_RETURN(T, X) \
do {                              \
    T __val = (X);                \
    IP_SNMP_CRIT_EXIT();          \
    return __val;                 \
} while(0)

#define IP_SNMP_CRIT_RETURN_RC(X) IP_SNMP_CRIT_RETURN(mesa_rc, X)

#define IP_SNMP_CRIT_RETURN_VOID() \
    IP_SNMP_CRIT_EXIT();       \
    return

typedef struct {
    bool                active;

    bool                has_ipv4;
    mesa_ipv4_network_t ipv4;
    vtss_tick_count_t   ipv4_created;
    vtss_tick_count_t   ipv4_changed;

    bool                has_ipv6;
    mesa_ipv6_network_t ipv6;
    vtss_tick_count_t   ipv6_created;
    vtss_tick_count_t   ipv6_changed;
} if_snmp_status_t;

static vtss_tick_count_t IP_SNMP_if_table_changed;

// TODO, use a map instead
static Map<vtss_ifindex_t, if_snmp_status_t> IP_SNMP_status;

static void IP_SNMP_if_change_callback(vtss_ifindex_t ifidx)
{
    vtss_appl_ip_if_status_t st;
    bool exists;

    IP_SNMP_CRIT_ENTER();
    exists = vtss_appl_ip_if_exists(ifidx);

    if ((!IP_SNMP_status[ifidx].active) && exists) {
        T_DG(IP_TRACE_GRP_SNMP, "vlan created %u", VTSS_IFINDEX_PRINTF_ARG(ifidx));
        IP_SNMP_status[ifidx].active = TRUE;
        IP_SNMP_if_table_changed = vtss_current_time();
    }

    if (IP_SNMP_status[ifidx].active && (!exists)) {
        T_DG(IP_TRACE_GRP_SNMP, "vlan deleted %u", VTSS_IFINDEX_PRINTF_ARG(ifidx));
        memset(&IP_SNMP_status[ifidx], 0, sizeof(if_snmp_status_t));
        IP_SNMP_if_table_changed = vtss_current_time();
        IP_SNMP_CRIT_RETURN_VOID();
    }

    T_IG(IP_TRACE_GRP_SNMP, "vlan = %u", VTSS_IFINDEX_PRINTF_ARG(ifidx));
    if (vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &st) == VTSS_RC_OK) {
        if (!IP_SNMP_status[ifidx].has_ipv4) {
            // IPv4 address created
            T_DG(IP_TRACE_GRP_SNMP, "ipv4 address created %u : %s", VTSS_IFINDEX_PRINTF_ARG(ifidx), st.u.ipv4.net);
            IP_SNMP_status[ifidx].ipv4_created = vtss_current_time();
            IP_SNMP_status[ifidx].ipv4_changed = vtss_current_time();

        } else if (vtss_ipv4_network_equal(&IP_SNMP_status[ifidx].ipv4,
                                           &st.u.ipv4.net)) {
            // IPv4 address changed
            T_DG(IP_TRACE_GRP_SNMP, "ipv4 address changed %u : %s", VTSS_IFINDEX_PRINTF_ARG(ifidx), st.u.ipv4.net);
            IP_SNMP_status[ifidx].ipv4_changed = vtss_current_time();
        }

        IP_SNMP_status[ifidx].ipv4 = st.u.ipv4.net;
        IP_SNMP_status[ifidx].has_ipv4 = TRUE;

    } else {
        if (IP_SNMP_status[ifidx].has_ipv4) {
            T_DG(IP_TRACE_GRP_SNMP, "ipv4 address deleted %u", VTSS_IFINDEX_PRINTF_ARG(ifidx));
        }

        IP_SNMP_status[ifidx].has_ipv4 = FALSE;
    }

    if (vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV6, 1, nullptr, &st) == VTSS_RC_OK) {

        if (!IP_SNMP_status[ifidx].has_ipv6) {
            T_DG(IP_TRACE_GRP_SNMP, "ipv6 address created %u : %s", VTSS_IFINDEX_PRINTF_ARG(ifidx), st.u.ipv6.net);
            IP_SNMP_status[ifidx].ipv6_created = vtss_current_time();
            IP_SNMP_status[ifidx].ipv6_changed = vtss_current_time();

        } else if (vtss_ipv6_network_equal(&IP_SNMP_status[ifidx].ipv6, &st.u.ipv6.net)) {
            T_DG(IP_TRACE_GRP_SNMP, "ipv6 address changed %u : %s", VTSS_IFINDEX_PRINTF_ARG(ifidx), st.u.ipv6.net);
            IP_SNMP_status[ifidx].ipv6_changed = vtss_current_time();
        }

        IP_SNMP_status[ifidx].ipv6 = st.u.ipv6.net;
        IP_SNMP_status[ifidx].has_ipv6 = TRUE;

    } else {
        if (IP_SNMP_status[ifidx].has_ipv6) {
            T_DG(IP_TRACE_GRP_SNMP, "ipv6 address deleted %u", VTSS_IFINDEX_PRINTF_ARG(ifidx));
        }

        IP_SNMP_status[ifidx].has_ipv6 = false;
    }

    IP_SNMP_CRIT_RETURN_VOID();
}

mesa_rc vtss_ip_snmp_init()
{
    mesa_rc rc;

    critd_init(&IP_SNMP_crit, "ip.snmp", VTSS_MODULE_ID_IP, CRITD_TYPE_MUTEX);

    T_IG(IP_TRACE_GRP_SNMP, "vtss_ip_snmp_init");

    if ((rc = vtss_ip_if_callback_add(IP_SNMP_if_change_callback)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_SNMP, "Failed to register callback");
    }

    return rc;
}

void vtss_ip_snmp_signal_global_changes()
{
    IP_SNMP_CRIT_ENTER();
    IP_SNMP_if_table_changed = vtss_current_time();
    IP_SNMP_CRIT_EXIT();
}

mesa_rc vtss_ip_interfaces_last_change(u64 *time)
{
    IP_SNMP_CRIT_ENTER();
    T_DG(IP_TRACE_GRP_SNMP, "vtss_ip_interfaces_last_change");
    *time = VTSS_OS_TICK2MSEC(IP_SNMP_if_table_changed) / 1000LLU;
    IP_SNMP_CRIT_RETURN_RC(VTSS_RC_OK);
}

mesa_rc vtss_ip_address_created_ipv4(vtss_ifindex_t ifidx, u64 *time)
{
    mesa_rc rc = VTSS_RC_ERROR;

    IP_SNMP_CRIT_ENTER();
    T_DG(IP_TRACE_GRP_SNMP, "%s", ifidx);
    if (IP_SNMP_status[ifidx].has_ipv4) {
        *time = VTSS_OS_TICK2MSEC(IP_SNMP_status[ifidx].ipv4_created) / 1000LLU;
        rc = VTSS_RC_OK;
    }

    IP_SNMP_CRIT_RETURN_RC(rc);
}

mesa_rc vtss_ip_address_changed_ipv4(vtss_ifindex_t ifidx, u64 *time)
{
    mesa_rc rc = VTSS_RC_ERROR;

    IP_SNMP_CRIT_ENTER();
    T_DG(IP_TRACE_GRP_SNMP, "%s", ifidx);
    if (IP_SNMP_status[ifidx].has_ipv4) {
        *time = VTSS_OS_TICK2MSEC(IP_SNMP_status[ifidx].ipv4_changed) / 1000LLU;
        rc = VTSS_RC_OK;
    }

    IP_SNMP_CRIT_RETURN_RC(rc);
}

mesa_rc vtss_ip_address_created_ipv6(vtss_ifindex_t ifidx, u64 *time)
{
    mesa_rc rc = VTSS_RC_ERROR;

    IP_SNMP_CRIT_ENTER();
    T_DG(IP_TRACE_GRP_SNMP, "%s", ifidx);
    if (IP_SNMP_status[ifidx].has_ipv6) {
        *time = VTSS_OS_TICK2MSEC(IP_SNMP_status[ifidx].ipv6_created) / 1000LLU;
        rc = VTSS_RC_OK;
    }

    IP_SNMP_CRIT_RETURN_RC(rc);
}

mesa_rc vtss_ip_address_changed_ipv6(vtss_ifindex_t ifidx, u64 *time)
{
    mesa_rc rc = VTSS_RC_ERROR;

    IP_SNMP_CRIT_ENTER();
    T_DG(IP_TRACE_GRP_SNMP, "%s", ifidx);
    if (IP_SNMP_status[ifidx].has_ipv6) {
        *time = VTSS_OS_TICK2MSEC(IP_SNMP_status[ifidx].ipv6_changed) / 1000LLU;
        rc = VTSS_RC_OK;
    }

    IP_SNMP_CRIT_RETURN_RC(rc);
}

#endif /* VTSS_SW_OPTION_SNMP */

