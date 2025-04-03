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

/*****************************************************************************
* This file contains code for handling received LLDP-MED frames. LLDP-MED is
* defined in TIA-1057. The Version used for delevoping this code is ANSI/TIA-1057-2006
* released at April 6, 2006.
******************************************************************************/
#include "vtss_lldp.h"
#include "lldp_trace.h"
#include "lldp_tlv.h"
#include "lldp_os.h"
#include "lldp_private.h"
#include "lldpmed_shared.h"
#include "lldpmed_tx.h"
#include "lldpmed_rx.h"
#include "board_if.h"
#include "vtss_api_if_api.h"
#include "lldp_api.h"
#include <vtss/appl/port.h>

//
// Converting a 64 bits number to 2's Complement
//
// In : tude_val - The 64 bits number that shall be converted.
//      negative_number - Indicate that the number is negative ( The direction bit for the latitude and longitude ).
//      fraction_bits_cnt - Telling how many bits that are used for the fraction part.
//      digits            - The number of digits in the tude_val. E.g. if digits = 4 and tude_val is 1001234 then it is intperted as 100.1234
//      mask              - Number of valid bit in the 2's complement number (Figure 9, in TIA1057)
// Out: tude_out_val - The 2's complement result
void lldpmed_cal_fraction(lldp_64_t tude_val, lldp_bool_t negative_number, lldp_u32_t fraction_bits_cnt, lldp_64_t *tude_out_val, u8 digits, lldp_u64_t mask)
{
    lldp_u32_t fraction;
    lldp_u32_t int_part;

    lldp_64_t tude_val_local;
    lldp_u32_t tude_multiplier = lldp_power_ten(digits);
    if (tude_val < 0) {
        tude_val_local = ~tude_val + 1; // Convert to positive number
        negative_number = TRUE;
    } else {
        tude_val_local = tude_val;
    }

    T_DG(TRACE_GRP_MED_TX, "tude_val:0x" VPRI64x", tude_val_local:0x" VPRI64x, tude_val, tude_val_local);
    int_part  = tude_val_local / tude_multiplier; // The input is multiplied with tude_multiplier to avoid using floating point variable.
    // Example: web input = 33.04, tude_multiplier = 100 gives tude_val_local = 3304. When dividing
    // 3304 with 100 we comes back to 33.

    lldp_u32_t tude_frac_part = (tude_val_local - ((lldp_64_t)int_part * tude_multiplier)); // Example: tude_val_local = 3304, int_part just calculated to 33 then fraction becomes (3304 - 33*100 = 4).

    T_NG(TRACE_GRP_MED_TX, "int_part = %u, tude_frac_part = %u", int_part, tude_frac_part);
    fraction = 0;

    // Loop through all franction bits. 1st bit = 0.5, 2nd bit = 0.25, 3rd bit = 0.125 and so on.
    // We increase the number of digits with 3 (multiply by 1000) in order to make sure that we get enough bits included to represent the requested number of Digits.
    lldp_u32_t frac_cal = (lldp_u32_t)(0.5 * tude_multiplier * 1000);
    tude_frac_part      = tude_frac_part * 1000;

    lldp_16_t  frac_bit;
    for (frac_bit = fraction_bits_cnt - 1; frac_bit >= 0; frac_bit --) {
        T_RG(TRACE_GRP_MED_TX, "frac_cal = %u,frac_bit = %d, tude_frac_part = %u, fraction = %u", frac_cal, frac_bit, tude_frac_part, fraction);
        T_IG(TRACE_GRP_MED_TX, "frac_bit:%d, frac_cal:%u, tude_frac_part:%u", frac_bit, frac_cal, tude_frac_part);
        if (frac_cal == 0) {
            break;
        }

        if (tude_frac_part >=  frac_cal) {
            fraction |= 1 << frac_bit;
            tude_frac_part -= frac_cal;
        }
        frac_cal /= 2;
    }

    // If it is a negative number the bits has to be inverted.
    if (negative_number) {
        fraction = ~fraction ;
        int_part = ~int_part;
        T_IG(TRACE_GRP_MED_TX, "int_part:0x%X, fraction:0x%X", int_part, fraction);
    }
    fraction &= ((1 << fraction_bits_cnt) - 1);

    T_IG(TRACE_GRP_MED_TX, "int_part:0x%X, fraction:0x%X", int_part, fraction);
    *tude_out_val =  (((u64) int_part)  << fraction_bits_cnt) | fraction;

    T_IG(TRACE_GRP_MED_TX, "Out:" VPRI64x VPRI64x, *tude_out_val, mask);
    if (negative_number) {
        *tude_out_val |= ~mask; // Set non-valid bit to one.
    } else {
        *tude_out_val &= mask; // Set non-valid bit to zero.
    }

    T_NG(TRACE_GRP_MED_TX, "Out:" VPRI64x, *tude_out_val);
}

//
// Create LLDP-MED capabilities 16 bits word for this device
//
// Return: 16 bit bitmask for capabilities supported
lldp_u16_t lldpmed_get_capabilities_word(lldp_port_t port_idx)
{
    lldp_u16_t capa_bits = 0;
    lldp_u8_t poe_bit = 0;

#ifdef VTSS_SW_OPTION_POE
    // Get PoE hardware board configuration
    poe_entry_t poe_hw_conf = {};
    poe_hw_conf = poe_hw_config_get(port_idx, &poe_hw_conf);

    // Only set PoE if PoE is available for this port
    if (poe_hw_conf.available) {
        poe_bit = 1; // We are a PSE.
    }
#endif

    // See Table 10, TIA1057
    capa_bits  = 1 |  // LLDP-MED capbilities
#ifdef VTSS_SW_OPTION_LLDP_MED_NETWORK_POLICY
                 1 << 1 |
#endif
                 1 << 2 | // Location bit
                 poe_bit << 3 |
                 0 << 4 |
                 0 << 5;

    return capa_bits;
}

//
// Append LLDP-MED capabilities TLVs to the LLDP frame being build.
//
// In:
//
// In/Out: tlv - Pointer to buffer containing the LLDP frame
//
// Return: The length of the buffer
static lldp_u8_t lldpmed_capabilities_tlv_add(lldp_u8_t *buf, lldp_port_t port_idx )
{
    lldp_u8_t i = 0;
    lldp_u16_t capabilities_bits = lldpmed_get_capabilities_word(port_idx);
    T_IG_PORT(TRACE_GRP_MED_TX, port_idx, "capabilities_bits:0x%X", capabilities_bits);
    buf[i++] = 0x01;   // Capabilities ID Subtype - Figure 6,TIA1057
    buf[i++] = (lldp_u8_t)((capabilities_bits >> 8) & 0xFF);
    buf[i++] = (lldp_u8_t)(capabilities_bits & 0xFF);

#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> conf;
    (void) lldp_conf_get(&conf[0]); // Get current configuration

    if (conf[port_idx].lldpmed_device_type == VTSS_APPL_LLDP_MED_END_POINT) {
        buf[i++] = LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_I; // We always act as endpoint class I when in endpoint mode.
    } else
#endif
    {
        buf[i++] = LLDPMED_DEVICE_TYPE_NETWORK_CONNECTIVITY; // Not end point mode then always connectivity  mode
    }

    return i;
}

//
// Append LLDP-MED policies TLVs to the LLDP frame being build.
//
// In:
//
// In/Out: tlv - Pointer to buffer containing the LLDP frame
//
// Return: The length of the buffer
static lldp_u8_t vtss_appl_lldp_med_network_policy_tlv_add(lldp_u8_t *buf, lldp_u8_t policy_idx)
{

    lldp_u8_t i = 0;


    vtss_appl_lldp_common_conf_t  conf;
    (void) lldp_common_local_conf_get(&conf); // Get current configuration

    vtss_appl_lldp_med_policy_t policy;
    VTSS_RC(lldp_conf_policy_get(policy_idx, &policy));

    if (policy.in_use == 0) {
        // Skip not in use policies
        T_NG(TRACE_GRP_MED_TX, "Non active policy(%d) configured for port", policy_idx);
    } else {
        // Ok - Policy configured for this port. Generate TLV
        buf[i++] = 0x02;   // Policy ID Subtype - Figure 7,TIA1057
        buf[i++] = (lldp_u8_t) policy.network_policy.application_type; // Figure 7,TIA1057
        buf[i++] = (policy.network_policy.tagged_flag << 6) | ((policy.network_policy.vlan_id & 0xF80) >> 7 ); // Figure 7,TIA1057
        buf[i++] = (((policy.network_policy.vlan_id & 0x7F) << 1) |
                    ((policy.network_policy.l2_priority & 0x04) >> 2)); // Figure 7,TIA1057
        buf[i++] = (((policy.network_policy.l2_priority & 0x03) << 6)  |
                    policy.network_policy.dscp_value); // Figure 7,TIA1057
        T_NG(TRACE_GRP_MED_TX, "buf:0x%X, l2:%d", buf[i - 1], policy.network_policy.l2_priority);
    }
    return i;
}

//
// Append LLDP-MED location TLVs to the LLDP frame being build. This is basically the build of TIA1057, Figure 9
//
// In:
//
// In/Out: tlv - Pointer to buffer containing the LLDP frame
//
// Return: The length of the buffer
lldp_u8_t lldpmed_coordinate_location_tlv_add(lldp_u8_t   *buf)
{
    lldp_u8_t i = 0;

    vtss_appl_lldp_common_conf_t conf;
    (void) lldp_common_local_conf_get(&conf); // Get current configuration

    // Latitude
    buf[i++] = (LLDPMED_LONG_LATI_TUDE_RES << 2) | ((conf.coordinate_location.latitude & 0x0300000000) >> 32);
    buf[i++] = (conf.coordinate_location.latitude >> 24) & 0x000000FF;
    buf[i++] = (conf.coordinate_location.latitude >> 16) & 0x000000FF;
    buf[i++] = (conf.coordinate_location.latitude >> 8) & 0x000000FF;
    buf[i++] = conf.coordinate_location.latitude & 0x000000FF;

    // Longitude
    buf[i++] = (LLDPMED_LONG_LATI_TUDE_RES << 2) | ((conf.coordinate_location.longitude & 0x0300000000) >> 32);
    buf[i++] = (conf.coordinate_location.longitude >> 24) & 0x000000FF;
    buf[i++] = (conf.coordinate_location.longitude >> 16) & 0x000000FF;
    buf[i++] = (conf.coordinate_location.longitude >> 8) & 0x000000FF;
    buf[i++] = conf.coordinate_location.longitude & 0x000000FF;

    T_NG(TRACE_GRP_MED_TX, "longitude:0x" VPRI64x", latitude:0x" VPRI64x, conf.coordinate_location.longitude, conf.coordinate_location.latitude);

    // Altitude (The negative information is included in the altitude value, (Sign indication is not used, we set it to TRUE)
    buf[i++] = ((lldp_u8_t)(conf.coordinate_location.altitude_type) << 4) | (LLDPMED_ALTITUDE_RES >> 2); // Figure 9, TIA1057
    buf[i++] = ((LLDPMED_ALTITUDE_RES & 0x3) << 6) | ((conf.coordinate_location.altitude & 0x3F000000) >> 24); // Figure 9, TIA1057
    buf[i++] = (conf.coordinate_location.altitude & 0x00FF0000) >> 16;
    buf[i++] = (conf.coordinate_location.altitude & 0x0000FF00) >> 8;
    buf[i++] = (conf.coordinate_location.altitude & 0x000000FF);

    buf[i++] = (lldp_u8_t) conf.coordinate_location.datum;
    return i;
}

//
// Append LLDP-MED ECS ELIN location TLVs to the LLDP frame being build.
//
// In:
//
// In/Out: tlv - Pointer to buffer containing the LLDP frame
//
// Return: The length of the buffer
lldp_u8_t lldpmed_ecs_location_tlv_add(lldp_u8_t *buf)
{
    vtss_appl_lldp_common_conf_t conf;
    (void) lldp_common_local_conf_get(&conf); // Get current configuration

    lldp_u8_t len = strlen(conf.elin_location);
    memcpy(buf, conf.elin_location, len);
    T_DG(TRACE_GRP_MED_TX, "ecs = %s, len = %d", conf.elin_location, len);
    return len;
}

//
// Append LLDP-MED ECS Civic location TLVs to the LLDP frame being build.
//
// In/Out: tlv - Pointer to buffer containing the LLDP frame
//
// Return: The length of the buffer
lldp_u16_t lldpmed_civic_location_tlv_add(lldp_u8_t *buf)
{
    vtss_appl_lldp_common_conf_t conf;
    (void) lldp_common_local_conf_get(&conf); // Get current configuration
    lldp_u8_t buf_index = 0;


    u8 ca_type = conf.civic.ca_value[0];
    u8 ca_len  = conf.civic.ca_value[1];
    u8 ca_data_index  = 0;

    // Find the length of the configuration (stored as TLV)
    while (ca_type != LLDPMED_CATYPE_UNDEFINED && ca_data_index < VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_INCL_TYPE_MAX && ca_len > 0) {
        T_IG(TRACE_GRP_MED_TX, "CA type:%d, ca_len:%d, ca_index:%d", ca_type, ca_len, ca_data_index);

        if (ca_type > LLDPMED_CATYPE_ADD_CODE) {
            T_E("un-expected ca type:%d, len:%d, index:%d", ca_type, ca_len, ca_data_index);
            return 0;
        }

        ca_data_index += ca_len + 2; // +2 for the CAtype and CALength, Se TIA1057 figure 10.

        // Just making sure.
        if (ca_data_index > VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_INCL_TYPE_MAX) {
            // This shall never happen. Shall be checked in web/cli
            T_WG(TRACE_GRP_MED_TX, "Total CA Value information exceeds 250 bytes ca_data_index:%d",
                 ca_data_index);
            return 0;
        }

        if (ca_data_index == VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_INCL_TYPE_MAX) {
            // We have reach the end of the string.
            break;
        }

        // Point to the next CA
        ca_type = conf.civic.ca_value[ca_data_index];

        // Just making sure that we don't get out of bounds when we find the ca_len
        if (ca_data_index == VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_INCL_TYPE_MAX - 1) {
            if (ca_type != LLDPMED_CATYPE_UNDEFINED) {
                T_E("ca type is expected to be LLDPMED_CATYPE_UNDEFINED when we reach the end of the string");
            }
            break;
        }

        ca_len  = conf.civic.ca_value[ca_data_index + 1];
    }

    // Update the frame.
    buf[buf_index++] = ca_data_index + 3; // LCI length, Figure 10, TIA1057 ( +3 for "WHAT" and country code )
    buf[buf_index++] = 2; // The "WHAT" value. Not quite sure what to set it to

    // Country code, Figure 10, TIA1057
    buf[buf_index++] = conf.ca_country_code[0];
    buf[buf_index++] = conf.ca_country_code[1];

    if (ca_data_index > 1) {
        T_IG(TRACE_GRP_MED_TX, "CA type:%d, ca_len:%d, ca_index:%d", ca_type, ca_len, ca_data_index);
        memcpy(&buf[buf_index], &conf.civic.ca_value[0], ca_data_index);
    }
    return buf_index + ca_data_index;
}

//
// Append LLDP-MED TLVs to the LLDP frame being build.
//
// In: p_idx - Mulitple functions - Can be a "port index", a "policy index" or "location data format" depending upon the subtype.
//
// In/Out: Buf - Pointer to buffer containing the LLDP frame
//
// Return: The length of the buffer
static lldp_u16_t lldpmed_update_tlv(lldp_u8_t *buf, lldp_port_t p_idx, lldpmed_tlv_subtype_t subtype)
{
    T_NG(TRACE_GRP_MED_TX, "p_idx:%d, subtype:%d", p_idx, subtype);
    // TIA OUI - Figure 8,TIA1057
    lldp_u16_t i = 0;
    buf[i++] = 0x00;
    buf[i++] = 0x12;
    buf[i++] = 0xBB;

    switch (subtype) {
    case LLDPMED_TLV_SUBTYPE_LOCATION_ID:
        buf[i++] = 0x03;   // Location ID Subtype - Figure 8,TIA1057
        switch (p_idx) {
        case COORDINATE_BASED:
            T_NG(TRACE_GRP_MED_TX, "Adding COORDINATE_BASED tlv");
            buf[i++] = 0x01 ; // Location format, Table 14, TIA1057
            i += lldpmed_coordinate_location_tlv_add(&buf[i]) ;
            break;
        case CIVIC:
            buf[i++] = 0x02;   // Location data format  - Table 14,TIA1057
            i += lldpmed_civic_location_tlv_add(&buf[i]) ;
            break;
        case ECS:
            buf[i++] = 0x03;   // Location data format  - Table 14,TIA1057
            i += lldpmed_ecs_location_tlv_add(&buf[i]) ;
            T_DG(TRACE_GRP_MED_TX, "buf[%d]:0x%X, buf[%d]: 0x%X, buf[%d]:0x%X, buf[%d]:0x%X ", i - 3, buf[i - 3], i - 2, buf[i - 2], i - 1, buf[i - 1], i, buf[i]);

            break;
        default:
            T_W("Invalid location data format");
            i = 0; // Don't add the TLV;
        }
        break;

    case LLDPMED_TLV_SUBTYPE_NETWORK_POLICY:
        i += vtss_appl_lldp_med_network_policy_tlv_add(&buf[i], p_idx) ;
        break;

    case LLDPMED_TLV_SUBTYPE_CAPABILITIIES:
        i += lldpmed_capabilities_tlv_add(&buf[i], p_idx) ;
        break;

    default :
        T_W("Unknown subtype: %d", subtype);
        i = 0; // Don't add the TLV;
        break;
    }

    return i;
}

//
// Append MAC/PHY LLDP-MED TLVs to the LLDP frame being build.
// This isn't a LLDP-MED TLV but is included here because
// it so far is the only Organizationally Specific TLV (Annex G in IEEE802.1AB) supported.
// If we are going to support more Organizationally Specific TLVs, it might be a idea to
// move them to a separate file.
// The MAC/PHY TLV is support because it is require by TIA1057 (Section 7.2).
//
// In: port_idx - The port in question
//
// In/Out: Buf - Pointer to buffer containing the LLDP frame
//
// Return: The length of the buffer
static lldp_u8_t lldp_mac_phy_update_tlv(lldp_u8_t *buf, lldp_port_t port_idx)
{
    meba_port_cap_t         cap;
    int                     i = 0;
    vtss_ifindex_t          ifindex;
    vtss_appl_port_conf_t   port_conf;
    vtss_appl_port_status_t port_status;

    if (vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_idx, &ifindex) != VTSS_RC_OK ||
        vtss_appl_port_conf_get(ifindex, &port_conf)                != VTSS_RC_OK ||
        vtss_appl_port_status_get(ifindex, &port_status)            != VTSS_RC_OK) {
        T_W("Didn't get port configuation");
    } else {
        // TIA OUI - Figure G-1, IEEE802.1AB
        buf[i++] = 0x00;
        buf[i++] = 0x12;
        buf[i++] = 0x0F;

        buf[i++] = 0x01; //Subtype - Figure G-1, IEEE802.1AB

        // Auto-negotiation
        if (port_custom_table[port_idx].cap & MEBA_PORT_CAP_AUTONEG) {
            buf[i] =  0x1; // Auto-negotiation supported, Table G-2, IEEE802.1AB
        }
        if (port_conf.speed == MESA_SPEED_AUTO) {
            buf[i++] |=  1 << 1; // Auto-negotiation enabled, Table G-2, IEEE802.1AB
        }

        // PDM auto-negotiation. I could not find the definetion in RFC3636 as specified in IEEE802.1AB
        // but have used the definition found at http://standards.ieee.org/reading/ieee/interp/802.1AB.html

        // MSB
        buf[i] = 0; // Setting all PDM auto-negotiation advertised capabilities to 0 as a start


        if (port_custom_table[port_idx].cap & MEBA_PORT_CAP_100M_FDX) {
            buf[i] |= 0x04; // 100BASE-TX full duplex, http://standards.ieee.org/reading/ieee/interp/802.1AB.html
        }

        if (port_custom_table[port_idx].cap & MEBA_PORT_CAP_100M_HDX) {
            buf[i] |= 0x08; // 100BASE-TX halfduplex , http://standards.ieee.org/reading/ieee/interp/802.1AB.html
        }

        if (port_custom_table[port_idx].cap & MEBA_PORT_CAP_10M_FDX) {
            buf[i] |= 0x20; // 10BASE-TX full duplex, http://standards.ieee.org/reading/ieee/interp/802.1AB.html
        }

        if (port_custom_table[port_idx].cap & MEBA_PORT_CAP_10M_HDX) {
            buf[i] |= 0x40; // 10BASE-TX halfduplex , http://standards.ieee.org/reading/ieee/interp/802.1AB.html
        }

        i++;

        // LSB
        buf[i] = 0x0; // Settingv all PDM auto-negotiation advertised capabilities to 0 as a start

        cap = port_custom_table[port_idx].cap;

        if (cap & MEBA_PORT_CAP_1G_FDX) {
            if (cap & MEBA_PORT_CAP_DUAL_COPPER ||
                cap & MEBA_PORT_CAP_DUAL_FIBER) {
                buf[i] |= 0x04; // 1000BASE-X, http://standards.ieee.org/reading/ieee/interp/802.1AB.html
                buf[i] |= 0x01; // 1000BASE-T, http://standards.ieee.org/reading/ieee/interp/802.1AB.html
            } else if (port_custom_table[port_idx].cap & MEBA_PORT_CAP_FIBER) {
                buf[i] |= 0x04; // 1000BASE-X, http://standards.ieee.org/reading/ieee/interp/802.1AB.html
            } else if (port_custom_table[port_idx].cap & MEBA_PORT_CAP_COPPER   ) {
                buf[i] |= 0x01; // 1000BASE-T, http://standards.ieee.org/reading/ieee/interp/802.1AB.html
            }
        }

        if (port_conf.flow_control) {
            buf[i] |= 0x10; // Asymmetric and Symmetric PAUSE, http://standards.ieee.org/reading/ieee/interp/802.1AB.html
            buf[i] |= 0x80; // PAUSE, http://standards.ieee.org/reading/ieee/interp/802.1AB.html
        }

        i++;

        // Operation Mau type. Defined in RFC3636
        buf[i++] = 0x00; // Max defined dot3MauType so is 40, so we do never use the MSB byte.

        // dot3MauType vaues is defined in section 4 in RFC3636
        buf[i] = 0;   // Default to "no internal MAU"
        if (port_status.speed == MESA_SPEED_10M) {
            if (port_status.fdx) {
                buf[i] = 11;   // 10M, fullduplex
            } else {
                buf[i] = 10;   // 10M, halfduplex
            }
        }

        if (port_status.speed == MESA_SPEED_100M) {
            if (port_status.fiber == false && port_status.fdx) {
                buf[i] = 16;   // 100M, fullduplex
            } else if (port_status.fiber == false && port_status.fdx == false) {
                buf[i] = 15;   // 100M, halfduplex
            } else if (port_status.fiber && port_status.fdx) {
                buf[i] = 18;   // 100M Full duplex + Fiber {
            } else {
                buf[i] = 17;   // 100M Half duplex+ Fiber
            }
        }

        if (port_status.speed == MESA_SPEED_1G) {
            if (port_status.fiber) {
                buf[i] = 22;   // 1G Full duplex + Fiber
            } else {
                buf[i] = 30;   // 1G Full duplex + cobber
            }
        }

        i++;
    }
    return i;
}

lldp_u16_t lldpmed_tlv_add(lldp_u8_t *buf, lldp_port_t port_idx)
{
    lldp_u16_t tlv_info_len = 0;
    T_DG_PORT(TRACE_GRP_MED_TX, port_idx, "%s", "Entering lldpmed_tlv_add");

    // According to section 11.1, TIA1057 we shall only transmit LLDP-MED tlvs when
    // a LLDP-MED capabilities TLV has been received on the port.
    if (lldpmed_medTransmitEnabled_get(port_idx)) {
        CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> conf;
        (void) lldp_conf_get(&conf[0]); // Get current configuration

        // Capabilities MUST be the first lldp-med TLV in the frame. Section 10.2.2.3, TIA1057
        if (conf[port_idx].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT) {
            tlv_info_len +=  set_tlv_type_and_length_non_zero_len(buf + tlv_info_len, LLDP_TLV_ORG_TLV,
                                                                  lldpmed_update_tlv(buf + 2 + tlv_info_len, port_idx, LLDPMED_TLV_SUBTYPE_CAPABILITIIES));
        }

        // Location TLV
        if (conf[port_idx].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT) {
            tlv_info_len +=  set_tlv_type_and_length_non_zero_len(buf + tlv_info_len, LLDP_TLV_ORG_TLV,
                                                                  lldpmed_update_tlv(buf + 2 + tlv_info_len, (lldp_port_t)COORDINATE_BASED, LLDPMED_TLV_SUBTYPE_LOCATION_ID));

            tlv_info_len +=  set_tlv_type_and_length_non_zero_len(buf + tlv_info_len, LLDP_TLV_ORG_TLV,
                                                                  lldpmed_update_tlv(buf + 2 + tlv_info_len, (lldp_port_t)CIVIC, LLDPMED_TLV_SUBTYPE_LOCATION_ID));

            tlv_info_len +=  set_tlv_type_and_length_non_zero_len(buf + tlv_info_len, LLDP_TLV_ORG_TLV,
                                                                  lldpmed_update_tlv(buf + 2 + tlv_info_len, (lldp_port_t)ECS, LLDPMED_TLV_SUBTYPE_LOCATION_ID));


        }
        // Organizationally TLV
        tlv_info_len +=  set_tlv_type_and_length_non_zero_len(buf + tlv_info_len, LLDP_TLV_ORG_TLV,
                                                              lldp_mac_phy_update_tlv(buf + 2 + tlv_info_len, port_idx));

        T_NG_PORT(TRACE_GRP_MED_TX, port_idx, "optional_tlv:%d", conf[port_idx].lldpmed_optional_tlvs_mask);

        for (lldp_u8_t policy_idx = LLDPMED_POLICY_MIN; policy_idx <= LLDPMED_POLICY_MAX; policy_idx++) {
            BOOL port_policy_enabled;
            if (lldp_conf_port_policy_get(port_idx, policy_idx, &port_policy_enabled) != VTSS_RC_OK) {
                T_E("Could not get policy");
                return 0;
            }
            if (port_policy_enabled) {
                if (conf[port_idx].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT) { // Bit 1 is the networkPolicy TLV, see lldpXMedPortConfigTLVsTxEnable MIB.
                    T_NG_PORT(TRACE_GRP_MED_TX, port_idx, "policy_idx:%d, optional_tlv:%d",
                              policy_idx, conf[port_idx].lldpmed_optional_tlvs_mask);

                    tlv_info_len +=  set_tlv_type_and_length_non_zero_len(buf + tlv_info_len, LLDP_TLV_ORG_TLV,
                                                                          lldpmed_update_tlv(buf + 2 + tlv_info_len, policy_idx, LLDPMED_TLV_SUBTYPE_NETWORK_POLICY));
                }

            }
        }
    }
    return tlv_info_len;
}
