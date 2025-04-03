/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_CFM_SERIALIZER_HXX__
#define __VTSS_CFM_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/cfm.hxx"
#include "vtss/basics/expose/json.hxx"
#include "vtss_appl_formatting_tags.hxx" // for AsInterfaceIndex
#include <vtss/appl/types.hxx>


const vtss_enum_descriptor_t vtss_appl_cfm_sender_id_tlv_option_txt[] = {
    {VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE,        "none"},
    {VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS,        "chassis"},
    {VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_MANAGE,         "manage"},
    {VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS_MANAGE, "chassismanage"},
    {VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER,          "defer"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_ma_format_txt[] = {
    {VTSS_APPL_CFM_MA_FORMAT_STRING,                    "charString"},
    {VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER,         "unsignedInt16"},
    {VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC,                 "y1731Icc"},
    {VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC,              "y1731IccCc"},
    {VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID,               "primaryVid"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_ccm_interval_txt[] = {
    {VTSS_APPL_CFM_CCM_INTERVAL_INVALID,                "intervalInvalid"},
    {VTSS_APPL_CFM_CCM_INTERVAL_300HZ,                  "interval300Hz"},
    {VTSS_APPL_CFM_CCM_INTERVAL_10MS,                   "interval10ms"},
    {VTSS_APPL_CFM_CCM_INTERVAL_100MS,                  "interval100ms"},
    {VTSS_APPL_CFM_CCM_INTERVAL_1S,                     "interval1s"},
    {VTSS_APPL_CFM_CCM_INTERVAL_10S,                    "interval10s"},
    {VTSS_APPL_CFM_CCM_INTERVAL_1MIN,                   "interval1min"},
    {VTSS_APPL_CFM_CCM_INTERVAL_10MIN,                  "interval10min"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_direction_txt[] = {
    {VTSS_APPL_CFM_DIRECTION_DOWN,                      "down"},
    {VTSS_APPL_CFM_DIRECTION_UP,                        "up"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_md_format_txt[] = {
    {VTSS_APPL_CFM_MD_FORMAT_NONE,                      "none"},
    {VTSS_APPL_CFM_MD_FORMAT_STRING,                    "string"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_mep_defect_txt[] = {
    {VTSS_APPL_CFM_MEP_DEFECT_NONE,                     "none"},
    {VTSS_APPL_CFM_MEP_DEFECT_RDI_CCM,                  "defRDICCM"},
    {VTSS_APPL_CFM_MEP_DEFECT_MAC_STATUS,               "defMACstatus"},
    {VTSS_APPL_CFM_MEP_DEFECT_REMOTE_CCM,               "defRemoteCCM"},
    {VTSS_APPL_CFM_MEP_DEFECT_ERROR_CCM,                "defErrorCCM"},
    {VTSS_APPL_CFM_MEP_DEFECT_XCON_CCM,                 "defXconCCM"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_tlv_option_txt[] = {
    {VTSS_APPL_CFM_TLV_OPTION_DISABLE,                  "disable"},
    {VTSS_APPL_CFM_TLV_OPTION_ENABLE,                   "enable"},
    {VTSS_APPL_CFM_TLV_OPTION_DEFER,                    "defer"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_rmep_state_txt[] = {
    {VTSS_APPL_CFM_RMEP_STATE_IDLE,                     "idle"},
    {VTSS_APPL_CFM_RMEP_STATE_START,                    "start"},
    {VTSS_APPL_CFM_RMEP_STATE_FAILED,                   "failed"},
    {VTSS_APPL_CFM_RMEP_STATE_OK,                       "ok"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_port_status_txt[] = {
    {VTSS_APPL_CFM_PORT_STATUS_NOT_RECEIVED,            "notReceived"},
    {VTSS_APPL_CFM_PORT_STATUS_BLOCKED,                 "blocked"},
    {VTSS_APPL_CFM_PORT_STATUS_UP,                      "up"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_interface_status_txt[] = {
    {VTSS_APPL_CFM_INTERFACE_STATUS_NOT_RECEIVED,       "notReceived"},
    {VTSS_APPL_CFM_INTERFACE_STATUS_UP,                 "up"},
    {VTSS_APPL_CFM_INTERFACE_STATUS_DOWN,               "down"},
    {VTSS_APPL_CFM_INTERFACE_STATUS_TESTING,            "testing"},
    {VTSS_APPL_CFM_INTERFACE_STATUS_UNKNOWN,            "unknown"},
    {VTSS_APPL_CFM_INTERFACE_STATUS_DORMANT,            "dormant"},
    {VTSS_APPL_CFM_INTERFACE_STATUS_NOT_PRESENT,        "notPresent"},
    {VTSS_APPL_CFM_INTERFACE_STATUS_LOWER_LAYER_DOWN,   "lowerLayerDown"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_fng_state_txt[] = {
    {VTSS_APPL_CFM_FNG_STATE_RESET,                     "reset"},
    {VTSS_APPL_CFM_FNG_STATE_DEFECT,                    "defect"},
    {VTSS_APPL_CFM_FNG_STATE_REPORT_DEFECT,             "reportDefect"},
    {VTSS_APPL_CFM_FNG_STATE_DEFECT_REPORTED,           "defectReported"},
    {VTSS_APPL_CFM_FNG_STATE_DEFECT_CLEARING,           "defectClearing"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_cfm_chassis_id_subtype_txt[] = {
    {VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_NOT_RECEIVED,      "notReceived"},
    {VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_CHASSIS_COMPONENT, "chassisComponent"},
    {VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_INTERFACE_ALIAS,   "interfaceAlias"},
    {VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_PORT_COMPONENT,    "portComponent"},
    {VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_MAC_ADDRESS,       "macAddress"},
    {VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_NETWORK_ADDRESS,   "networkAddress"},
    {VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_INTERFACE_NAME,    "interfaceName"},
    {VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_LOCAL,             "local"},
    {0, 0}
};


/*****************************************************************************
    Enum serializer
*****************************************************************************/
VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_sender_id_tlv_option_t,
    "CfmSenderIdTlv",
    vtss_appl_cfm_sender_id_tlv_option_txt,
    "This enumeration defines the sender ID Tlv options.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_ma_format_t,
    "CfmMaNameFormat",
    vtss_appl_cfm_ma_format_txt,
    "This enumeration defines the Maintenance Association name format.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_md_format_t,
    "CfmMdNameFormat",
    vtss_appl_cfm_md_format_txt,
    "This enumeration defines the Maintenance Domain name format.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_ccm_interval_t,
    "CfmCcmInterval",
    vtss_appl_cfm_ccm_interval_txt,
    "The enumeration values correspond to those used in the CCM PDUs interval field (802.1Q-2018, Table 21-15)");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_direction_t,
    "CfmDirection",
    vtss_appl_cfm_direction_txt,
    "The enumeration values correspond to the direction of a particular MIP/MEP.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_mep_defect_t,
    "CfmMepDefect",
    vtss_appl_cfm_mep_defect_txt,
    "The enumeration values correspond to the defect of a particular MEP.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_tlv_option_t,
    "CfmMepTlvOption",
    vtss_appl_cfm_tlv_option_txt,
    "The enumeration value determines if Port or Interface Status TLVs should be put into PDUs.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_rmep_state_t,
    "CfmMepRmepState",
    vtss_appl_cfm_rmep_state_txt,
    "The enumeration values correspond to oaperational state of a Remote MEP's state machine.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_port_status_t,
    "CfmMepPortStatus",
    vtss_appl_cfm_port_status_txt,
    "The enumeration values correspond to port status TLV.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_interface_status_t,
    "CfmMepInterfaceStatus",
    vtss_appl_cfm_interface_status_txt,
    "The enumeration values correspond to interface status TLV.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_fng_state_t,
    "CfmMepFngState",
    vtss_appl_cfm_fng_state_txt,
    "The enumeration values correspond to state of fault notification generator state machine.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_cfm_chassis_id_subtype_t,
    "CfmMepChassisIdSubtype",
    vtss_appl_cfm_chassis_id_subtype_txt,
    "The enumeration values correspond to allowed Chassis ID subtypes. They match those of LLDP-MIB 2005 for the LldpChassisIdSubtype textual convention.");


/****************************************************************************
 * vtss_appl_cfm_md_key_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_cfm_md_key_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_md_key_t"));
    int ix = 0;

    m.add_leaf(s.md,
               vtss::tag::Name("md"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Md table."));
}

/****************************************************************************
 * vtss_appl_cfm_ma_key_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_cfm_ma_key_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_ma_key_t"));
    int ix = 0;

    m.add_leaf(s.md,
               vtss::tag::Name("md"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Md table."));

    m.add_leaf(s.ma,
               vtss::tag::Name("ma"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Ma table."));

}

/****************************************************************************
  * vtss_appl_cfm_mep_key_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_cfm_mep_key_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_mep_key_t"));
    int ix = 0;

    m.add_leaf(s.md,
               vtss::tag::Name("md"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Md table."));

    m.add_leaf(s.ma,
               vtss::tag::Name("ma"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Ma table."));

    m.add_leaf(s.mepid,
               vtss::tag::Name("mep"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Mep table."));

}

/****************************************************************************
 * vtss_appl_cfm_rmep_key_t
 ****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_cfm_rmep_key_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_rmep_key_t"));
    int ix = 0;

    m.add_leaf(s.md,
               vtss::tag::Name("md"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Md table."));

    m.add_leaf(s.ma,
               vtss::tag::Name("ma"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Ma table."));

    m.add_leaf(s.mepid,
               vtss::tag::Name("mep"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the Mep table."));

    m.add_leaf(s.rmepid,
               vtss::tag::Name("rmep"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Index of the RMep table."));
}

template<typename T>
void serialize(T &a, vtss_appl_cfm_global_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_global_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.sender_id_tlv_option,
        vtss::tag::Name("SenderIdTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether and what to use as Sender ID for PDUs generated by this "
                               "switch. The value DEFER is invalid at this global level. Can be overridden "
                               "by MD and MA. Default is DISABLE.")
    );

    m.add_leaf(
        s.port_status_tlv_option,
        vtss::tag::Name("PortStatusTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Port Status TLVs in CCMs generated by this "
                               "switch. The value DEFER is invalid at this global level. "
                               "Can be overridden by MD and MA. Default DISABLE.")
    );

    m.add_leaf(
        s.interface_status_tlv_option,
        vtss::tag::Name("InterfaceStatusTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Interface Status TLVs in CCMs generated by this "
                               "switch. The value DEFER is invalid at this global level. "
                               "Can be overridden by MD and MA. Default DISABLE.")
    );

    m.add_leaf(
        s.organization_specific_tlv_option,
        vtss::tag::Name("OrgSpecificTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Organization-Specific TLV in CFM PDUs generated by "
                               "this switch. The value DEFER is invalid at this global level. Can be overridden "
                               "by MD and MA. Default is DISABLE.")
    );

    m.add_leaf(
        vtss::AsOctetString(s.organization_specific_tlv.oui, sizeof(s.organization_specific_tlv.oui)),
        vtss::tag::Name("OrgSpecificTlvOui"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This is the three-bytes OUI transmitted with the Organization-Specific "
                               "TLV. There is no check on contents.")
    );

    m.add_leaf(
        s.organization_specific_tlv.subtype,
        vtss::tag::Name("OrgSpecificTlvSubtype"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This is the subtype transmitted with the Organization-Specific TLV. "
                               "Can be any value in range [0; 255].")
    );

    m.add_leaf(
        s.organization_specific_tlv.value_len,
        vtss::tag::Name("OrgSpecificTlvValueLen"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of valid bytes in OrgSpecificTlvValue.")
    );

    m.add_leaf(
        vtss::AsDisplayString((char *)s.organization_specific_tlv.value, sizeof(s.organization_specific_tlv.value)),
        vtss::tag::Name("OrgSpecificTlvValue"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The value transmitted in the Organization-Specific TLV.")
    );

}

template<typename T>
void serialize(T &a, vtss_appl_cfm_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_capabilities_t"));
    int ix = 0;

    m.add_leaf(
        s.md_cnt_max,
        vtss::tag::Name("MdCntMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Maximum number of Maintenance Domains that can be created.")
    );

    m.add_leaf(
        s.ma_cnt_max,
        vtss::tag::Name("MaCntMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Maximum number of Maintenance Associations that can be created.")
    );

    m.add_leaf(
        s.mep_cnt_port_max,
        vtss::tag::Name("MepCntPortMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Maximum number of Port MEPs (untagged) that can be created.")
    );

    m.add_leaf(
        s.mep_cnt_service_max,
        vtss::tag::Name("MepCntServiceMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Maximum number of Service MEPs (on VLANs) that can be created.")
    );

    m.add_leaf(
        s.rmep_cnt_max,
        vtss::tag::Name("RmepCntMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Maximum number of Remote MEPs that can be monitored per MEP.")
    );

    m.add_leaf(
        s.ccm_interval_min,
        vtss::tag::Name("CcmIntervalMin"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Not all platforms support the 300 CCM frames per second that the standard "
                               "indicates. This value indicates the minimum value you may "
                               "assign to CCM interval on this platform. It will "
                               "contain 300HZ if the fastest standardized rate is supported.")
    );

    m.add_leaf(
        s.ccm_interval_max,
        vtss::tag::Name("CcmIntervalMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Not all platforms support the slowest CCM frames that the standard "
                               "otherwise dictates. This value indicates the maximum value you may "
                               "assign to CCM interval on this platform. It will "
                               "contain 10MIN if the slowest standardized rate is supported.")
    );

    m.add_leaf(
        vtss::AsBool(s.has_mips),
        vtss::tag::Name("HasMips"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is false, this platform only supports MEPs. "
                               "If true, it supports both MEPs and MIPs.")
    );

    m.add_leaf(
        vtss::AsBool(s.has_up_meps),
        vtss::tag::Name("HasUpMeps"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is false, this platform only supports down-MEPs. "
                               "If true, it supports both up- and down-MEPs.")
    );

    m.add_leaf(
        vtss::AsBool(s.has_vlan_meps),
        vtss::tag::Name("HasVlanMeps"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is false, only port MEPs are supported. "
                               "If true, both VLAN and port MEPs are supported.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_cfm_md_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_md_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.format,
        vtss::tag::Name("Format"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Select the MD name format. To mimic Y.1731 MEG IDs, use type NONE.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.name, sizeof(s.name)),
        vtss::tag::Name("Name"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Maintenance Domain Name. The contents of this pamameter depends on the value of the format member. "
                               "If format is NONE: name is not used, but will be set to all-zeros behind the scenes."
                               " This format is typically used by Y.1731-kind-of-PDUs."
                               "If format is STRING: name must contain a string from 1 to 43 characters long plus a "
                               " terminating NULL.")
    );

    m.add_leaf(
        vtss::AsInt(s.level),
        vtss::tag::Name("Level"),
        vtss::expose::snmp::RangeSpec<uint32_t>(0, 7),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("MD Level (0-7).")
    );


    m.add_leaf(
        s.sender_id_tlv_option,
        vtss::tag::Name("SenderIdTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether and what to use as Sender ID for PDUs generate in this MD. "
                               "Can be overridden by MA configuration. Default is DEFER, which means: "
                               "Let the global configuration apply.")
    );

    m.add_leaf(
        s.port_status_tlv_option,
        vtss::tag::Name("PortStatusTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Port Status TLVs in CCMs generated by this switch. "
                               "Can be overridden by MA configuration. Default is DEFER, which means: "
                               "Let the global configuration apply.")
    );

    m.add_leaf(
        s.interface_status_tlv_option,
        vtss::tag::Name("InterfaceStatusTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Interface Status TLVs in CCMs generated by this "
                               "switch. Can be overridden by MA configuration. Default is DEFER, "
                               "which means: Let the global configuration apply.")
    );

    m.add_leaf(
        s.organization_specific_tlv_option,
        vtss::tag::Name("OrgSpecificTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Organization-Specific TLV in CFM PDUs generated by "
                               "this switch. Can be overridden by MA configuration. Default is DEFER, "
                               "which means: Let the global configuration apply.")
    );

}

template<typename T>
void serialize(T &a, vtss_appl_cfm_ma_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_ma_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.format,
        vtss::tag::Name("Format"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Select the short MA name format. To mimic Y.1731 MEG IDs, "
                               "create an MD instance with an empty name and use Y1731_ICC or Y1731_ICC_CC.")
    );

    switch (s.format) {
    case VTSS_APPL_CFM_MA_FORMAT_STRING:
    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC:
    case VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC:
        m.add_leaf(
            vtss::AsDisplayString(s.name, sizeof(s.name)),
            vtss::tag::Name("Name"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Maintenance Association Name. "
                                   "The contents of this parameter depends on the value of the format member. "
                                   "Besides the limitations explained for each of them, the following applies in general: \n"
                                   "If the MD format is NONE, the size of this cannot exceed 45 bytes.\n"
                                   "If the MD format is not NONE, the size of this cannot exceed 44 bytes.\n"
                                   "If format is STRING, the following applies: \n"
                                   " length must be in range [1; 45] and must be NULL-terminated."
                                   "     Contents must be in range [32; 126] (isprint()).\n"
                                   "If format is TWO_OCTET_INTEGER, the following applies: \n"
                                   " name[0] and name[1] will both be interpreted as unsigned 8-bit integers"
                                   "(allowing a range of [0; 255]). name[0] will be placed in the PDU before name[1]. "
                                   "The remaining available bytes in name[] will not be used."
                                   "If format is Y1731_ICC, the following applies: \n"
                                   "  length must be 13. The string must be NULL-terminated.\n"
                                   "  Contents must be in range [a-zA-Z0-9] (isalnum()).\n"
                                   " Y.1731 specifies that it is a concatenation of ICC (ITU Carrier Code) "
                                   " and UMC (Unique MEG ID Code): \n"
                                   "    ICC: 1-6 bytes\n"
                                   "    UMC: 7-12 bytes\n"
                                   "In principle UMC can be any value in range [1; 127], but this API does "
                                   "not allow for specifying length of ICC, so the underlying code doesn't know "
                                   "where ICC ends and UMC starts."
                                   "When using this, the MD format must NONE.\n"
                                   "If format is Y1731_ICC_CC, the following applies:\n"
                                   " length must be 15.\n"
                                   " First 2 chars   (CC):  Must be amongst [A-Z] (isupper()).\n"
                                   " Next 1-6 chars  (ICC): Must be amongst [a-zA-Z0-9] (isalnum()).\n"
                                   " Next 7-12 chars (UMC): Must be amongst [a-zA-Z0-9] (isalnum()).\n"
                                   " There may be ONE (slash) present in name[3-7].\n"
                                   " When using this, the MD format must be NONE.")
        );
        break;
    case VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER:
        m.add_leaf(
            vtss::AsOctetString((uint8_t *)s.name, 2),
            vtss::tag::Name("Name"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("The two octet value.")
        );
        break;
    case VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID:
        m.add_leaf(
            vtss::AsOctetString((uint8_t *)s.name, 0),
            vtss::tag::Name("Name"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description("Empty name.")
        );

        break;
    }

    m.add_leaf(
        s.vlan,
        vtss::tag::Name("Vid"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The MA's primary VID. A primary VID of 0 means that all MEPs created within this MA "
                               "will be created as port MEPs (interface MEPs). There can only be one port MEP per interface. "
                               "A given port MEP may still be created with tags, if that MEP's VLAN is non-zero.\n"
                               "A non-zero primary VID means that all MEPs created within this MA will be created as VLAN MEPs. "
                               "A given MEP may be configured with another VLAN than the MA's primary VID, but it is impossible "
                               "to have untagged VLAN MEPs.")
    );

    m.add_leaf(
        s.ccm_interval,
        vtss::tag::Name("CcmInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The CCM rate of all MEPs bound to this MA. Must not be INVALID. "
                               "Also, not all rates are supported on all platforms "
                               "(see capabilities CcmIntervalMin and CcmIntervalMin).")
    );

    m.add_leaf(
        s.sender_id_tlv_option,
        vtss::tag::Name("SenderIdTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether and what to use as Sender ID for PDUs generated in this MA. "
                               "Default is DEFER, which means: Let the MD configuration apply.")
    );

    m.add_leaf(
        s.port_status_tlv_option,
        vtss::tag::Name("PortStatusTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Port Status TLVs in CCMs generated by this switch. "
                               "Default is DEFER, which means: Let the MD configuration apply. ")
    );

    m.add_leaf(
        s.interface_status_tlv_option,
        vtss::tag::Name("InterfaceStatusTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Interface Status TLVs in CCMs generated by this "
                               "switch. Default is DEFER, which means: Let the MD configuration apply.")
    );

    m.add_leaf(
        s.organization_specific_tlv_option,
        vtss::tag::Name("OrgSpecificTlv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Choose whether to send Organization-Specific TLV in CFM PDUs generated by "
                               "this switch. Default is DEFER, which means: Let the MD configuration apply.")
    );

}

template<typename T>
void serialize(T &a, vtss_appl_cfm_mep_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_mep_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.direction,
        vtss::tag::Name("Direction"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Determines whether this is an Up- or a Down-MEP. Notice, that not all platforms support Up-MEPs "
                               "(see capabilities HasUpMeps). ")
    );

    m.add_leaf(
        vtss::AsInterfaceIndex(s.ifindex),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Port on which this MEP resides.")
    );

    m.add_leaf(
        s.vlan,
        vtss::tag::Name("Vid"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("VLAN ID. Use the value 0 to indicate untagged traffic (implies a port MEP).")
    );

    m.add_leaf(
        s.pcp,
        vtss::tag::Name("Pcp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("PCP (priority) (default 7). The PCP value used in the VLAN tag unless the MEP is untagged. "
                               "Must be a value in range [0; 7]. ")
    );

    m.add_leaf(
        s.smac,
        vtss::tag::Name("Smac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Source MAC address used in all PDUs originating at this MEP. "
                               "Must be a unicast address. If all-zeros, the switch port's MAC address will be used instead.")
    );

    m.add_leaf(
        s.alarm_level,
        vtss::tag::Name("AlarmLevel"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The lowest priority defect that is allowed to generate a fault alarm. "
                               "See 802.1Q-2018, clause 20.9.5, LowestAlarmPri. "
                               "Valid range is [1; 6] with 1 indicating that any defect will cause a fault alarm "
                               "and 6 indicating that no defect can cause a fault alarm.\n"
                               "It follows the CFM MIB's definition closely: \n"
                               " 1: DefRDICCM, DefMACstatus, DefRemoteCCM, DefErrorCCM, DefXconCCM\n"
                               " 2:            DefMACstatus, DefRemoteCCM, DefErrorCCM, DefXconCCM\n"
                               " 3:                          DefRemoteCCM, DefErrorCCM, DefXconCCM\n"
                               " 4:                                        DefErrorCCM, DefXconCCM\n"
                               " 5:                                                     DefXconCCM\n"
                               " 6: No defects are to be reported\n"
                               "Default is 2.")
    );

    m.add_leaf(
        s.alarm_time_present_ms,
        vtss::tag::Name("AlarmPresentMs"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The time that defects must be present before a fault alarm is issued. "
                               "See 802.1Q-2018, clause 20.33.3 (fngAlarmTime). Valid values are in range "
                               "2500-10000 milliseconds (the MIB specifies it in 0.01s intervals, which gives a range of 250-1000)."
                               "Default is 2500 milliseconds.")
    );

    m.add_leaf(
        s.alarm_time_absent_ms,
        vtss::tag::Name("AlarmAbsentMs"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The time that defects must be absent before a fault alarm is cleared. "
                               "See 802.1Q-2018, clause 20.33.4 (fngResentTime). Valid values are in range "
                               "2500-10000 milliseconds (the MIB specifies it in 0.01s intervals, which gives a range of 250-1000. "
                               "Default is 10000 milliseconds.")
    );

    m.add_leaf(
        vtss::AsBool(s.ccm_enable),
        vtss::tag::Name("CcmEnable"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Start or stop generation of CCMs (default false). "
                               "Actual generation will only be started if both this member is true AND "
                               "CcmInterval is not INVALID.")
    );

    m.add_leaf(
        vtss::AsBool(s.admin_active),
        vtss::tag::Name("AdminActive"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative state of this MEP (default false). "
                               "Set to true to make it function normally and false to make it cease functioning. "
                               "When false, the MEP is torn down, so it will not respond to CFM PDUs requiring a response, "
                               "and it will not generate CFM PDUs.")
    );
}

struct vtss_appl_cfm_mep_conf_wrap_t {
    /**
     * The original MEP configuration
     */
    vtss_appl_cfm_mep_conf_t mep;
    /**
     * Remote MEPID
     */
    vtss_appl_cfm_mepid_t rmepid;
};

template<typename T>
void serialize(T &a, vtss_appl_cfm_mep_conf_wrap_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_mep_conf_wrap_t"));
    int ix = 0;

    m.add_leaf(
        s.mep.direction,
        vtss::tag::Name("Direction"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Determines whether this is an Up- or a Down-MEP. Notice, that not all platforms support Up-MEPs "
                               "(see capabilities HasUpMeps). ")
    );

    m.add_leaf(
        vtss::AsInterfaceIndex(s.mep.ifindex),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Port on which this MEP resides.")
    );

    m.add_leaf(
        s.mep.vlan,
        vtss::tag::Name("Vid"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("VLAN ID. Use the value 0 to indicate untagged traffic (implies a port MEP).")
    );

    m.add_leaf(
        s.mep.pcp,
        vtss::tag::Name("Pcp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("PCP (priority) (default 7). The PCP value used in the VLAN tag unless the MEP is untagged. "
                               "Must be a value in range [0; 7]. ")
    );

    m.add_leaf(
        s.mep.smac,
        vtss::tag::Name("Smac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Source MAC address used in all PDUs originating at this MEP. "
                               "Must be a unicast address. If all-zeros, the switch port's MAC address will be used instead.")
    );

    m.add_leaf(
        s.mep.alarm_level,
        vtss::tag::Name("AlarmLevel"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The lowest priority defect that is allowed to generate a fault alarm. "
                               "See 802.1Q-2018, clause 20.9.5, LowestAlarmPri. "
                               "Valid range is [1; 6] with 1 indicating that any defect will cause a fault alarm "
                               "and 6 indicating that no defect can cause a fault alarm.\n"
                               "It follows the CFM MIB's definition closely: \n"
                               " 1: DefRDICCM, DefMACstatus, DefRemoteCCM, DefErrorCCM, DefXconCCM\n"
                               " 2:            DefMACstatus, DefRemoteCCM, DefErrorCCM, DefXconCCM\n"
                               " 3:                          DefRemoteCCM, DefErrorCCM, DefXconCCM\n"
                               " 4:                                        DefErrorCCM, DefXconCCM\n"
                               " 5:                                                     DefXconCCM\n"
                               " 6: No defects are to be reported\n"
                               "Default is 2.")
    );

    m.add_leaf(
        s.mep.alarm_time_present_ms,
        vtss::tag::Name("AlarmPresentMs"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The time that defects must be present before a fault alarm is issued. "
                               "See 802.1Q-2018, clause 20.33.3 (fngAlarmTime). Valid values are in range "
                               "2500-10000 milliseconds (the MIB specifies it in 0.01s intervals, which gives a range of 250-1000)."
                               "Default is 2500 milliseconds.")
    );

    m.add_leaf(
        s.mep.alarm_time_absent_ms,
        vtss::tag::Name("AlarmAbsentMs"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The time that defects must be absent before a fault alarm is cleared. "
                               "See 802.1Q-2018, clause 20.33.4 (fngResentTime). Valid values are in range "
                               "2500-10000 milliseconds (the MIB specifies it in 0.01s intervals, which gives a range of 250-1000. "
                               "Default is 10000 milliseconds.")
    );

    m.add_leaf(
        vtss::AsBool(s.mep.ccm_enable),
        vtss::tag::Name("CcmEnable"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Start or stop generation of CCMs (default false). "
                               "Actual generation will only be started if both this member is true AND "
                               "CcmInterval is not INVALID.")
    );

    m.add_leaf(
        vtss::AsBool(s.mep.admin_active),
        vtss::tag::Name("AdminActive"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The administrative state of this MEP (default false). "
                               "Set to true to make it function normally and false to make it cease functioning. "
                               "When false, the MEP is torn down, so it will not respond to CFM PDUs requiring a response, "
                               "and it will not generate CFM PDUs.")
    );

    m.add_leaf(
        s.rmepid,
        vtss::tag::Name("RmepId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This is the Remote mep Id.")
    );

}


template<typename T>
void serialize(T &a, vtss_appl_cfm_rmep_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_rmep_conf_t"));
    int ix = 0;

    m.add_leaf(
        s.unused,
        vtss::tag::Name("unused"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This structure defines a remote MEP. "
                               "In the functions using this structure, it is indexed by the remote MEP's "
                               "MEPID, which must be an integer in range [1; 8191] and must not be the same "
                               "as other MEPs in this MA's MEPID.")
    );
}


template<typename T>
void serialize(T &a, vtss_appl_cfm_mep_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_mep_status_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mep_active),
        vtss::tag::Name("MepActive"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Operational state of the MEP. If it is adminstratively up and successfully created, this member is true, false otherwise.")
    );

    m.add_leaf(
        s.fng_state,
        vtss::tag::Name("FngState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Holds the current state of the Fault Notification Generator State Machine.")
    );

    m.add_leaf(
        s.smac,
        vtss::tag::Name("Smac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This MEP's MAC address.")
    );

    m.add_leaf(
        s.highest_defect,
        vtss::tag::Name("HighestDefect"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Highest priority defect that has been present since the MEP's fault "
                               "notification generator state machine was last in the FNG_RESET state.")
    );

    m.add_leaf(
        s.defects,
        vtss::tag::Name("Defects"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("A MEP can detect and report a number of defects, and multiple defects "
                               "can be present at the same time. This is a mask of defects, where: \n"
                               "+-----------------------------+\n"
                               "| Bit # | Description         |\n"
                               "|-------|---------------------|\n"
                               "|     0 | someRDIdefect       |\n"
                               "|     1 | someMACstatusDefect |\n"
                               "|     2 | someRMEPCCMdefect   |\n"
                               "|     3 | errorCCMdefect      |\n"
                               "|     4 | xconCCMdefect       |\n"
                               "+-----------------------------+")
    );

    m.add_leaf(
        vtss::AsBool(s.present_rdi),
        vtss::tag::Name("PresentRDI"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates whether this MEP sends CCMs with the RDI bit set.")
    );

    m.add_leaf(
        s.ccm_rx_valid_cnt,
        vtss::tag::Name("CcmRxValidCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Total number of CCMs that hit this MEP and passed the validation test.")
    );

    m.add_leaf(
        s.ccm_rx_invalid_cnt,
        vtss::tag::Name("CcmRxInvalidCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Total number of CCMs that hit this MEP and didn't pass the validation test.")
    );

    m.add_leaf(
        s.ccm_rx_sequence_error_cnt,
        vtss::tag::Name("CcmRxSequenceErrorCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Total number of out-of-sequence errors seen from RMEPs.")
    );

    m.add_leaf(
        s.ccm_tx_cnt,
        vtss::tag::Name("CcmTxCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Total number of CCM PDUs transmitted by this MEP.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.mep_creatable),
        vtss::tag::Name("MepCreatable"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, the configuration conditions for creating a MEP are fulfilled.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.enableRmepDefect),
        vtss::tag::Name("EnableRmepDefect"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, data can pass into the port managed by this MEP, that is, it is not blocked by (M)STP or VLAN configuration errors.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.no_rmeps),
        vtss::tag::Name("ErrnoRmeps"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, the MEP doesn't have any RMEPs.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.port_up_mep),
        vtss::tag::Name("ErrPortUpMep"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Port MEPs cannot be configured as Up-MEPs.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.multiple_rmeps_ccm_interval),
        vtss::tag::Name("ErrRmepsCcmInterval"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, the MEP has more than one RMEP, but the MA's CCM interval is faster than 1 sec.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.hw_resources),
        vtss::tag::Name("ErrHwResources"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, a H/W resource allocation failed while attempting to create the MEP.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.internal_error),
        vtss::tag::Name("ErrInternalError"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, an internal error has occurred while attempting to create the MEP.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.is_mirror_port),
        vtss::tag::Name("ErrIsMirrorPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, the MEP's residence port is used as a mirror destination port.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.is_npi_port),
        vtss::tag::Name("ErrIsNniPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(" If this member is true, the MEP's residence port is configured as an NPI port and is therefore not suitable for MEPs.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.no_link),
        vtss::tag::Name("ErrNoLink"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, the MEP's residence port has no link.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.vlan_unaware),
        vtss::tag::Name("ErrVlanUnaware"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, the MEP's residence port is VLAN unaware, but the MEP expects tagged PDUs.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.vlan_membership),
        vtss::tag::Name("ErrVlanMembership"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Ingress filtering is enabled on the MEP's residence port, which doesn't have the MEP's expected classified VLAN ID in its memberset of VLANs.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.stp_blocked),
        vtss::tag::Name("ErrStpBlocked"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, the MEP's residence port is not configured as forwarding by the Spanning Tree protocol.")
    );

    m.add_leaf(
        vtss::AsBool(s.errors.mstp_blocked),
        vtss::tag::Name("ErrMstpBlocked"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If this member is true, the MEP's residence port and VLAN is not configured as forwarding by the MSTP protocol.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_cfm_rmep_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_rmep_status_t"));
    int ix = 0;

    m.add_leaf(
        s.state,
        vtss::tag::Name("State"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(" Indication of the operational state of this RMEP's Remote MEP state machine.")
    );

    m.add_leaf(
        s.failed_ok_time,
        vtss::tag::Name("FailedokTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The time in seconds (Sysuptime) at which the RMEP last entered "
                               "RMEP_FAILED or RMEP_OK state. 0 if it hasn't yet entered any of these states.")
    );

    m.add_leaf(
        s.smac,
        vtss::tag::Name("Smac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Source MAC address of last received CCM from this RMEP or all-zeros if "
                               "no MAC address was received.")
    );

    m.add_leaf(
        vtss::AsBool(s.rdi),
        vtss::tag::Name("Rdi"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("RDI bit contained in last CCM received from this RMEP.")
    );

    m.add_leaf(
        s.port_status,
        vtss::tag::Name("PortStatus"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Contents of the Port Status TLV of the last received CCM from this RMEP.")
    );

    m.add_leaf(
        s.interface_status,
        vtss::tag::Name("InterfaceStatus"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(" Contents of the Interface Status TLV (21.5.5) of the last received CCM from this RMEP.")
    );

    m.add_leaf(
        s.sender_id.chassis_id_subtype,
        vtss::tag::Name("SenderIdChassisIdSubtype"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Contains the Sender ID TLV's Chassis ID Subtype of the last "
                               "received CCM from this RMEP.")
    );

    m.add_leaf(
        s.sender_id.chassis_id_len,
        vtss::tag::Name("SenderIdChassisIdLen"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Length of the SenderIdChassisId field. If 0, either the sender ID TLV is not present in the last CCM received "
                               "from this RMEP or the Chassis ID is not present in the TLV.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.sender_id.chassis_id, sizeof(s.sender_id.chassis_id)),
        vtss::tag::Name("SenderIdChassisId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Contains the Sender ID TLV's Chassis ID. It is a NULL-terminated string "
                               "which may be 0 bytes long if the sender ID TLV is not present in the "
                               "last CCM received from this RMEP or the Chassis ID is not present in the TLV.")
    );

    m.add_leaf(
        s.sender_id.mgmt_addr_domain_len,
        vtss::tag::Name("SenderIdMgmtAddrDomainLen"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Length of the SenderIdMgmtAddrDomain field. If 0, either the sender ID TLV is not present "
                               "in the last CCM received from this RMEP or the Management Address Domain field not present "
                               "in the TLV. If > sizeof(SenderIdMgmtAddrDomain), the contents was truncated to  sizeof(SenderIdMgmtAddrDomain) "
                               "received from this RMEP.")
    );

    m.add_leaf(
        vtss::AsOctetString(s.sender_id.mgmt_addr_domain, sizeof(s.sender_id.mgmt_addr_domain)),
        vtss::tag::Name("SenderIdMgmtAddrDomain"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Contains the Sender ID TLV's Management Address Domain from the last CCM "
                               "received from this RMEP.")
    );

    m.add_leaf(
        vtss::AsOctetString(s.sender_id.mgmt_addr, sizeof(s.sender_id.mgmt_addr)),
        vtss::tag::Name("SenderIdMgmtAddr"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Contains the Sender ID TLV's Management Address. The number of valid "
                               "bytes valid bytes in this array is implicitly given by the OID encoded in "
                               "SenderIdTlvAddrDomain.")
    );

    m.add_leaf(
        vtss::AsBool(s.organization_specific_tlv_present),
        vtss::tag::Name("OrgSpecificTlvPresent"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(" If the last received CCM from this RMEP contains an Organization-Specific "
                               "this field is true, and  Organization-Specific TLV contains valid data.")
    );

    m.add_leaf(
        vtss::AsOctetString(s.organization_specific_tlv.oui, sizeof(s.organization_specific_tlv.oui)),
        vtss::tag::Name("OrgSpecificTlvOui"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This is the three-bytes OUI transmitted or received with the "
                               "Organization-Specific TLV.")
    );

    m.add_leaf(
        s.organization_specific_tlv.subtype,
        vtss::tag::Name("OrgSpecificTlvSubtype"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This is the subtype transmitted or received with the Organization-Specific TLV. "
                               "Can be any value in range [0; 255].")
    );

    m.add_leaf(
        s.organization_specific_tlv.value_len,
        vtss::tag::Name("OrgSpecificTlvValueLen"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The length of the value transmitted in the Organization-Specific TLV.")
    );

    m.add_leaf(
        vtss::AsOctetString(s.organization_specific_tlv.value, sizeof(s.organization_specific_tlv.value)),
        vtss::tag::Name("OrgSpecificTlvValue"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The value transmitted in the Organization-Specific TLV.")
    );

}

template<typename T>
void serialize(T &a, vtss_appl_cfm_mep_notification_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_cfm_mep_notification_status_t"));
    int ix = 0;

    m.add_leaf(
        s.highest_defect,
        vtss::tag::Name("HighestDefect"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If a defect with a priority higher than the configured alarm level "
                               "is detected, this object will be set and generate a notification after the time indicated in "
                               "AlarmPresentMs has expired. The alarm will be cleared after no defect higher than the configured "
                               "alarm level has been absent for AlarmAbsentMs.")
    );

}


namespace vtss
{
namespace appl
{
namespace cfm
{
namespace interfaces
{

struct cfmConfigGlobalsLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_cfm_global_conf_t *>
    > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_global_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_cfm_global_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_CFM);
};

struct cfmCapabilitiesLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_cfm_capabilities_t *>
    > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_CFM);
};

mesa_rc vtss_appl_cfm_md_conf_default_get_wrap(vtss_appl_cfm_md_key_t *key, vtss_appl_cfm_md_conf_t *conf);

struct cfmConfigMdEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_cfm_md_key_t &>,
         vtss::expose::ParamVal<vtss_appl_cfm_md_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 10000;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    static constexpr const char *table_description =
        "This table contains the configuration of cfm Maintenance Domains.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_md_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_cfm_md_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_md_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_cfm_md_conf_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_cfm_md_itr);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_cfm_md_conf_del);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_cfm_md_conf_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_cfm_md_conf_default_get_wrap);

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_CFM);
};

mesa_rc vtss_appl_cfm_ma_itr_wrap(const vtss_appl_cfm_ma_key_t *prev_key, vtss_appl_cfm_ma_key_t *next_key);
mesa_rc vtss_appl_cfm_ma_conf_default_get_wrap(vtss_appl_cfm_ma_key_t *key, vtss_appl_cfm_ma_conf_t *conf);

struct cfmConfigMaEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_cfm_ma_key_t &>,
         vtss::expose::ParamVal<vtss_appl_cfm_ma_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 10000;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    static constexpr const char *table_description =
        "This table contains the configuration of cfm Maintenance Associations.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_ma_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_cfm_ma_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_ma_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_cfm_ma_conf_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_cfm_ma_itr_wrap);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_cfm_ma_conf_del);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_cfm_ma_conf_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_cfm_ma_conf_default_get_wrap);

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_CFM);
};

mesa_rc vtss_appl_cfm_mep_itr_wrap(const vtss_appl_cfm_mep_key_t *prev_key, vtss_appl_cfm_mep_key_t *next_key);
mesa_rc vtss_appl_cfm_mep_conf_default_get_wrap(vtss_appl_cfm_mep_key_t *key, vtss_appl_cfm_mep_conf_t *conf);

struct cfmConfigMepEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_cfm_mep_key_t &>,
         vtss::expose::ParamVal<vtss_appl_cfm_mep_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 10000;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    static constexpr const char *table_description =
        "This table contains the configuration of cfm MEPS.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_mep_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_cfm_mep_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_mep_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_cfm_mep_conf_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_cfm_mep_itr_wrap);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_cfm_mep_conf_del);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_cfm_mep_conf_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_cfm_mep_conf_default_get_wrap);

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_CFM);
};

mesa_rc vtss_appl_cfm_mep_conf_get_wrap(const vtss_appl_cfm_mep_key_t &key, vtss_appl_cfm_mep_conf_wrap_t *conf);
mesa_rc vtss_appl_cfm_mep_conf_set_wrap(const vtss_appl_cfm_mep_key_t &key, const vtss_appl_cfm_mep_conf_wrap_t *conf);
mesa_rc vtss_appl_cfm_mep_conf_wrap_default_get_wrap(vtss_appl_cfm_mep_key_t *key, vtss_appl_cfm_mep_conf_wrap_t *conf);

struct cfmConfigMepWrapEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_cfm_mep_key_t &>,
         vtss::expose::ParamVal<vtss_appl_cfm_mep_conf_wrap_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 10000;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    static constexpr const char *table_description =
        "This table contains the configuration of cfm MEPS.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_mep_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_cfm_mep_conf_wrap_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_mep_conf_get_wrap);
    VTSS_EXPOSE_SET_PTR(vtss_appl_cfm_mep_conf_set_wrap);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_cfm_mep_itr_wrap);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_cfm_mep_conf_del);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_cfm_mep_conf_set_wrap);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_cfm_mep_conf_wrap_default_get_wrap);

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_CFM);
};

mesa_rc vtss_appl_cfm_rmep_itr_wrap(const vtss_appl_cfm_rmep_key_t *prev_key, vtss_appl_cfm_rmep_key_t *next_key);
mesa_rc vtss_appl_cfm_rmep_conf_default_get_wrap(vtss_appl_cfm_rmep_key_t *key, vtss_appl_cfm_rmep_conf_t *conf);

struct cfmConfigRMepEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_cfm_rmep_key_t &>,
         vtss::expose::ParamVal<vtss_appl_cfm_rmep_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 10000;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    static constexpr const char *table_description =
        "This table contains the configuration of cfm Remote MEPS.";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_rmep_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_cfm_rmep_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }
    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_rmep_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_cfm_rmep_conf_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_cfm_rmep_itr_wrap);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_cfm_rmep_conf_del);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_cfm_rmep_conf_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_cfm_rmep_conf_default_get_wrap);

    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_CFM);
};

// //----------------------------------------------------------------------------
// // Status
// //----------------------------------------------------------------------------

struct cfmStatusMepEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_cfm_mep_key_t &>,
         vtss::expose::ParamVal<vtss_appl_cfm_mep_status_t *>
         > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    static constexpr const char *table_description =
        "This table contains the status of cfm MEPS.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_mep_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_cfm_mep_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_mep_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_cfm_mep_itr_wrap);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_CFM);
};

struct cfmStatusRMepEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_cfm_rmep_key_t &>,
         vtss::expose::ParamVal<vtss_appl_cfm_rmep_status_t *>
         > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    static constexpr const char *table_description =
        "This table contains the configuration of cfm Remote MEPS.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_rmep_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_cfm_rmep_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }
    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_rmep_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_cfm_rmep_itr_wrap);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_CFM);
};

struct cfmStatusNotificationEntry {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_cfm_mep_key_t &>,
         vtss::expose::ParamVal<vtss_appl_cfm_mep_notification_status_t *>
         > P;

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    static constexpr const char *table_description =
        "This table contains the status of cfm notifications.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_cfm_mep_key_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_cfm_mep_notification_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_cfm_mep_notification_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_cfm_mep_itr_wrap);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_CFM);
};

}  // namespace interfaces
}  // namespace cfm
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_CFM_SERIALIZER_HXX__ */
