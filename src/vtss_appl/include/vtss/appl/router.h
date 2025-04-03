/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief Public Router APIs
 * \details This header file describes router control functions and types.
 */

#ifndef _VTSS_APPL_ROUTER_H_
#define _VTSS_APPL_ROUTER_H_

#include <microchip/ethernet/switch/api/types.h>  // For type declarations
#include <vtss/appl/module_id.h>             // For MODULE_ERROR_START()
#include <vtss/appl/interface.h>             // For vtss_ifindex_t
#include <vtss/basics/map.hxx>
#include <vtss/basics/set.hxx>
#include <vtss/basics/vector.hxx>

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------
//** Router module error codes
//----------------------------------------------------------------------------
/** \brief router error return codes (mesa_rc) */
enum {
    /** Generic error code */
    VTSS_APPL_ROUTER_RC_INVALID_ARGUMENT = MODULE_ERROR_START(VTSS_MODULE_ID_FRR_ROUTER),
};

//----------------------------------------------------------------------------
//** Type declaration
//----------------------------------------------------------------------------
/** \brief The data type of metric value. */
typedef uint8_t vtss_appl_router_metric_t;

//----------------------------------------------------------------------------
//** Router variables valid ranges
//----------------------------------------------------------------------------
/**< The valid range of key chain name length. (1-31) */
constexpr uint32_t VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MIN_LEN = 1;     /**<Minimum length of key-chain name */
constexpr uint32_t VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MAX_LEN = 31;    /**<Maximum length of key-chain name */

/**< The valid value of key chain key id. (1-255) */
constexpr uint32_t VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MIN = 1;         /**<Minimum value of key-chain key id */
constexpr uint32_t VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MAX = 255;       /**<Maximum value of key-chain key id */
/**< When only key chain name is created, the key id will be empty */
#define VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY (0xffffffff)          /**<Empty key-chain key id */


/**< The valid range of key string length. (1-63) */
constexpr uint32_t VTSS_APPL_ROUTER_KEY_CHAIN_KEY_STR_MIN_LEN = 1;      /**<Minimum length of key string */
constexpr uint32_t VTSS_APPL_ROUTER_KEY_CHAIN_KEY_STR_MAX_LEN = 63;     /**<Maximum length of key string */

/** The formula of converting the plain text length to encrypted data length. */
#define VTSS_APPL_ROUTER_AUTH_ENCRYPTED_KEY_LEN(x) \
    ((16 + ((x + 15) / 16) * 16 + 32) * 2)

/** Minimum length of encrypted key of key chain.  */
#define VTSS_APPL_ROUTER_KEY_CHAIN_ENCRYPTED_KEY_STR_MIN_LEN \
        VTSS_APPL_ROUTER_AUTH_ENCRYPTED_KEY_LEN(VTSS_APPL_ROUTER_KEY_CHAIN_KEY_STR_MIN_LEN)

/** Maximum length of encrypted key of key chain.  */
#define VTSS_APPL_ROUTER_KEY_CHAIN_ENCRYPTED_KEY_STR_MAX_LEN \
        VTSS_APPL_ROUTER_AUTH_ENCRYPTED_KEY_LEN(VTSS_APPL_ROUTER_KEY_CHAIN_KEY_STR_MAX_LEN)

/** Maximum count of the router key chain. */
constexpr uint32_t VTSS_APPL_ROUTER_KEY_CHAIN_MAX_COUNT = 64;

/**< The valid range of access-list name length. (1-31) */
constexpr uint32_t VTSS_APPL_ROUTER_ACCESS_LIST_NAME_MIN_LEN = 1;   /**<Minimum length of access-list name */
constexpr uint32_t VTSS_APPL_ROUTER_ACCESS_LIST_NAME_MAX_LEN = 31;  /**<Maximum length of access-list name */

/** Maximum count of the router access-list. */
constexpr uint32_t VTSS_APPL_ROUTER_ACCESS_LIST_MAX_COUNT = 65 * 2;

/**< The valid range of router access-list precedence timers. (1-128) */
/** Minimum value of router access-list precedence. */
constexpr uint32_t VTSS_APPL_ROUTER_ACE_PRECEDENCE_MIN = 1;

/** Maximum value of router access-list precedence. */
constexpr uint32_t VTSS_APPL_ROUTER_ACE_PRECEDENCE_MAX = VTSS_APPL_ROUTER_ACCESS_LIST_MAX_COUNT;

//------------------------------------------------------------------------------
//** Router module capabilities
//------------------------------------------------------------------------------
/**
 * \brief Router module capabilities
 */
typedef struct {
    /** Maximum count of the router key-chain name list. */
    uint32_t key_chain_name_list_max_count = VTSS_APPL_ROUTER_KEY_CHAIN_MAX_COUNT;

    /** Minimum string length of key-chain name. */
    uint32_t key_chain_name_len_min = VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MIN_LEN;

    /** Maximum string length of key-chain name. */
    uint32_t key_chain_name_len_max = VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MAX_LEN;

    /** Minimum value of key-chain key id. */
    uint32_t key_chain_key_id_min = VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MIN;

    /** Maximum value of key-chain key id. */
    uint32_t key_chain_key_id_max = VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_MAX;

    /** Minimum string length of plain text key-chain key string. */
    uint32_t key_chain_plain_text_key_str_len_min = VTSS_APPL_ROUTER_KEY_CHAIN_KEY_STR_MIN_LEN;

    /** Maximum string length of plain text key-chain key string. */
    uint32_t key_chain_plain_text_key_str_len_max = VTSS_APPL_ROUTER_KEY_CHAIN_KEY_STR_MAX_LEN;

    /** Minimum string length of encrypted key-chain key string. */
    uint32_t key_chain_encrypted_key_str_len_min = VTSS_APPL_ROUTER_KEY_CHAIN_ENCRYPTED_KEY_STR_MIN_LEN;

    /** Maximum string length of encrypted key-chain key string. */
    uint32_t key_chain_encrypted_key_str_len_max = VTSS_APPL_ROUTER_KEY_CHAIN_ENCRYPTED_KEY_STR_MAX_LEN;

    /** Minimum string length of access-list name. */
    uint32_t access_list_name_len_min = VTSS_APPL_ROUTER_ACCESS_LIST_NAME_MIN_LEN;

    /** Maximum string length of access-list name. */
    uint32_t access_list_name_len_max = VTSS_APPL_ROUTER_ACCESS_LIST_NAME_MAX_LEN;

    /** Maximum count of the router access-list. */
    uint32_t access_list_max_count = VTSS_APPL_ROUTER_ACCESS_LIST_MAX_COUNT;

    /** Minimum value of router access-list entry precedence. */
    uint32_t ace_precedence_min = VTSS_APPL_ROUTER_ACE_PRECEDENCE_MIN;

    /** Maximum value of router access-list entry precedence. */
    uint32_t ace_precedence_max = VTSS_APPL_ROUTER_ACE_PRECEDENCE_MAX;
} vtss_appl_router_capabilities_t;

/**
 * \brief Get ROUTER capabilities. (valid ranges and support features)
 * \param cap [OUT] ROUTER capabilities
 * \return Error code.
 */
mesa_rc vtss_appl_router_capabilities_get(vtss_appl_router_capabilities_t *const cap);

//----------------------------------------------------------------------------
//** Key chain
//----------------------------------------------------------------------------

/** \brief Key Chain Identifier. */
typedef struct {
    /** Key chain identifier name */
    char     name[VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MAX_LEN + 1];
} vtss_appl_router_key_chain_name_t;

/** \brief Key Identifier Number. */
typedef uint32_t vtss_appl_router_key_chain_key_id_t;

/** \brief Key Structure. */
typedef struct {
    /** Set 'true' to indicate the key is encrypted by AES256,
     *  otherwise it's unencrypted.
     */
    mesa_bool_t is_encrypted;
    /** The key format is decided by 'is_encrypted'.
      * If 'is_encrypted' is false, the key is plain text
      * otherwise it's encrypted.
      */
    char key[VTSS_APPL_ROUTER_KEY_CHAIN_ENCRYPTED_KEY_STR_MAX_LEN + 1];
} vtss_appl_router_key_chain_key_conf_t;

/**
 * \brief Add/Set the key chain.
 * \param key_chain_name       [IN] key chain name.
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_name_add(
    const vtss_appl_router_key_chain_name_t   *const key_chain_name);

/**
 * \brief Get the the key chain name.
 * \param key_chain_name       [IN] key chain name.
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_name_get(
    const vtss_appl_router_key_chain_name_t    *const key_chain_name);

/**
 * \brief Delete the key chain.
 * \param key_chain_name       [IN] key chain name.
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_name_del(
    const vtss_appl_router_key_chain_name_t    *const key_chain_name);

/**
 * \brief Iterate the key chain entry.
 * \param curr_key_chain_name      [IN]  Current key chain name
 * \param next_key_chain_name      [OUT] Next key chain name
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_name_itr(
    const vtss_appl_router_key_chain_name_t     *const curr_key_chain_name,
    vtss_appl_router_key_chain_name_t           *const next_key_chain_name);


/**
 * \brief Get all entries of the router key chain name.
 * \param conf [OUT] An container with all key chain name entries.
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_name_get_all(
    vtss::Vector<vtss_appl_router_key_chain_name_t> &conf);


/**
 * \brief Add/Set the key configuration of a key chain.
 * \param key_chain_name       [IN] key chain name.
 * \param key_id               [IN] key ID.
 * \param key_conf             [IN] key configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_key_conf_add(
    const vtss_appl_router_key_chain_name_t        *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t      key_id,
    const vtss_appl_router_key_chain_key_conf_t    *const key_conf);

/**
 * \brief Get the key configuration for the sepcific key ID of a key chain.
 * \param key_chain_name       [IN] key chain name.
 * \param key_id               [IN] key ID.
 * \param key_conf             [IN] key configuration.
 * \return Error code.
 * Notice that the output key string is always encrypted.
 */
mesa_rc vtss_appl_router_key_chain_key_conf_get(
    const vtss_appl_router_key_chain_name_t        *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t      key_id,
    vtss_appl_router_key_chain_key_conf_t          *const key_conf);

/**
 * \brief Delete the specific key ID in a key chain.
 * \param key_chain_name       [IN] key chain name.
 * \param key_id               [IN] key ID.
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_key_conf_del(
    const vtss_appl_router_key_chain_name_t    *const key_chain_name,
    const vtss_appl_router_key_chain_key_id_t  key_id);

/**
 * \brief Iterate the specific key ID and key chain.
 * \param curr_key_chain_name      [IN]  Current key chain name
 * \param next_key_chain_name      [OUT] Next key chain name
 * \param curr_key_id              [IN]  Current key identifier
 * \param next_key_id              [OUT] Next key identifier
 * \return Error code.
 */
mesa_rc vtss_appl_router_key_chain_key_conf_itr(
    const vtss_appl_router_key_chain_name_t     *const curr_key_chain_name,
    vtss_appl_router_key_chain_name_t           *const next_key_chain_name,
    const vtss_appl_router_key_chain_key_id_t   *const curr_key_id,
    vtss_appl_router_key_chain_key_id_t         *const next_key_id);

/**
 * \brief Get all entries of the router key chain.
 * \param conf [OUT] An container with all key chain entries.
 * \return Error code.
 * Notice that the output key string is always encrypted
 * and the key_id is VTSS_APPL_ROUTER_KEY_CHAIN_KEY_ID_EMPTY
 * when only key chain name is created.
 */
mesa_rc vtss_appl_router_key_chain_key_conf_get_all(
    vtss::Map<vtss::Pair<std::string, uint32_t>, vtss_appl_router_key_chain_key_conf_t> &conf);


//----------------------------------------------------------------------------
//** Router Access List
//----------------------------------------------------------------------------
/** \brief The name of the router access-list. */
typedef struct {
    /** The name of the access-list */
    char name[VTSS_APPL_ROUTER_ACCESS_LIST_NAME_MAX_LEN + 1];
} vtss_appl_router_access_list_name_t;

/** \brief The access right mode of the router access-list entry. */
typedef enum {
    /** Deny mode (the access right is not granted). */
    VTSS_APPL_ROUTER_ACCESS_LIST_MODE_DENY,

    /** Permit mode (the access right is granted). */
    VTSS_APPL_ROUTER_ACCESS_LIST_MODE_PERMIT,

    /** The count of this enumeration. */
    VTSS_APPL_ROUTER_ACCESS_LIST_MODE_COUNT
} vtss_appl_router_access_list_mode_t;

/**
 * \brief Add/Set the router access-list configuration.
 * \param name    [IN] The access-list name
 * \param mode    [IN] The mode of the access-list entry
 * \param network [IN] The network address of the access-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_conf_add(
    const vtss_appl_router_access_list_name_t   *const name,
    const vtss_appl_router_access_list_mode_t   mode,
    const mesa_ipv4_network_t                   network);

/**
 * \brief Get the router access-list configuration.
 * \param name    [IN] The access-list name
 * \param mode    [IN] The mode of the access-list entry
 * \param network [IN] The network address of the access-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_conf_get(
    const vtss_appl_router_access_list_name_t   *const name,
    const vtss_appl_router_access_list_mode_t   mode,
    const mesa_ipv4_network_t                   network);

/**
 * \brief Delete the router access-list configuration.
 * \param name    [IN] The access-list name
 * \param mode    [IN] The mode of the access-list entry
 * \param network [IN] The network address of the access-list entry
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_conf_del(
    const vtss_appl_router_access_list_name_t   *const name,
    const vtss_appl_router_access_list_mode_t   mode,
    const mesa_ipv4_network_t                   network);

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
    const vtss_appl_router_access_list_name_t   *const current_name,
    vtss_appl_router_access_list_name_t         *const next_name,
    const vtss_appl_router_access_list_mode_t   *const current_mode,
    vtss_appl_router_access_list_mode_t         *const next_mode,
    const mesa_ipv4_network_t                   *const current_network,
    mesa_ipv4_network_t                         *const next_network);

/** \brief The data structure for the router access-list entry configuration. */
typedef struct {
    /** The access name of the access-list entry */
    char name[VTSS_APPL_ROUTER_ACCESS_LIST_NAME_MAX_LEN + 1];

    /** The access right mode of the access-list entry */
    vtss_appl_router_access_list_mode_t mode;

    /** The network address of the access-list entry */
    mesa_ipv4_network_t network;
} vtss_appl_router_ace_conf_t;

/**
 * \brief Get the entry precedence in the specific access list.
 * \param name       [IN]  The access-list name
 * \param precedence [IN]  The access-list entry precedence
 * \param conf       [OUT] The access-list entry configuration
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_precedence_get(
    const vtss_appl_router_access_list_name_t   *const name,
    const uint32_t                              precedence,
    vtss_appl_router_ace_conf_t                 *const conf);

/**
 * \brief Get all entries of the router access-list by precedence.
 * \param conf [OUT] An container with all access-list entries.
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_precedence_get_all(
    vtss::Vector<vtss_appl_router_ace_conf_t> &conf);

/**
 * \brief Iterate the entry precedence in the specific access list.
 * \param current_name       [IN]  Current access-list name
 * \param next_name          [OUT] Next access-list name
 * \param current_precedence [IN]  The current entry precedence
 * \param next_precedence    [OUT] The next entry precedence
 * \return Error code.
 */
mesa_rc vtss_appl_router_access_list_precedence_itr(
    const vtss_appl_router_access_list_name_t   *const current_name,
    vtss_appl_router_access_list_name_t         *const next_name,
    const uint32_t                              *const current_precedence,
    uint32_t                                    *const next_precedence);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_ROUTER_H_ */
