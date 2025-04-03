/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "ptp_icli_show_functions.h"
#include "ptp.h"

#include "vtss_tod_api.h"
#include "misc_api.h"
#include "vtss_ptp_local_clock.h"

/***************************************************************************/
/*  Internal functions                                                     */
/***************************************************************************/


/*
 * Implementation of ICLI show functions
 */

static const char *cli_bool_disp(bool b)
{
    return (b ? "True" : "False");
}

static const char *cli_adj_method(vtss_appl_ptp_preferred_adj_t m)
{
    switch (m) {
        case VTSS_APPL_PTP_PREFERRED_ADJ_LTC: return "LTC";
        case VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE: return "Single";
        case VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT: return "Independent";
        case VTSS_APPL_PTP_PREFERRED_ADJ_COMMON: return "Common";
        case VTSS_APPL_PTP_PREFERRED_ADJ_AUTO: return "Auto";
        default: return "unknown";
    }
}

static const char *cli_one_pps_mode_disp(u8 m)
{
    switch (m) {
        case VTSS_APPL_PTP_ONE_PPS_DISABLE: return "Disable";
        case VTSS_APPL_PTP_ONE_PPS_OUTPUT: return "Output";
        default: return "unknown";
    }
}

static const char *cli_state_disp(u8 b)
{

    switch (b) {
        case VTSS_APPL_PTP_COMM_STATE_IDLE:
            return "IDLE";
        case VTSS_APPL_PTP_COMM_STATE_INIT:
            return "INIT";
        case VTSS_APPL_PTP_COMM_STATE_CONN:
            return "CONN";
        case VTSS_APPL_PTP_COMM_STATE_SELL:
            return "SELL";
        case VTSS_APPL_PTP_COMM_STATE_SYNC:
            return "SYNC";
        default:
            return "?";
    }
}

static const char *cli_protocol_disp(u8 p)
{
    switch (p) {
        case VTSS_APPL_PTP_PROTOCOL_ETHERNET:
            return "Ethernet";
        case VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED:
            return "EthernetMixed";
        case VTSS_APPL_PTP_PROTOCOL_IP4MULTI:
            return "IPv4Multi";
        case VTSS_APPL_PTP_PROTOCOL_IP4MIXED:
            return "IPv4Mixed";
        case VTSS_APPL_PTP_PROTOCOL_IP4UNI:
            return "IPv4Uni";
        case VTSS_APPL_PTP_PROTOCOL_OAM:
            return "Oam";
        case VTSS_APPL_PTP_PROTOCOL_ONE_PPS:
            return "1pps";
        case VTSS_APPL_PTP_PROTOCOL_IP6MIXED:
            return "IPv6Mixed";
        case VTSS_APPL_PTP_PROTOCOL_ANY:
            return "EthIp4Ip6-Combo";
        default:
            return "?";
    }
}

static const char *cli_srv_opt_disp(vtss_appl_ptp_srv_clock_option_t p)
{
    switch (p) {
        case VTSS_APPL_PTP_CLOCK_FREE:
            return "free";
        case VTSS_APPL_PTP_CLOCK_SYNCE:
            return "synce";
        default:
            return "?";
    }
}

static const char *cli_delaymechanism_disp(uchar d)
{
    switch (d) {
        case 1:
            return "e2e";
        case 2:
            return "p2p";
        case 3:
            return "cp2p";
        default:
            return "?\?\?";
    }
}

static const char *cli_dest_adr_type_disp(vtss_appl_ptp_dest_adr_type_t d)
{
    switch (d) {
        case VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT:
            return "deflt";
        case VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL:
            return "liLoc";
        default:
            return "?\?\?";
    }
}

static const char *cli_two_step_option_disp(vtss_appl_ptp_twostep_override_t d)
{
    switch (d) {
        case VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE:
            return "Clk Def.";
        case VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE:
            return "False";
        case VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE:
            return "True";
        default:
            return "?\?\?";
    }
}

static const char *cli_profile_disp(vtss_appl_ptp_profile_t d)
{
    switch (d) {
        case VTSS_APPL_PTP_PROFILE_NO_PROFILE:
            return "No profile";
        case VTSS_APPL_PTP_PROFILE_1588:
            return "ieee1588";
        case VTSS_APPL_PTP_PROFILE_G_8265_1:
            return "g8265.1";
        case VTSS_APPL_PTP_PROFILE_G_8275_1:
            return "g8275.1";
        case VTSS_APPL_PTP_PROFILE_G_8275_2:
            return "g8275.2";
        case VTSS_APPL_PTP_PROFILE_IEEE_802_1AS:
            return "802.1as";
        case VTSS_APPL_PTP_PROFILE_AED_802_1AS:
            return "802.1as-aed";
        default:
            return "?\?\?";
    }
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
static const char *cli_port_role_disp(vtss_appl_ptp_802_1as_port_role_t r)
{
    switch (r) {
        case VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT:
            return "Disabled";
        case VTSS_APPL_PTP_PORT_ROLE_MASTER_PORT:
            return "Master";
        case VTSS_APPL_PTP_PORT_ROLE_PASSIVE_PORT:
            return "Passive";
        case VTSS_APPL_PTP_PORT_ROLE_SLAVE_PORT:
            return "Slave";
        default:
            return "?\?\?";
    }
}
#endif //defined (VTSS_SW_OPTION_P802_1_AS)

void ptp_cli_table_header(const char *txt, vtss_ptp_cli_pr *pr)
{
    int i, j, len, count = 0;

    pr("%s\n", txt);
    while (*txt == ' ') {
        (void)pr(" ");
        txt++;
    }
    len = strlen(txt);
    for (i = 0; i < len; i++) {
        if (txt[i] == ' ') {
            count++;
        } else {
            for (j = 0; j < count; j++) {
                (void)pr("%c", count > 1 && (j >= (count - 2)) ? ' ' : '-');
            }
            (void)pr("-");
            count = 0;
        }
    }
    for (j = 0; j < count; j++) {
        (void)pr("%c", count > 1 && (j >= (count - 2)) ? ' ' : '-');
    }
    (void)pr("\n");
}

void ptp_show_clock_foreign_master_record_ds(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr)
{
    ptp_foreign_ds_t bs;
    char str2 [40];
    char str3 [40];
    i16 ix;
    if(first) {
        ptp_cli_table_header("Port  ForeignmasterIdentity         ForeignmasterClockQality     Pri1  Pri2  Lpri  Qualif  Best ", pr);
    }
    for (ix = 0; ix < DEFAULT_MAX_FOREIGN_RECORDS; ix++) {
        if (ptp_get_port_foreign_ds(&bs,uport, ix, inst)) {
            (void)pr("%-4d  %-27s%-4d  %-27s%-6d%-6d%-6d%-8s%-5s\n",
                     uport,
                     ClockIdentityToString(bs.foreignmasterIdentity.clockIdentity, str2),
                     bs.foreignmasterIdentity.portNumber,
                     ClockQualityToString(&bs.foreignmasterClockQuality, str3),
                     bs.foreignmasterPriority1,
                     bs.foreignmasterPriority2,
                     bs.foreignmasterLocalPriority,
                     cli_bool_disp(bs.qualified),
                     cli_bool_disp(bs.best));
        } else {
            continue;
        }
    }

}

void ptp_show_clock_port_ds(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_appl_ptp_status_port_ds_t port_status;
    vtss_ifindex_t ifindex;
    char str1[34];
    char str2[34];
    char str3[34];
    char str4[34];
    std::string hdr;
    int ti_width = ptp_cap_sub_nano_second() ? 21 : 17;

    (void) vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if (first) {
        hdr = "Port  Enabled  Stat  MDR  PeerMeanPathDel  ";
        if (ptp_cap_sub_nano_second()) {
            hdr += std::string(4, ' ');
        }
        hdr += "Anv  ATo  Syv  SyvErr  Delm  MPR  DelayAsymmetry   ";
        if (ptp_cap_sub_nano_second()) {
            hdr += std::string(4, ' ');
        }
        hdr += "IngressLatency   ";
        if (ptp_cap_sub_nano_second()) {
            hdr += std::string(4, ' ');
        }
        hdr += "EgressLatency    ";
        if (ptp_cap_sub_nano_second()) {
            hdr += std::string(4, ' ');
        }
        hdr += "Ver  Lpri  NoSlv  McAdr  Two-step  NoMstr";
        ptp_cli_table_header(hdr.c_str(), pr);
    }
    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) &&
        (vtss_appl_ptp_status_clocks_port_ds_get(inst, ifindex, &port_status) == VTSS_RC_OK))
    {
        (void)pr("%-6d%-9s%-6s%-5d%-*s%-5d%-5d%-5d%-8s%-6s%-5d%-*s%-*s%-*s%-5d%-6d%-7s%-7s%-10s%-6s\n",
                 port_status.portIdentity.portNumber,
                 port_cfg.enabled == 1 ? "True" : "False",
                 PortStateToString(port_status.portState),
                 port_status.logMinDelayReqInterval,
                 ti_width,
                 vtss_tod_TimeInterval_To_String(&port_status.peerMeanPathDelay, str1,','),
                 port_cfg.logAnnounceInterval,
                 port_cfg.announceReceiptTimeout, port_cfg.logSyncInterval,
                 port_status.syncIntervalError ? "Yes" : "No",
                 cli_delaymechanism_disp(port_cfg.delayMechanism),
                 port_cfg.logMinPdelayReqInterval,
                 ti_width,
                 vtss_tod_TimeInterval_To_String(&port_cfg.delayAsymmetry, str2,','),
                 ti_width,
                 vtss_tod_TimeInterval_To_String(&port_cfg.ingressLatency, str3,','),
                 ti_width,
                 vtss_tod_TimeInterval_To_String(&port_cfg.egressLatency, str4,','),
                 port_cfg.versionNumber,
                 port_cfg.localPriority,
                 port_cfg.masterOnly ? "True" : "False",
                 cli_dest_adr_type_disp(port_cfg.dest_adr_type),
                 cli_two_step_option_disp(port_cfg.twoStepOverride),
                 port_cfg.notMaster ? "True" : "False");
    }
}

void ptp_show_clock_port_state_ds(int inst, vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_appl_ptp_status_port_ds_t port_status;
    vtss_ptp_port_link_state_t ds;
    vtss_ifindex_t ifindex;

    (void) vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if (first) {
        ptp_cli_table_header("Port  Enabled  PTP-State  Internal  Link  Port-Timer  Vlan-forw  Phy-timestamper  Peer-delay", pr);
    }
    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) &&
        (vtss_appl_ptp_status_clocks_port_ds_get(inst, ifindex, &port_status) == VTSS_RC_OK) &&
        (ptp_get_port_link_state(inst, uport, &ds) == VTSS_RC_OK))
    {
        (void) pr("%4d  %-7s  %-9s  %-8s  %-4s  %-10s  %-9s  %-15s  %-10s\n",
                  port_status.portIdentity.portNumber,
                  port_cfg.enabled ? "TRUE" : "FALSE",
                  PortStateToString(port_status.portState),
                  port_cfg.portInternal ? "TRUE" : "FALSE",
                  ds.link_state ? "Up" : "Down",
                  ds.in_sync_state ? "In Sync" : "OutOfSync",
                  ds.forw_state ? "Forward" : "Discard",
                  ds.phy_timestamper ? "TRUE" : "FALSE",
                  port_status.peer_delay_ok ? "OK" : "FAIL");
    }
}

void ptp_show_clock_port_statistics(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr, bool clear)
{
    vtss_appl_ptp_status_port_statistics_t port_stati;
    vtss_ifindex_t ifindex;
    mesa_rc rc;
    (void) vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    ptp_cli_table_header("Port  Parameter                                 counter     ", pr);
    if (clear) {
        rc = vtss_appl_ptp_status_clocks_port_statistics_get_clear(inst, ifindex, &port_stati);
    } else {
        rc = vtss_appl_ptp_status_clocks_port_statistics_get(inst, ifindex, &port_stati);
    }
    if (rc == VTSS_RC_OK)
    {
        (void) pr("%4d  %-40s  %10d\n", uport, "rxSyncCount", port_stati.rxSyncCount);
        (void) pr("      %-40s  %10d\n", "rxFollowUpCount", port_stati.rxFollowUpCount);
        (void) pr("      %-40s  %10d\n", "rxPdelayRequestCount", port_stati.peer_d.rxPdelayRequestCount);
        (void) pr("      %-40s  %10d\n", "rxPdelayResponseCount", port_stati.peer_d.rxPdelayResponseCount);
        (void) pr("      %-40s  %10d\n", "rxPdelayResponseFollowUpCount", port_stati.peer_d.rxPdelayResponseFollowUpCount);
        (void) pr("      %-40s  %10d\n", "rxAnnounceCount", port_stati.rxAnnounceCount);
        (void) pr("      %-40s  %10d\n", "rxPTPPacketDiscardCount", port_stati.rxPTPPacketDiscardCount);
        (void) pr("      %-40s  %10d\n", "syncReceiptTimeoutCount", port_stati.syncReceiptTimeoutCount);
        (void) pr("      %-40s  %10d\n", "announceReceiptTimeoutCount", port_stati.announceReceiptTimeoutCount);
        (void) pr("      %-40s  %10d\n", "pdelayAllowedLostResponsesExceededCount", port_stati.peer_d.pdelayAllowedLostResponsesExceededCount);
        (void) pr("      %-40s  %10d\n", "txSyncCount", port_stati.txSyncCount);
        (void) pr("      %-40s  %10d\n", "txFollowUpCount", port_stati.txFollowUpCount);
        (void) pr("      %-40s  %10d\n", "txPdelayRequestCount", port_stati.peer_d.txPdelayRequestCount);
        (void) pr("      %-40s  %10d\n", "txPdelayResponseCount", port_stati.peer_d.txPdelayResponseCount);
        (void) pr("      %-40s  %10d\n", "txPdelayResponseFollowUpCount", port_stati.peer_d.txPdelayResponseFollowUpCount);
        (void) pr("      %-40s  %10d\n", "txDelayRequestCount", port_stati.txDelayRequestCount);
        (void) pr("      %-40s  %10d\n", "txDelayResponseCount", port_stati.txDelayResponseCount);
        (void) pr("      %-40s  %10d\n", "rxDelayRequestCount", port_stati.rxDelayRequestCount);
        (void) pr("      %-40s  %10d\n", "rxDelayResponseCount", port_stati.rxDelayResponseCount);
        (void) pr("      %-40s  %10d\n", "txAnnounceCount", port_stati.txAnnounceCount);
        if (clear) {
        (void) pr("\n counters cleared\n");
        }
    }
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
void ptp_show_clock_port_802_1as_cfg(int inst, vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_appl_ptp_status_port_ds_t port_status;
    vtss_ifindex_t ifindex;
    vtss_appl_ptp_clock_config_default_ds_t cfg;
    char str[40];
    int ti_width = ptp_cap_sub_nano_second() ? 18 : 14;

    (void) vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);
    if (vtss_appl_ptp_clock_config_default_ds_get(inst, &cfg) == VTSS_RC_OK) {
        if (cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {

            if (first) {
                std::string hdr = "Port  delay-thresh    ";
                if (ptp_cap_sub_nano_second()) {
                    hdr += std::string(4, ' ');
                }
                hdr += "sync-rx-to  Init-comp-ratio  Use-Mgt-ratio  Mgt-comp-ratio  Init-comp-del  Use-Mgt-del  Mgt-comp-del  allow-lost-resp  allow-faults ";

                (void)pr("\n802.1AS port data set:\n");
                ptp_cli_table_header(hdr.c_str(), pr);
            }
            if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) &&
                    (vtss_appl_ptp_status_clocks_port_ds_get(inst, ifindex, &port_status) == VTSS_RC_OK))
            {
                (void) pr("%-6d%-*s  %10d  %-15s  %-13s  %-14s  %-13s  %-11s  %-12s  %15d  %12d \n",
                          port_status.portIdentity.portNumber,
                          ti_width,
                          vtss_tod_TimeInterval_To_String(&port_cfg.c_802_1as.peer_d.meanLinkDelayThresh,str, ','),
                          port_cfg.c_802_1as.syncReceiptTimeout,
                          cli_bool_disp(port_cfg.c_802_1as.peer_d.initialComputeNeighborRateRatio),
                          cli_bool_disp(port_cfg.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio),
                          cli_bool_disp(port_cfg.c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio),
                          cli_bool_disp(port_cfg.c_802_1as.peer_d.initialComputeMeanLinkDelay),
                          cli_bool_disp(port_cfg.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay),
                          cli_bool_disp(port_cfg.c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay),
                          port_cfg.c_802_1as.peer_d.allowedLostResponses,
                          port_cfg.c_802_1as.peer_d.allowedFaults);
            }
        }
    } else {
    }
}

void ptp_show_clock_port_802_1as_status(int inst, vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_status_port_ds_t port_status;
    vtss_ifindex_t ifindex;
    vtss_appl_ptp_clock_config_default_ds_t cfg;
    char str[40];
    int ti_width = ptp_cap_sub_nano_second() ? 19 : 15;

    (void) vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);
    if (vtss_appl_ptp_clock_config_default_ds_get(inst, &cfg) == VTSS_RC_OK) {
        if (cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
            if (first) {
                std::string hdr = "Port  port-role  is-mes-del  as-cap  rate-ratio   cur-anv  cur-syv  sync-time-intrv  ";
                if (ptp_cap_sub_nano_second()) {
                    hdr += std::string(4, ' ');
                }
                hdr += "cur-MPR  AMTE   comp-ratio  comp-delay  version  minor-ver";
                (void)pr("\n802.1AS port status:\n");
                ptp_cli_table_header(hdr.c_str(), pr);
            }
            if (vtss_appl_ptp_status_clocks_port_ds_get(inst, ifindex, &port_status) == VTSS_RC_OK)
            {
                (void) pr("%4d  %-9s  %-10s  %-6s  %11d  %7d  %7d  %*s  %7d  %5s  %-10s  %-10s  %7d  %9d\n",
                          port_status.portIdentity.portNumber,
                          cli_port_role_disp(port_status.s_802_1as.portRole),
                          cli_bool_disp(port_status.s_802_1as.peer_d.isMeasuringDelay),
                          cli_bool_disp(port_status.s_802_1as.asCapable),
                          port_status.s_802_1as.peer_d.neighborRateRatio,
                          port_status.s_802_1as.currentLogAnnounceInterval,
                          port_status.s_802_1as.currentLogSyncInterval,
                          ti_width,
                          vtss_tod_TimeInterval_To_String(&port_status.s_802_1as.syncReceiptTimeInterval,str, ','),
                          port_status.s_802_1as.peer_d.currentLogPDelayReqInterval,
                          port_status.s_802_1as.acceptableMasterTableEnabled ? "True " : "False",
                          cli_bool_disp(port_status.s_802_1as.peer_d.currentComputeNeighborRateRatio),
                          cli_bool_disp(port_status.s_802_1as.peer_d.currentComputeMeanLinkDelay),
                          port_status.s_802_1as.peer_d.versionNumber,
                          port_status.s_802_1as.peer_d.minorVersionNumber);
            }
        } else if (cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            if (first) {
                std::string hdr = "Port  aed-port-role  is-mes-del  as-cap  rate-ratio   cur-syv  sync-time-intrv  ";
                if (ptp_cap_sub_nano_second()) {
                    hdr += std::string(4, ' ');
                }
                hdr += "cur-MPR   comp-ratio  comp-delay  version  minor-ver";
                (void)pr("\n802.1AS-AED port status:\n");
                ptp_cli_table_header(hdr.c_str(), pr);
            }
            if (vtss_appl_ptp_status_clocks_port_ds_get(inst, ifindex, &port_status) == VTSS_RC_OK)
            {
                (void) pr("%4d  %-13s  %-10s  %-6s  %11d  %7d  %*s  %8d  %-10s  %-10s  %7d  %9d\n",
                          port_status.portIdentity.portNumber,
                          cli_port_role_disp(port_status.s_802_1as.portRole),
                          cli_bool_disp(port_status.s_802_1as.peer_d.isMeasuringDelay),
                          cli_bool_disp(port_status.s_802_1as.asCapable),
                          port_status.s_802_1as.peer_d.neighborRateRatio,
                          port_status.s_802_1as.currentLogSyncInterval,
                          ti_width,
                          vtss_tod_TimeInterval_To_String(&port_status.s_802_1as.syncReceiptTimeInterval,str, ','),
                          port_status.s_802_1as.peer_d.currentLogPDelayReqInterval,
                          cli_bool_disp(port_status.s_802_1as.peer_d.currentComputeNeighborRateRatio),
                          cli_bool_disp(port_status.s_802_1as.peer_d.currentComputeMeanLinkDelay),
                          port_status.s_802_1as.peer_d.versionNumber,
                          port_status.s_802_1as.peer_d.minorVersionNumber);
            }
        }
    }
}
#endif //defined (VTSS_SW_OPTION_P802_1_AS)

void ptp_show_clock_virtual_port_state_ds(int inst, u32 virtual_port, bool first, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_status_port_ds_t port_status;
    vtss_appl_ptp_virtual_port_config_t  virtual_port_cfg;
    vtss_appl_ptp_clock_config_default_ds_t cfg;
    u32 pin;
    if (vtss_appl_ptp_clock_config_default_ds_get(inst, &cfg) == VTSS_RC_OK) {
        if (cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            if (first) {
                ptp_cli_table_header("VirtualPort  Enabled  PTP-State  Io-pin", pr);
            }
            if ((ptp_clock_config_virtual_port_config_get(inst, &virtual_port_cfg) == VTSS_RC_OK) &&
                    (vtss_appl_ptp_status_clocks_virtual_port_ds_get(inst, virtual_port, &port_status) == VTSS_RC_OK))
            {   if(virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO ||
                virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN ||
                virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT ||
                virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT){
                pin = virtual_port_cfg.output_pin;
                } else if (virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB ||
                        virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN){
                pin = virtual_port_cfg.input_pin;
                } else {
                pin = 99999;
                }

                (void) pr("%11d  %-7s  %-9s  %6d\n",
                        iport2uport(virtual_port),
                        virtual_port_cfg.enable ? "TRUE" : "FALSE",
                        PortStateToString(port_status.portState),
                        pin);
            }
        }
    }
}

void ptp_show_clock_wireless_ds(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr)
{
    bool mode;
    vtss_ptp_delay_cfg_t delay_cfg;
    if (first) {
        ptp_cli_table_header("Port  Wireless Mode  Base_delay(ns)  Incr_delay(ns) ", pr);
    }
    if (ptp_port_wireless_delay_mode_get(&mode, uport,inst) &&
            ptp_port_wireless_delay_get(&delay_cfg, uport, inst)) {
        (void)pr("%4u  %-13s  %10d.%03d  %10d.%03d\n",
                 uport, mode ? "Enabled" : "Disabled",
                 VTSS_INTERVAL_NS(delay_cfg.base_delay),VTSS_INTERVAL_PS(delay_cfg.base_delay),
                 VTSS_INTERVAL_NS(delay_cfg.incr_delay),VTSS_INTERVAL_PS(delay_cfg.incr_delay));
    }
}

void ptp_show_ext_clock_mode(vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_ext_clock_mode_t mode;
    /* show the local clock  */
    (void) vtss_appl_ptp_ext_clock_out_get(&mode);
    (void)pr("PTP External One PPS mode: %s, Clock output enabled: %s, frequency : %d,\n"
             "Preferred adj method     : %s, PPS clock domain : %d \n",
             cli_one_pps_mode_disp(mode.one_pps_mode), cli_bool_disp(mode.clock_out_enable), mode.freq,
             cli_adj_method(mode.adj_method), mode.clk_domain);
}

static const char *clock_adjustment_method_txt(int m)
{
    switch (m) {
        case 0:
            return "Internal Timer";
        case 1:
            return "PTP DPLL";
        case 2:
            return "DAC option";
        case 3:
            return "Software";
        case 4:
            return "Synce DPLL";
        default:
            return "Unknown";
    }
}
void ptp_show_local_clock(int inst, vtss_ptp_cli_pr *pr)
{
    u64 hw_time;
    mesa_timestamp_t t;
    char str [14];
    /* show the local clock  */
    vtss_local_clock_time_get(&t,inst, &hw_time);
    (void)pr("PTP Time (%d)    : %s %s\n", inst, misc_time2str(t.seconds), vtss_tod_ns2str(t.nanoseconds, str,','));
    (void)pr("Clock Adjustment method: %s\n", clock_adjustment_method_txt(vtss_ptp_adjustment_method(inst)));
}

void ptp_show_clock_default_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_status_default_ds_t status;
    vtss_appl_ptp_clock_config_default_ds_t cfg;
    char str1 [40];
    char str2 [40];

    ptp_cli_table_header("ClockId  HW-Domain  DeviceType  Profile     2StepFlag  Ports  vtss_appl_clock_identity", pr);

    if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &cfg) == VTSS_RC_OK) &&
        (vtss_appl_ptp_clock_status_default_ds_get(inst, &status) == VTSS_RC_OK))
    {
        if (cfg.deviceType == VTSS_APPL_PTP_DEVICE_NONE) {
            (void)pr("%-9d%-11d%-12s%-12s\n",
                     inst,
                     cfg.clock_domain,
                     DeviceTypeToString(cfg.deviceType),
                     cli_profile_disp(cfg.profile));
        } else {
            (void)pr("%-9d%-11d%-12s%-12s%-11s%-7d%-24s\n",
                     inst,
                     cfg.clock_domain,
                     DeviceTypeToString(cfg.deviceType),
                     cli_profile_disp(cfg.profile),
                     cli_bool_disp(cfg.twoStepFlag),
                     status.numberPorts,
                     ClockIdentityToString(status.clockIdentity, str1));
            (void)pr("\n");
            ptp_cli_table_header("Dom  vtss_appl_clock_quality         Pri1  Pri2  Lpri", pr);
            (void)pr("%-5d%-32s%-6d%-6d%-4d\n",
                     cfg.domainNumber, ClockQualityToString(&status.clockQuality, str2), cfg.priority1, cfg.priority2, cfg.localPriority);
            (void)pr("\n");
            ptp_cli_table_header("Protocol         One-Way    VID    PCP  DSCP  PathTraceEnable", pr);
            (void)pr("%-17s%-11s%-7d%-5d%-6d%-17s\n",
                     cli_protocol_disp(cfg.protocol), cli_bool_disp(cfg.oneWay),
                     cfg.configured_vid, cfg.configured_pcp, cfg.dscp,
                     cli_bool_disp(cfg.path_trace_enable));
#if defined(VTSS_SW_OPTION_MEP)
            if (fast_cap(MESA_CAP_TS_MISSING_PTP_ON_INTERNAL_PORTS)) {
                (void)pr("\n");
                ptp_cli_table_header("Mep Id    ", pr);
                (void)pr("%-10d\n", cfg.mep_instance);
            }
#endif
#if defined(VTSS_SW_OPTION_P802_1_AS)
            if (cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                (void)pr("\n");
                ptp_cli_table_header("gmCapable  sdoId", pr);
                (void)pr("%-9s  ", cli_bool_disp(status.s_802_1as.gmCapable));
                (void)pr("0x%03x\n", status.s_802_1as.sdoId);
            }
#endif
        }
    } else {
        (void)pr("%-11d%-8s%-12s\n", inst, "", DeviceTypeToString(VTSS_APPL_PTP_DEVICE_NONE));
    }
}


void ptp_show_clock_current_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_current_ds_t bs;
    vtss_appl_ptp_clock_config_default_ds_t cfg;
    char str1[60];
    char str2[40];

    ptp_cli_table_header("stpRm  OffsetFromMaster    MeanPathDelay       ", pr);
    if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &cfg) == VTSS_RC_OK) &&
        (vtss_appl_ptp_clock_status_current_ds_get(inst, &bs) == VTSS_RC_OK)) {
        (void)pr("%-7d%-20s%s\n",
                 bs.stepsRemoved, vtss_tod_TimeInterval_To_String(&bs.offsetFromMaster, str1,','),
                 vtss_tod_TimeInterval_To_String(&bs.meanPathDelay, str2,','));
#if defined (VTSS_SW_OPTION_P802_1_AS)
        if (cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            ptp_cli_table_header("lastGMPhaseChange                             lastGMFreqChange  gmTimeBaseIndicator  ", pr);
            (void)pr("%44s  %16f  %19d\n",
                     vtss_tod_ScaledNs_To_String(&bs.cur_802_1as.lastGMPhaseChange, str1,','),
                     bs.cur_802_1as.lastGMFreqChange,
                     bs.cur_802_1as.gmTimeBaseIndicator);
            ptp_cli_table_header("gmChangeCount  timeOfLastGMChangeEvent  timeOfLastGMPhaseChangeEvent  timeOfLastGMFreqChangeEvent  ", pr);
            (void)pr("%13u  %23u  %28u  %27u\n",
                     bs.cur_802_1as.gmChangeCount,
                     bs.cur_802_1as.timeOfLastGMChangeEvent,
                     bs.cur_802_1as.timeOfLastGMPhaseChangeEvent,
                     bs.cur_802_1as.timeOfLastGMFreqChangeEvent);
        }
#endif //(VTSS_SW_OPTION_P802_1_AS)
    }
}

void ptp_show_clock_parent_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_parent_ds_t bs;
    vtss_appl_ptp_clock_config_default_ds_t cfg;
    char str1 [40];
    char str2 [40];
    char str3 [40];
    ptp_cli_table_header("ParentPortIdentity      port  Pstat  Var  ChangeRate  ", pr);
    if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &cfg) == VTSS_RC_OK) &&
        (vtss_appl_ptp_clock_status_parent_ds_get(inst, &bs) == VTSS_RC_OK)) {
        (void)pr("%-24s%-6d%-7s%-5d%-12d\n",
                 ClockIdentityToString(bs.parentPortIdentity.clockIdentity, str1), bs.parentPortIdentity.portNumber,
                 cli_bool_disp(bs.parentStats),
                 bs.observedParentOffsetScaledLogVariance,
                 bs.observedParentClockPhaseChangeRate);
        (void)pr("\n");
        ptp_cli_table_header("GrandmasterIdentity      GrandmasterClockQuality    Pri1  Pri2", pr);
        (void)pr("%-25s%-27s%-6d%-6d\n",
                 ClockIdentityToString(bs.grandmasterIdentity, str2),
                 ClockQualityToString(&bs.grandmasterClockQuality, str3),
                 bs.grandmasterPriority1,
                 bs.grandmasterPriority2);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        if (cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            (void)pr("\n");
            ptp_cli_table_header("cumulativeRateRatio  ", pr);
            (void)pr("%19d\n",
                     bs.par_802_1as.cumulativeRateRatio);
        }
#endif //(VTSS_SW_OPTION_P802_1_AS)
    }
}

void ptp_show_clock_time_property_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_timeproperties_ds_t bs;

    ptp_cli_table_header("UtcOffset  Valid  leap59  leap61  TimeTrac  FreqTrac  ptpTimeScale  TimeSource", pr);
    if (vtss_appl_ptp_clock_status_timeproperties_ds_get(inst, &bs) == VTSS_RC_OK) {
        (void)pr("%-11d%-7s%-8s%-8s%-10s%-10s%-14s%-10d\n",
                 bs.currentUtcOffset,
                 cli_bool_disp(bs.currentUtcOffsetValid),
                 cli_bool_disp(bs.leap59),
                 cli_bool_disp(bs.leap61),
                 cli_bool_disp(bs.timeTraceable),
                 cli_bool_disp(bs.frequencyTraceable),
                 cli_bool_disp(bs.ptpTimescale),
                 bs.timeSource);
    }
}

void ptp_show_clock_filter_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_filter_config_t filter_params;

    /* show the filter parameters  */
    ptp_cli_table_header("DelayFilter  Period  Dist", pr);
    if (vtss_appl_ptp_clock_filter_parameters_get(inst, &filter_params) == VTSS_RC_OK) {
        (void)pr("%-13d%-8d%-4d\n",
                 filter_params.delay_filter,
                 filter_params.period,
                 filter_params.dist
                );
    }
}

void ptp_show_clock_servo_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    /* show the servo parameters  */
    ptp_cli_table_header("Display  P-enable  I-enable  D-enable  'P'constant  'I'constant  'D'constant  gain const ",pr);
    if (vtss_appl_ptp_clock_servo_parameters_get(inst, &default_servo) == VTSS_RC_OK) {
        (void)pr("%-9s%-10s%-10s%-10s%-13d%-13d%-13d%-13d\n",
                 cli_bool_disp(default_servo.display_stats),
                 cli_bool_disp(default_servo.p_reg),
                 cli_bool_disp(default_servo.i_reg),
                 cli_bool_disp(default_servo.d_reg),
                 default_servo.ap,
                 default_servo.ai,
                 default_servo.ad,
                 default_servo.gain);
    }
}

void ptp_show_clock_clk_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    /* show the servo parameters  */
    ptp_cli_table_header("Option  threshold  'P'constant", pr);
    if (vtss_appl_ptp_clock_servo_parameters_get(inst, &default_servo) == VTSS_RC_OK) {
        (void)pr("%-9s%-11d%-13d\n",
                cli_srv_opt_disp(default_servo.srv_option),
                default_servo.synce_threshold,
                default_servo.synce_ap);
    }
}

void ptp_show_clock_ho_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    vtss_ptp_servo_status_t status;
    /* show the holdover parameters  */
    ptp_cli_table_header("Holdover filter  Adj threshold (ppb)", pr);
    if (vtss_appl_ptp_clock_servo_parameters_get(inst, &default_servo) == VTSS_RC_OK) {
        (void)pr("%15d  %17d.%d\n",
                 default_servo.ho_filter,
                 (i32)default_servo.stable_adj_threshold/10,(i32)default_servo.stable_adj_threshold%10);
    }

    /* show the holdover status  */
    if (vtss_appl_ptp_clock_servo_status_get(inst, &status) == VTSS_RC_OK) {
        ptp_cli_table_header("Holdover Ok  Holdover offset (ppb)", pr);
        (void)pr("%-11s  %19d.%ld\n",
                 status.holdover_ok ? "TRUE" : "FALSE",
                 (i32)status.holdover_adj / 10, labs((i32)status.holdover_adj % 10));
    }
}

void ptp_show_clock_uni_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_unicast_slave_config_t uni_slave_cfg;
    vtss_appl_ptp_unicast_slave_table_t uni_slave_status;
    char buf1[20];
    uint ix;

    /* show the unicast parameters  */
    ptp_cli_table_header("index  duration  ip_address       grant  CommState", pr);
    ix = 0;
    while ((vtss_appl_ptp_clock_config_unicast_slave_config_get(inst, ix, &uni_slave_cfg) == VTSS_RC_OK) &&
           (vtss_appl_ptp_clock_status_unicast_slave_table_get(inst, ix++, &uni_slave_status) == VTSS_RC_OK))
    {
        (void)pr("%-7d%-10d%-17s%-7d%-9s\n",
                 ix-1,
                 uni_slave_cfg.duration,
                 misc_ipv4_txt(uni_slave_cfg.ip_addr, buf1),
                 uni_slave_status.log_msg_period,
                 cli_state_disp(uni_slave_status.comm_state));
    }
}

void ptp_show_clock_master_table_unicast_ds(uint inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_unicast_master_table_t uni_master_table;
    uint clock_next, slave_next;
    mesa_rc rc;
    char str1[20];
    char str2[30];    

    ptp_cli_table_header("ip_addr          mac_addr           port  Ann  Sync ", pr);
    rc = vtss_appl_ptp_clock_slave_itr(&inst, &clock_next, NULL, &slave_next);
    while ((rc == VTSS_RC_OK) && (clock_next == inst)) {    
        rc = vtss_appl_ptp_clock_status_unicast_master_table_get(inst, slave_next, &uni_master_table);
        if (rc == VTSS_RC_OK) {         
            if (uni_master_table.ann) {
                if (uni_master_table.sync) {
                    (void)pr("%-17s%-19s%-6d%-4d %-4d\n",
                             misc_ipv4_txt(slave_next, str1),
                             misc_mac_txt((const uchar *) uni_master_table.mac.addr, str2),
                             uni_master_table.port,
                             uni_master_table.ann_log_msg_period,
                             uni_master_table.log_msg_period);
                } else {
                    (void)pr("%-17s%-19s%-6d%-4d %-4s\n",
                             misc_ipv4_txt(slave_next, str1),
                             misc_mac_txt((const uchar *)uni_master_table.mac.addr, str2),
                             uni_master_table.port,
                             uni_master_table.ann_log_msg_period,
                             "N.A.");
                }
            }
            rc = vtss_appl_ptp_clock_slave_itr(&inst, &clock_next, &slave_next, &slave_next);            
        }
    }
}

void ptp_show_clock_slave_ds(int inst, vtss_ptp_cli_pr *pr, bool details)
{
    vtss_appl_ptp_clock_slave_ds_t ss;
    char str1 [40];

    ptp_cli_table_header("Slave port  Slave state    Holdover(ppb)  ", pr);
    if (vtss_appl_ptp_clock_status_slave_ds_get(inst, &ss) == VTSS_RC_OK) {
        (void)pr("%-10d  %-13s  %-13s\n",
                 ss.port_number,
                 vtss_ptp_slave_state_2_text(ss.slave_state),
                 vtss_ptp_ho_state_2_text(ss.holdover_stable, ss.holdover_adj, str1, details));
    }
}

static const char *log_mode_2_text(int log_mode) {
    switch (log_mode) {
        case 1:  return "log offset";
        case 2:  return "log forward";
        case 3:  return "log reverse";
        case 4:  return "log fwd and rev";
        default: return "log not active";
    }
}

void ptp_show_log_mode(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_ptp_logmode_t log_mode;

    ptp_cli_table_header("Debug mode       Active  KeepControl  Log time (sec)  Time left (sec)  ", pr);
    if (ptp_debug_mode_get(inst, &log_mode)) {
        if (log_mode.debug_mode) {
            (void)pr("%-15s  %-6s  %-11s  %14d  %15d\n",
                     log_mode_2_text(log_mode.debug_mode),
                     log_mode.file_open ? "Yes" : "No",
                     log_mode.keep_control ? "Yes" : "No",
                     log_mode.log_time,
                     log_mode.time_left);
        } else {
            (void)pr("%-15s  \n",
                     log_mode_2_text(log_mode.debug_mode));
        }
    } else {
        (void)pr("Could not get log mode\n");
    }
}

void ptp_show_slave_cfg(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_clock_slave_config_t slave_cfg;

    ptp_cli_table_header("Stable Offset  Offset Ok      Offset Fail", pr);
    if (vtss_appl_ptp_clock_slave_config_get(inst, &slave_cfg) == VTSS_RC_OK) {
        (void)pr("%-13u  %-13u  %-13u\n", slave_cfg.stable_offset, slave_cfg.offset_ok, slave_cfg.offset_fail);
    }
}

void ptp_show_clock_slave_table_unicast_ds(int inst, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_unicast_slave_table_t uni_slave_table;
    char str1[20];
    char str2[30];
    char str3[30];
    int ix = 0;

    ptp_cli_table_header("Index  IP-addr      State  MAC-addr           Port  Srcport clock id         Srcport port  Grant ", pr);
    while (vtss_appl_ptp_clock_status_unicast_slave_table_get(inst, ix, &uni_slave_table) == VTSS_RC_OK) {
        if (uni_slave_table.conf_master_ip) {
            if (uni_slave_table.comm_state >= VTSS_APPL_PTP_COMM_STATE_CONN) {
                (void)pr("%-7d%-13s%-7s%-19s%-6d%-25s%-14d%-7d\n",
                         ix,
                         misc_ipv4_txt(uni_slave_table.conf_master_ip, str1),
                         cli_state_disp(uni_slave_table.comm_state),
                         misc_mac_txt((const uchar *) uni_slave_table.master.mac.addr, str2),
                         uni_slave_table.port,
                         ClockIdentityToString(uni_slave_table.sourcePortIdentity.clockIdentity, str3),
                         uni_slave_table.sourcePortIdentity.portNumber,
                         uni_slave_table.log_msg_period);
            } else {
                (void)pr("%-7d%-13s%-7s\n",
                         ix,
                         misc_ipv4_txt(uni_slave_table.conf_master_ip, str1),
                         cli_state_disp(uni_slave_table.comm_state));
            }
        }
        ix++;
    }
}

void ptp_show_cmlds_port_status(vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_802_1as_cmlds_status_port_ds_t status;
    char str[40];
    int ti_width = ptp_cap_sub_nano_second() ? 17 : 13;
    
    if (first) {
        std::string hdr = "Port  enabled  is-mes-del  as-cap-acr-dom  mean-link-del  ";
        if (ptp_cap_sub_nano_second()) {
            hdr += std::string(4, ' ');
        }
        hdr += "rate-ratio  cur-MPR  comp-ratio  comp-delay  version  minor-ver";
        ptp_cli_table_header(hdr.c_str(), pr);
    }
    if (vtss_appl_ptp_cmlds_port_status_get(uport, &status) == VTSS_RC_OK) {
        (void)pr("%-4d  %-7s  %-10s  %-14s %-*s  %-10d  %-7d  %-10s  %-10s  %-7d  %-9d\n",
                status.portIdentity.portNumber,
                cli_bool_disp(status.cmldsLinkPortEnabled),
                cli_bool_disp(status.peer_d.isMeasuringDelay),
                cli_bool_disp(status.asCapableAcrossDomains),
                ti_width,
                vtss_tod_TimeInterval_To_String(&status.meanLinkDelay,str, ','),
                status.peer_d.neighborRateRatio,
                status.peer_d.currentLogPDelayReqInterval,
                cli_bool_disp(status.peer_d.currentComputeNeighborRateRatio),
                cli_bool_disp(status.peer_d.currentComputeMeanLinkDelay),
                status.peer_d.versionNumber,
                status.peer_d.minorVersionNumber);
    }

}

void ptp_show_cmlds_port_conf(vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr)
{
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf;
    char str1[34];
    char str2[34];
    int ti_width = ptp_cap_sub_nano_second() ? 18 : 14;

    if (first) {
        std::string hdr = "Port  Delay-Asym     ";
        if (ptp_cap_sub_nano_second()) {
            hdr += std::string(4, ' ');
        }
        hdr += "Dly-thresh     ";
        if (ptp_cap_sub_nano_second()) {
            hdr += std::string(4, ' ');
        }
        hdr += "Init-Pdel-Int  Use-Mgt-Pdel-Int  Mgt-Pdel-Int  Init-comp-ratio  Use-Mgt-ratio  Mgt-comp-ratio  Init-comp-del  Use-Mgt-del  Mgt-comp-del  allow-lost-resp  allow-faults";
        ptp_cli_table_header(hdr.c_str(), pr);
    }
    if (vtss_appl_ptp_cmlds_port_conf_get(uport, &conf) == VTSS_RC_OK) {
        (void)pr("%4d %-*s %-*s  %-13d  %-16s  %-12d  %-15s  %-13s  %-14s  %-13s  %-11s  %-12s  %-15d  %-12d \n",
                 uport,
                 ti_width,
                 vtss_tod_TimeInterval_To_String(&conf.delayAsymmetry, str1, ','),
                 ti_width,
                 vtss_tod_TimeInterval_To_String(&conf.peer_d.meanLinkDelayThresh, str2, ','),
                 conf.peer_d.initialLogPdelayReqInterval,
                 cli_bool_disp(conf.peer_d.useMgtSettableLogPdelayReqInterval),
                 conf.peer_d.mgtSettableLogPdelayReqInterval,
                 cli_bool_disp(conf.peer_d.initialComputeNeighborRateRatio),
                 cli_bool_disp(conf.peer_d.useMgtSettableComputeNeighborRateRatio),
                 cli_bool_disp(conf.peer_d.mgtSettableComputeNeighborRateRatio),
                 cli_bool_disp(conf.peer_d.initialComputeMeanLinkDelay),
                 cli_bool_disp(conf.peer_d.useMgtSettableComputeMeanLinkDelay),
                 cli_bool_disp(conf.peer_d.mgtSettableComputeMeanLinkDelay),
                 conf.peer_d.allowedLostResponses,
                 conf.peer_d.allowedFaults);
    }
}

void ptp_show_cmlds_port_statistics(vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr, bool clear)
{
    vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t statis;
    mesa_rc rc;

    if (first) {
        ptp_cli_table_header("Port  Parameter                                 counter     ", pr);
    }
    if (clear) {
        rc = vtss_appl_ptp_cmlds_port_statistics_get_clear(uport, &statis);
    } else {
        rc = vtss_appl_ptp_cmlds_port_statistics_get(uport, &statis);
    }
    if (rc == VTSS_RC_OK) {
        (void) pr("%4d  %-40s  %10d\n", uport, "rxPdelayRequestCount", statis.peer_d.rxPdelayRequestCount);
        (void) pr("      %-40s  %10d\n", "rxPdelayResponseCount", statis.peer_d.rxPdelayResponseCount);
        (void) pr("      %-40s  %10d\n", "rxPdelayResponseFollowUpCount", statis.peer_d.rxPdelayResponseFollowUpCount);
        (void) pr("      %-40s  %10d\n", "rxPTPPacketDiscardCount", statis.rxPTPPacketDiscardCount);
        (void) pr("      %-40s  %10d\n", "pdelayAllowedLostResponsesExceededCount", statis.peer_d.pdelayAllowedLostResponsesExceededCount);
        (void) pr("      %-40s  %10d\n", "txPdelayRequestCount", statis.peer_d.txPdelayRequestCount);
        (void) pr("      %-40s  %10d\n", "txPdelayResponseCount", statis.peer_d.txPdelayResponseCount);
        (void) pr("      %-40s  %10d\n", "txPdelayResponseFollowUpCount", statis.peer_d.txPdelayResponseFollowUpCount);
    }
    if (clear) {
        (void) pr("\n counters cleared\n");
    }
}

