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

/**
 * \file
 * \brief Public UPNP API
 * \details UPnP is an acronym for Universal Plug and Play. The goals of UPnP
 *  are to allow devices to connect seamlessly and to simplify the
 *  implementation of networks in the home (data sharing, communications,
 *  and entertainment) and in corporate environments for simplified
 *  installation of computer components. When UPnP is enabled, the switch will
 *  send SSDP advertisement packets periodically to tell the UPnP browser the device property such as
 *  IP address, device name, and others information. Administrator will know how to configure it
 *  according to the information.
 *  This header file describes UPNP control functions and types.
 */

#ifndef _VTSS_APPL_UPNP_H_
#define _VTSS_APPL_UPNP_H_

#include <vtss/appl/module_id.h>
#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/*! \brief UPnP IP addressing mode type */
typedef enum {
    VTSS_APPL_UPNP_IPADDRESSING_MODE_DYNAMIC = 0,   /**< UPnP will find the IP address automatically */
    VTSS_APPL_UPNP_IPADDRESSING_MODE_STATIC,        /**< UPnP will use the IP address of the interface user configure */
} vtss_appl_upnp_ip_addressing_mode_t;

/** \brief Collection of capability properties of the UPnP module. */
typedef struct {
    /** TTl : The capability to support the authority of writing. */
    mesa_bool_t    support_ttl_write;
} vtss_appl_upnp_capabilities_t;

/**
 * \brief UPnP global configuration
 *  The configuration is the system configuration that can enable/disable
 *  the HTTP Secure function, configure the TTL and Advertising Duration.
 */
typedef struct {
    /**
     * \brief Global config mode, TRUE is to enable UPnP function
     * in the system and FALSE is to disable it.
     */
    mesa_bool_t    mode;

    /**
     * \brief TTL value, TTL value in the IP header.
     */
    uint8_t      ttl;

    /**
     * \brief SSDP advertisement mesaage interval.
     */
    uint32_t     adv_interval;

    /**
     * \brief UPnP IP addressing mode.
     */
    vtss_appl_upnp_ip_addressing_mode_t  ip_addressing_mode;

    /**
     * \brief The ID of Static VLAN interface IP will use to address.
     */
    vtss_ifindex_t static_ifindex;
} vtss_appl_upnp_param_t;

/**
 * \brief Get UPnP system parameters
 *
 * To read current system parameters in UPnP.
 *
 * \param param [OUT] The HTTPS system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_upnp_system_config_get(
    vtss_appl_upnp_param_t          *const param
);

/**
 * \brief Set UPnP system parameters
 *
 * To modify current system parameters in UPnP.
 *
 * \param param [IN] The HTTPS system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_upnp_system_config_set(
    const vtss_appl_upnp_param_t     *const param
);

/**
 * \brief Get the capabilities of UPnP.
 *
 * \param cap       [OUT]   The capability properties of the UPnP module.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_upnp_capabilities_get(
    vtss_appl_upnp_capabilities_t    *const cap
);

/**
 * \brief UPnP API return codes
 */
typedef enum {
    VTSS_APPL_UPNP_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_UPNP),   /* Generic error code */
    VTSS_APPL_UPNP_ERROR_MODE,                                            /* Illegal UPnP Mode */
    VTSS_APPL_UPNP_ERROR_ADV_INT,                                         /* Illegal Advertisement Interval */
    VTSS_APPL_UPNP_ERROR_TTL,                                             /* Illegal TTL */
    VTSS_APPL_UPNP_ERROR_IP_ADDR_MODE,                                    /* Illegal IP Addressing Mode */
    VTSS_APPL_UPNP_ERROR_IP_INTF_VID,                                     /* Illegal IP Interface VLAN ID */
    VTSS_APPL_UPNP_ERROR_STACK_STATE,                                     /* Illegal primary/secondary switch state */
    VTSS_APPL_UPNP_ERROR_CAPABILITY_TTL,                                  /* Capability: TTL isn't writeable */
    VTSS_APPL_UPNP_OK,
} vtss_appl_upnp_error_t;

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_UPNP_H_ */
