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
#include "syslog_api.h"
#include "syslog_icfg.h"
#include "misc_api.h"   // For misc_ipv4_txt(), misc_ipv6_txt()

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
static mesa_rc SYSLOG_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                       vtss_icfg_query_result_t *result)
{
    mesa_rc                 rc = VTSS_RC_OK;
    vtss_appl_syslog_server_conf_t conf, def_conf;

    (void) vtss_appl_syslog_server_conf_get(&conf);
    syslog_mgmt_default_get(&def_conf);

    /* command: logging on
                logging host {<ipv4_ucast> | <word45>}
                logging level [info|warning|error]
       */
    if (req->all_defaults ||
        (conf.server_mode != def_conf.server_mode)) {
        rc = vtss_icfg_printf(result, "%s\n",
                              conf.server_mode == TRUE ? "logging on" : "no logging on");
    }

    if (req->all_defaults ||
        memcmp(&conf.syslog_server, &def_conf.syslog_server, sizeof(conf.syslog_server))) {
        vtss_domain_name_t tmp;
        tmp.name[0] = '\0';
        if (conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_IPV4) {
            (void)misc_ipv4_txt(conf.syslog_server.address.ipv4, tmp.name);
#if defined(VTSS_SW_OPTION_IPV6)
        } else if (conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
            (void)misc_ipv6_txt(&conf.syslog_server.address.ipv6, tmp.name);
#endif /* VTSS_SW_OPTION_IPV6 */
#if defined(VTSS_SW_OPTION_DNS)
        } else if (conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_DNS) {
            strcpy(tmp.name, conf.syslog_server.address.domain_name.name);
#endif /* VTSS_SW_OPTION_DNS */
        }
        rc = vtss_icfg_printf(result, "%s%s%s%s\n",
                              strlen(tmp.name) ? "" : "no ",
                              "logging host",
                              strlen(tmp.name) ? " " : "",
                              strlen(tmp.name) ? tmp.name : "");
    }

    if (req->all_defaults ||
        (conf.syslog_level != def_conf.syslog_level)) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              "logging level ", syslog_lvl_to_string(conf.syslog_level, TRUE));
    }
#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
    vtss_appl_syslog_notif_name_t notif_itr;
    for (mesa_rc rc = vtss_appl_syslog_notif_itr(nullptr, &notif_itr);
         rc == VTSS_RC_OK;
         rc = vtss_appl_syslog_notif_itr(&notif_itr, &notif_itr)) {
        vtss_appl_syslog_notif_conf_t conf;
        vtss_appl_syslog_notif_get(&notif_itr, &conf);
        rc = vtss_icfg_printf(result, "logging notification listen %s level %s %s\n",
                              notif_itr.notif_name,
                              syslog_lvl_to_string(conf.level, TRUE),
                              conf.notification);
    }
#endif
    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc syslog_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_SYSLOG_GLOBAL_CONF, "logging", SYSLOG_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return rc;
}
