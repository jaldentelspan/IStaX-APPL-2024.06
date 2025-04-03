/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "web_api.h"
#include "mac_api.h"
#include "port_api.h"
#include "mgmt_api.h"
#include "microchip/ethernet/switch/api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MAC
#include <vtss_trace_api.h>
/* ============== */
#define MAC_WEB_BUF_LEN 512
/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
//
// Static MAC TABLE handler
//
static void configure_learning(CYG_HTTPD_STATE *p, vtss_usid_t sid)
{
    mesa_port_no_t    iport;
    vtss_uport_no_t   uport;
    mesa_learn_mode_t learn_mode;
    char              form_name[32];
    size_t            len;
    mesa_rc           rc = VTSS_RC_OK;
    BOOL              call_learn_mode_set;
    const char        *id;
    char              var_id[16];
    char              tmp[200];
    char              *err = NULL;
    u32               port_count = port_count_max();

    learn_mode.cpu       = 0; // cpu not used.

    for (iport = VTSS_PORT_NO_START; iport < port_count; iport++) {
        uport = iport2uport(iport);

        call_learn_mode_set = TRUE;

        // Get the id of the radio button that is set
        sprintf(form_name, "learn_port_%d", uport);// Radio button
        id = cyg_httpd_form_varable_string(p, form_name, &len);
        T_N("form_name = %s, id = %s", form_name, id);

        // Check the ID to figure out which radio button that was set
        sprintf(var_id, "Learn_Auto_%d", uport);
        T_N("var_id = %s ", var_id);
        if (len == strlen(var_id) && memcmp(id, var_id, len) == 0) {
            T_N("Learning for iport %d is set to automatic", iport);
            learn_mode.automatic = 1;
            learn_mode.discard   = 0;
        } else {
            sprintf(var_id, "Learn_Secure_%d", uport); // Radio button
            if (len == strlen(var_id) && memcmp(id, var_id, len) == 0) {
                T_N("Learning for iport %d is set to secure", iport);
                learn_mode.automatic = 0;
                learn_mode.discard   = 1;
            } else {
                sprintf(var_id, "Learn_Disable_%d", uport); // Radio button
                if (len == strlen(var_id) && memcmp(id, var_id, len) == 0) {
                    // Learning disabled
                    T_N("Learning for iport %d is set to disable, var_id = %s", iport, var_id );
                    learn_mode.automatic = 0;
                    learn_mode.discard   = 0;
                } else {
                    // No radio-buttons are selected (which is impossible) or the port is not
                    // allowed to be changed by the user (possible due to Port Security module's force secure learning).
                    call_learn_mode_set = FALSE;
                }
            }
        }
        if (call_learn_mode_set) {
            rc = mac_mgmt_learn_mode_set(sid, iport, &learn_mode);
        }

        if (rc == MAC_ERROR_LEARN_FORCE_SECURE) {
            sprintf(tmp, "The learn mode cannot be changed on port %d while the learn mode is forced to 'secure' (probably by Port Security module)", uport);
            err = tmp;
        }
    }

    if (err != NULL) {
        send_custom_error(p, "MAC Error", err, strlen(err));
    }
}

/******************************************************************************/
// VLAN_WEB_vlan_list_get()
/******************************************************************************/
static BOOL MAC_WEB_vlan_list_get(CYG_HTTPD_STATE *p, const char *id, char big_storage[3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES], char small_storage[VLAN_VID_LIST_AS_STRING_LEN_BYTES], u8 result[VTSS_APPL_VLAN_BITMASK_LEN_BYTES])
{
    size_t found_len, cnt = 0;
    char   *found_ptr;

    if ((found_ptr = (char *)cyg_httpd_form_varable_string(p, id, &found_len)) != NULL) {

        // Unescape the string
        if (!cgi_unescape(found_ptr, big_storage, found_len, 3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES)) {
            T_E("Unable to unescape %s", found_ptr);
            return FALSE;
        }

        // Filter out white space
        found_ptr = big_storage;
        while (*found_ptr) {
            char c = *(found_ptr++);
            if (c != ' ') {
                small_storage[cnt++] = c;
            }
        }
    } else {
        T_E("Element with id = %s not found", id);
        return FALSE;
    }

    small_storage[cnt] = '\0';

    VTSS_ASSERT(cnt < VLAN_VID_LIST_AS_STRING_LEN_BYTES);

    if (mgmt_txt2bf(small_storage, result, VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX, 0) != VTSS_RC_OK) {
        T_E("Failed to convert %s to binary form", small_storage);
        return FALSE;
    }

    return TRUE;
}

static i32 handler_config_static_mac(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t        iport;
    int                   ct;
    mac_age_conf_t        real_age_timer;
    static mac_age_conf_t age_timer;
    mac_mgmt_addr_entry_t return_mac, new_entry;
    size_t                len;
    const char            *value = NULL;
    int                   i;
    mesa_vid_mac_t        search_mac, delete_entry;
    static mesa_rc        error_num = VTSS_RC_OK ; // Used to select an error message to be given back to the web page -- 0 = no error
    mesa_learn_mode_t     learn_mode;
    char                  form_name[32];
    uint                  vid;
    ulong                 form_lint_value = 0;
    uint                  entry_num;
    BOOL                  skip_deleting;
    u32                   port_count = port_count_max();
    u8                    vid_bitmask[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    char                  *unescaped_vlan_list_as_string = NULL, *escaped_vlan_list_as_string = NULL;
    u32                   psfp = 0;

    T_N ("Static MAC web access - SID =  %d", sid );

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MAC)) {
        return -1;
    }
#endif

    (void)mac_mgmt_age_time_get(&real_age_timer);
    vtss_clear(new_entry);

    if ( real_age_timer.mac_age_time != 0 ) {
        age_timer = real_age_timer;
    }

    // A couple of things comment to both POST and GET operations:
    if ((unescaped_vlan_list_as_string = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Alloc of %d bytes failed", VLAN_VID_LIST_AS_STRING_LEN_BYTES);
        goto free_unescaped_vids;
    }

    if ((escaped_vlan_list_as_string = (char *)VTSS_MALLOC(3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Alloc of %d bytes failed", 3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES);
        goto free_escaped_and_unescaped_vids;
    }


    if (p->method == CYG_HTTPD_METHOD_POST) {

        T_N ("p->method == CYG_HTTPD_METHOD_POST");

        if (!cyg_httpd_form_varable_find(p, "DisableAgeing") ) { // Continue if age timer is enabled
            // Get age timer from WEB (Form name = "agebox")
            if (cyg_httpd_form_varable_long_int(p, "agebox", &form_lint_value)) {
                age_timer.mac_age_time = (ulong ) form_lint_value;
                real_age_timer = age_timer;
            }

        } else {
            // Disable aging
            real_age_timer.mac_age_time = 0;
        }

        // Update age timer
        (void)mac_mgmt_age_time_set(&real_age_timer);

        // Configure learning
        configure_learning(p, sid);

        // Learning-disabled VLANs.
        if (!MAC_WEB_vlan_list_get(p, "vlans", escaped_vlan_list_as_string, unescaped_vlan_list_as_string, vid_bitmask)) {
            goto free_escaped_and_unescaped_vids;
        }

        T_D("vlans is %s", unescaped_vlan_list_as_string);
        if ((error_num = mac_mgmt_learning_disabled_vids_set(vid_bitmask)) != VTSS_RC_OK) {
            T_E("mac_mgmt_learning_disabled_vids_set fail");
            goto free_escaped_and_unescaped_vids;
        }

        // Delete entries
        entry_num = 1; // first entry
        sprintf(form_name, "MAC_%d", entry_num);// First MAC input box

        T_N("Looking for form: %s", form_name);
        while ((value = cyg_httpd_form_varable_string(p, form_name, &len)) && len > 0) {
            T_N("Form : %s -  found", form_name);
            skip_deleting = 0;

            sprintf(form_name, "Delete_%d", entry_num); // select next delete check box
            if (cyg_httpd_form_varable_find(p, form_name) ) {

                sprintf(form_name, "MAC_%d", entry_num);// Hidden MAC input box
                // Set mac address
                if (!cyg_httpd_form_varable_mac(p, form_name, delete_entry.mac.addr)) {
                    T_E("Hidden MAC value did not have the right format -- Shall never happen");
                    skip_deleting = 1; // Skip the deleting
                }

                // Set vid
                sprintf(form_name, "VID_%d", entry_num); // Hidden VID input box
                if (!cyg_httpd_form_varable_int(p, form_name, (int *)&vid)) {
                    T_E("Hidden entry (form name = %s) has wrong VID format -- Shall never happen", form_name);
                    skip_deleting = 1; // Skip the deleting
                    delete_entry.vid = 0;
                } else {
                    delete_entry.vid = vid;
                }

                T_D("Deleting entry %d mac address = %02x-%02x-%02x-%02x-%02x-%02x, vid = %d",
                    entry_num,
                    delete_entry.mac.addr[0], delete_entry.mac.addr[1], delete_entry.mac.addr[2],
                    delete_entry.mac.addr[3], delete_entry.mac.addr[4], delete_entry.mac.addr[5],
                    delete_entry.vid);

                if (!skip_deleting) {
                    // Do the deleting
                    (void)mac_mgmt_table_del(sid, &delete_entry, 0);
                }
            }

            entry_num++;  // Select next entry
            sprintf(form_name, "MAC_%d", entry_num);// Hidden MAC input box
            T_N("Updating hidden MAC input box. Entry = %d", entry_num);
        }

        //
        // Adding new entries to the table
        //
        for (entry_num = 1; entry_num <=  MAC_ADDR_NON_VOLATILE_MAX ; entry_num++) {
            // If an entry exists, then add it.
            sprintf(form_name, "MAC_%d", entry_num);// First MAC input box
            // Set mac address
            if (cyg_httpd_form_varable_mac(p, form_name, new_entry.vid_mac.mac.addr)) {
                // Set vid
                sprintf(form_name, "VID_%d", entry_num); // select next MAC input box
                if (!cyg_httpd_form_varable_int(p, form_name, (int *)&vid)) {
                    T_E("New entry (form name = %s) has wrong VID format -- Shall never happen", form_name);
                    vid = 0;
                }
                new_entry.vid_mac.vid = vid;

                // Add the port mask
                for (iport = VTSS_PORT_NO_START; iport < port_count; iport++) {
                    sprintf(form_name, "Dest_%d_%d", entry_num, iport2uport(iport)); // select port check box
                    if (cyg_httpd_form_varable_find(p, form_name)) { /* "on" if checked */
                        new_entry.destination[iport] = 1;
                    } else {
                        new_entry.destination[iport] = 0;
                    }
                }

                sprintf(form_name, "Delete_%d", entry_num); // Find corresponding check box

                // Skip the adding if the delete check box is checked
                if (!cyg_httpd_form_varable_find(p, form_name)) {
                    (void)mac_mgmt_table_del(sid, &new_entry.vid_mac, 0); // we don't care if the address exists or not
                    T_N (" Do the table adding");
                    VTSS_OS_MSLEEP(10);
                    if ((error_num = mac_mgmt_table_add(sid, &new_entry)) != VTSS_RC_OK) {
                        // give error message to web ( MAC table full ).
                        T_D("MAC Table full");
                    }
                }
            }
        } // end for loop

    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */

        char data_string[160], *buf;
        char learn_string[160] = "";
        char learn_chg_allowed_str[160] = "";
        BOOL chg_allowed;

        //
        // Learning configuration
        //
        for (iport = VTSS_PORT_NO_START; iport < port_count; iport++) {
            mac_mgmt_learn_mode_get(sid, iport, &learn_mode, &chg_allowed);
            learn_string[iport - VTSS_PORT_NO_START] = learn_mode.automatic ? 'A' : learn_mode.discard ? 'S' : 'D';
            learn_chg_allowed_str[iport - VTSS_PORT_NO_START] = chg_allowed ? '1' : '0';
        }
        learn_string[iport - VTSS_PORT_NO_START] = '\0';
        learn_chg_allowed_str[iport - VTSS_PORT_NO_START] = '\0';

        (void)mac_mgmt_learning_disabled_vids_get(vid_bitmask);
        (void)cgi_escape(vlan_mgmt_vid_bitmask_to_txt(vid_bitmask, unescaped_vlan_list_as_string), escaped_vlan_list_as_string);
        cyg_httpd_start_chunked("html");

        // General setup
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%s/%s/%s/%s/%d/%d|",
                      age_timer.mac_age_time,
                      real_age_timer.mac_age_time == 0 ? 1 : 0,
                      learn_string,
                      learn_chg_allowed_str,
                      escaped_vlan_list_as_string,
                      error_num == VTSS_RC_OK ? "-" : error_txt(error_num),
                      0,
                      psfp);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        // Clear error messages
        error_num = VTSS_RC_OK;

        // Get entries
        memset(&search_mac, 0, sizeof(search_mac)); // Set search starting point.

        for (i = 0 ; i < MAC_ADDR_NON_VOLATILE_MAX; i++ ) {
            T_D("Next lookup %u", i);
            // We want to include the start mac address so first entry must be found using a lookup
            if (i == 0) {
                // Do a lookup for the first entry
                if (mac_mgmt_static_get_next(sid, &search_mac, &return_mac, FALSE, FALSE) != VTSS_RC_OK) {
                    // If lookup wasn't found do a lookup of the next entry
                    T_N("Did not find lookup, sid = %d", sid);
                    if (mac_mgmt_static_get_next(sid, &search_mac, &return_mac, TRUE, FALSE) != VTSS_RC_OK) {
                        T_N("Did not find any entries for sid: %d, search_mac = %s", sid, data_string);
                        break;
                    }
                }
            } else {
                // Find next entry
                if (mac_mgmt_static_get_next(sid, &search_mac, &return_mac, TRUE, FALSE) != VTSS_RC_OK) {
                    T_N("Did not find any more entries for sid: %d, search_mac = %s", sid, data_string);
                    break;
                }
            }

            search_mac = return_mac.vid_mac; // Point to next entry
            buf = data_string;
            buf += sprintf(buf, "%02X-%02X-%02X-%02X-%02X-%02X/%u",
                           return_mac.vid_mac.mac.addr[0],
                           return_mac.vid_mac.mac.addr[1],
                           return_mac.vid_mac.mac.addr[2],
                           return_mac.vid_mac.mac.addr[3],
                           return_mac.vid_mac.mac.addr[4],
                           return_mac.vid_mac.mac.addr[5],
                           return_mac.vid_mac.vid);

            for (iport = VTSS_PORT_NO_START; iport < port_count; iport++) {
                buf += sprintf(buf, "/%u", return_mac.destination.get(iport));
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", data_string);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
        (void)value;
    }

free_escaped_and_unescaped_vids:
    VTSS_FREE(escaped_vlan_list_as_string);
free_unescaped_vids:
    VTSS_FREE(unescaped_vlan_list_as_string);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        // Return to the mac page when update completed.
        redirect(p, "/mac.htm");
    }
    return -1; // Do not further search the file system.
}

//
// Dynamic MAC TABLE handler
//
static i32 handler_config_dynamic_mac(CYG_HTTPD_STATE *p)
{
    static BOOL            first_time = 1;
    static mesa_vid_mac_t  start_mac_addr;
    static int             num_of_entries;

    vtss_isid_t            sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t         iport;
    int                    ct;
    mesa_vid_mac_t         search_mac;
    mesa_mac_table_entry_t return_mac;
    uint                   mac_addr[6];
    int                    i;
    BOOL                   no_entries_found = 1;
    u32                    psfp = 0;

    T_N("Dynamic MAC web access - SID =  %d", sid);

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MAC)) {
        return -1;
    }
#endif

    if (first_time) {
        first_time = 0 ;
        // Clear search mac and vid
        memset(&start_mac_addr, 0, sizeof(start_mac_addr));
        num_of_entries = 20;
        start_mac_addr.vid = 1;
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        // Return to the mac page when update completed.
        redirect(p, "/mac.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        const char              *var_string;
        size_t                  var_len;
        int                     start_vid;

        T_D("Updating to dynamic MAC table");

        cyg_httpd_start_chunked("html");

        // Check if we shall flush
        if (cyg_httpd_form_varable_find(p, "Flush")) {
            T_D ("Flushing MAC table");
            (void)mac_mgmt_table_flush();
        }

        // Check if the get next entries button is pressed
        if ((var_string = cyg_httpd_form_varable_string(p, "DynGetNextAddr", &var_len))) {
            // Store the start mac address
            // Convert from string to mesa_mac_t
            if  (sscanf(var_string, "%2x-%2x-%2x-%2x-%2x-%2x", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) == 6 ) {
                for (i = 0 ; i <= 5; i++) {
                    start_mac_addr.mac.addr[i] = (uchar) mac_addr[i];
                }
                T_D ("Start MAC Address set to %s via web", var_string);
            }

            // Get  number of entries per page
            if (!cyg_httpd_form_varable_int(p, "DynNumberOfEntries", &num_of_entries) ||
                num_of_entries <= 0 || num_of_entries > 999 ) {
                T_E("number of entries has wrong format (Shall never happen), %u", num_of_entries);
                num_of_entries = 999;
            }

            // Get start vid
            if (!cyg_httpd_form_varable_int(p, "DynStartVid", &start_vid) ||
                start_vid < 1 || start_vid > 4095 ) {
                T_E("VID has wrong format (Shall never happen), %u", start_vid);
                start_vid = 1;
            }
            start_mac_addr.vid = start_vid;

            T_D("number of entries set to %u ", num_of_entries);
        }



        // General setup
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%02x-%02x-%02x-%02x-%02x-%02x/%d/%d|",
                      num_of_entries,
                      start_mac_addr.mac.addr[0],
                      start_mac_addr.mac.addr[1],
                      start_mac_addr.mac.addr[2],
                      start_mac_addr.mac.addr[3],
                      start_mac_addr.mac.addr[4],
                      start_mac_addr.mac.addr[5],
                      0,
                      psfp);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        // Get entries
        search_mac = start_mac_addr; // Set search starting point.

        for (i = 0 ; i < num_of_entries; i++ ) {
            T_D("search_mac_addr = %02x-%02x-%02x-%02x-%02x-%02x, vid = %u, sid = %d",
                search_mac.mac.addr[0],
                search_mac.mac.addr[1],
                search_mac.mac.addr[2],
                search_mac.mac.addr[3],
                search_mac.mac.addr[4],
                search_mac.mac.addr[5],
                search_mac.vid,
                sid);

            if (i ==  0 && cyg_httpd_form_varable_find(p, "DynNumberOfEntries") == NULL) {
                // We want to include the start mac address so first entry must be found using a lookup if it is the refresh button that are pressed.

                // Do a lookup for the first entry
                if (mac_mgmt_table_get_next(sid, &search_mac, &return_mac, FALSE) != VTSS_RC_OK) {
                    // If lookup wasn't found do a lookup of the next entry
                    if (mac_mgmt_table_get_next(sid, &search_mac, &return_mac, TRUE) != VTSS_RC_OK) {
                        break ;
                    }
                }
            } else {
                // Find next entry
                if (mac_mgmt_table_get_next(sid, &search_mac, &return_mac, TRUE) != VTSS_RC_OK) {
                    break ;
                }
            }

            no_entries_found = 0;   // If we reached this point at least one entry is found.
            search_mac = return_mac.vid_mac; // Point to next entry

            // Generate entry
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%02x-%02x-%02x-%02x-%02x-%02x/%u/%u/%u",
                          return_mac.vid_mac.mac.addr[0],
                          return_mac.vid_mac.mac.addr[1],
                          return_mac.vid_mac.mac.addr[2],
                          return_mac.vid_mac.mac.addr[3],
                          return_mac.vid_mac.mac.addr[4],
                          return_mac.vid_mac.mac.addr[5],
                          return_mac.vid_mac.vid,
                          return_mac.locked,
                          return_mac.copy_to_cpu);

            cyg_httpd_write_chunked(p->outbuffer, ct);
            T_D ("Return mac address = %s ", p->outbuffer);
            // Add destination
            char *buf = p->outbuffer;
            for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
                buf += snprintf(buf, sizeof(p->outbuffer) - (buf - p->outbuffer), "/%u", return_mac.destination.get(iport));
            }

            *buf++ = '|';
            cyg_httpd_write_chunked(p->outbuffer, (buf - p->outbuffer));
        }

        // Signal to WEB that no entries was found.
        if (no_entries_found) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "NoEntries/-/-/-|");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t mac_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[MAC_WEB_BUF_LEN];
    (void) snprintf(buff, MAC_WEB_BUF_LEN,
                    "var configMacStaticMax = %d;\n"
                    "var configMacAddressMax = %d;\n",
                    MAC_ADDR_NON_VOLATILE_MAX,
                    fast_cap(MESA_CAP_L2_MAC_ADDR_CNT));
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(mac_lib_config_js);


/****************************************************************************/
/*  HTTPD Handler Table Entries                                             */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_static_mac_table, "/config/static_mac_table", handler_config_static_mac);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_dynamic_mac_table, "/config/dynamic_mac_table", handler_config_dynamic_mac);

