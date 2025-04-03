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

#ifndef _VTSS_TRACE_LVL_API_H_
#define _VTSS_TRACE_LVL_API_H_

/* Named trace levels */
/* Note: If adding more trace levels, support must be added in cli.c */
#define VTSS_TRACE_LVL_NONE    10 /* No trace                                         */
#define VTSS_TRACE_LVL_ERROR    9 /* Code error encountered                           */
#define VTSS_TRACE_LVL_WARNING  8 /* Potential code error, manual inspection required */
#define VTSS_TRACE_LVL_INFO     6 /* Useful information                               */
#define VTSS_TRACE_LVL_DEBUG    4 /* Some debug information                           */
#define VTSS_TRACE_LVL_NOISE    2 /* Lot's of debug information                       */
#define VTSS_TRACE_LVL_RACKET   1 /* Even more ...                                    */

#define VTSS_TRACE_LVL_ALL      0 /* Enable all trace                                 */

/* Max string length of trace level */
#define VTSS_TRACE_MAX_LVL_LEN 7

#endif /* _VTSS_TRACE_LVL_API_H_ */

