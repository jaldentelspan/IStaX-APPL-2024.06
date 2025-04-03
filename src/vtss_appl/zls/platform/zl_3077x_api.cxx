/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "zl_3077x_api_api.h"
#include "zl_3077x_api.h"
#include "synce_spi_if.h"

// xl3077x base interface
#include "zl303xx_Api.h"
#include "zl303xx_DebugMisc.h"
#include "zl303xx_AddressMap.h"
#include "zl303xx_AddressMap77x.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_Dpll771.h"
#include "zl303xx_Dpll77x.h"
#include "zl303xx_DebugDpll77x.h"
#undef min // Undefine these because they are causing problems in interrupt_api.h below
#undef max //
#include "interrupt_api.h"

/* interface to other components */
#include "vtss_tod_api.h"

#define ZL_3077X_RC_CHECK(expr) {                                       \
                                 zlStatusE _rc_ = (expr);            \
                                 if (_rc_ != ZL303XX_OK) {           \
                                     T_W("ZL Error code: %d", _rc_); \
                                     return VTSS_RC_OK;              \
                                 } else {                            \
                                     return VTSS_RC_ERROR;           \
                                 }                                   \
                             }


/* ================================================================= *
 *  Configuration definitions
 * ================================================================= */

/****************************************************************************
 * Configuration API
 ****************************************************************************/


mesa_rc zl_3077x_register_get(const u32   page,
                              const u32   reg,
                              u32 *const  value)
{
    *value = 0;
    ZL_3077X_RC_CHECK(zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZL303XX_MAKE_MEM_ADDR_77X((Uint32T)(page * (ZL303XX_MEM_ADDR_MASK_77X + 1) + reg), ZL303XX_MEM_SIZE_1_BYTE), (Uint32T *)value));
}

mesa_rc zl_3077x_register_set(const u32   page,
                              const u32   reg,
                              const u32   value)
{
    ZL_3077X_RC_CHECK(zl303xx_Write(zl_3077x_zl303xx_synce_dpll, NULL, ZL303XX_MAKE_MEM_ADDR_77X((Uint32T)(page * (ZL303XX_MEM_ADDR_MASK_77X + 1) + reg), ZL303XX_MEM_SIZE_1_BYTE), (Uint32T)value));
}

mesa_rc zl_3077x_debug_pll_status(void)
{
    printf("\nSynce dpll\n");

    if (zl303xx_DebugPllStatus77x(zl_3077x_zl303xx_synce_dpll) != ZL303XX_OK) {
        T_D("Error during SyncE dpll status get");
        return VTSS_RC_ERROR;
    }
    // ptp dpll status
    printf("\nPTP dpll\n");
    if (zl303xx_DebugPllStatus77x(zl_3077x_zl303xx_ptp_dpll) != ZL303XX_OK) {
        T_D("Error during PTP dpll status get");
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

mesa_rc zl_3077x_debug_hw_ref_status(u32 ref_id)
{
    ZL_3077X_RC_CHECK(zl303xx_DebugHwRefStatus77x(zl_3077x_zl303xx_synce_dpll, ref_id));
}

mesa_rc zl_3077x_debug_hw_ref_cfg(u32 ref_id)
{
    ZL_3077X_RC_CHECK(zl303xx_DebugHwRefCfg77x(zl_3077x_zl303xx_synce_dpll, ref_id));
}

mesa_rc zl_3077x_debug_dpll_status(u32 pll_id)
{
    mesa_rc rc = VTSS_RC_OK;
    u32 my_pll_id,pll_id_cpy,reg_value=0;
    zlStatusE status = ZL303XX_OK;

    my_pll_id = zl_3077x_zl303xx_synce_dpll->pllParams.pllId;
    zl_3077x_zl303xx_synce_dpll->pllParams.pllId = pll_id_cpy = pll_id - 1;

    if (status == ZL303XX_OK)
    {
        printf("\n");
        /* read dpll status */
        status = zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL,
                ZLS3077X_HOLDOVER_LOCK_STATUS_REG(pll_id_cpy),
                &reg_value);
        printf("DPLL[%u]\n",pll_id_cpy+1);
        printf("\t%-20s :: 0x%x\n","lock",ZLS3077X_LOCK_STATUS_DPLL_GET(reg_value));
        printf("\t%-20s :: 0x%x\n","holdover",ZLS3077X_HOLDOVER_STATUS_DPLL_GET(reg_value));
        printf("\t%-20s :: 0x%x\n","pull_in_hit",((reg_value & 0x20)>>5));
        printf("\t%-20s :: 0x%x\n","psl_hit",((reg_value & 0x80)>>7));

        status = zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL,
                ZL303XX_MAKE_MEM_ADDR_77X(0x0120 + ((pll_id_cpy) * 0x1), ZL303XX_MEM_SIZE_1_BYTE),
                &reg_value);
        printf("\t%-20s :: 0x%x\n","status",(reg_value & 0x7));
        printf("\t%-20s :: 0x%x\n","ref_in",((reg_value & 0xf0)>>4));
    }

    zl_3077x_zl303xx_synce_dpll->pllParams.pllId = my_pll_id;
    return(rc);
}

mesa_rc zl_3077x_debug_dpll_cfg(u32 pll_id)
{
    mesa_rc rc = VTSS_RC_OK;
    u32 my_pll_id;

    my_pll_id = zl_3077x_zl303xx_synce_dpll->pllParams.pllId;
    zl_3077x_zl303xx_synce_dpll->pllParams.pllId = pll_id - 1;
    if (zl303xx_DebugDpllConfig77x(zl_3077x_zl303xx_synce_dpll) != ZL303XX_OK) {
        T_D("Error during Zarlink dpll cfg get");
        rc = VTSS_RC_ERROR;
    }
    zl_3077x_zl303xx_synce_dpll->pllParams.pllId = my_pll_id;
    return(rc);
}

mesa_rc zl_3077x_debug_nco_freq_read(i32 *freqOffsetUppm)
{
    mesa_rc rc = VTSS_RC_OK;
    /* last argument is changed in 3077x, as of now passing one of th valid argument ZLS3077X_NORMAL_I_PART */
    if (zl303xx_Dpll77xGetFreq(zl_3077x_zl303xx_synce_dpll, freqOffsetUppm, ZLS3077X_NCO_I_PART)) {
        T_D("Error during Zarlink dpll cfg get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}
mesa_rc zl_3077x_debug_print_mailbox_cfg(bool is_ref, u32 id)
{
    mesa_rc rc = VTSS_RC_OK;
    if (is_ref) {
        ZL_3077X_RC_CHECK(zl303xx_DebugPrintMailbox77x(zl_3077x_zl303xx_synce_dpll , ZLS3077X_MB_ref, id ));
    } else {
        zl303xx77xDpllMailboxS mbData;
        u32 my_pll_id;
        zl303xx77xDpllMailboxS *p = &mbData;
        my_pll_id = zl_3077x_zl303xx_synce_dpll->pllParams.pllId;
        zl_3077x_zl303xx_synce_dpll->pllParams.pllId = id-1;
        if (zl303xx_Dpll77xSetupMailboxForRead(zl_3077x_zl303xx_synce_dpll, ZLS3077X_MB_DPLL, id-1) != ZL303XX_OK) {
            rc = VTSS_RC_ERROR;
        } else {
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_BW_FIXED_REG,              &(p->dpll_bw_fixed));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_BW_VAR_REG,                &(p->dpll_bw_var));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_CONFIG_REG,                &(p->dpll_config));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_PSL_REG,                   &(p->dpll_psl));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_PSL_MAX_PHASE_REG,         &(p->dpll_psl_max_phase));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_PSL_SCALING_REG,           &(p->dpll_psl_scaling));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_PSL_DECAY_REG,             &(p->dpll_psl_decay));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_RANGE_REG,                 &(p->dpll_range));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_REF_SW_MASK_REG,           &(p->dpll_ref_sw_mask));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_REF_HO_MASK_REG,           &(p->dpll_ref_ho_mask));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_DPER_SW_MASK_REG,          &(p->dpll_dper_sw_mask));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_DPER_HO_MASK_REG,          &(p->dpll_dper_ho_mask));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_REF_PRIO_0_REG,            &(p->dpll_ref_prio_0));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_REF_PRIO_1_REG,            &(p->dpll_ref_prio_1));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_REF_PRIO_2_REG,            &(p->dpll_ref_prio_2));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_REF_PRIO_3_REG,            &(p->dpll_ref_prio_3));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_REF_PRIO_4_REG,            &(p->dpll_ref_prio_4));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_DPER_PRIO_1_0_REG,         &(p->dpll_dper_prio_1_0));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_DPER_PRIO_3_2_REG,         &(p->dpll_dper_prio_3_2));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_HO_FILTER_REG,             &(p->dpll_ho_filter));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_HO_DELAY_REG,              &(p->dpll_ho_delay));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_SPLIT_XO_CONFIG_REG,       &(p->dpll_split_xo_config));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FAST_LOCK_CTRL_REG,        &(p->dpll_fast_lock_ctrl));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FAST_LOCK_PHASE_ERR_REG,   &(p->dpll_fast_lock_phase_err));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FAST_LOCK_FREQ_ERR_REG,    &(p->dpll_fast_lock_freq_err));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FAST_LOCK_IDEAL_TIME_REG,  &(p->dpll_fast_lock_ideal_time));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FAST_LOCK_NOTIFY_TIME_REG, &(p->dpll_fast_lock_notify_time));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FAST_LOCK_FCL_REG,         &(p->dpll_fast_lock_fcl));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FCL_REG,                   &(p->dpll_fcl));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_DAMPING_REG,               &(p->dpll_damping));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_PHASE_BAD_REG,             &(p->dpll_phase_bad));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_PHASE_GOOD_REG,            &(p->dpll_phase_good));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_GOOD_DURATION_REG,         &(p->dpll_duration_good));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_LOCK_DELAY_REG,            &(p->dpll_lock_delay));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_TIE_REG,                   &(p->dpll_tie));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_TIE_THRESH_REG,            &(p->dpll_tie_wr_thresh));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FP_FIRST_REALIGN_REG,      &(p->dpll_fp_first_realign));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FP_REALIGN_INTVL_REG,      &(p->dpll_fp_align_intvl));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_FP_LOCK_THRESH_REG,        &(p->dpll_fp_lock_thresh));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_STEP_TIME_THRESH_REG,      &(p->dpll_step_time_thresh));
            zl303xx_Read(zl_3077x_zl303xx_synce_dpll, NULL, ZLS3077X_DPLLX_STEP_TIME_RESO_REG,        &(p->dpll_step_time_reso));

            printf( "DPLL mailbox: %d\n", id );

            printf( "  p->dpll_bw_fixed                 %3d  (0x%x)\n", p->dpll_bw_fixed             , p->dpll_bw_fixed             );
            printf( "  p->dpll_bw_var                   %3d  (0x%x)\n", p->dpll_bw_var               , p->dpll_bw_var               );
            printf( "  p->dpll_config                   %3d  (0x%x)\n", p->dpll_config               , p->dpll_config               );
            printf( "  p->dpll_psl                      %3d  (0x%x)\n", p->dpll_psl                  , p->dpll_psl                  );
            printf( "  p->dpll_psl_max_phase            %3d  (0x%x)\n", p->dpll_psl_max_phase        , p->dpll_psl_max_phase        );
            printf( "  p->dpll_psl_scaling              %3d  (0x%x)\n", p->dpll_psl_scaling          , p->dpll_psl_scaling          );
            printf( "  p->dpll_psl_decay                %3d  (0x%x)\n", p->dpll_psl_decay            , p->dpll_psl_decay            );
            printf( "  p->dpll_range                    %3d  (0x%x)\n", p->dpll_range                , p->dpll_range                );
            printf( "  p->dpll_ref_sw_mask              %3d  (0x%x)\n", p->dpll_ref_sw_mask          , p->dpll_ref_sw_mask          );
            printf( "  p->dpll_ref_ho_mask              %3d  (0x%x)\n", p->dpll_ref_ho_mask          , p->dpll_ref_ho_mask          );
            printf( "  p->dpll_dper_sw_mask             %3d  (0x%x)\n", p->dpll_dper_sw_mask         , p->dpll_dper_sw_mask         );
            printf( "  p->dpll_dper_ho_mask             %3d  (0x%x)\n", p->dpll_dper_ho_mask         , p->dpll_dper_ho_mask         );
            printf( "  p->dpll_ref_prio_0               %3d  (0x%x)\n", p->dpll_ref_prio_0           , p->dpll_ref_prio_0           );
            printf( "  p->dpll_ref_prio_1               %3d  (0x%x)\n", p->dpll_ref_prio_1           , p->dpll_ref_prio_1           );
            printf( "  p->dpll_ref_prio_2               %3d  (0x%x)\n", p->dpll_ref_prio_2           , p->dpll_ref_prio_2           );
            printf( "  p->dpll_ref_prio_3               %3d  (0x%x)\n", p->dpll_ref_prio_3           , p->dpll_ref_prio_3           );
            printf( "  p->dpll_ref_prio_4               %3d  (0x%x)\n", p->dpll_ref_prio_4           , p->dpll_ref_prio_4           );
            printf( "  p->dpll_dper_prio_1_0            %3d  (0x%x)\n", p->dpll_dper_prio_1_0        , p->dpll_dper_prio_1_0        );
            printf( "  p->dpll_dper_prio_3_2            %3d  (0x%x)\n", p->dpll_dper_prio_3_2        , p->dpll_dper_prio_3_2        );
            printf( "  p->dpll_ho_filter                %3d  (0x%x)\n", p->dpll_ho_filter            , p->dpll_ho_filter            );
            printf( "  p->dpll_ho_delay                 %3d  (0x%x)\n", p->dpll_ho_delay             , p->dpll_ho_delay             );
            printf( "  p->dpll_split_xo_config          %3d  (0x%x)\n", p->dpll_split_xo_config      , p->dpll_split_xo_config      );
            printf( "  p->dpll_fast_lock_ctrl           %3d  (0x%x)\n", p->dpll_fast_lock_ctrl       , p->dpll_fast_lock_ctrl       );
            printf( "  p->dpll_fast_lock_phase_err      %3d  (0x%x)\n", p->dpll_fast_lock_phase_err  , p->dpll_fast_lock_phase_err  );
            printf( "  p->dpll_fast_lock_freq_err       %3d  (0x%x)\n", p->dpll_fast_lock_freq_err   , p->dpll_fast_lock_freq_err   );
            printf( "  p->dpll_fast_lock_ideal_time     %3d  (0x%x)\n", p->dpll_fast_lock_ideal_time , p->dpll_fast_lock_ideal_time );
            printf( "  p->dpll_fast_lock_notify_time    %3d  (0x%x)\n", p->dpll_fast_lock_notify_time, p->dpll_fast_lock_notify_time);
            printf( "  p->dpll_fast_lock_fcl            %3d  (0x%x)\n", p->dpll_fast_lock_fcl        , p->dpll_fast_lock_fcl        );
            printf( "  p->dpll_fcl                      %3d  (0x%x)\n", p->dpll_fcl                  , p->dpll_fcl                  );
            printf( "  p->dpll_damping                  %3d  (0x%x)\n", p->dpll_damping              , p->dpll_damping              );
            printf( "  p->dpll_phase_bad                %3d  (0x%x)\n", p->dpll_phase_bad            , p->dpll_phase_bad            );
            printf( "  p->dpll_phase_good               %3d  (0x%x)\n", p->dpll_phase_good           , p->dpll_phase_good           );
            printf( "  p->dpll_duration_good            %3d  (0x%x)\n", p->dpll_duration_good        , p->dpll_duration_good        );
            printf( "  p->dpll_lock_delay               %3d  (0x%x)\n", p->dpll_lock_delay           , p->dpll_lock_delay           );
            printf( "  p->dpll_tie                      %3d  (0x%x)\n", p->dpll_tie                  , p->dpll_tie                  );
            printf( "  p->dpll_tie_wr_thresh            %3d  (0x%x)\n", p->dpll_tie_wr_thresh        , p->dpll_tie_wr_thresh        );
            printf( "  p->dpll_fp_first_realign         %3d  (0x%x)\n", p->dpll_fp_first_realign     , p->dpll_fp_first_realign     );
            printf( "  p->dpll_fp_align_intvl           %3d  (0x%x)\n", p->dpll_fp_align_intvl       , p->dpll_fp_align_intvl       );
            printf( "  p->dpll_fp_lock_thresh           %3d  (0x%x)\n", p->dpll_fp_lock_thresh       , p->dpll_fp_lock_thresh       );
            printf( "  p->dpll_step_time_thresh         %3d  (0x%x)\n", p->dpll_step_time_thresh     , p->dpll_step_time_thresh     );
            printf( "  p->dpll_step_time_reso           %3d  (0x%x)\n", p->dpll_step_time_reso       , p->dpll_step_time_reso       );
        }
        zl_3077x_zl303xx_synce_dpll->pllParams.pllId = my_pll_id;
    }

    return(rc);
}

// Dump all 30772 registers using PTP dpll.
mesa_rc zl_3077x_reg_dump_all()
{
    zlStatusE status;

    status = zl303xx_DebugPrintAllRegs77x(zl_3077x_zl303xx_ptp_dpll);

    return (status == ZL303XX_OK) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

mesa_rc zl_3077x_trace_init(FILE *logFd)
{
    zl303xx_TraceInit(logFd);

    return VTSS_RC_OK;
}

mesa_rc zl_3077x_trace_level_module_set(u32 module, u32 level)
{
    zl303xx_TraceSetLevel(module, level);

    return VTSS_RC_OK;
}

mesa_rc zl_3077x_trace_level_all_set(u32 level)
{
    zl303xx_TraceSetLevelAll(level);

    return VTSS_RC_OK;
}

void zl_3077x_spi_write(u32 address, u8 *data, u32 size)
{
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_write(address, data, size);
}

void zl_3077x_spi_read(u32 address, u8 *data, u32 size)
{
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_read(address, data, size);
}

