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

/**
 * \file acl_mib.cxx
 * \brief This file defines the private MIB for the ACL module
 */
#include "misc_api.h"
#include "acl_serializer.hxx"
#include "vtss/appl/acl.h"


using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("aclMib", "ACL", acl_mib_init, VTSS_MODULE_ID_ACL, root, h) {
    h.add_history_element("201408050000Z", "Initial version");
    h.add_history_element("201411130000Z", "Add ACE Ingress parameters");
    h.add_history_element("201501120000Z", "Add ACE parameters for stacking device");
    h.add_history_element("201904050000Z", "Obsoleted EvcPolicerId");
    h.description("This is a private version of ACL");
}

namespace vtss {
namespace appl {
namespace acl {
namespace interfaces {

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(acl_mib_objects,       root,            1, "AclMibObjects");
NS(acl_config,            acl_mib_objects, 2, "AclConfig");
NS(acl_config_interface,  acl_config,      2, "AclConfigInterface");
NS(acl_config_ace,        acl_config,      4, "AclConfigAce");
NS(acl_status,            acl_mib_objects, 3, "AclStatus");
NS(acl_status_interface,  acl_status,      2, "AclStatusInterface");
NS(acl_status_ace,        acl_status,      3, "AclStatusAce");
NS(acl_control,           acl_mib_objects, 4, "AclControl");
NS(acl_control_interface, acl_control,     2, "AclControlInterface");

static StructRO2<AclCapabilitiesLeaf> acl_capabilities_leaf(
        &acl_mib_objects, vtss::expose::snmp::OidElement(1, "AclCapabilities"));

static TableReadWrite2<AclConfigRateLimiterEntry> acl_config_rate_limiter_entry(
        &acl_config, vtss::expose::snmp::OidElement(3, "AclConfigRateLimiter"));

static TableReadWriteAddDelete2<AclConfigAceEntry> acl_config_ace_entry(
        &acl_config_ace, vtss::expose::snmp::OidElement(1, "AclConfigAce"), vtss::expose::snmp::OidElement(2, "AclConfigAceRowEditor"));

static TableReadOnly2<AclConfigAcePrecedenceEntry> acl_config_ace_precedence_entry(
        &acl_config_ace, vtss::expose::snmp::OidElement(3, "AclConfigAcePrecedence"));

static TableReadWrite2<AclConfigInterfaceEntry> acl_config_interface_entry(
        &acl_config_interface, vtss::expose::snmp::OidElement(1, "AclConfigInterfaceConfig"));

static TableReadOnly2<AclStatusAceEntry> acl_status_ace_entry(
        &acl_status_ace, vtss::expose::snmp::OidElement(1, "AclStatusAce"));

static TableReadOnly2<AclStatusAceHitCountEntry> acl_status_ace_hit_count_entry(
        &acl_status_ace, vtss::expose::snmp::OidElement(2, "AclStatusAceHitCount"));

static TableReadOnly2<AclStatusInterfaceHitCountEntry> acl_status_interface_hit_count_entry(
        &acl_status_interface, vtss::expose::snmp::OidElement(1, "AclStatusInterfaceHitCount"));

static StructRW2<AclControlGlobalsLeaf> acl_control_globals_leaf(
        &acl_control, vtss::expose::snmp::OidElement(1, "AclControlGlobals"));

static TableReadWrite2<AclControlInterfaceEntry> acl_control_interface_entry(
        &acl_control_interface, vtss::expose::snmp::OidElement(1, "AclControlInterfaceControl"));

}  // namespace interfaces
}  // namespace acl
}  // namespace appl
}  // namespace vtss
