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
#define BOARD_TYPE_LAGUNA_PCB8398 0x8398 /* Laguna 24x1G + 4x10G port */

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
    *xoff += PORT_ICON_WIDTH;
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_ptp_in, placement, name, *xoff, row);
    name = "1PPS Output";
    *xoff += PORT_ICON_WIDTH;
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_ptp_out, placement, name, *xoff, row);
    /* for SMA freq o/p connector */
    name = "Freq. Output";
    *xoff += PORT_ICON_WIDTH;
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_freq_out, placement, name, *xoff, row);
    *xoff += PORT_ICON_WIDTH;
}

static void time_port_state(CYG_HTTPD_STATE *p, int xoff)
{
    const char *name =            "<unknown>";
    const char *state_rs422_in =  "<unknown>";
    const char *state_rs422_out = "<unknown>";
    const char *type = "copper";
    const char *placement = "top";
    int        row = 0;

#if defined(VTSS_SW_OPTION_PTP) && defined(VTSS_SW_OPTION_SYNCE)
    vtss_ptp_rs422_conf_t mode;

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
    }
#else
    state_rs422_in  = "down";
    state_rs422_out = "down";
#endif

    // time interface i/p and o/p
    name = "Time Input";
    //xoff += PORT_BLOCK_GAP + PORT_ICON_WIDTH;
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_rs422_in, placement, name, xoff, row);
    name = "Time Output";
    xoff += (PORT_BLOCK_GAP + PORT_ICON_WIDTH);
    port_httpd_write(p, NON_ETH_PORT_NO, type, state_rs422_out, placement, name, xoff, row);
}

/*
 * The (basic) portstate handler
 */

void stat_portstate_switch_sparx5(CYG_HTTPD_STATE *p)
{
    mesa_port_no_t          iport, num_of_back_ports;
    vtss_uport_no_t         uport;
    vtss_appl_port_conf_t   conf;
    vtss_appl_port_status_t port_status;
    const char              *background = "switch_48.png", *placement, *type, *state, *speed;
    int                     ct, xoff_add, xoff = 0, row, block_size = 12;

    // Target
    mesa_target_type_t chip_id = vtss_api_chipid();
    // Board type
    uint32_t board_type = vtss_board_type();
    switch (board_type) {
    case BOARD_TYPE_SPARX5_PCB134:
        num_of_back_ports = 1;
        xoff = 8 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
        break;

    case BOARD_TYPE_SPARX5_PCB135:
        switch (chip_id) {
        case MESA_TARGET_7546:
        case MESA_TARGET_7546TSN:
        case MESA_TARGET_7549:
        case MESA_TARGET_7549TSN:
            num_of_back_ports = 1;
            break;

        case MESA_TARGET_7552:
        case MESA_TARGET_7552TSN:
        case MESA_TARGET_7558:
        case MESA_TARGET_7558TSN:
            num_of_back_ports = 5;
            break;

        default:
            T_E("Unsupported target (%04x) and board (%d) combination.", chip_id, board_type);
            return;
        }

        break;

    case BOARD_TYPE_LAGUNA_PCB8398:
        background = "switch.png";
        switch (chip_id) {
        case MESA_TARGET_LAN9694:
        case MESA_TARGET_LAN9691VAO:
        case MESA_TARGET_LAN9694TSN:
        case MESA_TARGET_LAN9694RED:
        case MESA_TARGET_LAN9696:
        case MESA_TARGET_LAN9692VAO:
        case MESA_TARGET_LAN9696TSN:
        case MESA_TARGET_LAN9696RED:
        case MESA_TARGET_LAN9698:
        case MESA_TARGET_LAN9693VAO:
        case MESA_TARGET_LAN9698TSN:
        case MESA_TARGET_LAN9698RED:
            num_of_back_ports = 1;
            break;
        default:
            T_E("Unsupported target (%04x) and board (%d) combination.", chip_id, board_type);
            return;
        }

        break;
    default:
        T_E("Unsupported board (%d)", board_type);
        return;
    }

    /* Backround, SID, managed */
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|", 1, background);
    cyg_httpd_write_chunked(p->outbuffer, ct);

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

        if ((board_type == BOARD_TYPE_SPARX5_PCB134)) {
            meba_port_cap_t cap = 0;
            // PCB134 has only SFPs on the front panel.
            type = "sfp";
            // latch is facing down
            placement = "bottom";
            // Ports are in two rows

            VTSS_RC_ERR_PRINT(port_cap_get(iport, &cap));
            if (cap & (MEBA_PORT_CAP_25G_FDX)) {
                // In configuration 10x10G + 4x25G port 11 and 12 are 25G ports in lower row
                // x-axis offset between quad-SFP cages
                if (iport == 10) {
                    xoff_add += PORT_BLOCK_GAP;
                }
            } else {
                if (iport < 12 ) {
                    if (iport % 2) {
                        // top row
                        row = 1;
                        // don't offset x-axis
                        xoff_add = 0;
                    } else {
                        // latch is facing up
                        placement = "top";
                    }
                }
                // x-axis offset between quad-SFP cages
                if (iport == 4 || iport == 8 || iport == 12) {
                    xoff_add += PORT_BLOCK_GAP;
                }
            }
        } else if (board_type == BOARD_TYPE_SPARX5_PCB135) {
            // PCB135, which has only Cu ports on the front panel.
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
        } else if (board_type == BOARD_TYPE_LAGUNA_PCB8398) {
            // Laguna ref board, 24 Cu ports and 4 SFP cages
            if (iport > 23 && iport < 28) {
                type = "sfp";
                placement = "bottom";
                if (iport == 24) {
                    // x-axis offset between port blocks
                    xoff_add += PORT_BLOCK_GAP;
                }
            } else {
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
            }
        } else {
            type = "copper";
            placement = "bottom";
        }

        xoff += xoff_add;
        port_httpd_write(p, uport, type, state, placement, speed, xoff, row);
    }

    // panel separator
    cyg_httpd_write_chunked(",", 1);
    // Back panel
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|", 2, background);
    cyg_httpd_write_chunked(p->outbuffer, ct);

    if (board_type == BOARD_TYPE_SPARX5_PCB135) {
        // PCB135 - 1) SMA interfaces
        xoff = 6 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP) + PORT_ICON_WIDTH;
        sma_port_state(p, &xoff);
        xoff += 2 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
    }
    if (board_type == BOARD_TYPE_LAGUNA_PCB8398) {
        // Laguna - 1) SMA interfaces
        xoff = (PORT_ICON_WIDTH + PORT_BLOCK_GAP) + PORT_ICON_WIDTH;
        sma_port_state(p, &xoff);
        xoff += 2 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
    }

    for (iport = fast_cap(MEBA_CAP_BOARD_PORT_COUNT) - num_of_back_ports; iport < fast_cap(MEBA_CAP_BOARD_PORT_COUNT); iport++) {
        uport = iport2uport(iport);
        vtss_ifindex_t ifindex;
        if (vtss_ifindex_from_port(VTSS_ISID_START, iport, &ifindex) != VTSS_RC_OK) {
            T_E("Could not get ifindex for uport %d", uport);
            return;
        };
        if (vtss_appl_port_conf_get(ifindex, &conf) < 0 ||
            vtss_appl_port_status_get(ifindex, &port_status) < 0) {
            T_E("Could not get status for uport %d", uport);
            return;
        }

        state = conf.admin.enable ? (port_status.link ? "link" : "down") : "disabled";
        speed = conf.admin.enable ? (port_status.link ? port_speed_duplex_to_txt_from_status(port_status) : "Down") : "Disabled";

        // bottom row
        row = 0;
        xoff_add = PORT_ICON_WIDTH + PORT_BLOCK_GAP;

        /* NPI port */
        if (iport == fast_cap(MEBA_CAP_BOARD_PORT_COUNT) - 1) {
            type = "copper";
            // Cu port, top row, so latch is facing up
            placement = "top";
            if (board_type == BOARD_TYPE_SPARX5_PCB134) {
                // PCB134 - 1) MGMT port
                xoff = 5 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP) + PORT_ICON_WIDTH;
            } else if (board_type == BOARD_TYPE_SPARX5_PCB135) {
                // PCB135 - 2) MGMT port
                if (num_of_back_ports == 5) {
                    // This is either 7552 or 7558 with 4 SFP cages on the back
                    xoff -= 6 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
                } else {
                    // This is either 7546 or 7549 with no SFP on the back
                    xoff -= 2 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
                }
            } else if (board_type == BOARD_TYPE_LAGUNA_PCB8398) {
                xoff = 4 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP) + PORT_ICON_WIDTH;
            }
        } else {
            // PCB135 - 2b) SFP cages
            type = "sfp";
            // SFP port, bottom row, so latch is facing down
            placement = "bottom";
        }

        xoff += xoff_add;
        port_httpd_write(p, uport, type, state, placement, speed, xoff, row);
    }

    if (board_type == BOARD_TYPE_SPARX5_PCB134 || board_type == BOARD_TYPE_SPARX5_PCB135) {
        xoff += 7 * (PORT_ICON_WIDTH + PORT_BLOCK_GAP);
    } else {
        xoff += 5 * PORT_BLOCK_GAP;
    }
    if (board_type == BOARD_TYPE_SPARX5_PCB134) {
        // PCB134 - 2) SMA interfaces
        sma_port_state(p, &xoff);
        xoff += PORT_ICON_WIDTH + PORT_BLOCK_GAP;
    }
    // PCB134/PCB135 - 3) Time interfaces
    time_port_state(p, xoff);
}

