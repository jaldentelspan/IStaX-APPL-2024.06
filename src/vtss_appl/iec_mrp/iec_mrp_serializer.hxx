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

#ifndef __VTSS_IEC_MRP_SERIALIZER_HXX__
#define __VTSS_IEC_MRP_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "iec_mrp_api.h"
#include <vtss/appl/iec_mrp.h>
#include "vtss_appl_formatting_tags.hxx" // for AsStdDisplayString

mesa_rc vtss_iec_mrp_create_conf_default(uint32_t *instance, vtss_appl_iec_mrp_conf_t *s);
mesa_rc vtss_iec_mrp_statistics_clear(uint32_t instance, const BOOL *clear);
mesa_rc vtss_iec_mrp_statistics_clear_dummy(uint32_t instance,  BOOL *clear);

//----------------------------------------------------------------------------
// SNMP tagging for basic types
//----------------------------------------------------------------------------

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_iec_mrp_u32_dsc, u32, a, s )
{
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("groupIndex"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("IEC_MRP group index number. Valid range is (1..max groups). The maximum group number is platform-specific and can be retrieved from the MRP capabilities."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_iec_mrp_ctrl_bool_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner), vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Set to TRUE to clear the counters of an MRP instance."));
}

//----------------------------------------------------------------------------
// SNMP enums; become textual conventions
//----------------------------------------------------------------------------

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_port_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_port_type_t,
                         "IecMrpPortType",
                         vtss_appl_iec_mrp_port_type_txt,
                         "Specifies ring- and interconnection-ports.");

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_oper_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_oper_state_t,
                         "IecMrpOperState",
                         vtss_appl_iec_mrp_oper_state_txt,
                         "The operational state of an MRP instance.");

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_recovery_profile_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_recovery_profile_t,
                         "IecMrpRecoveryProfile",
                         vtss_appl_iec_mrp_recovery_profile_txt,
                         "Mrp recovery profile.");

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_role_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_role_t,
                         "IecMrpRole",
                         vtss_appl_iec_mrp_role_txt,
                         "The ring role of this instance.");

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_in_role_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_in_role_t,
                         "IecMrpInRole",
                         vtss_appl_iec_mrp_in_role_txt,
                         "Mrp interconnect role.");

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_sf_trigger_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_sf_trigger_t,
                         "IecMrpSfTrigger",
                         vtss_appl_iec_mrp_sf_trigger_txt,
                         "Signal fail can either come from the physical link on a given port or from a Down-MEP.");

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_ring_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_ring_state_t,
                         "IecMrpRingState",
                         vtss_appl_iec_mrp_ring_state_txt,
                         "Ring state.");

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_in_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_in_mode_t,
                         "IecMrpInMode",
                         vtss_appl_iec_mrp_in_mode_txt,
                         "Interconnect mode");

extern vtss_enum_descriptor_t vtss_appl_iec_mrp_oui_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_iec_mrp_oui_type_t,
                         "IecMrpInOUIType",
                         vtss_appl_iec_mrp_oui_type_txt,
                         "OUI type");

//----------------------------------------------------------------------------
// Capabilities
//----------------------------------------------------------------------------

template<typename T>
void serialize(T &a, vtss_appl_iec_mrp_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_iec_mrp_capabilities_t"));
    int ix = 1;

    m.add_leaf(s.inst_cnt_max,
               vtss::tag::Name("InstCntMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of configurable MRP instances"));

    m.add_leaf(vtss::AsBool(s.hw_tx_test_pdus),
               vtss::tag::Name("HwTxTestPdus"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether hardware transmits MRP_Test and MRP_InTest PDUs, in which case they cannot be Tx counted"));

    m.add_leaf(s.fastest_recovery_profile,
               vtss::tag::Name("FastestRecoveryProfile"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("This is the fastest recovery profile this switch supports"));
}

//----------------------------------------------------------------------------
// Configuration
//----------------------------------------------------------------------------

template<typename T>
void serialize(T &a, vtss_appl_iec_mrp_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_iec_mrp_conf_t"));
    int ix = 2;

    m.add_leaf(vtss::AsBool(s.admin_active),
               vtss::tag::Name("AdminActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The administrative state of this IEC_MRP instance. Set to true to make it function normally "
                                      "and false to make it cease functioning."));

    m.add_leaf(s.role,
               vtss::tag::Name("Role"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The ring role of this instance."));

    m.add_leaf(vtss::AsDisplayString(s.name, sizeof(s.name)),
               vtss::tag::Name("DomainName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("String used to help identifying the ring."));

    m.add_leaf(vtss::AsOctetString((uint8_t *)s.domain_id, sizeof(s.domain_id)),
               vtss::tag::Name("DomainId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Defines the redundancy domain representing the ring this MRP instance belongs to. A 16 octet value."
                                      "Encoded as a Universally Unique Identifier (UUID) and goes into the MRP PDUs."
                                      "Default is all-ones (FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF)"));

    m.add_leaf(s.oui_type,
               vtss::tag::Name("OUIType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("MRP_Option TLVs inside certain MRP PDU types contain an OUI.\n"
                                      "This OUI defaults to the three first bytes of the MAC address of this "
                                      "switch, but can be changed to both the Siemens OUI and a custom OUI"));

    m.add_leaf(vtss::AsOctetString((uint8_t *)s.custom_oui, sizeof(s.custom_oui)),
               vtss::tag::Name("CustomOUI"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("OUI used when OUIType == custom"));

    m.add_leaf(vtss::AsInterfaceIndex(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].ifindex),
               vtss::tag::Name("RingPort1Interface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort1, this is the Interface index of ring port.\n"
                                      "Must always be filled in."));

    m.add_leaf(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].sf_trigger,
               vtss::tag::Name("RingPort1SfTrigger"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort1, this selects whether Signal Fail (SF) comes from the link state of a given interface, or from a Down-MEP."));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].mep.md, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("RingPort1MEPDomain"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort1, this is the MEP Domain"));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].mep.ma, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("RingPort1MEPService"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort1, this is the MEP Service."));

    m.add_leaf(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].mep.mepid,
               vtss::tag::Name("RingPort1MEPId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort1, this is the MEP ID"));

    m.add_leaf(vtss::AsInterfaceIndex(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].ifindex),
               vtss::tag::Name("RingPort2Interface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort2, this is the Interface index of ring port.\n"
                                      "Must always be filled in."));

    m.add_leaf(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].sf_trigger,
               vtss::tag::Name("RingPort2SfTrigger"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort2, this selects whether Signal Fail (SF) comes from the link state of a given interface, or from a Down-MEP."));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].mep.md, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("RingPort2MEPDomain"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort2, this is the MEP Domain"));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].mep.ma, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("RingPort2MEPService"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort2, this is the MEP Service."));

    m.add_leaf(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].mep.mepid,
               vtss::tag::Name("RingPort2MEPId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For RingPort2, this is the MEP ID"));

    m.add_leaf(s.vlan,
               vtss::tag::Name("Vid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The VLAN ID on which MRP PDUs are transmitted and received on the ring ports."));

    m.add_leaf(s.recovery_profile,
               vtss::tag::Name("RecoveryProfile"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Recovery time profile. Selects the recovery time profile in accordance with the standard's "
                                      "Table 59 (MRM) and Table 60 (MRC) on p. 129."));

    m.add_leaf(s.mrm.prio,
               vtss::tag::Name("MrmPrio"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Manager Priority.\n"
                                      "Lower values indicate a higher priority."));

    m.add_leaf(vtss::AsBool(s.mrm.react_on_link_change),
               vtss::tag::Name("MrmReactOnLinkChange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether the MRM reacts immediately on MRP_LinkChange PDUs or not."
                                      "Considered false on MRAs."));

    m.add_leaf(s.in_role,
               vtss::tag::Name("InRole"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The interconnect ring role of this instance.\n"
                                      "Defaults to 'none', that is, this is not an interconnect node."));

    m.add_leaf(s.in_mode,
               vtss::tag::Name("InMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls whether the MIM or MIC is in LC- or RC-mode.\n"
                                      "In LC-mode, link changes are used to figure out interconnection topology.\n"
                                      "In RC-mode, MRP_InTest PDUs are used to figure out the topology.\n"
                                      "Default is LC-mode."));

    m.add_leaf(s.in_id,
               vtss::tag::Name("InId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The interconnection ID used to identify this interconnection domain.\n"
                                      "The same ID must be used on all nodes that are part of this interconnection domain."));

    m.add_leaf(vtss::AsDisplayString(s.in_name, sizeof(s.in_name)),
               vtss::tag::Name("InName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Specifies the name of the interconnection domain.\n"
                                      "It is only used to help identifying the interconnection nodes."));

    m.add_leaf(vtss::AsInterfaceIndex(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION].ifindex),
               vtss::tag::Name("InPortInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface index of interconnection port.\n"
                                      "Must always be filled in."));

    m.add_leaf(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION].sf_trigger,
               vtss::tag::Name("InPortSfTrigger"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Selects whether Signal Fail (SF) comes from the link state of a given interface, or from a Down-MEP."));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION].mep.md, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("InPortMEPDomain"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For interconnection port, this is the MEP Domain."));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION].mep.ma, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("InPortMEPService"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For interconnection port, this is the MEP Service."));

    m.add_leaf(s.ring_port[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION].mep.mepid,
               vtss::tag::Name("InPortMEPId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For interconnection port, this is the MEP ID."));

    m.add_leaf(s.in_vlan,
               vtss::tag::Name("InVid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The VLAN ID on which MRP PDUs are transmitted and received on the interconnection port."));

    m.add_leaf(s.in_recovery_profile,
               vtss::tag::Name("InRecoveryProfile"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interconnection Recovery time profile.\n"
                                      "Selects the recovery time profile in accordance with the standard's "
                                      "Table 61 (MIM) on p. 129 and Table 62 (MIC) on p. 130."));

}

//----------------------------------------------------------------------------
// Status
//----------------------------------------------------------------------------

template<typename T>
void serialize(T &a, vtss_appl_iec_mrp_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_iec_mrp_status_t"));
    int                           ix = 2;
    vtss_appl_iec_mrp_port_type_t port_type;
    vtss_appl_iec_mrp_pdu_type_t  pdu_type;
    char                          namebuf[128];
    char                          dscrbuf[512];

    m.add_leaf(s.oper_state,
               vtss::tag::Name("OperState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The operational state of IEC_MRP instance."));

    m.add_leaf(s.oper_warnings,
               vtss::tag::Name("OperWarnings"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The operational warnings of IEC_MRP instance."));

    m.add_leaf(s.ring_state,
               vtss::tag::Name("RingState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Specifies current MRP instance ring state."));

    m.add_leaf(s.oper_role,
               vtss::tag::Name("OperRole"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The actual ring role of this instance."));

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {

#define _PORT_NAME(_name_, _dscr_)                                                      \
    sprintf(namebuf, "%s%s",   iec_mrp_util_port_type_to_short_str(port_type), _name_); \
    sprintf(dscrbuf, "%s: %s", iec_mrp_util_port_type_to_short_str(port_type), _dscr_);

#define _PORT_NAME_PDU(_rx_)                                                                                                                                                        \
    sprintf(namebuf, "%s%s%sCnt",                iec_mrp_util_port_type_to_short_str(port_type), _rx_ ? "Rx"       : "Tx",          iec_mrp_util_pdu_type_to_str(pdu_type, false)); \
    sprintf(dscrbuf, "%s: Number of %s %s PDUs", iec_mrp_util_port_type_to_short_str(port_type), _rx_ ? "received" : "transmitted", iec_mrp_util_pdu_type_to_str(pdu_type, true));

        // Tx counters first. Exclude the unknown counter
        for (pdu_type = (vtss_appl_iec_mrp_pdu_type_t)0; pdu_type < VTSS_APPL_IEC_MRP_PDU_TYPE_LAST - 1; pdu_type++) {
            _PORT_NAME_PDU(false);
            m.add_leaf(vtss::AsCounter(s.port_status[port_type].statistics.tx_cnt[pdu_type]),
                       vtss::tag::Name(namebuf),
                       vtss::expose::snmp::Status::Current,
                       vtss::expose::snmp::OidElementValue(ix++),
                       vtss::tag::Description(dscrbuf));
        }

        // Rx counters next. Include the unknown counter
        for (pdu_type = (vtss_appl_iec_mrp_pdu_type_t)0; pdu_type < VTSS_APPL_IEC_MRP_PDU_TYPE_LAST; pdu_type++) {
            _PORT_NAME_PDU(true); // Then Rx counters.
            m.add_leaf(vtss::AsCounter(s.port_status[port_type].statistics.rx_cnt[pdu_type]),
                       vtss::tag::Name(namebuf),
                       vtss::expose::snmp::Status::Current,
                       vtss::expose::snmp::OidElementValue(ix++),
                       vtss::tag::Description(dscrbuf));
        }
#undef _PORT_NAME_PDU

        _PORT_NAME("RxErrCnt", "Number of received erroneous MRP PDUs")
        m.add_leaf(vtss::AsCounter(s.port_status[port_type].statistics.rx_error_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscrbuf));

        _PORT_NAME("RxUnhandledCnt", "Number of received unhandled MRP PDUs")
        m.add_leaf(vtss::AsCounter(s.port_status[port_type].statistics.rx_unhandled_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscrbuf));

        _PORT_NAME("RxOwnCnt", "Number of received MRP PDUs with our own SMAC")
        m.add_leaf(vtss::AsCounter(s.port_status[port_type].statistics.rx_own_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscrbuf));

        _PORT_NAME("SfCnt", "Number of local signal fails")
        m.add_leaf(vtss::AsCounter(s.port_status[port_type].statistics.sf_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscrbuf));
#undef _PORT_NAME
    }

    m.add_leaf(vtss::AsCounter(s.transitions),
               vtss::tag::Name("Transitions"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of transitions between ring open and ring closed state.\n"
                                      "Corresponds to the standard's MRP_Transition for ring ports."));

    m.add_leaf(vtss::AsCounter(s.mrm_mrc_transitions),
               vtss::tag::Name("MrmMrcTransitions"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of transitions between MRM and MRC when configured role is MRA.\n"
                                      "Always 0 when configured role is MRM or MRC.\n"
                                      "No corresponding counter in the standard."));

    m.add_leaf(vtss::AsCounter(s.flush_cnt),
               vtss::tag::Name("FdbFlushCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of FDB flushes."));

    m.add_leaf(vtss::AsBool(s.round_trip_time.valid),
               vtss::tag::Name("RoundTripValid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("On many platforms, round-trip time cannot be computed, because the "
                                      "transmitted MRP_Test/MRP_InTest PDUs are not timestamped, because they "
                                      "are sent by H/W, which doesn't always have the option of timestamping, "
                                      "so the following timestamp round-trip times are only valid if this "
                                      "parameter is true."));

    m.add_leaf(s.round_trip_time.msec_min,
               vtss::tag::Name("RoundTripMsecMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum round-trip delay in milliseconds of own MRP_Test PDUs."));

    m.add_leaf(s.round_trip_time.msec_max,
               vtss::tag::Name("RoundTripMsecMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum round-trip delay in milliseconds of own MRP_Test PDUs."));

    m.add_leaf(s.round_trip_time.msec_last,
               vtss::tag::Name("RoundTripMaxLast"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Latest round-trip delay in milliseconds of own MRP_Test PDUs."));

    m.add_leaf(vtss::AsCounter(s.round_trip_time.last_update_secs),
               vtss::tag::Name("RoundTripLastUpdateSec"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Time since boot in seconds of last update of this structure."
                                      "If it's zero, the remaining members of RoundTrip elements are invalid."));

    m.add_leaf(s.in_ring_state,
               vtss::tag::Name("InRingState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The interconnection ring state."));

    m.add_leaf(vtss::AsCounter(s.in_transitions),
               vtss::tag::Name("InTransitions"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of transitions between interconnection toplogy open state and \n"
                                      "interconnection topology closed state.\n"
                                      "Corresponds to the standard's MRP_Transition for interconnection ports."));

    m.add_leaf(s.in_round_trip_time.msec_min,
               vtss::tag::Name("InRoundTripMsecMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum round-trip delay in milliseconds of own MRP_InTest PDUs."));

    m.add_leaf(s.in_round_trip_time.msec_max,
               vtss::tag::Name("InRoundTripMsecMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum round-trip delay in milliseconds of own MRP_InTest PDUs."));

    m.add_leaf(s.in_round_trip_time.msec_last,
               vtss::tag::Name("InRoundTripMaxLast"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Latest round-trip delay in milliseconds of own MRP_InTest PDUs."));

    m.add_leaf(s.in_round_trip_time.last_update_secs,
               vtss::tag::Name("InRoundTripLastUpdateSec"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Time since boot in seconds of last update of this structure. \n"
                                      "If it's zero, the remaining members of InRoundTrip elements are invalid."));
}

template<typename T>
void serialize(T &a, vtss_appl_iec_mrp_notification_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_iec_mrp_notification_status_t"));
    int ix = 0;

    m.add_leaf(vtss::AsBool(s.ring_open),
               vtss::tag::Name("RingOpen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether the ring is open or not.\n"
                                      "Can only be raised by nodes in the MRM operating role.\n"
                                      "Corresponds to the standard's RING_OPEN"));

    m.add_leaf(vtss::AsBool(s.multiple_mrms),
               vtss::tag::Name("MultipleMrms"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether there is more than one MRP manager on the ring.\n"
                                      "It is normal to have this raised shortly during manager negotiation when "
                                      "multiple ring devices are configured as MRAs.\n"
                                      "If no MRP_Test PDUs have been seen for 10 seconds, this event falls back to false."
                                      "When this is set, VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MRMS will be "
                                      "set in vtss_appl_iec_mrp_status::OperWarnings.\n"
                                      "Can only be raised by nodes in the MRM operating role.\n"
                                      "Corresponds to the standard's MULTIPLE_MANAGERS." ));

    m.add_leaf(vtss::AsBool(s.in_open),
               vtss::tag::Name("InOpen"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether the interconnection ring is open or not.\n"
                                      "Can only be raised by MIMs.\n"
                                      "Corresponds to the standard's INTERCONNECTION_OPEN."));

    m.add_leaf(vtss::AsBool(s.multiple_mims),
               vtss::tag::Name("MultipleMims"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether there is more than one MIM on the interconnection ring.\n"
                                      "This event is only raised if the MRP_InTest PDU's Interconnection ID "
                                      "field contains the same value as the one configured for this instance "
                                      "(see vtss_appl_iec_mrp_conf::in_id)."
                                      "If no MRP_InTest PDUs have been seen for 10 seconds, this event falls back to false.\n"
                                      "When this is set, VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MIMS will be "
                                      "set in vtss_appl_iec_mrp_status::OperWarnings.\n"
                                      "No correspoding event in the standard."));

}

namespace vtss
{
namespace appl
{
namespace iec_mrp
{
namespace interfaces
{

struct IecMrpCapabilities {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_iec_mrp_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_iec_mrp_capabilities_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_iec_mrp_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IEC_MRP);
};

struct IecMrpConfTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_iec_mrp_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is the ERPS group configuration table.";

    static constexpr const char *index_description =
        "Each entry in this table represents an IECMRP group.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, vtss_appl_iec_mrp_u32_dsc(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_iec_mrp_conf_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_iec_mrp_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iec_mrp_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_iec_mrp_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_iec_mrp_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_iec_mrp_conf_del);
    VTSS_EXPOSE_DEF_PTR(vtss_iec_mrp_create_conf_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IEC_MRP);
};

struct IecMrpStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_iec_mrp_status_t *>
         > P;

    static constexpr const char *table_description =
        "This table contains status per IECMRP group.";

    static constexpr const char *index_description =
        "Status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, vtss_appl_iec_mrp_u32_dsc(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_iec_mrp_status_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_iec_mrp_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iec_mrp_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IEC_MRP);
};

struct IecMrpControlStatisticsClearTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<BOOL *>
         > P;

    static constexpr const char *table_description =
        "This is a table of created IECMRP clear commands.";

    static constexpr const char *index_description =
        "This is a created IECMRP clear command.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, vtss_appl_iec_mrp_u32_dsc(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("clear"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_iec_mrp_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_iec_mrp_itr);
    VTSS_EXPOSE_SET_PTR(vtss_iec_mrp_statistics_clear);
    VTSS_EXPOSE_GET_PTR(vtss_iec_mrp_statistics_clear_dummy);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IEC_MRP);
};

struct IecMrpStatusNotificationTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_iec_mrp_notification_status_t *>
         > P;

    static constexpr const char *table_description =
        "This table contains status per IECMRP group.";

    static constexpr const char *index_description =
        "Status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, vtss_appl_iec_mrp_u32_dsc(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_iec_mrp_notification_status_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_iec_mrp_notification_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iec_mrp_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IEC_MRP);
};

}  // namespace interfaces
}  // namespace iec_mrp
}  // namespace appl
}  // namespace vtss
#endif  /* __VTSS_IEC_MRP_SERIALIZER_HXX__ */

