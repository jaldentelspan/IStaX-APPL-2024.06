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

#include "fan_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::fan::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_fan("fan");
extern "C" void vtss_appl_fan_json_init() { json_node_add(&ns_fan); }

NS(ns_conf,       ns_fan,  "config");
NS(ns_status,     ns_fan,  "status");

namespace vtss {
namespace appl {
namespace fan {
namespace interfaces {

static StructReadOnly<FanCapabilities> fan_capabilities(
                &ns_fan, "capabilities");

static StructReadWrite<FanConfigEntry> fan_config_entry(
                &ns_conf, "global");

static TableReadOnly<FanStatusSpeedEntry> fan_status_speed_entry(
                &ns_status, "speed");

static TableReadOnly<FanStatusSensorEntry> fan_status_sensor_entry(
                &ns_status, "sensor");

}  // namespace interfaces
}  // namespace eee
}  // namespace appl
}  // namespace vtss
