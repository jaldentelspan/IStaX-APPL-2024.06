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

#ifndef _DHCP_SNOOPING_API_H_
#define _DHCP_SNOOPING_API_H_


#include "dhcp_helper_api.h"
#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */

/* for public APIs */
#include "vtss/appl/dhcp_snooping.h"

/**
 * \file dhcp_snooping_api.h
 * \brief This file defines the APIs for the DHCP Snooping module
 */


/**
 * DHCP Snooping management enabled/disabled
 */
#define DHCP_SNOOPING_MGMT_ENABLED         (1)       /**< Enable option  */
#define DHCP_SNOOPING_MGMT_DISABLED        (0)       /**< Disable option */

/**
 * DHCP snooping port mode
 */
#define DHCP_SNOOPING_PORT_MODE_TRUSTED     DHCP_HELPER_PORT_MODE_TRUSTED   /* trust port mode */
#define DHCP_SNOOPING_PORT_MODE_UNTRUSTED   DHCP_HELPER_PORT_MODE_UNTRUSTED   /* untrust port mode */

/**
 * \brief DHCP Snooping frame information maximum entry count
 */
#define DHCP_SNOOPING_FRAME_INFO_MAX_CNT          1024

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_DHCP_SNOOPING), /**< Operation is only allowed on the primary switch. */
    DHCP_SNOOPING_ERROR_ISID,                                                                      /**< isid parameter is invalid.                       */
    DHCP_SNOOPING_ERROR_ISID_NON_EXISTING,                                                         /**< isid parameter is non-existing.                  */
    DHCP_SNOOPING_ERROR_INV_PARAM,                                                                 /**< Invalid parameter.                               */
};

/**
 * Default configuration
 */
#define DHCP_SNOOPING_DEFAULT_PORT_MODE     DHCP_HELPER_PORT_MODE_TRUSTED   /**< Default port mode */

/**
 * \brief DHCP Snooping configuration
 */
typedef struct {
    u32 snooping_mode;  /* DHCP snooping mode */
} dhcp_snooping_conf_t;

/**
 * \brief DHCP Snooping port configuration
 */
typedef struct {
    CapArray<u8, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_mode;  /* DHCP helper port mode */
    CapArray<u8, MEBA_CAP_BOARD_PORT_MAP_COUNT> veri_mode;  /* DHCP snooping verification mode */
} dhcp_snooping_port_conf_t;

inline int vtss_memcmp(const dhcp_snooping_port_conf_t &a,
                       const dhcp_snooping_port_conf_t &b)
{
    VTSS_MEMCMP_ELEMENT_CAP(a, b, port_mode);
    VTSS_MEMCMP_ELEMENT_CAP(a, b, veri_mode);
    return 0;
}


/**
 * DHCP snooping port statistics
 */
typedef dhcp_helper_stats_t dhcp_snooping_stats_t;

/**
 * DHCP snooping frame information
 * Notes that only two keys(mac and vid) is used for this data structure
 */
typedef dhcp_helper_frame_info_t dhcp_snooping_ip_assigned_info_t;

/**
 * Callback function for IP assigned information
 */
typedef enum {
    DHCP_SNOOPING_INFO_REASON_ASSIGN_COMPLETED,
    DHCP_SNOOPING_INFO_REASON_RELEASE,
    DHCP_SNOOPING_INFO_REASON_LEASE_TIMEOUT,
    DHCP_SNOOPING_INFO_REASON_MODE_DISABLED,
    DHCP_SNOOPING_INFO_REASON_PORT_LINK_DOWN,
    DHCP_SNOOPING_INFO_REASON_ENTRY_DUPLEXED
} dhcp_snooping_info_reason_t;

/**
 * DHCP snooping IP assigned information callback
 */
typedef void (*dhcp_snooping_ip_assigned_info_callback_t)(dhcp_snooping_ip_assigned_info_t *info, dhcp_snooping_info_reason_t reason);

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the DHCP Snooping API functions.
  *
  * \param rc [IN]: Error code that must be in the DHCP_SNOOPING_ERROR_xxx range.
  */
const char *dhcp_snooping_error_txt(mesa_rc rc);

/**
  * \brief Get the global DHCP snooping configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    DHCP_SNOOPING_ERROR_INV_PARAM if glbl_cfg is NULL.\n
  *    DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc dhcp_snooping_mgmt_conf_get(dhcp_snooping_conf_t *glbl_cfg);

/**
  * \brief Set the global DHCP snooping configuration.
  *
  * \param glbl_cfg [IN]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       voice VLAN module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    DHCP_SNOOPING_ERROR_INV_PARAM if glbl_cfg is NULL or parameters error.\n
  *    DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc dhcp_snooping_mgmt_conf_set(dhcp_snooping_conf_t *glbl_cfg);

/**
  * \brief Get a switch's per-port configuration.
  *
  * \param isid        [IN]: The Switch ID for which to retrieve the
  *                          configuration.
  * \param switch_cfg [OUT]: Pointer to structure that receives
  *                          the switch's per-port configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    DHCP_SNOOPING_ERROR_INV_PARAM if switch_cfg is NULL.\n
  *    DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    DHCP_SNOOPING_ERROR_ISID if called with an invalid ISID.\n
  */
mesa_rc dhcp_snooping_mgmt_port_conf_get(vtss_isid_t isid, dhcp_snooping_port_conf_t *switch_cfg);

/**
  * \brief Set a switch's per-port configuration.
  *
  * \param isid       [IN]: The switch ID for which to set the configuration.
  * \param switch_cfg [IN]: Pointer to structure that contains
  *                         the switch's per-port configuration to be applied.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    DHCP_SNOOPING_ERROR_INV_PARAM if switch_cfg is NULL or parameters error.\n
  *    DHCP_SNOOPING_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    DHCP_SNOOPING_ERROR_ISID if called with an invalid ISID.\n
  */
mesa_rc dhcp_snooping_mgmt_port_conf_set(vtss_isid_t isid, dhcp_snooping_port_conf_t *switch_cfg);

/**
  * \brief Initialize the DHCP Snooping module
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc dhcp_snooping_init(vtss_init_data_t *data);

/**
  * \brief Get DHCP snooping statistics
  *
  * \return
  *   0 on success.\n
  *   -1 if fail.\n
  */
int dhcp_snooping_stats_get(vtss_isid_t isid, mesa_port_no_t port_no, dhcp_snooping_stats_t *stats);

/**
  * \brief Clear DHCP snooping statistics
  *
  * \return
  *   0 on success.\n
  *   -1 if fail.\n
  */
int dhcp_snooping_stats_clear(vtss_isid_t isid, mesa_port_no_t port_no);

/**
  * \brief Getnext DHCP snooping IP assigned information entry
  *
  * \return
  *   TRUE on success.\n
  *   FALSE if get fail.\n
  */
BOOL dhcp_snooping_ip_assigned_info_getnext(u8 *mac, mesa_vid_t vid, dhcp_snooping_ip_assigned_info_t *info);

/**
  * \brief Register IP assigned information
  *
  * \return
  *   Nothing.
  */
void dhcp_snooping_ip_assigned_info_register(dhcp_snooping_ip_assigned_info_callback_t cb);

/**
  * \brief Unregister IP assigned information
  *
  * \return
  *   Nothing.
  */
void dhcp_snooping_ip_assigned_info_unregister(dhcp_snooping_ip_assigned_info_callback_t cb);

/**
  * \brief Get the global DHCP snooping default configuration.
  *
  * \param glbl_cfg [IN_OUT]: Pointer to structure that contains the
  *                           configuration to get the default setting.
  *
  * \return
  *   Nothing.
  */
void dhcp_snooping_mgmt_conf_get_default(dhcp_snooping_conf_t *conf);

/**
  * \brief Determine if DHCP snooping configuration has changed.
  *
  * \param old [IN]: Pointer to structure that contains the
  *                  old configuration.
  * \param new_conf [IN]: Pointer to structure that contains the
  *                       new configuration.
  *
  * \return
  *   0: No change.\n
  *   none zero: Configuration changed.\n
  */
int dhcp_snooping_mgmt_conf_changed(dhcp_snooping_conf_t *old, dhcp_snooping_conf_t *new_conf);

/**
  * \brief Get the global DHCP snooping port default configuration.
  *
  * \param glbl_cfg [IN_OUT]: Pointer to structure that contains the
  *                           configuration to get the default setting.
  *
  * \return
  *   Nothing.
  */
void dhcp_snooping_mgmt_port_get_default(vtss_isid_t isid, dhcp_snooping_port_conf_t *conf);

/**
  * \brief Determine if DHCP snooping port configuration has changed.
  *
  * \param old [IN]: Pointer to structure that contains the
  *                  old configuration.
  * \param new_conf [IN]: Pointer to structure that contains the
  *                       new configuration.
  *
  * \return
  *   0: No change.\n
  *   none zero: Configuration changed.\n
  */
int dhcp_snooping_mgmt_port_conf_changed(dhcp_snooping_port_conf_t *old, dhcp_snooping_port_conf_t *new_conf);

#endif /* _DHCP_SNOOPING_API_H_ */

