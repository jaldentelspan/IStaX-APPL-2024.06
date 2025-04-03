/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "psec_serializer.hxx"
#include <vtss/appl/psec.h>
#include "misc_api.h"

vtss_enum_descriptor_t psec_expose_violation_mode_txt[] {
    {VTSS_APPL_PSEC_VIOLATION_MODE_PROTECT,  "protect"},
    {VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT, "restrict"},
    {VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN, "shutdown"},
    {0, 0},
};

vtss_enum_descriptor_t psec_expose_mac_type_txt[] {
    {VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC, "dynamic"},
    {VTSS_APPL_PSEC_MAC_TYPE_STATIC,  "static"},
    {VTSS_APPL_PSEC_MAC_TYPE_STICKY,  "sticky"},
    {0, 0},
};

mesa_rc psec_expose_interface_status_get(vtss_ifindex_t ifindex, psec_semi_public_interface_status_t *const status)
{
    return psec_semi_public_interface_status.get(ifindex, status);
}

mesa_rc psec_expose_global_control_mac_clear_get(vtss_appl_psec_global_control_mac_clear_t *info)
{
    if (info) {
        memset(info, 0, sizeof(*info));
    }

    return VTSS_RC_OK;
}

