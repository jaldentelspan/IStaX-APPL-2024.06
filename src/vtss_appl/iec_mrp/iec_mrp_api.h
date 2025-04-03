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

#ifndef _IEC_MRP_API_H_
#define _IEC_MRP_API_H_

#include "main.h"
#include "main_types.h"
#include <vtss/appl/iec_mrp.h>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_INC() */

// To be able to do ring_port++
VTSS_ENUM_INC(vtss_appl_iec_mrp_port_type_t);

// To be able to do pdu_type++
VTSS_ENUM_INC(vtss_appl_iec_mrp_pdu_type_t);

const char *iec_mrp_util_port_type_to_str(vtss_appl_iec_mrp_port_type_t port_type, bool capital = false, bool short_form = false);
const char *iec_mrp_util_port_type_to_short_str(vtss_appl_iec_mrp_port_type_t port_type);
const char *iec_mrp_util_sf_trigger_to_str(vtss_appl_iec_mrp_sf_trigger_t sf_trigger);
const char *iec_mrp_util_role_to_str(vtss_appl_iec_mrp_role_t role, bool capital = false);
const char *iec_mrp_util_oui_type_to_str(vtss_appl_iec_mrp_oui_type_t oui_type);
char       *iec_mrp_util_domain_id_to_uuid(char *buf, size_t size, const uint8_t *domain_id); // buf must be at least 37 chars, domain_id must be 16 uint8.
mesa_rc     iec_mrp_util_domain_id_from_uuid(uint8_t *domain_id, const char *uuid);
const char *iec_mrp_util_recovery_profile_to_str(vtss_appl_iec_mrp_recovery_profile_t recovery_profile);
const char *iec_mrp_util_in_role_to_str(vtss_appl_iec_mrp_in_role_t in_role, bool capital = false);
const char *iec_mrp_util_in_mode_to_str(vtss_appl_iec_mrp_in_mode_t in_mode, bool short_form = false);
const char *iec_mrp_util_in_role_and_mode_to_str(vtss_appl_iec_mrp_in_role_t in_role, vtss_appl_iec_mrp_in_mode_t in_mode);
const char *iec_mrp_util_oper_state_to_str(vtss_appl_iec_mrp_oper_state_t oper_state);
const char *iec_mrp_util_oper_state_to_str(vtss_appl_iec_mrp_oper_state_t oper_state, vtss_appl_iec_mrp_oper_warnings_t oper_warnings);
char       *iec_mrp_util_oper_warnings_to_txt( char *buf, size_t size, vtss_appl_iec_mrp_oper_warnings_t oper_warnings); // sizeof(buf) >= 400 bytes
const char *iec_mrp_util_ring_state_to_str(vtss_appl_iec_mrp_ring_state_t ring_state);
const char *iec_mrp_util_pdu_type_to_str(vtss_appl_iec_mrp_pdu_type_t pdu_type, bool inc_mrp = true);
const char *iec_mrp_error_txt(mesa_rc error);
mesa_rc     iec_mrp_init(vtss_init_data_t *data);

#endif /* _IEC_MRP_API_H_ */

