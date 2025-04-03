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

#ifndef _DDMI_API_H_
#define _DDMI_API_H_

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */
#include "vtss/appl/ddmi.h"

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    DDMI_ERROR_INVALID_ARGUMENT = MODULE_ERROR_START(VTSS_MODULE_ID_DDMI), /**< Invalid argument supplied to function                      */
    DDMI_ERROR_IFINDEX_NOT_PORT,                                           /**< The supplied interface index does not represent a port     */
    DDMI_ERROR_INVALID_PORT,                                               /**< The supplied interface index's port number is out of range */
};

const char *ddmi_monitor_type_to_txt(vtss_appl_ddmi_monitor_type_t type);
const char *ddmi_monitor_state_to_txt(vtss_appl_ddmi_monitor_state_t state);
const char *ddmi_error_txt(mesa_rc rc);
mesa_rc ddmi_init(vtss_init_data_t *data);

#endif /* _DDMI_API_H_ */

