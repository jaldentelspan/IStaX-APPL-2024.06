/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_PTP_FILTERS_H_
#define _VTSS_PTP_FILTERS_H_

/**
 * \file vtss_ptp_offset_filter.h
 * \brief Define Offset filter callouts.
 *
 * This file contain the definitions of PTP filter functions and
 * associated types.
 *
 */

namespace vtss_ptp_filters {
/**
 * \brief Lowpass filter class
 */
typedef struct vtss_lowpass_filter_s {
private:
    i64 y;
    i64 s_exp;
    const u32 my_period;
    char my_name [40];
public:
    vtss_lowpass_filter_s();
    vtss_lowpass_filter_s(const char * name, u32 period);
    void reset();
    bool filter(i64 *value);
} vtss_lowpass_filter_t;

} //namespace

#endif // _VTSS_PTP_FILTERS_H_

