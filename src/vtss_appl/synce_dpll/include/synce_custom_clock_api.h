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

#ifndef _CLOCK_API_H_
#define _CLOCK_API_H_

/* Get common definitions, types and macros */
#include "microchip/ethernet/switch/api.h"
#include <main_types.h>
#include <vtss/appl/synce.h>
#include "synce_types.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Description: Detect the DPLL hardware available
 *
 * \param dpll_type (output)
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc get_dpll_type(meba_synce_clock_hw_id_t *dpll_type);

bool clock_zarlink(void);

/******************************************************************************
 * Description: Get Clock selection mode
 *
 * \param mode (output)         : Mode of clock operation
 *
 * \return : Mode of clock selection.
 ******************************************************************************/
mesa_rc clock_selection_mode_get(vtss_appl_synce_selection_mode_t *const mode);

/******************************************************************************
 * Description: Set Clock selection mode
 *
 * \param mode (input)          : Mode of clock selection.
 * \param clock_input (input)   : Clock input in manual mode.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode,
                                 const uint clock_input);

/******************************************************************************
 * Description: Get Clock Selector State
 *
 * \param clock_input (input)      : Clock input port number
 * \param selector_state (outputt) : Clock selector state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state);

/******************************************************************************
 * Description: Set Clock frequency
 *
 * \param clock_input (input)   : Clock input port number
 * \param frequency (input)     : Frequency for this clock input.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_frequency_set(const uint clock_input, const meba_synce_clock_frequency_t frequency);

/******************************************************************************
 * Description: get LOCS state
 *
 * \param clock_input (input)      : clock input port number - range is 0 to CLOCK_INPUT_MAX
 * \param state (output)           : LOCS state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_locs_state_get(const uint clock_input, bool *const state);

/******************************************************************************
 * Description: get LOSX state
 *
 * \param state (output)     : LOSX state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc   clock_losx_state_get(bool *const state);

/******************************************************************************
 * Description: get LOL state
 *
 * \param state (output)     : LOL state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_lol_state_get(bool *const state);

/******************************************************************************
 * Description: get Digital Hold Valid state
 *
 * \param state (output)     : DHOLD state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_dhold_state_get(bool *const state);

mesa_rc clock_event_poll(bool interrupt, clock_event_type_t *ev_mask);

/******************************************************************************
 * Description: Set station clock output frequency
 *
 * \param freq_khz (IN)     : frequency in KHz, the frequency is rounded to the closest multiple og 8 KHz.
 *                                              freq_khz < 8 => clockoutput is disabled
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_station_clk_out_freq_set(const u32 freq_khz);

/******************************************************************************
 * Description: Set DPLL input frequency
 *
 * \param freq_khz (IN)     : frequency in KHz, the frequency is rounded to the closest multiple og 8 KHz.
 *                                              freq_khz < 8 => clockinput is disabled
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_ref_clk_in_freq_set(const uint source, const u32 freq_khz);

/******************************************************************************
 * Description: Get station clock type
 *
 * \param clock_type (OUT)     : 0 = Full featured clock type, i.e. supports both in and out, and 1,544, 2,048 and 10 MHz
 *                             : 1 = PCB104, support only 2,048 and 10 MHz clock output
 *                             : 2 = others, no station clock support
 *                             : 3 = ServalT, supports in frequency 1,544, 2,048 and 10 MHz, support only 10 MHz clock output
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_station_clock_type_get(uint *const clock_type);

/******************************************************************************
 * Description: Get eec option type
 *
 * \param eec_type (OUT)       : 0 = Full featured clock type, i.e. supports both EEC option 1 and 2
 *                             : 1 = PCB104, support only EEC option 1
 *                             : 2 = others, no EEC option support
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_eec_option_type_get(uint *const eec_type);

/******************************************************************************
 * Description: Get station clock features
 *
 * \param features (OUT)    : Available clock features as defined in sync_clock_feature_t
 * \return : Return code.
 ******************************************************************************/
/* Enum defining values for the clock features.
 */
typedef enum {
    SYNCE_CLOCK_FEATURE_NONE,                      /**< No Synce clock function is present */
    SYNCE_CLOCK_FEATURE_SINGLE,                    /**< Single DPLL Clock adjustment feature present (ZL30343) */
    SYNCE_CLOCK_FEATURE_DUAL,                      /**< Separate Synce Clock adjustment and PTP clock adjustment feature present (ServalT) */
    SYNCE_CLOCK_FEATURE_DUAL_INDEPENDENT           /**< Separate Synce Clock adjustment and PTP clock adjustment feature present, but only in INDEPENDENT mode. (ZL30363) */
} sync_clock_feature_t;

mesa_rc clock_features_get(sync_clock_feature_t *features);

/******************************************************************************
 * Description: Get station clock hardware id
 *
 * \param clock_hw_id (OUT) : Actual Synce hardware detected
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_hardware_id_get(meba_synce_clock_hw_id_t *const clock_hw_id);

/******************************************************************************
 * Description: Set frequency adjust value
 *
 * \param adj [IN]          : Clock ratio frequency offset in units of scaled ppb
 *                            (parts pr billion) i.e. ppb*2*-16.
 *                            ratio > 0 => clock runs faster.
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_adjtimer_set(i64 adj);

/******************************************************************************
 * Description: Enable/Disable frequency adjust control
 *
 * \param enable [IN]       : true  => DCO control enable
 *                            false => DCO control disable (i.e. electrical control enabled)
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_adjtimer_enable(bool enable);

/******************************************************************************
 * Description: Enable/disable NCO assist mode
 *
 * \param enable [IN]        : Enable or disable NCO assist mode, choose NCO
 *                            assist DPLL.
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_nco_assist_set(bool enable);


/******************************************************************************
 * Description: Make phase adjust
 *
 * \param phase [IN]        : Clock phase offset in units of scaled ns
 *                            i.e. ns*2*-16.
 *                            phase is added to actual phase i.e. > 0 => clock advances.
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_adj_phase_set(i32 adj);

mesa_rc clock_output_adjtimer_set(i64 adj);


mesa_rc clock_ptp_timer_source_set(ptp_clock_source_t source);

/******************************************************************************
 * Description: Set station clock selector map
 *
 * \param reference (IN)        : Define the board specific reference number used in in the synce module for the clock_input
 * \param clock_input (IN)      : clock input [0..2] selected by the reference
 * \return : Return code.
 ******************************************************************************/
#define CLOCK_REF_0 0
#define CLOCK_REF_1 1
#define CLOCK_REF_2 2
#define CLOCK_REF_3 3
#define CLOCK_REF_4 4
#define CLOCK_REF_5 5
#define CLOCK_REF_6 6
#define CLOCK_REF_7 7
#define CLOCK_REF_8 8
#define CLOCK_REF_INVALID 9

mesa_rc clock_selector_map_set(const uint reference, const uint clock_input);

/******************************************************************************
 * Description: get Clock frequency holdover offset.
 *
 * \param offset [OUT]    Current frequency offset stored in the holdover stack in units of scaled ppb (parts per billion) i.e. ppb*2**-16.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_ho_frequency_offset_get(i64 *const offset);

/******************************************************************************
 * Description: Read value from Clock register.
 *
 * \param reg (input)      : Clock register address.
 * \param value (output) : Register value.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_read(const uint reg, uint *const value);

/******************************************************************************
 * Description: Write value to Clock register.
 *
 * \param reg (input)     : Clock register address.
 * \param value (input)  : Register value.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_write(const uint reg, const uint value);

/******************************************************************************
 * Description: Set clock EEC option.
 *
 * \eec_option (input)   : Clock EEC option.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_eec_option_set(const clock_eec_option_t clock_eec_option);

/******************************************************************************
 * Description: Check if DPLL needs a F/W update, and if so, update it right
 * away and return true once done. Otherwise return false.
 *
 * The err_str allows the F/W update process to issue an error message to the
 * syslog, which is not callable at the point in time the F/W update process
 * runs.
 ******************************************************************************/
bool clock_dpll_fw_update(char *err_str, size_t err_str_size);

/******************************************************************************
 * Description: Get the type of DPLL detected
 *
 * \param dpll_type (output)   : The type of the DPLL
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_dpll_type_get(vtss_zl_30380_dpll_type_t *dpll_type);

/******************************************************************************
 * Description: Get the firmware version of DPLL installed
 *
 * \param dpll_fw_ver (output)   : Version of the firmware inside the DPLL
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_dpll_fw_ver_get(meba_synce_clock_fw_ver_t *dpll_fw_ver);

/******************************************************************************
 * Description: Set los state in dpll based on port state
 *
 * \param clock_input(input) : clock input using the link
 * \param link       (input) : Set link up or down state.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_link_state_set(uint32_t clock_input, bool link);
/******************************************************************************
 * Description: Initialization function for the synce_dpll module
 *
 * \param data : Pointer to structure of type vtss_init_data_t
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc synce_dpll_init(vtss_init_data_t *data);

/******************************************************************************
 * Description: Trigged when "reload defaults" is executed. Gives possibility
 * to reset/re-init clock after reload of defaults.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc clock_reset_to_defaults(void);

void synce_dpll_clock_lock_enter(void);

void synce_dpll_clock_lock_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* _CLOCK_API_H_ */
