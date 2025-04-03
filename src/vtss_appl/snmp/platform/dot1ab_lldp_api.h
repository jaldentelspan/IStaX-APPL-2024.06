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

#ifndef DOT1AB_LLDP_API_H
#define DOT1AB_LLDP_API_H
#ifdef __cplusplus
extern "C" {
#endif
// LLDP notification Trap - Must be called when ever a entry is modified.
void snmpLLDPNotificationChange(vtss_isid_t isid, int port_index, vtss_appl_lldp_global_counters_t *stats, int interval);
int dot1ab_get_next_entry (int start_index, vtss_appl_lldp_remote_entry_t *entry, int table_size);
char BITS_type_swapbyte(unsigned char input);
#ifdef __cplusplus
}
#endif
#endif /* DOT1AB_LLDP_API_H */

