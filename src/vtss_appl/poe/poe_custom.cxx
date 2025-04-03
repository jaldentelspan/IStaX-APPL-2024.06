/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
Debugging tips:
To enable debugging of I2C communication to PoE controller do:
deb tra mod level main board debug

To decode the messages to the controller copy the following lines to pd69000.sed and output from the trace to i2c.log.

s/\(00 .. 05 0B\) \(..\) \(.. ..\) \(.*\)/\1 \2 \3 \4 Set Power Limit, Port \2, PPL \3/g
s/\(00 .. 05 0b\) \(..\) \(.. ..\) \(.*\)/\1 \2 \3 \4 Set Power Limit, Port \2, PPL \3/g
s/\(00 .. 05 A2\) \(..\) \(.. ..\) \(.*\)/\1 \2 \3 \4 Set Temporary Power Limit, Port \2, TPPL \3/g
s/\(00 .. 05 a2\) \(..\) \(.. ..\) \(.*\)/\1 \2 \3 \4 Set Temporary Power Limit, Port \2, TPPL \3/g
s/\(00 .. 07 56\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 Set Individual Mask, Key \2, val \3/g
s/\(00 .. 07 2B\) \(..\) \(.*\)/\1 \2 \3 Set System Masks: \2/g
s/\(00 .. 07 2b\) \(..\) \(.*\)/\1 \2 \3 Set System Masks: \2/g
s/\(00 .. 05 A6\) \(..\) \(..\) \(.. ..\) \(.. ..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 \6 \7 \8 Set Port Layer2 LLDP Data: Port \2, Type \3, Req \4 alloc \5 cable \6 exec \7/g
s/\(00 .. 05 a6\) \(..\) \(..\) \(.. ..\) \(.. ..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 \6 \7 \8 Set Port Layer2 LLDP Data: Port \2, Type \3, Req \4 alloc \5 cable \6 exec \7/g
s/\(00 .. 05 43\) \(..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 Set temporary matrix, Port \2 (\3, \4) /g
s/\(00 .. 07 43\) \(.*\)/\1 \2 Program Global Martrix/g
s/\(00 .. 07 0B 5F\) \(..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 Set PM Mode, PM1 \2, PM2 \3 PM3 \4/g
s/\(00 .. 07 0b 5f\) \(..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 Set PM Mode, PM1 \2, PM2 \3 PM3 \4/g
s/\(00 .. 05 0A\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 Set Port Priority, Port \2, Priority \3/g
s/\(00 .. 05 0a\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 Set Port Priority, Port \2, Priority \3/g
s/\(00 .. 07 0B 57\) \(..\) \(.. ..\) \(.. ..\) \(.. ..\) \(..\)\(.*\)/\1 \2 \3 \4 \5 \6 \7 Set Power Banks, Bank \2, Power Limit \3, V-Max \4, V-Min \5, Guard \6/g
s/\(00 .. 07 0b 57\) \(..\) \(.. ..\) \(.. ..\) \(.. ..\) \(..\)\(.*\)/\1 \2 \3 \4 \5 \6 \7 Set Power Banks, Bank \2, Power Limit \3, V-Max \4, V-Min \5, Guard \6/g
s/\(00 .. 05 0C\) \(..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 Enable\/Disable channels, Port \2, cmd \3 Type \4/g
s/\(00 .. 05 0c\) \(..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 Enable\/Disable channels, Port \2, cmd \3 Type \4/g
s/\(02 .. 07 1E 21\) \(.*\)/\1 \2 Get Software version/g
s/\(02 .. 07 1e 21\) \(.*\)/\1 \2 Get Software version/g
s/\(02 .. 05 0B\) \(..\) \(.*\)/\1 \2 \3 Get Port Power Limit, Port \2/g
s/\(02 .. 05 0b\) \(..\) \(.*\)/\1 \2 \3 Get Port Power Limit, Port \2/g
s/\(02 .. 07 3D\) \(.*\)/\1 \2 Get System Status/g
s/\(02 .. 07 3d\) \(.*\)/\1 \2 Get System Status/g
s/\(02 .. 05 0E\) \(..\) \(.*\)/\1 \2 \3 Get Single Port Status, Port \2/g
s/\(02 .. 05 0e\) \(..\) \(.*\)/\1 \2 \3 Get Single Port Status, Port \2/g
s/\(02 .. 05 A8\) \(..\) \(.*\)/\1 \2 \3 Get Port Layer2 LLDP PSE Data, Port \2/g
s/\(02 .. 05 a8\) \(..\) \(.*\)/\1 \2 \3 Get Port Layer2 LLDP PSE Data, Port \2/g
s/\(02 .. 05 25\) \(..\) \(.*\)/\1 \2 \3 Get Port Measurement, Port \2/g
s/\(02 .. 07 0B 17\) \(.*\)/\1 \2 Get Power Supply Parameters/g
s/\(02 .. 07 0b 17\) \(.*\)/\1 \2 Get Power Supply Parameters/g
s/\(02 .. 07 0B 57\) \(..\) \(.*\)/\1 \2 \3 Get Power Banks, Bank \2/g
s/\(02 .. 07 0b 57\) \(..\) \(.*\)/\1 \2 \3 Get Power Banks, Bank \2/g
s/\(02 .. 07 0B AD\) \(.*\)/\1 \2 Get Derating System Measurements/g
s/\(02 .. 07 0b ad\) \(.*\)/\1 \2 Get Derating System Measurements/g
s/\(02 .. 07 0B 60\) \(.*\)/\1 \2 Get Total Power/g
s/\(02 .. 07 0b 60\) \(.*\)/\1 \2 Get Total Power/g
s/\(01 .. 06 0F\) \(.*\)/\1 \2 Save System Settings/g
s/\(01 .. 06 0f\) \(.*\)/\1 \2 Save System Settings/g
s/\(02 .. 05 C5\) \(..\) \(.*\)/\1 \2 \3 Get BT Port Measurements, Port \2/g
s/\(02 .. 05 c5\) \(..\) \(.*\)/\1 \2 \3 Get BT Port Measurements, Port \2/g
s/\(02 .. 05 C4\) \(..\) \(.*\)/\1 \2 \3 Get BT Port Class, Port \2/g
s/\(02 .. 05 c4\) \(..\) \(.*\)/\1 \2 \3 Get BT Port Class, Port \2/g
s/\(02 .. 05 C1\) \(..\) \(.*\)/\1 \2 \3 Get BT Port Status, Port \2/g
s/\(02 .. 05 c1\) \(..\) \(.*\)/\1 \2 \3 Get BT Port Status, Port \2/g
s/\(00 .. 05 C0\) \(..\) \(..\) \(..\) \(..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 \6 \7 \8 Set BT Port Parameters, Port \2 cfg1: \3 cf2: \4 opmode: \5 add: \6 prio: \7/g
s/\(00 .. 05 c0\) \(..\) \(..\) \(..\) \(..\) \(..\) \(..\) \(.*\)/\1 \2 \3 \4 \5 \6 \7 \8 Set BT Port Parameters, Port \2 cfg1: \3 cf2: \4 opmode: \5 add: \6 prio: \7/g



Then use linux command
sed -f pd69000.sed <i2c.log > i2c.txt

I2c.txt will then be an annotated verison of i2c.log
*/

//**********************************************************************
//  This file contains functions that are general for the PoE system.
//  Depending upon which PoE chip set the hardware uses each function
//  calls the corresponding function for that chip set.
//**********************************************************************
#include "critd_api.h"
#include "misc_api.h"    // I2C
#include "poe.h"
#include "poe_custom_api.h"
#include "vtss/appl/poe.h"
#include "poe_trace.h"
#include <microchip/ethernet/board/api/poe.h>
//#include <microchip/ethernet/board/api/poe_driver_pd69200.h>
#include <microchip/ethernet/board/api.h>
#include "port_api.h"
#include "poe_options_cfg.h"
#include "conf_api.h"
#include "control_api.h"

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

static BOOL init_done = FALSE;  // OK - That is not semaphore protect. Only set one place.
static mesa_port_list_t new_pd_detected;
static BOOL poe_status_polling = TRUE;
static uint32_t board_count; // number of PoE controller boards detected

#define POE_DETECTION_FAIL_RETRY_TIME_MS    15000

#define FLASH_CONF_POE_I2C0_TAG   "POE_I2C0"
#define FLASH_CONF_POE_I2C1_TAG   "POE_I2C1"

/****************************************************************************/
/*  Semaphore and trace                                                     */
/****************************************************************************/
static critd_t crit_custom;  // Critial region for the shared custom poe chipset.
#define POE_CUSTOM_CRIT_SCOPE() VtssPoECustomCritdGuard __lock_guard__(__LINE__)

struct VtssPoECustomCritdGuard {
    VtssPoECustomCritdGuard(int line) : line_(line)
    {
        critd_enter(&crit_custom, __FUNCTION__, line);
    }

    ~VtssPoECustomCritdGuard()
    {
        critd_exit(&crit_custom, __FUNCTION__, line_);
    }
    int line_;
};

#define POE_CUSTOM_CRIT_ENTER() critd_enter(&crit_custom, __FILE__, __LINE__)
#define POE_CUSTOM_CRIT_EXIT()  critd_exit( &crit_custom, __FILE__, __LINE__)


void poe_custom_init_chips(mesa_bool_t interruptible_power, int16_t restart_cause)
{
    // On some systems the address bus for the NOR flash only allows
    // addressing 16MB.  If running NOR-only a bigger NOR is needed
    // and in that case the remaining part of the NOR flash is read
    // using bit-banging. When that is done, interrupt latency on the
    // I2C bus may be too high for correct operation.  To avoid that,
    // ensure that software upgrade and PoE polling does not go on at
    // the same time.  While software upgrade is on-going,
    // control_system_flash is locked thereby preventing PoE polling.
    // Likewise, while doing a PoE poll, lock control_system_flash to
    // prevent software upgrade.
    control_system_flash_lock();
    POE_CUSTOM_CRIT_SCOPE();
    new_pd_detected.clear_all();
    if (MESA_RC_OK == meba_poe_chip_initialization(board_instance, interruptible_power, restart_cause)) {
        meba_poe_status_t status = {};
        (void)meba_poe_status_get(board_instance, &status);
        board_count = status.operational_controller_count;
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Found %d poe boards, chip_state %d version %s", board_count, status.chip_state, status.version);
    } else {
        T_DG(VTSS_TRACE_GRP_CUSTOM, "PoE chipset detection failed");
    }
    control_system_flash_unlock();
}

// Function that shall be called after reset / board boot.
mesa_rc poe_custom_init(meba_poe_init_params_t *ptMeba_poe_init_params)
{
    T_IG(VTSS_TRACE_GRP_CUSTOM, "Initializing");
    if (init_done) {
        T_IG(VTSS_TRACE_GRP_CUSTOM, "PoE init already done");
        return MESA_RC_OK;
    }

    T_IG(VTSS_TRACE_GRP_CUSTOM, "Doing PoE init");
    critd_init(&crit_custom, "PoE custom", VTSS_MODULE_ID_POE, CRITD_TYPE_MUTEX);

    // It takes a loooong time to download and program firmware, so extend mutex timeout to 5 minutes
    crit_custom.max_lock_time = 300;

    if (!fast_cap(MEBA_CAP_POE)) {
        T_I("PoE not supported");
        return MESA_RC_ERROR;
    }

    const char *cur_value1 = conf_mgmt_board_tag_get(FLASH_CONF_POE_I2C0_TAG);
    if (cur_value1) {
        // here is the hex string converted to hex
        int num = (int)strtol(cur_value1, NULL, 16);
        if (num != 0) {
            board_instance->poe_i2c_tags.poe_12c0 = num;
            T_I("PoE controller #0 using I2C address Tag (%s=%s)", FLASH_CONF_POE_I2C0_TAG, cur_value1);
        }
    } else {
        board_instance->poe_i2c_tags.poe_12c0 = 0;
    }

    const char *cur_value2 = conf_mgmt_board_tag_get(FLASH_CONF_POE_I2C1_TAG);
    if (cur_value2) {
        // here is the hex string  converted to hex
        int num = (int)strtol(cur_value2, NULL, 16);
        if (num != 0) {
            board_instance->poe_i2c_tags.poe_12c1 = num;
            T_I("PoE controller #1 using I2C address Tag (%s=%s)", FLASH_CONF_POE_I2C1_TAG, cur_value2);
        }
    } else {
        board_instance->poe_i2c_tags.poe_12c1 = 0;
    }

    // init varibles
    if (meba_poe_system_initialize(board_instance, ptMeba_poe_init_params) != MESA_RC_OK) {
        T_E("Failed to initialize PoE");
        return MESA_RC_ERROR;
    }

    mesa_rc rc = MESA_RC_OK;
    mesa_bool_t report_once_no_poe_controller_found = true;

    // detect poe hw
    do {
        rc = meba_poe_do_detection(board_instance);
        if (rc != MESA_RC_OK) { // Detecting PoE chip set. Has to be called even on PoE less boards to initialise the PoE crit.
            if (report_once_no_poe_controller_found == TRUE) {
                report_once_no_poe_controller_found = FALSE;
#if defined(VTSS_SW_OPTION_SYSLOG)
                //S_I("PoE-Control: No poe controller was found.");
                poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_CONTROLLER_NOT_FOUND, NULL);
#endif  // VTSS_SW_OPTION_SYSLOG
                //thread_lock.wait();
            }

            T_DG(VTSS_TRACE_GRP_CUSTOM, "PoE chipset detection failed");
            // No chip found - retry delay
            VTSS_OS_MSLEEP(POE_DETECTION_FAIL_RETRY_TIME_MS);
        }
    } while (rc != MESA_RC_OK);

    // get poe global ststus
    meba_poe_status_t status = {};
    control_system_flash_lock();
    if (MESA_RC_OK != meba_poe_status_get(board_instance, &status)) {
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Do chip sync");
        (void)meba_poe_sync(board_instance);
    } else {
        board_count = status.operational_controller_count;
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Found %d poe boards, chip_state %d version %s", board_count, status.chip_state, status.version);
    }
    control_system_flash_unlock();

    init_done = TRUE;
    T_IG(VTSS_TRACE_GRP_CUSTOM, "PoE init done");

    int total_port_count = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);
    T_D("total_port_count=%d", total_port_count);

    return MESA_RC_OK;
}

void poe_status_polling_disable(BOOL disable)
{
    poe_status_polling = !disable;
}

static meba_poe_pd_power_priority_t vtss2meba_poe_port_priority(vtss_appl_poe_port_power_priority_t priority)
{
    meba_poe_pd_power_priority_t meba_prio;
    switch (priority) {
    /** Least port power priority. */
    case VTSS_APPL_POE_PORT_POWER_PRIORITY_LOW:
        meba_prio = MEBA_POE_PORT_PD_POWER_PRIORITY_LOW;
        break;
    /** Medium port power priority. */
    case VTSS_APPL_POE_PORT_POWER_PRIORITY_HIGH:
        meba_prio = MEBA_POE_PORT_PD_POWER_PRIORITY_HIGH;
        break;
    /** Highest port power priority. */
    case VTSS_APPL_POE_PORT_POWER_PRIORITY_CRITICAL:
        meba_prio = MEBA_POE_PORT_PD_POWER_PRIORITY_CRITICAL;
        break;
    default:
        meba_prio = MEBA_POE_PORT_PD_POWER_PRIORITY_LOW;
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Unknown priority %u. Returning Low priority\n", priority);
    }
    return meba_prio;
}



static meba_poe_port_type_t vtss2meba_poe_port_type(vtss_appl_poebt_port_type_t bt_pse_port_type)
{
    meba_poe_port_type_t meba_type;
    switch (bt_pse_port_type) {
    /** Least port power priority. */
    case VTSS_APPL_POE_PSE_PORT_TYPE3_15W:
        meba_type = MEBA_POE_PSE_PORT_TYPE3_15W;
        break;
    /** Medium port power priority. */
    case VTSS_APPL_POE_PSE_PORT_TYPE3_30W:
        meba_type = MEBA_POE_PSE_PORT_TYPE3_30W;
        break;
    /** Highest port power priority. */
    case VTSS_APPL_POE_PSE_PORT_TYPE3_60W:
        meba_type = MEBA_POE_PSE_PORT_TYPE3_60W;
        break;
    /** Highest port power priority. */
    case VTSS_APPL_POE_PSE_PORT_TYPE4_90W:
        meba_type = MEBA_POE_PSE_PORT_TYPE4_90W;
        break;
    default:
        meba_type = MEBA_POE_PSE_PORT_TYPE4_90W;
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Unknown pse port type %u. Returning type 4\n", bt_pse_port_type);
    }
    return meba_type;
}



static meba_poe_port_cable_length_t vtss2meba_poe_cable_length(vtss_appl_poe_port_cable_length_t cable_length)
{
    meba_poe_port_cable_length_t meba_cable_length;
    switch (cable_length) {
    /** Least port power priority. */
    case VTSS_APPL_POE_PORT_CABLE_LENGTH_10:
        meba_cable_length = MEBA_POE_PORT_CABLE_LENGTH_10;
        break;
    /** Medium port power priority. */
    case VTSS_APPL_POE_PORT_CABLE_LENGTH_30:
        meba_cable_length = MEBA_POE_PORT_CABLE_LENGTH_30;
        break;
    /** Highest port power priority. */
    case VTSS_APPL_POE_PORT_CABLE_LENGTH_60:
        meba_cable_length = MEBA_POE_PORT_CABLE_LENGTH_60;
        break;
    /** Highest port power priority. */
    case VTSS_APPL_POE_PORT_CABLE_LENGTH_100:
        meba_cable_length = MEBA_POE_PORT_CABLE_LENGTH_100;
        break;
    default:
        meba_cable_length = MEBA_POE_PORT_CABLE_LENGTH_100;
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Unknown cable length %u. Returning cable length 100\n", cable_length);
    }
    return meba_cable_length;
}



static meba_poe_bt_port_pm_mode_t vtss2meba_poe_bt_port_pm_mode(vtss_appl_poebt_port_pm_t bt_port_pm_mode)
{
    meba_poe_bt_port_pm_mode_t meba_bt_port_pm_mode;
    switch (bt_port_pm_mode) {
    // ** bt_port_pm_mode dynamic. *
    case VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC:
        meba_bt_port_pm_mode = MEBA_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC;
        break;
    // ** bt_port_pm_mode static. *
    case VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_STATIC:
        meba_bt_port_pm_mode = MEBA_POE_BT_PORT_POWER_MANAGEMENT_STATIC;
        break;
    // ** dynamic for non lldp cdp autoclass ports. *
    case VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_HYBRID:
        meba_bt_port_pm_mode = MEBA_POE_BT_PORT_POWER_MANAGEMENT_HYBRID;
        break;
    default:
        meba_bt_port_pm_mode = MEBA_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC;
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Unknown bt_port_pm_mode %u. Returning dynamic power management mode\n", bt_port_pm_mode);
    }
    return meba_bt_port_pm_mode;
}



static vtss_appl_poebt_port_type_t meba2vtss_poe_port_type(meba_poe_port_type_t meba_pse_port_type)
{
    vtss_appl_poebt_port_type_t appl_type;
    switch (meba_pse_port_type) {
    /** Least port power priority. */
    case MEBA_POE_PSE_PORT_TYPE3_15W:
        appl_type = VTSS_APPL_POE_PSE_PORT_TYPE3_15W;
        break;
    /** Medium port power priority. */
    case MEBA_POE_PSE_PORT_TYPE3_30W:
        appl_type = VTSS_APPL_POE_PSE_PORT_TYPE3_30W;
        break;
    /** Highest port power priority. */
    case MEBA_POE_PSE_PORT_TYPE3_60W:
        appl_type = VTSS_APPL_POE_PSE_PORT_TYPE3_60W;
        break;
    /** Highest port power priority. */
    case MEBA_POE_PSE_PORT_TYPE4_90W:
        appl_type = VTSS_APPL_POE_PSE_PORT_TYPE4_90W;
        break;
    default:
        appl_type = VTSS_APPL_POE_PSE_PORT_TYPE4_90W;
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Unknown pse port meba type %u. Returning type 4\n", meba_pse_port_type);
    }
    return appl_type;
}




static vtss_appl_poebt_port_pm_t meba2vtss_poe_bt_port_pm_mode(meba_poe_bt_port_pm_mode_t meba_bt_port_pm_mode)
{
    vtss_appl_poebt_port_pm_t appl_bt_port_pm_mode;
    switch (meba_bt_port_pm_mode) {
    // ** bt_port_pm_mode dynamic. *
    case MEBA_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC:
        appl_bt_port_pm_mode = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC;
        break;
    // ** bt_port_pm_mode static. *
    case MEBA_POE_BT_PORT_POWER_MANAGEMENT_STATIC:
        appl_bt_port_pm_mode = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_STATIC;
        break;
    // ** dynamic for non lldp cdp autoclass ports. *
    case MEBA_POE_BT_PORT_POWER_MANAGEMENT_HYBRID:
        appl_bt_port_pm_mode = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_HYBRID;
        break;
    default:
        appl_bt_port_pm_mode = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC;
        T_DG(VTSS_TRACE_GRP_CUSTOM, "Unknown meba bt_port_pm_mode %u. Returning dynamic power management mode\n", meba_bt_port_pm_mode);
    }
    return appl_bt_port_pm_mode;
}



// Debug function for updating the PoE chipset firmware - For MSCC PoE chips only
static mesa_rc poe_download_firmware(const char *firmware, size_t firmware_size)
{
    T_IG(VTSS_TRACE_GRP_CUSTOM, "Enter");
    // only upgrade firmware if firmware is different from the currently loaded
    int count = meba_poe_firmware_upgrade(board_instance, TRUE, firmware_size, firmware);

    T_IG(VTSS_TRACE_GRP_CUSTOM, "Exit");
    T_D("%s(), count=%d \n\r", __func__, count);
    return (count > 0) ? MESA_RC_OK : MESA_RC_ERROR;
}

// Provide a locked version of prepare firmware upgrade
static mesa_rc poe_custom_prepare_firmware_upgrade(const char *firmware, size_t firmware_size)
{
    POE_CUSTOM_CRIT_SCOPE();
    return meba_poe_prepare_firmware_upgrade(board_instance, TRUE, firmware_size, firmware);
}


#define MAX_RETRIES    5

// Debug function for updating the PoE chipset firmware
mesa_rc poe_mgmt_firmware_update(const char *firmware, size_t firmware_size)
{
    meba_poe_status_t meba_poe_status = {};
    mesa_rc rc = VTSS_RC_OK;
    T_IG(VTSS_TRACE_GRP_CUSTOM, "Enter");
    vtss_tick_count_t start_time = vtss_current_time();
    int iCount = 0;

    while (((rc = poe_custom_prepare_firmware_upgrade(firmware, firmware_size)) == MESA_RC_ERR_POE_FIRM_UPDATE_NEEDED)
           && (iCount < MAX_RETRIES)) {
        T_I("poe_custom_prepare_firmware_upgrade rc=%d", rc);

#ifdef VTSS_SW_OPTION_SYSLOG
        //S_I("PoE-Control: Firmware upgrade started");
        uPOE_SYSLOG_INFO uSysInfo;
        uSysInfo.poe_firmware_update_t.poe_mcu_index  = 0;
        uSysInfo.poe_firmware_update_t.firmware_status = "started";
        poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_FIRMWARE_UPDATE, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */

        rc = poe_download_firmware(firmware, firmware_size);
        T_D("poe_download_firmware rc=%d", rc);

        switch (rc) {
        case VTSS_RC_OK:
            T_I("PoE-Control: Firmware upgrade done");
#ifdef VTSS_SW_OPTION_SYSLOG
            //S_I("PoE-Control: Firmware upgrade done");
            uSysInfo.poe_firmware_update_t.poe_mcu_index  = 0;
            uSysInfo.poe_firmware_update_t.firmware_status = "done";
            poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_FIRMWARE_UPDATE, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */
            break;
        default:
            T_I("PoE-Control: Firmware upgrade failed");
#ifdef VTSS_SW_OPTION_SYSLOG
            //S_I("PoE-Control: Firmware upgrade failed");
            uSysInfo.poe_firmware_update_t.poe_mcu_index = 0;
            uSysInfo.poe_firmware_update_t.firmware_status = "failed";
            poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_FIRMWARE_UPDATE, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */
            break;
        }

        // verify that poe firmware is up to date and update poe firmware info
        if (poe_custom_prepare_firmware_upgrade(firmware, firmware_size) == MESA_RC_ERR_POE_FIRMWARE_IS_UP_TO_DATE) {
            {
                //read meba_poe_status - the poe firmware info that was already updated by 'poe_custom_prepare_firmware_upgrade'
                control_system_flash_lock();
                POE_CUSTOM_CRIT_SCOPE();
                rc = meba_poe_status_get(board_instance, &meba_poe_status);
                control_system_flash_unlock();
                if (rc != MESA_RC_OK) {
                    T_I("meba_poe_status_get exit rc= %d", rc);
                    return rc;
                }
            }

            iCount++;
        }
    }

    if ((iCount == 0) && (rc == MESA_RC_ERR_POE_FIRMWARE_IS_UP_TO_DATE)) {
        T_I("PoE-Control: PoE Firmware is up to date");
    } else if ((iCount > 0) && (iCount < 5) && (rc == MESA_RC_ERR_POE_FIRM_UPDATE_NEEDED)) {
        T_I("PoE-Control: PoE Firmware update operation succeed");
    } else {
        T_I("PoE-Control: PoE Firmware update operation failed");
    }

    T_IG(VTSS_TRACE_GRP_CUSTOM, "Exit (%d seconds)", VTSS_OS_TICK2MSEC(vtss_current_time() - start_time) / 1000);

    if (iCount >= MAX_RETRIES) {
        return VTSS_RC_ERROR;
    }

    return rc;
}

// Returns Maximum power supported for a port (in deciWatts)
u16 poe_custom_get_port_power_max(mesa_port_no_t iport)
{
    return 300; // PoE+ maximum power.
}


// Function that return TRUE if backup power supply is supported.
BOOL poe_custom_is_backup_power_supported (void)
{
    return FALSE;
}

static
vtss_appl_poe_pd_structure_t poe_meba2appl_pd_structure(
    meba_poe_port_pd_structure_t meba_pd_structure)
{
    switch (meba_pd_structure) {
    case MEBA_POE_PORT_PD_STRUCTURE_NOT_PERFORMED:
        return VTSS_APPL_POE_PD_STRUCTURE_NOT_PERFORMED;
    case MEBA_POE_PORT_PD_STRUCTURE_OPEN:
        return VTSS_APPL_POE_PD_STRUCTURE_OPEN;
    case MEBA_POE_PORT_PD_STRUCTURE_INVALID_SIGNATURE:
        return VTSS_APPL_POE_PD_STRUCTURE_INVALID_SIGNATURE;
    case MEBA_POE_PORT_PD_STRUCTURE_4P_SINGLE_IEEE:
        return VTSS_APPL_POE_PD_STRUCTURE_4P_SINGLE_IEEE;
    case MEBA_POE_PORT_PD_STRUCTURE_4P_SINGLE_LEGACY:
        return VTSS_APPL_POE_PD_STRUCTURE_4P_SINGLE_LEGACY;
    case MEBA_POE_PORT_PD_STRUCTURE_4P_DUAL_IEEE:
        return VTSS_APPL_POE_PD_STRUCTURE_4P_DUAL_IEEE;
    case MEBA_POE_PORT_PD_STRUCTURE_2P_DUAL_4P_CANDIDATE_FALSE:
        return VTSS_APPL_POE_PD_STRUCTURE_2P_DUAL_4P_CANDIDATE_FALSE;
    case MEBA_POE_PORT_PD_STRUCTURE_2P_IEEE:
        return VTSS_APPL_POE_PD_STRUCTURE_2P_IEEE;
    case MEBA_POE_PORT_PD_STRUCTURE_2P_LEGACY:
        return VTSS_APPL_POE_PD_STRUCTURE_2P_LEGACY;
    }
    return VTSS_APPL_POE_PD_STRUCTURE_INVALID_SIGNATURE;
}

// Function for updating the hardware with the current configuration.
static
void update_port_registers(poe_conf_t *new_conf, mesa_port_no_t iport, mesa_bool_t is_shutdown)
{
    T_D("Update chip port registers port %d shutdown %u", iport, is_shutdown);

    meba_poe_port_cfg_t port_cfg;

    port_cfg.enable                       = (is_shutdown) ? 0 : (new_conf->poe_mode[iport] != VTSS_APPL_POE_MODE_DISABLED) ? 1 : 0;
    port_cfg.ignore_pd_auto_class_request = (new_conf->pd_auto_class_request) ? 0 : 1;
    port_cfg.bt_port_pm_mode              = vtss2meba_poe_bt_port_pm_mode(new_conf->bt_port_pm_mode[iport]);
    port_cfg.bPoe_plus_mode               = (new_conf->poe_mode[iport] == VTSS_APPL_POE_MODE_POE_PLUS);
    port_cfg.priority                     = vtss2meba_poe_port_priority(new_conf->priority[iport]);
    port_cfg.bt_pse_port_type             = vtss2meba_poe_port_type(new_conf->bt_pse_port_type[iport]);
    port_cfg.cable_length                 = vtss2meba_poe_cable_length(new_conf->cable_length[iport]);

    POE_CUSTOM_CRIT_SCOPE();
    meba_poe_port_cfg_set(board_instance, iport, &port_cfg);

    T_D("Exit Update chip port registers");
}



static mesa_bool_t isFirstEntry = TRUE;


// Get status ( Real time current and power cunsumption )
mesa_rc poe_custom_get_status(poe_status_t *poe_status)
{
    mesa_rc rc;
    meba_poe_status_t meba_poe_status = {};
    vtss_appl_port_conf_t   conf;

    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t vtss_appl_port_status;

    if (!poe_status_polling) {
        return MESA_RC_OK;
    }

    {
        control_system_flash_lock();
        POE_CUSTOM_CRIT_SCOPE();
        rc = meba_poe_status_get(board_instance, &meba_poe_status);
        control_system_flash_unlock();
        if (rc != MESA_RC_OK) {
            T_E("meba_poe_status_get error code= %d", board_count);
            poe_status->is_chip_state_ok    = false;
            strcpy(poe_status->version, "-");
        } else {
            strncpy(poe_status->version, meba_poe_status.version, sizeof(poe_status->version));
        }
    }

    if (meba_poe_status.poe_power_source == MEBA_POE_POWER_SOURCE_BACKUP) {
        poe_status->pwr_in_status1 = FALSE;
        poe_status->pwr_in_status2 = TRUE;
    } else {
        poe_status->pwr_in_status1 = TRUE;
        poe_status->pwr_in_status2 = FALSE;
    }

    poe_status->is_bt                      = meba_poe_status.is_bt;
    poe_status->tPoe_individual_mask_info  = meba_poe_status.tPoe_individual_mask_info;

    poe_status->i2c_tx_error_counter   = meba_poe_status.i2c_tx_error_counter;

    // poe firmware info
    poe_status->prod_number_detected   = meba_poe_status.prod_number_detected;

    poe_status->sw_version_detected      = meba_poe_status.sw_version_detected;
    poe_status->sw_version_high_detected = meba_poe_status.sw_version_high_detected;
    poe_status->sw_version_low_detected  = meba_poe_status.sw_version_low_detected;

    poe_status->param_number_detected  = meba_poe_status.param_number_detected;
    poe_status->prod_number_from_file  = meba_poe_status.prod_number_from_file;
    poe_status->sw_version_from_file   = meba_poe_status.sw_version_from_file;
    poe_status->param_number_from_file = meba_poe_status.param_number_from_file;

    poe_status->build_number           = meba_poe_status.build_number ;
    poe_status->internal_sw_number     = meba_poe_status.internal_sw_number;
    poe_status->asic_patch_number      = meba_poe_status.asic_patch_number ;

    // telemetry info from PoE MCU
    poe_status->power_consumption_w  = meba_poe_status.power_consumption_w;
    poe_status->calculated_power_w   = meba_poe_status.calculated_power_w;
    poe_status->available_power_w    = meba_poe_status.available_power_w;
    poe_status->power_limit_w        = meba_poe_status.power_limit_w;
    poe_status->power_bank           = meba_poe_status.power_bank;
    poe_status->vmain_voltage_dv     = meba_poe_status.vmain_voltage_dv;
    poe_status->imain_current_ma     = meba_poe_status.imain_current_ma;
    poe_status->ePoE_Controller_Type = (vtss_appl_poe_controller_type_t) meba_poe_status.ePoE_Controller_Type;
    poe_status->vmain_out_of_range   = meba_poe_status.vmain_out_of_range;
    poe_status->is_chip_state_ok     = (meba_poe_status.chip_state == MEBA_POE_NO_CHIPSET_FOUND) ? false : true ;
    poe_status->max_number_of_poe_ports = meba_poe_status.max_number_of_poe_ports;

    T_D("adc_value: %d", meba_poe_status.adc_value);

    int total_port_count = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);
    T_N("total_port_count=%d, max poe ports=%d", total_port_count, poe_status->max_number_of_poe_ports);

    for (mesa_port_no_t iport = 0 ; iport < total_port_count; iport++) {
        int rc_ok = 1;

        poe_conf_t local_poe_conf = {};
        poe_config_get(&local_poe_conf);
        if (poe_config_updated()) {
            rc = poe_custom_cfg_set(&local_poe_conf);
            if (rc != VTSS_RC_OK) {
                T_D("poe_custom_cfg_set faild, rc= %d", rc);
                rc_ok = 0;
                //return rc;
            }
        }

        (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, iport, &ifindex);

        // get port link status and configuration
        if (vtss_appl_port_conf_get(ifindex, &conf)          != VTSS_RC_OK ||
            vtss_appl_port_status_get(ifindex, &vtss_appl_port_status) != VTSS_RC_OK) {
            T_E("Could not get port configuration or port status");
            rc_ok = 0;
            //break;
        }

        // sync configuration only after we already read all ststus block from PoE
        if (!isFirstEntry) {
            // We can not have update_port_registers depend on poe_config_updated() because
            // the shutdown state of ports from the port module also decides if the poe port
            // should be disabled. To ensure responsiveness, we iterate all ports
            if ((rc_ok == 1) && (iport == 0)) {
                for (mesa_port_no_t jport = 0; jport < total_port_count; jport++) {
                    update_port_registers(&local_poe_conf, jport, is_port_shutdown(jport));
                }
            }
        }

        meba_poe_port_status_t meba_poe_port_status = {};
        vtss_appl_poe_port_status_t &port_status = poe_status->port_status[iport];

        control_system_flash_lock();
        POE_CUSTOM_CRIT_SCOPE();
        rc = meba_poe_port_status_get(board_instance, iport, &meba_poe_port_status);   // get single port status
        control_system_flash_unlock();
        if ((MESA_RC_OK == rc) && (rc_ok == 1)) {
            port_status.cfg_pse_prebt_port_type    = (vtss_appl_poe_port_pse_prebt_port_type_t) meba_poe_port_status.port_type_prebt_af_at_poh;
            port_status.cfg_pse_port_type          = fast_cap(MEBA_CAP_POE_BT) ? meba2vtss_poe_port_type(meba_poe_port_status.bt_pse_port_type) : VTSS_APPL_POE_PSE_PORT_TYPE3_30W;
            port_status.pd_status_internal         = meba_poe_port_status.poe_internal_port_status;
            port_status.pd_type_sspd_dspd          = meba_poe_port_status.pd_type_sspd_dspd;
            port_status.cfg_bt_port_operation_mode = meba_poe_port_status.bt_port_operation_mode;
            port_status.cfg_bt_port_pm_mode        = meba2vtss_poe_bt_port_pm_mode(meba_poe_port_status.bt_port_pm_mode);

            strcpy(port_status.pd_status_internal_description, meba_poe_port_status.poe_port_status_description);

            port_status.power_requested_mw = meba_poe_port_status.power_requested_mw;
            port_status.power_assigned_mw  = meba_poe_port_status.power_assigned_mw;

            port_status.pd_status = (vtss_appl_poe_status_t) meba_poe_port_status.meba_poe_port_state;

            if (meba_poe_port_status.meba_poe_port_state == MEBA_POE_PD_FAULT) {
                if (meba_poe_port_status.is_fault_link_without_power && vtss_appl_port_status.link) {
                    port_status.pd_status = VTSS_APPL_POE_NO_PD_DETECTED;
                } else {
                    port_status.pd_status = VTSS_APPL_POE_PD_FAULT;
                }
            }

            // error counters
            port_status.bt_port_counters.udl_count               += meba_poe_port_status.bt_port_counters.udl_count;
            port_status.bt_port_counters.ovl_count               += meba_poe_port_status.bt_port_counters.ovl_count;
            port_status.bt_port_counters.sc_count                += meba_poe_port_status.bt_port_counters.sc_count;
            port_status.bt_port_counters.power_denied_count      += meba_poe_port_status.bt_port_counters.power_denied_count;

            if (!vtss_appl_port_status.link) {
                port_status.bt_port_counters.invalid_signature_count += meba_poe_port_status.bt_port_counters.invalid_signature_count;
            }

            // conf.admin.enable ? (port_status.link)

            int new_measured_pd_class_a;
            int new_measured_pd_class_b;
            int new_requested_pd_class_a;
            int new_requested_pd_class_b;
            int new_assigned_pd_class_a;
            int new_assigned_pd_class_b;

            meba_poe_port_pd_structure_t new_pd_structure;

            if (port_status.pd_status == VTSS_APPL_POE_NOT_SUPPORTED ||
                port_status.pd_status == VTSS_APPL_POE_NO_PD_DETECTED ||
                port_status.pd_status == VTSS_APPL_POE_DISABLED ||
                port_status.pd_status == VTSS_APPL_POE_UNKNOWN_STATE) {

                port_status.power_consume_mw = 0;
                port_status.power_reserved_dw = 0;
                port_status.current_consume_ma = 0;
                new_measured_pd_class_a = POE_UNDETERMINED_CLASS;
                new_measured_pd_class_b = POE_UNDETERMINED_CLASS;
                new_requested_pd_class_a = POE_UNDETERMINED_CLASS;
                new_requested_pd_class_b = POE_UNDETERMINED_CLASS;
                new_assigned_pd_class_a = POE_UNDETERMINED_CLASS;
                new_assigned_pd_class_b = POE_UNDETERMINED_CLASS;
                new_pd_structure = MEBA_POE_PORT_PD_STRUCTURE_NOT_PERFORMED;
            } else {
                port_status.power_consume_mw = meba_poe_port_status.power_mw;
                port_status.power_reserved_dw = meba_poe_port_status.reserved_power_mw / 100;  // Convert from milliwatt to deciwatt
                port_status.current_consume_ma = meba_poe_port_status.current_ma;

                new_measured_pd_class_a = meba_poe_port_status.measured_pd_class_a;;
                new_measured_pd_class_b = meba_poe_port_status.measured_pd_class_b;
                new_requested_pd_class_a = meba_poe_port_status.requested_pd_class_a;
                new_requested_pd_class_b = meba_poe_port_status.requested_pd_class_b;
                new_assigned_pd_class_a = meba_poe_port_status.assigned_pd_class_a;
                new_assigned_pd_class_b = meba_poe_port_status.assigned_pd_class_b;
                new_pd_structure = meba_poe_port_status.pd_structure;
            }

            port_status.measured_pd_class_a  = new_measured_pd_class_a;
            port_status.measured_pd_class_b  = new_measured_pd_class_b;
            port_status.requested_pd_class_a = new_requested_pd_class_a;
            port_status.requested_pd_class_b = new_requested_pd_class_b;
            port_status.assigned_pd_class_a  = new_assigned_pd_class_a;
            port_status.assigned_pd_class_b  = new_assigned_pd_class_b;

            port_status.pd_structure = poe_meba2appl_pd_structure(new_pd_structure);
            poe_status->poe_chip_found[iport] = meba_poe_port_status.chip_state;
        } else {
            if (rc == MESA_RC_ERR_NOT_POE_PORT_ERR) { //invalid handle - out of scope or not poe port
                //T_I("port: %d , MESA_RC_ERR_PARM", iport);
                port_status.pd_status = VTSS_APPL_POE_NOT_SUPPORTED;
            } else {
                //T_I("port: %d , VTSS_APPL_POE_UNKNOWN_STATE: %d",iport,rc);
                port_status.pd_status = VTSS_APPL_POE_UNKNOWN_STATE;
            }

            port_status.pd_type_sspd_dspd = 0;

            port_status.power_reserved_dw = 0;
            port_status.power_requested_mw = 0;
            port_status.power_consume_mw = 0;
            port_status.current_consume_ma = 0;
            port_status.assigned_pd_class_a = POE_UNDETERMINED_CLASS;  // Note: value 0:class=0, value 1:class=1, etc.
            port_status.assigned_pd_class_b = POE_UNDETERMINED_CLASS;  // Note: value 0:class=0, value 1:class=1, etc.
            port_status.pd_structure = VTSS_APPL_POE_PD_STRUCTURE_NOT_PERFORMED;
            poe_status->poe_chip_found[iport] = MEBA_POE_NO_CHIPSET_FOUND;
            port_status.pd_status_internal    = 0x37;  //Unknown device port status.
            strcpy(port_status.pd_status_internal_description, "Unknown");
        }

        poe_status->poe_pse_data[iport].pse_allocated_power_dw    = meba_poe_port_status.pse_data.pse_allocated_power_mw / 100;    // mW -> dW
        poe_status->poe_pse_data[iport].pd_requested_power_dw     = meba_poe_port_status.pse_data.pd_requested_power_mw / 100;     // mW -> dW
        poe_status->poe_pse_data[iport].pse_power_type            = meba_poe_port_status.pse_data.pse_power_type;
        poe_status->poe_pse_data[iport].power_class               = meba_poe_port_status.pse_data.power_class;  // Note: value 1:class=0. value 2:class=1, etc.
        poe_status->poe_pse_data[iport].pse_power_pair            = meba_poe_port_status.pse_data.pse_power_pair;
        poe_status->poe_pse_data[iport].cable_len                 = meba_poe_port_status.pse_data.cable_len;
        poe_status->poe_pse_data[iport].layer2_execution_status   = meba_poe_port_status.pse_data.layer2_execution_status;
        poe_status->poe_pse_data[iport].port_type_prebt_af_at_poh = meba_poe_port_status.pse_data.port_type_prebt_af_at_poh;


        poe_status->poe_pse_data[iport].requested_power_mode_a_dw  = meba_poe_port_status.pse_data.requested_power_mode_a_mw / 100;
        poe_status->poe_pse_data[iport].requested_power_mode_b_dw  = meba_poe_port_status.pse_data.requested_power_mode_b_mw / 100;
        poe_status->poe_pse_data[iport].pse_alloc_power_alt_a_dw   = meba_poe_port_status.pse_data.pse_alloc_power_alt_a_mw / 100;
        poe_status->poe_pse_data[iport].pse_alloc_power_alt_b_dw   = meba_poe_port_status.pse_data.pse_alloc_power_alt_b_mw / 100;
        poe_status->poe_pse_data[iport].power_status               = meba_poe_port_status.pse_data.power_status;
        poe_status->poe_pse_data[iport].system_setup               = meba_poe_port_status.system_setup;
        poe_status->poe_pse_data[iport].pse_maximum_avail_power_dw = meba_poe_port_status.pse_data.pse_max_avail_power_mw / 100;
        poe_status->poe_pse_data[iport].auto_class                 = meba_poe_port_status.auto_class;
    }

    isFirstEntry = FALSE; // set flag as done - only after reading poe ports status
    return MESA_RC_OK;
}

void poe_custom_pd_data_set(mesa_port_no_t iport, poe_pd_data_t *pd_data)
{
    meba_poe_pd_data_t meba_pd_data = {};
    // The pd_data->type is 8 bit, with the encoding described in IEEE802.3-2015 table 79.4
    // Here we map these bits to enumerated parameters to meba layer.

    switch ( 0x3 & (pd_data->type >> 6) ) { // switch on bit 7:6
    case 0:
        meba_pd_data.type = MEBA_POE_PORT_PD_POWER_TYPE2_PSE;
        break;
    case 1:
        meba_pd_data.type = MEBA_POE_PORT_PD_POWER_TYPE2_PD;
        break;
    case 2:
        meba_pd_data.type = MEBA_POE_PORT_PD_POWER_TYPE1_PSE;
        break;
    case 3:
        meba_pd_data.type = MEBA_POE_PORT_PD_POWER_TYPE1_PD;
        break;
    }

    if ( meba_pd_data.type == MEBA_POE_PORT_PD_POWER_TYPE2_PD ||
         meba_pd_data.type == MEBA_POE_PORT_PD_POWER_TYPE1_PD ) {
        // This is a PD
        switch ( 0x3 & (pd_data->type >> 4) ) { // switch on bit 5:4
        case 0:
            meba_pd_data.source = MEBA_POE_PORT_PD_PD_POWER_SOURCE_UNKNOWN;
            break;
        case 1:
            meba_pd_data.source = MEBA_POE_PORT_PD_PD_POWER_SOURCE_PSE;
            break;
        case 2:
            meba_pd_data.source =  MEBA_POE_PORT_PD_PD_POWER_SOURCE_RESERVED;
            break;
        case 3:
            meba_pd_data.source = MEBA_POE_PORT_PD_PD_POWER_SOURCE_PSE_LOCAL;
            break;
        }
    } else {
        // This is a PSE
        switch ( 0x3 & (pd_data->type >> 4) ) { // switch on bit 5:4
        case 0:
            meba_pd_data.source = MEBA_POE_PORT_PD_PSE_POWER_SOURCE_UNKNOWN;
            break;
        case 1:
            meba_pd_data.source = MEBA_POE_PORT_PD_PSE_POWER_SOURCE_PRIMARY;
            break;
        case 2:
            meba_pd_data.source =  MEBA_POE_PORT_PD_PSE_POWER_SOURCE_BACKUP;
            break;
        case 3:
            meba_pd_data.source = MEBA_POE_PORT_PD_PSE_POWER_SOURCE_RESERVED;
            break;
        }
    }

    // pd_data->type  bit 3:2 are reserved

    switch ( 0x3 & pd_data->type ) { // switch on bit 1:0
    case 0:
        meba_pd_data.prio = MEBA_POE_PORT_PD_POWER_PRIORITY_UNKNOWN;
        break;
    case 1:
        meba_pd_data.prio = MEBA_POE_PORT_PD_POWER_PRIORITY_CRITICAL;
        break;
    case 2:
        meba_pd_data.prio = MEBA_POE_PORT_PD_POWER_PRIORITY_HIGH;
        break;
    case 3:
        meba_pd_data.prio = MEBA_POE_PORT_PD_POWER_PRIORITY_LOW;
        break;
    }

    meba_pd_data.pd_requested_power_mw  = pd_data->pd_requested_power_dw * 100;     // convert from deciwatt to milliwatt
    meba_pd_data.pse_allocated_power_mw = pd_data->pse_allocated_power_dw * 100;    // convert from deciwatt to milliwatt

    POE_CUSTOM_CRIT_SCOPE();
    meba_poe_port_pd_data_set(board_instance, iport, &meba_pd_data);
}

void poe_custom_pd_bt_data_set(mesa_port_no_t iport, poe_pd_bt_data_t *pd_data)
{
    meba_poe_pd_bt_data_t meba_pd_data = {};
    // The pd_data->type is 8 bit, with the encoding described in IEEE802.3-2015 table 79.4
    // Here we map these bits to enumerated parameters to meba layer.

    switch ( 0x3 & (pd_data->type >> 6) ) { // switch on bit 7:6
    case 0:
        meba_pd_data.type = MEBA_POE_PORT_PD_POWER_TYPE2_PSE;
        break;
    case 1:
        meba_pd_data.type = MEBA_POE_PORT_PD_POWER_TYPE2_PD;
        break;
    case 2:
        meba_pd_data.type = MEBA_POE_PORT_PD_POWER_TYPE1_PSE;
        break;
    case 3:
        meba_pd_data.type = MEBA_POE_PORT_PD_POWER_TYPE1_PD;
        break;
    }

    if ( meba_pd_data.type == MEBA_POE_PORT_PD_POWER_TYPE2_PD ||
         meba_pd_data.type == MEBA_POE_PORT_PD_POWER_TYPE1_PD ) {
        // This is a PD
        switch ( 0x3 & (pd_data->type >> 4) ) { // switch on bit 5:4
        case 0:
            meba_pd_data.source = MEBA_POE_PORT_PD_PD_POWER_SOURCE_UNKNOWN;
            break;
        case 1:
            meba_pd_data.source = MEBA_POE_PORT_PD_PD_POWER_SOURCE_PSE;
            break;
        case 2:
            meba_pd_data.source =  MEBA_POE_PORT_PD_PD_POWER_SOURCE_RESERVED;
            break;
        case 3:
            meba_pd_data.source = MEBA_POE_PORT_PD_PD_POWER_SOURCE_PSE_LOCAL;
            break;
        }
    } else {
        // This is a PSE
        switch ( 0x3 & (pd_data->type >> 4) ) { // switch on bit 5:4
        case 0:
            meba_pd_data.source = MEBA_POE_PORT_PD_PSE_POWER_SOURCE_UNKNOWN;
            break;
        case 1:
            meba_pd_data.source = MEBA_POE_PORT_PD_PSE_POWER_SOURCE_PRIMARY;
            break;
        case 2:
            meba_pd_data.source =  MEBA_POE_PORT_PD_PSE_POWER_SOURCE_BACKUP;
            break;
        case 3:
            meba_pd_data.source = MEBA_POE_PORT_PD_PSE_POWER_SOURCE_RESERVED;
            break;
        }
    }

    // pd_data->type  bit 3:2 are reserved

    switch ( 0x3 & pd_data->type ) { // switch on bit 1:0
    case 0:
        meba_pd_data.prio = MEBA_POE_PORT_PD_POWER_PRIORITY_UNKNOWN;
        break;
    case 1:
        meba_pd_data.prio = MEBA_POE_PORT_PD_POWER_PRIORITY_CRITICAL;
        break;
    case 2:
        meba_pd_data.prio = MEBA_POE_PORT_PD_POWER_PRIORITY_HIGH;
        break;
    case 3:
        meba_pd_data.prio = MEBA_POE_PORT_PD_POWER_PRIORITY_LOW;
        break;
    }

    meba_pd_data.pd_requested_power_single_mw  = pd_data->pd_requested_power_dw * 100;     // convert from deciwatt to milliwatt
    meba_pd_data.pse_allocated_power_single_mw = pd_data->pse_allocated_power_dw * 100;    // convert from deciwatt to milliwatt
    meba_pd_data.pd_requested_power_alt_a_mw   = pd_data->requested_power_mode_a_dw * 100;
    meba_pd_data.pse_allocated_power_alt_a_mw  = pd_data->pse_alloc_power_alt_a_dw * 100;
    meba_pd_data.pd_requested_power_alt_b_mw   = pd_data->requested_power_mode_b_dw * 100;
    meba_pd_data.pse_allocated_power_alt_b_mw  = pd_data->pse_alloc_power_alt_b_dw * 100;

    POE_CUSTOM_CRIT_SCOPE();
    meba_poe_port_pd_bt_data_set(board_instance, iport, &meba_pd_data);
}

mesa_rc poe_custom_cfg_set(poe_conf_t *conf)
{
    meba_poe_global_cfg_t meba_cfg;

    if (conf->power_supply_max_power_w < conf->system_power_usage_w) {
        meba_cfg.power_supply_poe_limit_w = 0;
    } else {
        if (board_count <= 0) {
            T_E("board_count value: %d error", board_count);
            return VTSS_RC_ERROR;
        }

        meba_cfg.power_supply_poe_limit_w = (conf->power_supply_max_power_w - conf->system_power_usage_w) / board_count;
        T_I("meba_cfg.power_supply_poe_limit_w= %d, conf->power_supply_max_power_w= %d conf->system_power_usage_w= %d , board_count= %d", meba_cfg.power_supply_poe_limit_w, conf->power_supply_max_power_w, conf->system_power_usage_w, board_count);
    }
    T_D("update  meba_cfg.power_supply_poe_limit_w= %d", meba_cfg.power_supply_poe_limit_w);

    meba_cfg.legacy_detect                       = conf->cap_detect;
    meba_cfg.global_ignore_pd_auto_class_request = (conf->pd_auto_class_request) ? 0 : 1;  // checkbox and poe fields are opposites
    meba_cfg.global_legacy_pd_class_mode         = conf->global_legacy_pd_class_mode;
    POE_CUSTOM_CRIT_SCOPE();
    return meba_poe_cfg_set(board_instance, &meba_cfg);
}

BOOL poe_custom_new_pd_detected_set(mesa_port_no_t iport)
{
    T_NG_PORT(VTSS_TRACE_GRP_CUSTOM, iport, "Enter");
    POE_CUSTOM_CRIT_SCOPE();
    BOOL current_val = new_pd_detected[iport];
    new_pd_detected[iport] = TRUE;
    return current_val;
}

BOOL poe_custom_new_pd_detected_get(mesa_port_no_t iport, BOOL clear)
{
    T_NG_PORT(VTSS_TRACE_GRP_CUSTOM, iport, "Enter");
    POE_CUSTOM_CRIT_SCOPE();
    BOOL current_val = new_pd_detected[iport];
    if (clear) {
        new_pd_detected[iport] = FALSE;
    }
    return current_val;
}

//
// Debug functions
//

// Debug function for converting PoE chipset to a printable text string.
// In : poe_chipset - The PoE chipset type.
//      buf        - Pointer to a buffer that can contain the printable Poe chipset string.
char *poe_chipset2txt(meba_poe_chip_state_t poe_chip_state, char *buf)
{
    strcpy (buf, "-"); // Default to no chipset.

    if (poe_chip_state == MEBA_POE_CHIPSET_FOUND) {
        strcpy(buf, "MicroSemi PD69200");
    }

    return buf;
}


// For doing PD69200 access.
void poe_debug_access(mesa_port_no_t iport, char *var, uint32_t str_len, char *title, char *tx_str, char *rx_str, char *msg, int max_msg_len)
{
    POE_CUSTOM_CRIT_SCOPE();

    if (MESA_RC_OK != meba_poe_debug(board_instance, iport, var, str_len, title, tx_str, rx_str, msg, max_msg_len)) {
        T_IG(VTSS_TRACE_GRP_CUSTOM, "Debug pd69200 access failed");
    }
}

void poe_reset_command()
{
    POE_CUSTOM_CRIT_SCOPE();
    meba_poe_reset_command(board_instance);
}

void poe_save_command()
{
    POE_CUSTOM_CRIT_SCOPE();
    meba_poe_save_command(board_instance);
}

// Function for getting firmware information
// In: length of output buffer
// Out : info_txt - Pointer to printable string with firmware information
char *poe_custom_firmware_info_get(uint32_t max_size, char *info)
{
    POE_CUSTOM_CRIT_SCOPE();
    if (MESA_RC_OK != meba_poe_version_get(board_instance, max_size, info)) {
        strcpy(info, "-");
    }
    return info;
}

void poe_pd_data_clear(mesa_port_no_t iport)
{
    POE_CUSTOM_CRIT_SCOPE();
    meba_poe_port_pd_data_clear(board_instance, iport);
}

