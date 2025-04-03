/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef _MONITOR_API_H_
#define _MONITOR_API_H_
#ifdef __cplusplus
extern "C" {
#endif

#if defined(VTSS_SW_OPTION_PSU)
typedef enum {
    MONITOR_MAIN_PSU = 0x0,
    MONITOR_REDUNDANT_PSU,
    MONITOR_END
} monitor_sensor_id_t;

typedef enum {
    MONITOR_STATE_NORMAL = 0x0,
    MONITOR_STATE_ABNORMAL,
    MONITOR_STATE_NOTPRESENT,
    MONITOR_STATE_NOTFUNCTION,
    MONITOR_STATE_END
} monitor_sensor_status_t;

typedef struct {
    monitor_sensor_id_t  id;
    char                *descr;
    u8                  len;
    u8                  offset[4];
    u8                  offset_len;
    u8                  i2c_addr;
    i8                  i2c_clk_sel;
} monitor_sensor_t;

BOOL monitor_prop_get(monitor_sensor_t** monitor_sensor, int *tlv_num);

char *monitor_sensor_txt( monitor_sensor_id_t monitor_sensor_id );

BOOL monitor_sensor_status( monitor_sensor_id_t monitor_sensor_id, monitor_sensor_status_t *status);

monitor_sensor_id_t monitor_sensor_active( void );

char *monitor_descr_txt( monitor_sensor_id_t id );
char *monitor_state_txt(monitor_sensor_status_t status);
#endif /* VTSS_SW_OPTION_PSU */

#ifdef __cplusplus
}
#endif

#endif /*  _MONITOR_API_H_  */
