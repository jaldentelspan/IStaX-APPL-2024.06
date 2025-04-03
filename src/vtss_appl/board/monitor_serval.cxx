/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#if defined(VTSS_SW_OPTION_PSU)
#include "main.h"
#include "conf_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "monitor_api.h"

#define MONITOR_1_ADDR                   0x1d
#define MONITOR_2_ADDR                   0x1e
#define MONITOR_CLK                      14

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYSTEM
/*externC*/ /*const*/ monitor_sensor_t serval_monitor_sensor[] = {
    {MONITOR_MAIN_PSU,         "Main PSU",      1,  {0x24}, 1, MONITOR_1_ADDR, MONITOR_CLK },
    {MONITOR_REDUNDANT_PSU,    "Redundant PSU", 1,  {0x23}, 1, MONITOR_2_ADDR, MONITOR_CLK },
};




BOOL monitor_prop_get(monitor_sensor_t** monitor_sensor, int *tlv_num)
{
    int board_type = vtss_board_type();
    T_W("Not avaiable the board(%d)", board_type);
    return FALSE;
}
#endif /* VTSS_SW_OPTION_PSU */

