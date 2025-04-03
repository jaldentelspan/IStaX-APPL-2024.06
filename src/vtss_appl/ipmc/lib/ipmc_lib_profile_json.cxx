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
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::ipmc_lib_profile::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_ipmc_lib_profile("ipmcProfile");
extern "C" void vtss_appl_ipmc_lib_profile_json_init()
{
    json_node_add(&ns_ipmc_lib_profile);
}

NS(ns_conf,               ns_ipmc_lib_profile, "config");
NS(ns_conf_address_range, ns_conf,             "addressRange");

static StructReadWrite<IpmcProfileGlobalsConfig> ipmc_profile_globals_config(
    &ns_conf, "global");

static TableReadWriteAddDelete<IpmcProfileManagementTable> ipmc_profile_management_table(
    &ns_conf, "management");

static TableReadWriteAddDelete<IpmcProfileIpv4RangeTable> ipmc_profile_ipv4_range_table(
    &ns_conf_address_range, "ipv4");

#ifdef VTSS_SW_OPTION_IPV6
static TableReadWriteAddDelete<IpmcProfileIpv6RangeTable> ipmc_profile_ipv6_range_table(
    &ns_conf_address_range, "ipv6");
#endif /* VTSS_SW_OPTION_IPV6 */

static TableReadWriteAddDelete<IpmcProfileRuleTable> ipmc_profile_rule_table(
    &ns_conf, "rule");

