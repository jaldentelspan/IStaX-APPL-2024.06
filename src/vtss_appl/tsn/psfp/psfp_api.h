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

#ifndef _PSFP_API_H_
#define _PSFP_API_H_

#include <vtss/appl/psfp.h>

typedef enum {
    PSFP_UTIL_TIME_UNIT_NSEC,
    PSFP_UTIL_TIME_UNIT_USEC,
    PSFP_UTIL_TIME_UNIT_MSEC,
} psfp_util_time_unit_t;

typedef struct {
    uint32_t              numerator;
    psfp_util_time_unit_t unit;
} psfp_util_time_t;

// The following two functions convert a time measured in nanoseconds to a
// numerator and a symbolic denominator (units) and vice versa.
void    psfp_util_time_to_num_denom(  uint32_t time_ns,        psfp_util_time_t &psfp_time);
mesa_rc psfp_util_time_from_num_denom(uint32_t &time_ns, const psfp_util_time_t &psfp_time);

// Convert various enums to string.
const char *psfp_util_time_unit_to_str(psfp_util_time_unit_t unit);
const char *psfp_util_gate_state_to_str(vtss_appl_psfp_gate_state_t gate_state, bool capitals = false);

char *psfp_util_filter_oper_warnings_to_str(char *buf, size_t size, vtss_appl_psfp_filter_oper_warnings_t oper_warnings);

// Called by TSN when PTP is ready and stream gate configurations can be
// applied.
void psfp_ptp_ready(void);

const char *psfp_error_txt(mesa_rc rc);
mesa_rc psfp_init(vtss_init_data_t *data);
#endif

