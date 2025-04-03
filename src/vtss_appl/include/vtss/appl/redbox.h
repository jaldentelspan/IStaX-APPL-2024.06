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

/**
 * \file redbox.h
 * \brief Public PRP (Parallel Redundancy Protocol) and HSR (High-availability
 * Seamless Redundancy) for Redboxes (IEC 62439-3, Edition 4.0, 2021) API.
 *
 * \details This header file describes the Redbox control functions and types.
 * The term "SV" or "SV frame" is short for "supervision frame".
 */

#ifndef _VTSS_APPL_REDBOX_H_
#define _VTSS_APPL_REDBOX_H_

#include <microchip/ethernet/switch/api.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/vlan.h>
#include <vtss/appl/module_id.h>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_BITWISE() */

/**
 * Definition of error return codes.
 * See also redbox_error_txt() in redbox.cxx.
 */
enum {
    VTSS_APPL_REDBOX_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_REDBOX), /**< Invalid parameter                                                             */
    VTSS_APPL_REDBOX_RC_NOT_SUPPORTED,                                                 /**< RedBox functionality is not supported on this platform                        */
    VTSS_APPL_REDBOX_RC_INTERNAL_ERROR,                                                /**< Internal error. Requires code update                                          */
    VTSS_APPL_REDBOX_RC_HW_RESOURCES,                                                  /**< Couldn't create RB instance. Out of H/W resources                             */
    VTSS_APPL_REDBOX_RC_NO_SUCH_INSTANCE,                                              /**< Redbox instance is not created                                                */
    VTSS_APPL_REDBOX_RC_INSTANCE_NOT_ACTIVE,                                           /**< Redbox instance in not active                                                 */
    VTSS_APPL_REDBOX_RC_NO_SUCH_NT_INSTANCE,                                           /**< No such NodesTable instance                                                   */
    VTSS_APPL_REDBOX_RC_NO_SUCH_PNT_INSTANCE,                                          /**< No such ProxyNodeTable instance                                               */
    VTSS_APPL_REDBOX_RC_INVALID_MODE,                                                  /**< Invalid redbox mode                                                           */
    VTSS_APPL_REDBOX_RC_INVALID_HSR_MODE,                                              /**< Invalid HSR mode                                                              */
    VTSS_APPL_REDBOX_RC_INVALID_NET_ID,                                                /**< Invalid NetId                                                                 */
    VTSS_APPL_REDBOX_RC_INVALID_LAN_ID,                                                /**< Invalid LanId                                                                 */
    VTSS_APPL_REDBOX_RC_INVALID_NT_AGE_TIME,                                           /**< Invalid NodesTable age time                                                   */
    VTSS_APPL_REDBOX_RC_INVALID_PNT_AGE_TIME,                                          /**< Invalid ProxyNodeTable age time                                               */
    VTSS_APPL_REDBOX_RC_INVALID_DUPLICATE_DISCARD_AGE_TIME,                            /**< Invalid duplicate discard age time                                            */
    VTSS_APPL_REDBOX_RC_INVALID_SV_VLAN,                                               /**< Invalid SV VLAN                                                               */
    VTSS_APPL_REDBOX_RC_INVALID_SV_PCP,                                                /**< Invalid SV PCP                                                                */
    VTSS_APPL_REDBOX_RC_INVALID_SV_FRAME_INTERVAL,                                     /**< Invalid supervision frame interval                                            */
    VTSS_APPL_REDBOX_RC_INVALID_PORT_A_IFINDEX,                                        /**< The ifindex of Port A is invalid                                              */
    VTSS_APPL_REDBOX_RC_INVALID_PORT_B_IFINDEX,                                        /**< The ifindex of Port B is invalid                                              */
    VTSS_APPL_REDBOX_RC_REDBOX_1_CANNOT_HAVE_A_LEFT_NEIGHBOR,                          /**< Port A on RedBox instance #1 cannot refer to a neighbor to the left           */
    VTSS_APPL_REDBOX_RC_REDBOX_N_CANNOT_HAVE_A_RIGHT_NEIGHBOR,                         /**< Port B on the highest RedBox instance cannot refer to a neighbor to the right */
    VTSS_APPL_REDBOX_RC_PORT_A_OR_PORT_B_OR_BOTH_MUST_REFER_TO_A_PORT,                 /**< Port A or Port B or both of them must refer to a port (non-leaf RedBox)       */
    VTSS_APPL_REDBOX_RC_PORT_A_NOT_VALID_FOR_REDBOX_INSTANCE,                          /**< Port A not valid for this redbox instance                                     */
    VTSS_APPL_REDBOX_RC_PORT_B_NOT_VALID_FOR_REDBOX_INSTANCE,                          /**< Port B not valid for this redbox instance                                     */
    VTSS_APPL_REDBOX_RC_PORT_A_B_IDENTICAL,                                            /**< Port A and Port B cannot be identical                                         */
    VTSS_APPL_REDBOX_RC_MODE_MUST_BE_HSR_IF_USING_REDBOX_NEIGHBOR,                     /**< If using a neigboring RedBox, the RedBox mode cannot be PRP-SAN               */
    VTSS_APPL_REDBOX_RC_LIMIT_REACHED,                                                 /**< Limit is reached                                                              */
    VTSS_APPL_REDBOX_RC_OUT_OF_MEMORY,                                                 /**< Out of memory                                                                 */
};

/*
 * About Instances
 * ---------------
 *
 * A given redbox instance supports particular ports, only.
 *
 * On LAN969x, we have the following instance table:
 *
 * +------------------------------------------------------+
 * | Instance | Chip Devices           | Port A/Port B    |
 * |          |                        | EVB-LAN9694-24CU |
 * |----------|------------------------|------------------|
 * | 1        | 1G:  1,2,3,5,6,7,28,29 | Gi  1/1-8,25     |
 * |          | 5G:  0,4               |                  |
 * | 2        | 1G:  10,11,14,15       | Gi  1/9-16       |
 * |          | 5G:  9,13              |                  |
 * |          | 10G: 8,12              |                  |
 * | 3        | 1G:  18,19,22,23       | Gi  1/17-24      |
 * |          | 5G:  17,21             |                  |
 * |          | 10G: 16,20             |                  |
 * | 4        | 10G: 24,25             | 10G 1/1-2        |
 * | 5        | 10G: 26,27             | 10G 1/3-4        |
 * +------------------------------------------------------+
 *
 * Notice that the interface names may change, as they depend on chip layout.
 *
 * To obtain a list of selectable ports for a given Redbox instance number, use
 * 'show redbox interfaces [sort-by-interfaces]' or use
 * vtss_appl_redbox_capabilities_port_list_get().
 */

/**
 * Capabilities of Redbox.
 */
typedef struct {
    /**
     * Maximum number of configurable redboxes.
     */
    uint32_t inst_cnt_max;

    /**
     * Maximum number of entries in the combined NT/PNT.
     */
    uint32_t nt_pnt_size;

    /**
     * Minimum value of nt_age_time_secs.
     */
    uint16_t nt_age_time_secs_min;

    /**
     * Maximum value of nt_age_time_secs.
     */
    uint16_t nt_age_time_secs_max;

    /**
     * Minimum value of pnt_age_time_secs.
     */
    uint16_t pnt_age_time_secs_min;

    /**
     * Maximum value of pnt_age_time_secs.
     */
    uint16_t pnt_age_time_secs_max;

    /**
     * Minimum value of duplicate_discard_age_time_msecs.
     */
    uint16_t duplicate_discard_age_time_msecs_min;

    /**
     * Maximum value of duplicate_discard_age_time_msecs.
     */
    uint16_t duplicate_discard_age_time_msecs_max;

    /**
     * Minimum value of sv_frame_interval_secs.
     */
    uint16_t sv_frame_interval_secs_min;

    /**
     * Maximum value of sv_frame_interval_secs.
     */
    uint16_t sv_frame_interval_secs_max;

    /**
     * The number of seconds between polling of statistics.
     * This is useful for the user interface in order to show the user the time
     * it may take to detect a hsr_untagged_rx or cnt_err_wrong_lan error
     * condition.
     */
    uint32_t statistics_poll_interval_secs;

    /**
     * The number of seconds after a frame error condition is no longer detected
     * until the notification/alarm disappears.
     *
     * A frame error condition includes reception of an HSR untagged frame on
     * Port A, B, or C when in HSR mode (HSR-HSR for Port C) or reception of a
     * frame with wrong RCT.LanId on Port A, B in PRP-SAN mode or Port C in
     * HSR-PRP mode.
     */
    uint32_t alarm_raised_time_secs;
} vtss_appl_redbox_capabilities_t;

/**
 * \brief Get Redbox capabilities.
 *
 * \param cap [OUT] Redbox capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_capabilities_get(vtss_appl_redbox_capabilities_t *cap);

/**
 * Indicates whether RedBox functionality is supported on this platform.
 * This is a shorthand for vtss_appl_redbox_capabilities_t::inst_cnt_max > 0.
 *
 * \return true if RedBox functionality is supported, false if not.
 */
mesa_bool_t vtss_appl_redbox_supported(void);

/**
 * Get a list of valid ports for a given redbox instance number.
 *
 * \param inst      [IN]  Redbox instance
 * \param port_list [OUT] Port list
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_capabilities_port_list_get(uint32_t inst, mesa_port_list_t *port_list);

/**
 * Enumeration of the four different Redbox modes.
 */
typedef enum {
    VTSS_APPL_REDBOX_MODE_PRP_SAN, /**< PRP-SAN mode */
    VTSS_APPL_REDBOX_MODE_HSR_SAN, /**< HSR-SAN mode */
    VTSS_APPL_REDBOX_MODE_HSR_PRP, /**< HSR-PRP mode */
    VTSS_APPL_REDBOX_MODE_HSR_HSR, /**< HSR-HSR mode */
} vtss_appl_redbox_mode_t;

/**
 * The HSR modes are specified in clause 5.3.2.1.
 *
 * The enumeration is in accordance with the MIB's lsrHsrLREMode, except MODE_R,
 * which is not part of the MIB.
 *
 * Mode X is not supported.

 * Mode U can be supported, but isn't, because the Cisco box doesn't support it
 * (see also UNG_LAGUNA-367).
 */
typedef enum {
    VTSS_APPL_REDBOX_HSR_MODE_H = 1, /**< HSR LRE bridges HSR-tagged traffic                                                                                                                              */
    VTSS_APPL_REDBOX_HSR_MODE_N = 2, /**< No forwarding. Bridging between HSR ports is disabled                                                                                                           */
    VTSS_APPL_REDBOX_HSR_MODE_T = 3, /**< Transparent forwarding. Bridging occurs without HSR tag                                                                                                         */
    VTSS_APPL_REDBOX_HSR_MODE_U = 4, /**< Unicast forwarding. As Mode H, but also forwards traffic to other HSR port for which this is the unique destination - execpt for traffic destined to local host */
    VTSS_APPL_REDBOX_HSR_MODE_M = 5, /**< HSR LRE is configured in mixed mode (HSR-tagged frames forwarded as in MODE_H, HSR-untagged frames forwarded according to 802.1Q)                               */
    VTSS_APPL_REDBOX_HSR_MODE_R = 6, /**< BPDUs are encapsulated/decapsulated and forwarded to avoid HSR ring short-circuits an STP topology                                                              */
} vtss_appl_redbox_hsr_mode_t;

/**
 * Identifies the PRP LAN as LAN A or LAN B
 */
typedef enum {
    VTSS_APPL_REDBOX_LAN_ID_A, /**< LAN A */
    VTSS_APPL_REDBOX_LAN_ID_B, /**< LAN B */
} vtss_appl_redbox_lan_id_t;

/**
 * Configuration of one Redbox instance.
 */
typedef struct {
    /**
     * Specifies the mode of this Redbox.
     *
     * Partly corresponds to lreNodeType and lreSwitchingEndNode.
     *
     * Default: VTSS_APPL_REDBOX_MODE_PRP_SAN
     */
    vtss_appl_redbox_mode_t mode;

    /**
     * Interface index of Port A or VTSS_IFINDEX_REDBOX_NEIGHBOR
     *
     * When the interface index represents a port:
     *    The corresponding switch port becomes the interlink port.
     *    Notice that there are restrictions on which ports can be selected as
     *    \p port_a for a given RedBox instance (see "About Instances").
     *
     * When the interface index is VTSS_IFINDEX_REDBOX_NEIGHBOR:
     *    The RedBox is connected internally to the RedBox to the left of this
     *    RedBox. RedBox instance #1 doesn't have a RedBox to the left of it, so
     *    this is an illegal configuration.
     *
     * It is illegal to set both \p port_a and \p port_b to
     * VTSS_IFINDEX_REDBOX_NEIGHBOR.
     */
    vtss_ifindex_t port_a;

    /**
     * Interface index of Port B or VTSS_IFINDEX_REDBOX_NEIGHBOR
     *
     * When the interface index represents a port:
     *    If \p port_a represents a port, the corresponding switch port for \p
     *    port_b becomes unused. Otherwise (\p port_a is
     *    VTSS_IFINDEX_REDBOX_NEIGHBOR), the corresponding switch port for \p
     *    port_b becomes the interlink port.
     *    Notice that there are restrictions on which ports can be selected as
     *    \b port_b for a given RedBox instance (see "About Instances").
     *
     * When the interface index is VTSS_IFINDEX_REDBOX_NEIGHBOR:
     *    The RedBox is connected internally to the RedBox to the right of this
     *    RedBox. RedBox instance number N doesn't have a RedBox to the right of
     *    it, so this is an illegal configuration.
     *
     * It is illegal to set both \p port_a and \p port_b to
     * VTSS_IFINDEX_REDBOX_NEIGHBOR.
     */
    vtss_ifindex_t port_b;

    /**
     * Specifies the HSR mode of this RedBox.
     *
     * Only used if \p mode is NOT VTSS_APPL_REDBOX_MODE_PRP_SAN.
     *
     * RBNTBD: Only mode H is supported by MESA.
     *
     * Default: VTSS_APPL_REDBOX_HSR_MODE_H.
     */
    vtss_appl_redbox_hsr_mode_t hsr_mode;

    /**
     * If true, the duplicate discard algorithm is active at reception. If
     * false, it is not.
     *
     * For PRP frames, this controls TLV1.type in SV frames.
     *
     * Corresponds to lreDuplicateDiscard.
     *
     * RBNTBD: Not supported by MESA.
     *
     * Default: true.
     */
    mesa_bool_t duplicate_discard;

    /**
     * This identifier is used in VTSS_APPL_REDBOX_MODE_HSR_PRP and
     * VTSS_APPL_REDBOX_MODE_HSR_HSR, only.
     *
     * In HSR-PRP mode, it is used to filter frames from the HSR ring towards
     * the PRP network. It must be identical, yet unique for the two redboxes
     * connecting to the PRP network's LAN A and LAN B.
     *
     * In HSR-HSR mode, it is used to filter frames arriving on LRE ports. It
     * will not filter frames arriving from the switch core side.
     * In a QuadBox setup, the two HSR-HSR redboxes making up the QuadBox must
     * be configured with different NetIds.
     *
     * Corresponds to NetId and partly lreRedBoxIdentity.
     *
     * Valid values: [1; 7].
     *
     * Default: 1.
     */
    uint8_t net_id;

    /**
     * This identifier is used in VTSS_APPL_REDBOX_MODE_HSR_PRP, only.
     * It is used to filter frames from the PRP network towwards the HSR ring,
     * and to insert correct RCT when forwarding frames from the HSR ring
     * towards the PRP network.
     * It must be VTSS_APPL_REDBOX_LAN_ID_A for the redbox connecting to LAN A
     * and VTSS_APPL_REDBOX_LAN_ID_B for the redbox connecting to LAN B.
     *
     * Corresponds to LanId and partly lreRedBoxIdentity.
     *
     * Default: VTSS_APPL_REDBOX_LAN_ID_A.
     */
    vtss_appl_redbox_lan_id_t lan_id;

    /**
     * NodesTable age time measured in seconds.
     *
     * Number of seconds without activity before a remote node is removed from
     * the NodesTable.
     *
     * Corresponds to NodeForgetTime.
     *
     * Default: 60 seconds.
     *
     * Valid values: [1; 65] seconds.
     */
    uint16_t nt_age_time_secs;

    /**
     * ProxyNodeTable age time measured in seconds. The ProxyNodeTable is not
     * used in VTSS_APPL_REDBOX_MODE_HSR_HSR.
     *
     * Number of seconds without activity before a proxy node is removed from
     * the ProxyNodeTable (default is 60 seconds).
     *
     * Corresponds to ProxyNodeTableForgetTime.
     *
     * Default: 60 seconds.
     *
     * Valid values: [1; 65] seconds.
     */
    uint16_t pnt_age_time_secs;

    /**
      * Duplicate discard age time measured in milliseconds.
      *
      * Corresponds to lreDupListResideMaxTime and EntryForgetTime.
      *
      * Default: 40 milliseconds (matches 1 Gbps links).
      *
      * Valid values: [10; 10000] milliseconds (RBNTBD)
      */
    uint16_t duplicate_discard_age_time_msecs;

    /**
     * The VLAN on which supervision PDUs are transmitted on Port A and B.
     *
     * PDUs using the native VLAN (PVID) *can* have \p vlan set to 0 as an
     * alternative to using the actual value for the native VLAN.
     *
     * Default: 0 (use native VLAN (PVID))
     */
    mesa_vid_t sv_vlan;

    /**
     * The PCP value used in the VLAN tag of the supervision PDUs.
     *
     * Valid range is [0; 7], with 7 as default.
     *
     * Only used if \p vlan is non-zero.
     */
    mesa_pcp_t sv_pcp;

    /**
     * Least Significant Byte of destination MAC address used in PRP/HSR
     * supervision frames.
     * This is only used for SV frames generated by this RedBox. The RedBox
     * accepts all received SV frames where the first five bytes of the DMAC are
     * correct, and therefore doesn't care about the least significant byte.
     *
     * Defaults to 0.
     */
    uint8_t sv_dmac_lsb;

    /**
     * Interval (in seconds) between transmission of supervision frames.
     *
     * Corresponds to LifeCheckInterval.
     *
     * Default: 2 seconds.
     *
     * Valid values [1; 10] seconds.
     */
    uint16_t sv_frame_interval_secs;

    /**
     * Enable proxy-translation of supervision frames from PRP network to HSR
     * ring.
     *
     * This is only used if mode is VTSS_APPL_REDBOX_MODE_HSR_PRP.
     *
     * Corresponds to lreProxyTranslationPrpToHsrEnabled
     *
     * Default: true.
     */
    mesa_bool_t sv_xlat_prp_to_hsr;

    /**
     * Enable proxy-translation of supervision frames from HSR ring to PRP
     * network.
     *
     * This is only used if mode is VTSS_APPL_REDBOX_MODE_HSR_PRP.
     *
     * Corresponds to lreProxyTranslationHsrToPrpEnabled
     *
     * Default: true.
     */
    mesa_bool_t sv_xlat_hsr_to_prp;

    /**
     * The administrative state of this Redbox instance (default false).
     * Set to true to make it function normally and false to make it cease
     * functioning (in which case all H/W entries are removed).
     */
    mesa_bool_t admin_active;
} vtss_appl_redbox_conf_t;

/**
 * \brief Get a default Redbox configuration.
 *
 * \param *conf [OUT] Default Redbox configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_conf_default_get(vtss_appl_redbox_conf_t *conf);

/**
 * \brief Get Redbox instance configuration.
 *
 * See "About Instances" above for a description of valid Port A and Port B
 * interfaces for a given instance.
 *
 * \param instance [IN]  Redbox instance
 * \param conf     [OUT] Current configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_conf_get(uint32_t instance, vtss_appl_redbox_conf_t *conf);

/**
 * \brief Set Redbox instance configuration.
 *
 * \param instance [IN] Redbox instance
 * \param conf     [IN] Redbox new configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_conf_set(uint32_t instance, const vtss_appl_redbox_conf_t *conf);

/**
 * \brief Delete a redbox instance.
 *
 * \param instance [IN] Instance to be deleted
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_conf_del(uint32_t instance);

/**
 * \brief Instance iterator.
 *
 * This function returns the next defined Redbox instance number. The end is
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
mesa_rc vtss_appl_redbox_itr(const uint32_t *prev_instance, uint32_t *next_instance);

/**
 * Enumeration of Port-A, Port-B, and Port-C
 */
typedef enum {
    VTSS_APPL_REDBOX_PORT_TYPE_A, /**< Port A             */
    VTSS_APPL_REDBOX_PORT_TYPE_B, /**< Port B             */
    VTSS_APPL_REDBOX_PORT_TYPE_C  /**< Port C (interlink) */
} vtss_appl_redbox_port_type_t;

/**
 * Notifications that may be generated per LRE port by a Redbox instance.
 */
typedef struct {
    /**
     * Raised when link on LRE port is down.
     *
     * Not used by Port C, since the I/L port cannot be down.
     */
    mesa_bool_t down;

    /**
     * Raised for vtss_appl_redbox_capabilities_t::alarm_raised_time_secs
     * seconds if lreCntErrWrongLanA or lreCntErrWrongLanB changes in PRP-SAN
     * mode or if lreCntErrWrongLanC changes in HSR-PRP mode.
     *
     * Notice: It may take up to 10 seconds to be discovered that this counter
     * has changed.
     */
    mesa_bool_t cnt_err_wrong_lan;

    /**
     * Raised for vtss_appl_redbox_capabilities_t::alarm_raised_time_secs
     * seconds whenever an HSR-untagged frame is received on an LRE port in an
     * HSR ring or an HSR-untagged frame is received on the I/L port in
     * HSR-HSR mode.
     *
     * Whenever this flag is raised, we know that it's the immediate connection
     * to the LRE port in question that is the sinner, because non-HSR-tagged
     * frames are never forwarded by DANHs and RedBoxes on the ring.
     */
    mesa_bool_t hsr_untagged_rx;
} vtss_appl_redbox_port_notification_status_t;

/**
 * Notifications that may be generated by a Redbox instance.
 */
typedef struct {
    /**
     * Nodes Table or Proxy Node Table is full.
     * The NT/PNT full condition is checked every 10 seconds and whenever the
     * CPU adds an entry to the PNT table, and clears or polls the NT/PNT
     * table.
     */
    mesa_bool_t nt_pnt_full;

    /**
     * Per RedBox port notification status.
     * Index 0 == VTSS_APPL_REDBOX_PORT_TYPE_A, LRE-A
     * Index 1 == VTSS_APPL_REDBOX_PORT_TYPE_B, LRE-B
     * Index 2 == VTSS_APPL_REDBOX_PORT_TYPE_C, LRE-C
     */
    vtss_appl_redbox_port_notification_status_t port[3];
} vtss_appl_redbox_notification_status_t;

/**
 * Get the notification status of a redbox instance.
 *
 * \param instance     [IN]  Redbox instance.
 * \param notif_status [OUT] Pointer to structure receiving Redbox instance's notification status.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_notification_status_get(uint32_t instance, vtss_appl_redbox_notification_status_t *const notif_status);

/**
 * The operational state of a RedBox instance.
 * There are a few ways of not having the instance active. Each of them has its
 * own enumeration. Only when the state is VTSS_APPL_REDBOX_OPER_STATE_ACTIVE,
 * will the RedBox instance be active and up and running.
 * However, there may still be operational warnings that may cause the instance
 * not to run optimally. See vtss_appl_redbox_oper_warnings_t for a list of
 * possible warnings. These operational warnings are due to configuration
 * errors.
 *
 * Besides the operational warnings, the RedBox can issue notifications whenever
 * some kind of unexpected condition occurs within the RedBox. See
 * vtss_appl_redbox_notification_status_t for a list of these conditions.
 *
 * The reason for having operational warnings and notifications rather than just
 * one operationalstate is that we only want a RedBox instance to be inactive if
 * a true error has occurred, or if the user has selected it to be inactive,
 * because if one of the warnings also resulted in making the RedBox instance
 * inactive, we might get loops in our network.
 */
typedef enum {
    VTSS_APPL_REDBOX_OPER_STATE_ADMIN_DISABLED, /**< Instance is inactive, because it is administratively disabled. */
    VTSS_APPL_REDBOX_OPER_STATE_ACTIVE,         /**< The instance is active and up and running.                     */
    VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR, /**< Instance is inactive, because an internal error has occurred.  */
} vtss_appl_redbox_oper_state_t;

/**
 * Bitmask of operational warnings of a RedBox instance.
 */
typedef enum {
    VTSS_APPL_REDBOX_OPER_WARNING_NONE                                       = 0x000, /**< No warnings                                                                                      */
    VTSS_APPL_REDBOX_OPER_WARNING_MTU_TOO_HIGH_LRE_PORTS                     = 0x001, /**< The MTU is too high to fit in an HSR/PRP-encapsulated frame on LRE ports                         */
    VTSS_APPL_REDBOX_OPER_WARNING_MTU_TOO_HIGH_NON_LRE_PORTS                 = 0x002, /**< The MTU is too high to fit in an HSR/PRP-encapsulated frame on non-LRE ports                     */
    VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_C_TAGGED                     = 0x004, /**< Current S/W implementation only supports untagged or C-tagged operation on interlink port        */
    VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_MEMBER_OF_VLAN               = 0x008, /**< The interlink is not a member of the configured VLAN                                             */
    VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_NOT_CONFIGURED             = 0x010, /**< The neighbor RedBox is not configured                                                            */
    VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_NOT_ACTIVE                 = 0x020, /**< The neighbor RedBox is not active                                                                */
    VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_PORT_A_NOT_SET_TO_NEIGHBOR = 0x040, /**< The neighbor's port A is not configured as a RedBox neighbor                                     */
    VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_PORT_B_NOT_SET_TO_NEIGHBOR = 0x080, /**< The neighbor's port B is not configured as a RedBox neighbor                                     */
    VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_OVERLAPPING_VLANS          = 0x100, /**< The neighbor's interlink port has coinciding VLAN memberships with this RedBox's interlink port  */
    VTSS_APPL_REDBOX_OPER_WARNING_STP_ENABLED_INTERLINK                      = 0x200, /**< Spanning tree is enabled on the interlink port                                                   */
} vtss_appl_redbox_oper_warnings_t;

/**
 * Operators for vtss_appl_redbox_oper_warnings_t flags.
 */
VTSS_ENUM_BITWISE(vtss_appl_redbox_oper_warnings_t);

/**
 * Status per Redbox instance.
 *
 * RBNTBD: lreCntNodes/lreCntProxyNodes
 */
typedef struct {
    /**
     * Operational state of this RedBox instance.
     *
     * The RedBox instance is inactive unless oper_state is
     * VTSS_APPL_REDBOX_OPER_STATE_ACTIVE.
     *
     * When inactive, non of the remaining members of this struct are valid.
     * When active, the RedBox instance may, however, still have warnings. See
     * \p warning_state for a bitmask of possible warnings.
     */
    vtss_appl_redbox_oper_state_t oper_state;

    /**
     * The interlink port's (Port C) ifindex.
     */
    vtss_ifindex_t port_c;

    /**
     * Status that also can generate SNMP traps and JSON notifications.
     */
    vtss_appl_redbox_notification_status_t notif_status;

    /**
     * Operational warnings of this Redbox instance.
     */
    vtss_appl_redbox_oper_warnings_t oper_warnings;
} vtss_appl_redbox_status_t;

/**
 * Get Redbox instance status.
 *
 * \param instance [IN]  Redbox instance
 * \param status   [OUT] Redbox status to be updated
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_status_get(uint32_t instance, vtss_appl_redbox_status_t *status);

/**
 * NodesTable status.
 */
typedef struct {
    /**
     * Number of MAC addresses in the NodesTable.
     */
    uint32_t mac_cnt;

    /**
     * If true, at least one of the MAC addresses in the NodesTable has a
     * non-zero Rx Wrong LAN count.
     *
     * Only relevant in PRP-SAN mode.
     */
    mesa_bool_t wrong_lan;
} vtss_appl_redbox_nt_status_t;

/**
 * Get status of the NodesTable.
 *
 * \param instance [IN]  Redbox instance
 * \param status   [OUT] NodesTable status
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_nt_status_get(uint32_t instance, vtss_appl_redbox_nt_status_t *status);

/**
 * Node type.
 * Enumeration fits lreRemNodeType.
 *
 * Notice that the lreRemNodeType does not include a SAN type. A SAN type is
 * needed in PRP-SAN mode, where no SV frames have been received relating to a
 * particular MAC address, but the node type has an entry that is reported as a
 * SAN (frames not received on both LRE ports).
 */
typedef enum {
    VTSS_APPL_REDBOX_NODE_TYPE_DANP,     /**< DANP node        */
    VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB,  /**< RedBox DANP node */
    VTSS_APPL_REDBOX_NODE_TYPE_VDANP,    /**< VDANP node       */
    VTSS_APPL_REDBOX_NODE_TYPE_DANH,     /**< DANH node        */
    VTSS_APPL_REDBOX_NODE_TYPE_DANH_RB,  /**< RedBox DANH node */
    VTSS_APPL_REDBOX_NODE_TYPE_VDANH,    /**< VDANH node       */
    VTSS_APPL_REDBOX_NODE_TYPE_SAN,      /**< SAN node         */
} vtss_appl_redbox_node_type_t;

/**
 * Supervision frame's TLV1's type.
 */
typedef enum {
    VTSS_APPL_REDBOX_SV_TYPE_PRP_DD, /**< Number of PRP SV frames of type Duplicate-Discard     */
    VTSS_APPL_REDBOX_SV_TYPE_PRP_DA, /**< Number of PRP SV frames of type Duplicate-Accept      */
    VTSS_APPL_REDBOX_SV_TYPE_HSR,    /**< Number of HSR SV frames                               */
    VTSS_APPL_REDBOX_SV_TYPE_CNT,    /**< Number of entries in this enumeration. Must come last */
} vtss_appl_redbox_sv_type_t;

/**
 * Port A/B/C information in NodesTable/ProxyNodeTable
 */
typedef struct {
    /**
     * If Port A/B: Number of frames received on this LRE port (including \p
     * rx_wrong_lan_cnt)
     *
     * If Port C: Number of frames received by the RedBox from the interlink
     * port and possibly forwarded to LRE ports.
     */
    uint32_t rx_cnt;

    /**
     * The time (in seconds) since this MAC address was last seen on this
     * LRE or interlink port.
     *
     * This field is only valid if \p rx_cnt is non-zero. So if \p rx_cnt is
     * non-zero and \p last_seen_secs is 0, it's 0 seconds ago a data frame was
     * last seen.
     *
     * Note: This is a rough estimate. The H/W only has eight levels of "last
     * seen", so if the age time is e.g. 60 seconds, the last seen as reported
     * by H/W is 0, 7, 15, 22, 30, 37, 45, 52, where e.g. 0 means "somewhere
     * between 0 and 7 seconds ago".
     *
     * S/W attempts to tune this, to a better granularity from the first time
     * S/W saw this entry and what it's age was.
     */
    uint32_t last_seen_secs;

    /**
     * Number of valid SV frames received on this port related to this MAC
     * address.
     *
     * If port A/B: Relevant in all modes.
     * If port C: Only relevant in HSR-PRP mode.
     */
    uint32_t sv_rx_cnt;

    /**
     * The time (in seconds) since a SV frame on behalf of this MAC address was
     * last seen on this port.
     *
     * This field is only valid if \p sv_rx_cnt is non-zero. So if \p sv_rx_cnt
     * is non-zero and \p sv_last_seen_secs is 0, it's 0 seconds ago a SV frame
     * was last seen.
     *
     * Port A/B: If it is more than nt_age_time_secs ago a SV frame was received
     * for this MAC address, and no data frames have been received within that
     * amount of time, the entry ages out.
     *
     * Port C: If no data frames have arrived from this MAC address, but only SV
     * frames, the entry times out (unless it's locked) after pnt_age_time_secs
     * after the last SV frame was received.
     *
     * Port A/B: Relevant in all modes
     * Port C: Only relevant in HSR-PRP mode.
     */
    uint32_t sv_last_seen_secs;

    /**
     * If \p sv_rx_cnt is non-zero, this one indicates the SV frame type
     * received in the last SV frame received on this port related to this MAC
     * address.
     */
    vtss_appl_redbox_sv_type_t sv_last_type;

    /**
     * Number of frames with wrong LanId received on this port.
     *
     * Port A/B:
     *   Relevant in PRP-SAN mode, only.
     *   Counts if a DANP's RCT indicates LAN-B but is received on Port A and
     *   vice versa.
     *
     * Port C:
     *   Relevant in HSR-PRP mode, only.
     *   Counts if a DANP's RCT indicates LAN-B but is received on a RedBox
     *   configured for LanId A and vice versa.
     */
    uint32_t rx_wrong_lan_cnt;

    /**
     * Indicates whether the NodesTable indicates forwarding of frames from
     * switch core to this LRE port. If the entry is marked as a DAN, it will
     * transmit frames to both ports, otherwise (it's marked as a SAN) only to
     * the port where \p fwd is true.
     *
     * Relevant in PRP-SAN mode, only.
     *
     * Not used if Port C.
     */
    mesa_bool_t fwd;
} vtss_appl_redbox_mac_port_status_t;

/**
 * Contents of one NodesTable entry.
 *
 * A MAC address in the NodesTable can be kept alive by
 * 1) receiving traffic on at least one of the LRE ports with this MAC address
 *    as SMAC in the frames, or
 * 2) reception of (valid) SV frames on LRE ports, where this entry's MAC is
 *    represented in either TLV1 or TLV2.
 *
 * An entry will disappear from the NodesTable if no data has been received
 * within the NodeTable's age time (vtss_appl_redbox_conf_t::nt_age_time_secs)
 * and if no SV frames have been received on behalf of that node for the same
 * amount of time.
 */
typedef struct {
    /**
     * Type of NodesTable node.
     *
     * Basically, the H/W NodesTable has two node types, DAN and SAN.
     *
     * H/W learns new nodes based on traffic received on Port A and Port B.
     * By default, it marks a frame as a SAN, but it can auto-detect a DAN if
     * the same frame is received on both Port A and Port B with identical
     * sequence number and PathId (in PRP-SAN mode, it's the LanId that must be
     * correct for the two ports).
     *
     * S/W may refine what it finds in the NT by the reception of SV frames.
     * The following table outlines the node types shown in the NodesTable given
     * whether or not a SV frame is received, the H/W's perception of SAN/DAN
     * and the configured mode.
     *
     * =--------------------------------------------------------------=
     * | SV frame received      | H/W  || Mode              | Notes   |
     * |                        | type || PRP-SAN | HSR-xxx |         |
     * |------------------------|------||---------|---------|---------|
     * | No                     | SAN  || SAN     | DANH    |         |
     * | No                     | DAN  || DANP    | DANH    |         |
     * | TLV1, TLV2 not present | DAN  || DANP    | DANH    | 1, 2    |
     * | TLV1, TLV2 present     | DAN  || VDANP   | VDANH   | 1, 2, 3 |
     * | TLV2                   | DAN  || DANP-RB | DANH-RB | 1, 2    |
     * =--------------------------------------------------------------=
     *
     * 1) If a SV frame is received with a TLV2 at any point in time since this
     *    MAC address was added to the NT, that MAC will continue being
     *    displayed as a DANx-RB, whether the same MAC appears in a TLV1 later
     *    on. The reason for this is that a RedBox may send both proxy SV
     *    frames, in which case the RedBox' MAC address appears in TLV2, and it
     *    may send its own DANx SV frames, in which case the RedBox' MAC address
     *    appears in TLV1, and there's no TLV2. So to avoid shifting the type
     *    back and forth, we make the RB type sticky.
     *
     *  2) The H/W type is set by S/W to DAN.
     *
     *  3) It is impossible to detect the real origin of a given VDAN node.
     *     As stated in the table, the IStaX software names the nodes in the
     *     NodesTable after the RedBox's mode (PRP-SAN or any HSR mode).
     *     However, a given VDANx node may not really be of the specified type
     *     (VDANP or VDANH). As an example, consider an HSR ring, where we are
     *     attached with a RedBox in any HSR mode. Suppose another RedBox in
     *     HSR-PRP mode is connected to the same ring. Nodes connected behind
     *     that other RedBox are really VDANPs (unless yet another HSR-PRP
     *     RedBox connects another HSR ring) and not VDANHs as stated in the
     *     table. A similar argument can be made for a RedBox in PRP-SAN mode.
     */
    vtss_appl_redbox_node_type_t node_type;

    /**
     * Port A and Port B info.
     *
     * These two structures may be indexed by vtss_appl_redbox_port_type_t.
     */
    vtss_appl_redbox_mac_port_status_t port[2];
} vtss_appl_redbox_nt_mac_status_t;

/**
 * Iterate through entries in the NodesTable.
 *
 * One can interate over all entries as follows:
 *    uint32_t   inst = 0;
 *    mesa_mac_t mac = {};
 *    while (vtss_appl_redbox_nt_itr(&inst, &inst, &mac, &mac) == VTSS_RC_OK) {
 *        // Do something.
 *    }
 *
 * \param prev_inst [IN]  Pointer to the instance to find the next MAC address for or NULL to start over.
 * \param next_inst [OUT] The next instance (or the same if another MAC).
 * \param prev_mac [IN]   Pointer to the MAC to find the next for. NULL (or contents all-zeros) to start over.
 * \param next_mac [OUT]  The next MAC address in the NodesTable.
 *
 * \return VTSS_RC_OK if operation succeeds. VTSS_RC_ERROR if no more entries.
 */
mesa_rc vtss_appl_redbox_nt_itr(const uint32_t *prev_inst, uint32_t *next_inst, const mesa_mac_t *prev_mac, mesa_mac_t *next_mac);

/**
 * Get status of a NodesTable entry.
 *
 * \param instance [IN]  Redbox instance
 * \param mac      [IN]  MAC address to get status for
 * \param status   [OUT] Node status to be updated
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_nt_mac_status_get(uint32_t instance, const mesa_mac_t *mac, vtss_appl_redbox_nt_mac_status_t *status);

/**
 * Clear the NodesTable.
 *
 * Only non-locked entries are cleared.
 *
 * Corresponds to lreNodesTableClear.
 *
 * \param instance [IN] Redbox instance
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_nt_clear(uint32_t instance);

/**
 * ProxyNodeTable status.
 */
typedef struct {
    /**
     * Number of MAC addresses in the ProxyNodeTable.
     */
    uint32_t mac_cnt;

    /**
     * If true, at least one of the MAC addresses in the ProxyNodeTable has a
     * non-zero Rx Wrong LAN count.
     *
     * Only relevant in HSR-PRP mode.
     */
    mesa_bool_t wrong_lan;
} vtss_appl_redbox_pnt_status_t;

/**
 * Get status of the ProxyNodeTable.
 *
 * \param instance [IN]  Redbox instance
 * \param status   [OUT] ProxyNodeTable status
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_pnt_status_get(uint32_t instance, vtss_appl_redbox_pnt_status_t *status);

/**
 * Contents of one ProxyNodeTable entry.
 *
 * A MAC address in the ProxyNodeTable (PNT) can be kept alive by
 * 1) receiving traffic from the switch core side towards the RedBox for this
 *    MAC address as SMAC in the frames, or
 * 2) reception of (valid) SV frames from the switch core side towards the
 *    RedBox (HSR-PRP mode only), where the entry's AMC is represented in either
 *    TLV1 or TLV2.
 *
 * An entry will disappear from the PNT if no data has been received withint the
 * ProxyNodeTable's age time (vtss_appl_redbox_conf_t::pnt_age_time_secs) and if
 * no SV frames have been received on behlaf of that node for the same amount of
 * time.
 */
typedef struct {
    /**
     * The type of node.
     *
     * As for the NT, the PNT has two node types, DAN and SAN.
     * These node types aren't used by H/W except in HSR-PRP mode, where it is
     * used to indicate whether frames from the PRP network are expected to have
     * an RCT trailer appended to the frame. If not, it is marked as a SAN,
     * otherwise it is marked as a DAN. When marked as a DAN and frames from the
     * PRP network don't include the RCT, they will be discarded by the RedBox
     * and not be forwarded to the HSR ring.
     *
     * Whenever a frame with an unknown SMAC arrives from the PRP network, H/W
     * automatically adds it to the PNT as a SAN. H/W will never change the type
     * from DAN to SAN or vice versa.
     *
     * Upon activation of the RedBox, the PNT gets populated with two locked
     * entries that will never age out. These are:
     *   R0: The switch's management MAC address (global for the entire switch).
     *   R1: The RedBox' own MAC address (corresponds to the MAC address of the
     *       RedBox' interlink port).
     *
     * Only in HSR-PRP mode, will S/W listen to SV frames from the PRP network
     * and potentially mark the entry as a DAN in H/W. Reception of such SV
     * frames may refine the node type shown in the PNT.
     *
     * The following table outlines the node types shown in the PNT given
     * whether or not a SV frame is received and the configured mode.
     *
     * =-------------------------------------------------------------------------------------------------=
     * | SV frame received      | Locked | H/W  || Mode                                  | Notes         |
     * |                        |        | type || PRP-SAN | HSR-SAN | HSR-PRP | HSR-HSR |               |
     * |------------------------|--------|------||---------|---------|---------|---------|---------------|
     * | No                     | Yes    | SAN  || VDANP   | VDANH   | VDANP   | VDANH   | 1             |
     * | No                     | Yes    | DAN  || DANP-RB | DANH-RB | DANH-RB | DANH-RB | 2             |
     * | No                     | No     | SAN  || VDANP   | VDANH   | VDANP   | N/A     | 3             |
     * | TLV1, TLV2 not present | No     | DAN  || N/A     | N/A     | DANP    | N/A     | 3, 4, 5, 6, 7 |
     * | TLV1, TLV2 present     | No     | DAN  || N/A     | N/A     | VDANP   | N/A     | 3, 4, 5, 6, 7 |
     * | TLV2                   | No     | DAN  || N/A     | N/A     | DANP-RB | N/A     | 3, 5, 6, 7    |
     * =-------------------------------------------------------------------------------------------------=
     *
     * 1) This is the switch's management MAC address, R0.
     *
     * 2) This is the RedBox' own MAC address, R1.
     *
     * 3) The PNT is not used in HSR-HSR mode, except for our own two MAC
     *    addresses.
     *
     * 4) In HSR-PRP mode, it is impossible to detect the real origin of a given
     *    DANP or VDANP node. As an example, consider a PRP network, where
     *    another RedBox in HSR-PRP mode translates HSR supervision frames
     *    arriving on its LRE ports to PRP supervision frames before sending
     *    them to the PRP network. When this supervision frame arrives at our
     *    RedBox, it looks like any other supervision frame transmitted by e.g.
     *    a PRP-SAN RedBox connected to the PRP network.
     *
     * 5) SV frames from bridge-side of the RedBox are only used in HSR-PRP
     *    mode.
     *
     * 6) S/W changes the H/W type to a DAN.
     *
     * 7) See note 1 from \p vtss_appl_redbox_nt_mac_status_t::node_type above.
     *
     * Also notice that in HSR-PRP mode, we use the PRP-side's nomenclature
     * rather than the HSR-side's. This is because S/W knows more about the PRP
     * side based on reception of PRP SV frames than it knows about the HSR
     * side.
     */
    vtss_appl_redbox_node_type_t node_type;

    /**
     * Port C info.
     */
    vtss_appl_redbox_mac_port_status_t port;

    /**
     * Number of proxied supervision frames transmitted for this SAN.
     */
    uint32_t sv_tx_cnt;

    /**
     * Indicates whether this entry is locked (true) or not (false) in the
     * ProxyNodeTable. Only entries added by the RedBox itself are locked (R0
     * and R1 in the description of \p node_type above).
     */
    mesa_bool_t locked;
} vtss_appl_redbox_pnt_mac_status_t;

/**
 * Iterate through entries in the ProxyNodeTable.
 *
 * One can interate over all entries as follows:
 *    uint32_t   inst = 0;
 *    mesa_mac_t mac = {};
 *    while (vtss_appl_redbox_pnt_itr(&inst, &inst, &mac, &mac) == VTSS_RC_OK) {
 *        // Do something.
 *    }
 *
 * \param prev_inst [IN]  Pointer to the instance to find the next MAC address for or NULL to start over.
 * \param next_inst [OUT] The next instance (or the same if another MAC).
 * \param prev_mac [IN]   Pointer to the MAC to find the next for. NULL (or contents all-zeros) to start over.
 * \param next_mac [OUT]  The next MAC address in the ProxyNodeTable.
 *
 * \return VTSS_RC_OK if operation succeeds. VTSS_RC_ERROR if no more entries.
 */
mesa_rc vtss_appl_redbox_pnt_itr(const uint32_t *prev_inst, uint32_t *next_inst, const mesa_mac_t *prev_mac, mesa_mac_t *next_mac);

/**
 * Get status of a ProxyNodeTable entry.
 *
 * \param instance [IN]  Redbox instance
 * \param mac      [IN]  MAC address to get status for
 * \param status   [OUT] Node status to be updated
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_pnt_mac_status_get(uint32_t instance, const mesa_mac_t *mac, vtss_appl_redbox_pnt_mac_status_t *status);

/**
 * Clear the ProxyNodeTable.
 *
 * Only non-locked entries are cleared.
 *
 * Corresponds to lreProxyNodeTableClear.
 *
 * \param instance [IN] Redbox instance
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_pnt_clear(uint32_t instance);

/**
 * Redbox per-port statistics
 *
 * Rx/Tx is seen from within the RedBox' point of view, so Rx is inbound to the
 * RedBox and Tx is outbound of the RedBox.
 */
typedef struct {
    /**
     * Number of frames received on a port that are HSR- and/or PRP-tagged.
     *
     * Partly corresponds to lreCntRxA/lreCntRxB/lreCntRxC.
     */
    uint64_t rx_tagged_cnt;

    /**
     * Number of frames sent over a port that are HSR- and/or PRP-tagged.
     *
     * Partly corresponds to lreCntTxA/lreCntTxB/lreCntTxC.
     */
    uint64_t tx_tagged_cnt;

    /**
     * Number of frames received on a port that are neither HSR- nor PRP-tagged.
     *
     * Partly corresponds to lreCntRxA/lreCntRxB/lreCntRxC.
     */
    uint64_t rx_untagged_cnt;

    /**
     * Number of frames sent over a port that are neither HSR- nor PRP-tagged.
     *
     * Partly corresponds to lreCntTxA/lreCntTxB/lreCntTxC.
     */
    uint64_t tx_untagged_cnt;

    /**
     * Number of BPDU (link-local) frames received.
     *
     * Counts if receiving frame with DMAC in range [01:80:c2:00:00:00;
     * 01:80:c2:00:00:0f].
     *
     * If this one counts, rx_tagged_cnt and rx_untagged_cnt don't.
     *
     * No corresponding value in MIB.
     */
    uint64_t rx_bpdu_cnt;

    /**
     * Number of BPDU (link-local) frames transmitted.
     *
     * No corresponding value in MIB.
     */
    uint64_t tx_bpdu_cnt;

    /**
     * Number of HSR frames received whose SMAC matches the Redbox' MAC address
     * or appears in the ProxyNodeTable (the CPU's or a VDANH/VDANP's MAC
     * address).
     *
     * Corresponds to lreCntOwnRxA/lreCntOwnRxB.
     *
     * Always zero for Port C
     */
    uint64_t rx_own_cnt;

    /**
     * Number of frames received on a port with wrong LanId.
     *
     * When this counter counts, either rx_bpdu_cnt, rx_tagged_cnt, or
     * rx_untagged_cnt also counts.
     *
     * If receiving a frame with a wrong LanId, it is forwarded no matter what
     * and is not involved in duplicate discard detection.
     *
     * Corresponds to lreCntErrWrongLanA/lreCntErrWrongLanB/lreCntErrWrongLanC.
     */
    uint64_t rx_wrong_lan_cnt;

    /**
     * Number of frames transmitted without any duplicates seen (updated when
     * <MAC, SeqNr> times out).
     *
     * Closest correspondance is lreCntUniqueA/lreCntUniqueB/lreCntUniqueC.
     */
    uint64_t tx_dupl_zero_cnt;

    /**
     * Number of frames transmitted with one duplicate discarded (updated when
     * <MAC, SeqNr> times out).
     *
     * Closest correspondance is lreCntDuplicateA/lreCntDuplicateB/
     * lreCntDuplicateC.
     */
    uint64_t tx_dupl_one_cnt;

    /**
     * Number of frames transmitted with more than one duplicate discarded
     * (updated when <MAC, SeqNr> times out).
     *
     * Closest correspondance is lreCntMultiA/lreCntMultiB/lreCntMultiC.
     */
    uint64_t tx_dupl_multi_cnt;

    /**
     * Number of erroneous SV frames.
     * Resons can be:
     *   TLV1 not present or
     *   TLV1's length is invalid or
     *   TLV1.Type is not one of the three valid types (20, 21, 23) or
     *   TLV1.MAC is not a unicast MAC or
     *   TLV2 is present *and*
     *     TLV2's length is invalid, or
     *     TLV2.Type is not 30 or
     *     TLV2.MAC is not a unicast MAC or
     *   TLV0 is not present or
     *   frame is not long enough.
     *
     * This is a S/W-based counter.
     */
    uint64_t sv_rx_err_cnt;

    /**
     * Number of SV frames filtered on ingress.
     * Reasons can be:
     *   Received on LRE port and I/L port is blocked (for some reason) or
     *   the I/L port is not member of the classified VLAN.
     *   Received on I/L port or LRE port, but TLV1 or TLV2 contains the RB's
     *   own MAC address or the switch's management MAC address. Notice that if
     *   H/W forwarding SV frames from HSR-to-PRP or PRP-to-HSR, these frames
     *   will indeed get forwarded, but not if S/W-forwarding, that is, no
     *   TLV type translation is active).
     *   In HSR-PRP mode, a SV frame received on a bridge port will get filtered
     *   if it doesn't contain a valid RCT. If xlat is active in PRP-to-HSR
     *   direction the frame will not be forwarded to the HSR ring. Otherwise,
     *   it will, but the frame will contain a RB-generated HSR tag and SeqNr,
     *   unless the frame's SMAC is learned in the PNT as a DAN, in which case
     *   the frame will be dropped. Apart from a correct PRPsuffix, a valid RCT
     *   must contain the correct LanId according to the configured lan_id and
     *   an LSDUsize of 52.
     *
     * This is a S/W-based counter.
     */
    uint64_t sv_rx_filtered_cnt;

    /**
     * Number of received SV frames per type (S/W-based).
     * These count whenever a valid SV frame is seen by S/W.
     * It may happen that the SV frame gets filtered by S/W in which case both
     * this frame and the sv_rx_filtered_cnt both count.
     * An erroneous SV frame is only counted in rx_rx_err_cnt.
     */
    uint64_t sv_rx_cnt[VTSS_APPL_REDBOX_SV_TYPE_CNT];

    /**
     * Number of transmitted SV frames (S/W-based).
     */
    uint64_t sv_tx_cnt[VTSS_APPL_REDBOX_SV_TYPE_CNT];
} vtss_appl_redbox_port_statistics_t;

/**
 * Redbox statistics
 */
typedef struct {
    /**
     * Redbox port statistics
     * Index == 0 for Port A
     * Index == 1 for Port B
     * Index == 2 for Port C
     *
     * The vtss_appl_redbox_port_type_t enumeration may be used directly when
     * dereferencing this.
     */
    vtss_appl_redbox_port_statistics_t port[3];
} vtss_appl_redbox_statistics_t;

/**
 * \brief Get Redbox instance statistics
 *
 * \param instance   [IN]  Redbox instance
 * \param statistics [OUT] Redbox statistics to be updated
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_statistics_get(uint32_t instance, vtss_appl_redbox_statistics_t *statistics);

/**
 * \brief Clear Redbox instance statistics.
 *
 * \param instance [IN] Redbox instance
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_redbox_statistics_clear(uint32_t instance);

#endif  /* _VTSS_APPL_REDBOX_H_ */
