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

/**
 * \file
 * Public Stream API
 *
 * This header file describes Stream public functions and types, applicable to
 * stream management.
 *
 * Streams are used by FRER and PSFP to classify frames on ingress and assign an
 * action.
 *
 * An overlay function, stream-collections, compiles one or more streams, which
 * makes multiple streams available in functions such as FRER in generator mode,
 * where all the streams need to use the same sequence number generator, and
 * therefore point to the same IFLOW.
 */

#ifndef _VTSS_APPL_STREAM_H_
#define _VTSS_APPL_STREAM_H_

#include <vtss/appl/interface.h>
#include <vtss/appl/vcl.h>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_BITWISE() */

/**
 * Stream error codes (mesa_rc)
 */
enum {
    // Stream error codes start here
    VTSS_APPL_STREAM_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_STREAM), /**< Invalid argument (typically a NULL pointer) passed to a function              */
    VTSS_APPL_STREAM_RC_NO_SUCH_ID,                                                    /**< Stream instance is not created                                                */
    VTSS_APPL_STREAM_RC_HW_RESOURCES,                                                  /**< Out of hardware resources                                                     */
    VTSS_APPL_STREAM_RC_INTERNAL_ERROR,                                                /**< Internal error. Requires code update                                          */
    VTSS_APPL_STREAM_RC_INVALID_ID,                                                    /**< Invalid stream ID                                                             */
    VTSS_APPL_STREAM_RC_INVALID_CLIENT,                                                /**< Invalid client ID                                                             */
    VTSS_APPL_STREAM_RC_INVALID_DMAC_MATCH_TYPE,                                       /**< Invalid DMAC match type                                                       */
    VTSS_APPL_STREAM_RC_INVALID_DMAC_MASK,                                             /**< DMAC mask cannot be all-zeros                                                 */
    VTSS_APPL_STREAM_RC_INVALID_UNICAST_SMAC,                                          /**< The SMAC is not a unicast MAC address                                         */
    VTSS_APPL_STREAM_RC_INVALID_VLAN_MATCH_TYPE_OUTER_TAG,                             /**< Invalid outer tag match type                                                  */
    VTSS_APPL_STREAM_RC_INVALID_VLAN_MATCH_TYPE_INNER_TAG,                             /**< Invalid inner tag match type                                                  */
    VTSS_APPL_STREAM_RC_INVALID_TAG_TYPE_OUTER_TAG,                                    /**< Invalid outer tag tag type                                                    */
    VTSS_APPL_STREAM_RC_INVALID_TAG_TYPE_INNER_TAG,                                    /**< Invalid inner tag tag type                                                    */
    VTSS_APPL_STREAM_RC_INVALID_VID_VALUE_OUTER_TAG,                                   /**< Invalid outer tag VLAN ID                                                     */
    VTSS_APPL_STREAM_RC_INVALID_VID_VALUE_INNER_TAG,                                   /**< Invalid inner tag VLAN ID                                                     */
    VTSS_APPL_STREAM_RC_INVALID_VID_MASK_OUTER_TAG,                                    /**< Invalid outer tag VLAN mask                                                   */
    VTSS_APPL_STREAM_RC_INVALID_VID_MASK_INNER_TAG,                                    /**< Invalid inner tag VLAN mask                                                   */
    VTSS_APPL_STREAM_RC_INVALID_PCP_VALUE_OUTER_TAG,                                   /**< Invalid outer tag PCP value                                                   */
    VTSS_APPL_STREAM_RC_INVALID_PCP_VALUE_INNER_TAG,                                   /**< Invalid inner tag PCP value                                                   */
    VTSS_APPL_STREAM_RC_INVALID_PCP_MASK_OUTER_TAG,                                    /**< Invalid outer tag PCP mask                                                    */
    VTSS_APPL_STREAM_RC_INVALID_PCP_MASK_INNER_TAG,                                    /**< Invalid inner tag PCP mask                                                    */
    VTSS_APPL_STREAM_RC_INVALID_DEI_OUTER_TAG,                                         /**< Invalid outer tag DEI                                                         */
    VTSS_APPL_STREAM_RC_INVALID_DEI_INNER_TAG,                                         /**< Invalid inner tag DEI                                                         */
    VTSS_APPL_STREAM_RC_OUTER_UNTAGGED_INNER_TAGGED,                                   /**< Impossible match: outer-tag is untagged and inner-tag is tagged               */
    VTSS_APPL_STREAM_RC_INVALID_ETYPE,                                                 /**< Invalid EtherType. Valid range is 0x600 - 0xFFFF                              */
    VTSS_APPL_STREAM_RC_INVALID_SNAP_OUI,                                              /**< Invalid SNAP OUI                                                              */
    VTSS_APPL_STREAM_RC_INVALID_SNAP_OUI_TYPE,                                         /**< Invalid SNAP OUI type                                                         */
    VTSS_APPL_STREAM_RC_INVALID_SNAP_PID,                                              /**< If OUI is zero, PID is in range of EtherTypes (0x600 - 0xFFFF)                */
    VTSS_APPL_STREAM_RC_INVALID_IPV4_FRAGMENT,                                         /**< Invalid IPv4 fragment                                                         */
    VTSS_APPL_STREAM_RC_IPV4_DSCP_OUT_OF_RANGE,                                        /**< IPv4's DSCP value is out of range (0-63)                                      */
    VTSS_APPL_STREAM_RC_IPV4_DSCP_LOW_OUT_OF_RANGE,                                    /**< IPv4's DSCP low value out of range (0-63)                                     */
    VTSS_APPL_STREAM_RC_IPV4_DSCP_HIGH_OUT_OF_RANGE,                                   /**< IPv4's DSCP high value out of range (0-63)                                    */
    VTSS_APPL_STREAM_RC_IPV4_DSCP_HIGH_SMALLER_THAN_LOW,                               /**< IPv4's DSCP's high value is smaller than the low value                        */
    VTSS_APPL_STREAM_RC_IPV4_DSCP_MATCH_TYPE,                                          /**< Invalid IPv4 DSCP match type value                                            */
    VTSS_APPL_STREAM_RC_INVALID_IPV4_PROTO_TYPE,                                       /**< IPv4's protocol type is invalid                                               */
    VTSS_APPL_STREAM_RC_INVALID_IPV4_SIP_PREFIX_SIZE,                                  /**< IPv4's source IP prefix size must be in range 0-32                            */
    VTSS_APPL_STREAM_RC_INVALID_IPV4_DIP_PREFIX_SIZE,                                  /**< IPv4's destination IP prefix size must be in range 0-32                       */
    VTSS_APPL_STREAM_RC_IPV4_DPORT_HIGH_SMALLER_THAN_LOW,                              /**< IPv4's dport's high value is smaller than the low value                       */
    VTSS_APPL_STREAM_RC_IPV4_DPORT_MATCH_TYPE,                                         /**< "Invalid IPv4 UDP/TCP destination port match type value                       */
    VTSS_APPL_STREAM_RC_IPV6_DSCP_OUT_OF_RANGE,                                        /**< IPv6's DSCP value is out of range (0-63)                                      */
    VTSS_APPL_STREAM_RC_IPV6_DSCP_LOW_OUT_OF_RANGE,                                    /**< IPv6's DSCP low value out of range (0-63)                                     */
    VTSS_APPL_STREAM_RC_IPV6_DSCP_HIGH_OUT_OF_RANGE,                                   /**< IPv6's DSCP high value out of range (0-63)                                    */
    VTSS_APPL_STREAM_RC_IPV6_DSCP_HIGH_SMALLER_THAN_LOW,                               /**< IPv6's DSCP's high value is smaller than the low value                        */
    VTSS_APPL_STREAM_RC_IPV6_DSCP_MATCH_TYPE,                                          /**< Invalid IPv6 DSCP match type value                                            */
    VTSS_APPL_STREAM_RC_INVALID_IPV6_PROTO_TYPE,                                       /**< IPv6's protocol type is invalid                                               */
    VTSS_APPL_STREAM_RC_INVALID_IPV6_SIP_PREFIX_SIZE,                                  /**< IPv6's source IP prefix size must be in range 0-128                           */
    VTSS_APPL_STREAM_RC_INVALID_IPV6_DIP_PREFIX_SIZE,                                  /**< IPv6's destination IP prefix size must be in range 0-128                      */
    VTSS_APPL_STREAM_RC_IPV6_DPORT_HIGH_SMALLER_THAN_LOW,                              /**< IPv6's dport's high value is smaller than the low value                       */
    VTSS_APPL_STREAM_RC_IPV6_DPORT_MATCH_TYPE,                                         /**< "Invalid IPv6 UDP/TCP destination port match type value                       */
    VTSS_APPL_STREAM_RC_INVALID_PROTOCOL_TYPE,                                         /**< Invalid protocol type                                                         */
    VTSS_APPL_STREAM_RC_OUT_OF_MEMORY,                                                 /**< Out of memory                                                                 */
    VTSS_APPL_STREAM_RC_COUNTERS_NOT_ALLOCATED,                                        /**< Stream counters not allocated, because no clients are attached                */
    VTSS_APPL_STREAM_RC_PART_OF_COLLECTION,                                            /**< Stream is part of a collection. Cannot attach client directly                 */
    VTSS_APPL_STREAM_RC_COLLECTION_INVALID_ID,                                         /**< Invalid stream collection ID                                                  */
    VTSS_APPL_STREAM_RC_COLLECTION_NO_SUCH_ID,                                         /**< Stream collection instance is not created                                     */
    VTSS_APPL_STREAM_RC_COLLECTION_STREAM_ID_DOESNT_EXIST,                             /**< At least one of the configured stream IDs doesn't exist                       */
    VTSS_APPL_STREAM_RC_COLLECTION_STREAM_PART_OF_OTHER_COLLECTION,                    /**< At least one of the configured stream IDs is already part of other collection */
    VTSS_APPL_STREAM_RC_COLLECTION_COUNTERS_NOT_ALLOCATED,                             /**< Stream collection counters not allocated, because no clients are attached     */
};

/**
 * Capabilities of streams.
 */
typedef struct {
    /**
     * Maximum number of configurable streams.
     * Streams are numbered [1; inst_cnt_max].
     */
    uint32_t inst_cnt_max;
} vtss_appl_stream_capabilities_t;

/**
 * Get Stream capabilities.
 *
 * \param cap [OUT] Stream capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_stream_capabilities_get(vtss_appl_stream_capabilities_t *cap);

/**
 * Indicates an unused/invalid stream ID
 */
#define VTSS_APPL_STREAM_ID_NONE 0

/**
 * vtss_appl_stream_id_t is a unique number identifying a stream
 */
typedef uint32_t vtss_appl_stream_id_t;

/**
 * Indicates an unused/invalid stream collection ID.
 */
#define VTSS_APPL_STREAM_COLLECTION_ID_NONE 0

/**
 * vtss_appl_stream_collection_id_t is a unique number identifying a stream
 * collection.
 */
typedef uint32_t vtss_appl_stream_collection_id_t;

/**
 * Different DMAC match options
 */
typedef enum {
    VTSS_APPL_STREAM_DMAC_MATCH_TYPE_ANY,    /**< Match on ANY DMAC      */
    VTSS_APPL_STREAM_DMAC_MATCH_TYPE_MC,     /**< Match M/C, but not B/C */
    VTSS_APPL_STREAM_DMAC_MATCH_TYPE_BC,     /**< Match B/C, but not M/C */
    VTSS_APPL_STREAM_DMAC_MATCH_TYPE_UC,     /**< Match U/C              */
    VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_BC, /**< Match anything but B/C */
    VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_UC, /**< Match M/C or B/C       */
    VTSS_APPL_STREAM_DMAC_MATCH_TYPE_VALUE   /**< Use value and mask     */
} vtss_appl_stream_dmac_match_type_t;

/**
 * Configuration of DMAC matching.
 */
typedef struct {
    /**
     * Indicates how to match on DMAC - if at all.
     */
    vtss_appl_stream_dmac_match_type_t match_type;

    /**
     * If \p match_type is VTSS_APPL_STREAM_DMAC_MATCH_TYPE:VALUE, this contains
     * the DMAC that is matched against after bit-wise ANDing the incoming frame
     * with \p mask.
     *
     * \p value will automatically be set to value & mask.
     */
    mesa_mac_t value;

    /**
     * Mask that determines which bits in \p value that are matched
     * against.
     */
    mesa_mac_t mask;
} vtss_appl_stream_dmac_t;

/**
 * Configuration of SMAC matching.
 */
typedef struct {
    /**
     * Source MAC address to match.
     * See also \p mask.
     *
     * \p value will automatically be set to value & mask.
     */
    mesa_mac_t value;

    /**
     * Mask that determines which bits in \p value that are matched
     * against.
     * If all-zeros, source MAC matching is not used.
     */
    mesa_mac_t mask;
} vtss_appl_stream_smac_t;

/**
 * Different outer/inner VLAN tag match options
 */
typedef enum {
    VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_BOTH,     /**< Match both tagged and untagged frames */
    VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_UNTAGGED, /**< Match untagged frames                 */
    VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED,   /**< Match tagged frames                   */
} vtss_appl_stream_vlan_tag_match_type_t;

/**
 * Different outer/inner VLAN TPID match options
 */
typedef enum {
    VTSS_APPL_STREAM_VLAN_TAG_TYPE_ANY,      /**< Matches both C-, S- and S-custom tagged */
    VTSS_APPL_STREAM_VLAN_TAG_TYPE_C_TAGGED, /**< Matches C-tagged                        */
    VTSS_APPL_STREAM_VLAN_TAG_TYPE_S_TAGGED, /**< Matches S- and S-custom tagged          */
} vtss_appl_stream_vlan_tag_type_t;

/**
 * Properties used when matching a VLAN tag (either outer or inner).
 */
typedef struct {
    /**
     * Indicates whether to match on presence or absence of a VLAN tag, or
     * don't care.
     */
    vtss_appl_stream_vlan_tag_match_type_t match_type;

    /**
     * Select which TPID to match.
     *
     * Default is VTSS_APPL_STREAM_VLAN_TAG_TYPE_ANY.
     *
     * Only used if \p match_type is
     * VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED.
     */
    vtss_appl_stream_vlan_tag_type_t tag_type;

    /**
     * Indicates the VLAN ID to match on.
     *
     * Valid values: 0-4095.
     *
     * Only used if \p match_type is
     * VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED.
     *
     * Notice, the stream module automatically changes this value to
     * \p vid_value & \p vid_mask.
     *
     * So if there are bits set in the value that are not set in the mask, the
     * value will change.
     */
    mesa_vid_t vid_value;

    /**
     * Indicates the VLAN mask used in the matching.
     *
     * Valid values: [0x000; 0xFFF]
     *
     * Only used if \p match_type is
     * VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED.
     *
     * Set to all-zeros to match any.
     */
    mesa_vid_t vid_mask;

    /**
     * Indicates the PCP value to match on.
     *
     * Valid values: 0-7.
     *
     * Only used if \p match_type is
     * VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED.
     *
     * Notice, the stream module automatically changes this value to
     * \p pcp_value & \p pcp_mask.
     *
     * So if there are bits set in the value that are not set in the mask, the
     * value will change.
     */
    mesa_pcp_t pcp_value;

    /**
     * Indicates the PCP mask used in the matching.
     *
     * Valid values: [0x0; 0xF]
     *
     * Only used if \p match_type is
     * VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED.
     *
     * Set to all-zeros to match any.
     */
    mesa_pcp_t pcp_mask;

    /**
     * Indicates whether to match on DEI, and if so, which value (0 or 1).
     *
     * Only used if \p match_type is
     * VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED.
     */
    mesa_vcap_bit_t dei;
} vtss_appl_stream_vlan_tag_t;

/**
 * Configuration used when protocol type is MESA_VCE_TYPE_ETYPE.
 */
typedef struct {
    /**
     * The EtherType to match.
     *
     * Valid values are [0x600, 0xffff].
     */
    mesa_etype_t etype;
} vtss_appl_stream_proto_etype_t;

/**
 * Configuration used when protocol type is MESA_VCE_TYPE_LLC.
 */
typedef struct {
    /**
     * Destination Service Access Point
     */
    uint8_t dsap;

    /**
     * Source Service Access Point
     */
    uint8_t ssap;
} vtss_appl_stream_proto_llc_t;

/**
 * SNAP OUI types
 */
typedef enum {
    VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_RFC1042, /**< Use OUI = 00:00:00 */
    VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_8021H,   /**< Use OUI = 00:00:f8 */
    VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_CUSTOM,  /**< Use a custom OUI   */
} vtss_appl_stream_proto_snap_oui_type_t;

/**
 * Configuration used when protocol type is MESA_VCE_TYPE_SNAP.
 */
typedef struct {
    /**
     * Use either a pre-defined OUI or a custom OUI.
     */
    vtss_appl_stream_proto_snap_oui_type_t oui_type;

    /**
     * Organizationally Unique ID. Only used when \p oui_type is
     * VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_CUSTOM.
     *
     * When reading the configuration and \p oui_type is not set to the custom
     * value, the \p oui reflects the matched OUI.
     *
     * Valid values are in range [0x000000; 0xFFFFFF]
     */
    uint32_t oui;

    /**
     * Protocol ID.
     */
    uint16_t pid;
} vtss_appl_stream_proto_snap_t;

/**
 * Stream range match type
 */
typedef enum {
    VTSS_APPL_STREAM_RANGE_MATCH_TYPE_ANY,   /**< Match any range          */
    VTSS_APPL_STREAM_RANGE_MATCH_TYPE_VALUE, /**< Match a particular value */
    VTSS_APPL_STREAM_RANGE_MATCH_TYPE_RANGE, /**< Match a range of values  */
} vtss_appl_stream_range_match_type_t;

/**
 * Used when a range can be specified.
 */
typedef struct {
    /**
     * If ANY, there will be no attempt to match the field this range
     * represents.
     *
     * If VALUE, it will only match one value indicated by \p low.
     *
     * If RANGE, it will match the range [\p low; \p high].
     */
    vtss_appl_stream_range_match_type_t match_type;

    /**
     * Indicates the low value of the range to match.
     * Only used if \p match_type is VALUE or RANGE.
     *
     * Must be <= \p high if RANGE.
     */
    uint16_t low;

    /**
     * Indicates the high value of the range to match.
     * Only used if \p match_type is RANGE.
     *
     * Must be >= \p low.
     */
    uint16_t high;
} vtss_appl_stream_range_t;

/**
 * Selection of IPv4 or IPv6 header's Protocol field.
 */
typedef enum {
    VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_ANY,    /**< Match on any protocol value               */
    VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_CUSTOM, /**< Any protocol value other than TCP and UDP */
    VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_TCP,    /**< Protocol value is 6  (for TCP)            */
    VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_UDP     /**< Protocol value is 17 (for UDP)            */
} vtss_appl_stream_ip_protocol_type_t;

/**
 * An IPv4 or IPv6 header's protocol field
 */
typedef struct {
    /**
     * The IP header's protocol type.
     *
     * If set to Custom and \p value matches either UDP or TCP upon set,
     * this will get updated to UDP or TCP upon get.
     */
    vtss_appl_stream_ip_protocol_type_t type;

    /**
     * The IP header's protocol value.
     *
     * If \p type is set to UDP or TCP, this value is ignored upon set()
     * but updated to the correct value upon get().
     */
    uint8_t value;
} vtss_appl_stream_ip_protocol_t;

/**
 * Configuration used when protocol type is MESA_VCE_TYPE_IPV4.
 */
typedef struct {
    /**
     * Source IP address and prefix length.
     *
     * If prefix length is 0, the source IP address is not matched.
     *
     * The underlying source code will adjust sip.address so that bits outside
     * sip.prefix size will be cleared upon set.
     */
    mesa_ipv4_network_t sip;

    /**
    * Destination IP address and prefix length.
    *
    * If prefix length is 0, the source IP address is not matched.
    *
    * The underlying source code will adjust dip.address so that bits outside
    * dip.prefix size will be cleared upon set.
    */
    mesa_ipv4_network_t dip;

    /**
     * DSCP range of values to match.
     */
    vtss_appl_stream_range_t dscp;

    /**
     * Indicates whether to match on the IPv4 header's "More Fragments (MF)"
     * and "Fragment Offset" fields.
     *
     * MESA_VCAP_BIT_ANY:
     *   Don't care about fragments.
     * MESA_VCAP_BIT_0:
     *   MF and Fragment Offset must both be 0.
     * MESA_VCAP_BIT_1:
     *   At least one of MF and Fragment Offset must be non-zero.
     */
    mesa_vcap_bit_t fragment;

    /**
     * The IPv4 header's protocol
     */
    vtss_appl_stream_ip_protocol_t proto;

    /**
     * TCP/UDP destination port range of values to match.
     */
    vtss_appl_stream_range_t dport;
} vtss_appl_stream_proto_ipv4_t;

/**
 * Configuration used when protocol type is MESA_VCE_TYPE_IPV6.
 */
typedef struct {
    /**
     * Source IP address and prefix length.
     *
     * If prefix length is 0, the source IP address is not matched.
     *
     * The underlying source code will adjust sip.address so that bits outside
     * sip.prefix size will be cleared upon set.
     */
    mesa_ipv6_network_t sip;

    /**
    * Destination IP address and prefix length.
    *
    * If prefix length is 0, the source IP address is not matched.
    *
    * The underlying source code will adjust dip.address so that bits outside
    * dip.prefix size will be cleared upon set.
    */
    mesa_ipv6_network_t dip;

    /**
     * DSCP range of values to match.
     */
    vtss_appl_stream_range_t dscp;

    /**
     * The IPv6 header's protocol
     */
    vtss_appl_stream_ip_protocol_t proto;

    /**
     * TCP/UDP destination port range of values to match.
     */
    vtss_appl_stream_range_t dport;
} vtss_appl_stream_proto_ipv6_t;

/**
 * Union of all six possible Protocol field types supported by stream selection
 */
typedef struct {
    /**
     * Protocol type
     */
    mesa_vce_type_t type;

    /**
     * Union of protocol fields. Used when \p type != MESA_VCE_TYPE_ANY.
     */
    union {
        /**
         * Protocol field of the Ethernet II EtherType.
         * Used when \p type is MESA_VCE_TYPE_ETYPE.
         */
        vtss_appl_stream_proto_etype_t etype;

        /**
         * Protocol fields of LLC (Logical Link Control) frames, that is,
         * frames with EtherType/TypeLength < 0x600.
         * Used when \p type is MESA_VCE_TYPE_LLC.
         */
        vtss_appl_stream_proto_llc_t llc;

        /**
         * Protocol fields of SNAP (LLC frame with DSAP == 0xaa, SSAP == 0xaa,
         * and Control field = 0x03.
         *
         * Used when \p type is MESA_VCE_TYPE_SNAP.
         */
        vtss_appl_stream_proto_snap_t snap;

        /**
         * The protocol fields of IPv4.
         *
         * Used when \p type is MESA_VCE_TYPE_IPV4.
         */
        vtss_appl_stream_proto_ipv4_t ipv4;

        /**
         * The protocol fields of IPv6.
         *
         * Used when \p type is MESA_VCE_TYPE_IPV6.
         */
        vtss_appl_stream_proto_ipv6_t ipv6;
    } value;
} vtss_appl_stream_protocol_conf_t;

/**
 * Configuration of a stream (the match fields).
 */
typedef struct {
    /**
     * Configuration of DMAC matching.
     */
    vtss_appl_stream_dmac_t dmac;

    /**
     * Configuration of SMAC matching.
     */
    vtss_appl_stream_smac_t smac;

    /**
     * Match properties of a possible outer VLAN tag.
     */
    vtss_appl_stream_vlan_tag_t outer_tag;

    /**
     * Match properties of a possible inner VLAN tag.
     *
     * It is illegal to set outer_tag.match to untagged, while setting
     * inner_tag.match to tagged.
     */
    vtss_appl_stream_vlan_tag_t inner_tag;

    /**
     * The protocol field contains the protocol dependant matching rules
     */
    vtss_appl_stream_protocol_conf_t protocol;

    /**
     * The list of ports on which the stream shall be applied.
     */
    mesa_port_list_t port_list;
} vtss_appl_stream_conf_t;

/**
 * Function for getting default protocol configuration for a given protocol type
 *
 * \param type     [IN] Protocol type to get defaults for
 * \param protocol [IN] Protocol configuration to receive defaults.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_stream_conf_protocol_default_get(mesa_vce_type_t type, vtss_appl_stream_protocol_conf_t *protocol);

/**
 * Function for getting a default stream configuration
 *
 * \param stream_conf [OUT] Default stream.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_stream_conf_default_get(vtss_appl_stream_conf_t *stream_conf);

/**
 * Function for getting a stream configuration.
 *
 * \param stream_id   [IN]  The ID of the stream
 * \param stream_conf [OUT] The configuration of the stream
 *
 * \return VTSS_RC_OK if the operation succeeds, VTSS_RC_ERROR otherwise
 */
mesa_rc vtss_appl_stream_conf_get(vtss_appl_stream_id_t stream_id, vtss_appl_stream_conf_t *stream_conf);

/**
 * Function for setting a stream configuration. If the stream
 * already exists, the current configuration is overwritten, otherwise
 * a new stream is created.
 *
 * \param stream_id   [IN] The ID of the stream
 * \param stream_conf [IN] The configuration of the stream
 *
 * \return VTSS_RC_OK if the operation succeeds, VTSS_RC_ERROR otherwise
 */
mesa_rc vtss_appl_stream_conf_set(vtss_appl_stream_id_t stream_id, const vtss_appl_stream_conf_t *stream_conf);

/**
 * Function for deleting a stream configuration.
 *
 * \param stream_id [IN] The ID of the stream
 *
 * \return VTSS_RC_OK if the operation succeeds, VTSS_RC_ERROR otherwise
 */
mesa_rc vtss_appl_stream_conf_del(vtss_appl_stream_id_t stream_id);

/**
 * Iterate through created streams.
 * This function will iterate through all streams found in the table.
 *
 * The easiest way to iterate through all streams is to:
 *    vtss_appl_stream_id_t stream_id = VTSS_APPL_STREAM_ID_NONE;
 *    while (vtss_appl_stream_itr(&stream_id, &stream_id) == VTSS_RC_OK) {
 *        // Do what is needed.
 *    }
 *
 * \param prev_stream_id [IN]  Previous stream ID.
 * \param next_stream_id [OUT] Next stream ID.
 *
 * \return VTSS_RC_OK if a next entry was found, VTSS_RC_ERROR otherwise.
 */
mesa_rc vtss_appl_stream_itr(const vtss_appl_stream_id_t *prev_stream_id, vtss_appl_stream_id_t *next_stream_id);

/**
 * Enumeration of possible clients that can use a stream. At any time, a stream
 * can be associated with zero or more clients.
 * associated with one client.
 */
typedef enum  {
    /**
     * The stream is currently associated with a PSFP client
     */
    VTSS_APPL_STREAM_CLIENT_PSFP,

    /**
     * The stream is currently associated with a FRER client
     */
    VTSS_APPL_STREAM_CLIENT_FRER,

    /**
     * Indicates possible number of clients associated with this stream. Must
     * come last
     */
    VTSS_APPL_STREAM_CLIENT_CNT,
} vtss_appl_stream_client_t;

/**
 * Each client may configure certain fields of a stream's action.
 */
typedef struct {
    /**
     * If true, the client wishes to enable this action.
     * If false, the client wishes to withdraw from this action.
     */
    mesa_bool_t enable;

    /**
     * The \p cut_through_disable field is used by both FRER and PSFP.
     *
     * PSFP wants to set it to false and FRER must set it to true in recovery
     * mode. So if FRER is enabled and FRER is in recovery mode,
     * cut_through_disable must be set to true. If FRER is in generation mode,
     * it doesn't care and sets \p cut_through_override to false.
     */
    mesa_bool_t cut_through_override;

    /**
     * See discussion in \p cut_through_override.
     */
    mesa_bool_t cut_through_disable;

    /**
     * An ID that may be used by the client for its own identification.
     */
    uint32_t client_id;

    /**
     * When PSFP configures a stream's actions, it uses the psfp struct and when
     * FRER configures a stream's actions, it uses the frer struct.
     */
    union {
        /**
         * Structure used by PSFP
         */
        struct {
            /**
             * Enable DLB policer
             */
            mesa_bool_t dlb_enable;

            /**
             * DLB policer ID.
             */
            mesa_dlb_policer_id_t dlb_id;

            /**
             * PSFP ingress flow configuration.
             */
            mesa_psfp_iflow_conf_t psfp;
        } psfp;

        /**
         * Structure used by FRER
         */
        struct {
            /**
             * Classified VLAN ID.
             */
            mesa_vid_t vid;

            /**
             * VLAN tag pop enable.
             */
            mesa_bool_t pop_enable;

            /**
             * VLAN tag pop count.
             */
            uint8_t pop_cnt;

            /**
             * FRER ingress flow configuration.
             */
            mesa_frer_iflow_conf_t frer;
        } frer;
    };
} vtss_appl_stream_action_t;

/**
 * Set a client action.
 *
 * Clients attach to this stream and assigns various stream actions.
 *
 * \param stream_id          [IN] Stream ID.
 * \param client             [IN] Client ID.
 * \param action             [IN] Client's action.
 * \param frer_seq_gen_reset [IN] If true, \p action is not used, but the stream's IFLOW gets re-applied. This is in order to reset the FRER sequence number generator.
 *
 * \return VTSS_RC_OK if attachment succeeded.
 */
mesa_rc vtss_appl_stream_action_set(vtss_appl_stream_id_t stream_id, vtss_appl_stream_client_t client, vtss_appl_stream_action_t *action, bool frer_seq_gen_reset = false);

/**
 * This enum holds flags that indicate various configurational warnings.
 */
typedef enum {
    VTSS_APPL_STREAM_OPER_WARNING_NONE                      = 0x000, /**< No warnings found                */
    VTSS_APPL_STREAM_OPER_WARNING_NOT_INSTALLED_ON_ANY_PORT = 0x001, /**< Stream not installed on any port */
} vtss_appl_stream_oper_warnings_t;

/**
 * Operators for vtss_appl_stream_oper_warnings_t flags.
 */
VTSS_ENUM_BITWISE(vtss_appl_stream_oper_warnings_t);

/**
 * Stream or stream collection client status
 */
typedef struct {
    /**
     * The actions set by the clients and indexed by vtss_appl_stream_client_t.
     */
    vtss_appl_stream_action_t clients[VTSS_APPL_STREAM_CLIENT_CNT];
} vtss_appl_stream_client_status_t;

/**
 * Contains operational warnings and identifies clients attached to a stream.
 */
typedef struct {
    /**
     * Identification of the stream collection that this stream is part of. If
     * it is not part of any stream collections, it is
     * VTSS_APPL_STREAM_COLLECTION_ID_NONE.
     */
    vtss_appl_stream_collection_id_t stream_collection_id;

    /**
     * Configurational warnings.
     */
    vtss_appl_stream_oper_warnings_t oper_warnings;

    /**
     * Status of the clients attached to this stream. If the stream is part of a
     * stream collection, clients cannot be attached to the stream, but will be
     * attached to the stream collection.
     */
    vtss_appl_stream_client_status_t client_status;
} vtss_appl_stream_status_t;

/**
 * Function for getting the status of a specific stream.
 *
 * \param stream_id [IN]  A stream ID
 * \param status    [OUT] The status of the stream
 *
 * \return VTSS_RC_OK if the stream exists.
 */
mesa_rc vtss_appl_stream_status_get(vtss_appl_stream_id_t stream_id, vtss_appl_stream_status_t *status);

/******************************************************************************/
// STREAM COLLECTIONS
/******************************************************************************/

/**
 * Capabilities of stream collections.
 */
typedef struct {
    /**
     * Maximum number of configurable stream collections.
     * Stream collections are numbered [1; inst_cnt_max].
     */
    uint32_t inst_cnt_max;

    /**
     * Maximum number of streams per collection. This is an arbitrarily chosen
     * constant and may be changed without any side effects.
     */
    uint32_t streams_per_collection_max;
} vtss_appl_stream_collection_capabilities_t;

/**
 * Get Stream collection capabilities.
 *
 * \param cap [OUT] Stream collection capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_stream_collection_capabilities_get(vtss_appl_stream_collection_capabilities_t *cap);

/**
 * This defines the maximum number of streams a stream collection can carry. In
 * theory any number can be used, but 8 seems reasonable.
 */
#define VTSS_APPL_STREAM_COLLECTION_STREAM_CNT_MAX 8

/**
 * Configuration of a stream-collection.
 */
typedef struct {
    /**
     * List of streams going into the stream collection.
     *
     * A value of VTSS_APPL_STREAM_ID_NONE in an entry indicates that this entry
     * is not used.
     *
     * Another property of this one is that the order of the IDs can be any when
     * setting, whereas when retrieving the configuration, it is guaranteed that
     * they are in numberical order with duplicates removed and possible
     * VTSS_APPL_STREAM_ID_NONE entries come last.
     */
    vtss_appl_stream_id_t stream_ids[VTSS_APPL_STREAM_COLLECTION_STREAM_CNT_MAX];
} vtss_appl_stream_collection_conf_t;

/**
 * Function for getting a default stream collection configuration.
 *
 * \param stream_collection_conf [OUT] Default stream collection.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_stream_collection_conf_default_get(vtss_appl_stream_collection_conf_t *stream_collection_conf);

/**
 * Function for getting a stream collection configuration.
 *
 * \param stream_collection_id   [IN]  The ID of the stream collection
 * \param stream_collection_conf [OUT] The configuration of the stream collection.
 *
 * \return VTSS_RC_OK if the operation succeeds, VTSS_RC_ERROR otherwise
 */
mesa_rc vtss_appl_stream_collection_conf_get(vtss_appl_stream_collection_id_t stream_collection_id, vtss_appl_stream_collection_conf_t *stream_collection_conf);

/**
 * Function for setting a stream collection configuration. If the stream
 * collection already exists, the current configuration is overwritten,
 * otherwise a new stream collection is created.
 *
 * Streams in the stream collection that are already located in other stream
 * locations will be moved to the new collection.
 *
 * If at least one of the streams that get added doesn't exist, an error will be
 * returned.
 *
 * Whenever a stream gets removed, it also disappears from a stream collection
 * that it may exist in.
 *
 * \param stream_collection_id   [IN] The ID of the stream collection
 * \param stream_collection_conf [IN] The configuration of the stream collection
 *
 * \return VTSS_RC_OK if the operation succeeds, VTSS_RC_ERROR otherwise
 */
mesa_rc vtss_appl_stream_collection_conf_set(vtss_appl_stream_collection_id_t stream_collection_id, const vtss_appl_stream_collection_conf_t *stream_collection_conf);

/**
 * Function for deleting a stream collection.
 *
 * \param stream_collection_id [IN] The ID of the stream collection
 *
 * \return VTSS_RC_OK if the operation succeeds, VTSS_RC_ERROR otherwise
 */
mesa_rc vtss_appl_stream_collection_conf_del(vtss_appl_stream_collection_id_t stream_collection_id);

/**
 * Iterate through created stream collections.
 * This function will iterate through all stream collections found in the table.
 *
 * The easiest way to iterate through all stream collections is to:
 *    vtss_appl_stream_collection_id_t stream_collection_id = VTSS_APPL_STREAM_COLLECTION_ID_NONE;
 *    while (vtss_appl_stream_collection_itr(&stream_collection_id, &stream_collection_id) == VTSS_RC_OK) {
 *        // Do what is needed.
 *    }
 *
 * \param prev_stream_collection_id [IN]  Previous stream collection ID.
 * \param next_stream_collection_id [OUT] Next stream collection ID.
 *
 * \return VTSS_RC_OK if a next entry was found, VTSS_RC_ERROR otherwise.
 */
mesa_rc vtss_appl_stream_collection_itr(const vtss_appl_stream_collection_id_t *prev_stream_collection_id, vtss_appl_stream_collection_id_t *next_stream_collection_id);

/**
 * Set a client action.
 *
 * Clients attach to this stream collection and assigns various stream actions.
 *
 * \param stream_collection_id [IN] Stream ID.
 * \param client               [IN] Client ID.
 * \param action               [IN] Client's action.
 * \param frer_seq_gen_reset   [IN] If true, \p action is not used, but the stream's IFLOW gets re-applied. This is in order to reset the FRER sequence number generator.
 *
 * \return VTSS_RC_OK if attachment succeeded.
 */
mesa_rc vtss_appl_stream_collection_action_set(vtss_appl_stream_collection_id_t stream_collection_id, vtss_appl_stream_client_t client, vtss_appl_stream_action_t *action, bool frer_seq_gen_reset = false);

/**
 * This enum holds flags that indicate various configurational warnings.
 */
typedef enum {
    VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NONE                                         = 0x000, /**< No warnings found.                                                      */
    VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_STREAMS_ATTACHED                          = 0x001, /**< No stream attached                                                      */
    VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_CLIENTS_ATTACHED                          = 0x002, /**< No clients attached.                                                    */
    VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_AT_LEAST_ONE_STREAM_HAS_OPERATIONAL_WARNINGS = 0x004, /**< At least one of the streams in this collection has operational warnings */
} vtss_appl_stream_collection_oper_warnings_t;

/**
 * Operators for vtss_appl_stream_collection_oper_warnings_t flags.
 */
VTSS_ENUM_BITWISE(vtss_appl_stream_collection_oper_warnings_t);

/**
 * Contains operational warnings and identifies clients attached to a stream
 * collection.
 */
typedef struct {
    /**
     * Configurational warnings.
     */
    vtss_appl_stream_collection_oper_warnings_t oper_warnings;

    /**
     * Status of the clients attached to this stream collection.
     */
    vtss_appl_stream_client_status_t client_status;
} vtss_appl_stream_collection_status_t;

/**
 * Function for getting the status of a stream collection.
 *
 * \param stream_collection_id [IN]  A stream ID
 * \param status               [OUT] The status of the stream
 *
 * \return VTSS_RC_OK if the stream collection exists.
 */
mesa_rc vtss_appl_stream_collection_status_get(vtss_appl_stream_collection_id_t stream_collection_id, vtss_appl_stream_collection_status_t *status);

#endif  // _VTSS_APPL_STREAM_H_

