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

/****************************************************************************
 * Containing iCLI functions shared by LLDP and LLDP-MED
 ****************************************************************************/
#ifdef VTSS_SW_OPTION_LLDP

#include <time.h> // For time_t
#include "lldp_remote.h" // For vtss_appl_lldp_remote_entry_t
#include "icli_api.h" // For icli_port_info_txt
#include "icli_porting_util.h" // For icli_port_info_txt
#include "lldp_icli_shared_functions.h"
#include "lldp_trace.h"

// See lldp_icli_shared_functions.h
char *lldp_local_interface_txt_get(char *buf, const vtss_appl_lldp_remote_entry_t *entry, const switch_iter_t *sit, const port_iter_t *pit)
{
    // This function takes an iport and prints a uport or a LAG, GLAG
    if (lldp_remote_receive_port_to_string(entry->receive_port, buf, sit->isid)) {
        // Part of LAG
        return buf;
    } else {
        // Ok, this was a port. That we prints as an interface
        T_IG(TRACE_GRP_CLI, "uport:%d", pit->uport);
        return icli_port_info_txt(sit->usid, pit->uport, buf);
    }
}
#endif // #ifdef VTSS_SW_OPTION_LLDP

