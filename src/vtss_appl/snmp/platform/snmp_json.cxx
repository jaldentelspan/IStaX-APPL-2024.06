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

#include "snmp_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::snmp::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_snmp("snmp");
extern "C" void vtss_appl_snmp_json_init() { json_node_add(&ns_snmp); }

NS(ns_conf,       ns_snmp, "config");

static StructReadWrite<SnmpParamsLeaf> snmp_params_leaf(&ns_conf, "global");
static TableReadWriteAddDelete<SnmpCommunityEntry> snmp_community_entry(&ns_conf, "communities");
static TableReadWriteAddDelete<SnmpCommunity6Entry> snmp_community6_entry(&ns_conf, "communities6");
static TableReadWriteAddDelete<SnmpUserEntry> snmp_user_entry(&ns_conf, "users");
static TableReadWriteAddDelete<SnmpGroupEntry> snmp_group_entry(&ns_conf, "groups");
static TableReadWriteAddDelete<SnmpAccessEntry> snmp_accesses_entry(&ns_conf, "accesses");
static TableReadWriteAddDelete<SnmpViewEntry> snmp_view_entry(&ns_conf, "views");
static TableReadWriteAddDelete<SnmpTrapReceiverEntry> snmp_trap_receiver_entry(&ns_conf, "trap_receiver");
static TableReadWriteAddDelete<SnmpTrapSourceEntry> snmp_trap_source_entry(&ns_conf, "trap_source");
