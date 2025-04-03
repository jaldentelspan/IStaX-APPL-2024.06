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

#include "main.h"
#include "critd_api.h"
#if defined(VTSS_SW_OPTION_MRP)
#include "vtss/appl/mrp.h"
#endif
#if defined(VTSS_SW_OPTION_MVRP)
#include "vtss/appl/mvrp.h"
#endif
#include "xxrp_serializer.hxx"
#include "xxrp_api.h"
#include "vtss_mrp.hxx"
#include "xxrp_trace.h"
#include "vtss_common_iterator.hxx"

VTSS_CRIT_SCOPE_CLASS_EXTERN(xxrp_appl_crit, VtssXxrpApplCritdGuard);

#define XXRP_APPL_CRIT_SCOPE() VtssXxrpApplCritdGuard __lock_guard__(__LINE__)

#if defined(VTSS_SW_OPTION_MRP)
mesa_rc vtss_appl_mrp_capabilities_get(vtss_appl_mrp_capabilities_t *const cap)
{
    return VTSS_RC_OK;
}

uint32_t MrpCapJoinTimeoutMin::get() { return VTSS_MRP_JOIN_TIMER_MIN; }
uint32_t MrpCapJoinTimeoutMax::get() { return VTSS_MRP_JOIN_TIMER_MAX; }
uint32_t MrpCapLeaveTimeoutMin::get() { return VTSS_MRP_LEAVE_TIMER_MIN; }
uint32_t MrpCapLeaveTimeoutMax::get() { return VTSS_MRP_LEAVE_TIMER_MAX; }
uint32_t MrpCapLeaveAllTimeoutMin::get() { return VTSS_MRP_LEAVE_ALL_TIMER_MIN; }
uint32_t MrpCapLeaveAllTimeoutMax::get() { return VTSS_MRP_LEAVE_ALL_TIMER_MAX; }

mesa_rc vtss_appl_mrp_config_interface_set(vtss_ifindex_t ifindex,
                                           const vtss_appl_mrp_config_interface_t *const config)
{
    vtss_mrp_timer_conf_t timers;
    bool                  periodic = false;
    vtss_ifindex_elm_t    e;
    mesa_rc               rc       = VTSS_RC_OK;

    if (config == NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, WARNING) << "Input parameter 'config' is NULL";
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_ifindex_decompose(ifindex, &e));
    if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Interface " << vtss_ifindex_cast_to_u32(ifindex) << " is not a port interface";
        return XXRP_ERROR_INVALID_IF_TYPE;
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Enter set interface";
    timers.join_timer = config->join_timeout;
    timers.leave_timer = config->leave_timeout;
    timers.leave_all_timer = config->leave_all_timeout;
    /* Check timer values before setting anything */
    VTSS_RC(xxrp_mgmt_timers_check(VTSS_MRP_APPL_MVRP, &timers));
    VTSS_RC(mrp_mgmt_timers_set(e.isid, (mesa_port_no_t)e.ordinal, &timers));
    periodic = (bool) config->periodic_transmission;
    VTSS_RC(vtss::mrp::mgmt_periodic_state_set(e.isid, (mesa_port_no_t)e.ordinal, periodic));
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Exit set interface: rc = " << rc;
    return rc;
}

mesa_rc vtss_appl_mrp_config_interface_get(vtss_ifindex_t ifindex,
                                           vtss_appl_mrp_config_interface_t *config)
{
    vtss_mrp_timer_conf_t timers;
    bool                  periodic = false;
    vtss_ifindex_elm_t    e;
    mesa_rc               rc       = VTSS_RC_OK;

    if (config == NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, WARNING) << "Input parameter 'config' is NULL";
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_ifindex_decompose(ifindex, &e));
    if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Interface " << vtss_ifindex_cast_to_u32(ifindex) << " is not a port interface";
        return XXRP_ERROR_INVALID_IF_TYPE;
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Enter get interface";
    VTSS_RC(mrp_mgmt_timers_get(e.isid, (mesa_port_no_t)e.ordinal, &timers));
    config->join_timeout = timers.join_timer;
    config->leave_timeout = timers.leave_timer;
    config->leave_all_timeout = timers.leave_all_timer;
    VTSS_RC(vtss::mrp::mgmt_periodic_state_get(e.isid, (mesa_port_no_t)e.ordinal, periodic));
    config->periodic_transmission = (BOOL) periodic;
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Exit get interface: rc = " << rc;
    return rc;
}

mesa_rc vtss_appl_mrp_config_interface_itr(const vtss_ifindex_t *const prev_ifindex,
                                           vtss_ifindex_t *const next_ifindex)
{
    mesa_rc rc;

    if (next_ifindex == NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "next_ifindex == NULL";
        return VTSS_RC_ERROR;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Enter interface itr: *prev_ifindex = "
                                               << (prev_ifindex ? vtss_ifindex_cast_to_u32(*prev_ifindex) : -1);
    rc = vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Exit interface itr: (rc = " << rc
                                               << "): *prev_ifindex = "
                                               << (prev_ifindex ? vtss_ifindex_cast_to_u32(*prev_ifindex) : -1)
                                               << ", *next_ifindex = "
                                               << vtss_ifindex_cast_to_u32(*next_ifindex);

    return rc;
}
#endif // defined(VTSS_SW_OPTION_MRP)

#if defined(VTSS_SW_OPTION_MVRP)
mesa_rc vtss_appl_mvrp_config_global_set(const vtss_appl_mvrp_config_global_t *const config)
{
    if (config == NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, WARNING) << "Input is NULL";
        return VTSS_RC_ERROR;
    }

    vtss::VlanList &vls           = (vtss::VlanList &)config->vlans;
    bool           enabled        = (bool) config->state;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Enter set globals";
    /* Lock application crit */
    XXRP_APPL_CRIT_SCOPE();
    /* Check that no other MRP/GARP application is currently enabled */
    if (xxrp_mgmt_appl_exclusion(VTSS_MRP_APPL_MVRP)) {
        /* Another MRP/GARP application is enabled at the moment */
        if (enabled) {
            return XXRP_ERROR_APPL_OVERLAP;
        }
    }

    /* Check vlan list for invalid MVRP vlans, e.g. 0, 4095 */
    for (const auto &v : vls) {
        if (v < 1 || v > 4094) {
            return XXRP_ERROR_INVALID_VID;
        }
    }

    if (enabled) {
        VTSS_RC(xxrp_mgmt_global_managed_vids_set(VTSS_MRP_APPL_MVRP, vls));
        VTSS_RC(xxrp_mgmt_global_enabled_set(VTSS_MRP_APPL_MVRP, config->state));
    } else {
        VTSS_RC(xxrp_mgmt_global_enabled_set(VTSS_MRP_APPL_MVRP, config->state));
        VTSS_RC(xxrp_mgmt_global_managed_vids_set(VTSS_MRP_APPL_MVRP, vls));
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Exit set globals";
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mvrp_config_global_get(vtss_appl_mvrp_config_global_t *config)
{
    if (config == NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, WARNING) << "Input is NULL";
        return VTSS_RC_ERROR;
    }

    vtss::VlanList &vls = (vtss::VlanList &)config->vlans;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Enter get globals";
    VTSS_RC(xxrp_mgmt_global_enabled_get(VTSS_MRP_APPL_MVRP, &config->state));
    VTSS_RC(xxrp_mgmt_global_managed_vids_get(VTSS_MRP_APPL_MVRP, vls));
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Exit get globals";
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mvrp_config_interface_set(vtss_ifindex_t ifindex,
                                            const vtss_appl_mvrp_config_interface_t *const config)
{
    vtss_ifindex_elm_t e;
    mesa_rc            rc = VTSS_RC_OK;

    if (config == NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, WARNING) << "Input parameter 'config' is NULL";
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_ifindex_decompose(ifindex, &e));
    if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Interface " << vtss_ifindex_cast_to_u32(ifindex) << " is not a port interface";
        return XXRP_ERROR_INVALID_IF_TYPE;
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Enter set interface";
    rc = xxrp_mgmt_enabled_set(e.isid, (mesa_port_no_t)e.ordinal, VTSS_MRP_APPL_MVRP, config->state);
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Exit set interface: rc = " << rc;
    return rc;
}

mesa_rc vtss_appl_mvrp_config_interface_get(vtss_ifindex_t ifindex,
                                            vtss_appl_mvrp_config_interface_t *config)
{
    vtss_ifindex_elm_t e;
    mesa_rc            rc;

    if (config == NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, WARNING) << "Input parameter 'config' is NULL";
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_ifindex_decompose(ifindex, &e));
    if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Interface " << vtss_ifindex_cast_to_u32(ifindex) << " is not a port interface";
        return XXRP_ERROR_INVALID_IF_TYPE;
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Enter get interface";
    rc = xxrp_mgmt_enabled_get(e.isid, (mesa_port_no_t)e.ordinal, VTSS_MRP_APPL_MVRP, &(config->state));
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Exit get interface: rc = " << rc;
    return rc;
}

mesa_rc vtss_appl_mvrp_stat_interface_get(vtss_ifindex_t ifindex,
                                          vtss_appl_mvrp_stat_interface_t *stat)
{
    vtss_ifindex_elm_t         e;
    mesa_rc                    rc = VTSS_RC_OK;
    vtss::mrp::MrpApplPortStat status;

    if (stat == NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, WARNING) << "Input parameter 'stat' is NULL";
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_ifindex_decompose(ifindex, &e));
    if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Interface " << vtss_ifindex_cast_to_u32(ifindex) << " is not a port interface";
        return XXRP_ERROR_INVALID_IF_TYPE;
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Enter get interface";
    memset(stat, 0, sizeof(*stat));
    if ((rc = vtss::mrp::mgmt_stat_port_get(e.isid, (mesa_port_no_t)e.ordinal, VTSS_APPL_MRP_APPL_MVRP, status)) == VTSS_RC_OK) {
        stat->failed_registrations = status.failedRegistrations;
        memcpy(&(stat->last_pdu_origin), &(status.lastPduOrigin), sizeof(stat->last_pdu_origin));
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_MIB, DEBUG) << "Exit get interface: rc = " << rc;

    return rc;
}

/* The MRP interface iterator will always be there for all MRP applications
   Therefore call it, instead of duplicating code */
mesa_rc vtss_appl_mvrp_config_interface_itr(const vtss_ifindex_t *const prev_ifindex,
                                            vtss_ifindex_t *const next_ifindex)
{
    VTSS_RC(vtss_appl_mrp_config_interface_itr(prev_ifindex, next_ifindex));
    return VTSS_RC_OK;
}
#endif // defined(VTSS_SW_OPTION_MVRP)
