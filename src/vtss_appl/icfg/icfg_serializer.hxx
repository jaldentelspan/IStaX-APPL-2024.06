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
#ifndef __VTSS_ICFG_SERIALIZER_HXX__
#define __VTSS_ICFG_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/icfg.h"

/*****************************************************************************
    Enum serializer
*****************************************************************************/
extern vtss_enum_descriptor_t icfg_reload_default_txt[];
extern vtss_enum_descriptor_t icfg_config_type_txt[];
extern vtss_enum_descriptor_t icfg_config_status_txt[];

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_icfg_reload_default_t,
    "IcfgReloadDefault",
    icfg_reload_default_txt,
    "This enumeration defines the type of reload default.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_icfg_config_type_t,
    "IcfgConfigType",
    icfg_config_type_txt,
    "This enumeration defines the type of configuration.");

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_icfg_copy_status_t,
    "IcfgConfigStatus",
    icfg_config_status_txt,
    "This enumeration defines the type of configuration.");

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(icfg_file_no_index, u32, a, s ) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("FileNo"),
        vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The number of File. The number starts from 1.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_icfg_file_statistics_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_icfg_file_statistics_t"));
    int ix = 0;

    m.add_leaf(
        s.numberOfFiles,
        vtss::tag::Name("NumberOfFiles"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of files in flash.")
    );

    m.add_leaf(
        s.totalBytes,
        vtss::tag::Name("TotalBytes"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Total number of bytes used by all files in flash.")
    );

    m.add_leaf(
        s.flashSize,
        vtss::tag::Name("FlashSizeBytes"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Flash file system size in bytes.")
    );

    m.add_leaf(
        s.flashFree,
        vtss::tag::Name("FlashFreeBytes"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Flash file system number of free bytes.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_icfg_file_entry_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_icfg_file_entry_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.fileName, sizeof(s.fileName)),
        vtss::tag::Name("FileName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("File name.")
    );

    m.add_leaf(
        s.bytes,
        vtss::tag::Name("Bytes"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of bytes of the file.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.modifiedTime, sizeof(s.modifiedTime)),
        vtss::tag::Name("ModifiedTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Last modified time of the file.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.attribute, sizeof(s.attribute)),
        vtss::tag::Name("Attribute"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("File attribute in the format of 'rw'. "
            "'r' presents readable while 'w' presents writable.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_icfg_status_copy_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_icfg_status_copy_config_t"));
    int ix = 0;

    m.add_leaf(
        s.status,
        vtss::tag::Name("Status"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The status indicates the status of current copy operation. "
            "none(0) means no copy operation. "
            "success(1) means copy operation is successful. "
            "inProgress(2) means current copy operation is in progress. "
            "errOtherInProcessing(3) means copy operation is failed due to other in processing. "
            "errNoSuchFile(4) means copy operation is failed due to file not existing. "
            "errSameSrcDst(5) means copy operation is failed due to the source and destination are the same. "
            "errPermissionDenied(6) means copy operation is failed due to the destination is not permitted to modify. "
            "errLoadSrc(7) means copy operation is failed due to the error to load source file. "
            "errSaveDst(8) means copy operation is failed due to the error to save or commit destination.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_icfg_control_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_icfg_control_t"));
    int ix = 0;

    m.add_leaf(
        s.reloadDefault,
        vtss::tag::Name("ReloadDefault"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Reset system to default. "
            "none(0) is to do nothing. "
            "default(1) is to reset the whole system to default. "
            "defaultKeepIp(2) is to reset system to default, but keep IP address of VLAN 1.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.deleteFile, sizeof(s.deleteFile)),
        vtss::tag::Name("DeleteFile"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Delete file in flash. The format is flash:filename. "
            "Where 'default-config' is read-only and not allowed to be deleted.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_icfg_copy_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_icfg_copy_config_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.copy),
        vtss::tag::Name("Copy"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Action to do copy or not. "
            "true is to do the copy operation. false is to do nothing")
    );

    m.add_leaf(
        s.sourceConfigType,
        vtss::tag::Name("SourceConfigType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Source configuration type. "
            "none(0) means no configuration file. "
            "runningConfig(1) means running configuration. "
            "startupConfig(2) means startup configuration file in flash. "
            "configFile(3) is the configuration file specified in SourceConfigFile.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.sourceConfigFile, sizeof(s.sourceConfigFile)),
        vtss::tag::Name("SourceConfigFile"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Source configuration file. If the configuration file is in flash then the format is flash:'filename'. "
            "If the configuration file is from tftp then the format is tftp://server[:port]/path-to-file.")
    );

    m.add_leaf(
        s.destinationConfigType,
        vtss::tag::Name("DestinationConfigType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Source configuration type. "
            "none(0) means no configuration file. "
            "runningConfig(1) means running configuration. "
            "startupConfig(2) means startup configuration file in flash. "
            "configFile(3) is the configuration file specified in DestinationConfigFile.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.destinationConfigFile, sizeof(s.destinationConfigFile)),
        vtss::tag::Name("DestinationConfigFile"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Destination configuration file. If the configuration file is in flash then the format is flash:filename. "
            "If the configuration file is from tftp then the format is tftp://server[:port]/filename_with_path. "
            "Where 'default-config' is read-only and not allowed to be deleted.")
    );

    m.add_leaf(
        vtss::AsBool(s.merge),
        vtss::tag::Name("Merge"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("This flag works only if DestinationConfigType is runningConfig(1). "
            "true is to merge the source configuration into the current running configuration. "
            "false is to replace the current running configuration with the source configuration.")
    );
}

namespace vtss {
namespace appl {
namespace icfg {
namespace interfaces {
struct IcfgStatusFileStatisticsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_icfg_file_statistics_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_icfg_file_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_icfg_file_statistics_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MISC);
};

struct IcfgStatusFileEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_icfg_file_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of status of files in flash.";

    static constexpr const char *index_description =
        "Each entry has a set of file status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, icfg_file_no_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_icfg_file_entry_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_icfg_file_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_icfg_file_entry_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MISC);
};

struct IcfgStatusCopyConfigLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_icfg_status_copy_config_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_icfg_status_copy_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_icfg_status_copy_config_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MISC);
};


struct IcfgControlGlobalsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_icfg_control_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_icfg_control_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_icfg_control_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_icfg_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC);
};

struct IcfgControlCopyConfigLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_icfg_copy_config_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_icfg_copy_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_icfg_copy_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_icfg_copy_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC);
};
}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss


#endif /* __VTSS_ICFG_SERIALIZER_HXX__ */
