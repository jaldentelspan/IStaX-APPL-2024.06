/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __DAYLIGHT_SAVING_SERIALIZER_HXX__
#define __DAYLIGHT_SAVING_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/daylight_saving.h"

extern vtss_enum_descriptor_t vtss_appl_clock_summertime_mode_txt[];

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_clock_summertime_mode_t,
                        "DaylightSavingMode",
                         vtss_appl_clock_summertime_mode_txt,
                         "This enumeration defines the available summer time(Daylight Saving) mode.");

template<typename T>
void serialize(T &a, vtss_appl_clock_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_clock_conf_t"));
    int ix = 1;
    m.add_leaf(vtss::AsDisplayString(s.timezone_conf.timezone_acronym, sizeof(s.timezone_conf.timezone_acronym)),
               vtss::tag::Name("TimeZoneAcronym"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a acronym to identify the time zone."));

    m.add_leaf(s.timezone_conf.timezone_offset,
               vtss::tag::Name("TimeZoneOffset"),
               vtss::expose::snmp::RangeSpec<i32>(-1439, 1439),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("To set the system time zone with respect to UTC in minutes."));

    m.add_leaf(s.summertime_conf.stime_mode,
               vtss::tag::Name("SummerTimeMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time mode.\n "
                                      "Disabled: The daylight feature is disabled, "
                                      "and no input validation is performed on remaining configurations parameters.\n "
                                      "Recurring mode: Summer time configuration will repeat every year. "
                                      "To enable this mode requires that the parameters Month, Week and Day are configured with valid "
                                      "values (non zero). The parameters Year and Date must be set to 0, "
                                      "signaling that they are not used.\n "
                                      "Non recurring mode: Summer time configuration is done once. "
                                      "To enable this feature requires that the following values are configured with valid values.\n"
                                      "(non zero): Year, Month and Date. The parameters Week and Day must be set to 0 "
                                      "signaling that they are not used."));

    m.add_leaf(s.summertime_conf.stime_start.week,
               vtss::tag::Name("SummerTimeWeekStart"),
               vtss::expose::snmp::RangeSpec<u32>(0, 5),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time starting week. "
                                      "This object needs to be set when summer time mode is recurring. "
                                      "Object value 0 means unused object."));

    m.add_leaf(s.summertime_conf.stime_start.day,
               vtss::tag::Name("SummerTimeDayStart"),
               vtss::expose::snmp::RangeSpec<u32>(0, 7),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time starting day. "
                                      "where monday = 1, sunday = 7.\n "
                                      "This object needs to be set when summer time mode is recurring. "
                                      "Object value 0 means unused object."));

    m.add_leaf(s.summertime_conf.stime_start.month,
               vtss::tag::Name("SummerTimeMonthStart"),
               vtss::expose::snmp::RangeSpec<u32>(1, 12),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time starting month. "
                                      "This object needs to be set when summer time mode is not disabled."));

    m.add_leaf(s.summertime_conf.stime_start.date,
               vtss::tag::Name("SummerTimeDateStart"),
               vtss::expose::snmp::RangeSpec<u32>(0, 31),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time starting date. "
                                      "This object needs to be set when summer time mode is non recurring."));

    m.add_leaf(s.summertime_conf.stime_start.year,
               vtss::tag::Name("SummerTimeYearStart"),
               vtss::expose::snmp::RangeSpec<u32>(0, 2097),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time starting year. "
                                      "This object needs to be set when summer time mode is non recurring. "
                                      "Object value 0 means unused object."));

    m.add_leaf(s.summertime_conf.stime_start.hour,
               vtss::tag::Name("SummerTimeHourStart"),
               vtss::expose::snmp::RangeSpec<u32>(0, 23),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time starting hour. "
                                      "This object needs to be set when summer time mode is not disabled:"));

    m.add_leaf(s.summertime_conf.stime_start.minute,
               vtss::tag::Name("SummerTimeMinuteStart"),
               vtss::expose::snmp::RangeSpec<u32>(0, 59),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time starting minute. "
                                      "This object needs to be set when summer time mode is not disabled."));

    m.add_leaf(s.summertime_conf.stime_end.week,
               vtss::tag::Name("SummerTimeWeekEnd"),
               vtss::expose::snmp::RangeSpec<u32>(0, 5),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time ending week. "
                                      "This object needs to be set when summer time mode is recurring. "
                                      "Object value 0 means unused object."));

    m.add_leaf(s.summertime_conf.stime_end.day,
               vtss::tag::Name("SummerTimeDayEnd"),
               vtss::expose::snmp::RangeSpec<u32>(0, 7),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time ending day. "
                                      "This object needs to be set when summer time mode is recurring. "
                                      "Object value 0 means unused object."));

    m.add_leaf(s.summertime_conf.stime_end.month,
               vtss::tag::Name("SummerTimeMonthEnd"),
               vtss::expose::snmp::RangeSpec<u32>(1, 12),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time ending month. "
                                      "This object needs to be set when summer time mode is not disabled."));

    m.add_leaf(s.summertime_conf.stime_end.date,
               vtss::tag::Name("SummerTimeDateEnd"),
               vtss::expose::snmp::RangeSpec<u32>(0, 31),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time ending date. "
                                      "This object needs to be set when summer time mode is non recurring. "
                                      "Object value 0 means unused object."));

    m.add_leaf(s.summertime_conf.stime_end.year,
               vtss::tag::Name("SummerTimeYearEnd"),
               vtss::expose::snmp::RangeSpec<u32>(0, 2097),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time ending year. "
                                      "This object needs to be set when summer time mode is non recurring. "
                                      "Object value 0 means unused object."));

    m.add_leaf(s.summertime_conf.stime_end.hour,
               vtss::tag::Name("SummerTimeHourEnd"),
               vtss::expose::snmp::RangeSpec<u32>(0, 23),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time ending hour. "
                                      "This object needs to be set when summer time mode is not disabled."));

    m.add_leaf(s.summertime_conf.stime_end.minute,
               vtss::tag::Name("SummerTimeMinuteEnd"),
               vtss::expose::snmp::RangeSpec<u32>(0, 59),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is a summer time ending minute. "
                                      "This object needs to be set when summer time mode is not disabled."));

    m.add_leaf(s.summertime_conf.stime_offset,
               vtss::tag::Name("SummerTimeOffset"),
               vtss::expose::snmp::RangeSpec<u32>(1, 1439),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of this object indicates "
                                      "the number of minutes to add  or to subtract during summertime. "
                                      "This object needs to be set when summer time mode is not disabled."));
}

namespace vtss {
namespace appl {
namespace daylight_saving {
namespace interfaces {

struct DaylightSavingGlobalsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_clock_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_clock_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_clock_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_clock_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

}  // namespace interfaces
}  // namespace daylight_saving
}  // namespace appl
}  // namespace vtss
#endif
