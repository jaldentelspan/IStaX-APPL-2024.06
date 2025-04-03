/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_APPL_SYNCE_SERIALIZER_HXX_
#define _VTSS_APPL_SYNCE_SERIALIZER_HXX_

#include "vtss/appl/synce.h"
#include "synce.h"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/module_id.h"

extern "C" void vtss_synce_mib_init();

// Serialize enums

VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_synce_selection_mode_t, "synceSelectionMode", vtss_appl_synce_selection_mode_txt, "-");
VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_synce_quality_level_t, "synceQualityLevel", vtss_appl_synce_quality_level_txt, "-");
VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_synce_eec_option_t, "synceEecOption", vtss_appl_synce_eec_option_txt, "-");
VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_synce_aneg_mode_t, "synceAnegMode", vtss_appl_synce_aneg_mode_txt, "-");
VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_synce_selector_state_t, "synceSelectorState", vtss_appl_synce_selector_state_txt, "-");
VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_synce_frequency_t, "synceFrequency", vtss_appl_synce_frequency_txt, "-");
VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_synce_ptp_ptsf_state_t, "syncePtsfState", vtss_appl_synce_ptp_ptsf_state_txt, "-");
VTSS_SNMP_SERIALIZE_ENUM(vtss_appl_synce_lol_alarm_state_t, "synceLolAlarmState", vtss_appl_synce_lol_alarm_state_txt, "-");
VTSS_SNMP_SERIALIZE_ENUM(meba_synce_clock_hw_id_t, "synceDpllHwType", vtss_appl_synce_clock_hw_id_txt, "-");


VTSS_JSON_SERIALIZE_ENUM(vtss_appl_synce_selection_mode_t, "synceSelectionMode", vtss_appl_synce_selection_mode_txt, "-");
VTSS_JSON_SERIALIZE_ENUM(vtss_appl_synce_quality_level_t, "synceQualityLevel", vtss_appl_synce_quality_level_txt, "-");
VTSS_JSON_SERIALIZE_ENUM(vtss_appl_synce_eec_option_t, "synceEecOption", vtss_appl_synce_eec_option_txt, "-");
VTSS_JSON_SERIALIZE_ENUM(vtss_appl_synce_aneg_mode_t, "synceAnegMode", vtss_appl_synce_aneg_mode_txt, "-");
VTSS_JSON_SERIALIZE_ENUM(vtss_appl_synce_selector_state_t, "synceSelectorState", vtss_appl_synce_selector_state_txt, "-");
VTSS_JSON_SERIALIZE_ENUM(vtss_appl_synce_frequency_t, "synceFrequency", vtss_appl_synce_frequency_txt, "-");
VTSS_JSON_SERIALIZE_ENUM(vtss_appl_synce_ptp_ptsf_state_t, "syncePtsfState", vtss_appl_synce_ptp_ptsf_state_txt, "-");
VTSS_JSON_SERIALIZE_ENUM(vtss_appl_synce_lol_alarm_state_t, "synceLolAlarmState", vtss_appl_synce_lol_alarm_state_txt, "-");
VTSS_JSON_SERIALIZE_ENUM(meba_synce_clock_hw_id_t, "synceDpllHwType", vtss_appl_synce_clock_hw_id_txt, "-");


namespace vtss {
namespace appl {
namespace synce {
namespace interfaces {

struct SynceCapHasPtp {
    static constexpr const char *json_ref = "vtss_appl_synce_capabilities_t";
    static constexpr const char *name = "HasPtp";
    static constexpr const char *desc = "If true, the build supports PTP clocks as sources for SyncE.";
    static constexpr bool get() {

#if defined(VTSS_SW_OPTION_PTP)
    return true;
#else
    return false;
#endif
    }
};

struct CapabilitiesGlobal {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_synce_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_synce_capabilities_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_capabilities_t"));

        m.add_leaf(i.synce_source_count,
               vtss::tag::Name("sourceCount"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The number of SyncE sources supported by the device."));

        m.template capability<SynceCapHasPtp>(vtss::expose::snmp::OidElementValue(2));

        m.add_leaf(static_cast<meba_synce_clock_hw_id_t>(i.dpll_type),
               vtss::tag::Name("dpllType"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The type of dpll supported by the device."));

        m.add_leaf(i.clock_type,
               vtss::tag::Name("clockType"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("0 = Full featured clock type, i.e. supports both in and out, and 1,544, 2,048 and 10 MHz : 1 = PCB104, support only 2,048 and 10 MHz clock output : 2 = others, no station clock support      : 3 = ServalT, supports in frequency 1,544, 2,048 and 10 MHz, support only 10 MHz clock output"));

        m.add_leaf(static_cast<meba_synce_clock_fw_ver_t>(i.dpll_fw_ver),
               vtss::tag::Name("dpllFwVer"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("The firmware version of DPLL."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_capabilities_global_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct PossibleSources {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_synce_clock_possible_source_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.add_leaf(AsInt(i),
               vtss::tag::Name("sourceId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("A source Id"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(unsigned int &i) {
        h.add_leaf(AsInt(i),
               vtss::tag::Name("index"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_synce_clock_possible_source_t &s) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_clock_possible_source_t"));
        m.add_leaf(AsInterfaceIndex(s.network_port),
               vtss::tag::Name("networkPort"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("-"));
        m.add_leaf(AsInt(s.clk_in_port),
               vtss::tag::Name("clockInPort"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_clock_source_ports_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_synce_clock_source_ports_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct ConfigGlobalClockSelectionMode {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_synce_clock_selection_mode_config_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_synce_clock_selection_mode_config_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_clock_selection_mode_config_t"));

        m.add_leaf(i.selection_mode,
               vtss::tag::Name("selectionMode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The selection mode."));

        m.add_leaf(i.source,
               vtss::tag::Name("source"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Nominated source for manuel selection mode."));

        m.add_leaf(i.wtr_time,
               vtss::tag::Name("wtrTime"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("WTR timer value in minutes. Range is 0 to 12 minutes where 0 means that the timer is disabled."));

        m.add_leaf(i.ssm_holdover,
               vtss::tag::Name("ssmHoldover"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Tx overwrite SSM used when clock controller is hold over."));

        m.add_leaf(i.ssm_freerun,
               vtss::tag::Name("ssmFreerun"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Tx overwrite SSM used when clock controller is free run."));

        m.add_leaf(i.eec_option,
               vtss::tag::Name("eecOption"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Synchronous Ethernet Equipment Clock option."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_clock_selection_mode_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_synce_clock_selection_mode_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct ConfigGlobalStationClocks {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_synce_station_clock_config_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_synce_station_clock_config_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_station_clock_config_t"));
        
        m.add_leaf(i.station_clk_out,
               vtss::tag::Name("stationClkOut"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Station clock output frequency setting."));

        m.add_leaf(i.station_clk_in,
               vtss::tag::Name("stationClkIn"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Station clock input frequency setting."));               
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_station_clock_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_synce_station_clock_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct ConfigSourcesClockSourceNomination {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_synce_clock_source_nomination_config_t *>> P;

    static constexpr const char *table_description = "This is the SyncE source nomination configuration.";

    static constexpr const char *index_description = "The sourceId index must be a value from 0 up to the number of sources minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.add_leaf(AsInt(i),
               vtss::tag::Name("sourceId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_synce_clock_source_nomination_config_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_clock_source_nomination_config_t"));

        m.add_leaf(vtss::AsBool(i.nominated),
               vtss::tag::Name("nominated"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Indicates if source is nominated."));

        m.add_leaf(vtss::AsInterfaceIndex(i.network_port),
               vtss::tag::Name("networkPort"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Interface index of the norminated source."));

        m.add_leaf(i.clk_in_port,
               vtss::tag::Name("clkInPort"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Clock input of the norminated source."));

        m.add_leaf(i.priority,
               vtss::tag::Name("priority"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Priority of the nominated source."));

        m.add_leaf(i.aneg_mode,
               vtss::tag::Name("anegMode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Autonogotiation mode auto-master-slave."));

        m.add_leaf(i.ssm_overwrite,
               vtss::tag::Name("ssmOverwrite"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("SSM overwrite quality."));

        m.add_leaf(i.holdoff_time,
               vtss::tag::Name("holdoffTime"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("Hold Off timer value in 100ms (3 - 18). Zero means no hold off."));               
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_clock_source_nomination_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_synce_source_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_synce_clock_source_nomination_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct ConfigPortsPortConfig {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
                              expose::ParamVal<vtss_appl_synce_port_config_t *>> P;

    static constexpr const char *table_description = "This is the SyncE port configuration.";

    static constexpr const char *index_description = "The portId index must be a value from 0 up to the number of ports minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.add_leaf(vtss::AsInterfaceIndex(i),
               vtss::tag::Name("portId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_synce_port_config_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_port_config_t"));
        
        m.add_leaf(vtss::AsBool(i.ssm_enabled),
               vtss::tag::Name("ssmEnabled"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Quality level via SSM enabled."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_port_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_synce_port_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_synce_port_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct StatusGlobalClockSelectionMode {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_synce_clock_selection_mode_status_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_synce_clock_selection_mode_status_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_clock_selection_mode_status_t"));

        m.add_leaf(i.clock_input+1,
               vtss::tag::Name("clockInput"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The clock source locked to when clock selector is in locked state."
               "A clock source with value 5 indicates no clock source"));

        m.add_leaf(i.selector_state,
               vtss::tag::Name("selectorState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("This is indicating the state of the clock selector."));

        m.add_leaf(AsBool(i.losx),
               vtss::tag::Name("losx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("LOSX"));

        m.add_leaf(i.lol,
               vtss::tag::Name("lol"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Clock selector has raised the Los Of Lock alarm."));

        m.add_leaf(AsBool(i.dhold),
               vtss::tag::Name("dhold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Clock selector has not yet calculated the holdover frequency offset to local oscillator."));               
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_clock_selection_mode_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct StatusSourcesClockSourceNomination {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_synce_clock_source_nomination_status_t *>> P;

    static constexpr const char *table_description = "This is the clock source nomination status.";

    static constexpr const char *index_description = "The sourceId index must be a value must be a value from 0 up to the number of sources minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.add_leaf(AsInt(i),
               vtss::tag::Name("sourceId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_synce_clock_source_nomination_status_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_clock_source_nomination_status_t"));

        m.add_leaf(AsBool(i.locs),
               vtss::tag::Name("locs"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("LOCS"));

        m.add_leaf(AsBool(i.fos),
               vtss::tag::Name("fos"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("FOS"));

        m.add_leaf(AsBool(i.ssm),
               vtss::tag::Name("ssm"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("SSM"));

        m.add_leaf(AsBool(i.wtr),
               vtss::tag::Name("wtr"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("WTR"));
              
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_clock_source_nomination_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_synce_source_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct StatusPortsPortStatus {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
                              expose::ParamVal<vtss_appl_synce_port_status_t *>> P;

    static constexpr const char *table_description = "This is the port status.";

    static constexpr const char *index_description = "The portId index must be a value must be a value from 0 up to the number of ports minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.add_leaf(vtss::AsInterfaceIndex(i),
               vtss::tag::Name("portId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_synce_port_status_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_port_status_t"));

        m.add_leaf(i.ssm_rx,
               vtss::tag::Name("ssmRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Monitoring of the received SSM QL on this port."));

        m.add_leaf(i.ssm_tx,
               vtss::tag::Name("ssmTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Monitoring of the transmitted SSM QL on this port."));

        m.add_leaf(AsBool(i.master),
               vtss::tag::Name("master"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("If PHY is in 1000BaseT Mode then this is monitoring the master/slave mode."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_synce_port_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct StatusPtpPortStatus {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_synce_ptp_port_status_t *>> P;

    typedef SynceCapHasPtp depends_on_t;

    static constexpr const char *table_description = "This is the PTP port status.";

    static constexpr const char *index_description = "The sourceId index must be a value must be a value from 0 up to the number of PTP sources minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.add_leaf(AsInt(i),
               vtss::tag::Name("sourceId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_synce_ptp_port_status_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_ptp_port_status_t"));
        m.add_leaf(i.ssm_rx,
               vtss::tag::Name("ssmRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Monitoring of the received SSM QL on this port."));

        m.add_leaf(i.ptsf,
               vtss::tag::Name("ptsf"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("PTSF status for PTP source."));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_ptp_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_synce_ptp_source_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYNCE);
};

struct ControlSourcesClockSourceNomination {
    typedef expose::ParamList<expose::ParamKey<unsigned int>,
                              expose::ParamVal<vtss_appl_synce_clock_source_control_t *>> P;

    static constexpr const char *table_description = "This is the SyncE sources control structure.";

    static constexpr const char *index_description = "The sourceId index must be a value must be a value from 0 up to the number of sources minus one.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(unsigned int &i) {
        h.add_leaf(AsInt(i),
               vtss::tag::Name("sourceId"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::expose::snmp::RangeSpec<u32>(0, 32767),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_synce_clock_source_control_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_synce_clock_source_control_t"));

        m.add_leaf(i.clearWtr,
               vtss::tag::Name("clearWtr"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("-"));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_synce_clock_source_control_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_synce_clock_source_control_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_synce_source_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYNCE);
};

} // namespace interfaces
} // namespace synce
} // namespace appl
} // namespace vtss

#endif // _VTSS_APPL_SYNCE_SERIALIZER_HXX_
