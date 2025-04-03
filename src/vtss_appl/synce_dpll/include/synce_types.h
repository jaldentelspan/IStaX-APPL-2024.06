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

#if !defined(_SYNCE_TYPES_H)
#define _SYNCE_TYPES_H

typedef u32 clock_event_type_t;

typedef enum {
    CLOCK_EEC_OPTION_1,    /* EEC option 1: See ITU-T G.8262/Y.1362 */
    CLOCK_EEC_OPTION_2     /* EEC option 2  */
} clock_eec_option_t;

typedef enum {
    PTP_CLOCK_SOURCE_SYNCE,     /* PTP clock rate is synchronized to the Synce DPLL */
    PTP_CLOCK_SOURCE_INDEP      /* PTP base clock is independent of the Synce clock, and the PTP clock can be adjusted relative to that */
} ptp_clock_source_t;

typedef enum {
    VTSS_ZL_30380_DPLL_GENERIC,
    VTSS_ZL_30380_DPLL_ZLS3034X,
    VTSS_ZL_30380_DPLL_ZLS3036X,
    VTSS_ZL_30380_DPLL_ZLS3073X,
    VTSS_ZL_30380_DPLL_ZLS3077X
} vtss_zl_30380_dpll_type_t;

#endif // _SYNCE_TYPES_H
