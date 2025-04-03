/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief Public Port API
 * \details This header file describes Port control functions and types
 */

#ifndef _VTSS_APPL_PORT_H_
#define _VTSS_APPL_PORT_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/module_id.h>
#include <microchip/ethernet/switch/api/port.h>
#include <microchip/ethernet/board/api.h>
#include <vtss_phy_api.h>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_BITWISE() */

/**
 * Definition of error return codes return by the function in case of
 * malfunctioning behavior
 */
enum {
    VTSS_APPL_PORT_RC_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_PORT), /**< Generic error code                                                                                    */
    VTSS_APPL_PORT_RC_VERIPHY_RUNNING,                               /**< VeriPHY still running                                                                                 */
    VTSS_APPL_PORT_RC_PARM,                                          /**< Illegal parameter                                                                                     */
    VTSS_APPL_PORT_RC_REG_TABLE_FULL,                                /**< Registration table full                                                                               */
    VTSS_APPL_PORT_RC_REQ_TIMEOUT,                                   /**< Timeout on message request                                                                            */
    VTSS_APPL_PORT_RC_MUST_BE_PRIMARY_SWITCH,                        /**< Not allowed at secondary switch                                                                       */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_MASK,                        /**< Illegal advertise disable bitmask (bits set outside of valid bits)                                    */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_HDX_AND_FDX,                 /**< Advertisement of both half and full duplex cannot be disabled simultaneously                          */
    VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_HDX,                          /**< Advertisement of half duplex cannot be enabled, because port doesn't support half duplex              */
    VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_10M,                          /**< Advertisement of 10 Mbps cannot be enabled, because port doesn't support that speed                   */
    VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_100M,                         /**< Advertisement of 100 Mbps cannot be enabled, because port doesn't support that speed                  */
    VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_1G,                           /**< Advertisement of 1 Gbps cannot be enabled, because port doesn't support that speed                    */
    VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_2500M,                        /**< Advertisement of 2.5 Gbps cannot be enabled, because port doesn't support that speed                  */
    VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_5G,                           /**< Advertisement of 5 Gbps cannot be enabled, because port doesn't support that speed                    */
    VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_10G,                          /**< Advertisement of 5 Gbps cannot be enabled, because port doesn't support that speed                    */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_HDX,                         /**< Advertisement of half duplex cannot be disabled on SFP ports when the port supports half duplex       */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_FDX,                         /**< Advertisement of full duplex cannot be disabled on SFP ports                                          */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_10M,                         /**< Advertisement of 10 Mbps cannot be disabled on SFP ports when the port supports that speed            */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_100M,                        /**< Advertisement of 100 Mbps cannot be disabled on SFP ports when the port supports that speed           */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_1G,                          /**< Advertisement of 1 Gbps cannot be disabled on SFP ports when the port supports that speed             */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_2500M,                       /**< Advertisement of 2.5 Gbps cannot be disabled on SFP ports when the port supports that speed           */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_5G,                          /**< Advertisement of 5 Gbps cannot be disabled on SFP ports when the port supports that speed             */
    VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_10G,                         /**< Advertisement of 10 Gbps cannot be disabled on SFP ports when the port supports that speed            */
    VTSS_APPL_PORT_RC_MTU,                                           /**< MTU is outside the legal range (between VTSS_MAX_FRAME_LENGTH_STANDARD and VTSS_MAX_FRAME_LENGTH_MAX) */
    VTSS_APPL_PORT_RC_EXCESSIVE_COLLISSION,                          /**< Cannot enable excessive collision because the port does not support half duplex                       */
    VTSS_APPL_PORT_RC_LOOP_PORT,                                     /**< Loop ports cannot be configured                                                                       */
    VTSS_APPL_PORT_RC_MEDIA_TYPE_CU,                                 /**< Cu not supported for the port                                                                         */
    VTSS_APPL_PORT_RC_MEDIA_TYPE_SFP,                                /**< SFP not supported for the port                                                                        */
    VTSS_APPL_PORT_RC_MEDIA_TYPE_DUAL,                               /**< Port is not a dual media port                                                                         */
    VTSS_APPL_PORT_RC_MEDIA_TYPE,                                    /**< Unknown media type                                                                                    */
    VTSS_APPL_PORT_RC_SPEED_DUAL_MEDIA_SFP,                          /**< On dual media ports selected as SFP, only speeds higher than 10 Mbps are supported                    */
    VTSS_APPL_PORT_RC_SPEED_DUAL_MEDIA_AUTO,                         /**< On dual media ports configured as dual media, speed and duplex must be auto                           */
    VTSS_APPL_PORT_RC_DUPLEX_HALF,                                   /**< Port does not support half duplex                                                                     */
    VTSS_APPL_PORT_RC_SPEED_NO_FORCE,                                /**< Port doesn't support forced mode, only auto                                                           */
    VTSS_APPL_PORT_RC_SPEED_NO_AUTO,                                 /**< Port doesn't support auto, only forced mode                                                           */
    VTSS_APPL_PORT_RC_SPEED_10M_FDX,                                 /**< Port doesn't support  10M Full duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED_10M_HDX,                                 /**< Port doesn't support  10M Half duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED_100M_FDX,                                /**< Port doesn't support 100M Full duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED_100M_HDX,                                /**< Port doesn't support 100M Half duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED_1G,                                      /**< Port doesn't support   1G                                                                             */
    VTSS_APPL_PORT_RC_SPEED_1G_HDX,                                  /**< Port doesn't support   1G Half duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED_2G5,                                     /**< Port doesn't support 2.5G                                                                             */
    VTSS_APPL_PORT_RC_SPEED_2G5_HDX,                                 /**< Port doesn't support 2.5G Half duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED_5G,                                      /**< Port doesn't support   5G                                                                             */
    VTSS_APPL_PORT_RC_SPEED_5G_HDX,                                  /**< Port doesn't support   5G Half duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED_10G,                                     /**< Port doesn't support  10G                                                                             */
    VTSS_APPL_PORT_RC_SPEED_10G_HDX,                                 /**< Port doesn't support  10G Half duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED_25G,                                     /**< Port doesn't support  25G                                                                             */
    VTSS_APPL_PORT_RC_SPEED_25G_HDX,                                 /**< Port doesn't support  25G Half duplex                                                                 */
    VTSS_APPL_PORT_RC_SPEED,                                         /**< Unknown speed                                                                                         */
    VTSS_APPL_PORT_RC_IFINDEX_NOT_PORT,                              /**< The ifindex is not a port interface                                                                   */
    VTSS_APPL_PORT_RC_FLOWCONTROL,                                   /**< Standard flow control is not supported on this interface                                              */
    VTSS_APPL_PORT_RC_FLOWCONTROL_WHILE_PFC,                         /**< Standard flowcontrol cannot be enabled together with PFC                                              */
    VTSS_APPL_PORT_RC_FLOWCONTROL_PFC,                               /**< Priorify flow control not supported on this platform/chip                                             */
    VTSS_APPL_PORT_RC_INVALID_PORT,                                  /**< Invalid port number                                                                                   */
    VTSS_APPL_PORT_RC_KR_NOT_SUPPORTED_ON_THIS_PLATFORM,             /**< This chip/platform does not support KR                                                                */
    VTSS_APPL_PORT_RC_KR_NOT_SUPPORTED_ON_THIS_INTERFACE,            /**< This interface does not support KR                                                                    */
    VTSS_APPL_PORT_RC_KR_FORCED_REQUIRES_SPEED_AUTO,                 /**< When force_clause_73 is set to true, speed must be set to MESA_SPEED_AUTO                             */
    VTSS_APPL_PORT_RC_FEC_ILLEGAL,                                   /**< Illegal/unknown FEC mode selected                                                                     */
    VTSS_APPL_PORT_RC_FEC_R_NOT_SUPPORTED_ON_THIS_PLATFORM,          /**< This chip/platform does not support R-FEC                                                             */
    VTSS_APPL_PORT_RC_FEC_RS_NOT_SUPPORTED_ON_THIS_PLATFORM,         /**< This chip/platform does not support RS-FEC                                                            */
    VTSS_APPL_PORT_RC_FEC_R_NOT_SUPPORTED_ON_THIS_INTERFACE,         /**< This interface does not support R-FEC                                                                 */
    VTSS_APPL_PORT_RC_FEC_RS_NOT_SUPPORTED_ON_THIS_INTERFACE,        /**< This interface does not support RS-FEC                                                                */
    VTSS_APPL_PORT_RC_ICLI_DUPLEX_AUTO_OBSOLETE,                     /**< ICLI no longer supports 'duplex auto [half | full]'                                                   */
};

/**
 * Port module capabilities.
 * Contains platform specific port definitions.
 */
typedef struct {
    /**
     * The max length of the frames counted in the last range of the frame
     * counter group.
     */
    uint32_t last_pktsize_threshold;

    /**
     * The smallest maximum acceptable ingress frame length that can be
     * configured.
     */
    uint32_t frame_length_max_min;

    /**
     * The largest maximum acceptable ingress frame length that can be
     * configured.
     */
    uint32_t frame_length_max_max;

    /**
     * True if running v2 of BASE-KR (Clause 73), which is for the JR2 family
     */
    mesa_bool_t has_kr_v2;

    /**
     * True if running v3 of BASE-KR (Clause 73), which is for the SparX5 family
     */
    mesa_bool_t has_kr_v3;

    /**
     * True if either has_kr_v2 or has_kr_v3 is true.
     */
    mesa_bool_t has_kr;

    /**
     * Contains aggregated capabilities for all ports on this platform.
     */
    meba_port_cap_t aggr_caps;

    /**
     * Contains number of ports on this device.
     */
    uint32_t port_cnt;

    /**
     * True if this platform has priority-based flow control. False if not.
     */
    mesa_bool_t has_pfc;
}  vtss_appl_port_capabilities_t;

/**
 * Get port module capabilities.
 *
 * \param cap [OUT] Port capabilities.
 *
 * \return Can only fail if \p cap is NULL, so in general no need to check
 * return code.
 */
mesa_rc vtss_appl_port_capabilities_get(vtss_appl_port_capabilities_t *cap);

/**
 * Port module's per-port capabilities.
 */
typedef struct {
    /**
     * Contains the static capabilities for this port.
     */
    meba_port_cap_t static_caps;

    /**
     * Are 0 if an SFP is not inserted.
     *
     * Otherwise it's a subset of the \p static_caps where port speed that the
     * SFP doesn't support are filtered out.
     */
    meba_port_cap_t sfp_caps;

    /**
     * True if this port supports BASE-KR (Clause 73).
     */
    mesa_bool_t has_kr;
}  vtss_appl_port_interface_capabilities_t;

/**
 * Get particular port's capabilities.
 *
 * \param ifindex [IN] Interface to get capabilities for.
 * \param if_caps [OUT] A particular port's capabilities.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_port_interface_capabilities_get(vtss_ifindex_t ifindex, vtss_appl_port_interface_capabilities_t *if_caps);

/**
 * Port media type.
 */
typedef enum {
    VTSS_APPL_PORT_MEDIA_CU,   /**< Port in Cu-only mode    */
    VTSS_APPL_PORT_MEDIA_SFP,  /**< Port in SFP-only mode   */
    VTSS_APPL_PORT_MEDIA_DUAL, /**< Port in dual media mode */
} vtss_appl_port_media_t;

/**
 * The Forward Error Correction (FEC) method to use - if any.
 */
typedef enum {
    VTSS_APPL_PORT_FEC_MODE_NONE,   /**< Do not use or - if using clause 73 aneg - request FEC         */
    VTSS_APPL_PORT_FEC_MODE_R_FEC,  /**< Force it to IEEE802.3by clause 74 R-FEC (Firecode)            */
    VTSS_APPL_PORT_FEC_MODE_RS_FEC, /**< Force it to IEEE802.3by clause 108 RS-FEC (Reed-Solomon)      */
    VTSS_APPL_PORT_FEC_MODE_AUTO,   /**< Let the application decide based on transceiver and port type */
} vtss_appl_port_fec_mode_t;

/**
 * Port configuration structure
 */
typedef struct {
    /**
     * Media type.
     *
     * VTSS_APPL_PORT_MEDIA_CU  may be used on copper ports and dual-media ports
     * VTSS_APPL_PORT_MEDIA_SFP may be used on SFP ports and dual-media ports
     * VTSS_APPL_PORT_MEDIA_DUAL may be used on dual-media ports, only
     *
     * If VTSS_APPL_PORT_MEDIA_CU or VTSS_APPL_PORT_MEDIA_SFP is used on a dual
     * media port, only that interface will be able to get link.
     */
    vtss_appl_port_media_t media_type;

    /**
     * Port speed.
     *
     * You can either force a port into a given speed by setting it to a valid
     * speed != MESA_SPEED_AUTO.
     *
     * If you set it to MESA_SPEED_AUTO, then the following takes place:
     *   If the port's capabilities is < 10G, then clause 28/37 aneg starts.
     *   Notice that on non-10G-PHY ports, at most 1G can be achieved with
     *   clause 28/37.
     *
     *   Otherwise, if the port's capabilities is >= 10G, then the following
     *   happens:
     *     If the SFP's maximum speed is less than 10G, then clause 28/37 aneg
     *     starts and at most 1G can be achieved. If this is not what you want,
     *     force the port to a given speed.
     *
     *     If the "SFP" is
     *       a 10G Cu Backplane (10GBASE-KR(-S)) or
     *       a 25G Cu backplane (25GBASE-KR(-S)) or
     *       a 25G DAC cable    (25GBSAE-CR(-S)), then
     *     clause 73 will run if the port supports KR. Otherwise the "SFP" is
     *     considered an optical SFP, and the following will be used.
     *
     *     If the SFP is an optical SFP, the minimum of the port's capability
     *     and the SFP's max speed will be used, unless \p force_clause_73 is
     *     set to true, in which case clause 73 will run if the port and SFP
     *     support it (if not, an operational warning will be issued).
     *
     * If set to MESA_SPEED_AUTO and using clause 28/37 aneg, also the adv_dis
     * flags have a meaning.
     */
    mesa_port_speed_t speed;

    /**
     * Selects the auto negotion advertisements to *disable*.
     *
     * These flags are only used when \p speed is MESA_SPEED_AUTO,
     *
     * Use THE MEPA_ADV_DIS_xxx flag defines (e.g. MEPA_ADV_DIS_FDX)
     *
     * In the default configuration, these flags have bits set for unsupported
     * speeds and duplex modes. The remaining flags are cleared.
     *
     * Only true copper ports (MEBA_PORT_CAP_COPPER) may change these flags.
     */
    mepa_adv_dis_t adv_dis;

    /**
     * Duplex mode used when speed is forced (not auto).
     * True means full duplex, false means half duplex.
     */
    mesa_bool_t fdx;

    /**
     * Flow control (Standard 802.3x)
     */
    mesa_bool_t flow_control;

    /**
     * Priority Flow control (802.1Qbb)
     * May only contain true elements when has_fpc is true.
     * If it contains at least one true element, \p flow_control must be false.
     */
    mesa_bool_t pfc[MESA_PRIO_ARRAY_SIZE];

    /**
     * Maximum accepted frame length.
     */
    uint32_t max_length;

    /**
     * Excessive collision continuation.
     * Can only be set on ports that support half duplex.
     */
    mesa_bool_t exc_col_cont;

    /**
     * True to do 802.3 frame length check for ethertypes below 0x0600
     */
    mesa_bool_t frame_length_chk;

    /**
     * Administrative state (shut/no shut)
     */
    meba_port_admin_state_t admin;

    /**
     * This allows for controlling whether KR's parallel-detect runs or not
     * after an unsucessful aneg.
     *
     * Default is true, indicating that parallel detect will be attempted after
     * an unsuccessful KR auto-negotiation.
     */
    mesa_bool_t clause_73_pd;

    /**
     * This can be used to force a 10G or 25G port to run KR.
     * It can only be set on ports that support KR.
     * If it is set, \p speed must be set to MESA_SPEED_AUTO for it to take
     * effect.
     *
     * See \p speed for more information.
     */
    mesa_bool_t force_clause_73;

    /**
     * This controls whether the port is forced to run R-FEC (a.k.a. Firecode,
     IEEE802.3by, clause 74), RS-FEC (IEEE 802.3, clause 108), not to run any
     * FEC at all, or let it be up to the application to control it based on the
     * inserted SFP (recommended).
     *
     * Default is VTSS_APPL_PORT_FEC_MODE_AUTO.
     */
    vtss_appl_port_fec_mode_t fec_mode;

    /**
     * PHY power mode
     */
    vtss_phy_power_mode_t power_mode;

    /**
      * Textual interface description. Must include terminating '\0'.
      */
    char dscr[201];
} vtss_appl_port_conf_t;

/**
 * Operators for adv_dis flags
 */
VTSS_ENUM_BITWISE(mepa_adv_dis_t);

/**
 * Function for getting a default port configuration
 *
 * This depends on the port number because of a given port's capabilities.
 * This function does not work for CPU ports.
 *
 * \param ifindex [IN]  The logical interface index/number.
 * \param conf    [OUT] Pointer to where to put the current configuration
 *
 * \return VTSS_RC_OK if configuration get was perform correct else error code
 */
mesa_rc vtss_appl_port_conf_default_get(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *conf);

/**
 * Function for getting the current port configuration
 *
 * Works for CPU ports.
 *
 * \param ifindex [IN]  The logical interface index/number.
 * \param conf    [OUT] Pointer to where to put the current configuration
 *
 * \return VTSS_RC_OK if configuration get was perform correct else error code
 */
mesa_rc vtss_appl_port_conf_get(vtss_ifindex_t ifindex, vtss_appl_port_conf_t *conf);

/**
 * Function for setting the current port configuration.
 *
 * Works for CPU ports.
 *
 * \param ifindex [IN]  The logical interface index/number.
 * \param conf    [OUT] Pointer to new configuration
 *
 * \return VTSS_RC_OK if configuration set was perform correct else error code
 */
mesa_rc vtss_appl_port_conf_set(vtss_ifindex_t ifindex, const vtss_appl_port_conf_t *conf);

/**
 * Status for green Ethernet parameters.
 */
typedef struct {
    mesa_bool_t actiphy_capable;            /**< True when port is able to use actiphy */
    mesa_bool_t perfectreach_capable;       /**< True when port is able to use perfectreach */
    mesa_bool_t actiphy_power_savings;      /**< True when port is saving power due to actiphy */
    mesa_bool_t perfectreach_power_savings; /**< True when port is saving power due to perfectreach */
} vtss_appl_port_power_status_t;

/**
 * Flags containing a reason why a port may not be operational.
 */
typedef enum {
    VTSS_APPL_PORT_OPER_WARNING_CU_SFP_IN_DUAL_MEDIA_SLOT                = 0x001, /**< This dual media SFP slot does not support CuSFPs                                         */
    VTSS_APPL_PORT_OPER_WARNING_CU_SFP_SPEED_AUTO                        = 0x002, /**< CuSFPs are only supported with speed auto                                                */
    VTSS_APPL_PORT_OPER_WARNING_FORCED_SPEED_NOT_SUPPORTED_BY_SFP        = 0x004, /**< The configured, forced speed is not supported by the inserted SFP                        */
    VTSS_APPL_PORT_OPER_WARNING_PORT_DOES_NOT_SUPPORT_SFP                = 0x008, /**< The port's minimum speed is higher than the SFP's maximum speed                          */
    VTSS_APPL_PORT_OPER_WARNING_SFP_NOMINAL_SPEED_HIGHER_THAN_PORT_SPEED = 0x010, /**< The SFP's nominal (maximum) speed is higher than the actual port speed (=> may not work) */
    VTSS_APPL_PORT_OPER_WARNING_SFP_CANNOT_RUN_CLAUSE_73                 = 0x020, /**< The inserted SFP can not run KR (conf.force_clause_73 == true)                           */
    VTSS_APPL_PORT_OPER_WARNING_SFP_UNREADABLE_IN_THIS_PORT              = 0x040, /**< The port does not allow for reading SFP ROM. Results are unpredictable                   */
    VTSS_APPL_PORT_OPER_WARNING_SFP_READ_FAILED                          = 0x080, /**< The inserted SFP's ROM cannot be read. Results are unpredictable                         */
    VTSS_APPL_PORT_OPER_WARNING_SFP_DOES_NOT_SUPPORT_HDX                 = 0x100, /**< The inserted SFP does not support half duplex (100 Mbps SFP)                             */
    VTSS_APPL_PORT_OPER_WARNING_CANNOT_OBEY_ANEG_FC_WHEN_PFC_ENABLED     = 0x200, /**< Cannot obey anegged F/C enable while PFC is enabled                                      */
} vtss_appl_port_oper_warnings_t;

/**
 * Operators for oper_warnings flags
 */
VTSS_ENUM_BITWISE(vtss_appl_port_oper_warnings_t);

/**
 * SFP type
 */
typedef enum {
    VTSS_APPL_PORT_SFP_TYPE_NONE,    /**< Port is not an SFP port or no SFP is plugged in    */
    VTSS_APPL_PORT_SFP_TYPE_UNKNOWN, /**< SFP is plugged in, but its type cannot be detected */
    VTSS_APPL_PORT_SFP_TYPE_OPTICAL, /**< Optical SFP (fiber)                                */
    VTSS_APPL_PORT_SFP_TYPE_CU,      /**< Copper SFP (RJ45, twisted pair)                    */
    VTSS_APPL_PORT_SFP_TYPE_CU_BP,   /**< Copper Backplane                                   */
    VTSS_APPL_PORT_SFP_TYPE_DAC,     /**< Directly Attached Cable (coaxial)                  */
} vtss_appl_port_sfp_type_t;

/**
 * Tells which aneg method this interface uses with its link partner to obtain
 * a common speed and other parameters with.
 */
typedef enum {
    VTSS_APPL_PORT_ANEG_METHOD_UNKNOWN,   /**< Aneg method is not yet known, because no SFP is plugged in */
    VTSS_APPL_PORT_ANEG_METHOD_NONE,      /**< Aneg method is not used (forced speed)                     */
    VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_28, /**< IEEE 802.3 clause 28 aneg (PHY/twisted pair)               */
    VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_37, /**< IEEE 802.3 clause 37 aneg (SFP (but not CuSFP))            */
    VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_73, /**< IEEE 802.3 clause 73 aneg (KR)                             */
} vtss_appl_port_aneg_method_t;

/**
 * Port status
 */
typedef struct {
    /**
     * True if port has link, false otherwise.
     * Check \p oper_warnings if you think the port should have had link, but it
     * doesn't.
     */
    mesa_bool_t link;

    /**
     * Current speed of port.
     * This may me MESA_SPEED_UNDEFINED if the port doesn't have link.
     */
    mesa_port_speed_t speed;

    /**
     * Aneg method - if any - this port will try to use or is using or has been
     * using in order to obtain link.
     */
    vtss_appl_port_aneg_method_t aneg_method;

    /**
     * True if port is in full duplex, false if in half.
     * Only valid if \p link is true.
     */
    mesa_bool_t fdx;

    /**
     * True if current interface is SFP, false if copper.
     * This is fixed one or the other on non-dual-media interfaces.
     */
    mesa_bool_t fiber;

    /**
     * The type of SFP plugged in. Can only be != VTSS_APPL_PORT_SFP_TYPE_NONE
     * if SFP is plugged in. If != NONE and fiber == false an SFP is plugged in
     * but it is the RJ45 port that is being used.
     */
    vtss_appl_port_sfp_type_t sfp_type;

    /**
     * The minimum speed of the SFP. Can only be != MESA_SPEED_UNDEFINED if an
     * SFP is plugged in.
     */
    mesa_port_speed_t sfp_speed_min;

    /**
     * The maximum speed of the SFP. Can only be != MESA_SPEED_UNDEFINED if an
     * SFP is plugged in.
     */
    mesa_port_speed_t sfp_speed_max;

    /**
     * Auto negotiation result in case the port has run clause 28 or 37 aneg.
     */
    mesa_aneg_t aneg;

    /**
     * True if this port supports Clause 73 (KR).
     * This value never changes for a given port.
     */
    mesa_bool_t has_kr;

    /**
     * Static port capabilities flags. These never change.
     *
     * The bits are one or more of MEBA_PORT_CAP_xxx, e.g. MEBA_PORT_CAP_1G_FDX.
     */
    meba_port_cap_t static_caps;

    /**
     * Are 0 if no SFP is inserted.
     *
     * Otherwise it's a subset of the \p static_caps where port speed that the
     * SFP doesn't support are filtered out.
     *
     * If using it to runtime figure out port capabilities, then you should
     * consider whether you need the caps for the currently active port (given
     * a dual media port is in play).
     * If you need that, do the following:
     *     meba_port_cap_t caps = status.fiber && sfp_type != VTSS_APPL_PORT_SFP_TYPE_NONE ?
     *                            status.sfp_caps : status.static_caps;
     */
    meba_port_cap_t sfp_caps;

    /**
     * MSA vendor information read from the SFP's ROM or hand-crafted in case a
     * preprovisioned driver for the SFP is found.
     *
     * If sfp_info.vendor_pn[0] == '\0', no SFP is inserted.
     *
     * See also SFF-8472.
     */
    meba_sfp_device_info_t sfp_info;

    /**
     * The interface the port is currently using.
     */
    mesa_port_interface_t mac_if;

    /**
     * Contains the current status of FEC.
     */
    vtss_appl_port_fec_mode_t fec_mode;

    /**
     * Current green Ethernet modes
     */
    vtss_appl_port_power_status_t power;

    /**
     * Operational status of this port.
     *
     * This is a bit-mask of VTSS_APPL_PORT_OPER_WARNING_xxx flags.
     * Currently can only be set on SFP or dual media ports, and only if the
     * plugged in SFP isn't supported on the port.
     */
    vtss_appl_port_oper_warnings_t oper_warnings;

    /**
     * If true, the port is internally used as a loop port and is not
     * configurable.
     * A port can become a loop port in two ways:
     *   1) Fixed compiled as an up injection loop port
     *   2) Fixed compiled as a remote mirror loop port
     */
    mesa_bool_t loop_port;

    /**
     * Number of link ups since last cleared.
     * Cleared with vtss_appl_port_statistics_clear();
     */
    uint32_t link_up_cnt;

    /**
     * Number of link downs since last cleared.
     */
    uint32_t link_down_cnt;
} vtss_appl_port_status_t;

/**
 * Function for getting the current port status
 *
 * \param ifindex [IN]  The logical interface index/number.
 * \param status  [OUT] Pointer to where to put the port status
 *
 * \return VTSS_RC_OK if status get was perform correct else error code
 */
mesa_rc vtss_appl_port_status_get(vtss_ifindex_t ifindex, vtss_appl_port_status_t *status);

/**
 * Function for getting statistics for a specific port
 *
 * \param ifindex    [IN]  The logical interface index/number.
 * \param statistics [OUT] Pointer to where to put the counters
 *
 * \return VTSS_RC_OK if counters are valid else error code
 */
mesa_rc vtss_appl_port_statistics_get(vtss_ifindex_t ifindex, mesa_port_counters_t *statistics);

/**
 * Function for clearing statistics for a specific port.
 *
 * Notice that it also clears vtss_appl_port_status_t::link_up_cnt and
 * vtss_appl_port_status_t::link_down_cnt.
 *
 * \param ifindex [IN]  The logical interface index/number.
 *
 * \return VTSS_RC_OK if counter was cleared else error code
 */
mesa_rc vtss_appl_port_statistics_clear(vtss_ifindex_t ifindex);

/**
 * VeriPHY result for one port
 */
typedef struct {
    mepa_cable_diag_status_t status_pair_a; /**< Status, pair A */
    mepa_cable_diag_status_t status_pair_b; /**< Status, pair B */
    mepa_cable_diag_status_t status_pair_c; /**< Status, pair C */
    mepa_cable_diag_status_t status_pair_d; /**< Status, pair D */
    uint8_t                  length_pair_a; /**< Length (meters) pair A */
    uint8_t                  length_pair_b; /**< Length (meters) pair B */
    uint8_t                  length_pair_c; /**< Length (meters) pair_C */
    uint8_t                  length_pair_d; /**< Length (meters) pair D */
} vtss_appl_port_veriphy_result_t;

#endif  /* _VTSS_APPL_PORT_H_ */

