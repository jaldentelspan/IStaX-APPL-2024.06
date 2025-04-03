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

#ifndef _IPMC_LIB_TRACE_HXX_
#define _IPMC_LIB_TRACE_HXX_

#include "ipmc_lib_base.hxx"
#include <vtss/appl/ipmc_lib.h>

size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_src_list_t                     *src_list);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_pdu_group_record_t             *grp_rec);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_pdu_t                          *pdu);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_src_state_t                    *src_state);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_src_map_t                      *src_map);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_grp_state_t                    *grp_state);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_grp_key_t                      *grp_key);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_grp_itr_t                      *grp_itr);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_vlan_state_t                   *vlan_state);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_global_conf_t        *conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_port_conf_t          *conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_conf_t          *conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_port_conf_t     *conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_status_t        *s);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_profile_range_conf_t *conf);

#endif /* _IPMC_LIB_TRACE_HXX_ */

