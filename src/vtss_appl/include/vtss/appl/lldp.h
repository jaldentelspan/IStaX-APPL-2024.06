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
/**
 * \file
 * \brief Public LLDP API
 * \details This header file describes LLDP (Link layer protocol) and LLDP-MED (LLDP-MEDIA) control functions and types.
 *          LLDP is used to exchange information between network devices by sending device information to the link partners
 *          at each interface. The link partners are call neighbors or remote devices. LLDP is defined in IEEE 802.1AB.
 *          LLDP-MED is an extension to the LLDP standard which is defined in TIA1057.
 */

#ifndef _VTSS_APPL_LLDP_H_
#define _VTSS_APPL_LLDP_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/module_id.h>
#include <vtss/basics/enum-descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

extern vtss_enum_descriptor_t vtss_lldp_admin_state_txt[];  /*!< Enum descriptor text */
extern vtss_enum_descriptor_t vtss_lldp_cdp_aware_txt[];    /*!< Enum descriptor text */
extern vtss_enum_descriptor_t vtss_lldp_notification_txt[]; /*!< Enum descriptor text */
extern vtss_enum_descriptor_t lldpmed_latitude_dir_txt[];   /*!< Enum descriptor for latitude direction*/

/** Definition of error return codes return by the function in case of malfunctioning behavior*/
enum {
    VTSS_APPL_LLDP_ERROR_ISID =  MODULE_ERROR_START(VTSS_MODULE_ID_LLDP), /**< Invalid switch id */
    VTSS_APPL_LLDP_ERROR_ISID_LOCAL,                                      /**< Switch ID must not be VTSS_ISID_LOCAL.*/
    VTSS_APPL_LLDP_ERROR_VOICE_VLAN_CONFIGURATION_MISMATCH,               /**< Cannot set LLDP port mode to disabled or TX only when Voice-VLAN supports LLDP discovery protocol.*/
    VTSS_APPL_LLDP_ERROR_TX_DELAY,                                        /**< txDelay must not be larger than 0.25 * TxInterval - IEEE 802.1AB-clause 10.5.4.2.*/
    VTSS_APPL_LLDP_ERROR_NOTIFICATION_VALUE,                              /**< Notification enable - must be either 1 (enable) or 2 (disable).*/
    VTSS_APPL_LLDP_ERROR_NOTIFICATION_INTERVAL_VALUE,                     /**< Notification interval value must be between 5 and 3600 seconds.*/
    VTSS_APPL_LLDP_ERROR_NULL_POINTER,                                    /**< Unexpected reference to NULL pointer.*/
    VTSS_APPL_LLDP_ERROR_REINIT_VALUE,                                    /**< ReInit value was out of range.*/
    VTSS_APPL_LLDP_ERROR_TX_HOLD_VALUE,                                   /**< TX-hold value was out of range.*/
    VTSS_APPL_LLDP_ERROR_TX_INTERVAL_VALUE,                               /**< TX-Interval value was out of range.*/
    VTSS_APPL_LLDP_ERROR_TX_DELAY_VALUE,                                  /**< TX-Delay value was out of range.*/
    VTSS_APPL_LLDP_ERROR_FAST_START_REPEAT_COUNT,                         /**< Fast start repeat count value was out of range.*/
    VTSS_APPL_LLDP_ERROR_LATITUDE_OUT_OF_RANGE,                           /**< Latitude is out of range.*/
    VTSS_APPL_LLDP_ERROR_LONGITUDE_OUT_OF_RANGE,                          /**< Longitude is out of range.*/
    VTSS_APPL_LLDP_ERROR_ALTITUDE_OUT_OF_RANGE,                           /**< Altitude is out of range.*/
    VTSS_APPL_LLDP_ERROR_UNSUPPORTED_OPTIONAL_TLVS_BITS,                  /**< Trying to set optional TLVs bit(s) which is not supported*/
    VTSS_APPL_LLDP_ERROR_COUNTRY_CODE_LETTER_SIZE,                        /**< Country code must be 2 letters. From TIA1057, ANNEX B. - country code: The two-letter ISO 3166 country code in capital ASCII letters, e.g., DE or US.*/
    VTSS_APPL_LLDP_ERROR_COUNTRY_CODE_LETTER,                             /**< Country code contains non-letter character or is not capitalized. From TIA1057, ANNEX B. - country code: The two-letter ISO 3166 country code in capital ASCII letters, e.g., DE or US.*/
    VTSS_APPL_LLDP_ERROR_ELIN_SIZE,                                       /**< ECS ELIN must maximum be 25 character long. TIA1057, Figure 11.*/
    VTSS_APPL_LLDP_ERROR_ELIN,                                            /**< ECS ELIN must be a numerical string. TIA1057, Figure 11.*/
    VTSS_APPL_LLDP_ERROR_ENTRY_INDEX,                                     /**< Entry with the specific index is not in use.*/
    VTSS_APPL_LLDP_ERROR_CIVIC_EXCEED,                                    /**< Amount of Civic information exceeded.*/
    VTSS_APPL_LLDP_ERROR_CIVIC_TYPE,                                      /**< Invalid civic address type.*/
    VTSS_APPL_LLDP_ERROR_IFINDEX,                                         /**< Interface index (ifindex) is not a port index.*/
    VTSS_APPL_LLDP_ERROR_POLICY_OUT_OF_RANGE,                             /**< Policy index is out of range.*/
    VTSS_APPL_LLDP_ERROR_POLICY_NOT_DEFINED,                              /**< Trying to assign a undefined policy to an interface.*/
    VTSS_APPL_LLDP_ERROR_MGMT_ADDR_NOT_FOUND,                             /**< No management address with the given index found in the entry table*/
    VTSS_APPL_LLDP_ERROR_MGMT_ADDR_INDEX_INVALID,                         /**< The management address index is invalid*/
    VTSS_APPL_LLDP_ERROR_POE_NOT_VALID,                                   /**< PoE information is not valid.*/
    VTSS_APPL_LLDP_ERROR_POLICY_VLAN_OUT_OF_RANGE,                        /**< Policy VLAN out of valid range (VTSS_APPL_VLAN_ID_MIN-VTSS_APPL_VLAN_ID_MAX).*/
    VTSS_APPL_LLDP_ERROR_POLICY_DSCP_OUT_OF_RANGE,                        /**< Policy DSCP out of valid range (VTSS_APPL_LLDP_MED_DSCP_MIN-VTSS_APPL_LLDP_MED_DSCP_MAX).*/
    VTSS_APPL_LLDP_ERROR_POLICY_PRIO_OUT_OF_RANGE,                        /**< Policy Priority out of valid range (VTSS_APPL_LLDP_MED_L2_PRIORITY_MIN-VTSS_APPL_LLDP_MED_L2_PRIORITY_MAX).*/
    VTSS_APPL_LLDP_ERROR_MSG,                                             /**< Internal message issue.*/
};

/*
 * LLDP types and defines
 */

/** \brief LLDP configuration that are global for the whole switch/stack */
typedef struct {
    /** Each LLDP frame contains information about how long time the information in the LLDP frame shall be considered valid.
        The LLDP information valid period is set to msgTxHold multiplied by msgTxInterval seconds.
        Ref. IEEE 802.1AB-2005, Section 10.5.3.3, bullet a. */
    uint16_t msgTxHold;

    /** The switch periodically transmits LLDP frames to its neighbors for having the network discovery information up-to-date.
        The interval in seconds between each LLDP frame is determined by the msgTxInterval value.
        ref. IEEE 802.1AB-2005, Section 10.5.3.3, bullet b.  */
    uint16_t msgTxInterval;

    /** When a port is disabled, LLDP is disabled or the switch is rebooted, a LLDP shutdown frame is transmitted to
        the neighboring units, signaling that the LLDP information isn't valid anymore.
        reInitDelay controls the amount of seconds between the shutdown frame and a new LLDP initialization.
        Ref. IEEE 802.1AB-2005, Section 10.5.3.3, bullet c.*/
    uint16_t reInitDelay;

    /** If some configuration is changed (e.g. the IP address) a new LLDP frame is transmitted, but the time between the
        LLDP frames will always be at least txDelay seconds.
        Ref. IEEE 802.1AB-2005, Section 10.5.3.3, bullet d. */
    uint16_t txDelay;
} vtss_appl_lldp_global_conf_t;

/** \brief LLDP-MED remote device type - All devices in a network is divided types. The types are defined in  Table 11, TIA1057*/
typedef enum {
    LLDPMED_DEVICE_TYPE_NOT_DEFINED          = 0,
    LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_I     = 1,
    LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_II    = 2,
    LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_III   = 3,
    LLDPMED_DEVICE_TYPE_NETWORK_CONNECTIVITY = 4,
    LLDPMED_DEVICE_TYPE_RESERVED             = 5
} vtss_appl_lldp_med_remote_device_type_t;

/** Admin state for LLDP, Section 10.5.1, IEEE802.1AB-2005*/
typedef enum {
    VTSS_APPL_LLDP_DISABLED,         /**< LLDP frames not transmitted, received frames are ignored*/

    /** LLDP frames transmitted, LLDP information from received frames are updated to the LLDP entries table*/
    VTSS_APPL_LLDP_ENABLED_RX_TX,

    VTSS_APPL_LLDP_ENABLED_TX_ONLY,  /**< LLDP frames transmitted, received frames are ignored*/

    /** LLDP frames not transmitted, LLDP information from received frames are updated to the LLDP entries table*/
    VTSS_APPL_LLDP_ENABLED_RX_ONLY,
} vtss_appl_lldp_admin_state_t;

/** \brief LLDP-MED operation mode. This the operation type that the device shall work as (Device type) */
typedef enum {
    VTSS_APPL_LLDP_MED_CONNECTIVITY, /**< Device shall operate as a connectivity device*/
    VTSS_APPL_LLDP_MED_END_POINT     /**< Device shall operate as a class I endpoint device*/
} vtss_appl_lldp_med_device_type_t;

/** LLDP capabilities - Internal constants which can become handy for an application.*/
typedef struct {
    /** Number of remote entries (neighbors information) the internal neighbor entries table can contain.*/
    uint32_t remote_entries_cnt;

    /** Number of policies supported*/
    uint32_t policies_cnt;
} vtss_appl_lldp_cap_t;

/** For some of the LLDP information it is optional it shall be transmitted. The optional TLVs are
    shown in Table 9-1, IEEE802.1AB*/
typedef enum {
    VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_PORT_DESCR_BIT   = 1 << 0, /**< Port Description Optional Clause 9.5.5, IEEE802.1AB*/
    VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_NAME_BIT  = 1 << 1, /**< System Name Optional Clause 9.5.6, IEEE802.1AB*/
    VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_DESCR_BIT = 1 << 2, /**< System Description Optional Clause 9.5.7, IEEE802.1AB*/
    VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_CAPA_BIT  = 1 << 3, /**< System Capabilities Optional Clause 9.5.8, IEEE802.1AB*/
    VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_ADDR_BIT         = 1 << 4, /**< Management Address Optional Clause 9.5.9, IEEE802.1AB*/
    VTSS_APPL_LLDP_TLV_OPTIONAL_ALL_BITS              = 31      /**< Mask with all the bits supported set */
} vtss_appl_lldp_optional_tlvs_t;

/** For some of the LLDP-MED information it is optional it shall be transmitted. The optional TLVs are
    shown in Table 5, TIA1057.*/
typedef enum {
    /** Bit 0 is the capabilities TLV, TIA-1057, MIB lldpXMedPortConfigTLVsTxEnable*/
    VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT = 1 << 0,

    /** Bit 1 is the network Policy TLV, TIA-1057, lldpXMedPortConfigTLVsTxEnable MIB.*/
    VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT       = 1 << 1,

    /** Bit 2 is the location TLV, TIA-1057, lldpXMedPortConfigTLVsTxEnable MIB.*/
    VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT     = 1 << 2,

    /** Bit 3 is the lextendedPSE TLV, TIA-1057, lldpXMedPortConfigTLVsTxEnable MIB.*/
    VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT          = 1 << 3,
} lldpmed_optional_tlv_bits_t;

/** LLDP Configuration that applies for an interface/port */
typedef struct {
    vtss_appl_lldp_admin_state_t  admin_states;  /**< Port administration state (transmitting/receiving)*/

    /** If set to TRUE received CDP (Cisco Discovery Protocol) frames are transformed into LLDP information and added to the
        neighbor entries table.
        If set to FALSE CDP Frames are ignored and forwarded.*/
    mesa_bool_t  cdp_aware;

    /** Bit mask matching vtss_appl_lldp_optional_tlvs_t for enabling/disabling of optional TLVs*/
    uint8_t  optional_tlvs_mask;

    mesa_bool_t snmp_notification_ena;   /**< Enable/disable of SNMP Trap notification*/

    /** Bit mask matching lldpmed_optional_tlv_bits_t for selecting which of the optional TLVs to Enable (1) or disable (o)*/
    uint8_t   lldpmed_optional_tlvs_mask;

    mesa_bool_t lldpmed_snmp_notification_ena; /**< Enable/Disable of SNMP Trap notification for LLDP-MED*/

    /** Operation mode (connectivity/end-point) that the port/interface shall operate as*/
    vtss_appl_lldp_med_device_type_t lldpmed_device_type;
} vtss_appl_lldp_port_conf_t;

#define VTSS_APPL_MAX_CHASSIS_ID_LENGTH 255   /**< Maximum length of the chassis id, Figure 9-4, IEEE802.1AB-2005*/
#define VTSS_APPL_MAX_PORT_ID_LENGTH 255      /**< Maximum length of port id, Figure 9-5, IEEE802.1AB-205*/
#define VTSS_APPL_MAX_PORT_DESCR_LENGTH 255   /**< Maximum length of port_description, Figure 9-7, IEEE802.1AB-2005*/
#define VTSS_APPL_MAX_SYSTEM_NAME_LENGTH 255  /**< Maximum length of system name, Figure 9-8, IEEE802.1AB-2005*/
#define VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH 255 /**< Maximum length of system description, Figure 9-9, IEEE802.1AB-2005*/
#define VTSS_APPL_MAX_MGMT_ADDR_LENGTH 31     /**< Maximum length of management address, Figure 9-11, IEEE802.1AB-2005*/
#define VTSS_APPL_MAX_OID_LENGTH 128          /**< Maximum length of OID, Figure 9-11, IEEE802.1AB-2005*/
#define VTSS_APPL_MAX_ORG_LENGTH 507          /**< Maximum length of Oganizationally information string, Figure 9-12, IEEE802.1AB-2005*/
#define VTSS_APPL_MAX_MGMT_LENGTH 167         /**< Maximum length of management address information string, Figure 9-11, IEEE802.1AB-2005*/

typedef uint32_t vtss_appl_lldp_counter_t;         /**< LLDP statistic counters */

/*
 * LLDP types and defines
 */

/** \brief LLDP-MED location latitude direction as specified in RFC3825*/
typedef enum {
    NORTH = 0,
    SOUTH = 1,
} vtss_appl_lldp_med_latitude_dir_t;

/** \brief LLDP-MED location longitude direction as specified in RFC3825*/
typedef enum {
    EAST = 0,
    WEST = 1,
} vtss_appl_lldp_med_longitude_dir_t;

/** \brief Datum which a geodetic systems or geodetic data are used in geodesy, navigation, surveying by cartographers
    and satellite navigation systems to translate positions indicated on the products to their real position on earth.
    Defined in RFC3825,July 2004, section 2.1*/
typedef enum {
    DATUM_UNDEF  = 0,
    WGS84        = 1,
    NAD83_NAVD88 = 2,
    NAD83_MLLW   = 3,
} vtss_appl_lldp_med_datum_t;

/** \brief Types for altitude,  meters or floor, RFC3825,July2004 section 2.1*/
typedef enum {
    AT_TYPE_UNDEF = 0,
    METERS    = 1,
    FLOOR     = 2,
} vtss_appl_lldp_med_at_type_t;


#define VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_MIN 1     /**< Fast Start Repeat Count minimum value, TIA1057, medFastStartRepeatCount MIB*/
#define VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_MAX 10    /**< Fast Start Repeat Count maximum value, TIA1057, medFastStartRepeatCount MIB*/
#define VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_DEFAULT 4 /**< Fast Start Repeat Count default value, TIA1057, Section 12.1, bullet c)*/

/** \brief LLDP-MED Location information - CA (Civic Address) type, Section 3.4 in Annex B, TIA1057*/
typedef enum {
    LLDPMED_CATYPE_UNDEFINED = 0, /**< Undefined */
    LLDPMED_CATYPE_A1       = 1,  /**< State/National subdivisions*/
    LLDPMED_CATYPE_A2       = 2,  /**< County, parish, gun (JP), district (IN)*/
    LLDPMED_CATYPE_A3       = 3,  /**< City, township,*/
    LLDPMED_CATYPE_A4       = 4,  /**< City division, borough, city district, ward, chou (JP)*/
    LLDPMED_CATYPE_A5       = 5,  /**< Neighborhood, block*/
    LLDPMED_CATYPE_A6       = 6,  /**< Street*/
    LLDPMED_CATYPE_PRD      = 16, /**< Leading street direction*/
    LLDPMED_CATYPE_POD      = 17, /**< Trailing street direction*/
    LLDPMED_CATYPE_STS      = 18, /**< Street suffix*/
    LLDPMED_CATYPE_HNO      = 19, /**< House number*/
    LLDPMED_CATYPE_HNS      = 20, /**< House number suffix*/
    LLDPMED_CATYPE_LMK      = 21, /**< Landmark or vanity address*/
    LLDPMED_CATYPE_LOC      = 22, /**< Additional location information*/
    LLDPMED_CATYPE_NAM      = 23, /**< Name*/
    LLDPMED_CATYPE_ZIP      = 24, /**< Postal/zip code*/
    LLDPMED_CATYPE_BUILD    = 25, /**< Building*/
    LLDPMED_CATYPE_UNIT     = 26, /**< Unit*/
    LLDPMED_CATYPE_FLR      = 27, /**< Floor*/
    LLDPMED_CATYPE_ROOM     = 28, /**< Room*/
    LLDPMED_CATYPE_PLACE    = 29, /**< Place type*/
    LLDPMED_CATYPE_PCN      = 30, /**< Postal*/
    LLDPMED_CATYPE_POBOX    = 31, /**< Post office*/
    LLDPMED_CATYPE_ADD_CODE = 32  /**< Additional code*/
} vtss_appl_lldp_med_catype_t;

/**Max. string length for Country Code (including NULL termination), Figure 10, TIA1057*/
#define VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN 3

/** Number of elements in vtss_appl_lldp_med_catype_t. Keep this in sync with the vtss_appl_lldp_med_catype_t (Not likely to change unless the standard changes)*/
#define VTSS_APPL_LLDP_MED_CATYPE_CNT 23

/**Max. string length for a civic address, Figure 10, TIA1057*/
#define VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX 250

/**Max. string length for a civic address including ca-type and length, Figure 10, TIA1057*/
#define VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_INCL_TYPE_MAX 252

/** \brief type for holding the individual civic addresses in a stuct*/
typedef struct {
    // Location information all the CA (Civic Address) types, Section 3.4 in Annex B, TIA1057*/
    char ca_country_code[VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN]; /**< Location country code string, Figure 10, TIA1057*/
    char a1[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];           /**< National subdivisions*/
    char a2[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];           /**< County, parish, gun (JP), district (IN)*/
    char a3[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];           /**< City, township,*/
    char a4[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];           /**< City division, borough, city district, ward, chou (JP)*/
    char a5[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];           /**< Neighborhood, block*/
    char a6[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];           /**< Street*/
    char prd[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Leading street direction*/
    char pod[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Trailing street direction*/
    char sts[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Street suffix*/
    char hno[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< House number*/
    char hns[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< House number suffix*/
    char lmk[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Landmark or vanity address*/
    char loc[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Additional location information*/
    char nam[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Name*/
    char zip[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Postal/zip code*/
    char build[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];        /**< Building*/
    char unit[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];         /**< Unit*/
    char flr[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Floor*/
    char room[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];         /**< Room*/
    char place[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];        /**< Place type*/
    char pcn[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];          /**< Postal*/
    char pobox[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];        /**< Post office*/
    char add_code[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];     /**< Additional code*/
} vtss_appl_lldpmed_civic_t;

/** \brief The civic location is a set of multiple strings. One string for each CA type.
 *
 * \details
 *   In principle all strings can all be of up to 250 bytes long, but all together they
 *   must also be within 250 bytes (Figure 10, TIA1057)
 *   In order not to reserve a lot of space in flash and memory, all the individual strings are all concatenated within one single string.
 *   Each individual string is located with an offset in the concatenated string
 *
 *  Don't use this directly but use the
 *  vtss_appl_lldp_location_civic_info_get and vtss_appl_lldp_location_civic_info_set functions.
 */
typedef struct {
    /**  Single string containing all the concatenated civic strings stored as defined in TIA1057 (250 bytes, Figure 10, TIA1057). This is used instead of vtss_appl_lldpmed_civic_t in order to save memory*/
    char ca_value[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_INCL_TYPE_MAX];
} vtss_appl_lldp_med_civic_tlv_format_t;

#define VTSS_APPL_CIVIC_CA_TYPE_NOT_IN_USE -1  /**< Used to signal that the CA TYPE in the civic_ca_type_array is un-used */

/** Emergency call service maximum length, Figure 11, TIA1057*/
#define VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX 25

/** \brief LLDP-MED location from remote neighbor*/
typedef struct {
    // Coordinated based location, Figure 9, TIA1057
    uint8_t  latitude_res;                                   /**< Latitude resolution*/
    int64_t latitude;                                       /**< Latitude degrees - stored in 2s-complement as specified in RFC 3825*/
    vtss_appl_lldp_med_latitude_dir_t latitude_dir;     /**< Latitude direction, 0 = North, 1 = South*/
    uint8_t  longitude_res;                                  /**< Longitude resolution*/
    int64_t longitude;                                      /**< Longitude degrees - stored in 2s-complement as specified in RFC 3825*/
    vtss_appl_lldp_med_longitude_dir_t longitude_dir;   /**< Latitude direction, 0 = East, 1 = West*/
    uint8_t  altitude_res;                                   /**< Altitude resolution - stored in 2s-complement as specified in RFC 3825*/
    int32_t altitude;                                       /**< Altitude in meter or floors*/
    vtss_appl_lldp_med_at_type_t altitude_type;         /**< Coordinate based location - RFC3825, 1 = meters, 2 =floors*/
    vtss_appl_lldp_med_datum_t datum;                   /**< Datum*/
} vtss_appl_lldp_med_location_info_t;

/** \brief LLDP-MED location configuration*/
typedef struct {
    // Coordinated based location, Figure 9, TIA1057
    int64_t latitude;                                       /**< Latitude degrees - stored in 2s-complement as specified in RFC 3825 with a resolution of 34*/
    int64_t longitude;                                      /**< Longitude degrees - stored in 2s-complement as specified in RFC 3825*with a resolution of 34*/
    int32_t altitude;                                       /**< Altitude in meter or floors*/
    vtss_appl_lldp_med_at_type_t altitude_type;         /**< Coordinate based location - RFC3825, 1 = meters, 2 =floors*/
    vtss_appl_lldp_med_datum_t datum;                   /**< Datum*/
} vtss_appl_lldp_med_location_conf_t;

/** brief LLDP-MED TLV application types, Table 12,TIA1057*/
typedef enum {
    VOICE                 = 1,
    VOICE_SIGNALING       = 2,
    GUEST_VOICE           = 3,
    GUEST_VOICE_SIGNALING = 4,
    SOFTPHONE_VOICE       = 5,
    VIDEO_CONFERENCING    = 6,
    STREAMING_VIDEO       = 7,
    VIDEO_SIGNALING       = 8,
} vtss_appl_lldp_med_application_type_t;


/** Max Location ID data length in table 14,TIA1057*/
#define VTSS_APPL_LLDP_MED_LOCATION_LEN_MAX 256

typedef uint32_t vtss_lldp_entry_index_t;      /**< Type for indexing of the entries table*/
typedef uint32_t vtss_lldpmed_policy_index_t;   /**< Type for indexing polcies*/

/** Max inventory string length. See all inventory TLVs in TIA1057, Section 10.2.6*/
#define VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX 32

/** \brief LLDP-MED network policies parameters, section 10.2.3, TIA1057*/
typedef struct {
    vtss_appl_lldp_med_application_type_t application_type; /**< Application type */
    mesa_bool_t   tagged_flag;                                     /**< Vlan tagged */

    /** Unknown Policy - Note: Always set to TRUE for frames transmitted by this device*/
    mesa_bool_t   unknown_policy_flag;

    uint16_t    vlan_id;                                         /**< Vlan number */
    uint8_t     l2_priority;                                     /**< l2 priority */
    uint8_t     dscp_value ;                                     /**< dscp */
} vtss_appl_lldp_med_network_policy_t;

/** \brief, Network policy with indication of if it is in use, section 10.2.3, TIA1057*/
typedef struct {
    vtss_appl_lldp_med_network_policy_t network_policy; /**< The network policy*/
    mesa_bool_t   in_use;                                      /**< Signaling if this policy is in use.*/
} vtss_appl_lldp_med_policy_t;

/** \brief Configuration that are common for all interfaces*/
typedef struct {
    /** Transmit state machine timing parameters, IEEE 802.1AB, section 10.5.3.3*/
    vtss_appl_lldp_global_conf_t tx_sm;

    vtss_appl_lldp_med_location_conf_t  coordinate_location; /**< Location information*/
    uint8_t   medFastStartRepeatCount;                            /**< Fast Start Repeat Count, Section 11.2.1 TIA1057*/

    /** Max one snmp trap with this interval. See the LLDP MIB for further information.*/
    int  snmp_notification_interval;

    /** Emergency call service, Figure 11, TIA1057. Adding 1 for making space for "\0"*/
    char  elin_location[VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX + 1];

    char ca_country_code[VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN]; /**< Location country code string, Figure 10, TIA1057*/

    vtss_appl_lldp_med_civic_tlv_format_t civic;           /**< Civic Address Location information*/
} vtss_appl_lldp_common_conf_t;


/** \brief Statistics counters for an interface/port*/
typedef struct {
    vtss_appl_lldp_counter_t statsFramesOutTotal;           /**< Total LLDP frames transmitted*/
    vtss_appl_lldp_counter_t statsFramesInTotal;            /**< Total LLDP frames received*/
    vtss_appl_lldp_counter_t statsFramesInErrorsTotal;      /**< The number of received LLDP frames containing some kind of error.*/

    /**Each LLDP frame contains information about how long time the LLDP information is valid (age-out time).
       If no new LLDP frame is received within the age out time, the LLDP information is removed from the neighbor entries table,
       and the Age-Out counter is incremented. */
    vtss_appl_lldp_counter_t statsAgeoutsTotal;

    /** If a LLDP frame is received at a port, and the switch's entries table is full, the frame is counted and discarded.*/
    vtss_appl_lldp_counter_t statsFramesDiscardedTotal;

    /**Each LLDP frame can contain multiple pieces of information, known as TLVs (TLV is short for "Type Length Value").*/
    vtss_appl_lldp_counter_t statsTLVsDiscardedTotal;     /**<If a TLV is malformed, it is counted and discarded.*/
    vtss_appl_lldp_counter_t statsTLVsUnrecognizedTotal;  /**<If a TLV is unrecognized it is counted and discarded.*/

    /**If LLDP frame is received with a valid organizationally TLV, but the TLV is not supported,
       the TLV is discarded and counted.*/
    vtss_appl_lldp_counter_t statsOrgTVLsDiscarded;
} vtss_appl_lldp_port_counters_t;

/** \brief Statistics counters that are global for the switch stack*/
typedef struct {
    /** The number of times remote neighbor information has been added to the entries table,
        lldpStatsRemTablesInserts in IEEE 802.1AB-2005*/
    vtss_appl_lldp_counter_t table_inserts;

    /** The number of times remote neighbor information has been removed from the entries table,
        lldpStatsRemTablesDeletes in IEEE 802.1AB-2005*/
    vtss_appl_lldp_counter_t table_deletes;

    /** The number of times remote neighbor information was not added to the entries table because the table was full,
        lldpStatsRemTablesDrops IEEE 802.1AB-2005*/
    vtss_appl_lldp_counter_t table_drops;

    /** The number of times remote neighbor information has been removed from the entries table due to age-out,
        lldpStatsRemTablesAgeouts IEEE 802.1AB-2005*/
    vtss_appl_lldp_counter_t table_ageouts;

    /** The number of seconds since an entry has changed.
        Used for lldpStatsRemTablesLastChangeTimelldpStatsRemTablesLastChangeTime, IEEE802.1AB-2005*/
    uint64_t              last_change_ago;

} vtss_appl_lldp_global_counters_t;


/** Number of defined policy applications - Table 12 in TIA1057
    Multiple policies (applications) TLVs shall be supported (Section 10.2.3.1, TIA1057), so we have to define a
    number of Policy fields in the entry. This can be limited to the number of applications defined (Section 10.2.3.8, TIA1057).*/
#define VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT 9

/** \brief EEE (Energy Efficient Ethernet) TLV*/
typedef struct {
    mesa_bool_t valid;          /**< TRUE when the rest of the EEE fields contains valid information*/
    /** Transmit Tw_sys_tx (2 octets wide) shall be defined as the time (expressed in microseconds) that the
        transmitting link partner will wait before it starts transmitting data after leaving the Low Power Idle (LPI) mode.
        Section 79.3.5.1, IEEE802.3az*/
    uint16_t  RemTxTwSys;

    /** Receive Tw_sys_tx (2 octets wide) shall be defined as the time (expressed in microseconds) that the receiving
        link partner is requesting the transmitting link partner to wait before starting the transmission data following
        the LPI. Section 79.3.5.2, IEEE802.3az*/
    uint16_t  RemRxTwSys;

    /** Fall back value in case that the neighbor doesn't match the same setup time, Section 79.3.5.3, IEEE802.3az*/
    uint16_t  RemFbTwSys;

    /** The respective echo values shall be defined as the local link partners reflection (echo) of the remote link
        partners respective values. Section 79.3.5.4, IEEE802.3az*/
    uint16_t  RemTxTwSysEcho;
    uint16_t  RemRxTwSysEcho; /**< Same as RemTxTwSysEcho, just for rx*/
} vtss_appl_lldp_eee_t;

/** \brief Frame preemption TLV*/
typedef struct {
   mesa_bool_t RemFramePreemptSupported; /**< Frame preemption capability, Table 79-7a, IEEEP802.3brD2.0*/
   mesa_bool_t RemFramePreemptEnabled;   /**< Frame preemption status, Table 79-7a, IEEEP802.3brD2.0*/
   mesa_bool_t RemFramePreemptActive;    /**< TRUE when Frame preemption is active, Table 79-7a, IEEEP802.3brD2.0*/
   /** Minimum number of octets over 64 octets required in non-final fragments by the receiver, Table 79-7a, IEEEP802.3brD2.0*/
   uint8_t   RemFrameAddFragSize;
} vtss_appl_lldp_fp_t;


/**  IEEE 802.1AB-2005 Section 9.5.9.9 Bullet a) and b) says that multiple management addresses are supported, but not how many.
     For now we say multiple = 2.*/
#define LLDP_MGMT_ADDR_CNT 2

/** \brief Management address TLV*/
typedef struct {
    uint8_t   subtype;                       /**< Management address subtype , section 9.5.9.3 IEEE802.1AB-2005*/
    uint8_t   length;                        /**< Management address string length, section 9.5.9.2 IEEE802.1AB-2005*/
    char mgmt_address[VTSS_APPL_MAX_MGMT_ADDR_LENGTH];  /**< Management address string, section 9.5.9.4 IEEE802.1AB-2005*/
    uint8_t   if_number_subtype;             /**< Interface numbering subtype, section 9.5.9.5 IEEE802.1AB-2005*/
    uint8_t   if_number[4];                  /**< Interface number, section 9.5.9.6 IEEE802.1AB-2005*/
    uint8_t   oid_length;                    /**< Object identifier string length, section 9.5.9.7 IEEE802.1AB-2005*/
    char     oid[VTSS_APPL_MAX_OID_LENGTH]; /**< Object identifier, section 9.5.9.8 IEEE802.1AB-2005*/
} vtss_appl_lldp_mgmt_addr_tlv_t;

/** \brief All remote neighbor's information are stored in a entries table. This is a single entry*/
typedef struct {
    mesa_bool_t in_use;               /**< TRUE if the entry contains valid information*/

    uint32_t  entry_index;          /**< The entry index of this entry in the entry table*/

    mesa_port_no_t  receive_port; /**< The interface/port at which the entry was received*/

    int8_t   smac[6];              /**< SMAC Address - Not part of the LLDP standard, but needed for Voice Vlan*/
    uint16_t  lldp_remote_index;    /**< The index in the entries table*/
    uint32_t  time_mark;            /**< The time at which the entry was added to the table*/

    /* The following fields are "data fields" with received data */
    uint8_t   chassis_id_subtype;                            /**< Chassis ID subtype, Section 9.5.2.2 IEEE802.1AB-2005*/
    uint8_t   chassis_id_length;                             /**< Chassis string ID length, Section 9.5.2.1 IEEE802.1AB-2005*/
    char chassis_id[VTSS_APPL_MAX_CHASSIS_ID_LENGTH];   /**< Chassis ID, Section 9.5.2.3 IEEE802.1AB-2005*/

    uint8_t   port_id_subtype;                               /**< Port ID subtype, Section 9.5.3.2 IEEE802.1AB-2005*/
    uint8_t   port_id_length;                                /**< Port string ID length, Section 9.5.3.1 IEEE802.1AB-2005*/
    char port_id[VTSS_APPL_MAX_PORT_ID_LENGTH];         /**< Port ID, Section 9.5.3.3 IEEE802.1AB-2005*/

    uint8_t   port_description_length;                       /**< Port description string length, Section 9.5.5.1 IEEE802.1AB-2005*/
    char port_description[VTSS_APPL_MAX_PORT_DESCR_LENGTH]; /**< Port description, Section 9.5.5.2 IEEE802.1AB-2005*/

    uint8_t   system_name_length;                            /**< System Name string length, Section 9.5.6.1 IEEE802.1AB-2005*/
    char system_name[VTSS_APPL_MAX_SYSTEM_NAME_LENGTH]; /**< System Name, Section 9.5.6.2 IEEE802.1AB-2005*/

    uint8_t   system_description_length;                     /**< System description string length, Section 9.5.7.1 IEEE802.1AB-2005*/
    char system_description[VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH]; /**< System description, Section 9.5.7.2 IEEE802.1AB-2005*/

    uint8_t   system_capabilities[4];                        /**< System capabilities, Section 9.5.8.1 IEEE802.1AB-2005*/

    vtss_appl_lldp_mgmt_addr_tlv_t mgmt_addr[LLDP_MGMT_ADDR_CNT]; /**< Management address, section 9.5.9 IEEE802.1AB-2005*/

    /** PoE - Power Over Ethernet*/
    /** organizationally TLV for PoE - Figure 33-26 in IEEE802.3at/D3*/

    /**< Because there already are equipment that supports earlier versions of the IEEE standard,
       we have to support multiple versions*/
    uint8_t   ieee_draft_version;

    uint8_t   poe_info_valid_len;          /**< length > 0 if the PoE fields below contains valid information.*/
    uint8_t   requested_type_source_prio;  /**< Requested source type priority, Table 33-22 in IEEE802.3at/D3*/
    uint16_t  requested_power;             /**< Requested power, Table 33-23 in IEEE802.3at/D3 +  Figure 79-2, IEEE802.3at/D4*/
    uint8_t   actual_type_source_prio;     /**< The actually source type priority, Figure 33-26 in IEEE801.3at/D3*/
    uint16_t  actual_power;                /**< The actually power, Figure 33-26 in IEEE801.3at/D3 +  Figure 79-2, IEEE802.3at/D4*/

    uint8_t   poe_mdi_power_support;       /**< MDI power capabilities, Table 79-2, IEEE801.3at/D4*/
    uint8_t   poe_pse_power_pair;          /**< Value pethPsePortPowerPairs in IETF RFC 3621, Table 79-2, IEEE801.3at/D4*/
    uint8_t   poe_power_class;             /**< Value pethPsePortPowerClassifications in IETF RFC 3621, Table 79-2, IEEE801.3at/D4*/

    /** organizationally TLV for PoE - Figure 33-26 in IEEE802.3bt/D3*/
    uint16_t  requested_power_mode_a;     /**< Requested power for mode a, Table 79-6a-Dual-signature PD in IEEE801.3bt/D3 */
    uint16_t  requested_power_mode_b;     /**< Requested power for mode b, Table 79-6a-Dual-signature PD in IEEE801.3bt/D3 */
    uint16_t  pse_alloc_power_alt_a;      /**< Allocated power for alternative a, Table 79-6b-PSE allocated power in IEEE801.3bt/D3 */
    uint16_t  pse_alloc_power_alt_b;      /**< Allocated power for alternative b, Table 79-6b-PSE allocated power in IEEE801.3bt/D3 */
    uint16_t  power_status;               /**< Power status as defined in Table 79-6c-Power status field, in IEEE801.3bt/D3 */
    uint8_t   system_setup;               /**< System setup as defined in Table 79-6d-System setup field in IEEE801.3bt/D3 */
    uint16_t  pse_maximum_avail_power;    /**< PSE maximum available power as defined in Table 79-6e in IEEE801.3bt/D3 */
    uint8_t   auto_class;                 /**< Request for autoclass operation as defined in Table 79-6f in IEEE801.3bt/D3 */
    uint32_t  power_down;                 /**< Power down request as defined in Table 79-6g in IEEE801.3bt/D3 */
 
    /** TIA 1057*/
    uint8_t   tia_info_valid_len;          /**< Length > 0 if the PoE fields below contains valid information.*/
    uint8_t   tia_type_source_prio;        /**< PoE Power source priority, Section 10.2.5.3 TIA 1057*/
    uint16_t  tia_power;                   /**< PoE power value, section 10.2.5.4 TIA 1057*/

    /** Organizationally TLV*/
    mesa_bool_t lldporg_autoneg_vld;         /**< TRUE when the fields below contains valid data.*/
    uint8_t   lldporg_autoneg;             /**< Auto-negotiation Support/Status - Figure G-1, IEEE802.1AB*/
    uint16_t  lldporg_autoneg_advertised_capa; /**< Advertised auto negotiation capabilities, Figure G-1, IEEE802.1AB*/
    uint16_t  lldporg_operational_mau_type;/**< Mau Type, Figure G-1, IEEE802.1AB*/

    /** LLDP MEDIA TLVs*/
    mesa_bool_t lldpmed_info_vld;            /**< TRUE if any LLDP-MED TLV is received*/

    mesa_bool_t lldpmed_capabilities_vld;    /**< TRUE when the fields below contains valid data.*/
    uint16_t lldpmed_capabilities;         /**< LLDP-MED capabilities - Figure 6, TIA1057*/
    uint16_t lldpmed_capabilities_current; /**< LLDP-MED capabilities currently in use, See TIA1057, MIBS LLDPXMEDREMCAPCURRENT*/
    vtss_appl_lldp_med_remote_device_type_t lldpmed_device_type;   /**< Device type, section 10.2.2.2 TIA1057*/

    /** LLDP-MED Policy TLV - Figure 7,TIA1057*/
    vtss_appl_lldp_med_policy_t policy[VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT];

    /** LLDP-MED location TLV - Figure 8, TIA1057*/
    mesa_bool_t lldpmed_coordinate_location_vld;        /**< TRUE when location contains valid information*/

    /** LLDP-MED location information (latitude, longitude etc.) */
    vtss_appl_lldp_med_location_info_t coordinate_location;

    /** LLDP-MED location TLV - Figure 8, TIA1057*/
    mesa_bool_t lldpmed_civic_location_vld;   /**< TRUE when civic location contains valid information*/
    uint16_t  lldpmed_civic_location_length; /**< Civic Location string length, Section 10.2.4.1, TIA1057*/
    uint8_t   lldpmed_civic_location[VTSS_APPL_LLDP_MED_LOCATION_LEN_MAX + 1]; /**< Location, section 10.2.4.3 TIA1057*/

    /** LLDP-MED location TLV - Figure 8, TIA1057*/
    mesa_bool_t lldpmed_elin_location_vld;   /**< TRUE when civic location contains valid information*/

    /** Emergency call service, Figure 11, TIA1057. Adding 1 for making space for "\0"*/
    uint16_t  lldpmed_elin_location_length;     /**< ELIN Location string length*/
    uint8_t lldpmed_elin_location[VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX + 1]; /**< Location, section 10.2.4.3 TIA1057*/

    /** hardware revision TLV - Figure 13,  TIA1057*/
    uint8_t   lldpmed_hw_rev_length;                        /**< Hardware string length, Section 10.2.6.1.1 TIA1057*/
    char lldpmed_hw_rev[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX]; /**< Hardware string, Section 10.2.6.1.2 TIA1057*/

    /** firmware revision TLV - Figure 14,  TIA1057*/
    uint8_t    lldpmed_firm_rev_length;                         /**< Firmware revision string length, Section 10.2.6.2.1 TIA1057*/
    char  lldpmed_firm_rev[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];  /**< Firmware string, Section 10.2.6.2.2. TIA1057*/

    /** Software revision TLV - Figure 15,  TIA1057*/
    uint8_t    lldpmed_sw_rev_length;                          /**< Software revision string length, Section 10.2.6.3.1 TIA1057*/
    char  lldpmed_sw_rev[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];   /**< Software string, Section 10.2.6.3.2. TIA1057*/

    /** Serial number  TLV - Figure 16,  TIA1057*/
    uint8_t    lldpmed_serial_no_length;                       /**< Serial number string length, Section 10.2.6.4.1. TIA1057*/
    char  lldpmed_serial_no[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];/**< Serial number string, Section 10.2.6.4.2. TIA1057*/

    /** Manufacturer name TLV - Figure 17,  TIA1057*/
    uint8_t    lldpmed_manufacturer_name_length;               /**< Manufacturer Name string length, Section 10.2.6.5.1. TIA1057*/
    char  lldpmed_manufacturer_name[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX]; /**< Manufacturer Name string, Section 10.2.6.6.2. TIA1057*/

    /** Model Name TLV - Figure 18,  TIA1057*/
    uint8_t    lldpmed_model_name_length;                      /**< Model name string length, Section 10.2.6.5.1. TIA1057*/
    char  lldpmed_model_name[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];/**< Model name string, Section 10.2.6.6.2. TIA1057*/

    /** Asset ID TLV - Figure 19,  TIA1057*/
    uint8_t    lldpmed_asset_id_length;                              /**< Asset id string length, Section 10.2.6.7.1. TIA1057*/
    char  lldpmed_asset_id[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];/**< Asset id string, Section 10.2.6.7.2. TIA1057*/

    vtss_appl_lldp_eee_t eee;                                       /**< EEE (Energy Efficient Ethernet) TLV*/

    vtss_appl_lldp_fp_t fp;                                       /**< Frame Preemption TLV*/

    /** General information */
    uint16_t                   rx_info_ttl;                              /**< Time to live in seconds, before this entry is deleted*/
} vtss_appl_lldp_remote_entry_t;


/** Inventory types as define in Table 20, TIA1057*/
typedef enum {
    LLDPMED_HW_REV,              /**< Hardware revision*/
    LLDPMED_FW_REV,              /**< Firmware revision*/
    LLDPMED_SW_REV,              /**< Software revision*/
    LLDPMED_SER_NUM,             /**< Serial number*/
    LLDPMED_MANUFACTURER_NAME,   /**< Manufacturer name*/
    LLDPMED_MODEL_NAME,          /**< Model name*/
    LLDPMED_ASSET_ID,            /**< Asset Identifier number*/
} vtss_appl_lldp_med_inventory_type_t;


/*
 * LLDP management functions
 */
/**
 * Get configuration that are common for the whole switch/stack.
 *
 *  Purpose : To get the LLDP configuration that are common for all interfaces.
 *
 * \param common_conf [OUT] Pointer to where to put the configuration
 *
 *  \return VTSS_RC_OK if configuration returned is valid, else error code.
 */
mesa_rc vtss_appl_lldp_common_conf_get(vtss_appl_lldp_common_conf_t *const common_conf);

/**
 * Set configuration that are common for the whole switch/stack.
 *
 *  Purpose : To set the LLDP configuration that are common for all interfaces.
 *
 * \param common_conf [IN] Pointer to the new configuration
 *
 *  \return VTSS_RC_OK if configuration were applied correctly, else error code.
 */
mesa_rc vtss_appl_lldp_common_conf_set(const vtss_appl_lldp_common_conf_t *const common_conf);

/**
 * Get LLDP global statistics.
 *
 *  Purpose : To get the LLDP global statistics counters.

 * \param statistics [OUT] Pointer to where to put the statistic counters.
 *
 *  \return VTSS_RC_OK if statistics are valid, else error code.
 */
 mesa_rc vtss_appl_lldp_stat_global_get(vtss_appl_lldp_global_counters_t *statistics);

/**
 * Get LLDP statistics.
 *
 *  Purpose : To get the LLDP statistics counters.
 *
 * \param ifindex [IN] Interface index - the logical interface index of
 *                     the LLDP Port. This may be any physical switch port.
 *
 * \param statistics [OUT] Pointer to where to put the statistic counters.
 *
 *  \return VTSS_RC_OK if statistics are valid, else error code.
 */
    mesa_rc vtss_appl_lldp_stat_if_get(vtss_ifindex_t ifindex, vtss_appl_lldp_port_counters_t *statistics);

/**
 * Get configuration for a specific port interface.
 *
 *  Purpose : Getting the LLDP configuration that are specific for a port.
 *
 * \param ifindex [IN] Interface index - the logical interface index of
 *                     the LLDP Port. This may be any physical switch port.
 *
 * \param conf [IN] Pointer to the where to put the configuration.
 *
 *  \return VTSS_RC_OK if conf contains valid configuration, else error code.
 */
mesa_rc vtss_appl_lldp_port_conf_get(vtss_ifindex_t ifindex,
                                     vtss_appl_lldp_port_conf_t *conf);

/**
 * Set configuration for a specific port interface.
 *
 *  Purpose : To Set the LLDP configuration that are specific for a port.
 *
 * \param ifindex [IN] Interface index - the logical interface index of
 *                     the LLDP Port. This may be any physical switch port.
 *
 * \param conf    [IN] Pointer to the new configuration.
 *
 *  \return VTSS_RC_OK if configuration were applied correctly, else error code.
 */
mesa_rc vtss_appl_lldp_port_conf_set(vtss_ifindex_t ifindex,
                                     vtss_appl_lldp_port_conf_t *conf);

/**
 * Get the policy configuration
 *
 * \param policy_index [IN]  Policy index - the policy index in the policy list.
 *
 * \param conf         [OUT] Pointer to where to put the policy configuration.
 *
 *  \return VTSS_RC_OK if conf contains valid configuration, else error code.
 */
mesa_rc vtss_appl_lldp_conf_policy_get(uint8_t                          policy_index,
                                       vtss_appl_lldp_med_policy_t *conf);

/**
 * Set the policy configuration
 *
 * \param policy_index [IN] Policy index - the policy index in the policy list.
 *
 * \param conf         [IN] The new policy configuration.
 *
 *  \return VTSS_RC_OK if conf contains valid configuration, else error code.
 */
 mesa_rc vtss_appl_lldp_conf_policy_set(uint8_t                                policy_index,
                                        const vtss_appl_lldp_med_policy_t conf);

/**
 * Delete the policy configuration
 *
 * \param policy_index [IN] Policy index - the policy index in the policy list.
 *
 *  \return VTSS_RC_OK if the policy existed, else error code
 */
 mesa_rc vtss_appl_lldp_conf_policy_del(uint8_t policy_index);

/**
 * Get if a policy is enabled/mapped for an interface
 *
 *  Purpose : LLDP support having multiple policies assigned to an interface. The function returns
 *            if a specific port policy is mapped/enabled for a specific interface.
 *
 * \param ifindex      [IN]  Interface index - the logical interface index
 *                           the LLDP Port. This may be any physical switch port.
 *
 * \param policy_index [IN]  Interface policy index - the policy index in the policy list.
 *
 * \param enabled      [OUT] TRUE if the policy is enabled for the interface, else FALSE.
 *
 *  \return VTSS_RC_OK if "enabled" is valid, else error code.
 */
mesa_rc vtss_appl_lldp_conf_port_policy_get(vtss_ifindex_t ifindex,
                                            vtss_lldpmed_policy_index_t policy_index,
                                            mesa_bool_t           *enabled);

/** Set mapping of policy to an interface
 *
 *  Purpose : LLDP support having multiple policies assigned to an interface. The function enables/maps
 *            a policy to an interface.
 *
 * \param ifindex      [IN]  Interface index - the logical interface index of
 *                           the LLDP Port. This may be any physical switch port.
 *
 * \param policy_index [IN]  Interface policy index - the policy index in the policy list.
 *
 * \param enabled      [IN] TRUE to map the policy to the interface. FALSE to remove the policy from the interface.
 *
 *  \return VTSS_RC_OK if configuration was applied correctly, else error code.
 */
mesa_rc vtss_appl_lldp_conf_port_policy_set(vtss_ifindex_t ifindex,
                                            uint32_t            policy_index,
                                            const mesa_bool_t     enabled);

/**
 * Get capabilities for the LLDP implementation.
 *
 * \param cap     [out] Pointer to the where to put the capabilities.
  *
 *  \return VTSS_RC_OK if cap contains valid capabilities, else error code.
 */
mesa_rc vtss_appl_lldp_cap_get(vtss_appl_lldp_cap_t *cap);


/**
 * Get an entry from the remote neighbor entries table
 *
 * Purpose : Getting the entry that is valid for the given interface.
 *
 * \param ifindex          [IN]  Interface index - the logical interface index of
 *                               the LLDP Port. This may be any physical switch port.
 *
 * \param lldp_entry_index [INOUT] The neighbor table entry index
 *
 * \param lldp_entry       [OUT]  Pointer to where to put the entry information.
 *
 * \param next             [OUT]  Set to TRUE to get the next entry starting from lldp_entry_index. If set to FALSE only the lldp_entry_index entry is looked up in the table.
 *
 *  \return VTSS_RC_OK if lldp_entry points to a valid entry, else error code.
 *          Note: If the entry with lldp_entry_index doesn't have neighbor information corresponding to the
 *          ifindex VTSS_RC_ERROR is also returned.
 */
    mesa_rc vtss_appl_lldp_entry_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t *lldp_entry_index, vtss_appl_lldp_remote_entry_t *lldp_entry, mesa_bool_t next);

/**
 * Clear LLDP global statistics counters
 *
 *  \return VTSS_RC_OK if counter were cleared, else error code.
 */
mesa_rc vtss_appl_lldp_global_stat_clr(void);

/**
 * Clear LLDP interface statistics counters
 *
 * \param ifindex      [IN]  Interface index - the logical interface index of
 *                           the LLDP Port. This may be any physical switch port.
 *
 *  \return VTSS_RC_OK if counter were cleared, else error code.
 *
 */
mesa_rc vtss_appl_lldp_if_stat_clr(vtss_ifindex_t ifindex);


/*********************************************************************************************************************
 *   Help function for extracting information from a LLDP entry
 *********************************************************************************************************************/

/**
 * Get PoE (Power Over Ethernet) information from a LLDP entry as a printable string.
 * Because PoE for LLDP is defined in both IEEE802.3at (and different drafts) and TIA1057 this help function can be used
 * for getting the PoE information from the LLDP entry without having to worry about which PoE implementation the remote device
 * is using. By using this function you willget the same PoE information regards of this PoE standard the remote device is using.
 *
 * \param entry     [IN]  Entry containing the remote entry information
 *
 * \param str       [OUT] Pointer to the where to put the string.
 *
 * \param str_len   [IN]  The length of str.
 *
 * \param info_type [IN]  Which information to convert to string - 0=PoE type, 1=Source Type, 2=Power, 3=Priority
 *
 * \return Pointer to str.
 */
char *vtss_appl_lldp_remote_poeinfo2string(vtss_appl_lldp_remote_entry_t *entry, char *str, uint8_t str_len, uint8_t info_type);

/**
 * Getting chassis id as printable string
 *
 * \param entry [IN]  Entry containing the remote entry information
 *
 * \param str   [OUT] Pointer to the where to put the string. The sting length must be VTSS_APPL_MAX_CHASSIS_ID_LENGTH bytes
 *
 *  \return Pointer to str.
 */
char *vtss_appl_lldp_chassis_id2string(vtss_appl_lldp_remote_entry_t *entry, char *str);

/**
 * Getting management address as printable string
 *
 * \param mgmt_addr       [IN]  Pointer to the management address (from the entry containing the remote entry information).
 *
 * \param str             [OUT] Pointer to the where to put the string. The sting length must be VTSS_APPL_MAX_MGMT_LENGTH bytes.
 *
 *  \return Pointer to str.
 */
char *vtss_appl_lldp_mgmt_addr2string(vtss_appl_lldp_mgmt_addr_tlv_t *mgmt_addr, char *str);

/**
 * Getting system name as printable string
 *
 * \param entry [IN]  Entry containing the remote entry information
 *
 * \param str   [OUT] Pointer to the where to put the string. The sting length must be VTSS_APPL_MAX_SYSTEM_NAME_LENGTH bytes
 *
 *  \return Pointer to str.
 */
char *vtss_appl_lldp_system_name2string(vtss_appl_lldp_remote_entry_t *entry, char *str);

/**
 * Getting port description as printable string
 *
 * \param entry [IN]  Entry containing the remote entry information
 *
 * \param str   [OUT] Pointer to the where to put the string. The sting length must be VTSS_APPL_MAX_PORT_DESCR_LENGTH bytes
 *
 *  \return Pointer to str.
 */
char *vtss_appl_lldp_port_descr2string(vtss_appl_lldp_remote_entry_t *entry, char *str);

/**
 * Getting system description as printable string
 *
 * \param entry [IN]  Entry containing the remote entry information
 *
 * \param str   [OUT] Pointer to the where to put the string. The sting length must be VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH bytes
 *
 *  \return Pointer to str.
 */
char *vtss_appl_lldp_system_descr2string(vtss_appl_lldp_remote_entry_t *entry, char *str);

/**
 * Getting port id as printable string
 *
 * \param entry [IN]  Entry containing the remote entry information
 *
 * \param str   [OUT] Pointer to the where to put the string. The sting length must be VTSS_APPL_MAX_PORT_ID_LENGTH bytes
 *
 *  \return Pointer to str.
 */
char *vtss_appl_lldp_port_id2string(vtss_appl_lldp_remote_entry_t *entry, char *str);

 /** Iterator for looping through all civic address types.
 *
 * \param prev_catype [IN]  The previous civic address type. Set to NULL in order to start from first index
 *
 * \param next_catype [OUT] The next civic address type found.
 *
 *  \return VTSS_RC_OK while civic address type is found. VTSS_RC_ERROR when last type is exceeded.
 */
mesa_rc vtss_appl_lldp_med_catype_itr(const vtss_appl_lldp_med_catype_t *prev_catype, vtss_appl_lldp_med_catype_t *next_catype);

 /** Iterator for looping through all entries in the neighbor entries table.
 *
 * \param prev_ifindex [IN]  The previous interface index. Set to NULL in order to start from first index
 *
 * \param next_ifindex [OUT] The next interface index found
 *
 * \param prev_entry_index [IN]  The previous entry table index. Set to NULL in order to start from first index
 *
 * \param next_entry_index [OUT] The next entry index table found
 *
 *  \return VTSS_RC_OK while index is found. VTSS_RC_ERROR when last index is exceeded.
 */
 mesa_rc vtss_appl_lldp_port_entries_itr(const vtss_ifindex_t          *prev_ifindex,     vtss_ifindex_t          *next_ifindex,
                                         const vtss_lldp_entry_index_t *prev_entry_index, vtss_lldp_entry_index_t *next_entry_index);


 /** Iterator for looping through all entries and management addresses in the neighbor entries table.
 *
 * \param prev_ifindex     [IN]  The previous interface index. Set to NULL in order to start from first index
 *
 * \param next_ifindex     [OUT] The next interface index found
 *
 * \param prev_entry_index [IN]  The previous entry table index. Set to NULL in order to start from first index
 *
 * \param next_entry_index [OUT] The next entry index table found
 *
 * \param prev_mgmt_index  [IN]  The previous management address index. Set to NULL in order to start from first index
 *
 * \param next_mgmt_index  [OUT] The next management address index found
 *
 *  \return VTSS_RC_OK while index is found. VTSS_RC_ERROR when last index is exceeded.
 */
mesa_rc vtss_appl_lldp_port_mgmt_entries_itr(const vtss_ifindex_t          *prev_ifindex,
                                             vtss_ifindex_t                *next_ifindex,
                                             const vtss_lldp_entry_index_t *prev_entry_index,
                                             vtss_lldp_entry_index_t       *next_entry_index,
                                             const uint32_t                     *prev_mgmt_index,
                                             uint32_t                           *next_mgmt_index);

 /** Iterator for looping through all port policies
 *
 * \param prev_policy [IN]  The previous policy index. Set to NULL in order to start from first index
 *
 * \param next_policy [OUT] The next policy index found
 *
 *  \return VTSS_RC_OK while polilcy index is found. VTSS_RC_ERROR when last index is exceeded.
 */
mesa_rc vtss_appl_lldp_port_policies_itr(const vtss_lldpmed_policy_index_t *prev_policy, vtss_lldpmed_policy_index_t *next_policy);

 /** Iterator for looping through all interfaces + port policies giving the next port and policy index with a
  * policy in use.
 *
 * \param prev_ifindex [IN]  The previous interface index. Set to NULL in order to start from first index
 *
 * \param next_ifindex [OUT] The next interface index found
 *
 * \param prev_policy  [IN]  The previous policy index. Set to NULL in order to start from first index
 *
 * \param next_policy  [OUT] The next policy index found
 *
 *  \return VTSS_RC_OK while indexes is use are found. VTSS_RC_ERROR when no index in use is found.
 */
mesa_rc vtss_appl_lldp_port_policies_list_itr(const vtss_ifindex_t *prev_ifindex,             vtss_ifindex_t *next_ifindex,
                                              const vtss_lldpmed_policy_index_t *prev_policy, vtss_lldpmed_policy_index_t *next_policy);

 /** Function for extracting a CA string from the concatenated string which is containing all the CA strings.
 *
 * \param civic      [IN] Pointer to the civic string containing the concatenated civic location information.
 *
 * \param type       [IN] CaType to be extracted.
 *
 * \param max_length [IN] Max length of dest string.
 *
 * \param dest       [OUT] Pointer to where to put the result (string)
 *
 *  \return pointer to dest. If civic type is invalid, then dest is set to NULL.
 */
const char *vtss_appl_lldp_location_civic_info_get(const vtss_appl_lldp_med_civic_tlv_format_t *const civic,
                                             vtss_appl_lldp_med_catype_t type, uint32_t max_length, char *dest);


/** Function for injecting a CA string into the concatenated string which is containing all the CA strings.
 *
 * \param civic        [IN] Pointer to the civic string containing the concatenated civic location information.
 *
 * \param type         [IN] CaType to be injected.
 *
 * \param new_ca_value [IN] Pointer to the new string.
 *
 *  \return VTSS_RC_OK if the new string were injected correctly, else error code.
 */
mesa_rc vtss_appl_lldp_location_civic_info_set(vtss_appl_lldp_med_civic_tlv_format_t *civic,
                                               vtss_appl_lldp_med_catype_t type, const char *new_ca_value);

/** Function getting an inventory parameter from an neighbor entry as printable string
 *
 * \param entry      [IN] Pointer to entry containing the neighbor information.
 *
 * \param str        [IN] Pointer to where to put the string.
 *
 * \param max_length [IN]  Max length of str string.
 *
 * \param type       [IN] The inventory type to get.
 *
 *  \return pointer to str.
 */
const char *vtss_appl_lldp_med_invertory_info_get(vtss_appl_lldp_remote_entry_t *entry, vtss_appl_lldp_med_inventory_type_t type, uint8_t max_length, char *str);

/** Function converting LLDP OID to an array of printable IOD numbers. E.g. LLDP OID 2B 06 01 04 01 94 78 01 02 07 03 02 00 becomes 1.3.6.1.4.1.2680.1.2.7.3.2.0
 *
 * \param lldp_oid     [IN]  Pointer the data containing the LLDP OID.
 *
 * \param lldp_oid_len [IN]  Length of the LLDP OID.
 *
 * \param oid          [OUT] Pointer to where to put the OID result.
 *
 * \param oid_max_len  [IN]  The maximum length of the OID array.
 *
 * \param oid_len      [OUT] The number of words that are valid in the OID array.
 *
 *  \return VTSS_RC_OK if the oid contains valid data, else error code.
 */
mesa_rc oid_decode(uint8_t *lldp_oid, uint32_t lldp_oid_len, uint32_t *oid, uint32_t oid_max_len, uint32_t *oid_len);

/****************************************************************************
 * Defines which are useful for range checking and defined by the standards *
 ****************************************************************************/
#define VTSS_APPL_LLDP_TX_DELAY_MIN 1      /**<TX Delay minimum value, IEEE 802.1AB-2005, lldpTxDelay MIB*/
#define VTSS_APPL_LLDP_TX_DELAY_MAX 8192   /**<TX Delay maximum value, IEEE 802.1AB-2005, lldpTxDelay MIB*/
#define VTSS_APPL_LLDP_TX_DELAY_DEFAULT 2  /**<TX Delay default value, IEEE 802.1AB-2005, Section 10.5.3.3, bullet d)*/

#define VTSS_APPL_LLDP_TX_HOLD_MIN 2       /**< TX Hold minimum value, IEEE 802.1AB-2005, lldpMessageTxHoldMultiplier MIB (page 65)*/
#define VTSS_APPL_LLDP_TX_HOLD_MAX 10      /**< TX Hold maximum value, IEEE 802.1AB-2005, lldpMessageTxHoldMultiplier MIB (page 65)*/
#define VTSS_APPL_LLDP_TX_HOLD_DEFAULT 4   /**< TX Hold default value , IEEE 802.1AB-2005, Section 10.5.3.3, bullet a)*/

#define VTSS_APPL_LLDP_REINIT_MIN 1        /**< Re-init minimum value, IEEE 802.1AB-2005, lldpTxHold MIB (page 65)*/
#define VTSS_APPL_LLDP_REINIT_MAX 10       /**< Re-init maximum value, IEEE 802.1AB-2005, lldpReinitDelay MIB (page 65)*/
#define VTSS_APPL_LLDP_REINIT_DEFAULT 2    /**< Re-init default value,  IEEE 802.1AB-2005, Section 10.5.3.3, bullet c)*/

#define VTSS_APPL_LLDP_TX_INTERVAL_MIN 5     /**<Tx interval minimum value, IEEE 802.1AB-2005, lldpMessageTxInterval MIB (page 65)*/
#define VTSS_APPL_LLDP_TX_INTERVAL_MAX 32768 /**<Tx interval maximum value, IEEE 802.1AB-2005, lldpMessageTxInterval MIB (page 65)*/
#define VTSS_APPL_LLDP_TX_INTERVAL_DEFAULT 30/**<Tx interval default value IEEE 802.1AB-2005, Section 10.5.3.3, bullet b)*/

#define VTSS_APPL_LLDP_MED_L2_PRIORITY_MIN 0 /**< L2 priority minimum value ,Section 10.2.3.6, TIA1057*/
#define VTSS_APPL_LLDP_MED_L2_PRIORITY_MAX 7 /**< L2 priority maximum value ,Section 10.2.3.6, TIA1057*/

#define VTSS_APPL_LLDP_MED_DSCP_MIN 0        /**< DSCP minimum value,  Section 10.2.3.7, TIA1057*/
#define VTSS_APPL_LLDP_MED_DSCP_MAX 63       /**< DSCP maximum value,  Section 10.2.3.7, TIA1057*/

#define VTSS_APPL_LLDP_MED_VID_MIN 1         /**<VLAN minimum value, Table 13, TIA1057*/
#define VTSS_APPL_LLDP_MED_VID_MAX 4095      /**<VLAN minimum value, Table 13, TIA1057*/

#ifdef __cplusplus
}
#endif
#endif  /* _VTSS_APPL_LLDP_H_ */
