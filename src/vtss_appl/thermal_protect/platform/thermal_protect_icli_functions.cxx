/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifdef VTSS_SW_OPTION_THERMAL_PROTECT

#include "icli_api.h"
#include "icli_porting_util.h"
#include "misc_api.h" /* For uport2iport() */
#include "thermal_protect.h"
#include "thermal_protect_api.h"

#include "msg_api.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif

#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/*  Code start :)                                                           */
/****************************************************************************/
// See thermal_protect_icli_functions.h
void thermal_protect_status(i32 session_id, BOOL has_interface, icli_stack_port_range_t *port_list)
{
  vtss_appl_thermal_protect_local_status_t   switch_status;
  mesa_rc rc;
  switch_iter_t sit;
  port_iter_t pit;

  // Loop through all switches in stack
  // Loop all the switches in question.
  if (icli_switch_iter_init(&sit) != VTSS_RC_OK) {
    return;
  }

  while (icli_switch_iter_getnext(&sit, port_list)) {
    if (!msg_switch_exists(sit.isid)) {
      continue;
    }

    // Print out of status data
    ICLI_PRINTF("Interface     Chip Temp.  Port Status\n");
    if ((rc = vtss_appl_thermal_protect_get_switch_status(&switch_status, sit.isid))) {
      ICLI_PRINTF("%s", error_txt(rc));
      continue;
    }

    // Loop through all ports
    if (icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
      while (icli_port_iter_getnext(&pit, port_list)) {
        char port[20];
        (void) icli_port_info_txt_short(sit.usid, pit.uport, port);
        ICLI_PRINTF("%-12s %3d C        %s\n", port, switch_status.port_temp[pit.iport],
                    thermal_protect_power_down2txt(switch_status.port_powered_down[pit.iport]));
      }
    }
  }
}

// See thermal_protect_icli_functions.h
void thermal_protect_grp(icli_stack_port_range_t *port_list, u8 grp, BOOL no)
{
  vtss_appl_thermal_protect_switch_conf_t     switch_conf;
  mesa_rc rc;
  switch_iter_t sit;
  port_iter_t pit;

  vtss_appl_thermal_protect_switch_conf_t switch_conf_default;
  vtss_appl_thermal_protect_switch_conf_default_get(&switch_conf_default); // Get default configuration

  // Loop through all switches in stack
  (void) icli_switch_iter_init(&sit);
  while (icli_switch_iter_getnext(&sit, port_list)) {

    // Get configuration for the current switch
    vtss_appl_thermal_protect_switch_conf_get(&switch_conf, sit.isid);

    // Loop through all ports
    (void)icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT);
    while (icli_port_iter_getnext(&pit, port_list)) { // !interface is used to get all ports in case no interface is specified.

      // Set to default if this is the no command
      if (no) {
        switch_conf.local_conf.port_grp[pit.iport] = switch_conf_default.local_conf.port_grp[pit.iport];
      } else {
        switch_conf.local_conf.port_grp[pit.iport] = grp;
      }
    }

    // Set the new configuration
    if ((rc = vtss_appl_thermal_protect_switch_conf_set(&switch_conf, sit.isid)) != VTSS_RC_OK) {
      T_E("%s \n", error_txt(rc));
    }
  }
}

// See thermal_protect_icli_functions.h
mesa_rc thermal_protect_temp(i32 session_id, icli_unsigned_range_t *grp_list, i16 new_temp, BOOL no)
{
  u8 element_index;
  u8 grp_index;
  switch_iter_t sit;
  vtss_appl_thermal_protect_switch_conf_t     switch_conf;
  vtss_appl_thermal_protect_switch_conf_t switch_conf_default;

  vtss_appl_thermal_protect_switch_conf_default_get(&switch_conf_default); // Get default configuration

  // grp_list can be NULL if the uses simply want to set all groups
  if (grp_list != NULL) {

    // Loop through all switches in stack
    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);
    while (switch_iter_getnext(&sit)) {

      // Get configuration for the current switch
      vtss_appl_thermal_protect_switch_conf_get(&switch_conf, sit.isid);

      // Loop through the elements in the list
      for (element_index = 0; element_index < grp_list->cnt; element_index++) {
        for (grp_index = grp_list->range[element_index].min; grp_index <= grp_list->range[element_index].max; grp_index++) {
          // Make sure we don't get out of bounds
          if (grp_index > VTSS_APPL_THERMAL_PROTECT_GROUP_MAX_VALUE) {
            ICLI_PRINTF("%% Ignoring invalid grprity:%d. Valid range is %u-%u.\n",
                        grp_index, VTSS_APPL_THERMAL_PROTECT_GROUP_MIN_VALUE, VTSS_APPL_THERMAL_PROTECT_GROUP_MAX_VALUE);
            continue;
          }

          // Set to default if this is the no command
          if (no) {
            switch_conf.glbl_conf.grp_temperatures[grp_index] = switch_conf_default.glbl_conf.grp_temperatures[grp_index];
          } else {
            T_IG(TRACE_GRP_CLI, "grp_index:%d, new_temp:%d", grp_index, new_temp);
            switch_conf.glbl_conf.grp_temperatures[grp_index] = new_temp;
          }

        }
      }

      // Set the new configuration
      ICLI_RC_CHECK_PRINT_RC (vtss_appl_thermal_protect_switch_conf_set(&switch_conf, sit.isid));
    }
  }
  return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_ICFG
/* ICFG callback functions */
static mesa_rc thermal_protect_conf(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result)
{
  u8 grp_index;
  vtss_appl_thermal_protect_switch_conf_t switch_conf;
  vtss_appl_thermal_protect_switch_conf_t switch_conf_default;
  mesa_port_no_t iport;
  vtss_icfg_conf_print_t conf_print;
  vtss_icfg_conf_print_init(&conf_print, req, result);

  vtss_appl_thermal_protect_switch_conf_default_get(&switch_conf_default); // Get default configuration

  T_N("req->cmd_mode:%d", req->cmd_mode);
  switch (req->cmd_mode) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG:
    // Get configuration for the current switch
    vtss_appl_thermal_protect_switch_conf_get(&switch_conf, msg_primary_switch_isid());

    // Group temperatures configuration
    for (grp_index = VTSS_APPL_THERMAL_PROTECT_GROUP_MIN_VALUE; grp_index <= VTSS_APPL_THERMAL_PROTECT_GROUP_MAX_VALUE; grp_index++) {
      conf_print.is_default = switch_conf.glbl_conf.grp_temperatures[grp_index] == switch_conf_default.glbl_conf.grp_temperatures[grp_index];
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "thermal-protect", "grp %d temperature %d", grp_index, switch_conf.glbl_conf.grp_temperatures[grp_index]));
    }
    break;

  case ICLI_CMD_MODE_INTERFACE_PORT_LIST: {
    vtss_isid_t isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);
    VTSS_ASSERT(iport != VTSS_PORT_NO_NONE);

    // Get configuration for the current switch
    vtss_appl_thermal_protect_switch_conf_get(&switch_conf, isid);

    conf_print.is_default = switch_conf.local_conf.port_grp[iport] == switch_conf_default.local_conf.port_grp[iport];
    conf_print.force_no_keyword     = TRUE;  // If all-defaults, use "no"-form
    conf_print.print_no_arguments  = FALSE; // But do not add %d argument.
    VTSS_RC(vtss_icfg_conf_print(&conf_print, "thermal-protect grp", "%d", switch_conf.local_conf.port_grp[iport]));
    // Back to defaults
    conf_print.force_no_keyword     = FALSE;
    conf_print.print_no_arguments  = TRUE;
    break;
  }
  default:
    break;
  }
  return VTSS_RC_OK;
}

#ifdef __cplusplus
}
#endif

/* ICFG Initialization function */
mesa_rc thermal_protect_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_THERMAL_PROTECT_GLOBAL_CONF, "thermal-protect", thermal_protect_conf));
  return vtss_icfg_query_register(VTSS_ICFG_THERMAL_PROTECT_PORT_CONF, "thermal-protect", thermal_protect_conf);
}
#endif // VTSS_SW_OPTION_ICFG
#endif // #ifdef VTSS_SW_OPTION_THERMAL_PROTECT

