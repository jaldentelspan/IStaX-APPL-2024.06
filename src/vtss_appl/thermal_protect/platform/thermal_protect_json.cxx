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

#include "thermal_protect_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::thermal_protect::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_thermal_protect("thermalProtect");
extern "C" void vtss_appl_thermal_protect_json_init() { json_node_add(&ns_thermal_protect); }

NS(ns_conf,        ns_thermal_protect, "config");
NS(ns_status,     ns_thermal_protect, "status");

static StructReadOnly<ThermalProtectionCapabilitiesEntry> thermal_protection_capabilities_entry(&ns_thermal_protect, "capabilities");
static TableReadWrite<ThermalProtectPriorityTempEntry> thermal_protect_priority_temp_entry(&ns_conf, "global");
static TableReadWrite<ThermalProtectInterfacePriorityEntry> thermal_protect_interface_priority_entry(&ns_conf, "interface");
static TableReadOnly<ThermalProtectionInterfaceStatusEntry> thermal_protection_interface_status_entry(&ns_status, "interface");

