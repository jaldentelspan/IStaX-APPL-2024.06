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

#include "web_api.h"
#include "port_api.h"
#include "board_if.h"
#include "vtss_api_if_api.h"
#include "msg_api.h"
#ifdef VTSS_SW_OPTION_PTP
#include "ptp_api.h"
#endif
#ifdef VTSS_SW_OPTION_SYNCE
#include "synce.h"
#endif
#include <vtss_module_id.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_PORT

/* Stock resources in sw_mgd_html/images/jack_.... */
#define PORT_ICON_WIDTH     32
#define PORT_ICON_HEIGHT    23
#define PORT_BLOCK_GAP      12
#define NON_ETH_PORT_NO     0

static void port_httpd_write(CYG_HTTPD_STATE *p, vtss_uport_no_t uport, int type, const char *state,
                             const char *speed, int xoff, int row, BOOL non_eth_port)
{
    int ct;
    const char *status;
    if (!non_eth_port) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%u/jack_%s_%s_%s.png/Port %u: %s/%u/%u/%u/%u/%d|",
                      uport,
                      type ? "sfp" : "copper",
                      state,
                      row && !type ? "top" : "bottom",
                      uport,
                      speed,
                      xoff,
                      PORT_ICON_HEIGHT * (2 - row),
                      PORT_ICON_WIDTH,
                      PORT_ICON_HEIGHT,
                      row ? -1 : 1);
    } else {
        if (!strcmp(state, "link") || !strcmp(state, "enabled")) {
            status = "Enabled";
        } else {
            status = "Disabled";
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "jack_%s_%s_%s.png/%s: %s/%u/%u/%u/%u/|",
                      type ? "sma" : "copper",
                      state,
                      "bottom",
                      speed,
                      status,
                      xoff,
                      type ? (PORT_ICON_HEIGHT * 2 + 8) : (PORT_ICON_HEIGHT * 2),
                      type ? (4 * PORT_ICON_WIDTH / 5) : PORT_ICON_WIDTH,
                      type ? (4 * PORT_ICON_HEIGHT / 5) : PORT_ICON_HEIGHT);
    }

    cyg_httpd_write_chunked(p->outbuffer, ct);
}

static void
non_eth_port_state(CYG_HTTPD_STATE *p)
{
    const char *name =            "<unknown>";
    const char *state_freq_in =   "<unknown>";
    const char *state_freq_out =  "<unknown>";
    const char *state_ptp_in =    "<unknown>";
    const char *state_ptp_out =   "<unknown>";
    const char *state_rs422_in =  "<unknown>";
    const char *state_rs422_out = "<unknown>";
    const char *state_bits =      "<unknown>";
    int  xoff, is_sma_connector, row = 0;

#if defined(VTSS_SW_OPTION_PTP) && defined(VTSS_SW_OPTION_SYNCE)
    vtss_ptp_rs422_conf_t          mode;
    vtss_appl_ptp_ext_clock_mode_t mode_pp;
    vtss_appl_synce_frequency_t    freq;

    if (synce_mgmt_station_clock_in_get(&freq) == SYNCE_RC_OK) {
        switch (freq) {
        case VTSS_APPL_SYNCE_STATION_CLK_DIS :
            state_freq_in = "disabled";
            break;
        case VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ :
        case VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ :
        case VTSS_APPL_SYNCE_STATION_CLK_10_MHZ :
        case VTSS_APPL_SYNCE_STATION_CLK_MAX :
            state_freq_in = "enabled";
        }
    }
    (void) vtss_appl_ptp_ext_clock_out_get(&mode_pp);
    switch (mode_pp.one_pps_mode) {
    case VTSS_APPL_PTP_ONE_PPS_DISABLE :
        state_ptp_in = "disabled";
        state_ptp_out = "disabled";
        break;

    case VTSS_APPL_PTP_ONE_PPS_OUTPUT :
        state_ptp_out = "enabled";
        state_ptp_in  = "disabled";
        break;

    case VTSS_APPL_PTP_ONE_PPS_INPUT :
        state_ptp_out = "disabled";
        state_ptp_in  = "enabled";
        break;

    case VTSS_APPL_PTP_ONE_PPS_OUTPUT_INPUT :
        state_ptp_out = "enabled";
        state_ptp_in  = "enabled";
        break;
    }

    if (synce_mgmt_station_clock_out_get(&freq) == SYNCE_RC_OK) {
        switch (freq) {
        case VTSS_APPL_SYNCE_STATION_CLK_DIS :
            state_freq_out = "disabled";
            break;

        case VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ :
        case VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ :
        case VTSS_APPL_SYNCE_STATION_CLK_10_MHZ :
        case VTSS_APPL_SYNCE_STATION_CLK_MAX :
            state_freq_out = "enabled";
            break;
        }
    }

    vtss_ext_clock_rs422_conf_get(&mode);
    switch (mode.mode) {
    case VTSS_PTP_RS422_DISABLE :
        state_rs422_in = "down";
        state_rs422_out = "down";
        break;

    case VTSS_PTP_RS422_MAIN_AUTO :
    case VTSS_PTP_RS422_MAIN_MAN :
    case VTSS_PTP_RS422_CALIB :
        state_rs422_in = "down";
        state_rs422_out = "link";
        break;

    case VTSS_PTP_RS422_SUB :
        state_rs422_in = "link";
        state_rs422_out = "down";
        break;
    }

#else

    state_freq_in   = "disabled";
    state_freq_out  = "disabled";
    state_ptp_in   = "disabled";
    state_ptp_out   = "disabled";
    state_rs422_in  = "down";
    state_rs422_out = "down";
#endif

    /* for SMA freq i/p connector */
    xoff = PORT_ICON_WIDTH;
    is_sma_connector = 1;
    name = "Frequency Input";
    port_httpd_write(p, NON_ETH_PORT_NO, is_sma_connector, state_freq_in, name, xoff, row, TRUE);
    /* for 1 PPS i/p and o/p */
    name = "1PPS Input";
    xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, is_sma_connector, state_ptp_in, name, xoff, row, TRUE);
    name = "1PPS Output";
    xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, is_sma_connector, state_ptp_out, name, xoff, row, TRUE);
    /* for SMA freq o/p connector */
    name = "Frequency Output";
    xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, is_sma_connector, state_freq_out, name, xoff, row, TRUE);
    /* for time interface i/p and o/p */
    is_sma_connector = 0;
    name = "Time Input";
    xoff += PORT_BLOCK_GAP + PORT_ICON_WIDTH;
    port_httpd_write(p, NON_ETH_PORT_NO, is_sma_connector, state_rs422_in, name, xoff, row, TRUE);
    name = "Time Output";
    xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, is_sma_connector, state_rs422_out, name, xoff, row, TRUE);
    /* for BITS interface ports*/
    state_bits = "down";
    name = "BITS Interface";
    xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, is_sma_connector, state_bits, name, xoff, row, TRUE);
}
/*
 * The (basic) portstate handler
 */

void stat_portstate_switch_serval(CYG_HTTPD_STATE *p, vtss_usid_t usid, vtss_isid_t isid)
{
    mesa_port_no_t          iport;
    vtss_uport_no_t         uport;
    vtss_appl_port_conf_t   conf;
    vtss_appl_port_status_t port_status;
    BOOL                    fiber, odd_top;
    const char              *state, *speed, *back;
    int                     ct, xoff_add, xoff = 0, xoff_sfp = 0, row, sfp, pcb106 = 0, has_npi_port = 1;

    /* Board type */
    switch (vtss_board_type()) {
    case VTSS_BOARD_SERVAL_PCB106_REF:
        back = "switch_serval.png";
        pcb106++;
        xoff = 8 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
        odd_top = TRUE;
        break;

    case VTSS_BOARD_OCELOT_REF:
    case VTSS_BOARD_OCELOT_PCB123_REF:
        back = "switch_small.png";
        odd_top = TRUE;
        break;

    default:
        back = "switch_small.png";
        odd_top = FALSE;
        break;
    }

    /* For Orbweaver board NPI port is not part of port count */
    if (vtss_board_type() == VTSS_BOARD_OCELOT_REF) {
        has_npi_port = 0;
    }
    /* Backround, SID, managed */
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|", usid, back);
    cyg_httpd_write_chunked(p->outbuffer, ct);

    /* Emit port icons */
    for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_COUNT); iport++) {
        uport = iport2uport(iport);
        vtss_ifindex_t ifindex;
        if (vtss_ifindex_from_port(isid, iport, &ifindex) != VTSS_RC_OK) {
            T_E("Could not get ifindex");
            break;
        };

        if (vtss_appl_port_conf_get(ifindex, &conf) != VTSS_RC_OK ||
            vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK) {
            break;
        }

        fiber = port_status.fiber;
        if (port_has_phy(iport) || (vtss_board_type() == VTSS_BOARD_OCELOT_PCB123_REF && (port_status.static_caps & MEBA_PORT_CAP_DUAL_COPPER))) {
            /* NPI port, copper port or combo port */
            state = (conf.admin.enable ? ((port_status.link && !fiber) ? "link" : "down") : "disabled");
        } else {
            /* SFP port */
            state = (conf.admin.enable ? (port_status.link ? "link" : "down") : "disabled");
        }

        speed = (conf.admin.enable ? (port_status.link ? port_speed_duplex_to_txt_from_status(port_status) : "Down") : "Disabled");

        sfp = 0;
        row = 0;
        xoff_add = PORT_ICON_WIDTH;
        if (has_npi_port && iport == (fast_cap(MEBA_CAP_BOARD_PORT_COUNT) - 1)) {
            /* NPI port */
            if (pcb106) {
                xoff = 7 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
            } else {
                xoff_add += PORT_BLOCK_GAP;
            }
        } else if (port_has_phy(iport) || (vtss_board_type() == VTSS_BOARD_OCELOT_PCB123_REF && (port_status.static_caps & MEBA_PORT_CAP_DUAL_COPPER))) {
            /* Copper port or combo port */
            xoff_sfp = PORT_BLOCK_GAP;
            if (odd_top && (iport & 1)) {
                /* Odd ports on are in the top row */
                row = 1;
                xoff_add = 0;
            }
        } else {
            /* SFP port */
            sfp = 1;
            if (pcb106 && (port_status.static_caps & MEBA_PORT_CAP_2_5G_FDX)) {
                T_D("uport %d's capability is  MEBA_PORT_CAP_2_5G_FDX", uport);
                /* SFP 2.5G ports are on the bottom row */
                xoff_add += ((iport & 1) ? 0 : PORT_BLOCK_GAP);
            } else if (odd_top && (iport & 1)) {
                /* Odd ports are in the top row */
                if (vtss_board_type() != VTSS_BOARD_OCELOT_REF) {
                    row = 1;
                    xoff_add = 0;
                }
            }

            if (xoff_sfp) {
                xoff_add += xoff_sfp;
                xoff_sfp = 0;
            }
        }

        xoff += xoff_add;
        port_httpd_write(p, uport, sfp, state, speed, xoff, row, FALSE);

        if (port_status.static_caps & MEBA_PORT_CAP_DUAL_SFP_DETECT) {
            /* Dual media fiber port */
            sfp = 1;
            state = (conf.admin.enable ? ((fiber && port_status.link) ? "link" : "down") : "disabled");
            if (vtss_board_type() == VTSS_BOARD_OCELOT_PCB123_REF) {
                xoff_sfp = (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
            } else {
                xoff_sfp = (2 * PORT_ICON_WIDTH + PORT_BLOCK_GAP);
            }

            port_httpd_write(p, uport, sfp, state, speed, xoff + xoff_sfp, row, FALSE);
        }
    }

    if (pcb106) {
        non_eth_port_state(p);
    }

    cyg_httpd_write_chunked(",", 1);
}

