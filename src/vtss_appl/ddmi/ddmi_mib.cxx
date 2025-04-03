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

#include "ddmi_serializer.hxx"
#include "vtss/basics/enum-descriptor.h"    // vtss_enum_descriptor_t

#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("ddmiMib", "DDMI", ddmi_mib_init, VTSS_MODULE_ID_DDMI, root, h)
{
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201410100000Z", "Editorial changes");
    h.add_history_element("201711270000Z", "Add IfIndex to DdmiStatusInterfaceTableInfoGroup. Editorial changes");
    h.add_history_element("202102240000Z", "Renamed a few parameters and added alarm state");
    h.description("This is a private version of DDMI");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(objects, root, 1, "ddmiMibObjects");
NS(ddmi_config, objects, 2, "ddmiConfig");
NS(ddmi_status, objects, 3, "ddmiStatus");

namespace vtss
{
namespace appl
{
namespace ddmi
{
namespace interfaces
{
static StructRW2<ddmiConfigGlobalsLeaf> ddmi_config_globals_leaf(&ddmi_config, vtss::expose::snmp::OidElement(1, "ddmiConfigGlobals"));
static TableReadOnly2<ddmiStatusInterfaceEntry> ddmi_status_interface_entry(&ddmi_status, vtss::expose::snmp::OidElement(2, "ddmiStatusInterfaceTable"));
}  // namespace interfaces
}  // namespace ddmi
}  // namespace appl
}  // namespace vtss

