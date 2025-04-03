/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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


#ifndef LLDP_REMOTE_H
#define LLDP_REMOTE_H

#include "main.h"
#include "lldp_os.h"
#include "vtss_lldp.h"
#include "lldp_tlv.h"
#include "vtss/appl/lldp.h" // For e.g. MAX_CHASSIS_ID_LENGTH

#ifdef VTSS_SW_OPTION_POE
#include "poe_api.h"
#endif


typedef struct {
    lldp_u8_t subtype;
    lldp_u8_t length;
    lldp_u8_t *mgmt_address;
    lldp_u8_t if_number_subtype;
    lldp_u8_t *if_number;
    lldp_u8_t oid_length;
    lldp_u8_t *oid;
} lldp_mgmt_addr_tlv_p_t;


/* like a remote entry, but with pointers into the received frame instead
** of arrays */
typedef struct {
    lldp_port_t    receive_port;

    lldp_u8_t *smac; // Not part of the LLDP standard, but needed for Voice Vlan


    /* The following fields are "data fields" with received data */
    lldp_u8_t chassis_id_subtype;
    lldp_u8_t chassis_id_length;
    lldp_u8_t *chassis_id;

    lldp_u8_t port_id_subtype;
    lldp_u8_t port_id_length;
    lldp_u8_t *port_id;

    lldp_u8_t port_description_length;
    lldp_u8_t *port_description;

    lldp_u8_t system_name_length;
    lldp_u8_t *system_name;

    lldp_u8_t  system_description_length;
    lldp_u8_t *system_description;

    lldp_u8_t system_capabilities[4];

    lldp_mgmt_addr_tlv_p_t mgmt_addr[LLDP_MGMT_ADDR_CNT];

    lldp_u16_t     ttl;
#ifdef VTSS_SW_OPTION_POE
#define VTSS_IEEE_VERSION_UNDEFINED 255

    // organizationally TLV for PoE - See figure 33-26 in IEEE802.3at/D3
    lldp_u8_t      ieee_draft_version; // Because there are already equipment that supports earlier versions of the ieee standard,
    //we have to support multiple versions #!&&%"

    lldp_u8_t      poe_info_valid_len; // Set to length > 0 if the PoE fields below contains valid information.
    lldp_u8_t      requested_type_source_prio;
    lldp_u16_t     requested_power;
    lldp_u8_t      actual_type_source_prio;
    lldp_u16_t     actual_power;

    lldp_u8_t      poe_mdi_power_support; // Figure 79-2, IEEE801.3at/D4
    lldp_u8_t      poe_pse_power_pair;    // Figure 79-2, IEEE801.3at/D4
    lldp_u8_t      poe_power_class;       // Figure 79-2, IEEE801.3at/D4
  // Type 3 and Type 4 extensions
    lldp_u16_t     requested_power_mode_a;
    lldp_u16_t     requested_power_mode_b;
    lldp_u16_t     pse_alloc_power_alt_a;
    lldp_u16_t     pse_alloc_power_alt_b;
    lldp_u16_t     power_status;
    lldp_u8_t      system_setup;
    lldp_u16_t     pse_maximum_avail_power;
    lldp_u8_t      auto_class;
    lldp_u32_t     power_down;
  #endif

#ifdef VTSS_SW_OPTION_LLDP_ORG
    // Organizationally Specific TLVs
    lldp_bool_t lldporg_autoneg_vld; // Set to TRUE when the fields below contains valid data.
    lldp_u8_t   lldporg_autoneg; //Autonegotiation Support/Status - Figure G-1, IEEE802.1AB
    lldp_u16_t  lldporg_autoneg_advertised_capa; // Figure G-1, IEEE802.1AB
    lldp_u16_t  lldporg_operational_mau_type; // Figure G-1, IEEE802.1AB
#endif // VTSS_SW_OPTION_LLDP_ORG

#ifdef VTSS_SW_OPTION_LLDP_MED
    //
    // TIA 1057 - LLDP MED
    //
    lldp_bool_t lldpmed_info_vld; // Set to TRUE if any LLDP-MED TLV is received.

    // LLDP-MED Capabilites TLV - Figure 6, TIA1057
    lldp_bool_t lldpmed_capabilities_vld; // Set to TRUE when the fields below contains valid data.
    lldp_u16_t lldpmed_capabilities;
    lldp_u16_t lldpmed_capabilities_current; // See TIA1057, MIBS LLDPXMEDREMCAPCURRENT
    lldp_u8_t  lldpmed_device_type;

    // LLDP-MED Policy TLV - Figure 7,TIA1057
    vtss_appl_lldp_med_policy_t policy[VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT];

    // LLDP-MED location TLV - Figure 8,TIA1057
    lldp_bool_t lldpmed_coordinate_location_vld; // Valid bit signaling that the corrensponding lldpmed_location contains valid information
    vtss_appl_lldp_med_location_info_t coordinate_location;

    BOOL        lldpmed_civic_location_vld;   /**< TRUE when civic location contains valid information*/
    lldp_u16_t  lldpmed_civic_location_length;
    lldp_u8_t   *lldpmed_civic_location;

    BOOL        lldpmed_ecs_location_vld;   /**< TRUE when civic location contains valid information*/
    lldp_u16_t  lldpmed_ecs_location_length;
    lldp_u8_t   *lldpmed_ecs_location;

    // LLDP-MED hardware revision - Figure 13, TIA1057
    lldp_u8_t   lldpmed_hw_rev_length;
    lldp_u8_t   *lldpmed_hw_rev;

    // firmware revision TLV - Figure 14,  TIA1057
    lldp_u8_t   lldpmed_firm_rev_length;
    lldp_u8_t   *lldpmed_firm_rev;

    // Software revision TLV - Figure 15,  TIA1057
    lldp_u8_t   lldpmed_sw_rev_length;
    lldp_u8_t   *lldpmed_sw_rev;

    // Serial number  TLV - Figure 16,  TIA1057
    lldp_u8_t   lldpmed_serial_no_length;
    lldp_u8_t   *lldpmed_serial_no;

    // Manufacturer name TLV - Figure 17,  TIA1057
    lldp_u8_t   lldpmed_manufacturer_name_length;
    lldp_u8_t   *lldpmed_manufacturer_name;

    // Model Name TLV - Figure 18,  TIA1057
    lldp_u8_t   lldpmed_model_name_length;
    lldp_u8_t   *lldpmed_model_name;

    // Asset ID TLV - Figure 19,  TIA1057
    lldp_u8_t   lldpmed_asset_id_length;
    lldp_u8_t   *lldpmed_asset_id;
#endif

#ifdef VTSS_SW_OPTION_POE
    // POE
    lldp_u8_t   tia_info_valid_len;    // Set to length > 0 if the PoE fields below contains valid information.
    lldp_u8_t   tia_type_source_prio;  // For supporting TIA 1057
    lldp_u16_t  tia_power;             // For supporting TIA 1057
#endif

#ifdef VTSS_SW_OPTION_EEE
    vtss_appl_lldp_eee_t eee;
#endif

    vtss_appl_lldp_fp_t fp;
} lldp_rx_remote_entry_t;

typedef struct {
    // organizationally TLV for PoE - See figure 33-26 in IEEE802.3at/D3
    lldp_u8_t      ieee_draft_version; // Because there are already equipment that supports earlier versions of the ieee standard,
    //we have to support multiple versions #!&&%"

    lldp_u8_t      poe_info_valid_len; // Set to length > 0 if the PoE fields below contains valid information.
    lldp_u8_t      requested_type_source_prio;
    lldp_u16_t     requested_power;
    lldp_u8_t      actual_type_source_prio;
    lldp_u16_t     actual_power;

    lldp_u8_t      poe_mdi_power_support; // Figure 79-2, IEEE801.3at/D4
    lldp_u8_t      poe_pse_power_pair;    // Figure 79-2, IEEE801.3at/D4
    lldp_u8_t      poe_power_class;       // Figure 79-2, IEEE801.3at/D4
  // Type 3 and Type 4 extensions
    lldp_u16_t     requested_power_mode_a;
    lldp_u16_t     requested_power_mode_b;
    lldp_u16_t     pse_alloc_power_alt_a;
    lldp_u16_t     pse_alloc_power_alt_b;
    lldp_u16_t     power_status;
    lldp_u8_t      system_setup;
    lldp_u16_t     pse_maximum_avail_power;
    lldp_u8_t      auto_class;
    lldp_u32_t     power_down;

    lldp_u8_t   tia_info_valid_len;    // Set to length > 0 if the PoE fields below contains valid information.
    lldp_u8_t   tia_type_source_prio;  // For supporting TIA 1057
    lldp_u16_t  tia_power;             // For supporting TIA 1057
} lldp_poe_entry_t;


VTSS_BEGIN_HDR

void lldp_remote_delete_entries_for_local_port (lldp_port_t port);
lldp_bool_t lldp_remote_handle_msap (lldp_rx_remote_entry_t   *rx_entry);
vtss_appl_lldp_remote_entry_t   *lldp_get_remote_entry (lldp_u8_t idx);
void lldp_remote_1sec_timer (void);
void lldp_remote_tlv_to_string (vtss_appl_lldp_remote_entry_t   *entry, const lldp_tlv_t field, lldp_8_t *output_string, const u32 output_string_len, const lldp_u8_t mgmt_addr_index);
void lldp_port_type_to_string (vtss_appl_lldp_remote_entry_t   *entry, lldp_printf_t lldp_printf);
void lldp_get_mib_stats (vtss_appl_lldp_global_counters_t   *stat);
vtss_appl_lldp_remote_entry_t   *lldp_remote_get_next(lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx);
vtss_appl_lldp_remote_entry_t   *lldp_remote_get_next_non_zero_addr(lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx, u8 mgmt_addr_index);
vtss_appl_lldp_remote_entry_t   *lldp_remote_get(lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx);
void lldp_remote_system_capa_to_string (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr);

BOOL lldp_remote_receive_port_to_string (mesa_port_no_t port_number, char *string_ptr, vtss_isid_t  isid );
vtss_appl_lldp_remote_entry_t   *lldp_remote_get_entries(void);
lldp_u8_t lldp_remote_get_ieee_draft_version(lldp_u8_t port_index);
void lldp_remote_set_ieee_draft_version(lldp_u8_t port_index, lldp_u8_t value);
lldp_u8_t lldp_remote_get_ieee_poe_tlv_length(lldp_u8_t port_index);
void lldp_remote_set_ieee_poe_tlv_length(lldp_u8_t port_index, lldp_u8_t value);
void lldp_mib_stats_clear(void);

char *vtss_lldp_mgmt_addr_and_type2string(vtss_appl_lldp_remote_entry_t *entry, const u8 mgmt_addr_index, char *string_ptr, const size_t string_ptr_len);

mesa_rc lldp_remote_table_init(void);

#ifdef VTSS_SW_OPTION_POE
lldp_u16_t lldp_remote_get_requested_power(lldp_u8_t port_index);
void lldp_remote_set_requested_power(lldp_u8_t port_index, lldp_u16_t value);
lldp_bool_t lldp_remote_get_poe_power_info(const vtss_appl_lldp_remote_entry_t *entry, int *power_type, int *power_source, int *power_priority, int *power_value);
void lldp_remote_set_lldp_info_valid(lldp_port_t port_index, lldp_u8_t value);
lldp_u8_t lldp_remote_lldp_is_info_valid(lldp_port_t port_index);
#endif
VTSS_END_HDR
#endif
