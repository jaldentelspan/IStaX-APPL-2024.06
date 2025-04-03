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

#ifndef _VTSS_OPTIONAL_MODULES_HXX_
#define _VTSS_OPTIONAL_MODULES_HXX_

#include "main_types.h"

struct VtssOptionalModule;

typedef mesa_rc (*module_init_t)(vtss_init_data_t *data);
typedef const char *(*module_error_txt_t)(mesa_rc rc);

struct VtssOptionalModule {
    VtssOptionalModule *module_next;

    const char *name;
    uint32_t module_id;
    int32_t init_priority;  // lower number means higher priority. zero is
                            // neutral

    module_init_t init;

    module_error_txt_t err_to_txt;

    void *module_api;
};

VtssOptionalModule *vtss_module_create();
bool vtss_has_module(const char *path);

#endif
