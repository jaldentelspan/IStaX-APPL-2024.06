/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "cfm_serializer.hxx"
#include "cfm_expose.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::cfm::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_cfm("cfm");
extern "C" void vtss_appl_cfm_json_init()
{
    json_node_add(&ns_cfm);
}

NS(ns_conf, ns_cfm, "config");
NS(ns_status, ns_cfm, "status");

// config
NS(ns_conf_md, ns_conf, "md");
NS(ns_conf_ma, ns_conf, "ma");
NS(ns_conf_mep, ns_conf, "mep");
NS(ns_conf_mepr, ns_conf, "mepr");
NS(ns_conf_rmep, ns_conf, "rmep");


namespace vtss
{
namespace appl
{
namespace cfm
{
namespace interfaces
{
// Wrappers used in serializer to provide a fixed value = false in iterators
mesa_rc vtss_appl_cfm_ma_itr_wrap(const vtss_appl_cfm_ma_key_t *prev_key, vtss_appl_cfm_ma_key_t *next_key)
{
    return vtss_appl_cfm_ma_itr(prev_key, next_key, false);
}

mesa_rc vtss_appl_cfm_mep_itr_wrap(const vtss_appl_cfm_mep_key_t *prev_key, vtss_appl_cfm_mep_key_t *next_key)
{
    return vtss_appl_cfm_mep_itr(prev_key, next_key, false);
}

mesa_rc vtss_appl_cfm_rmep_itr_wrap(const vtss_appl_cfm_rmep_key_t *prev_key, vtss_appl_cfm_rmep_key_t *next_key)
{
    return vtss_appl_cfm_rmep_itr(prev_key, next_key, false);
}

// Wrappers used in serializer to provide default values
mesa_rc vtss_appl_cfm_md_conf_default_get_wrap(vtss_appl_cfm_md_key_t *key, vtss_appl_cfm_md_conf_t *conf)
{
    vtss_appl_cfm_md_conf_t def_conf;
    if (conf) {
        VTSS_RC(vtss_appl_cfm_md_conf_default_get(&def_conf));
        *conf = def_conf;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_cfm_ma_conf_default_get_wrap(vtss_appl_cfm_ma_key_t *key, vtss_appl_cfm_ma_conf_t *conf)
{
    vtss_appl_cfm_ma_conf_t def_conf;
    if (conf) {
        VTSS_RC(vtss_appl_cfm_ma_conf_default_get(&def_conf));
        *conf = def_conf;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_cfm_mep_conf_default_get_wrap(vtss_appl_cfm_mep_key_t *key, vtss_appl_cfm_mep_conf_t *conf)
{
    vtss_appl_cfm_mep_conf_t def_conf;
    if (conf) {
        VTSS_RC(vtss_appl_cfm_mep_conf_default_get(&def_conf));
        *conf = def_conf;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_cfm_rmep_conf_default_get_wrap(vtss_appl_cfm_rmep_key_t *key, vtss_appl_cfm_rmep_conf_t *conf)
{
    vtss_appl_cfm_rmep_conf_t def_conf;
    if (conf) {
        VTSS_RC(vtss_appl_cfm_rmep_conf_default_get(&def_conf));
        *conf = def_conf;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_cfm_mep_conf_wrap_default_get_wrap(vtss_appl_cfm_mep_key_t *key, vtss_appl_cfm_mep_conf_wrap_t *conf)
{
    vtss_appl_cfm_mep_conf_t def_conf;
    if (conf) {
        VTSS_RC(vtss_appl_cfm_mep_conf_default_get(&def_conf));
        conf->mep = def_conf;
        conf->rmepid = 0;
    }
    return VTSS_RC_OK;
}

// Wrappers to offer a single function to get/set both mep and rmep values. Used in JSON.
mesa_rc vtss_appl_cfm_mep_conf_get_wrap(const vtss_appl_cfm_mep_key_t &key, vtss_appl_cfm_mep_conf_wrap_t *confw)
{
    mesa_rc rc;
    vtss_appl_cfm_mep_conf_t conf;
    vtss_appl_cfm_rmep_key_t prev_rmep_key, next_rmep_key;
    if (VTSS_RC_OK != (rc = vtss_appl_cfm_mep_conf_get(key, &conf))) {
        return rc;
    };

    confw->mep              = conf;
    confw->rmepid           = 0;

    // Search for the one and only rmep below this mep
    prev_rmep_key.md        = key.md;
    prev_rmep_key.ma        = key.ma;
    prev_rmep_key.mepid     = key.mepid;
    prev_rmep_key.rmepid    = 0;

    if (VTSS_RC_OK == vtss_appl_cfm_rmep_itr(&prev_rmep_key, &next_rmep_key, true)) {
        confw->rmepid = next_rmep_key.rmepid;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_cfm_mep_conf_set_wrap(const vtss_appl_cfm_mep_key_t &key, const vtss_appl_cfm_mep_conf_wrap_t *confw)
{
    mesa_rc rc;
    bool rmep_found;
    vtss_appl_cfm_mep_conf_t  conf;
    vtss_appl_cfm_rmep_conf_t rmep_conf;
    vtss_appl_cfm_rmep_key_t  prev_rmep_key, next_rmep_key;

    conf = confw->mep;
    if (VTSS_RC_OK != (rc = vtss_appl_cfm_mep_conf_set(key, &conf))) {
        return rc;
    }
    prev_rmep_key.md        = key.md;
    prev_rmep_key.ma        = key.ma;
    prev_rmep_key.mepid     = key.mepid;
    prev_rmep_key.rmepid    = 0;
    rc = vtss_appl_cfm_rmep_itr(&prev_rmep_key, &next_rmep_key, true);
    switch (rc) {
    case VTSS_RC_OK:
        rmep_found = true;
        break;
    case VTSS_APPL_CFM_RC_END_OF_LIST:
        rmep_found = false;
        break;
    default:
        rmep_found = false;
    }

    if (rmep_found) {
        if (confw->rmepid == 0 || confw->rmepid != next_rmep_key.rmepid) { // remove or change rmep
            if (VTSS_RC_OK != (rc = vtss_appl_cfm_rmep_conf_del(next_rmep_key))) {
                return rc;
            }
        }
        if (confw->rmepid != 0 ) { // create rmep (again)
            vtss_appl_cfm_rmep_conf_default_get(&rmep_conf);
            prev_rmep_key.rmepid = confw->rmepid;
            if (VTSS_RC_OK != (rc = vtss_appl_cfm_rmep_conf_set(prev_rmep_key, &rmep_conf))) {
                return rc;
            }
        }
    } else {
        if (confw->rmepid != 0 ) { // no rmep found. Shall we create one?
            vtss_appl_cfm_rmep_conf_default_get(&rmep_conf);
            prev_rmep_key.rmepid = confw->rmepid;
            if (VTSS_RC_OK != (rc = vtss_appl_cfm_rmep_conf_set(prev_rmep_key, &rmep_conf))) {
                return rc;
            }
        }
    }
    return VTSS_RC_OK;
}

static StructReadWrite<cfmConfigGlobalsLeaf> cfm_config_globals_leaf(&ns_conf, "global");
static StructReadOnly<cfmCapabilitiesLeaf> cfm_capabilities(&ns_cfm, "capabilities");
static TableReadWriteAddDeleteNoNS<cfmConfigMdEntry> cfm_md_entry(&ns_conf_md);
static TableReadWriteAddDeleteNoNS<cfmConfigMaEntry> cfm_ma_entry(&ns_conf_ma);
static TableReadWriteAddDeleteNoNS<cfmConfigMepEntry> cfm_mep_entry(&ns_conf_mep);
static TableReadWriteAddDeleteNoNS<cfmConfigMepWrapEntry> cfm_mepr_entry(&ns_conf_mepr);
static TableReadWriteAddDeleteNoNS<cfmConfigRMepEntry> cfm_rmep_entry(&ns_conf_rmep);
static TableReadOnly<cfmStatusMepEntry> cfm_status_mep(&ns_status, "mep");
static TableReadOnly<cfmStatusRMepEntry> cfm_status_rmep(&ns_status, "rmep");
static TableReadOnlyNotification<cfmStatusNotificationEntry> cfm_status_notification(&ns_status, "notification", &cfm_mep_notification_status);


}  // namespace interfaces
}  // namespace cfm
}  // namespace appl
}  // namespace vtss
