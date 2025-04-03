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

#include "firmware_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::firmware::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_firmware("firmware");
extern "C" void vtss_appl_firmware_json_init() { json_node_add(&ns_firmware); }

NS(ns_status, ns_firmware, "status");
static TableReadOnly<FirmwareStatusImageEntry> firmware_status_image_entry(&ns_status, "image");
static StructReadOnly<FirmwareStatusImageUploadLeaf> firmware_status_image_upload_leaf(&ns_status, "imageUpload");
static TableReadOnly<FirmwareImageStatusTlvLeaf> firmware_status_image_tlv_leaf(&ns_status, "imageTlv");
static TableReadOnly<FirmwareStatusSwitchEntry> firmware_status_switch_entry(&ns_status, "switch");

NS(ns_control, ns_firmware, "control");
static StructWriteOnly<FirmwareControlGlobalsLeaf> firmware_control_globals_leaf(&ns_control, "global");
static StructWriteOnly<FirmwareControlCopyConfigLeaf> firmware_control_copy_config_leaf(&ns_control, "imageUpload");
