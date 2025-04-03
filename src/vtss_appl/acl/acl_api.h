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

/**
 * \file acl_api.h
 * \brief This file defines the APIs for the ACL module
 */
#ifndef _VTSS_ACL_API_H_
#define _VTSS_ACL_API_H_

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */
#include "main.h"     /* For MODULE_ERROR_START()      */
#include <vtss/appl/acl.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
 - Platform dependence variables
***************************************************************************/
/* ACL policer numbers */
#define ACL_MGMT_RATE_LIMITER_NONE      0xFFFFFFFF                         /*!< ACL policer disabled */
#define ACL_MGMT_RATE_LIMITER_NO_START  0                                  /*!< ACL policer start number */
#define ACL_MGMT_RATE_LIMITER_NO_END    fast_cap(MESA_CAP_ACL_POLICER_CNT) /*!< ACL policer end number */

#define ACL_MGMT_POLICY_NO_MAX          (fast_cap(MESA_CAP_ACL_POLICY_CNT) - 1)
#define ACL_MGMT_POLICIES_BITMASK       ACL_MGMT_POLICY_NO_MAX     /*!< ACL policies bitmask */

/* ACE ID numbers */
#define ACL_MGMT_ACE_ID_NONE            0   /*!< Reserved */
#define ACL_MGMT_ACE_ID_START           1   /*!< First ACE ID */

#define ACL_MGMT_ACE_MAX                acl_cap_ace_cnt_get()
#define ACL_MGMT_ACE_ID_END             (ACL_MGMT_ACE_MAX)                   /*!< Last ACE ID */

#define ACL_MGMT_PACKET_RATE_MAX                     fast_cap(MESA_CAP_ACL_POLICER_PACKET_RATE_MAX)
#define ACL_MGMT_PACKET_RATE_GRANULARITY             fast_cap(MESA_CAP_ACL_POLICER_PACKET_RATE_GRAN)
#define ACL_MGMT_PACKET_RATE_GRANULARITY_SMALL_RANGE fast_cap(MESA_CAP_ACL_POLICER_PACKET_RATE_SMALL)
#define ACL_MGMT_BIT_RATE_MAX                        fast_cap(MESA_CAP_ACL_POLICER_BIT_RATE_MAX)
#define ACL_MGMT_BIT_RATE_GRANULARITY                fast_cap(MESA_CAP_ACL_POLICER_BIT_RATE_GRAN)

/***************************************************************************
 - ACL module options
***************************************************************************/
#define ACL_IPV6_SUPPORTED      1   /**< Define it if IPv6 ACE is supported */

uint32_t acl_cap_ace_cnt_get(void);

/**
 * \brief ACL users declaration.
 */
typedef enum {
    ACL_USER_STATIC = 0,

    /*
     * Add your new ACL user below.
     * Be careful the location of your new ACL user.
     * The enum value also used to decide the order in ACL.
     */

    /*
     * The rule of IP management must be the last.
     */
#if defined(VTSS_SW_OPTION_IP_MGMT_ACL)
    ACL_USER_IP_MGMT,           /*!< ACL user for IP management */
#endif

#ifdef VTSS_SW_OPTION_IP_SOURCE_GUARD
    ACL_USER_IP_SOURCE_GUARD,   /*!< ACL user for IP source guard */
#endif

#ifdef VTSS_SW_OPTION_IPV6_SOURCE_GUARD
    ACL_USER_IPV6_SOURCE_GUARD,   /*!< ACL user for IPv6 source guard */
#endif

#if defined(VTSS_SW_OPTION_IP)
    ACL_USER_IP,                /*!< ACL user for IP module */
#endif

#if defined(VTSS_SW_OPTION_IPMC) || defined(VTSS_SW_OPTION_IGMPS) || defined(VTSS_SW_OPTION_MLDSNP)
    ACL_USER_IPMC,              /*!< ACL user for IPMC */
#endif

#ifdef VTSS_SW_OPTION_CFM
    ACL_USER_CFM,               /*!< ACL user for CFM */
#endif

#ifdef VTSS_SW_OPTION_APS
    ACL_USER_APS,               /*!< ACL user for APS */
#endif

#ifdef VTSS_SW_OPTION_ERPS
    ACL_USER_ERPS,              /*!< ACL user for ERPS */
#endif

#ifdef VTSS_SW_OPTION_IEC_MRP
    ACL_USER_IEC_MRP,              /*!< ACL user for MRP */
#endif

#ifdef VTSS_SW_OPTION_REDBOX
    ACL_USER_REDBOX,               /*!< ACL user for REDBOX */
#endif

#ifdef VTSS_SW_OPTION_ARP_INSPECTION
    ACL_USER_ARP_INSPECTION,    /*!< ACL user for ARP inspection */
#endif

#ifdef VTSS_SW_OPTION_UPNP
    ACL_USER_UPNP,              /*!< ACL user for UPNP */
#endif

#if defined(VTSS_SW_OPTION_PTP) || defined(VTSS_SW_OPTION_PHY_1588_SIM)
    ACL_USER_PTP,               /*!< ACL user for PTP */
#endif

#ifdef VTSS_SW_OPTION_DHCP_HELPER
    ACL_USER_DHCP,              /*!< ACL user for DHCP */
#endif

#ifdef VTSS_SW_OPTION_DHCP6_SNOOPING
    ACL_USER_DHCPV6_SNOOP,      /*!< ACL user for DHCPv6 snooping */
#endif

#ifdef VTSS_SW_OPTION_LOOP_PROTECTION
    ACL_USER_LOOP_PROTECT,      /*!< ACL user loop protect */
#endif

    /*
     * The rule of link OAM must be the first.
     */
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
    ACL_USER_LINK_OAM,          /*!< ACL user for OAM */
#endif

#ifdef VTSS_SW_OPTION_ZTP
    ACL_USER_ZTP,               /*!< ACL user for ACL */
#endif

    ACL_USER_TEST,              /*!< ACL user for test, always present */

    ACL_USER_CNT
} acl_user_t;

/**
 * \brief ACE bit flags
 */
enum {
    ACE_FLAG_DMAC_BC = 0,   /**> DMAC with broadcast address */
    ACE_FLAG_DMAC_MC,       /**> DMAC with multicast address */
    ACE_FLAG_VLAN_CFI,      /**< VLAN CFI */
    ACE_FLAG_ARP_ARP,       /**< ARP/RARP */
    ACE_FLAG_ARP_REQ,       /**< Request/Reply ARP frame */
    ACE_FLAG_ARP_UNKNOWN,   /**< Unknown ARP frame */
    ACE_FLAG_ARP_SMAC,      /**< ARP flag: Sender hardware address (SHA) */
    ACE_FLAG_ARP_DMAC,      /**< ARP flag: Target hardware address (THA) */
    ACE_FLAG_ARP_LEN,       /**< ARP flag: Hardware address length (HLN) */
    ACE_FLAG_ARP_IP,        /**< ARP flag: Hardware address space (HRD) */
    ACE_FLAG_ARP_ETHER,     /**< ARP flag: Protocol address space (PRO) */
    ACE_FLAG_IP_TTL,        /**< IPv4 frames with a Time-to-Live field */
    ACE_FLAG_IP_FRAGMENT,   /**< More Fragments (MF) bit and the Fragment Offset (FRAG OFFSET) field for an IPv4 frame */
    ACE_FLAG_IP_OPTIONS,    /**< IP options */
    ACE_FLAG_TCP_FIN,       /**< TCP flag: No more data from sender (FIN) */
    ACE_FLAG_TCP_SYN,       /**< TCP flag: Synchronize sequence numbers (SYN) */
    ACE_FLAG_TCP_RST,       /**< TCP flag: Reset the connection (RST) */
    ACE_FLAG_TCP_PSH,       /**< TCP flag: Push Function (PSH) */
    ACE_FLAG_TCP_ACK,       /**< TCP flag: Acknowledgment field significant (ACK) */
    ACE_FLAG_TCP_URG,       /**< TCP flag: Urgent Pointer field significant (URG) */
    ACE_FLAG_COUNT          /**< Last entry */
};
typedef int acl_flag_t;

#define ACE_FLAG_SIZE VTSS_BF_SIZE(ACE_FLAG_COUNT)

/* Reserved ACL policy numbers */
#define ACL_POLICY_IPV6_SOURCE_GUARD_ENTRY (ACL_MGMT_POLICY_NO_MAX - 17) /**< Ipv6 Source Guard Binding Entry */
#define ACL_POLICY_DHCPV6_SNOOP_CPU  (ACL_MGMT_POLICY_NO_MAX - 16) /**< DHCPv6 snooping redirect to CPU */
#define ACL_POLICY_DHCPV6_SNOOP_DROP (ACL_MGMT_POLICY_NO_MAX - 15) /**< DHCPv6 snooping drop packet*/
#define ACL_POLICY_LL_FAC_LOOP       (ACL_MGMT_POLICY_NO_MAX - 14) /**< Latching Loop Facility Loop */
#define ACL_POLICY_LL_FAC_DISC       (ACL_MGMT_POLICY_NO_MAX - 13) /**< Latching Loop Facility Discard */
#define ACL_POLICY_LL_TER_LOOP       (ACL_MGMT_POLICY_NO_MAX - 12) /**< Latching Loop Terminal Loop */
#define ACL_POLICY_LL_TER_DISC       (ACL_MGMT_POLICY_NO_MAX - 11) /**< Latching Loop Terminal Discard */
#define ACL_POLICY_TUNNEL_NNI_ANY    (ACL_MGMT_POLICY_NO_MAX - 10) /**< NNI Rx Cisco/Custom tunnel */
#define ACL_POLICY_TUNNEL_NNI_CUSTOM (ACL_MGMT_POLICY_NO_MAX - 9)  /**< NNI Rx Custom tunnel */
#define ACL_POLICY_TUNNEL_NNI_CISCO  (ACL_MGMT_POLICY_NO_MAX - 8)  /**< NNI Rx Cisco tunnel */
#define ACL_POLICY_TUNNEL_CUSTOM_40  (ACL_MGMT_POLICY_NO_MAX - 7)  /**< Custom tunnel, 40 bits */
#define ACL_POLICY_TUNNEL_CUSTOM     (ACL_MGMT_POLICY_NO_MAX - 6)  /**< Custom tunnel */
#define ACL_POLICY_TUNNEL_CISCO      (ACL_MGMT_POLICY_NO_MAX - 5)  /**< Cisco tunnel */
#define ACL_POLICY_CFM               (ACL_MGMT_POLICY_NO_MAX - 2)  /**< SW CFM OAM frame */
#define ACL_POLICY_CPU_REDIR         (ACL_MGMT_POLICY_NO_MAX - 1)  /**< Peer */
#define ACL_POLICY_DISCARD           (ACL_MGMT_POLICY_NO_MAX - 0)  /**< Discard */

/**
 * \brief ACL Action
 */
typedef struct {
    mesa_acl_policer_no_t   policer;                            /**< Policer number or VTSS_ACL_POLICY_NO_NONE */
    mesa_acl_port_action_t  port_action;                        /**< Port action */
    mesa_port_list_t        port_list;                          /**< Egress port list */
    BOOL                    mirror;                             /**< Enable mirroring */
    mesa_acl_ptp_action_t   ptp_action;                         /**< PTP action */
    mesa_acl_ptp_action_conf_t ptp;                             /**< PTP configuration */
    mesa_acl_addr_action_t  addr;                               /**< Address update */
    BOOL                    logging;                            /**< Logging */
    BOOL                    shutdown;                           /**< Port shut down */
    BOOL                    force_cpu;                          /**< Forward to CPU */
    BOOL                    cpu_once;                           /**< Only first frame forwarded to CPU */
    mesa_packet_rx_queue_t  cpu_queue;                          /**< CPU queue (if copied) */
    BOOL                    lm_cnt_disable;                     /**< Disable OAM LM Tx counting */
    BOOL                    inject_manual;                     /**< Inject the hit packet into Linux IP stack manually. Otherwise, the inject/discard action is determined automatically: For deny rule on all ports, the hit packet won't be forwarded into Linux IP stack neither */
    BOOL                    inject_into_ip_stack;              /**< Inject the hit packet into Linux IP stack (It's significant when inject_manual is TURE) */
    BOOL                    mac_swap;                           /**< Swap SMAC and DMAC */
} acl_action_t;

/** \brief ACL entry configuration */
typedef struct {
    mesa_ace_id_t           id;                                 /**< ACE ID */
    u8                      lookup;                             /**< Lookup, any non-zero value means second lookup */
    BOOL                    isdx_enable;                        /**< Use VID field for ISDX value */
    BOOL                    isdx_disable;                       /**< Match only frames with ISDX zero */
    mesa_port_list_t        port_list;                          /**< Port list */
    mesa_ace_u8_t           policy;                             /**< Policy number */
    mesa_ace_type_t         type;                               /**< ACE frame type */
    acl_action_t            action;                             /**< ACE action */
    vtss_isid_t             isid;                               /**< Switch ID, VTSS_ISID_GLOBAL means any */
    BOOL                    conflict;                           /**< Volatile ACE conflict flag */
    BOOL                    new_allocated;                      /**< This ACE entry is new allocated. (Only for internal used) */

    struct {
        uchar value[ACE_FLAG_SIZE];     /* ACE flag value */
        uchar mask[ACE_FLAG_SIZE];      /**< ACE flag mask */
    } flags;

    mesa_ace_vid_t  vid;                /**< VLAN ID (12 bit) */
    mesa_ace_u8_t   usr_prio;           /**< User priority (3 bit) */
    mesa_ace_bit_t  tagged;             /**< Tagged/untagged frame */

    /* Frame type specific data */
    union {
        /* MESA_ACE_TYPE_ANY: No specific fields */

        /**< MESA_ACE_TYPE_ETYPE */
        struct {
            mesa_ace_u48_t  dmac;   /**< DMAC */
            mesa_ace_u48_t  smac;   /**< SMAC */
            mesa_ace_u16_t  etype;  /**< Ethernet Type value */
            mesa_ace_u16_t  data;   /**< MAC data */
            mesa_ace_ptp_t  ptp;    /**< PTP header filtering (overrides smac byte 0-1 and data fields) */
        } etype;

        /**< MESA_ACE_TYPE_LLC */
        struct {
            mesa_ace_u48_t dmac; /**< DMAC */
            mesa_ace_u48_t smac; /**< SMAC */
            mesa_ace_u32_t llc;  /**< LLC */
        } llc;

        /**< MESA_ACE_TYPE_SNAP */
        struct {
            mesa_ace_u48_t dmac; /**< DMAC */
            mesa_ace_u48_t smac; /**< SMAC */
            mesa_ace_u40_t snap; /**< SNAP */
        } snap;

        /** MESA_ACE_TYPE_ARP */
        struct {
            mesa_ace_u48_t smac; /**< SMAC */
            mesa_ace_ip_t  sip;  /**< Sender IP address */
            mesa_ace_ip_t  dip;  /**< Target IP address */
        } arp;

        /**< MESA_ACE_TYPE_IPV4 */
        struct {
            mesa_ace_u8_t       ds;         /* DS field */
            mesa_ace_u8_t       proto;      /**< Protocol */
            mesa_ace_ip_t       sip;        /**< Source IP address */
            mesa_ace_ip_t       dip;        /**< Destination IP address */
            mesa_ace_u48_t      data;       /**< Not UDP/TCP: IP data */
            mesa_ace_udp_tcp_t  sport;      /**< UDP/TCP: Source port */
            mesa_ace_udp_tcp_t  dport;      /**< UDP/TCP: Destination port */
            mesa_ace_sip_smac_t sip_smac;   /**< SIP/SMAC matching (overrides sip field) */
            mesa_ace_ptp_t      ptp;        /**< PTP header filtering (overrides sip field) */
        } ipv4;

        /**< MESA_ACE_TYPE_IPV6 */
        struct {
            mesa_ace_u8_t      proto;     /**< IPv6 protocol */
            mesa_ace_u128_t    sip;       /**< IPv6 source address */
            mesa_ace_bit_t     ttl;       /**< TTL zero */
            mesa_ace_u8_t      ds;        /**< DS field */
            mesa_ace_u48_t     data;      /**< Not UDP/TCP: IP data */
            mesa_ace_udp_tcp_t sport;     /**< UDP/TCP: Source port */
            mesa_ace_udp_tcp_t dport;     /**< UDP/TCP: Destination port */
            mesa_ace_bit_t     tcp_fin;   /**< TCP FIN */
            mesa_ace_bit_t     tcp_syn;   /**< TCP SYN */
            mesa_ace_bit_t     tcp_rst;   /**< TCP RST */
            mesa_ace_bit_t     tcp_psh;   /**< TCP PSH */
            mesa_ace_bit_t     tcp_ack;   /**< TCP ACK */
            mesa_ace_bit_t     tcp_urg;   /**< TCP URG */
            mesa_ace_ptp_t     ptp;       /**< PTP header filtering (overrides sip byte 0-3) */
        } ipv6;
    } frame;
} acl_entry_conf_t;

/**
 * \brief ACL port configuration
 */
typedef struct {
    mesa_acl_policy_no_t    policy_no;          /**< Policy number */
    acl_action_t            action;             /**< Default action */
    BOOL                    sip_overloading;    /**< Source IP address overloading */
    BOOL                    smac_overloading;   /**< Source MAC address overloading */
} acl_port_conf_t;


/**
  * \brief Retrieve an error string based on a return code
  *        from one of the ACL API functions.
  *
  * \param rc [IN]: Error code that must be in the ACL_ERROR_xxx range.
  */
const char *acl_error_txt(mesa_rc rc);


/**
  * \brief Determine if ACL port configuration has changed.
  *
  * \param old [IN]: Pointer to structure that contains the
  *                  old configuration.
  * \param new [IN]: Pointer to structure that contains the
  *                  new configuration.
  *
  * \return
  *   0: No change.\n
  *   none zero: Configuration changed.\n
  */
int acl_mgmt_port_conf_changed(acl_port_conf_t *old, acl_port_conf_t *new_conf);


/**
  * \brief Get the ACL default port configuration.
  *
  * \param conf [IN_OUT]: Pointer to structure that contains the
  *                       configuration to get the default setting.
  *
  * \return
  *   Nothing.
  */
void acl_mgmt_port_conf_get_default(acl_port_conf_t *conf);

/**
  * \brief Get a switch's per-port configuration.
  *
  * \param port        [IN]: The port ID for which to retrieve the
  *                          configuration.
  * \param conf        [OUT]: Pointer to structure that receives
  *                          the switch's per-port configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    ACL_ERROR_PARM if Switch ID or port ID is invalid.\n
  */
mesa_rc acl_mgmt_port_conf_get(mesa_port_no_t port_no, acl_port_conf_t *conf);

/**
  * \brief Set a switch's per-port configuration.
  *
  * \param port_no    [IN]: The port ID for which to set the configuration.
  * \param conf       [IN]: Pointer to structure that contains
  *                         the switch's per-port configuration to be applied.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    ACL_ERROR_PARM if switch ID or port ID is invalid.\n
  *    ACL_ERROR_STACK_STATE if called on a secondary switch.\n
  */
mesa_rc acl_mgmt_port_conf_set(mesa_port_no_t port_no, acl_port_conf_t *conf);

/**
  * \brief Get a switch's port counter.
  *
  * \param port_no    [IN]: The port ID for which to get the counter.
  * \param counter    [OUT]: Pointer to structure that receives
  *                          the switch's port counter.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    ACL_ERROR_PARM if switch ID or port ID is invalid.\n
  */
mesa_rc acl_mgmt_port_counter_get(mesa_port_no_t port_no, mesa_acl_port_counter_t *const counter);


/**
  * \brief Determine if ACL policer configuration has changed.
  *
  * \param old [IN]: Pointer to structure that contains the
  *                  old configuration.
  * \param new [IN]: Pointer to structure that contains the
  *                  new configuration.
  *
  * \return
  *   0: No change.\n
  *   none zero: Configuration changed.\n
  */
int acl_mgmt_policer_conf_changed(const vtss_appl_acl_config_rate_limiter_t *const old, const vtss_appl_acl_config_rate_limiter_t *const new_conf);

/**
  * \brief Get the global ACL policer default configuration.
  *
  * \param conf [IN_OUT]: Pointer to structure that contains the
  *                       configuration to get the default setting.
  *
  * \return
  *   Nothing.
  */
void acl_mgmt_policer_conf_get_default(vtss_appl_acl_config_rate_limiter_t *conf);

/**
  * \brief Get a switch's port policer configuration.
  *
  * \param policer     [IN]: The policer ID for which to retrieve the
  *                          configuration.
  * \param conf        [OUT]: Pointer to structure that receives
  *                          the switch's port policer configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    ACL_ERROR_STACK_STATE if called on a secondary switch.\n
  */
mesa_rc acl_mgmt_policer_conf_get(mesa_acl_policer_no_t policer_no,
                                  vtss_appl_acl_config_rate_limiter_t *conf);

/**
  * \brief Set a switch's port policer configuration.
  *
  * \param policer_no [IN]: The policer ID for which to set the configuration.
  * \param conf       [IN]: Pointer to structure that contains
  *                         the switch's port policer configuration to be applied.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    ACL_ERROR_STACK_STATE if called on a secondary switch.\n
  */
mesa_rc acl_mgmt_policer_conf_set(mesa_acl_policer_no_t policer_no,
                                  const vtss_appl_acl_config_rate_limiter_t *const conf);

/* Get ACE or next ACE (use ACL_MGMT_ACE_ID_NONE to get first) */
/**
 * \brief Get/Getnext an ACE by user ID and ACE ID.
 *
 * \param user_id [IN]    The ACL user ID.
 *
 * \param isid    [IN]    The switch ID. This parameter must equal
 *                        VTSS_ISID_LOCAL when the role is secondary switch.
 *
 *                        isid = VTSS_ISID_LOCAL:
 *                        Can be called from both the primary and a secondary
 *                        switch.
 *                        It will return the entry that stored in the local
 *                        switch.
 *
 *                        isid = [VTSS_ISID_START - VTSS_ISID_END]:
 *                        isid = VTSS_ISID_GLOBAL:
 *                        Can only be called on the primary switch.
 *
 * \param id      [IN]    Indentify the ACE ID. Each ACL user can use the
 *                        independent ACE ID for its own entries.
 *
 *                        id = ACL_MGMT_ACE_ID_NONE:
 *                        It will return the first entry.
 *
 * \param conf    [OUT]   Pointer to structure that contains
 *                        the ACE configuration.
 *
 * \param counter [OUT]   The counter of the specific ACE. It can be equal
 *                        NULL if you don't need the information.
 *
 * \param next    [IN]    Indentify if it is getnext operation.
 *
 * \return
 *    VTSS_RC_OK on success.\n
 *    ACL_ERROR_STACK_STATE if the illegal primary/secondary switch state.\n
 *    ACL_ERROR_USER_NOT_FOUND if the user ID not found.\n
 *    ACL_ERROR_ACE_NOT_FOUND if the ACE ID not found.\n
 *    ACL_ERROR_REQ_TIMEOUT if the requirement timeout.\n
 **/
mesa_rc acl_mgmt_ace_get(acl_user_t user_id,
                         mesa_ace_id_t id, acl_entry_conf_t *conf,
                         mesa_ace_counter_t *counter, BOOL next);

/* Add ACE entry before given ACE or last (ACL_MGMT_ACE_ID_NONE) */
/**
 * \brief Add/Edit an ACE by user ID and ACE ID.
 *
 * Except for static configured ACEs, if the maximum number of entries
 * supported by the hardware is exceeded, the conflict flag of this entry
 * will be set and this entry will not be applied to the hardware.
 *
 * The order of the ACEs refers to the ACL user ID value. An ACE with
 * higher value will be placed the front of ACL.
 *
 * Only static configured ACEs will be stored in Flash.
 *
 * \param user_id [IN]    The ACL user ID.
 *
 * \param next_id [IN]    The next ACE ID that this ACE entry want
 *                        to insert before it. Each ACL user can use the
 *                        independent ACE ID for its own entries.
 *
 *                        next_id = ACL_MGMT_ACE_ID_NONE:
 *                        This entry will be placed at last in the given user ID.
 *
 * \param conf    [IN]    Pointer to structure that contains the ACE configuration.
 *
 *                        conf->isid = VTSS_ISID_LOCAL:
 *                        Can be called from both the primary and a secondary
 *                        switch.
 *                        It will add/edit the specific entry that stored
 *                        in the local switch. This parameter must equal
 *                        VTSS_ISID_LOCAL when the role is secondary switch.
 *
 *                        conf->isid = [VTSS_ISID_START - VTSS_ISID_END]:
 *                        Can only be called on the primary switch. This entry
 *                        will only apply to specific switch.
 *
 *                        conf->isid = VTSS_ISID_GLOBAL:
 *                        Can only be called on the primary switch. This entry
 *                        will apply to all switches.
 *
 *                        conf->id = ACL_MGMT_ACE_ID_NONE:
 *                        The ACE ID will be auto-assigned.
 *
 *                        conf->action.force_cpu = TRUE:
 *                        If the parameter is set, the specific packet will forward
 *                        to local switch.
 *                        The parameter of "conf->action.force_cpu" and
 *                        "conf->action.cpu_once" is insignificance when
 *                        user_id = ACL_USER_STATIC.
 *
 * \param conf    [OUT]   The parameter of "conf->id" will be update if using
 *                        auto-assigned.
 *                        The parameter of "conf->conflict" will be set if
 *                        it is not applied to the hardware due to hardware
 *                        limitations.
 *
 * \return
 *    VTSS_RC_OK on success.\n
 *    ACL_ERROR_STACK_STATE if illegal primary/secondary switch state.\n
 *    ACL_ERROR_USER_NOT_FOUND if the user ID not found.\n
 *    ACL_ERROR_MEM_ALLOC_FAIL if memory allocated fail.\n
 *    ACL_ERROR_PARM if input parameters error.\n
 *    ACL_ERROR_REQ_TIMEOUT if the requirement timeout.\n
 **/
mesa_rc acl_mgmt_ace_add(acl_user_t user_id, mesa_ace_id_t next_id, acl_entry_conf_t *conf);

/**
 * \brief Delete an ACE by user ID and ACE ID.
 *
 * \param user_id [IN]    The ACL user ID.
 *
 * \param id      [IN]    The ACE ID that we want to delete it.
 *                        Each ACL user can use the independent
 *                        ACE ID for its own entries.
 *
 * \return
 *    VTSS_RC_OK on success.\n
 *    ACL_ERROR_STACK_STATE if illegal primary/secondary switch state.\n
 *    ACL_ERROR_USER_NOT_FOUND if the user ID not found.\n
 *    ACL_ERROR_ACE_NOT_FOUND if the ACE ID not found.\n
 **/
mesa_rc acl_mgmt_ace_del(acl_user_t user_id, mesa_ace_id_t id);

/**
  * \brief Clear all ACE counter
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    ACL_ERROR_STACK_STATE if called on a secondary switch.\n
  */
mesa_rc acl_mgmt_counters_clear(void);

/**
  * \brief Get ACL user name
  *
  * \return
  *    not NULL on success.\n
  *    NULL on failed.\n
  */
const char *acl_mgmt_user_name_get(
    acl_user_t user_id
);

/**
  * \brief Get ACL user ID by name
  *
  * \param user_name [IN]: ACL user name.
  *
  * \return
  *    acl_user_t - ACL user ID.
  */
acl_user_t acl_mgmt_user_id_get_by_name(
    char *user_name);

/**
  * \brief Initialize the ACL module
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc acl_init(vtss_init_data_t *data);

/**
 * \brief Initialize ACE to default values.(permit on all front ports)
 *
 * \param type [IN]  ACE type.
 *
 * \param ace [OUT]  ACE structure.
 *
 * \return
 *    VTSS_RC_OK on success.\n
 *    ACL_ERROR_UNKNOWN_ACE_TYPE if the ACE type is unknown.\n
 **/
mesa_rc acl_mgmt_ace_init(mesa_ace_type_t type, acl_entry_conf_t *ace);

/* Get/Getnext ACE interface mux rule */
/**
 * \brief Get/Getnext ACE interface mux rule.
 *
 * \param ace_id   [IN/OUT] ACE ID. The value will be changed to the next avail ACE ID for the GETNEXT operation.
 *
 * \param ifmux_id [OUT]    The interface MUX ID.
 *
 * \param next     [IN]     Indentify if it is getnext operation. Use ACE ID 0 for GETFIRST operation.
 *
 * \return
 *    VTSS_RC_OK on success.\n
 *    VTSS_APPL_ACL_ERROR_ACE_NOT_FOUND if the ACE ID not found.\n
 **/
mesa_rc acl_mgmt_ifmux_get(mesa_ace_id_t *ace_id, int *ifmux_id, BOOL next);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_ACL_API_H_ */

