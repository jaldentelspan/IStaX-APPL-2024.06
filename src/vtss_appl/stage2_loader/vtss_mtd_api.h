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

#ifndef __VTSS_MTD_API_H__
#define __VTSS_MTD_API_H__

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "vtss_compat.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}  /* Fixing extra indent */
#endif

#include "stdlib.h"
#include <mtd/mtd-user.h>
typedef struct {
    int fd;  /* Fileno of /dev/<name> */
    int devno;
    char dev[64];  /* mtd device name */
    struct mtd_info_user info;
} vtss_mtd_t;

#define FLAGS (O_RDWR | O_SYNC)

/****************************************************************************
 * Public functions
 ****************************************************************************/
/*!
 * \brief Open a mtd device.
 *
 * \param *mtd [IN] pointer to a mtd device.
 *
 * \param *name [IN] name of the mtd device.
 *
 * \return VTSS_OK if the operation succeeded.
 *         VTSS_RC_ERROR if the operation failed.
 */
vtss_rc vtss_mtd_open(vtss_mtd_t *mtd, const char* name);

/*!
 * \brief Erase a mtd device.
 *
 * \param *mtd [IN] pointer to a mtd device.
 *
 * \param length [IN] size of the mtd device.
 *
 * \return VTSS_OK if the operation succeeded.
 *         VTSS_RC_ERROR if the operation failed.
 */
vtss_rc vtss_mtd_erase(const vtss_mtd_t *mtd, size_t length);

/*!
 * \brief Program a mtd device.
 *
 * \param *mtd [IN] pointer to a mtd device.
 *
 * \param *buffer [IN] program mtd device from the buffer pointed by *buffer.
 *
 * \param length [IN] program length bytes.
 *
 * \return VTSS_OK if the operation succeeded.
 *         VTSS_RC_ERROR if the operation failed.
 */
vtss_rc vtss_mtd_program(const vtss_mtd_t *mtd, const u8 *buffer, size_t length);

/*!
 * \brief Close a mtd device.
 *
 * \param *mtd [IN] pointer to a mtd device.
 *
 * \return VTSS_OK if the operation succeeded.
 *         VTSS_RC_ERROR if the operation failed.
 */
vtss_rc vtss_mtd_close(vtss_mtd_t *mtd);


#if 0
{  /* Fixing extra indent */
#endif

#ifdef __cplusplus
}
#endif
#endif  /* __VTSS_MTD_API_H__ */
