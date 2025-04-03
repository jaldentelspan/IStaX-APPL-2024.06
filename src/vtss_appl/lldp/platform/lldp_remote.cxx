/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "lldp_sm.h"
#include "lldp_trace.h"
#include "lldp_print.h"
#include "lldp_remote.h"
#include "lldp_api.h"
#include "lldp_tlv.h"
#include "lldp_private.h"
#include "vtss_lldp.h"
#include "misc_api.h" /* For iport2uport() */

#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"
#endif
#include "l2proto_api.h"
#ifdef VTSS_SW_OPTION_POE
#include "poe_api.h"
#endif

#ifdef VTSS_SW_OPTION_LLDP_ORG
#include "lldporg_spec_tlvs_rx.h"
#endif // VTSS_SW_OPTION_LLDP_ORG

#ifdef VTSS_SW_OPTION_EEE
#include "eee_rx.h"
#endif // VTSS_SW_OPTION_EEE

#ifdef VTSS_SW_OPTION_TSN
#include "fp_rx.h"
#endif

#ifdef VTSS_SW_OPTION_LLDP_MED
#include "lldpmed_rx.h"
#endif
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#include "icli_porting_util.h"

#include "port_api.h"

struct LLDP_mutex {
    LLDP_mutex(const char *file, int line)
    {
        vtss_appl_lldp_mutex_lock();
    }

    ~LLDP_mutex()
    {
        vtss_appl_lldp_mutex_unlock();
    }
};

#define LLDP_MUTEX_SCOPE() LLDP_mutex __lldp_mutex_guard__(__FILE__, __LINE__)
#define LLDP_MUTEX_ASSERT() vtss_appl_lldp_mutex_assert(__FILE__, __LINE__)


/* ************************************************************************ **
 *
 *
 * Defines
 *
 *
 *
 * ************************************************************************ */

#define MSAP_ID_IDX_UNKNOWN 0xFF


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif /* MIN */

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif /* MAX */

/* ************************************************************************ **
 *
 *
 * Typedefs and enums
 *
 *
 *
 * ************************************************************************ */
/* ************************************************************************ **
 *
 *
 * Prototypes for local functions
 *
 *
 *
 * ************************************************************************ */
static lldp_u8_t msap_id_idx (lldp_rx_remote_entry_t   *rx_entry);
static lldp_u8_t compare_msap_ids (lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *remote_entry);
static void update_entry (lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry);

static lldp_bool_t insert_new_entry (lldp_rx_remote_entry_t   *rx_entry);
static lldp_bool_t update_neccessary (lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry);
static lldp_8_t string_is_printable (lldp_8_t   *p, lldp_u8_t len);
static void delete_entry (lldp_rx_remote_entry_t   *rx_entry);
static void too_many_neighbors_discard_current_lldpdu (lldp_rx_remote_entry_t   *rx_entry);
static void mib_stats_inc_inserts (int entry_index);
static void mib_stats_inc_deletes (int entry_index);
static void mib_stats_inc_drops (void);
static void mib_stats_inc_ageouts (int entry_index);
static void update_entry_mib_info (vtss_appl_lldp_remote_entry_t   *entry, lldp_bool_t update_remote_idx);
static lldp_u8_t get_next(lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx);
static lldp_u8_t compare_values (lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx, vtss_appl_lldp_remote_entry_t   *entry);
static void lldp_remote_mgmt_addr_type2str(vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr, u8 mgmt_addr_index);

/* ************************************************************************ **
 *
 *
 * Public data
 *
 *
 *
 * ************************************************************************ */

/* ************************************************************************ **
 *
 *
 * Local data
 *
 *
 *
 * ************************************************************************ */

static lldp_u8_t too_many_neighbors = LLDP_FALSE;
static lldp_timer_t too_many_neighbors_timer = 0;
static CapArray<vtss_appl_lldp_remote_entry_t, VTSS_APPL_CAP_LLDP_REMOTE_ENTRY_CNT> remote_entries;
#ifdef VTSS_SW_OPTION_POE
CapArray<lldp_u8_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_ieee_draft_version;
CapArray<lldp_u8_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_ieee_poe_tlv_length;
#endif
static vtss_appl_lldp_global_counters_t lldp_mib_stats = {0};
static lldp_u32_t last_remote_index = 0;


#ifdef VTSS_SW_OPTION_POE


/*****************************************************************/
// Description: Extracting the power information from an entry
// Output: Returns the power information via pointers (type, power_source, power_priority and power value).
//         Returns 1 if power info is valid else 0.
/*****************************************************************/
lldp_bool_t lldp_remote_get_poe_power_info(const vtss_appl_lldp_remote_entry_t *entry, int *power_type, int *power_source, int *power_priority, int *power_value)
{
    if (entry->poe_info_valid_len) {
        // Get the LLDP data
        *power_type     = (entry->requested_type_source_prio >> 6) & 0x3;       // Power Type - See Table 33-22 in IEEE802.3at/D3
        *power_source   = (entry->requested_type_source_prio >> 4) & 0x3;       // Power source    - See Table 33-22 in IEEE802.3at/D3
        *power_priority = (entry->requested_type_source_prio     ) & 0x3;;      // Power priority  - See Table 33-22 in IEEE802.3at/D3
        *power_value    = lldp_remote_get_requested_power(entry->receive_port); // Power value     - See Table 33-23 in IEEE802.3at/D3
        T_DG_PORT(TRACE_GRP_RX, entry->receive_port, "TIA power_value = %d", *power_value);
        return 1;
    } else if (entry->tia_info_valid_len) {

        // Get the LLDP data
        *power_type     = (entry->tia_type_source_prio >> 6) & 0x3;    // Power Type - See Table 15 in TIA 1057
        *power_source   = (entry->tia_type_source_prio >> 4) & 0x3;    // Power source    - - See Table 16 in TIA 1057
        *power_priority = (entry->tia_type_source_prio     ) & 0x3;;   // Power priority  - See Table 17 in TIA 1057
        *power_value    = entry->tia_power;                            // Power value     - See Table 18 in TIA 1057

        T_DG_PORT(TRACE_GRP_RX, entry->receive_port, "TIA power = %d", *power_value);
        return 1;
    }

    return 0; // The power information isn't valid.
}

lldp_u8_t lldp_remote_get_ieee_poe_tlv_length(lldp_u8_t port_index)
{
    return lldp_ieee_poe_tlv_length[port_index];
}

void lldp_remote_set_ieee_poe_tlv_length(lldp_u8_t port_index, lldp_u8_t value)
{
    lldp_ieee_poe_tlv_length[port_index] = value;
}

lldp_u8_t lldp_remote_get_ieee_draft_version(lldp_u8_t port_index)
{
    return lldp_ieee_draft_version[port_index];
}

void lldp_remote_set_ieee_draft_version(lldp_u8_t port_index, lldp_u8_t value)
{
    lldp_ieee_draft_version[port_index] = value;
}
#endif


static void lldp_remote_clear_entry(lldp_u8_t entry_index)
{
#ifdef VTSS_SW_OPTION_POE
    if (remote_entries[entry_index].in_use &&
        remote_entries[entry_index].poe_info_valid_len &&
        !poe_port_lldp_disabled(remote_entries[entry_index].receive_port)) {
        T_IG_PORT(TRACE_GRP_POE, remote_entries[entry_index].receive_port, "Clearing PoE LLDP parameters");
        poe_pd_data_clear(remote_entries[entry_index].receive_port);
        lldp_remote_set_requested_power(remote_entries[entry_index].receive_port, 0);
    }
#endif

    remote_entries[entry_index].in_use = 0; // Remove the entry.
#ifdef VTSS_SW_OPTION_LLDP_MED
    remote_entries[entry_index].lldpmed_info_vld = FALSE;
#endif
#ifdef VTSS_SW_OPTION_POE
    lldp_remote_set_ieee_draft_version(remote_entries[entry_index].receive_port, VTSS_IEEE_VERSION_UNDEFINED);
    lldp_remote_set_lldp_info_valid(remote_entries[entry_index].receive_port, 0);
#endif
    mib_stats_inc_deletes(entry_index);

}

// For initializing the table. In fact it is only the in_use BOOL that is needed to be set to 0, but we set simply set everything.
mesa_rc lldp_remote_table_init(void)
{
    vtss_clear(remote_entries);

#ifdef VTSS_SW_OPTION_POE
    for (int i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) ; i++) {
        lldp_remote_set_ieee_draft_version(i, VTSS_IEEE_VERSION_UNDEFINED);
        lldp_remote_set_ieee_poe_tlv_length(i, 0);
    }
#endif
    return VTSS_RC_OK;
}

void lldp_remote_delete_entries_for_local_port (lldp_port_t port)
{
    lldp_u8_t i;
    for (i = 0; i < LLDP_REMOTE_ENTRIES; i++) {
        if (remote_entries[i].in_use && (remote_entries[i].receive_port == port)) {
            T_DG_PORT(TRACE_GRP_RX, port, "%s", "LLDP entry deleted");
            lldp_remote_clear_entry(i);
        }
    }
}

// Loops through all the entries and check if any of them are expired, and if so remove them.
void lldp_remote_1sec_timer (void)
{
    lldp_u8_t i;
    lldp_sm_t   *sm;

    vtss_isid_t             sid = VTSS_ISID_START; // 1st internal switch ID
    vtss_appl_port_status_t port_status;

    LLDP_MUTEX_SCOPE();

    for (i = 0; i < LLDP_REMOTE_ENTRIES; i++) {
        if (remote_entries[i].in_use) {
            T_NG_PORT(TRACE_GRP_RX, remote_entries[i].receive_port, "TTL = %d", remote_entries[i].rx_info_ttl);

            vtss_ifindex_t  ifindex;
            if (vtss_ifindex_from_port(sid, remote_entries[i].receive_port, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
                break;
            };

            if (vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK) {
                T_E("Could not get port configuration or port status");
                break;
            }

            if (!port_status.link) {
                T_DG_PORT(TRACE_GRP_RX, remote_entries[i].receive_port, "no link ,entry index=%d", i);
                remote_entries[i].rx_info_ttl = 1;
            }

            if (remote_entries[i].rx_info_ttl > 0) {
                if (--remote_entries[i].rx_info_ttl == 0) {
                    /* timer expired, we shall remove this entry */
                    lldp_remote_clear_entry(i);

                    mib_stats_inc_ageouts(i);
                    T_DG_PORT(TRACE_GRP_RX, remote_entries[i].receive_port, "%s", "Ageing performed for remote entry");
                    sm = lldp_get_port_sm(remote_entries[i].receive_port);
                    if (sm == NULL) {
                        T_E("Could not remove LLDP entry, i:%d, port:%d", i, remote_entries[i].receive_port);
                        continue;
                    }
                    sm->rx.rxInfoAge = LLDP_TRUE;
                    sm->stats.statsAgeoutsTotal++;
                    lldp_sm_step(sm, FALSE);
                    /* note that we do not clear tooManyNeighbors here but wait until its timer runs out */
                }
            }
        }
    }

    if (too_many_neighbors) {
        if (too_many_neighbors_timer > 0) {
            if (--too_many_neighbors_timer == 0) {
                too_many_neighbors = LLDP_FALSE;
            }
        }
    }
}

lldp_bool_t lldp_remote_handle_msap(lldp_rx_remote_entry_t   *rx_entry)
{
    lldp_u8_t msap_idx;
    LLDP_MUTEX_SCOPE();

    /*
    ** Check if we need to delete this entry (if TTL = 0)
    */
    if (rx_entry->ttl == 0) {
        T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "TTL is 0. Will delete remote entry ");
        delete_entry(rx_entry);
        return LLDP_FALSE; /* doesn't really matter what we return here */
    }
    T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "lldp_remote_handle_msap");

    /*
    ** check to see if this is an already know MSAP Identifier in our
    ** remote entries table
    */
    msap_idx = msap_id_idx(rx_entry);
    if (msap_idx == MSAP_ID_IDX_UNKNOWN) {
        T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "Inserting new remote entry");
        return insert_new_entry(rx_entry);
    } else {
        if (update_neccessary(rx_entry, &remote_entries[msap_idx])) {
            T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "Updating remote entry, msap_idx = %d", msap_idx);
            update_entry(rx_entry, &remote_entries[msap_idx]);
            update_entry_mib_info(&remote_entries[msap_idx], LLDP_FALSE);
            return LLDP_TRUE;
        } else {
            T_NG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "No update neccessary - new rxTTL=%u", (unsigned)rx_entry->ttl);
            remote_entries[msap_idx].rx_info_ttl = rx_entry->ttl;
        }
    }

    return LLDP_FALSE;
}

vtss_appl_lldp_remote_entry_t   *lldp_get_remote_entry (lldp_u8_t idx)
{
    return &remote_entries[idx];
}

vtss_appl_lldp_remote_entry_t *lldp_remote_get_entries (void)
{
    return  &remote_entries[0];
}

// See vtss/appl/lldp.h
mesa_rc oid_decode(u8 *data, u32 len, u32 *out, u32 out_max_length, u32 *outlen)
{
    out[0] = (data[0] / 40);
    out[1] = data[0] - out[0] * 40;

    uint32_t j = 2, tmp = 0;
    for (u32 i = 1; i < len; ++i) {
        if (data[i] & 0x80) {
            tmp += 128 * (data[i] & 0x7f);
        } else {
            tmp += data[i] & 0x7f;
            T_RG(TRACE_GRP_RX, "tmp:0x%X, out:0x%X, i:%d, j:%d, data:0x%X", tmp, out[j - 1], i, j, data[i]);
            if (j >= out_max_length) {
                return VTSS_RC_ERROR;
            }
            out[j++] = tmp;
            tmp = 0;
        }
        T_RG(TRACE_GRP_RX, "tmp:0x%X, out:0x%X, i:%d, j:%d, data:0x%X", tmp, out[j - 1], i, j, data[i]);

    }

    *outlen = j;
    return VTSS_RC_OK;
}

// Function for converting a oid to a character to and concatenate to string
// In : p - pointer the list of numbers.
//      len - The length of the list.
//      str - Pointer to where to put the result.
static void oid2str (lldp_8_t *p, lldp_u8_t len, lldp_8_t *str)
{
    u32 out[255];
    u32 out_len;
    u32 *out_p;
    if (oid_decode((u8 *)p, (u32)len, &out[0], 255, &out_len) != VTSS_RC_OK) {
        strcat(str, "Invalid OID");
        return;
    }

    len = out_len;
    out_p = &out[0];

    T_N("len = %d", len);
    while (len--) {
        str += sprintf(str, "%d%s", *out_p++, (len) ? "." : "");
    }
}


static void hex2str (lldp_8_t *p, lldp_u8_t len, lldp_8_t *str, u16 str_len_max)
{
    T_N("len = %d", len);
    int i = 0;
    while (len--) {
        if (i > str_len_max) {
            T_I("String length error:%d", i);
            break;
        }

        if (i == 0) {
            sprintf(&str[i], "%02X", (lldp_u8_t)*p++);
        } else {
            sprintf(&str[i], "-%02X", (lldp_u8_t)*p++);
            i++;
        }
        i += 2;
    }
}


// Converts a port number into a string. If the port is part of a LAG this is returned.
// Returns 1 if the port is part of a LAG, else return 0.
// Port number : Shall start for VTSS_PORT_NO_START
// string_ptr  : Pointer to the where to put the result
// Isid        : Switch Id - Used for checking LAG
BOOL lldp_remote_receive_port_to_string (mesa_port_no_t iport, char *string_ptr, vtss_isid_t  isid )
{
    char interface_str[80];
    string_ptr[0] = 0;
    (void) icli_port_info_txt(topo_isid2usid(isid), iport2uport(iport), interface_str);  // Get interface as printable string

#ifdef VTSS_SW_OPTION_AGGR
    mesa_glag_no_t aggr_no;
    for (aggr_no = AGGR_MGMT_GROUP_NO_START ; aggr_no < (AGGR_MGMT_GROUP_NO_START + AGGR_LLAG_CNT_); aggr_no++) {
        aggr_mgmt_group_member_t glag_members;
        // Get the port members
        (void) aggr_mgmt_members_get(isid, aggr_no, &glag_members, FALSE);

        // Loop through all port. Stop at first port that is part of the AGGR.
        if (glag_members.entry.member[iport]) {
            sprintf(string_ptr, "%s (part of LLAG%u)", interface_str, aggr_no);
            return 1;
        }
    } // End for loop
#endif // VTSS_SW_OPTION_AGGR
    sprintf(string_ptr, "%s", interface_str);
    return 0;
}

char *vtss_appl_lldp_chassis_id2string(vtss_appl_lldp_remote_entry_t *entry, char *string_ptr)
{
    string_ptr[0] = 0;
    switch (entry->chassis_id_subtype) {
    case 2: /* interface alias */
    case 3: /* port component */
    case 6: /* interface name */
    case 7: /* locally assigned */
        if (string_is_printable(entry->chassis_id, entry->chassis_id_length)) {
            T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "locally assigned is printable");
            sprintf(string_ptr, "%s", entry->chassis_id);
        } else {
            T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "locally assigned is hex");
            hex2str(entry->chassis_id, entry->chassis_id_length, string_ptr, VTSS_APPL_MAX_CHASSIS_ID_LENGTH);
        }
        break;

    case 4: /* MAC address */
        T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "MAC address");
        mac_addr2str(entry->chassis_id, string_ptr);
        break;

    case 5: /* network address */
        if (entry->chassis_id[0] == 1) { /* IANA Address Family = IPv4 */
            T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "Network address is IP");
            ip_addr2str(&entry->chassis_id[1], string_ptr);
        } else {
            T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "Network address is HEX");
            // We subtract  1 because we don't want to show the network family
            hex2str(&entry->chassis_id[1], entry->chassis_id_length - 1, string_ptr, VTSS_APPL_MAX_CHASSIS_ID_LENGTH);
        }
        break;

    /* we just show the binary (hex) data for the following */
    case 0: /* reserved */
    case 1: /* chassis component */
    default: /* reserved */
        T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "Default");
        hex2str(entry->chassis_id, entry->chassis_id_length, string_ptr, VTSS_APPL_MAX_CHASSIS_ID_LENGTH);
        break;
    }
    return string_ptr;
}

char *vtss_appl_lldp_system_name2string (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr)
{
    /*lint --e{685} */ // Note Dont want to change MIN function
    memcpy(string_ptr, entry->system_name, MIN(entry->system_name_length, VTSS_APPL_MAX_SYSTEM_NAME_LENGTH));
    string_ptr[MIN(entry->system_name_length, VTSS_APPL_MAX_SYSTEM_NAME_LENGTH - 1)] = 0;
    strcat(string_ptr, "");
    return string_ptr;
}


char *vtss_appl_lldp_system_descr2string (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr)
{
    /*lint --e{685} */ // Note Dont want to change MIN function
    memcpy(string_ptr, entry->system_description, MIN(entry->system_description_length, VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH));
    string_ptr[MIN(entry->system_description_length, VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH - 1)] = 0;
    T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "System desc = %s", string_ptr);
    return string_ptr;
}

char *vtss_appl_lldp_port_descr2string (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr)
{
    /*lint --e{685} */ // Note Dont want to change MIN function
    memcpy(string_ptr, entry->port_description, MIN(entry->port_description_length, VTSS_APPL_MAX_PORT_DESCR_LENGTH));
    string_ptr[MIN(entry->port_description_length, VTSS_APPL_MAX_PORT_DESCR_LENGTH - 1)] = 0;
    T_NG_PORT(TRACE_GRP_RX, entry->receive_port, "Port desc = %s", string_ptr);
    return string_ptr;
}

void lldp_remote_system_capa_to_string (vtss_appl_lldp_remote_entry_t   *entry, lldp_8_t *string_ptr)
{
    lldp_u16_t capa;
    lldp_u16_t capa_ena;
    lldp_u8_t bit_no;
    lldp_bool_t print_comma = LLDP_FALSE;
    const lldp_8_t *sys_capa[] = {"Other", "Repeater", "Bridge", "WLAN Access Point", "Router", "Telephone", "DOCSIS cable device", "Station Only", "Reserved"};

    string_ptr[0] = 0;
    capa     = ((lldp_u16_t)entry->system_capabilities[0]) << 8 | entry->system_capabilities[1];
    capa_ena = ((lldp_u16_t)entry->system_capabilities[2]) << 8 | entry->system_capabilities[3];

    for (bit_no = 0; bit_no <= 8; bit_no++) {
        if (capa & (1 << bit_no)) {
            if (print_comma) {
                strcat(string_ptr, ", ");
            }
            strcat(string_ptr, sys_capa[bit_no]);
            /* print enabled/disabled indicaion */
            strcat(string_ptr, "(");
            if (capa_ena & (1 << bit_no)) {
                strcat(string_ptr, "+");
            } else {
                strcat(string_ptr, "-");
            }
            strcat(string_ptr, ")");
            print_comma = LLDP_TRUE;
        }
    }
}

char *vtss_appl_lldp_port_id2string (vtss_appl_lldp_remote_entry_t   *entry, char *string_ptr)
{
    string_ptr[0] = 0;
    switch (entry->port_id_subtype) {
    case 1: /* interface Alias */
    case 2: /* port component */
    case 5: /* interface name */
    case 7: /* locally assigned */
        if (string_is_printable(entry->port_id, entry->port_id_length)) {
            strcpy(string_ptr, entry->port_id);
            string_ptr[entry->port_id_length] = 0;
            T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "port id is printable, %s, %s, %d", entry->port_id, string_ptr, entry ->port_id_length);
        } else {
            hex2str(entry->port_id, entry->port_id_length, string_ptr, VTSS_APPL_MAX_PORT_ID_LENGTH);
            T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "port id is  NOT prinfable, %s", string_ptr);
        }
        break;

    case 3: /* MAC address */
        mac_addr2str(entry->port_id, string_ptr);
        break;

    case 4: /* network address */
        if (entry->port_id[0] == 1) { /* IANA Address Family = IPv4 */
            ip_addr2str(&entry->port_id[1], string_ptr);
        } else {
            hex2str(entry->port_id, entry->port_id_length, string_ptr, VTSS_APPL_MAX_PORT_ID_LENGTH);
        }
        break;

    /* we just show the binary (hex) data for the following */
    case 0: /* reserved */
    case 6: /* agent circuit ID */
    default: /* reserved */
        hex2str(entry->port_id, entry->port_id_length, string_ptr, VTSS_APPL_MAX_PORT_ID_LENGTH);
        break;
    }

    strcat(string_ptr, "");
    return string_ptr;
}

// Converting a management address to a printable string
char *vtss_appl_lldp_mgmt_addr2string(vtss_appl_lldp_mgmt_addr_tlv_t *mgmt_addr, char *string_ptr)
{
    string_ptr[0] = 0;

    if (mgmt_addr == NULL) {
        T_E("mgmt_addr must not be NULL");
        return string_ptr;
    }

    if (mgmt_addr->length > 0 && mgmt_addr->length < sizeof(mgmt_addr->mgmt_address)) {
        T_NG(TRACE_GRP_RX, "mgmt_address_subtype:%d, mgmt_addr.oid_length:%d",
             mgmt_addr->subtype,
             mgmt_addr->oid_length);

        switch (mgmt_addr->subtype) {
        case 0: //
            hex2str(mgmt_addr->mgmt_address, mgmt_addr->length, string_ptr, VTSS_APPL_MAX_MGMT_ADDR_LENGTH);
            break;

        case 1: /* ipv4 */
            ip_addr2str(mgmt_addr->mgmt_address, string_ptr);
            break;

        case 2: /* ipv6 */
            mesa_ipv6_t ipv6;
            memcpy(ipv6.addr, mgmt_addr->mgmt_address, MIN(sizeof(ipv6.addr), VTSS_APPL_MAX_MGMT_ADDR_LENGTH));
            char ipv6_buf[40]; // misc_ipv6_txt has a fixed size of 40 characters. Making sure that we don't make a out-of-bounds access
            (void) misc_ipv6_txt(&ipv6, &ipv6_buf[0]);
            strncpy(string_ptr, ipv6_buf, VTSS_APPL_MAX_MGMT_ADDR_LENGTH);
            break;

        default:
            if (string_is_printable(mgmt_addr->mgmt_address, mgmt_addr->length)) {
                sprintf(string_ptr, "%s", mgmt_addr->mgmt_address);
            } else {
                hex2str(mgmt_addr->mgmt_address, mgmt_addr->length, string_ptr, VTSS_APPL_MAX_MGMT_ADDR_LENGTH);
            }
            break;
        }
    }
    return string_ptr;
}

// Getting the management address from an entry and returning it as a printable string
char *vtss_lldp_mgmt_addr_and_type2string(vtss_appl_lldp_remote_entry_t  *entry, const u8 mgmt_addr_index, char *string_ptr, const size_t string_ptr_len)
{
    string_ptr[0] = 0;
    char addr_string[VTSS_APPL_MAX_MGMT_ADDR_LENGTH] = "";
    char addr_type_string[VTSS_APPL_MAX_MGMT_LENGTH] = "";
    char if_subtype_string[100]                      = "";
    char oid_string[VTSS_APPL_MAX_OID_LENGTH]        = "";

    (void) vtss_appl_lldp_mgmt_addr2string(&entry->mgmt_addr[mgmt_addr_index], addr_string);

    // Add information about type
    if (entry->mgmt_addr[mgmt_addr_index].length > 0) {
        lldp_remote_mgmt_addr_type2str(entry, addr_type_string, mgmt_addr_index);
    }

    u32 if_number = (entry->mgmt_addr[mgmt_addr_index].if_number[0] << 24) +
                    (entry->mgmt_addr[mgmt_addr_index].if_number[1] << 16) +
                    (entry->mgmt_addr[mgmt_addr_index].if_number[2] << 8) +
                    (entry->mgmt_addr[mgmt_addr_index].if_number[3]);

    switch (entry->mgmt_addr[mgmt_addr_index].if_number_subtype) {
    case LLDP_MGMT_IF_SUBTYPE_IFINDEX:
        snprintf(if_subtype_string, sizeof(if_subtype_string), "- if-index:%d", if_number);
        break;

    case LLDP_MGMT_IF_SUBTYPE_SYS_PORT:
        snprintf(if_subtype_string, sizeof(if_subtype_string), "- sys-port:%d", if_number);
        break;

    case LLDP_MGMT_IF_SUBTYPE_UNKNOWN:
    default:
        // Ignoring unknown interface numbering subtypes.
        break;
    }

    if (entry->mgmt_addr[mgmt_addr_index].oid_length > 0) {
        oid2str(entry->mgmt_addr[mgmt_addr_index].oid, entry->mgmt_addr[mgmt_addr_index].oid_length, oid_string);
    }

    snprintf(string_ptr, string_ptr_len, "%s %s %s %s %s",
             addr_string,
             addr_type_string,
             if_subtype_string,
             (entry->mgmt_addr[mgmt_addr_index].oid_length > 0) ? "OID:" : "",
             oid_string);

    return string_ptr;
}


#ifdef VTSS_SW_OPTION_POE

//
// When PoE  get it's information via LLDP, we need to keep track
// of which ports that contains valid PoE information. That is done using the
// lldp_info_valid variable.
//
static CapArray<lldp_u16_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_info_valid;
lldp_u8_t lldp_remote_lldp_is_info_valid(lldp_port_t port_index)
{
    lldp_u8_t result = lldp_info_valid[port_index];
    T_NG_PORT(TRACE_GRP_POE, port_index, "lldp info valid = %d", result);
    return result;
}

void lldp_remote_set_lldp_info_valid(lldp_port_t port_index, lldp_u8_t value)
{
    T_NG_PORT(TRACE_GRP_POE, port_index, "setting lldp info valid to %d", value);
    lldp_info_valid[port_index] = value;
}



//
// When PoE  get it's information via LLDP, we need to keep track
// of the requested power for the diffrent ports . That is done using the
// lldp_requested_power variable.
//
static CapArray<lldp_u16_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_requested_power;
lldp_u16_t lldp_remote_get_requested_power(lldp_u8_t port_index)
{
    return  lldp_requested_power[port_index];
}


void lldp_remote_set_requested_power(lldp_u8_t port_index, lldp_u16_t value)
{
    lldp_requested_power[port_index] = value;
}



lldp_u16_t lldp_remote_get_allocated_power(lldp_u8_t port_index, vtss_appl_poe_port_mode_t poe_mode )
{
    lldp_u16_t result ;

    result = lldp_requested_power[port_index];

    if (poe_mode == VTSS_APPL_POE_MODE_POE) {
        if (result > 154) {
            result = 154;
        }
    }
    return result;
}


//
// Find the power information in the LLDP entries
//
lldp_bool_t lldp_remote_get_poe_power_value (lldp_u16_t port_index, lldp_u16_t *port_power )
{
    lldp_bool_t lldp_port_power_info_found = false;
    int idx;
    for (idx = 0 ; idx < LLDP_REMOTE_ENTRIES; idx++ ) {
        if (remote_entries[idx].in_use  == 0 || remote_entries[idx].receive_port != port_index) {
            continue;
        }

        if (remote_entries[idx].poe_info_valid_len) {
            lldp_port_power_info_found = TRUE;
            *port_power = remote_entries[idx].actual_power ;
        }
    }
    return lldp_port_power_info_found;
}



// Function for converting remote entry PoE information to a printable string
// In     : entry     - Pointer to the entry containing the remote neighbor information
// In/Out : str       - Pointer to where to put the output
// In     : str_len   - The maximum length for the string provided.
// In     : info_type - Which information to convert to string - 0 = PoE type, 1 = Source Type, 2 = Power, 3 = Priority
char *vtss_appl_lldp_remote_poeinfo2string(vtss_appl_lldp_remote_entry_t *entry, char *out_str, u8 str_len, u8 info_type)
{
    lldp_bool_t tlv_supported = 0; // By default we expect that the TLV isn't supported
    lldp_u8_t type_pse = 1; // By default we expect it to be pse type.
    lldp_u8_t type_source_prio = 0;
    lldp_u16_t power = 0;
    char type_str[50];
    char src_str[50];
    char power_str[50];
    char prio_str[50];

    // Clear strings to make sure to start with valid strings
    strcpy(type_str, "");
    strcpy(prio_str, "");
    strcpy(src_str, "");
    strcpy(power_str, "");

    T_DG(TRACE_GRP_POE, "LLDP_TLV_OUI_MED2 detected, poe_info_valid_len = %d, ieee_draft_version =%d, tia_info_valid_len = %d",
         entry->poe_info_valid_len, entry->ieee_draft_version, entry->tia_info_valid_len);

    int power_type     = 0;
    int power_source   = 0;
    int power_priority = 0;
    int power_value    = 0;


    if (lldp_remote_get_poe_power_info(entry, &power_type, &power_source, &power_priority, &power_value)) {
        if (entry->poe_info_valid_len &&
            (entry->ieee_draft_version == 3 || entry->ieee_draft_version == 4)) {
            tlv_supported = 1;
            // See table 33-22 in IEEE 802.3at/D3 or table 79-3a in IEEE 802.3at/D4
            if (power_type == 0x00) {
                strcpy(type_str, "Type 2 PSE Device");
            } else  if (power_type == 0x01) {
                strcpy(type_str, "Type 2 PD Device");
                type_pse = 0;
            } else  if (power_type == 0x02) {
                strcpy(type_str, "Type 1 PSE Device");
            } else {
                strcpy(type_str, "Type 1 PD Device");
                type_pse = 0;
            }
        } else if ( (entry->poe_info_valid_len && entry->ieee_draft_version == 1) ||
                    entry->tia_info_valid_len) {
            tlv_supported = 1;
            if (power_type == 0x00) {
                strcpy(type_str, "PSE Device");
            } else  if ((entry->tia_type_source_prio >> 6) == 0x01) {
                strcpy(type_str, "PD Device, ");
                type_pse = 0;
            } else {
                strcpy(type_str, "Reserved");
            }
        } else if (entry->poe_info_valid_len && entry->ieee_draft_version == 5 ) {
            tlv_supported = 1;
            if (entry->poe_info_valid_len == 29) {
                // See Table 79-6f-System setup field, IEEE Draft P802.3bt/D3.7.
                switch ((entry->system_setup & 0xE) >> 1 ) { // bits [3..1] determines power type
                case 0:
                    strcpy(type_str, "Type 3 PSE Device");
                    break;
                case 1:
                    strcpy(type_str, "Type 4 PSE Device");
                    break;
                case 2:
                    strcpy(type_str, "Type 3 single-signature PD Device");
                    type_pse = 0;
                    break;
                case 3:
                    strcpy(type_str, "Type 3 dual-signature PD Device");
                    type_pse = 0;
                    break;
                case 4:
                    strcpy(type_str, "Type 4 single-signature PD Device");
                    type_pse = 0;
                    break;
                case 5:
                    strcpy(type_str, "Type 4 dual-signature PD");
                    type_pse = 0;
                    break;
                case 6: // fall through
                case 7:
                    strcpy(type_str, "Reserved");
                    break;
                }
            } else if (entry->poe_info_valid_len == 12) { // len=12 -> tlv does not contain Type3/4 extensions
                if (power_type == 0x00) {
                    strcpy(type_str, "Type 2 PSE Device");
                } else  if (power_type == 0x01) {
                    strcpy(type_str, "Type 2 PD Device");
                    type_pse = 0;
                } else  if (power_type == 0x02) {
                    strcpy(type_str, "Type 1 PSE Device");
                } else {
                    strcpy(type_str, "Type 1 PD Device");
                    type_pse = 0;
                }
            } else if (entry->poe_info_valid_len == 7) { // len=7 -> the device is not powered
                // See Table 79-3-MDI power capabilities/status support field, IEEE Draft P802.3bt/D3.7
                // bit [0] decides if port class is PSE=1 or PD=0
                if (entry->poe_mdi_power_support & 0x1 ) {
                    strcpy(type_str, "PSE Device");
                } else {
                    strcpy(type_str, "PD Device, ");
                    type_pse = 0;
                }
            }
        }

        // Return an empty string if the TLV isn't supported
        if (!tlv_supported) {
            T_RG_PORT(TRACE_GRP_POE, entry->receive_port, "Unsupported organizationally TLV, %d", tlv_supported);
        } else {
            // Update the string
            T_RG_PORT(TRACE_GRP_POE, entry->receive_port, "Updated new PoE information, type_source_prio = %d,power = %d, type_pse = %d",
                      type_source_prio, power, type_pse );

            T_DG(TRACE_GRP_POE, "Power Source  = 0x%X", power_source);
            // See TIA 1057 - table 16 / table 33-22 in IEEE 802.3at/D3
            if (type_pse == 1 ) {
                if (power_source == 0x00) {
                    strcpy(src_str, "Unknown Power Source");
                } else  if (power_source == 0x01) {
                    strcpy(src_str, "Primary Power Source");
                } else  if (power_source == 0x02) {
                    strcpy(src_str, "Backup Power Source");
                } else {
                    strcpy(src_str, "Power Source Reserved");
                }
            } else {
                // This is a PD type
                if (power_source == 0x00) {
                    strcpy(src_str, "Unknown");
                } else  if (power_source == 0x01) {
                    strcpy(src_str, "PSE");
                } else  if (power_source == 0x02) {
                    strcpy(src_str, "Local");
                } else {
                    strcpy(src_str, "PSE and local");
                }
            }

            // See table 33-22 in IEEE 802.3at/D3 / TIA 1057 -table 17
            if (power_priority == 0x03) {
                strcpy(prio_str, "Low Priority");
            } else  if (power_priority == 0x01) {
                strcpy(prio_str, "Critial Priority");
            } else  if (power_priority == 0x02) {
                strcpy(prio_str, "High Priority");
            } else  {
                strcpy(prio_str, "Unknown Priority");
            }

            // See table 33-23 in IEEE 802.3at/D3 / TIA 1057 table 18
            T_NG(TRACE_GRP_POE, "power_value = 0x%X", power_value);

            strcpy(power_str, one_digi_float2str(power_value, &power_str[0]));
            strcat(power_str, " [W]");
        }
    }

    switch (info_type) {
    case 0:
        misc_strncpyz(out_str, type_str, str_len);
        break;
    case 1:
        misc_strncpyz(out_str, src_str, str_len);
        break;
    case 2:
        misc_strncpyz(out_str, power_str, str_len);
        break;
    case 3:
        misc_strncpyz(out_str, prio_str, str_len);
        break;
    default:
        strcpy(out_str, ""); // Clear string
    }

    return out_str;
}
#endif //VTSS_SW_OPTION_POE

static void characters2str (lldp_8_t   *p, lldp_u8_t len, lldp_8_t *output_string)
{
    char ch[2];
    strcpy(output_string, "");
    while (len--) {
        if (p != NULL) {
            sprintf(ch, "%c", *p++);
            strcat(output_string, ch);
        }
    }
}

static void hex_value2str (lldp_8_t   *p, lldp_u8_t len, lldp_8_t *output_string)
{
    T_N("len = %d", len);
    strcpy(output_string, "");
    while (len--) {
        if (p != NULL) {
            sprintf(output_string, "%02X ", *p);
            p++;
        }
    }
}

void lldp_remote_tlv_to_string (vtss_appl_lldp_remote_entry_t   *entry, const lldp_tlv_t field, lldp_8_t *output_string, const u32 output_string_len, const u8 mgmt_addr_index)
{
    /*lint --e{685} */ // Note Dont want to change MIN function
    lldp_8_t   *p = 0;
    lldp_u8_t len = 0;
    strcpy(output_string, ""); // Clear the string

    switch (field) {
    case LLDP_TLV_BASIC_MGMT_CHASSIS_ID:
        T_NG_PORT(TRACE_GRP_POE, entry->receive_port, "%s", "LLDP_TLV_BASIC_MGMT_CHASSIS_ID");
        (void) vtss_appl_lldp_chassis_id2string(entry, output_string);
        break;

    case LLDP_TLV_BASIC_MGMT_PORT_ID:
        (void) vtss_appl_lldp_port_id2string(entry, output_string);
        break;

    case LLDP_TLV_BASIC_MGMT_PORT_DESCR:
        len = MIN(entry->port_description_length, VTSS_APPL_MAX_PORT_DESCR_LENGTH);
        p = entry->port_description;
        break;

    case LLDP_TLV_BASIC_MGMT_SYSTEM_NAME:
        len = MIN(entry->system_name_length, VTSS_APPL_MAX_SYSTEM_NAME_LENGTH);
        p = entry->system_name;
        break;

    case LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR:
        len = MIN(entry->system_description_length, VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH);
        p = entry->system_description;
        break;

    case LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA:
        (void) lldp_remote_system_capa_to_string(entry, output_string);
        break;

    case LLDP_TLV_BASIC_MGMT_MGMT_ADDR:
        (void) vtss_lldp_mgmt_addr_and_type2string(entry, mgmt_addr_index, output_string, output_string_len);
        break;

    case LLDP_TLV_ORG_TLV:
        break;

    default:
        break;
    }

    if (len > 0) {
        if (string_is_printable(p, len)) {
            characters2str(p, len, output_string);
        } else {
            hex_value2str(p, len, output_string);
        }
    }
}

void lldp_chassis_type_to_string (vtss_appl_lldp_remote_entry_t   *entry, lldp_printf_t lldp_printf)
{
    switch (entry->chassis_id_subtype) {
    case 1: /* entPhyClass = port(10) | backplane(4) */
    case 3: /* entPhyClass = chassis(3) */
        (void) lldp_printf("EntPhysicalAlias");
        break;
    case 2:
        (void) lldp_printf("ifAlias");
        break;
    case 4:
        (void) lldp_printf("MAC-address");
        break;
    case 5:
        (void) lldp_printf("Network Address");
        break;
    case 6:
        (void) lldp_printf("ifName");
        break;
    case 7:
        (void) lldp_printf("local");
        break;
    }
}

void lldp_port_type_to_string (vtss_appl_lldp_remote_entry_t   *entry, lldp_printf_t lldp_printf)
{
    switch (entry->port_id_subtype) {
    case 1:
        (void) lldp_printf("ifAlias");
        break;
    case 2:
        (void) lldp_printf("entPhysicalAlias");
        break;
    case 3:
        (void) lldp_printf("MAC-address");
        break;
    case 4:
        (void) lldp_printf("Network Address");
        break;
    case 5:
        (void) lldp_printf("ifName");
        break;
    case 6:
        (void) lldp_printf("Agent circuit ID");
        break;
    case 7:
        (void) lldp_printf("local");
        break;
    }
}

void lldp_get_mib_stats (vtss_appl_lldp_global_counters_t   *stat)
{
    *stat = lldp_mib_stats;
    T_N("lldp_mib_stats.table_inserts = %d", lldp_mib_stats.table_inserts);
}

vtss_appl_lldp_remote_entry_t   *lldp_remote_get(lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx)
{
    lldp_u8_t i;

    /* run through all entries */
    for (i = 0; i < LLDP_REMOTE_ENTRIES; i++) {
        if (remote_entries[i].in_use) {
            /* if entry is equal to  what user supplied */
            if (compare_values(time_mark, port, remote_idx, &remote_entries[i]) == 2) {
                return &remote_entries[i];
            }
        }
    }
    return NULL;
}

vtss_appl_lldp_remote_entry_t   *lldp_remote_get_next(lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx)
{
    lldp_u8_t idx;

    idx = get_next(time_mark, port, remote_idx);
    if (idx == 0xFF) {
        /* no more entries larger than current */
        return NULL;
    } else {
        return &remote_entries[idx];
    }
}

vtss_appl_lldp_remote_entry_t   *lldp_remote_get_next_non_zero_addr (lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx, u8 mgmt_addr_index)
{
    lldp_u8_t idx;

    for (;;) {
        idx = get_next(time_mark, port, remote_idx);
        /* if no more entries, get out */
        if (idx == 0xFF) {
            break;
        }

        /* if we found something with a management address */
        if (remote_entries[idx].mgmt_addr[mgmt_addr_index].length != 0) {
            break;
        }

        /* otherwise, try to proceed, starting with this entry without management address  */
        time_mark = remote_entries[idx].time_mark;
        port = remote_entries[idx].receive_port;
        remote_idx = remote_entries[idx].lldp_remote_index;
    };

    if (idx == 0xFF) {
        /* no more entries larger than current */
        return NULL;
    } else {
        return &remote_entries[idx];
    }
}




static void lldp_remote_mgmt_addr_type2str(vtss_appl_lldp_remote_entry_t *entry, char *string_ptr, u8 mgmt_addr_index)
{
    /* from http://www.iana.org/assignments/address-family-numbers
       Number    Description                                          Reference
       ------    ---------------------------------------------------- ---------
       0    Reserved
       1    IP (IP version 4)
       2    IP6 (IP version 6)
       3    NSAP
       4    HDLC (8-bit multidrop)
       5    BBN 1822
       6    802 (includes all 802 media plus Ethernet "canonical format")
       7    E.163
       8    E.164 (SMDS, Frame Relay, ATM)
       9    F.69 (Telex)
       10    X.121 (X.25, Frame Relay)
       11    IPX
       12    Appletalk
       13    Decnet IV
       14    Banyan Vines
       15    E.164 with NSAP format subaddress           [UNI-3.1] [Malis]
       16    DNS (Domain Name System)
       17    Distinguished Name                                    [Lynn]
       18    AS Number                                             [Lynn]
       19    XTP over IP version 4                                 [Saul]
       20    XTP over IP version 6                                 [Saul]
       21    XTP native mode XTP                                   [Saul]
       22    Fibre Channel World-Wide Port Name                   [Bakke]
       23    Fibre Channel World-Wide Node Name                   [Bakke]
       24    GWID                                                 [Hegde]
       65535    Reserved
    */
    /* we support the following subset of these... */
    switch (entry->mgmt_addr[mgmt_addr_index].subtype) {
    case 1:
        sprintf(string_ptr, "%s", "(IPv4)");
        break;
    case 2:
        sprintf(string_ptr, "%s", "(IPv6)");
        break;
    case 3:
        sprintf(string_ptr, "%s", "(NSAP)");
        break;
    case 6:
        sprintf(string_ptr, "%s", "(802)");
        break;
    case 11:
        sprintf(string_ptr, "%s", "(IPX)");
        break;
    case 12:
        sprintf(string_ptr, "%s", "(Appletalk)");
        break;
    case 16:
        sprintf(string_ptr, "%s", "(DNS)");
        break;
    case 17:
        sprintf(string_ptr, "%s", "(Distinguished Name)");
        break;
    default:
        sprintf(string_ptr, "%s", "(Other)");
        break;
    }

}

static lldp_8_t string_is_printable (lldp_8_t   *p, lldp_u8_t len)
{
    while (len--) {
        if (p != NULL) {
            //Must be a valid ASCII charater or LF ( LF = 0x0A ) or CR ( CR = 0x0D )
            if (((*p > 31) && (*p < 127)) || (*p == 0x0A) || (*p == 0x00) || (*p == 0x0D) ) {

                // Replace LF and CR with Space
                if ((*p == 0x0A) || (*p == 0x00) || (*p == 0x0D)) {
                    *p = 0x20;
                }

                p++;
            } else {
                return LLDP_FALSE;
            }
        }
    }
    return LLDP_TRUE;
}

/* returns 0 if msap identifiers are equal, 1 otherwise */
static lldp_u8_t compare_msap_ids (lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *remote_entry)
{
    /* perform simple tests prior to doing memory compare */
    if (rx_entry->chassis_id_subtype != remote_entry->chassis_id_subtype) {
        return 1;
    }

    if (rx_entry->chassis_id_length != remote_entry->chassis_id_length) {
        return 1;
    }

    if (rx_entry->port_id_subtype != remote_entry->port_id_subtype) {
        return 1;
    }

    if (rx_entry->port_id_length != remote_entry->port_id_length) {
        return 1;
    }


    if (memcmp(rx_entry->chassis_id, remote_entry->chassis_id, rx_entry->chassis_id_length) != 0) {
        return 1;
    }

    if (memcmp(rx_entry->port_id, remote_entry->port_id, rx_entry->port_id_length) != 0) {
        return 1;
    }

    return 0;
}

static lldp_u8_t msap_id_idx (lldp_rx_remote_entry_t   *rx_entry)
{
    lldp_u8_t i;
    for (i = 0; i < LLDP_REMOTE_ENTRIES; i++) {
        if (remote_entries[i].in_use) {
            if (compare_msap_ids(rx_entry, &remote_entries[i]) == 0) {
                T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "Found MSAP identifier in index %u", (unsigned)i);
                return i;
            }
        }
    }
    return MSAP_ID_IDX_UNKNOWN;
}


static lldp_bool_t insert_new_entry (lldp_rx_remote_entry_t   *rx_entry)
{
    lldp_u8_t i;
    LLDP_MUTEX_ASSERT();

    for (i = 0; i < LLDP_REMOTE_ENTRIES; i++) {
        if (!remote_entries[i].in_use) {
            remote_entries[i].entry_index = i;
            update_entry(rx_entry, &remote_entries[i]);
            update_entry_mib_info(&remote_entries[i], LLDP_TRUE);
            mib_stats_inc_inserts(i);
            return LLDP_TRUE;
        }
    }

    /* no room */

    /* for now we just discard the new LLDPDU */
    too_many_neighbors_discard_current_lldpdu(rx_entry);
    mib_stats_inc_drops();

    return LLDP_FALSE;
}

static void update_entry_mib_info (vtss_appl_lldp_remote_entry_t   *entry, lldp_bool_t update_remote_idx)
{
    if (update_remote_idx) {
        entry->lldp_remote_index = ++last_remote_index;
    }
    entry->time_mark = lldp_os_get_sys_up_time();

    // From 802.AB-2005 Page 70
    // "The value of sysUpTime object (defined in IETF RFC 3418)
    // at the time an entry is created, modified, or deleted in the
    // in tables associated with the lldpRemoteSystemsData objects
    // and all LLDP extension objects associated with remote systems.
    lldp_entry_changed(entry);
}

static void too_many_neighbors_discard_current_lldpdu (lldp_rx_remote_entry_t   *rx_entry)
{
    lldp_sm_t   *sm;

    too_many_neighbors = LLDP_TRUE;
    too_many_neighbors_timer = MAX(too_many_neighbors_timer, rx_entry->ttl);

    sm = lldp_get_port_sm(rx_entry->receive_port);
    if (sm == NULL) {
        T_E("Could not get state machine, port:%d", rx_entry->receive_port);
        return;
    }

    sm->stats.statsFramesDiscardedTotal++;
}


static void update_entry (lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry)

{
    u8 mgmt_addr_index;
    LLDP_MUTEX_ASSERT();

    /*lint --e{685} */ // Note Dont want to change MIN function
    entry->receive_port = rx_entry->receive_port;
    memcpy(entry->smac, rx_entry->smac, 6);

    entry->chassis_id_subtype = rx_entry->chassis_id_subtype;
    entry->chassis_id_length = rx_entry->chassis_id_length;
    memcpy(entry->chassis_id, rx_entry->chassis_id, MIN(rx_entry->chassis_id_length, VTSS_APPL_MAX_CHASSIS_ID_LENGTH));

    entry->port_id_subtype = rx_entry->port_id_subtype;
    entry->port_id_length = rx_entry->port_id_length;

    memcpy(entry->port_id, rx_entry->port_id, MIN(rx_entry->port_id_length, VTSS_APPL_MAX_PORT_ID_LENGTH));
    entry->port_description_length = rx_entry->port_description_length;
    memcpy(entry->port_description, rx_entry->port_description, MIN(rx_entry->port_description_length, VTSS_APPL_MAX_PORT_DESCR_LENGTH));

    entry->system_name_length = rx_entry->system_name_length;
    memcpy(entry->system_name, rx_entry->system_name, MIN(rx_entry->system_name_length, VTSS_APPL_MAX_SYSTEM_NAME_LENGTH));

    entry->system_description_length = rx_entry->system_description_length;
    memcpy(entry->system_description, rx_entry->system_description, MIN(rx_entry->system_description_length, VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH));

    memcpy(entry->system_capabilities, rx_entry->system_capabilities, sizeof(entry->system_capabilities));

    // Make sure that management address is given (length > 0) to make sure that we don't make wild
    // pointer accesses


    for (mgmt_addr_index = 0; mgmt_addr_index < LLDP_MGMT_ADDR_CNT; mgmt_addr_index++) {
        T_RG_PORT(TRACE_GRP_RX, entry->receive_port, "rx_entry->mgmt_addr[%d].length:%d", mgmt_addr_index, rx_entry->mgmt_addr[mgmt_addr_index].length);
        if (rx_entry->mgmt_addr[mgmt_addr_index].length > 0 ) {
            entry->mgmt_addr[mgmt_addr_index].subtype = rx_entry->mgmt_addr[mgmt_addr_index].subtype;
            entry->mgmt_addr[mgmt_addr_index].length = rx_entry->mgmt_addr[mgmt_addr_index].length;
            memcpy(entry->mgmt_addr[mgmt_addr_index].mgmt_address, rx_entry->mgmt_addr[mgmt_addr_index].mgmt_address, MIN(rx_entry->mgmt_addr[mgmt_addr_index].length, VTSS_APPL_MAX_MGMT_ADDR_LENGTH));
            entry->mgmt_addr[mgmt_addr_index].if_number_subtype = rx_entry->mgmt_addr[mgmt_addr_index].if_number_subtype;

            memcpy(entry->mgmt_addr[mgmt_addr_index].if_number, rx_entry->mgmt_addr[mgmt_addr_index].if_number, sizeof(entry->mgmt_addr[mgmt_addr_index].if_number));

            entry->mgmt_addr[mgmt_addr_index].oid_length = rx_entry->mgmt_addr[mgmt_addr_index].oid_length;
            memcpy(entry->mgmt_addr[mgmt_addr_index].oid, rx_entry->mgmt_addr[mgmt_addr_index].oid, MIN(rx_entry->mgmt_addr[mgmt_addr_index].oid_length, VTSS_APPL_MAX_OID_LENGTH));
        } else {
            memset(&entry->mgmt_addr[mgmt_addr_index], 0, sizeof(entry->mgmt_addr[mgmt_addr_index]));
        }
        T_RG(TRACE_GRP_RX, "rx_entry->mgmt_addr[mgmt_addr_index].oid_length:%d, mgmt_addr_index:%d", rx_entry->mgmt_addr[mgmt_addr_index].oid_length, mgmt_addr_index);
    }

#ifdef VTSS_SW_OPTION_LLDP_MED
    lldpmed_update_entry(rx_entry, entry);
#endif // VTSS_SW_OPTION_LLDP_MED

#ifdef VTSS_SW_OPTION_LLDP_ORG
    lldporg_update_entry(rx_entry, entry);
#endif // VTSS_SW_OPTION_LLDP_ORG

#ifdef VTSS_SW_OPTION_EEE
    eee_update_entry(rx_entry, entry);
#endif // VTSS_SW_OPTION_EEE

#ifdef VTSS_SW_OPTION_TSN
    if (lldp_is_frame_preemption_supported()) {
        fp_update_entry(rx_entry, entry);
    }
#endif // VTSS_SW_OPTION_TSN

    // organizationally TLV for PoE
#ifdef VTSS_SW_OPTION_POE
    entry->poe_info_valid_len         = rx_entry->poe_info_valid_len;
    entry->ieee_draft_version         = rx_entry->ieee_draft_version;
    entry->actual_power               = rx_entry->actual_power;
    entry->actual_type_source_prio    = rx_entry->actual_type_source_prio;
    entry->requested_power            = rx_entry->requested_power;
    entry->requested_type_source_prio = rx_entry->requested_type_source_prio;
    entry->tia_info_valid_len         = rx_entry->tia_info_valid_len;
    entry->tia_power                  = rx_entry->tia_power;
    entry->tia_type_source_prio       = rx_entry->tia_type_source_prio;
    entry->poe_mdi_power_support      = rx_entry->poe_mdi_power_support;
    entry->poe_pse_power_pair         = rx_entry->poe_pse_power_pair;
    entry->poe_power_class            = rx_entry->poe_power_class;
    entry->requested_power_mode_a     = rx_entry->requested_power_mode_a;
    entry->requested_power_mode_b     = rx_entry->requested_power_mode_b;
    entry->pse_alloc_power_alt_a      = rx_entry->pse_alloc_power_alt_a;
    entry->pse_alloc_power_alt_b      = rx_entry->pse_alloc_power_alt_b;
    entry->power_status               = rx_entry->power_status;
    entry->system_setup               = rx_entry->system_setup;
    entry->pse_maximum_avail_power    = rx_entry->pse_maximum_avail_power;
    entry->auto_class                 = rx_entry->auto_class;
    entry->power_down                 = rx_entry->power_down;

    T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "LLDP tia_len %u poe_len %u ieee_version %u", entry->tia_info_valid_len, entry->poe_info_valid_len, entry->ieee_draft_version );

    // If poe port is lldp disabled, then do NOT let the incomming LLDP PDU have any effect on the PoE controller settings,
    // i.e. doe not call poe_pd_data_set, poe_pd_bt_data_set or lldp_poe_fast_start
    if ((!poe_port_lldp_disabled(rx_entry->receive_port)) && (is_poe_ready())) {
        int power_type     = 0;
        int power_source   = 0;
        int power_priority = 0;
        int power_value    = 0;
        bool fast_start = false;
        lldp_u16_t requested_power;

        if (rx_entry->tia_info_valid_len) {
            requested_power =  rx_entry->tia_power;
            if (!lldp_remote_get_poe_power_info(entry, &power_type, &power_source, &power_priority, &power_value)) {
                T_I("LLDP information not valid - All values remain 0");
            }
            if ( power_type == 0) {
                // This is from a PSE device, so do not adjust pd data
            } else {
                poe_pd_data_t pd_data;
                pd_data.type                = MEBA_POE_PORT_PD_POWER_PRIORITY_LOW;
                pd_data.pd_requested_power_dw  = entry->tia_power;
                pd_data.pse_allocated_power_dw = entry->tia_power;
                poe_pd_data_set(rx_entry->receive_port, &pd_data);
            }
        } else {
            requested_power =  rx_entry->requested_power;
        }

        lldp_remote_set_requested_power(rx_entry->receive_port, requested_power);

        if (entry->poe_info_valid_len && entry->ieee_draft_version < 5) {
            // Tell the PoE module the LLDP PoE configuration for TLV type 1 & 2
            if (!lldp_remote_get_poe_power_info(entry, &power_type, &power_source, &power_priority, &power_value)) {
                T_I("LLDP information not valid - All values remain 0");
            }
            if ( power_type == 0 || power_type == 2 ) {
                // This is from a PSE device, so do not adjust pd data
            } else {
                poe_pd_data_t pd_data;
                pd_data.type                = (power_type << 6) | (power_source << 4) | power_priority;
                pd_data.pd_requested_power_dw  = entry->requested_power;
                pd_data.pse_allocated_power_dw = entry->requested_power;
                poe_pd_data_set(rx_entry->receive_port, &pd_data);
            }
            T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "New PoE information rx");
            fast_start = true;
        } else if (entry->poe_info_valid_len ==  29 && entry->ieee_draft_version == 5) {
            T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "Received 903.2bt LLDP message with length 29");
            if (fast_cap(MEBA_CAP_POE_BT)) {
                // Tell the PoE module the LLDP PoE configuration for TLV type 3 & 4
                poe_pd_bt_data_t pd_bt_data;
                pd_bt_data.type                      = entry->requested_type_source_prio;
                pd_bt_data.pd_requested_power_dw     = entry->requested_power;
                pd_bt_data.pse_allocated_power_dw    = entry->requested_power;
                pd_bt_data.requested_power_mode_a_dw = entry->requested_power_mode_a;
                pd_bt_data.requested_power_mode_b_dw = entry->requested_power_mode_b;
                pd_bt_data.pse_alloc_power_alt_a_dw  = entry->pse_alloc_power_alt_a;
                pd_bt_data.pse_alloc_power_alt_b_dw  = entry->pse_alloc_power_alt_b;
                pd_bt_data.power_status              = entry->power_status;
                pd_bt_data.system_setup              = entry->system_setup;
                pd_bt_data.pse_maximum_avail_power   = entry->pse_maximum_avail_power;
                pd_bt_data.auto_class                = entry->auto_class;
                pd_bt_data.power_down                = entry->power_down;
                poe_pd_bt_data_set(rx_entry->receive_port, &pd_bt_data);

                T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "requested_power=%d requested_power_mode_a=%d requested_power_mode_b=%d", entry->requested_power, entry->requested_power_mode_a, entry->requested_power_mode_b);

                // in case its DSPD lets sum the requested power = reqPwr altA + reqPwr altB
                poe_status_t poe_status;
                poe_mgmt_get_status(&poe_status);
                vtss_appl_poe_port_status_t ps = poe_status.port_status[rx_entry->receive_port];
                if (ps.pd_type_sspd_dspd == 2) { // DSPD
                    lldp_remote_set_requested_power(rx_entry->receive_port, pd_bt_data.requested_power_mode_a_dw + pd_bt_data.requested_power_mode_b_dw);
                }
            } else {
                // PSE does not support BT standard, i.e. does not support Type 3 and Type 4 devices.
                poe_pd_data_t pd_data;
                pd_data.type                   = (power_type << 6) | (power_source << 4) | power_priority;
                pd_data.pd_requested_power_dw  = entry->requested_power;
                pd_data.pse_allocated_power_dw = entry->requested_power;
                poe_pd_data_set(rx_entry->receive_port, &pd_data);
                T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "entry->requested_power=%d", entry->requested_power);
            }
            fast_start = true;
        } else if (entry->poe_info_valid_len ==  12 && entry->ieee_draft_version == 5) {
            T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "Received 903.2bt LLDP message with length 12");
            // Tell the PoE module the LLDP PoE configuration for TLV type 1 & 2
            if (!lldp_remote_get_poe_power_info(entry, &power_type, &power_source, &power_priority, &power_value)) {
                T_I("LLDP information not valid - All values remain 0");
            }
            if ( power_type == 0 || power_type == 2 ) {
                // This is from a PSE device, so do not adjust pd data
            } else {
                poe_pd_data_t pd_data;
                pd_data.type                   = (power_type << 6) | (power_source << 4) | power_priority;
                pd_data.pd_requested_power_dw  = entry->requested_power;
                pd_data.pse_allocated_power_dw = entry->requested_power;
                poe_pd_data_set(rx_entry->receive_port, &pd_data);
                T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "entry->requested_power=%d", entry->requested_power);
            }
            T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "New PoE information rx");
            fast_start = true;
        }

        if (fast_start) {
            // Signal to transmit LLDP frame back to the PD
            (void) lldp_poe_fast_start(entry->receive_port, FALSE);
        }
    } else {
        T_IG_PORT(TRACE_GRP_RX, entry->receive_port, "Poe port is lldp disabled, and therefore "
                  "the received LLDP PDU has no effect on the PoE controller setting");
    }
    lldp_remote_set_lldp_info_valid(rx_entry->receive_port, rx_entry->poe_info_valid_len || rx_entry->tia_info_valid_len );
    lldp_remote_set_ieee_draft_version(rx_entry->receive_port, rx_entry->ieee_draft_version);
    lldp_remote_set_ieee_poe_tlv_length(rx_entry->receive_port, rx_entry->poe_info_valid_len);
#endif // VTSS_SW_OPTION_POE

    // Entry update
    entry->in_use = 1;
    entry->rx_info_ttl = rx_entry->ttl;

    T_DG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "Entry update");
}


static lldp_bool_t update_neccessary(lldp_rx_remote_entry_t   *rx_entry, vtss_appl_lldp_remote_entry_t   *entry)
{
    u8 mgmt_addr_index;
    /* we don't need to check the MSAP id here, we already know these are identical */

    /* we check simple variables first */
    if (rx_entry->receive_port != entry->receive_port) {
        T_D("- Receive port");
        return LLDP_TRUE;
    }

    if (rx_entry->port_description_length != entry->port_description_length) {
        T_D("- Port descr. len");
        return LLDP_TRUE;
    }

    if (rx_entry->system_name_length != entry->system_name_length) {
        T_D("- Sys name len");
        return LLDP_TRUE;
    }

    if (rx_entry->system_description_length != entry->system_description_length) {
        T_D("- Sys descr len");
        return LLDP_TRUE;
    }

    for (mgmt_addr_index = 0; mgmt_addr_index < LLDP_MGMT_ADDR_CNT; mgmt_addr_index++) {
        if (rx_entry->mgmt_addr[mgmt_addr_index].subtype != entry->mgmt_addr[mgmt_addr_index].subtype) {
            T_D("- mgmt addr subtype");
            return LLDP_TRUE;
        }

        if (rx_entry->mgmt_addr[mgmt_addr_index].length != entry->mgmt_addr[mgmt_addr_index].length) {
            T_D("- mgmt addr len");
            return LLDP_TRUE;
        }

        if (rx_entry->mgmt_addr[mgmt_addr_index].if_number_subtype != entry->mgmt_addr[mgmt_addr_index].if_number_subtype) {
            T_D("- mgmt addr ifnnumber subtype");
            return LLDP_TRUE;
        }

        if (rx_entry->mgmt_addr[mgmt_addr_index].oid_length != entry->mgmt_addr[mgmt_addr_index].oid_length) {
            T_D("- oid len");
            return LLDP_TRUE;
        }

        if (memcmp(rx_entry->mgmt_addr[mgmt_addr_index].mgmt_address, entry->mgmt_addr[mgmt_addr_index].mgmt_address, rx_entry->mgmt_addr[mgmt_addr_index].length) != 0) {
            T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "- mgmt addr");
            return LLDP_TRUE;
        }

        if (rx_entry->mgmt_addr[mgmt_addr_index].length > 0) {
            if (memcmp(rx_entry->mgmt_addr[mgmt_addr_index].if_number, entry->mgmt_addr[mgmt_addr_index].if_number, sizeof(entry->mgmt_addr[mgmt_addr_index].if_number)) != 0) {
                T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "- mgmt addr ifnum");
                return LLDP_TRUE;
            }
        }

        if (memcmp(rx_entry->mgmt_addr[mgmt_addr_index].oid, entry->mgmt_addr[mgmt_addr_index].oid, rx_entry->mgmt_addr[mgmt_addr_index].oid_length) != 0) {
            T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "- oid");
            return LLDP_TRUE;
        }
    }

    /* now check with memcmps */
    if (memcmp(rx_entry->port_description, entry->port_description, rx_entry->port_description_length) != 0) {
        T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "- port descr");
        return LLDP_TRUE;
    }

    if (memcmp(rx_entry->system_name, entry->system_name, rx_entry->system_name_length) != 0) {
        T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "- sys name");
        return LLDP_TRUE;
    }

    if (memcmp(rx_entry->system_description, entry->system_description, rx_entry->system_description_length) != 0) {
        T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "- sys descr");
        return LLDP_TRUE;
    }

    if (memcmp(rx_entry->system_capabilities, entry->system_capabilities, sizeof(entry->system_capabilities)) != 0) {
        T_DG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "- sys capa");
        return LLDP_TRUE;
    }

#ifdef VTSS_SW_OPTION_POE
    if (rx_entry->requested_type_source_prio != entry->requested_type_source_prio ||
        rx_entry->poe_info_valid_len         != entry->poe_info_valid_len ||
        rx_entry->requested_power            != entry->requested_power ||
        rx_entry->tia_type_source_prio       != entry->tia_type_source_prio ||
        rx_entry->tia_power                  != entry->tia_power ||
        rx_entry->tia_info_valid_len         != entry->tia_info_valid_len ||
        rx_entry->actual_type_source_prio    != entry->actual_type_source_prio ||
        rx_entry->ieee_draft_version         != entry->ieee_draft_version ||
        rx_entry->actual_power               != entry->actual_power ||
        rx_entry->poe_mdi_power_support      != entry->poe_mdi_power_support ||
        rx_entry->poe_pse_power_pair         != entry->poe_pse_power_pair ||
        rx_entry->poe_power_class            != entry->poe_power_class ||
        rx_entry->requested_power_mode_a     != entry->requested_power_mode_a ||
        rx_entry->requested_power_mode_b     != entry->requested_power_mode_b ||
        rx_entry->pse_alloc_power_alt_a      != entry->pse_alloc_power_alt_a ||
        rx_entry->pse_alloc_power_alt_b      != entry->pse_alloc_power_alt_b ||
        rx_entry->power_status               != entry->power_status ||
        rx_entry->system_setup               != entry->system_setup ||
        rx_entry->pse_maximum_avail_power    != entry->pse_maximum_avail_power ||
        rx_entry->auto_class                 != entry->auto_class ||
        rx_entry->power_down                 != entry->power_down) {
        return LLDP_TRUE;
    }
#endif // VTSS_SW_OPTION_POE


#ifdef VTSS_SW_OPTION_LLDP_MED
    if (lldpmed_update_neccessary(rx_entry, entry)) {
        return LLDP_TRUE;
    }
#endif

#ifdef VTSS_SW_OPTION_LLDP_ORG
    if (lldporg_update_neccessary(rx_entry, entry)) {
        return LLDP_TRUE;
    }
#endif

#ifdef VTSS_SW_OPTION_EEE
    if (eee_update_necessary(rx_entry, entry)) {
        return LLDP_TRUE;
    }
#endif

#ifdef VTSS_SW_OPTION_TSN
    if (lldp_is_frame_preemption_supported()) {
        if (fp_update_necessary(rx_entry, entry)) {
            return LLDP_TRUE;
        }
    }
#endif // VTSS_SW_OPTION_TSN

    /* everything the same */
    T_NG_PORT(TRACE_GRP_RX, rx_entry->receive_port, "%s", "... Current entry in remote table is up to date");
    return LLDP_FALSE;
}


static void delete_entry (lldp_rx_remote_entry_t   *rx_entry)
{
    T_DG_PORT(TRACE_GRP_CONF, rx_entry->receive_port, "%s", "deleting an entry");
    lldp_u8_t idx;
    /* try to find the existing index */
    idx = msap_id_idx(rx_entry);

    if (idx != MSAP_ID_IDX_UNKNOWN) {
        /* delete it */
        lldp_remote_clear_entry(idx);
    }

}

// Function for clearing the global counters
void lldp_mib_stats_clear(void)
{
    memset(&lldp_mib_stats, 0, sizeof(lldp_mib_stats));
}

static void mib_stats_inc_inserts (int entry_index)
{
    lldp_mib_stats.table_inserts++;
    T_DG_PORT(TRACE_GRP_RX, remote_entries[entry_index].receive_port,
              "New insert, table_insert = %d", lldp_mib_stats.table_inserts);

    // Entry shall be updated when new entry is inserted but it is already updated by the update_entry_mib_info function
    // lldp_entry_changed(&remote_entries[entry_index]);
}

static void mib_stats_inc_deletes (int entry_index)
{
    T_DG_PORT(TRACE_GRP_RX, remote_entries[entry_index].receive_port,
              "Delete entry, table_insert = %d", lldp_mib_stats.table_deletes);

    lldp_mib_stats.table_deletes++;
    // From 802.AB-2005 Page 70
    // "The value of sysUpTime object (defined in IETF RFC 3418)
    // at the time an entry is created, modified, or deleted in the
    // in tables associated with the lldpRemoteSystemsData objects
    // and all LLDP extension objects associated with remote systems.
    lldp_entry_changed(&remote_entries[entry_index]);

}

static void mib_stats_inc_drops ()
{
    lldp_mib_stats.table_drops++;
}

static void mib_stats_inc_ageouts (int entry_index)
{
    T_DG_PORT(TRACE_GRP_RX, remote_entries[entry_index].receive_port,
              "Age out, table_insert = %d", lldp_mib_stats.table_ageouts);
    lldp_mib_stats.table_ageouts++;
}

static lldp_u8_t compare_values (lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx, vtss_appl_lldp_remote_entry_t   *entry)
{
    if (entry->time_mark < time_mark) {
        return 1;
    } else if (entry->time_mark == time_mark) {
        if (entry->receive_port < port) {
            return 1;
        } else if (entry->receive_port == port) {
            if (entry->lldp_remote_index < remote_idx) {
                return 1;
            }
            if (entry->lldp_remote_index == remote_idx) {
                return 2;
            }
        }
    }

    return 0;
}

static lldp_u8_t get_next (lldp_u32_t time_mark, lldp_port_t port, lldp_u16_t remote_idx)
{
    lldp_u8_t i;
    lldp_u8_t idx_low = 0xFF;
    lldp_u32_t time_mark_low = ~0;
    lldp_port_t port_low = ~0;
    lldp_u16_t remote_idx_low = ~0;

    /* run through all entries */
    for (i = 0; i < LLDP_REMOTE_ENTRIES; i++) {
        if (remote_entries[i].in_use) {
            /* if entry is larger than what user supplied */
            if (compare_values(time_mark, port, remote_idx, &remote_entries[i]) == 0) {
                /* and lower than the current low */
                if (compare_values(time_mark_low, port_low, remote_idx_low, &remote_entries[i]) == 1) {
                    /* remember this index */
                    idx_low = i;
                    /* and update criteria */
                    time_mark_low  = remote_entries[i].time_mark;
                    port_low       = remote_entries[i].receive_port;
                    remote_idx_low = remote_entries[i].lldp_remote_index;
                }
            }
        }
    }

    return idx_low;
}
