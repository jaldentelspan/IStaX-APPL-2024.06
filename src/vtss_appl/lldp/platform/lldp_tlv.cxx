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

#include "lldp_sm.h"
#include "lldp_trace.h"
#include "vtss_common_os.h"
#include "lldp_tlv.h"
#include "lldp_private.h"
#include "misc_api.h"

#ifdef VTSS_SW_OPTION_LLDP_MED
#include "lldpmed_tx.h"
#endif

#include "lldp_api.h"

#ifdef VTSS_SW_OPTION_EEE
#include "eee_tx.h"
#endif

#ifdef VTSS_SW_OPTION_POE
#include "poe_api.h"
#endif

#include "fp_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

static lldp_u16_t append_chassis_id(lldp_u8_t *buf);
static lldp_u16_t append_port_id(lldp_u8_t *buf, lldp_port_t port);
static lldp_u16_t append_ttl(lldp_u8_t *buf, lldp_port_t port);
static lldp_u16_t append_end_of_pdu(void);
static lldp_16_t  append_port_descr(lldp_u8_t *buf, lldp_port_t uport);
static lldp_u16_t append_system_name(lldp_u8_t *buf);
static lldp_u16_t append_system_descr(lldp_u8_t *buf);
static lldp_u16_t append_system_capabilities (lldp_u8_t *buf);
#ifdef VTSS_SW_OPTION_POE
static lldp_u16_t append_poe(lldp_u8_t *buf, lldp_port_t port);
#endif
static lldp_u16_t append_mgmt_address(lldp_u8_t *buf, lldp_port_t port);
/* ************************************************************************ **
  * Functions
  * ************************************************************************ */
lldp_u16_t lldp_tlv_add(lldp_u8_t *buf, lldp_u16_t cur_len, lldp_tlv_t tlv, lldp_port_t port_idx)
{
    lldp_u16_t tlv_info_len = 0;

    switch (tlv) {
    case LLDP_TLV_BASIC_MGMT_CHASSIS_ID:
        T_R("Getting LLDP_TLV_BASIC_MGMT_CHASSIS_ID");
        tlv_info_len = append_chassis_id(buf + 2);
        break;

    case LLDP_TLV_BASIC_MGMT_PORT_ID:
        T_R("Getting LLDP_TLV_BASIC_MGMT_PORT_ID");
        tlv_info_len = append_port_id(buf + 2, port_idx);
        break;

    case LLDP_TLV_BASIC_MGMT_TTL:
        tlv_info_len = append_ttl(buf + 2, port_idx);
        break;

    case LLDP_TLV_BASIC_MGMT_END_OF_LLDPDU:
        tlv_info_len = append_end_of_pdu();
        break;

    case LLDP_TLV_BASIC_MGMT_PORT_DESCR:
        T_NG_PORT(TRACE_GRP_TX, port_idx, "%s", "Getting LLDP_TLV_BASIC_MGMT_PORT_DESCR");
        tlv_info_len = append_port_descr(buf + 2, iport2uport(port_idx));
        break;

    case LLDP_TLV_BASIC_MGMT_SYSTEM_NAME:
        T_R("Getting LLDP_TLV_BASIC_MGMT_SYSTEM_NAME");
        tlv_info_len = append_system_name(buf + 2);
        break;

    case LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR:
        T_R("Getting LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR");
        tlv_info_len = append_system_descr(buf + 2);
        break;

    case LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA:
        T_R("Getting LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA");
        tlv_info_len = append_system_capabilities(buf + 2);
        break;

    case LLDP_TLV_BASIC_MGMT_MGMT_ADDR:
        T_R("Getting LLDP_TLV_BASIC_MGMT_MGMT_ADDR");
        tlv_info_len = append_mgmt_address(buf + 2, port_idx);
        break;
    case LLDP_TLV_ORG_TLV:

#ifdef VTSS_SW_OPTION_LLDP_MED
        T_DG_PORT(TRACE_GRP_TX, port_idx, "%s", "Adding LLDP-MED TLV");
        tlv_info_len += lldpmed_tlv_add(buf + tlv_info_len, port_idx);
#endif

#ifdef VTSS_SW_OPTION_EEE
        T_DG_PORT(TRACE_GRP_TX, port_idx, "%s", "Adding EEE TLV");
        tlv_info_len += eee_tlv_add(buf + tlv_info_len, port_idx);
#endif

#ifdef VTSS_SW_OPTION_TSN
        if (lldp_is_frame_preemption_supported()) {
            T_DG_PORT(TRACE_GRP_FP_TX, port_idx, "%s", "Adding LLDP-FRAME PREEMPTION TLV");
            tlv_info_len += fp_tlv_add(buf + tlv_info_len, port_idx);
        }
#endif

#ifdef VTSS_SW_OPTION_LLDP_MED
        {
            CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> conf;
            (void) lldp_conf_get(&conf[0]); // Get current configuration
            if (conf[port_idx].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT) {  // Bit 3 is the extendedPSE TLV, see lldpXMedPortConfigTLVsTxEnable MIB.
#endif
#ifdef VTSS_SW_OPTION_POE
                if (poe_is_chip_found(port_idx, __FUNCTION__, __LINE__) == MEBA_POE_CHIPSET_FOUND) {
                    T_DG_PORT(TRACE_GRP_POE, port_idx, "tlv_info_len = %d,cur_len = %d", tlv_info_len, cur_len);
                    buf += tlv_info_len; // Point to last entry in the buffer
                    tlv_info_len += set_tlv_type_and_length_non_zero_len (buf, tlv, append_poe(buf + 2, port_idx)); // Append PoE TLV
                    T_DG_PORT(TRACE_GRP_POE, port_idx, "tlv_info_len = %d,cur_len = %d", tlv_info_len, cur_len);
                }
#endif // VTSS_SW_OPTION_POE
#ifdef VTSS_SW_OPTION_LLDP_MED
            }
        }
#endif

        return cur_len + tlv_info_len;

    default:
        T_D("Unhandled TLV Type %u", (unsigned)tlv);
        return cur_len;
    }

    set_tlv_type_and_length (buf, tlv, tlv_info_len);

    T_NG(TRACE_GRP_TX, "cur_len = %d, tlv_info_len = %d, tlv = %d", cur_len, tlv_info_len, tlv);
    /* add additional 2 octets for header */
    return cur_len + 2 + tlv_info_len;
}

lldp_u16_t lldp_tlv_add_zero_ttl(lldp_u8_t *buf, lldp_u16_t cur_len)
{
    buf[2] = 0;
    buf[3] = 0;
    set_tlv_type_and_length (buf, LLDP_TLV_BASIC_MGMT_TTL, 2);
    return cur_len + 4;
}

lldp_u32_t lldp_tlv_mgmt_addr_len (void)
{
    return 5;
}

lldp_u8_t lldp_tlv_get_local_port_id (lldp_port_t port, lldp_8_t *port_str)
{
    sprintf(port_str, "%u", port);
    T_D("port_str = %s, len = " VPRIz, &port_str[0], strlen(port_str));
    return strlen(port_str);
}

lldp_u16_t set_tlv_type_and_length_non_zero_len (lldp_u8_t *buf, lldp_tlv_t tlv_type, lldp_u16_t tlv_info_string_len)
{
    T_DG(TRACE_GRP_POE, "tlv_info_string_len =%d", tlv_info_string_len);
    if (tlv_info_string_len != 0) {
        set_tlv_type_and_length (buf, tlv_type, tlv_info_string_len);
        return tlv_info_string_len + 2;
    }
    return 0;
}

void set_tlv_type_and_length (lldp_u8_t *buf, lldp_tlv_t tlv_type, lldp_u16_t tlv_info_string_len)
{
    buf[0] = (0xfe & ((lldp_8_t)tlv_type << 1)) | (tlv_info_string_len >> 8);
    buf[1] = tlv_info_string_len & 0xFF;
    T_NG(TRACE_GRP_TX, "TLV type : 0x%X, 0x%X, tlv_type = 0x%X, tlv_info_string_len = 0x%X, buf_addr = %p", buf[0], buf[1], tlv_type, tlv_info_string_len, buf);
}

static lldp_u16_t append_chassis_id (lldp_u8_t *buf)
{
    vtss_common_macaddr_t mac_addr;

    /*
    ** we append MAC address, which gives us length MAC_ADDRESS + Chassis id Subtype, hence
    ** information string length = 7
    */
    buf[0] = lldp_tlv_get_chassis_id_subtype(); /* chassis ID subtype */

    mac_addr = lldp_os_get_primary_switch_mac();
    memcpy(&buf[1], mac_addr.macaddr, VTSS_COMMON_MACADDR_SIZE);
    return 7;
}

static lldp_u16_t append_port_id (lldp_u8_t *buf, lldp_port_t port)
{
    lldp_u8_t len;

    buf[0] = lldp_tlv_get_port_id_subtype(); /* Port ID subtype */
    len = lldp_tlv_get_local_port_id(port, (lldp_8_t *) &buf[1]);
    return 1 + len;
}

static lldp_u16_t append_ttl (lldp_u8_t *buf, lldp_port_t port)
{
    lldp_sm_t   *sm;
    sm = lldp_get_port_sm(port);
    if (sm == NULL) {
        T_E("sm is NULL, port:%d", port);
        return 2;
    }
    buf[0] = HIGH_BYTE(sm->tx.txTTL);
    buf[1] = LOW_BYTE(sm->tx.txTTL);

    return 2;
}

static lldp_u16_t append_end_of_pdu (void)
{
    return 0;
}


static lldp_16_t append_port_descr(lldp_u8_t *buf, lldp_port_t uport)
{
    T_DG_PORT(TRACE_GRP_TX, uport, "Entering append_port_descr, port:d", uport);
    lldp_os_get_if_descr(uport, (lldp_8_t *) buf, VTSS_APPL_MAX_PORT_DESCR_LENGTH);
    return strlen((lldp_8_t *) buf);
}

static lldp_u16_t append_system_name (lldp_u8_t *buf)
{
    lldp_tlv_get_system_name((lldp_8_t *) buf);
    return strlen((lldp_8_t *) buf);
}

static lldp_u16_t append_system_descr (lldp_u8_t *buf)
{
    lldp_tlv_get_system_descr((lldp_8_t *) buf);
    return strlen((lldp_8_t *) buf);
}

int lldp_tlv_get_system_capabilities (void)
{
    /*
    ** The Vitesse implementation of LLDP always (at least at the time of writing)
    ** runs on a bridge (that has bridging enabled)
    */
    return 4;
}

int lldp_tlv_get_system_capabilities_ena (void)
{
    /*
    ** The Vitesse implementation of LLDP always (at least at the time of writing)
    ** runs on a bridge (that has bridging enabled)
    */
    return 4;
}

#ifdef VTSS_SW_OPTION_POE
// Appending Power Over Ethernet TLV
static lldp_u16_t append_poe(lldp_u8_t *buf, lldp_port_t port_index)
{
    // Variable containing the power information.
    lldp_u8_t power_conf = 0x0;   //
    lldp_u8_t prio = VTSS_APPL_POE_PORT_POWER_PRIORITY_CRITICAL;

    // We MUST call poe_new_pd_detected_get with argument TRUE in order to clear the status.
    // Required to stop sending fast start (lldp_poe_fast_start)
    poe_new_pd_detected_get(port_index, TRUE);

    // Get the PoE configuration
    poe_conf_t poe_local_conf;
    poe_config_get(&poe_local_conf);
    if (poe_local_conf.poe_mode[port_index] == VTSS_APPL_POE_MODE_DISABLED) {
        return 0; // This port has PoE disabled so we don't transmit any LLDP PoE information
    }

    switch (poe_local_conf.priority[port_index]) {
    case VTSS_APPL_POE_PORT_POWER_PRIORITY_LOW:
        prio = 0x3;
        break;
    case VTSS_APPL_POE_PORT_POWER_PRIORITY_HIGH:
        prio = 0x2;
        break;
    case VTSS_APPL_POE_PORT_POWER_PRIORITY_CRITICAL:
        prio = 0x1;
        break;
    default:
        break;
    }

    // Note 2, for table 10.2.1.1, TIA1057
    lldp_u8_t power_source = (lldp_u8_t) poe_mgmt_get_power_source();
    power_conf |= power_source << 4;
    power_conf |= prio;

    // Get the request power from any PD on the port
    lldp_u16_t requested_power_dw = lldp_remote_get_requested_power(port_index);
    poe_status_t poe_status;
    poe_mgmt_get_status(&poe_status); // Get the status fields.

    lldp_u8_t ieee_draft_version = lldp_remote_get_ieee_draft_version(port_index);
    T_DG_PORT(TRACE_GRP_POE, port_index, "ieee_draft_version = %u", ieee_draft_version);

    lldp_u8_t poe_tlv_length = lldp_remote_get_ieee_poe_tlv_length(port_index);
    T_DG_PORT(TRACE_GRP_POE, port_index, "ieee_poe tlv length = %u", poe_tlv_length);

    if (ieee_draft_version == 0) {
        // TIA OUI = 00-12-BB, See section 10.2.5, figure 12 in TIA-1057
        buf[0] = 0x00;
        buf[1] = 0x12;
        buf[2] = 0xBB;
        // Extended Power via MDI Subtype = 04, See section 10.2.5, figure 12 in TIA-1057
        buf[3] = (lldp_u8_t) 0x4;
        // Set the power source
        buf[4] = power_conf;
        // Power Value
        buf[5] = (lldp_u8_t) ((poe_status.port_status[port_index].power_reserved_dw >> 8) & 0xFF);
        buf[6] = (lldp_u8_t) poe_status.port_status[port_index].power_reserved_dw & 0xFF;
        T_NG_PORT(TRACE_GRP_POE, port_index, "TIA allocated = %u", poe_status.port_status[port_index].power_reserved_dw);
        T_DG_PORT(TRACE_GRP_POE, port_index, "Appending PoE = 0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
        return 7; // Extended Power Via MDI String Length is always 7, See section 10.2.5, figure 12 in TIA-1057
    }

    // OUI - See table 33-18 in IEEE 803.2at/D1
    buf[0] = 0x00;
    buf[1] = 0x12;
    buf[2] = 0x0F;

    if (ieee_draft_version == 1 || ieee_draft_version == 3) {
        //
        //  figure 33-26 in IEEE 803.2at/D3 or table 33-18, IEEE 802.1at/D1
        //
        buf[3] = 0x5; // Subtype
        T_NG_PORT(TRACE_GRP_POE, port_index, "prio = %d, power_conf 2 =  0x%X", poe_local_conf.priority[port_index], power_conf);
        buf[4] = power_conf;
        // Power Value
        buf[5] = (lldp_u8_t) (requested_power_dw >> 8) & 0xFF; // High part of the power
        buf[6] = (lldp_u8_t) requested_power_dw & 0xFF; // Low part of the power

        T_NG_PORT(TRACE_GRP_POE, port_index, "requested_power_dw = %u", requested_power_dw);

        if (ieee_draft_version == 1) {
            buf[7] = 01; // Has to be updated
            return 8; // Length always 8 - See table 33-18 in IEEE 803.2at/D1
        } else {
            // We defaults to version IEEE802.3at/D3
            buf[7] = power_conf;
            buf[8] = (lldp_u8_t) (poe_status.port_status[port_index].power_reserved_dw >> 8) & 0xFF; // High part of the power
            buf[9] = (lldp_u8_t) poe_status.port_status[port_index].power_reserved_dw & 0xFF; // Low part of the power
            buf[10] = 01; // Acknowledge - See Table 33-26 in IEEE 803.2at/D3
            return 11; // Length always 11 - See figure 33-26 in IEEE 803.2at/D3
        }
    }

    u16 power_allocated_dw = 0;
    poe_entry_t board_poe_conf = {};
    board_poe_conf = poe_hw_config_get(port_index, &board_poe_conf);
    vtss_appl_lldp_admin_state_t admin_status;
    admin_status = lldp_os_get_admin_status(port_index);
    poe_pse_data_t pse_data;
    poe_pse_data_get(port_index, &pse_data);

    if (ieee_draft_version == 2 || ieee_draft_version == 4) {
        buf[3] = (lldp_u8_t) 0x2; // Subtype, Figure 79-2, IEEE 803.2at/D4
        // MDI power support, Figure 79-2, IEEE 803.2at/D4
        buf[4] = (lldp_u8_t) 0x0;
        buf[4] |= 1; // We are always a PSE - Table G-3, IEEE 802.1AB-2005
        buf[4] |= board_poe_conf.available << 1; // Table G-3, IEEE 802.1AB-2005
        if (admin_status == VTSS_APPL_LLDP_ENABLED_RX_TX || admin_status == VTSS_APPL_LLDP_ENABLED_TX_ONLY) {
            buf[4] |= 1 << 2; // Table G-3, IEEE 802.1AB-2005
        }
        buf[4] |= board_poe_conf.pse_pairs_control_ability << 3; // Table G-3, IEEE 802.1AB-2005
        buf[5] = pse_data.pse_power_pair;
        buf[6] = pse_data.power_class;
        buf[7] = pse_data.pse_power_type;
        requested_power_dw = pse_data.pd_requested_power_dw;
        power_allocated_dw = pse_data.pse_allocated_power_dw;
        // PD requested power, Figure 79-2, IEEE 803.2at/D4
        buf[8] = (lldp_u8_t) ((requested_power_dw >> 8) & 0xFF);
        buf[9] = (lldp_u8_t) requested_power_dw & 0xFF;
        // PSE allocated power value, Figure 79-2, IEEE 803.2at/D4
        buf[10] = (lldp_u8_t) ((power_allocated_dw >> 8) & 0xFF);
        buf[11] = (lldp_u8_t) power_allocated_dw & 0xFF;
        return 12; // Length always 12 for this TLV. Figure 79-2, IEEE 803.2at/D4
    }


    if (ieee_draft_version == 5) { // we assigned version 5 to BT standard with Type 3 and Type 4 extension
        // This IEEE 802.3bt
        buf[3] = (lldp_u8_t) 0x2; // Subtype, Figure 79-2, IEEE 803.2at/D4
        // MDI power support, Figure 79-2, IEEE 803.2at/D4
        buf[4] = (lldp_u8_t) 0x0;
        buf[4] |= 1; // We are always a PSE - Table G-3, IEEE 802.1AB-2005
        buf[4] |= board_poe_conf.available << 1; // Table G-3, IEEE 802.1AB-2005
        if (admin_status == VTSS_APPL_LLDP_ENABLED_RX_TX || admin_status == VTSS_APPL_LLDP_ENABLED_TX_ONLY) {
            buf[4] |= 1 << 2; // Table G-3, IEEE 802.1AB-2005
        }
        buf[4] |= board_poe_conf.pse_pairs_control_ability << 3; // Table G-3, IEEE 802.1AB-2005
        buf[5] = pse_data.pse_power_pair;
        buf[6] = pse_data.power_class;
        if (poe_status.port_status[port_index].pd_status != VTSS_APPL_POE_PD_ON) {
            // If PSE is NOT delivering power, only the Basic fields shall be sent
            return 7;
        }

        buf[7] = pse_data.pse_power_type;
        requested_power_dw = pse_data.pd_requested_power_dw;
        power_allocated_dw = pse_data.pse_allocated_power_dw;

        // DSPD and BT packet entry
        if ((poe_status.port_status[port_index].pd_type_sspd_dspd == 2) && ( poe_tlv_length == 29 )) {
            requested_power_dw = 0;
            power_allocated_dw = 0;
        }

        buf[8] = (lldp_u8_t) ((requested_power_dw >> 8) & 0xFF);
        buf[9] = (lldp_u8_t) requested_power_dw & 0xFF;
        buf[10] = (lldp_u8_t) ((power_allocated_dw >> 8) & 0xFF);
        buf[11] = (lldp_u8_t) power_allocated_dw & 0xFF;

        T_NG_PORT(TRACE_GRP_POE, port_index, "poe_tlv_length = %d", poe_tlv_length);

        // board not support the BT standard or not BT standard packet
        if ((!fast_cap(MEBA_CAP_POE_BT)) || (poe_tlv_length != 29)) {
            // This board does not support the BT standard, so Type 3 and Type 4 extension fields should not be filled out
            return 12;
        }

        uint16_t requested_power_mode_a_dw    = pse_data.requested_power_mode_a_dw;
        uint16_t requested_power_mode_b_dw    = pse_data.requested_power_mode_b_dw;
        uint16_t pse_alloc_power_alt_a_dw     = pse_data.pse_alloc_power_alt_a_dw;
        uint16_t pse_alloc_power_alt_b_dw     = pse_data.pse_alloc_power_alt_b_dw;
        uint16_t pse_maximum_avail_power_dw   = pse_data.pse_maximum_avail_power_dw;
        uint16_t power_status                 = pse_data.power_status; // pse_data.power_status fills bits [11..10] and [15..14]
        uint8_t  system_setup                 = pse_data.system_setup; // This value is specificed in Table 79-6f IEEE 803.2bt/D3.7  Type 3 PSE = 0. Type 4 PSE = 2.
        uint8_t  auto_class                   = pse_data.auto_class;

        // not DSPD packet so lets clear alt A and alt B fields
        if (poe_status.port_status[port_index].pd_type_sspd_dspd != 2) { // != DSPD
            requested_power_mode_a_dw    = 0;
            requested_power_mode_b_dw    = 0;
            pse_alloc_power_alt_a_dw     = 0;
            pse_alloc_power_alt_b_dw     = 0;
        }

        // For a PSE, power down request = 0 and power down time = 0
        uint32_t  power_down = 0;

        buf[12] = (lldp_u8_t) ((requested_power_mode_a_dw >> 8) & 0xFF);
        buf[13] = (lldp_u8_t) requested_power_mode_a_dw  & 0xFF;
        buf[14] = (lldp_u8_t) ((requested_power_mode_b_dw >> 8) & 0xFF);
        buf[15] = (lldp_u8_t) requested_power_mode_b_dw  & 0xFF;
        buf[16] = (lldp_u8_t) ((pse_alloc_power_alt_a_dw >> 8) & 0xFF);
        buf[17] = (lldp_u8_t) pse_alloc_power_alt_a_dw  & 0xFF;
        buf[18] = (lldp_u8_t) ((pse_alloc_power_alt_b_dw >> 8) & 0xFF);
        buf[19] = (lldp_u8_t) pse_alloc_power_alt_b_dw  & 0xFF;
        buf[20] = (lldp_u8_t) ((power_status  >> 8) & 0xFF);
        buf[21] = (lldp_u8_t) power_status  & 0xFF;
        buf[22] = (lldp_u8_t) system_setup;
        buf[23] = (lldp_u8_t) ((pse_maximum_avail_power_dw  >> 8) & 0xFF);
        buf[24] = (lldp_u8_t) pse_maximum_avail_power_dw & 0xFF;
        buf[25] = (lldp_u8_t) auto_class;
        buf[26] = (lldp_u8_t) ((power_down >> 16) & 0xFF);
        buf[27] = (lldp_u8_t) ((power_down >> 8) & 0xFF);
        buf[28] = (lldp_u8_t) power_down & 0xFF;

        return 29; // Length always 29 for this TLV. Figure 79-3, IEEE 803.2bt/D3.7
    }
    // We should not get here
    return 0;
}
#endif // VTSS_SW_OPTION_POE

static lldp_u16_t append_system_capabilities (lldp_u8_t *buf)
{
    int capa = lldp_tlv_get_system_capabilities();
    int capa_ena = lldp_tlv_get_system_capabilities_ena();

    buf[0] = (lldp_8_t) (capa >> 8);
    buf[1] = (lldp_8_t) capa;
    buf[2] = (lldp_8_t)(capa_ena >> 8);
    buf[3] = (lldp_8_t) capa_ena;
    T_DG(TRACE_GRP_TX, "Appending system capabilities = %c %c", buf[0], buf[1]);
    T_DG(TRACE_GRP_TX, "Appending system capabilities ena = %c %c", buf[2], buf[3]);

    return 4;
}


char lldp_tlv_get_mgmt_addr_subtype (void)
{
    /* management address subtype */
    return 1; /* IPv4 */
}

char lldp_tlv_get_mgmt_if_num_subtype (void)
{
    /* Interface Numbering subtype */
    return 2; /* ifIndex */
}

char lldp_tlv_get_mgmt_oid (void)
{
    return 0;
}

static lldp_u16_t append_mgmt_address (lldp_u8_t *buf, lldp_port_t port)
{
    u8 i = 0;
    lldp_u32_t mgmt_if_index = iport2uport(port);
    /* we receive a port parameter even though we don't care about it here
    ** (more exotic future implementations might have management addresses
    ** per-vlan, so the port is included to support this in some sense.
    */
    /*lint --e{438} */
    port = port;

    // From IEEE 802.1AB-2005, section 9.5.9.4
    if (lldp_ip_addr_get(port) == 0) { // Bullet b) - If no management address is available,
        // the return address should be the MAC address for the station or port.

        /* management address length = length(subtype + MAC address) */
        buf[i++] = 1 + VTSS_COMMON_MACADDR_SIZE;

        /* management address subtype - There is no good (8 bit) sub-type for the MAC address defined at
           https://www.iana.org/assignments/ianaaddressfamilynumbers-mib/ianaaddressfamilynumbers-mib
           so we use the others (0)*/
        buf[i++] = 0;

        vtss_common_macaddr_t mac_addr = lldp_os_get_primary_switch_mac();
        buf[i++] = mac_addr.macaddr[0];
        buf[i++] = mac_addr.macaddr[1];
        buf[i++] = mac_addr.macaddr[2];
        buf[i++] = mac_addr.macaddr[3];
        buf[i++] = mac_addr.macaddr[4];
        buf[i++] = mac_addr.macaddr[5];
    } else {
        /* management address length = length(subtype + IPv4 address) */
        buf[i++] = 5;

        /* management address subtype */
        buf[i++] = lldp_tlv_get_mgmt_addr_subtype();

        /* IPv4 Address */
        lldp_os_get_ip_address(&buf[i], port);
        i = i + 4;
    }

    /* Interface Numbering subtype */
    buf[i++] = lldp_tlv_get_mgmt_if_num_subtype();

    /* Interface number */
    buf[i++]  = (mgmt_if_index >> 24) & 0xFF;
    buf[i++]  = (mgmt_if_index >> 16) & 0xFF;
    buf[i++]  = (mgmt_if_index >>  8) & 0xFF;
    buf[i++] = (mgmt_if_index >>  0) & 0xFF;

    /* OID Length */
    buf[i++] = 0;

    /* if this function changes, make sure to update the lldp_tlv_mgmt_addr_len()
    ** function with the correct value: (from the MIB definition)
    ** "The total length of the management address subtype and the
    ** management address fields in LLDPDUs transmitted by the
    ** local LLDP agent."
    */
    return i;
}
#ifdef __cplusplus
}  // extern C
#endif
