/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "mac_serializer.hxx"
#include "vtss/appl/mac.h"
#include "vtss/appl/vlan.h" /* For VTSS_APPL_VLAN_ID_DEFAULT */
#include "mac_serializer.hxx"

vtss_enum_descriptor_t mac_learning_txt[] {
    {VTSS_APPL_MAC_LEARNING_AUTO,    "auto"},
    {VTSS_APPL_MAC_LEARNING_DISABLE, "disable"},
    {VTSS_APPL_MAC_LEARNING_SECURE,  "secure"},
    {0, 0},
};

mesa_rc vtss_appl_mac_table_itr_vid(const mesa_vid_t *prev_vlan, mesa_vid_t *next_vlan) {
    if (prev_vlan == NULL) {
        // Get first port
        *next_vlan = VTSS_APPL_VLAN_ID_DEFAULT;
        return VTSS_RC_OK;
    } else if (*prev_vlan < (VTSS_VIDS-1)) {
      *next_vlan = *prev_vlan + 1;
      return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_mac_appl_table_flush_dummy(vtss_appl_mac_flush_t *const flush)
{
  flush->flush_all = 0;
  return VTSS_RC_OK;
}

