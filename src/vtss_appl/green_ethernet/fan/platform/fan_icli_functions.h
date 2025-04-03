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
 * \brief mirror icli functions
 * \details This header file describes FAN iCLI
 */


#ifndef _VTSS_ICLI_FAN_H_
#define _VTSS_ICLI_FAN_H_

#include "icli_api.h"
#include "fan_api.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Function for showing fan status (chip temperature and fan speed)
 *
 * \param session_id [IN]  The session id.
 * \return None.
 **/
void fan_status(i32 session_id);


/**
 * \brief Function for setting fan configuration
 *
 * \param session_id    [IN]  Needed for being able to print error messages
 * \param has_t_on      [IN]  TRUE if temp_on parameter is set
 * \param new_temp_on   [IN]  New temperature on value
 * \param has_t_max     [IN]  TRUE if temp_max parameter is set
 * \param new_temp_max  [IN]  New temperature on value
 * \param has_pwm       [IN]  Is a pwm value configured
 * \param pwm           [IN]  The pwm frequency for the fan
 * \param no            [IN]  TRUE if temp_on/temp_max shall be set to default value
 * \returns Error Code.
 **/
mesa_rc fan_temp(u32 session_id, BOOL has_t_on, i8 new_temp_on, BOOL has_t_max, i8 new_temp_max, BOOL has_pwm, mesa_fan_pwd_freq_t pwm, BOOL no);

/**
 * \brief Init function ICLI CFG
 * \return Error code.
 **/
mesa_rc fan_icfg_init(void);
#ifdef __cplusplus
}
#endif
#endif /* _VTSS_ICLI_FAN_H_ */

