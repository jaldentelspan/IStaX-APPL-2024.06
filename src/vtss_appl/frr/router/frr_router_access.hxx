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

#ifndef _FRR_ROUTER_ACCESS_HXX_
#define _FRR_ROUTER_ACCESS_HXX_

#include <unistd.h>
#ifdef VTSS_BASICS_STANDALONE
#include <vtss/basics/api_types.h>
#else
#include <vtss/appl/interface.h>
#include <vtss/appl/types.h>
#endif
#include <string>
#include <vtss/basics/map.hxx>
#include <vtss/basics/optional.hxx>
#include <vtss/basics/string.hxx>
#include <vtss/basics/time.hxx>
#include <vtss/basics/vector.hxx>

#include "frr_daemon.hxx"

namespace vtss
{

//----------------------------------------------------------------------------
//** Key chain
//----------------------------------------------------------------------------
struct FrrKeyChainConf {
    vtss::Optional<std::string> key_str;
};

using FrrKeyChainEntryKey = vtss::Pair<std::string, uint32_t>;
using FrrRouterKeyChainResult = Map<FrrKeyChainEntryKey, FrrKeyChainConf>;

/* The Key ID 0xffffffff means empty */
FrrRouterKeyChainResult frr_key_chain_conf_get(std::string &running_conf, bool name_only);

/* Add/Del key chain name */
mesa_rc frr_key_chain_name_set(const std::string &keychain_name,
                               const bool is_delete);

/* Add/Del a key id of a specified "key chain" */
mesa_rc frr_key_chain_key_id_set(const std::string &keychain_name,
                                 const uint32_t key_id, const bool is_delete);

/* Set/Erase key config of a specified "key id" and "key chain" */
mesa_rc frr_key_chain_key_conf_set(const std::string &keychain_name,
                                   const uint32_t key_id,
                                   const FrrKeyChainConf &key_conf,
                                   const bool is_delete);

/** For unit-test purpose */
/* Add/Del key chain name */
Vector<std::string> to_vty_key_chain_name_set(const std::string &key_chain,
                                              const bool is_delete);

/* Add/Del a key id of a specified "key chain" */
Vector<std::string> to_vty_key_chain_key_id_set(const std::string &keychain_name,
                                                const uint32_t key_id,
                                                const bool is_delete);

/* Set/Erase key config of a specified "key id" and "key chain" */
Vector<std::string> to_vty_key_chain_key_conf_set(
    const std::string &keychain_name, const uint32_t key_id,
    const FrrKeyChainConf &key_conf, const bool is_erase);

//----------------------------------------------------------------------------
//** access-list
//----------------------------------------------------------------------------
/* The enumeration for the access list mode. */
enum FrrAccessListMode {
    FrrAccessListMode_Deny,
    FrrAccessListMode_Permit,
    FrrAccessListMode_End
};

struct FrrAccessList {
    std::string name;
    FrrAccessListMode mode;
    mesa_ipv4_network_t net; /* 0.0.0.0/0 means any in FRR */
};

Vector<FrrAccessList> frr_access_list_conf_get(std::string &running_conf);
Set<FrrAccessList> frr_access_list_conf_get_set(std::string &running_conf);
mesa_rc frr_access_list_conf_set(const FrrAccessList &val);

/**
 * \brief Convert the access list structure to the FRR command.
 * \param FrrAccessList  [IN] The access list structure.
 * \param is_list        [IN] When it is true, the whole access list is
 * removed.
 * When it is false, the specific entry in the list is removed
 * \return 'true' indicates the ifindex is valid for FRR.
 */
mesa_rc frr_access_list_conf_del(FrrAccessList &val, bool is_list);

/** For unit-test purpose */
Vector<std::string> to_vty_access_list_conf_set(const FrrAccessList &val);
Vector<std::string> to_vty_access_list_conf_del(FrrAccessList val, bool is_list);
bool operator<(const FrrAccessList &a, const FrrAccessList &b);

}  // namespace vtss

#endif  // _FRR_ROUTER_ACCESS_HXX_

