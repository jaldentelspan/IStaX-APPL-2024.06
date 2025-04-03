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

#include "icli_porting_util.h"
#include "zl_30361_api_icli_functions.h"

#include "zl_30361_api_api.h"

#include "misc_api.h"

/***************************************************************************/
/*  Internal types                                                         */
/***************************************************************************/

/***************************************************************************/
/*  Internal functions                                                     */
/***************************************************************************/


icli_rc_t zl_3036x_api_icli_debug_read(i32 session_id, u32 mem_page, u32 mem_addr, u32 mem_size)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u32 i, n;
    u32 value;

    for (i=0; i<mem_size; i++)
    {
        if (zl_3036x_register_get(mem_page,  mem_addr+i,  &value) != VTSS_RC_OK) {
            rc = ICLI_RC_ERROR;
        } else {
            n = (i & 0x7);
            if (n == 0)   ICLI_PRINTF("%03X: ", i + mem_addr);
            ICLI_PRINTF("%02x ", value);
            if (n == 0x7 || i == (mem_size - 1))  ICLI_PRINTF("\n");
        }
    }
    return rc;
}

icli_rc_t zl_3036x_api_icli_debug_write(i32 session_id, u32 mem_page, u32 mem_addr, u32 value)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3036x_register_set(mem_page,  mem_addr,  value) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3036x_api_icli_debug_pll_status(i32 session_id)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3036x_debug_pll_status() != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3036x_api_icli_debug_ref_status(i32 session_id, u32 refid)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3036x_debug_hw_ref_status(refid) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3036x_api_icli_debug_ref_cfg(i32 session_id, u32 refid)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3036x_debug_hw_ref_cfg(refid) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3036x_api_icli_debug_dpll_status(i32 session_id, u32 pllid)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3036x_debug_dpll_status(pllid) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3036x_api_icli_debug_dpll_cfg(i32 session_id, u32 pllid)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3036x_debug_dpll_cfg(pllid) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3036x_api_icli_debug_nco_freq_read(i32 session_id)
{
    i32 freqOffsetUppm;
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3036x_debug_nco_freq_read(&freqOffsetUppm) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    } else {
        ICLI_PRINTF("DcoFreqOffset %d.%ld ppb\n", freqOffsetUppm/1000, labs(freqOffsetUppm%1000));
    }
    return rc;
}

icli_rc_t zl_3036x_api_icli_debug_trace_level_module_set(i32 session_id, u32 v_uint_1, u32 v_uint_2)
{
    return zl_3036x_trace_level_module_set(v_uint_1, v_uint_2) ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t zl_3036x_api_icli_debug_trace_level_all_set(i32 session_id, u32 v_uint_1)
{
    return zl_3036x_trace_level_all_set(v_uint_1) ? ICLI_RC_OK : ICLI_RC_ERROR;
}

