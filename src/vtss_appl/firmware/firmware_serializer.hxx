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
#ifndef __VTSS_FIRMWARE_SERIALIZER_HXX__
#define __VTSS_FIRMWARE_SERIALIZER_HXX__

#include "vtss_remote_file_transfer.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/firmware.h"
#include "firmware_mfi_info.hxx"

/*****************************************************************************
    Enum serializer
*****************************************************************************/

extern vtss_enum_descriptor_t firmware_status_image_txt[];

VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_firmware_status_image_type_t,
    "FirmwareStatusImageEnum",
    firmware_status_image_txt,
    "This enumeration defines the type of image for status.");

extern vtss_enum_descriptor_t firmware_upload_image_txt[];
VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_firmware_upload_image_type_t,
    "FirmwareUploadImageEnum",
    firmware_upload_image_txt,
    "This enumeration defines the type of image to upload.");

extern vtss_enum_descriptor_t firmware_upload_status_txt[];
VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_firmware_upload_status_t,
    "FirmwareUploadStatusEnum",
    firmware_upload_status_txt,
    "This enumeration defines the status of upload operation.");

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(firmware_image_tlv_image_id_index, u32, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("ImageId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The image Id. Starts from 0.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(firmware_image_tlv_section_id_index, u32, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("SectionId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The section Id. Starts from 0.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(firmware_image_tlv_attribute_id_index, u32, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("AttributeId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The attribute Id. Starts from 0.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(firmware_image_number_index, u32, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("Number"),
        vtss::expose::snmp::RangeSpec<uint32_t>(0, 2),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The number of image. The number starts from 0.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(firmware_switch_id_index, u32, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("SwitchId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(1, 16),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The ID of switch.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/

template<typename T>
void serialize(T &a, vtss_appl_firmware_status_image_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_firmware_status_image_t"));
    int ix = 0;

    m.add_leaf(
        s.type,
        vtss::tag::Name("Type"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Image type of the status. "
            "bootloader(0) is for boot loader. "
            "activeFirmware(1) is for active (primary) firmware. "
            "alternativeFirmware(2) is for alternative (backup) firmware.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.name, sizeof(s.name)),
        vtss::tag::Name("Name"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Image name.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.version, sizeof(s.version)),
        vtss::tag::Name("Version"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Image version.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.built_date, sizeof(s.built_date)),
        vtss::tag::Name("BuiltDate"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The built date when the image is built.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.code_revision, sizeof(s.code_revision)),
        vtss::tag::Name("CodeRevision"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The code revesion which the image is built.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_firmware_status_image_upload_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_firmware_status_image_upload_t"));
    int ix = 0;

    m.add_leaf(
        s.status,
        vtss::tag::Name("Status"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The status indicates the status of current upload operation. "
            "It is updated automatically. Modifying this flag does not take any effect. "
            "none(0) means no upload operation. "
            "success(1) means upload operation is successful. "
            "inProgress(2) means current upload operation is in progress. "
            "errIvalidIp(3) means upload operation is failed due to invalid IP address. "
            "errTftpFailed(4) means upload operation is failed due to failed TFTP operation. "
            "errBusy(5) means upload operation is failed due to other upload in processing. "
            "errMemoryInsufficient(6) means upload operation is failed due to memory insufficient. "
            "errInvalidImage(7) means upload operation is failed due to invalid image. "
            "errWriteFlash(8) means upload operation is failed due to failed writing flash. "
            "errSameImageExisted(9) means upload operation is failed because the upload image is the same as the one in flash. "
            "errUnknownImage(10) means upload operation is failed because the type of upload image is unknown. "
            "errFlashImageNotFound(11) means upload operation is failed because the location in flash to upload the image is not found. "
            "errFlashEntryNotFound(12) means upload operation is failed because the corresponding entry in flash to upload the image is not found. "
            "errCrc(13) means upload operation is failed due to incorrect CRC in the upload image. "
            "errImageSize(14) means upload operation is failed due to invalid image size. "
            "errEraseFlash(15) means upload operation is failed due to failed erasing flash. "
            "errIncorrectImageVersion(16) means upload operation is failed due to incorrect version of the upload image. "
            "errDownloadUrl(17) means upload operation is failed due to fail to download image from URL. "
            "errInvalidUrl(18) means upload operation is failed due to invalid URL. "
            "errInvalidFilename(19) means upload operation is failed due to invalid filename of the upload image. "
            "errInvalidPath(20) means upload operation is failed due to invalid path of the upload image. ")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_firmware_status_switch_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_firmware_status_switch_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.chip_id, sizeof(s.chip_id)),
        vtss::tag::Name("ChipId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("ID of chip.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.board_type, sizeof(s.board_type)),
        vtss::tag::Name("BoardType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of board.")
    );

    m.add_leaf(
        s.port_cnt,
        vtss::tag::Name("PortCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Count of ports.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.product, sizeof(s.product)),
        vtss::tag::Name("Product"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Product name.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.version, sizeof(s.version)),
        vtss::tag::Name("Version"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Image version.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.built_date, sizeof(s.built_date)),
        vtss::tag::Name("BuiltDate"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The built date when the image is built.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_firmware_control_globals_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_firmware_control_globals_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.swap_firmware),
        vtss::tag::Name("SwapFirmware"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Swap firmware between active (primary) and alternative (backup). "
            "true is to swap the firmware. "
            "false is to do nothing.")
    );
}

template <typename T>
void serialize(T &a, vtss_appl_firmware_image_status_tlv_t &s) {
    int ix = 0;
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_firmware_image_status_tlv_t"));

    m.add_leaf(vtss::AsDisplayString(s.attr_name, sizeof(s.attr_name)),
               vtss::tag::Name("Name"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Attribute name."));

    switch (s.attr_type) {
    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TLV_TYPE_STR:
        m.add_leaf(vtss::AsDisplayString(s.value.str, sizeof(s.value.str)),
                   vtss::tag::Name("Value"), vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Attribute value."));
        break;
    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TLV_TYPE_INT:
        m.add_leaf(s.value.number,
                   vtss::tag::Name("Value"), vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Attribute value."));
        break;
    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TLV_TYPE_UINT:
        m.add_leaf(s.value.unumber,
                   vtss::tag::Name("Value"), vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Attribute value."));
        break;
    }
}

template<typename T>
void serialize(T &a, vtss_appl_firmware_control_image_upload_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_firmware_control_image_upload_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.upload),
        vtss::tag::Name("DoUpload"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Action to do upload image or not. "
            "true is to do the upload operation. "
            "false is to do nothing. "
            "The upload operation may need longer time to upload the image, "
            "so the SNMP timeout time needs to be modified accordingly.")
    );

    m.add_leaf(
        s.type,
        vtss::tag::Name("ImageType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Type of image to upload. "
            "bootloader(0) is to upload bootloader. "
            "firmware(1) is to upload application firmware.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.url, sizeof(s.url)),
        vtss::tag::Name("Url"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The location of image to upload. It is a specific character string that constitutes a reference to a resource. Syntax: <protocol>://[<username>[:<password>]@]<host>[:<port>][/<path>]/<file_name> For example, tftp://10.10.10.10/new_image_path/new_image.dat, http://username:password@10.10.10.10:80/new_image_path/new_image.dat. A valid file name is a text string drawn from alphabet (A-Za-z), digits (0-9), dot (.), hyphen (-), under score(_). The maximum length is 63 and hyphen must not be first character. The file name content that only contains '.' is not allowed.")
    );

    m.add_leaf(
        vtss::AsBool(s.ssh_save_host_keys),
        vtss::tag::Name("SaveSshHostKeys"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Set to true to save new SSH host keys in local cache. ")
    );

    m.add_leaf(
        vtss::AsBool(s.ftp_active),
        vtss::tag::Name("FtpActiveMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Set to true to force FTP session to use active mode (default is passive mode). ")
    );
}

namespace vtss {
namespace appl {
namespace firmware {
namespace interfaces {

struct FirmwareStatusImageEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_firmware_status_image_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of status of images in flash.";

    static constexpr const char *index_description =
        "Each entry has a set of image status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, firmware_image_number_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_firmware_status_image_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_firmware_status_image_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_firmware_status_image_entry_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FIRMWARE);
};

struct FirmwareStatusImageUploadLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_firmware_status_image_upload_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_firmware_status_image_upload_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_firmware_status_image_upload_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FIRMWARE);
};

struct FirmwareStatusSwitchEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_firmware_status_switch_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of status of images in switch.";

    static constexpr const char *index_description =
        "Each entry has a set of image status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, firmware_switch_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_firmware_status_switch_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_firmware_status_switch_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_firmware_status_switch_entry_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FIRMWARE);
};

struct FirmwareControlGlobalsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_firmware_control_globals_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_firmware_control_globals_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_firmware_control_globals_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_firmware_control_globals_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FIRMWARE);
};

struct FirmwareControlCopyConfigLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_firmware_control_image_upload_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_firmware_control_image_upload_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_firmware_control_image_upload_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_firmware_control_image_upload_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_FIRMWARE);
};

struct FirmwareImageStatusTlvLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_firmware_image_status_tlv_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, firmware_image_tlv_image_id_index(i));
    }


    VTSS_EXPOSE_SERIALIZE_ARG_2(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, firmware_image_tlv_section_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, firmware_image_tlv_attribute_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_firmware_image_status_tlv_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(4));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_firmware_status_image_tlv_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_firmware_status_image_tlv_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_FIRMWARE);
};

}  // namespace interfaces
}  // namespace firmware
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_FIRMWARE_SERIALIZER_HXX__ */
