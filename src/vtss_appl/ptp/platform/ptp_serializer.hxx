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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef _VTSS_APPL_PTP_SERIALIZER_HXX_
#define _VTSS_APPL_PTP_SERIALIZER_HXX_

#include "vtss/appl/ptp.h"
#include "vtss_appl_serialize.hxx"
#include "vtss/basics/expose.hxx"
#include "vtss_appl_formatting_tags.hxx"
#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl_3038x_api_pdv_api.h"
#endif

#define VTSS_PTP_HAS_HW_CLOCK_DOMAINS  // Note: Comment this line if the build does not support HW clock domains.

extern "C" void vtss_ptp_mib_init();

// Serialize enums for elements of GlobalsExternalClockMode

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_ext_clock_1pps_t, "ptpExtClock1pps",
                         vtss_appl_ptp_ext_clock_1pps_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_ext_clock_1pps_t, "ptpExtClock1pps",
                         vtss_appl_ptp_ext_clock_1pps_txt, "-");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ptp_preferred_adj_t, "ptpPreferredAdj",
                         vtss_appl_ptp_preferred_adj_txt, "-");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ptp_unicast_comm_state_t, "ptpUcCommState",
                         vtss_appl_ptp_unicast_comm_state_txt, "-");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_ptp_clock_port_state_t, "ptpClockPortState",
                         vtss_appl_ptp_clock_port_state_txt, "-");

// Serialize enum for element of GlobalsSystemTimeSyncMode

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_system_time_sync_mode_t, "ptpSystemTimeSyncMode",
                         vtss_appl_ptp_system_time_sync_mode_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_system_time_sync_mode_t, "ptpSystemTimeSyncMode",
                         vtss_appl_ptp_system_time_sync_mode_txt, "-");

// Serialize enums for elements of ConfigClocksDefaultDS

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_device_type_t, "ptpDeviceType",
                         vtss_appl_ptp_device_type_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_device_type_t, "ptpDeviceType",
                         vtss_appl_ptp_device_type_txt, "-");

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_protocol_t, "ptpProtocol",
                         vtss_appl_ptp_protocol_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_protocol_t, "ptpProtocol",
                         vtss_appl_ptp_protocol_txt, "-");

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_profile_t, "ptpProfile",
                         vtss_appl_ptp_profile_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_profile_t, "ptpProfile",
                         vtss_appl_ptp_profile_txt, "-");

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_filter_type_t, "ptpFilterType",
                         vtss_appl_ptp_filter_type_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_filter_type_t, "ptpFilterType",
                         vtss_appl_ptp_filter_type_txt, "-");
// Serialize enums for elements of Virtual-port parameters

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_virtual_port_mode_t, "ptpVirtualPortmode",
                         vtss_appl_virtual_port_mode_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_virtual_port_mode_t, "ptpVirtualPortmode",
                         vtss_appl_virtual_port_mode_txt, "-");

VTSS_SNMP_SERIALIZE_ENUM(vtss_ptp_appl_rs422_protocol_t, "ptpTodProtocol",
                         vtss_ptp_appl_rs422_protocol_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_ptp_appl_rs422_protocol_t, "ptpTodProtocol",
                         vtss_ptp_appl_rs422_protocol_txt, "-");
// Serialize enum vtss_appl_ptp_leap_second_type_t for leapType element of vtss_appl_ptp_clock_timeproperties_ds_t

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_leap_second_type_t, "ptpLeapSecondType",
                         vtss_appl_ptp_leap_second_type_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_leap_second_type_t, "ptpLeapSecondType",
                         vtss_appl_ptp_leap_second_type_txt, "-");

// Serialize enum for element of ConfigClocksServoParameters

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_srv_clock_option_t, "ptpServoClockOption",
                         vtss_appl_ptp_srv_clock_option_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_srv_clock_option_t, "ptpServoClockOption",
                         vtss_appl_ptp_srv_clock_option_txt, "-");

// Serialize enum for element of ConfigClocksPortDS

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_dest_adr_type_t, "ptpDestAdrType",
                         vtss_appl_ptp_dest_adr_type_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_dest_adr_type_t, "ptpDestAdrType",
                         vtss_appl_ptp_dest_adr_type_txt, "-");

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_delay_mechanism_t, "ptpDelayMechanism",
                         vtss_appl_ptp_delay_mechanism_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_delay_mechanism_t, "ptpDelayMechanism",
                         vtss_appl_ptp_delay_mechanism_txt, "-");                         

// Serialize enum vtss_appl_ptp_slave_clock_state_t for slave_state element of StatusClocksSlaveDS

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_slave_clock_state_t, "ptpSlaveClockState",
                         vtss_appl_ptp_slave_clock_state_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_slave_clock_state_t, "ptpSlaveClockState",
                         vtss_appl_ptp_slave_clock_state_txt, "-");

// Serialize enum vtss_appl_ptp_802_1as_port_role_t for portRole element of s_802_1as

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_ptp_802_1as_port_role_t, "ptp8021asPortRole",
                         vtss_appl_ptp_802_1as_port_role_txt, "-");

VTSS_JSON_SERIALIZE_ENUM(vtss_appl_ptp_802_1as_port_role_t, "ptp8021asPortRole",
                         vtss_appl_ptp_802_1as_port_role_txt, "-");

namespace vtss {
namespace appl {
namespace ptp {
namespace interfaces {

struct PtpCapHasMsPdv {
    static constexpr const char *json_ref = "vtss_appl_ptp_capabilities_t";
    static constexpr const char *name = "HasMsPdv";
    static constexpr const char *desc = "If true, the build supports the MS-PDV.";
    static bool get() {
#if defined(VTSS_SW_OPTION_ZLS30387)
        vtss_zarlink_servo_t servo_type = (vtss_zarlink_servo_t)fast_cap(VTSS_APPL_CAP_ZARLINK_SERVO_TYPE);
        if (servo_type == VTSS_ZARLINK_SERVO_ZLS30380) {
            return true;
        }
#endif
        return false;
    }
};

struct PtpCapHasHwClkDomains {
    static constexpr const char *json_ref = "vtss_appl_ptp_capabilities_t";
    static constexpr const char *name = "HasHwClkDomains";
    static constexpr const char *desc = "If true, the build supports hardware clock domains.";
    static constexpr bool get() {
#if defined(VTSS_PTP_HAS_HW_CLOCK_DOMAINS)
        return true;
#else
        return false;
#endif
    }
};

template<typename HANDLER>
void serialize(HANDLER &h, vtss_appl_ptp_clock_timeproperties_ds_t &i)
{
    typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_timeproperties_ds_t"));

    m.add_leaf(i.currentUtcOffset,
           vtss::tag::Name("currentUtcOffset"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
           vtss::tag::Description("The current UTC time offset. Range 0 to 10000."));

    m.add_leaf(vtss::AsBool(i.currentUtcOffsetValid),
           vtss::tag::Name("currentUtcOffsetValid"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
           vtss::tag::Description("Indicates whether the current UTC time offset value is valid."));

    m.add_leaf(vtss::AsBool(i.leap59),
           vtss::tag::Name("leap59"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
           vtss::tag::Description("Indicates that the last minute of the day has only 59 seconds."));

    m.add_leaf(vtss::AsBool(i.leap61),
           vtss::tag::Name("leap61"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
           vtss::tag::Description("Indicates that the last minute of the day has 61 seconds."));

    m.add_leaf(vtss::AsBool(i.timeTraceable),
           vtss::tag::Name("timeTraceable"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
           vtss::tag::Description("Indicates that time is traceable to a primary reference."));

    m.add_leaf(vtss::AsBool(i.frequencyTraceable),
           vtss::tag::Name("frequencyTraceable"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
           vtss::tag::Description("Indicates that frequency is traceable to a primary reference."));

    m.add_leaf(vtss::AsBool(i.ptpTimescale),
           vtss::tag::Name("ptpTimescale"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
           vtss::tag::Description("Indicates whether timescale of the grandmaster clock is PTP."));

    m.add_leaf(i.timeSource,
           vtss::tag::Name("timeSource"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
           vtss::tag::Description("Source of time used by the grandmaster clock.\n"
                                  "Value representation : 16 (Atomic clock), 32 (GPS), "
                                  "48 (Terrestrial radio), 64 (PTP), 80 (NTP), 96 (Hand set), "
                                  "144 (Other), 160 (Internal oscillator)."));

    m.add_leaf(vtss::AsBool(i.pendingLeap),
           vtss::tag::Name("pendingLeap"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
           vtss::tag::Description("Indicates whether a leap event is pending."));

    m.add_leaf(i.leapDate,
           vtss::tag::Name("leapDate"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
           vtss::tag::Description("Date of leap event represented as number of days after 1970/01/01"));

    m.add_leaf(i.leapType,
           vtss::tag::Name("leapType"),
           vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
           vtss::tag::Description("Type of leap event i.e. leap59 or leap61."));
}

struct GlobalCapabilities {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_ptp_capabilities_t *>>
            P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ptp_capabilities_t &i) {
        h.argument_properties(vtss::tag::Name("capabilities"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_capabilities_t"));

        m.add_leaf(i.ptp_clock_max,
               vtss::tag::Name("clockCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The number of PTP clocks supported by the device."));

        m.template capability<PtpCapHasMsPdv>(vtss::expose::snmp::OidElementValue(2));
        
        m.template capability<PtpCapHasHwClkDomains>(vtss::expose::snmp::OidElementValue(3));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_capabilities_global_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);
};

struct GlobalsExternalClockMode {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_ptp_ext_clock_mode_t *>>
            P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ptp_ext_clock_mode_t &i) {
        h.argument_properties(vtss::tag::Name("ext_clock_mode"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_ext_clock_mode_t"));

        m.add_leaf(i.one_pps_mode,
               vtss::tag::Name("onePpsMode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The mode of the PPS pin. Valid options : onePpsDisable, onePpsOutput."));

        m.add_leaf(vtss::AsBool(i.clock_out_enable),
               vtss::tag::Name("externalEnable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("External enable of PPS output."));

        m.add_leaf(i.adj_method,
               vtss::tag::Name("adjustMethod"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The adjustment method of the PTP timer.\n"
                                      "   0: LTC\n"
                                      "   1: Single DPLL\n"
                                      "   2: Independent DPLL\n"
                                      "   3: Common DPLL's"
                                      "   4: Auto"));

        m.add_leaf(i.freq,
               vtss::tag::Name("clockFrequency"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("The frequency in hertz (Hz) of the PPS external output. Range 1 to 25e6."));

        m.add_leaf(i.clk_domain,
               vtss::tag::Name("ppsDomain"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Clock domain from which 1PPS output need to be generated."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_ext_clock_out_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_ext_clock_out_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct GlobalsSystemTimeSyncMode {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_ptp_system_time_sync_conf_t *>>
            P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ptp_system_time_sync_conf_t &i) {
        h.argument_properties(vtss::tag::Name("time_sync_conf"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_system_time_sync_conf_t"));

        m.add_leaf(i.mode,
               vtss::tag::Name("mode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Mode of the System time <-> ptp time synchronization. Valid Options : systemTimeNoSync, systemTimeSyncGet and systemTimeSyncSet."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_system_time_sync_mode_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_system_time_sync_mode_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

// FIXME:
// 
// struct GlobalsMsPdvConfig {
//     typedef expose::ParamList<expose::ParamVal<vtss_appl_ptp_clock_ms_pdv_config_t *>>
//             P;
// 
//     typedef PtpCapHasMsPdv depends_on_t;
// 
//     VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ptp_clock_ms_pdv_config_t &i) {
//         h.argument_properties(vtss::tag::Name("ms_pdv_config"));
//         typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_ms_pdv_config_t"));
// 
//         m.add_leaf(vtss::AsBool(i.has_one_hz),
//                vtss::tag::Name("hasOneHz"),
//                vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
//                vtss::tag::Description("has_one_hz parameter of MS PDV config."));
// 
//         m.add_leaf(i.phase,
//                vtss::tag::Name("phase"),
//                vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
//                vtss::tag::Description("phase parameter of MS PDV config."));
// 
//         m.add_leaf(i.apr,
//                vtss::tag::Name("apr"),
//                vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
//                vtss::tag::Description("apr parameter of MS PDV config."));
//     }
// 
//     VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_ms_pdv_config_get);
//     VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_ms_pdv_config_set);
//     VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
// };

struct ConfigClocksDefaultDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_config_default_ds_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description = "This is the configurable part of the PTP clocks DefaultDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";
    

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_config_default_ds_t &i) {
        h.argument_properties(vtss::tag::Name("default_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_config_default_ds_t"));

        m.add_leaf(i.deviceType,
               vtss::tag::Name("deviceType"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("The PTP clock type. Valid options : none, ordBound, p2pTransparent, e2eTransparent, masterOnly, slaveOnly and bcFrontend."));

        m.add_leaf(vtss::AsBool(i.twoStepFlag),
               vtss::tag::Name("twoStepFlag"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Determines whether clock uses follow-up packets."));

        m.add_leaf(i.priority1,
               vtss::tag::Name("priority1"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("The priority1 value."));

        m.add_leaf(i.priority2,
               vtss::tag::Name("priority2"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The priority2 value."));

        m.add_leaf(vtss::AsBool(i.oneWay),
               vtss::tag::Name("oneWay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Determines whether clock uses sync packets only."));

        m.add_leaf(i.domainNumber,
               vtss::tag::Name("domainNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("The domain number. Range 0 to 127."));

        m.add_leaf(i.protocol,
               vtss::tag::Name("protocol"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("The protocol to be used for the encapsulation of the PTP "
                                      "packets. Valid options : ethernet, ethernetMixed, "
                                      "ip4multi, ip4mixed, ip4uni, oam, onePps, ip6mixed and ethip4ip6combo."));

        //m.add_leaf(vtss::AsBool(i.tagging_enable),
        //       vtss::tag::Name("vlanTagEnable"),
        //       vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
        //       vtss::tag::Description("Not used. Tagging depends on the VLAN configuration and the configured VLAN id."));
        // Tagging enable is not used any more, and the OidElementValue(9) shall not be reused.
        m.add_leaf(i.configured_vid,
               vtss::tag::Name("vid"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("The VLAN id for this PTP instance. Range 1 to 4095."));

        m.add_leaf(i.configured_pcp,
               vtss::tag::Name("pcp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("The PCP value for this PTP instance. Range 0 to 7."));

        m.add_leaf(i.mep_instance,
               vtss::tag::Name("mep"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("The mep instance number (if protocol is OAM). Range 0 to 100."));

        m.add_leaf(i.clock_domain,
                   vtss::tag::Name("clkDom"),
                   vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
                   vtss::tag::Description("Clock domain used. Range 0 to 127."),
                   vtss::tag::DependOnCapability<PtpCapHasHwClkDomains>());

        m.add_leaf(i.dscp,
                   vtss::tag::Name("dscp"),
                   vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(14),
                   vtss::tag::Description("The DSCP field value (if protocol is IPv4). Range 0 to 3."));

        m.add_leaf(i.profile,
               vtss::tag::Name("profile"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(15),
               vtss::tag::Description("The PTP profile. Valid options : noProfile, ieee1588, g8265, g8275d1, g8275d2, ieee802d1as."));

        m.add_leaf(i.localPriority,
               vtss::tag::Name("localPriority"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(16),
               vtss::tag::Description("The local priority value."));

        m.add_leaf(i.filter_type,
               vtss::tag::Name("filterType"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(17),
               vtss::tag::Description(
                   "Selects the type of filter/servo used. Valid Options : default, freqXo, "
                   "phaseXo, freqTcxo, phaseTcxo, freqOcxoS3e, phaseOcxoS3e, partialOnPathFreq, "
                   "partialOnPathPhase, fullOnPathFreq, fullOnPathPhase, "
                   "fullOnPathPhaseFasterLockLowPktRate, freqAccuracyFdd, freqAccuracyXdsl, "
                   "elecFreq, elecPhase, phaseRelaxedC60W, phaseRelaxedC150, phaseRelaxedC180, "
                   "phaseRelaxedC240, phaseOcx0S3eR461, basicPhase, basicPhaseLow, basic."));

        m.add_leaf(vtss::AsBool(i.path_trace_enable),
               vtss::tag::Name("pathTraceEnable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(18),
               vtss::tag::Description("The Announce Path Trace supported."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_config_default_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_config_default_ds_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_ptp_clock_config_default_ds_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_ptp_clock_config_default_ds_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct ConfigClocksTimePropertiesDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_timeproperties_ds_t *>> P;

    static constexpr const char *table_description = "This is the configurable part of the PTP clocks TimePropertiesDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_timeproperties_ds_t &i) {
        h.argument_properties(vtss::tag::Name("timeproperties"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_config_timeproperties_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_config_timeproperties_ds_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct ConfigClocksFilterParameters {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_filter_config_t *>> P;

    static constexpr const char *table_description = "This is the configurable part of the PTP clocks filter parameters.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_filter_config_t &i) {
        h.argument_properties(vtss::tag::Name("filter_config"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_filter_config_t"));

        m.add_leaf(i.delay_filter,
               vtss::tag::Name("delayFilter"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Defines the time constant of the delay filter."));

        m.add_leaf(i.period,
               vtss::tag::Name("period"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Defines the period of the offset filter."));

        m.add_leaf(i.dist,
               vtss::tag::Name("dist"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Sets the dist value of the offset filter."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_filter_parameters_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_filter_parameters_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct ConfigClocksServoParameters {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_servo_config_t *>> P;

    static constexpr const char *table_description = "This is the configurable part of the PTP clocks servo parameters.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_servo_config_t &i) {
        h.argument_properties(vtss::tag::Name("servo_config"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_servo_config_t"));

        m.add_leaf(vtss::AsBool(i.display_stats),
               vtss::tag::Name("display"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Indicates whether output shall be sent to the debug terminal."));

        m.add_leaf(vtss::AsBool(i.p_reg),
               vtss::tag::Name("pEnable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Indicates whether P-value of servo algorithm shall be used."));

        m.add_leaf(vtss::AsBool(i.i_reg),
               vtss::tag::Name("iEnable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Indicates whether I-value of servo algorithm shall be used."));

        m.add_leaf(vtss::AsBool(i.d_reg),
               vtss::tag::Name("dEnable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Indicates whether D-value of servo algorithm shall be used."));

        m.add_leaf(i.ap,
               vtss::tag::Name("pval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("P-value of the offset filter. Range 1 to 10000."));

        m.add_leaf(i.ai,
               vtss::tag::Name("ival"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("I-value of the offset filter. Range 1 to 10000."));

        m.add_leaf(i.ad,
               vtss::tag::Name("dval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("D-value of the offset filter. Range 1 to 10000."));

        m.add_leaf(i.srv_option,
               vtss::tag::Name("srvOption"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("Indicates whether clock is free running or locked to SyncE. Valid options : clockFree, clockSyncE."));

        m.add_leaf(i.synce_threshold,
               vtss::tag::Name("synceThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("SyncE Threshold. Range 1 to 1000."));

        m.add_leaf(i.synce_ap,
               vtss::tag::Name("synceAp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("SyncE Ap. Range 1 to 40."));

        m.add_leaf(i.ho_filter,
               vtss::tag::Name("hoFilter"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Holdoff low pass filter constant. Range 10 to 86000."));

        m.add_leaf(i.stable_adj_threshold,
               vtss::tag::Name("stableAdjThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("Threshold at which offset is assumed to be stable. Range 1 to 3000."));

        m.add_leaf(i.gain,
                   vtss::tag::Name("gain"),
                   vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(14),
                   vtss::tag::Description("gain-value of the offset filter. Range 1 to 10000."));

    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_servo_parameters_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_servo_parameters_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct ConfigClocksSlaveConfig {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_slave_config_t *>> P;

    static constexpr const char *table_description = "This is the configurable part of the PTP clocks slave configuration.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_slave_config_t &i) {
        h.argument_properties(vtss::tag::Name("slave_config"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_slave_config_t"));

        m.add_leaf(i.stable_offset,
               vtss::tag::Name("stableOffset"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Stable offset threshold in ns."));

        m.add_leaf(i.offset_ok,
               vtss::tag::Name("offsetOk"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Offset OK threshold in ns."));

        m.add_leaf(i.offset_fail,
               vtss::tag::Name("offsetFail"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Offset fail threshold in ns."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_slave_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_slave_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct ConfigClocksUnicastSlaveConfig {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_unicast_slave_config_t *>> P;

    static constexpr const char *table_description = "This is the configurable part of the PTP clocks unicast slave configuration.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("master_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("masterId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ptp_unicast_slave_config_t &i) {
        h.argument_properties(vtss::tag::Name("unicast_slave_config"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_unicast_slave_config_t"));

        m.add_leaf(i.duration,
               vtss::tag::Name("duration"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Number of seconds for which the Announce/Sync messages are requested."));

        m.add_leaf(AsIpv4(i.ip_addr),
               vtss::tag::Name("ipAddress"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("IPv4 address of requested master clock."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_config_unicast_slave_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_master_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_config_unicast_slave_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct ConfigClocksPortDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamKey<vtss_ifindex_t>,
                              expose::ParamVal<vtss_appl_ptp_config_port_ds_t *>> P;

    static constexpr const char *table_description = "This is the configurable part of the PTP clocks PortDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i) {
        h.argument_properties(vtss::tag::Name("port_id"));
        h.add_leaf(AsInterfaceIndex(i),
               vtss::tag::Name("portId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ptp_config_port_ds_t &i) {
        h.argument_properties(vtss::tag::Name("port_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_config_port_ds_t"));

        m.add_leaf(AsBool(i.enabled),
               vtss::tag::Name("enabled"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Defines whether port is enabled."));

        m.add_leaf(i.logAnnounceInterval,
               vtss::tag::Name("logAnnounceInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Interval between announce message transmissions. Range -3 to 4."));

        m.add_leaf(i.announceReceiptTimeout,
               vtss::tag::Name("announceReceiptTimeout"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The timeout for receiving announce messages on the port. Range 1 to 10."));

        m.add_leaf(i.logSyncInterval,
               vtss::tag::Name("logSyncInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("The interval for issuing sync meesages in the master. Range -7 to 4."));

        m.add_leaf(i.delayMechanism,
               vtss::tag::Name("delayMechanism"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("The delay mechanism used for the port. Valid options : e2e, p2p, commonP2P, disabled."));

        m.add_leaf(i.logMinPdelayReqInterval,
               vtss::tag::Name("logMinPdelayReqInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("The value of logMinPdelayReqInterval. Range -7 to 5."));

        m.add_leaf(i.delayAsymmetry,
               vtss::tag::Name("delayAsymmetry"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("The value for the communication path asymmetry. Range -10000 to 10000."));

        m.add_leaf(i.ingressLatency,
               vtss::tag::Name("ingressLatency"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("Ingress delay for port. Range -10000 to 10000."));

        m.add_leaf(i.egressLatency,
               vtss::tag::Name("egressLatency"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("Egress delay for port. Range -10000 to 10000."));

        m.add_leaf(AsBool(i.portInternal),
               vtss::tag::Name("portInternal"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Defines whether port is enabled as an internal interface."));

        m.add_leaf(i.versionNumber,
               vtss::tag::Name("versionNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("The version of PTP being used by the port. Current version is 2."));

        m.add_leaf(i.dest_adr_type,
               vtss::tag::Name("mcastAddr"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(14),
               vtss::tag::Description("The multicast address used. Valid options : default or linkLocal."));

        m.add_leaf(AsBool(i.masterOnly),
               vtss::tag::Name("masterOnly"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(15),
               vtss::tag::Description("When true, the port cannot enter slave state."));

        m.add_leaf(i.localPriority,
               vtss::tag::Name("localPriority"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(16),
               vtss::tag::Description("The local priority value."));

        m.add_leaf(AsBool(i.notMaster),
               vtss::tag::Name("notMaster"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(17),
               vtss::tag::Description("When true, the port will not enter Master state."));

       m.add_leaf(AsBool(i.c_802_1as.as2020),
               vtss::tag::Name("as2020"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(18),
               vtss::tag::Description("When true, the 802.1as version will be 2020."));

        m.add_leaf(i.c_802_1as.peer_d.meanLinkDelayThresh,
               vtss::tag::Name("c8021asNeighborPropDelayThresh"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(19),
               vtss::tag::Description("max allowed meanLinkDelay. Range 0 to 4e9."));

        m.add_leaf(i.c_802_1as.syncReceiptTimeout,
               vtss::tag::Name("c8021asSyncReceiptTimeout"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(20),
               vtss::tag::Description("Number of time-synchronization transmission intervals that a slave port waits without receiving synchronization information."));

        m.add_leaf(i.c_802_1as.peer_d.allowedLostResponses,
               vtss::tag::Name("c8021asAllowedLostResponses"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(21),
               vtss::tag::Description("Number of Pdelay_Req messages for which a valid response is not received, above which a port is considered to not be exchanging peer delay messages with its neighbor.\n\
                                      Range 0 to 10."));

        m.add_leaf(i.c_802_1as.peer_d.allowedFaults,
               vtss::tag::Name("c8021asAllowedFaults"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(22),
               vtss::expose::snmp::RangeSpec<u32>(1, 255),
               vtss::tag::Description("Number of allowed instances where the computed mean propagation delay exceeds the threshold meanLinkDelayThresh and/or instances where the computation of neighborRateRatio is invalid."));

        m.add_leaf(AsBool(i.c_802_1as.useMgtSettableLogAnnounceInterval),
               vtss::tag::Name("c8021asUseMgtSettableLogAnnounceIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(23),
               vtss::tag::Description("Flag to decide the value to be used for Announce  interval. Range -3 to 4."));

        m.add_leaf(AsBool(i.c_802_1as.useMgtSettableLogSyncInterval),
               vtss::tag::Name("c8021asUseMgtSettableLogSyncIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(24),
               vtss::tag::Description("Flag to decide the value to be used for sync packet interval. Range -7 to 4."));

        m.add_leaf(AsBool(i.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval),
               vtss::tag::Name("c8021asUseMgtSettableLogPdelayReqIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(25),
               vtss::tag::Description("Flag to decide the value to be used for Peer delay request interval. Range -7 to 5."));

        m.add_leaf(i.c_802_1as.mgtSettableLogAnnounceInterval,
               vtss::tag::Name("c8021asMgtSettableLogAnnounceIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(26),
               vtss::expose::snmp::RangeSpec<int >(-3, 4),
               vtss::tag::Description("If useMgmtAnnounce is set, port uses this value to decide the Announce packet interval , else default value is used. Range is -3 to 4."));

        m.add_leaf(i.c_802_1as.mgtSettableLogSyncInterval,
               vtss::tag::Name("c8021asMgtSettableLogSyncIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(27),
               vtss::expose::snmp::RangeSpec<int >(-7, 4),
               vtss::tag::Description("If useMgmtSync is set, port uses this value to decide the sync packet interval , else default value is used. Range is -7 to 4 ."));

        m.add_leaf(i.c_802_1as.peer_d.mgtSettableLogPdelayReqInterval,
               vtss::tag::Name("c8021asMgtSettableLogPdelayReqIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(28),
               vtss::expose::snmp::RangeSpec<int >(-7, 5),
               vtss::tag::Description("If useMgmtPdelay is set, port uses this value to decide the Peer delay request packet interval , else default value is used. Range is -7 to 5."));

        m.add_leaf(AsBool(i.c_802_1as.useMgtSettableLogGptpCapableMessageInterval),
               vtss::tag::Name("c8021asUseMgtSettableLogGptpCapMsgIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(29),
               vtss::tag::Description("Flag to set OPTION to configure gptp interval."));

        m.add_leaf(i.c_802_1as.mgtSettableLogGptpCapableMessageInterval,
               vtss::tag::Name("c8021asMgtSettableLogGptpCapMsgIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(30),
               vtss::tag::Description("If UseMgtSettableLogGptpCapableMessageInterval is set, set gptp capable tlv interval. Range -7 to 4."));

        m.add_leaf(i.c_802_1as.gPtpCapableReceiptTimeout,
               vtss::tag::Name("c8021GptpCapableReceiptTimeout"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(31),
               vtss::expose::snmp::RangeSpec<u32>(1, 255),
               vtss::tag::Description("Set gPtpCapableReceiptTimeout value per port."));

        m.add_leaf(i.c_802_1as.initialLogGptpCapableMessageInterval,
               vtss::tag::Name("c8021InitialLogGptpCapableMessageIntvl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(32),
               vtss::tag::Description("Set gptp interval per port."));

        m.add_leaf(AsBool(i.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio),
               vtss::tag::Name("c8021asUseMgtSetCompNbrRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(33),
               vtss::tag::Description("This value determines the source of the value of computeNeighborRateRatio. 'True' indicates source as 'mgtSettablecomputeNeighborRateRatio'. Otherwise, either initial value or value set by LinkDelayIntervalSetting state machine."));

        m.add_leaf(AsBool(i.c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio),
               vtss::tag::Name("c8021asMgtSetCompNbrRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(34),
               vtss::tag::Description("This value indicates the input through management interface whether to compute neighbor rate ratio or not."));

        m.add_leaf(AsBool(i.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay),
               vtss::tag::Name("c8021asUseMgtSetCompMeanLinkDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(35),
               vtss::tag::Description("This value determines the source of the value of computeMeanLinkDelay. 'True' indicates source as 'mgtSettableComputeMeanLinkDelay'. Otherwise, initial value or value set by LinkDelayInterval State machine is used."));

        m.add_leaf(AsBool(i.c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay),
               vtss::tag::Name("c8021asmgtSetCompMeanLinkDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(36),
               vtss::tag::Description("This value indicates the input through management interface whether to compute Mean Link delay or not."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_config_clocks_port_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_port_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_config_clocks_port_ds_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct ConfigClocksVirtualPortConfig {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_virtual_port_config_t *>> P;

    static constexpr const char *table_description = "This is the configurable part of the PTP clocks virtual port parameters.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_virtual_port_config_t &i) {
        h.argument_properties(vtss::tag::Name("virtual_port_cfg"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_virtual_port_config_t"));

        m.add_leaf(vtss::AsBool(i.enable),
               vtss::tag::Name("enable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Determines whether the virtual port is enabled."));

       m.add_leaf(i.input_pin,
               vtss::tag::Name("inputPin"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::expose::snmp::RangeSpec<unsigned int>(0, 3),
               vtss::tag::Description("Input io pin to be used in pps-in mode ."));

        m.add_leaf(i.clockQuality.clockClass,
               vtss::tag::Name("clockQualityClockClass"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("This is the clock class field of the clock quality structure.\n"
                                      "IEEE-1588-2008 value representation : 6 (GM locked to 1pps output), "
                                      "7 (GM in holdover after 1pps output), "
                                      "135 (Boundary clock in holdover), "
                                      "165 (Boundary clock out of holdover), "
                                      "140 (GM out of holdover CAT 1), "
                                      "150 (GM out of holdover CAT 2), 160 (GM out of holdover CAT 3), "
                                      "248 (Default), 255 (Time slave clock)."));

        m.add_leaf(i.clockQuality.clockAccuracy,
               vtss::tag::Name("clockQualityClockAccuracy"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("This is the clock accuracy field of the clock quality structure.\n\
                                      Value representation : 32 (25 ns), 33 (100 ns), 34 (250 ns), 35 (1 us), 36 (2.5 us)"
                                      "37 (10 us), 38 (25 us), 39 (100 us), 40 (250 ns), 41 (1 ms), 42 (2.5 ms), 43 (10 ms)"
                                      "44 (25 ms), 45 (100 ms), 46 (250 ms), 47 (1 s), 48 (10 s), 49 (> 10 s), 254 (unknown)."));

        m.add_leaf(i.clockQuality.offsetScaledLogVariance,
               vtss::tag::Name("clockQualityOffsetScaledLogVar"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("This is the offsetScaledLogVariance field of the clock quality structure."));

        m.add_leaf(i.priority1,
               vtss::tag::Name("priority1"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("The priority1 value."));

        m.add_leaf(i.priority2,
               vtss::tag::Name("priority2"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("The priority2 value."));

        m.add_leaf(i.localPriority,
               vtss::tag::Name("localPriority"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("The local priority value."));
       m.add_leaf(i.virtual_port_mode,
               vtss::tag::Name("virtualPortMode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("Virtual port mode : disable/main-auto/sub/main-man/pps-in/pps-out/freq-out"));
       m.add_leaf(i.output_pin,
               vtss::tag::Name("outputPin"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::expose::snmp::RangeSpec<unsigned int>(0, 3),
               vtss::tag::Description("Output io pin to be used in pps-out mode"));
       m.add_leaf(i.proto,
               vtss::tag::Name("proto"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Protocol used to transfer time of day. polyt/zda/gga/rmc/pim/none"));
       m.add_leaf(i.portnum,
               vtss::tag::Name("portnum"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("Port number used to transfer tod with pim protocol."));
       m.add_leaf(i.delay,
               vtss::tag::Name("ppsDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(14),
               vtss::tag::Description("PPS delay from source to receiver in sub mode approximated."));
        m.add_leaf(i.timeproperties.currentUtcOffset,
               vtss::tag::Name("currentUtcOffset"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(15),
               vtss::tag::Description("virtual port's UTC time offset. GNSS servers use UTC time. Range 0 to 10000."));

        m.add_leaf(vtss::AsBool(i.timeproperties.currentUtcOffsetValid),
               vtss::tag::Name("currentUtcOffsetValid"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(16),
               vtss::tag::Description("Indicates whether the current UTC time offset value is valid."));

        m.add_leaf(vtss::AsBool(i.timeproperties.leap59),
               vtss::tag::Name("leap59"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(17),
               vtss::tag::Description("Indicates that the last minute of the day has only 59 seconds."));

        m.add_leaf(vtss::AsBool(i.timeproperties.leap61),
               vtss::tag::Name("leap61"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(18),
               vtss::tag::Description("Indicates that the last minute of the day has 61 seconds."));

        m.add_leaf(vtss::AsBool(i.timeproperties.timeTraceable),
               vtss::tag::Name("timeTraceable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(19),
               vtss::tag::Description("Indicates that time is traceable to a primary reference."));

        m.add_leaf(vtss::AsBool(i.timeproperties.frequencyTraceable),
               vtss::tag::Name("frequencyTraceable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(20),
               vtss::tag::Description("Indicates that frequency is traceable to a primary reference."));

        m.add_leaf(vtss::AsBool(i.timeproperties.ptpTimescale),
               vtss::tag::Name("ptpTimescale"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(21),
               vtss::tag::Description("Indicates whether timescale of the grandmaster clock is PTP."));

        m.add_leaf(i.timeproperties.timeSource,
               vtss::tag::Name("timeSource"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(22),
               vtss::tag::Description("Source of time used by the grandmaster clock."));

        m.add_leaf(vtss::AsBool(i.timeproperties.pendingLeap),
               vtss::tag::Name("pendingLeap"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(23),
               vtss::tag::Description("Indicates whether a leap event is pending."));

        m.add_leaf(i.timeproperties.leapDate,
               vtss::tag::Name("leapDate"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(24),
               vtss::tag::Description("Date of leap event represented as number of days after 1970/01/01"));

        m.add_leaf(i.timeproperties.leapType,
               vtss::tag::Name("leapType"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(25),
               vtss::tag::Description("Type of leap event i.e. leap59 or leap61."));

        m.add_leaf(i.alarm,
               vtss::tag::Name("Alarm"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(26),
               vtss::tag::Description("Generate alarm or not on virtual port master to indicate GPS status."));

        m.add_leaf(AsOctetString(i.clock_identity, 8),
               vtss::tag::Name("clockIdentity"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(27),
               vtss::tag::Description("This is the 8 byte clockIdentify used by virtual port."));

        m.add_leaf(i.steps_removed,
               vtss::tag::Name("stepsRemoved"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(28),
               vtss::tag::Description("It is number of PTP clocks traversed from Grandmaster through virtual port to reach current clock."));

    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_config_virtual_port_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_config_virtual_port_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct ConfigCmldsPortDs {
    typedef expose::ParamList<expose::ParamKey<vtss_uport_no_t>,
                              expose::ParamVal<vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *>> P;

    static constexpr const char *table_description = "This is the configurable part of the PTP clocks CMLDS PortDS.";

    static constexpr const char *index_description = "The port_id index must be a value from 0 up to the number of ports.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_uport_no_t &i) {
        h.argument_properties(vtss::tag::Name("port_id"));
        h.add_leaf(i,
               vtss::tag::Name("portId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 255),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t &i) {
        h.argument_properties(vtss::tag::Name("cmlds_port_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t"));

        m.add_leaf(i.delayAsymmetry,
               vtss::tag::Name("delayAsymmetry"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("the asymmetry in the propagation delay on the link attached to this port relative to the grandmaster time base, as defined in 8.3. If propagation delay asymmetry is not modeled, then delayAsymmetry is zero."));

        m.add_leaf(i.peer_d.initialLogPdelayReqInterval,
               vtss::tag::Name("initialLogPdelayReqInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("the value is the logarithm to base 2 of the Pdelay_Req message transmission interval used when the Link Port is initialized, or a message interval request TLV is received with the linkDelayInterval field set to 126"));

        m.add_leaf(AsBool(i.peer_d.useMgtSettableLogPdelayReqInterval),
               vtss::tag::Name("useMgtSettableLogPdelayReqInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("This value determines the source of the sync interval and mean time interval between successive Pdelay_Req messages. TRUE indicates source as mgtSettableLogPdelayReqInterval. FALSE indicates source as LinkDelayIntervalSetting state machine "));
        
        m.add_leaf(i.peer_d.mgtSettableLogPdelayReqInterval,
               vtss::tag::Name("mgtSettableLogPdelayReqInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("This value is the logarithm to base 2 of the mean time interval between successive Pdelay_Req messages if useMgtSettableLogPdelayReqInterval is TRUE."));

        m.add_leaf(AsBool(i.peer_d.initialComputeNeighborRateRatio),
               vtss::tag::Name("initialCompNbrRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Initial value that indicates whether neighborRateRatio is to be computed by this port."));
        
        m.add_leaf(AsBool(i.peer_d.useMgtSettableComputeNeighborRateRatio),
               vtss::tag::Name("useMgtSetCompNbrRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("This value determines the source of the value of computeNeighborRateRatio. 'True' indicates source as 'mgtSettablecomputeNeighborRateRatio'. 'false' indicates source as 'LinkDelayIntervalSetting state machine'."));
        
        m.add_leaf(AsBool(i.peer_d.mgtSettableComputeNeighborRateRatio),
               vtss::tag::Name("mgtSetCompNbrRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("This value indicates the input through management interface whether to compute neighbor rate ratio or not."));
        
        m.add_leaf(AsBool(i.peer_d.initialComputeMeanLinkDelay),
               vtss::tag::Name("initialCompMeanLinkDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("Initial value that indicates whether mean Link delay is computed by this port or not."));
        
        m.add_leaf(AsBool(i.peer_d.useMgtSettableComputeMeanLinkDelay),
               vtss::tag::Name("useMgtSetCompMeanLinkDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("This value determines the source of the value of computeMeanLinkDelay. 'True' indicates source as 'mgtSettableComputeMeanLinkDelay'. 'false' indicates source as 'LinkDelayIntervalSetting state machine'."));
        
        m.add_leaf(AsBool(i.peer_d.mgtSettableComputeMeanLinkDelay),
               vtss::tag::Name("mgtSetCompMeanLinkDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("This value indicates the input through management interface whether to compute Mean Link Delay or not."));
        
        m.add_leaf(i.peer_d.allowedLostResponses,
               vtss::tag::Name("allowedLostResponses"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::expose::snmp::RangeSpec<u32>(0, 10),
               vtss::tag::Description("It is the number of Pdelay_Req messages for which a valid response is not received, above which a Link Port is considered to not be exchanging peer delay messages with its neighbor."));

        m.add_leaf(i.peer_d.allowedFaults,
               vtss::tag::Name("allowedFaults"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
               vtss::expose::snmp::RangeSpec<u32>(1, 255),
               vtss::tag::Description("It is the number of faults, above which asCapableAcrossDomains is set to FALSE."));

        m.add_leaf(i.peer_d.meanLinkDelayThresh,
               vtss::tag::Name("meanLinkDelayThresh"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(14),
               vtss::tag::Description("The propagation time threshold, above which a port is not considered capable of participating in the IEEE 802.1AS protocol."));

    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_cmlds_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_port_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_cmlds_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};
struct StatusClocksDefaultDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_status_default_ds_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP clocks DefaultDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_status_default_ds_t &i) {
        h.argument_properties(vtss::tag::Name("default_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_status_default_ds_t"));

        m.add_leaf(AsOctetString(i.clockIdentity, 8),
               vtss::tag::Name("clockIdentity"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("This is the unique 8 byte clockIdentify field from the DefaultDS structure."));

        m.add_leaf(i.clockQuality.clockClass,
               vtss::tag::Name("clockQualityClockClass"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("This is the clock class field of the clock quality structure.\n\
                                      IEEE-1588-2008 value representation : 6 (GM locked to 1pps output), 7 (GM in holdover after 1pps output), "
                                      "135 (Boundary clock in holdover), 165 (Boundary clock out of holdover), 140 (GM out of holdover CAT 1), "
                                      "150 (GM out of holdover CAT 2), 160 (GM out of holdover CAT 3), 248 (Default), 255 (Time slave clock)."));

        m.add_leaf(i.clockQuality.clockAccuracy,
               vtss::tag::Name("clockQualityClockAccuracy"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("This is the clock accuracy field of the clock quality structure.\n\
                                      Value representation : 32 (25 ns), 33 (100 ns), 34 (250 ns), 35 (1 us), 36 (2.5 us) "
                                      "37 (10 us), 38 (25 us), 39 (100 us), 40 (250 ns), 41 (1 ms), 42 (2.5 ms), 43 (10 ms)"
                                      "44 (25 ms), 45 (100 ms), 46 (250 ms), 47 (1 s), 48 (10 s), 49 (> 10 s), 254 (unknown)."));

        m.add_leaf(i.clockQuality.offsetScaledLogVariance,
               vtss::tag::Name("clockQualityOffsetScaledLogVar"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("This is the offsetScaledLogVariance field of the clock quality structure."));

        m.add_leaf(vtss::AsBool(i.s_802_1as.gmCapable),
               vtss::tag::Name("s8021asGmCapable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Defines IEEE 802.1AS specific default_DS status parameters, TRUE if the time-aware system is capable of being a Grandmaster."));

        m.add_leaf(i.s_802_1as.sdoId,
               vtss::tag::Name("s8021asSdoId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("Defines IEEE 802.1AS specific default_DS status parameters, part of ptp domain identifier."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_status_default_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

struct StatusClocksCurrentDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_current_ds_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP clocks CurrentDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_current_ds_t &i) {
        h.argument_properties(vtss::tag::Name("current_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_current_ds_t"));

        m.add_leaf(i.stepsRemoved,
               vtss::tag::Name("stepsRemoved"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("The number of PTP clocks traversed from the grandmaster to the local PTP clock."));

        m.add_leaf(i.offsetFromMaster,
               vtss::tag::Name("offsetFromMaster"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The time difference in ns from the grandmaster to the local PTP clock."));

        m.add_leaf(i.meanPathDelay,
               vtss::tag::Name("meanPathDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("The mean path delay from the master to the local slave."));

        m.add_leaf(i.cur_802_1as.lastGMPhaseChange.scaled_ns_low,
               vtss::tag::Name("cur8021asLastGMPhaseChangeLow"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The system time when the most recent change in grandmaster phase occurred due to a change of either the grandmaster or grandmaster time base 64 ls bits."));

        m.add_leaf(i.cur_802_1as.lastGMPhaseChange.scaled_ns_high,
               vtss::tag::Name("cur8021asLastGMPhaseChangeHigh"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("The system time when the most recent change in grandmaster phase occurred due to a change of either the grandmaster or grandmaster time base in 32 ms bits"));

        m.add_leaf(vtss::AsDecimalNumber(i.cur_802_1as.lastGMFreqChange,-9),
                   vtss::tag::Name("cur8021asLastGMFreqChange"),
                   vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
                   vtss::tag::Description("TRUE if the time-aware system is capable of being a Grandmaster."));

        m.add_leaf(i.cur_802_1as.gmTimeBaseIndicator,
               vtss::tag::Name("cur8021asGmTimeBaseIndicator"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("TimeBaseIndicator of the current grandmaster."));

        m.add_leaf(i.cur_802_1as.gmChangeCount,
               vtss::tag::Name("cur8021asGmChangeCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("The number of times the grandmaster has changed in a gPTP domain."));

        m.add_leaf(i.cur_802_1as.timeOfLastGMChangeEvent,
               vtss::tag::Name("cur8021asTimeOfLastGMChangeEvt"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("The system time when the most recent grandmaster change occurred."));

        m.add_leaf(i.cur_802_1as.timeOfLastGMPhaseChangeEvent,
               vtss::tag::Name("cur8021asTimeOfLastGMPhaseChangeEvt"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("The system time when the most recent change in grandmaster phase occurred due to a change of either the grandmaster or grandmaster time base."));

        m.add_leaf(i.cur_802_1as.timeOfLastGMFreqChangeEvent,
               vtss::tag::Name("cur8021asTimeOfLastGMFreqChangeEvent"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("The system time when the most recent change in grandmaster frequency occurred due to a change of either the grandmaster or grandmaster time base."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_status_current_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);
};

struct StatusClocksParentDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_parent_ds_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP clocks ParentDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_parent_ds_t &i) {
        h.argument_properties(vtss::tag::Name("parent_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_parent_ds_t"));

        m.add_leaf(AsOctetString(i.parentPortIdentity.clockIdentity, 8),
               vtss::tag::Name("parentPortIdentityClockIdentity"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("This is the 8 byte unique clock identity of the parent port."));

        m.add_leaf(i.parentPortIdentity.portNumber,
               vtss::tag::Name("parentPortIdentityPortNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("This is the port number on the parent associated with the parent clock."));

        m.add_leaf(vtss::AsBool(i.parentStats),
               vtss::tag::Name("parentStats"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Parents stats (always false)."));

        m.add_leaf(i.observedParentOffsetScaledLogVariance,
               vtss::tag::Name("observedParentOffsetScaledLogVar"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("This field is optional and is not computed (as signaled by parentStats being false)."));

        m.add_leaf(i.observedParentClockPhaseChangeRate,
               vtss::tag::Name("observedParentClockPhaseChangeRate"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("This field is optional and is not computed (as signaled by parentStats being false)."));

        m.add_leaf(AsOctetString(i.grandmasterIdentity, 8),
               vtss::tag::Name("grmstrIdentity"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("This is the 8 byte unique clock identity of the grand master clock."));

        m.add_leaf(i.grandmasterClockQuality.clockClass,
               vtss::tag::Name("grmstrClkQualClockClass"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("This is the clock class of the clock quality structure of the grand master clock.\n\
                                      IEEE-1588-2008 value representation : 6 (GM locked to 1pps output), 7 (GM in holdover after 1pps output), "
                                      "135 (Boundary clock in holdover), 165 (Boundary clock out of holdover), 140 (GM out of holdover CAT 1), "
                                      "150 (GM out of holdover CAT 2), 160 (GM out of holdover CAT 3), 248 (Default), 255 (Time slave clock)."));

        m.add_leaf(i.grandmasterClockQuality.clockAccuracy,
               vtss::tag::Name("gmstrClkQualClockAccuracy"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("This is the clock accuracy of the clock quality structure of the grand master clock.\n\
                                      Value representation : 32 (25 ns), 33 (100 ns), 34 (250 ns), 35 (1 us), 36 (2.5 us) "
                                      "37 (10 us), 38 (25 us), 39 (100 us), 40 (250 ns), 41 (1 ms), 42 (2.5 ms), 43 (10 ms) "
                                      "44 (25 ms), 45 (100 ms), 46 (250 ms), 47 (1 s), 48 (10 s), 49 (> 10 s), 254 (unknown)."));

        m.add_leaf(i.grandmasterClockQuality.offsetScaledLogVariance,
               vtss::tag::Name("gmstrClkQualOffsetScaledLogVar"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("This is the offsetScaledLogVariance field of the clock quality structure of the grand master clock."));

        m.add_leaf(i.grandmasterPriority1,
               vtss::tag::Name("gmstrPriority1"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("Grandmaster Priority1 value."));

        m.add_leaf(i.grandmasterPriority2,
               vtss::tag::Name("gmstrPriority2"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Grandmaster Priority2 value."));

        m.add_leaf(i.par_802_1as.cumulativeRateRatio,
               vtss::tag::Name("par8021asCumulativeRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("The ratio of the frequency og the grandmaster to the frequencu of the Local CLock entity, expressed as fractional frequency offset * 2**41 ."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_status_parent_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);
};

struct StatusClocksTimePropertiesDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_timeproperties_ds_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP clocks TimePropertiesDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_timeproperties_ds_t &i) {
        h.argument_properties(vtss::tag::Name("timeproperties"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_status_timeproperties_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);
};

struct StatusClocksSlaveDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_slave_ds_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP clocks SlaveDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));        
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_slave_ds_t &i) {
        h.argument_properties(vtss::tag::Name("slave_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_slave_ds_t"));

        m.add_leaf(i.port_number,
               vtss::tag::Name("slavePort"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("0 => no slave port, 1..n => selected slave port."));

        m.add_leaf(i.slave_state,
               vtss::tag::Name("slaveState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The slaves state. Valid options : freerun, freqLocking, freqLocked, phaseLocking, phaseLocked, holdover, invalid."));

        m.add_leaf(vtss::AsBool(i.holdover_stable),
               vtss::tag::Name("holdoverStable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("True if the stabilization period has expired."));

        m.add_leaf(i.holdover_adj,
               vtss::tag::Name("holdoverAdj"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The calculated holdover offset (ppb*10)."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_status_slave_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);
};

struct StatusClocksUnicastMasterTable {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_unicast_master_table_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP clocks unicast master table.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("slave_ip"));
        h.add_leaf(AsIpv4(i),
               vtss::tag::Name("slaveIp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ptp_unicast_master_table_t &i) {
        h.argument_properties(vtss::tag::Name("unicast_master_table"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_unicast_master_table_t"));

        m.add_leaf(i.mac,
               vtss::tag::Name("slaveMac"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("MAC address of the slave."));

        m.add_leaf(i.port,
               vtss::tag::Name("port"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Port on the master that slave is connected to."));

        m.add_leaf(i.ann_log_msg_period,
               vtss::tag::Name("annLogMsgPeriod"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The granted Announce interval. Range -3 to 4."));

        m.add_leaf(AsBool(i.ann),
               vtss::tag::Name("ann"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("True if sending announce messages."));

        m.add_leaf(i.log_msg_period,
               vtss::tag::Name("logMsgPeriod"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("The granted sync interval. Range -7 to 4."));

        m.add_leaf(AsBool(i.sync),
               vtss::tag::Name("sync"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("True if sending sync messages."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_status_unicast_master_table_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_slave_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);
};

struct StatusClocksUnicastSlaveTable {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_unicast_slave_table_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP clocks unicast slave table.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("master_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("masterId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ptp_unicast_slave_table_t &i) {
        h.argument_properties(vtss::tag::Name("unicast_slave_table"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_unicast_slave_table_t"));

        m.add_leaf(AsIpv4(i.master.ip),
               vtss::tag::Name("masterIp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("This is the IP address of the master."));

        m.add_leaf(i.master.mac,
               vtss::tag::Name("masterMac"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("This is the MAC address of the master."));

        m.add_leaf(AsOctetString(i.sourcePortIdentity.clockIdentity, 8),
               vtss::tag::Name("sourcePortIdentityClockIdentity"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("This is the 8 byte unique clock identity of the source port."));

        m.add_leaf(i.sourcePortIdentity.portNumber,
               vtss::tag::Name("sourcePortIdentityPortNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("This is port number of the port used on the source."));

        m.add_leaf(i.port,
               vtss::tag::Name("port"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("The port (on the slave) connected to the master."));

        m.add_leaf(i.log_msg_period,
               vtss::tag::Name("logMsgPeriod"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("The granted sync interval."));

        m.add_leaf(i.comm_state,
               vtss::tag::Name("commState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("Communication state."));

        m.add_leaf(AsIpv4(i.conf_master_ip),
               vtss::tag::Name("confMasterIp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("Copy of the destination ip address."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_status_unicast_slave_table_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_master_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);
};

struct StatusClocksPortsDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamKey<vtss_ifindex_t>,
                              expose::ParamVal<vtss_appl_ptp_status_port_ds_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP clocks PortDS.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i) {
        h.argument_properties(vtss::tag::Name("port_id"));
        h.add_leaf(AsInterfaceIndex(i),
               vtss::tag::Name("portId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ptp_status_port_ds_t &i) {
        h.argument_properties(vtss::tag::Name("port_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_status_port_ds_t"));

        m.add_leaf(i.portState,
               vtss::tag::Name("portState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The state of the port. Valid options : initializing, faulty, "
                                      "disabled, listening, preMaster, master, passive, "
                                      "uncalibrated, slave, p2pTransparent, e2eTransparent and frontend."));

        m.add_leaf(i.logMinDelayReqInterval,
               vtss::tag::Name("logMinDelayReqInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("The delay request interval announced by the master. Range -7 to 5."));

        m.add_leaf(i.peerMeanPathDelay,
               vtss::tag::Name("peerMeanPathDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The path delay measured in ns by the port in P2P mode."));

        m.add_leaf(vtss::AsBool(i.peer_delay_ok),
               vtss::tag::Name("peerDelayOk"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("False if portmode is P2P and peer delay has not been measured."));

//        m.add_leaf(i.portIdentity,
//               vtss::tag::Name("portIdentity"),
//               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
//               vtss::tag::Description("-"));

        m.add_leaf(i.s_802_1as.portRole,
               vtss::tag::Name("s8021asPortRole"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("Port role of this port. Valid options : disabledPort, masterPort, passivePort and slavePort."));

        m.add_leaf(vtss::AsBool(i.s_802_1as.peer_d.isMeasuringDelay),
               vtss::tag::Name("s8021asIsMeasuringDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("TRUE if the port is measuring link propagation delay."));

        m.add_leaf(vtss::AsBool(i.s_802_1as.asCapable),
               vtss::tag::Name("s8021asAsCapable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("TRUE if the time-aware system at the other end of the link is 802.1AS capable."));

        m.add_leaf(i.s_802_1as.peer_d.neighborRateRatio,
               vtss::tag::Name("s8021asNeighborRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("Calculated neighbor rate ratio expressed as the fractional frequency offset multiplied by 2**41."));

        m.add_leaf(i.s_802_1as.currentLogAnnounceInterval,
               vtss::tag::Name("s8021asCurrentLogAnnounceInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("log2 of the current announce interval."));

        m.add_leaf(i.s_802_1as.currentLogSyncInterval,
               vtss::tag::Name("s8021asCurrentLogSyncInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("log2 of the current sync interval."));

        m.add_leaf(i.s_802_1as.syncReceiptTimeInterval,
               vtss::tag::Name("s8021asSyncReceiptTimeInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(14),
               vtss::tag::Description("Time interval after which sync receipt timeout occurs if time-synchronization information has not been received during the interval."));

        m.add_leaf(i.s_802_1as.peer_d.currentLogPDelayReqInterval,
               vtss::tag::Name("s8021asCurrentLogPDelayReqInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(15),
               vtss::tag::Description("log2 of the current Pdelay_Req interval."));

       m.add_leaf(vtss::AsBool(i.s_802_1as.acceptableMasterTableEnabled),
               vtss::tag::Name("s8021asAcceptableMasterTableEnabled"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(16),
               vtss::tag::Description("Always FALSE."));

       m.add_leaf(i.s_802_1as.peer_d.versionNumber,
               vtss::tag::Name("s8021asVersionNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(17),
               vtss::tag::Description("IEEE 1588 PTP version number (always 2)."));

       m.add_leaf(vtss::AsBool(i.s_802_1as.peer_d.currentComputeNeighborRateRatio),
               vtss::tag::Name("s8021asCurrentCompNbrRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(18),
               vtss::tag::Description("current value of computeNeighborRateRatio."));

       m.add_leaf(vtss::AsBool(i.s_802_1as.peer_d.currentComputeMeanLinkDelay),
               vtss::tag::Name("s8021asCurrentCompMeanLinkDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(19),
               vtss::tag::Description("current value of computeMeanLinkDelay."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_status_clocks_port_ds_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_port_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);
};

struct StatusCmldsDefaultDs {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_ptp_802_1as_cmlds_default_ds_t *>>
            P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ptp_802_1as_cmlds_default_ds_t &i) {
        h.argument_properties(vtss::tag::Name("ptp_802_1as_cmlds_default_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_802_1as_cmlds_default_ds_t"));

        m.add_leaf(AsOctetString(i.clockIdentity,8),
               vtss::tag::Name("clockIdentity"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Identity of the PTP clock associated with common link delay service."));

        m.add_leaf(i.numberLinkPorts,
               vtss::tag::Name("numberLinkPorts"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("number of Link Ports of the time-aware system on which the Common Mean Link Delay Service can be enabled."));

        m.add_leaf(i.sdoId,
               vtss::tag::Name("sdoId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("sdoID for Common Mean Link Delay Service."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_cmlds_default_ds_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP);

};

struct StatusCmldsPortDs {
    typedef expose::ParamList<expose::ParamKey<vtss_uport_no_t>,
            expose::ParamVal<vtss_appl_ptp_802_1as_cmlds_status_port_ds_t *>> P;

    static constexpr const char *table_description = "This is the dynamic part of the PTP clocks CMLDS PortDS.";

    static constexpr const char *index_description = "The port_id index must be a value from 0 up to the number of ports.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_uport_no_t &i) {
        h.argument_properties(vtss::tag::Name("port_id"));
        h.add_leaf(i,
               vtss::tag::Name("portIdentityPortNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 255),
               vtss::tag::Description("This is the port number on which CMLDS can be enabled."));

    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_802_1as_cmlds_status_port_ds_t &i) {
        h.argument_properties(vtss::tag::Name("cmlds_port_ds"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_802_1as_cmlds_status_port_ds_t"));

        m.add_leaf(AsOctetString(i.portIdentity.clockIdentity, 8),
               vtss::tag::Name("portIdentityClockIdentity"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("This is the 8 byte unique clock identity of the parent port."));

        m.add_leaf(vtss::AsBool(i.cmldsLinkPortEnabled),
               vtss::tag::Name("cmldsLinkPortEnabled"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("This value is true if CMLDS is used on this port by atleast one PTP clock."));

        m.add_leaf(vtss::AsBool(i.peer_d.isMeasuringDelay),
               vtss::tag::Name("isMeasuringDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("true if the CMLDS service is measuring link propagation delay on this port."));

        m.add_leaf(vtss::AsBool(i.asCapableAcrossDomains),
               vtss::tag::Name("asCapableAcrossDomains"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("true if CMLDS service reaches asCapable state when the conditions for neighbor rate ratio and link delay are met."));

        m.add_leaf(i.meanLinkDelay,
               vtss::tag::Name("meanLinkDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("the measured mean propagation delay on the link attached to this port"));

        m.add_leaf(i.peer_d.neighborRateRatio,
               vtss::tag::Name("neighborRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description(" Calculated neighbor rate ratio expressed as the fractional frequency offset multiplied by 2**41."));

        m.add_leaf(i.peer_d.currentLogPDelayReqInterval,
               vtss::tag::Name("currentLogPDelayReqInterval"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("the value is the logarithm to the base 2 of the current Pdelay_Req message transmission interval."));

        m.add_leaf(vtss::AsBool(i.peer_d.currentComputeNeighborRateRatio),
               vtss::tag::Name("currentCompNbrRateRatio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("true if Neighbor rate ratio is being computed."));

        m.add_leaf(vtss::AsBool(i.peer_d.currentComputeMeanLinkDelay),
               vtss::tag::Name("currentCompMeanLinkDelay"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("true if MeanLinkDelay is being measured by CMLDS service currently."));

        m.add_leaf(i.peer_d.versionNumber,
               vtss::tag::Name("versionNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("Version number of IEEE 1588 PTP used in the PTP 802.1as profile."));

        m.add_leaf(i.peer_d.minorVersionNumber,
               vtss::tag::Name("minorVersionNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Minor version number set to 1 for transmitted messages of 802.1as profile."));

    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_cmlds_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_port_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};
struct StatisticsClocksPortsDS {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamKey<vtss_ifindex_t>,
                              expose::ParamVal<vtss_appl_ptp_status_port_statistics_t *>> P;

    static constexpr const char *table_description = "This is the dynamic (status) part of the PTP port parameter statistics.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_ifindex_t &i) {
        h.argument_properties(vtss::tag::Name("port_id"));
        h.add_leaf(AsInterfaceIndex(i),
               vtss::tag::Name("portId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_ptp_status_port_statistics_t &i) {
        h.argument_properties(vtss::tag::Name("port_statistics"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_status_port_statistics_t"));

        m.add_leaf(i.rxSyncCount,
               vtss::tag::Name("rxSyncCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Increments every time synchronization is received."));

        m.add_leaf(i.rxFollowUpCount,
               vtss::tag::Name("rxFollowUpCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Increments every time a Follow_Up is received."));

        m.add_leaf(i.peer_d.rxPdelayRequestCount,
               vtss::tag::Name("rxPdelayRequestCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Increments every time a Pdelay_Req is received."));

        m.add_leaf(i.peer_d.rxPdelayResponseCount,
               vtss::tag::Name("rxPdelayResponseCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Increments every time a Pdelay_Resp is received."));

        m.add_leaf(i.peer_d.rxPdelayResponseFollowUpCount,
               vtss::tag::Name("rxPdelayResponseFollowUpCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("Increments every time a Pdelay_Resp_Follow_Up is received."));

        m.add_leaf(i.rxAnnounceCount,
               vtss::tag::Name("rxAnnounceCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("Increments every time a Announce message is received."));

        m.add_leaf(i.rxPTPPacketDiscardCount,
               vtss::tag::Name("rxPTPPacketDiscardCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("Increments every time a PTP message is discarded due to the conditions described in IEEE 802.1AS clause 14.7.8."));

        m.add_leaf(i.syncReceiptTimeoutCount,
               vtss::tag::Name("syncReceiptTimeoutCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("Increments every time sync receipt timeout occurs."));

        m.add_leaf(i.announceReceiptTimeoutCount,
               vtss::tag::Name("announceReceiptTimeoutCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("Increments every time announce receipt timeout occurs."));

        m.add_leaf(i.peer_d.pdelayAllowedLostResponsesExceededCount,
               vtss::tag::Name("pdelayAllowedLostResExcCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Increments every time the value of the variable lostResponses exceeds the value of the variable allowedLostResponses."));

        m.add_leaf(i.txSyncCount,
               vtss::tag::Name("txSyncCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("Increments every time synchronization information is transmitted."));

        m.add_leaf(i.txFollowUpCount,
               vtss::tag::Name("txFollowUpCount"),
                   vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(14),
               vtss::tag::Description("Increments every time a Follow_Up message is transmitted."));

        m.add_leaf(i.peer_d.txPdelayRequestCount,
               vtss::tag::Name("txPdelayRequestCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(15),
               vtss::tag::Description("Increments every time a Pdelay_Req message is transmitted."));

        m.add_leaf(i.peer_d.txPdelayResponseCount,
               vtss::tag::Name("txPdelayResponseCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(16),
               vtss::tag::Description("Increments every time a Pdelay_Resp message is transmitted."));

        m.add_leaf(i.peer_d.txPdelayResponseFollowUpCount,
               vtss::tag::Name("txPdelayResponseFollowUpCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(17),
               vtss::tag::Description("Increments every time a Pdelay_Resp_Follow_Up message is transmitted."));

        m.add_leaf(i.txAnnounceCount,
               vtss::tag::Name("txAnnounceCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(18),
               vtss::tag::Description("Increments every time an Announce message is transmitted."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_status_clocks_port_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_port_itr);
};

struct StatisticsCmldsPortDs {
    typedef expose::ParamList<expose::ParamKey<vtss_uport_no_t>,
            expose::ParamVal<vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t *>> P;

    static constexpr const char *table_description = "This is the statistics part of Common Mean Link Delay Service Port.";

    static constexpr const char *index_description = "The port_id index must be a value from 0 up to the number of ports.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_uport_no_t &i) {
        h.argument_properties(vtss::tag::Name("portId"));
        h.add_leaf(i,
               vtss::tag::Name("portIdentityPortNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 255),
               vtss::tag::Description("This is the port number of the Common Mean Link Delay Service enabled port."));

    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t &i) {
        h.argument_properties(vtss::tag::Name("cmldsPortStatistics"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t"));

        m.add_leaf(i.peer_d.rxPdelayRequestCount,
               vtss::tag::Name("rxPdelayRequestCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Increments every time a Pdelay_Req is received."));

        m.add_leaf(i.peer_d.rxPdelayResponseCount,
               vtss::tag::Name("rxPdelayResponseCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Increments every time a Pdelay_Response is received."));

        m.add_leaf(i.peer_d.rxPdelayResponseFollowUpCount,
               vtss::tag::Name("rxPdelayResponseFollowUpCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Increments every time a PdelayResponseFollowup is received."));

        m.add_leaf(i.rxPTPPacketDiscardCount,
               vtss::tag::Name("rxPTPPacketDiscardCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Increments every time a PTP message of Common Mean Link Delay Service is discarded."));

        m.add_leaf(i.peer_d.pdelayAllowedLostResponsesExceededCount,
               vtss::tag::Name("pdelayAllowedLostResExcCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Increments every time the value of the variable lostResponses exceeds the value of the variable allowedLostResponses."));

        m.add_leaf(i.peer_d.txPdelayRequestCount,
               vtss::tag::Name("txPdelayRequestCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("Increments every time a Pdelay_Req message is transmitted."));

        m.add_leaf(i.peer_d.txPdelayResponseCount,
               vtss::tag::Name("txPdelayResponseCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("Increments every time a Pdelay_Resp message is transmitted."));

        m.add_leaf(i.peer_d.txPdelayResponseFollowUpCount,
               vtss::tag::Name("txPdelayResponseFollowUpCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("Increments every time a Pdelay_Resp_Follow_Up message is transmitted."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_cmlds_port_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_port_itr);
};

struct ControlClocks {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_ptp_clock_control_t *>> P;

    static constexpr const char *table_description = "This is the PTP clocks control structure.";

    static constexpr const char *index_description = "The clockId index must be a value must be a value from 0 up to the number of PTP clocks minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.argument_properties(vtss::tag::Name("clock_id"));
        h.add_leaf(AsInt(i),
               vtss::tag::Name("clockId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ptp_clock_control_t &i) {
        h.argument_properties(vtss::tag::Name("clock_control"));
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_ptp_clock_control_t"));

        m.add_leaf(i.syncToSystemClock,
               vtss::tag::Name("syncToSystemClock"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ptp_clock_control_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ptp_clock_control_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ptp_clock_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP);
};

} // namespace interfaces
} // namespace ptp
} // namespace appl
} // namespace vtss

#endif // _VTSS_APPL_PTP_SERIALIZER_HXX_
