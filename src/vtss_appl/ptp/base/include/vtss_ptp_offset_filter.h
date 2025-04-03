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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef _VTSS_PTP_OFFSET_FILTER_H_
#define _VTSS_PTP_OFFSET_FILTER_H_

/**
 * \file vtss_ptp_offset_filter.h
 * \brief Define Offset filter callouts.
 *
 * This file contain the definitions of PTP Offset filter functions and
 * associated types.
 * The functions are indirectly called vis function pointers, defined in
 * vtss_ptp_offset_filter_t structure, and can easily be exchanged with customer
 * defined functions.
 *
 */

#include "vtss_ptp_types.h"

static const i64 max_acceptable_delay_variation = VTSS_SEC_NS_INTERVAL(0, 10000000);  /* 10 ms */

typedef enum {
    DONT_SET_VCXO_FREQ,
    SET_VCXO_FREQ
} vtss_ptp_set_vcxo_freq;

/**
 * \brief Clock Offset filter and servo parameter structure
 */
typedef struct vtss_ptp_offset_filter_param_s {
    mesa_timeinterval_t offsetFromMaster;
    mesa_timeinterval_t previousOffsetFromMaster;
    mesa_timestamp_t rcvTime;
    bool indyPhy;
} vtss_ptp_offset_filter_param_t;

/**
 * \brief Clock servo status structure
 */

typedef struct vtss_lowpass_filter_s {
    i64 nsec_prev;
    i64 y;
    i64 s_exp;
    i32 skipped;
    const char * my_name;
    vtss_lowpass_filter_s();
    vtss_lowpass_filter_s(const char * name);
    void reset();
    void filter(mesa_timeinterval_t *value, double period, bool min_delay_option, bool customBW);
} vtss_lowpass_filter_t;

typedef struct vtss_wl_delay_filter_s {
    mesa_timeinterval_t act_min_delay;
    mesa_timeinterval_t act_max_delay;
    mesa_timeinterval_t act_mean_delay;
    mesa_timeinterval_t prev_delay;
    u32 prev_cnt;
    u32 actual_period;
    u32 actual_dist;
} vtss_wl_delay_filter_t;

typedef struct vtss_ptp_servo_status_s {
    bool holdover_ok;
    i64 holdover_adj;
} vtss_ptp_servo_status_t;

#endif // _VTSS_PTP_OFFSET_FILTER_H_

