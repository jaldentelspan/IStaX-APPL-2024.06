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
#ifdef VTSS_SW_OPTION_POE

#include "poe_api.h"
#include "poe.h" // For trace
#include "poe_trace.h"

#include "icli_api.h"
#include "icli_porting_util.h"
#include "poe_icli_functions.h"

#include "icfg_api.h" // For vtss_icfg_query_request_t
#include "misc_api.h" // for uport2iport
#include "msg_api.h" // For msg_switch_exists
#include "mgmt_api.h" //mgmt_str_float2long
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#include "vtss_tftp_api.h"
#include "poe_options_cfg.h"

/***************************************************************************/
/*  Type defines                                                           */
/***************************************************************************/
// Type used for selecting which poe icli configuration to update for the common function.
typedef enum {VTSS_POE_ICLI_MODE,       // dis , std ,legacy
              VTSS_POE_ICLI_PRIORITY,
              VTSS_POE_ICLI_POWER_MANAGEMENT,
              VTSS_POE_ICLI_TYPE,       // 4 types 15W - 90W
              VTSS_POE_ICLI_POWER_LIMIT,
              VTSS_POE_ICLI_LLDP,
              VTSS_POE_ICLI_CABLE_LENGTH
             } vtss_poe_icli_conf_t; // Which configuration do to

// Used to passed the PoE configuration value for the corresponding configuration type.
typedef union {
  // Mode
  struct {
    BOOL poe;
    BOOL poe_plus;
  } mode;

  // Priority
  struct {
    BOOL low;
    BOOL high;
    BOOL critical;
  } priority;

  // power_management
  struct {
    BOOL pm_dynamic;
    BOOL pm_static;
    BOOL pm_hybrid;
  } power_management;

  // PoE type
  struct {
    BOOL type3Pwr15w;
    BOOL type3Pwr30w;
    BOOL type3Pwr60w;
    BOOL type4Pwr90w;
  } bt_pse_port_type;

  // cable length
  struct {
    BOOL cableLength10;
    BOOL cableLength30;
    BOOL cableLength60;
    BOOL cableLength100;
  } cable_length;

  // Power limit
  char *power_limit_value;
} poe_conf_value_t;

// PoE power supply leds
extern vtss_appl_poe_led_t power1_led_color;
extern vtss_appl_poe_led_t power2_led_color;

// PoE status led
extern vtss_appl_poe_led_t status_led_color;

// Used to passed the PoE configuration value for the corresponding configuration type.
/***************************************************************************/
/*  Internal functions                                                     */
/****************************************************************************/
// Checking if PoE is supported for a least one of the interface currently selected
// IN : session_id - Current session id
// Return TRUE if at least one interface supporting PoE is found within the currently selected interfaces, else FALSE
static BOOL poe_icli_supported(u32 session_id)
{
  icli_variable_value_t       value;
  icli_stack_port_range_t     *plist;

  if (ICLI_MODE_PARA_GET(NULL, &value) != ICLI_RC_OK) {
    ICLI_PRINTF("%% Fail to get mode para\n");
    return FALSE;
  }

  CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;

  plist = &(value.u.u_port_type_list); // port list

  if (plist->cnt >= ICLI_RANGE_LIST_CNT) {
    T_E("Something wrong with plist, defaulting to all ports. plist->cnt:%d", plist->cnt);
    plist = NULL;
  }

  // In case that the PoE chipset is not detected yet, we simply allow to setup PoE. This is needed because startup config is applied before the
  // PoE chipset is detected.
  if (!is_poe_ready()) {
    return TRUE;
  }

  port_iter_t pit;
  poe_mgmt_is_chip_found(&poe_chip_found[0]); // Get a list with which PoE chip there is associated to the port.

  VTSS_RC(icli_port_iter_init(&pit, VTSS_ISID_START, PORT_ITER_FLAGS_ALL));
  while (icli_port_iter_getnext(&pit, plist)) {
    T_NG_PORT(VTSS_TRACE_GRP_ICLI, pit.iport, "found:%d", poe_chip_found[pit.iport]);

    if (poe_chip_found[pit.iport] != MEBA_POE_NO_CHIPSET_FOUND) {
      T_NG(VTSS_TRACE_GRP_ICLI, "found");
      return TRUE;
    }
  }

  T_NG(VTSS_TRACE_GRP_ICLI, "Not found");
  return FALSE;
}


char *bt_port_pm_mode_to_str(u8 bt_port_pm_mode)
{
  if (bt_port_pm_mode == VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC) {
    return "dynamic";
  } else if (bt_port_pm_mode == VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_STATIC) {
    return "static";
  } else if (bt_port_pm_mode == VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_HYBRID) {
    return "hybrid";
  } else {
    return "error";
  }
}



char *pse_port_type_to_str(u8 bt_pse_port_type)
{
  if (bt_pse_port_type == VTSS_APPL_POE_PSE_PORT_TYPE3_15W) {
    return "type3-15W";
  } else if (bt_pse_port_type == VTSS_APPL_POE_PSE_PORT_TYPE3_30W) {
    return "type3-30W";
  } else if (bt_pse_port_type == VTSS_APPL_POE_PSE_PORT_TYPE3_60W) {
    return "type3-60W";
  } else if (bt_pse_port_type == VTSS_APPL_POE_PSE_PORT_TYPE4_90W) {
    return "type4-90W";
  } else {
    return "error";
  }
}


// Function for checking is if PoE is supported for a specific port. If PoE isn't supported for the port, a printout is done.
// In : session_id - session_id of ICLI_PRINTF
//      iport - Internal port
//      uport - User port number
//      isid  - Internal switch id.
// Return: TRUE if PoE chipset is found for the iport, else FALSE
static BOOL is_poe_supported(i32 session_id, mesa_port_no_t iport, mesa_port_no_t uport, vtss_usid_t usid, meba_poe_chip_state_t *poe_chip_found)
{
  if (poe_chip_found[iport] == MEBA_POE_NO_CHIPSET_FOUND) {
    T_DG_PORT(VTSS_TRACE_GRP_ICLI, iport, "PoE NOT supported:%d", poe_chip_found[iport]);
    return FALSE;
  } else {
    T_DG_PORT(VTSS_TRACE_GRP_ICLI, iport, "PoE supported:%d", poe_chip_found[iport]);
    return TRUE;
  }
}

// Help function for printing out status
// In : session_id - Session_Id for ICLI_PRINTF
//      debug  - Set to TRUE in order to get more PoE information printed
//      has_interface - TRUE if user has specified a specific interface
//      list - List of interfaces (ports)
static void poe_status(i32 session_id, const switch_iter_t *sit, BOOL debug, BOOL has_interface, icli_stack_port_range_t *plist)
{
  port_iter_t        pit;
  poe_status_t       status;
  char               txt_string1[50];
  char               txt_string2[50];
  char               txt_string3[50];
  char               measured_class_string[10];
  char               allocated_class_string[10];
  char               buf[250];

  // Header

  // print header line 1
  ICLI_PRINTF("%-23s %-10s %-5s %-7s %-11s %-10s %-11s %-11s %-10s %-16s %-40s \n\r", "",          "PSE",  "Oper", "Pwr Mng", "Measured", "Assigned", "Power [W]", "Power",     "Power",    "Port",   "Port");

  // print header line 2
  ICLI_PRINTF("%-23s %-10s %-5s %-7s %-11s %-10s %-11s %-11s %-10s %-16s %-40s \n\r", "Interface", "Type", "Mode", "Mode",    "PD-Class", "PD-Class", "Requested", "Alloc [W]", "Used [W]", "Status", "Internal-Status");

  // print header line 3
  ICLI_PRINTF("----------------------  ---------  ----  ------  ----------  ---------  ----------  ----------  ---------  ---------------  ---------------------- \n\r");

  if (sit->isid != VTSS_ISID_START) {
    return;
  }
  poe_mgmt_get_status(&status); // Update the status fields

  poe_conf_t poe_conf;
  poe_config_get(&poe_conf);

  CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
  poe_mgmt_is_chip_found(&poe_chip_found[0]); // Get a list with which PoE chip there is associated to the port.

  // Loop through all front ports
  if (icli_port_iter_init(&pit, sit->isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_SORT_ORDER_IPORT) == VTSS_RC_OK) {
    while (icli_port_iter_getnext(&pit, plist)) {
      if (is_poe_supported(session_id, pit.iport, pit.uport, sit->usid, &poe_chip_found[0])) {
        meba_poe_port_cap_t cap;
        if (meba_poe_port_capabilities_get(board_instance, pit.iport, &cap) != VTSS_RC_OK) {
          continue;
        }
        (void) icli_port_info_txt(sit->usid, pit.uport, buf);  // Get interface as printable string
        vtss_appl_poe_port_status_t &ps = status.port_status[pit.iport];
        poe_class2str(&status, ps.measured_pd_class_a, ps.measured_pd_class_b, pit.iport, &measured_class_string[0]);  // measured class
        poe_class2str(&status, ps.assigned_pd_class_a, ps.assigned_pd_class_b, pit.iport, &allocated_class_string[0]); // assigned class

        ICLI_PRINTF("%-23s %-10s 0x%-3X %-7s %-11s %-10s %-11s %-11s %-10s %-16s %-40s\n",
                    &buf[0],                                                          // interface
                    pse_port_type_to_str(ps.cfg_pse_port_type),                       // PD type
                    ps.cfg_bt_port_operation_mode,                                    // bt port operation mode
                    bt_port_pm_mode_to_str(ps.cfg_bt_port_pm_mode),                   // bt port pm mode
                    measured_class_string,                                            // measured class
                    allocated_class_string,                                           // assigned class
                    one_digi_float2str(ps.power_requested_mw / 100, &txt_string1[0]), // power limit
                    one_digi_float2str(ps.power_assigned_mw / 100, &txt_string2[0]),  // power allocated
                    one_digi_float2str(ps.power_consume_mw / 100, &txt_string3[0]),   // power used
                    poe_icli_port_status2str(ps.pd_status),                           // port status
                    ps.pd_status_internal_description);                               // port internal-status
      }
    }
  }

  ICLI_PRINTF("-----                                                                               -----       ----- \n\r");

  ICLI_PRINTF("%-23s %-10s %-5s %-7s %-11s %-10s %-11s %-11d %-10d %-16s %-40s \n\r",
              "Total",
              "",
              "",
              "",
              "",
              "",
              "",
              status.calculated_power_w,
              status.power_consumption_w,
              "",
              "");
}


// Help function for printing out poe error counters
// In : session_id - Session_Id for ICLI_PRINTF
//      debug  - Set to TRUE in order to get more PoE information printed
//      has_interface - TRUE if user has specified a specific interface
//      list - List of interfaces (ports)
static void poe_debug_show_poe_error_counters(i32 session_id, const switch_iter_t *sit, BOOL debug, BOOL has_interface, icli_stack_port_range_t *plist)
{
  port_iter_t        pit;
  poe_status_t       status;
  char               buf[250];

  // Header

  // print header line 1
  ICLI_PRINTF("%-23s %-9s %-10s %-9s %-10s %-9s \n\r", " ", "udl", "ovl", "short", "invalid", "power");

  // print header line 2
  ICLI_PRINTF("%-23s %-9s %-10s %-9s %-10s %-9s \n\r",  "Interface", " ",    " ",     "circuit",    "signature", "denied");

  // print header line 3
  ICLI_PRINTF("----------------------  --------  ---------  --------  ---------  --------\n\r");

  if (sit->isid != VTSS_ISID_START) {
    return;
  }
  poe_mgmt_get_status(&status); // Update the status fields

  poe_conf_t poe_conf;
  poe_config_get(&poe_conf);

  CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
  poe_mgmt_is_chip_found(&poe_chip_found[0]); // Get a list with which PoE chip there is associated to the port.

  // Loop through all front ports
  if (icli_port_iter_init(&pit, sit->isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_SORT_ORDER_IPORT) == VTSS_RC_OK) {
    while (icli_port_iter_getnext(&pit, plist)) {
      if (is_poe_supported(session_id, pit.iport, pit.uport, sit->usid, &poe_chip_found[0])) {
        (void) icli_port_info_txt(sit->usid, pit.uport, buf);  // Get interface as printable string
        vtss_appl_poe_port_status_t &ps = status.port_status[pit.iport];

        ICLI_PRINTF("%-23s %-9d %-10d %-9d %-10d %-9d \n\r",
                    &buf[0],
                    ps.bt_port_counters.udl_count,
                    ps.bt_port_counters.ovl_count,
                    ps.bt_port_counters.sc_count,
                    ps.bt_port_counters.invalid_signature_count,
                    ps.bt_port_counters.power_denied_count);
      }
    }
  }

  //ICLI_PRINTF("%-23s %-9s %-10s %-9s %-10s %-9s %-9s %-11s \n\r",
  //            "Total",
  //            " ",
  //            " ",
  //            " ",
  //            " ",
  //            " ",
  //            " ",
  //            " ");
}


// Help function for setting poe mode
// In : has_poe - TRUE is PoE port power shall be poe mode
//      has_poe_plus - TRUE if PoE port power shall be poe+ mode.
//      iport - Port in question
//      no - TRUE if mode shall be set to default.
//      poe_conf - Pointer to the current configuration.
static void poe_icli_mode_conf(const poe_conf_value_t *poe_conf_value, mesa_port_no_t iport, BOOL no, poe_conf_t *poe_conf)
{
  T_DG_PORT(VTSS_TRACE_GRP_ICLI, iport, "poe:%d, poe_plus:%d, no:%d", poe_conf_value->mode.poe, poe_conf_value->mode.poe_plus, no);
  // Update mode
  if (poe_conf_value->mode.poe) {
    poe_conf->poe_mode[iport] = VTSS_APPL_POE_MODE_POE;
  } else if (poe_conf_value->mode.poe_plus) {
    poe_conf->poe_mode[iport] = VTSS_APPL_POE_MODE_POE_PLUS;
  } else if (no) {
    poe_conf->poe_mode[iport] = VTSS_APPL_POE_MODE_DISABLED;
  }
}


// Help function for printing out status
// In : session_id - Session_Id for ICLI_PRINTF
//      debug  - Set to TRUE in order to get more PoE information printed
//      has_interface - TRUE if user has specified a specific interface
//      list - List of interfaces (ports)
static void poe_debug_status(i32 session_id, const switch_iter_t *sit, BOOL debug, BOOL has_interface, icli_stack_port_range_t *plist)
{
  port_iter_t        pit;
  poe_status_t       status;
  char               txt_string1[50];
  char               txt_string2[50];
  char               txt_string3[50];
  char               measured_class_string[10];
  char               requested_class_string[10];
  char               allocated_class_string[10];
  char               buf[250];

  // Header

  // print header line 1
  ICLI_PRINTF("%-23s %-9s %-10s %-9s %-10s %-9s %-9s %-11s \n\r", "",          "Measured", "Requested", "Assigned", "Power",     "Power",    "Power",    "Current");

  // print header line 2
  ICLI_PRINTF("%-23s %-9s %-10s %-9s %-10s %-9s %-9s %-11s \n\r", "Interface", "Class",    "Class",     "Class",    "Requested", "Assigned", "Reserved", "Used [mA]");

  // print header line 3
  ICLI_PRINTF("----------------------  --------  ---------  --------  ---------  --------  --------  ----------\n\r");

  if (sit->isid != VTSS_ISID_START) {
    return;
  }
  poe_mgmt_get_status(&status); // Update the status fields

  poe_conf_t poe_conf;
  poe_config_get(&poe_conf);

  CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
  poe_mgmt_is_chip_found(&poe_chip_found[0]); // Get a list with which PoE chip there is associated to the port.

  // Loop through all front ports
  if (icli_port_iter_init(&pit, sit->isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_SORT_ORDER_IPORT) == VTSS_RC_OK) {
    while (icli_port_iter_getnext(&pit, plist)) {
      if (is_poe_supported(session_id, pit.iport, pit.uport, sit->usid, &poe_chip_found[0])) {
        (void) icli_port_info_txt(sit->usid, pit.uport, buf);  // Get interface as printable string
        vtss_appl_poe_port_status_t &ps = status.port_status[pit.iport];

        poe_class2str(&status, ps.measured_pd_class_a, ps.measured_pd_class_b, pit.iport, &measured_class_string[0]);     // measured  class
        poe_class2str(&status, ps.requested_pd_class_a, ps.requested_pd_class_b, pit.iport, &requested_class_string[0]);  // requested class
        poe_class2str(&status, ps.assigned_pd_class_a, ps.assigned_pd_class_b, pit.iport, &allocated_class_string[0]);    // assigned  class

        ICLI_PRINTF("%-23s %-9s %-10s %-9s %-10s %-9s %-9s %-11d \n\r",
                    &buf[0],                                                           // interface
                    measured_class_string,                                             // measured  class
                    requested_class_string,                                            // requested class
                    allocated_class_string,                                            // assigned  class
                    one_digi_float2str(ps.power_requested_mw / 100, &txt_string1[0]),  // Power Requested
                    one_digi_float2str(ps.power_assigned_mw / 100, &txt_string2[0]),   // Power assigned (from PoE MCU)
                    one_digi_float2str(ps.power_reserved_dw, &txt_string3[0]),         // Power reserved (UNG)
                    ps.current_consume_ma);
      }
    }
  }

  ICLI_PRINTF("%-23s %-9s %-10s %-9s %-10s %-9d %-9s %-11d \n\r",
              "Total",
              "",
              "",
              "",
              "",
              status.calculated_power_w,                            // total assigned power (from PoE MCU)
              one_digi_float2str(status.calculated_total_power_reserved_dw, &txt_string1[0]), // Power reserved (UNG calculation)
              status.calculated_total_current_used_ma);             // total current
}


// Function for configuring PoE priority
// IN - has_low - TRUE is priority
static void poe_icli_priority_conf(const poe_conf_value_t *poe_conf_value, mesa_port_no_t iport, BOOL no, poe_conf_t *poe_conf)
{
  // Update mode
  if (poe_conf_value->priority.low) {
    poe_conf->priority[iport] = VTSS_APPL_POE_PORT_POWER_PRIORITY_LOW;
  } else if (poe_conf_value->priority.high) {
    poe_conf->priority[iport] = VTSS_APPL_POE_PORT_POWER_PRIORITY_HIGH;
  } else if (poe_conf_value->priority.critical) {
    poe_conf->priority[iport] = VTSS_APPL_POE_PORT_POWER_PRIORITY_CRITICAL;
  } else if (no) {
    poe_conf->priority[iport] = POE_PRIORITY_DEFAULT;
  }
}


// Function for configuring PoE cable length
// IN - has_low - TRUE is cable length
static void poe_icli_cable_length_conf(const poe_conf_value_t *poe_conf_value, mesa_port_no_t iport, BOOL no, poe_conf_t *poe_conf)
{
  // Update cable_length
  if (poe_conf_value->cable_length.cableLength10) {
    poe_conf->cable_length[iport] = VTSS_APPL_POE_PORT_CABLE_LENGTH_10;
  } else if (poe_conf_value->cable_length.cableLength30) {
    poe_conf->cable_length[iport] = VTSS_APPL_POE_PORT_CABLE_LENGTH_30;
  } else if (poe_conf_value->cable_length.cableLength60) {
    poe_conf->cable_length[iport] = VTSS_APPL_POE_PORT_CABLE_LENGTH_60;
  } else if (poe_conf_value->cable_length.cableLength100) {
    poe_conf->cable_length[iport] = VTSS_APPL_POE_PORT_CABLE_LENGTH_100;
  } else if (no) {
    poe_conf->cable_length[iport] = POE_CABLE_LENGTH_DEFAULT;
  }
}


// Function for configuring PoE priority
// IN - has_low - TRUE is priority
static void poe_icli_power_management_conf(const poe_conf_value_t *poe_conf_value, mesa_port_no_t iport, BOOL no, poe_conf_t *poe_conf)
{
  // Update mode
  if (poe_conf_value->power_management.pm_dynamic) {
    poe_conf->bt_port_pm_mode[iport] = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC;
  } else if (poe_conf_value->power_management.pm_static) {
    poe_conf->bt_port_pm_mode[iport] = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_STATIC;
  } else if (poe_conf_value->power_management.pm_hybrid) {
    poe_conf->bt_port_pm_mode[iport] = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_HYBRID;
  } else if (no) {
    poe_conf->bt_port_pm_mode[iport] = VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DEFAULT;
  }
}


// Function for configuring PoE type
// IN - has_type4Pwr90w - TRUE is Type
static void poe_icli_poe_pse_port_type_conf(const poe_conf_value_t *poe_conf_value, mesa_port_no_t iport, BOOL no, poe_conf_t *poe_conf)
{
  // Update mode
  if (poe_conf_value->bt_pse_port_type.type3Pwr15w) {
    poe_conf->bt_pse_port_type[iport] = VTSS_APPL_POE_PSE_PORT_TYPE3_15W;
  } else if (poe_conf_value->bt_pse_port_type.type3Pwr30w) {
    poe_conf->bt_pse_port_type[iport] = VTSS_APPL_POE_PSE_PORT_TYPE3_30W;
  } else if (poe_conf_value->bt_pse_port_type.type3Pwr60w) {
    poe_conf->bt_pse_port_type[iport] = VTSS_APPL_POE_PSE_PORT_TYPE3_60W;
  } else if (poe_conf_value->bt_pse_port_type.type4Pwr90w) {
    poe_conf->bt_pse_port_type[iport] = VTSS_APPL_POE_PSE_PORT_TYPE4_90W;
  } else if (no) {
    poe_conf->bt_pse_port_type[iport] = POE_TYPE_DEFAULT;
  }
}


// Function for configuring PoE Lldp
static void poe_icli_lldp_conf(mesa_port_no_t iport, BOOL no, poe_conf_t *poe_conf)
{
  if (no) {
    poe_conf->lldp_disable[iport] = VTSS_APPL_POE_PORT_LLDP_DISABLED;
  } else {
    poe_conf->lldp_disable[iport] = POE_LLDP_DISABLE_DEFAULT;
  }
}

// Common function used for setting interface configurations.
// In : session_id - Session_Id for ICLI_PRINTF
//      poe_conf_type - Selecting which configuration to update.
//      poe_conf_value - New configuration value.
//      plist - List of interfaces to update
//      no - TRUE if user wants to set configuration to default
static void poe_icli_common(i32 session_id, vtss_poe_icli_conf_t poe_conf_type,
                            const poe_conf_value_t *poe_conf_value,
                            icli_stack_port_range_t *plist, BOOL no)
{
  poe_conf_t poe_conf;
  u8 isid_cnt = 0;

  // Just making sure that we don't access NULL pointer.
  if (plist == NULL) {
    T_E("plist was unexpected NULL");
    return;
  }
  T_IG(VTSS_TRACE_GRP_ICLI, "cnt:%d", plist->cnt);

  if (plist->switch_range[isid_cnt].isid == VTSS_ISID_START) {
    // Get current configuration
    poe_config_get(&poe_conf);

    CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
    poe_mgmt_is_chip_found(&poe_chip_found[0]); // Get a list with which PoE chip there is associated to the port.

    port_iter_t        pit;
    if (icli_port_iter_init(&pit, plist->switch_range[isid_cnt].isid, PORT_ITER_FLAGS_FRONT | PORT_ITER_SORT_ORDER_IPORT) == VTSS_RC_OK) {
      T_DG(VTSS_TRACE_GRP_ICLI, "isid:%d", plist->switch_range[isid_cnt].isid);
      while (icli_port_iter_getnext(&pit, plist)) {
        T_DG_PORT(VTSS_TRACE_GRP_ICLI, pit.iport, "poe_conf_type:%d", poe_conf_type);
        // Ignore if PoE isn't supported for this port.
        if (is_poe_supported(session_id, pit.iport, pit.uport, plist->switch_range[isid_cnt].usid, &poe_chip_found[0])) {
          switch  (poe_conf_type) {
          case VTSS_POE_ICLI_MODE:
            poe_icli_mode_conf(poe_conf_value, pit.iport, no, &poe_conf);
            break;

          case VTSS_POE_ICLI_PRIORITY:
            poe_icli_priority_conf(poe_conf_value, pit.iport, no, &poe_conf);
            break;

          case VTSS_POE_ICLI_POWER_MANAGEMENT:
            poe_icli_power_management_conf(poe_conf_value, pit.iport, no, &poe_conf);
            break;

          case VTSS_POE_ICLI_TYPE:
            poe_icli_poe_pse_port_type_conf(poe_conf_value, pit.iport, no, &poe_conf);
            break;

          case VTSS_POE_ICLI_LLDP:
            poe_icli_lldp_conf(pit.iport, no, &poe_conf);
            break;

          case VTSS_POE_ICLI_CABLE_LENGTH:
            poe_icli_cable_length_conf(poe_conf_value, pit.iport, no, &poe_conf);
            break;

          default:
            T_E("Unknown poe_conf_type:%d", poe_conf_type);
          }
        }
      }
      // Set new configuration
      poe_config_set(&poe_conf);
    }
  }
}


BOOL poe_icli_runtime_power_range(u32                session_id,
                                  icli_runtime_ask_t ask,
                                  icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    runtime->present = TRUE;
    return TRUE;
  case ICLI_ASK_BYWORD:
    return TRUE;
  case ICLI_ASK_RANGE:
    // While loading startup config, poe initialization is not yet done, so just accept any range
    runtime->range.type = ICLI_RANGE_TYPE_UNSIGNED;
    runtime->range.u.sr.cnt = 1;
    runtime->range.u.sr.range[0].min = is_poe_ready() ? vtss_appl_poe_supply_min_get() : 0;
    runtime->range.u.sr.range[0].max = is_poe_ready() ? vtss_appl_poe_supply_max_get() : 5000;
    return TRUE;
  case ICLI_ASK_HELP:
    icli_sprintf(runtime->help, "The power in Watt which the PoE power supply can deliver.");
    return TRUE;
  default:
    break;
  }
  return FALSE;
}


// Function for configuring PoE power supply
//      value - New power supply value
//      no    - TRUE to restore to default
static void poe_icli_power_supply_set(i32 session_id, u32 value, BOOL no)
{
  // Get current configuration
  poe_conf_t poe_conf;

  poe_config_get(&poe_conf);

  if (no) {
    poe_conf.power_supply_max_power_w = vtss_appl_poe_supply_default_get();
  } else {
    if (is_poe_ready() && value > vtss_appl_poe_supply_max_get()) {
      ICLI_PRINTF("Maximum allowed power supply is limited to %u W\n", vtss_appl_poe_supply_max_get());
    } else {
      poe_conf.power_supply_max_power_w = value;
    }
  }
  // Set new configuration
  poe_config_set(&poe_conf);
}


// Configuring System Reserve Power
// IN - isid  - isid for the switch to configure
//      value - New system power reserve
//      no    - TRUE to restore to default
static void poe_icli_system_reserve_power_set(i32 session_id, vtss_isid_t isid, u32 value, BOOL no)
{
  // Get current configuration
  poe_conf_t poe_conf;
  if (isid != VTSS_ISID_START) {
    return;
  }
  poe_config_get(&poe_conf);

  if (no) {
    poe_conf.system_power_usage_w = vtss_appl_poe_system_power_usage_get();
  } else {
    poe_conf.system_power_usage_w = value;
  }

  // Set new configuration
  poe_config_set(&poe_conf);
}

// Function for configuring PoE power supply
// IN - isid  - isid for the switch to configure
//      value - New power supply value
//      no    - TRUE to restore to default
void poe_icli_cap_detect_set(i32 session_id, BOOL no)
{
  // Get current configuration
  poe_conf_t poe_conf;

  poe_config_get(&poe_conf);

  if (no) {
    poe_conf.cap_detect = POE_CAP_DETECT_DISABLED;
    T_IG(VTSS_TRACE_GRP_ICLI, "PoE Cap det disable");
  } else {
    T_IG(VTSS_TRACE_GRP_ICLI, "PoE Cap det enable");
    poe_conf.cap_detect = POE_CAP_DETECT_ENABLED;
  }
  // Set new configuration
  poe_config_set(&poe_conf);
}


/*
 * brief Function for configuring PoE interruptible power
 *
 * param session_id [IN] The session id use by iCLI print.
 * param val [IN] TRUE to enable interruptible power, FALSE to disable
 * return None.
 */
void poe_icli_interruptible_power_set(i32 session_id, BOOL val)
{
  // Get current configuration
  poe_conf_t poe_conf;

  poe_config_get(&poe_conf);

  poe_conf.interruptible_power = val;
  poe_config_set(&poe_conf);
}


#define ARR_SIZE 100

void poe_icli_debug_access(i32 session_id, u32 iport, char *var, u32 str_len)
{
  char title [ARR_SIZE];
  char tx_str[ARR_SIZE];
  char rx_str[ARR_SIZE];
  char msg   [ARR_SIZE];

  memset(title, '\0', ARR_SIZE);
  memset(tx_str, '\0', ARR_SIZE);
  memset(rx_str, '\0', ARR_SIZE);
  memset(msg, '\0', ARR_SIZE);

  poe_debug_access(iport, var, str_len, title, tx_str, rx_str, msg, ARR_SIZE);

  if (title[0] != '\0') {
    ICLI_PRINTF("%s\n\r", title);
  }

  if (tx_str[0] != '\0') {
    ICLI_PRINTF("%s\n\r", tx_str);
  }

  if (rx_str[0] != '\0') {
    ICLI_PRINTF("%s\n\r", rx_str);
  }

  if (msg[0] != '\0') {
    ICLI_PRINTF("%s\n\r", msg);
  }
}


/*
 * brief Function for Auto class PD
 *
 * param session_id [IN] The session id use by iCLI print.
 * param val [IN] TRUE to enable Auto class PD request, FALSE
 *        to Ignore Auto class PD requset
 * return None.
 */
void poe_icli_pd_auto_class_request_set(i32 session_id, BOOL val)
{
  // Get current configuration
  poe_conf_t poe_conf;

  poe_config_get(&poe_conf);

  poe_conf.pd_auto_class_request = val;
  poe_config_set(&poe_conf);
}


/*
 * brief Function for Auto class PD
 *
 * param session_id [IN] The session id use by iCLI print.
 *
 */
void poe_icli_legacy_pd_class_mode_set(i32 session_id, BOOL has_standard, BOOL has_poh, BOOL has_ignore_pd_class, BOOL no)
{
  T_DG(VTSS_TRACE_GRP_ICLI, "no:%d, has_poh:%d, has_standard:%d", no, has_poh, has_standard);

  // Get current configuration
  poe_conf_t poe_conf;
  poe_config_get(&poe_conf);

  // Update mode
  if (has_standard) {
    poe_conf.global_legacy_pd_class_mode = VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_STANDARD;
  } else if (has_poh) {
    poe_conf.global_legacy_pd_class_mode = VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_POH;
  } else if (has_ignore_pd_class) {
    poe_conf.global_legacy_pd_class_mode = VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_IGNORE_PD_CLASS;
  } else if (no) {
    poe_conf.global_legacy_pd_class_mode = POE_LEGACY_PD_CLASS_MODE_DEFAULT;
  }

  poe_config_set(&poe_conf);
}


/***************************************************************************/
/*  Functions called by iCLI                                                */
/****************************************************************************/
/******************************************************************************/
// PORT_ICLI_loading_startup_config()
/******************************************************************************/
BOOL poe_icli_loading_startup_config(uint32_t session_id,
                                     icli_runtime_ask_t ask,
                                     icli_runtime_t     *runtime)
{
  icli_session_way_t way;
  icli_rc_t          icli_rc;

  switch (ask) {
  case ICLI_ASK_PRESENT:
    if ((icli_rc = (icli_rc_t)icli_session_way_get(session_id, &way)) == ICLI_RC_OK &&
        way == ICLI_SESSION_WAY_APP_EXEC) {
      // Only when ICFG is applying startup-config, will way be
      // ICLI_SESSION_WAY_APP_EXEC. When normal RS232 CLI is running, way will be
      // ICLI_SESSION_WAY_THREAD_CONSOLE - also when a config is copied from flash
      // to running-config (which is kind-of a shame).
      runtime->present = TRUE;
    } else {
      if (icli_rc != ICLI_RC_OK) {
        T_E("icli_session_way_get() failed: %d", icli_rc);
        return FALSE;
      }
      runtime->present = FALSE;
    }
    return TRUE;
  default:
    return FALSE;
  }
}


// Check if PoE is supported at all
BOOL poe_icli_runtime_supported(u32                session_id,
                                icli_runtime_ask_t ask,
                                icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    T_D("fast_cap(MEBA_CAP_POE):0x%X", fast_cap(MEBA_CAP_POE));
    if (fast_cap(MEBA_CAP_POE)) {
      runtime->present = TRUE;
    } else {
      runtime->present = FALSE;
    }
    return TRUE;
  default:
    return FALSE;
  }
}


// Checks if PoE is supported for interfaces
BOOL poe_at_icli_interface_runtime_supported(u32                session_id,
                                             icli_runtime_ask_t ask,
                                             icli_runtime_t     *runtime)
{
  poe_status_t status;

  switch (ask) {
  case ICLI_ASK_PRESENT:
    poe_mgmt_get_status(&status); // Update the status fields

    T_N("fast_cap(MEBA_CAP_POE):0x%X", fast_cap(MEBA_CAP_POE));
    if (fast_cap(MEBA_CAP_POE) && (!status.is_bt)) {
      runtime->present = poe_icli_supported(session_id);
    } else {
      runtime->present = FALSE;
    }
    return TRUE;
  default:
    return FALSE;
  }
}


// Checks if PoE is supported for interfaces
BOOL poe_bt_icli_interface_runtime_supported(u32                session_id,
                                             icli_runtime_ask_t ask,
                                             icli_runtime_t     *runtime)
{
  poe_status_t status;

  switch (ask) {
  case ICLI_ASK_PRESENT:
    poe_mgmt_get_status(&status); // Update the status fields

    T_N("fast_cap(MEBA_CAP_POE):0x%X", fast_cap(MEBA_CAP_POE));
    if ((fast_cap(MEBA_CAP_POE)) &&
        ((poe_init_done == FALSE) || (status.is_bt))) {
      runtime->present = poe_icli_supported(session_id);
    } else {
      runtime->present = FALSE;
    }
    return TRUE;
  default:
    return FALSE;
  }
}


// Checks if PoE is supported for interfaces
BOOL poe_at_bt_icli_interface_runtime_supported(u32                session_id,
                                                icli_runtime_ask_t ask,
                                                icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    T_N("fast_cap(MEBA_CAP_POE):0x%X", fast_cap(MEBA_CAP_POE));
    if (fast_cap(MEBA_CAP_POE)) {
      runtime->present = poe_icli_supported(session_id);
    } else {
      runtime->present = FALSE;
    }
    return TRUE;
  default:
    return FALSE;
  }
}


// Check if PoE is supported at all
BOOL poe_icli_external_power_supply(u32                session_id,
                                    icli_runtime_ask_t ask,
                                    icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    T_N("fast_cap(MEBA_CAP_POE):0x%X", fast_cap(MEBA_CAP_POE));
    runtime->present = vtss_appl_poe_psu_user_configurable_get();
    return TRUE;
  default:
    return FALSE;
  }
}

// Check if PoE is supported at all
BOOL poe_icli_capacitor_detect(u32                session_id,
                               icli_runtime_ask_t ask,
                               icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    runtime->present = vtss_appl_poe_pd_legacy_mode_configurable_get();
    return TRUE;
  default:
    return FALSE;
  }
}


/**
 * \brief Function for at runtime getting information if PoE interruptible power is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_interruptible_power(u32                session_id,
                                  icli_runtime_ask_t ask,
                                  icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    runtime->present = vtss_appl_poe_pd_interruptible_power_get();
    return TRUE;
  default:
    return FALSE;
  }
}


/**
 * \brief Function for at runtime getting information if Auto
 *        class is active
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_pd_auto_class_request(u32                session_id,
                                    icli_runtime_ask_t ask,
                                    icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    runtime->present = vtss_appl_poe_pd_auto_class_request_get();
    return TRUE;
  default:
    return FALSE;
  }
}


/**
 * \brief Function for at runtime getting information if
 *        legacy-pd-class-mode is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_legacy_pd_class_mode(u32                session_id,
                                   icli_runtime_ask_t ask,
                                   icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    runtime->present = vtss_appl_poe_legacy_pd_class_mode_get();
    return TRUE;
  default:
    return FALSE;
  }
}


// See poe_icli_functions.h
void poe_icli_power_supply(i32 session_id, u32 value, BOOL no, vtss_poe_icli_power_conf_t power_type)
{
  vtss_isid_t isid = VTSS_ISID_START;
  switch (power_type) {
  case VTSS_POE_ICLI_POWER_SUPPLY:
    poe_icli_power_supply_set(session_id, value, no);
    break;
  case VTSS_POE_ICLI_SYSTEM_RESERVE_POWER:
    poe_icli_system_reserve_power_set(session_id, isid, value, no);
    break;
  }
}


// See poe_icli_functions.h
void poe_icli_priority(i32 session_id, BOOL has_low, BOOL has_high, BOOL has_critical, icli_stack_port_range_t *plist, BOOL no)
{
  T_DG(VTSS_TRACE_GRP_ICLI, "no:%d, has_high:%d, has_low:%d", no, has_high, has_low);
  poe_conf_value_t poe_conf_value;
  poe_conf_value.priority.low = has_low;
  poe_conf_value.priority.high = has_high;
  poe_conf_value.priority.critical = has_critical;

  poe_icli_common(session_id, VTSS_POE_ICLI_PRIORITY, &poe_conf_value, plist, no);
}


// See poe_icli_functions.h
void poe_icli_power_management(i32 session_id, BOOL has_dynamic, BOOL has_static, BOOL has_hybrid, icli_stack_port_range_t *plist, BOOL no)
{
  T_DG(VTSS_TRACE_GRP_ICLI, "no:%d, has_static:%d, has_hybrid:%d", no, has_static, has_hybrid);
  poe_conf_value_t poe_conf_value;
  poe_conf_value.power_management.pm_dynamic = has_dynamic;
  poe_conf_value.power_management.pm_static  = has_static;
  poe_conf_value.power_management.pm_hybrid  = has_hybrid;

  poe_icli_common(session_id, VTSS_POE_ICLI_POWER_MANAGEMENT, &poe_conf_value, plist, no);
}


// See poe_icli_functions.h
void poe_icli_poe_type(i32 session_id, BOOL has_type3Pwr15w, BOOL has_type3Pwr30w, BOOL has_type3Pwr60w, BOOL has_type4Pwr90w, icli_stack_port_range_t *plist, BOOL no)
{
  T_DG(VTSS_TRACE_GRP_ICLI, "no:%d, has_type3Pwr15w:%d, has_type3Pwr30w:%d, has_type3Pwr60w:%d", no, has_type3Pwr15w, has_type3Pwr30w, has_type3Pwr60w);
  poe_conf_value_t poe_conf_value;
  poe_conf_value.bt_pse_port_type.type3Pwr15w = has_type3Pwr15w;
  poe_conf_value.bt_pse_port_type.type3Pwr30w = has_type3Pwr30w;
  poe_conf_value.bt_pse_port_type.type3Pwr60w = has_type3Pwr60w;
  poe_conf_value.bt_pse_port_type.type4Pwr90w = has_type4Pwr90w;

  poe_icli_common(session_id, VTSS_POE_ICLI_TYPE, &poe_conf_value, plist, no);
}


// See poe_icli_functions.h
void poe_icli_poe_cable_length(i32 session_id, BOOL has_max10, BOOL has_max30, BOOL has_max60, BOOL has_max100, icli_stack_port_range_t *plist, BOOL no)
{
  T_DG(VTSS_TRACE_GRP_ICLI, "no:%d, has_max10:%d, has_max30:%d, has_max60:%d", no, has_max10, has_max30, has_max60);
  poe_conf_value_t poe_conf_value;
  poe_conf_value.cable_length.cableLength10 = has_max10;
  poe_conf_value.cable_length.cableLength30 = has_max30;
  poe_conf_value.cable_length.cableLength60 = has_max60;
  poe_conf_value.cable_length.cableLength100 = has_max100;

  poe_icli_common(session_id, VTSS_POE_ICLI_CABLE_LENGTH, &poe_conf_value, plist, no);
}


void poe_icli_lldp(i32 session_id, icli_stack_port_range_t *plist, BOOL no)
{
  poe_conf_value_t poe_conf_value = {};
  poe_icli_common(session_id, VTSS_POE_ICLI_LLDP, &poe_conf_value, plist, no);
}


// See poe_icli_functions.h
mesa_rc poe_icli_mode(i32 session_id, BOOL has_poe, BOOL has_poe_plus, icli_stack_port_range_t *plist, BOOL no)
{
  poe_conf_value_t poe_conf_value;
  poe_conf_value.mode.poe = has_poe;
  poe_conf_value.mode.poe_plus = has_poe_plus;

  T_DG(VTSS_TRACE_GRP_ICLI, "has_poe:%d, has_poe_plus:%d, no:%d", has_poe, has_poe_plus, no);
  poe_icli_common(session_id, VTSS_POE_ICLI_MODE, &poe_conf_value, plist, no);
  return VTSS_RC_OK;
}


// See poe_icli_functions.h
void poe_icli_halt_at_i2c_err(i32 session_id)
{
  poe_halt_at_i2c_err(TRUE);
}


// See poe_icli_functions.h
void poe_icli_show(i32 session_id, BOOL has_interface, icli_stack_port_range_t *list)
{
  switch_iter_t sit;

  if (icli_switch_iter_init(&sit) != VTSS_RC_OK) {
    return;
  }
  while (icli_switch_iter_getnext(&sit, list)) {
    if (has_interface || msg_switch_exists(sit.isid)) {
      poe_status(session_id, &sit, FALSE, has_interface, list);
    }
  }

  vtss_appl_poe_psu_capabilities_t cap;
  poe_conf_t poe_conf;
  poe_config_get(&poe_conf);

  // Provide a warning to the user if the configurable power supply is set to 0
  vtss_appl_poe_psu_capabilities_get(&cap);
  if (cap.user_configurable) {
    if (poe_conf.power_supply_max_power_w == 0) {
      ICLI_PRINTF("!! Note: poe power-supply-limit is configured to 0 Watt, and therefore can not provide power to Powered Devices.\n");
    }
  }
  if (poe_conf.system_power_usage_w > 0) {
    ICLI_PRINTF("Power reserved for system is %d Watt\n", poe_conf.system_power_usage_w );
  }
}


// See poe_icli_functions.h
void poe_icli_debug_show(i32 session_id, BOOL has_interface, icli_stack_port_range_t *list)
{
  switch_iter_t sit;

  if (icli_switch_iter_init(&sit) != VTSS_RC_OK) {
    return;
  }
  while (icli_switch_iter_getnext(&sit, list)) {
    if (has_interface || msg_switch_exists(sit.isid)) {
      poe_debug_status(session_id, &sit, FALSE, has_interface, list);
    }
  }
}


void poe_icli_debug_show_poe_error_counters(i32 session_id, BOOL has_interface, icli_stack_port_range_t *list)
{
  switch_iter_t sit;

  if (icli_switch_iter_init(&sit) != VTSS_RC_OK) {
    return;
  }
  while (icli_switch_iter_getnext(&sit, list)) {
    if (has_interface || msg_switch_exists(sit.isid)) {
      poe_debug_show_poe_error_counters(session_id, &sit, FALSE, has_interface, list);
    }
  }
}


void poe_icli_i2c_status_show(i32 session_id)
{
  poe_status_t poe_status;
  poe_mgmt_get_status(&poe_status); // Update the status fields
  //poe_status_get(&poe_status);

  if (!poe_is_any_chip_found()) {
    T_E("PoE chipset not found");
    return;
  }

  ICLI_PRINTF("%s = %u \n\r", "i2c tx error counter", poe_status.i2c_tx_error_counter );
}


void poe_icli_individual_masks_show(i32 session_id)
{
  poe_status_t poe_status;
  poe_mgmt_get_status(&poe_status); // Update the status fields

  if (!poe_is_any_chip_found()) {
    T_E("PoE chipset not found");
    return;
  }

  if (poe_status.is_bt) {
    ICLI_PRINTF("BT individual masks: \n\r");
    meba_poe_indv_mask_bt_t *im_BT = &(poe_status.tPoe_individual_mask_info.im_BT );
    ICLI_PRINTF("%-30s = %d \n\r", "ignore_high_priority", im_BT->bt_ignore_high_priority );
    ICLI_PRINTF("%-30s = %d \n\r", "support_high_res_detection", im_BT->bt_support_high_res_detection );
    ICLI_PRINTF("%-30s = %d \n\r", "i2c_restart_enable", im_BT->bt_i2c_restart_enable );
    ICLI_PRINTF("%-30s = %d \n\r", "hocpp", im_BT->bt_hocpp );
    ICLI_PRINTF("%-30s = %d \n\r", "pse_powering_pse_checking", im_BT->bt_pse_powering_pse_checking );
    ICLI_PRINTF("%-30s = %d \n\r", "single_detection_fail_event", im_BT->bt_single_detection_fail_event );
    ICLI_PRINTF("%-30s = %d \n\r", "layer2_power_allocation_limit", im_BT->bt_layer2_power_allocation_limit );
    ICLI_PRINTF("%-30s = %d \n\r", "support_lldp_half_priority", im_BT->bt_support_lldp_half_priority );
    ICLI_PRINTF("%-30s = %d \n\r", "led_stream_type", im_BT->bt_led_stream_type );
    ICLI_PRINTF("%-30s = %d \n\r", "blinks_at_invalid_signature", im_BT->bt_blinks_at_invalid_signature );
  } else {
    ICLI_PRINTF("AT individual masks: \n\r");
    meba_poe_indv_mask_prebt_t *im_prebt = &(poe_status.tPoe_individual_mask_info.im_prebt);
    ICLI_PRINTF("%-30s = %d \n\r", "ignore_higher_priority", im_prebt->prebt_ignore_higher_priority );
    ICLI_PRINTF("%-30s = %d \n\r", "supports_legacy_detection", im_prebt->prebt_supports_legacy_detection );
    ICLI_PRINTF("%-30s = %d \n\r", "message_ready_notify", im_prebt->prebt_message_ready_notify );
    ICLI_PRINTF("%-30s = %d \n\r", "layer2_priority_by_PD", im_prebt->prebt_layer2_priority_by_PD );
    ICLI_PRINTF("%-30s = %d \n\r", "matrix_support_4p", im_prebt->prebt_matrix_support_4p );
    ICLI_PRINTF("%-30s = %d \n\r", "Supports_backoff", im_prebt->prebt_supports_backoff );
    ICLI_PRINTF("%-30s = %d \n\r", "LED_stream_type", im_prebt->prebt_led_stream_type );
    ICLI_PRINTF("%-30s = %d \n\r", "PSE_powering_PSE_checking", im_prebt->prebt_pse_powering_pse_checking );
    ICLI_PRINTF("%-30s = %d \n\r", "Enable_ASIC_Refresh", im_prebt->prebt_enable_asic_refresh );
    ICLI_PRINTF("%-30s = %d \n\r", "Layer2_LLDP_enable", im_prebt->prebt_layer2_lldp_enable );
    ICLI_PRINTF("%-30s = %d \n\r", "Class_0_equal_AF", im_prebt->prebt_class_0_equal_af );
    ICLI_PRINTF("%-30s = %d \n\r", "Class_1_2_3_equal_AF", im_prebt->prebt_class_1_2_3_equal_af );
    ICLI_PRINTF("%-30s = %d \n\r", "LLDP_best_effort", im_prebt->prebt_lldp_best_effort );
    ICLI_PRINTF("%-30s = %d \n\r", "Auto_Zone2_port_activation", im_prebt->prebt_auto_zone2_port_activation );
    ICLI_PRINTF("%-30s = %d \n\r", "HOCPP_high_over_current_pulse_protection", im_prebt->prebt_hocpp_high_over_current_pulse_protection );
  }
}


/* Get the current power supply status */
void poe_icli_get_power_in_status(i32 session_id)
{
  BOOL pwr_in_status1;
  BOOL pwr_in_status2;
  mesa_sgpio_port_data_t  data[MESA_SGPIO_PORTS];

  if ( mesa_sgpio_read(NULL, 0, 0, data) == VTSS_RC_OK ) {
    pwr_in_status1 = data[2].value[0];
    pwr_in_status2 = data[3].value[0];
    ICLI_PRINTF("%10s|%10s\n", "PowerIn", "Status");
    ICLI_PRINTF("%10s|%10s\n", "1", (pwr_in_status1) ? "ON" : "OFF");
    ICLI_PRINTF("%10s|%10s\n", "2", (pwr_in_status2) ? "ON" : "OFF");
  } else {
    ICLI_PRINTF("Failed to get power supply status");
  }
}


const char *color2str(vtss_appl_poe_led_t led)
{
  switch (led) {
  case VTSS_APPL_POE_LED_OFF: {
    return "Off";
  }
  case VTSS_APPL_POE_LED_GREEN: {
    return "Green";
  }
  case VTSS_APPL_POE_LED_RED: {
    return "Red";
  }
  case VTSS_APPL_POE_LED_BLINK_RED: {
    return "BlinkRed";
  }
  default:
    return "Unknown";
  }
}


/* Get the current led color of power supplies */
void poe_icli_get_power_in_led(i32 session_id)
{
  ICLI_PRINTF("%10s|%10s\n", "PowerIn", "Led");
  ICLI_PRINTF("%10s|%10s\n", "1", color2str(power1_led_color));
  ICLI_PRINTF("%10s|%10s\n", "2", color2str(power2_led_color));
}


/* Get the current led color indicating PoE status */
void poe_icli_get_status_led(i32 session_id)
{
  ICLI_PRINTF("%10s|%10s\n", "PoEStatus", "Led");
  ICLI_PRINTF("%10s|%10s\n", "1", color2str(status_led_color));
}


// Debug function for during PoE frimware upgrade for MSCC chips.
mesa_rc poe_icli_firmware_upgrade(u32 session_id, mesa_port_no_t iport, const char *url, BOOL has_built_in, BOOL has_brick)
{
  ICLI_PRINTF("PoE firmware upgrade starting -- Do not power off\n");
  int tftp_err;
  mesa_rc rc = poe_do_firmware_upgrade(url, tftp_err, has_built_in, has_brick);
  if (rc != VTSS_RC_OK) {
    if (!has_built_in && rc != MESA_RC_ERR_POE_FIRMWARE_IS_UP_TO_DATE) {
      ICLI_PRINTF("%% Load error: %s\n", vtss_tftp_err2str(tftp_err));
    }
  } else {
    ICLI_PRINTF("PoE firmware upgrade done -- OK to power off\n");
  }
  return rc;
}


/***************************************************************************/
/* ICFG (Show running)                                                     */
/***************************************************************************/
#ifdef VTSS_SW_OPTION_ICFG

// Function called by ICFG.
static mesa_rc poe_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result)
{
  mesa_port_no_t   iport;
  poe_conf_t       poe_conf;

  if (!poe_is_any_chip_found()) {
    T_DG(VTSS_TRACE_GRP_ICLI, "%s", "PoE NOT supported");
    return VTSS_RC_OK;
  }

  // In case that the PoE chipset is not detected yet, we simply allow to setup PoE. This is needed because startup config is applied before the
  // PoE chipset is detected.
  if (!is_poe_ready()) {
    return TRUE;
  }

  switch (req->cmd_mode) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG:
    poe_config_get(&poe_conf);
    if (poe_conf.firmware[0] != 0) {
      VTSS_RC(vtss_icfg_printf(result, "poe firmware %s\n", poe_conf.firmware));
    }
    //
    // Power supply
    //

    T_D("poe_conf.power_supply_max_power_w=%d , default=%d  vtss_appl_poe_psu_user_configurable_get()=%d , req->all_defaults = %d", poe_conf.power_supply_max_power_w, vtss_appl_poe_supply_default_get(), vtss_appl_poe_psu_user_configurable_get(), req->all_defaults);

    if (vtss_appl_poe_psu_user_configurable_get() &&
        (req->all_defaults ||
         poe_conf.power_supply_max_power_w != vtss_appl_poe_supply_default_get())) {
      // No internal power supply, available power in external supply must be given
      VTSS_RC(vtss_icfg_printf(result, "poe power-supply-limit %d\n", poe_conf.power_supply_max_power_w));
    }

    //
    // Capacitor detect
    //
    if (vtss_appl_poe_pd_legacy_mode_configurable_get()) {
      if (req->all_defaults ||
          poe_conf.cap_detect != POE_CAP_DETECT_DISABLED) {
        if (poe_conf.cap_detect == POE_CAP_DETECT_DISABLED) {
          VTSS_RC(vtss_icfg_printf(result, "no poe capacitor-detect\n"));
        } else {
          VTSS_RC(vtss_icfg_printf(result, "poe capacitor-detect\n"));
        }
      }
    }


    //
    // Interruptible power
    //
    if (vtss_appl_poe_pd_interruptible_power_get()) {
      if (req->all_defaults || poe_conf.interruptible_power != POE_PD_INTERRUPTIBLE_POWER_DEFAULT) {
        if (poe_conf.interruptible_power) {
          // Normal command
          VTSS_RC(vtss_icfg_printf(result, "poe interruptible-power\n"));
        } else {
          // No command
          VTSS_RC(vtss_icfg_printf(result, "no poe interruptible-power\n"));
        }
      }
    }


    //
    // PD Auto class request
    //

    if (vtss_appl_poe_pd_auto_class_request_get()) {
      if (req->all_defaults || poe_conf.pd_auto_class_request != POE_PD_AUTO_CLASS_REQUEST_DEFAULT) {
        if (poe_conf.pd_auto_class_request) {
          // Normal command
          VTSS_RC(vtss_icfg_printf(result, "poe pd-auto-class-request\n"));
        } else {
          // No command
          VTSS_RC(vtss_icfg_printf(result, "no poe pd-auto-class-request\n"));
        }
      }
    }

    //
    // PD class mode
    //
    if (vtss_appl_poe_legacy_pd_class_mode_get()) {
      if (req->all_defaults || poe_conf.global_legacy_pd_class_mode != POE_LEGACY_PD_CLASS_MODE_DEFAULT) {

        // Normal command
        VTSS_RC(vtss_icfg_printf(result, "poe legacy-pd-class-mode %s\n",
                                 poe_conf.global_legacy_pd_class_mode == VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_STANDARD
                                 ? "standard" :
                                 poe_conf.global_legacy_pd_class_mode == VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_POH
                                 ? "poh" :
                                 poe_conf.global_legacy_pd_class_mode == VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_IGNORE_PD_CLASS
                                 ? "ignore-pd-class" :
                                 "Unknown PoE pd class mode"));
      }
    }


    break;
  //
  // Interface configurations
  //
  case ICLI_CMD_MODE_INTERFACE_PORT_LIST:

    iport = uport2iport(req->instance_id.port.begin_uport);

    if (msg_switch_configurable(req->instance_id.port.isid) && (req->instance_id.port.isid == VTSS_ISID_START)) {
      CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
      poe_mgmt_is_chip_found(&poe_chip_found[0]); // Get a list with which PoE chip there is associated to the port.
      if (poe_chip_found[iport] != MEBA_POE_NO_CHIPSET_FOUND) {

        // Get current configuration
        poe_config_get(&poe_conf);

        //
        // poe type
        //
        if (req->all_defaults ||
            poe_conf.bt_pse_port_type[iport] != POE_TYPE_DEFAULT) {

          // Normal command
          VTSS_RC(vtss_icfg_printf(result, " poe type %s\n",
                                   poe_conf.bt_pse_port_type[iport] == VTSS_APPL_POE_PSE_PORT_TYPE3_15W
                                   ? "type3-15w" :
                                   poe_conf.bt_pse_port_type[iport] == VTSS_APPL_POE_PSE_PORT_TYPE3_30W
                                   ? "type3-30w" :
                                   poe_conf.bt_pse_port_type[iport] == VTSS_APPL_POE_PSE_PORT_TYPE3_60W
                                   ? "type3-60w" :
                                   poe_conf.bt_pse_port_type[iport] == VTSS_APPL_POE_PSE_PORT_TYPE4_90W
                                   ? "type4-90w" :
                                   "Unknown PSE port type"));
        }


        //
        // poe mode
        //
        if (req->all_defaults ||
            poe_conf.poe_mode[iport] != POE_MODE_DEFAULT) {
          if (poe_conf.poe_mode[iport] == VTSS_APPL_POE_MODE_DISABLED) {
            // No command
            VTSS_RC(vtss_icfg_printf(result, " no poe mode\n"));
          } else {
            // Normal command
            VTSS_RC(vtss_icfg_printf(result, " poe mode %s\n",
                                     poe_conf.poe_mode[iport] == VTSS_APPL_POE_MODE_POE_PLUS ? "plus" :
                                     poe_conf.poe_mode[iport] == VTSS_APPL_POE_MODE_POE ? "standard" :
                                     "Unknown PoE mode"));
          }
        }


        //
        // power-management
        //
        if (req->all_defaults ||
            poe_conf.bt_port_pm_mode[iport] != VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DEFAULT) {

          // Normal command
          VTSS_RC(vtss_icfg_printf(result, " poe power-management %s\n",
                                   poe_conf.bt_port_pm_mode[iport] == VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC
                                   ? "dynamic" :
                                   poe_conf.bt_port_pm_mode[iport] == VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_STATIC
                                   ? "static" :
                                   poe_conf.bt_port_pm_mode[iport] == VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_HYBRID
                                   ? "hybrid" :
                                   "Unknown PoE power-management"));
        }


        //
        // priority
        //
        if (req->all_defaults ||
            poe_conf.priority[iport] != POE_PRIORITY_DEFAULT) {

          // Normal command
          VTSS_RC(vtss_icfg_printf(result, " poe priority %s\n",
                                   poe_conf.priority[iport] == VTSS_APPL_POE_PORT_POWER_PRIORITY_LOW
                                   ? "low" :
                                   poe_conf.priority[iport] == VTSS_APPL_POE_PORT_POWER_PRIORITY_HIGH
                                   ? "high" :
                                   poe_conf.priority[iport] == VTSS_APPL_POE_PORT_POWER_PRIORITY_CRITICAL
                                   ? "critical" :
                                   "Unknown PoE priority"));
        }


        //
        // max cable length
        //
        if (req->all_defaults ||
            poe_conf.cable_length[iport] != POE_CABLE_LENGTH_DEFAULT) {

          // Normal command
          VTSS_RC(vtss_icfg_printf(result, " poe max-cable-length %s\n",
                                   poe_conf.cable_length[iport] == VTSS_APPL_POE_PORT_CABLE_LENGTH_10
                                   ? "max10" :
                                   poe_conf.cable_length[iport] == VTSS_APPL_POE_PORT_CABLE_LENGTH_30
                                   ? "max30" :
                                   poe_conf.cable_length[iport] == VTSS_APPL_POE_PORT_CABLE_LENGTH_60
                                   ? "max60" :
                                   poe_conf.cable_length[iport] == VTSS_APPL_POE_PORT_CABLE_LENGTH_100
                                   ? "max100" :
                                   "Unknown PoE max cable length"));
        }

        //
        // Lldp
        //
        if (req->all_defaults ||
            poe_conf.lldp_disable[iport] != POE_LLDP_DISABLE_DEFAULT) {

          // Normal command
          VTSS_RC(vtss_icfg_printf(result, "%s poe lldp\n",
                                   poe_conf.lldp_disable[iport] == VTSS_APPL_POE_PORT_LLDP_DISABLED ? " no" : ""));
        }

      }
    }
    break;
  default:
    //Not needed for PoE
    break;
  }

  return VTSS_RC_OK;
}


/* ICFG Initialization function */
mesa_rc poe_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_POE_GLOBAL_CONF, "poe", poe_icfg_global_conf));
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_POE_PORT_CONF, "poe", poe_icfg_global_conf));
  return VTSS_RC_OK;
}

#endif // VTSS_SW_OPTION_ICFG
#endif //VTSS_SW_OPTION_POE
