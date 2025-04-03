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
 * \brief Public OSPF API
 * \details This header file describes OSPF control functions and types.
 */

#ifndef _VTSS_APPL_OSPF_H_
#define _VTSS_APPL_OSPF_H_

#include <microchip/ethernet/switch/api/types.h>  // For type declarations
#include <vtss/appl/module_id.h>             // For MODULE_ERROR_START()
#include <vtss/appl/interface.h>             // For vtss_ifindex_t
#include <vtss/basics/map.hxx>
#include <vtss/basics/vector.hxx>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//** OSPF type declaration
//----------------------------------------------------------------------------

/** \brief The data type of OSPF instance ID. */
typedef uint32_t vtss_appl_ospf_id_t;

/** \brief The data type of OSPF area ID. */
typedef mesa_ipv4_t vtss_appl_ospf_area_id_t;

/** \brief The data type of OSPF router ID. */
typedef mesa_ipv4_t vtss_appl_ospf_router_id_t;

/** \brief The data type of OSPF priority value. */
typedef uint32_t vtss_appl_ospf_priority_t;

/** \brief The data type of OSPF cost value. */
typedef uint32_t vtss_appl_ospf_cost_t;

/** \brief The data type of OSPF metric value. */
typedef uint32_t vtss_appl_ospf_metric_t;

/** \brief The message digest key ID. */
typedef uint8_t vtss_appl_ospf_md_key_id_t;

//----------------------------------------------------------------------------
//** OSPF module error codes
//----------------------------------------------------------------------------

/** \brief FRR error return codes (mesa_rc) */
enum {
    /** Invalid router ID */
    VTSS_APPL_FRR_OSPF_ERROR_INVALID_ROUTER_ID = MODULE_ERROR_START(VTSS_MODULE_ID_FRR_OSPF),

    /** The OSPF router ID change doesn't take effect */
    VTSS_APPL_FRR_OSPF_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT,

    /** The OSPF area ID change doesn't take effect */
    VTSS_APPL_FRR_OSPF_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT,

    /** Backbone can not be configured as stub area */
    VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE,

    /* This area contains virtual link, can not be configured as stub area */
    VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_VIRTUAL_LINK,

    /** The password/key is too long */
    VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_TOO_LONG,

    /** The password/key is invalid */
    VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID,

    /** Backbone area can not be configured as virtual link */
    VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_BACKBONE,

    /** Virtual link can not be configured in stub area */
    VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_STUB,

    /** Area range not-advertise and cost can not be set at the same time */
    VTSS_APPL_FRR_OSPF_ERROR_AREA_RANGE_COST_CONFLICT,

    /** Area range network address can't be default */
    VTSS_APPL_FRR_OSPF_ERROR_AREA_RANGE_NETWORK_DEFAULT,

    /** Cannot enable OSPF due to deffered shutdown time in progress */
    VTSS_APPL_FRR_OSPF_ERROR_DEFFERED_SHUTDOWN_IN_PROGRESS
};

//----------------------------------------------------------------------------
//** OSPF variables valid range
//----------------------------------------------------------------------------
/**< The valid range of OSPF instance ID. (multiple instance is unsupported yet) */
constexpr uint32_t VTSS_APPL_OSPF_INSTANCE_ID_START = 1;                        /**< Valid starting ID of OSPF instance. */
constexpr uint32_t VTSS_APPL_OSPF_INSTANCE_ID_MAX   = 1;                        /**< Maximum ID of OSPF instance. */

/**< The valid range of OSPF router ID. (1-4294967294 or 0.0.0.1-255.255.255.254) */
constexpr vtss_appl_ospf_router_id_t VTSS_APPL_OSPF_ROUTER_ID_MIN = 1;          /**< Minimum value of OSPF router ID. */
constexpr vtss_appl_ospf_router_id_t VTSS_APPL_OSPF_ROUTER_ID_MAX = 4294967294; /**< Maximum value of OSPF router ID. */

/**< The valid range of OSPF priority. (0-255) */
constexpr vtss_appl_ospf_priority_t VTSS_APPL_OSPF_PRIORITY_MIN = 0;            /**< Minimum value of OSPF priority. */
constexpr vtss_appl_ospf_priority_t VTSS_APPL_OSPF_PRIORITY_MAX = 255;          /**< Maximum value of OSPF priority. */

/**< The valid range of OSPF general cost. (0-16777215) */
constexpr vtss_appl_ospf_cost_t VTSS_APPL_OSPF_GENERAL_COST_MIN = 0;            /**< Minimum value of OSPF general cost. */
constexpr vtss_appl_ospf_cost_t VTSS_APPL_OSPF_GENERAL_COST_MAX = 16777215;     /**< Maximum value of OSPF general cost. */

/**< The valid range of OSPF interface cost. (1-65535) */
constexpr vtss_appl_ospf_cost_t VTSS_APPL_OSPF_INTF_COST_MIN = 1;               /**< Minimum value of OSPF interface cost. */
constexpr vtss_appl_ospf_cost_t VTSS_APPL_OSPF_INTF_COST_MAX = 65535;           /**< Maximum value of OSPF interface cost. */

/**< The valid range of OSPF redistribute cost. (0-16777214) */
constexpr vtss_appl_ospf_cost_t VTSS_APPL_OSPF_REDIST_COST_MIN = 0;             /**< Minimum value of OSPF redistribute cost. */
constexpr vtss_appl_ospf_cost_t VTSS_APPL_OSPF_REDIST_COST_MAX = 16777214;      /**< Maximum value of OSPF redistribute cost. */

/**< The valid range of OSPF hello interval. (1-65535) */
constexpr uint32_t VTSS_APPL_OSPF_HELLO_INTERVAL_MIN = 1;                       /**< Minimum value of OSPF hello interval. */
constexpr uint32_t VTSS_APPL_OSPF_HELLO_INTERVAL_MAX = 65535;                   /**< Maximum value of OSPF hello interval. */

/**< The valid range of OSPF fast hello packets. (1-10) */
constexpr uint32_t VTSS_APPL_OSPF_FAST_HELLO_MIN = 1;                           /**< Minimum value of OSPF fast hello packets. */
constexpr uint32_t VTSS_APPL_OSPF_FAST_HELLO_MAX = 10;                          /**< Maximum value of OSPF fast hello packets. */

/**< The valid range of OSPF retransmit interval. (3-65535) */
constexpr uint32_t VTSS_APPL_OSPF_RETRANSMIT_INTERVAL_MIN = 3;                  /**< Minimum value of OSPF retransmit interval. */
constexpr uint32_t VTSS_APPL_OSPF_RETRANSMIT_INTERVAL_MAX = 65535;              /**< Maximum value of OSPF retransmit interval. */

/**< The valid range of OSPF dead interval. (1-65535) */
constexpr uint32_t VTSS_APPL_OSPF_DEAD_INTERVAL_MIN = 1;                        /**< Minimum value of OSPF dead interval. */
constexpr uint32_t VTSS_APPL_OSPF_DEAD_INTERVAL_MAX = 65535;                    /**< Maximum value of OSPF dead interval. */

/**< The valid range of OSPF max-metric router LSA on startup stage. (5-86400) */
constexpr uint32_t VTSS_APPL_OSPF_ROUTER_LSA_STARTUP_MIN = 5;                   /**< Minimum value of OSPF max metric on startup stage. */
constexpr uint32_t VTSS_APPL_OSPF_ROUTER_LSA_STARTUP_MAX = 86400;               /**< Maximum value of OSPF max metric on startup stage. */

/**< The valid range of OSPF max-metric router LSA on shutdown stage. (5-100) */
constexpr uint32_t VTSS_APPL_OSPF_ROUTER_LSA_SHUTDOWN_MIN = 5;                  /**< Minimum value of OSPF max metric on shutdown stage. */
constexpr uint32_t VTSS_APPL_OSPF_ROUTER_LSA_SHUTDOWN_MAX = 100;                /**< Maximum value of OSPF max metric on shutdown stage. */

/**< The valid range of OSPF authentication message digest key. (1-255) */
constexpr vtss_appl_ospf_md_key_id_t VTSS_APPL_OSPF_AUTH_DIGEST_KEY_ID_MIN = 1;     /**< Minimum ID of OSPF authentication message digest key ID. */
constexpr vtss_appl_ospf_md_key_id_t VTSS_APPL_OSPF_AUTH_DIGEST_KEY_ID_MAX = 255;   /**< Maximum ID of OSPF authentication message digest key ID. */

/**< The valid range of OSPF authentication simple password length. (1-8) */
constexpr uint32_t VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MIN_LEN = 1;                  /**< Minimum length of OSPF authentication simple password length. */
constexpr uint32_t VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN = 8;                  /**< Maximum length of OSPF authentication simple password length. */

/**< The valid range of OSPF authentication message digest key length. (1-16) */
constexpr uint32_t VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MIN_LEN = 1;                  /**< Minimum length of OSPF authentication message digest key length. */
constexpr uint32_t VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MAX_LEN = 16;                 /**< Maximum length of OSPF authentication message digest key length. */

/**< The valid range of administrative distance. (1-255) */
constexpr uint8_t VTSS_APPL_OSPF_ADMIN_DISTANCE_MIN = 1;      /**< Minimum value of OSPF administrative distance. */
constexpr uint8_t VTSS_APPL_OSPF_ADMIN_DISTANCE_MAX = 255;    /**< Maximum value of OSPF administrative distance. */

//----------------------------------------------------------------------------
//** OSPF capabilities
//----------------------------------------------------------------------------

/**
 * \brief OSPF capabilities
 */
typedef struct {
    /** Minimum OSPF instance ID */
    vtss_appl_ospf_id_t instance_id_min;

    /** Maximum OSPF instance ID */
    vtss_appl_ospf_id_t instance_id_max;

    /** Minimum OSPF router ID */
    vtss_appl_ospf_router_id_t router_id_min;

    /** Maximum OSPF router ID */
    vtss_appl_ospf_router_id_t router_id_max;

    /** Minimum OSPF priority value */
    vtss_appl_ospf_priority_t priority_min;

    /** Maximum OSPF priority value */
    vtss_appl_ospf_priority_t priority_max;

    /** Minimum OSPF general cost value */
    vtss_appl_ospf_cost_t general_cost_min;

    /** Maximum OSPF general cost value */
    vtss_appl_ospf_cost_t general_cost_max;

    /** Minimum OSPF interface cost value */
    vtss_appl_ospf_cost_t intf_cost_min;

    /** Maximum OSPF interface cost value */
    vtss_appl_ospf_cost_t intf_cost_max;

    /** Minimum OSPF redistribute cost value */
    vtss_appl_ospf_cost_t redist_cost_min;

    /** Maximum OSPF redistribute cost value */
    vtss_appl_ospf_cost_t redist_cost_max;

    /** Minimum OSPF hello interval */
    uint32_t hello_interval_min;

    /** Maximum OSPF hello interval */
    uint32_t hello_interval_max;

    /** Minimum OSPF fast hello packets */
    uint32_t fast_hello_packets_min;

    /** Maximum OSPF fast hello packets */
    uint32_t fast_hello_packets_max;

    /** Minimum OSPF retransmit interval */
    uint32_t retransmit_interval_min;

    /** Maximum OSPF retransmit interval */
    uint32_t retransmit_interval_max;

    /** Minimum OSPF dead interval */
    uint32_t dead_interval_min;

    /** Maximum OSPF dead interval */
    uint32_t dead_interval_max;

    /** Minimum OSPF max-metric router LSA on startup stage */
    uint32_t router_lsa_startup_min;

    /** Maximum OSPF  max-metric router LSA on startup stage */
    uint32_t router_lsa_startup_max;

    /** Minimum OSPF max-metric router LSA on shutdown stage */
    uint32_t router_lsa_shutdown_min;

    /** Maximum OSPF max-metric router LSA on shutdown stage */
    uint32_t router_lsa_shutdown_max;

    /** Minimum OSPF authentication message digest key ID */
    vtss_appl_ospf_md_key_id_t md_key_id_min;

    /** Maximum OSPF authentication message digest key ID */
    vtss_appl_ospf_md_key_id_t md_key_id_max;

    /** Minimum OSPF authentication simple password length */
    uint32_t simple_pwd_len_min;

    /** Maximum OSPF authentication simple password length */
    uint32_t simple_pwd_len_max;

    /** Minimum OSPF authentication message digest key length */
    uint32_t md_key_len_min;

    /** Maximum OSPF authentication message digest key length */
    uint32_t md_key_len_max;

    /** Indicate if RIP redistributed is supported or not */
    mesa_bool_t rip_redistributed_supported;

    /** Minimum value of OSPF administrative distance. */
    uint8_t admin_distance_min = VTSS_APPL_OSPF_ADMIN_DISTANCE_MIN;

    /** Maximum value of RIP administrative distance. */
    uint8_t admin_distance_max = VTSS_APPL_OSPF_ADMIN_DISTANCE_MAX;
} vtss_appl_ospf_capabilities_t;

/**
 * \brief Get OSPF capabilities to see what supported or not
 * \param cap [OUT] OSPF capabilities
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_capabilities_get(vtss_appl_ospf_capabilities_t *const cap);

//----------------------------------------------------------------------------
//** OSPF instance configuration
//----------------------------------------------------------------------------

/**
 * \brief Add the OSPF instance.
 * \param id [IN] OSPF instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_add(const vtss_appl_ospf_id_t id);

/**
 * \brief Delete the OSPF instance.
 * \param id [IN] OSPF instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_del(const vtss_appl_ospf_id_t id);

/**
 * \brief Get the OSPF instance which the OSPF routing process is enabled.
 * \param id [IN] OSPF instance ID.
 * \return Error code.  VTSS_RC_OK means that OSPF routing process is enabled
 *                      on the instance ID.
 *                      VTSS_RC_ERROR means that the instance ID is not created
 *                      and OSPF routing process is disabled.
 */
mesa_rc vtss_appl_ospf_get(const vtss_appl_ospf_id_t id);

/**
 * \brief Iterate through all OSPF instances.
 * \param current_id [IN]   Pointer to the current instance ID. Use null pointer
 *                          to get the first instance ID.
 * \param next_id    [OUT]  Pointer to the next instance ID
 * \return Error code.      VTSS_RC_OK means that the next instance ID is valid
 *                          and the value is saved in 'out' parameter.
 *                          VTSS_RC_ERROR means that the next instance ID is
 *                          non-existing.
 */
mesa_rc vtss_appl_ospf_inst_itr(
    const vtss_appl_ospf_id_t   *const current_id,
    vtss_appl_ospf_id_t         *const next_id);

/**
 * \brief OSPF control global options.
 */
typedef struct {
    /** Reload OSPF process */
    mesa_bool_t reload_process;
} vtss_appl_ospf_control_globals_t;

/**
 * \brief Set OSPF control of global options.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_control_globals(
    const vtss_appl_ospf_control_globals_t *const control);

//----------------------------------------------------------------------------
//** OSPF router configuration/status
//----------------------------------------------------------------------------
/** \brief The data structure for the OSPF router ID. */
typedef struct {
    /** Indicate the 'ospf_router_id' argument is a specific configured value
     * or not. */
    mesa_bool_t is_specific_id;

    /** The OSPF router ID. The value is used only when 'is_specific_id'
     * argument is true. */
    vtss_appl_ospf_router_id_t id;
} vtss_appl_ospf_router_id_conf_t;

/** \brief The OSPF redistributed metric type. */
typedef enum {
    /** No redistributed metric type is set. */
    VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE,

    /** External link type 1. */
    VTSS_APPL_OSPF_REDIST_METRIC_TYPE_1,

    /** External link type 2. */
    VTSS_APPL_OSPF_REDIST_METRIC_TYPE_2,

    /** The maximum of the redistributed metric type. */
    VTSS_APPL_OSPF_REDIST_METRIC_TYPE_COUNT
} vtss_appl_ospf_redist_metric_type_t;

/** \brief The OSPF redistributed protocol type. */
enum {
    /** The OSPF redistributed protocol type for the connected interfaces. */
    VTSS_APPL_OSPF_REDIST_PROTOCOL_CONNECTED,

    /** The OSPF redistributed protocol type for the static routes. */
    VTSS_APPL_OSPF_REDIST_PROTOCOL_STATIC,

    /** The OSPF redistributed protocol type for the RIP routes. */
    VTSS_APPL_OSPF_REDIST_PROTOCOL_RIP,

    /** The maximum of the OSPF route redistributed protocol type. */
    VTSS_APPL_OSPF_REDIST_PROTOCOL_COUNT
};

/** \brief The data structure for the OSPF router ID. */
typedef struct {
    /** The OSPF redistributed metric type. */
    vtss_appl_ospf_redist_metric_type_t type;

    /** Indicate the 'metric' argument is a specific configured value
      * or not.*/
    mesa_bool_t is_specific_metric;

    /** User specified metric value for the external routes.
      * The field is significant only when argument 'is_specific_metric' is
      * TRUE. */
    vtss_appl_ospf_metric_t metric;
} vtss_appl_ospf_redist_conf_t;

/** \brief The data struct for the OSPF default route configuration. */
typedef struct {
    /** specifies to always advertise a default route into all external-routing
      * capable areas. */
    mesa_bool_t is_always;

    /** Indicate the 'metric' argument is a specific configured value
     * or not.*/
    mesa_bool_t is_specific_metric;

    /** User specified metric value for the default routes.
      * The field is significant only when argument 'is_specific_metric' is
      * TRUE. */
    vtss_appl_ospf_metric_t metric;

    /** The OSPF redistributed metric type. */
    vtss_appl_ospf_redist_metric_type_t type;
} vtss_appl_ospf_default_route_conf_t;

/** \brief The data structure for the OSPF max metric. */
typedef struct {
    /** Enable the max-metric advertisement on startup. */
    mesa_bool_t is_on_startup;

    /** Configure time interval for the max-metric advertisement on startup. */
    uint32_t    on_startup_interval;

    /** Enable the max-metric advertisement on shutdown OSPF. */
    mesa_bool_t is_on_shutdown;

    /** Configure time interval for the max-metric advertisement on shutdown
      * OSPF. */
    uint32_t    on_shutdown_interval;

    /** Configure the max-metric advertisement infinitely. */
    mesa_bool_t is_administrative;
} vtss_appl_ospf_stub_router_conf_t;

/** \brief The data structure for the OSPF router configuration. */
typedef struct {
    /** Configure the OSPF router ID. */
    vtss_appl_ospf_router_id_conf_t router_id;

    /** Configure all interfaces as passive-interface by default. */
    mesa_bool_t default_passive_interface;

    /** Indicate the 'def_metric' argument is a specific configured value
      * or not. */
    mesa_bool_t is_specific_def_metric;

    /** User specified default metric value for the OSPF routing protocol.
      * The field is significant only when the argument 'is_specific_def_metric'
      * is TRUE */
    vtss_appl_ospf_metric_t def_metric;

    /** Configure OSPF route redistribution. */
    vtss_appl_ospf_redist_conf_t redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_COUNT];

    /** Configure OSPF default route */
    vtss_appl_ospf_default_route_conf_t def_route_conf;

    /** Configure max-metric router-LSAs.
      * This is for stub router advertisement RFC 3137. */
    vtss_appl_ospf_stub_router_conf_t stub_router;

    /** Administrative distance value. */
    uint8_t admin_distance;
} vtss_appl_ospf_router_conf_t;

/** \brief The data structure for the OSPF router interface configuration. */
typedef struct {
    /** Enable the interface as OSPF passive-interface. */
    mesa_bool_t passive_enabled;
} vtss_appl_ospf_router_intf_conf_t;

/** \brief The data structure for the OSPF router status. */
typedef struct {
    /** The OSPF router ID. */
    vtss_appl_ospf_router_id_t  ospf_router_id;

    /** Delay time (in seconds)of SPF calculations. */
    uint32_t    spf_delay;

    /** Minimum hold time (in milliseconds) between consecutive SPF calculations. */
    uint32_t    spf_holdtime;

    /** Maximum wait time (in milliseconds) between consecutive SPF calculations. */
    uint32_t    spf_max_waittime;

    /** Time (in milliseconds) that has passed between the start of the SPF
      algorithm execution and the current time. */
    uint64_t    last_executed_spf_ts;

    /** Minimum interval (in seconds) between link-state advertisements. */
    uint32_t    min_lsa_interval;

    /** Maximum arrival time (in milliseconds) of link-state advertisements. */
    uint32_t    min_lsa_arrival;

    /** Number of external link-state advertisements. */
    uint32_t    external_lsa_count;

    /** Number of external link-state checksum. */
    uint32_t    external_lsa_checksum;

    /** Number of areas attached to the router. */
    uint32_t    attached_area_count;

    /** Deferred/stub-router shutdown timer (in milliseconds) */
    uint32_t deferred_shutdown_time;
} vtss_appl_ospf_router_status_t;

/**
 * \brief Get the OSPF router configuration.
 * \param id   [IN] OSPF instance ID.
 * \param conf [OUT] OSPF router configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_conf_get(
    const vtss_appl_ospf_id_t       id,
    vtss_appl_ospf_router_conf_t    *const conf);

/**
 * \brief Set the OSPF router configuration.
 * \param id   [IN] OSPF instance ID.
 * \param conf [IN] OSPF router configuration. Set the metric type to
 *                  VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE will actually
 *                  delete the redistribution.
 * \return Error code.
 * VTSS_APPL_FRR_OSPF_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT means that router
 * ID change doesn't take effect immediately. The new setting will be applied
 * after OSPF process restarting.
 */
mesa_rc vtss_appl_ospf_router_conf_set(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_router_conf_t  *const conf);

/**
 * \brief Get the OSPF router interface configuration.
 * \param id      [IN] OSPF instance ID.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_intf_conf_get(
    const vtss_appl_ospf_id_t           id,
    const vtss_ifindex_t                ifindex,
    vtss_appl_ospf_router_intf_conf_t   *const conf);

/**
 * \brief Set the OSPF router interface configuration.
 * \param id      [IN] OSPF instance ID.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] OSPF router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_intf_conf_set(
    const vtss_appl_ospf_id_t               id,
    const vtss_ifindex_t                    ifindex,
    const vtss_appl_ospf_router_intf_conf_t *const conf);

/**
 * \brief Iterate through all OSPF router interfaces.
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_intf_conf_itr(
    const vtss_appl_ospf_id_t   *const current_id,
    vtss_appl_ospf_id_t         *const next_id,
    const vtss_ifindex_t        *const current_ifindex,
    vtss_ifindex_t              *const next_ifindex);

/**
 * \brief Get the OSPF router configuration.
 * \param id     [IN] OSPF instance ID.
 * \param status [OUT] Status for 'id'.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_status_get(
    const vtss_appl_ospf_id_t       id,
    vtss_appl_ospf_router_status_t  *const status);

//----------------------------------------------------------------------------
//** OSPF network area configuration/status
//----------------------------------------------------------------------------
/** \brief The authentication type. */
typedef enum {
    /** NULL authentication. */
    VTSS_APPL_OSPF_AUTH_TYPE_NULL,

    /** Simple password authentication. */
    VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD,

    /** MD5 digest authentication. */
    VTSS_APPL_OSPF_AUTH_TYPE_MD5,

    /** Area authentication. This type is only used for interface
     * authentication. When the interface is configured to this type.
     * It refers to area authentication configuration.
     * If the area authenticaton is disabled, the behavior is the same as NULL
     * authentication.
     */
    VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG,

    /** The maximum of the authentication type which is used for validation. */
    VTSS_APPL_OSPF_AUTH_TYPE_COUNT
} vtss_appl_ospf_auth_type_t;

/** \brief The authentication type for the OSPF area. */
typedef enum {
    /** Normal area. */
    VTSS_APPL_OSPF_AREA_NORMAL,

    /** Stub area. */
    VTSS_APPL_OSPF_AREA_STUB,

    /** Totally stub area. */
    VTSS_APPL_OSPF_AREA_TOTALLY_STUB,

    /** NSSA **/
    VTSS_APPL_OSPF_AREA_NSSA,

    /** The maximum of the area type. */
    VTSS_APPL_OSPF_AREA_COUNT
} vtss_appl_ospf_area_type_t;

/** \brief The NSSA translator state type */
typedef enum {
    /** NSSA translator state diabled */
    VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_DISABLED,

    /** NSSA translator state elected */
    VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_ELECTED,

    /** NSSA translator state enabled */
    VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_ENABLED
} vtss_appl_ospf_nssa_translator_state_t;

/** \brief The data structure for the OSPF area status. */
typedef struct {
    /** To indicate if it's backbone area or not. */
    mesa_bool_t is_backbone;

    /** To indicate the area type. */
    vtss_appl_ospf_area_type_t area_type;

    /** Number of active interfaces attached in the area. */
    uint32_t attached_intf_active_count;

    /** The authentication status for the area. */
    vtss_appl_ospf_auth_type_t  auth_type;

    /** Number of times SPF algorithm has been executed for the particular area. */
    uint32_t spf_executed_count;

    /** Number of the total LSAs for the particular area. */
    uint32_t lsa_count;

    /** Number of the router-LSAs(Type-1) of a given type for the particular area. */
    uint32_t router_lsa_count;

    /** The the router-LSAs(Type-1) checksum. */
    uint32_t router_lsa_checksum;

    /** Number of the network-LSAs(Type-2) of a given type for the particular area. */
    uint32_t network_lsa_count;

    /** The the network-LSAs(Type-2) checksum. */
    uint32_t network_lsa_checksum;

    /** Number of the summary-LSAs(Type-3) of a given type for the particular area. */
    uint32_t summary_lsa_count;

    /** The the summary-LSAs(Type-3) checksum. */
    uint32_t summary_lsa_checksum;

    /** Number of the ASBR-summary-LSAs(Type-4) of a given type for the particular area. */
    uint32_t asbr_summary_lsa_count;

    /** The the ASBR-summary-LSAs(Type-4) checksum. */
    uint32_t asbr_summary_lsa_checksum;

    /** Number of NSSA-LSAs of a given type for the particular area. **/
    uint32_t nssa_lsa_count;

    /** The the NSSA-LSAs checksum **/
    uint32_t nssa_lsa_checksum;

    /** Nssa translator state **/
    vtss_appl_ospf_nssa_translator_state_t nssa_trans_state;
} vtss_appl_ospf_area_status_t;

/**
 * \brief Get the OSPF area configuration.
 * \param id      [IN]  OSPF instance ID.
 * \param network [IN]  OSPF area network.
 * \param area_id [OUT] OSPF area ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_conf_get(
    const vtss_appl_ospf_id_t   id,
    const mesa_ipv4_network_t   *const network,
    vtss_appl_ospf_area_id_t    *const area_id);

/**
 * \brief Add/set the OSPF area configuration.
 * \param id      [IN] OSPF instance ID.
 * \param network [IN] OSPF area network.
 * \param area_id [IN] OSPF area ID.
 * \return Error code.
 * VTSS_APPL_FRR_OSPF_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT means that area ID
 * change doesn't take effect.
 */
mesa_rc vtss_appl_ospf_area_conf_add(
    const vtss_appl_ospf_id_t       id,
    const mesa_ipv4_network_t       *const network,
    const vtss_appl_ospf_area_id_t  *const area_id);

/**
 * \brief Delete the OSPF area configuration.
 * \param id      [IN] OSPF instance ID.
 * \param network [IN] OSPF area network.
 * \return Error code.
 * VTSS_APPL_FRR_OSPF_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT means that area ID
 * change doesn't take effect.
 */
mesa_rc vtss_appl_ospf_area_conf_del(
    const vtss_appl_ospf_id_t   id,
    const mesa_ipv4_network_t   *const network);

/**
 * \brief Iterate the OSPF areas
 * \param cur_id       [IN]  Current OSPF ID
 * \param next_id      [OUT] Next OSPF ID
 * \param cur_network  [IN]  Current area network
 * \param next_network [OUT] Next area network
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_conf_itr(
    const vtss_appl_ospf_id_t   *const cur_id,
    vtss_appl_ospf_id_t         *const next_id,
    const mesa_ipv4_network_t   *const cur_network,
    mesa_ipv4_network_t         *const next_network);

/**
 * \brief Iterate through the OSPF area status.
 * \param cur_id       [IN]  Current OSPF ID
 * \param next_id      [OUT] Next OSPF ID
 * \param cur_area_id  [IN]  Current area ID
 * \param next_area_id [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_status_itr(
    const vtss_appl_ospf_id_t       *const cur_id,
    vtss_appl_ospf_id_t             *const next_id,
    const vtss_appl_ospf_area_id_t  *const cur_area_id,
    vtss_appl_ospf_area_id_t        *const next_area_id);

/**
 * \brief Get the OSPF area status.
 * \param id        [IN] OSPF instance ID.
 * \param area      [IN] OSPF area key.
 * \param status    [OUT] OSPF area val.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_status_get(
    const vtss_appl_ospf_id_t         id,
    const mesa_ipv4_t                 area,
    vtss_appl_ospf_area_status_t      *const status);

//----------------------------------------------------------------------------
//** OSPF authentication
//----------------------------------------------------------------------------
/**< The precedence of message digest key starting number. */
#define VTSS_APPL_OSPF_MD_KEY_PRECEDENCE_START (1)

/** The formula of converting the plain text length to encrypted data length. */
#define VTSS_APPL_OSPF_AUTH_ENCRYPTED_KEY_LEN(x) \
    ((16 + ((x + 15) / 16) * 16 + 32) * 2)

/** Maximum length of encrypted simple password.  */
#define VTSS_APPL_OSPF_AUTH_ENCRYPTED_SIMPLE_KEY_LEN \
    VTSS_APPL_OSPF_AUTH_ENCRYPTED_KEY_LEN(VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN)

/** Maximum length of encrypted digest password.  */
#define VTSS_APPL_OSPF_AUTH_ENCRYPTED_DIGEST_KEY_LEN \
    VTSS_APPL_OSPF_AUTH_ENCRYPTED_KEY_LEN(VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MAX_LEN)

/** \brief The data structure for the authentication configuration. */
typedef struct {
    /** The authentication type. */
    vtss_appl_ospf_auth_type_t  auth_type;
    /** Set 'true' to indicate the simple password is encrypted by AES256,
     *  otherwise it's unencrypted.
     */
    mesa_bool_t        is_encrypted;
    /** The simple password. */
    char                        auth_key[VTSS_APPL_OSPF_AUTH_ENCRYPTED_SIMPLE_KEY_LEN + 1];
} vtss_appl_ospf_auth_conf_t;


/** \brief The data structure for the digest key configuration. */
typedef struct {
    /** Set 'true' to indicate the key is encrypted by AES256,
     *  otherwise it's unencrypted.
     */
    mesa_bool_t        is_encrypted;
    /** The digest key. */
    char        digest_key[VTSS_APPL_OSPF_AUTH_ENCRYPTED_DIGEST_KEY_LEN + 1];
} vtss_appl_ospf_auth_digest_key_t;

//----------------------------------------------------------------------------
//** OSPF authentication: VLAN interface mode
//----------------------------------------------------------------------------
/**
 * \brief Add the digest key in the specific interface.
 * \param ifindex       [IN] The index of VLAN interface.
 * \param key_id        [IN] The key ID.
 * \param digest_key    [IN] The digest key.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_TOO_LONG means the password
 *  is too long.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID means the key is not a valid
 *  key for AES256.
 */
mesa_rc vtss_appl_ospf_intf_auth_digest_key_add(
    const vtss_ifindex_t                    ifindex,
    const vtss_appl_ospf_md_key_id_t        key_id,
    const vtss_appl_ospf_auth_digest_key_t  *const digest_key);

/**
 * \brief Get the digest key in the specific interface.
 * \param ifindex       [IN]  The index of VLAN interface.
 * \param key_id        [IN]  The key ID.
 * \param digest_key    [OUT] The digest key.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_auth_digest_key_get(
    const vtss_ifindex_t                ifindex,
    const vtss_appl_ospf_md_key_id_t    key_id,
    vtss_appl_ospf_auth_digest_key_t    *const digest_key);

/**
 * \brief Delete a digest key in the specific interface.
 * \param ifindex       [IN]  The index of VLAN interface.
 * \param key_id        [IN] The key ID.
 * \return Error code.
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_intf_auth_digest_key_del(
    const vtss_ifindex_t                ifindex,
    const vtss_appl_ospf_md_key_id_t    key_id);

/**
 * \brief Iterate the digest key.
 * \param current_ifindex [IN]  The current ifIndex.
 * \param next_ifindex    [OUT] The next ifIndex.
 * \param current_key_id  [IN]  The current key ID.
 * \param next_key_id     [OUT] The next key ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_auth_digest_key_itr(
    const vtss_ifindex_t                *const current_ifindex,
    vtss_ifindex_t                      *const next_ifindex,
    const vtss_appl_ospf_md_key_id_t    *const current_key_id,
    vtss_appl_ospf_md_key_id_t          *const next_key_id);

/**
 * \brief Iterate the digest key by the precedence.
 * \param current_ifindex [IN]  The current ifIndex.
 * \param next_ifindex    [OUT] The next ifIndex.
 * \param current_pre_id  [IN]  The precedence ID.
 * \param next_pre_id     [OUT] The next precedence ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_md_key_precedence_itr(
    const vtss_ifindex_t                *const current_ifindex,
    vtss_ifindex_t                      *const next_ifindex,
    const uint32_t                      *const current_pre_id,
    uint32_t                            *const next_pre_id);

/**
 * \brief Get the digest key by the precedence.
 * \param ifindex [IN]  The current ifIndex.
 * \param pre_id  [IN]  The precedence ID.
 * \param key_id  [OUT] The key ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_md_key_precedence_get(
    const vtss_ifindex_t               ifindex,
    const uint32_t                     pre_id,
    vtss_appl_ospf_md_key_id_t         *const key_id);
//----------------------------------------------------------------------------
//** OSPF authentication: router configuration mode
//----------------------------------------------------------------------------
/**
 * \brief Add the authentication configuration in the specific area.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] OSPF area ID.
 * \param auth_type [IN] The authentication type.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_add(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_auth_type_t    auth_type);

/**
 * \brief Set the authentication configuration in the specific area.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] OSPF area ID.
 * \param auth_type [IN] The authentication type.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_set(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_auth_type_t    auth_type);

/**
 * \brief Get the authentication configuration in the specific area.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  OSPF area ID.
 * \param auth_type [OUT] The authentication type.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_get(
    const vtss_appl_ospf_id_t       id,
    const vtss_appl_ospf_area_id_t  area_id,
    vtss_appl_ospf_auth_type_t      *const auth_type);

/**
 * \brief Delete the authentication configuration in the specific area.
 * \param id      [IN]  OSPF instance ID.
 * \param area_id [IN]  OSPF area ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_del(
    const vtss_appl_ospf_id_t       id,
    const vtss_appl_ospf_area_id_t  area_id);

/**
 * \brief Iterate the specific area with authentication configuration.
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_area_id [IN]  Current area ID
 * \param next_area_id    [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_itr(
    const vtss_appl_ospf_id_t       *const current_id,
    vtss_appl_ospf_id_t             *const next_id,
    const vtss_appl_ospf_area_id_t  *const current_area_id,
    vtss_appl_ospf_area_id_t        *const next_area_id);

//----------------------------------------------------------------------------
//** OSPF area range
//----------------------------------------------------------------------------
/** \brief The data structure for the OSPF area range configuration. */
typedef struct {
    /** When the value is 'true', the ABR can summarize intra area paths from
     *  the address range in one summary-LSA(Type-3) advertised to other areas.
     *  When the value is 'false', the ABR does not advertised the summary-LSA
     *  (Type-3) for the address range. */
    mesa_bool_t is_advertised;

    /** Indicate the 'cost' argument is a specific configured value
      * or not. */
    mesa_bool_t is_specific_cost;

    /** User specified cost (or metric) for this summary route. */
    vtss_appl_ospf_cost_t cost;
} vtss_appl_ospf_area_range_conf_t;

/**
 * \brief Get the OSPF area range configuration.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] OSPF area ID.
 * \param network [IN] OSPF area range network.
 * \param conf    [IN] OSPF area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_get(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const mesa_ipv4_network_t           network,
    vtss_appl_ospf_area_range_conf_t    *const conf);

/**
 * \brief Set the OSPF area range configuration.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] OSPF area ID.
 * \param network [IN] OSPF area range network.
 * \param conf    [IN] OSPF area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_set(
    const vtss_appl_ospf_id_t               id,
    const vtss_appl_ospf_area_id_t          area_id,
    const mesa_ipv4_network_t               network,
    const vtss_appl_ospf_area_range_conf_t  *const conf);

/**
 * \brief Add the OSPF area range configuration.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] OSPF area ID.
 * \param network [IN] OSPF area range network.
 * \param conf    [IN] OSPF area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_add(
    const vtss_appl_ospf_id_t               id,
    const vtss_appl_ospf_area_id_t          area_id,
    const mesa_ipv4_network_t               network,
    const vtss_appl_ospf_area_range_conf_t  *const conf);

/**
 * \brief Delete the OSPF area range configuration.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] OSPF area ID.
 * \param network [IN] OSPF area range network.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_del(
    const vtss_appl_ospf_id_t       id,
    const vtss_appl_ospf_area_id_t  area_id,
    const mesa_ipv4_network_t       network);

/**
 * \brief Iterate the OSPF area ranges
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_area_id [IN]  Current area ID
 * \param next_area_id    [OUT] Next area ID
 * \param current_network [IN]  Current network address
 * \param next_network    [OUT] Next network address
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_itr(
    const vtss_appl_ospf_id_t       *const current_id,
    vtss_appl_ospf_id_t             *const next_id,
    const vtss_appl_ospf_area_id_t  *const current_area_id,
    vtss_appl_ospf_area_id_t        *const next_area_id,
    const mesa_ipv4_network_t       *const current_network,
    mesa_ipv4_network_t             *const next_network);

//----------------------------------------------------------------------------
//** OSPF virtual link
//----------------------------------------------------------------------------
/** \brief The data structure of OSPF virtual link configuration. */
typedef struct {
    /** The time interval (in seconds) between hello packets. */
    uint32_t    hello_interval;

    /** The time interval (in seconds) between hello packets. */
    uint32_t    dead_interval;

    /** The time interval (in seconds) between link-state advertisement
     *  (LSA) retransmissions for adjacencies. */
    uint32_t    retransmit_interval;

    /** The authentication type. */
    vtss_appl_ospf_auth_type_t  auth_type;

    /** Set 'true' to indicate the simple password is encrypted by AES256,
     *  otherwise it's unencrypted. */
    mesa_bool_t is_encrypted;

    /** The simple password. */
    char        simple_pwd[VTSS_APPL_OSPF_AUTH_ENCRYPTED_SIMPLE_KEY_LEN + 1];
} vtss_appl_ospf_vlink_conf_t;

/**
 * \brief Add a virtual link in the specific area.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] The area ID of the configuration.
 * \param router_id [IN] The destination router id of virtual link.
 * \param conf      [IN] The virtual link configuration for adding.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_vlink_conf_add(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_router_id_t    router_id,
    const vtss_appl_ospf_vlink_conf_t   *const conf);

/**
 * \brief Set the configuration for a virtual link.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] The area ID of the configuration.
 * \param router_id [IN] The destination router id of virtual link.
 * \param conf      [IN] The virtual link configuration for setting.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_vlink_conf_set(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_router_id_t    router_id,
    const vtss_appl_ospf_vlink_conf_t   *const conf);

/**
 * \brief Get the configuration for a virtual link.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  The area ID of the configuration.
 * \param router_id [IN]  The destination router id of virtual link.
 * \param conf      [OUT] The virtual link configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_conf_get(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_router_id_t    router_id,
    vtss_appl_ospf_vlink_conf_t         *const conf);

/**
 * \brief Delete a specific virtual link.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] The area ID of the configuration.
 * \param router_id [IN] The destination router id of virtual link.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_conf_del(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_router_id_t    router_id);

/**
 * \brief Iterate through all OSPF virtual links.
 * \param current_id        [IN]  Current OSPF ID
 * \param next_id           [OUT] Next OSPF ID
 * \param current_area_id   [IN]  Current Area
 * \param next_area_id      [OUT] Next Area
 * \param current_router_id [IN]  Current Destination
 * \param next_router_id    [OUT] Next Destination
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_itr(
    const vtss_appl_ospf_id_t           *const current_id,
    vtss_appl_ospf_id_t                 *const next_id,
    const vtss_appl_ospf_area_id_t      *const current_area_id,
    vtss_appl_ospf_area_id_t            *const next_area_id,
    const vtss_appl_ospf_router_id_t    *const current_router_id,
    vtss_appl_ospf_router_id_t          *const next_router_id);

//----------------------------------------------------------------------------
//** OSPF virtual link authentication: message digest key
//----------------------------------------------------------------------------
/**
 * \brief Get the message digest key for the specific virtual link.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  OSPF area ID.
 * \param router_id [IN]  OSPF router ID.
 * \param key_id    [IN]  The message digest key ID.
 * \param md_key    [OUT] The message digest key configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_md_key_conf_get(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_router_id_t    router_id,
    const vtss_appl_ospf_md_key_id_t    key_id,
    vtss_appl_ospf_auth_digest_key_t    *const md_key);

/**
 * \brief Add the message digest key for the specific virtual link.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] OSPF area ID.
 * \param router_id [IN] OSPF router ID.
 * \param key_id    [IN] The message digest key ID.
 * \param md_key    [IN] The message digest key configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_TOO_LONG means the password
 *  is too long.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID means the key is not a valid
 *  key for AES256.
 */
mesa_rc vtss_appl_ospf_vlink_md_key_conf_add(
    const vtss_appl_ospf_id_t               id,
    const vtss_appl_ospf_area_id_t          area_id,
    const vtss_appl_ospf_router_id_t        router_id,
    const vtss_appl_ospf_md_key_id_t        key_id,
    const vtss_appl_ospf_auth_digest_key_t  *const md_key);

/**
 * \brief Delete a message digest key for the specific virtual link.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] OSPF area ID.
 * \param router_id [IN] OSPF router ID.
 * \param key_id    [IN] The message digest key ID.
 * \return Error code.
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_vlink_md_key_conf_del(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_router_id_t    router_id,
    const vtss_appl_ospf_md_key_id_t    key_id);

/**
 * \brief Iterate the message digest key for the specific virtual link.
 * \param current_id        [IN]  Current OSPF ID
 * \param next_id           [OUT] Next OSPF ID
 * \param current_area_id   [IN]  Current area ID
 * \param next_area_id      [OUT] Next area ID
 * \param current_router_id [IN]  Current router ID
 * \param next_router_id    [OUT] Next router ID
 * \param current_key_id    [IN]  The current message digest key ID.
 * \param next_key_id       [OUT] The next message digest key ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_md_key_itr(
    const vtss_appl_ospf_id_t           *const current_id,
    vtss_appl_ospf_id_t                 *const next_id,
    const vtss_appl_ospf_area_id_t      *const current_area_id,
    vtss_appl_ospf_area_id_t            *const next_area_id,
    const mesa_ipv4_t                   *const current_router_id,
    mesa_ipv4_t                         *const next_router_id,
    const vtss_appl_ospf_md_key_id_t    *const current_key_id,
    vtss_appl_ospf_md_key_id_t          *const next_key_id);

/**
 * \brief Get the message digest key precedence for the specific virtual link.
 * \param id         [IN]  OSPF instance ID.
 * \param area_id    [IN]  OSPF area ID.
 * \param router_id  [IN]  OSPF router ID.
 * \param precedence [IN]  The message digest key precedence.
 * \param key_id     [OUT] The message digest key ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_md_key_precedence_get(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_area_id_t      area_id,
    const vtss_appl_ospf_router_id_t    router_id,
    const uint32_t                      precedence,
    vtss_appl_ospf_md_key_id_t          *const key_id);

/**
 * \brief Iterate the message digest key precedence for the specific virtual link.
 * \param current_id         [IN]  Current OSPF ID
 * \param next_id            [OUT] Next OSPF ID
 * \param current_area_id    [IN]  Current area ID
 * \param next_area_id       [OUT] Next area ID
 * \param current_router_id  [IN]  Current router ID
 * \param next_router_id     [OUT] Next router ID
 * \param current_precedence [IN]  The current message digest key precedence.
 * \param next_precedence    [OUT] The next message digest key precedence.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_md_key_precedence_itr(
    const vtss_appl_ospf_id_t           *const current_id,
    vtss_appl_ospf_id_t                 *const next_id,
    const vtss_appl_ospf_area_id_t      *const current_area_id,
    vtss_appl_ospf_area_id_t            *const next_area_id,
    const vtss_appl_ospf_router_id_t    *const current_router_id,
    vtss_appl_ospf_router_id_t          *const next_router_id,
    const uint32_t                      *const current_precedence,
    uint32_t                            *const next_precedence);

//----------------------------------------------------------------------------
//** OSPF stub area
//----------------------------------------------------------------------------

/** \brief The OSPF stub area configured type. */
typedef enum {
    /** Stub area. */
    VTSS_APPL_OSPF_STUB_CFG_TYPE_NORMAL = VTSS_APPL_OSPF_AREA_STUB,

    /** NSSA */
    VTSS_APPL_OSPF_STUB_CFG_TYPE_NSSA = VTSS_APPL_OSPF_AREA_NSSA,

    /** The maximum of the stub configured type. */
    VTSS_APPL_OSPF_STUB_CFG_TYPE_NSSA_COUNT
} vtss_appl_ospf_stub_cfg_type_t;

/** \brief The OSPF NSSA translator role. */
typedef enum {
    /** NSSA-ABR translate election. */
    VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_CANDIDATE,

    /** NSSA-ABR nerver translate. */
    VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_NEVER,

    /** NSSA-ABR always translate. */
    VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_ALWAYS,

    /** The maximum of the NSSA role type. */
    VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_COUNT
} vtss_appl_ospf_nssa_translator_role_t;

/** \brief The data structure of stub area configuration. */
typedef struct {
    /** The OSPF stub configured type. */
    mesa_bool_t is_nssa;

    /** Set 'true' to configure the inter-area routes do not inject into this
     *  stub area.
     */
    mesa_bool_t no_summary;

    /** NSSA translator role. */
    vtss_appl_ospf_nssa_translator_role_t nssa_translator_role;
} vtss_appl_ospf_stub_area_conf_t;

/**
 * \brief Add the specific area in the stub areas.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \param conf    [IN] The stub area configuration for adding.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_stub_area_conf_add(
    const vtss_appl_ospf_id_t               id,
    const vtss_appl_ospf_area_id_t          area_id,
    const vtss_appl_ospf_stub_area_conf_t   *const conf);

/**
 * \brief Set the configuration for a specific stub area.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \param conf    [IN] The stub area configuration for setting.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_stub_area_conf_set(
    const vtss_appl_ospf_id_t               id,
    const vtss_appl_ospf_area_id_t          area_id,
    const vtss_appl_ospf_stub_area_conf_t   *const conf);

/**
 * \brief Get the configuration for a specific stub area.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN]  The area ID of the stub area configuration.
 * \param conf    [OUT] The stub area configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_stub_area_conf_get(
    const vtss_appl_ospf_id_t               id,
    const vtss_appl_ospf_area_id_t          area_id,
    vtss_appl_ospf_stub_area_conf_t         *const conf);

/**
 * \brief Delete a specific stub area.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_stub_area_conf_del(
    const vtss_appl_ospf_id_t               id,
    const vtss_appl_ospf_area_id_t          area_id);

/**
 * \brief Iterate the stub areas.
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_area_id [IN]  Current area ID
 * \param next_area_id    [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_stub_area_conf_itr(
    const vtss_appl_ospf_id_t       *const current_id,
    vtss_appl_ospf_id_t             *const next_id,
    const vtss_appl_ospf_area_id_t  *const current_area_id,
    vtss_appl_ospf_area_id_t        *const next_area_id);

//----------------------------------------------------------------------------
//** OSPF interface parameter tuning
//----------------------------------------------------------------------------

/** \brief The data structure for the OSPF VLAN interface configuration. */
typedef struct {
    /** User specified router priority for the interface. */
    vtss_appl_ospf_priority_t priority;

    /** Link state metric for the interface. It used for Shortest Path First
     * (SPF) calculation. */
    /** Indicate the 'cost' argument is a specific configured value or not. */
    mesa_bool_t is_specific_cost;
    /** User specified cost for the interface. */
    vtss_appl_ospf_cost_t cost;

    /**
     * When cleared (default), the "Interface MTU" of received OSPF Database
     * Description (DBD) packets will be checked against our own MTU, and if the
     * remote MTU is greater than our own, the DBD will be ignored.
     *
     * When set, the "Interface MTU" of received OSPF DBD packets will be
     * ignored.
     */
    mesa_bool_t mtu_ignore;

    /** Enable the feature of fast hello packets or not. */
    mesa_bool_t is_fast_hello_enabled;

    /** It specifies how many Hello packets will be sent per second.
     *  It is significant only when the paramter 'is_fast_hello_enabled' is
     *  true */
    uint32_t fast_hello_packets;

    /** The time interval (in seconds) between hello packets.
     *  The value is set to 1 when the paramter 'is_fast_hello_enabled' is
     *  true */
    uint32_t dead_interval;

    /** The time interval (in seconds) between hello packets. */
    uint32_t hello_interval;

    /** The time interval (in seconds) between between link-state advertisement
     *  (LSA) retransmissions for adjacencies. */
    uint32_t retransmit_interval;

    /** The interface authentication type. */
    vtss_appl_ospf_auth_type_t  auth_type;

    /** Set 'true' to indicate the simple password is encrypted,
     *  otherwise it's unencrypted.
     */
    mesa_bool_t        is_encrypted;

    /** The simple password for interface authentication. Set null string to
     * delete the key.
     */
    char               auth_key[VTSS_APPL_OSPF_AUTH_ENCRYPTED_SIMPLE_KEY_LEN + 1];
} vtss_appl_ospf_intf_conf_t;

/**
 * \brief Get the OSPF VLAN interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_conf_get(
    const vtss_ifindex_t        ifindex,
    vtss_appl_ospf_intf_conf_t  *const conf);

/**
 * \brief Set the OSPF VLAN interface configuration.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] OSPF VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_conf_set(
    const vtss_ifindex_t                ifindex,
    const vtss_appl_ospf_intf_conf_t    *const conf);

/**
 * \brief Iterate through all OSPF VLAN interfaces.
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_conf_itr(
    const vtss_ifindex_t    *const current_ifindex,
    vtss_ifindex_t          *const next_ifindex);

//----------------------------------------------------------------------------
//** OSPF interface status
//----------------------------------------------------------------------------

/** \brief The link state of the OSPF interface. */
typedef enum {
    /** Down state */
    VTSS_APPL_OSPF_INTERFACE_DOWN = 1,

    /** lookback interface */
    VTSS_APPL_OSPF_INTERFACE_LOOPBACK = 2,

    /** Waiting state */
    VTSS_APPL_OSPF_INTERFACE_WAITING = 3,

    /** Point-To-Point interface */
    VTSS_APPL_OSPF_INTERFACE_POINT2POINT = 4,

    /** Select as DR other router */
    VTSS_APPL_OSPF_INTERFACE_DR_OTHER = 5,

    /** Select as BDR router */
    VTSS_APPL_OSPF_INTERFACE_BDR = 6,

    /** Select as DR router */
    VTSS_APPL_OSPF_INTERFACE_DR = 7,

    /** Unknown state */
    VTSS_APPL_OSPF_INTERFACE_UNKNOWN

} vtss_appl_ospf_interface_state_t;

/** \brief The data structure for the OSPF interface status.
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
    mesa_ipv4_network_t                 network;

    /** Area ID */
    vtss_appl_ospf_area_id_t            area_id;

    /** The OSPF router ID. */
    vtss_appl_ospf_router_id_t          router_id;

    /** The cost of the interface. */
    vtss_appl_ospf_cost_t               cost;

    /** define the state of the link */
    vtss_appl_ospf_interface_state_t    state;

    /** The OSPF priority */
    uint8_t                             priority;

    /** The router ID of DR. */
    mesa_ipv4_t                         dr_id;

    /** The IP address of DR. */
    mesa_ipv4_t                         dr_addr;

    /** The router ID of BDR. */
    mesa_ipv4_t                         bdr_id;

    /** The IP address of BDR. */
    mesa_ipv4_t                         bdr_addr;

    /** The IP address of Virtual link peer. */
    mesa_ipv4_t                         vlink_peer_addr;

    /** Hello timer, the unit of time is the second. */
    uint32_t                            hello_time;

    /** Dead timer, the unit of time is the second. */
    uint32_t                            dead_time;

    /** Retransmit timer, the unit of time is the second. */
    uint32_t                            retransmit_time;

    /** Hello due timer, the unit of time is the second. */
    uint32_t                            hello_due_time;

    /** Neighbor count. */
    uint32_t                            neighbor_count;

    /** Adjacent neighbor count */
    uint32_t                            adj_neighbor_count;

    /** Transmit Delay */
    uint32_t                            transmit_delay;
} vtss_appl_ospf_interface_status_t;

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
mesa_rc vtss_appl_ospf_interface_itr(
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
mesa_rc vtss_appl_ospf_interface_itr2(
    const mesa_ipv4_t *const current_addr,
    mesa_ipv4_t *const next_addr,
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx);


/**
 * \brief Get status for a specific interface.
 * \param ifindex   [IN] Ifindex to query.
 * \param status    [OUT] Status for 'key'.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_interface_status_get(
    const vtss_ifindex_t                   ifindex,
    vtss_appl_ospf_interface_status_t      *const status);

/**
 * \brief Get status for all route.
 * \param interface [IN,OUT] An empty container which gets populated
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_interface_status_get_all(
    vtss::Map<vtss_ifindex_t,
    vtss_appl_ospf_interface_status_t> &interface);

//----------------------------------------------------------------------------
//** OSPF neighbor status
//----------------------------------------------------------------------------
/**
 * \brief The OSPF options field is present in OSPF Hello packets, which
 *        enables OSPF routers to support (or not support) optional capabilities,
 *        and to communicate their capability level to other OSPF routers.
 *
 *        RFC5250 provides a description of each capability. See
 *        https://tools.ietf.org/html/rfc5250#appendix-A for a detail
 *        information.
 */
enum {
    VTSS_APPL_OSPF_OPTION_FIELD_MT       = (1 << 0), /**< Control MT-bit  */
    VTSS_APPL_OSPF_OPTION_FIELD_E        = (1 << 1), /**< Control E-bit  */
    VTSS_APPL_OSPF_OPTION_FIELD_MC       = (1 << 2), /**< Control MC-bit */
    VTSS_APPL_OSPF_OPTION_FIELD_NP       = (1 << 3), /**< Control NP-bit */
    VTSS_APPL_OSPF_OPTION_FIELD_EA       = (1 << 4), /**< Control EA-bit */
    VTSS_APPL_OSPF_OPTION_FIELD_DC       = (1 << 5), /**< Control DC-bit */
    VTSS_APPL_OSPF_OPTION_FIELD_O        = (1 << 6), /**< Control O-bit */
    VTSS_APPL_OSPF_OPTION_FIELD_DN       = (1 << 7)  /**< Control DN-bit */
};

/** \brief The neighbor state of the OSPF. */
typedef enum {
    VTSS_APPL_OSPF_NEIGHBOR_STATE_DEPENDUPON = 0,   /**< Depend Upon state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_DELETED = 1,      /**< Deleted state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_DOWN = 2,         /**< Down state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_ATTEMPT = 3,      /**< Attempt state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_INIT = 4,         /**< Init state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_2WAY = 5,         /**< 2-Way state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_EXSTART = 6,      /**< ExStart state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_EXCHANGE = 7,     /**< Exchange state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_LOADING = 8,      /**< Loading state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_FULL = 9,         /**< Full state */
    VTSS_APPL_OSPF_NEIGHBOR_STATE_UNKNOWN           /**< Unknown state */
} vtss_appl_ospf_neighbor_state_t;

/** \brief The data structure for the OSPF neighbor information. */
typedef struct {
    /** The IP address. */
    mesa_ipv4_t                         ip_addr;
    /** Neighbor's router ID. */
    vtss_appl_ospf_router_id_t          neighbor_id;
    /** Area ID */
    vtss_appl_ospf_area_id_t            area_id;
    /** Interface index */
    vtss_ifindex_t                      ifindex;
    /** The OSPF neighbor priority */
    uint8_t                             priority;
    /** Neighbor state */
    vtss_appl_ospf_neighbor_state_t     state;
    /** The router ID of DR. */
    mesa_ipv4_t                         dr_id;
    /** The IP address of DR. */
    mesa_ipv4_t                         dr_addr;
    /** The router ID of BDR. */
    mesa_ipv4_t                         bdr_id;
    /** The IP address of BDR. */
    mesa_ipv4_t                         bdr_addr;
    /** The option field which is present in OSPF hello packets */
    uint8_t                             options;
    /** Dead timer. */
    uint32_t                            dead_time;
    /** Transit Area ID */
    vtss_appl_ospf_area_id_t            transit_id;
} vtss_appl_ospf_neighbor_status_t;

/** \brief The data structure combining the OSPF neighbor's keys and status for
  * get_all container. */
typedef struct {
    /** OSPF instance ID. */
    vtss_appl_ospf_id_t              id;
    /** Neighbor id to query. */
    vtss_appl_ospf_router_id_t       neighbor_id;
    /** Neighbor IP to query. */
    mesa_ipv4_t                      neighbor_ip;
    /** Neighbor ifindex to query. */
    vtss_ifindex_t                   neighbor_ifidx;
    /** The OSPF neighbor information. */
    vtss_appl_ospf_neighbor_status_t status;
} vtss_appl_ospf_neighbor_data_t;

/**
 * \brief Get neighbor ID by neighbor IP address.
 * \param ip_addr   [IN] Neighbor IP address to query.
 * \return the Neighbor router ID or zero address if not found.
 */
vtss_appl_ospf_router_id_t vtss_appl_ospf_nbr_lookup_id_by_addr(const mesa_ipv4_t ip_addr);


/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by standard MIB
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_nip     [IN]  Pointer to current neighbor IP.
 * \param next_nip        [OUT] Next neighbor IP.
 * \param current_ifidx   [IN]  Pointer to current neighbor ifindex.
 * \param next_ifidx      [OUT] Next neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_neighbor_status_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t       *const next_id,
    const mesa_ipv4_t         *const current_nip,
    mesa_ipv4_t               *const next_nip,
    const vtss_ifindex_t      *const current_ifidx,
    vtss_ifindex_t            *const next_ifidx);

/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by CLI and JSON/Private MIB
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_nid     [IN]  Pointer to current neighbor's router id.
 * \param next_nid        [OUT] Next neighbor's router id.
 * \param current_nip     [IN]  Pointer to current neighbor IP.
 * \param next_nip        [OUT] Next to next neighbor ip.
 * \param current_ifidx   [IN]  Pointer to current neighbor ifindex.
 * \param next_ifidx      [OUT] Next to next neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_neighbor_status_itr2(
    const vtss_appl_ospf_id_t        *const current_id,
    vtss_appl_ospf_id_t              *const next_id,
    const vtss_appl_ospf_router_id_t *const current_nid,
    vtss_appl_ospf_router_id_t       *const next_nid,
    const mesa_ipv4_t                *const current_nip,
    mesa_ipv4_t                      *const next_nip,
    const vtss_ifindex_t             *const current_ifidx,
    vtss_ifindex_t                   *const next_ifidx);

/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by Standard MIB - ospfVirtNbrTable (key: Transit Area and Router ID)
 * \param current_id                [IN]  Current OSPF ID.
 * \param next_id                   [OUT] Next OSPF ID.
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
mesa_rc vtss_appl_ospf_neighbor_status_itr3(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_area_id_t *const current_transit_area_id,
    vtss_appl_ospf_area_id_t *const next_transit_area_id,
    const vtss_appl_ospf_router_id_t *const current_nid,
    vtss_appl_ospf_router_id_t *const next_nid,
    const mesa_ipv4_t *const current_nip, mesa_ipv4_t *const next_nip,
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx);

/** The search doesn't match neighbor's router ID.  */
#define VTSS_APPL_OSPF_DONTCARE_NID  (0)

/**
 * \brief Get status for a neighbor information.
 * \param id             [IN]  OSPF instance ID.
 * \param neighbor_id    [IN]  Neighbor's router ID.
 *  If the neighbor_id is VTSS_APPL_OSPF_DONTCARE_NID,
 *  it doesn't match the neighbor ID
 * \param neighbor_ip    [IN]  Neighbor's IP.
 * \param neighbor_ifidx [IN]  Neighbor's ifindex.
 * \param status         [OUT] Neighbor Status.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_neighbor_status_get(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_router_id_t    neighbor_id,
    const mesa_ipv4_t                   neighbor_ip,
    const vtss_ifindex_t                neighbor_ifidx,
    vtss_appl_ospf_neighbor_status_t    *const status);

/**
 * \brief Get status for all neighbor information.
 * \param neighbors [OUT] An container with all neighbor.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_neighbor_status_get_all(
    vtss::Vector<vtss_appl_ospf_neighbor_data_t>
    &neighbors);

/** \brief The route type of the OSPF route entry. */
typedef enum {
    /** The destination is an OSPF route which is located on intra-area. */
    VTSS_APPL_OSPF_ROUTE_TYPE_INTRA_AREA,

    /** The destination is an OSPF route which is located on inter-area. */
    VTSS_APPL_OSPF_ROUTE_TYPE_INTER_AREA,

    /** The destination is a border router. */
    VTSS_APPL_OSPF_ROUTE_TYPE_BORDER_ROUTER,

    /** The destination is an external Type-1 route. */
    VTSS_APPL_OSPF_ROUTE_TYPE_EXTERNAL_TYPE_1,

    /** The destination is an external Type-2 route. */
    VTSS_APPL_OSPF_ROUTE_TYPE_EXTERNAL_TYPE_2,

    /** The route type isn't supported. */
    VTSS_APPL_OSPF_ROUTE_TYPE_UNKNOWN,
} vtss_appl_ospf_route_type_t;

/** \brief The border router type of the OSPF route entry. */
typedef enum {
    /** The border router is an ABR. */
    VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_ABR,

    /** The border router is an ASBR located on Intra-area. */
    VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_INTRA_AREA_ASBR,

    /** The border router is an ASBR located on Inter-area. */
    VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_INTER_AREA_ASBR,

    /** The border router is an ASBR attached to at least 2 areas. */
    VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_ABR_ASBR,

    /** The entry isn't a border router. */
    VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_NONE,
} vtss_appl_ospf_route_br_type_t;

/** \brief The data structure for the OSPF route entry. */
typedef struct {
    /** The cost of the route or router path. */
    vtss_appl_ospf_cost_t                   cost;

    /** The cost of the route within the OSPF network. It is valid for external
      * Type-2 route and always '0' for other route type.
     */
    vtss_appl_ospf_cost_t                   as_cost;

    /** The area which the route or router can be reached via/to. */
    vtss_appl_ospf_area_id_t                area;

    /** The border router type. */
    vtss_appl_ospf_route_br_type_t          border_router_type;

    /** The destination is connected directly or not. */
    mesa_bool_t                             connected;

    /** The interface where the ip packet is outgoing. */
    vtss_ifindex_t                          ifindex;
} vtss_appl_ospf_route_status_t;

/** \brief The data structure for getting all OSPF IPv4 route entries. */
typedef struct {
    /** The OSPF ID. */
    vtss_appl_ospf_id_t             id;

    /** The route type. */
    vtss_appl_ospf_route_type_t     rt_type;

    /** The destination. */
    mesa_ipv4_network_t             dest;

    /** The nexthop. */
    mesa_ipv4_t                     nexthop;

    /** The area which the route or router can be reached via/to. */
    vtss_appl_ospf_area_id_t        area;

    /** The OSPF routing information. */
    vtss_appl_ospf_route_status_t   status;
} vtss_appl_ospf_route_ipv4_data_t;

/**
 * \brief Iterate through the OSPF IPv4 route entries.
 * \param current_id        [IN]  The current OSPF ID.
 * \param next_id           [OUT] The next OSPF ID.
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
mesa_rc vtss_appl_ospf_route_ipv4_status_itr(
    const vtss_appl_ospf_id_t          *const current_id,
    vtss_appl_ospf_id_t                *const next_id,
    const vtss_appl_ospf_route_type_t  *const current_rt_type,
    vtss_appl_ospf_route_type_t        *const next_rt_type,
    const mesa_ipv4_network_t          *const current_dest,
    mesa_ipv4_network_t                *const next_dest,
    const vtss_appl_ospf_area_id_t     *const current_area,
    vtss_appl_ospf_area_id_t           *const next_area,
    const mesa_ipv4_t                  *const current_nexthop,
    mesa_ipv4_t                        *const next_nexthop);

/**
 * \brief Get the specific OSPF IPv4 route entry.
 * \param id      [IN]  The OSPF instance ID.
 * \param rt_type [IN]  The route type.
 * \param dest    [IN]  The destination.
 * \param area    [IN]  The area.
 * \param nexthop [IN]  The nexthop.
 * \param status  [OUT] The OSPF route status.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_route_ipv4_status_get(
    const vtss_appl_ospf_id_t           id,
    const vtss_appl_ospf_route_type_t   rt_type,
    const mesa_ipv4_network_t           dest,
    const vtss_appl_ospf_area_id_t      area,
    const mesa_ipv4_t                   nexthop,
    vtss_appl_ospf_route_status_t       *const status);

/**
 * \brief Get all OSPF IPv4 route entries.
 * \param routes [OUT] The container with all routes.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_route_ipv4_status_get_all(
    vtss::Vector<vtss_appl_ospf_route_ipv4_data_t>
    &routes);

//----------------------------------------------------------------------------
//** OSPF database information
//----------------------------------------------------------------------------
/** \brief The type of the link state advertisement. */
typedef enum {
    /** No LS database type is specified. */
    VTSS_APPL_OSPF_LSDB_TYPE_NONE         = 0,

    /** Router link states. */
    VTSS_APPL_OSPF_LSDB_TYPE_ROUTER       = 1,

    /** Network link state. */
    VTSS_APPL_OSPF_LSDB_TYPE_NETWORK      = 2,

    /** Network summary link state. */
    VTSS_APPL_OSPF_LSDB_TYPE_SUMMARY      = 3,

    /** ASBR summary link state. */
    VTSS_APPL_OSPF_LSDB_TYPE_ASBR_SUMMARY = 4,

    /** External link state. */
    VTSS_APPL_OSPF_LSDB_TYPE_EXTERNAL     = 5,

    /** NSSA external link state. */
    VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL = 7,

    /** The route type isn't supported. */
    VTSS_APPL_OSPF_LSDB_TYPE_UNKNOWN = 8,

} vtss_appl_ospf_lsdb_type_t;

/**\brief The OSPF database external type. */
typedef enum {
    /** External link type is unused. */
    VTSS_APPL_OSPF_DB_EXTERNAL_TYPE_NONE,

    /** External link type 1. */
    VTSS_APPL_OSPF_DB_EXTERNAL_TYPE_1,

    /** External link type 2. */
    VTSS_APPL_OSPF_DB_EXTERNAL_TYPE_2
} vtss_appl_ospf_db_external_type_t;

/** \brief The OSPF link state database general summary information. */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** The link count of router link state.
     *  (The parameter is used for Type 1 only) */
    uint32_t router_link_count;
} vtss_appl_ospf_db_general_info_t;

/** \brief The OSPF link state database entry (For get_all API). */
typedef struct {
    /** The OSPF instance ID. */
    vtss_appl_ospf_id_t inst_id;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf_area_id_t area_id;

    /**  The type of the link state advertisement. */
    vtss_appl_ospf_lsdb_type_t lsdb_type;

    /** The OSPF link state ID. It identifies the piece of the routing domain
     * that is being described by the LSA.
     * The filed value has various meanings based on the LS type.
     *  LS Type     LS Description      Link State ID
     *  -------     --------------      -------------
     *  1           Router-LSAs         The originating router's Router ID.
     *  2           Network-LSAs        The IP interface address of the network's Designated Router.
     *  3           Summary-LSAs        The destination network's IP address.
     *  4           ASBR Summary-LSAs   The Router ID of the described AS boundary router.
     *  5           AS-external-LSAs    The destination network's IP address.
     *  7           NSSA-external-LSAs  The destination network's IP address.
     */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf_router_id_t adv_router_id;

    /** The database general summary informaton. */
    vtss_appl_ospf_db_general_info_t db;
} vtss_appl_ospf_db_entry_t;

/**
 * \brief Iterate through the OSPF database information.
 * \param cur_inst_id        [IN]  The current OSPF ID.
 * \param next_inst_id       [OUT] The next OSPF ID.
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
mesa_rc vtss_appl_ospf_db_itr(
    const vtss_appl_ospf_id_t        *const cur_inst_id,
    vtss_appl_ospf_id_t              *const next_inst_id,
    const vtss_appl_ospf_area_id_t   *const cur_area_id,
    vtss_appl_ospf_area_id_t         *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t       *const next_router_id);

/**
 * \brief Get the specific OSPF database general summary information.
 * \param inst_id         [IN]  The OSPF instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_general_info [OUT] The OSPF database general summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_db_get(
    const vtss_appl_ospf_id_t        inst_id,
    const vtss_appl_ospf_area_id_t   area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_general_info_t *const db_general_info);

/**
 * \brief Get all OSPF database entries of geranal information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_get_all(
    vtss::Vector<vtss_appl_ospf_db_entry_t>
    &db_entries);

//----------------------------------------------------------------------------
//** OSPF database detail common information
//----------------------------------------------------------------------------

/** \brief The OSPF link state database detailed common information. */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF hello packets */
    uint8_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

} vtss_appl_ospf_db_detail_common_info_t;


/** \brief The OSPF link state database type 1 link information. */
typedef struct {

    /** link connected type */
    uint32_t link_connected_to;

    /** Link ID address */
    mesa_ipv4_t link_id;

    /** Link Network Mask */
    mesa_ipv4_t link_data;

    /** User specified metric for this summary route. */
    uint32_t metric;
} vtss_appl_ospf_db_router_link_info_t;

//----------------------------------------------------------------------------
//** OSPF database detail router information
//----------------------------------------------------------------------------

/** \brief The OSPF link state database router entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF hello packets */
    uint8_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** The link count of router link state.
     *  (The parameter is used for Type 1 only) */
    uint32_t router_link_count;

} vtss_appl_ospf_db_router_data_entry_t;

/** \brief The OSPF link state database detail router entry (For get_all API). */
typedef struct {
    /** The OSPF instance ID. */
    vtss_appl_ospf_id_t inst_id;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf_area_id_t area_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf_lsdb_type_t lsdb_type;

    /** The OSPF link state ID. It identifies the piece of the routing domain
     * that is being described by the LSA.
     * The filed value has various meanings based on the LS type.
     *  LS Type     LS Description      Link State ID
     *  -------     --------------      -------------
     *  1           Router-LSAs         The originating router's Router ID.
     *  2           Network-LSAs        The IP interface address of the network's Designated Router.
     *  3           Summary-LSAs        The destination network's IP address.
     *  4           ASBR Summary-LSAs   The Router ID of the described AS boundary router.
     *  5           AS-external-LSAs    The destination network's IP address.
     *  7           NSSA-external-LSAs  The destination network's IP address.
     */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf_db_router_data_entry_t data;

} vtss_appl_ospf_db_detail_router_entry_t;

/**
 * \brief Iterate through the OSPF database detail router information.
 * \param cur_inst_id        [IN]  The current OSPF ID.
 * \param next_inst_id       [OUT] The next OSPF ID.
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
mesa_rc vtss_appl_ospf_db_detail_router_itr(
    const vtss_appl_ospf_id_t        *const cur_inst_id,
    vtss_appl_ospf_id_t              *const next_inst_id,
    const vtss_appl_ospf_area_id_t   *const cur_area_id,
    vtss_appl_ospf_area_id_t         *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF database detail router information.
 * \param inst_id         [IN]  The OSPF instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_db_detail_router_get(
    const vtss_appl_ospf_id_t        inst_id,
    const vtss_appl_ospf_area_id_t   area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_router_data_entry_t *const db_detail_info);

/**
 * \brief Get the OSPF database detail router information.
 * \param inst_id         [IN]  The OSPF instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param index           [IN]  The index of the router link.
 * \param link_info       [OUT] The OSPF database detail link information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_db_detail_router_entry_get_by_index(
    const vtss_appl_ospf_id_t inst_id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    const uint32_t index,
    vtss_appl_ospf_db_router_link_info_t *const link_info);


/**
 * \brief Get all OSPF database entries of detail router information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_router_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_router_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF database detail network information
//----------------------------------------------------------------------------

/** \brief The OSPF link state database network entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF hello packets */
    uint8_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** Network mask implemented. */
    uint32_t network_mask; // Need for type 2, 3, 4, 5

    /** Count of routers attached to the network. */
    uint32_t attached_router_count; // Need for type 2

} vtss_appl_ospf_db_network_data_entry_t;

/** \brief The OSPF link state database detail network entry (For get_all API). */
typedef struct {
    /** The OSPF instance ID. */
    vtss_appl_ospf_id_t inst_id;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf_area_id_t area_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf_lsdb_type_t lsdb_type;

    /** The OSPF link state ID. It identifies the piece of the routing domain
     * that is being described by the LSA.
     * The filed value has various meanings based on the LS type.
     *  LS Type     LS Description      Link State ID
     *  -------     --------------      -------------
     *  1           Router-LSAs         The originating router's Router ID.
     *  2           Network-LSAs        The IP interface address of the network's Designated Router.
     *  3           Summary-LSAs        The destination network's IP address.
     *  4           ASBR Summary-LSAs   The Router ID of the described AS boundary router.
     *  5           AS-external-LSAs    The destination network's IP address.
     *  7           NSSA-external-LSAs  The destination network's IP address.
     */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf_db_network_data_entry_t data;

} vtss_appl_ospf_db_detail_network_entry_t;

/**
 * \brief Iterate through the OSPF database detail network information.
 * \param cur_inst_id        [IN]  The current OSPF ID.
 * \param next_inst_id       [OUT] The next OSPF ID.
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
mesa_rc vtss_appl_ospf_db_detail_network_itr(
    const vtss_appl_ospf_id_t        *const cur_inst_id,
    vtss_appl_ospf_id_t              *const next_inst_id,
    const vtss_appl_ospf_area_id_t   *const cur_area_id,
    vtss_appl_ospf_area_id_t         *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF database detail network information.
 * \param inst_id         [IN]  The OSPF instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_db_detail_network_get(
    const vtss_appl_ospf_id_t        inst_id,
    const vtss_appl_ospf_area_id_t   area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_network_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF database entries of detail network information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_network_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_network_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF database detail summary/asbr-summary information
//----------------------------------------------------------------------------

/** \brief The OSPF link state database summary entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF hello packets */
    uint8_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** Network mask implemented. */
    uint32_t network_mask; // Need for type 2, 3, 4, 5

    /** User specified metric for this summary route. */
    uint32_t metric; // Need for type 3, 4, 5

} vtss_appl_ospf_db_summary_data_entry_t;

/** \brief The OSPF link state database detail summary/asbr-summary entry (For get_all API). */
typedef struct {
    /** The OSPF instance ID. */
    vtss_appl_ospf_id_t inst_id;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf_area_id_t area_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf_lsdb_type_t lsdb_type;

    /** The OSPF link state ID. It identifies the piece of the routing domain
     * that is being described by the LSA.
     * The filed value has various meanings based on the LS type.
     *  LS Type     LS Description      Link State ID
     *  -------     --------------      -------------
     *  1           Router-LSAs         The originating router's Router ID.
     *  2           Network-LSAs        The IP interface address of the network's Designated Router.
     *  3           Summary-LSAs        The destination network's IP address.
     *  4           ASBR Summary-LSAs   The Router ID of the described AS boundary router.
     *  5           AS-external-LSAs    The destination network's IP address.
     *  7           NSSA-external-LSAs  The destination network's IP address.
     */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf_db_summary_data_entry_t data;

} vtss_appl_ospf_db_detail_summary_entry_t;

/**
 * \brief Iterate through the OSPF database detail summary information.
 * \param cur_inst_id        [IN]  The current OSPF ID.
 * \param next_inst_id       [OUT] The next OSPF ID.
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
mesa_rc vtss_appl_ospf_db_detail_summary_itr(
    const vtss_appl_ospf_id_t        *const cur_inst_id,
    vtss_appl_ospf_id_t              *const next_inst_id,
    const vtss_appl_ospf_area_id_t   *const cur_area_id,
    vtss_appl_ospf_area_id_t         *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF database detail summary information.
 * \param inst_id         [IN]  The OSPF instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_db_detail_summary_get(
    const vtss_appl_ospf_id_t        inst_id,
    const vtss_appl_ospf_area_id_t   area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_summary_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF database entries of detail summary information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_summary_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_summary_entry_t> &db_entries);

/**
 * \brief Iterate through the OSPF database detail asbr-summary information.
 * \param cur_inst_id        [IN]  The current OSPF ID.
 * \param next_inst_id       [OUT] The next OSPF ID.
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
mesa_rc vtss_appl_ospf_db_detail_asbr_summary_itr(
    const vtss_appl_ospf_id_t        *const cur_inst_id,
    vtss_appl_ospf_id_t              *const next_inst_id,
    const vtss_appl_ospf_area_id_t   *const cur_area_id,
    vtss_appl_ospf_area_id_t         *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF database detail asbr-summary information.
 * \param inst_id         [IN]  The OSPF instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_db_detail_asbr_summary_get(
    const vtss_appl_ospf_id_t        inst_id,
    const vtss_appl_ospf_area_id_t   area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_summary_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF database entries of detail asbr-summary information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_asbr_summary_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_summary_entry_t> &db_entries);

//----------------------------------------------------------------------------
//** OSPF database detail external/nssa-external information
//----------------------------------------------------------------------------

/** \brief The OSPF link state database external entry (For get_all API). */
typedef struct {
    /** The time in seconds since the LSA was originated. */
    uint32_t age;

    /** The option field which is present in OSPF hello packets */
    uint8_t options;

    /** The LS sequence number. */
    uint32_t sequence;

    /** The checksum of the LSA contents. */
    uint32_t checksum;

    /** Length in bytes of the LSA. */
    uint32_t length;

    /** Network mask implemented. */
    uint32_t network_mask; // Need for type 2, 3, 4, 5

    /** User specified metric for this summary route. */
    uint32_t metric; // Need for type 3, 4, 5

    /** External type. */
    uint32_t metric_type; // Need for type 5

    /** Forwarding address. Data traffic for the advertised destination will
    be forwarded to this address. If the forwarding address is set to 0.0.0.0,
    data traffic will be forwarded instead to the advertisement's originator.
    */
    mesa_ipv4_t forward_address; // Need for type 5

} vtss_appl_ospf_db_external_data_entry_t;


/** \brief The OSPF link state database detail external/nssa-external entry (For get_all API). */
typedef struct {
    /** The OSPF instance ID. */
    vtss_appl_ospf_id_t inst_id;

    /** The OSPF area ID of the link state advertisement. */
    vtss_appl_ospf_area_id_t area_id;

    /** The type of the link state advertisement. */
    vtss_appl_ospf_lsdb_type_t lsdb_type;

    /** The OSPF link state ID. It identifies the piece of the routing domain
     * that is being described by the LSA.
     * The filed value has various meanings based on the LS type.
     *  LS Type     LS Description      Link State ID
     *  -------     --------------      -------------
     *  1           Router-LSAs         The originating router's Router ID.
     *  2           Network-LSAs        The IP interface address of the network's Designated Router.
     *  3           Summary-LSAs        The destination network's IP address.
     *  4           ASBR Summary-LSAs   The Router ID of the described AS boundary router.
     *  5           AS-external-LSAs    The destination network's IP address.
     *  7           NSSA-external-LSAs  The destination network's IP address.
     */
    mesa_ipv4_t link_state_id;

    /** The advertising router ID which originated the LSA. */
    vtss_appl_ospf_router_id_t adv_router_id;

    /** The database detail informaton. */
    vtss_appl_ospf_db_external_data_entry_t data;

} vtss_appl_ospf_db_detail_external_entry_t;

/**
 * \brief Iterate through the OSPF database detail external information.
 * \param cur_inst_id        [IN]  The current OSPF ID.
 * \param next_inst_id       [OUT] The next OSPF ID.
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
mesa_rc vtss_appl_ospf_db_detail_external_itr(
    const vtss_appl_ospf_id_t        *const cur_inst_id,
    vtss_appl_ospf_id_t              *const next_inst_id,
    const vtss_appl_ospf_area_id_t   *const cur_area_id,
    vtss_appl_ospf_area_id_t         *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF database detail external information.
 * \param inst_id         [IN]  The OSPF instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_db_detail_external_get(
    const vtss_appl_ospf_id_t        inst_id,
    const vtss_appl_ospf_area_id_t   area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_external_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF database entries of detail external information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_external_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_external_entry_t> &db_entries);

/**
 * \brief Iterate through the OSPF database detail nssa-external information.
 * \param cur_inst_id        [IN]  The current OSPF ID.
 * \param next_inst_id       [OUT] The next OSPF ID.
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
mesa_rc vtss_appl_ospf_db_detail_nssa_external_itr(
    const vtss_appl_ospf_id_t        *const cur_inst_id,
    vtss_appl_ospf_id_t              *const next_inst_id,
    const vtss_appl_ospf_area_id_t   *const cur_area_id,
    vtss_appl_ospf_area_id_t         *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t       *const next_lsdb_type,
    const mesa_ipv4_t                *const cur_link_state_id,
    mesa_ipv4_t                      *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t       *const next_router_id);

/**
 * \brief Get the OSPF database detail nssa-external information.
 * \param inst_id         [IN]  The OSPF instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_db_detail_nssa_external_get(
    const vtss_appl_ospf_id_t        inst_id,
    const vtss_appl_ospf_area_id_t   area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t                link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_external_data_entry_t *const db_detail_info);

/**
 * \brief Get all OSPF database entries of detail nssa-external information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_nssa_external_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_external_entry_t> &db_entries);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_OSPF_H_ */
