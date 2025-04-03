/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_PRIVILEGE_API_H_
#define _VTSS_PRIVILEGE_API_H_

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */
#include "vtss_module_id.h"
#include "vtss/appl/privilege.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \file vtss_privilege_api.h
 * \brief This file defines the APIs for the Privilege Level module
 */

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    VTSS_PRIV_ERROR_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_PRIV_LVL), /**< Operation is only allowed on the primary switch. */
    VTSS_PRIV_ERROR_ISID,                                                                 /**< isid parameter is invalid.                       */
    VTSS_PRIV_ERROR_INV_PARAM                                                             /**< Invalid parameter.                               */
};

/**
 * \brief Defines the minimum and maximum privilege level.
 */
#define VTSS_PRIV_LVL_NAME_LEN_MAX      (VTSS_APPL_PRIVILEGE_NAME_MAX_LEN + 1)

/**
 * \brief privilege level type.
 */
typedef enum {
    VTSS_PRIV_LVL_CONFIG_TYPE,    /**< Privilege level configuration type */
    VTSS_PRIV_LVL_STAT_TYPE       /**< Privilege level status/statistics type */
} vtss_priv_lvl_type_t;

/**
 * \brief Privilege Level configuration.
*/
typedef struct {
    vtss_appl_privilege_config_web_t privilege_level[VTSS_MODULE_ID_NONE]; /**< Privilege level */
} vtss_priv_conf_t;

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the Privilege Level API functions.
  *
  * \param rc [IN]: Error code that must be in the VTSS_PRIV_ERROR_xxx range.
  */
const char *vtss_privilege_error_txt(mesa_rc rc);

/**
  * \brief Get the default Privilege Level configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  *
  */
void VTSS_PRIVILEGE_default_get(vtss_priv_conf_t *conf);

/**
  * \brief Get the global Privilege Level configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VTSS_PRIV_ERROR_INV_PARAM if glbl_cfg is NULL.\n
  *    VTSS_PRIV_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc vtss_priv_mgmt_conf_get(vtss_priv_conf_t *glbl_cfg);

/**
  * \brief Set the global Privilege Level configuration.
  *
  * \param glbl_cfg [IN]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       voice VLAN module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VTSS_PRIV_ERROR_INV_PARAM if glbl_cfg is NULL or parameters error.\n
  *    VTSS_PRIV_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    Others value arises from sub-function.\n
  */
mesa_rc vtss_priv_mgmt_conf_set(vtss_priv_conf_t *glbl_cfg);

/**
  * \brief Verify privilege level is allowed for 'Configuration Read-only'
  *
  * Used by CLI and Web module in order to get the configured privilege
  * levels for a specific privilege level group.
  *
  * \param id               [IN]: The module ID.
  * \param current_level    [IN]: The current privilege level.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_priv_is_allowed_cro(vtss_module_id_t id, int current_level);

/**
  * \brief Verify privilege level is allowed for 'Configuration Read-write'
  *
  * Used by CLI and Web module in order to get the configured privilege
  * levels for a specific privilege level group.
  *
  * \param id               [IN]: The module ID.
  * \param current_level    [IN]: The current privilege level.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_priv_is_allowed_crw(vtss_module_id_t id, int current_level);

/**
  * \brief Verify privilege level is allowed for ''Status/Statistics Read-only'
  *
  * Used by CLI and Web module in order to get the configured privilege
  * levels for a specific privilege level group.
  *
  * \param id               [IN]: The module ID.
  * \param current_level    [IN]: The current privilege level.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_priv_is_allowed_sro(vtss_module_id_t id, int current_level);

/**
  * \brief Verify privilege level is allowed for 'Status/Statistics Read-write'
  *
  * Used by CLI and Web module in order to get the configured privilege
  * levels for a specific privilege level group.
  *
  * \param id               [IN]: The module ID.
  * \param current_level    [IN]: The current privilege level.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_priv_is_allowed_srw(vtss_module_id_t id, int current_level);

/**
  * \brief Initialize the Privilege Level module
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc vtss_priv_init(vtss_init_data_t *data);

/**
  * \brief Get module ID by privilege level group name
  *
  * \param name         [IN]: The privilege level group name.
  * \param module_id_p  [OUT]: The module ID.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_privilege_module_to_val(const char *name, vtss_module_id_t *module_id_p);

/**
  * \brief Get privilege group name
  *
  * \param name         [OUT]: The privilege level group name.
  * \param module_id_p  [OUT]: The module ID.
  * \param next         [IN]: is getnext operation.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
BOOL vtss_privilege_group_name_get(char *name, vtss_module_id_t *module_id_p, BOOL next);

/**
  * \brief Get privilege group name list
  *
  * \param max_cnt  [IN]: The maximum count of privilege group name.
  * \param list_p   [OUT]: The point list of  privilege group name.
  *
  * \return
  *    TRUE if the module name is found.\n
  *    FALSE otherwise.\n
  */
void vtss_privilege_group_name_list_get(const u32 max_cnt, const char *list_p[]);


#ifdef __cplusplus
}
#endif
#endif /* _VTSS_PRIVILEGE_API_H_ */

