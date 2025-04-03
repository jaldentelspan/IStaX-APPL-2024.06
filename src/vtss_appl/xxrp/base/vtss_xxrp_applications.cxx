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

#include "vtss_xxrp_applications.h"
#include "vtss_xxrp_callout.h"
#ifdef VTSS_SW_OPTION_GVRP
#include "vtss_gvrp.h"
#endif

mesa_rc mrp_appl_add_to_database(vtss_mrp_t *apps[], vtss_mrp_t *application)
{
    apps[application->appl_type] = application;
    apps[application->appl_type]->mad = (vtss_mrp_mad_t **)XXRP_SYS_MALLOC(sizeof(vtss_mrp_mad_t *) * L2_MAX_PORTS_);
    apps[application->appl_type]->map = (vtss_mrp_map_t **)XXRP_SYS_MALLOC(sizeof(vtss_mrp_map_t *) * L2_MAX_PORTS_);
    memset(apps[application->appl_type]->mad, 0, (sizeof(vtss_mrp_mad_t *) * L2_MAX_PORTS_));
    memset(apps[application->appl_type]->map, 0, (sizeof(vtss_mrp_map_t *) * L2_MAX_PORTS_));
    return VTSS_RC_OK;
}

mesa_rc mrp_appl_del_from_database(vtss_mrp_t *apps[], vtss_mrp_appl_t appl_type)
{
    u32 rc = VTSS_RC_OK, tmp;
    /* Release all MAD & MAP pointers */
    for (tmp = 0; tmp < L2_MAX_PORTS_; tmp++) {
        if (apps[appl_type]->mad[tmp]) {
            if (apps[appl_type]->mad[tmp]->machines) {
                rc += XXRP_SYS_FREE(apps[appl_type]->mad[tmp]->machines);
                apps[appl_type]->mad[tmp]->machines = NULL;
            }
            rc += XXRP_SYS_FREE(apps[appl_type]->mad[tmp]);
            apps[appl_type]->mad[tmp] = NULL;
        }
        if (apps[appl_type]->map[tmp]) {
            rc += XXRP_SYS_FREE(apps[appl_type]->map[tmp]);
            apps[appl_type]->map[tmp] = NULL;
        }
    }
    rc += XXRP_SYS_FREE(apps[appl_type]->mad);
    apps[appl_type]->mad = NULL;

    rc += XXRP_SYS_FREE(apps[appl_type]->map);
    apps[appl_type]->map = NULL;

    rc += XXRP_SYS_FREE(apps[appl_type]);
    apps[appl_type] = NULL;
    if (rc != VTSS_RC_OK) {
        rc = XXRP_ERROR_UNKNOWN;
    }


#ifdef VTSS_SW_OPTION_GVRP
    if (VTSS_GARP_APPL_GVRP == appl_type) {
        vtss_gvrp_destruct(FALSE);
    }

#endif

    return rc;
}
