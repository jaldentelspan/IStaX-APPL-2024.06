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

#ifndef _SYNCE_OMEGA_API_H_
#define _SYNCE_OMEGA_API_H_

/* Get common definitions, types and macros */
#include "microchip/ethernet/switch/api.h"
#include <main_types.h>
#include <vtss/appl/synce.h>
#include "main.h"
#include "synce_types.h"
#include "synce_types.h"
#include "synce_custom_clock_api.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 * Description: Initialize Clock API.
 *
 * \param setup (input)
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_init(bool  cold_init);



/******************************************************************************
 * Description: Startup the Clock Controller.
 *
 * \param setup (input)
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_startup(bool  cold_init, bool  pcb104_synce);

/******************************************************************************
 * Description: Set Clock selection mode
 *
 * \param mode (input)              : Mode of clock selection.
 * \param clock_input (input)   : Clock input in manual mode.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_selection_mode_set(const vtss_appl_synce_selection_mode_t mode, const uint clock_input, uint clock_my_input_max);

mesa_rc vtss_omega_clock_selector_state_get(uint *const clock_input, vtss_appl_synce_selector_state_t *const selector_state);

/******************************************************************************
 * Description: Set Clock frequency
 *
 * \param clock_input (input)   : Clock input port number
 * \param frequency (input)     : Frequency for this clock input.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_frequency_set(const uint clock_input, const meba_synce_clock_frequency_t frequency, uint clock_my_input_max);


/******************************************************************************
 * Description: Set Clock input port priority
 *
 * \param clock_input (input)   : Clock input port number
 * \param priority (input)      : Priority - 0 is highest
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_priority_set(const uint clock_input, const uint priority, uint clock_my_input_max, uint synce_my_prio_disabled);


/******************************************************************************
 * Description: Get Clock input port priority
 *
 * \param clock_input (input)   : Clock input port number
 * \param priority (output)     : Priority - 0 is highest
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_priority_get(const uint clock_input, uint *const priority, uint clock_my_input_max, uint synce_my_prio_disabled);


/******************************************************************************
 * Description: Set Clock hold off time
 *
 * \param clock_input (input)   : Clock input port number
 * \param holdoff (input)       : Zero is no holdoff. Hold Off time must be between 300 and 1800 ms.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_holdoff_time_set(const uint clock_input, const uint ho_time, uint clock_my_input_max);


/******************************************************************************
 * Description: Clock hold off timer run
 *
 * \param active (output)       : true means some clock source hold off timer still need to run
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_holdoff_run(bool *const active);



/******************************************************************************
 * Description: Clock hold off event - activating control of LOCS hold off timing
 *
 * \param clock_input (input)   : Clock input port number
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_holdoff_event(const uint   clock_input);     /* clock_input range is 0 - CLOCK_INPUT_MAX */



/******************************************************************************
 * Description: Clock hold off timer active get
 *
 * \param clock_input (input)   : Clock input port number
 * \param active (output)       : true means this clock source hold off timer is still active
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_holdoff_active_get(const uint   clock_input,     /* clock_input range is 0 - CLOCK_INPUT_MAX */
                                            bool         *const active);

/******************************************************************************
 * Description: get LOCS state
 *
 * \param clock_input (input)      : clock input port number
 * \param state (output)           : LOCS state
 *
 * \return : Return code.
 ******************************************************************************/

mesa_rc vtss_omega_clock_locs_state_get(const uint   clock_input,     /* clock_input range is 0 - SYNCE_CLOCK_MAX */
                                        bool         *const state);



/******************************************************************************
 * Description: get FOS state
 *
 * \param clock_input (input)       : clock input port number
 * \param state (output)                : FOS state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_fos_state_get(const uint   clock_input,
                                       bool         *const state);

/******************************************************************************
 * Description: get LOSX state
 *
 * \param state (output)     : LOSX state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_losx_state_get(bool         *const state);

/******************************************************************************
 * Description: get LOL state
 *
 * \param state (output)     : LOL state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_lol_state_get(bool         *const state);

/******************************************************************************
 * Description: get Digital Hold Valid state
 *
 * \param state (output)     : DHOLD state
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_dhold_state_get(bool         *const state);

/******************************************************************************
 * Description: Set station clock output frequency
 *
 * \param freq_khz (IN)     : frequency in KHz, the frequency is rounded to the closest multiple og 8 KHz.
 *                                              freq_khz < 8 => clockoutput is disabled
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_station_clk_out_freq_set(const u32 freq_khz);

/******************************************************************************
 * Description: Set station clock input frequency
 *
 * \param freq_khz (IN)     : frequency in KHz, the frequency is rounded to the closest multiple og 8 KHz.
 *                                              freq_khz < 8 => clockinput is disabled
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_station_clk_in_freq_set(const u32 freq_khz);

/******************************************************************************
 * Description: Get station clock type
 *
 * \param clock_type (OUT)     : 0 = Full featured clock type, i.e. supports both in and out, and 1,544, 2,048 and 10 MHz
 *                             : 1 = PCB104, support only 2,048 and 10 MHz clock output
 *                             : 2 = others, no station clock support
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_station_clock_type_get(uint *const clock_type);

/******************************************************************************
 * Description: Get eec option type
 *
 * \param eec_type (OUT)       : 0 = Full featured clock type, i.e. supports both EEC option 1 and 2
 *                             : 1 = PCB104, support only EEC option 1
 *                             : 2 = others, no EEC option support
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_eec_option_type_get(uint *const eec_type);

/******************************************************************************
 * Description: Get station clock features
 *
 * \param features (OUT)    : 0 = No Synce clock function is present
 *                          : 2 = Clock adjustment feature present
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_features_get(uint *features);

/******************************************************************************
 * Description: Enable/Disable Time over packet frequency adjustment
 *
 * \param enable (IN)           : true = Enable clock adjustment
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_adjtimer_enable(bool enable);

/******************************************************************************
 * Description: Set Controller DCO frequency adjust value
 *
 * \param adj [IN]          : Clock ratio frequency offset in units of scaled ppb
 *                            (parts pr billion) i.e. ppb*2*-16.
 *                            ratio > 0 => clock runs faster.
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_adjtimer_set(i64 adj);

/******************************************************************************
 * Description: Make phase adjust
 *
 * \param phase [IN]        : Clock phase offset in units of scaled ns
 *                            i.e. ns*2*-16.
 *                            phase is added to actual phase i.e. > 0 => clock advances.
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_adj_phase_set(i32 phase);

/******************************************************************************
 * Description: Set output frequency adjust value
 *
 * \param adj [IN]          : Clock ratio frequency offset in units of scaled ppb
 *                            (parts pr billion) i.e. ppb*2*-16.
 *                            ratio > 0 => clock runs faster.
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_output_adjtimer_set(i64 adj);

mesa_rc vtss_omega_clock_ptp_timer_source_set(ptp_clock_source_t source);

/******************************************************************************
 * Description: Set station clock selector map
 *
 * \param reference (IN)        : Define the board specific reference number used in in the synce module for the clock_input
 * \param clock_input (IN)      : clock input [0..2] selected by the reference
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_selector_map_set(const uint reference,
                                          const uint clock_input);

/******************************************************************************
 * Description: get Clock frequency holdover offset
 *
 * \param offset [OUT]    Current frequency offset stored in the holdover stack in units of scaled ppb (parts per billion) i.e. ppb*2**-16.
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_ho_frequency_offset_get(i64 *const offset);

/******************************************************************************
 * Description: Set wait to restore time
 *
 * \param wtr_time (IN)      : wait to restore time in seconds [0..720]
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_wtr_set(u32 wtr_time);

/******************************************************************************
 * Description: Get wait to restore time
 *
 * \param wtr_time (OUT)      : wait to restore time in seconds [0..720]
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_wtr_get(u32 *const wtr_time);

/******************************************************************************
 * Description: Read value from Clock register.
 *
 * \param reg (input)      : Clock register address.
 * \param value (output) : Register value.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_read(const uint  reg,
                              uint        *const value);

/******************************************************************************
 * Description: Write value to Clock register.
 *
 * \param reg (input)     : Clock register address.
 * \param value (input)  : Register value.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_write(const uint    reg,
                               const uint    value);

/******************************************************************************
 * Description: Read, modify and write value to Clock register.
 *
 * \param reg (input)     : Clock register address.
 * \param value  (input) : Register value.
 * \param mask (input)   : Register mask, only bits enabled are changed.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_writemasked(const uint     reg,
                                     const uint     value,
                                     const uint     mask);

/******************************************************************************
 * Description: Set clock EEC option.
 *
 * \eec_option (input)   : Clock EEC option.
 *
 * \return : Return code.
 ******************************************************************************/
mesa_rc vtss_omega_clock_eec_option_set(const clock_eec_option_t clock_eec_option);

mesa_rc vtss_omega_clock_shutdown();

#ifdef __cplusplus
}
#endif
#endif /* _SYNCE_OMEGA_API_H_*/
