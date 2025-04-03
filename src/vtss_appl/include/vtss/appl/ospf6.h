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
 * \brief Public OSPFv3 for IPv6 API
 * \details This header file describes OSPFv3 control functions and types.
 */

#ifndef _VTSS_APPL_OSPF6_H_
#define _VTSS_APPL_OSPF6_H_

#include <microchip/ethernet/switch/api/types.h> // For type declarations
#include <vtss/appl/module_id.h>                 // For MODULE_ERROR_START()
#include <vtss/appl/interface.h>                 // For vtss_ifindex_t
#include <vtss/basics/map.hxx>
#include <vtss/basics/vector.hxx>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//** OSPF6 type declaration
//----------------------------------------------------------------------------

/** \brief The data type of OSPF6 instance ID. */
typedef uint32_t vtss_appl_ospf6_id_t;

/** \brief The data type of OSPF6 area ID. */
typedef mesa_ipv4_t vtss_appl_ospf6_area_id_t;

/** \brief The data type of OSPF6 router ID. */
typedef mesa_ipv4_t vtss_appl_ospf6_router_id_t;

/** \brief The data type of OSPF6 priority value. */
typedef uint32_t vtss_appl_ospf6_priority_t;

/** \brief The data type of OSPF6 cost value. */
typedef uint32_t vtss_appl_ospf6_cost_t;

/** \brief The data type of OSPF6 metric value. */
typedef uint32_t vtss_appl_ospf6_metric_t;

/** \brief The message digest key ID. */
typedef uint8_t vtss_appl_ospf6_md_key_id_t;

//----------------------------------------------------------------------------
//** OSPF6 module error codes
//----------------------------------------------------------------------------

/** \brief FRR error return codes (mesa_rc) */
enum {
    /** Invalid router ID */
    VTSS_APPL_FRR_OSPF6_ERROR_INVALID_ROUTER_ID = MODULE_ERROR_START(VTSS_MODULE_ID_FRR_OSPF6),

    /** The OSPF6 router ID change doesn't take effect */
    VTSS_APPL_FRR_OSPF6_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT,

    /** The OSPF6 area ID change doesn't take effect */
    VTSS_APPL_FRR_OSPF6_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT,

    /** Backbone can not be configured as stub area */
    VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE,

    /** Area range not-advertise and cost can not be set at the same time */
    VTSS_APPL_FRR_OSPF6_ERROR_AREA_RANGE_COST_CONFLICT,

    /** Area range network address can't be default */
    VTSS_APPL_FRR_OSPF6_ERROR_AREA_RANGE_NETWORK_DEFAULT,

    /** Cannot enable OSPF6 due to deffered shutdown time in progress */
    VTSS_APPL_FRR_OSPF6_ERROR_DEFFERED_SHUTDOWN_IN_PROGRESS
};

//----------------------------------------------------------------------------
//** OSPF6 variables valid range
//----------------------------------------------------------------------------
/**< The valid range of OSPF6 instance ID. (multiple instance is unsupported yet) */
constexpr uint32_t VTSS_APPL_OSPF6_INSTANCE_ID_START = 1;                        /**< Valid starting ID of OSPF6 instance. */
constexpr uint32_t VTSS_APPL_OSPF6_INSTANCE_ID_MAX   = 1;                        /**< Maximum ID of OSPF6 instance. */

/**< The valid range of OSPF6 router ID. (1-4294967294 or 0.0.0.1-255.255.255.254) */
constexpr vtss_appl_ospf6_router_id_t VTSS_APPL_OSPF6_ROUTER_ID_MIN = 1;          /**< Minimum value of OSPF6 router ID. */
constexpr vtss_appl_ospf6_router_id_t VTSS_APPL_OSPF6_ROUTER_ID_MAX = 4294967294; /**< Maximum value of OSPF6 router ID. */

/**< The valid range of OSPF6 priority. (0-255) */
constexpr vtss_appl_ospf6_priority_t VTSS_APPL_OSPF6_PRIORITY_MIN = 0;            /**< Minimum value of OSPF6 priority. */
constexpr vtss_appl_ospf6_priority_t VTSS_APPL_OSPF6_PRIORITY_MAX = 255;          /**< Maximum value of OSPF6 priority. */

/**< The valid range of OSPF6 general cost. (0-16777215) */
constexpr vtss_appl_ospf6_cost_t VTSS_APPL_OSPF6_GENERAL_COST_MIN = 0;            /**< Minimum value of OSPF6 general cost. */
constexpr vtss_appl_ospf6_cost_t VTSS_APPL_OSPF6_GENERAL_COST_MAX = 16777215;     /**< Maximum value of OSPF6 general cost. */

/**< The valid range of OSPF6 interface cost. (1-65535) */
constexpr vtss_appl_ospf6_cost_t VTSS_APPL_OSPF6_INTF_COST_MIN = 1;               /**< Minimum value of OSPF6 interface cost. */
constexpr vtss_appl_ospf6_cost_t VTSS_APPL_OSPF6_INTF_COST_MAX = 65535;           /**< Maximum value of OSPF6 interface cost. */

/**< The valid range of OSPF6 hello interval. (1-65535) */
constexpr uint32_t VTSS_APPL_OSPF6_HELLO_INTERVAL_MIN = 1;                       /**< Minimum value of OSPF6 hello interval. */
constexpr uint32_t VTSS_APPL_OSPF6_HELLO_INTERVAL_MAX = 65535;                   /**< Maximum value of OSPF6 hello interval. */

/**< The valid range of OSPF6 retransmit interval. (3-65535) */
constexpr uint32_t VTSS_APPL_OSPF6_RETRANSMIT_INTERVAL_MIN = 3;                  /**< Minimum value of OSPF6 retransmit interval. */
constexpr uint32_t VTSS_APPL_OSPF6_RETRANSMIT_INTERVAL_MAX = 65535;              /**< Maximum value of OSPF6 retransmit interval. */

/**< The valid range of OSPF6 transmit delay. (1-3600) */
constexpr uint32_t VTSS_APPL_OSPF6_TRANSMIT_DELAY_MIN = 1;                  /**< Minimum value of OSPF6 retransmit interval. */
constexpr uint32_t VTSS_APPL_OSPF6_TRANSMIT_DELAY_MAX = 3600;              /**< Maximum value of OSPF6 retransmit interval. */

/**< The valid range of OSPF6 dead interval. (1-65535) */
constexpr uint32_t VTSS_APPL_OSPF6_DEAD_INTERVAL_MIN = 1;                        /**< Minimum value of OSPF6 dead interval. */
constexpr uint32_t VTSS_APPL_OSPF6_DEAD_INTERVAL_MAX = 65535;                    /**< Maximum value of OSPF6 dead interval. */

/**< The valid range of administrative distance. (1-255) */
constexpr uint8_t VTSS_APPL_OSPF6_ADMIN_DISTANCE_MIN = 1;      /**< Minimum value of OSPF6 administrative distance. */
constexpr uint8_t VTSS_APPL_OSPF6_ADMIN_DISTANCE_MAX = 255;    /**< Maximum value of OSPF6 administrative distance. */

//----------------------------------------------------------------------------
//** OSPF6 capabilities
//----------------------------------------------------------------------------

/**
 * \brief OSPF6 capabilities
 */
typedef struct {
    /** Minimum OSPF6 instance ID */
    vtss_appl_ospf6_id_t instance_id_min;

    /** Maximum OSPF6 instance ID */
    vtss_appl_ospf6_id_t instance_id_max;

    /** Minimum OSPF6 router ID */
    vtss_appl_ospf6_router_id_t router_id_min;

    /** Maximum OSPF6 router ID */
    vtss_appl_ospf6_router_id_t router_id_max;

    /** Minimum OSPF6 priority value */
    vtss_appl_ospf6_priority_t priority_min;

    /** Maximum OSPF6 priority value */
    vtss_appl_ospf6_priority_t priority_max;

    /** Minimum OSPF6 general cost value */
    vtss_appl_ospf6_cost_t general_cost_min;

    /** Maximum OSPF6 general cost value */
    vtss_appl_ospf6_cost_t general_cost_max;

    /** Minimum OSPF6 interface cost value */
    vtss_appl_ospf6_cost_t intf_cost_min;

    /** Maximum OSPF6 interface cost value */
    vtss_appl_ospf6_cost_t intf_cost_max;

    /** Minimum OSPF6 hello interval */
    uint32_t hello_interval_min;

    /** Maximum OSPF6 hello interval */
    uint32_t hello_interval_max;

    /** Minimum OSPF6 retransmit interval */
    uint32_t retransmit_interval_min;

    /** Maximum OSPF6 retransmit interval */
    uint32_t retransmit_interval_max;

    /** Minimum OSPF6 dead interval */
    uint32_t dead_interval_min;

    /** Maximum OSPF6 dead interval */
    uint32_t dead_interval_max;

    /** Indicate if RIPNG redistributed is supported or not */
    mesa_bool_t ripng_redistributed_supported;

    /** Minimum value of OSPF6 administrative distance. */
    uint8_t admin_distance_min = VTSS_APPL_OSPF6_ADMIN_DISTANCE_MIN;

    /** Maximum value of RIP administrative distance. */
    uint8_t admin_distance_max = VTSS_APPL_OSPF6_ADMIN_DISTANCE_MAX;
} vtss_appl_ospf6_capabilities_t;

/**
 * \brief Get OSPF6 capabilities to see what supported or not
 * \param cap [OUT] OSPF6 capabilities
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_capabilities_get(vtss_appl_ospf6_capabilities_t *const cap);

//----------------------------------------------------------------------------
//** OSPF6 instance configuration
//----------------------------------------------------------------------------

/**
 * \brief Add the OSPF6 instance.
 * \param id [IN] OSPF6 instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_add(const vtss_appl_ospf6_id_t id);

/**
 * \brief Delete the OSPF6 instance.
 * \param id [IN] OSPF6 instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_del(const vtss_appl_ospf6_id_t id);

/**
 * \brief Get the OSPF6 instance which the OSPF6 routing process is enabled.
 * \param id [IN] OSPF6 instance ID.
 * \return Error code.  VTSS_RC_OK means that OSPF6 routing process is enabled
 *                      on the instance ID.
 *                      VTSS_RC_ERROR means that the instance ID is not created
 *                      and OSPF6 routing process is disabled.
 */
mesa_rc vtss_appl_ospf6_get(const vtss_appl_ospf6_id_t id);

/**
 * \brief Iterate through all OSPF6 instances.
 * \param current_id [IN]   Pointer to the current instance ID. Use null pointer
 *                          to get the first instance ID.
 * \param next_id    [OUT]  Pointer to the next instance ID
 * \return Error code.      VTSS_RC_OK means that the next instance ID is valid
 *                          and the value is saved in 'out' parameter.
 *                          VTSS_RC_ERROR means that the next instance ID is
 *                          non-existing.
 */
mesa_rc vtss_appl_ospf6_inst_itr(
    const vtss_appl_ospf6_id_t   *const current_id,
    vtss_appl_ospf6_id_t         *const next_id);

/**
 * \brief OSPF6 control global options.
 */
typedef struct {
    /** Reload OSPF6 process */
    mesa_bool_t reload_process;
} vtss_appl_ospf6_control_globals_t;

/**
 * \brief Set OSPF6 control of global options.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_control_globals(
    const vtss_appl_ospf6_control_globals_t *const control);

//----------------------------------------------------------------------------
//** OSPF6 router configuration/status
//----------------------------------------------------------------------------
/** \brief The data structure for the OSPF6 router ID. */
typedef struct {
    /** Indicate the 'ospf6_router_id' argument is a specific configured value
     * or not. */
    mesa_bool_t is_specific_id;

    /** The OSPF6 router ID. The value is used only when 'is_specific_id'
     * argument is true. */
    vtss_appl_ospf6_router_id_t id;
} vtss_appl_ospf6_router_id_conf_t;

/** \brief The data structure for the OSPF6 area ID. */
typedef struct {
    /** Indicate the 'ospf6_area_id' argument is a specific configured value
     * or not. */
    mesa_bool_t is_specific_id;

    /** The OSPF6 interface area ID. The value is used only when 'is_specific_id'
     * argument is true. */
    vtss_appl_ospf6_area_id_t id;
} vtss_appl_ospf6_area_id_conf_t;

/** \brief The OSPF6 redistributed protocol type. */
enum {
    /** The OSPF6 redistributed protocol type for the connected interfaces. */
    VTSS_APPL_OSPF6_REDIST_PROTOCOL_CONNECTED,

    /** The OSPF6 redistributed protocol type for the static routes. */
    VTSS_APPL_OSPF6_REDIST_PROTOCOL_STATIC,

    /** The maximum of the OSPF6 route redistributed protocol type. */
    VTSS_APPL_OSPF6_REDIST_PROTOCOL_COUNT
};

/** \brief The data structure for the OSPF router ID. */
typedef struct {
    /** Indicate this redistribution protocol enabled or not. */
    mesa_bool_t is_redist_enable;
} vtss_appl_ospf6_redist_conf_t;

/** \brief The data structure for the OSPF6 router configuration. */
typedef struct {
    /** Configure the OSPF6 router ID. */
    vtss_appl_ospf6_router_id_conf_t router_id;

    /** Configure OSPF route redistribution. */
    vtss_appl_ospf6_redist_conf_t redist_conf[VTSS_APPL_OSPF6_REDIST_PROTOCOL_COUNT];

    /** Administrative distance value. */
    uint8_t admin_distance;
} vtss_appl_ospf6_router_conf_t;

/** \brief The data structure for the OSPF6 router interface configuration. */
typedef struct {
    /** Area Id */
    vtss_appl_ospf6_area_id_conf_t area_id;
} vtss_appl_ospf6_router_intf_conf_t;

/** \brief The data structure for the OSPF6 router status. */
typedef struct {
    /** The OSPF6 router ID. */
    vtss_appl_ospf6_router_id_t  ospf6_router_id;

    /** Delay time (in seconds)of SPF calculations. */
    uint32_t    spf_delay;

    /** Minimum hold time (in milliseconds) between consecutive SPF calculations. */
    uint32_t    spf_holdtime;

    /** Maximum wait time (in milliseconds) between consecutive SPF calculations. */
    uint32_t    spf_max_waittime;

    /** Time (in milliseconds) that has passed between the start of the SPF
      algorithm execution and the current time. */
    uint64_t    last_executed_spf_ts;

    /** Maximum arrival time (in milliseconds) of link-state advertisements. */
    uint32_t    min_lsa_arrival;

    /** Number of areas attached to the router. */
    uint32_t    attached_area_count;

} vtss_appl_ospf6_router_status_t;

/**
 * \brief Get the OSPF6 router configuration.
 * \param id   [IN] OSPF6 instance ID.
 * \param conf [OUT] OSPF6 router configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_conf_get(
    const vtss_appl_ospf6_id_t       id,
    vtss_appl_ospf6_router_conf_t    *const conf);

/**
 * \brief Set the OSPF6 router configuration.
 * \param id   [IN] OSPF6 instance ID.
 * \param conf [IN] OSPF6 router configuration. Set the metric type to
 *                  VTSS_APPL_OSPF6_REDIST_METRIC_TYPE_NONE will actually
 *                  delete the redistribution.
 * \return Error code.
 * VTSS_APPL_FRR_OSPF6_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT means that router
 * ID change doesn't take effect immediately. The new setting will be applied
 * after OSPF6 process restarting.
 */
mesa_rc vtss_appl_ospf6_router_conf_set(
    const vtss_appl_ospf6_id_t           id,
    const vtss_appl_ospf6_router_conf_t  *const conf);

/**
 * \brief Get the OSPF6 router interface configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF6 router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_intf_conf_get(
    const vtss_appl_ospf6_id_t           id,
    const vtss_ifindex_t                ifindex,
    vtss_appl_ospf6_router_intf_conf_t   *const conf);

/**
 * \brief Set the OSPF6 router interface configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] OSPF6 router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_intf_conf_set(
    const vtss_appl_ospf6_id_t               id,
    const vtss_ifindex_t                    ifindex,
    const vtss_appl_ospf6_router_intf_conf_t *const conf);

/**
 * \brief Iterate through all OSPF6 router interfaces.
 * \param current_id      [IN]  Current OSPF6 ID
 * \param next_id         [OUT] Next OSPF6 ID
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_intf_conf_itr(
    const vtss_appl_ospf6_id_t   *const current_id,
    vtss_appl_ospf6_id_t         *const next_id,
    const vtss_ifindex_t        *const current_ifindex,
    vtss_ifindex_t              *const next_ifindex);

/**
 * \brief Get the OSPF6 router configuration.
 * \param id     [IN] OSPF6 instance ID.
 * \param status [OUT] Status for 'id'.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_status_get(
    const vtss_appl_ospf6_id_t       id,
    vtss_appl_ospf6_router_status_t  *const status);

//----------------------------------------------------------------------------
//** OSPF6 network area configuration/status
//----------------------------------------------------------------------------
/** \brief The  type of the OSPF6 area. */
typedef enum {
    /** Normal area. */
    VTSS_APPL_OSPF6_AREA_NORMAL,

    /** Stub area. */
    VTSS_APPL_OSPF6_AREA_STUB,

    /** Totally stub area. */
    VTSS_APPL_OSPF6_AREA_TOTALLY_STUB,

    /** The maximum of the area type. */
    VTSS_APPL_OSPF6_AREA_COUNT
} vtss_appl_ospf6_area_type_t;

/** \brief The data structure for the OSPF6 area status. */
typedef struct {
    /** To indicate if it's backbone area or not. */
    mesa_bool_t is_backbone;

    /** To indicate the area type. */
    vtss_appl_ospf6_area_type_t area_type;

    /** Total Number of interfaces attached in the area. */
    uint32_t attached_intf_total_count;

    /** Number of times SPF algorithm has been executed for the particular area. */
    uint32_t spf_executed_count;

    /** Number of the total LSAs for the particular area. */
    uint32_t lsa_count;

} vtss_appl_ospf6_area_status_t;

/**
 * \brief Iterate through the OSPF6 area status.
 * \param cur_id       [IN]  Current OSPF6 ID
 * \param next_id      [OUT] Next OSPF6 ID
 * \param cur_area_id  [IN]  Current area ID
 * \param next_area_id [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_status_itr(
    const vtss_appl_ospf6_id_t       *const cur_id,
    vtss_appl_ospf6_id_t             *const next_id,
    const vtss_appl_ospf6_area_id_t  *const cur_area_id,
    vtss_appl_ospf6_area_id_t        *const next_area_id);

/**
 * \brief Get the OSPF6 area status.
 * \param id        [IN] OSPF6 instance ID.
 * \param area      [IN] OSPF6 area key.
 * \param status    [OUT] OSPF6 area val.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_status_get(
    const vtss_appl_ospf6_id_t         id,
    const mesa_ipv4_t                 area,
    vtss_appl_ospf6_area_status_t      *const status);

//----------------------------------------------------------------------------
//** OSPF6 area range
//----------------------------------------------------------------------------
/** \brief The data structure for the OSPF6 area range configuration. */
typedef struct {
    /** When the value is 'true', the ABR can summarize intra area paths from
     *  the address range in one inter-area-prefix-LSA(Type-3) advertised to other areas.
     *  When the value is 'false', the ABR does not advertised the inter-area-prefix-LSA
     *  (Type-3) for the address range. */
    mesa_bool_t is_advertised;

    /** Indicate the 'cost' argument is a specific configured value
      * or not. */
    mesa_bool_t is_specific_cost;

    /** User specified cost (or metric) for this summary route. */
    vtss_appl_ospf6_cost_t cost;
} vtss_appl_ospf6_area_range_conf_t;

/**
 * \brief Get the OSPF6 area range configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] OSPF6 area ID.
 * \param network [IN] OSPF6 area range network.
 * \param conf    [IN] OSPF6 area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_get(
    const vtss_appl_ospf6_id_t           id,
    const vtss_appl_ospf6_area_id_t      area_id,
    const mesa_ipv6_network_t           network,
    vtss_appl_ospf6_area_range_conf_t    *const conf);

/**
 * \brief Set the OSPF6 area range configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] OSPF6 area ID.
 * \param network [IN] OSPF6 area range network.
 * \param conf    [IN] OSPF6 area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_set(
    const vtss_appl_ospf6_id_t               id,
    const vtss_appl_ospf6_area_id_t          area_id,
    const mesa_ipv6_network_t               network,
    const vtss_appl_ospf6_area_range_conf_t  *const conf);

/**
 * \brief Add the OSPF6 area range configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] OSPF6 area ID.
 * \param network [IN] OSPF6 area range network.
 * \param conf    [IN] OSPF6 area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_add(
    const vtss_appl_ospf6_id_t               id,
    const vtss_appl_ospf6_area_id_t          area_id,
    const mesa_ipv6_network_t               network,
    const vtss_appl_ospf6_area_range_conf_t  *const conf);

/**
 * \brief Delete the OSPF6 area range configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] OSPF6 area ID.
 * \param network [IN] OSPF6 area range network.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_del(
    const vtss_appl_ospf6_id_t       id,
    const vtss_appl_ospf6_area_id_t  area_id,
    const mesa_ipv6_network_t       network);

/**
 * \brief Iterate the OSPF6 area ranges
 * \param current_id      [IN]  Current OSPF6 ID
 * \param next_id         [OUT] Next OSPF6 ID
 * \param current_area_id [IN]  Current area ID
 * \param next_area_id    [OUT] Next area ID
 * \param current_network [IN]  Current network address
 * \param next_network    [OUT] Next network address
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_itr(
    const vtss_appl_ospf6_id_t       *const current_id,
    vtss_appl_ospf6_id_t             *const next_id,
    const vtss_appl_ospf6_area_id_t  *const current_area_id,
    vtss_appl_ospf6_area_id_t        *const next_area_id,
    const mesa_ipv6_network_t       *const current_network,
    mesa_ipv6_network_t             *const next_network);

//----------------------------------------------------------------------------
//** OSPF6 stub area
//----------------------------------------------------------------------------

/** \brief The data structure of stub area configuration. */
typedef struct {
    /** Set 'true' to configure the inter-area routes do not inject into this
     *  stub area.
     */
    mesa_bool_t no_summary;
} vtss_appl_ospf6_stub_area_conf_t;

/**
 * \brief Add the specific area in the stub areas.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \param conf    [IN] The stub area configuration for adding.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_add(
    const vtss_appl_ospf6_id_t               id,
    const vtss_appl_ospf6_area_id_t          area_id,
    const vtss_appl_ospf6_stub_area_conf_t   *const conf);

/**
 * \brief Set the configuration for a specific stub area.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \param conf    [IN] The stub area configuration for setting.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_set(
    const vtss_appl_ospf6_id_t               id,
    const vtss_appl_ospf6_area_id_t          area_id,
    const vtss_appl_ospf6_stub_area_conf_t   *const conf);

/**
 * \brief Get the configuration for a specific stub area.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN]  The area ID of the stub area configuration.
 * \param conf    [OUT] The stub area configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_get(
    const vtss_appl_ospf6_id_t               id,
    const vtss_appl_ospf6_area_id_t          area_id,
    vtss_appl_ospf6_stub_area_conf_t         *const conf);

/**
 * \brief Delete a specific stub area.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_del(
    const vtss_appl_ospf6_id_t               id,
    const vtss_appl_ospf6_area_id_t          area_id);

/**
 * \brief Iterate the stub areas.
 * \param current_id      [IN]  Current OSPF6 ID
 * \param next_id         [OUT] Next OSPF6 ID
 * \param current_area_id [IN]  Current area ID
 * \param next_area_id    [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_itr(
    const vtss_appl_ospf6_id_t       *const current_id,
    vtss_appl_ospf6_id_t             *const next_id,
    const vtss_appl_ospf6_area_id_t  *const current_area_id,
    vtss_appl_ospf6_area_id_t        *const next_area_id);

//----------------------------------------------------------------------------
//** OSPF6 interface parameter tuning
//----------------------------------------------------------------------------

/** \brief The data structure for the OSPF6 VLAN interface configuration. */
typedef struct {
    /** User specified router priority for the interface. */
    vtss_appl_ospf6_priority_t priority;

    /** Link state metric for the interface. It used for Shortest Path First
     * (SPF) calculation. */
    /** Indicate the 'cost' argument is a specific configured value or not. */
    mesa_bool_t is_specific_cost;
    /** User specified cost for the interface. */
    vtss_appl_ospf6_cost_t cost;

    /**
     * When cleared (default), the "Interface MTU" of received OSPF Database
     * Description (DBD) packets will be checked against our own MTU, and if
     * they differ, the DBD will be ignored.
     *
     * When set, the "Interface MTU" of received OSPF DBD packets will be
     * ignored.
     */
    mesa_bool_t mtu_ignore;

    /** The time interval (in seconds) between hello packets.
     *  The value is set to 1 when the paramter 'is_fast_hello_enabled' is
     *  true */
    uint32_t dead_interval;

    /** The time interval (in seconds) between hello packets. */
    uint32_t hello_interval;

    /** The time interval (in seconds) between between link-state advertisement
     *  (LSA) retransmissions for adjacencies. */
    uint32_t retransmit_interval;

    /** Indicates whether the interface is passive or not */
    mesa_bool_t is_passive;

    /** The transmit-delay is the number of seconds required to transmit
      * a packet. */
    uint32_t transmit_delay;

} vtss_appl_ospf6_intf_conf_t;

/**
 * \brief Get the OSPF6 VLAN interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF6 VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_intf_conf_get(
    const vtss_ifindex_t        ifindex,
    vtss_appl_ospf6_intf_conf_t  *const conf);

/**
 * \brief Set the OSPF6 VLAN interface configuration.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] OSPF6 VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_intf_conf_set(
    const vtss_ifindex_t                ifindex,
    const vtss_appl_ospf6_intf_conf_t    *const conf);

/**
 * \brief Iterate through all OSPF6 VLAN interfaces.
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_intf_conf_itr(
    const vtss_ifindex_t    *const current_ifindex,
    vtss_ifindex_t          *const next_ifindex);

//----------------------------------------------------------------------------
//** OSPF6 interface status
//----------------------------------------------------------------------------

/** \brief The link state of the OSPF6 interface. */
typedef enum {
    /** Down state */
    VTSS_APPL_OSPF6_INTERFACE_DOWN = 1,

    /** lookback interface */
    VTSS_APPL_OSPF6_INTERFACE_LOOPBACK = 2,

    /** Waiting state */
    VTSS_APPL_OSPF6_INTERFACE_WAITING = 3,

    /** Point-To-Point interface */
    VTSS_APPL_OSPF6_INTERFACE_POINT2POINT = 4,

    /** Select as DR other router */
    VTSS_APPL_OSPF6_INTERFACE_DR_OTHER = 5,

    /** Select as BDR router */
    VTSS_APPL_OSPF6_INTERFACE_BDR = 6,

    /** Select as DR router */
    VTSS_APPL_OSPF6_INTERFACE_DR = 7,

    /** Unknown state */
    VTSS_APPL_OSPF6_INTERFACE_UNKNOWN

} vtss_appl_ospf6_interface_state_t;

/** \brief The data structure for the OSPF6 interface status.
 *  Notice that the timer parameters:
 *  hello_time, dead_time and retransmit_time will equal the
 *  current configured timer interval setting when the interface state is down.
 */
typedef struct {
    /** It's used to indicate if the interface is up or down */
    mesa_bool_t                         status;

    /** Indicate if the interface is passive interface */
    mesa_bool_t                         is_passive;

    /** IP address and prefix length */
    mesa_ipv6_network_t                 network;

    /** Area ID */
    vtss_appl_ospf6_area_id_t            area_id;

    /** The OSPF6 router ID. */
    vtss_appl_ospf6_router_id_t          router_id;

    /** The cost of the interface. */
    vtss_appl_ospf6_cost_t               cost;

    /** define the state of the link */
    vtss_appl_ospf6_interface_state_t    state;

    /** The OSPF6 priority */
    uint8_t                             priority;

    /** The router ID of DR. */
    mesa_ipv4_t                         dr_id;

    /** The router ID of BDR. */
    mesa_ipv4_t                         bdr_id;

    /** Hello timer, the unit of time is the second. */
    uint32_t                            hello_time;

    /** Dead timer, the unit of time is the second. */
    uint32_t                            dead_time;

    /** Retransmit timer, the unit of time is the second. */
    uint32_t                            retransmit_time;

    /** Transmit Delay */
    uint32_t                            transmit_delay;
} vtss_appl_ospf6_interface_status_t;

/**
 * \brief Iterator through the interface in the ospf
 *
 * \param prev      [IN]    Ifindex to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *                          Currently CLI and Web use this iterator.
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_ospf6_interface_itr(
    const vtss_ifindex_t        *const prev,
    vtss_ifindex_t              *const next
);

/**
 * \brief Iterator2 through the interface in the ospf
 *
 * When IN is NULL, assign the first index.
 * When IN is not NULL, assign the next index according to the given IN value.
 * Currently MIB uses this iterator.
 *
 * \param current_addr    [IN]   Current interface address.
 * \param current_ifidx   [IN]   Current interface ifindex.
 * \param next_addr       [OUT]  Next interface address.
 * \param next_ifidx      [OUT]  Next interface ifindex.
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_ospf6_interface_itr2(
    const mesa_ipv6_t *const current_addr,
    mesa_ipv6_t *const next_addr,
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx);

/**
 * \brief Get status for a specific interface.
 * \param ifindex   [IN] Ifindex to query.
 * \param status    [OUT] Status for 'key'.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_interface_status_get(
    const vtss_ifindex_t                   ifindex,
    vtss_appl_ospf6_interface_status_t      *const status);

/**
 * \brief Get status for all route.
 * \param interface [IN,OUT] An empty container which gets populated
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_interface_status_get_all(
    vtss::Map<vtss_ifindex_t,
    vtss_appl_ospf6_interface_status_t> &interface);

//----------------------------------------------------------------------------
//** OSPF6 neighbor status
//----------------------------------------------------------------------------
/**
 * \brief The OSPF6 options field is present in OSPF6 Hello packets, which
 *        enables OSPF6 routers to support (or not support) optional capabilities,
 *        and to communicate their capability level to other OSPF6 routers.
 *
 *        RFC5340 provides a description of each capability. See
 *        https://tools.ietf.org/html/rfc5340#appendix-A.2 for a detail
 *        information.
 */
enum {
    VTSS_APPL_OSPF6_OPTION_FIELD_V6       = (1 << 0), /**< Control V6-bit  */
    VTSS_APPL_OSPF6_OPTION_FIELD_E        = (1 << 1), /**< Control E-bit  */
    VTSS_APPL_OSPF6_OPTION_FIELD_MC        = (1 << 2), /**< Control x-bit */
    VTSS_APPL_OSPF6_OPTION_FIELD_N        = (1 << 3), /**< Control N-bit */
    VTSS_APPL_OSPF6_OPTION_FIELD_R        = (1 << 4), /**< Control R-bit */
    VTSS_APPL_OSPF6_OPTION_FIELD_DC       = (1 << 5), /**< Control DC-bit */
};

/** \brief The neighbor state of the OSPF. */
typedef enum {
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_DEPENDUPON = 0,   /**< Depend Upon state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_DELETED = 1,      /**< Deleted state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_DOWN = 2,         /**< Down state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_ATTEMPT = 3,      /**< Attempt state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_INIT = 4,         /**< Init state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_2WAY = 5,         /**< 2-Way state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_EXSTART = 6,      /**< ExStart state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_EXCHANGE = 7,     /**< Exchange state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_LOADING = 8,      /**< Loading state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_FULL = 9,         /**< Full state */
    VTSS_APPL_OSPF6_NEIGHBOR_STATE_UNKNOWN           /**< Unknown state */
} vtss_appl_ospf6_neighbor_state_t;

/** \brief The data structure for the OSPF6 neighbor information. */
typedef struct {
    /** The IP address. */
    mesa_ipv6_t                         ip_addr;
    /** Neighbor's router ID. */
    vtss_appl_ospf6_router_id_t          neighbor_id;
    /** Area ID */
    vtss_appl_ospf6_area_id_t            area_id;
    /** Interface index */
    vtss_ifindex_t                      ifindex;
    /** The OSPF6 neighbor priority */
    uint8_t                             priority;
    /** Neighbor state */
    vtss_appl_ospf6_neighbor_state_t     state;
    /** The router ID of DR. */
    mesa_ipv4_t                         dr_id;
    /** The router ID of BDR. */
    mesa_ipv4_t                         bdr_id;
    /** Dead timer. */
    uint32_t                            dead_time;
    /** Transit Area ID */
    vtss_appl_ospf6_area_id_t            transit_id;
} vtss_appl_ospf6_neighbor_status_t;

/** \brief The data structure combining the OSPF6 neighbor's keys and status for
  * get_all container. */
typedef struct {
    /** OSPF6 instance ID. */
    vtss_appl_ospf6_id_t              id;
    /** Neighbor id to query. */
    vtss_appl_ospf6_router_id_t       neighbor_id;
    /** Neighbor IP to query. */
    mesa_ipv6_t                      neighbor_ip;
    /** Neighbor ifindex to query. */
    vtss_ifindex_t                   neighbor_ifidx;
    /** The OSPF6 neighbor information. */
    vtss_appl_ospf6_neighbor_status_t status;
} vtss_appl_ospf6_neighbor_data_t;

/**
 * \brief Get neighbor ID by neighbor IP address.
 * \param ip_addr   [IN] Neighbor IP address to query.
 * \return the Neighbor router ID or zero address if not found.
 */
vtss_appl_ospf6_router_id_t vtss_appl_ospf6_nbr_lookup_id_by_addr(const mesa_ipv6_t ip_addr);

/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by standard MIB
 * \param current_id      [IN]  Current OSPF6 ID
 * \param next_id         [OUT] Next OSPF6 ID
 * \param current_nip     [IN]  Pointer to current neighbor IP.
 * \param next_nip        [OUT] Next neighbor IP.
 * \param current_ifidx   [IN]  Pointer to current neighbor ifindex.
 * \param next_ifidx      [OUT] Next neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_itr(
    const vtss_appl_ospf6_id_t *const current_id,
    vtss_appl_ospf6_id_t       *const next_id,
    const mesa_ipv6_t         *const current_nip,
    mesa_ipv6_t               *const next_nip,
    const vtss_ifindex_t      *const current_ifidx,
    vtss_ifindex_t            *const next_ifidx);

/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by CLI and JSON/Private MIB
 * \param current_id      [IN]  Current OSPF6 ID
 * \param next_id         [OUT] Next OSPF6 ID
 * \param current_nid     [IN]  Pointer to current neighbor's router id.
 * \param next_nid        [OUT] Next neighbor's router id.
 * \param current_nip     [IN]  Pointer to current neighbor IP.
 * \param next_nip        [OUT] Next to next neighbor ip.
 * \param current_ifidx   [IN]  Pointer to current neighbor ifindex.
 * \param next_ifidx      [OUT] Next to next neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_itr2(
    const vtss_appl_ospf6_id_t        *const current_id,
    vtss_appl_ospf6_id_t              *const next_id,
    const vtss_appl_ospf6_router_id_t *const current_nid,
    vtss_appl_ospf6_router_id_t       *const next_nid,
    const mesa_ipv6_t                *const current_nip,
    mesa_ipv6_t                      *const next_nip,
    const vtss_ifindex_t             *const current_ifidx,
    vtss_ifindex_t                   *const next_ifidx);

/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by Standard MIB - ospfVirtNbrTable (key: Transit Area and Router ID)
 * \param current_id                [IN]  Current OSPF6 ID.
 * \param next_id                   [OUT] Next OSPF6 ID.
 * \param current_transit_area_id   [IN]  Pointer to current transit area id.
 * \param next_transit_area_id      [OUT] Next entry transit area id.
 * \param current_nid               [IN]  Pointer to current neighbor id.
 * \param next_nid                  [OUT] Next entry neighbor id.
 * \param current_nip               [IN]  Pointer to current neighbor IP.
 * \param next_nip                  [OUT] Next entry neighbor IP.
 * \param current_ifidx             [IN]  Pointer to current neighbor ifindex.
 * \param next_ifidx                [OUT] Next entry neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_itr3(
    const vtss_appl_ospf6_id_t *const current_id,
    vtss_appl_ospf6_id_t *const next_id,
    const vtss_appl_ospf6_area_id_t *const current_transit_area_id,
    vtss_appl_ospf6_area_id_t *const next_transit_area_id,
    const vtss_appl_ospf6_router_id_t *const current_nid,
    vtss_appl_ospf6_router_id_t *const next_nid,
    const mesa_ipv6_t *const current_nip, mesa_ipv6_t *const next_nip,
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx);

/** The search doesn't match neighbor's router ID.  */
#define VTSS_APPL_OSPF6_DONTCARE_NID  (0)

/**
 * \brief Get status for a neighbor information.
 * \param id             [IN]  OSPF6 instance ID.
 * \param neighbor_id    [IN]  Neighbor's router ID.
 *  If the neighbor_id is VTSS_APPL_OSPF6_DONTCARE_NID,
 *  it doesn't match the neighbor ID
 * \param neighbor_ip    [IN]  Neighbor's IP.
 * \param neighbor_ifidx [IN]  Neighbor's ifindex.
 * \param status         [OUT] Neighbor Status.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_get(
    const vtss_appl_ospf6_id_t           id,
    const vtss_appl_ospf6_router_id_t    neighbor_id,
    const mesa_ipv6_t                   neighbor_ip,
    const vtss_ifindex_t                neighbor_ifidx,
    vtss_appl_ospf6_neighbor_status_t    *const status);

/**
 * \brief Get status for all neighbor information.
 * \param neighbors [OUT] An container with all neighbor.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_neighbor_status_get_all(
    vtss::Vector<vtss_appl_ospf6_neighbor_data_t>
    &neighbors);

/** \brief The route type of the OSPF6 route entry. */
typedef enum {
    /** The destination is an OSPF6 route which is located on intra-area. */
    VTSS_APPL_OSPF6_ROUTE_TYPE_INTRA_AREA,

    /** The destination is an OSPF6 route which is located on inter-area. */
    VTSS_APPL_OSPF6_ROUTE_TYPE_INTER_AREA,

    /** The destination is a border router. */
    VTSS_APPL_OSPF6_ROUTE_TYPE_BORDER_ROUTER,

    /** The destination is an external Type-1 route. */
    VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_1,

    /** The destination is an external Type-2 route. */
    VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_2,

    /** The route type isn't supported. */
    VTSS_APPL_OSPF6_ROUTE_TYPE_UNKNOWN,
} vtss_appl_ospf6_route_type_t;

/** \brief The border router type of the OSPF6 route entry. */
typedef enum {
    /** The border router is an ABR. */
    VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_ABR,

    /** The border router is an ASBR located on Intra-area. */
    VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_INTRA_AREA_ASBR,

    /** The border router is an ASBR located on Inter-area. */
    VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_INTER_AREA_ASBR,

    /** The border router is an ASBR attached to at least 2 areas. */
    VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_ABR_ASBR,

    /** The entry isn't a border router. */
    VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_NONE,
} vtss_appl_ospf6_route_br_type_t;

/** \brief The data structure for the OSPF6 route entry. */
typedef struct {
    /** The cost of the route or router path. */
    vtss_appl_ospf6_cost_t                   cost;

    /** The cost of the route within the OSPF6 network. It is valid for external
      * Type-2 route and always '0' for other route type.
     */
    vtss_appl_ospf6_cost_t                   as_cost;

    /** The area which the route or router can be reached via/to. */
    vtss_appl_ospf6_area_id_t                area;

    /** The border router type. */
    vtss_appl_ospf6_route_br_type_t          border_router_type;

    /** The destination is connected directly or not. */
    mesa_bool_t                             connected;

    /** The interface where the ip packet is outgoing. */
    vtss_ifindex_t                          ifindex;
} vtss_appl_ospf6_route_status_t;

/** \brief The data structure for getting all OSPF6 IPv6 route entries. */
typedef struct {
    /** The OSPF6 ID. */
    vtss_appl_ospf6_id_t             id;

    /** The route type. */
    vtss_appl_ospf6_route_type_t     rt_type;

    /** The destination. */
    mesa_ipv6_network_t             dest;

    /** The nexthop. */
    mesa_ipv6_t                     nexthop;

    /** The area which the route or router can be reached via/to. */
    vtss_appl_ospf6_area_id_t        area;

    /** The OSPF6 routing information. */
    vtss_appl_ospf6_route_status_t   status;
} vtss_appl_ospf6_route_ipv6_data_t;

/**
 * \brief Iterate through the OSPF6 IPv4 route entries.
 * \param current_id        [IN]  The current OSPF6 ID.
 * \param next_id           [OUT] The next OSPF6 ID.
 * \param current_rt_type   [IN]  The current route type.
 * \param next_rt_type      [OUT] The next route type.
 * \param current_dest      [IN]  The current destination.
 * \param next_dest         [OUT] The next destination.
 * \param current_area      [IN]  The current area.
 * \param next_area         [OUT] The next area.
 * \param current_nexthop   [IN]  The current nexthop.
 * \param next_nexthop      [OUT] The next nexthop.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_route_ipv6_status_itr(
    const vtss_appl_ospf6_id_t          *const current_id,
    vtss_appl_ospf6_id_t                *const next_id,
    const vtss_appl_ospf6_route_type_t  *const current_rt_type,
    vtss_appl_ospf6_route_type_t        *const next_rt_type,
    const mesa_ipv6_network_t          *const current_dest,
    mesa_ipv6_network_t                *const next_dest,
    const vtss_appl_ospf6_area_id_t     *const current_area,
    vtss_appl_ospf6_area_id_t           *const next_area,
    const mesa_ipv6_t                  *const current_nexthop,
    mesa_ipv6_t                        *const next_nexthop);

/**
 * \brief Get the specific OSPF6 IPv4 route entry.
 * \param id      [IN]  The OSPF6 instance ID.
 * \param rt_type [IN]  The route type.
 * \param dest    [IN]  The destination.
 * \param area    [IN]  The area.
 * \param nexthop [IN]  The nexthop.
 * \param status  [OUT] The OSPF6 route status.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_route_ipv6_status_get(
    const vtss_appl_ospf6_id_t           id,
    const vtss_appl_ospf6_route_type_t   rt_type,
    const mesa_ipv6_network_t           dest,
    const vtss_appl_ospf6_area_id_t      area,
    const mesa_ipv6_t                   nexthop,
    vtss_appl_ospf6_route_status_t       *const status);

/**
 * \brief Get all OSPF6 IPv4 route entries.
 * \param routes [OUT] The container with all routes.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_route_ipv6_status_get_all(
    vtss::Vector<vtss_appl_ospf6_route_ipv6_data_t>
    &routes);

//----------------------------------------------------------------------------
//** OSPF6 database information
//----------------------------------------------------------------------------
/** \brief The type of the link state advertisement. */
typedef enum {
    /** No LS database type is specified. */
    VTSS_APPL_OSPF6_LSDB_TYPE_NONE             = 0,

    /** The link states. */
    VTSS_APPL_OSPF6_LSDB_TYPE_LINK             = 0x1,

    /** Router link states. */
    VTSS_APPL_OSPF6_LSDB_TYPE_ROUTER           = 0x2,

    /** Network link state. */
    VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK          = 0x3,

    /** InterArea Prefix link state. */
    VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX = 0x4,

    /** InterArea Router link state. */
    VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_ROUTER = 0x5,

    /** NSSA external link state. */
    VTSS_APPL_OSPF6_LSDB_TYPE_NSSA_EXTERNAL    = 0x6,

    /** Intraarea prefix link state. */
    VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX = 0x7,

    /** External link state. */
    VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL         = 0x8,

    /** Type unknown to OSPF */
    VTSS_APPL_OSPF6_LSDB_TYPE_UNKNOWN,
} vtss_appl_ospf6_lsdb_type_t;

/** \brief The OSPF6 link state database general summary information. */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The link count of router link state.
     *  (The parameter is used for Type 1 only) */
    uint32_t router_link_count;
} vtss_appl_ospf6_db_general_info_t;

/** \brief The OSPF6 link state database entry (For get_all API). */
typedef struct {
    /** The OSPF6 instance ID. */
    vtss_appl_ospf6_id_t inst_id;

    /**  The type of the link state advertisement. */
    vtss_appl_ospf6_lsdb_type_t lsdb_type;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf6_area_id_t area_id;

    /** The OSPF6 link state ID. It identifies the piece of the routing domain
     * that is being described by the LSA.
     * The field value has various meanings based on the LS type.
     *  LS Type     LS Description        Link State ID
     *  -------     --------------        -------------
     *  0x2001      Router-LSAs           The originating router's Router ID.
     *  0x2002      Network-LSAs          The originating Interface ID.
     *  0x2003      inter_area-prefix-LSAs Has No significance, distinguish multiple LSAs originated by same routers.
     *  0x2004      inter_area-router-LSAs Has No significance, distinguish multiple LSAs originated by same router.
     *  0x4005      AS-external-LSAs      Has No significance, distinguish multiple LSAs originated by same router.
     *  0x2007      NSSA-external-LSAs    Has No significance, distinguish multiple LSAs originated by same router.
     *  0x8         Link-LSAs             The originating Interface ID.
     */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf6_router_id_t adv_router_id;

    /** The database general summary informaton. */
    vtss_appl_ospf6_db_general_info_t db;
} vtss_appl_ospf6_db_entry_t;

/**
 * \brief Iterate through the OSPF6 database information.
 * \param cur_inst_id        [IN]  The current OSPF6 ID.
 * \param next_inst_id       [OUT] The next OSPF6 ID.
 * \param cur_area_id        [IN]  The currenr Area ID
 * \param next_area_id       [OUT] Then next Area ID
 * \param cur_lsdb_type      [IN]  The current LS database type.
 * \param next_lsdb_type     [OUT] The next LS database type.
 * \param cur_link_state_id  [IN]  The current link state ID.
 * \param next_link_state_id [OUT] The next link state ID.
 * \param cur_router_id      [IN]  The current advertising router ID.
 * \param next_router_id     [OUT] The next advertising router ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_itr(
    const vtss_appl_ospf6_id_t        *const cur_inst_id,
    vtss_appl_ospf6_id_t              *const next_inst_id,
    const mesa_ipv4_t                 *const cur_area_id,
    mesa_ipv4_t                       *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t       *const next_router_id);

/**
 * \brief Get the specific OSPF6 database general summary information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The Area ID
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_general_info [OUT] The OSPF6 database general summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_get(
    const vtss_appl_ospf6_id_t        inst_id,
    const mesa_ipv4_t                 area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_general_info_t *const db_general_info);

/**
 * \brief Get all OSPF6 database entries of geranal information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_get_all(
    vtss::Vector<vtss_appl_ospf6_db_entry_t>
    &db_entries);

//----------------------------------------------------------------------------
//** OSPF6 database detail common information
//----------------------------------------------------------------------------

/** \brief The OSPF6 link state database type 1 link information. */
typedef struct {

    /** link connected type */
    uint32_t link_connected_to;

    /** Link ID address */
    mesa_ipv4_t link_id;

    /** Link Network Mask */
    mesa_ipv4_t link_data;

    /** User specified metric for this summary route. */
    uint32_t metric;
} vtss_appl_ospf6_db_router_link_info_t;

//----------------------------------------------------------------------------
//** OSPF6 database detail router information
//----------------------------------------------------------------------------

/** \brief The OSPF6 link state database router entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF6 hello packets */
    uint32_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** The link count of router link state.
     *  (The parameter is used for Type 1 only) */
    uint32_t router_link_count;

} vtss_appl_ospf6_db_router_data_entry_t;

/** \brief The OSPF6 link state database detail router entry (For get_all API). */
typedef struct {
    /** The OSPF6 instance ID. */
    vtss_appl_ospf6_id_t inst_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf6_lsdb_type_t lsdb_type;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf6_area_id_t area_id;

    /** The OSPF6 link state ID. It identifies the piece of the routing domain
      * that is being described by the LSA.
      * The field value has various meanings based on the LS type.
      *  LS Type     LS Description        Link State ID
      *  -------     --------------        -------------
      *  0x2001      Router-LSAs           The originating router's Router ID.
      *  0x2002      Network-LSAs          The originating Interface ID.
      *  0x2003      inter_area-prefix-LSAs Has No significance, distinguish multiple LSAs originated by same routers.
      *  0x2004      inter_area-router-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x4005      AS-external-LSAs      Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2007      NSSA-external-LSAs    Has No significance, distinguish multiple LSAs originated by same router.
      *  0x8         Link-LSAs             The originating Interface ID.
      */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf6_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf6_db_router_data_entry_t data;

} vtss_appl_ospf6_db_detail_router_entry_t;

/**
 * \brief Iterate through the OSPF6 database detail router information.
 * \param cur_inst_id        [IN]  The current OSPF6 ID.
 * \param next_inst_id       [OUT] The next OSPF6 ID.
 * \param cur_area_id        [IN]  The current area ID.
 * \param next_area_id       [OUT] The next area ID.
 * \param cur_lsdb_type      [IN]  The current LS database type.
 * \param next_lsdb_type     [OUT] The next LS database type.
 * \param cur_link_state_id  [IN]  The current link state ID.
 * \param next_link_state_id [OUT] The next link state ID.
 * \param cur_router_id      [IN]  The current advertising router ID.
 * \param next_router_id     [OUT] The next advertising router ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_router_itr(
    const vtss_appl_ospf6_id_t        *const cur_inst_id,
    vtss_appl_ospf6_id_t              *const next_inst_id,
    const vtss_appl_ospf6_area_id_t   *const cur_area_id,
    vtss_appl_ospf6_area_id_t         *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF6 database detail router information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_router_get(
    const vtss_appl_ospf6_id_t        inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_router_data_entry_t *const db_detail_info);

/**
 * \brief Get the OSPF6 database detail router information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param index           [IN]  The index of the router link.
 * \param link_info       [OUT] The OSPF6 database detail link information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_router_entry_get_by_index(
    const vtss_appl_ospf6_id_t inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    const uint32_t index,
    vtss_appl_ospf6_db_router_link_info_t *const link_info);

/**
 * \brief Get all OSPF6 database entries of detail router information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_router_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_router_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF6 database detail network information
//----------------------------------------------------------------------------

/** \brief The OSPF6 link state database network entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF6 hello packets */
    uint32_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** Count of routers attached to the network. */
    uint32_t attached_router_count; // Need for type 2

} vtss_appl_ospf6_db_network_data_entry_t;

/** \brief The OSPF6 link state database detail network entry (For get_all API). */
typedef struct {
    /** The OSPF6 instance ID. */
    vtss_appl_ospf6_id_t inst_id;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf6_area_id_t area_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf6_lsdb_type_t lsdb_type;

    /** The OSPF6 link state ID. It identifies the piece of the routing domain
      * that is being described by the LSA.
      * The field value has various meanings based on the LS type.
      *  LS Type     LS Description        Link State ID
      *  -------     --------------        -------------
      *  0x2001      Router-LSAs           The originating router's Router ID.
      *  0x2002      Network-LSAs          The originating Interface ID.
      *  0x2003      inter_area-prefix-LSAs Has No significance, distinguish multiple LSAs originated by same routers.
      *  0x2004      inter_area-router-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x4005      AS-external-LSAs      Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2007      NSSA-external-LSAs    Has No significance, distinguish multiple LSAs originated by same router.
      *  0x8         Link-LSAs             The originating Interface ID.
      */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf6_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf6_db_network_data_entry_t data;

} vtss_appl_ospf6_db_detail_network_entry_t;

/**
 * \brief Iterate through the OSPF6 database detail network information.
 * \param cur_inst_id        [IN]  The current OSPF6 ID.
 * \param next_inst_id       [OUT] The next OSPF6 ID.
 * \param cur_area_id        [IN]  The current area ID.
 * \param next_area_id       [OUT] The next area ID.
 * \param cur_lsdb_type      [IN]  The current LS database type.
 * \param next_lsdb_type     [OUT] The next LS database type.
 * \param cur_link_state_id  [IN]  The current link state ID.
 * \param next_link_state_id [OUT] The next link state ID.
 * \param cur_router_id      [IN]  The current advertising router ID.
 * \param next_router_id     [OUT] The next advertising router ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_network_itr(
    const vtss_appl_ospf6_id_t        *const cur_inst_id,
    vtss_appl_ospf6_id_t              *const next_inst_id,
    const vtss_appl_ospf6_area_id_t   *const cur_area_id,
    vtss_appl_ospf6_area_id_t         *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF6 database detail network information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_network_get(
    const vtss_appl_ospf6_id_t        inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_network_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF6 database entries of detail network information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_network_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_network_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF6 database detail inter-area prefix information
//----------------------------------------------------------------------------

/** \brief The OSPF6 link state database detail inter_area prefix entry (For get_all API). */

/** \brief The OSPF6 link state database inter_area entry prefix (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF6 hello packets */
    uint32_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** Network mask implemented. */
    mesa_ipv6_network_t prefix;

    /** User specified metric for this inter-area route. */
    uint32_t metric;
} vtss_appl_ospf6_db_inter_area_prefix_data_entry_t;

/** \brief The OSPF6 link state database detail inter_area prefix entry (For get_all API). */
typedef struct {
    /** The OSPF6 instance ID. */
    vtss_appl_ospf6_id_t inst_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf6_lsdb_type_t lsdb_type;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf6_area_id_t area_id;

    /** The OSPF6 link state ID. It identifies the piece of the routing domain
      * that is being described by the LSA.
      * The field value has various meanings based on the LS type.
      *  LS Type     LS Description        Link State ID
      *  -------     --------------        -------------
      *  0x2001      Router-LSAs           The originating router's Router ID.
      *  0x2002      Network-LSAs          The originating Interface ID.
      *  0x2003      inter_area-prefix-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2004      inter_area-router-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x4005      AS-external-LSAs      Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2007      NSSA-external-LSAs    Has No significance, distinguish multiple LSAs originated by same router.
      *  0x8         Link-LSAs             The originating Interface ID.
      */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf6_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf6_db_inter_area_prefix_data_entry_t data;

} vtss_appl_ospf6_db_detail_inter_area_prefix_entry_t;

/**
 * \brief Iterate through the OSPF6 database detail inter_area prefix information.
 * \param cur_inst_id        [IN]  The current OSPF6 ID.
 * \param next_inst_id       [OUT] The next OSPF6 ID.
 * \param cur_area_id        [IN]  The current area ID.
 * \param next_area_id       [OUT] The next area ID.
 * \param cur_lsdb_type      [IN]  The current LS database type.
 * \param next_lsdb_type     [OUT] The next LS database type.
 * \param cur_link_state_id  [IN]  The current link state ID.
 * \param next_link_state_id [OUT] The next link state ID.
 * \param cur_router_id      [IN]  The current advertising router ID.
 * \param next_router_id     [OUT] The next advertising router ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_prefix_itr(
    const vtss_appl_ospf6_id_t        *const cur_inst_id,
    vtss_appl_ospf6_id_t              *const next_inst_id,
    const vtss_appl_ospf6_area_id_t   *const cur_area_id,
    vtss_appl_ospf6_area_id_t         *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF6 database detail inter_area prefix information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail inter_area information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_prefix_get(
    const vtss_appl_ospf6_id_t        inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_inter_area_prefix_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF6 database entries of detail inter_area prefix information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_prefix_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_inter_area_prefix_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF6 database detail inter-area router information
//----------------------------------------------------------------------------

/** \brief The OSPF6 link state database detail inter_area router entry (For get_all API). */

/** \brief The OSPF6 link state database inter_area router entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF6 hello packets */
    uint32_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** Destination Router ID */
    mesa_ipv4_t destination_router_id;

    /** User specified metric for this inter-area route. */
    uint32_t metric;
} vtss_appl_ospf6_db_inter_area_router_data_entry_t;

/** \brief The OSPF6 link state database detail inter_area router entry (For get_all API). */
typedef struct {
    /** The OSPF6 instance ID. */
    vtss_appl_ospf6_id_t inst_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf6_lsdb_type_t lsdb_type;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf6_area_id_t area_id;

    /** The OSPF6 link state ID. It identifies the piece of the routing domain
      * that is being described by the LSA.
      * The field value has various meanings based on the LS type.
      *  LS Type     LS Description        Link State ID
      *  -------     --------------        -------------
      *  0x2001      Router-LSAs           The originating router's Router ID.
      *  0x2002      Network-LSAs          The originating Interface ID.
      *  0x2003      inter_area-prefix-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2004      inter_area-router-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x4005      AS-external-LSAs      Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2007      NSSA-external-LSAs    Has No significance, distinguish multiple LSAs originated by same router.
      *  0x8         Link-LSAs             The originating Interface ID.
      */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf6_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf6_db_inter_area_router_data_entry_t data;

} vtss_appl_ospf6_db_detail_inter_area_router_entry_t;

/**
 * \brief Iterate through the OSPF6 database detail inter_area router information.
 * \param cur_inst_id        [IN]  The current OSPF6 ID.
 * \param next_inst_id       [OUT] The next OSPF6 ID.
 * \param cur_area_id        [IN]  The current area ID.
 * \param next_area_id       [OUT] The next area ID.
 * \param cur_lsdb_type      [IN]  The current LS database type.
 * \param next_lsdb_type     [OUT] The next LS database type.
 * \param cur_link_state_id  [IN]  The current link state ID.
 * \param next_link_state_id [OUT] The next link state ID.
 * \param cur_router_id      [IN]  The current advertising router ID.
 * \param next_router_id     [OUT] The next advertising router ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_router_itr(
    const vtss_appl_ospf6_id_t        *const cur_inst_id,
    vtss_appl_ospf6_id_t              *const next_inst_id,
    const vtss_appl_ospf6_area_id_t   *const cur_area_id,
    vtss_appl_ospf6_area_id_t         *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF6 database detail inter_area router information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail inter_area information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_router_get(
    const vtss_appl_ospf6_id_t        inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_inter_area_router_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF6 database entries of detail inter_area-router information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_router_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_inter_area_router_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF6 database detail external/nssa-external information
//----------------------------------------------------------------------------

/** \brief The OSPF6 link state database external entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF6 hello packets */
    uint32_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** Network mask implemented. */
    mesa_ipv6_network_t prefix;

    /** User specified metric for this summary route. */
    uint32_t metric; // Need for type 3, 4, 5

    /** External type. */
    uint32_t metric_type; // Need for type 5

    /** Forwarding address. Data traffic for the advertised destination will
    be forwarded to this address.
    */
    mesa_ipv6_t forward_address; // Need for type 5

} vtss_appl_ospf6_db_external_data_entry_t;

/** \brief The OSPF6 link state database detail external/nssa-external entry (For get_all API). */
typedef struct {
    /** The OSPF6 instance ID. */
    vtss_appl_ospf6_id_t inst_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf6_lsdb_type_t lsdb_type;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf6_area_id_t area_id;

    /** The OSPF6 link state ID. It identifies the piece of the routing domain
      * that is being described by the LSA.
      * The field value has various meanings based on the LS type.
      *  LS Type     LS Description        Link State ID
      *  -------     --------------        -------------
      *  0x2001      Router-LSAs           The originating router's Router ID.
      *  0x2002      Network-LSAs          The originating Interface ID.
      *  0x2003      inter_area-prefix-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2004      inter_area-router-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x4005      AS-external-LSAs      Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2007      NSSA-external-LSAs    Has No significance, distinguish multiple LSAs originated by same router.
      *  0x8         Link-LSAs             The originating Interface ID.
      */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf6_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf6_db_external_data_entry_t data;

} vtss_appl_ospf6_db_detail_external_entry_t;

/**
 * \brief Iterate through the OSPF6 database detail external information.
 * \param cur_inst_id        [IN]  The current OSPF6 ID.
 * \param next_inst_id       [OUT] The next OSPF6 ID.
 * \param cur_area_id        [IN]  The current area ID.
 * \param next_area_id       [OUT] The next area ID.
 * \param cur_lsdb_type      [IN]  The current LS database type.
 * \param next_lsdb_type     [OUT] The next LS database type.
 * \param cur_link_state_id  [IN]  The current link state ID.
 * \param next_link_state_id [OUT] The next link state ID.
 * \param cur_router_id      [IN]  The current advertising router ID.
 * \param next_router_id     [OUT] The next advertising router ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_external_itr(
    const vtss_appl_ospf6_id_t        *const cur_inst_id,
    vtss_appl_ospf6_id_t              *const next_inst_id,
    const vtss_appl_ospf6_area_id_t   *const cur_area_id,
    vtss_appl_ospf6_area_id_t         *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF6 database detail external information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_external_get(
    const vtss_appl_ospf6_id_t        inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_external_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF6 database entries of detail external information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_external_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_external_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF6 database detail link information
//----------------------------------------------------------------------------
/** \brief The OSPF6 link state database Prefix Information */
typedef struct {

    /** Link prefix */
    mesa_ipv6_t link_prefix;

    /** Prefix length */
    uint32_t link_prefix_len;

    /** Prefix Options */
    uint8_t prefix_options;

} vtss_appl_ospf6_db_link_info_t;

/** \brief The OSPF6 link state database link entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF6 hello packets */
    uint32_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** Link Local Address of the link */
    mesa_ipv6_t link_local_address;

    /** The link count of router link state.
     *  (The parameter is used for Type 1 only) */
    uint32_t prefix_cnt;

} vtss_appl_ospf6_db_link_data_entry_t;

/** \brief The OSPF6 link state database detail link entry (For get_all API). */
typedef struct {
    /** The OSPF6 instance ID. */
    vtss_appl_ospf6_id_t inst_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf6_lsdb_type_t lsdb_type;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf6_area_id_t area_id;

    /** The OSPF6 link state ID. It identifies the piece of the routing domain
      * that is being described by the LSA.
      * The field value has various meanings based on the LS type.
      *  LS Type     LS Description        Link State ID
      *  -------     --------------        -------------
      *  0x2001      Router-LSAs           The originating router's Router ID.
      *  0x2002      Network-LSAs          The originating Interface ID.
      *  0x2003      inter_area-prefix-LSAs Has No significance, distinguish multiple LSAs originated by same routers.
      *  0x2004      inter_area-router-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x4005      AS-external-LSAs      Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2007      NSSA-external-LSAs    Has No significance, distinguish multiple LSAs originated by same router.
      *  0x8         Link-LSAs             The originating Interface ID.
      */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf6_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf6_db_link_data_entry_t data;

} vtss_appl_ospf6_db_detail_link_entry_t;

/**
 * \brief Iterate through the OSPF6 database detail link information.
 * \param cur_inst_id        [IN]  The current OSPF6 ID.
 * \param next_inst_id       [OUT] The next OSPF6 ID.
 * \param cur_area_id        [IN]  The current area ID.
 * \param next_area_id       [OUT] The next area ID.
 * \param cur_lsdb_type      [IN]  The current LS database type.
 * \param next_lsdb_type     [OUT] The next LS database type.
 * \param cur_link_state_id  [IN]  The current link state ID.
 * \param next_link_state_id [OUT] The next link state ID.
 * \param cur_router_id      [IN]  The current advertising router ID.
 * \param next_router_id     [OUT] The next advertising router ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_link_itr(
    const vtss_appl_ospf6_id_t        *const cur_inst_id,
    vtss_appl_ospf6_id_t              *const next_inst_id,
    const vtss_appl_ospf6_area_id_t   *const cur_area_id,
    vtss_appl_ospf6_area_id_t         *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF6 database detail router information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_link_get(
    const vtss_appl_ospf6_id_t        inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_link_data_entry_t *const db_detail_info);

/**
 * \brief Get the OSPF6 database detail link information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param index           [IN]  The index of the router link.
 * \param link_info       [OUT] The OSPF6 database detail link information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_link_entry_get_by_index(
    const vtss_appl_ospf6_id_t inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    const uint32_t index,
    vtss_appl_ospf6_db_link_info_t *const link_info);

/**
 * \brief Get all OSPF6 database entries of detail link information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_link_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_link_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF6 database detail intra area prefix information
//----------------------------------------------------------------------------
/** \brief The OSPF6 link state database intra area entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** The option field which is present in OSPF6 hello packets */
    uint32_t options;

    /** The link count of router link state.
     *  (The parameter is used for Type 1 only) */
    uint32_t prefix_cnt;

} vtss_appl_ospf6_db_intra_area_prefix_data_entry_t;

/** \brief The OSPF6 link state database detail intra area prefix entry (For get_all API). */
typedef struct {
    /** The OSPF6 instance ID. */
    vtss_appl_ospf6_id_t inst_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf6_lsdb_type_t lsdb_type;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf6_area_id_t area_id;

    /** The OSPF6 link state ID. It identifies the piece of the routing domain
      * that is being described by the LSA.
      * The field value has various meanings based on the LS type.
      *  LS Type     LS Description        Link State ID
      *  -------     --------------        -------------
      *  0x2001      Router-LSAs           The originating router's Router ID.
      *  0x2002      Network-LSAs          The originating Interface ID.
      *  0x2003      inter_area-prefix-LSAs Has No significance, distinguish multiple LSAs originated by same routers.
      *  0x2004      inter_area-router-LSAs Has No significance, distinguish multiple LSAs originated by same router.
      *  0x4005      AS-external-LSAs      Has No significance, distinguish multiple LSAs originated by same router.
      *  0x2007      NSSA-external-LSAs    Has No significance, distinguish multiple LSAs originated by same router.
      *  0x8         Link-LSAs             The originating Interface ID.
      */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf6_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf6_db_intra_area_prefix_data_entry_t data;

} vtss_appl_ospf6_db_detail_intra_area_prefix_entry_t;

/**
 * \brief Iterate through the OSPF6 database detail intra area prefix information.
 * \param cur_inst_id        [IN]  The current OSPF6 ID.
 * \param next_inst_id       [OUT] The next OSPF6 ID.
 * \param cur_area_id        [IN]  The current area ID.
 * \param next_area_id       [OUT] The next area ID.
 * \param cur_lsdb_type      [IN]  The current LS database type.
 * \param next_lsdb_type     [OUT] The next LS database type.
 * \param cur_link_state_id  [IN]  The current link state ID.
 * \param next_link_state_id [OUT] The next link state ID.
 * \param cur_router_id      [IN]  The current advertising router ID.
 * \param next_router_id     [OUT] The next advertising router ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_intra_area_prefix_itr(
    const vtss_appl_ospf6_id_t        *const cur_inst_id,
    vtss_appl_ospf6_id_t              *const next_inst_id,
    const vtss_appl_ospf6_area_id_t   *const cur_area_id,
    vtss_appl_ospf6_area_id_t         *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF6 database detail intra area prefix information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_intra_area_prefix_get(
    const vtss_appl_ospf6_id_t        inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_intra_area_prefix_data_entry_t *const db_detail_info);

/**
 * \brief Get the OSPF6 database detail intra area prefix information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param index           [IN]  The index of the router link.
 * \param link_info       [OUT] The OSPF6 database detail link information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_intra_area_prefix_entry_get_by_index(
    const vtss_appl_ospf6_id_t inst_id,
    const vtss_appl_ospf6_area_id_t   area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    const uint32_t index,
    vtss_appl_ospf6_db_link_info_t *const link_info);

/**
 * \brief Get all OSPF6 database entries of detail link information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_intra_area_prefix_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_intra_area_prefix_entry_t> &db_entries);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_OSPF6_H_ */
