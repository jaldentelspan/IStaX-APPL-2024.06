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

#include "main.h"
#include "main_types.h"
#include "microchip/ethernet/switch/api.h"
#include "zl_30361_api_api.h"
#include "zl_30361_api.h"
#include "synce_spi_if.h"

// xl3036x base interface
#include "zl303xx_Api.h"
#include "zl303xx_DebugMisc.h"
#include "zl303xx_DebugDpll36x.h"
#include "zl303xx_AddressMap.h"
#include "zl303xx_AddressMap36x.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_Dpll361.h"
#undef min // Undefine these because they are causing problems in interrupt_api.h below
#undef max //
#include "interrupt_api.h"

/* interface to other components */
#include "vtss_tod_api.h"


/* ================================================================= *
 *  Configuration definitions
 * ================================================================= */

/****************************************************************************
 * Configuration API
 ****************************************************************************/


mesa_rc zl_3036x_register_get(const u32   page,
                              const u32   reg,
                              u32 *const  value)
{
    *value = 0;

    ZL_3036X_CHECK(zl303xx_Read(zl_3036x_zl303xx_params, NULL, ZL303XX_MAKE_MEM_ADDR_36X((Uint32T)(page * (ZL303XX_MEM_ADDR_MASK_36X + 1) + reg), ZL303XX_MEM_SIZE_1_BYTE), (Uint32T *)value));

    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_register_set(const u32   page,
                              const u32   reg,
                              const u32   value)
{
    ZL_3036X_CHECK(zl303xx_Write(zl_3036x_zl303xx_params, NULL, ZL303XX_MAKE_MEM_ADDR_36X((Uint32T)(page * (ZL303XX_MEM_ADDR_MASK_36X + 1) + reg), ZL303XX_MEM_SIZE_1_BYTE), (Uint32T)value));

    return(VTSS_RC_OK);
}

mesa_rc zl_3036x_debug_pll_status(void)
{
    mesa_rc rc = VTSS_RC_OK;

    if (zl303xx_DebugPllStatus36x(zl_3036x_zl303xx_params) != ZL303XX_OK) {
        T_D("Error during Zarlink pll status get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);

}

mesa_rc zl_3036x_debug_hw_ref_status(u32 ref_id)
{
    mesa_rc rc = VTSS_RC_OK;

    if (zl303xx_DebugHwRefStatus36x(zl_3036x_zl303xx_params, ref_id) != ZL303XX_OK) {
        T_D("Error during Zarlink hw ref status get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}

mesa_rc zl_3036x_debug_hw_ref_cfg(u32 ref_id)
{
    mesa_rc rc = VTSS_RC_OK;
    if (zl303xx_DebugHwRefCfg36x(zl_3036x_zl303xx_params, ref_id) != ZL303XX_OK) {
        T_D("Error during Zarlink hw ref cfg get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}

mesa_rc zl_3036x_debug_dpll_status(u32 pll_id)
{
    mesa_rc rc = VTSS_RC_OK;

    if (ZL303XX_CHECK_POINTER(zl_3036x_zl303xx_params) == ZL303XX_INVALID_POINTER) {
        rc = VTSS_RC_ERROR;
        return(rc);
    }
    u32 my_pll_id;

    my_pll_id = zl_3036x_zl303xx_params->pllParams.pllId;
    zl_3036x_zl303xx_params->pllParams.pllId = pll_id - 1;
    if (zl303xx_DebugDpllStatus36x(zl_3036x_zl303xx_params) != ZL303XX_OK) {
        T_D("Error during Zarlink dpll status get");
        rc = VTSS_RC_ERROR;
    }
    zl_3036x_zl303xx_params->pllParams.pllId = my_pll_id;
    return(rc);
}

mesa_rc zl_3036x_debug_dpll_cfg(u32 pll_id)
{
    mesa_rc rc = VTSS_RC_OK;

    if (ZL303XX_CHECK_POINTER(zl_3036x_zl303xx_params) == ZL303XX_INVALID_POINTER) {
        rc = VTSS_RC_ERROR;
        return(rc);
    }
    u32 my_pll_id;

    my_pll_id = zl_3036x_zl303xx_params->pllParams.pllId;
    zl_3036x_zl303xx_params->pllParams.pllId = pll_id;
    if (zl303xx_DebugDpllConfig36x(zl_3036x_zl303xx_params) != ZL303XX_OK) {
        T_D("Error during Zarlink dpll cfg get");
        rc = VTSS_RC_ERROR;
    }
    zl_3036x_zl303xx_params->pllParams.pllId = my_pll_id;
    return(rc);
}

mesa_rc zl_3036x_debug_nco_freq_read(i32 *freqOffsetUppm)
{
    mesa_rc rc = VTSS_RC_OK;

    if (zl303xx_Dpll36xGetFreq(zl_3036x_zl303xx_params, freqOffsetUppm, ZLS3036X_AMEM)) {
        T_D("Error during Zarlink dpll cfg get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}

mesa_rc zl_3036x_trace_init(FILE *logFd)
{
    zl303xx_TraceInit(logFd);

    return VTSS_RC_OK;
}

mesa_rc zl_3036x_trace_level_module_set(u32 module, u32 level)
{
    zl303xx_TraceSetLevel(module, level);

    return VTSS_RC_OK;
}

mesa_rc zl_3036x_trace_level_all_set(u32 level)
{
    zl303xx_TraceSetLevelAll(level);

    return VTSS_RC_OK;
}

void zl_3036x_spi_write(u32 address, u8 *data, u32 size)
{
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_write(address, data, size);
}

void zl_3036x_spi_read(u32 address, u8 *data, u32 size)
{
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_read(address, data, size);
}

