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
 * \brief Public SysUtil API
 * \details This header file describes SysUtil control functions and types.
 */

#ifndef _VTSS_APPL_SYSUTIL_H_
#define _VTSS_APPL_SYSUTIL_H_

#include <vtss/appl/types.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif  // fix indent
//----------------------------------------------------------------------------

/** The maximum length of power supply description */
#define VTSS_APPL_SYSUTIL_PSU_DESCR_MAX_LEN         31

/** The maximum ID of power supply */
#define VTSS_APPL_SYSUTIL_PSU_MAX_ID                2

/** The maximum input length of host name */
#define VTSS_APPL_SYSUTIL_INPUT_HOSTNAME_LEN        45

/** The maximum length length of host name (plus one end-of-string character) */
#define VTSS_APPL_SYSUTIL_HOSTNAME_LEN              (VTSS_APPL_SYSUTIL_INPUT_HOSTNAME_LEN + 1)

/** The maximum input length of domain name */
#define VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN     253

/** The maximum length length of domain name (plus one end-of-string character) */
#define VTSS_APPL_SYSUTIL_DOMAIN_NAME_LEN           (VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1)

/** The maximum length of system LED description */
#define VTSS_APPL_SYSUTIL_SYSTEM_LED_DESCR_MAX_LEN  128

/** Length of the system uptime string */
#define VTSS_APPL_SYSUTIL_SYSTEM_UPTIME_LEN         11

/** Length of the board tags */
#define VTSS_APPL_SYSUTIL_TAG_LENGTH                64

/** Length of the Hostname */
#define VTSS_APPL_SYS_STRING_LEN                    256

/** The maximum length of system time  */
#define VTSS_APPL_SYSUTIL_SYSTEM_TIME_LEN           64

/** The defination of temperature monitor support */
#if defined(VTSS_SW_OPTION_TMP_43X_API)
#define VTSS_APPL_SYSUTIL_TM_SUPPORTED
#endif

/**
 * \brief SysUtil capabilities to indicate which is supported or not
 */
typedef struct {
    mesa_bool_t    warmReboot;     /*!< warm reboot supported or not  */
    mesa_bool_t    post;           /*!< POST(Power On Self Test) supported or not  */
    mesa_bool_t    ztp;            /*!< ZTP(Zero Touch Provisioning) supported or not  */
    mesa_bool_t    stack_fw_chk;   /*!< Stack firmware version check supported or not  */
} vtss_appl_sysutil_capabilities_t;

/**
 * \brief SysUtil status to get the average CPU loads in different periods
 */
typedef struct {
    uint32_t     average100msec;     /*!< Average load in 100 milli-seconds  */
    uint32_t     average1sec;        /*!< Average load in 1 second           */
    uint32_t     average10sec;       /*!< Average load in 10 seconds         */
} vtss_appl_sysutil_status_cpu_load_t;

/**
 * \brief Types of Image of Firmware status
 */
typedef enum {
    VTSS_APPL_SYSUTIL_REBOOT_TYPE_NONE,     /*!< No reboot      */
    VTSS_APPL_SYSUTIL_REBOOT_TYPE_COLD,     /*!< Cold reboot    */
    VTSS_APPL_SYSUTIL_REBOOT_TYPE_WARM,     /*!< Warm reboot    */
} vtss_appl_sysutil_reboot_type_t;

/**
 * \brief SysUtil control to reboot system
 */
typedef struct {
    vtss_appl_sysutil_reboot_type_t     type;       /*!< Reboot type, cold or warm if supported */
} vtss_appl_sysutil_control_reboot_t;

/**
 * \brief Types of power supply state
 */
typedef enum {
    VTSS_APPL_SYSUTIL_PSU_STATE_ACTIVE,      /*!< Active, the system used it as current power supply */
    VTSS_APPL_SYSUTIL_PSU_STATE_STANDBY,     /*!< Standby */
    VTSS_APPL_SYSUTIL_PSU_STATE_NOT_PRESENT  /*!< Not present */
} vtss_appl_sysutil_psu_state_t;

/**
 * \brief Power supply status
 */
typedef struct {
    vtss_appl_sysutil_psu_state_t   state;                                          /*!< The state of power supply */
    char                            descr[VTSS_APPL_SYSUTIL_PSU_DESCR_MAX_LEN + 1]; /*!< The description of power supply */
} vtss_appl_sysutil_psu_status_t;

/**
 * \brief System LED status
 */
typedef struct {
    char descr[VTSS_APPL_SYSUTIL_SYSTEM_LED_DESCR_MAX_LEN + 1]; /*!< The description of system LED */
} vtss_appl_sysutil_system_led_status_t;

/**
 * \brief Types of system LED status clearning
 */
typedef enum {
    VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_ALL,            /*!< Clear all error status of the system LED and back to normal indication */
    VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_FATAL,          /*!< Clear fatal error status of the system LED                             */
    VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_SW,             /*!< Clear generic software error status of the system LED                  */
    VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_POST,           /*!< Clear POST(Power On Self Test) error status of the system LED          */
    VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_ZTP,            /*!< Clear ZTP(Zero Touch Provisioning) error status of the system LED      */
    VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_STACK_FW_CHK    /*!< Clear stack firmware version check error status of the system LED                          */
} vtss_appl_sysutil_system_led_clear_type_t;

/**
 * \brief SysUtil control to reboot system
 */
typedef struct {
    vtss_appl_sysutil_system_led_clear_type_t type; /*!< System LED status clearing type */
} vtss_appl_sysutil_control_system_led_t;

/**
 * \brief SysUtil board information
 */
typedef struct {
    mesa_mac_t mac;                                      /*!< Switch board MAC address */
    uint32_t   board_id;                                      /*!< Switch board serial number */
    char  board_serial[VTSS_APPL_SYSUTIL_TAG_LENGTH];    /*!< Switch board serial (freeform text) */
    char  board_type[VTSS_APPL_SYSUTIL_TAG_LENGTH];      /*!< System board type (freeform text) */
} vtss_appl_sysutil_board_info_t;

/**
 * \brief SysUtil system time
 */
typedef struct {
    char sys_curtime[VTSS_APPL_SYSUTIL_SYSTEM_TIME_LEN];  /*!< System current time */
    char sys_curtime_format[VTSS_APPL_SYSUTIL_SYSTEM_TIME_LEN];  /*!< Formate for setting up system current time */
} vtss_appl_sysutil_system_time_t;

/**
 * \brief SysUtil system uptime information
 */
typedef struct {
    char   sys_uptime[VTSS_APPL_SYSUTIL_SYSTEM_UPTIME_LEN]; /*!< System Uptime*/
} vtss_appl_sysutil_sys_uptime_t;

/**
 * \brief SysUtil system configuration Info
 */
typedef struct {
    char   sys_name[VTSS_APPL_SYS_STRING_LEN];     /*!< System Hostname*/
    char   sys_location[VTSS_APPL_SYS_STRING_LEN]; /*!< System Location*/
    char   sys_contact[VTSS_APPL_SYS_STRING_LEN];  /*!< System Contact */
} vtss_appl_sysutil_sys_conf_t;

/** The maximum sensors of temperature monitor */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_SENSOR_CNT                2
/** The default value of temperature monitor board low threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_BOARD_LOW_DEFAULT         (-20)
/** The default value of temperature monitor board high threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_BOARD_HIGH_DEFAULT        85
/** The default value of temperature monitor board critical threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_BOARD_CRITICAL_DEFAULT    95
/** The default value of temperature monitor switch's junction low threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_JUNCTION_LOW_DEFAULT      (-40)
/** The default value of temperature monitor switch's junction high threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_JUNCTION_HIGH_DEFAULT     110
/** The default value of temperature monitor switch's junction critical threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_JUNCTION_CRITICAL_DEFAULT 120
/** The default value of temperature monitor hysteresis */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_HYSTERESIS_DEFAULT        5
/** The minimum value of temperature monitor configuration for low and high threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_LOW                 (-40)
/** The maximum value of temperature monitor configuration for low and high threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_HIGH                125
/** The minimum value of temperature monitor configuration for critical threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_CRITICAL_LOW              90
/** The maximum value of temperature monitor configuration for critical threshold */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_CRITICAL_HIGH             150
/** The minimum value of temperature monitor configuration for hysteresis */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_HYSTERESIS_LOW            1
/** The maximum value of temperature monitor configuration for hysteresis */
#define VTSS_APPL_SYSUTIL_TEMP_MONITOR_HYSTERESIS_HIGH           5

/**
 * \brief Temperature monitor sensor type
 */
typedef enum {
    VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD    = 0L,  /**< Board sensor */
    VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION = 1L   /**< Switch's junction sensor */
} vtss_appl_sysutil_tm_sensor_type_t;

/**
 * \brief Temperature monitor status
 */
typedef struct {
    mesa_bool_t low;      /*!< The alarm flag of temperature low status */
    mesa_bool_t high;     /*!< The alarm flag of temperature high status */
    mesa_bool_t critical; /*!< The alarm flag of temperature critical status */
    int  temp;     /*!< Current temperature */
} vtss_appl_sysutil_tm_status_t;

/**
 * \brief Temperature monitor config
 */
typedef struct {
    int low;        /*!< Config low threshold for board temperature monitor */
    int high;       /*!< Config high threshold for board temperature monitor */
    int critical;   /*!< Config critical threshold(Higher than temp_high) for board temperature monitor */
    int hysteresis; /*!< Config hysteresis for low and high threshold */
} vtss_appl_sysutil_tm_config_t;

/*
==============================================================================

    Public APIs

==============================================================================
*/
/**
 * \brief Get capabilities
 *
 * \param capabilities [OUT] The capabilities of sysutil.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_capabilities_get(
    vtss_appl_sysutil_capabilities_t    *const capabilities
);

/**
 * \brief Get average CPU loads in different periods
 *
 * \param cpuLoad [OUT] The average CPU loads.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_status_cpu_load_get(
    vtss_appl_sysutil_status_cpu_load_t     *const cpuLoad
);

/**
 * \brief Reboot iterate function
 *
 * \param prev_usid [IN]  previous switch ID.
 * \param next_usid [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_reboot_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
);

/**
 * \brief Get reboot parameters of sysutil control
 *
 * \param usid   [IN]  Switch ID.
 * \param reboot [OUT] The reboot parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_reboot_get(
    vtss_usid_t                             usid,
    vtss_appl_sysutil_control_reboot_t      *const reboot
);

/**
 * \brief Set reboot parameters of sysutil control
 *
 * \param usid   [IN] Switch ID.
 * \param reboot [IN] The reboot parameters.
 * \param wait_before_callback_msec [IN] An optional number of milliseconds to wait before the reboot callback handlers are invoked. Useful if a network reply is required before the actual reboot.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_reboot_set(
    vtss_usid_t                                 usid,
    const vtss_appl_sysutil_control_reboot_t    *const reboot,
    uint32_t                                    wait_before_callback_msec = 0
);

/**
 * \brief Power supply iterate function, it is used to get first and get next indexes.
 *
 * \param prev_usid   [IN]  previous switch ID.
 * \param next_usid   [OUT] next switch ID.
 * \param prev_psuid  [IN]  previous precedence.
 * \param next_psuid  [OUT] next precedence.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_psu_status_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid,
    const uint32_t           *const prev_psuid,
    uint32_t                 *const next_psuid
);

/**
 * \brief Get power supply status
 *
 * \param usid    [IN]  switch ID for user view (The value starts from 1)
 * \param psuid   [IN]  The index of power supply
 * \param status  [OUT] The power supply status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_psu_status_get(
    vtss_usid_t                     usid,
    uint32_t                             psuid,
    vtss_appl_sysutil_psu_status_t  *const status
);

/**
 * \brief User switch iterate function, it is used to get first and get next indexes.
 *
 * \param prev_usid   [IN]  previous switch ID.
 * \param next_usid   [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_usid_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
);

/**
 * \brief Get system LED status
 *
 * \param usid    [IN]  switch ID for user view (The value starts from 1)
 * \param status  [OUT] The system LED status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_system_led_status_get(
    vtss_usid_t                             usid,
    vtss_appl_sysutil_system_led_status_t   *const status
);

/**
 * \brief Get reboot parameters of sysutil control
 *
 * \param usid   [IN]  Switch ID.
 * \param clear [OUT] The clear system LED parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_system_led_get(
    vtss_usid_t                             usid,
    vtss_appl_sysutil_control_system_led_t  *const clear
);

/**
 * \brief Set clear system LED parameters of sysutil control
 *
 * \param usid  [IN] Switch ID.
 * \param clear [IN] The clear system LED parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_control_system_led_set(
    vtss_usid_t                                     usid,
    const vtss_appl_sysutil_control_system_led_t    *const clear
);

/**
 * \brief Provide board info
 *
 * \param conf [OUT] The MAC address, serial number of the switch board
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_board_info_get(
    vtss_appl_sysutil_board_info_t *const conf
);

/**
 * \brief Provide system current time info
 *
 * \param system_time [OUT] The current system time
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_system_time_get(
    vtss_appl_sysutil_system_time_t *const system_time
);

/**
 * \brief Setting up system current time
 *
 * \param system_time [in] new system time setup by the user
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_system_time_set(
    const vtss_appl_sysutil_system_time_t *const system_time
);

/**
 * \brief System uptime info
 *
 * \param status [OUT] The system uptime of the switch
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_sytem_uptime_get(
    vtss_appl_sysutil_sys_uptime_t *const status
);

/**
 * \brief System configuration info
 *
 * \param info [OUT] The system information of the switch
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_system_config_get(
    vtss_appl_sysutil_sys_conf_t *const info
);

/**
 * \brief Configure system parameters
 *
 * \param config [IN]  System parameters of the switch
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_system_config_set(
    const vtss_appl_sysutil_sys_conf_t *const config
);

/**
 * \brief Get temperature monitor configuration info
 *
 * \param sensor    [IN]    (key) Temperature monitor sensor ID.
 * \param config    [OUT]   The current configuration of the temperature monitor sensor.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_temperature_monitor_config_get(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_config_t      *const config
);

/**
 * \brief Configure temperature monitor parameters
 *
 * \param sensor    [IN]    (key) Temperature monitor sensor ID.
 * \param config    [IN]    The current configuration of the temperature monitor sensor.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_temperature_monitor_config_set(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    const vtss_appl_sysutil_tm_config_t      *const config
);

/**
 * \brief Get temperature monitor status
 *
 * \param sensor    [IN]    (key) Temperature monitor sensor ID.
 * \param status    [OUT]   The temperature monitor status of the temperature monitor sensor.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_temperature_monitor_status_get(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_status_t      *const status
);

/**
 * \brief Iterator for retrieving temperature monitor information key/index
 *
 * To walk information (configuration and status) index of the entry in temperature monitor.
 *
 * \param prev      [IN]    Sensor index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_sysutil_temperature_monitor_sensor_itr(
    const vtss_appl_sysutil_tm_sensor_type_t *const prev,
    vtss_appl_sysutil_tm_sensor_type_t       *const next
);

/**
 * \brief Get temperature monitor event status.
 *
 * \param sensor    [IN]    (key) Temperature monitor sensor ID.
 * \param status    [OUT]   The temperature monitor status of the temperature monitor sensor.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_sysutil_temperature_monitor_event_get(
    vtss_appl_sysutil_tm_sensor_type_t sensor,
    vtss_appl_sysutil_tm_status_t      *const status
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_SYSUTIL_H_ */
