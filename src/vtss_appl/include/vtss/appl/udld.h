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
 * \brief Public UDLD(Uni Directional Link Detection) API
 * \details This header file describes UDLD control functions and types
 */

#ifndef _VTSS_APPL_UDLD_H_
#define _VTSS_APPL_UDLD_H_

#include <vtss/appl/types.h>
#ifdef __cplusplus
extern "C" {
#endif

/** Default probe message interval*/
#define UDLD_DEFAULT_MESSAGE_INTERVAL       7
/** Length of device id, mac address used as device id */
#define MAX_DEVICE_ID_LENGTH                255
/** Length of port id, port number used as port id */
#define MAX_PORT_ID_LENGTH                  255
/** Length of device name, system name used as device name */
#define MAX_DEVICE_NAME_LENGTH              255
/**
 * \brief
 *     UDLD mode.
 */
typedef enum {
    VTSS_APPL_UDLD_MODE_DISABLE,        /**< In error case none mode */
    VTSS_APPL_UDLD_MODE_NORMAL,         /**< Normal Mode of the UDLD Operation */
    VTSS_APPL_UDLD_MODE_AGGRESSIVE      /**< Aggressive Mode of the UDLD Operation */
} vtss_appl_udld_mode_t;
/**
  *  \brief
  *      UDLD admin state.
 */
typedef enum {
    VTSS_APPL_UDLD_ADMIN_DISABLE,       /**< Disable admin state */
    VTSS_APPL_UDLD_ADMIN_ENABLE         /**< Enable admin state */
} vtss_appl_udld_admin_t;
/**
 *  \brief
 *      UDLD link state.
 */
typedef enum {
    VTSS_UDLD_DETECTION_STATE_UNKNOWN,             /**< Indeterminant link state */
    VTSS_UDLD_DETECTION_STATE_UNI_DIRECTIONAL,     /**< Uni directional link state */
    VTSS_UDLD_DETECTION_STATE_BI_DIRECTIONAL,      /**< Bi directional link state */
    VTSS_UDLD_DETECTION_STATE_NEIGHBOR_MISMATCH,   /**< Neighbor mismatch link state */
    VTSS_UDLD_DETECTION_STATE_LOOPBACK,            /**< Loopback link state */
    VTSS_UDLD_DETECTION_STATE_MULTIPLE_NEIGHBOR    /**< Multiple neighbor connected */
} vtss_udld_detection_state_t;
/**
 *  \brief
 *      UDLD protocol phase.
 */
typedef enum {
    VTSS_UDLD_PROTO_PHASE_LINK_UP,              /**< Protocol phase link up */
    VTSS_UDLD_PROTO_PHASE_ADV   ,               /**< Protocol phase adv */
    VTSS_UDLD_PROTO_PHASE_DETECTION             /**< Protocol phase detection */
} vtss_udld_proto_phase_t;
/**
 *  \brief
 *      UDLD port conf params.
 */
typedef struct {
    uint32_t                       probe_msg_interval;   /**< Probe msg interval */
    vtss_appl_udld_mode_t     udld_mode;            /**< UDLD none/normal/aggresive */
    vtss_appl_udld_admin_t    admin;                /**< UDLD admin enable/disable  */
} vtss_appl_udld_port_conf_struct_t;

/**
 *  \brief
 *      UDLD neighbor information.
 */
typedef struct {
    char                                 device_id[MAX_DEVICE_ID_LENGTH];      /**< Neighbor Device id */
    char                                 port_id[MAX_PORT_ID_LENGTH];          /**< Neighbor Port id */
    char                                 device_name[MAX_DEVICE_NAME_LENGTH];  /**< Neighbor Device name */
    vtss_udld_detection_state_t      detection_state;                      /**< Neighbor link state */
    struct udld_remote_cache_list_t  *next;                                /**< Next Neighbor*/
} vtss_appl_udld_neighbor_info_t;
/**
 *  \brief
 *      UDLD local port information.
 */
typedef struct vtss_appl_udld_port_info_struct_t {
    char                             device_id[MAX_DEVICE_ID_LENGTH];     /**< Device id */
    char                             port_id[MAX_PORT_ID_LENGTH];         /**< Port id */
    char                             device_name[MAX_DEVICE_NAME_LENGTH]; /**< Device name */
    char                             msg_interval;                        /**< Message interval */
    char                             timeout_interval;                    /**< Timeout interval */
    vtss_udld_proto_phase_t      proto_phase;                         /**< Protocol phase*/
    vtss_udld_detection_state_t  detection_state;                     /**< Port Detection state */
} vtss_appl_udld_port_info_t;

/*
   ==============================================================================

       Public APIs

   ==============================================================================
*/
/**
 * Get UDLD Port Parameters
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param conf    [OUT] The port configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_port_conf_get(vtss_isid_t isid, 
                                     mesa_port_no_t port_no, 
                                     vtss_appl_udld_port_conf_struct_t *conf);

/**
 * Set UDLD Port Parameters
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param conf    [IN] The port configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */

mesa_rc vtss_appl_udld_port_conf_set(vtss_isid_t isid, 
                                     mesa_port_no_t port_no, 
                                     const vtss_appl_udld_port_conf_struct_t *conf);
/**
 * Set UDLD Port Admin State 
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param admin    [IN] The port admin state
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_port_admin_set(vtss_isid_t isid, 
                                      mesa_port_no_t port_no,  
                                      vtss_appl_udld_admin_t admin);
/**
 * Get UDLD Port Admin State
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param admin   [OUT] The port admin state
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_port_admin_get(vtss_isid_t isid, 
                                      mesa_port_no_t port_no, 
                                      vtss_appl_udld_admin_t *admin);
/**
 * Set UDLD Port mode
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param mode    [IN] The port mode
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_port_mode_set(vtss_isid_t isid, 
                                     mesa_port_no_t port_no, 
                                     vtss_appl_udld_mode_t mode);
/**
 * Get UDLD Port mode
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param mode   [OUT] The port mode
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_port_mode_get(vtss_isid_t isid, 
                                     mesa_port_no_t port_no, 
                                     vtss_appl_udld_mode_t *mode);
/**
 * Set UDLD Port probe message interval
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param interval [IN] The port probe msg interval
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_port_probe_msg_interval_set(vtss_isid_t isid, 
                                                   mesa_port_no_t port_no, 
                                                   uint32_t interval);
/**
 * Get UDLD Port probe message interval
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param interval [OUT] The port probe msg interval
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_port_probe_msg_interval_get(vtss_isid_t isid, 
                                                   mesa_port_no_t port_no, 
                                                   uint32_t *interval);

/**
 * Get UDLD Port first neighbor information
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param info    [OUT] The port first neighbor information
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_neighbor_info_get_first(vtss_isid_t isid, 
                                               mesa_port_no_t port_no , 
                                               vtss_appl_udld_neighbor_info_t *info
                                               );
/**
 * Get UDLD Port next neighbor information
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param info    [OUT] The port next neighbor information
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_neighbor_info_get_next(vtss_isid_t isid, 
                                              mesa_port_no_t port_no , 
                                              vtss_appl_udld_neighbor_info_t *info
                                              );

/**
 * Get UDLD Port first neighbor information
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param info    [OUT] The port udld information
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_udld_port_info_get(vtss_isid_t isid,
                                               mesa_port_no_t port_no ,
                                               vtss_appl_udld_port_info_t *info
                                              );
#ifdef __cplusplus
}
#endif
#endif /* _VTSS_APPL_UDLD_H_ */
