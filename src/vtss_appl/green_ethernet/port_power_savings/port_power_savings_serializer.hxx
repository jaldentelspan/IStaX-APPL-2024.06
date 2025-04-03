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
#ifndef __PORT_POWER_SAVINGS_SERIALIZER_HXX__
#define __PORT_POWER_SAVINGS_SERIALIZER_HXX__

#include "vtss/basics/snmp.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/port_power_savings.h"
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"

extern const vtss_enum_descriptor_t vtss_appl_power_saving_status_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_power_saving_status_t, "PortPowerSavingsStatusType",
                         vtss_appl_power_saving_status_type_txt,
                         "This enumeration defines the feature status.");

/*****************************************************************************
  - MIB row/table indexes serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(PPS_ifindex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

template <typename T>
void serialize(T &a, vtss_appl_port_power_saving_capabilities_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_port_power_saving_capabilities_t"));
    int ix = 0;
    m.add_leaf(vtss::AsBool(s.energy_detect_capable), vtss::tag::Name("LinkPartner"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Indicate whether interface is capable for detecting link partner or not. "
                       "true means interface is capable to detect link partner, "
                       "false means interface is not capable to detect link partner."));

    m.add_leaf(vtss::AsBool(s.short_reach_capable), vtss::tag::Name("ShortReach"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Indicates whether interface is able to determine the cable length " 
                       "connected to partner port. "
                       "true means interface is capable to determine the cable length, "
                       "false means interface is not capable to determine the cable length."));
}

template <typename T>
void serialize(T &a, vtss_appl_port_power_saving_conf_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_port_power_saving_conf_t"));
    int ix = 0;
    m.add_leaf(vtss::AsBool(s.energy_detect), vtss::tag::Name("LinkPartner"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Save port power if there is no link partner connected to the port. "
                       "true is to enable port power saving when there is no link partner connected, "
                       "false is to disable it."));

    m.add_leaf(vtss::AsBool(s.short_reach), vtss::tag::Name("ShortReach"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Save port power if port is connected to link partner through short cable. "
                       "true is to enable port power saving when link partner connected through short cable, "
                       "false is to disable it."));
}

template <typename T>
void serialize(T &a, vtss_appl_port_power_saving_status_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_port_power_saving_status_t"));
    int ix = 0;
    m.add_leaf(s.energy_detect_power_savings, vtss::tag::Name("NoLinkPartner"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicate whether port is saving power due to "
                                      "no link partner connected."));

    m.add_leaf(s.short_reach_power_savings, vtss::tag::Name("ShortCable"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                       "Indicate whether port is saving power due to "
                       "link partner connected through short cable."));
}
namespace vtss {
namespace appl {
namespace port_power_savings {
namespace interfaces {

struct ppsInterfaceConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_port_power_saving_conf_t *>> P;

    static constexpr const char *table_description =
            "This table provides Port Power Savings configuration for an interface";

    static constexpr const char *index_description =
            "Each interface has a set of Port Power Savings configurable parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, PPS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_port_power_saving_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_port_power_saving_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(vtss_appl_port_power_saving_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct ppsInterfaceStatusEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_port_power_saving_status_t *>> P;

    static constexpr const char *table_description =
            "This is a table to Port Power Savings interface status";

    static constexpr const char *index_description =
            "Each interface has a set of status parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, PPS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_port_power_saving_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_port_power_saving_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};

struct ppsInterfaceCapabilitiesEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_port_power_saving_capabilities_t *>> P;

    static constexpr const char *table_description =
            "This is a table to interface capabilities";

    static constexpr const char *index_description =
            "Each interface has a set of capability parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, PPS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_port_power_saving_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_port_power_saving_capabilities_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET);
};
}  // namespace interfaces
}  // namespace port_power_savings
}  // namespace appl
}  // namespace vtss
#endif
