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

#include <vtss/basics/enum-descriptor.h>
#include <vtss/appl/ddmi.h>

vtss_enum_descriptor_t ddmi_monitor_type_txt[] {
    {VTSS_APPL_DDMI_MONITOR_TYPE_TEMPERATURE, "temperature"},
    {VTSS_APPL_DDMI_MONITOR_TYPE_VOLTAGE,     "voltage"},
    {VTSS_APPL_DDMI_MONITOR_TYPE_TX_BIAS,     "txBias"},
    {VTSS_APPL_DDMI_MONITOR_TYPE_TX_POWER,    "txPower"},
    {VTSS_APPL_DDMI_MONITOR_TYPE_RX_POWER,    "rxPower"},
    {0, 0},
};

vtss_enum_descriptor_t ddmi_monitor_state_txt[] {
    {VTSS_APPL_DDMI_MONITOR_STATE_NONE,     "none"},
    {VTSS_APPL_DDMI_MONITOR_STATE_WARN_LO,  "lowWarn"},
    {VTSS_APPL_DDMI_MONITOR_STATE_WARN_HI,  "highWarn"},
    {VTSS_APPL_DDMI_MONITOR_STATE_ALARM_LO, "lowAlarm"},
    {VTSS_APPL_DDMI_MONITOR_STATE_ALARM_HI, "highAlarm"},
    {0, 0},
};

