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
#include "vlan_translation_api.h"
#include "vlan_translation_trace.h"
#include "vtss/basics/trace.hxx"

/***************************************************************************/
/* ICFG callback functions */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_ICFG
static mesa_rc vlan_translation_icfg_conf(const vtss_icfg_query_request_t *req,
                                          vtss_icfg_query_result_t *result)
{
    switch (req->cmd_mode) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG: {
        vtss_appl_vlan_translation_group_mapping_key_t *in = NULL, temp = {}, out = {};
        mesa_vid_t tvid;
        const char *dir[3] = {"both", "ingress", "egress"};
        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "ICFG is building VLAN Translation mapping configuration";
        while (vtss_appl_vlan_translation_group_conf_itr(in, &out) == MESA_RC_OK) {
            if (vtss_appl_vlan_translation_group_conf_get(out, &tvid) == MESA_RC_OK) {
                VTSS_RC(vtss_icfg_printf(result, "switchport vlan mapping %d %s %d %d\n", out.gid, dir[(uint)out.dir], out.vid, tvid));
            }
            temp = out;
            in = &temp;
        }
        break;
    }
    case ICLI_CMD_MODE_INTERFACE_PORT_LIST: {
        mesa_port_no_t iport = req->instance_id.port.begin_iport;
        vtss_appl_vlan_translation_if_conf_value_t group;
        VTSS_TRACE(VTSS_TRACE_VT_GRP_MGMT, DEBUG) << "ICFG is building VLAN Translation interface configuration";
        VTSS_RC(vlan_trans_mgmt_port_conf_get(iport + 1, &group));
        // Check if the configuration is the default one
        // and don't print if it is.
        if (!req->all_defaults && group.gid != iport + 1) {
            VTSS_RC(vtss_icfg_printf(result, " switchport vlan mapping %d\n", group.gid));
        } else if (req->all_defaults) {
            VTSS_RC(vtss_icfg_printf(result, " switchport vlan mapping %d\n", group.gid));
        }
        break;
    }
    default:
        break;
    }
    return VTSS_RC_OK;
}

/* ICFG Initialization function */
mesa_rc vlan_translation_icfg_init(void)
{
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_VLAN_TRANSLATION_GLOBAL_CONF, "vlan", vlan_translation_icfg_conf));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_VLAN_TRANSLATION_INTERFACE_CONF, "vlan", vlan_translation_icfg_conf));
    return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG
