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

#include "lacp_serializer.hxx"
#include "vtss/appl/aggr.h"
#include "vtss/appl/lacp.h"
#include "vtss_common_iterator.hxx"
#include "lacp.h"

mesa_rc lacp_ifaceport_idx_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex)
{
    return vtss_appl_iterator_ifindex_front_port_exist(prev_ifindex, next_ifindex);
}

/*
 * Get next ifindex for an LACP-enabled aggregation group
 */
mesa_rc lacp_ifacegroup_idx_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex)
{
    mesa_rc rc;
    vtss_ifindex_t *prev_ = (vtss_ifindex_t *)prev_ifindex;     // we need prev index to be not const
    vtss_appl_aggr_group_conf_t gconf;

    while (TRUE) {
        /*
         * Get ifindex for next aggregation group. Any LACP group is an aggregation
         * group but not all aggregation groups are LACP groups.
         */
        rc = aggregation_iface_itr_lag_idx(prev_, next_ifindex);
        if (rc != VTSS_RC_OK) {
            // We ran out of groups
            break;
        }

        // Get group configuration
        if ((rc = vtss_appl_aggregation_group_get(*next_ifindex, &gconf)) != VTSS_RC_OK) {
            // This should not happen
            T_D("Failed to get aggr info for %u", next_ifindex->private_ifindex_data_do_not_use_directly);
            return rc;
        }

        // Check that it is an LACP group
        if (gconf.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE || gconf.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE) {
            break;
        }

        // If not LACP then try next group
        prev_ = next_ifindex;
    }

    return rc;
}

mesa_rc lacp_port_stats_clr_set( vtss_ifindex_t ifindex, const BOOL *const clear)
{
    if (clear && *clear) {
        return vtss_lacp_port_stats_clr(ifindex);
    }
    return VTSS_RC_OK;
}

mesa_rc lacp_port_stats_clr_dummy_get(vtss_ifindex_t ifindex, BOOL *const clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}

