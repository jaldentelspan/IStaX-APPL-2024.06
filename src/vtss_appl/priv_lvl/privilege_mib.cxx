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

#include "privilege_serializer.hxx"
#include "vtss/basics/enum-descriptor.h"  // vtss_enum_descriptor_t

using namespace vtss;
using namespace expose::snmp;
VTSS_MIB_MODULE("privilegeMib", "PRIVILEGE", privilege_mib_init,
                VTSS_MODULE_ID_PRIV_LVL, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.description("This is a private version of Privilege");
}


namespace vtss {
namespace appl {
namespace privilege {
namespace interfaces {
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(privilege_mib_objects, root, 1, "privilegeMibObjects");
NS(privilege_config, privilege_mib_objects, 2, "privilegeConfig");

static TableReadWrite2<PrivilegeConfigWebEntry> privilege_config_web_entry(
        &privilege_config, vtss::expose::snmp::OidElement(1, "privilegeConfigWeb"));
}  // namespace interfaces
}  // namespace privilege
}  // namespace appl
}  // namespace vtss

