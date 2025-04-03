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
#ifndef __VTSS_ALARM_SERIALIZER_HXX__
#define __VTSS_ALARM_SERIALIZER_HXX__

#include "vtss/basics/formatting_tags.hxx"
//#include "vtss_appl_formatting_tags.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/basics/expose/json.hxx"
#include "vtss/basics/expose/types.hxx"
#include "vtss/basics/expose/snmp/types.hxx"

#include "vtss/appl/alarm.h"

template <typename T>
void serialize(T &a, vtss_appl_alarm_name_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_alarm_name_t"));
    int ix = 1;

    m.add_leaf(vtss::AsDisplayString(s.alarm_name, sizeof(s.alarm_name)),
               vtss::tag::Name("alarmName"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The name of the alarm"));
}

template <typename T>
void serialize(T &a, vtss_appl_alarm_expression_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_alarm_expression_t"));

    int ix = 1;

    m.add_leaf(
            vtss::AsDisplayString(s.alarm_expression, sizeof(s.alarm_expression)),
            vtss::tag::Name("expression"), vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The expression defining the alarm."));
}

template <typename T>
void serialize(T &a, vtss_appl_alarm_status_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_alarm_status_t"));

    int ix = 1;

    m.add_leaf(
            vtss::AsBool(s.suppressed), vtss::tag::Name("suppressed"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(
                    "Indicates whether the alarm subtree is suppressed. When "
                    "a subtree is suppressed, the status does not contribute "
                    "to the state of the superior alarm tree."));
    m.add_leaf(vtss::AsBool(s.active), vtss::tag::Name("Active"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether the alarm is active"));
    m.add_leaf(vtss::AsBool(s.exposed_active), vtss::tag::Name("ExposedActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The exposed alarm status."));
}

template <typename T>
void serialize(T &a, vtss_appl_alarm_suppression_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_alarm_suppression_t"));

    int ix = 1;

    m.add_leaf(
            vtss::AsBool(s.suppress), vtss::tag::Name("suppress"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(
                    "Indicates whether to suppress the alarm subtree. When "
                    "a subtree is suppressed, the status does not contribute "
                    "to the state of the superior alarm tree."));
}

namespace vtss {
namespace appl {
namespace alarm {

struct AlarmConfTable {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_appl_alarm_name_t *>,
            vtss::expose::ParamVal<vtss_appl_alarm_expression_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
            "The table is the list of configured alarms. The index is the name "
            "of the alarm";

    static constexpr const char *index_description =
            "An index is a dotted name e.g. alarm.port.status";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_alarm_name_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_alarm_expression_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_alarm_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_alarm_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_alarm_conf_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_alarm_conf_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_alarm_conf_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_ALARM);
};

struct AlarmStatusTable {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_appl_alarm_name_t *>,
            vtss::expose::ParamVal<vtss_appl_alarm_status_t *>> P;

    static constexpr const char *table_description =
            "The table is the list of alarm nodes. The index is the name of "
            "the alarm node";

    static constexpr const char *index_description =
            "An index is a dotted name e.g. alarm.port.status";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_alarm_name_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_alarm_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_alarm_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_alarm_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ALARM);
};

struct AlarmControlTable {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_appl_alarm_name_t *>,
            vtss::expose::ParamVal<vtss_appl_alarm_suppression_t *>> P;
    static constexpr const char *table_description =
            "The table is the list of alarm nodes. The index is the name of "
            "the alarm node";

    static constexpr const char *index_description =
            "An index is a dotted name e.g. alarm.port.status";
    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_alarm_name_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_alarm_suppression_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_alarm_suppress_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_alarm_status_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_alarm_suppress_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_ALARM);
};

}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_ALARM_SERIALIZER_HXX__ */
