/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "vtss_snmp_api.h"
#include "snmp_icfg.h"
#include "misc_api.h" /* For uport2iport(), iport2uport() */
#include "vtss_auth_api.h"
#include "topo_api.h"
#ifdef VTSS_SW_OPTION_LLDP
#include "lldp_api.h"
#endif

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
static mesa_rc TRAP_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    mesa_rc    rc = VTSS_RC_OK;
    char       cmd[256];

    /*
       COMMAND = snmp-server trap <word64> [ id <0-127> ] [ <word255> { include | exclude } ]
    */

    vtss_trap_source_t  trap_source;
    vtss_trap_filter_t  *cfg = &trap_source.trap_filter;
    vtss_trap_filter_item_t *item;
    int                 i = 0;

    strcpy(trap_source.source_name, "");

    while (VTSS_RC_OK == trap_mgmt_source_get_next(&trap_source)) {
        for (i = 0; i < VTSS_TRAP_FILTER_MAX; i++) {
            item = cfg->item[i];
            if (item) {
                if (item->index_filter_len <= 1) {
                    sprintf(cmd, "snmp-server trap %s id %i", trap_source.source_name, i);
                } else {
                    sprintf(cmd, "snmp-server trap %s id %i %s %s", trap_source.source_name, i,
                            misc_oid2str(item->index_filter + 1, item->index_filter_len - 1, item->index_mask, item->index_mask_len),
                            item->filter_type == SNMPV3_MGMT_VIEW_INCLUDED ? "include" : "exclude");
                }
                rc = vtss_icfg_printf(result, "%s\n", cmd);
            }
        }
    }

    return rc;
}

static mesa_rc TRAP_ICFG_host_conf(const vtss_icfg_query_request_t *req,
                                   vtss_icfg_query_result_t *result)
{
    mesa_rc    rc = VTSS_RC_OK;
    char       ipStr[255];
    vtss_trap_entry_t trap_entry, def_entry;
    vtss_trap_conf_t  *conf = &trap_entry.trap_conf, *def_conf = &def_entry.trap_conf;

    /*
        SUB-MODE: snmp-server host <word32>
        COMMAND =
        COMMAND = shutdown
        COMMAND = host { <ipv4_ucast> | <ipv6_ucast> | <hostname> } [ <1-65535> ] [ traps | informs ]
        COMMAND = version { v1 encrypted <word96-224> | v2 encrypted <word96-224> | { v3 engineID <word10-32> [ <word32> ] } }
        COMMAND = informs retries <0-255> timeout <0-2147>
    */

    trap_mgmt_conf_default_get(&def_entry);
    strncpy( trap_entry.trap_conf_name, req->instance_id.string, TRAP_MAX_NAME_LEN );
    trap_entry.trap_conf_name[TRAP_MAX_NAME_LEN] = 0;
    if ( VTSS_RC_OK  == trap_mgmt_conf_get(&trap_entry)) {
        if (req->all_defaults ||
            (conf->enable != def_conf->enable)) {
            rc = vtss_icfg_printf(result, " %s%s\n",
                                  conf->enable == TRUE ? "no " : "", "shutdown");
        }

        if (req->all_defaults ||
            (conf->dip.type != def_conf->dip.type) ||
            (conf->trap_port != def_conf->trap_port) ||
            (conf->trap_inform_mode != def_conf->trap_inform_mode)) {
            if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_IPV4) {
                misc_ipv4_txt(conf->dip.address.ipv4, ipStr);
            } else if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
                misc_ipv6_txt(&conf->dip.address.ipv6, ipStr);
            } else if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_DNS) {
                strncpy(ipStr, conf->dip.address.domain_name.name, 254);
                conf->dip.address.domain_name.name[254] = 0;
            } else {
                strcpy(ipStr, "0.0.0.0");
            }
            rc = vtss_icfg_printf( result, " host %s %d %s\n", ipStr,
                                   conf->trap_port, conf->trap_inform_mode ? "informs" : "traps");

        }


        if (req->all_defaults ||
            (conf->trap_version != def_conf->trap_version) ||
            (conf->trap_version != SNMP_MGMT_VERSION_3 && strcmp(conf->trap_communitySecret, def_conf->trap_communitySecret)) ||
            (conf->trap_version == SNMP_MGMT_VERSION_3 && ((conf->trap_engineid_len != def_conf->trap_engineid_len) ||
                                                           memcmp(conf->trap_engineid, def_conf->trap_engineid, conf->trap_engineid_len) || strcmp(conf->trap_security_name, def_conf->trap_security_name)))) {
            if (conf->trap_version != SNMP_MGMT_VERSION_3) {
                rc = vtss_icfg_printf( result, " version %s encrypted %s\n", conf->trap_version == SNMP_MGMT_VERSION_1  ? "v1" : "v2", conf->trap_encryptedSecret);
            } else {
                char buf[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];
                rc = vtss_icfg_printf( result, " version v3 engineID %s %s\n", misc_engineid2str(buf, conf->trap_engineid, conf->trap_engineid_len), conf->trap_security_name);
            }
        }

        if (req->all_defaults ||
            conf->trap_inform_retries != def_conf->trap_inform_retries || conf->trap_inform_timeout != def_conf->trap_inform_timeout ) {
            rc = vtss_icfg_printf( result, " informs retries %u timeout %u\n", conf->trap_inform_retries, conf->trap_inform_timeout);
        }

    } else {
        return VTSS_RC_ERROR;
    }

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc trap_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_TRAP_GLOBAL_CONF, "snmp", TRAP_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_TRAP_HOST_CONF, "snmp", TRAP_ICFG_host_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return rc;
}
