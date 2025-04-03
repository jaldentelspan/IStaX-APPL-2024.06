/*
 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vlan_translation_api.h"
#include "vlan_translation_serializer.hxx"

VTSS_MIB_MODULE("VlanTranslationMib", "VLAN-TRANSLATION", vtss_vlan_trans_mib_init, VTSS_MODULE_ID_VLAN_TRANSLATION, root, h)
{
    h.add_history_element("201406300000Z", "Initial version");
    h.add_history_element("201710250000Z", "Added unidirectional translation");
    h.description("Private VLAN TRANSLATION MIB.");
}

using namespace vtss;
using namespace vtss::expose::snmp;
using namespace vtss::appl::vlan_translation::interfaces;

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

// Parent: vtss -------------------=--------------------------------------------------
NS(ns_vlan_trans, root, 1, "VlanTranslationMibObjects");

// Parent: vtss/vlan_trans -----------------------------------------------------------
NS(ns_config, ns_vlan_trans, 2, "VlanTranslationConfig");

// Parent: vtss/vlan_trans/config ----------------------------------------------------
NS(ns_conf_translation, ns_config, 1, "VlanTranslationConfigTranslation");
NS(ns_conf_interfaces, ns_config, 2, "VlanTranslationConfigInterfaces");

// Parent: vtss/vlan_trans -----------------------------------------------------------
static StructRO2<GlobalCapabilities> ro_global_capabilities(
    &ns_vlan_trans, expose::snmp::OidElement(1, "VlanTranslationCapabilities"));

// Parent: vtss/vlan_trans/config/translation ----------------------------------------
static TableReadWriteAddDelete2<VTEntry> entry_translation(
    &ns_conf_translation, OidElement(1, "VlanTranslationConfigTranslationTable"),
    OidElement(2, "VlanTranslationConfigTranslationRowEditor"));

// Parent: vtss/vlan_trans/config/interfaces -----------------------------------------
static TableReadWrite2<IfTable> if_table(
    &ns_conf_interfaces, OidElement(1, "VlanTranslationConfigInterfacesIfTable"));

