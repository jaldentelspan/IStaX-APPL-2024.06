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

#include "eth_link_oam_serializer.hxx"

const vtss_enum_descriptor_t vtss_appl_eth_link_oam_control_txt[] = {
    {VTSS_APPL_ETH_LINK_OAM_CONTROL_DISABLE, "disabled"},
    {VTSS_APPL_ETH_LINK_OAM_CONTROL_ENABLE,  "enabled"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_eth_link_oam_mode_txt[] = {
    {VTSS_APPL_ETH_LINK_OAM_MODE_PASSIVE, "passive"},
    {VTSS_APPL_ETH_LINK_OAM_MODE_ACTIVE,  "active"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_eth_link_oam_discovery_state_txt[] = {
    {VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_FAULT,                "linkFault"},
    {VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_ACTIVE_SEND_LOCAL,    "activeSendLocal"},
    {VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_PASSIVE_WAIT,         "passiveWait"},
    {VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_SEND_LOCAL_REMOTE,    "sendLocalAndRemote"},
    {VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_SEND_LOCAL_REMOTE_OK, "sendLocalAndRemoteOk"},
    {VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_SEND_ANY,             "operational"},
    {VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_LAST,                 "unknown"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_eth_link_oam_pdu_control_txt[] = {
    {VTSS_APPL_ETH_LINK_OAM_PDU_CONTROL_RX_INFO, "receiveOnly"},
    {VTSS_APPL_ETH_LINK_OAM_PDU_CONTROL_LF_INFO, "linkFault"},
    {VTSS_APPL_ETH_LINK_OAM_PDU_CONTROL_INFO,    "infoExchange"},
    {VTSS_APPL_ETH_LINK_OAM_PDU_CONTROL_ANY,     "any"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_eth_link_oam_mux_state_txt[] = {
    {VTSS_APPL_ETH_LINK_OAM_MUX_FWD_STATE,     "forwarding"},
    {VTSS_APPL_ETH_LINK_OAM_MUX_DISCARD_STATE, "discarding"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_eth_link_oam_parser_state_txt[] = {
    {VTSS_APPL_ETH_LINK_OAM_PARSER_FWD_STATE,     "forwarding"},
    {VTSS_APPL_ETH_LINK_OAM_PARSER_LB_STATE,      "loopback"},
    {VTSS_APPL_ETH_LINK_OAM_PARSER_DISCARD_STATE, "discarding"},
    {0, 0},
};
// Dummy function because the "Write Only" OID is implemented with a
// TableReadWrite
mesa_rc eth_link_oam_stats_dummy_get(vtss_ifindex_t ifindex, BOOL *const clear) {
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}
// Wrapper for clear counters
// Set to TRUE to clear counters for the select ifindex interface
mesa_rc eth_link_oam_stats_clr_set(vtss_ifindex_t ifindex, const BOOL *const clear)
{
    if (clear && *clear) { // Make sure the Clear is not a NULL pointer
        if (!vtss_ifindex_is_port(ifindex)) {
            return VTSS_RC_ERROR;
        }
        (void)vtss_appl_eth_link_oam_counters_clear(ifindex);
    }
    return VTSS_RC_OK;
}
