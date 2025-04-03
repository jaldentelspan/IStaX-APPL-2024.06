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
#ifndef _VTSS_RMON_TYPES_H_
#define _VTSS_RMON_TYPES_H_

#include "vtss_os_wrapper_snmp.h"

/* In NET-SNMP, the following APIs and data strcut is changed */
extern char *sprint_objid(char *buf, oid *objid, size_t objidlen);
#define find_subtree(a,b,c)  netsnmp_subtree_find((a),(b),(c),"")
typedef netsnmp_subtree vtss_subtree_t;
#define VTSS_SUBTREE_START(a) (a)->start_a
#define VTSS_SUBTREE_END(a) (a)->end_a
#define VTSS_SUBTREE_NAME(a) (a)->name_a

#endif /* _VTSS_RMON_TYPES_H_ */
