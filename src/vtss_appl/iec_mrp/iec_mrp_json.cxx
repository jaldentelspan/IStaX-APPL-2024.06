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

#include "iec_mrp_serializer.hxx"
#include "iec_mrp_expose.hxx"
//#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::iec_mrp::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_iec_mrp("iecMrp");
extern "C" void iec_mrp_json_init()
{
    json_node_add(&ns_iec_mrp);
}

NS(ns_control,      ns_iec_mrp,  "control");

static StructReadOnly<IecMrpCapabilities> iec_mrp_capabilities(&ns_iec_mrp, "capabilities");
static TableReadWriteAddDelete<IecMrpConfTable> iec_mrp_conf_table(&ns_iec_mrp, "config");
static TableReadOnly<IecMrpStatusTable> iec_mrp_status_table(&ns_iec_mrp, "status");

static TableReadWrite<IecMrpControlStatisticsClearTable> iec_mrp_control_statistics_clear_table(
    &ns_control, "statisticsClear");

static TableReadOnlyNotification<IecMrpStatusNotificationTable> iec_mrp_status_notification(&ns_iec_mrp, "notification", &mrp_notification_status);

