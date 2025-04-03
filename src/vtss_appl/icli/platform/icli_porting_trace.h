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

#ifndef __ICLI_PORTING_TRACE_H__
#define __ICLI_PORTING_TRACE_H__

#ifdef ICLI_TARGET

#include "vtss_trace_api.h"
#include "vtss_module_id.h"
#include "vtss_trace_lvl_api.h"

#ifdef VTSS_TRACE_MODULE_ID
#undef VTSS_TRACE_MODULE_ID
#endif

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ICLI
#include <vtss_trace_api.h>

#else // ICLI_TARGET

#ifndef T_E
#define T_E(...) \
    printf("\nError: %s::%s()#%d: ", __FILE__, __FUNCTION__, __LINE__); \
    printf(__VA_ARGS__);
#endif

#ifndef T_W
#define T_W(...) \
    printf("Warning: %s::%s()#%d: ", __FILE__, __FUNCTION__, __LINE__); \
    printf(__VA_ARGS__);
#endif

#endif // ICLI_TARGET

#endif  /* __ICLI_PORTING_TRACE_H__ */

