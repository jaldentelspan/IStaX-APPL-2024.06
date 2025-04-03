/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _ZL_30380_API_PDV_H_
#define _ZL_30380_API_PDV_H_

#include "zl303xx_Os.h"
#include "zl303xx_DataTypesEx.h"

#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_PTP_INTF     1
#define TRACE_GRP_OS_PORT      2
#define TRACE_GRP_ZL_TRACE     3

#define ZL30380_CONF_VERSION   1

#define ZL_3036X_DATA_LOCK()   critd_enter(&zl30380_global.datamutex, __FILE__, __LINE__)
#define ZL_3036X_DATA_UNLOCK() critd_exit (&zl30380_global.datamutex, __FILE__, __LINE__)


#define ZL_30380_RC(expr) { mesa_rc _rc_ = (expr); if (_rc_ < VTSS_RC_OK) { \
        T_I("Error code: %x", _rc_); }}

#define ZL_30380_CHECK(expr) { zlStatusE _rc_ = (expr); if (_rc_ != ZL303XX_OK) { \
        T_W("ZL Error code: %x", _rc_); }}


/**********************************************************************************
 * API Version information                                                        *
 **********************************************************************************/
extern "C" {
extern const char zl303xx_ApiReleaseVersion[];  // Expected API Version
extern const char zl303xx_ApiReleaseSwId[];
extern const char zl303xx_AprReleaseVersion[];  // Actual API Version (from the shared object)
extern const char zl303xx_AprReleaseSwId[];     // Actual API SW Variant (from the shared object)
}

/**********************************************************************************
 * ZL Trace Functions                                                             *
 **********************************************************************************/
#ifndef MAX_FMT_LEN
#define MAX_FMT_LEN 256
#endif

extern zl303xx_BooleanE aprLoggingEnabled;

#endif /* _ZL_30380_API_PDV_H_ */

