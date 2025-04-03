/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _ERPS_API_H_
#define _ERPS_API_H_

#include "main.h"
#include "main_types.h"
#include <vtss/appl/erps.h>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_INC() */

// To be able to do ring_port++
VTSS_ENUM_INC(vtss_appl_erps_ring_port_t);

bool        erps_util_using_ring_port_conf(const vtss_appl_erps_conf_t *conf, vtss_appl_erps_ring_port_t ring_port);
const char *erps_util_command_to_str(vtss_appl_erps_command_t command);
const char *erps_util_version_to_str(vtss_appl_erps_version_t version);
const char *erps_util_ring_type_to_str(vtss_appl_erps_ring_type_t ring_type, bool capital_first_letter = false);
const char *erps_util_sf_trigger_to_str(vtss_appl_erps_sf_trigger_t sf_trigger);
const char *erps_util_rpl_mode_to_str(vtss_appl_erps_rpl_mode_t rpl_mode, bool capital_first_letter = false);
const char *erps_util_ring_port_to_str(vtss_appl_erps_ring_port_t ring_port);
const char *erps_util_oper_state_to_str(vtss_appl_erps_oper_state_t oper_state);
const char *erps_util_oper_state_to_str(vtss_appl_erps_oper_state_t oper_state, vtss_appl_erps_oper_warning_t oper_warning);
const char *erps_util_oper_warning_to_str(vtss_appl_erps_oper_warning_t oper_warning);
const char *erps_util_node_state_to_str(vtss_appl_erps_node_state_t node_state);
const char *erps_util_request_to_str(vtss_appl_erps_request_t request);
const char *erps_util_raps_info_to_str(vtss_appl_erps_raps_info_t &raps_info, char str[26], bool active, bool include_bpr);
const char *erps_error_txt(mesa_rc error);
mesa_rc erps_init(vtss_init_data_t *data);

#endif /* _ERPS_API_H_ */

