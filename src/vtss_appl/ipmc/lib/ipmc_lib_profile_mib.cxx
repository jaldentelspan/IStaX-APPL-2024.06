/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "ipmc_lib_profile_serializer.hxx"

VTSS_MIB_MODULE("ipmcProfileMib", "IPMC-PROFILE", ipmc_lib_profile_mib_init,
                VTSS_MODULE_ID_IPMC_LIB, root, h)
{
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("202204190000Z", "Precedence group no longer supported. Add profile ranges in order. Deny/Permit is now a boolean.");
    h.description("This is a private version of the IPMC Profile MIB");
}

using namespace vtss;
using namespace expose::snmp;

namespace vtss
{
namespace appl
{
namespace ipmc_lib_profile
{
namespace interfaces
{

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(ipmc_profile_objects, root, 1, "ipmcProfileMibObjects");;
NS(ipmc_profile_config, ipmc_profile_objects, 2, "ipmcProfileConfig");;

static StructRW2<IpmcProfileGlobalsConfig> ipmc_profile_globals_config(
    &ipmc_profile_config, vtss::expose::snmp::OidElement(1, "ipmcProfileConfigGlobals"));

static TableReadWriteAddDelete2<IpmcProfileManagementTable> ipmc_profile_management_table(
    &ipmc_profile_config, vtss::expose::snmp::OidElement(2, "ipmcProfileConfigManagementTable"), vtss::expose::snmp::OidElement(3, "ipmcProfileConfigManagementTableRowEditor"));

static TableReadWriteAddDelete2<IpmcProfileIpv4RangeTable> ipmc_profile_ipv4_range_table(
    &ipmc_profile_config, vtss::expose::snmp::OidElement(4, "ipmcProfileConfigIpv4AddressRangeTable"), vtss::expose::snmp::OidElement(5, "ipmcProfileConfigIpv4AddressRangeTableRowEditor"));

#ifdef VTSS_SW_OPTION_IPV6
static TableReadWriteAddDelete2<IpmcProfileIpv6RangeTable> ipmc_profile_ipv6_range_table(
    &ipmc_profile_config, vtss::expose::snmp::OidElement(6, "ipmcProfileConfigIpv6AddressRangeTable"), vtss::expose::snmp::OidElement(7, "ipmcProfileConfigIpv6AddressRangeTableRowEditor"));
#endif /* VTSS_SW_OPTION_IPV6 */

static TableReadWriteAddDelete2<IpmcProfileRuleTable> ipmc_profile_rule_table(
    &ipmc_profile_config, vtss::expose::snmp::OidElement(8, "ipmcProfileConfigRuleTable"), vtss::expose::snmp::OidElement(9, "ipmcProfileConfigRuleTableRowEditor"));

}  // namespace interfaces
}  // namespace ipmc_lib_profile
}  // namespace appl
}  // namespace vtss

