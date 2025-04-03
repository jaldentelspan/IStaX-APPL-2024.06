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

#include "vtss_lacp.h"

/**
 * vtss_os_get_linkstate - Return link state for a given physical port
 * @portno: Port number (1 - VTSS_LACP_MAX_PORTS)
 *
 */
vtss_lacp_linkstate_t vtss_os_get_linkstate(vtss_lacp_port_t portno)
{
    return VTSS_LACP_LINKSTATE_DOWN;
}

/**
 * vtss_os_get_linkduplex - Return link duplex mode for a given physical port
 * If link state is "down" the duplexmode is returned as VTSS_LACP_LINKDUPLEX_HALF.
 * @portno: Port number (1 - VTSS_LACP_MAX_PORTS)
 *
 */
vtss_lacp_duplex_t vtss_os_get_linkduplex(vtss_lacp_port_t portno)
{
    return VTSS_LACP_LINKDUPLEX_HALF;
}

/**
 * vtss_os_make_key - Return the operational key given physical port and the admin key.
 * This value should be the same for ports that can physically aggregate together.
 * An obvious candidate is the link speed.
 * @portno: Port number (1 - VTSS_LACP_MAX_PORTS)
 * @new_key: The admin value of the new key. By convention VTSS_LACP_AUTOKEY means that
 *           the switch is free to choose the key.
 *
 */
vtss_lacp_key_t vtss_os_make_key(vtss_lacp_port_t portno, vtss_lacp_key_t new_key)
{
    return new_key;
}

/**
 * vtss_os_get_systemmac - Return MAC address associated with the system.
 * @system_macaddr: Return the value.
 *
 */
void vtss_os_get_systemmac(vtss_lacp_macaddr_t *system_macaddr)
{
}

/**
 * vtss_os_get_portmac - Return MAC address associated a specific physical port.
 * @portno: Port number (1 - VTSS_LACP_MAX_PORTS)
 * @port_macaddr: Return the value.
 *
 */
void vtss_os_get_portmac(vtss_lacp_port_t portno, vtss_lacp_macaddr_t *port_macaddr)
{
}

/**
 * vtss_os_send - Transmit a MAC frame on a specific physical port.
 * @portno: Port number (1 - VTSS_LACP_MAX_PORTS)
 * @frame: A full MAC frame.
 * @len: The length of the MAC frame.
 *
 * Return: VTSS_LACP_CC_OK if frame was successfully queued for transmission.
 *         VTSS_LACP_CC_GENERR in case of error.
 */
int vtss_os_send(vtss_lacp_port_t portno, vtss_lacp_frame_t *frame, vtss_lacp_framelen_t len)
{
    return VTSS_LACP_CC_OK;
}

/**
 * vtss_os_set_hwaggr - Add a specified port to an existing set of ports for aggregation.
 * @new_port: Port number (1 - VTSS_LACP_MAX_PORTS)
 * @portmembers: A set port ports to be aggregated together. new_port is included.
 *
 */
void vtss_os_set_hwaggr(vtss_lacp_port_t new_port,
                        vtss_lacp_port_t portmembers[VTSS_LACP_MAX_PORTS])
{
}

/**
 * vtss_os_clear_hwaggr - Remove a specified port from an existing set of ports for aggregation.
 * @old_port: Port number (1 - VTSS_LACP_MAX_PORTS)
 * @portmembers: A set port ports to be aggregated together. old_port is included.
 *
 */
void vtss_os_clear_hwaggr(vtss_lacp_port_t old_port,
                          vtss_lacp_port_t portmembers[VTSS_LACP_MAX_PORTS])
{
}






