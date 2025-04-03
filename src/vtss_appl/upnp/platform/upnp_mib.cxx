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

#include "upnp_serializer.hxx"
#include "vtss/appl/upnp.h"

VTSS_MIB_MODULE("upnpMib", "UPNP", upnp_mib_init, VTSS_MODULE_ID_UPNP, root,
                h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201410100000Z", "Editorial changes");
    h.add_history_element("201508170000Z", "Add parameters: IpAddressingMode/IpInterfaceId");
    h.description("This is a private version of UPnP");
}

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace upnp {
namespace interfaces {
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(upnp_mib_objects, root, 1, "upnpMibObjects");
NS(upnp_config, upnp_mib_objects, 2, "upnpConfig");

static StructRO2<UpnpCapabilitiesLeaf> upnp_capabilities_leaf(
        &upnp_mib_objects, vtss::expose::snmp::OidElement(1, "upnpCapabilities"));
static StructRW2<UpnpParamsLeaf> upnp_params_leaf(
        &upnp_config, vtss::expose::snmp::OidElement(1, "upnpConfigGlobals"));
}  // namespace interfaces
}  // namespace upnp
}  // namespace appl
}  // namespace vtss
