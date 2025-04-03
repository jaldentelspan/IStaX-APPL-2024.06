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


#ifndef _UDLD_API_H_
#define _UDLD_API_H_

#include "main.h"     /* MODULE_ERROR_START */
#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */
#include "vtss/appl/udld.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VTSS_APPL_UDLD_RC_OK = 0,
    VTSS_APPL_UDLD_RC_INVALID_PARAMETER
} vtss_appl_udld_error_code_t;

typedef enum {
    VTSS_UDLD_RC_OK = VTSS_RC_OK,
    VTSS_UDLD_RC_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_UDLD),
    VTSS_UDLD_RC_INVALID_PARAMETER,
    VTSS_UDLD_RC_NOT_ENABLED,
    VTSS_UDLD_RC_ALREADY_CONFIGURED,
    VTSS_UDLD_RC_NOT_SUPPORTED,
    VTSS_UDLD_RC_INVALID_STATE,
    VTSS_UDLD_RC_INVALID_FLAGS,
    VTSS_UDLD_RC_INVALID_CODES,
    VTSS_UDLD_RC_NOT_OK,
} vtss_udld_error_code_t;

mesa_rc udld_init(vtss_init_data_t *data);
void    udld_something_has_changed(void);
#ifdef __cplusplus
}
#endif
#endif /* _UDLD_API_H_ */
