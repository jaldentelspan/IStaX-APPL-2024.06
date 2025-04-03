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

#ifndef _FRER_API_H_
#define _FRER_API_H_

#include <vtss/appl/frer.hxx>

const char *frer_util_mode_to_str(vtss_appl_frer_mode_t mode, bool capital = false);
const char *frer_util_rcvy_alg_to_str(mesa_frer_recovery_alg_t alg, bool capital = false);
const char *frer_util_oper_state_to_str(vtss_appl_frer_oper_state_t oper_state);
const char *frer_util_oper_state_to_str(vtss_appl_frer_oper_state_t oper_state, vtss_appl_frer_oper_warnings_t oper_warnings);
const char *frer_util_yes_no_str(bool value);
const char *frer_util_ena_dis_str(bool value);
const char *frer_util_oper_warning_to_str( vtss_appl_frer_oper_warnings_t warning);
char       *frer_util_oper_warnings_to_str(vtss_appl_frer_oper_warnings_t warnings, char *buf);

typedef i32 (*frer_icli_pr_t)(u32 session_id, const char *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));

/**
 * Function for converting a FRER error code to a text string
 */
const char *frer_error_txt(mesa_rc rc);

/**
 * Function for initializing the FRER module
 */
mesa_rc frer_init(vtss_init_data_t *data);

#endif /* _FRER_API_H_ */

