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

#ifndef _PTP_1PPS_SERIAL_H_
#define _PTP_1PPS_SERIAL_H_

/* Define interfaces for the serial 1pps interface */
#include "main_types.h"
#include "ptp_api.h"

/**
 * \brief Send a 1pps message on the serial port.
 * \param t     [IN]  timestamp indicating the time of next 1pps pulse.
 * \param proto [IN]  protocol to use over serial line
 * \return nothing
 */
void ptp_1pps_msg_send(const mesa_timestamp_t *t, const ptp_rs422_protocol_t proto);

/**
 * \brief initialize the 1pps message serial port.
 * \return nothing
 */
mesa_rc ptp_1pps_serial_init(void);

/**
 * \brief Set the baudrate of the serial port
 * \param serial_info [IN]  structure holding the parameters to be configured for the serial port
 * \return Error code indicating whether operation succeeded or not.
 */
mesa_rc ptp_1pps_set_baudrate(const vtss_serial_info_t *serial_info);

/**
 * \brief Get the baudrate of the serial port
 * \param serial_info [IN]  structure for holding the parameters for the serial port
 * \return Error code indicating whether operation succeeded or not.
 */
mesa_rc ptp_1pps_get_baudrate(vtss_serial_info_t *serial_info);

/**
 * \brief Get the default baudrate (and other paramaters) of the serial port
 * \param serial_info [IN]  structure for holding the parameters for the serial port
 * \return nothing
 */
void ptp_1pps_get_default_baudrate(vtss_serial_info_t *serial_info);

// Send alarm through rs422 interface.
void ptp_rs422_alarm_send(mesa_bool_t alarm);
#endif /* _PTP_1PPS_SERIAL_H_ */

