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
#include "vtss_ssh_api.h"

static mesa_rc VTSS_SSH_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                         vtss_icfg_query_result_t *result)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_appl_ssh_conf_t conf;

    if ((rc = vtss_appl_ssh_conf_get(&conf)) != VTSS_RC_OK) {
        return rc;
    }

    if (req->all_defaults || (conf.mode != SSH_MGMT_DEF_MODE)) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              conf.mode == SSH_MGMT_ENABLED ? "" : "no ",
                              "ip ssh");
    }

    return rc;
}

mesa_rc vtss_ssh_icfg_init(void)
{
    return vtss_icfg_query_register(VTSS_SSH_ICFG_GLOBAL_CONF, "ssh",
                                    VTSS_SSH_ICFG_global_conf);
}
