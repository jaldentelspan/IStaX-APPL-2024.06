/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_SYSUTIL_SERIALIZER_HXX__
#define __VTSS_SYSUTIL_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/sysutil.h"

/*****************************************************************************
    Enum serializer
*****************************************************************************/

/******************************************************************************/
// sysutil_control_reboot_set()
// Before we execute all the reboot callback handlers, we must take a break of,
// say, 3 seconds in order to get a reply out of SNMP before ports are shut
// down. This function simply wraps the public ditto function with this
// additional, optional parameter.
/******************************************************************************/
static mesa_rc sysutil_control_reboot_set(vtss_usid_t usid, const vtss_appl_sysutil_control_reboot_t *const reboot)
{
    return vtss_appl_sysutil_control_reboot_set(usid, reboot, 3000 /* milliseconds */);
}

extern const vtss_enum_descriptor_t vtss_appl_sysutil_reboot_type_txt[];
extern const vtss_enum_descriptor_t vtss_appl_sysutil_psu_state_txt[];
extern const vtss_enum_descriptor_t vtss_appl_sysutil_system_led_clear_type_txt[];
extern const vtss_enum_descriptor_t vtss_appl_sysutil_tm_sensor_type_txt[];

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_sysutil_reboot_type_t,
    "SysutilRebootType",
    vtss_appl_sysutil_reboot_type_txt,
    "This enumeration defines the type of reboot.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_sysutil_psu_state_t,
    "SysutilPowerSupplyStateType",
    vtss_appl_sysutil_psu_state_txt,
    "This enumeration defines the type of power supply state.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_sysutil_system_led_clear_type_t,
    "SysutilSystemLedClearType",
    vtss_appl_sysutil_system_led_clear_type_txt,
    "This enumeration defines the type of system LED status clearing.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_sysutil_tm_sensor_type_t,
    "SysutilTemperatureMonitorSensorType",
    vtss_appl_sysutil_tm_sensor_type_txt,
    "This enumeration defines the type of temperature sensors.");
#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
/*****************************************************************************
 - JSON notification serializer
*****************************************************************************/
extern vtss::expose::TableStatus <
	vtss::expose::ParamKey<vtss_appl_sysutil_tm_sensor_type_t>,
	vtss::expose::ParamVal<vtss_appl_sysutil_tm_status_t *>
    > tm_status_event_update;
#endif

/*****************************************************************************
    Index serializer
*****************************************************************************/
// Table index of SysUtilPowerSupplyTable
VTSS_SNMP_TAG_SERIALIZE(sysutil_usid_index, u32, a, s)
{
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("SwitchId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(1, 16),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The identification of switch.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(sysutil_psuid_index, u32, a, s)
{
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("PsuId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(1, 2),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The identification of power supply.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(sysutil_tm_sensor_index, vtss_appl_sysutil_tm_sensor_type_t, a, s)
{
    a.add_leaf(
        s.inner,
        vtss::tag::Name("SensorId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The identification of sensor for tempeature monitor.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_sysutil_capabilities_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_capabilities_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.warmReboot),
        vtss::tag::Name("WarmRebootSupported"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicate if warm restart is supported or not. "
            "true means it is supported. "
            "false means it is not supported.")
    );

    m.add_leaf(
        vtss::AsBool(s.post),
        vtss::tag::Name("PostSupported"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicate if POST(Power On Self Test) is supported or not. "
            "true(1) means it is supported. "
            "false(2) means it is not supported.")
    );

    m.add_leaf(
        vtss::AsBool(s.ztp),
        vtss::tag::Name("ZtpSupported"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicate if ZTP(Zero Touch Provisioning) is supported or not. "
            "true(1) means it is supported. "
            "false(2) means it is not supported.")
    );

    m.add_leaf(
        vtss::AsBool(s.stack_fw_chk),
        vtss::tag::Name("StackFwChkSupported"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicate if stack firmware version check is supported or not. "
            "true(1) means it is supported. "
            "false(2) means it is not supported.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_status_cpu_load_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_status_cpu_load_t"));
    int ix = 0;

    m.add_leaf(
        s.average100msec,
        vtss::tag::Name("Average100msec"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Average CPU load (%) in 100 milli-seconds.")
    );

    m.add_leaf(
        s.average1sec,
        vtss::tag::Name("Average1sec"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Average CPU load (%) in 1 second.")
    );

    m.add_leaf(
        s.average10sec,
        vtss::tag::Name("Average10sec"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Average CPU load (%) in 10 seconds.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_control_reboot_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_control_reboot_t"));
    int ix = 0;

    m.add_leaf(
        s.type,
        vtss::tag::Name("Type"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of reboot. "
            "noReboot(0) does not reboot. "
            "coldReboot(1) is to do cold reboot. "
            "warmReboot(2) is to do warm reboot, but this is optional. "
            "The OID of vtssSysutilCapabilitiesWarmRebootSupported tells if "
            "warm reboot is supported or not."
            )
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_psu_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_psu_status_t"));
    int ix = 0;

    m.add_leaf(
        s.state,
        vtss::tag::Name("State"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The state of power supply.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.descr, VTSS_APPL_SYSUTIL_PSU_DESCR_MAX_LEN),
        vtss::tag::Name("Description"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The description of power supply.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_system_led_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_system_led_status_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.descr, VTSS_APPL_SYSUTIL_SYSTEM_LED_DESCR_MAX_LEN),
        vtss::tag::Name("Description"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The description of system LED status.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_control_system_led_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_control_system_led_t"));
    int ix = 0;

    m.add_leaf(
        s.type,
        vtss::tag::Name("ClearStatus"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of system LED status clearing."
            "all(0) is used to clear all error status of the system LED and back to normal indication. "
            "fatal(1) is used to clear fatal error status of the system LED. "
            "software(2) is used to clear generic software error status of the system LED. "
            "post(3) is used to clear POST(Power On Self Test) error status of the system LED. "
            "ztp(4) is used to clear ZTP(Zero Touch Provisioning) error status of the system LED. "
            "stackFwChk(5) is used to clear stack firmware version check error status of the system LED."
            )
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_board_info_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_board_info_t"));
    int ix = 0;

    m.add_leaf(
        s.mac,
        vtss::tag::Name("BoardMacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Board Mac Address")
               );

    m.add_leaf(
        s.board_id,
        vtss::tag::Name("BoardID"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Board ID")
               );

    m.add_leaf(
        vtss::AsDisplayString(s.board_serial, sizeof(s.board_serial)),
        vtss::tag::Name("BoardSerial"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Board ID")
               );

    m.add_leaf(
        vtss::AsDisplayString(s.board_type, sizeof(s.board_type)),
        vtss::tag::Name("BoardType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Board ID")
               );

}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_sys_uptime_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_sys_uptime_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.sys_uptime, VTSS_APPL_SYSUTIL_SYSTEM_UPTIME_LEN),
        vtss::tag::Name("SystemUptime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The time since the DUT is Up.")
    );

}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_sys_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_sys_conf_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.sys_name, VTSS_APPL_SYS_STRING_LEN),
        vtss::tag::Name("Hostname"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Hostname")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.sys_contact, VTSS_APPL_SYS_STRING_LEN),
        vtss::tag::Name("Contact"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Contact name.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.sys_location, VTSS_APPL_SYS_STRING_LEN),
        vtss::tag::Name("Location"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Location.")
    );

}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_system_time_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_system_time_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.sys_curtime, VTSS_APPL_SYSUTIL_SYSTEM_TIME_LEN),
        vtss::tag::Name("SystemCurTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Current system time")
               );

    m.add_leaf(
        vtss::AsDisplayString(s.sys_curtime_format, VTSS_APPL_SYSUTIL_SYSTEM_TIME_LEN),
        vtss::tag::Name("SystemCurTimeFormat"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Format for setting up current system time")
               );
}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_tm_config_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_tm_config_t"));
    int ix = 0;

    m.add_leaf(
        s.low,
        vtss::tag::Name("LowThreshold"),
        vtss::expose::snmp::RangeSpec<int>(VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_LOW, VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_HIGH),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The low threshold of temperature monior.")
    );

    m.add_leaf(
        s.high,
        vtss::tag::Name("HighThreshold"),
        vtss::expose::snmp::RangeSpec<int>(VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_LOW, VTSS_APPL_SYSUTIL_TEMP_MONITOR_RANGE_HIGH),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The high threshold of temperature monior.")
    );

    m.add_leaf(
        s.critical,
        vtss::tag::Name("CriticalThreshold"),
        vtss::expose::snmp::RangeSpec<int>(VTSS_APPL_SYSUTIL_TEMP_MONITOR_CRITICAL_LOW, VTSS_APPL_SYSUTIL_TEMP_MONITOR_CRITICAL_HIGH),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The critical threshold of temperature monior.")
    );

    m.add_leaf(
        s.hysteresis,
        vtss::tag::Name("Hysteresis"),
        vtss::expose::snmp::RangeSpec<int>(VTSS_APPL_SYSUTIL_TEMP_MONITOR_HYSTERESIS_LOW, VTSS_APPL_SYSUTIL_TEMP_MONITOR_HYSTERESIS_HIGH),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The value of hysteresis for temperature check.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sysutil_tm_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sysutil_tm_status_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.low),
        vtss::tag::Name("LowAlarm"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The alarm flag of temperature low status.")
    );

    m.add_leaf(
        vtss::AsBool(s.high),
        vtss::tag::Name("HighAlarm"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The alarm flag of temperature high status.")
    );

    m.add_leaf(
        vtss::AsBool(s.critical),
        vtss::tag::Name("CriticalAlarm"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The alarm flag of temperature critical status.")
    );

    m.add_leaf(
        s.temp,
        vtss::tag::Name("Temperature"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Current temperature.")
    );
}

namespace vtss {
namespace appl {
namespace sysutil {
namespace interfaces {

struct SysutilCapabilitiesLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_sysutil_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilStatusCpuLoadLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_sysutil_status_cpu_load_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_status_cpu_load_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_status_cpu_load_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilPowerSupplyLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_sysutil_psu_status_t *>
    > P;

    static constexpr const char *table_description =
        "Table of power supply status.";

    static constexpr const char *index_description =
        "Each row contains the power supply status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, sysutil_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, sysutil_psuid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_sysutil_psu_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_psu_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sysutil_psu_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilSystemLedLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamVal<vtss_appl_sysutil_system_led_status_t *>
    > P;

    static constexpr const char *table_description =
        "Table of system LED status.";

    static constexpr const char *index_description =
        "Each row contains the system LED status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, sysutil_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sysutil_system_led_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_system_led_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sysutil_usid_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilControlRebootEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamVal<vtss_appl_sysutil_control_reboot_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to reboot a swicth";

    static constexpr const char *index_description =
        "Each switch has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, sysutil_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sysutil_control_reboot_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_control_reboot_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sysutil_control_reboot_itr);
    VTSS_EXPOSE_SET_PTR(sysutil_control_reboot_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilControlSystemLedEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamVal<vtss_appl_sysutil_control_system_led_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to clear the system LED error status";

    static constexpr const char *index_description =
        "Each switch has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, sysutil_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sysutil_control_system_led_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_control_system_led_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sysutil_usid_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_sysutil_control_system_led_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilBoardInfoEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_sysutil_board_info_t *>
        > P;
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_board_info_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_board_info_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilSystemUptimeLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_sysutil_sys_uptime_t *>
        > P;
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_sys_uptime_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_sytem_uptime_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilConfigSystemInfo {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_sysutil_sys_conf_t *>
        > P;
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_sys_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_sysutil_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilSystemTimeInfo {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_sysutil_system_time_t *>
        > P;
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_system_time_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }
    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_system_time_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_sysutil_system_time_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilConfigTemperatureMonitor {
    typedef vtss::expose::ParamList<
		vtss::expose::ParamKey<vtss_appl_sysutil_tm_sensor_type_t>,
		vtss::expose::ParamVal<vtss_appl_sysutil_tm_config_t *>
	> P;

    static constexpr const char *table_description =
        "Table of temperature monitor config.";

    static constexpr const char *index_description =
        "Each row set the sensor config.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_tm_sensor_type_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, sysutil_tm_sensor_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sysutil_tm_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_temperature_monitor_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_sysutil_temperature_monitor_config_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sysutil_temperature_monitor_sensor_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SysutilStatusTemperatureMonitor {
    typedef vtss::expose::ParamList<
		vtss::expose::ParamKey<vtss_appl_sysutil_tm_sensor_type_t>,
		vtss::expose::ParamVal<vtss_appl_sysutil_tm_status_t *>
	> P;

    static constexpr const char *table_description =
        "Table of temperature monitor status.";

    static constexpr const char *index_description =
        "Each row contains the sensor status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_tm_sensor_type_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, sysutil_tm_sensor_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sysutil_tm_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_temperature_monitor_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sysutil_temperature_monitor_sensor_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

/* JSON notification */
struct SysutilEventTemperatureMonitor {
    typedef vtss::expose::ParamList<
		vtss::expose::ParamKey<vtss_appl_sysutil_tm_sensor_type_t>,
		vtss::expose::ParamVal<vtss_appl_sysutil_tm_status_t *>
	> P;

    static constexpr const char *table_description =
        "Table of temperature monitor event status.";

    static constexpr const char *index_description =
        "Each row contains the event status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sysutil_tm_sensor_type_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, sysutil_tm_sensor_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sysutil_tm_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sysutil_temperature_monitor_event_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sysutil_temperature_monitor_sensor_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

}  // namespace interfaces
}  // namespace sysutil
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_SYSUTIL_SERIALIZER_HXX__ */
