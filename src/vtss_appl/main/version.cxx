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

#include "version.h"
#include "misc_api.h"

#define xstr(s) str(s)
#define str(s) #s

static const char compile_time[] = MAGIC_ID_DATE xstr(COMPILE_TIME);

#ifndef VTSS_PRODUCT_NAME_LONG
#define VTSS_PRODUCT_NAME_LONG "Microchip " VTSS_PRODUCT_NAME " Switch"
#endif

#ifndef VTSS_PRODUCT_STACK_TYPE
#define VTSS_PRODUCT_STACK_TYPE " (" VTSS_STACK_TYPE ") "
#endif

// Vendor specific product name
static const char product_name[] = MAGIC_ID_PROD VTSS_PRODUCT_NAME_LONG;

// Build identification
#ifdef BUILD_NUMBER
  static const char version_string[VTSS_MAX_VERSION_STRING_SIZE] = MAGIC_ID_VERS VTSS_PRODUCT_NAME VTSS_PRODUCT_STACK_TYPE xstr(SW_RELEASE) " Build " xstr(BUILD_NUMBER);
#else
  static const char version_string[VTSS_MAX_VERSION_STRING_SIZE] = MAGIC_ID_VERS VTSS_PRODUCT_NAME xstr(SW_RELEASE);
#endif

// Version control
#ifdef CODE_REVISION
  static const char code_revision[] = MAGIC_ID_REV xstr(CODE_REVISION);
#else
  static const char code_revision[] = MAGIC_ID_REV;
#endif

/* Software version text string */
const char *misc_software_version_txt(void)
{
    return version_string + MAGIC_ID_LEN;
}

/* Software codebase revision string */
const char *misc_software_code_revision_txt(void)
{
    return code_revision + MAGIC_ID_LEN;
}

/* Software date text string */
const char *misc_software_date_txt(void)
{
    return compile_time + MAGIC_ID_LEN;
}

const char *misc_product_name(void)
{
    return product_name + MAGIC_ID_LEN;
}

