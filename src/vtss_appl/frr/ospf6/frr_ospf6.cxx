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
#include "frr_ospf6_access.hxx"
#include "frr_ospf6_api.hxx"         // For module APIs
#include "frr_ospf6_serializer.hxx"  // For module serializer
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
#include "frr_ospf6_icfg.hxx"  // For module ICFG
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
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_OSPF6
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Module semaphore/mutex declaration                                        */
/******************************************************************************/
static critd_t FRR_ospf6_crit;

struct FRR_ospf6_lock {
    FRR_ospf6_lock(int line)
    {
        critd_enter(&FRR_ospf6_crit, __FILE__, line);
    }
    ~FRR_ospf6_lock()
    {
        critd_exit( &FRR_ospf6_crit, __FILE__, 0);
    }
};

/* Semaphore/mutex protection
 * Usage:
 * 1. Every non-static function called `OSPF6_xxx` has a CRIT_SCOPE() as the
 *    first thing in the body.
 * 2. No static function has a CRIT_SCOPE()
 * 3. If the non-static functions are not allowed to call non-static functions.
 *   (if needed, then move the functionality to a static function)
 */
#define CRIT_SCOPE() FRR_ospf6_lock __lock_guard__(__LINE__)

/* This macro definition is used to make sure the following codes has been
 * protected by semaphore/mutex alreay. In most cases, we use it in the static
 * function. The system will raise an error if the upper layer caller doesn't
 * call CRIT_SCOPE() before calling the API. */
#define FRR_CRIT_ASSERT_LOCKED() \
    critd_assert_locked(&FRR_ospf6_crit, __FILE__, __LINE__)

/******************************************************************************/
/** Internal variables and APIs                                               */
/******************************************************************************/
/* The database to store the OSPF6 routing process state for the specific
 * instance ID.
 *
 * Background:
 * FRR only saves the instance(s) which OSPF6 routing process is enabled.
 * There are two ways to get the information via FRR VTY commands.
 * ('show running-config' or 'show ipv6 ospf6')
 * Both commands need to parsing the output via FRR VTY socket.
 * It can save the processing time if we store the enabled instances in a local
 * database.
 */
static vtss::Set<vtss_appl_ospf6_id_t> ospf6_enabled_instances;

static mesa_rc OSPF6_if_itr(const vtss_ifindex_t *in, vtss_ifindex_t *out)
{
    return vtss_appl_ip_if_itr(in, out, true /* VLAN interfaces, only */);
}

// TODO:
// FRR 2.0 doesn't support multiple OSPF6 instance ID yet.
// In application layer, the variable of OSPF6 instance ID is reserved for
// the further usage. Only 1 is accepted for the current stage.
//
/* Check OSPF6 instance ID is in valid range
 * Return true when it is in valid range. Otherwise, return false.
 */
static bool OSPF6_instance_id_valid(const vtss_appl_ospf6_id_t id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check valid range */
    if (id < VTSS_APPL_OSPF6_INSTANCE_ID_START ||
        id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is out of range. "
                          << "(range from = " << VTSS_APPL_OSPF6_INSTANCE_ID_START
                          << "to" << VTSS_APPL_OSPF6_INSTANCE_ID_MAX << ")";
        return false;
    }

    return true;
}

/* Check OSPF6 instance ID parameter is existing or not */
static bool OSPF6_instance_id_existing(const vtss_appl_ospf6_id_t id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!OSPF6_instance_id_valid(id)) {
        return false;
    }

    /* Lookup this entry if already existing */
    auto itr = ospf6_enabled_instances.find(id);
    return (itr != ospf6_enabled_instances.end()) ? true : false;
}

/* Disable all OSPF6 routing processes */
static void OSPF6_process_disabled(void)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Stop all OSPF6 routing processes */
    if (frr_daemon_stop(FRR_DAEMON_TYPE_OSPF6) == VTSS_RC_OK) {
        /* Clear the local database of OSPF6 enabled instances */
        ospf6_enabled_instances.clear();
    }
}

/**
 * \brief Get the OSPF6 default instance for clearing OSPF6 routing process .
 * \param id [OUT] OSPF6 instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_def(vtss_appl_ospf6_id_t *const id)
{
    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF6_INSTANCE_ID_START;
    return VTSS_RC_OK;
}

/* Detect if the router ID change take effect or not.
 * return 'true' means that the router ID change will take effect immediately.
 * 'false' means that the router ID change will take effect after restart OSPF6
 * process.
 */
static mesa_bool_t OSPF6_is_router_id_change_take_effect(
    const vtss_appl_ospf6_id_t id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        // Given an error message since it should never happen
        VTSS_TRACE(ERROR) << "Get OSPF6 status failed";
        return true;
    }

    if (result_list->areas.empty()) {
        VTSS_TRACE(DEBUG) << "Empty area";
        return true;
    }

    /* When there is one or more fully adjacent neighbors in area, the new
     * router
     * ID will take effect after restart OSPF6 process */
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

/* Mapping xxx to vtss_appl_ospf6_route_type_t */
static vtss_appl_ospf6_route_type_t frr_ospf6_route_type_mapping(
    const FrrOspf6RouteType rt_type)
{
    switch (rt_type) {
    case RT_Network:
        return VTSS_APPL_OSPF6_ROUTE_TYPE_INTRA_AREA;

    case RT_NetworkIA:
        return VTSS_APPL_OSPF6_ROUTE_TYPE_INTER_AREA;

    case RT_Router:
        return VTSS_APPL_OSPF6_ROUTE_TYPE_BORDER_ROUTER;

    case RT_ExtNetworkTypeOne:
        return VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_1;

    case RT_ExtNetworkTypeTwo:
        return VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_2;

    default:  // 'RT_DiscardIA' doens't support
        break;
    }

    return VTSS_APPL_OSPF6_ROUTE_TYPE_UNKNOWN;
}

/* Mapping xxx to vtss_appl_ospf6_route_type_t */
static FrrOspf6RouteType frr_ospf6_access_route_type_mapping(
    const vtss_appl_ospf6_route_type_t rt_type)
{
    switch (rt_type) {
    case VTSS_APPL_OSPF6_ROUTE_TYPE_INTRA_AREA:
        return RT_Network;

    case VTSS_APPL_OSPF6_ROUTE_TYPE_INTER_AREA:
        return RT_NetworkIA;

    case VTSS_APPL_OSPF6_ROUTE_TYPE_BORDER_ROUTER:
        return RT_Router;

    case VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_1:
        return RT_ExtNetworkTypeOne;

    case VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_2:
        return RT_ExtNetworkTypeTwo;

    case VTSS_APPL_OSPF6_ROUTE_TYPE_UNKNOWN:
    default:
        return RT_DiscardIA;
    }
}

/* Mapping xxx to vtss_appl_ospf6_lsdb_type_t */
static vtss_appl_ospf6_lsdb_type_t frr_ospf6_db_type_mapping(const int32_t lsdb_type)
{
    switch (lsdb_type) {
    case 0x2001:
        return VTSS_APPL_OSPF6_LSDB_TYPE_ROUTER;

    case 0x2002:
        return VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK;

    case 0x2003:
        return VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX;

    case 0x2004:
        return VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_ROUTER;

    case 0x4005:
        return VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL;

    case 0x0008:
        return VTSS_APPL_OSPF6_LSDB_TYPE_LINK;

    case 0x2009:
        return VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX;

    default:  // 'RT_DiscardIA' doens't support
        break;
    }

    return VTSS_APPL_OSPF6_LSDB_TYPE_NONE;
}

/* Mapping xxx to vtss_appl_ospf6_route_type_t */
static int32_t frr_ospf6_access_db_type_mapping(
    const vtss_appl_ospf6_lsdb_type_t lsdb_type)
{
    switch (lsdb_type) {
    case VTSS_APPL_OSPF6_LSDB_TYPE_ROUTER:
        return 0x2001;

    case VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK:
        return 0x2002;

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX:
        return 0x2003;

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_ROUTER:
        return 0x2004;

    case VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL:
        return 0x4005;

    case VTSS_APPL_OSPF6_LSDB_TYPE_LINK:
        return 0x0008;

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX:
        return 0x2009;

    case VTSS_APPL_OSPF6_LSDB_TYPE_NONE:
    default:
        return 0;
    }
}

/* Mapping xxx to vtss_appl_ospf6_route_type_t */
static FrrOspf6LsdbType frr_ospf6_access_lsdb_type_mapping(
    const vtss_appl_ospf6_lsdb_type_t lsdb_type)
{
    switch (lsdb_type) {
    case VTSS_APPL_OSPF6_LSDB_TYPE_ROUTER:
        return FrrOspf6LsdbType_Router;

    case VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK:
        return FrrOspf6LsdbType_Network;

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX:
        return FrrOspf6LsdbType_InterPrefix;

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_ROUTER:
        return FrrOspf6LsdbType_InterRouter;

    case VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL:
        return FrrOspf6LsdbType_AsExternal;

    case VTSS_APPL_OSPF6_LSDB_TYPE_LINK:
        return FrrOspf6LsdbType_Link;

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX:
        return FrrOspf6LsdbType_IntraPrefix;

    case VTSS_APPL_OSPF6_LSDB_TYPE_NONE:
    default:
        return FrrOspf6LsdbType_None;
    }
}

/******************************************************************************/
/** Module public header                                                      */
/******************************************************************************/

//------------------------------------------------------------------------------
//** OSPF6 capabilities
//------------------------------------------------------------------------------
/**
 * \brief Get OSPF6 capabilities to see what supported or not
 * \param cap [OUT] OSPF6 capabilities
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_capabilities_get(vtss_appl_ospf6_capabilities_t *const cap)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!cap) {
        VTSS_TRACE(ERROR) << "Parameter 'cap' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    cap->instance_id_min = VTSS_APPL_OSPF6_INSTANCE_ID_START;
    cap->instance_id_max = VTSS_APPL_OSPF6_INSTANCE_ID_MAX;
    cap->router_id_min = VTSS_APPL_OSPF6_ROUTER_ID_MIN;
    cap->router_id_max = VTSS_APPL_OSPF6_ROUTER_ID_MAX;
    cap->priority_min = VTSS_APPL_OSPF6_PRIORITY_MIN;
    cap->priority_max = VTSS_APPL_OSPF6_PRIORITY_MAX;
    cap->general_cost_min = VTSS_APPL_OSPF6_GENERAL_COST_MIN;
    cap->general_cost_max = VTSS_APPL_OSPF6_GENERAL_COST_MAX;
    cap->intf_cost_min = VTSS_APPL_OSPF6_INTF_COST_MIN;
    cap->intf_cost_max = VTSS_APPL_OSPF6_INTF_COST_MAX;
    cap->hello_interval_min = VTSS_APPL_OSPF6_HELLO_INTERVAL_MIN;
    cap->hello_interval_max = VTSS_APPL_OSPF6_HELLO_INTERVAL_MAX;
    cap->retransmit_interval_min = VTSS_APPL_OSPF6_RETRANSMIT_INTERVAL_MIN;
    cap->retransmit_interval_max = VTSS_APPL_OSPF6_RETRANSMIT_INTERVAL_MAX;
    cap->dead_interval_min = VTSS_APPL_OSPF6_DEAD_INTERVAL_MIN;
    cap->dead_interval_max = VTSS_APPL_OSPF6_DEAD_INTERVAL_MAX;
    cap->ripng_redistributed_supported = frr_has_ripd();

    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
//** OSPF6 instance configuration
//------------------------------------------------------------------------------
/**
 * \brief Add the OSPF6 instance.
 * \param id [IN] OSPF6 instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_add(const vtss_appl_ospf6_id_t id)
{
    CRIT_SCOPE();

    mesa_rc rc;

    /* Check illegal parameters */
    if (!OSPF6_instance_id_valid(id)) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Lookup this entry if existing or not. */
    if (OSPF6_instance_id_existing(id)) {
        return VTSS_RC_OK;  // Already existing, do nothing here
    }

    /* Get deferred shutdown timer from OSPF6 router status.
     * Notice that we have to call the FRR APIs directly since the OSPF6
     * process maybe still in progess due to the stub router setting.
     * Make sure the OSPF6 VTY is ready for the current status access
     * via API frr_daemon_started(FRR_DAEMON_TYPE_OSPF6).
     */
    if (frr_daemon_started(FRR_DAEMON_TYPE_OSPF6)) {
        auto router_status = frr_ip_ospf6_status_get();
        if (router_status.rc == VTSS_RC_OK) {
            if (router_status->deferred_shutdown_time.raw32() > 0) {
                VTSS_TRACE(DEBUG)
                        << "Cannot enable OSPF6 due to deferred shutdown "
                        "in progress, left "
                        << router_status->deferred_shutdown_time.raw32()
                        << "(ms) remaining ";
#ifdef VTSS_SW_OPTION_SYSLOG
                S_N("Cannot enable OSPF6 due to deferred shutdown in progress, "
                    "left "
                    "%d(ms) remaining",
                    router_status->deferred_shutdown_time.raw32());
#endif /* VTSS_SW_OPTION_SYSLOG */
                return VTSS_APPL_FRR_OSPF6_ERROR_DEFFERED_SHUTDOWN_IN_PROGRESS;
            }
        }
    }

    /* The instance ID doesnot exist. Add it as new one */
    /* Apply to FRR layer */
    rc = frr_ospf6_router_conf_set(id);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Add OSPF6 instance. "
                          "(instance_id = "
                          << id << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Update internal database when the operation is done successfully */
    ospf6_enabled_instances.insert(id);

    return VTSS_RC_OK;
}

/**
 * \brief Delete the OSPF6 instance.
 * \param id [IN] OSPF6 instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_del(const vtss_appl_ospf6_id_t id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF6_instance_id_valid(id)) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Lookup this entry if already existing */
    auto itr = vtss::find(ospf6_enabled_instances.begin(),
                          ospf6_enabled_instances.end(), id);
    if (itr == ospf6_enabled_instances.end()) {
        return VTSS_RC_OK;  // Quit silently even if it doesn't exist
    }

    /* Apply to FRR layer */
    mesa_rc rc = frr_ospf6_router_conf_del(id);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete OSPF6 instance. "
                          "(instance_id = "
                          << id << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Update internal database after FRR layer is applied successfully */
    ospf6_enabled_instances.erase(itr);

    /* Do not stop ospf6d here, maybe some ospf6 interface configurations exists,
       If we stop , we loss the configurations */

    /* If the stub router on-shutdown is configured, the router will not
     * terminate immediately, here do syslog to indicate the shutdown will
     * be deffered.
     */
    /* Get config from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 instance which the OSPF6 routing process is enabled.
 * \param id [IN] OSPF6 instance ID.
 * \return Error code.  VTSS_RC_OK means that OSPF6 routing process is enabled
 *                      on the instance ID.
 *                      VTSS_RC_ERROR means that the instance ID is not created
 *                      and OSPF6 routing process is disabled.
 */
mesa_rc vtss_appl_ospf6_get(const vtss_appl_ospf6_id_t id)
{
    CRIT_SCOPE();
    return OSPF6_instance_id_existing(id) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

static mesa_rc OSPF6_inst_itr(const vtss_appl_ospf6_id_t *const current_id,
                              vtss_appl_ospf6_id_t *const next_id)
{
    FRR_CRIT_ASSERT_LOCKED();

    vtss::Set<vtss_appl_ospf6_id_t>::iterator itr;

    if (current_id) {
        itr = ospf6_enabled_instances.greater_than(*current_id);
    } else {
        itr = ospf6_enabled_instances.begin();
    }

    if (itr == ospf6_enabled_instances.end()) {
        VTSS_TRACE(DEBUG) << "NOT_FOUND";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    VTSS_TRACE(DEBUG) << "Found: " << *itr;
    *next_id = *itr;

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all OSPF6 instances.
 * \param current_id [IN]   Pointer to the current instance ID. Use null pointer
 *                          to get the first instance ID.
 * \param next_id    [OUT]  Pointer to the next instance ID
 * \return Error code.      VTSS_RC_OK means that the next instance ID is valid
 *                          and the vaule is saved in 'out' paramater.
 *                          VTSS_RC_ERROR means that the next instance ID is
 *                          non-existing.
 */
mesa_rc vtss_appl_ospf6_inst_itr(const vtss_appl_ospf6_id_t *const current_id,
                                 vtss_appl_ospf6_id_t *const next_id)
{
    CRIT_SCOPE();
    return OSPF6_inst_itr(current_id, next_id);
}

/**
 * \brief Set OSPF6 control of global options.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_control_globals(
    const vtss_appl_ospf6_control_globals_t *const control)
{
    CRIT_SCOPE();

    mesa_rc rc;

    if (control->reload_process && !ospf6_enabled_instances.empty()) {
        /* Apply to FRR layer */
        if ((rc = frr_daemon_reload(FRR_DAEMON_TYPE_OSPF6)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Reload OSPF6 process failed";
            return rc;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get OSPF6 control of global options.
 * It is a dummy function for SNMP serialzer only.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc frr_ospf6_control_globals_dummy_get(
    vtss_appl_ospf6_control_globals_t *const control)
{
    if (control) {
        vtss_clear(*control);
    }
    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
//** OSPF6 router configuration/status
//------------------------------------------------------------------------------

/* Notice !!!
 * The command "no router-id" is unsupported in FRR v2.0.
 * To clear the current configured router ID, we need to set value 0 in FRRv2.0
 * command. For example, (config-router)# router-id 0
 */
#define FRR_V2_OSPF6_DEFAULT_ROUTER_ID 0

/**
 * \brief Get the OSPF6 router configuration.
 * \param id   [IN] OSPF6 instance ID.
 * \param conf [OUT] OSPF6 router configuration.
 * \return Error code.
 */
mesa_rc frr_ospf6_router_conf_def(vtss_appl_ospf6_id_t *const id,
                                  vtss_appl_ospf6_router_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero initial value below
    conf->router_id.is_specific_id = false;
    conf->router_id.id = VTSS_APPL_OSPF6_ROUTER_ID_MIN;
    for (u32 idx = 0; idx < VTSS_APPL_OSPF6_REDIST_PROTOCOL_COUNT; ++idx) {
        conf->redist_conf[idx].is_redist_enable = false;
    }

    conf->admin_distance = 110;

    return VTSS_RC_OK;
}

static mesa_rc OSPF6_router_conf_get(const vtss_appl_ospf6_id_t id,
                                     vtss_appl_ospf6_router_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();
    vtss_appl_ospf6_router_conf_t def_conf;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF6_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // Get default configuration
    if (frr_ospf6_router_conf_def((vtss_appl_ospf6_id_t *)&id, &def_conf) !=
        VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Get OSPF6 router default configuration failed.";
        return FRR_RC_INTERNAL_ERROR;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    // Get router ID
    auto frr_router_conf = frr_ospf6_router_conf_get(running_conf, id);
    if (frr_router_conf.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get router configuration. (instance_id = %u, rc = %s)", id, error_txt(frr_router_conf.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    memset(&conf->router_id, 0, sizeof(conf->router_id));
    if (frr_router_conf->ospf6_router_id.valid()) {
        conf->router_id.id = frr_router_conf->ospf6_router_id.get();
        conf->router_id.is_specific_id =
            (conf->router_id.id != FRR_V2_OSPF6_DEFAULT_ROUTER_ID);
    } else {
        conf->router_id.id = VTSS_APPL_OSPF6_ROUTER_ID_MIN;
    }

    // Get the route redistribution
    vtss_clear(conf->redist_conf);
    Vector<FrrOspf6RouterRedistribute> frr_redist = frr_ospf6_router_redistribute_conf_get(running_conf, id);
    for (const auto &itr : frr_redist) {
        vtss_appl_ospf6_redist_conf_t *redist_conf;
        if (itr.protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED) {
            redist_conf = &conf->redist_conf[VTSS_APPL_OSPF6_REDIST_PROTOCOL_CONNECTED];
        } else if (itr.protocol == VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC) {
            redist_conf = &conf->redist_conf[VTSS_APPL_OSPF6_REDIST_PROTOCOL_STATIC];
        } else {
            continue;
        }

        redist_conf->is_redist_enable = true;
    }

    // Get the administrative distance
    auto frr_admin_distance = frr_ospf6_router_admin_distance_get(running_conf, id);
    if (frr_admin_distance.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get administrative distance. (instance_id = %u, rc = %s)", id, error_txt(frr_admin_distance.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->admin_distance = frr_admin_distance.val;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 router configuration.
 * \param id   [IN] OSPF6 instance ID.
 * \param conf [OUT] OSPF6 router configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_conf_get(const vtss_appl_ospf6_id_t id,
                                        vtss_appl_ospf6_router_conf_t *const conf)
{
    CRIT_SCOPE();
    return OSPF6_router_conf_get(id, conf);
}

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
    const vtss_appl_ospf6_id_t id,
    const vtss_appl_ospf6_router_conf_t *const conf)
{
    CRIT_SCOPE();

    mesa_rc rc;
    vtss_appl_ospf6_router_conf_t orig_conf;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF6_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->router_id.is_specific_id &&
        (conf->router_id.id < VTSS_APPL_OSPF6_ROUTER_ID_MIN ||
         conf->router_id.id > VTSS_APPL_OSPF6_ROUTER_ID_MAX)) {
        VTSS_TRACE(DEBUG) << "Parameter 'router ID' is invalid"
                          << conf->router_id.id;
        return VTSS_APPL_FRR_OSPF6_ERROR_INVALID_ROUTER_ID;
    }

    if (conf->admin_distance &&
        (conf->admin_distance < VTSS_APPL_OSPF6_ADMIN_DISTANCE_MIN)) {
        VTSS_TRACE(DEBUG) << "Parameter 'admin_distance'("
                          << conf->admin_distance << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get the original configuration */
    rc = OSPF6_router_conf_get(id, &orig_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    if (orig_conf.router_id.is_specific_id != conf->router_id.is_specific_id ||
        (orig_conf.router_id.is_specific_id &&
         orig_conf.router_id.id != conf->router_id.id)) {
        vtss::FrrOspf6RouterConf router_conf;
        if (conf->router_id.is_specific_id) {
            router_conf.ospf6_router_id = conf->router_id.id;
        } else {
            router_conf.ospf6_router_id = FRR_V2_OSPF6_DEFAULT_ROUTER_ID;
        }

        rc = frr_ospf6_router_conf_set(id, router_conf);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set router configuration. "
                    "(instance_id = "
                    << id << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }

        if (!OSPF6_is_router_id_change_take_effect(id)) {
            return VTSS_APPL_FRR_OSPF6_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT;
        }
    }

// Apply new route redistribution
    for (uint32_t idx = 0; idx < VTSS_APPL_OSPF6_REDIST_PROTOCOL_COUNT; idx++) {
        const vtss_appl_ospf6_redist_conf_t *orig_redist_conf = &orig_conf.redist_conf[idx];
        const vtss_appl_ospf6_redist_conf_t *redist_conf = &conf->redist_conf[idx];
        if (orig_redist_conf->is_redist_enable == redist_conf->is_redist_enable) {
            continue;  // Do nothing when the values are the same.
        }

        vtss_appl_ip_route_protocol_t protocol;

        if (idx == VTSS_APPL_OSPF6_REDIST_PROTOCOL_CONNECTED) {
            protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED;
        } else  if (idx == VTSS_APPL_OSPF6_REDIST_PROTOCOL_STATIC) {
            protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
        }

        // Use VTSS_APPL_OSPF6_REDIST_METRIC_TYPE_NONE to delete
        // default route configuration
        if (redist_conf->is_redist_enable == false) {
            rc = frr_ospf6_router_redistribute_conf_del(id, protocol);
        } else {
            FrrOspf6RouterRedistribute frr_redist_conf;

            frr_redist_conf.protocol = protocol;
            rc = frr_ospf6_router_redistribute_conf_set(id, frr_redist_conf);
        }

        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework failed: Set route redistribution "
                    "(instance_id = "
                    << id << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    // Apply new administrative distance
    if (orig_conf.admin_distance != conf->admin_distance) {
        rc = frr_ospf6_router_admin_distance_set(id, conf->admin_distance);
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

static mesa_rc OSPF6_router_intf_conf_get(
    const vtss_appl_ospf6_id_t id, const vtss_ifindex_t ifindex,
    vtss_appl_ospf6_router_intf_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF6_instance_id_existing(id)) {
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
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    // area-id
    auto frr_intf_area_id = frr_ospf6_router_if_area_id_conf_get(running_conf, id, ifindex);
    if (frr_intf_area_id.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface retransmit interval. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_area_id.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->area_id.id = frr_intf_area_id.val.id;
    conf->area_id.is_specific_id = frr_intf_area_id.val.is_specific_id;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 router interface configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF6 router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_intf_conf_get(
    const vtss_appl_ospf6_id_t id, const vtss_ifindex_t ifindex,
    vtss_appl_ospf6_router_intf_conf_t *const conf)
{
    CRIT_SCOPE();
    return OSPF6_router_intf_conf_get(id, ifindex, conf);
}

/**
 * \brief Set the OSPF6 router interface configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] OSPF6 router interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_intf_conf_set(
    const vtss_appl_ospf6_id_t id, const vtss_ifindex_t ifindex,
    const vtss_appl_ospf6_router_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    vtss_appl_ospf6_router_intf_conf_t orig_conf;

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF6_instance_id_existing(id)) {
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
    mesa_rc rc = OSPF6_router_intf_conf_get(id, ifindex, &orig_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    if ((orig_conf.area_id.id != conf->area_id.id ||
         conf->area_id.is_specific_id != orig_conf.area_id.is_specific_id) &&
        conf->area_id.is_specific_id == true) {
        if (orig_conf.area_id.is_specific_id == true) {
            /* When Nw area id need to be configured old one need to be deleted */
            rc = frr_ospf6_router_if_area_conf_del(id, ifindex,
                                                   orig_conf.area_id);
            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Access framework: Clear Previous Area ID. (instance_id "
                        "= "
                        << id << ", ifindex = " << ifindex << ", rc = " << rc << ")";
                return FRR_RC_INTERNAL_ACCESS;
            }
        }
        rc = frr_ospf6_router_if_area_conf_set(id, ifindex,
                                               conf->area_id);
        if (rc != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG)
                    << "Access framework: Area id. (instance_id "
                    "= "
                    << id << ", ifindex = " << ifindex << ", rc = " << rc << ")";
            return FRR_RC_INTERNAL_ACCESS;
        }
    }

    /* true case is taken in previousif */
    if (conf->area_id.is_specific_id != orig_conf.area_id.is_specific_id) {
        if (conf->area_id.is_specific_id == false) {
            rc = frr_ospf6_router_if_area_conf_del(id, ifindex,
                                                   conf->area_id);
            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Access framework: Del Area ID. (instance_id "
                        "= "
                        << id << ", ifindex = " << ifindex << ", rc = " << rc << ")";
                return FRR_RC_INTERNAL_ACCESS;
            }
        }
    }


    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all OSPF6 router interfaces.
 * \param current_id      [IN]  Current OSPF6 ID
 * \param next_id         [OUT] Next OSPF6 ID
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_intf_conf_itr(
    const vtss_appl_ospf6_id_t *const current_id,
    vtss_appl_ospf6_id_t *const next_id,
    const vtss_ifindex_t *const current_ifindex,
    vtss_ifindex_t *const next_ifindex)
{
    CRIT_SCOPE();

    vtss::IteratorComposeN<vtss_appl_ospf6_id_t, vtss_ifindex_t> i(&OSPF6_inst_itr, &OSPF6_if_itr);
    return i(current_id, next_id, current_ifindex, next_ifindex);
}

static mesa_rc OSPF6_router_status_get(const vtss_appl_ospf6_id_t id,
                                       vtss_appl_ospf6_router_status_t *const status)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (!status) {
        return VTSS_RC_ERROR;
    }

    /* Lookup this entry if existing or not. */
    if (!OSPF6_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 status. "
                          "(instance_id = "
                          << id << ", rc = " << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    status->ospf6_router_id = result_list->router_id;
    status->spf_delay = result_list->spf_schedule_delay.raw32();
    status->spf_holdtime = result_list->hold_time_min.raw32();
    status->spf_max_waittime = result_list->hold_time_max.raw32();
    status->last_executed_spf_ts = result_list->spf_last_executed.raw32();
    VTSS_TRACE(DEBUG) << "spf_last_executed.raw() = "
                      << result_list->spf_last_executed.raw()
                      << "spf_last_executed.raw32() = "
                      << result_list->spf_last_executed.raw32();
    status->attached_area_count = result_list->attached_area_counter;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 router status.
 * \param id     [IN] OSPF6 instance ID.
 * \param status [OUT] Status for 'id'.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_router_status_get(
    const vtss_appl_ospf6_id_t id,
    vtss_appl_ospf6_router_status_t *const status)
{
    CRIT_SCOPE();
    return OSPF6_router_status_get(id, status);
}

//------------------------------------------------------------------------------
//** OSPF6 network area configuration/status
//------------------------------------------------------------------------------
static mesa_rc OSPF6_area_status_itr_k2(const vtss_appl_ospf6_area_id_t *const cur,
                                        vtss_appl_ospf6_area_id_t *const next,
                                        vtss_appl_ospf6_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 status. "
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
    vtss::Map<vtss_appl_ospf6_area_id_t, FrrIpOspf6Area> &key2_map =
        result_list->areas;
    vtss::Map<vtss_appl_ospf6_area_id_t, FrrIpOspf6Area>::iterator key2_itr;

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
 * \brief Iterate through the OSPF6 area status.
 * \param cur_id       [IN]  Current OSPF6 ID
 * \param next_id      [OUT] Next OSPF6 ID
 * \param cur_area_id  [IN]  Current area ID
 * \param next_area_id [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_status_itr(
    const vtss_appl_ospf6_id_t *const cur_id,
    vtss_appl_ospf6_id_t *const next_id,
    const vtss_appl_ospf6_area_id_t *const cur_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_id || !next_area_id) {
        VTSS_TRACE(ERROR)
                << "Parameter 'next_id' or 'next_area' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_id && *cur_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf6_id_t, vtss_appl_ospf6_area_id_t> itr(
        OSPF6_inst_itr, OSPF6_area_status_itr_k2);

    return itr(cur_id, next_id, cur_area_id, next_area_id);
}

/* This static function is implemented later. */
static mesa_rc OSPF6_stub_area_conf_get(const vtss_appl_ospf6_id_t id,
                                        const vtss_appl_ospf6_area_id_t area_id,
                                        vtss_appl_ospf6_stub_area_conf_t *const conf);

/**
 * \brief Get the OSPF6 area status.
 * \param id        [IN] OSPF6 instance ID.
 * \param area      [IN] OSPF6 area key.
 * \param status    [OUT] OSPF6 area val.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_status_get(const vtss_appl_ospf6_id_t id,
                                        const vtss_appl_ospf6_area_id_t area,
                                        vtss_appl_ospf6_area_status_t *const status)
{
    CRIT_SCOPE();

    if (!status) {
        return VTSS_RC_ERROR;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 status. "
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
    if (!OSPF6_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::Map<vtss_appl_ospf6_area_id_t, FrrIpOspf6Area> &key2_map =
        result_list->areas;
    vtss::Map<vtss_appl_ospf6_area_id_t, FrrIpOspf6Area>::iterator key2_itr =
        key2_map.find(area);
    if (key2_itr == key2_map.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* get the value of the map */
    const auto &frr_status = key2_itr->second;
    status->is_backbone = frr_status.backbone;

    /* get stub area configuration */
    vtss_appl_ospf6_stub_area_conf_t conf;
    mesa_rc rc = OSPF6_stub_area_conf_get(id, area, &conf);
    if (rc == VTSS_RC_OK) {
        status->area_type =
            conf.no_summary ? VTSS_APPL_OSPF6_AREA_TOTALLY_STUB
            : VTSS_APPL_OSPF6_AREA_STUB;
    } else if (rc == FRR_RC_ENTRY_NOT_FOUND) {
        status->area_type = VTSS_APPL_OSPF6_AREA_NORMAL;
    } else {
        // get stub configuration failed
        status->area_type = VTSS_APPL_OSPF6_AREA_COUNT;
    }

    status->attached_intf_total_count = frr_status.area_if_total_counter;
    status->spf_executed_count = frr_status.spf_executed_counter;
    status->lsa_count = frr_status.lsa_nr;

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 area range
//----------------------------------------------------------------------------
/**
 * \brief Get the OSPF6 area range default configuration.
 * \param id      [IN]  OSPF6 instance ID.
 * \param area_id [IN]  OSPF6 area ID.
 * \param network [IN]  OSPF6 area range network.
 * \param conf    [OUT] OSPF6 area range configuration.
 * \return Error code.
 */
mesa_rc frr_ospf6_area_range_conf_def(vtss_appl_ospf6_id_t *const id,
                                      vtss_appl_ospf6_area_id_t *const area_id,
                                      mesa_ipv6_network_t *const network,
                                      vtss_appl_ospf6_area_range_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF6_INSTANCE_ID_START;
    conf->is_specific_cost = false;
    conf->cost = VTSS_APPL_OSPF6_GENERAL_COST_MIN;
    conf->is_advertised = true;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 area range configuration.
 * \param id            [IN]  OSPF6 instance ID.
 * \param area_id       [IN]  OSPF6 area ID.
 * \param network       [IN]  OSPF6 area range network.
 * \param conf          [OUT] OSPF6 area range configuration.
 * \param check_overlap [IN]  Set 'true' to check if the address range is
 * overlap or not, otherwise not to check it.
 * \return Error code.
 */
static mesa_rc OSPF6_area_range_conf_get(const vtss_appl_ospf6_id_t id,
                                         const vtss_appl_ospf6_area_id_t area_id,
                                         const mesa_ipv6_network_t network,
                                         vtss_appl_ospf6_area_range_conf_t *const conf,
                                         mesa_bool_t check_overlap)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF6_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    auto frr_area_range_conf = frr_ospf6_area_range_conf_get(running_conf, id);
    if (frr_area_range_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty area range";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched entry */
    for (const auto &itr : frr_area_range_conf) {
        if (vtss_ipv6_net_mask_out(&itr.net) == vtss_ipv6_net_mask_out(&network) &&
            itr.area == area_id) {
            // Found it

            // Get area range advertise configuration
            conf->is_advertised = true;
            auto frr_area_range_advertise_conf = frr_ospf6_area_range_not_advertise_conf_get(running_conf, id);
            for (const auto &itr_advertise : frr_area_range_advertise_conf) {
                if (vtss_ipv6_net_mask_out(&itr_advertise.net) == vtss_ipv6_net_mask_out(&network) && itr_advertise.area == area_id) {
                    conf->is_advertised = false;
                    break;
                }
            }

            // Get area range cost configuration
            conf->is_specific_cost = false;
            conf->cost = VTSS_APPL_OSPF6_GENERAL_COST_MIN;
            auto frr_area_range_cost_conf = frr_ospf6_area_range_cost_conf_get(running_conf, id);
            for (const auto &itr_cost : frr_area_range_cost_conf) {
                if (vtss_ipv6_net_mask_out(&itr_cost.net) == vtss_ipv6_net_mask_out(&network) && itr_cost.area == area_id) {
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
            vtss_ipv6_net_overlap(&itr.net, &network)) {
            return FRR_RC_ADDR_RANGE_OVERLAP;
        }
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get the OSPF6 area range configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] OSPF6 area ID.
 * \param network [IN] OSPF6 area range network.
 * \param conf    [IN] OSPF6 area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_get(
    const vtss_appl_ospf6_id_t id, const vtss_appl_ospf6_area_id_t area_id,
    const mesa_ipv6_network_t network,
    vtss_appl_ospf6_area_range_conf_t *const conf)
{
    CRIT_SCOPE();
    return OSPF6_area_range_conf_get(id, area_id, network, conf, false);
}

/**
 * \brief Set the OSPF6 area range configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] OSPF6 area ID.
 * \param network [IN] OSPF6 area range network.
 * \param conf    [IN] OSPF6 area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_set(
    const vtss_appl_ospf6_id_t id, const vtss_appl_ospf6_area_id_t area_id,
    const mesa_ipv6_network_t network,
    const vtss_appl_ospf6_area_range_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF6_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!conf->is_advertised && conf->is_specific_cost) {
        return VTSS_APPL_FRR_OSPF6_ERROR_AREA_RANGE_COST_CONFLICT;
    }

    mesa_ipv6_network_t net = vtss_ipv6_net_mask_out(&network);
    if (vtss_ipv6_addr_is_zero(&net.address)) {
        return VTSS_APPL_FRR_OSPF6_ERROR_AREA_RANGE_NETWORK_DEFAULT;
    }

    mesa_bool_t is_unexp_deleted = false;  // unexpected deleted
    /* Get the original configuration */
    vtss_appl_ospf6_area_range_conf_t orig_conf;
    vtss::FrrOspf6AreaNetwork frr_area_range_conf = {
        vtss_ipv6_net_mask_out(&network), area_id
    };
    mesa_rc rc =
        OSPF6_area_range_conf_get(id, area_id, network, &orig_conf, false);
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
        vtss::FrrOspf6AreaNetworkCost frr_area_range_cost_conf = {
            vtss_ipv6_net_mask_out(&network), area_id, conf->cost
        };
        if (conf->is_specific_cost) {
            rc = frr_ospf6_area_range_cost_conf_set(id, frr_area_range_cost_conf);
        } else {
            frr_area_range_cost_conf.cost = VTSS_APPL_OSPF6_GENERAL_COST_MIN;
            // FRR will delete entry when apply 'no area range cost'!!
            rc = frr_ospf6_area_range_cost_conf_del(id, frr_area_range_cost_conf);
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
            rc = frr_ospf6_area_range_conf_set(id, frr_area_range_conf);
        } else {
            rc = frr_ospf6_area_range_not_advertise_conf_set(
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
 * \brief Add the OSPF6 area range configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] OSPF6 area ID.
 * \param network [IN] OSPF6 area range network.
 * \param conf    [IN] OSPF6 area range configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_add(
    const vtss_appl_ospf6_id_t id, const vtss_appl_ospf6_area_id_t area_id,
    const mesa_ipv6_network_t network,
    const vtss_appl_ospf6_area_range_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!OSPF6_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!conf->is_advertised && conf->is_specific_cost) {
        return VTSS_APPL_FRR_OSPF6_ERROR_AREA_RANGE_COST_CONFLICT;
    }

    mesa_ipv6_network_t net = vtss_ipv6_net_mask_out(&network);
    if (vtss_ipv6_addr_is_zero(&net.address)) {
        return VTSS_APPL_FRR_OSPF6_ERROR_AREA_RANGE_NETWORK_DEFAULT;
    }

    /* Check the entry is existing or not */
    mesa_rc rc;
    vtss_appl_ospf6_area_range_conf_t orig_conf;
    rc = OSPF6_area_range_conf_get(id, area_id, network, &orig_conf, true);
    if (rc == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    /* Apply to FRR layer when the entry is a new one. */
    vtss::FrrOspf6AreaNetwork frr_area_range_conf = {
        vtss_ipv6_net_mask_out(&network), area_id
    };
    rc = frr_ospf6_area_range_conf_set(id, frr_area_range_conf);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Add area range. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id
                          << ", network_addr = " << network << ", rc = " << rc
                          << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (conf->is_advertised) {
        rc = frr_ospf6_area_range_conf_set(id, frr_area_range_conf);
    } else {
        rc = frr_ospf6_area_range_not_advertise_conf_set(id, frr_area_range_conf);
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
        vtss::FrrOspf6AreaNetworkCost frr_area_range_cost_conf = {
            vtss_ipv6_net_mask_out(&network), area_id, conf->cost
        };
        rc = frr_ospf6_area_range_cost_conf_set(id, frr_area_range_cost_conf);

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
 * \brief Delete the OSPF6 area range configuration.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] OSPF6 area ID.
 * \param network [IN] OSPF6 area range network.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_area_range_conf_del(const vtss_appl_ospf6_id_t id,
                                            const vtss_appl_ospf6_area_id_t area_id,
                                            const mesa_ipv6_network_t network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!OSPF6_instance_id_existing(id)) {
        VTSS_TRACE(DEBUG) << "Parameter 'id'(" << id << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check the entry is existing or not */
    vtss_appl_ospf6_area_range_conf_t conf;
    mesa_rc rc = OSPF6_area_range_conf_get(id, area_id, network, &conf, false);
    if (rc != VTSS_RC_OK) {
        // For the deleting operation, quit silently when it does not exists
        return VTSS_RC_OK;
    }

    /* Apply to FRR layer */
    vtss::FrrOspf6AreaNetwork frr_area_range_conf = {
        vtss_ipv6_net_mask_out(&network), area_id
    };
    rc = frr_ospf6_area_range_conf_del(id, frr_area_range_conf);
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

static mesa_rc OSPF6_area_range_conf_itr_k2(const vtss_appl_ospf6_area_id_t *const cur,
                                            vtss_appl_ospf6_area_id_t *const next,
                                            vtss_appl_ospf6_id_t k1)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    /* Get data from FRR layer (not sorted) */
    auto frr_area_range_conf = frr_ospf6_area_range_conf_get(running_conf, k1);

    if (frr_area_range_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    /* Build the local sorted database for key2. */
    vtss::Set<vtss_appl_ospf6_area_id_t> key2_set;
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

static mesa_rc OSPF6_area_range_conf_itr_k3(const mesa_ipv6_network_t *const cur,
                                            mesa_ipv6_network_t *const next,
                                            vtss_appl_ospf6_id_t k1,
                                            vtss_appl_ospf6_area_id_t k2)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    auto frr_area_range_conf = frr_ospf6_area_range_conf_get(running_conf, k1);
    if (frr_area_range_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    vtss::Set<mesa_ipv6_network_t> key3_set;
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
    const vtss_appl_ospf6_id_t *const current_id,
    vtss_appl_ospf6_id_t *const next_id,
    const vtss_appl_ospf6_area_id_t *const current_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id,
    const mesa_ipv6_network_t *const current_network,
    mesa_ipv6_network_t *const next_network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_id || !next_area_id || !next_network) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_area_id' or "
                          "'next_network' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf6_id_t, vtss_appl_ospf6_area_id_t,
         mesa_ipv6_network_t>
         itr(OSPF6_inst_itr, OSPF6_area_range_conf_itr_k2,
             OSPF6_area_range_conf_itr_k3);

    return itr(current_id, next_id, current_area_id, next_area_id,
               current_network, next_network);
}

//----------------------------------------------------------------------------
//** OSPF6 stub area
//----------------------------------------------------------------------------
/**
 * \brief
 * \param id      [IN]  OSPF6 instance ID.
 * \param area_id [IN]  OSPF6 area ID.
 * \param conf    [OUT] OSPF6 area stub configuration.
 * \return Error code. The function doesn't validate the parameter
 * since it's only invoked locally.
 */
static mesa_rc OSPF6_stub_area_conf_get(const vtss_appl_ospf6_id_t id,
                                        const vtss_appl_ospf6_area_id_t area_id,
                                        vtss_appl_ospf6_stub_area_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer. */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    /* Lookup the matched entry in frr_stub_conf,
     * then lookup it again in frr_stub_no_summary_conf if not found.
     */
    auto frr_stub_area_conf = frr_ospf6_stub_area_conf_get(running_conf, id);
    if (!frr_stub_area_conf.empty()) {
        for (const auto &itr : frr_stub_area_conf) {
            if (itr.area > area_id) {
                break;
            }

            if (itr.area == area_id) {
                // Found it
                conf->no_summary = itr.no_summary;
                VTSS_TRACE(DEBUG)
                        << "Found entry: "
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
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \param conf    [IN] The stub area configuration for adding.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_add(
    const vtss_appl_ospf6_id_t id, const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_stub_area_conf_t *const conf)
{
    CRIT_SCOPE();

    mesa_rc rc;
    /* Check illegal parameters. */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Backbone area can't be configured as stub area. */
    if (area_id == FRR_OSPF6_BACKBONE_AREA_ID) {
        return VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE;
    }

    /* Check the entry is existing or not. */
    vtss_appl_ospf6_stub_area_conf_t orig_conf;
    rc = OSPF6_stub_area_conf_get(id, area_id, &orig_conf);
    if (rc == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        return rc;
    }

    FrrOspf6StubArea frr_stub_area_conf(area_id, conf->no_summary);
    rc = frr_ospf6_stub_area_conf_set(id, frr_stub_area_conf);

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
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \param conf    [IN] The stub area configuration for setting.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_set(
    const vtss_appl_ospf6_id_t id, const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_stub_area_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Backbone area can't be configured as stub area. */
    if (area_id == FRR_OSPF6_BACKBONE_AREA_ID) {
        return VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE;
    }

    /* Check the entry is existing or not. */
    mesa_rc rc;
    vtss_appl_ospf6_stub_area_conf_t orig_conf;
    rc = OSPF6_stub_area_conf_get(id, area_id, &orig_conf);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Return OK directly since no changes. */
    if (orig_conf.no_summary == conf->no_summary) {
        return VTSS_RC_OK;
    }

    FrrOspf6StubArea frr_stub_area_conf(area_id, conf->no_summary);
    rc = frr_ospf6_stub_area_conf_set(id, frr_stub_area_conf);

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
 * \param id      [IN]  OSPF6 instance ID.
 * \param area_id [IN]  The area ID of the stub area configuration.
 * \param conf    [OUT] The stub area configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_get(
    const vtss_appl_ospf6_id_t id, const vtss_appl_ospf6_area_id_t area_id,
    vtss_appl_ospf6_stub_area_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    return OSPF6_stub_area_conf_get(id, area_id, conf);
}

/**
 * \brief Delete a specific stub area.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN] The area ID of the stub area configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_del(const vtss_appl_ospf6_id_t id,
                                           const vtss_appl_ospf6_area_id_t area_id)
{
    CRIT_SCOPE();

    /* Check the entry is existing or not. */
    mesa_rc rc;
    vtss_appl_ospf6_stub_area_conf_t orig_conf;
    rc = OSPF6_stub_area_conf_get(id, area_id, &orig_conf);

    /* Return OK silently if not found. */
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Delete stub area. "
                          "(instance_id = "
                          << id << ", area_id = " << area_id << ", \'"
                          << error_txt(rc) << "\')";

        return rc == FRR_RC_ENTRY_NOT_FOUND ? VTSS_RC_OK : rc;
    }

    rc = frr_ospf6_stub_area_conf_del(id, area_id);
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
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN]  The area ID of the stub area configuration.
 * \param conf    [OUT] The stub area configuration.
 * \return Error code.
 */
mesa_rc frr_ospf6_stub_area_conf_def(vtss_appl_ospf6_id_t *const id,
                                     vtss_appl_ospf6_area_id_t *const area_id,
                                     vtss_appl_ospf6_stub_area_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero initial value below
    *id = VTSS_APPL_OSPF6_INSTANCE_ID_START;

    return VTSS_RC_OK;
}

static mesa_rc OSPF6_stub_area_conf_itr_k2(const vtss_appl_ospf6_area_id_t *const cur,
                                           vtss_appl_ospf6_area_id_t *const next,
                                           vtss_appl_ospf6_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Get FRR running-config */
    std::string running_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    /* Get data from FRR layer */
    auto frr_stub_area_conf = frr_ospf6_stub_area_conf_get(running_conf, key1);
    if (frr_stub_area_conf.empty()) {
        // No database here, process the next loop
        return VTSS_RC_ERROR;
    }

    /* Build the local sorted database for key2
     * The stub area database should include the stub areas and
     * totally stub areas both. */
    vtss::Set<vtss_appl_ospf6_area_id_t> key2_set;
    for (const auto &itr : frr_stub_area_conf) {
        key2_set.insert(itr.area);
    }

    Set<vtss_appl_ospf6_area_id_t>::iterator key2_itr;
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
 * \param current_id      [IN]  Current OSPF6 ID
 * \param next_id         [OUT] Next OSPF6 ID
 * \param current_area_id [IN]  Current area ID
 * \param next_area_id    [OUT] Next area ID
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_stub_area_conf_itr(
    const vtss_appl_ospf6_id_t *const current_id,
    vtss_appl_ospf6_id_t *const next_id,
    const vtss_appl_ospf6_area_id_t *const current_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_area_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_area_id' or "
                          "'next_network' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf6_id_t, vtss_appl_ospf6_area_id_t> itr(
        OSPF6_inst_itr, OSPF6_stub_area_conf_itr_k2);

    return itr(current_id, next_id, current_area_id, next_area_id);
}

//----------------------------------------------------------------------------
//** OSPF6 interface parameter tuning
//----------------------------------------------------------------------------
/**
 * \brief Get the OSPF6 VLAN interface default configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF6 VLAN interface configuration.
 * \return Error code.
 */
mesa_rc frr_ospf6_intf_conf_def(vtss_ifindex_t *const ifindex,
                                vtss_appl_ospf6_intf_conf_t *const conf)
{
    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_clear(*conf);

    // Fill the none zero default values below
    conf->priority = FRR_OSPF6_DEF_PRIORITY;
    conf->is_specific_cost = false;
    conf->cost = VTSS_APPL_OSPF6_INTF_COST_MIN;
    conf->mtu_ignore = false;
    conf->dead_interval = FRR_OSPF6_DEF_DEAD_INTERVAL;              // in seconds
    conf->hello_interval = FRR_OSPF6_DEF_HELLO_INTERVAL;            // in seconds
    conf->retransmit_interval = FRR_OSPF6_DEF_RETRANSMIT_INTERVAL;  // in seconds
    conf->is_passive = false;
    conf->transmit_delay = FRR_OSPF6_DEF_TRANSMIT_DELAY;            // in seconds

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 VLAN interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF6 VLAN interface configuration.
 * \return Error code.
 */
static mesa_rc OSPF6_intf_conf_get(const vtss_ifindex_t ifindex, vtss_appl_ospf6_intf_conf_t *const conf)
{
    vtss_appl_ospf6_intf_conf_t def_conf;

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
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_OSPF6, running_conf));

    // priority
    auto frr_intf_priority = frr_ospf6_if_priority_conf_get(running_conf, ifindex);
    if (frr_intf_priority.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface priority. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_priority.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->priority = frr_intf_priority.val;

    // cost
    auto frr_intf_cost = frr_ospf6_if_cost_conf_get(running_conf, ifindex);
    if (frr_intf_cost.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface cost. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_cost.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->is_specific_cost = (frr_intf_cost.val == 0) ? false : true;
    if (conf->is_specific_cost) {
        conf->cost = frr_intf_cost.val;
    } else {
        conf->cost = VTSS_APPL_OSPF6_INTF_COST_MIN;
    }

    // mtu-ignore
    conf->mtu_ignore = frr_ospf6_if_mtu_ignore_conf_get(running_conf, ifindex).val;

    // dead-interval
    auto frr_intf_dead_interval = frr_ospf6_if_dead_interval_conf_get(running_conf, ifindex);
    if (frr_intf_dead_interval.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface dead interval. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_dead_interval.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    VTSS_RC(frr_ospf6_intf_conf_def((vtss_ifindex_t *const) & ifindex, &def_conf));

    // If 'fast hello' is enabled, dead-interval is assigned 1 sec
    conf->dead_interval = frr_intf_dead_interval.val.multiplier ? 1 : frr_intf_dead_interval.val.val;

    // hello-interval
    auto frr_intf_hello_interval = frr_ospf6_if_hello_interval_conf_get(running_conf, ifindex);
    if (frr_intf_hello_interval.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface hello interval. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_hello_interval.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->hello_interval = frr_intf_hello_interval.val;

    // retransmit-interval
    auto frr_intf_retransmit_interval = frr_ospf6_if_retransmit_interval_conf_get(running_conf, ifindex);
    if (frr_intf_retransmit_interval.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface retransmit interval. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_retransmit_interval.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->retransmit_interval = frr_intf_retransmit_interval.val;

    //passive-interface
    auto frr_intf_passive = frr_ospf6_if_passive_conf_get(running_conf, ifindex);
    if (frr_intf_passive.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface passive config. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_retransmit_interval.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->is_passive = frr_intf_passive.val;

    // transmit-delay
    auto frr_intf_transmit_delay = frr_ospf6_if_transmit_delay_conf_get(running_conf, ifindex);
    if (frr_intf_transmit_delay.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface transmit delay. (ifindex = %s, rc = %s)", ifindex, error_txt(frr_intf_transmit_delay.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    conf->transmit_delay = frr_intf_transmit_delay.val;

    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 VLAN interface configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF6 VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_intf_conf_get(const vtss_ifindex_t ifindex,
                                      vtss_appl_ospf6_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // 'conf' must not be changed if any errors from OSPF6_intf_conf_get().
    vtss_appl_ospf6_intf_conf_t buf;
    auto rc = OSPF6_intf_conf_get(ifindex, &buf);
    if (rc == VTSS_RC_OK) {
        *conf = buf;
    }

    return rc;
}

/**
 * \brief Set the OSPF6 VLAN interface configuration.
 * \param ifindex [IN] The index of VLAN interface.
 * \param conf    [IN] OSPF6 VLAN interface configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_intf_conf_set(const vtss_ifindex_t ifindex,
                                      const vtss_appl_ospf6_intf_conf_t *const conf)
{
    CRIT_SCOPE();

    mesa_rc rc;
    vtss_appl_ospf6_intf_conf_t orig_conf;

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

    if (conf->priority > VTSS_APPL_OSPF6_PRIORITY_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'priority'(" << conf->priority
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->is_specific_cost && (conf->cost > VTSS_APPL_OSPF6_INTF_COST_MAX)) {
        VTSS_TRACE(DEBUG) << "Parameter 'cost'(" << conf->cost
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->hello_interval > VTSS_APPL_OSPF6_HELLO_INTERVAL_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'hello_interval'("
                          << conf->hello_interval << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->dead_interval > VTSS_APPL_OSPF6_DEAD_INTERVAL_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'dead_interval'(" << conf->dead_interval
                          << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->retransmit_interval > VTSS_APPL_OSPF6_RETRANSMIT_INTERVAL_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'retransmit_interval'("
                          << conf->retransmit_interval << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf->transmit_delay > VTSS_APPL_OSPF6_TRANSMIT_DELAY_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'transmit_delay'("
                          << conf->transmit_delay << ") is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }


    /* Check the interface is existing or not */
    if (!vtss_appl_ip_if_exists(ifindex)) {
        VTSS_TRACE(DEBUG) << "Parameter 'ifindex'(" << ifindex << ") does not exist";
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get the original configuration */
    if ((rc = OSPF6_intf_conf_get(ifindex, &orig_conf)) != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface param. "
                          "(rc = "
                          << rc << ")";
        return rc;
    }

    /* Apply to FRR layer when the configuration is changed. */
    // priority
    if (conf->priority != orig_conf.priority) {
        if ((rc = frr_ospf6_if_priority_conf_set(ifindex, conf->priority)) !=
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
            (rc = frr_ospf6_if_cost_conf_set(ifindex, conf->cost)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface cost. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }

        if (!conf->is_specific_cost &&
            (rc = frr_ospf6_if_cost_conf_del(ifindex)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Del interface cost. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }
    }

    // mtu-ignore
    if (conf->mtu_ignore != orig_conf.mtu_ignore) {
        if ((rc = frr_ospf6_if_mtu_ignore_conf_set(ifindex, conf->mtu_ignore)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface mtu-ignore. (rc = " << rc << ")";
            return rc;
        }
    }

    // fast hello packets
    bool apply_new_dead_interval = false;

    // dead interval
    if (apply_new_dead_interval ||
        (conf->dead_interval != orig_conf.dead_interval)) {
        if ((rc = frr_ospf6_if_dead_interval_conf_set(
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
        if ((rc = frr_ospf6_if_hello_interval_conf_set(
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
        if ((rc = frr_ospf6_if_retransmit_interval_conf_set(
                      ifindex, conf->retransmit_interval)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface "
                              "retransmit interval. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }
    }

    // retransmit-interval
    if (conf->is_passive != orig_conf.is_passive) {
        if ((rc = frr_ospf6_if_passive_conf_set(
                      ifindex, conf->is_passive)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface "
                              " passive. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }
    }

    // transmit-delay
    if (conf->transmit_delay != orig_conf.transmit_delay) {
        if ((rc = frr_ospf6_if_transmit_delay_conf_set(
                      ifindex, conf->transmit_delay)) != VTSS_RC_OK) {
            VTSS_TRACE(DEBUG) << "Access framework failed: Set interface "
                              "transmit delay. "
                              "(rc = "
                              << rc << ")";
            return rc;
        }
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through all OSPF6 VLAN interfaces.
 * \param current_ifindex [IN]  Current ifIndex
 * \param next_ifindex    [OUT] Next ifIndex
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_intf_conf_itr(const vtss_ifindex_t *const current_ifindex, vtss_ifindex_t *const next_ifindex)
{
    return OSPF6_if_itr(current_ifindex, next_ifindex);
}

//------------------------------------------------------------------------------
//** OSPF6 interface status
//------------------------------------------------------------------------------
/**
 * \brief Iterator through the interface in the ospf6
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
mesa_rc vtss_appl_ospf6_interface_itr(const vtss_ifindex_t *const prev,
                                      vtss_ifindex_t *const next)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next) {
        VTSS_TRACE(ERROR) << "Parameter 'next' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_interface_status_get();
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
    Map<vtss_ifindex_t, FrrIpOspf6IfStatus>::iterator itr;
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
using NeighborStatusMap = Map<mesa_ipv4_t, Vector<FrrIpOspf6NeighborStatus>>;
using CachedNeighborStatus = FrrCachedResult<NeighborStatusMap>;
CachedNeighborStatus OSPF6_frr_ospf6_nbr_status_cache {
    frr_ip_ospf6_neighbor_status_get
};

using InterfaceStatusMap = Map<vtss_ifindex_t, FrrIpOspf6IfStatus>;
using CachedInterfaceStatus = FrrCachedResult<InterfaceStatusMap>;
CachedInterfaceStatus OSPF6_frr_ospf6_intf_status_cache {
    frr_ip_ospf6_interface_status_get
};

/* OSPF6 interface status: iterate key1 */
static mesa_rc OSPF6_interface_status_itr2_k1(const mesa_ipv6_t *const current_addr,
                                              mesa_ipv6_t *const next_addr)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_addr_valid = false;
    mesa_ipv6_t ifst_addr = {};

    for (const auto &i : OSPF6_frr_ospf6_intf_status_cache()) {
        const auto &ifst = i.second;

        // output the IP addr as 0 for the virtual interface entry
        if (vtss_ifindex_is_frr_vlink(i.first)) {
            ifst_addr = {0};
        } else {
            ifst_addr = ifst.inet6.address;
        }

        if (current_addr && (ifst_addr == *current_addr || ifst_addr < *current_addr)) {
            continue;
        }

        if (next_addr_valid && (ifst_addr == *next_addr || ifst_addr > *next_addr)) {
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

/* OSPF6 interface status: iterate key1 */
static mesa_rc OSPF6_interface_status_itr2_k2(
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx, mesa_ipv6_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_ifidx_valid = false;
    mesa_ipv6_t ifst_addr = {};

    for (const auto &i : OSPF6_frr_ospf6_intf_status_cache()) {
        const auto &ifst = i.second;

        // ignore virtual interface because vitrual link's status is differrent
        // from
        // physical's in standard MIB.
        if (vtss_ifindex_is_frr_vlink(i.first)) {
            continue;
        }

        ifst_addr = ifst.inet6.address;

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
 * \brief Iterator through the interface in the ospf6
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
mesa_rc vtss_appl_ospf6_interface_itr2(const mesa_ipv6_t *const current_addr,
                                       mesa_ipv6_t *const next_addr,
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
    auto &res = OSPF6_frr_ospf6_intf_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res.val.empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<mesa_ipv6_t, vtss_ifindex_t> itr(
        OSPF6_interface_status_itr2_k1, OSPF6_interface_status_itr2_k2);

    auto rc = itr(current_addr, next_addr, current_ifidx, next_ifidx);

    OSPF6_frr_ospf6_intf_status_cache.invalidate();
    return rc;
}

/* Notice that the result of frr_ip_ospf6_interface_status_get() is
 * based on FRR command 'show ipv6 ospf6 interface', this command output
 * only includes those interfaces which OSPF6 is enabled.
 * When the interface status is link-down, only parameter'if_up' is
 * insignificant, the others information need to be obtained by other
 * way.
 */
static mesa_rc OSPF6_interface_link_down_status_get(vtss_ifindex_t ifindex, vtss_appl_ospf6_interface_status_t *status)
{
    vtss_appl_ip_if_key_ipv6_t key = {};

    FRR_CRIT_ASSERT_LOCKED();

    // Given the default initial value
    vtss_clear(*status);

    // Link status
    status->status = false;

    // Interface's IP address
    key.ifindex = ifindex;
    if (vtss_ifindex_is_vlan(ifindex) && (vtss_appl_ip_if_status_ipv6_itr(&key, &key) != VTSS_RC_OK || key.ifindex != ifindex)) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Can't find IPv6 addr for intf %s", ifindex);
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    status->network = key.addr;

    vtss_appl_ospf6_router_intf_conf_t intf_conf_area;
    if (OSPF6_router_intf_conf_get(FRR_OSPF6_DEFAULT_INSTANCE_ID, ifindex, &intf_conf_area) == VTSS_RC_OK) {
        if (intf_conf_area.area_id.is_specific_id == true) {
            status->area_id = intf_conf_area.area_id.id;
        } else {
            return FRR_RC_ENTRY_NOT_FOUND;
        }
    } else {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    // Router ID
    vtss_appl_ospf6_router_status_t router_status;
    if (OSPF6_router_status_get(FRR_OSPF6_DEFAULT_INSTANCE_ID, &router_status) !=
        VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Can't find router status for OSPF6 instance "
                          << FRR_OSPF6_DEFAULT_INSTANCE_ID;
        return FRR_RC_ENTRY_NOT_FOUND;
    }
    status->router_id = router_status.ospf6_router_id;
    // Default interface state
    status->state = VTSS_APPL_OSPF6_INTERFACE_DOWN;

    // TODO. Given a default value in M1. These values should be updated in M2.
    status->cost = 10;

    status->priority = FRR_OSPF6_DEF_PRIORITY;
    status->transmit_delay = FRR_OSPF6_DEF_TRANSMIT_DELAY;

    /* Notice that the timer parameters:
     * hello_time, dead_time and retransmit_time will equal the current
     * configured timer interval setting when the interface state is down. */
    vtss_appl_ospf6_intf_conf_t intf_conf;
    if (!vtss_ifindex_is_frr_vlink(ifindex) &&
        OSPF6_intf_conf_get(ifindex, &intf_conf) == VTSS_RC_OK) {
        status->hello_time = intf_conf.hello_interval;
        status->dead_time = intf_conf.dead_interval;
        status->retransmit_time = intf_conf.retransmit_interval;
        status->is_passive = intf_conf.is_passive;
    } else {
        status->hello_time = FRR_OSPF6_DEF_HELLO_INTERVAL;
        status->dead_time = FRR_OSPF6_DEF_DEAD_INTERVAL;
        status->retransmit_time = FRR_OSPF6_DEF_RETRANSMIT_INTERVAL;
        status->is_passive = false;
    }

    return VTSS_RC_OK;
}

/* Get the DR/BDR neighbor ID from its IP address.
 * The function is designed due to there is no DR/BDR ID information in the
 * FRR command "show ipv6 ospf6 neighbor detail json".
 *
 * Return a none-zero value when the DR/BDR neighbor ID is found, otherwise,
 * return the zero ID(0.0.0.0).
 *
 * Lookup procedures:
 * 1. Check the searching IP address is mine or not (by looking in myself
 *    OSPF6 interface table)
 * 2. When item 1) is failed, lookup the searching IP address in OSPF6
 *    neighbor table.
 */
static mesa_ipv4_t OSPF6_nbr_lookup_id_by_addr(const mesa_ipv6_t ip_addr)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (vtss_ipv6_addr_is_zero(&ip_addr)) {
        return (mesa_ipv4_t)0;
    }

    /* 1. Check the searching IP address is mine or not (by looking in myself
     *    OSPF6 interface table) */
    auto &intf_res = OSPF6_frr_ospf6_intf_status_cache.result();
    if (!intf_res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get interface status. "
                          "(rc = "
                          << intf_res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    for (const auto &itr : intf_res.val) {
        auto &itr_status = itr.second;
        if (itr_status.inet6.address == ip_addr) {
            return itr_status.dr_id;
        }
    }

    /* 2. When item 1) is failed, lookup the searching IP address in OSPF6
     *    neighbor table. */
    auto &res = OSPF6_frr_ospf6_nbr_status_cache.result();
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
vtss_appl_ospf6_router_id_t vtss_appl_ospf6_nbr_lookup_id_by_addr(
    const mesa_ipv6_t ip_addr)
{
    CRIT_SCOPE();

    return OSPF6_nbr_lookup_id_by_addr(ip_addr);
}

/**
 * \brief Get status for a specific interface.
 * \param ifindex   [IN]  ifindex to query (either VLAN or VLINK)
 * \param status    [OUT] Status for 'key'.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_interface_status_get(const vtss_ifindex_t ifindex, vtss_appl_ospf6_interface_status_t *const status)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!status) {
        VTSS_TRACE(ERROR) << "Parameter 'status' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!vtss_ifindex_is_vlan(ifindex) && !vtss_ifindex_is_frr_vlink(ifindex)) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Interface %s is not valid", ifindex);
        return FRR_RC_INVALID_ARGUMENT;
    }
    /* Lookup the default OSPF6 instance if existing or not. */
    if (!OSPF6_instance_id_existing(FRR_OSPF6_DEFAULT_INSTANCE_ID)) {
        T_DG(FRR_TRACE_GRP_OSPF6, "OSPF6 process ID %d does not exist", FRR_OSPF6_DEFAULT_INSTANCE_ID);
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* If it's a VLAN interface, check that it exists */
    if (vtss_ifindex_is_vlan(ifindex) && !vtss_appl_ip_if_exists(ifindex)) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Interface %s does not exist", ifindex);
        return FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_interface_status_get();
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

        /* Notice that the result of frr_ip_ospf6_interface_status_get() is
         * based on FRR command 'show ipv6 ospf6 interface', this command output
         * only includes those interfaces which OSPF6 is enabled.
         * When the interface status is link-down, only parameter'if_up' is
         * insignificant, the others information need to be obtained by other
         * way.
         */
        if (!map_status.if_up) {
            return OSPF6_interface_link_down_status_get(ifindex, status);
        }

        // Below parameters are significant only when the interface is UP
        status->status = map_status.if_up;
        status->area_id = map_status.area;
        status->router_id = map_status.router_id;
        status->cost = map_status.cost;
        status->network = map_status.inet6;
        switch (map_status.state) {
        case ISM_Loopback:
            status->state = VTSS_APPL_OSPF6_INTERFACE_LOOPBACK;
            break;
        case ISM_Waiting:
            status->state = VTSS_APPL_OSPF6_INTERFACE_WAITING;
            break;
        case ISM_PointToPoint:
            status->state = VTSS_APPL_OSPF6_INTERFACE_POINT2POINT;
            break;
        case ISM_DROther:
            status->state = VTSS_APPL_OSPF6_INTERFACE_DR_OTHER;
            break;
        case ISM_Backup:
            status->state = VTSS_APPL_OSPF6_INTERFACE_BDR;
            break;
        case ISM_DR:
            status->state = VTSS_APPL_OSPF6_INTERFACE_DR;
            break;
        case ISM_None:  // Fall through
        case ISM_Down:        // Fall through
        default:
            status->state = VTSS_APPL_OSPF6_INTERFACE_DOWN;
            break;
        };
        status->priority = map_status.priority;

        status->dr_id = map_status.dr_id;
        status->bdr_id = map_status.bdr_id;

        status->hello_time =
            map_status.hello_interval;
        status->dead_time = map_status.timer_dead;
        status->retransmit_time = map_status.timer_retransmit;
        status->transmit_delay = map_status.transmit_delay;
        vtss_appl_ospf6_intf_conf_t intf_conf;
        if (OSPF6_intf_conf_get(ifindex, &intf_conf) == VTSS_RC_OK) {
            status->is_passive = intf_conf.is_passive;
        }

        return VTSS_RC_OK;
    }

    /* Unlike the OSPF6 VLAN interface, the down state of VLINK interfaces
     * always can be found in the result of "show ipv6 ospf6 interface".
     * So, return here when it is not found in the result.
     */
    if (vtss_ifindex_is_frr_vlink(ifindex)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Not found in the result of "show ipv6 ospf6 interface".
     * Notice that the result of frr_ip_ospf6_interface_status_get() is
     * based on FRR command 'show ipv6 ospf6 interface', this command output
     * only includes those interfaces which OSPF6 is enabled.
     * In this case, we need to call OSPF6_interface_link_down_status_get()
     * to check the current interface state again.
     */
    return OSPF6_interface_link_down_status_get(ifindex, status);
}

/**
 * \brief Get status for a specific interface.
 * \param interface [OUT] An container with all neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_interface_status_get_all(vtss::Map<vtss_ifindex_t, vtss_appl_ospf6_interface_status_t> &interface)
{
    vtss_ifindex_t                    ifindex;
    vtss_appl_ospf6_interface_status_t status;

    CRIT_SCOPE();

    /* Lookup the default OSPF6 instance if existing or not. */
    if (!OSPF6_instance_id_existing(FRR_OSPF6_DEFAULT_INSTANCE_ID)) {
        VTSS_TRACE(DEBUG) << "OSPF6 process ID " << FRR_OSPF6_DEFAULT_INSTANCE_ID << " does not exist";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_interface_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_OSPF6, "Access framework failed: Get interface status (ifindex = %s, rc = %s)", ifindex, error_txt(result_list.rc));
        return FRR_RC_INTERNAL_ACCESS;
    }

    // Notice that the result of frr_ip_ospf6_interface_status_get() is based on
    // FRR command 'show ipv6 ospf6 interface'.
    // The output of this command only includes those interfaces on which OSPF6
    // is enabled.
    // In this case, we need to add all IP interfaces in the 'result_list'.
    vtss_appl_ip_if_status_link_t link;
    vtss_appl_ip_if_conf_ipv6_t   conf;
    vtss_appl_ip_if_key_ipv6_t    key = {};
    FrrIpOspf6IfStatus             frr_intf_status;

    while (vtss_appl_ip_if_status_ipv6_itr(&key, &key) == VTSS_RC_OK) {
        if (vtss_appl_ip_if_conf_ipv6_get(key.ifindex, &conf) != VTSS_RC_OK) {
            continue;
        }

        if (vtss_appl_ip_if_status_link_get(key.ifindex, &link) != VTSS_RC_OK) {
            continue;
        }

        if (link.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) {
            // Process down state interfaces only
            continue;
        }

        if (result_list->find(key.ifindex) == result_list->end() && OSPF6_interface_link_down_status_get(key.ifindex, &status) == VTSS_RC_OK) {
            // Not found in the original list. Add the interface with down state
            frr_intf_status.if_up = false;
            result_list->set(key.ifindex, std::move(frr_intf_status));
        }
    }

    for (const auto &itr : result_list.val) {
        auto &map_status = itr.second;
        ifindex = itr.first;

        /* Notice that the result of frr_ip_ospf6_interface_status_get() is
         * based on FRR command 'show ipv6 ospf6 interface', this command output
         * only includes those interfaces which OSPF6 is enabled.
         * When the interface status is link-down, only parameter'if_up' is
         * insignificant, the others information need to be obtained by other
         * way.
         */
        if (!map_status.if_up) {
            if (OSPF6_interface_link_down_status_get(ifindex, &status) != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG) << "Get OSPF6 interface " << ifindex
                                  << " status failed\n";
                // Don't insert the failed case in the Map object.
                continue;
            }
        } else {
            // Below parameters are significant only when the interface is UP
            status.status = map_status.if_up;
            status.area_id = map_status.area;
            status.router_id = map_status.router_id;
            status.cost = map_status.cost;
            status.network = map_status.inet6;
            switch (map_status.state) {
            case ISM_Loopback:
                status.state = VTSS_APPL_OSPF6_INTERFACE_LOOPBACK;
                break;
            case ISM_Waiting:
                status.state = VTSS_APPL_OSPF6_INTERFACE_WAITING;
                break;
            case ISM_PointToPoint:
                status.state = VTSS_APPL_OSPF6_INTERFACE_POINT2POINT;
                break;
            case ISM_DROther:
                status.state = VTSS_APPL_OSPF6_INTERFACE_DR_OTHER;
                break;
            case ISM_Backup:
                status.state = VTSS_APPL_OSPF6_INTERFACE_BDR;
                break;
            case ISM_DR:
                status.state = VTSS_APPL_OSPF6_INTERFACE_DR;
                break;
            case ISM_None:  // Fall through
            case ISM_Down:        // Fall through
            default:
                status.state = VTSS_APPL_OSPF6_INTERFACE_DOWN;
                break;
            };
            status.priority = map_status.priority;

            status.dr_id = map_status.dr_id;
            status.bdr_id = map_status.bdr_id;

            status.hello_time = map_status.hello_interval;
            status.dead_time = map_status.timer_dead;
            status.retransmit_time = map_status.timer_retransmit;
            status.transmit_delay = map_status.transmit_delay;
            vtss_appl_ospf6_intf_conf_t intf_conf;
            if (OSPF6_intf_conf_get(ifindex, &intf_conf) == VTSS_RC_OK) {
                status.is_passive = intf_conf.is_passive;
            }
        }
        /* Insert the entry */
        interface.insert(
            vtss::Pair<vtss_ifindex_t, vtss_appl_ospf6_interface_status_t>(
                ifindex, status));
    }

    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
// ** OSPF6 neighbor status
//------------------------------------------------------------------------------
static mesa_rc OSPF6_neighbor_status_itr_k2(const mesa_ipv6_t *const current_nip,
                                            mesa_ipv6_t *const next_nip,
                                            vtss_appl_ospf6_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nip_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
        const auto &neighbor = i.second;
        for (const auto &nst : neighbor) {
            if (current_nip && (nst.if_address == *current_nip || nst.if_address < *current_nip)) {
                continue;
            }

            if (next_nip_valid && (nst.if_address == *next_nip || nst.if_address > *next_nip)) {
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

static mesa_rc OSPF6_neighbor_status_itr_k3(const vtss_ifindex_t *const current_ifidx,
                                            vtss_ifindex_t *const next_ifidx,
                                            vtss_appl_ospf6_id_t key1,
                                            mesa_ipv6_t key2)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_ifidx_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
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
 * \param current_id    [IN]  Current OSPF6 ID.
 * \param next_id       [OUT] Next OSPF6 ID.
 * \param current_nip   [IN] Pointer to current neighbor IP.
 * \param next_nip      [OUT] Next neighbor IP.
 * \param current_ifidx [IN] Pointer to current neighbor ifindex.
 * \param next_ifidx    [OUT] Next neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_itr(
    const vtss_appl_ospf6_id_t *const current_id,
    vtss_appl_ospf6_id_t *const next_id, const mesa_ipv6_t *const current_nip,
    mesa_ipv6_t *const next_nip, const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_nip || !next_ifidx) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_nip' or "
                          "'next_ifidx' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto &res = OSPF6_frr_ospf6_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res->empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf6_id_t, mesa_ipv6_t, vtss_ifindex_t> itr(
        OSPF6_inst_itr, OSPF6_neighbor_status_itr_k2,
        OSPF6_neighbor_status_itr_k3);

    auto rc = itr(current_id, next_id, current_nip, next_nip, current_ifidx,
                  next_ifidx);

    OSPF6_frr_ospf6_nbr_status_cache.invalidate();
    return rc;
}

static mesa_rc OSPF6_neighbor_status_itr2_k2(
    const vtss_appl_ospf6_router_id_t *const current_nid,
    vtss_appl_ospf6_router_id_t *const next_nid, vtss_appl_ospf6_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nid_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
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

static mesa_rc OSPF6_neighbor_status_itr2_k3(const mesa_ipv6_t *const current_nip,
                                             mesa_ipv6_t *const next_nip,
                                             vtss_appl_ospf6_id_t key1,
                                             vtss_appl_ospf6_router_id_t key2)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nip_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
        const auto &neighbor = i.second;
        if (i.first != key2) {
            continue;
        }

        for (const auto &nst : neighbor) {
            if (current_nip && (nst.if_address == *current_nip || nst.if_address < *current_nip)) {
                continue;
            }

            if (next_nip_valid && (nst.if_address == *next_nip || nst.if_address > *next_nip)) {
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

static mesa_rc OSPF6_neighbor_status_itr2_k4(
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx, vtss_appl_ospf6_id_t key1,
    vtss_appl_ospf6_router_id_t key2, mesa_ipv6_t key3)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_ifidx_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
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
 * \param current_id    [IN]  Current OSPF6 ID.
 * \param next_id       [OUT] Next OSPF6 ID.
 * \param current_nid   [IN]  Pointer to current neighbor id.
 * \param next_nid      [OUT] Next entry neighbor id.
 * \param current_nip   [IN]  Pointer to current neighbor IP.
 * \param next_nip      [OUT] Next entry neighbor IP.
 * \param current_ifdix [IN]  Pointer to current neighbor ifindex.
 * \param next_ifidx    [OUT] Next entry neighbor ifindex.
 *                Provide a null pointer to get the first neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_itr2(
    const vtss_appl_ospf6_id_t *const current_id,
    vtss_appl_ospf6_id_t *const next_id,
    const vtss_appl_ospf6_router_id_t *const current_nid,
    vtss_appl_ospf6_router_id_t *const next_nid,
    const mesa_ipv6_t *const current_nip, mesa_ipv6_t *const next_nip,
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

    if (current_id && *current_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto &res = OSPF6_frr_ospf6_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res->empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf6_id_t, vtss_appl_ospf6_router_id_t,
         mesa_ipv6_t, vtss_ifindex_t>
         itr(OSPF6_inst_itr, OSPF6_neighbor_status_itr2_k2,
             OSPF6_neighbor_status_itr2_k3, OSPF6_neighbor_status_itr2_k4);

    auto rc = itr(current_id, next_id, current_nid, next_nid, current_nip,
                  next_nip, current_ifidx, next_ifidx);

    OSPF6_frr_ospf6_nbr_status_cache.invalidate();
    return rc;
}

static mesa_rc OSPF6_neighbor_status_itr3_k2(
    const vtss_appl_ospf6_area_id_t *const current_transit_area_id,
    vtss_appl_ospf6_area_id_t *const next_transit_area_id,
    vtss_appl_ospf6_id_t key1)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_transit_area_id_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
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

static mesa_rc OSPF6_neighbor_status_itr3_k3(
    const vtss_appl_ospf6_router_id_t *const current_nid,
    vtss_appl_ospf6_router_id_t *const next_nid, vtss_appl_ospf6_id_t key1,
    vtss_appl_ospf6_area_id_t key2)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nid_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
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

static mesa_rc OSPF6_neighbor_status_itr3_k4(const mesa_ipv6_t *const current_nip,
                                             mesa_ipv6_t *const next_nip,
                                             vtss_appl_ospf6_id_t key1,
                                             vtss_appl_ospf6_area_id_t key2,
                                             vtss_appl_ospf6_router_id_t key3)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_nip_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
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

            if (current_nip && (nst.if_address == *current_nip || nst.if_address < *current_nip)) {
                continue;
            }

            if (next_nip_valid && (nst.if_address == *next_nip || nst.if_address > *next_nip)) {
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

static mesa_rc OSPF6_neighbor_status_itr3_k5(
    const vtss_ifindex_t *const current_ifidx,
    vtss_ifindex_t *const next_ifidx, vtss_appl_ospf6_id_t key1,
    vtss_appl_ospf6_area_id_t key2, vtss_appl_ospf6_router_id_t key3,
    mesa_ipv6_t key4)
{
    FRR_CRIT_ASSERT_LOCKED();

    bool next_ifidx_valid = false;

    for (const auto &i : OSPF6_frr_ospf6_nbr_status_cache()) {
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
 *  This function is used by Standard MIB - ospf6VirtNbrTable (key: Transit Area
 * and Router ID)
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
    vtss_ifindex_t *const next_ifidx)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_transit_area_id || !next_nid) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_transit_area_id' or "
                          "'next_nid' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto &res = OSPF6_frr_ospf6_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res.val.empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf6_id_t, vtss_appl_ospf6_area_id_t,
         vtss_appl_ospf6_router_id_t, mesa_ipv6_t, vtss_ifindex_t>
         itr(OSPF6_inst_itr, OSPF6_neighbor_status_itr3_k2,
             OSPF6_neighbor_status_itr3_k3, OSPF6_neighbor_status_itr3_k4,
             OSPF6_neighbor_status_itr3_k5);

    auto rc = itr(current_id, next_id, current_transit_area_id,
                  next_transit_area_id, current_nid, next_nid, current_nip,
                  next_nip, current_ifidx, next_ifidx);

    OSPF6_frr_ospf6_nbr_status_cache.invalidate();
    return rc;
}

/**
 * \brief Get status for a neighbor information.
 * \param id            [IN]  OSPF6 instance ID.
 * \param neighbor_id   [IN]  Neighbor id to query.
 * \param neighbor_ip   [IN]  Neighbor IP to query.
 * \param neighbor_ifidx[IN]  Neighbor ifindex to query.
 * \param status        [OUT] Neighbor status.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_get(
    const vtss_appl_ospf6_id_t id,
    const vtss_appl_ospf6_router_id_t neighbor_id,
    const mesa_ipv6_t neighbor_ip, const vtss_ifindex_t neighbor_ifidx,
    vtss_appl_ospf6_neighbor_status_t *const status)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!status) {
        VTSS_TRACE(ERROR) << "Parameter 'status' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto &res = OSPF6_frr_ospf6_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(neighbor_id = "
                          << neighbor_id << ", rc = " << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    for (const auto &itr : res.val) {
        auto &itr_ips = itr.second;
        if (neighbor_id != VTSS_APPL_OSPF6_DONTCARE_NID &&
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
            status->area_id = itr_status.area;
            status->ifindex = itr_status.ifindex;
            status->priority = itr_status.nbr_priority;
            status->state = (vtss_appl_ospf6_neighbor_state_t)itr_status.nbr_state;

            status->dr_id = itr_status.dr_id;
            status->bdr_id = itr_status.bdr_id;

            status->dead_time = itr_status.router_dead_interval_timer_due.raw32();

            status->transit_id = itr_status.transit_id.area;
            OSPF6_frr_ospf6_nbr_status_cache.invalidate();
            return VTSS_RC_OK;
        }
    }

    OSPF6_frr_ospf6_nbr_status_cache.invalidate();
    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get status for all neighbor information.
 * \param ipv4_routes [OUT] An container with all neighbor.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_neighbor_status_get_all(
    vtss::Vector<vtss_appl_ospf6_neighbor_data_t> &neighbors)
{
    CRIT_SCOPE();

    vtss_appl_ospf6_id_t *current_id = NULL;
    vtss_appl_ospf6_id_t next_id;
    vtss_appl_ospf6_router_id_t *current_nid = NULL;
    vtss_appl_ospf6_router_id_t next_nid;
    mesa_ipv6_t *current_nip = NULL;
    mesa_ipv6_t next_nip;
    vtss_ifindex_t *current_ifidx = NULL;
    vtss_ifindex_t next_ifidx;
    vtss_appl_ospf6_neighbor_data_t data = {};
    BOOL get_first = TRUE;

    /* Get data from FRR layer */
    auto &res = OSPF6_frr_ospf6_nbr_status_cache.update();
    if (!res) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get neighbor status. "
                          "(rc = "
                          << res.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    if (res->empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    vtss::IteratorComposeDependN<vtss_appl_ospf6_id_t, vtss_appl_ospf6_router_id_t,
         mesa_ipv6_t, vtss_ifindex_t>
         itr(OSPF6_inst_itr, OSPF6_neighbor_status_itr2_k2,
             OSPF6_neighbor_status_itr2_k3, OSPF6_neighbor_status_itr2_k4);

    while (itr(current_id, &next_id, current_nid, &next_nid, current_nip,
               &next_nip, current_ifidx, &next_ifidx) == VTSS_RC_OK) {
        data.id = next_id;
        data.neighbor_id = next_nid;
        data.neighbor_ip = next_nip;
        data.neighbor_ifidx = next_ifidx;

        for (const auto &itr : res.val) {
            auto &itr_ips = itr.second;
            if (next_nid != VTSS_APPL_OSPF6_DONTCARE_NID && itr.first < next_nid) {
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
                status->area_id = itr_status.area;
                status->ifindex = itr_status.ifindex;
                status->priority = itr_status.nbr_priority;
                status->state =
                    (vtss_appl_ospf6_neighbor_state_t)itr_status.nbr_state;

                status->dr_id = itr_status.dr_id;
                status->bdr_id = itr_status.bdr_id;

                status->dead_time =
                    itr_status.router_dead_interval_timer_due.raw32();

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
    OSPF6_frr_ospf6_nbr_status_cache.invalidate();
    OSPF6_frr_ospf6_intf_status_cache.invalidate();

    return VTSS_RC_OK;
}

/* This function is the system reset callback to disable OSPF6 router, and
 * this may trigger stub router advertisement. If yes, this callback will wait
 * for the OSPF6 router shutdown complete. The longest waiting time
 * may reach 100 seconds.
 */
void frr_ospf6_pre_shutdown_callback(mesa_restart_t restart)
{
    vtss_appl_ospf6_id_t id = VTSS_APPL_OSPF6_INSTANCE_ID_START;
    uint32_t deferred_shutdown_time = 0;

    /* Disable OSPF6 router, this may start stub router */
    mesa_rc rc = vtss_appl_ospf6_del(id);
    if (rc != VTSS_RC_OK) {
        return;
    }
    /* OSPF6 process maybe still running due to stub router advertisement.
     * Get deferred shutdown time and wait for the OSPF6 process shutdown
     * complete.
     */
    {
        CRIT_SCOPE();
        auto router_status = frr_ip_ospf6_status_get();
        if (router_status.rc != VTSS_RC_OK) {
            return;
        }

        deferred_shutdown_time = router_status->deferred_shutdown_time.raw32();
    }

    if (deferred_shutdown_time > 0) {
        /* Print message on CLI and do syslog */
        (void)icli_session_printf_to_all(
            "\nDeclare OSPF6 router as a stub router.\nDeferred shutdown "
            "OSPF6 in progress, %d(ms) remaining.",
            deferred_shutdown_time);
#ifdef VTSS_SW_OPTION_SYSLOG
        S_I("Declare OSPF6 router as a stub router. Deferred shutdown OSPF6 in "
            "progress, %d(ms) remaining.",
            deferred_shutdown_time);
#endif /* VTSS_SW_OPTION_SYSLOG */

        /* Delay to wait for the shutdown timer expired */
        VTSS_OS_MSLEEP(deferred_shutdown_time);

        /* Print message on CLI and do syslog when OSPF6 router is disabled */
        (void)icli_session_printf_to_all("\nOSPF6 router is disabled.");
#ifdef VTSS_SW_OPTION_SYSLOG
        S_I("OSPF6 router is disabled.");
#endif /* VTSS_SW_OPTION_SYSLOG */
    }
}

//----------------------------------------------------------------------------
//** OSPF6 routing information
//----------------------------------------------------------------------------
/* convert FRR OSPF route entry to vtss_appl_ospf6_route_status_t */
static void OSPF6_frr_ospf6_route_ipv6_status_mapping(
    const FrrOspf6RouteType rt_type,
    const APPL_FrrOspf6RouteStatus *const frr_val,
    vtss_appl_ospf6_route_status_t *const status)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping FrrOspf6RouterType and 'is_ia' to
     *  vtss_appl_ospf6_route_br_type_t */
    switch (frr_val->router_type) {
    case RouterType_ABR:
        status->border_router_type = VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_ABR;
        break;
    case RouterType_ASBR:
        status->border_router_type =
            !frr_val->is_ia
            ? VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_INTRA_AREA_ASBR
            : VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_INTER_AREA_ASBR;
        break;
    case RouterType_ABR_ASBR:
        status->border_router_type =
            VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_ABR_ASBR;
        break;
    case RouterType_None:
        status->border_router_type = VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_NONE;

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
        status->as_cost = VTSS_APPL_OSPF6_GENERAL_COST_MIN;
    }

    status->connected = frr_val->is_connected;
    status->ifindex = frr_val->ifindex;
}

/* Convert the FRR access's key to the APPL's key. */
static void OSPF6_frr_ospf6_route_ipv6_key_mapping(
    const APPL_FrrOspf6RouteKey *const frr_key, vtss_appl_ospf6_id_t *const id,
    vtss_appl_ospf6_route_type_t *const rt_type,
    mesa_ipv6_network_t *const dest, vtss_appl_ospf6_area_id_t *const area,
    mesa_ipv6_t *const nexthop)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert FRR key (" << frr_key->inst_id << ", "
                      << frr_key->route_type << ", " << frr_key->network << ", "
                      << frr_key->area << ", " << frr_key->nexthop_ip << ") to";

    *id = frr_key->inst_id;
    *rt_type = frr_ospf6_route_type_mapping(frr_key->route_type);
    *dest = frr_key->network;
    *area = frr_key->area;
    *nexthop = frr_key->nexthop_ip;
    VTSS_TRACE(DEBUG) << "APPL key(" << *id << ", " << *rt_type << ", " << *dest
                      << ", " << *area << ", " << *nexthop << ")";
}

/* Convert the APPL's key to the FRR access's key. */
static APPL_FrrOspf6RouteKey OSPF6_frr_ospf6_route_ipv6_access_key_mapping(
    const vtss_appl_ospf6_id_t id, const vtss_appl_ospf6_route_type_t rt_type,
    const mesa_ipv6_network_t dest, vtss_appl_ospf6_area_id_t area,
    const mesa_ipv6_t nexthop)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert APPL key(" << id << ", " << rt_type << ", "
                      << dest << ", " << area << ", " << nexthop << ") to";

    APPL_FrrOspf6RouteKey frr_key;
    frr_key.inst_id = id;
    frr_key.route_type = frr_ospf6_access_route_type_mapping(rt_type);
    frr_key.network = dest;
    frr_key.area = area;
    frr_key.nexthop_ip = nexthop;

    VTSS_TRACE(DEBUG) << "FRR key (" << frr_key.inst_id << ", "
                      << frr_key.route_type << ", " << frr_key.network << ", "
                      << frr_key.area << ", " << frr_key.nexthop_ip << ")";
    return frr_key;
}

/**
 * \brief Iterate through the OSPF6 IPv4 route entries.
 * \param current_id        [IN]  The current OSPF6 ID.
 * \param next_id           [OUT] The next OSPF6 ID.
 * \param current_rt_type   [IN]  The current route type.
 * \param next_rt_type      [OUT] The next route type.
 * \param current_dest      [IN]  The current destination.
 * \param next_dest         [OUT] The next destination.
 * \param current_nexthop   [IN]  The current nexthop.
 * \param next_nexthop      [OUT] The next nexthop.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_route_ipv6_status_itr(
    const vtss_appl_ospf6_id_t *const current_id,
    vtss_appl_ospf6_id_t *const next_id,
    const vtss_appl_ospf6_route_type_t *const current_rt_type,
    vtss_appl_ospf6_route_type_t *const next_rt_type,
    const mesa_ipv6_network_t *const current_dest,
    mesa_ipv6_network_t *const next_dest,
    const vtss_appl_ospf6_area_id_t *const current_area,
    vtss_appl_ospf6_area_id_t *const next_area,
    const mesa_ipv6_t *const current_nexthop,
    mesa_ipv6_t *const next_nexthop)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_id || !next_rt_type || !next_dest || !next_area || !next_nexthop) {
        VTSS_TRACE(ERROR) << "Parameter 'next_id' or 'next_rt_type' or "
                          "'next_dest' or 'next_nexthop' cannot be null "
                          "point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_id && *current_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_route_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 routes . "
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
    Map<APPL_FrrOspf6RouteKey, APPL_FrrOspf6RouteStatus>::iterator itr;
    APPL_FrrOspf6RouteKey frr_key({0, RT_Network, {{0}, 0}, 0});
    bool is_greater_or_equal = false;
    if (!current_id || !current_rt_type || !current_dest || !current_area ||
        !current_nexthop) {
        is_greater_or_equal = true;
    }

    if (current_id) {
        if (!current_rt_type) {
            frr_key.inst_id = *current_id;
        } else if (*current_rt_type >= VTSS_APPL_OSPF6_ROUTE_TYPE_UNKNOWN) {
            // 'current_rt_type' is VTSS_APPL_OSPF6_ROUTE_TYPE_UNKNOWN means to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*current_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *current_id + 1;
            is_greater_or_equal = true;
        } else if (current_rt_type && !current_dest) {
            frr_key.inst_id = *current_id;
            frr_key.route_type =
                frr_ospf6_access_route_type_mapping(*current_rt_type);
        } else if (current_rt_type && current_dest && !current_area) {
            frr_key.inst_id = *current_id;
            frr_key.route_type =
                frr_ospf6_access_route_type_mapping(*current_rt_type);
            frr_key.network = *current_dest;
        } else if (current_rt_type && current_dest && current_area &&
                   !current_nexthop) {
            frr_key.inst_id = *current_id;
            frr_key.route_type =
                frr_ospf6_access_route_type_mapping(*current_rt_type);
            frr_key.network = *current_dest;
            frr_key.area = *current_area;
        } else {
            frr_key.inst_id = *current_id;
            frr_key.route_type =
                frr_ospf6_access_route_type_mapping(*current_rt_type);
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
                      << (frr_key.nexthop_ip) << ")";
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_route_ipv6_key_mapping(&itr->first, next_id, next_rt_type,
                                           next_dest, next_area, next_nexthop);

    return VTSS_RC_OK;
}

/**
 * \brief Get the specific OSPF6 IPv4 route entry.
 * \param id      [IN]  The OSPF6 instance ID.
 * \param rt_type [IN]  The route type.
 * \param dest    [IN]  The destination.
 * \param nexthop [IN]  The nexthop.
 * \param status  [OUT] The OSPF6 route status.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_route_ipv6_status_get(
    const vtss_appl_ospf6_id_t id, const vtss_appl_ospf6_route_type_t rt_type,
    const mesa_ipv6_network_t dest, const vtss_appl_ospf6_area_id_t area,
    const mesa_ipv6_t nexthop, vtss_appl_ospf6_route_status_t *const status)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!status) {
        VTSS_TRACE(ERROR) << "Parameter 'status' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_route_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 routes . "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* Return FRR_RC_ENTRY_NOT_FOUND if no entries found. */
    if (result_list.val.empty()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Invoke MAP.find() to get the entry. */
    auto frr_key = OSPF6_frr_ospf6_route_ipv6_access_key_mapping(
                       id, rt_type, dest, area, nexthop);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_route_ipv6_status_mapping(itr->first.route_type, &itr->second,
                                              status);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF6 IPv4 route entries.
 * \param routes [OUT] The container with all routes.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_route_ipv6_status_get_all(
    vtss::Vector<vtss_appl_ospf6_route_ipv6_data_t> &routes)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_route_status_get();
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 routes . "
                          "( rc = "
                          << result_list.rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    /* iterate all FRR entries */
    for (auto &itr : result_list.val) {
        vtss_appl_ospf6_route_ipv6_data_t entry;

        /* convert the FRR data to vtss_appl_ospf6_route_ipv6_data_t. */
        OSPF6_frr_ospf6_route_ipv6_key_mapping(&itr.first, &entry.id,
                                               &entry.rt_type, &entry.dest,
                                               &entry.area, &entry.nexthop);
        OSPF6_frr_ospf6_route_ipv6_status_mapping(itr.first.route_type,
                                                  &itr.second, &entry.status);

        /* save the in 'routes' */
        routes.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database information
//----------------------------------------------------------------------------
/* convert FRR OSPF db entry to vtss_appl_ospf6_db_general_info_t */
static void OSPF6_frr_ospf6_db_info_mapping(
    const APPL_FrrOspf6DbLinkStateVal *const frr_val,
    vtss_appl_ospf6_db_general_info_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspf6DbLinkStateVal to
     *  vtss_appl_ospf6_db_general_info_t */
    info->age = frr_val->age;
    info->sequence = frr_val->sequence;
    info->router_link_count = frr_val->router_link_count;
}

/* Convert the FRR access's key to the APPL's key. */
static void OSPF6_frr_ospf6_db_key_mapping(const APPL_FrrOspf6DbKey *const frr_key,
                                           vtss_appl_ospf6_id_t *const id,
                                           mesa_ipv4_t *const area_id,
                                           vtss_appl_ospf6_lsdb_type_t *const type,
                                           mesa_ipv4_t *const link_id,
                                           mesa_ipv4_t *const adv_router)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert FRR key (" << frr_key->inst_id << ", "
                      << frr_key->link_id << ", " << frr_key->adv_router
                      << ") to";

    *id = frr_key->inst_id;
    *type = frr_ospf6_db_type_mapping(frr_key->type);
    *link_id = frr_key->link_id;
    *adv_router = frr_key->adv_router;
    *area_id = frr_key->area_id;

    VTSS_TRACE(DEBUG) << "APPL key(" << *id << ", " << *type
                      << ", " << *link_id << ", " << *adv_router << ")";
}

/* Convert the APPL's key to the FRR access's key. */
static APPL_FrrOspf6DbKey OSPF6_frr_ospf6_db_access_key_mapping(
    const vtss_appl_ospf6_id_t id, const mesa_ipv4_t area_id,
    const vtss_appl_ospf6_lsdb_type_t type, const mesa_ipv4_t link_id,
    const mesa_ipv4_t adv_router)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert APPL key(" << id << ", "
                      << type << ", " << link_id << ", " << adv_router
                      << ") to";

    APPL_FrrOspf6DbKey frr_key;

    frr_key.inst_id = id;
    frr_key.type = frr_ospf6_access_db_type_mapping(type);
    frr_key.link_id = link_id;
    frr_key.adv_router = adv_router;
    frr_key.area_id = area_id;

    VTSS_TRACE(DEBUG) << "FRR key (" << frr_key.inst_id << ", "
                      << frr_key.type << ", "
                      << frr_key.link_id << ", " << frr_key.adv_router << ")";

    return frr_key;
}

/**
 * \brief Iterate through the OSPF6 database information.
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
mesa_rc vtss_appl_ospf6_db_itr(const vtss_appl_ospf6_id_t *const cur_inst_id,
                               vtss_appl_ospf6_id_t *const next_inst_id,
                               const mesa_ipv4_t *const cur_area_id,
                               mesa_ipv4_t *const next_area_id,
                               const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
                               vtss_appl_ospf6_lsdb_type_t *const next_lsdb_type,
                               const mesa_ipv4_t *const cur_link_state_id,
                               mesa_ipv4_t *const next_link_state_id,
                               const vtss_appl_ospf6_router_id_t *const cur_router_id,
                               vtss_appl_ospf6_router_id_t *const next_router_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters. */
    if (!next_inst_id || !next_lsdb_type ||
        !next_link_state_id || !next_router_id || !next_area_id) {
        VTSS_TRACE(ERROR) << "Parameter 'next_inst_id' or 'next_area_id' or "
                          "'next_lsdb_type' or 'next_link_state_id' or "
                          "'next_router_id' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 db. "
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
    Map<APPL_FrrOspf6DbKey, APPL_FrrOspf6DbLinkStateVal>::iterator itr;
    APPL_FrrOspf6DbKey frr_key({0, 0, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id || !cur_area_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF6_LSDB_TYPE_UNKNOWN) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF6_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.area_id = *cur_area_id;
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
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

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id
                      << ", " << frr_key.type << ", " << AsIpv4(frr_key.link_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_db_key_mapping(&itr->first, next_inst_id, next_area_id,
                                   next_lsdb_type, next_link_state_id,
                                   next_router_id);
    return VTSS_RC_OK;
}

/**
 * \brief Get the specific OSPF6 database general summary information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_general_info [OUT] The OSPF6 database general summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_get(
    const vtss_appl_ospf6_id_t inst_id, const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_general_info_t *const db_general_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_general_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_general_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_info_mapping(&itr->second, db_general_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF6 database entries of general information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_get_all(
    vtss::Vector<vtss_appl_ospf6_db_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 db. "
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
        vtss_appl_ospf6_db_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf6_db_entry_t. */
        entry.inst_id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
        OSPF6_frr_ospf6_db_key_mapping(&itr.first, &entry.inst_id, &entry.area_id,
                                       &entry.lsdb_type, &entry.link_state_id,
                                       &entry.adv_router_id);

        OSPF6_frr_ospf6_db_info_mapping(&itr.second, &entry.db);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database detail router information
//----------------------------------------------------------------------------

uint8_t OSPF6_parse_options(const std::string &options)
{
    std::string str_dc("DC");
    std::string str_e("E");
    std::string str_r("R");
    std::string str_n("N");
    std::string str_mc("MC");
    std::string str_v6("V6");

    uint8_t final_option = 0;

    if (options.find(str_dc) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF6_OPTION_FIELD_DC;
    }

    if (options.find(str_e) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF6_OPTION_FIELD_E;
    }

    if (options.find(str_r) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF6_OPTION_FIELD_R;
    }

    if (options.find(str_mc) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF6_OPTION_FIELD_MC;
    }

    if (options.find(str_v6) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF6_OPTION_FIELD_V6;
    }

    if (options.find(str_n) != std::string::npos) {
        final_option |= VTSS_APPL_OSPF6_OPTION_FIELD_N;
    }

    return final_option;
}

/* Convert the FRR access's key to the APPL's key. */
static void OSPF6_frr_ospf6_db_common_key_mapping(
    const APPL_FrrOspf6DbCommonKey *const frr_key,
    vtss_appl_ospf6_id_t *const id, mesa_ipv4_t *const area_id,
    vtss_appl_ospf6_lsdb_type_t *const type, mesa_ipv4_t *const link_id,
    mesa_ipv4_t *const adv_router)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert FRR key (" << frr_key->inst_id << ", "
                      << frr_key->type << ", "
                      << frr_key->link_state_id << ", " << frr_key->adv_router
                      << ") to";

    *id = frr_key->inst_id;
    *area_id = frr_key->area_id;
    *type = frr_ospf6_db_type_mapping(frr_key->type);
    *link_id = frr_key->link_state_id;
    *adv_router = frr_key->adv_router;

    VTSS_TRACE(DEBUG) << "APPL key(" << *id << ", " << *type
                      << ", " << *link_id << ", " << *adv_router << ")";
}

/* Convert the APPL's key to the FRR access's key. */
static APPL_FrrOspf6DbCommonKey OSPF6_frr_ospf6_db_common_access_key_mapping(
    const vtss_appl_ospf6_id_t id,
    const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_lsdb_type_t type, const mesa_ipv4_t link_id,
    const mesa_ipv4_t adv_router)
{
    FRR_CRIT_ASSERT_LOCKED();

    VTSS_TRACE(DEBUG) << "convert APPL key(" << id << ", "
                      << type << ", " << link_id << ", " << adv_router
                      << ") to";

    APPL_FrrOspf6DbCommonKey frr_key;

    frr_key.inst_id = id;
    frr_key.area_id = area_id;
    frr_key.type = frr_ospf6_access_lsdb_type_mapping(type);
    frr_key.link_state_id = link_id;
    frr_key.adv_router = adv_router;

    VTSS_TRACE(DEBUG) << "FRR key (" << frr_key.inst_id
                      << ", " << frr_key.type << ", " << frr_key.link_state_id
                      << ", " << frr_key.adv_router << ")";

    return frr_key;
}

/* convert FRR OSPF db entry to vtss_appl_ospf6_db_router_data_entry_t */
static void OSPF6_frr_ospf6_db_router_info_mapping(
    const APPL_FrrOspf6DbRouterStateVal *const frr_val,
    vtss_appl_ospf6_db_router_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspf6DbRouterStateVal to
     *  vtss_appl_ospf6_db_router_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF6_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->router_link_count = frr_val->links.size();
}

/* convert FRR OSPF db entry into vtss_appl_ospf6_db_router_link_info_t */
static void OSPF6_frr_ospf6_db_router_link_info_mapping(
    const APPL_FrrOspf6DbRouterStateVal *const frr_val, const uint32_t index,
    vtss_appl_ospf6_db_router_link_info_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (index < frr_val->links.size()) {
        info->link_connected_to = frr_val->links[index].link_connected_to;
        info->link_data = frr_val->links[index].link_data;
        info->link_id = frr_val->links[index].link_id;
        info->metric = frr_val->links[index].metric;
    } else {
        info->link_connected_to = 0;
        info->link_data = {0};
        info->link_id = 0;
        info->metric = 0;
    }
}

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
    const vtss_appl_ospf6_id_t *const cur_inst_id,
    vtss_appl_ospf6_id_t *const next_inst_id,
    const vtss_appl_ospf6_area_id_t *const cur_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t *const next_router_id)
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

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_router_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 router db. "
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
    Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbRouterStateVal>::iterator itr;
    APPL_FrrOspf6DbCommonKey frr_key({0, 0, vtss::FrrOspf6LsdbType_None, 0, 0});
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
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF6_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
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

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                          next_lsdb_type, next_link_state_id,
                                          next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf6_id_t inst_id, const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_router_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_router_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 router db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_router_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf6_id_t inst_id, const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id, const uint32_t index,
    vtss_appl_ospf6_db_router_link_info_t *const link_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!link_info) {
        VTSS_TRACE(ERROR) << "Parameter 'link_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_router_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 router db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_router_link_info_mapping(&itr->second, index, link_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF6 database entries of detail router information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_router_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_router_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_router_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 router db. "
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
        vtss_appl_ospf6_db_detail_router_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf6_db_detail_router_entry_t. */
        entry.inst_id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
        OSPF6_frr_ospf6_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF6_frr_ospf6_db_router_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database detail link information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf6_db_link_data_entry_t */
static void OSPF6_frr_ospf6_db_link_info_mapping(
    const APPL_FrrOspf6DbLinkLinkStateVal *const frr_val,
    vtss_appl_ospf6_db_link_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspf6DbLinkLinkStateVal to
     *  vtss_appl_ospf6_db_link_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF6_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->prefix_cnt = frr_val->links.size();
}

/* convert FRR OSPF db entry into vtss_appl_ospf6_db_link_info_t */
static void OSPF6_frr_ospf6_db_link_link_info_mapping(
    const APPL_FrrOspf6DbLinkLinkStateVal *const frr_val, const uint32_t index,
    vtss_appl_ospf6_db_link_info_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (index < frr_val->links.size()) {
        info->link_prefix = frr_val->links[index].prefix;
        info->link_prefix_len = frr_val->links[index].prefix_length;
        info->prefix_options = frr_val->links[index].prefix_options;
    } else {
        info->link_prefix = {0};
        info->link_prefix_len = 0;
        info->prefix_options = 0;
    }
}

/**
 * \brief Get the OSPF6 database detail link information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_link_get(
    const vtss_appl_ospf6_id_t inst_id, const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_link_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_link_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 router db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_link_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf6_id_t inst_id, const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id, const uint32_t index,
    vtss_appl_ospf6_db_link_info_t *const link_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!link_info) {
        VTSS_TRACE(ERROR) << "Parameter 'link_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_link_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 router db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_link_link_info_mapping(&itr->second, index, link_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF6 database entries of detail link information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_link_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_link_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_link_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 link db. "
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
        vtss_appl_ospf6_db_detail_link_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf6_db_detail_link_entry_t. */
        entry.inst_id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
        OSPF6_frr_ospf6_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF6_frr_ospf6_db_link_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf6_id_t *const cur_inst_id,
    vtss_appl_ospf6_id_t *const next_inst_id,
    const vtss_appl_ospf6_area_id_t *const cur_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t *const next_router_id)
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

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_link_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 router db. "
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
    Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbLinkLinkStateVal>::iterator itr;
    APPL_FrrOspf6DbCommonKey frr_key({0, 0, vtss::FrrOspf6LsdbType_None, 0, 0});
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
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF6_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
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

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                          next_lsdb_type, next_link_state_id,
                                          next_router_id);
    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database detail intra area prefix information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf6_db_link_data_entry_t */
static void OSPF6_frr_ospf6_db_intra_area_prefix_info_mapping(
    const APPL_FrrOspf6DbIntraPrefixStateVal *const frr_val,
    vtss_appl_ospf6_db_intra_area_prefix_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspf6DbIntraPrefixStateVal to
     *  vtss_appl_ospf6_db_intra_area_prefix_data_entry_t */
    info->age = frr_val->age;
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->prefix_cnt = frr_val->links.size();
}

/* convert FRR OSPF db entry into vtss_appl_ospf6_db_link_info_t */
static void OSPF6_frr_ospf6_db_intra_area_prefix_link_info_mapping(
    const APPL_FrrOspf6DbIntraPrefixStateVal *const frr_val, const uint32_t index,
    vtss_appl_ospf6_db_link_info_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    if (index < frr_val->links.size()) {
        info->link_prefix = frr_val->links[index].prefix;
        info->link_prefix_len = frr_val->links[index].prefix_length;
        info->prefix_options = frr_val->links[index].prefix_options;
    } else {
        info->link_prefix = {0};
        info->link_prefix_len = 0;
        info->prefix_options = 0;
    }
}

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
    const vtss_appl_ospf6_id_t *const cur_inst_id,
    vtss_appl_ospf6_id_t *const next_inst_id,
    const vtss_appl_ospf6_area_id_t *const cur_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t *const next_router_id)
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

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_intra_area_prefix_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 network db. "
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
    Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbIntraPrefixStateVal>::iterator itr;
    APPL_FrrOspf6DbCommonKey frr_key({0, 0, vtss::FrrOspf6LsdbType_None, 0, 0});
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
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF6_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
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

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                          next_lsdb_type, next_link_state_id,
                                          next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf6_id_t inst_id, const vtss_appl_ospf6_area_id_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id, const uint32_t index,
    vtss_appl_ospf6_db_link_info_t *const link_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!link_info) {
        VTSS_TRACE(ERROR) << "Parameter 'link_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_intra_area_prefix_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 router db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_intra_area_prefix_link_info_mapping(&itr->second, index, link_info);

    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf6_id_t inst_id,
    const mesa_ipv4_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_intra_area_prefix_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_intra_area_prefix_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 network db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_intra_area_prefix_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}


/**
 * \brief Get all OSPF6 database entries of detail intra area prefix information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_intra_area_prefix_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_intra_area_prefix_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_intra_area_prefix_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 link db. "
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
        vtss_appl_ospf6_db_detail_intra_area_prefix_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf6_db_detail_link_entry_t. */
        entry.inst_id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
        OSPF6_frr_ospf6_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF6_frr_ospf6_db_intra_area_prefix_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database detail network information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf6_db_network_data_entry_t */
static void OSPF6_frr_ospf6_db_network_info_mapping(
    const APPL_FrrOspf6DbNetStateVal *const frr_val,
    vtss_appl_ospf6_db_network_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspf6DbNetStateVal to
     *  vtss_appl_ospf6_db_network_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF6_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->attached_router_count = frr_val->attached_router.size();
}

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
    const vtss_appl_ospf6_id_t *const cur_inst_id,
    vtss_appl_ospf6_id_t *const next_inst_id,
    const vtss_appl_ospf6_area_id_t *const cur_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t *const next_router_id)
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

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_net_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 network db. "
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
    Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbNetStateVal>::iterator itr;
    APPL_FrrOspf6DbCommonKey frr_key({0, 0, vtss::FrrOspf6LsdbType_None, 0, 0});
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
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF6_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
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

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                          next_lsdb_type, next_link_state_id,
                                          next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf6_id_t inst_id,
    const mesa_ipv4_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_network_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_net_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 network db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_network_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF6 database entries of detail network information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_network_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_network_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_net_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 network db. "
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
        vtss_appl_ospf6_db_detail_network_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf6_db_detail_network_entry_t. */
        entry.inst_id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
        OSPF6_frr_ospf6_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF6_frr_ospf6_db_network_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database detail summary information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf6_db_inter_area_prefix_data_entry_t */
static void OSPF6_frr_ospf6_db_inter_area_prefix_info_mapping(
    const APPL_FrrOspf6DbInterAreaPrefixStateVal *const frr_val,
    vtss_appl_ospf6_db_inter_area_prefix_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspf6DbInterAreaPrefixStateVal to
     *  vtss_appl_ospf6_db_inter_area_prefix_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF6_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->metric = frr_val->metric;
    info->prefix = frr_val->prefix;
}

/**
 * \brief Iterate through the OSPF6 database detail summary information.
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
    const vtss_appl_ospf6_id_t *const cur_inst_id,
    vtss_appl_ospf6_id_t *const next_inst_id,
    const vtss_appl_ospf6_area_id_t *const cur_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t *const next_router_id)
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

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_inter_area_prefix_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 summary db. "
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
    Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaPrefixStateVal>::iterator itr;
    APPL_FrrOspf6DbCommonKey frr_key({0, 0, vtss::FrrOspf6LsdbType_None, 0, 0});
    bool is_greater_or_equal = false;
    if (!cur_inst_id || !cur_lsdb_type || !cur_link_state_id ||
        !cur_router_id) {
        is_greater_or_equal = true;
    }

    if (cur_inst_id) {
        if (!cur_area_id) {
            frr_key.inst_id = *cur_inst_id;
        } else if (cur_area_id && !cur_lsdb_type) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF6_LSDB_TYPE_NSSA_EXTERNAL means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
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

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                          next_lsdb_type, next_link_state_id,
                                          next_router_id);
    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 database detail summary information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_prefix_get(
    const vtss_appl_ospf6_id_t inst_id,
    const mesa_ipv4_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_inter_area_prefix_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_inter_area_prefix_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 summary db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_inter_area_prefix_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF6 database entries of detail summary information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_prefix_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_inter_area_prefix_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_inter_area_prefix_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 summary db. "
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
        vtss_appl_ospf6_db_detail_inter_area_prefix_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf6_db_detail_inter_area_prefix_entry_t. */
        entry.inst_id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
        OSPF6_frr_ospf6_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF6_frr_ospf6_db_inter_area_prefix_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database detail inter_area router lsa information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf6_db_inter_area_router_data_entry_t */
static void OSPF6_frr_ospf6_db_inter_area_router_info_mapping(
    const APPL_FrrOspf6DbInterAreaRouterStateVal *const frr_val,
    vtss_appl_ospf6_db_inter_area_router_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspf6DbInterAreaRouterStateVal to
     *  vtss_appl_ospf6_db_inter_area_router_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF6_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->destination_router_id = frr_val->destination;
    info->metric = frr_val->metric;
}

/**
 * \brief Iterate through the OSPF6 database detail inter-area-router information.
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
    const vtss_appl_ospf6_id_t *const cur_inst_id,
    vtss_appl_ospf6_id_t *const next_inst_id,
    const vtss_appl_ospf6_area_id_t *const cur_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t *const next_router_id)
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

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf6_db_inter_area_router_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF6 inter_area router lsa db. "
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
    Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbInterAreaRouterStateVal>::iterator itr;
    APPL_FrrOspf6DbCommonKey frr_key({0, 0, vtss::FrrOspf6LsdbType_None, 0, 0});
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
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.area_id = *cur_area_id;
            frr_key.inst_id = *cur_inst_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
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

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                          next_lsdb_type, next_link_state_id,
                                          next_router_id);
    return VTSS_RC_OK;
}

/**
 * \brief Get the OSPF6 database detail inter-area-router information.
 * \param inst_id         [IN]  The OSPF6 instance ID.
 * \param area_id         [IN]  The area ID.
 * \param lsdb_type       [IN]  The LS database type.
 * \param link_state_id   [IN]  The link state ID.
 * \param adv_router_id   [IN]  The advertising router ID.
 * \param db_detail_info  [OUT] The OSPF6 database detail summary information.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_router_get(
    const vtss_appl_ospf6_id_t inst_id,
    const mesa_ipv4_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_inter_area_router_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf6_db_inter_area_router_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF6 inter_area router lsa db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_inter_area_router_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF6 database entries of detail inter-area-router information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_inter_area_router_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_inter_area_router_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list =
        frr_ip_ospf6_db_inter_area_router_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Get OSPF6 inter_area router lsa db. "
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
        vtss_appl_ospf6_db_detail_inter_area_router_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf6_db_detail_inter_area_router_entry_t. */
        entry.inst_id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
        OSPF6_frr_ospf6_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF6_frr_ospf6_db_inter_area_router_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database detail external information
//----------------------------------------------------------------------------

/* convert FRR OSPF db entry to vtss_appl_ospf6_db_external_data_entry_t */
static void OSPF6_frr_ospf6_db_external_info_mapping(
    const APPL_FrrOspf6DbExternalStateVal *const frr_val,
    vtss_appl_ospf6_db_external_data_entry_t *const info)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Mapping APPL_FrrOspf6DbExternalStateVal to
     *  vtss_appl_ospf6_db_external_data_entry_t */
    info->age = frr_val->age;
    info->options = OSPF6_parse_options(frr_val->options);
    info->sequence = frr_val->sequence;
    info->checksum = frr_val->checksum;
    info->length = frr_val->length;
    info->metric = frr_val->metric;
    info->metric_type = frr_val->metric_type;
    info->forward_address = frr_val->forward_address;
    info->prefix = frr_val->prefix;
}

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
    const vtss_appl_ospf6_id_t *const cur_inst_id,
    vtss_appl_ospf6_id_t *const next_inst_id,
    const vtss_appl_ospf6_area_id_t *const cur_area_id,
    vtss_appl_ospf6_area_id_t *const next_area_id,
    const vtss_appl_ospf6_lsdb_type_t *const cur_lsdb_type,
    vtss_appl_ospf6_lsdb_type_t *const next_lsdb_type,
    const mesa_ipv4_t *const cur_link_state_id,
    mesa_ipv4_t *const next_link_state_id,
    const vtss_appl_ospf6_router_id_t *const cur_router_id,
    vtss_appl_ospf6_router_id_t *const next_router_id)
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

    if (cur_inst_id && *cur_inst_id > VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_external_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 external db. "
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
    Map<APPL_FrrOspf6DbCommonKey, APPL_FrrOspf6DbExternalStateVal>::iterator itr;
    APPL_FrrOspf6DbCommonKey frr_key({0, 0, vtss::FrrOspf6LsdbType_None, 0, 0});
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
        } else if (cur_lsdb_type && *cur_lsdb_type >= VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX) {
            // 'cur_lsdb_type' is VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX means
            // to
            // find the next instance ID. 'FRR_RC_ENTRY_NOT_FOUND'
            // will be returned directly if 'current_id' is the maximum instand
            // ID.
            if (*cur_inst_id == VTSS_APPL_OSPF6_INSTANCE_ID_MAX) {
                return FRR_RC_ENTRY_NOT_FOUND;
            }

            frr_key.inst_id = *cur_inst_id + 1;
            is_greater_or_equal = true;
        } else if (cur_area_id && cur_lsdb_type && !cur_link_state_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
        } else if (cur_area_id && cur_lsdb_type && cur_link_state_id &&
                   !cur_router_id) {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
            frr_key.link_state_id = *cur_link_state_id;
        } else {
            frr_key.inst_id = *cur_inst_id;
            frr_key.area_id = *cur_area_id;
            frr_key.type = frr_ospf6_access_lsdb_type_mapping(*cur_lsdb_type);
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

    VTSS_TRACE(DEBUG) << "(" << frr_key.inst_id << ", "
                      << frr_key.type << ", " << AsIpv4(frr_key.link_state_id)
                      << ", " << AsIpv4(frr_key.adv_router) << ")";

    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Convert the map output to APPL if the next entry is found. */
    OSPF6_frr_ospf6_db_common_key_mapping(&itr->first, next_inst_id, next_area_id,
                                          next_lsdb_type, next_link_state_id,
                                          next_router_id);
    return VTSS_RC_OK;
}

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
    const vtss_appl_ospf6_id_t inst_id,
    const mesa_ipv4_t area_id,
    const vtss_appl_ospf6_lsdb_type_t lsdb_type,
    const mesa_ipv4_t link_state_id,
    const vtss_appl_ospf6_router_id_t adv_router_id,
    vtss_appl_ospf6_db_external_data_entry_t *const db_detail_info)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!db_detail_info) {
        VTSS_TRACE(ERROR) << "Parameter 'db_detail_info' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if the instance ID exists or not. */
    if (!OSPF6_instance_id_existing(inst_id)) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_external_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 external db. "
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
    auto frr_key = OSPF6_frr_ospf6_db_common_access_key_mapping(
                       inst_id, area_id, lsdb_type, link_state_id, adv_router_id);
    auto itr = result_list.val.find(frr_key);
    if (itr == result_list.val.end()) {
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* convert the FRR data to APPL layer. */
    OSPF6_frr_ospf6_db_external_info_mapping(&itr->second, db_detail_info);

    return VTSS_RC_OK;
}

/**
 * \brief Get all OSPF6 database entries of detail external information.
 * \param db_entries [OUT] The container with all database entries.
 * \return Error code.
  */
mesa_rc vtss_appl_ospf6_db_detail_external_get_all(
    vtss::Vector<vtss_appl_ospf6_db_detail_external_entry_t> &db_entries)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    auto result_list = frr_ip_ospf6_db_external_get(FRR_OSPF6_DEFAULT_INSTANCE_ID);
    if (result_list.rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Get OSPF6 external db. "
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
        vtss_appl_ospf6_db_detail_external_entry_t entry;

        /* convert the FRR data to vtss_appl_ospf6_db_detail_external_entry_t. */
        entry.inst_id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
        OSPF6_frr_ospf6_db_common_key_mapping(
            &itr.first, &entry.inst_id, &entry.area_id, &entry.lsdb_type,
            &entry.link_state_id, &entry.adv_router_id);

        OSPF6_frr_ospf6_db_external_info_mapping(&itr.second, &entry.data);

        /* save the database entry in 'db_entries' */
        db_entries.emplace_back(entry);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
/** Module error text (convert the return code to error text)                 */
/******************************************************************************/
const char *frr_ospf6_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_FRR_OSPF6_ERROR_INVALID_ROUTER_ID:
        return "The OSPF6 router ID is invalid";

    case VTSS_APPL_FRR_OSPF6_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT:
        return "The router ID change will take effect after restart OSPF6 "
               "process";

    case VTSS_APPL_FRR_OSPF6_ERROR_AREA_ID_CHANGE_NOT_TAKE_EFFECT:
        return "The OSPF6 area ID change doesn't take effect";

    case FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST:
        return "The VLAN interface does not exist";

    case VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE:
        return "Backbone can not be configured as stub area";

    case VTSS_APPL_FRR_OSPF6_ERROR_AREA_RANGE_COST_CONFLICT:
        return "Area range not-advertise and cost can not be set at the same "
               "time";
    case VTSS_APPL_FRR_OSPF6_ERROR_AREA_RANGE_NETWORK_DEFAULT:
        return "Area range network address cannot represent default";
    }

    return "FRR OSPF6: Unknown error code";
}

/******************************************************************************/
/** Module initialization                                                     */
/******************************************************************************/
#if defined(VTSS_SW_OPTION_JSON_RPC)
VTSS_PRE_DECLS void frr_ospf6_json_init(void);
#endif /* VTSS_SW_OPTION_JSON_RPC */

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
/* Initialize private mib */
VTSS_PRE_DECLS void frr_ospf6_mib_init(void);
#endif

extern "C" int frr_ospf6_icli_cmd_register();

/* Initialize module */
mesa_rc frr_ospf6_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT: {
        VTSS_TRACE(INFO) << "INIT";
        /* Initialize and register OSPF6 mutex */
        critd_init(&FRR_ospf6_crit, "frr_ospf6.crit", VTSS_MODULE_ID_FRR_OSPF6, CRITD_TYPE_MUTEX);

#if defined(VTSS_SW_OPTION_ICFG)
        /* Initialize and register ICFG resources */
        if (frr_has_ospf6d()) {
            frr_ospf6_icfg_init();
        }
#endif /* VTSS_SW_OPTION_ICFG */

#if defined(VTSS_SW_OPTION_JSON_RPC)
        /* Initialize and register JSON resources */
        if (frr_has_ospf6d()) {
            frr_ospf6_json_init();
        }
#endif /* VTSS_SW_OPTION_JSON_RPC */

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        /* Initialize and register private MIB resources */
        if (frr_has_ospf6d()) {
            frr_ospf6_mib_init();
        }
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */

        /* Initialize and register ICLI resources */
        if (frr_has_ospf6d()) {
            frr_ospf6_icli_cmd_register();
        }

        /* Initialize local resources */
        ospf6_enabled_instances.clear();

        VTSS_TRACE(INFO) << "INIT - completed";
        break;
    }

    case INIT_CMD_START: {
        VTSS_TRACE(INFO) << "START";
        /* Register system reset callback -
         * If the callback triggers stub router advertisement, the
         * maximum waiting time is 100 seconds
         */
        control_system_reset_register(frr_ospf6_pre_shutdown_callback, VTSS_MODULE_ID_FRR_OSPF6);
        VTSS_TRACE(INFO) << "START - completed";
        break;
    }

    case INIT_CMD_CONF_DEF: {
        VTSS_TRACE(INFO) << "CONF_DEF, isid: " << isid;
        /* Disable all OSPF6 routing processes */
        CRIT_SCOPE();
        if (frr_has_ospf6d()) {
            OSPF6_process_disabled();
        }

        VTSS_TRACE(INFO) << "CONF_DEF - completed";
        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

