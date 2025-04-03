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

#include "web_api.h"
#include "port_api.h"
#ifdef VTSS_SW_OPTION_PTP
#include "ptp_api.h"
#endif
#ifdef VTSS_SW_OPTION_SYNCE
#include "synce.h"
#endif
#include <vtss_module_id.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_PORT

/* Stock resources in sw_mgd_html/images/jack_.... */
#define PORT_ICON_WIDTH  32
#define PORT_ICON_HEIGHT 23
#define PORT_BLOCK_GAP   12
#define NON_ETH_PORT_NO  0

#define BOARD_TYPE_SPARX5_PCB134 134
#define BOARD_TYPE_SPARX5_PCB135 135

static void port_httpd_write(CYG_HTTPD_STATE *p,
                             vtss_uport_no_t uport,
                             const char *type,
                             const char *state,
                             const char *placement,
                             const char *name,
                             int xoff,
                             int row)
{
    int ct;
    const char *status;
    if (uport) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%u/jack_%s_%s_%s.png/Port %u: %s/%u/%u/%u/%u/%d|",
                      uport,
                      type,
                      state,
                      placement,
                      uport,
                      name,
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
                      "jack_%s_%s_%s.png/%s: %s/%u/%u/%u/%u/%s/%s/%d|",
                      type,
                      state,
                      placement,
                      name,
                      status,
                      xoff,
                      !strcmp(type, "sma") ? (PORT_ICON_HEIGHT * 2 + 8) : (PORT_ICON_HEIGHT * 2),
                      !strcmp(type, "sma") ? (4 * PORT_ICON_WIDTH / 5) : PORT_ICON_WIDTH,
                      !strcmp(type, "sma") ? (4 * PORT_ICON_HEIGHT / 5) : PORT_ICON_HEIGHT,
                      name,
                      "portlabel",
                      row ? 1 : -2);
    }

    cyg_httpd_write_chunked(p->outbuffer, ct);
}

static void sma_port_state(CYG_HTTPD_STATE *p, int *xoff)
{
    const char *name =            "<unknown>";
    const char *state_freq_in =   "<unknown>";
    const char *state_freq_out =  "<unknown>";
    const char *state_ptp_in =    "<unknown>";
    const char *state_ptp_out =   "<unknown>";
    const char *type = "sma";
    const char *placement = "bottom";
    int        row = 0;

#if defined(VTSS_SW_OPTION_PTP) && defined(VTSS_SW_OPTION_SYNCE)
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

    vtss_appl_ptp_ext_clock_out_get(&mode_pp);
    switch (mode_pp.one_pps_mode) {
    case VTSS_APPL_PTP_ONE_PPS_DISABLE :
        state_ptp_in = "disabled";
        state_ptp_out = "disabled";
        break;
    case VTSS_APPL_PTP_ONE_PPS_OUTPUT :
        state_ptp_out = "enabled";
        state_ptp_in  = "disabled";
        break;
    case VTSS_APPL_PTP_ONE_PPS_OUTPUT_INPUT :
        state_ptp_out = "enabled";
        state_ptp_in  = "enabled";
        break;
    case VTSS_APPL_PTP_ONE_PPS_INPUT :
        state_ptp_out = "disabled";
        state_ptp_in  = "enabled";
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
        }
    }
#else
    state_freq_in   = "disabled";
    state_freq_out  = "disabled";
    state_ptp_in   = "disabled";
    state_ptp_out   = "disabled";
#endif

    /* for SMA freq i/p connector */
    name = "Freq. Input";
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_freq_in, placement, name, *xoff, row);
    /* for 1 PPS i/p and o/p */
    name = "1PPS Input";
    *xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_ptp_in, placement, name, *xoff, row);
    name = "1PPS Output";
    *xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_ptp_out, placement, name, *xoff, row);
    /* for SMA freq o/p connector */
    name = "Freq. Output";
    *xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_freq_out, placement, name, *xoff, row);
    *xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
}

/*
 * The (basic) portstate handler
 */

void stat_portstate_switch_lan966x(CYG_HTTPD_STATE *p)
{
    mesa_port_no_t          iport, num_of_back_ports;
    vtss_uport_no_t         uport;
    vtss_appl_port_conf_t   conf;
    vtss_appl_port_status_t port_status;
    const char              *background = "switch_small.png", *placement, *type, *state, *speed;
    int                     ct, xoff_add, xoff = 0, row, block_size = 12;

    // Target
    mesa_target_type_t chip_id = vtss_api_chipid();
    T_D("Chip ID %d (0x%x)", chip_id, chip_id);

    // Board type
    uint32_t board_type = vtss_board_type();
    T_D("Board Type %u (0x%x)", board_type, board_type);

    switch (board_type) {
    case VTSS_BOARD_LAN9668_SUNRISE_REF:
    case VTSS_BOARD_LAN9668_ADARO_REF:
    case VTSS_BOARD_LAN9668_SVB_REF:
    case VTSS_BOARD_LAN9668_8PORT_REF:
    case VTSS_BOARD_LAN9668_ENDNODE_REF:
    case VTSS_BOARD_LAN9668_ENDNODE_CARRIER_REF:
    case VTSS_BOARD_LAN9668_EDS2_REF:
        num_of_back_ports = 0;
        break;

    default:
        T_E("Unsupported board (%d)", board_type);
        return;
    }

    /* Backround, SID, managed */
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|", 1, background);
    cyg_httpd_write_chunked(p->outbuffer, ct);


    // TODO: Below is temporary front layout. To be changed when real design is available.
    // Front panel
    // Ethernet ports
    for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_COUNT) - num_of_back_ports; iport++) {
        uport = iport2uport(iport);
        vtss_ifindex_t ifindex;
        if (vtss_ifindex_from_port(VTSS_ISID_START, iport, &ifindex) != VTSS_RC_OK) {
            T_E("Could not get ifindex for uport %d", uport);
            return;
        }

        if (vtss_appl_port_conf_get(ifindex, &conf) < 0 ||
            vtss_appl_port_status_get(ifindex, &port_status) < 0) {
            T_E("Could not get status for uport %d", uport);
            return;
        }

        state = conf.admin.enable ? (port_status.link ? "link" : "down") : "disabled";
        speed = conf.admin.enable ? (port_status.link ? port_speed_duplex_to_txt_from_status(port_status) : "Down") : "Disabled";

        // bottom row
        row = 0;
        xoff_add = PORT_ICON_WIDTH;

        // Only has only Cu ports on the front panel.
        type = "copper";
        // all front panel Cu ports are in two rows,
        // so latch is facing down for the bottom row
        placement = "bottom";
        if (iport % 2) {
            // top row
            row = 1;
            // don't offset x-axis
            xoff_add = 0;
            // Cu port, top row, so latch is facing up
            placement = "top";
        } else if (iport != 0 && (iport % block_size) == 0) {
            // x-axis offset between port blocks
            xoff_add += PORT_BLOCK_GAP;
        }

        xoff += xoff_add;
        port_httpd_write(p, uport, type, state, placement, speed, xoff, row);
    }

    // panel separator
    cyg_httpd_write_chunked(",", 1);
    // Back panel
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|", 2, background);
    cyg_httpd_write_chunked(p->outbuffer, ct);

    if (board_type == VTSS_BOARD_LAN9668_8PORT_REF) {
        sma_port_state(p, &xoff);
    }
}
