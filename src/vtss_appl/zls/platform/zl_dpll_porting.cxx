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

#include "zl303xx_Api.h"
#include "zl303xx_LogToMsgQ.h"

#include "main_types.h"
#include "zl_30361_api.h"
#include "zl_30361_api_api.h"
#include "vtss_tod_api.h"
#include "vtss_os_wrapper.h"
#include "critd_api.h"

#include <stdlib.h>
#include <sys/time.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>

#undef min    // Pesky macross from ZL headers
#undef max
#include "vtss_timer_api.h"
#include "synce_spi_if.h"


#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ZL_3034X_API

static void zl_dpll_spi_write(u32 address, u8 *data, u32 size)
{
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_write(address, data, size);
}

static void zl_dpll_spi_read(u32 address, u8 *data, u32 size)
{
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_read(address, data, size);
}

#ifdef __cplusplus
extern "C" {
#endif
cpuStatusE cpuSpiWrite(void *par, Uint32T regAddr, Uint8T *dataBuf, Uint16T bufLen)
{
    T_DG(TRACE_GRP_OS_PORT, "cpuSpiWrite  regAddr %X  bufLen %u  data %X-%X\n", regAddr, bufLen, dataBuf[0], dataBuf[1]);
//printf("cpuSpiWrite  regAddr %X  bufLen %u  data %X-%X\n", regAddr, bufLen, dataBuf[0], dataBuf[1]);
    zl_dpll_spi_write(regAddr, dataBuf, bufLen);

    return(CPU_OK);
}

cpuStatusE cpuSpiRead(void *par, Uint32T regAddr, Uint8T *dataBuf, Uint16T bufLen)
{
    zl_dpll_spi_read(regAddr, dataBuf, bufLen);
    T_DG(TRACE_GRP_OS_PORT, "cpuSpiRead  regAddr %X  bufLen %u  data %X-%X\n", regAddr, bufLen, dataBuf[0], dataBuf[1]);
//printf("cpuSpiRead  regAddr %X  bufLen %u  data %X-%X\n", regAddr, bufLen, dataBuf[0], dataBuf[1]);

    return(CPU_OK);
}

#ifdef __cplusplus
}
#endif
