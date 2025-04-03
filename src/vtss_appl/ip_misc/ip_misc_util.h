/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _IP2_MISC_UTIL_H_
#define _IP2_MISC_UTIL_H_

#include "main.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Check that the given IP address in textual notation is used on an existing IP interface.
 */
bool vtss_ip_misc_is_src_address_used(const char *src_address, mesa_ip_type_t ip_version);

/*
 * Check that the given VID is used on an existing IP interface.
 */
bool vtss_ip_misc_is_vid_ip_interface(mesa_vid_t vid);

#ifdef __cplusplus
}
#endif

#endif /* _IP2_MISC_UTIL_H_ */

