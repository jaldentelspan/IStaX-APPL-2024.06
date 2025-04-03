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

#include "syslog_serializer.hxx"
#include "vtss/appl/syslog.h"
#include "vtss_common_iterator.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-depend-N.hxx"

VTSS_MIB_MODULE("syslogMib", "SYSLOG", syslog_mib_init, VTSS_MODULE_ID_SYSLOG, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.description("This is a private MIB for Syslog");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace syslog {
namespace interfaces {
NS(syslog_mib_objects, root, 1, "SyslogMibObjects");;
NS(syslog_config, syslog_mib_objects, 2, "SyslogConfig");;
NS(syslog_status, syslog_mib_objects, 3, "SyslogStatus");;
NS(syslog_control, syslog_mib_objects, 4, "SyslogControl");;

static StructRW2<SyslogConfigGlobals> syslog_config_globals(
        &syslog_config, vtss::expose::snmp::OidElement(1, "SyslogConfigServer"));

static TableReadOnly2<SyslogStatusHistoryTbl> syslog_status_history_tbl(
        &syslog_status, vtss::expose::snmp::OidElement(1, "SyslogStatusHistoryTable"));

static TableReadWrite2<SyslogControlHistoryTbl> syslog_control_history_tbl(
        &syslog_control, vtss::expose::snmp::OidElement(1, "SyslogControlHistoryTable"));

}  // namespace interfaces
}  // namespace syslog
}  // namespace appl
}  // namespace vtss
