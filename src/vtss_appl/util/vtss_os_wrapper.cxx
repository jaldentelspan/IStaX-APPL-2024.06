/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "vtss_os_wrapper.h"

const char *vtss_thread_prio_to_txt(vtss_thread_prio_t prio)
{
    switch (prio) {
    case VTSS_THREAD_PRIO_BELOW_NORMAL:
        return "Below";

    case VTSS_THREAD_PRIO_DEFAULT:
        return "Normal";

    case VTSS_THREAD_PRIO_ABOVE_NORMAL:
        return "Above";

    case VTSS_THREAD_PRIO_HIGH:
        return "High";

    case VTSS_THREAD_PRIO_HIGHER:
        return "Higher";

    case VTSS_THREAD_PRIO_HIGHEST:
        return "Highest";

    case VTSS_THREAD_PRIO_BELOW_NORMAL_RT:
        return "Below (RT)";

    case VTSS_THREAD_PRIO_DEFAULT_RT:
        return "Normal (RT)";

    case VTSS_THREAD_PRIO_ABOVE_NORMAL_RT:
        return "Above (RT)";

    case VTSS_THREAD_PRIO_HIGH_RT:
        return "High (RT)";

    case VTSS_THREAD_PRIO_HIGHER_RT:
        return "Higher (RT)";

    case VTSS_THREAD_PRIO_HIGHEST_RT:
        return "Highest (RT)";

    default:
        return "N/A";
    }
}

