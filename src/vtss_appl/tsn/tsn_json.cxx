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

#include "tsn_serializer.hxx"
#include "tsn_fp_serializer.hxx"
#include "tsn_tas_serializer.hxx"
#include "tsn_expose.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::tsn::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_tsn("tsn");
extern "C" void vtss_appl_tsn_json_init()
{
    json_node_add(&ns_tsn);
}

NS(ns_conf,             ns_tsn,         "config");
NS(ns_status,           ns_tsn,         "status");
NS(ns_control,          ns_tsn,         "control");

NS(ns_conf_if,          ns_conf,        "interface");
NS(ns_conf_if_tas,      ns_conf_if,     "tas");

NS(ns_status_if,        ns_status,      "interface");
NS(ns_status_if_tas,    ns_status_if,   "tas");

namespace vtss
{
namespace appl
{
namespace tsn
{
namespace interfaces
{

static StructReadWrite<TsnConfigGlobalsLeaf> tsn_config_globals_leaf(&ns_conf, "global");
static StructReadOnly<TsnCapabilitiesLeaf> tsn_capabilities(&ns_tsn, "capabilities");
static StructReadOnly<TsnStatusGlobalsLeaf> tsn_status_globals_leaf(&ns_status, "global");

static TableReadWrite<TsnFpCfgTable> tsn_fp_cfg_entry(&ns_conf_if, "fp");
static TableReadOnly<TsnFpStatusTable> tsn_fp_status_entry(&ns_status_if, "fp");

static StructReadWrite<TsnTasConfigGlobalsLeaf> tsn_tas_config_globals_leaf(&ns_conf, "tas");
static TableReadWrite<TsnTasPerQMaxSduEntry> tsn_tas_max_sdu_entry(&ns_conf_if_tas, "maxSdu");
static TableReadWrite<TsnTasGclAdminEntry> tsn_tas_gcl_admin_entry(&ns_conf_if_tas, "gclEntry");
static TableReadOnly<TsnTasGclOperEntry> tsn_tas_gcl_oper_entry(&ns_status_if_tas, "gclEntry");
static TableReadWrite<TsnTasParamsEntry> tsn_tas_params_entry(&ns_conf_if_tas, "params");
static TableReadOnly<TsnTasOperStatusTable> tsn_tas_oper_status_entry(&ns_status_if_tas, "params");

}  // namespace interfaces
}  // namespace tsn
}  // namespace appl
}  // namespace vtss
