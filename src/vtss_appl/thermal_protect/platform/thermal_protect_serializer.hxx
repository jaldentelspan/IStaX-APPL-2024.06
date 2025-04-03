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
#ifndef __THERMAL_PROTECT_SERIALIZER_HXX__
#define __THERMAL_PROTECT_SERIALIZER_HXX__
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss/appl/thermal_protect.h"

VTSS_SNMP_TAG_SERIALIZE(TPROTECT_ifindex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

VTSS_SNMP_TAG_SERIALIZE(thermal_protect_group_number, vtss_appl_thermal_protect_group_t, a, s) {
    a.add_leaf(vtss::AsInt(s.inner.group),
               vtss::tag::Name("GroupIndex"),
               vtss::expose::snmp::RangeSpec<u32>(0, 3),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Group number."));
}

template<typename T>
void serialize(T &a, vtss_appl_thermal_protect_capabilities_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_thermal_protect_capabilities_t"));
    int ix = 0;
    m.add_leaf(s.max_supported_group,
               vtss::tag::Name("MaxSupportedGroup"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of supported thermal protection groups."));
}

template<typename T>
void serialize(T &a, vtss_appl_thermal_protect_group_temperature_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_thermal_protect_group_temperature_t"));
    int ix = 0;
    m.add_leaf(s.group_temperature,
               vtss::tag::Name("GroupTemperature"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Temperature(in C) "
                   "where the interfaces mapped to the group will be shut down."));
}

template<typename T>
void serialize(T &a, vtss_appl_thermal_protect_group_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_thermal_protect_group_t"));
    m.add_leaf(s.group,
               vtss::tag::Name("Group"),
               vtss::expose::snmp::RangeSpec<u32>(0, 4),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Thermal protection groups. "
                   "Object value (4) mean disable thermal protect for the interface. "
                   "Object values from 0 to 3 are for the temperature group. "));
}

template<typename T>
void serialize(T &a, vtss_appl_thermal_protect_port_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_thermal_protect_port_status_t"));
    int ix = 0;
    m.add_leaf(s.temperature, 
               vtss::tag::Name("Temperature"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current port temperature(in C).")
               );
    m.add_leaf(vtss::AsBool(s.power_status), 
               vtss::tag::Name("Power"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Port thermal protection status. "
                   "false means port link is up and port is operating normally. "
                   "true means port link is down and port is thermal protected."));
}

namespace vtss {
namespace appl {
namespace thermal_protect {
namespace interfaces {
struct ThermalProtectPriorityTempEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_thermal_protect_group_t>,
        vtss::expose::ParamVal<vtss_appl_thermal_protect_group_temperature_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to assign a temperature to each of the groups";

    static constexpr const char *index_description =
        "Each group associates with a temperature";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_thermal_protect_group_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, thermal_protect_group_number(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_thermal_protect_group_temperature_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_thermal_protect_group_temp_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_thermal_protect_group_iterator);
    VTSS_EXPOSE_SET_PTR(vtss_appl_thermal_protect_group_temp_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct ThermalProtectInterfacePriorityEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_thermal_protect_group_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to interface group configuration";

    static constexpr const char *index_description =
        "Each physical port associates with a group temperature";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, TPROTECT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_thermal_protect_group_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_thermal_protect_port_group_get);
    VTSS_EXPOSE_ITR_PTR(vtss_ifindex_getnext_port);
    VTSS_EXPOSE_SET_PTR(vtss_appl_thermal_protect_port_group_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct ThermalProtectionInterfaceStatusEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_thermal_protect_port_status_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to thermal protection interface status";

    static constexpr const char *index_description =
        "Each interface has a set of status parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, TPROTECT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_thermal_protect_port_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_thermal_protect_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_ifindex_getnext_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct ThermalProtectionCapabilitiesEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_thermal_protect_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_thermal_protect_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_thermal_protect_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};
}  // namespace interfaces
}  // namespace thermal_protect
}  // namespace appl
}  // namespace vtss
#endif
