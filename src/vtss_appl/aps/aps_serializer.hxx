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

#ifndef __VTSS_APS_SERIALIZER_HXX__
#define __VTSS_APS_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include <vtss/appl/aps.h>
#include "vtss_appl_formatting_tags.hxx" // for AsStdDisplayString

mesa_rc vtss_aps_create_conf_default(uint32_t *instance, vtss_appl_aps_conf_t *s);
mesa_rc vtss_aps_statistics_clear(uint32_t instance, const BOOL *clear);
mesa_rc vtss_aps_statistics_clear_dummy(uint32_t instance,  BOOL *clear);

extern vtss_enum_descriptor_t aps_sf_trigger_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aps_sf_trigger_t,
                         "ApsSfTrigger",
                         aps_sf_trigger_txt,
                         "The APS signal-fail triggering method.");

extern vtss_enum_descriptor_t aps_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aps_mode_t,
                         "ApsMode",
                         aps_mode_txt,
                         "The APS protection mode.");

extern vtss_enum_descriptor_t aps_command_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aps_command_t,
                         "ApsCommand",
                         aps_command_txt,
                         "protection group command.");

extern vtss_enum_descriptor_t aps_oper_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aps_oper_state_t,
                         "ApsOperationalState",
                         aps_oper_state_txt,
                         "APS instance operational state.");

extern vtss_enum_descriptor_t aps_oper_warning_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aps_oper_warning_t,
                         "ApsOperationalWarning",
                         aps_oper_warning_txt,
                         "APS instance operational warning.");

extern vtss_enum_descriptor_t aps_prot_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aps_prot_state_t,
                         "ApsProtectionState",
                         aps_prot_state_txt,
                         "protection group state.");

extern vtss_enum_descriptor_t aps_defect_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aps_defect_state_t,
                         "ApsDefectState",
                         aps_defect_state_txt,
                         "Interface defect state.");

extern vtss_enum_descriptor_t aps_request_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_aps_request_t,
                         "ApsRequest",
                         aps_request_txt,
                         "APS request/state.");

VTSS_SNMP_TAG_SERIALIZE(ApsInstance, uint32_t, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("Id"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The APS instance ID"));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_aps_ctrl_bool_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner), vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Set to TRUE to clear the counters of an APS instance."));
}

template<typename T>
void serialize(T &a, vtss_appl_aps_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_aps_capabilities_t"));
    int ix = 1;

    m.add_leaf(s.inst_cnt_max,
               vtss::tag::Name("InstanceMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of created APS instances."));
    m.add_leaf(s.wtr_secs_max,
               vtss::tag::Name("WtrSecsMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum WTR timer value in secs."));
    m.add_leaf(s.hold_off_msecs_max,
               vtss::tag::Name("HoldOffMsecsMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum Hold Off timer value in msec."));
}

template<typename T>
void serialize(T &a, vtss_appl_aps_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_aps_conf_t"));

    m.add_leaf(vtss::AsBool(s.admin_active),
               vtss::tag::Name("AdminActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The administrative state of this APS instance. Set to true to make it function normally "
                                      "and false to make it cease functioning."));

    m.add_leaf(vtss::AsStdDisplayString(s.w_port_conf.mep.md, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("WorkingMEPDomain"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Domain name of the working MEP."));

    m.add_leaf(vtss::AsStdDisplayString(s.w_port_conf.mep.ma, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("WorkingMEPService"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Service name of the working MEP."));

    m.add_leaf(s.w_port_conf.mep.mepid,
               vtss::tag::Name("WorkingMEPId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("MEPID of the working MEP."));

    m.add_leaf(vtss::AsStdDisplayString(s.p_port_conf.mep.md, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("ProtectingMEPDomain"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Domain name of the protecting MEP."));

    m.add_leaf(vtss::AsStdDisplayString(s.p_port_conf.mep.ma, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("ProtectingMEPService"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Service name of the protecting MEP."));

    m.add_leaf(s.p_port_conf.mep.mepid,
               vtss::tag::Name("ProtectingMEPId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("MEPID name of the protecting MEP."));

    m.add_leaf(s.mode,
               vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("Select the architecture and direction of the APS instance."));

    m.add_leaf(vtss::AsBool(s.tx_aps),
               vtss::tag::Name("TxApsEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("Choose whether this end transmits APS PDUs. Only for 1+1, unidirectional."));

    m.add_leaf(vtss::AsBool(s.revertive),
               vtss::tag::Name("Revertive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("Revertive operation can be enabled or disabled."));

    m.add_leaf(s.wtr_secs,
               vtss::tag::Name("WaitToRestoreSecs"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("Wait to restore timer in seconds - max. capabilities:WtrSecsMax  - min. 1."));

    m.add_leaf(s.hold_off_msecs,
               vtss::tag::Name("HoldOffTimerMSecs"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("Hold off timer in 100 ms steps - max. capabilities:HoldOffMsecsMax  - min. 0 means no hold off"));

    m.add_leaf(vtss::AsInterfaceIndex(s.w_port_conf.ifindex),
               vtss::tag::Name("WorkingIfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("Working port."));

    m.add_leaf(s.w_port_conf.sf_trigger,
               vtss::tag::Name("WorkingSfTrigger"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(14),
               vtss::tag::Description("Select the signal-fail triggering method for the working port."));

    m.add_leaf(vtss::AsInterfaceIndex(s.p_port_conf.ifindex),
               vtss::tag::Name("ProtectingIfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(15),
               vtss::tag::Description("Protecting port."));

    m.add_leaf(s.p_port_conf.sf_trigger,
               vtss::tag::Name("ProtectingSfTrigger"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(16),
               vtss::tag::Description("Select the signal-fail triggering method for the protecting port."));

    m.add_leaf(vtss::AsInt(s.level),
               vtss::tag::Name("Level"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 7),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(17),
               vtss::tag::Description("MD/MEG Level (0-7)."));

    m.add_leaf(s.vlan,
               vtss::tag::Name("Vid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(18),
               vtss::tag::Description("The VLAN ID used in the L-APS PDUs. 0 means untagged"));

    m.add_leaf(s.pcp,
               vtss::tag::Name("Pcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(19),
               vtss::tag::Description("PCP (priority) (default 7). The PCP value used in the VLAN tag unless the L-APS PDU is untagged. Must be a value in range [0; 7]. "));

    m.add_leaf(s.smac,
               vtss::tag::Name("Smac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(20),
               vtss::tag::Description("Source MAC address used in L-APS PDUs. Must be a unicast address. If all-zeros, the switch port's MAC address will be used."));
}

template<typename T>
void serialize(T &a, vtss_appl_aps_control_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_aps_control_t"));
    int ix = 1;

    m.add_leaf(s.command,
               vtss::tag::Name("Command"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("protection group command."));
}

template<typename T>
void serialize(T &a, vtss_appl_aps_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_aps_status_t"));

    m.add_leaf(s.oper_state,
               vtss::tag::Name("OperationalState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Operational state."));

    m.add_leaf(s.prot_state,
               vtss::tag::Name("ProtectionState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Protection state according to to G.8031 Annex A."));

    m.add_leaf(s.w_state,
               vtss::tag::Name("WorkingState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Working interface defect state."));

    m.add_leaf(s.p_state,
               vtss::tag::Name("ProtectingState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Protecting interface defect state."));

    m.add_leaf(s.tx_aps.request,
               vtss::tag::Name("TxApsRequest"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("Transmitted APS request."));

    m.add_leaf(s.tx_aps.re_signal,
               vtss::tag::Name("TxApsReSignal"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(6),
               vtss::tag::Description("Transmitted APS requested signal."));

    m.add_leaf(s.tx_aps.br_signal,
               vtss::tag::Name("TxApsBrSignal"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(7),
               vtss::tag::Description("Transmitted APS bridged signal."));

    m.add_leaf(s.rx_aps.request,
               vtss::tag::Name("RxApsRequest"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(8),
               vtss::tag::Description("Received APS request."));

    m.add_leaf(s.rx_aps.re_signal,
               vtss::tag::Name("RxApsReSignal"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(9),
               vtss::tag::Description("Received APS requested signal."));

    m.add_leaf(s.rx_aps.br_signal,
               vtss::tag::Name("RxApsBrSignal"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("Received APS bridged signal."));

    m.add_leaf(vtss::AsBool(s.dFOP_CM),
               vtss::tag::Name("DfopCM"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("FOP Configuration Mismatch - APS received on working."));

    m.add_leaf(vtss::AsBool(s.dFOP_PM),
               vtss::tag::Name("DfopPM"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("FOP Provisioning Mismatch."));

    m.add_leaf(vtss::AsBool(s.dFOP_NR),
               vtss::tag::Name("DfopNR"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("FOP No Response."));

    m.add_leaf(vtss::AsBool(s.dFOP_TO),
               vtss::tag::Name("DfopTO"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(14),
               vtss::tag::Description("FOP TimeOut."));

    m.add_leaf(s.smac,
               vtss::tag::Name("Smac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(15),
               vtss::tag::Description("Source MAC address of last received LAPS PDU or all-zeros if no PDU has been received."));

    m.add_leaf(vtss::AsCounter(s.tx_cnt),
               vtss::tag::Name("TxCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(16),
               vtss::tag::Description("Number of APS PDU frames transmitted."));

    m.add_leaf(vtss::AsCounter(s.rx_valid_cnt),
               vtss::tag::Name("RxValidCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(17),
               vtss::tag::Description("Number of valid APS PDU frames received on the protect port."));

    m.add_leaf(vtss::AsCounter(s.rx_invalid_cnt),
               vtss::tag::Name("RxInvalidCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(18),
               vtss::tag::Description("Number of invalid APS PDU frames received on the protect port."));

    m.add_leaf(s.oper_warning,
               vtss::tag::Name("OperationalWarning"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(100),
               vtss::tag::Description("Operational warning."));
}

namespace vtss
{
namespace appl
{
namespace aps
{
namespace interfaces
{

struct ApsCapabilitiesImpl {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_aps_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_aps_capabilities_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_aps_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_APS);
};

struct ApsConfigTableImpl {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_aps_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table of created APS instance parameters.";

    static constexpr const char *index_description =
        "This is a created APS instance parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, ApsInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_aps_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_aps_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_aps_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_aps_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_aps_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_aps_conf_del);
    VTSS_EXPOSE_DEF_PTR(vtss_aps_create_conf_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_APS);
};

struct ApsControlCommandTableImpl {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_aps_control_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table of created APS instance command. When an APS instance "
        "is created in the 'InstanceTable', an entry is automatically created "
        "here with 'no command'.";

    static constexpr const char *index_description =
        "This is a created APS instance command.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ApsInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_aps_control_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_aps_control_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_aps_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_aps_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_APS);
};

struct ApsControlStatisticsClearTableImpl {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<BOOL *>
         > P;

    static constexpr const char *table_description =
        "This is a table of created APS clear commands.";

    static constexpr const char *index_description =
        "This is a created APS clear command.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ApsInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("clear"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_aps_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_aps_itr);
    VTSS_EXPOSE_SET_PTR(vtss_aps_statistics_clear);
    VTSS_EXPOSE_GET_PTR(vtss_aps_statistics_clear_dummy);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_APS);
};

struct ApsStatusTableImpl {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_aps_status_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table of created APS instance status.";

    static constexpr const char *index_description =
        "This is a created APS instance status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, ApsInstance(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_aps_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_aps_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_aps_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_APS);
};

}  // namespace interfaces
}  // namespace aps
}  // namespace appl
}  // namespace vtss

#endif
