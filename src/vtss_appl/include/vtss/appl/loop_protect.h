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
 * \brief Public Loop Protection API
 * \details This header file describes Loop Protection control functions and types
 */

#ifndef _VTSS_APPL_LOOP_PROTECT_H_
#define _VTSS_APPL_LOOP_PROTECT_H_

#include <vtss/appl/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default value for global enable */
#define VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_ENABLED         FALSE
/** Default value for global transmisssion interval */
#define VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_TX_TIME         5
/** Default value for global shutdown interval */
#define VTSS_APPL_LOOP_PROTECT_DEFAULT_GLOBAL_SHUTDOWN_TIME   180
/** Default value for port enable */
#define VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_ENABLED           TRUE
/** Default value for port action */
#define VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_ACTION            VTSS_APPL_LOOP_PROTECT_ACTION_SHUTDOWN
/** Default value for port transmit mode */
#define VTSS_APPL_LOOP_PROTECT_DEFAULT_PORT_TX_MODE           TRUE

/**
 * Action for a loop condition
 */
typedef enum {
    VTSS_APPL_LOOP_PROTECT_ACTION_SHUTDOWN,  /**< Shutdown port */
    VTSS_APPL_LOOP_PROTECT_ACTION_SHUT_LOG,  /**< Shutdown port and log event */
    VTSS_APPL_LOOP_PROTECT_ACTION_LOG_ONLY,  /**< Log event (only) */
} vtss_appl_loop_protect_action_t;

/**
 * Port parameters.
 */
typedef struct {
    mesa_bool_t                            enabled;  /**< Enabled loop protection on port */
    vtss_appl_loop_protect_action_t action;   /**< Action if loop detected */
    mesa_bool_t                            transmit; /**< Actively generate PDUs */
} vtss_appl_loop_protect_port_conf_t;

/**
 * Global parameters.
 */
typedef struct {
    mesa_bool_t enabled;           /**< Global loop protection enabling */
    uint32_t  transmission_time; /**< Port transmission interval (seconds). Valid range: 1-10 seconds. */
    uint32_t  shutdown_time;     /**< Port shutdown period (seconds). Valid range: 0 to 604800 seconds. */
} vtss_appl_loop_protect_conf_t;

/**
 * Set Loop Protection Global Parameters
 *
 * \param conf [IN] The global configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_loop_protect_conf_set(const vtss_appl_loop_protect_conf_t *conf);

/**
 * Get Loop Protection Global Parameters
 *
 * \param conf [OUT] The global configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_loop_protect_conf_get(vtss_appl_loop_protect_conf_t *conf);

/**
 * Set Loop Protection Port Parameters
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param conf    [IN] The port configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_loop_protect_conf_port_set(vtss_isid_t isid, 
                                             mesa_port_no_t port_no, 
                                             const vtss_appl_loop_protect_port_conf_t *conf);

/**
 * Get Loop Protection Port Parameters
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param conf    [OUT] The port configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_loop_protect_conf_port_get(vtss_isid_t isid, 
                                             mesa_port_no_t port_no, 
                                             vtss_appl_loop_protect_port_conf_t *conf);

/**
 * Port information structure
 */
typedef struct {
    mesa_bool_t   disabled;    /**< Whether a port is currently disabled  */
    mesa_bool_t   loop_detect; /**< Whether a port has a loop detected  */
    uint32_t    loops;       /**< Number of times a loop has been detected on a port */
    uint64_t    last_loop;   /**< Time of last loop condition. */
} vtss_appl_loop_protect_port_info_t;

/**
 * Get Loop Protection Port State Information
 *
 * \param isid    [IN] Internal switch id
 *
 * \param port_no [IN] Internal port id
 *
 * \param info    [OUT] The port information data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_loop_protect_port_info_get(vtss_isid_t isid, 
                                             mesa_port_no_t port_no, 
                                             vtss_appl_loop_protect_port_info_t *info);

#ifdef __cplusplus
}
#endif
#endif  /* _VTSS_APPL_LOOP_PROTECT_H_ */
