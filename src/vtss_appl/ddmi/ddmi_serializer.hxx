/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __DDMI_SERIALIZER_HXX__
#define __DDMI_SERIALIZER_HXX__

#include <vtss/basics/snmp.hxx>
#include "vtss_appl_serialize.hxx"
#include "ddmi_expose.hxx"
#include <vtss/appl/ddmi.h>
#include <vtss/appl/port.h>
#include "ddmi_api.h" /* For ddmi_monitor_type_to_txt() */

// Allow type++ on the following type
VTSS_ENUM_INC(vtss_appl_ddmi_monitor_type_t);

/*****************************************************************************
    Enum serializer
*****************************************************************************/
VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_ddmi_monitor_type_t,
    "ddmiMonitorType",
    ddmi_monitor_type_txt,
    "This enumeration defines the monitor type for SFPs.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_ddmi_monitor_state_t,
    "ddmiMonitorState",
    ddmi_monitor_state_txt,
    "This enumeration defines the monitor state for a particular monitor type for SFPs.");

VTSS_SNMP_TAG_SERIALIZE(DDMI_ifindex, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("ifIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Interface index number."));
}

template<typename T>
void serialize(T &a, vtss_appl_ddmi_global_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ddmi_global_conf_t"));

    m.add_leaf(
        vtss::AsBool(s.admin_enable),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Set to true to enable DDMI on all SFP ports."));
}

template<typename T>
void serialize(T &a, vtss_appl_ddmi_notification_status_key_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ddmi_notification_status_key_t"));
    int ix = 0;

    m.add_leaf(s.ifindex,
               vtss::tag::Name("interfaceIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface index (port number)."));

    m.add_leaf(s.type,
               vtss::tag::Name("type"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Monitor type."));
}

template<typename T>
void serialize(T &a, vtss_appl_ddmi_port_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ddmi_port_status_t"));
    vtss_appl_ddmi_monitor_status_t *monitor_status;
    vtss_appl_ddmi_monitor_type_t   type;
    char                            type_as_txt[50], buf1[100], buf2[100];
    const char                      *units;
    int                             ix = 0;

    m.add_leaf(
        vtss::AsBool(s.a0_supported),
        vtss::tag::Name("A0Supported"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Support transceiver status information or not. "
                               "true is to supported and false is not supported.")
    );

    m.add_leaf(
        vtss::AsBool(s.sfp_detected),
        vtss::tag::Name("A0SfpDetected"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("SFP module is detected or not. "
                               "true is to detected and false is not detected.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.vendor, sizeof(s.vendor)),
        vtss::tag::Name("A0Vendor"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Vendor name.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.part_number, sizeof(s.part_number)),
        vtss::tag::Name("A0PartNumber"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Part number.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.serial_number, sizeof(s.serial_number)),
        vtss::tag::Name("A0SerialNumber"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Serial number.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.revision, sizeof(s.revision)),
        vtss::tag::Name("A0Revision"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Revision.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.date_code, sizeof(s.date_code)),
        vtss::tag::Name("A0DateCode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Date Code.")
    );

    m.add_leaf(
        s.sfp_type,
        vtss::tag::Name("A0SfpType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("SFP type.")
    );

    /* Separating A0 and A2 data to extend in future */
    ix = 1000;

    m.add_leaf(
        vtss::AsBool(s.a2_supported),
        vtss::tag::Name("A2Supported"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("True if DDMI status information is supported by the SFP."));

    for (type = (vtss_appl_ddmi_monitor_type_t)0; type < VTSS_APPL_DDMI_MONITOR_TYPE_CNT; type++) {
        monitor_status = &s.monitor_status[type];
        sprintf(type_as_txt, "%s", ddmi_monitor_type_to_txt(type));

        switch (type) {
        case VTSS_APPL_DDMI_MONITOR_TYPE_TEMPERATURE:
            units = "degrees Celsius";
            break;

        case VTSS_APPL_DDMI_MONITOR_TYPE_VOLTAGE:
            units = "Volts";
            break;

        case VTSS_APPL_DDMI_MONITOR_TYPE_TX_BIAS:
            units = "mA";
            break;

        case VTSS_APPL_DDMI_MONITOR_TYPE_TX_POWER:
        case VTSS_APPL_DDMI_MONITOR_TYPE_RX_POWER:
            units = "mW";
            break;

        default:
            units = "";
            break;
        }

        sprintf(buf1, "A2%sState", type_as_txt);
        sprintf(buf2, "Current %s monitor state", type_as_txt);
        m.add_leaf(
            monitor_status->state,
            vtss::tag::Name(buf1),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(buf2));

        sprintf(buf1, "A2%sCurrent", type_as_txt);
        sprintf(buf2, "Current %s in %s.", type_as_txt, units);
        m.add_leaf(
            vtss::AsDisplayString(monitor_status->current, sizeof(monitor_status->current)),
            vtss::tag::Name(buf1),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(buf2));

        sprintf(buf1, "A2%sHighAlarmThreshold", type_as_txt);
        sprintf(buf2, "%s high alarm threshold in %s.", type_as_txt, units);
        m.add_leaf(
            vtss::AsDisplayString(monitor_status->alarm_hi, sizeof(monitor_status->alarm_hi)),
            vtss::tag::Name(buf1),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(buf2));

        sprintf(buf1, "A2%sLowAlarmThreshold", type_as_txt);
        sprintf(buf2, "%s low alarm threshold in %s.", type_as_txt, units);
        m.add_leaf(
            vtss::AsDisplayString(monitor_status->alarm_lo, sizeof(monitor_status->alarm_lo)),
            vtss::tag::Name(buf1),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(buf2));

        sprintf(buf1, "A2%sHighWarnThreshold", type_as_txt);
        sprintf(buf2, "%s high warning threshold in %s.", type_as_txt, units);
        m.add_leaf(
            vtss::AsDisplayString(monitor_status->warn_hi, sizeof(monitor_status->warn_hi)),
            vtss::tag::Name(buf1),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(buf2));

        sprintf(buf1, "A2%sLowWarnThreshold", type_as_txt);
        sprintf(buf2, "%s low warning threshold in %s.", type_as_txt, units);
        m.add_leaf(
            vtss::AsDisplayString(monitor_status->warn_lo, sizeof(monitor_status->warn_lo)),
            vtss::tag::Name(buf1),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(buf2));
    }
}

template<typename T>
void serialize(T &a, vtss_appl_ddmi_monitor_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_ddmi_monitor_status_t"));
    int ix = 0;

    m.add_leaf(
        s.state,
        vtss::tag::Name("State"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Current state")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.current, sizeof(s.current)),
        vtss::tag::Name("Current"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The current value when the event occurs.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.alarm_hi, sizeof(s.alarm_hi)),
        vtss::tag::Name("HighAlarmThreshold"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The High alarm threshold when the event occurs.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.alarm_lo, sizeof(s.alarm_lo)),
        vtss::tag::Name("LowAlarmThreshold"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The Low alarm threshold when the event occurs.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.warn_hi, sizeof(s.warn_hi)),
        vtss::tag::Name("HighWarnThreshold"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The high warning threshold when the event occurs.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.warn_lo, sizeof(s.warn_lo)),
        vtss::tag::Name("LowWarnThreshold"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The low warning threshold when the event occurs.")
    );
}

namespace vtss
{
namespace appl
{
namespace ddmi
{
namespace interfaces
{
struct ddmiConfigGlobalsLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_ddmi_global_conf_t *>
    > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ddmi_global_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ddmi_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ddmi_global_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DDMI);
};

struct ddmiStatusInterfaceEntry {
    typedef expose::ParamList<vtss::expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_ddmi_port_status_t *>> P;

    static constexpr const char *table_description =
        "This is a DDMI status table of port interface.";

    static constexpr const char *index_description =
        "Each entry has a set of DDMI status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, DDMI_ifindex(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ddmi_port_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ddmi_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DDMI);
};

struct DdmiNotificationStatusTable {
    typedef expose::ParamList<expose::ParamKey<vtss_appl_ddmi_notification_status_key_t *>,
            expose::ParamVal<vtss_appl_ddmi_monitor_status_t * >> P;

    static constexpr const char *table_description =
        "DDMI status table per ifindex (port) and monitor type.";

    static constexpr const char *index_description =
        "Ifindex (port) and monitor type";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ddmi_notification_status_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ddmi_monitor_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ddmi_notification_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ddmi_notification_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DDMI);
};

}  // namespace interfaces
}  // namespace ddmi
}  // namespace appl
}  // namespace vtss

#endif /* __DDMI_SERIALIZER_HXX__ */

