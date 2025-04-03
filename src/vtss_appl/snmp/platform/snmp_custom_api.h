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

#ifndef _VTSS_SNMP_CUSTOM_H_
#define _VTSS_SNMP_CUSTOM_H_

/*
Organization of Private MIB:

In the following, <product_id> is only inserted when SNMP_PRIVATE_MIB_ENTERPRISE == 6603 == VTSS.
It is encoded like this:
  Bits 31:24 is the software type (see misc_softwaretype()).
  Bits 15:00 is the chip ID with which the API is instantiated (see misc_chiptype()).

{iso, org, dod, internet, private, enterprise, VTSS, ...}
{1,   3,   6,   1,        4,       1,          6603, <product_id>, <sw_module_id>} concatenated with:

|---<switchMgmt(1)>
    |---<systemMgmt(1)>
    |---<ipMgmt(1)>
    |---<vlanMgmt(2)>
    |---<portMgmt(3)>
    |---...
|---<switchNotifications(2)>
    |---<SwitchTraps(1)>
        |---<trap_a(1)>
        |---<trap_b(2)>
        |---...
*/
#ifndef MIB_ENTERPRISE_OID
#define SNMP_PRIVATE_MIB_ENTERPRISE               6603 /* VTSS */
#else
#define SNMP_PRIVATE_MIB_ENTERPRISE               MIB_ENTERPRISE_OID // Determine from  config file option $(Custom/MibEnterpriseOid)
#endif
#ifndef MIB_ENTERPRISE_PRODUCT_ID
#define SNMP_PRIVATE_MIB_PRODUCT_ID               1 /* vtssSwitchMgmt */
#else
#define SNMP_PRIVATE_MIB_PRODUCT_ID               MIB_ENTERPRISE_PRODUCT_ID // Determine from config file option $(Custom/MibEnterpriseProductId)
#endif

/* Management branch */
#define SNMP_PRIVATE_MIB_SWITCH_MGMT              1

/* Notification branch */
#define SNMP_PRIVATE_MIB_SWITCH_NOTIFICATIONS     2
#define SNMP_PRIVATE_MIB_SWITCH_TRAPS             1

#ifdef __cplusplus
extern "C" {
#endif
extern  oid snmp_private_mib_oid[];
u32     snmp_private_mib_oid_len_get(void);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_SNMP_CUSTOM_H_ */

