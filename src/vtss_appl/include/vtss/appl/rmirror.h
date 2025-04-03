/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief Public API of Rmirror
 * \details Mirroring is a feature for switched port analyzer. The administrator
 * can use the Mirroring to debug network problems. The selected traffic can
 * be mirrored or copied on a destination port where a network analyzer can be
 * attached to analyze the network traffic. Remote Mirroring is an extend
 * function of Mirroring. It can extend the destination port in other switch.
 * So the administrator can analyze the network traffic on the other switches.
 */

#ifndef _VTSS_APPL_RMIRROR_H_
#define _VTSS_APPL_RMIRROR_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/** Maximum number of mirror sessions */
#define VTSS_APPL_RMIRROR_SESSION_MAX_CNT   1

/** Minmum ID of session */
#define VTSS_APPL_RMIRROR_SESSION_MIN_ID    1

/** Maxmum ID of session */
#define VTSS_APPL_RMIRROR_SESSION_MAX_ID    (1 + (VTSS_APPL_RMIRROR_SESSION_MAX_CNT) - (VTSS_APPL_RMIRROR_SESSION_MIN_ID))

/**
 * \brief Rmirror capabilities to indicate which is supported or not
 */
typedef struct {
    mesa_bool_t    reflector_port_support; /*!< reflector port supported or not    */
    mesa_bool_t    cpu_mirror_support;     /*!< CPU mirror supported or not        */
} vtss_appl_rmirror_capabilities_t;

/**
 * \brief Switch type in rmirror
 */
typedef enum {
    VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR,          /*!< The switch is running on mirror mode. */
    VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE,          /*!< The switch is a source node for monitor flow. */
    VTSS_APPL_RMIRROR_SWITCH_TYPE_INTERMEDIATE,    /*!< The switch is a forwarding node for monitor flow. The object is to forward traffic from source switch to destination switch. */
    VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION,     /*!< The switch is an end node for monitor flow. */
} vtss_appl_rmirror_switch_type_t;

/**
 * \brief Global configurations of rmirror
 */
typedef struct {
    mesa_bool_t                                mode;           /*!< Enable/Disable the mirror or Remote Mirroring function. */
    vtss_appl_rmirror_switch_type_t     switch_type;    /*!< Switch type in rmirror. */
    mesa_vid_t                          vid;            /*!< The VLAN ID points out where the monitor packet will copy to. */
} vtss_appl_rmirror_config_session_entry_t;

/**
 * \brief Mirror type
 */
typedef enum {
    VTSS_APPL_RMIRROR_MIRROR_TYPE_NONE,     /*!< Neither frames transmitted nor frames received are mirrored. */
    VTSS_APPL_RMIRROR_MIRROR_TYPE_TX,       /*!< Frames transmitted on this port are mirrored on the Intermediate/Destination port. Frames received are not mirrored. */
    VTSS_APPL_RMIRROR_MIRROR_TYPE_RX,       /*!< Frames received on this port are mirrored on the Intermediate/Destination port. Frames transmitted are not mirrored. */
    VTSS_APPL_RMIRROR_MIRROR_TYPE_BOTH,     /*!< Frames received and frames transmitted are mirrored on the Intermediate/Destination port. */
} vtss_appl_rmirror_mirror_type_t;

/**
 * \brief Source configurations of rmirror
 */
typedef struct {
    vtss_appl_rmirror_mirror_type_t     mirror_type;  /*!< Mirror type on frames to/from CPU. */
} vtss_appl_rmirror_config_session_source_cpu_t;

/**
 * \brief Source VLAN table of rmirror
 */
typedef struct {
    mesa_bool_t    mode;  /*!< Enable/Disable monitor on VLAN. */
} vtss_appl_rmirror_config_session_source_vlan_entry_t;

/**
 * \brief Source port table of rmirror
 */
typedef struct {
    vtss_appl_rmirror_mirror_type_t     mirror_type;  /*!< Mirror type on frames to/from port. */
} vtss_appl_rmirror_config_session_source_port_entry_t;

/**
 * \brief Port type in rmirror
 */
typedef enum {
    VTSS_APPL_RMIRROR_PORT_TYPE_NONE,           /*!< No role. */
    VTSS_APPL_RMIRROR_PORT_TYPE_INTERMEDIATE,   /*!< Intermediate port. */
    VTSS_APPL_RMIRROR_PORT_TYPE_DESTINATION,    /*!< Destination port. */
    VTSS_APPL_RMIRROR_PORT_TYPE_REFLECTOR,      /*!< Reflector port. */
} vtss_appl_rmirror_port_type_t;

/**
 * \brief Port table of rmirror.
 * To define the type of port in rmirror function.
 */
typedef struct {
    vtss_appl_rmirror_port_type_t     type;  /*!< Port type. */
} vtss_appl_rmirror_config_session_port_entry_t;

/*
==============================================================================

    Public APIs

==============================================================================
*/
/**
 * \brief Get capabilities
 *
 * \param cap [OUT] The capabilities of rmirror.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_capabilities_get(
    vtss_appl_rmirror_capabilities_t        *const cap
);

/**
 * \brief Iterate function of session configuration table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_entry_itr(
    const uint32_t       *const prev_session_id,
    uint32_t             *const next_session_id
);

/**
 * \brief Get session configurations
 *
 * \param session_id [IN]  The session ID.
 * \param config     [OUT] The session configurations.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_entry_get(
    uint32_t                                         session_id,
    vtss_appl_rmirror_config_session_entry_t    *const config
);

/**
 * \brief Set session configurations
 *
 * \param session_id [IN] The session ID.
 * \param config     [IN] The session configurations.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_entry_set(
    uint32_t                                             session_id,
    const vtss_appl_rmirror_config_session_entry_t  *const config
);

/**
 * \brief Iterate function of session source CPU table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 * \param prev_usid       [IN]  previous switch ID.
 * \param next_usid       [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_cpu_itr(
    const uint32_t           *const prev_session_id,
    uint32_t                 *const next_session_id,
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
);

/**
 * \brief Get source CPU configurations per session
 *
 * \param session_id [IN]  The session ID.
 * \param usid       [IN]  Switch ID
 * \param config     [OUT] The session source CPU configurations.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_cpu_get(
    uint32_t                                                 session_id,
    vtss_usid_t                                         usid,
    vtss_appl_rmirror_config_session_source_cpu_t       *const config
);

/**
 * \brief Set source CPU configurations per session
 *
 * \param session_id [IN] The session ID.
 * \param usid       [IN]  Switch ID
 * \param config     [IN] The session source CPU configurations.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_cpu_set(
    uint32_t                                                     session_id,
    vtss_usid_t                                             usid,
    const vtss_appl_rmirror_config_session_source_cpu_t     *const config
);

/**
 * \brief Iterate function of session source VLAN configuration table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 * \param prev_ifindex    [IN]  ifindex of previous VLAN.
 * \param next_ifindex    [OUT] ifindex of next VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_vlan_entry_itr(
    const uint32_t               *const prev_session_id,
    uint32_t                     *const next_session_id,
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Get source VLAN configuration entry per session
 *
 * \param session_id [IN]  The session ID.
 * \param ifindex    [IN]  ifindex of VLAN
 * \param entry      [OUT] source VLAN configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_vlan_entry_get(
    uint32_t                                                     session_id,
    vtss_ifindex_t                                          ifindex,
    vtss_appl_rmirror_config_session_source_vlan_entry_t    *const entry
);

/**
 * \brief Set source VLAN configuration entry per session
 *
 * \param session_id [IN] The session ID.
 * \param ifindex    [IN] ifindex of VLAN
 * \param entry      [IN] source VLAN configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_vlan_entry_set(
    uint32_t                                                         session_id,
    vtss_ifindex_t                                              ifindex,
    const vtss_appl_rmirror_config_session_source_vlan_entry_t  *const entry
);

/**
 * \brief Iterate function of session source port configuration table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 * \param prev_ifindex    [IN]  ifindex of previous port.
 * \param next_ifindex    [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_port_entry_itr(
    const uint32_t               *const prev_session_id,
    uint32_t                     *const next_session_id,
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Get source port configuration entry per session
 *
 * \param session_id [IN]  The session ID.
 * \param ifindex    [IN]  ifindex of port
 * \param entry      [OUT] source port configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_port_entry_get(
    uint32_t                                                     session_id,
    vtss_ifindex_t                                          ifindex,
    vtss_appl_rmirror_config_session_source_port_entry_t    *const entry
);

/**
 * \brief Set source port configuration entry per session
 *
 * \param session_id [IN] The session ID.
 * \param ifindex    [IN] ifindex of port
 * \param entry      [IN] source port configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_port_entry_set(
    uint32_t                                                         session_id,
    vtss_ifindex_t                                              ifindex,
    const vtss_appl_rmirror_config_session_source_port_entry_t  *const entry
);

/**
 * \brief Iterate function of session port configuration table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 * \param prev_ifindex    [IN]  ifindex of previous port.
 * \param next_ifindex    [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_port_entry_itr(
    const uint32_t               *const prev_session_id,
    uint32_t                     *const next_session_id,
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Get port configuration entry per session
 *
 * \param session_id [IN]  The session ID.
 * \param ifindex    [IN]  ifindex of port
 * \param entry      [OUT] Port configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_port_entry_get(
    uint32_t                                             session_id,
    vtss_ifindex_t                                  ifindex,
    vtss_appl_rmirror_config_session_port_entry_t   *const entry
);

/**
 * \brief Set port configuration entry
 *
 * To write port configuration.
 *
 * \param session_id [IN] The session ID.
 * \param ifindex    [IN] ifindex of port
 * \param entry      [IN] Port configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_port_entry_set(
    uint32_t                                                     session_id,
    vtss_ifindex_t                                          ifindex,
    const vtss_appl_rmirror_config_session_port_entry_t     *const entry
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_RMIRROR_H_ */
