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

#ifndef _CFM_API_H_
#define _CFM_API_H_

#include <vtss/appl/cfm.hxx>

#define CFM_ETYPE 0x8902u

/**
 * Utility function for converting a Sender ID TLV option enum to a string
 */
const char *cfm_util_sender_id_tlv_option_to_str(vtss_appl_cfm_sender_id_tlv_option_t sender_id_tlv_option, bool use_in_show_cmd);

/**
 * Utility function for converting a vtss_appl_cfm_tlv_option_t enum to a string.
 */
const char *cfm_util_tlv_option_to_str(vtss_appl_cfm_tlv_option_t status_tlv_option, bool use_in_show_cmd);

/**
 * Utility function for converting an MD format to a string
 */
const char *cfm_util_md_format_to_str(vtss_appl_cfm_md_format_t format, bool capital_first_letter);

/**
 * Utility function for converting an MA format to a string
 */
const char *cfm_util_ma_format_to_str(vtss_appl_cfm_ma_format_t format, bool use_in_show_cmd);

/**
 * Utility function for converting a CCM interval to a string
 */
const char *cfm_util_ccm_interval_to_str(vtss_appl_cfm_ccm_interval_t ccm_interval);

/**
 * Utility function for converting a MEP direction to a string
 */
const char *cfm_util_direction_to_str(vtss_appl_cfm_direction_t direction, bool capital_first_letter);

/**
 * Utility function for converting a Fault Notification State machine state to
 * a string.
 */
const char *cfm_util_fng_state_to_str(vtss_appl_cfm_fng_state_t fng_state);

/**
 * Utility function for converting a MEP defect enum to a string.
 */
const char *cfm_util_mep_defect_to_str(vtss_appl_cfm_mep_defect_t mep_defect);

/**
 * Utility function for converting multiple MEP defects (in a mask) to a string.
 */
const char *cfm_util_mep_defects_to_str(uint8_t defects, char buf[6]);

/**
 * Utility function for converting an RMEP state enum to a string.
 */
const char *cfm_util_rmep_state_to_str(vtss_appl_cfm_rmep_state_t rmep_state);

/**
 * Utility function for converting a "MEP Uncreatable" error to a string.
 */
const char *cfm_util_mep_creatable_error_to_str(const vtss_appl_cfm_mep_errors_t *errors);

/**
 * Utility function for converting an "enableRmepDefect" error to a string.
 */
const char *cfm_util_mep_enableRmepDefect_error_to_str(const vtss_appl_cfm_ma_conf_t *ma_conf, const vtss_appl_cfm_mep_conf_t *mep_conf, vtss_appl_cfm_mep_errors_t *errors);

/**
 * Utility function for converting a port status enum to a string.
 */
const char *cfm_util_port_status_to_str(vtss_appl_cfm_port_status_t port_status);

/**
 * Utility function for converting an interface status enum to a string.
 */
const char *cfm_util_interface_status_to_str(vtss_appl_cfm_interface_status_t interface_status);

/**
 * Utility function for converting an chassis ID enum to a string.
 */
const char *cfm_util_sender_id_chassis_id_subtype_to_str(vtss_appl_cfm_chassis_id_subtype_t chassis_id_subtype);

/**
 * Function for checking a CFM mep key.
 * If empty is non-nullptr, the function always succeeds, but sets empty to
 * true if either md is empty, ma is empty or the mepid is invalid.
 * If empty is nullptr, the contents of md, ma, mepid must follow the
 * requirements, and if so, the function return VTSS_RC_OK, otherwise a proper
 * error code.
 */
mesa_rc cfm_util_key_check(const vtss_appl_cfm_mep_key_t &key, bool *empty = nullptr);

/**
 * Utility function for registering a callback function that will be invoked
 * after any CFM configuration change has occured.
 * The callback function will be invoked without any mutexes held.
 */
mesa_rc cfm_util_conf_change_callback_register(vtss_module_id_t module_id, void (*callback)(void));

/**
 * Function for converting a CFM error code to a text string
 */
const char *cfm_error_txt(mesa_rc rc);

/**
 * Function for initializing the CFM module
 */
mesa_rc cfm_init(vtss_init_data_t *data);

#endif /* _CFM_API_H_ */

