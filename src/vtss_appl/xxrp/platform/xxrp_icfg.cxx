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

/*****************************************************************************
 * Include files
 *****************************************************************************/
#include "icfg_api.h"
#include "xxrp_api.h"
#include "xxrp_trace.h"
#include "vtss/basics/trace.hxx"
#include "vtss_gvrp.h"
#include "vtss_mrp.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_XXRP

/***************************************************************************/
/* ICFG callback functions */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_ICFG

#ifdef VTSS_SW_OPTION_MRP
static mesa_rc mrp_icfg_interface_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_rc               rc       = VTSS_RC_OK;
    bool                  periodic = false;
    vtss_mrp_timer_conf_t timers;

    u16 isid = req->instance_id.port.isid;
    u16 iport = req->instance_id.port.begin_iport;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "ICFG is building MRP interface #"
                                                   << req->instance_id.port.begin_uport << " configuration";

    if ((rc = mrp_mgmt_timers_get(isid, iport, &timers)) != VTSS_RC_OK) {
        return rc == XXRP_ERROR_PORT ? VTSS_RC_OK : rc; /* Ignore stack port errors */
    }
    if (timers.join_timer != VTSS_MRP_JOIN_TIMER_DEF ||
        timers.leave_timer != VTSS_MRP_LEAVE_TIMER_DEF ||
        timers.leave_all_timer != VTSS_MRP_LEAVE_ALL_TIMER_DEF || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result,
                                 " mrp timers join-time %d leave-time %d leave-all-time %d\n",
                                 timers.join_timer,
                                 timers.leave_timer,
                                 timers.leave_all_timer));
    }
    VTSS_RC(vtss::mrp::mgmt_periodic_state_get(isid, iport, periodic));
    if (periodic || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " %smrp periodic\n", periodic ? "" : "no "));
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_MRP */

#ifdef VTSS_SW_OPTION_MVRP
static mesa_rc mvrp_icfg_managed_vlan_print(vtss_icfg_query_result_t *result, char *keyword, vtss::VlanList &vls)
{
    mesa_rc rc;
    char    *buf = NULL;

    if (keyword == NULL) {
        // Caller doesn't have a special keyword to print, so print the #vid_bitmask.
        if ((buf = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, ERROR) << "Out of memory";
            return VTSS_RC_ERROR;
        }
        (void)xxrp_mgmt_vlan_list_to_txt(vls, buf);
    }

    rc = vtss_icfg_printf(result, "mvrp managed vlan %s\n", keyword ? (char *)keyword : buf);

    if (buf) {
        VTSS_FREE(buf);
    }

    return rc;
}

static mesa_rc mvrp_icfg_global_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    BOOL                  enable, is_all_zeros = FALSE;
    vtss::VlanList        vls, vls_all;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "ICFG is building MVRP global configuration";

    VTSS_RC(xxrp_mgmt_global_managed_vids_get(VTSS_MRP_APPL_MVRP, vls));
    if (req->all_defaults) {
        // Here, we always print the whole range, whether it's all VLANs or just partial set.
        // There is one exception, which is when it's empty, in which case we print "none"
        vls_all.clear_all();
        is_all_zeros = (vls == vls_all);
        VTSS_RC(mvrp_icfg_managed_vlan_print(result, (char *)(is_all_zeros ? "none" : NULL), vls));
    } else {
        vls_all.clear_all();
        for (uint i = 1; i < VTSS_APPL_VLAN_ID_MAX; ++i) {
            vls_all.set(i);
        }
        if (vls != vls_all) {
            // The current vlan list is not default. Gotta print a string.
            vls_all.clear_all();
            is_all_zeros = (vls == vls_all);
            VTSS_RC(mvrp_icfg_managed_vlan_print(result, (char *)(is_all_zeros ? "none" : NULL), vls));
        }
    }

    VTSS_RC(xxrp_mgmt_global_enabled_get(VTSS_MRP_APPL_MVRP, &enable));
    if (enable) {
        VTSS_RC(vtss_icfg_printf(result, "mvrp\n"));
    } else {
        if (req->all_defaults) {
            VTSS_RC(vtss_icfg_printf(result, "no mvrp\n"));
        }
    }

    return VTSS_RC_OK;
}

static mesa_rc mvrp_icfg_interface_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_rc rc;
    BOOL    enable;

    u16 isid = req->instance_id.port.isid;
    u16 iport = req->instance_id.port.begin_iport;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "ICFG is building MVRP interface #"
                                                   << req->instance_id.port.begin_uport << " configuration";
    if ((rc = xxrp_mgmt_enabled_get(isid, iport, VTSS_MRP_APPL_MVRP, &enable)) != VTSS_RC_OK) {
        return rc == XXRP_ERROR_PORT ? VTSS_RC_OK : rc; /* Ignore stack port errors */
    }
    if (enable || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " %smvrp\n", enable ? "" : "no "));
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_MVRP */

#ifdef VTSS_SW_OPTION_GVRP
static mesa_rc gvrp_icfg_global_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    BOOL enable;
    int  max_vlans;
    u32  jt, lt, lat;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "ICFG is building GVRP global configuration";
    // --- Get GVRP global enable state
    VTSS_RC(xxrp_mgmt_global_enabled_get(VTSS_GARP_APPL_GVRP, &enable));

    max_vlans = vtss_gvrp_max_vlans();
    if (max_vlans != VTSS_MRP_VLAN_MAX_DEF || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, "gvrp max-vlans %d\n", max_vlans));
    }

    if (enable || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, "%sgvrp\n", enable ? "" : "no "));
    }

    // --- Get protocol timers
    jt  = vtss_gvrp_get_timer(GARP_TC__transmitPDU);
    lt  = vtss_gvrp_get_timer(GARP_TC__leavetimer);
    lat = vtss_gvrp_get_timer(GARP_TC__leavealltimer);
    if (jt != VTSS_MRP_JOIN_TIMER_DEF || lt != VTSS_MRP_LEAVE_TIMER_DEF || lat != VTSS_MRP_LEAVE_ALL_TIMER_DEF || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, "gvrp time join-time %d leave-time %d leave-all-time %d\n", jt, lt, lat));
    }

    return VTSS_RC_OK;
}

static mesa_rc gvrp_icfg_interface_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_rc rc;
    BOOL    enable;

    u16 isid = req->instance_id.port.isid;
    u16 iport = req->instance_id.port.begin_iport;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "ICFG is building GVRP interface #"
                                                   << req->instance_id.port.begin_uport << " configuration";
    if ((rc = xxrp_mgmt_enabled_get(isid, iport, VTSS_GARP_APPL_GVRP, &enable))) {
        return rc == XXRP_ERROR_PORT ? VTSS_RC_OK : rc;     // Ignore stack port errors
    }
    if (enable || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " %sgvrp\n", enable ? "" : "no "));
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_GVRP */

/* ICFG Initialization function */
mesa_rc xxrp_icfg_init(void)
{
#ifdef VTSS_SW_OPTION_MRP
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_MRP_INTERFACE_CONF, "MRP",
            mrp_icfg_interface_conf));
#endif
#ifdef VTSS_SW_OPTION_MVRP
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_MVRP_GLOBAL_CONF, "MVRP", mvrp_icfg_global_conf));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_MVRP_INTERFACE_CONF, "MVRP", mvrp_icfg_interface_conf));
#endif
#ifdef VTSS_SW_OPTION_GVRP
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_GVRP_GLOBAL_CONF, "GVRP", gvrp_icfg_global_conf));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_GVRP_INTERFACE_CONF, "GVRP", gvrp_icfg_interface_conf));
#endif
    return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG
