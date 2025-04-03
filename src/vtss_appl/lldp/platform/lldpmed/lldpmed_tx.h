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

#ifndef LLDPMED_TX_H
#define LLDPMED_TX_H

VTSS_BEGIN_HDR
lldp_u16_t lldpmed_tlv_add(lldp_u8_t *buf, lldp_port_t port_idx);
lldp_u16_t lldpmed_get_capabilities_word(lldp_port_t port_idx);
lldp_u8_t lldpmed_coordinate_location_tlv_add(lldp_u8_t *buf);
lldp_u8_t lldpmed_ecs_location_tlv_add(lldp_u8_t *buf);
lldp_u16_t lldpmed_civic_location_tlv_add(lldp_u8_t *buf);
void lldpmed_cal_fraction (lldp_64_t tude_val, lldp_bool_t negative_number, lldp_u32_t fraction_bits_cnt, lldp_64_t *tude_val_out, u8 digits, lldp_u64_t bit_mask);
VTSS_END_HDR

#endif



