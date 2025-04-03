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

#ifndef _PSEC_LIMIT_API_H_
#define _PSEC_LIMIT_API_H_

#include "psec_api.h"
#include <vtss/appl/psec.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \file psec_limit_api.h
 * \brief This file defines the Limit Control API for the Port Security module
 */

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    PSEC_LIMIT_ERROR_INV_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_PSEC_LIMIT), /**< Invalid user parameter.                                   */
    PSEC_LIMIT_ERROR_MUST_BE_PRIMARY_SWITCH,                                    /**< Operation is only allowed on the primary switch.          */
    PSEC_LIMIT_ERROR_INV_ISID,                                                  /**< isid parameter is invalid.                                */
    PSEC_LIMIT_ERROR_INV_PORT,                                                  /**< Port parameter is invalid.                                */
    PSEC_LIMIT_ERROR_INV_IFINDEX,                                               /**< Ifindex parameter is invalid.                             */
    PSEC_LIMIT_ERROR_IFINDEX_NOT_REPRESENTING_A_PORT,                           /**< Ifindex not representing a port                           */
    PSEC_LIMIT_ERROR_INV_AGING_PERIOD,                                          /**< The supplied aging period is invalid.                     */
    PSEC_LIMIT_ERROR_INV_HOLD_TIME,                                             /**< The supplied hold time is invalid.                        */
    PSEC_LIMIT_ERROR_INV_LIMIT,                                                 /**< The supplied MAC address limit is out of range.           */
    PSEC_LIMIT_ERROR_INV_VIOLATE_LIMIT,                                         /**< The supplied violating MAC address limit is out of range. */
    PSEC_LIMIT_ERROR_INV_VIOLATION_MODE,                                        /**< The supplied violation mode is out of range.              */
    PSEC_LIMIT_ERROR_STATIC_AGGR_ENABLED,                                       /**< Cannot enable if also static aggregated.                  */
    PSEC_LIMIT_ERROR_DYNAMIC_AGGR_ENABLED,                                      /**< Cannot enable if also dynamic aggregated.                 */
    PSEC_LIMIT_ERROR_DYNAMIC_ENTRY_ON_PSEC_DISABLED_PORT,                       /**< Can't add dynamic entry to PSEC disabled port             */
    PSEC_LIMIT_ERROR_DYNAMIC_ENTRY_ON_STICKY_PORT,                              /**< Can't add dynamic entry to sticky port                    */
    PSEC_LIMIT_ERROR_STICKY_ENTRY_ON_NON_STICKY_PORT,                           /**< Can't add sticky entry on non-sticky port                 */
    PSEC_LIMIT_ERROR_DYNAMIC_REPLACE_STATIC,                                    /**< Dymamic entry cannot replace static/sticky entry          */
    PSEC_LIMIT_ERROR_CANT_DELETE_DYNAMIC_ENTRIES,                               /**< Cannot delete dynamic entries this way                    */
    PSEC_LIMIT_ERROR_NO_SUCH_ENTRY,                                             /**< Entry was not found                                       */
    PSEC_LIMIT_ERROR_ENTRY_FOUND_ON_ANOTHER_INTERFACE,                          /**< Entry was found on another interface                      */
    PSEC_LIMIT_ERROR_LIMIT_LOWER_THAN_CUR_CNT,                                  /**< Limit cannot be set to a value lower than the current #   */
    PSEC_LIMIT_ERROR_STATIC_STICKY_MAC_CNT_EXCEEDS_GLOBAL_POOL_SIZE,            /**< Cannot add sticky/static MAC beyond size of global pool   */
    PSEC_LIMIT_ERROR_MAC_POOL_DEPLETED,                                         /**< Cannot add sticky/static MAC beyond size of global pool   */
    PSEC_LIMIT_ERROR_MAC_NOT_UNICAST,                                           /**< MAC not unicast MAC address                               */
}; // Anonymous enum to satisfy Lint.

//
// Other public Port Security Limit Control functions.
//

/**
 * \brief Retrieve an error string based on a return code
 *        from one of the Port Security Limit Control API functions.
 *
 * \param rc [IN]: Error code that must be in the PSEC_LIMIT_ERROR_xxx range.
 */
const char *psec_limit_error_txt(mesa_rc rc);

/**
 * \brief Get all sticky and static entries on a port.
 *
 * Only to be called by PSEC module.
 */
void psec_limit_mgmt_static_macs_get(vtss_ifindex_t ifindex, BOOL force);

/**
 * \brief Get forwarding and blocking count
 *
 * Debug function.
 */
mesa_rc psec_limit_mgmt_debug_ref_cnt_get(vtss_ifindex_t ifindex, u32 *fwd_cnt, u32 *blk_cnt);

/**
 * \brief Get a FID from a VID.
 */
mesa_vid_t psec_limit_mgmt_fid_get(mesa_vid_t vid);

/**
 * \brief Initialize the Port Security Limit Control module
 *
 * \param cmd [IN]: Reason why calling this function.
 * \param p1  [IN]: Parameter 1. Usage varies with cmd.
 * \param p2  [IN]: Parameter 2. Usage varies with cmd.
 *
 * \return
 *    VTSS_RC_OK.
 */
mesa_rc psec_limit_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif
#endif /* _PSEC_LIMIT_API_H_ */

