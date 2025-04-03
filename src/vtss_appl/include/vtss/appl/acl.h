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

/**
 * \file acl.h
 * \brief Public ACL APIs
 * \details This is the header file of the ACL module. The ACL(Access Control List)
 *          is a prioritized list table that contains ACEs(Access Control Entries),
 *          each ACE can be used to determine the access rights of specific traffic.
 *          For example, permitting Telnet frames or filtering non-IP frames.
 *          An incoming packet will only hit the first matched ACE according to
 *          the ACE precedence. If the incoming packet doesn't hit any existing
 *          ACEs, it will use the incoming port ACL setting.
 *          A set of specific ACEs can be grouped into a policy ID and several
 *          ports configuration refer to the same policy ID. It is useful to
 *          group ports to obey the same traffic rules and can save the hardware
 *          resource too.
 *          An ACE also can be associated with a ACL rate limiter ID to limit the traffic
 *          rate. The rate limiter support PPS(Packets Per Seconds) rate or BPS
 *          (Bit Per Seconds) rate, depend on the hardware capabilities.
 *          ACL implementations can be quite complex, it deped on the various
 *          real netwrok situation. Generally, ACL is used to control the ingress
 *          traffic, similar to firewalls.
 */

#ifndef _VTSS_APPL_ACL_H_
#define _VTSS_APPL_ACL_H_

#include <vtss/appl/module_id.h>    // MODULE_ERROR_START()
#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/vcap_types.h>   // VCAP types

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    /** Generic error code              */
    VTSS_APPL_ACL_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_ACL),

    /** isid parameter is non-existing. */
    VTSS_APPL_ACL_ERROR_ISID_NON_EXISTING,

    /** Illegal parameter               */
    VTSS_APPL_ACL_ERROR_PARM,

    /** Timeout on message request      */
    VTSS_APPL_ACL_ERROR_REQ_TIMEOUT,

    /** Illegal primary/secondary switch state */
    VTSS_APPL_ACL_ERROR_STACK_STATE,

    /** ACE not found                   */
    VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND,

    /** ACE already existing            */
    VTSS_APPL_ACL_ERROR_ACE_ALREDY_EXIST,

    /** ACE table full                  */
    VTSS_APPL_ACL_ERROR_ACE_TABLE_FULL,

    /** ACL user ID not found           */
    VTSS_APPL_ACL_ERROR_USER_NOT_FOUND,

    /** Allocate memory fail            */
    VTSS_APPL_ACL_ERROR_MEM_ALLOC_FAIL,

    /** ACE auto-assigned fail          */
    VTSS_APPL_ACL_ERROR_ACE_AUTO_ASSIGNED_FAIL,

    /** Unknown ACE type                */
    VTSS_APPL_ACL_ERROR_UNKNOWN_ACE_TYPE
};

/** Disable ACL policer */
#define VTSS_APPL_ACL_POLICER_DISABLED      0

/** Maximum length of ACL user name string */
#define VTSS_APPL_ACL_USER_MAX_LEN          31

/**
 * \brief ACE ID
 */
typedef uint32_t vtss_appl_acl_ace_id_t;

/**
 * \brief ACE counter
 */
typedef uint32_t vtss_appl_acl_ace_counter_t;

/**
 * \brief ACL capabilities
 */
typedef struct {
    /** Maximum ACE ID */
    uint32_t     acl_ace_id_max;

    /** Maximum policy ID */
    uint32_t     acl_policy_id_max;

    /** Maximum rate limiter ID */
    uint32_t     acl_rate_limiter_id_max;

    /** Maximum EVC policer ID. Obsolete. Always 0 */
    uint32_t     acl_evc_policer_id_max;

    /** Indicates if bit rate setting in rate limiter is supported */
    mesa_bool_t    rate_limiter_bit_rate_supported;

    /** Inidicates if EVC policer field in action configuration is supported. Obsolete. Always false */
    mesa_bool_t    action_evc_policer_supported;

    /** Inidicate if mirror field in action configuration is supported */
    mesa_bool_t    action_mirror_supported;

    /** Inidicate if setting multiple redirect ports is supported.
      * If not supported, only single port is allowed. */
    mesa_bool_t    action_multiple_redirect_ports_supported;

    /** Inidicate if second_lookup field is supported */
    mesa_bool_t    ace_second_lookup_supported;

    /** Inidicate if setting multiple ingress ports is supported.
      * If not supported, only single port is allowed. */
    mesa_bool_t    ace_multiple_ingress_ports_supported;

    /** Inidicate if egress ports is supported.
      * If not supported, it means all ports are egress ports. */
    mesa_bool_t    ace_egress_port_supported;

    /** Inidicate if VLAN tagged field is supported */
    mesa_bool_t    ace_vlan_tagged_supported;

} vtss_appl_acl_capabilities_t;

/**
 * \brief ACL policer configuration
 */
typedef struct {
    /** Use bit rate policing instead of packet rate. */
    mesa_bool_t                bit_rate_enable;

    /** Bit rate in kbps. */
    mesa_bitrate_t      bit_rate;

    /** Packet rate in pps. */
    mesa_packet_rate_t  packet_rate;
} vtss_appl_acl_config_rate_limiter_t;

/**
 * \brief ACL ACE frame type
 */
typedef enum
{
    VTSS_APPL_ACL_ACE_FRAME_TYPE_ANY,     /*!< Any frame type */
    VTSS_APPL_ACL_ACE_FRAME_TYPE_ETYPE,   /*!< Ethernet Type */
    VTSS_APPL_ACL_ACE_FRAME_TYPE_LLC,     /*!< LLC */
    VTSS_APPL_ACL_ACE_FRAME_TYPE_SNAP,    /*!< SNAP */
    VTSS_APPL_ACL_ACE_FRAME_TYPE_ARP,     /*!< ARP/RARP */
    VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV4,    /*!< IPv4 */
    VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV6     /*!< IPv6 */
} vtss_appl_acl_ace_frame_type_t;

/**
 * \brief ACL ACE hit action
 */
typedef enum {
    /** Permit action for the ACE, the incoming frame will be forwarded to
      * the corresponding destination ports */
    VTSS_APPL_ACL_HIT_ACTION_PERMIT,

    /** Deny action for the ACE, the incoming frame will be discarded
      * and does not forward to others ports. */
    VTSS_APPL_ACL_HIT_ACTION_DENY,

    /** Redirect action for the ACE, the incoming frames will be
      * redirected to the redirect ports */
    VTSS_APPL_ACL_HIT_ACTION_REDIRECT,

    /** Egress action for the ACE, the action is to define what ports
      * can be egress ports. Currently supported only on interface */
    VTSS_APPL_ACL_HIT_ACTION_EGRESS,

    /** enum count */
    VTSS_APPL_ACL_ACE_HIT_ACTION_CNT
} vtss_appl_acl_hit_action_t;

/** \brief ACL ACE ingress type */
typedef enum {
    /*!< Indicates this ACE is applied on all switches and all ports.
         In this case, none of the ACE parameter ingress_list, ingress_switch,
         and ingress_switchport are used. */
    VTSS_APPL_ACL_ACE_INGRESS_MODE_ANY,

    /*!< Indicates this ACE is applied on specific switch and specific ports.
         In this case, the ACE parameter of ingress_port field is used and
         the other is ignored. The ACE parameter of ingress_switch
         and ingress_switchport are empty. */
    VTSS_APPL_ACL_ACE_INGRESS_MODE_SPECIFIC,

    /*!< Indicates this ACE is applied on all ports of specific switch in
         stackable devices. In this case, the ACE parameter of ingress_switch
         is used and the other is ignored. The ACE parameter of ingress_port
         and ingress_switchport are empty. */
    VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCH,

    /*!< Indicates this ACE is applied on specific switch port of all
         switches in stackable devices. In this case, the ACE parameter of 
         ingress_switchport is used and the other is ignored. The ACE
         parameter of ingress_port and ingress_switch are empty. */
    VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCHPORT,

    /*!< enum count */
    VTSS_APPL_ACL_ACE_INGRESS_MODE_CNT
} vtss_appl_acl_ace_ingress_mode_t;

/** \brief ACL ACE VLAN tagged */
typedef enum {
    VTSS_APPL_ACL_ACE_VLAN_TAGGED_ANY,  /*!< Match both untagged and tagged frames */
    VTSS_APPL_ACL_ACE_VLAN_UNTAGGED,    /*!< Match untagged frames only */
    VTSS_APPL_ACL_ACE_VLAN_TAGGED,      /*!< Match tagged frames only */
    VTSS_APPL_ACL_ACE_VLAN_TAGGED_CNT   /*!< enum count */
} vtss_appl_acl_ace_vlan_tagged_t;

/** \brief ACL ACE ARP opcode */
typedef enum {
    VTSS_APPL_ACL_ACE_ARP_OP_ANY,      /*!< Match ARP any opcode frames */
    VTSS_APPL_ACL_ACE_ARP_OP_ARP,      /*!< Match ARP arp opcode only */
    VTSS_APPL_ACL_ACE_ARP_OP_RARP,     /*!< Match ARP rarp opcpode frames only */
    VTSS_APPL_ACL_ACE_ARP_OP_OTHER,    /*!< Match ARP others opcode frames only */
    VTSS_APPL_ACL_ACE_ARP_OP_CNT       /*!< enum count */
} vtss_appl_acl_ace_arp_op_t;

/**
 * \brief ACL ACE action parameters
 */
typedef struct {
    /** ACE hit action */
    vtss_appl_acl_hit_action_t      hit_action;

    /** Frames that hit the ACE are redirected to these ports
      * when hit action is deny. */
    vtss_port_list_stackable_t      redirect_port;

    /** Indicates the redirect switch port. This field refers to the
        setting of ingress type(VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCHPORT).
        It is used when this ACE is applied on specific switch port of
        all switches in stackable devices. */
    mesa_port_no_t                  redirect_switchport;

    /** The port list is to define what ports are allowed to be egress ports.
      * If the egress port of an incoming frame is in the port list then the
      * frame will be forwared. Otherwise, if the egress port of an incoming
      * frame is not in the port list then it means the egress port is not
      * allowed and the incoming frame will be dropped. Currently supported
      * only on interface config. */
    vtss_port_list_stackable_t      egress_port;

    /** Rate limiter ID, VTSS_APPL_ACL_POLICER_DISABLED to disable  */
    mesa_acl_policer_no_t           rate_limiter_id;

    /** Enable/Disable mirroring. If the mirror feature is enabled and the
      * destination mirror port is set. Frames that hit the ACE are mirrored
      * to the destination mirror port. */
    mesa_bool_t                            mirror;

    /** The frame information (length, ingress port and content) that hit the
      * ACE are logged as an informational message. */
    mesa_bool_t                            logging;

    /** Port shut down */
    mesa_bool_t                            shutdown;
} vtss_appl_acl_action_t;

/**
 * \brief ACL ACE VLAN parameters
 */
typedef struct {
    /** VLAN ID from user view. Possible values are: 0(disabled), 1-4095 */
    mesa_vid_t                          vid;

    /** VLAN user priority filter mapping */
    vtss_appl_vcap_vlan_pri_type_t      usr_prio;

     /** VLAN tagged */
     vtss_appl_acl_ace_vlan_tagged_t    tagged;
} vtss_appl_acl_ace_vlan_t;

/**
 * \brief ACL ACE Ethernet type parameters
 */
typedef struct
{
    vtss_appl_vcap_as_type_t        smac_type;  /*!< Ethernet type source MAC address type */
    mesa_mac_t                      smac;       /*!< Ethernet type source MAC address */
    mesa_mac_t                      dmac;       /*!< Ethernet type destination MAC address */
    mesa_etype_t                    etype;      /*!< Ethernet type value */
} vtss_appl_acl_ace_frame_ether_t;

/**
 * \brief ACL ACE ARP flags
 */
typedef struct
{
    vtss_appl_acl_ace_arp_op_t  opcode; /*!< ARP opcode */
    mesa_vcap_bit_t             req;    /*!< ARP request/reply opcode */
    mesa_vcap_bit_t             sha;    /*!< ARP sender hardware address (SHA) */
    mesa_vcap_bit_t             tha;    /*!< ARP target hardware address (THA) */
    mesa_vcap_bit_t             hln;    /*!< ARP hardware address length (HLN) */
    mesa_vcap_bit_t             hrd;    /*!< ARP hardware address space (HRD) */
    mesa_vcap_bit_t             pro;    /*!< ARP protocol address space (PRO) */
} vtss_appl_acl_ace_arp_flags_t;

/**
 * \brief ACL ACE ARP parameters
 */
typedef struct
{
    vtss_appl_vcap_as_type_t        smac_type;  /*!< ARP source MAC address type */
    mesa_mac_t                      smac;       /*!< ARP source MAC address */
    mesa_vcap_ip_t                  sip;        /*!< ARP sender IP address */
    mesa_vcap_ip_t                  dip;        /*!< ARP target IP address */
    vtss_appl_acl_ace_arp_flags_t   flag;       /*!< ARP flags */
} vtss_appl_acl_ace_frame_arp_t;

/**
 * \brief ACL ACE IPv4 flags
 */
typedef struct
{
    /** IPv4 frames with a Time-to-Live field */
    mesa_vcap_bit_t     ttl;

    /** More Fragments (MF) bit and the Fragment Offset (FRAG OFFSET) field for an IPv4 frame */
    mesa_vcap_bit_t     fragment;

    /** IP option */
    mesa_vcap_bit_t     option;
} vtss_appl_acl_ace_ipv4_flag_t;

/**
 * \brief ACL ACE ICMP
 */
typedef struct
{
    vtss_appl_vcap_as_type_t    type_match; /*!< ICMP type match */
    uint8_t                          type;       /*!< ICMP type */
    vtss_appl_vcap_as_type_t    code_match; /*!< ICMP code match */
    uint8_t                          code;       /*!< ICMP code */
} vtss_appl_acl_ace_icmp_t;

/**
 * \brief ACL ACE TCP flags
 */
typedef struct
{
    mesa_vcap_bit_t     fin;    /*!< TCP FIN */
    mesa_vcap_bit_t     syn;    /*!< TCP SYN */
    mesa_vcap_bit_t     rst;    /*!< TCP RST */
    mesa_vcap_bit_t     psh;    /*!< TCP PSH */
    mesa_vcap_bit_t     ack;    /*!< TCP ACK */
    mesa_vcap_bit_t     urg;    /*!< TCP URG */
} vtss_appl_acl_ace_tcp_flag_t;

/**
 * \brief ACL ACE IPv4 parameters
 */
typedef struct
{
    vtss_appl_vcap_as_type_t        proto_type; /*!< IPv4 protocol type */
    uint8_t                              proto;      /*!< IPv4 protocol */
    mesa_vcap_ip_t                  sip;        /*!< IPv4 source IP address */
    mesa_vcap_ip_t                  dip;        /*!< IPv4 destination IP address */
    vtss_appl_acl_ace_icmp_t        icmp;       /*!< IPv4 ICMP parameters */
    vtss_appl_vcap_asr_t            sport;      /*!< IPv4 UDP/TCP: Source port */
    vtss_appl_vcap_asr_t            dport;      /*!< IPv4 UDP/TCP: Destination port */
    vtss_appl_acl_ace_ipv4_flag_t   ipv4_flag;  /*!< IPv4 flags */
    vtss_appl_acl_ace_tcp_flag_t    tcp_flag;   /*!< IPv4 TCP flags */
} vtss_appl_acl_ace_frame_ipv4_t;

/**
 * \brief ACL ACE IPv6 flags
 */
typedef struct
{
    mesa_vcap_bit_t         ttl;    /*!< IPv6 hop limit */
} vtss_appl_acl_ace_ipv6_flag_t;

/**
 * \brief ACL ACE IPv6 parameters
 */
typedef struct
{
    /** IPv6 next header type */
    vtss_appl_vcap_as_type_t        next_header_type;

    /** IPv6 next header */
    uint8_t                              next_header;

    /** IPv6 Source IP address (32 LSB) */
    vtss_appl_vcap_ipv6_t           sip;

    /** IPv6 ICMP parameters */
    vtss_appl_acl_ace_icmp_t        icmp;

    /** IPv6 UDP/TCP: Source port */
    vtss_appl_vcap_asr_t            sport;

    /** IPv6 UDP/TCP: Destination port */
    vtss_appl_vcap_asr_t            dport;

    /** IPv6 flags */
    vtss_appl_acl_ace_ipv6_flag_t   ipv6_flag;

    /** IPv6 TCP flags */
    vtss_appl_acl_ace_tcp_flag_t    tcp_flag;
} vtss_appl_acl_ace_frame_ipv6_t;

/**
 * \brief ACL ACE frame para,eters
 */
typedef struct {
    /** Type of destination MAC address
      * If frame type is ANY and ACL is V2, then it is not supported.
      * Specific type is supported only if frame type is ether. */
    vtss_appl_vcap_adv_dmac_type_t      dmac_type;

    /** Frame type */
    vtss_appl_acl_ace_frame_type_t      frame_type;

    /** Ethernet frame (Etype >= 0x600) */
    vtss_appl_acl_ace_frame_ether_t     ether;

    /** ARP frame type */
    vtss_appl_acl_ace_frame_arp_t       arp;

    /** IPv4 frame type */
    vtss_appl_acl_ace_frame_ipv4_t      ipv4;

    /** IPv6 frame type */
    vtss_appl_acl_ace_frame_ipv6_t      ipv6;
} vtss_appl_acl_ace_frame_t;

/**
 * \brief ACL ACE lookup key
 */
typedef struct {
    /** Indicates the ingress port list mode */
    vtss_appl_acl_ace_ingress_mode_t    ingress_mode;

    /** Indicates the ingress ports. This field refers to the setting
        of ingress type(VTSS_APPL_ACL_ACE_INGRESS_MODE_SPECIFIC).
        It is used when this ACE is applied on specific switch and
        specific ports. */
    vtss_port_list_stackable_t          ingress_port;

    /** Indicates the ingress switch ID. This field refers to the 
        setting of ingress type(VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCH).
        It is used when this ACE is applied on all ports of specific
        switch in stackable device. */
    vtss_usid_t                         ingress_switch;

    /** Indicates the ingress switch port. This field refers to the
        setting of ingress type(VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCHPORT).
        It is used when this ACE is applied on specific switch port of
        all switches in stackable devices. */
    mesa_port_no_t                      ingress_switchport;

    /** Policy number
      * For saving the H/W ACE rule, a set of specific ACEs can be
      * grouped into a policy ID and several ports configuration refer to the
      * same policy ID. */
    mesa_vcap_u8_t                      policy;

    /** If an incoming packet hits a rule that does not filter packet and
      * the second lookup of the hit rule is enabled, then ACL will continue
      * to lookup the packet to find the second hit rule. If the second hit
      * rule is found, then the packet is applied on the second hit rule.
      * Example A:
      *     Rule 1 Frame Any, Ingress port 1-3, Action Permit, with Second_Lookup
      *     Rule 2 Frame Any, Ingress port 1,   Action Filter
      *
      *     The packets from port 1 will be filtered because they hit rule 1
      *     and rule 2, where rule 2 is with filter action. The packets from
      *     port 2 and 3 will be forwarded because they hit rule 1 only.
      *
      * Example B:
      *     Rule 1 Frame Any, Ingress port 1-3, Action Filter, with Second_Lookup
      *     Rule 2 Frame Any, Ingress port 1,   Action Permit
      *
      *     The packets from port 1 will be filtered because they hit rule 1
      *     and rule 1 filters packets already. The packets from port 2 and 3
      *     will be filtered with the same reason.
      *
      *     In other words, in this case, rule 2 does not take any effect.
      */
    mesa_bool_t                                second_lookup;

    /** VLAN parameters */
    vtss_appl_acl_ace_vlan_t            vlan;

    /** Frame parameters */
    vtss_appl_acl_ace_frame_t           frame;
} vtss_appl_acl_ace_key_t;

/**
 * \brief ACL ACE configuration
 * The configuration of ACE(Access control Entry). Frames that hit the ACE
 * need to match the various parameters. (Ingress port, Frame type, VID and
 * etc.)
 */
typedef struct {
    /** Next ACE ID, this will indicate the precedence of ACEs,
      * that is, this ACE will be located before next_ace_id */
    vtss_appl_acl_ace_id_t          next_ace_id;

    /** Action parameters */
    vtss_appl_acl_action_t          action;

    /** Frame key. NOTE: Contains ALL frame types */
    vtss_appl_acl_ace_key_t         key;
} vtss_appl_acl_config_ace_t;

/**
 * \brief ACL interface configuration
 * The default port configuration of ACL setting. The port default setting is
 * used for the incoming frame if it doesn't hit any existing ACEs.
 */
typedef struct {
    /** Policy ID. For saving the H/W ACE rule, a set of specific ACEs can be
      * grouped into a policy ID and several ports configuration refer to the
      * same policy ID. */
    mesa_acl_policy_no_t                policy_id;

    /** Interface action parameters */
    vtss_appl_acl_action_t              action;
} vtss_appl_acl_config_interface_t;

/**
 * \brief ACL ACE hit count
 */
typedef struct {
    vtss_appl_acl_ace_counter_t         counter;    /*!< ACE hit count */
} vtss_appl_acl_status_ace_hit_count_t;

/**
 * \brief ACE precedence status. An incoming packet will only hit
 *        the first matched ACE according to the ACE precedence.
 *        It displays the sequence of ACEs to be hit.
 */
typedef struct {
    vtss_appl_acl_ace_id_t      ace_id;  /*!< ACE ID */
} vtss_appl_acl_config_ace_precedence_t;

/**
 * \brief ACE status
 */
typedef struct {
    /** ACE ID */
    vtss_appl_acl_ace_id_t      ace_id;

    /** ACL user, indicates which user create this rule.
      * It could be created by administer or internal software modules. */
    char                        acl_user[VTSS_APPL_ACL_USER_MAX_LEN + 1];

    /** Indicates the hardware status of the specific ACE.
      * The specific ACE is not applied to the hardware due to
      * hardware limitations. */
    mesa_bool_t                        conflict;
} vtss_appl_acl_status_ace_t;

/**
 * \brief ACL interface hit count status
 */
typedef struct {
    /** Interface hit count */
    vtss_appl_acl_ace_counter_t     counter;
} vtss_appl_acl_status_interface_hit_count_t;

/**
 * \brief ACE event status
 */
typedef struct {
    /** Indicates the ACE status is crossed the
      * hardware threshold or not. */
    mesa_bool_t    crossed_threshold;
} vtss_appl_acl_status_ace_event_t;

/**
 * \brief ACL global contol
 */
typedef struct {
    /** Clear all ACL hit count */
    mesa_bool_t    clear_all_hit_count;
} vtss_appl_acl_control_globals_t;

/**
 * \brief ACL interface control
 */
typedef struct {
    /** Enable or disable port state.
      * It can be used to re-enable the port feature if a specific
      * port is shutdown by ACE or interface rule.
      * It also can force the port to be disabled.
      */
    mesa_bool_t    state;
} vtss_appl_acl_control_interface_t;

/**
 * \brief Get ACL capabilities to see what supported or not
 *
 * \param cap [OUT] ACL capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_capabilities_get(
    vtss_appl_acl_capabilities_t    *const cap
);

/**
 * \brief ACL rate limiter iterate function,
 *
 * \param prev_rate_limiter_id [IN]  previous rate limter id.
 * \param next_rate_limiter_id [OUT] next rate limiter id.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_rate_limiter_itr(
    const mesa_acl_policer_no_t     *const prev_rate_limiter_id,
    mesa_acl_policer_no_t           *const next_rate_limiter_id
);

/**
 * \brief Get ACL rate limiter configuration.
 *        It is used to configure the rate limiter for received frames and
 *        the rate limiter ID can be associated with an ACE.
 *
 * \param rate_limiter_id [IN]  Rate limiter ID
 * \param conf            [OUT] Rate limiter configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_rate_limiter_get(
    mesa_acl_policer_no_t                   rate_limiter_id,
    vtss_appl_acl_config_rate_limiter_t     *const conf
);

/**
 * \brief Set ACL rate limiter configuration.
 *        It is used to configure the rate limiter for received frames and
 *        the rate limiter ID can be associated with an ACE
 *        to limit the frame received rate.
 *
 * \param rate_limiter_id [IN] Rate limiter ID
 * \param conf            [IN] Rate limiter configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_rate_limiter_set(
    mesa_acl_policer_no_t                       rate_limiter_id,
    const vtss_appl_acl_config_rate_limiter_t   *const conf
);

/**
 * \brief Iterate function of ACE config table.
 *
 * \param prev_ace_id [IN]  previous ACE ID.
 * \param next_ace_id [OUT] next ACE ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_itr(
    const vtss_appl_acl_ace_id_t    *const prev_ace_id,
    vtss_appl_acl_ace_id_t          *const next_ace_id
);

/**
 * \brief Get ACL ACE configuration.
 *
 * \param ace_id [IN]  ACE ID.
 * \param conf   [OUT] ACE configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_get(
    vtss_appl_acl_ace_id_t          ace_id,
    vtss_appl_acl_config_ace_t      *const conf
);

/**
 * \brief Set ACL ACE configuration.
 *
 * \param ace_id [IN] ACE ID.
 * \param conf   [IN] ACE configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_set(
    vtss_appl_acl_ace_id_t              ace_id,
    const vtss_appl_acl_config_ace_t    *const conf
);

/**
 * \brief Add ACL ACE configuration.
 *
 * \param ace_id [IN] ACE ID.
 * \param conf   [IN] ACE configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_add(
    vtss_appl_acl_ace_id_t              ace_id,
    const vtss_appl_acl_config_ace_t    *const conf
);

/**
 * \brief Delete ACL ACE configuration.
 *
 * \param ace_id [IN] ACE ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_del(
    vtss_appl_acl_ace_id_t  ace_id
);

/**
 * \brief Add ACL ACE default configuration.
 *
 * \param ace_id [IN] ACE ID.
 * \param conf   [IN] ACE configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_default(
    vtss_appl_acl_ace_id_t      *const ace_id,
    vtss_appl_acl_config_ace_t  *const conf
);

/**
 * \brief Iterate function of interface config table.
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_interface_itr(
    const vtss_ifindex_t    *prev_ifindex,
    vtss_ifindex_t          *next_ifindex
);

/**
 * \brief Get ACL interface configuration.
 *        The configuration will affect frames received on a port unless the
 *        frame matches a specific ACE, i.e. if a incoming frame doesn't hit
 *        any existing ACE, it will follows the ACL rules of received port.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param conf    [OUT] The data point of configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_interface_get(
    vtss_ifindex_t                      ifindex,
    vtss_appl_acl_config_interface_t    *const conf
);

/**
 * \brief Set ACL interface configuration.
 *        The configuration will affect frames received on a port unless the
 *        frame matches a specific ACE, i.e. if a incoming frame doesn't hit
 *        any existing ACE, it will follows the ACL rules of received port.
 *
 * \param ifindex [IN] ifindex of port.
 * \param conf    [IN] The data point of configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_interface_set(
    vtss_ifindex_t                          ifindex,
    const vtss_appl_acl_config_interface_t  *const conf
);

/**
 * \brief ACE precedence iterate function
 *
 * \param prev_precedence [IN]  previous precedence.
 * \param next_precedence [OUT] next precedence.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_precedence_itr(
    const uint32_t   *const prev_precedence,
    uint32_t         *const next_precedence
);

/**
 * \brief Get ACL ACE precedence status. An incoming packet will only hit
 *        the first matched ACE according to the ACE precedence.
 *
 * \param precedence [IN]  precedence.
 * \param status     [OUT] precedence tatus.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_config_ace_precedence_get(
    uint32_t                                     precedence,
    vtss_appl_acl_config_ace_precedence_t   *const status
);

/**
 * \brief ACE status iterate function, it is used to get first and
 *        get next indexes.
 *
 * \param prev_usid       [IN]  previous switch ID.
 * \param next_usid       [OUT] next switch ID.
 * \param prev_precedence [IN]  previous precedence.
 * \param next_precedence [OUT] next precedence.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid,
    const uint32_t           *const prev_precedence,
    uint32_t                 *const next_precedence
);

/**
 * \brief Get ACL ACE status.
 *
 * \param usid       [IN]  switch ID for user view (The value starts from 1).
 * \param precedence [IN]  precedence.
 * \param status     [OUT] ACE status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_get(
    vtss_usid_t                     usid,
    uint32_t                             precedence,
    vtss_appl_acl_status_ace_t      *const status
);

/**
 * \brief Iterate function of ACE hit count.
 *
 * \param prev_ace_id [IN]  previous ACE ID.
 * \param next_ace_id [OUT] next ACE ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_hit_count_itr(
    const vtss_appl_acl_ace_id_t    *const prev_ace_id,
    vtss_appl_acl_ace_id_t          *const next_ace_id
);

/**
 * \brief Get ACL ACE hit count.
 *
 * \param ace_id [IN]  ACE ID.
 * \param status [OUT] hit count status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_hit_count_get(
    vtss_appl_acl_ace_id_t                  ace_id,
    vtss_appl_acl_status_ace_hit_count_t    *const status
);

/**
 * \brief Iterate function of interface hit count.
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_interface_hit_count_itr(
    const vtss_ifindex_t    *prev_ifindex,
    vtss_ifindex_t          *next_ifindex
);

/**
 * \brief Get ACL interface hit count. If an incoming frame doesn't hit any
 *        existing ACE, the received port ACL hit count will be increased.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param status  [OUT] hit count status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_interface_hit_count_get(
    vtss_ifindex_t                                  ifindex,
    vtss_appl_acl_status_interface_hit_count_t      *const status
);

/**
 * \brief Get ACL global control.
 *
 * \param control [OUT]: global control.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_globals_get(
    vtss_appl_acl_control_globals_t     *const control
);

/**
 * \brief Set ACL global control. It is used to clear all ACL hit count.
 *
 * \param control [IN] global control.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_globals_set(
    const vtss_appl_acl_control_globals_t   *const control
);

/**
 * \brief Iterate function of interface control table.
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_interface_itr(
    const vtss_ifindex_t    *prev_ifindex,
    vtss_ifindex_t          *next_ifindex
);

/**
 * \brief Get ACL interface control.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param control [OUT] interface control data.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_interface_get(
    vtss_ifindex_t                      ifindex,
    vtss_appl_acl_control_interface_t   *const control
);

/**
 * \brief Set ACL interface control.
 *
 * \param ifindex [IN] ifindex of port.
 * \param control [IN] interface control data.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_control_interface_set(
    vtss_ifindex_t                              ifindex,
    const vtss_appl_acl_control_interface_t     *const control
);

/**
 * \brief ACE event status iterate function, it is used to get first and
 *        get next indexes.
 *
 * \param prev_usid       [IN]  previous switch ID.
 * \param next_usid       [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_event_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
);

/**
 * \brief Get ACL ACE event status.
 *
 * \param usid       [IN]  switch ID for user view (The value starts from 1).
 * \param status     [OUT] ACE event status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_event_get(
    vtss_usid_t                         usid,
    vtss_appl_acl_status_ace_event_t    *const status
);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_ACL_H_ */

