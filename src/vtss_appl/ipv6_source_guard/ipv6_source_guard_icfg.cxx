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
#include "icli_porting_util.h"
#include "ipv6_source_guard_icfg.h"
#include "ipv6_source_guard.h"
#include "vtss/appl/ipv6_source_guard.h"
#include "topo_api.h"
#include "vtss_common_iterator.hxx"
#include "misc_api.h"   //uport2iport(), iport2uport()

/*
******************************************************************************

    Static Functions

******************************************************************************
*/

/* ICFG callback functions */
static mesa_rc IPV6_SOURCE_GUARD_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_ifindex_elm_t ife;
    vtss_appl_ipv6_source_guard_global_config_t global_conf;
    vtss_appl_ipv6_source_guard_entry_index_t next, *prev = NULL;
    vtss_appl_ipv6_source_guard_entry_data_t e_data;
    char buf0[80], buf1[80], buf2[80], buf3[80];

    if ((rc = vtss_appl_ipv6_source_guard_global_config_get(&global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    /* Global mode. */
    if (req->all_defaults || global_conf.enabled) {
            rc = vtss_icfg_printf(result, "%s%s\n",
            global_conf.enabled ? "" : IPV6_SOURCE_GUARD_NO_FORM_TEXT,
            IPV6_SOURCE_GUARD_GLOBAL_MODE_ENABLE_TEXT);
    }

    /*Static entries.*/
    while (vtss_appl_ipv6_source_guard_static_entry_itr(prev, &next) == VTSS_RC_OK) {
        if ((rc = vtss_appl_ipv6_source_guard_static_entry_data_get(&next, &e_data)) != VTSS_RC_OK) {
            return rc;
        }
        if ((rc = vtss_appl_ifindex_port_configurable(next.ifindex, &ife)) != VTSS_RC_OK) {
            return rc;
        }
        sprintf(buf3,"vlan %d", next.vlan_id);
        rc = vtss_icfg_printf(result, "%s %s %s %s %s %s\n",
        IPV6_SOURCE_GUARD_GLOBAL_MODE_ENTRY_TEXT,
        IPV6_SOURCE_GUARD_INTERFACE_TEXT,
        icli_port_info_txt(ife.usid, iport2uport(ife.ordinal), buf0),
        next.vlan_id == 0 ? "" : buf3,
        misc_ipv6_txt(&next.ipv6_addr, buf1),
        misc_mac_txt(e_data.mac_addr.addr, buf2));
        
        prev = &next;
    }

    return rc;
}

static mesa_rc IPV6_SOURCE_GUARD_ICFG_port_conf(const vtss_icfg_query_request_t *req,
                                              vtss_icfg_query_result_t *result)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_ifindex_t  ifindex;
    vtss_appl_ipv6_source_guard_port_config_t port_conf;

    if ((rc = vtss_ifindex_from_port(req->instance_id.port.isid, req->instance_id.port.begin_iport, &ifindex)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = vtss_appl_ipv6_source_guard_port_config_get(ifindex, &port_conf)) != VTSS_RC_OK) {
        return rc;
    }

    if (req->all_defaults || port_conf.enabled) {
        rc = vtss_icfg_printf(result, " %s%s\n",
            port_conf.enabled ? "" : IPV6_SOURCE_GUARD_NO_FORM_TEXT,
            IPV6_SOURCE_GUARD_PORT_MODE_ENABLE_TEXT);
        
        if (port_conf.max_dynamic_entries == IPV6_SOURCE_GUARD_DYNAMIC_UNLIMITED) {
            rc = vtss_icfg_printf(result, " %s%s\n",
                IPV6_SOURCE_GUARD_NO_FORM_TEXT,
                IPV6_SOURCE_GUARD_PORT_MODE_LIMIT_TEXT);
        } else  {
            rc = vtss_icfg_printf(result, " %s %u\n",
                IPV6_SOURCE_GUARD_PORT_MODE_LIMIT_TEXT,
                port_conf.max_dynamic_entries);
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
mesa_rc ipv6_source_guard_icfg_init(void)
{
    mesa_rc rc;
    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_IPV6_SOURCE_GUARD_GLOBAL_CONF, "ipv6-source-guard", IPV6_SOURCE_GUARD_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    rc = vtss_icfg_query_register(VTSS_ICFG_IPV6_SOURCE_GUARD_PORT_CONF, "ipv6-source-guard", IPV6_SOURCE_GUARD_ICFG_port_conf);

    return rc;
}
    
