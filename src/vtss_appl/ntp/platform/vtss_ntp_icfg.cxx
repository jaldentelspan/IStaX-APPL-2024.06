/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "vtss_ntp_icfg.h"
#include "vtss_ntp.h"
#include "icli_porting_util.h"
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
static mesa_rc VTSS_NTP_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                         vtss_icfg_query_result_t *result)
{
    mesa_rc                             rc = VTSS_RC_OK;
    ntp_conf_t                          conf;
    ntp_conf_t                          def_conf;
    int                                 i;
#ifdef VTSS_SW_OPTION_IPV6
    char                                ip_buf0[80];
#endif

    if ((rc = ntp_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        return rc;
    }

    vtss_ntp_default_set(&def_conf);

    /* global mode */
    // example: ntp
    if (req->all_defaults || conf.mode_enabled != def_conf.mode_enabled) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              conf.mode_enabled == NTP_MGMT_ENABLED ? "" : VTSS_NTP_NO_FORM_TEXT,
                              VTSS_NTP_GLOBAL_MODE_ENABLE_TEXT);
    }

    /* entries */
    // example: ntp server <1-5> ip-address {<ipv4_ucast>|<ipv6_ucast>|<word>}
    for (i = 0; i < VTSS_APPL_NTP_SERVER_MAX_COUNT; i++) {

        if (strlen(conf.server[i].ip_host_string) > 0
#ifdef VTSS_SW_OPTION_IPV6
            || conf.server[i].ipv6_addr.addr[0] > 0) {
            rc = vtss_icfg_printf(result, "%s %d %s %s\n",
                                  VTSS_NTP_GLOBAL_MODE_SERVER_TEXT,
                                  i + 1,
                                  VTSS_NTP_GLOBAL_MODE_IP_TEXT,
                                  conf.server[i].ip_type == NTP_IP_TYPE_IPV4 ? (char *)conf.server[i].ip_host_string : misc_ipv6_txt(&conf.server[i].ipv6_addr, ip_buf0));
#else
           ) {
            rc = vtss_icfg_printf(result, "%s %d %s %s\n",
                                  VTSS_NTP_GLOBAL_MODE_SERVER_TEXT,
                                  i + 1,
                                  VTSS_NTP_GLOBAL_MODE_IP_TEXT,
                                  conf.server[i].ip_host_string);
#endif
        } else {
            if (req->all_defaults) {
                rc = vtss_icfg_printf(result, "%s%s %d\n",
                                      VTSS_NTP_NO_FORM_TEXT,
                                      VTSS_NTP_GLOBAL_MODE_SERVER_TEXT,
                                      i + 1);
            }
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
mesa_rc vtss_ntp_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_VTSS_NTP_GLOBAL_CONF, "ntp", VTSS_NTP_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return rc;
}
