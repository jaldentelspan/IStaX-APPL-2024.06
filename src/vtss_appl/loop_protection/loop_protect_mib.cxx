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

#include "loop_protect_serializer.hxx"
#include "vtss/appl/loop_protect.h"

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("LoopProtectionMib", "LOOP-PROTECTION", LoopProtection_mib_init,
                VTSS_MODULE_ID_LOOP_PROTECT, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.description("This is a private MIB for loop protection");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(lprot_obj, root, 1, "LoopProtectionMibObjects");
NS(lprot_config, lprot_obj, 2, "LoopProtectionConfig");
NS(lprot_config_if, lprot_config, 2, "LoopProtectionConfigInterface");
NS(lprot_status, lprot_obj, 3, "LoopProtectionStatus");
NS(lprot_status_if, lprot_status, 2, "LoopProtectionStatusInterface");

namespace vtss {
namespace appl {
namespace loop_protect {
namespace interfaces {
static StructRW2<LprotGlobalsLeaf> lprot_globals_leaf(
        &lprot_config,
        vtss::expose::snmp::OidElement(1, "LoopProtectionConfigGlobals"));
static TableReadWrite2<LprotPortParamsTable> lprot_port_params_table(
        &lprot_config_if,
        vtss::expose::snmp::OidElement(1, "LoopProtectionConfigInterfaceParamTable"));
static TableReadOnly2<LprotPortStatusTable> lprot_port_status_table(
        &lprot_status_if,
        vtss::expose::snmp::OidElement(1, "LoopProtectionStatusInterfaceTable"));
}  // namespace interfaces
}  // namespace loop_protect
}  // namespace appl
}  // namespace vtss

