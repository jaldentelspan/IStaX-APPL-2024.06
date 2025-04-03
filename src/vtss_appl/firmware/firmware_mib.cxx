/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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


using namespace vtss;
using namespace expose::snmp;
VTSS_MIB_MODULE("firmwareMib", "FIRMWARE", firmware_mib_init, VTSS_MODULE_ID_FIRMWARE , root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201410100000Z", "Editorial changes");
    h.add_history_element("201412160000Z", "Add switch table");
    h.add_history_element("201605310000Z", "Enhanced firmware upload status messages");
    h.add_history_element("201702030000Z", "Replaced firmware upload status message 'Incorrect image version' with 'Incompatible target'");
    h.add_history_element("201807020000Z", "Added 'SaveSshHostKeys' object to 'ControlImageUpload' table.");
    h.add_history_element("201807060000Z", "Added 'FtpActiveMode' object to 'ControlImageUpload' table and new 'StatusEnum' error codes.");
    h.description("This is a private version of Firmware");
}

namespace vtss {
namespace appl {
namespace firmware {
namespace interfaces {
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(firmware_mib_objects, root, 1, "firmwareMibObjects");
NS(firmware_status, firmware_mib_objects, 3, "firmwareStatus");
NS(firmware_control, firmware_mib_objects, 4, "firmwareControl");

static TableReadOnly2<FirmwareStatusImageEntry> firmware_status_image_entry(
        &firmware_status, vtss::expose::snmp::OidElement(1, "firmwareStatusImageTable"));

static StructRO2<FirmwareStatusImageUploadLeaf> firmware_status_image_upload_leaf(
        &firmware_status, vtss::expose::snmp::OidElement(2, "firmwareStatusImageUpload"));

static TableReadOnly2<FirmwareStatusSwitchEntry> firmware_status_switch_entry(
        &firmware_status, vtss::expose::snmp::OidElement(3, "firmwareStatusSwitchTable"));

static StructRW2<FirmwareControlGlobalsLeaf> firmware_control_globals_leaf(
        &firmware_control, vtss::expose::snmp::OidElement(1, "firmwareControlGlobals"));

static StructRW2<FirmwareControlCopyConfigLeaf> firmware_control_copy_config_leaf(
        &firmware_control, vtss::expose::snmp::OidElement(2, "firmwareControlImageUpload"));
}  // namespace interfaces
}  // namespace firmware
}  // namespace appl
}  // namespace vtss

