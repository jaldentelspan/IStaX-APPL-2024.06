/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "main.h"
#include "conf_api.h"
#include "msg_api.h"
#include "standalone_api.h"

/* Topology module functions replacements */
mesa_rc topo_isid2mac(const vtss_isid_t isid, mesa_mac_addr_t mac_addr)
{
    (void)conf_mgmt_mac_addr_get(mac_addr, 0);
    return VTSS_RC_OK;
}

/* Initialize module */
mesa_rc standalone_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_START) {
        msg_topo_event(MSG_TOPO_EVENT_MASTER_UP, VTSS_ISID_START);
        msg_topo_event(MSG_TOPO_EVENT_SWITCH_ADD, VTSS_ISID_START);
    }
    return VTSS_RC_OK;
}

