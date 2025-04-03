/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __POE_SERIALIZER_HXX__
#define __POE_SERIALIZER_HXX__

#include "vtss/basics/snmp.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/poe.h"
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_common_iterator.hxx"

extern const vtss_enum_descriptor_t vtss_appl_poe_management_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poe_management_mode_t, "poeMgmtMode",
                         vtss_appl_poe_management_mode_txt,
                         "This enumeration defines the types of PoE management mode.");

extern const vtss_enum_descriptor_t vtss_appl_poebt_port_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poebt_port_type_t, "poebtPowerType",
                         vtss_appl_poebt_port_type_txt,
                         "This enumeration defines the types of port PoE type.");

extern const vtss_enum_descriptor_t vtss_appl_poebt_port_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poe_port_mode_t, "poebtMode",
                         vtss_appl_poebt_port_mode_txt,
                         "This enumeration defines the types of port PoE mode.");

extern const vtss_enum_descriptor_t vtss_appl_poebt_port_pm_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poebt_port_pm_t, "poebtPowerManagement",
                         vtss_appl_poebt_port_pm_txt,
                         "This enumeration defines the types of port PoE PM.");

extern const vtss_enum_descriptor_t vtss_appl_poe_status_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poe_status_t, "poePortStatus",
                         vtss_appl_poe_status_txt,
                         "This enumeration define the status of the poe port.");

extern const vtss_enum_descriptor_t vtss_appl_poe_port_power_priority_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poe_port_power_priority_t, "poePowerPriority",
                         vtss_appl_poe_port_power_priority_txt,
                         "This enumeration defines the port power priority.");

extern const vtss_enum_descriptor_t vtss_appl_poe_port_lldp_disable_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poe_port_lldp_disable_t, "poeLldpDisable",
                         vtss_appl_poe_port_lldp_disable_txt,
                         "This enumeration defines the port disable status.");

extern const vtss_enum_descriptor_t vtss_appl_poe_port_cable_length_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poe_port_cable_length_t, "poeCableLength",
                         vtss_appl_poe_port_cable_length_txt,
                         "This enumeration defines the port cable length.");

extern const vtss_enum_descriptor_t vtss_appl_poe_led_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poe_led_t, "poeLedColor",
                         vtss_appl_poe_led_txt,
                         "This enumeration defines the led color.");

extern const vtss_enum_descriptor_t vtss_appl_poe_pd_structure_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_poe_pd_structure_t, "poePdStructure",
                         vtss_appl_poe_pd_structure_txt,
                         "This enumeration defines the PD structure.");
/*****************************************************************************
  - MIB row/table indexes serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(POE_ifindex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

VTSS_SNMP_TAG_SERIALIZE(POE_usid_index, vtss_usid_t, a, s) {
    a.add_leaf(s.inner, vtss::tag::Name("SwitchId"),
               vtss::expose::snmp::RangeSpec<uint32_t>(1, 16),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("User Switch Id."));
}

template <typename T>
void serialize(T &a, vtss_appl_poe_psu_capabilities_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_poe_psu_capabilities_t"));
    int ix = 0;
    m.add_leaf(vtss::AsBool(s.user_configurable), vtss::tag::Name("userConfigurable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether the user can and need to specify the amount "
                   "of power the PSU can deliver"));
    m.add_leaf(s.max_power_w, vtss::tag::Name("maxPower"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the max power in Watt the PoE board can handle. No "
                   "reason to use a bigger PSU than this. For systems with internal PSU, "
                   "this is the size of the built-in PSU."));
    m.add_leaf(s.system_reserved_power_w, vtss::tag::Name("systemReservedPower"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "For systems where the switch itself is powered by the same PSU as "
                   "used for the PoE functionality, this shall reflect the amount of "
                   "power required to keep the switch operating."));
    m.add_leaf(vtss::AsBool(s.legacy_mode_configurable), vtss::tag::Name("legacyModeConfigurable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether the PSE supports detection of legacy PD devices, and "
                   "that the user can configure this feature."));
    m.add_leaf(vtss::AsBool(s.interruptible_power_supported),
               vtss::tag::Name("interruptiblePowerSupported"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether switch can reset software without affecting PoE powered "
                   "devices connected to the switch."));
    m.add_leaf(vtss::AsBool(s.pd_auto_class_request),
               vtss::tag::Name("autoClass"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether Auto class is active "));
     m.add_leaf(vtss::AsBool(s.legacy_pd_class_mode),
               vtss::tag::Name("legacyPdClassMode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether legacy pd class mode mode is selected "));
     m.add_leaf(vtss::AsBool(s.is_bt),
               vtss::tag::Name("isBt"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether poe firmware is bt or at "));
}

template <typename T>
void serialize(T &a, vtss_appl_poe_port_capabilities_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_poe_port_capabilities_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.poe_capable), vtss::tag::Name("PoE"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Indicates whether interface is PoE capable or not."));
}

template <typename T>
void serialize(T &a, vtss_appl_poe_conf_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_poe_conf_t"));
    int ix = 0;
    m.add_leaf(s.power_supply_max_power_w, vtss::tag::Name("MaxPower"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The Maximum power(in Watt) "
                   "that the power sourcing equipment(such as switch) can deliver. "
                   "This value can only be configured if the CapabilitiesPsuUserConfigurable object is true. "
                   "If the Maximum power is configurable, the valid range is from 0 to the value of the CapabilitiesPsuMaxPower object."
                   ));

    m.add_leaf(s.system_power_usage_w, vtss::tag::Name("SystemPwrUsage"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The power which reserved to pwer the system (in Watt) "
                   "If the Maximum power is configurable, the valid range is from 0 to the value of the CapabilitiesPsuMaxPower object."
                   ));

    m.add_leaf(vtss::AsBool(s.capacitor_detect), vtss::tag::Name("CapacitorDetection"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether switch capacitor detection feature is enabled or not."));

    m.add_leaf(vtss::AsBool(s.interruptible_power), vtss::tag::Name("InterruptiblePower"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether software reset of switch shall affect PoE powered "
                   "devices connected to the switch."));

    m.add_leaf(vtss::AsBool(s.pd_auto_class_request), vtss::tag::Name("PDAutoClassRequest"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether PDAutoClassRequest mode is active. "));

    m.add_leaf(s.global_legacy_pd_class_mode, vtss::tag::Name("LegacyPdClassMode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates which legacy pd class mode is selected. "));
}

template <typename T>
void serialize(T &a, vtss_appl_poe_port_conf_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_poe_port_conf_t"));
    int ix = 0;

    m.add_leaf(s.bt_pse_port_type, vtss::tag::Name("Type"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Set PoE Type. "
                   ));

    m.add_leaf(s.poe_port_mode, vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Set PoE mode or disable PoE feature, two PoE modes are supported. "
                   "POE: Enables PoE based on IEEE 802.3af standard, "
                   "and provides power up to 15.4W(154 deciwatt) of DC power to powered device. When changing to standard mode the MaxPower is automatically adjust to 15.4 W, if it currently exceeds 15.4 W. "
                   "POE_PLUS: Enabled PoE based on IEEE 802.3at standard, "
                   "and provides power up to 30W(300 deciwatt) of DC power to powered device."));

    m.add_leaf(s.bt_port_pm, vtss::tag::Name("PwrMng"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Set PoE port power management. "
                   ));

    m.add_leaf(s.priority, vtss::tag::Name("Priority"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Set port power priority. "
                   "Priority determines the order in which the interfaces will receive power. "
                   "Interfaces with a higher priority will receive power before interfaces with a lower priority. "
                   "PRIORITY_LOW means lowest priority. "
                   "PRIORITY_HIGH means medium priority. "
                   "PRIORITY_CRITICAL means highest priority."));

    m.add_leaf(s.lldp_disable, vtss::tag::Name("Lldp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Set port lldp awareness. "
                   "If this value is disable, the PSE will ignore PoE related parts of received LLDP frames."));

    m.add_leaf(s.cable_length, vtss::tag::Name("CableLength"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Set port cable length. "
                   "length in meters x10 -> 0 to 10 "));
}

template <typename T>
void serialize(T &a, vtss_appl_poe_port_status_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_poe_port_status_t"));
    int ix = 0;
    m.add_leaf(s.assigned_pd_class_a, vtss::tag::Name("PDClassAltA"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Powered device(PD) negotiates a power class with sourcing equipment(PSE) "
                   "during the time of initial connection, each class have a maximum supported power. "
                   "Class assigned to PD alternative A is based on PD electrical characteristics. "
                   "Value -1 means either PD attached to the interface can not advertise its power class "
                   "or no PD detected or PoE is not supported or "
                   "PoE feature is disabled or unsupported PD class(classes 0-4 is supported)."));

    m.add_leaf(s.assigned_pd_class_b, vtss::tag::Name("PDClassAltB"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Powered device(PD) negotiates a power class with sourcing equipment(PSE) "
                   "during the time of initial connection, each class have a maximum supported power. "
                   "Class assigned to PD alternative B is based on PD electrical characteristics. "
                   "Value -1 means either PD attached to the interface can not advertise its power class "
                   "or no PD detected or PoE is not supported or "
                   "PoE feature is disabled or unsupported PD class(classes 0-4 is supported)."));

    m.add_leaf(s.pd_status, vtss::tag::Name("CurrentState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicate port status. "
                   "unknownState: PD state unknown"
                   "budgetExceeded: PoE is turned OFF due to power budget exceeded on PSE. "
                   "noPoweredDeviceDetected: No PD detected. "
                   "poweredDeviceOn: PSE supplying power to PD through PoE. "
                   "poweredDeviceOverloaded: PD consumes more power than the maximum limit configured on the PSE port."
                   "notSupported: PoE not supported"
                   "disabled: PoE is disabled for the interface"
                   "disabledInterfaceShutdown: PD is powered down due to interface shut-down"
                   "pdFault: pd fault"
                   "pseFault: pse fault"));

    m.add_leaf(s.cfg_pse_port_type, vtss::tag::Name("PSEType"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Type of power sourcing equipment(PSE) according to 802.3bt. "
                   "A PSE type 1 supports PD class 1-3 PDs, "
                   "a PSE type 2 supports PD class 1-4 PDs, "
                   "a PSE type 3 supports PD class 1-6 PDs, "
                   "a PSE type 4 supports PD class 1-8 PDs." ));

    m.add_leaf(s.pd_structure, vtss::tag::Name("PDStructure"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "notPerformed: Test not yet performed, "
                   "open: No device found, "
                   "invalidSignature: No valid signature found, "
                   "ieee4PairSingleSig: four pair single signature PD detected, "
                   "legacy4PairSingleSig: four pair single signature Microsemi legacy PD detected, "
                   "ieee4PairDualSig: four pair dual signature PD detected, "
                   "p2p4CandidateFalse: TBD, "
                   "ieee2Pair: two pair PD detected, "
                   "legacy2Pair: two pair Microsemi legacy PD detected."));

    m.add_leaf(s.power_requested_mw, vtss::tag::Name("PowerRequested"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the power limit (in milliwatt) given by the class of the PD."));

    m.add_leaf(s.power_assigned_mw, vtss::tag::Name("PowerAllocated"),       
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "The power (in deciwatt) reserved for the PD. When power is allocated on "
                   "basis of PD class, "
                   "this number will be equal to the consumed power. When LLDP is used to "
                   "allocated power, this will be the amount of power reserved through LLDP. "
                   "The value is only meaningful when the PD is on."));

    m.add_leaf(s.power_consume_mw, vtss::tag::Name("PowerConsumption"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the power(in milliwatt) that the PD is consuming right now."));

    m.add_leaf(s.current_consume_ma, vtss::tag::Name("CurrentConsumption"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the current(in mA) that the PD is consuming right now."));

    m.add_leaf(s.bt_port_counters.udl_count, vtss::tag::Name("UdlCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the total UDL counter."));

    m.add_leaf(s.bt_port_counters.ovl_count, vtss::tag::Name("OvlCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the total OVL counter."));

    m.add_leaf(s.bt_port_counters.sc_count, vtss::tag::Name("ShortCircuitCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the total short circuit counter."));

    m.add_leaf(s.bt_port_counters.invalid_signature_count, vtss::tag::Name("InvalidSignatureCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the total invalid signature counter."));

    m.add_leaf(s.bt_port_counters.power_denied_count, vtss::tag::Name("PowerDeniedCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the total power denied_counter."));
}

template <typename T>
void serialize(T &a, vtss_appl_poe_powerin_status_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_poe_powerin_status_t"));
    int ix = 0;
    m.add_leaf(
        vtss::AsBool(s.pwr_in_status1),
        vtss::tag::Name("PowerInStatus1"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The current status of 1st power supplies (ON/OFF)."
            "true  means power supply is available (ON)"
            "false means power supply is not available (OFF)"
                               ));

    m.add_leaf(
        vtss::AsBool(s.pwr_in_status2),
        vtss::tag::Name("PowerInStatus2"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The current status of 2nd power supplies (ON/OFF)."
            "true  means power supply is available (ON)"
            "false means power supply is not available (OFF)"
                               ));

    m.add_leaf(s.power_consumption_w, vtss::tag::Name("TotalPowerConsumption"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the total power(in watts) that is consumed right now."));

    m.add_leaf(s.calculated_total_current_used_ma, vtss::tag::Name("TotalCurrentConsumption"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the total current power(in mA) that is consumed right now."));

    m.add_leaf(s.calculated_power_w, vtss::tag::Name("TotalPowerAllocated"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the total power reserved by the PDs."));
}

template <typename T>
void serialize(T &a, vtss_appl_poe_powerin_led_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_poe_powerin_led_t"));
    int ix = 0;
    m.add_leaf(
        s.pwr_led1,
        vtss::tag::Name("PowerInLed1"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The current led color for power supply 1"
                               )
               );
    m.add_leaf(
        s.pwr_led2,
        vtss::tag::Name("PowerInLed2"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The current led color for power supply 2"
                               )
               );
}

template <typename T>
void serialize(T &a, vtss_appl_poe_status_led_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_poe_status_led_t"));
    int ix = 0;
    m.add_leaf(
        s.status_led,
        vtss::tag::Name("StatusLed"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The current led color indicating PoE status"
                               )
               );
}

namespace vtss {
namespace appl {
namespace poe {
namespace interfaces {

struct poeSwitchConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_usid_t>,
            vtss::expose::ParamVal<vtss_appl_poe_conf_t *>> P;

    static constexpr const char *table_description =
            "This is a table to configure PoE configurations for a switch.";

    static constexpr const char *index_description =
            "Each switch has a set of PoE configurable parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, POE_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_poe_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_poe_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_switch);
    VTSS_EXPOSE_SET_PTR(vtss_appl_poe_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_POE);
};

struct poeInterfaceConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_poe_port_conf_t *>> P;

    static constexpr const char *table_description =
            "This is a table to configure PoE configurations for a specific "
            "interface.";

    static constexpr const char *index_description =
            "Each interface has a set of PoE configurable parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, POE_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_poe_port_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_poe_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_poe_front_port_exist);
    VTSS_EXPOSE_SET_PTR(vtss_appl_poe_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_POE);
};

struct poeInterfaceStatusEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_poe_port_status_t *>> P;

    static constexpr const char *table_description =
            "This is a table to Power over Ethernet interface status";

    static constexpr const char *index_description =
            "Each interface has a set of status parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, POE_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_poe_port_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_poe_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_poe_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_POE);
};

struct poeInterfaceCapabilitiesEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_poe_port_capabilities_t *>> P;

    static constexpr const char *table_description =
            "This is a table to interface capabilities";

    static constexpr const char *index_description =
            "Each interface has a set of capability parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, POE_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_poe_port_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_poe_port_capabilities_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_POE);
};

struct poeInterfacePowerInStatusEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_poe_powerin_status_t *>
        > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_poe_powerin_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_poe_powerin_status_get);
};

struct poeInterfacePowerLedStatusEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_poe_powerin_led_t *>
        > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_poe_powerin_led_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_poe_powerin_led_get);
};

struct poeInterfaceStatusLedEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_poe_status_led_t *>
        > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_poe_status_led_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_poe_status_led_get);
};

struct poePsuCapabilitiesEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_poe_psu_capabilities_t *>
        > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_poe_psu_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_poe_psu_capabilities_get);
};

}  // namespace interfaces
}  // namespace poe
}  // namespace appl
}  // namespace vtss
#endif
