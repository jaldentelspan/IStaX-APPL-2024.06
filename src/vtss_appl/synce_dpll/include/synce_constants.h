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

#ifndef _SYNCE_CONSTANTS_H
#define _SYNCE_CONSTANTS_H

#include <microchip/ethernet/switch/api.h>

#define CLOCK_INPUT_MAX 3
#define SYNCE_NOMINATED_MAX   CLOCK_INPUT_MAX                                      /* maximum number of nominated ports */
#define SYNCE_PRIORITY_MAX    CLOCK_INPUT_MAX                                      /* maximum number of priorities */
#define SYNCE_PORT_COUNT      (fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT)) /* number of PHY ports possible running SSM (+1 reserved for station clock)*/
#define SYNCE_STATION_CLOCK_PORT fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)           /* number of PHY ports possible running SSM (+1 reserved for station clock)*/
#define STATION_CLOCK_SOURCE_NO 2                                                  /* source used to nominate the station clock input */

#define CLOCK_INTERRUPT_MASK_REG      23
#define CLOCK_INTERRUPT_PENDING_REG  131
#define CLOCK_INTERRUPT_STATUS_REG   129

#define CLOCK_LOSX_EV   0x00000001
#define CLOCK_LOL_EV    0x00000002
#define CLOCK_LOCS1_EV  0x00000004
#define CLOCK_LOCS2_EV  0x00000008
#define CLOCK_FOS1_EV   0x00000010
#define CLOCK_FOS2_EV   0x00000020

#endif // _SYNCE_CONSTANTS_H
