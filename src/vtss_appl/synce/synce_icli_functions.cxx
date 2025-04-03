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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "icli_api.h"
#include "icli_porting_util.h"
#include "misc_api.h" // For e.g. vtss_uport_no_t
#include "msg_api.h"
#include "mgmt_api.h" // For mgmt_prio2txt
#include "synce_icli_functions.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif
// #include "vtss/appl/synce.h"
#include "synce.h" // For management functions
#include "synce_trace.h"// For Trace
#if defined(VTSS_SW_OPTION_PTP)
#include "vtss_ptp_api.h"
#endif
#include "pcb107_cpld.h"

/***************************************************************************/
/*  Internal types                                                         */
/****************************************************************************/
// Enum for selecting which synce configuration to perform.
typedef enum {
  ANEG,              // Configuration of aneg
  HOLDOFF,           // Configuration of holdoff
  SELECTION_MODE,    // Configuration of selection mode
  EEC_OPTION,        // Configuration of EEC OPTION
  WTR_CONF,          // Configuration of WTR
  PRIO,              // Configuration of pririty
  SSM_HOLDOVER,      // Configuration of ssm hodover
  SSM_FREERUN,       // Configuration of ssm freerun
  SSM_OVERWRITE,     // Configuration of ssm overwrite
  NOMINATE           // Configuration of nominate
} synce_conf_type_t;

// Used to passed the synce values to configure
typedef union {
  vtss_appl_synce_aneg_mode_t         aneg_mode;       // Preferred auto negotiation mode
  u8                                  holdoff;         // Holdoff value
  struct {
    u8                                clk_src;         // Source clock when mode is manual
    vtss_appl_synce_selection_mode_t  mode;            // selection mode
  } selection;
  uint                                wtr_time;        // WTR timer value in minutes
  vtss_appl_synce_quality_level_t     ssm_holdover;    // tx overwrite SSM used when clock controller is hold over
  vtss_appl_synce_quality_level_t     ssm_freerun;     // tx overwrite SSM used when clock controller is free run
  vtss_appl_synce_quality_level_t     ssm_overwrite;
  vtss_appl_synce_eec_option_t        eec_option;      // Synchronous Ethernet Equipment Clock option
  u8                                  prio;            // Priority
  u8                                  wtr;             // Wait-To-Restore time
  u8                                  nominated_src;   // port number of the norminated sources
} synce_conf_value_t;

/***************************************************************************/
/*  Internal functions                                                     */
/****************************************************************************/
// Function to configure selection
// IN : conf  - Which configuration parameter to configure
//      value - New configuration value
//      no    - TRUE if value shall be set to default value
// Return - error code
static mesa_rc synce_icli_selection_set(synce_conf_type_t conf, synce_conf_value_t value, BOOL no)
{
  vtss_appl_synce_clock_selection_mode_config_t  clock_selection_mode_config;
  vtss_appl_synce_clock_selection_mode_config_t  clock_selection_mode_config_default;

  // get current configuration
  (void) vtss_appl_synce_clock_selection_mode_config_get(&clock_selection_mode_config);

  // Get default
  synce_get_clock_selection_mode_config_default(&clock_selection_mode_config_default);

  switch (conf) {
  case SELECTION_MODE:
    if (no) {
      clock_selection_mode_config.selection_mode = clock_selection_mode_config_default.selection_mode;
    } else {
      clock_selection_mode_config.selection_mode = value.selection.mode;
    }

    // If the selection mode is manual then the clock must to be set as well
    if (clock_selection_mode_config.selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL) {
      if (no) {
        clock_selection_mode_config.source = clock_selection_mode_config_default.source;
      } else {
        clock_selection_mode_config.source = value.selection.clk_src;
      }
    }
    break;
  case WTR_CONF:
    if (no) {
      clock_selection_mode_config.wtr_time = clock_selection_mode_config_default.wtr_time;
    } else {
      clock_selection_mode_config.wtr_time = value.wtr_time;
    }
    break;
  case SSM_HOLDOVER:
    if (no) {
      clock_selection_mode_config.ssm_holdover = clock_selection_mode_config_default.ssm_holdover;
    } else {
      clock_selection_mode_config.ssm_holdover = value.ssm_holdover;
    }
    break;
  case SSM_FREERUN:
    if (no) {
      clock_selection_mode_config.ssm_freerun = clock_selection_mode_config_default.ssm_freerun;
    } else {
      clock_selection_mode_config.ssm_freerun = value.ssm_freerun;
    }
    break;
  case EEC_OPTION:
    if (no) {
      clock_selection_mode_config.eec_option = clock_selection_mode_config_default.eec_option;
    } else {
      clock_selection_mode_config.eec_option = value.eec_option;
    }
    break;
  default:
    T_E("Unknown configuration");
  }

  T_NG(TRACE_GRP_CLI, "clock_selection_mode_config.source:%d", clock_selection_mode_config.source);

  // Update new settings
  VTSS_RC(vtss_appl_synce_clock_selection_mode_config_set(&clock_selection_mode_config));
  return VTSS_RC_OK;
}

// Function to configure station clock
// IN : input_source - TRUE to configure input clock, FALSE to configure output clock
//      freq - New configuration frequency
// Return - error code
static mesa_rc synce_icli_station_clock_set(BOOL input_source, vtss_appl_synce_frequency_t clk_freq)
{
  if (input_source) {
    VTSS_RC(synce_mgmt_station_clock_in_set(clk_freq));
  } else {
    VTSS_RC(synce_mgmt_station_clock_out_set(clk_freq));
  }
  return VTSS_RC_OK;
}

// Function to configure nominate
// IN : clk_src_list - list of clock sources to configure
//      conf  - Which configuration parameter to configure
//      value - New configuration value
//      no    - TRUE if value shall be set to default value
// Return - error code
static mesa_rc synce_icli_nominate_set(icli_range_t *clk_src_list, synce_conf_type_t conf, synce_conf_value_t value, BOOL no)
{
  vtss_appl_synce_clock_source_nomination_config_t clock_source_nomination_config_default[SYNCE_NOMINATED_MAX];
  vtss_appl_synce_clock_source_nomination_config_t clock_source_nomination_config;
  u8 clk_src;
  u8 list_cnt;
  u8 cnt_index;

  // Get default source nomination config
  synce_mgmt_set_clock_source_nomination_config_to_default(clock_source_nomination_config_default);

  T_RG(TRACE_GRP_CLI, "no:%d", no);

  // Determine which soirce clock to configure
  list_cnt = 1; // Default the list to only have one source list                   ;

  if (clk_src_list != NULL) { // User has given a source clock list
    list_cnt = clk_src_list->u.sr.cnt;
  }

  // Run through all the lists
  for (cnt_index = 0; cnt_index < list_cnt; cnt_index++) {
    u8 src_clk_min = 1;                       // Default to start from 1
    u8 src_clk_max = synce_my_nominated_max;  // Default to end at the last clock source

    if (clk_src_list != NULL) { // User has specified a source clock list
      src_clk_min = clk_src_list->u.sr.range[cnt_index].min;
      src_clk_max = clk_src_list->u.sr.range[cnt_index].max;
    }

    T_IG(TRACE_GRP_CLI, "src_clk_min:%d, src_clk_max:%d, list_cnt:%d, clk_src_list is NULL:%d",
         src_clk_min, src_clk_max, list_cnt, clk_src_list == NULL);

    // Clock src from a user point of view is starting from 1, while indexes starts from 0, so subtract 1 in the loop
    for (clk_src = synce_uclk2iclk(src_clk_min); clk_src <= synce_uclk2iclk(src_clk_max); clk_src++) {
      // get current configuration of clock source
      (void) vtss_appl_synce_clock_source_nomination_config_get((clk_src + 1), &clock_source_nomination_config);

      // Update the parameter in question
      switch (conf) {
      case ANEG:
        if (no) {
          clock_source_nomination_config.aneg_mode = clock_source_nomination_config_default[clk_src].aneg_mode;
        } else {
          clock_source_nomination_config.aneg_mode = value.aneg_mode;
        }
        break;
      case NOMINATE:
        if (no) {
          clock_source_nomination_config.nominated = clock_source_nomination_config_default[clk_src].nominated;
          clock_source_nomination_config.network_port = clock_source_nomination_config_default[clk_src].network_port;
          clock_source_nomination_config.clk_in_port = clock_source_nomination_config_default[clk_src].clk_in_port;
        } else {
          clock_source_nomination_config.nominated = TRUE;
          if (value.nominated_src < SYNCE_STATION_CLOCK_PORT) {
            vtss_ifindex_t ifindex;
            (void) vtss_ifindex_from_port(0, value.nominated_src, &ifindex);
            clock_source_nomination_config.network_port = ifindex;
            clock_source_nomination_config.clk_in_port = 0;
          } else {
            clock_source_nomination_config.network_port = VTSS_IFINDEX_NONE;  // data_port == 0 indicates that the station clock or PTP 8265.1 is used.
            if (value.nominated_src == SYNCE_STATION_CLOCK_PORT) {
              clock_source_nomination_config.clk_in_port = 0;
            } else {
              clock_source_nomination_config.clk_in_port = value.nominated_src - SYNCE_STATION_CLOCK_PORT - 1 + 128;
            }
          }
        }
        break;
      case HOLDOFF:
        if (no) {
          clock_source_nomination_config.holdoff_time = clock_source_nomination_config_default[clk_src].holdoff_time;
        } else {
          clock_source_nomination_config.holdoff_time = value.holdoff;
        }
        break;
      case SSM_OVERWRITE:
        if (no) {
          clock_source_nomination_config.ssm_overwrite = clock_source_nomination_config_default[clk_src].ssm_overwrite;
        } else {
          clock_source_nomination_config.ssm_overwrite = value.ssm_overwrite;
        }
        break;
      case PRIO:
        if (no) {
          clock_source_nomination_config.priority = clock_source_nomination_config_default[clk_src].priority;
        } else {
          clock_source_nomination_config.priority = value.prio;
        }
        break;

      default:
        T_E("Unknown configuration");
      }

      T_DG(TRACE_GRP_CLI, "clock_source_nomination_config.aneg_mode:%d, clk_src:%d, port (vtss_ifindex_t):%d, holdoff_time:%d, clock_source_nomination_config.ssm_overwrite[clk_src]:%d, nominate:%d",
           clock_source_nomination_config.aneg_mode,
           clk_src,
           VTSS_IFINDEX_PRINTF_ARG(clock_source_nomination_config.network_port),
           clock_source_nomination_config.holdoff_time,
           clock_source_nomination_config.ssm_overwrite,
           clock_source_nomination_config.nominated);

      // Update new settings
      VTSS_RC(vtss_appl_synce_clock_source_nomination_config_set((clk_src + 1), &clock_source_nomination_config));
    }
  }
  return VTSS_RC_OK;
}

// Function to configure SSM enable/disable
// IN : plist - list ports to configure
//      no    - TRUE if value shall be set to default value
// Return - error code
static mesa_rc sync_ssm_enable( icli_stack_port_range_t *plist, BOOL no)
{
  switch_iter_t   sit;
  vtss_ifindex_t ifindex;
  vtss_appl_synce_port_config_t config;
  // Loop through all switches in stack
  VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
  while (icli_switch_iter_getnext(&sit, plist)) {

    // Loop through all ports
    port_iter_t pit;
    VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
    while (icli_port_iter_getnext(&pit, plist)) {
      VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));
      config.ssm_enabled = !no;
      VTSS_RC(vtss_appl_synce_port_config_set(ifindex, &config));
    }
  }
  return VTSS_RC_OK;
}

// Function to show SyncE state
// IN : session_id - For icli print
// Return - error code
static mesa_rc synce_icli_show_state(i32 session_id)
{
  uint                              i;
  vtss_appl_synce_clock_selection_mode_status_t clock_selection_mode_status;
  vtss_appl_synce_port_config_t     port_config;
  vtss_appl_synce_port_status_t     port_status;
  port_iter_t                       pit;

  // Get current states
  VTSS_RC(vtss_appl_synce_clock_selection_mode_status_get(&clock_selection_mode_status));
  ICLI_PRINTF("\nSelector State is: ");
  switch (clock_selection_mode_status.selector_state) {
  case VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED:
    if (clock_selection_mode_status.clock_input != fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT)) {
      ICLI_PRINTF("Locked to %d\n", clock_selection_mode_status.clock_input + 1);
      break;
    } else {
      ICLI_PRINTF("Locked to None\n");
      break;
    }
  case VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER:
    ICLI_PRINTF("Holdover\n");
    break;
  case VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN:
    ICLI_PRINTF("Free Run\n");
    break;
  case VTSS_APPL_SYNCE_SELECTOR_STATE_PTP:
#if defined(VTSS_SW_OPTION_PTP)
    ICLI_PRINTF("Locked to PTP instance %d\n", synce_mgmt_get_best_master());
#else
    ICLI_PRINTF("Locked to PTP (but PTP is not supported in this build).");
#endif
    break;
  case VTSS_APPL_SYNCE_SELECTOR_STATE_REF_FAILED:
    ICLI_PRINTF("Ref. Fail\n");
    break;
  case VTSS_APPL_SYNCE_SELECTOR_STATE_ACQUIRING:
    ICLI_PRINTF("Acquire Lock\n");
    break;
  }

  // Report the error status for each of the sources
  {
    vtss_appl_synce_clock_source_nomination_status_t clock_source_nomination_status[synce_my_nominated_max];

    ICLI_PRINTF("\nAlarm State is:\n");
    ICLI_PRINTF("Clk:");
    for (i = 0; i < synce_my_nominated_max; ++i) {
      ICLI_PRINTF("       %2d", i + 1);
    }
    ICLI_PRINTF("\n");
    ICLI_PRINTF("LOCS:");
    for (i = 0; i < synce_my_nominated_max; ++i) {
      vtss_appl_synce_clock_source_nomination_status_get((i + 1), &clock_source_nomination_status[i]);
      if (clock_source_nomination_status[i].locs) {
        ICLI_PRINTF("    TRUE ");
      } else {
        ICLI_PRINTF("    FALSE");
      }
    }
    ICLI_PRINTF("\nSSM: ");
    for (i = 0; i < synce_my_nominated_max; ++i) {
      if (clock_source_nomination_status[i].ssm) {
        ICLI_PRINTF("    TRUE ");
      } else {
        ICLI_PRINTF("    FALSE");
      }
    }
    ICLI_PRINTF("\nWTR: ");
    for (i = 0; i < synce_my_nominated_max; ++i) {
      if (clock_source_nomination_status[i].wtr) {
        ICLI_PRINTF("    TRUE ");
      } else {
        ICLI_PRINTF("    FALSE");
      }
    }
  }

  ICLI_PRINTF("\n");
  if (clock_selection_mode_status.lol == VTSS_APPL_SYNCE_LOL_ALARM_STATE_TRUE) {
    ICLI_PRINTF("\nLOL:     TRUE");
  } else if (clock_selection_mode_status.lol == VTSS_APPL_SYNCE_LOL_ALARM_STATE_FALSE) {
    ICLI_PRINTF("\nLOL:     FALSE");
  } else {
    ICLI_PRINTF("\nLOL:     N/A");
  }

  if (clock_selection_mode_status.dhold) {
    ICLI_PRINTF("\nDHOLD:   TRUE \n");
  } else {
    ICLI_PRINTF("\nDHOLD:   FALSE\n");
  }

  ICLI_PRINTF("\nSSM State is:\n");

  ICLI_PRINTF("%-23s %12s %12s %-8s\n", "Interface", "Tx SSM", "Rx SSM", "Mode");

  port_iter_init_local(&pit);
  vtss_ifindex_t ifindex;
  switch_iter_t sit;
  VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
  while (icli_switch_iter_getnext(&sit, NULL)) {
    // Loop through all ports
    VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
    while (icli_port_iter_getnext(&pit, NULL)) {
      (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
      if (vtss_appl_synce_port_config_get(ifindex, &port_config) == VTSS_RC_OK) {
        if (port_config.ssm_enabled) {
          char buf[150];

          VTSS_RC(vtss_appl_synce_port_status_get(ifindex, &port_status));
          ICLI_PRINTF("%-23s %12s %12s %-8s\n",
                      icli_port_info_txt(sit.usid, pit.uport, buf), ssm_string(port_status.ssm_tx), ssm_string(port_status.ssm_rx), (!port_status.master) ? "Slave" : "Master");
        }
      }
    }
  }
  ICLI_PRINTF("\n");

  return VTSS_RC_OK;
}

// Function to show SyncE clock selection config
// IN : session_id - For icli print
// Return - error code
mesa_rc synce_icli_show_clock_selection_config(i32 session_id)
{
  vtss_appl_synce_clock_selection_mode_config_t clock_selection_mode_config;
  char tmp_buf[9];

  if (vtss_appl_synce_clock_selection_mode_config_get(&clock_selection_mode_config) == VTSS_RC_OK) {
    sprintf(tmp_buf, "%8hd", (short) clock_selection_mode_config.wtr_time);

    ICLI_PRINTF("Selection Mode    Source  Wtr Time  SSM Holdover  SSM Freerun  EEC Option\n");
    ICLI_PRINTF("----------------  ------  --------  ------------  -----------  ----------\n");
    ICLI_PRINTF("%-16s  %6d  %8s  %-12s  %-11s  %-10s\n",
                vtss_appl_synce_selection_mode_txt[clock_selection_mode_config.selection_mode].valueName,
                clock_selection_mode_config.source,
                clock_selection_mode_config.wtr_time == 0 ? "Disabled" : tmp_buf,
                vtss_appl_synce_quality_level_txt[clock_selection_mode_config.ssm_holdover].valueName,
                vtss_appl_synce_quality_level_txt[clock_selection_mode_config.ssm_freerun].valueName,
                vtss_appl_synce_eec_option_txt[clock_selection_mode_config.eec_option].valueName);
    return VTSS_RC_OK;
  } else {
    return VTSS_RC_ERROR;
  }
}

// Function to show SyncE station clock config
// IN : session_id - For icli print
// Return - error code
mesa_rc synce_icli_show_station_clock_config(i32 session_id)
{
  vtss_appl_synce_station_clock_config_t station_clock_config;

  if (vtss_appl_synce_station_clock_config_get(&station_clock_config) == VTSS_RC_OK) {
    ICLI_PRINTF("Clk input freq.  Clk output freq.\n");
    ICLI_PRINTF("---------------  ----------------\n");
    ICLI_PRINTF("%-15s  %-16s\n",
                vtss_appl_synce_frequency_txt[station_clock_config.station_clk_in].valueName,
                vtss_appl_synce_frequency_txt[station_clock_config.station_clk_out].valueName);
    return VTSS_RC_OK;
  } else {
    return VTSS_RC_ERROR;
  }
}

// Function to show SyncE source nomination config
// IN : session_id - For icli print
// Return - error code
mesa_rc synce_icli_show_source_nomination_config(i32 session_id)
{
  ICLI_PRINTF("Source  Nominated  Port   Priority  SSM Overwrite  Holdoff   ANEG mode     \n");
  ICLI_PRINTF("------  ---------  -----  --------  -------------  --------  --------------\n");
  for (uint n = 1; n <= SYNCE_NOMINATED_MAX; n++) {
    vtss_appl_synce_clock_source_nomination_config_t source_nomination_config;
    if (vtss_appl_synce_clock_source_nomination_config_get(n, &source_nomination_config) == VTSS_RC_OK) {
      char tmp_buf[9];
      char tmp_buf2[9];
      if (source_nomination_config.network_port != 0) {
        u32 port;
        (void) synce_network_port_clk_in_port_combo_to_port(source_nomination_config.network_port, 0, &port);
        sprintf(tmp_buf2, "%5d", port + 1);
      } else {
        if (source_nomination_config.clk_in_port == 0) {
          sprintf(tmp_buf2, "%s", "Clkin");
        } else {
          sprintf(tmp_buf2, "PTP-%d", source_nomination_config.clk_in_port - 128);
        }
      }
      sprintf(tmp_buf, "%6hdms", source_nomination_config.holdoff_time * 100);
      ICLI_PRINTF("%6d  %-9s  %5s  %8d  %-13s  %8s  %-14s\n",
                  n,
                  source_nomination_config.nominated == 1 ? "True" : "False",
                  tmp_buf2,
                  source_nomination_config.priority,
                  vtss_appl_synce_quality_level_txt[source_nomination_config.ssm_overwrite].valueName,
                  source_nomination_config.holdoff_time == 0 ? "Disabled" : tmp_buf,
                  vtss_appl_synce_aneg_mode_txt[source_nomination_config.aneg_mode].valueName);
    }
  }
  return VTSS_RC_OK;
}

// Function to show SyncE clock synchronization config
// IN : session_id - For icli print
// Return - error code
mesa_rc synce_icli_show_clock_synchronization_config(i32 session_id)
{
  vtss_appl_synce_port_config_t port_config;
  port_iter_t pit;
  vtss_ifindex_t ifindex;

  ICLI_PRINTF("Port  ssm enabled\n");
  ICLI_PRINTF("----  -----------\n");
  port_iter_init_local(&pit);
  while (port_iter_getnext(&pit)) {
    (void) vtss_ifindex_from_port(0, pit.iport, &ifindex);
    if (vtss_appl_synce_port_config_get(ifindex, &port_config) == VTSS_RC_OK) {
      ICLI_PRINTF("%4d  %-11s\n",
                  iport2uport(pit.iport),
                  port_config.ssm_enabled == 0 ? "False" : "True");
    }
  }
  return VTSS_RC_OK;
}

// Function for at runtime figure out which input/out frequencies is supported
// IN : ask - Asking
//      supported - Set to TRUE is supported (present gets set)
// OUT  runtime - Pointer to where to put the "answer"
// Return - TRUE if answer found
static BOOL synce_icli_runtime_synce_clk_chk(icli_runtime_ask_t ask,
                                             icli_runtime_t     *runtime,
                                             BOOL supported)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    runtime->present = supported;
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

// Function for checking if any input frequency is supported
// Return - TRUE if at least one input frequency is supported, else FALSE
static BOOL synce_icli_any_input_freq_supported(void)
{
  return clock_in_range_check(VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ) |
         clock_in_range_check(VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ) |
         clock_in_range_check(VTSS_APPL_SYNCE_STATION_CLK_10_MHZ);
}

// Function for checking if any output frequency is supported
// Return - TRUE if at least one output frequency is supported, else FALSE
static BOOL synce_icli_any_output_freq_supported(void)
{
  return clock_out_range_check(VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ) |
         clock_out_range_check(VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ) |
         clock_out_range_check(VTSS_APPL_SYNCE_STATION_CLK_10_MHZ);
}

/***************************************************************************/
/*  Functions called by iCLI                                                */
/****************************************************************************/
//  see synce_icli_functions.h

BOOL synce_icli_runtime_synce_eec1_options(u32                session_id,
                                           icli_runtime_ask_t ask,
                                           icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    vtss_appl_synce_clock_selection_mode_config_t clock_selection_mode_config;

    if (vtss_appl_synce_clock_selection_mode_config_get(&clock_selection_mode_config) == VTSS_RC_OK) {
      runtime->present = (clock_selection_mode_config.eec_option == VTSS_APPL_SYNCE_EEC_OPTION_1);
    } else {
      runtime->present = TRUE;
    }
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

BOOL synce_icli_runtime_synce_eec2_options(u32                session_id,
                                           icli_runtime_ask_t ask,
                                           icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    vtss_appl_synce_clock_selection_mode_config_t clock_selection_mode_config;

    if (vtss_appl_synce_clock_selection_mode_config_get(&clock_selection_mode_config) == VTSS_RC_OK) {
      runtime->present = (clock_selection_mode_config.eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2);
    } else {
      runtime->present = TRUE;
    }
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

BOOL synce_icli_runtime_synce_sources(u32                session_id,
                                      icli_runtime_ask_t ask,
                                      icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    runtime->present = TRUE;
    return TRUE;
  case ICLI_ASK_BYWORD:
    icli_sprintf(runtime->byword, "<clk-source : %u-%u>", 1, synce_my_nominated_max);
    return TRUE;
  case ICLI_ASK_RANGE:
    runtime->range.type = ICLI_RANGE_TYPE_UNSIGNED;
    runtime->range.u.sr.cnt = 1;
    runtime->range.u.sr.range[0].min = 1;
    runtime->range.u.sr.range[0].max = synce_my_nominated_max;
    T_RG(TRACE_GRP_CLI, "synce_my_nominated_max:%d", synce_my_nominated_max);
    return TRUE;
  case ICLI_ASK_HELP:
    icli_sprintf(runtime->help, "Clock source number");
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

//  see synce_icli_functions.h
BOOL synce_icli_runtime_synce_priorities(u32                session_id,
                                         icli_runtime_ask_t ask,
                                         icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
    runtime->present = TRUE;
    return TRUE;
  case ICLI_ASK_BYWORD:
    icli_sprintf(runtime->byword, "<priority : %u-%u>", 0, synce_my_priority_max - 1);
    return TRUE;
  case ICLI_ASK_RANGE:
    runtime->range.type = ICLI_RANGE_TYPE_UNSIGNED;
    runtime->range.u.sr.cnt = 1;
    runtime->range.u.sr.range[0].min = 0;
    runtime->range.u.sr.range[0].max = synce_my_priority_max - 1;
    T_RG(TRACE_GRP_CLI, "synce_my_priority_max:%d", synce_my_priority_max - 1);
    return TRUE;
  case ICLI_ASK_HELP:
    icli_sprintf(runtime->help, "Clock source priority");
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

//  see synce_icli_functions.h
BOOL synce_icli_runtime_input_10MHz(u32                session_id,
                                    icli_runtime_ask_t ask,
                                    icli_runtime_t     *runtime)
{
  return synce_icli_runtime_synce_clk_chk(ask, runtime, clock_in_range_check(VTSS_APPL_SYNCE_STATION_CLK_10_MHZ));
}

//  see synce_icli_functions.h
BOOL synce_icli_runtime_input_2048khz(u32               session_id,
                                      icli_runtime_ask_t ask,
                                      icli_runtime_t    *runtime)
{
  return synce_icli_runtime_synce_clk_chk(ask, runtime, clock_in_range_check(VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ));
}

//  see synce_icli_functions.h
BOOL synce_icli_runtime_input_1544khz(u32                session_id,
                                      icli_runtime_ask_t ask,
                                      icli_runtime_t     *runtime)
{
  return synce_icli_runtime_synce_clk_chk(ask, runtime, clock_in_range_check(VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ));
}

//  see synce_icli_functions.h
BOOL synce_icli_runtime_any_input_freq(u32                session_id,
                                       icli_runtime_ask_t ask,
                                       icli_runtime_t     *runtime)
{
  return synce_icli_runtime_synce_clk_chk(ask, runtime, synce_icli_any_input_freq_supported());
}


//  see synce_icli_functions.h
BOOL synce_icli_runtime_output_10MHz(u32                session_id,
                                     icli_runtime_ask_t ask,
                                     icli_runtime_t     *runtime)
{
  return synce_icli_runtime_synce_clk_chk(ask, runtime, clock_out_range_check(VTSS_APPL_SYNCE_STATION_CLK_10_MHZ));
}

//  see synce_icli_functions.h
BOOL synce_icli_runtime_output_2048khz(u32               session_id,
                                       icli_runtime_ask_t ask,
                                       icli_runtime_t    *runtime)
{
  return synce_icli_runtime_synce_clk_chk(ask, runtime, clock_out_range_check(VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ));
}

//  see synce_icli_functions.h
BOOL synce_icli_runtime_output_1544khz(u32                session_id,
                                       icli_runtime_ask_t ask,
                                       icli_runtime_t     *runtime)
{
  return synce_icli_runtime_synce_clk_chk(ask, runtime, clock_out_range_check(VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ));
}

//  see synce_icli_functions.h
BOOL synce_icli_runtime_any_output_freq(u32                session_id,
                                        icli_runtime_ask_t ask,
                                        icli_runtime_t     *runtime)
{
  return synce_icli_runtime_synce_clk_chk(ask, runtime, synce_icli_any_output_freq_supported());
}

//  see synce_icli_functions.h
mesa_rc synce_icli_station_clk(i32 session_id, BOOL input_source, BOOL has_1544, BOOL has_2048, BOOL has_10M, BOOL no)
{
  vtss_appl_synce_frequency_t clk_freq;

  if (has_1544) {
    clk_freq = VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ;
  } else  if (has_2048) {
    clk_freq = VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ;
  } else if (has_10M) {
    clk_freq = VTSS_APPL_SYNCE_STATION_CLK_10_MHZ;
  } else {
    clk_freq = VTSS_APPL_SYNCE_STATION_CLK_DIS;
  }

  ICLI_RC_CHECK_PRINT_RC(synce_icli_station_clock_set(input_source, clk_freq));
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_aneg(i32 session_id, icli_range_t *clk_list, BOOL has_master, BOOL has_slave, BOOL has_forced, BOOL no)
{
  synce_conf_type_t conf = ANEG;
  synce_conf_value_t value;

  if (has_master) {
    value.aneg_mode = VTSS_APPL_SYNCE_ANEG_PREFERED_MASTER;
  } else if (has_slave) {
    value.aneg_mode = VTSS_APPL_SYNCE_ANEG_PREFERED_SLAVE;
  } else if (has_forced) {
    value.aneg_mode = VTSS_APPL_SYNCE_ANEG_FORCED_SLAVE;
  } else if (no) {
    value.aneg_mode = VTSS_APPL_SYNCE_ANEG_FORCED_SLAVE; // Dummy assign, not real used.
  } else {
    T_E("At least one parameter must be selected");
    return VTSS_RC_ERROR;
  }
  T_IG(TRACE_GRP_CLI, "ang:%d, has_forced:%d, has_master:%d, has_slave:%d, no:%d", value.aneg_mode, has_forced, has_master, has_slave, no);
  ICLI_RC_CHECK_PRINT_RC(synce_icli_nominate_set(clk_list, conf, value, no));
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_hold_time(i32 session_id, icli_range_t *clk_list, u8 v_3_to_18, BOOL no)
{
  synce_conf_type_t conf = HOLDOFF;
  synce_conf_value_t value;
  value.holdoff = v_3_to_18;
  ICLI_RC_CHECK_PRINT_RC(synce_icli_nominate_set(clk_list, conf, value, no));
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_show(i32 session_id)
{
  ICLI_RC_CHECK_PRINT_RC(synce_icli_show_state(session_id));
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_show_ptp_ports(i32 session_id)
{
#if defined(VTSS_SW_OPTION_PTP)
  ICLI_PRINTF("Instance  SSM_RX   PTSF\n");
  ICLI_PRINTF("--------  -------  ------------\n");

  for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
    synce_ptp_port_state_t ptp_port_state;

    (void) vtss_synce_ptp_port_state_get(i, &ptp_port_state);

    const char *ssm_rx;
    switch (ptp_port_state.ssm_rx) {
    case SSM_QL_PRS_INV:
      ssm_rx = "QL_PRS";
      break;
    case SSM_QL_PRC:
      ssm_rx = "QL_PRC";
      break;
    case SSM_QL_SSUA_TNC:
      ssm_rx = "QL_SSUA";
      break;
    case SSM_QL_SSUB:
      ssm_rx = "QL_SSUB";
      break;
    case SSM_QL_EEC2:
      ssm_rx = "QL_EEC2";
      break;
    case SSM_QL_EEC1:
      ssm_rx = "QL_EEC1";
      break;
    case SSM_QL_PROV:
      ssm_rx = "QL_PROV";
      break;
    case SSM_QL_DNU_DUS:
      ssm_rx = "QL_DNU";
      break;
    case SSM_QL_FAIL:
      ssm_rx = "QL_FAIL";
      break;
    default:
      ssm_rx = "unknown";
      break;
    }

    const char *ptsf;
    switch (ptp_port_state.ptsf) {
    case SYNCE_PTSF_NONE:
      ptsf = "none";
      break;
    case SYNCE_PTSF_UNUSABLE:
      ptsf = "unusable";
      break;
    case SYNCE_PTSF_LOSS_OF_SYNC:
      ptsf = "lossSync";
      break;
    case SYNCE_PTSF_LOSS_OF_ANNOUNCE:
      ptsf = "lossAnnounce";
      break;
    default:
      ptsf = "unknown";
      break;
    }
    ICLI_PRINTF("%-8d  %-7s  %-12s\n", i, ssm_rx, ptsf);
  }
#else
  ICLI_PRINTF("No PTP support in this build.\n");
#endif
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_show_port_config(i32 session_id)
{
  port_iter_t pit;
  vtss_ifindex_t ifindex;
  switch_iter_t sit;
  vtss_appl_synce_port_config_t port_config;
  vtss_appl_synce_port_status_t port_status;

  ICLI_PRINTF("Interface               SSM Enabled\n");
  ICLI_PRINTF("----------------------  -----------\n");

  port_iter_init_local(&pit);
  VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
  while (icli_switch_iter_getnext(&sit, NULL)) {
    // Loop through all ports
    VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
    while (icli_port_iter_getnext(&pit, NULL)) {
      (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
      if (vtss_appl_synce_port_config_get(ifindex, &port_config) == VTSS_RC_OK) {
        char buf[150];

        VTSS_RC(vtss_appl_synce_port_status_get(ifindex, &port_status));
        ICLI_PRINTF("%-23s %s\n",
                    icli_port_info_txt(sit.usid, pit.uport, buf), port_config.ssm_enabled ? "True" : "False");
      }
    }
  }
  ICLI_PRINTF("\n");

  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_show_port_status(i32 session_id)
{
  port_iter_t pit;
  vtss_ifindex_t ifindex;
  switch_iter_t sit;
  vtss_appl_synce_port_config_t port_config;
  vtss_appl_synce_port_status_t port_status;

  ICLI_PRINTF("Interface               SSM_TX   SSM_RX   Mode\n");
  ICLI_PRINTF("----------------------  -------  -------  ------\n");

  port_iter_init_local(&pit);
  VTSS_RC(switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG));
  while (icli_switch_iter_getnext(&sit, NULL)) {
    // Loop through all ports
    VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
    while (icli_port_iter_getnext(&pit, NULL)) {
      (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
      if (vtss_appl_synce_port_config_get(ifindex, &port_config) == VTSS_RC_OK) {
        char buf[150];

        VTSS_RC(vtss_appl_synce_port_status_get(ifindex, &port_status));
        ICLI_PRINTF("%-23s %-8s %-8s %s\n",
                    icli_port_info_txt(sit.usid, pit.uport, buf), ssm_string(port_status.ssm_tx), ssm_string(port_status.ssm_rx), (!port_status.master) ? "Slave" : "Master");
      }
    }
  }
  ICLI_PRINTF("\n");

  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_clear(i32 session_id, icli_range_t *clk_src_list)
{
  u8 clk_src;
  u8 list_cnt;
  u8 cnt_index;

  list_cnt = 1;                    ;

  if (clk_src_list != NULL) {
    list_cnt = clk_src_list->u.sr.cnt;
  }

  for (cnt_index = 0; cnt_index < list_cnt; cnt_index++) {
    u8 src_clk_min = 1;
    u8 src_clk_max = synce_my_nominated_max;
    if (clk_src_list != NULL) {
      src_clk_min = clk_src_list->u.sr.range[cnt_index].min;
      src_clk_max = clk_src_list->u.sr.range[cnt_index].max;
    }

    for (clk_src = src_clk_min; clk_src <= src_clk_max; clk_src++) {
      T_DG(TRACE_GRP_CLI, "clk_src:%d, src_clk_min:%d, src_clk_max:%d, list_cnt:%d", clk_src, src_clk_min, src_clk_max, list_cnt);
      ICLI_RC_CHECK_PRINT_RC(synce_mgmt_wtr_clear_set(synce_uclk2iclk(clk_src)));
    }
  }
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_nominate(i32 session_id, icli_range_t *clk_list, BOOL clk_in, BOOL has_ptp, int ptp_inst, icli_switch_port_range_t *port, BOOL no)
{
  synce_conf_type_t conf = NOMINATE;
  synce_conf_value_t value;

  if (clk_in) {
    value.nominated_src = SYNCE_STATION_CLOCK_PORT;
  } else if (has_ptp) {
    value.nominated_src = SYNCE_STATION_CLOCK_PORT + 1 + ptp_inst;
  } else if (port != NULL) { // port is not set for the no command
    value.nominated_src = port->begin_iport;
  } else {
    value.nominated_src = 1;
  }
  T_IG(TRACE_GRP_CLI, "clk_in %d, has_ptp %d, src_clk_port:%d", clk_in, has_ptp, value.nominated_src);

  ICLI_RC_CHECK_PRINT_RC(synce_icli_nominate_set(clk_list, conf, value, no));
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_selector(i32 session_id, u8 clk_src, BOOL has_manual, BOOL has_selected, BOOL has_nonrevertive, BOOL has_revertive, BOOL has_holdover, BOOL has_freerun, BOOL no)
{

  synce_conf_type_t conf = SELECTION_MODE;
  synce_conf_value_t value;

  if (has_manual) {
    value.selection.mode = VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL;
  } else if (has_selected) {
    value.selection.mode = VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED;
  } else if (has_nonrevertive) {
    value.selection.mode = VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE;
  } else if (has_revertive) {
    value.selection.mode = VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE;
  } else if (has_holdover) {
    value.selection.mode = VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER;
  } else if (has_freerun) {
    value.selection.mode = VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN;
  } else {
    value.selection.mode = VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE; /* default selection mode */
  }

  value.selection.clk_src = clk_src;
  ICLI_RC_CHECK_PRINT_RC(synce_icli_selection_set(conf, value, no));
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_prio(i32 session_id, icli_range_t *clk_list, u8 prio, BOOL no)
{
  synce_conf_type_t conf = PRIO;
  synce_conf_value_t value;

  value.prio = prio;

  ICLI_RC_CHECK_PRINT_RC(synce_icli_nominate_set(clk_list, conf, value, no));
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_wtr(i32 session_id, u8 wtr_value, BOOL no)
{
  synce_conf_type_t conf = WTR_CONF;
  synce_conf_value_t value;

  value.wtr_time = wtr_value;

  ICLI_RC_CHECK_PRINT_RC(synce_icli_selection_set(conf, value, no));
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_ssm(i32 session_id, icli_range_t *clk_list, synce_icli_ssm_type_t type, BOOL has_prc, BOOL has_ssua, BOOL has_ssub, BOOL has_eec2, BOOL has_eec1, BOOL has_dnu, BOOL has_inv,
                       BOOL has_prs, BOOL has_stu, BOOL has_st2, BOOL has_tnc, BOOL has_st3e, BOOL has_smc, BOOL has_prov, BOOL has_dus, BOOL no)
{
  synce_conf_type_t conf;
  if (type == HOLDOVER) {
    conf = SSM_HOLDOVER;
  } else if (type == FREERUN) {
    conf = SSM_FREERUN;
  } else if (type == OVERWRITE) {
    conf = SSM_OVERWRITE;
  } else {
    T_E("Unknown type :%d", type);
    return VTSS_RC_ERROR;
  }

  synce_conf_value_t value;

  if (has_prc) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_PRC;
  } else if (has_ssua) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_SSUA;
  } else if (has_ssub) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_SSUB;
  } else if (has_eec2) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_EEC2;
  } else if (has_eec1) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_EEC1;
  } else if (has_inv) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_INV;
  } else if (has_dnu) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_DNU;
  } else if (has_prs) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_PRS;
  } else if (has_stu) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_STU;
  } else if (has_st2) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_ST2;
  } else if (has_tnc) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_TNC;
  } else if (has_st3e) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_ST3E;
  } else if (has_smc) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_SMC;
  } else if (has_prov) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_PROV;
  } else if (has_dus) {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_DUS;
  } else {
    value.ssm_holdover = VTSS_APPL_SYNCE_QL_NONE;
  }

  if (conf == SSM_OVERWRITE) {
    ICLI_RC_CHECK_PRINT_RC(synce_icli_nominate_set(clk_list, conf, value, no));
  } else {
    ICLI_RC_CHECK_PRINT_RC(synce_icli_selection_set(conf, value, no));
  }
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc sycne_icli_option(i32 session_id, BOOL has_eec1, BOOL has_eec2, BOOL no)
{
  synce_conf_type_t conf = EEC_OPTION;
  synce_conf_value_t value;

  if (has_eec1) {
    value.eec_option = VTSS_APPL_SYNCE_EEC_OPTION_1;
  } else if (has_eec2) {
    value.eec_option = VTSS_APPL_SYNCE_EEC_OPTION_2;
  } else {
    value.eec_option = VTSS_APPL_SYNCE_EEC_OPTION_1; // Assign a default value.
  }

  mesa_rc rc = synce_icli_selection_set(conf, value, no);
  if (rc == SYNCE_RC_INVALID_PARAMETER) {
    ICLI_PRINTF("Could not set EEC option. Please note when changing EEC option SSM Freerun and SSM Holdover must both be set to default.\n");
  } else {
    ICLI_RC_CHECK_PRINT_RC(rc);
  }
  return VTSS_RC_OK;
}

//  see synce_icli_functions.h
mesa_rc synce_icli_sync(i32 session_id, icli_stack_port_range_t *plist, BOOL no)
{
  ICLI_RC_CHECK_PRINT_RC(sync_ssm_enable(plist, no));
  return VTSS_RC_OK;

}

//  see synce_icli_functions.h
mesa_rc synce_icli_debug_testing_script_synce_clock_type(i32 session_id)
{
  uint clock_type;

  (void) clock_station_clock_type_get(&clock_type);
  ICLI_PRINTF("SyncE Clock Type is %u\n", clock_type);

  return VTSS_RC_OK;
}
mesa_rc synce_icli_debug_cpld_read(i32 session_id, u8 reg)
{
  u8 value = 0;
  if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
    if (reg > 0x23) {
      ICLI_PRINTF("Error:: reg = 0x%0x is not a valid register address\n", reg);
    } else {
      pcb107_cpld_read(reg, &value);
      pcb107_cpld_read(reg, &value);
      ICLI_PRINTF("reg[0x%0x] = 0x%0x\n", reg, value);
    }
  } else if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
    if (reg > 0x10) {
      ICLI_PRINTF("Error:: reg = 0x%0x is not a valid register address\n", reg);
    } else {
      pcb135_cpld_read(reg, &value);
      pcb135_cpld_read(reg, &value);
      ICLI_PRINTF("reg[0x%0x] = 0x%0x\n", reg, value);
    }
  } else {
    T_EG(TRACE_GRP_API, "CPLD not supported");
  }
  return VTSS_RC_OK;
}
mesa_rc synce_icli_debug_cpld_write(i32 session_id, u8 reg, u8 value)
{
  u8 value_rd = 0;
  ICLI_PRINTF("in param reg 0x%0x ,value 0x%0x\n", reg, value);
  if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
    if (reg > 0x23) {
      ICLI_PRINTF("Error:: reg = 0x%0x is not a valid register address\n", reg);
    } else {
      pcb107_cpld_write(reg, value);
      ICLI_PRINTF("Successfully written \n");
      pcb107_cpld_read(reg, &value_rd);
      pcb107_cpld_read(reg, &value_rd);
      ICLI_PRINTF("reg[0x%0x] = 0x%0x\n", reg, value_rd);
    }
  } else if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
    if (reg > 0x10) {
      ICLI_PRINTF("Error:: reg = 0x%0x is not a valid register address\n", reg);
    } else {
      pcb135_cpld_write(reg, value);
      ICLI_PRINTF("Successfully written \n");
      pcb135_cpld_read(reg, &value_rd);
      pcb135_cpld_read(reg, &value_rd);
      ICLI_PRINTF("reg[0x%0x] = 0x%0x\n", reg, value_rd);
    }
  } else {
    T_EG(TRACE_GRP_API, "CPLD not supported");
  }
  return VTSS_RC_OK;
}
/***************************************************************************/
/* ICFG callback functions */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_ICFG
static mesa_rc synce_icfg_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
  vtss_isid_t isid;
  mesa_port_no_t iport;
  vtss_icfg_conf_print_t conf_print;
  vtss_icfg_conf_print_init(&conf_print, req, result);

  char buf[75]; // Buffer for storage of string

  isid =  req->instance_id.port.isid;

  // Get default configuration
  vtss_appl_synce_clock_source_nomination_config_t clock_source_nomination_config_default[SYNCE_NOMINATED_MAX];
  vtss_appl_synce_clock_selection_mode_config_t    clock_selection_mode_config_default;
  vtss_appl_synce_station_clock_config_t           station_clock_config_default;
  vtss_appl_synce_port_config_t                    port_config_default[SYNCE_PORT_COUNT];

  synce_mgmt_set_clock_source_nomination_config_to_default(clock_source_nomination_config_default);
  synce_get_clock_selection_mode_config_default(&clock_selection_mode_config_default);
  synce_mgmt_set_station_clock_config_to_default(&station_clock_config_default);
  synce_mgmt_set_port_config_to_default(port_config_default);

  T_NG(TRACE_GRP_CLI, "mode:%d", req->cmd_mode);
  switch (req->cmd_mode) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG: {

    // Get current configuration
    vtss_appl_synce_clock_source_nomination_config_t clock_source_nomination_config[SYNCE_NOMINATED_MAX];
    vtss_appl_synce_clock_selection_mode_config_t    clock_selection_mode_config;
    vtss_appl_synce_station_clock_config_t           station_clock_config;

    (void)vtss_synce_clock_source_nomination_config_all_get(clock_source_nomination_config);
    (void)vtss_appl_synce_clock_selection_mode_config_get(&clock_selection_mode_config);
    (void)vtss_appl_synce_station_clock_config_get(&station_clock_config);

    //
    // Output clock
    //
    if (synce_icli_any_output_freq_supported()) {
      conf_print.is_default = station_clock_config_default.station_clk_out == station_clock_config.station_clk_out;
      conf_print.force_no_keyword = TRUE;  // This command doesn't have a default setting without the "no" keyword
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock output-source", "%s",
                                   station_clock_config.station_clk_out == VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ ? "1544khz" :
                                   station_clock_config.station_clk_out == VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ ? "2048khz" :
                                   station_clock_config.station_clk_out == VTSS_APPL_SYNCE_STATION_CLK_10_MHZ ? "10mhz" :
                                   conf_print.is_default ? "" : "Wrong parameter"));
      conf_print.force_no_keyword = FALSE; // Back to default
    }

    //
    // wtr
    //
    conf_print.is_default = clock_selection_mode_config.wtr_time == clock_selection_mode_config_default.wtr_time;
    VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock wait-to-restore", "%u", clock_selection_mode_config.wtr_time));

    //
    // EEC option
    //
    conf_print.is_default = clock_selection_mode_config.eec_option == clock_selection_mode_config_default.eec_option;
    VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock option", "%s",
                                 clock_selection_mode_config.eec_option == VTSS_APPL_SYNCE_EEC_OPTION_1 ? "eec1" :
                                 clock_selection_mode_config.eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2 ? "eec2" : "error"));

    //
    // SSH HOLDOVER
    //
    conf_print.force_no_keyword   = TRUE;  // This one comes with a default of VTSS_APPL_SYNCE_QL_NONE, which isn't supported in the no-non-form
    conf_print.print_no_arguments = FALSE; // Prevent it from printing %s.
    conf_print.is_default = clock_selection_mode_config.ssm_holdover == clock_selection_mode_config_default.ssm_holdover;
    VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock ssm-holdover", "%s",
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_PRC  ? "prc"  :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_SSUA ? "ssua" :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_SSUB ? "ssub" :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_EEC1 ? "eec1" :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_EEC2 ? "eec2" :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_INV  ? "inv"  :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_DNU  ? "dnu"  :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_PRS  ? "prs"  :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_STU  ? "stu"  :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_ST2  ? "st2"  :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_TNC  ? "tnc"  :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_ST3E ? "st3e" :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_SMC  ? "smc"  :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_PROV ? "prov" :
                                 clock_selection_mode_config.ssm_holdover == VTSS_APPL_SYNCE_QL_DUS  ? "dus"  :
                                 "dnu"));
    // Back to defaults
    conf_print.force_no_keyword   = FALSE;
    conf_print.print_no_arguments = TRUE;

    //
    // SSH FREE RUNNING
    //
    conf_print.force_no_keyword   = TRUE;  // This one comes with a default of VTSS_APPL_SYNCE_QL_NONE, which isn't supported in the no-non-form
    conf_print.print_no_arguments = FALSE; // Prevent it from printing %s.
    conf_print.is_default = (clock_selection_mode_config.ssm_freerun == clock_selection_mode_config_default.ssm_freerun);
    VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock ssm-freerun", "%s",
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_PRC  ? "prc"  :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_SSUA ? "ssua" :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_SSUB ? "ssub" :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_EEC1 ? "eec1" :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_EEC2 ? "eec2" :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_INV  ? "inv"  :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_DNU  ? "dnu"  :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_PRS  ? "prs"  :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_STU  ? "stu"  :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_ST2  ? "st2"  :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_TNC  ? "tnc"  :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_ST3E ? "st3e" :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_SMC  ? "smc"  :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_PROV ? "prov" :
                                 clock_selection_mode_config.ssm_freerun == VTSS_APPL_SYNCE_QL_DUS  ? "dus"  :
                                 "dnu"));

    // Back to defaults
    conf_print.force_no_keyword   = FALSE;
    conf_print.print_no_arguments = TRUE;

    u8 clk_src;

    for (clk_src = 0; clk_src < synce_my_nominated_max; clk_src++) {
      //
      // Nominate
      //

      // The nominate member works as an enabled-bit, and if not enabled (which is the default), no other lines are output.
      // Unfortunately, any non-no ICLI command that includes the "nominate" keyword will set the nominate bit to TRUE, enabling the feature.
      // The vtss_icfg_conf_print() cannot print the 'no' keyword unless it is forced to. This is what we have to do in this case, since
      // the nominate feature cannot be enabled separately.
      conf_print.force_no_keyword = TRUE;
      conf_print.is_default = clock_source_nomination_config[clk_src].nominated == clock_source_nomination_config_default[clk_src].nominated;

      if (clock_source_nomination_config[clk_src].network_port == 0) {  // data_port == 0 means station clock or PTP 8265.1
        if (clock_source_nomination_config[clk_src].clk_in_port == 0) {
          VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock clk-source", "%d nominate clk-in", synce_iclk2uclk(clk_src)));
        } else {
          VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock clk-source", "%d nominate ptp %d", synce_iclk2uclk(clk_src), clock_source_nomination_config[clk_src].clk_in_port - 128));
        }
      } else {
        u32 port;
        (void) synce_network_port_clk_in_port_combo_to_port(clock_source_nomination_config[clk_src].network_port, 0, &port);
        VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock clk-source", "%d nominate interface %s", synce_iclk2uclk(clk_src), icli_port_info_txt(VTSS_USID_START, iport2uport(port), buf)));
      }

      // Back to default
      conf_print.force_no_keyword = FALSE;

      //
      // Aneg
      //
      conf_print.force_no_keyword = TRUE; // This command doesn't have a non-no form of its default.
      vtss_appl_synce_aneg_mode_t aneg_mode = clock_source_nomination_config[clk_src].aneg_mode;

      conf_print.is_default = aneg_mode == clock_source_nomination_config_default[clk_src].aneg_mode;
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock clk-source", "%d aneg-mode%s", synce_iclk2uclk(clk_src),
                                   aneg_mode == VTSS_APPL_SYNCE_ANEG_PREFERED_MASTER ? " master" :
                                   aneg_mode == VTSS_APPL_SYNCE_ANEG_PREFERED_SLAVE  ? " slave"  :
                                   aneg_mode == VTSS_APPL_SYNCE_ANEG_FORCED_SLAVE    ? " forced" :
                                   conf_print.is_default ? "" : " Unknown Mode"));
      // Back to default
      conf_print.force_no_keyword = FALSE;

      //
      // Prio
      //
      uint priority = clock_source_nomination_config[clk_src].priority;
      conf_print.is_default = priority == clock_source_nomination_config_default[clk_src].priority;
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock clk-source", "%d priority %u", synce_iclk2uclk(clk_src), priority));

      //
      // SSM overwrite
      //
      conf_print.force_no_keyword = TRUE; // This one doesn't support the default ssm-overwrite, which is VTSS_APPL_SYNCE_QL_NONE in it's non-no-form, so we force vtss_icfg_conf_print() to use the no keyword
      vtss_appl_synce_quality_level_t ssm_overwrite = clock_source_nomination_config[clk_src].ssm_overwrite;
      conf_print.is_default = ssm_overwrite == clock_source_nomination_config_default[clk_src].ssm_overwrite;
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock clk-source", "%d ssm-overwrite%s", synce_iclk2uclk(clk_src),
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_PRC  ? " prc"  :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_SSUA ? " ssua" :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_SSUB ? " ssub" :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_EEC1 ? " eec1" :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_EEC2 ? " eec2" :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_DNU  ? " dnu"  :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_PRS  ? " prs"  :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_STU  ? " stu"  :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_ST2  ? " st2"  :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_TNC  ? " tnc"  :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_ST3E ? " st3e" :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_SMC  ? " smc"  :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_PROV ? " prov" :
                                   ssm_overwrite == VTSS_APPL_SYNCE_QL_DUS  ? " dus"  : conf_print.is_default ? "" : " Unknown Quality Level"));
      // Back to default
      conf_print.force_no_keyword = FALSE;

      //
      // Hold Off
      //
      conf_print.force_no_keyword = TRUE; // This one doesn't support the default hold-timeout of 0 in it's non-no-form, so we use the no keyword
      uint holdoff_time = clock_source_nomination_config[clk_src].holdoff_time;
      conf_print.is_default = holdoff_time == clock_source_nomination_config_default[clk_src].holdoff_time;
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock clk-source", "%d hold-timeout %u", synce_iclk2uclk(clk_src), holdoff_time));
      // Back to default
      conf_print.force_no_keyword = FALSE;
    }

    //
    // Input clock (must be set after nomination)
    //
    if (synce_icli_any_input_freq_supported()) {
      conf_print.is_default = station_clock_config_default.station_clk_in == station_clock_config.station_clk_in;
      conf_print.force_no_keyword = TRUE;  // This command doesn't have a default setting without the "no" keyword
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock input-source", "%s",
                                   station_clock_config.station_clk_in == VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ ? "1544khz" :
                                   station_clock_config.station_clk_in == VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ ? "2048khz" :
                                   station_clock_config.station_clk_in == VTSS_APPL_SYNCE_STATION_CLK_10_MHZ ? "10mhz" :
                                   conf_print.is_default ? "" : "Wrong parameter"));
      conf_print.force_no_keyword   = FALSE; // Back to default
    }

    //
    // Selection (must be set after nomination)
    //
    vtss_appl_synce_selection_mode_t selection_mode = clock_selection_mode_config.selection_mode;

    conf_print.is_default = selection_mode == clock_selection_mode_config_default.selection_mode;
    if (selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL) {
      sprintf(buf, " clk-source %u", clock_selection_mode_config.source);
    } else {
      buf[0] = '\0';
    }

    VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock selector", "%s%s",
                                 selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL                 ? "manual"       :
                                 selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED     ? "selected"     :
                                 selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE ? "nonrevertive" :
                                 selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE    ? "revertive"    :
                                 selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER        ? "holdover"     :
                                 selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN        ? "freerun"      : conf_print.is_default ? "" : " Unknown selector",
                                 buf));
    break;
  }  //  End of case ICLI_CMD_MODE_GLOBAL_CONFIG

  case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
    iport = req->instance_id.port.begin_iport;

    T_DG_PORT(TRACE_GRP_CLI, iport, "Isid:%d, req->instance_id.port.usid:%d configurable:%d", isid, req->instance_id.port.usid, msg_switch_configurable(isid));

    if (msg_switch_configurable(isid)) {
      // Get current configuration
      vtss_appl_synce_port_config_t port_config;
      vtss_ifindex_t                ifindex;

      (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, iport, &ifindex);
      (void)vtss_appl_synce_port_config_get(ifindex, &port_config);

      //
      // Enable/Disable
      //
      conf_print.is_default = port_config.ssm_enabled == port_config_default[iport].ssm_enabled;
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "network-clock synchronization ssm", "%s", ""));
    }
    break;

  default:
    // Not needed
    break;
  }

  return VTSS_RC_OK;
}

static void synce_print_error(uint   rc)
{
  (void)icli_session_self_printf(synce_error_txt(rc));
}


mesa_rc synce_icli_debug_read(i32 session_id, u32 reg, u32 cnt)
{
  uint32_t i;
  uint   value, n, rc = VTSS_RC_OK;

  if (cnt == 0) {
    cnt++;
  }
  if ((((reg + cnt - 1) > 55) && (reg < 128)) ||
      (((reg + cnt - 1) > 143) && (reg != 185))) {
    (void)icli_session_self_printf("Undefined address space\n");
  }

  for (i = 0; i < cnt; i++) {
    if ((rc = synce_mgmt_register_get(i + reg, &value)) != SYNCE_RC_OK) {
      synce_print_error(rc);
      return rc;
    }
    n = (i & 0x7);
    if (n == 0) {
      (void)icli_session_self_printf("%03u: ", i + reg);
    }
    (void)icli_session_self_printf("%02x ", value);
    if (n == 0x7 || i == (cnt - 1)) {
      (void)icli_session_self_printf("\n");
    }
  }

  return rc;
}

mesa_rc synce_icli_debug_write(i32 session_id, u32 reg, u32 value)
{
  uint rc = VTSS_RC_OK;

  if (((reg > 55) && (reg < 128)) || (reg > 143)) {
    (void)icli_session_self_printf("Undefined address space\n");
  }

  if ((rc = synce_mgmt_register_set(reg, value)) != SYNCE_RC_OK) {
    synce_print_error(rc);
  }

  return rc;
}

mesa_rc synce_icli_debug_subject_write(i32 session_id, char *subjectname, u32 ix, char *value)
{
  (void)icli_session_self_printf("write value %s to subject %s, index %d - ", value, subjectname, ix);
  mesa_rc rc = synce_subject_debug_write(subjectname, ix, value);
  if (rc != VTSS_RC_OK) {
    (void)icli_session_self_printf("failed\n");
  } else {
    (void)icli_session_self_printf("success\n");
  }

  return rc;
}

mesa_rc synce_icli_debug_test(i32 session_id)
{
  uint  rc = VTSS_RC_OK;
  uint  reg;

  if ((vtss_board_type() == VTSS_BOARD_JAG_CU24_REF) || (vtss_board_type() == VTSS_BOARD_JAG_SFP24_REF) || (vtss_board_type() == VTSS_BOARD_JAG_PCB107_REF)) {
    /* On Jaguar platform clock port 2 has to be used for 156,25 MHz to 10G PHY */
    if ((rc = synce_mgmt_register_get(25, &reg)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write_masked(25, 0x00, 0xE0);   /* N1_HS: 4 */
    }
    reg &= ~0xE0;
    if ((rc = synce_mgmt_register_set(25, reg)) != SYNCE_RC_OK) {
      synce_print_error(rc);
    }

    if ((rc = synce_mgmt_register_set(31, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(31, 0);   /* NC1_LS: 10 */
    }
    if ((rc = synce_mgmt_register_set(32, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(32, 0);   /* NC1_LS: 10 */
    }
    if ((rc = synce_mgmt_register_set(33, 9)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(33, 9);   /* NC1_LS: 10 */
    }

    if ((rc = synce_mgmt_register_set(34, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(34, 0);   /* NC2_LS: 8 */
    }
    if ((rc = synce_mgmt_register_set(35, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(35, 0);   /* NC2_LS: 8 */
    }
    if ((rc = synce_mgmt_register_set(36, 7)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(36, 7);   /* NC2_LS: 8 */
    }

    if ((rc = synce_mgmt_register_get(40, &reg)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write_masked(40, 0x20, 0xE0);   /* N2_HS: 5 */
    }
    reg &= ~0xEF;                                                                               //spi_write_masked(40, 0, 0x0F);      /* N2_LS: 600 */
    reg |= 0x20;
    if ((rc = synce_mgmt_register_set(40, reg)) != SYNCE_RC_OK) {
      synce_print_error(rc);
    }

    if ((rc = synce_mgmt_register_set(41, 0x02)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(41, 0x02);                /* N2_LS: 600 */
    }
    if ((rc = synce_mgmt_register_set(42, 0x57)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(42, 0x57);                /* N2_LS: 600 */
    }

    if ((rc = synce_mgmt_register_set(43, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(43, 0);   /* N31: 75 */
    }
    if ((rc = synce_mgmt_register_set(44, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(44, 0);   /* N31: 75 */
    }
    if ((rc = synce_mgmt_register_set(45, 74)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(45, 74);  /* N31: 75 */
    }

    if ((rc = synce_mgmt_register_set(46, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(46, 0);   /* N32: 6 */
    }
    if ((rc = synce_mgmt_register_set(47, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(47, 0);   /* N32: 6 */
    }
    if ((rc = synce_mgmt_register_set(48, 5)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(48, 5);   /* N32: 6 */
    }
  } else {
    /* 10 MHz clockoutput on port 2 is wanted and possible */
    if ((rc = synce_mgmt_register_get(25, &reg)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write_masked(25, 0x20, 0xE0);   /* N1_HS: 5 */
    }
    reg &= ~0xE0;
    reg |= 0x20;
    if ((rc = synce_mgmt_register_set(25, reg)) != SYNCE_RC_OK) {
      synce_print_error(rc);
    }

    if ((rc = synce_mgmt_register_set(31, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(31, 0);   /* NC1_LS: 8 */
    }
    if ((rc = synce_mgmt_register_set(32, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(32, 0);   /* NC1_LS: 8 */
    }
    if ((rc = synce_mgmt_register_set(33, 7)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(33, 7);   /* NC1_LS: 8 */
    }

    if ((rc = synce_mgmt_register_set(34, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(34, 0);   /* NC2_LS: 100 */
    }
    if ((rc = synce_mgmt_register_set(35, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(35, 0);   /* NC2_LS: 100 */
    }
    if ((rc = synce_mgmt_register_set(36, 63)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(36, 63);   /* NC2_LS: 100 */
    }

    if ((rc = synce_mgmt_register_get(40, &reg)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write_masked(40, 0x00, 0xE0);   /* N2_HS: 4 */
    }
    reg &= ~0xEF;                                                                               //spi_write_masked(40, 0, 0x0F);      /* N2_LS: 750 */
    if ((rc = synce_mgmt_register_set(40, reg)) != SYNCE_RC_OK) {
      synce_print_error(rc);
    }

    if ((rc = synce_mgmt_register_set(41, 0x02)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(41, 0x02);                /* N2_LS: 750 */
    }
    if ((rc = synce_mgmt_register_set(42, 0xED)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(42, 0xED);                /* N2_LS: 750 */
    }

    if ((rc = synce_mgmt_register_set(43, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(43, 0);   /* N31: 75 */
    }
    if ((rc = synce_mgmt_register_set(44, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(44, 0);   /* N31: 75 */
    }
    if ((rc = synce_mgmt_register_set(45, 74)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(45, 74);  /* N31: 75 */
    }

    if ((rc = synce_mgmt_register_set(46, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(46, 0);   /* N32: 6 */
    }
    if ((rc = synce_mgmt_register_set(47, 0)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(47, 0);   /* N32: 6 */
    }
    if ((rc = synce_mgmt_register_set(48, 5)) != SYNCE_RC_OK) {
      synce_print_error(rc);  //spi_write(48, 5);   /* N32: 6 */
    }
  }

  return rc;

}

/* ICFG Initialization function */
mesa_rc synce_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_SYNCE_GLOBAL_CONF, "network-clock", synce_icfg_conf));
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_SYNCE_INTERFACE_CONF, "network-clock", synce_icfg_conf));
  return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG

