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

#ifndef PTP_LOCAL_CLOCK_H
#define PTP_LOCAL_CLOCK_H

#include "vtss_ptp_types.h"
#include "ptp_api.h"

#define VTSS_PTP_SW_CLK_DOMAIN_CNT 4
#define VTSS_PTP_MAX_CLOCK_DOMAINS 7 // VTSS_PTP_SW_CLK_DOMAIN_CNT(4) + (max chip clock domains(3))
#define VTSS_PTP_SRC_CLK_DOMAIN_NONE -1

// Software clock data for each clock domain.
// in a sw based timer, actual time = hw_time + drift + ptp_offset
//                      drift += (hw_time - t0)*ratio
typedef struct {
    mesa_timestamp_t        t0;
    mesa_timeinterval_t     drift;
    mesa_timeinterval_t     ratio;
    i32                     ptp_offset;
    int64_t                 adj;
    uint32_t                set_time_count;
    uint32_t                ppsDomain;
    int64_t                 ppsProcDelay;
    uint32_t                ppsSyncCnt;
} localClockData_t;

/**
 * \brief Initialize local clock.
 *
 */
void vtss_local_clock_initialize();

mesa_rc vtss_local_clock_adj_method(uint32_t instance, vtss_appl_ptp_preferred_adj_t adj_method, vtss_appl_ptp_profile_t profile, uint32_t clk_domain, bool basic_servo);

void ptp_local_clock_critd_init();

bool vtss_local_clock_soft_data_get(uint32_t inst, localClockData_t *data);

void vtssLocalClockReset(uint32_t inst);

bool vtssLocalClockPpsConfSet(uint32_t inst, uint32_t ppsDomain, bool set);

bool vtss_local_clock_src_clk_domain_set(uint32_t clkDomain, int32_t srcClkDomain);

int vtss_local_clock_src_clk_domain_get(uint32_t inst);
#endif
