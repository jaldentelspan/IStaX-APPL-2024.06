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
/* vtss_ptp_local_clock.h */

#ifndef VTSS_PTP_LOCAL_CLOCK_H
#define VTSS_PTP_LOCAL_CLOCK_H

#include "vtss_ptp_types.h"

#define ADJ_FREQ_MAX  100000 // +/- 100 ppm
#define ADJ_FREQ_MAX_LL  100000LL // +/- 100 ppm
#define ADJ_FREQ_MAX_LL_INDY_PHY 16000 // +/- 16 ppm
#define ADJ_OFFSET_MAX  10000000LL  // Max offset used in the servo (measured in ns)
#define ADJ_SCALE  (1<<16LL)        // Scale from ppb to the internal used scaled ppb
#define ADJ_1AS_FREQ_LOCK 10  //10ppb threshold for 802.1as frequency lock adjustments.

#define VTSS_PTP_LOCK_THRESHOLD 1000LL<<16   // Lock threshols is 1000 ns
#define VTSS_PTP_LOCK_PERIODS 5            // Lock periods is 5 periods
#define RATE_TO_U32_CONVERT_FACTOR (1LL<<41)

/**
 * \brief Get current PTP time.
 *
 * \param t [OUT] The variable to store the time in.
 *
 *
 * The TimeStamp format is defined by IEEE1588.
 */
bool vtss_local_clock_time_get(mesa_timestamp_t *t, int instance, u64 *hw_time);

/**
 * \brief Set current PTP time.
 *
 * \param t [IN] The variable holding the time.
 * \param time_domain [IN]  PTP clock domain no.
 *
 */
bool vtss_local_clock_time_set(const mesa_timestamp_t *t, u32 time_domain);

/**
 * \brief Set current PTP delta time.
 *
 * \param t [IN] The variable holding the time.
 * \param time_domain [IN]  PTP clock domain no.
 * \param negative [IN]     TRUE => negative delta time
 *
 * \return                  returns true if success else false.
 */
bool vtss_local_clock_time_set_delta(const mesa_timestamp_t *t, u32 time_domain, BOOL negative);

/**
 * \brief Convert a timestamp from the packet module to PTP time.
 * \param cur_time [IN]  number of timer ticks since startup.
 * \param t        [OUT] The variable to store the time in.
 * \param instance [IN]  PTP clock instance no.
 * \return               returns true if success else false.
 */
bool vtss_local_clock_convert_to_time(u64 cur_time, mesa_timestamp_t *t, int instance);

/**
 * \brief Convert a nanosec value to HW timecounter.
 * \param t [IN] The nanosecond value.
 * \param cur_time [OUT] corresponding HW timecounter.
 * \param instance [IN]  PTP clock instance no.
 */
void vtss_local_clock_convert_to_hw_tc(u32 ns, u64 *cur_time);

/**
 * \brief Get the clock timer ratio
 *
 */
i64 vtss_local_clock_ratio_get(u32 instance);

/**
 * \brief Adjust the clock timer ratio
 *
 * \param ratio Clock ratio frequency offset in units of scaled ppb
 *                            (parts pr billion) i.e. ppb*2*-16..
 *      ratio > 0 => clock runs faster
 * The function has a buildin slewrate function, that limits the adjustment change to max 1 PPM pr call.
 *
 */
void vtss_local_clock_ratio_set(i64 ratio, u32 instance);

/**
 * \brief Clear the clock timer ratio and the slewrate value
 *
 * \param instance Clock instance number
 *
 */
void vtss_local_clock_ratio_clear(u32 instance);

/**
 * \brief Adjust the clock timer offset
 *
 * \param offset Clock offset in ns.
 *      offset is subtracted from the actual time
 * 
 */
#ifdef __cplusplus
extern "C" {
#endif
bool vtss_local_clock_adj_offset(i32 offset, u32 domain);
bool vtss_local_clock_fine_adj_offset(i64 offset, u32 domain);
u32 vtss_local_clock_set_time_count_get(int instance);
bool vtss_local_clock_set_time_count_incr(u32 domain);
u32 vtss_domain_clock_set_time_count_get(u32 domain);
bool vtss_domain_clock_convert_to_time(u64 cur_time, mesa_timestamp_t *t, uint time_domain);
#ifdef __cplusplus
}
#endif

/**
 * \brief Get the time of the local clock in a domain.
 *
 * \param timing domain
 *
 */
bool vtss_domain_clock_time_get(mesa_timestamp_t *t, u32 time_domain, u64 *hw_time);
/**
 * \brief Get clock adjustment method
 *
 * \param instance Clock instance number.
 * \returns Clock option adjustment method (see values below)
 *
 */
#define CLOCK_OPTION_INTERNAL_TIMER 0       // Used if LTC is the only possible adjustment ot id the LTC preferred method is selected.
#define CLOCK_OPTION_PTP_DPLL       1       // Used if dual DPLL's are present and Independent mode is selected.
#define CLOCK_OPTION_DAC            2       // Used if an external DAC for frequency adjust is present and Independent mode is selected.
#define CLOCK_OPTION_SOFTWARE       3       // Used if neither LTC nor DPLL options are available.
#define CLOCK_OPTION_SYNCE_DPLL     4       // Used i Single mode is preferred and one or more DPLLs are available.

int vtss_ptp_adjustment_method(int instance);

#endif

