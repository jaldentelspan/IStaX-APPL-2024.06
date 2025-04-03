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

#ifdef VTSS_SW_OPTION_ZLS3077X

#include "synce_dpll_zl3077x.h"
#include "synce_custom_clock_api.h"
#include "zl_3077x_synce_clock_api.h"
#include "zl_3077x_api_api.h"
#include "synce_dpll_trace.h"
#include "zl303xx_DeviceSpec.h"                         // This file is included to get the definition of zl303xx_ParamsS
#include "../zl30772_fw_update/zl303xx_Dpll77xFlash.h"  // for flashing of new firmware within DPLL (notice: dependant on zl303xx_DeviceSpec.h)
#include "synce_spi_if.h"                               // For vtss::synce::dpll:clock_chip_spi_if.spi_transfer()
#include <sys/stat.h>
#include <sys/types.h>

#define ZL3077x_FIRMWARE_UPDATE_UTIL_FILE          "/etc/mscc/dpll/firmware/zl30772.utility.hex"
#define ZL3077x_FIRMWARE_UPDATE_FILE1              "/etc/mscc/dpll/firmware/zl30772.firmware1.hex"
#define ZL3077x_FIRMWARE_UPDATE_FILE2              "/etc/mscc/dpll/firmware/zl30772.firmware2.hex"
#define ZL3077x_FIRMWARE_STATUS_DIR                "/switch/dpll.zl30772/" // Limit it to one directory level to avoid problems with mkdir()
#define ZL3077x_FIRMWARE_FORCE_UPDATE_FILE         ZL3077x_FIRMWARE_STATUS_DIR "dpll_force_update"
#define ZL3077x_FIRMWARE_UPDATE_COUNT_FILE         ZL3077x_FIRMWARE_STATUS_DIR "dpll_update_count"
#define ZL3077x_FIRMWARE_UPDATE_ATTEMPT_COUNT_FILE ZL3077x_FIRMWARE_STATUS_DIR "dpll_update_attempt_count"

#define CRIT_ENTER() critd_enter(&vtss::synce::dpll::crit, __FILE__, __LINE__)
#define CRIT_EXIT()  critd_exit( &vtss::synce::dpll::crit, __FILE__, __LINE__)

/*
 * This function implements the automatic mode if holdoff is configured.
 * I.e. When LOS is detected in automatic mode the switchover to an other source is postponed until the holdoff timer expires
 * pseudo code:
 * if automatic mode {
 *   if (los && holdoff timer active) keep selected source.
 *   if (los && holdof timer expired) select the best clock without active LOS.
 *   if new selected is not configured for holdoff, then enter automatic mode in hw
 * }
 */

mesa_rc synce_dpll_zl3077x::clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_clock_selector_state_get(selector_state, clock_input);
    CRIT_EXIT();
    //T_D("State for clock %d: %d", *clock_input, selector_state);
    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input)
{
    T_D("Set clock (%d) selection_mode to %d->%d", clock_input, (uint32_t)clock_selection_mode, (uint32_t)mode);
    CRIT_ENTER();
    clock_selection_mode = mode;
    mesa_rc rc = zl_3077x_clock_selection_mode_set(mode, clock_input);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_locs_state_get(const uint clock_input, bool *const state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_clock_los_get(clock_input, state);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_losx_state_get(bool *const state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_clock_losx_state_get(state);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_lol_state_get(bool *const state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_clock_lol_state_get(state);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_dhold_state_get(bool *const state)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_clock_dhold_state_get(state);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_event_poll(bool interrupt, clock_event_type_t *ev_mask)
{
    CRIT_ENTER();
    zl_3077x_clock_event_poll(interrupt, ev_mask, clock_my_input_max);
    CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc synce_dpll_zl3077x::clock_station_clk_out_freq_set(const u32 freq_khz)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_station_clk_out_freq_set(freq_khz);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_ref_clk_in_freq_set(const uint source, const u32 freq_khz)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_ref_clk_in_freq_set(source, freq_khz);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_eec_option_set(const clock_eec_option_t clock_eec_option)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_eec_option_set(clock_eec_option);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_features_get(sync_clock_feature_t *features)
{
    *features = SYNCE_CLOCK_FEATURE_DUAL_INDEPENDENT;

    return VTSS_RC_OK;
}

mesa_rc synce_dpll_zl3077x::clock_adj_phase_set(i32 adj)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_adj_phase_set(adj);
    T_D("adjust PTP DPLL phase %d, currently not implemented", adj);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_adjtimer_set(i64 adj)
{
    CRIT_ENTER();
    mesa_rc rc = zl_3077x_adjtimer_set(adj);
    T_D("adjust Synce DPLL " VPRI64d, adj);
    CRIT_EXIT();

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_adjtimer_enable(bool enable)
{
    mesa_rc rc = VTSS_RC_OK;

    CRIT_ENTER();
    if (enable) {
        zl3077x_clock_take_hw_nco_control();
    } else {
        zl3077x_clock_return_hw_nco_control();
    }
    T_D("adjust Synce DPLL enable %d ", enable);
    CRIT_EXIT();
    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_selector_map_set(const uint reference, const uint clock_input)
{
    CRIT_ENTER();
    /* set up the reference mapping in the DPLL */
    mesa_rc rc = zl_3077x_selector_map_set(reference, clock_input);
    CRIT_EXIT();

    return rc;
}

/*lint -sem(clock_startup,   thread_protected) ... We're protected */
mesa_rc synce_dpll_zl3077x::clock_startup(bool cold_init, bool pcb104_synce)
{
    /* Do zl30363 startup */
    return zl_3077x_clock_startup(cold_init, clock_my_input_max);
}

mesa_rc synce_dpll_zl3077x::clock_init(bool cold_init, void **device_ptr)
{
    mesa_rc rc;

    clock_my_input_max = CLOCK_INPUT_MAX;
    synce_my_prio_disabled = 0x0f;

    rc = zl_3077x_api_init(device_ptr);

    /* Do ZL30363 initialization */
    if (rc == VTSS_RC_OK) {
        rc = zl_3077x_clock_init(cold_init);
    }

    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_output_adjtimer_set(i64 adj)
{
    CRIT_ENTER();
    mesa_rc rc = zl3077x_clock_output_adjtimer_set(adj);
    T_I("adjust PTP DPLL " VPRI64d, adj);
    CRIT_EXIT();
    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_ptp_timer_source_set(ptp_clock_source_t source)
{
    CRIT_ENTER();
    mesa_rc rc = zl3077x_clock_ptp_timer_source_set(source);
//    T_W("Select PTP source %d", (int) source);
    CRIT_EXIT();
    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_nco_assist_set(bool enable)
{
    CRIT_ENTER();
    T_I("Hybrid mode %s", enable ? "enabled" : "disabled");
    zl_3077x_clock_nco_assist_set(enable);
    CRIT_EXIT();
    return VTSS_RC_OK;
}

mesa_rc synce_dpll_zl3077x::trace_init(FILE *logFd)
{
    return zl_3077x_trace_init(logFd);
}

mesa_rc synce_dpll_zl3077x::trace_level_module_set(u32 module, u32 level)
{
    return zl_3077x_trace_level_module_set(module, level);
}

mesa_rc synce_dpll_zl3077x::trace_level_all_set(u32 level)
{
    return zl_3077x_trace_level_all_set(level);
}

int32_t synce_dpll_zl3077x::fw_dpll_read(void *hwparams, void *arg, uint32_t address, uint16_t page, uint16_t offset, uint8_t size, uint32_t *value)
{
    // wrapper for read from chip
    // selects (writes) page on chip and then reads the actual data bytes from chip
    int32_t  i;
    uint32_t result = 0;
    uint8_t  rx_data[5];
    uint8_t page_byte = page;

    if (size > 4) {
        // size is never greater than 4, but just for sure, don't allow length > 4
        T_E("Size (%d) may not be greater than 4", size);
        return VTSS_RC_ERROR;
    }

    // select page (write)
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_write(0x7F, &page_byte, 1);

    memset(rx_data, 0, 4);
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_read(offset | 0x80, rx_data, size);

    // "extract" result
    for (i = 0; i < size; i++) {
        result <<= 8;
        result |= rx_data[i];
    }

    *value = result;
    return VTSS_RC_OK;
}

int32_t synce_dpll_zl3077x::fw_dpll_write(void *hwparams, void *arg, uint32_t address, uint16_t page, uint16_t offset, uint8_t size, uint32_t value)
{
    int32_t i;
    uint8_t tx_data[5];
    uint8_t page_byte = page;

    // wrapper for writing to chip.
    if (size > 4) {
        // size is never greater than 4, but just for sure, don't allow length > 4
        T_E("Size (%d) may not be greater than 4", size);
        return VTSS_RC_ERROR;
    }

    // selects page on chip and then writes the actual data bytes onto chip

    // select page (write)
    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_write(0x7F, &page_byte, 1);

    // write all bytes (up to 4) at once
    for (i = 0; i < size; i++) {
        tx_data[i] = (uint8_t)(value >> (size - i - 1) * 8);
    }

    vtss::synce::dpll::clock_chip_spi_if.zl_3034x_write(offset & 0x7F, tx_data, size);

    return VTSS_RC_OK;
}

void synce_dpll_zl3077x::fw_update_progress_info(void *arg, uint8_t progressPercent, const char *progressStr)
{
    printf("%3d%%: %s\n", progressPercent, progressStr);
}

void synce_dpll_zl3077x::fw_update_execute(const char *utility_path, const char *path, const char *path2)
{
    zl303xx_Dpll77xFlashBurnArgsT args;

    zl303xx_TraceInit(stdout);

    // Disable trace by setting level to 0. Set it to e.g. 2 to get a lot of
    // printf()-based trace.
    zl303xx_TraceSetLevel(ZL303XX_MOD_ID_PLL,    0);
    zl303xx_TraceSetLevel(ZL303XX_MOD_ID_RDWR,   0);
    zl303xx_TraceSetLevel(ZL303XX_MOD_ID_SYSINT, 0);

    zl303xx_Dpll77xFlashBurnStructInit(&args);

    args.uPath   = utility_path;
    args.fwPath  = path;
    args.fw2Path = path2;
    args.keepSynthesizersOn = ZL303XX_TRUE;

    /* Optional. Default TRUE. Set False for early porting testing (does not burn device) */
    args.burnEnabled = ZL303XX_TRUE;
    args.skipSanityCheck = ZL303XX_TRUE;

    args.readFn = fw_dpll_read;
    args.writeFn = fw_dpll_write;
    args.readWriteFnArg = (void *)board_instance;
    args.reportProgressFn = fw_update_progress_info;
    args.reportProgressFnArg = 0;

    ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_REM(args.skipIntegrityCheckMask, ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG0);
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_REM(args.skipIntegrityCheckMask, ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG1);
    ZL303XX_DPLL_77X_FLASH_INTEGRITY_MAP_REM(args.skipIntegrityCheckMask, ZL303XX_DPLL_77X_FLASH_INTEGRITY_CONFIG2);

    // Go! (takes approx 30 secs)
    zl303xx_Dpll77xFlashBurn(0, &args);
}

mesa_rc synce_dpll_zl3077x::fw_ver_from_disk(char **utility_path, char **path, char **path2, bool *force_update, meba_synce_clock_fw_ver_t *fw_ver_new)
{
    FILE     *pFile;
    FILE     *pUpgradeCountFile;
    char     line[32];
    uint32_t num_update_count;

    // verify that utility file is there
    if (access(ZL3077x_FIRMWARE_UPDATE_UTIL_FILE, R_OK) != 0) {
        // utility file doesn't exist
        return VTSS_RC_ERROR;
    }

    // so far, so good
    pFile = fopen(ZL3077x_FIRMWARE_UPDATE_FILE1, "r");
    if (pFile) {
        // firmware file exists - fetch version (from line 5)
        char line[32];
        for (uint32_t i = 0; i < 5; i++) {
            fgets(line, sizeof(line), pFile);
        }

        *fw_ver_new = strtol(line, 0, 16);
        T_I("New firmware version is %x", *fw_ver_new);
        fclose(pFile);
    } else {
        // firmware file doesn't exist
        return VTSS_RC_ERROR;
    }

    // we have both files - go on
    *utility_path = ZL3077x_FIRMWARE_UPDATE_UTIL_FILE;
    *path         = ZL3077x_FIRMWARE_UPDATE_FILE1;

    // see if the 2nd firmware file is there
    if (access(ZL3077x_FIRMWARE_UPDATE_FILE2, R_OK) == 0) {
        *path2 = ZL3077x_FIRMWARE_UPDATE_FILE2;
    } else {
        // fw2 file not present (this is ok; marked as non-existing)
        *path2 = NULL;
    }

    // Create the DPLL status directory if it doesn't exist.
    if (mkdir(ZL3077x_FIRMWARE_STATUS_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
        printf("Unable to create directory %s: error = %s\n", ZL3077x_FIRMWARE_STATUS_DIR, strerror(errno));
        return VTSS_RC_ERROR;
    }

    if (access(ZL3077x_FIRMWARE_FORCE_UPDATE_FILE, R_OK) == 0)  {
        // file present; delete it and set mark for force updating
        (void)remove(ZL3077x_FIRMWARE_FORCE_UPDATE_FILE);
        *force_update = true;
    } else {
        *force_update = false;
    }

    // Upgrade file count (used for stress testing multiple upgrades).
    // In order to perform this stress testing, the file must be created by hand
    // and contain the number of trials to run.
    pUpgradeCountFile = fopen(ZL3077x_FIRMWARE_UPDATE_COUNT_FILE, "r");
    if (pUpgradeCountFile) {
        num_update_count = 0;
        if (fgets(line, sizeof(line), pUpgradeCountFile)) {
            num_update_count = atoi(line);
        }

        fclose(pUpgradeCountFile);
        if (num_update_count > 0) {
            T_I("Test FW update count %d -> %d", num_update_count, num_update_count - 1);
            *force_update = true;
        }

        if (num_update_count > 1) {
            pUpgradeCountFile = fopen(ZL3077x_FIRMWARE_UPDATE_COUNT_FILE, "w");
            if (pUpgradeCountFile) {
                sprintf(line, "%d", num_update_count - 1);
                fwrite(line, 1, strlen(line), pUpgradeCountFile);
                fclose(pUpgradeCountFile);
            }
        } else {
            (void)remove(ZL3077x_FIRMWARE_UPDATE_COUNT_FILE);
        }
    }

    return VTSS_RC_OK;
}

bool synce_dpll_zl3077x::fw_update(char *err_str, size_t err_str_size)
{
    meba_synce_clock_fw_ver_t fw_ver_new;
    char                      *utility_path, *path, *path2;
    bool                      force_update;
    mesa_rc                   rc;
    FILE                      *attempt_count_file;
    char                      attempt_count_filename[128], line[32];
    uint32_t                  num_attempt_count;

    err_str[0] = '\0';

    // Read firmware version from chip
    if ((rc = meba_synce_spi_if_dpll_fw_ver_get(board_instance, &fw_ver)) == MESA_RC_OK) {
        T_I("F/W Version is %08x", fw_ver);
    } else {
        T_I("Can't fetch firmware version from DPLL");
    }

    fw_ver_ok = rc == MESA_RC_OK;

    // Currently, we only support update of 30772.
    if (clock_hw_id != MEBA_SYNCE_CLOCK_HW_ZL_30772) {
        T_I("Not a 30772 (%d), but a %d", MEBA_SYNCE_CLOCK_HW_ZL_30772, clock_hw_id);
        return false;
    }

    if (!fw_ver_ok) {
        T_I("Can't fetch firmware version from DPLL");
        return false;
    }

    if (fw_ver_from_disk(&utility_path, &path, &path2, &force_update, &fw_ver_new) != VTSS_RC_OK) {
        return false;
    }

    if (fw_ver_new <= fw_ver && !force_update) {
        snprintf(err_str, err_str_size, "ZL30772 DPLL firmware (v%x) is up-to-date. Version bundled with software is v%x.", fw_ver, fw_ver_new);
        T_I("%s", err_str);

        return false;
    }

    if (!force_update) {
        // If not force-updating, see how many times we've tried to upgrade to
        // this version of the F/W. If too many, stop the attempts (if force-
        // updating, we shouldn't do this check).

        // Construct the name of the file holding the number of attempts. In
        // order to support upgrade of future F/W versions, the filename must
        // include the current new F/W version number.
        sprintf(attempt_count_filename, "%s.%x", ZL3077x_FIRMWARE_UPDATE_ATTEMPT_COUNT_FILE, fw_ver_new);

        // fetch the number of attempts previously done
        attempt_count_file = fopen(attempt_count_filename, "r");
        num_attempt_count = 0;
        if (attempt_count_file) {
            if (fgets(line, sizeof(line), attempt_count_file)) {
                num_attempt_count = atoi(line);
            }

            fclose(attempt_count_file);
        }

        if (num_attempt_count >= 5) {
            snprintf(err_str, err_str_size, "Attempt to upgrade ZL30772 DPLL firmware from v%x to v%x has failed more than 5 times. Giving up.", fw_ver, fw_ver_new);
            T_I("%s", err_str);
            return false;
        } else {
            T_I("Attempt %u/5: upgrade DPLL from ver %x to %x", num_attempt_count + 1, fw_ver, fw_ver_new);
        }

        // increase attempt counter and write into file
        attempt_count_file = fopen(attempt_count_filename, "w");
        if (attempt_count_file) {
            // increase attempt with 1
            sprintf(line, "%d", num_attempt_count + 1);
            fwrite(line, 1, strlen(line), attempt_count_file);
            fclose(attempt_count_file);
        }
    }

    T_I("Use files:");
    T_I("Utility %s", utility_path);
    T_I("Fw %s",      path);
    T_I("Fw2 %s",     path2 ? path2 : "(not present)");

    if (force_update) {
        printf("ZL30772 DPLL F/W: Force-updating from v%x to v%x...\n", fw_ver, fw_ver_new);
    } else {
        // firmware on disk is higher than firmware within DPLL - go update it...
        printf("ZL30772 DPLL F/W: Updating from v%x to v%x...\n", fw_ver, fw_ver_new);
    }

    // Then update the firmware.
    fw_update_execute(utility_path, path, path2);

    // Upgrade was a success. Delete the attempt file if not force-updating.
    if (!force_update && access(attempt_count_filename, R_OK) != 0) {
        T_I("Success: Updated F/W in DPLL to ver %x in %u attempts", fw_ver_new, num_attempt_count);
        (void)remove(attempt_count_filename);
    }

    printf("DPLL F/W: Updated to v%x. Now rebooting...\n", fw_ver_new);

    // The caller reboots when we return true
    return true;
}

mesa_rc synce_dpll_zl3077x::clock_link_state_set(uint32_t clock_input, bool link)
{
    mesa_rc rc = MESA_RC_OK;
    CRIT_ENTER();
    rc = zl_3077x_clock_link_state_set(clock_input, link);
    CRIT_EXIT();
    return rc;
}

mesa_rc synce_dpll_zl3077x::clock_reset_to_defaults(void)
{
    T_D("zl3077x: clock_reset_to_defaults");
    return zl_3077x_clock_reset_to_defaults(clock_my_input_max);
}

#endif // VTSS_SW_OPTION_ZLS3077x

