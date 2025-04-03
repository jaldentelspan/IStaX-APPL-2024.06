/* -*- Mode: C; c-basic-offset: 2; tab-width: 8; c-comment-only-line-offset: 0; -*- */
/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "monitor_api.h"
#include "misc_api.h"
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include "adt_7476_api.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYSTEM
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SYSTEM
#endif /* VTSS_SW_OPTION_PSU */

/****************************************************************************/
/*                                                                          */
/*  MODULE INTERNAL FUNCTIONS - LED board dependent functions               */
/*                                                                          */
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(VTSS_SW_OPTION_PSU)
static u8 monitor_tlv_data[256];
/*
 * This implementation can be overridden.
 */
BOOL monitor_prop_get(monitor_sensor_t** monitor_prop, int *tlv_num) __attribute__ ((weak, alias("__monitor_prop_get")));

/* The reference implementation - dummy */
static 
BOOL __monitor_prop_get(monitor_sensor_t** monitor_sensor, int *tlv_num)
{
    return FALSE;
}

static BOOL monitor_sensor_get( monitor_sensor_id_t monitor_sensor_id, monitor_sensor_t *monitor_sensor ) 
{
    monitor_sensor_t *monitor_prop, *ptr;
    int i, monitor_num;

    if ( FALSE == monitor_prop_get(&monitor_prop, &monitor_num) ) {
        return FALSE;
    }
    
    for ( i = 0, ptr = &monitor_prop[0]; i < monitor_num; i++, ptr++ ) {
        if ( ptr->id == monitor_sensor_id ) {
            break;
        }
    }
    
    if ( i == monitor_num ) {
        return FALSE;
    }
    
    if ( monitor_sensor ) {
        *monitor_sensor = *ptr;
    }
    return TRUE;
}

char *monitor_state_txt(monitor_sensor_status_t status)
{
    switch (status) 
    {
        case MONITOR_STATE_NORMAL:
            return "NORMAL";

        case MONITOR_STATE_ABNORMAL:
            return "ABNORMAL";

        case MONITOR_STATE_NOTPRESENT:
            return "NOTPRESENT";

        case MONITOR_STATE_NOTFUNCTION:
            return "NOTFUNCTION";
        default:
            return "UNKNOWN";
    }

}

char *monitor_descr_txt( monitor_sensor_id_t id )
{
    monitor_sensor_t monitor_sensor;

    if (( FALSE == monitor_sensor_get (id, &monitor_sensor) ) ) {
        return "unKnown";
    } else {
        return monitor_sensor.descr;
    }
}

static adt7476_hw_t monitor_devs[] = {
    {0x2d, 14},
    {0x2e, 14},
};

BOOL monitor_sensor_status( monitor_sensor_id_t monitor_sensor_id, monitor_sensor_status_t *status)
{
    monitor_sensor_t    monitor_sensor;
    adt7476_status_t    monitor_status, tmp;
    u8                  reg_addr, data;

    if ( FALSE == monitor_sensor_get (monitor_sensor_id, &monitor_sensor) ) {
        *status = MONITOR_STATE_NOTPRESENT;
        return TRUE;
    }

    T_D("i2c_addr = 0x%02x, offset = 0x%02x, len = %d, data_len = %d, i2c_clk = %d ", 
            monitor_sensor.i2c_addr, monitor_sensor.offset[0], monitor_sensor.offset_len, monitor_sensor.len, monitor_sensor.i2c_clk_sel);

   if ( vtss_i2c_wr_rd(NULL, monitor_sensor_id == MONITOR_MAIN_PSU ? monitor_devs[0].addr: monitor_devs[1].addr, &monitor_sensor.offset[0], monitor_sensor.offset_len, &data, monitor_sensor.len, 100) != VTSS_RC_OK ) {
        *status = MONITOR_STATE_NOTFUNCTION;
        return TRUE;
    }

        /* 0x80 is 8V, if  the  */
    if ( (monitor_sensor_id == MONITOR_MAIN_PSU &&  data <= 0x80) ||
         (monitor_sensor_id != MONITOR_MAIN_PSU &&  data*4 <= 0x40)  ) {
        *status = MONITOR_STATE_ABNORMAL;
    } else {
        *status = MONITOR_STATE_NORMAL;
    }
    return TRUE;
}

#endif /* VTSS_SW_OPTION_PSU */

#ifdef __cplusplus
}
#endif

