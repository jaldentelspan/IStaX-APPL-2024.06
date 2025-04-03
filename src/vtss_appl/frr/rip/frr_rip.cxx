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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "critd_api.h"  // For semaphore/mutex wrapper
#include "frr_rip_access.hxx"
#include "frr_rip_api.hxx"  // For module APIs
#include "frr_utils.hxx"
#include "ip_utils.hxx"  // For the operator of mesa_ipv4_network_t
#include "main.h"      // For init_cmd_t
#include "vtss/appl/ip.h"
#include "vtss/appl/vlan.h"  // For VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX
#if defined(VTSS_SW_OPTION_ICFG)
#include "frr_rip_icfg.hxx"  // For module ICFG
#endif                       /* VTSS_SW_OPTION_ICFG */
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif /* VTSS_SW_OPTION_SYSLOG */

#include "frr_router_api.hxx"  // For frr_router_access_name_is_valid()

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_RIP
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Module semaphore/mutex declaration                                        */
/******************************************************************************/
static critd_t FRR_rip_crit;

struct FRR_rip_lock {
    FRR_rip_lock(int line)
    {
        critd_enter(&FRR_rip_crit, __FILE__, line);
    }
    ~FRR_rip_lock()
    {
        critd_exit(&FRR_rip_crit, __FILE__, 0);
    }
};

/* Semaphore/mutex protection
 * Usage:
 * 1. Every non-static function called `rip_xxx` has a CRIT_SCOPE() as the
 *    first thing in the body.
 * 2. No static function has a CRIT_SCOPE()
 * 3. If the non-static functions are not allowed to call non-static functions.
 *   (if needed, then move the functionality to a static function)
 */
#define CRIT_SCOPE() FRR_rip_lock __lock_guard__(__LINE__)

/* This macro definition is used to make sure the following codes has been
 * protected by semaphore/mutex alreay. In most cases, we use it in the static
 * function. The system will raise an error if the upper layer caller doesn't
 * call CRIT_SCOPE() before calling the API. */
#define FRR_CRIT_ASSERT_LOCKED() \
    critd_assert_locked(&FRR_rip_crit, __FILE__, __LINE__)

/******************************************************************************/
/** Module internal variables                                                 */
/******************************************************************************/
static mesa_bool_t RIP_enabled = false;

// Our capabilities
static vtss_appl_rip_capabilities_t RIP_cap;

/******************************************************************************/
// RIP_cap_init()
/******************************************************************************/
static void RIP_cap_init(void)
{
    // Most members of RIP_cap are statically initialized in the header.
    // This function adds the dynamic ones.

    // We only support redistribution of OSPF routes if we have the daemon
    // included.
    RIP_cap.ospf_redistributed_supported = frr_has_ospfd();

    // No need to be able to add more networks than we have VLAN interfaces.
    RIP_cap.network_segment_max_count = fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT);
}

/******************************************************************************/
/** Module internal APIs                                                      */
/******************************************************************************/
/**
 * Check if RIP is enabled or not.
 * Some configuration can't be set/got when RIP is disabled. In order to
 * save the effort to get the RIP mode from FRR access layer, there's a local
 * variable to keep the value when RIP mode is set, then the caller can invoke
 * this function to determine if RIP is enabled or not.
 */
static mesa_bool_t RIP_router_is_enabled(void)
{
    FRR_CRIT_ASSERT_LOCKED();
    return RIP_enabled;
}

/* Get IP by ifindex */
static mesa_rc RIP_get_ip_by_ifindex(vtss_ifindex_t ifindex, mesa_ipv4_t *const addr)
{
    vtss_appl_ip_if_key_ipv4_t key = {};

    // Get IPv4 addr via VLAN interface
    key.ifindex = ifindex;
    if (vtss_appl_ip_if_status_ipv4_itr(&key, &key) != VTSS_RC_OK || key.ifindex != ifindex) {
        T_DG(FRR_TRACE_GRP_RIP, "Get Current IP Address of %s failed", ifindex);
        return VTSS_RC_ERROR;
    } else {
        *addr = key.addr.address;
    }

    return VTSS_RC_OK;
}

/* Mapping FrrRipIfAuthMode to vtss_appl_rip_auth_type_t */
static vtss_appl_rip_auth_type_t RIP_auth_mode_mapping(FrrRipIfAuthMode mode)
{
    return static_cast<vtss_appl_rip_auth_type_t>(mode);
}
/* Mapping vtss_appl_rip_auth_type_t to FrrRipIfAuthMode */
static FrrRipIfAuthMode RIP_auth_type_mapping(vtss_appl_rip_auth_type_t type)
{
    return static_cast<FrrRipIfAuthMode>(type);
}

/******************************************************************************/
/** Application public APIs */
/******************************************************************************/
//------------------------------------------------------------------------------
//** RIP module capabilities
//------------------------------------------------------------------------------
/**
 * \brief Get RIP capabilities. (valid ranges and support features)
 * \param cap [OUT] RIP capabilities
 * \return Error code.
 */
mesa_rc vtss_appl_rip_capabilities_get(vtss_appl_rip_capabilities_t *const cap)
{
    /* Check illegal parameters */
    if (!cap) {
        VTSS_TRACE(ERROR) << "Parameter 'cap' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    *cap = RIP_cap;
    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------
/**
 * \brief Get the RIP router default configuration.
 * \param conf [OUT] RIP router configuration.
 * \return Error code.
 */
mesa_rc frr_rip_router_conf_def(vtss_appl_rip_router_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero initial value below
    conf->version = VTSS_APPL_RIP_GLOBAL_VER_DEFAULT;
    conf->redist_def_metric = 1;
    conf->timers.update_timer = 30;
    conf->timers.invalid_timer = 180;
    conf->timers.garbage_collection_timer = 120;
    conf->admin_distance = 120;
    for (u32 idx = 0; idx < VTSS_APPL_RIP_REDIST_PROTOCOL_COUNT; ++idx) {
        conf->redist_conf[idx].metric = VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MIN;
    }

    return VTSS_RC_OK;
}

static mesa_rc RIP_router_conf_get(vtss_appl_rip_router_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    VTSS_RC(frr_rip_router_conf_def(conf));

    /* Get running-config output from FRR layer */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    /* Get RIP router configuration from running-config output */
    auto frr_conf = frr_rip_router_conf_get(frr_running_conf);
    if (frr_conf.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_RIP, "Access framework failed: Get stub router configuration: %s", error_txt(frr_conf.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    // Router mode
    conf->router_mode = frr_conf->router_mode;

    // Version
    FrrRipVer frr_rip_ver = frr_conf->version.get();
    conf->version = frr_rip_ver == FrrRipVer_1
                    ? VTSS_APPL_RIP_GLOBAL_VER_1
                    : frr_rip_ver == FrrRipVer_2
                    ? VTSS_APPL_RIP_GLOBAL_VER_2
                    : VTSS_APPL_RIP_GLOBAL_VER_DEFAULT;

    // Timers
    conf->timers.update_timer = frr_conf->timers.get().update_timer;
    conf->timers.invalid_timer = frr_conf->timers.get().invalid_timer;
    conf->timers.garbage_collection_timer =
        frr_conf->timers.get().garbage_collection_timer;

    // Redistributed protocol types
    Vector<FrrRipRouterRedistribute> frr_redist_conf = frr_rip_router_redistribute_conf_get(frr_running_conf);
    for (auto redist_conf : frr_redist_conf) {
        switch (redist_conf.protocol) {
        case VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED:
            conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED].is_enabled =
                true;
            conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED]
            .is_specific_metric = redist_conf.metric.valid();
            if (redist_conf.metric.valid()) {
                conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED].metric =
                    redist_conf.metric.get();
            }

            break;

        case VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC:
            conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC].is_enabled =
                true;
            conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC]
            .is_specific_metric = redist_conf.metric.valid();
            if (redist_conf.metric.valid()) {
                conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC].metric =
                    redist_conf.metric.get();
            }

            break;

        case VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF:
            conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF].is_enabled =
                true;
            conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF].is_specific_metric =
                redist_conf.metric.valid();
            if (redist_conf.metric.valid()) {
                conf->redist_conf[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF].metric =
                    redist_conf.metric.get();
            }

            break;

        default:
            break;
        }
    }

    // Redistributed default metric
    conf->redist_def_metric = frr_conf->redist_def_metric.get();

    // Default route redistribution
    conf->def_route_redist = frr_conf->def_route_redist.get();

    // Passive-interface default mode
    conf->def_passive_intf = frr_conf->def_passive_intf.get();

    // Administrative distance
    conf->admin_distance = frr_conf->admin_distance.get();

    return VTSS_RC_OK;
}

/**
 * \brief Get the RIP router configuration.
 * \param conf [OUT] RIP router configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_conf_get(vtss_appl_rip_router_conf_t *const conf)
{
    CRIT_SCOPE();
    return RIP_router_conf_get(conf);
}

/**
 * \brief Set the RIP router configuration.
 * \param conf [IN] RIP router configuration.
 *                  only "router_mode" parameter is needed when rip router mode
 *                  disabled
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_conf_set(const vtss_appl_rip_router_conf_t *const conf)
{
    CRIT_SCOPE();
    mesa_rc rc = VTSS_RC_OK;
    mesa_bool_t is_config_changed = false;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }
    // Check parameters when rip router mode is enabled
    if (conf->router_mode) {
        if (conf->timers.update_timer < VTSS_APPL_RIP_TIMER_MIN ||
            conf->timers.update_timer > VTSS_APPL_RIP_TIMER_MAX) {
            VTSS_TRACE(DEBUG) << "Parameter 'timers'("
                              << conf->timers.update_timer << ") is invalid";
            return FRR_RC_INVALID_ARGUMENT;
        }

        if (conf->version >= VTSS_APPL_RIP_GLOBAL_VER_COUNT) {
            VTSS_TRACE(DEBUG) << "Parameter 'version'(" << conf->version
                              << ") is invalid";
            return FRR_RC_INVALID_ARGUMENT;
        }

        if (conf->timers.invalid_timer < VTSS_APPL_RIP_TIMER_MIN ||
            conf->timers.invalid_timer > VTSS_APPL_RIP_TIMER_MAX) {
            VTSS_TRACE(DEBUG) << "Parameter 'timers'("
                              << conf->timers.invalid_timer << ") is invalid";
            return FRR_RC_INVALID_ARGUMENT;
        }

        if (conf->timers.garbage_collection_timer < VTSS_APPL_RIP_TIMER_MIN ||
            conf->timers.garbage_collection_timer > VTSS_APPL_RIP_TIMER_MAX) {
            VTSS_TRACE(DEBUG)
                    << "Parameter 'timers'("
                    << conf->timers.garbage_collection_timer << ") is invalid";
            return FRR_RC_INVALID_ARGUMENT;
        }

        for (u32 idx = 0; idx < VTSS_APPL_RIP_REDIST_PROTOCOL_COUNT; ++idx) {
            if (conf->redist_conf[idx].is_enabled &&
                conf->redist_conf[idx].is_specific_metric &&
                (
#if VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MIN != 0
                    // Quiet Coverity warning:
                    // Operands don't affect result (The minimum value of
                    // 'uint8_t' is 0)
                    conf->redist_conf[idx].metric <
                    VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MIN ||
#endif
                    conf->redist_conf[idx].metric >
                    VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MAX)) {
                VTSS_TRACE(DEBUG)
                        << "Parameter 'metric'("
                        << conf->redist_conf[idx].metric << ") is invalid";
                return FRR_RC_INVALID_ARGUMENT;
            }
        }

        if (conf->redist_def_metric < VTSS_APPL_RIP_REDIST_DEF_METRIC_MIN ||
            conf->redist_def_metric > VTSS_APPL_RIP_REDIST_DEF_METRIC_MAX) {
            VTSS_TRACE(DEBUG) << "Parameter 'redist_def_metric "
                              << conf->admin_distance << " is invalid";
            return FRR_RC_INVALID_ARGUMENT;
        }

        if (conf->admin_distance < VTSS_APPL_RIP_ADMIN_DISTANCE_MIN) {
            VTSS_TRACE(DEBUG) << "Parameter 'admin_distance "
                              << conf->admin_distance << " is invalid";
            return FRR_RC_INVALID_ARGUMENT;
        }
    }

    /* Get original configuration */
    vtss_appl_rip_router_conf_t orig_conf;
    if (RIP_router_conf_get(&orig_conf) != VTSS_RC_OK) {
        VTSS_TRACE(WARNING) << "Get RIP router current configuration failed.";
        return VTSS_APPL_FRR_RIP_ERROR_GEN;
    }

    /* Check if the new RIP router mode is changed.
     * The new mode MUST be applied before other setting. */
    if (conf->router_mode != orig_conf.router_mode) {  // RIP mode is changed
        frr_rip_router_conf_set(conf->router_mode);
        RIP_enabled = conf->router_mode;
    }

    /* Terminate the process since all RIP router configured parameters
     * are significant only when the router mode is enabled. */
    if (!conf->router_mode) {
        return VTSS_RC_OK;
    }

    FrrRipRouterConf frr_conf;
    frr_conf.router_mode = conf->router_mode;

    // Check the configuration of global version is changed or not
    if (conf->version != orig_conf.version) {
        is_config_changed = true;
        frr_conf.version = conf->version == VTSS_APPL_RIP_GLOBAL_VER_1
                           ? FrrRipVer_1
                           : conf->version == VTSS_APPL_RIP_GLOBAL_VER_2
                           ? FrrRipVer_2
                           : FrrRipVer_Both;
    }

    // Check the configuration of timers is changed or not
    if (conf->timers.update_timer != orig_conf.timers.update_timer ||
        conf->timers.invalid_timer != orig_conf.timers.invalid_timer ||
        conf->timers.garbage_collection_timer !=
        orig_conf.timers.garbage_collection_timer) {
        is_config_changed = true;
        frr_conf.timers = {conf->timers.update_timer, conf->timers.invalid_timer,
                           conf->timers.garbage_collection_timer
                          };
    }

    // Check the configuration of passive-interface default mode is changed
    // or not
    if (conf->def_passive_intf != orig_conf.def_passive_intf) {
        is_config_changed = true;
        frr_conf.def_passive_intf = conf->def_passive_intf;
    }

    // Check the configuration of default metric is changed or not
    if (conf->redist_def_metric != orig_conf.redist_def_metric) {
        is_config_changed = true;
        frr_conf.redist_def_metric = conf->redist_def_metric;
    }

    // Check the configuration of default route redistribution is changed or not
    if (conf->def_route_redist != orig_conf.def_route_redist) {
        is_config_changed = true;
        frr_conf.def_route_redist = conf->def_route_redist;
    }

    // Check the configuration of administrative distance is changed or not
    if (conf->admin_distance != orig_conf.admin_distance) {
        is_config_changed = true;
        frr_conf.admin_distance = conf->admin_distance;
    }

    // Apply the new configuration if needed
    if (is_config_changed) {
        rc = frr_rip_router_conf_set(frr_conf);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set rip router conf ("
                    << "rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Check the configuration of redistribute protocol/metric is changed or not
    for (uint32_t idx = 0; idx < VTSS_APPL_RIP_REDIST_PROTOCOL_COUNT; idx++) {
        const vtss_appl_rip_redist_conf_t *orig_redist_conf = &orig_conf.redist_conf[idx];
        const vtss_appl_rip_redist_conf_t *redist_conf      = &conf->redist_conf[idx];

        if (orig_redist_conf->is_enabled == redist_conf->is_enabled && orig_redist_conf->is_specific_metric == redist_conf->is_specific_metric && orig_redist_conf->metric == redist_conf->metric) {
            continue;  // Do nothing when the values are the same.
        }

        // Apply new route redistribution configuration
        vtss_appl_ip_route_protocol_t protocol;
        if (idx == VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED) {
            protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED;
        } else if (idx == VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC) {
            protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
        } else {
            protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF;
        }

        if (redist_conf->is_enabled) {
            /* APPL-1652:
             * Change redistribute metric from the specific value to default
             * value does not work.
             *
             * FRR will disable the redistribute protocol type too when removing
             * the specific metric setting.
             * So we made a workaround:
             * Re-enable the redistribute protocol type to clear the specific
             * metric setting.
             */
            if (!redist_conf->is_specific_metric &&
                orig_redist_conf->is_specific_metric) {
                // Canged from specific metric value to default value
                rc = frr_rip_router_redistribute_conf_del(protocol);
                if (rc != VTSS_RC_OK) {
                    VTSS_TRACE(DEBUG) << "Access framework failed: Set route "
                                      "redistribution "
                                      << "metric_type = " << idx
                                      << ", rc = " << rc << ")";
                    return FRR_RC_INTERNAL_ACCESS;
                }
            }

            FrrRipRouterRedistribute frr_redist_conf;
            if (redist_conf->is_specific_metric) {
                frr_redist_conf.metric = redist_conf->metric;
            }

            frr_redist_conf.protocol = protocol;
            rc = frr_rip_router_redistribute_conf_set(frr_redist_conf);
        } else {
            rc = frr_rip_router_redistribute_conf_del(protocol);
        }

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set route redistribution "
                    << "metric_type = " << idx << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
static mesa_rc RIP_network_conf_get(const mesa_ipv4_network_t *const network, mesa_bool_t check_overlap, uint32_t *const total_cnt)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!network) {
        VTSS_TRACE(ERROR) << "Parameter 'network' cannot be null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!total_cnt) {
        VTSS_TRACE(ERROR) << "Parameter 'total_cnt' cannot be null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    } else {
        *total_cnt = 0;  // Given an initial value
    }

    /* Get data from FRR layer */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    auto frr_network_conf = frr_rip_network_conf_get(frr_running_conf);
    *total_cnt = frr_network_conf.size();
    if (frr_network_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty RIP network";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    for (const auto &itr : frr_network_conf) {
        if (vtss_ipv4_net_mask_out(&itr.net) == vtss_ipv4_net_mask_out(network)) {
            // Found it
            return VTSS_RC_OK;
        }

        if (check_overlap && vtss_ipv4_net_overlap(&itr.net, network)) {
            return FRR_RC_ADDR_RANGE_OVERLAP;
        }
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get the RIP network configuration.
 * \param network [IN]  RIP area network.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_network_conf_get(const mesa_ipv4_network_t *const network)
{
    CRIT_SCOPE();
    uint32_t total_cnt;
    return RIP_network_conf_get(network, false, &total_cnt);
}

/**
 * \brief Add the RIP network configuration.
 * \param network [IN] RIP network.
 * \return Error code.
 * FRR_RC_ADDR_RANGE_OVERLAP means that the network range is
 * overlapped.
 * change doesn't take effect.
 */
mesa_rc vtss_appl_rip_network_conf_add(const mesa_ipv4_network_t *const network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!network) {
        VTSS_TRACE(ERROR) << "Parameter 'network' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // Return error if the address isn't unicast address or it is loopback
    // address.
    if (!vtss_ipv4_addr_is_unicast(&network->address) ||
        vtss_ipv4_addr_is_loopback(&network->address)) {
        VTSS_TRACE(DEBUG) << "The address is invalid.";
        return VTSS_APPL_FRR_RIP_ERROR_NOT_UNICAST_ADDRESS;
    }

    // Check if RIP is enabled.
    if (!RIP_router_is_enabled()) {
        return VTSS_APPL_FRR_RIP_ERROR_ROUTER_DISABLED;
    }

    /* Lookup this entry if already existing or IP ranage is overlap */
    uint32_t total_cnt;
    mesa_rc rc = RIP_network_conf_get(network, true, &total_cnt);
    if (rc == FRR_RC_ADDR_RANGE_OVERLAP) {
        // Don't allow IP range is overlapped
        return rc;
    }

    /* Check if reach the max. entry count. */
    if (rc == FRR_RC_ENTRY_NOT_FOUND &&
        total_cnt >= RIP_cap.network_segment_max_count) {
        return FRR_RC_LIMIT_REACHED;
    }

    /* Apply to FRR layer */
    vtss::FrrRipNetwork frr_network_conf = {vtss_ipv4_net_mask_out(network)};
    rc = frr_rip_network_conf_set(frr_network_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set RIP network. "
                          << ", network_addr = " << *network << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the RIP network default configuration.
 * \param network [OUT] RIP network.
 * \return Error code.
 */
mesa_rc frr_rip_network_conf_def(mesa_ipv4_network_t *const network)
{
    /* Check illegal parameters */
    if (!network) {
        VTSS_TRACE(ERROR) << "Parameter 'network' cannot be null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*network);

    return VTSS_RC_OK;
}

/**
 * \brief Delete the RIP network configuration.
 * \param network [IN] RIP network.
 * \return Error code.
 * change doesn't take effect.
 */
mesa_rc vtss_appl_rip_network_conf_del(const mesa_ipv4_network_t *const network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!network) {
        VTSS_TRACE(ERROR) << "Parameter 'network' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // Return error if the address isn't unicast address or it is loopback
    // address.
    if (!vtss_ipv4_addr_is_unicast(&network->address) ||
        vtss_ipv4_addr_is_loopback(&network->address)) {
        VTSS_TRACE(DEBUG) << "The address is invalid.";
        return VTSS_APPL_FRR_RIP_ERROR_NOT_UNICAST_ADDRESS;
    }

    /* Check the entry is existing or not */
    uint32_t total_cnt;
    mesa_rc rc = RIP_network_conf_get(network, false, &total_cnt);
    if (rc != VTSS_RC_OK) {
        // For the deleting operation, quit silently when it does not exists
        return VTSS_RC_OK;
    }

    /* Apply to FRR layer */
    vtss::FrrRipNetwork frr_network_conf = {vtss_ipv4_net_mask_out(network)};
    rc = frr_rip_network_conf_del(frr_network_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete RIP network. "
                          << "network_addr = " << *network << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

static mesa_rc RIP_network_conf_itr(const mesa_ipv4_network_t *const cur, mesa_ipv4_network_t *const next)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    /* Get data from FRR layer */
    auto frr_network_conf = frr_rip_network_conf_get(frr_running_conf);
    if (frr_network_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    /* Build the local sorted database for the key  */
    vtss::Set<mesa_ipv4_network_t> key_set;
    for (const auto &itr : frr_network_conf) {
        key_set.insert(itr.net);
    }

    Set<mesa_ipv4_network_t>::iterator key_itr;
    if (!cur) {  // Get-First operation
        key_itr = key_set.begin();
    } else {  // Get-Next operation
        key_itr = key_set.greater_than(*cur);
    }

    if (key_itr != key_set.end()) {
        VTSS_TRACE(DEBUG) << "Found: key = " << *key_itr;
        *next = *key_itr;
        return VTSS_RC_OK;  // Found it, break the loop
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate the IP networks
 * \param cur_network   [IN]  Current RIP network
 * \param next_network  [OUT] Next RIP network
 * \return Error code.
 */
mesa_rc vtss_appl_rip_network_conf_itr(const mesa_ipv4_network_t *const cur_network,
                                       mesa_ipv4_network_t *const next_network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_network) {
        VTSS_TRACE(ERROR) << "Parameter 'next_network' cannot be null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    }

    return RIP_network_conf_itr(cur_network, next_network);
}

//----------------------------------------------------------------------------
//** RIP router interface configuration
//----------------------------------------------------------------------------
static mesa_rc RIP_router_intf_conf_get(const vtss_ifindex_t ifindex, vtss_appl_rip_router_intf_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // Check if RIP is enabled.
    if (!RIP_router_is_enabled()) {
        return VTSS_APPL_FRR_RIP_ERROR_ROUTER_DISABLED;
    }

    /* Check if the interface exists or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get data from FRR layer */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    /* Get configuration from FRR running config */
    auto passive_if_mode =
        frr_rip_router_passive_if_conf_get(frr_running_conf, ifindex);
    if (passive_if_mode.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get passive interface "
                          "mode. (ifindex = "
                          << ifindex << ", rc = " << passive_if_mode.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->passive_enabled = passive_if_mode.val;

    return VTSS_RC_OK;
}

/**
 * \brief Get the RIP router interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] RIP router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_intf_conf_get(
    const vtss_ifindex_t ifindex,
    vtss_appl_rip_router_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    return RIP_router_intf_conf_get(ifindex, conf);
}

/**
 * \brief Set the RIP router interface configuration.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] RIP router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_intf_conf_set(
    const vtss_ifindex_t ifindex,
    const vtss_appl_rip_router_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    vtss_appl_rip_router_intf_conf_t orig_conf;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (RIP_router_intf_conf_get(ifindex, &orig_conf) != VTSS_RC_OK) {
        return rc;
    }

    /* Set the configuration to FRR layer if needed */
    if (orig_conf.passive_enabled != conf->passive_enabled) {
        if ((rc = frr_rip_router_passive_if_conf_set(
                      ifindex, conf->passive_enabled)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set passive interface "
                    "mode. (ifindex = "
                    << ifindex << ", mode = " << conf->passive_enabled
                    << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all RIP router interfaces.
 * \param current_ifindex [IN]  The current ifIndex
 * \param next_ifindex    [OUT] The next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_rip_router_intf_conf_itr(const vtss_ifindex_t *const current_ifindex, vtss_ifindex_t *const next_ifindex)
{
    CRIT_SCOPE();

    // Terminate the iteration if RIP is disabled.
    if (!RIP_router_is_enabled()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    return vtss_appl_ip_if_itr(current_ifindex, next_ifindex);
}

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
static mesa_rc RIP_neighbor_conf_get_or_getnext(
    const mesa_ipv4_t *const neighbor_addr,
    mesa_ipv4_t *const next_addr = NULL, uint32_t *const total_cnt = NULL)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (total_cnt) {
        *total_cnt = 0;  // Given an initial value
    }

    // Check if RIP is enabled.
    if (!RIP_router_is_enabled()) {
        return VTSS_APPL_FRR_RIP_ERROR_ROUTER_DISABLED;
    }

    /* Get FRR running config data from FRR layer */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    /* Get configuration from FRR running config */
    auto configs = frr_rip_neighbor_conf_get(frr_running_conf);
    if (total_cnt) {
        *total_cnt = configs.size();
    }

    vtss::Set<mesa_ipv4_t>::iterator itr;
    if (!next_addr) {  // Get
        itr = configs.find(*neighbor_addr);
    } else if (neighbor_addr) {  // Get next
        itr = configs.greater_than(*neighbor_addr);
    } else {  // Get first
        itr = configs.begin();
    }

    if (itr == configs.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    if (next_addr) {
        *next_addr = *itr;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the RIP neighbor connection configuration.
 * \param neighbor_addr [OUT] The RIP neighbor address.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_neighbor_conf_get(const mesa_ipv4_t neighbor_addr)
{
    CRIT_SCOPE();

    return RIP_neighbor_conf_get_or_getnext(&neighbor_addr);
}

/* The neighbor address cannot be class D, class E, loopback or all zeros */
static mesa_bool_t rip_neighbor_address_is_valid(const mesa_ipv4_t neighbor_addr)
{
    FRR_CRIT_ASSERT_LOCKED();

    return vtss_ipv4_addr_is_unicast(&neighbor_addr) &&
           !vtss_ipv4_addr_is_loopback(&neighbor_addr) && neighbor_addr;
}

/**
 * \brief Add the RIP neighbor connection configuration.
 * \param neighbor_addr [IN] The RIP neighbor address.
 * \return Error code.
 * VTSS_APPL_FRR_RIP_ERROR_INVALID_ADDRESS_FOR_NEIGHBOR_CONNECTION means that
 * the IP address is not unicast, broadcast, or network IP address , which is
 * invalid for neighbor connection.
 */
mesa_rc vtss_appl_rip_neighbor_conf_add(const mesa_ipv4_t neighbor_addr)
{
    CRIT_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;

    /* Validate 'neighbor_addr' */
    if (!rip_neighbor_address_is_valid(neighbor_addr)) {
        return VTSS_APPL_FRR_RIP_ERROR_INVALID_ADDRESS_FOR_NEIGHBOR_CONNECTION;
    }

    /* Silent return if the entry exists */
    uint32_t total_cnt;
    if ((rc = RIP_neighbor_conf_get_or_getnext(&neighbor_addr, NULL,
                                               &total_cnt)) == VTSS_RC_OK) {
        return VTSS_RC_OK;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    /* Check if reach the max. entry count */
    if (total_cnt >= VTSS_APPL_RIP_NEIGHBOR_MAX_COUNT) {
        return FRR_RC_LIMIT_REACHED;
    }

    /* Set the configuration to FRR layer if needed */
    if ((rc = frr_rip_neighbor_conf_set(neighbor_addr)) != VTSS_RC_OK) {
        mesa_ipv4_t ipv4_addr {neighbor_addr};
        VTSS_TRACE(DEBUG) << "Access framework failed: Add neighbor connection "
                          "(address = "
                          << ipv4_addr << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Delete the RIP neighbor connection configuration.
 * \param neighbor_addr [IN] The RIP neighbor address.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_neighbor_conf_del(const mesa_ipv4_t neighbor_addr)
{
    CRIT_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;

    /* Validate 'neighbor_addr' */
    if (!rip_neighbor_address_is_valid(neighbor_addr)) {
        return VTSS_APPL_FRR_RIP_ERROR_INVALID_ADDRESS_FOR_NEIGHBOR_CONNECTION;
    }

    /* Silent return if the entry not found */
    if ((rc = RIP_neighbor_conf_get_or_getnext(&neighbor_addr)) != VTSS_RC_OK) {
        return rc == FRR_RC_ENTRY_NOT_FOUND ? VTSS_RC_OK : rc;
    }

    /* Set the configuration to FRR layer if needed */
    if ((rc = frr_rip_neighbor_conf_del(neighbor_addr)) != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Add neighbor connection "
                          "( address = "
                          << neighbor_addr << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Set the RIP neighbor connection configuration.
 * It is a dummy function for JSON/SNMP serialzer only.
 * \param neighbor_addr [IN] The RIP neighbor address.
 * \return Always return FRR_RC_NOT_SUPPORTED.
 */
mesa_rc frr_rip_neighbor_dummy_set(const mesa_ipv4_t neighbor_addr)
{
    return FRR_RC_NOT_SUPPORTED;
}

/**
 * \brief Iterate the RIP neighbor connection configuration
 * \param cur_nb_addr  [IN]  The current RIP neighbor address.
 * \param next_nb_addr [OUT] The next RIP neighbor address.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_neighbor_conf_itr(const mesa_ipv4_t *const cur_nb_addr,
                                        mesa_ipv4_t *const next_nb_addr)
{
    CRIT_SCOPE();
    return RIP_neighbor_conf_get_or_getnext(cur_nb_addr, next_nb_addr);
}

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
/**
 * \brief Get the authentication configuration in the specific interface.
 * \param ifindex      [IN]  The index of VLAN interface.
 * \param as_encrypted [IN]  Set 'true' to output conf->auth_key as encrypted,
 otherwise out it as enencrypted.
 * \param conf         [OUT] The authentication configuration.
 * \return Error code.
 */
static mesa_rc RIP_intf_auth_conf_get(std::string frr_running_conf, const vtss_ifindex_t ifindex, bool as_encrypted, vtss_appl_rip_intf_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check that the interface exists */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        T_DG(FRR_TRACE_GRP_RIP, "Interface %s does not exists", ifindex);
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get the authentication configuration from FRR layer. */
    auto auth_conf = frr_rip_if_authentication_conf_get(frr_running_conf, ifindex);
    if (!auth_conf) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get intf auth "
                          "configuration. (ifindex = "
                          << ifindex << ", rc = " << auth_conf.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    // Fill in the authentication mode and key chain name.
    conf->auth_type = RIP_auth_mode_mapping(auth_conf->auth_mode);
    strncpy(conf->md5_key_chain_name, auth_conf->keychain_name.c_str(),
            sizeof(conf->md5_key_chain_name) - 1);
    conf->md5_key_chain_name[sizeof(conf->md5_key_chain_name) - 1] = '\0';

    // Get the simple password from FRR layer and encrypt it if 'as_encrypted'
    // is true.
    if ((!auth_conf->simple_pwd.empty()) && as_encrypted) {
        VTSS_TRACE(DEBUG) << ifindex << " encrypting " << auth_conf->simple_pwd;
        if (frr_util_secret_key_cryptography(
                true, auth_conf->simple_pwd.c_str(),
                VTSS_APPL_RIP_AUTH_ENCRYPTED_SIMPLE_KEY_LEN + 1,
                conf->simple_pwd) != VTSS_RC_OK) {
            VTSS_TRACE(ERROR) << "Access framework failed: RIP simple "
                              "password encryption failed";
            return FRR_RC_INTERNAL_ERROR;
        }

        VTSS_TRACE(DEBUG) << ifindex << " encrypted data is " << conf->simple_pwd;
        conf->is_encrypted = true;
    } else {
        strncpy(conf->simple_pwd, auth_conf->simple_pwd.c_str(),
                sizeof(conf->simple_pwd) - 1);
        conf->simple_pwd[sizeof(conf->simple_pwd) - 1] = '\0';
        conf->is_encrypted = false;
    }

    return VTSS_RC_OK;
}

/* Validate the secret key is valid or not.
 * \param is_encrypted_key [IN]  Set 'true' if the key is encrypted.
 * \param key              [IN]  The key.
 * \param unencrypted_key  [OUT] The unencrypted key (when parameter key is
 * encrypted).
 * \return Error code.
 *  VTSS_APPL_FRR_RIP_ERROR_AUTH_KEY_INVALID means the key is not a valid
 *  key for AES256.
 */
static mesa_rc frr_rip_validate_secret_key(const bool is_encrypted_key,
                                           const char *const key)
{
    /* Check valid length */
    // Maximum length check for unencrypted key
    if (!is_encrypted_key && strlen(key) > VTSS_APPL_RIP_AUTH_SIMPLE_KEY_MAX_LEN) {
        VTSS_TRACE(DEBUG) << "Parameter 'unencrypted_key' length(" << strlen(key) << ") is invalid";
        return VTSS_APPL_FRR_RIP_ERROR_AUTH_KEY_INVALID;
    }

    // Minimum length check for encrypted key
    if (is_encrypted_key &&
        strlen(key) < VTSS_APPL_RIP_AUTH_ENCRYPTED_KEY_LEN(
            VTSS_APPL_RIP_AUTH_SIMPLE_KEY_MIN_LEN)
        /* excluded terminal character */) {
        VTSS_TRACE(DEBUG) << "Parameter 'encrypted_key' length(" << strlen(key)
                          << ") is invalid ";
        return VTSS_APPL_FRR_RIP_ERROR_AUTH_KEY_INVALID;
    }

    /* Check valid input (hex character only) */
    if (is_encrypted_key) {
        for (size_t i = 0; i < strlen(key); ++i) {
            if (!((key[i] >= '0' && key[i] <= '9') ||
                  (key[i] >= 'A' && key[i] <= 'F') ||
                  (key[i] >= 'a' && key[i] <= 'f'))) {
                // Not hex character
                VTSS_TRACE(DEBUG) << "Not hex character.";
                return VTSS_APPL_FRR_RIP_ERROR_AUTH_KEY_INVALID;
            }
        }
    } else {
        // For the unencrypted password, only printable characters are accepted
        // and space character is not allowed in FRR layer.
        for (size_t i = 0; i < strlen(key); ++i) {
            if (key[i] == ' ' || key[i] < 32 || key[i] > 126) {
                VTSS_TRACE(DEBUG) << "Not printable or space character.";
                return VTSS_APPL_FRR_RIP_ERROR_AUTH_KEY_INVALID;
            }
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Set the authentication configuration in the specific interface.
 * \param ifindex   [IN] The index of VLAN interface.
 * \param orig_conf [IN] The original authentication configuration.
 * \param conf      [IN] The authentication configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_RIP_ERROR_KEY_CHAIN_PWD_COEXISTS means the key chain and
 *  simple password can not be set at the same time.
 *  VTSS_APPL_FRR_RIP_ERROR_AUTH_KEY_INVALID means the key is not a valid
 *  key for AES256.
 */
static mesa_rc RIP_intf_auth_conf_set(
    const vtss_ifindex_t ifindex,
    const vtss_appl_rip_intf_conf_t *const orig_conf,
    const vtss_appl_rip_intf_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    // Check format of the input 'conf->simple_pwd'
    auto rc = frr_rip_validate_secret_key(conf->is_encrypted, conf->simple_pwd);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    // Check the original key conf format is plain text
    if (orig_conf->is_encrypted) {
        VTSS_TRACE(WARNING)
                << "Get the wrong (encrypted) format of the original "
                "simple password string. (ifindex = "
                << ifindex << ", string = " << conf->simple_pwd
                << ", rc = " << rc << ")";
        // should not reach here
        return FRR_RC_INVALID_ARGUMENT;
    }

    // Check simple pwd and md5 key chain name can not be set at the same time.
    if (strlen(conf->simple_pwd) && strlen(conf->md5_key_chain_name)) {
        VTSS_TRACE(DEBUG) << "Can not set key chain name '"
                          << conf->md5_key_chain_name << "' and simple pwd '"
                          << conf->simple_pwd << "' at the same time"
                          << "(ifindex = " << ifindex << ")";
        return VTSS_APPL_FRR_RIP_ERROR_KEY_CHAIN_PWD_COEXISTS;
    }

    // If the input password format is encrypted, decrypt it.
    char plain_txt[VTSS_APPL_RIP_AUTH_SIMPLE_KEY_MAX_LEN + 1] = "";
    if (conf->is_encrypted) {
        mesa_rc rc = frr_util_secret_key_cryptography(
                         false /* decrypt */, conf->simple_pwd,
                         // The length must INCLUDE terminal character.
                         VTSS_APPL_RIP_AUTH_SIMPLE_KEY_MAX_LEN + 1, plain_txt);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Parameter 'encrypted_key' is invalid format";
            return rc;
        }
    } else {
        strncpy(plain_txt, conf->simple_pwd, sizeof(plain_txt) - 1);
    }

    plain_txt[sizeof(plain_txt) - 1] = '\0';

    /* Apply new configuration to FRR layer */
    // Set authentication mode
    if (conf->auth_type != orig_conf->auth_type) {
        VTSS_TRACE(DEBUG) << "Set mode = " << conf->auth_type;
        FrrRipIfAuthMode mode = RIP_auth_type_mapping(conf->auth_type);
        auto rc = frr_rip_if_authentication_mode_conf_set(ifindex, mode);

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set intf authentication "
                    "mode. (ifindex = "
                    << ifindex << ", mode = " << conf->auth_type
                    << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Set simple pwd and MD5 key chain name
    if (strcmp(conf->md5_key_chain_name, orig_conf->md5_key_chain_name) ||
        strcmp(plain_txt, orig_conf->simple_pwd)) {
        // Delete simple pwd and MD5 key chain name
        auto rc = frr_rip_if_authentication_simple_pwd_set(ifindex, plain_txt,
                                                           true /* delete */);
        rc = frr_rip_if_authentication_key_chain_set(
                 ifindex, conf->md5_key_chain_name, true /* delete */);

        // Set simple pwd and MD5 key chain name
        if (strlen(plain_txt)) {
            VTSS_TRACE(DEBUG) << "Set sim pwd = " << plain_txt
                              << "len=" << strlen(plain_txt);
            rc = frr_rip_if_authentication_simple_pwd_set(ifindex, plain_txt,
                                                          false /* set */);

            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Access framework failed: Set intf authentication "
                        "simple password string. (ifindex = "
                        << ifindex << ", string = " << conf->simple_pwd
                        << ", rc = " << rc << ")";
                return FRR_RC_INTERNAL_ACCESS;
            }
        }

        if (strlen(conf->md5_key_chain_name)) {
            VTSS_TRACE(DEBUG)
                    << "Set key chain name = " << conf->md5_key_chain_name;
            auto rc = frr_rip_if_authentication_key_chain_set(
                          ifindex, conf->md5_key_chain_name, false /* set */);

            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Access framework failed: Set intf authentication "
                        "key chain profile name. (ifindex = "
                        << ifindex << ", name = " << conf->md5_key_chain_name
                        << ", rc = " << rc << ")";
                return FRR_RC_INTERNAL_ACCESS;
            }
        }
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP interface configuration
//----------------------------------------------------------------------------
static mesa_rc RIP_intf_conf_get(const vtss_ifindex_t ifindex,
                                 vtss_appl_rip_intf_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the interface exists or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        T_DG(FRR_TRACE_GRP_RIP, "Interface %s does not exist", ifindex);
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    T_DG(FRR_TRACE_GRP_RIP, "ifindex = %s", ifindex);

    /* Get data from FRR layer */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    T_DG(FRR_TRACE_GRP_RIP, "running-config = %s", frr_running_conf.c_str());

    /* Get configuration from FRR running config */
    // Version
    auto frr_rip_intf_ver = frr_rip_intf_ver_conf_get(frr_running_conf, ifindex);
    if (frr_rip_intf_ver.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface version. "
                          "(ifindex = "
                          << ifindex << ", rc = " << frr_rip_intf_ver.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    FrrRipVer send_ver = frr_rip_intf_ver->send_ver.get();
    conf->send_ver =
        (send_ver == FrrRipVer_1
         ? VTSS_APPL_RIP_INTF_SEND_VER_1
         : send_ver == FrrRipVer_2
         ? VTSS_APPL_RIP_INTF_SEND_VER_2
         : send_ver == FrrRipVer_Both
         ? VTSS_APPL_RIP_INTF_SEND_VER_BOTH
         : VTSS_APPL_RIP_INTF_SEND_VER_NOT_SPECIFIED);

    FrrRipVer recv_ver = frr_rip_intf_ver->recv_ver.get();
    conf->recv_ver =
        (recv_ver == FrrRipVer_None
         ? VTSS_APPL_RIP_INTF_RECV_VER_NONE
         : recv_ver == FrrRipVer_1
         ? VTSS_APPL_RIP_INTF_RECV_VER_1
         : recv_ver == FrrRipVer_2
         ? VTSS_APPL_RIP_INTF_RECV_VER_2
         : recv_ver == FrrRipVer_Both
         ? VTSS_APPL_RIP_INTF_RECV_VER_BOTH
         : VTSS_APPL_RIP_INTF_RECV_VER_NOT_SPECIFIED);

    // Split horizon
    auto split_horizon_mode = frr_rip_if_split_horizon_conf_get(frr_running_conf, ifindex);
    if (split_horizon_mode.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get split horizon "
                          "mode. (ifindex = "
                          << ifindex << ", rc = " << split_horizon_mode.rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    // 'split_horizon_mode.val' can be cast to
    // vtss_appl_rip_split_horizon_mode_t because the values in the two
    // enumerators are totally equal.
    conf->split_horizon_mode = (vtss_appl_rip_split_horizon_mode_t)split_horizon_mode.val;

    // Get authenticaton configuration.
    // According to the conf->is_encrypted value, the simple password
    // (conf->simple_pwd) is output as encrypted or cleartext.
    if (RIP_intf_auth_conf_get(frr_running_conf, ifindex, conf->is_encrypted, conf) != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface "
                          "authentication conf. (ifindex = "
                          << ifindex << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    T_DG(FRR_TRACE_GRP_RIP, "send_ver = %d, recv_ver = %d, split_horizon_mode = %d",
         frr_rip_intf_ver->send_ver.get(),
         frr_rip_intf_ver->recv_ver.get(),
         split_horizon_mode.val);

    return VTSS_RC_OK;
}

/**
 * \brief Get the RIP VLAN interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] The RIP VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_intf_conf_get(const vtss_ifindex_t ifindex,
                                    vtss_appl_rip_intf_conf_t *const conf)
{
    CRIT_SCOPE();


    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    T_DG(FRR_TRACE_GRP_RIP, "Enter, conf = %p", conf);

    // Public API always outputs encrypted key format except for the empty key
    // string.
    conf->is_encrypted = true;
    return RIP_intf_conf_get(ifindex, conf);
}

/**
 * \brief Set the RIP VLAN interface configuration.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] The RIP VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_intf_conf_set(const vtss_ifindex_t ifindex,
                                    const vtss_appl_rip_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->send_ver >= VTSS_APPL_RIP_INTF_SEND_VER_COUNT) {
        VTSS_TRACE(DEBUG) << "Parameter 'send_ver'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->recv_ver >= VTSS_APPL_RIP_INTF_RECV_VER_COUNT) {
        VTSS_TRACE(DEBUG) << "Parameter 'recv_ver'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->split_horizon_mode >= VTSS_APPL_RIP_SPLIT_HORIZON_MODE_COUNT) {
        VTSS_TRACE(DEBUG) << "Parameter 'split_horizon_mode'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the interface exists or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        T_DG(FRR_TRACE_GRP_RIP, "Interface %s does not exist", ifindex);
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get original configuration */
    vtss_appl_rip_intf_conf_t orig_conf = {};
    // Get simple_pwd as plain text to compare it with FRR layer config.
    orig_conf.is_encrypted = false;
    if (RIP_intf_conf_get(ifindex, &orig_conf) != VTSS_RC_OK) {
        VTSS_TRACE(WARNING) << "Get RIP interface current configuraiton failed."
                            << "(ifindex = " << ifindex << ")";
        return VTSS_APPL_FRR_RIP_ERROR_GEN;
    }

    /* Apply the new configuration if needed */
    // Send version
    if (conf->send_ver != orig_conf.send_ver ||
        conf->recv_ver != orig_conf.recv_ver) {
        FrrRipConfIntfVer frr_intf_ver_conf;
        if (conf->send_ver != orig_conf.send_ver) {
            frr_intf_ver_conf.send_ver =
                (conf->send_ver == VTSS_APPL_RIP_INTF_SEND_VER_1
                 ? FrrRipVer_1
                 : conf->send_ver == VTSS_APPL_RIP_INTF_SEND_VER_2
                 ? FrrRipVer_2
                 : conf->send_ver == VTSS_APPL_RIP_INTF_SEND_VER_BOTH
                 ? FrrRipVer_Both
                 : FrrRipVer_NotSpecified);
        }

        if (conf->recv_ver != orig_conf.recv_ver) {
            frr_intf_ver_conf.recv_ver =
                (conf->recv_ver == VTSS_APPL_RIP_INTF_RECV_VER_NONE
                 ? FrrRipVer_None
                 : conf->recv_ver == VTSS_APPL_RIP_INTF_RECV_VER_1
                 ? FrrRipVer_1
                 : conf->recv_ver == VTSS_APPL_RIP_INTF_RECV_VER_2
                 ? FrrRipVer_2
                 : conf->recv_ver == VTSS_APPL_RIP_INTF_RECV_VER_BOTH
                 ? FrrRipVer_Both
                 : FrrRipVer_NotSpecified);
        }

        auto rc = frr_rip_intf_ver_conf_set(ifindex, frr_intf_ver_conf);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set RIP interface version. "
                    "(ifindex = "
                    << ifindex << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Split horizon
    if (conf->split_horizon_mode != orig_conf.split_horizon_mode) {
        // 'conf->split_horizon_mode' can be cast to FrrRipIfSplitHorizonMode
        // because the values in the two enumerators are totally equal.
        auto rc = frr_rip_if_split_horizon_conf_set(
                      ifindex, static_cast<vtss::FrrRipIfSplitHorizonMode>(
                          conf->split_horizon_mode));

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set split horizon "
                    "mode. (ifindex = "
                    << ifindex << ", mode = " << conf->split_horizon_mode
                    << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Authentication
    mesa_rc rc;
    if ((rc = RIP_intf_auth_conf_set(ifindex, &orig_conf, conf)) != VTSS_RC_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all RIP VLAN interfaces.
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_rip_intf_conf_itr(const vtss_ifindex_t *const current_ifindex, vtss_ifindex_t *const next_ifindex)
{
    CRIT_SCOPE();
    return vtss_appl_ip_if_itr(current_ifindex, next_ifindex);
}

/**
 * \brief Get the RIP VLAN interface default configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] The RIP VLAN interface configuration.
 * \return Error code.
 */
mesa_rc frr_rip_intf_conf_def(vtss_ifindex_t *const ifindex,
                              vtss_appl_rip_intf_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);
    conf->send_ver = VTSS_APPL_RIP_INTF_SEND_VER_NOT_SPECIFIED;
    conf->recv_ver = VTSS_APPL_RIP_INTF_RECV_VER_NOT_SPECIFIED;
    conf->split_horizon_mode = VTSS_APPL_RIP_SPLIT_HORIZON_MODE_SIMPLE;
    conf->auth_type = VTSS_APPL_RIP_AUTH_TYPE_NULL;
    conf->is_encrypted = false;
    conf->simple_pwd[0] = conf->md5_key_chain_name[0] = '\0';

    return VTSS_RC_OK;
}

/**
 * \brief Get/Get-Next RIP interface configration by IP address
 *  This API is designed for standard MIB access. The keys of MIB interface
 *  table are IP addr and ifindex.  Notice that the valid configrations are
 *  excluded none IP VLAN interfaces.
 *
 *  For SNMP 'Get-First' operation, set curr_addr = NULL.
 *  For SNMP 'Get' operation, set getnext = false,
 *  For SNMP 'Get-Next' operation, set 'getnext' parameter to TRUE and input a
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
mesa_rc vtss_appl_rip_intf_conf_snmp_get(const mesa_bool_t is_getnext_oper,
                                         const mesa_ipv4_t *const current_addr,
                                         mesa_ipv4_t *const next_addr,
                                         vtss_appl_rip_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be an null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (is_getnext_oper && !next_addr) {
        VTSS_TRACE(ERROR) << "Parameter 'next_addr' cannot be an null pointer"
                          << " for get-next operation";
        return FRR_RC_INVALID_ARGUMENT;
    }

// variable declaration
#define INVALID_CANDIDATE_ADDR 0xffffffff
    vtss_ifindex_t invalid_ifIndex = {0};
    vtss_ifindex_t search_idx = invalid_ifIndex;
    vtss_ifindex_t next_ifindex = invalid_ifIndex;
    vtss_ifindex_t candidate_ifindex = invalid_ifIndex;
    mesa_ipv4_t search_addr = current_addr ? *current_addr : 0;
    mesa_ipv4_t candidate_addr = INVALID_CANDIDATE_ADDR;

    /* Iterate all interfaces */
    while (vtss_appl_ip_if_itr(&search_idx, &next_ifindex) == VTSS_RC_OK) {
        // For next loop
        search_idx = next_ifindex;

        /* Get IP address from ifIndex */
        mesa_ipv4_t ifIndex_addr = 0;
        if (RIP_get_ip_by_ifindex(next_ifindex, &ifIndex_addr) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Get interface ip addr failed."
                              << "(ifindex = " << next_ifindex << ")";
            continue;
        }

        if (!current_addr || is_getnext_oper) {  // Get-First/Get-Next operation
            // Update the candidate ifIndex
            if (ifIndex_addr > search_addr && ifIndex_addr < candidate_addr) {
                candidate_addr = ifIndex_addr;
                candidate_ifindex = next_ifindex;
            }
        } else {  // Get operation
            if (ifIndex_addr == search_addr) {
                candidate_ifindex = next_ifindex;
                break;
            }
        }
    }

    // found entry
    if (candidate_ifindex == invalid_ifIndex) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    // update next address
    if (next_addr) {
        *next_addr = candidate_addr;
    }

    /* Get original configuration */
    if (RIP_intf_conf_get(candidate_ifindex, conf) != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Get RIP interface configuration failed."
                          << "(ifindex = " << candidate_ifindex << ")";
        return VTSS_APPL_FRR_RIP_ERROR_GEN;
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
/* Mapping enum value from FrrRipOffsetListDirection to
 * vtss_appl_rip_offset_direction_t */
static vtss_appl_rip_offset_direction_t RIP_offset_direction_mapping(
    FrrRipOffsetListDirection mode)
{
    switch (mode) {
    case FrrRipOffsetListDirection_In:
        return VTSS_APPL_RIP_OFFSET_DIRECTION_IN;
    case FrrRipOffsetListDirection_Out:
        return VTSS_APPL_RIP_OFFSET_DIRECTION_OUT;
    case FrrRipOffsetListDirection_End:
        break;

        /* Ignore the default case in order to catch the compile warning
         * for the missing mapping case. */
    }

    return VTSS_APPL_RIP_OFFSET_DIRECTION_COUNT;
}

/* Mapping enum value from vtss_appl_rip_offset_direction_t to
 * FrrRipOffsetListDirection */
static FrrRipOffsetListDirection RIP_frr_offset_mode_mapping(
    vtss_appl_rip_offset_direction_t direction)
{
    switch (direction) {
    case VTSS_APPL_RIP_OFFSET_DIRECTION_IN:
        return FrrRipOffsetListDirection_In;
    case VTSS_APPL_RIP_OFFSET_DIRECTION_OUT:
        return FrrRipOffsetListDirection_Out;
    case VTSS_APPL_RIP_OFFSET_DIRECTION_COUNT:
        break;

        /* Ignore the default case in order to catch the compile warning
         * for the missing mapping case. */
    }

    return FrrRipOffsetListDirection_End;
}

static mesa_rc RIP_offset_conf_get(const vtss_ifindex_t ifindex,
                                   const vtss_appl_rip_offset_direction_t direction,
                                   vtss_appl_rip_offset_entry_data_t *const entry,
                                   uint32_t *const total_cnt)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!entry) {
        VTSS_TRACE(ERROR) << "Parameter 'entry' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!ifindex_is_none(ifindex) && !vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!total_cnt) {
        VTSS_TRACE(ERROR) << "Parameter 'total_cnt' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    } else {
        *total_cnt = 0;  // Given an initial value
    }

    /* Get running-config output from FRR layer */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    /* Get offset-list configuration from running-config output */
    auto frr_offset_list_conf = frr_rip_offset_list_conf_get_map(frr_running_conf);
    *total_cnt = frr_offset_list_conf.size();
    if (frr_offset_list_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty offset-list";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the next available entry */
    FrrRipOffsetListKey frr_offset_key = {};
    frr_offset_key.ifindex = ifindex;
    frr_offset_key.mode = RIP_frr_offset_mode_mapping(direction);

    vtss::FrrRipOffsetListMap::iterator itr;
    itr = frr_offset_list_conf.find(frr_offset_key);
    if (itr != frr_offset_list_conf.end()) {  // Found it
        strcpy(entry->name.name, itr->second.name.c_str());
        entry->offset_metric = itr->second.metric;
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get the RIP offset-list configuration.
 * \param ifindex   [IN]  The interface index of the offset-list entry
 * \param direction [IN]  The direction of the offset-list entry
 * \param entry     [OUT] The entry data of the offset-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_get(
    const vtss_ifindex_t ifindex,
    const vtss_appl_rip_offset_direction_t direction,
    vtss_appl_rip_offset_entry_data_t *const entry)
{
    CRIT_SCOPE();

    uint32_t total_cnt;
    return RIP_offset_conf_get(ifindex, direction, entry, &total_cnt);
}
/**
 * \brief Set the RIP offset-list configuration.
 * \param ifindex   [IN]  The interface index of the offset-list entry
 * \param direction [IN]  The direction of the offset-list entry
 * \param entry     [IN]  The entry data of the offset-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_set(
    const vtss_ifindex_t ifindex,
    const vtss_appl_rip_offset_direction_t direction,
    const vtss_appl_rip_offset_entry_data_t *const entry)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!entry) {
        VTSS_TRACE(ERROR) << "Parameter 'entry' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!ifindex_is_none(ifindex) && !vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!strlen(entry->name.name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' length is 0";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!ROUTER_name_is_valid(entry->name.name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (entry->offset_metric > VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'offset' great than the maximum value";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (direction >= VTSS_APPL_RIP_OFFSET_DIRECTION_COUNT) {
        VTSS_TRACE(DEBUG) << "Parameter 'direction'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if entry is existing or not */
    vtss_appl_rip_offset_entry_data_t dummy_entry;
    uint32_t total_cnt;
    mesa_rc rc =
        RIP_offset_conf_get(ifindex, direction, &dummy_entry, &total_cnt);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply the new configuration */
    FrrRipOffsetList frr_offset_list_conf = {};
    frr_offset_list_conf.name.append(entry->name.name);
    frr_offset_list_conf.mode = RIP_frr_offset_mode_mapping(direction);
    frr_offset_list_conf.metric = entry->offset_metric;
    if (!ifindex_is_none(ifindex)) {
        frr_offset_list_conf.ifindex = ifindex;
    }

    auto frr_rc = frr_rip_offset_list_conf_set(frr_offset_list_conf);
    if (frr_rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set RIP offset-list. "
                          << ", rc = " << frr_rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Add/Set the RIP offset-list configuration.
 * \param ifindex   [IN]  The interface index of the offset-list entry
 * \param direction [IN]  The direction of the offset-list entry
 * \param entry     [IN]  The entry data of the offset-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_add(
    const vtss_ifindex_t ifindex,
    const vtss_appl_rip_offset_direction_t direction,
    const vtss_appl_rip_offset_entry_data_t *const entry)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!entry) {
        VTSS_TRACE(ERROR) << "Parameter 'entry' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!ifindex_is_none(ifindex) && !vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!strlen(entry->name.name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' length is 0";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!ROUTER_name_is_valid(entry->name.name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (entry->offset_metric > VTSS_APPL_RIP_REDIST_SPECIFIC_METRIC_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'offset' great than the maximum value";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (direction >= VTSS_APPL_RIP_OFFSET_DIRECTION_COUNT) {
        VTSS_TRACE(DEBUG) << "Parameter 'direction'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if entry is existing or not */
    vtss_appl_rip_offset_entry_data_t dummy_entry;
    uint32_t total_cnt;
    if (RIP_offset_conf_get(ifindex, direction, &dummy_entry, &total_cnt) ==
        VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    }

    if (total_cnt == VTSS_APPL_RIP_OFFSET_LIST_MAX_COUNT) {
        return FRR_RC_LIMIT_REACHED;
    }

    /* Apply the new configuration */
    FrrRipOffsetList frr_offset_list_conf = {};
    frr_offset_list_conf.name.append(entry->name.name);
    frr_offset_list_conf.mode = RIP_frr_offset_mode_mapping(direction);
    frr_offset_list_conf.metric = entry->offset_metric;
    if (!ifindex_is_none(ifindex)) {
        frr_offset_list_conf.ifindex = ifindex;
    }

    auto rc = frr_rip_offset_list_conf_set(frr_offset_list_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set RIP offset-list. "
                          << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get all entries of the RIP offset-list.
 * \param conf [OUT] An container with all offset-list entries.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_get_all(vtss::Vector<vtss::Pair<vtss_appl_rip_offset_entry_key_t, vtss_appl_rip_offset_entry_data_t>> &conf)
{
    CRIT_SCOPE();

    /* Get running-config output from FRR layer */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    /* Get offset-list configuration from running-config output */
    auto frr_offset_list_conf = frr_rip_offset_list_conf_get_map(frr_running_conf);
    for (const auto &itr : frr_offset_list_conf) {
        vtss::Pair<vtss_appl_rip_offset_entry_key_t, vtss_appl_rip_offset_entry_data_t> entry_conf;
        entry_conf.first.ifindex = itr.first.ifindex;
        entry_conf.first.direction = RIP_offset_direction_mapping(itr.first.mode);
        strcpy(entry_conf.second.name.name, itr.second.name.c_str());
        entry_conf.second.offset_metric = itr.second.metric;
        conf.emplace_back(std::move(entry_conf));
    }

    return VTSS_RC_OK;
}

/**
 * \brief Delete the RIP offset-list configuration.
 * \param ifindex   [IN]  The interface index of the offset-list entry
 * \param direction [IN]  The direction of the offset-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_del(
    const vtss_ifindex_t ifindex,
    const vtss_appl_rip_offset_direction_t direction)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!ifindex_is_none(ifindex) && !vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Silent return if the entry not found */
    vtss_appl_rip_offset_entry_data_t existing_entry;
    uint32_t total_cnt;
    mesa_rc rc =
        RIP_offset_conf_get(ifindex, direction, &existing_entry, &total_cnt);
    if (rc == FRR_RC_ENTRY_NOT_FOUND) {
        return VTSS_RC_OK;
    } else if (rc) {
        return rc;
    }

    /* Apply the new configuration */
    FrrRipOffsetList frr_offset_list_conf = {};
    frr_offset_list_conf.name.append(existing_entry.name.name);
    frr_offset_list_conf.mode = RIP_frr_offset_mode_mapping(direction);
    frr_offset_list_conf.metric = existing_entry.offset_metric;
    if (!ifindex_is_none(ifindex)) {
        frr_offset_list_conf.ifindex = ifindex;
    }

    auto frr_rc = frr_rip_offset_list_conf_del(frr_offset_list_conf);
    if (frr_rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete RIP offset-list. "
                          << ", rc = " << frr_rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through RIP offset-list.
 * \param current_ifindex   [IN]  Current interface index
 * \param next_ifindex      [OUT] Next interface index
 * \param current_direction [IN]  Current direction
 * \param next_direction    [OUT] Next direction
 * \return Error code.
 */
mesa_rc vtss_appl_rip_offset_list_conf_itr(
    const vtss_ifindex_t *const current_ifindex,
    vtss_ifindex_t *const next_ifindex,
    const vtss_appl_rip_offset_direction_t *const current_direction,
    vtss_appl_rip_offset_direction_t *const next_direction)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_ifindex || !next_direction) {
        VTSS_TRACE(ERROR) << "Parameter 'next_ifindex' or 'next_direction' "
                          "cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get running-config output from FRR layer */
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    /* Get offset-list configuration from running-config output */
    auto frr_offset_list_conf = frr_rip_offset_list_conf_get_map(frr_running_conf);
    if (frr_offset_list_conf.empty()) {
        T_DG(FRR_TRACE_GRP_RIP, "Empty offset-list");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the next available entry */
    vtss::FrrRipOffsetListMap::iterator itr;
    FrrRipOffsetListKey frr_offset_key = {};
    if (!current_ifindex) {  // Get-First operation
        itr = frr_offset_list_conf.begin();
    } else {  // Get-Next operation
        frr_offset_key.ifindex = *current_ifindex;
        if (current_direction) {
            frr_offset_key.mode = RIP_frr_offset_mode_mapping(*current_direction);
            VTSS_TRACE(DEBUG) << "Invoke greater_than() ";
            itr = frr_offset_list_conf.greater_than(frr_offset_key);
        } else {
            VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal()";
            itr = frr_offset_list_conf.greater_than_or_equal(frr_offset_key);
        }
    }

    if (itr != frr_offset_list_conf.end()) {  // Found it
        *next_ifindex = itr->first.ifindex;
        *next_direction = RIP_offset_direction_mapping(itr->first.mode);
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
/**
 * \brief Get the RIP general status.
 * \param status [OUT] Status for RIP.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_general_status_get(
    vtss_appl_rip_general_status_t *const status)
{
    CRIT_SCOPE();

    vtss_clear(*status);

    auto res = frr_rip_general_status_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "RIP router mode is disabled.";
        status->is_enabled = false;
        return VTSS_RC_OK;
    } else {
        status->is_enabled = true;
        status->timers.update_timer = res->updateTimer;
        status->timers.invalid_timer = res->invalidTimer;
        status->timers.garbage_collection_timer = res->garbageTimer;
        status->next_update_time = res->updateRemainTime;
        status->default_metric = res->default_metric;
        if (res->sendVer == FrrRipVer_1 && res->recvVer == FrrRipVer_1) {
            status->version = VTSS_APPL_RIP_GLOBAL_VER_1;
        } else if (res->sendVer == FrrRipVer_2 && res->recvVer == FrrRipVer_2) {
            status->version = VTSS_APPL_RIP_GLOBAL_VER_2;
        } else if (res->sendVer == FrrRipVer_2 && res->recvVer == FrrRipVer_Both) {
            status->version = VTSS_APPL_RIP_GLOBAL_VER_DEFAULT;
        } else {
            status->version = VTSS_APPL_RIP_GLOBAL_VER_DEFAULT;
        }

        status->admin_distance = res->default_distance;
        status->global_route_changes = res->globalRouteChanges;
        status->global_queries = res->globalQueries;
    }

    /* Get Redistribute protocol config*/
    std::string frr_running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_running_conf));

    Vector<FrrRipRouterRedistribute> frr_redist_conf = vtss::frr_rip_router_redistribute_conf_get(frr_running_conf);
    for (const auto &itr : frr_redist_conf) {
        switch (itr.protocol) {
        case VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED:
            status->redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED] = true;
            break;

        case VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC:
            status->redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC] = true;
            break;

        case VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF:
            status->redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF] = true;
            break;

        default:
            break;
        }
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
/* Mapping enum value from FrrRipVer to vtss_appl_rip_intf_send_ver_t */
static vtss_appl_rip_intf_send_ver_t RIP_if_send_ver_mapping(FrrRipVer frrIfVer)
{
    switch (frrIfVer) {
    case FrrRipVer_1:
        return VTSS_APPL_RIP_INTF_SEND_VER_1;
    case FrrRipVer_2:
        return VTSS_APPL_RIP_INTF_SEND_VER_2;
    case FrrRipVer_Both:
        return VTSS_APPL_RIP_INTF_SEND_VER_BOTH;
    case FrrRipVer_None:
        return VTSS_APPL_RIP_INTF_SEND_VER_COUNT;
    case FrrRipVer_NotSpecified:
        return VTSS_APPL_RIP_INTF_SEND_VER_NOT_SPECIFIED;
    case FrrRipVer_End:
        break;

        /* Ignore the default case in order to catch the compile warning
         * for the missing mapping case. */
    }

    return VTSS_APPL_RIP_INTF_SEND_VER_COUNT;
}

/* Mapping enum value from FrrRipVer to vtss_appl_rip_intf_recv_ver_t */
static vtss_appl_rip_intf_recv_ver_t RIP_if_recv_ver_mapping(FrrRipVer frrIfVer)
{
    switch (frrIfVer) {
    case FrrRipVer_None:
        return VTSS_APPL_RIP_INTF_RECV_VER_NONE;
    case FrrRipVer_1:
        return VTSS_APPL_RIP_INTF_RECV_VER_1;
    case FrrRipVer_2:
        return VTSS_APPL_RIP_INTF_RECV_VER_2;
    case FrrRipVer_Both:
        return VTSS_APPL_RIP_INTF_RECV_VER_BOTH;
    case FrrRipVer_NotSpecified:
        return VTSS_APPL_RIP_INTF_RECV_VER_NOT_SPECIFIED;
    case FrrRipVer_End:
        break;

        /* Ignore the default case in order to catch the compile warning
         * for the missing mapping case. */
    }

    return VTSS_APPL_RIP_INTF_RECV_VER_COUNT;
}

/* Convert the data structure from FRR access to application layer. */
static void RIP_interface_convert_to_appl(
    vtss_appl_rip_interface_status_t *const appl_data,
    const FrrRipActiveIfStatus &access_data)
{
    appl_data->send_version = RIP_if_send_ver_mapping(access_data.sendVer);
    appl_data->recv_version = RIP_if_recv_ver_mapping(access_data.recvVer);
    // Frr always enable triggered update
    appl_data->triggered_update = true;
    appl_data->is_passive_intf = access_data.is_passive_intf;
    strncpy(appl_data->key_chain, access_data.key_chain.c_str(),
            sizeof(appl_data->key_chain) - 1);
    appl_data->auth_type =
        static_cast<vtss_appl_rip_auth_type_t>(access_data.auth_type);
    appl_data->key_chain[sizeof(appl_data->key_chain) - 1] = '\0';
    appl_data->recv_badpackets = access_data.ifStatRcvBadPackets;
    appl_data->recv_badroutes = access_data.ifStatRcvBadRoutes;
    appl_data->sent_updates = access_data.ifStatSentUpdates;
}

/**
 * \brief Iterator through the interface in the RIP
 *
 * \param prev      [IN]    Ifindex to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according
 * to the given IN value.
 *                          Currently CLI and Web use this iterator.
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_rip_intf_status_ifindex_itr(const vtss_ifindex_t *const prev,
                                              vtss_ifindex_t *const next)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next) {
        VTSS_TRACE(ERROR) << "Parameter 'next' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto result_list = frr_rip_interface_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface status. "
                          "(rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (result_list->empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Start the searching process */
    FrrRipActiveIfStatusMap::iterator itr;
    vtss_ifindex_t if_key = {0};
    if (prev) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() ";
        if_key = *prev;
        itr = result_list->greater_than(if_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal()";
        itr = result_list->greater_than_or_equal(if_key);
    }

    if (itr == result_list->end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Found, assign the new key */
    *next = itr->first;

    return VTSS_RC_OK;
}

/**
 * \brief Get status for a specific interface.
 * \param ifindex   [IN] Ifindex to query.
 * \param status    [OUT] Status for 'key'.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_intf_status_get(
    const vtss_ifindex_t ifindex,
    vtss_appl_rip_interface_status_t *const status)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!status) {
        VTSS_TRACE(ERROR) << "Parameter 'data' cannot be null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto res = frr_rip_interface_status_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get RIP interface table. "
                << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (!res->size()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the specific entry */
    auto entry = res->find(ifindex);
    if (entry == res->end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the data structure from FRR access to application layer. */
    RIP_interface_convert_to_appl(status, entry->second);

    return VTSS_RC_OK;
}

/**
 * \brief Get status for all interfaces.
 * \param intf [IN] An empty container.
 * \param intf [OUT] An container with all interface status.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_interface_status_get_all(
    vtss::Vector<vtss::Pair<vtss_ifindex_t, vtss_appl_rip_interface_status_t>>
    &intf)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto res = frr_rip_interface_status_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get RIP interface status. "
                << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Iterate all FRR entries */
    for (const auto &itr : res.val) {
        vtss_ifindex_t key;
        vtss_appl_rip_interface_status_t data;

        /* Convert the data structure from FRR access to application layer. */
        key = itr.first;

        RIP_interface_convert_to_appl(&data, itr.second);
        intf.emplace_back(
            vtss::Pair<vtss_ifindex_t, vtss_appl_rip_interface_status_t>(
                std::move(key), std::move(data)));
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterator through the RIP active interfaces by IP address
 *  This API is designed for standard MIB access. The keys of MIB interface
 * table are IP addr
 * and ifindex.
 *
 * For SNMP 'Get-First' operation, set curr_addr = NULL.
 * For SNMP 'Get' operation, set getnext = false,
 * FOr SNMP 'Get-Next' operation, set 'getnext' parameter to TRUE and input a
 * specific IP adddress
 *
 * \param is_getnext_oper [IN]   get-next operation
 * \param current_addr    [IN]   Current interface address.
 * \param next_addr       [OUT]  Next interface address. The field is
 * significant only when for the get-next operation.
 * \param status          [OUT]  active interface status
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_rip_intf_status_snmp_get(
    const mesa_bool_t is_getnext_oper,
    const mesa_ipv4_t *const current_addr, mesa_ipv4_t *const next_addr,
    vtss_appl_rip_interface_status_t *const status)
{
    CRIT_SCOPE();

    if (!status) {
        VTSS_TRACE(ERROR) << "Parameter 'status' cannot be an null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    }

#define INVALID_CANDIDATE_ADDR 0xffffffff
    mesa_ipv4_t candidate_addr =
        INVALID_CANDIDATE_ADDR;  // init with maximun address
    vtss_ifindex_t invalid_ifIndex = {0};
    vtss_ifindex_t candidate_ifindex = invalid_ifIndex;

    mesa_ipv4_t search_addr = current_addr ? *current_addr : 0;
    mesa_rc rc = VTSS_RC_OK;

    /* Get data from FRR layer */
    auto res = frr_rip_interface_status_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get RIP interface status. "
                << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (!res->size()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Iterate all active interface entries via ifIndex */
    for (const auto &itr : res.val) {
        vtss_ifindex_t ifindex;

        /* Convert the data structure from FRR access to application layer. */
        ifindex = itr.first;

        /* get IP address from ifIndex */
        mesa_ipv4_t ifIndex_addr = 0;
        rc = RIP_get_ip_by_ifindex(ifindex, &ifIndex_addr);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << ifindex << "No Interface IP Address";
            continue;
        }

        if (!current_addr || is_getnext_oper) {  // Get-First/Get-Next operation
            //  Update the candidator address
            if (ifIndex_addr > search_addr && ifIndex_addr < candidate_addr) {
                candidate_addr = ifIndex_addr;
                candidate_ifindex = ifindex;
            }
        } else {  // Get operation
            if (ifIndex_addr == search_addr) {
                candidate_ifindex = ifindex;
                break;
            }
        }
    }

    // found entry
    if (candidate_ifindex != invalid_ifIndex) {
        // update next address
        if (next_addr) {
            *next_addr = candidate_addr;
        }

        // update interface status
        auto entry = res->find(candidate_ifindex);
        if (entry == res->end()) {
            return FRR_RC_ENTRY_NOT_FOUND;
        }

        // Convert the data structure from FRR access to application layer.
        RIP_interface_convert_to_appl(status, entry->second);
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------

/* Convert the data structure from FRR access to application layer. */
static void RIP_peer_convert_to_appl(vtss_appl_rip_peer_data_t *const appl_data,
                                     const FrrRipPeerData &access_data)
{
    appl_data->recv_bad_routes = access_data.recv_bad_routes;
    appl_data->recv_bad_packets = access_data.recv_bad_packets;
    appl_data->last_update_time = access_data.last_update_time.raw32();
    appl_data->rip_version = static_cast<int>(access_data.rip_ver);
}

/**
 * \brief Iterate the peer entry key through the RIP peer table.
 * \param in  [IN]  Pointer to current key. Provide a null pointer to get the
 *                  first entry key.
 * \param out [OUT] Next available entry key.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_peer_itr(const mesa_ipv4_t *const in,
                               mesa_ipv4_t *const out)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!out) {
        VTSS_TRACE(ERROR) << "Parameter 'out' cannot be an null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto res = frr_rip_peer_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get RIP peer table. "
                          << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (!res.val.size()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the next available entry */
    FrrRipPeerMap::iterator itr;
    mesa_ipv4_t peer_key = 0;
    if (in) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() ";
        peer_key = *in;
        itr = res.val.greater_than(peer_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal()";
        itr = res.val.greater_than_or_equal(peer_key);
    }

    if (itr == res.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Found, assign the new key */
    *out = itr->first;
    return VTSS_RC_OK;
}

/**
 * \brief Get a specific entry from the RIP peer table.
 * \param key  [IN]  The entry key.
 * \param data [OUT] The entry data.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_peer_get(const mesa_ipv4_t key,
                               vtss_appl_rip_peer_data_t *const data)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!data) {
        VTSS_TRACE(ERROR) << "Parameter 'data' cannot be null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto res = frr_rip_peer_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get RIP peer table. "
                          << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (!res.val.size()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the specific entry */
    auto entry = res.val.find(key);
    if (entry == res.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the data structure from FRR access to application layer. */
    RIP_peer_convert_to_appl(data, entry->second);

    return VTSS_RC_OK;
}

/**
 * \brief Get all entries of the RIP peer table.
 * \param database [IN]  An empty container.
 * \param database [OUT] An container with all RIP peer entries.
 * \return Error code.
 */

mesa_rc vtss_appl_rip_peer_get_all(
    vtss::Vector<vtss::Pair<mesa_ipv4_t, vtss_appl_rip_peer_data_t>> &database)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto res = frr_rip_peer_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get RIP peer. "
                          << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    vtss_appl_rip_peer_data_t data;
    /* Iterate all FRR entries */
    for (const auto &itr : res.val) {
        /* Convert the data structure from FRR access to application layer. */
        RIP_peer_convert_to_appl(&data, itr.second);
        database.emplace_back(vtss::Pair<mesa_ipv4_t, vtss_appl_rip_peer_data_t>(
                                  std::move(itr.first), std::move(data)));
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------

/* Mapping enum value from FrrRipDbProtoType to vtss_appl_rip_db_proto_type_t */
static vtss_appl_rip_db_proto_type_t RIP_db_proto_type_mapping(
    FrrRipDbProtoType type)
{
    switch (type) {
    case FrrRipDbProtoType_Rip:
        return VTSS_APPL_RIP_DB_PROTO_TYPE_RIP;
    case FrrRipDbProtoType_Connected:
        return VTSS_APPL_RIP_DB_PROTO_TYPE_CONNECTED;
    case FrrRipDbProtoType_Static:
        return VTSS_APPL_RIP_DB_PROTO_TYPE_STATIC;
    case FrrRipDbProtoType_Ospf:
        return VTSS_APPL_RIP_DB_PROTO_TYPE_OSPF;

        /* Ignore the default case in order to catch the compile warning
         * for the missing mapping case. */
    }

    return VTSS_APPL_RIP_DB_PROTO_TYPE_COUNT;
}

/* Mapping enum value from FrrRIpDbProtoSubType to
 * vtss_appl_rip_db_proto_subtype_t */
static vtss_appl_rip_db_proto_subtype_t RIP_db_proto_subtype_mapping(
    FrrRIpDbProtoSubType subtype)
{
    switch (subtype) {
    case FrrRIpDbProtoSubType_Normal:
        return VTSS_APPL_RIP_DB_PROTO_SUBTYPE_NORMAL;
    case FrrRIpDbProtoSubType_Static:
        return VTSS_APPL_RIP_DB_PROTO_SUBTYPE_STATIC;
    case FrrRIpDbProtoSubType_Default:
        return VTSS_APPL_RIP_DB_PROTO_SUBTYPE_DEFAULT;
    case FrrRIpDbProtoSubType_Redistribute:
        return VTSS_APPL_RIP_DB_PROTO_SUBTYPE_REDIST;
    case FrrRIpDbProtoSubType_Interface:
        return VTSS_APPL_RIP_DB_PROTO_SUBTYPE_INTF;

        /* Ignore the default case in order to catch the compile warning
         * for the missing mapping case. */
    }

    return VTSS_APPL_RIP_DB_PROTO_SUBTYPE_COUNT;
}

/* Convert the data structure from FRR access to application layer. */
static void RIP_db_convert_to_appl(vtss_appl_rip_db_data_t *const appl_data,
                                   const FrrRipDbData &access_data)
{
    appl_data->type = RIP_db_proto_type_mapping(access_data.type);
    appl_data->subtype = RIP_db_proto_subtype_mapping(access_data.subtype);
    appl_data->metric = access_data.metric;
    appl_data->external_metric = access_data.external_metric;
    appl_data->self_intf = access_data.self_intf;
    appl_data->src_addr = access_data.src_addr;
    appl_data->tag = access_data.tag;
    appl_data->uptime = access_data.uptime.raw32();
}

/**
 * \brief Iterate the route entry key through the RIP database.
 * \param in  [IN]  Pointer to current key. Provide a null pointer to get the
 *                  first entry key.
 * \param out [OUT] Next available entry key.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_db_itr(const vtss_appl_rip_db_key_t *const in,
                             vtss_appl_rip_db_key_t *const out)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!out) {
        VTSS_TRACE(ERROR) << "Parameter 'out' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto res = frr_rip_db_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get RIP database. "
                          << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (!res.val.size()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the next available entry */
    FrrRipDbMap::iterator itr;
    FrrRipDbKey db_key({{0, 0}, 0});
    if (in) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() ";
        db_key.network = in->network;
        db_key.nexthop = in->nexthop;
        itr = res.val.greater_than(db_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal()";
        itr = res.val.greater_than_or_equal(db_key);
    }

    if (itr == res.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Found, assign the new key */
    out->network = itr->first.network;
    out->nexthop = itr->first.nexthop;

    return VTSS_RC_OK;
}

/**
 * \brief Get specific route entry from the RIP database.
 * \param key  [IN]  The entry key.
 * \param data [OUT] The entry data.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_db_get(const vtss_appl_rip_db_key_t *const key,
                             vtss_appl_rip_db_data_t *const data)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!key || !data) {
        VTSS_TRACE(ERROR) << "Parameter 'key' or 'data' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto res = frr_rip_db_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get RIP database. "
                          << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (!res.val.size()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the specific entry */
    auto entry = res.val.find({key->network, key->nexthop});
    if (entry == res.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the data structure from FRR access to application layer. */
    RIP_db_convert_to_appl(data, entry->second);

    return VTSS_RC_OK;
}

/**
 * \brief Get all entries of the RIP database.
 * \param database [IN]  An empty container.
 * \param database [OUT] An container with all RIP database entries.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_db_get_all(
    vtss::Vector<vtss::Pair<vtss_appl_rip_db_key_t, vtss_appl_rip_db_data_t>>
    &database)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto res = frr_rip_db_get();
    if (res.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get RIP database. "
                          << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Iterate all FRR entries */
    for (const auto &itr : res.val) {
        vtss_appl_rip_db_key_t key;
        vtss_appl_rip_db_data_t data;

        /* Convert the data structure from FRR access to application layer. */
        key.network = itr.first.network;
        key.nexthop = itr.first.nexthop;
        RIP_db_convert_to_appl(&data, itr.second);
        database.emplace_back(
            vtss::Pair<vtss_appl_rip_db_key_t, vtss_appl_rip_db_data_t>(
                key, data));
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP control global options
//----------------------------------------------------------------------------
/**
 * \brief Set RIP control of global options.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc vtss_appl_rip_control_globals(
    const vtss_appl_rip_control_globals_t *const control)
{
    CRIT_SCOPE();

    /* Reload RIP process when it is enabled */
    if (control->reload_process) {
        /* Get original configuration */
        vtss_appl_rip_router_conf_t orig_conf;
        if (RIP_router_conf_get(&orig_conf) != VTSS_RC_OK) {
            VTSS_TRACE(WARNING)
                    << "Get RIP router current configuration failed.";
            return VTSS_APPL_FRR_RIP_ERROR_GEN;
        }

        if (orig_conf.router_mode) {
            /* Apply to FRR layer */
            mesa_rc rc = frr_daemon_reload(FRR_DAEMON_TYPE_RIP);
            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG) << "Reload RIP process failed";
                return rc;
            }
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get RIP control of global options.
 * It is a dummy function for SNMP serialzer only.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc frr_rip_control_globals_dummy_get(
    vtss_appl_rip_control_globals_t *const control)
{
    // Since VTSS basic framework doesn't support 'Set-Only' structure yet,
    // so we return 'not supported' in current implementation.
    return FRR_RC_NOT_SUPPORTED;
}

/******************************************************************************/
/** Module error text (convert the return code to error text)                 */
/******************************************************************************/
const char *frr_rip_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_FRR_RIP_ERROR_GEN:
        return "RIP generic error code";

    case VTSS_APPL_FRR_RIP_ERROR_NOT_UNICAST_ADDRESS:
        return "The address is invalid. Only unicast address is allowed";

    case VTSS_APPL_FRR_RIP_ERROR_AUTH_KEY_INVALID:
        return "The password/key is invalid";

    case VTSS_APPL_FRR_RIP_ERROR_ROUTER_DISABLED:
        return "The RIP router mode is disabled";

    case VTSS_APPL_FRR_RIP_ERROR_INVALID_ADDRESS_FOR_NEIGHBOR_CONNECTION:
        return "Invalid address for neighbor connection";

    case VTSS_APPL_FRR_RIP_ERROR_KEY_CHAIN_PWD_COEXISTS:
        return "Can not set key chain and simple password at the same time";
    }

    return "FRR_RIP: Unknown error code";
}

/******************************************************************************/
/** Module initialization                                                     */
/******************************************************************************/
#if defined(VTSS_SW_OPTION_JSON_RPC)
VTSS_PRE_DECLS void frr_rip_json_init(void);
#endif /* VTSS_SW_OPTION_JSON_RPC */

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
/* Initialize private mib */
VTSS_PRE_DECLS void frr_rip_mib_init(void);
#endif

extern "C" int frr_rip_icli_cmd_register();

/* Initialize module */
mesa_rc frr_rip_init(vtss_init_data_t *data)
{
    mesa_rc rc;

    switch (data->cmd) {
    case INIT_CMD_INIT: {
        VTSS_TRACE(INFO) << "INIT";
        /* Initialize and register semaphore/mutex resources */
        critd_init(&FRR_rip_crit, "frr_rip.crit", VTSS_MODULE_ID_FRR_RIP, CRITD_TYPE_MUTEX);

        RIP_cap_init();

        if (!frr_has_ripd()) {
            T_DG(FRR_TRACE_GRP_RIP, "RIP is not enabled on this platform");
            break;
        }

        T_DG(FRR_TRACE_GRP_RIP, "Initializing");

#if defined(VTSS_SW_OPTION_ICFG)
        /* Initialize and register ICFG resources */
        frr_rip_icfg_init();
#endif /* VTSS_SW_OPTION_ICFG */

#if defined(VTSS_SW_OPTION_JSON_RPC)
        /* Initialize and register JSON resources */
        frr_rip_json_init();
#endif /* VTSS_SW_OPTION_JSON_RPC */

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        /* Initialize and register private MIB resources */
        frr_rip_mib_init();
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */

        /* Initialize and register ICLI resources */
        frr_rip_icli_cmd_register();
        VTSS_TRACE(INFO) << "INIT - completed";
        break;
    }

    case INIT_CMD_START: {
        break;
    }

    case INIT_CMD_CONF_DEF: {
        /* Disable RIP routing processes */
        if (!frr_has_ripd()) {
            break;
        }

        CRIT_SCOPE();
        if ((rc = frr_daemon_stop(FRR_DAEMON_TYPE_RIP)) != VTSS_RC_OK) {
            T_WG(FRR_TRACE_GRP_RIP, "frr_daemon_stop(FRR_DAEMON_TYPE_RIP) failed: %s", error_txt(rc));
        }

        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

