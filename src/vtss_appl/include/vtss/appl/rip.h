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
 * \file
 * \brief Public RIP API
 * \details This header file describes RIP control functions and types.
 */

#ifndef _VTSS_APPL_RIP_H_
#define _VTSS_APPL_RIP_H_

#include <microchip/ethernet/switch/api/types.h>  // For type declarations
#include <vtss/appl/module_id.h>             // For MODULE_ERROR_START()
#include <vtss/appl/interface.h>             // For vtss_ifindex_t
#include <vtss/appl/router.h>
#include <vtss/basics/map.hxx>
#include <vtss/basics/vector.hxx>

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------
//** RIP type declaration
//----------------------------------------------------------------------------
/** \brief The data type of RIP timer. */
typedef uint32_t vtss_appl_rip_timer_t;

/** \brief The data type of RIP metric value. */
typedef uint8_t vtss_appl_rip_metric_t;

/** \brief The data type of RIP distance value. */
typedef uint8_t vtss_appl_rip_distance_t;

//----------------------------------------------------------------------------
//** RIP module error codes
//----------------------------------------------------------------------------
/** \brief RIP error return codes (mesa_rc) */
enum {
    /** Generic error code */
    VTSS_APPL_FRR_RIP_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_FRR_RIP),

    /** The address is not unicast */
    VTSS_APPL_FRR_RIP_ERROR_NOT_UNICAST_ADDRESS,

    /** The password/key is invalid */
    VTSS_APPL_FRR_RIP_ERROR_AUTH_KEY_INVALID,

    /** The RIP router mode is disabled */
    VTSS_APPL_FRR_RIP_ERROR_ROUTER_DISABLED,

    /** The address is invalid for neighbor connection */
    VTSS_APPL_FRR_RIP_ERROR_INVALID_ADDRESS_FOR_NEIGHBOR_CONNECTION,

    /** The key chain and simple password configuration can not be set */
    VTSS_APPL_FRR_RIP_ERROR_KEY_CHAIN_PWD_COEXISTS
};

//----------------------------------------------------------------------------
//** RIP variables valid ranges
//----------------------------------------------------------------------------
/**< The valid range of RIP redistributed default metric. (1-16) */
constexpr vtss_appl_rip_metric_t VTSS_APPL_RIP_REDIST_DEF_METRIC_MIN = 1;  /**< Minimum value of RIP redistributed default metric. */
constexpr vtss_appl_rip_metric_t VTSS_APPL_RIP_REDIST_DEF_METRIC_MAX = 16; /**< Maximum value of RIP redistributed default metric. */

/**< The valid range of RIP redistributed specific metric. (1-16) */
constexpr vtss_appl_rip_metric_t VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MIN = 1;  /**< Minimum value of RIP redistributed specific metric. */
constexpr vtss_appl_rip_metric_t VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MAX = 16; /**< Maximum value of RIP redistributed specific metric. */

/**< The valid range of RIP timers. (5-2147483) */
constexpr vtss_appl_rip_timer_t VTSS_APPL_RIP_TIMER_MIN = 5;        /**< Minimum value of RIP timers. */
constexpr vtss_appl_rip_timer_t VTSS_APPL_RIP_TIMER_MAX = 2147483;  /**< Maximum value of RIP timers.*/

/**< The valid range of administrative distance. (1-255) */
constexpr vtss_appl_rip_distance_t VTSS_APPL_RIP_ADMIN_DISTANCE_MIN = 1;    /**< Minimum value of RIP administrative distance. */
constexpr vtss_appl_rip_distance_t VTSS_APPL_RIP_ADMIN_DISTANCE_MAX = 255;  /**< Maximum value of RIP administrative distance. */

/**< The valid range of simple password. (1-15) */
constexpr uint32_t VTSS_APPL_RIP_AUTH_SIMPLE_KEY_MIN_LEN = 1;       /**<Minimum length of simple password */
constexpr uint32_t VTSS_APPL_RIP_AUTH_SIMPLE_KEY_MAX_LEN = 15;      /**<Maximum length of simple password */

/**< The valid range of access-list name length. (1-31) */
constexpr uint32_t VTSS_APPL_RIP_ACCESS_LIST_NAME_MAX_LEN = 31;     /**<Maximum length of access-list name */

/** The formula of converting the plain text length to encrypted data length. */
#define VTSS_APPL_RIP_AUTH_ENCRYPTED_KEY_LEN(x) \
    ((16 + ((x + 15) / 16) * 16 + 32) * 2)

/** Maximum length of encrypted simple password.  */
#define VTSS_APPL_RIP_AUTH_ENCRYPTED_SIMPLE_KEY_LEN \
    VTSS_APPL_RIP_AUTH_ENCRYPTED_KEY_LEN(VTSS_APPL_RIP_AUTH_SIMPLE_KEY_MAX_LEN)

/** Maximum count of the RIP offset-list. */
constexpr uint32_t VTSS_APPL_RIP_OFFSET_LIST_MAX_COUNT = VTSS_APPL_ROUTER_ACCESS_LIST_MAX_COUNT;

/**< The valid range of RIP offset-list metric. (0-16) */
constexpr vtss_appl_rip_metric_t VTSS_APPL_RIP_OFFSET_LIST_METRIC_MIN = 0;  /**< Minimum value of RIP offset-list metric. */
constexpr vtss_appl_rip_metric_t VTSS_APPL_RIP_OFFSET_LIST_METRIC_MAX = 16; /**< Maximum value of RIP offset-list metric. */

/** Maximum count of the RIP neighbors. */
constexpr uint32_t VTSS_APPL_RIP_NEIGHBOR_MAX_COUNT = 128;

//------------------------------------------------------------------------------
//** RIP module capabilities
//------------------------------------------------------------------------------
/**
 * \brief RIP capabilities
 */
typedef struct {
    /** Minimum RIP redistributed default metric */
    vtss_appl_rip_metric_t redist_def_metric_min = VTSS_APPL_RIP_REDIST_DEF_METRIC_MIN;

    /** Maximum RIP redistributed default metric */
    vtss_appl_rip_metric_t redist_def_metric_max = VTSS_APPL_RIP_REDIST_DEF_METRIC_MAX;

    /** Minimum value of RIP redistributed specific metric. */
    vtss_appl_rip_metric_t redist_specific_metric_min = VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MIN;

    /** Maximum value of RIP redistributed specific metric. */
    vtss_appl_rip_metric_t redist_specific_metric_max = VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MAX;

    /** Minimum RIP interval of time (in seconds) */
    vtss_appl_rip_timer_t timer_min = VTSS_APPL_RIP_TIMER_MIN;

    /** Maximum RIP interval of time (in seconds) */
    vtss_appl_rip_timer_t timer_max = VTSS_APPL_RIP_TIMER_MAX;

    /** Minimum value of RIP administrative distance. */
    vtss_appl_rip_distance_t admin_distance_min = VTSS_APPL_RIP_ADMIN_DISTANCE_MIN;

    /** Maximum value of RIP administrative distance. */
    vtss_appl_rip_distance_t admin_distance_max = VTSS_APPL_RIP_ADMIN_DISTANCE_MAX;

    /** Maximum value of access-list name length. */
    uint32_t access_list_name_len_max = VTSS_APPL_RIP_ACCESS_LIST_NAME_MAX_LEN;

    /** Indicate if OSPF redistributed is supported or not */
    mesa_bool_t ospf_redistributed_supported;

    /** Maximum value of simple password string length. */
    uint32_t simple_pwd_string_len_max = VTSS_APPL_RIP_AUTH_SIMPLE_KEY_MAX_LEN;

    /** Maximum value of key chain name length. */
    uint32_t key_chain_name_len_max = VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MAX_LEN;

    /** Maximum count of the RIP offset-list. */
    uint32_t offset_list_max_count = VTSS_APPL_RIP_OFFSET_LIST_MAX_COUNT;

    /** Minimum value of RIP offset-list metric. */
    vtss_appl_rip_metric_t offset_list_metric_min = VTSS_APPL_RIP_OFFSET_LIST_METRIC_MIN;

    /** Maximum value of RIP offset-list metric. */
    vtss_appl_rip_metric_t offset_list_metric_max = VTSS_APPL_RIP_OFFSET_LIST_METRIC_MAX;

    /** Maximum count of the RIP network segments. */
    uint32_t network_segment_max_count;

    /** Maximum count of the RIP neighbors. */
    uint32_t neighbor_max_count = VTSS_APPL_RIP_NEIGHBOR_MAX_COUNT;
} vtss_appl_rip_capabilities_t;

/**
 * \brief Get RIP capabilities. (valid ranges and support features)
 * \param cap [OUT] RIP capabilities
 * \return Error code.
 */
mesa_rc vtss_appl_rip_capabilities_get(vtss_appl_rip_capabilities_t *const cap);

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------
/** \brief The global RIP version support. */
typedef enum {
    /** The router sends RIPv2 and accepts both RIPv1 and RIPv2.
      * When the router receives either version of REQUESTS or triggered
      * updates packets, it replies with the appropriate version. */
    VTSS_APPL_RIP_GLOBAL_VER_DEFAULT,

    /** Receive/Send RIPv1 only. */
    VTSS_APPL_RIP_GLOBAL_VER_1,

    /** Receive/Send RIPv2 only. */
    VTSS_APPL_RIP_GLOBAL_VER_2,

    /** The maximum of the RIP version. */
    VTSS_APPL_RIP_GLOBAL_VER_COUNT
} vtss_appl_rip_global_ver_t;

/** \brief The RIP version for the advertisement reception on the interface. */
typedef enum {
    /** Deny any RIP version reception. */
    VTSS_APPL_RIP_INTF_RECV_VER_NONE,

    /** Receive RIPv1 only. */
    VTSS_APPL_RIP_INTF_RECV_VER_1,

    /** Receive RIPv2 only. */
    VTSS_APPL_RIP_INTF_RECV_VER_2,

    /** Receive both RIPv1 and RIPv2. */
    VTSS_APPL_RIP_INTF_RECV_VER_BOTH,

    /** Not specified, refer to global version setting. */
    VTSS_APPL_RIP_INTF_RECV_VER_NOT_SPECIFIED,

    /** The maximum of the RIP receive version. */
    VTSS_APPL_RIP_INTF_RECV_VER_COUNT
} vtss_appl_rip_intf_recv_ver_t;

/** \brief The RIP version for the advertisement transmission on the interface. */
typedef enum {
    /** Send RIPv1 only. */
    VTSS_APPL_RIP_INTF_SEND_VER_1 = VTSS_APPL_RIP_INTF_RECV_VER_1,

    /** Send RIPv2 only. */
    VTSS_APPL_RIP_INTF_SEND_VER_2,

    /** Send both RIPv1 and RIPv2. */
    VTSS_APPL_RIP_INTF_SEND_VER_BOTH,

    /** Not specified, refer to global version setting. */
    VTSS_APPL_RIP_INTF_SEND_VER_NOT_SPECIFIED,

    /** The maximum of the RIP send version. */
    VTSS_APPL_RIP_INTF_SEND_VER_COUNT
} vtss_appl_rip_intf_send_ver_t;

/** \brief The data structure for the RIP timers. */
typedef struct {
    /** The update timer (in seconds). */
    vtss_appl_rip_timer_t update_timer;

    /** The invalid timer (in seconds). */
    vtss_appl_rip_timer_t invalid_timer;

    /** The garbage-collection timer (in seconds). */
    vtss_appl_rip_timer_t garbage_collection_timer;
} vtss_appl_rip_timers_basic_t;

/** \brief The RIP redistributed protocol type. */
typedef enum {
    /** The RIP redistributed protocol type for the connected interfaces. */
    VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED,

    /** The RIP redistributed protocol type for the static routes. */
    VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC,

    /** The RIP redistributed protocol type for the OSPF routes. */
    VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF,

    /** The maximum of the RIP route redistributed protocol type. */
    VTSS_APPL_RIP_REDIST_PROTOCOL_COUNT
} vtss_appl_rip_redist_proto_type_t;

/** \brief The data structure for the RIP redistribution configuration. */
typedef struct {
    /** Indicate the RIP redistribution of the specific protocol type
      * is enabled or not. */
    mesa_bool_t is_enabled;

    /** Indicate the 'metric' argument is a specific configured value
      * or not.*/
    mesa_bool_t is_specific_metric;

    /** User specified metric value for the external routes.
      * The field is significant only when argument 'is_specific_metric' is
      * TRUE. */
    vtss_appl_rip_metric_t metric;
} vtss_appl_rip_redist_conf_t;

/** \brief The data structure for the RIP router configuration.
 *  To disable the RIP router mode, only one essential parameter 'is_enabled'
 *  is required. The reset of parameters are significant only when the router
 *  mode is enabled.
 */
typedef struct {
    /** Configure RIP router mode. */
    mesa_bool_t router_mode;

    /** Configure RIP version support. */
    vtss_appl_rip_global_ver_t version;

    /** Configure RIP timers. */
    vtss_appl_rip_timers_basic_t timers;

    /** Configure RIP redistributed default metric.
      * It is used when the metric value isn't specificed for the
      * redistributed protocol type. */
    vtss_appl_rip_metric_t redist_def_metric;

    /** Configure RIP route redistribution. */
    vtss_appl_rip_redist_conf_t redist_conf[VTSS_APPL_RIP_REDIST_PROTOCOL_COUNT];

    /** Configure RIP default route redistribution. */
    mesa_bool_t def_route_redist;

    /** Configure all interfaces as passive-interface by default. */
    mesa_bool_t def_passive_intf;

    /** Configure RIP administrative distance. */
    vtss_appl_rip_distance_t admin_distance;
} vtss_appl_rip_router_conf_t;

/**
 * \brief Get the RIP router default configuration.
 * \param conf [OUT] RIP router configuration.
 * \return Error code.
 */
mesa_rc frr_rip_router_conf_def(vtss_appl_rip_router_conf_t *const conf);

/**
 * \brief Get the RIP router configuration.
 * \param conf [OUT] RIP router configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_conf_get(vtss_appl_rip_router_conf_t *const conf);

/**
 * \brief Set the RIP router configuration.
 * \param conf [IN] RIP router configuration.
 *                  only "router_mode" parameter is needed when rip router mode
 *                  disabled
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_conf_set(const vtss_appl_rip_router_conf_t *const conf);

//----------------------------------------------------------------------------
//** RIP router interface configuration
//----------------------------------------------------------------------------
/** \brief The data structure for the RIP router interface configuration. */
typedef struct {
    /** Enable the interface as RIP passive-interface. */
    mesa_bool_t passive_enabled;
} vtss_appl_rip_router_intf_conf_t;

/**
 * \brief Get the RIP router interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] RIP router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_intf_conf_get(
    const vtss_ifindex_t                ifindex,
    vtss_appl_rip_router_intf_conf_t    *const conf);

/**
 * \brief Set the RIP router interface configuration.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] RIP router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_intf_conf_set(
    const vtss_ifindex_t                    ifindex,
    const vtss_appl_rip_router_intf_conf_t  *const conf);

/**
 * \brief Iterate through all RIP router interfaces.
 * \param current_ifindex [IN]  The current ifIndex
 * \param next_ifindex    [OUT] The next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_intf_conf_itr(
    const vtss_ifindex_t        *const current_ifindex,
    vtss_ifindex_t              *const next_ifindex);

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
/**
 * \brief Get the RIP network configuration.
 * \param network [OUT]  RIP network.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_network_conf_get(
    const mesa_ipv4_network_t *const network);

/**
 * \brief Add/set the RIP network configuration.
 * \param network [IN] RIP network.
 * \return Error code.
 * FRR_RC_ADDR_RANGE_OVERLAP means that the network range is overlapped
 */
mesa_rc vtss_appl_rip_network_conf_add(const mesa_ipv4_network_t *const network);
/**
 * \brief Delete the RIP network configuration.
 * \param network [IN] RIP network.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_network_conf_del(const mesa_ipv4_network_t *const network);

/**
 * \brief Iterate the RIP network
 * \param cur_network  [IN]  Current RIP network
 * \param next_network [OUT] Next RIP network
 * \return Error code.
 */
mesa_rc vtss_appl_rip_network_conf_itr(
    const mesa_ipv4_network_t   *const cur_network,
    mesa_ipv4_network_t         *const next_network);


//----------------------------------------------------------------------------
//** RIP neighbor connection configuration
//----------------------------------------------------------------------------
/**
 * \brief Get the RIP neighbor connection configuration.
 * \param neighbor_addr [OUT] The RIP neighbor address.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_neighbor_conf_get(const mesa_ipv4_t neighbor_addr);

/**
 * \brief Add the RIP neighbor connection configuration.
 * \param neighbor_addr [IN] The RIP neighbor address.
 * \return Error code.
 * VTSS_APPL_FRR_RIP_ERROR_INVALID_ADDRESS_FOR_NEIGHBOR_CONNECTION means that
 * the IP address is not unicast, broadcast, or network IP address , which is
 * invalid for neighbor connection.
 */
mesa_rc vtss_appl_rip_neighbor_conf_add(const mesa_ipv4_t neighbor_addr);

/**
 * \brief Delete the RIP neighbor connection configuration.
 * \param neighbor_addr [IN] The RIP neighbor address.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_neighbor_conf_del(const mesa_ipv4_t neighbor_addr);

/**
 * \brief Iterate the RIP neighbor connection configuration
 * \param cur_nb_addr  [IN]  The current RIP neighbor address.
 * \param next_nb_addr [OUT] The next RIP neighbor address.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_neighbor_conf_itr(
    const mesa_ipv4_t   *const cur_nb_addr,
    mesa_ipv4_t         *const next_nb_addr);

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
/** \brief The authentication type. */
typedef enum {
    /** NULL authentication. */
    VTSS_APPL_RIP_AUTH_TYPE_NULL,

    /** Simple password authentication. */
    VTSS_APPL_RIP_AUTH_TYPE_SIMPLE_PASSWORD,

    /** MD5 digest authentication. */
    VTSS_APPL_RIP_AUTH_TYPE_MD5,

    /** The maximum of the authentication type which is used for validation. */
    VTSS_APPL_RIP_AUTH_TYPE_COUNT
} vtss_appl_rip_auth_type_t;

//----------------------------------------------------------------------------
//** RIP interface configuration
//----------------------------------------------------------------------------
/** \brief The RIP split horizon mode. */
typedef enum {
    /** The simple split horizon mode . */
    VTSS_APPL_RIP_SPLIT_HORIZON_MODE_SIMPLE,

    /** The split horizon with poisoned reverse mode. */
    VTSS_APPL_RIP_SPLIT_HORIZON_MODE_POISONED_REVERSE,

    /** The normal mode. */
    VTSS_APPL_RIP_SPLIT_HORIZON_MODE_DISABLED,

    /** The maximum of the RIP split horizon. */
    VTSS_APPL_RIP_SPLIT_HORIZON_MODE_COUNT
} vtss_appl_rip_split_horizon_mode_t;

/** \brief The data structure for the RIP VLAN interface configuration. */
typedef struct {
    /** The RIP version for the advertisement transmission on the interface. */
    vtss_appl_rip_intf_send_ver_t send_ver;

    /** The RIP version for the advertisement reception on the interface. */
    vtss_appl_rip_intf_recv_ver_t recv_ver;

    /** The RIP split horizon mode. */
    vtss_appl_rip_split_horizon_mode_t split_horizon_mode;

    /** The RIP authentication type.*/
    vtss_appl_rip_auth_type_t auth_type;

    /** The key chain name for the MD5 authentication. */
    char md5_key_chain_name[VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MAX_LEN + 1];

    /** Set 'true' to indicate the simple password is encrypted,
       otherwise it's unencrypted. */
    mesa_bool_t is_encrypted;
    /** The simple password. */
    char     simple_pwd[VTSS_APPL_RIP_AUTH_ENCRYPTED_SIMPLE_KEY_LEN + 1];
} vtss_appl_rip_intf_conf_t;

/**
 * \brief Get the RIP VLAN interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] The RIP VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_intf_conf_get(
    const vtss_ifindex_t        ifindex,
    vtss_appl_rip_intf_conf_t   *const conf);

/**
 * \brief Set the RIP VLAN interface configuration.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] The RIP VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_intf_conf_set(
    const vtss_ifindex_t                ifindex,
    const vtss_appl_rip_intf_conf_t     *const conf);

/**
 * \brief Iterate through all RIP VLAN interfaces.
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_rip_intf_conf_itr(
    const vtss_ifindex_t    *const current_ifindex,
    vtss_ifindex_t          *const next_ifindex);

/**
 * \brief Get/Get-Next RIP interface configration by IP address
 *  This API is designed for standard MIB access. The keys of MIB interface
 *  table are IP addr and ifindex.  Notice that the valid configrations are
 *  excluded none IP VLAN interfaces.
 *
 *  For SNMP 'Get-First' operation, set curr_addr = NULL.
 *  For SNMP 'Get' operation, set getnext = false,
 *  FOr SNMP 'Get-Next' operation, set 'getnext' parameter to TRUE and input a
 *  specific IP adddress
 *
 * \param is_getnext_oper [IN]   get-next operation
 * \param current_addr    [IN]   Current interface address.
 * \param next_addr       [OUT]  Next interface address. The field is
 *                               significant only when for the get-next
 *                               operation.
 * \param conf            [OUT]  Interface configuration
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_rip_intf_conf_snmp_get(
    const mesa_bool_t is_getnext_oper,
    const mesa_ipv4_t *const current_addr, mesa_ipv4_t *const next_addr,
    vtss_appl_rip_intf_conf_t *const conf);

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
/** \brief The direction to add the offset to routing metric update. */
typedef enum {
    /** Perform offset on incoming routing metric updates. */
    VTSS_APPL_RIP_OFFSET_DIRECTION_IN,

    /** Perform offset on outgoing routing metric updates. */
    VTSS_APPL_RIP_OFFSET_DIRECTION_OUT,

    /** The maximum count of the offset directions. */
    VTSS_APPL_RIP_OFFSET_DIRECTION_COUNT
} vtss_appl_rip_offset_direction_t;

/** \brief The data structure for the RIP offset-list. */
typedef struct {
    /** The interface index of the offset-list entry */
    vtss_ifindex_t ifindex;

    /** The direction to add the offset to routing metric update. */
    vtss_appl_rip_offset_direction_t direction;
} vtss_appl_rip_offset_entry_key_t;

/** \brief The data structure for the RIP offset-list. */
typedef struct {
    /** The name of the access-list */
    vtss_appl_router_access_list_name_t name;

    /** The offset to incoming or outgoing routing metric. */
    vtss_appl_rip_metric_t offset_metric;
} vtss_appl_rip_offset_entry_data_t;

/**
 * \brief Add the RIP offset-list configuration.
 * \param ifindex   [IN]  The interface index of the offset-list entry
 * \param direction [IN]  The direction of the offset-list entry
 * \param entry     [IN]  The entry data of the offset-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_add(
    const vtss_ifindex_t                    ifindex,
    const vtss_appl_rip_offset_direction_t  direction,
    const vtss_appl_rip_offset_entry_data_t *const entry);

/**
 * \brief Set the RIP offset-list configuration.
 * \param ifindex   [IN]  The interface index of the offset-list entry
 * \param direction [IN]  The direction of the offset-list entry
 * \param entry     [IN]  The entry data of the offset-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_set(
    const vtss_ifindex_t                    ifindex,
    const vtss_appl_rip_offset_direction_t  direction,
    const vtss_appl_rip_offset_entry_data_t *const entry);

/**
 * \brief Get the RIP offset-list configuration.
 * \param ifindex   [IN]  The interface index of the offset-list entry
 * \param direction [IN]  The direction of the offset-list entry
 * \param entry     [OUT] The entry data of the offset-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_get(
    const vtss_ifindex_t                    ifindex,
    const vtss_appl_rip_offset_direction_t  direction,
    vtss_appl_rip_offset_entry_data_t       *const entry);

/**
 * \brief Get all entries of the RIP offset-list.
 * \param conf [OUT] An container with all offset-list entries.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_get_all(
    vtss::Vector<vtss::Pair<vtss_appl_rip_offset_entry_key_t,
    vtss_appl_rip_offset_entry_data_t>> &conf);

/**
 * \brief Delete the RIP offset-list configuration.
 * \param ifindex   [IN]  The interface index of the offset-list entry
 * \param direction [IN]  The direction of the offset-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_del(
    const vtss_ifindex_t                    ifindex,
    const vtss_appl_rip_offset_direction_t  direction);

/**
 * \brief Iterate through RIP offset-list.
 * \param current_ifindex   [IN]  Current interface index
 * \param next_ifindex      [OUT] Next interface index
 * \param current_direction [IN]  Current direction
 * \param next_direction    [OUT] Next direction
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_itr(
    const vtss_ifindex_t                    *const current_ifindex,
    vtss_ifindex_t                          *const next_ifindex,
    const vtss_appl_rip_offset_direction_t  *const current_direction,
    vtss_appl_rip_offset_direction_t        *const next_direction);

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
/** \brief The data structure for the RIP general status. */
typedef struct {
    /** RIP router mode */
    mesa_bool_t is_enabled;

    /** RIP timers. */
    vtss_appl_rip_timers_basic_t timers;

    /** Specifies when the next round of updates will be sent out from this router in seconds. */
    uint32_t next_update_time;

    /** The default metric value of redistributed routes. */
    vtss_appl_rip_metric_t default_metric;

    /** Global RIP Version. */
    vtss_appl_rip_global_ver_t version;

    /** RIP protocol types redistribution. */
    mesa_bool_t redist_proto_type[VTSS_APPL_RIP_REDIST_PROTOCOL_COUNT];

    /** Administrative distance value. */
    vtss_appl_rip_distance_t admin_distance;

    /** RFC 1724 - RIP Version 2 MIB Extension. */
    /** The number of route changes made to the IP Route Database by RIP.
      * This does not include the refresh of a route's age. */
    uint32_t global_route_changes;

    /** The number of responses sent to RIP queries from other systems. */
    uint32_t global_queries;
} vtss_appl_rip_general_status_t;

/**
 * \brief Get the RIP general status.
 * \param status [OUT] Status for RIP.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_general_status_get(
    vtss_appl_rip_general_status_t  *const status);

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
/** \brief the RIP interface status. */
typedef struct {
    /** The RIP version for the advertisement transmission on the interface. */
    vtss_appl_rip_intf_send_ver_t send_version;

    /** The RIP version for the advertisement reception on the interface. */
    vtss_appl_rip_intf_recv_ver_t recv_version;

    /** This indicates the interface enable triggered update or not. */
    mesa_bool_t triggered_update;

    /** This indicates if the passive-interface is active on the interface or not. */
    mesa_bool_t is_passive_intf;

    /** This indicates the interface is associate with a specific key-chain name. */
    char key_chain[VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MAX_LEN + 1];

    /** This indicates the interface authentication type. */
    vtss_appl_rip_auth_type_t auth_type;

    /** RFC 1724 - RIP Version 2 MIB Extension. */
    /** The number of RIP response packets received by the RIP process which
      * were subsequently discarded for any reason. */
    uint32_t recv_badpackets;

    /** The number of routes, in valid RIP packets, which were ignored for any
      * reason (e.g. unknown address family, or invalid metric). */
    uint32_t recv_badroutes;

    /** The number of triggered RIP updates actually sent on this interface.
      * This explicitly does NOT include full updates sent containing new
      * information. */
    uint32_t sent_updates;
} vtss_appl_rip_interface_status_t;

/**
 * \brief Iterator through the interface in the RIP
 *
 * \param prev      [IN]    Ifindex to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *                          Currently CLI and Web use this iterator.
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_rip_intf_status_ifindex_itr(
    const vtss_ifindex_t        *const prev,
    vtss_ifindex_t              *const next
);

/**
 * \brief Get status for a specific interface.
 * \param ifindex   [IN] Ifindex to query.
 * \param status    [OUT] Status for 'key'.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_intf_status_get(
    const vtss_ifindex_t                   ifindex,
    vtss_appl_rip_interface_status_t      *const status);

/**
 * \brief Get status for all interfaces.
 * \param intf [IN,OUT] An empty container which gets populated
 * \return Error code.
 */
mesa_rc vtss_appl_rip_interface_status_get_all(
    vtss::Vector<vtss::Pair<vtss_ifindex_t, vtss_appl_rip_interface_status_t>> &intf);

/**
 * \brief Iterator through the RIP active interfaces by IP address
 *  This API is designed for standard MIB access. The keys of MIB interface table are IP addr
 * and ifindex.
 *
 * For SNMP 'Get-First' operation, set curr_addr = NULL.
 * For SNMP 'Get' operation, set getnext = false,
 * FOr SNMP 'Get-Next' operation, set 'getnext' parameter to TRUE and input a specific IP adddress
 *
 * \param is_getnext_oper [IN]   get-next operation
 * \param current_addr    [IN]   Current interface address.
 * \param next_addr       [OUT]  Next interface address. The field is significant only when for the get-next operation.
 * \param status          [OUT]  active interface status
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_rip_intf_status_snmp_get(
    const mesa_bool_t                     is_getnext_oper,
    const mesa_ipv4_t                     *const current_addr,
    mesa_ipv4_t                           *const next_addr,
    vtss_appl_rip_interface_status_t      *const status);

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
/** \brief The data structure for the the RIP neighbor entry data. */
typedef struct {
    /** The RIP version number in the header of the last RIP packet received
      * from the neighbor. The INT type follows the syntax of rip2PeerVersion
      * in RIP2-MIB. The possible value is 1 for RIPv1 or 2 for RIPv2. */
    int                            rip_version;

    /** The time duration in seconds from the time the last RIP
      * packet received from the neighbor to now. */
    uint32_t                            last_update_time;

    /** The number of RIP response packets from the neighbor discarded
      * as invalid. */
    uint32_t                            recv_bad_packets;

    /** The number of routes from the neighbor that were ignored because
      * the route entry was invalid. */
    uint32_t                            recv_bad_routes;
} vtss_appl_rip_peer_data_t;

/**
 * \brief Iterate the entry key through the RIP neighbor table.
 * \param in  [IN]  Pointer to current key. Provide a null pointer to get the
 *                  first entry key.
 * \param out [OUT] Next available entry key.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_peer_itr(
    const mesa_ipv4_t    *const in,
    mesa_ipv4_t          *const out);

/**
 * \brief Get specific neighbor entry from the RIP peer table.
 * \param key  [IN]  The entry key.
 * \param data [OUT] The entry data.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_peer_get(
    const mesa_ipv4_t    key,
    vtss_appl_rip_peer_data_t         *const data);


/**
 * \brief Get all entries of the RIP peer table.
 * \param database [IN,OUT]  An empty container which gets populated
 * \return Error code.
 */
mesa_rc vtss_appl_rip_peer_get_all(
    vtss::Vector<vtss::Pair<mesa_ipv4_t,
    vtss_appl_rip_peer_data_t>> &database);


//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------

/** \brief The protocol type of the RIP database entry. */
typedef enum {
    /** The route is learned through RIP. */
    VTSS_APPL_RIP_DB_PROTO_TYPE_RIP,

    /** The destination address is connected directly. */
    VTSS_APPL_RIP_DB_PROTO_TYPE_CONNECTED,

    /** The route is configured by the user for other protocol and redistributed
      * into the RIP domain. */
    VTSS_APPL_RIP_DB_PROTO_TYPE_STATIC,

    /** The route is learned through OSPF. */
    VTSS_APPL_RIP_DB_PROTO_TYPE_OSPF,

    /** The maximum of the protocol type which is used for validation. */
    VTSS_APPL_RIP_DB_PROTO_TYPE_COUNT
} vtss_appl_rip_db_proto_type_t;

/** \brief The protocol type of the RIP database entry. */
typedef enum {
    /** The route is configured by the user for RIP only. */
    VTSS_APPL_RIP_DB_PROTO_SUBTYPE_STATIC,

    /** The route is received from the RIP neighbors. */
    VTSS_APPL_RIP_DB_PROTO_SUBTYPE_NORMAL,

    /** The route is a default route. */
    VTSS_APPL_RIP_DB_PROTO_SUBTYPE_DEFAULT,

    /** The route is redistributed from other protocols. */
    VTSS_APPL_RIP_DB_PROTO_SUBTYPE_REDIST,

    /** The route is generated for the local interfaces. */
    VTSS_APPL_RIP_DB_PROTO_SUBTYPE_INTF,

    /** The maximum of the protocolsub type which is used for validation. */
    VTSS_APPL_RIP_DB_PROTO_SUBTYPE_COUNT
} vtss_appl_rip_db_proto_subtype_t;

/** \brief The data structure for the RIP database entry key. */
typedef struct {
    /** The destination IP address and mask of the route. */
    mesa_ipv4_network_t network;

    /** The first gateway along the route to the destination. */
    mesa_ipv4_t         nexthop;
} vtss_appl_rip_db_key_t;

/** \brief The data structure for the the RIP database entry data. */
typedef struct {
    /** This indicates the protocol type of the route. */
    vtss_appl_rip_db_proto_type_t       type;

    /** This indicates the protocol sub-type of the route. */
    vtss_appl_rip_db_proto_subtype_t    subtype;

    /** The metric of the route. */
    vtss_appl_rip_metric_t              metric;

    /** The field is significant only when the route is redistributed from other
      * protocol type, for example, OSPF. This indicates the metric value from
      * the original redistributed source. */
    uint32_t                            external_metric;

    /** This indicates the route is generated from one of the local interfaces
      * or not.
      */
    mesa_bool_t                         self_intf;

    /** The learning source IP address of the route. This indicates the route is
      * learned an IP address. The field is significant only when the route isn't
      * generated from the local interfaces.
      */
    mesa_ipv4_t                         src_addr;

    /** The tag of the route. It is used to provide a method of separating
      * "internal" RIP routes, which may have been imported from an EGP (Exterior
      * ateway protocol) or another IGP (Interior gateway protocol). For example,
      * routes imported from OSPF can have a route tag value which the other
      * routing protocols can use to prevent advertising the same route back to
      * the original protocol routing domain. */
    uint32_t                            tag;

    /** The time field is significant only when the route is learned from
      * the neighbors.
      * When the route destination is reachable (its metric value less
      * than 16), the time field means the invalid time of the route.
      * When the route destination is unreachable (its metric value great
      * than 16), the time field means the garbage-collection time of the route.
    */
    uint32_t                            uptime;
} vtss_appl_rip_db_data_t;

/**
 * \brief Iterate the route entry key through the RIP database.
 * \param in  [IN]  Pointer to current key. Provide a null pointer to get the
 *                  first entry key.
 * \param out [OUT] Next available entry key.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_db_itr(
    const vtss_appl_rip_db_key_t    *const in,
    vtss_appl_rip_db_key_t          *const out);

/**
 * \brief Get specific route entry from the RIP database.
 * \param key  [IN]  The entry key.
 * \param data [OUT] The entry data.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_db_get(
    const vtss_appl_rip_db_key_t    *const key,
    vtss_appl_rip_db_data_t         *const data);


/**
 * \brief Get all entries of the RIP database.
 * \param database [IN,OUT]  An empty container which gets populated
 * \return Error code.
 */
mesa_rc vtss_appl_rip_db_get_all(
    vtss::Vector<vtss::Pair<vtss_appl_rip_db_key_t, vtss_appl_rip_db_data_t>> &database);

//----------------------------------------------------------------------------
//** RIP control global options
//----------------------------------------------------------------------------
/**
 * \brief RIP control global options.
 */
typedef struct {
    /** Reload RIP process */
    mesa_bool_t reload_process;
} vtss_appl_rip_control_globals_t;

/**
 * \brief Set RIP control of global options.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_control_globals(
    const vtss_appl_rip_control_globals_t *const control);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_RIP_H_ */
