/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

// Avoid "cli_grp_help.h not used in module system_cli.c"
/*lint --e{766} */

#include "main.h"
#include "conf_api.h"
#include "icli_api.h"
#include "msg_api.h"
#include "misc_api.h"
#include "control_api.h"
#include "sysutil_api.h"
#include "topo_api.h"
#ifdef VTSS_SW_OPTION_FIRMWARE
#include "firmware_icli_functions.h"
#include "firmware_api.h"
#endif
#ifdef VTSS_SW_OPTION_POE
#include "poe_api.h"
#include "poe_icli_functions.h"
#endif
#include "sysutil_icli_func.h"
#include "icli_porting_util.h"
#include "vtss_os_wrapper.h"
#include "vtss_mtd_api.hxx"
#ifdef VTSS_SW_OPTION_SYNCE_DPLL
#include "synce_custom_clock_api.h"
#include "synce_types.h"
#endif

#ifdef VTSS_SW_OPTION_FIRMWARE
static char *vtss_flash_type()
{
    if (firmware_is_nor_only()) {
        return "NOR-only";
    } if (firmware_is_mmc()) {
        return "MMC-only";
    } if (firmware_is_nand_only()) {
        return "NAND-only";
    } else {
        return "NOR/NAND";
    }
}
#endif

/* System configuration */
void sysutil_icli_func_conf(
    u32     session_id
)
{
    uchar                   mac[6];
    msg_switch_info_t       info;
    vtss_isid_t             isid;
    system_conf_t           conf;
    mesa_restart_status_t   status;
    char                    buf[32];
    const char              *code_rev;
#if defined(VTSS_SW_OPTION_SYNCE_DPLL) && defined(VTSS_SW_OPTION_ZLS)
    meba_synce_clock_fw_ver_t dpll_fw_ver;
#endif

    ICLI_PRINTF("\n");

#if CYGINT_ISO_MALLINFO
{
    struct mallinfo mem_info;

    mem_info = mallinfo();

    ICLI_PRINTF("MEMORY           : Total=%d KBytes, Free=%d KBytes, Max=%d KBytes\n",
        mem_info.arena / 1024, mem_info.fordblks / 1024, mem_info.maxfree / 1024);
}
#endif /* CYGINT_ISO_MALLINFO */

#if defined(notdef)
    FILE *procmtd;
    char mtdentry[128];
    if ((procmtd = fopen("/proc/mtd", "r"))) {
        while (fgets(mtdentry, sizeof(mtdentry), procmtd)) {
            ICLI_PRINTF("%s", mtdentry);
        }
        fclose(procmtd);
    }
#endif

    if (conf_mgmt_mac_addr_get(mac, 0) >= 0) {
        ICLI_PRINTF("MAC Address      : %s\n", misc_mac_txt(mac, buf));
    }

    if (control_system_restart_status_get(NULL, &status) == VTSS_RC_OK) {
        strcpy(buf, control_system_restart_to_str(status.restart));
        buf[0] = toupper(buf[0]);
        ICLI_PRINTF("Previous Restart : %s\n", buf);
    }

    ICLI_PRINTF("\n");

    if (system_get_config(&conf) == VTSS_RC_OK) {
        ICLI_PRINTF("System Contact   : %s\n", conf.sys_contact);
        ICLI_PRINTF("System Name      : %s\n", conf.sys_name);
        ICLI_PRINTF("System Location  : %s\n", conf.sys_location);
#ifndef VTSS_SW_OPTION_DAYLIGHT_SAVING
        ICLI_PRINTF("Timezone Offset  : %d\n", conf.tz_off);
#endif
    }

    ICLI_PRINTF("System Time      : %s\n", misc_time2str(time(NULL)));
    ICLI_PRINTF("System Uptime    : %s\n", icli_time_txt(VTSS_OS_TICK2MSEC(vtss_current_time())/1000));

    ICLI_PRINTF("\n");
#ifdef VTSS_SW_OPTION_FIRMWARE
    firmware_icli_show_version( session_id );
#endif
    for ( isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++ ) {
        if ( msg_switch_info_get(isid, &info) == VTSS_RC_OK ) {
            ICLI_PRINTF("\n");
            ICLI_PRINTF("------------------\n");
            ICLI_PRINTF("SID : %-2d\n",    topo_isid2usid(isid));
            ICLI_PRINTF("------------------\n");
            ICLI_PRINTF("Chipset ID       : %s\n", misc_chip_id_txt());
            ICLI_PRINTF("Board Type       : %s\n", vtss_board_name());
#ifdef VTSS_SW_OPTION_FIRMWARE
            ICLI_PRINTF("Flash Type       : %s\n", vtss_flash_type());
#endif
            ICLI_PRINTF("Port Count       : %u\n", info.info.port_cnt);
            ICLI_PRINTF("Product          : %s\n", info.product_name);
            ICLI_PRINTF("Software Version : %s\n", misc_software_version_txt());
            ICLI_PRINTF("Build Date       : %s\n", misc_software_date_txt());

            code_rev = misc_software_code_revision_txt();
            if ( strlen(code_rev) ) {
                // version.c is always compiled, this file is not, so we must
                // check for whether there's something in the code revision
                // string or not. Only version.c knows about the CODE_REVISION
                // environment variable.
                ICLI_PRINTF("Code Revision    : %s\n", code_rev);
            }
        }
    }
#ifdef VTSS_SW_OPTION_POE
    if (fast_cap(MEBA_CAP_POE) || fast_cap(MEBA_CAP_POE_BT)) {
        char info[500];
        ICLI_PRINTF("PoE Version      : %s\n", poe_firmware_info_get(sizeof(info), &info[0]));
    }
#endif
#if defined(VTSS_SW_OPTION_SYNCE_DPLL) && defined(VTSS_SW_OPTION_ZLS)
    if (clock_dpll_fw_ver_get(&dpll_fw_ver) == VTSS_RC_OK) {
      ICLI_PRINTF("DPLL F/W Version : %08x\n", dpll_fw_ver);
    } else {
      ICLI_PRINTF("DPLL F/W Version : -\n");
    }
#endif
    ICLI_PRINTF("\n");
}

static BOOL is_inventory_support(int *type)
{
    return FALSE;
}

/* Only XXXXXXX support inventory CLI */
BOOL icli_inventory_present(IN u32 session_id, IN icli_runtime_ask_t ask, OUT icli_runtime_t *runtime)
{
    switch ( ask ) {
    case ICLI_ASK_PRESENT:
        runtime->present = is_inventory_support( NULL );
        return TRUE;
    case ICLI_ASK_BYWORD:
    case ICLI_ASK_HELP:
    case ICLI_ASK_RANGE:
    default:
        break;
    }

    return FALSE;
}

#if defined(VTSS_APPL_SYSUTIL_TM_SUPPORTED)
const char *get_sensor_type_name(vtss_appl_sysutil_tm_sensor_type_t sensor)
{
    switch (sensor) {
    case VTSS_APPL_SYSUTIL_TM_SENSOR_BOARD:
        return "board";
    case VTSS_APPL_SYSUTIL_TM_SENSOR_JUNCTION:
        return "junction";
    default:
        return "Error";
    }
}
#endif
