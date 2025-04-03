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

#ifndef _DHCP_RELAY_API_H_
#define _DHCP_RELAY_API_H_

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */
#include "vtss/appl/dhcp_relay.h"

/**
 * \file dhcp_relay_api.h
 * \brief This file defines the APIs for the DHCP Relay module
 */

/**
 * DHCP relay management enabled/disabled
 */
#define DHCP_RELAY_MGMT_ENABLED         (1)       /**< Enable option  */
#define DHCP_RELAY_MGMT_DISABLED        (0)       /**< Disable option */

/**
 * DHCP relay maximum DHCP server
 */
#define DHCP_RELAY_MGMT_MAX_DHCP_SERVER (1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    DHCP_RELAY_ERROR_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_DHCP_RELAY), /**< Operation is only allowed on the primary switch. */
    DHCP_RELAY_ERROR_INV_PARAM,                                                              /**< Invalid parameter.                               */
};

/**
 * DHCP relay information policy configuration
 */
#define DHCP_RELAY_INFO_POLICY_REPLACE      VTSS_APPL_DHCP_RELAY_INFO_POLICY_REPLACE
#define DHCP_RELAY_INFO_POLICY_KEEP         VTSS_APPL_DHCP_RELAY_INFO_POLICY_KEEP
#define DHCP_RELAY_INFO_POLICY_DROP         VTSS_APPL_DHCP_RELAY_INFO_POLICY_DROP

/**
 * \brief DHCP Relay default configuration values
 */
#define DHCP4R_DEF_MODE                     DHCP_RELAY_MGMT_DISABLED
#define DHCP4R_DEF_SRV_CNT                  0x0
#define DHCP4R_DEF_INFO_MODE                DHCP_RELAY_MGMT_DISABLED
#define DHCP4R_DEF_INFO_POLICY              DHCP_RELAY_INFO_POLICY_KEEP

/**
 * \brief DHCP Relay configuration.
 */
typedef struct {
    u32         relay_mode;                                     /* DHCP Relay Mode */
    u32         relay_server_cnt;                               /* DHCP Relay Mode */
    mesa_ipv4_t relay_server[DHCP_RELAY_MGMT_MAX_DHCP_SERVER];  /* DHCP Relay Server */
    u32         relay_info_mode;                                /* DHCP Relay Information Mode */
    u32         relay_info_policy;                              /* DHCP Relay Information Policy */
} dhcp_relay_conf_t;

/* the body moved to public header */
typedef vtss_appl_dhcp_relay_statistics_t    dhcp_relay_stats_t;

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the DHCP Relay API functions.
  *
  * \param rc [IN]: Error code that must be in the DHCP_RELAY_ERROR_xxx range.
  */
const char *dhcp_relay_error_txt(mesa_rc rc);

/**
  * \brief Get the global DHCP relay configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    DHCP_RELAY_ERROR_INV_PARAM if glbl_cfg is NULL.\n
  *    DHCP_RELAY_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc dhcp_relay_mgmt_conf_get(dhcp_relay_conf_t *glbl_cfg);

/**
  * \brief Set the global DHCP rleay configuration.
  *
  * \param glbl_cfg [IN]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       voice VLAN module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    DHCP_RELAY_ERROR_INV_PARAM if glbl_cfg is NULL or parameters error.\n
  *    DHCP_RELAY_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc dhcp_relay_mgmt_conf_set(dhcp_relay_conf_t *glbl_cfg);

/**
  * \brief Initialize the DHCP Relay module
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc dhcp_relay_init(vtss_init_data_t *data);

/**
  * \brief Notify DHCP relay module when system IP address changed
  *
  * \return
  *   Nothing.
  */
void dhcp_realy_sysip_changed(u32 ip_addr);

/**
  * \brief  Get DHCP relay statistics
  *
  * \return
  *   Nothing.
  */
void dhcp_relay_stats_get(dhcp_relay_stats_t *stats);

/**
  * \brief Clear DHCP relay statistics
  *
  * \return
  *   Nothing.
  */
void dhcp_relay_stats_clear(void);
#ifdef __cplusplus
}
#endif
#endif /* _DHCP_RELAY_API_H_ */

