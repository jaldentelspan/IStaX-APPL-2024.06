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

#include "synce_dpll_base.h"
#include "synce_dpll_trace.h"

#define CRIT_ENTER() critd_enter(&vtss::synce::dpll::crit, __FILE__, __LINE__)
#define CRIT_EXIT()  critd_exit( &vtss::synce::dpll::crit, __FILE__, __LINE__)

namespace vtss
{
namespace synce
{
namespace dpll
{
critd_t crit;
bool pcb104 = false;
bool si5328 = false;
bool pcb104_synce = false;
}
}
}

mesa_rc synce_dpll_base::clock_selection_mode_get(vtss_appl_synce_selection_mode_t *const mode)
{
    *mode = clock_selection_mode;
    T_D("Get selection mode (%d)", *mode);

    return VTSS_RC_OK;
}

mesa_rc synce_dpll_base::clock_ref_clk_in_freq_set(const uint source, const u32 freq_khz)
{
    return VTSS_RC_ERROR;
}

mesa_rc synce_dpll_base::clock_ho_frequency_offset_get(i64 *const offset)
{
    return VTSS_RC_ERROR;
}
