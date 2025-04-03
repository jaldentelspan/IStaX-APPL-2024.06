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
#ifndef _INCLUDE_VTSS_BASICS_CONFIG_H
#define _INCLUDE_VTSS_BASICS_CONFIG_H

// Use vtss unified API header files to get API types
#define VTSS_USE_API_HEADERS

// #define VTSS_SIZEOF_VOID_P 8

/* #if ((ULONG_MAX) == (UINT_MAX)) */
/* #define VTSS_SIZEOF_VOID_P 4 */
/* #else */
/* #define VTSS_SIZEOF_VOID_P 8 */
/* #endif */

#if __GNUC__
#if __x86_64__ || __ppc64__ || __aarch64__
#define VTSS_SIZEOF_VOID_P 8
#else
#define VTSS_SIZEOF_VOID_P 4
#endif
#endif


#endif  // _INCLUDE_VTSS_BASICS_CONFIG_H
