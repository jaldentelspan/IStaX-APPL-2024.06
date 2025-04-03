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

#ifndef _VTSS_OS_FILE_API_H_
#define _VTSS_OS_FILE_API_H_

#define OS_FILE_FILES_MAX             (4+32)

#if defined(CYGPKG_FS_RAM) && defined(VTSS_SW_OPTION_ICFG)

#include <time.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Synchronize flash to FS.
 */
void os_file_flash2fs(void);

/**
 * Synchronize FS to flash.
 * Return: VTSS_RC_OK:         Success
 *         VTSS_RC_ERROR:      Could not sync to flash
 */
mesa_rc os_file_fs2flash(void);

/**
 * Initialization
 */
mesa_rc os_file_init(vtss_init_data_t *data);
#ifdef __cplusplus
}
#endif

#endif  /* CYGPKG_FS_RAM && VTSS_SW_OPTION_ICFG */

#endif /* _VTSS_OS_FILE_API_H_ */

