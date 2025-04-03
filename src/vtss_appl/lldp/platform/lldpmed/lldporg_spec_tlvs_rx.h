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


/******************************************************************************
* Types / functions defintions for LLDP-MED receive part
******************************************************************************/

#ifndef LLDPORG_RX_H
#define LLDPORG_RX_H
#include "lldp_remote.h"

VTSS_BEGIN_HDR
lldp_bool_t lldporg_update_neccessary(lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry);
void lldporg_update_entry(lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry) ;
lldp_bool_t lldporg_validate_lldpdu(lldp_u8_t *tlv, lldp_rx_remote_entry_t   *rx_entry, lldp_u16_t len);
void lldporg_autoneg_support2str (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr);
void lldporg_autoneg_status2str (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr);
void lldporg_operational_mau_type2str (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr);
void lldporg_autoneg_capa2str (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr);
VTSS_END_HDR

#endif // LLDPORG_RX_H
