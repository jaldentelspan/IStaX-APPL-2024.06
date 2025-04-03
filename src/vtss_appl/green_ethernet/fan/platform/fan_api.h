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

#ifndef _FAN_API_H_
#define _FAN_API_H_
#include "vtss/appl/fan.h"
#ifdef __cplusplus
extern "C" {
#endif
/** Default temperature values for the fan start */
#define VTSS_APPL_FAN_CONF_T_ON_DEFAULT          75
/** Default temperature value at which the fan will be to set to run at full speed */
#define VTSS_APPL_FAN_CONF_T_MAX_DEFAULT         100
/** MAX temperature */
#define VTSS_APPL_FAN_TEMP_MAX                   127
/** MIN temperature */
#define VTSS_APPL_FAN_TEMP_MIN                   -127
//************************************************
// Definition of rc errors - See also fan_error_txt in fan.c
//************************************************
/************************************************
- Error return code
************************************************/
/*!
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    VTSS_APPL_FAN_ERROR_ISID = MODULE_ERROR_START(VTSS_MODULE_ID_FAN), /*!< Invalid Switch ID. */
    VTSS_APPL_FAN_ERROR_SECONDARY_SWITCH,                              /*!< Could not get data from secondary switch. */
    VTSS_APPL_FAN_ERROR_NOT_PRIMARY_SWITCH,                            /*!< Switch must to be the primary switch. */
    VTSS_APPL_FAN_ERROR_VALUE,                                         /*!< Invalid value. */
    VTSS_APPL_FAN_ERROR_T_CONF,                                        /*!< Max. Temperature must be lower than 'On' temperature. */
    VTSS_APPL_FAN_ERROR_T_ON_LOW,                                      /*!< Max. Temperature must be higher than 'On' temperature. */
    VTSS_APPL_FAN_ERROR_FAN_NOT_RUNNING                                /*!< Fan is supposed to be running, but fan speed is 0 rpm. Please make sure that the fan isn't blocked. */
};
const char *fan_error_txt(mesa_rc rc);
//************************************************
// Functions
//************************************************
/*!
 * Get Fan Current Status Parameters
 *
 * \param status  [OUT] The status data
 *
 * \param isid    [IN] Internal switch id
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_fan_status_get(
    vtss_appl_fan_status_t *status,
    vtss_isid_t isid
);

/*!
 * Get Fan Number Of Temperature Sensonrs
 *
 * \param isid    [IN] Internal switch id
 *
 * \param cnt     [OUT] Number of temp sensors
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_fan_temperature_sensors_count_get(
    vtss_isid_t isid,
    u8 *cnt
);

/*!
 * Get Fan module status
 *
 * \return true if module enabled/supported
 */
bool fan_module_enabled(void);

/*!
 * Get Fan init function
 *
 * \param data [IN] data pointer
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc fan_init(
    vtss_init_data_t
    *data
);

const char *mesa_fan_pwd_freq_t_to_str(mesa_fan_pwd_freq_t value);
mesa_fan_pwd_freq_t str_to_mesa_fan_pwd_freq_t(const char *value);

#ifdef __cplusplus
}
#endif
#endif // _FAN_API_H_

