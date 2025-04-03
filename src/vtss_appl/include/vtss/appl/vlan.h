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
 * \file
 * \brief VLAN API
 * \details This header file describes public functions applicable to VLAN management.
 */

#ifndef _VTSS_APPL_VLAN_H_
#define _VTSS_APPL_VLAN_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/interface.h>

/**
 * First configurable VLAN ID
 */
#define VTSS_APPL_VLAN_ID_MIN 1

/**
 * Last configurable VLAN ID. This is the only place to change
 * if you want the maximum VLAN ID to be something else.
 */
#define VTSS_APPL_VLAN_ID_MAX 4095

/**
 * Default VLAN ID.
 */
#define VTSS_APPL_VLAN_ID_DEFAULT 1

/**
 * Maximum length - including terminating NULL - of a VLAN name
 */
#define VTSS_APPL_VLAN_NAME_MAX_LEN 33

/**
 * Name of default VLAN (VTSS_APPL_VLAN_ID_DEFAULT).
 * This cannot be changed runtime.
 */
#define VTSS_APPL_VLAN_NAME_DEFAULT "default"

/**
 * C-tag Ethertype.
 */
#define VTSS_APPL_VLAN_C_TAG_ETHERTYPE 0x8100

/**
 * S-tag Ethertype.
 */
#define VTSS_APPL_VLAN_S_TAG_ETHERTYPE 0x88A8

/**
 * Default custom S-tag Ethertype.
 */
#define VTSS_APPL_VLAN_CUSTOM_S_TAG_DEFAULT VTSS_APPL_VLAN_S_TAG_ETHERTYPE

/**
 * Number of bytes needed to represent all valid VIDs
 * ([VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX]) as a bitmask.
 *
 * Use VTSS_BF_GET() and VTSS_BF_SET() macros
 * to manipulate and obtain its entries.
 */
#define VTSS_APPL_VLAN_BITMASK_LEN_BYTES (VTSS_APPL_VLAN_ID_MAX + 8)

/**
 * Read-only structure defining the VLAN capabilities.
 */
typedef struct {
    /**
     * The minimum VLAN ID supported by this switch.
     */
    mesa_vid_t vlan_id_min;

    /** The maximum VLAN ID supported by this switch.
     */
    mesa_vid_t vlan_id_max;

    /**
     * The number of Filtering Identifiers (FIDs) supported by this switch,
     * for use by Shared VLAN Learning (SVL).
     * If 0, SVL is not supported, that is, the switch only has Independent
     * VLAN Learning (IVL) support, and there is a one-to-one correspondence
     * between VLAN ID and FID.
     */
    uint16_t fid_cnt;

    /**
     * If true, flooding can be managed.
     */
    mesa_bool_t has_flooding;

} vtss_appl_vlan_capabilities_t;

/**
 * This enum identifies VLAN users.
 *
 * A VLAN user is a module that can modify VLAN configuration
 * at runtime. VTSS_APPL_VLAN_USER_STATIC corresponds to an end-user,
 * that is, an administrator using CLI, SNMP, or Web to change VLAN
 * configuration.
 *
 * Users may override each other's VLAN configuration in a hierarchical
 * way.
 */
typedef enum {
    VTSS_APPL_VLAN_USER_ALL,          /**< Used to retrieve the current configuration as programmed to H/W */
    VTSS_APPL_VLAN_USER_STATIC,       /**< Administrator */
    VTSS_APPL_VLAN_USER_FORBIDDEN,    /**< Not a real user, but easiest to deal with in terms of management, when forbidden VLAN is enumerated as a user */

    // Here come the volatile overriders:
    VTSS_APPL_VLAN_USER_DOT1X,            /**< 802.1X/NAS */
    VTSS_APPL_VLAN_USER_MVRP,             /**< MVRP */
    VTSS_APPL_VLAN_USER_GVRP,             /**< GVRP */
    VTSS_APPL_VLAN_USER_MVR,              /**< MVR */
    VTSS_APPL_VLAN_USER_VOICE_VLAN,       /**< Voice VLAN */
    VTSS_APPL_VLAN_USER_MSTP,             /**< MSTP */
    VTSS_APPL_VLAN_USER_ERPS,             /**< ERPS */
    VTSS_APPL_VLAN_USER_IEC_MRP,          /**< MRP */
    VTSS_APPL_VLAN_USER_MEP_OBSOLETE,     /**< MEP (obsolete) */
    VTSS_APPL_VLAN_USER_EVC_OBSOLETE,     /**< EVC (obsolete) */
    VTSS_APPL_VLAN_USER_VCL,              /**< VCL */
    VTSS_APPL_VLAN_USER_RMIRROR,          /**< Remote Mirroring */
    VTSS_APPL_VLAN_USER_TT_LOOP_OBSOLETE, /**< TT-LOOP (obsolete) */

    // For the sake of persistent layout of SNMP OIDs,
    // new volatile modules must come just before this line.
    VTSS_APPL_VLAN_USER_LAST        /**< May be needed for iterations across this enum. Must come last */
} vtss_appl_vlan_user_t;

/**
 * Controls how egress tagging occurs.
 *
 * Don't change the enumeration without also changing vlan_port.htm.
 */
typedef enum {
    VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS, /**< Send .untagged_vid untagged. User module doesn't care about other VIDs.                  */
    VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS,   /**< Send .untagged_vid tagged. User module doesn't care about other VIDs.                    */
    VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL,    /**< All 4K VLANs shall be sent tagged, despite this user module's membership configuration   */
    VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL,  /**< All 4K VLANs shall be sent untagged, despite this user module's membership configuration */
} vtss_appl_vlan_tx_tag_type_t;

/**
 * Flags to indicate what part of a VLAN port configuration,
 * the user wants to configure.
 *
 * Do a bit-wise OR of the flags to indicate which members
 * of vtss_appl_vlan_port_detailed_conf_t you wish to control.
 *
 * To uncontrol a feature that was previously overridden,
 * clear the corresponding flag (this only works for
 * volatile users (i.e. user != VTSS_APPL_VLAN_USER_STATIC).
 */
enum {
    VTSS_APPL_VLAN_PORT_FLAGS_PVID        = (1 << 0), /**< Control vtss_appl_vlan_port_detailed_conf_t::pvid                                                                            */
    VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT   = (1 << 1), /**< Control vtss_appl_vlan_port_detailed_conf_t::ingress_filter                                                                  */
    VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE = (1 << 2), /**< Control vtss_appl_vlan_port_detailed_conf_t::frame_type                                                                      */
    VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE = (1 << 3), /**< Control vtss_appl_vlan_port_detailed_conf_t::tx_tag_type and possibly also vtss_appl_vlan_port_detailed_conf_t::untagged_vid */
    VTSS_APPL_VLAN_PORT_FLAGS_AWARE       = (1 << 4), /**< Control vtss_appl_vlan_port_detailed_conf_t::port_type                                                                       */
    VTSS_APPL_VLAN_PORT_FLAGS_ALL         = (VTSS_APPL_VLAN_PORT_FLAGS_PVID | VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT | VTSS_APPL_VLAN_PORT_FLAGS_RX_TAG_TYPE | VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE | VTSS_APPL_VLAN_PORT_FLAGS_AWARE)
}; // Anonymous to satisfy Lint.

/**
 * VLAN awareness and port type configuration.
 * Ports that are not configured as VTSS_APPL_VLAN_PORT_TYPE_UNAWARE
 * are VLAN aware and react on the corresponding
 * tag type. VLAN aware ports always react on
 * C tags.
 */
typedef enum {
    VTSS_APPL_VLAN_PORT_TYPE_UNAWARE, /**< VLAN unaware port                       */
    VTSS_APPL_VLAN_PORT_TYPE_C,       /**< C-port (TPID = 0x8100)                  */
    VTSS_APPL_VLAN_PORT_TYPE_S,       /**< S-port (TPID = 0x88A8)                  */
    VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM /**< S-port using customizable ethernet type */
} vtss_appl_vlan_port_type_t;

/**
 * VLAN port configuration
 */
typedef struct {
    /**
     * Port VLAN ID. [VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX].
     */
    mesa_vid_t pvid;

    /**
     * Port Untagged VLAN ID (egress).
     *
     * If #tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
     *   #untagged_vid indicates the only VID [VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX] not to tag.
     *
     * If #tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
     *   #untagged_vid indicates the VID [VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX] to tag.
     *   In reality, this is implemented as follows:
     *   If #untagged_vid == #pvid, then all frames are tagged.
     *   If #untagged_vid != #pvid, then all but #pvid are tagged.
     *
     * If #tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
     *  All frames are tagged on egress. #untagged_vid is not used, and shouldn't be shown/used.
     *
     * If #tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
     *  All frames are untagged on egress. #untagged_vid is not used, and shouldn't be shown/used.
     */
    mesa_vid_t untagged_vid;

    /**
     * Controls VLAN awareness and whether it reacts to
     * C-tags, S-tags, and Custom-S-tags.
     */
    vtss_appl_vlan_port_type_t port_type;

    /**
     * Ingress filtering.
     * If TRUE, incoming frames classified to a VLAN that
     * the port is not a member of are discarded.
     * It's a compile-time option to get this user-controllable
     * (see VTSS_SW_OPTION_VLAN_INGRESS_FILTERING).
     * If FALSE, incoming frames will never be discarded as a result
     * of VLAN classification (that is, the port doesn't need be
     * member of the VLAN to which it gets classified).
     */
    mesa_bool_t ingress_filter;

    /**
     * Acceptable frame type (ingress).
     * Either, accept all, accept tagged only, or accept untagged only.
     */
    mesa_vlan_frame_t frame_type;

    /**
     * Indicates egress tag requirements. See also #untagged_vid.
     */
    vtss_appl_vlan_tx_tag_type_t tx_tag_type;

    /**
     * Flags to indicate what part of the VLAN port configuration,
     * a VLAN user has configured.
     * They are a bit-wise OR of VTSS_APPL_VLAN_PORT_FLAGS_xxx.
     *
     * If a volatile user (i.e. user != VTSS_APPL_VLAN_USER_STATIC) wishes
     * to un-override a feature that was previously overridden,
     * clear the flag.
     */
    uint8_t flags;
} vtss_appl_vlan_port_detailed_conf_t;

/**
 * Structure required in all VLAN membership manipulation functions.
 */
typedef struct {
    /**
     * VLAN ID.
     *
     * In vtss_appl_vlan_get(), this is an [OUT] parameter ranging
     * from [0; VTSS_APPL_VLAN_ID_MAX]. 0 (VTSS_VID_NULL) indicates that the
     * requested VLAN doesn't exist.
     *
     * In vtss_appl_vlan_add(), this is an [IN] parameter ranging
     * from [VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX].
     */
    mesa_vid_t vid;

    /**
     * Array of ports memberships.
     *
     * In vtss_appl_vlan_get(), this is an [OUT] parameter.
     *
     * In vtss_appl_vlan_add(), this is an [IN] parameter.
     *
     * If an entry is TRUE, the corresponding port is a
     * member of the VLAN, if FALSE, it's not.
     *
     * If getting/setting forbidden VLANs, a TRUE entry
     * indicates that the #vid is not allowed on the
     * indexed port.
     */
    mesa_port_list_t ports;
} vtss_appl_vlan_entry_t;

/**
 * VLAN Port Modes.
 *
 * Do not change the order of ACCESS, TRUNK, and HYBRID,
 * since these are used in iterators here and there.
 */
typedef enum {
    /**
     * Access port.
     * An access port:
     *  - is C-tag VLAN aware,
     *  - has ingress filtering enabled,
     *  - accepts both tagged and untagged frames,
     *  - has PVID set to vtss_appl_vlan_port_conf_t::access_pvid,
     *  - member of vtss_appl_vlan_port_conf_t::access_pvid, only,
     *  - untags all frames on egress
     *
     * An access port has these low-level properties:
     *  low_level->tx_tag_type    = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL;
     *  low_level->frame_type     = MESA_VLAN_FRAME_ALL;
     *  low_level->ingress_filter = TRUE;
     *  low_level->port_type      = VTSS_APPL_VLAN_PORT_TYPE_C;
     *  low_level->pvid           = high_level->access_pvid; // Default is VTSS_APPL_VLAN_ID_DEFAULT
     */
    VTSS_APPL_VLAN_PORT_MODE_ACCESS,

    /**
     * Trunk port.
     * A trunk port:
     *  - is C-tag VLAN aware,
     *  - has ingress filtering enabled,
     *  - has PVID set to vtss_appl_vlan_port_conf_t::trunk_pvid,
     *  - automatically becomes a member of all VLANs that are set in the array
     *    defining allowed VIDs (vtss_appl_vlan_port_conf_t::trunk_allowed_vids).
     *  - allows for having the trunk_pvid tagged or untagged on egress,
     *  - if tagging trunk_pvid, the port accepts tagged frames only
     *  - if untagging trunk_pvid, the port accepts both tagged and untagged frames.
     *
     * A trunk port has these low-level properties:
     *  low_level->pvid            = high_level->trunk_pvid; // Default is VTSS_APPL_VLAN_ID_DEFAULT
     *  low_level->untagged_vid    = high_level->trunk_pvid; // Default is VTSS_APPL_VLAN_ID_DEFAULT
     *  low_level->port_type       = VTSS_APPL_VLAN_PORT_TYPE_C;
     *  low_level->ingress_filter  = TRUE;
     *  if (high_level->trunk_tag_pvid) {             // Default is FALSE
     *       low_level->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL;
     *       low_level->frame_type  = MESA_VLAN_FRAME_TAGGED;
     *   } else {
     *       low_level->tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS;
     *       low_level->frame_type  = MESA_VLAN_FRAME_ALL;
     *   }
     */
    VTSS_APPL_VLAN_PORT_MODE_TRUNK,

    /**
     * Hybrid port.
     * A hybrid port is completely end-user-controllable w.r.t.:
     *  - VLAN awareness and port type,
     *  - PVID,
     *  - egress tagging,
     *  - ingress filtering
     * A hybrid port's port configuration is held in vtss_appl_vlan_port_conf_t::hybrid.
     * A hybrid port automatically becomes a member of all VLANs that
     * are set in the array defining allowed VIDs (vtss_appl_vlan_port_conf_t::hybrid_allowed_vids).
     *
     * Default low-level properties for a hybrid port are:
     *  low_level->pvid           = VTSS_APPL_VLAN_ID_DEFAULT;
     *  low_level->untagged_vid   = VTSS_APPL_VLAN_ID_DEFAULT;
     *  low_level->frame_type     = MESA_VLAN_FRAME_ALL;
     *  low_level->ingress_filter = FALSE;
     *  low_level->tx_tag_type    = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS;
     *  low_level->port_type      = VTSS_APPL_VLAN_PORT_TYPE_C;
     */
    VTSS_APPL_VLAN_PORT_MODE_HYBRID,
} vtss_appl_vlan_port_mode_t;

/**
 * Structure to hold the port configuration for Access, Trunk, and Hybrid modes.
 */
typedef struct {
    /**
     * Port mode as defined by vtss_appl_vlan_port_mode_t above.
     */
    vtss_appl_vlan_port_mode_t mode;

    /**
     * When #mode == VTSS_APPL_VLAN_PORT_MODE_ACCESS,
     * this is the PVID the port will be assigned.
     */
    mesa_vid_t access_pvid;

    /**
     * When #mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK,
     * this is the PVID the port will be assigned.
     */
    mesa_vid_t trunk_pvid;

    /**
     * When #mode == VTSS_APPL_VLAN_PORT_MODE_TRUNK,
     * this controls whether PVID (i.e. #trunk_pvid) will be
     * tagged on egress or not.
     */
    mesa_bool_t trunk_tag_pvid;

    /**
     * This is a bit-mask that indicates the VLANs that a port
     * will automatically become a member of provided the port
     * is in trunk \p mode.
     *
     * Use VTSS_BF_GET()/VTSS_BF_SET() on the array to get/set
     * the individual bits. A '1' indicates that the VLAN is allowed
     * and a 0, that it is disallowed.
     */
    uint8_t trunk_allowed_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

    /**
     * This is a bit-mask that indicates the VLANs that a port
     * will automatically become a member of provided the port
     * is in hybrid \p mode.
     *
     * Use VTSS_BF_GET()/VTSS_BF_SET() on the array to get/set
     * the individual bits. A '1' indicates that the VLAN is allowed
     * and a 0, that it is disallowed.
     */
    uint8_t hybrid_allowed_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

    /**
     * When #mode == VTSS_APPL_VLAN_PORT_MODE_HYBRID, this is the
     * port configuration the port will get.
     */
    vtss_appl_vlan_port_detailed_conf_t hybrid;

    /**
     * This is a bit-mask that indicates the VLANs that a port
     * must NEVER become a member of. This in order to prevent
     * dynamic protocols like GVRP from adding membership.
     *
     * Use VTSS_BF_GET()/VTSS_BF_SET() on the array to get/set
     * the individual bits. A '1' indicates that the VLAN is
     * forbidden, and a '0' that it is not forbidden.
     */
    uint8_t forbidden_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
} vtss_appl_vlan_port_conf_t;

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
//
// GLOBAL CONFIGURATION FUNCTIONS ET AL
//
/******************************************************************************/

/**
 * Get this switch's VLAN capabilities.
 *
 * \param cap [OUT] Pointer to structure receiving the R/O capabilities.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vtss_appl_vlan_capabilities_get(vtss_appl_vlan_capabilities_t *cap);

/**
 * Set ethertype for Custom S-port
 *
 * \param tpid [IN] TPID (Ethertype) for ports marked as Custom-S aware.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vtss_appl_vlan_s_custom_etype_set(mesa_etype_t tpid);

/**
 * Get ethertype for Custom S-port
 *
 * \param tpid [OUT] Pointer receiving current TPID (Ethertype) for ports marked as Custom-S aware.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vtss_appl_vlan_s_custom_etype_get(mesa_etype_t *tpid);

/******************************************************************************/
//
// INTERFACE FUNCTIONS ET AL
//
/******************************************************************************/

/**
 * Get VLAN configuration for a port interface.
 *
 * The function returns the VLAN port configuration for \p ifindex, which must index a non-stack port.
 *
 * The function can only be invoked on the primary switch.
 * \p user takes any value from vtss_appl_vlan_user_t.
 *
 * \p details controls whether to get the high-level port configuration (when FALSE)
 * or a detailed-out configuration (when TRUE). It has only effect when
 * \p user == VTSS_APPL_VLAN_USER_STATIC. All other values of \p user assumes that \p details is TRUE.
 * When details are requested, the only valid part of \p conf is what is in \p hybrid.
 *
 * \param ifindex [IN]  Interface index for which to get the configuration. Only non-stacking port interfaces are allowed.
 * \param user    [IN]  VLAN user to obtain configuration for. See also description above.
 * \param conf    [OUT] Pointer to structure retrieving the current interface configuration.
 *
 * \param details [IN]  Only applicable for \p user VTSS_APPL_VLAN_USER_STATIC. If TRUE, hybrid-part of \p conf is filled in with details, rather than the remaining members of \p conf.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vtss_appl_vlan_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_vlan_user_t user, vtss_appl_vlan_port_conf_t *conf, mesa_bool_t details);

/**
 * Set current interface configuration for administrative user.
 *
 * The VLAN user is implicitly set to VTSS_APPL_VLAN_USER_STATIC.
 *
 * There is no guarantee that the configuration will take effect, because
 * it could happen that a higher prioritized VLAN user has already configured
 * the features that are attempted configured now.
 *
 * \param ifindex [IN] Interface index for which to change the configuration. Only non-stacking port interfaces are allowed.
 * \param conf    [IN] Pointer to structure holding the new interface configuration.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vtss_appl_vlan_interface_conf_set(vtss_ifindex_t ifindex, const vtss_appl_vlan_port_conf_t *conf);

/******************************************************************************/
//
// MEMBERSHIP FUNCTIONS
//
/******************************************************************************/

/**
 * Get list of access VIDs
 *
 * Access ports may have an Access VLAN that is not yet created, and therefore
 * such ports may not be able to receive or transmit frames until it actually
 * gets created, by the use of this function.
 *
 * Use VTSS_BF_GET() on the array to figure out whether a given VLAN
 * is also an access VLAN. A '0' indicates that it is not, and a '1' indicates
 * that it is.
 *
 * \param access_vids [OUT] List of Access VLAN IDs.
 *
 * \return VTSS_RC_OK in most cases. If something different from VTSS_RC_OK is returned,
 *         it's because of parameters passed to the function or because the switch is
 *         currently not primary switch.
 */
mesa_rc vtss_appl_vlan_access_vids_get(uint8_t access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES]);

/**
 * Set list of access VIDs
 *
 * Ports configured as Access ports will not be able to receive or transmit frames
 * unless the port's Access VLAN is also set in the bitmask applied to this
 * function.
 *
 * \param access_vids [IN] List of Access VLAN IDs.
 *
 * \return VTSS_RC_OK in most cases. If something different from VTSS_RC_OK is returned,
 *         it's because of parameters passed to the function or because the switch is
 *         currently not primary switch.
 */
mesa_rc vtss_appl_vlan_access_vids_set(const uint8_t access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES]);

/**
 * Get VLAN membership.
 *
 * Use this function go get port memberships for either a
 * specific VID or the next defined VID.
 * The function can also be used to simply figure out
 * whether a given VLAN ID is defined or not.
 *
 * \p user must be a VLAN user in range [VTSS_APPL_VLAN_USER_STATIC; VTSS_APPL_VLAN_USER_ALL].
 *
 * What the \p user has configured is not necessarily what is in hardware,
 * because of forbidden VLANs, which override everything.
 *
 * If invoked with VTSS_APPL_VLAN_USER_ALL, the returned value is the combined
 * membership of all VLAN users as programmed to hardware.
 *
 * \p next == FALSE:
 *   Get specific VID membership.
 *
 *   \p vid must be a legal VID in range [VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX].
 *
 *   If \p isid is VTSS_ISID_LOCAL, the function will read directly
 *   from hardware. This is the only value of \p isid that is allowed
 *   on a secondary switch.
 *
 *   If \p isid is a legal ISID ([VTSS_ISID_START; VTSS_ISID_END[),
 *   and the \p vid exists on that switch for \p user, this function will
 *   return VTSS_RC_OK and \p membership's ports member will contain the
 *   membership information and \p membership's vid member will be set to \p vid.
 *
 *   If \p isid is a legal ISID but \p vid does not exist for \p user on this
 *   switch, the function returns VLAN_ERROR_ENTRY_NOT_FOUND, and
 *   \p membership->vid will be VTSS_VID_NULL.
 *
 *   If \p isid is VTSS_ISID_GLOBAL and at least one switch has
 *   \p vid defined for \p user, this function will return VTSS_RC_OK
 *   and \p membership's vid member will contain \p vid. The \p membership's ports member
 *   will NOT be valid. This can be used to figure out whether
 *   a VID is defined on any switch, but it can't be used to
 *   get membership information.
 *
 *   If \p isid is VTSS_ISID_GLOBAL but \p vid does not exist for \p user on any
 *   configurable switch in the stack, the function returns
 *   VLAN_ERROR_ENTRY_NOT_FOUND, and \p membership's vid member will be VTSS_VID_NULL.
 *
 * \p next == TRUE:
 *   Get the next defined VID greater than \p vid for \p user (so
 *   it doesn't make sense to invoke the function with \p next = TRUE
 *   and \p vid = VTSS_APPL_VLAN_ID_MAX).
 *
 *   If \p isid is VTSS_ISID_LOCAL, the function will read directly
 *   from hardware. This is the only value of \p isid that is allowed
 *   on a secondary switch.
 *
 *   If \p isid is a legal ISID ([VTSS_ISID_START; VTSS_ISID_END[),
 *   only this specific switch is searched for the next, closest
 *   VID > \p vid installed by \p user. If such a VID is found, this
 *   function returns VTSS_RC_OK and sets both \p membership's ports member
 *   and \p membership's vid member.
 *   If no such VID is found, this function returns
 *   VLAN_ERROR_ENTRY_NOT_FOUND and sets \p membership's vid member to VTSS_VID_NULL.
 *
 *   If \p isid is VTSS_ISID_GLOBAL, all switches are searched for
 *   the next, closest VID > \p vid installed by \p user. If such a VID
 *   is found, this function returns VTSS_RC_OK and sets \p membership's vid member,
 *   while leaving \p membership's ports member undefined.
 *   If no such VID is found, this function returns
 *   VLAN_ERROR_ENTRY_NOT_FOUND and sets \p membership's vid to VTSS_VID_NULL.
 *
 * \param isid       [IN]  VTSS_ISID_LOCAL, VTSS_ISID_GLOBAL, or legal ISID (see above).
 * \param vid        [IN]  VID to get (\p next == FALSE) or to start from (\p next == TRUE).
 * \param membership [OUT] Result of doing the get (see above).
 * \param next       [IN]  If FALSE, get \p vid, only. If TRUE, search sequentially from [\p vid + 1; VTSS_APPL_VLAN_ID_MAX].
 * \param user       [IN]  The VLAN user to lookup. VTSS_APPL_VLAN_USER_ALL holds the combined state and will be available if at least one other VLAN user has installed a VID.
 *
 * \return VTSS_RC_OK if a VID was found for \p user. \p membership contains valid information (hereunder the found \p vid).
 *         Returns VLAN_ERROR_ENTRY_NOT_FOUND if no entries were found. \p membership's vid member is VTSS_VID_NULL.
 *         Returns anything else on input parameter errors.
 */
mesa_rc vtss_appl_vlan_get(vtss_isid_t isid, mesa_vid_t vid, vtss_appl_vlan_entry_t *membership, mesa_bool_t next, vtss_appl_vlan_user_t user);

/******************************************************************************/
//
// VLAN flooding functions
//
/******************************************************************************/

/**
 * Get VLAN flooding option given a VLAN ID.
 *
 * \param vid        [IN]  VID to look up.
 * \param flooding   [OUT] Pointer to a mesa_bool_t. If TRUE, flooding is enabled.
 *
 * \return VTSS_RC_OK if entry was found and \p flooding filled in.
 *         Anything else means input parameter error.
 */
mesa_rc vtss_appl_vlan_flooding_get(mesa_vid_t vid, mesa_bool_t *flooding);

/**
 * Set VLAN flooding option for a VLAN ID.
 *
 * \param vid       [IN] VID to change
 * \param flooding  [IN] New value of flooding. TRUE is flooding enable, FALSE is flooding disable.
 *
 * \return VTSS_RC_OK on success.
 *         Anything else means input parameter error.
 */
mesa_rc vtss_appl_vlan_flooding_set(mesa_vid_t vid, mesa_bool_t flooding);

/******************************************************************************/
//
// VLAN naming functions
//
/******************************************************************************/

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/**
 * Get a VLAN name given a VLAN ID.
 *
 * \param vid        [IN]  VID to look up.
 * \param name       [OUT] Pointer to string receiving the resulting name.
 * \param is_default [OUT] Pointer to a mesa_bool_t that gets set to FALSE if this is the default VLAN name for this VID, FALSE otherwise. NULL is an OK value to pass.
 *
 * \return VTSS_RC_OK if entry was found and \p name filled in.
 *         Anything else means input parameter error.
 */
mesa_rc vtss_appl_vlan_name_get(mesa_vid_t vid, char name[VTSS_APPL_VLAN_NAME_MAX_LEN], mesa_bool_t *is_default);
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

#if defined(VTSS_SW_OPTION_VLAN_NAMING)
/**
 * Set a VLAN name for a VLAN ID.
 *
 * \p name must be an alphanumeric string of up to VTSS_APPL_VLAN_NAME_MAX_LEN - 1
 * characters, starting with a non-digit.
 *
 * Setting it to the empty string corresponds to defaulting the VLAN name.
 * Setting it to "VLANxxxx", where xxxx is four decimal digits (with leading
 * zeroes) and these digits translates to a valid VLAN ID and that VLAN ID
 * is not equal to \p vid, an error is returned. If it *is* equal to \p vid, it
 * corresponds to defaulting it.
 *
 * The word VTSS_APPL_VLAN_NAME_DEFAULT is reserved for VTSS_APPL_VLAN_ID_DEFAULT. The name of
 * VTSS_APPL_VLAN_ID_DEFAULT cannot be changed.
 *
 * Accepted characters are in the range [33; 126].
 *
 * \param vid  [IN]  VID to change the name of. Allowed range is [VTSS_APPL_VLAN_ID_MIN; VTSS_APPL_VLAN_ID_MAX] except VTSS_APPL_VLAN_ID_DEFAULT, unless \p name is VTSS_APPL_VLAN_NAME_DEFAULT.
 * \param name [OUT] New name of VLAN.
 *
 * \return VTSS_RC_OK on success.
 *         VLAN_ERROR_NAME_RESERVED if a reserved VLAN name (VTSS_APPL_VLAN_NAME_DEFAULT or "VLANxxxx") is used for a VLAN ID that is not supposed to have this name.
 *         VLAN_ERROR_NAME_ALREADY_EXISTS if a VLAN with that name is already configured.
 *         Anything else means input parameter error.
 */
mesa_rc vtss_appl_vlan_name_set(mesa_vid_t vid, const char name[VTSS_APPL_VLAN_NAME_MAX_LEN]);
#endif /* defined(VTSS_SW_OPTION_VLAN_NAMING) */

/**
 * Get FID assigned to a given VID.
 *
 * In Shared VLAN Learning (SVL), one or more VLAN IDs may map to a given
 * Filter ID (FID). By default, there is a one-to-one correspondence
 * between FID and VID, so that VID x maps to FID x. If the switch
 * does not support SVL, this one-to-one mapping cannot be changed,
 * and the switch can be considered a plain IVL bridge. Whether SVL
 * is supported can be read through vtss_appl_vlan_capabilities_t::fid_cnt.
 * If this value is 0, there is no SVL support.
 * Otherwise, a FID in range [1; fid_cnt] can be assigned to a given VID.
 *
 * As said, VID x maps to FID x by default, but since there might not
 * be as many FIDs as VIDs on a given platform, the special value 0
 * for FID means that there is a one-to-one correspondence between VID
 * and FID.
 * In fact, when calling vtss_appl_vlan_fid_set() with VID = x and FID = x
 * the FID will implicitly be set to 0, so that a subsequent read of the
 * mapping returns FID == 0.
 *
 * \param vid  [IN] VLAN ID to get FID for.
 * \param fid [OUT] FID corresponding to VID.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vtss_appl_vlan_fid_get(mesa_vid_t vid, mesa_vid_t *fid);

/**
 * Assign VID to a FID.
 *
 * See vtss_appl_vlan_fid_get() for a description of SVL.
 *
 * This function assigns a FID to a given VID. To take advantage
 * of SVL, you will probably assign the FID to multiple VIDs.
 * In this case, call this function multiple times with the same FID.
 *
 * Note that if you assign VID x to FID x, then the underlying code
 * will set the FID to 0, which has the same effect (see
 * vtss_appl_vlan_fid_get() for details).
 *
 * \param vid [IN] VLAN ID that should be assigned a new FID.
 * \param fid [IN] FID to assign to \p vid.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vtss_appl_vlan_fid_set(mesa_vid_t vid, mesa_vid_t fid);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_VLAN_H_ */

