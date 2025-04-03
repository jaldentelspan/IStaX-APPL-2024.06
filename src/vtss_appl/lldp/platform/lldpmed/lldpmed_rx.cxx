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


/*****************************************************************************
* This file contains code for handling received LLDP-MED frames. LLDP-MED is
* defined in TIA-1057. The Version used for delevoping this code is ANSI/TIA-1057-2006
* released at April 6, 2006.
******************************************************************************/
#include "lldp_trace.h"
#include "vtss_lldp.h"
#include "lldp_os.h"
#include "lldp_private.h"
#include "lldpmed_rx.h"
#include "lldpmed_shared.h"
#include "misc_api.h"
#include "lldp_api.h"

CapArray<lldp_bool_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> medTransmitEnabled;
//
// Gets the "global" variable medTransmitEnabled specified in TIA1057
//
// In: port_idx - Index for the port in question
//     tx_enable - New value for medTransmitEnabled
//     action : action to done, TX_EN_SET or TX_EN_GET
//
// Return: Current value of medTransmitEnabled

lldp_bool_t lldpmed_medTransmitEnabled_get(lldp_port_t port_idx)
{
    return medTransmitEnabled[port_idx];
}

//
// Sets/Gets the "global" variable medTransmitEnabled specified in TIA1057
//
// In: port_idx - Index for the port in question
//     tx_enable - New value for medTransmitEnabled
//     action : action to done, TX_EN_SET or TX_EN_GET
//
// Return: Current value of medTransmitEnabled

void lldpmed_medTransmitEnabled_set(lldp_port_t port_idx, lldp_bool_t tx_enable)
{
    medTransmitEnabled[port_idx] = tx_enable;
}




CapArray<lldp_bool_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> medFastStart; // "global" variable medFastStart specified in TIA1057
//
// Gets the "global" variable medFastStart specified in TIA1057
//
// In: port_idx - Index for the port in question
//
// Return: Current value of medTransmitEnabled

lldp_u8_t lldpmed_medFastStart_timer_get(lldp_port_t port_idx)
{
    T_NG_PORT(TRACE_GRP_MED_RX, (mesa_port_no_t)port_idx, "lldpmed_medFastStart_timer_get, medFastStart:%d", medFastStart[port_idx]);
    return medFastStart[port_idx];
}

//
// Performs an "action" to the "global" variable medFastStart specified in TIA1057
//
// In: port_idx - Index for the port in question
//     medFastStart_value - New value for medFastStart
//     action : Set = set medTransmitEnabled to medFastStart_value, GET = get medTransmitEnabled, DECREMENT = sumtract one from medFastStart
//

void lldpmed_medFastStart_timer_action(lldp_port_t port_idx, lldp_u8_t medFastStart_value, lldpmed_fast_start_repeat_count_t action)
{
    if (action == SET) {
        medFastStart[port_idx] = medFastStart_value;
    }

    if (action == DECREMENT) {
        if (medFastStart[port_idx] != 0) {
            medFastStart[port_idx]--;
        }
    }
}

// Converting a coordinate TLV to a printable string
//
// In : location_info - Containing the TLV information
//
// In/Out: string_ptr - Pointer to the string.
//
static void lldpmed_frm2location(lldp_u8_t *frm, vtss_appl_lldp_med_location_info_t *location)
{

    T_NG(TRACE_GRP_MED_RX, "Entering lldpmed_coordinate2str");
    lldp_8_t tude_str[255]; // Dummy string

    // Calculate Latitude, Figure 9, TIA1057
    location->latitude_res = (frm[0] >> 2) & 0x3f;
    /*lint --e{571} */

    location->latitude = ((lldp_u64_t)(frm[0] & 0x3) << 32) |
                         (frm[1] << 24 & 0xFF000000) |
                         (frm[2] << 16 & 0x00FF0000) |
                         (frm[3] << 8 & 0x0000FF00) |
                         (frm[4] << 0  & 0x000000FF);


    T_DG(TRACE_GRP_MED_RX, "latitude_res:%d, latitude:" VPRI64u", 0x" VPRI64x", frm:0x%X, 0x%X, 0x%X, 0x%X, 0x%X", location->latitude_res, location->latitude, location->latitude, frm[0], frm[1], frm[2], frm[3], frm[4]);

    //Integer part is the upper 9 bits, fraction 25 bits , Section 2.1 RFC3825,July2004
    if (lldpmed_tude2decimal_str(location->latitude_res, location->latitude, 9, 25, tude_str, 4, FALSE)) {
        location->latitude_dir = SOUTH;
    } else {
        location->latitude_dir = NORTH;
    }

    // Calculate Longitude, Figure 9, TIA1057
    location->longitude_res = (frm[5] >> 2) & 0x3f;

    /*lint --e{571} */
    location->longitude = ((lldp_u64_t)(frm[5] & 0x3) << 32) |
                          (frm[6] << 24 & 0xFF000000) |
                          (frm[7] << 16 & 0x00FF0000) |
                          (frm[8] << 8  & 0x0000FF00) |
                          (frm[9] << 0  & 0x000000FF);

    T_DG(TRACE_GRP_MED_RX, "longitude_res:%d, longitude:" VPRI64u", 0x" VPRI64x, location->longitude_res, location->longitude, location->longitude);


    //Integer part is the upper 9 bits, fraction 25 bits , Section 2.1 RFC3825,July2004
    if (lldpmed_tude2decimal_str(location->longitude_res, location->longitude, 9, 25, tude_str, 4, FALSE)) {
        location->longitude_dir = WEST;
    } else {
        location->longitude_dir = EAST;
    }

    // Find Altitude, Figure 9, TIA1057
    location->altitude_type = (vtss_appl_lldp_med_at_type_t) (frm[10] >> 4); // Altitude type is the 4 upper bits, Figure 9, TIA1057
    location->altitude_res = ((frm[10] & 0x0F) << 2) + ((frm[11]  >> 6) & 0x3); // resolution bits, Figure 9, TIA1057

    T_DG(TRACE_GRP_MED_RX, "resolution:%d, 0x%X, 0x%X", location->altitude_res, frm[10], frm[11]);

    // Altitude, Figure 9,TIA1057
    location->altitude = ((frm[11] << 24 & 0x3F000000) |
                          (frm[12] << 16 & 0x00FF0000) |
                          (frm[13] << 8  & 0x0000FF00) |
                          (frm[14] << 0  & 0x000000FF));

    T_NG(TRACE_GRP_MED_RX, "altitude:%d, 0x%X", location->altitude, location->altitude);
    // Add Datum
    location->datum = (vtss_appl_lldp_med_datum_t)frm[15]; // Datum, Figure 9, TIA1057
}


//
// Validate a LLDP_TLV_ORG_TLV TLV, and updates the LLDP entry table with the
// information
//
// In: tlv - Pointer to the TLV that shall be validated
//         len - The length of the TLV
//
// Out: rx_entry - Pointer to the entry that shall have information updated
//
// Return: TRUE is the TLV was accepted.
lldp_bool_t lldpmed_validate_lldpdu(lldp_u8_t *tlv, lldp_rx_remote_entry_t *rx_entry, lldp_u16_t len, lldp_bool_t first_lldpmed_tlv)
{
    T_NG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "ORG TLV = 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X",
              tlv[0], tlv[1], tlv[2], tlv[3], tlv[4], tlv[5], tlv[6], tlv[7], tlv[8]);

    lldp_bool_t org_tlv_supported = FALSE; // By default we don't support any org tlvs
    lldp_u8_t  org_oui[3];
    lldp_u8_t  p_index = 0;

    // Figure 6, TIA1057
    org_oui[0]    = (u8)tlv[2];
    org_oui[1]    = (u8)tlv[3];
    org_oui[2]    = (u8)tlv[4];

    if (first_lldpmed_tlv) {
        for (p_index = 0; p_index < VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT; p_index ++) {
            rx_entry->policy[p_index].in_use = FALSE; //
        }
        rx_entry->lldpmed_coordinate_location_vld = 0; // Store that this location field isn't used.
    }

    const lldp_u8_t LLDP_TLV_OUI_TIA[]  = {0x00, 0x12, 0xBB}; //  Figure 12 in TIA1057

    // Check if the OUI is of TIA-1057 type
    if (memcmp(org_oui, &LLDP_TLV_OUI_TIA[0], 3) == 0) {
        switch (tlv[5]) {
        case 0x01: // Subtype Capabilities - See Figure 6 in TIA1057.
            if (len == 7) { // Length Must always be 7 for this TLV.
                rx_entry->lldpmed_capabilities_vld = TRUE;
                rx_entry->lldpmed_capabilities = ((u8)tlv[6] << 8) + (u8)tlv[7];
                rx_entry->lldpmed_device_type  = (u8)tlv[8];
                org_tlv_supported = TRUE;
                lldpmed_medTransmitEnabled_set(rx_entry->receive_port, LLDP_TRUE); // Set medTransmitEnabled according to section 11.2.1 bullet a), TIA1057
                rx_entry->lldpmed_capabilities_current |= 0x1; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT

                T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "Capa = 0x%X", rx_entry->lldpmed_capabilities );
            }
            break;

        case 0x02: // Subtype Policy - See Figure 7 in TIA1057.
            if (len == 8) { // Length Must always be 8 for this TLV.
                rx_entry->lldpmed_capabilities_current |= 0x2; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT

                // Multiple Policy TLVs allowed within one LLDPDU - Section 10.2.3.1 in TIA1057
                for (p_index = 0; p_index < VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT; p_index ++) {
                    org_tlv_supported = TRUE;
                    T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "in_use:%d, p_index:%d", rx_entry->policy[p_index].in_use, p_index);
                    if (rx_entry->policy[p_index].in_use) {
                        // policy field is already used, continue to the next field.
                    } else {
                        // Figure 7, TIA1057
                        u32 network_policy = (((u8) tlv[6]) << 24) | (((u8) tlv[7]) << 16) | (((u8)tlv[8]) << 8)  | (u8) tlv[9];
                        rx_entry->policy[p_index].network_policy.tagged_flag         = (network_policy & 0x00400000) != 0;
                        rx_entry->policy[p_index].network_policy.unknown_policy_flag = (network_policy & 0x00800000) != 0;

                        // Default values (Selected to 0 to signal "ignore")
                        rx_entry->policy[p_index].network_policy.vlan_id             = 0;
                        rx_entry->policy[p_index].network_policy.l2_priority         = 0;
                        rx_entry->policy[p_index].network_policy.dscp_value          = 0;

                        // From TIA1058-1057-2006, section 10.2.3.2  - A value of 1 indicates that the network policy for the specified application type is currently unknown.
                        //                                             In this case, the VLAN ID, Layer 2 priority and DSCP value fields are ignored.
                        if (rx_entry->policy[p_index].network_policy.unknown_policy_flag == 0) {
                            rx_entry->policy[p_index].network_policy.dscp_value          =  network_policy & 0x3F;

                            // From TIA1057-1057-2006, Section 10.2.3.3 : A value of 0 indicates that the device is using an untagged frame format and as such does not include
                            // a tag header as defined by IEEE 802.1Q-2003 [4]. In this case, both the VLAN ID and the Layer 2 priority fields are ignored and only the DSCP value has relevance.
                            if (rx_entry->policy[p_index].network_policy.tagged_flag) {
                                rx_entry->policy[p_index].network_policy.vlan_id             = (network_policy >> 9) & 0xFFF;
                                rx_entry->policy[p_index].network_policy.l2_priority         = (network_policy >> 6) & 0x7;
                            }
                        }

                        rx_entry->policy[p_index].network_policy.application_type    = (vtss_appl_lldp_med_application_type_t) (network_policy >> 24);
                        rx_entry->policy[p_index].in_use = TRUE; // Store that this policy field is used.
                        T_NG(TRACE_GRP_MED_RX, "p_index:%d, network_policy:0x%X, application_type:%d, l2:%d", p_index, network_policy, rx_entry->policy[p_index].network_policy.application_type, rx_entry->policy[p_index].network_policy.l2_priority);
                        break; // Policy field updated, continue to next TLV.
                    }
                }
            }
            break;

        case 0x03: { // Subtype Location - See Figure 8 in TIA1057.
            rx_entry->lldpmed_capabilities_current |= 0x4; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT
            // As I understand it then according  to section 10.2.4 in TIA1057 we shall support multiple location TLVs ( For now 3, See table 14, TIA1057)

            lldp_u16_t length = (((u8)tlv[0] & 0x1) << 8) + (u8)tlv[1] - 5; // Figure 8 + Section 10.2.4.1, TIA1057
            lldp_u16_t format = (u16)tlv[6];  // Figure 8, TIA1057

            T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "LLDP-MED Location subtype format = %d, length = %d",
                      format,
                      length);


            switch (format) {
            case LLDPMED_LOCATION_COORDINATE:
                if (length != 16) { // Fixed length for coordinate-based LCI - Table 14, TIA1057
                    T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "Invalid LLDP location length:%u ", length);
                    rx_entry->lldpmed_coordinate_location_vld = 0; // Store that this location field isn't used.
                    org_tlv_supported = FALSE;
                } else {
                    org_tlv_supported = TRUE;
                    rx_entry->lldpmed_coordinate_location_vld = 1; // Store that this location field is used.
                    lldpmed_frm2location(&tlv[7], &rx_entry->coordinate_location);
                }
                break;

            case LLDPMED_LOCATION_CIVIC:
                rx_entry->lldpmed_civic_location_vld  = TRUE;
                rx_entry->lldpmed_civic_location_length = length;
                rx_entry->lldpmed_civic_location      = &tlv[7]; // Figure 8, TIA1057
                org_tlv_supported                     = TRUE;
                break;

            case LLDPMED_LOCATION_ECS:
                rx_entry->lldpmed_ecs_location_length = length;
                rx_entry->lldpmed_ecs_location_vld    = TRUE;
                rx_entry->lldpmed_ecs_location        = &tlv[7]; // Figure 8, TIA1057
                org_tlv_supported                     = TRUE;
                break;

            default:
                T_DG(TRACE_GRP_MED_RX, "Unknown location format:%d", format);
                org_tlv_supported = FALSE;
            }

            T_RG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "lldpmed_coordinate_location_vld:%d", rx_entry->lldpmed_coordinate_location_vld);
            break; // Location field updated, continue to next TLV.
        }
        case 0x04: // Subtype PoE - See Figure 12 in TIA1057
            // Handled specially and the line below should never be reached - see vtss_lldp.c file
            T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "%s", "Unexpected PoE TLV");
            org_tlv_supported = FALSE;
            break;


        // Hardware revision TLV - TIA1057, Figure 13
        case 0x5:
            rx_entry->lldpmed_capabilities_current |= 0x20; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT

            // See section 10.2.6.1.1 in TIA1057
            if (len >= 4) {
                org_tlv_supported = TRUE;
                rx_entry->lldpmed_hw_rev_length = MIN(len - 4, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX);
            } else {
                // Shall never happen
                rx_entry->lldpmed_hw_rev_length = 0;
                T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "%s", "Hardware revision TLV length too short");
            }

            rx_entry->lldpmed_hw_rev =  &tlv[6];
            break;

        // Firmware revision TLV - TIA1057, Figure 14
        case 0x6:
            rx_entry->lldpmed_capabilities_current |= 0x20; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT

            // See section 10.2.6.2.1 in TIA1057
            if (len >= 4) {
                rx_entry->lldpmed_firm_rev_length = MIN(len - 4, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX);
                org_tlv_supported = TRUE;
            } else {
                // Shall never happen
                rx_entry->lldpmed_firm_rev_length = 0;
                T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "%s", "Firmware revision TLV length too short");
            }

            rx_entry->lldpmed_firm_rev = &tlv[6];
            break;

        // Software revision TLV - TIA1057, Figure 15
        case 0x7:
            rx_entry->lldpmed_capabilities_current |= 0x20; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT

            // See section 10.2.6.3.1 in TIA1057
            if (len >= 4) {
                rx_entry->lldpmed_sw_rev_length = MIN(len - 4, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX);
                org_tlv_supported = TRUE;
            } else {
                // Shall never happen
                rx_entry->lldpmed_sw_rev_length = 0;
                T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "%s", "Software revision TLV length too short");
            }

            rx_entry->lldpmed_sw_rev = &tlv[6];
            break;

        // Serial number TLV - TIA1057, Figure 16
        case 0x8:
            rx_entry->lldpmed_capabilities_current |= 0x20; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT

            // See section 10.2.6.4.1 in TIA1057
            if (len >= 4) {
                rx_entry->lldpmed_serial_no_length = MIN(len - 4, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX);
                org_tlv_supported = TRUE;
            } else {
                // Shall never happen
                rx_entry->lldpmed_serial_no_length = 0;
                T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "%s", "Serial number TLV length too short");
            }

            rx_entry->lldpmed_serial_no = &tlv[6];
            break;

        // Manufacturer name TLV - TIA1057, Figure 17
        case 0x9:
            rx_entry->lldpmed_capabilities_current |= 0x20; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT

            // See section 10.2.6.5.1 in TIA1057
            if (len >= 4) {
                rx_entry->lldpmed_manufacturer_name_length = MIN(len - 4, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX);
                org_tlv_supported = TRUE;
            } else {
                // Shall never happen
                rx_entry->lldpmed_manufacturer_name_length = 0;
                T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "%s", "Manufacturer name TLV length too short");
            }

            rx_entry->lldpmed_manufacturer_name = &tlv[6];
            break;

        // Model name TLV - TIA1057, Figure 18
        case 0xA:
            rx_entry->lldpmed_capabilities_current |= 0x20; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT


            // See section 10.2.6.6.1 in TIA1057
            if (len >= 4) {
                rx_entry->lldpmed_model_name_length = MIN(len - 4, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX);
                org_tlv_supported = TRUE;
            } else {
                // Shall never happen
                rx_entry->lldpmed_model_name_length = 0;
                T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "%s", "Model name TLV length too short");
            }

            rx_entry->lldpmed_model_name = &tlv[6];
            break;

        // Asset ID - TIA1057, Figure 19
        case 0xB:
            rx_entry->lldpmed_capabilities_current |= 0x20; // See TIA1057 MIBS LLDPXMEDREMCAPCURRENT

            // See section 10.2.6.7.1 in TIA1057
            if (len >= 4) {
                rx_entry->lldpmed_asset_id_length = MIN(len - 4, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX);
                org_tlv_supported = TRUE;
            } else {
                // Shall never happen
                rx_entry->lldpmed_asset_id_length = 0;
                T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "%s", "Asset ID TLV length too short");
            }

            rx_entry->lldpmed_asset_id = &tlv[6];
            break;

        default:
            org_tlv_supported = FALSE;
        }
    }

    T_DG_PORT(TRACE_GRP_MED_RX, rx_entry->receive_port, "org_tlv_supported = 0x%X", org_tlv_supported);
    return org_tlv_supported;
}


//
// Getting the number of policies within an entry
//
// In : Pointer to the Entry containing the information
//
// Return : Number of policies
//


lldp_u8_t lldpmed_get_policies_cnt (vtss_appl_lldp_remote_entry_t   *entry)
{

    lldp_u8_t policy_cnt = 0;
    lldp_u8_t p_index;

    for (p_index = 0; p_index < VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT; p_index ++) {
        if (entry->policy[p_index].in_use) {
            policy_cnt++;
        }
    }

    return policy_cnt;
}

// Converting a hw rev to a printable string
//
// In : Pointer to the Entry containing the information
//
// In/Out: Pointer to the string.
//
// Return : Pointer to the string.
lldp_8_t *lldpmed_hw_rev2str(vtss_appl_lldp_remote_entry_t *entry, lldp_8_t *string_ptr)
{
    strncpy(string_ptr, entry->lldpmed_hw_rev, entry->lldpmed_hw_rev_length);
    T_DG(TRACE_GRP_MED_RX, "entry->lldpmed_hw_rev_length = %d", entry->lldpmed_hw_rev_length);
    string_ptr[entry->lldpmed_hw_rev_length] = '\0';// Clear the string since data from the LLDP entry does contain NULL pointe
    return string_ptr;
}

// Converting a firmware rev to a printable string
//
// In : Pointer to the Entry containing the information
//
// In/Out: Pointer to the string.
//
// Return : Pointer to the string.
const lldp_8_t *vtss_appl_lldp_med_invertory_info_get(vtss_appl_lldp_remote_entry_t *entry, vtss_appl_lldp_med_inventory_type_t type, u8 max_length, lldp_8_t *string_ptr)
{
    lldp_u8_t *inventory_string_string_len = NULL;
    lldp_8_t  *inventory_string            = NULL;

    switch (type) {
    case LLDPMED_HW_REV:
        inventory_string_string_len = &entry->lldpmed_hw_rev_length;
        inventory_string            = &entry->lldpmed_hw_rev[0];
        break;
    case LLDPMED_FW_REV:
        inventory_string_string_len = &entry->lldpmed_firm_rev_length;
        inventory_string            = &entry->lldpmed_firm_rev[0];
        break;
    case LLDPMED_SW_REV:
        inventory_string_string_len = &entry->lldpmed_sw_rev_length;
        inventory_string            = &entry->lldpmed_sw_rev[0];
        break;
    case LLDPMED_SER_NUM:
        inventory_string_string_len = &entry->lldpmed_serial_no_length;
        inventory_string            = &entry->lldpmed_serial_no[0];
        break;
    case LLDPMED_MANUFACTURER_NAME:
        inventory_string_string_len = &entry->lldpmed_manufacturer_name_length;
        inventory_string            = &entry->lldpmed_manufacturer_name[0];
        break;
    case LLDPMED_MODEL_NAME:
        inventory_string_string_len = &entry->lldpmed_model_name_length;
        inventory_string            = &entry->lldpmed_model_name[0];
        break;
    case LLDPMED_ASSET_ID:
        inventory_string_string_len = &entry->lldpmed_asset_id_length;
        inventory_string            = &entry->lldpmed_asset_id[0];
        break;
    }

    if (inventory_string == NULL || inventory_string_string_len == NULL) {
        T_E("Shall never happen");
        return "";
    }

    if (max_length < (u8)*inventory_string_string_len) {
        T_I("string too small :%d, *inventory_string_string_len:%d", max_length, *inventory_string_string_len);
        return "";
    }

    strncpy(string_ptr, inventory_string, *inventory_string_string_len);
    T_DG(TRACE_GRP_MED_RX, "inventory_string_string_len = %d", *inventory_string_string_len);
    string_ptr[*inventory_string_string_len] = '\0';// Add NULL pointer to the string, since data from the LLDP entry does contain NULL pointer
    return string_ptr;
}

// Converting a device type to a printable string
//
// In : Pointer to the Entry containing the information
//
// In/Out: Pointer to the string.
//
void lldpmed_device_type2str(vtss_appl_lldp_remote_entry_t *entry, lldp_8_t *string_ptr)
{
    T_NG(TRACE_GRP_MED_RX, "entry->lldpmed_device_type = %d", entry->lldpmed_device_type);
    switch (entry->lldpmed_device_type) {
    case LLDPMED_DEVICE_TYPE_NOT_DEFINED:
        sprintf(string_ptr, "%s", "Type Not Defined");
        break;

    case LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_I:
        sprintf(string_ptr, "%s", "Endpoint Class I");
        break;

    case LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_II:
        sprintf(string_ptr, "%s", "Endpoint Class II");
        break;

    case LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_III:
        sprintf(string_ptr, "%s", "Endpoint Class III");
        break;

    case LLDPMED_DEVICE_TYPE_NETWORK_CONNECTIVITY:
        sprintf(string_ptr, "%s", "Network Connectivity");
        break;

    default: /* reserved */
        sprintf(string_ptr, "%s", "Reserved");
        break;

    }
}

// Converting a LLDP-MED capabilities to a printable string
//
// In : Pointer to the Entry containing the information
//
// In/Out: Pointer to the string.
//
void lldpmed_capabilities2str (vtss_appl_lldp_remote_entry_t *entry, lldp_8_t *string_ptr )
{

    T_NG_PORT(TRACE_GRP_MED_RX, entry->receive_port, "Entering lldpmed_capabilities2str, capa = %d", entry->lldpmed_capabilities);

    // See Table 10, TIA1057 for capabilities.
    lldp_u8_t bit_no;
    lldp_bool_t print_comma = LLDP_FALSE;

    // Define the capabilities in Table 10 in TIA1057
    const lldp_8_t *sys_capa[] = {"LLDP-MED Capabilities",
                                  "Network Policy",
                                  "Location Identification",
                                  "Extended Power via MDI - PSE",
                                  "Extended Power via MDI - PD",
                                  "Inventory",
                                  "Reserved"
                                 };

    strcpy(string_ptr, "");
    // Make the string with comma between each capability.
    for (bit_no = 0; bit_no < 7; bit_no++) {
        if (entry->lldpmed_capabilities & (1 << bit_no)) {
            if (print_comma) {
                strncat(string_ptr, ", ", 254);
            }
            strncat(string_ptr, sys_capa[bit_no], 254);
            print_comma = LLDP_TRUE;
        }
    }
}

// Converting a LLDP-MED application type to a printable string
//
// In : appl_type_value - The application type
//
// In/Out: string_ptr - Pointer to the string.
//
char *lldpmed_appl_type2str(vtss_appl_lldp_med_application_type_t appl_type_value, char *string_ptr)
{

    // Define the application types in Table 12 in TIA1057
    const lldp_8_t *appl_type_str[] = {"Reserved",
                                       "Voice",
                                       "Voice Signaling",
                                       "Guest Voice",
                                       "Guest Voice signaling",
                                       "Softphone Voice",
                                       "Video Conferencing",
                                       "Streaming Video",
                                       "Video Signaling",
                                       "Reserved"
                                      };

    T_NG(TRACE_GRP_MED_RX, "Appl_type_value = %d, appl_type = %s", appl_type_value, appl_type_str[appl_type_value]);

    // Make the string
    if ((lldp_u16_t)appl_type_value < VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT) { // Make sure that we do point outside the array.
        sprintf(string_ptr, "%s", appl_type_str[appl_type_value]);
    } else {
        sprintf(string_ptr, "%s", "Reserved");
    }
    return string_ptr;
}

u64 lldp_power_ten(u8 power)
{
    switch (power) {
    case 1:
        return 10;
    case 2:
        return 100;
    case 3:
        return 1000;
    case 4:
        return 10000;
    case 5:
        return 100000;
    case 6:
        return 1000000;
    case 7:
        return 10000000;
    case 8:
        return 100000000;
    case 9:
        return 1000000000;

    default:
        T_E("Please update table for %d", power);
        return 1;
    }
}

// Converting a 64 bits number of a integer and fraction part(where it is possible to specify
// the number of bits used for both the integer and fraction part) to at printable string.
//
// In : res - Resolution. How many of the tude bits that are valid.
//      tude - The 64 bits number that should be converted. ( Comes from latitude, logitude and altitude)
//      int_bits - Number of bits used for the integer part.
//      frac_bits - Number of bits used for the fraction part.
// In/Out: string_ptr - Pointer to the string.
//
lldp_bool_t lldpmed_tude2decimal_str(lldp_u8_t res, lldp_u64_t tude, lldp_u8_t int_bits, lldp_u8_t frac_bits, lldp_8_t *string_ptr, u8 digit_cnt, BOOL ignore_neg_number)
{
    lldp_u8_t p_index;
    lldp_u8_t eraseBits = int_bits + frac_bits - res;
    lldp_u64_t mask = 0;
    lldp_bool_t neg_result = LLDP_FALSE;


    if (eraseBits > 0) while (eraseBits--) {
            mask = (mask << 1) | 1;
        }
    tude &= ~mask;

    // Create a mask matching the number of bits used for the integer and fraction parts.
    int int_bits_mask = ((1 << int_bits) - 1);
    int frac_bits_mask = ((1 << frac_bits) - 1);

    lldp_u32_t int_part = (tude >> (frac_bits)) & int_bits_mask;  // Example - Integer part is the upper 9 bits, Section 2.1 RFC3825,July2004
    lldp_u32_t fraction = tude & frac_bits_mask; // Example - Fraction is the lower 25 bits, Section 2.1 RFC3825,July2004

    // Check if MSB if the integer part is set ( negative number )
    if (int_part & (1 << (int_bits - 1))) {
        int_part = ~int_part & int_bits_mask;
        fraction = ~fraction & frac_bits_mask;
        neg_result = LLDP_TRUE;
    }

    T_NG(TRACE_GRP_MED_RX, "tude:0x" VPRI64x", int_bits_mask:0x%X, frac_bits_mask:0x%X, int_part:%u, fraction:%u, neg_result:%d, res:0x%X, mask:" VPRI64x,
         tude, int_bits_mask, frac_bits_mask, int_part, fraction, neg_result, res, mask);

    u8 max_digit_cnt = 7;
    lldp_u64_t digi_mulitiplier = lldp_power_ten(max_digit_cnt); // We calculate the fragment value with 7 digits (As long as the number of fragment bits allows it)

    // Calculate the fraction value;
    lldp_u32_t fraction_val = 0;

    lldp_u32_t value = (lldp_u32_t)(0.5 * 10 * digi_mulitiplier);

    for (p_index = 0; p_index < res - int_bits; p_index ++ ) {
        if (fraction & (1 << (frac_bits - 1 - p_index))) {
            fraction_val += value;
        }
        T_RG(TRACE_GRP_MED_RX, "fraction_val:%d, value:%d, fraction:0x%X", fraction_val, value, fraction);
        value /= 2;
    }

    u32 mod = fraction_val % lldp_power_ten(max_digit_cnt - digit_cnt + 1);
    T_IG(TRACE_GRP_MED_RX, "fraction_val:%d, max_digit_cnt:%d, digit_cnt:%d", fraction_val, max_digit_cnt, digit_cnt);
    fraction_val = fraction_val / lldp_power_ten(max_digit_cnt - digit_cnt + 1);

    T_IG(TRACE_GRP_MED_RX, "fraction_val:%d, value:%d, mod:%d pow:" VPRI64u", " VPRI64u, fraction_val, value, mod, lldp_power_ten(max_digit_cnt - digit_cnt), (mod / lldp_power_ten(max_digit_cnt - digit_cnt)));

    // Round up if needed
    if (mod / lldp_power_ten(max_digit_cnt - digit_cnt) >= 5 ) {
        fraction_val += 1;

        // Check if we wrapped over.
        if (fraction_val >= lldp_power_ten(digit_cnt)) {
            fraction_val = 0;
            int_part++;
        }
    }

    T_IG(TRACE_GRP_MED_RX, "fraction_val:%d, value:%d", fraction_val, value);

    // Generate printable string containing "integer.fraction"
    sprintf(&string_ptr[0], "%s%u.%0*d", (neg_result && !ignore_neg_number) ? "-" : "", int_part, digit_cnt, fraction_val);
    return neg_result;
}



//
// Converting altitude type to a printable string
//
// In : at - altitude type
//
// In/Out: string_ptr - Pointer to the string.
//
void lldpmed_at2str(vtss_appl_lldp_med_at_type_t at, lldp_8_t *string_ptr)
{
    // meters or floor, RFC3825,July2004 section 2.1
    switch (at) {
    case METERS:
        strcpy(string_ptr, " meter(s)");
        break;
    case FLOOR:
        strcpy(string_ptr, " floor");
        break;
    default:
        strcpy(string_ptr, "");
        // undefined at the moment.
        break;
    }
}


//
// Converting map datum type to a printable string
//
// In : datume - The datum
//
// In/Out: string_ptr - Pointer to the string.
//
void lldpmed_datum2str(vtss_appl_lldp_med_datum_t datum, lldp_8_t *string_ptr)
{
    // meters or floor, RFC3825,July2004 section 2.1
    switch (datum) {
    case WGS84:
        strcpy(string_ptr, "WGS84");
        break;
    case NAD83_NAVD88:
        strcpy(string_ptr, "NAD83/NAVD88");
        break;
    case NAD83_MLLW         :
        strcpy(string_ptr, "NAD83/MLLW");
        break;
    default:
        // undefined at the moment.
        strcpy(string_ptr, "");
        break;
    }
}


// Converting a coordinate TLV to a printable string
//
// In : location_info - Containing the TLV information
//
// In/Out: string_ptr - Pointer to the string.
//
static void location_info_t2str (vtss_appl_lldp_med_location_info_t *location_info, lldp_8_t *string_ptr)
{
    lldp_8_t tude_str[255];
    T_NG(TRACE_GRP_MED_RX, "location_info->latitude_res:%d, latitude:" VPRI64u, location_info->latitude_res, location_info->latitude);

    strcpy(string_ptr, "Latitude:");
    //Integer part is the upper 9 bits, fraction 25 bits , Section 2.1 RFC3825,July2004
    if (lldpmed_tude2decimal_str(location_info->latitude_res, location_info->latitude, 9, 25, tude_str, 4, location_info->latitude_dir == SOUTH)) {
        strcat(string_ptr, tude_str);
        strcat(string_ptr, " South, ");
    } else {
        strcat(string_ptr, tude_str);
        strcat(string_ptr, " North, ");
    }

    // Calculate Longitude, Figure 9, TIA1057
    strcat(string_ptr, "Longitude:");

    //Integer part is the upper 9 bits, fraction 25 bits , Section 2.1 RFC3825,July2004
    if (lldpmed_tude2decimal_str(location_info->longitude_res, location_info->longitude, 9, 25, tude_str, 4, location_info->longitude_dir == WEST )) {
        strcat(string_ptr, tude_str);
        strcat(string_ptr, " West, ");
    } else {
        strcat(string_ptr, tude_str);
        strcat(string_ptr, " East, ");
    }


    // Find Altitude, Figure 9, TIA1057
    strcat(string_ptr, "Altitude:");

    // RFC3825,July2004 section 2.1 - 22 bits integer and 8 bits fraction.
    (void) lldpmed_tude2decimal_str(location_info->altitude_res, location_info->altitude, 22, 8, tude_str, 1, FALSE);

    T_RG(TRACE_GRP_CLI, "altitude_res:%d, altitude:0x%X", location_info->altitude_res, location_info->altitude);
    strcat(string_ptr, tude_str);

    // Add altitude
    lldp_8_t altitude_type_str[25];
    lldpmed_at2str(location_info->altitude_type, altitude_type_str); // Get altitude type as string
    strcat(string_ptr, altitude_type_str); // Add altitude type to the total string.


    // Add Datum
    lldp_8_t datum_str[25];
    lldpmed_datum2str(location_info->datum, datum_str); // Get datum as string
    strcat(string_ptr, ", Map datum:");
    strcat(string_ptr, datum_str); // Add datum to the total string.

}

// Function for converting from the index used when storing the catype configuration and
// the lldpmed_catype_t type.
//
// In : Index - CA TYPE index.
//
// Return: lldpmed_catype_t corresponding to the index.
//

vtss_appl_lldp_med_catype_t lldpmed_index2catype(lldp_u8_t ca_index)
{
    // See LLDP-MED catype for civic location, Section 3.4, Annex B, TIA1057
    if (ca_index < 6) {
        return (vtss_appl_lldp_med_catype_t) (ca_index + 1);
    } else {
        return (vtss_appl_lldp_med_catype_t) (ca_index + 10);
    }
}

// Function for converting from the ca_type to index used when storing the catype configuration and
// the vtss_appl_lldp_med_catype_t type.
//
// In : ca_type - The CA TYPE.
//
// Return: Index for how the CA TYPE is stored in configuration.
//
lldp_u8_t lldpmed_catype2index(vtss_appl_lldp_med_catype_t ca_type)
{
    // See LLDP-MED catype for civic location, Section 3.4, Annex B, TIA1057
    if (ca_type < 7) {
        return (vtss_appl_lldp_med_catype_t) (ca_type - 1);
    } else {
        return (vtss_appl_lldp_med_catype_t) (ca_type - 10);
    }
}


// Getting Ca type as printable string
//
// In : ca_type - CA TYPE as integer
//
// In/Out: string_ptr - Pointer to the string.
//
const lldp_8_t *lldpmed_catype2str(vtss_appl_lldp_med_catype_t ca_type)
{
    // Table in ANNEX B, TIA1057
    switch (ca_type) {
    case LLDPMED_CATYPE_A1:
        return "National subdivision";

    case LLDPMED_CATYPE_A2:
        return "County";

    case LLDPMED_CATYPE_A3:
        return "City";

    case LLDPMED_CATYPE_A4:
        return "City district";

    case LLDPMED_CATYPE_A5:
        return "Block (Neighborhood)";

    case LLDPMED_CATYPE_A6:
        return "Street";

    case LLDPMED_CATYPE_PRD:
        return "Street Direction";

    case LLDPMED_CATYPE_POD:
        return "Trailling Street";

    case LLDPMED_CATYPE_STS:
        return "Street Suffix";

    case LLDPMED_CATYPE_HNO:
        return "House No.";

    case LLDPMED_CATYPE_HNS:
        return "House No. Suffix";

    case LLDPMED_CATYPE_LMK:
        return "Landmark";

    case LLDPMED_CATYPE_LOC:
        return "Additional Location Info";

    case LLDPMED_CATYPE_NAM:
        return "Name";

    case LLDPMED_CATYPE_ZIP:
        return "Zip Code";

    case LLDPMED_CATYPE_BUILD:
        return "Building";

    case LLDPMED_CATYPE_UNIT:
        return "Unit";

    case LLDPMED_CATYPE_FLR:
        return "Floor";

    case LLDPMED_CATYPE_ROOM:
        return "Room No.";

    case LLDPMED_CATYPE_PLACE:
        return "Placetype";

    case LLDPMED_CATYPE_PCN:
        return "Postal Community Name";

    case LLDPMED_CATYPE_POBOX:
        return "P.O. Box";

    case LLDPMED_CATYPE_ADD_CODE:
        return "Addination Code";

    default:
        return "Unknown ca type";
    }
}

// Function for getting a pointer that point to a entry in vtss_appl_lldpmed_civic_t struct.
// IN : civic   - Pointer to the struct.
//    : ca_type - Which Civic address to point to
// Return - Point to the struct entry.
char *civic_ptr_get(vtss_appl_lldpmed_civic_t *civic, vtss_appl_lldp_med_catype_t ca_type)
{
    switch (ca_type) {
    case LLDPMED_CATYPE_A1:
        return &civic->a1[0];
    case LLDPMED_CATYPE_A2:
        return &civic->a2[0];
    case LLDPMED_CATYPE_A3:
        return &civic->a3[0];
    case LLDPMED_CATYPE_A4:
        return &civic->a4[0];
    case LLDPMED_CATYPE_A5:
        return &civic->a5[0];
    case LLDPMED_CATYPE_A6:
        return &civic->a6[0];
    case LLDPMED_CATYPE_PRD:
        return &civic->prd[0];
    case LLDPMED_CATYPE_POD:
        return &civic->pod[0];
    case LLDPMED_CATYPE_STS:
        return &civic->sts[0];
    case LLDPMED_CATYPE_HNO:
        return &civic->hno[0];
    case LLDPMED_CATYPE_HNS:
        return &civic->hns[0];
    case LLDPMED_CATYPE_LMK:
        return &civic->lmk[0];
    case LLDPMED_CATYPE_LOC:
        return &civic->loc[0];
    case LLDPMED_CATYPE_NAM:
        return &civic->nam[0];
    case LLDPMED_CATYPE_ZIP:
        return &civic->zip[0];
    case LLDPMED_CATYPE_BUILD:
        return &civic->build[0];
    case LLDPMED_CATYPE_UNIT:
        return &civic->unit[0];
    case LLDPMED_CATYPE_FLR:
        return &civic->flr[0];
    case LLDPMED_CATYPE_ROOM:
        return &civic->room[0];
    case LLDPMED_CATYPE_PLACE:
        return &civic->place[0];
    case LLDPMED_CATYPE_PCN:
        return &civic->pcn[0];
    case LLDPMED_CATYPE_POBOX:
        return &civic->pobox[0];
    case LLDPMED_CATYPE_ADD_CODE:
        return &civic->add_code[0];
    default:
        T_I("Unknown ca_type:%d", ca_type);
        return NULL;
    }
}

// Function for converting from vtss_appl_lldp_med_civic_tlv_format_t to vtss_appl_lldpmed_civic_t
// IN  - civic_tlv - Pointer to the vtss_appl_lldp_med_civic_tlv_format_t to convert
// OUT - civic     - Pointer to where to put the converted type
// return - VTSS_RC_OK if civic is valid, else error code.
mesa_rc civic_tlv2civic(const vtss_appl_lldp_med_civic_tlv_format_t *civic_tlv, vtss_appl_lldpmed_civic_t *civic)
{
    u8 ca_index = 0;
    u8 ca_type  = LLDPMED_CATYPE_A1;
    u8 ca_len   = 0;

    // Start by clearing all the entries in the struct
    for (ca_type = LLDPMED_CATYPE_UNDEFINED; ca_type <= LLDPMED_CATYPE_ADD_CODE; ca_type++) {
        char *ca_ptr = civic_ptr_get(civic, (vtss_appl_lldp_med_catype_t)ca_type);

        if (ca_ptr == NULL) {
            continue;
        }
        strcpy(ca_ptr, "");
    }

    // Pick out the civic addresses in the civic tlv type and put them into the struct.
    while (ca_index < VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX && civic_tlv->ca_value[ca_index] != LLDPMED_CATYPE_UNDEFINED) {
        ca_type  = civic_tlv->ca_value[ca_index++];
        ca_len   = civic_tlv->ca_value[ca_index++];

        T_IG(TRACE_GRP_MED_RX, "ca_type:%d, ca_len:%d, ca_index:%d", ca_type, ca_len, ca_index);

        // Check that we don't go out of bounce
        T_RG(TRACE_GRP_MED_RX, "ca_len:%d, ca_index:%d", ca_len, ca_index);
        // Totally we must not exceed 252 bytes (including ca-type and ca-length), but one CA must not exceed 250 bytes (Section 10.2.4.3.2 in TIA1057).
        if (ca_len > VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX) {
            T_IG(TRACE_GRP_MED_RX, "VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX");
            return VTSS_APPL_LLDP_ERROR_CIVIC_EXCEED;
        }

        if ((ca_len + ca_index) > VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_INCL_TYPE_MAX) {
            T_IG(TRACE_GRP_MED_RX, "VTSS_APPL_LLDP_ERROR_CIVIC_EXCEED");
            return VTSS_APPL_LLDP_ERROR_CIVIC_EXCEED;
        }

        if (ca_type > LLDPMED_CATYPE_ADD_CODE) {
            T_IG(TRACE_GRP_MED_RX, "Invalid ca type:%d", ca_type);
            return VTSS_APPL_LLDP_ERROR_CIVIC_TYPE;
        }


        char *ca_ptr = civic_ptr_get(civic, (vtss_appl_lldp_med_catype_t)ca_type);

        if (ca_ptr == NULL) {
            continue; // Skipping invalid types.
        }

        memcpy(ca_ptr, &civic_tlv->ca_value[ca_index], ca_len);
        ca_ptr[ca_len] = '\0'; // Add NULL pointer since that is not in the TLVs
        ca_index += ca_len;
    }
    return VTSS_RC_OK;
}

// Opposite of civic_tlv2civic - See civic_tlv2civic
mesa_rc civic2civic_tlv(vtss_appl_lldpmed_civic_t *civic, vtss_appl_lldp_med_civic_tlv_format_t *civic_tlv)
{
    u8 ca_type;
    memset(civic_tlv, LLDPMED_CATYPE_UNDEFINED, sizeof(vtss_appl_lldp_med_civic_tlv_format_t));
    u16 ca_index = 0;
    for (ca_type = LLDPMED_CATYPE_UNDEFINED; ca_type <= LLDPMED_CATYPE_ADD_CODE; ca_type++) {
        T_NG(TRACE_GRP_MED_RX, "ca_type:%d", ca_type);
        char *ca_ptr = civic_ptr_get(civic, (vtss_appl_lldp_med_catype_t)ca_type);

        if (ca_ptr == NULL) {
            continue; // Skipping invalid CA types
        }

        u8 ca_len = strlen(ca_ptr);
        if (ca_len == 0) {
            continue; // Skipping empty strings
        }

        T_DG(TRACE_GRP_MED_RX, "ca_type:%d, ca_len:%d, ca_index:%d", ca_type, ca_len, ca_index);

        if (civic_tlv == NULL) { // Just making sure
            T_E("civic_tlv is NULL");
            return VTSS_APPL_LLDP_ERROR_NULL_POINTER;
        }

        // Stop if we exceeds that maximum string length
        if (ca_len > VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX) {
            T_IG(TRACE_GRP_MED_RX, "VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX");
            return VTSS_APPL_LLDP_ERROR_CIVIC_EXCEED;
        }

        if ((ca_len + ca_index) > VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX) {
            T_IG(TRACE_GRP_MED_RX, "VTSS_APPL_LLDP_ERROR_CIVIC_EXCEED, ca_len:%d", ca_len);
            return VTSS_APPL_LLDP_ERROR_CIVIC_EXCEED;
        }

        civic_tlv->ca_value[ca_index++] = ca_type;
        civic_tlv->ca_value[ca_index++] = ca_len;
        memcpy(&civic_tlv->ca_value[ca_index], ca_ptr, ca_len);
        ca_index += ca_len;
    }

    // If we end the TLV string with less than maximum string length then mark the last as undefined.
    if (ca_index < VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_INCL_TYPE_MAX - 1) {
        civic_tlv->ca_value[ca_index] = LLDPMED_CATYPE_UNDEFINED;
    }

    return VTSS_RC_OK;
}

// See lldp.h
mesa_rc vtss_appl_lldp_location_civic_info_set(vtss_appl_lldp_med_civic_tlv_format_t *civic_tlv,
                                               vtss_appl_lldp_med_catype_t new_ca_type, const lldp_8_t *new_ca_value)
{
    vtss_appl_lldpmed_civic_t civic;

    // Convert to vtss_appl_lldpmed_civic_t, because that is easier to work with.
    VTSS_RC(civic_tlv2civic(civic_tlv, &civic));

    // Get pointer to the entry in the vtss_appl_lldpmed_civic_t struct for the new civic address
    char *ca_ptr = civic_ptr_get(&civic, new_ca_type);
    if (ca_ptr == NULL) {
        T_IG(TRACE_GRP_MED_RX, "Invalid ca type:%d", new_ca_type);
        return VTSS_APPL_LLDP_ERROR_CIVIC_TYPE;
    }

    T_DG(TRACE_GRP_MED_RX, "ca_ptr:%s, ca_type:%d", ca_ptr, new_ca_type);

    // Update the entry
    strncpy(ca_ptr, new_ca_value, strlen(new_ca_value) + 1);
    T_IG(TRACE_GRP_MED_RX, "ca_ptr:%s, new_ca_value:%s, len:" VPRIz, ca_ptr, new_ca_value, strlen(new_ca_value));

    // Convert back to the vtss_appl_lldp_med_civic_tlv_format_t type
    VTSS_RC(civic2civic_tlv(&civic, civic_tlv));

    return VTSS_RC_OK;
}

mesa_rc lldpmed_entry_country_code2str_get(vtss_appl_lldp_remote_entry_t *entry, char *string_ptr)
{
    if (!entry ->lldpmed_civic_location_vld) {
        strcat(string_ptr, "");
        return VTSS_RC_ERROR;
    }

    // Figure 10, TIA1057
    string_ptr[0] = (char)entry->lldpmed_civic_location[2];
    string_ptr[1] = (char)entry->lldpmed_civic_location[3];
    string_ptr[2] = '\0';

    return VTSS_RC_OK;
}

//
// Converting a civic TLV to a printable string
//
// In : civic_info - Containing the TLV civic information
//
// In/Out: string_ptr - Pointer to the string.
//
static void lldpmed_civic2str(lldp_u8_t *location_info, lldp_8_t *string_ptr)
{
    strcpy(string_ptr, "");
    lldp_u16_t lci_length = location_info[0] + 1; // Figure 10, TIA1057 + Section 10.2.4.3.2 (adding 1 to the LCI length)
//  lldp_u8_t what = location_info[1]; // Figure 10, TIA1057
    char country_code[3] = {(char)location_info[2], (char)location_info[3], '\0'}; // Figure 10, TIA1057
    vtss_appl_lldp_med_catype_t ca_type; // See Figure 10, TIA1057
    lldp_u8_t ca_length; // See Figure 10, TIA1057
    lldp_u16_t tlv_index = 4; // First CAtype is byte 4 in the TLV. Figure 10, TIA1057
    lldp_8_t str_buf[255]; // Temp. string buffer

    BOOL insert_dash = FALSE;

    if (strlen(country_code) != 0) {
        sprintf(string_ptr, "Country code:%s ", country_code);
        insert_dash = TRUE;
    }
    T_IG(TRACE_GRP_MED_RX, "lci_length:%d", lci_length);
    while (tlv_index < lci_length) {
        T_NG(TRACE_GRP_SNMP, "tlv_index:%d, lci_length:%d", tlv_index, lci_length);
        ca_type = (vtss_appl_lldp_med_catype_t) location_info[tlv_index];
        ca_length = location_info[tlv_index + 1];

        if (tlv_index +  2 + ca_length  > lci_length) {
            T_WG(TRACE_GRP_MED_RX, "Invalid CA length, tlv_index = %d, ca_length = %d, lci_length = %d",
                 tlv_index, ca_length, lci_length);
            break;
        }
        // Insert -- between each CA value
        if (insert_dash) {
            strcat(string_ptr, " -- ");
        }

        insert_dash = TRUE;
        // Add the ca type (as string) to the string
        strcpy(str_buf, lldpmed_catype2str(ca_type));
        strcat(string_ptr, str_buf);
        strcat(string_ptr, ":");

        //Add CA value,  Figure 10, TIA1057
        memcpy(&str_buf[0], &location_info[tlv_index + 2], ca_length);
        str_buf[ca_length] = '\0';
        strcat(string_ptr, str_buf);

        tlv_index += 2 + ca_length; // Select next CA
        T_RG(TRACE_GRP_MED_RX, "string_ptr = %s, str_buf = %s", string_ptr, str_buf);
    }
}

//
// Converting a ECS ELIN TLV to a printable string
//
// In : civic_info - Containing the TLV civic information
//
// In/Out: string_ptr - Pointer to the string.
//

static void lldpmed_elin2str (u8 *location_info, lldp_u8_t length,  lldp_8_t *string_ptr)
{
    // Figure 11, TIA1057
    strcpy(string_ptr, "Emergency Call Service:");
    strncat(string_ptr, (lldp_8_t *) location_info, length);
}

// Converting a LLDP-MED location to a printable string
//
// In : Entry containing the location
//
// In/Out: string_ptr - Pointer to the string.
//
void lldpmed_location2str(vtss_appl_lldp_remote_entry_t *entry, lldp_8_t *string_ptr, lldpmed_location_type_t type)
{
    strcpy(string_ptr, "");
    switch (type)  {

    case LLDPMED_LOCATION_INVALID:
        sprintf(string_ptr, "%s", "Invalid location ID");
        break;

    case LLDPMED_LOCATION_COORDINATE:
        if (!entry->lldpmed_coordinate_location_vld) {
            strcat(string_ptr, "");
            return;
        }
        location_info_t2str(&entry->coordinate_location, string_ptr);
        break
        ;
    case LLDPMED_LOCATION_CIVIC:
        T_IG(TRACE_GRP_MED_RX, "lldpmed_civic_location_vld:%d", entry->lldpmed_civic_location_vld);
        if (!entry->lldpmed_civic_location_vld) {
            strcat(string_ptr, "");
            return;
        }
        lldpmed_civic2str(entry->lldpmed_civic_location, string_ptr);
        break;

    case LLDPMED_LOCATION_ECS:
        if (!entry->lldpmed_elin_location_vld) {
            strcat(string_ptr, "");
            return;
        }
        lldpmed_elin2str(&entry->lldpmed_elin_location[0], entry->lldpmed_elin_location_length, string_ptr);
        break;
    default:
        sprintf(string_ptr, "%s", "Reserved for future expansion");
    }
}

//
// Converting a LLDP-MED policy Policy Flag to a printable string
//
// In : The policy
//
// In/Out: Pointer to the string.
//
const char *lldpmed_policy_flag_type2str(BOOL unknown_flag)
{
    return unknown_flag ? "Unknown" : "Defined";
}

//
// Converting a LLDP-MED policy TAG to a printable string
//
// In : The policy
//
const char *lldpmed_policy_tag2str(BOOL tagged)
{
    return tagged ? "Tagged" : "Untagged";
}

//
// Converting a LLDP-MED policy VLAN ID to a printable string
//
// In : The policy
//
// In/Out: Pointer to the string.
//
char *lldpmed_policy_vlan_id2str(vtss_appl_lldp_med_policy_t &policy, char *string_ptr)
{
    // Make the string
    if (!policy.network_policy.tagged_flag || policy.network_policy.unknown_policy_flag)  { // See Section 10.2.3.2 and 10.2.3.3 in TIA1057
        sprintf(string_ptr, "%s", "-");
    } else {
        sprintf(string_ptr, "%u", policy.network_policy.vlan_id);
    }
    return string_ptr;
}

// Converting a LLDP-MED policy priority to a printable string
//
// In : The policy
//
// In/Out: Pointer to the string.
//
char *lldpmed_policy_prio2str(vtss_appl_lldp_med_policy_t &policy, char *string_ptr)
{
    // Make the string
    if (!policy.network_policy.tagged_flag || policy.network_policy.unknown_policy_flag)  { // See Section 10.2.3.2 and 10.2.3.3 in TIA1057
        sprintf(string_ptr, "%s", "-");
    } else {
        sprintf(string_ptr, "%u", policy.network_policy.l2_priority);
    }
    return string_ptr;
}

// Converting LLDP-MED policy dscp to a printable string
//
// In : The policy
//
// In/Out: Pointer to the string.
//
char *lldpmed_policy_dscp2str(vtss_appl_lldp_med_policy_t &policy, char *string_ptr)
{
    // Make the string
    if (policy.network_policy.unknown_policy_flag)  { // See Section 10.2.3.2 in TIA1057
        sprintf(string_ptr, "%s", "-");
    } else {
        sprintf(string_ptr, "%u", policy.network_policy.dscp_value);
    }
    return string_ptr;
}

//
// Returns true if the LLDP entry table needs to be updated with the information in the received LLDP frame.
//
lldp_bool_t lldpmed_update_neccessary(lldp_rx_remote_entry_t *rx_entry, vtss_appl_lldp_remote_entry_t *entry)
{
    if (rx_entry->lldpmed_info_vld         != entry->lldpmed_info_vld ||
        rx_entry->lldpmed_capabilities_vld != entry->lldpmed_capabilities_vld ||
        rx_entry->lldpmed_capabilities     != entry->lldpmed_capabilities ||
        rx_entry->lldpmed_device_type      != entry->lldpmed_device_type ||
        rx_entry->lldpmed_civic_location_vld      != entry->lldpmed_civic_location_vld ||
        rx_entry->lldpmed_ecs_location_vld        != entry->lldpmed_elin_location_vld ||
        rx_entry->lldpmed_coordinate_location_vld != entry->lldpmed_coordinate_location_vld ||
        memcmp(&rx_entry->coordinate_location, &entry->coordinate_location, sizeof(vtss_appl_lldp_med_location_info_t)) != 0 ||
        memcmp(rx_entry->policy, entry->policy, sizeof(entry->policy)) != 0 ||
        memcmp(rx_entry->lldpmed_hw_rev, entry->lldpmed_hw_rev, rx_entry->lldpmed_hw_rev_length) != 0 ||
        memcmp(rx_entry->lldpmed_firm_rev, entry->lldpmed_firm_rev, rx_entry->lldpmed_firm_rev_length) != 0 ||
        memcmp(rx_entry->lldpmed_sw_rev, entry->lldpmed_sw_rev, rx_entry->lldpmed_sw_rev_length) != 0 ||
        memcmp(rx_entry->lldpmed_serial_no, entry->lldpmed_serial_no, rx_entry->lldpmed_serial_no_length) != 0 ||
        memcmp(rx_entry->lldpmed_manufacturer_name, entry->lldpmed_manufacturer_name, rx_entry->lldpmed_manufacturer_name_length) != 0 ||
        memcmp(rx_entry->lldpmed_model_name, entry->lldpmed_model_name, rx_entry->lldpmed_model_name_length) != 0 ||
        memcmp(rx_entry->lldpmed_asset_id, entry->lldpmed_asset_id, rx_entry->lldpmed_asset_id_length) != 0) {
        T_IG(TRACE_GRP_MED_RX, "Update needed");
        return LLDP_TRUE;
    }

    if (rx_entry->lldpmed_civic_location_vld) {
        if (memcmp(entry->lldpmed_civic_location, rx_entry->lldpmed_civic_location, rx_entry->lldpmed_civic_location_length) != 0) {
            T_IG(TRACE_GRP_MED_RX, "Update needed");
            return LLDP_TRUE;
        }
    }

    if (rx_entry->lldpmed_ecs_location_vld) {
        if (memcmp(entry->lldpmed_elin_location, rx_entry->lldpmed_ecs_location, rx_entry->lldpmed_ecs_location_length) != 0) {
            T_IG(TRACE_GRP_MED_RX, "Update needed");
            return LLDP_TRUE;
        }
    }

    if (rx_entry->lldpmed_coordinate_location_vld) {
        if (memcmp(&entry->coordinate_location, &rx_entry->coordinate_location, sizeof(entry->coordinate_location)) != 0) {
            T_IG(TRACE_GRP_MED_RX, "Update needed");
            return LLDP_TRUE;
        }
    }

    T_NG(TRACE_GRP_MED_RX, "No Update needed");
    return LLDP_FALSE;
}

//
// Copies the information from the received packet (rx_enty) to the entry table.
//
void lldpmed_update_entry(lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry)
{
    T_DG_PORT(TRACE_GRP_MED_RX, (mesa_port_no_t)entry->receive_port, "%s", "lldpmed_update_entry LLDP-MEM update");

    // Section 11.2.4, TIA1057 specifies that medFastStart timer must be set if
    // the LLDP-MED capabilities TLV has changed
    vtss_appl_lldp_common_conf_t conf;
    (void) lldp_common_local_conf_get(&conf); // Get current configuration

    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;
    (void) lldp_conf_get(&port_conf[0]); // Get current configuration

    if ((rx_entry->lldpmed_capabilities != entry->lldpmed_capabilities) ||
        (rx_entry->lldpmed_info_vld &&  !entry->lldpmed_info_vld)) {
        T_NG_PORT(TRACE_GRP_MED_RX, (mesa_port_no_t)entry->receive_port,
                  "rx.lldpmed_capabilities:%lu, entry.lldpmed_capabilities:%d, rx.lldpmed_info_vld:%d, entry.lldpmed_info_vld:%d",
                  rx_entry->lldpmed_capabilities, entry->lldpmed_capabilities, rx_entry->lldpmed_info_vld, entry->lldpmed_info_vld);

#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
        if (port_conf[entry->receive_port].lldpmed_device_type == VTSS_APPL_LLDP_MED_END_POINT) {
            // No Frame reception is need - TIA1057, section 11.3.4
        } else {
#endif
            lldpmed_medFastStart_timer_action(rx_entry->receive_port, conf.medFastStartRepeatCount, SET); // Section 11.2.4, TIA1057
#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
        }
#endif
    }


    // Do the update
    entry->lldpmed_info_vld           = rx_entry->lldpmed_info_vld;
    entry->lldpmed_capabilities_vld   = rx_entry->lldpmed_capabilities_vld;
    entry->lldpmed_capabilities       = rx_entry->lldpmed_capabilities;
    entry->lldpmed_capabilities_current = rx_entry->lldpmed_capabilities_current;
    entry->lldpmed_device_type        = (vtss_appl_lldp_med_remote_device_type_t)rx_entry->lldpmed_device_type;
    memcpy(entry->policy, rx_entry->policy, sizeof(entry->policy));

    if (rx_entry->lldpmed_civic_location_vld) {
        memcpy(entry->lldpmed_civic_location, rx_entry->lldpmed_civic_location, rx_entry->lldpmed_civic_location_length);
    }
    entry->lldpmed_civic_location_length    = rx_entry->lldpmed_civic_location_length;
    entry->lldpmed_civic_location_vld       = rx_entry->lldpmed_civic_location_vld;

    if (rx_entry->lldpmed_ecs_location_vld) {
        memcpy(entry->lldpmed_elin_location, rx_entry->lldpmed_ecs_location, rx_entry->lldpmed_ecs_location_length);
    }
    entry->lldpmed_elin_location_length    = rx_entry->lldpmed_ecs_location_length;
    entry->lldpmed_elin_location_vld       = rx_entry->lldpmed_ecs_location_vld;

    entry->lldpmed_coordinate_location_vld   = rx_entry->lldpmed_coordinate_location_vld;
    memcpy(&entry->coordinate_location, &rx_entry->coordinate_location, sizeof(vtss_appl_lldp_med_location_info_t));

    entry->lldpmed_hw_rev_length = rx_entry->lldpmed_hw_rev_length;
    memcpy(entry->lldpmed_hw_rev, rx_entry->lldpmed_hw_rev, rx_entry->lldpmed_hw_rev_length);

    entry->lldpmed_firm_rev_length = rx_entry->lldpmed_firm_rev_length;
    memcpy(entry->lldpmed_firm_rev, rx_entry->lldpmed_firm_rev, rx_entry->lldpmed_firm_rev_length);

    entry->lldpmed_sw_rev_length = rx_entry->lldpmed_sw_rev_length;
    memcpy(entry->lldpmed_sw_rev, rx_entry->lldpmed_sw_rev, rx_entry->lldpmed_sw_rev_length);

    entry->lldpmed_serial_no_length = rx_entry->lldpmed_serial_no_length;
    memcpy(entry->lldpmed_serial_no, rx_entry->lldpmed_serial_no, rx_entry->lldpmed_serial_no_length);

    entry->lldpmed_manufacturer_name_length = rx_entry->lldpmed_manufacturer_name_length;
    memcpy(entry->lldpmed_manufacturer_name, rx_entry->lldpmed_manufacturer_name, rx_entry->lldpmed_manufacturer_name_length);

    entry->lldpmed_model_name_length = rx_entry->lldpmed_model_name_length;
    memcpy(entry->lldpmed_model_name, rx_entry->lldpmed_model_name, rx_entry->lldpmed_model_name_length);

    entry->lldpmed_asset_id_length  = rx_entry->lldpmed_asset_id_length;
    memcpy(entry->lldpmed_asset_id, rx_entry->lldpmed_asset_id, rx_entry->lldpmed_asset_id_length);
}
