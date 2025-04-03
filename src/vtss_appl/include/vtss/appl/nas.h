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

#ifndef _VTSS_APPL_NAS_H_
#define _VTSS_APPL_NAS_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/aggr.h>
#include <vtss/appl/defines.h>

/**
 * \file
 *
 * \brief Public NAS management API
 *
 * \details This header file describes the NAS control functions and
 * associated types and defines.
 *
 * NAS stands for Network Access Server in this respect.
 *
 * A Network Access Server is the initial entry point to a network for the majority of
 * users of network services. It is the first device in the network to provide services
 * to an end user, and acts as a gateway for all further services. As such, its importance
 * to users and service providers alike is paramount.
 *
 * NAS provide some underlining access control mechanisms. E.g. dot1x and
 * MAC-based authentication.
 *
 * The IEEE 802.1X standard defines a port-based access control procedure that prevents
 * unauthorized access to a network by requiring users to first submit credentials for
 * authentication.
 * One or more central servers, the backend servers, determine whether the user is allowed
 * access to the network.
 *
 * MAC-based authentication allows for authentication of more than one user on the same
 * port, and doesn't require the user to have special 802.1X supplicant software installed
 * on his system.
 * The switch uses the user's MAC address to authenticate against the backend server.
 * Intruders can create counterfeit MAC addresses, which makes MAC-based authentication
 * less secure than 802.1X authentication.
 */

/** \brief NAS error return codes (mesa_rc).*/
enum {
    /**
     * NULL parameter passed to one of the dot1x_mgmt_XXX functions, where a non-NULL was expected.
     */
    VTSS_APPL_NAS_ERROR_INV_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_DOT1X),

    /**
     * Re-authentication period out of bounds.
     */
    VTSS_APPL_NAS_ERROR_INVALID_REAUTH_PERIOD,

    /**
     * EAPOL timeout out of bounds.
     */
    VTSS_APPL_NAS_ERROR_INVALID_EAPOL_TIMEOUT,

    /**
     * Invalid administration state.
     */
    VTSS_APPL_NAS_ERROR_INVALID_ADMIN_STATE,

    /**
     * Management operation is not valid on secondary switches.
     */
    VTSS_APPL_NAS_ERROR_MUST_BE_PRIMARY_SWITCH,

    /**
     * Invalid Switch ID.
     */
    VTSS_APPL_NAS_ERROR_SID,

    /**
     * Invalid port number.
     */
    VTSS_APPL_NAS_ERROR_PORT,

    /**
     * Static aggregation is enabled on a port that is attempted set to Force Unauthorized
     * or Auto.
     */
    VTSS_APPL_NAS_ERROR_STATIC_AGGR_ENABLED,

    /**
     * Dynamic aggregation (LACP) is enabled on a port that is attempted set to Force
     * Unauthorized or Auto.
     */
    VTSS_APPL_NAS_ERROR_DYNAMIC_AGGR_ENABLED,

    /**
     * Spanning Tree is enabled on a port that is attempted set to Force Unauthorized or
     * Auto.
     */
    VTSS_APPL_NAS_ERROR_STP_ENABLED,

    /**
     * No state machine found corresponding to specified MAC address.
     */
    VTSS_APPL_NAS_ERROR_MAC_ADDRESS_NOT_FOUND,

    /**
     * The hold-time for clients whose authentication failed was out of bounds.
     */
    VTSS_APPL_NAS_ERROR_INVALID_HOLD_TIME,

    /**
     * The aging-period for clients whose authentication succeeded was out of bounds.
     */
    VTSS_APPL_NAS_ERROR_INVALID_AGING_PERIOD,

    /**
     * The Guest VLAN ID is invalid.
     */
    VTSS_APPL_NAS_ERROR_INVALID_GUEST_VLAN_ID,

    /**
     * The maximum numbers of re-authentications are invalid.
     */
    VTSS_APPL_NAS_ERROR_INVALID_REAUTH_MAX,

    /**
     * No backend (RADIUS) servers are ready to accept request
     */
    VTSS_APPL_NAS_ERROR_BACKEND_SERVERS_NOT_READY,

    /**
     * Backend (RADIUS) server timed out
     */
    VTSS_APPL_NAS_ERROR_BACKEND_SERVER_TIMEOUT,
};

/** \brief NAS port control */
typedef enum {
    VTSS_APPL_NAS_PORT_CONTROL_DISABLED           = 0, /**< Forces a port to be disabled.*/
    VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED   = 1, /**< Forces a port to grant access to all clients, 802.1X-aware or not.*/
    VTSS_APPL_NAS_PORT_CONTROL_AUTO               = 2, /**< Requires an 802.1X-aware client to be authorized by the authentication
                                                            server. Clients that are not 802.1X-aware will be denied access.*/
    VTSS_APPL_NAS_PORT_CONTROL_FORCE_UNAUTHORIZED = 3, /**< Forces a port to deny access to all clients, 802.1X-aware or not.*/
    VTSS_APPL_NAS_PORT_CONTROL_MAC_BASED          = 4, /**< The switch authenticates on behalf of the client, using the client's
                                                            MAC-address as the username and password and MD5 EAP method.*/
    VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE       = 5, /**< At most one supplicant is allowed to authenticate, and it authenticates
                                                            using normal 802.1X frames.*/
    VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI        = 6, /**< One or more supplicants are allowed to authenticate individually using
                                                            an 802.1X variant, where EAPOL frames sent from the switch are directed
                                                            towards the supplicants MAC address instead of using the multi-cast
                                                            BPDU MAC address. Unauthenticated supplicants won't get access.*/
} vtss_appl_nas_port_control_t;

/** \brief NAS port status */
typedef enum  {
    VTSS_APPL_NAS_PORT_STATUS_LINK_DOWN      = 0, /**< Port has link down. */
    VTSS_APPL_NAS_PORT_STATUS_AUTHORIZED     = 1, /**< Port is authenticated.*/
    VTSS_APPL_NAS_PORT_STATUS_UNAUTHORIZED   = 2, /**< Port is unauthenticated.*/
    VTSS_APPL_NAS_PORT_STATUS_DISABLED       = 3, /**< Port is disabled. */
    VTSS_APPL_NAS_PORT_STATUS_MULTI          = 4, /**< Port is in multi-client mode. Use #vtss_appl_nas_interface_status_t::auth_cnt and unauth_cnt to find number of authorized/unauthorized clients. */
} vtss_appl_nas_port_status_t;

/** \brief NAS Port configuration */
typedef struct {
    vtss_appl_nas_port_control_t admin_state;    /**< Administrative state.*/
    mesa_bool_t qos_backend_assignment_enabled;  /**< Set to TRUE if RADIUS-assigned QoS is enabled for this port.*/
    mesa_bool_t vlan_backend_assignment_enabled; /**< Set to TRUE if RADIUS-assigned VLAN is enabled for this port.*/
    mesa_bool_t guest_vlan_enabled;              /**< Set to TRUE if Guest-VLAN is enabled for this port.*/
} vtss_appl_nas_port_cfg_t;

/****************************************************************************/
// Global configuration.
// The following structure defines 802.1X and MAC-based parameters.
/****************************************************************************/
/** \brief NAS global configuration. Common to all switches in the stack.*/
typedef struct {
    mesa_bool_t enabled; /**< Globally enable/disable 802.1X/MAC-Based authentication.*/

    /** The following parameters define state machine behavior.
        If enabled, the switch will re-authenticate after the interval specified by reauth_timer.*/
    mesa_bool_t reauth_enabled;

    /** If .reauth_enabled, this specifies the period between client
        re-authentications in seconds.*/
    uint16_t reauth_period_secs;

    /** If the supplicant doesn't reply before this timeout, the switch
        retransmits EAPOL Request Identity packets.*/
    uint16_t eapol_timeout_secs;

    /** The following parameters define the amount of time that
        an authenticated MAC address will reside in the MAC table
        before it will be aged out if there's no traffic.*/
    mesa_bool_t psec_aging_enabled;

    /** If .psec_aging_enabled, this specifies the aging period in seconds.
        At \@psec_aging_period_secs, the CPU starts listening to frames from the
        given MAC address, and if none arrives before \@psec_aging_period_secs, the
        entry will be removed.*/
    uint32_t psec_aging_period_secs;

    /** The following parameters define the amount of time that
        a MAC address resides in the MAC table if authentication failed.*/
    mesa_bool_t psec_hold_enabled;

    /** If .psec_hold_enabled, this specifies the amount of time in seconds
        before a client whose authentication failed gets removed.*/
    uint32_t psec_hold_time_secs;

    /** Set to TRUE if RADIUS-assigned QoS is globally enabled.*/
    mesa_bool_t qos_backend_assignment_enabled;

    /** Set to TRUE if RADIUS-assigned VLAN is globally enabled.*/
    mesa_bool_t vlan_backend_assignment_enabled;

    /** Set to TRUE if Guest VLAN is globally enabled.*/
    mesa_bool_t guest_vlan_enabled;

    /** This is the guest VLAN.*/
    mesa_vid_t guest_vid;

    /** This is the number of times that the switch sends
        Request Identity EAPOL frames to the supplicant before
        restarting the authentication process. And if Guest VLAN
        is enabled, the supplicant will end up on this if no
        EAPOL frames were received in the meanwhile.*/
    uint32_t reauth_max;

    /** If the following is TRUE, then the supplicant is allowed into
        the Guest VLAN even if an EAPOL frame is seen for the lifetime
        of the port.
        If FALSE, then if just one EAPOL frame is seen on the port,
        then it will never get moved into the Guest VLAN.
        Note that the reception of an EAPOL frame *after* the port
        has been moved into Guest VLAN will take it out of Guest
        VLAN mode.
        Default is FALSE.*/
    mesa_bool_t guest_vlan_allow_eapols;
} vtss_appl_glbl_cfg_t;

/******************************************************************************/
// MAX and MIN values
/******************************************************************************/
#define VTSS_APPL_NAS_REAUTH_PERIOD_SECS_MIN         1 /**< Minimum time for re-authentication period in seconds.*/
#define VTSS_APPL_NAS_REAUTH_PERIOD_SECS_MAX      3600 /**< Maximum time for re-authentication period in seconds.*/

#define VTSS_APPL_NAS_EAPOL_TIMEOUT_SECS_MIN         1 /**< Minimum period time for EAPOL timeout in seconds.*/
#define VTSS_APPL_NAS_EAPOL_TIMEOUT_SECS_MAX     65535 /**< Maximum period time for EAPOL timeout in seconds.*/

#define VTSS_APPL_NAS_PSEC_AGING_PERIOD_SECS_MIN      10 /**< Minimum period time for PSEC aging in seconds.*/
#define VTSS_APPL_NAS_PSEC_AGING_PERIOD_SECS_MAX 1000000 /**< Maximum period time for PSEC aging in seconds.*/

#define VTSS_APPL_NAS_PSEC_HOLD_TIME_SECS_MIN            10 /**< Minimum period time for PSEC failure hold in seconds.*/
#define VTSS_APPL_NAS_PSEC_HOLD_TIME_SECS_MAX       1000000 /**< Maximum period time for PSEC failure hold in seconds.*/

#define VTSS_APPL_NAS_REAUTH_MIN                    1       /**< Minimum numbers of re-authentications */
#define VTSS_APPL_NAS_REAUTH_MAX                    255     /**< Maximum numbers of re-authentications */
/******************************************************************************/
// Supplicant/Client Info
/******************************************************************************/

#define VTSS_APPL_NAS_SUPPLICANT_ID_MAX_LENGTH (40) /**< Maximum length of identity string. */

/** \brief Supplicant/Client Info. */
typedef struct {
    /**
     * VLAN ID and binary version of mac_addr_str.
     */
    mesa_vid_mac_t vid_mac;

    /**
     * Identity string.
     */
    char           identity[VTSS_APPL_NAS_SUPPLICANT_ID_MAX_LENGTH];

    /**
     * Uptime of switch in seconds when this SM was created.
     */
    uint32_t            rel_creation_time_secs;

    /**
     * Absolute Uptime of switch, when it got authenticated
     * (successfully as well as unsuccessfully).
     */
    char           abs_auth_time[VTSS_APPL_RFC3339_TIME_STR_LEN];
} vtss_appl_nas_client_info_t;

/******************************************************************************/
// nas_eapol_counters_t
/******************************************************************************/
typedef uint32_t vtss_appl_nas_counter_t; /**< NAS counters type.*/

/** \brief NAS Eapol Counters.*/
typedef struct {
    vtss_appl_nas_counter_t dot1xAuthEapolFramesRx;          /**< Numbers of dot1x Auth Eapol Frames Received.           */
    vtss_appl_nas_counter_t dot1xAuthEapolFramesTx;          /**< Numbers of dot1x Auth Eapol Frames Transmitted.        */
    vtss_appl_nas_counter_t dot1xAuthEapolStartFramesRx;     /**< Numbers of dot1x Auth Eapol Start Frames Received.     */
    vtss_appl_nas_counter_t dot1xAuthEapolLogoffFramesRx;    /**< Numbers of dot1x Auth Eapol Logoff Frames Received.    */
    vtss_appl_nas_counter_t dot1xAuthEapolRespIdFramesRx;    /**< Numbers of dot1x Auth Eapol RespId Frames Received.    */
    vtss_appl_nas_counter_t dot1xAuthEapolRespFramesRx;      /**< Numbers of dot1x Auth Eapol Resp Frames Received.      */
    vtss_appl_nas_counter_t dot1xAuthEapolReqIdFramesTx;     /**< Numbers of dot1x Auth Eapol Req Id Frames Transmitted. */
    vtss_appl_nas_counter_t dot1xAuthEapolReqFramesTx;       /**< Numbers of dot1x Auth Eapol Req Frames Transmitted.    */
    vtss_appl_nas_counter_t dot1xAuthInvalidEapolFramesRx;   /**< Numbers of dot1x Auth Invalid Eapol Frames Received.   */
    vtss_appl_nas_counter_t dot1xAuthEapLengthErrorFramesRx; /**< Numbers of dot1x Auth Eap Length Error Frames Received.*/
} vtss_appl_nas_eapol_counters_t;

/******************************************************************************/
// vtss_appl_nas_backend_counters_t
/******************************************************************************/
/** \brief NAS backend Counters.*/
typedef struct {
    vtss_appl_nas_counter_t backendResponses;                 /**< Numbers of backend Responses.                   */
    vtss_appl_nas_counter_t backendAccessChallenges;          /**< Numbers of backend Access Challenges.           */
    vtss_appl_nas_counter_t backendOtherRequestsToSupplicant; /**< Numbers of backend Other Requests To Supplicant.*/
    vtss_appl_nas_counter_t backendAuthSuccesses;             /**< Numbers of backend Auth Successes.              */
    vtss_appl_nas_counter_t backendAuthFails;                 /**< Numbers of backend Auth Fails.                  */
} vtss_appl_nas_backend_counters_t;

/** \brief NAS VLAN types.*/
typedef enum {
    VTSS_APPL_NAS_VLAN_TYPE_NONE,             /**< No VLAN type.     */
    VTSS_APPL_NAS_VLAN_TYPE_BACKEND_ASSIGNED, /**< Backend VLAN type.*/
    VTSS_APPL_NAS_VLAN_TYPE_GUEST_VLAN        /**< Guest VLAN type.  */
} vtss_appl_nas_vlan_type_t;

/******************************************************************************/
// State of ports.
/******************************************************************************/
/** \brief NAS QoS Class type.*/
typedef enum {
    VTSS_APPL_NAS_QOS_CLASS_0    = 0, /**< Lowest         -  QoS priority 0 */
    VTSS_APPL_NAS_QOS_CLASS_1    = 1, /**< Lower          -  QoS priority 1 */
    VTSS_APPL_NAS_QOS_CLASS_2    = 2, /**< Low            -  QoS priority 2 */
    VTSS_APPL_NAS_QOS_CLASS_3    = 3, /**< Normal         -  QoS priority 3 */
    VTSS_APPL_NAS_QOS_CLASS_4    = 4, /**< Medium         -  QoS priority 4 */
    VTSS_APPL_NAS_QOS_CLASS_5    = 5, /**< High           -  QoS priority 5 */
    VTSS_APPL_NAS_QOS_CLASS_6    = 6, /**< Higher         -  QoS priority 6 */
    VTSS_APPL_NAS_QOS_CLASS_7    = 7, /**< Highest        -  QoS priority 7 */
    VTSS_APPL_NAS_QOS_CLASS_NONE = 0xFFFF /**< None/Undefined -  QoS priority Not defined*/
} vtss_appl_nas_qos_class_t;

/** \brief NAS Status.*/
typedef struct {
    vtss_appl_nas_port_status_t  status;     /**< NAS port status.                                     */
    mesa_prio_t                  qos_class;  /**< NAS QoS status. VTSS_PRIO_NO_NONE if unassigned      */
    vtss_appl_nas_vlan_type_t    vlan_type;  /**< NAS VLAN TYPE status.                                */
    mesa_vid_t                   vid;        /**< NAS VLAN status. 0 if unassigned.                    */
    uint32_t                     auth_cnt;   /**< In multi-client mode, number of authorized clients   */
    uint32_t                     unauth_cnt; /**< In multi-client mode, number of unauthorized clients */
} vtss_appl_nas_interface_status_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NAS management functions
 */

/**
 * \brief Get NAS global configuration.
 *
 * \param glbl_cfg [OUT] The global configuration.
 *
 * \return VTSS_RC_OK if glbl_cfg is valid, else error code.
 */
mesa_rc vtss_appl_nas_glbl_cfg_get(vtss_appl_glbl_cfg_t *const glbl_cfg);

/**
 * \brief Set NAS global configuration.
 *
 * \param glbl_cfg [IN] The new global configuration.
 *
 * \return VTSS_RC_OK if configuration went well, else error code.
 */
mesa_rc vtss_appl_nas_glbl_cfg_set(const vtss_appl_glbl_cfg_t *const glbl_cfg);

/**
 * \brief Get NAS configuration for a specific interface.
 *
 * \param ifindex [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \param port_cfg [OUT] The port configuration.
 *
 * \return VTSS_RC_OK if port_cfg is valid, else error code.
 */
mesa_rc vtss_appl_nas_port_cfg_get(vtss_ifindex_t ifindex, vtss_appl_nas_port_cfg_t *const port_cfg);

/**
 * \brief Set NAS configuration for a specific interface.
 *
 * \param ifindex [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \param port_cfg [IN] The new port configuration.
 *
 * \return VTSS_RC_OK if configuration went well, else error code.
 */
mesa_rc vtss_appl_nas_port_cfg_set(vtss_ifindex_t ifindex, const vtss_appl_nas_port_cfg_t *const port_cfg);

/**
 * \brief Get NAS status for a specific interface.
 *
 * \param ifindex [IN]  interface index.
 *
 * \param status [OUT] The port status.
 *
 * \return VTSS_RC_OK if successful, else error code.
 */
mesa_rc vtss_appl_nas_port_status_get(vtss_ifindex_t ifindex, vtss_appl_nas_interface_status_t *const status);

/**
 * \brief Get NAS EAPOL statistics for a specific interface
 *
 * \param ifindex [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \param stats [OUT] The dot1x EAPOL statistics counters for this interface.
 *
 * \return VTSS_RC_OK if statistics is valid, else error code.
 */
mesa_rc vtss_appl_nas_eapol_statistics_get(vtss_ifindex_t ifindex, vtss_appl_nas_eapol_counters_t *const stats);

/**
 * \brief Get NAS RADIUS statistics for a specific interface
 *
 * \param ifindex [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \param stats [OUT] The dot1x RADIUS statistics counters for this interface.
 *
 * \return VTSS_RC_OK if statistics is valid, else error code.
 */
mesa_rc vtss_appl_nas_radius_statistics_get(vtss_ifindex_t ifindex, vtss_appl_nas_backend_counters_t *const stats);

/**
 * \brief Clear NAS statistics for a specific interface.
 *
 * \param ifindex [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \return VTSS_RC_OK if clear was done correctly, else error code.
 */
mesa_rc vtss_appl_nas_port_statistics_clear(vtss_ifindex_t ifindex);

/**
 * \brief Get NAS last supplicant for a specific interface
 *
 * \param ifindex              [IN]  Interface index - the logical interface index of the physical switch port.
 *
 * \param last_supplicant_info [OUT] Information for the last supplicant.
 *
 * \return VTSS_RC_OK if last_supplicant_info is valid, else error code.
 */
mesa_rc vtss_appl_nas_last_supplicant_info_get(
    vtss_ifindex_t                      ifindex,
    vtss_appl_nas_client_info_t *const  last_supplicant_info
);

/**
 * \brief Start NAS re-authorization for a specific interface.
 *
 * \param ifindex [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \param reinit  [IN] TRUE to force re-initialize authentication.
                       FALSE to re-authenticate already authenticated clients.
 *
 * \return VTSS_RC_OK if successful, else error code.
 */
mesa_rc vtss_appl_nas_port_reinitialize(vtss_ifindex_t ifindex, const mesa_bool_t *reinit);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_NAS_H_ */

