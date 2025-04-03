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
#include "frr_ospf_access.hxx"
#include "frr_ospf_api.hxx"         // For module APIs
#include "frr_ospf_serializer.hxx"  // For module serializer
#include "frr_utils.hxx"
#include "ip_api.h"
#include "ip_utils.hxx"  // For the operator of mesa_ipv4_network_t
#include "main.h"        // For init_cmd_t
#include "vtss/appl/ip.h"
#include "vtss/appl/vlan.h"  // For VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"  // For IteratorComposeN class
#include "vtss/basics/expose/snmp/iterator-compose-depend-N.hxx" /* For IteratorComposeN class xxdadafdaf ffffffffffffffffffffffffff */
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"  // For IteratorComposeN class
#if defined(VTSS_SW_OPTION_ICFG)
#include "frr_ospf_icfg.hxx"  // For module ICFG
#endif                        /* VTSS_SW_OPTION_ICFG */
#include "vtss_os_wrapper.h"  // For vtss_aes256_decrypt(), vtss_aes256_encrypt()
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif /* VTSS_SW_OPTION_SYSLOG */

#include "control_api.h"     // For control_system_reset_register()
#include "icli_api.h"        // For icli_session_printf_to_all()

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_OSPF
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Module semaphore/mutex declaration                                        */
/******************************************************************************/
static critd_t FRR_ospf_crit;

struct FRR_ospf_lock {
    FRR_ospf_lock(int line)
    {
        critd_enter(&FRR_ospf_crit, __FILE__, line);
    }
    ~FRR_ospf_lock()
    {
        critd_exit( &FRR_ospf_crit, __FILE__, 0);
    }
};

/* Semaphore/mutex protection
 * Usage:
 * 1. Every non-static function called `OSPF_xxx` has a CRIT_SCOPE() as the
 *    first thing in the body.
 * 2. No static function has a CRIT_SCOPE()
 * 3. If the non-static functions are not allowed to call non-static functions.
 *   (if needed, then move the functionality to a static function)
 */
#define CRIT_SCOPE() FRR_ospf_lock __lock_guard__(__LINE__)

/* This macro definition is used to make sure the following codes has been
 * protected by semaphore/mutex alreay. In most cases, we use it in the static
 * function. The system will raise an error if the upper layer caller doesn't
 * call CRIT_SCOPE() before calling the API. */
#define FRR_CRIT_ASSERT_LOCKED() \
    critd_assert_locked(&FRR_ospf_crit, __FILE__, __LINE__)

/******************************************************************************/
/** Internal variables and APIs                                               */
/******************************************************************************/
/* The database to store the OSPF routing process state for the specific
 * instance ID.
 *
 * Background:
 * FRR only saves the instance(s) which OSPF routing process is enabled.
 * There are two ways to get the information via FRR VTY commands.
 * ('show running-config' or 'show ip ospf')
 * Both commands need to parsing the output via FRR VTY socket.
 * It can save the processing time if we store the enabled instances in a local
 * database.
 */
static vtss::Set<vtss_appl_ospf_id_t> ospf_enabled_instances;

static mesa_rc OSPF_if_itr(const vtss_ifindex_t *in, vtss_ifindex_t *out)
{
    return vtss_appl_ip_if_itr(in, out, true /* VLAN interfaces, only */);
}

// TODO:
// FRR 2.0 doesn't support multiple OSPF instance ID yet.
// In application layer, the variable of OSPF instance ID is reserved for
// the further usage. Only 1 is accepted for the current stage.
//
/* Check OSPF instance ID is in valid range
 * Return true when it is in valid range. Otherwise, return false.
 */
static bool OSPF_instance_id_valid(const vtss_appl_ospf_id_t id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check valid range */
    if (id < VTSS_APPL_OSPF_INSTANCE_ID_START ||
        id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is out of range. "
                          << "(range from = " << VTSS_APPL_OSPF_INSTANCE_ID_START
                          << "to" << VTSS_APPL_OSPF_INSTANCE_ID_MAX << ")";
        return false;
    }

    return true;
}

/* Check OSPF instance ID parameter is existing or not */
static bool OSPF_instance_id_existing(const vtss_appl_ospf_id_t id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!OSPF_instance_id_valid(id)) {
        return false;
    }

    /* Lookup this entry if already existing */
    auto itr = ospf_enabled_instances.find(id);
    return (itr != ospf_enabled_instances.end()) ? true : false;
}

/* Disable all OSPF routing processes */
static void OSPF_process_disabled(void)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Stop all OSPF routing processes */
    if (frr_daemon_stop(FRR_DAEMON_TYPE_OSPF) == VTSS_RC_OK) {
        /* Clear the local database of OSPF enabled instances */
        ospf_enabled_instances.clear();
    }
}

/**
 * \brief Get the OSPF default instance for clearing OSPF routing process .
 * \param id [OUT] OSPF instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_def(vtss_appl_ospf_id_t *const id)
{
    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF_INSTANCE_ID_START;
    return VTSS_RC_OK;
}

/* Detect if the router ID change take effect or not.
 * return 'true' means that the router ID change will take effect immediately.
 * 'false' means that the router ID change will take effect after restart OSPF
 * process.
 */
static mesa_bool_t OSPF_is_router_id_change_take_effect(
    const vtss_appl_ospf_id_t id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        // Given an error message since it should never happen
        VTSS_TRACE(ERROR) << "Get OSPF status failed";
        return true;
    }

    if (result_list->areas.empty()) {
        VTSS_TRACE(DEBUG) << "Empty area";
        return true;
    }

    /* When there is one or more fully adjacent neighbors in area, the new
     * router
     * ID will take effect after restart OSPF process */
    for (const auto &itr : result_list->areas) {
        const auto &frr_area_status = itr.second;
        if (frr_area_status.full_adjancet_counter) {
            VTSS_TRACE(DEBUG)
                    << "Area " << itr.first << ", full_adjancet_counter: "
                    << frr_area_status.full_adjancet_counter;
            return false;
        }
    }

    return true;
}

/* Detect if the area ID change take effect or not.
 * return 'true' means that the area ID change will take effect immediately.
 * 'false' means that the area ID change will take effect after restart OSPF
 * process.
 */
static mesa_bool_t OSPF_is_area_id_change_take_effect(
    const vtss_appl_ospf_id_t id, const mesa_ipv4_network_t *const network,
    const vtss_appl_ospf_area_id_t area_id, mesa_bool_t is_del_oper)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_interface_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        // Given an error message since it should never happen
        VTSS_TRACE(DEBUG) << "Get OSPF interface status failed";
        return true;
    }

    if (result_list->empty()) {
        VTSS_TRACE(DEBUG) << "Empty interface";
        return true;
    }

    /* Lookup if any existing entry is included in this network segment and have
     * the same area ID. */
    for (const auto &itr : result_list.val) {
        if (vtss_ipv4_net_include(network, &itr.second.net.address)) {
            VTSS_TRACE(DEBUG) << "Found matched interface: " << itr.second.net
                              << ", area ID: " << itr.second.area.area;

            if (is_del_oper && area_id == itr.second.area.area) {
                /* For the deleting operation, it means this area ID is current
                 * be used.
                 * Notice that in FRRv2.0, the current interface area ID won't
                 * be changed event if its reference entry is deleted.
                 */
                return false;
            } else if (!is_del_oper && area_id != itr.second.area.area) {
                /* For the adding operation, it means other area ID is current
                 * be used.
                 * Notice that in FRRv2.0, the current interface area ID won't
                 * be changed event if this current entry is the longest
                 * matching prefix length.
                 */
                return false;
            }
        }
    }

    return true;
}

/* Mapping FrrOspfAuthMode to vtss_appl_ospf_auth_type_t */
static vtss_appl_ospf_auth_type_t frr_ospf_auth_mode_mapping(FrrOspfAuthMode mode)
{
    switch (mode) {
    case FRR_OSPF_AUTH_MODE_AREA_CFG:
        return VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG;

    case FRR_OSPF_AUTH_MODE_NULL:
        return VTSS_APPL_OSPF_AUTH_TYPE_NULL;

    case FRR_OSPF_AUTH_MODE_PWD:
        return VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD;

    case FRR_OSPF_AUTH_MODE_MSG_DIGEST:
        return VTSS_APPL_OSPF_AUTH_TYPE_MD5;

        /* ignore default case to catch compile warning if any prtocol is
         * missing */
    }

    return VTSS_APPL_OSPF_AUTH_TYPE_COUNT;
}

/* Mapping vtss_appl_ospf_auth_type_t to FrrOspfAuthMode */
static FrrOspfAuthMode frr_ospf_auth_type_mapping(vtss_appl_ospf_auth_type_t type)
{
    switch (type) {
    case VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG:
        return FRR_OSPF_AUTH_MODE_AREA_CFG;

    case VTSS_APPL_OSPF_AUTH_TYPE_NULL:
        return FRR_OSPF_AUTH_MODE_NULL;

    case VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD:
        return FRR_OSPF_AUTH_MODE_PWD;

    case VTSS_APPL_OSPF_AUTH_TYPE_MD5:
        return FRR_OSPF_AUTH_MODE_MSG_DIGEST;

    default:
        break;
    }

    return FRR_OSPF_AUTH_MODE_AREA_CFG;
}

/* Mapping xxx to vtss_appl_ospf_route_type_t */
static vtss_appl_ospf_route_type_t frr_ospf_route_type_mapping(
    const FrrOspfRouteType rt_type)
{
    switch (rt_type) {
    case RT_Network:
        return VTSS_APPL_OSPF_ROUTE_TYPE_INTRA_AREA;

    case RT_NetworkIA:
        return VTSS_APPL_OSPF_ROUTE_TYPE_INTER_AREA;

    case RT_Router:
        return VTSS_APPL_OSPF_ROUTE_TYPE_BORDER_ROUTER;

    case RT_ExtNetworkTypeOne:
        return VTSS_APPL_OSPF_ROUTE_TYPE_EXTERNAL_TYPE_1;

    case RT_ExtNetworkTypeTwo:
        return VTSS_APPL_OSPF_ROUTE_TYPE_EXTERNAL_TYPE_2;

    default:  // 'RT_DiscardIA' doens't support
        break;
    }

    return VTSS_APPL_OSPF_ROUTE_TYPE_UNKNOWN;
}

/* Mapping xxx to vtss_appl_ospf_route_type_t */
static FrrOspfRouteType frr_ospf_access_route_type_mapping(
    const vtss_appl_ospf_route_type_t rt_type)
{
    switch (rt_type) {
    case VTSS_APPL_OSPF_ROUTE_TYPE_INTRA_AREA:
        return RT_Network;

    case VTSS_APPL_OSPF_ROUTE_TYPE_INTER_AREA:
        return RT_NetworkIA;

    case VTSS_APPL_OSPF_ROUTE_TYPE_BORDER_ROUTER:
        return RT_Router;

    case VTSS_APPL_OSPF_ROUTE_TYPE_EXTERNAL_TYPE_1:
        return RT_ExtNetworkTypeOne;

    case VTSS_APPL_OSPF_ROUTE_TYPE_EXTERNAL_TYPE_2:
        return RT_ExtNetworkTypeTwo;

    case VTSS_APPL_OSPF_ROUTE_TYPE_UNKNOWN:
    default:
        return RT_DiscardIA;
    }
}

/* Mapping xxx to vtss_appl_ospf_lsdb_type_t */
static vtss_appl_ospf_lsdb_type_t frr_ospf_db_type_mapping(const int32_t lsdb_type)
{
    switch (lsdb_type) {
    case 1:
        return VTSS_APPL_OSPF_LSDB_TYPE_ROUTER;

    case 2:
        return VTSS_APPL_OSPF_LSDB_TYPE_NETWORK;

    case 3:
        return VTSS_APPL_OSPF_LSDB_TYPE_SUMMARY;

    case 4:
        return VTSS_APPL_OSPF_LSDB_TYPE_ASBR_SUMMARY;

    case 5:
        return VTSS_APPL_OSPF_LSDB_TYPE_EXTERNAL;

    case 7:
        return VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL;

    default:  // 'RT_DiscardIA' doens't support
        break;
    }

    return VTSS_APPL_OSPF_LSDB_TYPE_NONE;
}

/* Mapping xxx to vtss_appl_ospf_route_type_t */
static int32_t frr_ospf_access_db_type_mapping(
    const vtss_appl_ospf_lsdb_type_t lsdb_type)
{
    switch (lsdb_type) {
    case VTSS_APPL_OSPF_LSDB_TYPE_ROUTER:
        return 1;

    case VTSS_APPL_OSPF_LSDB_TYPE_NETWORK:
        return 2;

    case VTSS_APPL_OSPF_LSDB_TYPE_SUMMARY:
        return 3;

    case VTSS_APPL_OSPF_LSDB_TYPE_ASBR_SUMMARY:
        return 4;

    case VTSS_APPL_OSPF_LSDB_TYPE_EXTERNAL:
        return 5;

    case VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL:
        return 7;

    case VTSS_APPL_OSPF_LSDB_TYPE_NONE:
    default:
        return 0;
    }
}

/* Mapping xxx to vtss_appl_ospf_route_type_t */
static FrrOspfLsdbType frr_ospf_access_lsdb_type_mapping(
    const vtss_appl_ospf_lsdb_type_t lsdb_type)
{
    switch (lsdb_type) {
    case VTSS_APPL_OSPF_LSDB_TYPE_ROUTER:
        return FrrOspfLsdbType_Router;

    case VTSS_APPL_OSPF_LSDB_TYPE_NETWORK:
        return FrrOspfLsdbType_Network;

    case VTSS_APPL_OSPF_LSDB_TYPE_SUMMARY:
        return FrrOspfLsdbType_Summary;

    case VTSS_APPL_OSPF_LSDB_TYPE_ASBR_SUMMARY:
        return FrrOspfLsdbType_AsbrSummary;

    case VTSS_APPL_OSPF_LSDB_TYPE_EXTERNAL:
        return FrrOspfLsdbType_AsExternal;

    case VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL:
        return FrrOspfLsdbType_Nssa;

    case VTSS_APPL_OSPF_LSDB_TYPE_NONE:
    default:
        return FrrOspfLsdbType_None;
    }
}

/******************************************************************************/
/** Module public header                                                      */
/******************************************************************************/

//------------------------------------------------------------------------------
//** OSPF capabilities
//------------------------------------------------------------------------------
/**
 * \brief Get OSPF capabilities to see what supported or not
 * \param cap [OUT] OSPF capabilities
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_capabilities_get(vtss_appl_ospf_capabilities_t *const cap)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!cap) {
        VTSS_TRACE(ERROR) << "Parameter 'cap' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    cap->instance_id_min = VTSS_APPL_OSPF_INSTANCE_ID_START;
    cap->instance_id_max = VTSS_APPL_OSPF_INSTANCE_ID_MAX;
    cap->router_id_min = VTSS_APPL_OSPF_ROUTER_ID_MIN;
    cap->router_id_max = VTSS_APPL_OSPF_ROUTER_ID_MAX;
    cap->priority_min = VTSS_APPL_OSPF_PRIORITY_MIN;
    cap->priority_max = VTSS_APPL_OSPF_PRIORITY_MAX;
    cap->general_cost_min = VTSS_APPL_OSPF_GENERAL_COST_MIN;
    cap->general_cost_max = VTSS_APPL_OSPF_GENERAL_COST_MAX;
    cap->intf_cost_min = VTSS_APPL_OSPF_INTF_COST_MIN;
    cap->intf_cost_max = VTSS_APPL_OSPF_INTF_COST_MAX;
    cap->redist_cost_min = VTSS_APPL_OSPF_REDIST_COST_MIN;
    cap->redist_cost_max = VTSS_APPL_OSPF_REDIST_COST_MAX;
    cap->hello_interval_min = VTSS_APPL_OSPF_HELLO_INTERVAL_MIN;
    cap->hello_interval_max = VTSS_APPL_OSPF_HELLO_INTERVAL_MAX;
    cap->fast_hello_packets_min = VTSS_APPL_OSPF_FAST_HELLO_MIN;
    cap->fast_hello_packets_max = VTSS_APPL_OSPF_FAST_HELLO_MAX;
    cap->retransmit_interval_min = VTSS_APPL_OSPF_RETRANSMIT_INTERVAL_MIN;
    cap->retransmit_interval_max = VTSS_APPL_OSPF_RETRANSMIT_INTERVAL_MAX;
    cap->dead_interval_min = VTSS_APPL_OSPF_DEAD_INTERVAL_MIN;
    cap->dead_interval_max = VTSS_APPL_OSPF_DEAD_INTERVAL_MAX;
    cap->router_lsa_startup_min = VTSS_APPL_OSPF_ROUTER_LSA_STARTUP_MIN;
    cap->router_lsa_startup_max = VTSS_APPL_OSPF_ROUTER_LSA_STARTUP_MAX;
    cap->router_lsa_shutdown_min = VTSS_APPL_OSPF_ROUTER_LSA_SHUTDOWN_MIN;
    cap->router_lsa_shutdown_max = VTSS_APPL_OSPF_ROUTER_LSA_SHUTDOWN_MAX;
    cap->md_key_id_min = VTSS_APPL_OSPF_AUTH_DIGEST_KEY_ID_MIN;
    cap->md_key_id_max = VTSS_APPL_OSPF_AUTH_DIGEST_KEY_ID_MAX;
    cap->simple_pwd_len_min = VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MIN_LEN;
    cap->simple_pwd_len_max = VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN;
    cap->md_key_len_min = VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MIN_LEN;
    cap->md_key_len_max = VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MAX_LEN;
    cap->rip_redistributed_supported = frr_has_ripd();

    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
//** OSPF instance configuration
//------------------------------------------------------------------------------
/**
 * \brief Add the OSPF instance.
 * \param id [IN] OSPF instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_add(const vtss_appl_ospf_id_t id)
{
    CRIT_SCOPE();

    mesa_rc rc;

    /* Check illegal parameters */
    if (!OSPF_instance_id_valid(id)) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Lookup this entry if existing or not. */
    if (OSPF_instance_id_existing(id)) {
        return VTSS_RC_OK;  // Already existing, do nothing here
    }

    /* Get deferred shutdown timer from OSPF router status.
     * Notice that we have to call the FRR APIs directly since the OSPF
     * process maybe still in progess due to the stub router setting.
     * Make sure the OSPF VTY is ready for the current status access
     * via API frr_daemon_started(FRR_DAEMON_TYPE_OSPF).
     */
    if (frr_daemon_started(FRR_DAEMON_TYPE_OSPF)) {
        auto router_status = frr_ip_ospf_status_get();
        if (router_status.rc == VTSS_RC_OK) {
            if (router_status->deferred_shutdown_time.raw32() > 0) {
                VTSS_TRACE(DEBUG)
                        << "Cannot enable OSPF due to deferred shutdown "
                        "in progress, left "
                        << router_status->deferred_shutdown_time.raw32()
                        << "(ms) remaining ";
#ifdef VTSS_SW_OPTION_SYSLOG
                S_N("Cannot enable OSPF due to deferred shutdown in progress, "
                    "left "
                    "%d(ms) remaining",
                    router_status->deferred_shutdown_time.raw32());
#endif /* VTSS_SW_OPTION_SYSLOG */
                return VTSS_APPL_FRR_OSPF_ERROR_DEFFERED_SHUTDOWN_IN_PROGRESS;
            }
        }
    }

    /* The instance ID doesnot exist. Add it as new one */
    /* Apply to FRR layer */
    rc = frr_ospf_router_conf_set(id);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Add OSPF instance. "
                          "(instance_id = "
                          << id << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Update internal database when the operation is done successfully */
    ospf_enabled_instances.insert(id);

    return VTSS_RC_OK;
}

/**
 * \brief Delete the OSPF instance.
 * \param id [IN] OSPF instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_del(const vtss_appl_ospf_id_t id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF_instance_id_valid(id)) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Lookup this entry if already existing */
    auto itr = vtss::find(ospf_enabled_instances.begin(),
                          ospf_enabled_instances.end(), id);
    if (itr == ospf_enabled_instances.end()) {
        return VTSS_RC_OK;  // Quit silently even if it doesn't exist
    }

    /* Apply to FRR layer */
    mesa_rc rc = frr_ospf_router_conf_del(id);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete OSPF instance. "
                          "(instance_id = "
                          << id << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Update internal database after FRR layer is applied successfully */
    ospf_enabled_instances.erase(itr);

    /* Do not stop ospfd here, maybe some ospf interface configurations exists,
       If we stop , we loss the configurations */

    /* If the stub router on-shutdown is configured, the router will not
     * terminate immediately, here do syslog to indicate the shutdown will
     * be deffered.
     */
    /* Get config from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    /* Get stub router configuration */
    auto stub_router_conf = frr_ospf_router_stub_router_conf_get(running_conf, id);
    if (stub_router_conf.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get stub router configuration. "
                "(instance_id = "
                << id << ", rc = " << stub_router_conf.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }
    /* Do syslog if stub router is configured on shutdown */
    if (stub_router_conf->on_shutdown_interval.valid()) {
#ifdef VTSS_SW_OPTION_SYSLOG
        S_I("Router will be terminated after stub router advertisement for %d"
            "(s)",
            stub_router_conf->on_shutdown_interval.get());
#endif /* VTSS_SW_OPTION_SYSLOG */
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF instance which the OSPF routing process is enabled.
 * \param id [IN] OSPF instance ID.
 * \return Error code.  VTSS_RC_OK means that OSPF routing process is enabled
 *                      on the instance ID.
 *                      VTSS_RC_ERROR means that the instance ID is not created
 *                      and OSPF routing process is disabled.
 */
mesa_rc vtss_appl_ospf_get(const vtss_appl_ospf_id_t id)
{
    CRIT_SCOPE();
    return OSPF_instance_id_existing(id) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

static mesa_rc OSPF_inst_itr(const vtss_appl_ospf_id_t *const current_id,
                             vtss_appl_ospf_id_t *const next_id)
{
    FRR_CRIT_ASSERT_LOCKED();

    vtss::Set<vtss_appl_ospf_id_t>::iterator itr;

    if (current_id) {
        itr = ospf_enabled_instances.greater_than(*current_id);
    } else {
        itr = ospf_enabled_instances.begin();
    }

    if (itr == ospf_enabled_instances.end()) {
        VTSS_TRACE(DEBUG) << "NOT_FOUND";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    VTSS_TRACE(DEBUG) << "Found: " << *itr;
    *next_id = *itr;

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all OSPF instances.
 * \param current_id [IN]   Pointer to the current instance ID. Use null pointer
 *                          to get the first instance ID.
 * \param next_id    [OUT]  Pointer to the next instance ID
 * \return Error code.      VTSS_RC_OK means that the next instance ID is valid
 *                          and the vaule is saved in 'out' paramater.
 *                          VTSS_RC_ERROR means that the next instance ID is
 *                          non-existing.
 */
mesa_rc vtss_appl_ospf_inst_itr(const vtss_appl_ospf_id_t *const current_id,
                                vtss_appl_ospf_id_t *const next_id)
{
    CRIT_SCOPE();
    return OSPF_inst_itr(current_id, next_id);
}

/**
 * \brief Set OSPF control of global options.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_control_globals(
    const vtss_appl_ospf_control_globals_t *const control)
{
    CRIT_SCOPE();

    mesa_rc rc;

    if (control->reload_process && !ospf_enabled_instances.empty()) {
        /* Apply to FRR layer */
        if ((rc = frr_daemon_reload(FRR_DAEMON_TYPE_OSPF)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Reload OSPF process failed";
            return rc;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get OSPF control of global options.
 * It is a dummy function for SNMP serialzer only.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc frr_ospf_control_globals_dummy_get(
    vtss_appl_ospf_control_globals_t *const control)
{
    if (control) {
        vtss_clear(*control);
    }

    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
//** OSPF router configuration/status
//------------------------------------------------------------------------------

/* Notice !!!
 * The command "no router-id" is unsupported in FRR v2.0.
 * To clear the current configured router ID, we need to set value 0 in FRRv2.0
 * command. For example, (config-router)# router-id 0
 */
#define FRR_V2_OSPF_DEFAULT_ROUTER_ID 0

/**
 * \brief Get the OSPF router configuration.
 * \param id   [IN] OSPF instance ID.
 * \param conf [OUT] OSPF router configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_router_conf_def(vtss_appl_ospf_id_t *const id,
                                 vtss_appl_ospf_router_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero initial value below
    conf->router_id.is_specific_id = false;
    conf->router_id.id = VTSS_APPL_OSPF_ROUTER_ID_MIN;
    conf->def_metric = VTSS_APPL_OSPF_REDIST_COST_MIN;
    conf->def_route_conf.metric = VTSS_APPL_OSPF_REDIST_COST_MIN;
    for (u32 idx = 0; idx < VTSS_APPL_OSPF_REDIST_PROTOCOL_COUNT; ++idx) {
        conf->redist_conf[idx].type = VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE;
        conf->redist_conf[idx].metric = VTSS_APPL_OSPF_REDIST_COST_MIN;
    }

    conf->stub_router.on_startup_interval = VTSS_APPL_OSPF_ROUTER_LSA_STARTUP_MIN;
    conf->stub_router.on_shutdown_interval =
        VTSS_APPL_OSPF_ROUTER_LSA_SHUTDOWN_MIN;
    conf->admin_distance = 110;

    return VTSS_RC_OK;
}

static mesa_rc OSPF_router_conf_get(const vtss_appl_ospf_id_t id,
                                    vtss_appl_ospf_router_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();
    vtss_appl_ospf_router_conf_t def_conf;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // Get default configuration
    if (frr_ospf_router_conf_def((vtss_appl_ospf_id_t *)&id, &def_conf) !=
        VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Get OSPF router default configuration failed.";
        return FRR_RC_INTERNAL_ERROR;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    // Get router ID
    auto frr_router_conf = frr_ospf_router_conf_get(running_conf, id);
    if (frr_router_conf.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get router configuration. (instance_id = %u, rc = %s)", id, error_txt(frr_router_conf.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    memset(&conf->router_id, 0, sizeof(conf->router_id));
    if (frr_router_conf->ospf_router_id.valid()) {
        conf->router_id.id = frr_router_conf->ospf_router_id.get();
        conf->router_id.is_specific_id =
            (conf->router_id.id != FRR_V2_OSPF_DEFAULT_ROUTER_ID);
    } else {
        conf->router_id.id = VTSS_APPL_OSPF_ROUTER_ID_MIN;
    }

    // Get default mode of passive-interface
    auto frr_def_passive_mode = frr_ospf_router_passive_if_default_get(running_conf, id);
    if (frr_def_passive_mode.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get default passive-default. (instance_id = %u, rc = %s)", id, error_txt(frr_def_passive_mode.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->default_passive_interface = frr_def_passive_mode.val;

    // Get the default metric
    auto frr_def_metric = frr_ospf_router_default_metric_conf_get(running_conf, id);
    if (frr_def_metric.val.valid()) {
        conf->is_specific_def_metric = true;
        conf->def_metric = frr_def_metric.val.get();
    } else {
        conf->is_specific_def_metric = false;
        conf->def_metric = VTSS_APPL_OSPF_REDIST_COST_MIN;
    }

    // Get the route redistribution
    vtss_clear(conf->redist_conf);
    Vector<FrrOspfRouterRedistribute> frr_redist = frr_ospf_router_redistribute_conf_get(running_conf, id);
    for (const auto &itr : frr_redist) {
        vtss_appl_ospf_redist_conf_t *redist_conf;
        if (itr.protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED) {
            redist_conf = &conf->redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_CONNECTED];
        } else if (itr.protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC) {
            redist_conf = &conf->redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_STATIC];
        } else if (itr.protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_RIP) {
            redist_conf = &conf->redist_conf[VTSS_APPL_OSPF_REDIST_PROTOCOL_RIP];
        } else {
            continue;
        }

        if (itr.metric.valid()) {
            redist_conf->is_specific_metric = true;
            redist_conf->metric = itr.metric.get();
        } else {
            redist_conf->is_specific_metric = false;
            redist_conf->metric = VTSS_APPL_OSPF_REDIST_COST_MIN;
        }
        // default metric-type is type 2
        redist_conf->type = itr.metric_type == MetricType_One ? VTSS_APPL_OSPF_REDIST_METRIC_TYPE_1 : VTSS_APPL_OSPF_REDIST_METRIC_TYPE_2;
    }

    // Get the default route configuration
    vtss_clear(conf->def_route_conf);
    auto frr_def_route_conf =
        frr_ospf_router_default_route_conf_get(running_conf, id);
    if (frr_def_route_conf.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get default route configuration. (instance_id = %u, rc = %s", id, error_txt(frr_def_route_conf.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (frr_def_route_conf.val.always.valid()) {
        conf->def_route_conf.is_always = frr_def_route_conf.val.always.get();
    } else {
        conf->def_route_conf.is_always = false;
    }

    if (frr_def_route_conf.val.metric.valid()) {
        conf->def_route_conf.is_specific_metric = true;
        conf->def_route_conf.metric = frr_def_route_conf.val.metric.get();
    } else {
        conf->def_route_conf.is_specific_metric = false;
        conf->def_route_conf.metric = VTSS_APPL_OSPF_REDIST_COST_MIN;
    }

    if (frr_def_route_conf.val.metric_type.valid()) {
        conf->def_route_conf.type =
            frr_def_route_conf.val.metric_type.get() == MetricType_One
            ? VTSS_APPL_OSPF_REDIST_METRIC_TYPE_1
            : VTSS_APPL_OSPF_REDIST_METRIC_TYPE_2;
    } else {
        conf->def_route_conf.type = VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE;
    }

    // Get the stub router
    vtss_clear(conf->stub_router);
    auto frr_stub_router = frr_ospf_router_stub_router_conf_get(running_conf, id);
    if (frr_stub_router.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get stub router. (instance_id = %d, rc = %s", id, error_txt(frr_stub_router.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->stub_router.is_on_startup = frr_stub_router.val.on_startup_interval.valid();
    if (conf->stub_router.is_on_startup) {
        conf->stub_router.on_startup_interval = frr_stub_router.val.on_startup_interval.get();
    } else {
        conf->stub_router.on_startup_interval = def_conf.stub_router.on_startup_interval;
    }

    conf->stub_router.is_on_shutdown = frr_stub_router.val.on_shutdown_interval.valid();
    if (conf->stub_router.is_on_shutdown) {
        conf->stub_router.on_shutdown_interval = frr_stub_router.val.on_shutdown_interval.get();
    } else {
        conf->stub_router.on_shutdown_interval = def_conf.stub_router.on_shutdown_interval;
    }

    conf->stub_router.is_administrative = frr_stub_router.val.is_administrative;

    // Get the administrative distance
    auto frr_admin_distance = frr_ospf_router_admin_distance_get(running_conf, id);
    if (frr_admin_distance.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get administrative distance. (instance_id = %u, rc = %s)", id, error_txt(frr_admin_distance.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->admin_distance = frr_admin_distance.val;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF router configuration.
 * \param id   [IN] OSPF instance ID.
 * \param conf [OUT] OSPF router configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_conf_get(const vtss_appl_ospf_id_t id,
                                       vtss_appl_ospf_router_conf_t *const conf)
{
    CRIT_SCOPE();
    return OSPF_router_conf_get(id, conf);
}

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
    const vtss_appl_ospf_id_t id,
    const vtss_appl_ospf_router_conf_t *const conf)
{
    CRIT_SCOPE();

    mesa_rc rc;
    vtss_appl_ospf_router_conf_t orig_conf;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->router_id.is_specific_id &&
        (conf->router_id.id < VTSS_APPL_OSPF_ROUTER_ID_MIN ||
         conf->router_id.id > VTSS_APPL_OSPF_ROUTER_ID_MAX)) {
        VTSS_TRACE(DEBUG) << "Parameter 'router ID' is invalid"
                          << conf->router_id.id;
        return VTSS_APPL_FRR_OSPF_ERROR_INVALID_ROUTER_ID;
    }

    if (conf->is_specific_def_metric &&
        (conf->def_metric > VTSS_APPL_OSPF_REDIST_COST_MAX)) {
        VTSS_TRACE(DEBUG) << "Parameter 'def_metric'(" << conf->def_metric
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    for (uint32_t idx = 0; idx < VTSS_APPL_OSPF_REDIST_PROTOCOL_COUNT; idx++) {
        if (conf->redist_conf[idx].is_specific_metric &&
            (conf->redist_conf[idx].metric > VTSS_APPL_OSPF_REDIST_COST_MAX)) {
            T_DG(FRR_TRACE_GRP_OSPF, "Parameter 'metric' (%u) is invalid", conf->redist_conf[idx].metric);
            return FRR_RC_INVALID_ARGUMENT;
        }
    }

    if (conf->def_route_conf.is_specific_metric &&
        (conf->def_route_conf.metric > VTSS_APPL_OSPF_REDIST_COST_MAX)) {
        T_DG(FRR_TRACE_GRP_OSPF, "Parameter 'metric' (%u) is invalid", conf->def_route_conf.metric);
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->stub_router.is_on_startup &&
        (conf->stub_router.on_startup_interval >
         VTSS_APPL_OSPF_ROUTER_LSA_STARTUP_MAX)) {
        VTSS_TRACE(DEBUG) << "Parameter 'on_startup_interval'("
                          << conf->stub_router.on_startup_interval
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->stub_router.is_on_shutdown &&
        (conf->stub_router.on_shutdown_interval >
         VTSS_APPL_OSPF_ROUTER_LSA_SHUTDOWN_MAX)) {
        VTSS_TRACE(DEBUG) << "Parameter 'on_shutdown_interval'("
                          << conf->stub_router.on_shutdown_interval
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->admin_distance &&
        (conf->admin_distance < VTSS_APPL_OSPF_ADMIN_DISTANCE_MIN)) {
        VTSS_TRACE(DEBUG) << "Parameter 'admin_distance'("
                          << conf->admin_distance << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get the original configuration */
    rc = OSPF_router_conf_get(id, &orig_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    if (orig_conf.router_id.is_specific_id != conf->router_id.is_specific_id ||
        (orig_conf.router_id.is_specific_id &&
         orig_conf.router_id.id != conf->router_id.id)) {
        vtss::FrrOspfRouterConf router_conf;
        if (conf->router_id.is_specific_id) {
            router_conf.ospf_router_id = conf->router_id.id;
        } else {
            router_conf.ospf_router_id = FRR_V2_OSPF_DEFAULT_ROUTER_ID;
        }

        rc = frr_ospf_router_conf_set(id, router_conf);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set router configuration. "
                    "(instance_id = "
                    << id << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }

        if (!OSPF_is_router_id_change_take_effect(id)) {
            return VTSS_APPL_FRR_OSPF_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT;
        }
    }

    if (orig_conf.default_passive_interface != conf->default_passive_interface) {
        rc = frr_ospf_router_passive_if_default_set(
                 id, conf->default_passive_interface);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set default passive-interface "
                    "(instance_id = "
                    << id << "default_passive_interface = "
                    << conf->default_passive_interface << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Apply new default metric
    if (orig_conf.is_specific_def_metric != conf->is_specific_def_metric ||
        orig_conf.def_metric != conf->def_metric) {
        if (conf->is_specific_def_metric) {
            rc = frr_ospf_router_default_metric_conf_set(id, conf->def_metric);
        } else {
            rc = frr_ospf_router_default_metric_conf_del(id);
        }

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set default metric "
                              "(instance_id = "
                              << id << "def_metric = " << conf->def_metric
                              << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Apply new route redistribution
    for (uint32_t idx = 0; idx < VTSS_APPL_OSPF_REDIST_PROTOCOL_COUNT; idx++) {
        const vtss_appl_ospf_redist_conf_t *orig_redist_conf = &orig_conf.redist_conf[idx];
        const vtss_appl_ospf_redist_conf_t *redist_conf = &conf->redist_conf[idx];
        if (orig_redist_conf->type == redist_conf->type && orig_redist_conf->is_specific_metric == redist_conf->is_specific_metric && orig_redist_conf->metric == redist_conf->metric) {
            continue;  // Do nothing when the values are the same.
        }

        vtss_appl_ip_route_protocol_t protocol;

        if (idx == VTSS_APPL_OSPF_REDIST_PROTOCOL_CONNECTED) {
            protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED;
        } else  if (idx == VTSS_APPL_OSPF_REDIST_PROTOCOL_STATIC) {
            protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
        } else {
            protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_RIP;
        }

        // Use VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE to delete
        // default route configuration
        if (redist_conf->type == VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE) {
            rc = frr_ospf_router_redistribute_conf_del(id, protocol);
        } else {
            FrrOspfRouterRedistribute frr_redist_conf;
            // default metric-type is type 2
            frr_redist_conf.metric_type =
                redist_conf->type == VTSS_APPL_OSPF_REDIST_METRIC_TYPE_1
                ? MetricType_One
                : MetricType_Two;
            if (redist_conf->is_specific_metric) {
                frr_redist_conf.metric = redist_conf->metric;
            }

            frr_redist_conf.protocol = protocol;
            rc = frr_ospf_router_redistribute_conf_set(id, frr_redist_conf);
        }

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set route redistribution "
                    "(instance_id = "
                    << id << "metric_type = " << redist_conf->type
                    << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Apply new default route configuration
    if (orig_conf.def_route_conf.is_always != conf->def_route_conf.is_always ||
        orig_conf.def_route_conf.is_specific_metric !=
        conf->def_route_conf.is_specific_metric ||
        orig_conf.def_route_conf.metric != conf->def_route_conf.metric ||
        orig_conf.def_route_conf.type != conf->def_route_conf.type) {
        // Use VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE to delete
        // default route configuration
        if (conf->def_route_conf.type == VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE) {
            rc = frr_ospf_router_default_route_conf_del(id);
            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Access framework failed: Delete default route ("
                        << "instance_id = " << id
                        << ", metric_type = " << conf->def_route_conf.type
                        << ", rc = " << rc << ")";
                return FRR_RC_INTERNAL_ACCESS;
            }
        } else {
            FrrOspfRouterDefaultRoute frr_def_route_conf = {};
            frr_def_route_conf.always = conf->def_route_conf.is_always;

            // default metric-type is type 2
            frr_def_route_conf.metric_type =
                conf->def_route_conf.type == VTSS_APPL_OSPF_REDIST_METRIC_TYPE_1
                ? MetricType_One
                : MetricType_Two;
            if (conf->def_route_conf.is_specific_metric) {
                frr_def_route_conf.metric = conf->def_route_conf.metric;
            }

            rc = frr_ospf_router_default_route_conf_set(id, frr_def_route_conf);
            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Access framework failed: Set default route ("
                        << "instance_id = " << id
                        << ", metric_type = " << conf->def_route_conf.type
                        << ", is_specific_metric = "
                        << conf->def_route_conf.is_specific_metric
                        << ", metric = " << conf->def_route_conf.metric
                        << ", is_always = " << conf->def_route_conf.is_always
                        << ", rc = " << rc << ")";
                return FRR_RC_INTERNAL_ACCESS;
            }
        }
    }

    // Apply new stub router configuration
    if ((orig_conf.stub_router.is_on_startup != conf->stub_router.is_on_startup ||
         (conf->stub_router.is_on_startup &&
          orig_conf.stub_router.on_startup_interval !=
          conf->stub_router.on_startup_interval)) ||
        (orig_conf.stub_router.is_on_shutdown != conf->stub_router.is_on_shutdown ||
         (conf->stub_router.is_on_shutdown &&
          orig_conf.stub_router.on_shutdown_interval !=
          conf->stub_router.on_shutdown_interval)) ||
        orig_conf.stub_router.is_administrative !=
        conf->stub_router.is_administrative) {
        FrrOspfRouterStubRouter frr_stub_router_set_conf = {};
        FrrOspfRouterStubRouter frr_stub_router_del_conf = {};

        if (conf->stub_router.is_on_startup) {
            frr_stub_router_set_conf.on_startup_interval =
                conf->stub_router.on_startup_interval;
        } else {
            frr_stub_router_del_conf.on_startup_interval =
                orig_conf.stub_router.on_startup_interval;
        }

        if (conf->stub_router.is_on_shutdown) {
            frr_stub_router_set_conf.on_shutdown_interval =
                conf->stub_router.on_shutdown_interval;
        } else {
            frr_stub_router_del_conf.on_shutdown_interval =
                orig_conf.stub_router.on_shutdown_interval;
        }

        if (conf->stub_router.is_administrative) {
            frr_stub_router_set_conf.is_administrative = true;
        } else {
            frr_stub_router_del_conf.is_administrative = true;
        }

        rc = frr_ospf_router_stub_router_conf_set(id, frr_stub_router_set_conf);
        if (rc == VTSS_RC_OK) {
            rc = frr_ospf_router_stub_router_conf_del(id,
                                                      frr_stub_router_del_conf);
        }

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set stub router "
                    "(instance_id = "
                    << id << "startup = " << conf->stub_router.is_on_startup
                    << "/" << conf->stub_router.on_startup_interval
                    << " shutdown = " << conf->stub_router.is_on_shutdown << "/"
                    << conf->stub_router.on_shutdown_interval
                    << " admin = " << conf->stub_router.is_administrative
                    << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Apply new administrative distance
    if (orig_conf.admin_distance != conf->admin_distance) {
        rc = frr_ospf_router_admin_distance_set(id, conf->admin_distance);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set administrative distance"
                    "(instance_id = "
                    << id << "admin_distance = " << conf->admin_distance
                    << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

static mesa_rc OSPF_router_intf_conf_get(
    const vtss_appl_ospf_id_t id, const vtss_ifindex_t ifindex,
    vtss_appl_ospf_router_intf_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the interface is existing or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_intf_passive_conf = frr_ospf_router_passive_if_conf_get(running_conf, id, ifindex);
    if (frr_intf_passive_conf.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get passive-interface. "
                          "(instance_id = "
                          << id << ", ifindex = " << ifindex
                          << ", rc = " << frr_intf_passive_conf.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->passive_enabled = frr_intf_passive_conf.val;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF router interface configuration.
 * \param id      [IN] OSPF instance ID.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_intf_conf_get(
    const vtss_appl_ospf_id_t id, const vtss_ifindex_t ifindex,
    vtss_appl_ospf_router_intf_conf_t *const conf)
{
    CRIT_SCOPE();
    return OSPF_router_intf_conf_get(id, ifindex, conf);
}

/**
 * \brief Set the OSPF router interface configuration.
 * \param id      [IN] OSPF instance ID.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] OSPF router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_intf_conf_set(
    const vtss_appl_ospf_id_t id, const vtss_ifindex_t ifindex,
    const vtss_appl_ospf_router_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    vtss_appl_ospf_router_intf_conf_t orig_conf;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the interface is existing or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get the original configuration */
    mesa_rc rc = OSPF_router_intf_conf_get(id, ifindex, &orig_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    if (orig_conf.passive_enabled != conf->passive_enabled) {
        rc = frr_ospf_router_passive_if_conf_set(id, ifindex,
                                                 conf->passive_enabled);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework: Set passive-interface. (instance_id "
                    "= "
                    << id << ", ifindex = " << ifindex << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all OSPF router interfaces.
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_intf_conf_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_ifindex_t *const current_ifindex,
    vtss_ifindex_t *const next_ifindex)
{
    CRIT_SCOPE();

    vtss::IteratorComposeN<vtss_appl_ospf_id_t, vtss_ifindex_t> i(&OSPF_inst_itr, &OSPF_if_itr);
    return i(current_id, next_id, current_ifindex, next_ifindex);
}

static mesa_rc OSPF_router_status_get(const vtss_appl_ospf_id_t id,
                                      vtss_appl_ospf_router_status_t *const status)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (!status) {
        return VTSS_RC_ERROR;
    }

    /* Lookup this entry if existing or not. */
    if (!OSPF_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF status. "
                          "(instance_id = "
                          << id << ", rc = " << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    status->ospf_router_id = result_list->router_id;
    status->deferred_shutdown_time = result_list->deferred_shutdown_time.raw32();
    status->spf_delay = result_list->spf_schedule_delay.raw32();
    status->spf_holdtime = result_list->hold_time_min.raw32();
    status->spf_max_waittime = result_list->hold_time_max.raw32();
    status->last_executed_spf_ts = result_list->spf_last_executed.raw32();
    VTSS_TRACE(DEBUG) << "spf_last_executed.raw() = "
                      << result_list->spf_last_executed.raw()
                      << "spf_last_executed.raw32() = "
                      << result_list->spf_last_executed.raw32();
    /* convert msec to sec */
    status->min_lsa_interval = result_list->lsa_min_interval.raw32() / 1000;
    status->min_lsa_arrival = result_list->lsa_min_arrival.raw32();
    status->external_lsa_count = result_list->lsa_external_counter;
    status->external_lsa_checksum = result_list->lsa_external_checksum;
    status->attached_area_count = result_list->attached_area_counter;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF router status.
 * \param id     [IN] OSPF instance ID.
 * \param status [OUT] Status for 'id'.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_router_status_get(
    const vtss_appl_ospf_id_t id,
    vtss_appl_ospf_router_status_t *const status)
{
    CRIT_SCOPE();
    return OSPF_router_status_get(id, status);
}

//------------------------------------------------------------------------------
//** OSPF network area configuration/status
//------------------------------------------------------------------------------
static mesa_rc OSPF_area_conf_get(const vtss_appl_ospf_id_t id,
                                  const mesa_ipv4_network_t *const network,
                                  vtss_appl_ospf_area_id_t *const area_id,
                                  mesa_bool_t check_overlap)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!network || !area_id) {
        VTSS_TRACE(ERROR)
                << "Parameter 'network' & 'area_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_area_conf = frr_ospf_area_network_conf_get(running_conf, id);
    if (frr_area_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty network area";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    for (const auto &itr : frr_area_conf) {
        if (vtss_ipv4_net_mask_out(&itr.net) == vtss_ipv4_net_mask_out(network)) {
            // Found it
            *area_id = itr.area;
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
 * \brief Get the OSPF area configuration.
 * \param id      [IN]  OSPF instance ID.
 * \param network [IN]  OSPF area network.
 * \param area_id [OUT] OSPF area ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_conf_get(const vtss_appl_ospf_id_t id,
                                     const mesa_ipv4_network_t *const network,
                                     vtss_appl_ospf_area_id_t *const area_id)
{
    CRIT_SCOPE();
    return OSPF_area_conf_get(id, network, area_id, false);
}

/**
 * \brief Add/set the OSPF area configuration.
 * \param id      [IN] OSPF instance ID.
 * \param network [IN] OSPF area network.
 * \param area_id [IN] OSPF area ID.
 * \return Error code.
 * VTSS_APPL_FRR_OSPF_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT means that area ID
 * change doesn't take effect.
 */
mesa_rc vtss_appl_ospf_area_conf_add(const vtss_appl_ospf_id_t id,
                                     const mesa_ipv4_network_t *const network,
                                     const vtss_appl_ospf_area_id_t *const area_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!network || !area_id) {
        VTSS_TRACE(ERROR)
                << "Parameter 'network' or 'area_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Lookup this entry if already existing or IP ranage is overlap */
    vtss_appl_ospf_area_id_t orig_area_id;
    mesa_rc rc = OSPF_area_conf_get(id, network, &orig_area_id, true);
    if (rc == FRR_RC_ADDR_RANGE_OVERLAP) {
        // Don't allow IP range is overlapped
        return rc;
    } else if (rc == VTSS_RC_OK && orig_area_id != *area_id) {
        // Don't allow different area ID on the same network
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    }

    /* Apply to FRR layer */
    vtss::FrrOspfAreaNetwork frr_area_conf = {vtss_ipv4_net_mask_out(network),
                                              *area_id
                                             };
    rc = frr_ospf_area_network_conf_set(id, frr_area_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set area ID. "
                          "(instance_id = "
                          << id << ", network_addr = " << *network
                          << ", area_id = " << *area_id << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Detect if the area ID change take effect or not.
     * The detection need to do after applying to FRR layer.
     */
    if (!OSPF_is_area_id_change_take_effect(id, network, *area_id, false)) {
        return VTSS_APPL_FRR_OSPF_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF area default configuration.
 * \param id [OUT] OSPF instance ID.
 * \param network [OUT] OSPF area network.
 * \param area_id [OUT] OSPF area ID.
 * \return Error code.
 */
mesa_rc frr_ospf_area_conf_def(vtss_appl_ospf_id_t *const id,
                               mesa_ipv4_network_t *const network,
                               vtss_appl_ospf_area_id_t *const area_id)
{
    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF_INSTANCE_ID_START;
    return VTSS_RC_OK;
}

/**
 * \brief Delete the OSPF area configuration.
 * \param id      [IN] OSPF instance ID.
 * \param network [IN] OSPF area network.
 * \return Error code.
 * VTSS_APPL_FRR_OSPF_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT means that area ID
 * change doesn't take effect.
 */
mesa_rc vtss_appl_ospf_area_conf_del(const vtss_appl_ospf_id_t id,
                                     const mesa_ipv4_network_t *const network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!network) {
        VTSS_TRACE(ERROR) << "Parameter 'network' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Notice that the area ID is required for the deleting operation of 'ospf
     * network area'.
     * So we need to get the area ID before applying to FRR layer.
     */
    vtss_appl_ospf_area_id_t orig_area_id;
    mesa_rc rc = OSPF_area_conf_get(id, network, &orig_area_id, false);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply to FRR layer */
    vtss::FrrOspfAreaNetwork frr_area_conf = {vtss_ipv4_net_mask_out(network),
                                              orig_area_id
                                             };
    rc = frr_ospf_area_network_conf_del(id, frr_area_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete area ID. "
                          "(instance_id = "
                          << id << ", network_addr = " << *network
                          << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Detect if the area ID change take effect or not. */
    if (!OSPF_is_area_id_change_take_effect(id, network, orig_area_id, true)) {
        return VTSS_APPL_FRR_OSPF_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT;
    }

    return VTSS_RC_OK;
}

static mesa_rc OSPF_area_conf_itr_k2(const mesa_ipv4_network_t *const cur,
                                     mesa_ipv4_network_t *const next,
                                     vtss_appl_ospf_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    /* Get data from FRR layer */
    auto frr_area_conf = frr_ospf_area_network_conf_get(running_conf, key1);
    if (frr_area_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    /* Build the local sorted database for key2  */
    vtss::Set<mesa_ipv4_network_t> key2_set;
    for (const auto &itr : frr_area_conf) {
        key2_set.insert(itr.net);
    }

    Set<mesa_ipv4_network_t>::iterator key2_itr;
    if (!cur) {  // Get-First operation
        key2_itr = key2_set.begin();
    } else {  // Get-Next operation
        key2_itr = key2_set.greater_than(*cur);
    }

    if (key2_itr != key2_set.end()) {
        VTSS_TRACE(DEBUG) << "Found: key1: " << key1 << ", key2 = " << *key2_itr;
        *next = *key2_itr;
        return VTSS_RC_OK;  // Found it, break the loop
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate the OSPF areas
 * \param cur_id    [IN]  Current OSPF ID
 * \param next_id   [OUT] Next OSPF ID
 * \param cur_net   [IN]  Current area network
 * \param next_net  [OUT] Next area network
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_conf_itr(const vtss_appl_ospf_id_t *const cur_id,
                                     vtss_appl_ospf_id_t *const next_id,
                                     const mesa_ipv4_network_t *const cur_network,
                                     mesa_ipv4_network_t *const next_network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_id || !next_network) {
        VTSS_TRACE(ERROR)
                << "Parameter 'next_id' or 'next_network' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_id && *cur_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, mesa_ipv4_network_t> itr(
        OSPF_inst_itr, OSPF_area_conf_itr_k2);

    return itr(cur_id, next_id, cur_network, next_network);
}

static mesa_rc OSPF_area_status_itr_k2(const vtss_appl_ospf_area_id_t *const cur,
                                       vtss_appl_ospf_area_id_t *const next,
                                       vtss_appl_ospf_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF status. "
                          "(rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (result_list->areas.empty()) {
        VTSS_TRACE(DEBUG) << "area is empty ";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Build the local sorted database for key2
     * The stub area database should include the stub areas and
     * totally stub areas both. */
    vtss::Map<vtss_appl_ospf_area_id_t, FrrIpOspfArea> &key2_map =
        result_list->areas;
    vtss::Map<vtss_appl_ospf_area_id_t, FrrIpOspfArea>::iterator key2_itr;

    if (!cur) {  // Get-First operation
        key2_itr = key2_map.begin();
    } else {  // Get-Next operation
        key2_itr = key2_map.greater_than(*cur);
    }

    if (key2_itr != key2_map.end()) {
        VTSS_TRACE(DEBUG) << "Found: key1: " << key1
                          << ", key2 = " << key2_itr->first;
        *next = key2_itr->first;
        return VTSS_RC_OK;  // Found it, break the loop
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate through the OSPF area status.
 * \param cur_id       [IN]  Current OSPF ID
 * \param next_id      [OUT] Next OSPF ID
 * \param cur_area_id  [IN]  Current area ID
 * \param next_area_id [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_status_itr(
    const vtss_appl_ospf_id_t *const cur_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_area_id_t *const cur_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_id || !next_area_id) {
        VTSS_TRACE(ERROR)
                << "Parameter 'next_id' or 'next_area' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_id && *cur_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_area_id_t> itr(
        OSPF_inst_itr, OSPF_area_status_itr_k2);

    return itr(cur_id, next_id, cur_area_id, next_area_id);
}

/* This static function is implemented later. */
static mesa_rc OSPF_stub_area_conf_get(const vtss_appl_ospf_id_t id,
                                       const vtss_appl_ospf_area_id_t area_id,
                                       vtss_appl_ospf_stub_area_conf_t *const conf);

/**
 * \brief Get the OSPF area status.
 * \param id        [IN] OSPF instance ID.
 * \param area      [IN] OSPF area key.
 * \param status    [OUT] OSPF area val.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_status_get(const vtss_appl_ospf_id_t id,
                                       const vtss_appl_ospf_area_id_t area,
                                       vtss_appl_ospf_area_status_t *const status)
{
    CRIT_SCOPE();

    if (!status) {
        return VTSS_RC_ERROR;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF status. "
                          "(instance_id = "
                          << id << ",area = " << area
                          << ", rc = " << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Check if there's area information */
    if (result_list->areas.empty()) {
        VTSS_TRACE(DEBUG) << "area is empty ";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup this entry if existing or not. */
    if (!OSPF_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::Map<vtss_appl_ospf_area_id_t, FrrIpOspfArea> &key2_map =
        result_list->areas;
    vtss::Map<vtss_appl_ospf_area_id_t, FrrIpOspfArea>::iterator key2_itr =
        key2_map.find(area);
    if (key2_itr == key2_map.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* get the value of the map */
    const auto &frr_status = key2_itr->second;
    status->is_backbone = frr_status.backbone;

    /* get stub area configuration */
    vtss_appl_ospf_stub_area_conf_t conf;
    mesa_rc rc = OSPF_stub_area_conf_get(id, area, &conf);
    if (rc == VTSS_RC_OK) {
        status->area_type =
            (conf.is_nssa ? VTSS_APPL_OSPF_AREA_NSSA
             : conf.no_summary ? VTSS_APPL_OSPF_AREA_TOTALLY_STUB
             : VTSS_APPL_OSPF_AREA_STUB);
    } else if (rc == FRR_RC_ENTRY_NOT_FOUND) {
        status->area_type = VTSS_APPL_OSPF_AREA_NORMAL;
    } else {
        // get stub configuration failed
        status->area_type = VTSS_APPL_OSPF_AREA_COUNT;
    }

    status->attached_intf_active_count = frr_status.area_if_activ_counter;
    status->auth_type = frr_status.authentication;
    status->spf_executed_count = frr_status.spf_executed_counter;
    status->lsa_count = frr_status.lsa_nr;
    status->router_lsa_count = frr_status.lsa_router_nr;
    status->router_lsa_checksum = frr_status.lsa_router_checksum;
    status->network_lsa_count = frr_status.lsa_network_nr;
    status->network_lsa_checksum = frr_status.lsa_network_checksum;
    status->summary_lsa_count = frr_status.lsa_summary_nr;
    status->summary_lsa_checksum = frr_status.lsa_summary_checksum;
    status->asbr_summary_lsa_count = frr_status.lsa_asbr_nr;
    status->asbr_summary_lsa_checksum = frr_status.lsa_asbr_checksum;
    status->nssa_lsa_count = frr_status.lsa_nssa_nr;
    status->nssa_lsa_checksum = frr_status.lsa_nssa_checksum;
    if (frr_status.nssa_translator_elected) {
        status->nssa_trans_state = VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_ELECTED;
    } else if (frr_status.nssa_translator_always) {
        status->nssa_trans_state = VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_ENABLED;
    } else {
        status->nssa_trans_state = VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_DISABLED;
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF authentication
//----------------------------------------------------------------------------
/**
 * \brief Get the default authentication configuration for the specific
 * interface.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [OUT] The authentication configuration.
 * \return Error code.
 */
static mesa_rc OSPF_intf_auth_conf_def(vtss_ifindex_t *const ifindex,
                                       vtss_appl_ospf_intf_conf_t *const conf)
{
    conf->auth_type = VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG;
    conf->is_encrypted = false;
    memset(conf->auth_key, 0, sizeof(conf->auth_key));

    return VTSS_RC_OK;
}

/**
 * \brief Get the authentication configuration in the specific interface.
 * \param ifindex      [IN]  The index of VLAN interface.
 * \param as_encrypted [IN]  Set 'true' to output conf->auth_key as encrypted,
 otherwise out it as enencrypted.
 * \param conf          [OUT] The authentication configuration.
 * \return Error code.
 */
static mesa_rc OSPF_intf_auth_conf_get(std::string &frr_config, const vtss_ifindex_t ifindex, bool as_encrypted, vtss_appl_ospf_intf_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the interface is existing or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get the authenticaito type from FRR layer. */
    auto frr_if_auth_mode =
        frr_ospf_if_authentication_conf_get(frr_config, ifindex);
    if (frr_if_auth_mode.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get interface authentication "
                "mode. ( ifindex = "
                << ifindex << ", \'" << error_txt(frr_if_auth_mode.rc)
                << "\')";
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->auth_type = frr_ospf_auth_mode_mapping(frr_if_auth_mode.val);

    /* Get the authentication key from FRR layer and encrypt it. */
    auto frr_auth_key =
        frr_ospf_if_authentication_key_conf_get(frr_config, ifindex);
    if (frr_auth_key.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get interface authentication key "
                "( ifindex = "
                << ifindex << ", \'" << error_txt(frr_auth_key.rc) << "\')";
        return FRR_RC_INTERNAL_ACCESS;
    }

    memset(conf->auth_key, 0, sizeof(conf->auth_key));
    if (frr_auth_key.val.empty()) {
        conf->is_encrypted = false;
    } else if (as_encrypted) {
        VTSS_TRACE(DEBUG) << ifindex << " encrypting " << frr_auth_key.val;
        if (frr_util_secret_key_cryptography(
                true, frr_auth_key.val.c_str(),
                VTSS_APPL_OSPF_AUTH_ENCRYPTED_SIMPLE_KEY_LEN + 1,
                conf->auth_key) != VTSS_RC_OK) {
            VTSS_TRACE(ERROR) << "Access framework failed: Encrypt "
                              "authentication key "
                              "failed";
            return FRR_RC_INTERNAL_ERROR;
        }

        VTSS_TRACE(DEBUG) << ifindex << " encrypted data is " << conf->auth_key;
        conf->is_encrypted = true;
    } else {
        strncpy(conf->auth_key, frr_auth_key.val.c_str(),
                sizeof(conf->auth_key) - 1);
        conf->is_encrypted = false;
    }

    return VTSS_RC_OK;
}

// It is defined later.
static mesa_rc frr_ospf_validate_secret_key(
    const vtss_appl_ospf_auth_type_t auth_type, const bool is_encrypted_key,
    const char *const key, char *const unencrypted_key);

/**
 * \brief Set the authentication configuration in the specific interface.
 * \param ifindex [IN] The index of VLAN interface.
 * \param orig_conf [IN] The original authentication configuration.
 * \param conf    [IN] The authentication configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_TOO_LONG means the password
 *  is too long.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID means the key is not a valid
 *  key for AES256.
 */
static mesa_rc OSPF_intf_auth_conf_set(
    const vtss_ifindex_t ifindex,
    const vtss_appl_ospf_intf_conf_t *const orig_conf,
    const vtss_appl_ospf_intf_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (conf->auth_type == VTSS_APPL_OSPF_AUTH_TYPE_COUNT) {
        VTSS_TRACE(DEBUG) << "Invalid \'auth_type\'";
        return FRR_RC_INVALID_ARGUMENT;
    }

    char plain_txt[VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN + 1];
    if (strlen(conf->auth_key)) {
        VTSS_TRACE(DEBUG) << "(" << conf->auth_type << ", "
                          << conf->is_encrypted << ", " << conf->auth_key << ")";
        auto rc = frr_ospf_validate_secret_key(
                      VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD, conf->is_encrypted,
                      conf->auth_key, plain_txt);
        if (rc != VTSS_RC_OK) {
            return rc;
        }

        plain_txt[sizeof(plain_txt) - 1] = '\0';
    }

    /* Set authentication mode to FRR layer if needed. */
    if (orig_conf->auth_type != conf->auth_type) {
        FrrOspfAuthMode mode = frr_ospf_auth_type_mapping(conf->auth_type);
        if (frr_ospf_if_authentication_conf_set(ifindex, mode) != VTSS_RC_OK) {
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    /* Decrypted 'conf->auth_key' if needed and compare it with
     * 'orig_conf.auth_key' to detemine if needed to set to FRR layer or not  */
    std::string key;
    if (strlen(conf->auth_key) != 0) {
        if (conf->is_encrypted) {
            //            if (frr_util_secret_key_cryptography(
            //                        false, conf->auth_key,
            //                        VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN,
            //                        plain_txt) != VTSS_RC_OK) {
            //                return FRR_RC_INTERNAL_ERROR;
            //            }
            //            plain_txt[sizeof(plain_txt) - 1] = '\0';
            key = plain_txt;
        } else {
            key = conf->auth_key;
        }

        // Set the key if it changes.
        if (strcmp(orig_conf->auth_key, conf->auth_key)) {
            // Set the key.
            VTSS_TRACE(INFO) << "set auth_key to " << key;
            if (frr_ospf_if_authentication_key_conf_set(ifindex, key) !=
                VTSS_RC_OK) {
                VTSS_TRACE(DEBUG) << "Access framework failed: Set interface "
                                  "authentication key "
                                  "( key = "
                                  << key << ")";
                return FRR_RC_INTERNAL_ACCESS;
            }
        }
    } else if (strlen(orig_conf->auth_key) != 0) {
        // Delete the key.
        VTSS_TRACE(INFO) << "delete auth_key to " << key;
        if (frr_ospf_if_authentication_key_conf_del(ifindex) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Delete interface "
                              "authentication key "
                              "( key = "
                              << key << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the digest key in the specific interface.
 * \param ifindex       [IN]  The index of VLAN interface.
 * \param key_id        [IN]  The key ID.
 * \param as_encrypted  [IN]  Set 'true' to output digest_key->digest_key as
 * encrypted.
 * \param digest_key    [OUT] The digest key.
 * \return Error code.
 */
static mesa_rc OSPF_intf_auth_digest_key_get(
    const vtss_ifindex_t ifindex, const vtss_appl_ospf_md_key_id_t key_id,
    const bool as_encrypted,
    vtss_appl_ospf_auth_digest_key_t *const digest_key)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the interface is existing or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Boundary check */
    if (key_id < VTSS_APPL_OSPF_AUTH_DIGEST_KEY_ID_MIN) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_auth_md_conf = frr_ospf_if_message_digest_conf_get(running_conf, ifindex);
    if (frr_auth_md_conf.empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    auto itr = find_if(frr_auth_md_conf.begin(), frr_auth_md_conf.end(),
    [&](const auto & x) {
        return x.keyid == key_id;
    });
    if (itr == frr_auth_md_conf.end()) {
        VTSS_TRACE(DEBUG) << "NOT Found";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    if (as_encrypted) {
        if (frr_util_secret_key_cryptography(
                true, itr->key.c_str(),
                VTSS_APPL_OSPF_AUTH_ENCRYPTED_DIGEST_KEY_LEN + 1,
                digest_key->digest_key) != VTSS_RC_OK) {
            return FRR_RC_INTERNAL_ERROR;
        }
    } else {
        strncpy(digest_key->digest_key, itr->key.c_str(),
                sizeof(digest_key->digest_key) - 1);
        digest_key->digest_key[sizeof(digest_key->digest_key) - 1] = '\0';
    }

    digest_key->is_encrypted = as_encrypted;
    VTSS_TRACE(DEBUG) << "Found entry(" << ifindex << ",  " << key_id << ")";
    return VTSS_RC_OK;
}

/**
 * \brief Get Get the default configuration for message digest key.
 * \param ifindex       [IN]  The index of VLAN interface.
 * \param key_id        [IN]  The key ID.
 * \param digest_key    [OUT] The digest key.
 * \return Error code.
 */
mesa_rc frr_ospf_intf_auth_digest_key_def(
    vtss_ifindex_t *const ifindex, vtss_appl_ospf_md_key_id_t *const key_id,
    vtss_appl_ospf_auth_digest_key_t *const digest_key)
{
    vtss_clear(*digest_key);

    // Fill the none zero initial value below

    return VTSS_RC_OK;
}

/**
 * \brief Get the digest key in the specific interface.
 * \param ifindex       [IN]  The index of VLAN interface.
 * \param key_id        [IN]  The key ID.
 * \param digest_key    [OUT] The digest key.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_auth_digest_key_get(
    const vtss_ifindex_t ifindex, const vtss_appl_ospf_md_key_id_t key_id,
    vtss_appl_ospf_auth_digest_key_t *const digest_key)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!digest_key) {
        VTSS_TRACE(ERROR) << "Parameter 'digest_key' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    return OSPF_intf_auth_digest_key_get(ifindex, key_id, true, digest_key);
}

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
    const vtss_ifindex_t ifindex, const vtss_appl_ospf_md_key_id_t key_id,
    const vtss_appl_ospf_auth_digest_key_t *const digest_key)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!digest_key) {
        VTSS_TRACE(ERROR) << "Parameter 'digest_key' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the valid key. */
    char plain_txt[VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MAX_LEN + 1];
    if (strlen(digest_key->digest_key)) {
        auto rc = frr_ospf_validate_secret_key(
                      VTSS_APPL_OSPF_AUTH_TYPE_MD5, digest_key->is_encrypted,
                      digest_key->digest_key, plain_txt);
        if (rc != VTSS_RC_OK) {
            return rc;
        }
    } else if (digest_key->is_encrypted) {
        // The empty key can't be treated as encrypted.
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the entry exists. */
    vtss_appl_ospf_auth_digest_key_t orig_conf;
    if (OSPF_intf_auth_digest_key_get(ifindex, key_id, digest_key->is_encrypted,
                                      &orig_conf) == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    }

    mesa_rc rc;
    if (digest_key->is_encrypted) {
        FrrOspfDigestData frr_auth_conf(key_id, plain_txt);
        rc = frr_ospf_if_message_digest_conf_set(ifindex, frr_auth_conf);
    } else {
        FrrOspfDigestData frr_auth_conf(key_id, digest_key->digest_key);
        rc = frr_ospf_if_message_digest_conf_set(ifindex, frr_auth_conf);
    }

    return rc != VTSS_RC_OK ? FRR_RC_INTERNAL_ACCESS : rc;
}

/**
 * \brief Delete a digest key in the specific interface.
 * \param ifindex       [IN]  The index of VLAN interface.
 * \param key_id        [IN] The key ID.
 * \return Error code.
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_intf_auth_digest_key_del(
    const vtss_ifindex_t ifindex, const vtss_appl_ospf_md_key_id_t key_id)
{
    CRIT_SCOPE();

    /* Return siliently if the entry isn't found. */
    vtss_appl_ospf_auth_digest_key_t orig_conf;
    mesa_rc rc =
        OSPF_intf_auth_digest_key_get(ifindex, key_id, false, &orig_conf);
    if (rc != VTSS_RC_OK) {
        return rc == FRR_RC_ENTRY_NOT_FOUND ? VTSS_RC_OK : rc;
    }

    return (rc = frr_ospf_if_message_digest_conf_del(ifindex, key_id)) != VTSS_RC_OK
           ? FRR_RC_INTERNAL_ACCESS
           : rc;
}

static mesa_rc OSPF_intf_auth_digest_key_itr_k1(const vtss_ifindex_t *const cur, vtss_ifindex_t *const next)
{
    FRR_CRIT_ASSERT_LOCKED();
    return OSPF_if_itr(cur, next);
}

static mesa_rc OSPF_intf_auth_digest_key_itr_k2(const vtss_appl_ospf_md_key_id_t *const cur, vtss_appl_ospf_md_key_id_t *const next, vtss_ifindex_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    VTSS_TRACE(INFO) << "The Key-1 is " << key1;

    auto frr_auth_md_conf = frr_ospf_if_message_digest_conf_get(running_conf, key1);
    if (frr_auth_md_conf.empty()) {
        return VTSS_RC_ERROR;
    }

    /* Build the local sorted database for key-2. */
    vtss::Set<vtss_appl_ospf_md_key_id_t> key2_set;
    for (const auto &itr : frr_auth_md_conf) {
        key2_set.insert(itr.keyid);
    }

    if (!cur) {
        *next = *key2_set.begin();
        VTSS_TRACE(DEBUG) << "Found entry(" << key1 << ",  " << *next << ")";
        return VTSS_RC_OK;
    }

    Set<vtss_appl_ospf_md_key_id_t>::iterator key2_itr;
    VTSS_TRACE(INFO) << "get NEXT Key-2 from " << *cur;
    key2_itr = key2_set.greater_than(*cur);
    if (key2_itr != key2_set.end()) {
        *next = *key2_itr;
        VTSS_TRACE(DEBUG) << "Found entry(" << key1 << ",  " << *next << ")";
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate the digest key.
 * \param current_ifindex [IN]  The current ifIndex.
 * \param next_ifindex    [OUT] The next ifIndex.
 * \param current_key_id  [IN]  The current key ID.
 * \param next_key_id     [OUT] The next key ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_auth_digest_key_itr(
    const vtss_ifindex_t *const current_ifindex,
    vtss_ifindex_t *const next_ifindex,
    const vtss_appl_ospf_md_key_id_t *const current_key_id,
    uint8_t *const next_key_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_ifindex || !next_key_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_ifindex' or 'next_key_id' cannot "
                          "be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss::IteratorComposeDependN<vtss_ifindex_t, vtss_appl_ospf_md_key_id_t> itr(
        OSPF_intf_auth_digest_key_itr_k1, OSPF_intf_auth_digest_key_itr_k2);

    return itr(current_ifindex, next_ifindex, current_key_id, next_key_id);
}

/**
 * \brief Get the digest key by the precedence.
 * \param ifindex [IN]  The current ifIndex.
 * \param pre_id [IN]  The precedence ID.
 * \param key_id    [OUT] The key ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_md_key_precedence_get(
    const vtss_ifindex_t ifindex, const uint32_t pre_id,
    vtss_appl_ospf_md_key_id_t *const key_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!key_id) {
        VTSS_TRACE(ERROR) << "Parameter 'key_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the interface is existing or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_auth_md_conf = frr_ospf_if_message_digest_conf_get(running_conf, ifindex);

    if (frr_auth_md_conf.empty() || pre_id > frr_auth_md_conf.size()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    *key_id = frr_auth_md_conf.at(pre_id - 1)->keyid;
    return VTSS_RC_OK;
}

static mesa_rc OSPF_intf_md_key_precedence_itr_k1(const vtss_ifindex_t *const current_ifindex, vtss_ifindex_t *const next_ifindex)
{
    FRR_CRIT_ASSERT_LOCKED();
    return OSPF_if_itr(current_ifindex, next_ifindex);
}

static mesa_rc OSPF_intf_md_key_precedence_itr_k2( const uint32_t *const current_pre_id, uint32_t *const next_pre_id, vtss_ifindex_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();
    VTSS_TRACE(INFO) << "The Key-1 is " << key1;

    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_auth_md_conf = frr_ospf_if_message_digest_conf_get(running_conf, key1);
    if (frr_auth_md_conf.empty()) {
        VTSS_TRACE(INFO) << "goto NEXT Key-1 from " << key1;
        return VTSS_RC_ERROR;
    }

    // Check if this is a get first
    if (!current_pre_id) {
        // We know that we have atleast one
        *next_pre_id = 1;
        VTSS_TRACE(DEBUG) << "Found entry(" << key1 << ",  " << *next_pre_id
                          << ")";
        return VTSS_RC_OK;
    }

    if (*current_pre_id < frr_auth_md_conf.size()) {
        *next_pre_id = *current_pre_id + 1;
        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

/**
 * \brief Iterate the digest key by the precedence.
 * \param current_ifindex [IN]  The current ifIndex.
 * \param next_ifindex    [OUT] The next ifIndex.
 * \param current_ifindex [IN]  The precedence ID.
 * \param next_ifindex    [OUT] The next precedence ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_md_key_precedence_itr(
    const vtss_ifindex_t *const current_ifindex,
    vtss_ifindex_t *const next_ifindex,
    const uint32_t *const current_pre_id, uint32_t *const next_pre_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_pre_id || !next_ifindex) {
        VTSS_TRACE(ERROR) << "Parameter 'next_pre_id' or 'next_ifindex' or "
                          "'next_key_id' cannot "
                          "be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss::IteratorComposeDependN<vtss_ifindex_t, uint32_t> itr(
        OSPF_intf_md_key_precedence_itr_k1,
        OSPF_intf_md_key_precedence_itr_k2);

    return itr(current_ifindex, next_ifindex, current_pre_id, next_pre_id);
}

/**
 * \brief Set the digest key in the specific interface.
 *        It is a dummy function for SNMP serialzer only.
 * \param ifindex       [IN] The index of VLAN interface.
 * \param key_id        [IN] The key ID.
 * \param digest_key    [IN] The digest key.
 * \return Error code.
 */
mesa_rc frr_ospf_intf_auth_digest_key_dummy_set(
    const vtss_ifindex_t ifindex, const vtss_appl_ospf_md_key_id_t key_id,
    const vtss_appl_ospf_auth_digest_key_t *const digest_key)
{
    return FRR_RC_NOT_SUPPORTED;
}

/**
 * \brief Get the authentication configuration in the specific area.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  OSPF area ID.
 * \param auth_type [OUT] The authentication type.
 * \return Error code.
 */
static mesa_rc OSPF_area_auth_conf_get(const vtss_appl_ospf_id_t id,
                                       const vtss_appl_ospf_area_id_t area_id,
                                       vtss_appl_ospf_auth_type_t *const auth_type)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!auth_type) {
        VTSS_TRACE(ERROR) << "Parameter 'auth_type' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_auth_conf = frr_ospf_area_authentication_conf_get(running_conf, id);
    if (frr_auth_conf.empty()) {
        T_DG(FRR_TRACE_GRP_OSPF, "Empty area range");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    for (const auto &itr : frr_auth_conf) {
        if (itr.area == area_id) {
            // Found it
            *auth_type = (itr.auth_mode == FRR_OSPF_AUTH_MODE_PWD
                          ? VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD
                          : VTSS_APPL_OSPF_AUTH_TYPE_MD5);
            VTSS_TRACE(DEBUG)
                    << "Found entry: Area authentication. (instance_id = " << id
                    << ", area_id = " << area_id
                    << ", auth_type = " << *auth_type << ")";
            return VTSS_RC_OK;
        }
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get the authentication configuration in the specific area.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  OSPF area ID.
 * \param auth_type [OUT] The authentication type.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_get(const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id, vtss_appl_ospf_auth_type_t *const auth_type)
{
    CRIT_SCOPE();
    return OSPF_area_auth_conf_get(id, area_id, auth_type);
}

/**
 * \brief Add the authentication configuration in the specific area.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] OSPF area ID.
 * \param auth_type [IN] The authentication type.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_add(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_auth_type_t auth_type)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (auth_type != VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD &&
        auth_type != VTSS_APPL_OSPF_AUTH_TYPE_MD5) {
        VTSS_TRACE(DEBUG) << "Parameter 'auth_type'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the entry is existing or not */
    mesa_rc rc;
    vtss_appl_ospf_auth_type_t orig_auth_type;
    rc = OSPF_area_auth_conf_get(id, area_id, &orig_auth_type);
    if (rc == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    /* Apply to FRR layer when the entry is a new one. */
    FrrOspfAreaAuth area_auth;
    area_auth.area = area_id;
    area_auth.auth_mode = (auth_type == VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD
                           ? FRR_OSPF_AUTH_MODE_PWD
                           : FRR_OSPF_AUTH_MODE_MSG_DIGEST);
    rc = frr_ospf_area_authentication_conf_set(id, area_auth);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Delete area authentication. "
                "(instance_id = "
                << id << ", area_id = " << area_id
                << ", auth_type = " << auth_type << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Set the authentication configuration in the specific area.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] OSPF area ID.
 * \param auth_type [IN] The authentication type.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_set(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_auth_type_t auth_type)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (auth_type != VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD &&
        auth_type != VTSS_APPL_OSPF_AUTH_TYPE_MD5) {
        VTSS_TRACE(DEBUG) << "Parameter 'auth_type'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get the original configuration */
    vtss_appl_ospf_auth_type_t orig_auth_type;
    mesa_rc rc = OSPF_area_auth_conf_get(id, area_id, &orig_auth_type);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    if (orig_auth_type != auth_type) {
        FrrOspfAreaAuth area_auth;
        area_auth.area = area_id;
        area_auth.auth_mode = (auth_type == VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD
                               ? FRR_OSPF_AUTH_MODE_PWD
                               : FRR_OSPF_AUTH_MODE_MSG_DIGEST);
        rc = frr_ospf_area_authentication_conf_set(id, area_auth);

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set area authentication. "
                    "(instance_id = "
                    << id << ", area_id = " << area_id
                    << ", auth_type = " << auth_type << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Delete the authentication configuration in the specific area.
 * \param id      [IN]  OSPF instance ID.
 * \param area_id [IN]  OSPF area ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_del(const vtss_appl_ospf_id_t id,
                                          const vtss_appl_ospf_area_id_t area_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the entry is existing or not */
    vtss_appl_ospf_auth_type_t auth_type;
    mesa_rc rc = OSPF_area_auth_conf_get(id, area_id, &auth_type);
    if (rc != VTSS_RC_OK) {
        // For the deleting operation, quit silently when it does not exists
        return VTSS_RC_OK;
    }

    /* Apply to FRR layer */
    FrrOspfAreaAuth area_auth;
    area_auth.area = area_id;
    area_auth.auth_mode = FRR_OSPF_AUTH_MODE_NULL;
    rc = frr_ospf_area_authentication_conf_set(id, area_auth);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Delete area authentication. "
                "(instance_id = "
                << id << ", area_id = " << area_id
                << ", auth_type = " << auth_type << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

static mesa_rc OSPF_area_auth_conf_itr_k2(const vtss_appl_ospf_id_t *const cur,
                                          vtss_appl_ospf_id_t *const next,
                                          vtss_appl_ospf_area_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_area_auth_conf = frr_ospf_area_authentication_conf_get(running_conf, key1);
    if (frr_area_auth_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    /* Build the local sorted database for key2. */
    vtss::Set<vtss_appl_ospf_area_id_t> key2_set;
    for (const auto &i : frr_area_auth_conf) {
        key2_set.insert(i.area);
    }

    Set<vtss_appl_ospf_area_id_t>::iterator key2_itr;
    if (!cur) {  // Get-First operation
        key2_itr = key2_set.begin();
    } else {  // Get-Next operation
        key2_itr = key2_set.greater_than(*cur);
    }

    if (key2_itr != key2_set.end()) {
        VTSS_TRACE(DEBUG) << "Found: key1: " << key1 << ", key2 = " << *key2_itr;
        *next = *key2_itr;
        return VTSS_RC_OK;  // Found it, break the loop
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate the specific area with authentication configuration.
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_area_id [IN]  Current area ID
 * \param next_area_id    [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_auth_conf_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_area_id_t *const current_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_id || !next_area_id) {
        VTSS_TRACE(ERROR)
                << "Parameter 'next_id' or 'next_area_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_area_id_t> itr(
        OSPF_inst_itr, OSPF_area_auth_conf_itr_k2);

    return itr(current_id, next_id, current_area_id, next_area_id);
}

/**
 * \brief Get the default configuration for a specific stub areas.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  OSPF area ID.
 * \param auth_type [OUT] The authentication type.
 * \return Error code.
 */
mesa_rc frr_ospf_area_auth_conf_def(vtss_appl_ospf_id_t *const id,
                                    vtss_appl_ospf_area_id_t *const area_id,
                                    vtss_appl_ospf_auth_type_t *const auth_type)
{
    if (!auth_type) {
        VTSS_TRACE(ERROR) << "Parameter 'auth_type' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    *id = VTSS_APPL_OSPF_INSTANCE_ID_START;
    *auth_type = VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD;
    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF area range
//----------------------------------------------------------------------------
/**
 * \brief Get the OSPF area range default configuration.
 * \param id      [IN]  OSPF instance ID.
 * \param area_id [IN]  OSPF area ID.
 * \param network [IN]  OSPF area range network.
 * \param conf    [OUT] OSPF area range configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_area_range_conf_def(vtss_appl_ospf_id_t *const id,
                                     vtss_appl_ospf_area_id_t *const area_id,
                                     mesa_ipv4_network_t *const network,
                                     vtss_appl_ospf_area_range_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF_INSTANCE_ID_START;
    conf->is_specific_cost = false;
    conf->cost = VTSS_APPL_OSPF_GENERAL_COST_MIN;
    conf->is_advertised = true;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF area range configuration.
 * \param id            [IN]  OSPF instance ID.
 * \param area_id       [IN]  OSPF area ID.
 * \param network       [IN]  OSPF area range network.
 * \param conf          [OUT] OSPF area range configuration.
 * \param check_overlap [IN]  Set 'true' to check if the address range is
 * overlap or not, otherwise not to check it.
 * \return Error code.
 */
static mesa_rc OSPF_area_range_conf_get(const vtss_appl_ospf_id_t id,
                                        const vtss_appl_ospf_area_id_t area_id,
                                        const mesa_ipv4_network_t network,
                                        vtss_appl_ospf_area_range_conf_t *const conf,
                                        mesa_bool_t check_overlap)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_area_range_conf = frr_ospf_area_range_conf_get(running_conf, id);
    if (frr_area_range_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty area range";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    for (const auto &itr : frr_area_range_conf) {
        if (vtss_ipv4_net_mask_out(&itr.net) == vtss_ipv4_net_mask_out(&network) &&
            itr.area == area_id) {
            // Found it

            // Get area range advertise configuration
            conf->is_advertised = true;
            auto frr_area_range_advertise_conf = frr_ospf_area_range_not_advertise_conf_get(running_conf, id);
            for (const auto &itr_advertise : frr_area_range_advertise_conf) {
                if (vtss_ipv4_net_mask_out(&itr_advertise.net) == vtss_ipv4_net_mask_out(&network) && itr_advertise.area == area_id) {
                    conf->is_advertised = false;
                    break;
                }
            }

            // Get area range cost configuration
            conf->is_specific_cost = false;
            conf->cost = VTSS_APPL_OSPF_GENERAL_COST_MIN;
            auto frr_area_range_cost_conf = frr_ospf_area_range_cost_conf_get(running_conf, id);
            for (const auto &itr_cost : frr_area_range_cost_conf) {
                if (vtss_ipv4_net_mask_out(&itr_cost.net) == vtss_ipv4_net_mask_out(&network) && itr_cost.area == area_id) {
                    conf->is_specific_cost = true;
                    conf->cost = itr_cost.cost;
                    break;
                }
            }

            VTSS_TRACE(DEBUG) << "Found entry: Area range. (instance_id = " << id
                              << ", area_id = " << area_id
                              << ", network_addr = " << network
                              << ", is_advertised =" << conf->is_advertised
                              << ", cost = " << conf->cost << ")";
            return VTSS_RC_OK;
        }

        if (check_overlap && itr.area == area_id &&
            vtss_ipv4_net_overlap(&itr.net, &network)) {
            return FRR_RC_ADDR_RANGE_OVERLAP;
        }
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get the OSPF area range configuration.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] OSPF area ID.
 * \param network [IN] OSPF area range network.
 * \param conf    [IN] OSPF area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_get(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const mesa_ipv4_network_t network,
    vtss_appl_ospf_area_range_conf_t *const conf)
{
    CRIT_SCOPE();
    return OSPF_area_range_conf_get(id, area_id, network, conf, false);
}

/**
 * \brief Set the OSPF area range configuration.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] OSPF area ID.
 * \param network [IN] OSPF area range network.
 * \param conf    [IN] OSPF area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_set(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const mesa_ipv4_network_t network,
    const vtss_appl_ospf_area_range_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!conf->is_advertised && conf->is_specific_cost) {
        return VTSS_APPL_FRR_OSPF_ERROR_AREA_RANGE_COST_CONFLICT;
    }

    mesa_ipv4_network_t net = vtss_ipv4_net_mask_out(&network);
    if (net.address == 0) {
        return VTSS_APPL_FRR_OSPF_ERROR_AREA_RANGE_NETWORK_DEFAULT;
    }

    mesa_bool_t is_unexp_deleted = false;  // unexpected deleted
    /* Get the original configuration */
    vtss_appl_ospf_area_range_conf_t orig_conf;
    vtss::FrrOspfAreaNetwork frr_area_range_conf = {
        vtss_ipv4_net_mask_out(&network), area_id
    };
    mesa_rc rc =
        OSPF_area_range_conf_get(id, area_id, network, &orig_conf, false);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    /* Apply the new cost configuration */
    /* We intentionally apply cost first and advertise second,
       Because when apply 'no area range cost', FRR deletes area entry
       instead of resetting the cost.
       We manage to add the area entry back when applying advertise */

    if (orig_conf.is_specific_cost != conf->is_specific_cost ||
        orig_conf.cost != conf->cost) {
        // Apply the cost advertise configuration
        vtss::FrrOspfAreaNetworkCost frr_area_range_cost_conf = {
            vtss_ipv4_net_mask_out(&network), area_id, conf->cost
        };
        if (conf->is_specific_cost) {
            rc = frr_ospf_area_range_cost_conf_set(id, frr_area_range_cost_conf);
        } else {
            frr_area_range_cost_conf.cost = VTSS_APPL_OSPF_GENERAL_COST_MIN;
            // FRR will delete entry when apply 'no area range cost'!!
            rc = frr_ospf_area_range_cost_conf_del(id, frr_area_range_cost_conf);
            is_unexp_deleted = true;
        }

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set area range cost. "
                    "(instance_id = "
                    << id << ", area_id = " << area_id
                    << ", network_addr = " << network << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Apply the new advertise configuration
    if (is_unexp_deleted || orig_conf.is_advertised != conf->is_advertised) {
        if (conf->is_advertised) {
            rc = frr_ospf_area_range_conf_set(id, frr_area_range_conf);
        } else {
            rc = frr_ospf_area_range_not_advertise_conf_set(
                     id, frr_area_range_conf);
        }

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set area range advertise. "
                    "(instance_id = "
                    << id << ", area_id = " << area_id
                    << ", network_addr = " << network << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Add the OSPF area range configuration.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] OSPF area ID.
 * \param network [IN] OSPF area range network.
 * \param conf    [IN] OSPF area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_add(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const mesa_ipv4_network_t network,
    const vtss_appl_ospf_area_range_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!conf->is_advertised && conf->is_specific_cost) {
        return VTSS_APPL_FRR_OSPF_ERROR_AREA_RANGE_COST_CONFLICT;
    }

    mesa_ipv4_network_t net = vtss_ipv4_net_mask_out(&network);
    if (net.address == 0) {
        return VTSS_APPL_FRR_OSPF_ERROR_AREA_RANGE_NETWORK_DEFAULT;
    }

    /* Check the entry is existing or not */
    mesa_rc rc;
    vtss_appl_ospf_area_range_conf_t orig_conf;
    rc = OSPF_area_range_conf_get(id, area_id, network, &orig_conf, true);
    if (rc == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    /* Apply to FRR layer when the entry is a new one. */
    vtss::FrrOspfAreaNetwork frr_area_range_conf = {
        vtss_ipv4_net_mask_out(&network), area_id
    };
    rc = frr_ospf_area_range_conf_set(id, frr_area_range_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Add area range. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", network_addr = " << network << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (conf->is_advertised) {
        rc = frr_ospf_area_range_conf_set(id, frr_area_range_conf);
    } else {
        rc = frr_ospf_area_range_not_advertise_conf_set(id, frr_area_range_conf);
    }

    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Set area range advertise. "
                "(instance_id = "
                << id << ", area_id = " << area_id
                << ", network_addr = " << network << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (conf->is_specific_cost) {
        vtss::FrrOspfAreaNetworkCost frr_area_range_cost_conf = {
            vtss_ipv4_net_mask_out(&network), area_id, conf->cost
        };
        rc = frr_ospf_area_range_cost_conf_set(id, frr_area_range_cost_conf);

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set area range cost. "
                    "(instance_id = "
                    << id << ", area_id = " << area_id
                    << ", network_addr = " << network << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Delete the OSPF area range configuration.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] OSPF area ID.
 * \param network [IN] OSPF area range network.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_area_range_conf_del(const vtss_appl_ospf_id_t id,
                                           const vtss_appl_ospf_area_id_t area_id,
                                           const mesa_ipv4_network_t network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the entry is existing or not */
    vtss_appl_ospf_area_range_conf_t conf;
    mesa_rc rc = OSPF_area_range_conf_get(id, area_id, network, &conf, false);
    if (rc != VTSS_RC_OK) {
        // For the deleting operation, quit silently when it does not exists
        return VTSS_RC_OK;
    }

    /* Apply to FRR layer */
    vtss::FrrOspfAreaNetwork frr_area_range_conf = {
        vtss_ipv4_net_mask_out(&network), area_id
    };
    rc = frr_ospf_area_range_conf_del(id, frr_area_range_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete area range. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", network_addr = " << network << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

static mesa_rc OSPF_area_range_conf_itr_k2(const vtss_appl_ospf_area_id_t *const cur,
                                           vtss_appl_ospf_area_id_t *const next,
                                           vtss_appl_ospf_id_t k1)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    /* Get data from FRR layer (not sorted) */
    auto frr_area_range_conf = frr_ospf_area_range_conf_get(running_conf, k1);

    if (frr_area_range_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    /* Build the local sorted database for key2. */
    vtss::Set<vtss_appl_ospf_area_id_t> key2_set;
    for (const auto &i : frr_area_range_conf) {
        key2_set.insert(i.area);
    }

    /* Walk through the second layer database. (already sorted)
     * Notice that the third key should be treated as a Get-First operation
     * for the next loop.
     */
    auto i = !cur ? key2_set.begin() : key2_set.greater_than(*cur);
    if (i != key2_set.end()) {
        VTSS_TRACE(DEBUG) << "Found: key1: " << k1 << ", key2 = " << *i;
        *next = *i;
        return VTSS_RC_OK;  // Found it, break the loop
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

static mesa_rc OSPF_area_range_conf_itr_k3(const mesa_ipv4_network_t *const cur,
                                           mesa_ipv4_network_t *const next,
                                           vtss_appl_ospf_id_t k1,
                                           vtss_appl_ospf_area_id_t k2)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_area_range_conf = frr_ospf_area_range_conf_get(running_conf, k1);
    if (frr_area_range_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    vtss::Set<mesa_ipv4_network_t> key3_set;
    for (const auto &i : frr_area_range_conf) {
        if (k2 == i.area) {
            key3_set.insert(i.net);
        }
    }

    auto i = !cur ? key3_set.begin() : key3_set.greater_than(*cur);
    if (i != key3_set.end()) {
        VTSS_TRACE(DEBUG) << "Found: key1: " << k1 << ", key2 = " << k2
                          << ", key3: " << *i;
        *next = *i;
        return VTSS_RC_OK;  // Found it, break the loop
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

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
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_area_id_t *const current_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const mesa_ipv4_network_t *const current_network,
    mesa_ipv4_network_t *const next_network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_id || !next_area_id || !next_network) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_area_id' or "
                          "'next_network' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_area_id_t,
         mesa_ipv4_network_t>
         itr(OSPF_inst_itr, OSPF_area_range_conf_itr_k2,
             OSPF_area_range_conf_itr_k3);

    return itr(current_id, next_id, current_area_id, next_area_id,
               current_network, next_network);
}

/* Validate the secret key is valid or not.
 * \param auth_type        [IN]  Only accept types: simple password or md5.
 * \param is_encrypted_key [IN]  Set 'true' if the key is encrypted.
 * \param key              [IN]  The key.
 * \param unencrypted_key  [OUT] The unencrypted key (when parameter key is
 * encrypted).
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_TOO_LONG means the password
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID means the key is not a valid
 *  key for AES256.
 */
static mesa_rc frr_ospf_validate_secret_key(
    const vtss_appl_ospf_auth_type_t auth_type, const bool is_encrypted_key,
    const char *const key, char *const unencrypted_key)
{
    /* Check valid length */
    if (auth_type == VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD) {
        // unencrypted_key
        if (!is_encrypted_key &&
            strlen(key) > VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN) {
            VTSS_TRACE(DEBUG) << "Parameter 'unencrypted_key' length("
                              << strlen(key) << ") is invalid";
            return VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID;
        }

        // encrypted_key
        if (is_encrypted_key &&
            strlen(key) < VTSS_APPL_OSPF_AUTH_ENCRYPTED_KEY_LEN(
                VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MIN_LEN)
            /* excluded terminal character */) {
            VTSS_TRACE(DEBUG) << "Parameter 'encrypted_key' length("
                              << strlen(key) << ") is invalid ";
            return VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID;
        }
    } else if (auth_type == VTSS_APPL_OSPF_AUTH_TYPE_MD5) {
        // unencrypted_key
        if (!is_encrypted_key &&
            (strlen(key) < VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MIN_LEN ||
             strlen(key) > VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MAX_LEN)) {
            VTSS_TRACE(DEBUG) << "Parameter 'unencrypted_key' length("
                              << strlen(key) << ") is invalid";
            return VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID;
        }

        // encrypted_key
        if (is_encrypted_key &&
            strlen(key) < VTSS_APPL_OSPF_AUTH_ENCRYPTED_KEY_LEN(
                VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MIN_LEN)
            /* excluded terminal character */) {
            VTSS_TRACE(DEBUG) << "Parameter 'encrypted_key' length("
                              << strlen(key) << ") is invalid";
            return VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID;
        }
    } else {
        return VTSS_RC_ERROR;
    }

    /* Check valid input (hex character only) */
    if (is_encrypted_key) {
        for (size_t i = 0; i < strlen(key); ++i) {
            if (!((key[i] >= '0' && key[i] <= '9') ||
                  (key[i] >= 'A' && key[i] <= 'F') ||
                  (key[i] >= 'a' && key[i] <= 'f'))) {
                // Not hex character
                VTSS_TRACE(DEBUG) << "Not hex character.";
                return VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID;
            }
        }

        /* Decrypt the input key */
        if (unencrypted_key) {
            mesa_rc rc = frr_util_secret_key_cryptography(
                             false, key,
                             // The length must INCLUDE terminal character.
                             auth_type == VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD
                             ? (VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN + 1)
                             : (VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MAX_LEN + 1),
                             unencrypted_key);
            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Parameter 'encrypted_key' is invalid format";
                return rc;
            }
        }
    } else {
        // For the unencrypted password, only printable characters are accepted
        // and space character is not allowed in FRR layer.
        for (size_t i = 0; i < strlen(key); ++i) {
            if (key[i] == ' ' || key[i] < 32 || key[i] > 126) {
                VTSS_TRACE(DEBUG) << "Not printable or space character.";
                return VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID;
            }
        }
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF virtual link
//----------------------------------------------------------------------------
/**
 * \brief Get the default configuration of OSPF virtual link.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  OSPF area ID.
 * \param router_id [IN]  OSPF destination router id of virtual link.
 * \param conf      [OUT] OSPF virtual link configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_vlink_conf_def(vtss_appl_ospf_id_t *const id,
                                vtss_appl_ospf_area_id_t *const area_id,
                                vtss_appl_ospf_router_id_t *const router_id,
                                vtss_appl_ospf_vlink_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF_INSTANCE_ID_START;
    conf->hello_interval = FRR_OSPF_DEF_HELLO_INTERVAL;
    conf->dead_interval = FRR_OSPF_DEF_DEAD_INTERVAL;
    conf->retransmit_interval = FRR_OSPF_DEF_RETRANSMIT_INTERVAL;
    conf->auth_type = VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG;
    strcpy(conf->simple_pwd, "");

    return VTSS_RC_OK;
}

/**
 * \brief Find if virtual link exists
 * \param id             [IN]  OSPF instance ID.
 * \param area_id        [IN]  The area ID.
 * \return Error code.
 */
static mesa_rc OSPF_vlink_simple_find(const vtss_appl_ospf_id_t id,
                                      const vtss_appl_ospf_area_id_t area_id)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool found_entry = false;

    /* Check illegal parameters */
    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_vlink_conf = frr_ospf_area_virtual_link_conf_get(running_conf, id);
    if (frr_vlink_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty entry: virtual link";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    for (const auto &itr : frr_vlink_conf) {
        if (itr.area == area_id) {
            // Found it
            found_entry = true;
            break;
        }
    }

    if (!found_entry) {
        VTSS_TRACE(DEBUG) << "NOT_FOUND: virtual link";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF virtual link configuration.
 * \param id             [IN]  OSPF instance ID.
 * \param area_id        [IN]  The area ID of the configuration.
 * \param router_id      [IN]  The destination router id of virtual link.
 * \param conf           [OUT] The virtual link configuration.
 * \param get_plain_text [IN]  Output plain text in 'conf' parameter.
 * \return Error code.
 */
static mesa_rc OSPF_vlink_conf_get(const vtss_appl_ospf_id_t id,
                                   const vtss_appl_ospf_area_id_t area_id,
                                   const vtss_appl_ospf_router_id_t router_id,
                                   vtss_appl_ospf_vlink_conf_t *const conf,
                                   const mesa_bool_t get_plain_text)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool found_entry = false;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_vlink_conf = frr_ospf_area_virtual_link_conf_get(running_conf, id);
    if (frr_vlink_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty entry: virtual link";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    for (const auto &itr : frr_vlink_conf) {
        if (itr.area == area_id && itr.dst == router_id) {
            // Found it
            conf->hello_interval = itr.hello_interval.get();
            conf->dead_interval = itr.dead_interval.get();
            conf->retransmit_interval = itr.retransmit_interval.get();
            found_entry = true;
            break;
        }
    }

    if (!found_entry) {
        T_DG(FRR_TRACE_GRP_OSPF, "NOT_FOUND: virtual link");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    // Get authentication type
    conf->auth_type = VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG;
    auto frr_vlink_auth_conf =
        frr_ospf_area_virtual_link_authentication_conf_get(running_conf, id);
    for (const auto &itr : frr_vlink_auth_conf) {
        if (itr.virtual_link.area == area_id &&
            itr.virtual_link.dst == router_id) {
            // Found it
            conf->auth_type = frr_ospf_auth_mode_mapping(itr.auth_mode);
            if (conf->auth_type == VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG) {
                /* Continue the searching process since there maybe more entries
                 * also matched the same searching keys
                 * For example,
                 * area 1 virtual-link 1.2.3.4
                 * area 1 virtual-link 1.2.3.4 authentication message-digest
                 */
                continue;
            }

            break;
        }
    }

    // Get authentication simple password
    conf->is_encrypted = false;
    strcpy(conf->simple_pwd, "");
    auto frr_vlink_simple_pwd = frr_ospf_area_virtual_link_authentication_key_conf_get(running_conf, id);

    for (const auto &itr : frr_vlink_simple_pwd) {
        if (itr.virtual_link.area == area_id &&
            itr.virtual_link.dst == router_id) {
            // Found it
            if (get_plain_text) {
                strcpy(conf->simple_pwd, itr.key_data.c_str());
            } else {  // Convert to encrypted hex
                char unencrypted_key[VTSS_APPL_OSPF_AUTH_ENCRYPTED_SIMPLE_KEY_LEN + 1] =
                    "";
                strcpy(unencrypted_key, itr.key_data.c_str());
                mesa_rc rc = frr_util_secret_key_cryptography(
                                 true /* encrypt */, unencrypted_key,
                                 sizeof(conf->simple_pwd), conf->simple_pwd);
                if (rc != VTSS_RC_OK) {
                    // Should never happen
                    VTSS_TRACE(ERROR)
                            << "Internal error: OSPF secret key cryptography";
                    return FRR_RC_INTERNAL_ERROR;
                }

                conf->is_encrypted = true;
            }

            break;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the configuration for a virtual link.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  The area ID of the configuration.
 * \param router_id [IN]  The destination router id of virtual link.
 * \param conf      [OUT] The virtual link configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_conf_get(const vtss_appl_ospf_id_t id,
                                      const vtss_appl_ospf_area_id_t area_id,
                                      const vtss_appl_ospf_router_id_t router_id,
                                      vtss_appl_ospf_vlink_conf_t *const conf)
{
    CRIT_SCOPE();
    return OSPF_vlink_conf_get(id, area_id, router_id, conf, false);
}

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
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id,
    const vtss_appl_ospf_vlink_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Backbone area can't be configured as virtual link. */
    if (area_id == FRR_OSPF_BACKBONE_AREA_ID) {
        return VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_BACKBONE;
    }

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the area is stub or not. */
    mesa_rc rc;
    vtss_appl_ospf_stub_area_conf_t stub_orig_conf;
    rc = OSPF_stub_area_conf_get(id, area_id, &stub_orig_conf);
    if (rc == VTSS_RC_OK) {
        return VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_STUB;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    /* Check simple password */
    char unencrypted_key[VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN + 1] = "";
    rc = frr_ospf_validate_secret_key(VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD,
                                      conf->is_encrypted, conf->simple_pwd,
                                      unencrypted_key);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Parameter 'simple_pwd' is invalid";
        return rc;
    }

    /* Check the entry is existing or not */
    vtss_appl_ospf_vlink_conf_t orig_conf;
    rc = OSPF_vlink_conf_get(id, area_id, router_id, &orig_conf, true);
    if (rc == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    /* Apply to FRR layer when the entry is a new one. */
    FrrOspfAreaVirtualLink frr_vlink_conf(area_id, router_id);

    frr_vlink_conf.hello_interval = conf->hello_interval;
    frr_vlink_conf.dead_interval = conf->dead_interval;
    frr_vlink_conf.retransmit_interval = conf->retransmit_interval;

    rc = frr_ospf_area_virtual_link_conf_set(id, frr_vlink_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Add virtual link. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", router_id = " << router_id << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    // Set authentication type
    FrrOspfAreaVirtualLinkAuth frr_vlink_auth_conf;
    frr_vlink_auth_conf.virtual_link = {area_id, router_id};
    frr_vlink_auth_conf.auth_mode = frr_ospf_auth_type_mapping(conf->auth_type);
    rc = frr_ospf_area_virtual_link_authentication_conf_set(
             id, frr_vlink_auth_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set "
                          "virtual authentication type. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", router_id = " << router_id << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    // Set authentication simple password
    if (!conf->is_encrypted) {
        strcpy(unencrypted_key, conf->simple_pwd);
    }

    FrrOspfAreaVirtualLink frr_vlink_key(area_id, router_id);
    if (strlen(unencrypted_key)) {
        rc = frr_ospf_area_virtual_link_authentication_key_conf_set(
                 id, frr_vlink_key, unencrypted_key);
    }

    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set "
                          "virtual authentication simple password. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", router_id = " << router_id << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id,
    const vtss_appl_ospf_vlink_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Backbone area can't be configured as virtual link. */
    if (area_id == FRR_OSPF_BACKBONE_AREA_ID) {
        return VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_BACKBONE;
    }

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check simple password */
    char unencrypted_key[VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN + 1] = "";
    mesa_rc rc = frr_ospf_validate_secret_key(
                     VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD, conf->is_encrypted,
                     conf->simple_pwd, unencrypted_key);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Parameter 'simple_pwd' is invalid";
        return rc;
    }

    /* Get the original configuration */
    vtss_appl_ospf_vlink_conf_t orig_conf;
    rc = OSPF_vlink_conf_get(id, area_id, router_id, &orig_conf, true);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    if (orig_conf.hello_interval != conf->hello_interval ||
        orig_conf.dead_interval != conf->dead_interval ||
        orig_conf.retransmit_interval != conf->retransmit_interval) {
        FrrOspfAreaVirtualLink frr_vlink_conf(area_id, router_id);

        frr_vlink_conf.hello_interval = conf->hello_interval;
        frr_vlink_conf.dead_interval = conf->dead_interval;
        frr_vlink_conf.retransmit_interval = conf->retransmit_interval;

        rc = frr_ospf_area_virtual_link_conf_set(id, frr_vlink_conf);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set virtual link. "
                    "(instance_id = "
                    << id << ", area_id = " << area_id
                    << ", router_id = " << router_id << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Set authentication type
    if (orig_conf.auth_type != conf->auth_type) {
        FrrOspfAreaVirtualLinkAuth frr_vlink_auth_conf;
        frr_vlink_auth_conf.virtual_link = {area_id, router_id};
        frr_vlink_auth_conf.auth_mode =
            frr_ospf_auth_type_mapping(conf->auth_type);
        rc = frr_ospf_area_virtual_link_authentication_conf_set(
                 id, frr_vlink_auth_conf);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set "
                    "virtual authentication type. "
                    "(instance_id = "
                    << id << ", area_id = " << area_id
                    << ", router_id = " << router_id << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Set authentication simple password
    if (!conf->is_encrypted) {
        strcpy(unencrypted_key, conf->simple_pwd);
    }

    if (strcmp(orig_conf.simple_pwd, unencrypted_key)) {
        FrrOspfAreaVirtualLink frr_vlink_key(area_id, router_id);
        if (strlen(unencrypted_key)) {
            rc = frr_ospf_area_virtual_link_authentication_key_conf_set(
                     id, frr_vlink_key, unencrypted_key);
        } else {
            rc = frr_ospf_area_virtual_link_authentication_key_conf_del(
                     id, {frr_vlink_key, orig_conf.simple_pwd});
        }

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set "
                    "virtual authentication simple password. "
                    "(instance_id = "
                    << id << ", area_id = " << area_id
                    << ", router_id = " << router_id << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Delete a specific virtual link.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] The area ID of the configuration.
 * \param router_id [IN] The destination router id of virtual link.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_vlink_conf_del(const vtss_appl_ospf_id_t id,
                                      const vtss_appl_ospf_area_id_t area_id,
                                      const vtss_appl_ospf_router_id_t router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the entry is existing or not */
    vtss_appl_ospf_vlink_conf_t conf;
    mesa_rc rc = OSPF_vlink_conf_get(id, area_id, router_id, &conf, true);
    if (rc != VTSS_RC_OK) {
        // For the deleting operation, quit silently when it does not exists
        return VTSS_RC_OK;
    }

    /* Apply to FRR layer */
    FrrOspfAreaVirtualLink frr_vlink_key(area_id, router_id);
    rc = frr_ospf_area_virtual_link_conf_del(id, frr_vlink_key);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete virtual link. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", router_id = " << router_id << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

static mesa_rc OSPF_vlink_itr_k3(const vtss_appl_ospf_router_id_t *const prev,
                                 vtss_appl_ospf_router_id_t *const next,
                                 vtss_appl_ospf_id_t id,
                                 vtss_appl_ospf_area_id_t area)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_vlink = frr_ospf_area_virtual_link_conf_get(running_conf, id);

    vtss::Set<vtss_appl_ospf_router_id_t> key3_set;
    for (const auto &i : frr_vlink) {
        if (i.area == area) {
            key3_set.insert(i.dst);
        }
    }

    auto i = prev ? key3_set.greater_than(*prev) : key3_set.begin();
    if (i == key3_set.end()) {
        VTSS_TRACE(DEBUG) << "NOT_FOUND";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    *next = *i;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_ospf_vlink_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_area_id_t *const current_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_router_id_t *const current_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_area_id_t,
         vtss_appl_ospf_router_id_t>
         itr(OSPF_inst_itr, OSPF_area_status_itr_k2, OSPF_vlink_itr_k3);

    return itr(current_id, next_id, current_area_id, next_area_id,
               current_router_id, next_router_id);
}

//----------------------------------------------------------------------------
//** OSPF virtual link authentication: message digest key
//----------------------------------------------------------------------------
/**
 * \brief Get the default configuration of message digest key for the specific
 * virtual
 * link.
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
mesa_rc frr_ospf_vlink_md_key_conf_def(
    vtss_appl_ospf_id_t *const id, vtss_appl_ospf_area_id_t *const area_id,
    vtss_appl_ospf_router_id_t *const router_id,
    vtss_appl_ospf_md_key_id_t *const key_id,
    vtss_appl_ospf_auth_digest_key_t *const md_key)
{
    /* Check illegal parameters */
    if (!md_key) {
        VTSS_TRACE(ERROR) << "Parameter 'md_key' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*md_key);

    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF_INSTANCE_ID_START;

    return VTSS_RC_OK;
}

/**
 * \brief Get the message digest key for the specific virtual link.
 * \param id             [IN]  OSPF instance ID.
 * \param area_id        [IN]  OSPF area ID.
 * \param router_id      [IN]  OSPF router ID.
 * \param key_id         [IN]  The message digest key ID.
 * \param md_key         [OUT] The message digest key configuration.
 * \param get_plain_text [IN]  Output plain text in 'md_key' parameter.
 * \return Error code.
 */
static mesa_rc OSPF_vlink_md_key_conf_get(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id,
    const vtss_appl_ospf_md_key_id_t key_id,
    vtss_appl_ospf_auth_digest_key_t *const md_key,
    const mesa_bool_t get_plain_text)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!md_key) {
        VTSS_TRACE(ERROR) << "Parameter 'md_key' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (key_id < VTSS_APPL_OSPF_AUTH_DIGEST_KEY_ID_MIN) {
        VTSS_TRACE(DEBUG) << "Parameter 'key_id'(" << key_id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_vlink_conf = frr_ospf_area_virtual_link_message_digest_conf_get(running_conf, id);
    if (frr_vlink_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty area range";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    for (const auto &itr : frr_vlink_conf) {
        if (itr.virtual_link.area == area_id &&
            itr.virtual_link.dst == router_id && itr.digest_data.keyid == key_id) {
            // Found it
            if (get_plain_text) {
                strcpy(md_key->digest_key, itr.digest_data.key.c_str());
                md_key->is_encrypted = false;
            } else {
                char unencrypted_key[VTSS_APPL_OSPF_AUTH_DIGEST_KEY_MAX_LEN + 1];
                strcpy(unencrypted_key, itr.digest_data.key.c_str());
                mesa_rc rc = frr_util_secret_key_cryptography(
                                 true /* encrypt */, unencrypted_key,
                                 sizeof(md_key->digest_key), md_key->digest_key);
                if (rc != VTSS_RC_OK) {
                    return rc;
                }

                md_key->is_encrypted = true;

                VTSS_TRACE(DEBUG) << "Found entry: Virtual link area "
                                  "message digest entry. (instance_id = "
                                  << id << ", area_id = " << area_id
                                  << ", router_id = " << router_id
                                  << ", key_id = " << key_id << ")";
            }

            return VTSS_RC_OK;
        }
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

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
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id,
    const vtss_appl_ospf_md_key_id_t key_id,
    vtss_appl_ospf_auth_digest_key_t *const md_key)
{
    CRIT_SCOPE();
    return OSPF_vlink_md_key_conf_get(id, area_id, router_id, key_id, md_key,
                                      false);
}

/**
 * \brief Set the message digest key for the specific virtual link.
 * It is a dummy function for SNMP serialzer only.
 */
mesa_rc frr_ospf_vlink_md_key_conf_dummy_set(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id,
    const vtss_appl_ospf_md_key_id_t key_id,
    const vtss_appl_ospf_auth_digest_key_t *const md_key)
{
    // The 'SET' operation is unsupported.
    // It requires delete it first.
    return FRR_RC_NOT_SUPPORTED;
}

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
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id,
    const vtss_appl_ospf_md_key_id_t key_id,
    const vtss_appl_ospf_auth_digest_key_t *const md_key)
{
    CRIT_SCOPE();

    /* Backbone area can't be configured as virtual link. */
    if (area_id == FRR_OSPF_BACKBONE_AREA_ID) {
        return VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_BACKBONE;
    }

    /* Check illegal parameters */
    if (!md_key) {
        VTSS_TRACE(ERROR) << "Parameter 'md_key' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (key_id < VTSS_APPL_OSPF_AUTH_DIGEST_KEY_ID_MIN) {
        VTSS_TRACE(DEBUG) << "Parameter 'key_id'(" << key_id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check message digest key */
    char unencrypted_key[VTSS_APPL_OSPF_AUTH_SIMPLE_KEY_MAX_LEN + 1] = "";
    mesa_rc rc = frr_ospf_validate_secret_key(
                     VTSS_APPL_OSPF_AUTH_TYPE_MD5, md_key->is_encrypted,
                     md_key->digest_key, unencrypted_key);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Parameter 'md_key' is invalid";
        return rc;
    }

    /* Check the entry is existing or not */
    vtss_appl_ospf_auth_digest_key_t orig_md_key;
    rc = OSPF_vlink_md_key_conf_get(id, area_id, router_id, key_id,
                                    &orig_md_key, true);
    if (rc == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    /* Apply to FRR layer when the entry is a new one. */
    FrrOspfAreaVirtualLink frr_vlink_key(area_id, router_id);
    FrrOspfDigestData frr_md_key(
        key_id,
        md_key->is_encrypted ? (unencrypted_key) : (md_key->digest_key));

    rc = frr_ospf_area_virtual_link_message_digest_conf_set(id, frr_vlink_key,
                                                            frr_md_key);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Add virtual link "
                          "message digest key entry. (instance_id = "
                          << id << ", area_id = " << area_id
                          << ", router_id = " << router_id << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id,
    const vtss_appl_ospf_md_key_id_t key_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (key_id < VTSS_APPL_OSPF_AUTH_DIGEST_KEY_ID_MIN) {
        VTSS_TRACE(DEBUG) << "Parameter 'key_id'(" << key_id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the entry is existing or not */
    vtss_appl_ospf_auth_digest_key_t md_key;
    mesa_rc rc = OSPF_vlink_md_key_conf_get(id, area_id, router_id, key_id,
                                            &md_key, true);
    if (rc != VTSS_RC_OK) {
        // For the deleting operation, quit silently when it does not exists
        return VTSS_RC_OK;
    }

    /* Apply to FRR layer */
    FrrOspfAreaVirtualLink frr_vlink_key(area_id, router_id);
    FrrOspfDigestData frr_md_key(key_id, md_key.digest_key);

    rc = frr_ospf_area_virtual_link_message_digest_conf_del(id, frr_vlink_key,
                                                            frr_md_key);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Delete area authentication. "
                "(instance_id = "
                << id << ", area_id = " << area_id
                << ", router_id = " << router_id << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

static mesa_rc OSPF_vlink_md_key_itr_k4(
    const vtss_appl_ospf_md_key_id_t *const prev,
    vtss_appl_ospf_md_key_id_t *const next, vtss_appl_ospf_id_t id,
    vtss_appl_ospf_area_id_t area, vtss_appl_ospf_router_id_t router_id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_vlink_conf = frr_ospf_area_virtual_link_message_digest_conf_get(running_conf, id);

    vtss::Set<vtss_appl_ospf_md_key_id_t> key4_set;
    for (const auto &i : frr_vlink_conf) {
        if (i.virtual_link.area == area && i.virtual_link.dst == router_id) {
            key4_set.insert(i.digest_data.keyid);
        }
    }

    auto i = prev ? key4_set.greater_than(*prev) : key4_set.begin();
    if (i == key4_set.end()) {
        VTSS_TRACE(DEBUG) << "NOT_FOUND";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    *next = *i;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_ospf_vlink_md_key_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_area_id_t *const current_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_router_id_t *const current_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id,
    const vtss_appl_ospf_md_key_id_t *const current_key_id,
    vtss_appl_ospf_md_key_id_t *const next_key_id)
{
    CRIT_SCOPE();

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_area_id_t,
         vtss_appl_ospf_router_id_t, vtss_appl_ospf_md_key_id_t>
         itr(OSPF_inst_itr, OSPF_area_status_itr_k2, OSPF_vlink_itr_k3,
             OSPF_vlink_md_key_itr_k4);

    return itr(current_id, next_id, current_area_id, next_area_id,
               current_router_id, next_router_id, current_key_id, next_key_id);
}

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
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id, const uint32_t precedence,
    vtss_appl_ospf_md_key_id_t *const key_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!key_id) {
        VTSS_TRACE(ERROR) << "Parameter 'key_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (precedence < VTSS_APPL_OSPF_MD_KEY_PRECEDENCE_START) {
        VTSS_TRACE(DEBUG) << "Parameter 'precedence'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_vlink_conf = frr_ospf_area_virtual_link_message_digest_conf_get(running_conf, id);
    if (frr_vlink_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty area range";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    if (precedence > frr_vlink_conf.size()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    int array_idx = VTSS_APPL_OSPF_MD_KEY_PRECEDENCE_START;
    for (const auto &itr : frr_vlink_conf) {
        if (itr.virtual_link.area == area_id &&
            itr.virtual_link.dst == router_id && array_idx == precedence) {
            // Found it
            *key_id = itr.digest_data.keyid;
            VTSS_TRACE(DEBUG) << "Found entry: Virtual link area "
                              "authentication. (instance_id = "
                              << id << ", area_id = " << area_id
                              << ", router_id = " << router_id
                              << ", precedence = " << precedence << ")";
            return VTSS_RC_OK;
        }

        array_idx++;
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

static mesa_rc OSPF_vlink_md_key_itr_precedence_k4(
    const uint32_t *const prev, uint32_t *const next, vtss_appl_ospf_id_t id,
    vtss_appl_ospf_area_id_t area, vtss_appl_ospf_router_id_t router_id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    auto frr_vlink_conf = frr_ospf_area_virtual_link_message_digest_conf_get(running_conf, id);

    uint32_t idx = VTSS_APPL_OSPF_MD_KEY_PRECEDENCE_START;
    for (const auto &i : frr_vlink_conf) {
        if (area == i.virtual_link.area && router_id == i.virtual_link.dst) {
            if (!prev || idx > *prev) {
                *next = idx;
                return VTSS_RC_OK;
            }
            ++idx;
        }
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

mesa_rc vtss_appl_ospf_vlink_md_key_precedence_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_area_id_t *const current_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_router_id_t *const current_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id,
    const uint32_t *const current_precedence,
    uint32_t *const next_precedence)
{
    CRIT_SCOPE();

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_area_id_t,
         vtss_appl_ospf_router_id_t, uint32_t>
         itr(OSPF_inst_itr, OSPF_area_status_itr_k2, OSPF_vlink_itr_k3,
             OSPF_vlink_md_key_itr_precedence_k4);

    return itr(current_id, next_id, current_area_id, next_area_id,
               current_router_id, next_router_id, current_precedence,
               next_precedence);
}

//----------------------------------------------------------------------------
//** OSPF stub area
//----------------------------------------------------------------------------
/**
 * \brief
 * \param id      [IN]  OSPF instance ID.
 * \param area_id [IN]  OSPF area ID.
 * \param conf    [OUT] OSPF area stub configuration.
 * \return Error code. The function doesn't validate the parameter
 * since it's only invoked locally.
 */
static mesa_rc OSPF_stub_area_conf_get(const vtss_appl_ospf_id_t id,
                                       const vtss_appl_ospf_area_id_t area_id,
                                       vtss_appl_ospf_stub_area_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer. */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    /* Lookup the matched entry in frr_stub_conf,
     * then lookup it again in frr_stub_no_summary_conf if not found.
     */
    auto frr_stub_area_conf = frr_ospf_stub_area_conf_get(running_conf, id);
    if (!frr_stub_area_conf.empty()) {
        for (const auto &itr : frr_stub_area_conf) {
            if (itr.area > area_id) {
                break;
            }

            if (itr.area == area_id) {
                // Found it
                conf->is_nssa = itr.is_nssa;
                conf->no_summary = itr.no_summary;
                conf->nssa_translator_role =
                    itr.nssa_translator_role == NssaTranslatorRoleCandidate
                    ? VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_CANDIDATE
                    : itr.nssa_translator_role ==
                    NssaTranslatorRoleAlways
                    ? VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_ALWAYS
                    : VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_NEVER;
                VTSS_TRACE(DEBUG)
                        << "Found entry: "
                        << (itr.is_nssa ? "NSSA" : "stub area")
                        << ". (instance_id = " << id << ", area_id = " << area_id
                        << ", no_summary =" << conf->no_summary << ")";
                return VTSS_RC_OK;
            }
        }
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

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
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_stub_area_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->nssa_translator_role >= VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_COUNT) {
        VTSS_TRACE(ERROR) << "Invalid parameter: nssa_translator_role";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Backbone area can't be configured as stub area. */
    if (area_id == FRR_OSPF_BACKBONE_AREA_ID) {
        return VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE;
    }

    /* Check if the virtual link entry is existing or not */
    mesa_rc rc;
    rc = OSPF_vlink_simple_find(id, area_id);
    if (rc == VTSS_RC_OK) {
        return VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_VIRTUAL_LINK;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    /* Check the entry is existing or not. */
    vtss_appl_ospf_stub_area_conf_t orig_conf;
    rc = OSPF_stub_area_conf_get(id, area_id, &orig_conf);
    if (rc == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    FrrOspfStubArea frr_stub_area_conf(area_id, conf->is_nssa, conf->no_summary,
                                       NssaTranslatorRoleCandidate);
    if (conf->is_nssa) {
        frr_stub_area_conf.nssa_translator_role =
            conf->nssa_translator_role ==
            VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_CANDIDATE
            ? NssaTranslatorRoleCandidate
            : conf->nssa_translator_role ==
            VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_ALWAYS
            ? NssaTranslatorRoleAlways
            : NssaTranslatorRoleNever;
    }

    rc = frr_ospf_stub_area_conf_set(id, frr_stub_area_conf);

    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Add stub area. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", no_summary =" << conf->no_summary << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_stub_area_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->nssa_translator_role >= VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_COUNT) {
        VTSS_TRACE(ERROR) << "Invalid parameter: nssa_translator_role";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Backbone area can't be configured as stub area. */
    if (area_id == FRR_OSPF_BACKBONE_AREA_ID) {
        return VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE;
    }

    /* Check the entry is existing or not. */
    mesa_rc rc;
    vtss_appl_ospf_stub_area_conf_t orig_conf;
    rc = OSPF_stub_area_conf_get(id, area_id, &orig_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Return OK directly since no changes. */
    if (orig_conf.is_nssa == conf->is_nssa &&
        orig_conf.no_summary == conf->no_summary &&
        orig_conf.nssa_translator_role == conf->nssa_translator_role) {
        return VTSS_RC_OK;
    }

    FrrOspfStubArea frr_stub_area_conf(area_id, conf->is_nssa, conf->no_summary,
                                       NssaTranslatorRoleCandidate);
    if (conf->is_nssa) {
        frr_stub_area_conf.nssa_translator_role =
            conf->nssa_translator_role ==
            VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_CANDIDATE
            ? NssaTranslatorRoleCandidate
            : conf->nssa_translator_role ==
            VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_ALWAYS
            ? NssaTranslatorRoleAlways
            : NssaTranslatorRoleNever;
    }

    rc = frr_ospf_stub_area_conf_set(id, frr_stub_area_conf);

    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set stub area. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", no_summary =" << conf->no_summary << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the configuration for a specific stub area.
 * \param id      [IN]  OSPF instance ID.
 * \param area_id [IN]  The area ID of the stub area configuration.
 * \param conf    [OUT] The stub area configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_stub_area_conf_get(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    vtss_appl_ospf_stub_area_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    return OSPF_stub_area_conf_get(id, area_id, conf);
}

/**
 * \brief Delete a specific stub area.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf_stub_area_conf_del(const vtss_appl_ospf_id_t id,
                                          const vtss_appl_ospf_area_id_t area_id)
{
    CRIT_SCOPE();

    /* Check the entry is existing or not. */
    mesa_rc rc;
    vtss_appl_ospf_stub_area_conf_t orig_conf;
    rc = OSPF_stub_area_conf_get(id, area_id, &orig_conf);

    /* Return OK silently if not found. */
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete stub area. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id << ", \'"
                          << error_txt(rc) << "\')";

        return rc == FRR_RC_ENTRY_NOT_FOUND ? VTSS_RC_OK : rc;
    }

    rc = frr_ospf_stub_area_conf_del(id, area_id);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete stub area. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", no_summary =" << orig_conf.no_summary << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the default configuration for a specific stub areas.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN]  The area ID of the stub area configuration.
 * \param conf    [OUT] The stub area configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_stub_area_conf_def(vtss_appl_ospf_id_t *const id,
                                    vtss_appl_ospf_area_id_t *const area_id,
                                    vtss_appl_ospf_stub_area_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF_INSTANCE_ID_START;

    return VTSS_RC_OK;
}

static mesa_rc OSPF_stub_area_conf_itr_k2(const vtss_appl_ospf_area_id_t *const cur,
                                          vtss_appl_ospf_area_id_t *const next,
                                          vtss_appl_ospf_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    /* Get data from FRR layer */
    auto frr_stub_area_conf = frr_ospf_stub_area_conf_get(running_conf, key1);
    if (frr_stub_area_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    /* Build the local sorted database for key2
     * The stub area database should include the stub areas and
     * totally stub areas both. */
    vtss::Set<vtss_appl_ospf_area_id_t> key2_set;
    for (const auto &itr : frr_stub_area_conf) {
        key2_set.insert(itr.area);
    }

    Set<vtss_appl_ospf_area_id_t>::iterator key2_itr;
    if (!cur) {  // Get-First operation
        key2_itr = key2_set.begin();
    } else {  // Get-Next operation
        key2_itr = key2_set.greater_than(*cur);
    }

    if (key2_itr != key2_set.end()) {
        VTSS_TRACE(DEBUG) << "Found: key1: " << key1 << ", key2 = " << *key2_itr;
        *next = *key2_itr;
        return VTSS_RC_OK;  // Found it, break the loop
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate the stub areas.
 * \param current_id      [IN]  Current OSPF ID
 * \param next_id         [OUT] Next OSPF ID
 * \param current_area_id [IN]  Current area ID
 * \param next_area_id    [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_stub_area_conf_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_area_id_t *const current_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_area_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_area_id' or "
                          "'next_network' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_area_id_t> itr(
        OSPF_inst_itr, OSPF_stub_area_conf_itr_k2);

    return itr(current_id, next_id, current_area_id, next_area_id);
}

//----------------------------------------------------------------------------
//** OSPF interface parameter tuning
//----------------------------------------------------------------------------
/**
 * \brief Get the OSPF VLAN interface default configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF VLAN interface configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_intf_conf_def(vtss_ifindex_t *const ifindex,
                               vtss_appl_ospf_intf_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero default values below
    conf->priority = FRR_OSPF_DEF_PRIORITY;
    conf->is_specific_cost = false;
    conf->cost = VTSS_APPL_OSPF_INTF_COST_MIN;
    conf->mtu_ignore = false;
    conf->is_fast_hello_enabled = false;
    conf->fast_hello_packets = FRR_OSPF_DEF_FAST_HELLO_PKTS;
    conf->dead_interval = FRR_OSPF_DEF_DEAD_INTERVAL;              // in seconds
    conf->hello_interval = FRR_OSPF_DEF_HELLO_INTERVAL;            // in seconds
    conf->retransmit_interval = FRR_OSPF_DEF_RETRANSMIT_INTERVAL;  // in seconds
    OSPF_intf_auth_conf_def(ifindex, conf);

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF VLAN interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF VLAN interface configuration.
 * \return Error code.
 */
static mesa_rc OSPF_intf_conf_get(const vtss_ifindex_t ifindex, vtss_appl_ospf_intf_conf_t *const conf)
{
    vtss_appl_ospf_intf_conf_t def_conf;
    mesa_rc                    rc;

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

    /* Check the interface is existing or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF, running_conf));

    // priority
    auto frr_intf_priority = frr_ospf_if_priority_conf_get(running_conf, ifindex);
    if (frr_intf_priority.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get interface priority. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_priority.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->priority = frr_intf_priority.val;

    // cost
    auto frr_intf_cost = frr_ospf_if_cost_conf_get(running_conf, ifindex);
    if (frr_intf_cost.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get interface cost. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_cost.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->is_specific_cost = (frr_intf_cost.val == 0) ? false : true;
    if (conf->is_specific_cost) {
        conf->cost = frr_intf_cost.val;
    } else {
        conf->cost = VTSS_APPL_OSPF_INTF_COST_MIN;
    }

    // mtu-ignore
    conf->mtu_ignore = frr_ospf_if_mtu_ignore_conf_get(running_conf, ifindex).val;

    // dead-interval
    auto frr_intf_dead_interval = frr_ospf_if_dead_interval_conf_get(running_conf, ifindex);
    if (frr_intf_dead_interval.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get interface dead interval. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_dead_interval.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    VTSS_RC(frr_ospf_intf_conf_def((vtss_ifindex_t *const) & ifindex, &def_conf));

    // If 'minimal multiplier' is set, that means 'fast hello' is enabled
    conf->is_fast_hello_enabled = frr_intf_dead_interval.val.multiplier;

    // If 'fast hello' is disabled, 'fast hello packets' is assigned the default
    // value
    conf->fast_hello_packets = frr_intf_dead_interval.val.multiplier ? frr_intf_dead_interval.val.val : def_conf.fast_hello_packets;

    // If 'fast hello' is enabled, dead-interval is assigned 1 sec
    conf->dead_interval = frr_intf_dead_interval.val.multiplier ? 1 : frr_intf_dead_interval.val.val;

    // hello-interval
    auto frr_intf_hello_interval = frr_ospf_if_hello_interval_conf_get(running_conf, ifindex);
    if (frr_intf_hello_interval.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get interface hello interval. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_hello_interval.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->hello_interval = frr_intf_hello_interval.val;

    // retransmit-interval
    auto frr_intf_retransmit_interval = frr_ospf_if_retransmit_interval_conf_get(running_conf, ifindex);
    if (frr_intf_retransmit_interval.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get interface retransmit interval. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_retransmit_interval.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->retransmit_interval = frr_intf_retransmit_interval.val;

    // Get authenticaton configuration, and detemine to save the authentication
    // key as
    // encrypted or not according to 'conf->is_encrypted'. Although
    // 'conf->is_encrypted'
    // can be assigned by end-user, but we will override it internally.
    if ((rc = OSPF_intf_auth_conf_get(running_conf, ifindex, conf->is_encrypted, conf)) != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get interface authentication mode. (ifindex = %s, rc = %s)", ifindex, error_txt(rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF VLAN interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_conf_get(const vtss_ifindex_t ifindex,
                                     vtss_appl_ospf_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // 'conf' must not be changed if any errors from OSPF_intf_conf_get().
    vtss_appl_ospf_intf_conf_t buf;
    // Enforce to get 'auth_key' as encrypted.
    // If 'auth_key' got is null string, 'is_encrypted' will return 'false'.
    buf.is_encrypted = true;
    auto rc = OSPF_intf_conf_get(ifindex, &buf);
    if (rc == VTSS_RC_OK) {
        *conf = buf;
    }

    return rc;
}

/**
 * \brief Set the OSPF VLAN interface configuration.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] OSPF VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_conf_set(const vtss_ifindex_t ifindex,
                                     const vtss_appl_ospf_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    mesa_rc rc;
    vtss_appl_ospf_intf_conf_t orig_conf;

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

    if (conf->priority > VTSS_APPL_OSPF_PRIORITY_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'priority'(" << conf->priority
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->is_specific_cost && (conf->cost < VTSS_APPL_OSPF_INTF_COST_MIN ||
                                   conf->cost > VTSS_APPL_OSPF_INTF_COST_MAX)) {
        VTSS_TRACE(DEBUG) << "Parameter 'cost'(" << conf->cost
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->hello_interval < VTSS_APPL_OSPF_HELLO_INTERVAL_MIN ||
        conf->hello_interval > VTSS_APPL_OSPF_HELLO_INTERVAL_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'hello_interval'("
                          << conf->hello_interval << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->is_fast_hello_enabled &&
        (conf->fast_hello_packets < VTSS_APPL_OSPF_FAST_HELLO_MIN ||
         conf->fast_hello_packets > VTSS_APPL_OSPF_FAST_HELLO_MAX)) {
        VTSS_TRACE(DEBUG) << "Parameter 'fast_hello_packets'("
                          << conf->fast_hello_packets << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->dead_interval < VTSS_APPL_OSPF_DEAD_INTERVAL_MIN ||
        conf->dead_interval > VTSS_APPL_OSPF_DEAD_INTERVAL_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'dead_interval'(" << conf->dead_interval
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->retransmit_interval < VTSS_APPL_OSPF_RETRANSMIT_INTERVAL_MIN ||
        conf->retransmit_interval > VTSS_APPL_OSPF_RETRANSMIT_INTERVAL_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'retransmit_interval'("
                          << conf->retransmit_interval << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the interface is existing or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get the original configuration */
    // Get auth_key as plain text to compare it with FRR layer config.
    orig_conf.is_encrypted = false;
    if ((rc = OSPF_intf_conf_get(ifindex, &orig_conf)) != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface param. "
                          "(rc = "
                          << rc << ")";
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    // priority
    if (conf->priority != orig_conf.priority) {
        if ((rc = frr_ospf_if_priority_conf_set(ifindex, conf->priority)) !=
            VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface cost. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }
    }

    // cost
    if (conf->is_specific_cost != orig_conf.is_specific_cost ||
        conf->cost != orig_conf.cost) {
        if (conf->is_specific_cost &&
            (rc = frr_ospf_if_cost_conf_set(ifindex, conf->cost)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface cost. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }

        if (!conf->is_specific_cost &&
            (rc = frr_ospf_if_cost_conf_del(ifindex)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Del interface cost. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }
    }

    // mtu-ignore
    if (conf->mtu_ignore != orig_conf.mtu_ignore) {
        if ((rc = frr_ospf_if_mtu_ignore_conf_set(ifindex, conf->mtu_ignore)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface mtu-ignore. (rc = " << rc << ")";
            return rc;
        }
    }

    // fast hello packets
    bool apply_new_dead_interval = false;
    if (conf->is_fast_hello_enabled != orig_conf.is_fast_hello_enabled ||
        conf->fast_hello_packets != orig_conf.fast_hello_packets) {
        // when fast hello is enabled, set the 'minimal hello-multiplier'
        if (conf->is_fast_hello_enabled) {
            if ((rc = frr_ospf_if_dead_interval_minimal_conf_set(
                          ifindex, conf->fast_hello_packets)) != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Access framework failed: Set interface fast hello. "
                        "(rc = "
                        << rc << ")";
                return rc;
            }
        } else {
            apply_new_dead_interval = true;
        }
    }

    // dead interval
    if (apply_new_dead_interval ||
        (conf->dead_interval != orig_conf.dead_interval)) {
        if ((rc = frr_ospf_if_dead_interval_conf_set(
                      ifindex, conf->dead_interval)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set interface dead interval. "
                    "(rc = "
                    << rc << ")";
            return rc;
        }
    }

    // hello-interval
    if (conf->hello_interval != orig_conf.hello_interval) {
        if ((rc = frr_ospf_if_hello_interval_conf_set(
                      ifindex, conf->hello_interval)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set interface hello interval. "
                    "(rc = "
                    << rc << ")";
            return rc;
        }
    }

    // retransmit-interval
    if (conf->retransmit_interval != orig_conf.retransmit_interval) {
        if ((rc = frr_ospf_if_retransmit_interval_conf_set(
                      ifindex, conf->retransmit_interval)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface "
                              "retransmit interval. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }
    }

    if ((rc = OSPF_intf_auth_conf_set(ifindex, &orig_conf, conf)) != VTSS_RC_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all OSPF VLAN interfaces.
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_intf_conf_itr(const vtss_ifindex_t *const current_ifindex, vtss_ifindex_t *const next_ifindex)
{
    return OSPF_if_itr(current_ifindex, next_ifindex);
}

//------------------------------------------------------------------------------
//** OSPF interface status
//------------------------------------------------------------------------------
/**
 * \brief Iterator through the interface in the ospf
 *
 * \param prev      [IN]    Ifindex to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according
 * to the given IN value.
 * This iterator is used by CLI and Web.
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_ospf_interface_itr(const vtss_ifindex_t *const prev,
                                     vtss_ifindex_t *const next)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next) {
        VTSS_TRACE(ERROR) << "Parameter 'next' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_interface_status_get();
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
    Map<vtss_ifindex_t, FrrIpOspfIfStatus>::iterator itr;
    if (prev) {  // Get-Next operation
        itr = result_list->greater_than(*prev);
    } else {  // Get-First operation
        itr = result_list->begin();
    }

    if (itr != result_list->end()) {
        // Found it
        *next = itr->first;
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

template <typename Type>
struct FrrCachedResult {
    using GetFunction = std::function<FrrRes<Type>()>;

    FrrCachedResult(GetFunction fct)
        : _get_fct {fct}, _result {MESA_RC_INV_STATE}, _is_cached {false} {}

    explicit operator bool()
    {
        return _result.rc == VTSS_RC_OK;
    }

    Type &operator()()
    {
        return _result.val;
    }

    /* Update cache */
    FrrRes<Type> &update()
    {
        _result = std::move(_get_fct());
        _is_cached = _result.rc == VTSS_RC_OK;
        return _result;
    }

    /* Invalidate cache */
    void invalidate()
    {
        _is_cached = false;
    }

    FrrRes<Type> &result()
    {
        if (!_is_cached) {
            return update();
        }

        return _result;
    }

private:
    GetFunction _get_fct;
    FrrRes<Type> _result;
    bool _is_cached;
};

/* Temporary local database for speed up the processing time.
 *
 * The following database may be refered many times in the same API.
 * It takes a long processing time in a large database.
 */
using NeighborStatusMap = Map<mesa_ipv4_t, Vector<FrrIpOspfNeighborStatus>>;
using CachedNeighborStatus = FrrCachedResult<NeighborStatusMap>;
CachedNeighborStatus OSPF_frr_ospf_nbr_status_cache {
    frr_ip_ospf_neighbor_status_get
};

using InterfaceStatusMap = Map<vtss_ifindex_t, FrrIpOspfIfStatus>;
using CachedInterfaceStatus = FrrCachedResult<InterfaceStatusMap>;
CachedInterfaceStatus OSPF_frr_ospf_intf_status_cache {
    frr_ip_ospf_interface_status_get
};

/* OSPF interface status: iterate key1 */
static mesa_rc OSPF_interface_status_itr2_k1(const mesa_ipv4_t *const current_addr,
                                             mesa_ipv4_t *const next_addr)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_addr_valid = false;
    mesa_ipv4_t ifst_addr = {};

    for (const auto &i : OSPF_frr_ospf_intf_status_cache()) {
        const auto &ifst = i.second;

        // output the IP addr as 0 for the virtual interface entry
        if (vtss_ifindex_is_frr_vlink(i.first)) {
            ifst_addr = 0;
        } else {
            ifst_addr = ifst.net.address;
        }

        if (current_addr && ifst_addr <= *current_addr) {
            continue;
        }

        if (next_addr_valid && ifst_addr >= *next_addr) {
            continue;
        }

        *next_addr = ifst_addr;
        next_addr_valid = true;
    }

    if (next_addr_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/* OSPF interface status: iterate key1 */
static mesa_rc OSPF_interface_status_itr2_k2(
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx, mesa_ipv4_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_ifidx_valid = false;
    mesa_ipv4_t ifst_addr = {};

    for (const auto &i : OSPF_frr_ospf_intf_status_cache()) {
        const auto &ifst = i.second;

        // ignore virtual interface because vitrual link's status is differrent
        // from
        // physical's in standard MIB.
        if (vtss_ifindex_is_frr_vlink(i.first)) {
            continue;
        }

        ifst_addr = ifst.net.address;

        if (ifst_addr != key1) {
            continue;
        }

        if (current_ifidx && (i.first <= *current_ifidx)) {
            continue;
        }

        if (next_ifidx_valid && (i.first >= *next_ifidx)) {
            continue;
        }

        *next_ifidx = i.first;
        next_ifidx_valid = true;
    }

    if (next_ifidx_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterator through the interface in the ospf
 *
 * \param current_addr      [IN]    Current interface address.
 *
 * \param current_ifidx     [IN]   Current interface ifindex.
 *
 * \param next_addr         [OUT]    Next interface address.
 *
 * \param next_ifidx        [OUT]   Next interface ifindex.
 *                          Provide a null pointer to get the first neighbor.
 * This iterator is written for MIB. The keys of MIB interface table are IP addr
 * and ifindex.
 * MIB outputs the IP addr as 0 for the virtual interface entry.
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_ospf_interface_itr2(const mesa_ipv4_t *const current_addr,
                                      mesa_ipv4_t *const next_addr,
                                      const vtss_ifindex_t *const current_ifidx,
                                      vtss_ifindex_t *const next_ifidx)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_addr || !next_ifidx) {
        VTSS_TRACE(ERROR)
                << "Parameter 'next_addr' or 'next_ifidx' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto &res = OSPF_frr_ospf_intf_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res.val.empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<mesa_ipv4_t, vtss_ifindex_t> itr(
        OSPF_interface_status_itr2_k1, OSPF_interface_status_itr2_k2);

    auto rc = itr(current_addr, next_addr, current_ifidx, next_ifidx);

    OSPF_frr_ospf_intf_status_cache.invalidate();
    return rc;
}

/* Notice that the result of frr_ip_ospf_interface_status_get() is
 * based on FRR command 'show ip ospf interface', this command output
 * only includes those interfaces which OSPF is enabled.
 * When the interface status is link-down, only parameter'if_up' is
 * insignificant, the others information need to be obtained by other
 * way.
 */
static mesa_rc OSPF_interface_link_down_status_get(vtss_ifindex_t ifindex, vtss_appl_ospf_interface_status_t *status)
{
    vtss_appl_ip_if_key_ipv4_t key = {};

    FRR_CRIT_ASSERT_LOCKED();

    // Given the default initial value
    vtss_clear(*status);

    // Link status
    status->status = false;

    // Interface's IP address
    key.ifindex = ifindex;
    if (vtss_ifindex_is_vlan(ifindex) && (vtss_appl_ip_if_status_ipv4_itr(&key, &key) != VTSS_RC_OK || key.ifindex != ifindex)) {
        T_DG(FRR_TRACE_GRP_OSPF, "Can't find IPv4 addr for intf %s", ifindex);
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    status->network = key.addr;

    // Area ID
    // Look up if the interface IP address matched any the exiting OSPF
    // network domain in order to get the OSPF feature is disabled on this
    // interface or not.
    vtss_appl_ospf_area_id_t area_id = 0;
    if (vtss_ifindex_is_vlan(ifindex) && OSPF_area_conf_get(FRR_OSPF_DEFAULT_INSTANCE_ID, &key.addr, &area_id, false) != VTSS_RC_OK) {
        vtss::AsIpv4 area_id_as_ipv4(area_id);
        T_DG(FRR_TRACE_GRP_OSPF, "Can't find area configuration for %s area id %s", key.addr, &area_id_as_ipv4);
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    status->area_id = area_id;

    // Router ID
    vtss_appl_ospf_router_status_t router_status;
    if (OSPF_router_status_get(FRR_OSPF_DEFAULT_INSTANCE_ID, &router_status) !=
        VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Can't find router status for OSPF instance "
                          << FRR_OSPF_DEFAULT_INSTANCE_ID;
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    // Default interface state
    status->state = VTSS_APPL_OSPF_INTERFACE_DOWN;

    status->router_id = router_status.ospf_router_id;

    // TODO. Given a default value in M1. These values should be updated in M2.
    status->cost = 10;

    status->priority = FRR_OSPF_DEF_PRIORITY;
    status->transmit_delay = FRR_OSPF_DEF_TRANSMIT_DELAY;

    /* Notice that the timer parameters:
     * hello_time, dead_time and retransmit_time will equal the current
     * configured timer interval setting when the interface state is down. */
    vtss_appl_ospf_intf_conf_t intf_conf;
    intf_conf.is_encrypted = false;  // Get auth_key as plain text
    if (!vtss_ifindex_is_frr_vlink(ifindex) &&
        OSPF_intf_conf_get(ifindex, &intf_conf) == VTSS_RC_OK) {
        status->hello_time = intf_conf.hello_interval;
        status->dead_time = intf_conf.dead_interval;
        status->retransmit_time = intf_conf.retransmit_interval;
    } else {
        status->hello_time = FRR_OSPF_DEF_HELLO_INTERVAL;
        status->dead_time = FRR_OSPF_DEF_DEAD_INTERVAL;
        status->retransmit_time = FRR_OSPF_DEF_RETRANSMIT_INTERVAL;
    }

    return VTSS_RC_OK;
}

/* Get the DR/BDR neighbor ID from its IP address.
 * The function is designed due to there is no DR/BDR ID information in the
 * FRR command "show ip ospf neighbor detail json".
 *
 * Return a none-zero value when the DR/BDR neighbor ID is found, otherwise,
 * return the zero ID(0.0.0.0).
 *
 * Lookup procedures:
 * 1. Check the searching IP address is mine or not (by looking in myself
 *    OSPF interface table)
 * 2. When item 1) is failed, lookup the searching IP address in OSPF
 *    neighbor table.
 */
static mesa_ipv4_t OSPF_nbr_lookup_id_by_addr(const mesa_ipv4_t ip_addr)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (ip_addr == 0) {
        return (mesa_ipv4_t)0;
    }

    /* 1. Check the searching IP address is mine or not (by looking in myself
     *    OSPF interface table) */
    auto &intf_res = OSPF_frr_ospf_intf_status_cache.result();
    if (!intf_res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface status. "
                          "(rc = "
                          << intf_res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    for (const auto &itr : intf_res.val) {
        auto &itr_status = itr.second;
        if (itr_status.net.address == ip_addr) {
            return itr_status.router_id;
        }

        if (itr_status.bdr_address == ip_addr) {
            return itr_status.bdr_id;
        }
    }

    /* 2. When item 1) is failed, lookup the searching IP address in OSPF
     *    neighbor table. */
    auto &res = OSPF_frr_ospf_nbr_status_cache.result();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    for (const auto &itr : res.val) {
        auto &itr_status = itr.second;
        for (const auto &l : itr_status) {
            if (l.if_address != ip_addr) {
                continue;
            }

            return itr.first;
        }
    }

    return 0;
}

/**
 * \brief Get neighbor ID by neighbor IP address.
 * \param ip_addr   [IN] Neighbor IP address to query.
 * \return the Neighbor router ID or zero address if not found.
 */
vtss_appl_ospf_router_id_t vtss_appl_ospf_nbr_lookup_id_by_addr(
    const mesa_ipv4_t ip_addr)
{
    CRIT_SCOPE();

    return OSPF_nbr_lookup_id_by_addr(ip_addr);
}

/* Get the DR addr from neighbor database through function
 * frr_ip_ospf_neighbor_status_get().
 * To obtain state Full/Dr -
 *         the ifaceAddress must be equal with routerDesignatedId.
 * To obtain state: Full/Backup -
 *         the ifaceAddress must be equal with routerDesignatedBackupId.
 * We should check the Dr data and return the ifaceAddress and router ID.
 * If no entry find, we will return zero address.
 */
static mesa_rc OSPF_nbr_lookup_dr(const vtss_ifindex_t ifindex,
                                  mesa_ipv4_t *const dr_addr,
                                  vtss_appl_ospf_router_id_t *const dr_id)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (!dr_addr || !dr_id) {
        VTSS_TRACE(ERROR)
                << "Parameter 'dr_addr' or 'dr_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* The virtual link interface always acts "DROther" role */
    if (vtss_ifindex_is_frr_vlink(ifindex)) {
        *dr_addr = *dr_id = 0;
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_neighbor_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    for (const auto &itr : result_list.val) {
        auto &itr_neighbors = itr.second;
        for (const auto &itr_status : itr_neighbors) {
            VTSS_TRACE(DEBUG) << " if_address: " << itr_status.if_address
                              << " dr_ip_addr: " << itr_status.dr_ip_addr
                              << " itr_status.ifindex: " << itr_status.ifindex
                              << " ifindex: " << ifindex;
            /* Notice that the dr_ip_addr
               represents the IP address of DR on the network.
               So matching the IP address to find DR */
            if ((itr_status.if_address != itr_status.dr_ip_addr) ||
                (itr_status.ifindex != ifindex)) {
                continue;
            }

            VTSS_TRACE(DEBUG) << " done";
            *dr_addr = itr_status.if_address;
            *dr_id = itr.first;
            return VTSS_RC_OK;
        }
    }
    *dr_addr = *dr_id = 0;
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get status for a specific interface.
 * \param ifindex   [IN]  ifindex to query (either VLAN or VLINK)
 * \param status    [OUT] Status for 'key'.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_interface_status_get(const vtss_ifindex_t ifindex, vtss_appl_ospf_interface_status_t *const status)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!status) {
        VTSS_TRACE(ERROR) << "Parameter 'status' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex) && !vtss_ifindex_is_frr_vlink(ifindex)) {
        T_DG(FRR_TRACE_GRP_OSPF, "Interface %s is not valid", ifindex);
        return FRR_RC_INVALID_ARGUMENT;
    }
    /* Lookup the default OSPF instance if existing or not. */
    if (!OSPF_instance_id_existing(FRR_OSPF_DEFAULT_INSTANCE_ID)) {
        T_DG(FRR_TRACE_GRP_OSPF, "OSPF process ID %d does not exist", FRR_OSPF_DEFAULT_INSTANCE_ID);
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* If it's a VLAN interface, check that it exists */
    if (vtss_ifindex_is_vlan(ifindex) && !vtss_appl_ip_if_exists(ifindex)) {
        T_DG(FRR_TRACE_GRP_OSPF, "Interface %s does not exist", ifindex);
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_interface_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface status. "
                          "(ifindex = "
                          << ifindex << ", rc = " << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Start the searching process */
    auto entry = result_list->find(ifindex);
    if (entry != result_list->end()) {
        // Found it
        auto &map_status = entry->second;

        /* Notice that the result of frr_ip_ospf_interface_status_get() is
         * based on FRR command 'show ip ospf interface', this command output
         * only includes those interfaces which OSPF is enabled.
         * When the interface status is link-down, only parameter'if_up' is
         * insignificant, the others information need to be obtained by other
         * way.
         */
        if (!map_status.if_up) {
            return OSPF_interface_link_down_status_get(ifindex, status);
        }

        // Below parameters are significant only when the interface is UP
        status->status = map_status.if_up;
        status->network = map_status.net;
        status->area_id = map_status.area.area;
        status->router_id = map_status.router_id;
        status->cost = map_status.cost;
        switch (map_status.state) {
        case ISM_Loopback:
            status->state = VTSS_APPL_OSPF_INTERFACE_LOOPBACK;
            break;
        case ISM_Waiting:
            status->state = VTSS_APPL_OSPF_INTERFACE_WAITING;
            break;
        case ISM_PointToPoint:
            status->state = VTSS_APPL_OSPF_INTERFACE_POINT2POINT;
            break;
        case ISM_DROther:
            status->state = VTSS_APPL_OSPF_INTERFACE_DR_OTHER;
            break;
        case ISM_Backup:
            status->state = VTSS_APPL_OSPF_INTERFACE_BDR;
            break;
        case ISM_DR:
            status->state = VTSS_APPL_OSPF_INTERFACE_DR;
            break;
        case ISM_DependUpon:  // Fall through
        case ISM_Down:        // Fall through
        default:
            status->state = VTSS_APPL_OSPF_INTERFACE_DOWN;
            break;
        };
        status->priority = map_status.priority;

        if (map_status.state == ISM_DR) {
            // if I'm DR, fillin myself as DR
            status->dr_addr = map_status.net.address;
            status->dr_id = map_status.router_id;
        } else {
            // otherwise search DR in the neighbor table
            OSPF_nbr_lookup_dr(ifindex, &status->dr_addr, &status->dr_id);
        }

        status->bdr_id = map_status.bdr_id;
        status->bdr_addr = map_status.bdr_address;

        status->vlink_peer_addr = map_status.vlink_peer_addr;
        status->hello_time =
            map_status.timer.raw32() ? map_status.timer.raw32() : 0;
        status->dead_time = map_status.timer_dead.raw32()
                            ? map_status.timer_dead.raw32()
                            : 0;
        status->retransmit_time = map_status.timer_retransmit.raw32()
                                  ? map_status.timer_retransmit.raw32()
                                  : 0;
        status->hello_due_time = map_status.timer_hello.raw32() /* microsecond */;
        status->hello_due_time /= 1000;  // convert micro-second to second
        status->neighbor_count = map_status.nbr_count;
        status->adj_neighbor_count = map_status.nbr_adjacent_count;
        status->transmit_delay = map_status.transmit_delay.raw32();
        status->is_passive = map_status.timer_passive_iface;

        return VTSS_RC_OK;
    }

    /* Unlike the OSPF VLAN interface, the down state of VLINK interfaces
     * always can be found in the result of "show ip ospf interface".
     * So, return here when it is not found in the result.
     */
    if (vtss_ifindex_is_frr_vlink(ifindex)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Not found in the result of "show ip ospf interface".
     * Notice that the result of frr_ip_ospf_interface_status_get() is
     * based on FRR command 'show ip ospf interface', this command output
     * only includes those interfaces which OSPF is enabled.
     * In this case, we need to call OSPF_interface_link_down_status_get()
     * to check the current interface state again.
     */
    return OSPF_interface_link_down_status_get(ifindex, status);
}

/**
 * \brief Get status for a specific interface.
 * \param interface [OUT] An container with all neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_interface_status_get_all(vtss::Map<vtss_ifindex_t, vtss_appl_ospf_interface_status_t> &interface)
{
    vtss_ifindex_t                    ifindex;
    vtss_appl_ospf_interface_status_t status;

    CRIT_SCOPE();

    /* Lookup the default OSPF instance if existing or not. */
    if (!OSPF_instance_id_existing(FRR_OSPF_DEFAULT_INSTANCE_ID)) {
        VTSS_TRACE(DEBUG) << "OSPF process ID " << FRR_OSPF_DEFAULT_INSTANCE_ID << " does not exist";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_interface_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF, "Access framework failed: Get interface status (ifindex = %s, rc = %s)", ifindex, error_txt(result_list.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    // Notice that the result of frr_ip_ospf_interface_status_get() is based on
    // FRR command 'show ip ospf interface'.
    // The output of this command only includes those interfaces on which OSPF
    // is enabled.
    // In this case, we need to add all IP interfaces in the 'result_list'.
    vtss_appl_ip_if_status_link_t link;
    vtss_appl_ip_if_conf_ipv4_t   conf;
    vtss_appl_ip_if_key_ipv4_t    key = {};
    FrrIpOspfIfStatus             frr_intf_status;

    while (vtss_appl_ip_if_status_ipv4_itr(&key, &key) == VTSS_RC_OK) {
        if (vtss_appl_ip_if_conf_ipv4_get(key.ifindex, &conf) != VTSS_RC_OK) {
            continue;
        }

        if (vtss_appl_ip_if_status_link_get(key.ifindex, &link) != VTSS_RC_OK) {
            continue;
        }

        if (link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) {
            // Process down state interfaces only
            continue;
        }

        if (result_list->find(key.ifindex) == result_list->end() && OSPF_interface_link_down_status_get(key.ifindex, &status) == VTSS_RC_OK) {
            // Not found in the original list. Add the interface with down state
            frr_intf_status.if_up = false;
            result_list->set(key.ifindex, std::move(frr_intf_status));
        }
    }

    for (const auto &itr : result_list.val) {
        auto &map_status = itr.second;
        ifindex = itr.first;

        /* Notice that the result of frr_ip_ospf_interface_status_get() is
         * based on FRR command 'show ip ospf interface', this command output
         * only includes those interfaces which OSPF is enabled.
         * When the interface status is link-down, only parameter'if_up' is
         * insignificant, the others information need to be obtained by other
         * way.
         */
        if (!map_status.if_up) {
            if (OSPF_interface_link_down_status_get(ifindex, &status) != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG) << "Get OSPF interface " << ifindex
                                  << " status failed\n";
                // Don't insert the failed case in the Map object.
                continue;
            }
        } else {
            // Below parameters are significant only when the interface is UP
            status.status = map_status.if_up;
            status.network = map_status.net;
            status.area_id = map_status.area.area;
            status.router_id = map_status.router_id;
            status.cost = map_status.cost;
            switch (map_status.state) {
            case ISM_Loopback:
                status.state = VTSS_APPL_OSPF_INTERFACE_LOOPBACK;
                break;
            case ISM_Waiting:
                status.state = VTSS_APPL_OSPF_INTERFACE_WAITING;
                break;
            case ISM_PointToPoint:
                status.state = VTSS_APPL_OSPF_INTERFACE_POINT2POINT;
                break;
            case ISM_DROther:
                status.state = VTSS_APPL_OSPF_INTERFACE_DR_OTHER;
                break;
            case ISM_Backup:
                status.state = VTSS_APPL_OSPF_INTERFACE_BDR;
                break;
            case ISM_DR:
                status.state = VTSS_APPL_OSPF_INTERFACE_DR;
                break;
            case ISM_DependUpon:  // Fall through
            case ISM_Down:        // Fall through
            default:
                status.state = VTSS_APPL_OSPF_INTERFACE_DOWN;
                break;
            };
            status.priority = map_status.priority;

            if (map_status.state == ISM_DR) {
                // if I'm DR, fillin myself as DR
                status.dr_addr = map_status.net.address;
                status.dr_id = map_status.router_id;
            } else {
                // otherwise search DR in the neighbor table
                OSPF_nbr_lookup_dr(ifindex, &status.dr_addr, &status.dr_id);
            }

            status.bdr_id = map_status.bdr_id;
            status.bdr_addr = map_status.bdr_address;

            status.vlink_peer_addr = map_status.vlink_peer_addr;
            status.hello_time =
                map_status.timer.raw32() ? map_status.timer.raw32() : 0;
            status.dead_time = map_status.timer_dead.raw32()
                               ? map_status.timer_dead.raw32()
                               : 0;
            status.retransmit_time = map_status.timer_retransmit.raw32()
                                     ? map_status.timer_retransmit.raw32()
                                     : 0;
            status.hello_due_time =
                map_status.timer_hello.raw32() /* microsecond */;
            status.hello_due_time /= 1000;  // convert micro-second to second
            status.neighbor_count = map_status.nbr_count;
            status.adj_neighbor_count = map_status.nbr_adjacent_count;
            status.transmit_delay = map_status.transmit_delay.raw32();
            status.is_passive = map_status.timer_passive_iface;
        }
        /* Insert the entry */
        interface.insert(
            vtss::Pair<vtss_ifindex_t, vtss_appl_ospf_interface_status_t>(
                ifindex, status));
    }

    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
// ** OSPF neighbor status
//------------------------------------------------------------------------------
static mesa_rc OSPF_neighbor_status_itr_k2(const mesa_ipv4_t *const current_nip,
                                           mesa_ipv4_t *const next_nip,
                                           vtss_appl_ospf_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nip_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        const auto &neighbor = i.second;
        for (const auto &nst : neighbor) {
            if (current_nip && nst.if_address <= *current_nip) {
                continue;
            }

            if (next_nip_valid && nst.if_address >= *next_nip) {
                continue;
            }

            *next_nip = nst.if_address;
            next_nip_valid = true;
        }
    }

    if (next_nip_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

static mesa_rc OSPF_neighbor_status_itr_k3(const vtss_ifindex_t *const current_ifidx,
                                           vtss_ifindex_t *const next_ifidx,
                                           vtss_appl_ospf_id_t key1,
                                           mesa_ipv4_t key2)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_ifidx_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        auto &neighbor = i.second;

        for (const auto &nst : neighbor) {
            if (nst.if_address != key2) {
                continue;
            }

            if (current_ifidx && nst.ifindex <= *current_ifidx) {
                continue;
            }

            if (next_ifidx_valid && nst.ifindex >= *next_ifidx) {
                continue;
            }

            *next_ifidx = nst.ifindex;
            next_ifidx_valid = true;
        }
    }

    if (next_ifidx_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by standard MIB
 * \param current_id    [IN]  Current OSPF ID.
 * \param next_id       [OUT] Next OSPF ID.
 * \param current_nip   [IN] Pointer to current neighbor IP.
 * \param next_nip      [OUT] Next neighbor IP.
 * \param current_ifidx [IN] Pointer to current neighbor ifindex.
 * \param next_ifidx    [OUT] Next neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_neighbor_status_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id, const mesa_ipv4_t *const current_nip,
    mesa_ipv4_t *const next_nip, const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_nip || !next_ifidx) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_nip' or "
                          "'next_ifidx' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto &res = OSPF_frr_ospf_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res->empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, mesa_ipv4_t, vtss_ifindex_t> itr(
        OSPF_inst_itr, OSPF_neighbor_status_itr_k2,
        OSPF_neighbor_status_itr_k3);

    auto rc = itr(current_id, next_id, current_nip, next_nip, current_ifidx,
                  next_ifidx);

    OSPF_frr_ospf_nbr_status_cache.invalidate();
    return rc;
}

static mesa_rc OSPF_neighbor_status_itr2_k2(
    const vtss_appl_ospf_router_id_t *const current_nid,
    vtss_appl_ospf_router_id_t *const next_nid, vtss_appl_ospf_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nid_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        const auto &neighbor_id = i.first;
        if (current_nid && neighbor_id <= *current_nid) {
            continue;
        }

        *next_nid = neighbor_id;
        next_nid_valid = true;
        break;
    }

    if (next_nid_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

static mesa_rc OSPF_neighbor_status_itr2_k3(const mesa_ipv4_t *const current_nip,
                                            mesa_ipv4_t *const next_nip,
                                            vtss_appl_ospf_id_t key1,
                                            vtss_appl_ospf_router_id_t key2)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nip_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        const auto &neighbor = i.second;
        if (i.first != key2) {
            continue;
        }

        for (const auto &nst : neighbor) {
            if (current_nip && nst.if_address <= *current_nip) {
                continue;
            }

            if (next_nip_valid && nst.if_address >= *next_nip) {
                continue;
            }

            *next_nip = nst.if_address;
            next_nip_valid = true;
        }

        break;
    }

    if (next_nip_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

static mesa_rc OSPF_neighbor_status_itr2_k4(
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx, vtss_appl_ospf_id_t key1,
    vtss_appl_ospf_router_id_t key2, mesa_ipv4_t key3)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_ifidx_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        const auto &neighbor = i.second;
        if (i.first != key2) {
            continue;
        }

        for (const auto &nst : neighbor) {
            if (nst.if_address != key3) {
                continue;
            }

            if (current_ifidx && nst.ifindex <= *current_ifidx) {
                continue;
            }

            if (next_ifidx_valid && nst.ifindex >= *next_ifidx) {
                continue;
            }

            *next_ifidx = nst.ifindex;
            next_ifidx_valid = true;
        }

        break;
    }

    if (next_ifidx_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by CLI and JSON/Private MIB
 * \param current_id    [IN]  Current OSPF ID.
 * \param next_id       [OUT] Next OSPF ID.
 * \param current_nid   [IN]  Pointer to current neighbor id.
 * \param next_nid      [OUT] Next entry neighbor id.
 * \param current_nip   [IN]  Pointer to current neighbor IP.
 * \param next_nip      [OUT] Next entry neighbor IP.
 * \param current_ifdix [IN]  Pointer to current neighbor ifindex.
 * \param next_ifidx    [OUT] Next entry neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_neighbor_status_itr2(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_router_id_t *const current_nid,
    vtss_appl_ospf_router_id_t *const next_nid,
    const mesa_ipv4_t *const current_nip, mesa_ipv4_t *const next_nip,
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_nid || !next_nip || !next_ifidx) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_nid' or "
                          "'next_nip' or 'next_ifidx' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto &res = OSPF_frr_ospf_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res->empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_router_id_t,
         mesa_ipv4_t, vtss_ifindex_t>
         itr(OSPF_inst_itr, OSPF_neighbor_status_itr2_k2,
             OSPF_neighbor_status_itr2_k3, OSPF_neighbor_status_itr2_k4);

    auto rc = itr(current_id, next_id, current_nid, next_nid, current_nip,
                  next_nip, current_ifidx, next_ifidx);

    OSPF_frr_ospf_nbr_status_cache.invalidate();
    return rc;
}

static mesa_rc OSPF_neighbor_status_itr3_k2(
    const vtss_appl_ospf_area_id_t *const current_transit_area_id,
    vtss_appl_ospf_area_id_t *const next_transit_area_id,
    vtss_appl_ospf_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_transit_area_id_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        const auto &neighbor = i.second;

        for (const auto &nst : neighbor) {
            if (!vtss_ifindex_is_frr_vlink(nst.ifindex)) {
                continue;
            }

            if (current_transit_area_id &&
                nst.transit_id.area <= *current_transit_area_id) {
                continue;
            }

            if (next_transit_area_id_valid &&
                nst.transit_id.area >= *next_transit_area_id) {
                continue;
            }

            *next_transit_area_id = nst.transit_id.area;
            next_transit_area_id_valid = true;
        }
    }

    if (next_transit_area_id_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

static mesa_rc OSPF_neighbor_status_itr3_k3(
    const vtss_appl_ospf_router_id_t *const current_nid,
    vtss_appl_ospf_router_id_t *const next_nid, vtss_appl_ospf_id_t key1,
    vtss_appl_ospf_area_id_t key2)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nid_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        const auto &neighbor_id = i.first;
        const auto &neighbor = i.second;

        for (const auto &nst : neighbor) {
            if (!vtss_ifindex_is_frr_vlink(nst.ifindex)) {
                continue;
            }

            if (nst.transit_id.area != key2) {
                continue;
            }

            if (current_nid && neighbor_id <= *current_nid) {
                continue;
            }

            if (next_nid_valid && neighbor_id >= *next_nid) {
                continue;
            }

            *next_nid = neighbor_id;
            next_nid_valid = true;
        }
    }

    if (next_nid_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

static mesa_rc OSPF_neighbor_status_itr3_k4(const mesa_ipv4_t *const current_nip,
                                            mesa_ipv4_t *const next_nip,
                                            vtss_appl_ospf_id_t key1,
                                            vtss_appl_ospf_area_id_t key2,
                                            vtss_appl_ospf_router_id_t key3)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nip_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        const auto &neighbor = i.second;
        if (i.first != key3) {
            continue;
        }

        for (const auto &nst : neighbor) {
            if (!vtss_ifindex_is_frr_vlink(nst.ifindex)) {
                continue;
            }

            if (nst.transit_id.area != key2) {
                continue;
            }

            if (current_nip && nst.if_address <= *current_nip) {
                continue;
            }

            if (next_nip_valid && nst.if_address >= *next_nip) {
                continue;
            }

            *next_nip = nst.if_address;
            next_nip_valid = true;
        }

        break;
    }

    if (next_nip_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

static mesa_rc OSPF_neighbor_status_itr3_k5(
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx, vtss_appl_ospf_id_t key1,
    vtss_appl_ospf_area_id_t key2, vtss_appl_ospf_router_id_t key3,
    mesa_ipv4_t key4)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_ifidx_valid = false;

    for (const auto &i : OSPF_frr_ospf_nbr_status_cache()) {
        const auto &neighbor = i.second;
        if (i.first != key3) {
            continue;
        }

        for (const auto &nst : neighbor) {
            if (!vtss_ifindex_is_frr_vlink(nst.ifindex)) {
                continue;
            }

            if (nst.transit_id.area != key2 || nst.if_address != key4) {
                continue;
            }

            if (current_ifidx && nst.ifindex <= *current_ifidx) {
                continue;
            }

            if (next_ifidx_valid && nst.ifindex >= *next_ifidx) {
                continue;
            }

            *next_ifidx = nst.ifindex;
            next_ifidx_valid = true;
        }

        break;
    }

    if (next_ifidx_valid) {
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Iterate through the neighbor entry.
 *  This function is used by Standard MIB - ospfVirtNbrTable (key: Transit Area
 * and Router ID)
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
    vtss_ifindex_t *const next_ifidx)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_transit_area_id || !next_nid) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_transit_area_id' or "
                          "'next_nid' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto &res = OSPF_frr_ospf_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res.val.empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_area_id_t,
         vtss_appl_ospf_router_id_t, mesa_ipv4_t, vtss_ifindex_t>
         itr(OSPF_inst_itr, OSPF_neighbor_status_itr3_k2,
             OSPF_neighbor_status_itr3_k3, OSPF_neighbor_status_itr3_k4,
             OSPF_neighbor_status_itr3_k5);

    auto rc = itr(current_id, next_id, current_transit_area_id,
                  next_transit_area_id, current_nid, next_nid, current_nip,
                  next_nip, current_ifidx, next_ifidx);

    OSPF_frr_ospf_nbr_status_cache.invalidate();
    return rc;
}

/**
 * \brief Get status for a neighbor information.
 * \param id            [IN]  OSPF instance ID.
 * \param neighbor_id   [IN]  Neighbor id to query.
 * \param neighbor_ip   [IN]  Neighbor IP to query.
 * \param neighbor_ifidx[IN]  Neighbor ifindex to query.
 * \param status        [OUT] Neighbor status.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_neighbor_status_get(
    const vtss_appl_ospf_id_t id,
    const vtss_appl_ospf_router_id_t neighbor_id,
    const mesa_ipv4_t neighbor_ip, const vtss_ifindex_t neighbor_ifidx,
    vtss_appl_ospf_neighbor_status_t *const status)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!status) {
        VTSS_TRACE(ERROR) << "Parameter 'status' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto &res = OSPF_frr_ospf_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(neighbor_id = "
                          << neighbor_id << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    for (const auto &itr : res.val) {
        auto &itr_ips = itr.second;
        if (neighbor_id != VTSS_APPL_OSPF_DONTCARE_NID &&
            itr.first != neighbor_id) {
            continue;
        }

        for (const auto &itr_status : itr_ips) {
            VTSS_TRACE(DEBUG) << "if address " << itr_status.if_address;
            if (itr_status.if_address != neighbor_ip) {
                continue;
            }

            if (itr_status.ifindex != neighbor_ifidx) {
                continue;
            }

            status->ip_addr = itr_status.if_address;
            status->neighbor_id = itr.first;
            status->area_id = itr_status.area.area;
            status->ifindex = itr_status.ifindex;
            status->priority = itr_status.nbr_priority;
            status->state = (vtss_appl_ospf_neighbor_state_t)itr_status.nbr_state;

            status->dr_addr = itr_status.dr_ip_addr;
            status->bdr_addr = itr_status.bdr_ip_addr;

            if (vtss_ifindex_is_frr_vlink(status->ifindex)) {
                status->dr_id = status->bdr_id = 0;
            } else {
                // Quick calculate DR ID in local entry
                if (status->dr_addr == status->ip_addr) {
                    status->dr_id = neighbor_id;
                } else {
                    status->dr_id =
                        OSPF_nbr_lookup_id_by_addr(itr_status.dr_ip_addr);
                }

                if (status->bdr_addr != status->dr_addr) {
                    // Quick calculate BDR ID in local entry
                    if (status->bdr_addr == status->ip_addr) {
                        status->bdr_id = neighbor_id;
                    } else {
                        status->bdr_id = OSPF_nbr_lookup_id_by_addr(
                                             itr_status.bdr_ip_addr);
                    }
                } else {
                    status->bdr_id = status->dr_id;
                }
            }

            status->dead_time = itr_status.router_dead_interval_timer_due.raw32();

            status->options = itr_status.options_counter;
            status->transit_id = itr_status.transit_id.area;
            OSPF_frr_ospf_nbr_status_cache.invalidate();
            return VTSS_RC_OK;
        }
    }

    OSPF_frr_ospf_nbr_status_cache.invalidate();
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get status for all neighbor information.
 * \param ipv4_routes [OUT] An container with all neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_neighbor_status_get_all(
    vtss::Vector<vtss_appl_ospf_neighbor_data_t> &neighbors)
{
    CRIT_SCOPE();

    vtss_appl_ospf_id_t *current_id = NULL;
    vtss_appl_ospf_id_t next_id;
    vtss_appl_ospf_router_id_t *current_nid = NULL;
    vtss_appl_ospf_router_id_t next_nid;
    mesa_ipv4_t *current_nip = NULL;
    mesa_ipv4_t next_nip;
    vtss_ifindex_t *current_ifidx = NULL;
    vtss_ifindex_t next_ifidx;
    vtss_appl_ospf_neighbor_data_t data = {};
    BOOL get_first = TRUE;

    /* Get data from FRR layer */
    auto &res = OSPF_frr_ospf_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res->empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf_id_t, vtss_appl_ospf_router_id_t,
         mesa_ipv4_t, vtss_ifindex_t>
         itr(OSPF_inst_itr, OSPF_neighbor_status_itr2_k2,
             OSPF_neighbor_status_itr2_k3, OSPF_neighbor_status_itr2_k4);

    while (itr(current_id, &next_id, current_nid, &next_nid, current_nip,
               &next_nip, current_ifidx, &next_ifidx) == VTSS_RC_OK) {
        data.id = next_id;
        data.neighbor_id = next_nid;
        data.neighbor_ip = next_nip;
        data.neighbor_ifidx = next_ifidx;

        for (const auto &itr : res.val) {
            auto &itr_ips = itr.second;
            if (next_nid != VTSS_APPL_OSPF_DONTCARE_NID && itr.first < next_nid) {
                continue;
            }

            // Found
            for (const auto &itr_status : itr_ips) {
                VTSS_TRACE(DEBUG) << "if address " << itr_status.if_address;
                if (itr_status.if_address != next_nip) {
                    continue;
                }

                if (itr_status.ifindex != next_ifidx) {
                    continue;
                }

                auto status = &data.status;
                status->ip_addr = itr_status.if_address;
                status->neighbor_id = itr.first;
                status->area_id = itr_status.area.area;
                status->ifindex = itr_status.ifindex;
                status->priority = itr_status.nbr_priority;
                status->state =
                    (vtss_appl_ospf_neighbor_state_t)itr_status.nbr_state;

                status->dr_addr = itr_status.dr_ip_addr;
                status->bdr_addr = itr_status.bdr_ip_addr;

                if (vtss_ifindex_is_frr_vlink(status->ifindex)) {
                    status->dr_id = status->bdr_id = 0;
                } else {
                    // Quick calculate DR ID in local entry
                    if (status->dr_addr == status->ip_addr) {
                        status->dr_id = itr.first;
                    } else {
                        status->dr_id =
                            OSPF_nbr_lookup_id_by_addr(itr_status.dr_ip_addr);
                    }

                    if (status->bdr_addr != status->dr_addr) {
                        // Quick calculate BDR ID in local entry
                        if (status->bdr_addr == status->ip_addr) {
                            status->bdr_id = itr.first;
                        } else {
                            status->bdr_id = OSPF_nbr_lookup_id_by_addr(
                                                 itr_status.bdr_ip_addr);
                        }
                    } else {
                        status->bdr_id = status->dr_id;
                    }
                }

                status->dead_time =
                    itr_status.router_dead_interval_timer_due.raw32();

                status->options = itr_status.options_counter;
                status->transit_id = itr_status.transit_id.area;
            }
        }
        // 'itr(current_id, &next_id,...) == VTSS_RC_OK' makes sure the entry is
        // existent so the data is got completely.
        neighbors.push_back(data);

        if (get_first) {
            current_id = &next_id;
            current_nid = &next_nid;
            current_nip = &next_nip;
            current_ifidx = &next_ifidx;
            get_first = FALSE;
        }
    }

    // Caches disabled
    OSPF_frr_ospf_nbr_status_cache.invalidate();
    OSPF_frr_ospf_intf_status_cache.invalidate();

    return VTSS_RC_OK;
}

/* This function is the system reset callback to disable OSPF router, and
 * this may trigger stub router advertisement. If yes, this callback will wait
 * for the OSPF router shutdown complete. The longest waiting time
 * may reach 100 seconds.
 */
void frr_ospf_pre_shutdown_callback(mesa_restart_t restart)
{
    vtss_appl_ospf_id_t id = VTSS_APPL_OSPF_INSTANCE_ID_START;
    uint32_t deferred_shutdown_time = 0;

    /* Disable OSPF router, this may start stub router */
    mesa_rc rc = vtss_appl_ospf_del(id);
    if (rc != VTSS_RC_OK) {
        return;
    }
    /* OSPF process maybe still running due to stub router advertisement.
     * Get deferred shutdown time and wait for the OSPF process shutdown
     * complete.
     */
    {
        CRIT_SCOPE();
        auto router_status = frr_ip_ospf_status_get();
        if (router_status.rc != VTSS_RC_OK) {
            return;
        }

        deferred_shutdown_time = router_status->deferred_shutdown_time.raw32();
    }

    if (deferred_shutdown_time > 0) {
        /* Print message on CLI and do syslog */
        (void)icli_session_printf_to_all(
            "\nDeclare OSPF router as a stub router.\nDeferred shutdown "
            "OSPF in progress, %d(ms) remaining.",
            deferred_shutdown_time);
#ifdef VTSS_SW_OPTION_SYSLOG
        S_I("Declare OSPF router as a stub router. Deferred shutdown OSPF in "
            "progress, %d(ms) remaining.",
            deferred_shutdown_time);
#endif /* VTSS_SW_OPTION_SYSLOG */

        /* Delay to wait for the shutdown timer expired */
        VTSS_OS_MSLEEP(deferred_shutdown_time);

        /* Print message on CLI and do syslog when OSPF router is disabled */
        (void)icli_session_printf_to_all("\nOSPF router is disabled.");
#ifdef VTSS_SW_OPTION_SYSLOG
        S_I("OSPF router is disabled.");
#endif /* VTSS_SW_OPTION_SYSLOG */
    }
}

//----------------------------------------------------------------------------
//** OSPF routing information
//----------------------------------------------------------------------------
/* convert FRR OSPF route entry to vtss_appl_ospf_route_status_t */
static void OSPF_frr_ospf_route_ipv4_status_mapping(
    const FrrOspfRouteType rt_type,
    const APPL_FrrOspfRouteStatus *const frr_val,
    vtss_appl_ospf_route_status_t *const status)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping FrrOspfRouterType and 'is_ia' to
     *  vtss_appl_ospf_route_br_type_t */
    switch (frr_val->router_type) {
    case RouterType_ABR:
        status->border_router_type = VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_ABR;
        break;
    case RouterType_ASBR:
        status->border_router_type =
            !frr_val->is_ia
            ? VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_INTRA_AREA_ASBR
            : VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_INTER_AREA_ASBR;
        break;
    case RouterType_ABR_ASBR:
        status->border_router_type =
            VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_ABR_ASBR;
        break;
    case RouterType_None:
        status->border_router_type = VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_NONE;

        /* ignore default case to catch compile warning if any router type is
         * missing */
    }

    if (rt_type == RT_ExtNetworkTypeTwo) {
        /* For external Type-2 routes, 'status->cost' is equal to
         * 'frr_val->ext_cost' */
        status->cost = frr_val->ext_cost;
        status->as_cost = frr_val->cost;
    } else {
        status->cost = frr_val->cost;
        status->as_cost = VTSS_APPL_OSPF_GENERAL_COST_MIN;
    }

    status->connected = frr_val->is_connected;
    status->ifindex = frr_val->ifindex;
}

/* Convert the FRR access's key to the APPL's key. */
static void OSPF_frr_ospf_route_ipv4_key_mapping(
    const APPL_FrrOspfRouteKey *const frr_key, vtss_appl_ospf_id_t *const id,
    vtss_appl_ospf_route_type_t *const rt_type,
    mesa_ipv4_network_t *const dest, vtss_appl_ospf_area_id_t *const area,
    mesa_ipv4_t *const nexthop)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert FRR key (" << frr_key->inst_id << ", "
                      << frr_key->route_type << ", " << frr_key->network << ", "
                      << frr_key->area << ", " << frr_key->nexthop_ip << ") to";

    *id = frr_key->inst_id;
    *rt_type = frr_ospf_route_type_mapping(frr_key->route_type);
    *dest = frr_key->network;
    *area = frr_key->area;
    *nexthop = frr_key->nexthop_ip;
    VTSS_TRACE(DEBUG) << "APPL key(" << *id << ", " << *rt_type << ", " << *dest
                      << ", " << *area << ", " << *nexthop << ")";
}

/* Convert the APPL's key to the FRR access's key. */
static APPL_FrrOspfRouteKey OSPF_frr_ospf_route_ipv4_access_key_mapping(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_route_type_t rt_type,
    const mesa_ipv4_network_t dest, vtss_appl_ospf_area_id_t area,
    const mesa_ipv4_t nexthop)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert APPL key(" << id << ", " << rt_type << ", "
                      << dest << ", " << area << ", " << nexthop << ") to";

    APPL_FrrOspfRouteKey frr_key;
    frr_key.inst_id = id;
    frr_key.route_type = frr_ospf_access_route_type_mapping(rt_type);
    frr_key.network = dest;
    frr_key.area = area;
    frr_key.nexthop_ip = nexthop;

    VTSS_TRACE(DEBUG) << "FRR key (" << frr_key.inst_id << ", "
                      << frr_key.route_type << ", " << frr_key.network << ", "
                      << frr_key.area << ", " << frr_key.nexthop_ip << ")";
    return frr_key;
}

/**
 * \brief Iterate through the OSPF IPv4 route entries.
 * \param current_id        [IN]  The current OSPF ID.
 * \param next_id           [OUT] The next OSPF ID.
 * \param current_rt_type   [IN]  The current route type.
 * \param next_rt_type      [OUT] The next route type.
 * \param current_dest      [IN]  The current destination.
 * \param next_dest         [OUT] The next destination.
 * \param current_nexthop   [IN]  The current nexthop.
 * \param next_nexthop      [OUT] The next nexthop.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_route_ipv4_status_itr(
    const vtss_appl_ospf_id_t *const current_id,
    vtss_appl_ospf_id_t *const next_id,
    const vtss_appl_ospf_route_type_t *const current_rt_type,
    vtss_appl_ospf_route_type_t *const next_rt_type,
    const mesa_ipv4_network_t *const current_dest,
    mesa_ipv4_network_t *const next_dest,
    const vtss_appl_ospf_area_id_t *const current_area,
    vtss_appl_ospf_area_id_t *const next_area,
    const mesa_ipv4_t *const current_nexthop,
    mesa_ipv4_t *const next_nexthop)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_rt_type || !next_dest || !next_area || !next_nexthop) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_rt_type' or "
                          "'next_dest' or 'next_nexthop' cannot be null "
                          "point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_route_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF routes . "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Invoke Map.greater_than() or Map.greater_than_or_equal() get the next
     * entry.
     * If all keys are assigned from caller, then invoking greater_than()
     * If the n-th key is NULL, then all the keys after n-th key (including
     * itself) are assigned the first possible value.
     */
    Map<APPL_FrrOspfRouteKey, APPL_FrrOspfRouteStatus>::iterator itr;
    APPL_FrrOspfRouteKey frr_key({0, RT_Network, {0, 0}, 0});
    bool is_greater_or_equal = false;
    if (!current_id || !current_rt_type || !current_dest || !current_area ||
        !current_nexthop) {
        is_greater_or_equal = true;
    }

    if (current_id) {
        if (!current_rt_type) {
            frr_key.inst_id = *current_id;
        } else if (*current_rt_type >= VTSS_APPL_OSPF_ROUTE_TYPE_UNKNOWN) {
            // 'current_rt_type' is VTSS_APPL_OSPF_ROUTE_TYPE_UNKNOWN means to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*current_id == VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *current_id + 1;
            is_greater_or_equal = true;
        } else if (current_rt_type && !current_dest) {
            frr_key.inst_id = *current_id;
            frr_key.route_type =
                frr_ospf_access_route_type_mapping(*current_rt_type);
        } else if (current_rt_type && current_dest && !current_area) {
            frr_key.inst_id = *current_id;
            frr_key.route_type =
                frr_ospf_access_route_type_mapping(*current_rt_type);
            frr_key.network = *current_dest;
        } else if (current_rt_type && current_dest && current_area &&
                   !current_nexthop) {
            frr_key.inst_id = *current_id;
            frr_key.route_type =
                frr_ospf_access_route_type_mapping(*current_rt_type);
            frr_key.network = *current_dest;
            frr_key.area = *current_area;
        } else {
            frr_key.inst_id = *current_id;
            frr_key.route_type =
                frr_ospf_access_route_type_mapping(*current_rt_type);
            frr_key.network = *current_dest;
            frr_key.area = *current_area;
            frr_key.nexthop_ip = *current_nexthop;
        }
    }

    if (is_greater_or_equal) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal() from ";
        itr = result_list.val.greater_than_or_equal(frr_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() from ";
        itr = result_list.val.greater_than(frr_key);
    }

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << ", " << frr_key.route_type
                      << ", " << frr_key.network << ", "
                      << AsIpv4(frr_key.nexthop_ip) << ")";
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF_frr_ospf_route_ipv4_key_mapping(&itr->first, next_id, next_rt_type,
                                         next_dest, next_area, next_nexthop);

    return VTSS_RC_OK;
}

/**
 * \brief Get the specific OSPF IPv4 route entry.
 * \param id      [IN]  The OSPF instance ID.
 * \param rt_type [IN]  The route type.
 * \param dest    [IN]  The destination.
 * \param nexthop [IN]  The nexthop.
 * \param status  [OUT] The OSPF route status.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_route_ipv4_status_get(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_route_type_t rt_type,
    const mesa_ipv4_network_t dest, const vtss_appl_ospf_area_id_t area,
    const mesa_ipv4_t nexthop, vtss_appl_ospf_route_status_t *const status)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!status) {
        VTSS_TRACE(ERROR) << "Parameter 'status' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_route_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF routes . "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_route_ipv4_access_key_mapping(
                       id, rt_type, dest, area, nexthop);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_route_ipv4_status_mapping(itr->first.route_type, &itr->second,
                                            status);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF IPv4 route entries.
 * \param routes [OUT] The container with all routes.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_route_ipv4_status_get_all(
    vtss::Vector<vtss_appl_ospf_route_ipv4_data_t> &routes)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_route_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF routes . "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf_route_ipv4_data_t entry;

        /* convert the FRR data to vtss_appl_ospf_route_ipv4_data_t. */
        OSPF_frr_ospf_route_ipv4_key_mapping(&itr.first, &entry.id,
                                             &entry.rt_type, &entry.dest,
                                             &entry.area, &entry.nexthop);
        OSPF_frr_ospf_route_ipv4_status_mapping(itr.first.route_type,
                                                &itr.second, &entry.status);

        /* save the in 'routes' */
        routes.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF database information
//----------------------------------------------------------------------------
/* convert FRR OSPF db entry to vtss_appl_ospf_db_general_info_t */
static void OSPF_frr_ospf_db_info_mapping(
    const APPL_FrrOspfDbLinkStateVal *const frr_val,
    vtss_appl_ospf_db_general_info_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspfDbLinkStateVal to
     *  vtss_appl_ospf_db_general_info_t */
    info->age = frr_val->age;
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->router_link_count = frr_val->router_link_count;
}

/* Convert the FRR access's key to the APPL's key. */
static void OSPF_frr_ospf_db_key_mapping(const APPL_FrrOspfDbKey *const frr_key,
                                         vtss_appl_ospf_id_t *const id,
                                         mesa_ipv4_t *const area_id,
                                         vtss_appl_ospf_lsdb_type_t *const type,
                                         mesa_ipv4_t *const link_id,
                                         mesa_ipv4_t *const adv_router)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert FRR key (" << frr_key->inst_id << ", "
                      << frr_key->area_id << ", " << frr_key->type << ", "
                      << frr_key->link_id << ", " << frr_key->adv_router
                      << ") to";

    *id = frr_key->inst_id;
    *area_id = frr_key->area_id;
    *type = frr_ospf_db_type_mapping(frr_key->type);
    *link_id = frr_key->link_id;
    *adv_router = frr_key->adv_router;

    VTSS_TRACE(DEBUG) << "APPL key(" << *id << ", " << *area_id << ", " << *type
                      << ", " << *link_id << ", " << *adv_router << ")";
}

/* Convert the APPL's key to the FRR access's key. */
static APPL_FrrOspfDbKey OSPF_frr_ospf_db_access_key_mapping(
    const vtss_appl_ospf_id_t id, const mesa_ipv4_t area_id,
    const vtss_appl_ospf_lsdb_type_t type, const mesa_ipv4_t link_id,
    const mesa_ipv4_t adv_router)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert APPL key(" << id << ", " << area_id << ", "
                      << type << ", " << link_id << ", " << adv_router
                      << ") to";

    APPL_FrrOspfDbKey frr_key;

    frr_key.inst_id = id;
    frr_key.area_id = area_id;
    frr_key.type = frr_ospf_access_db_type_mapping(type);
    frr_key.link_id = link_id;
    frr_key.adv_router = adv_router;

    VTSS_TRACE(DEBUG) << "FRR key (" << frr_key.inst_id << ", "
                      << frr_key.area_id << ", " << frr_key.type << ", "
                      << frr_key.link_id << ", " << frr_key.adv_router << ")";

    return frr_key;
}

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
mesa_rc vtss_appl_ospf_db_itr(const vtss_appl_ospf_id_t *const cur_inst_id,
                              vtss_appl_ospf_id_t *const next_inst_id,
                              const vtss_appl_ospf_area_id_t *const cur_area_id,
                              vtss_appl_ospf_area_id_t *const next_area_id,
                              const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
                              vtss_appl_ospf_lsdb_type_t *const next_lsdb_type,
                              const mesa_ipv4_t *const cur_link_state_id,
                              mesa_ipv4_t *const next_link_state_id,
                              const vtss_appl_ospf_router_id_t *const cur_router_id,
                              vtss_appl_ospf_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_inst_id || !next_area_id || !next_lsdb_type ||
        !next_link_state_id || !next_router_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_inst_id' or 'next_area_id' or "
                          "'next_lsdb_type' or 'next_link_state_id' or "
                          "'next_router_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Invoke Map.greater_than() or Map.greater_than_or_equal() get the next
     * entry.
     * If all keys are assigned from caller, then invoking greater_than()
     * If the n-th key is NULL, then all the keys after n-th key (including
     * itself) are assigned the first possible value.
     */
    Map<APPL_FrrOspfDbKey, APPL_FrrOspfDbLinkStateVal>::iterator itr;
    APPL_FrrOspfDbKey frr_key({0, 0, 0, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_area_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF_LSDB_TYPE_UNKNOWN) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = *cur_lsdb_type;
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = *cur_lsdb_type;
            frr_key.link_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = *cur_lsdb_type;
            frr_key.link_id = *cur_link_state_id;
            frr_key.adv_router = *cur_router_id;
        }
    }

    if (is_greater_or_equal) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal() from ";
        itr = result_list.val.greater_than_or_equal(frr_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() from ";
        itr = result_list.val.greater_than(frr_key);
    }

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << AsIpv4(frr_key.area_id)
                      << ", " << frr_key.type << ", " << AsIpv4(frr_key.link_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF_frr_ospf_db_key_mapping(&itr->first, next_inst_id, next_area_id,
                                 next_lsdb_type, next_link_state_id,
                                 next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t inst_id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_general_info_t *const db_general_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_general_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_general_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_db_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_db_info_mapping(&itr->second, db_general_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF database entries of general information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_get_all(
    vtss::Vector<vtss_appl_ospf_db_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf_db_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf_db_entry_t. */
        entry.inst_id = FRR_OSPF_DEFAULT_INSTANCE_ID;
        OSPF_frr_ospf_db_key_mapping(&itr.first, &entry.inst_id, &entry.area_id,
                                     &entry.lsdb_type, &entry.link_state_id,
                                     &entry.adv_router_id);

        OSPF_frr_ospf_db_info_mapping(&itr.second, &entry.db);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF database detail router information
//----------------------------------------------------------------------------

uint8_t OSPF_parse_options(const std::string &options)
{
    std::string str_o("O");
    std::string str_dc("DC");
    std::string str_ea("EA");
    std::string str_np("N/P");
    std::string str_mc("MC");
    std::string str_e("E");
    std::string str_mt("M/T");

    uint8_t final_option = 0;

    if (options.find(str_o) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF_OPTION_FIELD_O;
    }

    if (options.find(str_dc) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF_OPTION_FIELD_DC;
    }

    if (options.find(str_ea) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF_OPTION_FIELD_EA;
    }

    if (options.find(str_np) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF_OPTION_FIELD_NP;
    }

    if (options.find(str_mc) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF_OPTION_FIELD_MC;
    }

    if (options.find(str_e) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF_OPTION_FIELD_E;
    }

    if (options.find(str_mt) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF_OPTION_FIELD_MT;
    }

    return final_option;
}

/* Convert the FRR access's key to the APPL's key. */
static void OSPF_frr_ospf_db_common_key_mapping(
    const APPL_FrrOspfDbCommonKey *const frr_key,
    vtss_appl_ospf_id_t *const id, mesa_ipv4_t *const area_id,
    vtss_appl_ospf_lsdb_type_t *const type, mesa_ipv4_t *const link_id,
    mesa_ipv4_t *const adv_router)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert FRR key (" << frr_key->inst_id << ", "
                      << frr_key->area_id << ", " << frr_key->type << ", "
                      << frr_key->link_state_id << ", " << frr_key->adv_router
                      << ") to";

    *id = frr_key->inst_id;
    *area_id = frr_key->area_id;
    *type = frr_ospf_db_type_mapping(frr_key->type);
    *link_id = frr_key->link_state_id;
    *adv_router = frr_key->adv_router;

    VTSS_TRACE(DEBUG) << "APPL key(" << *id << ", " << *area_id << ", " << *type
                      << ", " << *link_id << ", " << *adv_router << ")";
}

/* Convert the APPL's key to the FRR access's key. */
static APPL_FrrOspfDbCommonKey OSPF_frr_ospf_db_common_access_key_mapping(
    const vtss_appl_ospf_id_t id, const mesa_ipv4_t area_id,
    const vtss_appl_ospf_lsdb_type_t type, const mesa_ipv4_t link_id,
    const mesa_ipv4_t adv_router)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert APPL key(" << id << ", " << area_id << ", "
                      << type << ", " << link_id << ", " << adv_router
                      << ") to";

    APPL_FrrOspfDbCommonKey frr_key;

    frr_key.inst_id = id;
    frr_key.area_id = area_id;
    frr_key.type = frr_ospf_access_lsdb_type_mapping(type);
    frr_key.link_state_id = link_id;
    frr_key.adv_router = adv_router;

    VTSS_TRACE(DEBUG) << "FRR key (" << frr_key.inst_id << ", " << frr_key.area_id
                      << ", " << frr_key.type << ", " << frr_key.link_state_id
                      << ", " << frr_key.adv_router << ")";

    return frr_key;
}

/* convert FRR OSPF db entry to vtss_appl_ospf_db_router_data_entry_t */
static void OSPF_frr_ospf_db_router_info_mapping(
    const APPL_FrrOspfDbRouterStateVal *const frr_val,
    vtss_appl_ospf_db_router_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspfDbRouterStateVal to
     *  vtss_appl_ospf_db_router_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->router_link_count = frr_val->links.size();
}

/* convert FRR OSPF db entry into vtss_appl_ospf_db_router_link_info_t */
static void OSPF_frr_ospf_db_router_link_info_mapping(
    const APPL_FrrOspfDbRouterStateVal *const frr_val, const uint32_t index,
    vtss_appl_ospf_db_router_link_info_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (index < frr_val->links.size()) {
        info->link_connected_to = frr_val->links[index].link_connected_to;
        info->link_data = frr_val->links[index].link_data;
        info->link_id = frr_val->links[index].link_id;
        info->metric = frr_val->links[index].metric;
    } else {
        info->link_connected_to = 0;
        info->link_data = 0;
        info->link_id = 0;
        info->metric = 0;
    }
}

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
    const vtss_appl_ospf_id_t *const cur_inst_id,
    vtss_appl_ospf_id_t *const next_inst_id,
    const vtss_appl_ospf_area_id_t *const cur_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_inst_id || !next_area_id || !next_lsdb_type ||
        !next_link_state_id || !next_router_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_inst_id' or 'next_area_id' or "
                          "'next_lsdb_type' or 'next_link_state_id' or "
                          "'next_router_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_router_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF router db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Invoke Map.greater_than() or Map.greater_than_or_equal() get the next
     * entry.
     * If all keys are assigned from caller, then invoking greater_than()
     * If the n-th key is NULL, then all the keys after n-th key (including
     * itself) are assigned the first possible value.
     */
    Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbRouterStateVal>::iterator itr;
    APPL_FrrOspfDbCommonKey frr_key({0, 0, vtss::FrrOspfLsdbType_None, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_area_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF_LSDB_TYPE_NETWORK) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
            frr_key.adv_router = *cur_router_id;
        }
    }

    if (is_greater_or_equal) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal() from ";
        itr = result_list.val.greater_than_or_equal(frr_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() from ";
        itr = result_list.val.greater_than(frr_key);
    }

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << AsIpv4(frr_key.area_id) << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF_frr_ospf_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                        next_lsdb_type, next_link_state_id,
                                        next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t inst_id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_router_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_router_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF router db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_db_router_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_router_id_t adv_router_id, const uint32_t index,
    vtss_appl_ospf_db_router_link_info_t *const link_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!link_info) {
        VTSS_TRACE(ERROR) << "Parameter 'link_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_router_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF router db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_db_router_link_info_mapping(&itr->second, index, link_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF database entries of detail router information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_router_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_router_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_router_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF router db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf_db_detail_router_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf_db_detail_router_entry_t. */
        entry.inst_id = FRR_OSPF_DEFAULT_INSTANCE_ID;
        OSPF_frr_ospf_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF_frr_ospf_db_router_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF database detail network information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf_db_network_data_entry_t */
static void OSPF_frr_ospf_db_network_info_mapping(
    const APPL_FrrOspfDbNetStateVal *const frr_val,
    vtss_appl_ospf_db_network_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspfDbNetStateVal to
     *  vtss_appl_ospf_db_network_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->network_mask = frr_val->network_mask;
    info->attached_router_count = frr_val->attached_router.size();
}

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
    const vtss_appl_ospf_id_t *const cur_inst_id,
    vtss_appl_ospf_id_t *const next_inst_id,
    const vtss_appl_ospf_area_id_t *const cur_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_inst_id || !next_area_id || !next_lsdb_type ||
        !next_link_state_id || !next_router_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_inst_id' or 'next_area_id' or "
                          "'next_lsdb_type' or 'next_link_state_id' or "
                          "'next_router_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_net_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF network db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Invoke Map.greater_than() or Map.greater_than_or_equal() get the next
     * entry.
     * If all keys are assigned from caller, then invoking greater_than()
     * If the n-th key is NULL, then all the keys after n-th key (including
     * itself) are assigned the first possible value.
     */
    Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNetStateVal>::iterator itr;
    APPL_FrrOspfDbCommonKey frr_key({0, 0, vtss::FrrOspfLsdbType_None, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_area_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF_LSDB_TYPE_SUMMARY) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
            frr_key.adv_router = *cur_router_id;
        }
    }

    if (is_greater_or_equal) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal() from ";
        itr = result_list.val.greater_than_or_equal(frr_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() from ";
        itr = result_list.val.greater_than(frr_key);
    }

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << AsIpv4(frr_key.area_id) << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF_frr_ospf_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                        next_lsdb_type, next_link_state_id,
                                        next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t inst_id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_network_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_net_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF network db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_db_network_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF database entries of detail network information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_network_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_network_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_net_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF network db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf_db_detail_network_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf_db_detail_network_entry_t. */
        entry.inst_id = FRR_OSPF_DEFAULT_INSTANCE_ID;
        OSPF_frr_ospf_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF_frr_ospf_db_network_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF database detail summary information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf_db_summary_data_entry_t */
static void OSPF_frr_ospf_db_summary_info_mapping(
    const APPL_FrrOspfDbSummaryStateVal *const frr_val,
    vtss_appl_ospf_db_summary_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspfDbSummaryStateVal to
     *  vtss_appl_ospf_db_summary_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->network_mask = frr_val->network_mask;
    info->metric = frr_val->metric;
}

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
    const vtss_appl_ospf_id_t *const cur_inst_id,
    vtss_appl_ospf_id_t *const next_inst_id,
    const vtss_appl_ospf_area_id_t *const cur_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_inst_id || !next_area_id || !next_lsdb_type ||
        !next_link_state_id || !next_router_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_inst_id' or 'next_area_id' or "
                          "'next_lsdb_type' or 'next_link_state_id' or "
                          "'next_router_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_summary_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF summary db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Invoke Map.greater_than() or Map.greater_than_or_equal() get the next
     * entry.
     * If all keys are assigned from caller, then invoking greater_than()
     * If the n-th key is NULL, then all the keys after n-th key (including
     * itself) are assigned the first possible value.
     */
    Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbSummaryStateVal>::iterator itr;
    APPL_FrrOspfDbCommonKey frr_key({0, 0, vtss::FrrOspfLsdbType_None, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_area_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF_LSDB_TYPE_ASBR_SUMMARY) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
            frr_key.adv_router = *cur_router_id;
        }
    }

    if (is_greater_or_equal) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal() from ";
        itr = result_list.val.greater_than_or_equal(frr_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() from ";
        itr = result_list.val.greater_than(frr_key);
    }

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << AsIpv4(frr_key.area_id) << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF_frr_ospf_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                        next_lsdb_type, next_link_state_id,
                                        next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t inst_id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_summary_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_summary_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF summary db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_db_summary_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF database entries of detail summary information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_summary_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_summary_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_summary_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF summary db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf_db_detail_summary_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf_db_detail_summary_entry_t. */
        entry.inst_id = FRR_OSPF_DEFAULT_INSTANCE_ID;
        OSPF_frr_ospf_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF_frr_ospf_db_summary_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF database detail asbr summary information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf_db_summary_data_entry_t */
static void OSPF_frr_ospf_db_asbr_summary_info_mapping(
    const APPL_FrrOspfDbASBRSummaryStateVal *const frr_val,
    vtss_appl_ospf_db_summary_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspfDbASBRSummaryStateVal to
     *  vtss_appl_ospf_db_summary_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->network_mask = frr_val->network_mask;
    info->metric = frr_val->metric;
}

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
    const vtss_appl_ospf_id_t *const cur_inst_id,
    vtss_appl_ospf_id_t *const next_inst_id,
    const vtss_appl_ospf_area_id_t *const cur_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_inst_id || !next_area_id || !next_lsdb_type ||
        !next_link_state_id || !next_router_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_inst_id' or 'next_area_id' or "
                          "'next_lsdb_type' or 'next_link_state_id' or "
                          "'next_router_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf_db_asbr_summary_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF asbr summary db. "
                "( rc = "
                << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Invoke Map.greater_than() or Map.greater_than_or_equal() get the next
     * entry.
     * If all keys are assigned from caller, then invoking greater_than()
     * If the n-th key is NULL, then all the keys after n-th key (including
     * itself) are assigned the first possible value.
     */
    Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbASBRSummaryStateVal>::iterator itr;
    APPL_FrrOspfDbCommonKey frr_key({0, 0, vtss::FrrOspfLsdbType_None, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_area_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF_LSDB_TYPE_EXTERNAL) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
            frr_key.adv_router = *cur_router_id;
        }
    }

    if (is_greater_or_equal) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal() from ";
        itr = result_list.val.greater_than_or_equal(frr_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() from ";
        itr = result_list.val.greater_than(frr_key);
    }

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << AsIpv4(frr_key.area_id) << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF_frr_ospf_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                        next_lsdb_type, next_link_state_id,
                                        next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t inst_id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_summary_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf_db_asbr_summary_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF asbr summary db. "
                "( rc = "
                << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_db_asbr_summary_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF database entries of detail asbr-summary information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_asbr_summary_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_summary_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf_db_asbr_summary_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF asbr summary db. "
                "( rc = "
                << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf_db_detail_summary_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf_db_detail_summary_entry_t. */
        entry.inst_id = FRR_OSPF_DEFAULT_INSTANCE_ID;
        OSPF_frr_ospf_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF_frr_ospf_db_asbr_summary_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF database detail external information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf_db_external_data_entry_t */
static void OSPF_frr_ospf_db_external_info_mapping(
    const APPL_FrrOspfDbExternalStateVal *const frr_val,
    vtss_appl_ospf_db_external_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspfDbExternalStateVal to
     *  vtss_appl_ospf_db_external_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->network_mask = frr_val->network_mask;
    info->metric = frr_val->metric;
    info->metric_type = frr_val->metric_type;
    info->forward_address = frr_val->forward_address;
}

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
    const vtss_appl_ospf_id_t *const cur_inst_id,
    vtss_appl_ospf_id_t *const next_inst_id,
    const vtss_appl_ospf_area_id_t *const cur_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_inst_id || !next_area_id || !next_lsdb_type ||
        !next_link_state_id || !next_router_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_inst_id' or 'next_area_id' or "
                          "'next_lsdb_type' or 'next_link_state_id' or "
                          "'next_router_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_external_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF external db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Invoke Map.greater_than() or Map.greater_than_or_equal() get the next
     * entry.
     * If all keys are assigned from caller, then invoking greater_than()
     * If the n-th key is NULL, then all the keys after n-th key (including
     * itself) are assigned the first possible value.
     */
    Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbExternalStateVal>::iterator itr;
    APPL_FrrOspfDbCommonKey frr_key({0, 0, vtss::FrrOspfLsdbType_None, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_area_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
            frr_key.adv_router = *cur_router_id;
        }
    }

    if (is_greater_or_equal) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal() from ";
        itr = result_list.val.greater_than_or_equal(frr_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() from ";
        itr = result_list.val.greater_than(frr_key);
    }

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << AsIpv4(frr_key.area_id) << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF_frr_ospf_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                        next_lsdb_type, next_link_state_id,
                                        next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t inst_id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_external_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_external_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF external db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_db_external_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF database entries of detail external information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_external_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_external_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf_db_external_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF external db. "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf_db_detail_external_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf_db_detail_external_entry_t. */
        entry.inst_id = FRR_OSPF_DEFAULT_INSTANCE_ID;
        OSPF_frr_ospf_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF_frr_ospf_db_external_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF database detail nssa external information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf_db_external_data_entry_t */
static void OSPF_frr_ospf_db_nssa_external_info_mapping(
    const APPL_FrrOspfDbNSSAExternalStateVal *const frr_val,
    vtss_appl_ospf_db_external_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspfDbNSSAExternalStateVal to
     *  vtss_appl_ospf_db_external_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->network_mask = frr_val->network_mask;
    info->metric = frr_val->metric;
    info->metric_type = frr_val->metric_type;
    info->forward_address = frr_val->forward_address;
}

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
    const vtss_appl_ospf_id_t *const cur_inst_id,
    vtss_appl_ospf_id_t *const next_inst_id,
    const vtss_appl_ospf_area_id_t *const cur_area_id,
    vtss_appl_ospf_area_id_t *const next_area_id,
    const vtss_appl_ospf_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf_router_id_t *const cur_router_id,
    vtss_appl_ospf_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_inst_id || !next_area_id || !next_lsdb_type ||
        !next_link_state_id || !next_router_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_inst_id' or 'next_area_id' or "
                          "'next_lsdb_type' or 'next_link_state_id' or "
                          "'next_router_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf_db_nssa_external_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF nssa external db. "
                "( rc = "
                << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Invoke Map.greater_than() or Map.greater_than_or_equal() get the next
     * entry.
     * If all keys are assigned from caller, then invoking greater_than()
     * If the n-th key is NULL, then all the keys after n-th key (including
     * itself) are assigned the first possible value.
     */
    Map<APPL_FrrOspfDbCommonKey, APPL_FrrOspfDbNSSAExternalStateVal>::iterator itr;
    APPL_FrrOspfDbCommonKey frr_key({0, 0, vtss::FrrOspfLsdbType_None, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_area_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF_LSDB_TYPE_UNKNOWN) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
            frr_key.adv_router = *cur_router_id;
        }
    }

    if (is_greater_or_equal) {
        VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal() from ";
        itr = result_list.val.greater_than_or_equal(frr_key);
    } else {
        VTSS_TRACE(DEBUG) << "Invoke greater_than() from ";
        itr = result_list.val.greater_than(frr_key);
    }

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << AsIpv4(frr_key.area_id) << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF_frr_ospf_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                        next_lsdb_type, next_link_state_id,
                                        next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf_id_t inst_id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf_router_id_t adv_router_id,
    vtss_appl_ospf_db_external_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf_db_nssa_external_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF nssa external db. "
                "( rc = "
                << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF_frr_ospf_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF_frr_ospf_db_nssa_external_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF database entries of detail nssa-external information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf_db_detail_nssa_external_get_all(
    vtss::Vector<vtss_appl_ospf_db_detail_external_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf_db_nssa_external_get(FRR_OSPF_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF nssa external db. "
                "( rc = "
                << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        VTSS_TRACE(DEBUG) << "Access framework done: no entry.";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf_db_detail_external_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf_db_detail_external_entry_t. */
        entry.inst_id = FRR_OSPF_DEFAULT_INSTANCE_ID;
        OSPF_frr_ospf_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF_frr_ospf_db_nssa_external_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
/** Module error text (convert the return code to error text)                 */
/******************************************************************************/
const char *frr_ospf_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_FRR_OSPF_ERROR_INVALID_ROUTER_ID:
        return "The OSPF router ID is invalid";

    case VTSS_APPL_FRR_OSPF_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT:
        return "The router ID change will take effect after restart OSPF "
               "process";

    case VTSS_APPL_FRR_OSPF_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT:
        return "The OSPF area ID change doesn't take effect";

    case FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST:
        return "The VLAN interface does not exist";

    case VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE:
        return "Backbone can not be configured as stub area";

    case VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_VIRTUAL_LINK:
        return "This area contains virtual link, can not be configured as stub "
               "area";

    case VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_TOO_LONG:
        return "The password/key is too long";

    case VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID:
        return "The password/key is invalid";

    case VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_BACKBONE:
        return "Backbone area can not be configured as virtual link";

    case VTSS_APPL_FRR_OSPF_ERROR_VIRTUAL_LINK_NOT_ON_STUB:
        return "Virtual link can not be configured in stub area";

    case VTSS_APPL_FRR_OSPF_ERROR_AREA_RANGE_COST_CONFLICT:
        return "Area range not-advertise and cost can not be set at the same "
               "time";
    case VTSS_APPL_FRR_OSPF_ERROR_AREA_RANGE_NETWORK_DEFAULT:
        return "Area range network address cannot represent default";

    case VTSS_APPL_FRR_OSPF_ERROR_DEFFERED_SHUTDOWN_IN_PROGRESS:
        return "Cannot enable OSPF due to deferred shutdown in progress";
    }

    return "FRR OSPF: Unknown error code";
}

/******************************************************************************/
/** Module initialization                                                     */
/******************************************************************************/
#if defined(VTSS_SW_OPTION_JSON_RPC)
VTSS_PRE_DECLS void frr_ospf_json_init(void);
#endif /* VTSS_SW_OPTION_JSON_RPC */

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
/* Initialize private mib */
VTSS_PRE_DECLS void frr_ospf_mib_init(void);
#endif

extern "C" int frr_ospf_icli_cmd_register();

/* Initialize module */
mesa_rc frr_ospf_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT: {
        VTSS_TRACE(INFO) << "INIT";
        /* Initialize and register OSPF mutex */
        critd_init(&FRR_ospf_crit, "frr_ospf.crit", VTSS_MODULE_ID_FRR, CRITD_TYPE_MUTEX);

#if defined(VTSS_SW_OPTION_ICFG)
        /* Initialize and register ICFG resources */
        if (frr_has_ospfd()) {
            frr_ospf_icfg_init();
        }
#endif /* VTSS_SW_OPTION_ICFG */

#if defined(VTSS_SW_OPTION_JSON_RPC)
        /* Initialize and register JSON resources */
        if (frr_has_ospfd()) {
            frr_ospf_json_init();
        }
#endif /* VTSS_SW_OPTION_JSON_RPC */

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        /* Initialize and register private MIB resources */
        if (frr_has_ospfd()) {
            frr_ospf_mib_init();
        }
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */

        /* Initialize and register ICLI resources */
        if (frr_has_ospfd()) {
            frr_ospf_icli_cmd_register();
        }

        /* Initialize local resources */
        ospf_enabled_instances.clear();

        VTSS_TRACE(INFO) << "INIT - completed";
        break;
    }

    case INIT_CMD_START: {
        VTSS_TRACE(INFO) << "START";
        /* Register system reset callback -
         * If the callback triggers stub router advertisement, the
         * maximum waiting time is 100 seconds
         */
        control_system_reset_register(frr_ospf_pre_shutdown_callback, VTSS_MODULE_ID_FRR_OSPF);
        VTSS_TRACE(INFO) << "START - completed";
        break;
    }

    case INIT_CMD_CONF_DEF: {
        VTSS_TRACE(INFO) << "CONF_DEF, isid: " << isid;
        /* Disable all OSPF routing processes */
        CRIT_SCOPE();
        if (frr_has_ospfd()) {
            OSPF_process_disabled();
        }

        VTSS_TRACE(INFO) << "CONF_DEF - completed";
        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

