/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_LLDP_SERIALIZER_HXX__
#define __VTSS_LLDP_SERIALIZER_HXX__

#include "lldp_trace.h"          // For e.g. T_D
#include "vtss/appl/interface.h" // For PORT_ifindex_index
#include "vtss/appl/lldp.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss/appl/vlan.h"
#include "vtss/basics/trace.hxx"
#include "vtss/basics/string.hxx"
#include "vtss_common_iterator.hxx"

/****************************************************************************
 * Generic index serializers
 ****************************************************************************/

//
// For serializing Interface Index
//

struct LLDP_ifindex_index {
    LLDP_ifindex_index(vtss_ifindex_t &x) : inner(x) { }
    vtss_ifindex_t &inner;
};

template<typename T>
void serialize(T &a, LLDP_ifindex_index s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Logical interface number index."));
}

//
// For serializing policy Index
//

// This is for policy with the OidElement 1
struct LLDP_policy_index_1 {
    LLDP_policy_index_1(vtss_lldpmed_policy_index_t &x) : inner(x) { }
    vtss_lldpmed_policy_index_t &inner;
};

template<typename T>
void serialize(T &a, LLDP_policy_index_1 s) {
    vtss_appl_lldp_cap_t capabilites;
    vtss_appl_lldp_cap_get(&capabilites); // For getting the number of policies supported

    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("LldpmedPolicy"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, capabilites.policies_cnt),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Policy index."));
}

// This is for policy with the OidElement 3
struct LLDP_policy_index_3 {
    LLDP_policy_index_3(vtss_lldpmed_policy_index_t &x) : inner(x) { }
    vtss_lldpmed_policy_index_t &inner;
};

template<typename T>
void serialize(T &a, LLDP_policy_index_3 s) {
    vtss_appl_lldp_cap_t capabilites;
    vtss_appl_lldp_cap_get(&capabilites); // For getting the number of policies supported

    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("LldpmedPolicy"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, capabilites.policies_cnt),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Policy index."));
}

//
// For serializing entry table index
//

struct LLDP_entry_index {
    LLDP_entry_index(vtss_lldp_entry_index_t &x) : inner(x) { }
    vtss_lldp_entry_index_t &inner;
};

template<typename T>
void serialize(T &a, LLDP_entry_index s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("lldpmedIndex"),
               // Note: We use a fixed number in order to have the same MIB for all our platforms. The number is the number of entries that the platform with maximum number of entries can contain.
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 48 * 4),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Neighbor information table entry index."));
}

struct LLDP_catype_index {
    LLDP_catype_index(vtss_appl_lldp_med_catype_t &x) : inner(x) { }
    vtss_appl_lldp_med_catype_t &inner;
};

template<typename T>
void serialize(T &a, LLDP_catype_index s) {
    a.add_leaf(s.inner,
               vtss::tag::Name("lldpmedIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Civic address type.\
                                       \n1 - State/National subdivisions\
                                       \n2 - County, parish, gun (JP), district (IN)\
                                       \n3 - City, township\
                                       \n4 - City division, borough, city district, ward, chou (JP)\
                                       \n5 - Neighborhood, block\
                                       \n6 - Street\
                                       \n16 - Leading street direction\
                                       \n17 - Trailing street direction\
                                       \n18 - Street suffix\
                                       \n19 - House number\
                                       \n20 - House number suffix\
                                       \n21 - Landmark or vanity address\
                                       \n22 - Additional location information\
                                       \n23 - Name\
                                       \n24 - Postal/zip code\
                                       \n25 - Building\
                                       \n26 - Unit\
                                       \n27 - Floor\
                                       \n28 - Room\
                                       \n29 - Place type\
                                       \n30 - Postal\
                                       \n31 - Post office\
                                       \n32 - Additional code"));
}

//
// For serialize management address index
//
typedef u32 vtss_lldp_management_index_t;

struct LLDP_mgmt_index {
    LLDP_mgmt_index(vtss_lldp_management_index_t &x) : inner(x) { }
    vtss_lldp_management_index_t &inner;
};

template<typename T>
void serialize(T &a, LLDP_mgmt_index s) {
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("lldpManagement"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, LLDP_MGMT_ADDR_CNT -1),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("Neighbor management information table entry index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_lldpmed_policies_list_bool_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("lldpmedPoliciesList"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("Set to TRUE assign the corresponding policy index to the interface.")
              );
}


//
// Serializer for statistics clearing
//
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_lldp_statistics_clr_t, BOOL, a,
                        s) {
    a.add_leaf(vtss::AsBool(s.inner), vtss::tag::Name("StatisticsClear"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Set to TRUE to clear the LLDP statistics of an interface."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_lldp_global_statistics_clr_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Set to TRUE to clear the LLDP global statistics.")
              );
}

//****************************************
// Descriptions
//****************************************
#define LATITUDE_DESCRIPTION  "Latitude degrees in 2s-complement as specified in RFC 3825. Positive numbers are north of the \
equator and negative numbers are south of the equator."

#define LONGITUDE_DESCRIPTION "Longitude degrees in 2s-complement as specified in RFC 3825. Positive values are East of the \
prime meridian and negative numbers are West of the prime meridian."

#define ALTITUDE_DESCRIPTION  "Altitude value in 2s-complement as specified in RFC 3825"

#define ELIN_DESCRIPTION "Emergency Call Service ELIN identifier data format is defined to carry the ELIN identifier \
as used during emergency call setup to a traditional CAMA or ISDN trunk-based PSAP. \
This format consists of a numerical digit string, corresponding to the ELIN to be used \
for emergency calling. Maximum number of octets are 25."

//****************************************
// Enum  and types definitions
//****************************************
extern vtss_enum_descriptor_t lldpmed_altitude_type_txt[];
extern vtss_enum_descriptor_t lldpmed_datum_txt[];
extern vtss_enum_descriptor_t lldpmed_device_type_txt[];
extern vtss_enum_descriptor_t lldpmed_remote_device_type_txt[];
extern vtss_enum_descriptor_t lldpmed_network_policy_application_type_txt[];
extern vtss_enum_descriptor_t vtss_lldp_admin_state_txt[];
extern vtss_enum_descriptor_t lldpmed_ca_type_txt[];

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_lldp_admin_state_t,
                         "lldpAdminState",
                         vtss_lldp_admin_state_txt,
                         "This enumerations the admin state.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_lldp_med_at_type_t,
                         "lldpmedAltitudeType",
                         lldpmed_altitude_type_txt,
                         "This enumerations the altitude type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_lldp_med_datum_t,
                         "lldpmedDatumType",
                         lldpmed_datum_txt,
                         "This enumerations the datum (geodetic system).");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_lldp_med_remote_device_type_t,
                         "lldpmedRemoteDeviceType",
                         lldpmed_remote_device_type_txt,
                         "This enumerations the remote neighbor's device type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_lldp_med_application_type_t,
                         "lldpmedRemoteNetworkPolicyApplicationType",
                         lldpmed_network_policy_application_type_txt,
                         "This enumerations the remote neighbor's network policy's application type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_lldp_med_device_type_t,
                         "lldpmedDeviceType",
                         lldpmed_device_type_txt,
                         "This enumerations the device type that the device shall operate as.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_lldp_med_catype_t,
                         "lldpmedCivicAddressType",
                         lldpmed_ca_type_txt,
                         "This enumerations the civic address type.");

/** \brief LLDP neighbor information  */
typedef struct {
    char chassis_id[VTSS_APPL_MAX_CHASSIS_ID_LENGTH];          /**< Chassis ID string.*/
    char port_id[VTSS_APPL_MAX_PORT_ID_LENGTH];                /**< Port ID string.*/
    char port_description[VTSS_APPL_MAX_PORT_DESCR_LENGTH];    /**< Port description string.*/
    char system_name[VTSS_APPL_MAX_SYSTEM_NAME_LENGTH];        /**< System name string.*/
    char system_description[VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH];/**< System description string.*/
    u16  capa;                                                 /**< Capabilities bit mask, Figure 9-10, IEEE802.1AB-2005*/
    u16  capa_ena;                                             /**< Capabilities which are enabled bit mask. Figure 9-10, IEEE802.1AB-2005*/
} lldp_neighbors_information_t;

typedef struct {
    vtss_appl_lldp_admin_state_t admin_state;
#ifdef VTSS_SW_OPTION_CDP
    BOOL        cdp_aware;
#endif
    uchar       optional_tlv;
    BOOL        snmp_notification_ena;
} lldp_port_mib_conf_t;


/** \brief LLDP-MED configuration that are global for the whole switch/stack */
typedef struct {
    /** Fast Start Repeat Count (medFastStart), TIA1057, Section 11.2.1 bullet b*/
    u8  medFastStartRepeatCount;

    /** Location information (latitude, longitude, altitude etc)*/
    vtss_appl_lldp_med_location_conf_t coordinate_location;

    /** Emergency call service, Figure 11, TIA1057. Adding 1 for making space for "\0"*/
    char  elin_location[VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX + 1];

    char ca_country_code[VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN]; /**< Location country code string, Figure 10, TIA1057*/
} lldpmed_global_conf_t;


/** \brief LLDP-MED per port remote neighbor information for snmp*/
typedef struct {
    /**< The capabilities the device has, Figure 6, TIA1057*/
    u16 lldpmed_capabilities;

    /** Capabilities that is currently enabled*, TIA1057, MIBS LLDPXMEDREMCAPCURRENT*/
    u16 lldpmed_capabilities_current;

    /** Device type, Table 11, TIA1057*/
    vtss_appl_lldp_med_remote_device_type_t device_type;

    /** Hardware revision TLV - Figure 13,  TIA1057*/
    char lldpmed_hw_rev[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];

    /** Firmware revision TLV - Figure 14,  TIA1057*/
    char lldpmed_firm_rev[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];

    /** Software revision TLV - Figure 15,  TIA1057*/
    char lldpmed_sw_rev[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];

    /** Serial number  TLV - Figure 16,  TIA1057*/
    char lldpmed_serial_no[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];

    /** Manufacturer name TLV - Figure 17,  TIA1057*/
    char lldpmed_manufacturer_name[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];

    /** Model name - Figure 17,  TIA1057*/
    char lldpmed_model_name[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];

    /** Asset ID TLV - Figure 19,  TIA1057*/
    char lldpmed_asset_id[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];

    /** Location information*/
    vtss_appl_lldp_med_location_info_t coordinate_location;

    /** Emergency call service, Figure 11, TIA1057. Adding 1 for making space for "\0"*/
    char elin_location[VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX + 1];

    vtss_appl_lldp_eee_t eee;         /**< EEE (Energy Efficient Ethernet) TLV*/
} lldpmed_remote_info_t;

typedef struct {
    BOOL global_clr;
} vtss_appl_lldp_global_cnt_t;


typedef struct {
    vtss_appl_lldp_med_network_policy_t conf_policy;
} vtss_appl_lldp_med_conf_network_policy_t;

/** \brief LLDP-MED per port configuration for snmp*/
typedef struct {
    /** Enable/disable of the LLDP-MED optional TLVs (TIA-1057, MIB LLDPXMEDPORTCONFIGTLVSTXENABLE).*/
    u8                       lldpmed_optional_tlv;

    /** Selecting the LLDP-MED operating mode (Same as device type, either end-point of network connectivity device)*/
    vtss_appl_lldp_med_device_type_t device_type;
} lldpmed_port_conf_t;

/****************************************************************************
 * Type serializers
 ****************************************************************************/
template<typename T>
void serialize(T &a, lldpmed_port_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("lldpmed_port_conf_t"));
    int ix = 3;

    m.add_leaf(s.lldpmed_optional_tlv,
               vtss::tag::Name("OptionalTlvs"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Enables/Disables the LLDP optional TLVs. Bit mask, where setting the bit to 1 means\n\
                                      enable transmission of the corresponding optional TLV.\n        \
                                      Bit 0 represents the capabilities TLV.\n\
                                      Bit 1 represents the network Policy TLV.\n\
                                      Bit 2 represents the location TLV.\n\
                                      Bit 3 represents the PoE TLV."));

    m.add_leaf(s.device_type,
               vtss::tag::Name("DeviceType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Setting device type to configure the mode the device shall operate as."));
}

template<typename T>
void serialize(T &a, vtss_appl_lldp_med_conf_network_policy_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lldp_med_conf_network_policy_t"));
    int ix = 3;

    m.add_leaf(s.conf_policy.application_type,
               vtss::tag::Name("ApplicationType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LLDP policy application type."));


    m.add_leaf(vtss::AsBool(s.conf_policy.tagged_flag),
               vtss::tag::Name("Tagged"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LLDP policy tagged flag. Defines if LLDP policy uses tagged VLAN."));

    m.add_leaf(s.conf_policy.vlan_id,
               vtss::tag::Name("VlanId"),
               vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LLDP policy VLAN ID. Only valid when policy 'Tagged' is TRUE"));

    m.add_leaf(s.conf_policy.l2_priority,
               vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_LLDP_MED_L2_PRIORITY_MIN, VTSS_APPL_LLDP_MED_L2_PRIORITY_MAX),
               vtss::tag::Name("L2Priority"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LLDP policy L2 priority."));

    m.add_leaf(s.conf_policy.dscp_value,
               vtss::tag::Name("Dscp"),
               vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_LLDP_MED_DSCP_MIN, VTSS_APPL_LLDP_MED_DSCP_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LLDP policy DSCP."));
}

template<typename T>
void serialize(T &a, vtss_appl_lldp_med_network_policy_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lldp_med_network_policy_t"));
    int ix = 5;
    m.add_leaf(s.application_type,
               vtss::tag::Name("ApplicationType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The LLDP policy application type."));

    m.add_leaf(vtss::AsBool(s.unknown_policy_flag),
               vtss::tag::Name("UnknownPolicy"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Policy indicates that an Endpoint Device wants to explicitly advertise that\
                                       the policy is required by the device. Can be either Defined or Unknown\n\
                                       Unknown: The network policy for the specified application type is currently unknown.\n\
                                       Defined: The network policy is defined (known)."));

    m.add_leaf(vtss::AsBool(s.tagged_flag),
               vtss::tag::Name("Tagged"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Defines if LLDP policy uses tagged VLAN."));

    m.add_leaf(s.vlan_id,
               vtss::tag::Name("VlanId"),
               vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The policy VLAN ID. Only valid when policy 'Tagged' is TRUE"));

    m.add_leaf(s.l2_priority,
               vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_LLDP_MED_L2_PRIORITY_MIN, VTSS_APPL_LLDP_MED_L2_PRIORITY_MAX),
               vtss::tag::Name("L2Priority"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's policy L2 priority."));

    m.add_leaf(s.dscp_value,
               vtss::tag::Name("Dscp"),
               vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_LLDP_MED_DSCP_MIN, VTSS_APPL_LLDP_MED_DSCP_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("LLDP policy DSCP."));
}

template<typename T>
void serialize(T &a, lldpmed_remote_info_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("lldpmed_remote_info_t"));
    int ix = 4;
    m.add_leaf(s.lldpmed_capabilities,
               vtss::tag::Name("Capabilities"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               // Table 10 in TIA-1057
               vtss::tag::Description("LLDP neighbor's capabilities bit mask.\n\
                                      Bit 0 represents LLDP-MED capabilities.\n\
                                      Bit 1 represents Network Policy.\n\
                                      Bit 2 represents Location Identification.\n\
                                      Bit 3 represents Extended Power via MDI - PSE.\n\
                                      Bit 4 represents Extended Power via MDI - PD\n\
                                      Bit 5 represents Inventory\n"));

    m.add_leaf(s.lldpmed_capabilities_current,
               vtss::tag::Name("CapabilitiesEnabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               // Table 10 in TIA-1057
               vtss::tag::Description("LLDP neighbor's capabilities bit mask for the capabilities which are currently enabled.\n\
                                      Bit 0 represents LLDP-MED capabilities.\n\
                                      Bit 1 represents Network Policy.\n\
                                      Bit 2 represents Location Identification.\n\
                                      Bit 3 represents Extended Power via MDI - PSE.\n\
                                      Bit 4 represents Extended Power via MDI - PD\n\
                                      Bit 5 represents Inventory\n"));

    m.add_leaf(s.coordinate_location.latitude,
               vtss::tag::Name("Latitude"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(LATITUDE_DESCRIPTION));

    m.add_leaf(s.coordinate_location.longitude,
               vtss::tag::Name("Longitude"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(LONGITUDE_DESCRIPTION));

    m.add_leaf(s.coordinate_location.altitude_type,
               vtss::tag::Name("AltitudeType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Altitude type as either floors or meters. \
Meters are representing meters of altitude defined by the vertical datum specified. \
Floors are representing altitude in a form more relevant in buildings which have different \
floor-to-floor dimensions. An altitude = 0.0 is meaningful even outside a building, \
and represents ground level at the given latitude and longitude. \
Inside a building, 0.0 represents the floor level associated with ground level at the main entrance."));

    m.add_leaf(s.coordinate_location.altitude,
               vtss::tag::Name("Altitude"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(ALTITUDE_DESCRIPTION));

    m.add_leaf(s.coordinate_location.datum,
               vtss::tag::Name("Datum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Datum (geodetic system) .\n\
The Map Datum is used for the coordinates given in these options:\n\
WGS84:  (Geographical 3D) - World Geodesic System 1984, CRS Code 4327, Prime Meridian Name: Greenwich.\n\
NAD83/NAVD88: North American Datum 1983, CRS Code 4269, Prime Meridian Name: Greenwich; The associated vertical datum is the North American Vertical Datum of 1988 (NAVD88). This datum pair is to be used when referencing locations on land, not near tidal water\
(which would use Datum = NAD83/MLLW).\n\
NAD83/MLLW:  North American Datum 1983, CRS Code 4269, Prime Meridian Name: Greenwich; The associated vertical datum is Mean \
Lower Low Water (MLLW). This datum pair is to be used when referencing locations on water/sea/ocean."));

    m.add_leaf(vtss::AsDisplayString(&s.elin_location[0], VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX + 1), // Plus 1 for "\0"
               vtss::tag::Name("Elinaddr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(ELIN_DESCRIPTION));

    m.add_leaf(s.device_type,
               vtss::tag::Name("DeviceType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's device type."));

    m.add_leaf(vtss::AsDisplayString(s.lldpmed_sw_rev, sizeof(s.lldpmed_hw_rev)),
               vtss::tag::Name("HwRev"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's hardware revision."));

    m.add_leaf(vtss::AsDisplayString(s.lldpmed_sw_rev, sizeof(s.lldpmed_firm_rev)),
               vtss::tag::Name("FwRev"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's firmware revision."));

    m.add_leaf(vtss::AsDisplayString(s.lldpmed_sw_rev, sizeof(s.lldpmed_sw_rev)),
               vtss::tag::Name("SwRev"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's software revision."));

    m.add_leaf(vtss::AsDisplayString(s.lldpmed_serial_no, sizeof(s.lldpmed_serial_no)),
               vtss::tag::Name("SerialNo"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's serial number."));

    m.add_leaf(vtss::AsDisplayString(s.lldpmed_manufacturer_name, sizeof(s.lldpmed_manufacturer_name)),
               vtss::tag::Name("ManufacturerName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's manufacturer name."));

    m.add_leaf(vtss::AsDisplayString(s.lldpmed_model_name, sizeof(s.lldpmed_model_name)),
               vtss::tag::Name("ModelName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's model name."));

    m.add_leaf(vtss::AsDisplayString(s.lldpmed_asset_id, sizeof(s.lldpmed_asset_id)),
               vtss::tag::Name("AssetId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's asset id."));

    m.add_leaf(s.eee.RemRxTwSys,
               vtss::tag::Name("EeeRxTwSys"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's Receive tw_sys_rx . Tw_sys_rx is defined as the time (expressed in microseconds) that the transmitting link partner will wait before it starts transmitting data after leaving the Low Power Idle (LPI) mode. Section 79.3.5.2, IEEE802.3az."));

    m.add_leaf(s.eee.RemTxTwSys,
               vtss::tag::Name("EeeTxTwSys"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's Transmit Tw_sys_tx . Tw_sys_tx is defined as the time (expressed in microseconds) that the receiving link partner is requesting the transmitting link partner to wait before starting the transmission data following the Low Power Idle (LPI) mode. Section 79.3.5.1, IEEE802.3az."));

    m.add_leaf(s.eee.RemFbTwSys,
               vtss::tag::Name("EeeFbTwSys"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's Fallback tw_sys_tx. A receiving link partner may inform the transmitter of an alternate desired Tw_sys_tx. Since a receiving link partner is likely to have discrete levels for savings, this provides the transmitter with additional information that it may use for a more efficient allocation. Section 79.3.5.3, IEEE802.3az."));

    m.add_leaf(s.eee.RemTxTwSysEcho,
               vtss::tag::Name("EeeTxTwSysEcho"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's echo transmit Tw. The respective echo values shall be defined as the local link partners reflection (echo) of the remote link partners respective values. When a local link partner receives its echoed values from the remote link partner it can determine whether or not the remote link partner has received, registered, and processed its most recent values. Section 79.3.5.4, IEEE802.3az."));

    m.add_leaf(s.eee.RemRxTwSysEcho,
               vtss::tag::Name("EeeRxTwSysEcho"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's echo receive Tw. The respective echo values shall be defined as the local link partners reflection (echo) of the remote link partners respective values. When a local link partner receives its echoed values from the remote link partner it can determine whether or not the remote link partner has received, registered, and processed its most recent values. Section 79.3.5.4, IEEE802.3az."));

}

/** \brief Ca type containing one of the civic location strings. Used for serialization */
typedef struct {
    char ca_type[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];      /**< String containing any of the civic location strings*/
} vtss_lldpmed_ca_type_t;


template<typename T>
void serialize(T &a, vtss_lldpmed_ca_type_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_lldpmed_ca_type_t"));
    int ix = 3;

    m.add_leaf(vtss::AsDisplayString(&s.ca_type[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("civicAddress"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Civic address"));
}

template<typename T>
void serialize(T &a, vtss_appl_lldpmed_civic_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lldpmed_civic_t"));
    int ix = 5;

    m.add_leaf(vtss::AsDisplayString(&s.a1[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("State"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("National subdivision"));

    m.add_leaf(vtss::AsDisplayString(&s.a2[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("County"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("County"));

    m.add_leaf(vtss::AsDisplayString(&s.a3[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("City"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("City"));

    m.add_leaf(vtss::AsDisplayString(&s.a4[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("District"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("City district"));

    m.add_leaf(vtss::AsDisplayString(&s.a5[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("Block"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Block (Neighborhood)"));


    m.add_leaf(vtss::AsDisplayString(&s.a6[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("Street"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Street"));



    m.add_leaf(vtss::AsDisplayString(&s.prd[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("LeadingStreetDirection"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Street Direction"));


    m.add_leaf(vtss::AsDisplayString(&s.pod[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("TrailingStreetSuffix"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Trailing Street Suffix"));


    m.add_leaf(vtss::AsDisplayString(&s.sts[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("StreetSuffix"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Street Suffix"));


    m.add_leaf(vtss::AsDisplayString(&s.hno[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("HouseNo"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("House No."));


    m.add_leaf(vtss::AsDisplayString(&s.hns[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("HouseNoSuffix"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("House No. Suffix"));


    m.add_leaf(vtss::AsDisplayString(&s.lmk[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("Landmark"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Landmark"));


    m.add_leaf(vtss::AsDisplayString(&s.loc[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("AdditionalInfo"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Additional Location Info"));


    m.add_leaf(vtss::AsDisplayString(&s.nam[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("Name"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Name"));


    m.add_leaf(vtss::AsDisplayString(&s.zip[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("ZipCode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Zip code"));


    m.add_leaf(vtss::AsDisplayString(&s.build[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("Building"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Building"));


    m.add_leaf(vtss::AsDisplayString(&s.unit[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("Apartment"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Apartment/unit"));


    m.add_leaf(vtss::AsDisplayString(&s.flr[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("Floor"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Floor"));


    m.add_leaf(vtss::AsDisplayString(&s.room[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("RoomNumber"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Room Number"));


    m.add_leaf(vtss::AsDisplayString(&s.place[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("PlaceType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Place type"));


    m.add_leaf(vtss::AsDisplayString(&s.pcn[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("PostalCommunityName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Postal Community Name"));


    m.add_leaf(vtss::AsDisplayString(&s.pobox[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("PoBox"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Post Office Box"));


    m.add_leaf(vtss::AsDisplayString(&s.add_code[0], VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX  + 1), // Plus 1 for "\0"
               vtss::tag::Name("AdditionalCode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Additional Code"));

    m.add_leaf(vtss::AsDisplayString(&s.ca_country_code[0], VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN), // "\0" already included in the VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN
               vtss::tag::Name("CountryCode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The two-letter ISO 3166 country code in capital ASCII letters - Example: DK, DE or US."));

}

template<typename T>
void serialize(T &a, lldpmed_global_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("lldpmed_global_conf_t"));
    int ix = 1;

    m.add_leaf(s.medFastStartRepeatCount,
               vtss::expose::snmp::RangeSpec<u32>(VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_MIN, VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_MAX),
               vtss::tag::Name("FastRepeatCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of times to repeat LLDP frame transmission at fast start\n\
Rapid startup and Emergency Call Service Location Identification Discovery of endpoints is a critically \
important aspect of VoIP systems in general. In addition, it is best to advertise only those pieces of \
information which are specifically relevant to particular endpoint types (for example only advertise the \
voice network policy to permitted voice-capable devices), both in order to conserve the limited LLDPU \
space and to reduce security and system integrity issues that can come with inappropriate knowledge \
of the network policy.\n\
With this in mind LLDP-MED defines an LLDP-MED Fast Start interaction between the protocol and the application \
layers on top of the protocol, in order to achieve these related properties. Initially, a Network \
Connectivity Device will only transmit LLDP TLVs in an LLDPDU. Only after an LLDP-MED Endpoint Device is detected, \
will an LLDP-MED capable Network Connectivity Device start to advertise LLDP-MED TLVs in outgoing \
LLDPDUs on the associated port. The LLDP-MED application will temporarily speed up the transmission of the LLDPDU to start \
within a second, when a new LLDP-MED neighbor has been detected in order share LLDP-MED information as fast as possible to \
new neighbors.\n\
Because there is a risk of an LLDP frame being lost during transmission between neighbors, it is recommended  to repeat the fast \
start transmission multiple times to increase the possibility of the neighbors receiving the LLDP frame. With fast start repeat \
count it is possible to specify the number of times the fast start transmission would be repeated. \
The recommended value is 4 times, given that 4 LLDP frames with a 1 second interval will be transmitted, when an LLDP \
frame with new information is received.\n\
It should be noted that LLDP-MED and the LLDP-MED Fast Start mechanism is only intended to run on links between \
LLDP-MED Network Connectivity Devices and Endpoint Devices, and as such does not apply to links between LAN infrastructure \
elements, including Network Connectivity Devices, or other types of links."));


    m.add_leaf(s.coordinate_location.latitude,
               vtss::tag::Name("Latitude"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(LATITUDE_DESCRIPTION));


    m.add_leaf(s.coordinate_location.longitude,
               vtss::tag::Name("Longitude"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(LONGITUDE_DESCRIPTION));

    m.add_leaf(s.coordinate_location.altitude_type,
               vtss::tag::Name("AltitudeType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Setting altitude type \n\
Possible to select between two altitude types (floors or meters). Meters are representing meters of altitude defined by the \
vertical datum specified. Floors are representing altitude in a form more relevant in buildings which have different \
floor-to-floor dimensions. An altitude = 0.0 is meaningful even outside a building, and represents ground level at the given \
latitude and longitude. Inside a building, 0.0 represents the floor level associated with ground level at the main entrance."));

    m.add_leaf(s.coordinate_location.altitude,
               vtss::tag::Name("Altitude"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(ALTITUDE_DESCRIPTION));

    m.add_leaf(vtss::AsDisplayString(&s.elin_location[0], VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX + 1), // Plus 1 for "\0"
               vtss::tag::Name("ElinAddr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(ELIN_DESCRIPTION));

    m.add_leaf(s.coordinate_location.datum,
               vtss::tag::Name("Datum"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Setting datum to configure the datum (geodetic system) to use.\n\
The Map Datum is used for the coordinates given in these options:\n\
WGS84:  (Geographical 3D) - World Geodesic System 1984, CRS Code 4327, Prime Meridian Name: Greenwich.\n\
NAD83/NAVD88: North American Datum 1983, CRS Code 4269, Prime Meridian Name: Greenwich;\
The associated vertical datum is the North American Vertical Datum of 1988 (NAVD88).\
This datum pair is to be used when referencing locations on land, not near tidal water\
(which would use Datum = NAD83/MLLW).\n\
NAD83/MLLW:  North American Datum 1983, CRS Code 4269, Prime Meridian Name: Greenwich; The associated vertical datum is \
Mean Lower Low Water (MLLW). This datum pair is to be used when referencing locations on water/sea/ocean."));

    m.add_leaf(vtss::AsDisplayString(&s.ca_country_code[0], VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN),
               vtss::tag::Name("CountryCode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The two-letter ISO 3166 country code in capital ASCII letters - Example: DK, DE or US."));
}

template<typename T>
void serialize(T &a, u64 &s)
{
    int ix = 1;
    a.add_leaf(s,
               vtss::tag::Name("LastChangeTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the time when the last entry was last deleted or added. It also shows the time elapsed since the last change was detected."));
}

template<typename T>
void serialize(T &a, lldp_neighbors_information_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("lldp_neighbors_information_t"));
    int ix = 4;

    m.add_leaf(vtss::AsDisplayString(s.chassis_id, sizeof(s.chassis_id)),
               vtss::tag::Name("ChassisId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's chassis Id."));

    m.add_leaf(vtss::AsDisplayString(s.port_id, sizeof(s.port_id)),
               vtss::tag::Name("PortId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's port id."));

    m.add_leaf(vtss::AsDisplayString(s.port_description, sizeof(s.port_description)),
               vtss::tag::Name("PortDescription"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's port description."));

    m.add_leaf(vtss::AsDisplayString(s.system_name, sizeof(s.system_name)),
               vtss::tag::Name("SystemName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's system name."));

    m.add_leaf(vtss::AsDisplayString(s.system_description, sizeof(s.system_description)),
               vtss::tag::Name("SystemDescription"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's system description."));

    m.add_leaf(s.capa, sizeof(s.capa),
               vtss::tag::Name("SystemCapabilities"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's system capabilities as bit mask.\n\
If the bit is set, it means that the functionality is supported by the neighbor system.\n\
                                       Bit 0 represents Other.\n \
                                       Bit 1 represents Repeater.\n\
                                       Bit 2 represents Bridge.\n\
                                       Bit 3 represents WLAN Access Point.\n\
                                       Bit 4 represents Router.\n\
                                       Bit 5 represents Telephone.\n\
                                       Bit 6 represents DOCSIS cable device.\n\
                                       Bit 7 represents Station Only.\n\
                                       Bit 8 represents Reserved."));

    m.add_leaf(s.capa_ena, sizeof(s.capa_ena),
               vtss::tag::Name("SystemCapabilitiesEnable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the LLDP neighbor's system capabilities which is enabled.\n\
If the bit is set, it means that the functionality is currently enabled at the neighbor system.\n\
                                       Bit 0 represents Other.\n \
                                       Bit 1 represents Repeater.\n\
                                       Bit 2 represents Bridge.\n\
                                       Bit 3 represents WLAN Access Point.\n\
                                       Bit 4 represents Router.\n\
                                       Bit 5 represents Telephone.\n\
                                       Bit 6 represents DOCSIS cable device.\n\
                                       Bit 7 represents Station Only.\n\
                                       Bit 8 represents Reserved."));

}

// Because the serialize function is called with uninitialized data at boot time, we don't want to do any conversions in the serialize function (Valgrind complains if we do).
// In order to avoid conversion we have a specific type for the management address which fits directly to the MIB and can be used to serialize.
typedef struct {
    u8   subtype;                                       /**< Management address subtype , section 9.5.9.3 IEEE802.1AB-2005*/
    char mgmt_address[VTSS_APPL_MAX_MGMT_ADDR_LENGTH];  /**< Management address string, section 9.5.9.4 IEEE802.1AB-2005*/
    u8   if_number_subtype;                             /**< Interface numbering subtype, section 9.5.9.5 IEEE802.1AB-2005*/
    u32  if_number;                                     /**< Interface number, section 9.5.9.6 IEEE802.1AB-2005*/
    u32  oid_length;                                    /**< Object identifier length*/
    u32  oid[VTSS_APPL_MAX_OID_LENGTH];                 /**< Object identifier*/
} vtss_lldp_mgmt_addr_tlv_t;

typedef struct {
    BOOL                             loc_preempt_supported; /*!< The value of the 802.3br LocPreemptSupported
                                                             * parameter for the port.
                                                             * The value is TRUE when preemption is supported
                                                             * on the port, and FALSE otherwise.
                                                             **/
    BOOL                             loc_preempt_enabled;   /*!< The value of the 802.3br LocPreemptEnabled
                                                             * parameter for the port.
                                                             * The value is TRUE when preemption is enabled
                                                             * on the port, and FALSE otherwise.
                                                             **/
    BOOL                             loc_preempt_active;    /*!< The value of the 802.3br LocPreemptActive
                                                             * parameter for the port.
                                                             * The value is TRUE when preemption is operationally
                                                             * active on the port, and FALSE otherwise.
                                                             **/
    u8                               loc_add_frag_size;     /*!< The value of the 802.3br LocAddFragSize
                                                             * parameter for the port.
                                                             * The minimum size of non-final fragments supported by the
                                                             * receiver on the local port. This value is expressed in units
                                                             * of 64 octets of additional fragment length.
                                                             * The minimum non-final fragment size is:
                                                             * (LocAddFragSize + 1) * 64 octets.
                                                             **/
    BOOL                             RemFramePreemptSupported; /*!< Frame preemption capability, Table 79-7a, IEEEP802.3brD2.0*/
    BOOL                             RemFramePreemptEnabled;   /*!< Frame preemption status, Table 79-7a, IEEEP802.3brD2.0*/
    BOOL                             RemFramePreemptActive;    /*!< TRUE when Frame preemption is active, Table 79-7a, IEEEP802.3brD2.0*/
    u8                               RemFrameAddFragSize;    /*!< Minimum number of octets over 64 octets required in non-final fragments
                                                              * by the receiver, Table 79-7a, IEEEP802.3brD2.0*/
} vtss_lldp_mgmt_fp_t;

struct AsLldpTlvAddressString {
    AsLldpTlvAddressString (vtss_lldp_mgmt_addr_tlv_t &v) : val(v) {}
    vtss_lldp_mgmt_addr_tlv_t &val;
};

template<typename T>
void serialize(T &a, AsLldpTlvAddressString &s) {
    vtss::AsDisplayString ss(&s.val.mgmt_address[0], VTSS_APPL_MAX_MGMT_ADDR_LENGTH + 1);
    serialize(a, ss);
}

template<typename T>
void serialize(T &a, vtss_lldp_mgmt_addr_tlv_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_ldp_mgmt_addr_tlv_t"));
    int ix = 5;
    m.add_leaf(s.subtype,
               vtss::tag::Name("SystemMgmAddressSubtype"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("LLDP neighbor's management address subtype , section 9.5.9.3 IEEE802.1AB-2005."));

    m.add_leaf(AsLldpTlvAddressString(s),
          vtss::tag::Name("SystemMgmtAddress"),
          vtss::expose::snmp::Status::Current,
          vtss::expose::snmp::OidElementValue(ix++),
          vtss::tag::Description("LLDP neighbor's management address string, section 9.5.9.4 IEEE802.1AB-2005."));

    m.add_leaf(vtss::AsInt(s.if_number_subtype),
               vtss::tag::Name("SystemMgmtInterfaceSubtype"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface numbering subtype, section 9.5.9.5 IEEE802.1AB-2005."));

    m.add_leaf(vtss::AsInt(s.if_number),
               vtss::tag::Name("SystemMgmtInterface"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface number, section 9.5.9.6 IEEE802.1AB-2005."));

    T_DG(TRACE_GRP_SNMP, "s.oid:%u, %u, %u, %u, %u", s.oid[0], s.oid[1], s.oid[2], s.oid[3], s.oid[4]);

    m.add_leaf(vtss::AsSnmpObjectIdentifier(s.oid, VTSS_APPL_MAX_OID_LENGTH, s.oid_length),
               vtss::tag::Name("SystemMgmtOid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Object identifier, section 9.5.9.8 IEEE802.1AB-2005."));
}

template<typename T>
void serialize(T &a, vtss_lldp_mgmt_fp_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_lldp_mgmt_fp_t"));
    int ix = 5;
    m.add_leaf(vtss::AsBool(s.loc_preempt_supported),
               vtss::tag::Name("LocPreemptSupported"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br LocPreemptSupported \
                                       parameter for the port. The value is TRUE when preemption is supported \
                                       on the port, and FALSE otherwise."));

    m.add_leaf(vtss::AsBool(s.loc_preempt_enabled),
               vtss::tag::Name("LocPreemptEnabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br LocPreemptEnabled parameter for the port. \
                                       The value is TRUE when preemption is enabled on the port, and FALSE otherwise."));

    m.add_leaf(vtss::AsBool(s.loc_preempt_active),
               vtss::tag::Name("LocPreemptActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br LocPreemptActive parameter for the port. \
                                       The value is TRUE when preemption is operationally \
                                       active on the port, and FALSE otherwise."));

    m.add_leaf(s.loc_add_frag_size,
               vtss::tag::Name("LocAddFragSize"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br LocAddFragSize parameter for the port. \
                                       The minimum size of non-final fragments supported by the \
                                       receiver on the local port. This value is expressed in units \
                                       of 64 octets of additional fragment length. The minimum non-final fragment size is: \
                                       (LocAddFragSize + 1) * 64 octets"));

    m.add_leaf(vtss::AsBool(s.RemFramePreemptSupported),
               vtss::tag::Name("RemFramePreemptSupported"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br RemPreemptSupported \
                                       parameter for the port. The value is TRUE when preemption is supported \
                                       on the port, and FALSE otherwise."));

    m.add_leaf(vtss::AsBool(s.RemFramePreemptEnabled),
               vtss::tag::Name("RemFramePreemptEnabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br RemPreemptEnabled parameter for the port. \
                                       The value is TRUE when preemption is enabled on the port, and FALSE otherwise."));

    m.add_leaf(vtss::AsBool(s.RemFramePreemptActive),
               vtss::tag::Name("RemFramePreemptActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br RemPreemptActive parameter for the port. \
                                       The value is TRUE when preemption is operationally \
                                       active on the port, and FALSE otherwise."));


    m.add_leaf(s.RemFrameAddFragSize,
               vtss::tag::Name("RemFrameAddFragSize"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br RemAddFragSize parameter for the port. \
                                       The minimum size of non-final fragments supported by the \
                                       receiver on the local port. This value is expressed in units \
                                       of 64 octets of additional fragment length. The minimum non-final fragment size is: \
                                       (LocAddFragSize + 1) * 64 octets"));
}
template<typename T>
void serialize(T &a, vtss_appl_lldp_global_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lldp_global_conf_t"));
    int ix = 1;
    char description_range_txt[500];

    snprintf(&description_range_txt[0], sizeof(description_range_txt),
             "Set the LLDP tx reinitialization delay in seconds. Valid range %d-%d.\n\
              When a port is disabled, LLDP is disabled or the switch is rebooted, a LLDP shutdown frame is transmitted to the neighboring units,\
              signaling that the LLDP information isn't valid anymore.\n\
              Tx reinitialization delay controls the amount of seconds between the shutdown frame and a new LLDP initialization.",
             VTSS_APPL_LLDP_REINIT_MIN, VTSS_APPL_LLDP_REINIT_MAX);

    m.add_leaf(s.reInitDelay,
               vtss::tag::Name("ReInitDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(description_range_txt));

    snprintf(&description_range_txt[0], sizeof(description_range_txt),
             "Set the LLDP tx hold times . Valid range %d-%d.\n\
              Each LLDP frame contains information about how long time the information in the LLDP frame shall be considered valid.\
              The LLDP information valid period is set to tx hold times multiplied by tx interval seconds.",
             VTSS_APPL_LLDP_TX_HOLD_MIN, VTSS_APPL_LLDP_TX_HOLD_MAX);

    m.add_leaf(s.msgTxHold,
               vtss::tag::Name("msgTxHold"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(description_range_txt));

    snprintf(&description_range_txt[0], sizeof(description_range_txt), "Set the LLDP tx interval in seconds.\n\
The switch periodically transmits LLDP frames to its neighbors for having the network discovery information up-to-date. The interval between each LLDP frame is determined by the tx Interval value.\n\
\n\
Valid range %d-%d seconds.",
             VTSS_APPL_LLDP_TX_INTERVAL_MIN, VTSS_APPL_LLDP_TX_INTERVAL_MAX);

    m.add_leaf(s.msgTxInterval,
               vtss::tag::Name("msgTxInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(description_range_txt));


    snprintf(&description_range_txt[0], sizeof(description_range_txt),
             "Set the LLDP tx delay in seconds. Valid range %d-%d.\n\
              If some configuration is changed (e.g. the IP address) a new LLDP frame is transmitted, but the time between the LLDP frames will always be at least the value of tx delay seconds.\n \
              Note: tx Delay cannot be larger than 1/4 of the tx interval.",
             VTSS_APPL_LLDP_TX_DELAY_MIN, VTSS_APPL_LLDP_TX_DELAY_MAX);
    m.add_leaf(s.txDelay,
               vtss::tag::Name("txDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(description_range_txt));
}

template<typename T>
void serialize(T &a, lldp_port_mib_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("lldp_port_mib_conf_t"));
    int ix = 3;
    m.add_leaf(s.admin_state,
               vtss::tag::Name("AdminState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Sets the LLDP admin state for the interface."));

#ifdef VTSS_SW_OPTION_CDP
    m.add_leaf(vtss::AsBool(s.cdp_aware),
               vtss::tag::Name("CdpAware"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Enables/disables CDP awareness for the interface.\n\
CDP is Cisco's equivalent to LLDP.\n\
\n\
The CDP operation is restricted to decoding incoming CDP frames (The switch doesn't transmit CDP frames). CDP frames are only decoded if LLDP on the port is enabled.\n\
\n\
Only CDP TLVs that can be mapped to a corresponding field in the LLDP neighbors' table are decoded. All other TLVs are discarded (Unrecognized CDP TLVs and discarded CDP frames are not shown in the LLDP statistics.).\n\
CDP TLVs are mapped onto LLDP neighbors' table as shown below.\n\
CDP TLV 'Device ID' is mapped to the LLDP 'Chassis ID' field.\n\
CDP TLV 'Address' is mapped to the LLDP 'Management Address' field. The CDP address TLV can contain multiple addresses, but only the first address is shown in the LLDP neighbors table.\n\
CDP TLV 'Port ID' is mapped to the LLDP 'Port ID' field.\n\
CDP TLV 'Version and Platform' is mapped to the LLDP 'System Description' field.\n\
Both the CDP and LLDP support 'system capabilities', but the CDP capabilities cover capabilities that are not part of the LLDP. These capabilities are shown as 'others' in the LLDP neighbors' table.\n\
\n\
If all ports have CDP awareness disabled the switch forwards CDP frames received from neighbor devices. If at least one port has CDP awareness enabled all CDP frames are terminated by the switch.\n\
\n\
Note: When CDP awareness on a port is disabled the CDP information isn't removed immediately, but gets removed when the hold time is exceeded."));
#endif

    m.add_leaf(s.optional_tlv,
               vtss::tag::Name("OptionalTlvs"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Enables/Disables the LLDP optional TLVs. Bit mask, where setting the bit to 1 means enable transmission of the corresponding TLV.\n\
                                       Bit 0 represents Port Description TLV.\n \
                                       Bit 1 represents System Name TLV.\n\
                                       Bit 2 represents System Description.\n\
                                       Bit 3 represents System Capabilities TLV.\n\
                                       Bit 4 represents Management Address TLV."));

#ifdef VTSS_SW_OPTION_SNMP
    m.add_leaf(vtss::AsBool(s.snmp_notification_ena),
               vtss::tag::Name("SnmpNotificationEna"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Enable/disable of SNMP Trap notification."));
#endif
}

template<typename T>
void serialize(T &a, vtss_appl_lldp_global_counters_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lldp_global_counters_t"));
    int ix = 1;

    m.add_leaf(s.table_inserts,
               vtss::tag::Name("TableInserts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of new entries added since switch reboot."));

    m.add_leaf(s.table_deletes,
               vtss::tag::Name("TableDeletes"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of new entries deleted since switch reboot."));

    m.add_leaf(s.table_drops,
               vtss::tag::Name("TableDrops"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of LLDP frames dropped due to the entry table being full."));

    m.add_leaf(s.table_ageouts,
               vtss::tag::Name("TableAgeOuts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of entries deleted due to Time-To-Live expiring."));

    m.add_leaf(s.last_change_ago,
               vtss::tag::Name("LastChangeTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the time when the last entry was last deleted or added. It also shows the time elapsed since the last change was detected."));
}

template<typename T>
void serialize(T &a, vtss_appl_lldp_port_counters_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_lldp_port_counters_t"));
    int ix = 2;
    m.add_leaf(s.statsFramesOutTotal, vtss::tag::Name("TxTotal"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of LLDP frames transmitted."));

    m.add_leaf(s.statsFramesInTotal, vtss::tag::Name("RxTotal"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of LLDP frames received."));

    m.add_leaf(s.statsFramesInErrorsTotal, vtss::tag::Name("RxError"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of received LLDP frames containing some kind of error."));

    m.add_leaf(s.statsFramesDiscardedTotal, vtss::tag::Name("RxDiscarded"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Show the number of LLDP frames discarded.\
                                       If a LLDP frame is received at an interface, and the switch's internal table has run full, the LLDP frame is counted and discarded.\
                                       This situation is known as \'Too Many Neighbors\' in the LLDP standard.\
                                       LLDP frames require a new entry in the table when the Chassis ID or Remote Port ID is not already contained within the table.\
                                       Entries are removed from the table when a given interface's link is down, an LLDP shutdown frame is received, or when the entry ages out."));

    m.add_leaf(s.statsTLVsDiscardedTotal, vtss::tag::Name("TLVsDiscarded"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of TLVs discarded.\
                                       Each LLDP frame can contain multiple pieces of information, known as TLVs (TLV is short for \'Type Length Value\').\
                                       If a TLV is malformed, it is counted and discarded."));


    m.add_leaf(s.statsTLVsUnrecognizedTotal, vtss::tag::Name("TLVsUnrecognized"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of well-formed TLVs, but with an unknown type value."));

    m.add_leaf(s.statsOrgTVLsDiscarded, vtss::tag::Name("TLVsOrgDiscarded"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of well-formed TLVs, but with an organizationally TLV which is not supported."));

    m.add_leaf(s.statsAgeoutsTotal, vtss::tag::Name("AgeOuts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Show the number of age-outs.\
                                       Each frame contains information about how long time the LLDP information is valid (age-out time).\
                                       If no new LLDP frame is received within the age out time, the information is removed, and the counter is incremented."));
}

/****************************************************************************
 * Iterators
 ****************************************************************************/
mesa_rc vtss_ifindex_management_entry_itr(vtss_appl_lldp_remote_entry_t *entry, const u32 *prev_management_index,
                                          u32 *next_management_index);
mesa_rc vtss_ifindex_port_entry_itr(vtss_isid_t isid, mesa_port_no_t iport, const u32 *prev_entry, u32 *next_entry,
                                    BOOL get_manengement_addr, const u32 *prev_management_index, u32 *next_management_index);

mesa_rc remote_policies_itr(const vtss_ifindex_t              *prev_port_ifindex, vtss_ifindex_t              *next_port_ifindex,
                            const vtss_lldp_entry_index_t  *prev_table_entry,  vtss_lldp_entry_index_t  *next_table_entry,
                            const vtss_lldpmed_policy_index_t *prev_policy,       vtss_lldpmed_policy_index_t *next_policy);

mesa_rc policies_itr(const vtss_lldpmed_policy_index_t *prev_index, vtss_lldpmed_policy_index_t *next_index);

/****************************************************************************
 * Get and get declarations  -- See lldp_expose.cxx
 ****************************************************************************/
mesa_rc lldp_global_stats_dummy_get( BOOL *const clear);
mesa_rc lldp_stats_global_clr_set(const BOOL *const clear);

mesa_rc vtss_lldp_global_conf_set(const vtss_appl_lldp_global_conf_t *lldp_conf);
mesa_rc vtss_lldp_global_conf_get(vtss_appl_lldp_global_conf_t *lldp_conf);

mesa_rc vtss_lldpmed_global_conf_set(const lldpmed_global_conf_t *lldpmed_conf);
mesa_rc vtss_lldpmed_global_conf_get(lldpmed_global_conf_t *lldpmed_conf);

mesa_rc lldpmed_policies_def(vtss_lldpmed_policy_index_t *policy_index,
                             vtss_appl_lldp_med_conf_network_policy_t *network_policy);
mesa_rc lldpmed_policies_get(vtss_lldpmed_policy_index_t policy_index,
                             vtss_appl_lldp_med_conf_network_policy_t *network_policy);
mesa_rc lldpmed_policies_set(vtss_lldpmed_policy_index_t policy_index,
                             const vtss_appl_lldp_med_conf_network_policy_t *network_policy);
mesa_rc lldpmed_policies_del(vtss_lldpmed_policy_index_t policy_index);


mesa_rc lldpmed_neighbors_network_policy_info_get(vtss_ifindex_t ifindex,
                                                  vtss_lldp_entry_index_t entry_index,
                                                  vtss_lldpmed_policy_index_t policy_index,
                                                  vtss_appl_lldp_med_network_policy_t *lldp_policy);

mesa_rc lldp_neighbors_management_infor_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, u32 mgmt_addr_index,
                                            vtss_lldp_mgmt_addr_tlv_t *lldp_info);
mesa_rc lldp_preempt_management_infor_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, u32 mgmt_addr_index,
                                          vtss_lldp_mgmt_fp_t *lldp_preempt);

mesa_rc lldpmed_conf_location_get(vtss_appl_lldp_med_catype_t ca_type_index, vtss_lldpmed_ca_type_t *civic_location);
mesa_rc lldpmed_conf_location_set(vtss_appl_lldp_med_catype_t ca_type_index, const vtss_lldpmed_ca_type_t *civic_location);

mesa_rc lldpmed_neighbors_location_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, vtss_appl_lldpmed_civic_t *lldp_info);

mesa_rc lldpmed_neighbors_information_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, lldpmed_remote_info_t *lldp_info);
mesa_rc lldp_neighbors_information_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, lldp_neighbors_information_t *lldp_info);

mesa_rc lldp_stats_dummy_get(vtss_ifindex_t ifindex,  BOOL *const clear);
mesa_rc lldp_stats_clr_set(vtss_ifindex_t ifindex, const BOOL *const clear);

mesa_rc vtss_lldp_port_conf_get(vtss_ifindex_t ifindex, lldp_port_mib_conf_t *lldp_port_conf);
mesa_rc vtss_lldp_port_conf_set(vtss_ifindex_t ifindex, const lldp_port_mib_conf_t *lldp_port_conf);

mesa_rc vtss_lldpmed_port_conf_get(vtss_ifindex_t ifindex, lldpmed_port_conf_t *lldpmed_port_conf);
mesa_rc vtss_lldpmed_port_conf_set(vtss_ifindex_t ifindex, const lldpmed_port_conf_t *lldpmed_port_conf);

mesa_rc lldpmed_policies_list_info_set(vtss_ifindex_t port_ifindex, vtss_lldpmed_policy_index_t policy_index, const BOOL *policy);
/****************************************************************************
 * Leafs
 ****************************************************************************/
namespace vtss {
namespace appl {
namespace lldp {
namespace interfaces {
struct LldpStatClearLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<BOOL *>
    > P;

    static constexpr const char *table_description =
        "This is a table to clear LLDP statistics for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of statistics counters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_lldp_statistics_clr_t(i));
    }

    VTSS_EXPOSE_GET_PTR(lldp_stats_dummy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(lldp_stats_clr_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpPortConfLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<lldp_port_mib_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to configure LLDP configurations for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of control parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(lldp_port_mib_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_lldp_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_lldp_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpMedPortConfLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<lldpmed_port_conf_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to configure LLDP MEDIA configurations for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of control parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(lldpmed_port_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_lldpmed_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_lldpmed_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpMedPoliciesConfLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_lldpmed_policy_index_t>,
        vtss::expose::ParamVal<vtss_appl_lldp_med_conf_network_policy_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table to configure LLDP MED Policies for the device.";

    static constexpr const char *index_description =
        "Network Policy Discovery enables the efficient discovery and diagnosis of mismatch issues with \
the VLAN configuration, along with the associated Layer 2 and Layer 3 attributes, which apply \
for a set of specific protocol applications on that port. Improper network policy configurations \
are a very significant issue in VoIP environments that frequently result in voice quality \
degradation or loss of service. Policies are only intended for use with applications that have \
specific 'real-time' network policy requirements, such as interactive voice and/or video services.";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_lldpmed_policy_index_t &i) {
        serialize(h, LLDP_policy_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_lldp_med_conf_network_policy_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(lldpmed_policies_get);
    VTSS_EXPOSE_ITR_PTR(policies_itr);
    VTSS_EXPOSE_SET_PTR(lldpmed_policies_set);
    VTSS_EXPOSE_ADD_PTR(lldpmed_policies_set);
    VTSS_EXPOSE_DEL_PTR(lldpmed_policies_del);
    VTSS_EXPOSE_DEF_PTR(lldpmed_policies_def);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP);
};


struct LldpStatusNetworkPolicyLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<vtss_lldp_entry_index_t>,
        vtss::expose::ParamKey<vtss_lldpmed_policy_index_t>,
        vtss::expose::ParamVal<vtss_appl_lldp_med_network_policy_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to show the LLDP-MED remote device (neighbor) network policies information for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of neighbors information";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_lldp_entry_index_t &i) {
        serialize(h, LLDP_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_lldpmed_policy_index_t &i) {
        serialize(h, LLDP_policy_index_3(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_lldp_med_network_policy_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(lldpmed_neighbors_network_policy_info_get);
    VTSS_EXPOSE_ITR_PTR(remote_policies_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpPoliciesListLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<vtss_lldpmed_policy_index_t>,
        vtss::expose::ParamVal<BOOL *>
    > P;

    static constexpr const char *table_description =
        "Each interface can be mapped to multiple policies. Set to TRUE in order to enable the corresponding policy to be transmitted at the interface. It is a requirement that the policy is defined.";

    static constexpr const char *index_description =
        "Each location information has a control parameter";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_lldpmed_policy_index_t &i) {
        serialize(h, LLDP_policy_index_3(i));
    }


    VTSS_EXPOSE_SERIALIZE_ARG_3(BOOL &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_lldpmed_policies_list_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lldp_conf_port_policy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_lldp_port_policies_list_itr);
    VTSS_EXPOSE_SET_PTR(lldpmed_policies_list_info_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpStatistics {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_lldp_port_counters_t *>
    > P;

    static constexpr const char *table_description =
        "This table represents the LLDP  interface counters";

    static constexpr const char *index_description =
        "Each port interface has a set of statistics counters";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_lldp_port_counters_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lldp_stat_if_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpNeighbors {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<vtss_lldp_entry_index_t>,
        vtss::expose::ParamVal<lldp_neighbors_information_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to show the LLDP neighbors information for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of neighbors information";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_lldp_entry_index_t &i) {
        serialize(h, LLDP_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(lldp_neighbors_information_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(lldp_neighbors_information_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_lldp_port_entries_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpNeighborsMgmt {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<vtss_lldp_entry_index_t>,
        vtss::expose::ParamKey<vtss_lldp_management_index_t>,
        vtss::expose::ParamVal<vtss_lldp_mgmt_addr_tlv_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to show the LLDP neighbors information for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of neighbors information";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_lldp_entry_index_t &i) {
        serialize(h, LLDP_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_lldp_management_index_t &i) {
        serialize(h, LLDP_mgmt_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_lldp_mgmt_addr_tlv_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(lldp_neighbors_management_infor_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_lldp_port_mgmt_entries_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpPreempt {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<vtss_lldp_entry_index_t>,
        vtss::expose::ParamKey<vtss_lldp_management_index_t>,
        vtss::expose::ParamVal<vtss_lldp_mgmt_fp_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to show the LLDP Frame Preemption information for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a Frame preemption information";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_lldp_entry_index_t &i) {
        serialize(h, LLDP_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_lldp_management_index_t &i) {
        serialize(h, LLDP_mgmt_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_lldp_mgmt_fp_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(lldp_preempt_management_infor_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_lldp_port_mgmt_entries_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LldpMedRemote {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<vtss_lldp_entry_index_t>,
        vtss::expose::ParamVal<lldpmed_remote_info_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to show the LLDP neighbors information for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of neighbors information";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_lldp_entry_index_t &i) {
        serialize(h, LLDP_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(lldpmed_remote_info_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(lldpmed_neighbors_information_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_lldp_port_entries_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};


struct LldpMedCivic {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<vtss_lldp_entry_index_t>,
        vtss::expose::ParamVal<vtss_appl_lldpmed_civic_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to show the LLDP-MED remote device (neighbor) civic location information for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of neighbors information";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, LLDP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_lldp_entry_index_t &i) {
        serialize(h, LLDP_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_lldpmed_civic_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(lldpmed_neighbors_location_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_lldp_port_entries_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LllpStatGlobalClr {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<BOOL *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(BOOL &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_lldp_global_statistics_clr_t(i));
    }

    VTSS_EXPOSE_GET_PTR(lldp_global_stats_dummy_get);
    VTSS_EXPOSE_SET_PTR(lldp_stats_global_clr_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LllpConfigMedGlobal {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<lldpmed_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(lldpmed_global_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_lldpmed_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_lldpmed_global_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP);
};


struct LllpConfigMedLoc {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_lldp_med_catype_t>,
        vtss::expose::ParamVal<vtss_lldpmed_ca_type_t *>
    > P;

    static constexpr const char *table_description =
        "The civic address location information. Each civic address can contain up to 250 characters, but the total amount of characters for the combined civic address locations must not exceed 250 bytes. Note: If an civic address location is non-empty it uses the amount of characters plus addition two characters. This is described in TIA1057, Section 10.2.4.3.2.";

    static constexpr const char *index_description =
        "Each civic address type as defined in TIA1057, Section 3.4 in Annex B";


    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_lldp_med_catype_t &i) {
        serialize(h, LLDP_catype_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_lldpmed_ca_type_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(lldpmed_conf_location_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_lldp_med_catype_itr);
    VTSS_EXPOSE_SET_PTR(lldpmed_conf_location_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LllpConfigGlobal {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_lldp_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_lldp_global_conf_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_lldp_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_lldp_global_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_LLDP);
};

struct LllpGlobalStat {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_lldp_global_counters_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_lldp_global_counters_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_lldp_stat_global_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_LLDP);
};

}  // namespace interfaces
}  // namespace topo
}  // namespace appl
}  // namespace vtss


/****************************************************************************
  Add more mib related details to the mib
****************************************************************************/

#endif
