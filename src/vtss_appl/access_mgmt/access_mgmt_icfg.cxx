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
    > CP.Wang, 2012/10/18 10:10
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
#include "access_mgmt_api.h"
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
static mesa_rc _access_mgmt_icfg(
    IN const vtss_icfg_query_request_t  *req,
    IN vtss_icfg_query_result_t         *result
)
{
    access_mgmt_conf_t  conf;
    u32                 i, all_service_type = 0;
    char                ip_buf1[64], ip_buf2[64];

    if ( req == NULL ) {
        return VTSS_RC_ERROR;
    }

    if ( result == NULL ) {
        return VTSS_RC_ERROR;
    }

    switch ( req->cmd_mode ) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
        if ( access_mgmt_conf_get(&conf) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }

        // access management
        if ( conf.mode ) {
            (void)vtss_icfg_printf(result, "access management\n");
        } else if ( req->all_defaults ) {
            (void)vtss_icfg_printf(result, "no access management\n");
        }

        // get all service type
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
        all_service_type |= ACCESS_MGMT_SERVICES_TYPE_WEB;
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#if defined(VTSS_SW_OPTION_SNMP)
        all_service_type |= ACCESS_MGMT_SERVICES_TYPE_SNMP;
#endif /* VTSS_SW_OPTION_SNMP */
#if defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_SW_OPTION_SSH)
        all_service_type |= ACCESS_MGMT_SERVICES_TYPE_TELNET;
#endif /* VTSS_SW_OPTION_CLI_TELNET || VTSS_SW_OPTION_SSH */

        // access management <access-id:1-16> <ipv4_addr> {[ to <ipv4_addr> ]} [web] [snmp] [telnet]
        // access management <access-id:1-16> <ipv6_addr> {[ to <ipv6_addr> ]} [web] [snmp] [telnet]
        for (i = ACCESS_MGMT_ACCESS_ID_START; i < ACCESS_MGMT_MAX_ENTRIES + ACCESS_MGMT_ACCESS_ID_START; i++) {
            if ( conf.entry[i].valid ) {
                (void)vtss_icfg_printf(result, "access management %u ", i);
                (void)vtss_icfg_printf(result, "%u ", conf.entry[i].vid);
                if ( conf.entry[i].entry_type == ACCESS_MGMT_ENTRY_TYPE_IPV4 ) {
                    (void)vtss_icfg_printf(result, "%s%s%s",
                                           misc_ipv4_txt(conf.entry[i].start_ip, ip_buf1),
                                           conf.entry[i].start_ip != conf.entry[i].end_ip ? " to " : "",
                                           conf.entry[i].start_ip != conf.entry[i].end_ip ? misc_ipv4_txt(conf.entry[i].end_ip, ip_buf2) : "");
                }
#if defined(VTSS_SW_OPTION_IPV6)
                else {
                    (void)vtss_icfg_printf(result, "%s%s%s",
                                           misc_ipv6_txt(&conf.entry[i].start_ipv6, ip_buf1),
                                           memcmp(&conf.entry[i].start_ipv6,  &conf.entry[i].end_ipv6, sizeof(mesa_ipv6_t)) ? " to " : "",
                                           memcmp(&conf.entry[i].start_ipv6,  &conf.entry[i].end_ipv6, sizeof(mesa_ipv6_t)) ? misc_ipv6_txt(&conf.entry[i].end_ipv6, ip_buf2) : "");
                }
#endif /* VTSS_SW_OPTION_IPV6 */

                if ( all_service_type != 0 && conf.entry[i].service_type == all_service_type ) {
                    (void)vtss_icfg_printf(result, " all");
                } else {
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
                    if ( conf.entry[i].service_type & ACCESS_MGMT_SERVICES_TYPE_WEB ) {
                        (void)vtss_icfg_printf(result, " web");
                    }
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */
#if defined(VTSS_SW_OPTION_SNMP)
                    if ( conf.entry[i].service_type & ACCESS_MGMT_SERVICES_TYPE_SNMP ) {
                        (void)vtss_icfg_printf(result, " snmp");
                    }
#endif /* VTSS_SW_OPTION_SNMP */
#if defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_SW_OPTION_SSH)
                    if ( conf.entry[i].service_type & ACCESS_MGMT_SERVICES_TYPE_TELNET ) {
                        (void)vtss_icfg_printf(result, " telnet");
                    }
#endif /* VTSS_SW_OPTION_CLI_TELNET || VTSS_SW_OPTION_SSH */
                }
                (void)vtss_icfg_printf(result, "\n");
            }
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
mesa_rc access_mgmt_icfg_init(void)
{
    mesa_rc rc;

    /*
        Register Global config callback functions to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_GLOBAL_ACCESS_MGMT, "access", _access_mgmt_icfg);
    return rc;
}
