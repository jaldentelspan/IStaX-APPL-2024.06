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
#ifndef __VTSS_UPNP_SERIALIZER_HXX__
#define __VTSS_UPNP_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/upnp.h"

/*****************************************************************************
     Enumerator serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t upnp_ip_addr_mode_Type_txt[];

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_upnp_ip_addressing_mode_t,
                         "UpnpIpAddressingModeType",
                         upnp_ip_addr_mode_Type_txt,
                         "This enumeration indicates the type of IP addressing mode.");

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_upnp_capabilities_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_upnp_capabilities_t"));
    int                 ix = 0;

    m.add_leaf(
        vtss::AsBool(s.support_ttl_write),
        vtss::tag::Name("SupportTtlWrite"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The capability to support the authority of writing function.")
    );

}

template<typename T>
void serialize(T &a, vtss_appl_upnp_param_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_upnp_param_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Global mode of UPnP. true is to "
            "enable the functions of HTTPS and false is to disable it.")
    );

    m.add_leaf(
        s.ttl,
        vtss::tag::Name("Ttl"),
        vtss::expose::snmp::RangeSpec<u32>(1, 255),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The TTL value is used by UPnP to send SSDP advertisement messages."
            "Valid values are in the range 1 to 255. ")
    );

    m.add_leaf(
        vtss::AsInt(s.adv_interval),
        vtss::tag::Name("AdvertisingDuration"),
        vtss::expose::snmp::RangeSpec<u32>(100, 86400),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The duration, carried in SSDP packets, is used to inform a control "
            "point or control points how often it or they should receive an SSDP advertisement message "
            "from this switch. If a control point does not receive any message within the duration, "
            "it will think that the switch no longer exists. Due to the unreliable nature of UDP, "
            "in the standard it is recommended that such refreshing of advertisements to be done "
            "at less than one-half of the advertising duration. In the implementation, "
            "the switch sends SSDP messages periodically at the interval one-half of the "
            "advertising duration minus 30 seconds. Valid values are in the range 100 to 86400. ")
    );

    m.add_leaf(
        s.ip_addressing_mode,
        vtss::tag::Name("IpAddressingMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("IP addressing mode provides two ways to determine IP address assignment: "
            "dynamic(0) and static(1). "
            "Dynamic: Default selection for UPnP. UPnP module helps users choosing the IP address "
            "of the switch device. It finds the first available system IP address. "
            "Static: User specifies the IP interface VLAN for choosing the IP address of the switch device.")
    );

    m.add_leaf(
        vtss::AsInterfaceIndex(s.static_ifindex),
        vtss::tag::Name("StaticIpInterfaceId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The index of the specific IP VLAN interface. It will only be applied when IP Addressing Mode is static. "
            "Valid configurable values ranges from 1 to 4095. Default value is 1.")
    );

}

namespace vtss {
namespace appl {
namespace upnp {
namespace interfaces {

struct UpnpCapabilitiesLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_upnp_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_upnp_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1), vtss::tag::Name("capability"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_upnp_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_UPNP);
};

struct UpnpParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_upnp_param_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_upnp_param_t &i) {
        h.argument_properties(tag::Name("global_config"));
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_upnp_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_upnp_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_UPNP);
};
}  // namespace interfaces
}  // namespace upnp
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_UPNP_SERIALIZER_HXX__ */
