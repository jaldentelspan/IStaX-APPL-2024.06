/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _REDBOX_API_H_
#define _REDBOX_API_H_

#include "main.h"
#include "main_types.h"
#include <vtss/appl/redbox.h>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_INC() */

// To be able to do redbox_port_type++
VTSS_ENUM_INC(vtss_appl_redbox_port_type_t);

// To be able to do sv_type++
VTSS_ENUM_INC(vtss_appl_redbox_sv_type_t);

const char *redbox_util_mode_to_str(      vtss_appl_redbox_mode_t mode,           bool capital = false);
const char *redbox_util_hsr_mode_to_str(  vtss_appl_redbox_hsr_mode_t hsr_mode,   bool capital = false);
const char *redbox_util_port_type_to_str( vtss_appl_redbox_port_type_t port_type, bool capital = false, bool contract = false);
const char *redbox_util_lan_id_to_str(    vtss_appl_redbox_lan_id_t lan_id,       bool capital = false);
const char *redbox_util_node_type_to_str( vtss_appl_redbox_node_type_t node_type);
const char *redbox_util_sv_type_to_str(   vtss_appl_redbox_sv_type_t sv_type, bool contract = false);
const char *redbox_util_oper_state_to_str(vtss_appl_redbox_oper_state_t oper_state);
char       *redbox_util_oper_warnings_to_str(char *buf, size_t size, vtss_appl_redbox_oper_warnings_t oper_warnings);

typedef int32_t (*redbox_icli_pr_t)(const char *fmt, ...) __attribute__((__format__(__printf__, 1, 2)));
mesa_rc redbox_debug_port_state_dump(redbox_icli_pr_t pr);
mesa_rc redbox_debug_state_dump(uint32_t inst, redbox_icli_pr_t pr);

const char *redbox_error_txt(mesa_rc error);
mesa_rc     redbox_init(vtss_init_data_t *data);

#endif /* _REDBOX_API_H_ */

