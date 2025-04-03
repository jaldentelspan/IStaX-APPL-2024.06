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

#ifndef _VTSS_IPV6_SOURCE_GUARD_H_
#define _VTSS_IPV6_SOURCE_GUARD_H_

#include "vtss/appl/ipv6_source_guard.h"
#include "vtss/basics/expose/table-status.hxx"

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_ipv6_source_guard_json_init(void);
#endif

typedef struct {
    /** \brief Assigned MAC address of binding entry.*/
    mesa_mac_t                      mac_addr;

    /** \brief QCE ID of binding entry.*/
    mesa_qce_id_t                   qce_id;
} vtss_appl_ipv6_source_guard_entry_qce_data_t;

typedef struct {
    /**
     * \brief Port configuration.
     */
    vtss_appl_ipv6_source_guard_port_config_t conf;

    /**
     * \brief Current number of dynamic entries bound to port.
    */
    uint32_t curr_dynamic_entries;
    
    /** \brief QCE ID of binding entry.*/
    mesa_qce_id_t  link_local_qce_id;
} vtss_appl_ipv6_source_guard_port_config_info_t;

extern vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_ifindex_t>,
    vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_port_config_info_t *>
> ipv6_source_guard_port_configuration;

extern vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_appl_ipv6_source_guard_entry_index_t>,
    vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_entry_qce_data_t *>
> ipv6_source_guard_static_entries;

extern vtss::expose::TableStatus<
    vtss::expose::ParamKey<vtss_appl_ipv6_source_guard_entry_index_t>,
    vtss::expose::ParamVal<vtss_appl_ipv6_source_guard_entry_qce_data_t *>
> ipv6_source_guard_dynamic_entries;

#endif //_VTSS_DHCP6_RELAY_H_

