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
#include "board_if.h"
#include "msg_api.h"
#include "vtss_api_if_api.h"

/* Stock resources in sw_mgd_html/images/jack_.... */
#define PORT_ICON_WIDTH     32
#define PORT_ICON_HEIGHT    23

static const char *port_type(mesa_port_no_t iport)
{
    switch (port_custom_table[iport].mac_if) {
    case MESA_PORT_INTERFACE_SGMII:
    case MESA_PORT_INTERFACE_QSGMII:
        return "copper";
    case MESA_PORT_INTERFACE_XAUI:
        return port_custom_table[iport].cap & MEBA_PORT_CAP_VTSS_10G_PHY ? "sfp" : "x2";
    case MESA_PORT_INTERFACE_SERDES:
    default:
        return "sfp";
    }
}

/*
 * The (basic) portstate handler
 */

void stat_portstate_switch_lu26(CYG_HTTPD_STATE *p, vtss_usid_t usid, vtss_isid_t isid)
{
    mesa_port_no_t     iport;
    vtss_uport_no_t    uport;
    vtss_appl_port_conf_t conf;
    vtss_appl_port_status_t port_status;
    int                ct, xoff, yoff, row, col;
    char               combo_port = 0;
    char               sfp_port = 0;
    BOOL               lu26 = (vtss_board_type() == VTSS_BOARD_LUTON26_REF);
    u32                port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    /* Backround, SID, managed */
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|",
                  usid,
                  lu26 ? "switch_enzo.png" : "switch_small.png");
    cyg_httpd_write_chunked(p->outbuffer, ct);

    /* Emit port icons */
    for (iport = 0; iport < port_cnt; iport++) {
        uport = iport2uport(iport);

        vtss_ifindex_t ifindex;
        if (vtss_ifindex_from_port(isid, iport, &ifindex) != VTSS_RC_OK) {
            return;
        }

        if (vtss_appl_port_conf_get(ifindex, &conf) < 0 ||
            vtss_appl_port_status_get(ifindex, &port_status) < 0) {
            break;    /* Most likely stack error - bail out */
        }

        const char *state = conf.admin.enable ? (port_status.link ? "link" : "down") : "disabled";
        const char *speed = conf.admin.enable ?
                            (port_status.link ?
                             port_speed_duplex_to_txt_from_status(port_status) :
                             "Down") : "Disabled";
        if (port_custom_table[iport].cap & MEBA_PORT_CAP_SFP_DETECT) {
            if (lu26) {
                row = 0;
                col = 18 + (iport + 2 - fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)); // 2x SFP ports
                xoff = PORT_ICON_WIDTH + col * PORT_ICON_WIDTH + 20;
                yoff = PORT_ICON_HEIGHT * (2 - row);
            } else {
                row = 0;
                col = 10 + sfp_port;
                xoff = PORT_ICON_WIDTH + col * PORT_ICON_WIDTH + (12 * (col / 4 + (sfp_port++)));
                yoff = PORT_ICON_HEIGHT * 2;
            }
        } else {
            if (lu26) {
                /* Two columns */
                row = iport % 2;
                col = iport / 2;
                xoff = PORT_ICON_WIDTH + col * PORT_ICON_WIDTH + (12 * (col / 6));
                yoff = PORT_ICON_HEIGHT * (2 - row);
            } else {
                /* Single column */
                row = 0;
                col = iport;
                xoff = PORT_ICON_WIDTH + col * PORT_ICON_WIDTH + (12 * (col / 4));
                yoff = PORT_ICON_HEIGHT * 2;
            }
        }

        // If the active link is fiber and the port has link then
        // change the state to down for the copper port.
        if (port_custom_table[iport].cap & (MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER) &&
            port_status.fiber && port_status.link) {
            state = "down";
        }

        // Update the copper ports
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%u/jack_%s_%s_%s.png/Port %u: %s/%u/%u/%u/%u/%d|",
                      uport,
                      port_type(iport),
                      state,
                      lu26 && row != 0 ? "top" : "bottom",
                      uport, speed,
                      xoff,     /* xoff */
                      yoff,     /* yoff */
                      PORT_ICON_WIDTH,
                      PORT_ICON_HEIGHT,
                      row == 0 ? 1 : -1);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        // Update the fiber ports
        if (port_custom_table[iport].cap & (MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER)) {
            col = 12 + (combo_port);
            xoff = PORT_ICON_WIDTH + col * PORT_ICON_WIDTH + (12 * (col / 4));
            yoff = PORT_ICON_HEIGHT * 2;
            combo_port++;

            // If the active link is cobber and the port has link then
            // change the state to down for the fiber port.
            if (!conf.admin.enable) {
                state = "disabled";
            } else if (!port_status.fiber || !port_status.link) {
                state = "down";
            } else {
                state = "link";
            }

            // Transmit fiber port information.
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%u/jack_%s_%s_%s.png/Port %u: %s/%u/%u/%u/%u/%d|",
                          uport,
                          "sfp",
                          state,
                          "bottom",
                          uport, speed,
                          xoff,     /* xoff */
                          yoff,     /* yoff */
                          PORT_ICON_WIDTH,
                          PORT_ICON_HEIGHT,
                          1);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    }

    cyg_httpd_write_chunked(",", 1);
}
