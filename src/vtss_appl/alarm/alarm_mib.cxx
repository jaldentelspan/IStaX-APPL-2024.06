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

#include "alarm_serializer.hxx"
#include "alarm-expose.hxx"
#include "vtss/appl/alarm.h"

#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("alarmMib", "ALARM", alarm_mib_init, VTSS_MODULE_ID_ALARM, root, h) {
    h.add_history_element("201602080000Z", "Initial version");
    h.description("This is a private mib for alarms");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(objects, root, 1, "alarmMibObjects");
NS(alarm_config, objects, 2, "alarmConfig");
NS(alarm_status, objects, 3, "alarmStatus");
NS(alarm_control, objects, 4, "alarmControl");
NS(ns_traps, objects, 6, "alarmTrap");


namespace vtss {
namespace appl {
namespace alarm {

static TableReadWriteAddDelete2<AlarmConfTable> alarm_conf_table(
    &alarm_config, vtss::expose::snmp::OidElement(1, "alarmConfigTable"),
    vtss::expose::snmp::OidElement(2, "alarmConfigTableRowEditor"));

// static TableReadOnly2<AlarmStatusTable> alarm_status_table(
//     &alarm_status,
//     vtss::expose::snmp::OidElement(1, "alarmStatusTable"));

static TableReadOnlyTrap<AlarmStatusTable> _alarm_status(
        &alarm_status, OidElement(1, "alarmStatus"),
        &the_alarm_status, &ns_traps, "alarmTrapStatus",
        OidElement(1, "alarmTrapStatusAdd"),
        OidElement(2, "alarmTrapStatusMod"),
        OidElement(3, "alarmTrapStatusDel"));

static TableReadWrite2<AlarmControlTable> alarm_control_table(
    &alarm_control, vtss::expose::snmp::OidElement(1, "alarmControlTable"));

}  // namespace alarm
}  // namespace appl
}  // namespace vtss
