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

#include "udld_serializer.hxx"
#include "vtss/appl/udld.h"

VTSS_MIB_MODULE("UdldMib", "UDLD", Udld_mib_init, VTSS_MODULE_ID_UDLD, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.description("This is a private MIB for udld");
}


using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace udld {
namespace interfaces {
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(udld_obj, root, 1, "UdldMibObjects");
NS(udld_config, udld_obj, 2, "UdldConfig");
NS(udld_config_if, udld_config, 1, "UdldConfigInterface");
NS(udld_status, udld_obj, 3, "UdldStatus");
NS(udld_status_if, udld_status, 1, "UdldStatusInterface");

static TableReadWrite2<UdldPortParamsTable> udld_port_params_table(
        &udld_config_if, vtss::expose::snmp::OidElement(1, "UdldConfigInterfaceParamTable"));

static TableReadOnly2<UdldPortInterfaceStatusTable> udld_port_interface_status_table(
        &udld_status_if, vtss::expose::snmp::OidElement(1, "UdldStatusInterfaceTable"));

static TableReadOnly2<UdldPortNeighborStatusTable> udld_port_neighbor_status_table(
        &udld_status_if, vtss::expose::snmp::OidElement(2, "UdldStatusInterfaceNeighborTable"));

}  // namespace interfaces
}  // namespace udld
}  // namespace appl
}  // namespace vtss
