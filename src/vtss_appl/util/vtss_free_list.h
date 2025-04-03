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
#ifndef __VTSS_FREE_LIST_H__
#define __VTSS_FREE_LIST_H__

/* For debugging only */
#define DBG_PRINTF printf

//#define FREE_LIST_DEBUG_ENABLED

#if defined(FREE_LIST_DEBUG_ENABLED)
#define _FLIST_DEBUG(args)  { DBG_PRINTF args; }
#else
#define _FLIST_DEBUG(args)
#endif /* defined(FREE_LIST_DEBUG_ENABLED) */

#define _FLIST_T_E(...)     _FLIST_DEBUG(("Error: %s, %s, %d, ", __FILE__, __FUNCTION__, __LINE__)); \
                            _FLIST_DEBUG((__VA_ARGS__));

#endif /* __VTSS_FREE_LIST_H__ */

