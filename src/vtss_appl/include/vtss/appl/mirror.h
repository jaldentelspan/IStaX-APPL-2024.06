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
 * \brief Public API of Mirror module.
 * \details Mirroring is a feature that allows the administrator to debug
 * network problems. Selected traffic (ports or VLANs) may be mirrored
 * (copied) onto a destination port, where a network analyzer (sniffer) can be
 * attached for further analysis.
 *
 * Remote Mirroring (RMirror) is an extension to mirroring. It allows for
 * mirroring selected traffic onto a VLAN, which can be carried across a
 * network to a port on a remote switch for further analysis.
 *
 * Sessions are used to establish the streaming of mirrored traffic
 * between source interfaces and destination port/VLAN. There are three types
 * of session, mirror session (configure both source ports/VLAN and destination
 * port), RMirror source session (configure source ports/VLAN and destination
 * VLAN - the mirror VLAN), and RMirror destination session (configure
 * destination port for a given mirror VLAN).
 */

#ifndef _VTSS_APPL_MIRROR_H_
#define _VTSS_APPL_MIRROR_H_

#include <vtss/appl/module_id.h>
#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief API Error Return Codes (mesa_rc)
 */
typedef enum {
    VTSS_APPL_MIRROR_ERROR_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_MIRROR), /*!< Operation is only allowed on the primary switch                */
    VTSS_APPL_MIRROR_ERROR_ISID,                                                               /*!< isid parameter is invalid                                      */
    VTSS_APPL_MIRROR_ERROR_INV_PARAM,                                                          /*!< Invalid parameter                                              */
    VTSS_APPL_MIRROR_ERROR_VID_IS_CONFLICT,                                                    /*!< RMirror VLAN ID conflict                                       */
    VTSS_APPL_MIRROR_ERROR_ALLOCATE_MEMORY,                                                    /*!< Allocate memory error                                          */
    VTSS_APPL_MIRROR_ERROR_CONFLICT_WITH_EEE,                                                  /*!< RMirror only allowed when EEE is disabled (Jaguar1)            */
    VTSS_APPL_MIRROR_ERROR_SOURCE_SESSION_EXCEED_THE_LIMIT,                                    /*!< Mirror or RMirror source session has reached the limit         */
    VTSS_APPL_MIRROR_ERROR_SOURCE_PORTS_ARE_USED_BY_OTHER_SESSIONS,                            /*!< The source ports are used by other sessions                    */
    VTSS_APPL_MIRROR_ERROR_DESTINATION_PORTS_ARE_USED_BY_OTHER_SESSIONS,                       /*!< The destination ports are used by other sessions               */
    VTSS_APPL_MIRROR_ERROR_REFLECTOR_PORT_IS_USED_BY_OTHER_SESSIONS,                           /*!< The reflector port is used by other sessions                   */
    VTSS_APPL_MIRROR_ERROR_REFLECTOR_PORT_IS_INCLUDED_IN_SRC_OR_DEST_PORTS,                    /*!< The reflector port is included in source or destination ports  */
    VTSS_APPL_MIRROR_ERROR_REFLECTOR_PORT_IS_INVALID,                                          /*!< The port can't be used as reflector port                       */
    VTSS_APPL_MIRROR_ERROR_SOURCE_VIDS_INCLUDE_RMIRROR_VID,                                    /*!< RMirror VLAN can't be included in source VLAN                  */
    VTSS_APPL_MIRROR_ERROR_DESTINATION_PORTS_EXCEED_THE_LIMIT                                  /*!< The destination ports of Mirror session have reached the limit */
} vtss_appl_mirror_error_t;

/**
 * \brief Mirror capabilities to indicate which is supported or not
 */
typedef struct {
    uint32_t     session_cnt_max;                           /*!< The maximum number of sessions                             */
    uint32_t     session_source_cnt_max;                    /*!< The maximum number of mirror and RMirror source sessions   */
    mesa_bool_t    rmirror_support;                           /*!< RMirror feature supported or not                           */
    mesa_bool_t    internal_reflector_port_support;           /*!< Internal reflector port supported or not                   */
    mesa_bool_t    cpu_mirror_support;                        /*!< CPU mirror supported or not                                */
} vtss_appl_mirror_capabilities_t;

/**
 * \brief Session type
 */
typedef enum {
    VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR,              /*!< Mirror session                    */
    VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE,      /*!< RMirror source session            */
    VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_DESTINATION, /*!< RMirror destination session       */
} vtss_appl_mirror_session_type_t;

/**
 * \brief Session configuration
 */
typedef struct {
    /**
     * Controls whether this session is enabled or disabled.
     *
     * Multiple criteria must be fulfilled in order to be able to enable a session.
     * The criteria depend on the session #type.
     *
     * VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR:
     *   - At most one destination port may be selected.
     *   - No other mirror or rmirror source sessions may be enabled.
     *
     * VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE:
     *   - No other RMirror destination sessions may use this VLAN ID.
     *   - No other mirror or rmirror source sessions may be enabled.
     *
     * VTSS_APPL_RMIRROR_SESSION_TYPE_RMIRROR_DESTINATION:
     *   - No other RMirror destination sessions may use this VLAN ID.
     */
    mesa_bool_t enable;

    /**
     * The session type.
     */
    vtss_appl_mirror_session_type_t type;

    /**
     * The remote mirror VLAN ID. Only used for RMirror types.
     *
     * VTSS_APPL_RMIRROR_SESSION_TYPE_RMIRROR_SOURCE:
     *   The mirrored traffic is copied onto this VLAN ID.
     *   Traffic will flood to all ports that are members of the remote mirror VLAN ID.
     *
     * VTSS_APPL_RMIRROR_SESSION_TYPE_RMIRROR_DESTINATION:
     *   The #destination_port_list contains the port(s) that the mirror VLAN will be copied to
     *   in addition to ports that are already configured (through the VLAN module) to be members
     *   of this VLAN.
     */
    mesa_vid_t rmirror_vid;

    /**
     * A reflector port is a port that the administrator may have to specify in case the device
     * does not have internal (unused) ports available. Whether this is the case or not for this
     * device can be derived from #vtss_appl_mirror_capabilities_t::internal_reflector_port_support.
     * When this is TRUE, #reflector_port is not used. Otherwise it must be specified when an RMirror
     * source session is enabled. In this case, the reflector port will be shut down for normal
     * front port usage, because the switch needs a port where it can loop frames in order to get
     * mirrored traffic copied onto a particular VLAN ID (the #rmirror_vid).
     */
    vtss_ifindex_t reflector_port;

    /**
     * This configuration is used to strip the original VLAN ID of the mirrored traffic or not. When
     * it is set to TRUE, the the original VLAN ID of the mirrored traffic will be stripped, otherwise
     * the original VLAN ID will be carried to destination interface. It may have to specify in case 
     * the device does not have internal (unused) ports available. Whether this is the case or not for 
     * this device can be derived from #vtss_appl_mirror_capabilities_t::internal_reflector_port_support.
     * When this is TRUE, #strip_inner_tag is used.
     */
    mesa_bool_t strip_inner_tag;

    /**
     * Source VLAN list.
     * All traffic in the VLANs specified in this list will get mirrored onto either
     * the destination port (mirror session) or the destination VLAN (rmirror source session).
     * It's a bit-mask that indicates the VLANs. A '1' indicates the VLAN ID is selected,
     * a '0' indicates that the VLAN ID isn't selected.
     */
    vtss_vlan_list_t source_vids;

    /**
     * A bit-mask that controls whether a given port is enabled for mirroring of incoming traffic.
     * A '1' indicates that the port is included, whereas a '0' indicates it isn't.
     * Only source sessions (mirror and RMirror Source) use this value.
     */
    vtss_port_list_stackable_t source_port_list_rx;

    /**
     * A bit-mask that controls whether a given port is enabled for mirroring of outgoing traffic.
     * A '1' indicates that the port is included, whereas a '0' indicates it isn't.
     * Only source sessions (mirror and RMirror Source) use this value.
     */
    vtss_port_list_stackable_t source_port_list_tx;

    /**
     * Controls whether mirroring of traffic received by the internal CPU is enabled or disabled.
     * Only devices where #vtss_appl_mirror_capabilities_t::cpu_mirror_support is TRUE support
     * this feature.
     * Only source sessions (mirror and RMirror Source) use this value.
     */
    mesa_bool_t cpu_rx;

    /**
     * Controls whether mirroring of traffic transmitted by the internal CPU is enabled or disabled.
     * Only devices where #vtss_appl_mirror_capabilities_t::cpu_mirror_support is TRUE support
     * this feature.
     * Only source sessions (mirror and RMirror Source) use this value.
     */
    mesa_bool_t cpu_tx;

    /**
     * Destination port list implemented as a bit-mask, where a '1' indicates
     * that the port is included and a '0' indicates that it isn't.
     * Only used in plain mirror sessions and rmirror destination sessions.
     *
     * VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR:
     *   At most one bit may be set in this mask.
     *
     * VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_DESTINATION:
     *   Zero or more bits may be set in this mask.
     */
    vtss_port_list_stackable_t destination_port_list;
} vtss_appl_mirror_session_entry_t;

/**
 * \brief Get mirror module capabilities.
 *
 * The returned structure contains info about the level of
 * mirror/rmirror support of this device.
 *
 * \param cap [OUT] Capabilities of this module.
 *
 * \return VTSS_RC_OK if the operation succeeds.
 */
mesa_rc vtss_appl_mirror_capabilities_get(vtss_appl_mirror_capabilities_t *const cap);

/**
 * \brief Get configuration of a particular session.
 *
 * \param session_id [IN]  Session ID.
 * \param config     [OUT] Session configuration.
 *
 * \return VTSS_RC_OK if the operation succeeds.
 */
mesa_rc vtss_appl_mirror_session_entry_get(uint16_t session_id, vtss_appl_mirror_session_entry_t *const config);

/**
 * \brief Set or update existing session configuration.
 *
 * \param session_id [IN] Session ID.
 * \param config     [IN] New session configuration.
 *
 * \return VTSS_RC_OK if the operation succeeds.
 */
mesa_rc vtss_appl_mirror_session_entry_set(uint16_t session_id, const vtss_appl_mirror_session_entry_t  *const config);

/**
 * \brief Function for iterating across all sessions.
 *
 * \param prev_session_id [IN]  Pointer of previous session ID.
 * \param next_session_id [OUT] Pointer of next session ID. 
 *                              When prev_sesssion_id is NULL, the next_session_id is assigned to the first session ID.
 *                              When prev_sesssion_id is not NULL, the next_session_id is assigned to the next session ID.   
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_mirror_session_entry_itr(const uint16_t *const prev_session_id, uint16_t *const next_session_id);
#ifdef __cplusplus
}
#endif /* extern "C" */
#endif /* _VTSS_APPL_MIRROR_H_ */ 

