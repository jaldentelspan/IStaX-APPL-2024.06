/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/**
 * \file
 * \brief Public Privilege API
 * \details This header file describes Privilege control functions and types.
 */

#ifndef _VTSS_APPL_PRIVILEGE_H_
#define _VTSS_APPL_PRIVILEGE_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The minimum privilege level */
#define VTSS_APPL_PRIVILEGE_LVL_MIN 0

/** The maximum privilege level */
#define VTSS_APPL_PRIVILEGE_LVL_MAX 15

/** The maximum length of module name */
#define VTSS_APPL_PRIVILEGE_NAME_MAX_LEN 31

/**
 * \brief Module name
 *  The struct is used to represent the module name stored as a c-string.
 */
typedef struct {
    char name[VTSS_APPL_PRIVILEGE_NAME_MAX_LEN + 1]; /*!< module name */
} vtss_appl_privilege_module_name_t;

/**
 * \brief Configuration of web privilege level.
 */
typedef struct {
    uint32_t configRoPriv; /*!< privilege of Web read-only configuration page  */
    uint32_t configRwPriv; /*!< privilege of Web read-write configuration page */
    uint32_t statusRoPriv; /*!< privilege of Web read-only status page         */
    uint32_t statusRwPriv; /*!< privilege of Web read-clear status page        */
} vtss_appl_privilege_config_web_t;

/**
 * \brief Iterate function of Web Privilege Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_moduleName [IN]  previous module name.
 * \param next_moduleName [OUT] next module name.
 *
 * \return VTSS_RC_ERROR when no more indexes exists - otherwise VTSS_RC_OK
 */
mesa_rc vtss_appl_privilege_config_web_itr(
        const vtss_appl_privilege_module_name_t *const prev_moduleName,
        vtss_appl_privilege_module_name_t *const next_moduleName);

/**
 * \brief Get Web Privilege Configuration
 *
 * To read configuration of web privilege.
 *
 * \param moduleName [IN]  (key) Module name.
 * \param conf       [OUT] The configuration of web privilege
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_privilege_config_web_get(
        vtss_appl_privilege_module_name_t moduleName,
        vtss_appl_privilege_config_web_t *const conf);

/**
 * \brief Set Web Privilege Configuration
 *
 * To modify configuration of web privilege.
 *
 * \param moduleName [IN] (key) Module name.
 * \param conf       [IN] The configuration of web privilege
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_privilege_config_web_set(
        vtss_appl_privilege_module_name_t moduleName,
        const vtss_appl_privilege_config_web_t *const conf);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_PRIVILEGE_H_ */
