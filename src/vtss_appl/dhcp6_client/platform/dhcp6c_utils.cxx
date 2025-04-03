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

#include "conf_api.h"
#include "dhcp6c_priv.hxx"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define VTSS_TRACE_MODULE_ID            VTSS_MODULE_ID_DHCP6C

namespace vtss
{
namespace dhcp6c
{
namespace utils
{
static mesa_mac_t   system_mac = {};
static mesa_mac_t   device_mac = {};
/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

/*
******************************************************************************

    Public functions

******************************************************************************
*/
BOOL device_mac_get(mesa_mac_t *const mac)
{
    u8  macx;

    if (!mac) {
        return FALSE;
    }

    for (macx = 0; macx < sizeof(mesa_mac_t); ++macx) {
        if (device_mac.addr[macx]) {
            memcpy(mac, &device_mac, sizeof(mesa_mac_t));
            return TRUE;
        }
    }

    return FALSE;
}

void device_mac_set(const mesa_mac_t *const mac)
{
    if (mac) {
        memcpy(&device_mac, mac, sizeof(mesa_mac_t));
    } else {
        if (conf_mgmt_mac_addr_get((u8 *)&device_mac, 0) != VTSS_RC_OK) {
            T_W("Device MAC for DHCPv6 SetFail!");
        }
    }
}

void device_mac_clr(void)
{
    memset(&device_mac, 0x0, sizeof(mesa_mac_t));
}

BOOL system_mac_get(mesa_mac_t *const mac)
{
    u8  macx;

    if (!mac) {
        return FALSE;
    }

    for (macx = 0; macx < sizeof(mesa_mac_t); ++macx) {
        if (system_mac.addr[macx]) {
            memcpy(mac, &system_mac, sizeof(mesa_mac_t));
            return TRUE;
        }
    }

    return FALSE;
}

void system_mac_set(const mesa_mac_t *const mac)
{
    if (mac) {
        memcpy(&system_mac, mac, sizeof(mesa_mac_t));
    } else {
        if (conf_mgmt_mac_addr_get((u8 *)&system_mac, 0) != VTSS_RC_OK) {
            T_W("System MAC for DHCPv6 SetFail!");
            device_mac_set(0);
        } else {
            device_mac_set(&system_mac);
        }
    }
}

void system_mac_clr(void)
{
    memset(&system_mac, 0x0, sizeof(mesa_mac_t));
}

mesa_rc eui64_linklocal_addr_get(mesa_ipv6_t *const ipv6_addr)
{
    mesa_mac_t  mac;

    if (!ipv6_addr || (!device_mac_get(&mac) && !system_mac_get(&mac))) {
        return VTSS_RC_ERROR;
    }

    memset(ipv6_addr, 0x0, sizeof(mesa_ipv6_t));
    memcpy(&ipv6_addr->addr[13], &mac.addr[3], sizeof(u8) * 3);
    ipv6_addr->addr[12] = 0xFE;
    ipv6_addr->addr[11] = 0xFF;
    memcpy(&ipv6_addr->addr[8], &mac.addr[0], sizeof(u8) * 3);

    ipv6_addr->addr[8] &= 0xFE;  /* G/I Bit */
    ipv6_addr->addr[8] |= 0x02;  /* U/L Bit */

    ipv6_addr->addr[1] = 0x80;
    ipv6_addr->addr[0] = 0xFE;

    return VTSS_RC_OK;
}

mesa_rc convert_ip_to_mac(const mesa_ipv6_t *const ipa, mesa_mac_t *const mac)
{
    if (!ipa || !mac) {
        return VTSS_RC_ERROR;
    }

    mac->addr[0] = 0x33;
    mac->addr[1] = 0x33;
    mac->addr[2] = ipa->addr[12];
    mac->addr[3] = ipa->addr[13];
    mac->addr[4] = ipa->addr[14];
    mac->addr[5] = ipa->addr[15];

    return VTSS_RC_OK;
}

} /* utils */
} /* dhcp6c */
} /* vtss */
