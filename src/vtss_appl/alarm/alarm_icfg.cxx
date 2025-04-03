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
#include "topo_api.h"
#include "alarm_api.hxx"
#include "alarm_icfg.h"
#include "vtss/appl/alarm.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ALARM

//******************************************************************************
// ICFG callback functions
//******************************************************************************

static mesa_rc alarm_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                      vtss_icfg_query_result_t *result) {
    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    vtss_appl_alarm_name_t itr;
    for (mesa_rc rc = vtss_appl_alarm_conf_itr(nullptr, &itr); rc == VTSS_RC_OK;
         rc = vtss_appl_alarm_conf_itr(&itr, &itr)) {
        vtss_appl_alarm_expression_t conf;
        VTSS_RC(vtss_appl_alarm_conf_get(&itr, &conf));
        VTSS_RC(vtss_icfg_printf(result, "alarm %s %s\n", itr.alarm_name,
                                 conf.alarm_expression));
    }
    return VTSS_RC_OK;
}

//******************************************************************************
//   Public functions
//******************************************************************************

mesa_rc alarm_icfg_init(void) {
    mesa_rc rc;

    /* Register callback functions to ICFG module. */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_ALARM_GLOBAL_CONF, "alarm",
                                       alarm_icfg_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}
