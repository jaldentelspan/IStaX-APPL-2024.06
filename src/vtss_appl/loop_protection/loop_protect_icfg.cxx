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

    Revision history
    > CP.Wang, 2013/01/02 11:34
        - create

******************************************************************************
*/

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include <stdlib.h>
#include "icfg_api.h"
#include "loop_protect_api.h"
#include "topo_api.h"
#include "misc_api.h"

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
/* ICFG callback functions */
static mesa_rc _loop_protect_icfg(
    IN const vtss_icfg_query_request_t  *req,
    IN vtss_icfg_query_result_t         *result
)
{
    mesa_rc                               rc;
    vtss_isid_t                           isid;
    mesa_port_no_t                        iport;
    vtss_appl_loop_protect_conf_t         sc;
    vtss_appl_loop_protect_port_conf_t    conf;

    if ( req == NULL ) {
        return VTSS_RC_ERROR;
    }

    if ( result == NULL ) {
        return VTSS_RC_ERROR;
    }

    switch ( req->cmd_mode ) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
        rc = vtss_appl_loop_protect_conf_get( &sc );
        if ( rc != VTSS_RC_OK ) {
            return rc;
        }
    
        if ( sc.enabled != VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_ENABLED ) {
            (void)vtss_icfg_printf(result, "%sloop-protect\n", sc.enabled ? "" : "no ");
        } else if ( req->all_defaults ) {
            (void)vtss_icfg_printf(result, "%sloop-protect\n", sc.enabled ? "" : "no ");
        }

        if ( sc.transmission_time != VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_TX_TIME ) {
            (void)vtss_icfg_printf(result, "loop-protect transmit-time %d\n", sc.transmission_time);
        } else if ( req->all_defaults ) {
            (void)vtss_icfg_printf(result, "loop-protect transmit-time %u\n", VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_TX_TIME);
        }

        if ( sc.shutdown_time != VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_SHUTDOWN_TIME ) {
            (void)vtss_icfg_printf(result, "loop-protect shutdown-time %d\n", sc.shutdown_time);
        } else if ( req->all_defaults ) {
            (void)vtss_icfg_printf(result, "loop-protect shutdown-time %u\n", VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_SHUTDOWN_TIME);
        }
        break;

    case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
        /* get isid and iport */
        isid = topo_usid2isid(req->instance_id.port.usid);
        iport = uport2iport(req->instance_id.port.begin_uport);

        rc = vtss_appl_loop_protect_conf_port_get(isid, iport, &conf);
        if ( rc != VTSS_RC_OK ) {
            return rc;
        }

        if ( conf.enabled != VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_ENABLED ) {
            (void)vtss_icfg_printf(result, " %sloop-protect\n", conf.enabled ? "" : "no ");
        } else if ( req->all_defaults ) {
            (void)vtss_icfg_printf(result, " %sloop-protect\n", conf.enabled ? "" : "no ");
        }

        if ( conf.action != VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_ACTION ) {
            switch ( conf.action ) {
            case VTSS_APPL_LOOP_PROTECT_ACTION_SHUTDOWN:
                (void)vtss_icfg_printf(result, " loop-protect action shutdown\n");
                break;
            case VTSS_APPL_LOOP_PROTECT_ACTION_SHUT_LOG:
                (void)vtss_icfg_printf(result, " loop-protect action shutdown log\n");
                break;
            case VTSS_APPL_LOOP_PROTECT_ACTION_LOG_ONLY:
                (void)vtss_icfg_printf(result, " loop-protect action log\n");
                break;
            default:
                break;
            }
        } else if ( req->all_defaults ) {
            (void)vtss_icfg_printf(result, " no loop-protect action\n");
        }

        if ( conf.transmit != VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_TX_MODE ) {
            (void)vtss_icfg_printf(result, " %sloop-protect tx-mode\n", conf.transmit ? "" : "no ");
        } else if ( req->all_defaults ) {
            (void)vtss_icfg_printf(result, " %sloop-protect tx-mode\n", conf.transmit ? "" : "no ");
        }
        break;

    default:
        /* no config in other modes */
        break;
    }
    return VTSS_RC_OK;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc loop_protect_icfg_init(void)
{
    mesa_rc     rc;

    /*
        Register Global config callback functions to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_GLOBAL_LOOP_PROTECT, "loop-protect", _loop_protect_icfg);
    if ( rc != VTSS_RC_OK ) {
        return rc;
    }

    /*
        Register Interface port list config callback functions to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_INTERFACE_ETHERNET_LOOP_PROTECT, "loop-protect", _loop_protect_icfg);
    return rc;
}
