/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _ZL_30361_API_API_H_
#define _ZL_30361_API_API_H_

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

mesa_rc zl_3036x_register_get(const u32   page,
                              const u32   reg,
                              u32 *const  value);

mesa_rc zl_3036x_register_set(const u32   page,
                              const u32   reg,
                              const u32   value);

mesa_rc zl_3036x_debug_pll_status(void);
mesa_rc zl_3036x_debug_hw_ref_status(u32 ref_id);
mesa_rc zl_3036x_debug_hw_ref_cfg(u32 ref_id);
mesa_rc zl_3036x_debug_dpll_status(u32 pll_id);
mesa_rc zl_3036x_debug_dpll_cfg(u32 pll_id);
mesa_rc zl_3036x_debug_nco_freq_read(i32 *freqOffsetUppm);

mesa_rc zl_3036x_trace_init(FILE *logFd);
mesa_rc zl_3036x_trace_level_module_set(u32 module, u32 level);
mesa_rc zl_3036x_trace_level_all_set(u32 level);

#ifdef __cplusplus
}
#endif

#endif // _ZL_30361_API_API_H_

