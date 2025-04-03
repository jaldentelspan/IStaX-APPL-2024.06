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

#ifndef _APS_API_H_
#define _APS_API_H_

#include "main.h"
#include "main_types.h"
#include <vtss/appl/aps.h>

const char *aps_util_mode_to_str(vtss_appl_aps_mode_t mode);
const char *aps_util_command_to_str(vtss_appl_aps_command_t command);
const char *aps_util_sf_trigger_to_str(vtss_appl_aps_sf_trigger_t sf_trigger);
const char *aps_util_oper_state_to_str(vtss_appl_aps_oper_state_t oper_state);
const char *aps_util_oper_state_to_str(vtss_appl_aps_oper_state_t oper_state, vtss_appl_aps_oper_warning_t oper_warning);
const char *aps_util_oper_warning_to_str(vtss_appl_aps_oper_warning_t oper_warning);
const char *aps_util_prot_state_to_str(vtss_appl_aps_prot_state_t oper_state, bool short_format = true);
const char *aps_util_defect_state_to_str(vtss_appl_aps_defect_state_t defect_state);
const char *aps_util_request_to_str(vtss_appl_aps_request_t request);
const char *aps_error_txt(mesa_rc rc);
mesa_rc     aps_init(vtss_init_data_t *data);

#endif /* _APS_API_H_ */

