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

/*lint -esym(457,lldp_remote_set_requested_power) */ // Ok - LLDP is writing and PoE is reading. It doesn't matter if the read is missed for one cycle.
/****************************************************************************
PoE ( Power Over Ethernet ) is used to control external PoE chips. For more
information see the Design Spec (DS 0153)
*****************************************************************************/

#include <sys/sysinfo.h>
#include "critd_api.h"
#include "poe.h"
#ifdef VTSS_SW_OPTION_LLDP
#include "lldp_sm.h"
#include "lldp_remote.h"
#include "lldp_api.h"
#include "lldp_basic_types.h"
#endif //  VTSS_SW_OPTION_LLDP
#include "poe_custom_api.h"
#include "misc_api.h"
#include "interrupt_api.h"
#include "poe_trace.h"
#include "port_api.h"           /* For port_count_max() */
#include "port_serializer.hxx"  /* For PHY_INST?!? */
#ifdef VTSS_SW_OPTION_ICFG
#include "poe_icli_functions.h" // For poe_icfg_init
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>
#include<sys/mman.h>

#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif /* VTSS_SW_OPTION_WEB */

#include "vtss_common_iterator.hxx"

#include "meba.h"

#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif  // VTSS_SW_OPTION_SYSLOG

#include "led_api.h"
#include "lock.hxx"

#if defined(VTSS_SW_OPTION_LED_STATUS)
#include "vtss/appl/led.h"
#endif /* VTSS_SW_OPTION_LED_STATUS */
#include "vtss_tftp_api.h"
#include "control_api.h"
#include "poe_options_cfg.h"
#if !defined(USE_POE_STATIC_PARAMETERS)
#include "prod_mngr_db.hxx"
#endif // USE_POE_STATIC_PARAMETERS

#include "icfg.h"

bool i2c_trace = false;

//#define LLDP_LOG_FILTER

/****************************************************************************/
/*  TRACE system                                                            */
/****************************************************************************/
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "poe", "Power Over Ethernet"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_MGMT] = {
        "pow_mgmt",
        "Power Management",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_GRP_CONF] = {
        "conf",
        "Configuration",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_GRP_STATUS] = {
        "status",
        "Status",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_GRP_CUSTOM] = {
        "custom",
        "PoE Chip set",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_GRP_ICLI] = {
        "iCLI",
        "iCLI",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_GRP_LLDP] = {
        "lldp",
        "lldp messages",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define POE_CRIT_ENTER()         critd_enter(&crit,        __FILE__, __LINE__)
#define POE_CRIT_EXIT()          critd_exit( &crit,        __FILE__, __LINE__)
#define POE_CRIT_STATUS_ENTER()  critd_enter(&crit_status, __FILE__, __LINE__)
#define POE_CRIT_STATUS_EXIT()   critd_exit( &crit_status, __FILE__, __LINE__)


const char *poe_controller_type_to_str(poe_controller_type_t eType)
{
    switch (eType) {
    case ePD69200:
        return "PD69200";
    case ePD69210:
        return "PD69210";
    case ePD69220:
        return "PD69220";
    default:
        return "PD-????";
    }
}


const char *poe_syslog_port_status_to_str(uint8_t status)
{
    // MOTE - SysLog message max length is limited. So we will shrink the strings
    switch (status) {
    case VTSS_APPL_POE_UNKNOWN_STATE:
        return "unknown status";
    case VTSS_APPL_POE_POWER_BUDGET_EXCEEDED:       // PoE is turned OFF due to power budget exceeded on PSE
        return "pwr bdgt exced";
    case VTSS_APPL_POE_NO_PD_DETECTED:              // No PD detected
        return "no pd det";
    case VTSS_APPL_POE_PD_ON:                       // PSE supplying power to PD through PoE
        return "pd on";
    case VTSS_APPL_POE_PD_OVERLOAD:                 // PD consumes more power than the maximum limit configured on the PSE port
        return "pd ovl";
    case VTSS_APPL_POE_NOT_SUPPORTED:               // PoE feature is not supported
        return "not supported";
    case VTSS_APPL_POE_DISABLED:                    // PoE feature is disabled on PSE.
        return "disabled";
    case VTSS_APPL_POE_DISABLED_INTERFACE_SHUTDOWN: // PoE disabled due to interface shutdown
        return "dis intf down";
    case VTSS_APPL_POE_PD_FAULT:                    // PoE PD fault
        return "pd fault";
    case VTSS_APPL_POE_PSE_FAULT:                   // PoE pse fault
        return "pse fault";
    default:
        return "??????";
    }
}
// --- syslog end ---


/****************************************************************************/
/*  Pre-defined functions */
/****************************************************************************/

/****************************************************************************/
/*  Global variables */
/****************************************************************************/
// Variable to signaling the PoE chipset has been initialized. It is OK that it
// is not semaphore protected, since it is only supposed to be set one place.
BOOL poe_init_done = FALSE;

// Message
#define POE_TIMEOUT (vtss_current_time() + VTSS_OS_MSEC2TICK(20000)) /* Wait for timeout (20 seconds) or synch. flag to be set ( I2C might be blocked by other modules/threads ). */

// Configuration
static poe_conf_t my_private_poe_conf;   // Current configuration.
static bool poe_conf_updated = false;

/* Critical region protection */
static critd_t crit;  // Critical region for global variables
static critd_t crit_status;  // Critial region for the poe_status.

//when the status is updated from the chip, it can be stored in poe_status_global.
// The status can be accessed without retrieving values from the chip.
// poe_status_global is accessed through function poe_status_get and poe_status_set.
static poe_status_t poe_status_global;

// PoE power supply leds
vtss_appl_poe_led_t power1_led_color = VTSS_APPL_POE_LED_OFF;  // by default
vtss_appl_poe_led_t power2_led_color = VTSS_APPL_POE_LED_OFF;  // by default

// PoE status led
vtss_appl_poe_led_t status_led_color = VTSS_APPL_POE_LED_OFF;  // by default
vtss_appl_poe_led_t prev_status_led_color = VTSS_APPL_POE_LED_OFF;  // by default

/*************************************************************************
** Misc Functions
*************************************************************************/

////////////////////////////////////////////////////////////////////
// Report how the system was started - power-up, software reset, etc.
// In :
// Return :
//    MESA_RESTART_COLD - Internal Ether Switch+internal CPU restart, e.g. power cycling
//    MESA_RESTART_COOL - Internal Ether Switch+internal CPU restart done by CPU
//    MESA_RESTART_WARM - Only internal CPU was reset. No effect on internal Switch
////////////////////////////////////////////////////////////////////
static mesa_restart_t poe_get_system_reset_cause()
{
    mesa_restart_status_t status;
    mesa_restart_t        restart;
    restart = (control_system_restart_status_get(NULL, &status) == VTSS_RC_OK ? status.restart : MESA_RESTART_COLD);
    return restart;
}

//
// Converts error to printable text
//
// In : rc - The error type
//
// Retrun : Error text
//
const char *poe_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_POE_ERROR_NULL_POINTER:
        return "Unexpected reference to NULL pointer.";

    case VTSS_APPL_POE_ERROR_UNKNOWN_BOARD:
        return "Unknown board type.";

    case VTSS_APPL_POE_ERROR_PRIM_SUPPLY_RANGE:
        return "Primary power supply value out of range.";

    case VTSS_APPL_POE_ERROR_CONF_ERROR:
        return "Internal error - Configuration could not be done.";

    case VTSS_APPL_POE_ERROR_PORT_POWER_EXCEEDED:
        return "Port power exceeded.";

    case VTSS_APPL_POE_ERROR_NOT_SUPPORTED:
        return "Port is not supporting PoE.";

    case VTSS_APPL_POE_ERROR_FIRMWARE:
        return "PoE firmware download failed.";

    case VTSS_APPL_POE_ERROR_DETECT:
        return "PoE chip detection still in progress.";

    case VTSS_APPL_POE_ERROR_FIRMWARE_VER:
        return "PoE firmware version not found.";

    case VTSS_APPL_POE_ERROR_FIRMWARE_VER_NOT_NEW:
        return "Already contains this PoE firmware version";

    case VTSS_RC_OK:
        return "";
    }

    T_I("rc:%d", rc);
    return "Unknown PoE error";
}


#include <iostream>
#include <fstream>
#include <string>

// Doing firmware upgrade.
// url - Only valid if "has_built_in" if false. String with tftp path to new firmware file
// tftp_err - Only valid if "has_built_in" if false. tftp error code, to be used with vtss_tftp_err2str
// has_built_in - TRUE to use the built in firmware placed in /etc/mscc/poe/firmware/mscc_firmware.s19. FALSE to use the URL to find the firmware
mesa_rc poe_do_firmware_upgrade(const char *url, int &tftp_err, BOOL has_built_in, BOOL has_brick)
{
    mesa_rc rc = VTSS_RC_OK;
    int size = 0;
    char *buffer = NULL;
    int fd = -1;
    bool has_mmap = false;
    bool has_malloc = false;
    bool has_fd = false;

    if (has_built_in) {
        size = 0;
        buffer = 0;
    } else if (has_brick || strncmp("flash:", url, strlen("flash:")) == 0) {
        struct stat st;
        char file_name[128] = "/etc/mscc/poe/firmware/brick_firmware.txt";
        if (!has_brick) {
            sprintf(file_name, "/switch/icfg/%s", url + 6);
        }
        if ((fd = open(file_name, O_RDONLY)) < 0) {
            T_E("Could not open %s for reading.\n", file_name );
            return MESA_RC_ERROR;
        }
        has_fd = true;

        if (fstat(fd, &st) < 0) {
            T_E("Could not determine size of %s.\n", file_name );
            close(fd);
            return MESA_RC_ERROR;
        }
        if ((buffer = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
            T_E("Could not map %s.\n", file_name );
        }
        has_mmap = true;
        size = st.st_size;
    } else {
        if ((VTSS_MALLOC_CAST(buffer, POE_FIRMWARE_SIZE_MAX)) == NULL) {
            T_E("Memory allocation issue");
            rc = VTSS_APPL_POE_ERROR_FIRMWARE;
            goto out;
        }
        has_malloc = true;

        // Get tftp data
        misc_url_parts_t url_parts;
        misc_url_parts_init(&url_parts, MISC_URL_PROTOCOL_TFTP  |
                            MISC_URL_PROTOCOL_FTP   |
                            MISC_URL_PROTOCOL_HTTP  |
                            MISC_URL_PROTOCOL_HTTPS |
                            MISC_URL_PROTOCOL_FILE);

        if (!misc_url_decompose(url, &url_parts)) {
            T_I("Invalid url");
            rc = VTSS_APPL_POE_ERROR_FIRMWARE;
            goto out;
        }

        if ((size = vtss_tftp_get(url_parts.path, url_parts.host, url_parts.port,
                                  (char *) buffer, POE_FIRMWARE_SIZE_MAX, TRUE, &tftp_err)) <= 0) {
            rc = VTSS_APPL_POE_ERROR_FIRMWARE;
            goto out;
        }

    }

    T_N("%s", buffer);

    rc = poe_mgmt_firmware_update(buffer, size);
    T_I("size:%d, rc:%d", size, rc);

out:
    if (buffer && has_mmap) {
        (void) munmap(buffer, size);
    }
    // Free resource
    if (buffer && has_malloc) {
        VTSS_FREE(buffer);
    }

    if (has_fd) {
        close(fd);
    }

    if (rc == VTSS_RC_OK) {
        poe_conf_t poe_conf;
        poe_config_get(&poe_conf);
        mesa_restart_t  restart_cause = poe_get_system_reset_cause();

        poe_custom_init_chips(poe_conf.interruptible_power, restart_cause); // PoE chip has been reset during the upgrade process. We need to re-initialize the chip.
    }
    return rc;
}
/*****************************************************************/
// Description: Converting a integer to a string with one digit. E.g. 102 becomes 10.2. *
// Output: Returns pointer to string
/*****************************************************************/

char *one_digi_float2str(int val, char *max_power_val_str)
{
    char digi[2] = "";

    if (val == 0) {
        return "0";
    }

    // Convert the integer to a string
    sprintf(max_power_val_str, "%d", val);

    int str_len = strlen(max_power_val_str);

    // get the last charater in the string
    digi[0] =  max_power_val_str[str_len - 1];
    digi[1] =  '\0';

    // Remove the last digi in the string
    max_power_val_str[str_len - 1] = '\0';


    if (str_len == 1) {
        // If the integer only were one digi then add "0." in front. E.g. 4 becomes 0.4
        strcat(max_power_val_str, "0.");
    } else {
        // Add the "dot" to the string
        strcat(max_power_val_str, ".");
    }

    // Add the digi to the string
    strcat(max_power_val_str, &digi[0]);


    // return the string
    return max_power_val_str;
}

// Return if a port is in shut-down by user.
BOOL is_port_shutdown(mesa_port_no_t iport)
{
    vtss_ifindex_t ifindex;
    vtss_appl_port_conf_t port_conf;

    // Make sure that we don't get out-of-bounds
    if (iport >= port_count_max()) {
        T_N("PORT:%d, count:%d", iport, port_count_max());
        return TRUE;
    }

    VTSS_RC_ERR_PRINT(vtss_ifindex_from_port(VTSS_ISID_START, iport, &ifindex));
    VTSS_RC_ERR_PRINT(vtss_appl_port_conf_get(ifindex, &port_conf));

#if defined(DISABLE_LINK_ALSO_DISABLE_POE)
    if (DISABLE_LINK_ALSO_DISABLE_POE != 0 && !port_conf.admin.enable) {
        return TRUE;
    }
#endif

    return FALSE;
}

// Getting local mode configuration when taking into account that port can
// be shut-down by the port module.
static vtss_appl_poe_port_mode_t poe_mode_get(mesa_port_no_t iport)
{
    POE_CRIT_ENTER(); // Protect poe_conf
    vtss_appl_poe_port_mode_t status = my_private_poe_conf.poe_mode[iport];
    POE_CRIT_EXIT();
    T_RG_PORT(VTSS_TRACE_GRP_STATUS, iport, "Status:0x%X", status);
    if (is_port_shutdown(iport)) {
        status = VTSS_APPL_POE_MODE_DISABLED;
    }
    T_RG_PORT(VTSS_TRACE_GRP_STATUS, iport, "Status:0x%X", status);
    return status;
}

// Function for updating a local copy of the PoE status.
// IN :  local_poe_status - Pointer to the new status.
static void poe_status_set(poe_status_t *local_poe_status)
{
    // The status from PoE is updated every sec in order to be able to respond upon changes. To get fast access to the status from management
    // we keep a local copy of the status which we give back fast.
    T_R("Enter poe_status_set");
    POE_CRIT_STATUS_ENTER();
    poe_status_global = *local_poe_status;
    POE_CRIT_STATUS_EXIT();
    T_NG(VTSS_TRACE_GRP_STATUS, "Total power:%d, total_current:%d, total reserved_dw:%d",
         poe_status_global.power_consumption_w, poe_status_global.calculated_power_w, poe_status_global.calculated_total_power_reserved_dw);
}

// Function for updating a local copy of the PoE status.
// IN :  local_poe_status - Pointer to where to put the current status.
static void poe_status_get(poe_status_t *local_poe_status)
{
    // The status from PoE is updated every sec in order to be able to respond upon changes. To get fast access to the status from management
    // we keep a local copy of the status which we give back fast.
    T_R("Enter poe_status_get");
    POE_CRIT_STATUS_ENTER();
    *local_poe_status = poe_status_global;
    POE_CRIT_STATUS_EXIT();
    T_NG(VTSS_TRACE_GRP_STATUS, "Total power:%d, total_current:%d, total reserved_dw:%d",
         poe_status_global.power_consumption_w, poe_status_global.calculated_power_w, poe_status_global.calculated_total_power_reserved_dw);
}

// Function for getting which PoE chipset is found from outside this file (In order to do semaphore protection)
meba_poe_chip_state_t poe_is_chip_found(mesa_port_no_t port_index, const char *file, u32 line)
{
    meba_poe_chip_state_t chip_state;
    POE_CRIT_STATUS_ENTER();
    chip_state = poe_status_global.poe_chip_found[port_index];
    POE_CRIT_STATUS_EXIT();
    return chip_state;
}

// Send PoE syslog message. pInfo pointer can be NULL for messages with no extra info
void poe_Send_SysLog_Msg(poe_syslog_level_t log_level, poe_syslog_message_type_t eMsgType, uPOE_SYSLOG_INFO *pInfo)
{
    char        IfName[50]     = "";
    char        SysLogMsg[200] = "";
    const char *StartMsg = "POE-CONTROL: ";

    // build the syslog message string
    switch (eMsgType) {
    case ePORT_STAT_CHANGE: {
        vtss_ifindex_t  ifindex; // physical index not logical

        if (pInfo == NULL) {
            T_E("NULL Pointer");
            return;
        }

        if (pInfo->port_stat_change_t.port_index >= fast_cap(MEBA_CAP_BOARD_PORT_COUNT)) {
            return ;
        }

        if (pInfo->port_stat_change_t.new_poe_port_stat == pInfo->port_stat_change_t.old_poe_port_stat) {
            return;    // this event was already reported once
        }

        // conv log port index to physical Switch port number (HW mapped)
        if (vtss_ifindex_from_port(0, pInfo->port_stat_change_t.port_index, &ifindex) != VTSS_RC_OK) {
            T_E("Could not get ifindex");
            return;
        }
        vtss_ifindex2str(IfName, sizeof(IfName), ifindex);  // conv phys port to log string format as "Port 1/3"

        char str[10] = {0};

        if (pInfo->port_stat_change_t.pd_type_sspd_dspd == 2) {
            strcpy(str, " DSPD");
        } else if (pInfo->port_stat_change_t.pd_type_sspd_dspd == 1) {
            strcpy(str, " SSPD");
        }

        T_I("port = %d  old-Status = %s -> New-Status=%s%s", pInfo->port_stat_change_t.port_index,
            poe_syslog_port_status_to_str(pInfo->port_stat_change_t.old_poe_port_stat),
            poe_syslog_port_status_to_str(pInfo->port_stat_change_t.new_poe_port_stat),
            str);

        snprintf(SysLogMsg, sizeof(SysLogMsg), "%s %s changed from: %s (0x%02X) to: %s (0x%02X%s)",
                 StartMsg,
                 IfName,    // as "Port 1/3"
                 poe_syslog_port_status_to_str(pInfo->port_stat_change_t.old_poe_port_stat),
                 pInfo->port_stat_change_t.old_internal_poe_status,
                 poe_syslog_port_status_to_str(pInfo->port_stat_change_t.new_poe_port_stat),
                 pInfo->port_stat_change_t.new_internal_poe_status,
                 str);
        break;
    }

    case ePOE_LLDP_PD_REQUEST: {
        vtss_ifindex_t  ifindex; // physical index not logical
        char txt_string1[50];
        char txt_string2[50];
        char txt_string3[50];

        if (pInfo == NULL) {
            T_E("NULL Pointer");
            return;
        }

        if (pInfo->lldp_pd_power_request_t.port_index >= fast_cap(MEBA_CAP_BOARD_PORT_COUNT)) {
            return ;
        }

        // conv log port index to physical Switch port number (HW mapped)
        if (vtss_ifindex_from_port(0, pInfo->lldp_pd_power_request_t.port_index, &ifindex) != VTSS_RC_OK) {
            T_E("Could not get ifindex");
            return;
        }
        vtss_ifindex2str(IfName, sizeof(IfName), ifindex);  // conv phys port to log string format as "Port 1/3"

        if (pInfo->lldp_pd_power_request_t.is_bt_port) {
            T_I("Syslog: LLDP-BT: port# %d ,Set Max Power SINGLE=%d mW, ALT-A=%d mW, ALT-B=%d mW. Cable length = %d meter\n\r",
                pInfo->lldp_pd_power_request_t.port_index,
                pInfo->lldp_pd_power_request_t.single_mw,
                pInfo->lldp_pd_power_request_t.alt_a_mw,
                pInfo->lldp_pd_power_request_t.alt_b_mw,
                pInfo->lldp_pd_power_request_t.cable_length * 10);

            snprintf(SysLogMsg, sizeof(SysLogMsg), "%s %s LLDP-BT set Power[W] Single=%s, Alt-A=%s, Alt_B=%s, CL=%dm",
                     StartMsg,
                     IfName,    // as "Port 1/3"
                     one_digi_float2str(pInfo->lldp_pd_power_request_t.single_mw / 100, &txt_string1[0]),  // Power Requested single
                     one_digi_float2str(pInfo->lldp_pd_power_request_t.alt_a_mw / 100,  &txt_string2[0]),  // Power Requested alt-A
                     one_digi_float2str(pInfo->lldp_pd_power_request_t.alt_b_mw / 100,  &txt_string3[0]),  // Power Requested alt-B
                     pInfo->lldp_pd_power_request_t.cable_length * 10);
        } else {
            T_I("Syslog: LLDP-AT: port# %d ,%s Set Max Power SINGLE=%d mW ,Cable length = %d meter\n\r",
                pInfo->lldp_pd_power_request_t.port_index,
                IfName,
                pInfo->lldp_pd_power_request_t.single_mw,
                pInfo->lldp_pd_power_request_t.cable_length * 10);

            snprintf(SysLogMsg, sizeof(SysLogMsg), "%s %s LLDP-BT set Power[W] Single=%s, CL=%dm",
                     StartMsg,
                     IfName,    // as "Port 1/3"
                     one_digi_float2str(pInfo->lldp_pd_power_request_t.single_mw / 100, &txt_string1[0]),   // Power Requested single
                     pInfo->lldp_pd_power_request_t.cable_length * 10);
        }

        break;
    }

    case ePOE_CONTROLLER_FOUND: {
        snprintf(SysLogMsg, sizeof(SysLogMsg), "%s controller found", StartMsg);
        break;
    }

    case ePOE_CONTROLLER_NOT_FOUND: {
        snprintf(SysLogMsg, sizeof(SysLogMsg), "%s controller not found !!", StartMsg);
        break;
    }

    case ePOE_FIRMWARE_UPDATE: {
        if (pInfo == NULL) {
            T_E("NULL Pointer");
            return;
        }
        snprintf(SysLogMsg, sizeof(SysLogMsg), "%s controller #%d %s firmware-update %s",
                 StartMsg,
                 pInfo->poe_firmware_update_t.poe_mcu_index + 1,
                 poe_controller_type_to_str(pInfo->poe_firmware_update_t.eType),
                 pInfo->poe_firmware_update_t.firmware_status);
        break;
    }

    case ePOE_EXIT_POE_THREAD: {
        if (pInfo == NULL) {
            T_E("NULL Pointer");
            return;
        }
        snprintf(SysLogMsg, sizeof(SysLogMsg), "%s %s - Exit PoE thread", StartMsg, pInfo->exit_thread_t.error_msg);
        break;
    }

    case ePOE_GLOBAL_MSG: {
        if (pInfo == NULL) {
            T_E("NULL Pointer");
            return;
        }
        snprintf(SysLogMsg, sizeof(SysLogMsg), "%s", pInfo->general_global_msg_t.msg);
        break;
    }

    case ePOE_PORT_MSG: {
        vtss_ifindex_t  ifindex; // physical index not logical

        if (pInfo == NULL) {
            T_E("NULL Pointer");
            return;
        }

        if (pInfo->port_stat_change_t.port_index >= fast_cap(MEBA_CAP_BOARD_PORT_COUNT)) {
            return ;
        }

        //if (pInfo->port_stat_change_t.new_poe_port_stat == pInfo->port_stat_change_t.old_poe_port_stat)
        //    return;  // this event was already reported once

        // conv log port index to physical Switch port number (HW mapped)
        if (vtss_ifindex_from_port(0, pInfo->port_stat_change_t.port_index, &ifindex) != VTSS_RC_OK) {
            T_E("Could not get ifindex");
            return;
        }
        vtss_ifindex2str(IfName, sizeof(IfName), ifindex);  // conv phys port to log string format as "Port 1/3"

        T_D("ifindex=%s, msg= %s", ifindex, pInfo->general_port_msg_t.msg);

        snprintf(SysLogMsg, sizeof(SysLogMsg), "%s %s %s",
                 StartMsg,
                 IfName,    // as "Port 1/3"
                 pInfo->general_port_msg_t.msg);

        break;
    }
    }

    T_I("syslog msg='%s'", SysLogMsg);

#if defined(VTSS_SW_OPTION_SYSLOG)

    // Send the correct level syslog message
    switch (log_level) {
    case eSYSLOG_LVL_ERROR:
        S_E("%s", SysLogMsg);
        break;
    case eSYSLOG_LVL_WARNING:
        S_W("%s", SysLogMsg);
        break;
    case eSYSLOG_LVL_NOTICE:
        S_N("%s", SysLogMsg);
        break;
    case eSYSLOG_LVL_INFO:
        S_I("%s", SysLogMsg);
        break;
    }
#endif
}



/**
 * \brief Function for Analyze PoE port status for updating led later
 *
 * \param poe_status [IN]  Current poe status
 * \param port_index [IN]  port number
 * \param poe_led    [OUT] update poe led bitset
 *
 * \return void
 **/
static void vtss_appl_poe_led_analyze(poe_status_t *poe_status,
                                      mesa_port_no_t port_index,
                                      u8 *poe_led)
{
    switch (poe_status->port_status[port_index].pd_status) {
    case VTSS_APPL_POE_POWER_BUDGET_EXCEEDED :
    case VTSS_APPL_POE_PD_OVERLOAD : {
        *poe_led = POE_LED_POE_DENIED;
        break;
    }
    case VTSS_APPL_POE_PD_ON : {
        *poe_led = POE_LED_POE_ON;
        break;
    }
    case VTSS_APPL_POE_PD_FAULT :
    case VTSS_APPL_POE_PSE_FAULT :
    case VTSS_APPL_POE_UNKNOWN_STATE: {
        *poe_led = POE_LED_POE_ERROR;
        break;
    }
    //case VTSS_APPL_POE_DISABLED :      // PoE feature is disabled on PSE.
    //case VTSS_APPL_POE_NO_PD_DETECTED :
    //case VTSS_APPL_POE_NOT_SUPPORTED:  // PoE feature is not supported.
    //VTSS_APPL_POE_DISABLED_INTERFACE_SHUTDOWN,  // PoE disabled due to interface shutdown
    default :
        *poe_led = POE_LED_POE_OFF;
        break;
    }
}

/**
 * \brief Function for setting poe power supply leds
 * Green - Power is present on the associated circuit, system is operating
 *         normally.
 * Off   - Power is not present on the associated circuit, or the system is not
 *         powered up.
 * Red   - Power is not present on the associated circuit, and the power supply
 *         alarm is configured.
 *
 * \return VTSS_RC_OK if operation succeeded.
 **/
static mesa_rc vtss_appl_poe_pwr_led_set(void)
{
    poe_status_t poe_status;
    poe_status_get(&poe_status);

    // led has been set ok, update the global variable
    power1_led_color = poe_status.pwr_in_status1 ? VTSS_APPL_POE_LED_GREEN : VTSS_APPL_POE_LED_OFF;

    // led has been set ok, update the global variable
    power2_led_color = poe_status.pwr_in_status2 ? VTSS_APPL_POE_LED_GREEN : VTSS_APPL_POE_LED_OFF;

    meba_status_led_set(board_instance, MEBA_LED_TYPE_DC_A, poe_status.pwr_in_status1 ? MEBA_LED_COLOR_GREEN : MEBA_LED_COLOR_OFF);
    meba_status_led_set(board_instance, MEBA_LED_TYPE_DC_B, poe_status.pwr_in_status2 ? MEBA_LED_COLOR_GREEN : MEBA_LED_COLOR_OFF);

    /* TODO: Handle Red status */
    return VTSS_RC_OK;
}

/**
 * \brief Function for getting the status of the 2 power leds on the PoE board.
 *
 * \param pwr_led1 [IN/OUT] Led status for power supply 1.
 *
 * \param pwr_led2 [IN/OUT] Led status for power supply 2.
 *
 * \return VTSS_RC_OK  if operation is succeeded.
 **/
mesa_rc vtss_appl_poe_powerin_led_get(vtss_appl_poe_powerin_led_t *const pwr_led)
{
    poe_status_t poe_status;
    poe_status_get(&poe_status);

    pwr_led->pwr_led1 = poe_status.pwr_in_status1 ? VTSS_APPL_POE_LED_GREEN : VTSS_APPL_POE_LED_OFF;
    pwr_led->pwr_led2 = poe_status.pwr_in_status2 ? VTSS_APPL_POE_LED_GREEN : VTSS_APPL_POE_LED_OFF;

    return VTSS_RC_OK;
}


/**
 * \brief Function for setting PoE status led color
 *
 * \param bitset [IN] bitset for setting the PoE status led.
 *
 * \return VTSS_RC_OK if operation is succeeded.
 */
mesa_rc vtss_appl_poe_status_led_set(const u8 bitset)
{
    if (bitset & POE_LED_POE_ERROR) {
        meba_status_led_set(board_instance, MEBA_LED_TYPE_POE, MEBA_LED_COLOR_RED);
    } else if (bitset & POE_LED_POE_DENIED) {
        meba_status_led_set(board_instance, MEBA_LED_TYPE_POE, MEBA_LED_COLOR_YELLOW);
    } else if (bitset & POE_LED_POE_ON) {
        meba_status_led_set(board_instance, MEBA_LED_TYPE_POE, MEBA_LED_COLOR_GREEN);
    } else {
        meba_status_led_set(board_instance, MEBA_LED_TYPE_POE, MEBA_LED_COLOR_OFF);
    }
#if defined(VTSS_SW_OPTION_LED_STATUS)
    u32 state;
    if (bitset & POE_LED_POE_ERROR) {
        state = VTSS_APPL_LED_CTRL_POE_FAULT;
        status_led_color = VTSS_APPL_POE_LED_RED;
    } else if (bitset & POE_LED_POE_DENIED) {
        state = VTSS_APPL_LED_CTRL_POE_EXCEED;
        status_led_color = VTSS_APPL_POE_LED_BLINK_RED;
    } else if (bitset & POE_LED_POE_ON) {
        state = VTSS_APPL_LED_CTRL_POE_ON;
        status_led_color = VTSS_APPL_POE_LED_GREEN;
    } else if (bitset & POE_LED_POE_OFF) {
        state = VTSS_APPL_LED_CTRL_POE_OFF;
        status_led_color = VTSS_APPL_POE_LED_OFF;
    } else {
        T_WG(VTSS_TRACE_GRP_STATUS, "Unknown PoE LED Status %u!", bitset);
        state = VTSS_APPL_LED_CTRL_POE_OFF;
        status_led_color = VTSS_APPL_POE_LED_OFF;
    }

    // set led color
    if (led_system_led_state_set(SYSTEM_LED_ID_POE, state, FALSE) == VTSS_RC_OK) {

        T_NG(VTSS_TRACE_GRP_STATUS, "poe status led bitset: %u, color: %d\n",
             bitset,
             (int)status_led_color);

        // update previous color
        prev_status_led_color = status_led_color;
    } else {
        // return error if set LED failed
        return VTSS_RC_ERROR;
    }
#else
    if (bitset & POE_LED_POE_ERROR) {
        status_led_color = VTSS_APPL_POE_LED_RED;
    } else if (bitset & POE_LED_POE_DENIED) {
        status_led_color = VTSS_APPL_POE_LED_BLINK_RED;
    } else if (bitset & POE_LED_POE_ON) {
        status_led_color = VTSS_APPL_POE_LED_GREEN;
    } else if (bitset & POE_LED_POE_OFF) {
        status_led_color = VTSS_APPL_POE_LED_OFF;
    } else {
        T_WG(VTSS_TRACE_GRP_STATUS, "Unknown PoE LED Status %u!", bitset);
        status_led_color = VTSS_APPL_POE_LED_OFF;
    }
    T_NG(VTSS_TRACE_GRP_STATUS, "poe status led bitset: %u, color: %d\n",
         bitset,
         (int)status_led_color);

    // update previous color
    prev_status_led_color = status_led_color;

    // set led color via sgpio interface
#if defined(VTSS_POE_POE_STATUS_LED)
    return vtss_appl_poe_led_set(VTSS_POE_POE_STATUS_LED, status_led_color);
#endif
#endif /* VTSS_SW_OPTION_LED_STATUS */
    return VTSS_RC_OK;
}

/**
 * \brief Function for getting PoE status led color
 *
 * \param status_led [IN/OUT] The current color of the PoE status led.
 *
 * \return VTSS_RC_OK if operation succeeded.
 **/
mesa_rc vtss_appl_poe_status_led_get(vtss_appl_poe_status_led_t *status_led)
{
    status_led->status_led = status_led_color;
    return VTSS_RC_OK;
}


/****************************************************************************/
/*  poe status mutex functions                                              */
/****************************************************************************/

static critd_t poe_status_crit;

#define POE_STATUS_CRIT_ENTER()         critd_enter(        &poe_status_crit, __FILE__, __LINE__)
#define POE_STATUS_CRIT_EXIT()          critd_exit(         &poe_status_crit, __FILE__, __LINE__)
#define POE_STATUS_CRIT_ASSERT_LOCKED() critd_assert_locked(&poe_status_crit, __FILE__, __LINE__)


/****************************************************************************/
/*  API functions                                                           */
/****************************************************************************/
// Because we want to get the whole LLDP entry table/statistics from the switch, and since the entry table can be quite large (280K) at
// the time of writing, we give the applications the possibility to take the semaphore, and work directly with the entry table in the LLDP module,
// instead of having to copy the table between the different modules.
void vtss_appl_poe_status_mutex_lock(void)
{
    // Avoid Lint Warning 454: A thread mutex has been locked but not unlocked
    /*lint --e{454} */
    POE_STATUS_CRIT_ENTER();
}

void vtss_appl_poe_status_mutex_unlock(void)
{
    // Avoid Lint Warning 455: A thread mutex that had not been locked is being unlocked
    /*lint -e(455) */
    POE_STATUS_CRIT_EXIT();
}

void vtss_appl_poe_status_mutex_assert(const char *file, int line)
{
    critd_assert_locked(&poe_status_crit, file, line);
}
/****************************************************************************/
/*  END of poe status mutex functions                                       */
/****************************************************************************/

static char poe_port_msg[100];

char *poe_status_txt[] = {
    /*VTSS_APPL_POE_UNKNOWN_STATE,              */ "unknown State",
    /*VTSS_APPL_POE_POWER_BUDGET_EXCEEDED,      */ "budget Exceeded",
    /*VTSS_APPL_POE_NO_PD_DETECTED,             */ "no Pd Detected",
    /*VTSS_APPL_POE_PD_ON,                      */ "pd On",
    /*VTSS_APPL_POE_PD_OVERLOAD,                */ "pd Overloaded",
    /*VTSS_APPL_POE_NOT_SUPPORTED,              */ "not Supported",
    /*VTSS_APPL_POE_DISABLED,                   */ "disabled",
    /*VTSS_APPL_POE_DISABLED_INTERFACE_SHUTDOWN,*/ "shutdown",
    /*VTSS_APPL_POE_PD_FAULT,                   */ "pd fault",
    /*VTSS_APPL_POE_PSE_FAULT,                  */ "pse fault"
};


// Getting and updating poe status
static void poe_status_update(void)
{
    vtss_appl_poe_status_mutex_lock();  // 'poe_status_update' function called from poe and lldp threads

    poe_status_t poe_status;
    mesa_port_no_t port_index;

    CapArray<vtss_appl_poe_port_status_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_port_status_old;
    u8 poe_led = POE_LED_POE_OFF;  // bitset for PoE led status
    T_NG(VTSS_TRACE_GRP_STATUS, "Getting PoE poe_status" );

    //i2c_trace = false;
    // Update Power and current used fields
    poe_status_get(&poe_status);
    poe_port_status_old = poe_status.port_status; // backup old poe ports status

    for (port_index = 0 ; port_index < fast_cap(MEBA_CAP_BOARD_PORT_COUNT); port_index++) {
        poe_status.port_status[port_index].bISPoEStatusFault = false;

        // update last pd status error
        if ((poe_status.port_status[port_index].pd_status == VTSS_APPL_POE_POWER_BUDGET_EXCEEDED) ||
            (poe_status.port_status[port_index].pd_status == VTSS_APPL_POE_PD_OVERLOAD) ||
            (poe_status.port_status[port_index].pd_status == VTSS_APPL_POE_PD_FAULT) ||
            (poe_status.port_status[port_index].pd_status == VTSS_APPL_POE_PSE_FAULT)) {
            poe_status.port_status[port_index].port_status_last_error = poe_status.port_status[port_index].pd_status;
            poe_status.port_status[port_index].bISPoEStatusFault = true;
        }
    }

    if (poe_custom_get_status(&poe_status) != MESA_RC_OK) {
        for (port_index = 0 ; port_index < fast_cap(MEBA_CAP_BOARD_PORT_COUNT); port_index++) {
            poe_status.port_status[port_index].pd_status = VTSS_APPL_POE_UNKNOWN_STATE ;
        }
        //return;
    }

    // Set PoE two power supply leds
    vtss_appl_poe_pwr_led_set();

    poe_status.calculated_total_power_used_mw = 0;
    poe_status.calculated_total_current_used_ma = 0;
    poe_status.calculated_total_power_reserved_dw =  0;

    for (port_index = 0 ; port_index < fast_cap(MEBA_CAP_BOARD_PORT_COUNT); port_index++) {

        // Only set PoE if PoE is available for this port
        if (!port_custom_table[port_index].poe_support) {
            continue;
        }

        T_DG_PORT(VTSS_TRACE_GRP_STATUS, port_index, "Found:%d , port status: %d (%s), internal port status: 0x%X",
                  poe_status.poe_chip_found[port_index],
                  poe_status.port_status[port_index].pd_status,
                  poe_icli_port_status2str(poe_status.port_status[port_index].pd_status),
                  poe_status.port_status[port_index].pd_status_internal);

        vtss_appl_poe_port_status_t &port_status = poe_status.port_status[port_index];

        if (port_status.pd_status != VTSS_APPL_POE_PD_ON) {
            port_status.previous_pd_request_power.single_mw = 0xFFFF;
            port_status.previous_pd_request_power.alt_a_mw  = 0xFFFF;
            port_status.previous_pd_request_power.alt_b_mw  = 0xFFFF;
        }

        // Make sure that power used and current used have valid value even when no PoE chip is not found.
        if (poe_status.poe_chip_found[port_index] == MEBA_POE_NO_CHIPSET_FOUND ||
            port_status.pd_status    == VTSS_APPL_POE_NOT_SUPPORTED ||
            //port_status.pd_status    == VTSS_APPL_POE_NO_PD_DETECTED ||
            port_status.pd_status    == VTSS_APPL_POE_DISABLED ||
            port_status.pd_status    == VTSS_APPL_POE_DISABLED_INTERFACE_SHUTDOWN ||
            port_status.pd_status    == VTSS_APPL_POE_UNKNOWN_STATE ||
            port_status.pd_status    == VTSS_APPL_POE_PD_FAULT ||
            port_status.pd_status    == VTSS_APPL_POE_PSE_FAULT ||
            poe_mode_get(port_index) == VTSS_APPL_POE_MODE_DISABLED ) {
            T_DG_PORT(VTSS_TRACE_GRP_STATUS, port_index, "Resetting status for port with not PoE chipset=%d pd_status_internal=%d ,pd_status=0x%X", poe_mode_get(port_index), port_status.pd_status_internal, port_status.pd_status);
            port_status.power_consume_mw = 0;
            port_status.current_consume_ma = 0;
            port_status.assigned_pd_class_a = POE_UNDETERMINED_CLASS;
            port_status.assigned_pd_class_b = POE_UNDETERMINED_CLASS;
            port_status.pd_structure = VTSS_APPL_POE_PD_STRUCTURE_NOT_PERFORMED;
            port_status.power_requested_mw = 0;
            port_status.power_reserved_dw =  0;
            port_status.pd_type_sspd_dspd = 0;


            if (poe_status.poe_chip_found[port_index] == MEBA_POE_NO_CHIPSET_FOUND ||
                port_status.pd_status      == VTSS_APPL_POE_NOT_SUPPORTED ||
                port_status.pd_status      == VTSS_APPL_POE_UNKNOWN_STATE
                //port_status.pd_status    == VTSS_APPL_POE_NO_PD_DETECTED ||
                //port_status.pd_status    == VTSS_APPL_POE_DISABLED ||
                //port_status.pd_status    == VTSS_APPL_POE_DISABLED_INTERFACE_SHUTDOWN ||
                //port_status.pd_status    == VTSS_APPL_POE_PD_FAULT ||
                //port_status.pd_status    == VTSS_APPL_POE_PSE_FAULT ||
                //poe_mode_get(port_index) == VTSS_APPL_POE_MODE_DISABLED
               ) {
                port_status.pd_status = VTSS_APPL_POE_UNKNOWN_STATE;
                port_status.pd_status_internal    = 0x37;  //Unknown device port status.
                strcpy(port_status.pd_status_internal_description, "Unknown");
            }

            // !unknown -> unknown
            if ( (poe_port_status_old[port_index].pd_status != VTSS_APPL_POE_UNKNOWN_STATE) &&
                 poe_status.port_status[port_index].pd_status == VTSS_APPL_POE_UNKNOWN_STATE) {
                T_I("syslog port#%d, !unknown -> unknown", port_index);
#ifdef VTSS_SW_OPTION_SYSLOG

                uPOE_SYSLOG_INFO uSysInfo;
                uSysInfo.port_stat_change_t.port_index   = port_index;

                sprintf(poe_port_msg, "changed to %s", poe_status_txt[port_status.pd_status]);

                uSysInfo.general_port_msg_t.msg = poe_port_msg;

                poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_PORT_MSG, &uSysInfo);

#endif // VTSS_SW_OPTION_SYSLOG
            } else if ( (!poe_port_status_old[port_index].bISPoEStatusFault) &&          // !Fault -> Fault
                        poe_status.port_status[port_index].bISPoEStatusFault) {
                T_I("syslog port#%d, !Fault -> Fault", port_index);

#ifdef VTSS_SW_OPTION_SYSLOG

                uPOE_SYSLOG_INFO uSysInfo;
                uSysInfo.port_stat_change_t.port_index   = port_index;

                sprintf(poe_port_msg, "changed to %s (0x%02X)", poe_status_txt[port_status.pd_status], poe_status.port_status[port_index].pd_status_internal);

                uSysInfo.general_port_msg_t.msg = poe_port_msg;

                poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_PORT_MSG, &uSysInfo);

#endif // VTSS_SW_OPTION_SYSLOG
            }

            // Check PoE port status for led update
            vtss_appl_poe_led_analyze(&poe_status, port_index, &poe_led);


            // We have seen that some PoE Chipsets don't give PoE disable back when the port is powered down,
            // so we force the status in order to make sure that status is shown correct.
            continue; // continue to next port
        }

        bool bSyslogSent = FALSE;

        // !On -> On   - port turned on
        if ((poe_port_status_old[port_index].pd_status != VTSS_APPL_POE_PD_ON) &&  // old status is !On (searching , fault , disabled..)
            (port_status.pd_status == VTSS_APPL_POE_PD_ON)) {                      // new status is On
            T_I("syslog port#%d, !On -> On", port_index);
            poe_custom_new_pd_detected_set(port_index); //Required to keep poe_custom_new_pd_detected_get() up to date.

            //printf("\n\r ------  port# %d ,   %d \n\r", port_index , port_status.pd_type_sspd_dspd);
            bSyslogSent = TRUE;

#if defined(VTSS_SW_OPTION_SYSLOG)
            uPOE_SYSLOG_INFO uSysInfo;
            uSysInfo.port_stat_change_t.port_index              = port_index;
            uSysInfo.port_stat_change_t.old_poe_port_stat       = poe_port_status_old[port_index].pd_status;
            uSysInfo.port_stat_change_t.new_poe_port_stat       = port_status.pd_status;
            uSysInfo.port_stat_change_t.old_internal_poe_status = poe_port_status_old[port_index].pd_status_internal;
            uSysInfo.port_stat_change_t.new_internal_poe_status = port_status.pd_status_internal;
            uSysInfo.port_stat_change_t.pd_type_sspd_dspd       = port_status.pd_type_sspd_dspd;
            poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePORT_STAT_CHANGE, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */
        }

        // On -> Fault/Searching (exclude disabled)
        if ((poe_port_status_old[port_index].pd_status == VTSS_APPL_POE_PD_ON) &&   // old status is On
            (port_status.pd_status != VTSS_APPL_POE_PD_ON) &&                       // new status is Fault/Searching
            poe_status.port_status[port_index].bISPoEStatusFault &&
            (poe_port_status_old[port_index].port_status_last_error != port_status.pd_status)  // its not the same error as was before
           )
            //(port_status.pd_status != VTSS_APPL_POE_DISABLED))                    // new status not disabled
        {
            bSyslogSent = TRUE;

            T_I("syslog port#%d, On -> Fault/Searching", port_index);

#if defined(VTSS_SW_OPTION_SYSLOG)
            uPOE_SYSLOG_INFO uSysInfo;
            uSysInfo.port_stat_change_t.port_index              = port_index;
            uSysInfo.port_stat_change_t.old_poe_port_stat       = poe_port_status_old[port_index].pd_status;
            uSysInfo.port_stat_change_t.new_poe_port_stat       = port_status.pd_status;
            uSysInfo.port_stat_change_t.old_internal_poe_status = poe_port_status_old[port_index].pd_status_internal;
            uSysInfo.port_stat_change_t.new_internal_poe_status = port_status.pd_status_internal;
            uSysInfo.port_stat_change_t.pd_type_sspd_dspd       = 0;
            poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePORT_STAT_CHANGE, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */
        }


        // unknown State -> !unknown State
        if ((poe_port_status_old[port_index].pd_status == VTSS_APPL_POE_UNKNOWN_STATE)    // old status is On
            && (port_status.pd_status != VTSS_APPL_POE_UNKNOWN_STATE)                     // new status is Fault/Searching
            && (port_status.pd_status != VTSS_APPL_POE_NO_PD_DETECTED)                    // ignore the detetction stage which occur once on startup
            && (!bSyslogSent ))
            //(port_status.pd_status != VTSS_APPL_POE_DISABLED))                         // new status not disabled
        {
            T_I("syslog port#%d, unknown State -> !unknown State", port_index);


#if defined(VTSS_SW_OPTION_SYSLOG)
            uPOE_SYSLOG_INFO uSysInfo;
            uSysInfo.port_stat_change_t.port_index              = port_index;
            uSysInfo.port_stat_change_t.old_poe_port_stat       = poe_port_status_old[port_index].pd_status;
            uSysInfo.port_stat_change_t.new_poe_port_stat       = port_status.pd_status;
            uSysInfo.port_stat_change_t.old_internal_poe_status = poe_port_status_old[port_index].pd_status_internal;
            uSysInfo.port_stat_change_t.new_internal_poe_status = port_status.pd_status_internal;
            uSysInfo.port_stat_change_t.pd_type_sspd_dspd       = 0;
            poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePORT_STAT_CHANGE, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */
        }

        // Check PoE port status for led update
        vtss_appl_poe_led_analyze(&poe_status, port_index, &poe_led);

        T_DG_PORT(VTSS_TRACE_GRP_STATUS, port_index, "power_requested = %d (via class:%X, %X)",
                  port_status.power_requested_mw,
                  port_status.assigned_pd_class_a,
                  port_status.assigned_pd_class_b);

        // Work around because the pd69200 PoE card sets some ports error state when no PD is connected.
        // This is still being investigated.
        if (port_status.pd_status == VTSS_APPL_POE_UNKNOWN_STATE) {
            port_status.power_requested_mw = 0;
        }

        // If PoE is disabled for a port then it shall of course request 0 W.
        if (poe_mode_get(port_index) == VTSS_APPL_POE_MODE_DISABLED ||
            poe_is_chip_found(port_index, __FUNCTION__, __LINE__) == MEBA_POE_NO_CHIPSET_FOUND  ||
            port_status.pd_status == VTSS_APPL_POE_NO_PD_DETECTED) {
            port_status.power_requested_mw = 0;
        }

        T_DG_PORT(VTSS_TRACE_GRP_STATUS, port_index, "Port status = %d, Requested Power = %d [mw],",
                  port_status.pd_status,
                  port_status.power_requested_mw);

        T_DG_PORT(VTSS_TRACE_GRP_STATUS,
                  port_index,
                  "Class = %d-%d, power_reserved_dw = %d, power requested_mw = %d",
                  port_status.assigned_pd_class_a,
                  port_status.assigned_pd_class_b,
                  port_status.power_reserved_dw,
                  port_status.power_requested_mw);

        // Signal to LLDP module to transmit LLDP information as soon as possible.
#ifdef VTSS_SW_OPTION_LLDP
        if (poe_custom_new_pd_detected_get(port_index, FALSE)) {
            (void) lldp_poe_fast_start(port_index, FALSE);
        }
#endif
        poe_status.calculated_total_power_used_mw     += port_status.power_consume_mw;
        poe_status.calculated_total_current_used_ma   += port_status.current_consume_ma;
        poe_status.calculated_total_power_reserved_dw += port_status.power_reserved_dw;

        T_NG_PORT(VTSS_TRACE_GRP_STATUS, port_index, "Total power:%d, total_current_ma:%d, total reserved_dw:%d",
                  poe_status.power_consumption_w, poe_status.calculated_total_current_used_ma, poe_status.calculated_total_power_reserved_dw);
    }

    // Set the color for PoE status led
    vtss_appl_poe_status_led_set(poe_led);

    poe_status_set(&poe_status); // Save local variable poe_status to global

    vtss_appl_poe_status_mutex_unlock();
}

//
// Debug functions for debugging i2c issues
//
static BOOL poe_halt_at_i2c_err_v = FALSE;
void poe_halt_at_i2c_err_ena(void)
{
    poe_halt_at_i2c_err_v = TRUE;
}

void poe_halt_at_i2c_err(BOOL i2c_err)
{
    if (i2c_err && poe_halt_at_i2c_err_v) {
        T_E("Halting due to i2c error");
        POE_CRIT_ENTER();
        while (1) {
            // Loop forever in order to capture the last i2c access with a probe.
        }
    }
}
/*************************************************************************
** Message functions
*************************************************************************/
static vtss::Lock    thread_lock;

/*********************************************************************
** Power management
*********************************************************************/

uint32_t vtss_appl_poe_supply_default_get(void)
{
    meba_poe_psu_input_prob_t psu = {};
    if (meba_poe_supply_limits_get(board_instance, MEBA_POE_CTRL_PSU_ALL, &psu) != VTSS_RC_OK) {
        T_I("vtss_appl_poe_supply_default_get - exit meba_poe_supply_limits_get");
        return 0;
    }

#if defined(USE_POE_STATIC_PARAMETERS)
    if (psu.user_configurable) {
        // The user needs to specify a value, default is 0
        return 0;
    }
#endif //USE_POE_STATIC_PARAMETERS

    return psu.def_w;
}

uint32_t vtss_appl_poe_supply_max_get(void)
{
    meba_poe_psu_input_prob_t psu = {};
    if (meba_poe_supply_limits_get(board_instance, MEBA_POE_CTRL_PSU_ALL, &psu) != VTSS_RC_OK) {
        return 0;
    }

    return psu.max_w;
}

uint32_t vtss_appl_poe_system_power_usage_get(void)
{
    meba_poe_psu_input_prob_t psu  = {};
    if (meba_poe_supply_limits_get(board_instance, MEBA_POE_CTRL_PSU_ALL, &psu) != VTSS_RC_OK) {
        return 0;
    }

    T_I("psu.system_pwr_usage_w=%d", psu.system_pwr_usage_w);
    return psu.system_pwr_usage_w;
}

mesa_bool_t vtss_appl_poe_psu_user_configurable_get(void)
{
    // In case that the PoE chipset is not detected yet, we simply allow to setup PoE. This is needed because startup config is applied before the
    // PoE chipset is detected.
    if (!is_poe_ready()) {
        return TRUE;
    }

    meba_poe_psu_input_prob_t psu  = {};
    if (meba_poe_supply_limits_get(board_instance, MEBA_POE_CTRL_PSU_ALL, &psu) != VTSS_RC_OK) {
        return 0;
    }

    return psu.user_configurable;
}

mesa_bool_t vtss_appl_poe_pd_legacy_mode_configurable_get(void)
{
    meba_poe_ctrl_cap_t capabilities = {};
    if (meba_poe_capabilities_get(board_instance, &capabilities) == MESA_RC_OK) {
        return (capabilities & MEBA_POE_CTRL_CAP_PD_LEGACY_DETECTION);
    } else {
        return false;
    }
}

mesa_bool_t vtss_appl_poe_pd_interruptible_power_get(void)
{
    // In case that the PoE chipset is not detected yet, we simply allow to setup PoE. This is needed because startup config is applied before the
    // PoE chipset is detected.
    if (!is_poe_ready()) {
        return TRUE;
    }

    meba_poe_ctrl_cap_t capabilities = {};
    if (meba_poe_capabilities_get(board_instance, &capabilities) == MESA_RC_OK) {
        return ((capabilities & MEBA_POE_CTRL_INTERRUPTIBLE_POWER) != 0);
    } else {
        return false;
    }
}


mesa_bool_t vtss_appl_poe_pd_auto_class_request_get(void)
{
    // In case that the PoE chipset is not detected yet, we simply allow to setup PoE. This is needed because startup config is applied before the
    // PoE chipset is detected.
    if (!is_poe_ready()) {
        return TRUE;
    }

    meba_poe_ctrl_cap_t capabilities = {};
    if (meba_poe_capabilities_get(board_instance, &capabilities) == MESA_RC_OK) {
        return ((capabilities & MEBA_POE_CTRL_PD_AUTO_CLASS_REQUEST) != 0);
    } else {
        return false;
    }
}


mesa_bool_t vtss_appl_poe_legacy_pd_class_mode_get(void)
{
    // In case that the PoE chipset is not detected yet, we simply allow to setup PoE. This is needed because startup config is applied before the
    // PoE chipset is detected.
    if (!is_poe_ready()) {
        return TRUE;
    }

    meba_poe_ctrl_cap_t capabilities = {};
    if (meba_poe_capabilities_get(board_instance, &capabilities) == MESA_RC_OK) {
        return ((capabilities & MEBA_POE_CTRL_LEGACY_PD_CLASS_MODE) != 0);
    } else {
        return false;
    }
}


uint32_t vtss_appl_poe_supply_min_get(void)
{
    meba_poe_psu_input_prob_t psu = {};
    if (meba_poe_supply_limits_get(board_instance, MEBA_POE_CTRL_PSU_ALL, &psu) != VTSS_RC_OK) {
        return 0;
    }

    return psu.min_w;
}

/**********************************************************************
** Configuration
**********************************************************************/
static void poe_conf_default_set(void)
{
    uint              port_index;

    T_RG(VTSS_TRACE_GRP_CONF, "Entering");

    // Set default configuration
    T_NG(VTSS_TRACE_GRP_CONF, "Restoring to default value");
    poe_conf_t poe_conf;

    // First reset whole structure...
    vtss_clear(poe_conf);

    // Set the individual fields that need to be something other than 0
    poe_conf.power_supply_max_power_w = vtss_appl_poe_supply_default_get();
    poe_conf.system_power_usage_w     = vtss_appl_poe_system_power_usage_get();
    poe_conf.cap_detect               = POE_CAP_DETECT_DISABLED;

    if (vtss_appl_poe_pd_interruptible_power_get()) { // support poe_pd_interruptible_power
        poe_conf.interruptible_power = POE_PD_INTERRUPTIBLE_POWER_DEFAULT;
    }

    if (vtss_appl_poe_pd_auto_class_request_get()) {
        poe_conf.pd_auto_class_request = POE_PD_AUTO_CLASS_REQUEST_DEFAULT;
    }

    if ( vtss_appl_poe_legacy_pd_class_mode_get()) {
        poe_conf.global_legacy_pd_class_mode = POE_LEGACY_PD_CLASS_MODE_DEFAULT;
    }

    // Set default PoE for all ports
    for (port_index = 0; port_index < fast_cap(MEBA_CAP_BOARD_PORT_COUNT); port_index++) {
        poe_conf.bt_pse_port_type[port_index] = POE_TYPE_DEFAULT;
        poe_conf.poe_mode[port_index]         = POE_MODE_DEFAULT;
        poe_conf.bt_port_pm_mode[port_index]  = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DEFAULT;
        poe_conf.priority[port_index]         = POE_PRIORITY_DEFAULT;
        poe_conf.lldp_disable[port_index]     = POE_LLDP_DISABLE_DEFAULT;
        poe_conf.cable_length[port_index]     = POE_CABLE_LENGTH_DEFAULT;
    }
    poe_config_set(&poe_conf);
}


// Return TRUE if any PoE chipset is found.
BOOL poe_is_any_chip_found(void)
{
    for (auto i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_COUNT); i++) {
        if (poe_is_chip_found(i, __FUNCTION__, __LINE__) != MEBA_POE_NO_CHIPSET_FOUND) {
            return TRUE;
        }
    }
    return FALSE;
}


#if !defined(USE_POE_STATIC_PARAMETERS)
BOOL prod_db_read_done = FALSE;

// flag to enable poe thread to resume its operation depend on products db parameters
// (applicable only when poe obtains its parameters from prod process)
void prod_db_file_read_done()
{
    // release poe tread
    prod_db_read_done = TRUE;
}
#endif //USE_POE_STATIC_PARAMETERS


// in case poe_status_update() was called by lldp_tlv - so data is already updated -
// so we will skip the poe_status_update()
bool bDo_Poe_status = true;


/**********************************************************************
** Thread
**********************************************************************/

static vtss_handle_t poe_thread_handle;
static vtss_thread_t poe_thread_block;

meba_poe_init_params_t poe_init_params;

static void poe_thread(vtss_addrword_t data)
{
    int total_port_count   = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);
    poe_init_params.use_poe_static_parameters = TRUE;
    // if poe parameters are applied from product then we need to sync product and poe threads
#if !defined(USE_POE_STATIC_PARAMETERS)
    // Wait until IGFG will finish to configure all the threads with their startup-config values
    while (!prod_db_read_done) {
        VTSS_OS_MSLEEP(500);
    }

    poe_init_params.use_poe_static_parameters = FALSE;
#endif //USE_POE_STATIC_PARAMETERS

    mesa_rc rc;
    critd_init(&poe_status_crit, "poe_status", VTSS_MODULE_ID_POE, CRITD_TYPE_MUTEX);
    poe_status_crit.max_lock_time = 600; // When doing "reload default" many times in a row (100+)
    // The reading of status from PoE module can take very long time.

    // init ports state whether its poe with UNKNOWN state or not poe ports - NOT_SUPPORTED
    poe_status_t poe_status;
    poe_conf_t poe_local_conf;    // local copy of current configuration.

    poe_status_get(&poe_status);

    for (mesa_port_no_t iport = 0 ; iport < total_port_count; iport++) {
        BOOL poe_available = port_custom_table[iport].poe_support;
        T_D("port: %d , poe_available: %d\n", iport, poe_available);

        // Only set PoE if PoE is available for this port
        if (poe_available) {
            poe_status.port_status[iport].pd_status = VTSS_APPL_POE_UNKNOWN_STATE;
        } else {
            poe_status.port_status[iport].pd_status = VTSS_APPL_POE_NOT_SUPPORTED;
        }
    }

    poe_status_set(&poe_status); // Save local variable poe_status to global
    /* Do the board-initialize, and wait 2 seconds to allow for the the PD69200 and the i2c bus to get up and running.
     * For some reason, not doing it here (or in the port module), but only in main/vtss_api_if.cxx makes the PD69200
     * communication fail, unless there is an SFP-module inserted during boot.
     */

    T_D("Send MEBA_POE_INITIALIZE");
    meba_reset(board_instance, MEBA_POE_INITIALIZE);
    VTSS_OS_MSLEEP(2000);
    T_D("done MEBA_POE_INITIALIZE");

    // Detecting PoE chip set. Has to be called even on PoE less boards to initialise the PoE crit.
    // loop until POE CONTROLLER WAS FOUND
    if (MESA_RC_OK != poe_custom_init(&poe_init_params)) { // Detecting PoE chip set. Has to be called even on PoE less boards to initialise the PoE crit.
        poe_init_done = TRUE; // Signal that PoE chipset has been initialized.
        return; // No chip found
    }

    T_I("PoE-Control: PoE detection done");

#if defined(VTSS_SW_OPTION_SYSLOG)
    poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_CONTROLLER_FOUND, NULL);
#endif /* VTSS_SW_OPTION_SYSLOG */

    if (!fast_cap(MEBA_CAP_POE)) {
        T_I("No PoE Board feature");
        poe_init_done = TRUE; // Signal that PoE chipset has been initialized.
        return; // Stop thread if no PoE chipsets is found.
    }

    int poe_supply = vtss_appl_poe_supply_default_get();
    int system_power_usage_w = vtss_appl_poe_system_power_usage_get();
    T_I("poe_supply=%d, system_power_usage_w=%d", poe_supply, system_power_usage_w);

    poe_config_get(&poe_local_conf);
    poe_local_conf.system_power_usage_w = system_power_usage_w;
    poe_config_set(&poe_local_conf);

    if (poe_supply != 0) {
        // When poe supply not is user configurable, make sure the default value is used.
        poe_config_get(&poe_local_conf);
        poe_local_conf.power_supply_max_power_w = poe_supply;
        poe_config_set(&poe_local_conf);
    }

    // Wait until configuration has been applied
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_POE);
    poe_config_get(&poe_local_conf);

    T_D("Firmware upgrade");
    int dummy;

    if (poe_local_conf.firmware[0] == 0) {
        rc = poe_do_firmware_upgrade(NULL, dummy, TRUE, FALSE);
    } else {
        rc = poe_do_firmware_upgrade(poe_local_conf.firmware, dummy, FALSE, FALSE);
    }

    // Note - Firmware is upgraded in the case that it doesn't match the built in version or missing/damaged
    if (rc != MESA_RC_ERR_POE_FIRMWARE_IS_UP_TO_DATE) {
        T_E("No valid PoE ICs were found or missing/damaged firmware - Exit PoE thread, error: %s", error_txt(rc));

#ifdef VTSS_SW_OPTION_SYSLOG
        uPOE_SYSLOG_INFO uSysInfo;
        uSysInfo.exit_thread_t.error_msg = "No valid PoE ICs";
        poe_Send_SysLog_Msg(eSYSLOG_LVL_ERROR, ePOE_EXIT_POE_THREAD, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */

        return;
    }

    mesa_restart_t  restart_cause = poe_get_system_reset_cause();
    mesa_bool_t interruptible_power = poe_local_conf.interruptible_power;
    T_I("PoE interruptible_power= %d , restart_cause= %d", interruptible_power, restart_cause);

    // set parameters: Pwr supply
    poe_custom_init_chips(interruptible_power, restart_cause);

    if (!poe_is_any_chip_found()) {
        T_I("No PoE ICs were found - Exit PoE thread");
        poe_init_done = TRUE; // Signal that PoE chipset has been initialized.

#if defined(VTSS_SW_OPTION_SYSLOG)
        uPOE_SYSLOG_INFO uSysInfo;
        uSysInfo.exit_thread_t.error_msg = "No valid PoE ICs";
        poe_Send_SysLog_Msg(eSYSLOG_LVL_ERROR, ePOE_EXIT_POE_THREAD, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */

        return; // Stop thread if no PoE chipsets is found.
    }

    poe_status_update();

    //if (report_once_no_poe_controller_found == TRUE)
    thread_lock.wait();
    POE_CRIT_ENTER();
    poe_init_done = TRUE; // Signal that PoE chipset has been initialized.
    POE_CRIT_EXIT();

    struct sysinfo SysInfo;
    memset(&SysInfo, 0x0, sizeof(SysInfo));
    sysinfo(&SysInfo);
    T_I("PoE init done on Linux system time: %ld", SysInfo.uptime);

    poe_mgmt_get_status(&poe_status);
    T_I("ports count=%d, max poe ports=%d", total_port_count, poe_status.max_number_of_poe_ports);

    // update periodically poe status every 3 seconds
    for (;;) {
        thread_lock.lock(true); // Set thread to locked state
        thread_lock.timed_wait(vtss_current_time() + 3000); // wait for thread to be unlocked

        // in case poe_status_update() was called by lldp_tlv - so data is already updated.
        // so we will skip the poe_status_update()
        if (bDo_Poe_status) {
            // Do thread stuff
            poe_status_update();
        }

        bDo_Poe_status = true;
    }
}

/**********************************************************************
** Wrapper-methods for hardware-access.
**********************************************************************/


BOOL pd69200bt_poe_info_get(poe_info_t *poe_info)
{
    poe_status_t poe_status;
    //poe_conf_t poe_conf;

    poe_status_get(&poe_status);
    // poe_config_get(&poe_conf);

    //vtss_appl_poe_port_status_t &port_status = poe_status.port_status[port_index];

    if (!poe_is_any_chip_found()) {
        sprintf(poe_info->Firmware_version, "Fail to detect PoE ICs");
        //sprintf(poe_info->Product_name,   "Fail to detect PoE ICs");
        //sprintf(poe_info->SN,             "Fail to detect PoE ICs");
    } else {
        sprintf(poe_info->Firmware_version, "%02d%04d.%04d.%02d.%04d.%03d",
                poe_status.prod_number_detected,
                poe_status.sw_version_high_detected,
                poe_status.sw_version_low_detected,
                poe_status.param_number_detected,
                poe_status.internal_sw_number,
                poe_status.build_number);

        //sprintf(poe_info->Product_name, "%04d", poe_status.PN);
        //sprintf(poe_info->SN, "%06lu", (unsigned long)poe_status.UN);
    }

    return TRUE;
}

poe_entry_t poe_hw_config_get(mesa_port_no_t port_idx, poe_entry_t *hw_conf)
{
    meba_poe_port_cap_t capabilities = {};
    mesa_rc rc = meba_poe_port_capabilities_get(board_instance, port_idx, &capabilities);
    if (rc != MESA_RC_OK) {
        hw_conf->available = FALSE;
        return *hw_conf;
    }

    hw_conf->available = capabilities & MEBA_POE_PORT_CAP_POE;
    hw_conf->pse_pairs_control_ability = ((capabilities & MEBA_POE_PORT_CAP_2PAIR_CONTROL) == capabilities);
    hw_conf->pse_power_pair = 1; // 1 = Signal pins pinout, 2 =  Spare pins pinout, ee pethPsePortPowerPairs, rfc 3621
    return *hw_conf;
}

BOOL poe_port_lldp_disabled(mesa_port_no_t port_index)
{
    poe_conf_t tmp_conf;
    poe_config_get(&tmp_conf);
    return tmp_conf.lldp_disable[port_index] ? TRUE : FALSE;
}

void poe_pse_data_get(mesa_port_no_t port_index, poe_pse_data_t *pse_data)
{
    poe_status_t local_poe_status;
    T_IG_PORT(VTSS_TRACE_GRP_MGMT, port_index, "%s", "Enter");
    poe_status_get(&local_poe_status);
    *pse_data = local_poe_status.poe_pse_data[port_index]; // power in deciwatt
}

void poe_pd_data_set(mesa_port_no_t port_index, poe_pd_data_t *pd_data)
{
    T_IG_PORT(VTSS_TRACE_GRP_MGMT, port_index, "%s", "Enter");

    poe_status_t poe_status;
    poe_conf_t poe_conf;

    poe_status_get(&poe_status);
    poe_config_get(&poe_conf);

    vtss_appl_poe_port_status_t &port_status = poe_status.port_status[port_index];

    mesa_poe_milliwatt_t pd_requested_power_mw  = pd_data->pd_requested_power_dw * 100;     // dw -> mw

#ifdef LLDP_LOG_FILTER
    if (port_status.previous_pd_request_power.single_mw != pd_requested_power_mw)
#endif //LLDP_LOG_FILTER
    {
        T_I("NEW LLDP-AT: port# %d, Set Max Power SINGLE=%d -> %d mW, Cable length = %d meter",
            port_index,
            port_status.previous_pd_request_power.single_mw,
            pd_requested_power_mw,
            poe_conf.cable_length[port_index] * 10);

        port_status.previous_pd_request_power.single_mw = pd_requested_power_mw;
        poe_status_set(&poe_status); // Save local variable poe_status to global

#ifdef VTSS_SW_OPTION_SYSLOG
        uPOE_SYSLOG_INFO uSysInfo;
        uSysInfo.lldp_pd_power_request_t.port_index    = port_index;
        uSysInfo.lldp_pd_power_request_t.is_bt_port    = false;
        uSysInfo.lldp_pd_power_request_t.single_mw     = pd_requested_power_mw;
        uSysInfo.lldp_pd_power_request_t.cable_length  = poe_conf.cable_length[port_index];
        poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_LLDP_PD_REQUEST, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */
    }

    // sending lldp PD request
    poe_custom_pd_data_set(port_index, pd_data);
    bDo_Poe_status = false;
    VTSS_OS_MSLEEP(300);

    // ensure the poe lldp message from PSE is ready
    for (int i = 0 ; i < 3 ; i++) {
        poe_status_update(); // reading lldp PSE data
        poe_status_get(&poe_status);
        if (poe_status.poe_pse_data[port_index].layer2_execution_status == 1) { // Layer2 LLDP/CDP request pending
            T_I("port#%d lldp packet pending count:%d", port_index, (i + 1));
            bDo_Poe_status = false;
            VTSS_OS_MSLEEP(200);
        } else { // message is ready
            break;
        }
    }
}


void poe_pd_bt_data_set(mesa_port_no_t port_index, poe_pd_bt_data_t *pd_data)
{
    T_IG_PORT(VTSS_TRACE_GRP_MGMT, port_index, "%s", "Enter");

    poe_status_t poe_status;
    poe_conf_t poe_conf;

    poe_status_get(&poe_status);
    poe_config_get(&poe_conf);
    vtss_appl_poe_port_status_t &port_status = poe_status.port_status[port_index];

    mesa_poe_milliwatt_t pd_requested_power_single_mw  = pd_data->pd_requested_power_dw * 100;     // dw -> mw
    mesa_poe_milliwatt_t pd_requested_power_alt_a_mw   = pd_data->requested_power_mode_a_dw * 100; // dw -> mw
    mesa_poe_milliwatt_t pd_requested_power_alt_b_mw   = pd_data->requested_power_mode_b_dw * 100; // dw -> mw

#ifdef LLDP_LOG_FILTER
    if ((port_status.previous_pd_request_power.single_mw != pd_requested_power_single_mw) ||
        (port_status.previous_pd_request_power.alt_a_mw != pd_requested_power_alt_a_mw) ||
        (port_status.previous_pd_request_power.alt_b_mw != pd_requested_power_alt_b_mw))
#endif //LLDP_LOG_FILTER
    {
        T_I("NEW LLDP-BT: port# %d ,Set Max Power SINGLE=%d -> %d mW, ALT-A=%d -> %d mW, ALT-B=%d -> %d mW. Cable length = %d meter",
            port_index,
            port_status.previous_pd_request_power.single_mw,
            pd_requested_power_single_mw,
            port_status.previous_pd_request_power.alt_a_mw,
            pd_requested_power_alt_a_mw,
            port_status.previous_pd_request_power.alt_b_mw,
            pd_requested_power_alt_b_mw,
            poe_conf.cable_length[port_index] * 10);

        port_status.previous_pd_request_power.single_mw = pd_requested_power_single_mw;
        port_status.previous_pd_request_power.alt_a_mw  = pd_requested_power_alt_a_mw;
        port_status.previous_pd_request_power.alt_b_mw  = pd_requested_power_alt_b_mw;

        poe_status_set(&poe_status); // Save local variable poe_status to global

#ifdef VTSS_SW_OPTION_SYSLOG
        uPOE_SYSLOG_INFO uSysInfo;
        uSysInfo.lldp_pd_power_request_t.port_index    = port_index;
        uSysInfo.lldp_pd_power_request_t.is_bt_port    = true;
        uSysInfo.lldp_pd_power_request_t.single_mw     = pd_requested_power_single_mw;
        uSysInfo.lldp_pd_power_request_t.alt_a_mw      = pd_requested_power_alt_a_mw;
        uSysInfo.lldp_pd_power_request_t.alt_b_mw      = pd_requested_power_alt_b_mw;
        uSysInfo.lldp_pd_power_request_t.cable_length  = poe_conf.cable_length[port_index];
        poe_Send_SysLog_Msg(eSYSLOG_LVL_NOTICE, ePOE_LLDP_PD_REQUEST, &uSysInfo);
#endif /* VTSS_SW_OPTION_SYSLOG */
    }

    // sending lldp PD request
    poe_custom_pd_bt_data_set(port_index, pd_data);
    bDo_Poe_status = false;
    VTSS_OS_MSLEEP(300);

    // ensure the poe lldp message from PSE is ready
    for (int i = 0 ; i < 3 ; i++) {
        poe_status_update(); // reading lldp PSE data
        poe_status_get(&poe_status);
        if (poe_status.poe_pse_data[port_index].layer2_execution_status == 1) { // Layer2 LLDP/CDP request pending
            T_I("port#%d lldp packet pending count:%d", port_index, (i + 1));
            bDo_Poe_status = false;
            VTSS_OS_MSLEEP(200);
        } else { // message is ready
            break;
        }
    }
}


BOOL poe_new_pd_detected_get(mesa_port_no_t port_index, BOOL clear)
{
    bool val = poe_custom_new_pd_detected_get(port_index, clear);
    T_IG_PORT(VTSS_TRACE_GRP_MGMT, port_index, "%s clear %u val %u", "Exit", clear, val);
    return val;
}

char *poe_firmware_info_get(uint32_t max_size, char *info)
{
    return poe_custom_firmware_info_get(max_size, info);
}

/**********************************************************************
** Management functions
**********************************************************************/
// Returns TRUE if the PoE chips are detected and initialized, else FALSE.
BOOL is_poe_ready(void)
{
    T_NG(VTSS_TRACE_GRP_STATUS, "poe_init_done:%d", poe_init_done);
    return poe_init_done;
}

BOOL poe_mgmt_is_backup_power_supported(void)
{
    return poe_custom_is_backup_power_supported();
}

meba_poe_power_source_t poe_mgmt_get_power_source(void)
{
    poe_status_t local_status;
    poe_status_get(&local_status);
    return local_status.pwr_in_status1 ? MEBA_POE_POWER_SOURCE_PRIMARY : MEBA_POE_POWER_SOURCE_BACKUP;
}

// Function for getting if PoE chipset is found (Critical region protected)
void poe_mgmt_is_chip_found(meba_poe_chip_state_t *chip_found)
{
    poe_status_t local_poe_status;
    poe_mgmt_get_status(&local_poe_status);
    memcpy(chip_found, local_poe_status.poe_chip_found.data(), local_poe_status.poe_chip_found.mem_size());
}

// Function that returns the current configuration.
void poe_config_get(poe_conf_t *conf)
{
    T_NG(VTSS_TRACE_GRP_CONF, "Getting local conf");
    POE_CRIT_ENTER();
    *conf = my_private_poe_conf; // Return this switch configuration
    POE_CRIT_EXIT();
}

// Function that can set the current configuration.
mesa_rc poe_config_set(poe_conf_t *new_conf)
{
    T_DG(VTSS_TRACE_GRP_CONF, "Setting local conf");

    // Update new configuration, update the switch in question
    POE_CRIT_ENTER();
    my_private_poe_conf = *new_conf;
    poe_conf_updated = true;
    POE_CRIT_EXIT();
    thread_lock.lock(false); // Unlock poe thread to changes are applied as fast as possible
    return VTSS_RC_OK;
}

bool poe_config_updated()
{
    POE_CRIT_ENTER();
    bool updated_ = poe_conf_updated;
    poe_conf_updated = false;
    POE_CRIT_EXIT();
    return updated_;
}

// Getting status for mangement
void poe_mgmt_get_status(poe_status_t *local_poe_status)
{
    if (!poe_init_done) {
        vtss_clear(*local_poe_status);
        return;
    }
    poe_status_get(local_poe_status);
}

mesa_rc vtss_appl_poe_conf_set(vtss_usid_t usid, const vtss_appl_poe_conf_t  *const conf)
{
    poe_conf_t tmp_poe_conf;

    if ((conf == NULL) || (usid != VTSS_USID_START)) {
        T_I("Input parameter is NULL");
        return VTSS_APPL_POE_ERROR_NULL_POINTER;
    }

    if (conf->power_supply_max_power_w > vtss_appl_poe_supply_max_get() ||
        conf->power_supply_max_power_w < vtss_appl_poe_supply_min_get()   ) {
        return VTSS_APPL_POE_ERROR_PRIM_SUPPLY_RANGE;
    }

    // Get current conf
    poe_config_get(&tmp_poe_conf);

    // Update fields
    strncpy(tmp_poe_conf.firmware, conf->firmware, sizeof(tmp_poe_conf.firmware));
    tmp_poe_conf.power_supply_max_power_w    = conf->power_supply_max_power_w;
    tmp_poe_conf.system_power_usage_w        = conf->system_power_usage_w;
    tmp_poe_conf.cap_detect                  = (conf->capacitor_detect ? POE_CAP_DETECT_ENABLED : POE_CAP_DETECT_DISABLED);
    tmp_poe_conf.interruptible_power         = conf->interruptible_power;
    tmp_poe_conf.pd_auto_class_request       = conf->pd_auto_class_request;
    tmp_poe_conf.global_legacy_pd_class_mode = (conf->global_legacy_pd_class_mode == 0) ? VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_STANDARD : (conf->global_legacy_pd_class_mode == 1) ? VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_POH : VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_IGNORE_PD_CLASS;

    // Set conf
    poe_config_set(&tmp_poe_conf);
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_poe_conf_get(vtss_usid_t usid, vtss_appl_poe_conf_t *const conf)
{
    poe_conf_t tmp_poe_conf;
    if ((conf == NULL) || (usid != VTSS_USID_START)) {
        T_I("Input parameter is NULL");
        return VTSS_APPL_POE_ERROR_NULL_POINTER;
    }
    //Get config
    poe_config_get(&tmp_poe_conf);

    // Update fields
    conf->power_supply_max_power_w    = tmp_poe_conf.power_supply_max_power_w;
    conf->system_power_usage_w        = tmp_poe_conf.system_power_usage_w;
    conf->capacitor_detect            = tmp_poe_conf.cap_detect;
    conf->interruptible_power         = tmp_poe_conf.interruptible_power;
    conf->pd_auto_class_request       = tmp_poe_conf.pd_auto_class_request;
    conf->global_legacy_pd_class_mode = tmp_poe_conf.global_legacy_pd_class_mode;
    strncpy(conf->firmware, tmp_poe_conf.firmware, sizeof(conf->firmware));
    return VTSS_RC_OK;
}

/**
 * Set PoE port configuration.
 * ifIndex  [IN]: Interface index
 * conf     [IN]: PoE port configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_poe_port_conf_set(vtss_ifindex_t                   ifIndex,
                                    const vtss_appl_poe_port_conf_t  *const conf)
{
    CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
    vtss_ifindex_elm_t  ife;
    poe_conf_t          tmp_conf;

    if (conf == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_APPL_POE_ERROR_NULL_POINTER;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))
    if (ife.isid != VTSS_ISID_START) {
        return VTSS_APPL_POE_ERROR_NOT_SUPPORTED;
    }
    poe_mgmt_is_chip_found(&poe_chip_found[0]);

    poe_config_get(&tmp_conf);

    if (poe_chip_found[ife.ordinal] != MEBA_POE_NO_CHIPSET_FOUND) {
        tmp_conf.bt_pse_port_type[ife.ordinal]    = conf->bt_pse_port_type;
        tmp_conf.poe_mode[ife.ordinal]            = conf->poe_port_mode;
        tmp_conf.bt_port_pm_mode[ife.ordinal]     = conf->bt_port_pm;
        tmp_conf.priority[ife.ordinal]            = conf->priority;
        tmp_conf.lldp_disable[ife.ordinal]        = conf->lldp_disable;
        tmp_conf.cable_length[ife.ordinal]        = conf->cable_length;
        //Set configuration
        VTSS_RC(poe_config_set(&tmp_conf));
    } else {
        T_D("Interface: %u does not have PoE support", ife.ordinal);
        return VTSS_APPL_POE_ERROR_NOT_SUPPORTED;
    }
    return VTSS_RC_OK;
}
/**
 * Get PoE port configuration.
 * ifIndex   [IN]: Interface index
 * conf     [OUT]: PoE port configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_poe_port_conf_get(vtss_ifindex_t            ifIndex,
                                    vtss_appl_poe_port_conf_t *const conf)
{
    CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
    vtss_ifindex_elm_t  ife;
    poe_conf_t          tmp_conf;

    if (conf == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_APPL_POE_ERROR_NULL_POINTER;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))
    if (ife.isid != VTSS_ISID_START) {
        return VTSS_APPL_POE_ERROR_NOT_SUPPORTED;
    }

    poe_mgmt_is_chip_found(&poe_chip_found[0]);

    if (poe_chip_found[ife.ordinal] != MEBA_POE_NO_CHIPSET_FOUND) {
        poe_config_get(&tmp_conf);
        conf->bt_pse_port_type = tmp_conf.bt_pse_port_type[ife.ordinal];
        conf->poe_port_mode    = tmp_conf.poe_mode[ife.ordinal];
        conf->bt_port_pm       = tmp_conf.bt_port_pm_mode[ife.ordinal];
        conf->priority         = tmp_conf.priority[ife.ordinal];
        conf->lldp_disable     = tmp_conf.lldp_disable[ife.ordinal];
        conf->cable_length     = tmp_conf.cable_length[ife.ordinal];
    } else {
        T_D("Interface: %u does not have PoE support", ife.ordinal);
        return VTSS_APPL_POE_ERROR_NOT_SUPPORTED;
    }
    return VTSS_RC_OK;
}
/**
 * Get PoE port status
 * ifIndex     [IN]: Interface index
 * status     [OUT]: Port status
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_poe_port_status_get(vtss_ifindex_t              ifIndex,
                                      vtss_appl_poe_port_status_t *const status)
{
    poe_status_t        local_status;
    CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
    vtss_ifindex_elm_t  ife;

    /* Check illegal parameters */
    if (status == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_APPL_POE_ERROR_NULL_POINTER;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))
    if (ife.isid != VTSS_ISID_START) {
        return VTSS_APPL_POE_ERROR_NOT_SUPPORTED;
    }

    poe_mgmt_is_chip_found(&poe_chip_found[0]);

    if (poe_chip_found[ife.ordinal] != MEBA_POE_NO_CHIPSET_FOUND) {
        poe_mgmt_get_status(&local_status);
        *status = local_status.port_status[ife.ordinal];
    } else {
        T_D("Interface: %u does not have PoE support", ife.ordinal);
        return VTSS_APPL_POE_ERROR_NOT_SUPPORTED;
    }
    return VTSS_RC_OK;
}
/**
 * Get PoE port capabilities
 * ifIndex       [IN]: Interface index
 * capabilities [OUT]: PoE platform specific port capabilities
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_poe_port_capabilities_get(vtss_ifindex_t                    ifIndex,
                                            vtss_appl_poe_port_capabilities_t *const capabilities)
{
    vtss_ifindex_elm_t  ife;
    CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;

    /* Check illegal parameters */
    if (capabilities == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_APPL_POE_ERROR_NULL_POINTER;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))
    if (ife.isid != VTSS_ISID_START) {
        return VTSS_APPL_POE_ERROR_NOT_SUPPORTED;
    }
    poe_mgmt_is_chip_found(&poe_chip_found[0]);

    if (poe_chip_found[ife.ordinal] == MEBA_POE_NO_CHIPSET_FOUND) {
        capabilities->poe_capable = false;
    } else {
        capabilities->poe_capable = true;
    }
    return VTSS_RC_OK;
}

/**
 * \brief Get PSU capabilities
 *
 * \param capabilities [OUT]: PoE platform specific PSU capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_poe_psu_capabilities_get(
    vtss_appl_poe_psu_capabilities_t *const capabilities)
{
    poe_status_t local_status;
    poe_status_get(&local_status);

    meba_poe_psu_input_prob_t psu = {};
    VTSS_RC(meba_poe_supply_limits_get(board_instance, MEBA_POE_CTRL_PSU_ALL, &psu));
    capabilities->user_configurable             = psu.user_configurable;
    capabilities->max_power_w                   = psu.max_w;
    capabilities->system_reserved_power_w       = psu.system_pwr_usage_w;
    capabilities->legacy_mode_configurable      = vtss_appl_poe_pd_legacy_mode_configurable_get();
    capabilities->interruptible_power_supported = vtss_appl_poe_pd_interruptible_power_get();
    capabilities->pd_auto_class_request         = vtss_appl_poe_pd_auto_class_request_get();
    capabilities->legacy_pd_class_mode          = vtss_appl_poe_legacy_pd_class_mode_get();
    capabilities->is_bt                         = local_status.is_bt;
    return VTSS_RC_OK;
}

/**
 * \brief Function for getting the current power supply status
 *
 * \param pwr_in_status [IN,OUT] The status of the power supply.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_poe_powerin_status_get(vtss_appl_poe_powerin_status_t *const powerin_status)
{
    poe_status_t local_status;
    poe_status_get(&local_status);

    poe_mgmt_get_status(&local_status);
    powerin_status->calculated_total_current_used_ma   = local_status.calculated_total_current_used_ma;
    powerin_status->calculated_total_power_used_mw     = local_status.calculated_total_power_used_mw;
    powerin_status->calculated_total_power_reserved_dw = local_status.calculated_total_power_reserved_dw;

    powerin_status->power_consumption_w = local_status.power_consumption_w;
    powerin_status->calculated_power_w  = local_status.calculated_power_w;
    powerin_status->available_power_w   = local_status.available_power_w;
    powerin_status->power_limit_w       = local_status.power_limit_w;
    powerin_status->power_bank          = local_status.power_bank;
    powerin_status->vmain_voltage_dv    = local_status.vmain_voltage_dv;
    powerin_status->imain_current_ma    = local_status.imain_current_ma;

    return VTSS_RC_OK;
}



// Function converting the port status to a printable string.
const char *poe_icli_port_status2str(vtss_appl_poe_status_t status)
{
    if (!poe_init_done) {
        return "Initializing";
    } else {
        switch (status) {
        case VTSS_APPL_POE_DISABLED :
            return "Disabled";

        case VTSS_APPL_POE_POWER_BUDGET_EXCEEDED:
            return "Off Exceeded";

        case VTSS_APPL_POE_UNKNOWN_STATE:
            return "UnKnown";

        case VTSS_APPL_POE_NO_PD_DETECTED:
            return "No PD Detected";

        case VTSS_APPL_POE_PD_ON:
            return "On";

        case VTSS_APPL_POE_PD_OVERLOAD:
            return "Off PD-Overload";

        case VTSS_APPL_POE_PD_FAULT:
            return "Off Pd-Fault";

        case VTSS_APPL_POE_PSE_FAULT:
            return "Off PSE-Fault";

        case VTSS_APPL_POE_DISABLED_INTERFACE_SHUTDOWN:
            return "Off Shutdown";

        default :
            // This should never happen.
            T_I("Unknown PoE status:%d", status);
            return "state unknown";
        }
    }
}

// Function converting the class to a printable string.
char *poe_class2str(const poe_status_t *status, u8 assigned_pd_class_a, u8 assigned_pd_class_b, mesa_port_no_t port_index, char *class_str)
{
    char *tmp = class_str;
    T_DG_PORT(VTSS_TRACE_GRP_STATUS, port_index, "port_status:%d, pd_class:%x-%x",
              status->port_status[port_index].pd_status,
              assigned_pd_class_a,
              assigned_pd_class_b);
    if (status->port_status[port_index].pd_status != VTSS_APPL_POE_PD_ON &&
        status->port_status[port_index].pd_status != VTSS_APPL_POE_POWER_BUDGET_EXCEEDED &&
        status->port_status[port_index].pd_status != VTSS_APPL_POE_PD_OVERLOAD) {
        sprintf(class_str, "--");
        return class_str;
    }

    switch (status->port_status[port_index].pd_structure) {
    case MEBA_POE_PORT_PD_STRUCTURE_2P_DUAL_4P_CANDIDATE_FALSE:
    case MEBA_POE_PORT_PD_STRUCTURE_2P_IEEE:
    case MEBA_POE_PORT_PD_STRUCTURE_2P_LEGACY:
    case MEBA_POE_PORT_PD_STRUCTURE_4P_SINGLE_IEEE:
    case MEBA_POE_PORT_PD_STRUCTURE_4P_SINGLE_LEGACY:
        if (assigned_pd_class_a < 0) {
            sprintf(class_str, "--");
        } else {
            sprintf(class_str, "%d", assigned_pd_class_a);
        }
        break;
    case MEBA_POE_PORT_PD_STRUCTURE_NOT_PERFORMED:
    case MEBA_POE_PORT_PD_STRUCTURE_OPEN:
    case MEBA_POE_PORT_PD_STRUCTURE_INVALID_SIGNATURE:
        sprintf(class_str, "--");
        break;
    case MEBA_POE_PORT_PD_STRUCTURE_4P_DUAL_IEEE:
        if (assigned_pd_class_a < 0) {
            tmp += sprintf(tmp, "--");
        } else {
            tmp += sprintf(tmp, "%d", assigned_pd_class_a);
        }
        tmp += sprintf(tmp, ", ");
        if (assigned_pd_class_b < 0) {
            tmp += sprintf(tmp, "--");
        } else {
            tmp += sprintf(tmp, "%d", assigned_pd_class_b);
        }
        break;
    default:
        sprintf(class_str, "?");
    }

    return class_str;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void poe_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_poe_json_init(void);
#endif
extern "C" int poe_icli_cmd_register();

/* Initialize module */
mesa_rc poe_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_I("INIT_CMD_INIT");
        critd_init(&crit, "PoE", VTSS_MODULE_ID_POE, CRITD_TYPE_MUTEX);

        POE_CRIT_ENTER();

        if (fast_cap(MEBA_CAP_POE)) {
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
            /* Register private mib */
            poe_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
            vtss_appl_poe_json_init();
#endif
            poe_icli_cmd_register();
        }
#ifdef VTSS_SW_OPTION_WEB
        if (!fast_cap(MEBA_CAP_POE)) {
            /* Hide POE pages if we are not POE-capable */
            web_page_disable("poe_config.htm");
            web_page_disable("poe_status.htm");
#if defined VTSS_SW_OPTION_LLDP
            web_page_disable("lldp_poe_neighbors.htm");
#endif  /* VTSS_SW_OPTION_LLDP */
        }
#endif /* VTSS_SW_OPTION_WEB */

        POE_CRIT_EXIT();

        critd_init(&crit_status, "PoE status", VTSS_MODULE_ID_POE, CRITD_TYPE_MUTEX);

        poe_conf_default_set();
#ifdef VTSS_SW_OPTION_ICFG
        if (fast_cap(MEBA_CAP_POE) && poe_icfg_init() != VTSS_RC_OK) {
            T_E("ICFG not initialized correctly");
        }
#endif

        T_I("INIT_CMD_INIT");
        break;

    case INIT_CMD_START:
        thread_lock.lock(false);
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           poe_thread,
                           0,
                           "POE",
                           nullptr,
                           0,
                           &poe_thread_handle,
                           &poe_thread_block);
        break;

    case INIT_CMD_CONF_DEF:
        if (fast_cap(MEBA_CAP_POE)) {
            poe_conf_default_set(); // Restore to default.
        }
        break;

    case INIT_CMD_SUSPEND_RESUME:
        POE_CRIT_ENTER();
        thread_lock.lock(!data->resume);
        POE_CRIT_EXIT();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

