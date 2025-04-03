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

#ifndef _VTSS_PTP_OS_H_
#define _VTSS_PTP_OS_H_

#include "stddef.h"
#include "main_types.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_PTP

/* bit array manipulation */
#define getFlag(x,y)  (((x) & (y)) != 0)
#define setFlag(x,y)    ((x) |= (y))
#define clearFlag(x,y)  ((x) &= ~(y))

u16 vtss_ptp_get_rand(u32 *seed);
void *vtss_ptp_calloc(size_t nmemb, size_t sz);
void  vtss_ptp_free(void *ptr);
#define VTSS_PTP_CREATE(TYPE) vtss_create<TYPE>(VTSS_MODULE_ID_PTP, __FILE__, __LINE__)

#endif /* _VTSS_PTP_OS_H_ */
