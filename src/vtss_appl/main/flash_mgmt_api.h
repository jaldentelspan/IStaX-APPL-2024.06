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

#ifndef _FLASH_MGMT_API_H_
#define _FLASH_MGMT_API_H_

#include <main_types.h>
#include "vtss_os_wrapper.h"

/****************************************************************************/
/****************************************************************************/
typedef struct {
    vtss_flashaddr_t base_fladdr; /* For flash read/write access */
    size_t          size_bytes;
} flash_mgmt_section_info_t;

/****************************************************************************/
// flash_mgmt_fis_lookup()
// Low-level lookup.
// Lookup a named FIS index named @section_name. Result will be stored in pEntry.
// Returns -1 on error, the entry number on success.
/****************************************************************************/
int flash_mgmt_fis_lookup(const char *section_name, struct fis_table_entry *pEntry);

/****************************************************************************/
// flash_mgmt_lookup()
// High-level lookup.
// Given a FIS section name, lookup the base address and size in flash.
// If it doesn't exist in flash, fall back to statically defined addresses
// if the flash's size permits it, and return TRUE. Otherwise return FALSE.
/****************************************************************************/
BOOL flash_mgmt_lookup(const char *section_name, flash_mgmt_section_info_t *info);

#endif /* _FLASH_MGMT_API_H_ */

