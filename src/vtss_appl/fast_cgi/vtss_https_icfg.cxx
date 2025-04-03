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
#include "vtss_https_api.hxx"
#include "vtss_https_icfg.hxx"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_HTTPS

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

/* ICFG callback functions */
static mesa_rc VTSS_HTTPS_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                           vtss_icfg_query_result_t *result)
{
    mesa_rc         rc = VTSS_RC_ERROR;
    https_conf_t    *conf, *def_conf;
    int             conf_changed = 0;

    conf     = (https_conf_t *) VTSS_MALLOC(sizeof(https_conf_t));
    def_conf = (https_conf_t *) VTSS_MALLOC(sizeof(https_conf_t));

    if (!conf || !def_conf) {
        printf("Invalid conf or def_conf***************\n");
        goto out;
    }

    if ((rc = https_mgmt_conf_get(conf)) != VTSS_RC_OK) {
        printf("https_mgmt_conf_get failed**************\n");
        goto out;
    }

    https_conf_mgmt_get_default(def_conf);
    conf_changed = https_mgmt_conf_changed(def_conf, conf);
    if (req->all_defaults ||
        (conf_changed && conf->mode != def_conf->mode)) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              conf->mode == HTTPS_MGMT_ENABLED ? "" : VTSS_HTTPS_NO_FORM_TEXT,
                              VTSS_HTTPS_GLOBAL_MODE_ENABLE_TEXT);
    }

    if (req->all_defaults ||
        (conf_changed && conf->redirect != def_conf->redirect)) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              conf->redirect == HTTPS_MGMT_ENABLED ? "" : VTSS_HTTPS_NO_FORM_TEXT,
                              VTSS_HTTPS_REDIRECT_MODE_ENABLE_TEXT);
    }

out:
    if (conf) {
        VTSS_FREE(conf);
    }
    if (def_conf) {
        VTSS_FREE(def_conf);
    }

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc vtss_https_icfg_init(void)
{
    /* Register callback functions to ICFG module. */
    return vtss_icfg_query_register(VTSS_HTTPS_ICFG_GLOBAL_CONF, "http", VTSS_HTTPS_ICFG_global_conf);
}
