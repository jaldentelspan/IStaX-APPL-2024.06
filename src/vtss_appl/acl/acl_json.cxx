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

#include "misc_api.h"
#include "acl_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::acl::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_acl("acl");
extern "C" void vtss_appl_acl_json_init() { json_node_add(&ns_acl); }

NS(ns_conf,             ns_acl,    "config");
NS(ns_conf_ace,         ns_conf,   "ace");
NS(ns_status,           ns_acl,    "status");
NS(ns_status_ace,       ns_status, "ace");
NS(ns_status_interface, ns_status, "interface");
NS(ns_control,          ns_acl,    "control");

static StructReadOnly<AclCapabilitiesLeaf> acl_capabilities_leaf(
        &ns_acl, "capabilities");

static TableReadWrite<AclConfigRateLimiterEntry> acl_config_rate_limiter_entry(
        &ns_conf, "ratelimiter");

static TableReadWriteAddDelete<AclConfigAceEntry> acl_config_ace_entry(
        &ns_conf_ace, "config");

static TableReadOnly<AclConfigAcePrecedenceEntry> acl_config_ace_precedence_entry(
        &ns_conf_ace, "precedence");

static TableReadWrite<AclConfigInterfaceEntry> acl_config_interface_entry(
        &ns_conf, "interface");

static TableReadOnly<AclStatusAceEntry> acl_status_ace_entry(
        &ns_status_ace, "status");

static TableReadOnly<AclStatusAceHitCountEntry> acl_status_ace_hit_count_entry(
        &ns_status_ace, "hitCount");

static TableReadOnly<AclStatusInterfaceHitCountEntry> acl_status_interface_hit_count_entry(
        &ns_status_interface, "hitCount");

static StructWriteOnly<AclControlGlobalsLeaf> acl_control_globals_leaf(
        &ns_control, "global");

static TableReadWrite<AclControlInterfaceEntry> acl_control_interface_entry(
        &ns_control, "interface");

/* JSON notification */
static TableReadOnlyNotification<AclStatusAceEventEntry> acl_status_ace_event_entry(
        &ns_status_ace, "crossedThreshold", &acl_status_ace_event_update);

