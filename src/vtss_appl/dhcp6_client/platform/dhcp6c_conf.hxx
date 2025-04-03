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

#ifndef _DHCP6C_CONF_HXX_
#define _DHCP6C_CONF_HXX_

#include "ip_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief DHCP6 DUID type */
typedef enum {
    DHCP6_DUID_LL = 0,                  /**< DUID Based on Link-layer Address */
    DHCP6_DUID_EN,                      /**< DUID Assigned by Vendor Based on Enterprise Number */
    DHCP6_DUID_LLT                      /**< DUID Based on Link-layer Address Plus Time */
} dhcp6_duid_type_t;

/*! \brief DHCP6 Config Operation */
typedef enum {
    DHCP6_CONF_OP_ADD = 1,              /**< Add an entry */
    DHCP6_CONF_OP_DEL,                  /**< Delete an entry */
    DHCP6_CONF_OP_UPD                   /**< Update an entry */
} dhcp6_conf_op_t;

#define DHCP6C_MAX_INTERFACES fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT) /**< Maximum DHCPv6 Interface */

/*! \brief Default DHCPv6 Client Setting */
#define DHCP6C_CONF_DEF_DUID_TYPE       DHCP6_DUID_LL       /**< Default DUID type */
#define DHCP6C_CONF_DEF_SLAAC_STATE     FALSE               /**< Default autoconfiguration state */
#define DHCP6C_CONF_DEF_DHCP6_STATE     FALSE               /**< Default DHCPv6 client state */
#define DHCP6C_CONF_DEF_RAPID_COMMIT    FALSE               /**< Default rapid commit capability */

typedef struct {
    BOOL                                valid;

    BOOL                                slaac;
    BOOL                                dhcp6;

    BOOL                                rapid_commit;

    mesa_vid_t                          index;
    u8                                  reserved[2];
} dhcp6c_conf_intf_t;

typedef struct {
    dhcp6_duid_type_t                   duid_type;
    CapArray<dhcp6c_conf_intf_t, VTSS_APPL_CAP_IP_INTERFACE_CNT> interface;
} dhcp6c_configuration_t;

#ifdef __cplusplus
}
#endif

#ifdef VTSS_SW_OPTION_ICFG

/**
 * \brief Initialization function.
 *
 * Call once, preferably from the INIT_CMD_INIT section of
 * the module's _init() function.
 */
mesa_rc dhcp6_client_icfg_init(void);

#endif /* VTSS_SW_OPTION_ICFG */

#endif /* _DHCP6C_CONF_HXX_ */
