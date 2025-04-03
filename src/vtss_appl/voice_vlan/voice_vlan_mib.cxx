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

#include "voice_vlan_serializer.hxx"

VTSS_MIB_MODULE("voiceVlanMib", "VOICE-VLAN", vtss_appl_voice_vlan_mib_init, VTSS_MODULE_ID_VOICE_VLAN, root, h)
{
    h.add_history_element("201409160000Z", "Initial version.");
    h.add_history_element("201503250000Z", "Change syntax type of VoiceVlanConfigGlobalsMgmtVlanId.");
    h.add_history_element("201508250000Z", "Replace the SYNTAX of vtssVoiceVlanConfigOuiTableRowEditorPrefix from 'OCTET STRING (SIZE(3..3))' "
                          "to 'OCTET STRING (SIZE(3))' according to RFC2578.");

    h.description("This is a private version of the Voice VLAN MIB.");
}
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace expose::snmp;

namespace vtss
{
namespace appl
{
namespace voice_vlan
{
namespace interfaces
{

/* Construct the MIB Objects hierarchy */
/*
    xxxVoiceVlanMIBObjects  +-  xxxVoiceVlanCapabilities
                            +-  xxxVoiceVlanConfig  -> ...

    xxxVoiceVlanMIBConformance will be generated automatically
*/
/* root: parent node (See VTSS_MIB_MODULE) */
NS(voice_vlan_mib_objects, root, 1, "voiceVlanMibObjects");

/* xxxVoiceVlanCapabilities */
static StructRO2<VoiceVlanCapabilitiesLeaf> voice_vlan_capabilities_leaf(
    &voice_vlan_mib_objects,
    vtss::expose::snmp::OidElement(1, "voiceVlanCapabilities"));

/* xxxVoiceVlanConfig */
NS(voice_vlan_config,               voice_vlan_mib_objects, 2,  "voiceVlanConfig");

/* xxxVoiceVlanConfig:xxxVoiceVlanConfigGlobals */
NS(voice_vlan_config_globals,       voice_vlan_config,      1,  "voiceVlanConfigGlobals");

/* xxxVoiceVlanConfig:xxxVoiceVlanConfigInterface */
NS(voice_vlan_config_interface,     voice_vlan_config,      2,  "voiceVlanConfigInterface");

/* xxxVoiceVlanConfig:xxxVoiceVlanConfigOui */
NS(voice_vlan_config_oui,           voice_vlan_config,      3,  "voiceVlanConfigOui");

/* xxxVoiceVlanConfigGlobalsMgmt */
static StructRW2<VoiceVlanGlobalsMgmt> voice_vlan_globals_mgmt(
    &voice_vlan_config_globals,
    vtss::expose::snmp::OidElement(1, "voiceVlanConfigGlobalsMgmt"));

/* xxxVoiceVlanConfigInterfacePortTable */
static TableReadWrite2<VoiceVlanPortTable> voice_vlan_port_table(
    &voice_vlan_config_interface,
    vtss::expose::snmp::OidElement(1, "voiceVlanConfigInterfacePortTable"));

/* xxxVoiceVlanConfigOuiTable */
static TableReadWriteAddDelete2<VoiceVlanOuiTable> voice_vlan_oui_table(
    &voice_vlan_config_oui,
    vtss::expose::snmp::OidElement(1, "voiceVlanConfigOuiTable"),
    vtss::expose::snmp::OidElement(2, "voiceVlanConfigOuiTableRowEditor"));

}  // namespace interfaces
}  // namespace voice_vlan
}  // namespace appl
}  // namespace vtss
