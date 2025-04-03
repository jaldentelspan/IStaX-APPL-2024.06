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
#include "main_conf.hxx"
#include "vtss_optional_modules.hxx"
#include "vtss_ssh_api.h"
#include "vtss_ssh_trace.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "vtss_ssh_icfg.h"
#endif /* VTSS_SW_OPTION_ICFG */
#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif /* VTSS_SW_OPTION_WEB */

static vtss_trace_reg_t SSH_trace_reg = {
    VTSS_TRACE_MODULE_ID, "ssh", "SSH"
};

static vtss_trace_grp_t SSH_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&SSH_trace_reg, SSH_trace_grps);

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void ssh_mib_init(void);
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_ssh_json_init(void);
#endif

extern "C" int vtss_ssh_icli_cmd_register();

mesa_rc ssh_init(vtss_init_data_t *data)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);
    switch (data->cmd) {
    case INIT_CMD_INIT:
        if ((rc = vtss_ssh_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling vtss_ssh_icfg_init() failed rc = %s", error_txt(rc));
        }
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        ssh_mib_init(); /* Register SSH private mib */
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_ssh_json_init();
#endif
        vtss_ssh_icli_cmd_register();
        break;

    case INIT_CMD_START:
        break;

    default:
        break;
    }

    // Call OS dependend init function
    ssh_os_init(data);

    return VTSS_RC_OK;
}
