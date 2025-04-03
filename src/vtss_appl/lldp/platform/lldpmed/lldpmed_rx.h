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

#ifndef LLDPMED_RX_H
#define LLDPMED_RX_H
#include "lldp_remote.h"
#include "vtss/appl/lldp.h"

VTSS_BEGIN_HDR
void        lldpmed_update_entry(lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry);
lldp_bool_t lldpmed_validate_lldpdu(lldp_u8_t *tlv, lldp_rx_remote_entry_t   *rx_entry, lldp_u16_t len, lldp_bool_t first_lldpmed_tlv);
lldp_bool_t lldpmed_update_neccessary(lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry);
void        lldpmed_device_type2str (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr );
void        lldpmed_capabilities2str (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr );
char        *lldpmed_policy_dscp2str(vtss_appl_lldp_med_policy_t &policy, char *string_ptr);
char        *lldpmed_policy_prio2str(vtss_appl_lldp_med_policy_t &policy, char *string_ptr);
char        *lldpmed_policy_vlan_id2str(vtss_appl_lldp_med_policy_t &policy, char *string_ptr);
const char  *lldpmed_policy_tag2str(BOOL tagged);
const char  *lldpmed_policy_flag_type2str(BOOL unknown_flag);
char        *lldpmed_appl_type2str (vtss_appl_lldp_med_application_type_t appl_type_value, char *string_ptr);
lldp_u8_t   lldpmed_get_policies_cnt (vtss_appl_lldp_remote_entry_t   *entry);
void        lldpmed_cal_la (void);
void        lldpmed_location2str (vtss_appl_lldp_remote_entry_t   *entry, lldp_8_t *string_ptr, lldpmed_location_type_t type);
void        lldpmed_medTransmitEnabled_set(lldp_port_t p_index, lldp_bool_t tx_enable);
lldp_bool_t lldpmed_medTransmitEnabled_get(lldp_port_t p_index);
lldp_u8_t   lldpmed_medFastStart_timer_get(lldp_port_t p_index);
void        lldpmed_medFastStart_timer_action(lldp_port_t p_index, lldp_u8_t tx_enable, lldpmed_fast_start_repeat_count_t action);
BOOL        lldpmed_get_tagged_flag(lldp_u32_t lldpmed_policy);

lldp_bool_t lldpmed_tude2decimal_str (lldp_u8_t res,  lldp_u64_t tude, lldp_u8_t int_bits, lldp_u8_t frac_bits, lldp_8_t *string_ptr, u8 digi_cnt, BOOL ignore_neg_number);
void        lldpmed_at2str(vtss_appl_lldp_med_at_type_t, lldp_8_t *string_ptr);
void        lldpmed_datum2str(vtss_appl_lldp_med_datum_t datum, lldp_8_t *string_ptr);


mesa_rc civic_tlv2civic(const vtss_appl_lldp_med_civic_tlv_format_t *civic_location, vtss_appl_lldpmed_civic_t *civic);
mesa_rc civic2civic_tlv(vtss_appl_lldpmed_civic_t *civic, vtss_appl_lldp_med_civic_tlv_format_t *civic_tlv);

char        *civic_ptr_get(vtss_appl_lldpmed_civic_t *civic, vtss_appl_lldp_med_catype_t ca_type);
VTSS_END_HDR

#endif //LLDPMED_RX_H
