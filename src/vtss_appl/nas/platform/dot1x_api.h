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

#ifndef _DOT1X_API_H_
#define _DOT1X_API_H_

#include "main.h"
#include "vtss/appl/nas.h"     // For NAS public types
#include "vtss_nas_api.h"  /* For nax_XXX, NAS_USES_PSEC, NAS_MULTI_CLIENT, etc. */
#include "main_types.h"    /* For uXXX, iXXX, and BOOL                           */

#ifdef VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
#include "qos_api.h"     /* For mesa_prio_t                                    */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
// Various defines of min/max/defaults
/****************************************************************************/
#define DOT1X_REAUTH_PERIOD_SECS_DEFAULT  3600
#define DOT1X_EAPOL_TIMEOUT_SECS_DEFAULT    30

#ifdef NAS_USES_PSEC
#define NAS_PSEC_AGING_ENABLED_DEFAULT       TRUE /* NO SUPPORT IN CLI OR WEB FOR CHANGING IT, SO BE CAREFUL IF CHANGING DEFAULT */
#define NAS_PSEC_AGING_PERIOD_SECS_DEFAULT    300

#define NAS_PSEC_HOLD_ENABLED_DEFAULT        TRUE /* NO SUPPORT IN CLI OR WEB FOR CHANGING IT, SO BE CAREFUL IF CHANGING DEFAULT */
#define NAS_PSEC_HOLD_TIME_SECS_DEFAULT        10
#endif

typedef struct {
    CapArray<vtss_appl_nas_port_cfg_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_cfg; /**< Administrative state.*/
} vtss_nas_switch_cfg_t;

typedef struct {
    CapArray<vtss_appl_nas_port_status_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> status;       /**<NAS port status.      */
    CapArray<u32, MEBA_CAP_BOARD_PORT_MAP_COUNT> auth_cnt;                             /**<Authorized count (in multi-client mode only)   */
    CapArray<u32, MEBA_CAP_BOARD_PORT_MAP_COUNT> unauth_cnt;                           /**<Unauthorized count (in multi-client mode only) */
    CapArray<vtss_appl_nas_port_control_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> admin_state; /**<NAS admin status.     */
    CapArray<mesa_prio_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> qos_class;                    /**<NAS QOS status.       */
    CapArray<vtss_appl_nas_vlan_type_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> vlan_type;      /**<NAS VLAN TYPE status. */
    CapArray<mesa_vid_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> vid;                           /**<NAS VLAN status.      */
} vtss_nas_switch_status_t;

/** \brief Supplicant/Client Info. */
typedef struct {
    mesa_vid_mac_t vid_mac;          /**< VLAN ID and binary version of mac_addr_str.*/
    char           mac_addr_str[18]; /**< Console-presentable string (e.g. "AA-BB-CC-DD-EE-FF").*/
    char           identity[VTSS_APPL_NAS_SUPPLICANT_ID_MAX_LENGTH]; /**< Identity string.*/
    time_t         rel_creation_time_secs; /**< Uptime of switch in seconds when this SM was created.*/
    time_t         rel_auth_time_secs;     /**< Uptime of switch in seconds when it got authenticated (successfully as well as unsuccessfully).*/
} vtss_nas_client_info_t;

/** \brief NAS State of a given client on a multi-client port. */
typedef struct {
    vtss_nas_client_info_t client_info; /**< Client information.*/
    vtss_appl_nas_port_status_t status;      /**< Interface status.*/
} vtss_nas_multi_client_status_t;

/** \brief NAS Eapol Counters.*/
typedef struct {
    vtss_appl_nas_counter_t authEntersConnecting;                 /**< Numbers of Enters connecting.                   */
    vtss_appl_nas_counter_t authEapLogoffsWhileConnecting;        /**< Numbers of Eap Logoff while connecting.         */
    vtss_appl_nas_counter_t authEntersAuthenticating;             /**< Numbers of Enters Authenticating.               */
    vtss_appl_nas_counter_t authAuthSuccessesWhileAuthenticating; /**< Numbers of Successes While Authenticating.      */
    vtss_appl_nas_counter_t authAuthTimeoutsWhileAuthenticating;  /**< Numbers of Auth Timeouts While Authenticating.  */
    vtss_appl_nas_counter_t authAuthFailWhileAuthenticating;      /**< Numbers of Auth Fail While Authenticating.      */
    vtss_appl_nas_counter_t authAuthEapStartsWhileAuthenticating; /**< Numbers of Auth Eap Starts While Authenticating.*/
    vtss_appl_nas_counter_t authAuthEapLogoffWhileAuthenticating; /**< Numbers of Auth Eap Logoff While Authenticating.*/
    vtss_appl_nas_counter_t authAuthReauthsWhileAuthenticated;    /**< Numbers of Auth Reauths While Authenticated.    */
    vtss_appl_nas_counter_t authAuthEapStartsWhileAuthenticated;  /**< Numbers of Auth Eap Starts While Authenticated. */
    vtss_appl_nas_counter_t authAuthEapLogoffWhileAuthenticated;  /**< Numbers of Auth Eap Logoff While Authenticated. */

    // Authenticator Statistics Table
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
    vtss_appl_nas_counter_t dot1xAuthLastEapolFrameVersion;  /**< Numbers of dot1x Auth Last Eapol Frame Version.        */
} vtss_nas_eapol_counters_t;

/** \brief NAS Statistics Counters.*/
typedef struct {
    /** If top-level SM: Last client info
        If sub-SM: Current client_info.*/
    vtss_nas_client_info_t      client_info;

    /** Only valid for BPDU-based admin_states.*/
    vtss_nas_eapol_counters_t   eapol_counters;

    /** Only valid for admin_states using a backend server.*/
    vtss_appl_nas_backend_counters_t backend_counters;

    /** The status is encoded as follows:
        If top-level SM:
        0 = Link down, 1 = Authorized, 2 = Unauthorized, 3 = NAS globally disabled, 4 = multi-client.
        In case of multi-client, #auth_cnt and #unauth_cnt tells the number of (un)authorized clients.
        Of sub-SM:
        1 = Authorized, 2 = Unauthorized.*/
    vtss_appl_nas_port_status_t status;

    /** The QoS class that this port is assigned to by the backend server.
        VTSS_PRIO_NO_NONE if unassigned.*/
    mesa_prio_t            qos_class;

    /** The VLAN that this port is assigned to and the reason (RADIUS- or Guest VLAN).
        0 if unassigned.*/
    mesa_vid_t             vid;

    vtss_appl_nas_vlan_type_t        vlan_type; /**< VLAN type*/

    /** This one saves a call to the management functions, since the caller
        sometimes needs to know the administrative state of the port in
        order to tell which counters are actually active.*/
    vtss_appl_nas_port_control_t     admin_state;

    /**
     * In multi-client mode and only valid in top-level SM:
     * Number of authorized clients on this port.
     */
    u32 auth_cnt;

    /**
     * In multi-client mode and only valid in top-level SM:
     * Number of unauthorized clients on this port.
     */
    u32 unauth_cnt;
} vtss_nas_statistics_t;

/******************************************************************************/
// dot1x_error_txt()
/******************************************************************************/
const char *dot1x_error_txt(mesa_rc rc);

/******************************************************************************/
// dot1x_port_control_to_str()
/******************************************************************************/
const char *dot1x_port_control_to_str(vtss_appl_nas_port_control_t port_control, BOOL brief);

/******************************************************************************/
// dot1x_qos_class_to_str()
// Helper function to create a useful RADIUS-assigned QoS string.
// It'll become empty if the option is not supported.
// String must be at least 20 chars long.
/******************************************************************************/
void dot1x_qos_class_to_str(mesa_prio_t iprio, char *str);

/******************************************************************************/
// dot1x_vlan_to_str()
// Helper function to create a useful VLAN string.
// It'll become empty if the option is not supported.
// String must be at least 20 chars long.
/******************************************************************************/
void dot1x_vlan_to_str(mesa_vid_t vid, char *str);

/******************************************************************************/
// dot1x_init()
// Initialize 802.1X Module
/******************************************************************************/
mesa_rc dot1x_init(vtss_init_data_t *data);

/******************************************************************************/
// DOT1X_cfg_default_glbl()
// Initialize global settings to defaults.
/******************************************************************************/
void DOT1X_cfg_default_glbl(vtss_appl_glbl_cfg_t *cfg);

/******************************************************************************/
// DOT1X_cfg_default_switch()
// Initialize per-switch settings to defaults.
/******************************************************************************/
void DOT1X_cfg_default_switch(vtss_nas_switch_cfg_t *cfg);

/**
 * \brief Get NAS configuration for a specific switch.
 *
 * \param sid        [IN]  The switch ID of the switch to get configuration from.
 *
 * \param switch_cfg [OUT] The switch configuration.
 *
 * \return VTSS_RC_OK if switch_cfg is valid, else error code.
 */
mesa_rc vtss_nas_switch_cfg_get(vtss_usid_t sid, vtss_nas_switch_cfg_t *switch_cfg);

/**
 * \brief Set NAS configuration for a specific switch.
 *
 * \param sid        [IN] Switch ID for the switch to configure.
 *
 * \param switch_cfg [IN] The new switch configuration.
 *
 * \return VTSS_RC_OK if configuration went well, else error code.
 */
mesa_rc vtss_nas_switch_cfg_set(vtss_usid_t sid, vtss_nas_switch_cfg_t *switch_cfg);

/**
 * \brief Get NAS status for a specific switch.
 *
 * \param sid           [IN]  Switch ID for the switch in question.
 *
 * \param switch_status [OUT] The switch status.
 *
 * \return VTSS_RC_OK if switch_status is valid, else error code.
 */
mesa_rc vtss_nas_switch_status_get(vtss_usid_t sid, vtss_nas_switch_status_t *switch_status);

/**
 * \brief Get multi client statistics for a specific interface.
 *
 * \param ifindex        [IN]  Interface index - the logical interface index of the physical switch port.
 *
 * \param statistics     [INOUT]  The statistics counters.
 *
 * \param start_all_over [IN]  TRUE to start with the first client. FALSE to start with next client found after the client
 *                             with .client_info.vid_mac given with the statistics input parameter.
 *
 * \param found          [OUT] TRUE if statistics parameter contains valid information.
 *
 * \return VTSS_RC_OK if statistics is valid (found must be TRUE as well), else error code.
 */
mesa_rc vtss_nas_multi_client_statistics_get(vtss_ifindex_t ifindex, vtss_nas_statistics_t *statistics, BOOL *found, BOOL start_all_over);

/**
 * \brief Get multi client status for a specific interface.
 *
 * \param ifindex        [IN]  Interface index - the logical interface index of the physical switch port.
 *
 * \param found          [OUT] TRUE if client status were found.
 *
 * \param client_status  [INOUT] The client status.
 *
 * \param start_all_over [IN]  TRUE to start with the first client. FALSE to start with next client found after the client
 *                             with .client_info.vid_mac given with the client_status input parameter.
 *
 * \return VTSS_RC_OK if client_status is valid (found must be TRUE as well), else error code.
 */
mesa_rc vtss_nas_multi_client_status_get(vtss_ifindex_t ifindex, vtss_nas_multi_client_status_t *client_status, BOOL *found, BOOL start_all_over);

/**
 * \brief Get NAS statistics for a specific interface
 *
 * \param ifindex    [IN]  Interface index - the logical interface index of the physical switch port.
 *
 * \param vid_mac    [IN]  \@vid_mac == NULL or vid_mac->vid == 0, get the interface statistics,
 *                         otherwise get the statistics given by \@vid_mac.
 *
 * \param statistics [OUT] The interface statistics.
 *
 * \return VTSS_RC_OK if statistics is valid, else error code.
 */
mesa_rc vtss_nas_statistics_get(vtss_ifindex_t ifindex, mesa_vid_mac_t *vid_mac, vtss_nas_statistics_t *statistics);

/**
 * \brief Get NAS statistics for a specific interface.
 *
 * \param vid_mac    [IN]  \@vid_mac == NULL or vid_mac->vid == 0, clear everything on that port,
 *                         otherwise only clear entry given by \@vid_mac.
 *
 * \param ifindex    [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \return VTSS_RC_OK if clear was done correctly, else error code.
 */
mesa_rc vtss_nas_statistics_clear(vtss_ifindex_t ifindex, mesa_vid_mac_t *vid_mac);

/**
 * \brief Start NAS re-authorization for a specific interface.
 *
 * \param ifindex    [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \param now        [IN] TRUE to force re-authentication immediately. FALSE to refresh (restart) 802.1X authentication process.
 *
 * \return VTSS_RC_OK if re-authentication were preformed correctly, else error code.
 */
mesa_rc vtss_nas_reauth(vtss_ifindex_t ifindex, const BOOL now);

/**
 * \brief Get last supplicant MAC address as a printable string.
 *
 * \param ifindex      [IN] Interface index - the logical interface index of the physical switch port.
 *
 * \param mac_addr_str [OUT] Pointer to where to put the string result.
 *
 * \return VTSS_RC_OK if mac_addr_str is valid, else error code.
 */
mesa_rc nas_last_supplicant_info_mac_addr_as_str_get(vtss_ifindex_t ifindex, char *mac_addr_str);
#ifdef __cplusplus
}
#endif
#endif /* _DOT1X_API_H_ */

