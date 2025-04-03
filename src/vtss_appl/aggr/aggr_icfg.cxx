/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "topo_api.h"
#include "aggr_api.h"
#include "aggr_icfg.h"

#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_AGGR

//******************************************************************************
// ICFG callback functions
//******************************************************************************

static mesa_rc aggr_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    mesa_rc               rc = VTSS_RC_OK;
    mesa_aggr_mode_t      mode;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Global level: aggregation mode */

    if ((rc = aggr_mgmt_aggr_mode_get(&mode)) != VTSS_RC_OK) {
        T_E("%s\n", aggr_error_txt(rc));
        return 0;
    }

    if (req->all_defaults ||
        ((mode.smac_enable && !mode.dmac_enable && mode.sip_dip_enable && mode.sport_dport_enable) != 1)) {
        VTSS_RC(vtss_icfg_printf(result, "aggregation mode%s%s%s%s\n",
                                 mode.smac_enable ? " smac" : "",
                                 mode.dmac_enable ? " dmac" : "",
                                 mode.sip_dip_enable ? " ip" : "",
                                 mode.sport_dport_enable ? " port" : ""));
    }
    return rc;
}

static mesa_rc aggr_icfg_intf_conf(const vtss_icfg_query_request_t *req,
                                   vtss_icfg_query_result_t *result)
{
    mesa_rc                  rc = VTSS_RC_OK;
    vtss_isid_t              isid;
    mesa_port_no_t           iport;
    aggr_mgmt_group_member_t aggr_static;
    BOOL                     found = 0;
    vtss_ifindex_t           ifindex;
    vtss_appl_aggr_group_status_t  grp_status;
    aggr_mgmt_group_no_t     aggr_no;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Interface level: aggregation group <group_id> */

    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);

    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END_; aggr_no++) {
        VTSS_RC(vtss_ifindex_from_llag(isid, aggr_no, &ifindex));

        if ((rc = vtss_appl_aggregation_status_get(ifindex, &grp_status)) != VTSS_RC_OK) {
            continue;
        }
        if (grp_status.mode != VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE  &&
            grp_status.mode != VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE &&
            grp_status.mode != VTSS_APPL_AGGR_GROUP_MODE_STATIC_ON) {
            continue;
        }
        if (portlist_state_get(isid, iport, &grp_status.cfg_ports.member)) {
            found = TRUE;
            break;
        }
    }

    if (req->all_defaults || found) {
        if (found) {
            VTSS_RC(vtss_icfg_printf(result, " aggregation group %u mode %s\n", aggr_no,
                                     grp_status.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE ? "active" :
                                     grp_status.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE ? "passive" : "on"));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " no aggregation group\n"));
        }
    }

    return rc;
}

//******************************************************************************
//   Public functions
//******************************************************************************

mesa_rc aggr_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module. */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_AGGR_INTERFACE_CONF, "aggregation", aggr_icfg_intf_conf)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_AGGR_GLOBAL_CONF, "aggregation", aggr_icfg_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}
