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

#ifdef VTSS_SW_OPTION_LLDP

#include "icli_api.h"
#include "icli_porting_util.h"
#include "lldp_trace.h"
#include "lldp_api.h"
#include "lldp_os.h"
#include "msg_api.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif
#include "lldp_icli_shared_functions.h" /* For lldp_local_interface_txt_get */
#include "lldp_icli_functions.h"
#ifdef VTSS_SW_OPTION_EEE
#include "eee_api.h"
#endif
#ifdef VTSS_SW_OPTION_TSN
#include "vtss/appl/tsn.h"
#endif
#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"                   /* For aggr_mgmt_group_member_t */
#endif
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#include "qos_api.h"


/***************************************************************************/
/*  Code start :)                                                           */
/****************************************************************************/

// Help function for printing the TLV information
// In : Session_Id : Session_Id for ICLI_PRINTF
//      entry      : LLDP neighbor entry containing the neighbor information
//      field      : Which TLV field to print
//      mgmt_addr_index : We support multiple management address. Index for which one to print
static void icli_lldp_tlv(u32 session_id, const char *name, vtss_appl_lldp_remote_entry_t *entry, lldp_tlv_t field, u8 mgmt_addr_index)
{
  char string[512];
  ICLI_PRINTF("%-20s: ", name);
  lldp_remote_tlv_to_string(entry, field, &string[0], sizeof(string),  mgmt_addr_index);
  ICLI_PRINTF("%s\n", string);
}

// Function printing the statistics header
//In: session_id : For ICLI printing
static void lldp_icli_print_statistic_header(i32 session_id)
{
  char   buf[255];

  sprintf(buf, "%-23s %-11s %-11s %-11s %-11s %-11s %-11s %-11s %-11s", "", "Rx", "Tx", "Rx", "Rx", "Rx TLV", "Rx TLV", "Rx TLV", "");
  ICLI_PRINTF("%s\n", buf);
  sprintf(buf, "%-23s %-11s %-11s %-11s %-11s %-11s %-11s %-11s %-11s", "Interface", "Frames", "Frames", "Errors", "Discards", "Errors", "Unknown", "Organiz.", "Aged");
  icli_parm_header(session_id, buf);
}

// Help function for printing LLDP status
// Same arguments as for the lldp_icli_status function
// Return : Vitesse return code
//      show_eee      - TRUE to print EEE information, else default information is printed
static mesa_rc lldp_icli_status_print(i32 session_id, BOOL show_neighbors, BOOL show_statistics, BOOL has_interface, icli_stack_port_range_t *list, BOOL show_eee, BOOL show_preempt)
{
  switch_iter_t sit;
  port_iter_t pit;
  BOOL lldp_no_entry_found = TRUE;
  BOOL print_global = TRUE;
  char buf[255];
  u8 mgmt_addr_index;

  // Loop through all switches in a stack
  VTSS_RC(icli_switch_iter_init(&sit));
  T_IG(TRACE_GRP_CLI, "show_eee:%d", show_eee);
  while (icli_switch_iter_getnext(&sit, list)) {
    if (!msg_switch_exists(sit.isid)) {
      T_IG(TRACE_GRP_CLI, "isid:%d doesn't exit", sit.isid);
      continue;
    }

    // Showing neighbor information
    if (show_neighbors || show_eee || show_preempt) {
      // Loop though the ports
      T_IG(TRACE_GRP_CLI, "getting ports sid:%d", sit.isid);
      VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
      while (icli_port_iter_getnext(&pit, list)) {

        vtss_ifindex_t ifindex;
        VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));
        vtss_appl_lldp_remote_entry_t entry;
        // Loop though all the entries containing information
        vtss_lldp_entry_index_t entry_index = 0;
        while (vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, TRUE) == VTSS_RC_OK) {
          entry_index++;
          lldp_no_entry_found = FALSE; // Remember that at least one entry was found.

          // Print all the TLVs
          ICLI_PRINTF("%-20s: %s\n", "Local Interface", lldp_local_interface_txt_get(buf, &entry, &sit, &pit));

#ifdef VTSS_SW_OPTION_EEE
          if (show_eee) {
            eee_switch_state_t  eee_state;
            VTSS_RC(eee_mgmt_switch_state_get(sit.isid, &eee_state));

            eee_switch_conf_t switch_conf;
            VTSS_RC(eee_mgmt_switch_conf_get(sit.isid, &switch_conf));

            if (!eee_state.port[pit.iport].eee_capable) {
              ICLI_PRINTF("EEE not supported for this interface\n");

            } else if (!switch_conf.port[pit.iport].eee_ena) {
              ICLI_PRINTF("EEE not enabled for this interface\n");

            } else if (!eee_state.port[pit.iport].link_partner_eee_capable) {
              ICLI_PRINTF("Link partner is not EEE capable\n");

            } else {
              u8 in_sync = (entry.eee.RemTxTwSysEcho == eee_state.port[pit.iport].LocTxSystemValue) &&
                           (entry.eee.RemRxTwSysEcho == eee_state.port[pit.iport].LocRxSystemValue);

              T_DG_PORT(TRACE_GRP_CLI,                      entry.receive_port, "in_sync:%d", in_sync);
              ICLI_PRINTF("%-20s: %d \n", "Tx Tw",          entry.eee.RemTxTwSys);
              ICLI_PRINTF("%-20s: %d \n", "Rx Tw",          entry.eee.RemRxTwSys);
              ICLI_PRINTF("%-20s: %d \n", "Fallback Rx Tw", entry.eee.RemFbTwSys);
              ICLI_PRINTF("%-20s: %d \n", "Echo Tx Tw",     entry.eee.RemTxTwSysEcho);
              ICLI_PRINTF("%-20s: %d \n", "Echo Rx Tw",     entry.eee.RemRxTwSysEcho);
              ICLI_PRINTF("%-20s: %d \n", "Resolved Tx Tw", eee_state.port[pit.iport].LocResolvedTxSystemValue);
              ICLI_PRINTF("%-20s: %d \n", "Resolved Rx Tw", eee_state.port[pit.iport].LocResolvedRxSystemValue);
              ICLI_PRINTF("%-20s: %s \n", "EEE in sync",    in_sync ? "Yes" : "No");
              ICLI_PRINTF("\n");
              continue;
            }
          } // Show_eee
#endif
          if (show_neighbors) {
            icli_lldp_tlv(session_id, "Chassis ID",          &entry, LLDP_TLV_BASIC_MGMT_CHASSIS_ID, 0);
            icli_lldp_tlv(session_id, "Port ID",             &entry, LLDP_TLV_BASIC_MGMT_PORT_ID, 0);
            icli_lldp_tlv(session_id, "Port Description",    &entry, LLDP_TLV_BASIC_MGMT_PORT_DESCR, 0);
            icli_lldp_tlv(session_id, "System Name",         &entry, LLDP_TLV_BASIC_MGMT_SYSTEM_NAME, 0);
            icli_lldp_tlv(session_id, "System Description",  &entry, LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR, 0);
            icli_lldp_tlv(session_id, "System Capabilities", &entry, LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA, 0);

            // Printing management address if present
            for (mgmt_addr_index = 0; mgmt_addr_index < LLDP_MGMT_ADDR_CNT; mgmt_addr_index++) {
              if (entry.mgmt_addr[mgmt_addr_index].length > 0) {
                icli_lldp_tlv(session_id, "Management Address", &entry, LLDP_TLV_BASIC_MGMT_MGMT_ADDR, mgmt_addr_index);
              }
            }
#ifdef VTSS_SW_OPTION_POE
            (void) vtss_appl_lldp_remote_poeinfo2string(&entry, &buf[0], sizeof(buf), 0);
            ICLI_PRINTF("%-20s: %s\n", "PoE Type", buf);
            (void) vtss_appl_lldp_remote_poeinfo2string(&entry, &buf[0], sizeof(buf), 1);
            ICLI_PRINTF("%-20s: %s\n", "PoE Source", buf);
            (void) vtss_appl_lldp_remote_poeinfo2string(&entry, &buf[0], sizeof(buf), 2);
            ICLI_PRINTF("%-20s: %s\n", "PoE Power", buf);
            (void) vtss_appl_lldp_remote_poeinfo2string(&entry, &buf[0], sizeof(buf), 3);
            ICLI_PRINTF("%-20s: %s\n", "PoE Priority", buf);
#endif
          }
#ifdef VTSS_SW_OPTION_TSN
          if (lldp_is_frame_preemption_supported()) {
            if (show_preempt) {
              vtss_appl_tsn_fp_status_t local_preempt_status;
              VTSS_RC(vtss_appl_tsn_fp_status_get(ifindex, &local_preempt_status));
              ICLI_PRINTF("Frame Preemption Status Local & Remote \n");
              ICLI_PRINTF("=========================================   \n");
              ICLI_PRINTF("%-20s: %s \n", "LocalPreemptSupported",      local_preempt_status.loc_preempt_supported ? "TRUE" : "FALSE");
              ICLI_PRINTF("%-20s: %s \n", "LocalPreemptEnabled",        local_preempt_status.loc_preempt_enabled ? "TRUE" : "FALSE");
              ICLI_PRINTF("%-20s: %s \n", "LocalPreemptActive",         local_preempt_status.loc_preempt_active ? "TRUE" : "FALSE");
              ICLI_PRINTF("%-20s: %u (%u octets) \n", "LocalFragSize",  local_preempt_status.loc_add_frag_size, (local_preempt_status.loc_add_frag_size + 1) * 64);
              ICLI_PRINTF("%-20s: %s \n", "RemotePreemptSupported",     entry.fp.RemFramePreemptSupported ? "TRUE" : "FALSE");
              ICLI_PRINTF("%-20s: %s \n", "RemotePreemptEnabled",       entry.fp.RemFramePreemptEnabled ? "TRUE" : "FALSE");
              ICLI_PRINTF("%-20s: %s \n", "RemotePreemptActive",        entry.fp.RemFramePreemptActive ? "TRUE" : "FALSE");
              ICLI_PRINTF("%-20s: %u (%u octets) \n", "RemoteFragSize", entry.fp.RemFrameAddFragSize, (entry.fp.RemFrameAddFragSize + 1) * 64);
            }
          }
#endif
          ICLI_PRINTF("\n");
          // Select next entry
        }
      }

      // If no entries with valid information were found then tell the user.
      if (lldp_no_entry_found) {
        ICLI_PRINTF("No LLDP entries found \n");
      }
    } // End show neighbors


    // Don't print global information if user has specified at specific interface.
    if (has_interface) {
      print_global = FALSE;
    }


    // showing  statistics
    if (show_statistics) {

      // Get the statistics
      vtss_appl_lldp_global_counters_t global_stat;
      VTSS_RC(vtss_appl_lldp_stat_global_get(&global_stat));

      // Printing global information
      if (print_global) {
        char             last_change_str[255];

        lldp_mgmt_last_change_ago_to_str(global_stat.last_change_ago, &last_change_str[0]);

        // Global counters //
        ICLI_PRINTF("LLDP global counters\n");
        ICLI_PRINTF("Neighbor entries was last changed at %s.\n", last_change_str);
        ICLI_PRINTF("Total Neighbors Entries Added    %d.\n", global_stat.table_inserts);
        ICLI_PRINTF("Total Neighbors Entries Deleted  %d.\n", global_stat.table_deletes);
        ICLI_PRINTF("Total Neighbors Entries Dropped  %d.\n", global_stat.table_drops);
        ICLI_PRINTF("Total Neighbors Entries Aged Out %d.\n", global_stat.table_ageouts);

        // Local counters //
        ICLI_PRINTF("\nLLDP local counters\n");
      } // end print_global

      lldp_icli_print_statistic_header(session_id);
#ifdef VTSS_SW_OPTION_AGGR
      aggr_mgmt_group_member_t glag_members;
      mesa_glag_no_t glag_no;
      // Insert the statistic for the GLAGs. The lowest port number in the GLAG collection contains statistic (See also packet_api.h).
      for (glag_no = AGGR_MGMT_GROUP_NO_START; glag_no < (AGGR_MGMT_GROUP_NO_START + AGGR_LLAG_CNT_); glag_no++) {
        mesa_rc aggr_rc;
        // Get the port members
        T_D("Get glag members, isid:%d, glag_no:%u", sit.isid, glag_no);
        aggr_rc = aggr_mgmt_members_get(sit.isid, glag_no, &glag_members, FALSE);

        if (aggr_rc == AGGR_ERROR_ENTRY_NOT_FOUND) {
          continue;
        }

        VTSS_RC(aggr_rc); // Stop in case of error from the aggr module

        mesa_port_no_t      iport_number;
        // Loop through all ports. Stop at first port that is part of the GLAG.
        for (iport_number = 0; iport_number < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport_number++) {
          if (iport_number < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) { // Make sure that we don't index array out-of-bounds.
            if (glag_members.entry.member[iport_number]) {
              char lag_string[70] = "";
              sprintf(lag_string, "LLAG %u", glag_no - AGGR_MGMT_GROUP_NO_START + 1);

              vtss_appl_lldp_port_counters_t stati;
              vtss_ifindex_t ifindex;
              VTSS_RC(vtss_ifindex_from_port(sit.isid, iport_number, &ifindex));
              VTSS_RC(vtss_appl_lldp_stat_if_get(ifindex, &stati));

              ICLI_PRINTF("%-23s %-11d %-11d %-11d %-11d %-11d %-11d %-11d %-11d\n",
                          lag_string,
                          stati.statsFramesInTotal,
                          stati.statsFramesOutTotal,
                          stati.statsFramesInErrorsTotal,
                          stati.statsFramesDiscardedTotal,
                          stati.statsTLVsDiscardedTotal,
                          stati.statsTLVsUnrecognizedTotal,
                          stati.statsOrgTVLsDiscarded,
                          stati.statsAgeoutsTotal);
              break;
            }
          }
        }
      }
#endif // VTSS_SW_OPTION_AGGR

      // loop through the ports.
      VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
      while (icli_port_iter_getnext(&pit, list)) {
        // Check if the port is part of a LAG
        // This function takes an iport and prints a uport.
        if (lldp_remote_receive_port_to_string(pit.iport, buf, sit.isid) == 1) {
          ICLI_PRINTF("%s\n", buf);
        } else {
          vtss_appl_lldp_port_counters_t stati;
          vtss_ifindex_t ifindex;
          VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));
          VTSS_RC(vtss_appl_lldp_stat_if_get(ifindex, &stati));

          ICLI_PRINTF("%-23s %-11d %-11d %-11d %-11d %-11d %-11d %-11d %-11d\n",
                      buf,
                      stati.statsFramesInTotal,
                      stati.statsFramesOutTotal,
                      stati.statsFramesInErrorsTotal,
                      stati.statsFramesDiscardedTotal,
                      stati.statsTLVsDiscardedTotal,
                      stati.statsTLVsUnrecognizedTotal,
                      stati.statsOrgTVLsDiscarded,
                      stati.statsAgeoutsTotal);
        }
      } // End while icli_stack_port_iter_getnext
    } // End show statistics
    print_global = FALSE; // Only print global information once.
  } // end icli_stack_isid_iter_getnext
  return VTSS_RC_OK;
}

// See lldp_icli_functions.h
mesa_rc lldp_icli_status(i32 session_id, BOOL show_neighbors, BOOL show_statistics,  BOOL show_eee, BOOL show_preempt, BOOL has_interface, icli_stack_port_range_t *list)
{
  ICLI_RC_CHECK_PRINT_RC(lldp_icli_status_print(session_id, show_neighbors, show_statistics, has_interface, list, show_eee, show_preempt));
  return VTSS_RC_OK;
}

// See lldp_icli_functions.h
mesa_rc lldp_icli_clear_counters(BOOL has_global, BOOL has_interface, icli_stack_port_range_t *list)
{
  T_IG(TRACE_GRP_CLI, "has_global:%d, has_interface:%d", has_global, has_interface);

  // If the use hasn't specified which counters he wants to clear, then we clear the all.
  if (!has_global && !has_interface) {
    has_global = TRUE;
    has_interface = TRUE;
  }

  if (has_global) {
    VTSS_RC(vtss_appl_lldp_global_stat_clr());
  }

  if (has_interface) {
    switch_iter_t sit;
    port_iter_t pit;

    VTSS_RC(icli_switch_iter_init(&sit));

    while (icli_switch_iter_getnext(&sit, list)) {
      T_IG(TRACE_GRP_CLI, "isid:%d", sit.isid);
      if (!msg_switch_exists(sit.isid)) {
        T_IG(TRACE_GRP_CLI, "isid:%d doesn't exit", sit.isid);
        continue;
      }

      // loop through the ports.
      VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
      while (icli_port_iter_getnext(&pit, list)) {
        vtss_ifindex_t ifindex;
        VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));

        T_IG(TRACE_GRP_CLI, "isid:%d, iport:%d", sit.isid, pit.iport);
        VTSS_RC(vtss_appl_lldp_if_stat_clr(ifindex));
      }
    }
  }
  return VTSS_RC_OK;
}

// See lldp_icli_functions.h
mesa_rc lldp_icli_global_conf(i32 session_id, BOOL holdtime, BOOL tx_interval, BOOL reinit, BOOL transmission_delay, u16 new_val, BOOL no)
{
  vtss_appl_lldp_common_conf_t      lldp_conf;

  // Get current configuration
  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));

  // holdtime
  if (holdtime) {
    if (no) {
      lldp_conf.tx_sm.msgTxHold = VTSS_APPL_LLDP_TX_HOLD_DEFAULT;
    } else {
      lldp_conf.tx_sm.msgTxHold = new_val;
    }
  }

  // transmission delay
  if (transmission_delay) {
    if (no) {
      lldp_conf.tx_sm.txDelay = VTSS_APPL_LLDP_TX_DELAY_DEFAULT;
    } else {
      lldp_conf.tx_sm.txDelay = new_val;
    }

    if (lldp_conf.tx_sm.txDelay > lldp_conf.tx_sm.msgTxInterval * 0.25 ) {
      // We are auto adjusting in order not to stop iCFG when/if the interval is not configured yet
      lldp_conf.tx_sm.msgTxInterval = lldp_conf.tx_sm.txDelay * 4;
      ICLI_PRINTF("Note: According to IEEE 802.1AB-clause 10.5.4.2 the transmission-delay must not be larger than LLDP timer * 0.25. LLDP timer changed to %d\n", lldp_conf.tx_sm.msgTxInterval);
    }
  }

  // interval
  if (tx_interval) {
    if (no) {
      lldp_conf.tx_sm.msgTxInterval = VTSS_APPL_LLDP_TX_INTERVAL_DEFAULT;
    } else {
      lldp_conf.tx_sm.msgTxInterval = new_val;
    }

    if (lldp_conf.tx_sm.txDelay > lldp_conf.tx_sm.msgTxInterval * 0.25 ) {
      // We are auto adjusting in order not to stop iCFG when/if the delay is not configured yet
      lldp_conf.tx_sm.txDelay = (u16)(lldp_conf.tx_sm.msgTxInterval * 0.25);
      ICLI_PRINTF("Note: According to IEEE 802.1AB-clause 10.5.4.2 the transmission-delay must not be larger than LLDP timer * 0.25. Transmission-delay changed to %d\n", lldp_conf.tx_sm.txDelay);
    }
  }

  // Reinit
  if (reinit) {
    if (no) {
      lldp_conf.tx_sm.reInitDelay = VTSS_APPL_LLDP_REINIT_DEFAULT;
    } else {
      lldp_conf.tx_sm.reInitDelay = new_val;
    }
  }

  // Set new configuration.
  ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_common_conf_set(&lldp_conf));
  return VTSS_RC_OK;
}

// See lldp_icli_functions.h
mesa_rc lldp_icli_tlv_select(i32 session_id, icli_stack_port_range_t *plist, BOOL mgmt, BOOL port, BOOL sys_capa, BOOL sys_des, BOOL sys_name, BOOL no)
{
  vtss_appl_lldp_port_conf_t lldp_conf;
  switch_iter_t  sit;
  port_iter_t    pit;

  VTSS_RC(icli_switch_iter_init(&sit));

  while (icli_switch_iter_getnext(&sit, plist)) {

    // Update which optional TLVs to transmit
    if (msg_switch_configurable(sit.isid)) {


      // Loop through ports.
      VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));

      while (icli_port_iter_getnext(&pit, plist)) {
        T_DG(TRACE_GRP_CONF, "mgmt:%d, port:%d, sys_capa:%d, sys_des:%d, sys_name:%d, no:%d", mgmt, port, sys_capa, sys_des, sys_name, no);

        vtss_ifindex_t ifindex;
        VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));

        // Get current configuration
        VTSS_RC(vtss_appl_lldp_port_conf_get(ifindex, &lldp_conf));


        if (mgmt) {
          if (no) {
            lldp_conf.optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_ADDR_BIT;
          } else {
            lldp_conf.optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_ADDR_BIT;
          }
        }
        if (port) {
          if (no) {
            lldp_conf.optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_PORT_DESCR_BIT;
          } else {
            lldp_conf.optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_PORT_DESCR_BIT;
          }
        }

        if (sys_capa) {
          if (no) {
            lldp_conf.optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_CAPA_BIT;
          } else {
            lldp_conf.optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_CAPA_BIT;
          }
        }

        if (sys_des) {
          if (no) {
            lldp_conf.optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_DESCR_BIT;
          } else {
            lldp_conf.optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_DESCR_BIT;
          }
        }

        if (sys_name) {
          if (no) {
            lldp_conf.optional_tlvs_mask &= ~VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_NAME_BIT;
          } else {
            lldp_conf.optional_tlvs_mask |= VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_NAME_BIT;
          }
        }

        // Set new configuration.
        ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_port_conf_set(ifindex, &lldp_conf));
      }

    }
  }
  return VTSS_RC_OK;
}

// Help function for setting LLDP mode
// IN plist - Port list with ports to configure.
//     tx - if we shall transmit LLDP frames.
//     rx - TRUE if we shall add LLDP information received from neighbors into the entry table.
//     no - TRUE if the no command is used
// Return - Vitesse return code
static mesa_rc lldp_icli_mode_set(icli_stack_port_range_t *plist, BOOL tx, BOOL rx, BOOL no)
{
  switch_iter_t  sit;
  port_iter_t    pit;
  vtss_appl_lldp_port_conf_t lldp_conf;

  VTSS_RC(icli_switch_iter_init(&sit));

  while (icli_switch_iter_getnext(&sit, plist)) {

    // Loop through ports.
    VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));


    while (icli_port_iter_getnext(&pit, plist)) {
      vtss_ifindex_t ifindex;
      VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));

      // Get current configuration
      VTSS_RC(vtss_appl_lldp_port_conf_get(ifindex, &lldp_conf));


      switch (lldp_conf.admin_states) {
      case VTSS_APPL_LLDP_DISABLED:
        if (tx && !no) {
          lldp_conf.admin_states = VTSS_APPL_LLDP_ENABLED_TX_ONLY;
        }

        if (rx && !no) {
          lldp_conf.admin_states = VTSS_APPL_LLDP_ENABLED_RX_ONLY;
        }
        break;
      case VTSS_APPL_LLDP_ENABLED_RX_ONLY:
        if (rx && no) {
          lldp_conf.admin_states = VTSS_APPL_LLDP_DISABLED;
        }

        if (tx && !no) {
          lldp_conf.admin_states = VTSS_APPL_LLDP_ENABLED_RX_TX;
        }
        break;
      case VTSS_APPL_LLDP_ENABLED_TX_ONLY:
        if (rx && !no) {
          lldp_conf.admin_states = VTSS_APPL_LLDP_ENABLED_RX_TX;
        }

        if (tx && no) {
          lldp_conf.admin_states = VTSS_APPL_LLDP_DISABLED;
        }
        break;

      case VTSS_APPL_LLDP_ENABLED_RX_TX:
        if (rx && no) {
          lldp_conf.admin_states = VTSS_APPL_LLDP_ENABLED_TX_ONLY;
        }

        if (tx && no) {
          lldp_conf.admin_states = VTSS_APPL_LLDP_ENABLED_RX_ONLY;
        }
        break;
      }
      // Set new configuration.
      VTSS_RC(vtss_appl_lldp_port_conf_set(ifindex, &lldp_conf));

    }

  }  // while icli_switch_iter_getnext
  return VTSS_RC_OK;
}

// See lldp_icli_functions.h
mesa_rc lldp_icli_mode(i32 session_id, icli_stack_port_range_t *plist, BOOL tx, BOOL rx, BOOL no)
{
  ICLI_RC_CHECK_PRINT_RC(lldp_icli_mode_set(plist, tx, rx, no));
  return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_CDP
// Help function for setting LLDP CDP awareness
// IN plist - Port list with ports to configure.
//     no - TRUE if the no command is used
// Return - Vitesse return code
mesa_rc lldp_icli_cdp_set(icli_stack_port_range_t *plist, BOOL no)
{
  port_iter_t    pit;
  CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_conf;

  // Get current configuration
  VTSS_RC(lldp_mgmt_conf_get(&lldp_conf[0]));

  // Loop through ports.
  VTSS_RC(icli_port_iter_init(&pit, VTSS_ISID_START, PORT_ITER_FLAGS_FRONT));

  while (icli_port_iter_getnext(&pit, plist)) {
    lldp_conf[pit.iport].cdp_aware = !no;
  }

  // Set new configuration.
  VTSS_RC(lldp_mgmt_conf_set(&lldp_conf[0]));

  return VTSS_RC_OK;
}

// See lldp_icli_functions.h
mesa_rc lldp_icli_cdp(i32 session_id, icli_stack_port_range_t *plist, BOOL no)
{
  ICLI_RC_CHECK_PRINT_RC(lldp_icli_cdp_set(plist, no));
  return VTSS_RC_OK;
}
#endif

#ifdef VTSS_SW_OPTION_SNMP
// Help function for setting LLDP SNMP trap
// IN plist - Port list with ports to configure.
//     no - TRUE if the no command is used
// Return - Vitesse return code
mesa_rc lldp_icli_trap_set(icli_stack_port_range_t *plist, BOOL no)
{
  switch_iter_t  sit;
  port_iter_t    pit;
  CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_conf;

  VTSS_RC(icli_switch_iter_init(&sit));

  while (icli_switch_iter_getnext(&sit, plist) && (sit.isid == VTSS_ISID_START)) {
    // Get current configuration
    VTSS_RC(lldp_mgmt_conf_get(&lldp_conf[0]));

    // Loop through ports.
    VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));

    while (icli_port_iter_getnext(&pit, plist)) {
      lldp_conf[pit.iport].snmp_notification_ena = !no;
    }

    // Set new configuration.
    VTSS_RC(lldp_mgmt_conf_set(&lldp_conf[0]));
  }
  return VTSS_RC_OK;
}

// See lldp_icli_functions.h
mesa_rc lldp_icli_trap(i32 session_id, icli_stack_port_range_t *plist, BOOL no)
{
  ICLI_RC_CHECK_PRINT_RC(lldp_icli_trap_set(plist, no));
  return VTSS_RC_OK;
}
#endif

//
// ICFG (Show running)
//
#ifdef VTSS_SW_OPTION_ICFG

// help function for printing the mode.
// IN : lldp_conf - Pointer to the configuration.
//      all_defaults - TRUE if we shall be printing everything (else we are only printing non-default configurations).
//      result - To be used by vtss_icfg_printf
static mesa_rc lldp_icfg_print_mode(const vtss_appl_lldp_port_conf_t lldp_conf, BOOL all_defaults, vtss_icfg_query_result_t *result)
{
  // Since we work with a single state containing both RX and TX we need to figure out if rx and tx is enabled.
  // The default value is different depending upon if LLDP-MED is included or not, so that has to be taking into account as well
  BOOL tx_en = FALSE;
  BOOL rx_en = FALSE;
  BOOL print_tx = TRUE;
  BOOL print_rx = TRUE;

  /*lint --e{506} ... yes, LLDP_ADMIN_STATE_DEFAULT is constant! */
  switch (lldp_conf.admin_states) {
  case VTSS_APPL_LLDP_ENABLED_RX_ONLY:
    rx_en = TRUE;
    if (LLDP_ADMIN_STATE_DEFAULT != VTSS_APPL_LLDP_DISABLED) {
      print_rx = FALSE;
    }

    break;
  case VTSS_APPL_LLDP_ENABLED_TX_ONLY:
    tx_en = TRUE;
    if (LLDP_ADMIN_STATE_DEFAULT != VTSS_APPL_LLDP_DISABLED) {
      print_tx = FALSE;
    }
    break;
  case VTSS_APPL_LLDP_ENABLED_RX_TX:
    tx_en = TRUE;
    rx_en = TRUE;
    if (LLDP_ADMIN_STATE_DEFAULT != VTSS_APPL_LLDP_DISABLED) {
      print_tx = FALSE;
      print_rx = FALSE;
    }
    break;
  case VTSS_APPL_LLDP_DISABLED:
    if (LLDP_ADMIN_STATE_DEFAULT == VTSS_APPL_LLDP_DISABLED) {
      print_tx = FALSE;
      print_rx = FALSE;
    }
    break;
  }

  if (all_defaults ||
      print_rx) {
    VTSS_RC(vtss_icfg_printf(result, " %slldp receive\n", rx_en ? "" : "no "));
  }

  if (all_defaults ||
      print_tx) {
    VTSS_RC(vtss_icfg_printf(result, " %slldp transmit\n", tx_en ? "" : "no "));
  }
  return VTSS_RC_OK;
}

// Help function for print global configuration.
// help function for printing the mode.
// IN : lldp_conf - Pointer to the configuration.
//      iport - Port in question
//      all_defaults - TRUE if we shall be printing everything (else we are only printing non-default configurations).
//      result - To be used by vtss_icfg_printf
static mesa_rc lldp_icfg_print_global(vtss_appl_lldp_common_conf_t *lldp_conf, mesa_port_no_t iport, BOOL all_defaults, vtss_icfg_query_result_t *result)
{
  // TX hold
  if (all_defaults ||
      (lldp_conf->tx_sm.msgTxHold != VTSS_APPL_LLDP_TX_HOLD_DEFAULT)) {
    VTSS_RC(vtss_icfg_printf(result, "lldp holdtime %d\n", lldp_conf->tx_sm.msgTxHold));
  }

  // TX delay
  if (all_defaults ||
      (lldp_conf->tx_sm.txDelay != VTSS_APPL_LLDP_TX_DELAY_DEFAULT)) {
    VTSS_RC(vtss_icfg_printf(result, "lldp transmission-delay %d\n", lldp_conf->tx_sm.txDelay));
  }

  // Timer
  if (all_defaults ||
      (lldp_conf->tx_sm.msgTxInterval != VTSS_APPL_LLDP_TX_INTERVAL_DEFAULT)) {

    VTSS_RC(vtss_icfg_printf(result, "lldp timer %d\n", lldp_conf->tx_sm.msgTxInterval));
  }

  // reinit delay
  if (all_defaults ||
      (lldp_conf->tx_sm.reInitDelay != VTSS_APPL_LLDP_REINIT_DEFAULT)) {
    VTSS_RC(vtss_icfg_printf(result, "lldp reinit %d\n", lldp_conf->tx_sm.reInitDelay));
  }

  return VTSS_RC_OK;
}

/* ICFG callback functions */
static mesa_rc lldp_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
  vtss_isid_t    isid;
  mesa_port_no_t iport;
  BOOL           optional_tlv_mgmt;
  BOOL           optional_tlv_port;
  BOOL           optional_tlv_sys_name;
  BOOL           optional_tlv_sys_des;
  BOOL           optional_tlv_sys_capa;

  switch (req->cmd_mode) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG:
    vtss_appl_lldp_common_conf_t lldp_conf;
    // Get current configuration for this switch
    VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));

    // Any port can be used
    VTSS_RC(lldp_icfg_print_global(&lldp_conf, 0, req->all_defaults, result));
    break;

  case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
    vtss_appl_lldp_port_conf_t lldp_port_conf;
    isid = req->instance_id.port.isid;
    iport = req->instance_id.port.begin_iport;
    T_D("Isid:%d, iport:%u, req->instance_id.port.usid:%d", isid, iport, req->instance_id.port.usid);

    if (msg_switch_configurable(isid)) {
      vtss_ifindex_t ifindex;
      VTSS_RC(vtss_ifindex_from_port(isid, iport, &ifindex));

      // Get current configuration for this switch
      VTSS_RC(vtss_appl_lldp_port_conf_get(ifindex, &lldp_port_conf));

      VTSS_RC(lldp_icfg_print_mode(lldp_port_conf, req->all_defaults, result));

      T_NG(TRACE_GRP_CLI, "optional_tlvs_mask:0x%X", lldp_port_conf.optional_tlvs_mask);

      optional_tlv_mgmt     = lldp_port_conf.optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_ADDR_BIT ? TRUE : FALSE;
      optional_tlv_port     = lldp_port_conf.optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_PORT_DESCR_BIT ? TRUE : FALSE;
      optional_tlv_sys_name = lldp_port_conf.optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_NAME_BIT ? TRUE : FALSE;
      optional_tlv_sys_des  = lldp_port_conf.optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_DESCR_BIT ? TRUE : FALSE;
      optional_tlv_sys_capa = lldp_port_conf.optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_CAPA_BIT ? TRUE : FALSE;

      if (req->all_defaults ||
          optional_tlv_mgmt == FALSE) {
        VTSS_RC(vtss_icfg_printf(result, " %slldp tlv-select management-address\n", optional_tlv_mgmt ? "" : "no "));
      }

      if (req->all_defaults ||
          optional_tlv_port == FALSE) {
        VTSS_RC(vtss_icfg_printf(result, " %slldp tlv-select port-description\n", optional_tlv_port ? "" : "no "));
      }

      if (req->all_defaults ||
          optional_tlv_sys_capa == FALSE) {
        VTSS_RC(vtss_icfg_printf(result, " %slldp tlv-select system-capabilities\n", optional_tlv_sys_capa ? "" : "no "));
      }

      if (req->all_defaults ||
          optional_tlv_sys_name == FALSE) {
        VTSS_RC(vtss_icfg_printf(result, " %slldp tlv-select system-name\n", optional_tlv_sys_name ? "" : "no "));
      }

      if (req->all_defaults ||
          optional_tlv_sys_des == FALSE) {
        VTSS_RC(vtss_icfg_printf(result, " %slldp tlv-select system-description\n", optional_tlv_sys_des ? "" : "no "));
      }

#ifdef VTSS_SW_OPTION_CDP
      // CDP Awareness
      if (req->all_defaults ||
          (lldp_port_conf.cdp_aware != LLDP_CDP_AWARE_DEFAULT)) {
        VTSS_RC(vtss_icfg_printf(result, " %slldp cdp-aware\n", lldp_port_conf.cdp_aware ? "" : "no "));
      }
#endif

#ifdef VTSS_SW_OPTION_SNMP
      // SNMP trap
      if (req->all_defaults ||
          (lldp_port_conf.snmp_notification_ena != FALSE)) {
        VTSS_RC(vtss_icfg_printf(result, " %slldp trap\n", lldp_port_conf.snmp_notification_ena ? "" : "no "));
      }
#endif
    }
    break;
  default:
    //Not needed for LLDP
    break;
  }
  return VTSS_RC_OK;
}

/* ICFG Initialization function */
mesa_rc lldp_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_LLDP_GLOBAL_CONF, "lldp", lldp_icfg_global_conf));
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_LLDP_PORT_CONF, "lldp", lldp_icfg_global_conf));
  return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG
#endif // #ifdef VTSS_SW_OPTION_LLDP

