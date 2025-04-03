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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "pvlan_api.h"
#include "pvlan_icfg.h"
#include "misc_api.h" /* For uport2iport()    */
#include "topo_api.h" /* For topo_usid2isid() */

#if defined(PVLAN_SRC_MASK_ENA)
#include "mgmt_api.h" /* For mgmt_list2txt()  */
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PVLAN

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define PVLAN_IDS_BUF_LEN       256

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/
static mesa_rc PVLAN_ICFG_port_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    mesa_rc                     rc = VTSS_RC_OK;
    vtss_isid_t                 isid = topo_usid2isid(req->instance_id.port.usid);
    mesa_port_no_t              iport = uport2iport(req->instance_id.port.begin_uport);
    int                         conf_changed = 0;
    mesa_port_list_t            member;
#if defined(PVLAN_SRC_MASK_ENA)
    u32                         pvlan_id;
    pvlan_mgmt_entry_t          conf;
    char                        buf[PVLAN_IDS_BUF_LEN];
    BOOL                        member_of_at_least_one_pvlan_id = FALSE;
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> pvlan_ids;
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> def_pvlan_ids;
    u32                        pvlan_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
#endif

    if (req->instance_id.port.port_type == ICLI_PORT_TYPE_CPU) {
        return rc;
    }

    if ((rc = pvlan_mgmt_isolate_conf_get(isid, member)) != VTSS_RC_OK) {
        return rc;
    }

    conf_changed = (member[iport] == 0) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc = vtss_icfg_printf(result, " %s%s\n", ((member[iport] != 0) ? "" : PVLAN_NO_FORM_TEXT), PVLAN_PORT_ISOLATE_TEXT);
    }

#if defined(PVLAN_SRC_MASK_ENA)
    def_pvlan_ids[0] = TRUE;
    conf_changed = FALSE;
    for (pvlan_id = 0; pvlan_id < pvlan_cnt; pvlan_id++) {
        if (pvlan_mgmt_pvlan_get(isid, pvlan_id, &conf, 0) != VTSS_RC_OK) {
            continue;
        }

        if (conf.ports[iport]) {
            member_of_at_least_one_pvlan_id = TRUE;
            pvlan_ids[pvlan_id] = TRUE;
        }
        if (pvlan_ids[pvlan_id] != def_pvlan_ids[pvlan_id]) {
            conf_changed = TRUE;
        }
    }

    if (req->all_defaults || conf_changed) {
        if (pvlan_ids[0] == 0) {
            rc = vtss_icfg_printf(result, " no pvlan 1\n");
        }

        if (member_of_at_least_one_pvlan_id) {
            rc = vtss_icfg_printf(result, " %s %s\n", PVLAN_PORT_MEMBER_TEXT, mgmt_list2txt(pvlan_ids.data(), 0, pvlan_cnt - 1, buf));
        }
    }
#endif

    return (rc == VTSS_RC_OK) ? rc : VTSS_RC_ERROR;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc PVLAN_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       1. Port configuration
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_PVLAN_PORT_CONF, "pvlan", PVLAN_ICFG_port_conf);

    return rc;
}
