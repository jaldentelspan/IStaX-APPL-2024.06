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

#include "sysutil_serializer.hxx"

const vtss_enum_descriptor_t vtss_appl_sysutil_reboot_type_txt[] {
    {VTSS_APPL_SYSUTIL_REBOOT_TYPE_NONE,    "noReboot"},
    {VTSS_APPL_SYSUTIL_REBOOT_TYPE_COLD,    "coldReboot"},
    {VTSS_APPL_SYSUTIL_REBOOT_TYPE_WARM,    "warmReboot"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_sysutil_psu_state_txt[] {
    {VTSS_APPL_SYSUTIL_PSU_STATE_ACTIVE,      "active"},
    {VTSS_APPL_SYSUTIL_PSU_STATE_STANDBY,     "standby"},
    {VTSS_APPL_SYSUTIL_PSU_STATE_NOT_PRESENT, "notPresent"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_sysutil_system_led_clear_type_txt[] {
    {VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_ALL,           "all"},
    {VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_FATAL,         "fatal"},
    {VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_SW,            "software"},
    {VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_POST,          "post"},
    {VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_ZTP,           "ztp"},
    {VTSS_APPL_SYSUTIL_SYSTEM_LED_CLEAR_TYPE_STACK_FW_CHK,  "stackFwChk"},
    {0, 0},
};


const vtss_enum_descriptor_t vtss_appl_sysutil_tm_sensor_type_txt[] {
    {VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD,    "board"},
    {VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION, "junction"},
    {0, 0},
};

