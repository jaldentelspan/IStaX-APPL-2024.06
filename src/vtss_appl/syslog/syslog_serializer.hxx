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


/**
 * \file syslog_serializer.h
 * \brief This file defines the serializers for the SYSLOG module
 */


/*----------------------------------------------------------------------------
 This file is used to:
 - MIB enum serializer
 - MIB row/table indexes serializer
 - MIB row/table entry serializer
 - MIB module-identity and module-conformance serializer
 - MIB header serializer

 Notice that MIB OID name should start with the lowercase letter.
----------------------------------------------------------------------------*/


#ifndef __SYSLOG_SERIALIZER_HXX__
#define __SYSLOG_SERIALIZER_HXX__


#include <vtss/appl/syslog.h>
#include "vtss_appl_serialize.hxx"

/*****************************************************************************
 - Iteration functions
*****************************************************************************/
mesa_rc syslog_id_itr(const u32 *const prev_syslog_idx, u32 *const next_syslog_idx, vtss_usid_t usid);

/*!
 * \brief  Syslog history iterate function, it is used to get first and get next indexes.
 * \param  prev_swid_idx   [IN]:  previous switch ID.
 * \param  next_swid_idx   [OUT]: next switch ID.
 * \param  prev_syslog_idx [IN]:  previous syslog ID.
 * \param  next_syslog_idx [OUT]: next syslog ID.
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_history_itr(
    const vtss_usid_t   *const prev_swid_idx,
    vtss_usid_t         *const next_swid_idx,
    const u32           *const prev_syslog_idx,
    u32                 *const next_syslog_idx);

mesa_rc SL_lvl_itr(
    const vtss_appl_syslog_lvl_t    *const prev_lvl_idx,
    vtss_appl_syslog_lvl_t          *const next_lvl_idx);

/*!
 * \brief  Syslog history control iterate function, it is used to get first and get next indexes.
 * \param  prev_swid_idx  [IN]: previous switch ID.
 * \param  next_swid_idx [OUT]: next switch ID.
 * \param  prev_lvl_idx   [IN]: previous syslog level.
 * \param  next_lvl_idx  [OUT]: next syslog level.
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_history_control_itr(
    const vtss_usid_t               *const prev_swid_idx,
    vtss_usid_t                     *const next_swid_idx,
    const vtss_appl_syslog_lvl_t    *const prev_lvl_idx,
    vtss_appl_syslog_lvl_t          *const next_lvl_idx);

// Parent: SyslogControl
/* A dummy function for getting Syslog control entry */
extern mesa_rc vtss_appl_syslog_history_control_get(vtss_usid_t usid, vtss_appl_syslog_lvl_t lvl, vtss_appl_syslog_history_control_t *const control);
// get all function
mesa_rc vtss_appl_syslog_history_get_all(const vtss::expose::json::Request *req,
                                       vtss::ostreamBuf *os);

/*****************************************************************************
 - MIB enum serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t syslog_lvl_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_syslog_lvl_t,
                         "SyslogLevelType",
                         syslog_lvl_txt,
                         "The syslog severity level.");


/*****************************************************************************
 - MIB row/table indexes serializer
*****************************************************************************/
// Table index of SyslogStatusHistoryTable
VTSS_SNMP_TAG_SERIALIZE(syslog_history_tbl_idx_usid, u32, a, s)
{
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("SwitchId"),
        vtss::expose::snmp::RangeSpec<u32>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The identification of switch. "
            "For non-stackable switch, the valid value is limited to 1. "));
}

VTSS_SNMP_TAG_SERIALIZE(syslog_history_tbl_idx_msg_id, u32, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("MsgId"),
               vtss::expose::snmp::RangeSpec<u32>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("The identification of Syslog message."));
}

// Table index of SyslogControlHistoryTable
VTSS_SNMP_TAG_SERIALIZE(syslog_control_tbl_idx_usid, u32, a, s)
{
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("SwitchId"),
        vtss::expose::snmp::RangeSpec<u32>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The identification of switch. "
        "For non-stackable switch, the valid value is limited to 1. "
        "For stackable switch, value 0 means the action is applied to all switches."));
}

VTSS_SNMP_TAG_SERIALIZE(syslog_control_tbl_idx_lvl, vtss_appl_syslog_lvl_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("ClearLevel"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Indicates which level of message want to clear."));
}


/*****************************************************************************
 - MIB row/table entry serializer
*****************************************************************************/
// Scalars of SyslogConfigServer
template<typename T>
void serialize(T &a, vtss_appl_syslog_server_conf_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_syslog_server_conf_t"));
    int ix = 1;

    m.add_leaf(vtss::AsBool(s.server_mode),
               vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates the syslog server mode operation. When the mode operation is enabled, the syslog message will send out to syslog server."));

    m.add_leaf(s.syslog_server,
               vtss::tag::Name("Address"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The domain name of syslog server."));

    m.add_leaf(s.syslog_level,
               vtss::tag::Name("Level"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                "Indicates what level of message will send to syslog server. "                                                          \
                "For example, the error level will send the specific messages which severity code is less or equal than error(3), "     \
                "the warning level will send the specific messages which severity code is less or equal  than warning(4), "             \
                "the notice level will send the specific messages which severity code is less or equal  than notice(5), "               \
                "the informational level will send the specific messages which severity code is less or equal than informational(6) "   \
                "and the enumeration option of all(8) isn't used in this case."
               ));
}

// Table row entry of SyslogStatusHistoryTable
template<typename T>
void serialize(T &a, vtss_appl_syslog_history_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_syslog_history_t"));
    int ix = 0;

    m.add_leaf(s.lvl,
               vtss::tag::Name("MsgLevel"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The severity level of the system log message. Note that enumeration option of all(8) isn't used in this case."));

    m.add_leaf(vtss::AsUnixTimeStampSeconds(s.time),
               vtss::tag::Name("MsgTimeStamp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of sysUpTime when this message was generated."));

    m.add_leaf(vtss::AsDisplayString(s.msg, sizeof(s.msg)),
               vtss::tag::Name("MsgText"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The detailed context of the system log message."));
}

// Table row entry of SyslogControlHistoryTable
template<typename T>
void serialize(T &a, vtss_appl_syslog_history_control_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_syslog_history_control_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.clear_syslog),
               vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Clear syslog history by setting to true."));
}

template <typename T>
void serialize(T &a, vtss_appl_syslog_notif_name_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_syslog_notif_name_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(s.notif_name, sizeof(s.notif_name)),
               vtss::tag::Name("notificationName"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The name of the syslog notification"));
}

template <typename T>
void serialize(T &a, vtss_appl_syslog_notif_conf_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_syslog_notif_conf_t"));
    int ix = 0;

    m.add_leaf(vtss::AsDisplayString(s.notification, sizeof(s.notification)),
               vtss::tag::Name("notification"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The reference to the notification"));

    m.add_leaf(s.level,
               vtss::tag::Name("syslogLevel"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The syslog level"));
}

namespace vtss {
namespace appl {
namespace syslog {
namespace interfaces {

struct SyslogConfigGlobals {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_syslog_server_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_syslog_server_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_syslog_server_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_syslog_server_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SyslogConfigNotifications {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_syslog_notif_name_t *>,
        vtss::expose::ParamVal<vtss_appl_syslog_notif_conf_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "The syslog notification configuration table.";

    static constexpr const char *index_description =
        "Each row contains configuration of a syslog notification.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_syslog_notif_name_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_syslog_notif_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_syslog_notif_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_syslog_notif_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_syslog_notif_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_syslog_notif_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_syslog_notif_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SyslogStatusHistoryTbl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_syslog_history_t *>
    > P;

    static constexpr const char *table_description =
        "The syslog history table.";

    static constexpr const char *index_description =
        "Each row contains a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, syslog_history_tbl_idx_usid(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, syslog_history_tbl_idx_msg_id(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_syslog_history_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_syslog_history_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_syslog_history_itr);
    VTSS_JSON_GET_ALL_PTR(vtss_appl_syslog_history_get_all);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM);
};

struct SyslogControlHistoryTbl {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamKey<vtss_appl_syslog_lvl_t>,
        vtss::expose::ParamVal<vtss_appl_syslog_history_control_t *>
    > P;

    static constexpr const char *table_description =
        "The syslog history clear table.";

    static constexpr const char *index_description =
        "Each row contains a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, syslog_control_tbl_idx_usid(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_syslog_lvl_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, syslog_control_tbl_idx_lvl(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_syslog_history_control_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_syslog_history_control_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_syslog_history_control_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_syslog_history_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM);
};

}  // namespace interfaces
}  // namespace syslog
}  // namespace appl
}  // namespace vtss
#endif /* __SYSLOG_SERIALIZER_HXX__ */
