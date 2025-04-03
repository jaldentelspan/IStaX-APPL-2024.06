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

#ifndef _ZL_3073x_API_H_
#define _ZL_3073x_API_H_

#include "zl303xx.h"
#include "zl303xx_Os.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ZL_3034X_API
#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_OS_PORT      1
#define TRACE_GRP_SYNC_INTF    2
#define TRACE_GRP_ZL_TRACE     3

#define ZL3073X_CONF_VERSION    1

#define ZL_3073X_DATA_LOCK()   critd_enter(&zl3073x_global.datamutex, __FILE__, __LINE__)
#define ZL_3073X_DATA_UNLOCK() critd_exit (&zl3073x_global.datamutex, __FILE__, __LINE__)

#define ZL_3073X_RC(expr) { mesa_rc _rc_ = (expr); if (_rc_ < VTSS_RC_OK) { \
        T_I("Error code: 0x%x", _rc_); }}

#define ZL_3073X_CHECK(expr) { zlStatusE _rc_ = (expr); if (_rc_ != ZL303XX_OK) { \
        T_W("ZL Error code: %d", _rc_); }}

/**********************************************************************************
 * ZL Trace Functions                                                             *
 **********************************************************************************/
#ifndef MAX_FMT_LEN
#define MAX_FMT_LEN 256
#endif

extern zl303xx_ParamsS *zl_3073x_zl303xx_synce_dpll;
extern zl303xx_ParamsS *zl_3073x_zl303xx_ptp_dpll;
extern zl303xx_ParamsS *zl_3073x_zl303xx_nco_asst_dpll;

void zl_3073x_spi_write(u32 address, u8 *data, u32 size);
void zl_3073x_spi_read(u32 address, u8 *data, u32 size);

#endif /* _ZL_3073x_API_H_ */

