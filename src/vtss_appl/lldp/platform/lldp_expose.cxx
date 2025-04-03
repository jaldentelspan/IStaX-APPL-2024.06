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
#include "lldp_serializer.hxx"
#include "vtss/appl/lldp.h"
#include "vtss/appl/tsn.h"
#include "lldp_tlv.h"
#include "lldp_api.h"
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-depend-N.hxx"

// Make variable here in order to put it at the stack (we have seen stack overflow).
// It is OK. The SNMP frame work will make sure that it is semaphore protected.
static vtss_appl_lldp_common_conf_t lldp_common_conf;

/****************************************************************************
 * Get/Set functions
 ****************************************************************************/
// Dummy function because the "Write Only" OID is implemented with a ReadWrite
mesa_rc lldp_global_stats_dummy_get( BOOL *const clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}
// Getting Preemption management information from an entry
mesa_rc lldp_preempt_management_infor_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, u32 mgmt_addr_index, vtss_lldp_mgmt_fp_t *lldp_preempt) {
    vtss_appl_lldp_remote_entry_t entry;

    if (mgmt_addr_index >= LLDP_MGMT_ADDR_CNT) {
        return VTSS_APPL_LLDP_ERROR_MGMT_ADDR_INDEX_INVALID;
    }

#ifdef VTSS_SW_OPTION_TSN
    vtss_appl_tsn_fp_status_t local_preempt_status;
    VTSS_RC(vtss_appl_tsn_fp_status_get(ifindex, &local_preempt_status));
    lldp_preempt->loc_preempt_supported     = local_preempt_status.loc_preempt_supported;
    lldp_preempt->loc_preempt_enabled       = local_preempt_status.loc_preempt_enabled;
    lldp_preempt->loc_preempt_active        = local_preempt_status.loc_preempt_active;
    lldp_preempt->loc_add_frag_size         = local_preempt_status.loc_add_frag_size;
#else
    lldp_preempt->loc_preempt_supported     = LLDP_FALSE;
    lldp_preempt->loc_preempt_enabled       = LLDP_FALSE;
    lldp_preempt->loc_preempt_active        = LLDP_FALSE;
    lldp_preempt->loc_add_frag_size         = 0;
#endif // VTSS_SW_OPTION_TSN

    VTSS_RC(vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, FALSE));
    lldp_preempt->RemFramePreemptSupported  = entry.fp.RemFramePreemptSupported;
    lldp_preempt->RemFramePreemptEnabled    = entry.fp.RemFramePreemptEnabled;
    lldp_preempt->RemFramePreemptActive     = entry.fp.RemFramePreemptActive;
    lldp_preempt->RemFrameAddFragSize       = entry.fp.RemFrameAddFragSize;

    return VTSS_RC_OK;
}

// Getting Neighbors management information from an entry
mesa_rc lldp_neighbors_management_infor_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, u32 mgmt_addr_index, vtss_lldp_mgmt_addr_tlv_t *lldp_info) {
    vtss_appl_lldp_remote_entry_t entry;

    VTSS_RC(vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, FALSE));

    if (mgmt_addr_index >= LLDP_MGMT_ADDR_CNT) {
        return VTSS_APPL_LLDP_ERROR_MGMT_ADDR_INDEX_INVALID;
    }

    // Get the management information.
    T_NG(TRACE_GRP_SNMP, "entry.mgmt_addr[%d].length:%d", mgmt_addr_index, entry.mgmt_addr[mgmt_addr_index].length);
    T_NG(TRACE_GRP_SNMP, "entry.mgmt_addr[%d].oid_length:%d", mgmt_addr_index, entry.mgmt_addr[mgmt_addr_index].oid_length);
    T_NG(TRACE_GRP_SNMP, "oid:%d, %d, %d, %d, %d",
         entry.mgmt_addr[mgmt_addr_index].oid[0], entry.mgmt_addr[mgmt_addr_index].oid[1], entry.mgmt_addr[mgmt_addr_index].oid[2], entry.mgmt_addr[mgmt_addr_index].oid[3], entry.mgmt_addr[mgmt_addr_index].oid[4]);
    if (entry.in_use && entry.mgmt_addr[mgmt_addr_index].length > 0) {
        lldp_info->subtype = entry.mgmt_addr[mgmt_addr_index].subtype;

        char mgmt_addr_buf[VTSS_APPL_MAX_MGMT_ADDR_LENGTH + 1]; // Plus one for the "\0"
        vtss_appl_lldp_mgmt_addr2string(&entry.mgmt_addr[mgmt_addr_index], &mgmt_addr_buf[0]);
        strcpy(lldp_info->mgmt_address, mgmt_addr_buf);

        lldp_info->if_number_subtype= entry.mgmt_addr[mgmt_addr_index].if_number_subtype;
        lldp_info->if_number = (entry.mgmt_addr[mgmt_addr_index].if_number[0] << 24) +
            (entry.mgmt_addr[mgmt_addr_index].if_number[1] << 16) +
            (entry.mgmt_addr[mgmt_addr_index].if_number[2] << 8) +
            (entry.mgmt_addr[mgmt_addr_index].if_number[3]);

        u32 buf[VTSS_APPL_MAX_OID_LENGTH];
        u32 buf_len = 0;
        memset(buf, 0, VTSS_APPL_MAX_OID_LENGTH);

        if (entry.mgmt_addr[mgmt_addr_index].oid_length > 0) {
            oid_decode((u8*) &entry.mgmt_addr[mgmt_addr_index].oid, entry.mgmt_addr[mgmt_addr_index].oid_length, &buf[0], VTSS_APPL_MAX_OID_LENGTH, &buf_len);
        }

        lldp_info->oid_length = buf_len;
        memcpy(lldp_info->oid , &buf[0], VTSS_APPL_MAX_OID_LENGTH);
        return VTSS_RC_OK;
    }

    return VTSS_APPL_LLDP_ERROR_MGMT_ADDR_NOT_FOUND;
}

// Getting LLDP-MED neighbors network information from an entry
mesa_rc lldpmed_neighbors_network_policy_info_get(vtss_ifindex_t ifindex,
                                                  vtss_lldp_entry_index_t entry_index,
                                                  vtss_lldpmed_policy_index_t policy_index,
                                                  vtss_appl_lldp_med_network_policy_t *lldp_policy)
{
    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    vtss_appl_lldp_remote_entry_t entry;
    if (vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, FALSE) != VTSS_RC_OK) {
        T_IG(TRACE_GRP_SNMP, "No entry found for entry_index:%d", entry_index);
        return VTSS_RC_ERROR;
    };

    if (!entry.policy[policy_index].in_use) {
        T_IG(TRACE_GRP_SNMP, "No entry found for policy_index:%d", policy_index);
        return VTSS_APPL_LLDP_ERROR_POLICY_NOT_DEFINED;
    }

    memcpy(lldp_policy, &entry.policy[policy_index].network_policy, sizeof(vtss_appl_lldp_med_network_policy_t));

    T_DG(TRACE_GRP_SNMP, "entry_index:%d, port:%d, policy_index:%d, table_p[i].lldpmed_policy_vld[*next_policy]:%d",
         entry_index, ife.ordinal, policy_index, entry.policy[policy_index].in_use);

    return VTSS_RC_OK;
}

mesa_rc lldpmed_policies_def(vtss_lldpmed_policy_index_t *policy_index, vtss_appl_lldp_med_conf_network_policy_t *network_policy)
{
    *policy_index = 0;
    memset(network_policy,0,sizeof(*network_policy));
    network_policy->conf_policy.application_type = VOICE;
    network_policy->conf_policy.vlan_id = 1;
    network_policy->conf_policy.tagged_flag = 0;
    return VTSS_RC_OK;
}

// Configuring a policy
mesa_rc lldpmed_policies_set(vtss_lldpmed_policy_index_t policy_index, const vtss_appl_lldp_med_conf_network_policy_t *network_policy)
{
    vtss_appl_lldp_med_policy_t policy;
    policy.network_policy = network_policy->conf_policy;
    policy.in_use = TRUE;

    VTSS_RC(vtss_appl_lldp_conf_policy_set((u8) policy_index, policy));
    return VTSS_RC_OK;
}

mesa_rc lldpmed_policies_del(vtss_lldpmed_policy_index_t policy_index)
{
    vtss_appl_lldp_med_policy_t policy;

    VTSS_RC(vtss_appl_lldp_conf_policy_get((u8) policy_index, &policy));
    if (!policy.in_use) return VTSS_APPL_LLDP_ERROR_POLICY_NOT_DEFINED;

    policy.in_use = FALSE;
    VTSS_RC(vtss_appl_lldp_conf_policy_set((u8) policy_index, policy));
    return VTSS_RC_OK;
}


// Wrapper for port configuration get
mesa_rc vtss_lldpmed_port_conf_get(vtss_ifindex_t ifindex, lldpmed_port_conf_t *lldpmed_port_conf)
{
    vtss_appl_lldp_port_conf_t current_lldp_conf;

    if (!vtss_ifindex_is_port(ifindex)) {
        return VTSS_APPL_LLDP_ERROR_IFINDEX;
    }

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    // Get current configuration for this switch
    vtss_appl_lldp_port_conf_get(ifindex, &current_lldp_conf);

#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
    lldpmed_port_conf->device_type        = current_lldp_conf.lldpmed_device_type;
#endif
    lldpmed_port_conf->lldpmed_optional_tlv  = current_lldp_conf.lldpmed_optional_tlvs_mask;
    return VTSS_RC_OK;
}

// Getting LLDP port configuration
mesa_rc vtss_lldp_port_conf_get(vtss_ifindex_t ifindex, lldp_port_mib_conf_t *lldp_port_conf)
{
    vtss_appl_lldp_port_conf_t current_lldp_conf;

    // Get current configuration for this switch
    VTSS_RC(vtss_appl_lldp_port_conf_get(ifindex, &current_lldp_conf));

    lldp_port_conf->admin_state  = current_lldp_conf.admin_states;
#ifdef VTSS_SW_OPTION_CDP
    lldp_port_conf->cdp_aware    = current_lldp_conf.cdp_aware;
#endif
    lldp_port_conf->optional_tlv = current_lldp_conf.optional_tlvs_mask;
    lldp_port_conf->snmp_notification_ena = current_lldp_conf.snmp_notification_ena;

    return VTSS_RC_OK;
}

// Setting LLDP-MED port configuration
mesa_rc vtss_lldpmed_port_conf_set(vtss_ifindex_t ifindex, const lldpmed_port_conf_t *lldpmed_port_conf)
{
    vtss_appl_lldp_port_conf_t current_lldp_conf;

    if (!vtss_ifindex_is_port(ifindex)) {
        T_IG(TRACE_GRP_SNMP, "ifindex:%d is not port", VTSS_IFINDEX_PRINTF_ARG(ifindex));
        return VTSS_APPL_LLDP_ERROR_IFINDEX;
    }


    u32 all_mask = VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT |
                   VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT |
#ifdef VTSS_SW_OPTION_POE
                   VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT |
#endif
                   VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT;

    if (lldpmed_port_conf->lldpmed_optional_tlv > all_mask) {
        T_IG(TRACE_GRP_SNMP, "Invalid lldpmed_optional_tlv:%d", lldpmed_port_conf->lldpmed_optional_tlv);
        return VTSS_APPL_LLDP_ERROR_UNSUPPORTED_OPTIONAL_TLVS_BITS;
    }

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    // Get current configuration for this switch
    vtss_appl_lldp_port_conf_get(ifindex, &current_lldp_conf);

    current_lldp_conf.lldpmed_optional_tlvs_mask = lldpmed_port_conf->lldpmed_optional_tlv;
#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
    current_lldp_conf.lldpmed_device_type     = lldpmed_port_conf->device_type;
#endif
    vtss_appl_lldp_port_conf_set(ifindex, &current_lldp_conf);
    return VTSS_RC_OK;
}

// Setting LLDP port configuration
mesa_rc vtss_lldp_port_conf_set(vtss_ifindex_t ifindex, const lldp_port_mib_conf_t *lldp_port_conf)
{
    vtss_appl_lldp_port_conf_t current_lldp_conf;

    // Get current configuration for this switch
    VTSS_RC(vtss_appl_lldp_port_conf_get(ifindex, &current_lldp_conf));

     current_lldp_conf.admin_states  = lldp_port_conf->admin_state;

#ifdef VTSS_SW_OPTION_CDP
    current_lldp_conf.cdp_aware    = lldp_port_conf->cdp_aware;
#endif

    // If trying to set some bits which we don't support then return an error
    if ((lldp_port_conf->optional_tlv & ~VTSS_APPL_LLDP_TLV_OPTIONAL_ALL_BITS) != 0) {
        return VTSS_APPL_LLDP_ERROR_UNSUPPORTED_OPTIONAL_TLVS_BITS;
    }

    current_lldp_conf.snmp_notification_ena = lldp_port_conf->snmp_notification_ena;

    current_lldp_conf.optional_tlvs_mask = lldp_port_conf->optional_tlv;

    VTSS_RC(vtss_appl_lldp_port_conf_set(ifindex, &current_lldp_conf));
    return VTSS_RC_OK;
}

// Getting LLDP neighbors information from an entry
mesa_rc lldp_neighbors_information_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, lldp_neighbors_information_t *lldp_info)
{
    vtss_appl_lldp_remote_entry_t entry;

    VTSS_RC(vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, FALSE));

    (void) vtss_appl_lldp_chassis_id2string(&entry, lldp_info->chassis_id);

    lldp_remote_tlv_to_string(&entry, LLDP_TLV_BASIC_MGMT_PORT_ID,      &lldp_info->port_id[0],            sizeof(lldp_info->port_id),            0);
    lldp_remote_tlv_to_string(&entry, LLDP_TLV_BASIC_MGMT_PORT_DESCR,   &lldp_info->port_description[0],   sizeof(lldp_info->port_description),   0);
    lldp_remote_tlv_to_string(&entry, LLDP_TLV_BASIC_MGMT_SYSTEM_NAME,  &lldp_info->system_name[0],        sizeof(lldp_info->system_name),        0);
    lldp_remote_tlv_to_string(&entry, LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR, &lldp_info->system_description[0], sizeof(lldp_info->system_description), 0);

    lldp_info->capa     = (entry.system_capabilities[0] << 8) | entry.system_capabilities[1];
    lldp_info->capa_ena = (entry.system_capabilities[2] << 8) | entry.system_capabilities[3];
    return VTSS_RC_OK;
}

// Getting LLDP-MED neighbors information
mesa_rc lldpmed_neighbors_information_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, lldpmed_remote_info_t *lldp_info)
{
    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    memset(lldp_info, 0, sizeof(lldpmed_remote_info_t));

    vtss_appl_lldp_remote_entry_t entry;
    if (vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, FALSE) != VTSS_RC_OK) {
        T_IG(TRACE_GRP_SNMP, "Not entry found for entry_index:%d", entry_index);
        return VTSS_RC_ERROR;
    };

    // The entry can contain "normal" LLDP information, here we are interested in the LLDP-MED information,
    // if there is no LLDP-MED information we signal no entry found.
    if (!entry.lldpmed_info_vld) {
        return VTSS_RC_ERROR;
    }

    T_IG(TRACE_GRP_SNMP, "entry:%d, lldpmed_capabilities_vld:%d", entry_index, entry.lldpmed_capabilities_vld);
    if (entry.lldpmed_capabilities_vld) {
        lldp_info->lldpmed_capabilities         = entry.lldpmed_capabilities;
        lldp_info->lldpmed_capabilities_current = entry.lldpmed_capabilities_current;
    }

    lldp_info->device_type = (vtss_appl_lldp_med_remote_device_type_t) entry.lldpmed_device_type;


    (void) vtss_appl_lldp_med_invertory_info_get(&entry, LLDPMED_HW_REV, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &lldp_info->lldpmed_hw_rev[0]);
    (void) vtss_appl_lldp_med_invertory_info_get(&entry, LLDPMED_FW_REV, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &lldp_info->lldpmed_firm_rev[0]);
    (void) vtss_appl_lldp_med_invertory_info_get(&entry, LLDPMED_SW_REV, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &lldp_info->lldpmed_sw_rev[0]);
    (void) vtss_appl_lldp_med_invertory_info_get(&entry, LLDPMED_SER_NUM, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &lldp_info->lldpmed_serial_no[0]);
    (void) vtss_appl_lldp_med_invertory_info_get(&entry, LLDPMED_MANUFACTURER_NAME, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &lldp_info->lldpmed_manufacturer_name[0]);
    (void) vtss_appl_lldp_med_invertory_info_get(&entry, LLDPMED_MODEL_NAME, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &lldp_info->lldpmed_model_name[0]);
    (void) vtss_appl_lldp_med_invertory_info_get(&entry, LLDPMED_ASSET_ID, VTSS_APPL_LLDPMED_INVENTORY_LEN_MAX, &lldp_info->lldpmed_asset_id[0]);

    if (entry.lldpmed_coordinate_location_vld) {
        memcpy(&lldp_info->coordinate_location, &entry.coordinate_location, sizeof(lldp_info->coordinate_location));
    }

    if (entry.lldpmed_elin_location_vld) {
        memcpy(&lldp_info->elin_location, &entry.lldpmed_elin_location, entry.lldpmed_elin_location_length);
    }

    lldp_info->eee.RemRxTwSys     = entry.eee.RemRxTwSys;
    lldp_info->eee.RemTxTwSys     = entry.eee.RemTxTwSys;
    lldp_info->eee.RemFbTwSys     = entry.eee.RemFbTwSys;
    lldp_info->eee.RemTxTwSysEcho = entry.eee.RemTxTwSysEcho;
    lldp_info->eee.RemRxTwSysEcho = entry.eee.RemRxTwSysEcho;
    return VTSS_RC_OK;
}


// Clear global counters
mesa_rc lldp_stats_global_clr_set(const BOOL *const clear)
{
    if (clear && *clear) { // Make sure the Clear is not a NULL pointer
        vtss_appl_lldp_global_stat_clr();
    }
    return VTSS_RC_OK;
}

// Getting LLDP part of the global configuration
mesa_rc vtss_lldp_global_conf_get(vtss_appl_lldp_global_conf_t *lldp_conf)
{
    VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_common_conf));
    lldp_conf->reInitDelay   = lldp_common_conf.tx_sm.reInitDelay;
    lldp_conf->msgTxHold     = lldp_common_conf.tx_sm.msgTxHold;
    lldp_conf->msgTxInterval = lldp_common_conf.tx_sm.msgTxInterval;
    lldp_conf->txDelay       = lldp_common_conf.tx_sm.txDelay;

    return VTSS_RC_OK;
}

// Setting LLDP part of the global configuration
mesa_rc vtss_lldp_global_conf_set(const vtss_appl_lldp_global_conf_t *lldp_conf)
{
    VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_common_conf));
    lldp_common_conf.tx_sm.reInitDelay   = lldp_conf->reInitDelay;
    lldp_common_conf.tx_sm.msgTxHold     = lldp_conf->msgTxHold;
    lldp_common_conf.tx_sm.msgTxInterval = lldp_conf->msgTxInterval;
    lldp_common_conf.tx_sm.txDelay       = lldp_conf->txDelay;
    VTSS_RC(vtss_appl_lldp_common_conf_set(&lldp_common_conf));
    return VTSS_RC_OK;
}

// Getting LLDP-MED part of the global configuration
mesa_rc vtss_lldpmed_global_conf_get(lldpmed_global_conf_t *lldpmed_conf)
{
    vtss_appl_lldp_common_conf_t lldp_common_conf;
    VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_common_conf));
    lldpmed_conf->medFastStartRepeatCount = lldp_common_conf.medFastStartRepeatCount;
    lldpmed_conf->coordinate_location     = lldp_common_conf.coordinate_location;
    strcpy(lldpmed_conf->elin_location,     lldp_common_conf.elin_location);
    strcpy(lldpmed_conf->ca_country_code,   lldp_common_conf.ca_country_code); // Country code is stored for itself. See. TIA1057, figure 10
    return VTSS_RC_OK;
}

// Setting LLDP-MED part of the global configuration
mesa_rc vtss_lldpmed_global_conf_set(const lldpmed_global_conf_t *lldpmed_conf)
{
    vtss_appl_lldp_common_conf_t lldp_common_conf;
    VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_common_conf));
    lldp_common_conf.medFastStartRepeatCount = lldpmed_conf->medFastStartRepeatCount;
    lldp_common_conf.coordinate_location     = lldpmed_conf->coordinate_location;
    strcpy(lldp_common_conf.elin_location,     lldpmed_conf->elin_location);
    strcpy(lldp_common_conf.ca_country_code,   lldpmed_conf->ca_country_code); // Country code is stored for itself. See. TIA1057, figure 10
    VTSS_RC(vtss_appl_lldp_common_conf_set(&lldp_common_conf));
    T_IG(TRACE_GRP_SNMP, "Done setting LLDPMED");
    return VTSS_RC_OK;
}

// Setting location configuration. vtss_lldpmed_ca_type_t  contain the civic string as a type which can be serialized.
// The configuration is stored as a concatenated string in order to save place.
// IN : civic_location - The civic locations to be stored in the configuration.
mesa_rc lldpmed_conf_location_set(vtss_appl_lldp_med_catype_t ca_type_index, const vtss_lldpmed_ca_type_t *civic_location)
{
    vtss_appl_lldp_common_conf_t lldp_common_conf;
    T_DG(TRACE_GRP_SNMP, "ca_type_index:%d", ca_type_index);
    VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_common_conf)); // Getting the location information configuration

    VTSS_RC(vtss_appl_lldp_location_civic_info_set(&lldp_common_conf.civic, (vtss_appl_lldp_med_catype_t)ca_type_index,
                                                   civic_location->ca_type));
    VTSS_RC(vtss_appl_lldp_common_conf_set(&lldp_common_conf));
    return VTSS_RC_OK;
}

// Getting civic location configuration - See also lldpmed_conf_location_set
mesa_rc lldpmed_conf_location_get(vtss_appl_lldp_med_catype_t ca_type_index, vtss_lldpmed_ca_type_t *civic_location)
{
    vtss_appl_lldp_common_conf_t lldp_common_conf;
    VTSS_RC(vtss_appl_lldp_common_conf_get(&lldp_common_conf)); // Getting the location information configuration
    if (vtss_appl_lldp_location_civic_info_get(&lldp_common_conf.civic,(vtss_appl_lldp_med_catype_t)ca_type_index,
                                               VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX -1, civic_location->ca_type) == NULL) {

        T_IG(TRACE_GRP_SNMP, "Invalid index:%d", ca_type_index);
        return VTSS_APPL_LLDP_ERROR_CIVIC_TYPE;
    };
    T_NG(TRACE_GRP_SNMP, "ca_type_index:%d", ca_type_index);
    return VTSS_RC_OK;
}

// Getting policy configuration
mesa_rc lldpmed_policies_get(vtss_lldpmed_policy_index_t policy_index, vtss_appl_lldp_med_conf_network_policy_t *network_policy)
{
    vtss_appl_lldp_med_policy_t policy;
    vtss_appl_lldp_cap_t cap;
    vtss_appl_lldp_cap_get(&cap);

    if (policy_index > cap.policies_cnt) return VTSS_APPL_LLDP_ERROR_POLICY_OUT_OF_RANGE;

    VTSS_RC(vtss_appl_lldp_conf_policy_get(policy_index, &policy));

    if (!policy.in_use) {
        return VTSS_APPL_LLDP_ERROR_POLICY_NOT_DEFINED;
    } else {
        network_policy->conf_policy = policy.network_policy;
    }
    return VTSS_RC_OK;
}

// Wrapper for clear counters, converting from ifindex to isid, port
// In : ifindex - OID ifindex
//      clear   - Set to TRUE to clear counters for the select ifindex interface
mesa_rc lldp_stats_clr_set(vtss_ifindex_t ifindex, const BOOL *const clear)
{
    if (clear && *clear) { // Make sure the Clear is not a NULL pointer
        if (!vtss_ifindex_is_port(ifindex)) {
            return VTSS_APPL_LLDP_ERROR_IFINDEX;
        }

        vtss_appl_lldp_if_stat_clr(ifindex);
    }
    return VTSS_RC_OK;
}

// Getting neighbors civic location from a specific entry
// IN: ifindex     - Interface that the entry must be assigned to.
//     entry_index - Entry index for the entry table.
// OUT: civic      - Pointer to where put the result
// RETURN : VTSS_RC_OK if civic is valid, else error code
mesa_rc lldpmed_neighbors_location_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t entry_index, vtss_appl_lldpmed_civic_t *civic)
{
    T_RG(TRACE_GRP_SNMP, "entry index:%d", entry_index);

    // looking up the entry
    vtss_appl_lldp_remote_entry_t entry;
    VTSS_RC(vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, FALSE));

    // The entry can contain "normal" LLDP information, here we are interested in the LLDP-MED information,
    // if there is no LLDP-MED information we signal no entry found.
    if (!entry.lldpmed_info_vld) {
        return VTSS_RC_ERROR;
    }

    u8 *location_info    = &entry.lldpmed_civic_location[0];
    lldp_u16_t lci_length = location_info[0] + 1; // Figure 10, TIA1057 + Section 10.2.4.3.2 (adding 1 to the LCI length)

    vtss_appl_lldp_med_catype_t ca_type; // See Figure 10, TIA1057
    lldp_u8_t ca_length = 0; // See Figure 10, TIA1057
    lldp_u16_t tlv_index = 4; // First CAtype is byte 4 in the TLV. Figure 10, TIA1057

    // Getting the country code which is a little bit special
    char country_code[3] = {(char)location_info[2], (char)location_info[3], '\0'}; // Figure 10, TIA1057
    if (strlen(country_code) != 0) {
        T_IG(TRACE_GRP_SNMP, "country_code:%s", country_code);
        strncpy(civic->ca_country_code, country_code, VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN);
    }

    // Loop through the civic location TLS from the entry and pick out the civic location in it.
    T_NG(TRACE_GRP_SNMP, "tlv_index:%d, lci_length:%d", tlv_index, lci_length);
    while (tlv_index < lci_length) {
        T_NG(TRACE_GRP_SNMP, "tlv_index:%d, lci_length:%d", tlv_index, lci_length);
        ca_type = (vtss_appl_lldp_med_catype_t) location_info[tlv_index];
        ca_length = location_info[tlv_index + 1];

        if (tlv_index +  2 + ca_length  > lci_length || ca_length >= VTSS_APPL_LLDP_CIVIC_CA_VAL_LEN_MAX) {
            T_WG(TRACE_GRP_MED_RX, "Invalid CA length, tlv_index = %d, ca_length = %d, lci_length = %d",
                 tlv_index, ca_length, lci_length);
            break;
        }
        T_NG(TRACE_GRP_SNMP, "ca_type:%d", ca_type);
        char *ca_ptr; // Pointer for pointing to the civic type
        switch (ca_type) {
        case LLDPMED_CATYPE_A1:       ca_ptr = civic->a1;       break;
        case LLDPMED_CATYPE_A2:       ca_ptr = civic->a2;       break;
        case LLDPMED_CATYPE_A3:       ca_ptr = civic->a3;       break;
        case LLDPMED_CATYPE_A4:       ca_ptr = civic->a4;       break;
        case LLDPMED_CATYPE_A5:       ca_ptr = civic->a5;       break;
        case LLDPMED_CATYPE_A6:       ca_ptr = civic->a6;       break;
        case LLDPMED_CATYPE_PRD:      ca_ptr = civic->prd;      break;
        case LLDPMED_CATYPE_POD:      ca_ptr = civic->pod;      break;
        case LLDPMED_CATYPE_STS:      ca_ptr = civic->sts;      break;
        case LLDPMED_CATYPE_HNO:      ca_ptr = civic->hno;      break;
        case LLDPMED_CATYPE_HNS:      ca_ptr = civic->hns;      break;
        case LLDPMED_CATYPE_LMK:      ca_ptr = civic->lmk;      break;
        case LLDPMED_CATYPE_LOC:      ca_ptr = civic->loc;      break;
        case LLDPMED_CATYPE_NAM:      ca_ptr = civic->nam;      break;
        case LLDPMED_CATYPE_ZIP:      ca_ptr = civic->zip;      break;
        case LLDPMED_CATYPE_BUILD:    ca_ptr = civic->build;    break;
        case LLDPMED_CATYPE_UNIT:     ca_ptr = civic->unit;     break;
        case LLDPMED_CATYPE_FLR:      ca_ptr = civic->flr;      break;
        case LLDPMED_CATYPE_ROOM:     ca_ptr = civic->room;     break;
        case LLDPMED_CATYPE_PLACE:    ca_ptr = civic->place;    break;
        case LLDPMED_CATYPE_PCN:      ca_ptr = civic->pcn;      break;
        case LLDPMED_CATYPE_POBOX:    ca_ptr = civic->pobox;    break;
        case LLDPMED_CATYPE_ADD_CODE: ca_ptr = civic->add_code; break;
        default:
            T_W("Unknown ca_type:%d", ca_type);
            return VTSS_APPL_LLDP_ERROR_CIVIC_TYPE;
        }

        memcpy(ca_ptr, &location_info[tlv_index + 2], ca_length);
        ca_ptr[ca_length] = '\0'; // Add NULL pointer since that is not in the TLVs
        tlv_index += 2 + ca_length; // Select next CA
    }

    return VTSS_RC_OK;
}

// Setting/mapping policies to port
mesa_rc lldpmed_policies_list_info_set(vtss_ifindex_t port_ifindex, vtss_lldpmed_policy_index_t policy_index, const BOOL *policy)
{
    T_IG(TRACE_GRP_SNMP, "policy:%d", *policy);
    VTSS_RC(vtss_appl_lldp_conf_port_policy_set(port_ifindex, policy_index, *policy));
    return VTSS_RC_OK;
}

// We support multiple management interface information for each entry, so we need an iterator for them.
mesa_rc vtss_ifindex_management_entry_itr(vtss_appl_lldp_remote_entry_t *entry, const u32 *prev_management_index, u32 *next_management_index) {
    u32 start_index;
    u32 mgmt_addr_index;
    if (prev_management_index == NULL) {
        start_index = 0;
    } else {
        start_index = *prev_management_index + 1;
    }

    for (mgmt_addr_index = start_index; mgmt_addr_index < LLDP_MGMT_ADDR_CNT; mgmt_addr_index++) {
        if (entry->mgmt_addr[mgmt_addr_index].length > 0) {
            *next_management_index = mgmt_addr_index;
            T_DG(TRACE_GRP_SNMP, "next_management_index:%d", *next_management_index);
            return VTSS_RC_OK;
        }
    }
    T_IG(TRACE_GRP_SNMP, "no management address found, start_index:%d", start_index);
    return VTSS_RC_ERROR; // Signal no more management addresses found
}

// Port iterator
static mesa_rc vtss_ifindex_port_entry_itr(vtss_isid_t isid, mesa_port_no_t iport, const vtss_lldp_entry_index_t *prev_entry, vtss_lldp_entry_index_t *next_entry) {
    vtss_lldp_entry_index_t start_index;
    vtss_lldp_entry_index_t i;
    vtss_appl_lldp_cap_t cap;
    vtss_appl_lldp_cap_get(&cap);
    vtss_appl_lldp_remote_entry_t *p_entry;

    if (prev_entry == NULL) {
        start_index = 0;
    } else {
        start_index = *prev_entry +1;
    }

    vtss_ifindex_t ifindex;
    VTSS_RC(vtss_ifindex_from_port(isid, iport, &ifindex));
    if (isid != VTSS_ISID_START) {
        return VTSS_RC_ERROR;
    }

    vtss_appl_lldp_mutex_lock();
    // We get the whole entry table instead of using vtss_appl_lldp_entry_get, because that is a lot faster when we have to loop through all entries.
    p_entry = vtss_appl_lldp_entries_get();

    for (i = start_index; i < cap.remote_entries_cnt; i++) {
        if (p_entry[i].in_use && p_entry[i].receive_port == iport) {
            *next_entry = i;

            T_IG(TRACE_GRP_SNMP, "Found entry index:%d", i);
            vtss_appl_lldp_mutex_unlock();
            return VTSS_RC_OK;
        };
    }
    vtss_appl_lldp_mutex_unlock();
    T_DG(TRACE_GRP_SNMP, "Not found, start_index:%d, isid:%d, iport:%d", start_index, isid, iport);
    return VTSS_APPL_LLDP_ERROR_ENTRY_INDEX; // Signal that no entry was found
}

// Getting next entry index.
static mesa_rc vtss_lldp_next_entry(const vtss_lldp_entry_index_t *prev_entry_index,
                                    vtss_lldp_entry_index_t       *next_entry_index,
                                    vtss_ifindex_t                ifindex) {
    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    return vtss_ifindex_port_entry_itr(ife.isid, ife.ordinal, prev_entry_index, next_entry_index);
}

// Getting the next management index
static mesa_rc vtss_lldp_next_mgmt_entry(const u32                     *prev_management_index,
                                         u32                           *next_management_index,
                                         vtss_ifindex_t                ifindex,
                                         vtss_lldp_entry_index_t       entry_index) {
    vtss_appl_lldp_remote_entry_t entry;
    VTSS_RC(vtss_appl_lldp_entry_get(ifindex, &entry_index, &entry, TRUE));
    return (vtss_ifindex_management_entry_itr(&entry, prev_management_index, next_management_index));
}


// Iterating over interface index, lldp entries and management address indexes
mesa_rc vtss_appl_lldp_port_mgmt_entries_itr(const vtss_ifindex_t          *prev_ifindex,
                                             vtss_ifindex_t                *next_ifindex,
                                             const vtss_lldp_entry_index_t *prev_entry_index,
                                             vtss_lldp_entry_index_t       *next_entry_index,
                                             const u32                     *prev_mgmt_index,
                                             u32                           *next_mgmt_index)
{
    vtss::IteratorComposeDependN<vtss_ifindex_t, vtss_lldp_entry_index_t, u32> itr(vtss_appl_iterator_ifindex_front_port,
                                                                                   vtss_lldp_next_entry,
                                                                                   vtss_lldp_next_mgmt_entry);

    return (itr(prev_ifindex, next_ifindex, prev_entry_index, next_entry_index, prev_mgmt_index, next_mgmt_index));
}


// Iterating over interface index and lldp entries
mesa_rc vtss_appl_lldp_port_entries_itr(const vtss_ifindex_t          *prev_ifindex,     vtss_ifindex_t *next_ifindex,
                                        const vtss_lldp_entry_index_t *prev_entry_index, vtss_lldp_entry_index_t *next_entry_index)
{
    vtss::IteratorComposeDependN<vtss_ifindex_t, vtss_lldp_entry_index_t> itr(vtss_appl_iterator_ifindex_front_port,
                                                                              vtss_lldp_next_entry);
    return itr(prev_ifindex, next_ifindex, prev_entry_index, next_entry_index);
}

mesa_rc remote_policies_itr(const vtss_ifindex_t              *prev_port_ifindex, vtss_ifindex_t              *next_port_ifindex,
                            const vtss_lldp_entry_index_t     *prev_table_entry,  vtss_lldp_entry_index_t     *next_table_entry,
                            const vtss_lldpmed_policy_index_t *prev_policy,       vtss_lldpmed_policy_index_t *next_policy)
{
    *next_port_ifindex = prev_port_ifindex ? *prev_port_ifindex : VTSS_IFINDEX_START;
    *next_table_entry  = prev_table_entry  ? *prev_table_entry  : 0;
    *next_policy = prev_policy ? *prev_policy + 1 : 0;

    vtss_appl_lldp_remote_entry_t entry;

    // Starting from first entry if previous not found
    vtss_ifindex_t _next_port_ifindex = prev_port_ifindex ? *prev_port_ifindex : VTSS_IFINDEX_START;
    vtss_lldp_entry_index_t start_table_entry  = prev_table_entry  ? *prev_table_entry : 0;

    T_DG(TRACE_GRP_SNMP, "next_port_ifindex:%d, next_policy:%d, next_table_entry:%d, start_table_entry:%d",
         VTSS_IFINDEX_PRINTF_ARG(_next_port_ifindex), *next_policy, *next_table_entry, start_table_entry);

    do {
        T_DG(TRACE_GRP_SNMP, "_next_port_ifindex:%d, next_policy:%d, next_table_entry:%d, start_table_entry:%d",
             VTSS_IFINDEX_PRINTF_ARG(_next_port_ifindex), *next_policy, *next_table_entry, start_table_entry);

        vtss_ifindex_elm_t ife;
        VTSS_RC(vtss_ifindex_decompose(_next_port_ifindex, &ife));

        while (vtss_appl_lldp_entry_get(_next_port_ifindex, &start_table_entry, &entry, TRUE) == VTSS_RC_OK) {
            while (*next_policy < VTSS_APPL_LLDP_MED_POLICY_APPLICATIONS_CNT) {
                if (entry.policy[*next_policy].in_use) {
                    T_DG(TRACE_GRP_SNMP, "*next_policy:%d, port:%d, _next_port_ifindex:%d, next_table_entry:%d",
                         *next_policy, ife.ordinal, VTSS_IFINDEX_PRINTF_ARG(_next_port_ifindex), *next_table_entry);
                    *next_port_ifindex = _next_port_ifindex; // Policy found - return the port number
                    *next_table_entry = start_table_entry;   // Policy found - return the entry number
                    return VTSS_RC_OK;
                }
                *next_policy = *next_policy + 1; // Select next policy
            }
            start_table_entry++;  // Select next entry in the table.
            *next_policy = 0;     // Start for the first policy for the next entry:
        }

        T_DG_PORT(TRACE_GRP_SNMP, ife.ordinal, "*next_policy:%d, _next_port_ifindex:%d, next_table_entry:%d",
                  *next_policy, VTSS_IFINDEX_PRINTF_ARG(_next_port_ifindex), *next_table_entry);

        *next_table_entry = 0; // Restart table indexes
    } while (vtss_appl_iterator_ifindex_front_port_exist(&_next_port_ifindex, &_next_port_ifindex) == VTSS_RC_OK);

    return VTSS_RC_ERROR;  // Signal done.
}

// Generic iterator for policies index
mesa_rc policies_itr(const vtss_lldpmed_policy_index_t *prev_index, vtss_lldpmed_policy_index_t *next_index)
{
    if (prev_index == NULL) {
        *next_index = 0;
    } else {
        *next_index = *prev_index + 1;
    }

    // Loop until a policy that is in use is found.
    vtss_appl_lldp_cap_t cap;
    vtss_appl_lldp_cap_get(&cap);
    while (*next_index <= cap.policies_cnt) {
        vtss_appl_lldp_med_policy_t policy;
        VTSS_RC(vtss_appl_lldp_conf_policy_get(*next_index, &policy));

        if (policy.in_use) {
            T_RG(TRACE_GRP_SNMP, "Found in use for index:%d", *next_index);
            return VTSS_RC_OK;
        }

        *next_index = *next_index + 1;
        T_RG(TRACE_GRP_SNMP, "index:%d", *next_index);
    }
    return VTSS_RC_ERROR; // Signal done
}

// Dummy function because the "Write Only" OID is implemented with a TableReadWrite
mesa_rc lldp_stats_dummy_get(vtss_ifindex_t ifindex,  BOOL *const clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}
vtss_enum_descriptor_t lldpmed_ca_type_txt[] {
    {LLDPMED_CATYPE_A1       , "state"},
    {LLDPMED_CATYPE_A2       , "county"},
    {LLDPMED_CATYPE_A3       , "city"},
    {LLDPMED_CATYPE_A4       , "district"},
    {LLDPMED_CATYPE_A5       , "block"},
    {LLDPMED_CATYPE_A6       , "street"},
    {LLDPMED_CATYPE_PRD      , "leadingStreetDirection"},
    {LLDPMED_CATYPE_POD      , "trailingStreetSuffix"},
    {LLDPMED_CATYPE_STS      , "streetSuffix"},
    {LLDPMED_CATYPE_HNO      , "houseNo"},
    {LLDPMED_CATYPE_HNS      , "houseNoSuffix"},
    {LLDPMED_CATYPE_LMK      , "landmark"},
    {LLDPMED_CATYPE_LOC      , "additionalInfo"},
    {LLDPMED_CATYPE_NAM      , "name"},
    {LLDPMED_CATYPE_ZIP      , "zipCode"},
    {LLDPMED_CATYPE_BUILD    , "building"},
    {LLDPMED_CATYPE_UNIT     , "apartment"},
    {LLDPMED_CATYPE_FLR      , "floor"},
    {LLDPMED_CATYPE_ROOM     , "roomNumber"},
    {LLDPMED_CATYPE_PLACE    , "placeType"},
    {LLDPMED_CATYPE_PCN      , "postalCommunityName"},
    {LLDPMED_CATYPE_POBOX    , "poBox"},
    {LLDPMED_CATYPE_ADD_CODE , "additionalCode"},
    {0, 0}
};


vtss_enum_descriptor_t lldpmed_altitude_type_txt[] {
    {AT_TYPE_UNDEF,  "undefined"},
    {METERS,    "meters"},
    {FLOOR,     "floors"},
    {0, 0}
};

vtss_enum_descriptor_t lldpmed_datum_txt[] {
    {DATUM_UNDEF,  "undefined"},
    {WGS84,        "wgs84"},
    {NAD83_NAVD88, "nad83navd88"},
    {NAD83_MLLW,   "nad83mllw"},
    {0, 0}
};

vtss_enum_descriptor_t lldpmed_device_type_txt[] {
    {VTSS_APPL_LLDP_MED_CONNECTIVITY, "connectivity"},
    {VTSS_APPL_LLDP_MED_END_POINT,    "endpoint"},
    {0, 0}
};

vtss_enum_descriptor_t lldpmed_remote_device_type_txt[] {
    {LLDPMED_DEVICE_TYPE_NOT_DEFINED,          "notDefined"},
    {LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_I,     "endpointClassI"},
    {LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_II,    "endpointClassII"},
    {LLDPMED_DEVICE_TYPE_ENDPOINT_CLASS_III,   "endpointClassIII"},
    {LLDPMED_DEVICE_TYPE_NETWORK_CONNECTIVITY, "networkConnectivity"},
    {LLDPMED_DEVICE_TYPE_RESERVED,             "reserved"},
    {0, 0}
};


vtss_enum_descriptor_t lldpmed_network_policy_application_type_txt[] {
    {VOICE,                 "voice"},
    {VOICE_SIGNALING,       "voiceSignaling"},
    {GUEST_VOICE,           "guestVoice"},
    {GUEST_VOICE_SIGNALING, "guestVoiceSignaling"},
    {SOFTPHONE_VOICE,       "softphoneVoice"},
    {VIDEO_CONFERENCING,    "videoConferencing"},
    {STREAMING_VIDEO,       "streamingVideo"},
    {VIDEO_SIGNALING,       "videoSignaling"},
    {0, 0}
};


vtss_enum_descriptor_t vtss_lldp_admin_state_txt[] {
    {VTSS_APPL_LLDP_DISABLED,        "disabled"},
    {VTSS_APPL_LLDP_ENABLED_RX_TX,   "txAndRx"},
    {VTSS_APPL_LLDP_ENABLED_TX_ONLY, "txOnly"},
    {VTSS_APPL_LLDP_ENABLED_RX_ONLY, "rxOnly"},
    {0, 0}
};
