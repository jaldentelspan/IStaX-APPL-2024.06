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

#ifndef _DOT1X_H_
#define _DOT1X_H_

#include "main_types.h"   /* For BOOL               */
#include "vtss_nas_api.h" /* For nas_port_control_t */

// Semi-public functions and macros

/******************************************************************************/
// Port number conversion macros.
// nas_port_t counts from 1.
/******************************************************************************/
#ifndef NAS_PORT_NO_START
#define NAS_PORT_NO_START 1
#endif
// So in the 802.1X core-library, ports are numbered as follows (ex with 16 isids and 26 ports with two as stack ports):
// -------------------------------------------------------------
// 802.1X Core Lib Port Number | ISID | Switch API Port Number |
// -------------------------------------------------------------
//      1                      |  1   |  0                     |
//      2                      |  1   |  1                     |
//    ...                      | ...  | ...                    |
//     26                      |  1   | 25                     |
//     27                      |  2   |  0                     |
//    ...                      | ...  | ...                    |
//    416                      | 16   | 25                     |
// -------------------------------------------------------------
// To confuse it even more, the l2proto module uses other port numbers, since
// it somehow includes GLAGs, but the l2proto module operates on isid and
// switch API port numbers and contains its own conversion functions
// called L2PORT2PORT(isid, api_port) to convert to an l2proto port.
#define DOT1X_NAS_PORT_2_ISID(nas_port)                       (VTSS_ISID_START)
#define DOT1X_NAS_PORT_2_SWITCH_API_PORT(nas_port)            ((nas_port) - NAS_PORT_NO_START)
#define DOT1X_ISID_SWITCH_API_PORT_2_NAS_PORT(isid, api_port) ((api_port) + NAS_PORT_NO_START)

#ifdef __cplusplus
extern "C" {
#endif

void dot1x_crit_enter(void);
void dot1x_crit_exit(void);
void dot1x_crit_assert_locked(void);
void dot1x_disable_due_to_soon_boot(void);
BOOL dot1x_glbl_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* _DOT1X_H_ */

