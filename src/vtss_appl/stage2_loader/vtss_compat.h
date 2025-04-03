/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_COMPAT_HXX_
#define _VTSS_COMPAT_HXX_

#include "main.h"
#include "mfi.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compat
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned char vtss_rc;

#define VTSS_OK                          0
#define VTSS_RC_ERROR                    1
#define FIRMWARE_ERROR_FLASH_ERASE       101
#define FIRMWARE_ERROR_FLASH_PROGRAM     102
#define FIRMWARE_ERROR_CURRENT_NOT_FOUND 103

#define T_E(s, ...) warn(s "\n", ##__VA_ARGS__)
#define T_W(s, ...) warn(s "\n", ##__VA_ARGS__)
#define T_I(s, ...) info(s "\n", ##__VA_ARGS__)
#define T_D(s, ...) debug(s "\n", ##__VA_ARGS__)

#define VTSS_MALLOC  malloc
#define VTSS_REALLOC realloc
#define VTSS_FREE    free

#define MD5_MAC_LEN 16

#ifdef __cplusplus
}
#endif

#endif    // _VTSS_COMPAT_HXX_
