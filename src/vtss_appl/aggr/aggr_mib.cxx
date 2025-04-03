/*
 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "aggr_serializer.hxx"
#include "vtss/appl/aggr.h"

#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("aggrMib", "AGGR", aggr_mib_init, VTSS_MODULE_ID_AGGR, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201411180000Z", "Added aggregation group status table");
    h.add_history_element("201507070000Z", "Port speed is moved into the TC MIB");
    h.add_history_element("201707310000Z", "Added new aggregation modes: disabled, reserved, static, lacpActive, lacpPassive");
    h.description("This is a private mib of aggregation management");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(objects, root, 1, "aggrMibObjects");
NS(aggr_config, objects, 2, "aggrConfig");
NS(aggr_status, objects, 3, "aggrStatus");


namespace vtss {
namespace appl {
namespace aggr {
namespace interfaces {

    static StructRW2<AggrModeParamsLeaf> aggr_mode_params_leaf(
        &aggr_config, vtss::expose::snmp::OidElement(1, "aggrConfigModeGlobals"));

    static TableReadWrite2<AggrGroupConfTable> aggr_group_conf_table(
        &aggr_config, vtss::expose::snmp::OidElement(2, "aggrConfigGroupTable"));

    static TableReadOnly2<AggrGroupStatusTable> aggr_group_status(
        &aggr_status, OidElement(3, "aggrStatusGroupTable"));

}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss

