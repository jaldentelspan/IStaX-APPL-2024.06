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

#include "synce_dpll_zl303xx.h"

#include "synce_dpll_trace.h"

#define CRIT_ENTER() critd_enter(&vtss::synce::dpll::crit, __FILE__, __LINE__)
#define CRIT_EXIT()  critd_exit( &vtss::synce::dpll::crit, __FILE__, __LINE__)

mesa_rc synce_dpll_zl303xx::clock_eec_option_type_get(uint *const eec_type)
{
    *eec_type = 0;
    T_D("Get EEC type %d", *eec_type);
    return MESA_RC_OK;
}

mesa_rc synce_dpll_zl303xx::clock_frequency_set(const uint clock_input, const meba_synce_clock_frequency_t frequency)
{
    if (clock_input >= clock_my_input_max) {
        return MESA_RC_ERROR;
    }
    T_D("Set clock %d frequency: %d", clock_input, frequency);
    return MESA_RC_OK;
}

mesa_rc synce_dpll_zl303xx::clock_station_clock_type_get(uint *const clock_type)
{
    *clock_type = 0;
    T_D("Get station clock type %d", *clock_type);
    return MESA_RC_OK;
}

mesa_rc synce_dpll_zl303xx::clock_adj_phase_set(i32 adj)
{
    return MESA_RC_ERROR;
}

mesa_rc synce_dpll_zl303xx::clock_output_adjtimer_set(i64 adj)
{
    return MESA_RC_ERROR;
}

mesa_rc synce_dpll_zl303xx::clock_ptp_timer_source_set(ptp_clock_source_t source)
{
    return MESA_RC_ERROR;
}

mesa_rc synce_dpll_zl303xx::clock_read(const uint reg, uint *const value)
{
    u8 val;

    CRIT_ENTER();
    vtss::synce::dpll::clock_chip_spi_if.read(reg, &val);
    *value = val;
    T_I("spi clock read val %d", *value);
    CRIT_EXIT();

    return MESA_RC_OK;
}

mesa_rc synce_dpll_zl303xx::clock_write(const uint reg, const uint value)
{
    CRIT_ENTER();
    vtss::synce::dpll::clock_chip_spi_if.write(reg, value);
    CRIT_EXIT();

    return MESA_RC_OK;

}

