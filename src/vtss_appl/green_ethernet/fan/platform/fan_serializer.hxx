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
#ifndef __FAN_SERIALIZER_HXX__
#define __FAN_SERIALIZER_HXX__

#include "vtss/basics/snmp.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/fan.h"
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_common_iterator.hxx"
#include "fan.h"

VTSS_SNMP_TAG_SERIALIZE(fan_usid_index, u32, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner),
            vtss::tag::Name("SwitchId"),
            vtss::expose::snmp::RangeSpec<u32>(1, 16),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(0),
            vtss::tag::Description("The identification of switch. "
                "For non-stackable switch, the valid value is limited to 1."));
}

VTSS_SNMP_TAG_SERIALIZE(fan_temp_sensors_index, u8, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner),
            vtss::tag::Name("Index"),
            vtss::expose::snmp::RangeSpec<u32>(0, 4),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(0),
            vtss::tag::Description("The temperature sensor index. "
                "Some switches may also have more than one temperature sensor."));
}

VTSS_XXXX_SERIALIZE_ENUM(mesa_fan_pwd_freq_t, "FanPwmFrequency",
                         vtss_appl_fan_pwm_freq_type_txt,
                         "This enumeration defines the PWM frequency used for controlling the fan." );

/****************************************************************************
* Capabilities
****************************************************************************/
struct FanCapSensorCount {
    static constexpr const char *json_ref = "vtss_appl_fan_capabilities_t";
    static constexpr const char *name = "SensorCount";
    static constexpr const char *desc =
        "Maximum supported temperature sensors in a switch. ";
    static constexpr uint8_t get() {
        return VTSS_APPL_FAN_TEMPERATURE_SENSOR_CNT_MAX;
    }
};


template<typename T>
void serialize(T &a, vtss_appl_fan_chip_temp_t &s) {
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_fan_chip_temp_t"));
    m.add_leaf(s.chip_temp,
            vtss::tag::Name("ChipTemp"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(1),
            vtss::tag::Description("Current chip temperature (in C)."));
}

template<typename T>
void serialize(T &a, vtss_appl_fan_capabilities_t &s) {
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_fan_capabilities_t"));
    m.add_leaf(s.sensor_count,
            vtss::tag::Name("SensorCount"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(1),
            vtss::tag::Description(
                "Maximum supported temperature sensors in a switch. "));
}

template<typename T>
void serialize(T &a, vtss_appl_fan_speed_t &s) {
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_fan_speed_t"));
    m.add_leaf(s.fan_speed,
            vtss::tag::Name("Running"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(1),
            vtss::tag::Description(
                "The fan speed, currently running (in RPM)."));
}

template<typename T>
void serialize(T &a, vtss_appl_fan_conf_t &s) {
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_fan_conf_t"));
    m.add_leaf(s.glbl_conf.t_max,
               vtss::tag::Name("MaxSpeedTemperature"),
               vtss::expose::snmp::RangeSpec<i32>(-127, 127),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description(
                   "The temperature(in C) where fan shall be running at full "
                   "speed (maximum cooling). Valid range:-127 to 127"));
    m.add_leaf(s.glbl_conf.t_on,
               vtss::tag::Name("OnTemperature"),
               vtss::expose::snmp::RangeSpec<i32>(-127, 127),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description(
                   "The temperature(in C) where cooling is needed "
                   "(fan is started). Valid range:-127 to 127"));
    m.add_leaf(s.glbl_conf.pwm,
               vtss::tag::Name("pwmFrequency"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description(
                   "The PWM frequency used for controlling the speed of the FAN"));
}

namespace vtss {
namespace appl {
namespace fan {
namespace interfaces {

struct FanCapabilities {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_fan_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_fan_capabilities_t &s) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        typename HANDLER::Map_t m =
            h.as_map(vtss::tag::Typename("vtss_appl_fan_capabilities_t"));
        m.template capability<FanCapSensorCount>(vtss::expose::snmp::OidElementValue(1));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_fan_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};


struct FanConfigEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_fan_conf_t * >> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_fan_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, i);
    }
    VTSS_EXPOSE_GET_PTR(vtss_appl_fan_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_fan_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};


struct FanStatusSpeedEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamVal<vtss_appl_fan_speed_t * >> P;

    static constexpr const char *table_description =
        "This is a table for switch fan speed";

    static constexpr const char *index_description =
        "Each switch have fan module";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, fan_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_fan_speed_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_fan_speed_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_switch);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};


struct FanStatusSensorEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamKey<u8>,
        vtss::expose::ParamVal<vtss_appl_fan_chip_temp_t  * >> P;

    static constexpr const char *table_description =
        "This is a table to chip temperature";

    static constexpr const char *index_description =
        "Each switch have temperature sensor status";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, fan_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u8 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, fan_temp_sensors_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_fan_chip_temp_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_fan_sensor_temp_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_fan_sensors_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

}  // namespace interfaces
}  // namespace fan
}  // namespace appl
}  // namespace vtss


#endif
