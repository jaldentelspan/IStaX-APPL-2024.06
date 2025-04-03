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

#include "daylight_saving_serializer.hxx"
#include "vtss/appl/daylight_saving.h"

VTSS_MIB_MODULE("DaylightSavingMib", "DAYLIGHT-SAVING",
                Daylight_saving_mib_init, VTSS_MODULE_ID_DAYLIGHT_SAVING, root,
                h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201701060000Z", "Update time zone offset valid range");
    h.description(
            "This is a private version of daylight saving. Used to configure "
            "system Summer time(Daylight Saving) and Time Zone.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace daylight_saving {
namespace interfaces {

NS(daylight_saving_mib_objects, root, 1, "DaylightSavingMibObjects");
NS(daylight_saving_config, daylight_saving_mib_objects, 2,
   "DaylightSavingConfig");

static StructRW2<DaylightSavingGlobalsLeaf> daylight_saving_globals_leaf(
        &daylight_saving_config,
        vtss::expose::snmp::OidElement(1, "DaylightSavingConfigGlobals"));

}  // namespace interfaces
}  // namespace daylight_saving
}  // namespace appl
}  // namespace vtss
