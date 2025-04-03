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

#include <main.h>
#include "vtss_os_wrapper_snmp.h"
#include "vtss_snmp_api.h"
#include "mibContextTable.h"  //mibContextTable_register
#include "rfc3415_vacm.h"

#if RFC3415_SUPPORTED_VACMCONTEXTTABLE
/*
 * Initializes the MIBObjects module
 */
void init_vacmContextTable(void)
{
    oid vacmContextTable_variables_oid[] = { 1, 3, 6, 1, 6, 3, 16, 1, 1 };

    // Register mibContextTable
    mibContextTable_register(vacmContextTable_variables_oid,
                             sizeof(vacmContextTable_variables_oid) / sizeof(oid),
                             "SNMP-VIEW-BASED-ACM-MIB : vacmContextTable");

}
#endif /* RFC3415_SUPPORTED_VACMCONTEXTTABLE */

#if RFC3415_SUPPORTED_VACMSECURITYTOGROUPTABLE
/*
 * Initializes the MIBObjects module
 */
void init_vacmSecurityToGroupTable(void)
{
    oid vacmSecurityToGroupTable_variables_oid[] = { 1, 3, 6, 1, 6, 3, 16, 1, 2 };

    // Register mibContextTable
    mibContextTable_register(vacmSecurityToGroupTable_variables_oid,
                             sizeof(vacmSecurityToGroupTable_variables_oid) / sizeof(oid),
                             "SNMP-VIEW-BASED-ACM-MIB : vacmSecurityToGroupTable");

}
#endif /* RFC3415_SUPPORTED_VACMSECURITYTOGROUPTABLE */

#if RFC3415_SUPPORTED_VACMACCESSTABLE
/*
 * Initializes the MIBObjects module
 */
void init_vacmAccessTable(void)
{
    oid vacmAccessTable_variables_oid[] = { 1, 3, 6, 1, 6, 3, 16, 1, 4 };

    // Register mibContextTable
    mibContextTable_register(vacmAccessTable_variables_oid,
                             sizeof(vacmAccessTable_variables_oid) / sizeof(oid),
                             "SNMP-VIEW-BASED-ACM-MIB : vacmAccessTable");

}
#endif /* RFC3415_SUPPORTED_VACMACCESSTABLE */

#if RFC3415_SUPPORTED_VACMMIBVIEWS
/*
 * Initializes the MIBObjects module
 */
void init_vacmMIBViews(void)
{
    oid vacmMIBViews_variables_oid[] = { 1, 3, 6, 1, 6, 3, 16, 1, 5 };

    // Register mibContextTable
    mibContextTable_register(vacmMIBViews_variables_oid,
                             sizeof(vacmMIBViews_variables_oid) / sizeof(oid),
                             "SNMP-VIEW-BASED-ACM-MIB : vacmMIBViews");

}
#endif /* RFC3415_SUPPORTED_VACMMIBVIEWS */

