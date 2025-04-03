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

#if !defined(_SYNCE_DPLL_ZL303XX_H)
#define _SYNCE_DPLL_ZL303XX_H

#include "synce_dpll_base.h"

struct synce_dpll_zl303xx : synce_dpll_base {
    virtual mesa_rc clock_eec_option_type_get(uint *const eec_type);
    virtual mesa_rc clock_frequency_set(const uint clock_input, const meba_synce_clock_frequency_t frequency);
    virtual mesa_rc clock_station_clock_type_get(uint *const clock_type);
    virtual mesa_rc clock_adj_phase_set(i32 adj);
    virtual mesa_rc clock_read(const uint reg, uint *const value);
    virtual mesa_rc clock_write(const uint reg, const uint value);
    virtual mesa_rc clock_output_adjtimer_set(i64 adj);
    virtual mesa_rc clock_ptp_timer_source_set(ptp_clock_source_t source);
};

#endif // _SYNCE_DPLL_ZL303XX_H
