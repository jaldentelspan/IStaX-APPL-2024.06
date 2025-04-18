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

#ifndef _VTSS_LOOP_PROTECT_API_H_
#define _VTSS_LOOP_PROTECT_API_H_

#include "main.h"     /* MODULE_ERROR_START */
#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */

#include "vtss/appl/loop_protect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LOOP_PROTECT error codes (mesa_rc) */
enum {
    LOOP_PROTECT_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_LOOP_PROTECT),  /* Generic error code */
    LOOP_PROTECT_ERROR_PARM,    /* Illegal parameter */
    LOOP_PROTECT_ERROR_INACTIVE, /* Port is inactive */
    LOOP_PROTECT_ERROR_TIMEOUT, /* Timeout */
    LOOP_PROTECT_ERROR_MSGALLOC, /* Malloc error */
};

/* LOOP_PROTECT error text */
const char *loop_protect_error_txt(mesa_rc rc);

const char *loop_protect_action2string(vtss_appl_loop_protect_action_t action);

/* Initialize module */
mesa_rc loop_protect_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_LOOP_PROTECT_API_H_ */

