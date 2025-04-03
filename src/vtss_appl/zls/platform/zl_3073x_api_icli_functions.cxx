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

#include "icli_porting_util.h"
#include "zl_3073x_api_icli_functions.h"

#include "zl_3073x_api.h"
#include "zl_3073x_api_api.h"
#include "misc_api.h"
#include "zl_3038x_api_pdv_api.h"
#include "zl_3073x_synce_clock_api.h"

/***************************************************************************/
/*  Internal types                                                         */
/***************************************************************************/

/***************************************************************************/
/*  Internal functions                                                     */
/***************************************************************************/


icli_rc_t zl_3073x_api_icli_debug_read(i32 session_id, u32 mem_page, u32 mem_addr, u32 mem_size)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u32 i, n;
    u32 value;

    for (i=0; i<mem_size; i++)
    {
        if (zl_3073x_register_get(mem_page,  mem_addr+i,  &value) != VTSS_RC_OK) {
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

icli_rc_t zl_3073x_api_icli_debug_write(i32 session_id, u32 mem_page, u32 mem_addr, u32 value)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3073x_register_set(mem_page,  mem_addr,  value) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3073x_api_icli_debug_pll_status(i32 session_id)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3073x_debug_pll_status() != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3073x_api_icli_debug_ref_status(i32 session_id, u32 refid)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3073x_debug_hw_ref_status(refid) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3073x_api_icli_debug_ref_cfg(i32 session_id, u32 refid)
{

    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3073x_debug_hw_ref_cfg(refid) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3073x_api_icli_debug_dpll_status(i32 session_id, u32 pllid)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3073x_debug_dpll_status(pllid) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3073x_api_icli_debug_dpll_cfg(i32 session_id, u32 pllid)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (zl_3073x_debug_dpll_cfg(pllid) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t zl_3073x_api_icli_debug_trace_level_module_set(i32 session_id, u32 module, u32 level)
{
    return zl_3073x_trace_level_module_set(module, level) ? ICLI_RC_OK : ICLI_RC_ERROR;
}
//icli_rc_t zl_3073x_api_icli_debug_dco_freq_read(i32 session_id)
//{
//    i32 freqOffsetUppm;
//    icli_rc_t rc =  ICLI_RC_OK;
//    if (zl_3073x_debug_dco_freq_read(&freqOffsetUppm) != VTSS_RC_OK) {
//        rc = ICLI_RC_ERROR;
//    } else {
//        ICLI_PRINTF("DcoFreqOffset %d.%ld ppb\n", freqOffsetUppm/1000, labs(freqOffsetUppm%1000));
//    }
//    return rc;
//}
//
icli_rc_t zl_3073x_icli_spi_read(i32 session_id, u8 page, u16 addr, u8 cnt)
{
    u8 data[16],n = 0;

    data[0] = page;
    /* Set page number first */
    zl_3073x_spi_write(0x7f, &data[0], 1);

    ICLI_PRINTF("\n");
    for (int i = 0; i < cnt;i+=16) {
        int nread = 0;
        if (i+16 > cnt) {
            nread = cnt - i;
        } else {
            nread = 16;
        }
        memset(data, 0, sizeof(data));
        zl_3073x_spi_read(addr + i, data, nread);

        for (int j = 0 ; j < nread; j++) {
            n = ((i+j) & 0x7);
            if (n == 0)   ICLI_PRINTF("%03X: ", i+j+addr);
            ICLI_PRINTF("%02x ",data[j]);
            if (n == 0x7 || (i+j) == (cnt - 1))  ICLI_PRINTF("\n");
        }
    }

    return ICLI_RC_OK;
}

icli_rc_t zl_3073x_icli_spi_write(i32 session_id, u8 page, u16 addr, u8 data)
{
    /* Set page number first */
    zl_3073x_spi_write(0x7f, &page, 1);
    /* Write data */
    zl_3073x_spi_write(addr, &data, 1);

    return ICLI_RC_OK;
}

icli_rc_t zl_3073x_api_icli_print_mailbox_cfg(i32 session_id, bool is_ref, bool is_output, bool is_dpll, u8 id)
{
    if (is_ref) {
        ICLI_PRINTF("Input reference %u\n",id);
        zl_3073x_debug_print_ref_mailbox_cfg(id);
    } else if (is_output) {
        ICLI_PRINTF("Output id %u\n",id);
        zl_3073x_debug_print_output_mailbox_cfg(id);
    } else {
        ICLI_PRINTF("DPLL id %u\n",id);
        zl_3073x_debug_print_pll_mailbox_cfg(id);
    }
    return ICLI_RC_OK;
}

// Dump the zl30732 registers using ptp dpll
icli_rc_t zl_3073x_icli_regdump(i32 session_id)
{
    ICLI_PRINTF("\n  Dump registers using PTP dpll\n");
    zl_3073x_reg_dump_all();
    ICLI_PRINTF("\n  Dumped dpll registers\n");
    return ICLI_RC_OK;
}

// Configure 1PPS reference
void zl_3073x_icli_pps_ref_conf(i32 session_id, int ref, bool enable)
{
    zl_3073x_1pps_ref_conf(ref, enable);
}

// Configure reference frequency
icli_rc_t zl_3073x_icli_ref_freq_update(i32 session_id, uint8_t ref, uint32_t freq)
{
    zl303xx_RefIdE refId = (zl303xx_RefIdE)ref;
    if (zl_3073x_ref_input_freq_set(refId, freq) != MESA_RC_OK) {
        ICLI_PRINTF("reference updation not successful\n");
    }
    return ICLI_RC_OK;
}
// Configure dpll to lock to reference or set in NCO mode.
void zl_3073x_icli_force_lock_ref(i32 session_id, uint32_t dpll, int ref, bool enable)
{
    (void)zl_3073x_dpll_force_lock_ref(dpll, ref, enable);
}
