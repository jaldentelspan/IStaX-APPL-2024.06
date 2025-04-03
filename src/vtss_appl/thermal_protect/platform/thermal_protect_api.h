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

#ifndef _THERMAL_PROTECT_API_H_
#define _THERMAL_PROTECT_API_H_

#include "vtss/appl/thermal_protect.h"

#ifdef __cplusplus
extern "C" {
#endif
//************************************************
// Definition of rc errors - See also thermal_protect_error_txt in thermal_protect.c
//************************************************

/************************************************
- Error return code
************************************************/

char *thermal_protect_error_txt(mesa_rc rc);
char *thermal_protect_power_down2txt(BOOL powered_down);

//************************************************
// Constants
//************************************************
/**  Maximum chip temperature value */
#define VTSS_APPL_THERMAL_PROTECT_TEMP_MAX            255
/**  Minimum chip temperature value */
#define VTSS_APPL_THERMAL_PROTECT_TEMP_MIN            0
/**  Total group count */
#define VTSS_APPL_THERMAL_PROTECT_GROUP_CNT           4
/**  Maximum group value*/
#define VTSS_APPL_THERMAL_PROTECT_GROUP_MAX_VALUE     (VTSS_APPL_THERMAL_PROTECT_GROUP_CNT - 1)
/**  Minimum group value */
#define VTSS_APPL_THERMAL_PROTECT_GROUP_MIN_VALUE     0
/**  Setting the port_grp to this, mean disable thermal protect for the interface */
#define VTSS_APPL_THERMAL_PROTECT_GROUP_DISABLED      VTSS_APPL_THERMAL_PROTECT_GROUP_CNT

//************************************************
// Configuration definition
//************************************************
/*!
 *  \brief
 *      thermal protect global configuration
 *      configuration that are shared for all switches in the stack
 */
typedef struct {
    i16   grp_temperatures[VTSS_APPL_THERMAL_PROTECT_GROUP_CNT]; /**< Array with the shut down temperature for each thermal protect group */
} vtss_appl_thermal_protect_glbl_conf_t;

/*!
 *  \brief
 *      Switch local configurations that are local for a switch in the stack.
 */
typedef struct {
    u8 port_grp[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD]; /**< The thermal protect group for each port */
} vtss_appl_thermal_protect_local_conf_t;

/*!
 *   \brief
 *     Switch status .
 */
typedef struct {
    i16  port_temp[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];                            /**< The temperature associated to a given port. */
    BOOL port_powered_down[VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];                  /**< True if the port is powered down due to thermal protection. */
} vtss_appl_thermal_protect_local_status_t;

// Switch configuration (Both local information, and global configuration which is common for all switches in the stack.
typedef struct {
    vtss_appl_thermal_protect_glbl_conf_t      glbl_conf;     /**< The configuration that is common for all switches in the stack. */
    vtss_appl_thermal_protect_local_conf_t     local_conf;   /**< The thermal protect group for each port. */
} vtss_appl_thermal_protect_switch_conf_t;

// See thermal_protect.c
void vtss_appl_thermal_protect_switch_conf_default_get(vtss_appl_thermal_protect_switch_conf_t *switch_conf);

//************************************************
// Functions
//************************************************
void    vtss_appl_thermal_protect_switch_conf_get(vtss_appl_thermal_protect_switch_conf_t *local_conf, vtss_isid_t isid);
mesa_rc vtss_appl_thermal_protect_switch_conf_set(vtss_appl_thermal_protect_switch_conf_t *new_local_conf, vtss_isid_t isid);
mesa_rc vtss_appl_thermal_protect_get_switch_status(vtss_appl_thermal_protect_local_status_t *status, vtss_isid_t isid);

bool thermal_protect_module_enabled(void);

mesa_rc thermal_protect_init(vtss_init_data_t *data);
#ifdef __cplusplus
}
#endif
#endif // _THERMAL_PROTECT_API_H_

