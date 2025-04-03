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

#ifndef RFC1213_MIB2_H
#define RFC1213_MIB2_H

#define RFC1213_SUPPORTED_SYSTEM       1
#define RFC1213_SUPPORTED_INTERFACES   1
#define RFC1213_SUPPORTED_ICMP         0 /* deprecated by RFC4293 */
#define RFC1213_SUPPORTED_TCP          1
#define RFC1213_SUPPORTED_UDP          1

#define RFC1213_SUPPORTED_ORTABLE      0
#define RFC1213_SUPPORTED_IP           0 /* deprecated by RFC4293 */
#define RFC1213_SUPPORTED_SNMP         0

#include "ifIndex_api.h"
#include "iana_ifType.h"
#include <vtss/appl/port.h> // For vtss_appl_port_status_t

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Function declarations
 */
#if RFC1213_SUPPORTED_SYSTEM
/* system ----------------------------------------------------------*/
void init_mib2_system(void);
FindVarMethod var_system;
WriteMethod write_sysName;
WriteMethod write_sysLocation;
WriteMethod write_sysContact;
#endif /* RFC1213_SUPPORTED_SYSTEM */

#if RFC1213_SUPPORTED_INTERFACES
/* interfaces ----------------------------------------------------------*/
#include "ifIndex_api.h"

#define IFDESCR_MAX_LEN       255

typedef struct {
    u_long ifIndex;
    u_char ifDescr[IFDESCR_MAX_LEN + 1];
    u_long ifType;
    u_long ifMtu;
    u_long ifSpeed;
    u_char ifPhysAddress[6];
    u_long ifAdminStatus;
    u_long ifOperStatus;
    u_long ifLastChange;
    u_long ifInOctets;
    u_long ifInUcastPkts;
    u_long ifInNUcastPkts;
    u_long ifInDiscards;
    u_long ifInErrors;
    u_long ifInUnknownProtos;
    u_long ifOutOctets;
    u_long ifOutUcastPkts;
    u_long ifOutNUcastPkts;
    u_long ifOutDiscards;
    u_long ifOutErrors;
    u_long ifOutQLen;
    oid    ifSpecific[MAX_OID_LEN];
    size_t ifSpecific_len;
} ifTable_entry_t;

void init_mib2_interfaces(void);
ulong rfc1213_get_ifNumber(void);
BOOL get_ifTable_entry(iftable_info_t *table_info, ifTable_entry_t *table_entry_p);
mesa_rc rfc1213_set_ifAdminStatus(vtss_isid_t isid, mesa_port_no_t port_no, BOOL status);
void interfaces_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status);
WriteMethod write_ifAdminStatus;
#endif /* RFC1213_SUPPORTED_INTERFACES */

#if RFC1213_SUPPORTED_IP
/* ip ----------------------------------------------------------*/
void init_mib2_ip(void);
#endif /* RFC1213_SUPPORTED_IP */

#if RFC1213_SUPPORTED_ICMP
/* icmp ----------------------------------------------------------*/
void init_mib2_icmp(void);
#endif /* RFC1213_SUPPORTED_ICMP */

#if RFC1213_SUPPORTED_TCP
/* tcp ----------------------------------------------------------*/
void init_mib2_tcp(void);
#endif /* RFC1213_SUPPORTED_TCP */

#if RFC1213_SUPPORTED_UDP
/* udp ----------------------------------------------------------*/
void init_mib2_udp(void);
#endif /* RFC1213_SUPPORTED_UDP */

#if RFC1213_SUPPORTED_SNMP
/* snmp ----------------------------------------------------------*/
void init_mib2_snmp(void);
#endif /* RFC1213_SUPPORTED_SNMP */

int get_available_lport_ifTableIndex(int if_num);
int get_available_lag_ifTableIndex(int if_num);
int get_available_vlan_ifTableIndex(int if_num);
int get_available_ifTableIndex(int if_num);
BOOL get_ifTableIndex_info(int if_index, iftable_info_t *table_info_p);
BOOL RFC1213_MIB2C_is_nvt_string(char *str);

#ifdef __cplusplus
}
#endif
#endif /* RFC1213_MIB2_H */

