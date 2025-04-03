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

// FIXME: Decide whether range check on clock_input parameter should be made in this file or in the classes derived from synce_dpll_base

#include "critd_api.h"
#include "main.h"
#include "microchip/ethernet/switch/api.h"
#include "board_if.h"
#include "misc_api.h"
#include <vtss_trace_lvl_api.h>
#include <stdio.h>
#include <dirent.h>
#include "synce_custom_clock_api.h"
#include "synce_omega_clock_api.h"  // FIXME: This is a "quick and dirty" hack. This should be moved into synce_dpll_servalt.c (see also below in clock_adjtimer_enable)
#include "control_api.h"
#if defined(VTSS_SW_OPTION_PTP)
#include "ptp_api.h"
#endif

#include "synce_dpll_base.h"
#include "synce_dpll_servalt.h"





#ifdef VTSS_SW_OPTION_ZLS30361
#include "synce_dpll_zl30363.h"
#endif // VTSS_SW_OPTION_ZLS30361

#ifdef VTSS_SW_OPTION_ZLS3077X
#include "synce_dpll_zl3077x.h"
#endif // VTSS_SW_OPTION_ZLS3077X

#ifdef VTSS_SW_OPTION_ZLS3073X
#include "synce_dpll_zl3073x.h"
#endif // VTSS_SW_OPTION_ZLS3077X

#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl303xx_DeviceSpec.h"             // This file is included to get the definition of zl303xx_ParamsS
extern zl303xx_ParamsS *zl303xx_Params_dpll;        // Pointers to zl303xx_ParamsS structures. Must be initialized by driver for zls30343, zls30363, Silabs or ServalT DPLL
extern zl303xx_ParamsS *zl303xx_Params_generic[4];  //
#endif

#include "synce_dpll_trace.h"

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "SyncE_DPLL", "SyncE DPLL module."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_OMEGA] = {
        "omega",
        "Omega interface",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define SYNCE_DPLL_CRIT_ENTER() critd_enter(&vtss::synce::dpll::crit, __FILE__, __LINE__)
#define SYNCE_DPLL_CRIT_EXIT()  critd_exit( &vtss::synce::dpll::crit, __FILE__, __LINE__)

static synce_dpll_servalt dpll_servalt;

#ifdef VTSS_SW_OPTION_ZLS30361
static synce_dpll_zl30363 dpll_zl30363;
#endif // VTSS_SW_OPTION_ZLS30361

#ifdef VTSS_SW_OPTION_ZLS3077X
static synce_dpll_zl3077x dpll_zl3077x;
#endif //VTSS_SW_OPTION_ZLS3077X

#ifdef VTSS_SW_OPTION_ZLS3073X
static synce_dpll_zl3073x dpll_zl3073x;
#endif //VTSS_SW_OPTION_ZLS3073X

synce_dpll_base *synce_dpll = 0;

#define SPI_SET_ADDRESS      0x00
#define SPI_WRITE            0x40
#define SPI_READ             0x80

static mesa_rc clock_startup(bool  cold_init, bool  pcb104_synce);


static void dpll_detect(void)
{
    meba_synce_clock_hw_id_t dpll_type = MEBA_SYNCE_CLOCK_HW_NONE;

    if (meba_synce_spi_if_get_dpll_type(board_instance, &dpll_type) != MESA_RC_OK) {
        dpll_type = MEBA_SYNCE_CLOCK_HW_NONE;
    }

    T_I("H/W DPLL type = %d", dpll_type);

    switch (dpll_type) {
    case MEBA_SYNCE_CLOCK_HW_SI_5328:
        vtss::synce::dpll::si5328 = true;
    // Fall through

    case MEBA_SYNCE_CLOCK_HW_SI_5326:
        vtss::synce::dpll::clock_chip_spi_if.si5326 = true;
        //synce_dpll = &dpll_si5326;
        // TGO: No longer supported.
        break;









#ifdef VTSS_SW_OPTION_ZLS30361
    case MEBA_SYNCE_CLOCK_HW_ZL_30363:
        vtss::synce::dpll::clock_chip_spi_if.zarlink = true;
        synce_dpll = &dpll_zl30363;
        break;
#endif

    case MEBA_SYNCE_CLOCK_HW_OMEGA:
        synce_dpll = &dpll_servalt;
        break;

#ifdef VTSS_SW_OPTION_ZLS3073X
    case MEBA_SYNCE_CLOCK_HW_ZL_30731:
    case MEBA_SYNCE_CLOCK_HW_ZL_30732:
    case MEBA_SYNCE_CLOCK_HW_ZL_30733:
    case MEBA_SYNCE_CLOCK_HW_ZL_30734:
    case MEBA_SYNCE_CLOCK_HW_ZL_30735:
        vtss::synce::dpll::clock_chip_spi_if.zarlink = true;
        synce_dpll = &dpll_zl3073x;
        break;
#endif

#ifdef VTSS_SW_OPTION_ZLS3077X
    case MEBA_SYNCE_CLOCK_HW_ZL_30771:
    case MEBA_SYNCE_CLOCK_HW_ZL_30772:
    case MEBA_SYNCE_CLOCK_HW_ZL_30773:
        vtss::synce::dpll::clock_chip_spi_if.zarlink = true;
        synce_dpll = &dpll_zl3077x;
        break;
#endif

    default:
        synce_dpll = NULL;
        break;
    }

    if (synce_dpll) {
        // Tell the object itself about its real H/W ID.
        synce_dpll->clock_hw_id = dpll_type;
    }
}

static mesa_rc clock_init(bool cold_init)
{
    if (synce_dpll) {
        void    *device_ptr;
        mesa_rc rc;

        rc = synce_dpll->clock_init(cold_init, &device_ptr);

#if defined(VTSS_SW_OPTION_ZLS30387)
        zl303xx_Params_dpll = (zl303xx_ParamsS *)device_ptr; // Pointer to a zl303xx_ParamsS structure. Must be initialized with value returned by driver for zls30343, zls30363, Silabs or ServalT DPLL

        for (int i = 0; i < 4; i++) {
            zl303xx_Params_generic[i] = (zl303xx_ParamsS *)malloc(sizeof(zl303xx_ParamsS));
            if (!zl303xx_Params_generic[i]) {
                T_W("Could not allocate memory for zl303xx_ParamsS structure");
            }
        }
#endif

        return rc;
    } else {
#if defined(VTSS_SW_OPTION_ZLS30387)
        zl303xx_Params_dpll = (zl303xx_ParamsS *)malloc(sizeof(zl303xx_ParamsS));
        if (!zl303xx_Params_dpll) {
            T_W("Could not allocate memory for zl303xx_ParamsS structure");
        }
        for (int i = 0; i < 4; i++) {
            zl303xx_Params_generic[i] = (zl303xx_ParamsS *)malloc(sizeof(zl303xx_ParamsS));
            if (!zl303xx_Params_generic[i]) {
                T_W("Could not allocate memory for zl303xx_ParamsS structure");
            }
        }
#endif

        return VTSS_RC_OK;
    }
}

/*lint -sem(clock_startup,   thread_protected) ... We're protected */
static mesa_rc clock_startup(bool cold_init, bool pcb104_synce)
{
    if (synce_dpll) {
        return synce_dpll->clock_startup(cold_init, pcb104_synce);
    } else {
        return VTSS_RC_ERROR;
    }
}

bool clock_zarlink(void)
{
    return vtss::synce::dpll::clock_chip_spi_if.zarlink;
}

mesa_rc clock_selection_mode_get(vtss_appl_synce_selection_mode_t *const mode)
{
    if (synce_dpll) {
        return synce_dpll->clock_selection_mode_get(mode);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input)
{
    if (synce_dpll) {
        return synce_dpll->clock_selection_mode_set(mode, clock_input);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state)
{
    static vtss_appl_synce_selector_state_t old_selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED;
    mesa_rc rc = VTSS_RC_OK;
    if (synce_dpll) {
        rc = synce_dpll->clock_selector_state_get(clock_input, selector_state);
    } else {
        // if no dpll present, we asume freerun, to satisfy coverity
        *selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
        rc = VTSS_RC_OK;
    }
    if (*selector_state != old_selector_state) {
        T_I("selector_state changed to %d", *selector_state);
        old_selector_state = *selector_state;
    }
    return rc;
}

mesa_rc clock_frequency_set(const uint clock_input, const meba_synce_clock_frequency_t frequency)
{
    if (synce_dpll) {
        return synce_dpll->clock_frequency_set(clock_input, frequency);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_locs_state_get(const uint clock_input, bool *const state)
{
    if (synce_dpll) {
        return synce_dpll->clock_locs_state_get(clock_input, state);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_losx_state_get(bool *const state)
{
    if (synce_dpll) {
        return synce_dpll->clock_losx_state_get(state);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_lol_state_get(bool *const state)
{
    if (synce_dpll) {
        return synce_dpll->clock_lol_state_get(state);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_dhold_state_get(bool *const state)
{
    if (synce_dpll) {
        return synce_dpll->clock_dhold_state_get(state);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_event_poll(bool interrupt, clock_event_type_t *ev_mask)
{
    if (synce_dpll) {
        return synce_dpll->clock_event_poll(interrupt, ev_mask);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_station_clk_out_freq_set(const u32 freq_khz)
{
    if (synce_dpll) {
        return synce_dpll->clock_station_clk_out_freq_set(freq_khz);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_ref_clk_in_freq_set(const uint source, const u32 freq_khz)
{
    if (synce_dpll) {
        return synce_dpll->clock_ref_clk_in_freq_set(source, freq_khz);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_station_clock_type_get(uint *const clock_type)
{
    if (synce_dpll) {
        return synce_dpll->clock_station_clock_type_get(clock_type);
    } else {
        // No station clock support
        *clock_type = 2;
        T_D("clock_type %d", *clock_type);
        return VTSS_RC_OK;
    }
}

mesa_rc clock_eec_option_type_get(uint *const eec_type)
{
    if (synce_dpll) {
        return synce_dpll->clock_eec_option_type_get(eec_type);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_features_get(sync_clock_feature_t *features)
{
    if (synce_dpll) {
        return synce_dpll->clock_features_get(features);
    } else {
        *features = SYNCE_CLOCK_FEATURE_NONE;
    }
    return VTSS_RC_OK;
}

mesa_rc clock_hardware_id_get(meba_synce_clock_hw_id_t *const clock_hw_id)
{
    if (synce_dpll) {
        return synce_dpll->clock_hardware_id_get(clock_hw_id);
    } else {
        *clock_hw_id = MEBA_SYNCE_CLOCK_HW_NONE;
        T_D("clock_hw id %d", *clock_hw_id);
        return VTSS_RC_OK;
    }
}

mesa_rc clock_adjtimer_set(i64 adj)
{
    if (synce_dpll) {
        return synce_dpll->clock_adjtimer_set(adj);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_adjtimer_enable(bool enable)
{
    if (synce_dpll) {
        return synce_dpll->clock_adjtimer_enable(enable);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_adj_phase_set(i32 adj)
{
    if (synce_dpll) {
        return synce_dpll->clock_adj_phase_set(adj);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_output_adjtimer_set(i64 adj)
{
    if (synce_dpll) {
        return synce_dpll->clock_output_adjtimer_set(adj);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_ptp_timer_source_set(ptp_clock_source_t source)
{
    if (synce_dpll) {
        return synce_dpll->clock_ptp_timer_source_set(source);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_eec_option_set(const clock_eec_option_t clock_eec_option)
{
    if (synce_dpll) {
        return synce_dpll->clock_eec_option_set(clock_eec_option);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_selector_map_set(const uint reference, const uint clock_input)
{
    if (synce_dpll) {
        return synce_dpll->clock_selector_map_set(reference, clock_input);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_ho_frequency_offset_get(i64 *const offset)
{
    if (synce_dpll) {
        return synce_dpll->clock_ho_frequency_offset_get(offset);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_read(const uint reg, uint *const value)
{
    if (synce_dpll) {
        return synce_dpll->clock_read(reg, value);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_write(const uint reg, const uint value)
{
    if (synce_dpll) {
        return synce_dpll->clock_write(reg, value);
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_nco_assist_set(bool enable)
{
    if (synce_dpll) {
        return synce_dpll->clock_nco_assist_set(enable);
    } else {
        return VTSS_RC_ERROR;
    }
}

static void system_reset(mesa_restart_t restart)
{
    if (synce_dpll) {
        if (VTSS_RC_OK != synce_dpll->clock_shutdown()) {
        }
    }
}

bool clock_dpll_fw_update(char *err_str, size_t err_str_size)
{
    if (synce_dpll) {
        return synce_dpll->fw_update(err_str, err_str_size);
    }

    return false;
}

mesa_rc clock_dpll_type_get(vtss_zl_30380_dpll_type_t *dpll_type)
{
    if (synce_dpll) {
        *dpll_type = synce_dpll->dpll_type();
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_dpll_fw_ver_get(meba_synce_clock_fw_ver_t *dpll_fw_ver)
{
    if (synce_dpll) {
        return synce_dpll->fw_ver_get(dpll_fw_ver);
    } else {
        *dpll_fw_ver = 0;
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_reset_to_defaults(void)
{
    if (synce_dpll) {
        return synce_dpll->clock_reset_to_defaults();
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc clock_link_state_set(uint32_t clock_input, bool link)
{
    if (synce_dpll) {
        return synce_dpll->clock_link_state_set(clock_input, link);
    } else {
        return VTSS_RC_ERROR;
    }
}

void synce_dpll_clock_lock_enter(void)
{
    SYNCE_DPLL_CRIT_ENTER();
}

void synce_dpll_clock_lock_exit(void)
{
    SYNCE_DPLL_CRIT_EXIT();
}

extern "C" int zl_30341_api_icli_cmd_register();
extern "C" int zl_30361_api_icli_cmd_register();
extern "C" int zl_3077x_api_icli_cmd_register();
extern "C" int zl_3073x_api_icli_cmd_register();

mesa_rc synce_dpll_init(vtss_init_data_t *data)
{
    mesa_restart_status_t status;
    u8 rx_buf[16], tx_buf[2];
    int i2c_cpld_dev, i2c_cpld_addr, i2c_silabs_dev, i2c_silabs_addr;

    switch (data->cmd) {
    case INIT_CMD_EARLY_INIT:
        // At this early stage, we need to detect if we have a DPLL, and if so
        // initiate a F/W update if required.
        dpll_detect();
        break;

    case INIT_CMD_INIT:
        /* Locking mechanism is initialized but not released - this is to protect any call to API until clock_startup has been done */
        critd_init(&vtss::synce::dpll::crit, "synce.clock", VTSS_MODULE_ID_SYNCE_DPLL, CRITD_TYPE_MUTEX);
        SYNCE_DPLL_CRIT_ENTER();








#ifdef VTSS_SW_OPTION_ZLS30361
        /* Initialize the trace system of the zls30361 driver. This happens no matter if the zls30363 module is actually detected. */
        (void) synce_dpll_zl30363::trace_init(stdout);
        (void) synce_dpll_zl30363::trace_level_all_set(0);
        zl_30361_api_icli_cmd_register();
#endif

#ifdef VTSS_SW_OPTION_ZLS3077X
        (void) synce_dpll_zl3077x::trace_init(stdout);
        (void) synce_dpll_zl3077x::trace_level_all_set(0);
        zl_3077x_api_icli_cmd_register();
#endif

#ifdef VTSS_SW_OPTION_ZLS3073X
        (void) synce_dpll_zl3073x::trace_init(stdout);
        (void) synce_dpll_zl3073x::trace_level_all_set(0);
        zl_3073x_api_icli_cmd_register();
#endif
        if (control_system_restart_status_get(NULL, &status) != VTSS_RC_OK) {
            T_D("control_system_restart_status_get() error returned");
        }
        if (clock_init((status.restart == MESA_RESTART_COLD)) != VTSS_RC_OK) {
            T_D("clock_init()  error returned");
        }

        /* This is checkking for Vitesse SyncE module (PCB104). If it is the CPLD must be initialized and the 5338 must be reset */
        i2c_cpld_dev = vtss_i2c_dev_open("synce_cpld", NULL, &i2c_cpld_addr);
        i2c_silabs_dev = vtss_i2c_dev_open("synce_silabs", NULL, &i2c_silabs_addr);
        if (i2c_cpld_dev >= 0 && i2c_silabs_dev >= 0) {
            tx_buf[0] = 1;
            if (vtss_i2c_dev_wr_rd(i2c_cpld_dev, i2c_cpld_addr, tx_buf, 1, rx_buf, 2) == VTSS_RC_OK) {
                T_D("received correctly PCB104 CPLD Version %X  Device %x", rx_buf[0], rx_buf[1]);
                if ((rx_buf[0] >= 0x03) && (rx_buf[1] == 0x33)) {   /* This PCB104 SyncE */
                    T_D("PCB104 CPLD Version accepted");
                    vtss::synce::dpll::pcb104_synce = true;
                    if (rx_buf[0] < 0x0A) {   /* This PCB104 SyncE has an old Firmware thes causes reboot problems on the PCB107*/
                        T_E("Please upgrade PCB104 CPLD Version to 10 or greater, older versions causes reboot problems");
                    }
                    tx_buf[0] = 16; /* Set CPLD SPI selector to Si5326 */
                    if (vtss_i2c_dev_wr_rd(i2c_cpld_dev, i2c_cpld_addr, tx_buf, 1, rx_buf, 1) == VTSS_RC_OK) {
                        rx_buf[0] &= ~0x1C;
                        rx_buf[0] |= 0x18;
                        tx_buf[1] = rx_buf[0];
                        if (vtss_i2c_dev_wr(i2c_cpld_dev, tx_buf, 2) != VTSS_RC_OK) {
                            T_D("Failed SPI selector set to SI5326  %X", tx_buf[1]);
                        }
                    } else {
                        T_D("Failed CPLD SPI selector get");
                    }
                    tx_buf[0] = 0xF6;  /* This is reset of Silabs 5338 */
                    tx_buf[1] = 2;
                    if (vtss_i2c_dev_wr(i2c_silabs_dev, tx_buf, 2) != VTSS_RC_OK) {
                        T_D("Failed reset of SI5338");
                    }
                }
            } else {
                T_D("faild read of PCB104 CPLD Version %X  Device %x", rx_buf[0], rx_buf[1]);
            }
        } else {
            T_I("No synce device found!");
        }

        vtss_i2c_dev_close(i2c_cpld_dev);
        vtss_i2c_dev_close(i2c_silabs_dev);
        if (clock_startup(true, vtss::synce::dpll::pcb104_synce) != VTSS_RC_OK) {
            T_D("error returned");
        }

        SYNCE_DPLL_CRIT_EXIT();

        T_D("INIT_CMD_INIT synce_dpll" );
        break;

    case INIT_CMD_START:
        control_system_reset_register(system_reset, VTSS_MODULE_ID_SYNCE_DPLL);
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}
