/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss_ptp_os.h"
#include "vtss_ptp_api.h"
#include "vtss_ptp_filters.hxx"

#define TEST_TRACE_I T_IG
#define TEST_TRACE_D T_DG
namespace vtss_ptp_filters {

vtss_lowpass_filter_s::vtss_lowpass_filter_s() : my_period(1)
{
    strncpy(my_name, "anonym", sizeof(my_name) - 1);
    TEST_TRACE_I(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filter %s ctor", my_name);
    reset();
}
vtss_lowpass_filter_s::vtss_lowpass_filter_s(const char * name, u32 period) : my_period(period)
{
    strncpy(my_name, name, sizeof(my_name) - 1);
    my_name[sizeof(my_name) - 1] = '\0';
    TEST_TRACE_I(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filter %s ctor", my_name);
    reset();
}


void vtss_lowpass_filter_s::reset(void)
{
    y = 0;
    s_exp = 0;
    TEST_TRACE_I(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filter %s reset", my_name);
}

bool vtss_lowpass_filter_s::filter(i64 *value)
{
    /* crank down filter cutoff by increasing 's_exp' */
    if (s_exp < 1)
        s_exp = 1;
    else if (s_exp < my_period)
        ++s_exp;
    else if (s_exp > my_period)
        s_exp = my_period;

    TEST_TRACE_D(VTSS_TRACE_GRP_PTP_BASE_FILTER, "value before filtering " VPRI64d "", (*value));
    TEST_TRACE_D(VTSS_TRACE_GRP_PTP_BASE_FILTER, "LP filt: s_exp " VPRI64d " , y " VPRI64d "", s_exp, y);

    /* filter 'low pass' */
    y = ((s_exp-1)*y + *value)/s_exp;
    TEST_TRACE_D(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filtered value " VPRI64d ", new value " VPRI64d ", divisor " VPRI64d, y, *value, s_exp);
    *value = y;
    return (s_exp == my_period);
}
} //namespace vtss_ptp_filters
