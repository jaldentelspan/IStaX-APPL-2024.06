/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \file erps.h
 * \brief Public ERPS (Ethernet Ring Protection Switching) API
 * \details This header file describes the ERPS (Ethernet Ring Protection
 * Switching) control functions and types.
 */

#ifndef _VTSS_APPL_ERPS_H_
#define _VTSS_APPL_ERPS_H_

#include <microchip/ethernet/switch/api.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/vlan.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/cfm.hxx>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Definition of error return codes.
 * See also erps_error_txt() in erps.cxx.
 */
enum {
    VTSS_APPL_ERPS_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_ERPS),    /**< Invalid parameter                                                          */
    VTSS_APPL_ERPS_RC_INTERNAL_ERROR,                                                 /**< Internal error. Requires code update                                       */
    VTSS_APPL_ERPS_RC_NO_SUCH_INSTANCE,                                               /**< ERPS instance is not created                                               */
    VTSS_APPL_ERPS_RC_INVALID_PORT0_IFINDEX,                                          /**< Invalid port0 ifindex                                                      */
    VTSS_APPL_ERPS_RC_INVALID_PORT1_IFINDEX,                                          /**< Invalid port1 ifindex                                                      */
    VTSS_APPL_ERPS_RC_INVALID_VERSION,                                                /**< Invalid version                                                            */
    VTSS_APPL_ERPS_RC_INVALID_RING_TYPE,                                              /**< Invalid ring-type                                                          */
    VTSS_APPL_ERPS_RC_INVALID_RING_ID,                                                /**< Invalid ring ID                                                            */
    VTSS_APPL_ERPS_RC_INVALID_LEVEL,                                                  /**< Invalid MEG level                                                          */
    VTSS_APPL_ERPS_RC_INVALID_CONTROL_VLAN,                                           /**< Invalid control VLAN                                                       */
    VTSS_APPL_ERPS_RC_INVALID_PCP,                                                    /**< Invalid PCP                                                                */
    VTSS_APPL_ERPS_RC_INVALID_CONNECTED_RING_INST,                                    /**< Invalid instance number specified for connected ring                       */
    VTSS_APPL_ERPS_RC_INVALID_SF_TRIGGER_PORT0,                                       /**< Invalid SF trigger for port0                                               */
    VTSS_APPL_ERPS_RC_INVALID_SF_TRIGGER_PORT1,                                       /**< Invalid SF trigger for port1                                               */
    VTSS_APPL_ERPS_RC_INVALID_SMAC_PORT0,                                             /**< Port0 SMAC not a unicast MAC address                                       */
    VTSS_APPL_ERPS_RC_INVALID_SMAC_PORT1,                                             /**< Port1 SMAC not a unicast MAC address                                       */
    VTSS_APPL_ERPS_RC_INVALID_NODE_ID,                                                /**< Node ID not a unicast MAC address                                          */
    VTSS_APPL_ERPS_RC_INVALID_WTR,                                                    /**< Invalid WTR time                                                           */
    VTSS_APPL_ERPS_RC_INVALID_GUARD_TIME,                                             /**< Invalid guard-time                                                         */
    VTSS_APPL_ERPS_RC_INVALID_HOLD_OFF,                                               /**< Invalid hold-off time                                                      */
    VTSS_APPL_ERPS_RC_INVALID_RPL_MODE,                                               /**< Invalid RPL mode                                                           */
    VTSS_APPL_ERPS_RC_INVALID_RPL_PORT,                                               /**< Invalid RPL port                                                           */
    VTSS_APPL_ERPS_RC_INVALID_PROTECTED_VLANS,                                        /**< At least one VLAN must be protected                                        */
    VTSS_APPL_ERPS_RC_CONTROL_VLAN_CANNOT_BE_PROTECTED_VLAN,                          /**< The control VLAN cannot be one of the protected VLANs                      */
    VTSS_APPL_ERPS_RC_RING_TYPE_MUST_BE_MAJOR_WHEN_USING_V1,                          /**< Ring type must be major when using G.8032v1                                */
    VTSS_APPL_ERPS_RC_RING_ID_MUST_BE_1_WHEN_USING_V1,                                /**< Ring ID must be 1 when using G.8032v1                                      */
    VTSS_APPL_ERPS_RC_REVERTIVE_MUST_BE_TRUE_WHEN_USING_V1,                           /**< Must use revertive switching when using G.8032v1                           */
    VTSS_APPL_ERPS_RC_INTERCONNECTED_SUB_RING_CANNOT_REFERENCE_ITSELF,                /**< Interconnected sub-rings cannot have itself as referenced connected ring   */
    VTSS_APPL_ERPS_RC_PORT0_MEP_MUST_BE_SPECIFIED,                                    /**< Port0 MEP must be specified when SF-trigger is MEP                         */
    VTSS_APPL_ERPS_RC_PORT1_MEP_MUST_BE_SPECIFIED,                                    /**< Port1 MEP must be specified when SF-trigger is MEP                         */
    VTSS_APPL_ERPS_RC_0_1_MEP_IDENTICAL,                                              /**< Port0 and Port1 MEPs cannot be the same                                    */
    VTSS_APPL_ERPS_RC_0_1_IFINDEX_IDENTICAL,                                          /**< Port0 and Port1 interfaces cannot be the same                              */
    VTSS_APPL_ERPS_RC_INTERCONNECTED_SUB_RING_CANNOT_USE_RPL_PORT1,                   /**< Interconnected sub-ring cannot use Port1 as RPL port                       */
    VTSS_APPL_ERPS_RC_LIMIT_REACHED,                                                  /**< The maximum number of ERPS instances is reached                            */
    VTSS_APPL_ERPS_RC_TWO_INST_ON_SAME_INTERFACES_WITH_OVERLAPPING_VLANS,             /**< Two instances with ring ports in common have overlapping VLANs             */
    VTSS_APPL_ERPS_RC_TWO_INST_ON_SAME_INTERFACES_WITH_SAME_CONTROL_VLAN_AND_RING_ID, /**< Two instances with ring ports in common have same control VLAN and ring ID */
    VTSS_APPL_ERPS_RC_OUT_OF_MEMORY,                                                  /**< Out of memory                                                              */
    VTSS_APPL_ERPS_RC_INVALID_COMMAND,                                                /**< Invalid command                                                            */
    VTSS_APPL_ERPS_RC_COMMAND_NOT_SUPPORTED_WHEN_USING_V1,                            /**< Invalid command when using G.8032v1.                                       */
    VTSS_APPL_ERPS_RC_NOT_ACTIVE,                                                     /**< Instance is not active                                                     */
    VTSS_APPL_ERPS_RC_NOT_READY_TRY_AGAIN_IN_A_FEW_SECONDS,                           /**< ERPS is not yet ready. Try again in a few seconds                          */
    VTSS_APPL_ERPS_RC_HW_RESOURCES,                                                   /**< Out of H/W resources                                                       */
};

/**
 * Capabilities of ERPS
 */
typedef struct {
    /**
     * Maximum number of configured ERPS instances.
     */
    uint32_t inst_cnt_max;

    /**
     * The maximum number of seconds vtss_appl_erps_conf_t::wtr_secs can be set
     * to.
     */
    uint32_t wtr_secs_max;

    /**
     * The maximum number of milliseconds vtss_appl_erps_conf_t::guard_time_ms.
     */
    uint32_t guard_time_msecs_max;

    /**
     * The maximum number of milliseconds vtss_appl_erps_conf_t::hold_off_msecs
     */
    uint32_t hold_off_msecs_max;
} vtss_appl_erps_capabilities_t;

/**
 * \brief Get ERPS capabilities.
 *
 * \param cap [OUT] ERPS capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_capabilities_get(vtss_appl_erps_capabilities_t *cap);

/**
 * Enumeration of ERPS ring types.
 */
typedef enum {
    VTSS_APPL_ERPS_RING_TYPE_MAJOR,              /**< ERPS major ring (G.8001-2016, clause 3.2.39)                          */
    VTSS_APPL_ERPS_RING_TYPE_SUB,                /**< ERPS sub-ring   (G.8001-2016, clause 3.2.66)                          */
    VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB, /**< ERPS sub-ring on an interconnection node (G.8001-2016, clause 3.2.66) */
} vtss_appl_erps_ring_type_t;

/**
 * Enumeration of the Ring Protection Link (RPL) mode of the port.
 */
typedef enum {
    VTSS_APPL_ERPS_RPL_MODE_NONE,     /**< This switch doesn't have the RPL port in the ring                     */
    VTSS_APPL_ERPS_RPL_MODE_OWNER,    /**< This switch is RPL owner for the ring    (G.8001-2016, clause 3.2.61) */
    VTSS_APPL_ERPS_RPL_MODE_NEIGHBOR, /**< This switch is RPL neighbor for the ring (G.8001-2016, clause 3.2.60) */
} vtss_appl_erps_rpl_mode_t;

/**
 * Enumeration of port 0 and port 1.
 */
typedef enum {
    VTSS_APPL_ERPS_RING_PORT0, /**< Port0 (a.k.a. East) */
    VTSS_APPL_ERPS_RING_PORT1, /**< Port1 (a.k.a. West) */
} vtss_appl_erps_ring_port_t;

/**
 * Enumeration of ERPS version.
 * The enumeration matches the number to store in the R-APS PDU.
 */
typedef enum {
    VTSS_APPL_ERPS_VERSION_V1 = 0, /**< ERPS version 1 */
    VTSS_APPL_ERPS_VERSION_V2 = 1  /**< ERPS version 2 */
} vtss_appl_erps_version_t;

/**
 * Signal fail can either come from the physical link on a given port or from a
 * Down-MEP.
 */
typedef enum {
    VTSS_APPL_ERPS_SF_TRIGGER_LINK, /**< Signal Fail comes from the physical port's link state */
    VTSS_APPL_ERPS_SF_TRIGGER_MEP,  /**< Signal Fail comes from a down-MEP                     */
} vtss_appl_erps_sf_trigger_t;

/**
 * Interconnected sub-ring configuration.
 * Only used when the ring-type is VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB.
 */
typedef struct {
    /**
     * For a sub-ring on an interconnection node, this must reference the
     * instance ID of the ring to which this sub-ring is connected.
     *
     * The referenced ring must not be of type
     * VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB.
     */
    uint32_t connected_ring_inst;

    /**
     * Controls whether the ring referenced by \p connected_ring_inst shall
     * propagate R-APS flush PDUs whenever this sub-ring's topology changes.
     */
    mesa_bool_t tc_propagate;

} vtss_appl_erps_interconnect_conf_t;

/**
 * Configuration for a particular ring-port.
 * port1 is not used when ring type is
 * VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB, unless it's with a virtual
 * channel, in which case the smac is used.
 */
typedef struct {
    /**
     * Selects whether Signal Fail (SF) comes from the link state of a given
     * interface, or from a Down-MEP.
     */
    vtss_appl_erps_sf_trigger_t sf_trigger;

    /**
     * Interface index of ring port.
     * Must always be filled in.
     */
    vtss_ifindex_t ifindex;

    /**
     * Reference to a down-MEP that provides SF for the ring port that this
     * structure instance represents.
     *
     * The MEP's residence port must be the same as \p ifindex or the instance
     * will get an operational warning and use link as SF trigger until the
     * problem is remedied.
     *
     * Only used when sf_trigger is VTSS_APPL_ERPS_SF_TRIGGER_MEP.
     */
    vtss_appl_cfm_mep_key_t mep;

    /**
     * Source MAC address (must be unicast) used in R-APS PDUs sent on this
     * ring-port.
     *
     * If all-zeros, the ring-port's native MAC address will be used.
     */
    mesa_mac_t smac;
} vtss_appl_erps_ring_port_conf_t;

/**
 * Configuration of one ERPS instance
 */
typedef struct {
    /**
     * ERPS protocol version.
     *
     * VTSS_APPL_ERPS_VERSION_V1 refers to G.8032-2008 and its ammendment 1.
     * There are several restriction on using v1:
     *   - Only revertive switching is supported. If you change to non-revertive
     *     while having configured v1 or change the version number to v1 while
     *     having configured non-revertive switching, an error will be issued.
     *   - Only ring type VTSS_APPL_ERPS_RING_MAJOR is supported. If you
     *     change a sub-ring configuration while currently using v2 to using v1
     *     or change a v1 configuration to be a sub-ring, you will get an error
     *     if the instance is administratively enabled.
     *   - Only Ring ID 1 is supported. If you change the version from v2 to v1
     *     while having configured a Ring ID different from 1 or change the ring
     *     ID to a ring ID different from 1 when v1 is configured, you will get
     *     an error if the instance is administratively enabled.
     *
     * VTSS_APPL_ERPS_VERSION_V2 refers to G.8032-2016 and supports sub-rings
     * and non-revertive switching.
     *
     * Default is VTSS_APPL_ERPS_VERSION_V2.
     */
    vtss_appl_erps_version_t version;

    /**
     * Both Major and Sub-rings are supported.
     *
     * If set to VTSS_APPL_ERPS_RING_TYPE_MAJOR:
     *    Having two ring-ports, so both port0 and port1 must be configured.
     *
     * Is set to VTSS_APPL_ERPS_RING_TYPE_SUB:
     *   Instance has two ring-ports because this is not an interconnect switch,
     *   so both port0 and port1 must be configured.
     *
     * Is set to VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB:
     *   Instance has only one ring-port, so only port0 must be configured.
     *   A possible configuration of port1 may get lost.
     *
     * See also restrictions under \p version.
     */
    vtss_appl_erps_ring_type_t ring_type;

    /**
     * Controls whether to use a virtual channel with a sub-ring.
     *
     * This boolean is only available when \p ring_type is
     * VTSS_APPL_ERPS_RING_TYPE_SUB or
     * VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB.
     *
     * The functionality is as follows depending on \p ring_type:
     * VTSS_APPL_ERPS_RING_TYPE_SUB:
     *   If virtual_channel is false, R-APS PDUs are forwarded between ring
     *   ports whether they are blocked or not.
     *
     *   If virtual_channel is true, R-APS PDUs are not forwarded from a
     *   non-blocked ring port to a blocked ring port. R-APS PDUs may still be
     *   sent by the ERP control process on a blocked ring port.
     *
     * VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB:
     *   If virtual_channel is false, R-APS PDUs for this ERP are terminated at
     *   port0 and only transmitted by this ring's ERP process towards port0.
     *   port1 is not used for this ring type.
     *
     *   If virtual_channel is true, R-APS PDUs are forwarded between port0 and
     *   the referenced connected ring's ring ports if the ports are not
     *   blocked.
     */
    mesa_bool_t virtual_channel;

    /**
     * Interconnected sub-ring configuration.
     *
     * This configuration is only used when \p ring_type is
     * VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB, which requires \p version to
     * be configured as G.8032v2.
     *
     * See the structure's members for descriptions.
     */
    vtss_appl_erps_interconnect_conf_t interconnect_conf;

    /**
     * Ring ID.
     *
     * The Ring ID is used - along with the control VLAN - to identify R-APS
     * PDUs as belonging to a particular ring.
     *
     * The Ring ID is a number in range [1; 239] (default 1) that goes into the
     * last byte of the destination MAC address of the R-APS PDUs (XX):
     *   01-19-a7-00-00-XX
     *
     * If \p version is 1, only ring ID 1 is supported (see \p version for
     * details).
     */
    uint8_t ring_id;

    /**
     * Node ID.
     *
     * The Node ID is used inside the R-APS specific PDU to uniquely identify
     * this node (switch) on the ring.
     *
     * It must be a valid unicast MAC address, and if left to all-zeros, the
     * chassis MAC address (the switch's own management MAC address) will be
     * used.
     */
    mesa_mac_t node_id;

    /**
     * MD/MEG Level of R-APS PDUs we transmit.
     *
     * Notice on Ocelot and Serval-1 platforms:
     *   This MEG level must be higher than the MEG level of a possible MEP on
     *   the ring-ports.
     *
     * Notice on other platforms:
     *   This MEG level must be higher than the MEG level of a possible MEP on
     *   the same VLAN on the ring ports.
     *
     * Valid range is [0; 7], with 7 as default.
     */
    uint8_t level;

    /**
     * The VLAN on which R-APS PDUs are transmitted and received on the ring
     * ports.
     *
     * Make sure to add this VLAN to both ring ports, or pass-through of R-APS
     * PDUs from one ring port to another will not work.
     *
     * Ring instances on the same ports must use different control VLANs.
     */
    mesa_vid_t control_vlan;

    /**
     * The PCP value used in the VLAN tag of the R-APS PDUs.
     *
     * Valid range is [0; 7], with 7 as default.
     */
    mesa_pcp_t pcp;

    /**
     * Ring port0 and port1 configurations.
     *
     * Index 0 is for port0.
     * Index 1 is for port1.
     *
     * Index 1 is not used when \p ring_type is
     * VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB.
     *
     * See vtss_appl_erps_ring_port_conf_t for details.
     */
    vtss_appl_erps_ring_port_conf_t ring_port_conf[2];

    /**
     * When set, the port recovery mode is revertive, that is traffic switches
     * back to the working transport entity, i.e. blocked on the RPL. If a
     * defect is cleared, the traffic channel reverts after the expiry of the
     * WTR (Wait-To-Restore) timer.
     *
     * When cleared, the port recovery mode is non-revertive and traffic channel
     * continues to use the RPL, if it has not failed, after a switch condition
     * has cleared.
     *
     * Default is true (see G.8032-2015, clause 10.1.13).
     *
     * If configured as G.8032v1, you cannot set this to false (see also \p
     * version).
     */
    mesa_bool_t revertive;

    /**
     * When #revertive is true, this member tells how many seconds to wait
     * before restoring to the normal condition (block RPL) after a fault
     * condition has cleared.
     *
     * Valid range is [1; vtss_appl_erps_capabilities_t::wtr_secs_max] seconds
     * with a default of 300 seconds.
     */
    uint32_t wtr_secs;

    /**
     * The forwarding method, in which R-APS PDUs are copied and forwarded at
     * every ring node, can result in a PDU corresponding to an old request,
     * that is no longer relevant, being received by ring nodes. Reception of an
     * old R-APS PDU may result in errnoneous ring state interpretation by some
     * ring nodes. The guard timer is used to prevent ring nodes from acting
     * upon outdated R-APS PDUs and prevents the possibility of forming a closed
     * loop.
     *
     * Valid range is [10; vtss_appl_erps_capabilities_t::guard_time_msecs_max]
     * milliseconds in steps of 10 ms with a default of 500 ms.
     */
    uint32_t guard_time_msecs;

    /**
     * Hold-off timer value.
     *
     * When a new defect or more severe defect occurs, this event will not be
     * reported until a hold-off timer expires. When the hold-off timer expires,
     * it will be checked whether a defect still exists. If it does, that defect
     * will be reported to protection switching.
     *
     * The hold-off timer is measured in milliseconds, and valid values are in
     * the range [0; vtss_appl_erpps_capabilities_t::hold_off_msecs_max] in
     * steps of 100 milliseconds.
     * Default is 0, which means immediate reporting of the defect.
     */
    uint32_t hold_off_msecs;

    /**
     * Ring Protection Link mode.
     *
     * Notice that even though RPL neighbor was not included in v1 of G.8032,
     * you may still configure this option when \p version is set to
     * VTSS_APPL_ERPS_VERSION_V1, since it does not affect R-APS PDUs, and just
     * causes the non-owner-end of the RPL to also block its end.
     */
    vtss_appl_erps_rpl_mode_t rpl_mode;

    /**
     * RPL port.
     * Indicates whether it is port0 or port1 that is the RPL.
     *
     * There is no restriction on which ring link on a ring may be set as RPL.
     *
     * Not used if \p rpl_mode is VTSS_APPL_ERPS_RPL_MODE_NONE.
     */
    vtss_appl_erps_ring_port_t rpl_port;

    /**
     * Bit-array indicating whether a VLAN is protected by this ring instance
     * ('1') or not ('0').
     *
     * Use VTSS_BF_GET() and VTSS_BF_SET() macros to manipulate and obtain its
     * entries.
     */
    uint8_t protected_vlans[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

    /**
     * The administrative state of this ERPS instance (default false).
     * Set to true to make it function normally and false to make it cease
     * functioning.
     */
    mesa_bool_t admin_active;
} vtss_appl_erps_conf_t;

/**
 * \brief Get a default ERPS configuration.
 *
 * \param *conf [OUT] Default ERPS configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_conf_default_get(vtss_appl_erps_conf_t *conf);

/**
 * \brief Get ERPS instance configuration.
 *
 * \param instance [IN]  ERPS instance
 * \param conf     [OUT] Current configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_conf_get(uint32_t instance, vtss_appl_erps_conf_t *conf);

/**
 * \brief Set ERPS instance configuration.
 *
 * \param instance [IN] ERPS instance
 * \param conf     [IN] ERPS new configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_conf_set(uint32_t instance, const vtss_appl_erps_conf_t *conf);

/**
 * \brief Delete an ERPS instance.
 *
 * \param instance [IN] Instance to be deleted
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_conf_del(uint32_t instance);

/**
 * \brief Instance iterator.
 *
 * This function returns the next defined ERPS instance number. The end is
 * reached when VTSS_RC_ERROR is returned.
 * The search for an enabled instance will start with 'prev_instance' + 1.
 * If 'prev_instance' pointer is NULL, the search start with the lowest possible
 * instance number.
 *
 * \param prev_instance [IN]  Instance number
 * \param next_instance [OUT] Next instance
 *
 * \return VTSS_RC_OK as long as next is valid.
 */
mesa_rc vtss_appl_erps_itr(const uint32_t *prev_instance, uint32_t *next_instance);

/**
 * Enumeration of administrative commands (G.8032-2015, clause 8).
 * Notice that not all commands are valid in all ring modes.
 */
typedef enum {
    VTSS_APPL_ERPS_COMMAND_NR,          /**< No request (not a command, only a status)                               */
    VTSS_APPL_ERPS_COMMAND_FS_TO_PORT0, /**< Force a block of ring port1                                             */
    VTSS_APPL_ERPS_COMMAND_FS_TO_PORT1, /**< Force a block of ring port0                                             */
    VTSS_APPL_ERPS_COMMAND_MS_TO_PORT0, /**< In the absence of SF and FS, force a block of ring port1                */
    VTSS_APPL_ERPS_COMMAND_MS_TO_PORT1, /**< In the absence of SF and FS, force a block of ring port0                */
    VTSS_APPL_ERPS_COMMAND_CLEAR        /**< Clears FS, MS, WTB/WTR condition or triggers reversion if not revertive */
} vtss_appl_erps_command_t;

/**
 * This structure is used to set or get the protection group command on a
 * particular port.
 */
typedef struct {
    /**
     * See vtss_appl_erps_command_t for possible requests (commands).
     */
    vtss_appl_erps_command_t command;
} vtss_appl_erps_control_t;

/**
 * \brief Get ERPS control configuration for a particular instance.
 *
 * \param instance [IN]  ERPS instance
 * \param ctrl     [OUT] Current control configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_control_get(uint32_t instance, vtss_appl_erps_control_t *ctrl);

/**
 * \brief Set ERPS control configuration for a particular instance.
 *
 * You may go from one command directly to another (e.g. MS to FS), but if you
 * want to undo the current command, you must issue a CLEAR.
 *
 * FS and MS commands are not possible if the ERPS instance is configured to use
 * version 1 of G.8032.
 *
 * You may not use NR in the call to set(). NR can only be returned by get().
 *
 * \param instance [IN] ERPS instance
 * \param ctrl     [IN] ERPS control to be applied
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_control_set(uint32_t instance, const vtss_appl_erps_control_t *ctrl);

/**
 * The possible transmitted or received requests/states according to
 * G.8032-2015, table 10-3. The enumeration values are selected according to
 * that table.
 */
typedef enum {
    VTSS_APPL_ERPS_REQUEST_NR    =  0, /**< No Request    */
    VTSS_APPL_ERPS_REQUEST_MS    =  7, /**< Manual Switch */
    VTSS_APPL_ERPS_REQUEST_SF    = 11, /**< Signal Fail   */
    VTSS_APPL_ERPS_REQUEST_FS    = 13, /**< Forced Switch */
    VTSS_APPL_ERPS_REQUEST_EVENT = 14  /**< Event         */
} vtss_appl_erps_request_t;

/**
 * ERPS statistics
 */
typedef struct {
    uint64_t rx_error_cnt;     /**< Number of received erroneous R-APS PDUs            */
    uint64_t rx_own_cnt;       /**< Number of received R-APS PDUs with our own node ID */
    uint64_t rx_guard_cnt;     /**< Number of received R-APS PDUs during guard timer   */
    uint64_t rx_fop_pm_cnt;    /**< Number of received R-APS PDUs causing FOP-PM       */
    uint64_t rx_nr_cnt;        /**< Number of received NR R-APS PDUs                   */
    uint64_t rx_nr_rb_cnt;     /**< Number of received NR, RB R-APS PDUs               */
    uint64_t rx_sf_cnt;        /**< Number of received SF R-APS PDUs                   */
    uint64_t rx_fs_cnt;        /**< Number of received FS R-APS PDUs                   */
    uint64_t rx_ms_cnt;        /**< Number of received MS R-APS PDUs                   */
    uint64_t rx_event_cnt;     /**< Number of received Event R-APS PDUs                */

    uint64_t tx_nr_cnt;        /**< Number of transmitted NR R-APS PDUs                */
    uint64_t tx_nr_rb_cnt;     /**< Number of transmitted NR, RB R-APS PDUs            */
    uint64_t tx_sf_cnt;        /**< Number of transmitted SF R-APS PDUs                */
    uint64_t tx_fs_cnt;        /**< Number of transmitted FS R-APS PDUs                */
    uint64_t tx_ms_cnt;        /**< Number of transmitted MS R-APS PDUs                */
    uint64_t tx_event_cnt;     /**< Number of transmitted Event R-APS PDUs             */

    uint64_t sf_cnt;           /**< Number of local signal fails                       */
    uint64_t flush_cnt;        /**< Number of FDB flushes (same for both rings)        */
} vtss_appl_erps_statistics_t;

/**
 * \brief ERPS R-APS PDU information structure.
 *
 * This structure is used to contain transmitted or received R-APS information.
 * See G.8032-2015, clause 10.3 (R-APS format) for details.
 */
typedef struct {
    /**
     * Time in seconds since boot that this structure was last updated.
     * 0 if it has never been updated (e.g. if no R-APS PDU has been received on
     * the given ring port).
     */
    uint64_t update_time_secs;

    /**
     * Request/state according to G.8032, table 10-3.
     */
    vtss_appl_erps_request_t request;

    /**
     * Version of received/used R-APS Protocol.
     *
     * 0 means v1, 1 means v2, etc.
     *
     * Cannot use vtss_appl_erps_version_t because we may not receive a version
     * that fits into it.
     */
    uint8_t version;

    /**
     * RB (RPL blocked) bit of R-APS info.
     * See Figure 10-3 of G.8032.
     */
    mesa_bool_t rb;

    /**
     * DNF (Do Not Flush) bit of R-APS info.
     * See Figure 10-3 of G.8032.
     */
    mesa_bool_t dnf;

    /**
     * BPR (Blocked Port Reference) of R-APS info.
     * See Figure 10-3 of G.8032.
     */
    vtss_appl_erps_ring_port_t bpr;

    /**
     * Node ID of this request.
     * Is our own if this is our transmitted R-APS info.
     */
    mesa_mac_t node_id;

    /**
     * The Source MAC address used in the request/state (not really part of
     * R-APS info, but may be useful for debugging a ring).
     *
     * All-zeros if this structure is used as Tx Info, because we transmit with
     * different SMACs on different ring ports.
     */
    mesa_mac_t smac;
} vtss_appl_erps_raps_info_t;

/**
 * Status per ERPS port.
 */
typedef struct {
    /**
     * Specifies whether ring port is blocked or not.
     *
     * If it is blocked, all service traffic doesn't egress the port.
     * R-APS PDUs may egress the port if originated by us or if we are a
     * sub-ring with a virtual channel.
     *
     * See also G.8032, clause 9.5 and 10.1.14.
     */
    mesa_bool_t blocked;

    /**
     * Specifies the Signal Fail state of ring port after hold-off timer has
     * expired. If true, the link/MEP has signalled signal fail, otherwise
     * everything is OK.
     */
    mesa_bool_t sf;

    /**
     * Failure of Protocol - Provisioning Mismatch.
     *
     * This boolean indicates whether there are two RPL owners on the ring.
     *
     * If we are currently RPL owner and we have received a R-APS PDU on this
     * ring port with a node ID different from our own and with a request equal
     * to VTSS_APPL_ERPS_REQUEST_NR and the RB is set, this will be set to
     * true.
     *
     * It will be cleared after 17.5 seconds after we haven't seen such a PDU.
     *
     * See G.8021, Table 6-1, RAPSpm, G.8021, Table 6-2 dFOP-PM, and G.8021,
     * clause 6.1.4.3.1 and G.8021, clause 9.1.3 as well as G.8032, clause 10.4.
     *
     * Note: G.8021 and G.8032 specifies one single such alarm independent of
     * ring port, but this implementation has chosen to provide information as
     * to which ring port such a R-APS PDU is received, so it is per ring port.
     */
    mesa_bool_t cFOP_PM;

    /**
     * The last received R-APS PDU info on this ring port.
     */
    vtss_appl_erps_raps_info_t rx_raps_info;

    /**
     * Statistics. Can be cleared separately with a call to
     * vtss_appl_erps_statistics_clear().
     */
    vtss_appl_erps_statistics_t statistics;
} vtss_appl_erps_ring_port_status_t;

/**
 * The operational state of an ERPS instance.
 * There are a few ways of not having the instance active. Each of them has its
 * own enumeration. Only when the state is VTSS_APPL_ERPS_OPER_STATE_ACTIVE,
 * will the ERPS instance be active and up and running.
 * However, there may still be operational warnings that may cause the instance
 * not to run optimally. See vtss_appl_erps_oper_warnings_t for a list of
 * possible warnings.
 *
 * The reason for having operational warnings rather than just an operational
 * state is that we only want an ERPS instance to be inactive if a true error
 * has occurred, or if the user has selected it to be inactive, because
 * if one of the warnings also resulted in making the ERPS instance inactive, we
 * might get loops in our network
 */
typedef enum {
    VTSS_APPL_ERPS_OPER_STATE_ADMIN_DISABLED, /**< Instance is inactive, because it is administratively disabled. */
    VTSS_APPL_ERPS_OPER_STATE_ACTIVE,         /**< The instance is active and up and running.                     */
    VTSS_APPL_ERPS_OPER_STATE_INTERNAL_ERROR, /**< Instance is inactive, because an internal error has occurred.  */
} vtss_appl_erps_oper_state_t;

/**
 * Operational warnings of an ERPS instance.
 *
 * If the operational state is VTSS_APPL_ERPS_OPER_STATE_ACTIVE, the ERPS
 * instance is indeed active, but it may be that it doesn't run as the
 * administrator thinks, because of configuration errors, which are reflected in
 * the warnings below.
 */
typedef enum {
    VTSS_APPL_ERPS_OPER_WARNING_NONE,                                         /**< No warnings. If oper. state is VTSS_APPL_ERPS_OPER_STATE_ACTIVE, everything is fine.                        */
    VTSS_APPL_ERPS_OPER_WARNING_PORT0_NOT_MEMBER_OF_CONTROL_VLAN,             /**< Port0 ring port is not member of the control VLAN.                                                          */
    VTSS_APPL_ERPS_OPER_WARNING_PORT1_NOT_MEMBER_OF_CONTROL_VLAN,             /**< Port1 ring port is not member of the control VLAN.                                                          */
    VTSS_APPL_ERPS_OPER_WARNING_PORT0_UNTAGS_CONTROL_VLAN,                    /**< Port0's VLAN configuration causes control VLAN to become untagged on egress.                                */
    VTSS_APPL_ERPS_OPER_WARNING_PORT1_UNTAGS_CONTROL_VLAN,                    /**< Port1's VLAN configuration causes control VLAN to become untagged on egress.                                */
    VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_FOUND,                          /**< Port0 MEP is not found. Using link-state for SF instead.                                                    */
    VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_FOUND,                          /**< Port1 MEP is not found. Using link-state for SF instead.                                                    */
    VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_ADMIN_DISABLED,                     /**< Port0 MEP is administratively disabled. Using link-state for SF instead.                                    */
    VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_ADMIN_DISABLED,                     /**< Port1 MEP is administratively disabled. Using link-state for SF instead.                                    */
    VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_DOWN_MEP,                       /**< Port0 MEP is not a Down-MEP. Using link-state for SF instead.                                               */
    VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_DOWN_MEP,                       /**< Port1 MEP is not a Down-MEP. Using link-state for SF instead.                                               */
    VTSS_APPL_ERPS_OPER_WARNING_PORT0_AND_MEP_IFINDEX_DIFFER,                 /**< Port0 MEP's residence port is not that of Port0. Using link-state for SF instead.                           */
    VTSS_APPL_ERPS_OPER_WARNING_PORT1_AND_MEP_IFINDEX_DIFFER,                 /**< Port1 MEP's residence port is not that of Port1. Using link-state for SF instead.                           */
    VTSS_APPL_ERPS_OPER_WARNING_PORT_MEP_SHADOWS_PORT0_MIP,                   /**< Port0's port MEP shadows reception of R-APS PDUs                                                            */
    VTSS_APPL_ERPS_OPER_WARNING_PORT_MEP_SHADOWS_PORT1_MIP,                   /**< Port1's port MEP shadows reception of R-APS PDUs                                                            */
    VTSS_APPL_ERPS_OPER_WARNING_MEP_SHADOWS_PORT0_MIP,                        /**< A MEP on port0 that matches control-vlan shadows reception of R-APS PDUs.                                   */
    VTSS_APPL_ERPS_OPER_WARNING_MEP_SHADOWS_PORT1_MIP,                        /**< A MEP on port1 that matches control-vlan shadows reception of R-APS PDUs.                                   */
    VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_DOESNT_EXIST,                  /**< This is an interconnected sub-ring whose connected ring doesn't exist.                                      */
    VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_IS_AN_INTERCONNECTED_SUB_RING, /**< This is an interconnected sub-ring whose connected ring is also an interconnected sub-ring.                 */
    VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_IS_NOT_OPERATIVE,              /**< This is an interconnected sub-ring whose connected ring is not operatively up.                              */
    VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_INTERFACE_CONFLICT,            /**< This is an interconnected sub-ring whose connected ring shares interfaces with this one.                    */
    VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_DOESNT_PROTECT_CONTROL_VLAN,   /**< This is an interconnected sub-ring w/ virtual-channel, but connected ring doesn't protect our control VLAN. */
} vtss_appl_erps_oper_warning_t;

/**
 * Enumeration of the ring node states (G.8032-2015, table 10-2).
 */
typedef enum {
    VTSS_APPL_ERPS_NODE_STATE_INIT,       /**< (-) State machine initalization                                  */
    VTSS_APPL_ERPS_NODE_STATE_IDLE,       /**< (A) No failures detected, no commands active                     */
    VTSS_APPL_ERPS_NODE_STATE_PROTECTION, /**< (B) The ring protection is in protected state, the RPL is active */
    VTSS_APPL_ERPS_NODE_STATE_MS,         /**< (C) The ring protection is in manual switch mode                 */
    VTSS_APPL_ERPS_NODE_STATE_FS,         /**< (D) The ring protection is in forced switch mode                 */
    VTSS_APPL_ERPS_NODE_STATE_PENDING     /**< (E) The ring protection is in pending mode                       */
} vtss_appl_erps_node_state_t;

/**
 * Status per ERPS instance
 */
typedef struct {
    /**
     * Operational state of this ERPS instance.
     *
     * The ERPS instance is inactive unless oper_state is
     * VTSS_APPL_ERPS_OPER_STATE_ACTIVE.
     *
     * When inactive, non of the remaining members of this struct are valid.
     * When active, the ERPS instance may, however, still have warnings. See
     * \p warning_state for a list of possible warnings.
     */
    vtss_appl_erps_oper_state_t oper_state;

    /**
     * Operational warnings of this ERPS instance.
     *
     * The ERPS instance is error and warning free if \p oper_state is
     * VTSS_APPL_ERPS_OPER_STATE_ACTIVE and \p oper_warning is
     * VTSS_APPL_ERPS_OPER_WARNING_NONE.
     */
    vtss_appl_erps_oper_warning_t oper_warning;

    /**
     * Specifies current ERPS instance protection/node state
     */
    vtss_appl_erps_node_state_t node_state;

    /**
     * Specifies whether we are currently supposed to be transmitting R-APS PDUs
     * on our ring ports. The \p tx_raps_info member contains the R-APS info we
     * are transmitting. Notice that we transmit R-APS PDUs on both ring ports
     * whether they are blocked or not.
     */
    mesa_bool_t tx_raps_active;

    /**
     * The R-APS PDU info we are currently transmitting if \p tx_raps_active is
     * true or the R-APS PDU info we transmitted last time \p tx_raps_active was
     * true.
     *
     * We do not save EVENT requests to this structure and the 'smac' member
     * will always be all-zeros, because we use two different SMACs - one per
     * ring port, so it doesn't make sense to present it as one.
     */
    vtss_appl_erps_raps_info_t tx_raps_info;

    /**
     * Failure of Protocol - R-APS Rx Time Out.
     *
     * If no R-APS PDUs have been received by this ring instance within the last
     * 17.5 seconds, this property gets set to true unless at least we are in
     * one or both of the following two exceptions:
     *   1) Signal Fail:
     *      a) On a major ring and a sub-ring, we don't expect any R-APS PDUs if
     *         both ring ports have SF.
     *      b) On an interconnected sub-ring without virtual channel, we don't
     *         expect any R-APS PDUs if port0 has SF.
     *   2) If we are RPL owner and currently transmitting R-APS(NR, RB), we
     *      don't expect any PDUs to return to ourselves, because it might be
     *      that an RPL neighbor is configured on the ring, and that neighbor
     *      will terminate R-APS PDUs, so that they aren't forwarded onto the
     *      RPL port back to us.
     *
     * Exception 1b) above implies that on an interconnected sub-ring with a
     * virtual channel, we always expect R-APS PDUs whether or not port0 has SF,
     * unless condition 2) is true.
     *
     * Clearance of a FOP-TO occurs as soon as a valid R-APS PDU is received on
     * one of our ring ports.
     *
     * See G.8021, Table 6-2, dFOP-TO (TimeOut), G.8021, clause 6.1.4.3.4,
     * G.8021, clause 9.1.2 and G.8032, clause 10.4.
     *
     * Notice that we have bent the standard so that this property covers both
     * ring ports instead of having it per ring port.
     */
    mesa_bool_t cFOP_TO;

    /**
     * ERPS Port status
     * Index == 0 for ring port0
     * Index == 1 for ring port1
     */
    vtss_appl_erps_ring_port_status_t ring_port_status[2];
} vtss_appl_erps_status_t;

/**
 * \brief Get ERPS instance status.
 *
 * \param instance [IN]  ERPS instance
 * \param status   [OUT] ERPS status to be updated
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_status_get(uint32_t instance, vtss_appl_erps_status_t *status);

/**
 * \brief Clear ERPS instance statistics.
 *
 * \param instance [IN] ERPS instance
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_erps_statistics_clear(uint32_t instance);

#ifdef __cplusplus
}
#endif
#endif  /* _VTSS_APPL_ERPS_H_ */

