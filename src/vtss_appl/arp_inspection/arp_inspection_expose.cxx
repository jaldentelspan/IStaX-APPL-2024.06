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

#include "arp_inspection_serializer.hxx"
#include "vtss/basics/preprocessor.h"

vtss_enum_descriptor_t arp_inspection_logType_txt[] {
    {VTSS_APPL_ARP_INSPECTION_LOG_NONE,      "none"},
    {VTSS_APPL_ARP_INSPECTION_LOG_DENY,      "deny"},
    {VTSS_APPL_ARP_INSPECTION_LOG_PERMIT,    "permit"},
    {VTSS_APPL_ARP_INSPECTION_LOG_ALL,       "all"},
    {0, 0}
};

vtss_enum_descriptor_t arp_inspection_regStatus_txt[] {
    {VTSS_APPL_ARP_INSPECTION_STATIC_TYPE,   "static"},
    {VTSS_APPL_ARP_INSPECTION_DYNAMIC_TYPE,  "dynamic"},
    {0, 0}
};

mesa_rc vtss_appl_arp_inspection_control_dummy_get(BOOL *const act_flag) {
    if (act_flag) {
        *act_flag = FALSE;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get ARP Inspection event status.
 *
 * \param status     [OUT] ARP inspection event status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_status_event_get(
    vtss_appl_arp_inspection_status_event_t *const status
)
{
    return arp_inspection_status_event_update.get(status);
}
