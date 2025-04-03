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
#include "icli_api.h"
#include "misc_api.h"
#include "udld_api.h"
#include "udld_icfg.h"
#include "topo_api.h"

#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_UDLD

//******************************************************************************
// ICFG callback functions
//******************************************************************************

static mesa_rc udld_icfg_intf_conf ( const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result
                                   )
{
    vtss_isid_t                        isid;
    mesa_port_no_t                     iport;
    vtss_appl_udld_port_conf_struct_t  conf;
    if (!req || !result) {
        return VTSS_RC_ERROR;
    }
    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);
    memset(&conf, 0, sizeof(conf));
    if (vtss_appl_udld_port_conf_get(isid, iport, &conf) != VTSS_RC_OK) {
        T_E("Could not get UDLD port conf\n");
        return VTSS_RC_ERROR;
    }
    if (conf.udld_mode != VTSS_APPL_UDLD_MODE_DISABLE) {
        if (conf.probe_msg_interval != UDLD_DEFAULT_MESSAGE_INTERVAL) {
            VTSS_RC(vtss_icfg_printf(result, " udld port%s message time-interval %u\n", conf.udld_mode == VTSS_APPL_UDLD_MODE_AGGRESSIVE ? " aggressive" : "", conf.probe_msg_interval));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " udld port%s\n", conf.udld_mode == VTSS_APPL_UDLD_MODE_AGGRESSIVE ? " aggressive" : ""));
        }
    } else if (req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " %sudld port\n", conf.udld_mode ? "" : "no "));
    }
    return VTSS_RC_OK;
}

//******************************************************************************
//   Public functions
//******************************************************************************

mesa_rc udld_icfg_init(void)
{
    return vtss_icfg_query_register ( VTSS_ICFG_UDLD_INTERFACE_CONF,
                                      "udld", udld_icfg_intf_conf );
}
