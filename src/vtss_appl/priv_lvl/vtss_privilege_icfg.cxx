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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "vtss_privilege_api.h"
#include "vtss_privilege_icfg.h"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

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
static BOOL diffConf(vtss_appl_privilege_config_web_t *conf_ptr, vtss_appl_privilege_config_web_t *defconf_ptr)
{
    return (conf_ptr->configRoPriv != defconf_ptr->configRoPriv ||
            conf_ptr->configRwPriv != defconf_ptr->configRwPriv ||
            conf_ptr->statusRoPriv != defconf_ptr->statusRoPriv ||
            conf_ptr->statusRwPriv != defconf_ptr->statusRwPriv);
}

/* ICFG callback functions */
static mesa_rc PRIV_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    vtss_priv_conf_t            conf, def_conf;
    vtss_appl_privilege_config_web_t     *conf_ptr = &conf.privilege_level[0], *defconf_ptr = &def_conf.privilege_level[0]; /**< Privilege level */
    vtss_module_id_t            idx = 0;
    char                        name[VTSS_PRIV_LVL_NAME_LEN_MAX] = "";
    mesa_rc                     rc = VTSS_RC_OK;

    if ((rc = vtss_priv_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        return rc;
    }
    VTSS_PRIVILEGE_default_get(&def_conf);

    /* command: snmp-server
                web privilege group <group_name> level { [configRoPriv <configRoPriv:0-15>]  [configRwPriv <configRwPriv:0-15>] [statusRoPriv <statusRoPriv:0-15>] [statusRwPriv <statusRwPriv:0-15>] }*1
       */
    while (TRUE == vtss_privilege_group_name_get(name, &idx, TRUE)) {
        if (req->all_defaults || diffConf(&conf_ptr[idx], &defconf_ptr[idx])) {
            rc += vtss_icfg_printf(result, "web privilege group %s level configRoPriv %d configRwPriv %d statusRoPriv %d statusRwPriv %d\n", name,
                                   conf_ptr[idx].configRoPriv, conf_ptr[idx].configRwPriv, conf_ptr[idx].statusRoPriv, conf_ptr[idx].statusRwPriv);
        }
    }

    return rc;
}


/*
******************************************************************************

    Public functions

******************************************************************************
*/
/* Initialization function */
mesa_rc priv_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_WEB_PRIV_LVL_GLOBAL_CONF, "web-privilege-group-level", PRIV_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return rc;
}
