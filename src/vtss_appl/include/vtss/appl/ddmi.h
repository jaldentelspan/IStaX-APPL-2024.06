/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief Public API of DDMI
 * \details DDMI is an acronym for Digital Diagnostics Monitoring Interface.
 * It provides an enhanced digital diagnostic monitoring interface for optical
 * transceivers which allows real time access to device operating parameters.
 */

#ifndef _VTSS_APPL_DDMI_H_
#define _VTSS_APPL_DDMI_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <microchip/ethernet/board/api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum string length including terminating '\0'
 */
#define VTSS_APPL_DDMI_STR_LEN_MAX 17

/**
 * Global configuration
 */
typedef struct {
    /**
     * Enable/disable DDMI function
     */
    mesa_bool_t admin_enable;
} vtss_appl_ddmi_global_conf_t;

/**
 * Get global default configuration.
 *
 * \param conf [OUT] Global default configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ddmi_global_conf_default_get(vtss_appl_ddmi_global_conf_t *conf);

/**
 * Get global configuration.
 *
 * \param conf [OUT] Global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ddmi_global_conf_get(vtss_appl_ddmi_global_conf_t *conf);

/**
 * Set global configuration.
 *
 * \param conf [IN] Global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ddmi_global_conf_set(const vtss_appl_ddmi_global_conf_t *conf);

/**
 * DDMI monitor type.
 */
typedef enum {
    VTSS_APPL_DDMI_MONITOR_TYPE_TEMPERATURE, /**< Temperature    */
    VTSS_APPL_DDMI_MONITOR_TYPE_VOLTAGE,     /**< Voltage        */
    VTSS_APPL_DDMI_MONITOR_TYPE_TX_BIAS,     /**< Tx Bias        */
    VTSS_APPL_DDMI_MONITOR_TYPE_TX_POWER,    /**< Tx Power       */
    VTSS_APPL_DDMI_MONITOR_TYPE_RX_POWER,    /**< Rx Power       */
    VTSS_APPL_DDMI_MONITOR_TYPE_CNT          /**< Must come last */
} vtss_appl_ddmi_monitor_type_t;

/**
 * DDMI monitor state (none/alarm/warning)
 */
typedef enum {
    VTSS_APPL_DDMI_MONITOR_STATE_NONE,     /**< No warning or alarm is present                          */
    VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO,  /**< Current value is smaller than the low warning threshold */
    VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI,  /**< Current value is greater than the hi  warning threshold */
    VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO, /**< Current value is smaller than the low alarm   threshold */
    VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI, /**< Current value is greater than the hi  alarm   threshold */
    VTSS_APPL_DDMI_MONITOR_STATE_CNT       /**< Must come last                                          */
} vtss_appl_ddmi_monitor_state_t;

/**
 * DDMI feature status
 */
typedef struct {
    /**
     * Indicates whether a warning or alarm is present or not for this feature.
     */
    vtss_appl_ddmi_monitor_state_t state;

    /**
     * Indicates the current value of the feature.
     */
    char current[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * Indicates the value that will cause a warning if current drops below it.
     */
    char warn_lo[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * Indicates the value that will cause a warning if current exceeds it.
     */
    char warn_hi[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * Indicates the value that will cause an alarm if current drops below it.
     */
    char alarm_lo[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * Indicates the value that will cause an alarm if current exceeds it.
     */
    char alarm_hi[VTSS_APPL_DDMI_STR_LEN_MAX];
} vtss_appl_ddmi_monitor_status_t;

/**
 * DDMI port status
 */
typedef struct {
    /**
     * Indicates whether this port is an SFP port and that it supports SFP
     * insertion detection.
     */
    mesa_bool_t a0_supported;

    /**
     * Indicates whether an SFP is inserted or not.
     */
    mesa_bool_t sfp_detected;

    /**
     * Vendor name as read from SFP ROM.
     * Only valid if a0_supported and sfp_detected are true.
     */
    char vendor[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * Part number as read from SFP ROM.
     * Only valid if a0_supported and sfp_detected are true.
     */
    char part_number[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * Serial number as read from SFP ROM.
     * Only valid if a0_supported and sfp_detected are true.
     */
    char serial_number[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * Revision as read from SFP ROM.
     * Only valid if a0_supported and sfp_detected are true.
     */
    char revision[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * Date code as read from SFP ROM.
     * Only valid if a0_supported and sfp_detected are true.
     */
    char date_code[VTSS_APPL_DDMI_STR_LEN_MAX];

    /**
     * SFP transceiver type as read from SFP ROM.
     * Only valid if a0_supported and sfp_detected are true.
     */
    meba_sfp_transreceiver_t sfp_type;

    /**
     * Indicates wether DDMI status information (A2) is supported or not
     * Only valid if a0_supported and sfp_detected are true.
     */
    mesa_bool_t a2_supported;

    /**
     * If a2_supported, this one holds status about the individual monitor
     * types.
     * The units are:
     *  - VTSS_APPL_DDMI_MONITOR_TYPE_TEMPERATURE: Degrees Celsius
     *  - VTSS_APPL_DDMI_MONITOR_TYPE_VOLTAGE:     Volts
     *  - VTSS_APPL_DDMI_MONITOR_TYPE_TX_BIAS:     mA
     *  - VTSS_APPL_DDMI_MONITOR_TYPE_TX_POWER:    mW
     *  - VTSS_APPL_DDMI_MONITOR_TYPE_RX_POWER:    mW
     */
    vtss_appl_ddmi_monitor_status_t monitor_status[VTSS_APPL_DDMI_MONITOR_TYPE_CNT];
} vtss_appl_ddmi_port_status_t;

/**
 * Get port status.
 *
 * \param ifindex [IN]  Interface index of the port to get status for
 * \param status  [OUT] Port status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ddmi_port_status_get(vtss_ifindex_t ifindex, vtss_appl_ddmi_port_status_t *status);

/**
 * Notifications are retrieved with this key.
 */
typedef struct {
    /**
     * Port Interface index
     */
    vtss_ifindex_t ifindex;

    /**
     * Type
     */
    vtss_appl_ddmi_monitor_type_t type;
} vtss_appl_ddmi_notification_status_key_t;

/**
 * Specialized operator< for vtss_appl_ddmi_notification_status_key_t in order
 * to be able to use it as a key in a vtss::Map (useful for the notification
 * serializer, for instance).
 *
 * \param lhs [IN] left-hand-side of operator <
 * \param rhs [IN] right-hand-side of operator <
 *
 * \return true if lhs < rhs, false otherwise.
 */
bool operator<(const vtss_appl_ddmi_notification_status_key_t &lhs, const vtss_appl_ddmi_notification_status_key_t &rhs);

/**
 * Get notification status of a particular monitor type for a particular port.
 *
 * \param key          [IN]  Key indicating which port and monitor type to get.
 * \param notif_status [OUT] Pointer to structure receiving notification status.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ddmi_notification_status_get(const vtss_appl_ddmi_notification_status_key_t *key, vtss_appl_ddmi_monitor_status_t *notif_status);

/**
 * Iterate across all DDMI monitor notifications.
 *
 * Only ports with SFPs that contain A2 information are returned.
 * To start iteration, set key.ifindex to 0 or key_prev to NULL.
 *
 * \param key_prev [IN]  Pointer to a value with the previous iteration's key_next values.
 * \param key_next [OUT] Pointer to the next key for which notification status can be obtained.
 *
 * \return VTSS_RC_OK as long as key_next contains valid values.
 * values.
 */
mesa_rc vtss_appl_ddmi_notification_status_itr(const vtss_appl_ddmi_notification_status_key_t *key_prev, vtss_appl_ddmi_notification_status_key_t *key_next);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_DDMI_H_ */
