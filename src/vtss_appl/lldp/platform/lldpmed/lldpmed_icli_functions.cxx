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

#ifdef VTSS_SW_OPTION_LLDP_MED

#include "icli_api.h"
#include "icli_porting_util.h"
#include "lldp_api.h"
#include "lldp_os.h"
#include "msg_api.h"
#include "misc_api.h" // for uport2iport
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif
#include "mgmt_api.h"
#include "lldpmed_rx.h"
#include "lldpmed_shared.h"
#include "lldp_icli_shared_functions.h" // For lldp_local_interface_txt_get
#include "lldpmed_icli_functions.h"
#include "lldp_trace.h"
#include "lldpmed_tx.h"

/***************************************************************************/
/*  Code start :)                                                           */
/****************************************************************************/

/***************************************************************************/
/*  Internal functions                                                     */
/****************************************************************************/

// Help function for setting optional TLVs
// Input parameters same as for function lldpmed_icli_transmit_tlv_set
// Return - Vitesse return code
static mesa_rc lldpmed_icli_transmit_tlv_set(icli_stack_port_range_t *plist, BOOL has_capabilities, BOOL has_location, BOOL has_network_policy, BOOL has_poe, BOOL no)
{
  switch_iter_t  sit;
  port_iter_t    pit;
  CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_conf;

  VTSS_RC(icli_switch_iter_init(&sit));

  while (icli_switch_iter_getnext(&sit, plist) && (sit.isid == VTSS_ISID_START)) {
    // get current configuration
    VTSS_RC(lldp_mgmt_conf_get(&lldp_conf[0]));

    // Loop through ports.
    VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
    while (icli_port_iter_getnext(&pit, plist)) {

      if (has_capabilities) {
        if (no) {
          // Clear the capabilities bit.
          lldp_conf[pit.iport].lldpmed_optional_tlvs_mask &= ~VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT;
        } else {
          // Set the capabilities bit.
          lldp_conf[pit.iport].lldpmed_optional_tlvs_mask |= VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT;
        }
      }

      if (has_network_policy) {
        if (no) {
          // Clear the network-policy bit.
          lldp_conf[pit.iport].lldpmed_optional_tlvs_mask &= ~VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT;
        } else {
          // Set the network-policy bit.
          lldp_conf[pit.iport].lldpmed_optional_tlvs_mask |= VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT;
        }
      }

      if (has_location) {
        if (no) {
          // Clear the location bit.
          lldp_conf[pit.iport].lldpmed_optional_tlvs_mask &= ~VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT;
        } else {
          // Set the location bit.
          lldp_conf[pit.iport].lldpmed_optional_tlvs_mask |= VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT;
        }
      }

      if (has_poe) {
        if (no) {
          // Clear the location bit.
          lldp_conf[pit.iport].lldpmed_optional_tlvs_mask &= ~VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT;
        } else {
          // Set the location bit.
          lldp_conf[pit.iport].lldpmed_optional_tlvs_mask |= VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT;
        }
      }
      T_NG_PORT(TRACE_GRP_CLI, pit.iport, "Mask:0x%X, has_poe:%d", lldp_conf[pit.iport].lldpmed_optional_tlvs_mask, has_poe);
    }

    VTSS_RC(lldp_mgmt_conf_set(&lldp_conf[0]));
  }  // while icli_switch_iter_getnext
  return VTSS_RC_OK;
}

// Help function for printing neighbor inventory list
// In : session_id - Session_Id for ICLI_PRINTF
//      entry      - The information from the remote neighbor
static void icli_cmd_lldpmed_print_inventory(i32 session_id, vtss_appl_lldp_remote_entry_t *entry)
{
  lldp_8_t inventory_str[VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX];


  if (entry->lldpmed_hw_rev_length            > 0 ||
      entry->lldpmed_firm_rev_length          > 0 ||
      entry->lldpmed_sw_rev_length            > 0 ||
      entry->lldpmed_serial_no_length         > 0 ||
      entry->lldpmed_manufacturer_name_length > 0 ||
      entry->lldpmed_model_name_length        > 0 ||
      entry->lldpmed_asset_id_length          > 0) {
    ICLI_PRINTF("\nInventory \n");

    ICLI_PRINTF("%-20s: %s \n", "Hardware Revision", vtss_appl_lldp_med_invertory_info_get(entry, LLDPMED_HW_REV, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &inventory_str[0]));
    ICLI_PRINTF("%-20s: %s \n", "Firmware Revision", vtss_appl_lldp_med_invertory_info_get(entry, LLDPMED_FW_REV, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &inventory_str[0]));
    ICLI_PRINTF("%-20s: %s \n", "Software Revision", vtss_appl_lldp_med_invertory_info_get(entry, LLDPMED_SW_REV, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &inventory_str[0]));
    ICLI_PRINTF("%-20s: %s \n", "Serial Number",     vtss_appl_lldp_med_invertory_info_get(entry, LLDPMED_SER_NUM, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &inventory_str[0]));
    ICLI_PRINTF("%-20s: %s \n", "Manufacturer Name", vtss_appl_lldp_med_invertory_info_get(entry, LLDPMED_MANUFACTURER_NAME, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &inventory_str[0]));
    ICLI_PRINTF("%-20s: %s \n", "Model Name",        vtss_appl_lldp_med_invertory_info_get(entry, LLDPMED_MODEL_NAME, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &inventory_str[0]));
    ICLI_PRINTF("%-20s: %s \n", "Asset ID",          vtss_appl_lldp_med_invertory_info_get(entry, LLDPMED_ASSET_ID, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &inventory_str[0]));
  }
}

// Help function for printing the LLDP-MED neighbor information in an entry.
//
// In : session_id    - Session_Id for ICLI_PRINTF
//      sit           - Pointer to switch information
//      has_interface - TRUE if the user has specified a specific interface to show
//      plist         - list containing information about which ports to show the information for.
static void icli_cmd_lldpmed_print_info(i32 session_id, switch_iter_t *sit, BOOL has_interface, icli_stack_port_range_t *plist)
{
  lldp_8_t   buf[1000];
  lldp_u8_t  p_index;
  port_iter_t    pit;
  u32 i;
  vtss_appl_lldp_remote_entry_t *table_m, *table_p = NULL, *entry = NULL;
  BOOL lldpmed_no_entry_found = TRUE;

  vtss_appl_lldp_cap_t cap;
  if (vtss_appl_lldp_cap_get(&cap) != VTSS_RC_OK) {
    T_E("Could not get capabilities");
    return;
  }

  if (!msg_switch_exists(sit->isid) || (sit->isid != VTSS_ISID_START)) {
    return;
  }

  T_IG(TRACE_GRP_CLI, "****** vtss_appl_lldp_remote_entry_t size:" VPRIz, sizeof(vtss_appl_lldp_remote_entry_t));

  // Loop through all front ports
  if (icli_port_iter_init(&pit, sit->isid, PORT_ITER_SORT_ORDER_IPORT | PORT_ITER_FLAGS_FRONT) == VTSS_RC_OK) {
    // Because the iCLI can be stopped in the middle of the printout (waiting for user input to continue),
    // we need to make a copy of the entry table in order not to block for access to the entry table (and holding the
    // semaphore too long, with the lldp_mgmt_get_lock function.
    if ((VTSS_MALLOC_CAST(table_m, LLDP_ENTRIES_TABLE_SIZE)) == NULL) {
      T_E("Error trying to malloc");
      return;
    }

    vtss_appl_lldp_mutex_lock();
    table_p = vtss_appl_lldp_entries_get(); // Get the LLDP entries for the switch in question.
    memcpy(table_m, table_p, LLDP_ENTRIES_TABLE_SIZE);
    vtss_appl_lldp_mutex_unlock();
    T_IG(TRACE_GRP_CLI, "has_interface:%d", has_interface);

    while (icli_port_iter_getnext(&pit, plist)) {
      for (i = 0, entry = table_m; i < cap.remote_entries_cnt; i++, entry++) {
        if (entry->in_use == 0 || !entry->lldpmed_info_vld || (entry->receive_port != pit.iport)) {
          T_RG_PORT(TRACE_GRP_CLI, pit.iport, "in_use:%d, lldpmed_info_vld:%d, receive_port:%d",
                    entry->in_use, entry->lldpmed_info_vld, entry->receive_port);
          continue;
        }
        lldpmed_no_entry_found = FALSE;

        T_NG_PORT(TRACE_GRP_CLI, entry->receive_port, "size = %zu, lldpmed_info_vld = %d", sizeof(buf), entry->lldpmed_info_vld);

        if (entry->lldpmed_info_vld) {
          ICLI_PRINTF("%-20s: %s\n", "Local Interface", lldp_local_interface_txt_get(buf, entry, sit, &pit));

          // Device type / capabilities
          if (entry->lldpmed_capabilities_vld) {
            lldpmed_device_type2str(entry, buf);
            ICLI_PRINTF("%-20s: %s \n", "Device Type", buf);

            lldpmed_capabilities2str(entry, buf);
            ICLI_PRINTF("%-20s: %s \n", "Capabilities", buf);
          }

          // Loop through policies
          for (p_index = 0; p_index < VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT; p_index ++) {
            // make sure that policy exist.
            if (!entry->policy[p_index].in_use) {
              T_NG(TRACE_GRP_CLI, "Continue");
              continue;
            }

            T_IG(TRACE_GRP_CLI, "p_index:%d, entry:%d", p_index, i);

            // Policies
            ICLI_PRINTF("\n%-20s: %s \n", "Application Type",
                        lldpmed_appl_type2str(entry->policy[p_index].network_policy.application_type, buf));

            ICLI_PRINTF("%-20s: %s \n", "Policy",
                        lldpmed_policy_flag_type2str(entry->policy[p_index].network_policy.unknown_policy_flag));

            ICLI_PRINTF("%-20s: %s \n", "Tag",
                        lldpmed_policy_tag2str(entry->policy[p_index].network_policy.tagged_flag));

            ICLI_PRINTF("%-20s: %s \n", "VLAN ID",
                        lldpmed_policy_vlan_id2str(entry->policy[p_index], buf));

            ICLI_PRINTF("%-20s: %s \n", "Priority",
                        lldpmed_policy_prio2str(entry->policy[p_index], buf));

            ICLI_PRINTF("%-20s: %s \n", "DSCP",
                        lldpmed_policy_dscp2str(entry->policy[p_index], buf));
          }

          if (entry->lldpmed_coordinate_location_vld) {
            lldpmed_location2str(entry, buf, LLDPMED_LOCATION_COORDINATE);
            ICLI_PRINTF("%-20s: %s\n", "Coordinate Location", buf);
          }

          if (entry->lldpmed_civic_location_vld) {
            lldpmed_location2str(entry, buf, LLDPMED_LOCATION_CIVIC);
            ICLI_PRINTF("%-20s: %s\n", "Civic Location", buf);
          }

          if (entry->lldpmed_elin_location_vld) {
            lldpmed_location2str(entry, buf, LLDPMED_LOCATION_ECS);
            ICLI_PRINTF("%-20s: %s\n", "ECS Location", buf);
          }

          icli_cmd_lldpmed_print_inventory(session_id, entry);

          ICLI_PRINTF("\n");
        }
      }
    }

    VTSS_FREE(table_m);

    T_DG(TRACE_GRP_CLI, "lldpmed_no_entry_found:%d", lldpmed_no_entry_found);
    if (lldpmed_no_entry_found) {
      ICLI_PRINTF("No LLDP-MED entries found\n");
    }
  }
}

// See lldpmed_icli_functions.h
void lldpmed_icli_show_remote_device(i32 session_id, BOOL has_interface, icli_stack_port_range_t *plist)
{
  switch_iter_t sit;

  // Loop all the switches in question.
  if (icli_switch_iter_init(&sit) == VTSS_RC_OK) {
    while (icli_switch_iter_getnext(&sit, plist)) {
      icli_cmd_lldpmed_print_info(session_id, &sit, has_interface, plist);
    }
  }
}

// Help function for printing the policies list.
// In : Session_Id - session_id for ICLI_PRINTF
//      conf       - Current configuration
//      policy_index - Index for the policy to print.
static void lldpmed_icli_print_policies(i32 session_id, u32 policy_index)
{
  char application_type_str[50];
  char vlanbuf[10];
  char l2priobuf[15];
  char dscpbuf[10];

  vtss_appl_lldp_med_policy_t policy;
  (void) vtss_appl_lldp_conf_policy_get(policy_index, &policy);

  if (policy.in_use) {
    ICLI_PRINTF("%-10d %-25s %-8s %-8s %-12s %-8s \n",
                policy_index,
                lldpmed_appl_type2str(policy.network_policy.application_type, &application_type_str[0]),
                policy.network_policy.tagged_flag ? "Tagged" : "Untagged",
                lldpmed_policy_vlan_id2str(policy, vlanbuf),
                lldpmed_policy_prio2str(policy, l2priobuf),
                lldpmed_policy_dscp2str(policy, dscpbuf));
  }
}

// Help function for assigning LLDP policies
// Input parameters same as lldpmed_icli_assign_policy function
// return - Vitesse return code
static mesa_rc lldpmed_icli_assign_policy_set(i32 session_id, icli_stack_port_range_t *plist, icli_range_t *policies_list, BOOL no)
{
  switch_iter_t  sit;
  port_iter_t    pit;
  CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_port_conf;
  vtss_appl_lldp_common_conf_t lldp_conf;

  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));
  VTSS_RC(icli_switch_iter_init(&sit));
  while (icli_switch_iter_getnext(&sit, plist) && (sit.isid == VTSS_ISID_START)) {
    // Get current configuration
    VTSS_RC(lldp_mgmt_conf_get(&lldp_port_conf[0]));

    // Loop through ports.
    VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
    while (icli_port_iter_getnext(&pit, plist)) {
      u32 p_index;
      i32 range_index;

      vtss_ifindex_t ifindex;
      VTSS_RC(vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex));

      // If user doesn't select some specific policies then remove them all (This only should only happen for the "no" command
      if (policies_list == NULL) {
        if (!no) {
          T_E("policies list is NULL. Should only happen for the no command");
        }
        // Remove all policies from this interface port.
        for (range_index = 0; range_index < LLDPMED_POLICIES_CNT; range_index++) {
          ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_conf_port_policy_set(ifindex, range_index, FALSE));
        }
      } else {
        for (p_index = 0; p_index < policies_list->u.sr.cnt; p_index++) {
          for (range_index = policies_list->u.sr.range[p_index].min; range_index <= policies_list->u.sr.range[p_index].max; range_index++) {
            vtss_appl_lldp_med_policy_t policy;
            // Make sure we don't get out of bounce
            if (range_index > LLDPMED_POLICY_MAX) {
              ICLI_PRINTF("%% Ignoring invalid policy:%d. Valid range is %u-%u\n", range_index, LLDPMED_POLICY_MIN, LLDPMED_POLICY_MAX);
              continue;
            }
            (void) vtss_appl_lldp_conf_policy_get(range_index, &policy);

            if (no) {
              ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_conf_port_policy_set(ifindex, range_index, FALSE));
            } else {
              mesa_rc rc = vtss_appl_lldp_conf_port_policy_set(ifindex, range_index, TRUE);
              char buf[30];
              if (rc == VTSS_APPL_LLDP_ERROR_POLICY_NOT_DEFINED) {
                ICLI_PRINTF("Ignoring policy %d for %s, because no such policy is defined\n",
                            range_index, icli_port_info_txt(sit.usid, pit.uport, buf));
              } else if (rc != VTSS_RC_OK) {
                ICLI_RC_CHECK_PRINT_RC(rc);
              }
            }
          }
        }
      }
    }

    ICLI_RC_CHECK_PRINT_RC(lldp_mgmt_conf_set(&lldp_port_conf[0]));
  }  // while icli_switch_iter_getnext

  return VTSS_RC_OK;
}
#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
mesa_rc lldpmed_icli_type_private(i32 session_id, icli_stack_port_range_t *plist, BOOL has_connectivity, BOOL has_end_point, BOOL no)
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
      if (no) {
        lldp_conf[pit.iport].lldpmed_device_type = LLDPMED_DEVICE_TYPE_DEFAULT;
      } else {
        if (has_end_point) {
          lldp_conf[pit.iport].lldpmed_device_type = VTSS_APPL_LLDP_MED_END_POINT;
        }

        if (has_connectivity) {
          lldp_conf[pit.iport].lldpmed_device_type = VTSS_APPL_LLDP_MED_CONNECTIVITY;
        }
      }
    }
    VTSS_RC(lldp_mgmt_conf_set(&lldp_conf[0]));
  }
  return VTSS_RC_OK;
}
#endif
/***************************************************************************/
/*  Functions called from iCLI                                             */
/****************************************************************************/
// See lldpmed_icli_functions.h
#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
mesa_rc lldpmed_icli_type(i32 session_id, icli_stack_port_range_t *plist, BOOL has_connectivity, BOOL has_end_point, BOOL no)
{
  ICLI_RC_CHECK_PRINT_RC(lldpmed_icli_type_private(session_id, plist, has_connectivity, has_end_point, no));
  return VTSS_RC_OK;
}
#endif

// In reality lldpmed_icli_civic_addr, but needed for being able to print the return code
mesa_rc _lldpmed_icli_civic_addr(i32 session_id, BOOL has_country, BOOL has_state, BOOL has_county, BOOL has_city, BOOL has_district, BOOL has_block, BOOL has_street, BOOL has_leading_street_direction, BOOL has_trailing_street_suffix, BOOL has_str_suf, BOOL has_house_no, BOOL has_house_no_suffix, BOOL has_landmark, BOOL has_additional_info, BOOL has_name, BOOL has_zip_code, BOOL has_building, BOOL has_apartment, BOOL has_floor, BOOL has_room_number, BOOL has_place_type, BOOL has_postal_com_name, BOOL has_p_o_box, BOOL has_additional_code, const char *v_string250)
{
  vtss_appl_lldp_common_conf_t lldp_conf;

  if (v_string250 == NULL) {
    T_E("Sting is NULL - Should never happen");
    return VTSS_RC_ERROR;
  }

  T_IG(TRACE_GRP_CLI, "has_state:%d", has_state);

  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));

  if (has_country) {
    T_RG(TRACE_GRP_CLI, "strlen(v_string250):" VPRIz", v_string250:%s", strlen(v_string250), v_string250);
    if (strlen(v_string250) != (VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN - 1) && strlen(v_string250) != 0) { // Minus one because VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN has space for the \0. Zero length for the no command.
      ICLI_RC_CHECK_PRINT_RC(VTSS_APPL_LLDP_ERROR_COUNTRY_CODE_LETTER_SIZE);
    }
    misc_strncpyz(lldp_conf.ca_country_code, v_string250, VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN);
  }

  if (has_state) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_A1, v_string250));
  }

  if (has_county) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_A2, v_string250));
  }

  if (has_city) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_A3, v_string250));
  }

  if (has_district) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_A4, v_string250));
  }

  if (has_block) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_A5, v_string250));
  }

  if (has_street) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_A6, v_string250));
  }

  if (has_leading_street_direction) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_PRD, v_string250));
  }

  if (has_trailing_street_suffix) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_POD, v_string250));
  }

  if (has_str_suf) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_STS, v_string250));
  }

  if (has_house_no_suffix) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_HNS, v_string250));
  }

  if (has_house_no) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_HNO, v_string250));
  }

  if (has_landmark) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_LMK, v_string250));
  }

  if (has_additional_info) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_LOC, v_string250));
  }

  if (has_name) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_NAM, v_string250));
  }

  T_N("has_zip_code:%d - %s", has_zip_code, v_string250);
  if (has_zip_code) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_ZIP, v_string250));
  }

  if (has_building) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_BUILD, v_string250));
  }

  if (has_apartment) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_UNIT, v_string250));
  }

  if (has_floor) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_FLR, v_string250));
  }

  if (has_room_number) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_ROOM, v_string250));
  }

  if (has_place_type) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_PLACE, v_string250));
  }

  if (has_postal_com_name) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_PCN, v_string250));
  }

  if (has_p_o_box) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_POBOX, v_string250));
  }

  if (has_additional_code) {
    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_conf.civic, LLDPMED_CATYPE_ADD_CODE, v_string250));
  }

  VTSS_RC(vtss_appl_lldp_common_conf_set(&lldp_conf));

  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_civic_addr(i32 session_id, BOOL has_country, BOOL has_state, BOOL has_county, BOOL has_city, BOOL has_district, BOOL has_block, BOOL has_street, BOOL has_leading_street_direction, BOOL has_trailing_street_suffix, BOOL has_str_suf, BOOL has_house_no, BOOL has_house_no_suffix, BOOL has_landmark, BOOL has_additional_info, BOOL has_name, BOOL has_zip_code, BOOL has_building, BOOL has_apartment, BOOL has_floor, BOOL has_room_number, BOOL has_place_type, BOOL has_postal_com_name, BOOL has_p_o_box, BOOL has_additional_code, const char *v_string250)
{
  ICLI_RC_CHECK_PRINT_RC(_lldpmed_icli_civic_addr(session_id, has_country, has_state, has_county, has_city, has_district, has_block, has_street, has_leading_street_direction, has_trailing_street_suffix, has_str_suf, has_house_no, has_house_no_suffix, has_landmark, has_additional_info, has_name, has_zip_code, has_building, has_apartment, has_floor, has_room_number, has_place_type, has_postal_com_name, has_p_o_box, has_additional_code, v_string250));
  return VTSS_RC_OK;
}


// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_elin_addr(i32 session_id, const char *elin_string)
{
  vtss_appl_lldp_common_conf_t lldp_conf;

  if (elin_string == NULL) {
    T_E("elin_string is NULL. Shall never happen");
    return VTSS_RC_ERROR;
  }

  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf)); // Get current configuration

  misc_strncpyz(&lldp_conf.elin_location[0], elin_string, VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX + 1); // Update the elin parameter. Plus 1 for the "\0"

  ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_common_conf_set(&lldp_conf));
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_show_policies(i32 session_id, icli_unsigned_range_t *policies_list)
{
  lldp_16_t policy_index;
  u32 i, idx;
  BOOL at_least_one_policy_defined = FALSE;

  // Find out if any policy is defined.
  for (policy_index = LLDPMED_POLICY_MIN ; policy_index <= LLDPMED_POLICY_MAX; policy_index++) {
    vtss_appl_lldp_med_policy_t policy;
    VTSS_RC(vtss_appl_lldp_conf_policy_get(policy_index, &policy));
    if (policy.in_use) {
      at_least_one_policy_defined = TRUE;
      break;
    }
  }

  // Print out "header"
  if (at_least_one_policy_defined) {
    ICLI_PRINTF("%-10s %-25s %-8s %-8s %-12s %-8s \n", "Policy Id", "Application Type", "Tag", "Vlan ID", "L2 Priority", "DSCP");
  } else {
    ICLI_PRINTF("No policies defined\n");
    return VTSS_RC_OK;
  }

  // Print all policies that are currently in use.
  if (policies_list != NULL) {
    // User want some specific policies
    T_IG(TRACE_GRP_CLI, "cnt:%u", policies_list->cnt);
    for ( i = 0; i < policies_list->cnt; i++ ) {
      T_IG(TRACE_GRP_CLI, "(%u, %u) ", policies_list->range[i].min, policies_list->range[i].max);
      for ( idx = policies_list->range[i].min; idx <= policies_list->range[i].max; idx++ ) {
        lldpmed_icli_print_policies(session_id, idx);
      }
    }
  } else {
    // User want all policies
    for (policy_index = LLDPMED_POLICY_MIN ; policy_index <= LLDPMED_POLICY_MAX; policy_index++) {
      lldpmed_icli_print_policies(session_id, policy_index);
    }
  }
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_latitude(i32 session_id, BOOL north, BOOL south, char *degree)
{
  vtss_appl_lldp_common_conf_t lldp_conf;
  long value = 0;
  // get current configuration
  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));

  // convert floating point "string value" to long
  T_IG(TRACE_GRP_CLI, "Degree:%s", degree);
  if (mgmt_str_float2long(degree, &value, LLDPMED_LATITUDE_VALUE_MIN, LLDPMED_LATITUDE_VALUE_MAX, TUDE_DIGIT) != VTSS_RC_OK) {
    ICLI_PRINTF("Degree is not valid. Must be in the range 0.0000 to 90.0000\n");
    return VTSS_APPL_LLDP_ERROR_LATITUDE_OUT_OF_RANGE;
  }

  lldpmed_cal_fraction(value, south, 25, &lldp_conf.coordinate_location.latitude, 4, LLDPMED_LATITUDE_BIT_MASK);

  T_IG(TRACE_GRP_CLI, "value:%ld, latitude:0x" VPRI64x,
       value, lldp_conf.coordinate_location.latitude);

  //Set current configuration
  ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_common_conf_set(&lldp_conf));
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_longitude(i32 session_id, BOOL east, BOOL west, char *degree)
{
  vtss_appl_lldp_common_conf_t lldp_conf;
  long value = 0;

  // get current configuration
  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));


  // convert floating point "string value" to long
  T_IG(TRACE_GRP_CLI, "Degree:%s", degree);
  if (mgmt_str_float2long(degree, &value, LLDPMED_LONGITUDE_VALUE_MIN, LLDPMED_LONGITUDE_VALUE_MAX, TUDE_DIGIT) != VTSS_RC_OK) {
    ICLI_PRINTF("Degree is not valid. Must be in the range 0.0000 to 180.0000\n");
    return VTSS_APPL_LLDP_ERROR_LONGITUDE_OUT_OF_RANGE;
  }

  lldpmed_cal_fraction(value, west, 25, &lldp_conf.coordinate_location.longitude, 4, LLDPMED_LONGITUDE_BIT_MASK);

  T_DG(TRACE_GRP_CLI, "value:%ld, longitude:0x" VPRI64x,
       value, lldp_conf.coordinate_location.longitude);

  //Set current configuration
  ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_common_conf_set(&lldp_conf));
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_altitude(i32 session_id, BOOL meters, BOOL floors, char *value_str)
{
  vtss_appl_lldp_common_conf_t lldp_conf;
  long value = 0;
  // get current configuration
  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));


  // convert floating point "string value" to long
  if (mgmt_str_float2long(value_str, &value, (long)LLDPMED_ALTITUDE_VALUE_MIN, (ulong)LLDPMED_ALTITUDE_VALUE_MAX, ALTITUDE_DIGIT) != VTSS_RC_OK) {
    ICLI_PRINTF("Altitude is not valid. Must be in the range %.1f to %.1f\n", LLDPMED_ALTITUDE_VALUE_MIN_FLOAT, LLDPMED_ALTITUDE_VALUE_MAX_FLOAT);
    return VTSS_APPL_LLDP_ERROR_ALTITUDE_OUT_OF_RANGE;
  }

  if (meters) {
    lldp_conf.coordinate_location.altitude_type = METERS;
  }

  if (floors) {
    lldp_conf.coordinate_location.altitude_type = FLOOR;
  }

  i64 altitude_i64;

  lldpmed_cal_fraction(value, (value < 0 ? TRUE : FALSE), 8, &altitude_i64, ALTITUDE_DIGIT, LLDPMED_ALTITUDE_BIT_MASK);

  T_DG(TRACE_GRP_CLI, "altitude_i64:0x" VPRI64x", value:%ld", altitude_i64, value);

  lldp_conf.coordinate_location.altitude = (i32) altitude_i64;

  //Set current configuration
  ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_common_conf_set(&lldp_conf));
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_datum(i32 session_id, BOOL has_wgs84, BOOL has_nad83_navd88, BOOL has_nad83_mllw, BOOL no)
{
  // get current configuration
  vtss_appl_lldp_common_conf_t lldp_conf;
  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));

  if (no) {
    lldp_conf.coordinate_location.datum = LLDPMED_DATUM_DEFAULT;
  } else if (has_wgs84) {
    lldp_conf.coordinate_location.datum = WGS84;
  } else if (has_nad83_navd88) {
    lldp_conf.coordinate_location.datum = NAD83_NAVD88;
  } else if (has_nad83_mllw) {
    lldp_conf.coordinate_location.datum = NAD83_MLLW;
  }

  //Set current configuration
  ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_common_conf_set(&lldp_conf));
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_fast_start(i32 session_id, u32 value, BOOL no)
{
  vtss_appl_lldp_common_conf_t lldp_conf;
  VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));

  if (no) {
    lldp_conf.medFastStartRepeatCount = VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_DEFAULT;
  } else {
    lldp_conf.medFastStartRepeatCount = value;
  }

  //Set current configuration
  ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_common_conf_set(&lldp_conf));
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_assign_policy(i32 session_id, icli_stack_port_range_t *plist, icli_range_t *policies_list, BOOL no)
{
  ICLI_RC_CHECK_PRINT_RC(lldpmed_icli_assign_policy_set(session_id, plist, policies_list, no));
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_media_vlan_policy(i32 session_id, u32 policy_index, BOOL has_voice, BOOL has_voice_signaling, BOOL has_guest_voice_signaling,
                                       BOOL has_guest_voice, BOOL has_softphone_voice, BOOL has_video_conferencing, BOOL has_streaming_video,
                                       BOOL has_video_signaling, BOOL has_tagged, BOOL has_untagged, u32 v_vlan_id, u32 v_0_to_7, u32 v_0_to_63)
{
  BOOL update_conf = TRUE;

  vtss_appl_lldp_med_policy_t policy;
  VTSS_RC(vtss_appl_lldp_conf_policy_get(policy_index, &policy));
  policy.in_use = TRUE;
  if (has_voice) {
    policy.network_policy.application_type = VOICE;
  } else if (has_voice_signaling) {
    policy.network_policy.application_type = VOICE_SIGNALING;
  } else if (has_guest_voice) {
    policy.network_policy.application_type = GUEST_VOICE;
  } else if (has_guest_voice_signaling) {
    policy.network_policy.application_type = GUEST_VOICE_SIGNALING;
  } else if (has_softphone_voice) {
    policy.network_policy.application_type = SOFTPHONE_VOICE;
  } else if (has_video_conferencing) {
    policy.network_policy.application_type = VIDEO_CONFERENCING;
  } else if (has_streaming_video) {
    policy.network_policy.application_type = STREAMING_VIDEO;
  } else if (has_video_signaling) {
    policy.network_policy.application_type = VIDEO_SIGNALING;
  }

  policy.network_policy.tagged_flag = has_tagged;


  T_IG(TRACE_GRP_CLI, "vlan_id:%u has_tagged:%d, policy.network_policy.vlan_id:%d",
       v_vlan_id, has_tagged, policy.network_policy.vlan_id);

  if (has_tagged) { // If not tagged then ignore VLAN id and l2 priority according to TIA1057, section 10.2.3.2
    policy.network_policy.vlan_id = v_vlan_id;
    policy.network_policy.l2_priority = v_0_to_7;
  }

  policy.network_policy.dscp_value = v_0_to_63;

  if (update_conf) {
    //Set current configuration
    ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_conf_policy_set(policy_index, policy));
  }
  return VTSS_RC_OK;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_media_vlan_policy_delete(i32 session_id, icli_unsigned_range_t *policies_list)
{
  u32 p_index;

  if (policies_list == NULL) {
    T_E("policies_list should never be NULL at this point");
    return VTSS_RC_ERROR;
  }

  //Loop through all the policies
  for (p_index = 0; p_index < policies_list->cnt; p_index++) {
    u32 policy_index;
    for (policy_index = policies_list->range[p_index].min; policy_index <= policies_list->range[p_index].max; policy_index++) {
      vtss_appl_lldp_med_policy_t policy;
      VTSS_RC(vtss_appl_lldp_conf_policy_get(policy_index, &policy));

      policy.in_use = FALSE;
      ICLI_RC_CHECK_PRINT_RC(vtss_appl_lldp_conf_policy_set(policy_index, policy));
    }
  }

  return VTSS_RC_OK;
}

// Runtime function for ICLI that determines PoE is supported
BOOL lldp_icli_runtime_poe(u32                session_id,
                           icli_runtime_ask_t ask,
                           icli_runtime_t     *runtime)
{
  switch (ask) {
  case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_POE)
    runtime->present = TRUE;
#else
    runtime->present = FALSE;
#endif
    return TRUE;
  case ICLI_ASK_HELP:
    icli_sprintf(runtime->help, "Enable/Disable transmission of the optional PoE TLV.");
    return TRUE;
  default:
    break;
  }
  return FALSE;
}

// See lldpmed_icli_functions.h
mesa_rc lldpmed_icli_transmit_tlv(i32 session_id, icli_stack_port_range_t *plist, BOOL has_capabilities, BOOL has_location, BOOL has_network_policy, BOOL has_poe,  BOOL no)
{
  // If the user hasn't given any parameters as input we interpret that as he wan them all.
  if (!has_location && !has_poe && !has_network_policy && !has_capabilities) {
    has_location       = TRUE;
    has_poe            = TRUE;
    has_network_policy = TRUE;
    has_capabilities   = TRUE;
  }

  ICLI_RC_CHECK_PRINT_RC(lldpmed_icli_transmit_tlv_set(plist, has_capabilities, has_location, has_network_policy, has_poe, no));
  return VTSS_RC_OK;
}

//
// ICFG (Show running)
//
#ifdef VTSS_SW_OPTION_ICFG

// help function for getting Ca type as printable keyword
//
// In : ca_type - CA TYPE as integer
//
// In/Out: key_word - Pointer to the string.
//
static void lldpmed_catype2keyword(vtss_appl_lldp_med_catype_t ca_type, char *key_word)
{
  // Table in ANNEX B, TIA1057
  strcpy(key_word, "");
  switch (ca_type) {
  case LLDPMED_CATYPE_A1:
    strcat(key_word, "state");
    break;
  case LLDPMED_CATYPE_A2:
    strcat(key_word, "county");
    break;
  case LLDPMED_CATYPE_A3:
    strcat(key_word, "city");
    break;
  case LLDPMED_CATYPE_A4:
    strcat(key_word, "district");
    break;
  case LLDPMED_CATYPE_A5:
    strcat(key_word, "block");
    break;
  case LLDPMED_CATYPE_A6:
    strcat(key_word, "street");
    break;
  case LLDPMED_CATYPE_PRD:
    strcat(key_word, "leading-street-direction");
    break;
  case LLDPMED_CATYPE_POD:
    strcat(key_word, "trailing-street-suffix");
    break;
  case LLDPMED_CATYPE_STS:
    strcat(key_word, "street-suffix");
    break;
  case LLDPMED_CATYPE_HNO:
    strcat(key_word, "house-no");
    break;
  case LLDPMED_CATYPE_HNS:
    strcat(key_word, "house-no-suffix");
    break;
  case LLDPMED_CATYPE_LMK:
    strcat(key_word, "landmark");
    break;
  case LLDPMED_CATYPE_LOC:
    strcat(key_word, "additional-info");
    break;
  case LLDPMED_CATYPE_NAM:
    strcat(key_word, "name");
    break;
  case LLDPMED_CATYPE_ZIP:
    strcat(key_word, "zip-code");
    break;
  case LLDPMED_CATYPE_BUILD:
    strcat(key_word, "building");
    break;
  case LLDPMED_CATYPE_UNIT:
    strcat(key_word, "apartment");
    break;
  case LLDPMED_CATYPE_FLR:
    strcat(key_word, "floor");
    break;
  case LLDPMED_CATYPE_ROOM:
    strcat(key_word, "room-number");
    break;
  case LLDPMED_CATYPE_PLACE:
    strcat(key_word, "place-type");
    break;
  case LLDPMED_CATYPE_PCN:
    strcat(key_word, "postal-community-name");
    break;
  case LLDPMED_CATYPE_POBOX:
    strcat(key_word, "p-o-box");
    break;
  case LLDPMED_CATYPE_ADD_CODE:
    strcat(key_word, "additional-code");
    break;
  default:
    break;
  }
}

// Help function for printing the civic address
//
// In : all_defaults - TRUE if user want include printing of default values.
//      ca_type      - CA type to print
//      lldp_conf    - Pointer to LLDP configuration.
// In/Out: result - Pointer for ICFG
static mesa_rc lldpmed_icfg_print_civic(BOOL all_defaults, vtss_appl_lldp_med_catype_t ca_type, vtss_icfg_query_result_t *result, vtss_appl_lldp_common_conf_t *lldp_conf)
{
  lldp_8_t key_word_str[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];
  BOOL civic_empty;

  char location_str[VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX];
  if (vtss_appl_lldp_location_civic_info_get(&lldp_conf->civic, ca_type, VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX, &location_str[0]) == NULL) {
    return VTSS_APPL_LLDP_ERROR_CIVIC_TYPE;
  };

  civic_empty = (strlen(&location_str[0]) == 0);


  if (all_defaults ||
      (!civic_empty)) {

    lldpmed_catype2keyword(ca_type, &key_word_str[0]); // Converting ca type to a printable string

    VTSS_RC(vtss_icfg_printf(result, "%slldp med location-tlv civic-addr %s %s\n",
                             civic_empty ? "no " : "",
                             key_word_str,
                             civic_empty ? "" : &location_str[0]));
  }
  return VTSS_RC_OK;
}

// Help function for print global configuration.
// help function for printing the mode.
// IN : lldp_conf - Pointer to the configuration.
//      iport - Port in question
//      all_defaults - TRUE if we shall be printing everything (else we are only printing non-default configurations).
//      result - To be used by vtss_icfg_printf
static mesa_rc lldpmed_icfg_print_global(vtss_appl_lldp_common_conf_t *lldp_conf, mesa_port_no_t iport, BOOL all_defaults, vtss_icfg_query_result_t *result)
{
  char buf[25];


  long altitude;
  long longitude;
  long latitude;
  VTSS_RC(lldp_tudes_as_long(lldp_conf, &altitude, &longitude, &latitude));

  // Altitude_
  if (all_defaults ||
      (lldp_conf->coordinate_location.altitude_type != LLDPMED_ALTITUDE_TYPE_DEFAULT) ||
      (altitude != LLDPMED_ALTITUDE_DEFAULT)) {

    mgmt_long2str_float(&buf[0], altitude, ALTITUDE_DIGIT);

    VTSS_RC(vtss_icfg_printf(result, "lldp med location-tlv altitude %s %s\n",
                             lldp_conf->coordinate_location.altitude_type == METERS ? "meters" : "floors",
                             buf));
  }

  // Latitude_
  vtss_appl_lldp_med_latitude_dir_t latitude_dir = get_latitude_dir(lldp_conf->coordinate_location.latitude);
  if (all_defaults ||
      (latitude_dir != LLDPMED_LATITUDE_DIR_DEFAULT) ||
      (latitude != LLDPMED_LATITUDE_DEFAULT)) {

    mgmt_long2str_float(&buf[0], latitude, TUDE_DIGIT);

    VTSS_RC(vtss_icfg_printf(result, "lldp med location-tlv latitude %s %s\n",
                             latitude_dir == SOUTH ? "south" : "north",
                             buf));
  }


  // Longitude_
  vtss_appl_lldp_med_longitude_dir_t longitude_dir = get_longitude_dir(lldp_conf->coordinate_location.longitude);
  T_IG(TRACE_GRP_CLI, "Longitude as 2':0x" VPRI64x, lldp_conf->coordinate_location.longitude);
  if (all_defaults ||
      longitude_dir != LLDPMED_LONGITUDE_DIR_DEFAULT ||
      (lldp_conf->coordinate_location.longitude != LLDPMED_LONGITUDE_DEFAULT)) {

    mgmt_long2str_float(&buf[0], longitude, TUDE_DIGIT);

    VTSS_RC(vtss_icfg_printf(result, "lldp med location-tlv longitude %s %s\n",
                             longitude_dir == EAST ? "east" : "west",
                             buf));
  }


  // Datum
  if (all_defaults ||
      lldp_conf->coordinate_location.datum != LLDPMED_DATUM_DEFAULT) {

    VTSS_RC(vtss_icfg_printf(result, "%slldp med datum %s\n",
                             lldp_conf->coordinate_location.datum == LLDPMED_DATUM_DEFAULT ? "no " : "",
                             lldp_conf->coordinate_location.datum == WGS84 ? "wgs84" :
                             lldp_conf->coordinate_location.datum == NAD83_NAVD88 ? "nad83-navd88" :
                             "nad83-mllw"));
  }

  // Fast start
  if (all_defaults ||
      lldp_conf->medFastStartRepeatCount != VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_DEFAULT) {
    VTSS_RC(vtss_icfg_printf(result, "lldp med fast %u\n", lldp_conf->medFastStartRepeatCount));
  }

  // Civic address
  BOOL is_default = strlen(lldp_conf->ca_country_code) == 0;

  if (all_defaults || !is_default) {
    VTSS_RC(vtss_icfg_printf(result, "%slldp med location-tlv civic-addr country %s\n",
                             is_default ? "no " : "",
                             is_default ? "" : lldp_conf->ca_country_code));
  }

  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A1, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A2, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A3, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A4, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A5, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_A6, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_PRD, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_POD, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_STS, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_HNO, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_HNS, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_LMK, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_LOC, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_NAM, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_ZIP, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_BUILD, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_UNIT, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_FLR, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_ROOM, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_PLACE, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_PCN, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_POBOX, result, lldp_conf));
  VTSS_RC(lldpmed_icfg_print_civic(all_defaults, LLDPMED_CATYPE_ADD_CODE, result, lldp_conf));


  // Elin
  BOOL is_ecs_non_default = strlen(lldp_conf->elin_location) != 0; // Default value for ecs is empty string
  if (all_defaults ||
      is_ecs_non_default) {
    VTSS_RC(vtss_icfg_printf(result, "%slldp med location-tlv elin-addr %s\n",
                             is_ecs_non_default ? "" : "no ",
                             lldp_conf->elin_location));
  }

  //
  // Policies
  //
  // First print all no used policies (Default is "no policy", so only print if "print all default" is used)
  u32 policy_index;

  // Print all policies defined
  for (policy_index = LLDPMED_POLICY_MIN ; policy_index <= LLDPMED_POLICY_MAX; policy_index++) {
    vtss_appl_lldp_med_policy_t policy;
    VTSS_RC(vtss_appl_lldp_conf_policy_get(policy_index, &policy));

    if (policy.in_use) {

      VTSS_RC(vtss_icfg_printf(result, "lldp med media-vlan-policy %d", policy_index));

      switch (policy.network_policy.application_type) {
      case VOICE:
        VTSS_RC(vtss_icfg_printf(result, " voice"));
        break;
      case VOICE_SIGNALING:
        VTSS_RC(vtss_icfg_printf(result, " voice-signaling"));
        break;
      case GUEST_VOICE:
        VTSS_RC(vtss_icfg_printf(result, " guest-voice"));
        break;
      case GUEST_VOICE_SIGNALING:
        VTSS_RC(vtss_icfg_printf(result, " guest-voice-signaling"));
        break;
      case SOFTPHONE_VOICE:
        VTSS_RC(vtss_icfg_printf(result, " softphone-voice"));
        break;
      case VIDEO_CONFERENCING:
        VTSS_RC(vtss_icfg_printf(result, " video-conferencing"));
        break;
      case STREAMING_VIDEO:
        VTSS_RC(vtss_icfg_printf(result, " streaming-video"));
        break;

      case VIDEO_SIGNALING:
        VTSS_RC(vtss_icfg_printf(result, " video-signaling"));
        break;
      }


      if (policy.network_policy.tagged_flag) {
        VTSS_RC(vtss_icfg_printf(result, " tagged"));
        VTSS_RC(vtss_icfg_printf(result, " %d", policy.network_policy.vlan_id));
        VTSS_RC(vtss_icfg_printf(result, " l2-priority %d", policy.network_policy.l2_priority));
      } else {
        VTSS_RC(vtss_icfg_printf(result, " untagged"));
      }
      VTSS_RC(vtss_icfg_printf(result, " dscp %d\n", policy.network_policy.dscp_value));
    }
  }

  return VTSS_RC_OK;
}


// Help function for determining if a port has any policies assigned.
// In : iport - port in question.
//      lldp_conf - LLDP configuration
// Return: TRUE if at least on policy is assigned to the port, else FALSE
static BOOL is_any_policies_assigned(vtss_ifindex_t ifindex, const vtss_appl_lldp_port_conf_t *lldp_conf)
{
  u32 policy;

  for (policy = LLDPMED_POLICY_MIN; policy <= LLDPMED_POLICY_MAX; policy++) {
    BOOL enabled;
    if (vtss_appl_lldp_conf_port_policy_get(ifindex, policy, &enabled) != VTSS_RC_OK) {
      T_D("Could not get polciy");
      return FALSE;
    }

    if (enabled) {
      return TRUE; // At least one policy set
    }
  }

  return FALSE;
}

// Function called by ICFG.
static mesa_rc lldpmed_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                        vtss_icfg_query_result_t *result)
{
  vtss_isid_t         isid;
  mesa_port_no_t      iport;
  CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_port_conf;

  switch (req->cmd_mode) {
  case ICLI_CMD_MODE_GLOBAL_CONFIG:
    vtss_appl_lldp_common_conf_t   lldp_conf;
    VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_conf));
    // Any port can be used
    VTSS_RC(lldpmed_icfg_print_global(&lldp_conf, 0, req->all_defaults, result));
    break;
  case ICLI_CMD_MODE_INTERFACE_PORT_LIST: {
    // Get current configuration for this switch
    isid = req->instance_id.port.isid;
    if (isid != VTSS_ISID_START) {
      return VTSS_RC_ERROR;
    }
    VTSS_RC(lldp_mgmt_conf_get(&lldp_port_conf[0]));

    iport = req->instance_id.port.begin_iport;
    T_D("Isid:%d, iport:%u, req->instance_id.port.usid:%d", isid, iport, req->instance_id.port.usid);

    if (msg_switch_configurable(isid)) {
      //
      // Optional TLVs
      //
      BOOL tlv_capabilities_disabled = (lldp_port_conf[iport].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT) == 0;
      BOOL tlv_location_disabled = (lldp_port_conf[iport].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT) == 0;
      BOOL tlv_policy_disabled = (lldp_port_conf[iport].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT) == 0;

      BOOL at_least_one_tlv_is_disabled = tlv_capabilities_disabled || tlv_location_disabled  || tlv_policy_disabled;

      BOOL at_least_one_tlv_is_enabled = !tlv_capabilities_disabled || !tlv_location_disabled || !tlv_policy_disabled;

#if defined(VTSS_SW_OPTION_POE)
      BOOL tlv_poe_disabled = (lldp_port_conf[iport].lldpmed_optional_tlvs_mask & VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT) == 0;
      at_least_one_tlv_is_disabled |= tlv_poe_disabled;
      at_least_one_tlv_is_enabled  |= !tlv_poe_disabled;

      T_NG_PORT(TRACE_GRP_CLI, iport, "at_least_one_tlv_is_enabled:%d, tlv_capabilities_disabled:%d, tlv_policy_disabled:%d, tlv_location_disabled:%d, tlv_poe_disabled:%d", at_least_one_tlv_is_disabled, tlv_capabilities_disabled, tlv_policy_disabled, tlv_location_disabled, tlv_poe_disabled);
#endif
      T_NG_PORT(TRACE_GRP_CLI, iport, "Mask:0x%X, tlv_poe_disabled:%d", lldp_port_conf[iport].lldpmed_optional_tlvs_mask);

      //Default is enabled
      if (at_least_one_tlv_is_enabled) {
        if (req->all_defaults) {
          VTSS_RC(vtss_icfg_printf(result, " lldp med transmit-tlv"));
          VTSS_RC(vtss_icfg_printf(result, "%s", tlv_capabilities_disabled ? "" : " capabilities"));
          VTSS_RC(vtss_icfg_printf(result, "%s", tlv_policy_disabled       ? "" : " network-policy"));
          VTSS_RC(vtss_icfg_printf(result, "%s", tlv_location_disabled     ? "" : " location"));
#if defined(VTSS_SW_OPTION_POE)
          VTSS_RC(vtss_icfg_printf(result, "%s", tlv_poe_disabled          ? "" : " poe"));
#endif
          VTSS_RC(vtss_icfg_printf(result, "\n"));
        }
      }

      T_NG_PORT(TRACE_GRP_CLI, iport, "at_least_one_tlv_is_disabled:%d, tlv_capabilities_disabled:%d, tlv_policy_disabled:%d, tlv_location_disabled:%d", at_least_one_tlv_is_disabled, tlv_capabilities_disabled, tlv_policy_disabled, tlv_location_disabled);
      //Default is enabled, so if one of them is disable, we will show
      if (at_least_one_tlv_is_disabled) {
        VTSS_RC(vtss_icfg_printf(result, " no lldp med transmit-tlv"));
        VTSS_RC(vtss_icfg_printf(result, "%s", tlv_capabilities_disabled ? " capabilities" : ""));
        VTSS_RC(vtss_icfg_printf(result, "%s", tlv_policy_disabled       ? " network-policy" : ""));
        VTSS_RC(vtss_icfg_printf(result, "%s", tlv_location_disabled     ? " location" : ""));
#if defined(VTSS_SW_OPTION_POE)
        VTSS_RC(vtss_icfg_printf(result, "%s", tlv_poe_disabled          ? " poe" : ""));
#endif
        VTSS_RC(vtss_icfg_printf(result, "\n"));
      }

      // Policies assigned to the port.
      char buf[150];

      vtss_ifindex_t ifindex;
      VTSS_RC(vtss_ifindex_from_port(isid, iport, &ifindex));

      if (req->all_defaults ||
          is_any_policies_assigned(ifindex, &lldp_port_conf[0])) {

        if (is_any_policies_assigned(ifindex, &lldp_port_conf[0])) {
          BOOL enabled[LLDPMED_POLICIES_CNT];
          memset (&enabled[0], FALSE, sizeof(enabled));


          vtss_lldpmed_policy_index_t *prev_policy_index = NULL, next_policy_index;

          while (vtss_appl_lldp_port_policies_itr(prev_policy_index, &next_policy_index) == VTSS_RC_OK) {
            prev_policy_index = &next_policy_index;

            VTSS_RC(vtss_appl_lldp_conf_port_policy_get(ifindex, next_policy_index, &enabled[next_policy_index]));
            T_DG(TRACE_GRP_CLI, "next_policy_index:%d, ena:%d", next_policy_index, enabled[next_policy_index]);
          }

          T_IG(TRACE_GRP_CLI, "next_policy_index:%d, ena:%d", next_policy_index, enabled[0]);
          // Convert boolean list to printable string.
          (void) mgmt_non_portlist2txt(&enabled[0], LLDPMED_POLICY_MIN, LLDPMED_POLICY_MAX, buf);
          VTSS_RC(vtss_icfg_printf(result, " lldp med media-vlan policy-list %s\n", buf));
        } else {
          VTSS_RC(vtss_icfg_printf(result, " no lldp med media-vlan policy-list\n"));
        }
      }

      //
      // Operation mode
      //
#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
      vtss_icfg_conf_print_t conf_print;
      vtss_icfg_conf_print_init(&conf_print, req, result);
      vtss_appl_lldp_med_device_type_t operation_mode = lldp_port_conf[iport].lldpmed_device_type;
      conf_print.is_default = operation_mode == LLDPMED_DEVICE_TYPE_DEFAULT;
      VTSS_RC(vtss_icfg_conf_print(&conf_print, "lldp med type", "%s",
                                   operation_mode == VTSS_APPL_LLDP_MED_END_POINT    ? "end-point" :
                                   operation_mode == VTSS_APPL_LLDP_MED_CONNECTIVITY ? "connectivity" : conf_print.is_default ? "" : "Unknown LLDPMED type"));
#endif

    }
    break;
    default:
      //Not needed for LLDP
      break;
    }
  } // End msg_switch_configurable
  return VTSS_RC_OK;
}

/* ICFG Initialization function */
mesa_rc lldpmed_icfg_init(void)
{
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_LLDPMED_GLOBAL_CONF, "lldp", lldpmed_icfg_global_conf));
  VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_LLDPMED_PORT_CONF, "lldp", lldpmed_icfg_global_conf));
  return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG

#endif // #ifdef VTSS_SW_OPTION_LLDP_MED

