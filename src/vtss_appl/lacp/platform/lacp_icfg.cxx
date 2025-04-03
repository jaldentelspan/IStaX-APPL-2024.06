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

#include "icfg_api.h"
#include "misc_api.h"
#include "icli_api.h"
#include "icli_porting_util.h"
#include "lacp_api.h"
#include "lacp_icfg.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_LACP

//******************************************************************************
// ICFG callback functions
//******************************************************************************

static mesa_rc lacp_icfg_global_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    vtss_lacp_system_config_t sysconf;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Global level: lacp system-priority <prio> */

    if (lacp_mgmt_system_conf_get(&sysconf) != VTSS_RC_OK) {
        T_E("Could not get LACP sysconf\n");
        return VTSS_RC_ERROR;
    }

    if (req->all_defaults || sysconf.system_prio != VTSS_LACP_DEFAULT_SYSTEMPRIO) {
        VTSS_RC(vtss_icfg_printf(result, "lacp system-priority %u\n", sysconf.system_prio));
    }
    return VTSS_RC_OK;
}

static mesa_rc lacp_icfg_intf_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_rc                 rc;
    vtss_lacp_port_config_t conf;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Interface level: lacp, lacp port-prio, lacp active|passive, lacp key <key>, lacp timeout fast|slow */

    if ((rc = lacp_mgmt_port_conf_get(req->instance_id.port.isid, req->instance_id.port.begin_iport, &conf)) != VTSS_RC_OK) {
        T_E("Could not get LACP portconf\n");
        return rc;
    }

    if (req->all_defaults || conf.port_prio != VTSS_LACP_DEFAULT_PORTPRIO) {
        VTSS_RC(vtss_icfg_printf(result, " lacp port-priority %u\n", conf.port_prio));
    }

    if (conf.xmit_mode != VTSS_LACP_DEFAULT_FSMODE || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " lacp timeout %s\n", conf.xmit_mode == VTSS_LACP_FSMODE_SLOW ? "slow" : "fast"));
    }

    return rc;
}

static mesa_rc lacp_icfg_llag_conf(const vtss_icfg_query_request_t *req,
                                   vtss_icfg_query_result_t *result)
{
    mesa_rc                  rc = VTSS_RC_OK;
    aggr_mgmt_group_no_t     aggr_no;
    vtss_ifindex_t           ifindex;
    vtss_appl_aggr_group_status_t grp_status;
    vtss_appl_lacp_group_conf_t   conf;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* llag Interface level */

    aggr_no = req->instance_id.llag_no;

    VTSS_RC(vtss_ifindex_from_llag(VTSS_ISID_START, aggr_no, &ifindex));

    if ((rc = vtss_appl_aggregation_status_get(ifindex, &grp_status)) != VTSS_RC_OK) {
        return rc;
    }

    if (grp_status.mode != VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE  &&
        grp_status.mode != VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE &&
        grp_status.mode != VTSS_APPL_AGGR_GROUP_MODE_RESERVED) {
        return VTSS_RC_OK;
    }
    if ((rc = vtss_appl_lacp_group_conf_get(aggr_no, &conf)) != VTSS_RC_OK) {
        T_E("Could not get LACP group conf\n");
        return rc;
    }

    if (req->all_defaults || conf.revertive != VTSS_LACP_DEFAULT_REVERTIVE) {
        VTSS_RC(vtss_icfg_printf(result, " lacp failover %s\n", conf.revertive ? "revertive" : "non-revertive"));
    }

    if (req->all_defaults || conf.max_bundle != VTSS_LACP_MAX_PORTS_IN_AGGR) {
        VTSS_RC(vtss_icfg_printf(result, " lacp max-bundle %d\n", conf.max_bundle));
    }
    return rc;

}

//******************************************************************************
//   Public functions
//******************************************************************************

mesa_rc lacp_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module. */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_LACP_INTERFACE_CONF, "lacp", lacp_icfg_intf_conf)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_LACP_GLOBAL_CONF, "lacp", lacp_icfg_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_LACP_INTERFACE_LLAG_CONF, "lacp", lacp_icfg_llag_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}

