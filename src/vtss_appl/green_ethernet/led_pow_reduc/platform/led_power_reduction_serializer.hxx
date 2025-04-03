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
#ifndef __LED_POWER_REDUCTION_SERIALIZER_HXX__
#define __LED_POWER_REDUCTION_SERIALIZER_HXX__
#include "vtss/basics/snmp.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss/appl/led_power_reduction.h"

VTSS_SNMP_TAG_SERIALIZE(led_power_reduction_clock_time_idx, 
                        vtss_appl_led_power_reduction_led_glow_start_time_t, a, s){
    a.add_leaf(vtss::AsInt(s.inner.hour), vtss::tag::Name("ClockTimeHour"),
               vtss::expose::snmp::RangeSpec<u32>(0, 23), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description(" Clock time hour. "
               "Hour is based on 24-hour time notation. "
               "If hour is x then LEDs will start glow at clock time x:00 of the day "
               "with associated LEDs intensity level."));
}

template<typename T>
void serialize(T &a, vtss_appl_led_power_reduction_intensity_t &s) {
    typename T::Map_t m =
             a.as_map(vtss::tag::Typename("vtss_appl_led_power_reduction_intensity_t"));
    int ix = 0;
    m.add_leaf(s.intensity, vtss::tag::Name("IntensityLevel"),
               vtss::expose::snmp::RangeSpec<u32>(0, 100), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(" Led power intensity level. "
               "The LEDs brightness in percentage. "
               "100 means full intensity(maximum power consumption), "
               "0 means LEDs are off(no power consumption)."));
}
template<typename T>
void serialize(T &a, vtss_appl_led_power_reduction_led_full_brightness_conf_t &s) {
    typename T::Map_t m =
             a.as_map(vtss::tag::Typename("vtss_appl_led_power_reduction_led_full_brightness_conf_t"));
    int ix = 0;
    m.add_leaf(s.maintenance_duration, vtss::tag::Name("MaintenanceDuration"),
               vtss::expose::snmp::RangeSpec<u32>(0, 65535), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(" The switch maintenance duration (in seconds). "
               "During switch maintenance LEDs will glow in full intensity after " 
               "either a port has changed link state or the LED push button has been pushed."));

    m.add_leaf(vtss::AsBool(s.error_enable), vtss::tag::Name("ErrorEnable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(" Turned on LEDs at full brightness(100% intensity) " 
               "when LED is blinking in red because of either software error or fatal occurred. "
               "true means LEDs will glow in full brightness, "
               "false means LEDs will not glow in full brightness."));
}
namespace vtss {
namespace appl {
namespace led_power_reduction {
namespace interfaces {
struct ledGlobalConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamVal<vtss_appl_led_power_reduction_led_full_brightness_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_led_power_reduction_led_full_brightness_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }
    VTSS_EXPOSE_GET_PTR(vtss_appl_led_power_reduction_full_led_brightness_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_led_power_reduction_full_led_brightness_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct ledGlobalTimeIntensityEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_appl_led_power_reduction_led_glow_start_time_t>,
            vtss::expose::ParamVal<vtss_appl_led_power_reduction_intensity_t *>> P;

    static constexpr const char *table_description =
            "This is a table to assign led intensity level "
            "to each clock hour(based on 24-hour time notaion) of the day";

    static constexpr const char *index_description =
            "Each clock hour of the day associates with led intensity level";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_led_power_reduction_led_glow_start_time_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, led_power_reduction_clock_time_idx(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_led_power_reduction_intensity_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_led_power_reduction_clock_time_intensity_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_led_power_reduction_clock_time_iterator);
    VTSS_EXPOSE_SET_PTR(vtss_appl_led_power_reduction_clock_time_intensity_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};
}  // namespace interfaces
}  // namespace led_power_reduction
}  // namespace appl
}  // namespace vtss
#endif //__LED_POWER_REDUCTION_SERIALIZER_HXX__
