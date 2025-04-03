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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "critd_api.h"  // For semaphore/mutex wrapper
#include "frr_router_access.hxx"
#include "frr_router_api.hxx"  // For module APIs
#include "frr_utils.hxx"       // For frr_util_secret_key_cryptography()
#include "ip_utils.hxx"        // For the operator of mesa_ipv4_network_t
#include "main.h"              // For init_cmd_t
#include "vtss/appl/ip.h"
#include "vtss/appl/vlan.h"  // For VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX
#if defined(VTSS_SW_OPTION_ICFG)
#include "frr_router_icfg.hxx"  // For module ICFG
#endif                          /* VTSS_SW_OPTION_ICFG */

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_ROUTER
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Module semaphore/mutex declaration                                        */
/******************************************************************************/
static critd_t FRR_router_crit;

struct FRR_router_lock {
    FRR_router_lock(int line)
    {
        critd_enter(&FRR_router_crit, __FILE__, line);
    }
    ~FRR_router_lock()
    {
        critd_exit(&FRR_router_crit, __FILE__, 0);
    }
};

/* Semaphore/mutex protection
 * Usage:
 * 1. Every non-static function called `router_xxx` has a CRIT_SCOPE() as the
 *    first thing in the body.
 * 2. No static function has a CRIT_SCOPE()
 * 3. If the non-static functions are not allowed to call non-static functions.
 *   (if needed, then move the functionality to a static function)
 */
#define CRIT_SCOPE() FRR_router_lock __lock_guard__(__LINE__)

/* This macro definition is used to make sure the following codes has been
 * protected by semaphore/mutex alreay. In most cases, we use it in the static
 * function. The system will raise an error if the upper layer caller doesn't
 * call CRIT_SCOPE() before calling the API. */
#define FRR_CRIT_ASSERT_LOCKED() \
    critd_assert_locked(&FRR_router_crit, __FILE__, __LINE__)

/******************************************************************************/
/** Internal APIs                                                             */
/******************************************************************************/
/**
 * Check if the access-list/key-chain name is valid or not
 * It allows all printable chacters excluding space.
 *
 * @param  name [IN]
 * @return
 * true when the name is valid. Otherwise, false.
 */
mesa_bool_t ROUTER_name_is_valid(const char *const name)
{
    size_t idx, len = strlen(name);
    for (idx = 0; idx < len; ++idx) {
        if (name[idx] < 33 || name[idx] > 126) {
            return false;
        }
    }

    return true;
}

/******************************************************************************/
/** Application public APIs                                                   */
/******************************************************************************/
//------------------------------------------------------------------------------
//** Router module capabilities
//------------------------------------------------------------------------------
/**
 * \brief Get Router module capabilities. (valid ranges and support features)
 * \param cap [OUT] Router capabilities
 * \return Error code.
 */
mesa_rc vtss_appl_router_capabilities_get(
    vtss_appl_router_capabilities_t *const cap)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!cap) {
        VTSS_TRACE(ERROR) << "Parameter 'cap' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    vtss_appl_router_capabilities_t def_cap;
    *cap = def_cap;

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** Key-chain
//----------------------------------------------------------------------------
/* Used by both 'key chain name' and 'key ID' get function.
 * For 'key chain name' get function, it passes the key_id
 * VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY
 * meaning don't care key_id. For 'key ID' get function, it passes
 * both key_chain_name and key_id. */
static mesa_rc ROUTER_key_chain_conf_get(
    const vtss_appl_router_key_chain_name_t *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t key_id, const bool as_encrypted,
    vtss_appl_router_key_chain_key_conf_t *const key_conf,
    uint32_t *const total_cnt)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!key_chain_name) {
        T_EG(FRR_TRACE_GRP_ROUTER, "Parameter 'key_chain_name' cannot be null pointer");
        return FRR_RC_INVALID_ARGUMENT;
    }

    // key_conf can be NULL, means no return configuration
    // total_cnt can be NULL, means no return the count
    if (total_cnt) {
        *total_cnt = 0;  // Given an initial value
    }

    /* Get data from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    auto frr_key_chain_conf = frr_key_chain_conf_get(frr_conf, key_id == VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY ? true /* get key-chain name*/ : false /* get key config */);
    if (total_cnt) {
        *total_cnt = frr_key_chain_conf.size();
    }

    if (frr_key_chain_conf.empty()) {
        T_DG(FRR_TRACE_GRP_OSPF, "Empty key chain");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the matched key chain name and key id entry */
    FrrRouterKeyChainResult::iterator itr;

    if (key_id != VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY) {
        itr = frr_key_chain_conf.find({key_chain_name->name, key_id});
    } else {
        itr = frr_key_chain_conf.greater_than({key_chain_name->name, 0});
    }

    if (itr == frr_key_chain_conf.end() ||
        strcmp(itr->first.first.c_str(), key_chain_name->name)) {
        VTSS_TRACE(DEBUG) << "NOT_FOUND";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    if (!key_conf) {
        return VTSS_RC_OK;  // Found it but the key configuration is not required
    }

    // Found it and encrypt the key if 'as_encrypted' is true.
    if ((!itr->second.key_str.get().empty()) && as_encrypted) {
        VTSS_TRACE(DEBUG) << " encrypting key:" << itr->second.key_str.get();
        if (frr_util_secret_key_cryptography(
                true, itr->second.key_str.get().c_str(),
                VTSS_APPL_ROUTER_KEY_CHAIN_ENCRYPTED_KEY_STR_MAX_LEN + 1,
                key_conf->key) != VTSS_RC_OK) {
            VTSS_TRACE(ERROR) << "Access framework failed: Router key chain "
                              "key encryption failed";
            return FRR_RC_INTERNAL_ERROR;
        }

        VTSS_TRACE(NOISE) << " encrypted data is " << key_conf->key;
        key_conf->is_encrypted = true;
    } else {
        strncpy(key_conf->key, itr->second.key_str.get().c_str(),
                sizeof(key_conf->key) - 1);
        key_conf->key[sizeof(key_conf->key) - 1] = '\0';
        key_conf->is_encrypted = false;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_router_key_chain_key_conf_get(
    const vtss_appl_router_key_chain_name_t *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t key_id,
    vtss_appl_router_key_chain_key_conf_t *const key_conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!key_conf) {
        VTSS_TRACE(ERROR) << "Parameter 'key_conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    return ROUTER_key_chain_conf_get(key_chain_name, key_id,
                                     true /* key as encrypted */, key_conf,
                                     NULL /* cnt */);
}

/**
 * \brief Add/Set the key configuration of a key chain.
 * \param key_chain_name       [IN] key chain name.
 * \param key_id               [IN] key ID.
 * \param key_conf             [IN] key configuration.
 * \return Error code.
 * Used by both "key chain name" and "key ID" set function.
 * For "key chain name" set function, it passes the 'key_id'
 * VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY and the conf must be NULL.
 * For "key ID" set function, it passes both 'key_chain_name' and 'key_id'
 * , 'conf' can not be NULL. */
static mesa_rc ROUTER_key_chain_key_conf_add(
    const vtss_appl_router_key_chain_name_t *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t key_id,
    const vtss_appl_router_key_chain_key_conf_t *const conf)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!key_chain_name) {
        VTSS_TRACE(ERROR) << "Parameter 'key_chain_name' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }
    // When key_id is VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY, conf must be NULL
    if ((key_id == VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY) && conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' must be null pointer when key "
                          "id is an empty key ID";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // When 'key_id' is NOT VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY,
    // 'conf' cannot be NULL, the key string cannot be empty
    if ((key_id != VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY) && !conf) {
        VTSS_TRACE(ERROR) << "Parameter 'conf' cannot be null pointer";
        return FRR_RC_INVALID_ARGUMENT;
    } else if (conf && !strlen(conf->key)) {
        VTSS_TRACE(DEBUG) << "Parameter 'conf->key' cannot be empty string";
        return FRR_RC_INVALID_ARGUMENT;
    }

    // Check invalid name and key string
    if (!ROUTER_name_is_valid(key_chain_name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'key_chain_name->name' is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (conf && !ROUTER_name_is_valid(conf->key)) {
        VTSS_TRACE(DEBUG) << "Parameter 'conf->key' is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Lookup this entry if already existing */
    uint32_t total_cnt = 0;
    mesa_rc rc = ROUTER_key_chain_conf_get(key_chain_name, key_id,
                                           false /* key as plain text */,
                                           NULL /* key conf */, &total_cnt);
    /* CHeck if reach the max. key-chain name entry count */
    if (key_id == VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY /* Add key-chain name only */ &&
        rc == FRR_RC_ENTRY_NOT_FOUND /* Add operation */ &&
        total_cnt == VTSS_APPL_ROUTER_KEY_CHAIN_MAX_COUNT) {
        return FRR_RC_LIMIT_REACHED;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND && rc != VTSS_RC_OK) {
        T_DG(FRR_TRACE_GRP_ROUTER, "Access framework failed: Get key chain configuration. (%s)", error_txt(rc));
        return rc;
    }

    /* Apply to FRR layer. */
    std::string kc_name = key_chain_name->name;
    if (key_id == VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY) {
        // Add the key chain name
        rc = frr_key_chain_name_set(kc_name, false /* add */);
    } else {
        // Set the key conf
        // If the input key format is encrypted, decrypt it.
        char plain_txt[VTSS_APPL_ROUTER_KEY_CHAIN_KEY_STR_MAX_LEN + 1] = "";
        if (conf && conf->is_encrypted) {
            mesa_rc rc = frr_util_secret_key_cryptography(
                             false /* decrypt */, conf->key,
                             // The length must INCLUDE terminal character.
                             VTSS_APPL_ROUTER_KEY_CHAIN_KEY_STR_MAX_LEN + 1, plain_txt);
            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(DEBUG)
                        << "Parameter 'encrypted_key' is invalid format";
                return rc;
            }
        } else if (conf) {
            strncpy(plain_txt, conf->key, sizeof(plain_txt) - 1);
        }

        plain_txt[sizeof(plain_txt) - 1] = '\0';

        // Set the plain text key string to FRR
        FrrKeyChainConf frr_key_conf;
        frr_key_conf.key_str = plain_txt;
        rc = frr_key_chain_key_conf_set(kc_name, key_id, frr_key_conf,
                                        false /* add */);
    }

    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set key chain. "
                          << ", key chain name = " << key_chain_name->name
                          << ", key id = " << key_id << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_router_key_chain_key_conf_add(
    const vtss_appl_router_key_chain_name_t *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t key_id,
    const vtss_appl_router_key_chain_key_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (key_id < VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MIN ||
        key_id > VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'key_id' value(" << key_id
                          << ") is invalid ";
        return FRR_RC_INVALID_ARGUMENT;
    }

    return ROUTER_key_chain_key_conf_add(key_chain_name, key_id, conf);
}

/* Used by both 'key chain name' and 'key ID' delete function.
 * For 'key chain name' delete function, it passes the key_id
 * VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY.
 * For 'key ID' delete function, it passes both key_chain_name and
 * meaning key_id and this deletes the specifc key ID. */
static mesa_rc ROUTER_key_chain_key_conf_del(
    const vtss_appl_router_key_chain_name_t *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t key_id)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!key_chain_name) {
        VTSS_TRACE(ERROR) << "Parameter 'key_chain_name' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Apply to FRR layer */
    mesa_rc rc;
    std::string kc_name = key_chain_name->name;
    if (key_id == VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY) {
        // delete the key chain name
        rc = frr_key_chain_name_set(kc_name, true /* delete */);
    } else {
        // delete the key id configuration
        rc = frr_key_chain_key_id_set(kc_name, key_id, true /* delete */);
    }

    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Delete key chain key id. "
                "key chain name = "
                << kc_name << ", key_id = " << key_id << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_router_key_chain_key_conf_del(
    const vtss_appl_router_key_chain_name_t *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t key_id)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (key_id < VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MIN ||
        key_id > VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'key_id' value(" << key_id
                          << ") is invalid ";
        return FRR_RC_INVALID_ARGUMENT;
    }

    return ROUTER_key_chain_key_conf_del(key_chain_name, key_id);
}

/**
 * \brief Get all entries of the router key chain.
 * \param conf [OUT] An container with all key chain entries.
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_key_conf_get_all(
    vtss::Map<vtss::Pair<std::string, uint32_t>,
    vtss_appl_router_key_chain_key_conf_t> &conf)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    auto frr_key_chain_conf = frr_key_chain_conf_get(frr_conf, false /* get key config */);

    for (auto &itr : frr_key_chain_conf) {
        vtss_appl_router_key_chain_key_conf_t key_conf;

        if (itr.first.second == VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY) {
            continue;
        }

        if (itr.second.key_str.valid()) {
            strncpy(key_conf.key, itr.second.key_str.get().c_str(),
                    sizeof(key_conf.key) - 1);
            key_conf.key[sizeof(key_conf.key) - 1] = '\0';

            // always get the encrypted key string
            VTSS_TRACE(DEBUG) << " encrypting key:" << itr.second.key_str.get();
            if (frr_util_secret_key_cryptography(
                    true, itr.second.key_str.get().c_str(),
                    VTSS_APPL_ROUTER_KEY_CHAIN_ENCRYPTED_KEY_STR_MAX_LEN + 1,
                    key_conf.key) != VTSS_RC_OK) {
                VTSS_TRACE(ERROR)
                        << "Access framework failed: Router key chain "
                        "key encryption failed";
                return FRR_RC_INTERNAL_ERROR;
            }

            VTSS_TRACE(DEBUG) << " encrypted data is " << key_conf.key;
            key_conf.is_encrypted = true;
        } else {
            key_conf.key[0] = '\0';
            key_conf.is_encrypted = false;
        }

        conf.insert(vtss::Pair<vtss::Pair<std::string, uint32_t>,
                    vtss_appl_router_key_chain_key_conf_t>(
                        std::move(itr.first), std::move(key_conf)));
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_router_key_chain_key_conf_itr(
    const vtss_appl_router_key_chain_name_t *const curr_key_chain_name,
    vtss_appl_router_key_chain_name_t *const next_key_chain_name,
    const vtss_appl_router_key_chain_key_id_t *const curr_key_id,
    vtss_appl_router_key_chain_key_id_t *const next_key_id)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    auto frr_key_chain_conf = frr_key_chain_conf_get(frr_conf, false /* get key config */);
    if (frr_key_chain_conf.empty()) {
        T_DG(FRR_TRACE_GRP_OSPF, "Empty key chain");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    FrrRouterKeyChainResult::iterator itr;
    if (!curr_key_chain_name) {  // Get-First operation
        itr = frr_key_chain_conf.begin();
    } else if (curr_key_chain_name) {  // Get-Next operation
        itr = frr_key_chain_conf.greater_than( {
            curr_key_chain_name->name,
            curr_key_id ? *curr_key_id : 0
        });
    }

    /* If key id is VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY means that
     * there is no key configuration
     * exists for the specified key chain, here to find the next key
     * configuration till key id is NOT VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY
     */
    while ((itr != frr_key_chain_conf.end()) &&
           (itr->first.second == VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY)) {
        itr++;

        VTSS_TRACE(DEBUG) << "find the next is: " << itr->first.first << " "
                          << itr->first.second;
    }

    if (itr != frr_key_chain_conf.end()) {
        VTSS_TRACE(DEBUG) << "Found: key chain name: " << itr->first.first
                          << ", key id = " << itr->first.second;
        strcpy(next_key_chain_name->name, itr->first.first.c_str());
        *next_key_id = itr->first.second;
        return VTSS_RC_OK;  // Found it, break the loop
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

mesa_rc vtss_appl_router_key_chain_name_add(
    const vtss_appl_router_key_chain_name_t *const key_chain_name)
{
    CRIT_SCOPE();
    return ROUTER_key_chain_key_conf_add(
               key_chain_name, VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY, NULL);
}

mesa_rc vtss_appl_router_key_chain_name_get(
    const vtss_appl_router_key_chain_name_t *const key_chain_name)
{
    CRIT_SCOPE();

    return ROUTER_key_chain_conf_get(
               key_chain_name, VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY,
               true /* key as encrypted */, NULL /* key conf */, NULL /* cnt */);
}

mesa_rc vtss_appl_router_key_chain_name_del(
    const vtss_appl_router_key_chain_name_t *const key_chain_name)
{
    CRIT_SCOPE();
    return ROUTER_key_chain_key_conf_del(
               key_chain_name, VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY);
}

/**
 * \brief Get all entries of the router key chain name list.
 * \param conf [OUT] An container with all key chain name entries.
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_name_get_all(
    vtss::Vector<vtss_appl_router_key_chain_name_t> &conf)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    auto frr_key_chain_conf = frr_key_chain_conf_get(frr_conf, true /* get key-chain name*/);
    auto itr = frr_key_chain_conf.begin();
    auto idx = itr->first;
    while (itr != frr_key_chain_conf.end()) {
        vtss_appl_router_key_chain_name_t name_conf = {};
        strcpy(name_conf.name, itr->first.first.c_str());
        conf.emplace_back(std::move(name_conf));
        idx.second = 0xffffffff;  // assign key id the maximum value to get the next name
        itr = frr_key_chain_conf.greater_than_or_equal(idx);
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_router_key_chain_name_itr(const vtss_appl_router_key_chain_name_t *const curr_key_chain_name, vtss_appl_router_key_chain_name_t *const next_key_chain_name)
{
    CRIT_SCOPE();

    /* Get data from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    auto frr_key_chain_conf = frr_key_chain_conf_get(frr_conf, true /* get key-chain name*/);
    if (frr_key_chain_conf.empty()) {
        T_DG(FRR_TRACE_GRP_ROUTER, "Empty key chain");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    FrrRouterKeyChainResult::iterator itr;
    if (!curr_key_chain_name) {  // Get-First operation
        itr = frr_key_chain_conf.begin();
    } else {  // Get-Next from the given key chain name and max value of key id
        itr = frr_key_chain_conf.greater_than(
        {curr_key_chain_name->name, 0xffffffff});
    }

    if (itr != frr_key_chain_conf.end()) {
        VTSS_TRACE(DEBUG) << "Found: key chain name: " << itr->first.first
                          << ", key id = " << itr->first.second;
        strncpy(next_key_chain_name->name, itr->first.first.c_str(),
                sizeof(next_key_chain_name->name) - 1);
        next_key_chain_name->name[sizeof(next_key_chain_name->name) - 1] = '\0';
        return VTSS_RC_OK;  // Found it
    }

    VTSS_TRACE(DEBUG) << "NOT_FOUND";
    return FRR_RC_ENTRY_NOT_FOUND;
}

//----------------------------------------------------------------------------
//** Access-list
//----------------------------------------------------------------------------
/* Mapping enum value from FrrAccessListMode to
 * vtss_appl_router_access_list_mode_t */
static vtss_appl_router_access_list_mode_t Router_ace_mode_mapping(
    FrrAccessListMode mode)
{
    switch (mode) {
    case FrrAccessListMode_Deny:
        return VTSS_APPL_ROUTER_ACCESS_LIST_MODE_DENY;
    case FrrAccessListMode_Permit:
        return VTSS_APPL_ROUTER_ACCESS_LIST_MODE_PERMIT;
    case FrrAccessListMode_End:
        break;

        /* Ignore the default case in order to catch the compile warning
         * for the missing mapping case. */
    }

    return VTSS_APPL_ROUTER_ACCESS_LIST_MODE_COUNT;
}

/* Mapping enum value from vtss_appl_router_access_list_mode_t to
 * FrrAccessListMode */
static FrrAccessListMode Router_frr_ace_mode_mapping(
    vtss_appl_router_access_list_mode_t mode)
{
    switch (mode) {
    case VTSS_APPL_ROUTER_ACCESS_LIST_MODE_DENY:
        return FrrAccessListMode_Deny;
    case VTSS_APPL_ROUTER_ACCESS_LIST_MODE_PERMIT:
        return FrrAccessListMode_Permit;
    case VTSS_APPL_ROUTER_ACCESS_LIST_MODE_COUNT:
        break;

        /* Ignore the default case in order to catch the compile warning
         * for the missing mapping case. */
    }

    return FrrAccessListMode_End;
}

static mesa_rc Router_access_list_conf_get(
    const vtss_appl_router_access_list_name_t *const name,
    const vtss_appl_router_access_list_mode_t mode,
    const mesa_ipv4_network_t network, uint32_t *const total_cnt)
{
    FRR_CRIT_ASSERT_LOCKED();

    /* Check illegal parameters */
    if (!name) {
        VTSS_TRACE(ERROR) << "Parameter 'name' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!strlen(name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' length is 0";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!total_cnt) {
        VTSS_TRACE(ERROR) << "Parameter 'total_cnt' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    } else {
        *total_cnt = 0;  // Given an initial value
    }

    /* Get running-config output from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    /* Get router access-list configuration from running-config output */
    auto frr_ace_conf = frr_access_list_conf_get_set(frr_conf);
    *total_cnt = frr_ace_conf.size();
    if (frr_ace_conf.empty()) {
        VTSS_TRACE(DEBUG) << "Empty access-list";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the next available entry */
    Set<FrrAccessList>::iterator itr;
    FrrAccessList searching_entry = {};
    searching_entry.name.append(name->name);
    searching_entry.mode = Router_frr_ace_mode_mapping(mode);
    searching_entry.net = network;
    itr = frr_ace_conf.find(searching_entry);

    if (itr != frr_ace_conf.end()) {
        return VTSS_RC_OK;  // Found it
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get the router access-list configuration.
 * \param name    [IN] The access-list name
 * \param mode    [IN] The mode of the access-list entry
 * \param network [IN] The network address of the access-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_conf_get(
    const vtss_appl_router_access_list_name_t *const name,
    const vtss_appl_router_access_list_mode_t mode,
    const mesa_ipv4_network_t network)
{
    CRIT_SCOPE();

    uint32_t total_cnt;
    auto rc = Router_access_list_conf_get(name, mode, network, &total_cnt);
    return rc;
}

/**
 * \brief Add/Set the router access-list configuration.
 * \param name    [IN] The access-list name
 * \param mode    [IN] The mode of the access-list entry
 * \param network [IN] The network address of the access-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_conf_add(
    const vtss_appl_router_access_list_name_t *const name,
    const vtss_appl_router_access_list_mode_t mode,
    const mesa_ipv4_network_t network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!name) {
        VTSS_TRACE(ERROR) << "Parameter 'name' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!strlen(name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' length is 0";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!ROUTER_name_is_valid(name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Check if entry is existing or not */
    uint32_t total_cnt;
    if (Router_access_list_conf_get(name, mode, network, &total_cnt) == VTSS_RC_OK) {
        return FRR_RC_ENTRY_ALREADY_EXISTS;
    }

    if (total_cnt == VTSS_APPL_ROUTER_ACCESS_LIST_MAX_COUNT) {
        return FRR_RC_LIMIT_REACHED;
    }

    /* Apply the new configuration */
    FrrAccessListMode frr_access_mode = Router_frr_ace_mode_mapping(mode);
    FrrAccessList frr_ace = {name->name, frr_access_mode, network};
    auto rc = frr_access_list_conf_set(frr_ace);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG) << "Access framework failed: Set router access-list. "
                          << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Delete the router access-list configuration.
 * \param name    [IN] The access-list name
 * \param mode    [IN] The mode of the access-list entry
 * \param network [IN] The network address of the access-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_conf_del(
    const vtss_appl_router_access_list_name_t *const name,
    const vtss_appl_router_access_list_mode_t mode,
    const mesa_ipv4_network_t network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!name) {
        VTSS_TRACE(ERROR) << "Parameter 'name' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!strlen(name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' length is 0";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!ROUTER_name_is_valid(name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Silent return if the entry not found */
    uint32_t total_cnt;
    mesa_rc rc = Router_access_list_conf_get(name, mode, network, &total_cnt);
    if (rc == FRR_RC_ENTRY_NOT_FOUND) {
        return VTSS_RC_OK;
    } else if (rc) {
        return rc;
    }

    /* Apply the new configuration */
    FrrAccessListMode frr_access_mode = Router_frr_ace_mode_mapping(mode);
    FrrAccessList frr_ace = {name->name, frr_access_mode, network};
    auto frr_rc = frr_access_list_conf_del(frr_ace, false);
    if (frr_rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Delete router access-list. "
                << ", rc = " << frr_rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Delete the router access-list configuration by name.
 * \param name    [IN] The access-list name
 * \return Error code.
 */
mesa_rc frr_router_access_list_conf_del_by_name(
    const vtss_appl_router_access_list_name_t *const name)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!name) {
        VTSS_TRACE(ERROR) << "Parameter 'name' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!strlen(name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' length is 0";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!ROUTER_name_is_valid(name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' is invalid";
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Apply the new configuration */
    FrrAccessList frr_ace = {};
    frr_ace.name.append(name->name);
    auto rc = frr_access_list_conf_del(frr_ace, true);
    if (rc != VTSS_RC_OK) {
        VTSS_TRACE(DEBUG)
                << "Access framework failed: Delter router access-list. "
                << ", rc = " << rc << ")";
        return FRR_RC_INTERNAL_ACCESS;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate through the router access-list.
 * \param current_name    [IN]  Current access-list name
 * \param next_name       [OUT] Next access-list name
 * \param current_mode    [IN]  Current mode
 * \param next_mode       [OUT] Next mode of the access-list entry
 * \param current_network [IN]  Current network address of the access-list entry
 * \param next_network    [OUT] Next network address of the access-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_conf_itr(
    const vtss_appl_router_access_list_name_t *const current_name,
    vtss_appl_router_access_list_name_t *const next_name,
    const vtss_appl_router_access_list_mode_t *const current_mode,
    vtss_appl_router_access_list_mode_t *const next_mode,
    const mesa_ipv4_network_t *const current_network,
    mesa_ipv4_network_t *const next_network)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_name || !next_mode || !next_network) {
        T_EG(FRR_TRACE_GRP_ROUTER, "Parameter 'next_name', 'next_mode' or 'next_network' cannot be null pointer");
        return FRR_RC_INVALID_ARGUMENT;
    }

    /* Get running-config output from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    /* Get router access-list configuration from running-config output */
    auto frr_ace_conf = frr_access_list_conf_get_set(frr_conf);
    if (frr_ace_conf.empty()) {
        T_DG(FRR_TRACE_GRP_ROUTER, "Empty access-list");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the next available entry */
    vtss::Set<FrrAccessList>::iterator itr;
    FrrAccessList searching_entry = {};
    if (!current_name) {  // Get-First operation
        itr = frr_ace_conf.begin();
    } else {  // Get-Next operation
        searching_entry.name.append(current_name->name);
        if (current_mode) {
            searching_entry.mode = Router_frr_ace_mode_mapping(*current_mode);
            if (current_network) {
                searching_entry.net = *current_network;
                VTSS_TRACE(DEBUG) << "Invoke greater_than() ";
                itr = frr_ace_conf.greater_than(searching_entry);
            } else {
                VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal()";
                itr = frr_ace_conf.greater_than_or_equal(searching_entry);
            }
        } else {
            VTSS_TRACE(DEBUG) << "Invoke greater_than_or_equal()";
            itr = frr_ace_conf.greater_than_or_equal(searching_entry);
        }
    }

    if (itr != frr_ace_conf.end()) {  // Found it
        strcpy(next_name->name, itr->name.c_str());
        *next_mode = Router_ace_mode_mapping(itr->mode);
        *next_network = itr->net;
        return VTSS_RC_OK;
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get the entry precedence in the specific access list.
 * \param name       [IN]  The access-list name
 * \param precedence [IN]  The access-list entry precedence
 * \param conf       [OUT] The access-list entry configuration
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_precedence_get(
    const vtss_appl_router_access_list_name_t *const name,
    const uint32_t precedence, vtss_appl_router_ace_conf_t *const conf)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!name || !conf) {
        VTSS_TRACE(ERROR) << "Parameter 'name' or 'conf' cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (!strlen(name->name)) {
        VTSS_TRACE(DEBUG) << "Parameter 'name' length is 0";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (precedence < VTSS_APPL_ROUTER_ACE_PRECEDENCE_MIN ||
        precedence > VTSS_APPL_ROUTER_ACE_PRECEDENCE_MAX) {
        VTSS_TRACE(DEBUG) << "Parameter 'precedence' out of valid range";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get running-config output from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    /* Get router access-list configuration from running-config output */
    auto frr_ace_conf = frr_access_list_conf_get(frr_conf);
    if (frr_ace_conf.empty()) {
        T_DG(FRR_TRACE_GRP_ROUTER, "Empty access-list");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Lookup the specific entry by precedence */
    std::string search_name = "";
    search_name.append(name->name);
    uint32_t ace_idx = 0;
    for (const auto &itr : frr_ace_conf) {
        if (itr.name == search_name) {
            ace_idx++;
            if (ace_idx == precedence) {  // Found it
                conf->mode = Router_ace_mode_mapping(itr.mode);
                conf->network = itr.net;
                return VTSS_RC_OK;
            }
        } else if (itr.name > search_name ||
                   (ace_idx && itr.name != search_name)) {
            break;
        }
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/**
 * \brief Get all entries of the router access-list by precedence.
 * \param conf [OUT] An container with all access-list entries.
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_precedence_get_all(vtss::Vector<vtss_appl_router_ace_conf_t> &conf)
{
    CRIT_SCOPE();

    /* Get running-config output from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    /* Get router access-list configuration from running-config output */
    auto frr_ace_conf = frr_access_list_conf_get(frr_conf);
    for (const auto &itr : frr_ace_conf) {
        vtss_appl_router_ace_conf_t ace_conf;
        strcpy(ace_conf.name, itr.name.c_str());
        ace_conf.mode = Router_ace_mode_mapping(itr.mode);
        ace_conf.network = itr.net;
        conf.emplace_back(std::move(ace_conf));
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate the entry precedence in the specific access list.
 * \param current_name       [IN]  Current access-list name
 * \param next_name          [OUT] Next access-list name
 * \param current_precedence [IN]  The current entry precedence
 * \param next_precedence    [OUT] The next entry precedence
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_precedence_itr(
    const vtss_appl_router_access_list_name_t *const current_name,
    vtss_appl_router_access_list_name_t *const next_name,
    const uint32_t *const current_precedence,
    uint32_t *const next_precedence)
{
    CRIT_SCOPE();

    /* Check illegal parameters */
    if (!next_name || !next_precedence) {
        VTSS_TRACE(ERROR) << "Parameter 'next_name' or 'next_precedence' "
                          "cannot be null point";
        return FRR_RC_INVALID_ARGUMENT;
    }

    if (current_precedence &&
        *current_precedence > VTSS_APPL_ROUTER_ACE_PRECEDENCE_MAX) {
        VTSS_TRACE(WARNING) << "Parameter 'precedence' out of valid range";
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Get running-config output from FRR layer */
    std::string frr_conf;
    VTSS_RC(frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, frr_conf));

    /* Get router access-list configuration from running-config output */
    auto frr_ace_conf = frr_access_list_conf_get(frr_conf);
    if (frr_ace_conf.empty()) {
        T_DG(FRR_TRACE_GRP_ROUTER, "Empty access-list");
        return FRR_RC_ENTRY_NOT_FOUND;
    }

    /* Process the Get-First operation */
    if (!current_name) {
        strcpy(next_name->name, frr_ace_conf[0].name.c_str());
        *next_precedence = 1;
        return VTSS_RC_OK;
    }

    /* Lookup the next valid entry */
    uint32_t ace_idx = 0;
    uint32_t search_precedence = current_precedence ? *current_precedence : 0;
    std::string search_name = "";
    search_name.append(current_name->name);

    for (const auto &itr : frr_ace_conf) {
        if (itr.name == search_name) {
            ace_idx++;
            if (ace_idx > search_precedence) {
                // Found it (same name and greater precedence)
                strcpy(next_name->name, itr.name.c_str());
                *next_precedence = ace_idx;
                return VTSS_RC_OK;
            }
        } else if ((ace_idx && itr.name != search_name) ||
                   itr.name > search_name) {
            // Found it (greater name)
            strcpy(next_name->name, itr.name.c_str());
            *next_precedence = 1;
            return VTSS_RC_OK;
        } else {
            ace_idx = 0;
        }
    }

    return FRR_RC_ENTRY_NOT_FOUND;
}

/******************************************************************************/
/** Module error text (convert the return code to error text)                 */
/******************************************************************************/
const char *frr_router_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_ROUTER_RC_INVALID_ARGUMENT:
        return "Invalid argument";
    }

    T_EG(FRR_TRACE_GRP_ROUTER, "Unknown error code: 0x%x", rc);
    return "FRR-Router: Unknown error code";
}

/******************************************************************************/
/** Module initialization                                                     */
/******************************************************************************/
#if defined(VTSS_SW_OPTION_JSON_RPC)
VTSS_PRE_DECLS void frr_router_json_init(void);
#endif /* VTSS_SW_OPTION_JSON_RPC */

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
/* Initialize private mib */
VTSS_PRE_DECLS void frr_router_mib_init(void);
#endif

extern "C" int frr_router_icli_cmd_register();

/* Initialize module */
mesa_rc frr_router_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        VTSS_TRACE(INFO) << "INIT";
        /* Initialize and register semaphore/mutex resources */
        critd_init(&FRR_router_crit, "frr_router.crit", VTSS_MODULE_ID_FRR_ROUTER, CRITD_TYPE_MUTEX);

#if defined(VTSS_SW_OPTION_ICFG)
        /* Initialize and register ICFG resources */
        if (frr_has_router()) {
            frr_router_icfg_init();
        }
#endif /* VTSS_SW_OPTION_ICFG */

#if defined(VTSS_SW_OPTION_JSON_RPC)
        /* Initialize and register JSON resources */
        if (frr_has_router()) {
            frr_router_json_init();
        }
#endif /* VTSS_SW_OPTION_JSON_RPC */

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        /* Initialize and register private MIB resources */
        if (frr_has_router()) {
            frr_router_mib_init();
        }
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */

        /* Initialize and register ICLI resources */
        if (frr_has_router()) {
            frr_router_icli_cmd_register();
        }

        VTSS_TRACE(INFO) << "INIT - completed";
    }

    return VTSS_RC_OK;
}

